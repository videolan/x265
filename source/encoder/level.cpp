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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "slice.h"
#include "level.h"

namespace x265 {
typedef struct
{
    uint32_t maxLumaSamples;
    uint32_t maxLumaSamplesPerSecond;
    uint32_t maxBitrateMain;
    uint32_t maxBitrateHigh;
    uint32_t maxCpbSizeMain;
    uint32_t maxCpbSizeHigh;
    uint32_t minCompressionRatio;
    Level::Name levelEnum;
    const char* name;
    int levelIdc;
} LevelSpec;

LevelSpec levels[] =
{
    { 36864,    552960,     128,      MAX_UINT, 350,    MAX_UINT, 2, Level::LEVEL1,   "1",   10 },
    { 122880,   3686400,    1500,     MAX_UINT, 1500,   MAX_UINT, 2, Level::LEVEL2,   "2",   20 },
    { 245760,   7372800,    3000,     MAX_UINT, 3000,   MAX_UINT, 2, Level::LEVEL2_1, "2.1", 21 },
    { 552960,   16588800,   6000,     MAX_UINT, 6000,   MAX_UINT, 2, Level::LEVEL3,   "3",   30 },
    { 983040,   33177600,   10000,    MAX_UINT, 10000,  MAX_UINT, 2, Level::LEVEL3_1, "3.1", 31 },
    { 2228224,  66846720,   12000,    30000,    12000,  30000,    4, Level::LEVEL4,   "4",   40 },
    { 2228224,  133693440,  20000,    50000,    20000,  50000,    4, Level::LEVEL4_1, "4.1", 41 },
    { 8912896,  267386880,  25000,    100000,   25000,  100000,   6, Level::LEVEL5,   "5",   50 },
    { 8912896,  534773760,  40000,    160000,   40000,  160000,   8, Level::LEVEL5_1, "5.1", 51 },
    { 8912896,  1069547520, 60000,    240000,   60000,  240000,   8, Level::LEVEL5_2, "5.2", 52 },
    { 35651584, 1069547520, 60000,    240000,   60000,  240000,   8, Level::LEVEL6,   "6",   60 },
    { 35651584, 2139095040, 120000,   480000,   120000, 480000,   8, Level::LEVEL6_1, "6.1", 61 },
    { 35651584, 4278190080U, 240000,  800000,   240000, 800000,   6, Level::LEVEL6_2, "6.2", 62 },
};

/* determine minimum decoder level required to decode the described video */
void determineLevel(const x265_param &param, VPS& vps)
{
    vps.ptl.profileIdc = Profile::NONE;
    if (param.internalCsp == X265_CSP_I420)
    {
        /* other misc requirements that we enforce in other areas:
         * 1. chroma bitdpeth is same as luma bit depth
         * 2. CTU size is 16, 32, or 64
         * 3. some limitations on tiles, we do not support tiles
         *
         * Technically, Mainstillpicture implies one picture per bitstream but
         * we do not enforce this limit. We do repeat SPS, PPS, and VPS each
         * frame */
        if (param.internalBitDepth == 8 && param.keyframeMax == 1)
            vps.ptl.profileIdc = Profile::MAINSTILLPICTURE;
        if (param.internalBitDepth == 8)
            vps.ptl.profileIdc = Profile::MAIN;
        else if (param.internalBitDepth == 10)
            vps.ptl.profileIdc = Profile::MAIN10;
    }
    /* TODO: Range extension profiles */

    uint32_t lumaSamples = param.sourceWidth * param.sourceHeight;
    uint32_t samplesPerSec = (uint32_t)(lumaSamples * ((double)param.fpsNum / param.fpsDenom));
    uint32_t bitrate = param.rc.vbvMaxBitrate ? param.rc.vbvMaxBitrate : param.rc.bitrate;

    const uint32_t MaxDpbPicBuf = 6;
    vps.ptl.levelIdc = Level::NONE;
    vps.ptl.tierFlag = Level::MAIN;

    const size_t NumLevels = sizeof(levels) / sizeof(levels[0]);
    uint32_t i;
    for (i = 0; i < NumLevels; i++)
    {
        if (lumaSamples > levels[i].maxLumaSamples)
            continue;
        else if (samplesPerSec > levels[i].maxLumaSamplesPerSecond)
            continue;
        else if (bitrate > levels[i].maxBitrateMain && levels[i].maxBitrateHigh == MAX_UINT)
            continue;
        else if (bitrate > levels[i].maxBitrateHigh)
            continue;
        else if (param.sourceWidth > sqrt(levels[i].maxLumaSamples * 8.0f))
            continue;
        else if (param.sourceHeight > sqrt(levels[i].maxLumaSamples * 8.0f))
            continue;

        uint32_t maxDpbSize = MaxDpbPicBuf;
        if (lumaSamples <= (levels[i].maxLumaSamples >> 2))
            maxDpbSize = X265_MIN(4 * MaxDpbPicBuf, 16);
        else if (lumaSamples <= (levels[i].maxLumaSamples >> 1))
            maxDpbSize = X265_MIN(2 * MaxDpbPicBuf, 16);
        else if (lumaSamples <= ((3 * levels[i].maxLumaSamples) >> 2))
            maxDpbSize = X265_MIN((4 * MaxDpbPicBuf) / 3, 16);

        /* The value of sps_max_dec_pic_buffering_minus1[ HighestTid ] + 1 shall be less than
         * or equal to MaxDpbSize */
        if (vps.maxDecPicBuffering > maxDpbSize)
            continue;

        /* For level 5 and higher levels, the value of CtbSizeY shall be equal to 32 or 64 */
        if (levels[i].levelEnum >= Level::LEVEL5 && param.maxCUSize < 32)
            x265_log(&param, X265_LOG_WARNING, "CTU size is too small, stream will be non-compliant for level %s\n", levels[i].name);

        /* The value of NumPocTotalCurr shall be less than or equal to 8 */
        int numPocTotalCurr = param.maxNumReferences + !!param.bframes;
        if (numPocTotalCurr > 8)
            x265_log(&param, X265_LOG_WARNING, "Too many reference frames, stream will be non-compliant for level %s\n", levels[i].name);

        vps.ptl.levelIdc = levels[i].levelEnum;
        if (bitrate > levels[i].maxBitrateMain && bitrate <= levels[i].maxBitrateHigh &&
            levels[i].maxBitrateHigh != MAX_UINT)
            vps.ptl.tierFlag = Level::HIGH;
        else
            vps.ptl.tierFlag = Level::MAIN;
        break;
    }

    static const char *profiles[] = { "None", "Main", "Main10", "Mainstillpicture" };
    static const char *tiers[]    = { "Main", "High" };
    x265_log(&param, X265_LOG_INFO, "%s profile, Level-%s (%s tier)\n",
             profiles[vps.ptl.profileIdc], levels[i].name, tiers[vps.ptl.tierFlag]);
}

/* enforce a maximum decoder level requirement, in other words assure that a
 * decoder of the specified level may decode the video about to be created.
 * Lower parameters where necessary to ensure the video will be decodable by a
 * decoder meeting this level of requirement.  Some parameters (resolution and
 * frame rate) are non-negotiable and thus this function may fail. In those
 * circumstances it will be quite noisy */
bool enforceLevel(x265_param& param, VPS& vps)
{
    vps.numReorderPics = (param.bBPyramid && param.bframes > 1) ? 2 : 1;
    vps.maxDecPicBuffering = X265_MIN(MAX_NUM_REF, X265_MAX(vps.numReorderPics + 1, (uint32_t)param.maxNumReferences) + vps.numReorderPics);

    /* no level specified by user, just auto-detect from the configuration */
    if (param.levelIdc <= 0)
        return true;

    uint32_t level = 0;
    while (levels[level].levelIdc != param.levelIdc && level + 1 < sizeof(levels) / sizeof(levels[0]))
        level++;
    if (levels[level].levelIdc != param.levelIdc)
    {
        x265_log(&param, X265_LOG_WARNING, "specified level %d does not exist\n", param.levelIdc);
        return false;
    }

    LevelSpec& l = levels[level];
    bool highTier = !!param.bHighTier;
    if (highTier && l.maxBitrateHigh == MAX_UINT)
    {
        highTier = false;
        x265_log(&param, X265_LOG_WARNING, "Level %s has no High tier, using Main tier\n", l.name);
    }

    uint32_t lumaSamples = param.sourceWidth * param.sourceHeight;
    uint32_t samplesPerSec = (uint32_t)(lumaSamples * ((double)param.fpsNum / param.fpsDenom));
    bool ok = true;
    if (lumaSamples > l.maxLumaSamples)
        ok = false;
    else if (param.sourceWidth > sqrt(l.maxLumaSamples * 8.0f))
        ok = false;
    else if (param.sourceHeight > sqrt(l.maxLumaSamples * 8.0f))
        ok = false;
    if (!ok)
    {
        x265_log(&param, X265_LOG_WARNING, "picture dimensions are out of range for specified level\n");
        return false;
    }
    else if (samplesPerSec > l.maxLumaSamplesPerSecond)
    {
        x265_log(&param, X265_LOG_WARNING, "frame rate is out of range for specified level\n");
        return false;
    }

    if ((uint32_t)param.rc.vbvMaxBitrate > (highTier ? l.maxBitrateHigh : l.maxBitrateMain))
    {
        param.rc.vbvMaxBitrate = highTier ? l.maxBitrateHigh : l.maxBitrateMain;
        x265_log(&param, X265_LOG_INFO, "lowering VBV max bitrate to %dKbps\n", param.rc.vbvMaxBitrate);
    }
    if ((uint32_t)param.rc.vbvBufferSize > (highTier ? l.maxCpbSizeHigh : l.maxCpbSizeMain))
    {
        param.rc.vbvMaxBitrate = highTier ? l.maxCpbSizeHigh : l.maxCpbSizeMain;
        x265_log(&param, X265_LOG_INFO, "lowering VBV buffer size to %dKb\n", param.rc.vbvBufferSize);
    }

    switch (param.rc.rateControlMode)
    {
    case X265_RC_ABR:
        if ((uint32_t)param.rc.bitrate > (highTier ? l.maxBitrateHigh : l.maxBitrateMain))
        {
            param.rc.bitrate = l.maxBitrateHigh;
            x265_log(&param, X265_LOG_INFO, "lowering target bitrate to High tier limit of %dKbps\n", param.rc.bitrate);
        }
        break;

    case X265_RC_CQP:
        x265_log(&param, X265_LOG_WARNING, "Constant QP is inconsistent with specifying a decoder level, no bitrate guarantee is possible.\n");
        return false;

    case X265_RC_CRF:
        if (!param.rc.vbvBufferSize || !param.rc.vbvMaxBitrate)
        {
            if (!param.rc.vbvMaxBitrate)
                param.rc.vbvMaxBitrate = highTier ? l.maxBitrateHigh : l.maxBitrateMain;
            if (!param.rc.vbvBufferSize)
                param.rc.vbvBufferSize = highTier ? l.maxCpbSizeHigh : l.maxCpbSizeMain;
            x265_log(&param, X265_LOG_WARNING, "Specifying a decoder level with constant rate factor rate-control requires\n");
            x265_log(&param, X265_LOG_WARNING, "enabling VBV with vbv-bufsize=%dkb vbv-maxrate=%dkbps. VBV outputs are non-deterministic!\n",
                     param.rc.vbvBufferSize, param.rc.vbvMaxBitrate);
        }
        break;

    default:
        x265_log(&param, X265_LOG_ERROR, "Unknown rate control mode is inconsistent with specifying a decoder level\n");
        return false;
    }

    /* The value of sps_max_dec_pic_buffering_minus1[ HighestTid ] + 1 shall be less than or equal to MaxDpbSize */
    const uint32_t MaxDpbPicBuf = 6;
    uint32_t maxDpbSize = MaxDpbPicBuf;
    if (lumaSamples <= (l.maxLumaSamples >> 2))
        maxDpbSize = X265_MIN(4 * MaxDpbPicBuf, 16);
    else if (lumaSamples <= (l.maxLumaSamples >> 1))
        maxDpbSize = X265_MIN(2 * MaxDpbPicBuf, 16);
    else if (lumaSamples <= ((3 * l.maxLumaSamples) >> 2))
        maxDpbSize = X265_MIN((4 * MaxDpbPicBuf) / 3, 16);

    int savedRefCount = param.maxNumReferences;
    while (vps.maxDecPicBuffering > maxDpbSize && param.maxNumReferences > 1)
    {
        param.maxNumReferences--;
        vps.maxDecPicBuffering = X265_MIN(MAX_NUM_REF, X265_MAX(vps.numReorderPics + 1, (uint32_t)param.maxNumReferences) + vps.numReorderPics);
    }
    if (param.maxNumReferences != savedRefCount)
        x265_log(&param, X265_LOG_INFO, "Lowering max references to %d to meet level requirement\n", param.maxNumReferences);

    /* For level 5 and higher levels, the value of CtbSizeY shall be equal to 32 or 64 */
    if (param.levelIdc >= 50 && param.maxCUSize < 32)
    {
        param.maxCUSize = 32;
        x265_log(&param, X265_LOG_INFO, "Levels 5.0 and above require a maximum CTU size of at least 32, using --ctu 32\n");
    }

    /* The value of NumPocTotalCurr shall be less than or equal to 8 */
    int numPocTotalCurr = param.maxNumReferences + !!param.bframes;
    if (numPocTotalCurr > 8)
    {
        param.maxNumReferences = 8 - !!param.bframes;
        x265_log(&param, X265_LOG_INFO, "Lowering max references to %d to meet numPocTotalCurr requirement\n", param.maxNumReferences);
    }

    return true;
}
}
