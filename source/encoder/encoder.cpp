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
#include "encoder.h"

#include <stdio.h>
#include <string.h>

struct x265_t : public x265::Encoder {};

using namespace x265;

#if _MSC_VER
#pragma warning(disable: 4800) // 'int' : forcing value to bool 'true' or 'false' (performance warning)
#endif

void Encoder::configure(x265_param_t *param)
{
    setFrameRate(param->iFrameRate);
    setSourceWidth(param->iSourceWidth);
    setSourceHeight(param->iSourceHeight);
    setIntraPeriod(param->iIntraPeriod);
    setQP(param->iQP);

    //====== Motion search ========
    setSearchMethod(param->searchMethod);
    setSearchRange(param->iSearchRange);
    setBipredSearchRange(param->bipredSearchRange);
    setUseAMP(param->enableAMP);
    setUseRectInter(param->enableRectInter);

    //====== Quality control ========
    setMaxCuDQPDepth(param->iMaxCuDQPDepth);
    setChromaCbQpOffset(param->cbQpOffset);
    setChromaCrQpOffset(param->crQpOffset);
    setUseAdaptQpSelect(param->bUseAdaptQpSelect);

    setUseAdaptiveQP(param->bUseAdaptiveQP);
    setQPAdaptationRange(param->iQPAdaptationRange);

    //====== Coding Tools ========
    setUseRDOQ(param->useRDOQ);
    setUseRDOQTS(param->useRDOQTS);
    setRDpenalty(param->rdPenalty);
    setQuadtreeTULog2MaxSize(param->uiQuadtreeTULog2MaxSize);
    setQuadtreeTULog2MinSize(param->uiQuadtreeTULog2MinSize);
    setQuadtreeTUMaxDepthInter(param->uiQuadtreeTUMaxDepthInter);
    setQuadtreeTUMaxDepthIntra(param->uiQuadtreeTUMaxDepthIntra);
    setUseFastDecisionForMerge(param->useFastDecisionForMerge);
    setUseCbfFastMode(param->bUseCbfFastMode);
    setUseEarlySkipDetection(param->useEarlySkipDetection);
    setUseTransformSkip(param->useTransformSkip);
    setUseTransformSkipFast(param->useTransformSkipFast);
    setUseConstrainedIntraPred(param->bUseConstrainedIntraPred);
    setMaxNumMergeCand(param->maxNumMergeCand);
    setUseSAO(param->bUseSAO);
    setMaxNumOffsetsPerPic(param->maxNumOffsetsPerPic);
    setSaoLcuBoundary(param->saoLcuBoundary);
    setSaoLcuBasedOptimization(param->saoLcuBasedOptimization);

    //====== Parallel Merge Estimation ========
    setLog2ParallelMergeLevelMinus2(param->log2ParallelMergeLevel - 2);

    //====== Weighted Prediction ========
    setUseWP(param->useWeightedPred);
    setWPBiPred(param->useWeightedBiPred);

    setWaveFrontSynchro(param->iWaveFrontSynchro);

    setTMVPModeId(param->TMVPModeId);
    setSignHideFlag(param->signHideFlag);

    setUseStrongIntraSmoothing(param->useStrongIntraSmoothing);

    //====== Settings derived from user configuration ======
    setProfile(m_profile);
    setLevel(m_levelTier, m_level);

    //====== HM Settings not exposed for configuration ======
    InitializeGOP(param);
    setGOPSize(m_iGOPSize);
    for (int i = 0; i < MAX_TLAYER; i++)
    {
        setNumReorderPics(0, i);
        setMaxDecPicBuffering(2, i);
        setLambdaModifier(i, 1.0);
    }

    TComVPS vps;
    vps.setMaxTLayers(1);
    vps.setTemporalNestingFlag(true);
    vps.setMaxLayers(1);
    for (Int i = 0; i < MAX_TLAYER; i++)
    {
        vps.setNumReorderPics(0, i);
        vps.setMaxDecPicBuffering(2, i);
    }
    setVPS(&vps);
    setMaxTempLayer(1);

    setConformanceWindow(0, 0, 0, 0);
    int nullpad[2] = { 0, 0 };
    setPad(nullpad);
    setProgressiveSourceFlag(0);
    setInterlacedSourceFlag(0);
    setNonPackedConstraintFlag(0);
    setFrameOnlyConstraintFlag(0);
    setDecodingRefreshType(0);
    setUseASR(0);
    setUseHADME(1);
    setdQPs(NULL);
    setDecodedPictureHashSEIEnabled(0);
    setRecoveryPointSEIEnabled(0);
    setBufferingPeriodSEIEnabled(0);
    setPictureTimingSEIEnabled(0);
    setDisplayOrientationSEIAngle(0);
    setTemporalLevel0IndexSEIEnabled(0);
    setGradualDecodingRefreshInfoEnabled(0);
    setDecodingUnitInfoSEIEnabled(0);
    setSOPDescriptionSEIEnabled(0);
    setScalableNestingSEIEnabled(0);
    setUseScalingListId(0);
    setScalingListFile(NULL);
    setUseRecalculateQPAccordingToLambda(0);
    setActiveParameterSetsSEIEnabled(0);
    setVuiParametersPresentFlag(0);
    setMinSpatialSegmentationIdc(0);
    setAspectRatioIdc(0);
    setSarWidth(0);
    setSarHeight(0);
    setOverscanInfoPresentFlag(0);
    setOverscanAppropriateFlag(0);
    setVideoSignalTypePresentFlag(0);
    setVideoFormat(5);
    setVideoFullRangeFlag(0);
    setColourDescriptionPresentFlag(0);
    setColourPrimaries(2);
    setTransferCharacteristics(2);
    setMatrixCoefficients(2);
    setChromaLocInfoPresentFlag(0);
    setChromaSampleLocTypeTopField(0);
    setChromaSampleLocTypeBottomField(0);
    setNeutralChromaIndicationFlag(0);
    setDefaultDisplayWindow(0, 0, 0, 0);
    setFrameFieldInfoPresentFlag(0);
    setPocProportionalToTimingFlag(0);
    setNumTicksPocDiffOneMinus1(0);
    setBitstreamRestrictionFlag(0);
    setMotionVectorsOverPicBoundariesFlag(0);
    setMaxBytesPerPicDenom(2);
    setMaxBitsPerMinCuDenom(1);
    setLog2MaxMvLengthHorizontal(15);
    setLog2MaxMvLengthVertical(15);

    setUsePCM(0);
    setPCMLog2MinSize(3);
    setPCMLog2MaxSize(5);
    setPCMInputBitDepthFlag(1);
    setPCMFilterDisableFlag(0);

    setUseRateCtrl(0);
    setTargetBitrate(0);
    setKeepHierBit(0);
    setLCULevelRC(0);
    setUseLCUSeparateModel(0);
    setInitialQP(0);
    setForceIntraQP(0);

    setUseLossless(0); // x264 configures this via --qp=0

    setLoopFilterDisable(0);
    setLoopFilterOffsetInPPS(0);
    setLoopFilterBetaOffset(0);
    setLoopFilterTcOffset(0);
    setDeblockingFilterControlPresent(0);
    setDeblockingFilterMetric(0);

    setTransquantBypassEnableFlag(0);
    setCUTransquantBypassFlagValue(0);
}

static inline int _confirm(bool bflag, const char* message)
{
    if (!bflag)
        return 0;

    printf("Error: %s\n", message);
    return 1;
}

// TODO: All of this junk should go away when we get a real lookahead
bool Encoder::InitializeGOP(x265_param_t *param)
{
#define CONFIRM(expr, msg) check_failed |= _confirm(expr, msg)
    int check_failed = 0; /* abort if there is a fatal configuration problem */

    /* STATIC GOP structure borrowed from our config file */
    int offsets[] = { 3, 2, 3, 1 };
    for (int i = 0; i < 4; i++)
    {
        m_GOPList[i].m_POC = i+1;
        m_GOPList[i].m_QPFactor = 0.4624;
        m_GOPList[i].m_QPOffset = offsets[i];
        m_GOPList[i].m_numRefPicsActive = 1;
        m_GOPList[i].m_numRefPics = 1;
        m_GOPList[i].m_referencePics[0] = -1;
        m_GOPList[i].m_usedByCurrPic[0] = 1;
    }
    m_GOPList[3].m_QPFactor = 0.578;

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
    CONFIRM(param->iIntraPeriod >= 0 && (param->iIntraPeriod % m_iGOPSize != 0), "Intra period must be a multiple of GOPSize, or -1");

    for (int i = 0; i < m_iGOPSize; i++)
    {
        if (m_GOPList[i].m_POC == m_iGOPSize)
        {
            CONFIRM(m_GOPList[i].m_temporalId != 0, "The last frame in each GOP must have temporal ID = 0 ");
        }
    }

    if ((param->iIntraPeriod != 1) && !m_loopFilterOffsetInPPS && m_DeblockingFilterControlPresent && (!m_bLoopFilterDisable))
    {
        for (Int i = 0; i < m_iGOPSize; i++)
        {
            CONFIRM((m_GOPList[i].m_betaOffsetDiv2 + m_loopFilterBetaOffsetDiv2) < -6 || (m_GOPList[i].m_betaOffsetDiv2 + m_loopFilterBetaOffsetDiv2) > 6, "Loop Filter Beta Offset div. 2 for one of the GOP entries exceeds supported range (-6 to 6)");
            CONFIRM((m_GOPList[i].m_tcOffsetDiv2 + m_loopFilterTcOffsetDiv2) < -6 || (m_GOPList[i].m_tcOffsetDiv2 + m_loopFilterTcOffsetDiv2) > 6, "Loop Filter Tc Offset div. 2 for one of the GOP entries exceeds supported range (-6 to 6)");
        }
    }

    m_extraRPSs = 0;
    //start looping through frames in coding order until we can verify that the GOP structure is correct.
    while (!verifiedGOP && !errorGOP)
    {
        Int curGOP = (checkGOP - 1) % m_iGOPSize;
        Int curPOC = ((checkGOP - 1) / m_iGOPSize) * m_iGOPSize + m_GOPList[curGOP].m_POC;
        if (m_GOPList[curGOP].m_POC < 0)
        {
            printf("\nError: found fewer Reference Picture Sets than GOPSize\n");
            errorGOP = true;
        }
        else
        {
            //check that all reference pictures are available, or have a POC < 0 meaning they might be available in the next GOP.
            Bool beforeI = false;
            for (Int i = 0; i < m_GOPList[curGOP].m_numRefPics; i++)
            {
                Int absPOC = curPOC + m_GOPList[curGOP].m_referencePics[i];
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
                            for (Int k = 0; k < m_iGOPSize; k++)
                            {
                                if (absPOC % m_iGOPSize == m_GOPList[k].m_POC % m_iGOPSize)
                                {
                                    if (m_GOPList[k].m_temporalId == m_GOPList[curGOP].m_temporalId)
                                    {
                                        m_GOPList[k].m_refPic = true;
                                    }
                                    m_GOPList[curGOP].m_usedByCurrPic[i] = m_GOPList[k].m_temporalId <= m_GOPList[curGOP].m_temporalId;
                                }
                            }
                        }
                    }

                    if (!found)
                    {
                        printf("\nError: ref pic %d is not available for GOP frame %d\n", m_GOPList[curGOP].m_referencePics[i], curGOP + 1);
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
                    if (numOK == m_iGOPSize)
                    {
                        verifiedGOP = true;
                    }
                }
            }
            else
            {
                //create a new GOPEntry for this frame containing all the reference pictures that were available (POC > 0)
                m_GOPList[m_iGOPSize + m_extraRPSs] = m_GOPList[curGOP];
                Int newRefs = 0;
                for (Int i = 0; i < m_GOPList[curGOP].m_numRefPics; i++)
                {
                    Int absPOC = curPOC + m_GOPList[curGOP].m_referencePics[i];
                    if (absPOC >= 0)
                    {
                        m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[newRefs] = m_GOPList[curGOP].m_referencePics[i];
                        m_GOPList[m_iGOPSize + m_extraRPSs].m_usedByCurrPic[newRefs] = m_GOPList[curGOP].m_usedByCurrPic[i];
                        newRefs++;
                    }
                }

                Int numPrefRefs = m_GOPList[curGOP].m_numRefPicsActive;

                for (Int offset = -1; offset > -checkGOP; offset--)
                {
                    //step backwards in coding order and include any extra available pictures we might find useful to replace the ones with POC < 0.
                    Int offGOP = (checkGOP - 1 + offset) % m_iGOPSize;
                    Int offPOC = ((checkGOP - 1 + offset) / m_iGOPSize) * m_iGOPSize + m_GOPList[offGOP].m_POC;
                    if (offPOC >= 0 && m_GOPList[offGOP].m_temporalId <= m_GOPList[curGOP].m_temporalId)
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
                            if (m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[i] == offPOC - curPOC)
                            {
                                newRef = false;
                            }
                        }

                        if (newRef)
                        {
                            Int insertPoint = newRefs;
                            //this picture can be added, find appropriate place in list and insert it.
                            if (m_GOPList[offGOP].m_temporalId == m_GOPList[curGOP].m_temporalId)
                            {
                                m_GOPList[offGOP].m_refPic = true;
                            }
                            for (Int j = 0; j < newRefs; j++)
                            {
                                if (m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[j] < offPOC - curPOC || m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[j] > 0)
                                {
                                    insertPoint = j;
                                    break;
                                }
                            }

                            Int prev = offPOC - curPOC;
                            Int prevUsed = m_GOPList[offGOP].m_temporalId <= m_GOPList[curGOP].m_temporalId;
                            for (Int j = insertPoint; j < newRefs + 1; j++)
                            {
                                Int newPrev = m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[j];
                                Int newUsed = m_GOPList[m_iGOPSize + m_extraRPSs].m_usedByCurrPic[j];
                                m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[j] = prev;
                                m_GOPList[m_iGOPSize + m_extraRPSs].m_usedByCurrPic[j] = prevUsed;
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

                m_GOPList[m_iGOPSize + m_extraRPSs].m_numRefPics = newRefs;
                m_GOPList[m_iGOPSize + m_extraRPSs].m_POC = curPOC;
                if (m_extraRPSs == 0)
                {
                    m_GOPList[m_iGOPSize + m_extraRPSs].m_interRPSPrediction = 0;
                    m_GOPList[m_iGOPSize + m_extraRPSs].m_numRefIdc = 0;
                }
                else
                {
                    Int rIdx =  m_iGOPSize + m_extraRPSs - 1;
                    Int refPOC = m_GOPList[rIdx].m_POC;
                    Int refPics = m_GOPList[rIdx].m_numRefPics;
                    Int newIdc = 0;
                    for (Int i = 0; i <= refPics; i++)
                    {
                        Int deltaPOC = ((i != refPics) ? m_GOPList[rIdx].m_referencePics[i] : 0); // check if the reference abs POC is >= 0
                        Int absPOCref = refPOC + deltaPOC;
                        Int refIdc = 0;
                        for (Int j = 0; j < m_GOPList[m_iGOPSize + m_extraRPSs].m_numRefPics; j++)
                        {
                            if ((absPOCref - curPOC) == m_GOPList[m_iGOPSize + m_extraRPSs].m_referencePics[j])
                            {
                                if (m_GOPList[m_iGOPSize + m_extraRPSs].m_usedByCurrPic[j])
                                {
                                    refIdc = 1;
                                }
                                else
                                {
                                    refIdc = 2;
                                }
                            }
                        }

                        m_GOPList[m_iGOPSize + m_extraRPSs].m_refIdc[newIdc] = refIdc;
                        newIdc++;
                    }

                    m_GOPList[m_iGOPSize + m_extraRPSs].m_interRPSPrediction = 1;
                    m_GOPList[m_iGOPSize + m_extraRPSs].m_numRefIdc = newIdc;
                    m_GOPList[m_iGOPSize + m_extraRPSs].m_deltaRPS = refPOC - m_GOPList[m_iGOPSize + m_extraRPSs].m_POC;
                }
                curGOP = m_iGOPSize + m_extraRPSs;
                m_extraRPSs++;
            }
            numRefs = 0;
            for (Int i = 0; i < m_GOPList[curGOP].m_numRefPics; i++)
            {
                Int absPOC = curPOC + m_GOPList[curGOP].m_referencePics[i];
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
    m_maxTempLayer = 1;
    for (Int i = 0; i < m_iGOPSize; i++)
    {
        if (m_GOPList[i].m_temporalId >= m_maxTempLayer)
        {
            m_maxTempLayer = m_GOPList[i].m_temporalId + 1;
        }
        CONFIRM(m_GOPList[i].m_sliceType != 'B' && m_GOPList[i].m_sliceType != 'P', "Slice type must be equal to B or P");
    }

    for (Int i = 0; i < MAX_TLAYER; i++)
    {
        m_numReorderPics[i] = 0;
        m_maxDecPicBuffering[i] = 1;
    }

    for (Int i = 0; i < m_iGOPSize; i++)
    {
        if (m_GOPList[i].m_numRefPics + 1 > m_maxDecPicBuffering[m_GOPList[i].m_temporalId])
        {
            m_maxDecPicBuffering[m_GOPList[i].m_temporalId] = m_GOPList[i].m_numRefPics + 1;
        }
        Int highestDecodingNumberWithLowerPOC = 0;
        for (Int j = 0; j < m_iGOPSize; j++)
        {
            if (m_GOPList[j].m_POC <= m_GOPList[i].m_POC)
            {
                highestDecodingNumberWithLowerPOC = j;
            }
        }

        Int numReorder = 0;
        for (Int j = 0; j < highestDecodingNumberWithLowerPOC; j++)
        {
            if (m_GOPList[j].m_temporalId <= m_GOPList[i].m_temporalId &&
                m_GOPList[j].m_POC > m_GOPList[i].m_POC)
            {
                numReorder++;
            }
        }

        if (numReorder > m_numReorderPics[m_GOPList[i].m_temporalId])
        {
            m_numReorderPics[m_GOPList[i].m_temporalId] = numReorder;
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
    if (x265_check_params(param))
        return NULL;

    x265_t *encoder = new x265_t;
    if (encoder)
    {
        encoder->configure(param);
        encoder->create();
        encoder->init();

        for (int i = 0; i < MAX_GOP; i++)
        {
            TComPicYuv *pcPicYuvRec = new TComPicYuv;
            pcPicYuvRec->create(param->iSourceWidth, param->iSourceHeight, param->uiMaxCUSize, param->uiMaxCUSize, param->uiMaxCUDepth);
            encoder->m_cListPicYuvRec.pushBack(pcPicYuvRec);
        }
    }

    return encoder;
}

extern "C"
int x265_encoder_encode(x265_t *encoder, x265_nal_t **pp_nal, int *pi_nal, x265_picture_t *pic_in, x265_picture_t *pic_out)
{
    /* A boat-load of ugly hacks here until we have a proper lookahead */
    list<AccessUnit> outputAccessUnits;
    if (pi_nal) *pi_nal = 0;

    /* TEncTop::encode uses this strange mechanism of pushing frames to the back of the list and then returning
     * reconstructed images ordered from the middle to the end */
    TComPicYuv *pcPicYuvRec = encoder->m_cListPicYuvRec.popFront();
    encoder->m_cListPicYuvRec.pushBack(pcPicYuvRec);
    int iNumEncoded = encoder->encode(!pic_in, pic_in, encoder->m_cListPicYuvRec, outputAccessUnits);
    if (iNumEncoded > 0)
    {
        if (pp_nal)
        {
            /* Copy NAL output packets into x265_nal_t structures */
            encoder->m_nals.clear();
            encoder->m_nals.reserve(*pi_nal);
            encoder->m_packetData.clear();

            list<AccessUnit>::const_iterator iterBitstream = outputAccessUnits.begin();
            for (int i = 0; i < iNumEncoded; i++)
            {
                const AccessUnit &au = *(iterBitstream++);

                for (AccessUnit::const_iterator it = au.begin(); it != au.end(); it++)
                {
                    const NALUnitEBSP& nalu = **it;
                    UInt size = 0; /* size of annexB unit in bytes */

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
                        encoder->m_packetData.write(start_code_prefix, 4);
                        size += 4;
                    }
                    else
                    {
                        encoder->m_packetData.write(start_code_prefix + 1, 3);
                        size += 3;
                    }
                    encoder->m_packetData << nalu.m_nalUnitData;
                    size += UInt(nalu.m_nalUnitData.str().size());

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
                nal.p_payload = (uint8_t*) &encoder->m_packetData.str()[0] + offset;
                offset += nal.i_payload;
            }

            *pp_nal = &encoder->m_nals[0];
            if (pi_nal) *pi_nal = (int)encoder->m_nals.size();
        }

        /* push these recon output frame pointers onto m_cListRecQueue */
        TComList<TComPicYuv *>::iterator iterPicYuvRec = encoder->m_cListPicYuvRec.end();
        for (int i = 0; i < iNumEncoded; i++)
            --iterPicYuvRec;
        for (int i = 0; i < iNumEncoded; i++)
            encoder->m_cListRecQueue.pushBack(*(iterPicYuvRec++));
    }

    if (pic_out && encoder->m_cListRecQueue.size())
    {
        TComPicYuv *recpic = encoder->m_cListRecQueue.popFront();
        pic_out->planes[0] = recpic->getLumaAddr(); pic_out->stride[0] = recpic->getStride();
        pic_out->planes[1] = recpic->getCbAddr();   pic_out->stride[1] = recpic->getCStride();
        pic_out->planes[2] = recpic->getCrAddr();   pic_out->stride[2] = recpic->getCStride();
        pic_out->bitDepth = sizeof(Pel)*8;
        return 1;
    }

    return pp_nal ? 0 : 0;  // just to avoid unused parameter warning
}

extern "C"
void x265_encoder_close(x265_t *encoder)
{
    encoder->printSummary();

    // delete used buffers in encoder class
    encoder->deletePicBuffer();
    encoder->destroy();
    delete encoder;
}

extern "C"
void x265_cleanup(void)
{
    x265::BitCost::destroy();
}
