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

#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPicYuv.h"
#include "common.h"
#include "threadpool.h"

#include "TLibEncoder/TEncTop.h"
#include "bitcost.h"
#include "x265.h"

#include <stdio.h>
#include <string.h>

using namespace x265;

/* "Glue" interface class between TEncTop and C API */

struct x265_t : public TEncTop
{
    x265_t();
    virtual ~x265_t();

    x265_nal_t *m_nals;
    char *m_packetData;

    void configure(x265_param_t *param);
    void determineLevelAndProfile(x265_param_t *param);
};

x265_t::x265_t()
{
    m_nals = NULL;
    m_packetData = NULL;
}

x265_t::~x265_t()
{
    if (m_nals)
        X265_FREE(m_nals);

    if (m_packetData)
        X265_FREE(m_packetData);
}

void x265_t::determineLevelAndProfile(x265_param_t *_param)
{
    // this is all based on the table at on Wikipedia at
    // http://en.wikipedia.org/wiki/High_Efficiency_Video_Coding#Profiles

    // TODO: there are minimum CTU sizes for higher levels, needs to be enforced

    uint32_t lumaSamples = _param->sourceWidth * _param->sourceHeight;
    uint32_t samplesPerSec = lumaSamples * _param->frameRate;
    uint32_t bitrate = _param->rc.bitrate;

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
    else if (_param->keyframeMax == 1)
        m_profile = Profile::MAINSTILLPICTURE;
    else
        m_profile = Profile::MAIN;

    static const char *profiles[] = { "None", "Main", "Main10", "Mainstillpicture" };
    static const char *tiers[]    = { "Main", "High" };
    x265_log(_param, X265_LOG_INFO, "%s profile, Level-%s (%s tier)\n", profiles[m_profile], level, tiers[m_levelTier]);
}

void x265_t::configure(x265_param_t *_param)
{
    // Trim the thread pool if WPP is disabled
    if (_param->bEnableWavefront == 0)
        _param->poolNumThreads = 1;

    setThreadPool(ThreadPool::allocThreadPool(_param->poolNumThreads));
    int actual = ThreadPool::getThreadPool()->getThreadCount();
    if (actual > 1)
    {
        x265_log(_param, X265_LOG_INFO, "WPP streams / pool / frames  : %d / %d / %d\n",
                 (_param->sourceHeight + _param->maxCUSize - 1) / _param->maxCUSize, actual, _param->frameNumThreads);
    }
    else if (_param->frameNumThreads > 1)
    {
        x265_log(_param, X265_LOG_INFO, "Concurrently encoded frames  : %d\n", _param->frameNumThreads);
        _param->bEnableWavefront = 0;
    }
    else
    {
        x265_log(_param, X265_LOG_INFO, "Parallelism disabled, single thread mode\n");
        _param->bEnableWavefront = 0;
    }
        
    if (_param->keyframeMin == 0)
    {
        _param->keyframeMin = _param->keyframeMax;
    }
    // if a bitrate is specified, chose ABR.  Else default to CQP
    if (_param->rc.bitrate)
    {
        _param->rc.rateControlMode = X265_RC_ABR;
    }

    /* Set flags according to RDLevel specified - check_params has verified that RDLevel is within range */
    switch(_param->bRDLevel)
    {
    case X265_NO_RDO_NO_RDOQ:
        _param->bEnableRDO = _param->bEnableRDOQ = 0; 
        break;
    case X265_NO_RDO:
        _param->bEnableRDO = 0;
        _param->bEnableRDOQ = 1;
        break;
    case X265_FULL_RDO:
        _param->bEnableRDO = _param->bEnableRDOQ = 1;
        break;
    }
    //====== Coding Tools ========

    uint32_t tuQTMaxLog2Size = g_convertToBit[_param->maxCUSize] + 2 - 1;
    m_quadtreeTULog2MaxSize = tuQTMaxLog2Size;
    uint32_t tuQTMinLog2Size = 2; //log2(4)
    m_quadtreeTULog2MinSize = tuQTMinLog2Size;

    //====== Enforce these hard coded settings before initializeGOP() to
    //       avoid a valgrind warning
    m_loopFilterOffsetInPPS = 0;
    m_loopFilterBetaOffsetDiv2 = 0;
    m_loopFilterTcOffsetDiv2 = 0;
    m_loopFilterAcrossTilesEnabledFlag = 1;

    //====== HM Settings not exposed for configuration ======
    TComVPS vps;
    vps.setMaxTLayers(1);
    vps.setTemporalNestingFlag(true);
    vps.setMaxLayers(1);
    for (int i = 0; i < MAX_TLAYER; i++)
    {
        m_numReorderPics[i] = _param->bframes;
        m_maxDecPicBuffering[i] = _param->bframes + 2;
        vps.setNumReorderPics(m_numReorderPics[i], i);
        vps.setMaxDecPicBuffering(m_maxDecPicBuffering[i], i);
    }

    m_vps = vps;
    m_maxCuDQPDepth = 0;
    m_maxNumOffsetsPerPic = 2048;
    m_log2ParallelMergeLevelMinus2 = 0;
    m_conformanceWindow.setWindow(0, 0, 0, 0);
    int nullpad[2] = { 0, 0 };
    setPad(nullpad);

    m_progressiveSourceFlag = true;
    m_interlacedSourceFlag = false;
    m_nonPackedConstraintFlag = false;
    m_frameOnlyConstraintFlag = false;
    m_bUseASR = false; // adapt search range based on temporal distances
    m_recoveryPointSEIEnabled = 0;
    m_bufferingPeriodSEIEnabled = 0;
    m_pictureTimingSEIEnabled = 0;
    m_displayOrientationSEIAngle = 0;
    m_gradualDecodingRefreshInfoEnabled = 0;
    m_decodingUnitInfoSEIEnabled = 0;
    m_useScalingListId = 0;
    m_activeParameterSetsSEIEnabled = 0;
    m_vuiParametersPresentFlag = false;
    m_minSpatialSegmentationIdc = 0;
    m_aspectRatioIdc = 0;
    m_sarWidth = 0;
    m_sarHeight = 0;
    m_overscanInfoPresentFlag = false;
    m_overscanAppropriateFlag = false;
    m_videoSignalTypePresentFlag = false;
    m_videoFormat = 5;
    m_videoFullRangeFlag = false;
    m_colourDescriptionPresentFlag = false;
    m_colourPrimaries = 2;
    m_transferCharacteristics = 2;
    m_matrixCoefficients = 2;
    m_chromaLocInfoPresentFlag = false;
    m_chromaSampleLocTypeTopField = 0;
    m_chromaSampleLocTypeBottomField = 0;
    m_neutralChromaIndicationFlag = false;
    m_defaultDisplayWindow.setWindow(0, 0, 0, 0);
    m_frameFieldInfoPresentFlag = false;
    m_pocProportionalToTimingFlag = false;
    m_numTicksPocDiffOneMinus1 = 0;
    m_bitstreamRestrictionFlag = false;
    m_motionVectorsOverPicBoundariesFlag = false;
    m_maxBytesPerPicDenom = 2;
    m_maxBitsPerMinCuDenom = 1;
    m_log2MaxMvLengthHorizontal = 15;
    m_log2MaxMvLengthVertical = 15;
    m_usePCM = 0;
    m_pcmLog2MinSize = 3;
    m_pcmLog2MaxSize = 5;
    m_bPCMInputBitDepthFlag = true;
    m_bPCMFilterDisableFlag = false;

    m_useLossless = false;  // x264 configures this via --qp=0
    m_TransquantBypassEnableFlag = false;
    m_CUTransquantBypassFlagValue = false;
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
        // these may change params for auto-detect, etc
        encoder->determineLevelAndProfile(param);
        encoder->configure(param);

        // save a copy of final parameters in TEncCfg
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
    if (!pp_nal)
        return 0;

    AccessUnit au;
    if (encoder->getStreamHeaders(au) == 0)
    {
        UInt memsize = 0;
        int nalcount = 0;
        for (AccessUnit::const_iterator t = au.begin(); t != au.end(); t++)
        {
            const NALUnitEBSP& temp = **t;
            memsize += (UInt)temp.m_nalUnitData.str().size() + 4;
            nalcount++;
        }

        if (encoder->m_packetData)
            X265_FREE(encoder->m_packetData);

        if (encoder->m_nals)
            X265_FREE(encoder->m_nals);

        encoder->m_packetData = (char*)X265_MALLOC(char, memsize);
        encoder->m_nals = (x265_nal_t*)X265_MALLOC(x265_nal_t, nalcount);
        nalcount = 0;
        memsize = 0;

        /* Copy NAL output packets into x265_nal_t structures */
        for (AccessUnit::const_iterator it = au.begin(); it != au.end(); it++)
        {
            const NALUnitEBSP& nalu = **it;
            int size = 0; /* size of annexB unit in bytes */

            static const char start_code_prefix[] = { 0, 0, 0, 1 };
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
                ::memcpy(encoder->m_packetData + memsize, start_code_prefix, 4);
                size += 4;
            }
            else
            {
                ::memcpy(encoder->m_packetData + memsize, start_code_prefix + 1, 3);
                size += 3;
            }
            memsize += size;
            size_t nalSize = nalu.m_nalUnitData.str().size();
            ::memcpy(encoder->m_packetData + memsize, nalu.m_nalUnitData.str().c_str(), nalSize);
            size += (int)nalSize;
            memsize += (int)nalSize;

            encoder->m_nals[nalcount].i_type = nalu.m_nalUnitType;
            encoder->m_nals[nalcount].i_payload = size;
            nalcount++;
        }

        /* Setup payload pointers, now that we're done adding content to m_packetData */
        size_t offset = 0;
        for (size_t i = 0; i < (size_t)nalcount; i++)
        {
            encoder->m_nals[i].p_payload = (uint8_t*)encoder->m_packetData + offset;
            offset += encoder->m_nals[i].i_payload;
        }

        *pp_nal = &encoder->m_nals[0];
        if (pi_nal) *pi_nal = (int)nalcount;
        return 0;
    }
    else
        return -1;
}

extern "C"
int x265_encoder_encode(x265_t *encoder, x265_nal_t **pp_nal, int *pi_nal, x265_picture_t *pic_in, x265_picture_t *pic_out)
{
    AccessUnit au;

    int numEncoded = encoder->encode(!pic_in, pic_in, pic_out, au);

    if (pp_nal && numEncoded)
    {
        UInt memsize = 0;
        int nalcount = 0;
        for (AccessUnit::const_iterator t = au.begin(); t != au.end(); t++)
        {
            const NALUnitEBSP& temp = **t;
            memsize += (UInt)temp.m_nalUnitData.str().size() + 4;
            nalcount++;
        }

        if (encoder->m_nals)
            X265_FREE(encoder->m_nals);

        if (encoder->m_packetData)
           X265_FREE(encoder->m_packetData);

        encoder->m_packetData = (char*)X265_MALLOC(char, memsize);
        encoder->m_nals = (x265_nal_t*)X265_MALLOC(x265_nal_t, nalcount);
        nalcount = 0;
        memsize = 0;

        /* Copy NAL output packets into x265_nal_t structures */
        for (AccessUnit::const_iterator it = au.begin(); it != au.end(); it++)
        {
            const NALUnitEBSP& nalu = **it;
            int size = 0; /* size of annexB unit in bytes */

            static const char start_code_prefix[] = { 0, 0, 0, 1 };
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
                ::memcpy(encoder->m_packetData + memsize, start_code_prefix, 4);
                size += 4;
            }
            else
            {
                ::memcpy(encoder->m_packetData + memsize, start_code_prefix + 1, 3);
                size += 3;
            }
            memsize += size;
            size_t nalSize = nalu.m_nalUnitData.str().size();
            ::memcpy(encoder->m_packetData + memsize, nalu.m_nalUnitData.str().c_str(), nalSize);
            size += (int)nalSize;
            memsize += (int)nalSize;

            encoder->m_nals[nalcount].i_type = nalu.m_nalUnitType;
            encoder->m_nals[nalcount].i_payload = size;
            nalcount++;
        }

        /* Setup payload pointers, now that we're done adding content to m_packetData */
        size_t offset = 0;
        for (size_t i = 0; i < (size_t)nalcount; i++)
        {
            encoder->m_nals[i].p_payload = (uint8_t*)encoder->m_packetData + offset;
            offset += encoder->m_nals[i].i_payload;
        }

        *pp_nal = &encoder->m_nals[0];
        if (pi_nal) *pi_nal =(int) nalcount;
    }
    else if (pi_nal)
        *pi_nal = 0;
    return numEncoded;
}

EXTERN_CYCLE_COUNTER(ME);

extern "C"
void x265_encoder_close(x265_t *encoder, double *outPsnr)
{
    double globalPsnr = encoder->printSummary();

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
    BitCost::destroy();
}
