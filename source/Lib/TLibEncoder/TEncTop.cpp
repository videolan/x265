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
#include "TLibCommon/ContextModel.h"
#include "TEncTop.h"
#include "TEncPic.h"
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

    m_recon = new x265_picture_t[m_iGOPSize];
}

Void TEncTop::destroy()
{
    // destroy processing unit classes
    m_cGOPEncoder.destroy();
    if (getUseSAO())
    {
        m_cEncSAO.destroy();
        m_cEncSAO.destroyEncBuffer();
    }
    m_cRateCtrl.destroy();

    if (m_recon)
        delete [] m_recon;
    deletePicBuffer();

    // destroy ROM
    destroyROM();

    if (m_threadPool)
        m_threadPool->Release();
}

Void TEncTop::init()
{
    // initialize processing unit classes
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
 \param   flush               force encoder to encode a partial GOP
 \param   pic                 input original YUV picture
 \param   pic_out             pointer to list of reconstructed pictures
 \param   accessUnitsOut      list of output bitstreams
 \retval                      number of returned recon pictures
 */
int TEncTop::encode(Bool flush, const x265_picture_t* pic, x265_picture_t **pic_out, std::list<AccessUnit>& accessUnitsOut)
{
    if (pic)
    {
        m_iNumPicRcvd++;
        
        // get original YUV
        TComPic* pcPicCurr = xGetNewPicBuffer();
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
    m_cGOPEncoder.compressGOP(m_iPOCLast, m_iNumPicRcvd, m_cListPic, accessUnitsOut);

    if (pic_out)
    {
        *pic_out = m_recon;
        // make references to the last N TComPic's recon frames
        TComList<TComPic*>::iterator iterPic = m_cListPic.end();
        for (int i = 0; i < m_iNumPicRcvd; i++)
            iterPic--;
        for (int i = 0; i < m_iNumPicRcvd; i++)
        {
            TComPicYuv *recpic = (*iterPic++)->getPicYuvRec();
            x265_picture_t& recon = m_recon[i];
            recon.planes[0] = recpic->getLumaAddr();
            recon.stride[0] = recpic->getStride();
            recon.planes[1] = recpic->getCbAddr();
            recon.stride[1] = recpic->getCStride();
            recon.planes[2] = recpic->getCrAddr();
            recon.stride[2] = recpic->getCStride();
            recon.bitDepth = sizeof(Pel) * 8;
        }
    }
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
TComPic* TEncTop::xGetNewPicBuffer()
{
    TComSlice::sortPicList(m_cListPic);
    TComPic *pcPic = NULL;

    if (m_cListPic.size() >= (UInt)(m_iGOPSize + getMaxDecPicBuffering(MAX_TLAYER - 1) + 2))
    {
        TComList<TComPic*>::iterator iterPic  = m_cListPic.begin();
        Int iSize = Int(m_cListPic.size());
        for (Int i = 0; i < iSize; i++)
        {
            pcPic = *(iterPic++);
            if (pcPic->getSlice()->isReferenced() == false)
            {
                break;
            }
        }
    }
    if (pcPic == NULL)
    {
        if (getUseAdaptiveQP())
        {
            TEncPic* pcEPic = new TEncPic;
            pcEPic->create(m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth, /* m_cPPS. */ getMaxCuDQPDepth() + 1,
                           m_conformanceWindow, m_defaultDisplayWindow);
            pcPic = pcEPic;
        }
        else
        {
            pcPic = new TComPic;

            pcPic->create(m_iSourceWidth, m_iSourceHeight, g_uiMaxCUWidth, g_uiMaxCUHeight, g_uiMaxCUDepth,
                          m_conformanceWindow, m_defaultDisplayWindow);
        }
        if (getUseSAO())
        {
            pcPic->getPicSym()->allocSaoParam(&m_cEncSAO);
        }
        m_cListPic.pushBack(pcPic);
    }
    pcPic->getSlice()->setPOC(++m_iPOCLast);
    pcPic->getPicYuvRec()->setBorderExtension(false);
    return pcPic;
}

Void TEncTop::xInitSPS(TComSPS *pcSPS)
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

    pcSPS->setPicWidthInLumaSamples(m_iSourceWidth);
    pcSPS->setPicHeightInLumaSamples(m_iSourceHeight);
    pcSPS->setConformanceWindow(m_conformanceWindow);
    pcSPS->setMaxCUWidth(g_uiMaxCUWidth);
    pcSPS->setMaxCUHeight(g_uiMaxCUHeight);
    pcSPS->setMaxCUDepth(g_uiMaxCUDepth);

    Int minCUSize = pcSPS->getMaxCUWidth() >> (pcSPS->getMaxCUDepth() - g_uiAddCUDepth);
    Int log2MinCUSize = 0;
    while (minCUSize > 1)
    {
        minCUSize >>= 1;
        log2MinCUSize++;
    }

    pcSPS->setLog2MinCodingBlockSize(log2MinCUSize);
    pcSPS->setLog2DiffMaxMinCodingBlockSize(pcSPS->getMaxCUDepth() - g_uiAddCUDepth);

    pcSPS->setPCMLog2MinSize(m_uiPCMLog2MinSize);
    pcSPS->setUsePCM(m_usePCM);
    pcSPS->setPCMLog2MaxSize(m_pcmLog2MaxSize);

    pcSPS->setQuadtreeTULog2MaxSize(m_uiQuadtreeTULog2MaxSize);
    pcSPS->setQuadtreeTULog2MinSize(m_uiQuadtreeTULog2MinSize);
    pcSPS->setQuadtreeTUMaxDepthInter(m_uiQuadtreeTUMaxDepthInter);
    pcSPS->setQuadtreeTUMaxDepthIntra(m_uiQuadtreeTUMaxDepthIntra);

    pcSPS->setTMVPFlagsPresent(false);
    pcSPS->setUseLossless(m_useLossless);

    pcSPS->setMaxTrSize(1 << m_uiQuadtreeTULog2MaxSize);

    Int i;

    for (i = 0; i < g_uiMaxCUDepth - g_uiAddCUDepth; i++)
    {
        pcSPS->setAMPAcc(i, m_useAMP);
    }

    pcSPS->setUseAMP(m_useAMP);

    for (i = g_uiMaxCUDepth - g_uiAddCUDepth; i < g_uiMaxCUDepth; i++)
    {
        pcSPS->setAMPAcc(i, 0);
    }

    pcSPS->setBitDepthY(g_bitDepthY);
    pcSPS->setBitDepthC(g_bitDepthC);

    pcSPS->setQpBDOffsetY(6 * (g_bitDepthY - 8));
    pcSPS->setQpBDOffsetC(6 * (g_bitDepthC - 8));

    pcSPS->setUseSAO(m_bUseSAO);

    pcSPS->setMaxTLayers(m_maxTempLayer);
    pcSPS->setTemporalIdNestingFlag((m_maxTempLayer == 1) ? true : false);
    for (i = 0; i < pcSPS->getMaxTLayers(); i++)
    {
        pcSPS->setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
        pcSPS->setNumReorderPics(m_numReorderPics[i], i);
    }

    pcSPS->setPCMBitDepthLuma(g_uiPCMBitDepthLuma);
    pcSPS->setPCMBitDepthChroma(g_uiPCMBitDepthChroma);
    pcSPS->setPCMFilterDisableFlag(m_bPCMFilterDisableFlag);

    pcSPS->setScalingListFlag((m_useScalingListId == 0) ? 0 : 1);

    pcSPS->setUseStrongIntraSmoothing(m_useStrongIntraSmoothing);

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

Void TEncTop::xInitPPS(TComPPS *pcPPS)
{
    pcPPS->setConstrainedIntraPred(m_bUseConstrainedIntraPred);
    Bool bUseDQP = (getMaxCuDQPDepth() > 0) ? true : false;

    Int lowestQP = -(6 * (g_bitDepthY - 8)); //m_cSPS.getQpBDOffsetY();

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
        pcPPS->setUseDQP(true);
        pcPPS->setMaxCuDQPDepth(m_iMaxCuDQPDepth);
        pcPPS->setMinCuDQPSize(pcPPS->getSPS()->getMaxCUWidth() >> (pcPPS->getMaxCuDQPDepth()));
    }
    else
    {
        pcPPS->setUseDQP(false);
        pcPPS->setMaxCuDQPDepth(0);
        pcPPS->setMinCuDQPSize(pcPPS->getSPS()->getMaxCUWidth() >> (pcPPS->getMaxCuDQPDepth()));
    }

    if (m_RCEnableRateControl)
    {
        pcPPS->setUseDQP(true);
        pcPPS->setMaxCuDQPDepth(0);
        pcPPS->setMinCuDQPSize(pcPPS->getSPS()->getMaxCUWidth() >> (pcPPS->getMaxCuDQPDepth()));
    }

    pcPPS->setChromaCbQpOffset(m_chromaCbQpOffset);
    pcPPS->setChromaCrQpOffset(m_chromaCrQpOffset);

    pcPPS->setEntropyCodingSyncEnabledFlag(m_iWaveFrontSynchro > 0);
    pcPPS->setUseWP(m_useWeightedPred);
    pcPPS->setWPBiPred(m_useWeightedBiPred);
    pcPPS->setOutputFlagPresentFlag(false);
    pcPPS->setSignHideFlag(getSignHideFlag());
    pcPPS->setDeblockingFilterControlPresentFlag(m_DeblockingFilterControlPresent);
    pcPPS->setLog2ParallelMergeLevelMinus2(m_log2ParallelMergeLevelMinus2);
    pcPPS->setCabacInitPresentFlag(CABAC_INIT_PRESENT_FLAG);
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
    pcPPS->setNumRefIdxL0DefaultActive(bestPos);
    pcPPS->setNumRefIdxL1DefaultActive(bestPos);
    pcPPS->setTransquantBypassEnableFlag(getTransquantBypassEnableFlag());
    pcPPS->setUseTransformSkip(m_useTransformSkip);
    pcPPS->setLoopFilterAcrossTilesEnabledFlag(m_loopFilterAcrossTilesEnabledFlag);
}

//Function for initializing m_RPSList, a list of TComReferencePictureSet, based on the GOPEntry objects read from the config file.
Void TEncTop::xInitRPS(TComSPS *pcSPS)
{
    TComReferencePictureSet*      rps;

    pcSPS->createRPSList(getGOPSize() + m_extraRPSs);
    TComRPSList* rpsList = pcSPS->getRPSList();

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
        rps->setDeltaRIdxMinus1(0);                              // index to the Reference RPS is always the previous one.
        TComReferencePictureSet* RPSRef = rpsList->getReferencePictureSet(i - 1); // get the reference RPS

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

//! \}
