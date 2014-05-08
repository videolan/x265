/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#include "TLibCommon/NAL.h"
#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibCommon/TComRom.h"
#include "primitives.h"
#include "threadpool.h"
#include "common.h"
#include "param.h"

#include "TLibEncoder/NALwrite.h"
#include "bitcost.h"
#include "encoder.h"
#include "slicetype.h"
#include "frameencoder.h"
#include "ratecontrol.h"
#include "dpb.h"

#include "x265.h"

#if HAVE_INT_TYPES_H
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#define LL "%" PRIu64
#else
#define LL "%lld"
#endif

using namespace x265;

Encoder::Encoder()
{
    m_aborted = false;
    m_encodedFrameNum = 0;
    m_pocLast = -1;
    m_maxRefPicNum = 0;
    m_curEncoder = 0;
    m_numLumaWPFrames = 0;
    m_numChromaWPFrames = 0;
    m_numLumaWPBiFrames = 0;
    m_numChromaWPBiFrames = 0;
    m_lookahead = NULL;
    m_frameEncoder = NULL;
    m_rateControl = NULL;
    m_dpb = NULL;
    m_nals = NULL;
    m_packetData = NULL;
    m_outputCount = 0;
    m_csvfpt = NULL;
    param = NULL;

#if ENC_DEC_TRACE
    g_hTrace = fopen("TraceEnc.txt", "wb");
    g_bJustDoIt = g_bEncDecTraceDisable;
    g_nSymbolCounter = 0;
#endif
}

Encoder::~Encoder()
{
#if ENC_DEC_TRACE
    fclose(g_hTrace);
#endif
}

void Encoder::create()
{
    if (!primitives.sad[0])
    {
        // this should be an impossible condition when using our public API, and indicates a serious bug.
        x265_log(param, X265_LOG_ERROR, "Primitives must be initialized before encoder is created\n");
        abort();
    }

    m_frameEncoder = new FrameEncoder[param->frameNumThreads];
    if (m_frameEncoder)
    {
        for (int i = 0; i < param->frameNumThreads; i++)
        {
            m_frameEncoder[i].setThreadPool(m_threadPool);
        }
    }
    m_lookahead = new Lookahead(this, m_threadPool);
    m_dpb = new DPB(this);
    m_rateControl = new RateControl(this);

    /* Try to open CSV file handle */
    if (param->csvfn)
    {
        m_csvfpt = fopen(param->csvfn, "r");
        if (m_csvfpt)
        {
            // file already exists, re-open for append
            fclose(m_csvfpt);
            m_csvfpt = fopen(param->csvfn, "ab");
        }
        else
        {
            // new CSV file, write header
            m_csvfpt = fopen(param->csvfn, "wb");
            if (m_csvfpt)
            {
                if (param->logLevel >= X265_LOG_DEBUG)
                {
                    fprintf(m_csvfpt, "Encode Order, Type, POC, QP, Bits, ");
                    if (param->rc.rateControlMode == X265_RC_CRF)
                        fprintf(m_csvfpt, "RateFactor, ");
                    fprintf(m_csvfpt, "Y PSNR, U PSNR, V PSNR, YUV PSNR, SSIM, SSIM (dB), Encoding time, Elapsed time, List 0, List 1\n");
                }
                else
                    fprintf(m_csvfpt, "Command, Date/Time, Elapsed Time, FPS, Bitrate, Y PSNR, U PSNR, V PSNR, Global PSNR, SSIM, SSIM (dB), Version\n");
            }
        }
    }
}

void Encoder::destroy()
{
    if (m_frameEncoder)
    {
        for (int i = 0; i < param->frameNumThreads; i++)
        {
            // Ensure frame encoder is idle before destroying it
            m_frameEncoder[i].getEncodedPicture(NULL);
            m_frameEncoder[i].destroy();
        }

        delete [] m_frameEncoder;
    }

    while (!m_freeList.empty())
    {
        TComPic* pic = m_freeList.popFront();
        pic->destroy();
        delete pic;
    }

    if (m_lookahead)
    {
        m_lookahead->destroy();
        delete m_lookahead;
    }

    delete m_dpb;
    delete m_rateControl;

    // thread pool release should always happen last
    if (m_threadPool)
        m_threadPool->release();

    X265_FREE(m_nals);
    X265_FREE(m_packetData);
    X265_FREE(param);
    if (m_csvfpt)
        fclose(m_csvfpt);
}

void Encoder::init()
{
    if (m_frameEncoder)
    {
        int numRows = (param->sourceHeight + g_maxCUSize - 1) / g_maxCUSize;
        for (int i = 0; i < param->frameNumThreads; i++)
        {
            if (!m_frameEncoder[i].init(this, numRows))
            {
                x265_log(param, X265_LOG_ERROR, "Unable to initialize frame encoder, aborting\n");
                m_aborted = true;
            }
        }
    }
    m_lookahead->init();
    m_encodeStartTime = x265_mdate();
}

int Encoder::getStreamHeaders(NALUnitEBSP **nalunits)
{
    return m_frameEncoder->getStreamHeaders(nalunits);
}

void Encoder::updateVbvPlan(RateControl* rc)
{
    int encIdx, curIdx;

    curIdx = (m_curEncoder + param->frameNumThreads - 1) % param->frameNumThreads;
    encIdx = (curIdx + 1) % param->frameNumThreads;
    while (encIdx != curIdx)
    {
        FrameEncoder *encoder = &m_frameEncoder[encIdx];
        double bits;
        bits = encoder->m_rce.frameSizePlanned;
        if (!encoder->m_rce.isActive)
        {
            encIdx = (encIdx + 1) % param->frameNumThreads;
            continue;
        }
        bits = X265_MAX(bits, encoder->m_rce.frameSizeEstimated);
        rc->bufferFill -= bits;
        rc->bufferFill = X265_MAX(rc->bufferFill, 0);
        rc->bufferFill += encoder->m_rce.bufferRate;
        rc->bufferFill = X265_MIN(rc->bufferFill, rc->bufferSize);
        encIdx = (encIdx + 1) % param->frameNumThreads;
    }
}

#define VERBOSE_RATE 0
#if VERBOSE_RATE
static const char* nalUnitTypeToString(NalUnitType type)
{
    switch (type)
    {
    case NAL_UNIT_CODED_SLICE_TRAIL_R:    return "TRAIL_R";
    case NAL_UNIT_CODED_SLICE_TRAIL_N:    return "TRAIL_N";
    case NAL_UNIT_CODED_SLICE_TLA_R:      return "TLA_R";
    case NAL_UNIT_CODED_SLICE_TSA_N:      return "TSA_N";
    case NAL_UNIT_CODED_SLICE_STSA_R:     return "STSA_R";
    case NAL_UNIT_CODED_SLICE_STSA_N:     return "STSA_N";
    case NAL_UNIT_CODED_SLICE_BLA_W_LP:   return "BLA_W_LP";
    case NAL_UNIT_CODED_SLICE_BLA_W_RADL: return "BLA_W_RADL";
    case NAL_UNIT_CODED_SLICE_BLA_N_LP:   return "BLA_N_LP";
    case NAL_UNIT_CODED_SLICE_IDR_W_RADL: return "IDR_W_RADL";
    case NAL_UNIT_CODED_SLICE_IDR_N_LP:   return "IDR_N_LP";
    case NAL_UNIT_CODED_SLICE_CRA:        return "CRA";
    case NAL_UNIT_CODED_SLICE_RADL_R:     return "RADL_R";
    case NAL_UNIT_CODED_SLICE_RASL_R:     return "RASL_R";
    case NAL_UNIT_VPS:                    return "VPS";
    case NAL_UNIT_SPS:                    return "SPS";
    case NAL_UNIT_PPS:                    return "PPS";
    case NAL_UNIT_ACCESS_UNIT_DELIMITER:  return "AUD";
    case NAL_UNIT_EOS:                    return "EOS";
    case NAL_UNIT_EOB:                    return "EOB";
    case NAL_UNIT_FILLER_DATA:            return "FILLER";
    case NAL_UNIT_PREFIX_SEI:             return "SEI";
    case NAL_UNIT_SUFFIX_SEI:             return "SEI";
    default:                              return "UNK";
    }
}

#endif // if VERBOSE_RATE

/**
 \param   flush               force encoder to encode a frame
 \param   pic_in              input original YUV picture or NULL
 \param   pic_out             pointer to reconstructed picture struct
 \param   nalunits            output NAL packets
 \retval                      number of encoded pictures
 */
int Encoder::encode(bool flush, const x265_picture* pic_in, x265_picture *pic_out, NALUnitEBSP **nalunits)
{
    if (m_aborted)
        return -1;

    if (pic_in)
    {
        if (pic_in->colorSpace != param->internalCsp)
        {
            x265_log(param, X265_LOG_ERROR, "Unsupported color space (%d) on input\n",
                     pic_in->colorSpace);
            return -1;
        }
        if (pic_in->bitDepth < 8 || pic_in->bitDepth > 16)
        {
            x265_log(param, X265_LOG_ERROR, "Input bit depth (%d) must be between 8 and 16\n",
                     pic_in->bitDepth);
            return -1;
        }

        TComPic *pic;
        if (m_freeList.empty())
        {
            pic = new TComPic;
            if (!pic || !pic->create(this))
            {
                m_aborted = true;
                x265_log(param, X265_LOG_ERROR, "memory allocation failure, aborting encode\n");
                if (pic)
                {
                    pic->destroy();
                    delete pic;
                }
                return -1;
            }
            if (param->bEnableSAO)
            {
                // TODO: these should be allocated on demand within the encoder
                // NOTE: the SAO pointer from m_frameEncoder for read m_maxSplitLevel, etc, we can remove it later
                pic->getPicSym()->allocSaoParam(m_frameEncoder->getSAO());
            }
        }
        else
            pic = m_freeList.popBack();
        /* Copy input picture into a TComPic, send to lookahead */
        pic->getSlice()->setPOC(++m_pocLast);
        pic->reInit(this);
        pic->getPicYuvOrg()->copyFromPicture(*pic_in, m_pad);
        pic->m_userData = pic_in->userData;
        pic->m_pts = pic_in->pts;
        pic->m_forceqp = pic_in->forceqp;

        if (m_pocLast == 0)
            m_firstPts = pic->m_pts;
        if (m_bframeDelay && m_pocLast == m_bframeDelay)
            m_bframeDelayTime = pic->m_pts - m_firstPts;

        // Encoder holds a reference count until collecting stats
        ATOMIC_INC(&pic->m_countRefEncoders);
        bool bEnableWP = param->bEnableWeightedPred || param->bEnableWeightedBiPred;
        if (param->rc.aqMode || bEnableWP)
            m_rateControl->calcAdaptiveQuantFrame(pic);
        m_lookahead->addPicture(pic, pic_in->sliceType);
    }

    if (flush)
        m_lookahead->flush();

    FrameEncoder *curEncoder = &m_frameEncoder[m_curEncoder];
    m_curEncoder = (m_curEncoder + 1) % param->frameNumThreads;
    int ret = 0;

    // getEncodedPicture() should block until the FrameEncoder has completed
    // encoding the frame.  This is how back-pressure through the API is
    // accomplished when the encoder is full.
    TComPic *out = curEncoder->getEncodedPicture(nalunits);

    if (!out && flush)
    {
        // if the current encoder did not return an output picture and we are
        // flushing, check all the other encoders in logical order until
        // we find an output picture or have cycled around.  We cannot return
        // 0 until the entire stream is flushed
        // (can only be an issue when --frames < --frame-threads)
        int flushed = m_curEncoder;
        do
        {
            curEncoder = &m_frameEncoder[m_curEncoder];
            m_curEncoder = (m_curEncoder + 1) % param->frameNumThreads;
            out = curEncoder->getEncodedPicture(nalunits);
        }
        while (!out && flushed != m_curEncoder);
    }
    if (out)
    {
        if (pic_out)
        {
            TComPicYuv *recpic = out->getPicYuvRec();
            pic_out->poc = out->getSlice()->getPOC();
            pic_out->bitDepth = X265_DEPTH;
            pic_out->userData = out->m_userData;
            pic_out->colorSpace = param->internalCsp;

            pic_out->pts = out->m_pts;
            pic_out->dts = out->m_dts;

            switch (out->getSlice()->getSliceType())
            {
            case I_SLICE:
                pic_out->sliceType = out->m_lowres.bKeyframe ? X265_TYPE_IDR : X265_TYPE_I;
                break;
            case P_SLICE:
                pic_out->sliceType = X265_TYPE_P;
                break;
            case B_SLICE:
                pic_out->sliceType = X265_TYPE_B;
                break;
            }

            pic_out->planes[0] = recpic->getLumaAddr();
            pic_out->stride[0] = recpic->getStride() * sizeof(pixel);
            pic_out->planes[1] = recpic->getCbAddr();
            pic_out->stride[1] = recpic->getCStride() * sizeof(pixel);
            pic_out->planes[2] = recpic->getCrAddr();
            pic_out->stride[2] = recpic->getCStride() * sizeof(pixel);
        }

        if (out->getSlice()->getSliceType() == P_SLICE)
        {
            if (out->getSlice()->m_weightPredTable[0][0][0].bPresentFlag)
                m_numLumaWPFrames++;
            if (out->getSlice()->m_weightPredTable[0][0][1].bPresentFlag ||
                out->getSlice()->m_weightPredTable[0][0][2].bPresentFlag)
                m_numChromaWPFrames++;
        }
        else if (out->getSlice()->getSliceType() == B_SLICE)
        {
            bool bLuma = false, bChroma = false;
            for (int l = 0; l < 2; l++)
            {
                if (out->getSlice()->m_weightPredTable[l][0][0].bPresentFlag)
                    bLuma = true;
                if (out->getSlice()->m_weightPredTable[l][0][1].bPresentFlag ||
                    out->getSlice()->m_weightPredTable[l][0][2].bPresentFlag)
                    bChroma = true;
            }

            if (bLuma)
                m_numLumaWPBiFrames++;
            if (bChroma)
                m_numChromaWPBiFrames++;
        }

        /* calculate the size of the access unit, excluding:
         *  - any AnnexB contributions (start_code_prefix, zero_byte, etc.,)
         *  - SEI NAL units
         */
        uint32_t numRBSPBytes = 0;
        for (int count = 0; nalunits[count] != NULL; count++)
        {
            uint32_t numRBSPBytes_nal = nalunits[count]->m_packetSize;
#if VERBOSE_RATE
            printf("*** %6s numBytesInNALunit: %u\n", nalUnitTypeToString(nalunits[count]->m_nalUnitType), numRBSPBytes_nal);
#endif
            if (nalunits[count]->m_nalUnitType != NAL_UNIT_PREFIX_SEI && nalunits[count]->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
            {
                numRBSPBytes += numRBSPBytes_nal;
            }
        }

        uint64_t bits = numRBSPBytes * 8;
        m_rateControl->rateControlEnd(out, bits, &curEncoder->m_rce);
        finishFrameStats(out, curEncoder, bits);

        // Allow this frame to be recycled if no frame encoders are using it for reference
        ATOMIC_DEC(&out->m_countRefEncoders);
        m_dpb->recycleUnreferenced(m_freeList);
        ret = 1;
    }

    // pop a single frame from decided list, then provide to frame encoder
    // curEncoder is guaranteed to be idle at this point
    TComPic* fenc = m_lookahead->getDecidedPicture();
    if (fenc)
    {
        m_encodedFrameNum++;
        if (m_bframeDelay)
        {
            int64_t *prevReorderedPts = m_prevReorderedPts;
            fenc->m_dts = m_encodedFrameNum > m_bframeDelay
                ? prevReorderedPts[(m_encodedFrameNum - m_bframeDelay) % m_bframeDelay]
                : fenc->m_reorderedPts - m_bframeDelayTime;
            prevReorderedPts[m_encodedFrameNum % m_bframeDelay] = fenc->m_reorderedPts;
        }
        else
            fenc->m_dts = fenc->m_reorderedPts;

        // Initialize slice for encoding with this FrameEncoder
        curEncoder->initSlice(fenc);

        // determine references, setup RPS, etc
        m_dpb->prepareEncode(fenc);

        // set slice QP
        m_rateControl->rateControlStart(fenc, m_lookahead, &curEncoder->m_rce, this);

        // Allow FrameEncoder::compressFrame() to start in a worker thread
        curEncoder->m_enable.trigger();
    }

    return ret;
}

void EncStats::addPsnr(double psnrY, double psnrU, double psnrV)
{
    m_psnrSumY += psnrY;
    m_psnrSumU += psnrU;
    m_psnrSumV += psnrV;
}

void EncStats::addBits(uint64_t bits)
{
    m_accBits += bits;
    m_numPics++;
}

void EncStats::addSsim(double ssim)
{
    m_globalSsim += ssim;
}

void EncStats::addQP(double aveQp)
{
    m_totalQp += aveQp;
}

char* Encoder::statsString(EncStats& stat, char* buffer)
{
    double fps = (double)param->fpsNum / param->fpsDenom;
    double scale = fps / 1000 / (double)stat.m_numPics;

    int len = sprintf(buffer, "%-6d ", stat.m_numPics);

    len += sprintf(buffer + len, "Avg QP:%2.2lf", stat.m_totalQp / (double)stat.m_numPics);
    len += sprintf(buffer + len, "  kb/s: %-8.2lf", stat.m_accBits * scale);
    if (param->bEnablePsnr)
    {
        len += sprintf(buffer + len, "  PSNR Mean: Y:%.3lf U:%.3lf V:%.3lf",
                       stat.m_psnrSumY / (double)stat.m_numPics,
                       stat.m_psnrSumU / (double)stat.m_numPics,
                       stat.m_psnrSumV / (double)stat.m_numPics);
    }
    if (param->bEnableSsim)
    {
        sprintf(buffer + len, "  SSIM Mean: %.6lf (%.3lfdB)",
                stat.m_globalSsim / (double)stat.m_numPics,
                x265_ssim2dB(stat.m_globalSsim / (double)stat.m_numPics));
    }
    return buffer;
}

void Encoder::printSummary()
{
#if LOG_CU_STATISTICS
    for (int sliceType = 2; sliceType >= 0; sliceType--)
    {
        if (sliceType == P_SLICE && !m_analyzeP.m_numPics)
            continue;
        if (sliceType == B_SLICE && !m_analyzeB.m_numPics)
            continue;

        StatisticLog finalLog;
        for (int depth = 0; depth < (int)g_maxCUDepth; depth++)
        {
            for (int j = 0; j < param->frameNumThreads; j++)
            {
                for (int row = 0; row < m_frameEncoder[0].m_numRows; row++)
                {
                    StatisticLog& enclog = m_frameEncoder[j].m_rows[row].m_cuCoder.m_sliceTypeLog[sliceType];
                    if (depth == 0)
                        finalLog.totalCu += enclog.totalCu;
                    finalLog.cntIntra[depth] += enclog.cntIntra[depth];
                    for (int m = 0; m < INTER_MODES; m++)
                    {
                        if (m < INTRA_MODES)
                        {
                            finalLog.cuIntraDistribution[depth][m] += enclog.cuIntraDistribution[depth][m];
                        }
                        finalLog.cuInterDistribution[depth][m] += enclog.cuInterDistribution[depth][m];
                    }

                    if (depth == (int)g_maxCUDepth - 1)
                        finalLog.cntIntraNxN += enclog.cntIntraNxN;
                    if (sliceType != I_SLICE)
                    {
                        finalLog.cntTotalCu[depth] += enclog.cntTotalCu[depth];
                        finalLog.cntInter[depth] += enclog.cntInter[depth];
                        finalLog.cntSkipCu[depth] += enclog.cntSkipCu[depth];
                    }
                }
            }

            uint64_t cntInter, cntSkipCu, cntIntra = 0, cntIntraNxN = 0, encCu = 0;
            uint64_t cuInterDistribution[INTER_MODES], cuIntraDistribution[INTRA_MODES];

            // check for 0/0, if true assign 0 else calculate percentage
            for (int n = 0; n < INTER_MODES; n++)
            {
                if (finalLog.cntInter[depth] == 0)
                    cuInterDistribution[n] = 0;
                else
                    cuInterDistribution[n] = (finalLog.cuInterDistribution[depth][n] * 100) / finalLog.cntInter[depth];
                if (n < INTRA_MODES)
                {
                    if (finalLog.cntIntra[depth] == 0)
                    {
                        cntIntraNxN = 0;
                        cuIntraDistribution[n] = 0;
                    }
                    else
                    {
                        cntIntraNxN = (finalLog.cntIntraNxN * 100) / finalLog.cntIntra[depth];
                        cuIntraDistribution[n] = (finalLog.cuIntraDistribution[depth][n] * 100) / finalLog.cntIntra[depth];
                    }
                }
            }

            if (finalLog.totalCu == 0)
            {
                encCu = 0;
            }
            else
            {
                if (sliceType == I_SLICE)
                {
                    cntIntra = (finalLog.cntIntra[depth] * 100) / finalLog.totalCu;
                    cntIntraNxN = (finalLog.cntIntraNxN * 100) / finalLog.totalCu;
                }
                else
                {
                    encCu = ((finalLog.cntIntra[depth] + finalLog.cntInter[depth]) * 100) / finalLog.totalCu;
                }
            }
            if (sliceType == I_SLICE)
            {
                cntInter = 0;
                cntSkipCu = 0;
            }
            else
            {
                if (finalLog.cntTotalCu[depth] == 0)
                {
                    cntInter = 0;
                    cntIntra = 0;
                    cntSkipCu = 0;
                }
                else
                {
                    cntInter = (finalLog.cntInter[depth] * 100) / finalLog.cntTotalCu[depth];
                    cntIntra = (finalLog.cntIntra[depth] * 100) / finalLog.cntTotalCu[depth];
                    cntSkipCu = (finalLog.cntSkipCu[depth] * 100) / finalLog.cntTotalCu[depth];
                }
            }
            // print statistics
            int cuSize = g_maxCUSize >> depth;
            char stats[256] = { 0 };
            int len = 0;
            if (sliceType != I_SLICE)
            {
                len += sprintf(stats + len, "EncCU "LL "%% Merge "LL "%%", encCu, cntSkipCu);
            }
            if (cntInter)
            {
                len += sprintf(stats + len, " Inter "LL "%%", cntInter);
                if (param->bEnableAMP)
                    len += sprintf(stats + len, "(%dx%d "LL "%% %dx%d "LL "%% %dx%d "LL "%% AMP "LL "%%)",
                                   cuSize, cuSize, cuInterDistribution[0],
                                   cuSize / 2, cuSize, cuInterDistribution[2],
                                   cuSize, cuSize / 2, cuInterDistribution[1],
                                   cuInterDistribution[3]);
                else if (param->bEnableRectInter)
                    len += sprintf(stats + len, "(%dx%d "LL "%% %dx%d "LL "%% %dx%d "LL "%%)",
                                   cuSize, cuSize, cuInterDistribution[0],
                                   cuSize / 2, cuSize, cuInterDistribution[2],
                                   cuSize, cuSize / 2, cuInterDistribution[1]);
            }
            if (cntIntra)
            {
                len += sprintf(stats + len, " Intra "LL "%%(DC "LL "%% P "LL "%% Ang "LL "%%",
                               cntIntra, cuIntraDistribution[0],
                               cuIntraDistribution[1], cuIntraDistribution[2]);
                if (sliceType != I_SLICE)
                {
                    if (depth == (int)g_maxCUDepth - 1)
                        len += sprintf(stats + len, " %dx%d "LL "%%", cuSize / 2, cuSize / 2, cntIntraNxN);
                }

                len += sprintf(stats + len, ")");
                if (sliceType == I_SLICE)
                {
                    if (depth == (int)g_maxCUDepth - 1)
                        len += sprintf(stats + len, " %dx%d: "LL "%%", cuSize / 2, cuSize / 2, cntIntraNxN);
                }
            }
            const char slicechars[] = "BPI";
            if (stats[0])
                x265_log(param, X265_LOG_INFO, "%c%-2d: %s\n", slicechars[sliceType], cuSize, stats);
        }
    }

#endif // if LOG_CU_STATISTICS
    if (param->logLevel >= X265_LOG_INFO)
    {
        char buffer[200];
        if (m_analyzeI.m_numPics)
            x265_log(param, X265_LOG_INFO, "frame I: %s\n", statsString(m_analyzeI, buffer));
        if (m_analyzeP.m_numPics)
            x265_log(param, X265_LOG_INFO, "frame P: %s\n", statsString(m_analyzeP, buffer));
        if (m_analyzeB.m_numPics)
            x265_log(param, X265_LOG_INFO, "frame B: %s\n", statsString(m_analyzeB, buffer));
        if (m_analyzeAll.m_numPics)
            x265_log(param, X265_LOG_INFO, "global : %s\n", statsString(m_analyzeAll, buffer));
        if (param->bEnableWeightedPred && m_analyzeP.m_numPics)
        {
            x265_log(param, X265_LOG_INFO, "Weighted P-Frames: Y:%.1f%% UV:%.1f%%\n",
                     (float)100.0 * m_numLumaWPFrames / m_analyzeP.m_numPics,
                     (float)100.0 * m_numChromaWPFrames / m_analyzeP.m_numPics);
        }
        if (param->bEnableWeightedBiPred && m_analyzeB.m_numPics)
        {
            x265_log(param, X265_LOG_INFO, "Weighted B-Frames: Y:%.1f%% UV:%.1f%%\n",
                     (float)100.0 * m_numLumaWPBiFrames / m_analyzeB.m_numPics,
                     (float)100.0 * m_numChromaWPBiFrames / m_analyzeB.m_numPics);
        }
        int pWithB = 0;
        for (int i = 0; i <= param->bframes; i++)
        {
            pWithB += m_lookahead->histogram[i];
        }

        if (pWithB)
        {
            int p = 0;
            for (int i = 0; i <= param->bframes; i++)
            {
                p += sprintf(buffer + p, "%.1f%% ", 100. * m_lookahead->histogram[i] / pWithB);
            }

            x265_log(param, X265_LOG_INFO, "consecutive B-frames: %s\n", buffer);
        }
    }
}

void Encoder::fetchStats(x265_stats *stats, size_t statsSizeBytes)
{
    if (statsSizeBytes >= sizeof(stats))
    {
        stats->globalPsnrY = m_analyzeAll.m_psnrSumY;
        stats->globalPsnrU = m_analyzeAll.m_psnrSumU;
        stats->globalPsnrV = m_analyzeAll.m_psnrSumV;
        stats->encodedPictureCount = m_analyzeAll.m_numPics;
        stats->totalWPFrames = m_numLumaWPFrames;
        stats->accBits = m_analyzeAll.m_accBits;
        stats->elapsedEncodeTime = (double)(x265_mdate() - m_encodeStartTime) / 1000000;
        if (stats->encodedPictureCount > 0)
        {
            stats->globalSsim = m_analyzeAll.m_globalSsim / stats->encodedPictureCount;
            stats->globalPsnr = (stats->globalPsnrY * 6 + stats->globalPsnrU + stats->globalPsnrV) / (8 * stats->encodedPictureCount);
            stats->elapsedVideoTime = (double)stats->encodedPictureCount * param->fpsDenom / param->fpsNum;
            stats->bitrate = (0.001f * stats->accBits) / stats->elapsedVideoTime;
        }
        else
        {
            stats->globalSsim = 0;
            stats->globalPsnr = 0;
            stats->bitrate = 0;
            stats->elapsedVideoTime = 0;
        }
    }

    /* If new statistics are added to x265_stats, we must check here whether the
     * structure provided by the user is the new structure or an older one (for
     * future safety) */
}

void Encoder::writeLog(int argc, char **argv)
{
    if (m_csvfpt)
    {
        if (param->logLevel >= X265_LOG_DEBUG)
        {
            fprintf(m_csvfpt, "Summary\n");
            fprintf(m_csvfpt, "Command, Date/Time, Elapsed Time, FPS, Bitrate, Y PSNR, U PSNR, V PSNR, Global PSNR, SSIM, SSIM (dB), Version\n");
        }
        // CLI arguments or other
        for (int i = 1; i < argc; i++)
        {
            if (i) fputc(' ', m_csvfpt);
            fputs(argv[i], m_csvfpt);
        }

        // current date and time
        time_t now;
        struct tm* timeinfo;
        time(&now);
        timeinfo = localtime(&now);
        char buffer[128];
        strftime(buffer, 128, "%c", timeinfo);
        fprintf(m_csvfpt, ", %s, ", buffer);

        x265_stats stats;
        fetchStats(&stats, sizeof(stats));

        // elapsed time, fps, bitrate
        fprintf(m_csvfpt, "%.2f, %.2f, %.2f,",
                stats.elapsedEncodeTime, stats.encodedPictureCount / stats.elapsedEncodeTime, stats.bitrate);

        if (param->bEnablePsnr)
            fprintf(m_csvfpt, " %.3lf, %.3lf, %.3lf, %.3lf,",
                    stats.globalPsnrY / stats.encodedPictureCount, stats.globalPsnrU / stats.encodedPictureCount,
                    stats.globalPsnrV / stats.encodedPictureCount, stats.globalPsnr);
        else
            fprintf(m_csvfpt, " -, -, -, -,");
        if (param->bEnableSsim)
            fprintf(m_csvfpt, " %.6f, %6.3f,", stats.globalSsim, x265_ssim2dB(stats.globalSsim));
        else
            fprintf(m_csvfpt, " -, -,");

        fprintf(m_csvfpt, " %s\n", x265_version_str);
    }
}

/**
 * Produce an ascii(hex) representation of picture digest.
 *
 * Returns: a statically allocated null-terminated string.  DO NOT FREE.
 */
static const char*digestToString(const unsigned char digest[3][16], int numChar)
{
    const char* hex = "0123456789abcdef";
    static char string[99];
    int cnt = 0;

    for (int yuvIdx = 0; yuvIdx < 3; yuvIdx++)
    {
        for (int i = 0; i < numChar; i++)
        {
            string[cnt++] = hex[digest[yuvIdx][i] >> 4];
            string[cnt++] = hex[digest[yuvIdx][i] & 0xf];
        }

        string[cnt++] = ',';
    }

    string[cnt - 1] = '\0';
    return string;
}

void Encoder::finishFrameStats(TComPic* pic, FrameEncoder *curEncoder, uint64_t bits)
{
    TComPicYuv* recon = pic->getPicYuvRec();

    //===== calculate PSNR =====
    int width  = recon->getWidth() - m_pad[0];
    int height = recon->getHeight() - m_pad[1];
    int size = width * height;

    int maxvalY = 255 << (X265_DEPTH - 8);
    int maxvalC = 255 << (X265_DEPTH - 8);
    double refValueY = (double)maxvalY * maxvalY * size;
    double refValueC = (double)maxvalC * maxvalC * size / 4.0;
    uint64_t ssdY, ssdU, ssdV;

    ssdY = pic->m_SSDY;
    ssdU = pic->m_SSDU;
    ssdV = pic->m_SSDV;
    double psnrY = (ssdY ? 10.0 * log10(refValueY / (double)ssdY) : 99.99);
    double psnrU = (ssdU ? 10.0 * log10(refValueC / (double)ssdU) : 99.99);
    double psnrV = (ssdV ? 10.0 * log10(refValueC / (double)ssdV) : 99.99);

    TComSlice*  slice = pic->getSlice();

    //===== add bits, psnr and ssim =====
    m_analyzeAll.addBits(bits);
    m_analyzeAll.addQP(pic->m_avgQpAq);

    if (param->bEnablePsnr)
    {
        m_analyzeAll.addPsnr(psnrY, psnrU, psnrV);
    }

    double ssim = 0.0;
    if (param->bEnableSsim && pic->m_ssimCnt > 0)
    {
        ssim = pic->m_ssim / pic->m_ssimCnt;
        m_analyzeAll.addSsim(ssim);
    }
    if (slice->isIntra())
    {
        m_analyzeI.addBits(bits);
        m_analyzeI.addQP(pic->m_avgQpAq);
        if (param->bEnablePsnr)
            m_analyzeI.addPsnr(psnrY, psnrU, psnrV);
        if (param->bEnableSsim)
            m_analyzeI.addSsim(ssim);
    }
    else if (slice->isInterP())
    {
        m_analyzeP.addBits(bits);
        m_analyzeP.addQP(pic->m_avgQpAq);
        if (param->bEnablePsnr)
            m_analyzeP.addPsnr(psnrY, psnrU, psnrV);
        if (param->bEnableSsim)
            m_analyzeP.addSsim(ssim);
    }
    else if (slice->isInterB())
    {
        m_analyzeB.addBits(bits);
        m_analyzeB.addQP(pic->m_avgQpAq);
        if (param->bEnablePsnr)
            m_analyzeB.addPsnr(psnrY, psnrU, psnrV);
        if (param->bEnableSsim)
            m_analyzeB.addSsim(ssim);
    }

    // if debug log level is enabled, per frame logging is performed
    if (param->logLevel >= X265_LOG_DEBUG)
    {
        char c = (slice->isIntra() ? 'I' : slice->isInterP() ? 'P' : 'B');
        int poc = slice->getPOC();
        if (!slice->isReferenced())
            c += 32; // lower case if unreferenced

        char buf[1024];
        int p;
        p = sprintf(buf, "POC:%d %c QP %2.2lf(%d) %10d bits", poc, c, pic->m_avgQpAq, slice->getSliceQp(), (int)bits);
        if (param->rc.rateControlMode == X265_RC_CRF)
            p += sprintf(buf + p, " RF:%.3lf", pic->m_rateFactor);
        if (param->bEnablePsnr)
            p += sprintf(buf + p, " [Y:%6.2lf U:%6.2lf V:%6.2lf]", psnrY, psnrU, psnrV);
        if (param->bEnableSsim)
            p += sprintf(buf + p, " [SSIM: %.3lfdB]", x265_ssim2dB(ssim));

        if (!slice->isIntra())
        {
            int numLists = slice->isInterP() ? 1 : 2;
            for (int list = 0; list < numLists; list++)
            {
                p += sprintf(buf + p, " [L%d ", list);
                for (int ref = 0; ref < slice->getNumRefIdx(list); ref++)
                {
                    int k = slice->getRefPOC(list, ref) - slice->getLastIDR();
                    p += sprintf(buf + p, "%d ", k);
                }

                p += sprintf(buf + p, "]");
            }
        }

        // per frame CSV logging if the file handle is valid
        if (m_csvfpt)
        {
            fprintf(m_csvfpt, "%d, %c-SLICE, %4d, %2.2lf, %10d,", m_outputCount++, c, poc, pic->m_avgQpAq, (int)bits);
            if (param->rc.rateControlMode == X265_RC_CRF)
                fprintf(m_csvfpt, "%.3lf,", pic->m_rateFactor);
            double psnr = (psnrY * 6 + psnrU + psnrV) / 8;
            if (param->bEnablePsnr)
                fprintf(m_csvfpt, "%.3lf, %.3lf, %.3lf, %.3lf,", psnrY, psnrU, psnrV, psnr);
            else
                fprintf(m_csvfpt, " -, -, -, -,");
            if (param->bEnableSsim)
                fprintf(m_csvfpt, " %.6f, %6.3f,", ssim, x265_ssim2dB(ssim));
            else
                fprintf(m_csvfpt, " -, -,");
            fprintf(m_csvfpt, " %.3lf, %.3lf", pic->m_frameTime, pic->m_elapsedCompressTime);
            if (!slice->isIntra())
            {
                int numLists = slice->isInterP() ? 1 : 2;
                for (int list = 0; list < numLists; list++)
                {
                    fprintf(m_csvfpt, ", ");
                    for (int ref = 0; ref < slice->getNumRefIdx(list); ref++)
                    {
                        int k = slice->getRefPOC(list, ref) - slice->getLastIDR();
                        fprintf(m_csvfpt, " %d", k);
                    }
                }

                if (numLists == 1)
                    fprintf(m_csvfpt, ", -");
            }
            else
                fprintf(m_csvfpt, ", -, -");
            fprintf(m_csvfpt, "\n");
        }

        if (param->decodedPictureHashSEI && param->logLevel >= X265_LOG_FULL)
        {
            const char* digestStr = NULL;
            if (param->decodedPictureHashSEI == 1)
            {
                digestStr = digestToString(curEncoder->m_seiReconPictureDigest.digest, 16);
                p += sprintf(buf + p, " [MD5:%s]", digestStr);
            }
            else if (param->decodedPictureHashSEI == 2)
            {
                digestStr = digestToString(curEncoder->m_seiReconPictureDigest.digest, 2);
                p += sprintf(buf + p, " [CRC:%s]", digestStr);
            }
            else if (param->decodedPictureHashSEI == 3)
            {
                digestStr = digestToString(curEncoder->m_seiReconPictureDigest.digest, 4);
                p += sprintf(buf + p, " [Checksum:%s]", digestStr);
            }
        }
        x265_log(param, X265_LOG_DEBUG, "%s\n", buf);
        fflush(stderr);
    }
}

#if defined(_MSC_VER)
#pragma warning(disable: 4800) // forcing int to bool
#pragma warning(disable: 4127) // conditional expression is constant
#endif

void Encoder::initSPS(TComSPS *sps)
{
    ProfileTierLevel& profileTierLevel = *sps->getPTL()->getGeneralPTL();

    profileTierLevel.setLevelIdc(m_level);
    profileTierLevel.setTierFlag(m_levelTier ? true : false);
    profileTierLevel.setProfileIdc(m_profile);
    profileTierLevel.setProfileCompatibilityFlag(m_profile, 1);
    profileTierLevel.setProgressiveSourceFlag(!param->interlaceMode);
    profileTierLevel.setInterlacedSourceFlag(!!param->interlaceMode);
    profileTierLevel.setNonPackedConstraintFlag(m_nonPackedConstraintFlag);
    profileTierLevel.setFrameOnlyConstraintFlag(m_frameOnlyConstraintFlag);

    if (m_profile == Profile::MAIN10 && X265_DEPTH == 8)
    {
        /* The above constraint is equal to Profile::MAIN */
        profileTierLevel.setProfileCompatibilityFlag(Profile::MAIN, 1);
    }
    if (m_profile == Profile::MAIN)
    {
        /* A Profile::MAIN10 decoder can always decode Profile::MAIN */
        profileTierLevel.setProfileCompatibilityFlag(Profile::MAIN10, 1);
    }

    /* XXX: should Main be marked as compatible with still picture? */

    /* XXX: may be a good idea to refactor the above into a function
     * that chooses the actual compatibility based upon options */

    /* set the VPS profile information */
    *m_vps.getPTL() = *sps->getPTL();

    sps->setPicWidthInLumaSamples(param->sourceWidth);
    sps->setPicHeightInLumaSamples(param->sourceHeight);
    sps->setConformanceWindow(m_conformanceWindow);
    sps->setChromaFormatIdc(param->internalCsp);
    sps->setMaxCUSize(g_maxCUSize);
    sps->setMaxCUDepth(g_maxCUDepth);

    int minCUSize = sps->getMaxCUSize() >> (sps->getMaxCUDepth() - g_addCUDepth);
    int log2MinCUSize = 0;
    while (minCUSize > 1)
    {
        minCUSize >>= 1;
        log2MinCUSize++;
    }

    sps->setLog2MinCodingBlockSize(log2MinCUSize);
    sps->setLog2DiffMaxMinCodingBlockSize(sps->getMaxCUDepth() - g_addCUDepth);

    sps->setPCMLog2MinSize(m_pcmLog2MinSize);
    sps->setUsePCM(m_usePCM);
    sps->setPCMLog2MaxSize(m_pcmLog2MaxSize);

    sps->setQuadtreeTULog2MaxSize(m_quadtreeTULog2MaxSize);
    sps->setQuadtreeTULog2MinSize(m_quadtreeTULog2MinSize);
    sps->setQuadtreeTUMaxDepthInter(param->tuQTMaxInterDepth);
    sps->setQuadtreeTUMaxDepthIntra(param->tuQTMaxIntraDepth);

    sps->setTMVPFlagsPresent(false);
    sps->setUseLossless(m_useLossless);

    sps->setMaxTrSize(1 << m_quadtreeTULog2MaxSize);

    for (uint32_t i = 0; i < g_maxCUDepth - g_addCUDepth; i++)
    {
        sps->setAMPAcc(i, param->bEnableAMP);
    }

    sps->setUseAMP(param->bEnableAMP);

    for (uint32_t i = g_maxCUDepth - g_addCUDepth; i < g_maxCUDepth; i++)
    {
        sps->setAMPAcc(i, 0);
    }

    sps->setBitDepthY(X265_DEPTH);
    sps->setBitDepthC(X265_DEPTH);

    sps->setQpBDOffsetY(QP_BD_OFFSET);
    sps->setQpBDOffsetC(QP_BD_OFFSET);

    sps->setUseSAO(param->bEnableSAO);

    // TODO: hard-code these values in SPS code
    sps->setMaxTLayers(1);
    sps->setTemporalIdNestingFlag(true);
    for (uint32_t i = 0; i < sps->getMaxTLayers(); i++)
    {
        sps->setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
        sps->setNumReorderPics(m_numReorderPics[i], i);
    }

    // TODO: it is recommended for this to match the input bit depth
    sps->setPCMBitDepthLuma(X265_DEPTH);
    sps->setPCMBitDepthChroma(X265_DEPTH);
    sps->setPCMFilterDisableFlag(m_bPCMFilterDisableFlag);
    sps->setScalingListFlag((m_useScalingListId == 0) ? 0 : 1);
    sps->setUseStrongIntraSmoothing(param->bEnableStrongIntraSmoothing);

    sps->setVuiParametersPresentFlag(true);
    TComVUI* vui = sps->getVuiParameters();
    vui->setAspectRatioInfoPresentFlag(!!param->vui.aspectRatioIdc);
    vui->setAspectRatioIdc(param->vui.aspectRatioIdc);
    vui->setSarWidth(param->vui.sarWidth);
    vui->setSarHeight(param->vui.sarHeight);

    vui->setOverscanInfoPresentFlag(param->vui.bEnableOverscanInfoPresentFlag);
    vui->setOverscanAppropriateFlag(param->vui.bEnableOverscanAppropriateFlag);

    vui->setVideoSignalTypePresentFlag(param->vui.bEnableVideoSignalTypePresentFlag);
    vui->setVideoFormat(param->vui.videoFormat);
    vui->setVideoFullRangeFlag(param->vui.bEnableVideoFullRangeFlag);
    vui->setColourDescriptionPresentFlag(param->vui.bEnableColorDescriptionPresentFlag);
    vui->setColourPrimaries(param->vui.colorPrimaries);
    vui->setTransferCharacteristics(param->vui.transferCharacteristics);
    vui->setMatrixCoefficients(param->vui.matrixCoeffs);
    vui->setChromaLocInfoPresentFlag(param->vui.bEnableChromaLocInfoPresentFlag);
    vui->setChromaSampleLocTypeTopField(param->vui.chromaSampleLocTypeTopField);
    vui->setChromaSampleLocTypeBottomField(param->vui.chromaSampleLocTypeBottomField);
    vui->setNeutralChromaIndicationFlag(m_neutralChromaIndicationFlag);
    vui->setDefaultDisplayWindow(m_defaultDisplayWindow);

    vui->setFrameFieldInfoPresentFlag(!!param->interlaceMode);
    vui->setFieldSeqFlag(!!param->interlaceMode);

    vui->setHrdParametersPresentFlag(false);
    vui->getHrdParameters()->setNalHrdParametersPresentFlag(false);
    vui->getHrdParameters()->setSubPicHrdParamsPresentFlag(false);

    vui->getTimingInfo()->setTimingInfoPresentFlag(true);
    vui->getTimingInfo()->setNumUnitsInTick(param->fpsDenom);
    vui->getTimingInfo()->setTimeScale(param->fpsNum);
    vui->getTimingInfo()->setPocProportionalToTimingFlag(m_pocProportionalToTimingFlag);
    vui->getTimingInfo()->setNumTicksPocDiffOneMinus1(m_numTicksPocDiffOneMinus1);

    vui->setBitstreamRestrictionFlag(false);
    vui->setTilesFixedStructureFlag(m_tilesFixedStructureFlag);
    vui->setMotionVectorsOverPicBoundariesFlag(m_motionVectorsOverPicBoundariesFlag);
    vui->setRestrictedRefPicListsFlag(m_restrictedRefPicListsFlag);
    vui->setMinSpatialSegmentationIdc(m_minSpatialSegmentationIdc);
    vui->setMaxBytesPerPicDenom(m_maxBytesPerPicDenom);
    vui->setMaxBitsPerMinCuDenom(m_maxBitsPerMinCuDenom);
    vui->setLog2MaxMvLengthHorizontal(m_log2MaxMvLengthHorizontal);
    vui->setLog2MaxMvLengthVertical(m_log2MaxMvLengthVertical);
}

void Encoder::initPPS(TComPPS *pps)
{
    pps->setConstrainedIntraPred(param->bEnableConstrainedIntra);
    bool isVbv = param->rc.vbvBufferSize > 0 && param->rc.vbvMaxBitrate > 0;

    /* TODO: This variable m_maxCuDQPDepth needs to be a CLI option to allow us to choose AQ granularity */
    bool bUseDQP = (m_maxCuDQPDepth > 0 || param->rc.aqMode || isVbv) ? true : false;

    int lowestQP = -QP_BD_OFFSET;

    if (m_useLossless)
    {
        if ((m_maxCuDQPDepth == 0) && (param->rc.qp == lowestQP))
        {
            bUseDQP = false;
        }
        else
        {
            bUseDQP = true;
        }
    }

    if (bUseDQP)
    {
        pps->setUseDQP(true);
        pps->setMaxCuDQPDepth(m_maxCuDQPDepth);
        pps->setMinCuDQPSize(pps->getSPS()->getMaxCUSize() >> (pps->getMaxCuDQPDepth()));
    }
    else
    {
        pps->setUseDQP(false);
        pps->setMaxCuDQPDepth(0);
        pps->setMinCuDQPSize(pps->getSPS()->getMaxCUSize() >> (pps->getMaxCuDQPDepth()));
    }

    pps->setChromaCbQpOffset(param->cbQpOffset);
    pps->setChromaCrQpOffset(param->crQpOffset);

    pps->setEntropyCodingSyncEnabledFlag(param->bEnableWavefront);
    pps->setUseWP(param->bEnableWeightedPred);
    pps->setWPBiPred(param->bEnableWeightedBiPred);
    pps->setOutputFlagPresentFlag(false);
    pps->setSignHideFlag(param->bEnableSignHiding);
    pps->setDeblockingFilterControlPresentFlag(!param->bEnableLoopFilter);
    pps->setDeblockingFilterOverrideEnabledFlag(!m_loopFilterOffsetInPPS);
    pps->setPicDisableDeblockingFilterFlag(!param->bEnableLoopFilter);
    pps->setLog2ParallelMergeLevelMinus2(m_log2ParallelMergeLevelMinus2);
    pps->setCabacInitPresentFlag(param->frameNumThreads > 1 ? 0 : CABAC_INIT_PRESENT_FLAG);

    pps->setNumRefIdxL0DefaultActive(1);
    pps->setNumRefIdxL1DefaultActive(1);

    pps->setTransquantBypassEnableFlag(m_TransquantBypassEnableFlag);
    pps->setUseTransformSkip(param->bEnableTransformSkip);
    pps->setLoopFilterAcrossTilesEnabledFlag(m_loopFilterAcrossTilesEnabledFlag);
}

void Encoder::configure(x265_param *p)
{
    // Trim the thread pool if WPP is disabled
    if (!p->bEnableWavefront)
        p->poolNumThreads = 1;

    setThreadPool(ThreadPool::allocThreadPool(p->poolNumThreads));
    int poolThreadCount = ThreadPool::getThreadPool()->getThreadCount();
    int rows = (p->sourceHeight + p->maxCUSize - 1) / p->maxCUSize;

    if (p->frameNumThreads == 0)
    {
        // auto-detect frame threads
        int cpuCount = getCpuCount();
        if (poolThreadCount <= 1)
            p->frameNumThreads = X265_MIN(cpuCount, rows / 2);
        else if (cpuCount > 32)
            p->frameNumThreads = 6; // dual-socket 10-core IvyBridge or higher
        else if (cpuCount >= 16)
            p->frameNumThreads = 5; // 8 HT cores, or dual socket
        else if (cpuCount >= 8)
            p->frameNumThreads = 3; // 4 HT cores
        else if (cpuCount >= 4)
            p->frameNumThreads = 2; // Dual or Quad core
        else
            p->frameNumThreads = 1;
    }
    if (poolThreadCount > 1)
    {
        x265_log(p, X265_LOG_INFO, "WPP streams / pool / frames         : %d / %d / %d\n", rows, poolThreadCount, p->frameNumThreads);
    }
    else if (p->frameNumThreads > 1)
    {
        x265_log(p, X265_LOG_INFO, "Concurrently encoded frames         : %d\n", p->frameNumThreads);
        p->bEnableWavefront = 0;
    }
    else
    {
        x265_log(p, X265_LOG_INFO, "Parallelism disabled, single thread mode\n");
        p->bEnableWavefront = 0;
    }
    if (!p->saoLcuBasedOptimization && p->frameNumThreads > 1)
    {
        x265_log(p, X265_LOG_INFO, "Warning: picture-based SAO used with frame parallelism\n");
    }
    if (p->keyframeMax < 0)
    {
        /* A negative max GOP size indicates the user wants only one I frame at
         * the start of the stream. Set an infinite GOP distance and disable
         * adaptive I frame placement */
        p->keyframeMax = INT_MAX;
        p->scenecutThreshold = 0;
    }
    else if (p->keyframeMax <= 1)
    {
        // disable lookahead for all-intra encodes
        p->bFrameAdaptive = 0;
        p->bframes = 0;
    }
    if (!p->keyframeMin)
    {
        double fps = (double)p->fpsNum / p->fpsDenom;
        p->keyframeMin = X265_MIN((int)fps, p->keyframeMax / 10);
    }
    p->keyframeMin = X265_MAX(1, X265_MIN(p->keyframeMin, p->keyframeMax / 2 + 1));
    if (!p->bEnableRectInter)
    {
        p->bEnableAMP = false;
    }
    if (p->bBPyramid && !p->bframes)
    {
        p->bBPyramid = 0;
    }
    /* Set flags according to RDLevel specified - check_params has verified that RDLevel is within range */
    switch (p->rdLevel)
    {
    case 6:
        bEnableRDOQ = bEnableRDOQTS = 1;
        break;
    case 5:
        bEnableRDOQ = bEnableRDOQTS = 1;
        break;
    case 4:
        bEnableRDOQ = bEnableRDOQTS = 1;
        break;
    case 3:
        bEnableRDOQ = bEnableRDOQTS = 0;
        break;
    case 2:
        bEnableRDOQ = bEnableRDOQTS = 0;
        break;
    case 1:
        bEnableRDOQ = bEnableRDOQTS = 0;
        break;
    case 0:
        bEnableRDOQ = bEnableRDOQTS = 0;
        break;
    }

    if (!(bEnableRDOQ && p->bEnableTransformSkip))
    {
        bEnableRDOQTS = 0;
    }

    if (p->rc.rateControlMode == X265_RC_CQP)
    {
        p->rc.aqMode = X265_AQ_NONE;
        p->rc.bitrate = 0;
        p->rc.cuTree = 0;
        p->rc.aqStrength = 0;
    }

    if (p->rc.aqMode == 0 && p->rc.cuTree)
    {
        p->rc.aqMode = X265_AQ_VARIANCE;
        p->rc.aqStrength = 0.0;
    }

    if (p->lookaheadDepth == 0 && p->rc.cuTree)
    {
        x265_log(p, X265_LOG_WARNING, "cuTree disabled, requires lookahead to be enabled\n");
        p->rc.cuTree = 0;
    }

    if (p->rc.aqStrength == 0 && p->rc.cuTree == 0)
    {
        p->rc.aqMode = X265_AQ_NONE;
    }

    if (p->rc.aqMode == X265_AQ_NONE && p->rc.cuTree == 0)
    {
        p->rc.aqStrength = 0;
    }

    if (p->internalCsp != X265_CSP_I420)
    {
        x265_log(p, X265_LOG_WARNING, "!! HEVC Range Extension specifications are not finalized !!\n");
        x265_log(p, X265_LOG_WARNING, "!! This output bitstream may not be compliant with the final spec !!\n");
    }

    if (p->interlaceMode)
    {
        x265_log(p, X265_LOG_WARNING, "Support for interlaced video is experimental\n");
    }

    if (p->rc.rfConstantMin > p->rc.rfConstant)
    {
        x265_log(param, X265_LOG_WARNING, "CRF min must be less than CRF\n");
        p->rc.rfConstantMin = 0;
    }

    m_bframeDelay = p->bframes ? (p->bBPyramid ? 2 : 1) : 0;

    //====== Coding Tools ========

    uint32_t tuQTMaxLog2Size = g_convertToBit[p->maxCUSize] + 2 - 1;
    m_quadtreeTULog2MaxSize = tuQTMaxLog2Size;
    uint32_t tuQTMinLog2Size = 2; //log2(4)
    m_quadtreeTULog2MinSize = tuQTMinLog2Size;

    //========= set default display window ==================================
    m_defaultDisplayWindow.m_enabledFlag = p->vui.bEnableDefaultDisplayWindowFlag;
    m_defaultDisplayWindow.m_winRightOffset = p->vui.defDispWinRightOffset;
    m_defaultDisplayWindow.m_winTopOffset = p->vui.defDispWinTopOffset;
    m_defaultDisplayWindow.m_winBottomOffset = p->vui.defDispWinBottomOffset;
    m_defaultDisplayWindow.m_winLeftOffset = p->vui.defDispWinLeftOffset;
    m_pad[0] = m_pad[1] = 0;

    //======== set pad size if width is not multiple of the minimum CU size =========
    uint32_t maxCUDepth = (uint32_t)g_convertToBit[p->maxCUSize];
    uint32_t minCUDepth = (p->maxCUSize >> (maxCUDepth - 1));
    if ((p->sourceWidth % minCUDepth) != 0)
    {
        uint32_t padsize = 0;
        uint32_t rem = p->sourceWidth % minCUDepth;
        padsize = minCUDepth - rem;
        p->sourceWidth += padsize;
        m_pad[0] = padsize; //pad width

        /* set the confirmation window offsets  */
        m_conformanceWindow.m_enabledFlag = true;
        m_conformanceWindow.m_winRightOffset = m_pad[0];
    }

    //======== set pad size if height is not multiple of the minimum CU size =========
    if ((p->sourceHeight % minCUDepth) != 0)
    {
        uint32_t padsize = 0;
        uint32_t rem = p->sourceHeight % minCUDepth;
        padsize = minCUDepth - rem;
        p->sourceHeight += padsize;
        m_pad[1] = padsize; //pad height

        /* set the confirmation window offsets  */
        m_conformanceWindow.m_enabledFlag = true;
        m_conformanceWindow.m_winBottomOffset = m_pad[1];
    }

    //====== HM Settings not exposed for configuration ======
    m_loopFilterOffsetInPPS = 0;
    m_loopFilterBetaOffsetDiv2 = 0;
    m_loopFilterTcOffsetDiv2 = 0;
    m_loopFilterAcrossTilesEnabledFlag = 1;

    m_vps.setMaxTLayers(1);
    m_vps.setTemporalNestingFlag(true);
    m_vps.setMaxLayers(1);
    for (int i = 0; i < MAX_TLAYER; i++)
    {
        /* Increase the DPB size and reorder picture if bpyramid is enabled */
        m_numReorderPics[i] = (p->bBPyramid && p->bframes > 1) ? 2 : 1;
        m_maxDecPicBuffering[i] = X265_MIN(MAX_NUM_REF, X265_MAX(m_numReorderPics[i] + 1, p->maxNumReferences) + m_numReorderPics[i]);

        m_vps.setNumReorderPics(m_numReorderPics[i], i);
        m_vps.setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
    }

    m_maxCuDQPDepth = 0;
    m_maxNumOffsetsPerPic = 2048;
    m_log2ParallelMergeLevelMinus2 = 0;

    m_nonPackedConstraintFlag = false;
    m_frameOnlyConstraintFlag = false;
    m_bufferingPeriodSEIEnabled = 0;
    m_displayOrientationSEIAngle = 0;
    m_gradualDecodingRefreshInfoEnabled = 0;
    m_decodingUnitInfoSEIEnabled = 0;
    m_useScalingListId = 0;
    m_activeParameterSetsSEIEnabled = 0;
    m_minSpatialSegmentationIdc = 0;
    m_neutralChromaIndicationFlag = false;
    m_pocProportionalToTimingFlag = false;
    m_numTicksPocDiffOneMinus1 = 0;
    m_motionVectorsOverPicBoundariesFlag = false;
    m_maxBytesPerPicDenom = 2;
    m_maxBitsPerMinCuDenom = 1;
    m_log2MaxMvLengthHorizontal = 15;
    m_log2MaxMvLengthVertical = 15;
    m_usePCM = 0;
    m_pcmLog2MinSize = 3;
    m_pcmLog2MaxSize = 5;
    m_bPCMInputBitDepthFlag = true;
    m_bPCMFilterDisableFlag = false;

    m_useLossless = false;  // x264 configures this via --qp=0
    m_TransquantBypassEnableFlag = false;
    m_CUTransquantBypassFlagValue = false;
}

int Encoder::extractNalData(NALUnitEBSP **nalunits, int& memsize)
{
    int offset = 0;
    int nalcount = 0;
    int num = 0;

    memsize = 0;
    for (; num < MAX_NAL_UNITS && nalunits[num] != NULL; num++)
    {
        const NALUnitEBSP& temp = *nalunits[num];
        memsize += temp.m_packetSize + 4;
    }

    X265_FREE(m_packetData);
    X265_FREE(m_nals);
    CHECKED_MALLOC(m_packetData, char, memsize);
    CHECKED_MALLOC(m_nals, x265_nal, num);

    memsize = 0;

    /* Copy NAL output packets into x265_nal_t structures */
    for (; nalcount < num; nalcount++)
    {
        const NALUnitEBSP& nalu = *nalunits[nalcount];
        int size; /* size of annexB unit in bytes */

        static const char start_code_prefix[] = { 0, 0, 0, 1 };
        if (nalcount == 0 || nalu.m_nalUnitType == NAL_UNIT_SPS || nalu.m_nalUnitType == NAL_UNIT_PPS)
        {
            /* From AVC, When any of the following conditions are fulfilled, the
             * zero_byte syntax element shall be present:
             *  - the nal_unit_type within the nal_unit() is equal to 7 (sequence
             *    parameter set) or 8 (picture parameter set),
             *  - the byte stream NAL unit syntax structure contains the first NAL
             *    unit of an access unit in decoding order, as specified by subclause
             *    7.4.1.2.3.
             */
            ::memcpy(m_packetData + memsize, start_code_prefix, 4);
            size = 4;
        }
        else
        {
            ::memcpy(m_packetData + memsize, start_code_prefix + 1, 3);
            size = 3;
        }
        memsize += size;
        ::memcpy(m_packetData + memsize, nalu.m_nalUnitData, nalu.m_packetSize);
        memsize += nalu.m_packetSize;

        m_nals[nalcount].type = nalu.m_nalUnitType;
        m_nals[nalcount].sizeBytes = size + nalu.m_packetSize;
    }

    /* Setup payload pointers, now that we're done adding content to m_packetData */
    for (int i = 0; i < nalcount; i++)
    {
        m_nals[i].payload = (uint8_t*)m_packetData + offset;
        offset += m_nals[i].sizeBytes;
    }

fail:
    return nalcount;
}
