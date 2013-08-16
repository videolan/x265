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

enum SCALING_LIST_PARAMETER
{
    SCALING_LIST_OFF,
    SCALING_LIST_DEFAULT,
};

TEncGOP::TEncGOP()
{
    m_cfg          = NULL;
    m_top          = NULL;
    m_frameEncoder = NULL;
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
    entropyCoder->encodeVPS(m_cfg->getVPS());
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
        SEIActiveParameterSets sei;
        sei.activeVPSId = m_cfg->getVPS()->getVPSId();
        sei.m_fullRandomAccessFlag = false;
        sei.m_noParamSetUpdateFlag = false;
        sei.numSpsIdsMinus1 = 0;
        sei.activeSeqParamSetId.resize(sei.numSpsIdsMinus1 + 1);
        sei.activeSeqParamSetId[0] = m_sps.getSPSId();

        entropyCoder->setBitstream(&nalu.m_Bitstream);
        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei, &m_sps);
        writeRBSPTrailingBits(nalu.m_Bitstream);
        accessUnit.push_back(new NALUnitEBSP(nalu));
    }

    if (m_cfg->getDisplayOrientationSEIAngle())
    {
        SEIDisplayOrientation sei;
        sei.cancelFlag = false;
        sei.horFlip = false;
        sei.verFlip = false;
        sei.anticlockwiseRotation = m_cfg->getDisplayOrientationSEIAngle();

        nalu = NALUnit(NAL_UNIT_PREFIX_SEI);
        entropyCoder->setBitstream(&nalu.m_Bitstream);
        m_seiWriter.writeSEImessage(nalu.m_Bitstream, sei, &m_sps);
        writeRBSPTrailingBits(nalu.m_Bitstream);
        accessUnit.push_back(new NALUnitEBSP(nalu));
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

    UInt                  oneBitstreamPerSliceLength = 0; // TODO: Remove
    TComOutputBitstream*  bitstreamRedirect = new TComOutputBitstream;
    TComOutputBitstream*  outStreams = NULL;

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

    if ((m_cfg->getRecoveryPointSEIEnabled()) && (slice->getSliceType() == I_SLICE))
    {
        if (m_cfg->getGradualDecodingRefreshInfoEnabled() && !slice->getRapPicFlag())
        {
            // Gradual decoding refresh SEI
            OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);

            SEIGradualDecodingRefreshInfo seiGradualDecodingRefreshInfo;
            seiGradualDecodingRefreshInfo.m_gdrForegroundFlag = true; // Indicating all "foreground"

            m_seiWriter.writeSEImessage(nalu.m_Bitstream, seiGradualDecodingRefreshInfo, slice->getSPS());
            writeRBSPTrailingBits(nalu.m_Bitstream);
            accessUnit.push_back(new NALUnitEBSP(nalu));
        }
        // Recovery point SEI
        OutputNALUnit nalu(NAL_UNIT_PREFIX_SEI);

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
        PCMLFDisableProcess(pic);

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

    // If current NALU is the first NALU of slice (containing slice header) and
    // more NALUs exist (due to multiple dependent slices) then buffer it.  If
    // current NALU is the last NALU of slice and a NALU was buffered, then (a)
    // Write current NALU (b) Update an write buffered NALU at appropriate
    // location in NALU list.
    nalu.m_Bitstream.writeByteAlignment(); // Slice header byte-alignment

    // Perform bitstream concatenation
    if (bitstreamRedirect->getNumberOfWrittenBits() > 0)
    {
        nalu.m_Bitstream.addSubstream(bitstreamRedirect);
    }
    entropyCoder->setBitstream(&nalu.m_Bitstream);
    bitstreamRedirect->clear();

    accessUnit.push_back(new NALUnitEBSP(nalu));
    oneBitstreamPerSliceLength += nalu.m_Bitstream.getNumberOfWrittenBits(); // length of bitstream after byte-alignment

    if (m_sps.getUseSAO())
    {
        sao->destroyPicSaoInfo();
        pic->destroyNonDBFilterInfo();
    }
    pic->compressMotion();

    delete[] outStreams;
    delete bitstreamRedirect;
}
