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

/** \file     TEncGOP.cpp
    \brief    GOP encoder class
*/

#include "TEncTop.h"
#include "TEncGOP.h"
#include "TEncAnalyze.h"
#include "TLibCommon/SEI.h"
#include "TLibCommon/NAL.h"
#include "PPA/ppa.h"
#include "NALwrite.h"
#include "common.h"

#include <list>
#include <algorithm>
#include <functional>
#include <time.h>
#include <math.h>

using namespace std;
using namespace x265;

static const char *digestToString(const unsigned char digest[3][16], int numChar);

enum SCALING_LIST_PARAMETER
{
    SCALING_LIST_OFF,
    SCALING_LIST_DEFAULT,
};

TEncGOP::TEncGOP()
{
    m_cfg             = NULL;
    m_frameEncoder    = NULL;
    m_top             = NULL;

    m_lastBPSEI       = 0;
    m_totalCoded      = 0;
}

Void TEncGOP::destroy()
{
    if (m_frameEncoder)
    {
        m_frameEncoder->destroy();
        delete m_frameEncoder;
    }
}

Void TEncGOP::init(TEncTop* top)
{
    m_top = top;
    m_cfg = top;

    // initialize SPS
    top->xInitSPS(&m_sps);

    // initialize PPS
    m_pps.setSPS(&m_sps);

    top->xInitPPS(&m_pps);
    top->xInitRPS(&m_sps);

    m_sps.setNumLongTermRefPicSPS(0);
    if (m_cfg->getPictureTimingSEIEnabled() || m_cfg->getDecodingUnitInfoSEIEnabled())
    {
        m_sps.getVuiParameters()->getHrdParameters()->setNumDU(0);
        m_sps.setHrdParameters(m_cfg->param.frameRate, 0, m_cfg->param.rc.bitrate, m_cfg->param.bframes > 0);
    }
    if (m_cfg->getBufferingPeriodSEIEnabled() || m_cfg->getPictureTimingSEIEnabled() || m_cfg->getDecodingUnitInfoSEIEnabled())
    {
        m_sps.getVuiParameters()->setHrdParametersPresentFlag(true);
    }

    m_sps.setTMVPFlagsPresent(true);

    int numRows = (m_cfg->param.sourceHeight + m_sps.getMaxCUHeight() - 1) / m_sps.getMaxCUHeight();
    m_frameEncoder = new x265::FrameEncoder(ThreadPool::getThreadPool());
    m_frameEncoder->init(top, numRows);

    // set default slice level flag to the same as SPS level flag
    if (m_cfg->getUseScalingListId() == SCALING_LIST_OFF)
    {
        m_frameEncoder->setFlatScalingList();
        m_frameEncoder->setUseScalingList(false);
        m_sps.setScalingListPresentFlag(false);
        m_pps.setScalingListPresentFlag(false);
    }
    else if (m_cfg->getUseScalingListId() == SCALING_LIST_DEFAULT)
    {
        m_sps.setScalingListPresentFlag(false);
        m_pps.setScalingListPresentFlag(false);
        m_frameEncoder->setScalingList(m_top->getScalingList());
        m_frameEncoder->setUseScalingList(true);
    }
    else
    {
        printf("error : ScalingList == %d not supported\n", m_top->getUseScalingListId());
        assert(0);
    }
}

int TEncGOP::getStreamHeaders(AccessUnit& accessUnit)
{
    TEncEntropy* entropyCoder = m_frameEncoder->getEntropyCoder(0);
    TEncCavlc*   cavlcCoder   = m_frameEncoder->getCavlcCoder();

    entropyCoder->setEntropyCoder(cavlcCoder, NULL);

    /* headers for start of bitstream */
    OutputNALUnit nalu(NAL_UNIT_VPS);
    entropyCoder->setBitstream(&nalu.m_Bitstream);
    entropyCoder->encodeVPS(m_top->getVPS());
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));

    nalu = NALUnit(NAL_UNIT_SPS);
    entropyCoder->setBitstream(&nalu.m_Bitstream);
    entropyCoder->encodeSPS(&m_sps);
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));

    nalu = NALUnit(NAL_UNIT_PPS);
    entropyCoder->setBitstream(&nalu.m_Bitstream);
    entropyCoder->encodePPS(&m_pps);
    writeRBSPTrailingBits(nalu.m_Bitstream);
    accessUnit.push_back(new NALUnitEBSP(nalu));

    if (m_cfg->getActiveParameterSetsSEIEnabled())
    {
        SEIActiveParameterSets *sei = xCreateSEIActiveParameterSets();

        entropyCoder->setBitstream(&nalu.m_Bitstream);
        m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, &m_sps);
        writeRBSPTrailingBits(nalu.m_Bitstream);
        accessUnit.push_back(new NALUnitEBSP(nalu));
        delete sei;
    }

    if (m_cfg->getDisplayOrientationSEIAngle())
    {
        SEIDisplayOrientation *sei = xCreateSEIDisplayOrientation();

        nalu = NALUnit(NAL_UNIT_PREFIX_SEI);
        entropyCoder->setBitstream(&nalu.m_Bitstream);
        m_seiWriter.writeSEImessage(nalu.m_Bitstream, *sei, &m_sps);
        writeRBSPTrailingBits(nalu.m_Bitstream);
        accessUnit.push_back(new NALUnitEBSP(nalu));
        delete sei;
    }
    return 0;
}

Void TEncGOP::compressFrame(TComPic *pic, AccessUnit& accessUnit)
{
    PPAScopeEvent(compressFrame);

    x265::FrameEncoder*   frameEncoder = m_frameEncoder;
    TEncEntropy*          entropyCoder = frameEncoder->getEntropyCoder(0);
    TEncCavlc*            cavlcCoder   = frameEncoder->getCavlcCoder();
    TEncSbac*             sbacCoder    = frameEncoder->getSingletonSbac();
    TEncBinCABAC*         binCABAC     = frameEncoder->getBinCABAC();
    TComBitCounter*       bitCounter   = frameEncoder->getBitCounter();
    TEncSampleAdaptiveOffset* sao      = frameEncoder->getSAO();
    TComSlice*            slice        = pic->getSlice();

    Int                   picSptDpbOutputDuDelay = 0;
    UInt*                 accumBitsDU = NULL;
    UInt*                 accumNalsDU = NULL;
    UInt                  oneBitstreamPerSliceLength = 0; // TODO: Remove
    TComOutputBitstream*  bitstreamRedirect = new TComOutputBitstream;
    TComOutputBitstream*  outStreams = NULL;
    Bool bBufferingPeriodSEIPresentInAU = false;
    Bool bPictureTimingSEIPresentInAU = false;

    SEIPictureTiming pictureTimingSEI;
    SEIDecodingUnitInfo decodingUnitInfoSEI;

    Int numSubstreams = m_cfg->param.bEnableWavefront ? pic->getPicSym()->getFrameHeightInCU() : 1;
    outStreams = new TComOutputBitstream[numSubstreams];

    // Slice compression, most of the hard work is done here
    // frame is compressed in a wave-front pattern if WPP is enabled. Loop filter runs as a
    // wave-front behind the CU compression and reconstruction
    frameEncoder->compressSlice(pic);

    // SAO parameter estimation using non-deblocked pixels for LCU bottom and right boundary areas
    if (m_cfg->param.saoLcuBasedOptimization && m_cfg->param.saoLcuBoundary)
    {
        sao->resetStats();
        sao->calcSaoStatsCu_BeforeDblk(pic);
    }

    //-- Loop filter
    if (m_cfg->param.bEnableLoopFilter)
    {
        frameEncoder->wait_lft();
    }

    if (m_sps.getUseSAO())
    {
        pic->createNonDBFilterInfo(slice->getSliceCurEndCUAddr(), 0);
        sao->createPicSaoInfo(pic);
    }

    /* write various header sets. */

    entropyCoder->setEntropyCoder(cavlcCoder, slice);

    if ((m_cfg->getPictureTimingSEIEnabled() || m_cfg->getDecodingUnitInfoSEIEnabled()) &&
        (m_sps.getVuiParametersPresentFlag()) &&
        ((m_sps.getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag())
            || (m_sps.getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag())))
    {
        if (m_sps.getVuiParameters()->getHrdParameters()->getSubPicCpbParamsPresentFlag())
        {
            UInt numDU = m_sps.getVuiParameters()->getHrdParameters()->getNumDU();
            pictureTimingSEI.m_numDecodingUnitsMinus1 = (numDU - 1);
            pictureTimingSEI.m_duCommonCpbRemovalDelayFlag = false;

            if (pictureTimingSEI.m_numNalusInDuMinus1 == NULL)
            {
                pictureTimingSEI.m_numNalusInDuMinus1 = new UInt[numDU];
            }
            if (pictureTimingSEI.m_duCpbRemovalDelayMinus1 == NULL)
            {
                pictureTimingSEI.m_duCpbRemovalDelayMinus1  = new UInt[numDU];
            }
            if (accumBitsDU == NULL)
            {
                accumBitsDU = new UInt[numDU];
            }
            if (accumNalsDU == NULL)
            {
                accumNalsDU = new UInt[numDU];
            }
        }
        pictureTimingSEI.m_auCpbRemovalDelay = std::max<Int>(1, m_totalCoded - m_lastBPSEI); // Syntax element signaled as minus, hence the .
        pictureTimingSEI.m_picDpbOutputDelay = m_sps.getNumReorderPics(0) + slice->getPOC() - m_totalCoded;
        Int factor = m_sps.getVuiParameters()->getHrdParameters()->getTickDivisorMinus2() + 2;
        pictureTimingSEI.m_picDpbOutputDuDelay = factor * pictureTimingSEI.m_picDpbOutputDelay;
        if (m_cfg->getDecodingUnitInfoSEIEnabled())
        {
            picSptDpbOutputDuDelay = factor * pictureTimingSEI.m_picDpbOutputDelay;
        }
    }

    if ((m_cfg->getBufferingPeriodSEIEnabled()) && (slice->getSliceType() == I_SLICE) &&
        (m_sps.getVuiParametersPresentFlag()) &&
        ((m_sps.getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag())
            || (m_sps.getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag())))
    {
        OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
        entropyCoder->setEntropyCoder(cavlcCoder, slice);
        entropyCoder->setBitstream(&nalu.m_Bitstream);

        SEIBufferingPeriod sei_buffering_period;

        UInt initialCpbRemovalDelay = (90000 / 2);              // 0.5 sec
        sei_buffering_period.m_initialCpbRemovalDelay[0][0]     = initialCpbRemovalDelay;
        sei_buffering_period.m_initialCpbRemovalDelayOffset[0][0]     = initialCpbRemovalDelay;
        sei_buffering_period.m_initialCpbRemovalDelay[0][1]     = initialCpbRemovalDelay;
        sei_buffering_period.m_initialCpbRemovalDelayOffset[0][1]     = initialCpbRemovalDelay;

        Double dTmp = (Double)m_sps.getVuiParameters()->getTimingInfo()->getNumUnitsInTick() / (Double)m_sps.getVuiParameters()->getTimingInfo()->getTimeScale();

        UInt tmp = (UInt)(dTmp * 90000.0);
        initialCpbRemovalDelay -= tmp;
        initialCpbRemovalDelay -= tmp / (m_sps.getVuiParameters()->getHrdParameters()->getTickDivisorMinus2() + 2);
        sei_buffering_period.m_initialAltCpbRemovalDelay[0][0]  = initialCpbRemovalDelay;
        sei_buffering_period.m_initialAltCpbRemovalDelayOffset[0][0]  = initialCpbRemovalDelay;
        sei_buffering_period.m_initialAltCpbRemovalDelay[0][1]  = initialCpbRemovalDelay;
        sei_buffering_period.m_initialAltCpbRemovalDelayOffset[0][1]  = initialCpbRemovalDelay;

        sei_buffering_period.m_rapCpbParamsPresentFlag = 0;
        //for the concatenation, it can be set to one during splicing.
        sei_buffering_period.m_concatenationFlag = 0;
        //since the temporal layer HRD is not ready, we assumed it is fixed
        sei_buffering_period.m_auCpbRemovalDelayDelta = 1;
        sei_buffering_period.m_cpbDelayOffset = 0;
        sei_buffering_period.m_dpbDelayOffset = 0;

        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei_buffering_period, slice->getSPS());
        writeRBSPTrailingBits(nalu.m_Bitstream);
        {
            UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
            UInt offsetPosition = 0;
            AccessUnit::iterator it = accessUnit.begin();
            for (int j = 0; j < seiPositionInAu + offsetPosition; j++)
            {
                it++;
            }

            accessUnit.insert(it, new NALUnitEBSP(nalu));
            bBufferingPeriodSEIPresentInAU = true;
        }

        m_lastBPSEI = m_totalCoded;
    }
    if ((m_cfg->getRecoveryPointSEIEnabled()) && (slice->getSliceType() == I_SLICE))
    {
        if (m_cfg->getGradualDecodingRefreshInfoEnabled() && !slice->getRapPicFlag())
        {
            // Gradual decoding refresh SEI
            OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
            entropyCoder->setEntropyCoder(cavlcCoder, slice);
            entropyCoder->setBitstream(&nalu.m_Bitstream);

            SEIGradualDecodingRefreshInfo seiGradualDecodingRefreshInfo;
            seiGradualDecodingRefreshInfo.m_gdrForegroundFlag = true; // Indicating all "foreground"

            m_seiWriter.writeSEImessage(nalu.m_Bitstream, seiGradualDecodingRefreshInfo, slice->getSPS());
            writeRBSPTrailingBits(nalu.m_Bitstream);
            accessUnit.push_back(new NALUnitEBSP(nalu));
        }
        // Recovery point SEI
        OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);
        entropyCoder->setEntropyCoder(cavlcCoder, slice);
        entropyCoder->setBitstream(&nalu.m_Bitstream);

        SEIRecoveryPoint sei_recovery_point;
        sei_recovery_point.m_recoveryPocCnt    = 0;
        sei_recovery_point.m_exactMatchingFlag = (slice->getPOC() == 0) ? (true) : (false);
        sei_recovery_point.m_brokenLinkFlag    = false;

        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei_recovery_point, slice->getSPS());
        writeRBSPTrailingBits(nalu.m_Bitstream);
        accessUnit.push_back(new NALUnitEBSP(nalu));
    }

    /* use the main bitstream buffer for storing the marshaled picture */
    entropyCoder->setBitstream(NULL);

    if (m_sps.getUseSAO())
    {
        // set entropy coder for RD
        entropyCoder->setEntropyCoder(sbacCoder, slice);
        entropyCoder->resetEntropy();
        entropyCoder->setBitstream(bitCounter);

        // CHECK_ME: I think the SAO uses a temp Sbac only, so I always use [0], am I right?
        sao->startSaoEnc(pic, entropyCoder, frameEncoder->getRDGoOnSbacCoder(0));

        SAOParam& saoParam = *slice->getPic()->getPicSym()->getSaoParam();
        sao->SAOProcess(&saoParam, pic->getSlice()->getLambdaLuma(), pic->getSlice()->getLambdaChroma(), pic->getSlice()->getDepth());
        sao->endSaoEnc();
        sao->PCMLFDisableProcess(pic);

        pic->getSlice()->setSaoEnabledFlag((saoParam.bSaoFlag[0] == 1) ? true : false);
    }

    // Reconstruction slice
    slice->setNextSlice(true);
    slice->setRPS(pic->getSlice()->getRPS());
    slice->setRPSidx(pic->getSlice()->getRPSidx());
    frameEncoder->determineSliceBounds(pic, true);

    slice->allocSubstreamSizes(numSubstreams);
    for (UInt ui = 0; ui < numSubstreams; ui++)
    {
        outStreams[ui].clear();
    }

    entropyCoder->setEntropyCoder(cavlcCoder, slice);
    entropyCoder->resetEntropy();

    /* start slice NALunit */
    OutputNALUnit nalu(slice->getNalUnitType(), 0);
    Bool sliceSegment = (!slice->isNextSlice());
    if (!sliceSegment)
    {
        oneBitstreamPerSliceLength = 0; // start of a new slice
    }
    entropyCoder->setBitstream(&nalu.m_Bitstream);
    entropyCoder->encodeSliceHeader(slice);

    // is it needed?
    if (!sliceSegment)
    {
        bitstreamRedirect->writeAlignOne();
    }
    else
    {
        // We've not completed our slice header info yet, do the alignment later.
    }
    sbacCoder->init((TEncBinIf*)binCABAC);
    entropyCoder->setEntropyCoder(sbacCoder, slice);
    entropyCoder->resetEntropy();
    frameEncoder->resetEntropy(slice);

    if (slice->isNextSlice())
    {
        // set entropy coder for writing
        sbacCoder->init((TEncBinIf*)binCABAC);
        frameEncoder->resetEntropy(slice);
        frameEncoder->getSbacCoder(0)->load(sbacCoder);
        //ALF is written in substream #0 with CABAC coder #0 (see ALF param encoding below)
        entropyCoder->setEntropyCoder(frameEncoder->getSbacCoder(0), slice); 
        entropyCoder->resetEntropy();

        // File writing
        if (!sliceSegment)
        {
            entropyCoder->setBitstream(bitstreamRedirect);
        }
        else
        {
            entropyCoder->setBitstream(&nalu.m_Bitstream);
        }

        // for now, override the TILES_DECODER setting in order to write substreams.
        entropyCoder->setBitstream(&outStreams[0]);
    }
    slice->setFinalized(true);

    sbacCoder->load(frameEncoder->getSbacCoder(0));

    slice->setTileOffstForMultES(oneBitstreamPerSliceLength);
    slice->setTileLocationCount(0);
    frameEncoder->encodeSlice(pic, outStreams);

    {
        // Construct the final bitstream by flushing and concatenating substreams.
        // The final bitstream is either nalu.m_Bitstream or pcBitstreamRedirect;
        UInt* substreamSizes = slice->getSubstreamSizes();
        for (UInt i = 0; i < numSubstreams; i++)
        {
            // Flush all substreams -- this includes empty ones.
            // Terminating bit and flush.
            entropyCoder->setEntropyCoder(frameEncoder->getSbacCoder(i), slice);
            entropyCoder->setBitstream(&outStreams[i]);
            entropyCoder->encodeTerminatingBit(1);
            entropyCoder->encodeSliceFinish();

            outStreams[i].writeByteAlignment(); // Byte-alignment in slice_data() at end of sub-stream

            // Byte alignment is necessary between tiles when tiles are independent.
            if (i + 1 < numSubstreams)
            {
                substreamSizes[i] = outStreams[i].getNumberOfWrittenBits() + (outStreams[i].countStartCodeEmulations() << 3);
            }
        }

        // Complete the slice header info.
        entropyCoder->setEntropyCoder(cavlcCoder, slice);
        entropyCoder->setBitstream(&nalu.m_Bitstream);
        entropyCoder->encodeTilesWPPEntryPoint(slice);

        // Substreams...
        Int nss = m_pps.getEntropyCodingSyncEnabledFlag() ? slice->getNumEntryPointOffsets() + 1 : numSubstreams;
        for (Int i = 0; i < nss; i++)
        {
            bitstreamRedirect->addSubstream(&outStreams[i]);
        }
    }

    // If current NALU is the first NALU of slice (containing slice header) and more NALUs exist (due to multiple dependent slices) then buffer it.
    // If current NALU is the last NALU of slice and a NALU was buffered, then (a) Write current NALU (b) Update an write buffered NALU at approproate location in NALU list.
    xAttachSliceDataToNalUnit(entropyCoder, nalu, bitstreamRedirect);
    accessUnit.push_back(new NALUnitEBSP(nalu));
    oneBitstreamPerSliceLength += nalu.m_Bitstream.getNumberOfWrittenBits(); // length of bitstream after byte-alignment

    if ((m_cfg->getPictureTimingSEIEnabled() || m_cfg->getDecodingUnitInfoSEIEnabled()) &&
        (m_sps.getVuiParametersPresentFlag()) &&
        ((m_sps.getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag())
            || (m_sps.getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag())) &&
        (m_sps.getVuiParameters()->getHrdParameters()->getSubPicCpbParamsPresentFlag()))
    {
        UInt numNalus = 0;
        UInt numRBSPBytes = 0;
        for (AccessUnit::const_iterator it = accessUnit.begin(); it != accessUnit.end(); it++)
        {
            UInt numRBSPBytes_nal = UInt((*it)->m_nalUnitData.str().size());
            if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
            {
                numRBSPBytes += numRBSPBytes_nal;
                numNalus++;
            }
        }

        accumBitsDU[0] = (numRBSPBytes << 3);
        accumNalsDU[0] = numNalus; // SEI not counted for bit count; hence shouldn't be counted for # of NALUs - only for consistency
    }

    if (m_sps.getUseSAO())
    {
        sao->destroyPicSaoInfo();
        pic->destroyNonDBFilterInfo();
    }
    pic->compressMotion();

    entropyCoder->setEntropyCoder(cavlcCoder, slice);

    calculateHashAndPSNR(pic, pic->getPicYuvRec(), accessUnit);

    if ((m_cfg->getPictureTimingSEIEnabled() || m_cfg->getDecodingUnitInfoSEIEnabled()) &&
        (m_sps.getVuiParametersPresentFlag()) &&
        ((m_sps.getVuiParameters()->getHrdParameters()->getNalHrdParametersPresentFlag())
            || (m_sps.getVuiParameters()->getHrdParameters()->getVclHrdParametersPresentFlag())))
    {
        TComVUI *vui = m_sps.getVuiParameters();
        TComHRD *hrd = vui->getHrdParameters();

        if (hrd->getSubPicCpbParamsPresentFlag())
        {
            Int i;
            UInt64 tmp64;
            UInt prev = 0;
            UInt numDU = (pictureTimingSEI.m_numDecodingUnitsMinus1 + 1);
            UInt *pCRD = &pictureTimingSEI.m_duCpbRemovalDelayMinus1[0];
            UInt maxDiff = (hrd->getTickDivisorMinus2() + 2) - 1;

            for (i = 0; i < numDU; i++)
            {
                pictureTimingSEI.m_numNalusInDuMinus1[i] = (i == 0) ? (accumNalsDU[i] - 1) : (accumNalsDU[i] - accumNalsDU[i - 1] - 1);
            }

            if (numDU == 1)
            {
                pCRD[0] = 0; /* don't care */
            }
            else
            {
                pCRD[numDU - 1] = 0; /* by definition */
                UInt tmp = 0;
                UInt accum = 0;
                int bitrate = m_cfg->param.rc.bitrate;

                for (i = (numDU - 2); i >= 0; i--)
                {
                    tmp64 = (((accumBitsDU[numDU - 1] - accumBitsDU[i]) * (vui->getTimingInfo()->getTimeScale() / vui->getTimingInfo()->getNumUnitsInTick()) * (hrd->getTickDivisorMinus2() + 2)) / bitrate);
                    if ((UInt)tmp64 > maxDiff)
                    {
                        tmp++;
                    }
                }

                prev = 0;

                UInt flag = 0;
                for (i = (numDU - 2); i >= 0; i--)
                {
                    flag = 0;
                    tmp64 = (((accumBitsDU[numDU - 1] - accumBitsDU[i]) * (vui->getTimingInfo()->getTimeScale() / vui->getTimingInfo()->getNumUnitsInTick()) * (hrd->getTickDivisorMinus2() + 2)) / bitrate);

                    if ((UInt)tmp64 > maxDiff)
                    {
                        if (prev >= maxDiff - tmp)
                        {
                            tmp64 = prev + 1;
                            flag = 1;
                        }
                        else tmp64 = maxDiff - tmp + 1;
                    }
                    pCRD[i] = (UInt)tmp64 - prev - 1;
                    if ((Int)pCRD[i] < 0)
                    {
                        pCRD[i] = 0;
                    }
                    else if (tmp > 0 && flag == 1)
                    {
                        tmp--;
                    }
                    accum += pCRD[i] + 1;
                    prev = accum;
                }
            }
        }
        if (m_cfg->getPictureTimingSEIEnabled())
        {
            OutputNALUnit onalu(NAL_UNIT_PREFIX_SEI, 0);
            m_seiWriter.writeSEImessage(onalu.m_Bitstream, pictureTimingSEI, slice->getSPS());
            writeRBSPTrailingBits(onalu.m_Bitstream);
            UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
            // Insert PT SEI after APS and BP SEI
            UInt offsetPosition = bBufferingPeriodSEIPresentInAU;
            AccessUnit::iterator it = accessUnit.begin();
            for (int j = 0; j < seiPositionInAu + offsetPosition; j++)
            {
                it++;
            }

            accessUnit.insert(it, new NALUnitEBSP(onalu));
            bPictureTimingSEIPresentInAU = true;
        }
        if (m_cfg->getDecodingUnitInfoSEIEnabled() && hrd->getSubPicCpbParamsPresentFlag())
        {
            for (Int i = 0; i < (pictureTimingSEI.m_numDecodingUnitsMinus1 + 1); i++)
            {
                OutputNALUnit onalu(NAL_UNIT_PREFIX_SEI, 0);

                SEIDecodingUnitInfo tempSEI;
                tempSEI.m_decodingUnitIdx = i;
                tempSEI.m_duSptCpbRemovalDelay = pictureTimingSEI.m_duCpbRemovalDelayMinus1[i] + 1;
                tempSEI.m_dpbOutputDuDelayPresentFlag = false;
                tempSEI.m_picSptDpbOutputDuDelay = picSptDpbOutputDuDelay;

                AccessUnit::iterator it = accessUnit.begin();
                // Insert the first one in the right location, before the first slice
                if (i == 0)
                {
                    // Insert before the first slice.
                    m_seiWriter.writeSEImessage(onalu.m_Bitstream, tempSEI, slice->getSPS());
                    writeRBSPTrailingBits(onalu.m_Bitstream);
                    UInt seiPositionInAu = xGetFirstSeiLocation(accessUnit);
                    // Insert DU info SEI after APS, BP and PT SEI
                    UInt offsetPosition = bBufferingPeriodSEIPresentInAU + bPictureTimingSEIPresentInAU;
                    for (int j = 0; j < seiPositionInAu + offsetPosition; j++)
                    {
                        it++;
                    }

                    accessUnit.insert(it, new NALUnitEBSP(onalu));
                }
                else
                {
                    it = accessUnit.begin();
                    // For the second decoding unit onwards we know how many NALUs are present
                    for (Int ctr = 0; it != accessUnit.end(); it++)
                    {
                        if (ctr == accumNalsDU[i - 1])
                        {
                            // Insert before the first slice.
                            m_seiWriter.writeSEImessage(onalu.m_Bitstream, tempSEI, slice->getSPS());
                            writeRBSPTrailingBits(onalu.m_Bitstream);

                            accessUnit.insert(it, new NALUnitEBSP(onalu));
                            break;
                        }
                        if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
                        {
                            ctr++;
                        }
                    }
                }
            }
        }
    }

    m_totalCoded++;

    delete[] outStreams;
    delete bitstreamRedirect;

    if (accumBitsDU != NULL) delete [] accumBitsDU;
    if (accumNalsDU != NULL) delete [] accumNalsDU;
}

SEIActiveParameterSets* TEncGOP::xCreateSEIActiveParameterSets()
{
    SEIActiveParameterSets *seiActiveParameterSets = new SEIActiveParameterSets();

    seiActiveParameterSets->activeVPSId = m_cfg->getVPS()->getVPSId();
    seiActiveParameterSets->m_fullRandomAccessFlag = false;
    seiActiveParameterSets->m_noParamSetUpdateFlag = false;
    seiActiveParameterSets->numSpsIdsMinus1 = 0;
    seiActiveParameterSets->activeSeqParamSetId.resize(seiActiveParameterSets->numSpsIdsMinus1 + 1);
    seiActiveParameterSets->activeSeqParamSetId[0] = m_sps.getSPSId();
    return seiActiveParameterSets;
}

SEIDisplayOrientation* TEncGOP::xCreateSEIDisplayOrientation()
{
    SEIDisplayOrientation *seiDisplayOrientation = new SEIDisplayOrientation();

    seiDisplayOrientation->cancelFlag = false;
    seiDisplayOrientation->horFlip = false;
    seiDisplayOrientation->verFlip = false;
    seiDisplayOrientation->anticlockwiseRotation = m_cfg->getDisplayOrientationSEIAngle();
    return seiDisplayOrientation;
}

#define VERBOSE_RATE 0
#if VERBOSE_RATE
static const Char* nalUnitTypeToString(NalUnitType type)
{
    switch (type)
    {
    case NAL_UNIT_CODED_SLICE_TRAIL_R:    return "TRAIL_R";
    case NAL_UNIT_CODED_SLICE_TRAIL_N:    return "TRAIL_N";
    case NAL_UNIT_CODED_SLICE_TLA_R:      return "TLA_R";
    case NAL_UNIT_CODED_SLICE_TSA_N:      return "TSA_N";
    case NAL_UNIT_CODED_SLICE_STSA_R:     return "STSA_R";
    case NAL_UNIT_CODED_SLICE_STSA_N:     return "STSA_N";
    case NAL_UNIT_CODED_SLICE_BLA_W_LP:   return "BLA_W_LP";
    case NAL_UNIT_CODED_SLICE_BLA_W_RADL: return "BLA_W_RADL";
    case NAL_UNIT_CODED_SLICE_BLA_N_LP:   return "BLA_N_LP";
    case NAL_UNIT_CODED_SLICE_IDR_W_RADL: return "IDR_W_RADL";
    case NAL_UNIT_CODED_SLICE_IDR_N_LP:   return "IDR_N_LP";
    case NAL_UNIT_CODED_SLICE_CRA:        return "CRA";
    case NAL_UNIT_CODED_SLICE_RADL_R:     return "RADL_R";
    case NAL_UNIT_CODED_SLICE_RASL_R:     return "RASL_R";
    case NAL_UNIT_VPS:                    return "VPS";
    case NAL_UNIT_SPS:                    return "SPS";
    case NAL_UNIT_PPS:                    return "PPS";
    case NAL_UNIT_ACCESS_UNIT_DELIMITER:  return "AUD";
    case NAL_UNIT_EOS:                    return "EOS";
    case NAL_UNIT_EOB:                    return "EOB";
    case NAL_UNIT_FILLER_DATA:            return "FILLER";
    case NAL_UNIT_PREFIX_SEI:             return "SEI";
    case NAL_UNIT_SUFFIX_SEI:             return "SEI";
    default:                              return "UNK";
    }
}

#endif // if VERBOSE_RATE

// TODO:
//   1 - as a performance optimization, if we're not reporting PSNR we do not have to measure PSNR
//       (we do not yet have a switch to disable PSNR reporting)
//   2 - it would be better to accumulate SSD of each CTU at the end of processCTU() while it is cache-hot
//       in fact, we almost certainly are already measuring the CTU distortion and not accumulating it
static UInt64 computeSSD(Pel *fenc, Pel *rec, Int stride, Int width, Int height)
{
    UInt64 ssd = 0;

    if ((width | height) & 3)
    {
        /* Slow Path */
        for (Int y = 0; y < height; y++)
        {
            for (Int x = 0; x < width; x++)
            {
                Int diff = (Int)(fenc[x] - rec[x]);
                ssd += diff * diff;
            }

            fenc += stride;
            rec += stride;
        }

        return ssd;
    }

    Int y = 0;
    /* Consume Y in chunks of 64 */
    for (; y + 64 <= height; y += 64)
    {
        Int x = 0;

        if (!(stride & 31))
            for (; x + 64 <= width; x += 64)
                ssd += x265::primitives.sse_pp[x265::PARTITION_64x64](fenc + x, stride, rec + x, stride);

        if (!(stride & 15))
            for (; x + 16 <= width; x += 16)
                ssd += x265::primitives.sse_pp[x265::PARTITION_16x64](fenc + x, stride, rec + x, stride);

        for (; x + 4 <= width; x += 4)
            ssd += x265::primitives.sse_pp[x265::PARTITION_4x64](fenc + x, stride, rec + x, stride);

        fenc += stride * 64;
        rec += stride * 64;
    }

    /* Consume Y in chunks of 16 */
    for (; y + 16 <= height; y += 16)
    {
        Int x = 0;

        if (!(stride & 31))
            for (; x + 64 <= width; x += 64)
                ssd += x265::primitives.sse_pp[x265::PARTITION_64x16](fenc + x, stride, rec + x, stride);

        if (!(stride & 15))
            for (; x + 16 <= width; x += 16)
                ssd += x265::primitives.sse_pp[x265::PARTITION_16x16](fenc + x, stride, rec + x, stride);

        for (; x + 4 <= width; x += 4)
            ssd += x265::primitives.sse_pp[x265::PARTITION_4x16](fenc + x, stride, rec + x, stride);

        fenc += stride * 16;
        rec += stride * 16;
    }

    /* Consume Y in chunks of 4 */
    for (; y + 4 <= height; y += 4)
    {
        Int x = 0;

        if (!(stride & 31))
            for (; x + 64 <= width; x += 64)
                ssd += x265::primitives.sse_pp[x265::PARTITION_64x4](fenc + x, stride, rec + x, stride);

        if (!(stride & 15))
            for (; x + 16 <= width; x += 16)
                ssd += x265::primitives.sse_pp[x265::PARTITION_16x4](fenc + x, stride, rec + x, stride);

        for (; x + 4 <= width; x += 4)
            ssd += x265::primitives.sse_pp[x265::PARTITION_4x4](fenc + x, stride, rec + x, stride);

        fenc += stride * 4;
        rec += stride * 4;
    }

    return ssd;
}

Void TEncGOP::calculateHashAndPSNR(TComPic* pic, TComPicYuv* recon, AccessUnit& accessUnit)
{
    //===== calculate PSNR =====
    Int stride = recon->getStride();
    Int width  = recon->getWidth() - m_cfg->getPad(0);
    Int height = recon->getHeight() - m_cfg->getPad(1);
    Int size = width * height;

    UInt64 ssdY = computeSSD(pic->getPicYuvOrg()->getLumaAddr(), recon->getLumaAddr(), stride, width, height);

    height >>= 1;
    width  >>= 1;
    stride = recon->getCStride();

    UInt64 ssdU = computeSSD(pic->getPicYuvOrg()->getCbAddr(), recon->getCbAddr(), stride, width, height);
    UInt64 ssdV = computeSSD(pic->getPicYuvOrg()->getCrAddr(), recon->getCrAddr(), stride, width, height);

    Int maxvalY = 255 << (X265_DEPTH - 8);
    Int maxvalC = 255 << (X265_DEPTH - 8);
    Double refValueY = (Double)maxvalY * maxvalY * size;
    Double refValueC = (Double)maxvalC * maxvalC * size / 4.0;
    Double psnrY = (ssdY ? 10.0 * log10(refValueY / (Double)ssdY) : 99.99);
    Double psnrU = (ssdU ? 10.0 * log10(refValueC / (Double)ssdU) : 99.99);
    Double psnrV = (ssdV ? 10.0 * log10(refValueC / (Double)ssdV) : 99.99);

    const Char* digestStr = NULL;
    if (m_cfg->getDecodedPictureHashSEIEnabled())
    {
        SEIDecodedPictureHash sei_recon_picture_digest;
        if (m_cfg->getDecodedPictureHashSEIEnabled() == 1)
        {
            /* calculate MD5sum for entire reconstructed picture */
            sei_recon_picture_digest.method = SEIDecodedPictureHash::MD5;
            calcMD5(*pic->getPicYuvRec(), sei_recon_picture_digest.digest);
            digestStr = digestToString(sei_recon_picture_digest.digest, 16);
        }
        else if (m_cfg->getDecodedPictureHashSEIEnabled() == 2)
        {
            sei_recon_picture_digest.method = SEIDecodedPictureHash::CRC;
            calcCRC(*pic->getPicYuvRec(), sei_recon_picture_digest.digest);
            digestStr = digestToString(sei_recon_picture_digest.digest, 2);
        }
        else if (m_cfg->getDecodedPictureHashSEIEnabled() == 3)
        {
            sei_recon_picture_digest.method = SEIDecodedPictureHash::CHECKSUM;
            calcChecksum(*pic->getPicYuvRec(), sei_recon_picture_digest.digest);
            digestStr = digestToString(sei_recon_picture_digest.digest, 4);
        }
        OutputNALUnit onalu(NAL_UNIT_SUFFIX_SEI, 0);

        /* write the SEI messages */
        m_seiWriter.writeSEImessage(onalu.m_Bitstream, sei_recon_picture_digest, pic->getSlice()->getSPS());
        writeRBSPTrailingBits(onalu.m_Bitstream);

        accessUnit.insert(accessUnit.end(), new NALUnitEBSP(onalu));
    }

    /* calculate the size of the access unit, excluding:
     *  - any AnnexB contributions (start_code_prefix, zero_byte, etc.,)
     *  - SEI NAL units
     */
    UInt numRBSPBytes = 0;
    for (AccessUnit::const_iterator it = accessUnit.begin(); it != accessUnit.end(); it++)
    {
        UInt numRBSPBytes_nal = UInt((*it)->m_nalUnitData.str().size());
#if VERBOSE_RATE
        printf("*** %6s numBytesInNALunit: %u\n", nalUnitTypeToString((*it)->m_nalUnitType), numRBSPBytes_nal);
#endif
        if ((*it)->m_nalUnitType != NAL_UNIT_PREFIX_SEI && (*it)->m_nalUnitType != NAL_UNIT_SUFFIX_SEI)
        {
            numRBSPBytes += numRBSPBytes_nal;
        }
    }

    UInt bits = numRBSPBytes * 8;

    /* Acquire encoder global lock to accumulate statistics and print debug info to console */
    x265::ScopedLock(m_top->m_statLock);

    //===== add PSNR =====
    m_top->m_analyzeAll.addResult(psnrY, psnrU, psnrV, (Double)bits);
    TComSlice*  slice = pic->getSlice();
    if (slice->isIntra())
    {
        m_top->m_analyzeI.addResult(psnrY, psnrU, psnrV, (Double)bits);
    }
    if (slice->isInterP())
    {
        m_top->m_analyzeP.addResult(psnrY, psnrU, psnrV, (Double)bits);
    }
    if (slice->isInterB())
    {
        m_top->m_analyzeB.addResult(psnrY, psnrU, psnrV, (Double)bits);
    }

    if (m_cfg->param.logLevel >= X265_LOG_DEBUG)
    {
        Char c = (slice->isIntra() ? 'I' : slice->isInterP() ? 'P' : 'B');

        if (!slice->isReferenced())
            c += 32; // lower case if unreferenced

        fprintf(stderr, "\rPOC %4d ( %c-SLICE, nQP %d QP %d Depth %d) %10d bits",
               slice->getPOC(),
               c,
               slice->getSliceQpBase(),
               slice->getSliceQp(),
               slice->getDepth(),
               bits);

        fprintf(stderr, " [Y:%6.2lf U:%6.2lf V:%6.2lf]", psnrY, psnrU, psnrV);

        if (!slice->isIntra())
        {
            Int numLists = slice->isInterP() ? 1 : 2;
            for (Int list = 0; list < numLists; list++)
            {
                fprintf(stderr, " [L%d ", list);
                for (Int ref = 0; ref < slice->getNumRefIdx(RefPicList(list)); ref++)
                {
                    fprintf(stderr, "%d ", slice->getRefPOC(RefPicList(list), ref) - slice->getLastIDR());
                }

                fprintf(stderr, "]");
            }
        }
        if (digestStr)
        {
            if (m_cfg->getDecodedPictureHashSEIEnabled() == 1)
            {
                fprintf(stderr, " [MD5:%s]", digestStr);
            }
            else if (m_cfg->getDecodedPictureHashSEIEnabled() == 2)
            {
                fprintf(stderr, " [CRC:%s]", digestStr);
            }
            else if (m_cfg->getDecodedPictureHashSEIEnabled() == 3)
            {
                fprintf(stderr, " [Checksum:%s]", digestStr);
            }
        }
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

/** Attaches the input bitstream to the stream in the output NAL unit
    Updates rNalu to contain concatenated bitstream. rpcBitstreamRedirect is cleared at the end of this function call.
 *  \param codedSliceData contains the coded slice data (bitstream) to be concatenated to rNalu
 *  \param rNalu          target NAL unit
 */
Void TEncGOP::xAttachSliceDataToNalUnit(TEncEntropy* entropyCoder, OutputNALUnit& nalu, TComOutputBitstream* codedSliceData)
{
    // Byte-align
    nalu.m_Bitstream.writeByteAlignment(); // Slice header byte-alignment

    // Perform bitstream concatenation
    if (codedSliceData->getNumberOfWrittenBits() > 0)
    {
        nalu.m_Bitstream.addSubstream(codedSliceData);
    }

    entropyCoder->setBitstream(&nalu.m_Bitstream);

    codedSliceData->clear();
}

/** Function for finding the position to insert the first of APS and non-nested BP, PT, DU info SEI messages.
 * \param accessUnit Access Unit of the current picture
 * This function finds the position to insert the first of APS and non-nested BP, PT, DU info SEI messages.
 */
Int TEncGOP::xGetFirstSeiLocation(AccessUnit &accessUnit)
{
    // Find the location of the first SEI message
    AccessUnit::iterator it;
    Int seiStartPos = 0;

    for (it = accessUnit.begin(); it != accessUnit.end(); it++, seiStartPos++)
    {
        if ((*it)->isSei() || (*it)->isVcl())
        {
            break;
        }
    }

    return seiStartPos;
}

/**
 * Produce an ascii(hex) representation of picture digest.
 *
 * Returns: a statically allocated null-terminated string.  DO NOT FREE.
 */
const char*digestToString(const unsigned char digest[3][16], int numChar)
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
//! \}
