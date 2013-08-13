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
#include "primitives.h"
#include "slicetype.h"
#include "common.h"

#include <limits.h>
#include <math.h> // sqrt

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TEncTop::TEncTop()
{
    m_pocLast = -1;
    m_framesToBeEncoded = INT_MAX;
    m_picsQueued = 0;
    m_picsEncoded = 0;
    m_maxRefPicNum = 0;
    m_lookahead = NULL;
    m_GOPEncoder = NULL;
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
    if (!x265::primitives.sad[0])
    {
        printf("Primitives must be initialized before encoder is created\n");
        // we call exit() here because this should be an impossible condition when
        // using our public API, and indicates a serious bug.
        exit(1);
    }

    // create processing unit classes
    m_GOPEncoder = new TEncGOP;
    if (m_GOPEncoder)
    {
        m_GOPEncoder->create();
    }

    m_lookahead = new x265::Lookahead(&param);
}

Void TEncTop::destroy()
{
    // destroy processing unit classes
    if (m_GOPEncoder)
    {
        m_GOPEncoder->destroy();
        delete m_GOPEncoder;
    }

    while (!m_picList.empty())
    {
        TComPic* pic = m_picList.popFront();
        pic->destroy();
        delete pic;
    }

    if (m_recon)
        delete [] m_recon;

    if (m_lookahead)
        delete m_lookahead;

    if (m_threadPool)
        m_threadPool->release();
}

Void TEncTop::init()
{
    if (m_GOPEncoder)
    {
        m_GOPEncoder->init(this);
    }

    int maxGOP = X265_MAX(1, getGOPSize()) + getGOPSize();
    m_recon = new x265_picture_t[maxGOP];

    // pre-allocate a mini-GOP of TComPic
    for (int i = 0; i < maxGOP; i++)
    {
        TComPic *pic = new TComPic;
        pic->create(param.sourceWidth, param.sourceHeight, g_maxCUWidth, g_maxCUHeight, g_maxCUDepth,
                    getConformanceWindow(), getDefaultDisplayWindow(), param.bframes);
        if (param.bEnableSAO)
        {
            // TODO: these should be allocated on demand within the encoder
            pic->getPicSym()->allocSaoParam(m_GOPEncoder->m_frameEncoders->getSAO());
        }
        pic->getSlice()->setPOC(MAX_INT);
        m_picList.pushBack(pic);
    }

    m_analyzeI.setFrmRate(param.frameRate);
    m_analyzeP.setFrmRate(param.frameRate);
    m_analyzeB.setFrmRate(param.frameRate);
    m_analyzeAll.setFrmRate(param.frameRate);
}

void TEncTop::addPicture(const x265_picture_t *picture)
{
    TComSlice::sortPicList(m_picList);

    TComList<TComPic*>::iterator iterPic = m_picList.begin();
    while (iterPic != m_picList.end())
    {
        TComPic *pic = *(iterPic++);
        if (pic->getSlice()->isReferenced() == false)
        {
            pic->getSlice()->setPOC(++m_pocLast);
            pic->getPicYuvOrg()->copyFromPicture(*picture);
            pic->getPicYuvRec()->clearExtendedFlag();
            pic->getSlice()->setReferenced(true);
            pic->m_lowres.init(pic->getPicYuvOrg());
            return;
        }
    }

    assert(!"No room for added picture");
}

/**
 \param   flush               force encoder to encode a partial GOP
 \param   pic                 input original YUV picture or NULL
 \param   pic_out             pointer to list of reconstructed pictures
 \param   accessUnitsOut      list of output bitstreams
 \retval                      number of returned recon pictures
 */
int TEncTop::encode(Bool flush, const x265_picture_t* pic, x265_picture_t **pic_out, std::list<AccessUnit>& accessUnitsOut)
{
    if (pic)
    {
        addPicture(pic);
        m_picsQueued++;
    }

    int batchSize = m_picsEncoded == 0 ? 1 : getGOPSize();

    if (!flush && m_picsQueued < batchSize)
        // Wait until we have a full batch of pictures
        return 0;
    else if (m_picsQueued == 0)
        // nothing to flush
        return 0;
    if (flush)
        m_framesToBeEncoded = m_picsEncoded + m_picsQueued;

    m_GOPEncoder->compressGOP(m_pocLast, m_picsQueued, m_picList, accessUnitsOut);

    if (pic_out)
    {
        for (Int i = 0; i < m_picsQueued; i++)
        {
            TComList<TComPic*>::iterator iterPic = m_picList.begin();
            while (iterPic != m_picList.end() && (*iterPic)->getPOC() != (m_picsEncoded + i))
            {
                iterPic++;
            }

            TComPicYuv *recpic = (*iterPic)->getPicYuvRec();
            x265_picture_t& recon = m_recon[i];
            recon.planes[0] = recpic->getLumaAddr();
            recon.stride[0] = recpic->getStride();
            recon.planes[1] = recpic->getCbAddr();
            recon.stride[1] = recpic->getCStride();
            recon.planes[2] = recpic->getCrAddr();
            recon.stride[2] = recpic->getCStride();
            recon.bitDepth = sizeof(Pel) * 8;
            recon.poc = m_picsEncoded + i;
        }

        *pic_out = m_recon;
    }

    int ret = m_picsQueued;
    m_picsEncoded += m_picsQueued;
    m_picsQueued = 0;

    return ret;
}

int TEncTop::getStreamHeaders(std::list<AccessUnit>& accessUnitsOut)
{
    return m_GOPEncoder->getStreamHeaders(accessUnitsOut);
}

Double TEncTop::printSummary()
{
    if (param.logLevel >= X265_LOG_INFO)
    {
        m_analyzeI.printOut('i');
        m_analyzeP.printOut('p');
        m_analyzeB.printOut('b');
        m_analyzeAll.printOut('a');
    }

#if _SUMMARY_OUT_
    m_analyzeAll.printSummaryOut();
#endif
#if _SUMMARY_PIC_
    m_analyzeI.printSummary('I');
    m_analyzeP.printSummary('P');
    m_analyzeB.printSummary('B');
#endif

    return (m_analyzeAll.getPsnrY() * 6 + m_analyzeAll.getPsnrU() + m_analyzeAll.getPsnrV()) / (8 * m_analyzeAll.getNumPic());
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

    Int minCUSize = pcSPS->getMaxCUWidth() >> (pcSPS->getMaxCUDepth() - g_addCUDepth);
    Int log2MinCUSize = 0;
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

    Int i;

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

Void TEncTop::xInitPPS(TComPPS *pcPPS)
{
    pcPPS->setConstrainedIntraPred(param.bEnableConstrainedIntra);
    Bool bUseDQP = (getMaxCuDQPDepth() > 0) ? true : false;

    Int lowestQP = -(6 * (X265_DEPTH - 8)); //m_cSPS.getQpBDOffsetY();

    if (getUseLossless())
    {
        if ((getMaxCuDQPDepth() == 0) && (param.qp == lowestQP))
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
    pcPPS->setUseTransformSkip(param.bEnableTransformSkip);
    pcPPS->setLoopFilterAcrossTilesEnabledFlag(m_loopFilterAcrossTilesEnabledFlag);
}

//Function for initializing m_RPSList, a list of TComReferencePictureSet, based on the GOPEntry objects read from the config file.
Void TEncTop::xInitRPS(TComSPS *pcSPS)
{
    TComReferencePictureSet* rps;

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
