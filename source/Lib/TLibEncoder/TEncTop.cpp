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
#include "TEncTop.h"
#include "TEncPic.h"
#include "TLibCommon/ContextModel.h"
#include "primitives.h"

#include <limits.h>
#include <math.h> // sqrt

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncTop::TEncTop()
{
    m_iPOCLast          = -1;
    m_framesToBeEncoded = INT_MAX;
    m_iNumPicRcvd       = 0;
    m_uiNumAllPicCoded  = 0;
    m_iMaxRefPicNum     = 0;
    ContextModel::buildNextStateTable();

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

Void TEncTop::create()
{
    if (x265::primitives.sad[0] == NULL)
    {
        printf("Primitives must be initialized before encoder is created\n");
        exit(1);
    }

    // initialize global variables
    initROM();

    // create processing unit classes
    m_cGOPEncoder.create();
    if (m_bUseSAO)
    {
        m_cEncSAO.setSaoLcuBoundary(getSaoLcuBoundary());
        m_cEncSAO.setSaoLcuBasedOptimization(getSaoLcuBasedOptimization());
        m_cEncSAO.setMaxNumOffsetsPerPic(getMaxNumOffsetsPerPic());
        m_cEncSAO.create(getSourceWidth(), getSourceHeight(), g_uiMaxCUWidth, g_uiMaxCUHeight);
        m_cEncSAO.createEncBuffer();
    }

    if (m_RCEnableRateControl)
    {
        m_cRateCtrl.init(m_framesToBeEncoded, m_RCTargetBitrate, m_iFrameRate, m_iGOPSize, m_iSourceWidth, m_iSourceHeight,
                         g_uiMaxCUWidth, g_uiMaxCUHeight, m_RCKeepHierarchicalBit, m_RCUseLCUSeparateModel, m_GOPList);
    }
}

Void TEncTop::destroy()
{
    // destroy processing unit classes
    m_cGOPEncoder.destroy();
    if (m_cSPS.getUseSAO())
    {
        m_cEncSAO.destroy();
        m_cEncSAO.destroyEncBuffer();
    }
    m_cRateCtrl.destroy();

   
    deletePicBuffer();

    // destroy ROM
    destroyROM();

    if (m_threadPool)
        m_threadPool->Release();
}

Void TEncTop::init()
{
    // initialize SPS
    xInitSPS();

    /* set the VPS profile information */
    *m_cVPS.getPTL() = *m_cSPS.getPTL();
    m_cVPS.getTimingInfo()->setTimingInfoPresentFlag(false);
    // initialize PPS
    m_cPPS.setSPS(&m_cSPS);
    xInitPPS();
    xInitRPS();

    // initialize processing unit classes
    m_iNumSubstreams = (getSourceHeight() + m_cSPS.getMaxCUHeight() - 1) / m_cSPS.getMaxCUHeight();
    m_cGOPEncoder.init(this);

    m_gcAnalyzeAll.setFrmRate(getFrameRate());
    m_gcAnalyzeI.setFrmRate(getFrameRate());
    m_gcAnalyzeP.setFrmRate(getFrameRate());
    m_gcAnalyzeB.setFrmRate(getFrameRate());
    m_vRVM_RP.reserve(512);
}

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

Void TEncTop::deletePicBuffer()
{
    TComList<TComPic*>::iterator iterPic = m_cListPic.begin();
    Int iSize = Int(m_cListPic.size());

    for (Int i = 0; i < iSize; i++)
    {
        TComPic* pcPic = *(iterPic++);

        pcPic->destroy();
        delete pcPic;
        pcPic = NULL;
    }
}

/**
 - Application has picture buffer list with size of GOP + 1
 - Picture buffer list acts like as ring buffer
 - End of the list has the latest picture
 .
 \param   flush               cause encoder to encode a partial GOP
 \param   pcPicYuvOrg         original YUV picture
 \retval  rcListPicYuvRecOut  list of reconstruction YUV pictures
 \retval  rcListBitstreamOut  list of output bitstreams
 \retval                      number of encoded pictures
 */
int TEncTop::encode(Bool flush, const x265_picture_t* pic, TComList<TComPicYuv*>& rcListPicYuvRecOut, std::list<AccessUnit>& accessUnitsOut)
{
    if (pic)
    {
        m_iNumPicRcvd++;
        
        // get original YUV
        TComPic* pcPicCurr = NULL;
        xGetNewPicBuffer(pcPicCurr);
        pcPicCurr->getPicYuvOrg()->copyFromPicture(*pic);

        // compute image characteristics
        if (getUseAdaptiveQP())
        {
            m_cPreanalyzer.xPreanalyze(dynamic_cast<TEncPic*>(pcPicCurr));
        }
    }

    // Wait until we have a full GOP of pictures
    if (!m_iNumPicRcvd || (!flush && m_iPOCLast != 0 && m_iNumPicRcvd != m_iGOPSize && m_iGOPSize))
    {
        return 0;
    }
    if (flush)
    {
        m_framesToBeEncoded = m_iNumPicRcvd + m_uiNumAllPicCoded;
    }

    // compress GOP
    m_cGOPEncoder.compressGOP(m_iPOCLast, m_iNumPicRcvd, m_cListPic, rcListPicYuvRecOut, accessUnitsOut);

    m_uiNumAllPicCoded += m_iNumPicRcvd;
    Int iNumEncoded = m_iNumPicRcvd;
    m_iNumPicRcvd = 0;
    return iNumEncoded;
}

Void TEncTop::printSummary()
{
    if (getLogLevel() >= X265_LOG_INFO)
    {
        m_gcAnalyzeI.printOut('i');
        m_gcAnalyzeP.printOut('p');
        m_gcAnalyzeB.printOut('b');
        m_gcAnalyzeAll.printOut('a');
        if (getGOPSize() == 1 && getIntraPeriod() != 1 && getFramesToBeEncoded() > RVM_VCEGAM10_M * 2)
            fprintf(stderr, "\nRVM: %.3lf\n", xCalculateRVM());
    }

#if _SUMMARY_OUT_
    m_gcAnalyzeAll.printSummaryOut();
#endif
#if _SUMMARY_PIC_
    m_gcAnalyzeI.printSummary('I');
    m_gcAnalyzeP.printSummary('P');
    m_gcAnalyzeB.printSummary('B');
#endif
}

Double TEncTop::xCalculateRVM()
{
    // calculate RVM only for lowdelay configurations
    std::vector<Double> vRL, vB;
    size_t N = m_vRVM_RP.size();
    vRL.resize(N);
    vB.resize(N);

    Int i;
    Double dRavg = 0, dBavg = 0;
    vB[RVM_VCEGAM10_M] = 0;
    for (i = RVM_VCEGAM10_M + 1; i < N - RVM_VCEGAM10_M + 1; i++)
    {
        vRL[i] = 0;
        for (Int j = i - RVM_VCEGAM10_M; j <= i + RVM_VCEGAM10_M - 1; j++)
        {
            vRL[i] += m_vRVM_RP[j];
        }

        vRL[i] /= (2 * RVM_VCEGAM10_M);
        vB[i] = vB[i - 1] + m_vRVM_RP[i] - vRL[i];
        dRavg += m_vRVM_RP[i];
        dBavg += vB[i];
    }

    dRavg /= (N - 2 * RVM_VCEGAM10_M);
    dBavg /= (N - 2 * RVM_VCEGAM10_M);

    Double dSigamB = 0;
    for (i = RVM_VCEGAM10_M + 1; i < N - RVM_VCEGAM10_M + 1; i++)
    {
        Double tmp = vB[i] - dBavg;
        dSigamB += tmp * tmp;
    }

    dSigamB = sqrt(dSigamB / (N - 2 * RVM_VCEGAM10_M));

    Double f = sqrt(12.0 * (RVM_VCEGAM10_M - 1) / (RVM_VCEGAM10_M + 1));

    return dSigamB / dRavg * f;
}

// ====================================================================================================================
// Protected member functions
// ====================================================================================================================

/**
 - Application has picture buffer list with size of GOP + 1
 - Picture buffer list acts like as ring buffer
 - End of the list has the latest picture
 .
 \retval rpcPic obtained picture buffer
 */
Void TEncTop::xGetNewPicBuffer(TComPic*& rpcPic)
{
    TComSlice::sortPicList(m_cListPic);

    if (m_cListPic.size() >= (UInt)(m_iGOPSize + getMaxDecPicBuffering(MAX_TLAYER - 1) + 2))
    {
        TComList<TComPic*>::iterator iterPic  = m_cListPic.begin();
        Int iSize = Int(m_cListPic.size());
        for (Int i = 0; i < iSize; i++)
        {
            rpcPic = *(iterPic++);
            if (rpcPic->getSlice()->isReferenced() == false)
            {
                break;
            }
        }
    }
    else
    {
        if (getUseAdaptiveQP())
        {
            TEncPic* pcEPic = new TEncPic;
            pcEPic->create(m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, m_cPPS.getMaxCuDQPDepth() + 1,
                           m_conformanceWindow, m_defaultDisplayWindow, m_numReorderPics);
            rpcPic = pcEPic;
        }
        else
        {
            rpcPic = new TComPic;

            rpcPic->create(m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth,
                           m_conformanceWindow, m_defaultDisplayWindow, m_numReorderPics);
        }
        if (getUseSAO())
        {
            rpcPic->getPicSym()->allocSaoParam(&m_cEncSAO);
        }
        m_cListPic.pushBack(rpcPic);
    }
    rpcPic->getSlice()->setPOC(++m_iPOCLast);
    rpcPic->getPicYuvRec()->setBorderExtension(false);
}

Void TEncTop::xInitSPS()
{
    ProfileTierLevel& profileTierLevel = *m_cSPS.getPTL()->getGeneralPTL();

    profileTierLevel.setLevelIdc(m_level);
    profileTierLevel.setTierFlag(m_levelTier);
    profileTierLevel.setProfileIdc(m_profile);
    profileTierLevel.setProfileCompatibilityFlag(m_profile, 1);
    profileTierLevel.setProgressiveSourceFlag(m_progressiveSourceFlag);
    profileTierLevel.setInterlacedSourceFlag(m_interlacedSourceFlag);
    profileTierLevel.setNonPackedConstraintFlag(m_nonPackedConstraintFlag);
    profileTierLevel.setFrameOnlyConstraintFlag(m_frameOnlyConstraintFlag);

    if (m_profile == Profile::MAIN10 && g_bitDepthY == 8 && g_bitDepthC == 8)
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

    m_cSPS.setPicWidthInLumaSamples(m_iSourceWidth);
    m_cSPS.setPicHeightInLumaSamples(m_iSourceHeight);
    m_cSPS.setConformanceWindow(m_conformanceWindow);
    m_cSPS.setMaxCUWidth(g_uiMaxCUWidth);
    m_cSPS.setMaxCUHeight(g_uiMaxCUHeight);
    m_cSPS.setMaxCUDepth(g_uiMaxCUDepth);

    Int minCUSize = m_cSPS.getMaxCUWidth() >> (m_cSPS.getMaxCUDepth() - g_uiAddCUDepth);
    Int log2MinCUSize = 0;
    while (minCUSize > 1)
    {
        minCUSize >>= 1;
        log2MinCUSize++;
    }

    m_cSPS.setLog2MinCodingBlockSize(log2MinCUSize);
    m_cSPS.setLog2DiffMaxMinCodingBlockSize(m_cSPS.getMaxCUDepth() - g_uiAddCUDepth);

    m_cSPS.setPCMLog2MinSize(m_uiPCMLog2MinSize);
    m_cSPS.setUsePCM(m_usePCM);
    m_cSPS.setPCMLog2MaxSize(m_pcmLog2MaxSize);

    m_cSPS.setQuadtreeTULog2MaxSize(m_uiQuadtreeTULog2MaxSize);
    m_cSPS.setQuadtreeTULog2MinSize(m_uiQuadtreeTULog2MinSize);
    m_cSPS.setQuadtreeTUMaxDepthInter(m_uiQuadtreeTUMaxDepthInter);
    m_cSPS.setQuadtreeTUMaxDepthIntra(m_uiQuadtreeTUMaxDepthIntra);

    m_cSPS.setTMVPFlagsPresent(false);
    m_cSPS.setUseLossless(m_useLossless);

    m_cSPS.setMaxTrSize(1 << m_uiQuadtreeTULog2MaxSize);

    Int i;

    for (i = 0; i < g_uiMaxCUDepth - g_uiAddCUDepth; i++)
    {
        m_cSPS.setAMPAcc(i, m_useAMP);
    }

    m_cSPS.setUseAMP(m_useAMP);

    for (i = g_uiMaxCUDepth - g_uiAddCUDepth; i < g_uiMaxCUDepth; i++)
    {
        m_cSPS.setAMPAcc(i, 0);
    }

    m_cSPS.setBitDepthY(g_bitDepthY);
    m_cSPS.setBitDepthC(g_bitDepthC);

    m_cSPS.setQpBDOffsetY(6 * (g_bitDepthY - 8));
    m_cSPS.setQpBDOffsetC(6 * (g_bitDepthC - 8));

    m_cSPS.setUseSAO(m_bUseSAO);

    m_cSPS.setMaxTLayers(m_maxTempLayer);
    m_cSPS.setTemporalIdNestingFlag((m_maxTempLayer == 1) ? true : false);
    for (i = 0; i < m_cSPS.getMaxTLayers(); i++)
    {
        m_cSPS.setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
        m_cSPS.setNumReorderPics(m_numReorderPics[i], i);
    }

    m_cSPS.setPCMBitDepthLuma(g_uiPCMBitDepthLuma);
    m_cSPS.setPCMBitDepthChroma(g_uiPCMBitDepthChroma);
    m_cSPS.setPCMFilterDisableFlag(m_bPCMFilterDisableFlag);

    m_cSPS.setScalingListFlag((m_useScalingListId == 0) ? 0 : 1);

    m_cSPS.setUseStrongIntraSmoothing(m_useStrongIntraSmoothing);

    m_cSPS.setVuiParametersPresentFlag(getVuiParametersPresentFlag());
    if (m_cSPS.getVuiParametersPresentFlag())
    {
        TComVUI* pcVUI = m_cSPS.getVuiParameters();
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
}

Void TEncTop::xInitPPS()
{
    m_cPPS.setConstrainedIntraPred(m_bUseConstrainedIntraPred);
    Bool bUseDQP = (getMaxCuDQPDepth() > 0) ? true : false;

    Int lowestQP = -m_cSPS.getQpBDOffsetY();

    if (getUseLossless())
    {
        if ((getMaxCuDQPDepth() == 0) && (getQP() == lowestQP))
        {
            bUseDQP = false;
        }
        else
        {
            bUseDQP = true;
        }
    }
    else
    {
        bUseDQP |= getUseAdaptiveQP();
    }

    if (bUseDQP)
    {
        m_cPPS.setUseDQP(true);
        m_cPPS.setMaxCuDQPDepth(m_iMaxCuDQPDepth);
        m_cPPS.setMinCuDQPSize(m_cPPS.getSPS()->getMaxCUWidth() >> (m_cPPS.getMaxCuDQPDepth()));
    }
    else
    {
        m_cPPS.setUseDQP(false);
        m_cPPS.setMaxCuDQPDepth(0);
        m_cPPS.setMinCuDQPSize(m_cPPS.getSPS()->getMaxCUWidth() >> (m_cPPS.getMaxCuDQPDepth()));
    }

    if (m_RCEnableRateControl)
    {
        m_cPPS.setUseDQP(true);
        m_cPPS.setMaxCuDQPDepth(0);
        m_cPPS.setMinCuDQPSize(m_cPPS.getSPS()->getMaxCUWidth() >> (m_cPPS.getMaxCuDQPDepth()));
    }

    m_cPPS.setChromaCbQpOffset(m_chromaCbQpOffset);
    m_cPPS.setChromaCrQpOffset(m_chromaCrQpOffset);

    m_cPPS.setEntropyCodingSyncEnabledFlag(m_iWaveFrontSynchro > 0);
    m_cPPS.setUseWP(m_useWeightedPred);
    m_cPPS.setWPBiPred(m_useWeightedBiPred);
    m_cPPS.setOutputFlagPresentFlag(false);
    m_cPPS.setSignHideFlag(getSignHideFlag());
    if (getDeblockingFilterMetric())
    {
        m_cPPS.setDeblockingFilterControlPresentFlag(true);
        m_cPPS.setDeblockingFilterOverrideEnabledFlag(true);
        m_cPPS.setPicDisableDeblockingFilterFlag(false);
        m_cPPS.setDeblockingFilterBetaOffsetDiv2(0);
        m_cPPS.setDeblockingFilterTcOffsetDiv2(0);
    }
    else
    {
        m_cPPS.setDeblockingFilterControlPresentFlag(m_DeblockingFilterControlPresent);
    }
    m_cPPS.setLog2ParallelMergeLevelMinus2(m_log2ParallelMergeLevelMinus2);
    m_cPPS.setCabacInitPresentFlag(CABAC_INIT_PRESENT_FLAG);
    Int histogram[MAX_NUM_REF + 1];
    for (Int i = 0; i <= MAX_NUM_REF; i++)
    {
        histogram[i] = 0;
    }

    for (Int i = 0; i < getGOPSize(); i++)
    {
        assert(getGOPEntry(i).m_numRefPicsActive >= 0 && getGOPEntry(i).m_numRefPicsActive <= MAX_NUM_REF);
        histogram[getGOPEntry(i).m_numRefPicsActive]++;
    }

    Int maxHist = -1;
    Int bestPos = 0;
    for (Int i = 0; i <= MAX_NUM_REF; i++)
    {
        if (histogram[i] > maxHist)
        {
            maxHist = histogram[i];
            bestPos = i;
        }
    }

    assert(bestPos <= 15);
    m_cPPS.setNumRefIdxL0DefaultActive(bestPos);
    m_cPPS.setNumRefIdxL1DefaultActive(bestPos);
    m_cPPS.setTransquantBypassEnableFlag(getTransquantBypassEnableFlag());
    m_cPPS.setUseTransformSkip(m_useTransformSkip);
    m_cPPS.setLoopFilterAcrossTilesEnabledFlag(m_loopFilterAcrossTilesEnabledFlag);
}

//Function for initializing m_RPSList, a list of TComReferencePictureSet, based on the GOPEntry objects read from the config file.
Void TEncTop::xInitRPS()
{
    TComReferencePictureSet*      rps;

    m_cSPS.createRPSList(getGOPSize() + m_extraRPSs);
    TComRPSList* rpsList = m_cSPS.getRPSList();

    for (Int i = 0; i < getGOPSize() + m_extraRPSs; i++)
    {
        GOPEntry ge = getGOPEntry(i);
        rps = rpsList->getReferencePictureSet(i);
        rps->setNumberOfPictures(ge.m_numRefPics);
        rps->setNumRefIdc(ge.m_numRefIdc);
        Int numNeg = 0;
        Int numPos = 0;
        for (Int j = 0; j < ge.m_numRefPics; j++)
        {
            rps->setDeltaPOC(j, ge.m_referencePics[j]);
            rps->setUsed(j, ge.m_usedByCurrPic[j]);
            if (ge.m_referencePics[j] > 0)
            {
                numPos++;
            }
            else
            {
                numNeg++;
            }
        }

        rps->setNumberOfNegativePictures(numNeg);
        rps->setNumberOfPositivePictures(numPos);

        // handle inter RPS initialization from the config file.
        rps->setInterRPSPrediction(ge.m_interRPSPrediction > 0); // not very clean, converting anything > 0 to true.
        rps->setDeltaRIdxMinus1(0);                           // index to the Reference RPS is always the previous one.
        TComReferencePictureSet*     RPSRef = rpsList->getReferencePictureSet(i - 1); // get the reference RPS

        if (ge.m_interRPSPrediction == 2) // Automatic generation of the inter RPS idc based on the RIdx provided.
        {
            Int deltaRPS = getGOPEntry(i - 1).m_POC - ge.m_POC; // the ref POC - current POC
            Int numRefDeltaPOC = RPSRef->getNumberOfPictures();

            rps->setDeltaRPS(deltaRPS);     // set delta RPS
            rps->setNumRefIdc(numRefDeltaPOC + 1); // set the numRefIdc to the number of pictures in the reference RPS + 1.
            Int count = 0;
            for (Int j = 0; j <= numRefDeltaPOC; j++) // cycle through pics in reference RPS.
            {
                Int RefDeltaPOC = (j < numRefDeltaPOC) ? RPSRef->getDeltaPOC(j) : 0; // if it is the last decoded picture, set RefDeltaPOC = 0
                rps->setRefIdc(j, 0);
                for (Int k = 0; k < rps->getNumberOfPictures(); k++) // cycle through pics in current RPS.
                {
                    if (rps->getDeltaPOC(k) == (RefDeltaPOC + deltaRPS)) // if the current RPS has a same picture as the reference RPS.
                    {
                        rps->setRefIdc(j, (rps->getUsed(k) ? 1 : 2));
                        count++;
                        break;
                    }
                }
            }

            if (count != rps->getNumberOfPictures())
            {
                printf("Warning: Unable fully predict all delta POCs using the reference RPS index given in the config file.  Setting Inter RPS to false for this RPS.\n");
                rps->setInterRPSPrediction(0);
            }
        }
        else if (ge.m_interRPSPrediction == 1) // inter RPS idc based on the RefIdc values provided in config file.
        {
            rps->setDeltaRPS(ge.m_deltaRPS);
            rps->setNumRefIdc(ge.m_numRefIdc);
            for (Int j = 0; j < ge.m_numRefIdc; j++)
            {
                rps->setRefIdc(j, ge.m_refIdc[j]);
            }

            // the following code overwrite the deltaPOC and Used by current values read from the config file with the ones
            // computed from the RefIdc.  A warning is printed if they are not identical.
            numNeg = 0;
            numPos = 0;
            TComReferencePictureSet RPSTemp; // temporary variable

            for (Int j = 0; j < ge.m_numRefIdc; j++)
            {
                if (ge.m_refIdc[j])
                {
                    Int deltaPOC = ge.m_deltaRPS + ((j < RPSRef->getNumberOfPictures()) ? RPSRef->getDeltaPOC(j) : 0);
                    RPSTemp.setDeltaPOC((numNeg + numPos), deltaPOC);
                    RPSTemp.setUsed((numNeg + numPos), ge.m_refIdc[j] == 1 ? 1 : 0);
                    if (deltaPOC < 0)
                    {
                        numNeg++;
                    }
                    else
                    {
                        numPos++;
                    }
                }
            }

            if (numNeg != rps->getNumberOfNegativePictures())
            {
                printf("Warning: number of negative pictures in RPS is different between intra and inter RPS specified in the config file.\n");
                rps->setNumberOfNegativePictures(numNeg);
                rps->setNumberOfPositivePictures(numNeg + numPos);
            }
            if (numPos != rps->getNumberOfPositivePictures())
            {
                printf("Warning: number of positive pictures in RPS is different between intra and inter RPS specified in the config file.\n");
                rps->setNumberOfPositivePictures(numPos);
                rps->setNumberOfPositivePictures(numNeg + numPos);
            }
            RPSTemp.setNumberOfPictures(numNeg + numPos);
            RPSTemp.setNumberOfNegativePictures(numNeg);
            RPSTemp.sortDeltaPOC(); // sort the created delta POC before comparing
            // check if Delta POC and Used are the same
            // print warning if they are not.
            for (Int j = 0; j < ge.m_numRefIdc; j++)
            {
                if (RPSTemp.getDeltaPOC(j) != rps->getDeltaPOC(j))
                {
                    printf("Warning: delta POC is different between intra RPS and inter RPS specified in the config file.\n");
                    rps->setDeltaPOC(j, RPSTemp.getDeltaPOC(j));
                }
                if (RPSTemp.getUsed(j) != rps->getUsed(j))
                {
                    printf("Warning: Used by Current in RPS is different between intra and inter RPS specified in the config file.\n");
                    rps->setUsed(j, RPSTemp.getUsed(j));
                }
            }
        }
    }
}

// This is a function that
// determines what Reference Picture Set to use
// for a specific slice (with POC = POCCurr)
Void TEncTop::selectReferencePictureSet(TComSlice* slice, Int POCCurr, Int GOPid)
{
    slice->setRPSidx(GOPid);

    for (Int extraNum = m_iGOPSize; extraNum < m_extraRPSs + m_iGOPSize; extraNum++)
    {
        if (m_uiIntraPeriod > 0 && getDecodingRefreshType() > 0)
        {
            Int POCIndex = POCCurr % m_uiIntraPeriod;
            if (POCIndex == 0)
            {
                POCIndex = m_uiIntraPeriod;
            }
            if (POCIndex == m_GOPList[extraNum].m_POC)
            {
                slice->setRPSidx(extraNum);
            }
        }
        else
        {
            if (POCCurr == m_GOPList[extraNum].m_POC)
            {
                slice->setRPSidx(extraNum);
            }
        }
    }

    slice->setRPS(getSPS()->getRPSList()->getReferencePictureSet(slice->getRPSidx()));
    slice->getRPS()->setNumberOfPictures(slice->getRPS()->getNumberOfNegativePictures() + slice->getRPS()->getNumberOfPositivePictures());
}

Int TEncTop::getReferencePictureSetIdxForSOP(TComSlice* slice, Int POCCurr, Int GOPid)
{
    int rpsIdx = GOPid;

    for (Int extraNum = m_iGOPSize; extraNum < m_extraRPSs + m_iGOPSize; extraNum++)
    {
        if (m_uiIntraPeriod > 0 && getDecodingRefreshType() > 0)
        {
            Int POCIndex = POCCurr % m_uiIntraPeriod;
            if (POCIndex == 0)
            {
                POCIndex = m_uiIntraPeriod;
            }
            if (POCIndex == m_GOPList[extraNum].m_POC)
            {
                rpsIdx = extraNum;
            }
        }
        else
        {
            if (POCCurr == m_GOPList[extraNum].m_POC)
            {
                rpsIdx = extraNum;
            }
        }
    }

    return rpsIdx;
}

//! \}
