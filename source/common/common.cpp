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

#include "x265.h"
#include "common.h"
#include "TLibCommon/TComRom.h"
#include "TLibCommon/TComSlice.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if _WIN32
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#include <time.h>

#if HIGH_BIT_DEPTH
const int x265_bit_depth = 10;
#else
const int x265_bit_depth = 8;
#endif

void x265_log( x265_param_t *param, int i_level, const char *fmt, ...)
{
    if( i_level > param->logLevel )
        return;
    char *s_level;
    switch( i_level )
    {
    case X265_LOG_ERROR:
        s_level = "error";
        break;
    case X265_LOG_WARNING:
        s_level = "warning";
        break;
    case X265_LOG_INFO:
        s_level = "info";
        break;
    case X265_LOG_DEBUG:
        s_level = "debug";
        break;
    default:
        s_level = "unknown";
        break;
    }
    fprintf( stderr, "x265 [%s]: ", s_level );
    va_list arg;
    va_start( arg, fmt );
    vfprintf( stderr, fmt, arg );
    va_end( arg );
}

extern "C"
void x265_param_default( x265_param_t *param )
{
    memset(param, 0, sizeof(x265_param_t));
    param->logLevel = X265_LOG_INFO;
    param->searchMethod = X265_STAR_SEARCH;
    param->iSearchRange = 64;
    param->bipredSearchRange = 4;
    param->iIntraPeriod = 16;
    param->internalBitDepth = 8;
    param->uiMaxCUSize = 64;
    param->uiMaxCUDepth = 4;
    param->uiQuadtreeTULog2MaxSize = 5;
    param->uiQuadtreeTULog2MinSize = 2;
    param->uiQuadtreeTUMaxDepthInter = 3;
    param->uiQuadtreeTUMaxDepthIntra = 3;
    param->enableAMP = 0;
    param->enableRectInter = 0;
    param->iQP = 32;
    param->iQPAdaptationRange = 6;
    param->bUseSAO = 0;
    param->maxNumOffsetsPerPic = 2048;
    param->saoLcuBasedOptimization = 1;
    param->log2ParallelMergeLevel = 2;
    param->maxNumMergeCand = 5u;
    param->TMVPModeId = 1;
    param->signHideFlag = 1;
    param->useFastDecisionForMerge = 1;
    param->useStrongIntraSmoothing = 1;
    param->useRDOQ = 1;
    param->useRDOQTS = 1;
    param->useTransformSkip = 1;
    param->useTransformSkipFast = 1;
}

extern "C"
int x265_param_apply_profile( x265_param_t *param, const char *profile )
{
    if (!profile)
        return 0;
    if (!strcmp(profile, "main"))
    {
        param->iIntraPeriod = 16;
    }
    else if (!strcmp(profile, "main10"))
    {
        param->iIntraPeriod = 16;
#if HIGH_BIT_DEPTH
        param->internalBitDepth = 10;
#else
        x265_log(param, X265_LOG_WARNING, "not compiled for 16bpp. Falling back to main profile.\n");
        return -1;
#endif
    }
    else if (!strcmp(profile, "mainstillpicture"))
    {
        param->iIntraPeriod = 1;
    }
    else
    {
        x265_log(param, X265_LOG_ERROR, "unknown profile <%s>\n", profile);
        return -1;
    }

    return 0;
}

static inline int _confirm(x265_param_t *param, bool bflag, const char* message)
{
    if (!bflag)
        return 0;

    x265_log(param, X265_LOG_ERROR, "%s\n", message);
    return 1;
}

int x265_check_params(x265_param_t *param)
{
#define CONFIRM(expr, msg) check_failed |= _confirm(param, expr, msg)
    int check_failed = 0; /* abort if there is a fatal configuration problem */

    if (param->iWaveFrontSynchro == 0)
        param->poolNumThreads = 1;

#if !HIGH_BIT_DEPTH
    CONFIRM(param->internalBitDepth != 8,
        "InternalBitDepth must be 8");
#endif
    CONFIRM(param->iQP <  -6 * (param->internalBitDepth - 8) || param->iQP > 51,
        "QP exceeds supported range (-QpBDOffsety to 51)");
    CONFIRM(param->iFrameRate <= 0,
        "Frame rate must be more than 1");
    CONFIRM(param->searchMethod < 0 || param->searchMethod > X265_ORIG_SEARCH,
        "Search method is not supported value (0:DIA 1:HEX 2:UMH 3:HM 4:ORIG)");
    CONFIRM(param->iSearchRange < 0,
        "Search Range must be more than 0");
    CONFIRM(param->bipredSearchRange < 0,
        "Search Range must be more than 0");
    CONFIRM(param->iMaxCuDQPDepth > param->uiMaxCUDepth - 1,
        "Absolute depth for a minimum CuDQP exceeds maximum coding unit depth");

    CONFIRM(param->cbQpOffset < -12,   "Min. Chroma Cb QP Offset is -12");
    CONFIRM(param->cbQpOffset >  12,   "Max. Chroma Cb QP Offset is  12");
    CONFIRM(param->crQpOffset < -12,   "Min. Chroma Cr QP Offset is -12");
    CONFIRM(param->crQpOffset >  12,   "Max. Chroma Cr QP Offset is  12");

    CONFIRM(param->iQPAdaptationRange <= 0,
        "QP Adaptation Range must be more than 0");
    CONFIRM((param->uiMaxCUSize >> param->uiMaxCUDepth) < 4,
        "Minimum partition width size should be larger than or equal to 8");
    CONFIRM(param->uiMaxCUSize < 16,
        "Maximum partition width size should be larger than or equal to 16");
    CONFIRM((param->iSourceWidth  % (param->uiMaxCUSize >> (param->uiMaxCUDepth - 1))) != 0,
        "Resulting coded frame width must be a multiple of the minimum CU size");
    CONFIRM((param->iSourceHeight % (param->uiMaxCUSize >> (param->uiMaxCUDepth - 1))) != 0,
        "Resulting coded frame height must be a multiple of the minimum CU size");

    CONFIRM(param->uiQuadtreeTULog2MinSize < 2,
        "QuadtreeTULog2MinSize must be 2 or greater.");
    CONFIRM(param->uiQuadtreeTULog2MaxSize > 5,
        "QuadtreeTULog2MaxSize must be 5 or smaller.");
    CONFIRM((1u << param->uiQuadtreeTULog2MaxSize) > param->uiMaxCUSize,
        "QuadtreeTULog2MaxSize must be log2(maxCUSize) or smaller.");

    CONFIRM(param->uiQuadtreeTULog2MaxSize < param->uiQuadtreeTULog2MinSize,
        "QuadtreeTULog2MaxSize must be greater than or equal to m_uiQuadtreeTULog2MinSize.");
    CONFIRM((1u << param->uiQuadtreeTULog2MinSize) > (param->uiMaxCUSize >> (param->uiMaxCUDepth - 1)),
        "QuadtreeTULog2MinSize must not be greater than minimum CU size"); // HS
    CONFIRM((1u << param->uiQuadtreeTULog2MinSize) > (param->uiMaxCUSize >> (param->uiMaxCUDepth - 1)),
        "QuadtreeTULog2MinSize must not be greater than minimum CU size"); // HS
    CONFIRM((1u << param->uiQuadtreeTULog2MinSize) > (param->uiMaxCUSize >> param->uiMaxCUDepth),
        "Minimum CU width must be greater than minimum transform size.");
    CONFIRM((1u << param->uiQuadtreeTULog2MinSize) > (param->uiMaxCUSize >> param->uiMaxCUDepth),
        "Minimum CU height must be greater than minimum transform size.");
    CONFIRM(param->uiQuadtreeTUMaxDepthInter < 1,
        "QuadtreeTUMaxDepthInter must be greater than or equal to 1");
    CONFIRM(param->uiMaxCUSize < (1u << (param->uiQuadtreeTULog2MinSize + param->uiQuadtreeTUMaxDepthInter - 1)),
        "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1");
    CONFIRM(param->uiQuadtreeTUMaxDepthIntra < 1,
        "QuadtreeTUMaxDepthIntra must be greater than or equal to 1");
    CONFIRM(param->uiMaxCUSize < (1u << (param->uiQuadtreeTULog2MinSize + param->uiQuadtreeTUMaxDepthIntra - 1)),
        "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1");

    CONFIRM(param->maxNumMergeCand < 1, "MaxNumMergeCand must be 1 or greater.");
    CONFIRM(param->maxNumMergeCand > 5, "MaxNumMergeCand must be 5 or smaller.");

    CONFIRM(param->bUseAdaptQpSelect && param->iQP < 0,
        "AdaptiveQpSelection must be disabled when QP < 0.");
    CONFIRM(param->bUseAdaptQpSelect && (param->cbQpOffset != 0 || param->crQpOffset != 0),
        "AdaptiveQpSelection must be disabled when ChromaQpOffset is not equal to 0.");

    //TODO:ChromaFmt assumes 4:2:0 below
    CONFIRM(param->iSourceWidth  % TComSPS::getWinUnitX(CHROMA_420) != 0,
        "Picture width must be an integer multiple of the specified chroma subsampling");
    CONFIRM(param->iSourceHeight % TComSPS::getWinUnitY(CHROMA_420) != 0,
        "Picture height must be an integer multiple of the specified chroma subsampling");

    // max CU size should be power of 2
    uint32_t ui = param->uiMaxCUSize;
    while (ui)
    {
        ui >>= 1;
        if ((ui & 1) == 1)
            CONFIRM(ui != 1, "Width should be 2^n");
    }

    CONFIRM(param->iWaveFrontSynchro < 0, "WaveFrontSynchro cannot be negative");

    CONFIRM(param->log2ParallelMergeLevel < 2,
        "Log2ParallelMergeLevel should be larger than or equal to 2");

    CONFIRM(param->iWaveFrontSynchro && param->bUseAdaptQpSelect,
        "Adaptive QP Select cannot be used together with WPP");

    return check_failed;
}

void x265_set_globals(x265_param_t *param, uint32_t inputBitDepth)
{
    // set max CU width & height
    g_uiMaxCUWidth  = param->uiMaxCUSize;
    g_uiMaxCUHeight = param->uiMaxCUSize;

    // compute actual CU depth with respect to config depth and max transform size
    g_uiAddCUDepth  = 0;
    while ((param->uiMaxCUSize >> param->uiMaxCUDepth) > (1u << (param->uiQuadtreeTULog2MinSize + g_uiAddCUDepth)))
    {
        g_uiAddCUDepth++;
    }

    param->uiMaxCUDepth += g_uiAddCUDepth;
    g_uiAddCUDepth++;
    g_uiMaxCUDepth = param->uiMaxCUDepth;

    // set internal bit-depth and constants
#if HIGH_BIT_DEPTH
    g_bitDepthY = param->internalBitDepth;
    g_bitDepthC = param->internalBitDepth;
#else
    g_bitDepthY = g_bitDepthC = 8;
    g_uiPCMBitDepthLuma = g_uiPCMBitDepthChroma = 8;
#endif

    g_uiPCMBitDepthLuma = inputBitDepth;
    g_uiPCMBitDepthChroma = inputBitDepth;
}

void x265_print_params(x265_param_t *param)
{
    if (param->logLevel < X265_LOG_INFO)
        return;
    x265_log(param, X265_LOG_INFO, "Format                       : %dx%d %dHz\n", param->iSourceWidth, param->iSourceHeight, param->iFrameRate);
#if HIGH_BIT_DEPTH
    x265_log(param, X265_LOG_INFO, "Internal bit depth           : %d\n", param->internalBitDepth);
#endif
    x265_log(param, X265_LOG_INFO, "CU size / depth              : %d / %d\n", param->uiMaxCUSize, param->uiMaxCUDepth);
    x265_log(param, X265_LOG_INFO, "RQT trans. size (min / max)  : %d / %d\n", 1 << param->uiQuadtreeTULog2MinSize, 1 << param->uiQuadtreeTULog2MaxSize);
    x265_log(param, X265_LOG_INFO, "Max RQT depth inter / intra  : %d / %d\n", param->uiQuadtreeTUMaxDepthInter, param->uiQuadtreeTUMaxDepthIntra);

    x265_log(param, X265_LOG_INFO, "Motion search / range        : %s / %d\n", x265_motion_est_names[param->searchMethod], param->iSearchRange );
    x265_log(param, X265_LOG_INFO, "Max Num Merge Candidates     : %d ", param->maxNumMergeCand);
    printf("PME:%d ", param->log2ParallelMergeLevel);
    printf("TMVPMode:%d\n", param->TMVPModeId);
    x265_log(param, X265_LOG_INFO, "Intra period                 : %d\n", param->iIntraPeriod);
    if (param->iWaveFrontSynchro)
    {
        x265_log(param, X265_LOG_INFO, "WaveFrontSubstreams          : %d\n", (param->iSourceHeight + param->uiMaxCUSize - 1) / param->uiMaxCUSize);
    }
    x265_log(param, X265_LOG_INFO, "QP                           : %d\n", param->iQP);
    if (param->iMaxCuDQPDepth)
    {
        x265_log(param, X265_LOG_INFO, "Max dQP signaling depth      : %d\n", param->iMaxCuDQPDepth);
    }
    if (param->cbQpOffset || param->crQpOffset)
    {
        x265_log(param, X265_LOG_INFO, "Cb/Cr QP Offset              : %d / %d\n", param->cbQpOffset, param->crQpOffset);
    }
    if (param->bUseAdaptiveQP)
    {
        x265_log(param, X265_LOG_INFO, "QP adaptation                : %d (range=%d)\n", param->bUseAdaptiveQP, param->iQPAdaptationRange);
    }
    if (param->rdPenalty)
    {
        x265_log(param, X265_LOG_INFO, "RDpenalty                    : %d\n", param->rdPenalty);
    }
    x265_log(param, X265_LOG_INFO, "enabled coding tools: ");
#if FAST_MODE_DECISION
    printf("fmd ");
#endif
#define TOOLOPT(FLAG, STR) if (FLAG) printf("%s ", STR)
    TOOLOPT(param->enableRectInter, "rect");
    TOOLOPT(param->enableAMP, "amp");
    TOOLOPT(param->useFastDecisionForMerge, "fdm");
    TOOLOPT(param->bUseCbfFastMode, "cfm");
    TOOLOPT(param->useEarlySkipDetection, "esd");
    TOOLOPT(param->useRDOQ, "rdoq");
    if (param->useTransformSkip)
    {
        TOOLOPT(param->useTransformSkip, "tskip");
        TOOLOPT(param->useTransformSkipFast, "tskip-fast");
        TOOLOPT(param->useRDOQTS, "rdoqts");
    }
    if (param->bUseSAO)
    {
        TOOLOPT(param->bUseSAO, "sao");
        TOOLOPT(param->saoLcuBasedOptimization, "sao-lcu");
    }
    TOOLOPT(param->useWeightedPred, "weightp");
    TOOLOPT(param->useWeightedBiPred, "weightbp");
    TOOLOPT(param->bUseAdaptQpSelect, "aq");
    TOOLOPT(param->signHideFlag, "sign-hide");
    TOOLOPT(param->bUseConstrainedIntraPred, "cip");
    printf("\n");
    fflush(stderr);
}

int64_t x265_mdate(void)
{
#if _WIN32
    struct timeb tb;
    ftime( &tb );
    return ((int64_t)tb.time * 1000 + (int64_t)tb.millitm) * 1000;
#else
    struct timeval tv_date;
    gettimeofday( &tv_date, NULL );
    return (int64_t)tv_date.tv_sec * 1000000 + (int64_t)tv_date.tv_usec;
#endif
}

int dumpBuffer(void * pbuf, size_t bufsize, const char * filename)
{
    const char * mode = "wb";

    FILE * fp = fopen(filename, mode);
    if(!fp)
    {
        printf("ERROR: dumpBuffer: fopen('%s','%s') failed\n", filename, mode); return -1;
    }
    fwrite(pbuf, 1, bufsize, fp);
    fclose(fp);
    printf("dumpBuffer: dumped %8ld bytes into %s\n", (long)bufsize, filename);
    return 0;
}
