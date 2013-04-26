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

/** \file     TAppEncTop.cpp
    \brief    Encoder application class
*/

#include <list>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "x265enc.h"
#include "TLibEncoder/AnnexBwrite.h"
#include "PPA/ppa.h"

using namespace std;

//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / initialization / destroy
// ====================================================================================================================

TAppEncTop::TAppEncTop()
{
    m_iFrameRcvd = 0;
    m_totalBytes = 0;
    m_essentialBytes = 0;
}

TAppEncTop::~TAppEncTop()
{}

Void TAppEncTop::xInitLibCfg()
{
    TComVPS vps;

    vps.setMaxTLayers(m_maxTempLayer);
    if (m_maxTempLayer == 1)
    {
        vps.setTemporalNestingFlag(true);
    }

    vps.setMaxLayers(1);
    for (Int i = 0; i < MAX_TLAYER; i++)
    {
        vps.setNumReorderPics(m_numReorderPics[i], i);
        vps.setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
    }

    m_cTEncTop.setVPS(&vps);

    m_cTEncTop.setProfile(m_profile);
    m_cTEncTop.setLevel(m_levelTier, m_level);
#if L0046_CONSTRAINT_FLAGS
    m_cTEncTop.setProgressiveSourceFlag(m_progressiveSourceFlag);
    m_cTEncTop.setInterlacedSourceFlag(m_interlacedSourceFlag);
    m_cTEncTop.setNonPackedConstraintFlag(m_nonPackedConstraintFlag);
    m_cTEncTop.setFrameOnlyConstraintFlag(m_frameOnlyConstraintFlag);
#endif

    m_cTEncTop.setFrameRate(m_iFrameRate);
    m_cTEncTop.setFrameSkip(m_FrameSkip);
    m_cTEncTop.setSourceWidth(m_iSourceWidth);
    m_cTEncTop.setSourceHeight(m_iSourceHeight);
    m_cTEncTop.setConformanceWindow(0, 0, 0, 0);
    m_cTEncTop.setFramesToBeEncoded(m_framesToBeEncoded);
    int nullpad[2] = { 0, 0 };
    m_cTEncTop.setPad(nullpad);

    //====== Coding Structure ========
    m_cTEncTop.setIntraPeriod(m_iIntraPeriod);
    m_cTEncTop.setDecodingRefreshType(m_iDecodingRefreshType);
    m_cTEncTop.setGOPSize(m_iGOPSize);
    m_cTEncTop.setGopList(m_GOPList);
    m_cTEncTop.setExtraRPSs(m_extraRPSs);
    for (Int i = 0; i < MAX_TLAYER; i++)
    {
        m_cTEncTop.setNumReorderPics(m_numReorderPics[i], i);
        m_cTEncTop.setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
    }

    for (UInt uiLoop = 0; uiLoop < MAX_TLAYER; ++uiLoop)
    {
        m_cTEncTop.setLambdaModifier(uiLoop, m_adLambdaModifier[uiLoop]);
    }

    m_cTEncTop.setQP(m_iQP);

    m_cTEncTop.setMaxTempLayer(m_maxTempLayer);
    m_cTEncTop.setUseAMP(m_enableAMP);

    //===== Slice ========

    //====== Loop/Deblock Filter ========
    m_cTEncTop.setLoopFilterDisable(m_bLoopFilterDisable);
    m_cTEncTop.setLoopFilterOffsetInPPS(m_loopFilterOffsetInPPS);
    m_cTEncTop.setLoopFilterBetaOffset(m_loopFilterBetaOffsetDiv2);
    m_cTEncTop.setLoopFilterTcOffset(m_loopFilterTcOffsetDiv2);
    m_cTEncTop.setDeblockingFilterControlPresent(m_DeblockingFilterControlPresent);
#if L0386_DB_METRIC
    m_cTEncTop.setDeblockingFilterMetric(m_DeblockingFilterMetric);
#endif

    //====== Motion search ========
    m_cTEncTop.setFastSearch(m_iFastSearch);
    m_cTEncTop.setSearchRange(m_iSearchRange);
    m_cTEncTop.setBipredSearchRange(m_bipredSearchRange);

    //====== Quality control ========
    m_cTEncTop.setMaxDeltaQP(m_iMaxDeltaQP);
    m_cTEncTop.setMaxCuDQPDepth(m_iMaxCuDQPDepth);

    m_cTEncTop.setChromaCbQpOffset(m_cbQpOffset);
    m_cTEncTop.setChromaCrQpOffset(m_crQpOffset);

#if ADAPTIVE_QP_SELECTION
    m_cTEncTop.setUseAdaptQpSelect(m_bUseAdaptQpSelect);
#endif

    Int lowestQP;
    lowestQP =  -6 * (g_bitDepthY - 8); // XXX: check

    if ((m_iMaxDeltaQP == 0) && (m_iQP == lowestQP) && m_useLossless)
    {
        m_bUseAdaptiveQP = 0;
    }

    m_cTEncTop.setUseAdaptiveQP(m_bUseAdaptiveQP);
    m_cTEncTop.setQPAdaptationRange(m_iQPAdaptationRange);

    //====== Tool list ========
    m_cTEncTop.setUseSBACRD(m_bUseSBACRD);
    m_cTEncTop.setDeltaQpRD(m_uiDeltaQpRD);
    m_cTEncTop.setUseASR(m_bUseASR);
    m_cTEncTop.setUseHADME(m_bUseHADME);
    m_cTEncTop.setUseLossless(m_useLossless);
    m_cTEncTop.setdQPs(m_aidQP);
    m_cTEncTop.setUseRDOQ(m_useRDOQ);
    m_cTEncTop.setUseRDOQTS(m_useRDOQTS);
#if L0232_RD_PENALTY
    m_cTEncTop.setRDpenalty(m_rdPenalty);
#endif
    m_cTEncTop.setQuadtreeTULog2MaxSize(m_uiQuadtreeTULog2MaxSize);
    m_cTEncTop.setQuadtreeTULog2MinSize(m_uiQuadtreeTULog2MinSize);
    m_cTEncTop.setQuadtreeTUMaxDepthInter(m_uiQuadtreeTUMaxDepthInter);
    m_cTEncTop.setQuadtreeTUMaxDepthIntra(m_uiQuadtreeTUMaxDepthIntra);
    m_cTEncTop.setUseFastEnc(m_bUseFastEnc);
    m_cTEncTop.setUseEarlyCU(m_bUseEarlyCU);
    m_cTEncTop.setUseFastDecisionForMerge(m_useFastDecisionForMerge);
    m_cTEncTop.setUseCbfFastMode(m_bUseCbfFastMode);
    m_cTEncTop.setUseEarlySkipDetection(m_useEarlySkipDetection);

    m_cTEncTop.setUseTransformSkip(m_useTransformSkip);
    m_cTEncTop.setUseTransformSkipFast(m_useTransformSkipFast);
    m_cTEncTop.setUseConstrainedIntraPred(m_bUseConstrainedIntraPred);
    m_cTEncTop.setPCMLog2MinSize(m_uiPCMLog2MinSize);
    m_cTEncTop.setUsePCM(m_usePCM);
    m_cTEncTop.setPCMLog2MaxSize(m_pcmLog2MaxSize);
    m_cTEncTop.setMaxNumMergeCand(m_maxNumMergeCand);

    //====== Weighted Prediction ========
    m_cTEncTop.setUseWP(m_useWeightedPred);
    m_cTEncTop.setWPBiPred(m_useWeightedBiPred);
    //====== Parallel Merge Estimation ========
    m_cTEncTop.setLog2ParallelMergeLevelMinus2(m_log2ParallelMergeLevel - 2);

    //====== Slice ========
    m_cTEncTop.setSliceMode(m_sliceMode);
    m_cTEncTop.setSliceArgument(m_sliceArgument);

    //====== Dependent Slice ========
    m_cTEncTop.setSliceSegmentMode(m_sliceSegmentMode);
    m_cTEncTop.setSliceSegmentArgument(m_sliceSegmentArgument);
    Int iNumPartInCU = 1 << (m_uiMaxCUDepth << 1);
    if (m_sliceSegmentMode == FIXED_NUMBER_OF_LCU)
    {
        m_cTEncTop.setSliceSegmentArgument(m_sliceSegmentArgument * iNumPartInCU);
    }

    if (m_sliceMode == FIXED_NUMBER_OF_LCU)
    {
        m_cTEncTop.setSliceArgument(m_sliceArgument * iNumPartInCU);
    }

    if (m_sliceMode == FIXED_NUMBER_OF_TILES)
    {
        m_cTEncTop.setSliceArgument(m_sliceArgument);
    }

    if (m_sliceMode == 0)
    {
        m_bLFCrossSliceBoundaryFlag = true;
    }

    m_cTEncTop.setLFCrossSliceBoundaryFlag(m_bLFCrossSliceBoundaryFlag);
    m_cTEncTop.setUseSAO(m_bUseSAO);
    m_cTEncTop.setMaxNumOffsetsPerPic(m_maxNumOffsetsPerPic);

    m_cTEncTop.setSaoLcuBoundary(m_saoLcuBoundary);
    m_cTEncTop.setSaoLcuBasedOptimization(m_saoLcuBasedOptimization);
    m_cTEncTop.setPCMInputBitDepthFlag(m_bPCMInputBitDepthFlag);
    m_cTEncTop.setPCMFilterDisableFlag(m_bPCMFilterDisableFlag);

    m_cTEncTop.setDecodedPictureHashSEIEnabled(m_decodedPictureHashSEIEnabled);
    m_cTEncTop.setRecoveryPointSEIEnabled(m_recoveryPointSEIEnabled);
    m_cTEncTop.setBufferingPeriodSEIEnabled(m_bufferingPeriodSEIEnabled);
    m_cTEncTop.setPictureTimingSEIEnabled(m_pictureTimingSEIEnabled);
#if J0149_TONE_MAPPING_SEI
    m_cTEncTop.setToneMappingInfoSEIEnabled(m_toneMappingInfoSEIEnabled);
    m_cTEncTop.setTMISEIToneMapId(m_toneMapId);
    m_cTEncTop.setTMISEIToneMapCancelFlag(m_toneMapCancelFlag);
    m_cTEncTop.setTMISEIToneMapPersistenceFlag(m_toneMapPersistenceFlag);
    m_cTEncTop.setTMISEICodedDataBitDepth(m_toneMapCodedDataBitDepth);
    m_cTEncTop.setTMISEITargetBitDepth(m_toneMapTargetBitDepth);
    m_cTEncTop.setTMISEIModelID(m_toneMapModelId);
    m_cTEncTop.setTMISEIMinValue(m_toneMapMinValue);
    m_cTEncTop.setTMISEIMaxValue(m_toneMapMaxValue);
    m_cTEncTop.setTMISEISigmoidMidpoint(m_sigmoidMidpoint);
    m_cTEncTop.setTMISEISigmoidWidth(m_sigmoidWidth);
    m_cTEncTop.setTMISEIStartOfCodedInterva(m_startOfCodedInterval);
    m_cTEncTop.setTMISEINumPivots(m_numPivots);
    m_cTEncTop.setTMISEICodedPivotValue(m_codedPivotValue);
    m_cTEncTop.setTMISEITargetPivotValue(m_targetPivotValue);
    m_cTEncTop.setTMISEICameraIsoSpeedIdc(m_cameraIsoSpeedIdc);
    m_cTEncTop.setTMISEICameraIsoSpeedValue(m_cameraIsoSpeedValue);
    m_cTEncTop.setTMISEIExposureCompensationValueSignFlag(m_exposureCompensationValueSignFlag);
    m_cTEncTop.setTMISEIExposureCompensationValueNumerator(m_exposureCompensationValueNumerator);
    m_cTEncTop.setTMISEIExposureCompensationValueDenomIdc(m_exposureCompensationValueDenomIdc);
    m_cTEncTop.setTMISEIRefScreenLuminanceWhite(m_refScreenLuminanceWhite);
    m_cTEncTop.setTMISEIExtendedRangeWhiteLevel(m_extendedRangeWhiteLevel);
    m_cTEncTop.setTMISEINominalBlackLevelLumaCodeValue(m_nominalBlackLevelLumaCodeValue);
    m_cTEncTop.setTMISEINominalWhiteLevelLumaCodeValue(m_nominalWhiteLevelLumaCodeValue);
    m_cTEncTop.setTMISEIExtendedWhiteLevelLumaCodeValue(m_extendedWhiteLevelLumaCodeValue);
#endif // if J0149_TONE_MAPPING_SEI
    m_cTEncTop.setFramePackingArrangementSEIEnabled(m_framePackingSEIEnabled);
    m_cTEncTop.setFramePackingArrangementSEIType(m_framePackingSEIType);
    m_cTEncTop.setFramePackingArrangementSEIId(m_framePackingSEIId);
    m_cTEncTop.setFramePackingArrangementSEIQuincunx(m_framePackingSEIQuincunx);
    m_cTEncTop.setFramePackingArrangementSEIInterpretation(m_framePackingSEIInterpretation);
    m_cTEncTop.setDisplayOrientationSEIAngle(m_displayOrientationSEIAngle);
    m_cTEncTop.setTemporalLevel0IndexSEIEnabled(m_temporalLevel0IndexSEIEnabled);
    m_cTEncTop.setGradualDecodingRefreshInfoEnabled(m_gradualDecodingRefreshInfoEnabled);
    m_cTEncTop.setDecodingUnitInfoSEIEnabled(m_decodingUnitInfoSEIEnabled);
#if L0208_SOP_DESCRIPTION_SEI
    m_cTEncTop.setSOPDescriptionSEIEnabled(m_SOPDescriptionSEIEnabled);
#endif
#if K0180_SCALABLE_NESTING_SEI
    m_cTEncTop.setScalableNestingSEIEnabled(m_scalableNestingSEIEnabled);
#endif
    m_cTEncTop.setUniformSpacingIdr(m_iUniformSpacingIdr);
    m_cTEncTop.setNumColumnsMinus1(m_iNumColumnsMinus1);
    m_cTEncTop.setNumRowsMinus1(m_iNumRowsMinus1);
    if (m_iUniformSpacingIdr == 0)
    {
        m_cTEncTop.setColumnWidth(m_pColumnWidth);
        m_cTEncTop.setRowHeight(m_pRowHeight);
    }

    m_cTEncTop.xCheckGSParameters();
    Int uiTilesCount          = (m_iNumRowsMinus1 + 1) * (m_iNumColumnsMinus1 + 1);
    if (uiTilesCount == 1)
    {
        m_bLFCrossTileBoundaryFlag = true;
    }

    m_cTEncTop.setLFCrossTileBoundaryFlag(m_bLFCrossTileBoundaryFlag);
    m_cTEncTop.setWaveFrontSynchro(m_iWaveFrontSynchro);
    m_cTEncTop.setWaveFrontSubstreams(m_iWaveFrontSubstreams);
    m_cTEncTop.setTMVPModeId(m_TMVPModeId);
    m_cTEncTop.setUseScalingListId(m_useScalingListId);
    m_cTEncTop.setScalingListFile(m_scalingListFile);
    m_cTEncTop.setSignHideFlag(m_signHideFlag);
#if RATE_CONTROL_LAMBDA_DOMAIN
    m_cTEncTop.setUseRateCtrl(m_RCEnableRateControl);
    m_cTEncTop.setTargetBitrate(m_RCTargetBitrate);
    m_cTEncTop.setKeepHierBit(m_RCKeepHierarchicalBit);
    m_cTEncTop.setLCULevelRC(m_RCLCULevelRC);
    m_cTEncTop.setUseLCUSeparateModel(m_RCUseLCUSeparateModel);
    m_cTEncTop.setInitialQP(m_RCInitialQP);
    m_cTEncTop.setForceIntraQP(m_RCForceIntraQP);
#else
    m_cTEncTop.setUseRateCtrl(m_enableRateCtrl);
    m_cTEncTop.setTargetBitrate(m_targetBitrate);
    m_cTEncTop.setNumLCUInUnit(m_numLCUInUnit);
#endif // if RATE_CONTROL_LAMBDA_DOMAIN
    m_cTEncTop.setTransquantBypassEnableFlag(m_TransquantBypassEnableFlag);
    m_cTEncTop.setCUTransquantBypassFlagValue(m_CUTransquantBypassFlagValue);
    m_cTEncTop.setUseRecalculateQPAccordingToLambda(m_recalculateQPAccordingToLambda);
    m_cTEncTop.setUseStrongIntraSmoothing(m_useStrongIntraSmoothing);
    m_cTEncTop.setActiveParameterSetsSEIEnabled(m_activeParameterSetsSEIEnabled);
    m_cTEncTop.setVuiParametersPresentFlag(m_vuiParametersPresentFlag);
    m_cTEncTop.setAspectRatioIdc(m_aspectRatioIdc);
    m_cTEncTop.setSarWidth(m_sarWidth);
    m_cTEncTop.setSarHeight(m_sarHeight);
    m_cTEncTop.setOverscanInfoPresentFlag(m_overscanInfoPresentFlag);
    m_cTEncTop.setOverscanAppropriateFlag(m_overscanAppropriateFlag);
    m_cTEncTop.setVideoSignalTypePresentFlag(m_videoSignalTypePresentFlag);
    m_cTEncTop.setVideoFormat(m_videoFormat);
    m_cTEncTop.setVideoFullRangeFlag(m_videoFullRangeFlag);
    m_cTEncTop.setColourDescriptionPresentFlag(m_colourDescriptionPresentFlag);
    m_cTEncTop.setColourPrimaries(m_colourPrimaries);
    m_cTEncTop.setTransferCharacteristics(m_transferCharacteristics);
    m_cTEncTop.setMatrixCoefficients(m_matrixCoefficients);
    m_cTEncTop.setChromaLocInfoPresentFlag(m_chromaLocInfoPresentFlag);
    m_cTEncTop.setChromaSampleLocTypeTopField(m_chromaSampleLocTypeTopField);
    m_cTEncTop.setChromaSampleLocTypeBottomField(m_chromaSampleLocTypeBottomField);
    m_cTEncTop.setNeutralChromaIndicationFlag(m_neutralChromaIndicationFlag);
    m_cTEncTop.setDefaultDisplayWindow(m_defDispWinLeftOffset,
                                       m_defDispWinRightOffset,
                                       m_defDispWinTopOffset,
                                       m_defDispWinBottomOffset);
    m_cTEncTop.setFrameFieldInfoPresentFlag(m_frameFieldInfoPresentFlag);
    m_cTEncTop.setPocProportionalToTimingFlag(m_pocProportionalToTimingFlag);
    m_cTEncTop.setNumTicksPocDiffOneMinus1(m_numTicksPocDiffOneMinus1);
    m_cTEncTop.setBitstreamRestrictionFlag(m_bitstreamRestrictionFlag);
    m_cTEncTop.setTilesFixedStructureFlag(m_tilesFixedStructureFlag);
    m_cTEncTop.setMotionVectorsOverPicBoundariesFlag(m_motionVectorsOverPicBoundariesFlag);
    m_cTEncTop.setMinSpatialSegmentationIdc(m_minSpatialSegmentationIdc);
    m_cTEncTop.setMaxBytesPerPicDenom(m_maxBytesPerPicDenom);
    m_cTEncTop.setMaxBitsPerMinCuDenom(m_maxBitsPerMinCuDenom);
    m_cTEncTop.setLog2MaxMvLengthHorizontal(m_log2MaxMvLengthHorizontal);
    m_cTEncTop.setLog2MaxMvLengthVertical(m_log2MaxMvLengthVertical);
#if SIGNAL_BITRATE_PICRATE_IN_VPS
    TComBitRatePicRateInfo *bitRatePicRateInfo = m_cTEncTop.getVPS()->getBitratePicrateInfo();
    // The number of bit rate/pic rate have to equal to number of sub-layers.
    if (m_bitRatePicRateMaxTLayers)
    {
        assert(m_bitRatePicRateMaxTLayers == m_cTEncTop.getVPS()->getMaxTLayers());
    }

    for (Int i = 0; i < m_bitRatePicRateMaxTLayers; i++)
    {
        bitRatePicRateInfo->setBitRateInfoPresentFlag(i, m_bitRateInfoPresentFlag[i]);
        if (bitRatePicRateInfo->getBitRateInfoPresentFlag(i))
        {
            bitRatePicRateInfo->setAvgBitRate(i, m_avgBitRate[i]);
            bitRatePicRateInfo->setMaxBitRate(i, m_maxBitRate[i]);
        }
    }

    for (Int i = 0; i < m_bitRatePicRateMaxTLayers; i++)
    {
        bitRatePicRateInfo->setPicRateInfoPresentFlag(i, m_picRateInfoPresentFlag[i]);
        if (bitRatePicRateInfo->getPicRateInfoPresentFlag(i))
        {
            bitRatePicRateInfo->setAvgPicRate(i, m_avgPicRate[i]);
            bitRatePicRateInfo->setConstantPicRateIdc(i, m_constantPicRateIdc[i]);
        }
    }

#endif // if SIGNAL_BITRATE_PICRATE_IN_VPS
}

Void TAppEncTop::xCreateLib()
{
    m_cTEncTop.create();
}

Void TAppEncTop::xDestroyLib()
{
    m_cTEncTop.destroy();
}

Void TAppEncTop::xInitLib()
{
    m_cTEncTop.init();
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 - create internal class
 - initialize internal variable
 - until the end of input YUV file, call encoding function in TEncTop class
 - delete allocated buffers
 - destroy internal class
 .
 */
Void TAppEncTop::encode()
{
    fstream bitstreamFile(m_pchBitstreamFile, fstream::binary | fstream::out);

    if (!bitstreamFile)
    {
        fprintf(stderr, "\nfailed to open bitstream file `%s' for writing\n", m_pchBitstreamFile);
        exit(EXIT_FAILURE);
    }

    TComPicYuv       *pcPicYuvOrg = new TComPicYuv;
    TComPicYuv       *pcPicYuvRec = NULL;

    // initialize internal class & member variables
    xInitLibCfg();
    xCreateLib();
    xInitLib();

    // main encoder loop
    Int   iNumEncoded = 0;
    Bool  bEos = false;

    list<AccessUnit> outputAccessUnits; ///< list of access units to write out.  is populated by the encoding process

    // allocate original YUV buffer
    pcPicYuvOrg->create(m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth);

    while (!bEos)
    {
        // get buffers
        xGetBuffer(pcPicYuvRec);

        // read input YUV file
        x265_picture pic;
        Bool flush = false;
        if (m_input->readPicture(pic))
        {
            m_iFrameRcvd++;
            bEos = (m_iFrameRcvd == m_framesToBeEncoded);
        }
        else
        {
            flush = true;
            bEos = true;
            m_cTEncTop.setFramesToBeEncoded(m_iFrameRcvd);
        }

        // call encoding function for one frame
        PPAStartCpuEventFunc(encode_frame);
        m_cTEncTop.encode(bEos, flush ? 0 : &pic, m_cListPicYuvRec, outputAccessUnits, iNumEncoded);
        PPAStopCpuEventFunc(encode_frame);

        // write bistream to file if necessary
        if (iNumEncoded > 0)
        {
            PPAStartCpuEventFunc(bitstream_write);
            xWriteOutput(bitstreamFile, iNumEncoded, outputAccessUnits);
            outputAccessUnits.clear();
            PPAStopCpuEventFunc(bitstream_write);
        }
    }

    m_cTEncTop.printSummary();

    // delete original YUV buffer
    pcPicYuvOrg->destroy();
    delete pcPicYuvOrg;
    pcPicYuvOrg = NULL;

    // delete used buffers in encoder class
    m_cTEncTop.deletePicBuffer();

    // delete buffers & classes
    xDeleteBuffer();
    xDestroyLib();

    printRateSummary();
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 - application has picture buffer list with size of GOP
 - picture buffer list acts as ring buffer
 - end of the list has the latest picture
 .
 */
Void TAppEncTop::xGetBuffer(TComPicYuv*& rpcPicYuvRec)
{
    assert(m_iGOPSize > 0);

    // org. buffer
    if (m_cListPicYuvRec.size() == (UInt)m_iGOPSize)
    {
        rpcPicYuvRec = m_cListPicYuvRec.popFront();
    }
    else
    {
        rpcPicYuvRec = new TComPicYuv;

        rpcPicYuvRec->create(m_iSourceWidth, m_iSourceHeight, m_uiMaxCUWidth, m_uiMaxCUHeight, m_uiMaxCUDepth);
    }

    m_cListPicYuvRec.pushBack(rpcPicYuvRec);
}

Void TAppEncTop::xDeleteBuffer()
{
    TComList<TComPicYuv *>::iterator iterPicYuvRec  = m_cListPicYuvRec.begin();

    Int iSize = Int(m_cListPicYuvRec.size());

    for (Int i = 0; i < iSize; i++)
    {
        TComPicYuv  *pcPicYuvRec  = *(iterPicYuvRec++);
        pcPicYuvRec->destroy();
        delete pcPicYuvRec;
        pcPicYuvRec = NULL;
    }
}

/** \param iNumEncoded  number of encoded frames
 */
Void TAppEncTop::xWriteOutput(std::ostream &bitstreamFile, Int iNumEncoded, const std::list<AccessUnit>& accessUnits)
{
    Int i;

    TComList<TComPicYuv *>::iterator iterPicYuvRec = m_cListPicYuvRec.end();
    list<AccessUnit>::const_iterator iterBitstream = accessUnits.begin();

    for (i = 0; i < iNumEncoded; i++)
    {
        --iterPicYuvRec;
    }

    x265_picture pic;
    for (i = 0; i < iNumEncoded; i++)
    {
        if (m_recon)
        {
            TComPicYuv  *pcPicYuvRec  = *(iterPicYuvRec++);
            pic.planes[0] = pcPicYuvRec->getLumaAddr(); pic.stride[0] = pcPicYuvRec->getStride();
            pic.planes[1] = pcPicYuvRec->getCbAddr();   pic.stride[1] = pcPicYuvRec->getCStride();
            pic.planes[2] = pcPicYuvRec->getCrAddr();   pic.stride[2] = pcPicYuvRec->getCStride();
            pic.bitDepth = sizeof(Pel) == 8 ? 8 : 16;
            m_recon->writePicture(pic);
        }

        const AccessUnit &au = *(iterBitstream++);
        const vector<UInt>& stats = writeAnnexB(bitstreamFile, au);
        rateStatsAccum(au, stats);
    }
}

/**
 *
 */
void TAppEncTop::rateStatsAccum(const AccessUnit &au, const std::vector<UInt>& annexBsizes)
{
    AccessUnit::const_iterator it_au = au.begin();

    vector<UInt>::const_iterator it_stats = annexBsizes.begin();

    for (; it_au != au.end(); it_au++, it_stats++)
    {
        switch ((*it_au)->m_nalUnitType)
        {
        case NAL_UNIT_CODED_SLICE_TRAIL_R:
        case NAL_UNIT_CODED_SLICE_TRAIL_N:
        case NAL_UNIT_CODED_SLICE_TLA_R:
        case NAL_UNIT_CODED_SLICE_TSA_N:
        case NAL_UNIT_CODED_SLICE_STSA_R:
        case NAL_UNIT_CODED_SLICE_STSA_N:
        case NAL_UNIT_CODED_SLICE_BLA_W_LP:
        case NAL_UNIT_CODED_SLICE_BLA_W_RADL:
        case NAL_UNIT_CODED_SLICE_BLA_N_LP:
        case NAL_UNIT_CODED_SLICE_IDR_W_RADL:
        case NAL_UNIT_CODED_SLICE_IDR_N_LP:
        case NAL_UNIT_CODED_SLICE_CRA:
        case NAL_UNIT_CODED_SLICE_RADL_N:
        case NAL_UNIT_CODED_SLICE_RADL_R:
        case NAL_UNIT_CODED_SLICE_RASL_N:
        case NAL_UNIT_CODED_SLICE_RASL_R:
        case NAL_UNIT_VPS:
        case NAL_UNIT_SPS:
        case NAL_UNIT_PPS:
            m_essentialBytes += *it_stats;
            break;
        default:
            break;
        }

        m_totalBytes += *it_stats;
    }
}

void TAppEncTop::printRateSummary()
{
    Double time = (Double)m_iFrameRcvd / m_iFrameRate;

    printf("Bytes written to file: %u (%.3f kbps)\n", m_totalBytes, 0.008 * m_totalBytes / time);
#if VERBOSE_RATE
    printf("Bytes for SPS/PPS/Slice (Incl. Annex B): %u (%.3f kbps)\n", m_essentialBytes, 0.008 * m_essentialBytes / time);
#endif
}

//! \}
