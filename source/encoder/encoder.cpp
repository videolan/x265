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
    setUseAMP(param->enableAMP);
    setUseRectInter(param->enableRectInter);

    //====== Motion search ========
    setSearchMethod(param->searchMethod);
    setSearchRange(param->iSearchRange);
    setBipredSearchRange(param->bipredSearchRange);

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
    setGOPSize(4);
    int offsets[] = { 3, 2, 3, 1 };
    for (int i = 0; i < 4; i++)
    {
        m_GOPList[i].m_POC = i+1;
        m_GOPList[i].m_QPFactor = 0.4624;
        m_GOPList[i].m_QPOffset = offsets[i];
        m_GOPList[i].m_betaOffsetDiv2 = 0;
        m_GOPList[i].m_tcOffsetDiv2 = 0;
        m_GOPList[i].m_numRefPicsActive = 1;
        m_GOPList[i].m_numRefPics = 1;
        m_GOPList[i].m_referencePics[0] = -1;
        m_GOPList[i].m_usedByCurrPic[0] = 1;
        m_GOPList[i].m_refPic = true;
        m_GOPList[i].m_temporalId = 0;
        m_GOPList[i].m_numRefIdc = 0;
        m_GOPList[i].m_interRPSPrediction = 1;
        m_GOPList[i].m_deltaRPS = -1;
        m_GOPList[i].m_sliceType = 'P';
        m_GOPList[i].m_numRefIdc = 2;
        m_GOPList[i].m_refIdc[2] = 1;
    }
    m_GOPList[0].m_interRPSPrediction = 0;
    m_GOPList[0].m_deltaRPS = 0;
    m_GOPList[0].m_numRefIdc = 0;
    m_GOPList[0].m_refIdc[2] = 0;
    m_GOPList[3].m_QPFactor = 0.578;
    setGopList(m_GOPList);
    setExtraRPSs(0);
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
    setProgressiveSourceFlag(1);
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

    /* TEncTop::encode uses this strange mechanism of pushing frames to the back of the list and then returning
     * reconstructed images ordered from the middle to the end */
    TComPicYuv *pcPicYuvRec = encoder->m_cListPicYuvRec.popFront();
    encoder->m_cListPicYuvRec.pushBack(pcPicYuvRec);
    int iNumEncoded = encoder->encode(!!pic_in, pic_in, encoder->m_cListPicYuvRec, outputAccessUnits);
    if (iNumEncoded > 0)
    {
        if (pi_nal)
            *pi_nal = (int)outputAccessUnits.size();
        //if (pp_nal) *pp_nal = reinterpret_cast<x265_nal_t*>(outputAccessUnits.begin());

        /* push these recon output frame pointers onto m_cListRecQueue */
        TComList<TComPicYuv *>::iterator iterPicYuvRec = encoder->m_cListPicYuvRec.end();
        for (int i = 0; i < iNumEncoded; i++)
            --iterPicYuvRec;
        for (int i = 0; i < iNumEncoded; i++)
            encoder->m_cListRecQueue.pushBack(*(iterPicYuvRec++));
    }
    else if (pi_nal)
        *pi_nal = 0;

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
