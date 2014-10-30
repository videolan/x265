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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "primitives.h"
#include "threadpool.h"
#include "param.h"
#include "frame.h"
#include "framedata.h"
#include "picyuv.h"

#include "bitcost.h"
#include "encoder.h"
#include "slicetype.h"
#include "frameencoder.h"
#include "ratecontrol.h"
#include "dpb.h"
#include "nal.h"

#include "x265.h"

namespace x265 {
const char g_sliceTypeToChar[] = {'B', 'P', 'I'};
}

static const char *summaryCSVHeader =
    "Command, Date/Time, Elapsed Time, FPS, Bitrate, "
    "Y PSNR, U PSNR, V PSNR, Global PSNR, SSIM, SSIM (dB), "
    "I count, I ave-QP, I kpbs, I-PSNR Y, I-PSNR U, I-PSNR V, I-SSIM (dB), "
    "P count, P ave-QP, P kpbs, P-PSNR Y, P-PSNR U, P-PSNR V, P-SSIM (dB), "
    "B count, B ave-QP, B kpbs, B-PSNR Y, B-PSNR U, B-PSNR V, B-SSIM (dB), "
    "Version\n";

using namespace x265;

Encoder::Encoder()
{
    m_aborted = false;
    m_encodedFrameNum = 0;
    m_pocLast = -1;
    m_curEncoder = 0;
    m_numLumaWPFrames = 0;
    m_numChromaWPFrames = 0;
    m_numLumaWPBiFrames = 0;
    m_numChromaWPBiFrames = 0;
    m_lookahead = NULL;
    m_frameEncoder = NULL;
    m_rateControl = NULL;
    m_dpb = NULL;
    m_exportedPic = NULL;
    m_numDelayedPic = 0;
    m_outputCount = 0;
    m_csvfpt = NULL;
    m_param = NULL;
    m_cuOffsetY = NULL;
    m_cuOffsetC = NULL;
    m_buOffsetY = NULL;
    m_buOffsetC = NULL;
    m_threadPool = 0;
    m_numThreadLocalData = 0;
}

void Encoder::create()
{
    if (!primitives.sad[0])
    {
        // this should be an impossible condition when using our public API, and indicates a serious bug.
        x265_log(m_param, X265_LOG_ERROR, "Primitives must be initialized before encoder is created\n");
        abort();
    }

    x265_param* p = m_param;

    int rows = (p->sourceHeight + p->maxCUSize - 1) >> g_log2Size[p->maxCUSize];

    // Do not allow WPP if only one row, it is pointless and unstable
    if (rows == 1)
        p->bEnableWavefront = 0;

    int poolThreadCount = p->poolNumThreads ? p->poolNumThreads : getCpuCount();

    // Trim the thread pool if --wpp, --pme, and --pmode are disabled
    if (!p->bEnableWavefront && !p->bDistributeModeAnalysis && !p->bDistributeMotionEstimation)
        poolThreadCount = 0;

    if (poolThreadCount > 1)
    {
        m_threadPool = ThreadPool::allocThreadPool(poolThreadCount);
        poolThreadCount = m_threadPool->getThreadCount();
    }
    else
        poolThreadCount = 0;

    if (!poolThreadCount)
    {
        // issue warnings if any of these features were requested
        if (p->bEnableWavefront)
            x265_log(p, X265_LOG_WARNING, "No thread pool allocated, --wpp disabled\n");
        if (p->bDistributeMotionEstimation)
            x265_log(p, X265_LOG_WARNING, "No thread pool allocated, --pme disabled\n");
        if (p->bDistributeModeAnalysis)
            x265_log(p, X265_LOG_WARNING, "No thread pool allocated, --pmode disabled\n");

        // disable all pool features if the thread pool is disabled or unusable.
        p->bEnableWavefront = p->bDistributeModeAnalysis = p->bDistributeMotionEstimation = 0;
    }

    if (!p->frameNumThreads)
    {
        // auto-detect frame threads
        int cpuCount = getCpuCount();
        if (!p->bEnableWavefront)
            p->frameNumThreads = X265_MIN(cpuCount, (rows + 1) / 2);
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

    x265_log(p, X265_LOG_INFO, "WPP streams / frame threads / pool  : %d / %d / %d%s%s\n", 
             p->bEnableWavefront ? rows : 0, p->frameNumThreads, poolThreadCount,
             p->bDistributeMotionEstimation ? " / pme" : "", p->bDistributeModeAnalysis ? " / pmode" : "");

    m_frameEncoder = new FrameEncoder[m_param->frameNumThreads];
    for (int i = 0; i < m_param->frameNumThreads; i++)
        m_frameEncoder[i].setThreadPool(m_threadPool);

    if (!m_scalingList.init())
    {
        x265_log(m_param, X265_LOG_ERROR, "Unable to allocate scaling list arrays\n");
        m_aborted = true;
    }
    else if (!m_param->scalingLists || !strcmp(m_param->scalingLists, "off"))
        m_scalingList.m_bEnabled = false;
    else if (!strcmp(m_param->scalingLists, "default"))
        m_scalingList.setDefaultScalingList();
    else if (m_scalingList.parseScalingList(m_param->scalingLists))
        m_aborted = true;
    m_scalingList.setupQuantMatrices();

    /* Allocate thread local data, one for each thread pool worker and
     * if --no-wpp, one for each frame encoder */
    m_numThreadLocalData = poolThreadCount;
    if (!m_param->bEnableWavefront)
        m_numThreadLocalData += m_param->frameNumThreads;
    m_threadLocalData = new ThreadLocalData[m_numThreadLocalData];
    for (int i = 0; i < m_numThreadLocalData; i++)
    {
        m_threadLocalData[i].analysis.setThreadPool(m_threadPool);
        m_threadLocalData[i].analysis.initSearch(*m_param, m_scalingList);
        m_threadLocalData[i].analysis.create(m_threadLocalData);
    }

    if (!m_param->bEnableWavefront)
        for (int i = 0; i < m_param->frameNumThreads; i++)
            m_frameEncoder[i].m_tld = &m_threadLocalData[poolThreadCount + i];

    m_lookahead = new Lookahead(m_param, m_threadPool);
    m_dpb = new DPB(m_param);
    m_rateControl = new RateControl(m_param);

    initSPS(&m_sps);
    initPPS(&m_pps);

    /* Try to open CSV file handle */
    if (m_param->csvfn)
    {
        m_csvfpt = fopen(m_param->csvfn, "r");
        if (m_csvfpt)
        {
            // file already exists, re-open for append
            fclose(m_csvfpt);
            m_csvfpt = fopen(m_param->csvfn, "ab");
        }
        else
        {
            // new CSV file, write header
            m_csvfpt = fopen(m_param->csvfn, "wb");
            if (m_csvfpt)
            {
                if (m_param->logLevel >= X265_LOG_DEBUG)
                {
                    fprintf(m_csvfpt, "Encode Order, Type, POC, QP, Bits, ");
                    if (m_param->rc.rateControlMode == X265_RC_CRF)
                        fprintf(m_csvfpt, "RateFactor, ");
                    fprintf(m_csvfpt, "Y PSNR, U PSNR, V PSNR, YUV PSNR, SSIM, SSIM (dB), "
                                      "Encoding time, Elapsed time, List 0, List 1\n");
                }
                else
                    fputs(summaryCSVHeader, m_csvfpt);
            }
        }
    }

    m_aborted |= parseLambdaFile(m_param);
}

void Encoder::destroy()
{
    if (m_exportedPic)
    {
        ATOMIC_DEC(&m_exportedPic->m_countRefEncoders);
        m_exportedPic = NULL;
    }

    if (m_rateControl)
        m_rateControl->terminate(); // unblock all blocked RC calls

    if (m_frameEncoder)
    {
        for (int i = 0; i < m_param->frameNumThreads; i++)
        {
            // Ensure frame encoder is idle before destroying it
            m_frameEncoder[i].getEncodedPicture(m_nalList);
            m_frameEncoder[i].destroy();
        }

        delete [] m_frameEncoder;
    }

    for (int i = 0; i < m_numThreadLocalData; i++)
        m_threadLocalData[i].destroy();

    delete [] m_threadLocalData;

    if (m_lookahead)
    {
        m_lookahead->destroy();
        delete m_lookahead;
    }

    delete m_dpb;
    if (m_rateControl)
    {
        m_rateControl->destroy();
        delete m_rateControl;
    }
    // thread pool release should always happen last
    if (m_threadPool)
        m_threadPool->release();

    X265_FREE(m_cuOffsetY);
    X265_FREE(m_cuOffsetC);
    X265_FREE(m_buOffsetY);
    X265_FREE(m_buOffsetC);

    if (m_csvfpt)
        fclose(m_csvfpt);
    free(m_param->rc.statFileName); // alloc'd by strdup

    X265_FREE(m_param);
}

void Encoder::init()
{
    if (m_frameEncoder)
    {
        int numRows = (m_param->sourceHeight + g_maxCUSize - 1) / g_maxCUSize;
        int numCols = (m_param->sourceWidth  + g_maxCUSize - 1) / g_maxCUSize;
        for (int i = 0; i < m_param->frameNumThreads; i++)
        {
            if (!m_frameEncoder[i].init(this, numRows, numCols, i))
            {
                x265_log(m_param, X265_LOG_ERROR, "Unable to initialize frame encoder, aborting\n");
                m_aborted = true;
            }
        }
    }
    if (m_param->bEmitHRDSEI)
        m_rateControl->initHRD(&m_sps);
    if (!m_rateControl->init(&m_sps))
        m_aborted = true;
    m_lookahead->init();
    m_encodeStartTime = x265_mdate();
}

void Encoder::updateVbvPlan(RateControl* rc)
{
    for (int i = 0; i < m_param->frameNumThreads; i++)
    {
        FrameEncoder *encoder = &m_frameEncoder[i];
        if (encoder->m_rce.isActive && encoder->m_rce.poc != rc->m_curSlice->m_poc)
        {
            int64_t bits = (int64_t) X265_MAX(encoder->m_rce.frameSizeEstimated, encoder->m_rce.frameSizePlanned);
            rc->m_bufferFill -= bits;
            rc->m_bufferFill = X265_MAX(rc->m_bufferFill, 0);
            rc->m_bufferFill += encoder->m_rce.bufferRate;
            rc->m_bufferFill = X265_MIN(rc->m_bufferFill, rc->m_bufferSize);
            if (rc->m_2pass)
                rc->m_predictedBits += bits;
        }
    }
}

/**
 * Feed one new input frame into the encoder, get one frame out. If pic_in is
 * NULL, a flush condition is implied and pic_in must be NULL for all subsequent
 * calls for this encoder instance.
 *
 * pic_in  input original YUV picture or NULL
 * pic_out pointer to reconstructed picture struct
 *
 * returns 0 if no frames are currently available for output
 *         1 if frame was output, m_nalList contains access unit
 *         negative on malloc error or abort */
int Encoder::encode(const x265_picture* pic_in, x265_picture* pic_out)
{
    if (m_aborted)
        return -1;

    if (m_exportedPic)
    {
        ATOMIC_DEC(&m_exportedPic->m_countRefEncoders);
        m_exportedPic = NULL;
        m_dpb->recycleUnreferenced();
    }

    if (pic_in)
    {
        if (pic_in->colorSpace != m_param->internalCsp)
        {
            x265_log(m_param, X265_LOG_ERROR, "Unsupported color space (%d) on input\n",
                     pic_in->colorSpace);
            return -1;
        }
        if (pic_in->bitDepth < 8 || pic_in->bitDepth > 16)
        {
            x265_log(m_param, X265_LOG_ERROR, "Input bit depth (%d) must be between 8 and 16\n",
                     pic_in->bitDepth);
            return -1;
        }

        Frame *inFrame;
        if (m_dpb->m_freeList.empty())
        {
            inFrame = new Frame;
            if (inFrame->create(m_param))
            {
                /* the first PicYuv created is asked to generate the CU and block unit offset
                 * arrays which are then shared with all subsequent PicYuv (orig and recon) 
                 * allocated by this top level encoder */
                if (m_cuOffsetY)
                {
                    inFrame->m_origPicYuv->m_cuOffsetC = m_cuOffsetC;
                    inFrame->m_origPicYuv->m_cuOffsetY = m_cuOffsetY;
                    inFrame->m_origPicYuv->m_buOffsetC = m_buOffsetC;
                    inFrame->m_origPicYuv->m_buOffsetY = m_buOffsetY;
                }
                else
                {
                    if (!inFrame->m_origPicYuv->createOffsets(m_sps))
                    {
                        m_aborted = true;
                        x265_log(m_param, X265_LOG_ERROR, "memory allocation failure, aborting encode\n");
                        inFrame->destroy();
                        delete inFrame;
                        return -1;
                    }
                    else
                    {
                        m_cuOffsetC = inFrame->m_origPicYuv->m_cuOffsetC;
                        m_cuOffsetY = inFrame->m_origPicYuv->m_cuOffsetY;
                        m_buOffsetC = inFrame->m_origPicYuv->m_buOffsetC;
                        m_buOffsetY = inFrame->m_origPicYuv->m_buOffsetY;
                    }
                }
            }
            else
            {
                m_aborted = true;
                x265_log(m_param, X265_LOG_ERROR, "memory allocation failure, aborting encode\n");
                inFrame->destroy();
                delete inFrame;
                return -1;
            }
        }
        else
            inFrame = m_dpb->m_freeList.popBack();

        /* Copy input picture into a Frame and PicYuv, send to lookahead */
        inFrame->m_poc = ++m_pocLast;
        inFrame->m_origPicYuv->copyFromPicture(*pic_in, m_sps.conformanceWindow.rightOffset, m_sps.conformanceWindow.bottomOffset);
        inFrame->m_intraData = pic_in->analysisData.intraData;
        inFrame->m_interData = pic_in->analysisData.interData;
        inFrame->m_userData  = pic_in->userData;
        inFrame->m_pts       = pic_in->pts;
        inFrame->m_forceqp   = pic_in->forceqp;

        if (m_pocLast == 0)
            m_firstPts = inFrame->m_pts;
        if (m_bframeDelay && m_pocLast == m_bframeDelay)
            m_bframeDelayTime = inFrame->m_pts - m_firstPts;

        /* Encoder holds a reference count until stats collection is finished */
        ATOMIC_INC(&inFrame->m_countRefEncoders);
        bool bEnableWP = m_param->bEnableWeightedPred || m_param->bEnableWeightedBiPred;
        if (m_param->rc.aqMode || bEnableWP)
        {
            if (m_param->rc.cuTree && m_param->rc.bStatRead)
            {
                if (!m_rateControl->cuTreeReadFor2Pass(inFrame))
                {
                    m_aborted = 1;
                    return -1;
                }
            }
            else
                m_rateControl->calcAdaptiveQuantFrame(inFrame);
        }

        /* Use the frame types from the first pass, if available */
        int sliceType = (m_param->rc.bStatRead) ? m_rateControl->rateControlSliceType(inFrame->m_poc) : pic_in->sliceType;
        m_lookahead->addPicture(inFrame, sliceType);
        m_numDelayedPic++;
    }
    else
        m_lookahead->flush();

    FrameEncoder *curEncoder = &m_frameEncoder[m_curEncoder];
    m_curEncoder = (m_curEncoder + 1) % m_param->frameNumThreads;
    int ret = 0;

    // getEncodedPicture() should block until the FrameEncoder has completed
    // encoding the frame.  This is how back-pressure through the API is
    // accomplished when the encoder is full.
    Frame *outFrame = curEncoder->getEncodedPicture(m_nalList);

    if (outFrame)
    {
        Slice *slice = outFrame->m_encData->m_slice;
        if (pic_out)
        {
            PicYuv *recpic = outFrame->m_reconPicYuv;
            pic_out->poc = slice->m_poc;
            pic_out->bitDepth = X265_DEPTH;
            pic_out->userData = outFrame->m_userData;
            pic_out->colorSpace = m_param->internalCsp;

            pic_out->pts = outFrame->m_pts;
            pic_out->dts = outFrame->m_dts;

            switch (slice->m_sliceType)
            {
            case I_SLICE:
                pic_out->sliceType = outFrame->m_lowres.bKeyframe ? X265_TYPE_IDR : X265_TYPE_I;
                break;
            case P_SLICE:
                pic_out->sliceType = X265_TYPE_P;
                break;
            case B_SLICE:
                pic_out->sliceType = X265_TYPE_B;
                break;
            }

            pic_out->planes[0] = recpic->m_picOrg[0];
            pic_out->stride[0] = (int)(recpic->m_stride * sizeof(pixel));
            pic_out->planes[1] = recpic->m_picOrg[1];
            pic_out->stride[1] = (int)(recpic->m_strideC * sizeof(pixel));
            pic_out->planes[2] = recpic->m_picOrg[2];
            pic_out->stride[2] = (int)(recpic->m_strideC * sizeof(pixel));
        }

        if (m_param->analysisMode)
        {
            pic_out->analysisData.interData = outFrame->m_interData;
            pic_out->analysisData.intraData = outFrame->m_intraData;
            pic_out->analysisData.numCUsInFrame = slice->m_sps->numCUsInFrame;
            pic_out->analysisData.numPartitions = slice->m_sps->numPartitions;
        }

        if (slice->m_sliceType == P_SLICE)
        {
            if (slice->m_weightPredTable[0][0][0].bPresentFlag)
                m_numLumaWPFrames++;
            if (slice->m_weightPredTable[0][0][1].bPresentFlag ||
                slice->m_weightPredTable[0][0][2].bPresentFlag)
                m_numChromaWPFrames++;
        }
        else if (slice->m_sliceType == B_SLICE)
        {
            bool bLuma = false, bChroma = false;
            for (int l = 0; l < 2; l++)
            {
                if (slice->m_weightPredTable[l][0][0].bPresentFlag)
                    bLuma = true;
                if (slice->m_weightPredTable[l][0][1].bPresentFlag ||
                    slice->m_weightPredTable[l][0][2].bPresentFlag)
                    bChroma = true;
            }

            if (bLuma)
                m_numLumaWPBiFrames++;
            if (bChroma)
                m_numChromaWPBiFrames++;
        }
        if (m_aborted)
            return -1;

        finishFrameStats(outFrame, curEncoder, curEncoder->m_accessUnitBits);
        // Allow this frame to be recycled if no frame encoders are using it for reference
        if (!pic_out)
        {
            ATOMIC_DEC(&outFrame->m_countRefEncoders);
            m_dpb->recycleUnreferenced();
        }
        else
            m_exportedPic = outFrame;

        m_numDelayedPic--;

        ret = 1;
    }

    // pop a single frame from decided list, then provide to frame encoder
    // curEncoder is guaranteed to be idle at this point
    Frame* frameEnc = m_lookahead->getDecidedPicture();
    if (frameEnc)
    {
        // give this picture a FrameData instance before encoding
        if (m_dpb->m_picSymFreeList)
        {
            frameEnc->m_encData = m_dpb->m_picSymFreeList;
            m_dpb->m_picSymFreeList = m_dpb->m_picSymFreeList->m_freeListNext;
            frameEnc->reinit(m_sps);
        }
        else
        {
            frameEnc->allocEncodeData(m_param, m_sps);
            Slice* slice = frameEnc->m_encData->m_slice;
            slice->m_sps = &m_sps;
            slice->m_pps = &m_pps;
            slice->m_maxNumMergeCand = m_param->maxNumMergeCand;
            slice->m_endCUAddr = slice->realEndAddress(m_sps.numCUsInFrame * NUM_CU_PARTITIONS);
            frameEnc->m_reconPicYuv->m_cuOffsetC = m_cuOffsetC;
            frameEnc->m_reconPicYuv->m_cuOffsetY = m_cuOffsetY;
            frameEnc->m_reconPicYuv->m_buOffsetC = m_buOffsetC;
            frameEnc->m_reconPicYuv->m_buOffsetY = m_buOffsetY;
        }
        curEncoder->m_rce.encodeOrder = m_encodedFrameNum++;
        if (m_bframeDelay)
        {
            int64_t *prevReorderedPts = m_prevReorderedPts;
            frameEnc->m_dts = m_encodedFrameNum > m_bframeDelay
                ? prevReorderedPts[(m_encodedFrameNum - m_bframeDelay) % m_bframeDelay]
                : frameEnc->m_reorderedPts - m_bframeDelayTime;
            prevReorderedPts[m_encodedFrameNum % m_bframeDelay] = frameEnc->m_reorderedPts;
        }
        else
            frameEnc->m_dts = frameEnc->m_reorderedPts;

        // determine references, setup RPS, etc
        m_dpb->prepareEncode(frameEnc);

        if (m_param->rc.rateControlMode != X265_RC_CQP)
            m_lookahead->getEstimatedPictureCost(frameEnc);

        // Allow FrameEncoder::compressFrame() to start in the frame encoder thread
        if (!curEncoder->startCompressFrame(frameEnc))
            m_aborted = true;
    }
    else if (m_encodedFrameNum)
        m_rateControl->setFinalFrameCount(m_encodedFrameNum);

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

char* Encoder::statsCSVString(EncStats& stat, char* buffer)
{
    if (!stat.m_numPics)
    {
        sprintf(buffer, "-, -, -, -, -, -, -, ");
        return buffer;
    }

    double fps = (double)m_param->fpsNum / m_param->fpsDenom;
    double scale = fps / 1000 / (double)stat.m_numPics;

    int len = sprintf(buffer, "%-6u, ", stat.m_numPics);

    len += sprintf(buffer + len, "%2.2lf, ", stat.m_totalQp / (double)stat.m_numPics);
    len += sprintf(buffer + len, "%-8.2lf, ", stat.m_accBits * scale);
    if (m_param->bEnablePsnr)
    {
        len += sprintf(buffer + len, "%.3lf, %.3lf, %.3lf, ",
                       stat.m_psnrSumY / (double)stat.m_numPics,
                       stat.m_psnrSumU / (double)stat.m_numPics,
                       stat.m_psnrSumV / (double)stat.m_numPics);
    }
    else
        len += sprintf(buffer + len, "-, -, -, ");

    if (m_param->bEnableSsim)
        sprintf(buffer + len, "%.3lf, ", x265_ssim2dB(stat.m_globalSsim / (double)stat.m_numPics));
    else
        sprintf(buffer + len, "-, ");
    return buffer;
}

char* Encoder::statsString(EncStats& stat, char* buffer)
{
    double fps = (double)m_param->fpsNum / m_param->fpsDenom;
    double scale = fps / 1000 / (double)stat.m_numPics;

    int len = sprintf(buffer, "%6u, ", stat.m_numPics);

    len += sprintf(buffer + len, "Avg QP:%2.2lf", stat.m_totalQp / (double)stat.m_numPics);
    len += sprintf(buffer + len, "  kb/s: %-8.2lf", stat.m_accBits * scale);
    if (m_param->bEnablePsnr)
    {
        len += sprintf(buffer + len, "  PSNR Mean: Y:%.3lf U:%.3lf V:%.3lf",
                       stat.m_psnrSumY / (double)stat.m_numPics,
                       stat.m_psnrSumU / (double)stat.m_numPics,
                       stat.m_psnrSumV / (double)stat.m_numPics);
    }
    if (m_param->bEnableSsim)
    {
        sprintf(buffer + len, "  SSIM Mean: %.6lf (%.3lfdB)",
                stat.m_globalSsim / (double)stat.m_numPics,
                x265_ssim2dB(stat.m_globalSsim / (double)stat.m_numPics));
    }
    return buffer;
}

void Encoder::printSummary()
{
    if (m_param->logLevel < X265_LOG_INFO)
        return;

    char buffer[200];
    if (m_analyzeI.m_numPics)
        x265_log(m_param, X265_LOG_INFO, "frame I: %s\n", statsString(m_analyzeI, buffer));
    if (m_analyzeP.m_numPics)
        x265_log(m_param, X265_LOG_INFO, "frame P: %s\n", statsString(m_analyzeP, buffer));
    if (m_analyzeB.m_numPics)
        x265_log(m_param, X265_LOG_INFO, "frame B: %s\n", statsString(m_analyzeB, buffer));
    if (m_analyzeAll.m_numPics)
        x265_log(m_param, X265_LOG_INFO, "global : %s\n", statsString(m_analyzeAll, buffer));
    if (m_param->bEnableWeightedPred && m_analyzeP.m_numPics)
    {
        x265_log(m_param, X265_LOG_INFO, "Weighted P-Frames: Y:%.1f%% UV:%.1f%%\n",
            (float)100.0 * m_numLumaWPFrames / m_analyzeP.m_numPics,
            (float)100.0 * m_numChromaWPFrames / m_analyzeP.m_numPics);
    }
    if (m_param->bEnableWeightedBiPred && m_analyzeB.m_numPics)
    {
        x265_log(m_param, X265_LOG_INFO, "Weighted B-Frames: Y:%.1f%% UV:%.1f%%\n",
            (float)100.0 * m_numLumaWPBiFrames / m_analyzeB.m_numPics,
            (float)100.0 * m_numChromaWPBiFrames / m_analyzeB.m_numPics);
    }
    int pWithB = 0;
    for (int i = 0; i <= m_param->bframes; i++)
        pWithB += m_lookahead->m_histogram[i];

    if (pWithB)
    {
        int p = 0;
        for (int i = 0; i <= m_param->bframes; i++)
            p += sprintf(buffer + p, "%.1f%% ", 100. * m_lookahead->m_histogram[i] / pWithB);

        x265_log(m_param, X265_LOG_INFO, "consecutive B-frames: %s\n", buffer);
    }
    if (m_param->bLossless)
    {
        float frameSize = (float)(m_param->sourceWidth - m_sps.conformanceWindow.rightOffset) *
                                 (m_param->sourceHeight - m_sps.conformanceWindow.bottomOffset);
        float uncompressed = frameSize * X265_DEPTH * m_analyzeAll.m_numPics;

        x265_log(m_param, X265_LOG_INFO, "lossless compression ratio %.2f::1\n", uncompressed / m_analyzeAll.m_accBits);
    }

    if (!m_param->bLogCuStats)
        return;

    for (int sliceType = 2; sliceType >= 0; sliceType--)
    {
        if (sliceType == P_SLICE && !m_analyzeP.m_numPics)
            continue;
        if (sliceType == B_SLICE && !m_analyzeB.m_numPics)
            continue;

        StatisticLog finalLog;
        for (uint32_t depth = 0; depth <= g_maxCUDepth; depth++)
        {
            for (int i = 0; i < m_param->frameNumThreads; i++)
            {
                StatisticLog& enclog = m_frameEncoder[i].m_sliceTypeLog[sliceType];
                if (!depth)
                    finalLog.totalCu += enclog.totalCu;
                finalLog.cntIntra[depth] += enclog.cntIntra[depth];
                for (int m = 0; m < INTER_MODES; m++)
                {
                    if (m < INTRA_MODES)
                        finalLog.cuIntraDistribution[depth][m] += enclog.cuIntraDistribution[depth][m];
                    finalLog.cuInterDistribution[depth][m] += enclog.cuInterDistribution[depth][m];
                }

                if (depth == g_maxCUDepth)
                    finalLog.cntIntraNxN += enclog.cntIntraNxN;
                if (sliceType != I_SLICE)
                {
                    finalLog.cntTotalCu[depth] += enclog.cntTotalCu[depth];
                    finalLog.cntInter[depth] += enclog.cntInter[depth];
                    finalLog.cntSkipCu[depth] += enclog.cntSkipCu[depth];
                }
            }

            uint64_t cntInter, cntSkipCu, cntIntra = 0, cntIntraNxN = 0, encCu = 0;
            uint64_t cuInterDistribution[INTER_MODES], cuIntraDistribution[INTRA_MODES];

            // check for 0/0, if true assign 0 else calculate percentage
            for (int n = 0; n < INTER_MODES; n++)
            {
                if (!finalLog.cntInter[depth])
                    cuInterDistribution[n] = 0;
                else
                    cuInterDistribution[n] = (finalLog.cuInterDistribution[depth][n] * 100) / finalLog.cntInter[depth];

                if (n < INTRA_MODES)
                {
                    if (!finalLog.cntIntra[depth])
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

            if (!finalLog.totalCu)
                encCu = 0;
            else if (sliceType == I_SLICE)
            {
                cntIntra = (finalLog.cntIntra[depth] * 100) / finalLog.totalCu;
                cntIntraNxN = (finalLog.cntIntraNxN * 100) / finalLog.totalCu;
            }
            else
                encCu = ((finalLog.cntIntra[depth] + finalLog.cntInter[depth]) * 100) / finalLog.totalCu;

            if (sliceType == I_SLICE)
            {
                cntInter = 0;
                cntSkipCu = 0;
            }
            else if (!finalLog.cntTotalCu[depth])
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

            // print statistics
            int cuSize = g_maxCUSize >> depth;
            char stats[256] = { 0 };
            int len = 0;
            if (sliceType != I_SLICE)
                len += sprintf(stats + len, " EncCU "X265_LL "%% Merge "X265_LL "%%", encCu, cntSkipCu);

            if (cntInter)
            {
                len += sprintf(stats + len, " Inter "X265_LL "%%", cntInter);
                if (m_param->bEnableAMP)
                    len += sprintf(stats + len, "(%dx%d "X265_LL "%% %dx%d "X265_LL "%% %dx%d "X265_LL "%% AMP "X265_LL "%%)",
                                   cuSize, cuSize, cuInterDistribution[0],
                                   cuSize / 2, cuSize, cuInterDistribution[2],
                                   cuSize, cuSize / 2, cuInterDistribution[1],
                                   cuInterDistribution[3]);
                else if (m_param->bEnableRectInter)
                    len += sprintf(stats + len, "(%dx%d "X265_LL "%% %dx%d "X265_LL "%% %dx%d "X265_LL "%%)",
                                   cuSize, cuSize, cuInterDistribution[0],
                                   cuSize / 2, cuSize, cuInterDistribution[2],
                                   cuSize, cuSize / 2, cuInterDistribution[1]);
            }
            if (cntIntra)
            {
                len += sprintf(stats + len, " Intra "X265_LL "%%(DC "X265_LL "%% P "X265_LL "%% Ang "X265_LL "%%",
                               cntIntra, cuIntraDistribution[0],
                               cuIntraDistribution[1], cuIntraDistribution[2]);
                if (sliceType != I_SLICE)
                {
                    if (depth == g_maxCUDepth)
                        len += sprintf(stats + len, " %dx%d "X265_LL "%%", cuSize / 2, cuSize / 2, cntIntraNxN);
                }

                len += sprintf(stats + len, ")");
                if (sliceType == I_SLICE)
                {
                    if (depth == g_maxCUDepth)
                        len += sprintf(stats + len, " %dx%d: "X265_LL "%%", cuSize / 2, cuSize / 2, cntIntraNxN);
                }
            }
            const char slicechars[] = "BPI";
            if (stats[0])
                x265_log(m_param, X265_LOG_INFO, "%c%-2d:%s\n", slicechars[sliceType], cuSize, stats);
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
            stats->elapsedVideoTime = (double)stats->encodedPictureCount * m_param->fpsDenom / m_param->fpsNum;
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
        if (m_param->logLevel >= X265_LOG_DEBUG)
        {
            // adding summary to a per-frame csv log file needs a summary header
            fprintf(m_csvfpt, "\nSummary\n");
            fputs(summaryCSVHeader, m_csvfpt);
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
        char buffer[200];
        strftime(buffer, 128, "%c", timeinfo);
        fprintf(m_csvfpt, ", %s, ", buffer);

        x265_stats stats;
        fetchStats(&stats, sizeof(stats));

        // elapsed time, fps, bitrate
        fprintf(m_csvfpt, "%.2f, %.2f, %.2f,",
                stats.elapsedEncodeTime, stats.encodedPictureCount / stats.elapsedEncodeTime, stats.bitrate);

        if (m_param->bEnablePsnr)
            fprintf(m_csvfpt, " %.3lf, %.3lf, %.3lf, %.3lf,",
                    stats.globalPsnrY / stats.encodedPictureCount, stats.globalPsnrU / stats.encodedPictureCount,
                    stats.globalPsnrV / stats.encodedPictureCount, stats.globalPsnr);
        else
            fprintf(m_csvfpt, " -, -, -, -,");
        if (m_param->bEnableSsim)
            fprintf(m_csvfpt, " %.6f, %6.3f,", stats.globalSsim, x265_ssim2dB(stats.globalSsim));
        else
            fprintf(m_csvfpt, " -, -,");

        fputs(statsCSVString(m_analyzeI, buffer), m_csvfpt);
        fputs(statsCSVString(m_analyzeP, buffer), m_csvfpt);
        fputs(statsCSVString(m_analyzeB, buffer), m_csvfpt);
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

void Encoder::finishFrameStats(Frame* curFrame, FrameEncoder *curEncoder, uint64_t bits)
{
    PicYuv* reconPic = curFrame->m_reconPicYuv;

    //===== calculate PSNR =====
    int width  = reconPic->m_picWidth - m_sps.conformanceWindow.rightOffset;
    int height = reconPic->m_picHeight - m_sps.conformanceWindow.bottomOffset;
    int size = width * height;

    int maxvalY = 255 << (X265_DEPTH - 8);
    int maxvalC = 255 << (X265_DEPTH - 8);
    double refValueY = (double)maxvalY * maxvalY * size;
    double refValueC = (double)maxvalC * maxvalC * size / 4.0;
    uint64_t ssdY, ssdU, ssdV;

    ssdY = curEncoder->m_SSDY;
    ssdU = curEncoder->m_SSDU;
    ssdV = curEncoder->m_SSDV;
    double psnrY = (ssdY ? 10.0 * log10(refValueY / (double)ssdY) : 99.99);
    double psnrU = (ssdU ? 10.0 * log10(refValueC / (double)ssdU) : 99.99);
    double psnrV = (ssdV ? 10.0 * log10(refValueC / (double)ssdV) : 99.99);

    FrameData& curEncData = *curFrame->m_encData;
    Slice* slice = curEncData.m_slice;

    //===== add bits, psnr and ssim =====
    m_analyzeAll.addBits(bits);
    m_analyzeAll.addQP(curEncData.m_avgQpAq);

    if (m_param->bEnablePsnr)
        m_analyzeAll.addPsnr(psnrY, psnrU, psnrV);

    double ssim = 0.0;
    if (m_param->bEnableSsim && curEncoder->m_ssimCnt)
    {
        ssim = curEncoder->m_ssim / curEncoder->m_ssimCnt;
        m_analyzeAll.addSsim(ssim);
    }
    if (slice->isIntra())
    {
        m_analyzeI.addBits(bits);
        m_analyzeI.addQP(curEncData.m_avgQpAq);
        if (m_param->bEnablePsnr)
            m_analyzeI.addPsnr(psnrY, psnrU, psnrV);
        if (m_param->bEnableSsim)
            m_analyzeI.addSsim(ssim);
    }
    else if (slice->isInterP())
    {
        m_analyzeP.addBits(bits);
        m_analyzeP.addQP(curEncData.m_avgQpAq);
        if (m_param->bEnablePsnr)
            m_analyzeP.addPsnr(psnrY, psnrU, psnrV);
        if (m_param->bEnableSsim)
            m_analyzeP.addSsim(ssim);
    }
    else if (slice->isInterB())
    {
        m_analyzeB.addBits(bits);
        m_analyzeB.addQP(curEncData.m_avgQpAq);
        if (m_param->bEnablePsnr)
            m_analyzeB.addPsnr(psnrY, psnrU, psnrV);
        if (m_param->bEnableSsim)
            m_analyzeB.addSsim(ssim);
    }

    // if debug log level is enabled, per frame logging is performed
    if (m_param->logLevel >= X265_LOG_DEBUG)
    {
        char c = (slice->isIntra() ? 'I' : slice->isInterP() ? 'P' : 'B');
        int poc = slice->m_poc;
        if (!IS_REFERENCED(curFrame))
            c += 32; // lower case if unreferenced

        char buf[1024];
        int p;
        p = sprintf(buf, "POC:%d %c QP %2.2lf(%d) %10d bits", poc, c, curEncData.m_avgQpAq, slice->m_sliceQp, (int)bits);
        if (m_param->rc.rateControlMode == X265_RC_CRF)
            p += sprintf(buf + p, " RF:%.3lf", curEncData.m_rateFactor);
        if (m_param->bEnablePsnr)
            p += sprintf(buf + p, " [Y:%6.2lf U:%6.2lf V:%6.2lf]", psnrY, psnrU, psnrV);
        if (m_param->bEnableSsim)
            p += sprintf(buf + p, " [SSIM: %.3lfdB]", x265_ssim2dB(ssim));

        if (!slice->isIntra())
        {
            int numLists = slice->isInterP() ? 1 : 2;
            for (int list = 0; list < numLists; list++)
            {
                p += sprintf(buf + p, " [L%d ", list);
                for (int ref = 0; ref < slice->m_numRefIdx[list]; ref++)
                {
                    int k = slice->m_refPOCList[list][ref] - slice->m_lastIDR;
                    p += sprintf(buf + p, "%d ", k);
                }

                p += sprintf(buf + p, "]");
            }
        }

        // per frame CSV logging if the file handle is valid
        if (m_csvfpt)
        {
            fprintf(m_csvfpt, "%d, %c-SLICE, %4d, %2.2lf, %10d,", m_outputCount++, c, poc, curEncData.m_avgQpAq, (int)bits);
            if (m_param->rc.rateControlMode == X265_RC_CRF)
                fprintf(m_csvfpt, "%.3lf,", curEncData.m_rateFactor);
            double psnr = (psnrY * 6 + psnrU + psnrV) / 8;
            if (m_param->bEnablePsnr)
                fprintf(m_csvfpt, "%.3lf, %.3lf, %.3lf, %.3lf,", psnrY, psnrU, psnrV, psnr);
            else
                fprintf(m_csvfpt, " -, -, -, -,");
            if (m_param->bEnableSsim)
                fprintf(m_csvfpt, " %.6f, %6.3f,", ssim, x265_ssim2dB(ssim));
            else
                fprintf(m_csvfpt, " -, -,");
            fprintf(m_csvfpt, " %.3lf, %.3lf", curEncoder->m_frameTime, curEncoder->m_elapsedCompressTime);
            if (!slice->isIntra())
            {
                int numLists = slice->isInterP() ? 1 : 2;
                for (int list = 0; list < numLists; list++)
                {
                    fprintf(m_csvfpt, ", ");
                    for (int ref = 0; ref < slice->m_numRefIdx[list]; ref++)
                    {
                        int k = slice->m_refPOCList[list][ref] - slice->m_lastIDR;
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

        if (m_param->decodedPictureHashSEI && m_param->logLevel >= X265_LOG_FULL)
        {
            const char* digestStr = NULL;
            if (m_param->decodedPictureHashSEI == 1)
            {
                digestStr = digestToString(curEncoder->m_seiReconPictureDigest.m_digest, 16);
                p += sprintf(buf + p, " [MD5:%s]", digestStr);
            }
            else if (m_param->decodedPictureHashSEI == 2)
            {
                digestStr = digestToString(curEncoder->m_seiReconPictureDigest.m_digest, 2);
                p += sprintf(buf + p, " [CRC:%s]", digestStr);
            }
            else if (m_param->decodedPictureHashSEI == 3)
            {
                digestStr = digestToString(curEncoder->m_seiReconPictureDigest.m_digest, 4);
                p += sprintf(buf + p, " [Checksum:%s]", digestStr);
            }
        }
        x265_log(m_param, X265_LOG_DEBUG, "%s\n", buf);
        fflush(stderr);
    }
}

#if defined(_MSC_VER)
#pragma warning(disable: 4800) // forcing int to bool
#pragma warning(disable: 4127) // conditional expression is constant
#endif

void Encoder::getStreamHeaders(NALList& list, Entropy& sbacCoder, Bitstream& bs)
{
    sbacCoder.setBitstream(&bs);

    /* headers for start of bitstream */
    bs.resetBits();
    sbacCoder.codeVPS(m_vps);
    bs.writeByteAlignment();
    list.serialize(NAL_UNIT_VPS, bs);

    bs.resetBits();
    sbacCoder.codeSPS(m_sps, m_scalingList, m_vps.ptl);
    bs.writeByteAlignment();
    list.serialize(NAL_UNIT_SPS, bs);

    bs.resetBits();
    sbacCoder.codePPS(m_pps);
    bs.writeByteAlignment();
    list.serialize(NAL_UNIT_PPS, bs);

    if (m_param->bEmitInfoSEI)
    {
        char *opts = x265_param2string(m_param);
        if (opts)
        {
            char *buffer = X265_MALLOC(char, strlen(opts) + strlen(x265_version_str) +
                                             strlen(x265_build_info_str) + 200);
            if (buffer)
            {
                sprintf(buffer, "x265 (build %d) - %s:%s - H.265/HEVC codec - "
                        "Copyright 2013-2014 (c) Multicoreware Inc - "
                        "http://x265.org - options: %s",
                        X265_BUILD, x265_version_str, x265_build_info_str, opts);
                
                bs.resetBits();
                SEIuserDataUnregistered idsei;
                idsei.m_userData = (uint8_t*)buffer;
                idsei.m_userDataLength = (uint32_t)strlen(buffer);
                idsei.write(bs, m_sps);
                bs.writeByteAlignment();
                list.serialize(NAL_UNIT_PREFIX_SEI, bs);

                X265_FREE(buffer);
            }

            X265_FREE(opts);
        }
    }

    if (m_param->bEmitHRDSEI)
    {
        /* Picture Timing and Buffering Period SEI require the SPS to be "activated" */
        SEIActiveParameterSets sei;
        sei.m_selfContainedCvsFlag = true;
        sei.m_noParamSetUpdateFlag = true;

        bs.resetBits();
        sei.write(bs, m_sps);
        bs.writeByteAlignment();
        list.serialize(NAL_UNIT_PREFIX_SEI, bs);
    }
}

void Encoder::initSPS(SPS *sps)
{
    m_vps.ptl.progressiveSourceFlag = !m_param->interlaceMode;
    m_vps.ptl.interlacedSourceFlag = !!m_param->interlaceMode;
    m_vps.ptl.nonPackedConstraintFlag = false;
    m_vps.ptl.frameOnlyConstraintFlag = false;

    sps->conformanceWindow = m_conformanceWindow;
    sps->chromaFormatIdc = m_param->internalCsp;
    sps->picWidthInLumaSamples = m_param->sourceWidth;
    sps->picHeightInLumaSamples = m_param->sourceHeight;
    sps->numCuInWidth = (m_param->sourceWidth + g_maxCUSize - 1) / g_maxCUSize;
    sps->numCuInHeight = (m_param->sourceHeight + g_maxCUSize - 1) / g_maxCUSize;
    sps->numCUsInFrame = sps->numCuInWidth * sps->numCuInHeight;
    sps->numPartitions = NUM_CU_PARTITIONS;
    sps->numPartInCUSize = 1 << g_maxFullDepth;

    sps->log2MinCodingBlockSize = g_maxLog2CUSize - g_maxCUDepth;
    sps->log2DiffMaxMinCodingBlockSize = g_maxCUDepth;

    sps->quadtreeTULog2MaxSize = X265_MIN(g_maxLog2CUSize, 5);
    sps->quadtreeTULog2MinSize = 2;
    sps->quadtreeTUMaxDepthInter = m_param->tuQTMaxInterDepth;
    sps->quadtreeTUMaxDepthIntra = m_param->tuQTMaxIntraDepth;

    sps->bUseSAO = m_param->bEnableSAO;

    sps->bUseAMP = m_param->bEnableAMP;
    sps->maxAMPDepth = m_param->bEnableAMP ? g_maxCUDepth : 0;

    sps->maxDecPicBuffering = m_vps.maxDecPicBuffering;
    sps->numReorderPics = m_vps.numReorderPics;

    sps->bUseStrongIntraSmoothing = m_param->bEnableStrongIntraSmoothing;
    sps->bTemporalMVPEnabled = m_param->bEnableTemporalMvp;

    VUI& vui = sps->vuiParameters;
    vui.aspectRatioInfoPresentFlag = !!m_param->vui.aspectRatioIdc;
    vui.aspectRatioIdc = m_param->vui.aspectRatioIdc;
    vui.sarWidth = m_param->vui.sarWidth;
    vui.sarHeight = m_param->vui.sarHeight;

    vui.overscanInfoPresentFlag = m_param->vui.bEnableOverscanInfoPresentFlag;
    vui.overscanAppropriateFlag = m_param->vui.bEnableOverscanAppropriateFlag;

    vui.videoSignalTypePresentFlag = m_param->vui.bEnableVideoSignalTypePresentFlag;
    vui.videoFormat = m_param->vui.videoFormat;
    vui.videoFullRangeFlag = m_param->vui.bEnableVideoFullRangeFlag;

    vui.colourDescriptionPresentFlag = m_param->vui.bEnableColorDescriptionPresentFlag;
    vui.colourPrimaries = m_param->vui.colorPrimaries;
    vui.transferCharacteristics = m_param->vui.transferCharacteristics;
    vui.matrixCoefficients = m_param->vui.matrixCoeffs;

    vui.chromaLocInfoPresentFlag = m_param->vui.bEnableChromaLocInfoPresentFlag;
    vui.chromaSampleLocTypeTopField = m_param->vui.chromaSampleLocTypeTopField;
    vui.chromaSampleLocTypeBottomField = m_param->vui.chromaSampleLocTypeBottomField;

    vui.defaultDisplayWindow.bEnabled = m_param->vui.bEnableDefaultDisplayWindowFlag;
    vui.defaultDisplayWindow.rightOffset = m_param->vui.defDispWinRightOffset;
    vui.defaultDisplayWindow.topOffset = m_param->vui.defDispWinTopOffset;
    vui.defaultDisplayWindow.bottomOffset = m_param->vui.defDispWinBottomOffset;
    vui.defaultDisplayWindow.leftOffset = m_param->vui.defDispWinLeftOffset;

    vui.frameFieldInfoPresentFlag = !!m_param->interlaceMode;
    vui.fieldSeqFlag = !!m_param->interlaceMode;

    vui.hrdParametersPresentFlag = m_param->bEmitHRDSEI;

    vui.timingInfo.numUnitsInTick = m_param->fpsDenom;
    vui.timingInfo.timeScale = m_param->fpsNum;
}

void Encoder::initPPS(PPS *pps)
{
    bool bIsVbv = m_param->rc.vbvBufferSize > 0 && m_param->rc.vbvMaxBitrate > 0;

    if (!m_param->bLossless && (m_param->rc.aqMode || bIsVbv))
    {
        pps->bUseDQP = true;
        pps->maxCuDQPDepth = 0; /* TODO: make configurable? */
    }
    else
    {
        pps->bUseDQP = false;
        pps->maxCuDQPDepth = 0;
    }

    pps->chromaCbQpOffset = m_param->cbQpOffset;
    pps->chromaCrQpOffset = m_param->crQpOffset;

    pps->bConstrainedIntraPred = m_param->bEnableConstrainedIntra;
    pps->bUseWeightPred = m_param->bEnableWeightedPred;
    pps->bUseWeightedBiPred = m_param->bEnableWeightedBiPred;
    pps->bTransquantBypassEnabled = m_param->bCULossless || m_param->bLossless;
    pps->bTransformSkipEnabled = m_param->bEnableTransformSkip;
    pps->bSignHideEnabled = m_param->bEnableSignHiding;

    /* If offsets are ever configured, enable bDeblockingFilterControlPresent and set
     * deblockingFilterBetaOffsetDiv2 / deblockingFilterTcOffsetDiv2 */
    bool bDeblockOffsetInPPS = 0;
    pps->bDeblockingFilterControlPresent = !m_param->bEnableLoopFilter || bDeblockOffsetInPPS;
    pps->bPicDisableDeblockingFilter = !m_param->bEnableLoopFilter;
    pps->deblockingFilterBetaOffsetDiv2 = 0;
    pps->deblockingFilterTcOffsetDiv2 = 0;

    pps->bEntropyCodingSyncEnabled = m_param->bEnableWavefront;
}

void Encoder::configure(x265_param *p)
{
    this->m_param = p;

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

    if (p->bBPyramid && !p->bframes)
        p->bBPyramid = 0;

    /* Disable features which are not supported by the current RD level */
    if (p->rdLevel < 4)
    {
        if (p->psyRdoq > 0)             /* impossible */
            x265_log(p, X265_LOG_WARNING, "--psy-rdoq disabled, requires --rdlevel 4 or higher\n");
        p->psyRdoq = 0;
    }
    if (p->rdLevel < 3)
    {
        if (p->bCULossless)             /* impossible */
            x265_log(p, X265_LOG_WARNING, "--cu-lossless disabled, requires --rdlevel 3 or higher\n");
        if (p->bEnableTransformSkip)    /* impossible */
            x265_log(p, X265_LOG_WARNING, "--tskip disabled, requires --rdlevel 3 or higher\n");
        p->bCULossless = p->bEnableTransformSkip = 0;
    }
    if (p->rdLevel < 2)
    {
        if (p->bDistributeModeAnalysis) /* not useful */
            x265_log(p, X265_LOG_WARNING, "--pmode disabled, requires --rdlevel 2 or higher\n");
        p->bDistributeModeAnalysis = 0;

        if (p->psyRd > 0)               /* impossible */
            x265_log(p, X265_LOG_WARNING, "--psy-rd disabled, requires --rdlevel 2 or higher\n");
        p->psyRd = 0;

        if (p->bEnableRectInter)        /* broken, not very useful */
            x265_log(p, X265_LOG_WARNING, "--rect disabled, requires --rdlevel 2 or higher\n");
        p->bEnableRectInter = 0;
    }

    if (!p->bEnableRectInter)          /* not useful */
        p->bEnableAMP = false;

    /* In 444, chroma gets twice as much resolution, so halve quality when psy-rd is enabled */
    if (p->internalCsp == X265_CSP_I444 && p->psyRd)
    {
        p->cbQpOffset += 6;
        p->crQpOffset += 6;
    }

    if (p->bLossless)
    {
        p->rc.rateControlMode = X265_RC_CQP;
        p->rc.qp = 4; // An oddity, QP=4 is more lossless than QP=0 and gives better lambdas
        p->bEnableSsim = 0;
        p->bEnablePsnr = 0;
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

    if (p->lookaheadDepth == 0 && p->rc.cuTree && !p->rc.bStatRead)
    {
        x265_log(p, X265_LOG_WARNING, "cuTree disabled, requires lookahead to be enabled\n");
        p->rc.cuTree = 0;
    }

    if (p->rc.aqStrength == 0 && p->rc.cuTree == 0)
        p->rc.aqMode = X265_AQ_NONE;

    if (p->rc.aqMode == X265_AQ_NONE && p->rc.cuTree == 0)
        p->rc.aqStrength = 0;

    if (p->internalCsp != X265_CSP_I420)
    {
        x265_log(p, X265_LOG_WARNING, "!! HEVC Range Extension specifications are not finalized !!\n");
        x265_log(p, X265_LOG_WARNING, "!! This output bitstream may not be compliant with the final spec !!\n");
    }

    if (p->scalingLists && p->internalCsp == X265_CSP_I444)
    {
        x265_log(p, X265_LOG_WARNING, "Scaling lists are not yet supported for 4:4:4 color space\n");
        p->scalingLists = 0;
    }

    if (p->interlaceMode)
        x265_log(p, X265_LOG_WARNING, "Support for interlaced video is experimental\n");

    if (p->rc.rfConstantMin > p->rc.rfConstant)
    {
        x265_log(m_param, X265_LOG_WARNING, "CRF min must be less than CRF\n");
        p->rc.rfConstantMin = 0;
    }

    m_bframeDelay = p->bframes ? (p->bBPyramid ? 2 : 1) : 0;

    p->bFrameBias = X265_MIN(X265_MAX(-90, p->bFrameBias), 100);

    if (p->logLevel < X265_LOG_INFO)
    {
        /* don't measure these metrics if they will not be reported */
        p->bEnablePsnr = 0;
        p->bEnableSsim = 0;
    }
    /* Warn users trying to measure PSNR/SSIM with psy opts on. */
    if (p->bEnablePsnr || p->bEnableSsim)
    {
        const char *s = NULL;

        if (p->psyRd || p->psyRdoq)
        {
            s = p->bEnablePsnr ? "psnr" : "ssim";
            x265_log(p, X265_LOG_WARNING, "--%s used with psy on: results will be invalid!\n", s);
        }
        else if (!p->rc.aqMode && p->bEnableSsim)
        {
            x265_log(p, X265_LOG_WARNING, "--ssim used with AQ off: results will be invalid!\n");
            s = "ssim";
        }
        else if (p->rc.aqStrength > 0 && p->bEnablePsnr)
        {
            x265_log(p, X265_LOG_WARNING, "--psnr used with AQ on: results will be invalid!\n");
            s = "psnr";
        }
        if (s)
            x265_log(p, X265_LOG_WARNING, "--tune %s should be used if attempting to benchmark %s!\n", s, s);
    }

    //========= set default display window ==================================
    m_conformanceWindow.bEnabled = false;
    m_conformanceWindow.rightOffset = 0;
    m_conformanceWindow.topOffset = 0;
    m_conformanceWindow.bottomOffset = 0;
    m_conformanceWindow.leftOffset = 0;

    //======== set pad size if width is not multiple of the minimum CU size =========
    const uint32_t minCUSize = MIN_CU_SIZE;
    if (p->sourceWidth & (minCUSize - 1))
    {
        uint32_t rem = p->sourceWidth & (minCUSize - 1);
        uint32_t padsize = minCUSize - rem;
        p->sourceWidth += padsize;

        /* set the confirmation window offsets  */
        m_conformanceWindow.bEnabled = true;
        m_conformanceWindow.rightOffset = padsize;
    }

    //======== set pad size if height is not multiple of the minimum CU size =========
    if (p->sourceHeight & (minCUSize - 1))
    {
        uint32_t rem = p->sourceHeight & (minCUSize - 1);
        uint32_t padsize = minCUSize - rem;
        p->sourceHeight += padsize;

        /* set the confirmation window offsets  */
        m_conformanceWindow.bEnabled = true;
        m_conformanceWindow.bottomOffset = padsize;
    }
}
