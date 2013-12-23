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

#include "TLibEncoder/NALwrite.h"
#include "bitcost.h"
#include "encoder.h"
#include "slicetype.h"
#include "frameencoder.h"
#include "ratecontrol.h"
#include "dpb.h"

#include "x265.h"

#include <cstdlib>
#include <math.h> // log10
#include <stdio.h>
#include <string.h>
#include <time.h>

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
    m_pocLast = -1;
    m_maxRefPicNum = 0;
    m_curEncoder = 0;
    m_lookahead = NULL;
    m_frameEncoder = NULL;
    m_rateControl = NULL;
    m_dpb = NULL;
    m_nals = NULL;
    m_packetData = NULL;
    m_outputCount = 0;
    m_csvfpt = NULL;

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
        x265_log(&param, X265_LOG_ERROR, "Primitives must be initialized before encoder is created\n");
        abort();
    }

    m_frameEncoder = new FrameEncoder[param.frameNumThreads];
    if (m_frameEncoder)
    {
        for (int i = 0; i < param.frameNumThreads; i++)
        {
            m_frameEncoder[i].setThreadPool(m_threadPool);
        }
    }
    m_lookahead = new Lookahead(this, m_threadPool);
    m_dpb = new DPB(this);
    m_rateControl = new RateControl(this);
    m_numWPFrames = 0;

    /* Try to open CSV file handle */
    if (param.csvfn)
    {
        m_csvfpt = fopen(param.csvfn, "r");
        if (m_csvfpt)
        {
            // file already exists, re-open for append
            fclose(m_csvfpt);
            m_csvfpt = fopen(param.csvfn, "ab");
        }
        else
        {
            // new CSV file, write header
            m_csvfpt = fopen(param.csvfn, "wb");
            if (m_csvfpt)
            {
                if (param.logLevel >= X265_LOG_DEBUG)
                    fprintf(m_csvfpt, "Encode Order, Type, POC, nQP, QP, Bits, Y PSNR, U PSNR, V PSNR, YUV PSNR, SSIM, Encoding time, Elapsed time, List 0, List 1\n");
                else
                    fprintf(m_csvfpt, "Command, Date/Time, Elapsed Time, FPS, Bitrate, Y PSNR, U PSNR, V PSNR, Global PSNR, Global SSIM, Version\n");
            }
        }
    }
}

void Encoder::destroy()
{
    if (m_frameEncoder)
    {
        for (int i = 0; i < param.frameNumThreads; i++)
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
        pic->destroy(param.bframes);
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
    if (m_csvfpt)
        fclose(m_csvfpt);
}

void Encoder::init()
{
    if (m_frameEncoder)
    {
        int numRows = (param.sourceHeight + g_maxCUHeight - 1) / g_maxCUHeight;
        for (int i = 0; i < param.frameNumThreads; i++)
        {
            m_frameEncoder[i].init(this, numRows);
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

    curIdx = (m_curEncoder + param.frameNumThreads - 1) % param.frameNumThreads;
    encIdx = (curIdx + 1) % param.frameNumThreads;
    while (encIdx != curIdx)
    {
        FrameEncoder *encoder = &m_frameEncoder[encIdx];
        double bits;
        bits = encoder->m_rce.frameSizePlanned;
        rc->bufferFill -= bits;
        rc->bufferFill = X265_MAX(rc->bufferFill, 0);
        rc->bufferFill += encoder->m_rce.bufferRate;
        rc->bufferFill = X265_MIN(rc->bufferFill, rc->bufferSize);
        encIdx = (encIdx + 1) % param.frameNumThreads;
    }
}

/**
 \param   flush               force encoder to encode a frame
 \param   pic_in              input original YUV picture or NULL
 \param   pic_out             pointer to reconstructed picture struct
 \param   nalunits            output NAL packets
 \retval                      number of encoded pictures
 */
int Encoder::encode(bool flush, const x265_picture* pic_in, x265_picture *pic_out, NALUnitEBSP **nalunits)
{
    if (pic_in)
    {
        TComPic *pic;
        if (m_freeList.empty())
        {
            pic = new TComPic;
            pic->create(this);
            if (param.bEnableSAO)
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
        pic->getPicYuvOrg()->copyFromPicture(*pic_in, m_pad);
        pic->m_userData = pic_in->userData;
        pic->m_pts = pic_in->pts;

        // Encoder holds a reference count until collecting stats
        ATOMIC_INC(&pic->m_countRefEncoders);
        if (param.rc.aqMode || param.bEnableWeightedPred)
            m_rateControl->calcAdaptiveQuantFrame(pic);
        m_lookahead->addPicture(pic, pic_in->sliceType);
    }

    if (flush)
        m_lookahead->flush();

    FrameEncoder *curEncoder = &m_frameEncoder[m_curEncoder];
    m_curEncoder = (m_curEncoder + 1) % param.frameNumThreads;
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
            m_curEncoder = (m_curEncoder + 1) % param.frameNumThreads;
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
            pic_out->pts = out->m_pts;
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
            pic_out->stride[0] = recpic->getStride();
            pic_out->planes[1] = recpic->getCbAddr();
            pic_out->stride[1] = recpic->getCStride();
            pic_out->planes[2] = recpic->getCrAddr();
            pic_out->stride[2] = recpic->getCStride();
        }

        if (out->getSlice()->m_numWPRefs > 0)
            m_numWPFrames++;
        uint64_t bits = calculateHashAndPSNR(out, nalunits);
        // Allow this frame to be recycled if no frame encoders are using it for reference
        ATOMIC_DEC(&out->m_countRefEncoders);

        m_rateControl->rateControlEnd(bits, &(curEncoder->m_rce));

        m_dpb->recycleUnreferenced(m_freeList);

        ret = 1;
    }

    if (!m_lookahead->outputQueue.empty())
    {
        // pop a single frame from decided list, then provide to frame encoder
        // curEncoder is guaranteed to be idle at this point
        TComPic *fenc = m_lookahead->outputQueue.popFront();

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

char* Encoder::statsString(EncStats& stat, char* buffer)
{
    double fps = (double)param.frameRate;
    double scale = fps / 1000 / (double)stat.m_numPics;

    int len = sprintf(buffer, "%-6d ", stat.m_numPics);

    len += sprintf(buffer + len, "kb/s: %-8.2lf", stat.m_accBits * scale);
    if (param.bEnablePsnr)
    {
        len += sprintf(buffer + len, " PSNR Mean: Y:%.3lf U:%.3lf V:%.3lf",
                       stat.m_psnrSumY / (double)stat.m_numPics,
                       stat.m_psnrSumU / (double)stat.m_numPics,
                       stat.m_psnrSumV / (double)stat.m_numPics);
    }
    if (param.bEnableSsim)
    {
        len += sprintf(buffer + len, " SSIM Mean: %.6lf",
                       stat.m_globalSsim / (double)stat.m_numPics);
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
            for (int j = 0; j < param.frameNumThreads; j++)
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
            int cuSize = g_maxCUWidth >> depth;
            char stats[256] = {0};
            int len = 0;
            if (sliceType != I_SLICE)
            {
                len += sprintf(stats + len, "EncCU "LL"%% Merge "LL"%%", encCu, cntSkipCu);
            }
            if (cntInter)
            {
                len += sprintf(stats + len, " Inter "LL"%%", cntInter);
                if (param.bEnableAMP)
                    len += sprintf(stats + len, "(%dx%d "LL "%% %dx%d "LL "%% %dx%d "LL "%% AMP "LL "%%)",
                                   cuSize, cuSize, cuInterDistribution[0],
                                   cuSize / 2, cuSize, cuInterDistribution[2],
                                   cuSize, cuSize / 2, cuInterDistribution[1],
                                   cuInterDistribution[3]);
                else if (param.bEnableRectInter)
                    len += sprintf(stats + len, "(%dx%d "LL "%% %dx%d "LL "%% %dx%d "LL "%%)",
                                   cuSize, cuSize, cuInterDistribution[0],
                                   cuSize / 2, cuSize, cuInterDistribution[2],
                                   cuSize, cuSize / 2, cuInterDistribution[1]);
            }
            if (cntIntra)
            {
                len += sprintf(stats + len, " Intra "LL"%%(DC "LL"%% P "LL"%% Ang "LL"%%",
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
                        len += sprintf(stats + len, " %dx%d: "LL"%%", cuSize/2, cuSize/2, cntIntraNxN);
                }
            }
            if (stats[0])
                x265_log(&param, X265_LOG_INFO, "%c%-2d: %s\n", sliceType == P_SLICE ? 'P' : sliceType == B_SLICE ? 'B' : 'I', cuSize, stats);
        }
    }
#endif
    if (param.logLevel >= X265_LOG_INFO)
    {
        char buffer[200];
        if (m_analyzeI.m_numPics)
            x265_log(&param, X265_LOG_INFO, "frame I: %s\n", statsString(m_analyzeI, buffer));
        if (m_analyzeP.m_numPics)
            x265_log(&param, X265_LOG_INFO, "frame P: %s\n", statsString(m_analyzeP, buffer));
        if (m_analyzeB.m_numPics)
            x265_log(&param, X265_LOG_INFO, "frame B: %s\n", statsString(m_analyzeB, buffer));
        if (m_analyzeAll.m_numPics)
            x265_log(&param, X265_LOG_INFO, "global : %s\n", statsString(m_analyzeAll, buffer));
        if (param.bEnableWeightedPred && m_analyzeP.m_numPics)
        {
            x265_log(&param, X265_LOG_INFO, "%d of %d (%.2f%%) P frames weighted\n",
                     m_numWPFrames, m_analyzeP.m_numPics, (float)100.0 * m_numWPFrames / m_analyzeP.m_numPics);
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
        stats->totalWPFrames = m_numWPFrames;
        stats->accBits = m_analyzeAll.m_accBits;
        stats->elapsedEncodeTime = (double)(x265_mdate() - m_encodeStartTime) / 1000000;
        if (stats->encodedPictureCount > 0)
        {
            stats->globalSsim = m_analyzeAll.m_globalSsim / stats->encodedPictureCount;
            stats->globalPsnr = (stats->globalPsnrY * 6 + stats->globalPsnrU + stats->globalPsnrV) / (8 * stats->encodedPictureCount);
            stats->elapsedVideoTime = (double)stats->encodedPictureCount / param.frameRate;
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
    if (param.logLevel < X265_LOG_DEBUG && m_csvfpt)
    {
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

        if (param.bEnablePsnr)
            fprintf(m_csvfpt, " %.3lf, %.3lf, %.3lf, %.3lf,",
                    stats.globalPsnrY / stats.encodedPictureCount, stats.globalPsnrU / stats.encodedPictureCount,
                    stats.globalPsnrV / stats.encodedPictureCount, stats.globalPsnr);
        else
            fprintf(m_csvfpt, " -, -, -, -,");
        if (param.bEnableSsim)
            fprintf(m_csvfpt, " %.6f, %6.3fdb,", stats.globalSsim, x265_ssim(stats.globalSsim));
        else
            fprintf(m_csvfpt, " -, -,");

        fprintf(m_csvfpt, " %s\n", x265_version_str);
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

// TODO:
//   1 - as a performance optimization, if we're not reporting PSNR we do not have to measure PSNR
//       (we do not yet have a switch to disable PSNR reporting)
//   2 - it would be better to accumulate SSD of each CTU at the end of processCTU() while it is cache-hot
//       in fact, we almost certainly are already measuring the CTU distortion and not accumulating it

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

/* Returns Number of bits in current encoded pic */
uint64_t Encoder::calculateHashAndPSNR(TComPic* pic, NALUnitEBSP **nalunits)
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

    const char* digestStr = NULL;
    if (param.decodedPictureHashSEI)
    {
        if (param.decodedPictureHashSEI == 1)
        {
            digestStr = digestToString(m_frameEncoder->m_seiReconPictureDigest.digest, 16);
        }
        else if (param.decodedPictureHashSEI == 2)
        {
            digestStr = digestToString(m_frameEncoder->m_seiReconPictureDigest.digest, 2);
        }
        else if (param.decodedPictureHashSEI == 3)
        {
            digestStr = digestToString(m_frameEncoder->m_seiReconPictureDigest.digest, 4);
        }
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
        printf("*** %6s numBytesInNALunit: %u\n", nalUnitTypeToString((*it)->m_nalUnitType), numRBSPBytes_nal);
#endif
        if (nalunits[count]->m_nalUnitType != NAL_UNIT_PREFIX_SEI && nalunits[count]->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
        {
            numRBSPBytes += numRBSPBytes_nal;
        }
    }

    uint32_t bits = numRBSPBytes * 8;

    TComSlice*  slice = pic->getSlice();

    //===== add bits, psnr and ssim =====
    m_analyzeAll.addBits(bits);

    if (param.bEnablePsnr)
    {
        m_analyzeAll.addPsnr(psnrY, psnrU, psnrV);
    }

    double ssim = 0.0;
    if (param.bEnableSsim && pic->m_ssimCnt > 0)
    {
        ssim = pic->m_ssim / pic->m_ssimCnt;
        m_analyzeAll.addSsim(ssim);
    }
    if (slice->isIntra())
    {
        m_analyzeI.addBits(bits);
        if (param.bEnablePsnr)
            m_analyzeI.addPsnr(psnrY, psnrU, psnrV);
        if (param.bEnableSsim)
            m_analyzeI.addSsim(ssim);
    }
    else if (slice->isInterP())
    {
        m_analyzeP.addBits(bits);
        if (param.bEnablePsnr)
            m_analyzeP.addPsnr(psnrY, psnrU, psnrV);
        if (param.bEnableSsim)
            m_analyzeP.addSsim(ssim);
    }
    else if (slice->isInterB())
    {
        m_analyzeB.addBits(bits);
        if (param.bEnablePsnr)
            m_analyzeB.addPsnr(psnrY, psnrU, psnrV);
        if (param.bEnableSsim)
            m_analyzeB.addSsim(ssim);
    }

    // if debug log level is enabled, per frame logging is performed
    if (param.logLevel >= X265_LOG_DEBUG)
    {
        char c = (slice->isIntra() ? 'I' : slice->isInterP() ? 'P' : 'B');
        int poc = slice->getPOC();
        int QP_Base = slice->getSliceQpBase();
        int QP = slice->getSliceQp();
        if (!slice->isReferenced())
            c += 32; // lower case if unreferenced
        fprintf(stderr, "\rPOC %4d ( %c-SLICE, nQP %d QP %d) %10d bits", poc, c, QP_Base, QP, bits);
        if (param.bEnablePsnr)
            fprintf(stderr, " [Y:%6.2lf U:%6.2lf V:%6.2lf]", psnrY, psnrU, psnrV);
        if (param.bEnableSsim)
            fprintf(stderr, "[SSIM: %.3lf]", ssim);

        if (!slice->isIntra())
        {
            int numLists = slice->isInterP() ? 1 : 2;
            for (int list = 0; list < numLists; list++)
            {
                fprintf(stderr, " [L%d ", list);
                for (int ref = 0; ref < slice->getNumRefIdx(list); ref++)
                {
                    int k = slice->getRefPOC(list, ref) - slice->getLastIDR();
                    fprintf(stderr, "%d ", k);
                }

                fprintf(stderr, "]");
            }
        }

        // per frame CSV logging if the file handle is valid
        if (m_csvfpt)
        {
            fprintf(m_csvfpt, "%d, %c-SLICE, %4d, %d, %d, %10d,", m_outputCount++, c, poc, QP_Base, QP, bits);
            double psnr = (psnrY * 6 + psnrU + psnrV) / 8;
            if (param.bEnablePsnr)
                fprintf(m_csvfpt, "%.3lf, %.3lf, %.3lf, %.3lf,", psnrY, psnrU, psnrV, psnr);
            else
                fprintf(m_csvfpt, " -, -, -, -,");
            if (param.bEnableSsim)
                fprintf(m_csvfpt, " %.3lf,", ssim);
            else
                fprintf(m_csvfpt, " -,");
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

        if (digestStr && param.logLevel >= 4)
        {
            if (param.decodedPictureHashSEI == 1)
            {
                fprintf(stderr, " [MD5:%s]", digestStr);
            }
            else if (param.decodedPictureHashSEI == 2)
            {
                fprintf(stderr, " [CRC:%s]", digestStr);
            }
            else if (param.decodedPictureHashSEI == 3)
            {
                fprintf(stderr, " [Checksum:%s]", digestStr);
            }
        }
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    return bits;
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
    profileTierLevel.setProgressiveSourceFlag(m_progressiveSourceFlag);
    profileTierLevel.setInterlacedSourceFlag(m_interlacedSourceFlag);
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

    sps->setPicWidthInLumaSamples(param.sourceWidth);
    sps->setPicHeightInLumaSamples(param.sourceHeight);
    sps->setConformanceWindow(m_conformanceWindow);
    sps->setChromaFormatIdc(param.internalCsp);
    sps->setMaxCUWidth(g_maxCUWidth);
    sps->setMaxCUHeight(g_maxCUHeight);
    sps->setMaxCUDepth(g_maxCUDepth);

    int minCUSize = sps->getMaxCUWidth() >> (sps->getMaxCUDepth() - g_addCUDepth);
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
    sps->setQuadtreeTUMaxDepthInter(param.tuQTMaxInterDepth);
    sps->setQuadtreeTUMaxDepthIntra(param.tuQTMaxIntraDepth);

    sps->setTMVPFlagsPresent(false);
    sps->setUseLossless(m_useLossless);

    sps->setMaxTrSize(1 << m_quadtreeTULog2MaxSize);

    for (uint32_t i = 0; i < g_maxCUDepth - g_addCUDepth; i++)
    {
        sps->setAMPAcc(i, param.bEnableAMP);
    }

    sps->setUseAMP(param.bEnableAMP);

    for (uint32_t i = g_maxCUDepth - g_addCUDepth; i < g_maxCUDepth; i++)
    {
        sps->setAMPAcc(i, 0);
    }

    sps->setBitDepthY(X265_DEPTH);
    sps->setBitDepthC(X265_DEPTH);

    sps->setQpBDOffsetY(6 * (X265_DEPTH - 8));
    sps->setQpBDOffsetC(6 * (X265_DEPTH - 8));

    sps->setUseSAO(param.bEnableSAO);

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

    sps->setUseStrongIntraSmoothing(param.bEnableStrongIntraSmoothing);

    sps->setVuiParametersPresentFlag(getVuiParametersPresentFlag());
    if (sps->getVuiParametersPresentFlag())
    {
        TComVUI* vui = sps->getVuiParameters();
        vui->setAspectRatioInfoPresentFlag(getAspectRatioIdc() != -1);
        vui->setAspectRatioIdc(getAspectRatioIdc());
        vui->setSarWidth(getSarWidth());
        vui->setSarHeight(getSarHeight());
        vui->setOverscanInfoPresentFlag(getOverscanInfoPresentFlag());
        vui->setOverscanAppropriateFlag(getOverscanAppropriateFlag());
        vui->setVideoSignalTypePresentFlag(getVideoSignalTypePresentFlag());
        vui->setVideoFormat(getVideoFormat());
        vui->setVideoFullRangeFlag(getVideoFullRangeFlag());
        vui->setColourDescriptionPresentFlag(getColourDescriptionPresentFlag());
        vui->setColourPrimaries(getColourPrimaries());
        vui->setTransferCharacteristics(getTransferCharacteristics());
        vui->setMatrixCoefficients(getMatrixCoefficients());
        vui->setChromaLocInfoPresentFlag(getChromaLocInfoPresentFlag());
        vui->setChromaSampleLocTypeTopField(getChromaSampleLocTypeTopField());
        vui->setChromaSampleLocTypeBottomField(getChromaSampleLocTypeBottomField());
        vui->setNeutralChromaIndicationFlag(getNeutralChromaIndicationFlag());
        vui->setDefaultDisplayWindow(getDefaultDisplayWindow());
        vui->setFrameFieldInfoPresentFlag(getFrameFieldInfoPresentFlag());
        vui->setFieldSeqFlag(false);
        vui->setHrdParametersPresentFlag(false);
        vui->getTimingInfo()->setPocProportionalToTimingFlag(getPocProportionalToTimingFlag());
        vui->getTimingInfo()->setNumTicksPocDiffOneMinus1(getNumTicksPocDiffOneMinus1());
        vui->setBitstreamRestrictionFlag(getBitstreamRestrictionFlag());
        vui->setMotionVectorsOverPicBoundariesFlag(getMotionVectorsOverPicBoundariesFlag());
        vui->setMinSpatialSegmentationIdc(getMinSpatialSegmentationIdc());
        vui->setMaxBytesPerPicDenom(getMaxBytesPerPicDenom());
        vui->setMaxBitsPerMinCuDenom(getMaxBitsPerMinCuDenom());
        vui->setLog2MaxMvLengthHorizontal(getLog2MaxMvLengthHorizontal());
        vui->setLog2MaxMvLengthVertical(getLog2MaxMvLengthVertical());
    }

    /* set the VPS profile information */
    *getVPS()->getPTL() = *sps->getPTL();
    getVPS()->getTimingInfo()->setTimingInfoPresentFlag(false);
}

void Encoder::initPPS(TComPPS *pps)
{
    pps->setConstrainedIntraPred(param.bEnableConstrainedIntra);
    bool bUseDQP = ((getMaxCuDQPDepth() > 0) || param.rc.aqMode) ? true : false;

    int lowestQP = -(6 * (X265_DEPTH - 8)); //m_cSPS.getQpBDOffsetY();

    if (getUseLossless())
    {
        if ((getMaxCuDQPDepth() == 0) && (param.rc.qp == lowestQP))
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
        pps->setMinCuDQPSize(pps->getSPS()->getMaxCUWidth() >> (pps->getMaxCuDQPDepth()));
    }
    else
    {
        pps->setUseDQP(false);
        pps->setMaxCuDQPDepth(0);
        pps->setMinCuDQPSize(pps->getSPS()->getMaxCUWidth() >> (pps->getMaxCuDQPDepth()));
    }

    pps->setChromaCbQpOffset(param.cbQpOffset);
    pps->setChromaCrQpOffset(param.crQpOffset);

    pps->setEntropyCodingSyncEnabledFlag(param.bEnableWavefront);
    pps->setUseWP(param.bEnableWeightedPred);
    pps->setWPBiPred(param.bEnableWeightedBiPred);
    pps->setOutputFlagPresentFlag(false);
    pps->setSignHideFlag(param.bEnableSignHiding);
    pps->setDeblockingFilterControlPresentFlag(!param.bEnableLoopFilter);
    pps->setDeblockingFilterOverrideEnabledFlag(!m_loopFilterOffsetInPPS);
    pps->setPicDisableDeblockingFilterFlag(!param.bEnableLoopFilter);
    pps->setLog2ParallelMergeLevelMinus2(m_log2ParallelMergeLevelMinus2);
    pps->setCabacInitPresentFlag(param.frameNumThreads > 1 ? 0 : CABAC_INIT_PRESENT_FLAG);

    pps->setNumRefIdxL0DefaultActive(1);
    pps->setNumRefIdxL1DefaultActive(1);

    pps->setTransquantBypassEnableFlag(getTransquantBypassEnableFlag());
    pps->setUseTransformSkip(param.bEnableTransformSkip);
    pps->setLoopFilterAcrossTilesEnabledFlag(m_loopFilterAcrossTilesEnabledFlag);
}

void Encoder::determineLevelAndProfile(x265_param *_param)
{
    // this is all based on the table at on Wikipedia at
    // http://en.wikipedia.org/wiki/High_Efficiency_Video_Coding#Profiles

    // TODO: there are minimum CTU sizes for higher levels, needs to be enforced

    uint32_t lumaSamples = _param->sourceWidth * _param->sourceHeight;
    uint32_t samplesPerSec = lumaSamples * _param->frameRate;
    uint32_t bitrate = _param->rc.bitrate;

    m_level = Level::LEVEL1;
    const char *level = "1";
    if (samplesPerSec > 552960 || lumaSamples > 36864 || bitrate > 128)
    {
        m_level = Level::LEVEL2;
        level = "2";
    }
    if (samplesPerSec > 3686400 || lumaSamples > 122880 || bitrate > 1500)
    {
        m_level = Level::LEVEL2_1;
        level = "2.1";
    }
    if (samplesPerSec > 7372800 || lumaSamples > 245760 || bitrate > 3000)
    {
        m_level = Level::LEVEL3;
        level = "3";
    }
    if (samplesPerSec > 16588800 || lumaSamples > 552960 || bitrate > 6000)
    {
        m_level = Level::LEVEL3_1;
        level = "3.1";
    }
    if (samplesPerSec > 33177600 || lumaSamples > 983040 || bitrate > 10000)
    {
        m_level = Level::LEVEL4;
        level = "4";
    }
    if (samplesPerSec > 66846720 || bitrate > 30000)
    {
        m_level = Level::LEVEL4_1;
        level = "4.1";
    }
    if (samplesPerSec > 133693440 || lumaSamples > 2228224 || bitrate > 50000)
    {
        m_level = Level::LEVEL5;
        level = "5";
    }
    if (samplesPerSec > 267386880 || bitrate > 100000)
    {
        m_level = Level::LEVEL5_1;
        level = "5.1";
    }
    if (samplesPerSec > 534773760 || bitrate > 160000)
    {
        m_level = Level::LEVEL5_2;
        level = "5.2";
    }
    if (samplesPerSec > 1069547520 || lumaSamples > 8912896 || bitrate > 240000)
    {
        m_level = Level::LEVEL6;
        level = "6";
    }
    if (samplesPerSec > 1069547520 || bitrate > 240000)
    {
        m_level = Level::LEVEL6_1;
        level = "6.1";
    }
    if (samplesPerSec > 2139095040 || bitrate > 480000)
    {
        m_level = Level::LEVEL6_2;
        level = "6.2";
    }
    if (samplesPerSec > 4278190080U || lumaSamples > 35651584 || bitrate > 800000)
        x265_log(_param, X265_LOG_WARNING, "video size or bitrate out of scope for HEVC\n");

    /* Within a given level, we might be at a high tier, depending on bitrate */
    m_levelTier = Level::MAIN;
    switch (m_level)
    {
    case Level::LEVEL4:
        if (bitrate > 12000) m_levelTier = Level::HIGH;
        break;
    case Level::LEVEL4_1:
        if (bitrate > 20000) m_levelTier = Level::HIGH;
        break;
    case Level::LEVEL5:
        if (bitrate > 25000) m_levelTier = Level::HIGH;
        break;
    case Level::LEVEL5_1:
        if (bitrate > 40000) m_levelTier = Level::HIGH;
        break;
    case Level::LEVEL5_2:
        if (bitrate > 60000) m_levelTier = Level::HIGH;
        break;
    case Level::LEVEL6:
        if (bitrate > 60000) m_levelTier = Level::HIGH;
        break;
    case Level::LEVEL6_1:
        if (bitrate > 120000) m_levelTier = Level::HIGH;
        break;
    case Level::LEVEL6_2:
        if (bitrate > 240000) m_levelTier = Level::HIGH;
        break;
    default:
        break;
    }

    if (_param->inputBitDepth > 8)
        m_profile = Profile::MAIN10;
    else if (_param->keyframeMax == 1)
        m_profile = Profile::MAINSTILLPICTURE;
    else
        m_profile = Profile::MAIN;

    static const char *profiles[] = { "None", "Main", "Main10", "Mainstillpicture" };
    static const char *tiers[]    = { "Main", "High" };
    x265_log(_param, X265_LOG_INFO, "%s profile, Level-%s (%s tier)\n", profiles[m_profile], level, tiers[m_levelTier]);
}

void Encoder::configure(x265_param *_param)
{
    // Trim the thread pool if WPP is disabled
    if (!_param->bEnableWavefront)
        _param->poolNumThreads = 1;

    setThreadPool(ThreadPool::allocThreadPool(_param->poolNumThreads));
    int poolThreadCount = ThreadPool::getThreadPool()->getThreadCount();
    int rows = (_param->sourceHeight + _param->maxCUSize - 1) / _param->maxCUSize;

    if (_param->frameNumThreads == 0)
    {
        // auto-detect frame threads
        if (poolThreadCount > 32)
            _param->frameNumThreads = 6; // dual-socket 10-core IvyBridge or higher
        else if (poolThreadCount >= 16)
            _param->frameNumThreads = 5; // 8 HT cores, or dual socket
        else if (poolThreadCount >= 8)
            _param->frameNumThreads = 3; // 4 HT cores
        else if (poolThreadCount >= 4)
            _param->frameNumThreads = 2; // Dual or Quad core
        else
            _param->frameNumThreads = 1;
    }
    if (poolThreadCount > 1)
    {
        x265_log(_param, X265_LOG_INFO, "WPP streams / pool / frames  : %d / %d / %d\n", rows, poolThreadCount, _param->frameNumThreads);
    }
    else if (_param->frameNumThreads > 1)
    {
        x265_log(_param, X265_LOG_INFO, "Concurrently encoded frames  : %d\n", _param->frameNumThreads);
        _param->bEnableWavefront = 0;
    }
    else
    {
        x265_log(_param, X265_LOG_INFO, "Parallelism disabled, single thread mode\n");
        _param->bEnableWavefront = 0;
    }
    if (!_param->saoLcuBasedOptimization && _param->frameNumThreads > 1)
    {
        x265_log(_param, X265_LOG_INFO, "Warning: picture-based SAO used with frame parallelism\n");
    }
    if (!_param->keyframeMin)
    {
        _param->keyframeMin = _param->keyframeMax;
    }
    if (_param->keyframeMax <= 1)
    {
        // disable lookahead for all-intra encodes
        _param->bFrameAdaptive = 0;
        _param->bframes = 0;
    }
    if (!_param->bEnableRectInter)
    {
        _param->bEnableAMP = false;
    }
    if (_param->bBPyramid && !_param->bframes)
    {
        _param->bBPyramid = 0;
    }
    /* Set flags according to RDLevel specified - check_params has verified that RDLevel is within range */
    switch (_param->rdLevel)
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

    if (!(bEnableRDOQ && _param->bEnableTransformSkip))
    {
        bEnableRDOQTS = 0;
    }

    //====== Coding Tools ========

    uint32_t tuQTMaxLog2Size = g_convertToBit[_param->maxCUSize] + 2 - 1;
    m_quadtreeTULog2MaxSize = tuQTMaxLog2Size;
    uint32_t tuQTMinLog2Size = 2; //log2(4)
    m_quadtreeTULog2MinSize = tuQTMinLog2Size;

    //====== Enforce these hard coded settings before initializeGOP() to
    //       avoid a valgrind warning
    m_loopFilterOffsetInPPS = 0;
    m_loopFilterBetaOffsetDiv2 = 0;
    m_loopFilterTcOffsetDiv2 = 0;
    m_loopFilterAcrossTilesEnabledFlag = 1;

    //====== HM Settings not exposed for configuration ======
    TComVPS vps;
    vps.setMaxTLayers(1);
    vps.setTemporalNestingFlag(true);
    vps.setMaxLayers(1);
    for (int i = 0; i < MAX_TLAYER; i++)
    {
        /* Increase the DPB size and reorderpicture if enabled the bpyramid */
        m_numReorderPics[i] = (_param->bBPyramid && _param->bframes > 1) ? 2 : 1;
        m_maxDecPicBuffering[i] = X265_MIN(MAX_NUM_REF, X265_MAX(m_numReorderPics[i] + 1, _param->maxNumReferences) + m_numReorderPics[i]);

        vps.setNumReorderPics(m_numReorderPics[i], i);
        vps.setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
    }

    m_vps = vps;
    m_maxCuDQPDepth = 0;
    m_maxNumOffsetsPerPic = 2048;
    m_log2ParallelMergeLevelMinus2 = 0;

    //========= set default display window ==================================
    m_defaultDisplayWindow.m_enabledFlag = true;
    m_defaultDisplayWindow.m_winRightOffset = 0;
    m_defaultDisplayWindow.m_winTopOffset = 0;
    m_defaultDisplayWindow.m_winBottomOffset = 0;
    m_defaultDisplayWindow.m_winLeftOffset = 0;
    m_pad[0] = m_pad[1] = 0;

    //======== set pad size if width is not multiple of the minimum CU size =========
    uint32_t maxCUDepth = (uint32_t)g_convertToBit[_param->maxCUSize];
    uint32_t minCUDepth = (_param->maxCUSize >> (maxCUDepth - 1));
    if ((_param->sourceWidth % minCUDepth) != 0)
    {
        uint32_t padsize = 0;
        uint32_t rem = _param->sourceWidth % minCUDepth;
        padsize = minCUDepth - rem;
        _param->sourceWidth += padsize;
        m_pad[0] = padsize; //pad width

        /* set the confirmation window offsets  */
        m_conformanceWindow.m_enabledFlag = true;
        m_conformanceWindow.m_winRightOffset = m_pad[0];
    }

    //======== set pad size if height is not multiple of the minimum CU size =========
    if ((_param->sourceHeight % minCUDepth) != 0)
    {
        uint32_t padsize = 0;
        uint32_t rem = _param->sourceHeight % minCUDepth;
        padsize = minCUDepth - rem;
        _param->sourceHeight += padsize;
        m_pad[1] = padsize; //pad height

        /* set the confirmation window offsets  */
        m_conformanceWindow.m_enabledFlag = true;
        m_conformanceWindow.m_winBottomOffset = m_pad[1];
    }

    m_progressiveSourceFlag = true;
    m_interlacedSourceFlag = false;
    m_nonPackedConstraintFlag = false;
    m_frameOnlyConstraintFlag = false;
    m_bUseASR = false; // adapt search range based on temporal distances
    m_recoveryPointSEIEnabled = 0;
    m_bufferingPeriodSEIEnabled = 0;
    m_pictureTimingSEIEnabled = 0;
    m_displayOrientationSEIAngle = 0;
    m_gradualDecodingRefreshInfoEnabled = 0;
    m_decodingUnitInfoSEIEnabled = 0;
    m_useScalingListId = 0;
    m_activeParameterSetsSEIEnabled = 0;
    m_vuiParametersPresentFlag = false;
    m_minSpatialSegmentationIdc = 0;
    m_aspectRatioIdc = 0;
    m_sarWidth = 0;
    m_sarHeight = 0;
    m_overscanInfoPresentFlag = false;
    m_overscanAppropriateFlag = false;
    m_videoSignalTypePresentFlag = false;
    m_videoFormat = 5;
    m_videoFullRangeFlag = false;
    m_colourDescriptionPresentFlag = false;
    m_colourPrimaries = 2;
    m_transferCharacteristics = 2;
    m_matrixCoefficients = 2;
    m_chromaLocInfoPresentFlag = false;
    m_chromaSampleLocTypeTopField = 0;
    m_chromaSampleLocTypeBottomField = 0;
    m_neutralChromaIndicationFlag = false;
    m_frameFieldInfoPresentFlag = false;
    m_pocProportionalToTimingFlag = false;
    m_numTicksPocDiffOneMinus1 = 0;
    m_bitstreamRestrictionFlag = false;
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

int Encoder::extractNalData(NALUnitEBSP **nalunits)
{
    uint32_t memsize = 0;
    uint32_t offset = 0;
    int nalcount = 0;

    int num = 0;

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
        uint32_t size; /* size of annexB unit in bytes */

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

extern "C"
x265_param *x265_param_alloc()
{
    return (x265_param*)x265_malloc(sizeof(x265_param));
}

extern "C"
void x265_param_free(x265_param *p)
{
    return x265_free(p);
}

extern "C"
x265_picture *x265_picture_alloc()
{
    return (x265_picture*)x265_malloc(sizeof(x265_picture));
}

extern "C"
void x265_picture_free(x265_picture *p)
{
    return x265_free(p);
}

extern "C"
x265_encoder *x265_encoder_open(x265_param *param)
{
    x265_setup_primitives(param, -1);  // -1 means auto-detect if uninitialized

    if (x265_check_params(param))
        return NULL;

    if (x265_set_globals(param))
        return NULL;

    Encoder *encoder = new Encoder;
    if (encoder)
    {
        // these may change params for auto-detect, etc
        encoder->determineLevelAndProfile(param);
        encoder->configure(param);

        // save a copy of final parameters in TEncCfg
        memcpy(&encoder->param, param, sizeof(*param));

        x265_print_params(param);
        encoder->create();
        encoder->init();
    }

    return encoder;
}

extern "C"
int x265_encoder_headers(x265_encoder *enc, x265_nal **pp_nal, uint32_t *pi_nal)
{
    if (!pp_nal)
        return 0;

    Encoder *encoder = static_cast<Encoder*>(enc);

    int ret = 0;
    NALUnitEBSP *nalunits[MAX_NAL_UNITS] = { 0, 0, 0, 0, 0 };
    if (!encoder->getStreamHeaders(nalunits))
    {
        int nalcount = encoder->extractNalData(nalunits);
        *pp_nal = &encoder->m_nals[0];
        if (pi_nal) *pi_nal = nalcount;
    }
    else if (pi_nal)
    {
        *pi_nal = 0;
        ret = -1;
    }

    for (int i = 0; i < MAX_NAL_UNITS; i++)
    {
        if (nalunits[i])
        {
            free(nalunits[i]->m_nalUnitData);
            X265_FREE(nalunits[i]);
        }
    }

    return ret;
}

extern "C"
int x265_encoder_encode(x265_encoder *enc, x265_nal **pp_nal, uint32_t *pi_nal, x265_picture *pic_in, x265_picture *pic_out)
{
    Encoder *encoder = static_cast<Encoder*>(enc);
    NALUnitEBSP *nalunits[MAX_NAL_UNITS] = { 0, 0, 0, 0, 0 };
    int numEncoded = encoder->encode(!pic_in, pic_in, pic_out, nalunits);

    if (pp_nal && numEncoded > 0)
    {
        int nalcount = encoder->extractNalData(nalunits);
        *pp_nal = &encoder->m_nals[0];
        if (pi_nal) *pi_nal = nalcount;
    }
    else if (pi_nal)
        *pi_nal = 0;

    for (int i = 0; i < MAX_NAL_UNITS; i++)
    {
        if (nalunits[i])
        {
            free(nalunits[i]->m_nalUnitData);
            X265_FREE(nalunits[i]);
        }
    }

    return numEncoded;
}

EXTERN_CYCLE_COUNTER(ME);

extern "C"
void x265_encoder_get_stats(x265_encoder *enc, x265_stats *outputStats, uint32_t statsSizeBytes)
{
    Encoder *encoder = static_cast<Encoder*>(enc);

    encoder->fetchStats(outputStats, statsSizeBytes);
}

extern "C"
void x265_encoder_log(x265_encoder* enc, int argc, char **argv)
{
    Encoder *encoder = static_cast<Encoder*>(enc);

    encoder->writeLog(argc, argv);
}

extern "C"
void x265_encoder_close(x265_encoder *enc)
{
    Encoder *encoder = static_cast<Encoder*>(enc);

    REPORT_CYCLE_COUNTER(ME);

    encoder->printSummary();
    encoder->destroy();
    delete encoder;
}

extern "C"
void x265_cleanup(void)
{
    destroyROM();
    BitCost::destroy();
}

extern "C"
double x265_ssim(double ssim)
{
    double inv_ssim = 1 - ssim;
    if (inv_ssim <= 0.0000000001) /* Max 100dB */
        return 100;

    return -10.0 * log10(inv_ssim);
}

