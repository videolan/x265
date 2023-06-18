/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
 *
 * Authors: Deepthi Nandakumar <deepthi@multicorewareinc.com>
 *          Min Chen <min.chen@multicorewareinc.com>
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
#include "threading.h"
#include "param.h"
#include "cpu.h"
#include "x265.h"
#include "svt.h"

#if _MSC_VER
#pragma warning(disable: 4996) // POSIX functions are just fine, thanks
#pragma warning(disable: 4706) // assignment within conditional
#pragma warning(disable: 4127) // conditional expression is constant
#endif

#if _WIN32
#define strcasecmp _stricmp
#endif

#if !defined(HAVE_STRTOK_R)

/*
 * adapted from public domain strtok_r() by Charlie Gordon
 *
 *   from comp.lang.c  9/14/2007
 *
 *      http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 *     (Declaration that it's public domain):
 *      http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */

#undef strtok_r
static char* strtok_r(char* str, const char* delim, char** nextp)
{
    if (!str)
        str = *nextp;

    str += strspn(str, delim);

    if (!*str)
        return NULL;

    char *ret = str;

    str += strcspn(str, delim);

    if (*str)
        *str++ = '\0';

    *nextp = str;

    return ret;
}

#endif // if !defined(HAVE_STRTOK_R)

#if EXPORT_C_API

/* these functions are exported as C functions (default) */
using namespace X265_NS;
extern "C" {

#else

/* these functions exist within private namespace (multilib) */
namespace X265_NS {

#endif

x265_param *x265_param_alloc()
{
    x265_param* param = (x265_param*)x265_malloc(sizeof(x265_param));
#ifdef SVT_HEVC
    param->svtHevcParam = (EB_H265_ENC_CONFIGURATION*)x265_malloc(sizeof(EB_H265_ENC_CONFIGURATION));
#endif
    return param;
}

void x265_param_free(x265_param* p)
{
    x265_zone_free(p);
#ifdef SVT_HEVC
     x265_free(p->svtHevcParam);
#endif
    x265_free(p);
}

void x265_param_default(x265_param* param)
{
#ifdef SVT_HEVC
    EB_H265_ENC_CONFIGURATION* svtParam = (EB_H265_ENC_CONFIGURATION*)param->svtHevcParam;
#endif

    memset(param, 0, sizeof(x265_param));

    /* Applying default values to all elements in the param structure */
    param->cpuid = X265_NS::cpu_detect(false);
    param->bEnableWavefront = 1;
    param->frameNumThreads = 0;

    param->logLevel = X265_LOG_INFO;
    param->csvLogLevel = 0;
    param->csvfn = NULL;
    param->rc.lambdaFileName = NULL;
    param->bLogCuStats = 0;
    param->decodedPictureHashSEI = 0;

    /* Quality Measurement Metrics */
    param->bEnablePsnr = 0;
    param->bEnableSsim = 0;

    /* Source specifications */
    param->internalBitDepth = X265_DEPTH;
    param->sourceBitDepth = 8;
    param->internalCsp = X265_CSP_I420;
    param->levelIdc = 0; //Auto-detect level
    param->uhdBluray = 0;
    param->bHighTier = 1; //Allow high tier by default
    param->interlaceMode = 0;
    param->bField = 0;
    param->bAnnexB = 1;
    param->bRepeatHeaders = 0;
    param->bEnableAccessUnitDelimiters = 0;
    param->bEnableEndOfBitstream = 0;
    param->bEnableEndOfSequence = 0;
    param->bEmitHRDSEI = 0;
    param->bEmitInfoSEI = 1;
    param->bEmitHDRSEI = 0; /*Deprecated*/
    param->bEmitHDR10SEI = 0;
    param->bEmitIDRRecoverySEI = 0;

    /* CU definitions */
    param->maxCUSize = 64;
    param->minCUSize = 8;
    param->tuQTMaxInterDepth = 1;
    param->tuQTMaxIntraDepth = 1;
    param->maxTUSize = 32;

    /* Coding Structure */
    param->keyframeMin = 0;
    param->keyframeMax = 250;
    param->gopLookahead = 0;
    param->bOpenGOP = 1;
    param->bframes = 4;
    param->lookaheadDepth = 20;
    param->bFrameAdaptive = X265_B_ADAPT_TRELLIS;
    param->bBPyramid = 1;
    param->scenecutThreshold = 40; /* Magic number pulled in from x264 */
    param->bHistBasedSceneCut = 0;
    param->lookaheadSlices = 8;
    param->lookaheadThreads = 0;
    param->scenecutBias = 5.0;
    param->radl = 0;
    param->chunkStart = 0;
    param->chunkEnd = 0;
    param->bEnableHRDConcatFlag = 0;
    param->bEnableFades = 0;
    param->bEnableSceneCutAwareQp = 0;
    param->fwdMaxScenecutWindow = 1200;
    param->bwdMaxScenecutWindow = 600;
    for (int i = 0; i < 6; i++)
    {
        int deltas[6] = { 5, 4, 3, 2, 1, 0 };

        param->fwdScenecutWindow[i] = 200;
        param->fwdRefQpDelta[i] = deltas[i];
        param->fwdNonRefQpDelta[i] = param->fwdRefQpDelta[i] + (SLICE_TYPE_DELTA * param->fwdRefQpDelta[i]);

        param->bwdScenecutWindow[i] = 100;
        param->bwdRefQpDelta[i] = -1;
        param->bwdNonRefQpDelta[i] = -1;
    }

    /* Intra Coding Tools */
    param->bEnableConstrainedIntra = 0;
    param->bEnableStrongIntraSmoothing = 1;
    param->bEnableFastIntra = 0;
    param->bEnableSplitRdSkip = 0;

    /* Inter Coding tools */
    param->searchMethod = X265_HEX_SEARCH;
    param->subpelRefine = 2;
    param->searchRange = 57;
    param->maxNumMergeCand = 3;
    param->limitReferences = 1;
    param->limitModes = 0;
    param->bEnableWeightedPred = 1;
    param->bEnableWeightedBiPred = 0;
    param->bEnableEarlySkip = 1;
    param->recursionSkipMode = 1;
    param->edgeVarThreshold = 0.05f;
    param->bEnableAMP = 0;
    param->bEnableRectInter = 0;
    param->rdLevel = 3;
    param->rdoqLevel = 0;
    param->bEnableSignHiding = 1;
    param->bEnableTransformSkip = 0;
    param->bEnableTSkipFast = 0;
    param->maxNumReferences = 3;
    param->bEnableTemporalMvp = 1;
    param->bEnableHME = 0;
    param->hmeSearchMethod[0] = X265_HEX_SEARCH;
    param->hmeSearchMethod[1] = param->hmeSearchMethod[2] = X265_UMH_SEARCH;
    param->hmeRange[0] = 16;
    param->hmeRange[1] = 32;
    param->hmeRange[2] = 48;
    param->bSourceReferenceEstimation = 0;
    param->limitTU = 0;
    param->dynamicRd = 0;

    /* Loop Filter */
    param->bEnableLoopFilter = 1;

    /* SAO Loop Filter */
    param->bEnableSAO = 1;
    param->bSaoNonDeblocked = 0;
    param->bLimitSAO = 0;
    param->selectiveSAO = 0;

    /* Coding Quality */
    param->cbQpOffset = 0;
    param->crQpOffset = 0;
    param->rdPenalty = 0;
    param->psyRd = 2.0;
    param->psyRdoq = 0.0;
    param->analysisReuseMode = 0; /*DEPRECATED*/
    param->analysisMultiPassRefine = 0;
    param->analysisMultiPassDistortion = 0;
    param->analysisReuseFileName = NULL;
    param->analysisSave = NULL;
    param->analysisLoad = NULL;
    param->bIntraInBFrames = 1;
    param->bLossless = 0;
    param->bCULossless = 0;
    param->bEnableTemporalSubLayers = 0;
    param->bEnableRdRefine = 0;
    param->bMultiPassOptRPS = 0;
    param->bSsimRd = 0;

    /* Rate control options */
    param->rc.vbvMaxBitrate = 0;
    param->rc.vbvBufferSize = 0;
    param->rc.vbvBufferInit = 0.9;
    param->vbvBufferEnd = 0;
    param->vbvEndFrameAdjust = 0;
    param->minVbvFullness = 50;
    param->maxVbvFullness = 80;
    param->rc.rfConstant = 28;
    param->rc.bitrate = 0;
    param->rc.qCompress = 0.6;
    param->rc.ipFactor = 1.4f;
    param->rc.pbFactor = 1.3f;
    param->rc.qpStep = 4;
    param->rc.rateControlMode = X265_RC_CRF;
    param->rc.qp = 32;
    param->rc.aqMode = X265_AQ_AUTO_VARIANCE;
    param->rc.hevcAq = 0;
    param->rc.qgSize = 32;
    param->rc.aqStrength = 1.0;
    param->rc.qpAdaptationRange = 1.0;
    param->rc.cuTree = 1;
    param->rc.rfConstantMax = 0;
    param->rc.rfConstantMin = 0;
    param->rc.bStatRead = 0;
    param->rc.bStatWrite = 0;
    param->rc.dataShareMode = X265_SHARE_MODE_FILE;
    param->rc.statFileName = NULL;
    param->rc.sharedMemName = NULL;
    param->rc.bEncFocusedFramesOnly = 0;
    param->rc.complexityBlur = 20;
    param->rc.qblur = 0.5;
    param->rc.zoneCount = 0;
    param->rc.zonefileCount = 0;
    param->rc.zones = NULL;
    param->rc.bEnableSlowFirstPass = 1;
    param->rc.bStrictCbr = 0;
    param->rc.bEnableGrain = 0;
    param->rc.qpMin = 0;
    param->rc.qpMax = QP_MAX_MAX;
    param->rc.bEnableConstVbv = 0;
    param->bResetZoneConfig = 1;
    param->reconfigWindowSize = 0;
    param->decoderVbvMaxRate = 0;
    param->bliveVBV2pass = 0;

    /* Video Usability Information (VUI) */
    param->vui.aspectRatioIdc = 0;
    param->vui.sarWidth = 0;
    param->vui.sarHeight = 0;
    param->vui.bEnableOverscanAppropriateFlag = 0;
    param->vui.bEnableVideoSignalTypePresentFlag = 0;
    param->vui.videoFormat = 5;
    param->vui.bEnableVideoFullRangeFlag = 0;
    param->vui.bEnableColorDescriptionPresentFlag = 0;
    param->vui.colorPrimaries = 2;
    param->vui.transferCharacteristics = 2;
    param->vui.matrixCoeffs = 2;
    param->vui.bEnableChromaLocInfoPresentFlag = 0;
    param->vui.chromaSampleLocTypeTopField = 0;
    param->vui.chromaSampleLocTypeBottomField = 0;
    param->vui.bEnableDefaultDisplayWindowFlag = 0;
    param->vui.defDispWinLeftOffset = 0;
    param->vui.defDispWinRightOffset = 0;
    param->vui.defDispWinTopOffset = 0;
    param->vui.defDispWinBottomOffset = 0;
    param->maxCLL = 0;
    param->maxFALL = 0;
    param->minLuma = 0;
    param->maxLuma = PIXEL_MAX;
    param->log2MaxPocLsb = 8;
    param->maxSlices = 1;
    param->videoSignalTypePreset = NULL;

    /*Conformance window*/
    param->confWinRightOffset = 0;
    param->confWinBottomOffset = 0;

    param->bEmitVUITimingInfo   = 1;
    param->bEmitVUIHRDInfo      = 1;
    param->bOptQpPPS            = 0;
    param->bOptRefListLengthPPS = 0;
    param->bOptCUDeltaQP        = 0;
    param->bAQMotion = 0;
    param->bHDROpt = 0; /*DEPRECATED*/
    param->bHDR10Opt = 0;
    param->analysisReuseLevel = 0;  /*DEPRECATED*/
    param->analysisSaveReuseLevel = 0;
    param->analysisLoadReuseLevel = 0;
    param->toneMapFile = NULL;
    param->bDhdr10opt = 0;
    param->dolbyProfile = 0;
    param->bCTUInfo = 0;
    param->bUseRcStats = 0;
    param->scaleFactor = 0;
    param->intraRefine = 0;
    param->interRefine = 0;
    param->bDynamicRefine = 0;
    param->mvRefine = 1;
    param->ctuDistortionRefine = 0;
    param->bUseAnalysisFile = 1;
    param->csvfpt = NULL;
    param->forceFlush = 0;
    param->bDisableLookahead = 0;
    param->bCopyPicToFrame = 1;
    param->maxAUSizeFactor = 1;
    param->naluFile = NULL;

    /* DCT Approximations */
    param->bLowPassDct = 0;
    param->bAnalysisType = 0;
    param->bSingleSeiNal = 0;

    /* SEI messages */
    param->preferredTransferCharacteristics = -1;
    param->pictureStructure = -1;
    param->bEmitCLL = 1;

    param->bEnableFrameDuplication = 0;
    param->dupThreshold = 70;

    /* SVT Hevc Encoder specific params */
    param->bEnableSvtHevc = 0;
    param->svtHevcParam = NULL;

    /* MCSTF */
    param->bEnableTemporalFilter = 0;
    param->temporalFilterStrength = 0.95;

#ifdef SVT_HEVC
    param->svtHevcParam = svtParam;
    svt_param_default(param);
#endif
    /* Film grain characteristics model filename */
    param->filmGrain = NULL;
    param->bEnableSBRC = 0;
}

int x265_param_default_preset(x265_param* param, const char* preset, const char* tune)
{
#if EXPORT_C_API
    ::x265_param_default(param);
#else
    X265_NS::x265_param_default(param);
#endif

    if (preset)
    {
        char *end;
        int i = strtol(preset, &end, 10);
        if (*end == 0 && i >= 0 && i < (int)(sizeof(x265_preset_names) / sizeof(*x265_preset_names) - 1))
            preset = x265_preset_names[i];

        if (!strcmp(preset, "ultrafast"))
        {
            param->maxNumMergeCand = 2;
            param->bIntraInBFrames = 0;
            param->lookaheadDepth = 5;
            param->scenecutThreshold = 0; // disable lookahead
            param->maxCUSize = 32;
            param->minCUSize = 16;
            param->bframes = 3;
            param->bFrameAdaptive = 0;
            param->subpelRefine = 0;
            param->searchMethod = X265_DIA_SEARCH;
            param->bEnableSAO = 0;
            param->bEnableSignHiding = 0;
            param->bEnableWeightedPred = 0;
            param->rdLevel = 2;
            param->maxNumReferences = 1;
            param->limitReferences = 0;
            param->rc.aqStrength = 0.0;
            param->rc.aqMode = X265_AQ_NONE;
            param->rc.hevcAq = 0;
            param->rc.qgSize = 32;
            param->bEnableFastIntra = 1;
        }
        else if (!strcmp(preset, "superfast"))
        {
            param->maxNumMergeCand = 2;
            param->bIntraInBFrames = 0;
            param->lookaheadDepth = 10;
            param->maxCUSize = 32;
            param->bframes = 3;
            param->bFrameAdaptive = 0;
            param->subpelRefine = 1;
            param->bEnableWeightedPred = 0;
            param->rdLevel = 2;
            param->maxNumReferences = 1;
            param->limitReferences = 0;
            param->rc.aqStrength = 0.0;
            param->rc.aqMode = X265_AQ_NONE;
            param->rc.hevcAq = 0;
            param->rc.qgSize = 32;
            param->bEnableSAO = 0;
            param->bEnableFastIntra = 1;
        }
        else if (!strcmp(preset, "veryfast"))
        {
            param->maxNumMergeCand = 2;
            param->limitReferences = 3;
            param->bIntraInBFrames = 0;
            param->lookaheadDepth = 15;
            param->bFrameAdaptive = 0;
            param->subpelRefine = 1;
            param->rdLevel = 2;
            param->maxNumReferences = 2;
            param->rc.qgSize = 32;
            param->bEnableFastIntra = 1;
        }
        else if (!strcmp(preset, "faster"))
        {
            param->maxNumMergeCand = 2;
            param->limitReferences = 3;
            param->bIntraInBFrames = 0;
            param->lookaheadDepth = 15;
            param->bFrameAdaptive = 0;
            param->rdLevel = 2;
            param->maxNumReferences = 2;
            param->bEnableFastIntra = 1;
        }
        else if (!strcmp(preset, "fast"))
        {
            param->maxNumMergeCand = 2;
            param->limitReferences = 3;
            param->bEnableEarlySkip = 0;
            param->bIntraInBFrames = 0;
            param->lookaheadDepth = 15;
            param->bFrameAdaptive = 0;
            param->rdLevel = 2;
            param->maxNumReferences = 3;
            param->bEnableFastIntra = 1;
        }
        else if (!strcmp(preset, "medium"))
        {
            /* defaults */
        }
        else if (!strcmp(preset, "slow"))
        {
            param->limitReferences = 3;
            param->bEnableEarlySkip = 0;
            param->bIntraInBFrames = 0;
            param->bEnableRectInter = 1;
            param->lookaheadDepth = 25;
            param->rdLevel = 4;
            param->rdoqLevel = 2;
            param->psyRdoq = 1.0;
            param->subpelRefine = 3;
            param->searchMethod = X265_STAR_SEARCH;
            param->maxNumReferences = 4;
            param->limitModes = 1;
            param->lookaheadSlices = 4; // limit parallelism as already enough work exists
        }
        else if (!strcmp(preset, "slower"))
        {
            param->bEnableEarlySkip = 0;
            param->bEnableWeightedBiPred = 1;
            param->bEnableAMP = 1;
            param->bEnableRectInter = 1;
            param->lookaheadDepth = 40;
            param->bframes = 8;
            param->tuQTMaxInterDepth = 3;
            param->tuQTMaxIntraDepth = 3;
            param->rdLevel = 6;
            param->rdoqLevel = 2;
            param->psyRdoq = 1.0;
            param->subpelRefine = 4;
            param->maxNumMergeCand = 4;
            param->searchMethod = X265_STAR_SEARCH;
            param->maxNumReferences = 5;
            param->limitModes = 1;
            param->lookaheadSlices = 0; // disabled for best quality
            param->limitTU = 4;
        }
        else if (!strcmp(preset, "veryslow"))
        {
            param->bEnableEarlySkip = 0;
            param->bEnableWeightedBiPred = 1;
            param->bEnableAMP = 1;
            param->bEnableRectInter = 1;
            param->lookaheadDepth = 40;
            param->bframes = 8;
            param->tuQTMaxInterDepth = 3;
            param->tuQTMaxIntraDepth = 3;
            param->rdLevel = 6;
            param->rdoqLevel = 2;
            param->psyRdoq = 1.0;
            param->subpelRefine = 4;
            param->maxNumMergeCand = 5;
            param->searchMethod = X265_STAR_SEARCH;
            param->maxNumReferences = 5;
            param->limitReferences = 0;
            param->limitModes = 0;
            param->lookaheadSlices = 0; // disabled for best quality
            param->limitTU = 0;
        }
        else if (!strcmp(preset, "placebo"))
        {
            param->bEnableEarlySkip = 0;
            param->bEnableWeightedBiPred = 1;
            param->bEnableAMP = 1;
            param->bEnableRectInter = 1;
            param->lookaheadDepth = 60;
            param->searchRange = 92;
            param->bframes = 8;
            param->tuQTMaxInterDepth = 4;
            param->tuQTMaxIntraDepth = 4;
            param->rdLevel = 6;
            param->rdoqLevel = 2;
            param->psyRdoq = 1.0;
            param->subpelRefine = 5;
            param->maxNumMergeCand = 5;
            param->searchMethod = X265_STAR_SEARCH;
            param->bEnableTransformSkip = 1;
            param->recursionSkipMode = 0;
            param->maxNumReferences = 5;
            param->limitReferences = 0;
            param->lookaheadSlices = 0; // disabled for best quality
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
            param->psyRd = 0.0;
            param->psyRdoq = 0.0;
        }
        else if (!strcmp(tune, "ssim"))
        {
            param->rc.aqMode = X265_AQ_AUTO_VARIANCE;
            param->psyRd = 0.0;
            param->psyRdoq = 0.0;
        }
        else if (!strcmp(tune, "fastdecode") ||
                 !strcmp(tune, "fast-decode"))
        {
            param->bEnableLoopFilter = 0;
            param->bEnableSAO = 0;
            param->bEnableWeightedPred = 0;
            param->bEnableWeightedBiPred = 0;
            param->bIntraInBFrames = 0;
        }
        else if (!strcmp(tune, "zerolatency") ||
                 !strcmp(tune, "zero-latency"))
        {
            param->bFrameAdaptive = 0;
            param->bframes = 0;
            param->lookaheadDepth = 0;
            param->scenecutThreshold = 0;
            param->bHistBasedSceneCut = 0;
            param->rc.cuTree = 0;
            param->frameNumThreads = 1;
        }
        else if (!strcmp(tune, "grain"))
        {
            param->rc.ipFactor = 1.1;
            param->rc.pbFactor = 1.0;
            param->rc.cuTree = 0;
            param->rc.aqMode = 0;
            param->rc.hevcAq = 0;
            param->rc.qpStep = 1;
            param->rc.bEnableGrain = 1;
            param->recursionSkipMode = 0;
            param->psyRd = 4.0;
            param->psyRdoq = 10.0;
            param->bEnableSAO = 0;
            param->rc.bEnableConstVbv = 1;
        }
        else if (!strcmp(tune, "animation"))
        {
            param->bframes = (param->bframes + 2) >= param->lookaheadDepth? param->bframes : param->bframes + 2;
            param->psyRd = 0.4;
            param->rc.aqStrength = 0.4;
            param->deblockingFilterBetaOffset = 1;
            param->deblockingFilterTCOffset = 1;
        }
        else if (!strcmp(tune, "vmaf"))  /*Adding vmaf for x265 + SVT-HEVC integration support*/
        {
            /*vmaf is under development, currently x265 won't support vmaf*/
        }
        else
            return -1;
    }

#ifdef SVT_HEVC
    if (svt_set_preset(param, preset))
        return -1;
#endif

    return 0;
}

static int x265_atobool(const char* str, bool& bError)
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

static int parseName(const char* arg, const char* const* names, bool& bError)
{
    for (int i = 0; names[i]; i++)
        if (!strcmp(arg, names[i]))
            return i;

    return x265_atoi(arg, bError);
}
/* internal versions of string-to-int with additional error checking */
#undef atoi
#undef atof
#define atoi(str) x265_atoi(str, bError)
#define atof(str) x265_atof(str, bError)
#define atobool(str) (x265_atobool(str, bError))

int x265_scenecut_aware_qp_param_parse(x265_param* p, const char* name, const char* value)
{
    bool bError = false;
    char nameBuf[64];
    if (!name)
        return X265_PARAM_BAD_NAME;
    // skip -- prefix if provided
    if (name[0] == '-' && name[1] == '-')
        name += 2;
    // s/_/-/g
    if (strlen(name) + 1 < sizeof(nameBuf) && strchr(name, '_'))
    {
        char *c;
        strcpy(nameBuf, name);
        while ((c = strchr(nameBuf, '_')) != 0)
            *c = '-';
        name = nameBuf;
    }
    if (!value)
        value = "true";
    else if (value[0] == '=')
        value++;
#define OPT(STR) else if (!strcmp(name, STR))
    if (0);
    OPT("scenecut-aware-qp") p->bEnableSceneCutAwareQp = x265_atoi(value, bError);
    OPT("masking-strength") bError = parseMaskingStrength(p, value);
    else
        return X265_PARAM_BAD_NAME;
#undef OPT
    return bError ? X265_PARAM_BAD_VALUE : 0;
}


/* internal versions of string-to-int with additional error checking */
#undef atoi
#undef atof
#define atoi(str) x265_atoi(str, bError)
#define atof(str) x265_atof(str, bError)
#define atobool(str) (x265_atobool(str, bError))

int x265_zone_param_parse(x265_param* p, const char* name, const char* value)
{
    bool bError = false;
    char nameBuf[64];

    if (!name)
        return X265_PARAM_BAD_NAME;

    // skip -- prefix if provided
    if (name[0] == '-' && name[1] == '-')
        name += 2;

    // s/_/-/g
    if (strlen(name) + 1 < sizeof(nameBuf) && strchr(name, '_'))
    {
        char *c;
        strcpy(nameBuf, name);
        while ((c = strchr(nameBuf, '_')) != 0)
            *c = '-';

        name = nameBuf;
    }

    if (!strncmp(name, "no-", 3))
    {
        name += 3;
        value = !value || x265_atobool(value, bError) ? "false" : "true";
    }
    else if (!strncmp(name, "no", 2))
    {
        name += 2;
        value = !value || x265_atobool(value, bError) ? "false" : "true";
    }
    else if (!value)
        value = "true";
    else if (value[0] == '=')
        value++;

#define OPT(STR) else if (!strcmp(name, STR))
#define OPT2(STR1, STR2) else if (!strcmp(name, STR1) || !strcmp(name, STR2))

    if (0);
    OPT("ref") p->maxNumReferences = atoi(value);
    OPT("fast-intra") p->bEnableFastIntra = atobool(value);
    OPT("early-skip") p->bEnableEarlySkip = atobool(value);
    OPT("rskip") p->recursionSkipMode = atoi(value);
    OPT("rskip-edge-threshold") p->edgeVarThreshold = atoi(value)/100.0f;
    OPT("me") p->searchMethod = parseName(value, x265_motion_est_names, bError);
    OPT("subme") p->subpelRefine = atoi(value);
    OPT("merange") p->searchRange = atoi(value);
    OPT("rect") p->bEnableRectInter = atobool(value);
    OPT("amp") p->bEnableAMP = atobool(value);
    OPT("max-merge") p->maxNumMergeCand = (uint32_t)atoi(value);
    OPT("rd") p->rdLevel = atoi(value);
    OPT("radl") p->radl = atoi(value);
    OPT2("rdoq", "rdoq-level")
    {
        int bval = atobool(value);
        if (bError || bval)
        {
            bError = false;
            p->rdoqLevel = atoi(value);
        }
        else
            p->rdoqLevel = 0;
    }
    OPT("b-intra") p->bIntraInBFrames = atobool(value);
    OPT("scaling-list") p->scalingLists = strdup(value);
    OPT("crf")
    {
        p->rc.rfConstant = atof(value);
        p->rc.rateControlMode = X265_RC_CRF;
    }
    OPT("qp")
    {
        p->rc.qp = atoi(value);
        p->rc.rateControlMode = X265_RC_CQP;
    }
    OPT("bitrate")
    {
        p->rc.bitrate = atoi(value);
        p->rc.rateControlMode = X265_RC_ABR;
    }
    OPT("aq-mode") p->rc.aqMode = atoi(value);
    OPT("aq-strength") p->rc.aqStrength = atof(value);
    OPT("nr-intra") p->noiseReductionIntra = atoi(value);
    OPT("nr-inter") p->noiseReductionInter = atoi(value);
    OPT("limit-modes") p->limitModes = atobool(value);
    OPT("splitrd-skip") p->bEnableSplitRdSkip = atobool(value);
    OPT("cu-lossless") p->bCULossless = atobool(value);
    OPT("rd-refine") p->bEnableRdRefine = atobool(value);
    OPT("limit-tu") p->limitTU = atoi(value);
    OPT("tskip") p->bEnableTransformSkip = atobool(value);
    OPT("tskip-fast") p->bEnableTSkipFast = atobool(value);
    OPT("rdpenalty") p->rdPenalty = atoi(value);
    OPT("dynamic-rd") p->dynamicRd = atof(value);
    else
        return X265_PARAM_BAD_NAME;

#undef OPT
#undef OPT2

    return bError ? X265_PARAM_BAD_VALUE : 0;
}

#undef atobool
#undef atoi
#undef atof

/* internal versions of string-to-int with additional error checking */
#undef atoi
#undef atof
#define atoi(str) x265_atoi(str, bError)
#define atof(str) x265_atof(str, bError)
#define atobool(str) (bNameWasBool = true, x265_atobool(str, bError))

int x265_param_parse(x265_param* p, const char* name, const char* value)
{
    bool bError = false;
    bool bNameWasBool = false;
    bool bValueWasNull = !value;
    bool bExtraParams = false;
    char nameBuf[64];
    static int count;

    if (!name)
        return X265_PARAM_BAD_NAME;

    count++;
    // skip -- prefix if provided
    if (name[0] == '-' && name[1] == '-')
        name += 2;

    // s/_/-/g
    if (strlen(name) + 1 < sizeof(nameBuf) && strchr(name, '_'))
    {
        char *c;
        strcpy(nameBuf, name);
        while ((c = strchr(nameBuf, '_')) != 0)
            *c = '-';

        name = nameBuf;
    }

    if (!strncmp(name, "no-", 3))
    {
        name += 3;
        value = !value || x265_atobool(value, bError) ? "false" : "true";
    }
    else if (!strncmp(name, "no", 2))
    {
        name += 2;
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
#define OPT2(STR1, STR2) else if (!strcmp(name, STR1) || !strcmp(name, STR2))

#ifdef SVT_HEVC
    if (p->bEnableSvtHevc)
    {
        if(svt_param_parse(p, name, value))
        {
            x265_log(p, X265_LOG_ERROR, "Error while parsing params \n");
            bError = true;
        }
        return bError ? X265_PARAM_BAD_VALUE : 0;
    }
#endif

    if (0) ;
    OPT("asm")
    {
#if X265_ARCH_X86
        if (!strcasecmp(value, "avx512"))
        {
            p->cpuid = X265_NS::cpu_detect(true);
            if (!(p->cpuid & X265_CPU_AVX512))
                x265_log(p, X265_LOG_WARNING, "AVX512 is not supported\n");
        }
        else
        {
            if (bValueWasNull)
                p->cpuid = atobool(value);
            else
                p->cpuid = parseCpuName(value, bError, false);
        }
#else
        if (bValueWasNull)
            p->cpuid = atobool(value);
        else
            p->cpuid = parseCpuName(value, bError, false);
#endif
    }
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
    OPT("frame-threads") p->frameNumThreads = atoi(value);
    OPT("pmode") p->bDistributeModeAnalysis = atobool(value);
    OPT("pme") p->bDistributeMotionEstimation = atobool(value);
    OPT2("level-idc", "level")
    {
        /* allow "5.1" or "51", both converted to integer 51 */
        /* if level-idc specifies an obviously wrong value in either float or int, 
        throw error consistently. Stronger level checking will be done in encoder_open() */
        if (atof(value) < 10)
            p->levelIdc = (int)(10 * atof(value) + .5);
        else if (atoi(value) < 100)
            p->levelIdc = atoi(value);
        else 
            bError = true;
    }
    OPT("high-tier") p->bHighTier = atobool(value);
    OPT("allow-non-conformance") p->bAllowNonConformance = atobool(value);
    OPT2("log-level", "log")
    {
        p->logLevel = atoi(value);
        if (bError)
        {
            bError = false;
            p->logLevel = parseName(value, logLevelNames, bError) - 1;
        }
    }
    OPT("cu-stats") p->bLogCuStats = atobool(value);
    OPT("total-frames") p->totalFrames = atoi(value);
    OPT("annexb") p->bAnnexB = atobool(value);
    OPT("repeat-headers") p->bRepeatHeaders = atobool(value);
    OPT("wpp") p->bEnableWavefront = atobool(value);
    OPT("ctu") p->maxCUSize = (uint32_t)atoi(value);
    OPT("min-cu-size") p->minCUSize = (uint32_t)atoi(value);
    OPT("tu-intra-depth") p->tuQTMaxIntraDepth = (uint32_t)atoi(value);
    OPT("tu-inter-depth") p->tuQTMaxInterDepth = (uint32_t)atoi(value);
    OPT("max-tu-size") p->maxTUSize = (uint32_t)atoi(value);
    OPT("subme") p->subpelRefine = atoi(value);
    OPT("merange") p->searchRange = atoi(value);
    OPT("rect") p->bEnableRectInter = atobool(value);
    OPT("amp") p->bEnableAMP = atobool(value);
    OPT("max-merge") p->maxNumMergeCand = (uint32_t)atoi(value);
    OPT("temporal-mvp") p->bEnableTemporalMvp = atobool(value);
    OPT("early-skip") p->bEnableEarlySkip = atobool(value);
    OPT("rskip") p->recursionSkipMode = atoi(value);
    OPT("rdpenalty") p->rdPenalty = atoi(value);
    OPT("tskip") p->bEnableTransformSkip = atobool(value);
    OPT("no-tskip-fast") p->bEnableTSkipFast = atobool(value);
    OPT("tskip-fast") p->bEnableTSkipFast = atobool(value);
    OPT("strong-intra-smoothing") p->bEnableStrongIntraSmoothing = atobool(value);
    OPT("lossless") p->bLossless = atobool(value);
    OPT("cu-lossless") p->bCULossless = atobool(value);
    OPT2("constrained-intra", "cip") p->bEnableConstrainedIntra = atobool(value);
    OPT("fast-intra") p->bEnableFastIntra = atobool(value);
    OPT("open-gop") p->bOpenGOP = atobool(value);
    OPT("intra-refresh") p->bIntraRefresh = atobool(value);
    OPT("lookahead-slices") p->lookaheadSlices = atoi(value);
    OPT("scenecut")
    {
       p->scenecutThreshold = atobool(value);
       if (bError || p->scenecutThreshold)
       {
           bError = false;
           p->scenecutThreshold = atoi(value);
       }
    }
    OPT("temporal-layers") p->bEnableTemporalSubLayers = atoi(value);
    OPT("keyint") p->keyframeMax = atoi(value);
    OPT("min-keyint") p->keyframeMin = atoi(value);
    OPT("rc-lookahead") p->lookaheadDepth = atoi(value);
    OPT("bframes") p->bframes = atoi(value);
    OPT("bframe-bias") p->bFrameBias = atoi(value);
    OPT("b-adapt")
    {
        p->bFrameAdaptive = atobool(value);
        if (bError || p->bFrameAdaptive)
        {
            bError = false;
            p->bFrameAdaptive = atoi(value);
        }
    }
    OPT("interlace")
    {
        p->interlaceMode = atobool(value);
        if (bError || p->interlaceMode)
        {
            bError = false;
            p->interlaceMode = parseName(value, x265_interlace_names, bError);
        }
    }
    OPT("ref") p->maxNumReferences = atoi(value);
    OPT("limit-refs") p->limitReferences = atoi(value);
    OPT("limit-modes") p->limitModes = atobool(value);
    OPT("weightp") p->bEnableWeightedPred = atobool(value);
    OPT("weightb") p->bEnableWeightedBiPred = atobool(value);
    OPT("cbqpoffs") p->cbQpOffset = atoi(value);
    OPT("crqpoffs") p->crQpOffset = atoi(value);
    OPT("rd") p->rdLevel = atoi(value);
    OPT2("rdoq", "rdoq-level")
    {
        int bval = atobool(value);
        if (bError || bval)
        {
            bError = false;
            p->rdoqLevel = atoi(value);
        }
        else
            p->rdoqLevel = 0;
    }
    OPT("psy-rd")
    {
        int bval = atobool(value);
        if (bError || bval)
        {
            bError = false;
            p->psyRd = atof(value);
        }
        else
            p->psyRd = 0.0;
    }
    OPT("psy-rdoq")
    {
        int bval = atobool(value);
        if (bError || bval)
        {
            bError = false;
            p->psyRdoq = atof(value);
        }
        else
            p->psyRdoq = 0.0;
    }
    OPT("rd-refine") p->bEnableRdRefine = atobool(value);
    OPT("signhide") p->bEnableSignHiding = atobool(value);
    OPT("b-intra") p->bIntraInBFrames = atobool(value);
    OPT("lft") p->bEnableLoopFilter = atobool(value); /* DEPRECATED */
    OPT("deblock")
    {
        if (2 == sscanf(value, "%d:%d", &p->deblockingFilterTCOffset, &p->deblockingFilterBetaOffset) ||
            2 == sscanf(value, "%d,%d", &p->deblockingFilterTCOffset, &p->deblockingFilterBetaOffset))
        {
            p->bEnableLoopFilter = true;
        }
        else if (sscanf(value, "%d", &p->deblockingFilterTCOffset))
        {
            p->bEnableLoopFilter = 1;
            p->deblockingFilterBetaOffset = p->deblockingFilterTCOffset;
        }
        else
            p->bEnableLoopFilter = atobool(value);
    }
    OPT("sao") p->bEnableSAO = atobool(value);
    OPT("sao-non-deblock") p->bSaoNonDeblocked = atobool(value);
    OPT("ssim") p->bEnableSsim = atobool(value);
    OPT("psnr") p->bEnablePsnr = atobool(value);
    OPT("hash") p->decodedPictureHashSEI = atoi(value);
    OPT("aud") p->bEnableAccessUnitDelimiters = atobool(value);
    OPT("info") p->bEmitInfoSEI = atobool(value);
    OPT("b-pyramid") p->bBPyramid = atobool(value);
    OPT("hrd") p->bEmitHRDSEI = atobool(value);
    OPT2("ipratio", "ip-factor") p->rc.ipFactor = atof(value);
    OPT2("pbratio", "pb-factor") p->rc.pbFactor = atof(value);
    OPT("qcomp") p->rc.qCompress = atof(value);
    OPT("qpstep") p->rc.qpStep = atoi(value);
    OPT("cplxblur") p->rc.complexityBlur = atof(value);
    OPT("qblur") p->rc.qblur = atof(value);
    OPT("aq-mode") p->rc.aqMode = atoi(value);
    OPT("aq-strength") p->rc.aqStrength = atof(value);
    OPT("vbv-maxrate") p->rc.vbvMaxBitrate = atoi(value);
    OPT("vbv-bufsize") p->rc.vbvBufferSize = atoi(value);
    OPT("vbv-init")    p->rc.vbvBufferInit = atof(value);
    OPT("crf-max")     p->rc.rfConstantMax = atof(value);
    OPT("crf-min")     p->rc.rfConstantMin = atof(value);
    OPT("qpmax")       p->rc.qpMax = atoi(value);
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
    OPT("rc-grain") p->rc.bEnableGrain = atobool(value);
    OPT("zones")
    {
        p->rc.zoneCount = 1;
        const char* c;

        for (c = value; *c; c++)
            p->rc.zoneCount += (*c == '/');

        p->rc.zones = X265_MALLOC(x265_zone, p->rc.zoneCount);
        c = value;
        for (int i = 0; i < p->rc.zoneCount; i++ )
        {
            int len;
            if (3 == sscanf(c, "%d,%d,q=%d%n", &p->rc.zones[i].startFrame, &p->rc.zones[i].endFrame, &p->rc.zones[i].qp, &len))
                p->rc.zones[i].bForceQp = 1;
            else if (3 == sscanf(c, "%d,%d,b=%f%n", &p->rc.zones[i].startFrame, &p->rc.zones[i].endFrame, &p->rc.zones[i].bitrateFactor, &len))
                p->rc.zones[i].bForceQp = 0;
            else
            {
                bError = true;
                break;
            }
            c += len + 1;
        }
    }
    OPT("input-res") bError |= sscanf(value, "%dx%d", &p->sourceWidth, &p->sourceHeight) != 2;
    OPT("input-csp") p->internalCsp = parseName(value, x265_source_csp_names, bError);
    OPT("me")        p->searchMethod = parseName(value, x265_motion_est_names, bError);
    OPT("cutree")    p->rc.cuTree = atobool(value);
    OPT("slow-firstpass") p->rc.bEnableSlowFirstPass = atobool(value);
    OPT("strict-cbr")
    {
        p->rc.bStrictCbr = atobool(value);
        p->rc.pbFactor = 1.0;
    }
    OPT("analysis-reuse-mode") p->analysisReuseMode = parseName(value, x265_analysis_names, bError); /*DEPRECATED*/
    OPT("sar")
    {
        p->vui.aspectRatioIdc = parseName(value, x265_sar_names, bError);
        if (bError)
        {
            p->vui.aspectRatioIdc = X265_EXTENDED_SAR;
            bError = sscanf(value, "%d:%d", &p->vui.sarWidth, &p->vui.sarHeight) != 2;
        }
    }
    OPT("overscan")
    {
        if (!strcmp(value, "show"))
            p->vui.bEnableOverscanInfoPresentFlag = 1;
        else if (!strcmp(value, "crop"))
        {
            p->vui.bEnableOverscanInfoPresentFlag = 1;
            p->vui.bEnableOverscanAppropriateFlag = 1;
        }
        else if (!strcmp(value, "unknown"))
            p->vui.bEnableOverscanInfoPresentFlag = 0;
        else
            bError = true;
    }
    OPT("videoformat")
    {
        p->vui.bEnableVideoSignalTypePresentFlag = 1;
        p->vui.videoFormat = parseName(value, x265_video_format_names, bError);
    }
    OPT("range")
    {
        p->vui.bEnableVideoSignalTypePresentFlag = 1;
        p->vui.bEnableVideoFullRangeFlag = parseName(value, x265_fullrange_names, bError);
    }
    OPT("colorprim")
    {
        p->vui.bEnableVideoSignalTypePresentFlag = 1;
        p->vui.bEnableColorDescriptionPresentFlag = 1;
        p->vui.colorPrimaries = parseName(value, x265_colorprim_names, bError);
    }
    OPT("transfer")
    {
        p->vui.bEnableVideoSignalTypePresentFlag = 1;
        p->vui.bEnableColorDescriptionPresentFlag = 1;
        p->vui.transferCharacteristics = parseName(value, x265_transfer_names, bError);
    }
    OPT("colormatrix")
    {
        p->vui.bEnableVideoSignalTypePresentFlag = 1;
        p->vui.bEnableColorDescriptionPresentFlag = 1;
        p->vui.matrixCoeffs = parseName(value, x265_colmatrix_names, bError);
    }
    OPT("chromaloc")
    {
        p->vui.bEnableChromaLocInfoPresentFlag = 1;
        p->vui.chromaSampleLocTypeTopField = atoi(value);
        p->vui.chromaSampleLocTypeBottomField = p->vui.chromaSampleLocTypeTopField;
    }
    OPT2("display-window", "crop-rect")
    {
        p->vui.bEnableDefaultDisplayWindowFlag = 1;
        bError |= sscanf(value, "%d,%d,%d,%d",
                         &p->vui.defDispWinLeftOffset,
                         &p->vui.defDispWinTopOffset,
                         &p->vui.defDispWinRightOffset,
                         &p->vui.defDispWinBottomOffset) != 4;
    }
    OPT("nr-intra") p->noiseReductionIntra = atoi(value);
    OPT("nr-inter") p->noiseReductionInter = atoi(value);
    OPT("pass")
    {
        int pass = x265_clip3(0, 3, atoi(value));
        p->rc.bStatWrite = pass & 1;
        p->rc.bStatRead = pass & 2;
        p->rc.dataShareMode = X265_SHARE_MODE_FILE;
    }
    OPT("stats") p->rc.statFileName = strdup(value);
    OPT("scaling-list") p->scalingLists = strdup(value);
    OPT2("pools", "numa-pools") p->numaPools = strdup(value);
    OPT("lambda-file") p->rc.lambdaFileName = strdup(value);
    OPT("analysis-reuse-file") p->analysisReuseFileName = strdup(value);
    OPT("qg-size") p->rc.qgSize = atoi(value);
    OPT("master-display") p->masteringDisplayColorVolume = strdup(value);
    OPT("max-cll") bError |= sscanf(value, "%hu,%hu", &p->maxCLL, &p->maxFALL) != 2;
    OPT("min-luma") p->minLuma = (uint16_t)atoi(value);
    OPT("max-luma") p->maxLuma = (uint16_t)atoi(value);
    OPT("uhd-bd") p->uhdBluray = atobool(value);
    else
        bExtraParams = true;

    // solve "fatal error C1061: compiler limit : blocks nested too deeply"
    if (bExtraParams)
    {
        if (0) ;
        OPT("csv") p->csvfn = strdup(value);
        OPT("csv-log-level") p->csvLogLevel = atoi(value);
        OPT("qpmin") p->rc.qpMin = atoi(value);
        OPT("analyze-src-pics") p->bSourceReferenceEstimation = atobool(value);
        OPT("log2-max-poc-lsb") p->log2MaxPocLsb = atoi(value);
        OPT("vui-timing-info") p->bEmitVUITimingInfo = atobool(value);
        OPT("vui-hrd-info") p->bEmitVUIHRDInfo = atobool(value);
        OPT("slices") p->maxSlices = atoi(value);
        OPT("limit-tu") p->limitTU = atoi(value);
        OPT("opt-qp-pps") p->bOptQpPPS = atobool(value);
        OPT("opt-ref-list-length-pps") p->bOptRefListLengthPPS = atobool(value);
        OPT("multi-pass-opt-rps") p->bMultiPassOptRPS = atobool(value);
        OPT("scenecut-bias") p->scenecutBias = atof(value);
        OPT("hist-scenecut") p->bHistBasedSceneCut = atobool(value);
        OPT("rskip-edge-threshold") p->edgeVarThreshold = atoi(value)/100.0f;
        OPT("lookahead-threads") p->lookaheadThreads = atoi(value);
        OPT("opt-cu-delta-qp") p->bOptCUDeltaQP = atobool(value);
        OPT("multi-pass-opt-analysis") p->analysisMultiPassRefine = atobool(value);
        OPT("multi-pass-opt-distortion") p->analysisMultiPassDistortion = atobool(value);
        OPT("aq-motion") p->bAQMotion = atobool(value);
        OPT("dynamic-rd") p->dynamicRd = atof(value);
        OPT("analysis-reuse-level")
        {
            p->analysisReuseLevel = atoi(value);
            p->analysisSaveReuseLevel = atoi(value);
            p->analysisLoadReuseLevel = atoi(value);
        }
        OPT("analysis-save-reuse-level") p->analysisSaveReuseLevel = atoi(value);
        OPT("analysis-load-reuse-level") p->analysisLoadReuseLevel = atoi(value);
        OPT("ssim-rd")
        {
            int bval = atobool(value);
            if (bError || bval)
            {
                bError = false;
                p->psyRd = 0.0;
                p->bSsimRd = atobool(value);
            }
        }
        OPT("hdr") p->bEmitHDR10SEI = atobool(value);  /*DEPRECATED*/
        OPT("hdr10") p->bEmitHDR10SEI = atobool(value);
        OPT("hdr-opt") p->bHDR10Opt = atobool(value); /*DEPRECATED*/
        OPT("hdr10-opt") p->bHDR10Opt = atobool(value);
        OPT("limit-sao") p->bLimitSAO = atobool(value);
        OPT("dhdr10-info") p->toneMapFile = strdup(value);
        OPT("dhdr10-opt") p->bDhdr10opt = atobool(value);
        OPT("idr-recovery-sei") p->bEmitIDRRecoverySEI = atobool(value);
        OPT("const-vbv") p->rc.bEnableConstVbv = atobool(value);
        OPT("ctu-info") p->bCTUInfo = atoi(value);
        OPT("scale-factor") p->scaleFactor = atoi(value);
        OPT("refine-intra")p->intraRefine = atoi(value);
        OPT("refine-inter")p->interRefine = atoi(value);
        OPT("refine-mv")p->mvRefine = atoi(value);
        OPT("force-flush")p->forceFlush = atoi(value);
        OPT("splitrd-skip") p->bEnableSplitRdSkip = atobool(value);
        OPT("lowpass-dct") p->bLowPassDct = atobool(value);
        OPT("vbv-end") p->vbvBufferEnd = atof(value);
        OPT("vbv-end-fr-adj") p->vbvEndFrameAdjust = atof(value);
        OPT("copy-pic") p->bCopyPicToFrame = atobool(value);
        OPT("refine-analysis-type")
        {
            if (strcmp(strdup(value), "avc") == 0)
            {
                p->bAnalysisType = AVC_INFO;
            }
            else if (strcmp(strdup(value), "hevc") == 0)
            {
                p->bAnalysisType = HEVC_INFO;
            }
            else if (strcmp(strdup(value), "off") == 0)
            {
                p->bAnalysisType = DEFAULT;
            }
            else
            {
                bError = true;
            }
        }
        OPT("gop-lookahead") p->gopLookahead = atoi(value);
        OPT("analysis-save") p->analysisSave = strdup(value);
        OPT("analysis-load") p->analysisLoad = strdup(value);
        OPT("radl") p->radl = atoi(value);
        OPT("max-ausize-factor") p->maxAUSizeFactor = atof(value);
        OPT("dynamic-refine") p->bDynamicRefine = atobool(value);
        OPT("single-sei") p->bSingleSeiNal = atobool(value);
        OPT("atc-sei") p->preferredTransferCharacteristics = atoi(value);
        OPT("pic-struct") p->pictureStructure = atoi(value);
        OPT("chunk-start") p->chunkStart = atoi(value);
        OPT("chunk-end") p->chunkEnd = atoi(value);
        OPT("nalu-file") p->naluFile = strdup(value);
        OPT("dolby-vision-profile")
        {
            if (atof(value) < 10)
                p->dolbyProfile = (int)(10 * atof(value) + .5);
            else if (atoi(value) < 100)
                p->dolbyProfile = atoi(value);
            else
                bError = true;
        }
        OPT("hrd-concat") p->bEnableHRDConcatFlag = atobool(value);
        OPT("refine-ctu-distortion") p->ctuDistortionRefine = atoi(value);
        OPT("hevc-aq") p->rc.hevcAq = atobool(value);
        OPT("qp-adaptation-range") p->rc.qpAdaptationRange = atof(value);
#ifdef SVT_HEVC
        OPT("svt")
        {
            p->bEnableSvtHevc = atobool(value);
            if (count > 1 && p->bEnableSvtHevc)
            {
                x265_log(NULL, X265_LOG_ERROR, "Enable SVT should be the first call to x265_parse_parse \n");
                bError = true;
            }
        }
        OPT("svt-hme") x265_log(p, X265_LOG_WARNING, "Option %s is SVT-HEVC Encoder specific; Disabling it here \n", name);
        OPT("svt-search-width") x265_log(p, X265_LOG_WARNING, "Option %s is SVT-HEVC Encoder specific; Disabling it here \n", name);
        OPT("svt-search-height") x265_log(p, X265_LOG_WARNING, "Option %s is SVT-HEVC Encoder specific; Disabling it here \n", name);
        OPT("svt-compressed-ten-bit-format") x265_log(p, X265_LOG_WARNING, "Option %s is SVT-HEVC Encoder specific; Disabling it here \n", name);
        OPT("svt-speed-control") x265_log(p, X265_LOG_WARNING, "Option %s is SVT-HEVC Encoder specific; Disabling it here \n", name);
        OPT("input-depth") x265_log(p, X265_LOG_WARNING, "Option %s is SVT-HEVC Encoder specific; Disabling it here \n", name);
        OPT("svt-preset-tuner") x265_log(p, X265_LOG_WARNING, "Option %s is SVT-HEVC Encoder specific; Disabling it here \n", name);
        OPT("svt-hierarchical-level") x265_log(p, X265_LOG_WARNING, "Option %s is SVT-HEVC Encoder specific; Disabling it here \n", name);
        OPT("svt-base-layer-switch-mode") x265_log(p, X265_LOG_WARNING, "Option %s is SVT-HEVC Encoder specific; Disabling it here \n", name);
        OPT("svt-pred-struct") x265_log(p, X265_LOG_WARNING, "Option %s is SVT-HEVC Encoder specific; Disabling it here \n", name);
        OPT("svt-fps-in-vps") x265_log(p, X265_LOG_WARNING, "Option %s is SVT-HEVC Encoder specific; Disabling it here \n", name);
#endif
        OPT("selective-sao")
        {
            p->selectiveSAO = atoi(value);
        }
        OPT("fades") p->bEnableFades = atobool(value);
        OPT("scenecut-aware-qp") p->bEnableSceneCutAwareQp = atoi(value);
        OPT("masking-strength") bError |= parseMaskingStrength(p, value);
        OPT("field") p->bField = atobool( value );
        OPT("cll") p->bEmitCLL = atobool(value);
        OPT("frame-dup") p->bEnableFrameDuplication = atobool(value);
        OPT("dup-threshold") p->dupThreshold = atoi(value);
        OPT("hme") p->bEnableHME = atobool(value);
        OPT("hme-search")
        {
            char search[3][5];
            memset(search, '\0', 15 * sizeof(char));
            if(3 == sscanf(value, "%d,%d,%d", &p->hmeSearchMethod[0], &p->hmeSearchMethod[1], &p->hmeSearchMethod[2]) ||
               3 == sscanf(value, "%4[^,],%4[^,],%4[^,]", search[0], search[1], search[2]))
            {
                if(search[0][0])
                    for(int level = 0; level < 3; level++)
                        p->hmeSearchMethod[level] = parseName(search[level], x265_motion_est_names, bError);
            }
            else if (sscanf(value, "%d", &p->hmeSearchMethod[0]) || sscanf(value, "%s", search[0]))
            {
                if (search[0][0]) {
                    p->hmeSearchMethod[0] = parseName(search[0], x265_motion_est_names, bError);
                    p->hmeSearchMethod[1] = p->hmeSearchMethod[2] = p->hmeSearchMethod[0];
                }
            }
            p->bEnableHME = true;
        }
        OPT("hme-range")
        {
            sscanf(value, "%d,%d,%d", &p->hmeRange[0], &p->hmeRange[1], &p->hmeRange[2]);
            p->bEnableHME = true;
        }
        OPT("vbv-live-multi-pass") p->bliveVBV2pass = atobool(value);
        OPT("min-vbv-fullness") p->minVbvFullness = atof(value);
        OPT("max-vbv-fullness") p->maxVbvFullness = atof(value);
        OPT("video-signal-type-preset") p->videoSignalTypePreset = strdup(value);
        OPT("eob") p->bEnableEndOfBitstream = atobool(value);
        OPT("eos") p->bEnableEndOfSequence = atobool(value);
        /* Film grain characterstics model filename */
        OPT("film-grain") p->filmGrain = (char* )value;
        OPT("mcstf") p->bEnableTemporalFilter = atobool(value);
        OPT("sbrc") p->bEnableSBRC = atobool(value);
        else
            return X265_PARAM_BAD_NAME;
    }
#undef OPT
#undef atobool
#undef atoi
#undef atof

    bError |= bValueWasNull && !bNameWasBool;
    return bError ? X265_PARAM_BAD_VALUE : 0;
}

} /* end extern "C" or namespace */

namespace X265_NS {
// internal encoder functions

int x265_atoi(const char* str, bool& bError)
{
    char *end;
    int v = strtol(str, &end, 0);

    if (end == str || *end != '\0')
        bError = true;
    return v;
}

double x265_atof(const char* str, bool& bError)
{
    char *end;
    double v = strtod(str, &end);

    if (end == str || *end != '\0')
        bError = true;
    return v;
}

/* cpu name can be:
 *   auto || true - x265::cpu_detect()
 *   false || no  - disabled
 *   integer bitmap value
 *   comma separated list of SIMD names, eg: SSE4.1,XOP */
int parseCpuName(const char* value, bool& bError, bool bEnableavx512)
{
    if (!value)
    {
        bError = 1;
        return 0;
    }
    int cpu;
    if (isdigit(value[0]))
        cpu = x265_atoi(value, bError);
    else
        cpu = !strcmp(value, "auto") || x265_atobool(value, bError) ? X265_NS::cpu_detect(bEnableavx512) : 0;

    if (bError)
    {
        char *buf = strdup(value);
        char *tok, *saveptr = NULL, *init;
        bError = 0;
        cpu = 0;
        for (init = buf; (tok = strtok_r(init, ",", &saveptr)); init = NULL)
        {
            int i;
            for (i = 0; X265_NS::cpu_names[i].flags && strcasecmp(tok, X265_NS::cpu_names[i].name); i++)
            {
            }

            cpu |= X265_NS::cpu_names[i].flags;
            if (!X265_NS::cpu_names[i].flags)
                bError = 1;
        }

        free(buf);
        if ((cpu & X265_CPU_SSSE3) && !(cpu & X265_CPU_SSE2_IS_SLOW))
            cpu |= X265_CPU_SSE2_IS_FAST;
    }

    return cpu;
}

static const int fixedRatios[][2] =
{
    { 1,  1 },
    { 12, 11 },
    { 10, 11 },
    { 16, 11 },
    { 40, 33 },
    { 24, 11 },
    { 20, 11 },
    { 32, 11 },
    { 80, 33 },
    { 18, 11 },
    { 15, 11 },
    { 64, 33 },
    { 160, 99 },
    { 4, 3 },
    { 3, 2 },
    { 2, 1 },
};

void setParamAspectRatio(x265_param* p, int width, int height)
{
    p->vui.aspectRatioIdc = X265_EXTENDED_SAR;
    p->vui.sarWidth = width;
    p->vui.sarHeight = height;
    for (size_t i = 0; i < sizeof(fixedRatios) / sizeof(fixedRatios[0]); i++)
    {
        if (width == fixedRatios[i][0] && height == fixedRatios[i][1])
        {
            p->vui.aspectRatioIdc = (int)i + 1;
            return;
        }
    }
}

void getParamAspectRatio(x265_param* p, int& width, int& height)
{
    if (!p->vui.aspectRatioIdc)
        width = height = 0;
    else if ((size_t)p->vui.aspectRatioIdc <= sizeof(fixedRatios) / sizeof(fixedRatios[0]))
    {
        width  = fixedRatios[p->vui.aspectRatioIdc - 1][0];
        height = fixedRatios[p->vui.aspectRatioIdc - 1][1];
    }
    else if (p->vui.aspectRatioIdc == X265_EXTENDED_SAR)
    {
        width  = p->vui.sarWidth;
        height = p->vui.sarHeight;
    }
    else
        width = height = 0;
}

static inline int _confirm(x265_param* param, bool bflag, const char* message)
{
    if (!bflag)
        return 0;

    x265_log(param, X265_LOG_ERROR, "%s\n", message);
    return 1;
}

int x265_check_params(x265_param* param)
{
#define CHECK(expr, msg) check_failed |= _confirm(param, expr, msg)
    int check_failed = 0; /* abort if there is a fatal configuration problem */
    CHECK(param->uhdBluray == 1 && (X265_DEPTH != 10 || param->internalCsp != 1 || param->interlaceMode != 0),
        "uhd-bd: bit depth, chroma subsample, source picture type must be 10, 4:2:0, progressive");
    CHECK(param->maxCUSize != 64 && param->maxCUSize != 32 && param->maxCUSize != 16,
          "max cu size must be 16, 32, or 64");
    if (check_failed == 1)
        return check_failed;

    uint32_t maxLog2CUSize = (uint32_t)g_log2Size[param->maxCUSize];
    uint32_t tuQTMaxLog2Size = X265_MIN(maxLog2CUSize, 5);
    uint32_t tuQTMinLog2Size = 2; //log2(4)

    CHECK((param->maxSlices > 1) && !param->bEnableWavefront,
        "Multiple-Slices mode must be enable Wavefront Parallel Processing (--wpp)");
    CHECK(param->internalBitDepth != X265_DEPTH,
          "internalBitDepth must match compiled bit depth");
    CHECK(param->minCUSize != 32 && param->minCUSize != 16 && param->minCUSize != 8,
          "minimim CU size must be 8, 16 or 32");
    CHECK(param->minCUSize > param->maxCUSize,
          "min CU size must be less than or equal to max CU size");
    CHECK(param->rc.qp < -6 * (param->internalBitDepth - 8) || param->rc.qp > QP_MAX_SPEC,
          "QP exceeds supported range (-QpBDOffsety to 51)");
    CHECK(param->fpsNum == 0 || param->fpsDenom == 0,
          "Frame rate numerator and denominator must be specified");
    CHECK(param->interlaceMode < 0 || param->interlaceMode > 2,
          "Interlace mode must be 0 (progressive) 1 (top-field first) or 2 (bottom field first)");
    CHECK(param->searchMethod < 0 || param->searchMethod > X265_FULL_SEARCH,
          "Search method is not supported value (0:DIA 1:HEX 2:UMH 3:HM 4:SEA 5:FULL)");
    CHECK(param->searchRange < 0,
          "Search Range must be more than 0");
    CHECK(param->searchRange >= 32768,
          "Search Range must be less than 32768");
    CHECK(param->subpelRefine > X265_MAX_SUBPEL_LEVEL,
          "subme must be less than or equal to X265_MAX_SUBPEL_LEVEL (7)");
    CHECK(param->subpelRefine < 0,
          "subme must be greater than or equal to 0");
    CHECK(param->limitReferences > 3,
          "limitReferences must be 0, 1, 2 or 3");
    CHECK(param->limitModes > 1,
          "limitRectAmp must be 0, 1");
    CHECK(param->frameNumThreads < 0 || param->frameNumThreads > X265_MAX_FRAME_THREADS,
          "frameNumThreads (--frame-threads) must be [0 .. X265_MAX_FRAME_THREADS)");
    CHECK(param->cbQpOffset < -12, "Min. Chroma Cb QP Offset is -12");
    CHECK(param->cbQpOffset >  12, "Max. Chroma Cb QP Offset is  12");
    CHECK(param->crQpOffset < -12, "Min. Chroma Cr QP Offset is -12");
    CHECK(param->crQpOffset >  12, "Max. Chroma Cr QP Offset is  12");

    CHECK(tuQTMaxLog2Size > maxLog2CUSize,
          "QuadtreeTULog2MaxSize must be log2(maxCUSize) or smaller.");

    CHECK(param->tuQTMaxInterDepth < 1 || param->tuQTMaxInterDepth > 4,
          "QuadtreeTUMaxDepthInter must be greater than 0 and less than 5");
    CHECK(maxLog2CUSize < tuQTMinLog2Size + param->tuQTMaxInterDepth - 1,
          "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1");
    CHECK(param->tuQTMaxIntraDepth < 1 || param->tuQTMaxIntraDepth > 4,
          "QuadtreeTUMaxDepthIntra must be greater 0 and less than 5");
    CHECK(maxLog2CUSize < tuQTMinLog2Size + param->tuQTMaxIntraDepth - 1,
          "QuadtreeTUMaxDepthInter must be less than or equal to the difference between log2(maxCUSize) and QuadtreeTULog2MinSize plus 1");
    CHECK((param->maxTUSize != 32 && param->maxTUSize != 16 && param->maxTUSize != 8 && param->maxTUSize != 4),
          "max TU size must be 4, 8, 16, or 32");
    CHECK(param->limitTU > 4, "Invalid limit-tu option, limit-TU must be between 0 and 4");
    CHECK(param->maxNumMergeCand < 1, "MaxNumMergeCand must be 1 or greater.");
    CHECK(param->maxNumMergeCand > 5, "MaxNumMergeCand must be 5 or smaller.");

    CHECK(param->maxNumReferences < 1, "maxNumReferences must be 1 or greater.");
    CHECK(param->maxNumReferences > MAX_NUM_REF, "maxNumReferences must be 16 or smaller.");

    CHECK(param->sourceWidth < (int)param->maxCUSize || param->sourceHeight < (int)param->maxCUSize,
          "Picture size must be at least one CTU");
    CHECK(param->internalCsp < X265_CSP_I400 || X265_CSP_I444 < param->internalCsp,
          "chroma subsampling must be i400 (4:0:0 monochrome), i420 (4:2:0 default), i422 (4:2:0), i444 (4:4:4)");
    CHECK(param->sourceWidth & !!CHROMA_H_SHIFT(param->internalCsp),
          "Picture width must be an integer multiple of the specified chroma subsampling");
    CHECK(param->sourceHeight & !!CHROMA_V_SHIFT(param->internalCsp),
          "Picture height must be an integer multiple of the specified chroma subsampling");

    CHECK(param->rc.rateControlMode > X265_RC_CRF || param->rc.rateControlMode < X265_RC_ABR,
          "Rate control mode is out of range");
    CHECK(param->rdLevel < 1 || param->rdLevel > 6,
          "RD Level is out of range");
    CHECK(param->rdoqLevel < 0 || param->rdoqLevel > 2,
          "RDOQ Level is out of range");
    CHECK(param->dynamicRd < 0 || param->dynamicRd > x265_ADAPT_RD_STRENGTH,
          "Dynamic RD strength must be between 0 and 4");
    CHECK(param->recursionSkipMode > 2 || param->recursionSkipMode < 0,
          "Invalid Recursion skip mode. Valid modes 0,1,2");
    if (param->recursionSkipMode == EDGE_BASED_RSKIP)
    {
        CHECK(param->edgeVarThreshold < 0.0f || param->edgeVarThreshold > 1.0f,
              "Minimum edge density percentage for a CU should be an integer between 0 to 100");
    }
    CHECK(param->bframes && param->bframes >= param->lookaheadDepth && !param->rc.bStatRead,
          "Lookahead depth must be greater than the max consecutive bframe count");
    CHECK(param->bframes < 0,
          "bframe count should be greater than zero");
    CHECK(param->bframes > X265_BFRAME_MAX,
          "max consecutive bframe count must be 16 or smaller");
    CHECK(param->lookaheadDepth > X265_LOOKAHEAD_MAX,
          "Lookahead depth must be less than 256");
    CHECK(param->lookaheadSlices > 16 || param->lookaheadSlices < 0,
          "Lookahead slices must between 0 and 16");
    CHECK(param->rc.aqMode < X265_AQ_NONE || X265_AQ_EDGE < param->rc.aqMode,
          "Aq-Mode is out of range");
    CHECK(param->rc.aqStrength < 0 || param->rc.aqStrength > 3,
          "Aq-Strength is out of range");
    CHECK(param->rc.qpAdaptationRange < 1.0f || param->rc.qpAdaptationRange > 6.0f,
        "qp adaptation range is out of range");
    CHECK(param->deblockingFilterTCOffset < -6 || param->deblockingFilterTCOffset > 6,
          "deblocking filter tC offset must be in the range of -6 to +6");
    CHECK(param->deblockingFilterBetaOffset < -6 || param->deblockingFilterBetaOffset > 6,
          "deblocking filter Beta offset must be in the range of -6 to +6");
    CHECK(param->psyRd < 0 || 5.0 < param->psyRd, "Psy-rd strength must be between 0 and 5.0");
    CHECK(param->psyRdoq < 0 || 50.0 < param->psyRdoq, "Psy-rdoq strength must be between 0 and 50.0");
    CHECK(param->bEnableWavefront < 0, "WaveFrontSynchro cannot be negative");
    CHECK((param->vui.aspectRatioIdc < 0
           || param->vui.aspectRatioIdc > 16)
          && param->vui.aspectRatioIdc != X265_EXTENDED_SAR,
          "Sample Aspect Ratio must be 0-16 or 255");
    CHECK(param->vui.aspectRatioIdc == X265_EXTENDED_SAR && param->vui.sarWidth <= 0,
          "Sample Aspect Ratio width must be greater than 0");
    CHECK(param->vui.aspectRatioIdc == X265_EXTENDED_SAR && param->vui.sarHeight <= 0,
          "Sample Aspect Ratio height must be greater than 0");
    CHECK(param->vui.videoFormat < 0 || param->vui.videoFormat > 5,
          "Video Format must be component,"
          " pal, ntsc, secam, mac or unknown");
    CHECK(param->vui.colorPrimaries < 0
          || param->vui.colorPrimaries > 12
          || param->vui.colorPrimaries == 3,
          "Color Primaries must be unknown, bt709, bt470m,"
          " bt470bg, smpte170m, smpte240m, film, bt2020, smpte-st-428, smpte-rp-431 or smpte-eg-432");
    CHECK(param->vui.transferCharacteristics < 0
          || param->vui.transferCharacteristics > 18
          || param->vui.transferCharacteristics == 3,
          "Transfer Characteristics must be unknown, bt709, bt470m, bt470bg,"
          " smpte170m, smpte240m, linear, log100, log316, iec61966-2-4, bt1361e,"
          " iec61966-2-1, bt2020-10, bt2020-12, smpte-st-2084, smpte-st-428 or arib-std-b67");
    CHECK(param->vui.matrixCoeffs < 0
          || param->vui.matrixCoeffs > 14
          || param->vui.matrixCoeffs == 3,
          "Matrix Coefficients must be unknown, bt709, fcc, bt470bg, smpte170m,"
          " smpte240m, gbr, ycgco, bt2020nc, bt2020c, smpte-st-2085, chroma-nc, chroma-c or ictcp");
    CHECK(param->vui.chromaSampleLocTypeTopField < 0
          || param->vui.chromaSampleLocTypeTopField > 5,
          "Chroma Sample Location Type Top Field must be 0-5");
    CHECK(param->vui.chromaSampleLocTypeBottomField < 0
          || param->vui.chromaSampleLocTypeBottomField > 5,
          "Chroma Sample Location Type Bottom Field must be 0-5");
    CHECK(param->vui.defDispWinLeftOffset < 0,
          "Default Display Window Left Offset must be 0 or greater");
    CHECK(param->vui.defDispWinRightOffset < 0,
          "Default Display Window Right Offset must be 0 or greater");
    CHECK(param->vui.defDispWinTopOffset < 0,
          "Default Display Window Top Offset must be 0 or greater");
    CHECK(param->vui.defDispWinBottomOffset < 0,
          "Default Display Window Bottom Offset must be 0 or greater");
    CHECK(param->rc.rfConstant < -6 * (param->internalBitDepth - 8) || param->rc.rfConstant > 51,
          "Valid quality based range: -qpBDOffsetY to 51");
    CHECK(param->rc.rfConstantMax < -6 * (param->internalBitDepth - 8) || param->rc.rfConstantMax > 51,
          "Valid quality based range: -qpBDOffsetY to 51");
    CHECK(param->rc.rfConstantMin < -6 * (param->internalBitDepth - 8) || param->rc.rfConstantMin > 51,
          "Valid quality based range: -qpBDOffsetY to 51");
    CHECK(param->bFrameAdaptive < 0 || param->bFrameAdaptive > 2,
          "Valid adaptive b scheduling values 0 - none, 1 - fast, 2 - full");
    CHECK(param->logLevel<-1 || param->logLevel> X265_LOG_FULL,
          "Valid Logging level -1:none 0:error 1:warning 2:info 3:debug 4:full");
    CHECK(param->scenecutThreshold < 0,
          "scenecutThreshold must be greater than 0");
    CHECK(param->scenecutBias < 0 || 100 < param->scenecutBias,
            "scenecut-bias must be between 0 and 100");
    CHECK(param->radl < 0 || param->radl > param->bframes,
          "radl must be between 0 and bframes");
    CHECK(param->rdPenalty < 0 || param->rdPenalty > 2,
          "Valid penalty for 32x32 intra TU in non-I slices. 0:disabled 1:RD-penalty 2:maximum");
    CHECK(param->keyframeMax < -1,
          "Invalid max IDR period in frames. value should be greater than -1");
    CHECK(param->gopLookahead < -1,
          "GOP lookahead must be greater than -1");
    CHECK(param->decodedPictureHashSEI < 0 || param->decodedPictureHashSEI > 3,
          "Invalid hash option. Decoded Picture Hash SEI 0: disabled, 1: MD5, 2: CRC, 3: Checksum");
    CHECK(param->rc.vbvBufferSize < 0,
          "Size of the vbv buffer can not be less than zero");
    CHECK(param->rc.vbvMaxBitrate < 0,
          "Maximum local bit rate can not be less than zero");
    CHECK(param->rc.vbvBufferInit < 0,
          "Valid initial VBV buffer occupancy must be a fraction 0 - 1, or size in kbits");
    CHECK(param->vbvBufferEnd < 0,
        "Valid final VBV buffer emptiness must be a fraction 0 - 1, or size in kbits");
    CHECK(param->vbvEndFrameAdjust < 0,
        "Valid vbv-end-fr-adj must be a fraction 0 - 1");
    CHECK(!param->totalFrames && param->vbvEndFrameAdjust,
        "vbv-end-fr-adj cannot be enabled when total number of frames is unknown");
    CHECK(param->minVbvFullness < 0 && param->minVbvFullness > 100,
        "min-vbv-fullness must be a fraction 0 - 100");
    CHECK(param->maxVbvFullness < 0 && param->maxVbvFullness > 100,
        "max-vbv-fullness must be a fraction 0 - 100");
    CHECK(param->rc.bitrate < 0,
          "Target bitrate can not be less than zero");
    CHECK(param->rc.qCompress < 0.5 || param->rc.qCompress > 1.0,
          "qCompress must be between 0.5 and 1.0");
    if (param->noiseReductionIntra)
        CHECK(0 > param->noiseReductionIntra || param->noiseReductionIntra > 2000, "Valid noise reduction range 0 - 2000");
    if (param->noiseReductionInter)
        CHECK(0 > param->noiseReductionInter || param->noiseReductionInter > 2000, "Valid noise reduction range 0 - 2000");
    CHECK(param->rc.rateControlMode == X265_RC_CQP && param->rc.bStatRead,
          "Constant QP is incompatible with 2pass");
    CHECK(param->rc.bStrictCbr && (param->rc.bitrate <= 0 || param->rc.vbvBufferSize <=0),
          "Strict-cbr cannot be applied without specifying target bitrate or vbv bufsize");
    CHECK(param->analysisSave && (param->analysisSaveReuseLevel < 0 || param->analysisSaveReuseLevel > 10),
        "Invalid analysis save refine level. Value must be between 1 and 10 (inclusive)");
    CHECK(param->analysisLoad && (param->analysisLoadReuseLevel < 0 || param->analysisLoadReuseLevel > 10),
        "Invalid analysis load refine level. Value must be between 1 and 10 (inclusive)");
    CHECK(param->analysisLoad && (param->mvRefine < 1 || param->mvRefine > 3),
        "Invalid mv refinement level. Value must be between 1 and 3 (inclusive)");
    CHECK(param->scaleFactor > 2, "Invalid scale-factor. Supports factor <= 2");
    CHECK(param->rc.qpMax < QP_MIN || param->rc.qpMax > QP_MAX_MAX,
        "qpmax exceeds supported range (0 to 69)");
    CHECK(param->rc.qpMin < QP_MIN || param->rc.qpMin > QP_MAX_MAX,
        "qpmin exceeds supported range (0 to 69)");
    CHECK(param->log2MaxPocLsb < 4 || param->log2MaxPocLsb > 16,
        "Supported range for log2MaxPocLsb is 4 to 16");
    CHECK(param->bCTUInfo < 0 || (param->bCTUInfo != 0 && param->bCTUInfo != 1 && param->bCTUInfo != 2 && param->bCTUInfo != 4 && param->bCTUInfo != 6) || param->bCTUInfo > 6,
        "Supported values for bCTUInfo are 0, 1, 2, 4, 6");
    CHECK(param->interRefine > 3 || param->interRefine < 0,
        "Invalid refine-inter value, refine-inter levels 0 to 3 supported");
    CHECK(param->intraRefine > 4 || param->intraRefine < 0,
        "Invalid refine-intra value, refine-intra levels 0 to 3 supported");
    CHECK(param->ctuDistortionRefine < 0 || param->ctuDistortionRefine > 1,
        "Invalid refine-ctu-distortion value, must be either 0 or 1");
    CHECK(param->maxAUSizeFactor < 0.5 || param->maxAUSizeFactor > 1.0,
        "Supported factor for controlling max AU size is from 0.5 to 1");
    CHECK((param->dolbyProfile != 0) && (param->dolbyProfile != 50) && (param->dolbyProfile != 81) && (param->dolbyProfile != 82) && (param->dolbyProfile != 84),
        "Unsupported Dolby Vision profile, only profile 5, profile 8.1, profile 8.2 and profile 8.4 enabled");
    CHECK(param->dupThreshold < 1 || 99 < param->dupThreshold,
        "Invalid frame-duplication threshold. Value must be between 1 and 99.");
    if (param->dolbyProfile)
    {
        CHECK((param->rc.vbvMaxBitrate <= 0 || param->rc.vbvBufferSize <= 0), "Dolby Vision requires VBV settings to enable HRD.\n");
        CHECK((param->internalBitDepth != 10), "Dolby Vision profile - 5, profile - 8.1, profile - 8.2 and profile - 8.4 are Main10 only\n");
        CHECK((param->internalCsp != X265_CSP_I420), "Dolby Vision profile - 5, profile - 8.1, profile - 8.2 and profile - 8.4 requires YCbCr 4:2:0 color space\n");
        if (param->dolbyProfile == 81)
            CHECK(!(param->masteringDisplayColorVolume), "Dolby Vision profile - 8.1 requires Mastering display color volume information\n");
    }
    if (param->bField && param->interlaceMode)
    {
        CHECK( (param->bFrameAdaptive==0), "Adaptive B-frame decision method should be closed for field feature.\n" );
        // to do
    }
    CHECK(param->selectiveSAO < 0 || param->selectiveSAO > 4,
        "Invalid SAO tune level. Value must be between 0 and 4 (inclusive)");
    if (param->bEnableSceneCutAwareQp)
    {
        if (!param->rc.bStatRead)
        {
            param->bEnableSceneCutAwareQp = 0;
            x265_log(param, X265_LOG_WARNING, "Disabling Scenecut Aware Frame Quantizer Selection since it works only in pass 2\n");
        }
        else
        {
            CHECK(param->bEnableSceneCutAwareQp < 0 || param->bEnableSceneCutAwareQp > 3,
            "Invalid masking direction. Value must be between 0 and 3(inclusive)");
            for (int i = 0; i < 6; i++)
            {
                CHECK(param->fwdScenecutWindow[i] < 0 || param->fwdScenecutWindow[i] > 1000,
                    "Invalid forward scenecut Window duration. Value must be between 0 and 1000(inclusive)");
                CHECK(param->fwdRefQpDelta[i] < 0 || param->fwdRefQpDelta[i] > 20,
                    "Invalid fwdRefQpDelta value. Value must be between 0 and 20 (inclusive)");
                CHECK(param->fwdNonRefQpDelta[i] < 0 || param->fwdNonRefQpDelta[i] > 20,
                    "Invalid fwdNonRefQpDelta value. Value must be between 0 and 20 (inclusive)");

                CHECK(param->bwdScenecutWindow[i] < 0 || param->bwdScenecutWindow[i] > 1000,
                    "Invalid backward scenecut Window duration. Value must be between 0 and 1000(inclusive)");
                CHECK(param->bwdRefQpDelta[i] < -1 || param->bwdRefQpDelta[i] > 20,
                    "Invalid bwdRefQpDelta value. Value must be between 0 and 20 (inclusive)");
                CHECK(param->bwdNonRefQpDelta[i] < -1 || param->bwdNonRefQpDelta[i] > 20,
                    "Invalid bwdNonRefQpDelta value. Value must be between 0 and 20 (inclusive)");
            }
        }
    }
    if (param->bEnableHME)
    {
        for (int level = 0; level < 3; level++)
            CHECK(param->hmeRange[level] < 0 || param->hmeRange[level] >= 32768,
                "Search Range for HME levels must be between 0 and 32768");
    }
#if !X86_64
    CHECK(param->searchMethod == X265_SEA && (param->sourceWidth > 840 || param->sourceHeight > 480),
        "SEA motion search does not support resolutions greater than 480p in 32 bit build");
#endif

    if (param->masteringDisplayColorVolume || param->maxFALL || param->maxCLL)
        param->bEmitHDR10SEI = 1;

    bool isSingleSEI = (param->bRepeatHeaders
                     || param->bEmitHRDSEI
                     || param->bEmitInfoSEI
                     || param->bEmitHDR10SEI
                     || param->bEmitIDRRecoverySEI
                   || !!param->interlaceMode
                     || param->preferredTransferCharacteristics > 1
                     || param->toneMapFile
                     || param->naluFile);

    if (!isSingleSEI && param->bSingleSeiNal)
    {
        param->bSingleSeiNal = 0;
        x265_log(param, X265_LOG_WARNING, "None of the SEI messages are enabled. Disabling Single SEI NAL\n");
    }
    if (param->bEnableTemporalFilter && (param->frameNumThreads > 1))
    {
        param->bEnableTemporalFilter = 0;
        x265_log(param, X265_LOG_WARNING, "MCSTF can be enabled with frame thread = 1 only. Disabling MCSTF\n");
    }
    CHECK(param->confWinRightOffset < 0, "Conformance Window Right Offset must be 0 or greater");
    CHECK(param->confWinBottomOffset < 0, "Conformance Window Bottom Offset must be 0 or greater");
    CHECK(param->decoderVbvMaxRate < 0, "Invalid Decoder Vbv Maxrate. Value can not be less than zero");
    if (param->bliveVBV2pass)
    {
        CHECK((param->rc.bStatRead == 0), "Live VBV in multi pass option requires rate control 2 pass to be enabled");
        if ((param->rc.vbvMaxBitrate <= 0 || param->rc.vbvBufferSize <= 0))
        {
            param->bliveVBV2pass = 0;
            x265_log(param, X265_LOG_WARNING, "Live VBV enabled without VBV settings.Disabling live VBV in 2 pass\n");
        }
    }
    CHECK(param->rc.dataShareMode != X265_SHARE_MODE_FILE && param->rc.dataShareMode != X265_SHARE_MODE_SHAREDMEM, "Invalid data share mode. It must be one of the X265_DATA_SHARE_MODES enum values\n" );
    return check_failed;
}

void x265_param_apply_fastfirstpass(x265_param* param)
{
    /* Set faster options in case of turbo firstpass */
    if (param->rc.bStatWrite && !param->rc.bStatRead)
    {
        param->maxNumReferences = 1;
        param->maxNumMergeCand = 1;
        param->bEnableRectInter = 0;
        param->bEnableFastIntra = 1;
        param->bEnableAMP = 0;
        param->searchMethod = X265_DIA_SEARCH;
        param->subpelRefine = X265_MIN(2, param->subpelRefine);
        param->bEnableEarlySkip = 1;
        param->rdLevel = X265_MIN(2, param->rdLevel);
    }
}

static void appendtool(x265_param* param, char* buf, size_t size, const char* toolstr)
{
    static const int overhead = (int)strlen("x265 [info]: tools: ");

    if (strlen(buf) + strlen(toolstr) + overhead >= size)
    {
        x265_log(param, X265_LOG_INFO, "tools:%s\n", buf);
        sprintf(buf, " %s", toolstr);
    }
    else
    {
        strcat(buf, " ");
        strcat(buf, toolstr);
    }
}

void x265_print_params(x265_param* param)
{
    if (param->logLevel < X265_LOG_INFO)
        return;

    if (param->interlaceMode)
        x265_log(param, X265_LOG_INFO, "Interlaced field inputs             : %s\n", x265_interlace_names[param->interlaceMode]);

    x265_log(param, X265_LOG_INFO, "Coding QT: max CU size, min CU size : %d / %d\n", param->maxCUSize, param->minCUSize);

    x265_log(param, X265_LOG_INFO, "Residual QT: max TU size, max depth : %d / %d inter / %d intra\n",
             param->maxTUSize, param->tuQTMaxInterDepth, param->tuQTMaxIntraDepth);

    if (param->bEnableHME)
        x265_log(param, X265_LOG_INFO, "HME L0,1,2 / range / subpel / merge : %s, %s, %s / %d / %d / %d\n",
            x265_motion_est_names[param->hmeSearchMethod[0]], x265_motion_est_names[param->hmeSearchMethod[1]], x265_motion_est_names[param->hmeSearchMethod[2]], param->searchRange, param->subpelRefine, param->maxNumMergeCand);
    else
        x265_log(param, X265_LOG_INFO, "ME / range / subpel / merge         : %s / %d / %d / %d\n",
            x265_motion_est_names[param->searchMethod], param->searchRange, param->subpelRefine, param->maxNumMergeCand);

    if (param->scenecutThreshold && param->keyframeMax != INT_MAX) 
        x265_log(param, X265_LOG_INFO, "Keyframe min / max / scenecut / bias  : %d / %d / %d / %.2lf \n",
                 param->keyframeMin, param->keyframeMax, param->scenecutThreshold, param->scenecutBias * 100);
    else if (param->bHistBasedSceneCut && param->keyframeMax != INT_MAX) 
        x265_log(param, X265_LOG_INFO, "Keyframe min / max / scenecut  : %d / %d / %d\n",
                 param->keyframeMin, param->keyframeMax, param->bHistBasedSceneCut);
    else if (param->keyframeMax == INT_MAX)
        x265_log(param, X265_LOG_INFO, "Keyframe min / max / scenecut       : disabled\n");

    if (param->cbQpOffset || param->crQpOffset)
        x265_log(param, X265_LOG_INFO, "Cb/Cr QP Offset                     : %d / %d\n", param->cbQpOffset, param->crQpOffset);

    if (param->rdPenalty)
        x265_log(param, X265_LOG_INFO, "Intra 32x32 TU penalty type         : %d\n", param->rdPenalty);

    x265_log(param, X265_LOG_INFO, "Lookahead / bframes / badapt        : %d / %d / %d\n", param->lookaheadDepth, param->bframes, param->bFrameAdaptive);
    x265_log(param, X265_LOG_INFO, "b-pyramid / weightp / weightb       : %d / %d / %d\n",
             param->bBPyramid, param->bEnableWeightedPred, param->bEnableWeightedBiPred);
    x265_log(param, X265_LOG_INFO, "References / ref-limit  cu / depth  : %d / %s / %s\n",
             param->maxNumReferences, (param->limitReferences & X265_REF_LIMIT_CU) ? "on" : "off",
             (param->limitReferences & X265_REF_LIMIT_DEPTH) ? "on" : "off");

    if (param->rc.aqMode)
        x265_log(param, X265_LOG_INFO, "AQ: mode / str / qg-size / cu-tree  : %d / %0.1f / %d / %d\n", param->rc.aqMode,
                 param->rc.aqStrength, param->rc.qgSize, param->rc.cuTree);

    if (param->bLossless)
        x265_log(param, X265_LOG_INFO, "Rate Control                        : Lossless\n");
    else switch (param->rc.rateControlMode)
    {
    case X265_RC_ABR:
        x265_log(param, X265_LOG_INFO, "Rate Control / qCompress            : ABR-%d kbps / %0.2f\n", param->rc.bitrate, param->rc.qCompress); break;
    case X265_RC_CQP:
        x265_log(param, X265_LOG_INFO, "Rate Control                        : CQP-%d\n", param->rc.qp); break;
    case X265_RC_CRF:
        x265_log(param, X265_LOG_INFO, "Rate Control / qCompress            : CRF-%0.1f / %0.2f\n", param->rc.rfConstant, param->rc.qCompress); break;
    }

    if (param->rc.vbvBufferSize)
    {
        if (param->vbvBufferEnd)
            x265_log(param, X265_LOG_INFO, "VBV/HRD buffer / max-rate / init / end / fr-adj: %d / %d / %.3f / %.3f / %.3f\n",
            param->rc.vbvBufferSize, param->rc.vbvMaxBitrate, param->rc.vbvBufferInit, param->vbvBufferEnd, param->vbvEndFrameAdjust);
        else
            x265_log(param, X265_LOG_INFO, "VBV/HRD buffer / max-rate / init    : %d / %d / %.3f\n",
            param->rc.vbvBufferSize, param->rc.vbvMaxBitrate, param->rc.vbvBufferInit);
    }
    
    char buf[80] = { 0 };
    char tmp[40];
#define TOOLOPT(FLAG, STR) if (FLAG) appendtool(param, buf, sizeof(buf), STR);
#define TOOLVAL(VAL, STR)  if (VAL) { sprintf(tmp, STR, VAL); appendtool(param, buf, sizeof(buf), tmp); }
    TOOLOPT(param->bEnableRectInter, "rect");
    TOOLOPT(param->bEnableAMP, "amp");
    TOOLOPT(param->limitModes, "limit-modes");
    TOOLVAL(param->rdLevel, "rd=%d");
    TOOLVAL(param->dynamicRd, "dynamic-rd=%.2f");
    TOOLOPT(param->bSsimRd, "ssim-rd");
    TOOLVAL(param->psyRd, "psy-rd=%.2lf");
    TOOLVAL(param->rdoqLevel, "rdoq=%d");
    TOOLVAL(param->psyRdoq, "psy-rdoq=%.2lf");
    TOOLOPT(param->bEnableRdRefine, "rd-refine");
    TOOLOPT(param->bEnableEarlySkip, "early-skip");
    TOOLVAL(param->recursionSkipMode, "rskip mode=%d");
    if (param->recursionSkipMode == EDGE_BASED_RSKIP)
        TOOLVAL(param->edgeVarThreshold, "rskip-edge-threshold=%.2f");
    TOOLOPT(param->bEnableSplitRdSkip, "splitrd-skip");
    TOOLVAL(param->noiseReductionIntra, "nr-intra=%d");
    TOOLVAL(param->noiseReductionInter, "nr-inter=%d");
    TOOLOPT(param->bEnableTSkipFast, "tskip-fast");
    TOOLOPT(!param->bEnableTSkipFast && param->bEnableTransformSkip, "tskip");
    TOOLVAL(param->limitTU , "limit-tu=%d");
    TOOLOPT(param->bCULossless, "cu-lossless");
    TOOLOPT(param->bEnableSignHiding, "signhide");
    TOOLOPT(param->bEnableTemporalMvp, "tmvp");
    TOOLOPT(param->bEnableConstrainedIntra, "cip");
    TOOLOPT(param->bIntraInBFrames, "b-intra");
    TOOLOPT(param->bEnableFastIntra, "fast-intra");
    TOOLOPT(param->bEnableStrongIntraSmoothing, "strong-intra-smoothing");
    TOOLVAL(param->lookaheadSlices, "lslices=%d");
    TOOLVAL(param->lookaheadThreads, "lthreads=%d")
    TOOLVAL(param->bCTUInfo, "ctu-info=%d");
    if (param->bAnalysisType == AVC_INFO)
    {
        TOOLOPT(param->bAnalysisType, "refine-analysis-type=avc");
    }
    else if (param->bAnalysisType == HEVC_INFO)
        TOOLOPT(param->bAnalysisType, "refine-analysis-type=hevc");
    TOOLOPT(param->bDynamicRefine, "dynamic-refine");
    if (param->maxSlices > 1)
        TOOLVAL(param->maxSlices, "slices=%d");
    if (param->bEnableLoopFilter)
    {
        if (param->deblockingFilterBetaOffset || param->deblockingFilterTCOffset)
        {
            sprintf(tmp, "deblock(tC=%d:B=%d)", param->deblockingFilterTCOffset, param->deblockingFilterBetaOffset);
            appendtool(param, buf, sizeof(buf), tmp);
        }
        else
            TOOLOPT(param->bEnableLoopFilter, "deblock");
    }
    TOOLOPT(param->bSaoNonDeblocked, "sao-non-deblock");
    TOOLOPT(!param->bSaoNonDeblocked && param->bEnableSAO, "sao");
    if (param->selectiveSAO && param->selectiveSAO != 4)
        TOOLOPT(param->selectiveSAO, "selective-sao");
    TOOLOPT(param->rc.bStatWrite, "stats-write");
    TOOLOPT(param->rc.bStatRead,  "stats-read");
    TOOLOPT(param->bSingleSeiNal, "single-sei");
#if ENABLE_HDR10_PLUS
    TOOLOPT(param->toneMapFile != NULL, "dhdr10-info");
#endif
    x265_log(param, X265_LOG_INFO, "tools:%s\n", buf);
    fflush(stderr);
}

char *x265_param2string(x265_param* p, int padx, int pady)
{
    char *buf, *s;
    size_t bufSize = 4000 + p->rc.zoneCount * 64;
    if (p->numaPools)
        bufSize += strlen(p->numaPools);
    if (p->masteringDisplayColorVolume)
        bufSize += strlen(p->masteringDisplayColorVolume);
    if (p->videoSignalTypePreset)
        bufSize += strlen(p->videoSignalTypePreset);

    buf = s = X265_MALLOC(char, bufSize);
    if (!buf)
        return NULL;
#define BOOL(param, cliopt) \
    s += sprintf(s, " %s", (param) ? cliopt : "no-" cliopt);

    s += sprintf(s, "cpuid=%d", p->cpuid);
    s += sprintf(s, " frame-threads=%d", p->frameNumThreads);
    if (p->numaPools)
        s += sprintf(s, " numa-pools=%s", p->numaPools);
    BOOL(p->bEnableWavefront, "wpp");
    BOOL(p->bDistributeModeAnalysis, "pmode");
    BOOL(p->bDistributeMotionEstimation, "pme");
    BOOL(p->bEnablePsnr, "psnr");
    BOOL(p->bEnableSsim, "ssim");
    s += sprintf(s, " log-level=%d", p->logLevel);
    if (p->csvfn)
        s += sprintf(s, " csv csv-log-level=%d", p->csvLogLevel);
    s += sprintf(s, " bitdepth=%d", p->internalBitDepth);
    s += sprintf(s, " input-csp=%d", p->internalCsp);
    s += sprintf(s, " fps=%u/%u", p->fpsNum, p->fpsDenom);
    s += sprintf(s, " input-res=%dx%d", p->sourceWidth - padx, p->sourceHeight - pady);
    s += sprintf(s, " interlace=%d", p->interlaceMode);
    s += sprintf(s, " total-frames=%d", p->totalFrames);
    if (p->chunkStart)
        s += sprintf(s, " chunk-start=%d", p->chunkStart);
    if (p->chunkEnd)
        s += sprintf(s, " chunk-end=%d", p->chunkEnd);
    s += sprintf(s, " level-idc=%d", p->levelIdc);
    s += sprintf(s, " high-tier=%d", p->bHighTier);
    s += sprintf(s, " uhd-bd=%d", p->uhdBluray);
    s += sprintf(s, " ref=%d", p->maxNumReferences);
    BOOL(p->bAllowNonConformance, "allow-non-conformance");
    BOOL(p->bRepeatHeaders, "repeat-headers");
    BOOL(p->bAnnexB, "annexb");
    BOOL(p->bEnableAccessUnitDelimiters, "aud");
    BOOL(p->bEnableEndOfBitstream, "eob");
    BOOL(p->bEnableEndOfSequence, "eos");
    BOOL(p->bEmitHRDSEI, "hrd");
    BOOL(p->bEmitInfoSEI, "info");
    s += sprintf(s, " hash=%d", p->decodedPictureHashSEI);
    s += sprintf(s, " temporal-layers=%d", p->bEnableTemporalSubLayers);
    BOOL(p->bOpenGOP, "open-gop");
    s += sprintf(s, " min-keyint=%d", p->keyframeMin);
    s += sprintf(s, " keyint=%d", p->keyframeMax);
    s += sprintf(s, " gop-lookahead=%d", p->gopLookahead);
    s += sprintf(s, " bframes=%d", p->bframes);
    s += sprintf(s, " b-adapt=%d", p->bFrameAdaptive);
    BOOL(p->bBPyramid, "b-pyramid");
    s += sprintf(s, " bframe-bias=%d", p->bFrameBias);
    s += sprintf(s, " rc-lookahead=%d", p->lookaheadDepth);
    s += sprintf(s, " lookahead-slices=%d", p->lookaheadSlices);
    s += sprintf(s, " scenecut=%d", p->scenecutThreshold);
    BOOL(p->bHistBasedSceneCut, "hist-scenecut");
    s += sprintf(s, " radl=%d", p->radl);
    BOOL(p->bEnableHRDConcatFlag, "splice");
    BOOL(p->bIntraRefresh, "intra-refresh");
    s += sprintf(s, " ctu=%d", p->maxCUSize);
    s += sprintf(s, " min-cu-size=%d", p->minCUSize);
    BOOL(p->bEnableRectInter, "rect");
    BOOL(p->bEnableAMP, "amp");
    s += sprintf(s, " max-tu-size=%d", p->maxTUSize);
    s += sprintf(s, " tu-inter-depth=%d", p->tuQTMaxInterDepth);
    s += sprintf(s, " tu-intra-depth=%d", p->tuQTMaxIntraDepth);
    s += sprintf(s, " limit-tu=%d", p->limitTU);
    s += sprintf(s, " rdoq-level=%d", p->rdoqLevel);
    s += sprintf(s, " dynamic-rd=%.2f", p->dynamicRd);
    BOOL(p->bSsimRd, "ssim-rd");
    BOOL(p->bEnableSignHiding, "signhide");
    BOOL(p->bEnableTransformSkip, "tskip");
    s += sprintf(s, " nr-intra=%d", p->noiseReductionIntra);
    s += sprintf(s, " nr-inter=%d", p->noiseReductionInter);
    BOOL(p->bEnableConstrainedIntra, "constrained-intra");
    BOOL(p->bEnableStrongIntraSmoothing, "strong-intra-smoothing");
    s += sprintf(s, " max-merge=%d", p->maxNumMergeCand);
    s += sprintf(s, " limit-refs=%d", p->limitReferences);
    BOOL(p->limitModes, "limit-modes");
    s += sprintf(s, " me=%d", p->searchMethod);
    s += sprintf(s, " subme=%d", p->subpelRefine);
    s += sprintf(s, " merange=%d", p->searchRange);
    BOOL(p->bEnableTemporalMvp, "temporal-mvp");
    BOOL(p->bEnableFrameDuplication, "frame-dup");
    if(p->bEnableFrameDuplication)
        s += sprintf(s, " dup-threshold=%d", p->dupThreshold);
    BOOL(p->bEnableHME, "hme");
    if (p->bEnableHME)
    {
        s += sprintf(s, " Level 0,1,2=%d,%d,%d", p->hmeSearchMethod[0], p->hmeSearchMethod[1], p->hmeSearchMethod[2]);
        s += sprintf(s, " merange L0,L1,L2=%d,%d,%d", p->hmeRange[0], p->hmeRange[1], p->hmeRange[2]);
    }
    BOOL(p->bEnableWeightedPred, "weightp");
    BOOL(p->bEnableWeightedBiPred, "weightb");
    BOOL(p->bSourceReferenceEstimation, "analyze-src-pics");
    BOOL(p->bEnableLoopFilter, "deblock");
    if (p->bEnableLoopFilter)
        s += sprintf(s, "=%d:%d", p->deblockingFilterTCOffset, p->deblockingFilterBetaOffset);
    BOOL(p->bEnableSAO, "sao");
    BOOL(p->bSaoNonDeblocked, "sao-non-deblock");
    s += sprintf(s, " rd=%d", p->rdLevel);
    s += sprintf(s, " selective-sao=%d", p->selectiveSAO);
    BOOL(p->bEnableEarlySkip, "early-skip");
    BOOL(p->recursionSkipMode, "rskip");
    if (p->recursionSkipMode == EDGE_BASED_RSKIP)
        s += sprintf(s, " rskip-edge-threshold=%f", p->edgeVarThreshold);

    BOOL(p->bEnableFastIntra, "fast-intra");
    BOOL(p->bEnableTSkipFast, "tskip-fast");
    BOOL(p->bCULossless, "cu-lossless");
    BOOL(p->bIntraInBFrames, "b-intra");
    BOOL(p->bEnableSplitRdSkip, "splitrd-skip");
    s += sprintf(s, " rdpenalty=%d", p->rdPenalty);
    s += sprintf(s, " psy-rd=%.2f", p->psyRd);
    s += sprintf(s, " psy-rdoq=%.2f", p->psyRdoq);
    BOOL(p->bEnableRdRefine, "rd-refine");
    BOOL(p->bLossless, "lossless");
    s += sprintf(s, " cbqpoffs=%d", p->cbQpOffset);
    s += sprintf(s, " crqpoffs=%d", p->crQpOffset);
    s += sprintf(s, " rc=%s", p->rc.rateControlMode == X265_RC_ABR ? (
         p->rc.bitrate == p->rc.vbvMaxBitrate ? "cbr" : "abr")
         : p->rc.rateControlMode == X265_RC_CRF ? "crf" : "cqp");
    if (p->rc.rateControlMode == X265_RC_ABR || p->rc.rateControlMode == X265_RC_CRF)
    {
        if (p->rc.rateControlMode == X265_RC_CRF)
            s += sprintf(s, " crf=%.1f", p->rc.rfConstant);
        else
            s += sprintf(s, " bitrate=%d", p->rc.bitrate);
        s += sprintf(s, " qcomp=%.2f qpstep=%d", p->rc.qCompress, p->rc.qpStep);
        s += sprintf(s, " stats-write=%d", p->rc.bStatWrite);
        s += sprintf(s, " stats-read=%d", p->rc.bStatRead);
        if (p->rc.bStatRead)
            s += sprintf(s, " cplxblur=%.1f qblur=%.1f",
            p->rc.complexityBlur, p->rc.qblur);
        if (p->rc.bStatWrite && !p->rc.bStatRead)
            BOOL(p->rc.bEnableSlowFirstPass, "slow-firstpass");
        if (p->rc.vbvBufferSize)
        {
            s += sprintf(s, " vbv-maxrate=%d vbv-bufsize=%d vbv-init=%.1f min-vbv-fullness=%.1f max-vbv-fullness=%.1f",
                p->rc.vbvMaxBitrate, p->rc.vbvBufferSize, p->rc.vbvBufferInit, p->minVbvFullness, p->maxVbvFullness);
            if (p->vbvBufferEnd)
                s += sprintf(s, " vbv-end=%.1f vbv-end-fr-adj=%.1f", p->vbvBufferEnd, p->vbvEndFrameAdjust);
            if (p->rc.rateControlMode == X265_RC_CRF)
                s += sprintf(s, " crf-max=%.1f crf-min=%.1f", p->rc.rfConstantMax, p->rc.rfConstantMin);   
        }
    }
    else if (p->rc.rateControlMode == X265_RC_CQP)
        s += sprintf(s, " qp=%d", p->rc.qp);
    if (!(p->rc.rateControlMode == X265_RC_CQP && p->rc.qp == 0))
    {
        s += sprintf(s, " ipratio=%.2f", p->rc.ipFactor);
        if (p->bframes)
            s += sprintf(s, " pbratio=%.2f", p->rc.pbFactor);
    }
    s += sprintf(s, " aq-mode=%d", p->rc.aqMode);
    s += sprintf(s, " aq-strength=%.2f", p->rc.aqStrength);
    BOOL(p->rc.cuTree, "cutree");
    s += sprintf(s, " zone-count=%d", p->rc.zoneCount);
    if (p->rc.zoneCount)
    {
        for (int i = 0; i < p->rc.zoneCount; ++i)
        {
            s += sprintf(s, " zones: start-frame=%d end-frame=%d",
                 p->rc.zones[i].startFrame, p->rc.zones[i].endFrame);
            if (p->rc.zones[i].bForceQp)
                s += sprintf(s, " qp=%d", p->rc.zones[i].qp);
            else
                s += sprintf(s, " bitrate-factor=%f", p->rc.zones[i].bitrateFactor);
        }
    }
    BOOL(p->rc.bStrictCbr, "strict-cbr");
    s += sprintf(s, " qg-size=%d", p->rc.qgSize);
    BOOL(p->rc.bEnableGrain, "rc-grain");
    s += sprintf(s, " qpmax=%d qpmin=%d", p->rc.qpMax, p->rc.qpMin);
    BOOL(p->rc.bEnableConstVbv, "const-vbv");
    s += sprintf(s, " sar=%d", p->vui.aspectRatioIdc);
    if (p->vui.aspectRatioIdc == X265_EXTENDED_SAR)
        s += sprintf(s, " sar-width : sar-height=%d:%d", p->vui.sarWidth, p->vui.sarHeight);
    s += sprintf(s, " overscan=%d", p->vui.bEnableOverscanInfoPresentFlag);
    if (p->vui.bEnableOverscanInfoPresentFlag)
        s += sprintf(s, " overscan-crop=%d", p->vui.bEnableOverscanAppropriateFlag);
    s += sprintf(s, " videoformat=%d", p->vui.videoFormat);
    s += sprintf(s, " range=%d", p->vui.bEnableVideoFullRangeFlag);
    s += sprintf(s, " colorprim=%d", p->vui.colorPrimaries);
    s += sprintf(s, " transfer=%d", p->vui.transferCharacteristics);
    s += sprintf(s, " colormatrix=%d", p->vui.matrixCoeffs);
    s += sprintf(s, " chromaloc=%d", p->vui.bEnableChromaLocInfoPresentFlag);
    if (p->vui.bEnableChromaLocInfoPresentFlag)
        s += sprintf(s, " chromaloc-top=%d chromaloc-bottom=%d",
        p->vui.chromaSampleLocTypeTopField, p->vui.chromaSampleLocTypeBottomField);
    s += sprintf(s, " display-window=%d", p->vui.bEnableDefaultDisplayWindowFlag);
    if (p->vui.bEnableDefaultDisplayWindowFlag)
        s += sprintf(s, " left=%d top=%d right=%d bottom=%d",
        p->vui.defDispWinLeftOffset, p->vui.defDispWinTopOffset,
        p->vui.defDispWinRightOffset, p->vui.defDispWinBottomOffset);
    if (p->masteringDisplayColorVolume)
        s += sprintf(s, " master-display=%s", p->masteringDisplayColorVolume);
    if (p->bEmitCLL)
        s += sprintf(s, " cll=%hu,%hu", p->maxCLL, p->maxFALL);
    s += sprintf(s, " min-luma=%hu", p->minLuma);
    s += sprintf(s, " max-luma=%hu", p->maxLuma);
    s += sprintf(s, " log2-max-poc-lsb=%d", p->log2MaxPocLsb);
    BOOL(p->bEmitVUITimingInfo, "vui-timing-info");
    BOOL(p->bEmitVUIHRDInfo, "vui-hrd-info");
    s += sprintf(s, " slices=%d", p->maxSlices);
    BOOL(p->bOptQpPPS, "opt-qp-pps");
    BOOL(p->bOptRefListLengthPPS, "opt-ref-list-length-pps");
    BOOL(p->bMultiPassOptRPS, "multi-pass-opt-rps");
    s += sprintf(s, " scenecut-bias=%.2f", p->scenecutBias);
    BOOL(p->bOptCUDeltaQP, "opt-cu-delta-qp");
    BOOL(p->bAQMotion, "aq-motion");
    BOOL(p->bEmitHDR10SEI, "hdr10");
    BOOL(p->bHDR10Opt, "hdr10-opt");
    BOOL(p->bDhdr10opt, "dhdr10-opt");
    BOOL(p->bEmitIDRRecoverySEI, "idr-recovery-sei");
    if (p->analysisSave)
        s += sprintf(s, " analysis-save");
    if (p->analysisLoad)
        s += sprintf(s, " analysis-load");
    s += sprintf(s, " analysis-reuse-level=%d", p->analysisReuseLevel);
    s += sprintf(s, " analysis-save-reuse-level=%d", p->analysisSaveReuseLevel);
    s += sprintf(s, " analysis-load-reuse-level=%d", p->analysisLoadReuseLevel);
    s += sprintf(s, " scale-factor=%d", p->scaleFactor);
    s += sprintf(s, " refine-intra=%d", p->intraRefine);
    s += sprintf(s, " refine-inter=%d", p->interRefine);
    s += sprintf(s, " refine-mv=%d", p->mvRefine);
    s += sprintf(s, " refine-ctu-distortion=%d", p->ctuDistortionRefine);
    BOOL(p->bLimitSAO, "limit-sao");
    s += sprintf(s, " ctu-info=%d", p->bCTUInfo);
    BOOL(p->bLowPassDct, "lowpass-dct");
    s += sprintf(s, " refine-analysis-type=%d", p->bAnalysisType);
    s += sprintf(s, " copy-pic=%d", p->bCopyPicToFrame);
    s += sprintf(s, " max-ausize-factor=%.1f", p->maxAUSizeFactor);
    BOOL(p->bDynamicRefine, "dynamic-refine");
    BOOL(p->bSingleSeiNal, "single-sei");
    BOOL(p->rc.hevcAq, "hevc-aq");
    BOOL(p->bEnableSvtHevc, "svt");
    BOOL(p->bField, "field");
    s += sprintf(s, " qp-adaptation-range=%.2f", p->rc.qpAdaptationRange);
    s += sprintf(s, " scenecut-aware-qp=%d", p->bEnableSceneCutAwareQp);
    if (p->bEnableSceneCutAwareQp)
        s += sprintf(s, " fwd-scenecut-window=%d fwd-ref-qp-delta=%f fwd-nonref-qp-delta=%f bwd-scenecut-window=%d bwd-ref-qp-delta=%f bwd-nonref-qp-delta=%f", p->fwdMaxScenecutWindow, p->fwdRefQpDelta[0], p->fwdNonRefQpDelta[0], p->bwdMaxScenecutWindow, p->bwdRefQpDelta[0], p->bwdNonRefQpDelta[0]);
    s += sprintf(s, "conformance-window-offsets right=%d bottom=%d", p->confWinRightOffset, p->confWinBottomOffset);
    s += sprintf(s, " decoder-max-rate=%d", p->decoderVbvMaxRate);
    BOOL(p->bliveVBV2pass, "vbv-live-multi-pass");
    if (p->filmGrain)
        s += sprintf(s, " film-grain=%s", p->filmGrain); // Film grain characteristics model filename
    BOOL(p->bEnableTemporalFilter, "mcstf");
    BOOL(p->bEnableSBRC, "sbrc");
#undef BOOL
    return buf;
}

bool parseLambdaFile(x265_param* param)
{
    if (!param->rc.lambdaFileName)
        return false;

    FILE *lfn = x265_fopen(param->rc.lambdaFileName, "r");
    if (!lfn)
    {
        x265_log_file(param, X265_LOG_ERROR, "unable to read lambda file <%s>\n", param->rc.lambdaFileName);
        return true;
    }

    char line[2048];
    char *toksave = NULL, *tok = NULL, *buf = NULL;

    for (int t = 0; t < 3; t++)
    {
        double *table = t ? x265_lambda2_tab : x265_lambda_tab;

        for (int i = 0; i < QP_MAX_MAX + 1; i++)
        {
            double value;

            do
            {
                if (!tok)
                {
                    /* consume a line of text file */
                    if (!fgets(line, sizeof(line), lfn))
                    {
                        fclose(lfn);

                        if (t < 2)
                        {
                            x265_log(param, X265_LOG_ERROR, "lambda file is incomplete\n");
                            return true;
                        }
                        else
                            return false;
                    }

                    /* truncate at first hash */
                    char *hash = strchr(line, '#');
                    if (hash) *hash = 0;
                    buf = line;
                }

                tok = strtok_r(buf, " ,", &toksave);
                buf = NULL;
                if (tok && sscanf(tok, "%lf", &value) == 1)
                    break;
            }
            while (1);

            if (t == 2)
            {
                x265_log(param, X265_LOG_ERROR, "lambda file contains too many values\n");
                fclose(lfn);
                return true;
            }
            else
                x265_log(param, X265_LOG_DEBUG, "lambda%c[%d] = %lf\n", t ? '2' : ' ', i, value);
            table[i] = value;
        }
    }

    fclose(lfn);
    return false;
}

bool parseMaskingStrength(x265_param* p, const char* value)
{
    bool bError = false;
    int window1[6];
    double refQpDelta1[6], nonRefQpDelta1[6];
    if (p->bEnableSceneCutAwareQp == FORWARD)
    {
        if (3 == sscanf(value, "%d,%lf,%lf", &window1[0], &refQpDelta1[0], &nonRefQpDelta1[0]))
        {
            if (window1[0] > 0)
                p->fwdMaxScenecutWindow = window1[0];
            if (refQpDelta1[0] > 0)
                p->fwdRefQpDelta[0] = refQpDelta1[0];
            if (nonRefQpDelta1[0] > 0)
                p->fwdNonRefQpDelta[0] = nonRefQpDelta1[0];

            p->fwdScenecutWindow[0] = p->fwdMaxScenecutWindow / 6;
            for (int i = 1; i < 6; i++)
            {
                p->fwdScenecutWindow[i] = p->fwdMaxScenecutWindow / 6;
                p->fwdRefQpDelta[i] = p->fwdRefQpDelta[i - 1] - (0.15 * p->fwdRefQpDelta[i - 1]);
                p->fwdNonRefQpDelta[i] = p->fwdNonRefQpDelta[i - 1] - (0.15 * p->fwdNonRefQpDelta[i - 1]);
            }
        }
        else if (18 == sscanf(value, "%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf"
            , &window1[0], &refQpDelta1[0], &nonRefQpDelta1[0], &window1[1], &refQpDelta1[1], &nonRefQpDelta1[1]
            , &window1[2], &refQpDelta1[2], &nonRefQpDelta1[2], &window1[3], &refQpDelta1[3], &nonRefQpDelta1[3]
            , &window1[4], &refQpDelta1[4], &nonRefQpDelta1[4], &window1[5], &refQpDelta1[5], &nonRefQpDelta1[5]))
        {
            p->fwdMaxScenecutWindow = 0;
            for (int i = 0; i < 6; i++)
            {
                p->fwdScenecutWindow[i] = window1[i];
                p->fwdRefQpDelta[i] = refQpDelta1[i];
                p->fwdNonRefQpDelta[i] = nonRefQpDelta1[i];
                p->fwdMaxScenecutWindow += p->fwdScenecutWindow[i];
            }
        }
        else
        {
            x265_log(NULL, X265_LOG_ERROR, "Specify all the necessary offsets for masking-strength \n");
            bError = true;
        }
    }
    else if (p->bEnableSceneCutAwareQp == BACKWARD)
    {
        if (3 == sscanf(value, "%d,%lf,%lf", &window1[0], &refQpDelta1[0], &nonRefQpDelta1[0]))
        {
            if (window1[0] > 0)
                p->bwdMaxScenecutWindow = window1[0];
            if (refQpDelta1[0] > 0)
                p->bwdRefQpDelta[0] = refQpDelta1[0];
            if (nonRefQpDelta1[0] > 0)
                p->bwdNonRefQpDelta[0] = nonRefQpDelta1[0];

            p->bwdScenecutWindow[0] = p->bwdMaxScenecutWindow / 6;
            for (int i = 1; i < 6; i++)
            {
                p->bwdScenecutWindow[i] = p->bwdMaxScenecutWindow / 6;
                p->bwdRefQpDelta[i] = p->bwdRefQpDelta[i - 1] - (0.15 * p->bwdRefQpDelta[i - 1]);
                p->bwdNonRefQpDelta[i] = p->bwdNonRefQpDelta[i - 1] - (0.15 * p->bwdNonRefQpDelta[i - 1]);
            }
        }
        else if (18 == sscanf(value, "%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf"
            , &window1[0], &refQpDelta1[0], &nonRefQpDelta1[0], &window1[1], &refQpDelta1[1], &nonRefQpDelta1[1]
            , &window1[2], &refQpDelta1[2], &nonRefQpDelta1[2], &window1[3], &refQpDelta1[3], &nonRefQpDelta1[3]
            , &window1[4], &refQpDelta1[4], &nonRefQpDelta1[4], &window1[5], &refQpDelta1[5], &nonRefQpDelta1[5]))
        {
            p->bwdMaxScenecutWindow = 0;
            for (int i = 0; i < 6; i++)
            {
                p->bwdScenecutWindow[i] = window1[i];
                p->bwdRefQpDelta[i] = refQpDelta1[i];
                p->bwdNonRefQpDelta[i] = nonRefQpDelta1[i];
                p->bwdMaxScenecutWindow += p->bwdScenecutWindow[i];
            }
        }
        else
        {
            x265_log(NULL, X265_LOG_ERROR, "Specify all the necessary offsets for masking-strength \n");
            bError = true;
        }
    }
    else if (p->bEnableSceneCutAwareQp == BI_DIRECTIONAL)
    {
        int window2[6];
        double refQpDelta2[6], nonRefQpDelta2[6];
        if (6 == sscanf(value, "%d,%lf,%lf,%d,%lf,%lf", &window1[0], &refQpDelta1[0], &nonRefQpDelta1[0], &window2[0], &refQpDelta2[0], &nonRefQpDelta2[0]))
        {
            if (window1[0] > 0)
                p->fwdMaxScenecutWindow = window1[0];
            if (refQpDelta1[0] > 0)
                p->fwdRefQpDelta[0] = refQpDelta1[0];
            if (nonRefQpDelta1[0] > 0)
                p->fwdNonRefQpDelta[0] = nonRefQpDelta1[0];
            if (window2[0] > 0)
                p->bwdMaxScenecutWindow = window2[0];
            if (refQpDelta2[0] > 0)
                p->bwdRefQpDelta[0] = refQpDelta2[0];
            if (nonRefQpDelta2[0] > 0)
                p->bwdNonRefQpDelta[0] = nonRefQpDelta2[0];

            p->fwdScenecutWindow[0] = p->fwdMaxScenecutWindow / 6;
            p->bwdScenecutWindow[0] = p->bwdMaxScenecutWindow / 6;
            for (int i = 1; i < 6; i++)
            {
                p->fwdScenecutWindow[i] = p->fwdMaxScenecutWindow / 6;
                p->bwdScenecutWindow[i] = p->bwdMaxScenecutWindow / 6;
                p->fwdRefQpDelta[i] = p->fwdRefQpDelta[i - 1] - (0.15 * p->fwdRefQpDelta[i - 1]);
                p->fwdNonRefQpDelta[i] = p->fwdNonRefQpDelta[i - 1] - (0.15 * p->fwdNonRefQpDelta[i - 1]);
                p->bwdRefQpDelta[i] = p->bwdRefQpDelta[i - 1] - (0.15 * p->bwdRefQpDelta[i - 1]);
                p->bwdNonRefQpDelta[i] = p->bwdNonRefQpDelta[i - 1] - (0.15 * p->bwdNonRefQpDelta[i - 1]);
            }
        }
        else if (36 == sscanf(value, "%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf,%d,%lf,%lf"
            , &window1[0], &refQpDelta1[0], &nonRefQpDelta1[0], &window1[1], &refQpDelta1[1], &nonRefQpDelta1[1]
            , &window1[2], &refQpDelta1[2], &nonRefQpDelta1[2], &window1[3], &refQpDelta1[3], &nonRefQpDelta1[3]
            , &window1[4], &refQpDelta1[4], &nonRefQpDelta1[4], &window1[5], &refQpDelta1[5], &nonRefQpDelta1[5]
            , &window2[0], &refQpDelta2[0], &nonRefQpDelta2[0], &window2[1], &refQpDelta2[1], &nonRefQpDelta2[1]
            , &window2[2], &refQpDelta2[2], &nonRefQpDelta2[2], &window2[3], &refQpDelta2[3], &nonRefQpDelta2[3]
            , &window2[4], &refQpDelta2[4], &nonRefQpDelta2[4], &window2[5], &refQpDelta2[5], &nonRefQpDelta2[5]))
        {
            p->fwdMaxScenecutWindow = 0;
            p->bwdMaxScenecutWindow = 0;
            for (int i = 0; i < 6; i++)
            {
                p->fwdScenecutWindow[i] = window1[i];
                p->fwdRefQpDelta[i] = refQpDelta1[i];
                p->fwdNonRefQpDelta[i] = nonRefQpDelta1[i];
                p->bwdScenecutWindow[i] = window2[i];
                p->bwdRefQpDelta[i] = refQpDelta2[i];
                p->bwdNonRefQpDelta[i] = nonRefQpDelta2[i];
                p->fwdMaxScenecutWindow += p->fwdScenecutWindow[i];
                p->bwdMaxScenecutWindow += p->bwdScenecutWindow[i];
            }
        }
        else
        {
            x265_log(NULL, X265_LOG_ERROR, "Specify all the necessary offsets for masking-strength \n");
            bError = true;
        }
    }
    return bError;
}

void x265_copy_params(x265_param* dst, x265_param* src)
{
    dst->cpuid = src->cpuid;
    dst->frameNumThreads = src->frameNumThreads;
    if (src->numaPools) dst->numaPools = strdup(src->numaPools);
    else dst->numaPools = NULL;

    dst->bEnableWavefront = src->bEnableWavefront;
    dst->bDistributeModeAnalysis = src->bDistributeModeAnalysis;
    dst->bDistributeMotionEstimation = src->bDistributeMotionEstimation;
    dst->bLogCuStats = src->bLogCuStats;
    dst->bEnablePsnr = src->bEnablePsnr;
    dst->bEnableSsim = src->bEnableSsim;
    dst->logLevel = src->logLevel;
    dst->csvLogLevel = src->csvLogLevel;
    if (src->csvfn) dst->csvfn = strdup(src->csvfn);
    else dst->csvfn = NULL;
    dst->internalBitDepth = src->internalBitDepth;
    dst->sourceBitDepth = src->sourceBitDepth;
    dst->internalCsp = src->internalCsp;
    dst->fpsNum = src->fpsNum;
    dst->fpsDenom = src->fpsDenom;
    dst->sourceHeight = src->sourceHeight;
    dst->sourceWidth = src->sourceWidth;
    dst->interlaceMode = src->interlaceMode;
    dst->totalFrames = src->totalFrames;
    dst->levelIdc = src->levelIdc;
    dst->bHighTier = src->bHighTier;
    dst->uhdBluray = src->uhdBluray;
    dst->maxNumReferences = src->maxNumReferences;
    dst->bAllowNonConformance = src->bAllowNonConformance;
    dst->bRepeatHeaders = src->bRepeatHeaders;
    dst->bAnnexB = src->bAnnexB;
    dst->bEnableAccessUnitDelimiters = src->bEnableAccessUnitDelimiters;
    dst->bEnableEndOfBitstream = src->bEnableEndOfBitstream;
    dst->bEnableEndOfSequence = src->bEnableEndOfSequence;
    dst->bEmitInfoSEI = src->bEmitInfoSEI;
    dst->decodedPictureHashSEI = src->decodedPictureHashSEI;
    dst->bEnableTemporalSubLayers = src->bEnableTemporalSubLayers;
    dst->bOpenGOP = src->bOpenGOP;
    dst->keyframeMax = src->keyframeMax;
    dst->keyframeMin = src->keyframeMin;
    dst->bframes = src->bframes;
    dst->bFrameAdaptive = src->bFrameAdaptive;
    dst->bFrameBias = src->bFrameBias;
    dst->bBPyramid = src->bBPyramid;
    dst->lookaheadDepth = src->lookaheadDepth;
    dst->lookaheadSlices = src->lookaheadSlices;
    dst->lookaheadThreads = src->lookaheadThreads;
    dst->scenecutThreshold = src->scenecutThreshold;
    dst->bHistBasedSceneCut = src->bHistBasedSceneCut;
    dst->bIntraRefresh = src->bIntraRefresh;
    dst->maxCUSize = src->maxCUSize;
    dst->minCUSize = src->minCUSize;
    dst->bEnableRectInter = src->bEnableRectInter;
    dst->bEnableAMP = src->bEnableAMP;
    dst->maxTUSize = src->maxTUSize;
    dst->tuQTMaxInterDepth = src->tuQTMaxInterDepth;
    dst->tuQTMaxIntraDepth = src->tuQTMaxIntraDepth;
    dst->limitTU = src->limitTU;
    dst->rdoqLevel = src->rdoqLevel;
    dst->bEnableSignHiding = src->bEnableSignHiding;
    dst->bEnableTransformSkip = src->bEnableTransformSkip;
    dst->noiseReductionInter = src->noiseReductionInter;
    dst->noiseReductionIntra = src->noiseReductionIntra;
    if (src->scalingLists) dst->scalingLists = strdup(src->scalingLists);
    else dst->scalingLists = NULL;
    dst->bEnableStrongIntraSmoothing = src->bEnableStrongIntraSmoothing;
    dst->bEnableConstrainedIntra = src->bEnableConstrainedIntra;
    dst->maxNumMergeCand = src->maxNumMergeCand;
    dst->limitReferences = src->limitReferences;
    dst->limitModes = src->limitModes;
    dst->searchMethod = src->searchMethod;
    dst->subpelRefine = src->subpelRefine;
    dst->searchRange = src->searchRange;
    dst->bEnableTemporalMvp = src->bEnableTemporalMvp;
    dst->bEnableFrameDuplication = src->bEnableFrameDuplication;
    dst->dupThreshold = src->dupThreshold;
    dst->bEnableHME = src->bEnableHME;
    if (src->bEnableHME)
    {
        for (int level = 0; level < 3; level++)
        {
            dst->hmeSearchMethod[level] = src->hmeSearchMethod[level];
            dst->hmeRange[level] = src->hmeRange[level];
        }
    }
    dst->bEnableWeightedBiPred = src->bEnableWeightedBiPred;
    dst->bEnableWeightedPred = src->bEnableWeightedPred;
    dst->bSourceReferenceEstimation = src->bSourceReferenceEstimation;
    dst->bEnableLoopFilter = src->bEnableLoopFilter;
    dst->deblockingFilterBetaOffset = src->deblockingFilterBetaOffset;
    dst->deblockingFilterTCOffset = src->deblockingFilterTCOffset;
    dst->bEnableSAO = src->bEnableSAO;
    dst->bSaoNonDeblocked = src->bSaoNonDeblocked;
    dst->rdLevel = src->rdLevel;
    dst->bEnableEarlySkip = src->bEnableEarlySkip;
    dst->recursionSkipMode = src->recursionSkipMode;
    dst->edgeVarThreshold = src->edgeVarThreshold;
    dst->bEnableFastIntra = src->bEnableFastIntra;
    dst->bEnableTSkipFast = src->bEnableTSkipFast;
    dst->bCULossless = src->bCULossless;
    dst->bIntraInBFrames = src->bIntraInBFrames;
    dst->rdPenalty = src->rdPenalty;
    dst->psyRd = src->psyRd;
    dst->psyRdoq = src->psyRdoq;
    dst->bEnableRdRefine = src->bEnableRdRefine;
    dst->analysisReuseMode = src->analysisReuseMode;
    if (src->analysisReuseFileName) dst->analysisReuseFileName=strdup(src->analysisReuseFileName);
    else dst->analysisReuseFileName = NULL;
    dst->bLossless = src->bLossless;
    dst->cbQpOffset = src->cbQpOffset;
    dst->crQpOffset = src->crQpOffset;
    dst->preferredTransferCharacteristics = src->preferredTransferCharacteristics;
    dst->pictureStructure = src->pictureStructure;

    dst->rc.rateControlMode = src->rc.rateControlMode;
    dst->rc.qp = src->rc.qp;
    dst->rc.bitrate = src->rc.bitrate;
    dst->rc.qCompress = src->rc.qCompress;
    dst->rc.ipFactor = src->rc.ipFactor;
    dst->rc.pbFactor = src->rc.pbFactor;
    dst->rc.rfConstant = src->rc.rfConstant;
    dst->rc.qpStep = src->rc.qpStep;
    dst->rc.aqMode = src->rc.aqMode;
    dst->rc.aqStrength = src->rc.aqStrength;
    dst->rc.vbvBufferSize = src->rc.vbvBufferSize;
    dst->rc.vbvMaxBitrate = src->rc.vbvMaxBitrate;

    dst->rc.vbvBufferInit = src->rc.vbvBufferInit;
    dst->minVbvFullness = src->minVbvFullness;
    dst->maxVbvFullness = src->maxVbvFullness;
    dst->rc.cuTree = src->rc.cuTree;
    dst->rc.rfConstantMax = src->rc.rfConstantMax;
    dst->rc.rfConstantMin = src->rc.rfConstantMin;
    dst->rc.bStatWrite = src->rc.bStatWrite;
    dst->rc.bStatRead = src->rc.bStatRead;
    dst->rc.dataShareMode = src->rc.dataShareMode;
    if (src->rc.statFileName) dst->rc.statFileName=strdup(src->rc.statFileName);
    else dst->rc.statFileName = NULL;
    if (src->rc.sharedMemName) dst->rc.sharedMemName = strdup(src->rc.sharedMemName);
    else dst->rc.sharedMemName = NULL;
    dst->rc.qblur = src->rc.qblur;
    dst->rc.complexityBlur = src->rc.complexityBlur;
    dst->rc.bEnableSlowFirstPass = src->rc.bEnableSlowFirstPass;
    dst->rc.zoneCount = src->rc.zoneCount;
    dst->rc.zonefileCount = src->rc.zonefileCount;
    dst->reconfigWindowSize = src->reconfigWindowSize;
    dst->bResetZoneConfig = src->bResetZoneConfig;
    dst->bNoResetZoneConfig = src->bNoResetZoneConfig;
    dst->decoderVbvMaxRate = src->decoderVbvMaxRate;

    if (src->rc.zonefileCount && src->rc.zones && src->bResetZoneConfig)
    {
        for (int i = 0; i < src->rc.zonefileCount; i++)
        {
            dst->rc.zones[i].startFrame = src->rc.zones[i].startFrame;
            dst->rc.zones[0].keyframeMax = src->rc.zones[0].keyframeMax;
            memcpy(dst->rc.zones[i].zoneParam, src->rc.zones[i].zoneParam, sizeof(x265_param));
        }
    }
    else if (src->rc.zoneCount && src->rc.zones)
    {
        for (int i = 0; i < src->rc.zoneCount; i++)
        {
            dst->rc.zones[i].startFrame = src->rc.zones[i].startFrame;
            dst->rc.zones[i].endFrame = src->rc.zones[i].endFrame;
            dst->rc.zones[i].bForceQp = src->rc.zones[i].bForceQp;
            dst->rc.zones[i].qp = src->rc.zones[i].qp;
            dst->rc.zones[i].bitrateFactor = src->rc.zones[i].bitrateFactor;
        }
    }
    else
        dst->rc.zones = NULL;

    if (src->rc.lambdaFileName) dst->rc.lambdaFileName = strdup(src->rc.lambdaFileName);
    else dst->rc.lambdaFileName = NULL;
    dst->rc.bStrictCbr = src->rc.bStrictCbr;
    dst->rc.qgSize = src->rc.qgSize;
    dst->rc.bEnableGrain = src->rc.bEnableGrain;
    dst->rc.qpMax = src->rc.qpMax;
    dst->rc.qpMin = src->rc.qpMin;
    dst->rc.bEnableConstVbv = src->rc.bEnableConstVbv;
    dst->rc.hevcAq = src->rc.hevcAq;
    dst->rc.qpAdaptationRange = src->rc.qpAdaptationRange;

    dst->vui.aspectRatioIdc = src->vui.aspectRatioIdc;
    dst->vui.sarWidth = src->vui.sarWidth;
    dst->vui.sarHeight = src->vui.sarHeight;
    dst->vui.bEnableOverscanAppropriateFlag = src->vui.bEnableOverscanAppropriateFlag;
    dst->vui.bEnableOverscanInfoPresentFlag = src->vui.bEnableOverscanInfoPresentFlag;
    dst->vui.bEnableVideoSignalTypePresentFlag = src->vui.bEnableVideoSignalTypePresentFlag;
    dst->vui.videoFormat = src->vui.videoFormat;
    dst->vui.bEnableVideoFullRangeFlag = src->vui.bEnableVideoFullRangeFlag;
    dst->vui.bEnableColorDescriptionPresentFlag = src->vui.bEnableColorDescriptionPresentFlag;
    dst->vui.colorPrimaries = src->vui.colorPrimaries;
    dst->vui.transferCharacteristics = src->vui.transferCharacteristics;
    dst->vui.matrixCoeffs = src->vui.matrixCoeffs;
    dst->vui.bEnableChromaLocInfoPresentFlag = src->vui.bEnableChromaLocInfoPresentFlag;
    dst->vui.chromaSampleLocTypeTopField = src->vui.chromaSampleLocTypeTopField;
    dst->vui.chromaSampleLocTypeBottomField = src->vui.chromaSampleLocTypeBottomField;
    dst->vui.bEnableDefaultDisplayWindowFlag = src->vui.bEnableDefaultDisplayWindowFlag;
    dst->vui.defDispWinBottomOffset = src->vui.defDispWinBottomOffset;
    dst->vui.defDispWinLeftOffset = src->vui.defDispWinLeftOffset;
    dst->vui.defDispWinRightOffset = src->vui.defDispWinRightOffset;
    dst->vui.defDispWinTopOffset = src->vui.defDispWinTopOffset;

    if (src->masteringDisplayColorVolume) dst->masteringDisplayColorVolume=strdup( src->masteringDisplayColorVolume);
    else dst->masteringDisplayColorVolume = NULL;
    dst->maxLuma = src->maxLuma;
    dst->minLuma = src->minLuma;
    dst->bEmitCLL = src->bEmitCLL;
    dst->maxCLL = src->maxCLL;
    dst->maxFALL = src->maxFALL;
    dst->log2MaxPocLsb = src->log2MaxPocLsb;
    dst->bEmitVUIHRDInfo = src->bEmitVUIHRDInfo;
    dst->bEmitVUITimingInfo = src->bEmitVUITimingInfo;
    dst->maxSlices = src->maxSlices;
    dst->bOptQpPPS = src->bOptQpPPS;
    dst->bOptRefListLengthPPS = src->bOptRefListLengthPPS;
    dst->bMultiPassOptRPS = src->bMultiPassOptRPS;
    dst->scenecutBias = src->scenecutBias;
    dst->gopLookahead = src->lookaheadDepth;
    dst->bOptCUDeltaQP = src->bOptCUDeltaQP;
    dst->analysisMultiPassDistortion = src->analysisMultiPassDistortion;
    dst->analysisMultiPassRefine = src->analysisMultiPassRefine;
    dst->bAQMotion = src->bAQMotion;
    dst->bSsimRd = src->bSsimRd;
    dst->dynamicRd = src->dynamicRd;
    dst->bEmitHDR10SEI = src->bEmitHDR10SEI;
    dst->bEmitHRDSEI = src->bEmitHRDSEI;
    dst->bHDROpt = src->bHDROpt; /*DEPRECATED*/
    dst->bHDR10Opt = src->bHDR10Opt;
    dst->analysisReuseLevel = src->analysisReuseLevel;
    dst->analysisSaveReuseLevel = src->analysisSaveReuseLevel;
    dst->analysisLoadReuseLevel = src->analysisLoadReuseLevel;
    dst->bLimitSAO = src->bLimitSAO;
    if (src->toneMapFile) dst->toneMapFile = strdup(src->toneMapFile);
    else dst->toneMapFile = NULL;
    dst->bDhdr10opt = src->bDhdr10opt;
    dst->bCTUInfo = src->bCTUInfo;
    dst->bUseRcStats = src->bUseRcStats;
    dst->interRefine = src->interRefine;
    dst->intraRefine = src->intraRefine;
    dst->mvRefine = src->mvRefine;
    dst->maxLog2CUSize = src->maxLog2CUSize;
    dst->maxCUDepth = src->maxCUDepth;
    dst->unitSizeDepth = src->unitSizeDepth;
    dst->num4x4Partitions = src->num4x4Partitions;

    dst->csvfpt = src->csvfpt;
    dst->bEnableSplitRdSkip = src->bEnableSplitRdSkip;
    dst->bUseAnalysisFile = src->bUseAnalysisFile;
    dst->forceFlush = src->forceFlush;
    dst->bDisableLookahead = src->bDisableLookahead;
    dst->bLowPassDct = src->bLowPassDct;
    dst->vbvBufferEnd = src->vbvBufferEnd;
    dst->vbvEndFrameAdjust = src->vbvEndFrameAdjust;
    dst->bAnalysisType = src->bAnalysisType;
    dst->bCopyPicToFrame = src->bCopyPicToFrame;
    if (src->analysisSave) dst->analysisSave=strdup(src->analysisSave);
    else dst->analysisSave = NULL;
    if (src->analysisLoad) dst->analysisLoad=strdup(src->analysisLoad);
    else dst->analysisLoad = NULL;
    dst->gopLookahead = src->gopLookahead;
    dst->radl = src->radl;
    dst->selectiveSAO = src->selectiveSAO;
    dst->maxAUSizeFactor = src->maxAUSizeFactor;
    dst->bEmitIDRRecoverySEI = src->bEmitIDRRecoverySEI;
    dst->bDynamicRefine = src->bDynamicRefine;
    dst->bSingleSeiNal = src->bSingleSeiNal;
    dst->chunkStart = src->chunkStart;
    dst->chunkEnd = src->chunkEnd;
    if (src->naluFile) dst->naluFile=strdup(src->naluFile);
    else dst->naluFile = NULL;
    dst->scaleFactor = src->scaleFactor;
    dst->ctuDistortionRefine = src->ctuDistortionRefine;
    dst->bEnableHRDConcatFlag = src->bEnableHRDConcatFlag;
    dst->dolbyProfile = src->dolbyProfile;
    dst->bEnableSvtHevc = src->bEnableSvtHevc;
    dst->bEnableFades = src->bEnableFades;
    dst->bEnableSceneCutAwareQp = src->bEnableSceneCutAwareQp;
    dst->fwdMaxScenecutWindow = src->fwdMaxScenecutWindow;
    dst->bwdMaxScenecutWindow = src->bwdMaxScenecutWindow;
    for (int i = 0; i < 6; i++)
    {
        dst->fwdScenecutWindow[i] = src->fwdScenecutWindow[i];
        dst->fwdRefQpDelta[i] = src->fwdRefQpDelta[i];
        dst->fwdNonRefQpDelta[i] = src->fwdNonRefQpDelta[i];
        dst->bwdScenecutWindow[i] = src->bwdScenecutWindow[i];
        dst->bwdRefQpDelta[i] = src->bwdRefQpDelta[i];
        dst->bwdNonRefQpDelta[i] = src->bwdNonRefQpDelta[i];
    }
    dst->bField = src->bField;
    dst->bEnableTemporalFilter = src->bEnableTemporalFilter;
    dst->temporalFilterStrength = src->temporalFilterStrength;
    dst->confWinRightOffset = src->confWinRightOffset;
    dst->confWinBottomOffset = src->confWinBottomOffset;
    dst->bliveVBV2pass = src->bliveVBV2pass;

    if (src->videoSignalTypePreset) dst->videoSignalTypePreset = strdup(src->videoSignalTypePreset);
    else dst->videoSignalTypePreset = NULL;
#ifdef SVT_HEVC
    memcpy(dst->svtHevcParam, src->svtHevcParam, sizeof(EB_H265_ENC_CONFIGURATION));
#endif
    /* Film grain */
    if (src->filmGrain)
        dst->filmGrain = src->filmGrain;
    dst->bEnableSBRC = src->bEnableSBRC;
}

#ifdef SVT_HEVC

void svt_param_default(x265_param* param)
{
    EB_H265_ENC_CONFIGURATION* svtHevcParam = (EB_H265_ENC_CONFIGURATION*)param->svtHevcParam;

    // Channel info
    svtHevcParam->channelId = 0;
    svtHevcParam->activeChannelCount = 0;

    // GOP Structure
    svtHevcParam->intraPeriodLength = -2;
    svtHevcParam->intraRefreshType = 1;
    svtHevcParam->predStructure = 2;
    svtHevcParam->baseLayerSwitchMode = 0;
    svtHevcParam->hierarchicalLevels = 3;
    svtHevcParam->sourceWidth = 0;
    svtHevcParam->sourceHeight = 0;
    svtHevcParam->latencyMode = 0;

    //Preset & Tune
    svtHevcParam->encMode = 7;
    svtHevcParam->tune = 1;

    // Interlaced Video 
    svtHevcParam->interlacedVideo = 0;

    // Quantization
    svtHevcParam->qp = 32;
    svtHevcParam->useQpFile = 0;

    // Deblock Filter
    svtHevcParam->disableDlfFlag = 0;

    // SAO
    svtHevcParam->enableSaoFlag = 1;

    // ME Tools
    svtHevcParam->useDefaultMeHme = 1;
    svtHevcParam->enableHmeFlag = 1;

    // ME Parameters
    svtHevcParam->searchAreaWidth = 16;
    svtHevcParam->searchAreaHeight = 7;

    // MD Parameters
    svtHevcParam->constrainedIntra = 0;

    // Rate Control
    svtHevcParam->frameRate = 60;
    svtHevcParam->frameRateNumerator = 0;
    svtHevcParam->frameRateDenominator = 0;
    svtHevcParam->encoderBitDepth = 8;
    svtHevcParam->encoderColorFormat = EB_YUV420;
    svtHevcParam->compressedTenBitFormat = 0;
    svtHevcParam->rateControlMode = 0;
    svtHevcParam->sceneChangeDetection = 1;
    svtHevcParam->lookAheadDistance = (uint32_t)~0;
    svtHevcParam->framesToBeEncoded = 0;
    svtHevcParam->targetBitRate = 7000000;
    svtHevcParam->maxQpAllowed = 48;
    svtHevcParam->minQpAllowed = 10;
    svtHevcParam->bitRateReduction = 0;

    // Thresholds
    svtHevcParam->improveSharpness = 0;
    svtHevcParam->videoUsabilityInfo = 0;
    svtHevcParam->highDynamicRangeInput = 0;
    svtHevcParam->accessUnitDelimiter = 0;
    svtHevcParam->bufferingPeriodSEI = 0;
    svtHevcParam->pictureTimingSEI = 0;
    svtHevcParam->registeredUserDataSeiFlag = 0;
    svtHevcParam->unregisteredUserDataSeiFlag = 0;
    svtHevcParam->recoveryPointSeiFlag = 0;
    svtHevcParam->enableTemporalId = 1;
    svtHevcParam->profile = 1;
    svtHevcParam->tier = 0;
    svtHevcParam->level = 0;

    svtHevcParam->injectorFrameRate = 60 << 16;
    svtHevcParam->speedControlFlag = 0;

    // ASM Type
    svtHevcParam->asmType = 1;

    svtHevcParam->codeVpsSpsPps = 1;
    svtHevcParam->codeEosNal = 0;
    svtHevcParam->reconEnabled = 0;
    svtHevcParam->maxCLL = 0;
    svtHevcParam->maxFALL = 0;
    svtHevcParam->useMasteringDisplayColorVolume = 0;
    svtHevcParam->useNaluFile = 0;
    svtHevcParam->whitePointX = 0;
    svtHevcParam->whitePointY = 0;
    svtHevcParam->maxDisplayMasteringLuminance = 0;
    svtHevcParam->minDisplayMasteringLuminance = 0;
    svtHevcParam->dolbyVisionProfile = 0;
    svtHevcParam->targetSocket = -1;
    svtHevcParam->logicalProcessors = 0;
    svtHevcParam->switchThreadsToRtPriority = 1;
    svtHevcParam->fpsInVps = 0;

    svtHevcParam->tileColumnCount = 1;
    svtHevcParam->tileRowCount = 1;
    svtHevcParam->tileSliceMode = 0;
    svtHevcParam->unrestrictedMotionVector = 1;
    svtHevcParam->threadCount = 0;

    // vbv
    svtHevcParam->hrdFlag = 0;
    svtHevcParam->vbvMaxrate = 0;
    svtHevcParam->vbvBufsize = 0;
    svtHevcParam->vbvBufInit = 90;
}

int svt_set_preset(x265_param* param, const char* preset)
{
    EB_H265_ENC_CONFIGURATION* svtHevcParam = (EB_H265_ENC_CONFIGURATION*)param->svtHevcParam;
    
    if (preset)
    {
        if (!strcmp(preset, "ultrafast")) svtHevcParam->encMode = 11;
        else if (!strcmp(preset, "superfast")) svtHevcParam->encMode = 10;
        else if (!strcmp(preset, "veryfast")) svtHevcParam->encMode = 9;
        else if (!strcmp(preset, "faster")) svtHevcParam->encMode = 8;
        else if (!strcmp(preset, "fast")) svtHevcParam->encMode = 7;
        else if (!strcmp(preset, "medium")) svtHevcParam->encMode = 6;
        else if (!strcmp(preset, "slow")) svtHevcParam->encMode = 5;
        else if (!strcmp(preset, "slower")) svtHevcParam->encMode =4;
        else if (!strcmp(preset, "veryslow")) svtHevcParam->encMode = 3;
        else if (!strcmp(preset, "placebo")) svtHevcParam->encMode = 2;
        else  return -1;
    }
    return 0;
}

int svt_param_parse(x265_param* param, const char* name, const char* value)
{
    bool bError = false;
#define OPT(STR) else if (!strcmp(name, STR))

    EB_H265_ENC_CONFIGURATION* svtHevcParam = (EB_H265_ENC_CONFIGURATION*)param->svtHevcParam;
    if (0);
    OPT("input-res")  bError |= sscanf(value, "%dx%d", &svtHevcParam->sourceWidth, &svtHevcParam->sourceHeight) != 2;
    OPT("input-depth") svtHevcParam->encoderBitDepth = atoi(value);
    OPT("total-frames") svtHevcParam->framesToBeEncoded = atoi(value);
    OPT("frames") svtHevcParam->framesToBeEncoded = atoi(value);
    OPT("fps")
    {
        if (sscanf(value, "%u/%u", &svtHevcParam->frameRateNumerator, &svtHevcParam->frameRateDenominator) == 2)
            ;
        else
        {
            int fps = atoi(value);
            svtHevcParam->frameRateDenominator = 1;

            if (fps < 1000)
                svtHevcParam->frameRate = fps << 16;
            else
                svtHevcParam->frameRate = fps;
        }
    }
    OPT2("level-idc", "level")
    {
        /* allow "5.1" or "51", both converted to integer 51 */
        /* if level-idc specifies an obviously wrong value in either float or int,
        throw error consistently. Stronger level checking will be done in encoder_open() */
        if (atof(value) < 10)
            svtHevcParam->level = (int)(10 * atof(value) + .5);
        else if (atoi(value) < 100)
            svtHevcParam->level = atoi(value);
        else
            bError = true;
    }
    OPT2("pools", "numa-pools")
    {
        char *pools = strdup(value);
        char *temp1, *temp2;
        int count = 0;

        for (temp1 = strstr(pools, ","); temp1 != NULL; temp1 = strstr(temp2, ","))
        {
            temp2 = ++temp1;
            count++;
        }

        if (count > 1)
            x265_log(param, X265_LOG_WARNING, "SVT-HEVC Encoder supports pools option only upto 2 sockets \n");
        else if (count == 1)
        {
            temp1 = strtok(pools, ",");
            temp2 = strtok(NULL, ",");

            if (!strcmp(temp1, "+"))
            {
                if (!strcmp(temp2, "+")) svtHevcParam->targetSocket = -1;
                else if (!strcmp(temp2, "-")) svtHevcParam->targetSocket = 0;
                else svtHevcParam->targetSocket = -1;
            }
            else if (!strcmp(temp1, "-"))
            {
                if (!strcmp(temp2, "+")) svtHevcParam->targetSocket = 1;
                else if (!strcmp(temp2, "-")) x265_log(param, X265_LOG_ERROR, "Shouldn't exclude both sockets for pools option %s \n", pools);
                else if (!strcmp(temp2, "*")) svtHevcParam->targetSocket = 1;
                else
                {
                    svtHevcParam->targetSocket = 1;
                    svtHevcParam->logicalProcessors = atoi(temp2);
                }
            }
            else svtHevcParam->targetSocket = -1;
        }
        else
        {
            if (!strcmp(temp1, "*")) svtHevcParam->targetSocket = -1;
            else
            {
                svtHevcParam->targetSocket = 0;
                svtHevcParam->logicalProcessors = atoi(temp1);
            }
        }
    }
    OPT("high-tier") svtHevcParam->tier = x265_atobool(value, bError);
    OPT("qpmin") svtHevcParam->minQpAllowed = atoi(value);
    OPT("qpmax") svtHevcParam->maxQpAllowed = atoi(value);
    OPT("rc-lookahead") svtHevcParam->lookAheadDistance = atoi(value);
    OPT("scenecut")
    {
        svtHevcParam->sceneChangeDetection = x265_atobool(value, bError);
        if (bError || svtHevcParam->sceneChangeDetection)
        {
            bError = false;
            svtHevcParam->sceneChangeDetection = 1;
        }
    }
    OPT("open-gop")
    {
        if (x265_atobool(value, bError))
            svtHevcParam->intraRefreshType = 1;
        else
            svtHevcParam->intraRefreshType = 2;
    }
    OPT("deblock")
    {
        if (strtol(value, NULL, 0))
            svtHevcParam->disableDlfFlag = 0;
        else if (x265_atobool(value, bError) == 0 && !bError)
            svtHevcParam->disableDlfFlag = 1;
    }
    OPT("sao") svtHevcParam->enableSaoFlag = (uint8_t)x265_atobool(value, bError);
    OPT("keyint") svtHevcParam->intraPeriodLength = atoi(value);
    OPT2("constrained-intra", "cip") svtHevcParam->constrainedIntra = (uint8_t)x265_atobool(value, bError);
    OPT("vui-timing-info") svtHevcParam->videoUsabilityInfo = x265_atobool(value, bError);
    OPT("hdr") svtHevcParam->highDynamicRangeInput = x265_atobool(value, bError);
    OPT("aud") svtHevcParam->accessUnitDelimiter = x265_atobool(value, bError);
    OPT("qp")
    {
        svtHevcParam->rateControlMode = 0;
        svtHevcParam->qp = atoi(value);
    }
    OPT("bitrate")
    {
        svtHevcParam->rateControlMode = 1;
        svtHevcParam->targetBitRate = atoi(value);
    }
    OPT("interlace")
    {
        svtHevcParam->interlacedVideo = (uint8_t)x265_atobool(value, bError);
        if (bError || svtHevcParam->interlacedVideo)
        {
            bError = false;
            svtHevcParam->interlacedVideo = 1;
        }
    }
    OPT("svt-hme")
    {
        svtHevcParam->enableHmeFlag = (uint8_t)x265_atobool(value, bError);
        if (svtHevcParam->enableHmeFlag) svtHevcParam->useDefaultMeHme = 1;
    }
    OPT("svt-search-width") svtHevcParam->searchAreaWidth = atoi(value);
    OPT("svt-search-height") svtHevcParam->searchAreaHeight = atoi(value);
    OPT("svt-compressed-ten-bit-format") svtHevcParam->compressedTenBitFormat = x265_atobool(value, bError);
    OPT("svt-speed-control") svtHevcParam->speedControlFlag = x265_atobool(value, bError);
    OPT("svt-preset-tuner")
    {
        if (svtHevcParam->encMode == 2)
        {
            if (!strcmp(value, "0")) svtHevcParam->encMode = 0;
            else if (!strcmp(value, "1")) svtHevcParam->encMode = 1;
            else
            {
                x265_log(param, X265_LOG_ERROR, " Unsupported value=%s for svt-preset-tuner \n", value);
                bError = true;
            }
        }
        else
            x265_log(param, X265_LOG_WARNING, " svt-preset-tuner should be used only with ultrafast preset; Ignoring it \n");
    }
    OPT("svt-hierarchical-level") svtHevcParam->hierarchicalLevels = atoi(value);
    OPT("svt-base-layer-switch-mode") svtHevcParam->baseLayerSwitchMode = atoi(value);
    OPT("svt-pred-struct") svtHevcParam->predStructure = (uint8_t)atoi(value);
    OPT("svt-fps-in-vps") svtHevcParam->fpsInVps = (uint8_t)x265_atobool(value, bError);
    OPT("master-display") svtHevcParam->useMasteringDisplayColorVolume = (uint8_t)atoi(value);
    OPT("max-cll") bError |= sscanf(value, "%hu,%hu", &svtHevcParam->maxCLL, &svtHevcParam->maxFALL) != 2;
    OPT("nalu-file") svtHevcParam->useNaluFile = (uint8_t)atoi(value);
    OPT("dolby-vision-profile")
    {
        if (atof(value) < 10)
            svtHevcParam->dolbyVisionProfile = (int)(10 * atof(value) + .5);
        else if (atoi(value) < 100)
            svtHevcParam->dolbyVisionProfile = atoi(value);
        else
            bError = true;
    }
    OPT("hrd")
        svtHevcParam->hrdFlag = (uint32_t)x265_atobool(value, bError);
    OPT("vbv-maxrate")
        svtHevcParam->vbvMaxrate = (uint32_t)x265_atoi(value, bError);
    OPT("vbv-bufsize")
        svtHevcParam->vbvBufsize = (uint32_t)x265_atoi(value, bError);
    OPT("vbv-init")
        svtHevcParam->vbvBufInit = (uint64_t)x265_atof(value, bError);
    OPT("frame-threads")
        svtHevcParam->threadCount = (uint32_t)x265_atoi(value, bError);
    else
        x265_log(param, X265_LOG_INFO, "SVT doesn't support %s param; Disabling it \n", name);


    return bError ? X265_PARAM_BAD_VALUE : 0;
}

#endif //ifdef SVT_HEVC

}
