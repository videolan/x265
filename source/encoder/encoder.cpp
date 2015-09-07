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

#if _MSC_VER
#pragma warning(disable: 4996) // POSIX functions are just fine, thanks
#endif

namespace X265_NS {
const char g_sliceTypeToChar[] = {'B', 'P', 'I'};
}

static const char* defaultAnalysisFileName = "x265_analysis.dat";

using namespace X265_NS;

Encoder::Encoder()
{
    m_aborted = false;
    m_reconfigured = false;
    m_encodedFrameNum = 0;
    m_pocLast = -1;
    m_curEncoder = 0;
    m_numLumaWPFrames = 0;
    m_numChromaWPFrames = 0;
    m_numLumaWPBiFrames = 0;
    m_numChromaWPBiFrames = 0;
    m_lookahead = NULL;
    m_rateControl = NULL;
    m_dpb = NULL;
    m_exportedPic = NULL;
    m_numDelayedPic = 0;
    m_outputCount = 0;
    m_param = NULL;
    m_latestParam = NULL;
    m_threadPool = NULL;
    m_analysisFile = NULL;
    m_offsetEmergency = NULL;
    for (int i = 0; i < X265_MAX_FRAME_THREADS; i++)
        m_frameEncoder[i] = NULL;

    MotionEstimate::initScales();
}

void Encoder::create()
{
    if (!primitives.pu[0].sad)
    {
        // this should be an impossible condition when using our public API, and indicates a serious bug.
        x265_log(m_param, X265_LOG_ERROR, "Primitives must be initialized before encoder is created\n");
        abort();
    }

    x265_param* p = m_param;

    int rows = (p->sourceHeight + p->maxCUSize - 1) >> g_log2Size[p->maxCUSize];
    int cols = (p->sourceWidth  + p->maxCUSize - 1) >> g_log2Size[p->maxCUSize];

    // Do not allow WPP if only one row or fewer than 3 columns, it is pointless and unstable
    if (rows == 1 || cols < 3)
    {
        x265_log(p, X265_LOG_WARNING, "Too few rows/columns, --wpp disabled\n");
        p->bEnableWavefront = 0;
    }

    bool allowPools = !p->numaPools || strcmp(p->numaPools, "none");

    // Trim the thread pool if --wpp, --pme, and --pmode are disabled
    if (!p->bEnableWavefront && !p->bDistributeModeAnalysis && !p->bDistributeMotionEstimation && !p->lookaheadSlices)
        allowPools = false;

    if (!p->frameNumThreads)
    {
        // auto-detect frame threads
        int cpuCount = ThreadPool::getCpuCount();
        if (!p->bEnableWavefront)
            p->frameNumThreads = X265_MIN3(cpuCount, (rows + 1) / 2, X265_MAX_FRAME_THREADS);
        else if (cpuCount >= 32)
            p->frameNumThreads = (p->sourceHeight > 2000) ? 8 : 6; // dual-socket 10-core IvyBridge or higher
        else if (cpuCount >= 16)
            p->frameNumThreads = 5; // 8 HT cores, or dual socket
        else if (cpuCount >= 8)
            p->frameNumThreads = 3; // 4 HT cores
        else if (cpuCount >= 4)
            p->frameNumThreads = 2; // Dual or Quad core
        else
            p->frameNumThreads = 1;
    }

    m_numPools = 0;
    if (allowPools)
        m_threadPool = ThreadPool::allocThreadPools(p, m_numPools);

    if (!m_numPools)
    {
        // issue warnings if any of these features were requested
        if (p->bEnableWavefront)
            x265_log(p, X265_LOG_WARNING, "No thread pool allocated, --wpp disabled\n");
        if (p->bDistributeMotionEstimation)
            x265_log(p, X265_LOG_WARNING, "No thread pool allocated, --pme disabled\n");
        if (p->bDistributeModeAnalysis)
            x265_log(p, X265_LOG_WARNING, "No thread pool allocated, --pmode disabled\n");
        if (p->lookaheadSlices)
            x265_log(p, X265_LOG_WARNING, "No thread pool allocated, --lookahead-slices disabled\n");

        // disable all pool features if the thread pool is disabled or unusable.
        p->bEnableWavefront = p->bDistributeModeAnalysis = p->bDistributeMotionEstimation = p->lookaheadSlices = 0;
    }

    if (!p->bEnableWavefront && p->rc.vbvBufferSize)
    {
        x265_log(p, X265_LOG_ERROR, "VBV requires wavefront parallelism\n");
        m_aborted = true;
    }

    char buf[128];
    int len = 0;
    if (p->bEnableWavefront)
        len += sprintf(buf + len, "wpp(%d rows)", rows);
    if (p->bDistributeModeAnalysis)
        len += sprintf(buf + len, "%spmode", len ? "+" : "");
    if (p->bDistributeMotionEstimation)
        len += sprintf(buf + len, "%spme ", len ? "+" : "");
    if (!len)
        strcpy(buf, "none");

    x265_log(p, X265_LOG_INFO, "frame threads / pool features       : %d / %s\n", p->frameNumThreads, buf);

    for (int i = 0; i < m_param->frameNumThreads; i++)
    {
        m_frameEncoder[i] = new FrameEncoder;
        m_frameEncoder[i]->m_nalList.m_annexB = !!m_param->bAnnexB;
    }

    if (m_numPools)
    {
        for (int i = 0; i < m_param->frameNumThreads; i++)
        {
            int pool = i % m_numPools;
            m_frameEncoder[i]->m_pool = &m_threadPool[pool];
            m_frameEncoder[i]->m_jpId = m_threadPool[pool].m_numProviders++;
            m_threadPool[pool].m_jpTable[m_frameEncoder[i]->m_jpId] = m_frameEncoder[i];
        }
        for (int i = 0; i < m_numPools; i++)
            m_threadPool[i].start();
    }
    else
    {
        /* CU stats and noise-reduction buffers are indexed by jpId, so it cannot be left as -1 */
        for (int i = 0; i < m_param->frameNumThreads; i++)
            m_frameEncoder[i]->m_jpId = 0;
    }

    if (!m_scalingList.init())
    {
        x265_log(m_param, X265_LOG_ERROR, "Unable to allocate scaling list arrays\n");
        m_aborted = true;
        return;
    }
    else if (!m_param->scalingLists || !strcmp(m_param->scalingLists, "off"))
        m_scalingList.m_bEnabled = false;
    else if (!strcmp(m_param->scalingLists, "default"))
        m_scalingList.setDefaultScalingList();
    else if (m_scalingList.parseScalingList(m_param->scalingLists))
        m_aborted = true;

    m_lookahead = new Lookahead(m_param, m_threadPool);
    if (m_numPools)
    {
        m_lookahead->m_jpId = m_threadPool[0].m_numProviders++;
        m_threadPool[0].m_jpTable[m_lookahead->m_jpId] = m_lookahead;
    }

    m_dpb = new DPB(m_param);
    m_rateControl = new RateControl(*m_param);

    initVPS(&m_vps);
    initSPS(&m_sps);
    initPPS(&m_pps);
   
    if (m_param->rc.vbvBufferSize)
    {
        m_offsetEmergency = (uint16_t(*)[MAX_NUM_TR_CATEGORIES][MAX_NUM_TR_COEFFS])X265_MALLOC(uint16_t, MAX_NUM_TR_CATEGORIES * MAX_NUM_TR_COEFFS * (QP_MAX_MAX - QP_MAX_SPEC));
        if (!m_offsetEmergency)
        {
            x265_log(m_param, X265_LOG_ERROR, "Unable to allocate memory\n");
            m_aborted = true;
            return;
        }

        bool scalingEnabled = m_scalingList.m_bEnabled;
        if (!scalingEnabled)
        {
            m_scalingList.setDefaultScalingList();
            m_scalingList.setupQuantMatrices();
        }
        else
            m_scalingList.setupQuantMatrices();

        for (int q = 0; q < QP_MAX_MAX - QP_MAX_SPEC; q++)
        {
            for (int cat = 0; cat < MAX_NUM_TR_CATEGORIES; cat++)
            {
                uint16_t *nrOffset = m_offsetEmergency[q][cat];

                int trSize = cat & 3;

                int coefCount = 1 << ((trSize + 2) * 2);

                /* Denoise chroma first then luma, then DC. */
                int dcThreshold = (QP_MAX_MAX - QP_MAX_SPEC) * 2 / 3;
                int lumaThreshold = (QP_MAX_MAX - QP_MAX_SPEC) * 2 / 3;
                int chromaThreshold = 0;

                int thresh = (cat < 4 || (cat >= 8 && cat < 12)) ? lumaThreshold : chromaThreshold;

                double quantF = (double)(1ULL << (q / 6 + 16 + 8));

                for (int i = 0; i < coefCount; i++)
                {
                    /* True "emergency mode": remove all DCT coefficients */
                    if (q == QP_MAX_MAX - QP_MAX_SPEC - 1)
                    {
                        nrOffset[i] = INT16_MAX;
                        continue;
                    }

                    int iThresh = i == 0 ? dcThreshold : thresh;
                    if (q < iThresh)
                    {
                        nrOffset[i] = 0;
                        continue;
                    }

                    int numList = (cat >= 8) * 3 + ((int)!iThresh);

                    double pos = (double)(q - iThresh + 1) / (QP_MAX_MAX - QP_MAX_SPEC - iThresh);
                    double start = quantF / (m_scalingList.m_quantCoef[trSize][numList][QP_MAX_SPEC % 6][i]);

                    // Formula chosen as an exponential scale to vaguely mimic the effects of a higher quantizer.
                    double bias = (pow(2, pos * (QP_MAX_MAX - QP_MAX_SPEC)) * 0.003 - 0.003) * start;
                    nrOffset[i] = (uint16_t)X265_MIN(bias + 0.5, INT16_MAX);
                }
            }
        }

        if (!scalingEnabled)
        {
            m_scalingList.m_bEnabled = false;
            m_scalingList.m_bDataPresent = false;
            m_scalingList.setupQuantMatrices();
        }
    }
    else
        m_scalingList.setupQuantMatrices();

    int numRows = (m_param->sourceHeight + g_maxCUSize - 1) / g_maxCUSize;
    int numCols = (m_param->sourceWidth  + g_maxCUSize - 1) / g_maxCUSize;
    for (int i = 0; i < m_param->frameNumThreads; i++)
    {
        if (!m_frameEncoder[i]->init(this, numRows, numCols))
        {
            x265_log(m_param, X265_LOG_ERROR, "Unable to initialize frame encoder, aborting\n");
            m_aborted = true;
        }
    }

    for (int i = 0; i < m_param->frameNumThreads; i++)
    {
        m_frameEncoder[i]->start();
        m_frameEncoder[i]->m_done.wait(); /* wait for thread to initialize */
    }

    if (m_param->bEmitHRDSEI)
        m_rateControl->initHRD(m_sps);
    if (!m_rateControl->init(m_sps))
        m_aborted = true;
    if (!m_lookahead->create())
        m_aborted = true;

    if (m_param->analysisMode)
    {
        const char* name = m_param->analysisFileName;
        if (!name)
            name = defaultAnalysisFileName;
        const char* mode = m_param->analysisMode == X265_ANALYSIS_LOAD ? "rb" : "wb";
        m_analysisFile = fopen(name, mode);
        if (!m_analysisFile)
        {
            x265_log(NULL, X265_LOG_ERROR, "Analysis load/save: failed to open file %s\n", name);
            m_aborted = true;
        }
    }

    m_bZeroLatency = !m_param->bframes && !m_param->lookaheadDepth && m_param->frameNumThreads == 1;

    m_aborted |= parseLambdaFile(m_param);

    m_encodeStartTime = x265_mdate();

    m_nalList.m_annexB = !!m_param->bAnnexB;

    m_emitCLLSEI = p->maxCLL || p->maxFALL;
}

void Encoder::stopJobs()
{
    if (m_rateControl)
        m_rateControl->terminate(); // unblock all blocked RC calls

    if (m_lookahead)
        m_lookahead->stopJobs();
    
    for (int i = 0; i < m_param->frameNumThreads; i++)
    {
        if (m_frameEncoder[i])
        {
            m_frameEncoder[i]->getEncodedPicture(m_nalList);
            m_frameEncoder[i]->m_threadActive = false;
            m_frameEncoder[i]->m_enable.trigger();
            m_frameEncoder[i]->stop();
        }
    }

    if (m_threadPool)
        m_threadPool->stopWorkers();
}

void Encoder::destroy()
{
    if (m_exportedPic)
    {
        ATOMIC_DEC(&m_exportedPic->m_countRefEncoders);
        m_exportedPic = NULL;
    }

    for (int i = 0; i < m_param->frameNumThreads; i++)
    {
        if (m_frameEncoder[i])
        {
            m_frameEncoder[i]->destroy();
            delete m_frameEncoder[i];
        }
    }

    // thread pools can be cleaned up now that all the JobProviders are
    // known to be shutdown
    delete [] m_threadPool;

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

    X265_FREE(m_offsetEmergency);

    if (m_analysisFile)
        fclose(m_analysisFile);

    if (m_param)
    {
        /* release string arguments that were strdup'd */
        free((char*)m_param->rc.lambdaFileName);
        free((char*)m_param->rc.statFileName);
        free((char*)m_param->analysisFileName);
        free((char*)m_param->scalingLists);
        free((char*)m_param->numaPools);
        free((char*)m_param->masteringDisplayColorVolume);

        PARAM_NS::x265_param_free(m_param);
    }

    PARAM_NS::x265_param_free(m_latestParam);
}

void Encoder::updateVbvPlan(RateControl* rc)
{
    for (int i = 0; i < m_param->frameNumThreads; i++)
    {
        FrameEncoder *encoder = m_frameEncoder[i];
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
#if CHECKED_BUILD || _DEBUG
    if (g_checkFailures)
    {
        x265_log(m_param, X265_LOG_ERROR, "encoder aborting because of internal error\n");
        return -1;
    }
#endif
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
            x265_param* p = m_reconfigured? m_latestParam : m_param;
            if (inFrame->create(p, pic_in->quantOffsets))
            {
                /* the first PicYuv created is asked to generate the CU and block unit offset
                 * arrays which are then shared with all subsequent PicYuv (orig and recon) 
                 * allocated by this top level encoder */
                if (m_sps.cuOffsetY)
                {
                    inFrame->m_fencPic->m_cuOffsetC = m_sps.cuOffsetC;
                    inFrame->m_fencPic->m_cuOffsetY = m_sps.cuOffsetY;
                    inFrame->m_fencPic->m_buOffsetC = m_sps.buOffsetC;
                    inFrame->m_fencPic->m_buOffsetY = m_sps.buOffsetY;
                }
                else
                {
                    if (!inFrame->m_fencPic->createOffsets(m_sps))
                    {
                        m_aborted = true;
                        x265_log(m_param, X265_LOG_ERROR, "memory allocation failure, aborting encode\n");
                        inFrame->destroy();
                        delete inFrame;
                        return -1;
                    }
                    else
                    {
                        m_sps.cuOffsetC = inFrame->m_fencPic->m_cuOffsetC;
                        m_sps.cuOffsetY = inFrame->m_fencPic->m_cuOffsetY;
                        m_sps.buOffsetC = inFrame->m_fencPic->m_buOffsetC;
                        m_sps.buOffsetY = inFrame->m_fencPic->m_buOffsetY;
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
        {
            inFrame = m_dpb->m_freeList.popBack();
            inFrame->m_lowresInit = false;
        }

        /* Copy input picture into a Frame and PicYuv, send to lookahead */
        inFrame->m_fencPic->copyFromPicture(*pic_in, *m_param, m_sps.conformanceWindow.rightOffset, m_sps.conformanceWindow.bottomOffset);

        inFrame->m_poc       = ++m_pocLast;
        inFrame->m_userData  = pic_in->userData;
        inFrame->m_pts       = pic_in->pts;
        inFrame->m_forceqp   = pic_in->forceqp;
        inFrame->m_param     = m_reconfigured ? m_latestParam : m_param;
        
        if (pic_in->quantOffsets != NULL)
        {
            int cuCount = inFrame->m_lowres.maxBlocksInRow * inFrame->m_lowres.maxBlocksInCol;
            memcpy(inFrame->m_quantOffsets, pic_in->quantOffsets, cuCount * sizeof(float));
        }

        if (m_pocLast == 0)
            m_firstPts = inFrame->m_pts;
        if (m_bframeDelay && m_pocLast == m_bframeDelay)
            m_bframeDelayTime = inFrame->m_pts - m_firstPts;

        /* Encoder holds a reference count until stats collection is finished */
        ATOMIC_INC(&inFrame->m_countRefEncoders);

        if ((m_param->rc.aqMode || m_param->bEnableWeightedPred || m_param->bEnableWeightedBiPred) &&
            (m_param->rc.cuTree && m_param->rc.bStatRead))
        {
            if (!m_rateControl->cuTreeReadFor2Pass(inFrame))
            {
                m_aborted = 1;
                return -1;
            }
        }

        /* Use the frame types from the first pass, if available */
        int sliceType = (m_param->rc.bStatRead) ? m_rateControl->rateControlSliceType(inFrame->m_poc) : pic_in->sliceType;

        /* In analysisSave mode, x265_analysis_data is allocated in pic_in and inFrame points to this */
        /* Load analysis data before lookahead->addPicture, since sliceType has been decided */
        if (m_param->analysisMode == X265_ANALYSIS_LOAD)
        {
            x265_picture* inputPic = const_cast<x265_picture*>(pic_in);
            /* readAnalysisFile reads analysis data for the frame and allocates memory based on slicetype */
            readAnalysisFile(&inputPic->analysisData, inFrame->m_poc);
            inFrame->m_analysisData.poc = inFrame->m_poc;
            inFrame->m_analysisData.sliceType = inputPic->analysisData.sliceType;
            inFrame->m_analysisData.numCUsInFrame = inputPic->analysisData.numCUsInFrame;
            inFrame->m_analysisData.numPartitions = inputPic->analysisData.numPartitions;
            inFrame->m_analysisData.interData = inputPic->analysisData.interData;
            inFrame->m_analysisData.intraData = inputPic->analysisData.intraData;
            sliceType = inputPic->analysisData.sliceType;
        }

        m_lookahead->addPicture(*inFrame, sliceType);
        m_numDelayedPic++;
    }
    else
        m_lookahead->flush();

    FrameEncoder *curEncoder = m_frameEncoder[m_curEncoder];
    m_curEncoder = (m_curEncoder + 1) % m_param->frameNumThreads;
    int ret = 0;

    /* Normal operation is to wait for the current frame encoder to complete its current frame
     * and then to give it a new frame to work on.  In zero-latency mode, we must encode this
     * input picture before returning so the order must be reversed. This do/while() loop allows
     * us to alternate the order of the calls without ugly code replication */
    Frame* outFrame = NULL;
    Frame* frameEnc = NULL;
    int pass = 0;
    do
    {
        /* getEncodedPicture() should block until the FrameEncoder has completed
         * encoding the frame.  This is how back-pressure through the API is
         * accomplished when the encoder is full */
        if (!m_bZeroLatency || pass)
            outFrame = curEncoder->getEncodedPicture(m_nalList);
        if (outFrame)
        {
            Slice *slice = outFrame->m_encData->m_slice;
            x265_frame_stats* frameData = NULL;

            /* Free up pic_in->analysisData since it has already been used */
            if (m_param->analysisMode == X265_ANALYSIS_LOAD)
                freeAnalysis(&outFrame->m_analysisData);

            if (pic_out)
            {
                PicYuv *recpic = outFrame->m_reconPic;
                pic_out->poc = slice->m_poc;
                pic_out->bitDepth = X265_DEPTH;
                pic_out->userData = outFrame->m_userData;
                pic_out->colorSpace = m_param->internalCsp;
                frameData = &(pic_out->frameData);

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

                /* Dump analysis data from pic_out to file in save mode and free */
                if (m_param->analysisMode == X265_ANALYSIS_SAVE)
                {
                    pic_out->analysisData.poc = pic_out->poc;
                    pic_out->analysisData.sliceType = pic_out->sliceType;
                    pic_out->analysisData.numCUsInFrame = outFrame->m_analysisData.numCUsInFrame;
                    pic_out->analysisData.numPartitions = outFrame->m_analysisData.numPartitions;
                    pic_out->analysisData.interData = outFrame->m_analysisData.interData;
                    pic_out->analysisData.intraData = outFrame->m_analysisData.intraData;
                    writeAnalysisFile(&pic_out->analysisData);
                    freeAnalysis(&pic_out->analysisData);
                }
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

            finishFrameStats(outFrame, curEncoder, curEncoder->m_accessUnitBits, frameData);

            /* Write RateControl Frame level stats in multipass encodes */
            if (m_param->rc.bStatWrite)
                if (m_rateControl->writeRateControlFrameStats(outFrame, &curEncoder->m_rce))
                    m_aborted = true;

            /* Allow this frame to be recycled if no frame encoders are using it for reference */
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

        /* pop a single frame from decided list, then provide to frame encoder
         * curEncoder is guaranteed to be idle at this point */
        if (!pass)
            frameEnc = m_lookahead->getDecidedPicture();
        if (frameEnc && !pass)
        {
            /* give this frame a FrameData instance before encoding */
            if (m_dpb->m_frameDataFreeList)
            {
                frameEnc->m_encData = m_dpb->m_frameDataFreeList;
                m_dpb->m_frameDataFreeList = m_dpb->m_frameDataFreeList->m_freeListNext;
                frameEnc->reinit(m_sps);
            }
            else
            {
                frameEnc->allocEncodeData(m_param, m_sps);
                Slice* slice = frameEnc->m_encData->m_slice;
                slice->m_sps = &m_sps;
                slice->m_pps = &m_pps;
                slice->m_maxNumMergeCand = m_param->maxNumMergeCand;
                slice->m_endCUAddr = slice->realEndAddress(m_sps.numCUsInFrame * NUM_4x4_PARTITIONS);
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

            /* Allocate analysis data before encode in save mode. This is allocated in frameEnc */
            if (m_param->analysisMode == X265_ANALYSIS_SAVE)
            {
                x265_analysis_data* analysis = &frameEnc->m_analysisData;
                analysis->poc = frameEnc->m_poc;
                analysis->sliceType = frameEnc->m_lowres.sliceType;
                uint32_t widthInCU       = (m_param->sourceWidth  + g_maxCUSize - 1) >> g_maxLog2CUSize;
                uint32_t heightInCU      = (m_param->sourceHeight + g_maxCUSize - 1) >> g_maxLog2CUSize;

                uint32_t numCUsInFrame   = widthInCU * heightInCU;
                analysis->numCUsInFrame  = numCUsInFrame;
                analysis->numPartitions  = NUM_4x4_PARTITIONS;
                allocAnalysis(analysis);
            }

            /* determine references, setup RPS, etc */
            m_dpb->prepareEncode(frameEnc);

            if (m_param->rc.rateControlMode != X265_RC_CQP)
                m_lookahead->getEstimatedPictureCost(frameEnc);

            /* Allow FrameEncoder::compressFrame() to start in the frame encoder thread */
            if (!curEncoder->startCompressFrame(frameEnc))
                m_aborted = true;
        }
        else if (m_encodedFrameNum)
            m_rateControl->setFinalFrameCount(m_encodedFrameNum); 
    }
    while (m_bZeroLatency && ++pass < 2);

    return ret;
}

int Encoder::reconfigureParam(x265_param* encParam, x265_param* param)
{
    encParam->maxNumReferences = param->maxNumReferences; // never uses more refs than specified in stream headers
    encParam->bEnableLoopFilter = param->bEnableLoopFilter;
    encParam->deblockingFilterTCOffset = param->deblockingFilterTCOffset;
    encParam->deblockingFilterBetaOffset = param->deblockingFilterBetaOffset; 
    encParam->bEnableFastIntra = param->bEnableFastIntra;
    encParam->bEnableEarlySkip = param->bEnableEarlySkip;
    encParam->bEnableTemporalMvp = param->bEnableTemporalMvp;
    /* Scratch buffer prevents me_range from being increased for esa/tesa
    if (param->searchMethod < X265_FULL_SEARCH || param->searchMethod < encParam->searchRange)
        encParam->searchRange = param->searchRange; */
    encParam->noiseReductionInter = param->noiseReductionInter;
    encParam->noiseReductionIntra = param->noiseReductionIntra;
    /* We can't switch out of subme=0 during encoding. */
    if (encParam->subpelRefine)
        encParam->subpelRefine = param->subpelRefine;
    encParam->rdoqLevel = param->rdoqLevel;
    encParam->rdLevel = param->rdLevel;
    encParam->bEnableTSkipFast = param->bEnableTSkipFast;
    encParam->psyRd = param->psyRd;
    encParam->psyRdoq = param->psyRdoq;
    encParam->bEnableSignHiding = param->bEnableSignHiding;
    encParam->bEnableFastIntra = param->bEnableFastIntra;
    encParam->maxTUSize = param->maxTUSize;
    return x265_check_params(encParam);
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

    if (m_analyzeAll.m_numPics)
    {
        int p = 0;
        double elapsedEncodeTime = (double)(x265_mdate() - m_encodeStartTime) / 1000000;
        double elapsedVideoTime = (double)m_analyzeAll.m_numPics * m_param->fpsDenom / m_param->fpsNum;
        double bitrate = (0.001f * m_analyzeAll.m_accBits) / elapsedVideoTime;

        p += sprintf(buffer + p, "\nencoded %d frames in %.2fs (%.2f fps), %.2f kb/s, Avg QP:%2.2lf", m_analyzeAll.m_numPics,
                     elapsedEncodeTime, m_analyzeAll.m_numPics / elapsedEncodeTime, bitrate, m_analyzeAll.m_totalQp / (double)m_analyzeAll.m_numPics);

        if (m_param->bEnablePsnr)
        {
            double globalPsnr = (m_analyzeAll.m_psnrSumY * 6 + m_analyzeAll.m_psnrSumU + m_analyzeAll.m_psnrSumV) / (8 * m_analyzeAll.m_numPics);
            p += sprintf(buffer + p, ", Global PSNR: %.3f", globalPsnr);
        }

        if (m_param->bEnableSsim)
            p += sprintf(buffer + p, ", SSIM Mean Y: %.7f (%6.3f dB)", m_analyzeAll.m_globalSsim / m_analyzeAll.m_numPics, x265_ssim2dB(m_analyzeAll.m_globalSsim / m_analyzeAll.m_numPics));

        sprintf(buffer + p, "\n");
        general_log(m_param, NULL, X265_LOG_INFO, buffer);
    }
    else
        general_log(m_param, NULL, X265_LOG_INFO, "\nencoded 0 frames\n");

#if DETAILED_CU_STATS
    /* Summarize stats from all frame encoders */
    CUStats cuStats;
    for (int i = 0; i < m_param->frameNumThreads; i++)
        cuStats.accumulate(m_frameEncoder[i]->m_cuStats);

    if (!cuStats.totalCTUTime)
        return;

    int totalWorkerCount = 0;
    for (int i = 0; i < m_numPools; i++)
        totalWorkerCount += m_threadPool[i].m_numWorkers;

    int64_t  batchElapsedTime, coopSliceElapsedTime;
    uint64_t batchCount, coopSliceCount;
    m_lookahead->getWorkerStats(batchElapsedTime, batchCount, coopSliceElapsedTime, coopSliceCount);
    int64_t lookaheadWorkerTime = m_lookahead->m_slicetypeDecideElapsedTime + m_lookahead->m_preLookaheadElapsedTime +
                                  batchElapsedTime + coopSliceElapsedTime;

    int64_t totalWorkerTime = cuStats.totalCTUTime + cuStats.loopFilterElapsedTime + cuStats.pmodeTime +
                              cuStats.pmeTime + lookaheadWorkerTime + cuStats.weightAnalyzeTime;
    int64_t elapsedEncodeTime = x265_mdate() - m_encodeStartTime;

    int64_t interRDOTotalTime = 0, intraRDOTotalTime = 0;
    uint64_t interRDOTotalCount = 0, intraRDOTotalCount = 0;
    for (uint32_t i = 0; i <= g_maxCUDepth; i++)
    {
        interRDOTotalTime += cuStats.interRDOElapsedTime[i];
        intraRDOTotalTime += cuStats.intraRDOElapsedTime[i];
        interRDOTotalCount += cuStats.countInterRDO[i];
        intraRDOTotalCount += cuStats.countIntraRDO[i];
    }

    /* Time within compressCTU() and pmode tasks not captured by ME, Intra mode selection, or RDO (2Nx2N merge, 2Nx2N bidir, etc) */
    int64_t unaccounted = (cuStats.totalCTUTime + cuStats.pmodeTime) -
                          (cuStats.intraAnalysisElapsedTime + cuStats.motionEstimationElapsedTime + interRDOTotalTime + intraRDOTotalTime);

#define ELAPSED_SEC(val)  ((double)(val) / 1000000)
#define ELAPSED_MSEC(val) ((double)(val) / 1000)

    if (m_param->bDistributeMotionEstimation && cuStats.countPMEMasters)
    {
        x265_log(m_param, X265_LOG_INFO, "CU: %%%05.2lf time spent in motion estimation, averaging %.3lf CU inter modes per CTU\n",
                 100.0 * (cuStats.motionEstimationElapsedTime + cuStats.pmeTime) / totalWorkerTime,
                 (double)cuStats.countMotionEstimate / cuStats.totalCTUs);
        x265_log(m_param, X265_LOG_INFO, "CU: %.3lf PME masters per inter CU, each blocked an average of %.3lf ns\n",
                 (double)cuStats.countPMEMasters / cuStats.countMotionEstimate,
                 (double)cuStats.pmeBlockTime / cuStats.countPMEMasters);
        x265_log(m_param, X265_LOG_INFO, "CU:       %.3lf slaves per PME master, each took an average of %.3lf ms\n",
                 (double)cuStats.countPMETasks / cuStats.countPMEMasters,
                 ELAPSED_MSEC(cuStats.pmeTime) / cuStats.countPMETasks);
    }
    else
    {
        x265_log(m_param, X265_LOG_INFO, "CU: %%%05.2lf time spent in motion estimation, averaging %.3lf CU inter modes per CTU\n",
                 100.0 * cuStats.motionEstimationElapsedTime / totalWorkerTime,
                 (double)cuStats.countMotionEstimate / cuStats.totalCTUs);

        if (cuStats.skippedMotionReferences[0] || cuStats.skippedMotionReferences[1] || cuStats.skippedMotionReferences[2])
            x265_log(m_param, X265_LOG_INFO, "CU: Skipped motion searches per depth %%%.2lf %%%.2lf %%%.2lf %%%.2lf\n",
                     100.0 * cuStats.skippedMotionReferences[0] / cuStats.totalMotionReferences[0],
                     100.0 * cuStats.skippedMotionReferences[1] / cuStats.totalMotionReferences[1],
                     100.0 * cuStats.skippedMotionReferences[2] / cuStats.totalMotionReferences[2],
                     100.0 * cuStats.skippedMotionReferences[3] / cuStats.totalMotionReferences[3]);
    }
    x265_log(m_param, X265_LOG_INFO, "CU: %%%05.2lf time spent in intra analysis, averaging %.3lf Intra PUs per CTU\n",
             100.0 * cuStats.intraAnalysisElapsedTime / totalWorkerTime,
             (double)cuStats.countIntraAnalysis / cuStats.totalCTUs);
    if (cuStats.skippedIntraCU[0] || cuStats.skippedIntraCU[1] || cuStats.skippedIntraCU[2])
        x265_log(m_param, X265_LOG_INFO, "CU: Skipped intra CUs at depth %%%.2lf %%%.2lf %%%.2lf\n",
                 100.0 * cuStats.skippedIntraCU[0] / cuStats.totalIntraCU[0],
                 100.0 * cuStats.skippedIntraCU[1] / cuStats.totalIntraCU[1],
                 100.0 * cuStats.skippedIntraCU[2] / cuStats.totalIntraCU[2]);
    x265_log(m_param, X265_LOG_INFO, "CU: %%%05.2lf time spent in inter RDO, measuring %.3lf inter/merge predictions per CTU\n",
             100.0 * interRDOTotalTime / totalWorkerTime,
             (double)interRDOTotalCount / cuStats.totalCTUs);
    x265_log(m_param, X265_LOG_INFO, "CU: %%%05.2lf time spent in intra RDO, measuring %.3lf intra predictions per CTU\n",
             100.0 * intraRDOTotalTime / totalWorkerTime,
             (double)intraRDOTotalCount / cuStats.totalCTUs);
    x265_log(m_param, X265_LOG_INFO, "CU: %%%05.2lf time spent in loop filters, average %.3lf ms per call\n",
             100.0 * cuStats.loopFilterElapsedTime / totalWorkerTime,
             ELAPSED_MSEC(cuStats.loopFilterElapsedTime) / cuStats.countLoopFilter);
    if (cuStats.countWeightAnalyze && cuStats.weightAnalyzeTime)
    {
        x265_log(m_param, X265_LOG_INFO, "CU: %%%05.2lf time spent in weight analysis, average %.3lf ms per call\n",
                 100.0 * cuStats.weightAnalyzeTime / totalWorkerTime,
                 ELAPSED_MSEC(cuStats.weightAnalyzeTime) / cuStats.countWeightAnalyze);
    }
    if (m_param->bDistributeModeAnalysis && cuStats.countPModeMasters)
    {
        x265_log(m_param, X265_LOG_INFO, "CU: %.3lf PMODE masters per CTU, each blocked an average of %.3lf ns\n",
                 (double)cuStats.countPModeMasters / cuStats.totalCTUs,
                 (double)cuStats.pmodeBlockTime / cuStats.countPModeMasters);
        x265_log(m_param, X265_LOG_INFO, "CU:       %.3lf slaves per PMODE master, each took average of %.3lf ms\n",
                 (double)cuStats.countPModeTasks / cuStats.countPModeMasters, 
                 ELAPSED_MSEC(cuStats.pmodeTime) / cuStats.countPModeTasks);
    }

    x265_log(m_param, X265_LOG_INFO, "CU: %%%05.2lf time spent in slicetypeDecide (avg %.3lfms) and prelookahead (avg %.3lfms)\n",
             100.0 * lookaheadWorkerTime / totalWorkerTime,
             ELAPSED_MSEC(m_lookahead->m_slicetypeDecideElapsedTime) / m_lookahead->m_countSlicetypeDecide,
             ELAPSED_MSEC(m_lookahead->m_preLookaheadElapsedTime) / m_lookahead->m_countPreLookahead);

    x265_log(m_param, X265_LOG_INFO, "CU: %%%05.2lf time spent in other tasks\n",
             100.0 * unaccounted / totalWorkerTime);

    if (intraRDOTotalTime && intraRDOTotalCount)
    {
        x265_log(m_param, X265_LOG_INFO, "CU: Intra RDO time  per depth %%%05.2lf %%%05.2lf %%%05.2lf %%%05.2lf\n",
                 100.0 * cuStats.intraRDOElapsedTime[0] / intraRDOTotalTime,  // 64
                 100.0 * cuStats.intraRDOElapsedTime[1] / intraRDOTotalTime,  // 32
                 100.0 * cuStats.intraRDOElapsedTime[2] / intraRDOTotalTime,  // 16
                 100.0 * cuStats.intraRDOElapsedTime[3] / intraRDOTotalTime); // 8
        x265_log(m_param, X265_LOG_INFO, "CU: Intra RDO calls per depth %%%05.2lf %%%05.2lf %%%05.2lf %%%05.2lf\n",
                 100.0 * cuStats.countIntraRDO[0] / intraRDOTotalCount,  // 64
                 100.0 * cuStats.countIntraRDO[1] / intraRDOTotalCount,  // 32
                 100.0 * cuStats.countIntraRDO[2] / intraRDOTotalCount,  // 16
                 100.0 * cuStats.countIntraRDO[3] / intraRDOTotalCount); // 8
    }

    if (interRDOTotalTime && interRDOTotalCount)
    {
        x265_log(m_param, X265_LOG_INFO, "CU: Inter RDO time  per depth %%%05.2lf %%%05.2lf %%%05.2lf %%%05.2lf\n",
                 100.0 * cuStats.interRDOElapsedTime[0] / interRDOTotalTime,  // 64
                 100.0 * cuStats.interRDOElapsedTime[1] / interRDOTotalTime,  // 32
                 100.0 * cuStats.interRDOElapsedTime[2] / interRDOTotalTime,  // 16
                 100.0 * cuStats.interRDOElapsedTime[3] / interRDOTotalTime); // 8
        x265_log(m_param, X265_LOG_INFO, "CU: Inter RDO calls per depth %%%05.2lf %%%05.2lf %%%05.2lf %%%05.2lf\n",
                 100.0 * cuStats.countInterRDO[0] / interRDOTotalCount,  // 64
                 100.0 * cuStats.countInterRDO[1] / interRDOTotalCount,  // 32
                 100.0 * cuStats.countInterRDO[2] / interRDOTotalCount,  // 16
                 100.0 * cuStats.countInterRDO[3] / interRDOTotalCount); // 8
    }

    x265_log(m_param, X265_LOG_INFO, "CU: " X265_LL " %dX%d CTUs compressed in %.3lf seconds, %.3lf CTUs per worker-second\n",
             cuStats.totalCTUs, g_maxCUSize, g_maxCUSize,
             ELAPSED_SEC(totalWorkerTime),
             cuStats.totalCTUs / ELAPSED_SEC(totalWorkerTime));

    if (m_threadPool)
        x265_log(m_param, X265_LOG_INFO, "CU: %.3lf average worker utilization, %%%05.2lf of theoretical maximum utilization\n",
                 (double)totalWorkerTime / elapsedEncodeTime,
                 100.0 * totalWorkerTime / (elapsedEncodeTime * totalWorkerCount));

#undef ELAPSED_SEC
#undef ELAPSED_MSEC
#endif
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

        double fps = (double)m_param->fpsNum / m_param->fpsDenom;
        double scale = fps / 1000;

        stats->statsI.numPics = m_analyzeI.m_numPics;
        stats->statsI.avgQp   = m_analyzeI.m_totalQp / (double)m_analyzeI.m_numPics;
        stats->statsI.bitrate = m_analyzeI.m_accBits * scale / (double)m_analyzeI.m_numPics;
        stats->statsI.psnrY   = m_analyzeI.m_psnrSumY / (double)m_analyzeI.m_numPics;
        stats->statsI.psnrU   = m_analyzeI.m_psnrSumU / (double)m_analyzeI.m_numPics;
        stats->statsI.psnrV   = m_analyzeI.m_psnrSumV / (double)m_analyzeI.m_numPics;
        stats->statsI.ssim    = x265_ssim2dB(m_analyzeI.m_globalSsim / (double)m_analyzeI.m_numPics);

        stats->statsP.numPics = m_analyzeP.m_numPics;
        stats->statsP.avgQp   = m_analyzeP.m_totalQp / (double)m_analyzeP.m_numPics;
        stats->statsP.bitrate = m_analyzeP.m_accBits * scale / (double)m_analyzeP.m_numPics;
        stats->statsP.psnrY   = m_analyzeP.m_psnrSumY / (double)m_analyzeP.m_numPics;
        stats->statsP.psnrU   = m_analyzeP.m_psnrSumU / (double)m_analyzeP.m_numPics;
        stats->statsP.psnrV   = m_analyzeP.m_psnrSumV / (double)m_analyzeP.m_numPics;
        stats->statsP.ssim    = x265_ssim2dB(m_analyzeP.m_globalSsim / (double)m_analyzeP.m_numPics);

        stats->statsB.numPics = m_analyzeB.m_numPics;
        stats->statsB.avgQp   = m_analyzeB.m_totalQp / (double)m_analyzeB.m_numPics;
        stats->statsB.bitrate = m_analyzeB.m_accBits * scale / (double)m_analyzeB.m_numPics;
        stats->statsB.psnrY   = m_analyzeB.m_psnrSumY / (double)m_analyzeB.m_numPics;
        stats->statsB.psnrU   = m_analyzeB.m_psnrSumU / (double)m_analyzeB.m_numPics;
        stats->statsB.psnrV   = m_analyzeB.m_psnrSumV / (double)m_analyzeB.m_numPics;
        stats->statsB.ssim    = x265_ssim2dB(m_analyzeB.m_globalSsim / (double)m_analyzeB.m_numPics);

        stats->maxCLL         = m_analyzeAll.m_maxCLL;
        stats->maxFALL        = (uint16_t)(m_analyzeAll.m_maxFALL / m_analyzeAll.m_numPics);

        if (m_emitCLLSEI)
        {
            m_param->maxCLL = stats->maxCLL;
            m_param->maxFALL = stats->maxFALL;
        }
    }

    /* If new statistics are added to x265_stats, we must check here whether the
     * structure provided by the user is the new structure or an older one (for
     * future safety) */
}

void Encoder::finishFrameStats(Frame* curFrame, FrameEncoder *curEncoder, uint64_t bits, x265_frame_stats* frameStats)
{
    PicYuv* reconPic = curFrame->m_reconPic;

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

    m_analyzeAll.m_maxFALL += curFrame->m_fencPic->m_avgLumaLevel;
    m_analyzeAll.m_maxCLL = X265_MAX(m_analyzeAll.m_maxCLL, curFrame->m_fencPic->m_maxLumaLevel);

    char c = (slice->isIntra() ? 'I' : slice->isInterP() ? 'P' : 'B');
    int poc = slice->m_poc;
    if (!IS_REFERENCED(curFrame))
        c += 32; // lower case if unreferenced

    if (frameStats)
    {
        frameStats->encoderOrder = m_outputCount++;
        frameStats->sliceType = c;
        frameStats->poc = poc;
        frameStats->qp = curEncData.m_avgQpAq;
        frameStats->bits = bits;
        frameStats->bScenecut = curFrame->m_lowres.bScenecut;
        if (m_param->rc.rateControlMode == X265_RC_CRF)
            frameStats->rateFactor = curEncData.m_rateFactor;
        frameStats->psnrY = psnrY;
        frameStats->psnrU = psnrU;
        frameStats->psnrV = psnrV;
        double psnr = (psnrY * 6 + psnrU + psnrV) / 8;
        frameStats->psnr = psnr;
        frameStats->ssim = ssim;
        if (!slice->isIntra())
        {
            for (int ref = 0; ref < 16; ref++)
                frameStats->list0POC[ref] = ref < slice->m_numRefIdx[0] ? slice->m_refPOCList[0][ref] - slice->m_lastIDR : -1;

            if (!slice->isInterP())
            {
                for (int ref = 0; ref < 16; ref++)
                    frameStats->list1POC[ref] = ref < slice->m_numRefIdx[1] ? slice->m_refPOCList[1][ref] - slice->m_lastIDR : -1;
            }
        }

#define ELAPSED_MSEC(start, end) (((double)(end) - (start)) / 1000)

        frameStats->decideWaitTime = ELAPSED_MSEC(0, curEncoder->m_slicetypeWaitTime);
        frameStats->row0WaitTime = ELAPSED_MSEC(curEncoder->m_startCompressTime, curEncoder->m_row0WaitTime);
        frameStats->wallTime = ELAPSED_MSEC(curEncoder->m_row0WaitTime, curEncoder->m_endCompressTime);
        frameStats->refWaitWallTime = ELAPSED_MSEC(curEncoder->m_row0WaitTime, curEncoder->m_allRowsAvailableTime);
        frameStats->totalCTUTime = ELAPSED_MSEC(0, curEncoder->m_totalWorkerElapsedTime);
        frameStats->stallTime = ELAPSED_MSEC(0, curEncoder->m_totalNoWorkerTime);
        if (curEncoder->m_totalActiveWorkerCount)
            frameStats->avgWPP = (double)curEncoder->m_totalActiveWorkerCount / curEncoder->m_activeWorkerCountSamples;
        else
            frameStats->avgWPP = 1;
        frameStats->countRowBlocks = curEncoder->m_countRowBlocks;

        frameStats->cuStats.percentIntraNxN = curFrame->m_encData->m_frameStats.percentIntraNxN;
        frameStats->avgChromaDistortion     = curFrame->m_encData->m_frameStats.avgChromaDistortion;
        frameStats->avgLumaDistortion       = curFrame->m_encData->m_frameStats.avgLumaDistortion;
        frameStats->avgPsyEnergy            = curFrame->m_encData->m_frameStats.avgPsyEnergy;
        frameStats->avgResEnergy            = curFrame->m_encData->m_frameStats.avgResEnergy;
        frameStats->avgLumaLevel            = curFrame->m_fencPic->m_avgLumaLevel;
        frameStats->maxLumaLevel            = curFrame->m_fencPic->m_maxLumaLevel;
        for (uint32_t depth = 0; depth <= g_maxCUDepth; depth++)
        {
            frameStats->cuStats.percentSkipCu[depth]  = curFrame->m_encData->m_frameStats.percentSkipCu[depth];
            frameStats->cuStats.percentMergeCu[depth] = curFrame->m_encData->m_frameStats.percentMergeCu[depth];
            frameStats->cuStats.percentInterDistribution[depth][0] = curFrame->m_encData->m_frameStats.percentInterDistribution[depth][0];
            frameStats->cuStats.percentInterDistribution[depth][1] = curFrame->m_encData->m_frameStats.percentInterDistribution[depth][1];
            frameStats->cuStats.percentInterDistribution[depth][2] = curFrame->m_encData->m_frameStats.percentInterDistribution[depth][2];
            for (int n = 0; n < INTRA_MODES; n++)
                frameStats->cuStats.percentIntraDistribution[depth][n] = curFrame->m_encData->m_frameStats.percentIntraDistribution[depth][n];
        }
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

    if (m_param->masteringDisplayColorVolume)
    {
        SEIMasteringDisplayColorVolume mdsei;
        if (mdsei.parse(m_param->masteringDisplayColorVolume))
        {
            bs.resetBits();
            mdsei.write(bs, m_sps);
            bs.writeByteAlignment();
            list.serialize(NAL_UNIT_PREFIX_SEI, bs);
        }
        else
            x265_log(m_param, X265_LOG_WARNING, "unable to parse mastering display color volume info\n");
    }

    if (m_emitCLLSEI)
    {
        SEIContentLightLevel cllsei;
        cllsei.max_content_light_level = m_param->maxCLL;
        cllsei.max_pic_average_light_level = m_param->maxFALL;
        bs.resetBits();
        cllsei.write(bs, m_sps);
        bs.writeByteAlignment();
        list.serialize(NAL_UNIT_PREFIX_SEI, bs);
    }

    if (m_param->bEmitInfoSEI)
    {
        char *opts = x265_param2string(m_param);
        if (opts)
        {
            char *buffer = X265_MALLOC(char, strlen(opts) + strlen(PFX(version_str)) +
                                             strlen(PFX(build_info_str)) + 200);
            if (buffer)
            {
                sprintf(buffer, "x265 (build %d) - %s:%s - H.265/HEVC codec - "
                        "Copyright 2013-2015 (c) Multicoreware Inc - "
                        "http://x265.org - options: %s",
                        X265_BUILD, PFX(version_str), PFX(build_info_str), opts);
                
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

    if (m_param->bEmitHRDSEI || !!m_param->interlaceMode)
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

void Encoder::initVPS(VPS *vps)
{
    /* Note that much of the VPS is initialized by determineLevel() */
    vps->ptl.progressiveSourceFlag = !m_param->interlaceMode;
    vps->ptl.interlacedSourceFlag = !!m_param->interlaceMode;
    vps->ptl.nonPackedConstraintFlag = false;
    vps->ptl.frameOnlyConstraintFlag = !m_param->interlaceMode;
}

void Encoder::initSPS(SPS *sps)
{
    sps->conformanceWindow = m_conformanceWindow;
    sps->chromaFormatIdc = m_param->internalCsp;
    sps->picWidthInLumaSamples = m_param->sourceWidth;
    sps->picHeightInLumaSamples = m_param->sourceHeight;
    sps->numCuInWidth = (m_param->sourceWidth + g_maxCUSize - 1) / g_maxCUSize;
    sps->numCuInHeight = (m_param->sourceHeight + g_maxCUSize - 1) / g_maxCUSize;
    sps->numCUsInFrame = sps->numCuInWidth * sps->numCuInHeight;
    sps->numPartitions = NUM_4x4_PARTITIONS;
    sps->numPartInCUSize = 1 << g_unitSizeDepth;

    sps->log2MinCodingBlockSize = g_maxLog2CUSize - g_maxCUDepth;
    sps->log2DiffMaxMinCodingBlockSize = g_maxCUDepth;
    uint32_t maxLog2TUSize = (uint32_t)g_log2Size[m_param->maxTUSize];
    sps->quadtreeTULog2MaxSize = X265_MIN(g_maxLog2CUSize, maxLog2TUSize);
    sps->quadtreeTULog2MinSize = 2;
    sps->quadtreeTUMaxDepthInter = m_param->tuQTMaxInterDepth;
    sps->quadtreeTUMaxDepthIntra = m_param->tuQTMaxIntraDepth;

    sps->bUseSAO = m_param->bEnableSAO;

    sps->bUseAMP = m_param->bEnableAMP;
    sps->maxAMPDepth = m_param->bEnableAMP ? g_maxCUDepth : 0;

    sps->maxTempSubLayers = m_param->bEnableTemporalSubLayers ? 2 : 1;
    sps->maxDecPicBuffering = m_vps.maxDecPicBuffering;
    sps->numReorderPics = m_vps.numReorderPics;
    sps->maxLatencyIncrease = m_vps.maxLatencyIncrease = m_param->bframes;

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
        pps->maxCuDQPDepth = g_log2Size[m_param->maxCUSize] - g_log2Size[m_param->rc.qgSize];
        X265_CHECK(pps->maxCuDQPDepth <= 2, "max CU DQP depth cannot be greater than 2\n");
    }
    else
    {
        pps->bUseDQP = false;
        pps->maxCuDQPDepth = 0;
    }

    pps->chromaQpOffset[0] = m_param->cbQpOffset;
    pps->chromaQpOffset[1] = m_param->crQpOffset;

    pps->bConstrainedIntraPred = m_param->bEnableConstrainedIntra;
    pps->bUseWeightPred = m_param->bEnableWeightedPred;
    pps->bUseWeightedBiPred = m_param->bEnableWeightedBiPred;
    pps->bTransquantBypassEnabled = m_param->bCULossless || m_param->bLossless;
    pps->bTransformSkipEnabled = m_param->bEnableTransformSkip;
    pps->bSignHideEnabled = m_param->bEnableSignHiding;

    pps->bDeblockingFilterControlPresent = !m_param->bEnableLoopFilter || m_param->deblockingFilterBetaOffset || m_param->deblockingFilterTCOffset;
    pps->bPicDisableDeblockingFilter = !m_param->bEnableLoopFilter;
    pps->deblockingFilterBetaOffsetDiv2 = m_param->deblockingFilterBetaOffset;
    pps->deblockingFilterTcOffsetDiv2 = m_param->deblockingFilterTCOffset;

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
        p->keyframeMax = 1;

        // disable lookahead for all-intra encodes
        p->bFrameAdaptive = 0;
        p->bframes = 0;
        p->bOpenGOP = 0;
        p->bRepeatHeaders = 1;
        p->lookaheadDepth = 0;
        p->bframes = 0;
        p->scenecutThreshold = 0;
        p->bFrameAdaptive = 0;
        p->rc.cuTree = 0;
        p->bEnableWeightedPred = 0;
        p->bEnableWeightedBiPred = 0;

        /* SPSs shall have sps_max_dec_pic_buffering_minus1[ sps_max_sub_layers_minus1 ] equal to 0 only */
        p->maxNumReferences = 1;
    }
    if (!p->keyframeMin)
    {
        double fps = (double)p->fpsNum / p->fpsDenom;
        p->keyframeMin = X265_MIN((int)fps, p->keyframeMax / 10);
    }
    p->keyframeMin = X265_MAX(1, X265_MIN(p->keyframeMin, p->keyframeMax / 2 + 1));

    if (!p->bframes)
        p->bBPyramid = 0;
    if (!p->rdoqLevel)
        p->psyRdoq = 0;

    /* Disable features which are not supported by the current RD level */
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

        p->psyRd = 0;                   /* impossible */

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

    if (p->maxTUSize > p->maxCUSize)
    {
        x265_log(p, X265_LOG_WARNING, "Max TU size should be less than or equal to max CU size, setting max TU size = %d\n", p->maxCUSize);
        p->maxTUSize = p->maxCUSize;
    }

    if (p->rc.aqStrength == 0 && p->rc.cuTree == 0)
        p->rc.aqMode = X265_AQ_NONE;

    if (p->rc.aqMode == X265_AQ_NONE && p->rc.cuTree == 0)
        p->rc.aqStrength = 0;

    if (p->totalFrames && p->totalFrames <= 2 * ((float)p->fpsNum) / p->fpsDenom && p->rc.bStrictCbr)
        p->lookaheadDepth = p->totalFrames;

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

    if (p->analysisMode && (p->bDistributeModeAnalysis || p->bDistributeMotionEstimation))
    {
        x265_log(p, X265_LOG_WARNING, "Analysis load/save options incompatible with pmode/pme, Disabling pmode/pme\n");
        p->bDistributeMotionEstimation = p->bDistributeModeAnalysis = 0;
    }

    if (p->bDistributeModeAnalysis && (p->limitReferences >> 1) && 1)
    {
        x265_log(p, X265_LOG_WARNING, "Limit reference options 2 and 3 are not supported with pmode. Disabling limit reference\n");
        p->limitReferences = 0;
    }

    if (p->bEnableTemporalSubLayers && !p->bframes)
    {
        x265_log(p, X265_LOG_WARNING, "B frames not enabled, temporal sublayer disabled\n");
        p->bEnableTemporalSubLayers = 0;
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

    /* some options make no sense if others are disabled */
    p->bSaoNonDeblocked &= p->bEnableSAO;
    p->bEnableTSkipFast &= p->bEnableTransformSkip;

    /* initialize the conformance window */
    m_conformanceWindow.bEnabled = false;
    m_conformanceWindow.rightOffset = 0;
    m_conformanceWindow.topOffset = 0;
    m_conformanceWindow.bottomOffset = 0;
    m_conformanceWindow.leftOffset = 0;

    /* set pad size if width is not multiple of the minimum CU size */
    if (p->sourceWidth & (p->minCUSize - 1))
    {
        uint32_t rem = p->sourceWidth & (p->minCUSize - 1);
        uint32_t padsize = p->minCUSize - rem;
        p->sourceWidth += padsize;

        m_conformanceWindow.bEnabled = true;
        m_conformanceWindow.rightOffset = padsize;
    }

    /* set pad size if height is not multiple of the minimum CU size */
    if (p->sourceHeight & (p->minCUSize - 1))
    {
        uint32_t rem = p->sourceHeight & (p->minCUSize - 1);
        uint32_t padsize = p->minCUSize - rem;
        p->sourceHeight += padsize;

        m_conformanceWindow.bEnabled = true;
        m_conformanceWindow.bottomOffset = padsize;
    }
    if (p->bDistributeModeAnalysis && p->analysisMode)
    {
        p->analysisMode = X265_ANALYSIS_OFF;
        x265_log(p, X265_LOG_WARNING, "Analysis save and load mode not supported for distributed mode analysis\n");
    }

    bool bIsVbv = m_param->rc.vbvBufferSize > 0 && m_param->rc.vbvMaxBitrate > 0;
    if (!m_param->bLossless && (m_param->rc.aqMode || bIsVbv))
    {
        if (p->rc.qgSize < X265_MAX(16, p->minCUSize))
        {
            p->rc.qgSize = X265_MAX(16, p->minCUSize);
            x265_log(p, X265_LOG_WARNING, "QGSize should be greater than or equal to 16 and minCUSize, setting QGSize = %d\n", p->rc.qgSize);
        }
        if (p->rc.qgSize > p->maxCUSize)
        {
            p->rc.qgSize = p->maxCUSize;
            x265_log(p, X265_LOG_WARNING, "QGSize should be less than or equal to maxCUSize, setting QGSize = %d\n", p->rc.qgSize);
        }
    }
    else
        m_param->rc.qgSize = p->maxCUSize;

    if (p->bLogCuStats)
        x265_log(p, X265_LOG_WARNING, "--cu-stats option is now deprecated\n");

    if (p->csvfn)
        x265_log(p, X265_LOG_WARNING, "libx265 no longer supports CSV file statistics\n");
}

void Encoder::allocAnalysis(x265_analysis_data* analysis)
{
    analysis->interData = analysis->intraData = NULL;
    if (analysis->sliceType == X265_TYPE_IDR || analysis->sliceType == X265_TYPE_I)
    {
        analysis_intra_data *intraData = (analysis_intra_data*)analysis->intraData;
        CHECKED_MALLOC_ZERO(intraData, analysis_intra_data, 1);
        CHECKED_MALLOC(intraData->depth, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC(intraData->modes, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC(intraData->partSizes, char, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC(intraData->chromaModes, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        analysis->intraData = intraData;
    }
    else
    {
        analysis_inter_data *interData = (analysis_inter_data*)analysis->interData;
        CHECKED_MALLOC_ZERO(interData, analysis_inter_data, 1);
        CHECKED_MALLOC_ZERO(interData->ref, int32_t, analysis->numCUsInFrame * X265_MAX_PRED_MODE_PER_CTU * 2);
        CHECKED_MALLOC(interData->depth, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC(interData->modes, uint8_t, analysis->numPartitions * analysis->numCUsInFrame);
        CHECKED_MALLOC_ZERO(interData->bestMergeCand, uint32_t, analysis->numCUsInFrame * CUGeom::MAX_GEOMS);
        analysis->interData = interData;
    }
    return;

fail:
    freeAnalysis(analysis);
    m_aborted = true;
}

void Encoder::freeAnalysis(x265_analysis_data* analysis)
{
    if (analysis->intraData)
    {
        X265_FREE(((analysis_intra_data*)analysis->intraData)->depth);
        X265_FREE(((analysis_intra_data*)analysis->intraData)->modes);
        X265_FREE(((analysis_intra_data*)analysis->intraData)->partSizes);
        X265_FREE(((analysis_intra_data*)analysis->intraData)->chromaModes);
        X265_FREE(analysis->intraData);
    }
    else
    {
        X265_FREE(((analysis_inter_data*)analysis->interData)->ref);
        X265_FREE(((analysis_inter_data*)analysis->interData)->depth);
        X265_FREE(((analysis_inter_data*)analysis->interData)->modes);
        X265_FREE(((analysis_inter_data*)analysis->interData)->bestMergeCand);
        X265_FREE(analysis->interData);
    }
}

void Encoder::readAnalysisFile(x265_analysis_data* analysis, int curPoc)
{

#define X265_FREAD(val, size, readSize, fileOffset)\
    if (fread(val, size, readSize, fileOffset) != readSize)\
    {\
        x265_log(NULL, X265_LOG_ERROR, "Error reading analysis data\n");\
        freeAnalysis(analysis);\
        m_aborted = true;\
        return;\
    }\

    static uint64_t consumedBytes = 0;
    static uint64_t totalConsumedBytes = 0;
    fseeko(m_analysisFile, totalConsumedBytes, SEEK_SET);

    int poc; uint32_t frameRecordSize;
    X265_FREAD(&frameRecordSize, sizeof(uint32_t), 1, m_analysisFile);
    X265_FREAD(&poc, sizeof(int), 1, m_analysisFile);

    uint64_t currentOffset = totalConsumedBytes;

    /* Seeking to the right frame Record */
    while (poc != curPoc && !feof(m_analysisFile))
    {
        currentOffset += frameRecordSize;
        fseeko(m_analysisFile, currentOffset, SEEK_SET);
        X265_FREAD(&frameRecordSize, sizeof(uint32_t), 1, m_analysisFile);
        X265_FREAD(&poc, sizeof(int), 1, m_analysisFile);
    }

    if (poc != curPoc || feof(m_analysisFile))
    {
        x265_log(NULL, X265_LOG_WARNING, "Error reading analysis data: Cannot find POC %d\n", curPoc);
        freeAnalysis(analysis);
        return;
    }

    /* Now arrived at the right frame, read the record */
    analysis->poc = poc;
    analysis->frameRecordSize = frameRecordSize;
    X265_FREAD(&analysis->sliceType, sizeof(int), 1, m_analysisFile);
    X265_FREAD(&analysis->numCUsInFrame, sizeof(int), 1, m_analysisFile);
    X265_FREAD(&analysis->numPartitions, sizeof(int), 1, m_analysisFile);

    /* Memory is allocated for inter and intra analysis data based on the slicetype */
    allocAnalysis(analysis);

    if (analysis->sliceType == X265_TYPE_IDR || analysis->sliceType == X265_TYPE_I)
    {
        X265_FREAD(((analysis_intra_data *)analysis->intraData)->depth, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FREAD(((analysis_intra_data *)analysis->intraData)->modes, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FREAD(((analysis_intra_data *)analysis->intraData)->partSizes, sizeof(char), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FREAD(((analysis_intra_data *)analysis->intraData)->chromaModes, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        analysis->sliceType = X265_TYPE_I;
        consumedBytes += frameRecordSize;
    }
    else if (analysis->sliceType == X265_TYPE_P)
    {
        X265_FREAD(((analysis_inter_data *)analysis->interData)->ref, sizeof(int32_t), analysis->numCUsInFrame * X265_MAX_PRED_MODE_PER_CTU, m_analysisFile);
        X265_FREAD(((analysis_inter_data *)analysis->interData)->depth, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FREAD(((analysis_inter_data *)analysis->interData)->modes, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FREAD(((analysis_inter_data *)analysis->interData)->bestMergeCand, sizeof(uint32_t), analysis->numCUsInFrame * CUGeom::MAX_GEOMS, m_analysisFile);
        consumedBytes += frameRecordSize;
        totalConsumedBytes = consumedBytes;
    }
    else
    {
        X265_FREAD(((analysis_inter_data *)analysis->interData)->ref, sizeof(int32_t), analysis->numCUsInFrame * X265_MAX_PRED_MODE_PER_CTU * 2, m_analysisFile);
        X265_FREAD(((analysis_inter_data *)analysis->interData)->depth, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FREAD(((analysis_inter_data *)analysis->interData)->modes, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FREAD(((analysis_inter_data *)analysis->interData)->bestMergeCand, sizeof(uint32_t), analysis->numCUsInFrame * CUGeom::MAX_GEOMS, m_analysisFile);
        consumedBytes += frameRecordSize;
    }
#undef X265_FREAD
}

void Encoder::writeAnalysisFile(x265_analysis_data* analysis)
{

#define X265_FWRITE(val, size, writeSize, fileOffset)\
    if (fwrite(val, size, writeSize, fileOffset) < writeSize)\
    {\
        x265_log(NULL, X265_LOG_ERROR, "Error writing analysis data\n");\
        freeAnalysis(analysis);\
        m_aborted = true;\
        return;\
    }\

    /* calculate frameRecordSize */
    analysis->frameRecordSize = sizeof(analysis->frameRecordSize) + sizeof(analysis->poc) + sizeof(analysis->sliceType) +
                      sizeof(analysis->numCUsInFrame) + sizeof(analysis->numPartitions);
    if (analysis->sliceType == X265_TYPE_IDR || analysis->sliceType == X265_TYPE_I)
        analysis->frameRecordSize += sizeof(uint8_t) * analysis->numCUsInFrame * analysis->numPartitions * 4;
    else if (analysis->sliceType == X265_TYPE_P)
    {
        analysis->frameRecordSize += sizeof(int32_t) * analysis->numCUsInFrame * X265_MAX_PRED_MODE_PER_CTU;
        analysis->frameRecordSize += sizeof(uint8_t) * analysis->numCUsInFrame * analysis->numPartitions * 2;
        analysis->frameRecordSize += sizeof(uint32_t) * analysis->numCUsInFrame * CUGeom::MAX_GEOMS;
    }
    else
    {
        analysis->frameRecordSize += sizeof(int32_t) * analysis->numCUsInFrame * X265_MAX_PRED_MODE_PER_CTU * 2;
        analysis->frameRecordSize += sizeof(uint8_t) * analysis->numCUsInFrame * analysis->numPartitions * 2;
        analysis->frameRecordSize += sizeof(uint32_t) * analysis->numCUsInFrame * CUGeom::MAX_GEOMS;
    }

    X265_FWRITE(&analysis->frameRecordSize, sizeof(uint32_t), 1, m_analysisFile);
    X265_FWRITE(&analysis->poc, sizeof(int), 1, m_analysisFile);
    X265_FWRITE(&analysis->sliceType, sizeof(int), 1, m_analysisFile);
    X265_FWRITE(&analysis->numCUsInFrame, sizeof(int), 1, m_analysisFile);
    X265_FWRITE(&analysis->numPartitions, sizeof(int), 1, m_analysisFile);

    if (analysis->sliceType == X265_TYPE_IDR || analysis->sliceType == X265_TYPE_I)
    {
        X265_FWRITE(((analysis_intra_data*)analysis->intraData)->depth, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FWRITE(((analysis_intra_data*)analysis->intraData)->modes, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FWRITE(((analysis_intra_data*)analysis->intraData)->partSizes, sizeof(char), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FWRITE(((analysis_intra_data*)analysis->intraData)->chromaModes, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
    }
    else if (analysis->sliceType == X265_TYPE_P)
    {
        X265_FWRITE(((analysis_inter_data*)analysis->interData)->ref, sizeof(int32_t), analysis->numCUsInFrame * X265_MAX_PRED_MODE_PER_CTU, m_analysisFile);
        X265_FWRITE(((analysis_inter_data*)analysis->interData)->depth, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FWRITE(((analysis_inter_data*)analysis->interData)->modes, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FWRITE(((analysis_inter_data*)analysis->interData)->bestMergeCand, sizeof(uint32_t), analysis->numCUsInFrame * CUGeom::MAX_GEOMS, m_analysisFile);
    }
    else
    {
        X265_FWRITE(((analysis_inter_data*)analysis->interData)->ref, sizeof(int32_t), analysis->numCUsInFrame * X265_MAX_PRED_MODE_PER_CTU * 2, m_analysisFile);
        X265_FWRITE(((analysis_inter_data*)analysis->interData)->depth, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FWRITE(((analysis_inter_data*)analysis->interData)->modes, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFile);
        X265_FWRITE(((analysis_inter_data*)analysis->interData)->bestMergeCand, sizeof(uint32_t), analysis->numCUsInFrame * CUGeom::MAX_GEOMS, m_analysisFile);
    }
#undef X265_FWRITE
}
