/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
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

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComRom.h"
#include "TLibCommon/TComSlice.h"
#include "x265.h"
#include "threading.h"
#include "common.h"

#include <climits>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if _WIN32
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

int64_t x265_mdate(void)
{
#if _WIN32
    struct timeb tb;
    ftime(&tb);
    return ((int64_t)tb.time * 1000 + (int64_t)tb.millitm) * 1000;
#else
    struct timeval tv_date;
    gettimeofday(&tv_date, NULL);
    return (int64_t)tv_date.tv_sec * 1000000 + (int64_t)tv_date.tv_usec;
#endif
}

using namespace x265;

#define X265_ALIGNBYTES 32

#if _WIN32
#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#define _aligned_malloc __mingw_aligned_malloc
#define _aligned_free   __mingw_aligned_free
#include "malloc.h"
#endif

void *x265_malloc(size_t size)
{
    return _aligned_malloc(size, X265_ALIGNBYTES);
}

void x265_free(void *ptr)
{
    if (ptr) _aligned_free(ptr);
}

#else // if _WIN32
void *x265_malloc(size_t size)
{
    void *ptr;

    if (posix_memalign((void**)&ptr, X265_ALIGNBYTES, size) == 0)
        return ptr;
    else
        return NULL;
}

void x265_free(void *ptr)
{
    if (ptr) free(ptr);
}

#endif // if _WIN32

/* Not a general-purpose function; multiplies input by -1/6 to convert
 * qp to qscale. */
int x265_exp2fix8(double x)
{
    int i = (int)(x * (-64.f / 6.f) + 512.5f);

    if (i < 0) return 0;
    if (i > 1023) return 0xffff;
    return (x265_exp2_lut[i & 63] + 256) << (i >> 6) >> 8;
}

void x265_log(x265_param *param, int level, const char *fmt, ...)
{
    if (param && level > param->logLevel)
        return;
    const char *log_level;
    switch (level)
    {
    case X265_LOG_ERROR:
        log_level = "error";
        break;
    case X265_LOG_WARNING:
        log_level = "warning";
        break;
    case X265_LOG_INFO:
        log_level = "info";
        break;
    case X265_LOG_DEBUG:
        log_level = "debug";
        break;
    default:
        log_level = "unknown";
        break;
    }

    fprintf(stderr, "x265 [%s]: ", log_level);
    va_list arg;
    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);
}

extern "C"
void x265_param_default(x265_param *param)
{
    memset(param, 0, sizeof(x265_param));

    /* Applying non-zero default values to all elements in the param structure */
    param->logLevel = X265_LOG_INFO;
    param->bEnableWavefront = 1;
    param->frameNumThreads = 0;
    param->poolNumThreads = 0;
    param->csvfn = NULL;

    /* Source specifications */
    param->internalBitDepth = x265_max_bit_depth;
    param->internalCsp = X265_CSP_I420;

    /* CU definitions */
    param->maxCUSize = 64;
    param->tuQTMaxInterDepth = 1;
    param->tuQTMaxIntraDepth = 1;

    /* Coding Structure */
    param->keyframeMin = 0;
    param->keyframeMax = 250;
    param->bOpenGOP = 1;
    param->bframes = 4;
    param->lookaheadDepth = 20;
    param->bFrameAdaptive = X265_B_ADAPT_TRELLIS;
    param->bBPyramid = 1;
    param->scenecutThreshold = 40; /* Magic number pulled in from x264 */

    /* Intra Coding Tools */
    param->bEnableConstrainedIntra = 0;
    param->bEnableStrongIntraSmoothing = 1;

    /* Inter Coding tools */
    param->searchMethod = X265_HEX_SEARCH;
    param->subpelRefine = 2;
    param->searchRange = 57;
    param->maxNumMergeCand = 2;
    param->bEnableWeightedPred = 1;
    param->bEnableWeightedBiPred = 0;
    param->bEnableEarlySkip = 0;
    param->bEnableCbfFastMode = 0;
    param->bEnableAMP = 1;
    param->bEnableRectInter = 1;
    param->rdLevel = 3;
    param->bEnableSignHiding = 1;
    param->bEnableTransformSkip = 0;
    param->bEnableTSkipFast = 0;
    param->maxNumReferences = 3;

    /* Loop Filter */
    param->bEnableLoopFilter = 1;

    /* SAO Loop Filter */
    param->bEnableSAO = 1;
    param->saoLcuBoundary = 0;
    param->saoLcuBasedOptimization = 1;

    /* Coding Quality */
    param->cbQpOffset = 0;
    param->crQpOffset = 0;
    param->rdPenalty = 0;

    /* Rate control options */
    param->rc.vbvMaxBitrate = 0;
    param->rc.vbvBufferSize = 0;
    param->rc.vbvBufferInit = 0.9;
    param->rc.rfConstant = 28;
    param->rc.bitrate = 0;
    param->rc.rateTolerance = 1.0;
    param->rc.qCompress = 0.6;
    param->rc.ipFactor = 1.4f;
    param->rc.pbFactor = 1.3f;
    param->rc.qpStep = 4;
    param->rc.rateControlMode = X265_RC_CRF;
    param->rc.qp = 32;
    param->rc.aqMode = X265_AQ_VARIANCE;
    param->rc.aqStrength = 1.0;
    param->rc.cuTree = 1;

    /* Quality Measurement Metrics */
    param->bEnablePsnr = 0;
    param->bEnableSsim = 0;

    /* Video Usability Information (VUI) */
    param->bEnableVuiParametersPresentFlag = 0;
    param->bEnableAspectRatioIdc = 0;
    param->aspectRatioIdc = 0;
    param->sarWidth = 0;
    param->sarHeight = 0;
    param->bEnableOverscanAppropriateFlag = 0;
    param->bEnableVideoSignalTypePresentFlag = 0;
    param->videoFormat = 5;
    param->bEnableVideoFullRangeFlag = 0;
    param->bEnableColorDescriptionPresentFlag = 0;
    param->colorPrimaries = 2;
    param->transferCharacteristics = 2;
    param->matrixCoeffs = 2;
    param->bEnableChromaLocInfoPresentFlag = 0;
    param->chromaSampleLocTypeTopField = 0;
    param->chromaSampleLocTypeBottomField = 0;
    param->bEnableFieldSeqFlag = 0;
    param->bEnableFrameFieldInfoPresentFlag = 0;
    param->bEnableDefaultDisplayWindowFlag = 0;
    param->defDispWinLeftOffset = 0;
    param->defDispWinRightOffset = 0;
    param->defDispWinTopOffset = 0;
    param->defDispWinBottomOffset = 0;
    param->bEnableVuiTimingInfoPresentFlag = 0;
    param->bEnableVuiHrdParametersPresentFlag = 0;
    param->bEnableBitstreamRestrictionFlag = 0;
    param->bEnableSubPicHrdParamsPresentFlag = 0;
}

extern "C"
void x265_picture_init(x265_param *param, x265_picture *pic)
{
    memset(pic, 0, sizeof(x265_picture));

    pic->bitDepth = param->internalBitDepth;
    pic->colorSpace = param->internalCsp;
}

extern "C"
int x265_param_apply_profile(x265_param *param, const char *profile)
{
    if (!profile)
        return 0;
    if (!strcmp(profile, "main"))
    {}
    else if (!strcmp(profile, "main10"))
    {
#if HIGH_BIT_DEPTH
        param->internalBitDepth = 10;
#else
        x265_log(param, X265_LOG_WARNING, "Main10 not supported, not compiled for 16bpp.\n");
        return -1;
#endif
    }
    else if (!strcmp(profile, "mainstillpicture"))
    {
        param->keyframeMax = 1;
        param->bOpenGOP = 0;
    }
    else
    {
        x265_log(param, X265_LOG_ERROR, "unknown profile <%s>\n", profile);
        return -1;
    }

    return 0;
}

extern "C"
int x265_param_default_preset(x265_param *param, const char *preset, const char *tune)
{
    x265_param_default(param);

    if (preset)
    {
        char *end;
        int i = strtol(preset, &end, 10);
        if (*end == 0 && i >= 0 && i < (int)(sizeof(x265_preset_names) / sizeof(*x265_preset_names) - 1))
            preset = x265_preset_names[i];

        if (!strcmp(preset, "ultrafast"))
        {
            param->lookaheadDepth = 10;
            param->scenecutThreshold = 0; // disable lookahead
            param->maxCUSize = 32;
            param->searchRange = 25;
            param->bFrameAdaptive = 0;
            param->subpelRefine = 0;
            param->searchMethod = X265_DIA_SEARCH;
            param->bEnableRectInter = 0;
            param->bEnableAMP = 0;
            param->bEnableEarlySkip = 1;
            param->bEnableCbfFastMode = 1;
            param->bEnableSAO = 0;
            param->bEnableSignHiding = 0;
            param->bEnableWeightedPred = 0;
            param->maxNumReferences = 1;
            param->rc.aqStrength = 0.0;
            param->rc.aqMode = X265_AQ_NONE;
            param->rc.cuTree = 0;
        }
        else if (!strcmp(preset, "superfast"))
        {
            param->lookaheadDepth = 10;
            param->maxCUSize = 32;
            param->searchRange = 44;
            param->bFrameAdaptive = 0;
            param->subpelRefine = 1;
            param->bEnableRectInter = 0;
            param->bEnableAMP = 0;
            param->bEnableEarlySkip = 1;
            param->bEnableCbfFastMode = 1;
            param->bEnableWeightedPred = 0;
            param->maxNumReferences = 1;
            param->rc.aqStrength = 0.0;
            param->rc.aqMode = X265_AQ_NONE;
            param->rc.cuTree = 0;
        }
        else if (!strcmp(preset, "veryfast"))
        {
            param->lookaheadDepth = 15;
            param->maxCUSize = 32;
            param->bFrameAdaptive = 0;
            param->subpelRefine = 1;
            param->bEnableRectInter = 0;
            param->bEnableAMP = 0;
            param->bEnableEarlySkip = 1;
            param->bEnableCbfFastMode = 1;
            param->maxNumReferences = 1;
            param->rc.cuTree = 0;
        }
        else if (!strcmp(preset, "faster"))
        {
            param->lookaheadDepth = 15;
            param->bFrameAdaptive = 0;
            param->bEnableRectInter = 0;
            param->bEnableAMP = 0;
            param->bEnableEarlySkip = 1;
            param->bEnableCbfFastMode = 1;
            param->maxNumReferences = 1;
            param->rc.cuTree = 0;
        }
        else if (!strcmp(preset, "fast"))
        {
            param->lookaheadDepth = 15;
            param->bEnableRectInter = 0;
            param->bEnableAMP = 0;
        }
        else if (!strcmp(preset, "medium"))
        {
            /* defaults */
        }
        else if (!strcmp(preset, "slow"))
        {
            param->lookaheadDepth = 25;
            param->rdLevel = 4;
            param->subpelRefine = 3;
            param->maxNumMergeCand = 3;
            param->searchMethod = X265_STAR_SEARCH;
        }
        else if (!strcmp(preset, "slower"))
        {
            param->lookaheadDepth = 30;
            param->bframes = 8;
            param->tuQTMaxInterDepth = 2;
            param->tuQTMaxIntraDepth = 2;
            param->rdLevel = 6;
            param->subpelRefine = 3;
            param->maxNumMergeCand = 3;
            param->searchMethod = X265_STAR_SEARCH;
        }
        else if (!strcmp(preset, "veryslow"))
        {
            param->lookaheadDepth = 40;
            param->bframes = 8;
            param->tuQTMaxInterDepth = 3;
            param->tuQTMaxIntraDepth = 3;
            param->rdLevel = 6;
            param->subpelRefine = 4;
            param->maxNumMergeCand = 4;
            param->searchMethod = X265_STAR_SEARCH;
            param->maxNumReferences = 5;
        }
        else if (!strcmp(preset, "placebo"))
        {
            param->lookaheadDepth = 60;
            param->searchRange = 92;
            param->bframes = 8;
            param->tuQTMaxInterDepth = 4;
            param->tuQTMaxIntraDepth = 4;
            param->rdLevel = 6;
            param->subpelRefine = 5;
            param->maxNumMergeCand = 5;
            param->searchMethod = X265_STAR_SEARCH;
            param->bEnableTransformSkip = 1;
            param->maxNumReferences = 5;
            // TODO: optimized esa
        }
        else
            return -1;
    }
    if (tune)
    {
        if (!strcmp(tune, "psnr"))
        {
            param->rc.aqStrength = 0.0;
        }
        else if (!strcmp(tune, "ssim"))
        {
            param->rc.aqMode = X265_AQ_AUTO_VARIANCE;
        }
        else if (!strcmp(tune, "zero-latency"))
        {
            param->bFrameAdaptive = 0;
            param->bframes = 0;
            param->lookaheadDepth = 0;
        }
        else
            return -1;
    }

    return 0;
}

static inline int _confirm(x265_param *param, bool bflag, const char* message)
{
    if (!bflag)
        return 0;

    x265_log(param, X265_LOG_ERROR, "%s\n", message);
    return 1;
}

int x265_check_params(x265_param *param)
{
#define CHECK(expr, msg) check_failed |= _confirm(param, expr, msg)
    int check_failed = 0; /* abort if there is a fatal configuration problem */

    CHECK(param->maxCUSize > 64,
          "max ctu size should be less than 64");
    CHECK(param->maxCUSize < 16,
          "Maximum partition width size should be larger than or equal to 16");
    if (check_failed == 1)
        return check_failed;

    uint32_t maxCUDepth = (uint32_t)g_convertToBit[param->maxCUSize];
    uint32_t tuQTMaxLog2Size = maxCUDepth + 2 - 1;
    uint32_t tuQTMinLog2Size = 2; //log2(4)

    CHECK((param->maxCUSize >> maxCUDepth) < 4,
          "Minimum partition width size should be larger than or equal to 8");
    CHECK(param->internalCsp != X265_CSP_I420 && param->internalCsp != X265_CSP_I444,
          "Only 4:2:0 and 4:4:4 color spaces is supported at this time");

    /* These checks might be temporary */
#if HIGH_BIT_DEPTH
    CHECK(param->internalBitDepth != 10,
          "x265 was compiled for 10bit encodes, only 10bit inputs supported");
#endif

    CHECK(param->internalBitDepth > x265_max_bit_depth,
          "internalBitDepth must be <= x265_max_bit_depth");
    CHECK(param->rc.qp < -6 * (param->internalBitDepth - 8) || param->rc.qp > 51,
          "QP exceeds supported range (-QpBDOffsety to 51)");
    CHECK(param->fpsNum == 0 || param->fpsDenom == 0,
          "Frame rate numerator and denominator must be specified");
    CHECK(param->searchMethod<0 || param->searchMethod> X265_FULL_SEARCH,
          "Search method is not supported value (0:DIA 1:HEX 2:UMH 3:HM 5:FULL)");
    CHECK(param->searchRange < 0,
          "Search Range must be more than 0");
    CHECK(param->searchRange >= 32768,
          "Search Range must be less than 32768");
    CHECK(param->subpelRefine > X265_MAX_SUBPEL_LEVEL,
          "subme must be less than or equal to X265_MAX_SUBPEL_LEVEL (7)");
    CHECK(param->subpelRefine < 0,
          "subme must be greater than or equal to 0");
    CHECK(param->frameNumThreads < 0,
          "frameNumThreads (--frame-threads) must be 0 or higher");
    CHECK(param->cbQpOffset < -12, "Min. Chroma Cb QP Offset is -12");
    CHECK(param->cbQpOffset >  12, "Max. Chroma Cb QP Offset is  12");
    CHECK(param->crQpOffset < -12, "Min. Chroma Cr QP Offset is -12");
    CHECK(param->crQpOffset >  12, "Max. Chroma Cr QP Offset is  12");

    CHECK((1u << tuQTMaxLog2Size) > param->maxCUSize,
          "QuadtreeTULog2MaxSize must be log2(maxCUSize) or smaller.");

    CHECK(param->tuQTMaxInterDepth < 1,
          "QuadtreeTUMaxDepthInter must be greater than or equal to 1");
    CHECK(param->maxCUSize < (1u << (tuQTMinLog2Size + param->tuQTMaxInterDepth - 1)),
          "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1");
    CHECK(param->tuQTMaxIntraDepth < 1,
          "QuadtreeTUMaxDepthIntra must be greater than or equal to 1");
    CHECK(param->maxCUSize < (1u << (tuQTMinLog2Size + param->tuQTMaxIntraDepth - 1)),
          "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1");

    CHECK(param->maxNumMergeCand < 1, "MaxNumMergeCand must be 1 or greater.");
    CHECK(param->maxNumMergeCand > 5, "MaxNumMergeCand must be 5 or smaller.");

    CHECK(param->maxNumReferences < 1, "maxNumReferences must be 1 or greater.");
    CHECK(param->maxNumReferences > MAX_NUM_REF, "maxNumReferences must be 16 or smaller.");

    CHECK(param->sourceWidth  % TComSPS::getWinUnitX(param->internalCsp) != 0,
          "Picture width must be an integer multiple of the specified chroma subsampling");
    CHECK(param->sourceHeight % TComSPS::getWinUnitY(param->internalCsp) != 0,
          "Picture height must be an integer multiple of the specified chroma subsampling");

    CHECK(param->rc.rateControlMode<X265_RC_ABR || param->rc.rateControlMode> X265_RC_CRF,
          "Rate control mode is out of range");
    CHECK(param->rdLevel < 0 || param->rdLevel > 6,
          "RD Level is out of range");
    CHECK(param->bframes > param->lookaheadDepth,
          "Lookahead depth must be greater than the max consecutive bframe count");
    CHECK(param->bframes < 0,
          "bframe count should be greater than zero");
    CHECK(param->bframes > X265_BFRAME_MAX,
          "max consecutive bframe count must be 16 or smaller");
    CHECK(param->lookaheadDepth > X265_LOOKAHEAD_MAX,
          "Lookahead depth must be less than 256");
    CHECK(param->rc.aqMode < X265_AQ_NONE || X265_AQ_AUTO_VARIANCE < param->rc.aqMode,
          "Aq-Mode is out of range");
    CHECK(param->rc.aqStrength < 0 || param->rc.aqStrength > 3,
          "Aq-Strength is out of range");

    // max CU size should be power of 2
    uint32_t i = param->maxCUSize;
    while (i)
    {
        i >>= 1;
        if ((i & 1) == 1)
            CHECK(i != 1, "Max CU size should be 2^n");
    }

    CHECK(param->bEnableWavefront < 0, "WaveFrontSynchro cannot be negative");
    CHECK((param->aspectRatioIdc < 0
           || param->aspectRatioIdc > 16)
          && param->aspectRatioIdc != 255,
          "Sample Aspect Ratio must be 0-16 or 255");
    CHECK(param->sarWidth <= 0,
          "Sample Aspect Ratio width must be greater than 0");
    CHECK(param->sarHeight <= 0,
          "Sample Aspect Ratio height must be greater than 0");
    CHECK(param->videoFormat < 0 || param->videoFormat > 5,
          "Video Format must be Component component,"
          " pal, ntsc, secam, mac or undef");
    CHECK(param->colorPrimaries < 0
          || param->colorPrimaries > 9
          || param->colorPrimaries == 3,
          "Color Primaries must be undef, bt709, bt470m,"
          " bt470bg, smpte170m, smpte240m, film or bt2020");
    CHECK(param->transferCharacteristics < 0
          || param->transferCharacteristics > 15
          || param->transferCharacteristics == 3,
          "Transfer Characteristics must be undef, bt709, bt470m, bt470bg,"
          " smpte170m, smpte240m, linear, log100, log316, iec61966-2-4, bt1361e,"
          " iec61966-2-1, bt2020-10 or bt2020-12");
    CHECK(param->matrixCoeffs < 0
          || param->matrixCoeffs > 10
          || param->matrixCoeffs == 3,
          "Matrix Coefficients must be undef, bt709, fcc, bt470bg, smpte170m,"
          " smpte240m, GBR, YCgCo, bt2020nc or bt2020c");
    CHECK(param->chromaSampleLocTypeTopField < 0
          || param->chromaSampleLocTypeTopField > 5,
          "Chroma Sample Location Type Top Field must be 0-5");
    CHECK(param->chromaSampleLocTypeBottomField < 0
          || param->chromaSampleLocTypeBottomField > 5,
          "Chroma Sample Location Type Bottom Field must be 0-5");
    CHECK(param->defDispWinLeftOffset < 0,
          "Default Display Window Left Offset must be 0 or greater");
    CHECK(param->defDispWinRightOffset < 0,
          "Default Display Window Right Offset must be 0 or greater");
    CHECK(param->defDispWinTopOffset < 0,
          "Default Display Window Top Offset must be 0 or greater");
    CHECK(param->defDispWinBottomOffset < 0,
          "Default Display Window Bottom Offset must be 0 or greater");
    return check_failed;
}

int x265_set_globals(x265_param *param)
{
    uint32_t maxCUDepth = (uint32_t)g_convertToBit[param->maxCUSize];
    uint32_t tuQTMinLog2Size = 2; //log2(4)

    static int once /* = 0 */;

    if (ATOMIC_CAS32(&once, 0, 1) == 1)
    {
        if (param->maxCUSize != g_maxCUWidth)
        {
            x265_log(param, X265_LOG_ERROR, "maxCUSize must be the same for all encoders in a single process");
            return -1;
        }
        if (param->internalBitDepth != g_bitDepth)
        {
            x265_log(param, X265_LOG_ERROR, "internalBitDepth must be the same for all encoders in a single process");
            return -1;
        }
    }
    else
    {
        // set max CU width & height
        g_maxCUWidth  = param->maxCUSize;
        g_maxCUHeight = param->maxCUSize;
        g_bitDepth = param->internalBitDepth;

        // compute actual CU depth with respect to config depth and max transform size
        g_addCUDepth = 0;
        while ((param->maxCUSize >> maxCUDepth) > (1u << (tuQTMinLog2Size + g_addCUDepth)))
        {
            g_addCUDepth++;
        }

        maxCUDepth += g_addCUDepth;
        g_addCUDepth++;
        g_maxCUDepth = maxCUDepth;

        // initialize partition order
        uint32_t* tmp = &g_zscanToRaster[0];
        initZscanToRaster(g_maxCUDepth + 1, 1, 0, tmp);
        initRasterToZscan(g_maxCUWidth, g_maxCUHeight, g_maxCUDepth + 1);

        // initialize conversion matrix from partition index to pel
        initRasterToPelXY(g_maxCUWidth, g_maxCUHeight, g_maxCUDepth + 1);
    }
    return 0;
}

void x265_print_params(x265_param *param)
{
    if (param->logLevel < X265_LOG_INFO)
        return;
#if HIGH_BIT_DEPTH
    x265_log(param, X265_LOG_INFO, "Internal bit depth                  : %d\n", param->internalBitDepth);
#endif
    x265_log(param, X265_LOG_INFO, "CU size                             : %d\n", param->maxCUSize);
    x265_log(param, X265_LOG_INFO, "Max RQT depth inter / intra         : %d / %d\n", param->tuQTMaxInterDepth, param->tuQTMaxIntraDepth);

    x265_log(param, X265_LOG_INFO, "ME / range / subpel / merge         : %s / %d / %d / %d\n",
             x265_motion_est_names[param->searchMethod], param->searchRange, param->subpelRefine, param->maxNumMergeCand);
    if (param->keyframeMax != INT_MAX || param->scenecutThreshold)
    {
        x265_log(param, X265_LOG_INFO, "Keyframe min / max / scenecut       : %d / %d / %d\n", param->keyframeMin, param->keyframeMax, param->scenecutThreshold);
    }
    else
    {
        x265_log(param, X265_LOG_INFO, "Keyframe min / max / scenecut       : disabled\n");
    }
    if (param->cbQpOffset || param->crQpOffset)
    {
        x265_log(param, X265_LOG_INFO, "Cb/Cr QP Offset              : %d / %d\n", param->cbQpOffset, param->crQpOffset);
    }
    if (param->rdPenalty)
    {
        x265_log(param, X265_LOG_INFO, "RDpenalty                    : %d\n", param->rdPenalty);
    }
    x265_log(param, X265_LOG_INFO, "Lookahead / bframes / badapt        : %d / %d / %d\n", param->lookaheadDepth, param->bframes, param->bFrameAdaptive);
    x265_log(param, X265_LOG_INFO, "b-pyramid / weightp / refs          : %d / %d / %d\n", param->bBPyramid, param->bEnableWeightedPred, param->maxNumReferences);
    switch (param->rc.rateControlMode)
    {
    case X265_RC_ABR:
        x265_log(param, X265_LOG_INFO, "Rate Control / AQ-Strength / CUTree : ABR-%d kbps / %0.1f / %d\n", param->rc.bitrate,
                 param->rc.aqStrength, param->rc.cuTree);
        break;
    case X265_RC_CQP:
        x265_log(param, X265_LOG_INFO, "Rate Control / AQ-Strength / CUTree : CQP-%d / %0.1f / %d\n", param->rc.qp, param->rc.aqStrength,
                 param->rc.cuTree);
        break;
    case X265_RC_CRF:
        x265_log(param, X265_LOG_INFO, "Rate Control / AQ-Strength / CUTree : CRF-%0.1f / %0.1f / %d\n", param->rc.rfConstant,
                 param->rc.aqStrength, param->rc.cuTree);
        break;
    }

    x265_log(param, X265_LOG_INFO, "tools: ");
#define TOOLOPT(FLAG, STR) if (FLAG) fprintf(stderr, "%s ", STR)
    TOOLOPT(param->bEnableRectInter, "rect");
    TOOLOPT(param->bEnableAMP, "amp");
    TOOLOPT(param->bEnableCbfFastMode, "cfm");
    TOOLOPT(param->bEnableConstrainedIntra, "cip");
    TOOLOPT(param->bEnableEarlySkip, "esd");
    fprintf(stderr, "rd=%d ", param->rdLevel);

    TOOLOPT(param->bEnableLoopFilter, "lft");
    if (param->bEnableSAO)
    {
        if (param->saoLcuBasedOptimization)
            fprintf(stderr, "sao-lcu ");
        else
            fprintf(stderr, "sao-frame ");
    }
    TOOLOPT(param->bEnableSignHiding, "sign-hide");
    if (param->bEnableTransformSkip)
    {
        if (param->bEnableTSkipFast)
            fprintf(stderr, "tskip(fast) ");
        else
            fprintf(stderr, "tskip ");
    }
    TOOLOPT(param->bEnableWeightedBiPred, "weightbp");
    fprintf(stderr, "\n");
    fflush(stderr);
}

static int x265_atobool(const char *str, bool& bError)
{
    if (!strcmp(str, "1") ||
        !strcmp(str, "true") ||
        !strcmp(str, "yes"))
        return 1;
    if (!strcmp(str, "0") ||
        !strcmp(str, "false") ||
        !strcmp(str, "no"))
        return 0;
    bError = true;
    return 0;
}

static int x265_atoi(const char *str, bool& bError)
{
    char *end;
    int v = strtol(str, &end, 0);

    if (end == str || *end != '\0')
        bError = true;
    return v;
}

static double x265_atof(const char *str, bool& bError)
{
    char *end;
    double v = strtod(str, &end);

    if (end == str || *end != '\0')
        bError = true;
    return v;
}

static int parseName(const char *arg, const char * const * names, bool& bError)
{
    for (int i = 0; names[i]; i++)
    {
        if (!strcmp(arg, names[i]))
        {
            return i;
        }
    }

    return x265_atoi(arg, bError);
}

/* internal versions of string-to-int with additional error checking */
#undef atoi
#undef atof
#define atoi(str) x265_atoi(str, bError)
#define atof(str) x265_atof(str, bError)
#define atobool(str) (bNameWasBool = true, x265_atobool(str, bError))

extern "C"
int x265_param_parse(x265_param *p, const char *name, const char *value)
{
    bool bError = false;
    bool bNameWasBool = false;
    bool bValueWasNull = !value;
    char nameBuf[64];

    if (!name)
        return X265_PARAM_BAD_NAME;

    // s/_/-/g
    if (strlen(name) + 1 < sizeof(nameBuf) && strchr(name, '_'))
    {
        char *c;
        strcpy(nameBuf, name);
        while ((c = strchr(nameBuf, '_')))
        {
            *c = '-';
        }

        name = nameBuf;
    }

    int i;
    if ((!strncmp(name, "no-", 3) && (i = 3)) ||
        (!strncmp(name, "no", 2) && (i = 2)))
    {
        name += i;
        value = !value || x265_atobool(value, bError) ? "false" : "true";
    }
    else if (!value)
        value = "true";
    else if (value[0] == '=')
        value++;

#if defined(_MSC_VER)
#pragma warning(disable: 4127) // conditional expression is constant
#endif
#define OPT(STR) else if (!strcmp(name, STR))
    if (0) ;
    OPT("fps")
    {
        if (sscanf(value, "%u/%u", &p->fpsNum, &p->fpsDenom) == 2)
            ;
        else
        {
            float fps = (float)atof(value);
            if (fps > 0 && fps <= INT_MAX / 1000)
            {
                p->fpsNum = (int)(fps * 1000 + .5);
                p->fpsDenom = 1000;
            }
            else
            {
                p->fpsNum = atoi(value);
                p->fpsDenom = 1;
            }
        }
    }
    OPT("csv") p->csvfn = value;
    OPT("threads") p->poolNumThreads = atoi(value);
    OPT("frame-threads") p->frameNumThreads = atoi(value);
    OPT("log") p->logLevel = atoi(value);
    OPT("wpp") p->bEnableWavefront = atobool(value);
    OPT("ctu") p->maxCUSize = (uint32_t)atoi(value);
    OPT("tu-intra-depth") p->tuQTMaxIntraDepth = (uint32_t)atoi(value);
    OPT("tu-inter-depth") p->tuQTMaxInterDepth = (uint32_t)atoi(value);
    OPT("subme") p->subpelRefine = atoi(value);
    OPT("merange") p->searchRange = atoi(value);
    OPT("rect") p->bEnableRectInter = atobool(value);
    OPT("amp") p->bEnableAMP = atobool(value);
    OPT("max-merge") p->maxNumMergeCand = (uint32_t)atoi(value);
    OPT("early-skip") p->bEnableEarlySkip = atobool(value);
    OPT("fast-cbf") p->bEnableCbfFastMode = atobool(value);
    OPT("rdpenalty") p->rdPenalty = atoi(value);
    OPT("tskip") p->bEnableTransformSkip = atobool(value);
    OPT("no-tskip-fast") p->bEnableTSkipFast = atobool(value);
    OPT("tskip-fast") p->bEnableTSkipFast = atobool(value);
    OPT("strong-intra-smoothing") p->bEnableStrongIntraSmoothing = atobool(value);
    OPT("constrained-intra") p->bEnableConstrainedIntra = atobool(value);
    OPT("open-gop") p->bOpenGOP = atobool(value);
    OPT("scenecut") p->scenecutThreshold = atoi(value);
    OPT("keyint") p->keyframeMax = atoi(value);
    OPT("min-keyint") p->keyframeMin = atoi(value);
    OPT("rc-lookahead") p->lookaheadDepth = atoi(value);
    OPT("bframes") p->bframes = atoi(value);
    OPT("bframe-bias") p->bFrameBias = atoi(value);
    OPT("b-adapt") p->bFrameAdaptive = atoi(value);
    OPT("ref") p->maxNumReferences = atoi(value);
    OPT("weightp") p->bEnableWeightedPred = atobool(value);
    OPT("cbqpoffs") p->cbQpOffset = atoi(value);
    OPT("crqpoffs") p->crQpOffset = atoi(value);
    OPT("rd") p->rdLevel = atoi(value);
    OPT("signhide") p->bEnableSignHiding = atobool(value);
    OPT("lft") p->bEnableLoopFilter = atobool(value);
    OPT("sao") p->bEnableSAO = atobool(value);
    OPT("sao-lcu-bounds") p->saoLcuBoundary = atoi(value);
    OPT("sao-lcu-opt") p->saoLcuBasedOptimization = atoi(value);
    OPT("ssim") p->bEnableSsim = atobool(value);
    OPT("psnr") p->bEnablePsnr = atobool(value);
    OPT("hash") p->decodedPictureHashSEI = atoi(value);
    OPT("b-pyramid") p->bBPyramid = atobool(value);
    OPT("aq-mode") p->rc.aqMode = atoi(value);
    OPT("aq-strength") p->rc.aqStrength = atof(value);
    OPT("vbv-maxrate") p->rc.vbvMaxBitrate = atoi(value);
    OPT("vbv-bufsize") p->rc.vbvBufferSize = atoi(value);
    OPT("vbv-init")    p->rc.vbvBufferInit = atof(value);
    OPT("crf")
    {
        p->rc.rfConstant = atof(value);
        p->rc.rateControlMode = X265_RC_CRF;
    }
    OPT("bitrate")
    {
        p->rc.bitrate = atoi(value);
        p->rc.rateControlMode = X265_RC_ABR;
    }
    OPT("qp")
    {
        p->rc.qp = atoi(value);
        p->rc.rateControlMode = X265_RC_CQP;
    }
    OPT("input-csp") p->internalCsp = parseName(value, x265_source_csp_names, bError);
    OPT("me")        p->searchMethod = parseName(value, x265_motion_est_names, bError);
    OPT("cutree")    p->rc.cuTree = atobool(value);
    OPT("vui")
    {
        int bvalue = atobool(value);

        p->bEnableVuiParametersPresentFlag = bvalue;
        p->bEnableAspectRatioIdc = bvalue;
        p->bEnableOverscanInfoPresentFlag = bvalue;
        p->bEnableVideoSignalTypePresentFlag = bvalue;
        p->bEnableColorDescriptionPresentFlag = bvalue;
        p->bEnableChromaLocInfoPresentFlag = bvalue;
        p->bEnableFieldSeqFlag = bvalue;
        p->bEnableFrameFieldInfoPresentFlag = bvalue;
        p->bEnableDefaultDisplayWindowFlag = bvalue;
        p->bEnableVuiTimingInfoPresentFlag = bvalue;
        p->bEnableVuiHrdParametersPresentFlag = bvalue;
        p->bEnableBitstreamRestrictionFlag = bvalue;
        p->bEnableSubPicHrdParamsPresentFlag = bvalue;
    }
    OPT("sar")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableAspectRatioIdc = atobool(value);
        p->aspectRatioIdc = atoi(value);
    }
    OPT("extended-sar")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableAspectRatioIdc = atobool(value);
        p->aspectRatioIdc = 255;
        bError |= sscanf(value, "%dx%d", &p->sarWidth, &p->sarHeight) != 2;
    }
    OPT("overscan")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        if (!strcmp(value, "show"))
            p->bEnableOverscanInfoPresentFlag = atobool(value);
        else if (!strcmp(value, "crop"))
        {
            p->bEnableOverscanInfoPresentFlag = atobool(value);
            p->bEnableOverscanAppropriateFlag = atobool(value);
        }
        else
            p->bEnableOverscanInfoPresentFlag = -1;
    }
    OPT("videoformat")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableVideoSignalTypePresentFlag = atobool(value);
        if (!strcmp(value, "component"))
            p->videoFormat = 0;
        else if (!strcmp(value, "pal"))
            p->videoFormat = 1;
        else if (!strcmp(value, "ntsc"))
            p->videoFormat = 2;
        else if (!strcmp(value, "secam"))
            p->videoFormat = 3;
        else if (!strcmp(value, "mac"))
            p->videoFormat = 4;
        else if (!strcmp(value, "undef"))
            p->videoFormat = 5;
        else
            p->videoFormat = -1;
    }
    OPT("range")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableVideoSignalTypePresentFlag = atobool(value);
        p->bEnableVideoFullRangeFlag = atobool(value);
    }
    OPT("colorprim")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableVideoSignalTypePresentFlag = atobool(value);
        p->bEnableColorDescriptionPresentFlag = atobool(value);
        if (!strcmp(value, "bt709"))
            p->colorPrimaries = 1;
        else if (!strcmp(value, "undef"))
            p->colorPrimaries = 2;
        else if (!strcmp(value, "bt470m"))
            p->colorPrimaries = 4;
        else if (!strcmp(value, "bt470bg"))
            p->colorPrimaries = 5;
        else if (!strcmp(value, "smpte170m"))
            p->colorPrimaries = 6;
        else if (!strcmp(value, "smpte240m"))
            p->colorPrimaries = 7;
        else if (!strcmp(value, "film"))
            p->colorPrimaries = 8;
        else if (!strcmp(value, "bt2020"))
            p->colorPrimaries = 9;
        else
            p->colorPrimaries = -1;
    }
    OPT("transfer")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableVideoSignalTypePresentFlag = atobool(value);
        p->bEnableColorDescriptionPresentFlag = atobool(value);
        if (!strcmp(value, "bt709"))
            p->transferCharacteristics = 1;
        else if (!strcmp(value, "undef"))
            p->transferCharacteristics = 2;
        else if (!strcmp(value, "bt470m"))
            p->transferCharacteristics = 4;
        else if (!strcmp(value, "bt470bg"))
            p->transferCharacteristics = 5;
        else if (!strcmp(value, "smpte170m"))
            p->transferCharacteristics = 6;
        else if (!strcmp(value, "smpte240m"))
            p->transferCharacteristics = 7;
        else if (!strcmp(value, "linear"))
            p->transferCharacteristics = 8;
        else if (!strcmp(value, "log100"))
            p->transferCharacteristics = 9;
        else if (!strcmp(value, "log316"))
            p->transferCharacteristics = 10;
        else if (!strcmp(value, "iec61966-2-4"))
            p->transferCharacteristics = 11;
        else if (!strcmp(value, "bt1361e"))
            p->transferCharacteristics = 12;
        else if (!strcmp(value, "iec61966-2-1"))
            p->transferCharacteristics = 13;
        else if (!strcmp(value, "bt2020-10"))
            p->transferCharacteristics = 14;
        else if (!strcmp(value, "bt2020-12"))
            p->transferCharacteristics = 15;
        else
            p->transferCharacteristics = -1;
    }
    OPT("colormatrix")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableVideoSignalTypePresentFlag = atobool(value);
        p->bEnableColorDescriptionPresentFlag = atobool(value);
        if (!strcmp(value, "GBR"))
            p->matrixCoeffs = 0;
        else if (!strcmp(value, "bt709"))
            p->matrixCoeffs = 1;
        else if (!strcmp(value, "undef"))
            p->matrixCoeffs = 2;
        else if (!strcmp(value, "fcc"))
            p->matrixCoeffs = 4;
        else if (!strcmp(value, "bt470bg"))
            p->matrixCoeffs = 5;
        else if (!strcmp(value, "smpte170m"))
            p->matrixCoeffs = 6;
        else if (!strcmp(value, "smpte240m"))
            p->matrixCoeffs = 7;
        else if (!strcmp(value, "YCgCo"))
            p->matrixCoeffs = 8;
        else if (!strcmp(value, "bt2020nc"))
            p->matrixCoeffs = 9;
        else if (!strcmp(value, "bt2020c"))
            p->matrixCoeffs = 10;
        else
            p->matrixCoeffs = -1;
    }
    OPT("chromaloc")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableChromaLocInfoPresentFlag = atobool(value);
        p->chromaSampleLocTypeTopField = atoi(value);
        p->chromaSampleLocTypeBottomField = p->chromaSampleLocTypeTopField;
    }
    OPT("fieldseq")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableFieldSeqFlag = atobool(value);
    }
    OPT("framefieldinfo")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableFrameFieldInfoPresentFlag = atobool(value);
    }
    OPT("crop-rect")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableDefaultDisplayWindowFlag = atobool(value);
        bError |= sscanf(value, "%d,%d,%d,%d",
                         &p->defDispWinLeftOffset,
                         &p->defDispWinTopOffset,
                         &p->defDispWinRightOffset,
                         &p->defDispWinBottomOffset) != 4;
    }
    OPT("timinginfo")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableVuiTimingInfoPresentFlag = atobool(value);
    }
    OPT("nal-hrd")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableVuiTimingInfoPresentFlag = atobool(value);
        p->bEnableVuiHrdParametersPresentFlag = atobool(value);
    }
    OPT("bitstreamrestriction")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableBitstreamRestrictionFlag = atobool(value);
    }
    OPT("subpichrd")
    {
        p->bEnableVuiParametersPresentFlag = 1;
        p->bEnableVuiHrdParametersPresentFlag = atobool(value);
        p->bEnableSubPicHrdParamsPresentFlag = atobool(value);
    }
    else
        return X265_PARAM_BAD_NAME;
#undef OPT
#undef atobool
#undef atoi
#undef atof

    bError |= bValueWasNull && !bNameWasBool;
    return bError ? X265_PARAM_BAD_VALUE : 0;
}

char *x265_param2string(x265_param *p)
{
    char *buf, *s;

    buf = s = X265_MALLOC(char, 2000);
    if (!buf)
        return NULL;

#define BOOL(param, cliopt) \
    s += sprintf(s, " %s", (param) ? cliopt : "no-"cliopt);

    BOOL(p->bEnableWavefront, "wpp");
    s += sprintf(s, " fps=%d/%d", p->fpsNum, p->fpsDenom);
    s += sprintf(s, " ctu=%d", p->maxCUSize);
    s += sprintf(s, " tu-intra-depth=%d", p->tuQTMaxIntraDepth);
    s += sprintf(s, " tu-inter-depth=%d", p->tuQTMaxInterDepth);
    s += sprintf(s, " me=%d", p->searchMethod);
    s += sprintf(s, " subme=%d", p->subpelRefine);
    s += sprintf(s, " merange=%d", p->searchRange);
    BOOL(p->bEnableRectInter, "rect");
    BOOL(p->bEnableAMP, "amp");
    s += sprintf(s, " max-merge=%d", p->maxNumMergeCand);
    BOOL(p->bEnableEarlySkip, "early-skip");
    BOOL(p->bEnableCbfFastMode, "fast-cbf");
    s += sprintf(s, " rdpenalty=%d", p->rdPenalty);
    BOOL(p->bEnableTransformSkip, "tskip");
    BOOL(p->bEnableTSkipFast, "tskip-fast");
    BOOL(p->bEnableStrongIntraSmoothing, "strong-intra-smoothing");
    BOOL(p->bEnableConstrainedIntra, "constrained-intra");
    BOOL(p->bOpenGOP, "open-gop");
    s += sprintf(s, " keyint=%d", p->keyframeMax);
    s += sprintf(s, " min-keyint=%d", p->keyframeMin);
    s += sprintf(s, " scenecut=%d", p->scenecutThreshold);
    s += sprintf(s, " rc-lookahead=%d", p->lookaheadDepth);
    s += sprintf(s, " bframes=%d", p->bframes);
    s += sprintf(s, " bframe-bias=%d", p->bFrameBias);
    s += sprintf(s, " b-adapt=%d", p->bFrameAdaptive);
    s += sprintf(s, " ref=%d", p->maxNumReferences);
    BOOL(p->bEnableWeightedPred, "weightp");
    s += sprintf(s, " bitrate=%d", p->rc.bitrate);
    s += sprintf(s, " qp=%d", p->rc.qp);
    s += sprintf(s, " aq-mode=%d", p->rc.aqMode);
    s += sprintf(s, " aq-strength=%.2f", p->rc.aqStrength);
    s += sprintf(s, " cbqpoffs=%d", p->cbQpOffset);
    s += sprintf(s, " crqpoffs=%d", p->crQpOffset);
    s += sprintf(s, " rd=%d", p->rdLevel);
    BOOL(p->bEnableSignHiding, "signhide");
    BOOL(p->bEnableLoopFilter, "lft");
    BOOL(p->bEnableSAO, "sao");
    s += sprintf(s, " sao-lcu-bounds=%d", p->saoLcuBoundary);
    s += sprintf(s, " sao-lcu-opt=%d", p->saoLcuBasedOptimization);
    s += sprintf(s, " b-pyramid=%d", p->bBPyramid);
    BOOL(p->rc.cuTree, "cutree");
#undef BOOL

    return buf;
}
