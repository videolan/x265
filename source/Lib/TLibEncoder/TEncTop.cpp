/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncTop.cpp
    \brief    encoder class
*/

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComPic.h"
#include "primitives.h"
#include "common.h"

#include "TEncTop.h"
#include "NALwrite.h"

#include "slicetype.h"
#include "frameencoder.h"
#include "ratecontrol.h"
#include "dpb.h"
#include "TLibCommon/TComRom.h"
#include <math.h> // log10

using namespace x265;

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncTop::TEncTop()
{
    m_pocLast = -1;
    m_maxRefPicNum = 0;
    m_curEncoder = 0;
    m_lookahead = NULL;
    m_frameEncoder = NULL;
    m_rateControl = NULL;
    m_dpb = NULL;

#if ENC_DEC_TRACE
    g_hTrace = fopen("TraceEnc.txt", "wb");
    g_bJustDoIt = g_bEncDecTraceDisable;
    g_nSymbolCounter = 0;
#endif
}

TEncTop::~TEncTop()
{
#if ENC_DEC_TRACE
    fclose(g_hTrace);
#endif
}

void TEncTop::create()
{
    if (!primitives.sad[0])
    {
        printf("Primitives must be initialized before encoder is created\n");
        // we call exit() here because this should be an impossible condition when
        // using our public API, and indicates a serious bug.
        exit(1);
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

void TEncTop::destroy()
{
    if (m_frameEncoder)
    {
        for (int i = 0; i < param.frameNumThreads; i++)
        {
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
        delete m_lookahead;

    if (m_dpb)
        delete m_dpb;

    if (m_rateControl)
        delete m_rateControl;

    // thread pool release should always happen last
    if (m_threadPool)
        m_threadPool->release();
}

void TEncTop::init()
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

int TEncTop::getStreamHeaders(AccessUnit& accessUnit)
{
    return m_frameEncoder->getStreamHeaders(accessUnit);
}

/**
 \param   flush               force encoder to encode a frame
 \param   pic_in              input original YUV picture or NULL
 \param   pic_out             pointer to reconstructed picture struct
 \param   accessUnitsOut      output bitstream
 \retval                      number of encoded pictures
 */
int TEncTop::encode(bool flush, const x265_picture_t* pic_in, x265_picture_t *pic_out, AccessUnit& accessUnitOut)
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
        m_lookahead->addPicture(pic);
    }

    if (flush)
        m_lookahead->flush();

    FrameEncoder *curEncoder = &m_frameEncoder[m_curEncoder];
    m_curEncoder = (m_curEncoder + 1) % param.frameNumThreads;
    int ret = 0;

    // getEncodedPicture() should block until the FrameEncoder has completed
    // encoding the frame.  This is how back-pressure through the API is
    // accomplished when the encoder is full.
    TComPic *out = curEncoder->getEncodedPicture(accessUnitOut);

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
            out = curEncoder->getEncodedPicture(accessUnitOut);
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
            pic_out->planes[0] = recpic->getLumaAddr();
            pic_out->stride[0] = recpic->getStride();
            pic_out->planes[1] = recpic->getCbAddr();
            pic_out->stride[1] = recpic->getCStride();
            pic_out->planes[2] = recpic->getCrAddr();
            pic_out->stride[2] = recpic->getCStride();
        }

        double bits = calculateHashAndPSNR(out, accessUnitOut);

        m_rateControl->rateControlEnd(bits);

        m_dpb->recycleUnreferenced(m_freeList);

        ret = 1;
    }

    if (!m_lookahead->outputQueue.empty())
    {
        // pop a single frame from decided list, prepare, then provide to frame encoder
        // curEncoder is guaranteed to be idle at this point
        TComPic *fenc = m_lookahead->outputQueue.popFront();

        // Initialise slice in Frame Encoder
        int pocCurr = fenc->getSlice()->getPOC();
        bool forceIntra = m_dpb->m_cfg->param.keyframeMax == 1 || (pocCurr % m_dpb->m_cfg->param.keyframeMax == 0) || pocCurr == 0;
        int gopIdx = fenc->m_lowres.gopIdx;
        curEncoder->initSlice(fenc, forceIntra, gopIdx);

        // determine references, set QP, etc
        m_dpb->prepareEncode(fenc, curEncoder);
        if (m_rateControl->rateControlMode == X265_RC_ABR)
        {
            int lookAheadCost = m_lookahead->getEstimatedPictureCost(fenc);  
            m_rateControl->rateControlStart(fenc, lookAheadCost);
            computeLambdaForQp(fenc->getSlice());
        }

        // main encode processing, TBD multi-threading
        curEncoder->m_enable.trigger();
    }

    return ret;
}

double TEncTop::printSummary()
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

    return (m_analyzeAll.getPsnrY() * 6 + m_analyzeAll.getPsnrU() + m_analyzeAll.getPsnrV()) / (8 * m_analyzeAll.getNumPic());
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

double TEncTop::calculateHashAndPSNR(TComPic* pic, AccessUnit& accessUnit)
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
    if (getDecodedPictureHashSEIEnabled())
    {
        SEIDecodedPictureHash sei_recon_picture_digest;
        if (getDecodedPictureHashSEIEnabled() == 1)
        {
            /* calculate MD5sum for entire reconstructed picture */
            sei_recon_picture_digest.method = SEIDecodedPictureHash::MD5;
            calcMD5(*recon, sei_recon_picture_digest.digest);
            digestStr = digestToString(sei_recon_picture_digest.digest, 16);
        }
        else if (getDecodedPictureHashSEIEnabled() == 2)
        {
            sei_recon_picture_digest.method = SEIDecodedPictureHash::CRC;
            calcCRC(*recon, sei_recon_picture_digest.digest);
            digestStr = digestToString(sei_recon_picture_digest.digest, 2);
        }
        else if (getDecodedPictureHashSEIEnabled() == 3)
        {
            sei_recon_picture_digest.method = SEIDecodedPictureHash::CHECKSUM;
            calcChecksum(*recon, sei_recon_picture_digest.digest);
            digestStr = digestToString(sei_recon_picture_digest.digest, 4);
        }

        /* write the SEI messages */
        OutputNALUnit onalu(NAL_UNIT_SUFFIX_SEI, 0);
        m_frameEncoder->m_seiWriter.writeSEImessage(onalu.m_Bitstream, sei_recon_picture_digest, pic->getSlice()->getSPS());
        writeRBSPTrailingBits(onalu.m_Bitstream);

        accessUnit.insert(accessUnit.end(), new NALUnitEBSP(onalu));
    }

    /* calculate the size of the access unit, excluding:
     *  - any AnnexB contributions (start_code_prefix, zero_byte, etc.,)
     *  - SEI NAL units
     */
    UInt numRBSPBytes = 0;
    for (AccessUnit::const_iterator it = accessUnit.begin(); it != accessUnit.end(); it++)
    {
        UInt numRBSPBytes_nal = UInt((*it)->m_nalUnitData.str().size());
#if VERBOSE_RATE
        printf("*** %6s numBytesInNALunit: %u\n", nalUnitTypeToString((*it)->m_nalUnitType), numRBSPBytes_nal);
#endif
        if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
        {
            numRBSPBytes += numRBSPBytes_nal;
        }
    }

    UInt bits = numRBSPBytes * 8;

    /* Acquire encoder global lock to accumulate statistics and print debug info to console */
    ScopedLock s(m_statLock);

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

    if (param.logLevel >= X265_LOG_DEBUG)
    {
        char c = (slice->isIntra() ? 'I' : slice->isInterP() ? 'P' : 'B');

        if (!slice->isReferenced())
            c += 32; // lower case if unreferenced

        fprintf(stderr, "\rPOC %4d ( %c-SLICE, nQP %d QP %d Depth %d) %10d bits",
                slice->getPOC(),
                c,
                slice->getSliceQpBase(),
                slice->getSliceQp(),
                slice->getDepth(),
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
        if (digestStr)
        {
            if (getDecodedPictureHashSEIEnabled() == 1)
            {
                fprintf(stderr, " [MD5:%s]", digestStr);
            }
            else if (getDecodedPictureHashSEIEnabled() == 2)
            {
                fprintf(stderr, " [CRC:%s]", digestStr);
            }
            else if (getDecodedPictureHashSEIEnabled() == 3)
            {
                fprintf(stderr, " [Checksum:%s]", digestStr);
            }
        }
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    return bits;
}

void TEncTop::xInitSPS(TComSPS *pcSPS)
{
    ProfileTierLevel& profileTierLevel = *pcSPS->getPTL()->getGeneralPTL();

    profileTierLevel.setLevelIdc(m_level);
    profileTierLevel.setTierFlag(m_levelTier);
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

    pcSPS->setPicWidthInLumaSamples(param.sourceWidth);
    pcSPS->setPicHeightInLumaSamples(param.sourceHeight);
    pcSPS->setConformanceWindow(m_conformanceWindow);
    pcSPS->setMaxCUWidth(g_maxCUWidth);
    pcSPS->setMaxCUHeight(g_maxCUHeight);
    pcSPS->setMaxCUDepth(g_maxCUDepth);

    int minCUSize = pcSPS->getMaxCUWidth() >> (pcSPS->getMaxCUDepth() - g_addCUDepth);
    int log2MinCUSize = 0;
    while (minCUSize > 1)
    {
        minCUSize >>= 1;
        log2MinCUSize++;
    }

    pcSPS->setLog2MinCodingBlockSize(log2MinCUSize);
    pcSPS->setLog2DiffMaxMinCodingBlockSize(pcSPS->getMaxCUDepth() - g_addCUDepth);

    pcSPS->setPCMLog2MinSize(m_pcmLog2MinSize);
    pcSPS->setUsePCM(m_usePCM);
    pcSPS->setPCMLog2MaxSize(m_pcmLog2MaxSize);

    pcSPS->setQuadtreeTULog2MaxSize(m_quadtreeTULog2MaxSize);
    pcSPS->setQuadtreeTULog2MinSize(m_quadtreeTULog2MinSize);
    pcSPS->setQuadtreeTUMaxDepthInter(param.tuQTMaxInterDepth);
    pcSPS->setQuadtreeTUMaxDepthIntra(param.tuQTMaxIntraDepth);

    pcSPS->setTMVPFlagsPresent(false);
    pcSPS->setUseLossless(m_useLossless);

    pcSPS->setMaxTrSize(1 << m_quadtreeTULog2MaxSize);

    int i;

    for (i = 0; i < g_maxCUDepth - g_addCUDepth; i++)
    {
        pcSPS->setAMPAcc(i, param.bEnableAMP);
    }

    pcSPS->setUseAMP(param.bEnableAMP);

    for (i = g_maxCUDepth - g_addCUDepth; i < g_maxCUDepth; i++)
    {
        pcSPS->setAMPAcc(i, 0);
    }

    pcSPS->setBitDepthY(X265_DEPTH);
    pcSPS->setBitDepthC(X265_DEPTH);

    pcSPS->setQpBDOffsetY(6 * (X265_DEPTH - 8));
    pcSPS->setQpBDOffsetC(6 * (X265_DEPTH - 8));

    pcSPS->setUseSAO(param.bEnableSAO);

    // TODO: hard-code these values in SPS code
    pcSPS->setMaxTLayers(1);
    pcSPS->setTemporalIdNestingFlag(true);
    for (i = 0; i < pcSPS->getMaxTLayers(); i++)
    {
        pcSPS->setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
        pcSPS->setNumReorderPics(m_numReorderPics[i], i);
    }

    // TODO: it is recommended for this to match the input bit depth
    pcSPS->setPCMBitDepthLuma(X265_DEPTH);
    pcSPS->setPCMBitDepthChroma(X265_DEPTH);

    pcSPS->setPCMFilterDisableFlag(m_bPCMFilterDisableFlag);

    pcSPS->setScalingListFlag((m_useScalingListId == 0) ? 0 : 1);

    pcSPS->setUseStrongIntraSmoothing(param.bEnableStrongIntraSmoothing);

    pcSPS->setVuiParametersPresentFlag(getVuiParametersPresentFlag());
    if (pcSPS->getVuiParametersPresentFlag())
    {
        TComVUI* pcVUI = pcSPS->getVuiParameters();
        pcVUI->setAspectRatioInfoPresentFlag(getAspectRatioIdc() != -1);
        pcVUI->setAspectRatioIdc(getAspectRatioIdc());
        pcVUI->setSarWidth(getSarWidth());
        pcVUI->setSarHeight(getSarHeight());
        pcVUI->setOverscanInfoPresentFlag(getOverscanInfoPresentFlag());
        pcVUI->setOverscanAppropriateFlag(getOverscanAppropriateFlag());
        pcVUI->setVideoSignalTypePresentFlag(getVideoSignalTypePresentFlag());
        pcVUI->setVideoFormat(getVideoFormat());
        pcVUI->setVideoFullRangeFlag(getVideoFullRangeFlag());
        pcVUI->setColourDescriptionPresentFlag(getColourDescriptionPresentFlag());
        pcVUI->setColourPrimaries(getColourPrimaries());
        pcVUI->setTransferCharacteristics(getTransferCharacteristics());
        pcVUI->setMatrixCoefficients(getMatrixCoefficients());
        pcVUI->setChromaLocInfoPresentFlag(getChromaLocInfoPresentFlag());
        pcVUI->setChromaSampleLocTypeTopField(getChromaSampleLocTypeTopField());
        pcVUI->setChromaSampleLocTypeBottomField(getChromaSampleLocTypeBottomField());
        pcVUI->setNeutralChromaIndicationFlag(getNeutralChromaIndicationFlag());
        pcVUI->setDefaultDisplayWindow(getDefaultDisplayWindow());
        pcVUI->setFrameFieldInfoPresentFlag(getFrameFieldInfoPresentFlag());
        pcVUI->setFieldSeqFlag(false);
        pcVUI->setHrdParametersPresentFlag(false);
        pcVUI->getTimingInfo()->setPocProportionalToTimingFlag(getPocProportionalToTimingFlag());
        pcVUI->getTimingInfo()->setNumTicksPocDiffOneMinus1(getNumTicksPocDiffOneMinus1());
        pcVUI->setBitstreamRestrictionFlag(getBitstreamRestrictionFlag());
        pcVUI->setMotionVectorsOverPicBoundariesFlag(getMotionVectorsOverPicBoundariesFlag());
        pcVUI->setMinSpatialSegmentationIdc(getMinSpatialSegmentationIdc());
        pcVUI->setMaxBytesPerPicDenom(getMaxBytesPerPicDenom());
        pcVUI->setMaxBitsPerMinCuDenom(getMaxBitsPerMinCuDenom());
        pcVUI->setLog2MaxMvLengthHorizontal(getLog2MaxMvLengthHorizontal());
        pcVUI->setLog2MaxMvLengthVertical(getLog2MaxMvLengthVertical());
    }

    /* set the VPS profile information */
    *getVPS()->getPTL() = *pcSPS->getPTL();
    getVPS()->getTimingInfo()->setTimingInfoPresentFlag(false);
}

void TEncTop::xInitPPS(TComPPS *pcPPS)
{
    pcPPS->setConstrainedIntraPred(param.bEnableConstrainedIntra);
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
        pcPPS->setUseDQP(true);
        pcPPS->setMaxCuDQPDepth(m_maxCuDQPDepth);
        pcPPS->setMinCuDQPSize(pcPPS->getSPS()->getMaxCUWidth() >> (pcPPS->getMaxCuDQPDepth()));
    }
    else
    {
        pcPPS->setUseDQP(false);
        pcPPS->setMaxCuDQPDepth(0);
        pcPPS->setMinCuDQPSize(pcPPS->getSPS()->getMaxCUWidth() >> (pcPPS->getMaxCuDQPDepth()));
    }

    pcPPS->setChromaCbQpOffset(param.cbQpOffset);
    pcPPS->setChromaCrQpOffset(param.crQpOffset);

    pcPPS->setEntropyCodingSyncEnabledFlag(param.bEnableWavefront);
    pcPPS->setUseWP(param.bEnableWeightedPred);
    pcPPS->setWPBiPred(param.bEnableWeightedBiPred);
    pcPPS->setOutputFlagPresentFlag(false);
    pcPPS->setSignHideFlag(param.bEnableSignHiding);
    pcPPS->setDeblockingFilterControlPresentFlag(!param.bEnableLoopFilter);
    pcPPS->setDeblockingFilterOverrideEnabledFlag(!m_loopFilterOffsetInPPS);
    pcPPS->setPicDisableDeblockingFilterFlag(!param.bEnableLoopFilter);
    pcPPS->setLog2ParallelMergeLevelMinus2(m_log2ParallelMergeLevelMinus2);
    pcPPS->setCabacInitPresentFlag(CABAC_INIT_PRESENT_FLAG);

    /* TODO: this must be replaced with a user-parameter or hard-coded value */
    int histogram[MAX_NUM_REF + 1];
    for (int i = 0; i <= MAX_NUM_REF; i++)
    {
        histogram[i] = 0;
    }

    for (int i = 0; i < getGOPSize(); i++)
    {
        assert(getGOPEntry(i).m_numRefPicsActive >= 0 && getGOPEntry(i).m_numRefPicsActive <= MAX_NUM_REF);
        histogram[getGOPEntry(i).m_numRefPicsActive]++;
    }

    int maxHist = -1;
    int bestPos = 0;
    for (int i = 0; i <= MAX_NUM_REF; i++)
    {
        if (histogram[i] > maxHist)
        {
            maxHist = histogram[i];
            bestPos = i;
        }
    }

    assert(bestPos <= 15);
    pcPPS->setNumRefIdxL0DefaultActive(bestPos);
    pcPPS->setNumRefIdxL1DefaultActive(bestPos);

    pcPPS->setTransquantBypassEnableFlag(getTransquantBypassEnableFlag());
    pcPPS->setUseTransformSkip(param.bEnableTransformSkip);
    pcPPS->setLoopFilterAcrossTilesEnabledFlag(m_loopFilterAcrossTilesEnabledFlag);
}

void TEncTop::computeLambdaForQp(TComSlice* slice)
{
    FrameEncoder *curEncoder = &m_frameEncoder[m_curEncoder];
    int qp = slice->getSliceQp();
    int lambda = x265_lambda2_tab[qp] >> 8;

    // for RDO
    // in RdCost there is only one lambda because the luma and chroma bits are not separated,
    // instead we weight the distortion of chroma.
    double weight = 1.0;
    int qpc;
    int chromaQPOffset;

    chromaQPOffset = slice->getPPS()->getChromaCbQpOffset() + slice->getSliceQpDeltaCb();
    qpc = Clip3(0, 57, qp + chromaQPOffset);
    weight = pow(2.0, (qp - g_chromaScale[qpc])); // takes into account of the chroma qp mapping and chroma qp Offset
    curEncoder->setCbDistortionWeight(weight);
    chromaQPOffset = slice->getPPS()->getChromaCrQpOffset() + slice->getSliceQpDeltaCr();
    qpc = Clip3(0, 57, qp + chromaQPOffset);
    weight = pow(2.0, (qp - g_chromaScale[qpc])); // takes into account of the chroma qp mapping and chroma qp Offset
    curEncoder->setCrDistortionWeight(weight);

    // for RDOQ
    curEncoder->setQPLambda(qp, lambda, lambda / weight, slice->getDepth());

    // For SAO
    slice->setLambda(lambda, lambda / weight);

    slice->setSliceQpBase(qp);
    slice->setSliceQpDelta(0);
    slice->setSliceQpDeltaCb(0);
    slice->setSliceQpDeltaCr(0);
}

//! \}
