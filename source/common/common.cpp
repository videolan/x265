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

static int parseName(const char *arg, const char * const * names, int& error);

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
    param->inputBitDepth = 8;
    param->internalCsp = X265_CSP_I420;

    /* CU definitions */
    param->maxCUSize = 64;
    param->tuQTMaxInterDepth = 1;
    param->tuQTMaxIntraDepth = 1;

    /* Coding Structure */
    param->decodingRefreshType = 1;
    param->keyframeMin = 0;
    param->keyframeMax = 250;
    param->bOpenGOP = 0;
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
    param->searchRange = 60;
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
    param->bEnableSsim = 1;
}

extern "C"
void x265_picture_init(x265_param *param, x265_picture *pic)
{
    memset(pic, 0, sizeof(x265_picture));
    /* This is the encoder internal bit depth */
    pic->bitDepth = param->inputBitDepth;
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
        param->inputBitDepth = 10;
#else
        x265_log(param, X265_LOG_WARNING, "not compiled for 16bpp. Falling back to main profile.\n");
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
            param->maxCUSize = 32;
            param->searchRange = 28;
            param->bFrameAdaptive = 0;
            param->subpelRefine = 0;
            param->maxNumMergeCand = 2;
            param->searchMethod = X265_DIA_SEARCH;
            param->bEnableRectInter = 0;
            param->bEnableAMP = 0;
            param->bEnableEarlySkip = 1;
            param->bEnableCbfFastMode = 1;
            param->bEnableLoopFilter = 1;
            param->bEnableSAO = 0;
            param->bEnableSignHiding = 0;
            param->bEnableWeightedPred = 0;
            param->maxNumReferences = 1;
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
            param->bframes = 4;
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
            param->rc.aqMode = X265_AQ_NONE;
            param->rc.aqStrength = 0.0;
            param->rc.cuTree = 0;
        }
        else if (!strcmp(tune, "ssim"))
        {
            /* Current Default: Do nothing */
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
    uint32_t maxCUDepth = (uint32_t)g_convertToBit[param->maxCUSize];
    uint32_t tuQTMaxLog2Size = maxCUDepth + 2 - 1;
    uint32_t tuQTMinLog2Size = 2; //log2(4)

    CHECK(param->inputBitDepth > x265_max_bit_depth,
          "inputBitDepth must be <= x265_max_bit_depth");

    CHECK(param->rc.qp < -6 * (param->inputBitDepth - 8) || param->rc.qp > 51,
          "QP exceeds supported range (-QpBDOffsety to 51)");
    CHECK(param->frameRate <= 0,
          "Frame rate must be more than 1");
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
    CHECK(param->keyframeMax < 0,
          "Keyframe interval must be 0 (auto) 1 (intra-only) or greater than 1");
    CHECK(param->frameNumThreads < 0,
          "frameNumThreads (--frame-threads) must be 0 or higher");
    CHECK(param->cbQpOffset < -12, "Min. Chroma Cb QP Offset is -12");
    CHECK(param->cbQpOffset >  12, "Max. Chroma Cb QP Offset is  12");
    CHECK(param->crQpOffset < -12, "Min. Chroma Cr QP Offset is -12");
    CHECK(param->crQpOffset >  12, "Max. Chroma Cr QP Offset is  12");

    CHECK((param->maxCUSize >> maxCUDepth) < 4,
          "Minimum partition width size should be larger than or equal to 8");
    CHECK(param->maxCUSize < 16,
          "Maximum partition width size should be larger than or equal to 16");

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
    CHECK(param->bframes > X265_BFRAME_MAX,
          "max consecutive bframe count must be 16 or smaller");
    CHECK(param->lookaheadDepth > X265_LOOKAHEAD_MAX,
          "Lookahead depth must be less than 256");
    CHECK(param->rc.aqMode < X265_AQ_NONE || param->rc.aqMode > X265_AQ_AUTO_VARIANCE,
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
    if (param->rc.rateControlMode == X265_RC_CQP)
    {
        param->rc.aqMode = X265_AQ_NONE;
        param->rc.bitrate = 0;   
        param->rc.cuTree = 0;
    }
    
    if (param->rc.aqMode == 0 && param->rc.cuTree)
    {
        param->rc.aqMode = X265_AQ_VARIANCE;
        param->rc.aqStrength = 0;
    }

    return check_failed;
}

int x265_set_globals(x265_param *param)
{
    uint32_t maxCUDepth = (uint32_t)g_convertToBit[param->maxCUSize];
    uint32_t tuQTMinLog2Size = 2; //log2(4)

    static int once /* = 0 */;

    if (ATOMIC_CAS(&once, 0, 1) == 1)
    {
        if (param->maxCUSize != g_maxCUWidth)
        {
            x265_log(param, X265_LOG_ERROR, "maxCUSize must be the same for all encoders in a single process");
            return -1;
        }
        if (param->inputBitDepth != g_bitDepth)
        {
            x265_log(param, X265_LOG_ERROR, "inputBitDepth must be the same for all encoders in a single process");
            return -1;
        }
    }
    else
    {
        // set max CU width & height
        g_maxCUWidth  = param->maxCUSize;
        g_maxCUHeight = param->maxCUSize;
        g_bitDepth = param->inputBitDepth;

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
    x265_log(param, X265_LOG_INFO, "Input bit depth                     : %d\n", param->inputBitDepth);
#endif
    x265_log(param, X265_LOG_INFO, "CU size                             : %d\n", param->maxCUSize);
    x265_log(param, X265_LOG_INFO, "Max RQT depth inter / intra         : %d / %d\n", param->tuQTMaxInterDepth, param->tuQTMaxIntraDepth);

    x265_log(param, X265_LOG_INFO, "ME / range / subpel / merge         : %s / %d / %d / %d\n",
             x265_motion_est_names[param->searchMethod], param->searchRange, param->subpelRefine, param->maxNumMergeCand);
    x265_log(param, X265_LOG_INFO, "Keyframe min / max                  : %d / %d\n", param->keyframeMin, param->keyframeMax);
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

extern "C"
int x265_param_parse(x265_param *p, const char *name, const char *value)
{
    int berror = 0;
    int valuewasnull;

    /* Enable or Disable - default is Enable */
    int bvalue = 1;

    if (!name)
        return X265_PARAM_BAD_NAME;

    if (!value)
        value = "1";

    if (!strncmp(name, "no-", 3))
    {
        bvalue = 0;
        name += 3;
    }
    else
        bvalue = 1;

    valuewasnull = !value;

#if defined(_MSC_VER)
#pragma warning(disable: 4127) // conditional expression is constant
#endif
#define OPT(STR) else if (!strcmp(name, STR))
    if (0) ;
    OPT("fps") p->frameRate = atoi(value);
    OPT("csv") p->csvfn = value;
    OPT("threads") p->poolNumThreads = atoi(value);
    OPT("frame-threads") p->frameNumThreads = atoi(value);
    OPT("log") p->logLevel = atoi(value);
    OPT("wpp") p->bEnableWavefront = bvalue;
    OPT("ctu") p->maxCUSize = (uint32_t)atoi(value);
    OPT("tu-intra-depth") p->tuQTMaxIntraDepth = (uint32_t)atoi(value);
    OPT("tu-inter-depth") p->tuQTMaxInterDepth = (uint32_t)atoi(value);
    OPT("subme") p->subpelRefine = atoi(value);
    OPT("merange") p->searchRange = atoi(value);
    OPT("rect") p->bEnableRectInter = bvalue;
    OPT("amp") p->bEnableAMP = bvalue;
    OPT("max-merge") p->maxNumMergeCand = (uint32_t)atoi(value);
    OPT("early-skip") p->bEnableEarlySkip = bvalue;
    OPT("fast-cbf") p->bEnableCbfFastMode = bvalue;
    OPT("rdpenalty") p->rdPenalty = atoi(value);
    OPT("tskip") p->bEnableTransformSkip = bvalue;
    OPT("no-tskip-fast") p->bEnableTSkipFast = bvalue;
    OPT("tskip-fast") p->bEnableTSkipFast = bvalue;
    OPT("strong-intra-smoothing") p->bEnableStrongIntraSmoothing = bvalue;
    OPT("constrained-intra") p->bEnableConstrainedIntra = bvalue;
    OPT("refresh") p->decodingRefreshType = atoi(value);
    OPT("keyint") p->keyframeMax = atoi(value);
    OPT("rc-lookahead") p->lookaheadDepth = atoi(value);
    OPT("bframes") p->bframes = atoi(value);
    OPT("bframe-bias") p->bFrameBias = atoi(value);
    OPT("b-adapt") p->bFrameAdaptive = atoi(value);
    OPT("ref") p->maxNumReferences = atoi(value);
    OPT("weightp") p->bEnableWeightedPred = bvalue;
    OPT("cbqpoffs") p->cbQpOffset = atoi(value);
    OPT("crqpoffs") p->crQpOffset = atoi(value);
    OPT("rd") p->rdLevel = atoi(value);
    OPT("signhide") p->bEnableSignHiding = bvalue;
    OPT("lft") p->bEnableLoopFilter = bvalue;
    OPT("sao") p->bEnableSAO = bvalue;
    OPT("sao-lcu-bounds") p->saoLcuBoundary = atoi(value);
    OPT("sao-lcu-opt") p->saoLcuBasedOptimization = atoi(value);
    OPT("ssim") p->bEnableSsim = bvalue;
    OPT("psnr") p->bEnablePsnr = bvalue;
    OPT("hash") p->decodedPictureHashSEI = atoi(value);
    OPT("b-pyramid") p->bBPyramid = bvalue;
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
    OPT("input-csp") p->internalCsp = parseName(value, x265_source_csp_names, berror);
    OPT("me")        p->searchMethod = parseName(value, x265_motion_est_names, berror);
    OPT("cutree")    p->rc.cuTree = bvalue;
    OPT("no-cutree") p->rc.cuTree = bvalue;

    else
        return X265_PARAM_BAD_NAME;
#undef OPT

    berror |= valuewasnull;
    return berror ? X265_PARAM_BAD_VALUE : 0;
}

char *x265_param2string(x265_param *p)
{
    char *buf, *s;

    buf = s = (char*)X265_MALLOC(char, 2000);
    if (!buf)
        return NULL;

#define BOOL(param, cliopt) \
    s += sprintf(s, " %s", (param) ? cliopt : "no-"cliopt);

    BOOL(p->bEnableWavefront, "wpp");
    s += sprintf(s, " fps=%d", p->frameRate);
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
    s += sprintf(s, " refresh=%d", p->decodingRefreshType);
    s += sprintf(s, " keyint=%d", p->keyframeMax);
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

static int parseName(const char *arg, const char * const * names, int& error)
{
    for (int i = 0; names[i]; i++)
    {
        if (!strcmp(arg, names[i]))
        {
            return i;
        }
    }

    int a = atoi(arg);
    if (a == 0 && strcmp(arg, "0"))
        error = 1;
    return a;
}
