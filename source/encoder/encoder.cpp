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
    m_globalSsim = 0;
    m_nals = NULL;
    m_packetData = NULL;

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
    m_lookahead = new Lookahead(this);
    m_dpb = new DPB(this);
    m_rateControl = new RateControl(&param);
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
}

int Encoder::getStreamHeaders(NALUnitEBSP **nalunits)
{
    return m_frameEncoder->getStreamHeaders(nalunits);
}

/**
 \param   flush               force encoder to encode a frame
 \param   pic_in              input original YUV picture or NULL
 \param   pic_out             pointer to reconstructed picture struct
 \param   nalunits            output NAL packets
 \retval                      number of encoded pictures
 */
int Encoder::encode(bool flush, const x265_picture_t* pic_in, x265_picture_t *pic_out, NALUnitEBSP **nalunits)
{
    if (pic_in)
    {
        TComPic *pic;
        if (m_freeList.empty())
        {
            pic = new TComPic;
            pic->create(param.sourceWidth, param.sourceHeight, g_maxCUWidth, g_maxCUHeight, g_maxCUDepth,
                        getConformanceWindow(), getDefaultDisplayWindow(), param.bframes);
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
        pic->getPicYuvOrg()->copyFromPicture(*pic_in);
        pic->m_userData = pic_in->userData;

        // Encoder holds a reference count until collecting stats
        ATOMIC_INC(&pic->m_countRefEncoders);
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
            pic_out->bitDepth = sizeof(Pel) * 8;
            pic_out->userData = out->m_userData;
            switch (out->getSlice()->getSliceType())
            {
            case I_SLICE:
                pic_out->sliceType = X265_TYPE_I;
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
        m_rateControl->rateControlStart(fenc, m_lookahead, &(curEncoder->m_rce));

        // Allow FrameEncoder::compressFrame() to start in a worker thread
        curEncoder->m_enable.trigger();
    }

    return ret;
}

double Encoder::printSummary()
{
    double fps = (double)param.frameRate;
    if (param.logLevel >= X265_LOG_INFO)
    {
        m_analyzeI.printOut('i', fps);
        m_analyzeP.printOut('p', fps);
        m_analyzeB.printOut('b', fps);
        m_analyzeAll.printOut('a', fps);
    }

#if _SUMMARY_OUT_
    m_analyzeAll.printSummaryOut(fps);
#endif
#if _SUMMARY_PIC_
    m_analyzeI.printSummary('I', fps);
    m_analyzeP.printSummary('P', fps);
    m_analyzeB.printSummary('B', fps);
#endif

    if (m_analyzeAll.getNumPic())
        return (m_analyzeAll.getPsnrY() * 6 + m_analyzeAll.getPsnrU() + m_analyzeAll.getPsnrV()) / (8 * m_analyzeAll.getNumPic());
    else
        return 100.0;
}

void Encoder::fetchStats(x265_stats_t *stats)
{
    stats->globalPsnrY = m_analyzeAll.getPsnrY();
    stats->globalPsnrU = m_analyzeAll.getPsnrU();
    stats->globalPsnrV = m_analyzeAll.getPsnrV();
    stats->totalNumPics = m_analyzeAll.getNumPic();
    stats->accBits = m_analyzeAll.getBits();
    if (stats->totalNumPics > 0)
    {
        stats->globalSsim = m_globalSsim / stats->totalNumPics;
        stats->globalPsnr = (stats->globalPsnrY * 6 + stats->globalPsnrU + stats->globalPsnrV) / (8 * stats->totalNumPics);
    }
    else
    {
        stats->globalSsim = 0;
        stats->globalPsnr = 0;
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
static UInt64 computeSSD(Pel *fenc, Pel *rec, int stride, int width, int height)
{
    UInt64 ssd = 0;

    if ((width | height) & 3)
    {
        /* Slow Path */
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                int diff = (int)(fenc[x] - rec[x]);
                ssd += diff * diff;
            }

            fenc += stride;
            rec += stride;
        }

        return ssd;
    }

    int y = 0;
    /* Consume Y in chunks of 64 */
    for (; y + 64 <= height; y += 64)
    {
        int x = 0;

        if (!(stride & 31))
            for (; x + 64 <= width; x += 64)
            {
                ssd += primitives.sse_pp[PARTITION_64x64](fenc + x, stride, rec + x, stride);
            }

        if (!(stride & 15))
            for (; x + 16 <= width; x += 16)
            {
                ssd += primitives.sse_pp[PARTITION_16x64](fenc + x, stride, rec + x, stride);
            }

        for (; x + 4 <= width; x += 4)
        {
            ssd += primitives.sse_pp[PARTITION_4x64](fenc + x, stride, rec + x, stride);
        }

        fenc += stride * 64;
        rec += stride * 64;
    }

    /* Consume Y in chunks of 16 */
    for (; y + 16 <= height; y += 16)
    {
        int x = 0;

        if (!(stride & 31))
            for (; x + 64 <= width; x += 64)
            {
                ssd += primitives.sse_pp[PARTITION_64x16](fenc + x, stride, rec + x, stride);
            }

        if (!(stride & 15))
            for (; x + 16 <= width; x += 16)
            {
                ssd += primitives.sse_pp[PARTITION_16x16](fenc + x, stride, rec + x, stride);
            }

        for (; x + 4 <= width; x += 4)
        {
            ssd += primitives.sse_pp[PARTITION_4x16](fenc + x, stride, rec + x, stride);
        }

        fenc += stride * 16;
        rec += stride * 16;
    }

    /* Consume Y in chunks of 4 */
    for (; y + 4 <= height; y += 4)
    {
        int x = 0;

        if (!(stride & 31))
            for (; x + 64 <= width; x += 64)
            {
                ssd += primitives.sse_pp[PARTITION_64x4](fenc + x, stride, rec + x, stride);
            }

        if (!(stride & 15))
            for (; x + 16 <= width; x += 16)
            {
                ssd += primitives.sse_pp[PARTITION_16x4](fenc + x, stride, rec + x, stride);
            }

        for (; x + 4 <= width; x += 4)
        {
            ssd += primitives.sse_pp[PARTITION_4x4](fenc + x, stride, rec + x, stride);
        }

        fenc += stride * 4;
        rec += stride * 4;
    }

    return ssd;
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

/* Returns Number of bits in current encoded pic */
uint64_t Encoder::calculateHashAndPSNR(TComPic* pic, NALUnitEBSP **nalunits)
{
    TComPicYuv* recon = pic->getPicYuvRec();
    TComPicYuv* orig  = pic->getPicYuvOrg();

    //===== calculate PSNR =====
    int stride = recon->getStride();
    int width  = recon->getWidth() - getPad(0);
    int height = recon->getHeight() - getPad(1);
    int size = width * height;

    UInt64 ssdY = computeSSD(orig->getLumaAddr(), recon->getLumaAddr(), stride, width, height);

    height >>= 1;
    width  >>= 1;
    stride = recon->getCStride();

    UInt64 ssdU = computeSSD(orig->getCbAddr(), recon->getCbAddr(), stride, width, height);
    UInt64 ssdV = computeSSD(orig->getCrAddr(), recon->getCrAddr(), stride, width, height);

    int maxvalY = 255 << (X265_DEPTH - 8);
    int maxvalC = 255 << (X265_DEPTH - 8);
    double refValueY = (double)maxvalY * maxvalY * size;
    double refValueC = (double)maxvalC * maxvalC * size / 4.0;
    double psnrY = (ssdY ? 10.0 * log10(refValueY / (double)ssdY) : 99.99);
    double psnrU = (ssdU ? 10.0 * log10(refValueC / (double)ssdU) : 99.99);
    double psnrV = (ssdV ? 10.0 * log10(refValueC / (double)ssdV) : 99.99);

    const char* digestStr = NULL;
    if (param.decodedPictureHashSEI)
    {
        SEIDecodedPictureHash sei_recon_picture_digest;
        if (param.decodedPictureHashSEI == 1)
        {
            /* calculate MD5sum for entire reconstructed picture */
            sei_recon_picture_digest.method = SEIDecodedPictureHash::MD5;
            calcMD5(*recon, sei_recon_picture_digest.digest);
            digestStr = digestToString(sei_recon_picture_digest.digest, 16);
        }
        else if (param.decodedPictureHashSEI == 2)
        {
            sei_recon_picture_digest.method = SEIDecodedPictureHash::CRC;
            calcCRC(*recon, sei_recon_picture_digest.digest);
            digestStr = digestToString(sei_recon_picture_digest.digest, 2);
        }
        else if (param.decodedPictureHashSEI == 3)
        {
            sei_recon_picture_digest.method = SEIDecodedPictureHash::CHECKSUM;
            calcChecksum(*recon, sei_recon_picture_digest.digest);
            digestStr = digestToString(sei_recon_picture_digest.digest, 4);
        }

        /* write the SEI messages */
        OutputNALUnit onalu(NAL_UNIT_SUFFIX_SEI, 0);
        m_frameEncoder->m_seiWriter.writeSEImessage(onalu.m_Bitstream, sei_recon_picture_digest, pic->getSlice()->getSPS());
        writeRBSPTrailingBits(onalu.m_Bitstream);

        int count = 0;
        while(nalunits[count] != NULL)
            count++;
        nalunits[count] = (NALUnitEBSP*)X265_MALLOC(NALUnitEBSP, 1);
        if (nalunits[count])
            nalunits[count]->init(onalu);
        else
            digestStr = NULL;
    }

    /* calculate the size of the access unit, excluding:
     *  - any AnnexB contributions (start_code_prefix, zero_byte, etc.,)
     *  - SEI NAL units
     */
    UInt numRBSPBytes = 0;
    for (int count = 0; nalunits[count] != NULL; count++)
    {
        UInt numRBSPBytes_nal = nalunits[count]->m_packetSize;
#if VERBOSE_RATE
        printf("*** %6s numBytesInNALunit: %u\n", nalUnitTypeToString((*it)->m_nalUnitType), numRBSPBytes_nal);
#endif
        if (nalunits[count]->m_nalUnitType != NAL_UNIT_PREFIX_SEI && nalunits[count]->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
        {
            numRBSPBytes += numRBSPBytes_nal;
        }
    }

    UInt bits = numRBSPBytes * 8;

    //===== add PSNR =====
    m_analyzeAll.addResult(psnrY, psnrU, psnrV, (double)bits);
    TComSlice*  slice = pic->getSlice();
    if (slice->isIntra())
    {
        m_analyzeI.addResult(psnrY, psnrU, psnrV, (double)bits);
    }
    if (slice->isInterP())
    {
        m_analyzeP.addResult(psnrY, psnrU, psnrV, (double)bits);
    }
    if (slice->isInterB())
    {
        m_analyzeB.addResult(psnrY, psnrU, psnrV, (double)bits);
    }
    if (param.bEnableSsim)
    {
        if (pic->getSlice()->m_ssimCnt > 0)
        {
            double ssim = pic->getSlice()->m_ssim / pic->getSlice()->m_ssimCnt;
            m_globalSsim += ssim;
        }
    }
    if (param.logLevel >= X265_LOG_DEBUG)
    {
        char c = (slice->isIntra() ? 'I' : slice->isInterP() ? 'P' : 'B');

        if (!slice->isReferenced())
            c += 32; // lower case if unreferenced

        fprintf(stderr, "\rPOC %4d ( %c-SLICE, nQP %d QP %d) %10d bits",
                slice->getPOC(),
                c,
                slice->getSliceQpBase(),
                slice->getSliceQp(),
                bits);

        fprintf(stderr, " [Y:%6.2lf U:%6.2lf V:%6.2lf]", psnrY, psnrU, psnrV);

        if (!slice->isIntra())
        {
            int numLists = slice->isInterP() ? 1 : 2;
            for (int list = 0; list < numLists; list++)
            {
                fprintf(stderr, " [L%d ", list);
                for (int ref = 0; ref < slice->getNumRefIdx(RefPicList(list)); ref++)
                {
                    fprintf(stderr, "%d ", slice->getRefPOC(RefPicList(list), ref) - slice->getLastIDR());
                }

                fprintf(stderr, "]");
            }
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

    for (UInt i = 0; i < g_maxCUDepth - g_addCUDepth; i++)
    {
        sps->setAMPAcc(i, param.bEnableAMP);
    }

    sps->setUseAMP(param.bEnableAMP);

    for (UInt i = g_maxCUDepth - g_addCUDepth; i < g_maxCUDepth; i++)
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
    for (UInt i = 0; i < sps->getMaxTLayers(); i++)
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
    bool bUseDQP = (getMaxCuDQPDepth() > 0) ? true : false;

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

void Encoder::determineLevelAndProfile(x265_param_t *_param)
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

    if (_param->internalBitDepth == 10)
        m_profile = Profile::MAIN10;
    else if (_param->keyframeMax == 1)
        m_profile = Profile::MAINSTILLPICTURE;
    else
        m_profile = Profile::MAIN;

    static const char *profiles[] = { "None", "Main", "Main10", "Mainstillpicture" };
    static const char *tiers[]    = { "Main", "High" };
    x265_log(_param, X265_LOG_INFO, "%s profile, Level-%s (%s tier)\n", profiles[m_profile], level, tiers[m_levelTier]);
}

void Encoder::configure(x265_param_t *_param)
{
    // Trim the thread pool if WPP is disabled
    if (!_param->bEnableWavefront)
        _param->poolNumThreads = 1;

    setThreadPool(ThreadPool::allocThreadPool(_param->poolNumThreads));
    int actual = ThreadPool::getThreadPool()->getThreadCount();
    if (actual > 1)
    {
        x265_log(_param, X265_LOG_INFO, "WPP streams / pool / frames  : %d / %d / %d\n",
                 (_param->sourceHeight + _param->maxCUSize - 1) / _param->maxCUSize, actual, _param->frameNumThreads);
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
    if (_param->keyframeMin == 1)
    {
        // disable lookahead for all-intra encodes
        _param->bFrameAdaptive = 0;
        _param->bframes = 0;
    }
    if (!_param->bEnableRectInter)
    {
        _param->bEnableAMP = false;
    }
    // if a bitrate is specified, chose ABR.  Else default to CQP
    if (_param->rc.bitrate)
    {
        _param->rc.rateControlMode = X265_RC_ABR;
    }

    if(!(_param->bEnableRDOQ && _param->bEnableTransformSkip))
    {
        _param->bEnableRDOQTS = 0;
    }

    /* Set flags according to RDLevel specified - check_params has verified that RDLevel is within range */
    switch(_param->bRDLevel)
    {
    case X265_NO_RDO_NO_RDOQ:
        _param->bEnableRDO = _param->bEnableRDOQ = 0; 
        break;
    case X265_NO_RDO:
        _param->bEnableRDO = 0;
        _param->bEnableRDOQ = 1;
        break;
    case X265_FULL_RDO:
        _param->bEnableRDO = _param->bEnableRDOQ = 1;
        break;
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
        m_numReorderPics[i] = 1; 
        m_maxDecPicBuffering[i] = X265_MIN(MAX_NUM_REF, X265_MAX(m_numReorderPics[i] + 1, _param->maxNumReferences) + 1);
        vps.setNumReorderPics(m_numReorderPics[i], i);
        vps.setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
    }

    m_vps = vps;
    m_maxCuDQPDepth = 0;
    m_maxNumOffsetsPerPic = 2048;
    m_log2ParallelMergeLevelMinus2 = 0;
    m_conformanceWindow.setWindow(0, 0, 0, 0);
    int nullpad[2] = { 0, 0 };
    setPad(nullpad);

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
    m_defaultDisplayWindow.setWindow(0, 0, 0, 0);
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
    CHECKED_MALLOC(m_nals, x265_nal_t, num);

    memsize = 0;

    /* Copy NAL output packets into x265_nal_t structures */
    for (; nalcount < num; nalcount++)
    {
        const NALUnitEBSP& nalu = *nalunits[nalcount];
        uint32_t size = 0; /* size of annexB unit in bytes */

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
            size += 4;
        }
        else
        {
            ::memcpy(m_packetData + memsize, start_code_prefix + 1, 3);
            size += 3;
        }
        memsize += size;
        uint32_t nalSize = nalu.m_packetSize;
        ::memcpy(m_packetData + memsize, nalu.m_nalUnitData, nalSize);
        size += nalSize;
        memsize += nalSize;

        m_nals[nalcount].i_type = nalu.m_nalUnitType;
        m_nals[nalcount].i_payload = size;
    }

    /* Setup payload pointers, now that we're done adding content to m_packetData */
    for (int i = 0; i < nalcount; i++)
    {
        m_nals[i].p_payload = (uint8_t*)m_packetData + offset;
        offset += m_nals[i].i_payload;
    }

fail:
    return nalcount;
}

extern "C"
x265_t *x265_encoder_open(x265_param_t *param)
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
int x265_encoder_headers(x265_t *enc, x265_nal_t **pp_nal, int *pi_nal)
{
    if (!pp_nal)
        return 0;

    Encoder *encoder = static_cast<Encoder*>(enc);

    int ret = 0;
    NALUnitEBSP *nalunits[MAX_NAL_UNITS] = {0, 0, 0, 0, 0};
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
int x265_encoder_encode(x265_t *enc, x265_nal_t **pp_nal, int *pi_nal, x265_picture_t *pic_in, x265_picture_t *pic_out)
{
    Encoder *encoder = static_cast<Encoder*>(enc);
    NALUnitEBSP *nalunits[MAX_NAL_UNITS] = {0, 0, 0, 0, 0};
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
void x265_encoder_get_stats(x265_t *enc, x265_stats_t *outputStats)
{
    Encoder *encoder = static_cast<Encoder*>(enc);
    encoder->fetchStats(outputStats);
}

extern "C"
void x265_encoder_close(x265_t *enc, double *outPsnr)
{
    Encoder *encoder = static_cast<Encoder*>(enc);
    double globalPsnr = encoder->printSummary();

    if (outPsnr)
        *outPsnr = globalPsnr;

    REPORT_CYCLE_COUNTER(ME);

    encoder->destroy();
    delete encoder;
}

extern "C"
void x265_cleanup(void)
{
    destroyROM();
    BitCost::destroy();
}
