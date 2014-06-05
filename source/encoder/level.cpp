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
#include "level.h"

namespace x265 {
typedef struct
{
    uint32_t maxLumaSamples;
    uint32_t maxLumaSamplesPerSecond;
    uint32_t maxBitrateMain;
    uint32_t maxBitrateHigh;
    uint32_t minCompressionRatio;
    Level::Name levelEnum;
    const char* name;
    int levelIdc;
} LevelSpec;

LevelSpec levels[] =
{
    { MAX_UINT, MAX_UINT,   MAX_UINT, MAX_UINT, 0, Level::NONE,     "none", 0 },
    { 36864,    552960,     128,      MAX_UINT, 2, Level::LEVEL1,   "1",   10 },
    { 122880,   3686400,    1500,     MAX_UINT, 2, Level::LEVEL2,   "2",   20 },
    { 245760,   7372800,    3000,     MAX_UINT, 2, Level::LEVEL2_1, "2.1", 21 },
    { 552960,   16588800,   6000,     MAX_UINT, 2, Level::LEVEL3,   "3",   30 },
    { 983040,   33177600,   10000,    MAX_UINT, 2, Level::LEVEL3_1, "3.1", 31 },
    { 2228224,  66846720,   12000,    30000,    4, Level::LEVEL4,   "4",   40 },
    { 2228224,  133693440,  20000,    50000,    4, Level::LEVEL4_1, "4.1", 41 },
    { 8912896,  267386880,  25000,    100000,   6, Level::LEVEL5,   "5",   50 },
    { 8912896,  534773760,  40000,    160000,   8, Level::LEVEL5_1, "5.1", 51 },
    { 8912896,  1069547520, 60000,    240000,   8, Level::LEVEL5_2, "5.2", 52 },
    { 35651584, 1069547520, 60000,    240000,   8, Level::LEVEL6,   "6",   60 },
    { 35651584, 2139095040, 120000,   480000,   8, Level::LEVEL6_1, "6.1", 61 },
    { 35651584, 4278190080U, 240000,  800000,   6, Level::LEVEL6_2, "6.2", 62 },
    { 0, 0, 0, 0, 0, Level::NONE, "\0", 0 }
};

/* determine minimum decoder level requiremented to decode the described video */
void determineLevel(const x265_param &param, Profile::Name& profile, Level::Name& level, Level::Tier& tier)
{
    profile = Profile::NONE;
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
            profile = Profile::MAINSTILLPICTURE;
        if (param.internalBitDepth == 8)
            profile = Profile::MAIN;
        else if (param.internalBitDepth == 10)
            profile = Profile::MAIN10;
    }
    /* TODO: Range extension profiles */

    uint32_t lumaSamples = param.sourceWidth * param.sourceHeight;
    uint32_t samplesPerSec = (uint32_t)(lumaSamples * ((double)param.fpsNum / param.fpsDenom));
    uint32_t bitrate = param.rc.bitrate ? param.rc.bitrate : param.rc.vbvMaxBitrate;

    /* TODO; Keep in sync with encoder.cpp, or pass in maxDecPicBuffering */
    int numReorderPics = (param.bBPyramid && param.bframes > 1) ? 2 : 1;
    int maxDecPicBuffering = X265_MIN(MAX_NUM_REF, X265_MAX(numReorderPics + 1, param.maxNumReferences) + numReorderPics);
    const int MaxDpbPicBuf = 6;

    level = Level::NONE;
    tier = Level::MAIN;
    const char *levelName = "(none)";

    for (int i = 1; levels[i].maxLumaSamples; i++)
    {
        if (lumaSamples > levels[i].maxLumaSamples)
            continue;
        else if (samplesPerSec > levels[i].maxLumaSamplesPerSecond)
            continue;
        else if (bitrate > levels[i].maxBitrateHigh)
            continue;
        else if (param.sourceWidth > sqrt(levels[i].maxLumaSamples * 8.0f))
            continue;
        else if (param.sourceHeight > sqrt(levels[i].maxLumaSamples * 8.0f))
            continue;

        int maxDpbSize = MaxDpbPicBuf;
        if (lumaSamples <= (levels[i].maxLumaSamples >> 2))
            maxDpbSize = X265_MIN(4 * MaxDpbPicBuf, 16);
        else if (lumaSamples <= (levels[i].maxLumaSamples >> 1))
            maxDpbSize = X265_MIN(2 * MaxDpbPicBuf, 16);
        else if (lumaSamples <= ((3 * levels[i].maxLumaSamples) >> 2))
            maxDpbSize = X265_MIN((4 * MaxDpbPicBuf) / 3, 16);

        /* The value of sps_max_dec_pic_buffering_minus1[ HighestTid ] + 1 shall be less than
         * or equal to MaxDpbSize */
        if (maxDecPicBuffering > maxDpbSize)
            continue;

        /* For level 5 and higher levels, the value of CtbSizeY shall be equal to 32 or 64 */
        if (levels[i].levelEnum >= Level::LEVEL5 && param.maxCUSize < 32)
            continue;

        level = levels[i].levelEnum;
        levelName = levels[i].name;
        if (bitrate > levels[i].maxBitrateMain && bitrate <= levels[i].maxBitrateHigh &&
            levels[i].maxBitrateHigh != MAX_UINT)
            tier = Level::HIGH;
        else
            tier = Level::MAIN;
        /* TODO: The value of NumPocTotalCurr shall be less than or equal to 8 */
        break;
    }

    static const char *profiles[] = { "None", "Main", "Main10", "Mainstillpicture" };
    static const char *tiers[]    = { "Main", "High" };
    x265_log(&param, X265_LOG_INFO, "%s profile, Level-%s (%s tier)\n", profiles[profile], levelName, tiers[tier]);
}

/* enforce a maximum decoder level requirement, in other words assure that a
 * decoder of the specified level may decode the video about to be created.
 * Lower parameters where necessary to ensure the video will be decodable by a
 * decoder meeting this level of requirement.  Some parameters (resolution and
 * frame rate) are non-negotiable and thus this function may fail. In those
 * circumstances it will be quite noisy */
void enforceLevel(x265_param& param)
{
    if (param.levelIdc < 0)
        return;

    uint32_t lumaSamples = param.sourceWidth * param.sourceHeight;
    uint32_t samplesPerSec = (uint32_t)(lumaSamples * ((double)param.fpsNum / param.fpsDenom));
    int level = 1;
    while (levels[level].levelIdc < param.levelIdc && levels[level].levelIdc)
        level++;
    LevelSpec& l = levels[level];

    if (!l.levelIdc)
    {
        x265_log(&param, X265_LOG_WARNING, "specified level does not exist\n");
        return;
    }
    if (l.levelIdc != param.levelIdc)
        x265_log(&param, X265_LOG_WARNING, "Using nearest matching level %s\n", l.name);

    bool ok = true;

    if (lumaSamples > l.maxLumaSamples)
        ok = false;
    else if (param.sourceWidth > sqrt(l.maxLumaSamples * 8.0f))
        ok = false;
    else if (param.sourceHeight > sqrt(l.maxLumaSamples * 8.0f))
        ok = false;
    if (!ok)
        x265_log(&param, X265_LOG_WARNING, "picture dimensions are out of range for specified level\n");
    else if (samplesPerSec > l.maxLumaSamplesPerSecond)
        x265_log(&param, X265_LOG_WARNING, "frame rate is out of range for specified level\n");

    if (param.rc.bitrate > (int)l.maxBitrateHigh && l.maxBitrateHigh != MAX_UINT)
    {
        param.rc.bitrate = l.maxBitrateHigh;
        x265_log(&param, X265_LOG_INFO, "Lowering target bitrate to High tier limit of %dKbps\n", param.rc.bitrate);
    }

    const int MaxDpbPicBuf = 6;
    int maxDpbSize = MaxDpbPicBuf;
    if (lumaSamples <= (l.maxLumaSamples >> 2))
        maxDpbSize = X265_MIN(4 * MaxDpbPicBuf, 16);
    else if (lumaSamples <= (l.maxLumaSamples >> 1))
        maxDpbSize = X265_MIN(2 * MaxDpbPicBuf, 16);
    else if (lumaSamples <= ((3 * l.maxLumaSamples) >> 2))
        maxDpbSize = X265_MIN((4 * MaxDpbPicBuf) / 3, 16);
    int savedRefCount = param.maxNumReferences;

    for (;; )
    {
        int numReorderPics = (param.bBPyramid && param.bframes > 1) ? 2 : 1;
        int maxDecPicBuffering = X265_MIN(MAX_NUM_REF, X265_MAX(numReorderPics + 1, param.maxNumReferences) + numReorderPics);

        if (maxDecPicBuffering > maxDpbSize)
            param.maxNumReferences--;
        else
            break;
    }

    if (param.maxNumReferences != savedRefCount)
    {
        x265_log(&param, X265_LOG_INFO, "Lowering max references to %d to meet level requirement\n", param.maxNumReferences);
    }
}
}
