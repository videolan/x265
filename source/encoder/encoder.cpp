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

#include "TLibEncoder/TEncSlice.h"
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPicYuv.h"
#include "x265.h"
#include "common.h"
#include "threadpool.h"
#include "encoder.h"

#include <stdio.h>
#include <string.h>

struct x265_t : public x265::Encoder {};

using namespace x265;

#if _MSC_VER
#pragma warning(disable: 4800) // 'int' : forcing value to bool 'true' or 'false' (performance warning)
#endif

void Encoder::determineLevelAndProfile(x265_param_t *_param)
{
    // this is all based on the table at on Wikipedia at
    // http://en.wikipedia.org/wiki/High_Efficiency_Video_Coding#Profiles

    // TODO: there are minimum CTU sizes for higher levels, needs to be enforced

    uint32_t lumaSamples = _param->sourceWidth * _param->sourceHeight;
    uint32_t samplesPerSec = lumaSamples * _param->frameRate;
    uint32_t bitrate = 100; // in kbps TODO: ABR

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
    else if (_param->keyframeInterval == 1)
        m_profile = Profile::MAINSTILLPICTURE;
    else
        m_profile = Profile::MAIN;

    static const char *profiles[] = { "None", "Main", "Main10", "Mainstillpicture" };
    static const char *tiers[]    = { "Main", "High" };
    x265_log(_param, X265_LOG_INFO, "%s profile, Level-%s (%s tier)\n", profiles[m_profile], level, tiers[m_levelTier]);
}

void Encoder::configure(x265_param_t *_param)
{
    determineLevelAndProfile(_param);

    // Trim the thread pool if WPP is disabled
    if (_param->bEnableWavefront == 0)
        _param->poolNumThreads = 1;

    setThreadPool(ThreadPool::allocThreadPool(_param->poolNumThreads));
    int actual = ThreadPool::getThreadPool()->getThreadCount();
    if (actual > 1)
    {
        x265_log(_param, X265_LOG_INFO, "thread pool with %d threads, WPP enabled\n", actual);
    }
    else
    {
        x265_log(_param, X265_LOG_INFO, "Parallelism disabled, single thread mode\n");
        _param->bEnableWavefront = 0;
    }

    m_gopSize = 4;
    m_decodingRefreshType = _param->decodingRefreshType;
    m_qp = _param->qp;

    //====== Motion search ========
    m_searchMethod = _param->searchMethod;
    m_searchRange = _param->searchRange;
    m_bipredSearchRange = _param->bipredSearchRange;
    m_useAMP = _param->bEnableAMP;
    m_useRectInter = _param->bEnableRectInter;

    //====== Quality control ========
    m_useRDO = _param->bEnableRDO;
    m_chromaCbQpOffset = _param->cbQpOffset;
    m_chromaCrQpOffset = _param->crQpOffset;

    //====== Coding Tools ========
    m_useRDOQ = _param->bEnableRDOQ;
    m_useRDOQTS = _param->bEnableRDOQTS;
    m_rdPenalty = _param->rdPenalty;
    uint32_t tuQTMaxLog2Size = g_convertToBit[_param->maxCUSize] + 2 - 1;
    m_quadtreeTULog2MaxSize = tuQTMaxLog2Size;
    uint32_t tuQTMinLog2Size = 2; //log2(4)
    m_quadtreeTULog2MinSize = tuQTMinLog2Size;
    m_quadtreeTUMaxDepthInter = _param->tuQTMaxInterDepth;
    m_quadtreeTUMaxDepthIntra = _param->tuQTMaxIntraDepth;
    m_bUseCbfFastMode = _param->bEnableCbfFastMode;
    m_useEarlySkipDetection = _param->bEnableEarlySkip;
    m_useTransformSkip = _param->bEnableTransformSkip;
    m_useTransformSkipFast = _param->bEnableTSkipFast;
    m_bUseConstrainedIntraPred = _param->bEnableConstrainedIntra;
    m_maxNumMergeCand = _param->maxNumMergeCand;
    m_bUseSAO = _param->bEnableSAO;
    m_saoLcuBoundary = _param->saoLcuBoundary;
    m_saoLcuBasedOptimization = _param->saoLcuBasedOptimization;

    //====== Weighted Prediction ========
    m_useWeightedPred = _param->bEnableWeightedPred;
    m_useWeightedBiPred = _param->bEnableWeightedBiPred;

    m_signHideFlag = _param->bEnableSignHiding;
    m_useStrongIntraSmoothing = _param->bEnableStrongIntraSmoothing;
    m_decodedPictureHashSEIEnabled = _param->bEnableDecodedPictureHashSEI;

    //====== Enforce these hard coded settings before initializeGOP() to
    //       avoid a valgrind warning
    m_bLoopFilterDisable = 0;
    m_loopFilterOffsetInPPS = 0;
    m_loopFilterBetaOffsetDiv2 = 0;
    m_loopFilterTcOffsetDiv2 = 0;
    m_loopFilterAcrossTilesEnabledFlag = 1;
    m_deblockingFilterControlPresent = 0;

    //====== HM Settings not exposed for configuration ======
    initializeGOP(_param);
    for (int i = 0; i < MAX_TLAYER; i++)
    {
        m_adLambdaModifier[i] = 1.0;
    }

    TComVPS vps;
    vps.setMaxTLayers(1);
    vps.setTemporalNestingFlag(true);
    vps.setMaxLayers(1);
    for (Int i = 0; i < MAX_TLAYER; i++)
    {
        vps.setNumReorderPics(m_numReorderPics[i], i);
        vps.setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
    }

    m_vps = vps;
    m_maxCuDQPDepth = 0;
    m_maxNumOffsetsPerPic = 2048;
    m_log2ParallelMergeLevelMinus2 = 0;
    m_TMVPModeId = 1; // 0 disabled, 1: enabled, 2: auto
    m_conformanceWindow.setWindow(0, 0, 0, 0);
    int nullpad[2] = { 0, 0 };
    setPad(nullpad);

    m_progressiveSourceFlag = 1;
    m_interlacedSourceFlag = 0;
    m_nonPackedConstraintFlag = 0;
    m_frameOnlyConstraintFlag = 0;
    m_bUseASR = 0;    // adapt search range based on temporal distances
    m_dqpTable = NULL;
    m_recoveryPointSEIEnabled = 0;
    m_bufferingPeriodSEIEnabled = 0;
    m_pictureTimingSEIEnabled = 0;
    m_displayOrientationSEIAngle = 0;
    m_gradualDecodingRefreshInfoEnabled = 0;
    m_decodingUnitInfoSEIEnabled = 0;
    m_SOPDescriptionSEIEnabled = 0;
    m_useScalingListId = 0;
    m_scalingListFile = NULL;
    m_recalculateQPAccordingToLambda = 0;
    m_activeParameterSetsSEIEnabled = 0;
    m_vuiParametersPresentFlag = 0;
    m_minSpatialSegmentationIdc = 0;
    m_aspectRatioIdc = 0;
    m_sarWidth = 0;
    m_sarHeight = 0;
    m_overscanInfoPresentFlag = 0;
    m_overscanAppropriateFlag = 0;
    m_videoSignalTypePresentFlag = 0;
    m_videoFormat = 5;
    m_videoFullRangeFlag = 0;
    m_colourDescriptionPresentFlag = 0;
    m_colourPrimaries = 2;
    m_transferCharacteristics = 2;
    m_matrixCoefficients = 2;
    m_chromaLocInfoPresentFlag = 0;
    m_chromaSampleLocTypeTopField = 0;
    m_chromaSampleLocTypeBottomField = 0;
    m_neutralChromaIndicationFlag = 0;
    m_defaultDisplayWindow.setWindow(0, 0, 0, 0);
    m_frameFieldInfoPresentFlag = 0;
    m_pocProportionalToTimingFlag = 0;
    m_numTicksPocDiffOneMinus1 = 0;
    m_bitstreamRestrictionFlag = 0;
    m_motionVectorsOverPicBoundariesFlag = 0;
    m_maxBytesPerPicDenom = 2;
    m_maxBitsPerMinCuDenom = 1;
    m_log2MaxMvLengthHorizontal = 15;
    m_log2MaxMvLengthVertical = 15;
    m_usePCM = 0;
    m_pcmLog2MinSize = 3;
    m_pcmLog2MaxSize = 5;
    m_bPCMInputBitDepthFlag = 1; 
    m_bPCMFilterDisableFlag = 0;

    m_useLossless = 0;  // x264 configures this via --qp=0
    m_TransquantBypassEnableFlag = 0;
    m_CUTransquantBypassFlagValue = 0;
}

static inline int _confirm(bool bflag, const char* message)
{
    if (!bflag)
        return 0;

    printf("Error: %s\n", message);
    return 1;
}

// TODO: All of this junk should go away when we get a real lookahead
bool Encoder::initializeGOP(x265_param_t *_param)
{
#define CONFIRM(expr, msg) check_failed |= _confirm(expr, msg)
    int check_failed = 0; /* abort if there is a fatal configuration problem */

    if (_param->keyframeInterval == 1)
    {
        /* encoder_all_I */
        m_gopList[0] = GOPEntry();
        m_gopList[0].m_QPFactor = 1;
        m_gopList[0].m_betaOffsetDiv2 = 0;
        m_gopList[0].m_tcOffsetDiv2 = 0;
        m_gopList[0].m_POC = 1;
        m_gopList[0].m_numRefPicsActive = 4;
    }
    else if (_param->bframes > 0) // hacky temporary way to select random access
    {
        /* encoder_randomaccess_main */
        int offsets[] = { 1, 2, 3, 4, 4, 3, 4, 4 };
        double factors[] = { 0, 0.442, 0.3536, 0.3536, 0.68 };
        int pocs[] = { 8, 4, 2, 1, 3, 6, 5, 7 };
        int rps[] = { 0, 4, 2, 1, -2, -3, 1, -2 };
        for (int i = 0; i < 8; i++)
        {
            m_gopList[i].m_POC = pocs[i];
            m_gopList[i].m_QPFactor = factors[offsets[i]];
            m_gopList[i].m_QPOffset = offsets[i];
            m_gopList[i].m_deltaRPS = rps[i];
            m_gopList[i].m_sliceType = 'B';
            m_gopList[i].m_numRefPicsActive = i ? 2 : 4;
            m_gopList[i].m_numRefPics = i == 1 ? 3 : 4;
            m_gopList[i].m_interRPSPrediction = i ? 1 : 0;
            m_gopList[i].m_numRefIdc = i == 2 ? 4 : 5;
        }

#define SET4(id, VAR, a, b, c, d) \
    m_gopList[id].VAR[0] = a; m_gopList[id].VAR[1] = b; m_gopList[id].VAR[2] = c; m_gopList[id].VAR[3] = d;
#define SET5(id, VAR, a, b, c, d, e) \
    m_gopList[id].VAR[0] = a; m_gopList[id].VAR[1] = b; m_gopList[id].VAR[2] = c; m_gopList[id].VAR[3] = d; m_gopList[id].VAR[4] = e;

        SET4(0, m_referencePics, -8, -10, -12, -16);
        SET4(1, m_referencePics, -4, -6, 4, 0);
        SET4(2, m_referencePics, -2, -4, 2, 6);
        SET4(3, m_referencePics, -1, 1, 3, 7);
        SET4(4, m_referencePics, -1, -3, 1, 5);
        SET4(5, m_referencePics, -2, -4, -6, 2);
        SET4(6, m_referencePics, -1, -5, 1, 3);
        SET4(7, m_referencePics, -1, -3, -7, 1);
        SET5(1, m_refIdc, 1, 1, 0, 0, 1);
        SET5(2, m_refIdc, 1, 1, 1, 1, 0);
        SET5(3, m_refIdc, 1, 0, 1, 1, 1);
        SET5(4, m_refIdc, 1, 1, 1, 1, 0);
        SET5(5, m_refIdc, 1, 1, 1, 1, 0);
        SET5(6, m_refIdc, 1, 0, 1, 1, 1);
        SET5(7, m_refIdc, 1, 1, 1, 1, 0);
        m_gopSize = 8;
    }
    else
    {
        /* encoder_I_15P */
        int offsets[] = { 3, 2, 3, 1 };
        for (int i = 0; i < 4; i++)
        {
            m_gopList[i].m_POC = i + 1;
            m_gopList[i].m_QPFactor = 0.4624;
            m_gopList[i].m_QPOffset = offsets[i];
            m_gopList[i].m_numRefPicsActive = 1;
            m_gopList[i].m_numRefPics = 1;
            m_gopList[i].m_referencePics[0] = -1;
            m_gopList[i].m_usedByCurrPic[0] = 1;
            m_gopList[i].m_sliceType = 'P';
            if (i > 0)
            {
                m_gopList[i].m_interRPSPrediction = 1;
                m_gopList[i].m_deltaRPS = -1;
                m_gopList[i].m_numRefIdc = 2;
                m_gopList[i].m_refIdc[0] = 0;
                m_gopList[i].m_refIdc[1] = 1;
            }
        }

        m_gopList[3].m_QPFactor = 0.578;
    }

    if (_param->keyframeInterval > 0)
    {
        m_gopSize = X265_MIN(_param->keyframeInterval, m_gopSize);
        m_gopSize = X265_MAX(1, m_gopSize);
        int remain = _param->keyframeInterval % m_gopSize;
        if (remain)
        {
            _param->keyframeInterval += m_gopSize - remain;
            x265_log(_param, X265_LOG_WARNING, "Keyframe interval must be multiple of %d, forcing --keyint %d\n",
                     m_gopSize, _param->keyframeInterval);
        }
        if (_param->bframes && _param->keyframeInterval < 16)
        {
            _param->keyframeInterval = 16;
            x265_log(_param, X265_LOG_WARNING, "Keyframe interval must be at least 16 for B GOP structure\n");
        }
    }
    else if (_param->keyframeInterval == 0)
    {
        // default keyframe interval of 1 second
        _param->keyframeInterval = _param->frameRate;
        int remain = _param->keyframeInterval % m_gopSize;
        if (remain)
            _param->keyframeInterval += m_gopSize - remain;
    }

    bool verifiedGOP = false;
    bool errorGOP = false;
    int checkGOP = 1;
    int numRefs = 1;

    int refList[MAX_NUM_REF_PICS + 1];
    refList[0] = 0;
    bool isOK[MAX_GOP];
    for (Int i = 0; i < MAX_GOP; i++)
    {
        isOK[i] = false;
    }

    int numOK = 0;
    CONFIRM(_param->keyframeInterval >= 0 && (_param->keyframeInterval % m_gopSize != 0), "Intra period must be a multiple of GOPSize, or -1");

    if ((_param->keyframeInterval != 1) && !m_loopFilterOffsetInPPS && m_deblockingFilterControlPresent && (!m_bLoopFilterDisable))
    {
        for (Int i = 0; i < m_gopSize; i++)
        {
            CONFIRM((m_gopList[i].m_betaOffsetDiv2 + m_loopFilterBetaOffsetDiv2) < -6 || (m_gopList[i].m_betaOffsetDiv2 + m_loopFilterBetaOffsetDiv2) > 6, "Loop Filter Beta Offset div. 2 for one of the GOP entries exceeds supported range (-6 to 6)");
            CONFIRM((m_gopList[i].m_tcOffsetDiv2 + m_loopFilterTcOffsetDiv2) < -6 || (m_gopList[i].m_tcOffsetDiv2 + m_loopFilterTcOffsetDiv2) > 6, "Loop Filter Tc Offset div. 2 for one of the GOP entries exceeds supported range (-6 to 6)");
        }
    }

    m_extraRPSs = 0;
    //start looping through frames in coding order until we can verify that the GOP structure is correct.
    while (!verifiedGOP && !errorGOP)
    {
        Int curGOP = (checkGOP - 1) % m_gopSize;
        Int curPOC = ((checkGOP - 1) / m_gopSize) * m_gopSize + m_gopList[curGOP].m_POC;
        if (m_gopList[curGOP].m_POC < 0)
        {
            printf("\nError: found fewer Reference Picture Sets than GOPSize\n");
            errorGOP = true;
        }
        else
        {
            //check that all reference pictures are available, or have a POC < 0 meaning they might be available in the next GOP.
            Bool beforeI = false;
            for (Int i = 0; i < m_gopList[curGOP].m_numRefPics; i++)
            {
                Int absPOC = curPOC + m_gopList[curGOP].m_referencePics[i];
                if (absPOC < 0)
                {
                    beforeI = true;
                }
                else
                {
                    Bool found = false;
                    for (Int j = 0; j < numRefs; j++)
                    {
                        if (refList[j] == absPOC)
                        {
                            found = true;
                            for (Int k = 0; k < m_gopSize; k++)
                            {
                                if (absPOC % m_gopSize == m_gopList[k].m_POC % m_gopSize)
                                {
                                    m_gopList[k].m_refPic = true;
                                    m_gopList[curGOP].m_usedByCurrPic[i] = 1;
                                }
                            }
                        }
                    }

                    if (!found)
                    {
                        printf("\nError: ref pic %d is not available for GOP frame %d\n", m_gopList[curGOP].m_referencePics[i], curGOP + 1);
                        errorGOP = true;
                    }
                }
            }

            if (!beforeI && !errorGOP)
            {
                //all ref frames were present
                if (!isOK[curGOP])
                {
                    numOK++;
                    isOK[curGOP] = true;
                    if (numOK == m_gopSize)
                    {
                        verifiedGOP = true;
                    }
                }
            }
            else
            {
                //create a new GOPEntry for this frame containing all the reference pictures that were available (POC > 0)
                m_gopList[m_gopSize + m_extraRPSs] = m_gopList[curGOP];
                Int newRefs = 0;
                for (Int i = 0; i < m_gopList[curGOP].m_numRefPics; i++)
                {
                    Int absPOC = curPOC + m_gopList[curGOP].m_referencePics[i];
                    if (absPOC >= 0)
                    {
                        m_gopList[m_gopSize + m_extraRPSs].m_referencePics[newRefs] = m_gopList[curGOP].m_referencePics[i];
                        m_gopList[m_gopSize + m_extraRPSs].m_usedByCurrPic[newRefs] = m_gopList[curGOP].m_usedByCurrPic[i];
                        newRefs++;
                    }
                }

                Int numPrefRefs = m_gopList[curGOP].m_numRefPicsActive;

                for (Int offset = -1; offset > -checkGOP; offset--)
                {
                    //step backwards in coding order and include any extra available pictures we might find useful to replace the ones with POC < 0.
                    Int offGOP = (checkGOP - 1 + offset) % m_gopSize;
                    Int offPOC = ((checkGOP - 1 + offset) / m_gopSize) * m_gopSize + m_gopList[offGOP].m_POC;
                    if (offPOC >= 0)
                    {
                        Bool newRef = false;
                        for (Int i = 0; i < numRefs; i++)
                        {
                            if (refList[i] == offPOC)
                            {
                                newRef = true;
                            }
                        }

                        for (Int i = 0; i < newRefs; i++)
                        {
                            if (m_gopList[m_gopSize + m_extraRPSs].m_referencePics[i] == offPOC - curPOC)
                            {
                                newRef = false;
                            }
                        }

                        if (newRef)
                        {
                            Int insertPoint = newRefs;
                            //this picture can be added, find appropriate place in list and insert it.
                            m_gopList[offGOP].m_refPic = true;
                            for (Int j = 0; j < newRefs; j++)
                            {
                                if (m_gopList[m_gopSize + m_extraRPSs].m_referencePics[j] < offPOC - curPOC || m_gopList[m_gopSize + m_extraRPSs].m_referencePics[j] > 0)
                                {
                                    insertPoint = j;
                                    break;
                                }
                            }

                            Int prev = offPOC - curPOC;
                            Int prevUsed = 1;
                            for (Int j = insertPoint; j < newRefs + 1; j++)
                            {
                                Int newPrev = m_gopList[m_gopSize + m_extraRPSs].m_referencePics[j];
                                Int newUsed = m_gopList[m_gopSize + m_extraRPSs].m_usedByCurrPic[j];
                                m_gopList[m_gopSize + m_extraRPSs].m_referencePics[j] = prev;
                                m_gopList[m_gopSize + m_extraRPSs].m_usedByCurrPic[j] = prevUsed;
                                prevUsed = newUsed;
                                prev = newPrev;
                            }

                            newRefs++;
                        }
                    }
                    if (newRefs >= numPrefRefs)
                    {
                        break;
                    }
                }

                m_gopList[m_gopSize + m_extraRPSs].m_numRefPics = newRefs;
                m_gopList[m_gopSize + m_extraRPSs].m_POC = curPOC;
                if (m_extraRPSs == 0)
                {
                    m_gopList[m_gopSize + m_extraRPSs].m_interRPSPrediction = 0;
                    m_gopList[m_gopSize + m_extraRPSs].m_numRefIdc = 0;
                }
                else
                {
                    Int rIdx =  m_gopSize + m_extraRPSs - 1;
                    Int refPOC = m_gopList[rIdx].m_POC;
                    Int refPics = m_gopList[rIdx].m_numRefPics;
                    Int newIdc = 0;
                    for (Int i = 0; i <= refPics; i++)
                    {
                        Int deltaPOC = ((i != refPics) ? m_gopList[rIdx].m_referencePics[i] : 0); // check if the reference abs POC is >= 0
                        Int absPOCref = refPOC + deltaPOC;
                        Int refIdc = 0;
                        for (Int j = 0; j < m_gopList[m_gopSize + m_extraRPSs].m_numRefPics; j++)
                        {
                            if ((absPOCref - curPOC) == m_gopList[m_gopSize + m_extraRPSs].m_referencePics[j])
                            {
                                if (m_gopList[m_gopSize + m_extraRPSs].m_usedByCurrPic[j])
                                {
                                    refIdc = 1;
                                }
                                else
                                {
                                    refIdc = 2;
                                }
                            }
                        }

                        m_gopList[m_gopSize + m_extraRPSs].m_refIdc[newIdc] = refIdc;
                        newIdc++;
                    }

                    m_gopList[m_gopSize + m_extraRPSs].m_interRPSPrediction = 1;
                    m_gopList[m_gopSize + m_extraRPSs].m_numRefIdc = newIdc;
                    m_gopList[m_gopSize + m_extraRPSs].m_deltaRPS = refPOC - m_gopList[m_gopSize + m_extraRPSs].m_POC;
                }
                curGOP = m_gopSize + m_extraRPSs;
                m_extraRPSs++;
            }
            numRefs = 0;
            for (Int i = 0; i < m_gopList[curGOP].m_numRefPics; i++)
            {
                Int absPOC = curPOC + m_gopList[curGOP].m_referencePics[i];
                if (absPOC >= 0)
                {
                    refList[numRefs] = absPOC;
                    numRefs++;
                }
            }

            refList[numRefs] = curPOC;
            numRefs++;
        }
        checkGOP++;
    }

    CONFIRM(errorGOP, "Invalid GOP structure given");
    for (Int i = 0; i < m_gopSize; i++)
    {
        CONFIRM(m_gopList[i].m_sliceType != 'B' && m_gopList[i].m_sliceType != 'P', "Slice type must be equal to B or P");
    }

    for (Int i = 0; i < MAX_TLAYER; i++)
    {
        m_numReorderPics[i] = 0;
        m_maxDecPicBuffering[i] = 1;
    }

    for (Int i = 0; i < m_gopSize; i++)
    {
        if (m_gopList[i].m_numRefPics + 1 > m_maxDecPicBuffering[0])
        {
            m_maxDecPicBuffering[0] = m_gopList[i].m_numRefPics + 1;
        }
        Int highestDecodingNumberWithLowerPOC = 0;
        for (Int j = 0; j < m_gopSize; j++)
        {
            if (m_gopList[j].m_POC <= m_gopList[i].m_POC)
            {
                highestDecodingNumberWithLowerPOC = j;
            }
        }

        Int numReorder = 0;
        for (Int j = 0; j < highestDecodingNumberWithLowerPOC; j++)
        {
            if (m_gopList[j].m_POC > m_gopList[i].m_POC)
            {
                numReorder++;
            }
        }

        if (numReorder > m_numReorderPics[0])
        {
            m_numReorderPics[0] = numReorder;
        }
    }

    for (Int i = 0; i < MAX_TLAYER - 1; i++)
    {
        // a lower layer can not have higher value of m_numReorderPics than a higher layer
        if (m_numReorderPics[i + 1] < m_numReorderPics[i])
        {
            m_numReorderPics[i + 1] = m_numReorderPics[i];
        }
        // the value of num_reorder_pics[ i ] shall be in the range of 0 to max_dec_pic_buffering[ i ] - 1, inclusive
        if (m_numReorderPics[i] > m_maxDecPicBuffering[i] - 1)
        {
            m_maxDecPicBuffering[i] = m_numReorderPics[i] + 1;
        }
        // a lower layer can not have higher value of m_uiMaxDecPicBuffering than a higher layer
        if (m_maxDecPicBuffering[i + 1] < m_maxDecPicBuffering[i])
        {
            m_maxDecPicBuffering[i + 1] = m_maxDecPicBuffering[i];
        }
    }

    // the value of num_reorder_pics[ i ] shall be in the range of 0 to max_dec_pic_buffering[ i ] -  1, inclusive
    if (m_numReorderPics[MAX_TLAYER - 1] > m_maxDecPicBuffering[MAX_TLAYER - 1] - 1)
    {
        m_maxDecPicBuffering[MAX_TLAYER - 1] = m_numReorderPics[MAX_TLAYER - 1] + 1;
    }

    return check_failed;
}

extern "C"
x265_t *x265_encoder_open(x265_param_t *param)
{
    x265_setup_primitives(param, -1);  // -1 means auto-detect if uninitialized

    if (x265_check_params(param))
        return NULL;

    if (x265_set_globals(param))
        return NULL;

    x265_t *encoder = new x265_t;
    if (encoder)
    {
        encoder->configure(param); // this may change params for auto-detect, etc
        memcpy(&encoder->param, param, sizeof(*param));
        x265_print_params(param);
        encoder->create();
        encoder->init();
    }

    return encoder;
}

int x265_encoder_headers(x265_t *encoder, x265_nal_t **pp_nal, int *pi_nal)
{
    // there is a lot of duplicated code here, see x265_encoder_encode()
    list<AccessUnit> outputAccessUnits;

    int ret = encoder->getStreamHeaders(outputAccessUnits);
    if (pp_nal && !outputAccessUnits.empty())
    {
        encoder->m_nals.clear();
        encoder->m_packetData.clear();

        /* Copy NAL output packets into x265_nal_t structures */
        list<AccessUnit>::const_iterator iterBitstream = outputAccessUnits.begin();
        for (size_t i = 0; i < outputAccessUnits.size(); i++)
        {
            const AccessUnit &au = *(iterBitstream++);

            for (AccessUnit::const_iterator it = au.begin(); it != au.end(); it++)
            {
                const NALUnitEBSP& nalu = **it;
                int size = 0; /* size of annexB unit in bytes */

                static const Char start_code_prefix[] = { 0, 0, 0, 1 };
                if (it == au.begin() || nalu.m_nalUnitType == NAL_UNIT_SPS || nalu.m_nalUnitType == NAL_UNIT_PPS)
                {
                    /* From AVC, When any of the following conditions are fulfilled, the
                     * zero_byte syntax element shall be present:
                     *  - the nal_unit_type within the nal_unit() is equal to 7 (sequence
                     *    parameter set) or 8 (picture parameter set),
                     *  - the byte stream NAL unit syntax structure contains the first NAL
                     *    unit of an access unit in decoding order, as specified by subclause
                     *    7.4.1.2.3.
                     */
                    encoder->m_packetData.append(start_code_prefix, 4);
                    size += 4;
                }
                else
                {
                    encoder->m_packetData.append(start_code_prefix + 1, 3);
                    size += 3;
                }
                size_t nalSize = nalu.m_nalUnitData.str().size();
                encoder->m_packetData.append(nalu.m_nalUnitData.str().c_str(), nalSize);
                size += (int)nalSize;

                x265_nal_t nal;
                nal.i_type = nalu.m_nalUnitType;
                nal.i_payload = size;
                encoder->m_nals.push_back(nal);
            }
        }

        /* Setup payload pointers, now that we're done adding content to m_packetData */
        size_t offset = 0;
        for (size_t i = 0; i < encoder->m_nals.size(); i++)
        {
            x265_nal_t& nal = encoder->m_nals[i];
            nal.p_payload = (uint8_t*)encoder->m_packetData.c_str() + offset;
            offset += nal.i_payload;
        }

        *pp_nal = &encoder->m_nals[0];
        if (pi_nal) *pi_nal = (int)encoder->m_nals.size();
    }

    return ret;
}

extern "C"
int x265_encoder_encode(x265_t *encoder, x265_nal_t **pp_nal, int *pi_nal, x265_picture_t *pic_in, x265_picture_t **pic_out)
{
    /* A boat-load of ugly hacks here until we have a proper lookahead */
    list<AccessUnit> outputAccessUnits;

    int iNumEncoded = encoder->encode(!pic_in, pic_in, pic_out, outputAccessUnits);
    if (pp_nal && !outputAccessUnits.empty())
    {
        encoder->m_nals.clear();
        encoder->m_packetData.clear();

        /* Copy NAL output packets into x265_nal_t structures */
        list<AccessUnit>::const_iterator iterBitstream = outputAccessUnits.begin();
        for (int i = 0; i < iNumEncoded; i++)
        {
            const AccessUnit &au = *(iterBitstream++);

            for (AccessUnit::const_iterator it = au.begin(); it != au.end(); it++)
            {
                const NALUnitEBSP& nalu = **it;
                int size = 0; /* size of annexB unit in bytes */

                static const Char start_code_prefix[] = { 0, 0, 0, 1 };
                if (it == au.begin() || nalu.m_nalUnitType == NAL_UNIT_SPS || nalu.m_nalUnitType == NAL_UNIT_PPS)
                {
                    /* From AVC, When any of the following conditions are fulfilled, the
                     * zero_byte syntax element shall be present:
                     *  - the nal_unit_type within the nal_unit() is equal to 7 (sequence
                     *    parameter set) or 8 (picture parameter set),
                     *  - the byte stream NAL unit syntax structure contains the first NAL
                     *    unit of an access unit in decoding order, as specified by subclause
                     *    7.4.1.2.3.
                     */
                    encoder->m_packetData.append(start_code_prefix, 4);
                    size += 4;
                }
                else
                {
                    encoder->m_packetData.append(start_code_prefix + 1, 3);
                    size += 3;
                }
                size_t nalSize = nalu.m_nalUnitData.str().size();
                encoder->m_packetData.append(nalu.m_nalUnitData.str().c_str(), nalSize);
                size += (int)nalSize;

                x265_nal_t nal;
                nal.i_type = nalu.m_nalUnitType;
                nal.i_payload = size;
                encoder->m_nals.push_back(nal);
            }
        }

        /* Setup payload pointers, now that we're done adding content to m_packetData */
        size_t offset = 0;
        for (size_t i = 0; i < encoder->m_nals.size(); i++)
        {
            x265_nal_t& nal = encoder->m_nals[i];
            nal.p_payload = (uint8_t*)encoder->m_packetData.c_str() + offset;
            offset += nal.i_payload;
        }

        *pp_nal = &encoder->m_nals[0];
        if (pi_nal) *pi_nal = (int)encoder->m_nals.size();
    }
    else if (pi_nal)
        *pi_nal = 0;
    return iNumEncoded;
}

EXTERN_CYCLE_COUNTER(ME);

extern "C"
void x265_encoder_close(x265_t *encoder, double *outPsnr)
{
    Double globalPsnr = encoder->printSummary();

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
    x265::BitCost::destroy();
}
