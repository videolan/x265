/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <chenm003@163.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
 *          Aruna Matheswaran <aruna@multicorewareinc.com>
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

/* Dolby Vision profile specific settings */
typedef struct
{
    int bEmitHRDSEI;
    int bEnableVideoSignalTypePresentFlag;
    int bEnableColorDescriptionPresentFlag;
    int bEnableAccessUnitDelimiters;
    int bAnnexB;

    /* VUI parameters specific to Dolby Vision Profile */
    int videoFormat;
    int bEnableVideoFullRangeFlag;
    int transferCharacteristics;
    int colorPrimaries;
    int matrixCoeffs;

    int doviProfileId;
}DolbyVisionProfileSpec;

DolbyVisionProfileSpec dovi[] =
{
    { 1, 1, 1, 1, 1, 5, 1,  2, 2, 2, 50 },
    { 1, 1, 1, 1, 1, 5, 0, 16, 9, 9, 81 },
    { 1, 1, 1, 1, 1, 5, 0,  1, 1, 1, 82 }
};
}

/* Threshold for motion vection, based on expermental result.
 * TODO: come up an algorithm for adoptive threshold */
#define MVTHRESHOLD (10*10)
#define PU_2Nx2N 1
#define MAX_CHROMA_QP_OFFSET 12
#define CONF_OFFSET_BYTES (2 * sizeof(int))
static const char* defaultAnalysisFileName = "x265_analysis.dat";

using namespace X265_NS;

Encoder::Encoder()
{
    m_aborted = false;
    m_reconfigure = false;
    m_reconfigureRc = false;
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
    m_analysisFileIn = NULL;
    m_analysisFileOut = NULL;
    m_naluFile = NULL;
    m_offsetEmergency = NULL;
    m_iFrameNum = 0;
    m_iPPSQpMinus26 = 0;
    m_rpsInSpsCount = 0;
    m_cB = 1.0;
    m_cR = 1.0;
    for (int i = 0; i < X265_MAX_FRAME_THREADS; i++)
        m_frameEncoder[i] = NULL;
    for (uint32_t i = 0; i < DUP_BUFFER; i++)
        m_dupBuffer[i] = NULL;
    MotionEstimate::initScales();

#if ENABLE_HDR10_PLUS
    m_hdr10plus_api = hdr10plus_api_get();
    m_numCimInfo = 0;
    m_cim = NULL;
#endif

#if SVT_HEVC
    m_svtAppData = NULL;
#endif
    m_prevTonemapPayload.payload = NULL;
    m_startPoint = 0;
    m_saveCTUSize = 0;
    m_edgePic = NULL;
    m_edgeHistThreshold = 0;
    m_chromaHistThreshold = 0.0;
    m_scaledEdgeThreshold = 0.0;
    m_scaledChromaThreshold = 0.0;
    m_zoneIndex = 0;
}

inline char *strcatFilename(const char *input, const char *suffix)
{
    char *output = X265_MALLOC(char, strlen(input) + strlen(suffix) + 1);
    if (!output)
    {
        x265_log(NULL, X265_LOG_ERROR, "unable to allocate memory for filename\n");
        return NULL;
    }
    strcpy(output, input);
    strcat(output, suffix);
    return output;
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

    if (m_param->bEnableFrameDuplication)
    {
        size_t framesize = 0;
        int pixelbytes = p->sourceBitDepth > 8 ? 2 : 1;
        for (int i = 0; i < x265_cli_csps[p->internalCsp].planes; i++)
        {
            int stride = (p->sourceWidth >> x265_cli_csps[p->internalCsp].width[i]) * pixelbytes;
            framesize += (stride * (p->sourceHeight >> x265_cli_csps[p->internalCsp].height[i]));
        }

        //Sets the picture structure and emits it in the picture timing SEI message
        m_param->pictureStructure = 0; 

        for (uint32_t i = 0; i < DUP_BUFFER; i++)
        {
            m_dupBuffer[i] = (AdaptiveFrameDuplication*)x265_malloc(sizeof(AdaptiveFrameDuplication));
            m_dupBuffer[i]->dupPic = NULL;
            m_dupBuffer[i]->dupPic = x265_picture_alloc();
            x265_picture_init(p, m_dupBuffer[i]->dupPic);
            m_dupBuffer[i]->dupPlane = NULL;
            m_dupBuffer[i]->dupPlane = X265_MALLOC(char, framesize);
            m_dupBuffer[i]->dupPic->planes[0] = m_dupBuffer[i]->dupPlane;
            m_dupBuffer[i]->bOccupied = false;
            m_dupBuffer[i]->bDup = false;
        }

        if (!(p->sourceBitDepth == 8 && p->internalBitDepth == 8))
        {
            int size = p->sourceWidth * p->sourceHeight;
            int hshift = CHROMA_H_SHIFT(p->internalCsp);
            int vshift = CHROMA_V_SHIFT(p->internalCsp);
            int widthC = p->sourceWidth >> hshift;
            int heightC = p->sourceHeight >> vshift;

            m_dupPicOne[0] = X265_MALLOC(pixel, size);
            m_dupPicTwo[0] = X265_MALLOC(pixel, size);
            if (p->internalCsp != X265_CSP_I400)
            {
                for (int k = 1; k < 3; k++)
                {
                    m_dupPicOne[k] = X265_MALLOC(pixel, widthC * heightC);
                    m_dupPicTwo[k] = X265_MALLOC(pixel, widthC * heightC);
                }
            }
        }
    }

    if (m_param->bHistBasedSceneCut)
    {
        m_planeSizes[0] = (m_param->sourceWidth >> x265_cli_csps[p->internalCsp].width[0]) * (m_param->sourceHeight >> x265_cli_csps[m_param->internalCsp].height[0]);
        uint32_t pixelbytes = m_param->internalBitDepth > 8 ? 2 : 1;
        m_edgePic = X265_MALLOC(pixel, m_planeSizes[0] * pixelbytes);
        m_edgeHistThreshold = m_param->edgeTransitionThreshold;
        m_chromaHistThreshold = x265_min(m_edgeHistThreshold * 10.0, MAX_SCENECUT_THRESHOLD);
        m_scaledEdgeThreshold = x265_min(m_edgeHistThreshold * SCENECUT_STRENGTH_FACTOR, MAX_SCENECUT_THRESHOLD);
        m_scaledChromaThreshold = x265_min(m_chromaHistThreshold * SCENECUT_STRENGTH_FACTOR, MAX_SCENECUT_THRESHOLD);
        if (m_param->sourceBitDepth != m_param->internalBitDepth)
        {
            int size = m_param->sourceWidth * m_param->sourceHeight;
            int hshift = CHROMA_H_SHIFT(m_param->internalCsp);
            int vshift = CHROMA_V_SHIFT(m_param->internalCsp);
            int widthC = m_param->sourceWidth >> hshift;
            int heightC = m_param->sourceHeight >> vshift;

            m_inputPic[0] = X265_MALLOC(pixel, size);
            if (m_param->internalCsp != X265_CSP_I400)
            {
                for (int j = 1; j < 3; j++)
                {
                    m_inputPic[j] = X265_MALLOC(pixel, widthC * heightC);
                }
            }
        }
    }

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

    m_numPools = 0;
    if (allowPools)
        m_threadPool = ThreadPool::allocThreadPools(p, m_numPools, 0);
    else
    {
        if (!p->frameNumThreads)
        {
            // auto-detect frame threads
            int cpuCount = ThreadPool::getCpuCount();
            ThreadPool::getFrameThreadsCount(p, cpuCount);
        }
    }

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

    x265_log(p, X265_LOG_INFO, "Slices                              : %d\n", p->maxSlices);

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
    int pools = m_numPools;
    ThreadPool* lookAheadThreadPool = 0;
    if (m_param->lookaheadThreads > 0)
    {
        lookAheadThreadPool = ThreadPool::allocThreadPools(p, pools, 1);
    }
    else
        lookAheadThreadPool = m_threadPool;
    m_lookahead = new Lookahead(m_param, lookAheadThreadPool);
    if (pools)
    {
        m_lookahead->m_jpId = lookAheadThreadPool[0].m_numProviders++;
        lookAheadThreadPool[0].m_jpTable[m_lookahead->m_jpId] = m_lookahead;
    }
    if (m_param->lookaheadThreads > 0)
        for (int i = 0; i < pools; i++)
            lookAheadThreadPool[i].start();
    m_lookahead->m_numPools = pools;
    m_dpb = new DPB(m_param);
    m_rateControl = new RateControl(*m_param, this);
    if (!m_param->bResetZoneConfig)
    {
        zoneReadCount = new ThreadSafeInteger[m_param->rc.zonefileCount];
        zoneWriteCount = new ThreadSafeInteger[m_param->rc.zonefileCount];
    }

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
            m_scalingList.setupQuantMatrices(m_sps.chromaFormatIdc);
        }
        else
            m_scalingList.setupQuantMatrices(m_sps.chromaFormatIdc);

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
            m_scalingList.setupQuantMatrices(m_sps.chromaFormatIdc);
        }
    }
    else
        m_scalingList.setupQuantMatrices(m_sps.chromaFormatIdc);

    int numRows = (m_param->sourceHeight + m_param->maxCUSize - 1) / m_param->maxCUSize;
    int numCols = (m_param->sourceWidth  + m_param->maxCUSize - 1) / m_param->maxCUSize;
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

    initRefIdx();
    if (m_param->analysisSave && m_param->bUseAnalysisFile)
    {
        char* temp = strcatFilename(m_param->analysisSave, ".temp");
        if (!temp)
            m_aborted = true;
        else
        {
            m_analysisFileOut = x265_fopen(temp, "wb");
            X265_FREE(temp);
        }
        if (!m_analysisFileOut)
        {
            x265_log_file(NULL, X265_LOG_ERROR, "Analysis save: failed to open file %s.temp\n", m_param->analysisSave);
            m_aborted = true;
        }
    }

    if (m_param->analysisMultiPassRefine || m_param->analysisMultiPassDistortion)
    {
        const char* name = m_param->analysisReuseFileName;
        if (!name)
            name = defaultAnalysisFileName;
        if (m_param->rc.bStatWrite)
        {
            char* temp = strcatFilename(name, ".temp");
            if (!temp)
                m_aborted = true;
            else
            {
                m_analysisFileOut = x265_fopen(temp, "wb");
                X265_FREE(temp);
            }
            if (!m_analysisFileOut)
            {
                x265_log_file(NULL, X265_LOG_ERROR, "Analysis 2 pass: failed to open file %s.temp\n", name);
                m_aborted = true;
            }
        }
        if (m_param->rc.bStatRead)
        {
            m_analysisFileIn = x265_fopen(name, "rb");
            if (!m_analysisFileIn)
            {
                x265_log_file(NULL, X265_LOG_ERROR, "Analysis 2 pass: failed to open file %s\n", name);
                m_aborted = true;
            }
        }
    }
    m_bZeroLatency = !m_param->bframes && !m_param->lookaheadDepth && m_param->frameNumThreads == 1 && m_param->maxSlices == 1;
    m_aborted |= parseLambdaFile(m_param);

    m_encodeStartTime = x265_mdate();

    m_nalList.m_annexB = !!m_param->bAnnexB;

    if (m_param->naluFile)
    {
        m_naluFile = x265_fopen(m_param->naluFile, "r");
        if (!m_naluFile)
        {
            x265_log_file(NULL, X265_LOG_ERROR, "%s file not found or Failed to open\n", m_param->naluFile);
            m_aborted = true;
        }
        else
             m_enableNal = 1;
    }
    else
         m_enableNal = 0;

#if ENABLE_HDR10_PLUS
    if (m_bToneMap)
        m_numCimInfo = m_hdr10plus_api->hdr10plus_json_to_movie_cim(m_param->toneMapFile, m_cim);
#endif
    if (m_param->bDynamicRefine)
    {
        /* Allocate memory for 1 GOP and reuse it for the subsequent GOPs */
        int size = (m_param->keyframeMax + m_param->lookaheadDepth) * m_param->maxCUDepth * X265_REFINE_INTER_LEVELS;
        CHECKED_MALLOC_ZERO(m_variance, uint64_t, size);
        CHECKED_MALLOC_ZERO(m_rdCost, uint64_t, size);
        CHECKED_MALLOC_ZERO(m_trainingCount, uint32_t, size);
        return;
    fail:
        m_aborted = true;
    }
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
    {
        for (int i = 0; i < m_numPools; i++)
            m_threadPool[i].stopWorkers();
    }
}

int Encoder::copySlicetypePocAndSceneCut(int *slicetype, int *poc, int *sceneCut)
{
    Frame *FramePtr = m_dpb->m_picList.getCurFrame();
    if (FramePtr != NULL)
    {
        *slicetype = FramePtr->m_lowres.sliceType;
        *poc = FramePtr->m_encData->m_slice->m_poc;
        *sceneCut = FramePtr->m_lowres.bScenecut;
    }
    else
    {
        x265_log(NULL, X265_LOG_WARNING, "Frame is still in lookahead pipeline, this API must be called after (poc >= lookaheadDepth + bframes + 2) condition check\n");
        return -1;
    }
    return 0;
}

int Encoder::getRefFrameList(PicYuv** l0, PicYuv** l1, int sliceType, int poc, int* pocL0, int* pocL1)
{
    if (!(IS_X265_TYPE_I(sliceType)))
    {
        Frame *framePtr = m_dpb->m_picList.getPOC(poc);
        if (framePtr != NULL)
        {
            for (int j = 0; j < framePtr->m_encData->m_slice->m_numRefIdx[0]; j++)    // check only for --ref=n number of frames.
            {
                if (framePtr->m_encData->m_slice->m_refFrameList[0][j] && framePtr->m_encData->m_slice->m_refFrameList[0][j]->m_reconPic != NULL)
                {
                    int l0POC = framePtr->m_encData->m_slice->m_refFrameList[0][j]->m_poc;
                    pocL0[j] = l0POC;
                    Frame* l0Fp = m_dpb->m_picList.getPOC(l0POC);
                    while (l0Fp->m_reconRowFlag[l0Fp->m_numRows - 1].get() == 0)
                        l0Fp->m_reconRowFlag[l0Fp->m_numRows - 1].waitForChange(0); /* If recon is not ready, current frame encoder has to wait. */
                    l0[j] = l0Fp->m_reconPic;
                }
            }
            for (int j = 0; j < framePtr->m_encData->m_slice->m_numRefIdx[1]; j++)    // check only for --ref=n number of frames.
            {
                if (framePtr->m_encData->m_slice->m_refFrameList[1][j] && framePtr->m_encData->m_slice->m_refFrameList[1][j]->m_reconPic != NULL)
                {
                    int l1POC = framePtr->m_encData->m_slice->m_refFrameList[1][j]->m_poc;
                    pocL1[j] = l1POC;
                    Frame* l1Fp = m_dpb->m_picList.getPOC(l1POC);
                    while (l1Fp->m_reconRowFlag[l1Fp->m_numRows - 1].get() == 0)
                        l1Fp->m_reconRowFlag[l1Fp->m_numRows - 1].waitForChange(0); /* If recon is not ready, current frame encoder has to wait. */
                    l1[j] = l1Fp->m_reconPic;
                }
            }
        }
        else
        {
            x265_log(NULL, X265_LOG_WARNING, "Current frame is not in DPB piclist.\n");
            return 1;
        }
    }
    else
    {
        x265_log(NULL, X265_LOG_ERROR, "I frames does not have a refrence List\n");
        return -1;
    }
    return 0;
}

int Encoder::setAnalysisDataAfterZScan(x265_analysis_data *analysis_data, Frame* curFrame)
{
    int mbImageWidth, mbImageHeight;
    mbImageWidth = (curFrame->m_fencPic->m_picWidth + 16 - 1) >> 4; //AVC block sizes
    mbImageHeight = (curFrame->m_fencPic->m_picHeight + 16 - 1) >> 4;
    if (analysis_data->sliceType == X265_TYPE_IDR || analysis_data->sliceType == X265_TYPE_I)
    {
        curFrame->m_analysisData.sliceType = X265_TYPE_I;
        if (m_param->analysisLoadReuseLevel < 7)
            return -1;
        curFrame->m_analysisData.numPartitions = m_param->num4x4Partitions;
        int num16x16inCUWidth = m_param->maxCUSize >> 4;
        uint32_t ctuAddr, offset, cuPos;
        x265_analysis_intra_data * intraData = curFrame->m_analysisData.intraData;
        x265_analysis_intra_data * srcIntraData = analysis_data->intraData;
        for (int i = 0; i < mbImageHeight; i++)
        {
            for (int j = 0; j < mbImageWidth; j++)
            {
                int mbIndex = j + i * mbImageWidth;
                ctuAddr = (j / num16x16inCUWidth + ((i / num16x16inCUWidth) * (mbImageWidth / num16x16inCUWidth)));
                offset = ((i % num16x16inCUWidth) << 5) + ((j % num16x16inCUWidth) << 4);
                if ((j % 4 >= 2) && m_param->maxCUSize == 64)
                    offset += (2 * 16);
                if ((i % 4 >= 2) && m_param->maxCUSize == 64)
                    offset += (2 * 32);
                cuPos = ctuAddr  * curFrame->m_analysisData.numPartitions + offset;
                memcpy(&(intraData)->depth[cuPos], &(srcIntraData)->depth[mbIndex * 16], 16);
                memcpy(&(intraData)->chromaModes[cuPos], &(srcIntraData)->chromaModes[mbIndex * 16], 16);
                memcpy(&(intraData)->partSizes[cuPos], &(srcIntraData)->partSizes[mbIndex * 16], 16);
                memcpy(&(intraData)->partSizes[cuPos], &(srcIntraData)->partSizes[mbIndex * 16], 16);
            }
        }
        memcpy(&(intraData)->modes, (srcIntraData)->modes, curFrame->m_analysisData.numPartitions * analysis_data->numCUsInFrame);
    }
    else
    {
        uint32_t numDir = analysis_data->sliceType == X265_TYPE_P ? 1 : 2;
        if (m_param->analysisLoadReuseLevel < 7)
            return -1;
        curFrame->m_analysisData.numPartitions = m_param->num4x4Partitions;
        int num16x16inCUWidth = m_param->maxCUSize >> 4;
        uint32_t ctuAddr, offset, cuPos;
        x265_analysis_inter_data * interData = curFrame->m_analysisData.interData;
        x265_analysis_inter_data * srcInterData = analysis_data->interData;
        for (int i = 0; i < mbImageHeight; i++)
        {
            for (int j = 0; j < mbImageWidth; j++)
            {
                int mbIndex = j + i * mbImageWidth;
                ctuAddr = (j / num16x16inCUWidth + ((i / num16x16inCUWidth) * (mbImageWidth / num16x16inCUWidth)));
                offset = ((i % num16x16inCUWidth) << 5) + ((j % num16x16inCUWidth) << 4);
                if ((j % 4 >= 2) && m_param->maxCUSize == 64)
                    offset += (2 * 16);
                if ((i % 4 >= 2) && m_param->maxCUSize == 64)
                    offset += (2 * 32);
                cuPos = ctuAddr  * curFrame->m_analysisData.numPartitions + offset;
                memcpy(&(interData)->depth[cuPos], &(srcInterData)->depth[mbIndex * 16], 16);
                memcpy(&(interData)->modes[cuPos], &(srcInterData)->modes[mbIndex * 16], 16);

                memcpy(&(interData)->partSize[cuPos], &(srcInterData)->partSize[mbIndex * 16], 16);

                int bytes = curFrame->m_analysisData.numPartitions >> ((srcInterData)->depth[mbIndex * 16] * 2);
                int cuCount = 1;
                if (bytes < 16)
                    cuCount = 4;
                for (int cuI = 0; cuI < cuCount; cuI++)
                {
                    int numPU = nbPartsTable[(srcInterData)->partSize[mbIndex * 16 + cuI * bytes]];
                    for (int pu = 0; pu < numPU; pu++)
                    {
                        int cuOffset = cuI * bytes + pu;
                        (interData)->mergeFlag[cuPos + cuOffset] = (srcInterData)->mergeFlag[(mbIndex * 16) + cuOffset];
                        (interData)->sadCost[cuPos + cuOffset] = (srcInterData)->sadCost[(mbIndex * 16) + cuOffset];
                        (interData)->interDir[cuPos + cuOffset] = (srcInterData)->interDir[(mbIndex * 16) + cuOffset];
                        for (uint32_t k = 0; k < numDir; k++)
                        {
                            (interData)->mvpIdx[k][cuPos + cuOffset] = (srcInterData)->mvpIdx[k][(mbIndex * 16) + cuOffset];
                            (interData)->refIdx[k][cuPos + cuOffset] = (srcInterData)->refIdx[k][(mbIndex * 16) + cuOffset];
                            memcpy(&(interData)->mv[k][cuPos + cuOffset], &(srcInterData)->mv[k][(mbIndex * 16) + cuOffset], sizeof(MV));
                            if (m_param->analysisLoadReuseLevel == 7 && numPU == PU_2Nx2N &&
                                ((interData)->depth[cuPos + cuOffset] == (m_param->maxCUSize >> 5)))
                            {
                                int mv_x = (interData)->mv[k][cuPos + cuOffset].x;
                                int mv_y = (interData)->mv[k][cuPos + cuOffset].y;
                                if ((mv_x*mv_x + mv_y*mv_y) <= MVTHRESHOLD)
                                    memset(&curFrame->m_analysisData.modeFlag[k][cuPos + cuOffset], 1, bytes);
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int Encoder::setAnalysisData(x265_analysis_data *analysis_data, int poc, uint32_t cuBytes)
{
    uint32_t widthInCU = (m_param->sourceWidth + m_param->maxCUSize - 1) >> m_param->maxLog2CUSize;
    uint32_t heightInCU = (m_param->sourceHeight + m_param->maxCUSize - 1) >> m_param->maxLog2CUSize;

    Frame* curFrame = m_dpb->m_picList.getPOC(poc);
    if (curFrame != NULL)
    {
        curFrame->m_analysisData = (*analysis_data);
        curFrame->m_analysisData.numCUsInFrame = widthInCU * heightInCU;
        curFrame->m_analysisData.numPartitions = m_param->num4x4Partitions;
        x265_alloc_analysis_data(m_param, &curFrame->m_analysisData);
        if (m_param->maxCUSize == 16)
        {
            if (analysis_data->sliceType == X265_TYPE_IDR || analysis_data->sliceType == X265_TYPE_I)
            {
                curFrame->m_analysisData.sliceType = X265_TYPE_I;
                if (m_param->analysisLoadReuseLevel < 2)
                    return -1;

                curFrame->m_analysisData.numPartitions = m_param->num4x4Partitions;
                size_t count = 0;
                x265_analysis_intra_data * currIntraData = curFrame->m_analysisData.intraData;
                x265_analysis_intra_data * intraData = analysis_data->intraData;
                for (uint32_t d = 0; d < cuBytes; d++)
                {
                    int bytes = curFrame->m_analysisData.numPartitions >> ((intraData)->depth[d] * 2);
                    memset(&(currIntraData)->depth[count], (intraData)->depth[d], bytes);
                    memset(&(currIntraData)->chromaModes[count], (intraData)->chromaModes[d], bytes);
                    memset(&(currIntraData)->partSizes[count], (intraData)->partSizes[d], bytes);
                    memset(&(currIntraData)->partSizes[count], (intraData)->partSizes[d], bytes);
                    count += bytes;
                }
                memcpy(&(currIntraData)->modes, (intraData)->modes, curFrame->m_analysisData.numPartitions * analysis_data->numCUsInFrame);
            }
            else
            {
                uint32_t numDir = analysis_data->sliceType == X265_TYPE_P ? 1 : 2;
                if (m_param->analysisLoadReuseLevel < 2)
                    return -1;

                curFrame->m_analysisData.numPartitions = m_param->num4x4Partitions;
                size_t count = 0;
                x265_analysis_inter_data * currInterData = curFrame->m_analysisData.interData;
                x265_analysis_inter_data * interData = analysis_data->interData;
                for (uint32_t d = 0; d < cuBytes; d++)
                {
                    int bytes = curFrame->m_analysisData.numPartitions >> ((interData)->depth[d] * 2);
                    memset(&(currInterData)->depth[count], (interData)->depth[d], bytes);
                    memset(&(currInterData)->modes[count], (interData)->modes[d], bytes);
                    memcpy(&(currInterData)->sadCost[count], &(analysis_data->interData)->sadCost[d], bytes);
                    if (m_param->analysisLoadReuseLevel > 4)
                    {
                        memset(&(currInterData)->partSize[count], (interData)->partSize[d], bytes);
                        int numPU = nbPartsTable[(interData)->partSize[d]];
                        for (int pu = 0; pu < numPU; pu++)
                        {
                            if (pu) d++;
                            (currInterData)->mergeFlag[count + pu] = (interData)->mergeFlag[d];
                            if (m_param->analysisLoadReuseLevel >= 7)
                            {
                                (currInterData)->interDir[count + pu] = (interData)->interDir[d];
                                for (uint32_t i = 0; i < numDir; i++)
                                {
                                    (currInterData)->mvpIdx[i][count + pu] = (interData)->mvpIdx[i][d];
                                    (currInterData)->refIdx[i][count + pu] = (interData)->refIdx[i][d];
                                    memcpy(&(currInterData)->mv[i][count + pu], &(interData)->mv[i][d], sizeof(MV));
                                    if (m_param->analysisLoadReuseLevel == 7 && numPU == PU_2Nx2N && m_param->num4x4Partitions <= 16)
                                    {
                                        int mv_x = (currInterData)->mv[i][count + pu].x;
                                        int mv_y = (currInterData)->mv[i][count + pu].y;
                                        if ((mv_x*mv_x + mv_y*mv_y) <= MVTHRESHOLD)
                                            memset(&curFrame->m_analysisData.modeFlag[i][count + pu], 1, bytes);
                                    }
                                }
                            }
                        }
                    }
                    count += bytes;
                }
            }
        }
        else
            setAnalysisDataAfterZScan(analysis_data, curFrame);

        curFrame->m_copyMVType.trigger();
        return 0;
    }
    return -1;
}

void Encoder::destroy()
{
#if ENABLE_HDR10_PLUS
    if (m_bToneMap)
        m_hdr10plus_api->hdr10plus_clear_movie(m_cim, m_numCimInfo);
#endif

    if (m_param->bDynamicRefine)
    {
        X265_FREE(m_variance);
        X265_FREE(m_rdCost);
        X265_FREE(m_trainingCount);
    }
    if (m_exportedPic)
    {
        ATOMIC_DEC(&m_exportedPic->m_countRefEncoders);
        m_exportedPic = NULL;
    }

    if (m_param->bEnableFrameDuplication)
    {
        for (uint32_t i = 0; i < DUP_BUFFER; i++)
        {
            X265_FREE(m_dupBuffer[i]->dupPlane);
            x265_picture_free(m_dupBuffer[i]->dupPic);
            X265_FREE(m_dupBuffer[i]);
        }

        if (!(m_param->sourceBitDepth == 8 && m_param->internalBitDepth == 8))
        {
            for (int k = 0; k < 3; k++)
            {
                if (k == 0)
                {
                    X265_FREE(m_dupPicOne[k]);
                    X265_FREE(m_dupPicTwo[k]);
                }
                else if(k >= 1 && m_param->internalCsp != X265_CSP_I400)
                {
                    X265_FREE(m_dupPicOne[k]);
                    X265_FREE(m_dupPicTwo[k]);
                }
            }
        }
    }

    if (m_param->bHistBasedSceneCut)
    {
        if (m_edgePic != NULL)
        {
            X265_FREE_ZERO(m_edgePic);
        }

        if (m_param->sourceBitDepth != m_param->internalBitDepth)
        {
            X265_FREE_ZERO(m_inputPic[0]);
            if (m_param->internalCsp != X265_CSP_I400)
            {
                for (int i = 1; i < 3; i++)
                {
                    X265_FREE_ZERO(m_inputPic[i]);
                }
            }
        }
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
    if (!m_param->bResetZoneConfig && m_param->rc.zonefileCount)
    {
        delete[] zoneReadCount;
        delete[] zoneWriteCount;
    }
    if (m_rateControl)
    {
        m_rateControl->destroy();
        delete m_rateControl;
    }

    X265_FREE(m_offsetEmergency);

    if (m_latestParam != NULL && m_latestParam != m_param)
    {
        if (m_latestParam->scalingLists != m_param->scalingLists)
            free((char*)m_latestParam->scalingLists);

        PARAM_NS::x265_param_free(m_latestParam);
    }
    if (m_analysisFileIn)
        fclose(m_analysisFileIn);

    if (m_analysisFileOut)
    {
        int bError = 1;
        fclose(m_analysisFileOut);
        const char* name = m_param->analysisSave ? m_param->analysisSave : m_param->analysisReuseFileName;
        if (!name)
            name = defaultAnalysisFileName;
        char* temp = strcatFilename(name, ".temp");
        if (temp)
        {
            x265_unlink(name);
            bError = x265_rename(temp, name);
        }
        if (bError)
        {
            x265_log_file(m_param, X265_LOG_ERROR, "failed to rename analysis stats file to \"%s\"\n", name);
        }
        X265_FREE(temp);
     }
    if (m_naluFile)
        fclose(m_naluFile);

#ifdef SVT_HEVC
    X265_FREE(m_svtAppData);
#endif
    if (m_param)
    {
        if (m_param->csvfpt)
            fclose(m_param->csvfpt);
        /* release string arguments that were strdup'd */
        free((char*)m_param->rc.lambdaFileName);
        free((char*)m_param->rc.statFileName);
        free((char*)m_param->analysisReuseFileName);
        free((char*)m_param->scalingLists);
        free((char*)m_param->csvfn);
        free((char*)m_param->numaPools);
        free((char*)m_param->masteringDisplayColorVolume);
        free((char*)m_param->toneMapFile);
        free((char*)m_param->analysisSave);
        free((char*)m_param->analysisLoad);
        PARAM_NS::x265_param_free(m_param);
    }
}

void Encoder::updateVbvPlan(RateControl* rc)
{
    for (int i = 0; i < m_param->frameNumThreads; i++)
    {
        FrameEncoder *encoder = m_frameEncoder[i];
        if (encoder->m_rce.isActive && encoder->m_rce.poc != rc->m_curSlice->m_poc)
        {
            int64_t bits = m_param->rc.bEnableConstVbv ? (int64_t)encoder->m_rce.frameSizePlanned : (int64_t)X265_MAX(encoder->m_rce.frameSizeEstimated, encoder->m_rce.frameSizePlanned);
            rc->m_bufferFill -= bits;
            rc->m_bufferFill = X265_MAX(rc->m_bufferFill, 0);
            rc->m_bufferFill += encoder->m_rce.bufferRate;
            rc->m_bufferFill = X265_MIN(rc->m_bufferFill, rc->m_bufferSize);
            if (rc->m_2pass)
                rc->m_predictedBits += bits;
        }
    }
}

void Encoder::calcRefreshInterval(Frame* frameEnc)
{
    Slice* slice = frameEnc->m_encData->m_slice;
    uint32_t numBlocksInRow = slice->m_sps->numCuInWidth;
    FrameData::PeriodicIR* pir = &frameEnc->m_encData->m_pir;
    if (slice->m_sliceType == I_SLICE)
    {
        pir->framesSinceLastPir = 0;
        m_bQueuedIntraRefresh = 0;
        /* PIR is currently only supported with ref == 1, so any intra frame effectively refreshes
         * the whole frame and counts as an intra refresh. */
        pir->pirEndCol = numBlocksInRow;
    }
    else if (slice->m_sliceType == P_SLICE)
    {
        Frame* ref = frameEnc->m_encData->m_slice->m_refFrameList[0][0];
        int pocdiff = frameEnc->m_poc - ref->m_poc;
        int numPFramesInGOP = m_param->keyframeMax / pocdiff;
        int increment = (numBlocksInRow + numPFramesInGOP - 1) / numPFramesInGOP;
        pir->pirEndCol = ref->m_encData->m_pir.pirEndCol;
        pir->framesSinceLastPir = ref->m_encData->m_pir.framesSinceLastPir + pocdiff;
        if (pir->framesSinceLastPir >= m_param->keyframeMax ||
            (m_bQueuedIntraRefresh && pir->pirEndCol >= numBlocksInRow))
        {
            pir->pirEndCol = 0;
            pir->framesSinceLastPir = 0;
            m_bQueuedIntraRefresh = 0;
            frameEnc->m_lowres.bKeyframe = 1;
        }
        pir->pirStartCol = pir->pirEndCol;
        pir->pirEndCol += increment;
        /* If our intra refresh has reached the right side of the frame, we're done. */
        if (pir->pirEndCol >= numBlocksInRow)
        {
            pir->pirEndCol = numBlocksInRow;
        }
    }
}

void Encoder::copyUserSEIMessages(Frame *frame, const x265_picture* pic_in)
{
    x265_sei_payload toneMap;
    toneMap.payload = NULL;
    int toneMapPayload = 0;

#if ENABLE_HDR10_PLUS
    if (m_bToneMap)
    {
        int currentPOC = m_pocLast;
        if (currentPOC < m_numCimInfo)
        {
            int32_t i = 0;
            toneMap.payloadSize = 0;
            while (m_cim[currentPOC][i] == 0xFF)
                toneMap.payloadSize += m_cim[currentPOC][i++];
            toneMap.payloadSize += m_cim[currentPOC][i];

            toneMap.payload = (uint8_t*)x265_malloc(sizeof(uint8_t) * toneMap.payloadSize);
            toneMap.payloadType = USER_DATA_REGISTERED_ITU_T_T35;
            memcpy(toneMap.payload, &m_cim[currentPOC][i + 1], toneMap.payloadSize);
            toneMapPayload = 1;
        }
    }
#endif
    /* seiMsg will contain SEI messages specified in a fixed file format in POC order.
    * Format of the file : <POC><space><PREFIX><space><NAL UNIT TYPE>/<SEI TYPE><space><SEI Payload> */
    x265_sei_payload seiMsg;
    seiMsg.payload = NULL;
    int userPayload = 0;
    if (m_enableNal)
    {
        readUserSeiFile(seiMsg, m_pocLast);
        if (seiMsg.payload)
            userPayload = 1;;
    }

    int numPayloads = pic_in->userSEI.numPayloads + toneMapPayload + userPayload;
    frame->m_userSEI.numPayloads = numPayloads;

    if (frame->m_userSEI.numPayloads)
    {
        if (!frame->m_userSEI.payloads)
        {
            frame->m_userSEI.payloads = new x265_sei_payload[numPayloads];
            for (int i = 0; i < numPayloads; i++)
                frame->m_userSEI.payloads[i].payload = NULL;
        }
        for (int i = 0; i < numPayloads; i++)
        {
            x265_sei_payload input;
            if ((i == (numPayloads - 1)) && toneMapPayload)
                input = toneMap;
            else if (m_enableNal)
                input = seiMsg;
            else
                input = pic_in->userSEI.payloads[i];

            if (!frame->m_userSEI.payloads[i].payload)
                frame->m_userSEI.payloads[i].payload = new uint8_t[input.payloadSize];
            memcpy(frame->m_userSEI.payloads[i].payload, input.payload, input.payloadSize);
            frame->m_userSEI.payloads[i].payloadSize = input.payloadSize;
            frame->m_userSEI.payloads[i].payloadType = input.payloadType;
        }
        if (toneMap.payload)
            x265_free(toneMap.payload);
        if (seiMsg.payload)
            x265_free(seiMsg.payload);
    }
}

//Find Sum of Squared Difference (SSD) between two pictures
uint64_t Encoder::computeSSD(pixel *fenc, pixel *rec, intptr_t stride, uint32_t width, uint32_t height, x265_param *param)
{
    uint64_t ssd = 0;

    if (!param->bEnableFrameDuplication || (width & 3))
    {
        if ((width | height) & 3)
        {
            /* Slow Path */
            for (uint32_t y = 0; y < height; y++)
            {
                for (uint32_t x = 0; x < width; x++)
                {
                    int diff = (int)(fenc[x] - rec[x]);
                    ssd += diff * diff;
                }

                fenc += stride;
                rec += stride;
            }

            return ssd;
        }
    }

    uint32_t y = 0;

    /* Consume rows in ever narrower chunks of height */
    for (int size = BLOCK_64x64; size >= BLOCK_4x4 && y < height; size--)
    {
        uint32_t rowHeight = 1 << (size + 2);

        for (; y + rowHeight <= height; y += rowHeight)
        {
            uint32_t y1, x = 0;

            /* Consume each row using the largest square blocks possible */
            if (size == BLOCK_64x64 && !(stride & 31))
                for (; x + 64 <= width; x += 64)
                    ssd += primitives.cu[BLOCK_64x64].sse_pp(fenc + x, stride, rec + x, stride);

            if (size >= BLOCK_32x32 && !(stride & 15))
                for (; x + 32 <= width; x += 32)
                    for (y1 = 0; y1 + 32 <= rowHeight; y1 += 32)
                        ssd += primitives.cu[BLOCK_32x32].sse_pp(fenc + y1 * stride + x, stride, rec + y1 * stride + x, stride);

            if (size >= BLOCK_16x16)
                for (; x + 16 <= width; x += 16)
                    for (y1 = 0; y1 + 16 <= rowHeight; y1 += 16)
                        ssd += primitives.cu[BLOCK_16x16].sse_pp(fenc + y1 * stride + x, stride, rec + y1 * stride + x, stride);

            if (size >= BLOCK_8x8)
                for (; x + 8 <= width; x += 8)
                    for (y1 = 0; y1 + 8 <= rowHeight; y1 += 8)
                        ssd += primitives.cu[BLOCK_8x8].sse_pp(fenc + y1 * stride + x, stride, rec + y1 * stride + x, stride);

            for (; x + 4 <= width; x += 4)
                for (y1 = 0; y1 + 4 <= rowHeight; y1 += 4)
                    ssd += primitives.cu[BLOCK_4x4].sse_pp(fenc + y1 * stride + x, stride, rec + y1 * stride + x, stride);

            fenc += stride * rowHeight;
            rec += stride * rowHeight;
        }
    }

    /* Handle last few rows of frames for videos 
    with height not divisble by 4 */
    uint32_t h = height % y;
    if (param->bEnableFrameDuplication && h)
    {
        for (uint32_t i = 0; i < h; i++)
        {
            for (uint32_t j = 0; j < width; j++)
            {
                int diff = (int)(fenc[j] - rec[j]);
                ssd += diff * diff;
            }

            fenc += stride;
            rec += stride;
        }
    }

    return ssd;
}

//Compute the PSNR weightage between two pictures
double Encoder::ComputePSNR(x265_picture *firstPic, x265_picture *secPic, x265_param *param)
{
    uint64_t ssdY = 0, ssdU = 0, ssdV = 0;
    intptr_t strideL, strideC;
    uint32_t widthL, heightL, widthC, heightC;
    double psnrY = 0, psnrU = 0, psnrV = 0, psnrWeight = 0;
    int width = firstPic->width;
    int height = firstPic->height;
    int hshift = CHROMA_H_SHIFT(firstPic->colorSpace);
    int vshift = CHROMA_V_SHIFT(firstPic->colorSpace);
    pixel *yFirstPic = NULL, *ySecPic = NULL;
    pixel *uFirstPic = NULL, *uSecPic = NULL;
    pixel *vFirstPic = NULL, *vSecPic = NULL;

    strideL = widthL = width;
    heightL = height;

    strideC = widthC = widthL >> hshift;
    heightC = heightL >> vshift;

    int size = width * height;
    int maxvalY = 255 << (X265_DEPTH - 8);
    int maxvalC = 255 << (X265_DEPTH - 8);
    double refValueY = (double)maxvalY * maxvalY * size;
    double refValueC = (double)maxvalC * maxvalC * size / 4.0;

    if (firstPic->bitDepth == 8 && X265_DEPTH == 8)
    {
        yFirstPic = (pixel*)firstPic->planes[0];
        ySecPic = (pixel*)secPic->planes[0];
        if (param->internalCsp != X265_CSP_I400)
        {
            uFirstPic = (pixel*)firstPic->planes[1];
            uSecPic = (pixel*)secPic->planes[1];
            vFirstPic = (pixel*)firstPic->planes[2];
            vSecPic = (pixel*)secPic->planes[2];
        }
    }
    else if (firstPic->bitDepth == 8 && X265_DEPTH > 8)
    {
        int shift = (X265_DEPTH - 8);
        uint8_t *yChar1, *yChar2, *uChar1, *uChar2, *vChar1, *vChar2;

        yChar1 = (uint8_t*)firstPic->planes[0];
        yChar2 = (uint8_t*)secPic->planes[0];

        primitives.planecopy_cp(yChar1, firstPic->stride[0] / sizeof(*yChar1), m_dupPicOne[0], firstPic->stride[0] / sizeof(*yChar1), width, height, shift);
        primitives.planecopy_cp(yChar2, secPic->stride[0] / sizeof(*yChar2), m_dupPicTwo[0], secPic->stride[0] / sizeof(*yChar2), width, height, shift);

        if (param->internalCsp != X265_CSP_I400)
        {
            uChar1 = (uint8_t*)firstPic->planes[1];
            uChar2 = (uint8_t*)secPic->planes[1];
            vChar1 = (uint8_t*)firstPic->planes[2];
            vChar2 = (uint8_t*)secPic->planes[2];

            primitives.planecopy_cp(uChar1, firstPic->stride[1] / sizeof(*uChar1), m_dupPicOne[1], firstPic->stride[1] / sizeof(*uChar1), widthC, heightC, shift);
            primitives.planecopy_cp(uChar2, secPic->stride[1] / sizeof(*uChar2), m_dupPicTwo[1], secPic->stride[1] / sizeof(*uChar2), widthC, heightC, shift);

            primitives.planecopy_cp(vChar1, firstPic->stride[2] / sizeof(*vChar1), m_dupPicOne[2], firstPic->stride[2] / sizeof(*vChar1), widthC, heightC, shift);
            primitives.planecopy_cp(vChar2, secPic->stride[2] / sizeof(*vChar2), m_dupPicTwo[2], secPic->stride[2] / sizeof(*vChar2), widthC, heightC, shift);
        }
    }
    else
    {
        uint16_t *yShort1, *yShort2, *uShort1, *uShort2, *vShort1, *vShort2;
        /* defensive programming, mask off bits that are supposed to be zero */
        uint16_t mask = (1 << X265_DEPTH) - 1;
        int shift = abs(firstPic->bitDepth - X265_DEPTH);

        yShort1 = (uint16_t*)firstPic->planes[0];
        yShort2 = (uint16_t*)secPic->planes[0];

        if (firstPic->bitDepth > X265_DEPTH)
        {
            /* shift right and mask pixels to final size */
            primitives.planecopy_sp(yShort1, firstPic->stride[0] / sizeof(*yShort1), m_dupPicOne[0], firstPic->stride[0] / sizeof(*yShort1), width, height, shift, mask);
            primitives.planecopy_sp(yShort2, secPic->stride[0] / sizeof(*yShort2), m_dupPicTwo[0], secPic->stride[0] / sizeof(*yShort2), width, height, shift, mask);
        }
        else /* Case for (pic.bitDepth <= X265_DEPTH) */
        {
            /* shift left and mask pixels to final size */
            primitives.planecopy_sp_shl(yShort1, firstPic->stride[0] / sizeof(*yShort1), m_dupPicOne[0], firstPic->stride[0] / sizeof(*yShort1), width, height, shift, mask);
            primitives.planecopy_sp_shl(yShort2, secPic->stride[0] / sizeof(*yShort2), m_dupPicTwo[0], secPic->stride[0] / sizeof(*yShort2), width, height, shift, mask);
        }

        if (param->internalCsp != X265_CSP_I400)
        {
            uShort1 = (uint16_t*)firstPic->planes[1];
            uShort2 = (uint16_t*)secPic->planes[1];
            vShort1 = (uint16_t*)firstPic->planes[2];
            vShort2 = (uint16_t*)secPic->planes[2];

            if (firstPic->bitDepth > X265_DEPTH)
            {
                primitives.planecopy_sp(uShort1, firstPic->stride[1] / sizeof(*uShort1), m_dupPicOne[1], firstPic->stride[1] / sizeof(*uShort1), widthC, heightC, shift, mask);
                primitives.planecopy_sp(uShort2, secPic->stride[1] / sizeof(*uShort2), m_dupPicTwo[1], secPic->stride[1] / sizeof(*uShort2), widthC, heightC, shift, mask);

                primitives.planecopy_sp(vShort1, firstPic->stride[2] / sizeof(*vShort1), m_dupPicOne[2], firstPic->stride[2] / sizeof(*vShort1), widthC, heightC, shift, mask);
                primitives.planecopy_sp(vShort2, secPic->stride[2] / sizeof(*vShort2), m_dupPicTwo[2], secPic->stride[2] / sizeof(*vShort2), widthC, heightC, shift, mask);
            }
            else /* Case for (pic.bitDepth <= X265_DEPTH) */
            {
                primitives.planecopy_sp_shl(uShort1, firstPic->stride[1] / sizeof(*uShort1), m_dupPicOne[1], firstPic->stride[1] / sizeof(*uShort1), widthC, heightC, shift, mask);
                primitives.planecopy_sp_shl(uShort2, secPic->stride[1] / sizeof(*uShort2), m_dupPicTwo[1], secPic->stride[1] / sizeof(*uShort2), widthC, heightC, shift, mask);

                primitives.planecopy_sp_shl(vShort1, firstPic->stride[2] / sizeof(*vShort1), m_dupPicOne[2], firstPic->stride[2] / sizeof(*vShort1), widthC, heightC, shift, mask);
                primitives.planecopy_sp_shl(vShort2, secPic->stride[2] / sizeof(*vShort2), m_dupPicTwo[2], secPic->stride[2] / sizeof(*vShort2), widthC, heightC, shift, mask);
            }
        }
    }

    if (!(firstPic->bitDepth == 8 && X265_DEPTH == 8))
    {
        yFirstPic = m_dupPicOne[0]; ySecPic = m_dupPicTwo[0];
        uFirstPic = m_dupPicOne[1]; uSecPic = m_dupPicTwo[1];
        vFirstPic = m_dupPicOne[2]; vSecPic = m_dupPicTwo[2];
    }

    //Compute SSD
    ssdY = computeSSD(yFirstPic, ySecPic, strideL, widthL, heightL, param);
    psnrY = (ssdY ? 10.0 * log10(refValueY / (double)ssdY) : 99.99);

    if (param->internalCsp != X265_CSP_I400)
    {
        ssdU = computeSSD(uFirstPic, uSecPic, strideC, widthC, heightC, param);
        ssdV = computeSSD(vFirstPic, vSecPic, strideC, widthC, heightC, param);
        psnrU = (ssdU ? 10.0 * log10(refValueC / (double)ssdU) : 99.99);
        psnrV = (ssdV ? 10.0 * log10(refValueC / (double)ssdV) : 99.99);
    }

    //Compute PSNR(picN,pic(N+1))
    return psnrWeight = (psnrY * 6 + psnrU + psnrV) / 8;
}

void Encoder::copyPicture(x265_picture *dest, const x265_picture *src)
{
    dest->poc = src->poc;
    dest->pts = src->pts;
    dest->userSEI = src->userSEI;
    dest->bitDepth = src->bitDepth;
    dest->framesize = src->framesize;
    dest->height = src->height;
    dest->width = src->width;
    dest->colorSpace = src->colorSpace;
    dest->userSEI = src->userSEI;
    dest->rpu.payload = src->rpu.payload;
    dest->picStruct = src->picStruct;
    dest->stride[0] = src->stride[0];
    dest->stride[1] = src->stride[1];
    dest->stride[2] = src->stride[2];
    memcpy(dest->planes[0], src->planes[0], src->framesize * sizeof(char));
    dest->planes[1] = (char*)dest->planes[0] + src->stride[0] * src->height;
    dest->planes[2] = (char*)dest->planes[1] + src->stride[1] * (src->height >> x265_cli_csps[src->colorSpace].height[1]);
}

bool Encoder::computeHistograms(x265_picture *pic)
{
    pixel *src = NULL, *planeV = NULL, *planeU = NULL;
    uint32_t widthC, heightC;
    int hshift, vshift;

    hshift = CHROMA_H_SHIFT(pic->colorSpace);
    vshift = CHROMA_V_SHIFT(pic->colorSpace);
    widthC = pic->width >> hshift;
    heightC = pic->height >> vshift;

    if (pic->bitDepth == X265_DEPTH)
    {
        src = (pixel*)pic->planes[0];
        if (m_param->internalCsp != X265_CSP_I400)
        {
            planeU = (pixel*)pic->planes[1];
            planeV = (pixel*)pic->planes[2];
        }
    }
    else if (pic->bitDepth == 8 && X265_DEPTH > 8)
    {
        int shift = (X265_DEPTH - 8);
        uint8_t *yChar, *uChar, *vChar;

        yChar = (uint8_t*)pic->planes[0];
        primitives.planecopy_cp(yChar, pic->stride[0] / sizeof(*yChar), m_inputPic[0], pic->stride[0] / sizeof(*yChar), pic->width, pic->height, shift);
        src = m_inputPic[0];
        if (m_param->internalCsp != X265_CSP_I400)
        {
            uChar = (uint8_t*)pic->planes[1];
            vChar = (uint8_t*)pic->planes[2];
            primitives.planecopy_cp(uChar, pic->stride[1] / sizeof(*uChar), m_inputPic[1], pic->stride[1] / sizeof(*uChar), widthC, heightC, shift);
            primitives.planecopy_cp(vChar, pic->stride[2] / sizeof(*vChar), m_inputPic[2], pic->stride[2] / sizeof(*vChar), widthC, heightC, shift);
            planeU = m_inputPic[1];
            planeV = m_inputPic[2];
        }
    }
    else
    {
        uint16_t *yShort, *uShort, *vShort;
        /* mask off bits that are supposed to be zero */
        uint16_t mask = (1 << X265_DEPTH) - 1;
        int shift = abs(pic->bitDepth - X265_DEPTH);

        yShort = (uint16_t*)pic->planes[0];
        uShort = (uint16_t*)pic->planes[1];
        vShort = (uint16_t*)pic->planes[2];

        if (pic->bitDepth > X265_DEPTH)
        {
            /* shift right and mask pixels to final size */
            primitives.planecopy_sp(yShort, pic->stride[0] / sizeof(*yShort), m_inputPic[0], pic->stride[0] / sizeof(*yShort), pic->width, pic->height, shift, mask);
            if (m_param->internalCsp != X265_CSP_I400)
            {
                primitives.planecopy_sp(uShort, pic->stride[1] / sizeof(*uShort), m_inputPic[1], pic->stride[1] / sizeof(*uShort), widthC, heightC, shift, mask);
                primitives.planecopy_sp(vShort, pic->stride[2] / sizeof(*vShort), m_inputPic[2], pic->stride[2] / sizeof(*vShort), widthC, heightC, shift, mask);
            }
        }
        else /* Case for (pic.bitDepth < X265_DEPTH) */
        {
            /* shift left and mask pixels to final size */
            primitives.planecopy_sp_shl(yShort, pic->stride[0] / sizeof(*yShort), m_inputPic[0], pic->stride[0] / sizeof(*yShort), pic->width, pic->height, shift, mask);
            if (m_param->internalCsp != X265_CSP_I400)
            {
                primitives.planecopy_sp_shl(uShort, pic->stride[1] / sizeof(*uShort), m_inputPic[1], pic->stride[1] / sizeof(*uShort), widthC, heightC, shift, mask);
                primitives.planecopy_sp_shl(vShort, pic->stride[2] / sizeof(*vShort), m_inputPic[2], pic->stride[2] / sizeof(*vShort), widthC, heightC, shift, mask);
            }
        }

        src = m_inputPic[0];
        planeU = m_inputPic[1];
        planeV = m_inputPic[2];
    }

    size_t bufSize = sizeof(pixel) * m_planeSizes[0];
    memset(m_edgePic, 0, bufSize);

    if (!computeEdge(m_edgePic, src, NULL, pic->width, pic->height, pic->width, false, 1))
    {
        x265_log(m_param, X265_LOG_ERROR, "Failed to compute edge!");
        return false;
    }

    pixel pixelVal;
    int32_t *edgeHist = m_curEdgeHist;
    memset(edgeHist, 0, EDGE_BINS * sizeof(int32_t));
    for (uint32_t i = 0; i < m_planeSizes[0]; i++)
    {
        if (m_edgePic[i])
            edgeHist[1]++;
        else
            edgeHist[0]++;
    }

    /* Y Histogram Calculation */
    int32_t *yHist = m_curYUVHist[0];
    memset(yHist, 0, HISTOGRAM_BINS * sizeof(int32_t));
    for (uint32_t i = 0; i < m_planeSizes[0]; i++)
    {
        pixelVal = src[i];
        yHist[pixelVal]++;
    }

    if (pic->colorSpace != X265_CSP_I400)
    {
        /* U Histogram Calculation */
        int32_t *uHist = m_curYUVHist[1];
        memset(uHist, 0, sizeof(m_curYUVHist[1]));
        for (uint32_t i = 0; i < m_planeSizes[1]; i++)
        {
            pixelVal = planeU[i];
            uHist[pixelVal]++;
        }

        /* V Histogram Calculation */
        pixelVal = 0;
        int32_t *vHist = m_curYUVHist[2];
        memset(vHist, 0, sizeof(m_curYUVHist[2]));
        for (uint32_t i = 0; i < m_planeSizes[2]; i++)
        {
            pixelVal = planeV[i];
            vHist[pixelVal]++;
        }
    }
    return true;
}

void Encoder::computeHistogramSAD(double *normalizedMaxUVSad, double *normalizedEdgeSad, int curPoc)
{

    if (curPoc == 0)
    {   /* first frame is scenecut by default no sad computation for the same. */
        *normalizedMaxUVSad = 0.0;
        *normalizedEdgeSad = 0.0;
    }
    else
    {
        /* compute sum of absolute differences of histogram bins of chroma and luma edge response between the current and prev pictures. */
        int32_t edgeHistSad = 0;
        int32_t uHistSad = 0;
        int32_t vHistSad = 0;
        double normalizedUSad = 0.0;
        double normalizedVSad = 0.0;

        for (int j = 0; j < HISTOGRAM_BINS; j++)
        {
            if (j < 2)
            {
                edgeHistSad += abs(m_curEdgeHist[j] - m_prevEdgeHist[j]);
            }
            uHistSad += abs(m_curYUVHist[1][j] - m_prevYUVHist[1][j]);
            vHistSad += abs(m_curYUVHist[2][j] - m_prevYUVHist[2][j]);
        }
        *normalizedEdgeSad = normalizeRange(edgeHistSad, 0, 2 * m_planeSizes[0], 0.0, 1.0);
        normalizedUSad = normalizeRange(uHistSad, 0, 2 * m_planeSizes[1], 0.0, 1.0);
        normalizedVSad = normalizeRange(vHistSad, 0, 2 * m_planeSizes[2], 0.0, 1.0);
        *normalizedMaxUVSad = x265_max(normalizedUSad, normalizedVSad);
    }

    /* store histograms of previous frame for reference */
    memcpy(m_prevEdgeHist, m_curEdgeHist, sizeof(m_curEdgeHist));
    memcpy(m_prevYUVHist, m_curYUVHist, sizeof(m_curYUVHist));
}

double Encoder::normalizeRange(int32_t value, int32_t minValue, int32_t maxValue, double rangeStart, double rangeEnd)
{
    return (double)(value - minValue) * (rangeEnd - rangeStart) / (maxValue - minValue) + rangeStart;
}

void Encoder::findSceneCuts(x265_picture *pic, bool& bDup, double maxUVSad, double edgeSad, bool& isMaxThres, bool& isHardSC)
{
    double minEdgeT = m_edgeHistThreshold * MIN_EDGE_FACTOR;
    double minChromaT = minEdgeT * SCENECUT_CHROMA_FACTOR;
    double maxEdgeT = m_edgeHistThreshold * MAX_EDGE_FACTOR;
    double maxChromaT = maxEdgeT * SCENECUT_CHROMA_FACTOR;
    pic->frameData.bScenecut = false;

    if (pic->poc == 0)
    {
        /* for first frame */
        pic->frameData.bScenecut = false;
        bDup = false;
    }
    else
    {
        if (edgeSad == 0.0 && maxUVSad == 0.0)
        {
            bDup = true;
        }
        else if (edgeSad < minEdgeT && maxUVSad < minChromaT)
        {
            pic->frameData.bScenecut = false;
        }
        else if (edgeSad > maxEdgeT && maxUVSad > maxChromaT)
        {
            pic->frameData.bScenecut = true;
            isMaxThres = true;
            isHardSC = true;
        }
        else if (edgeSad > m_scaledEdgeThreshold || maxUVSad >= m_scaledChromaThreshold
                 || (edgeSad > m_edgeHistThreshold && maxUVSad >= m_chromaHistThreshold))
        {
            pic->frameData.bScenecut = true;
            bDup = false;
            if (edgeSad > m_scaledEdgeThreshold || maxUVSad >= m_scaledChromaThreshold)
                isHardSC = true;
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

    const x265_picture* inputPic = NULL;
    static int written = 0, read = 0;
    bool dontRead = false;
    bool bdropFrame = false;
    bool dropflag = false;
    bool isMaxThres = false;
    bool isHardSC = false;

    if (m_exportedPic)
    {
        if (!m_param->bUseAnalysisFile && m_param->analysisSave)
            x265_free_analysis_data(m_param, &m_exportedPic->m_analysisData);
        ATOMIC_DEC(&m_exportedPic->m_countRefEncoders);
        m_exportedPic = NULL;
        m_dpb->recycleUnreferenced();
    }
    if ((pic_in && (!m_param->chunkEnd || (m_encodedFrameNum < m_param->chunkEnd))) || (m_param->bEnableFrameDuplication && !pic_in && (read < written)))
    {
        if (m_param->bHistBasedSceneCut && pic_in)
        {
            x265_picture *pic = (x265_picture *) pic_in;

            if (pic->poc == 0)
            {
                /* for entire encode compute the chroma plane sizes only once */
                for (int i = 1; i < x265_cli_csps[m_param->internalCsp].planes; i++)
                    m_planeSizes[i] = (pic->width >> x265_cli_csps[m_param->internalCsp].width[i]) * (pic->height >> x265_cli_csps[m_param->internalCsp].height[i]);
            }

            if (computeHistograms(pic))
            {
                double maxUVSad = 0.0, edgeSad = 0.0;
                computeHistogramSAD(&maxUVSad, &edgeSad, pic_in->poc);
                findSceneCuts(pic, bdropFrame, maxUVSad, edgeSad, isMaxThres, isHardSC);
            }
        }

        if ((m_param->bEnableFrameDuplication && !pic_in && (read < written)))
            dontRead = true;
        else
        {
            if (m_latestParam->forceFlush == 1)
            {
                m_lookahead->setLookaheadQueue();
                m_latestParam->forceFlush = 0;
            }
            if (m_latestParam->forceFlush == 2)
            {
                m_lookahead->m_filled = false;
                m_latestParam->forceFlush = 0;
            }

            if (pic_in->bitDepth < 8 || pic_in->bitDepth > 16)
            {
                x265_log(m_param, X265_LOG_ERROR, "Input bit depth (%d) must be between 8 and 16\n",
                         pic_in->bitDepth);
                return -1;
            }
        }

        if (m_param->bEnableFrameDuplication)
        {
            double psnrWeight = 0;

            if (!dontRead)
            {
                if (!m_dupBuffer[0]->bOccupied)
                {
                    copyPicture(m_dupBuffer[0]->dupPic, pic_in);
                    m_dupBuffer[0]->bOccupied = true;
                    written++;
                    return 0;
                }
                else if (!m_dupBuffer[1]->bOccupied)
                {
                    copyPicture(m_dupBuffer[1]->dupPic, pic_in);
                    m_dupBuffer[1]->bOccupied = true;
                    written++;
                }

                if (m_param->bEnableFrameDuplication && m_param->bHistBasedSceneCut)
                {
                    if (!bdropFrame && m_dupBuffer[1]->dupPic->frameData.bScenecut == false)
                    {
                        psnrWeight = ComputePSNR(m_dupBuffer[0]->dupPic, m_dupBuffer[1]->dupPic, m_param);
                        if (psnrWeight >= m_param->dupThreshold)
                            dropflag = true;
                    }
                    else
                    {
                        dropflag = true;
                    }
                }
                else if (m_param->bEnableFrameDuplication)
                {
                    psnrWeight = ComputePSNR(m_dupBuffer[0]->dupPic, m_dupBuffer[1]->dupPic, m_param);
                    if (psnrWeight >= m_param->dupThreshold)
                        dropflag = true;
                }

                if (dropflag)
                {
                    if (m_dupBuffer[0]->bDup)
                    {
                        m_dupBuffer[0]->dupPic->picStruct = tripling;
                        m_dupBuffer[0]->bDup = false;
                        read++;
                    }
                    else
                    {
                        m_dupBuffer[0]->dupPic->picStruct = doubling;
                        m_dupBuffer[0]->bDup = true;
                        m_dupBuffer[1]->bOccupied = false;
                        read++;
                        return 0;
                    }
                }
                else if (m_dupBuffer[0]->bDup)
                    m_dupBuffer[0]->bDup = false;
                else
                    m_dupBuffer[0]->dupPic->picStruct = 0;
            }

            if (read < written)
            {
                inputPic = m_dupBuffer[0]->dupPic;
                read++;
            }
        }
        else
            inputPic = pic_in;

        Frame *inFrame;
        x265_param *p = (m_reconfigure || m_reconfigureRc) ? m_latestParam : m_param;
        if (m_dpb->m_freeList.empty())
        {
            inFrame = new Frame;
            inFrame->m_encodeStartTime = x265_mdate();
            if (inFrame->create(p, inputPic->quantOffsets))
            {
                /* the first PicYuv created is asked to generate the CU and block unit offset
                 * arrays which are then shared with all subsequent PicYuv (orig and recon) 
                 * allocated by this top level encoder */
                if (m_sps.cuOffsetY)
                {
                    inFrame->m_fencPic->m_cuOffsetY = m_sps.cuOffsetY;
                    inFrame->m_fencPic->m_buOffsetY = m_sps.buOffsetY;
                    if (m_param->internalCsp != X265_CSP_I400)
                    {
                        inFrame->m_fencPic->m_cuOffsetC = m_sps.cuOffsetC;
                        inFrame->m_fencPic->m_buOffsetC = m_sps.buOffsetC;
                    }
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
                        m_sps.cuOffsetY = inFrame->m_fencPic->m_cuOffsetY;
                        m_sps.buOffsetY = inFrame->m_fencPic->m_buOffsetY;
                        if (m_param->internalCsp != X265_CSP_I400)
                        {
                            m_sps.cuOffsetC = inFrame->m_fencPic->m_cuOffsetC;
                            m_sps.cuOffsetY = inFrame->m_fencPic->m_cuOffsetY;
                            m_sps.buOffsetC = inFrame->m_fencPic->m_buOffsetC;
                            m_sps.buOffsetY = inFrame->m_fencPic->m_buOffsetY;
                        }
                    }
                }
                if (m_param->recursionSkipMode == EDGE_BASED_RSKIP && m_param->bHistBasedSceneCut)
                {
                    pixel* src = m_edgePic;
                    primitives.planecopy_pp_shr(src, inFrame->m_fencPic->m_picWidth, inFrame->m_edgeBitPic, inFrame->m_fencPic->m_stride,
                        inFrame->m_fencPic->m_picWidth, inFrame->m_fencPic->m_picHeight, 0);
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
            inFrame->m_encodeStartTime = x265_mdate();
            /* Set lowres scencut and satdCost here to aovid overwriting ANALYSIS_READ
               decision by lowres init*/
            inFrame->m_lowres.bScenecut = false;
            inFrame->m_lowres.satdCost = (int64_t)-1;
            inFrame->m_lowresInit = false;
            inFrame->m_isInsideWindow = 0;
        }

        /* Copy input picture into a Frame and PicYuv, send to lookahead */
        inFrame->m_fencPic->copyFromPicture(*inputPic, *m_param, m_sps.conformanceWindow.rightOffset, m_sps.conformanceWindow.bottomOffset);

        inFrame->m_poc       = ++m_pocLast;
        inFrame->m_userData  = inputPic->userData;
        inFrame->m_pts       = inputPic->pts;
        if (m_param->bHistBasedSceneCut)
        {
            inFrame->m_lowres.bScenecut = (inputPic->frameData.bScenecut == 1) ? true : false;
            inFrame->m_lowres.m_bIsMaxThres = isMaxThres;
            if (m_param->radl && m_param->keyframeMax != m_param->keyframeMin)
                inFrame->m_lowres.m_bIsHardScenecut = isHardSC;
        }

        if (m_param->bEnableSceneCutAwareQp && m_param->rc.bStatRead)
        {
            RateControlEntry * rcEntry = NULL;
            rcEntry = &(m_rateControl->m_rce2Pass[inFrame->m_poc]);
            if(rcEntry->scenecut)
            {
                int backwardWindow = X265_MIN(int((p->fpsNum / p->fpsDenom) / 10), p->lookaheadDepth);
                for (int i = 1; i <= backwardWindow; i++)
                {
                    int frameNum = inFrame->m_poc - i;
                    Frame * frame = m_lookahead->m_inputQueue.getPOC(frameNum);
                    if (frame)
                        frame->m_isInsideWindow = BACKWARD_WINDOW;
                }
            }
        }
        if (m_param->bHistBasedSceneCut && m_param->analysisSave)
        {
            memcpy(inFrame->m_analysisData.edgeHist, m_curEdgeHist, EDGE_BINS * sizeof(int32_t));
            memcpy(inFrame->m_analysisData.yuvHist[0], m_curYUVHist[0], HISTOGRAM_BINS *sizeof(int32_t));
            if (inputPic->colorSpace != X265_CSP_I400)
            {
                memcpy(inFrame->m_analysisData.yuvHist[1], m_curYUVHist[1], HISTOGRAM_BINS * sizeof(int32_t));
                memcpy(inFrame->m_analysisData.yuvHist[2], m_curYUVHist[2], HISTOGRAM_BINS * sizeof(int32_t));
            }
        }
        inFrame->m_forceqp   = inputPic->forceqp;
        inFrame->m_param     = (m_reconfigure || m_reconfigureRc) ? m_latestParam : m_param;
        inFrame->m_picStruct = inputPic->picStruct;
        if (m_param->bField && m_param->interlaceMode)
            inFrame->m_fieldNum = inputPic->fieldNum;

        copyUserSEIMessages(inFrame, inputPic);

        /*Copy Dolby Vision RPU from inputPic to frame*/
        if (inputPic->rpu.payloadSize)
        {
            inFrame->m_rpu.payloadSize = inputPic->rpu.payloadSize;
            inFrame->m_rpu.payload = new uint8_t[inputPic->rpu.payloadSize];
            memcpy(inFrame->m_rpu.payload, inputPic->rpu.payload, inputPic->rpu.payloadSize);
        }

        if (inputPic->quantOffsets != NULL)
        {
            int cuCount;
            if (m_param->rc.qgSize == 8)
                cuCount = inFrame->m_lowres.maxBlocksInRowFullRes * inFrame->m_lowres.maxBlocksInColFullRes;
            else
                cuCount = inFrame->m_lowres.maxBlocksInRow * inFrame->m_lowres.maxBlocksInCol;
            memcpy(inFrame->m_quantOffsets, inputPic->quantOffsets, cuCount * sizeof(float));
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
        int sliceType = (m_param->rc.bStatRead) ? m_rateControl->rateControlSliceType(inFrame->m_poc) : inputPic->sliceType;

        /* In analysisSave mode, x265_analysis_data is allocated in inputPic and inFrame points to this */
        /* Load analysis data before lookahead->addPicture, since sliceType has been decided */
        if (m_param->analysisLoad)
        {
            /* reads analysis data for the frame and allocates memory based on slicetype */
            static int paramBytes = CONF_OFFSET_BYTES;
            if (!inFrame->m_poc && m_param->bAnalysisType != HEVC_INFO)
            {
                x265_analysis_validate saveParam = inputPic->analysisData.saveParam;
                paramBytes += validateAnalysisData(&saveParam, 0);
                if (paramBytes == -1)
                {
                    m_aborted = true;
                    return -1;
                }
            }
            if (m_saveCTUSize)
            {
                cuLocation cuLocInFrame;
                cuLocInFrame.init(m_param);
                /* Set skipWidth/skipHeight flags when the out of bound pixels in lowRes is greater than half of maxCUSize */
                int extendedWidth = ((m_param->sourceWidth / 2 + m_param->maxCUSize - 1) >> m_param->maxLog2CUSize) * m_param->maxCUSize;
                int extendedHeight = ((m_param->sourceHeight / 2 + m_param->maxCUSize - 1) >> m_param->maxLog2CUSize) * m_param->maxCUSize;
                uint32_t outOfBoundaryLowres = extendedWidth - m_param->sourceWidth / 2;
                if (outOfBoundaryLowres * 2 >= m_param->maxCUSize)
                    cuLocInFrame.skipWidth = true;
                uint32_t outOfBoundaryLowresH = extendedHeight - m_param->sourceHeight / 2;
                if (outOfBoundaryLowresH * 2 >= m_param->maxCUSize)
                    cuLocInFrame.skipHeight = true;
                readAnalysisFile(&inFrame->m_analysisData, inFrame->m_poc, inputPic, paramBytes, cuLocInFrame);
            }
            else
                readAnalysisFile(&inFrame->m_analysisData, inFrame->m_poc, inputPic, paramBytes);
            inFrame->m_poc = inFrame->m_analysisData.poc;
            sliceType = inFrame->m_analysisData.sliceType;
            inFrame->m_lowres.bScenecut = !!inFrame->m_analysisData.bScenecut;
            inFrame->m_lowres.satdCost = inFrame->m_analysisData.satdCost;
            if (m_param->bDisableLookahead)
            {
                inFrame->m_lowres.sliceType = sliceType;
                inFrame->m_lowres.bKeyframe = !!inFrame->m_analysisData.lookahead.keyframe;
                inFrame->m_lowres.bLastMiniGopBFrame = !!inFrame->m_analysisData.lookahead.lastMiniGopBFrame;
                if (m_rateControl->m_isVbv)
                {
                    int vbvCount = m_param->lookaheadDepth + m_param->bframes + 2;
                    for (int index = 0; index < vbvCount; index++)
                    {
                        inFrame->m_lowres.plannedSatd[index] = inFrame->m_analysisData.lookahead.plannedSatd[index];
                        inFrame->m_lowres.plannedType[index] = inFrame->m_analysisData.lookahead.plannedType[index];
                    }
                }
            }
        }
        if (m_param->bUseRcStats && inputPic->rcData)
        {
            RcStats* rc = (RcStats*)inputPic->rcData;
            m_rateControl->m_accumPQp = rc->cumulativePQp;
            m_rateControl->m_accumPNorm = rc->cumulativePNorm;
            m_rateControl->m_isNextGop = true;
            for (int j = 0; j < 3; j++)
                m_rateControl->m_lastQScaleFor[j] = rc->lastQScaleFor[j];
            m_rateControl->m_wantedBitsWindow = rc->wantedBitsWindow;
            m_rateControl->m_cplxrSum = rc->cplxrSum;
            m_rateControl->m_totalBits = rc->totalBits;
            m_rateControl->m_encodedBits = rc->encodedBits;
            m_rateControl->m_shortTermCplxSum = rc->shortTermCplxSum;
            m_rateControl->m_shortTermCplxCount = rc->shortTermCplxCount;
            if (m_rateControl->m_isVbv)
            {
                m_rateControl->m_bufferFillFinal = rc->bufferFillFinal;
                for (int i = 0; i < 4; i++)
                {
                    m_rateControl->m_pred[i].coeff = rc->coeff[i];
                    m_rateControl->m_pred[i].count = rc->count[i];
                    m_rateControl->m_pred[i].offset = rc->offset[i];
                }
            }
            m_param->bUseRcStats = 0;
        }

        if (m_param->bEnableFrameDuplication && ((read < written) || (m_dupBuffer[0]->dupPic->picStruct == tripling && (read <= written))))
        {
            if (m_dupBuffer[0]->dupPic->picStruct == tripling)
                m_dupBuffer[0]->bOccupied = m_dupBuffer[1]->bOccupied = false;
            else
            {
                copyPicture(m_dupBuffer[0]->dupPic, m_dupBuffer[1]->dupPic);
                m_dupBuffer[1]->bOccupied = false;
            }
        }

        if (m_reconfigureRc)
            inFrame->m_reconfigureRc = true;

        m_lookahead->addPicture(*inFrame, sliceType);
        m_numDelayedPic++;
    }
    else if (m_latestParam->forceFlush == 2)
        m_lookahead->m_filled = true;
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

            /* Free up inputPic->analysisData since it has already been used */
            if ((m_param->analysisLoad && !m_param->analysisSave) || ((m_param->bAnalysisType == AVC_INFO) && slice->m_sliceType != I_SLICE))
                x265_free_analysis_data(m_param, &outFrame->m_analysisData);

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
                pic_out->reorderedPts = outFrame->m_reorderedPts;
                pic_out->sliceType = outFrame->m_lowres.sliceType;
                pic_out->planes[0] = recpic->m_picOrg[0];
                pic_out->stride[0] = (int)(recpic->m_stride * sizeof(pixel));
                if (m_param->internalCsp != X265_CSP_I400)
                {
                    pic_out->planes[1] = recpic->m_picOrg[1];
                    pic_out->stride[1] = (int)(recpic->m_strideC * sizeof(pixel));
                    pic_out->planes[2] = recpic->m_picOrg[2];
                    pic_out->stride[2] = (int)(recpic->m_strideC * sizeof(pixel));
                }

                /* Dump analysis data from pic_out to file in save mode and free */
                if (m_param->analysisSave)
                {
                    pic_out->analysisData.poc = pic_out->poc;
                    pic_out->analysisData.sliceType = pic_out->sliceType;
                    pic_out->analysisData.bScenecut = outFrame->m_lowres.bScenecut;
                    if (m_param->bHistBasedSceneCut)
                    {
                        memcpy(pic_out->analysisData.edgeHist, outFrame->m_analysisData.edgeHist, EDGE_BINS * sizeof(int32_t));
                        memcpy(pic_out->analysisData.yuvHist[0], outFrame->m_analysisData.yuvHist[0], HISTOGRAM_BINS * sizeof(int32_t));
                        if (pic_out->colorSpace != X265_CSP_I400)
                        {
                            memcpy(pic_out->analysisData.yuvHist[1], outFrame->m_analysisData.yuvHist[1], HISTOGRAM_BINS * sizeof(int32_t));
                            memcpy(pic_out->analysisData.yuvHist[2], outFrame->m_analysisData.yuvHist[2], HISTOGRAM_BINS * sizeof(int32_t));
                        }
                    }
                    pic_out->analysisData.satdCost  = outFrame->m_lowres.satdCost;
                    pic_out->analysisData.numCUsInFrame = outFrame->m_analysisData.numCUsInFrame;
                    pic_out->analysisData.numPartitions = outFrame->m_analysisData.numPartitions;
                    pic_out->analysisData.wt = outFrame->m_analysisData.wt;
                    pic_out->analysisData.interData = outFrame->m_analysisData.interData;
                    pic_out->analysisData.intraData = outFrame->m_analysisData.intraData;
                    pic_out->analysisData.distortionData = outFrame->m_analysisData.distortionData;
                    pic_out->analysisData.modeFlag[0] = outFrame->m_analysisData.modeFlag[0];
                    pic_out->analysisData.modeFlag[1] = outFrame->m_analysisData.modeFlag[1];
                    if (m_param->bDisableLookahead)
                    {
                        int factor = 1;
                        if (m_param->scaleFactor)
                            factor = m_param->scaleFactor * 2;
                        pic_out->analysisData.numCuInHeight = outFrame->m_analysisData.numCuInHeight;
                        pic_out->analysisData.lookahead.dts = outFrame->m_dts;
                        pic_out->analysisData.lookahead.reorderedPts = outFrame->m_reorderedPts;
                        pic_out->analysisData.satdCost *= factor;
                        pic_out->analysisData.lookahead.keyframe = outFrame->m_lowres.bKeyframe;
                        pic_out->analysisData.lookahead.lastMiniGopBFrame = outFrame->m_lowres.bLastMiniGopBFrame;
                        if (m_rateControl->m_isVbv)
                        {
                            int vbvCount = m_param->lookaheadDepth + m_param->bframes + 2;
                            for (int index = 0; index < vbvCount; index++)
                            {
                                pic_out->analysisData.lookahead.plannedSatd[index] = outFrame->m_lowres.plannedSatd[index];
                                pic_out->analysisData.lookahead.plannedType[index] = outFrame->m_lowres.plannedType[index];
                            }
                            for (uint32_t index = 0; index < pic_out->analysisData.numCuInHeight; index++)
                            {
                                outFrame->m_analysisData.lookahead.intraSatdForVbv[index] = outFrame->m_encData->m_rowStat[index].intraSatdForVbv;
                                outFrame->m_analysisData.lookahead.satdForVbv[index] = outFrame->m_encData->m_rowStat[index].satdForVbv;
                            }
                            pic_out->analysisData.lookahead.intraSatdForVbv = outFrame->m_analysisData.lookahead.intraSatdForVbv;
                            pic_out->analysisData.lookahead.satdForVbv = outFrame->m_analysisData.lookahead.satdForVbv;
                            for (uint32_t index = 0; index < pic_out->analysisData.numCUsInFrame; index++)
                            {
                                outFrame->m_analysisData.lookahead.intraVbvCost[index] = outFrame->m_encData->m_cuStat[index].intraVbvCost;
                                outFrame->m_analysisData.lookahead.vbvCost[index] = outFrame->m_encData->m_cuStat[index].vbvCost;
                            }
                            pic_out->analysisData.lookahead.intraVbvCost = outFrame->m_analysisData.lookahead.intraVbvCost;
                            pic_out->analysisData.lookahead.vbvCost = outFrame->m_analysisData.lookahead.vbvCost;
                        }
                    }
                    writeAnalysisFile(&pic_out->analysisData, *outFrame->m_encData);
                    pic_out->analysisData.saveParam = pic_out->analysisData.saveParam;
                    if (m_param->bUseAnalysisFile)
                        x265_free_analysis_data(m_param, &pic_out->analysisData);
                }
            }
            if (m_param->rc.bStatWrite && (m_param->analysisMultiPassRefine || m_param->analysisMultiPassDistortion))
            {
                if (pic_out)
                {
                    pic_out->analysisData.poc = pic_out->poc;
                    pic_out->analysisData.interData = outFrame->m_analysisData.interData;
                    pic_out->analysisData.intraData = outFrame->m_analysisData.intraData;
                    pic_out->analysisData.distortionData = outFrame->m_analysisData.distortionData;
                }
                writeAnalysisFileRefine(&outFrame->m_analysisData, *outFrame->m_encData);
            }
            if (m_param->analysisMultiPassRefine || m_param->analysisMultiPassDistortion)
                x265_free_analysis_data(m_param, &outFrame->m_analysisData);
            if (m_param->internalCsp == X265_CSP_I400)
            {
                if (slice->m_sliceType == P_SLICE)
                {
                    if (slice->m_weightPredTable[0][0][0].wtPresent)
                        m_numLumaWPFrames++;
                }
                else if (slice->m_sliceType == B_SLICE)
                {
                    bool bLuma = false;
                    for (int l = 0; l < 2; l++)
                    {
                        if (slice->m_weightPredTable[l][0][0].wtPresent)
                            bLuma = true;
                    }
                    if (bLuma)
                        m_numLumaWPBiFrames++;
                }
            }
            else
            {
                if (slice->m_sliceType == P_SLICE)
                {
                    if (slice->m_weightPredTable[0][0][0].wtPresent)
                        m_numLumaWPFrames++;
                    if (slice->m_weightPredTable[0][0][1].wtPresent ||
                        slice->m_weightPredTable[0][0][2].wtPresent)
                        m_numChromaWPFrames++;
                }
                else if (slice->m_sliceType == B_SLICE)
                {
                    bool bLuma = false, bChroma = false;
                    for (int l = 0; l < 2; l++)
                    {
                        if (slice->m_weightPredTable[l][0][0].wtPresent)
                            bLuma = true;
                        if (slice->m_weightPredTable[l][0][1].wtPresent ||
                            slice->m_weightPredTable[l][0][2].wtPresent)
                            bChroma = true;
                    }

                    if (bLuma)
                        m_numLumaWPBiFrames++;
                    if (bChroma)
                        m_numChromaWPBiFrames++;
                }
            }
            if (m_aborted)
                return -1;

            if ((m_outputCount + 1)  >= m_param->chunkStart)
                finishFrameStats(outFrame, curEncoder, frameData, m_pocLast);
            if (m_param->analysisSave)
            {
                pic_out->analysisData.frameBits = frameData->bits;
                if (!slice->isIntra())
                {
                    for (int ref = 0; ref < MAX_NUM_REF; ref++)
                        pic_out->analysisData.list0POC[ref] = frameData->list0POC[ref];

                    double totalIntraPercent = 0;

                    for (uint32_t depth = 0; depth < m_param->maxCUDepth; depth++)
                        for (uint32_t intramode = 0; intramode < 3; intramode++)
                            totalIntraPercent += frameData->cuStats.percentIntraDistribution[depth][intramode];
                    totalIntraPercent += frameData->cuStats.percentIntraNxN;

                    for (uint32_t depth = 0; depth < m_param->maxCUDepth; depth++)
                        totalIntraPercent += frameData->puStats.percentIntraPu[depth];
                    pic_out->analysisData.totalIntraPercent = totalIntraPercent;

                    if (!slice->isInterP())
                    {
                        for (int ref = 0; ref < MAX_NUM_REF; ref++)
                            pic_out->analysisData.list1POC[ref] = frameData->list1POC[ref];
                    }
                }
            }

            /* Write RateControl Frame level stats in multipass encodes */
            if (m_param->rc.bStatWrite)
                if (m_rateControl->writeRateControlFrameStats(outFrame, &curEncoder->m_rce))
                    m_aborted = true;
            if (pic_out)
            { 
                /* m_rcData is allocated for every frame */
                pic_out->rcData = outFrame->m_rcData;
                outFrame->m_rcData->qpaRc = outFrame->m_encData->m_avgQpRc;
                outFrame->m_rcData->qRceq = curEncoder->m_rce.qRceq;
                outFrame->m_rcData->qpNoVbv = curEncoder->m_rce.qpNoVbv;
                outFrame->m_rcData->coeffBits = outFrame->m_encData->m_frameStats.coeffBits;
                outFrame->m_rcData->miscBits = outFrame->m_encData->m_frameStats.miscBits;
                outFrame->m_rcData->mvBits = outFrame->m_encData->m_frameStats.mvBits;
                outFrame->m_rcData->qScale = outFrame->m_rcData->newQScale = x265_qp2qScale(outFrame->m_encData->m_avgQpRc);
                outFrame->m_rcData->poc = curEncoder->m_rce.poc;
                outFrame->m_rcData->encodeOrder = curEncoder->m_rce.encodeOrder;
                outFrame->m_rcData->sliceType = curEncoder->m_rce.sliceType;
                outFrame->m_rcData->keptAsRef = curEncoder->m_rce.sliceType == B_SLICE && !IS_REFERENCED(outFrame) ? 0 : 1;
                outFrame->m_rcData->qpAq = outFrame->m_encData->m_avgQpAq;
                outFrame->m_rcData->iCuCount = outFrame->m_encData->m_frameStats.percent8x8Intra * m_rateControl->m_ncu;
                outFrame->m_rcData->pCuCount = outFrame->m_encData->m_frameStats.percent8x8Inter * m_rateControl->m_ncu;
                outFrame->m_rcData->skipCuCount = outFrame->m_encData->m_frameStats.percent8x8Skip  * m_rateControl->m_ncu;
            }

            /* Allow this frame to be recycled if no frame encoders are using it for reference */
            if (!pic_out)
            {
                ATOMIC_DEC(&outFrame->m_countRefEncoders);
                m_dpb->recycleUnreferenced();
            }
            else
                m_exportedPic = outFrame;
            
            m_outputCount++;
            if (m_param->chunkEnd == m_outputCount)
                m_numDelayedPic = 0;
            else 
                m_numDelayedPic--;

            ret = 1;
        }

        /* pop a single frame from decided list, then provide to frame encoder
         * curEncoder is guaranteed to be idle at this point */
        if (!pass)
            frameEnc = m_lookahead->getDecidedPicture();
        if (frameEnc && !pass && (!m_param->chunkEnd || (m_encodedFrameNum < m_param->chunkEnd)))
        {
            if (m_param->bEnableSceneCutAwareQp && m_param->rc.bStatRead)
            {
                RateControlEntry * rcEntry;
                rcEntry = &(m_rateControl->m_rce2Pass[frameEnc->m_poc]);

                if (rcEntry->scenecut)
                {
                    if (m_rateControl->m_lastScenecut == -1)
                        m_rateControl->m_lastScenecut = frameEnc->m_poc;
                    else
                    {
                        int maxWindowSize = int((m_param->scenecutWindow / 1000.0) * (m_param->fpsNum / m_param->fpsDenom) + 0.5);
                        if (frameEnc->m_poc > (m_rateControl->m_lastScenecut + maxWindowSize))
                            m_rateControl->m_lastScenecut = frameEnc->m_poc;
                    }
                }
            }

            if (m_param->analysisMultiPassRefine || m_param->analysisMultiPassDistortion)
            {
                uint32_t widthInCU = (m_param->sourceWidth + m_param->maxCUSize - 1) >> m_param->maxLog2CUSize;
                uint32_t heightInCU = (m_param->sourceHeight + m_param->maxCUSize - 1) >> m_param->maxLog2CUSize;
                frameEnc->m_analysisData.numCUsInFrame = widthInCU * heightInCU;
                frameEnc->m_analysisData.numPartitions = m_param->num4x4Partitions;
                x265_alloc_analysis_data(m_param, &frameEnc->m_analysisData);
                frameEnc->m_analysisData.poc = frameEnc->m_poc;
                if (m_param->rc.bStatRead)
                    readAnalysisFile(&frameEnc->m_analysisData, frameEnc->m_poc, frameEnc->m_lowres.sliceType);
            }

            if (m_param->bResetZoneConfig)
            {
                for (int i = 0; i < m_param->rc.zonefileCount; i++)
                {
                    if (m_param->rc.zones[i].startFrame == frameEnc->m_poc)
                        x265_encoder_reconfig(this, m_param->rc.zones[i].zoneParam);
                }
            }

            if (frameEnc->m_reconfigureRc && m_reconfigureRc)
            {
                x265_copy_params(m_param, m_latestParam);
                m_rateControl->reconfigureRC();
                m_reconfigureRc = false;
            }
            if (frameEnc->m_reconfigureRc && !m_reconfigureRc)
                frameEnc->m_reconfigureRc = false;
            if (curEncoder->m_reconfigure)
            {
                /* One round robin cycle of FE reconfigure is complete */
                /* Safe to copy m_latestParam to Encoder::m_param, encoder reconfigure complete */
                for (int frameEncId = 0; frameEncId < m_param->frameNumThreads; frameEncId++)
                    m_frameEncoder[frameEncId]->m_reconfigure = false;
                x265_copy_params(m_param, m_latestParam);
                m_reconfigure = false;
            }

            /* Initiate reconfigure for this FE if necessary */
            curEncoder->m_param = m_reconfigure ? m_latestParam : m_param;
            curEncoder->m_reconfigure = m_reconfigure;

            /* give this frame a FrameData instance before encoding */
            if (m_dpb->m_frameDataFreeList)
            {
                frameEnc->m_encData = m_dpb->m_frameDataFreeList;
                m_dpb->m_frameDataFreeList = m_dpb->m_frameDataFreeList->m_freeListNext;
                frameEnc->reinit(m_sps);
                frameEnc->m_param = m_reconfigure ? m_latestParam : m_param;
                frameEnc->m_encData->m_param = m_reconfigure ? m_latestParam : m_param;
            }
            else
            {
                frameEnc->allocEncodeData(m_reconfigure ? m_latestParam : m_param, m_sps);
                Slice* slice = frameEnc->m_encData->m_slice;
                slice->m_sps = &m_sps;
                slice->m_pps = &m_pps;
                slice->m_param = m_param;
                slice->m_maxNumMergeCand = m_param->maxNumMergeCand;
                slice->m_endCUAddr = slice->realEndAddress(m_sps.numCUsInFrame * m_param->num4x4Partitions);
            }
            if (m_param->analysisLoad && m_param->bDisableLookahead)
            {
                frameEnc->m_dts = frameEnc->m_analysisData.lookahead.dts;
                frameEnc->m_reorderedPts = frameEnc->m_analysisData.lookahead.reorderedPts;
                if (m_rateControl->m_isVbv)
                {
                    for (uint32_t index = 0; index < frameEnc->m_analysisData.numCuInHeight; index++)
                    {
                        frameEnc->m_encData->m_rowStat[index].intraSatdForVbv = frameEnc->m_analysisData.lookahead.intraSatdForVbv[index];
                        frameEnc->m_encData->m_rowStat[index].satdForVbv = frameEnc->m_analysisData.lookahead.satdForVbv[index];
                    }
                    for (uint32_t index = 0; index < frameEnc->m_analysisData.numCUsInFrame; index++)
                    {
                        frameEnc->m_encData->m_cuStat[index].intraVbvCost = frameEnc->m_analysisData.lookahead.intraVbvCost[index];
                        frameEnc->m_encData->m_cuStat[index].vbvCost = frameEnc->m_analysisData.lookahead.vbvCost[index];
                    }
                }
            }
            if (m_param->searchMethod == X265_SEA && frameEnc->m_lowres.sliceType != X265_TYPE_B)
            {
                int padX = m_param->maxCUSize + 32;
                int padY = m_param->maxCUSize + 16;
                uint32_t numCuInHeight = (frameEnc->m_encData->m_reconPic->m_picHeight + m_param->maxCUSize - 1) / m_param->maxCUSize;
                int maxHeight = numCuInHeight * m_param->maxCUSize;
                for (int i = 0; i < INTEGRAL_PLANE_NUM; i++)
                {
                    frameEnc->m_encData->m_meBuffer[i] = X265_MALLOC(uint32_t, frameEnc->m_reconPic->m_stride * (maxHeight + (2 * padY)));
                    if (frameEnc->m_encData->m_meBuffer[i])
                    {
                        memset(frameEnc->m_encData->m_meBuffer[i], 0, sizeof(uint32_t)* frameEnc->m_reconPic->m_stride * (maxHeight + (2 * padY)));
                        frameEnc->m_encData->m_meIntegral[i] = frameEnc->m_encData->m_meBuffer[i] + frameEnc->m_encData->m_reconPic->m_stride * padY + padX;
                    }
                    else
                        x265_log(m_param, X265_LOG_ERROR, "SEA motion search: POC %d Integral buffer[%d] unallocated\n", frameEnc->m_poc, i);
                }
            }

            if (m_param->bOptQpPPS && frameEnc->m_lowres.bKeyframe && m_param->bRepeatHeaders)
            {
                ScopedLock qpLock(m_sliceQpLock);
                if (m_iFrameNum > 0)
                {
                    //Search the least cost
                    int64_t iLeastCost = m_iBitsCostSum[0];
                    int iLeastId = 0;
                    for (int i = 1; i < QP_MAX_MAX + 1; i++)
                    {
                        if (iLeastCost > m_iBitsCostSum[i])
                        {
                            iLeastId = i;
                            iLeastCost = m_iBitsCostSum[i];
                        }
                    }
                    /* If last slice Qp is close to (26 + m_iPPSQpMinus26) or outputs is all I-frame video,
                       we don't need to change m_iPPSQpMinus26. */
                    if (m_iFrameNum > 1)
                        m_iPPSQpMinus26 = (iLeastId + 1) - 26;
                    m_iFrameNum = 0;
                }

                for (int i = 0; i < QP_MAX_MAX + 1; i++)
                    m_iBitsCostSum[i] = 0;
            }

            frameEnc->m_encData->m_slice->m_iPPSQpMinus26 = m_iPPSQpMinus26;
            frameEnc->m_encData->m_slice->numRefIdxDefault[0] = m_pps.numRefIdxDefault[0];
            frameEnc->m_encData->m_slice->numRefIdxDefault[1] = m_pps.numRefIdxDefault[1];
            frameEnc->m_encData->m_slice->m_iNumRPSInSPS = m_sps.spsrpsNum;

            curEncoder->m_rce.encodeOrder = frameEnc->m_encodeOrder = m_encodedFrameNum++;

            if (!m_param->analysisLoad || !m_param->bDisableLookahead)
            {
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
            }

            /* Allocate analysis data before encode in save mode. This is allocated in frameEnc */
            if (m_param->analysisSave && !m_param->analysisLoad)
            {
                x265_analysis_data* analysis = &frameEnc->m_analysisData;
                analysis->poc = frameEnc->m_poc;
                analysis->sliceType = frameEnc->m_lowres.sliceType;
                uint32_t widthInCU       = (m_param->sourceWidth  + m_param->maxCUSize - 1) >> m_param->maxLog2CUSize;
                uint32_t heightInCU      = (m_param->sourceHeight + m_param->maxCUSize - 1) >> m_param->maxLog2CUSize;

                uint32_t numCUsInFrame   = widthInCU * heightInCU;
                analysis->numCUsInFrame  = numCUsInFrame;
                analysis->numCuInHeight = heightInCU;
                analysis->numPartitions  = m_param->num4x4Partitions;
                x265_alloc_analysis_data(m_param, analysis);
            }
            /* determine references, setup RPS, etc */
            m_dpb->prepareEncode(frameEnc);
            if (!!m_param->selectiveSAO)
            {
                Slice* slice = frameEnc->m_encData->m_slice;
                slice->m_bUseSao = curEncoder->m_frameFilter.m_useSao = 1;
                switch (m_param->selectiveSAO)
                {
                case 3: if (!IS_REFERENCED(frameEnc))
                            slice->m_bUseSao = curEncoder->m_frameFilter.m_useSao = 0;
                        break;
                case 2: if (!!m_param->bframes && slice->m_sliceType == B_SLICE)
                            slice->m_bUseSao = curEncoder->m_frameFilter.m_useSao = 0;
                        break;
                case 1: if (slice->m_sliceType != I_SLICE)
                            slice->m_bUseSao = curEncoder->m_frameFilter.m_useSao = 0;
                        break;
                }
            }
            else
            {
                Slice* slice = frameEnc->m_encData->m_slice;
                slice->m_bUseSao = curEncoder->m_frameFilter.m_useSao = 0;
            }

            if (m_param->rc.rateControlMode != X265_RC_CQP)
                m_lookahead->getEstimatedPictureCost(frameEnc);
            if (m_param->bIntraRefresh)
                 calcRefreshInterval(frameEnc);

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
    if (isReconfigureRc(encParam, param) && !param->rc.zonefileCount)
    {
        /* VBV can't be turned ON if it wasn't ON to begin with and can't be turned OFF if it was ON to begin with*/
        if (param->rc.vbvMaxBitrate > 0 && param->rc.vbvBufferSize > 0 &&
            encParam->rc.vbvMaxBitrate > 0 && encParam->rc.vbvBufferSize > 0)
        {
            m_reconfigureRc |= encParam->rc.vbvMaxBitrate != param->rc.vbvMaxBitrate;
            m_reconfigureRc |= encParam->rc.vbvBufferSize != param->rc.vbvBufferSize;
            if (m_reconfigureRc && m_param->bEmitHRDSEI)
                x265_log(m_param, X265_LOG_WARNING, "VBV parameters cannot be changed when HRD is in use.\n");
            else
            {
                encParam->rc.vbvMaxBitrate = param->rc.vbvMaxBitrate;
                encParam->rc.vbvBufferSize = param->rc.vbvBufferSize;
            }
        }
        m_reconfigureRc |= encParam->rc.bitrate != param->rc.bitrate;
        encParam->rc.bitrate = param->rc.bitrate;
        m_reconfigureRc |= encParam->rc.rfConstant != param->rc.rfConstant;
        encParam->rc.rfConstant = param->rc.rfConstant;
    }
    else
    {
        encParam->maxNumReferences = param->maxNumReferences; // never uses more refs than specified in stream headers
        encParam->bEnableFastIntra = param->bEnableFastIntra;
        encParam->bEnableEarlySkip = param->bEnableEarlySkip;
        encParam->recursionSkipMode = param->recursionSkipMode;
        encParam->searchMethod = param->searchMethod;
        /* Scratch buffer prevents me_range from being increased for esa/tesa */
        if (param->searchRange < encParam->searchRange)
            encParam->searchRange = param->searchRange;
        /* We can't switch out of subme=0 during encoding. */
        if (encParam->subpelRefine)
            encParam->subpelRefine = param->subpelRefine;
        encParam->rdoqLevel = param->rdoqLevel;
        encParam->rdLevel = param->rdLevel;
        encParam->bEnableRectInter = param->bEnableRectInter;
        encParam->maxNumMergeCand = param->maxNumMergeCand;
        encParam->bIntraInBFrames = param->bIntraInBFrames;
        if (param->scalingLists && !encParam->scalingLists)
            encParam->scalingLists = strdup(param->scalingLists);

        encParam->rc.aqMode = param->rc.aqMode;
        encParam->rc.aqStrength = param->rc.aqStrength;
        encParam->noiseReductionInter = param->noiseReductionInter;
        encParam->noiseReductionIntra = param->noiseReductionIntra;

        encParam->limitModes = param->limitModes;
        encParam->bEnableSplitRdSkip = param->bEnableSplitRdSkip;
        encParam->bCULossless = param->bCULossless;
        encParam->bEnableRdRefine = param->bEnableRdRefine;
        encParam->limitTU = param->limitTU;
        encParam->bEnableTSkipFast = param->bEnableTSkipFast;
        encParam->rdPenalty = param->rdPenalty;
        encParam->dynamicRd = param->dynamicRd;
        encParam->bEnableTransformSkip = param->bEnableTransformSkip;
        encParam->bEnableAMP = param->bEnableAMP;

        /* Resignal changes in params in Parameter Sets */
        m_sps.maxAMPDepth = (m_sps.bUseAMP = param->bEnableAMP && param->bEnableAMP) ? param->maxCUDepth : 0;
        m_pps.bTransformSkipEnabled = param->bEnableTransformSkip ? 1 : 0;

    }
    encParam->forceFlush = param->forceFlush;
    /* To add: Loop Filter/deblocking controls, transform skip, signhide require PPS to be resent */
    /* To add: SAO, temporal MVP, AMP, TU depths require SPS to be resent, at every CVS boundary */
    return x265_check_params(encParam);
}

bool Encoder::isReconfigureRc(x265_param* latestParam, x265_param* param_in)
{
    return (latestParam->rc.vbvMaxBitrate != param_in->rc.vbvMaxBitrate
        || latestParam->rc.vbvBufferSize != param_in->rc.vbvBufferSize
        || latestParam->rc.bitrate != param_in->rc.bitrate
        || latestParam->rc.rfConstant != param_in->rc.rfConstant);
}

void Encoder::copyCtuInfo(x265_ctu_info_t** frameCtuInfo, int poc)
{
    uint32_t widthInCU = (m_param->sourceWidth + m_param->maxCUSize - 1) >> m_param->maxLog2CUSize;
    uint32_t heightInCU = (m_param->sourceHeight + m_param->maxCUSize - 1) >> m_param->maxLog2CUSize;
    Frame* curFrame;
    Frame* prevFrame = NULL;
    int32_t* frameCTU;
    uint32_t numCUsInFrame = widthInCU * heightInCU;
    uint32_t maxNum8x8Partitions = 64;
    bool copied = false;
    do
    {
        curFrame = m_lookahead->m_inputQueue.getPOC(poc);
        if (!curFrame)
            curFrame = m_lookahead->m_outputQueue.getPOC(poc);

        if (poc > 0)
        {
            prevFrame = m_lookahead->m_inputQueue.getPOC(poc - 1);
            if (!prevFrame)
                prevFrame = m_lookahead->m_outputQueue.getPOC(poc - 1);
            if (!prevFrame)
            {
                FrameEncoder* prevEncoder;
                for (int i = 0; i < m_param->frameNumThreads; i++)
                {
                    prevEncoder = m_frameEncoder[i];
                    prevFrame = prevEncoder->m_frame;
                    if (prevFrame && (prevEncoder->m_frame->m_poc == poc - 1))
                    {
                        prevFrame = prevEncoder->m_frame;
                        break;
                    }
                }
            }
        }
        x265_ctu_info_t* ctuTemp, *prevCtuTemp;
        if (curFrame)
        {
            if (!curFrame->m_ctuInfo)
                CHECKED_MALLOC(curFrame->m_ctuInfo, x265_ctu_info_t*, 1);
            CHECKED_MALLOC(*curFrame->m_ctuInfo, x265_ctu_info_t, numCUsInFrame);
            CHECKED_MALLOC_ZERO(curFrame->m_prevCtuInfoChange, int, numCUsInFrame * maxNum8x8Partitions);
            for (uint32_t i = 0; i < numCUsInFrame; i++)
            {
                ctuTemp = *curFrame->m_ctuInfo + i;
                CHECKED_MALLOC(frameCTU, int32_t, maxNum8x8Partitions);
                ctuTemp->ctuInfo = (int32_t*)frameCTU;
                ctuTemp->ctuAddress = frameCtuInfo[i]->ctuAddress;
                memcpy(ctuTemp->ctuPartitions, frameCtuInfo[i]->ctuPartitions, sizeof(int32_t) * maxNum8x8Partitions);
                memcpy(ctuTemp->ctuInfo, frameCtuInfo[i]->ctuInfo, sizeof(int32_t) * maxNum8x8Partitions);
                if (prevFrame && curFrame->m_poc > 1)
                {
                    prevCtuTemp = *prevFrame->m_ctuInfo + i;
                    for (uint32_t j = 0; j < maxNum8x8Partitions; j++)
                        curFrame->m_prevCtuInfoChange[i * maxNum8x8Partitions + j] = (*((int32_t *)prevCtuTemp->ctuInfo + j) == 2) ? (poc - 1) : prevFrame->m_prevCtuInfoChange[i * maxNum8x8Partitions + j];
                }
            }
            copied = true;
            curFrame->m_copied.trigger();
        }
        else
        {
            FrameEncoder* curEncoder;
            for (int i = 0; i < m_param->frameNumThreads; i++)
            {
                curEncoder = m_frameEncoder[i];
                curFrame = curEncoder->m_frame;
                if (curFrame)
                {
                    if (poc == curFrame->m_poc)
                    {
                        if (!curFrame->m_ctuInfo)
                            CHECKED_MALLOC(curFrame->m_ctuInfo, x265_ctu_info_t*, 1);
                        CHECKED_MALLOC(*curFrame->m_ctuInfo, x265_ctu_info_t, numCUsInFrame);
                        CHECKED_MALLOC_ZERO(curFrame->m_prevCtuInfoChange, int, numCUsInFrame * maxNum8x8Partitions);
                        for (uint32_t l = 0; l < numCUsInFrame; l++)
                        {
                            ctuTemp = *curFrame->m_ctuInfo + l;
                            CHECKED_MALLOC(frameCTU, int32_t, maxNum8x8Partitions);
                            ctuTemp->ctuInfo = (int32_t*)frameCTU;
                            ctuTemp->ctuAddress = frameCtuInfo[l]->ctuAddress;
                            memcpy(ctuTemp->ctuPartitions, frameCtuInfo[l]->ctuPartitions, sizeof(int32_t) * maxNum8x8Partitions);
                            memcpy(ctuTemp->ctuInfo, frameCtuInfo[l]->ctuInfo, sizeof(int32_t) * maxNum8x8Partitions);
                            if (prevFrame && curFrame->m_poc > 1)
                            {
                                prevCtuTemp = *prevFrame->m_ctuInfo + l;
                                for (uint32_t j = 0; j < maxNum8x8Partitions; j++)
                                    curFrame->m_prevCtuInfoChange[l * maxNum8x8Partitions + j] = (*((int32_t *)prevCtuTemp->ctuInfo + j) == CTU_INFO_CHANGE) ? (poc - 1) : prevFrame->m_prevCtuInfoChange[l * maxNum8x8Partitions + j];
                            }
                        }
                        copied = true;
                        curFrame->m_copied.trigger();
                        break;
                    }
                }
            }
        }
    } while (!copied);
    return;
fail:
    for (uint32_t i = 0; i < numCUsInFrame; i++)
    {
        X265_FREE((*curFrame->m_ctuInfo + i)->ctuInfo);
        (*curFrame->m_ctuInfo + i)->ctuInfo = NULL;
    }
    X265_FREE(*curFrame->m_ctuInfo);
    *(curFrame->m_ctuInfo) = NULL;
    X265_FREE(curFrame->m_ctuInfo);
    curFrame->m_ctuInfo = NULL;
    X265_FREE(curFrame->m_prevCtuInfoChange);
    curFrame->m_prevCtuInfoChange = NULL;
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
    if (m_param->bMultiPassOptRPS && m_param->rc.bStatRead)
    {
        x265_log(m_param, X265_LOG_INFO, "RPS in SPS: %d frames (%.2f%%), RPS not in SPS: %d frames (%.2f%%)\n", 
            m_rpsInSpsCount, (float)100.0 * m_rpsInSpsCount / m_rateControl->m_numEntries, 
            m_rateControl->m_numEntries - m_rpsInSpsCount, 
            (float)100.0 * (m_rateControl->m_numEntries - m_rpsInSpsCount) / m_rateControl->m_numEntries);
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
        cuStats.accumulate(m_frameEncoder[i]->m_cuStats, *m_param);

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
    for (uint32_t i = 0; i <= m_param->maxCUDepth; i++)
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
             cuStats.totalCTUs, m_param->maxCUSize, m_param->maxCUSize,
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
        if (m_param->csvLogLevel >= 2 || m_param->maxCLL || m_param->maxFALL)
        {
            stats->maxCLL = m_analyzeAll.m_maxCLL;
            stats->maxFALL = (uint16_t)(m_analyzeAll.m_maxFALL / m_analyzeAll.m_numPics);
        }
    }
    /* If new statistics are added to x265_stats, we must check here whether the
     * structure provided by the user is the new structure or an older one (for
     * future safety) */
}

void Encoder::finishFrameStats(Frame* curFrame, FrameEncoder *curEncoder, x265_frame_stats* frameStats, int inPoc)
{
    PicYuv* reconPic = curFrame->m_reconPic;
    uint64_t bits = curEncoder->m_accessUnitBits;

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
    if (m_param->csvLogLevel >= 2 || m_param->maxCLL || m_param->maxFALL)
    {
        m_analyzeAll.m_maxFALL += curFrame->m_fencPic->m_avgLumaLevel;
        m_analyzeAll.m_maxCLL = X265_MAX(m_analyzeAll.m_maxCLL, curFrame->m_fencPic->m_maxLumaLevel);
    }
    char c = (slice->isIntra() ? (curFrame->m_lowres.sliceType == X265_TYPE_IDR ? 'I' : 'i') : slice->isInterP() ? 'P' : 'B');
    int poc = slice->m_poc;
    if (!IS_REFERENCED(curFrame))
        c += 32; // lower case if unreferenced

    if (frameStats)
    {
        const int picOrderCntLSB = slice->m_poc - slice->m_lastIDR;

        frameStats->encoderOrder = m_outputCount;
        frameStats->sliceType = c;
        frameStats->poc = picOrderCntLSB;
        frameStats->qp = curEncData.m_avgQpAq;
        frameStats->bits = bits;
        frameStats->bScenecut = curFrame->m_lowres.bScenecut;
        if (m_param->csvLogLevel >= 2)
            frameStats->ipCostRatio = curFrame->m_lowres.ipCostRatio;
        frameStats->bufferFill = m_rateControl->m_bufferFillActual;
        frameStats->bufferFillFinal = m_rateControl->m_bufferFillFinal;
        if (m_param->csvLogLevel >= 2)
            frameStats->unclippedBufferFillFinal = m_rateControl->m_unclippedBufferFillFinal;
        frameStats->frameLatency = inPoc - poc;
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
            for (int ref = 0; ref < MAX_NUM_REF; ref++)
                frameStats->list0POC[ref] = ref < slice->m_numRefIdx[0] ? slice->m_refPOCList[0][ref] - slice->m_lastIDR : -1;

            if (!slice->isInterP())
            {
                for (int ref = 0; ref < MAX_NUM_REF; ref++)
                    frameStats->list1POC[ref] = ref < slice->m_numRefIdx[1] ? slice->m_refPOCList[1][ref] - slice->m_lastIDR : -1;
            }
        }
#define ELAPSED_MSEC(start, end) (((double)(end) - (start)) / 1000)
        if (m_param->csvLogLevel >= 2)
        {
#if ENABLE_LIBVMAF
            frameStats->vmafFrameScore = curFrame->m_fencPic->m_vmafScore;
#endif
            frameStats->decideWaitTime = ELAPSED_MSEC(0, curEncoder->m_slicetypeWaitTime);
            frameStats->row0WaitTime = ELAPSED_MSEC(curEncoder->m_startCompressTime, curEncoder->m_row0WaitTime);
            frameStats->wallTime = ELAPSED_MSEC(curEncoder->m_row0WaitTime, curEncoder->m_endCompressTime);
            frameStats->refWaitWallTime = ELAPSED_MSEC(curEncoder->m_row0WaitTime, curEncoder->m_allRowsAvailableTime);
            frameStats->totalCTUTime = ELAPSED_MSEC(0, curEncoder->m_totalWorkerElapsedTime);
            frameStats->stallTime = ELAPSED_MSEC(0, curEncoder->m_totalNoWorkerTime);
            frameStats->totalFrameTime = ELAPSED_MSEC(curFrame->m_encodeStartTime, x265_mdate());
            if (curEncoder->m_totalActiveWorkerCount)
                frameStats->avgWPP = (double)curEncoder->m_totalActiveWorkerCount / curEncoder->m_activeWorkerCountSamples;
            else
                frameStats->avgWPP = 1;
            frameStats->countRowBlocks = curEncoder->m_countRowBlocks;

            frameStats->avgChromaDistortion = curFrame->m_encData->m_frameStats.avgChromaDistortion;
            frameStats->avgLumaDistortion = curFrame->m_encData->m_frameStats.avgLumaDistortion;
            frameStats->avgPsyEnergy = curFrame->m_encData->m_frameStats.avgPsyEnergy;
            frameStats->avgResEnergy = curFrame->m_encData->m_frameStats.avgResEnergy;
            frameStats->maxLumaLevel = curFrame->m_fencPic->m_maxLumaLevel;
            frameStats->minLumaLevel = curFrame->m_fencPic->m_minLumaLevel;
            frameStats->avgLumaLevel = curFrame->m_fencPic->m_avgLumaLevel;

            frameStats->maxChromaULevel = curFrame->m_fencPic->m_maxChromaULevel;
            frameStats->minChromaULevel = curFrame->m_fencPic->m_minChromaULevel;
            frameStats->avgChromaULevel = curFrame->m_fencPic->m_avgChromaULevel;

            frameStats->maxChromaVLevel = curFrame->m_fencPic->m_maxChromaVLevel;
            frameStats->minChromaVLevel = curFrame->m_fencPic->m_minChromaVLevel;
            frameStats->avgChromaVLevel = curFrame->m_fencPic->m_avgChromaVLevel;

            if (curFrame->m_encData->m_frameStats.totalPu[4] == 0)
                frameStats->puStats.percentNxN = 0;
            else
                frameStats->puStats.percentNxN = (double)(curFrame->m_encData->m_frameStats.cnt4x4 / (double)curFrame->m_encData->m_frameStats.totalPu[4]) * 100;
            for (uint32_t depth = 0; depth <= m_param->maxCUDepth; depth++)
            {
                if (curFrame->m_encData->m_frameStats.totalPu[depth] == 0)
                {
                    frameStats->puStats.percentSkipPu[depth] = 0;
                    frameStats->puStats.percentIntraPu[depth] = 0;
                    frameStats->puStats.percentAmpPu[depth] = 0;
                    for (int i = 0; i < INTER_MODES - 1; i++)
                    {
                        frameStats->puStats.percentInterPu[depth][i] = 0;
                        frameStats->puStats.percentMergePu[depth][i] = 0;
                    }
                }
                else
                {
                    frameStats->puStats.percentSkipPu[depth] = (double)(curFrame->m_encData->m_frameStats.cntSkipPu[depth] / (double)curFrame->m_encData->m_frameStats.totalPu[depth]) * 100;
                    frameStats->puStats.percentIntraPu[depth] = (double)(curFrame->m_encData->m_frameStats.cntIntraPu[depth] / (double)curFrame->m_encData->m_frameStats.totalPu[depth]) * 100;
                    frameStats->puStats.percentAmpPu[depth] = (double)(curFrame->m_encData->m_frameStats.cntAmp[depth] / (double)curFrame->m_encData->m_frameStats.totalPu[depth]) * 100;
                    for (int i = 0; i < INTER_MODES - 1; i++)
                    {
                        frameStats->puStats.percentInterPu[depth][i] = (double)(curFrame->m_encData->m_frameStats.cntInterPu[depth][i] / (double)curFrame->m_encData->m_frameStats.totalPu[depth]) * 100;
                        frameStats->puStats.percentMergePu[depth][i] = (double)(curFrame->m_encData->m_frameStats.cntMergePu[depth][i] / (double)curFrame->m_encData->m_frameStats.totalPu[depth]) * 100;
                    }
                }
            }
        }

        if (m_param->csvLogLevel >= 1)
        {
            frameStats->cuStats.percentIntraNxN = curFrame->m_encData->m_frameStats.percentIntraNxN;

            for (uint32_t depth = 0; depth <= m_param->maxCUDepth; depth++)
            {
                frameStats->cuStats.percentSkipCu[depth] = curFrame->m_encData->m_frameStats.percentSkipCu[depth];
                frameStats->cuStats.percentMergeCu[depth] = curFrame->m_encData->m_frameStats.percentMergeCu[depth];
                frameStats->cuStats.percentInterDistribution[depth][0] = curFrame->m_encData->m_frameStats.percentInterDistribution[depth][0];
                frameStats->cuStats.percentInterDistribution[depth][1] = curFrame->m_encData->m_frameStats.percentInterDistribution[depth][1];
                frameStats->cuStats.percentInterDistribution[depth][2] = curFrame->m_encData->m_frameStats.percentInterDistribution[depth][2];
                for (int n = 0; n < INTRA_MODES; n++)
                    frameStats->cuStats.percentIntraDistribution[depth][n] = curFrame->m_encData->m_frameStats.percentIntraDistribution[depth][n];
            }
        }
    }
}

#if defined(_MSC_VER)
#pragma warning(disable: 4800) // forcing int to bool
#pragma warning(disable: 4127) // conditional expression is constant
#endif

void Encoder::initRefIdx()
{
    int j = 0;

    for (j = 0; j < MAX_NUM_REF_IDX; j++)
    {
        m_refIdxLastGOP.numRefIdxl0[j] = 0;
        m_refIdxLastGOP.numRefIdxl1[j] = 0;
    }

    return;
}

void Encoder::analyseRefIdx(int *numRefIdx)
{
    int i_l0 = 0;
    int i_l1 = 0;

    i_l0 = numRefIdx[0];
    i_l1 = numRefIdx[1];

    if ((0 < i_l0) && (MAX_NUM_REF_IDX > i_l0))
        m_refIdxLastGOP.numRefIdxl0[i_l0]++;
    if ((0 < i_l1) && (MAX_NUM_REF_IDX > i_l1))
        m_refIdxLastGOP.numRefIdxl1[i_l1]++;

    return;
}

void Encoder::updateRefIdx()
{
    int i_max_l0 = 0;
    int i_max_l1 = 0;
    int j = 0;

    i_max_l0 = 0;
    i_max_l1 = 0;
    m_refIdxLastGOP.numRefIdxDefault[0] = 1;
    m_refIdxLastGOP.numRefIdxDefault[1] = 1;
    for (j = 0; j < MAX_NUM_REF_IDX; j++)
    {
        if (i_max_l0 < m_refIdxLastGOP.numRefIdxl0[j])
        {
            i_max_l0 = m_refIdxLastGOP.numRefIdxl0[j];
            m_refIdxLastGOP.numRefIdxDefault[0] = j;
        }
        if (i_max_l1 < m_refIdxLastGOP.numRefIdxl1[j])
        {
            i_max_l1 = m_refIdxLastGOP.numRefIdxl1[j];
            m_refIdxLastGOP.numRefIdxDefault[1] = j;
        }
    }

    m_pps.numRefIdxDefault[0] = m_refIdxLastGOP.numRefIdxDefault[0];
    m_pps.numRefIdxDefault[1] = m_refIdxLastGOP.numRefIdxDefault[1];
    initRefIdx();

    return;
}

void Encoder::getStreamHeaders(NALList& list, Entropy& sbacCoder, Bitstream& bs)
{
    sbacCoder.setBitstream(&bs);

    if (m_param->dolbyProfile && !m_param->bRepeatHeaders)
    {
        bs.resetBits();
        bs.write(0x10, 8);
        list.serialize(NAL_UNIT_ACCESS_UNIT_DELIMITER, bs);
    }
    
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
    sbacCoder.codePPS(m_pps, (m_param->maxSlices <= 1), m_iPPSQpMinus26);
    bs.writeByteAlignment();
    list.serialize(NAL_UNIT_PPS, bs);

    if (m_param->bSingleSeiNal)
        bs.resetBits();

    if (m_param->bEmitHDR10SEI)
    {
        if (m_param->bEmitCLL)
        {
            SEIContentLightLevel cllsei;
            cllsei.max_content_light_level = m_param->maxCLL;
            cllsei.max_pic_average_light_level = m_param->maxFALL;
            cllsei.writeSEImessages(bs, m_sps, NAL_UNIT_PREFIX_SEI, list, m_param->bSingleSeiNal);
        }

        if (m_param->masteringDisplayColorVolume)
        {
            SEIMasteringDisplayColorVolume mdsei;
            if (mdsei.parse(m_param->masteringDisplayColorVolume))
                mdsei.writeSEImessages(bs, m_sps, NAL_UNIT_PREFIX_SEI, list, m_param->bSingleSeiNal);
            else
                x265_log(m_param, X265_LOG_WARNING, "unable to parse mastering display color volume info\n");
        }
    }

    if (m_param->bEmitInfoSEI)
    {
        char *opts = x265_param2string(m_param, m_sps.conformanceWindow.rightOffset, m_sps.conformanceWindow.bottomOffset);
        if (opts)
        {
            char *buffer = X265_MALLOC(char, strlen(opts) + strlen(PFX(version_str)) +
                strlen(PFX(build_info_str)) + 200);
            if (buffer)
            {
                sprintf(buffer, "x265 (build %d) - %s:%s - H.265/HEVC codec - "
                    "Copyright 2013-2018 (c) Multicoreware, Inc - "
                    "http://x265.org - options: %s",
                    X265_BUILD, PFX(version_str), PFX(build_info_str), opts);

                SEIuserDataUnregistered idsei;
                idsei.m_userData = (uint8_t*)buffer;
                idsei.setSize((uint32_t)strlen(buffer));
                idsei.writeSEImessages(bs, m_sps, NAL_UNIT_PREFIX_SEI, list, m_param->bSingleSeiNal);

                X265_FREE(buffer);
            }

            X265_FREE(opts);
        }
    }

    if ((m_param->bEmitHRDSEI || !!m_param->interlaceMode))
    {
        /* Picture Timing and Buffering Period SEI require the SPS to be "activated" */
        SEIActiveParameterSets sei;
        sei.m_selfContainedCvsFlag = true;
        sei.m_noParamSetUpdateFlag = true;
        sei.writeSEImessages(bs, m_sps, NAL_UNIT_PREFIX_SEI, list, m_param->bSingleSeiNal);
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
    sps->numCuInWidth = (m_param->sourceWidth + m_param->maxCUSize - 1) / m_param->maxCUSize;
    sps->numCuInHeight = (m_param->sourceHeight + m_param->maxCUSize - 1) / m_param->maxCUSize;
    sps->numCUsInFrame = sps->numCuInWidth * sps->numCuInHeight;
    sps->numPartitions = m_param->num4x4Partitions;
    sps->numPartInCUSize = 1 << m_param->unitSizeDepth;

    sps->log2MinCodingBlockSize = m_param->maxLog2CUSize - m_param->maxCUDepth;
    sps->log2DiffMaxMinCodingBlockSize = m_param->maxCUDepth;
    uint32_t maxLog2TUSize = (uint32_t)g_log2Size[m_param->maxTUSize];
    sps->quadtreeTULog2MaxSize = X265_MIN((uint32_t)m_param->maxLog2CUSize, maxLog2TUSize);
    sps->quadtreeTULog2MinSize = 2;
    sps->quadtreeTUMaxDepthInter = m_param->tuQTMaxInterDepth;
    sps->quadtreeTUMaxDepthIntra = m_param->tuQTMaxIntraDepth;

    sps->bUseSAO = m_param->bEnableSAO;

    sps->bUseAMP = m_param->bEnableAMP;
    sps->maxAMPDepth = m_param->bEnableAMP ? m_param->maxCUDepth : 0;

    sps->maxTempSubLayers = m_param->bEnableTemporalSubLayers ? 2 : 1;
    sps->maxDecPicBuffering = m_vps.maxDecPicBuffering;
    sps->numReorderPics = m_vps.numReorderPics;
    sps->maxLatencyIncrease = m_vps.maxLatencyIncrease = m_param->bframes;

    sps->bUseStrongIntraSmoothing = m_param->bEnableStrongIntraSmoothing;
    sps->bTemporalMVPEnabled = m_param->bEnableTemporalMvp;
    sps->bEmitVUITimingInfo = m_param->bEmitVUITimingInfo;
    sps->bEmitVUIHRDInfo = m_param->bEmitVUIHRDInfo;
    sps->log2MaxPocLsb = m_param->log2MaxPocLsb;
    int maxDeltaPOC = (m_param->bframes + 2) * (!!m_param->bBPyramid + 1) * 2;
    while ((1 << sps->log2MaxPocLsb) <= maxDeltaPOC * 2)
        sps->log2MaxPocLsb++;

    if (sps->log2MaxPocLsb != m_param->log2MaxPocLsb)
        x265_log(m_param, X265_LOG_WARNING, "Reset log2MaxPocLsb to %d to account for all POC values\n", sps->log2MaxPocLsb);

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

    vui.frameFieldInfoPresentFlag = !!m_param->interlaceMode || (m_param->pictureStructure >= 0);
    vui.fieldSeqFlag = !!m_param->interlaceMode;

    vui.hrdParametersPresentFlag = m_param->bEmitHRDSEI;

    vui.timingInfo.numUnitsInTick = m_param->fpsDenom;
    vui.timingInfo.timeScale = m_param->fpsNum;
}

void Encoder::initPPS(PPS *pps)
{
    bool bIsVbv = m_param->rc.vbvBufferSize > 0 && m_param->rc.vbvMaxBitrate > 0;
    bool bEnableDistOffset = m_param->analysisMultiPassDistortion && m_param->rc.bStatRead;

    if (!m_param->bLossless && (m_param->rc.aqMode || bIsVbv || m_param->bAQMotion))
    {
        pps->bUseDQP = true;
        pps->maxCuDQPDepth = g_log2Size[m_param->maxCUSize] - g_log2Size[m_param->rc.qgSize];
        X265_CHECK(pps->maxCuDQPDepth <= 3, "max CU DQP depth cannot be greater than 3\n");
    }
    else if (!m_param->bLossless && bEnableDistOffset)
    {
        pps->bUseDQP = true;
        pps->maxCuDQPDepth = 0;
    }
    else
    {
        pps->bUseDQP = false;
        pps->maxCuDQPDepth = 0;
    }

    pps->chromaQpOffset[0] = m_param->cbQpOffset;
    pps->chromaQpOffset[1] = m_param->crQpOffset;
    pps->pps_slice_chroma_qp_offsets_present_flag = m_param->bHDR10Opt;

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

    pps->numRefIdxDefault[0] = 1;
    pps->numRefIdxDefault[1] = 1;
}

void Encoder::configureZone(x265_param *p, x265_param *zone)
{
    if (m_param->bResetZoneConfig)
    {
        p->maxNumReferences = zone->maxNumReferences;
        p->bEnableFastIntra = zone->bEnableFastIntra;
        p->bEnableEarlySkip = zone->bEnableEarlySkip;
        p->recursionSkipMode = zone->recursionSkipMode;
        p->searchMethod = zone->searchMethod;
        p->searchRange = zone->searchRange;
        p->subpelRefine = zone->subpelRefine;
        p->rdoqLevel = zone->rdoqLevel;
        p->rdLevel = zone->rdLevel;
        p->bEnableRectInter = zone->bEnableRectInter;
        p->maxNumMergeCand = zone->maxNumMergeCand;
        p->bIntraInBFrames = zone->bIntraInBFrames;
        if (zone->scalingLists)
            p->scalingLists = strdup(zone->scalingLists);

        p->rc.aqMode = zone->rc.aqMode;
        p->rc.aqStrength = zone->rc.aqStrength;
        p->noiseReductionInter = zone->noiseReductionInter;
        p->noiseReductionIntra = zone->noiseReductionIntra;

        p->limitModes = zone->limitModes;
        p->bEnableSplitRdSkip = zone->bEnableSplitRdSkip;
        p->bCULossless = zone->bCULossless;
        p->bEnableRdRefine = zone->bEnableRdRefine;
        p->limitTU = zone->limitTU;
        p->bEnableTSkipFast = zone->bEnableTSkipFast;
        p->rdPenalty = zone->rdPenalty;
        p->dynamicRd = zone->dynamicRd;
        p->bEnableTransformSkip = zone->bEnableTransformSkip;
        p->bEnableAMP = zone->bEnableAMP;

        if (m_param->rc.rateControlMode == X265_RC_ABR)
            p->rc.bitrate = zone->rc.bitrate;
        if (m_param->rc.rateControlMode == X265_RC_CRF)
            p->rc.rfConstant = zone->rc.rfConstant;
        if (m_param->rc.rateControlMode == X265_RC_CQP)
        {
            p->rc.qp = zone->rc.qp;
            p->rc.aqMode = X265_AQ_NONE;
            p->rc.hevcAq = 0;
        }
        p->radl = zone->radl;
    }
    memcpy(zone, p, sizeof(x265_param));
}

void Encoder::configureDolbyVisionParams(x265_param* p)
{
    uint32_t doviProfile = 0;

    while (dovi[doviProfile].doviProfileId != p->dolbyProfile && doviProfile + 1 < sizeof(dovi) / sizeof(dovi[0]))
        doviProfile++;

    p->bEmitHRDSEI = dovi[doviProfile].bEmitHRDSEI;
    p->vui.bEnableVideoSignalTypePresentFlag = dovi[doviProfile].bEnableVideoSignalTypePresentFlag;
    p->vui.bEnableColorDescriptionPresentFlag = dovi[doviProfile].bEnableColorDescriptionPresentFlag;
    p->bEnableAccessUnitDelimiters = dovi[doviProfile].bEnableAccessUnitDelimiters;
    p->bAnnexB = dovi[doviProfile].bAnnexB;
    p->vui.videoFormat = dovi[doviProfile].videoFormat;
    p->vui.bEnableVideoFullRangeFlag = dovi[doviProfile].bEnableVideoFullRangeFlag;
    p->vui.transferCharacteristics = dovi[doviProfile].transferCharacteristics;
    p->vui.colorPrimaries = dovi[doviProfile].colorPrimaries;
    p->vui.matrixCoeffs = dovi[doviProfile].matrixCoeffs;

    if (dovi[doviProfile].doviProfileId == 81)
        p->bEmitHDR10SEI = p->bEmitCLL = 1;

    if (dovi[doviProfile].doviProfileId == 50)
        p->crQpOffset = 3;
}

void Encoder::configure(x265_param *p)
{
    this->m_param = p;
    if (p->bAnalysisType == AVC_INFO)
        this->m_externalFlush = true;
    else 
        this->m_externalFlush = false;

    if (p->bAnalysisType == AVC_INFO && (p->limitTU == 3 || p->limitTU == 4))
    {
        x265_log(p, X265_LOG_WARNING, "limit TU = 3 or 4 with MVType AVCINFO produces inconsistent output\n");
    }

    if (p->bAnalysisType == AVC_INFO && p->minCUSize != 8)
    {
        p->minCUSize = 8;
        x265_log(p, X265_LOG_WARNING, "Setting minCuSize = 8, AVCINFO expects 8x8 blocks\n");
    }

    if (p->keyframeMax < 0)
    {
        /* A negative max GOP size indicates the user wants only one I frame at
         * the start of the stream. Set an infinite GOP distance and disable
         * adaptive I frame placement */
        p->keyframeMax = INT_MAX;
        p->scenecutThreshold = 0;
        p->bHistBasedSceneCut = 0;
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
        p->bHistBasedSceneCut = 0;
        p->bFrameAdaptive = 0;
        p->rc.cuTree = 0;
        p->bEnableWeightedPred = 0;
        p->bEnableWeightedBiPred = 0;
        p->bIntraRefresh = 0;

        /* SPSs shall have sps_max_dec_pic_buffering_minus1[ sps_max_sub_layers_minus1 ] equal to 0 only */
        p->maxNumReferences = 1;
    }
    if (!p->keyframeMin)
    {
        double fps = (double)p->fpsNum / p->fpsDenom;
        p->keyframeMin = X265_MIN((int)fps, p->keyframeMax / 10);
    }
    p->keyframeMin = X265_MAX(1, p->keyframeMin);

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
        if (!p->cbQpOffset && !p->crQpOffset)
        {
            p->cbQpOffset = MAX_CHROMA_QP_OFFSET / 2;
            p->crQpOffset = MAX_CHROMA_QP_OFFSET / 2;
            x265_log(p, X265_LOG_WARNING, "halving the quality when psy-rd is enabled for 444 input."
                     " Setting cbQpOffset = %d and crQpOffset = %d\n", p->cbQpOffset, p->crQpOffset);
        }
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
        p->rc.hevcAq = 0;
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
    {
        p->rc.aqMode = X265_AQ_NONE;
        p->rc.hevcAq = 0;
    }
    if (p->rc.aqMode == X265_AQ_NONE && p->rc.cuTree == 0)
        p->rc.aqStrength = 0;
    if (p->rc.hevcAq && p->rc.aqMode)
    {
        x265_log(p, X265_LOG_WARNING, "hevc-aq enabled, disabling other aq-modes\n");
    }

    if (p->totalFrames && p->totalFrames <= 2 * ((float)p->fpsNum) / p->fpsDenom && p->rc.bStrictCbr)
        p->lookaheadDepth = p->totalFrames;
    if (p->bIntraRefresh)
    {
        int numCuInWidth = (m_param->sourceWidth + m_param->maxCUSize - 1) / m_param->maxCUSize;
        if (p->maxNumReferences > 1)
        {
            x265_log(p,  X265_LOG_WARNING, "Max References > 1 + intra-refresh is not supported , setting max num references = 1\n");
            p->maxNumReferences = 1;
        }

        if (p->bBPyramid && p->bframes)
            x265_log(p,  X265_LOG_WARNING, "B pyramid cannot be enabled when max references is 1, Disabling B pyramid\n");
        p->bBPyramid = 0;


        if (p->bOpenGOP)
        {
            x265_log(p,  X265_LOG_WARNING, "Open Gop disabled, Intra Refresh is not compatible with openGop\n");
            p->bOpenGOP = 0;
        }

        x265_log(p,  X265_LOG_WARNING, "Scenecut is disabled when Intra Refresh is enabled\n");

        if (((float)numCuInWidth - 1) / m_param->keyframeMax > 1)
            x265_log(p,  X265_LOG_WARNING, "Keyint value is very low.It leads to frequent intra refreshes, can be almost every frame."
                     "Prefered use case would be high keyint value or an API call to refresh when necessary\n");

    }

    if (p->selectiveSAO && !p->bEnableSAO)
    {
        p->bEnableSAO = 1;
        x265_log(p, X265_LOG_WARNING, "SAO turned ON when selective-sao is ON\n");
    }

    if (!p->selectiveSAO && p->bEnableSAO)
        p->selectiveSAO = 4;

    if (p->interlaceMode)
        x265_log(p, X265_LOG_WARNING, "Support for interlaced video is experimental\n");

    if (p->rc.rfConstantMin > p->rc.rfConstant)
    {
        x265_log(m_param, X265_LOG_WARNING, "CRF min must be less than CRF\n");
        p->rc.rfConstantMin = 0;
    }

    if (p->analysisSaveReuseLevel && !p->analysisSave)
    {
        x265_log(p, X265_LOG_WARNING, "analysis-save-reuse-level can be set only when analysis-save is enabled."
            " Resetting analysis-save-reuse-level to 0.\n");
        p->analysisSaveReuseLevel = 0;
    }

    if (p->analysisLoadReuseLevel && !p->analysisLoad)
    {
        x265_log(p, X265_LOG_WARNING, "analysis-load-reuse-level can be set only when analysis-load is enabled."
            " Resetting analysis-load-reuse-level to 0.\n");
        p->analysisLoadReuseLevel = 0;
    }

    if (p->analysisSave && !p->analysisSaveReuseLevel)
        p->analysisSaveReuseLevel = 5;

    if (p->analysisLoad && !p->analysisLoadReuseLevel)
        p->analysisLoadReuseLevel = 5;

    if ((p->analysisLoad || p->analysisSave) && (p->bDistributeModeAnalysis || p->bDistributeMotionEstimation))
    {
        x265_log(p, X265_LOG_WARNING, "Analysis load/save options incompatible with pmode/pme, Disabling pmode/pme\n");
        p->bDistributeMotionEstimation = p->bDistributeModeAnalysis = 0;
    }

    if ((p->analysisLoad || p->analysisSave) && (p->analysisMultiPassRefine || p->analysisMultiPassDistortion))
    {
        x265_log(p, X265_LOG_WARNING, "Cannot use Analysis load/save option and multi-pass-opt-analysis/multi-pass-opt-distortion together,"
            "Disabling Analysis load/save and multi-pass-opt-analysis/multi-pass-opt-distortion\n");
        p->analysisSave = p->analysisLoad = NULL;
        p->analysisMultiPassRefine = p->analysisMultiPassDistortion = 0;
    }
    if (p->scaleFactor)
    {
        if (p->scaleFactor == 1)
        {
            p->scaleFactor = 0;
        }
        else if ((p->analysisSaveReuseLevel > 6 && p->analysisSaveReuseLevel != 10) || (p->analysisLoadReuseLevel > 6 && p->analysisLoadReuseLevel != 10))
        {
            x265_log(p, X265_LOG_WARNING, "Input scaling works with analysis-save/load and analysis-save/load-reuse-level 1-6 and 10. Disabling scale-factor.\n");
            p->scaleFactor = 0;
        }
    }

    if (p->intraRefine && p->analysisLoadReuseLevel && p->analysisLoadReuseLevel < 10)
    {
        x265_log(p, X265_LOG_WARNING, "Intra refinement requires analysis load, analysis-load-reuse-level 10. Disabling intra refine.\n");
        p->intraRefine = 0;
    }

    if (p->interRefine && p->analysisLoadReuseLevel && p->analysisLoadReuseLevel < 10)
    {
        x265_log(p, X265_LOG_WARNING, "Inter refinement requires analysis load, analysis-load-reuse-level 10. Disabling inter refine.\n");
        p->interRefine = 0;
    }

    if (p->bDynamicRefine && p->analysisLoadReuseLevel && p->analysisLoadReuseLevel < 10)
    {
        x265_log(p, X265_LOG_WARNING, "Dynamic refinement requires analysis load, analysis-load-reuse-level 10. Disabling dynamic refine.\n");
        p->bDynamicRefine = 0;

        if (p->interRefine)
        {
            x265_log(p, X265_LOG_WARNING, "Inter refine cannot be used with dynamic refine. Disabling refine-inter.\n");
            p->interRefine = 0;
        }
    }
    if (p->scaleFactor && !p->interRefine && !p->bDynamicRefine && p->analysisLoadReuseLevel == 10)
    {
        x265_log(p, X265_LOG_WARNING, "Inter refinement 0 is not supported with scaling and analysis-reuse-level=10. Enabling refine-inter 1.\n");
        p->interRefine = 1;
    }

    if (!(p->bAnalysisType == HEVC_INFO) && p->limitTU && (p->interRefine || p->bDynamicRefine))
    {
        x265_log(p, X265_LOG_WARNING, "Inter refinement does not support limitTU. Disabling limitTU.\n");
        p->limitTU = 0;
    }

    if (p->ctuDistortionRefine == CTU_DISTORTION_INTERNAL)
    {
        if (!p->analysisLoad && !p->analysisSave)
        {
            x265_log(p, X265_LOG_WARNING, "refine-ctu-distortion 1 requires analysis save/load. Disabling refine-ctu-distortion\n");
            p->ctuDistortionRefine = 0;
        }
        if (p->scaleFactor && p->analysisLoad)
        {
            x265_log(p, X265_LOG_WARNING, "refine-ctu-distortion 1 cannot be enabled along with multi resolution analysis refinement. Disabling refine-ctu-distortion\n");
            p->ctuDistortionRefine = 0;
        }
    }

    if ((p->analysisMultiPassRefine || p->analysisMultiPassDistortion) && (p->bDistributeModeAnalysis || p->bDistributeMotionEstimation))
    {
        x265_log(p, X265_LOG_WARNING, "multi-pass-opt-analysis/multi-pass-opt-distortion incompatible with pmode/pme, Disabling pmode/pme\n");
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
    p->scenecutBias = (double)(p->scenecutBias / 100);

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
    if (p->searchMethod == X265_SEA && (p->bDistributeMotionEstimation || p->bDistributeModeAnalysis))
    {
        x265_log(p, X265_LOG_WARNING, "Disabling pme and pmode: --pme and --pmode cannot be used with SEA motion search!\n");
        p->bDistributeMotionEstimation = 0;
        p->bDistributeModeAnalysis = 0;
    }

    if (!p->rc.bStatWrite && !p->rc.bStatRead && (p->analysisMultiPassRefine || p->analysisMultiPassDistortion))
    {
        x265_log(p, X265_LOG_WARNING, "analysis-multi-pass/distortion is enabled only when rc multi pass is enabled. Disabling multi-pass-opt-analysis and multi-pass-opt-distortion\n");
        p->analysisMultiPassRefine = 0;
        p->analysisMultiPassDistortion = 0;
    }
    if (p->analysisMultiPassRefine && p->rc.bStatWrite && p->rc.bStatRead)
    {
        x265_log(p, X265_LOG_WARNING, "--multi-pass-opt-analysis doesn't support refining analysis through multiple-passes; it only reuses analysis from the second-to-last pass to the last pass.Disabling reading\n");
        p->rc.bStatRead = 0;
    }

    /* some options make no sense if others are disabled */
    p->bSaoNonDeblocked &= p->bEnableSAO;
    p->bEnableTSkipFast &= p->bEnableTransformSkip;
    p->bLimitSAO &= p->bEnableSAO;

    if (m_param->bUseAnalysisFile && m_param->analysisLoad && (p->confWinRightOffset || p->confWinBottomOffset))
        x265_log(p, X265_LOG_WARNING, "It is recommended not to set conformance window offset in file based analysis-load."
                                      " Offsets are shared in the analysis file already.\n");
    /* initialize the conformance window */
    m_conformanceWindow.bEnabled = false;
    m_conformanceWindow.rightOffset = 0;
    m_conformanceWindow.topOffset = 0;
    m_conformanceWindow.bottomOffset = 0;
    m_conformanceWindow.leftOffset = 0;

    uint32_t padsize = 0;
    if (m_param->analysisLoad && m_param->bUseAnalysisFile)
    {
        m_analysisFileIn = x265_fopen(m_param->analysisLoad, "rb");
        if (!m_analysisFileIn)
        {
            x265_log_file(NULL, X265_LOG_ERROR, "Analysis load: failed to open file %s\n", m_param->analysisLoad);
            m_aborted = true;
        }
        else
        {
            int rightOffset, bottomOffset;
            if (fread(&rightOffset, sizeof(int), 1, m_analysisFileIn) != 1)
            {
                x265_log(NULL, X265_LOG_ERROR, "Error reading analysis data. Conformance window right offset missing\n");
                m_aborted = true;
            }
            else if (rightOffset && p->analysisLoadReuseLevel > 1)
            {
                int scaleFactor = p->scaleFactor < 2 ? 1 : p->scaleFactor;
                padsize = rightOffset * scaleFactor;
                p->sourceWidth += padsize;
                m_conformanceWindow.bEnabled = true;
                m_conformanceWindow.rightOffset = padsize;
            }

            if (fread(&bottomOffset, sizeof(int), 1, m_analysisFileIn) != 1)
            {
                x265_log(NULL, X265_LOG_ERROR, "Error reading analysis data. Conformance window bottom offset missing\n");
                m_aborted = true;
            }
            else if (bottomOffset && p->analysisLoadReuseLevel > 1)
            {
                int scaleFactor = p->scaleFactor < 2 ? 1 : p->scaleFactor;
                padsize = bottomOffset * scaleFactor;
                p->sourceHeight += padsize;
                m_conformanceWindow.bEnabled = true;
                m_conformanceWindow.bottomOffset = padsize;
            }
        }
    }

    /* set pad size if width is not multiple of the minimum CU size */
    if (p->confWinRightOffset)
    {
        if ((p->sourceWidth + p->confWinRightOffset) & (p->minCUSize - 1))
        {
            x265_log(p, X265_LOG_ERROR, "Incompatible conformance window right offset."
                                          " This when added to the source width should be a multiple of minCUSize\n");
            m_aborted = true;
        }
        else
        {
            p->sourceWidth += p->confWinRightOffset;
            m_conformanceWindow.bEnabled = true;
            m_conformanceWindow.rightOffset = p->confWinRightOffset;
        }
    }
    else if (p->sourceWidth & (p->minCUSize - 1))
    {
        uint32_t rem = p->sourceWidth & (p->minCUSize - 1);
        padsize = p->minCUSize - rem;
        p->sourceWidth += padsize;

        m_conformanceWindow.bEnabled = true;
        m_conformanceWindow.rightOffset = padsize;
    }

    if (p->bEnableRdRefine && (p->rdLevel < 5 || !p->rc.aqMode))
    {
        p->bEnableRdRefine = false;
        x265_log(p, X265_LOG_WARNING, "--rd-refine disabled, requires RD level > 4 and adaptive quant\n");
    }

    if (p->bOptCUDeltaQP && p->rdLevel < 5)
    {
        p->bOptCUDeltaQP = false;
        x265_log(p, X265_LOG_WARNING, "--opt-cu-delta-qp disabled, requires RD level > 4\n");
    }

    if (p->limitTU && p->tuQTMaxInterDepth < 2)
    {
        p->limitTU = 0;
        x265_log(p, X265_LOG_WARNING, "limit-tu disabled, requires tu-inter-depth > 1\n");
    }
    bool bIsVbv = m_param->rc.vbvBufferSize > 0 && m_param->rc.vbvMaxBitrate > 0;
    if (!m_param->bLossless && (m_param->rc.aqMode || bIsVbv || m_param->bAQMotion))
    {
        if (p->rc.qgSize < X265_MAX(8, p->minCUSize))
        {
            p->rc.qgSize = X265_MAX(8, p->minCUSize);
            x265_log(p, X265_LOG_WARNING, "QGSize should be greater than or equal to 8 and minCUSize, setting QGSize = %d\n", p->rc.qgSize);
        }
        if (p->rc.qgSize > p->maxCUSize)
        {
            p->rc.qgSize = p->maxCUSize;
            x265_log(p, X265_LOG_WARNING, "QGSize should be less than or equal to maxCUSize, setting QGSize = %d\n", p->rc.qgSize);
        }
    }
    else
        m_param->rc.qgSize = p->maxCUSize;

    if (m_param->dynamicRd && (!bIsVbv || !p->rc.aqMode || p->rdLevel > 4))
    {
        p->dynamicRd = 0;
        x265_log(p, X265_LOG_WARNING, "Dynamic-rd disabled, requires RD <= 4, VBV and aq-mode enabled\n");
    }

    if (!p->bEnableFrameDuplication && p->dupThreshold && p->dupThreshold != 70)
    {
        x265_log(p, X265_LOG_WARNING, "Frame-duplication threshold works only with frame-duplication enabled. Enabling frame-duplication.\n");
        p->bEnableFrameDuplication = 1;
    }

    if (p->bEnableFrameDuplication && p->interlaceMode)
    {
        x265_log(p, X265_LOG_WARNING, "Frame-duplication does not support interlace mode. Disabling Frame Duplication.\n");
        p->bEnableFrameDuplication = 0;
    }

    if (p->bEnableFrameDuplication && p->pictureStructure != 0 && p->pictureStructure != -1)
    {
        x265_log(p, X265_LOG_WARNING, "Frame-duplication works only with pic_struct = 0. Setting pic-struct = 0.\n");
        p->pictureStructure = 0;
    }

    if (m_param->bEnableFrameDuplication && (!bIsVbv || !m_param->bEmitHRDSEI))
    {
        x265_log(m_param, X265_LOG_WARNING, "Frame-duplication require NAL HRD and VBV parameters. Disabling frame duplication\n");
        m_param->bEnableFrameDuplication = 0;
    }
#ifdef ENABLE_HDR10_PLUS
    if (m_param->bDhdr10opt && m_param->toneMapFile == NULL)
    {
        x265_log(p, X265_LOG_WARNING, "Disabling dhdr10-opt. dhdr10-info must be enabled.\n");
        m_param->bDhdr10opt = 0;
    }

    if (m_param->toneMapFile)
    {
        if (!x265_fopen(p->toneMapFile, "r"))
        {
            x265_log(p, X265_LOG_ERROR, "Unable to open tone-map file.\n");
            m_bToneMap = 0;
            m_param->toneMapFile = NULL;
            m_aborted = true;
        }
        else
            m_bToneMap = 1;
    }
    else
        m_bToneMap = 0;
#else
    if (m_param->toneMapFile)
    {
        x265_log(p, X265_LOG_WARNING, "--dhdr10-info disabled. Enable HDR10_PLUS in cmake.\n");
        m_bToneMap = 0;
        m_param->toneMapFile = NULL;
    }
    else if (m_param->bDhdr10opt)
    {
        x265_log(p, X265_LOG_WARNING, "Disabling dhdr10-opt. dhdr10-info must be enabled.\n");
        m_param->bDhdr10opt = 0;
    }
#endif

    if (p->uhdBluray)
    {
        p->bEnableAccessUnitDelimiters = 1;
        p->vui.aspectRatioIdc = 1;
        p->bEmitHRDSEI = 1;
        int disableUhdBd = 0;

        if (p->levelIdc && p->levelIdc != 51)
        {
            x265_log(p, X265_LOG_WARNING, "uhd-bd: Wrong level specified, UHD Bluray mandates Level 5.1\n");
        }
        p->levelIdc = 51;

        if (!p->bHighTier)
        {
            x265_log(p, X265_LOG_WARNING, "uhd-bd: Turning on high tier\n");
            p->bHighTier = 1;
        }

        if (!p->bRepeatHeaders)
        {
            x265_log(p, X265_LOG_WARNING, "uhd-bd: Turning on repeat-headers\n");
            p->bRepeatHeaders = 1;
        }

        if (p->bOpenGOP)
        {
            x265_log(p, X265_LOG_WARNING, "uhd-bd: Turning off open GOP\n");
            p->bOpenGOP = false;
        }

        if (p->bIntraRefresh)
        {
            x265_log(p, X265_LOG_WARNING, "uhd-bd: turning off intra-refresh\n");
            p->bIntraRefresh = 0;
        }

        if (p->keyframeMin != 1)
        {
            x265_log(p, X265_LOG_WARNING, "uhd-bd: keyframeMin is always 1\n");
            p->keyframeMin = 1;
        }

        int fps = (p->fpsNum + p->fpsDenom - 1) / p->fpsDenom;
        if (p->keyframeMax > fps)
        {
            x265_log(p, X265_LOG_WARNING, "uhd-bd: reducing keyframeMax to %d\n", fps);
            p->keyframeMax = fps;
        }

        if (p->maxNumReferences > 6)
        {
            x265_log(p, X265_LOG_WARNING, "uhd-bd: reducing references to 6\n");
            p->maxNumReferences = 6;
        }

        if (p->bEnableTemporalSubLayers)
        {
            x265_log(p, X265_LOG_WARNING, "uhd-bd: Turning off temporal layering\n");
            p->bEnableTemporalSubLayers = 0;
        }

        if (p->vui.colorPrimaries != 1 && p->vui.colorPrimaries != 9)
        {
            x265_log(p, X265_LOG_ERROR, "uhd-bd: colour primaries should be either BT.709 or BT.2020\n");
            disableUhdBd = 1;
        }
        else if (p->vui.colorPrimaries == 9)
        {
            p->vui.bEnableChromaLocInfoPresentFlag = 1;
            p->vui.chromaSampleLocTypeTopField = 2;
            p->vui.chromaSampleLocTypeBottomField = 2;
        }

        if (p->vui.transferCharacteristics != 1 && p->vui.transferCharacteristics != 14 && p->vui.transferCharacteristics != 16)
        {
            x265_log(p, X265_LOG_ERROR, "uhd-bd: transfer characteristics supported are BT.709, BT.2020-10 or SMPTE ST.2084\n");
            disableUhdBd = 1;
        }
        if (p->vui.matrixCoeffs != 1 && p->vui.matrixCoeffs != 9)
        {
            x265_log(p, X265_LOG_ERROR, "uhd-bd: matrix coeffs supported are either BT.709 or BT.2020\n");
            disableUhdBd = 1;
        }
        if (!((p->sourceWidth == 1920 && p->sourceHeight == 1080) || (p->sourceWidth == 3840 && p->sourceHeight == 2160)))
        {
            x265_log(p, X265_LOG_ERROR, "uhd-bd: Supported resolutions are 1920x1080 and 3840x2160\n");
            disableUhdBd = 1;
        }
        if (disableUhdBd)
        {
            p->uhdBluray = 0;
            x265_log(p, X265_LOG_ERROR, "uhd-bd: Disabled\n");
        }
    }
    /* set pad size if height is not multiple of the minimum CU size */
    if (p->confWinBottomOffset)
    {
        if ((p->sourceHeight + p->confWinBottomOffset) & (p->minCUSize - 1))
        {
            x265_log(p, X265_LOG_ERROR, "Incompatible conformance window bottom offset."
                " This when added to the source height should be a multiple of minCUSize\n");
            m_aborted = true;
        }
        else
        {
            p->sourceHeight += p->confWinBottomOffset;
            m_conformanceWindow.bEnabled = true;
            m_conformanceWindow.bottomOffset = p->confWinBottomOffset;
        }
    }
    else if(p->sourceHeight & (p->minCUSize - 1))
    {
        uint32_t rem = p->sourceHeight & (p->minCUSize - 1);
        padsize = p->minCUSize - rem;
        p->sourceHeight += padsize;
        m_conformanceWindow.bEnabled = true;
        m_conformanceWindow.bottomOffset = padsize;
    }

    if (p->bLogCuStats)
        x265_log(p, X265_LOG_WARNING, "--cu-stats option is now deprecated\n");

    if (p->log2MaxPocLsb < 4)
    {
        x265_log(p, X265_LOG_WARNING, "maximum of the picture order count can not be less than 4\n");
        p->log2MaxPocLsb = 4;
    }

    if (p->maxSlices < 1)
    {
        x265_log(p, X265_LOG_WARNING, "maxSlices can not be less than 1, force set to 1\n");
        p->maxSlices = 1;
    }
    const uint32_t numRows = (p->sourceHeight + p->maxCUSize - 1) / p->maxCUSize;
    const uint32_t slicesLimit = X265_MIN(numRows, NALList::MAX_NAL_UNITS - 1);
    if (p->maxSlices > slicesLimit)
    {
        x265_log(p, X265_LOG_WARNING, "maxSlices can not be more than min(rows, MAX_NAL_UNITS-1), force set to %d\n", slicesLimit);
        p->maxSlices = slicesLimit;
    }
    if (p->bHDR10Opt)
    {
        if (p->internalCsp != X265_CSP_I420 || p->internalBitDepth != 10 || p->vui.colorPrimaries != 9 ||
            p->vui.transferCharacteristics != 16 || p->vui.matrixCoeffs != 9)
        {
            x265_log(p, X265_LOG_ERROR, "Recommended Settings for HDR10-opt: colour primaries should be BT.2020,\n"
                                        "                                            transfer characteristics should be SMPTE ST.2084,\n"
                                        "                                            matrix coeffs should be BT.2020,\n"
                                        "                                            the input video should be 10 bit 4:2:0\n"
                                        "                                            Disabling hdr10-opt.\n");
            p->bHDR10Opt = 0;
        }
    }

    if (m_param->toneMapFile || p->bHDR10Opt || p->bEmitHDR10SEI)
    {
        if (!p->bRepeatHeaders)
        {
            p->bRepeatHeaders = 1;
            x265_log(p, X265_LOG_WARNING, "Turning on repeat-headers for HDR compatibility\n");
        }
    }

    p->maxLog2CUSize = g_log2Size[p->maxCUSize];
    p->maxCUDepth    = p->maxLog2CUSize - g_log2Size[p->minCUSize];
    p->unitSizeDepth = p->maxLog2CUSize - LOG2_UNIT_SIZE;
    p->num4x4Partitions = (1U << (p->unitSizeDepth << 1));

    if (p->radl && p->bOpenGOP)
    {
        p->radl = 0;
        x265_log(p, X265_LOG_WARNING, "Radl requires closed gop structure. Disabling radl.\n");
    }

    if ((p->chunkStart || p->chunkEnd) && p->bOpenGOP && m_param->bResetZoneConfig)
    {
        p->chunkStart = p->chunkEnd = 0;
        x265_log(p, X265_LOG_WARNING, "Chunking requires closed gop structure. Disabling chunking.\n");
    }

    if (p->chunkEnd < p->chunkStart)
    {
        p->chunkStart = p->chunkEnd = 0;
        x265_log(p, X265_LOG_WARNING, "chunk-end cannot be less than chunk-start. Disabling chunking.\n");
    }

    if (p->dolbyProfile)     // Default disabled.
        configureDolbyVisionParams(p);

    if (p->rc.zonefileCount && p->rc.zoneCount)
    {
        p->rc.zoneCount = 0;
        x265_log(p, X265_LOG_WARNING, "Only zone or zonefile can be used. Enabling only zonefile\n");
    }

    if (m_param->rc.zonefileCount && p->bOpenGOP)
    {
        p->bOpenGOP = 0;
        x265_log(p, X265_LOG_WARNING, "Zone encoding requires closed gop structure. Enabling closed GOP.\n");
    }

    if (m_param->rc.zonefileCount && !p->bRepeatHeaders)
    {
        p->bRepeatHeaders = 1;
        x265_log(p, X265_LOG_WARNING, "Turning on repeat - headers for zone encoding\n");
    }

    if (m_param->bEnableHME)
    {
        if (m_param->sourceHeight < 540)
        {
            x265_log(p, X265_LOG_WARNING, "Source height < 540p is too low for HME. Disabling HME.\n");
            p->bEnableHME = 0;
        }
    }

    if (m_param->bEnableHME)
    {
        if (m_param->searchMethod != m_param->hmeSearchMethod[2])
            m_param->searchMethod = m_param->hmeSearchMethod[2];
        if (m_param->searchRange != m_param->hmeRange[2])
            m_param->searchRange = m_param->hmeRange[2];
    }

   if (p->bHistBasedSceneCut && !p->edgeTransitionThreshold)
   {
       p->edgeTransitionThreshold = 0.03;
       x265_log(p, X265_LOG_WARNING, "using  default threshold %.2lf for scene cut detection\n", p->edgeTransitionThreshold);
   }

}

void Encoder::readAnalysisFile(x265_analysis_data* analysis, int curPoc, const x265_picture* picIn, int paramBytes)
{
#define X265_FREAD(val, size, readSize, fileOffset, src)\
    if (!m_param->bUseAnalysisFile)\
        {\
        memcpy(val, src, (size * readSize));\
        }\
        else if (fread(val, size, readSize, fileOffset) != readSize)\
    {\
        x265_log(NULL, X265_LOG_ERROR, "Error reading analysis data\n");\
        x265_free_analysis_data(m_param, analysis);\
        m_aborted = true;\
        return;\
    }\

    static uint64_t consumedBytes = 0;
    static uint64_t totalConsumedBytes = 0;
    uint32_t depthBytes = 0;
    if (m_param->bUseAnalysisFile)
        fseeko(m_analysisFileIn, totalConsumedBytes + paramBytes, SEEK_SET);
    const x265_analysis_data *picData = &(picIn->analysisData);
    x265_analysis_intra_data *intraPic = picData->intraData;
    x265_analysis_inter_data *interPic = picData->interData;
    x265_analysis_distortion_data *picDistortion = picData->distortionData;

    int poc; uint32_t frameRecordSize;
    X265_FREAD(&frameRecordSize, sizeof(uint32_t), 1, m_analysisFileIn, &(picData->frameRecordSize));
    X265_FREAD(&depthBytes, sizeof(uint32_t), 1, m_analysisFileIn, &(picData->depthBytes));
    X265_FREAD(&poc, sizeof(int), 1, m_analysisFileIn, &(picData->poc));

    if (m_param->bUseAnalysisFile)
    {
        uint64_t currentOffset = totalConsumedBytes;

        /* Seeking to the right frame Record */
        while (poc != curPoc && !feof(m_analysisFileIn))
        {
            currentOffset += frameRecordSize;
            fseeko(m_analysisFileIn, currentOffset + paramBytes, SEEK_SET);
            X265_FREAD(&frameRecordSize, sizeof(uint32_t), 1, m_analysisFileIn, &(picData->frameRecordSize));
            X265_FREAD(&depthBytes, sizeof(uint32_t), 1, m_analysisFileIn, &(picData->depthBytes));
            X265_FREAD(&poc, sizeof(int), 1, m_analysisFileIn, &(picData->poc));
        }
        if (poc != curPoc || feof(m_analysisFileIn))
        {
            x265_log(NULL, X265_LOG_WARNING, "Error reading analysis data: Cannot find POC %d\n", curPoc);
            x265_free_analysis_data(m_param, analysis);
            return;
        }
    }

    uint32_t numCUsLoad, numCUsInHeightLoad;

    /* Now arrived at the right frame, read the record */
    analysis->poc = poc;
    analysis->frameRecordSize = frameRecordSize;
    X265_FREAD(&analysis->sliceType, sizeof(int), 1, m_analysisFileIn, &(picData->sliceType));
    X265_FREAD(&analysis->bScenecut, sizeof(int), 1, m_analysisFileIn, &(picData->bScenecut));
    if (m_param->bHistBasedSceneCut)
    {
        X265_FREAD(&analysis->edgeHist, sizeof(int32_t), EDGE_BINS, m_analysisFileIn, &m_curEdgeHist);
        X265_FREAD(&analysis->yuvHist[0], sizeof(int32_t), HISTOGRAM_BINS, m_analysisFileIn, &m_curYUVHist[0]);
        if (m_param->internalCsp != X265_CSP_I400)
        {
            X265_FREAD(&analysis->yuvHist[1], sizeof(int32_t), HISTOGRAM_BINS, m_analysisFileIn, &m_curYUVHist[1]);
            X265_FREAD(&analysis->yuvHist[2], sizeof(int32_t), HISTOGRAM_BINS, m_analysisFileIn, &m_curYUVHist[2]);
        }
    }
    X265_FREAD(&analysis->satdCost, sizeof(int64_t), 1, m_analysisFileIn, &(picData->satdCost));
    X265_FREAD(&numCUsLoad, sizeof(int), 1, m_analysisFileIn, &(picData->numCUsInFrame));
    X265_FREAD(&analysis->numPartitions, sizeof(int), 1, m_analysisFileIn, &(picData->numPartitions));

    /* Update analysis info to save current settings */
    uint32_t widthInCU = (m_param->sourceWidth + m_param->maxCUSize - 1) >> m_param->maxLog2CUSize;
    uint32_t heightInCU = (m_param->sourceHeight + m_param->maxCUSize - 1) >> m_param->maxLog2CUSize;
    uint32_t numCUsInFrame = widthInCU * heightInCU;
    analysis->numCUsInFrame = numCUsInFrame;
    analysis->numCuInHeight = heightInCU;

    if (m_param->bDisableLookahead)
    {
        X265_FREAD(&numCUsInHeightLoad, sizeof(uint32_t), 1, m_analysisFileIn, &(picData->numCuInHeight));
        X265_FREAD(&analysis->lookahead, sizeof(x265_lookahead_data), 1, m_analysisFileIn, &(picData->lookahead));
    }
    int scaledNumPartition = analysis->numPartitions;
    int factor = 1 << m_param->scaleFactor;

    if (m_param->scaleFactor)
        analysis->numPartitions *= factor;
    /* Memory is allocated for inter and intra analysis data based on the slicetype */
    x265_alloc_analysis_data(m_param, analysis);

    if (m_param->ctuDistortionRefine == CTU_DISTORTION_INTERNAL)
    {
        X265_FREAD((analysis->distortionData)->ctuDistortion, sizeof(sse_t), numCUsLoad, m_analysisFileIn, picDistortion);
        computeDistortionOffset(analysis);
    }
    if (m_param->bDisableLookahead && m_rateControl->m_isVbv)
    {
        size_t vbvCount = m_param->lookaheadDepth + m_param->bframes + 2;
        X265_FREAD(analysis->lookahead.intraVbvCost, sizeof(uint32_t), numCUsLoad, m_analysisFileIn, picData->lookahead.intraVbvCost);
        X265_FREAD(analysis->lookahead.vbvCost, sizeof(uint32_t), numCUsLoad, m_analysisFileIn, picData->lookahead.vbvCost);
        X265_FREAD(analysis->lookahead.satdForVbv, sizeof(uint32_t), numCUsInHeightLoad, m_analysisFileIn, picData->lookahead.satdForVbv);
        X265_FREAD(analysis->lookahead.intraSatdForVbv, sizeof(uint32_t), numCUsInHeightLoad, m_analysisFileIn, picData->lookahead.intraSatdForVbv);
        X265_FREAD(analysis->lookahead.plannedSatd, sizeof(int64_t), vbvCount, m_analysisFileIn, picData->lookahead.plannedSatd);

        if (m_param->scaleFactor)
        {
            for (uint64_t index = 0; index < vbvCount; index++)
                analysis->lookahead.plannedSatd[index] *= factor;

            for (uint32_t i = 0; i < numCUsInHeightLoad; i++)
            {
                analysis->lookahead.satdForVbv[i] *= factor;
                analysis->lookahead.intraSatdForVbv[i] *= factor;
            }
            for (uint32_t i = 0; i < numCUsLoad; i++)
            {
                analysis->lookahead.vbvCost[i] *= factor;
                analysis->lookahead.intraVbvCost[i] *= factor;
            }
        }
    }
    if (analysis->sliceType == X265_TYPE_IDR || analysis->sliceType == X265_TYPE_I)
    {
        if (m_param->bAnalysisType == HEVC_INFO)
            return;
        if (m_param->analysisLoadReuseLevel < 2)
            return;

        uint8_t *tempBuf = NULL, *depthBuf = NULL, *modeBuf = NULL, *partSizes = NULL;
        int8_t *cuQPBuf = NULL;

        tempBuf = X265_MALLOC(uint8_t, depthBytes * 3);
        depthBuf = tempBuf;
        modeBuf = tempBuf + depthBytes;
        partSizes = tempBuf + 2 * depthBytes;
        if (m_param->rc.cuTree)
            cuQPBuf = X265_MALLOC(int8_t, depthBytes);

        X265_FREAD(depthBuf, sizeof(uint8_t), depthBytes, m_analysisFileIn, intraPic->depth);
        X265_FREAD(modeBuf, sizeof(uint8_t), depthBytes, m_analysisFileIn, intraPic->chromaModes);
        X265_FREAD(partSizes, sizeof(uint8_t), depthBytes, m_analysisFileIn, intraPic->partSizes);
        if (m_param->rc.cuTree) { X265_FREAD(cuQPBuf, sizeof(int8_t), depthBytes, m_analysisFileIn, intraPic->cuQPOff); }

        size_t count = 0;
        for (uint32_t d = 0; d < depthBytes; d++)
        {
            int bytes = analysis->numPartitions >> (depthBuf[d] * 2);
            if (m_param->scaleFactor)
            {
                if (depthBuf[d] == 0)
                    depthBuf[d] = 1;
                if (partSizes[d] == SIZE_NxN)
                    partSizes[d] = SIZE_2Nx2N;
            }
            memset(&(analysis->intraData)->depth[count], depthBuf[d], bytes);
            memset(&(analysis->intraData)->chromaModes[count], modeBuf[d], bytes);
            memset(&(analysis->intraData)->partSizes[count], partSizes[d], bytes);
            if (m_param->rc.cuTree)
                memset(&(analysis->intraData)->cuQPOff[count], cuQPBuf[d], bytes);
            count += bytes;
        }

        if (!m_param->scaleFactor)
        {
            X265_FREAD((analysis->intraData)->modes, sizeof(uint8_t), numCUsLoad * analysis->numPartitions, m_analysisFileIn, intraPic->modes);
        }
        else
        {
            uint8_t *tempLumaBuf = X265_MALLOC(uint8_t, numCUsLoad * scaledNumPartition);
            X265_FREAD(tempLumaBuf, sizeof(uint8_t), numCUsLoad * scaledNumPartition, m_analysisFileIn, intraPic->modes);
            for (uint32_t ctu32Idx = 0, cnt = 0; ctu32Idx < numCUsLoad * scaledNumPartition; ctu32Idx++, cnt += factor)
                memset(&(analysis->intraData)->modes[cnt], tempLumaBuf[ctu32Idx], factor);
            X265_FREE(tempLumaBuf);
        }
        if (m_param->rc.cuTree)
            X265_FREE(cuQPBuf);
        X265_FREE(tempBuf);
        consumedBytes += frameRecordSize;
    }

    else
    {
        uint32_t numDir = analysis->sliceType == X265_TYPE_P ? 1 : 2;
        uint32_t numPlanes = m_param->internalCsp == X265_CSP_I400 ? 1 : 3;
        X265_FREAD((WeightParam*)analysis->wt, sizeof(WeightParam), numPlanes * numDir, m_analysisFileIn, (picIn->analysisData.wt));
        if (m_param->analysisLoadReuseLevel < 2)
            return;

        uint8_t *tempBuf = NULL, *depthBuf = NULL, *modeBuf = NULL, *partSize = NULL, *mergeFlag = NULL;
        uint8_t *interDir = NULL, *chromaDir = NULL, *mvpIdx[2];
        MV* mv[2];
        int8_t* refIdx[2];
        int8_t* cuQPBuf = NULL;

        int numBuf = m_param->analysisLoadReuseLevel > 4 ? 4 : 2;
        bool bIntraInInter = false;
        if (m_param->analysisLoadReuseLevel == 10)
        {
            numBuf++;
            bIntraInInter = (analysis->sliceType == X265_TYPE_P || m_param->bIntraInBFrames);
            if (bIntraInInter) numBuf++;
        }
        if (m_param->bAnalysisType == HEVC_INFO)
        {
            depthBytes = numCUsLoad * analysis->numPartitions;
            memcpy(((x265_analysis_inter_data *)analysis->interData)->depth, interPic->depth, depthBytes);
        }
        else
        {
            tempBuf = X265_MALLOC(uint8_t, depthBytes * numBuf);
            depthBuf = tempBuf;
            modeBuf = tempBuf + depthBytes;
            if (m_param->rc.cuTree)
                cuQPBuf = X265_MALLOC(int8_t, depthBytes);

            X265_FREAD(depthBuf, sizeof(uint8_t), depthBytes, m_analysisFileIn, interPic->depth);
            X265_FREAD(modeBuf, sizeof(uint8_t), depthBytes, m_analysisFileIn, interPic->modes);
            if (m_param->rc.cuTree) { X265_FREAD(cuQPBuf, sizeof(int8_t), depthBytes, m_analysisFileIn, interPic->cuQPOff); }

            if (m_param->analysisLoadReuseLevel > 4)
            {
                partSize = modeBuf + depthBytes;
                mergeFlag = partSize + depthBytes;
                X265_FREAD(partSize, sizeof(uint8_t), depthBytes, m_analysisFileIn, interPic->partSize);
                X265_FREAD(mergeFlag, sizeof(uint8_t), depthBytes, m_analysisFileIn, interPic->mergeFlag);

                if (m_param->analysisLoadReuseLevel == 10)
                {
                    interDir = mergeFlag + depthBytes;
                    X265_FREAD(interDir, sizeof(uint8_t), depthBytes, m_analysisFileIn, interPic->interDir);
                    if (bIntraInInter)
                    {
                        chromaDir = interDir + depthBytes;
                        X265_FREAD(chromaDir, sizeof(uint8_t), depthBytes, m_analysisFileIn, intraPic->chromaModes);
                    }
                    for (uint32_t i = 0; i < numDir; i++)
                    {
                        mvpIdx[i] = X265_MALLOC(uint8_t, depthBytes);
                        refIdx[i] = X265_MALLOC(int8_t, depthBytes);
                        mv[i] = X265_MALLOC(MV, depthBytes);
                        X265_FREAD(mvpIdx[i], sizeof(uint8_t), depthBytes, m_analysisFileIn, interPic->mvpIdx[i]);
                        X265_FREAD(refIdx[i], sizeof(int8_t), depthBytes, m_analysisFileIn, interPic->refIdx[i]);
                        X265_FREAD(mv[i], sizeof(MV), depthBytes, m_analysisFileIn, interPic->mv[i]);
                    }
                }
            }

            size_t count = 0;
            for (uint32_t d = 0; d < depthBytes; d++)
            {
                int bytes = analysis->numPartitions >> (depthBuf[d] * 2);
                if (m_param->scaleFactor && modeBuf[d] == MODE_INTRA && depthBuf[d] == 0)
                    depthBuf[d] = 1;
                memset(&(analysis->interData)->depth[count], depthBuf[d], bytes);
                memset(&(analysis->interData)->modes[count], modeBuf[d], bytes);
                if (m_param->rc.cuTree)
                    memset(&(analysis->interData)->cuQPOff[count], cuQPBuf[d], bytes);
                if (m_param->analysisLoadReuseLevel > 4)
                {
                    if (m_param->scaleFactor && modeBuf[d] == MODE_INTRA && partSize[d] == SIZE_NxN)
                        partSize[d] = SIZE_2Nx2N;
                    memset(&(analysis->interData)->partSize[count], partSize[d], bytes);
                    int numPU = (modeBuf[d] == MODE_INTRA) ? 1 : nbPartsTable[(int)partSize[d]];
                    for (int pu = 0; pu < numPU; pu++)
                    {
                        if (pu) d++;
                        (analysis->interData)->mergeFlag[count + pu] = mergeFlag[d];
                        if (m_param->analysisLoadReuseLevel == 10)
                        {
                            (analysis->interData)->interDir[count + pu] = interDir[d];
                            for (uint32_t i = 0; i < numDir; i++)
                            {
                                (analysis->interData)->mvpIdx[i][count + pu] = mvpIdx[i][d];
                                (analysis->interData)->refIdx[i][count + pu] = refIdx[i][d];
                                if (m_param->scaleFactor)
                                {
                                    mv[i][d].x *= (int32_t)m_param->scaleFactor;
                                    mv[i][d].y *= (int32_t)m_param->scaleFactor;
                                }
                                memcpy(&(analysis->interData)->mv[i][count + pu], &mv[i][d], sizeof(MV));
                            }
                        }
                    }
                    if (m_param->analysisLoadReuseLevel == 10 && bIntraInInter)
                        memset(&(analysis->intraData)->chromaModes[count], chromaDir[d], bytes);
                }
                count += bytes;
            }

            if (m_param->rc.cuTree)
                X265_FREE(cuQPBuf);
            X265_FREE(tempBuf);
        }
        if (m_param->analysisLoadReuseLevel == 10)
        {
            if (m_param->bAnalysisType != HEVC_INFO)
            {
                for (uint32_t i = 0; i < numDir; i++)
                {
                    X265_FREE(mvpIdx[i]);
                    X265_FREE(refIdx[i]);
                    X265_FREE(mv[i]);
                }
            }
            if (bIntraInInter)
            {
                if (!m_param->scaleFactor)
                {
                    X265_FREAD((analysis->intraData)->modes, sizeof(uint8_t), numCUsLoad * analysis->numPartitions, m_analysisFileIn, intraPic->modes);
                }
                else
                {
                    uint8_t *tempLumaBuf = X265_MALLOC(uint8_t, numCUsLoad * scaledNumPartition);
                    X265_FREAD(tempLumaBuf, sizeof(uint8_t), numCUsLoad * scaledNumPartition, m_analysisFileIn, intraPic->modes);
                    for (uint32_t ctu32Idx = 0, cnt = 0; ctu32Idx < numCUsLoad * scaledNumPartition; ctu32Idx++, cnt += factor)
                        memset(&(analysis->intraData)->modes[cnt], tempLumaBuf[ctu32Idx], factor);
                    X265_FREE(tempLumaBuf);
                }
            }
        }
        else
            X265_FREAD((analysis->interData)->ref, sizeof(int32_t), numCUsLoad * X265_MAX_PRED_MODE_PER_CTU * numDir, m_analysisFileIn, interPic->ref);

        consumedBytes += frameRecordSize;
        if (numDir == 1)
            totalConsumedBytes = consumedBytes;
    }

#undef X265_FREAD
}

void Encoder::readAnalysisFile(x265_analysis_data* analysis, int curPoc, const x265_picture* picIn, int paramBytes, cuLocation cuLoc)
{
#define X265_FREAD(val, size, readSize, fileOffset, src)\
    if (!m_param->bUseAnalysisFile)\
    {\
        memcpy(val, src, (size * readSize));\
    }\
    else if (fread(val, size, readSize, fileOffset) != readSize)\
    {\
        x265_log(NULL, X265_LOG_ERROR, "Error reading analysis data\n");\
        x265_free_analysis_data(m_param, analysis);\
        m_aborted = true;\
        return;\
    }\

    static uint64_t consumedBytes = 0;
    static uint64_t totalConsumedBytes = 0;
    uint32_t depthBytes = 0;
    if (m_param->bUseAnalysisFile)
        fseeko(m_analysisFileIn, totalConsumedBytes + paramBytes, SEEK_SET);

    const x265_analysis_data *picData = &(picIn->analysisData);
    x265_analysis_intra_data *intraPic = picData->intraData;
    x265_analysis_inter_data *interPic = picData->interData;
    x265_analysis_distortion_data *picDistortion = picData->distortionData;

    int poc; uint32_t frameRecordSize;
    X265_FREAD(&frameRecordSize, sizeof(uint32_t), 1, m_analysisFileIn, &(picData->frameRecordSize));
    X265_FREAD(&depthBytes, sizeof(uint32_t), 1, m_analysisFileIn, &(picData->depthBytes));
    X265_FREAD(&poc, sizeof(int), 1, m_analysisFileIn, &(picData->poc));

    if (m_param->bUseAnalysisFile)
    {
        uint64_t currentOffset = totalConsumedBytes;

        /* Seeking to the right frame Record */
        while (poc != curPoc && !feof(m_analysisFileIn))
        {
            currentOffset += frameRecordSize;
            fseeko(m_analysisFileIn, currentOffset + paramBytes, SEEK_SET);
            X265_FREAD(&frameRecordSize, sizeof(uint32_t), 1, m_analysisFileIn, &(picData->frameRecordSize));
            X265_FREAD(&depthBytes, sizeof(uint32_t), 1, m_analysisFileIn, &(picData->depthBytes));
            X265_FREAD(&poc, sizeof(int), 1, m_analysisFileIn, &(picData->poc));
        }
        if (poc != curPoc || feof(m_analysisFileIn))
        {
            x265_log(NULL, X265_LOG_WARNING, "Error reading analysis data: Cannot find POC %d\n", curPoc);
            x265_free_analysis_data(m_param, analysis);
            return;
        }
    }

    /* Now arrived at the right frame, read the record */
    analysis->poc = poc;
    analysis->frameRecordSize = frameRecordSize;
    X265_FREAD(&analysis->sliceType, sizeof(int), 1, m_analysisFileIn, &(picData->sliceType));
    X265_FREAD(&analysis->bScenecut, sizeof(int), 1, m_analysisFileIn, &(picData->bScenecut));
    if (m_param->bHistBasedSceneCut)
    {
        X265_FREAD(&analysis->edgeHist, sizeof(int32_t), EDGE_BINS, m_analysisFileIn, &m_curEdgeHist);
        X265_FREAD(&analysis->yuvHist[0], sizeof(int32_t), HISTOGRAM_BINS, m_analysisFileIn, &m_curYUVHist[0]);
        if (m_param->internalCsp != X265_CSP_I400)
        {
            X265_FREAD(&analysis->yuvHist[1], sizeof(int32_t), HISTOGRAM_BINS, m_analysisFileIn, &m_curYUVHist[1]);
            X265_FREAD(&analysis->yuvHist[2], sizeof(int32_t), HISTOGRAM_BINS, m_analysisFileIn, &m_curYUVHist[2]);
        }
    }
    X265_FREAD(&analysis->satdCost, sizeof(int64_t), 1, m_analysisFileIn, &(picData->satdCost));
    X265_FREAD(&analysis->numCUsInFrame, sizeof(int), 1, m_analysisFileIn, &(picData->numCUsInFrame));
    X265_FREAD(&analysis->numPartitions, sizeof(int), 1, m_analysisFileIn, &(picData->numPartitions));
    
    if (m_param->bDisableLookahead)
    {
        X265_FREAD(&analysis->numCuInHeight, sizeof(uint32_t), 1, m_analysisFileIn, &(picData->numCuInHeight));
        X265_FREAD(&analysis->lookahead, sizeof(x265_lookahead_data), 1, m_analysisFileIn, &(picData->lookahead));
    }
    int scaledNumPartition = analysis->numPartitions;
    int factor = 1 << m_param->scaleFactor;

    int numPartitions = analysis->numPartitions;
    int numCUsInFrame = analysis->numCUsInFrame;
    int numCuInHeight = analysis->numCuInHeight;
    /* Allocate memory for scaled resoultion's numPartitions and numCUsInFrame*/
    analysis->numPartitions = m_param->num4x4Partitions;
    analysis->numCUsInFrame = cuLoc.heightInCU * cuLoc.widthInCU;
    analysis->numCuInHeight = cuLoc.heightInCU;

    /* Memory is allocated for inter and intra analysis data based on the slicetype */
    x265_alloc_analysis_data(m_param, analysis);

    if (m_param->ctuDistortionRefine == CTU_DISTORTION_INTERNAL)
    {
        X265_FREAD((analysis->distortionData)->ctuDistortion, sizeof(sse_t), analysis->numCUsInFrame, m_analysisFileIn, picDistortion);
        computeDistortionOffset(analysis);
    }

    analysis->numPartitions = numPartitions * factor;
    analysis->numCUsInFrame = numCUsInFrame;
    analysis->numCuInHeight = numCuInHeight;
    if (m_param->bDisableLookahead && m_rateControl->m_isVbv)
    {
        uint32_t width = analysis->numCUsInFrame / analysis->numCuInHeight;
        bool skipLastRow = (analysis->numCuInHeight * 2) > cuLoc.heightInCU;
        bool skipLastCol = (width * 2) > cuLoc.widthInCU;
        uint32_t *intraVbvCostBuf = NULL, *vbvCostBuf = NULL, *satdForVbvBuf = NULL, *intraSatdForVbvBuf = NULL;
        intraVbvCostBuf = X265_MALLOC(uint32_t, analysis->numCUsInFrame);
        vbvCostBuf = X265_MALLOC(uint32_t, analysis->numCUsInFrame);
        satdForVbvBuf = X265_MALLOC(uint32_t, analysis->numCuInHeight);
        intraSatdForVbvBuf = X265_MALLOC(uint32_t, analysis->numCuInHeight);

        X265_FREAD(intraVbvCostBuf, sizeof(uint32_t), analysis->numCUsInFrame, m_analysisFileIn, picData->lookahead.intraVbvCost);
        X265_FREAD(vbvCostBuf, sizeof(uint32_t), analysis->numCUsInFrame, m_analysisFileIn, picData->lookahead.vbvCost);
        X265_FREAD(satdForVbvBuf, sizeof(uint32_t), analysis->numCuInHeight, m_analysisFileIn, picData->lookahead.satdForVbv);
        X265_FREAD(intraSatdForVbvBuf, sizeof(uint32_t), analysis->numCuInHeight, m_analysisFileIn, picData->lookahead.intraSatdForVbv);

        int k = 0;
        for (uint32_t i = 0; i < analysis->numCuInHeight; i++)
        {
            analysis->lookahead.satdForVbv[m_param->scaleFactor * i] = satdForVbvBuf[i] * m_param->scaleFactor;
            analysis->lookahead.intraSatdForVbv[m_param->scaleFactor * i] = intraSatdForVbvBuf[i] * m_param->scaleFactor;
            if (!(i == (analysis->numCuInHeight - 1) && skipLastRow))
            {
                analysis->lookahead.satdForVbv[(m_param->scaleFactor * i) + 1] = satdForVbvBuf[i] * m_param->scaleFactor;
                analysis->lookahead.intraSatdForVbv[(m_param->scaleFactor * i) + 1] = intraSatdForVbvBuf[i] * m_param->scaleFactor;
            }

            for (uint32_t j = 0; j < width; j++, k++)
            {
                analysis->lookahead.vbvCost[(i * m_param->scaleFactor * cuLoc.widthInCU) + (j * m_param->scaleFactor)] = vbvCostBuf[k];
                analysis->lookahead.intraVbvCost[(i * m_param->scaleFactor * cuLoc.widthInCU) + (j * m_param->scaleFactor)] = intraVbvCostBuf[k];

                if (!(j == (width - 1) && skipLastCol))
                {
                    analysis->lookahead.vbvCost[(i * m_param->scaleFactor * cuLoc.widthInCU) + (j * m_param->scaleFactor) + 1] = vbvCostBuf[k];
                    analysis->lookahead.intraVbvCost[(i * m_param->scaleFactor * cuLoc.widthInCU) + (j * m_param->scaleFactor) + 1] = intraVbvCostBuf[k];
                }
                if (!(i == (analysis->numCuInHeight - 1) && skipLastRow))
                {
                    analysis->lookahead.vbvCost[(i * m_param->scaleFactor * cuLoc.widthInCU) + cuLoc.widthInCU + (j * m_param->scaleFactor)] = vbvCostBuf[k];
                    analysis->lookahead.intraVbvCost[(i * m_param->scaleFactor * cuLoc.widthInCU) + cuLoc.widthInCU + (j * m_param->scaleFactor)] = intraVbvCostBuf[k];
                    if (!(j == (width - 1) && skipLastCol))
                    {
                        analysis->lookahead.vbvCost[(i * m_param->scaleFactor * cuLoc.widthInCU) + cuLoc.widthInCU + (j * m_param->scaleFactor) + 1] = vbvCostBuf[k];
                        analysis->lookahead.intraVbvCost[(i * m_param->scaleFactor * cuLoc.widthInCU) + cuLoc.widthInCU + (j * m_param->scaleFactor) + 1] = intraVbvCostBuf[k];
                    }
                }
            }
        }
        X265_FREE(satdForVbvBuf);
        X265_FREE(intraSatdForVbvBuf);
        X265_FREE(intraVbvCostBuf);
        X265_FREE(vbvCostBuf);
    }

    if (analysis->sliceType == X265_TYPE_IDR || analysis->sliceType == X265_TYPE_I)
    {
        if (m_param->analysisLoadReuseLevel < 2)
            return;

        uint8_t *tempBuf = NULL, *depthBuf = NULL, *modeBuf = NULL, *partSizes = NULL;
        int8_t *cuQPBuf = NULL;

        tempBuf = X265_MALLOC(uint8_t, depthBytes * 3);
        depthBuf = tempBuf;
        modeBuf = tempBuf + depthBytes;
        partSizes = tempBuf + 2 * depthBytes;
        if (m_param->rc.cuTree)
            cuQPBuf = X265_MALLOC(int8_t, depthBytes);

        X265_FREAD(depthBuf, sizeof(uint8_t), depthBytes, m_analysisFileIn, intraPic->depth);
        X265_FREAD(modeBuf, sizeof(uint8_t), depthBytes, m_analysisFileIn, intraPic->chromaModes);
        X265_FREAD(partSizes, sizeof(uint8_t), depthBytes, m_analysisFileIn, intraPic->partSizes);
        if (m_param->rc.cuTree) { X265_FREAD(cuQPBuf, sizeof(int8_t), depthBytes, m_analysisFileIn, intraPic->cuQPOff); }

        uint32_t count = 0;
        for (uint32_t d = 0; d < depthBytes; d++)
        {
            int bytes = analysis->numPartitions >> (depthBuf[d] * 2);
            int numCTUCopied = 1;
            if (!depthBuf[d]) //copy data of one 64x64 to four scaled 64x64 CTUs.
            {
                bytes /= 4;
                numCTUCopied = 4;
            }
            if (partSizes[d] == SIZE_NxN)
                partSizes[d] = SIZE_2Nx2N;
            if ((depthBuf[d] > 1 && m_param->maxCUSize == 64) || (depthBuf[d] && m_param->maxCUSize != 64))
                depthBuf[d]--;

            for (int numCTU = 0; numCTU < numCTUCopied; numCTU++)
            {
                memset(&(analysis->intraData)->depth[count], depthBuf[d], bytes);
                memset(&(analysis->intraData)->chromaModes[count], modeBuf[d], bytes);
                memset(&(analysis->intraData)->partSizes[count], partSizes[d], bytes);
                if (m_param->rc.cuTree)
                    memset(&(analysis->intraData)->cuQPOff[count], cuQPBuf[d], bytes);
                count += bytes;
                d += getCUIndex(&cuLoc, &count, bytes, 1);
            }
        }

        cuLoc.evenRowIndex = 0;
        cuLoc.oddRowIndex = m_param->num4x4Partitions * cuLoc.widthInCU;
        cuLoc.switchCondition = 0;
        uint8_t *tempLumaBuf = X265_MALLOC(uint8_t, analysis->numCUsInFrame * scaledNumPartition);
        X265_FREAD(tempLumaBuf, sizeof(uint8_t), analysis->numCUsInFrame * scaledNumPartition, m_analysisFileIn, intraPic->modes);
        uint32_t cnt = 0;
        for (uint32_t ctu32Idx = 0; ctu32Idx < analysis->numCUsInFrame * scaledNumPartition; ctu32Idx++)
        {
            memset(&(analysis->intraData)->modes[cnt], tempLumaBuf[ctu32Idx], factor);
            cnt += factor;
            ctu32Idx += getCUIndex(&cuLoc, &cnt, factor, 0);
        }
        X265_FREE(tempLumaBuf);
        if (m_param->rc.cuTree)
            X265_FREE(cuQPBuf);
        X265_FREE(tempBuf);
        consumedBytes += frameRecordSize;
    }

    else
    {
        uint32_t numDir = analysis->sliceType == X265_TYPE_P ? 1 : 2;
        uint32_t numPlanes = m_param->internalCsp == X265_CSP_I400 ? 1 : 3;
        X265_FREAD((WeightParam*)analysis->wt, sizeof(WeightParam), numPlanes * numDir, m_analysisFileIn, (picIn->analysisData.wt));
        if (m_param->analysisLoadReuseLevel < 2)
            return;

        uint8_t *tempBuf = NULL, *depthBuf = NULL, *modeBuf = NULL, *partSize = NULL, *mergeFlag = NULL;
        uint8_t *interDir = NULL, *chromaDir = NULL, *mvpIdx[2];
        MV* mv[2];
        int8_t* refIdx[2];
        int8_t* cuQPBuf = NULL;

        int numBuf = m_param->analysisLoadReuseLevel > 4 ? 4 : 2;
        bool bIntraInInter = false;
        if (m_param->analysisLoadReuseLevel == 10)
        {
            numBuf++;
            bIntraInInter = (analysis->sliceType == X265_TYPE_P || m_param->bIntraInBFrames);
            if (bIntraInInter) numBuf++;
        }

        tempBuf = X265_MALLOC(uint8_t, depthBytes * numBuf);
        depthBuf = tempBuf;
        modeBuf = tempBuf + depthBytes;
        if (m_param->rc.cuTree)
            cuQPBuf = X265_MALLOC(int8_t, depthBytes);

        X265_FREAD(depthBuf, sizeof(uint8_t), depthBytes, m_analysisFileIn, interPic->depth);
        X265_FREAD(modeBuf, sizeof(uint8_t), depthBytes, m_analysisFileIn, interPic->modes);
        if (m_param->rc.cuTree) { X265_FREAD(cuQPBuf, sizeof(int8_t), depthBytes, m_analysisFileIn, interPic->cuQPOff); }
        if (m_param->analysisLoadReuseLevel > 4)
        {
            partSize = modeBuf + depthBytes;
            mergeFlag = partSize + depthBytes;
            X265_FREAD(partSize, sizeof(uint8_t), depthBytes, m_analysisFileIn, interPic->partSize);
            X265_FREAD(mergeFlag, sizeof(uint8_t), depthBytes, m_analysisFileIn, interPic->mergeFlag);
            if (m_param->analysisLoadReuseLevel == 10)
            {
                interDir = mergeFlag + depthBytes;
                X265_FREAD(interDir, sizeof(uint8_t), depthBytes, m_analysisFileIn, interPic->interDir);
                if (bIntraInInter)
                {
                    chromaDir = interDir + depthBytes;
                    X265_FREAD(chromaDir, sizeof(uint8_t), depthBytes, m_analysisFileIn, intraPic->chromaModes);
                }
                for (uint32_t i = 0; i < numDir; i++)
                {
                    mvpIdx[i] = X265_MALLOC(uint8_t, depthBytes);
                    refIdx[i] = X265_MALLOC(int8_t, depthBytes);
                    mv[i] = X265_MALLOC(MV, depthBytes);
                    X265_FREAD(mvpIdx[i], sizeof(uint8_t), depthBytes, m_analysisFileIn, interPic->mvpIdx[i]);
                    X265_FREAD(refIdx[i], sizeof(int8_t), depthBytes, m_analysisFileIn, interPic->refIdx[i]);
                    X265_FREAD(mv[i], sizeof(MV), depthBytes, m_analysisFileIn, interPic->mv[i]);
                }
            }
        }

        uint32_t count = 0;
        cuLoc.switchCondition = 0;
        for (uint32_t d = 0; d < depthBytes; d++)
        {
            int bytes = analysis->numPartitions >> (depthBuf[d] * 2);
            bool isScaledMaxCUSize = false;
            int numCTUCopied = 1;
            int writeDepth = depthBuf[d];
            if (!depthBuf[d]) //copy data of one 64x64 to four scaled 64x64 CTUs.
            {
                isScaledMaxCUSize = true;
                bytes /= 4;
                numCTUCopied = 4;
            }
            if ((modeBuf[d] != MODE_INTRA && depthBuf[d] != 0) || (modeBuf[d] == MODE_INTRA && depthBuf[d] > 1))
                writeDepth--;

            for (int numCTU = 0; numCTU < numCTUCopied; numCTU++)
            {
                memset(&(analysis->interData)->depth[count], writeDepth, bytes);
                memset(&(analysis->interData)->modes[count], modeBuf[d], bytes);
                if (m_param->rc.cuTree)
                    memset(&(analysis->interData)->cuQPOff[count], cuQPBuf[d], bytes);
                if (m_param->analysisLoadReuseLevel == 10 && bIntraInInter)
                    memset(&(analysis->intraData)->chromaModes[count], chromaDir[d], bytes);

                if (m_param->analysisLoadReuseLevel > 4)
                {
                    puOrientation puOrient;
                    puOrient.init();
                    if (modeBuf[d] == MODE_INTRA && partSize[d] == SIZE_NxN)
                        partSize[d] = SIZE_2Nx2N;
                    int partitionSize = partSize[d];
                    if (isScaledMaxCUSize && partSize[d] != SIZE_2Nx2N)
                        partitionSize = getPuShape(&puOrient, partSize[d], numCTU);
                    memset(&(analysis->interData)->partSize[count], partitionSize, bytes);
                    int numPU = (modeBuf[d] == MODE_INTRA) ? 1 : nbPartsTable[(int)partSize[d]];
                    for (int pu = 0; pu < numPU; pu++)
                    {
                        if (!isScaledMaxCUSize && pu)
                            d++;
                        int restoreD = d;
                        /* Adjust d value when the current CTU takes data from 2nd PU */
                        if (puOrient.isRect || (puOrient.isAmp && partitionSize == SIZE_2Nx2N))
                        {
                            if ((numCTU > 1 && !puOrient.isVert) || ((numCTU % 2 == 1) && puOrient.isVert))
                                d++;
                        }
                        if (puOrient.isAmp && pu)
                            d++;

                        (analysis->interData)->mergeFlag[count + pu] = mergeFlag[d];
                        if (m_param->analysisLoadReuseLevel == 10)
                        {
                            (analysis->interData)->interDir[count + pu] = interDir[d];
                            MV mvCopy[2];
                            for (uint32_t i = 0; i < numDir; i++)
                            {
                                (analysis->interData)->mvpIdx[i][count + pu] = mvpIdx[i][d];
                                (analysis->interData)->refIdx[i][count + pu] = refIdx[i][d];
                                mvCopy[i].x = mv[i][d].x * (int32_t)m_param->scaleFactor;
                                mvCopy[i].y = mv[i][d].y * (int32_t)m_param->scaleFactor;
                                memcpy(&(analysis->interData)->mv[i][count + pu], &mvCopy[i], sizeof(MV));
                            }
                        }
                        d = restoreD; // Restore d value after copying each of the 4 64x64 CTUs

                        if (isScaledMaxCUSize && (puOrient.isRect || puOrient.isAmp))
                        {
                            /* Skip PU index when current CTU is a 2Nx2N */
                            if (partitionSize == SIZE_2Nx2N)
                                pu++;
                            /* Adjust d after completion of all 4 CTU copies */
                            if (numCTU == 3 && (pu == (numPU - 1)))
                                d++;
                        }
                    }
                }
                count += bytes;
                d += getCUIndex(&cuLoc, &count, bytes, 1);
            }
        }

        if (m_param->rc.cuTree)
            X265_FREE(cuQPBuf);
        X265_FREE(tempBuf);

        if (m_param->analysisLoadReuseLevel == 10)
        {
            for (uint32_t i = 0; i < numDir; i++)
            {
                X265_FREE(mvpIdx[i]);
                X265_FREE(refIdx[i]);
                X265_FREE(mv[i]);
            }
            if (bIntraInInter)
            {
                cuLoc.evenRowIndex = 0;
                cuLoc.oddRowIndex = m_param->num4x4Partitions * cuLoc.widthInCU;
                cuLoc.switchCondition = 0;
                uint8_t *tempLumaBuf = X265_MALLOC(uint8_t, analysis->numCUsInFrame * scaledNumPartition);
                X265_FREAD(tempLumaBuf, sizeof(uint8_t), analysis->numCUsInFrame * scaledNumPartition, m_analysisFileIn, intraPic->modes);
                uint32_t cnt = 0;
                for (uint32_t ctu32Idx = 0; ctu32Idx < analysis->numCUsInFrame * scaledNumPartition; ctu32Idx++)
                {
                    memset(&(analysis->intraData)->modes[cnt], tempLumaBuf[ctu32Idx], factor);
                    cnt += factor;
                    ctu32Idx += getCUIndex(&cuLoc, &cnt, factor, 0);
                }
                X265_FREE(tempLumaBuf);
            }
        }
        else
            X265_FREAD((analysis->interData)->ref, sizeof(int32_t), analysis->numCUsInFrame * X265_MAX_PRED_MODE_PER_CTU * numDir, m_analysisFileIn, interPic->ref);

        consumedBytes += frameRecordSize;
        if (numDir == 1)
            totalConsumedBytes = consumedBytes;
    }

    /* Restore to the current encode's numPartitions and numCUsInFrame */
    analysis->numPartitions = m_param->num4x4Partitions;
    analysis->numCUsInFrame = cuLoc.heightInCU * cuLoc.widthInCU;
    analysis->numCuInHeight = cuLoc.heightInCU;
#undef X265_FREAD
}


int Encoder::validateAnalysisData(x265_analysis_validate* saveParam, int writeFlag)
{
#define X265_PARAM_VALIDATE(analysisParam, size, bytes, param, errorMsg)\
    if(!writeFlag)\
    {\
        fileOffset = m_analysisFileIn;\
        if ((!m_param->bUseAnalysisFile && analysisParam != (int)*param) || \
            (m_param->bUseAnalysisFile && (fread(&readValue, size, bytes, fileOffset) != bytes || (readValue != (int)*param))))\
        {\
            x265_log(NULL, X265_LOG_ERROR, "Error reading analysis data. Incompatible option : <%s> \n", #errorMsg);\
            m_aborted = true;\
            return -1;\
        }\
    }\
    if(writeFlag)\
    {\
        fileOffset = m_analysisFileOut;\
        if(!m_param->bUseAnalysisFile)\
            analysisParam = *param;\
        else if(fwrite(param, size, bytes, fileOffset) < bytes)\
        {\
            x265_log(NULL, X265_LOG_ERROR, "Error writing analysis data\n"); \
            m_aborted = true;\
            return -1; \
        }\
    }\
    count++;

#define X265_FREAD(val, size, readSize, fileOffset, src)\
    if (!m_param->bUseAnalysisFile)\
    {\
        memcpy(val, src, (size * readSize));\
    }\
    else if (fread(val, size, readSize, fileOffset) != readSize)\
    {\
        x265_log(NULL, X265_LOG_ERROR, "Error reading analysis data\n");\
        m_aborted = true;\
        return -1;\
    }\
    count++;

    FILE*     fileOffset = NULL;
    int       readValue = 0;
    int       count = 0;

    if (m_param->bUseAnalysisFile && writeFlag)
    {
        X265_PARAM_VALIDATE(saveParam->rightOffset, sizeof(int), 1, &m_conformanceWindow.rightOffset, right-offset);
        X265_PARAM_VALIDATE(saveParam->bottomOffset, sizeof(int), 1, &m_conformanceWindow.bottomOffset, bottom-offset);
    }

    X265_PARAM_VALIDATE(saveParam->intraRefresh, sizeof(int), 1, &m_param->bIntraRefresh, intra-refresh);
    X265_PARAM_VALIDATE(saveParam->maxNumReferences, sizeof(int), 1, &m_param->maxNumReferences, ref);
    X265_PARAM_VALIDATE(saveParam->keyframeMax, sizeof(int), 1, &m_param->keyframeMax, keyint);
    X265_PARAM_VALIDATE(saveParam->keyframeMin, sizeof(int), 1, &m_param->keyframeMin, min-keyint);
    X265_PARAM_VALIDATE(saveParam->openGOP, sizeof(int), 1, &m_param->bOpenGOP, open-gop);
    X265_PARAM_VALIDATE(saveParam->bframes, sizeof(int), 1, &m_param->bframes, bframes);
    X265_PARAM_VALIDATE(saveParam->bPyramid, sizeof(int), 1, &m_param->bBPyramid, bPyramid);
    X265_PARAM_VALIDATE(saveParam->minCUSize, sizeof(int), 1, &m_param->minCUSize, min - cu - size);
    X265_PARAM_VALIDATE(saveParam->lookaheadDepth, sizeof(int), 1, &m_param->lookaheadDepth, rc - lookahead);
    X265_PARAM_VALIDATE(saveParam->chunkStart, sizeof(int), 1, &m_param->chunkStart, chunk-start);
    X265_PARAM_VALIDATE(saveParam->chunkEnd, sizeof(int), 1, &m_param->chunkEnd, chunk-end);
    X265_PARAM_VALIDATE(saveParam->ctuDistortionRefine, sizeof(int), 1, &m_param->ctuDistortionRefine, ctu - distortion);
    X265_PARAM_VALIDATE(saveParam->frameDuplication, sizeof(int), 1, &m_param->bEnableFrameDuplication, frame - dup);

    int sourceHeight, sourceWidth;
    if (writeFlag)
    {
        X265_PARAM_VALIDATE(saveParam->analysisReuseLevel, sizeof(int), 1, &m_param->analysisSaveReuseLevel, analysis - save - reuse - level);
        X265_PARAM_VALIDATE(saveParam->cuTree, sizeof(int), 1, &m_param->rc.cuTree, cutree-offset);
        sourceHeight = m_param->sourceHeight - m_conformanceWindow.bottomOffset;
        sourceWidth = m_param->sourceWidth - m_conformanceWindow.rightOffset;
        X265_PARAM_VALIDATE(saveParam->sourceWidth, sizeof(int), 1, &sourceWidth, res-width);
        X265_PARAM_VALIDATE(saveParam->sourceHeight, sizeof(int), 1, &sourceHeight, res-height);
        X265_PARAM_VALIDATE(saveParam->maxCUSize, sizeof(int), 1, &m_param->maxCUSize, ctu);
    }
    else
    {
        fileOffset = m_analysisFileIn;

        int saveLevel = 0;
        bool isIncompatibleReuseLevel = false;
        int loadLevel = m_param->analysisLoadReuseLevel;

        X265_FREAD(&saveLevel, sizeof(int), 1, m_analysisFileIn, &(saveParam->analysisReuseLevel));
        
        if (loadLevel == 10 && saveLevel != 10)
            isIncompatibleReuseLevel = true;
        else if (((loadLevel >= 7) && (loadLevel <= 9)) && ((saveLevel < 7) || (saveLevel > 9)))
            isIncompatibleReuseLevel = true;
        else if ((loadLevel == 5 || loadLevel == 6) && ((saveLevel != 5) && (saveLevel != 6)))
            isIncompatibleReuseLevel = true;
        else if ((loadLevel >= 2 && loadLevel <= 4) && (saveLevel < 2 || saveLevel > 6))
            isIncompatibleReuseLevel = true;
        else if (!saveLevel)
            isIncompatibleReuseLevel = true;

        if (isIncompatibleReuseLevel)
        {
            x265_log(NULL, X265_LOG_ERROR, "Error reading analysis data. Incompatible reuse-levels.\n");
            m_aborted = true;
            return -1;
        }

        int bcutree;
        X265_FREAD(&bcutree, sizeof(int), 1, m_analysisFileIn, &(saveParam->cuTree));
        if (loadLevel >= 2 && m_param->rc.cuTree && (!bcutree || saveLevel < 2))
        {
            x265_log(NULL, X265_LOG_ERROR, "Error reading cu-tree info. Disabling cutree offsets. \n");
            m_param->rc.cuTree = 0;
            return -1;
        }

        bool error = false;
        int curSourceHeight = m_param->sourceHeight - m_conformanceWindow.bottomOffset;
        int curSourceWidth = m_param->sourceWidth - m_conformanceWindow.rightOffset;
      
        X265_FREAD(&sourceWidth, sizeof(int), 1, m_analysisFileIn, &(saveParam->sourceWidth));
        X265_FREAD(&sourceHeight, sizeof(int), 1, m_analysisFileIn, &(saveParam->sourceHeight));
        X265_FREAD(&readValue, sizeof(int), 1, m_analysisFileIn, &(saveParam->maxCUSize));

        bool isScaledRes = (2 * sourceHeight == curSourceHeight) && (2 * sourceWidth == curSourceWidth);
        if (!isScaledRes && (m_param->analysisLoadReuseLevel > 1) && (sourceHeight != curSourceHeight
            || sourceWidth != curSourceWidth || readValue != (int)m_param->maxCUSize || m_param->scaleFactor))
            error = true;
        else if (isScaledRes && !m_param->scaleFactor)
            error = true;
        else if (isScaledRes && (int)m_param->maxCUSize == readValue)
            m_saveCTUSize = 1;
        else if (isScaledRes && (g_log2Size[m_param->maxCUSize] - g_log2Size[readValue]) != 1)
            error = true;

        if (error)
        {
            x265_log(NULL, X265_LOG_ERROR, "Error reading analysis data. Incompatible option : <input-res / scale-factor / ctu> \n");
            m_aborted = true;
            return -1;
        }
    }
    return (count * sizeof(int));

#undef X265_FREAD
#undef X265_PARAM_VALIDATE
}

/* Toggle between two consecutive CTU rows. The save's CTU is copied
twice consecutively in the first and second CTU row of load*/

int Encoder::getCUIndex(cuLocation* cuLoc, uint32_t* count, int bytes, int flag)
{
    int index = 0;
    cuLoc->switchCondition += bytes;
    int isBoundaryW = (*count % (m_param->num4x4Partitions * cuLoc->widthInCU) == 0);

    /* Width boundary case :
    Skip to appropriate index when out of boundary cases occur
    Out of boundary may occur when the out of bound pixels along
    the width in low resoultion is greater than half of the maxCUSize */
    if (cuLoc->skipWidth && isBoundaryW)
    {
        if (flag)
            index++;
        else
        {
            /* Number of 4x4 blocks in out of bound region */
            int outOfBound = m_param->maxCUSize / 2;
            uint32_t sum = (uint32_t)pow((outOfBound >> 2), 2);
            index += sum;
        }
        cuLoc->switchCondition += m_param->num4x4Partitions;
    }

    /* Completed writing 2 CTUs - move to the last remembered index of the next CTU row*/
    if (cuLoc->switchCondition == 2 * m_param->num4x4Partitions)
    {
        if (isBoundaryW)
            cuLoc->evenRowIndex = *count + (m_param->num4x4Partitions * cuLoc->widthInCU); // end of row - skip to the next even row
        else
            cuLoc->evenRowIndex = *count;
        *count = cuLoc->oddRowIndex;

        /* Height boundary case :
        Skip to appropriate index when out of boundary cases occur
        Out of boundary may occur when the out of bound pixels along
        the height in low resoultion is greater than half of the maxCUSize */
        int isBoundaryH = (*count >= (m_param->num4x4Partitions * cuLoc->heightInCU * cuLoc->widthInCU));
        if (cuLoc->skipHeight && isBoundaryH)
        {
            if (flag)
                index += 2;
            else
            {
                int outOfBound = m_param->maxCUSize / 2;
                uint32_t sum = (uint32_t)(2 * pow((abs(outOfBound) >> 2), 2));
                index += sum;
            }
            *count = cuLoc->evenRowIndex;
            cuLoc->switchCondition = 0;
        }
    }
    /* Completed writing 4 CTUs - move to the last remembered index of
    the previous CTU row to copy the next save CTU's data*/
    else if (cuLoc->switchCondition == 4 * m_param->num4x4Partitions)
    {
        if (isBoundaryW)
            cuLoc->oddRowIndex = *count + (m_param->num4x4Partitions * cuLoc->widthInCU); // end of row - skip to the next odd row
        else
            cuLoc->oddRowIndex = *count;
        *count = cuLoc->evenRowIndex;
        cuLoc->switchCondition = 0;
    }
    return index;
}

/*      save                        load
                       CTU0    CTU1    CTU2    CTU3
        2NxN          2Nx2N   2Nx2N   2Nx2N   2Nx2N
        NX2N          2Nx2N   2Nx2N   2Nx2N   2Nx2N
        2NxnU          2NxN    2NxN   2Nx2N   2Nx2N
        2NxnD         2Nx2N   2Nx2N    2NxN    2NxN
        nLx2N          Nx2N   2Nx2N    Nx2N   2Nx2N
        nRx2N         2Nx2N    Nx2N    2Nx2N   Nx2N
*/
int Encoder::getPuShape(puOrientation* puOrient, int partSize, int numCTU)
{
    puOrient->isRect = true;
    if (partSize == SIZE_Nx2N)
        puOrient->isVert = true;
    if (partSize >= SIZE_2NxnU) // All AMP modes
    {
        puOrient->isAmp = true;
        puOrient->isRect = false;
        if (partSize == SIZE_2NxnD && numCTU > 1)
            return SIZE_2NxN;
        else if (partSize == SIZE_2NxnU && numCTU < 2)
            return SIZE_2NxN;
        else if (partSize == SIZE_nLx2N)
        {
            puOrient->isVert = true;
            if (!(numCTU % 2))
                return SIZE_Nx2N;
        }
        else if (partSize == SIZE_nRx2N)
        {
            puOrient->isVert = true;
            if (numCTU % 2)
                return SIZE_Nx2N;
        }
    }
    return SIZE_2Nx2N;
}
void Encoder::computeDistortionOffset(x265_analysis_data* analysis)
{
    x265_analysis_distortion_data *distortionData = analysis->distortionData;

    double sum = 0.0, sqrSum = 0.0;
    for (uint32_t i = 0; i < analysis->numCUsInFrame; ++i)
    {
        distortionData->scaledDistortion[i] = X265_LOG2(X265_MAX(distortionData->ctuDistortion[i], 1));
        sum += distortionData->scaledDistortion[i];
        sqrSum += distortionData->scaledDistortion[i] * distortionData->scaledDistortion[i];
    }
    double avg = sum / analysis->numCUsInFrame;
    distortionData->sdDistortion = pow(((sqrSum / analysis->numCUsInFrame) - (avg * avg)), 0.5);
    distortionData->averageDistortion = avg;
    distortionData->highDistortionCtuCount = distortionData->lowDistortionCtuCount = 0;
    for (uint32_t i = 0; i < analysis->numCUsInFrame; ++i)
    {
        distortionData->threshold[i] = distortionData->scaledDistortion[i] / distortionData->averageDistortion;
        distortionData->offset[i] = (distortionData->averageDistortion - distortionData->scaledDistortion[i]) / distortionData->sdDistortion;
        if (distortionData->threshold[i] < 0.9 && distortionData->offset[i] >= 1)
            distortionData->lowDistortionCtuCount++;
        else if (distortionData->threshold[i] > 1.1 && distortionData->offset[i] <= -1)
            distortionData->highDistortionCtuCount++;
    }
}
void Encoder::readAnalysisFile(x265_analysis_data* analysis, int curPoc, int sliceType)
{

#define X265_FREAD(val, size, readSize, fileOffset)\
    if (fread(val, size, readSize, fileOffset) != readSize)\
    {\
    x265_log(NULL, X265_LOG_ERROR, "Error reading analysis 2 pass data\n"); \
    x265_alloc_analysis_data(m_param, analysis); \
    m_aborted = true; \
    return; \
}\

    uint32_t depthBytes = 0;
    int poc; uint32_t frameRecordSize;
    X265_FREAD(&frameRecordSize, sizeof(uint32_t), 1, m_analysisFileIn);
    X265_FREAD(&depthBytes, sizeof(uint32_t), 1, m_analysisFileIn);
    X265_FREAD(&poc, sizeof(int), 1, m_analysisFileIn);

    if (poc != curPoc || feof(m_analysisFileIn))
    {
        x265_log(NULL, X265_LOG_WARNING, "Error reading analysis 2 pass data: Cannot find POC %d\n", curPoc);
        x265_free_analysis_data(m_param, analysis);
        return;
    }
    /* Now arrived at the right frame, read the record */
    analysis->frameRecordSize = frameRecordSize;
    uint8_t* tempBuf = NULL, *depthBuf = NULL;
    X265_FREAD((analysis->distortionData)->ctuDistortion, sizeof(sse_t), analysis->numCUsInFrame, m_analysisFileIn);
    tempBuf = X265_MALLOC(uint8_t, depthBytes);
    X265_FREAD(tempBuf, sizeof(uint8_t), depthBytes, m_analysisFileIn);
    depthBuf = tempBuf;
    x265_analysis_data *analysisData = (x265_analysis_data*)analysis;
    x265_analysis_intra_data *intraData = analysisData->intraData;
    x265_analysis_inter_data *interData = analysisData->interData;

    computeDistortionOffset(analysis);
    size_t count = 0;
    for (uint32_t d = 0; d < depthBytes; d++)
    {
        int bytes = analysis->numPartitions >> (depthBuf[d] * 2);
        if (IS_X265_TYPE_I(sliceType))
            memset(&intraData->depth[count], depthBuf[d], bytes);
        else
            memset(&interData->depth[count], depthBuf[d], bytes);
        count += bytes;
    }


    if (!IS_X265_TYPE_I(sliceType))
    {
        MV *tempMVBuf[2], *MVBuf[2];
        int32_t *tempRefBuf, *refBuf;
        uint8_t *tempMvpBuf[2], *mvpBuf[2];
        uint8_t* tempModeBuf = NULL, *modeBuf = NULL;
        int numDir = sliceType == X265_TYPE_P ? 1 : 2;
        tempRefBuf = X265_MALLOC(int32_t, numDir * depthBytes);

        for (int i = 0; i < numDir; i++)
        {
            tempMVBuf[i] = X265_MALLOC(MV, depthBytes);
            X265_FREAD(tempMVBuf[i], sizeof(MV), depthBytes, m_analysisFileIn);
            MVBuf[i] = tempMVBuf[i];
            tempMvpBuf[i] = X265_MALLOC(uint8_t, depthBytes);
            X265_FREAD(tempMvpBuf[i], sizeof(uint8_t), depthBytes, m_analysisFileIn);
            mvpBuf[i] = tempMvpBuf[i];
            X265_FREAD(&tempRefBuf[i*depthBytes], sizeof(int32_t), depthBytes, m_analysisFileIn);
        }
        refBuf = tempRefBuf;
        tempModeBuf = X265_MALLOC(uint8_t, depthBytes);
        X265_FREAD(tempModeBuf, sizeof(uint8_t), depthBytes, m_analysisFileIn);
        modeBuf = tempModeBuf;
        
        count = 0;

        for (uint32_t d = 0; d < depthBytes; d++)
        {
            size_t bytes = analysis->numPartitions >> (depthBuf[d] * 2);
            for (int i = 0; i < numDir; i++)
            {
                int32_t* ref = &(analysis->interData)->ref[i * analysis->numPartitions * analysis->numCUsInFrame];
                for (size_t j = count, k = 0; k < bytes; j++, k++)
                {
                    memcpy(&(analysis->interData)->mv[i][j], MVBuf[i] + d, sizeof(MV));
                    memcpy(&(analysis->interData)->mvpIdx[i][j], mvpBuf[i] + d, sizeof(uint8_t));
                    memcpy(&ref[j], refBuf + (i * depthBytes) + d, sizeof(int32_t));
                }
            }
            memset(&(analysis->interData)->modes[count], modeBuf[d], bytes);
            count += bytes;
        }

        for (int i = 0; i < numDir; i++)
        {
            X265_FREE(tempMVBuf[i]);
            X265_FREE(tempMvpBuf[i]);
        }
        X265_FREE(tempRefBuf);
        X265_FREE(tempModeBuf);
    }
    X265_FREE(tempBuf);

#undef X265_FREAD
}

void Encoder::copyDistortionData(x265_analysis_data* analysis, FrameData &curEncData)
{
    for (uint32_t cuAddr = 0; cuAddr < analysis->numCUsInFrame; cuAddr++)
    {
        uint8_t depth = 0;
        CUData* ctu = curEncData.getPicCTU(cuAddr);
        x265_analysis_distortion_data *distortionData = (x265_analysis_distortion_data *)analysis->distortionData;
        distortionData->ctuDistortion[cuAddr] = 0;
        for (uint32_t absPartIdx = 0; absPartIdx < ctu->m_numPartitions;)
        {
            depth = ctu->m_cuDepth[absPartIdx];
            distortionData->ctuDistortion[cuAddr] += ctu->m_distortion[absPartIdx];
            absPartIdx += ctu->m_numPartitions >> (depth * 2);
        }
    }
}

void Encoder::writeAnalysisFile(x265_analysis_data* analysis, FrameData &curEncData)
{

#define X265_FWRITE(val, size, writeSize, fileOffset)\
    if (fwrite(val, size, writeSize, fileOffset) < writeSize)\
    {\
        x265_log(NULL, X265_LOG_ERROR, "Error writing analysis data\n");\
        x265_free_analysis_data(m_param, analysis);\
        m_aborted = true;\
        return;\
    }\

    uint32_t depthBytes = 0;
    uint32_t numDir, numPlanes;
    bool bIntraInInter = false;

    if (!analysis->poc)
    {
        if (validateAnalysisData(&analysis->saveParam, 1) == -1)
        {
            m_aborted = true;
            return;
        }
    }

    /* calculate frameRecordSize */
    analysis->frameRecordSize = sizeof(analysis->frameRecordSize) + sizeof(depthBytes) + sizeof(analysis->poc) + sizeof(analysis->sliceType) +
                      sizeof(analysis->numCUsInFrame) + sizeof(analysis->numPartitions) + sizeof(analysis->bScenecut) + sizeof(analysis->satdCost);
    if (m_param->bHistBasedSceneCut)
    {
        analysis->frameRecordSize += sizeof(analysis->edgeHist);
        analysis->frameRecordSize += sizeof(int32_t) * HISTOGRAM_BINS;
        if (m_param->internalCsp != X265_CSP_I400)
        {
            analysis->frameRecordSize += sizeof(int32_t) * HISTOGRAM_BINS;
            analysis->frameRecordSize += sizeof(int32_t) * HISTOGRAM_BINS;
        }
    }

    if (analysis->sliceType > X265_TYPE_I)
    {
        numDir = (analysis->sliceType == X265_TYPE_P) ? 1 : 2;
        numPlanes = m_param->internalCsp == X265_CSP_I400 ? 1 : 3;
        analysis->frameRecordSize += sizeof(WeightParam) * numPlanes * numDir;
    }

    if (m_param->ctuDistortionRefine == CTU_DISTORTION_INTERNAL)
    {
        copyDistortionData(analysis, curEncData);
        analysis->frameRecordSize += analysis->numCUsInFrame * sizeof(sse_t);
    }

    if (m_param->analysisSaveReuseLevel > 1)
    {

        if (analysis->sliceType == X265_TYPE_IDR || analysis->sliceType == X265_TYPE_I)
        {
            for (uint32_t cuAddr = 0; cuAddr < analysis->numCUsInFrame; cuAddr++)
            {
                uint8_t depth = 0;
                uint8_t mode = 0;
                uint8_t partSize = 0;

                CUData* ctu = curEncData.getPicCTU(cuAddr);
                x265_analysis_intra_data* intraDataCTU = analysis->intraData;
                int baseQP = (int)(ctu->m_encData->m_cuStat[cuAddr].baseQp + 0.5);

                for (uint32_t absPartIdx = 0; absPartIdx < ctu->m_numPartitions; depthBytes++)
                {
                    depth = ctu->m_cuDepth[absPartIdx];
                    intraDataCTU->depth[depthBytes] = depth;

                    mode = ctu->m_chromaIntraDir[absPartIdx];
                    intraDataCTU->chromaModes[depthBytes] = mode;

                    partSize = ctu->m_partSize[absPartIdx];
                    intraDataCTU->partSizes[depthBytes] = partSize;

                    if (m_param->rc.cuTree)
                        intraDataCTU->cuQPOff[depthBytes] = (int8_t)(ctu->m_qpAnalysis[absPartIdx] - baseQP);
                    absPartIdx += ctu->m_numPartitions >> (depth * 2);
                }
                memcpy(&intraDataCTU->modes[ctu->m_cuAddr * ctu->m_numPartitions], ctu->m_lumaIntraDir, sizeof(uint8_t)* ctu->m_numPartitions);
            }
        }
        else
        {
            bIntraInInter = (analysis->sliceType == X265_TYPE_P || m_param->bIntraInBFrames);
            for (uint32_t cuAddr = 0; cuAddr < analysis->numCUsInFrame; cuAddr++)
            {
                uint8_t depth = 0;
                uint8_t predMode = 0;
                uint8_t partSize = 0;

                CUData* ctu = curEncData.getPicCTU(cuAddr);
                x265_analysis_inter_data* interDataCTU = analysis->interData;
                x265_analysis_intra_data* intraDataCTU = analysis->intraData;
                int baseQP = (int)(ctu->m_encData->m_cuStat[cuAddr].baseQp + 0.5);

                for (uint32_t absPartIdx = 0; absPartIdx < ctu->m_numPartitions; depthBytes++)
                {
                    depth = ctu->m_cuDepth[absPartIdx];
                    interDataCTU->depth[depthBytes] = depth;

                    predMode = ctu->m_predMode[absPartIdx];
                    if (m_param->analysisSaveReuseLevel != 10 && ctu->m_refIdx[1][absPartIdx] != -1)
                        predMode = 4; // used as indicator if the block is coded as bidir

                    interDataCTU->modes[depthBytes] = predMode;
                    if (m_param->rc.cuTree)
                        interDataCTU->cuQPOff[depthBytes] = (int8_t)(ctu->m_qpAnalysis[absPartIdx] - baseQP);

                    if (m_param->analysisSaveReuseLevel > 4)
                    {
                        partSize = ctu->m_partSize[absPartIdx];
                        interDataCTU->partSize[depthBytes] = partSize;

                        /* Store per PU data */
                        uint32_t numPU = (predMode == MODE_INTRA) ? 1 : nbPartsTable[(int)partSize];
                        for (uint32_t puIdx = 0; puIdx < numPU; puIdx++)
                        {
                            uint32_t puabsPartIdx = ctu->getPUOffset(puIdx, absPartIdx) + absPartIdx;
                            if (puIdx) depthBytes++;
                            interDataCTU->mergeFlag[depthBytes] = ctu->m_mergeFlag[puabsPartIdx];

                            if (m_param->analysisSaveReuseLevel == 10)
                            {
                                interDataCTU->interDir[depthBytes] = ctu->m_interDir[puabsPartIdx];
                                for (uint32_t dir = 0; dir < numDir; dir++)
                                {
                                    interDataCTU->mvpIdx[dir][depthBytes] = ctu->m_mvpIdx[dir][puabsPartIdx];
                                    interDataCTU->refIdx[dir][depthBytes] = ctu->m_refIdx[dir][puabsPartIdx];
                                    interDataCTU->mv[dir][depthBytes].word = ctu->m_mv[dir][puabsPartIdx].word;
                                }
                            }
                        }
                        if (m_param->analysisSaveReuseLevel == 10 && bIntraInInter)
                            intraDataCTU->chromaModes[depthBytes] = ctu->m_chromaIntraDir[absPartIdx];
                    }
                    absPartIdx += ctu->m_numPartitions >> (depth * 2);
                }
                if (m_param->analysisSaveReuseLevel == 10 && bIntraInInter)
                    memcpy(&intraDataCTU->modes[ctu->m_cuAddr * ctu->m_numPartitions], ctu->m_lumaIntraDir, sizeof(uint8_t)* ctu->m_numPartitions);
            }
        }

        if ((analysis->sliceType == X265_TYPE_IDR || analysis->sliceType == X265_TYPE_I) && m_param->rc.cuTree)
            analysis->frameRecordSize += sizeof(uint8_t)* analysis->numCUsInFrame * analysis->numPartitions + depthBytes * 3 + (sizeof(int8_t) * depthBytes);
        else if (analysis->sliceType == X265_TYPE_IDR || analysis->sliceType == X265_TYPE_I)
            analysis->frameRecordSize += sizeof(uint8_t)* analysis->numCUsInFrame * analysis->numPartitions + depthBytes * 3;
        else
        {
            /* Add sizeof depth, modes, partSize, cuQPOffset, mergeFlag */
            analysis->frameRecordSize += depthBytes * 2;
            if (m_param->rc.cuTree)
            analysis->frameRecordSize += (sizeof(int8_t) * depthBytes);
            if (m_param->analysisSaveReuseLevel > 4)
                analysis->frameRecordSize += (depthBytes * 2);

            if (m_param->analysisSaveReuseLevel == 10)
            {
                /* Add Size of interDir, mvpIdx, refIdx, mv, luma and chroma modes */
                analysis->frameRecordSize += depthBytes;
                analysis->frameRecordSize += sizeof(uint8_t)* depthBytes * numDir;
                analysis->frameRecordSize += sizeof(int8_t)* depthBytes * numDir;
                analysis->frameRecordSize += sizeof(MV)* depthBytes * numDir;
                if (bIntraInInter)
                    analysis->frameRecordSize += sizeof(uint8_t)* analysis->numCUsInFrame * analysis->numPartitions + depthBytes;
            }
            else
                analysis->frameRecordSize += sizeof(int32_t)* analysis->numCUsInFrame * X265_MAX_PRED_MODE_PER_CTU * numDir;
        }
        analysis->depthBytes = depthBytes;
    }

    if (!m_param->bUseAnalysisFile)
        return;

    X265_FWRITE(&analysis->frameRecordSize, sizeof(uint32_t), 1, m_analysisFileOut);
    X265_FWRITE(&depthBytes, sizeof(uint32_t), 1, m_analysisFileOut);
    X265_FWRITE(&analysis->poc, sizeof(int), 1, m_analysisFileOut);
    X265_FWRITE(&analysis->sliceType, sizeof(int), 1, m_analysisFileOut);
    X265_FWRITE(&analysis->bScenecut, sizeof(int), 1, m_analysisFileOut);
    if (m_param->bHistBasedSceneCut)
    {
        X265_FWRITE(&analysis->edgeHist, sizeof(int32_t), EDGE_BINS, m_analysisFileOut);
        X265_FWRITE(&analysis->yuvHist[0], sizeof(int32_t), HISTOGRAM_BINS, m_analysisFileOut);
        if (m_param->internalCsp != X265_CSP_I400)
        {
            X265_FWRITE(&analysis->yuvHist[1], sizeof(int32_t), HISTOGRAM_BINS, m_analysisFileOut);
            X265_FWRITE(&analysis->yuvHist[2], sizeof(int32_t), HISTOGRAM_BINS, m_analysisFileOut);
        }
    }

    X265_FWRITE(&analysis->satdCost, sizeof(int64_t), 1, m_analysisFileOut);
    X265_FWRITE(&analysis->numCUsInFrame, sizeof(int), 1, m_analysisFileOut);
    X265_FWRITE(&analysis->numPartitions, sizeof(int), 1, m_analysisFileOut);
    if (m_param->ctuDistortionRefine == CTU_DISTORTION_INTERNAL)
        X265_FWRITE((analysis->distortionData)->ctuDistortion, sizeof(sse_t), analysis->numCUsInFrame, m_analysisFileOut);
    if (analysis->sliceType > X265_TYPE_I)
        X265_FWRITE((WeightParam*)analysis->wt, sizeof(WeightParam), numPlanes * numDir, m_analysisFileOut);

    if (m_param->analysisSaveReuseLevel < 2)
        return;

    if (analysis->sliceType == X265_TYPE_IDR || analysis->sliceType == X265_TYPE_I)
    {
        X265_FWRITE((analysis->intraData)->depth, sizeof(uint8_t), depthBytes, m_analysisFileOut);
        X265_FWRITE((analysis->intraData)->chromaModes, sizeof(uint8_t), depthBytes, m_analysisFileOut);
        X265_FWRITE((analysis->intraData)->partSizes, sizeof(char), depthBytes, m_analysisFileOut);
        if (m_param->rc.cuTree)
            X265_FWRITE((analysis->intraData)->cuQPOff, sizeof(int8_t), depthBytes, m_analysisFileOut);
        X265_FWRITE((analysis->intraData)->modes, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFileOut);
    }
    else
    {
        X265_FWRITE((analysis->interData)->depth, sizeof(uint8_t), depthBytes, m_analysisFileOut);
        X265_FWRITE((analysis->interData)->modes, sizeof(uint8_t), depthBytes, m_analysisFileOut);
        if (m_param->rc.cuTree)
            X265_FWRITE((analysis->interData)->cuQPOff, sizeof(int8_t), depthBytes, m_analysisFileOut);
        if (m_param->analysisSaveReuseLevel > 4)
        {
            X265_FWRITE((analysis->interData)->partSize, sizeof(uint8_t), depthBytes, m_analysisFileOut);
            X265_FWRITE((analysis->interData)->mergeFlag, sizeof(uint8_t), depthBytes, m_analysisFileOut);
            if (m_param->analysisSaveReuseLevel == 10)
            {
                X265_FWRITE((analysis->interData)->interDir, sizeof(uint8_t), depthBytes, m_analysisFileOut);
                if (bIntraInInter) X265_FWRITE((analysis->intraData)->chromaModes, sizeof(uint8_t), depthBytes, m_analysisFileOut);
                for (uint32_t dir = 0; dir < numDir; dir++)
                {
                    X265_FWRITE((analysis->interData)->mvpIdx[dir], sizeof(uint8_t), depthBytes, m_analysisFileOut);
                    X265_FWRITE((analysis->interData)->refIdx[dir], sizeof(int8_t), depthBytes, m_analysisFileOut);
                    X265_FWRITE((analysis->interData)->mv[dir], sizeof(MV), depthBytes, m_analysisFileOut);
                }
                if (bIntraInInter)
                    X265_FWRITE((analysis->intraData)->modes, sizeof(uint8_t), analysis->numCUsInFrame * analysis->numPartitions, m_analysisFileOut);
            }
        }
        if (m_param->analysisSaveReuseLevel != 10)
            X265_FWRITE((analysis->interData)->ref, sizeof(int32_t), analysis->numCUsInFrame * X265_MAX_PRED_MODE_PER_CTU * numDir, m_analysisFileOut);

    }
#undef X265_FWRITE
}

void Encoder::writeAnalysisFileRefine(x265_analysis_data* analysis, FrameData &curEncData)
{
#define X265_FWRITE(val, size, writeSize, fileOffset)\
    if (fwrite(val, size, writeSize, fileOffset) < writeSize)\
    {\
    x265_log(NULL, X265_LOG_ERROR, "Error writing analysis 2 pass data\n"); \
    x265_free_analysis_data(m_param, analysis); \
    m_aborted = true; \
    return; \
}\

    uint32_t depthBytes = 0;
    x265_analysis_data *analysisData = (x265_analysis_data*)analysis;
    x265_analysis_intra_data *intraData = analysisData->intraData;
    x265_analysis_inter_data *interData = analysisData->interData;
    x265_analysis_distortion_data *distortionData = analysisData->distortionData;

    copyDistortionData(analysis, curEncData);

    if (curEncData.m_slice->m_sliceType == I_SLICE)
    {
        for (uint32_t cuAddr = 0; cuAddr < analysis->numCUsInFrame; cuAddr++)
        {
            uint8_t depth = 0;
            CUData* ctu = curEncData.getPicCTU(cuAddr);
            for (uint32_t absPartIdx = 0; absPartIdx < ctu->m_numPartitions; depthBytes++)
            {
                depth = ctu->m_cuDepth[absPartIdx];
                intraData->depth[depthBytes] = depth;
                absPartIdx += ctu->m_numPartitions >> (depth * 2);
            }
        }
    }

    else
    {
        int32_t* ref[2];
        ref[0] = (analysis->interData)->ref;
        ref[1] = &(analysis->interData)->ref[analysis->numPartitions * analysis->numCUsInFrame];
        depthBytes = 0;
        for (uint32_t cuAddr = 0; cuAddr < analysis->numCUsInFrame; cuAddr++)
        {
            uint8_t depth = 0;
            uint8_t predMode = 0;

            CUData* ctu = curEncData.getPicCTU(cuAddr);
            for (uint32_t absPartIdx = 0; absPartIdx < ctu->m_numPartitions; depthBytes++)
            {
                depth = ctu->m_cuDepth[absPartIdx];
                interData->depth[depthBytes] = depth;
                interData->mv[0][depthBytes].word = ctu->m_mv[0][absPartIdx].word;
                interData->mvpIdx[0][depthBytes] = ctu->m_mvpIdx[0][absPartIdx];
                ref[0][depthBytes] = ctu->m_refIdx[0][absPartIdx];
                predMode = ctu->m_predMode[absPartIdx];
                if (ctu->m_refIdx[1][absPartIdx] != -1)
                {
                    interData->mv[1][depthBytes].word = ctu->m_mv[1][absPartIdx].word;
                    interData->mvpIdx[1][depthBytes] = ctu->m_mvpIdx[1][absPartIdx];
                    ref[1][depthBytes] = ctu->m_refIdx[1][absPartIdx];
                    predMode = 4; // used as indiacator if the block is coded as bidir
                }
                interData->modes[depthBytes] = predMode;

                absPartIdx += ctu->m_numPartitions >> (depth * 2);
            }
        }
    }

    /* calculate frameRecordSize */
    analysis->frameRecordSize = sizeof(analysis->frameRecordSize) + sizeof(depthBytes) + sizeof(analysis->poc);
    analysis->frameRecordSize += depthBytes * sizeof(uint8_t);
    analysis->frameRecordSize += analysis->numCUsInFrame * sizeof(sse_t);
    if (curEncData.m_slice->m_sliceType != I_SLICE)
    {
        int numDir = (curEncData.m_slice->m_sliceType == P_SLICE) ? 1 : 2;
        analysis->frameRecordSize += depthBytes * sizeof(MV) * numDir;
        analysis->frameRecordSize += depthBytes * sizeof(int32_t) * numDir;
        analysis->frameRecordSize += depthBytes * sizeof(uint8_t) * numDir;
        analysis->frameRecordSize += depthBytes * sizeof(uint8_t);
    }
    X265_FWRITE(&analysis->frameRecordSize, sizeof(uint32_t), 1, m_analysisFileOut);
    X265_FWRITE(&depthBytes, sizeof(uint32_t), 1, m_analysisFileOut);
    X265_FWRITE(&analysis->poc, sizeof(uint32_t), 1, m_analysisFileOut);
    X265_FWRITE(distortionData->ctuDistortion, sizeof(sse_t), analysis->numCUsInFrame, m_analysisFileOut);
    if (curEncData.m_slice->m_sliceType == I_SLICE)
    {
        X265_FWRITE((analysis->intraData)->depth, sizeof(uint8_t), depthBytes, m_analysisFileOut);
    }
    else
    {
        X265_FWRITE((analysis->interData)->depth, sizeof(uint8_t), depthBytes, m_analysisFileOut);
    }
    if (curEncData.m_slice->m_sliceType != I_SLICE)
    {
        int numDir = curEncData.m_slice->m_sliceType == P_SLICE ? 1 : 2;
        for (int i = 0; i < numDir; i++)
        {
            int32_t* ref = &(analysis->interData)->ref[i * analysis->numPartitions * analysis->numCUsInFrame];
            X265_FWRITE(interData->mv[i], sizeof(MV), depthBytes, m_analysisFileOut);
            X265_FWRITE(interData->mvpIdx[i], sizeof(uint8_t), depthBytes, m_analysisFileOut);
            X265_FWRITE(ref, sizeof(int32_t), depthBytes, m_analysisFileOut);
        }
        X265_FWRITE((analysis->interData)->modes, sizeof(uint8_t), depthBytes, m_analysisFileOut);
    }
#undef X265_FWRITE
}

void Encoder::printReconfigureParams()
{
    if (!(m_reconfigure || m_reconfigureRc))
        return;
    x265_param* oldParam = m_param;
    x265_param* newParam = m_latestParam;
    
    x265_log(newParam, X265_LOG_DEBUG, "Reconfigured param options, input Frame: %d\n", m_pocLast + 1);

    char tmp[60];
#define TOOLCMP(COND1, COND2, STR)  if (COND1 != COND2) { sprintf(tmp, STR, COND1, COND2); x265_log(newParam, X265_LOG_DEBUG, tmp); }
    TOOLCMP(oldParam->maxNumReferences, newParam->maxNumReferences, "ref=%d to %d\n");
    TOOLCMP(oldParam->bEnableFastIntra, newParam->bEnableFastIntra, "fast-intra=%d to %d\n");
    TOOLCMP(oldParam->bEnableEarlySkip, newParam->bEnableEarlySkip, "early-skip=%d to %d\n");
    TOOLCMP(oldParam->recursionSkipMode, newParam->recursionSkipMode, "rskip=%d to %d\n");
    TOOLCMP(oldParam->searchMethod, newParam->searchMethod, "me=%d to %d\n");
    TOOLCMP(oldParam->searchRange, newParam->searchRange, "merange=%d to %d\n");
    TOOLCMP(oldParam->subpelRefine, newParam->subpelRefine, "subme= %d to %d\n");
    TOOLCMP(oldParam->rdLevel, newParam->rdLevel, "rd=%d to %d\n");
    TOOLCMP(oldParam->rdoqLevel, newParam->rdoqLevel, "rdoq=%d to %d\n" );
    TOOLCMP(oldParam->bEnableRectInter, newParam->bEnableRectInter, "rect=%d to %d\n");
    TOOLCMP(oldParam->maxNumMergeCand, newParam->maxNumMergeCand, "max-merge=%d to %d\n");
    TOOLCMP(oldParam->bIntraInBFrames, newParam->bIntraInBFrames, "b-intra=%d to %d\n");
    TOOLCMP(oldParam->scalingLists, newParam->scalingLists, "scalinglists=%s to %s\n");
    TOOLCMP(oldParam->rc.vbvMaxBitrate, newParam->rc.vbvMaxBitrate, "vbv-maxrate=%d to %d\n");
    TOOLCMP(oldParam->rc.vbvBufferSize, newParam->rc.vbvBufferSize, "vbv-bufsize=%d to %d\n");
    TOOLCMP(oldParam->rc.bitrate, newParam->rc.bitrate, "bitrate=%d to %d\n");
    TOOLCMP(oldParam->rc.rfConstant, newParam->rc.rfConstant, "crf=%f to %f\n");
}

void Encoder::readUserSeiFile(x265_sei_payload& seiMsg, int curPoc)
{
    char line[1024];
    while (fgets(line, sizeof(line), m_naluFile))
    {
        int poc = atoi(strtok(line, " "));
        char *prefix = strtok(NULL, " ");
        int nalType = atoi(strtok(NULL, "/"));
        int payloadType = atoi(strtok(NULL, " "));
        char *base64Encode = strtok(NULL, "\n");
        int base64EncodeLength = (int)strlen(base64Encode);
        char *base64Decode = SEI::base64Decode(base64Encode, base64EncodeLength);
        if (nalType == NAL_UNIT_PREFIX_SEI && (!strcmp(prefix, "PREFIX")))
        {
            int currentPOC = curPoc;
            if (currentPOC == poc)
            {
                seiMsg.payloadSize = (base64EncodeLength / 4) * 3;
                seiMsg.payload = (uint8_t*)x265_malloc(sizeof(uint8_t) * seiMsg.payloadSize);
                if (!seiMsg.payload)
                {
                    x265_log(m_param, X265_LOG_ERROR, "Unable to allocate memory for SEI payload\n");
                    break;
                }
                if (payloadType == 4)
                    seiMsg.payloadType = USER_DATA_REGISTERED_ITU_T_T35;
                else if (payloadType == 5)
                    seiMsg.payloadType = USER_DATA_UNREGISTERED;
                else
                {
                    x265_log(m_param, X265_LOG_WARNING, "Unsupported SEI payload Type for frame %d\n", poc);
                    break;
                }
                memcpy(seiMsg.payload, base64Decode, seiMsg.payloadSize);
                break;
            }
        }
        else
        {
            x265_log(m_param, X265_LOG_WARNING, "SEI message for frame %d is not inserted. Will support only PREFIX SEI messages.\n", poc);
            break;
        }
    }
}

bool Encoder::computeSPSRPSIndex()
{
    RPS* rpsInSPS = m_sps.spsrps;
    int* rpsNumInPSP = &m_sps.spsrpsNum;
    int  beginNum = m_sps.numGOPBegin;
    int  endNum;
    RPS* rpsInRec;
    RPS* rpsInIdxList;
    RPS* thisRpsInSPS;
    RPS* thisRpsInList;
    RPSListNode* headRpsIdxList = NULL;
    RPSListNode* tailRpsIdxList = NULL;
    RPSListNode* rpsIdxListIter = NULL;
    RateControlEntry *rce2Pass = m_rateControl->m_rce2Pass;
    int numEntries = m_rateControl->m_numEntries;
    RateControlEntry *rce;
    int idx = 0;
    int pos = 0;
    int resultIdx[64];
    memset(rpsInSPS, 0, sizeof(RPS) * MAX_NUM_SHORT_TERM_RPS);

    // find out all RPS date in current GOP
    beginNum++;
    endNum = beginNum;
    if (!m_param->bRepeatHeaders)
    {
        endNum = numEntries;
    }
    else
    {
        while (endNum < numEntries)
        {
            rce = &rce2Pass[endNum];
            if (rce->sliceType == I_SLICE)
            {
                if (m_param->keyframeMin && (endNum - beginNum + 1 < m_param->keyframeMin))
                {
                    endNum++;
                    continue;
                }
                break;
            }
            endNum++;
        }
    }
    m_sps.numGOPBegin = endNum;

    // find out all kinds of RPS
    for (int i = beginNum; i < endNum; i++)
    {
        rce = &rce2Pass[i];
        rpsInRec = &rce->rpsData;
        rpsIdxListIter = headRpsIdxList;
        // i frame don't recode RPS info
        if (rce->sliceType != I_SLICE)
        {
            while (rpsIdxListIter)
            {
                rpsInIdxList = rpsIdxListIter->rps;
                if (rpsInRec->numberOfPictures == rpsInIdxList->numberOfPictures
                    && rpsInRec->numberOfNegativePictures == rpsInIdxList->numberOfNegativePictures
                    && rpsInRec->numberOfPositivePictures == rpsInIdxList->numberOfPositivePictures)
                {
                    for (pos = 0; pos < rpsInRec->numberOfPictures; pos++)
                    {
                        if (rpsInRec->deltaPOC[pos] != rpsInIdxList->deltaPOC[pos]
                            || rpsInRec->bUsed[pos] != rpsInIdxList->bUsed[pos])
                            break;
                    }
                    if (pos == rpsInRec->numberOfPictures)    // if this type of RPS has exist
                    {
                        rce->rpsIdx = rpsIdxListIter->idx;
                        rpsIdxListIter->count++;
                        // sort RPS type link after reset RPS type count.
                        RPSListNode* next = rpsIdxListIter->next;
                        RPSListNode* prior = rpsIdxListIter->prior;
                        RPSListNode* iter = prior;
                        if (iter)
                        {
                            while (iter)
                            {
                                if (iter->count > rpsIdxListIter->count)
                                    break;
                                iter = iter->prior;
                            }
                            if (iter)
                            {
                                prior->next = next;
                                if (next)
                                    next->prior = prior;
                                else
                                    tailRpsIdxList = prior;
                                rpsIdxListIter->next = iter->next;
                                rpsIdxListIter->prior = iter;
                                iter->next->prior = rpsIdxListIter;
                                iter->next = rpsIdxListIter;
                            }
                            else
                            {
                                prior->next = next;
                                if (next)
                                    next->prior = prior;
                                else
                                    tailRpsIdxList = prior;
                                headRpsIdxList->prior = rpsIdxListIter;
                                rpsIdxListIter->next = headRpsIdxList;
                                rpsIdxListIter->prior = NULL;
                                headRpsIdxList = rpsIdxListIter;
                            }
                        }
                        break;
                    }
                }
                rpsIdxListIter = rpsIdxListIter->next;
            }
            if (!rpsIdxListIter)  // add new type of RPS
            {
                RPSListNode* newIdxNode = new RPSListNode();
                if (newIdxNode == NULL)
                    goto fail;
                newIdxNode->rps = rpsInRec;
                newIdxNode->idx = idx++;
                newIdxNode->count = 1;
                newIdxNode->next = NULL;
                newIdxNode->prior = NULL;
                if (!tailRpsIdxList)
                    tailRpsIdxList = headRpsIdxList = newIdxNode;
                else
                {
                    tailRpsIdxList->next = newIdxNode;
                    newIdxNode->prior = tailRpsIdxList;
                    tailRpsIdxList = newIdxNode;
                }
                rce->rpsIdx = newIdxNode->idx;
            }
        }
        else
        {
            rce->rpsIdx = -1;
        }
    }

    // get commonly RPS set
    memset(resultIdx, 0, sizeof(resultIdx));
    if (idx > MAX_NUM_SHORT_TERM_RPS)
        idx = MAX_NUM_SHORT_TERM_RPS;

    *rpsNumInPSP = idx;
    rpsIdxListIter = headRpsIdxList;
    for (int i = 0; i < idx; i++)
    {
        resultIdx[i] = rpsIdxListIter->idx;
        m_rpsInSpsCount += rpsIdxListIter->count;
        thisRpsInSPS = rpsInSPS + i;
        thisRpsInList = rpsIdxListIter->rps;
        thisRpsInSPS->numberOfPictures = thisRpsInList->numberOfPictures;
        thisRpsInSPS->numberOfNegativePictures = thisRpsInList->numberOfNegativePictures;
        thisRpsInSPS->numberOfPositivePictures = thisRpsInList->numberOfPositivePictures;
        for (pos = 0; pos < thisRpsInList->numberOfPictures; pos++)
        {
            thisRpsInSPS->deltaPOC[pos] = thisRpsInList->deltaPOC[pos];
            thisRpsInSPS->bUsed[pos] = thisRpsInList->bUsed[pos];
        }
        rpsIdxListIter = rpsIdxListIter->next;
    }

    //reset every frame's RPS index
    for (int i = beginNum; i < endNum; i++)
    {
        int j;
        rce = &rce2Pass[i];
        for (j = 0; j < idx; j++)
        {
            if (rce->rpsIdx == resultIdx[j])
            {
                rce->rpsIdx = j;
                break;
            }
        }

        if (j == idx)
            rce->rpsIdx = -1;
    }

    rpsIdxListIter = headRpsIdxList;
    while (rpsIdxListIter)
    {
        RPSListNode* freeIndex = rpsIdxListIter;
        rpsIdxListIter = rpsIdxListIter->next;
        delete freeIndex;
    }
    return true;

fail:
    rpsIdxListIter = headRpsIdxList;
    while (rpsIdxListIter)
    {
        RPSListNode* freeIndex = rpsIdxListIter;
        rpsIdxListIter = rpsIdxListIter->next;
        delete freeIndex;
    }
    return false;
}

