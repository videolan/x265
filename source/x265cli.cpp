/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Min Chen <chenm003@163.com>
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
#if _MSC_VER
#pragma warning(disable: 4127) // conditional expression is constant, yes I know
#endif

#include "x265cli.h"
#include "svt.h"

#define START_CODE 0x00000001
#define START_CODE_BYTES 4

#ifdef __cplusplus
namespace X265_NS {
#endif

    static void printVersion(x265_param *param, const x265_api* api)
    {
        x265_log(param, X265_LOG_INFO, "HEVC encoder version %s\n", api->version_str);
        x265_log(param, X265_LOG_INFO, "build info %s\n", api->build_info_str);
    }

    static void showHelp(x265_param *param)
    {
        int level = param->logLevel;

#define OPT(value) (value ? "enabled" : "disabled")
#define H0 printf
#define H1 if (level >= X265_LOG_DEBUG) printf

        H0("\nSyntax: x265 [options] infile [-o] outfile\n");
        H0("    infile can be YUV or Y4M\n");
        H0("    outfile is raw HEVC bitstream\n");
        H0("\nExecutable Options:\n");
        H0("-h/--help                        Show this help text and exit\n");
        H0("   --fullhelp                    Show all options and exit\n");
        H0("-V/--version                     Show version info and exit\n");
        H0("\nOutput Options:\n");
        H0("-o/--output <filename>           Bitstream output file name\n");
        H0("-D/--output-depth 8|10|12        Output bit depth (also internal bit depth). Default %d\n", param->internalBitDepth);
        H0("   --log-level <string>          Logging level: none error warning info debug full. Default %s\n", X265_NS::logLevelNames[param->logLevel + 1]);
        H0("   --no-progress                 Disable CLI progress reports\n");
        H0("   --csv <filename>              Comma separated log file, if csv-log-level > 0 frame level statistics, else one line per run\n");
        H0("   --csv-log-level <integer>     Level of csv logging, if csv-log-level > 0 frame level statistics, else one line per run: 0-2\n");
        H0("\nInput Options:\n");
        H0("   --input <filename>            Raw YUV or Y4M input file name. `-` for stdin\n");
        H1("   --y4m                         Force parsing of input stream as YUV4MPEG2 regardless of file extension\n");
        H0("   --fps <float|rational>        Source frame rate (float or num/denom), auto-detected if Y4M\n");
        H0("   --input-res WxH               Source picture size [w x h], auto-detected if Y4M\n");
        H1("   --input-depth <integer>       Bit-depth of input file. Default 8\n");
        H1("   --input-csp <string>          Chroma subsampling, auto-detected if Y4M\n");
        H1("                                 0 - i400 (4:0:0 monochrome)\n");
        H1("                                 1 - i420 (4:2:0 default)\n");
        H1("                                 2 - i422 (4:2:2)\n");
        H1("                                 3 - i444 (4:4:4)\n");
#if ENABLE_HDR10_PLUS
        H0("   --dhdr10-info <filename>      JSON file containing the Creative Intent Metadata to be encoded as Dynamic Tone Mapping\n");
        H0("   --[no-]dhdr10-opt             Insert tone mapping SEI only for IDR frames and when the tone mapping information changes. Default disabled\n");
#endif
        H0("   --dolby-vision-profile <float|integer> Specifies Dolby Vision profile ID. Currently only profile 5, profile 8.1 and profile 8.2 enabled. Specified as '5' or '50'. Default 0 (disabled).\n");
        H0("   --dolby-vision-rpu <filename> File containing Dolby Vision RPU metadata.\n"
            "                                 If given, x265's Dolby Vision metadata parser will fill the RPU field of input pictures with the metadata read from the file. Default NULL(disabled).\n");
        H0("   --nalu-file <filename>        Text file containing SEI messages in the following format : <POC><space><PREFIX><space><NAL UNIT TYPE>/<SEI TYPE><space><SEI Payload>\n");
        H0("-f/--frames <integer>            Maximum number of frames to encode. Default all\n");
        H0("   --seek <integer>              First frame to encode\n");
        H1("   --[no-]interlace <bff|tff>    Indicate input pictures are interlace fields in temporal order. Default progressive\n");
        H0("   --[no-]field                  Enable or disable field coding. Default %s\n", OPT(param->bField));
        H1("   --dither                      Enable dither if downscaling to 8 bit pixels. Default disabled\n");
        H0("   --[no-]copy-pic               Copy buffers of input picture in frame. Default %s\n", OPT(param->bCopyPicToFrame));
        H0("\nQuality reporting metrics:\n");
        H0("   --[no-]ssim                   Enable reporting SSIM metric scores. Default %s\n", OPT(param->bEnableSsim));
        H0("   --[no-]psnr                   Enable reporting PSNR metric scores. Default %s\n", OPT(param->bEnablePsnr));
        H0("\nProfile, Level, Tier:\n");
        H0("-P/--profile <string>            Enforce an encode profile: main, main10, mainstillpicture\n");
        H0("   --level-idc <integer|float>   Force a minimum required decoder level (as '5.0' or '50')\n");
        H0("   --[no-]high-tier              If a decoder level is specified, this modifier selects High tier of that level\n");
        H0("   --uhd-bd                      Enable UHD Bluray compatibility support\n");
        H0("   --[no-]allow-non-conformance  Allow the encoder to generate profile NONE bitstreams. Default %s\n", OPT(param->bAllowNonConformance));
        H0("\nThreading, performance:\n");
        H0("   --pools <integer,...>         Comma separated thread count per thread pool (pool per NUMA node)\n");
        H0("                                 '-' implies no threads on node, '+' implies one thread per core on node\n");
        H0("-F/--frame-threads <integer>     Number of concurrently encoded frames. 0: auto-determined by core count\n");
        H0("   --[no-]wpp                    Enable Wavefront Parallel Processing. Default %s\n", OPT(param->bEnableWavefront));
        H0("   --[no-]slices <integer>       Enable Multiple Slices feature. Default %d\n", param->maxSlices);
        H0("   --[no-]pmode                  Parallel mode analysis. Default %s\n", OPT(param->bDistributeModeAnalysis));
        H0("   --[no-]pme                    Parallel motion estimation. Default %s\n", OPT(param->bDistributeMotionEstimation));
        H0("   --[no-]asm <bool|int|string>  Override CPU detection. Default: auto\n");
        H0("\nPresets:\n");
        H0("-p/--preset <string>             Trade off performance for compression efficiency. Default medium\n");
        H0("                                 ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow, or placebo\n");
        H0("-t/--tune <string>               Tune the settings for a particular type of source or situation:\n");
        H0("                                 psnr, ssim, grain, zerolatency, fastdecode\n");
        H0("\nQuad-Tree size and depth:\n");
        H0("-s/--ctu <64|32|16>              Maximum CU size (WxH). Default %d\n", param->maxCUSize);
        H0("   --min-cu-size <64|32|16|8>    Minimum CU size (WxH). Default %d\n", param->minCUSize);
        H0("   --max-tu-size <32|16|8|4>     Maximum TU size (WxH). Default %d\n", param->maxTUSize);
        H0("   --tu-intra-depth <integer>    Max TU recursive depth for intra CUs. Default %d\n", param->tuQTMaxIntraDepth);
        H0("   --tu-inter-depth <integer>    Max TU recursive depth for inter CUs. Default %d\n", param->tuQTMaxInterDepth);
        H0("   --limit-tu <0..4>             Enable early exit from TU recursion for inter coded blocks. Default %d\n", param->limitTU);
        H0("\nAnalysis:\n");
        H0("   --rd <1..6>                   Level of RDO in mode decision 1:least....6:full RDO. Default %d\n", param->rdLevel);
        H0("   --[no-]psy-rd <0..5.0>        Strength of psycho-visual rate distortion optimization, 0 to disable. Default %.1f\n", param->psyRd);
        H0("   --[no-]rdoq-level <0|1|2>     Level of RDO in quantization 0:none, 1:levels, 2:levels & coding groups. Default %d\n", param->rdoqLevel);
        H0("   --[no-]psy-rdoq <0..50.0>     Strength of psycho-visual optimization in RDO quantization, 0 to disable. Default %.1f\n", param->psyRdoq);
        H0("   --dynamic-rd <0..4.0>         Strength of dynamic RD, 0 to disable. Default %.2f\n", param->dynamicRd);
        H0("   --[no-]ssim-rd                Enable ssim rate distortion optimization, 0 to disable. Default %s\n", OPT(param->bSsimRd));
        H0("   --[no-]rd-refine              Enable QP based RD refinement for rd levels 5 and 6. Default %s\n", OPT(param->bEnableRdRefine));
        H0("   --[no-]early-skip             Enable early SKIP detection. Default %s\n", OPT(param->bEnableEarlySkip));
        H0("   --rskip <mode>                Set mode for early exit from recursion. Mode 1: exit using rdcost & CU homogenity. Mode 2: exit using CU edge density.\n"
            "                                 Mode 0: disabled. Default %d\n", param->recursionSkipMode);
        H1("   --rskip-edge-threshold        Threshold in terms of percentage (integer of range [0,100]) for minimum edge density in CUs used to prun the recursion depth. Applicable only for rskip mode 2. Value is preset dependent. Default: %.f\n", param->edgeVarThreshold*100.0f);
        H1("   --[no-]tskip-fast             Enable fast intra transform skipping. Default %s\n", OPT(param->bEnableTSkipFast));
        H1("   --[no-]splitrd-skip           Enable skipping split RD analysis when sum of split CU rdCost larger than one split CU rdCost for Intra CU. Default %s\n", OPT(param->bEnableSplitRdSkip));
        H1("   --nr-intra <integer>          An integer value in range of 0 to 2000, which denotes strength of noise reduction in intra CUs. Default 0\n");
        H1("   --nr-inter <integer>          An integer value in range of 0 to 2000, which denotes strength of noise reduction in inter CUs. Default 0\n");
        H0("   --ctu-info <integer>          Enable receiving ctu information asynchronously and determine reaction to the CTU information (0, 1, 2, 4, 6) Default 0\n"
            "                                    - 1: force the partitions if CTU information is present\n"
            "                                    - 2: functionality of (1) and reduce qp if CTU information has changed\n"
            "                                    - 4: functionality of (1) and force Inter modes when CTU Information has changed, merge/skip otherwise\n"
            "                                    Enable this option only when planning to invoke the API function x265_encoder_ctu_info to copy ctu-info asynchronously\n");
        H0("\nCoding tools:\n");
        H0("-w/--[no-]weightp                Enable weighted prediction in P slices. Default %s\n", OPT(param->bEnableWeightedPred));
        H0("   --[no-]weightb                Enable weighted prediction in B slices. Default %s\n", OPT(param->bEnableWeightedBiPred));
        H0("   --[no-]cu-lossless            Consider lossless mode in CU RDO decisions. Default %s\n", OPT(param->bCULossless));
        H0("   --[no-]signhide               Hide sign bit of one coeff per TU (rdo). Default %s\n", OPT(param->bEnableSignHiding));
        H1("   --[no-]tskip                  Enable intra 4x4 transform skipping. Default %s\n", OPT(param->bEnableTransformSkip));
        H0("\nTemporal / motion search options:\n");
        H0("   --max-merge <1..5>            Maximum number of merge candidates. Default %d\n", param->maxNumMergeCand);
        H0("   --ref <integer>               max number of L0 references to be allowed (1 .. 16) Default %d\n", param->maxNumReferences);
        H0("   --limit-refs <0|1|2|3>        Limit references per depth (1) or CU (2) or both (3). Default %d\n", param->limitReferences);
        H0("   --me <string>                 Motion search method dia hex umh star full. Default %d\n", param->searchMethod);
        H0("-m/--subme <integer>             Amount of subpel refinement to perform (0:least .. 7:most). Default %d \n", param->subpelRefine);
        H0("   --merange <integer>           Motion search range. Default %d\n", param->searchRange);
        H0("   --[no-]rect                   Enable rectangular motion partitions Nx2N and 2NxN. Default %s\n", OPT(param->bEnableRectInter));
        H0("   --[no-]amp                    Enable asymmetric motion partitions, requires --rect. Default %s\n", OPT(param->bEnableAMP));
        H0("   --[no-]limit-modes            Limit rectangular and asymmetric motion predictions. Default %d\n", param->limitModes);
        H1("   --[no-]temporal-mvp           Enable temporal MV predictors. Default %s\n", OPT(param->bEnableTemporalMvp));
        H1("   --[no-]hme                    Enable Hierarchical Motion Estimation. Default %s\n", OPT(param->bEnableHME));
        H1("   --hme-search <string>         Motion search-method for HME L0,L1 and L2. Default(L0,L1,L2) is %d,%d,%d\n", param->hmeSearchMethod[0], param->hmeSearchMethod[1], param->hmeSearchMethod[2]);
        H1("   --hme-range <int>,<int>,<int> Motion search-range for HME L0,L1 and L2. Default(L0,L1,L2) is %d,%d,%d\n", param->hmeRange[0], param->hmeRange[1], param->hmeRange[2]);
        H0("\nSpatial / intra options:\n");
        H0("   --[no-]strong-intra-smoothing Enable strong intra smoothing for 32x32 blocks. Default %s\n", OPT(param->bEnableStrongIntraSmoothing));
        H0("   --[no-]constrained-intra      Constrained intra prediction (use only intra coded reference pixels) Default %s\n", OPT(param->bEnableConstrainedIntra));
        H0("   --[no-]b-intra                Enable intra in B frames in veryslow presets. Default %s\n", OPT(param->bIntraInBFrames));
        H0("   --[no-]fast-intra             Enable faster search method for angular intra predictions. Default %s\n", OPT(param->bEnableFastIntra));
        H0("   --rdpenalty <0..2>            penalty for 32x32 intra TU in non-I slices. 0:disabled 1:RD-penalty 2:maximum. Default %d\n", param->rdPenalty);
        H0("\nSlice decision options:\n");
        H0("   --[no-]open-gop               Enable open-GOP, allows I slices to be non-IDR. Default %s\n", OPT(param->bOpenGOP));
        H0("-I/--keyint <integer>            Max IDR period in frames. -1 for infinite-gop. Default %d\n", param->keyframeMax);
        H0("-i/--min-keyint <integer>        Scenecuts closer together than this are coded as I, not IDR. Default: auto\n");
        H0("   --gop-lookahead <integer>     Extends gop boundary if a scenecut is found within this from keyint boundary. Default 0\n");
        H0("   --no-scenecut                 Disable adaptive I-frame decision\n");
        H0("   --scenecut <integer>          How aggressively to insert extra I-frames. Default %d\n", param->scenecutThreshold);
        H1("   --scenecut-bias <0..100.0>    Bias for scenecut detection. Default %.2f\n", param->scenecutBias);
        H0("   --hist-scenecut               Enables histogram based scene-cut detection using histogram based algorithm.\n");
        H0("   --no-hist-scenecut            Disables histogram based scene-cut detection using histogram based algorithm.\n");
        H1("   --hist-threshold <0.0..1.0>   Luma Edge histogram's Normalized SAD threshold for histogram based scenecut detection Default %.2f\n", param->edgeTransitionThreshold);
        H0("   --[no-]fades                  Enable detection and handling of fade-in regions. Default %s\n", OPT(param->bEnableFades));
        H1("   --[no-]scenecut-aware-qp      Enable increasing QP for frames inside the scenecut window after scenecut. Default %s\n", OPT(param->bEnableSceneCutAwareQp));
        H1("   --scenecut-window <0..1000>   QP incremental duration(in milliseconds) when scenecut-aware-qp is enabled. Default %d\n", param->scenecutWindow);
        H1("   --qp-delta-ref <0..10>        QP offset to increment with base QP for inter-frames. Default %f\n", param->refQpDelta);
        H1("   --qp-delta-nonref <0..10>     QP offset to increment with base QP for non-referenced inter-frames. Default %f\n", param->nonRefQpDelta);
        H0("   --radl <integer>              Number of RADL pictures allowed in front of IDR. Default %d\n", param->radl);
        H0("   --intra-refresh               Use Periodic Intra Refresh instead of IDR frames\n");
        H0("   --rc-lookahead <integer>      Number of frames for frame-type lookahead (determines encoder latency) Default %d\n", param->lookaheadDepth);
        H1("   --lookahead-slices <0..16>    Number of slices to use per lookahead cost estimate. Default %d\n", param->lookaheadSlices);
        H0("   --lookahead-threads <integer> Number of threads to be dedicated to perform lookahead only. Default %d\n", param->lookaheadThreads);
        H0("-b/--bframes <0..16>             Maximum number of consecutive b-frames. Default %d\n", param->bframes);
        H1("   --bframe-bias <integer>       Bias towards B frame decisions. Default %d\n", param->bFrameBias);
        H0("   --b-adapt <0..2>              0 - none, 1 - fast, 2 - full (trellis) adaptive B frame scheduling. Default %d\n", param->bFrameAdaptive);
        H0("   --[no-]b-pyramid              Use B-frames as references. Default %s\n", OPT(param->bBPyramid));
        H1("   --qpfile <string>             Force frametypes and QPs for some or all frames\n");
        H1("                                 Format of each line: framenumber frametype QP\n");
        H1("                                 QP is optional (none lets x265 choose). Frametypes: I,i,K,P,B,b.\n");
        H1("                                 QPs are restricted by qpmin/qpmax.\n");
        H1("   --force-flush <integer>       Force the encoder to flush frames. Default %d\n", param->forceFlush);
        H1("                                 0 - flush the encoder only when all the input pictures are over.\n");
        H1("                                 1 - flush all the frames even when the input is not over. Slicetype decision may change with this option.\n");
        H1("                                 2 - flush the slicetype decided frames only.\n");
        H0("   --[no-]-hrd-concat            Set HRD concatenation flag for the first keyframe in the buffering period SEI. Default %s\n", OPT(param->bEnableHRDConcatFlag));
        H0("\nRate control, Adaptive Quantization:\n");
        H0("   --bitrate <integer>           Target bitrate (kbps) for ABR (implied). Default %d\n", param->rc.bitrate);
        H1("-q/--qp <integer>                QP for P slices in CQP mode (implied). --ipratio and --pbration determine other slice QPs\n");
        H0("   --crf <float>                 Quality-based VBR (0-51). Default %.1f\n", param->rc.rfConstant);
        H1("   --[no-]lossless               Enable lossless: bypass transform, quant and loop filters globally. Default %s\n", OPT(param->bLossless));
        H1("   --crf-max <float>             With CRF+VBV, limit RF to this value. Default %f\n", param->rc.rfConstantMax);
        H1("                                 May cause VBV underflows!\n");
        H1("   --crf-min <float>             With CRF+VBV, limit RF to this value. Default %f\n", param->rc.rfConstantMin);
        H1("                                 this specifies a minimum rate factor value for encode!\n");
        H0("   --vbv-maxrate <integer>       Max local bitrate (kbit/s). Default %d\n", param->rc.vbvMaxBitrate);
        H0("   --vbv-bufsize <integer>       Set size of the VBV buffer (kbit). Default %d\n", param->rc.vbvBufferSize);
        H0("   --vbv-init <float>            Initial VBV buffer occupancy (fraction of bufsize or in kbits). Default %.2f\n", param->rc.vbvBufferInit);
        H0("   --vbv-end <float>             Final VBV buffer emptiness (fraction of bufsize or in kbits). Default 0 (disabled)\n");
        H0("   --min-vbv-fullness <double>   Minimum VBV fullness percentage to be maintained. Default %.2f\n", param->minVbvFullness);
        H0("   --max-vbv-fullness <double>   Maximum VBV fullness percentage to be maintained. Default %.2f\n", param->maxVbvFullness);
        H0("   --vbv-end-fr-adj <float>      Frame from which qp has to be adjusted to achieve final decode buffer emptiness. Default 0\n");
        H0("   --chunk-start <integer>       First frame of the chunk. Default 0 (disabled)\n");
        H0("   --chunk-end <integer>         Last frame of the chunk. Default 0 (disabled)\n");
        H0("   --pass                        Multi pass rate control.\n"
            "                                   - 1 : First pass, creates stats file\n"
            "                                   - 2 : Last pass, does not overwrite stats file\n"
            "                                   - 3 : Nth pass, overwrites stats file\n");
        H0("   --[no-]multi-pass-opt-analysis   Refine analysis in 2 pass based on analysis information from pass 1\n");
        H0("   --[no-]multi-pass-opt-distortion Use distortion of CTU from pass 1 to refine qp in 2 pass\n");
        H0("   --[no-]vbv-live-multi-pass    Enable realtime VBV in rate control 2 pass.Default %s\n", OPT(param->bliveVBV2pass));
        H0("   --stats                       Filename for stats file in multipass pass rate control. Default x265_2pass.log\n");
        H0("   --[no-]analyze-src-pics       Motion estimation uses source frame planes. Default disable\n");
        H0("   --[no-]slow-firstpass         Enable a slow first pass in a multipass rate control mode. Default %s\n", OPT(param->rc.bEnableSlowFirstPass));
        H0("   --[no-]strict-cbr             Enable stricter conditions and tolerance for bitrate deviations in CBR mode. Default %s\n", OPT(param->rc.bStrictCbr));
        H0("   --analysis-save <filename>    Dump analysis info into the specified file. Default Disabled\n");
        H0("   --analysis-load <filename>    Load analysis buffers from the file specified. Default Disabled\n");
        H0("   --analysis-reuse-file <filename>    Specify file name used for either dumping or reading analysis data. Deault x265_analysis.dat\n");
        H0("   --analysis-reuse-level <1..10>      Level of analysis reuse indicates amount of info stored/reused in save/load mode, 1:least..10:most. Now deprecated. Default %d\n", param->analysisReuseLevel);
        H0("   --analysis-save-reuse-level <1..10> Indicates the amount of analysis info stored in save mode, 1:least..10:most. Default %d\n", param->analysisSaveReuseLevel);
        H0("   --analysis-load-reuse-level <1..10> Indicates the amount of analysis info reused in load mode, 1:least..10:most. Default %d\n", param->analysisLoadReuseLevel);
        H0("   --refine-analysis-type <string>     Reuse anlaysis information received through API call. Supported options are avc and hevc. Default disabled - %d\n", param->bAnalysisType);
        H0("   --scale-factor <int>          Specify factor by which input video is scaled down for analysis save mode. Default %d\n", param->scaleFactor);
        H0("   --refine-intra <0..4>         Enable intra refinement for encode that uses analysis-load.\n"
            "                                    - 0 : Forces both mode and depth from the save encode.\n"
            "                                    - 1 : Functionality of (0) + evaluate all intra modes at min-cu-size's depth when current depth is one smaller than min-cu-size's depth.\n"
            "                                    - 2 : Functionality of (1) + irrespective of size evaluate all angular modes when the save encode decides the best mode as angular.\n"
            "                                    - 3 : Functionality of (1) + irrespective of size evaluate all intra modes.\n"
            "                                    - 4 : Re-evaluate all intra blocks, does not reuse data from save encode.\n"
            "                                Default:%d\n", param->intraRefine);
        H0("   --refine-inter <0..3>         Enable inter refinement for encode that uses analysis-load.\n"
            "                                    - 0 : Forces both mode and depth from the save encode.\n"
            "                                    - 1 : Functionality of (0) + evaluate all inter modes at min-cu-size's depth when current depth is one smaller than\n"
            "                                          min-cu-size's depth. When save encode decides the current block as skip(for all sizes) evaluate skip/merge.\n"
            "                                    - 2 : Functionality of (1) + irrespective of size restrict the modes evaluated when specific modes are decided as the best mode by the save encode.\n"
            "                                    - 3 : Functionality of (1) + irrespective of size evaluate all inter modes.\n"
            "                                Default:%d\n", param->interRefine);
        H0("   --[no-]dynamic-refine         Dynamically changes refine-inter level for each CU. Default %s\n", OPT(param->bDynamicRefine));
        H0("   --refine-mv <1..3>            Enable mv refinement for load mode. Default %d\n", param->mvRefine);
        H0("   --refine-ctu-distortion       Store/normalize ctu distortion in analysis-save/load.\n"
            "                                    - 0 : Disabled.\n"
            "                                    - 1 : Store/Load ctu distortion to/from the file specified in analysis-save/load.\n"
            "                                Default 0 - Disabled\n");
        H0("   --aq-mode <integer>           Mode for Adaptive Quantization - 0:none 1:uniform AQ 2:auto variance 3:auto variance with bias to dark scenes 4:auto variance with edge information. Default %d\n", param->rc.aqMode);
        H0("   --[no-]hevc-aq                Mode for HEVC Adaptive Quantization. Default %s\n", OPT(param->rc.hevcAq));
        H0("   --aq-strength <float>         Reduces blocking and blurring in flat and textured areas (0 to 3.0). Default %.2f\n", param->rc.aqStrength);
        H0("   --qp-adaptation-range <float> Delta QP range by QP adaptation based on a psycho-visual model (1.0 to 6.0). Default %.2f\n", param->rc.qpAdaptationRange);
        H0("   --[no-]aq-motion              Block level QP adaptation based on the relative motion between the block and the frame. Default %s\n", OPT(param->bAQMotion));
        H0("   --qg-size <int>               Specifies the size of the quantization group (64, 32, 16, 8). Default %d\n", param->rc.qgSize);
        H0("   --[no-]cutree                 Enable cutree for Adaptive Quantization. Default %s\n", OPT(param->rc.cuTree));
        H0("   --[no-]rc-grain               Enable ratecontrol mode to handle grains specifically. turned on with tune grain. Default %s\n", OPT(param->rc.bEnableGrain));
        H1("   --ipratio <float>             QP factor between I and P. Default %.2f\n", param->rc.ipFactor);
        H1("   --pbratio <float>             QP factor between P and B. Default %.2f\n", param->rc.pbFactor);
        H1("   --qcomp <float>               Weight given to predicted complexity. Default %.2f\n", param->rc.qCompress);
        H1("   --qpstep <integer>            The maximum single adjustment in QP allowed to rate control. Default %d\n", param->rc.qpStep);
        H1("   --qpmin <integer>             sets a hard lower limit on QP allowed to ratecontrol. Default %d\n", param->rc.qpMin);
        H1("   --qpmax <integer>             sets a hard upper limit on QP allowed to ratecontrol. Default %d\n", param->rc.qpMax);
        H0("   --[no-]const-vbv              Enable consistent vbv. turned on with tune grain. Default %s\n", OPT(param->rc.bEnableConstVbv));
        H1("   --cbqpoffs <integer>          Chroma Cb QP Offset [-12..12]. Default %d\n", param->cbQpOffset);
        H1("   --crqpoffs <integer>          Chroma Cr QP Offset [-12..12]. Default %d\n", param->crQpOffset);
        H1("   --scaling-list <string>       Specify a file containing HM style quant scaling lists or 'default' or 'off'. Default: off\n");
        H1("   --zones <zone0>/<zone1>/...   Tweak the bitrate of regions of the video\n");
        H1("                                 Each zone is of the form\n");
        H1("                                   <start frame>,<end frame>,<option>\n");
        H1("                                   where <option> is either\n");
        H1("                                       q=<integer> (force QP)\n");
        H1("                                   or  b=<float> (bitrate multiplier)\n");
        H0("   --zonefile <filename>         Zone file containing the zone boundaries and the parameters to be reconfigured.\n");
        H1("   --lambda-file <string>        Specify a file containing replacement values for the lambda tables\n");
        H1("                                 MAX_MAX_QP+1 floats for lambda table, then again for lambda2 table\n");
        H1("                                 Blank lines and lines starting with hash(#) are ignored\n");
        H1("                                 Comma is considered to be white-space\n");
        H0("   --max-ausize-factor <float>   This value controls the maximum AU size defined in specification.\n");
        H0("                                 It represents the percentage of maximum AU size used. Default %.1f\n", param->maxAUSizeFactor);
        H0("\nLoop filters (deblock and SAO):\n");
        H0("   --[no-]deblock                Enable Deblocking Loop Filter, optionally specify tC:Beta offsets Default %s\n", OPT(param->bEnableLoopFilter));
        H0("   --[no-]sao                    Enable Sample Adaptive Offset. Default %s\n", OPT(param->bEnableSAO));
        H1("   --[no-]sao-non-deblock        Use non-deblocked pixels, else right/bottom boundary areas skipped. Default %s\n", OPT(param->bSaoNonDeblocked));
        H0("   --[no-]limit-sao              Limit Sample Adaptive Offset types. Default %s\n", OPT(param->bLimitSAO));
        H0("   --selective-sao <int>         Enable slice-level SAO filter. Default %d\n", param->selectiveSAO);
        H0("\nVUI options:\n");
        H0("   --sar <width:height|int>      Sample Aspect Ratio, the ratio of width to height of an individual pixel.\n");
        H0("                                 Choose from 0=undef, 1=1:1(\"square\"), 2=12:11, 3=10:11, 4=16:11,\n");
        H0("                                 5=40:33, 6=24:11, 7=20:11, 8=32:11, 9=80:33, 10=18:11, 11=15:11,\n");
        H0("                                 12=64:33, 13=160:99, 14=4:3, 15=3:2, 16=2:1 or custom ratio of <int:int>. Default %d\n", param->vui.aspectRatioIdc);
        H1("   --display-window <string>     Describe overscan cropping region as 'left,top,right,bottom' in pixels\n");
        H1("   --overscan <string>           Specify whether it is appropriate for decoder to show cropped region: unknown, show or crop. Default unknown\n");
        H0("   --videoformat <string>        Specify video format from unknown, component, pal, ntsc, secam, mac. Default unknown\n");
        H0("   --range <string>              Specify black level and range of luma and chroma signals as full or limited Default limited\n");
        H0("   --colorprim <string>          Specify color primaries from  bt709, unknown, reserved, bt470m, bt470bg, smpte170m,\n");
        H0("                                 smpte240m, film, bt2020, smpte428, smpte431, smpte432. Default unknown\n");
        H0("   --transfer <string>           Specify transfer characteristics from bt709, unknown, reserved, bt470m, bt470bg, smpte170m,\n");
        H0("                                 smpte240m, linear, log100, log316, iec61966-2-4, bt1361e, iec61966-2-1,\n");
        H0("                                 bt2020-10, bt2020-12, smpte2084, smpte428, arib-std-b67. Default unknown\n");
        H1("   --colormatrix <string>        Specify color matrix setting from unknown, bt709, fcc, bt470bg, smpte170m,\n");
        H1("                                 smpte240m, gbr, ycgco, bt2020nc, bt2020c, smpte2085, chroma-derived-nc, chroma-derived-c, ictcp. Default unknown\n");
        H1("   --chromaloc <integer>         Specify chroma sample location (0 to 5). Default of %d\n", param->vui.chromaSampleLocTypeTopField);
        H0("   --master-display <string>     SMPTE ST 2086 master display color volume info SEI (HDR)\n");
        H0("                                    format: G(x,y)B(x,y)R(x,y)WP(x,y)L(max,min)\n");
        H0("   --max-cll <string>            Specify content light level info SEI as \"cll,fall\" (HDR).\n");
        H0("   --[no-]cll                    Emit content light level info SEI. Default %s\n", OPT(param->bEmitCLL));
        H0("   --[no-]hdr10                  Control dumping of HDR10 SEI packet. If max-cll or master-display has non-zero values, this is enabled. Default %s\n", OPT(param->bEmitHDR10SEI));
        H0("   --[no-]hdr-opt                Add luma and chroma offsets for HDR/WCG content. Default %s. Now deprecated.\n", OPT(param->bHDROpt));
        H0("   --[no-]hdr10-opt              Block-level QP optimization for HDR10 content. Default %s.\n", OPT(param->bHDR10Opt));
        H0("   --min-luma <integer>          Minimum luma plane value of input source picture\n");
        H0("   --max-luma <integer>          Maximum luma plane value of input source picture\n");
        H0("\nBitstream options:\n");
        H0("   --[no-]repeat-headers         Emit SPS and PPS headers at each keyframe. Default %s\n", OPT(param->bRepeatHeaders));
        H0("   --[no-]info                   Emit SEI identifying encoder and parameters. Default %s\n", OPT(param->bEmitInfoSEI));
        H0("   --[no-]hrd                    Enable HRD parameters signaling. Default %s\n", OPT(param->bEmitHRDSEI));
        H0("   --[no-]idr-recovery-sei      Emit recovery point infor SEI at each IDR frame \n");
        H0("   --[no-]temporal-layers        Enable a temporal sublayer for unreferenced B frames. Default %s\n", OPT(param->bEnableTemporalSubLayers));
        H0("   --[no-]aud                    Emit access unit delimiters at the start of each access unit. Default %s\n", OPT(param->bEnableAccessUnitDelimiters));
        H1("   --hash <integer>              Decoded Picture Hash SEI 0: disabled, 1: MD5, 2: CRC, 3: Checksum. Default %d\n", param->decodedPictureHashSEI);
        H0("   --atc-sei <integer>           Emit the alternative transfer characteristics SEI message where the integer is the preferred transfer characteristics. Default disabled\n");
        H0("   --pic-struct <integer>        Set the picture structure and emits it in the picture timing SEI message. Values in the range 0..12. See D.3.3 of the HEVC spec. for a detailed explanation.\n");
        H0("   --log2-max-poc-lsb <integer>  Maximum of the picture order count\n");
        H0("   --[no-]vui-timing-info        Emit VUI timing information in the bistream. Default %s\n", OPT(param->bEmitVUITimingInfo));
        H0("   --[no-]vui-hrd-info           Emit VUI HRD information in the bistream. Default %s\n", OPT(param->bEmitVUIHRDInfo));
        H0("   --[no-]opt-qp-pps             Dynamically optimize QP in PPS (instead of default 26) based on QPs in previous GOP. Default %s\n", OPT(param->bOptQpPPS));
        H0("   --[no-]opt-ref-list-length-pps  Dynamically set L0 and L1 ref list length in PPS (instead of default 0) based on values in last GOP. Default %s\n", OPT(param->bOptRefListLengthPPS));
        H0("   --[no-]multi-pass-opt-rps     Enable storing commonly used RPS in SPS in multi pass mode. Default %s\n", OPT(param->bMultiPassOptRPS));
        H0("   --[no-]opt-cu-delta-qp        Optimize to signal consistent CU level delta QPs in frame. Default %s\n", OPT(param->bOptCUDeltaQP));
        H1("\nReconstructed video options (debugging):\n");
        H1("-r/--recon <filename>            Reconstructed raw image YUV or Y4M output file name\n");
        H1("   --recon-depth <integer>       Bit-depth of reconstructed raw image file. Defaults to input bit depth, or 8 if Y4M\n");
        H1("   --recon-y4m-exec <string>     pipe reconstructed frames to Y4M viewer, ex:\"ffplay -i pipe:0 -autoexit\"\n");
        H0("   --lowpass-dct                 Use low-pass subband dct approximation. Default %s\n", OPT(param->bLowPassDct));
        H0("   --[no-]frame-dup              Enable Frame duplication. Default %s\n", OPT(param->bEnableFrameDuplication));
        H0("   --dup-threshold <integer>     PSNR threshold for Frame duplication. Default %d\n", param->dupThreshold);
#ifdef SVT_HEVC
        H0("   --[no]svt                     Enable SVT HEVC encoder %s\n", OPT(param->bEnableSvtHevc));
        H0("   --[no-]svt-hme                Enable Hierarchial motion estimation(HME) in SVT HEVC encoder \n");
        H0("   --svt-search-width            Motion estimation search area width for SVT HEVC encoder \n");
        H0("   --svt-search-height           Motion estimation search area height for SVT HEVC encoder \n");
        H0("   --[no-]svt-compressed-ten-bit-format  Enable 8+2 encoding mode for 10bit input in SVT HEVC encoder \n");
        H0("   --[no-]svt-speed-control      Enable speed control functionality to achieve real time encoding speed for  SVT HEVC encoder \n");
        H0("   --svt-preset-tuner            Enable additional faster presets of SVT; This only has to be used on top of x265's ultrafast preset. Accepts values in the range of 0-2 \n");
        H0("   --svt-hierarchical-level      Hierarchical layer for SVT-HEVC encoder; Accepts inputs in the range 0-3 \n");
        H0("   --svt-base-layer-switch-mode  Select whether B/P slice should be used in base layer for SVT-HEVC encoder. 0-Use B-frames; 1-Use P frames in the base layer \n");
        H0("   --svt-pred-struct             Select pred structure for SVT HEVC encoder;  Accepts inputs in the range 0-2 \n");
        H0("   --[no-]svt-fps-in-vps         Enable VPS timing info for SVT HEVC encoder  \n");
#endif
        H0(" ABR-ladder settings\n");
        H0("   --abr-ladder <file>           File containing config settings required for the generation of ABR-ladder\n");
        H1("\nExecutable return codes:\n");
        H1("    0 - encode successful\n");
        H1("    1 - unable to parse command line\n");
        H1("    2 - unable to open encoder\n");
        H1("    3 - unable to generate stream headers\n");
        H1("    4 - encoder abort\n");
#undef OPT
#undef H0
#undef H1
        if (level < X265_LOG_DEBUG)
            printf("\nUse --fullhelp for a full listing (or --log-level full --help)\n");
        printf("\n\nComplete documentation may be found at http://x265.readthedocs.org/en/default/cli.html\n");
        exit(1);
    }

    void CLIOptions::destroy()
    {
        if (isAbrLadderConfig)
        {
            for (int idx = 1; idx < argCnt; idx++)
                free(argString[idx]);
            free(argString);
        }

        if (input)
            input->release();
        input = NULL;
        if (recon)
            recon->release();
        recon = NULL;
        if (qpfile)
            fclose(qpfile);
        qpfile = NULL;
        if (zoneFile)
            fclose(zoneFile);
        zoneFile = NULL;
        if (dolbyVisionRpu)
            fclose(dolbyVisionRpu);
        dolbyVisionRpu = NULL;
        if (output)
            output->release();
        output = NULL;
    }

    void CLIOptions::printStatus(uint32_t frameNum)
    {
        char buf[200];
        int64_t time = x265_mdate();

        if (!bProgress || !frameNum || (prevUpdateTime && time - prevUpdateTime < UPDATE_INTERVAL))
            return;

        int64_t elapsed = time - startTime;
        double fps = elapsed > 0 ? frameNum * 1000000. / elapsed : 0;
        float bitrate = 0.008f * totalbytes * (param->fpsNum / param->fpsDenom) / ((float)frameNum);
        if (framesToBeEncoded)
        {
            int eta = (int)(elapsed * (framesToBeEncoded - frameNum) / ((int64_t)frameNum * 1000000));
            sprintf(buf, "x265 [%.1f%%] %d/%d frames, %.2f fps, %.2f kb/s, eta %d:%02d:%02d",
                100. * frameNum / (param->chunkEnd ? param->chunkEnd : param->totalFrames), frameNum, (param->chunkEnd ? param->chunkEnd : param->totalFrames), fps, bitrate,
                eta / 3600, (eta / 60) % 60, eta % 60);
        }
        else
            sprintf(buf, "x265 %d frames: %.2f fps, %.2f kb/s", frameNum, fps, bitrate);

        fprintf(stderr, "%s  \r", buf + 5);
        SetConsoleTitle(buf);
        fflush(stderr); // needed in windows
        prevUpdateTime = time;
    }

    bool CLIOptions::parseZoneParam(int argc, char **argv, x265_param* globalParam, int zonefileCount)
    {
        bool bError = false;
        int bShowHelp = false;
        int outputBitDepth = 0;
        const char *profile = NULL;

        /* Presets are applied before all other options. */
        for (optind = 0;;)
        {
            int c = getopt_long(argc, argv, short_options, long_options, NULL);
            if (c == -1)
                break;
            else if (c == 'D')
                outputBitDepth = atoi(optarg);
            else if (c == 'P')
                profile = optarg;
            else if (c == '?')
                bShowHelp = true;
        }

        if (!outputBitDepth && profile)
        {
            /* try to derive the output bit depth from the requested profile */
            if (strstr(profile, "10"))
                outputBitDepth = 10;
            else if (strstr(profile, "12"))
                outputBitDepth = 12;
            else
                outputBitDepth = 8;
        }

        api = x265_api_get(outputBitDepth);
        if (!api)
        {
            x265_log(NULL, X265_LOG_WARNING, "falling back to default bit-depth\n");
            api = x265_api_get(0);
        }

        if (bShowHelp)
        {
            printVersion(globalParam, api);
            showHelp(globalParam);
        }

        globalParam->rc.zones[zonefileCount].zoneParam = api->param_alloc();
        if (!globalParam->rc.zones[zonefileCount].zoneParam)
        {
            x265_log(NULL, X265_LOG_ERROR, "param alloc failed\n");
            return true;
        }

        memcpy(globalParam->rc.zones[zonefileCount].zoneParam, globalParam, sizeof(x265_param));

        for (optind = 0;;)
        {
            int long_options_index = -1;
            int c = getopt_long(argc, argv, short_options, long_options, &long_options_index);
            if (c == -1)
                break;

            if (long_options_index < 0 && c > 0)
            {
                for (size_t i = 0; i < sizeof(long_options) / sizeof(long_options[0]); i++)
                {
                    if (long_options[i].val == c)
                    {
                        long_options_index = (int)i;
                        break;
                    }
                }

                if (long_options_index < 0)
                {
                    /* getopt_long might have already printed an error message */
                    if (c != 63)
                        x265_log(NULL, X265_LOG_WARNING, "internal error: short option '%c' has no long option\n", c);
                    return true;
                }
            }
            if (long_options_index < 0)
            {
                x265_log(NULL, X265_LOG_WARNING, "short option '%c' unrecognized\n", c);
                return true;
            }

            bError |= !!api->zone_param_parse(globalParam->rc.zones[zonefileCount].zoneParam, long_options[long_options_index].name, optarg);

            if (bError)
            {
                const char *name = long_options_index > 0 ? long_options[long_options_index].name : argv[optind - 2];
                x265_log(NULL, X265_LOG_ERROR, "invalid argument: %s = %s\n", name, optarg);
                return true;
            }
        }

        if (optind < argc)
        {
            x265_log(param, X265_LOG_WARNING, "extra unused command arguments given <%s>\n", argv[optind]);
            return true;
        }
        return false;
    }

    bool CLIOptions::parse(int argc, char **argv)
    {
        bool bError = false;
        int bShowHelp = false;
        int inputBitDepth = 8;
        int outputBitDepth = 0;
        int reconFileBitDepth = 0;
        const char *inputfn = NULL;
        const char *reconfn = NULL;
        const char *outputfn = NULL;
        const char *preset = NULL;
        const char *tune = NULL;
        const char *profile = NULL;
        int svtEnabled = 0;
        argCnt = argc;
        argString = argv;

        if (argc <= 1)
        {
            x265_log(NULL, X265_LOG_ERROR, "No input file. Run x265 --help for a list of options.\n");
            return true;
        }

        /* Presets are applied before all other options. */
        for (optind = 0;;)
        {
            int optionsIndex = -1;
            int c = getopt_long(argc, argv, short_options, long_options, &optionsIndex);
            if (c == -1)
                break;
            else if (c == 'p')
                preset = optarg;
            else if (c == 't')
                tune = optarg;
            else if (c == 'D')
                outputBitDepth = atoi(optarg);
            else if (c == 'P')
                profile = optarg;
            else if (c == '?')
                bShowHelp = true;
            else if (!c && !strcmp(long_options[optionsIndex].name, "svt"))
                svtEnabled = 1;
        }

        if (!outputBitDepth && profile)
        {
            /* try to derive the output bit depth from the requested profile */
            if (strstr(profile, "10"))
                outputBitDepth = 10;
            else if (strstr(profile, "12"))
                outputBitDepth = 12;
            else
                outputBitDepth = 8;
        }

        api = x265_api_get(outputBitDepth);
        if (!api)
        {
            x265_log(NULL, X265_LOG_WARNING, "falling back to default bit-depth\n");
            api = x265_api_get(0);
        }

        param = api->param_alloc();
        if (!param)
        {
            x265_log(NULL, X265_LOG_ERROR, "param alloc failed\n");
            return true;
        }
#if ENABLE_LIBVMAF
        vmafData = (x265_vmaf_data*)x265_malloc(sizeof(x265_vmaf_data));
        if (!vmafData)
        {
            x265_log(NULL, X265_LOG_ERROR, "vmaf data alloc failed\n");
            return true;
        }
#endif

        if (api->param_default_preset(param, preset, tune) < 0)
        {
            x265_log(NULL, X265_LOG_ERROR, "preset or tune unrecognized\n");
            return true;
        }

        if (bShowHelp)
        {
            printVersion(param, api);
            showHelp(param);
        }

        //Set enable SVT-HEVC encoder first if found in the command line
        if (svtEnabled) api->param_parse(param, "svt", NULL);

        for (optind = 0;;)
        {
            int long_options_index = -1;
            int c = getopt_long(argc, argv, short_options, long_options, &long_options_index);
            if (c == -1)
                break;

            switch (c)
            {
            case 'h':
                printVersion(param, api);
                showHelp(param);
                break;

            case 'V':
                printVersion(param, api);
                x265_report_simd(param);
                exit(0);

            default:
                if (long_options_index < 0 && c > 0)
                {
                    for (size_t i = 0; i < sizeof(long_options) / sizeof(long_options[0]); i++)
                    {
                        if (long_options[i].val == c)
                        {
                            long_options_index = (int)i;
                            break;
                        }
                    }

                    if (long_options_index < 0)
                    {
                        /* getopt_long might have already printed an error message */
                        if (c != 63)
                            x265_log(NULL, X265_LOG_WARNING, "internal error: short option '%c' has no long option\n", c);
                        return true;
                    }
                }
                if (long_options_index < 0)
                {
                    x265_log(NULL, X265_LOG_WARNING, "short option '%c' unrecognized\n", c);
                    return true;
                }
#define OPT(longname) \
                                            else if (!strcmp(long_options[long_options_index].name, longname))
#define OPT2(name1, name2) \
                                            else if (!strcmp(long_options[long_options_index].name, name1) || \
             !strcmp(long_options[long_options_index].name, name2))

                if (0);
                OPT2("frame-skip", "seek") this->seek = (uint32_t)x265_atoi(optarg, bError);
                OPT("frames") this->framesToBeEncoded = (uint32_t)x265_atoi(optarg, bError);
                OPT("no-progress") this->bProgress = false;
                OPT("output") outputfn = optarg;
                OPT("input") inputfn = optarg;
                OPT("recon") reconfn = optarg;
                OPT("input-depth") inputBitDepth = (uint32_t)x265_atoi(optarg, bError);
                OPT("dither") this->bDither = true;
                OPT("recon-depth") reconFileBitDepth = (uint32_t)x265_atoi(optarg, bError);
                OPT("y4m") this->bForceY4m = true;
                OPT("profile") /* handled above */;
                OPT("preset")  /* handled above */;
                OPT("tune")    /* handled above */;
                OPT("output-depth")   /* handled above */;
                OPT("recon-y4m-exec") reconPlayCmd = optarg;
                OPT("svt")    /* handled above */;
                OPT("qpfile")
                {
                    this->qpfile = x265_fopen(optarg, "rb");
                    if (!this->qpfile)
                        x265_log_file(param, X265_LOG_ERROR, "%s qpfile not found or error in opening qp file\n", optarg);
                }
                OPT("dolby-vision-rpu")
                {
                    this->dolbyVisionRpu = x265_fopen(optarg, "rb");
                    if (!this->dolbyVisionRpu)
                    {
                        x265_log_file(param, X265_LOG_ERROR, "Dolby Vision RPU metadata file %s not found or error in opening file\n", optarg);
                        return true;
                    }
                }
                OPT("zonefile")
                {
                    this->zoneFile = x265_fopen(optarg, "rb");
                    if (!this->zoneFile)
                        x265_log_file(param, X265_LOG_ERROR, "%s zone file not found or error in opening zone file\n", optarg);
                }
                OPT("fullhelp")
                {
                    param->logLevel = X265_LOG_FULL;
                    printVersion(param, api);
                    showHelp(param);
                    break;
                }
                else
                    bError |= !!api->param_parse(param, long_options[long_options_index].name, optarg);
                if (bError)
                {
                    const char *name = long_options_index > 0 ? long_options[long_options_index].name : argv[optind - 2];
                    x265_log(NULL, X265_LOG_ERROR, "invalid argument: %s = %s\n", name, optarg);
                    return true;
                }
#undef OPT
            }
        }

        if (optind < argc && !inputfn)
            inputfn = argv[optind++];
        if (optind < argc && !outputfn)
            outputfn = argv[optind++];
        if (optind < argc)
        {
            x265_log(param, X265_LOG_WARNING, "extra unused command arguments given <%s>\n", argv[optind]);
            return true;
        }

        if (argc <= 1)
        {
            api->param_default(param);
            printVersion(param, api);
            showHelp(param);
        }

        if (!inputfn || !outputfn)
        {
            x265_log(param, X265_LOG_ERROR, "input or output file not specified, try --help for help\n");
            return true;
        }

        if (param->internalBitDepth != api->bit_depth)
        {
            x265_log(param, X265_LOG_ERROR, "Only bit depths of %d are supported in this build\n", api->bit_depth);
            return true;
        }

#ifdef SVT_HEVC
        if (svtEnabled)
        {
            EB_H265_ENC_CONFIGURATION* svtParam = (EB_H265_ENC_CONFIGURATION*)param->svtHevcParam;
            param->sourceWidth = svtParam->sourceWidth;
            param->sourceHeight = svtParam->sourceHeight;
            param->fpsNum = svtParam->frameRateNumerator;
            param->fpsDenom = svtParam->frameRateDenominator;
            svtParam->encoderBitDepth = inputBitDepth;
        }
#endif

        InputFileInfo info;
        info.filename = inputfn;
        info.depth = inputBitDepth;
        info.csp = param->internalCsp;
        info.width = param->sourceWidth;
        info.height = param->sourceHeight;
        info.fpsNum = param->fpsNum;
        info.fpsDenom = param->fpsDenom;
        info.sarWidth = param->vui.sarWidth;
        info.sarHeight = param->vui.sarHeight;
        info.skipFrames = seek;
        info.frameCount = 0;
        getParamAspectRatio(param, info.sarWidth, info.sarHeight);


        this->input = InputFile::open(info, this->bForceY4m);
        if (!this->input || this->input->isFail())
        {
            x265_log_file(param, X265_LOG_ERROR, "unable to open input file <%s>\n", inputfn);
            return true;
        }

        if (info.depth < 8 || info.depth > 16)
        {
            x265_log(param, X265_LOG_ERROR, "Input bit depth (%d) must be between 8 and 16\n", inputBitDepth);
            return true;
        }

        /* Unconditionally accept height/width/csp/bitDepth from file info */
        param->sourceWidth = info.width;
        param->sourceHeight = info.height;
        param->internalCsp = info.csp;
        param->sourceBitDepth = info.depth;

        /* Accept fps and sar from file info if not specified by user */
        if (param->fpsDenom == 0 || param->fpsNum == 0)
        {
            param->fpsDenom = info.fpsDenom;
            param->fpsNum = info.fpsNum;
        }
        if (!param->vui.aspectRatioIdc && info.sarWidth && info.sarHeight)
            setParamAspectRatio(param, info.sarWidth, info.sarHeight);
        if (this->framesToBeEncoded == 0 && info.frameCount > (int)seek)
            this->framesToBeEncoded = info.frameCount - seek;
        param->totalFrames = this->framesToBeEncoded;

#ifdef SVT_HEVC
        if (svtEnabled)
        {
            EB_H265_ENC_CONFIGURATION* svtParam = (EB_H265_ENC_CONFIGURATION*)param->svtHevcParam;
            svtParam->sourceWidth = param->sourceWidth;
            svtParam->sourceHeight = param->sourceHeight;
            svtParam->frameRateNumerator = param->fpsNum;
            svtParam->frameRateDenominator = param->fpsDenom;
            svtParam->framesToBeEncoded = param->totalFrames;
            svtParam->encoderColorFormat = (EB_COLOR_FORMAT)param->internalCsp;
        }
#endif

        /* Force CFR until we have support for VFR */
        info.timebaseNum = param->fpsDenom;
        info.timebaseDenom = param->fpsNum;

        if (param->bField && param->interlaceMode)
        {   // Field FPS
            param->fpsNum *= 2;
            // Field height
            param->sourceHeight = param->sourceHeight >> 1;
            // Number of fields to encode
            param->totalFrames *= 2;
        }

        if (api->param_apply_profile(param, profile))
            return true;

        if (param->logLevel >= X265_LOG_INFO)
        {
            char buf[128];
            int p = sprintf(buf, "%dx%d fps %d/%d %sp%d", param->sourceWidth, param->sourceHeight,
                param->fpsNum, param->fpsDenom, x265_source_csp_names[param->internalCsp], info.depth);

            int width, height;
            getParamAspectRatio(param, width, height);
            if (width && height)
                p += sprintf(buf + p, " sar %d:%d", width, height);

            if (framesToBeEncoded <= 0 || info.frameCount <= 0)
                strcpy(buf + p, " unknown frame count");
            else
                sprintf(buf + p, " frames %u - %d of %d", this->seek, this->seek + this->framesToBeEncoded - 1, info.frameCount);

            general_log(param, input->getName(), X265_LOG_INFO, "%s\n", buf);
        }

        this->input->startReader();

        if (reconfn)
        {
            if (reconFileBitDepth == 0)
                reconFileBitDepth = param->internalBitDepth;
            this->recon = ReconFile::open(reconfn, param->sourceWidth, param->sourceHeight, reconFileBitDepth,
                param->fpsNum, param->fpsDenom, param->internalCsp);
            if (this->recon->isFail())
            {
                x265_log(param, X265_LOG_WARNING, "unable to write reconstructed outputs file\n");
                this->recon->release();
                this->recon = 0;
            }
            else
                general_log(param, this->recon->getName(), X265_LOG_INFO,
                "reconstructed images %dx%d fps %d/%d %s\n",
                param->sourceWidth, param->sourceHeight, param->fpsNum, param->fpsDenom,
                x265_source_csp_names[param->internalCsp]);
        }
#if ENABLE_LIBVMAF
        if (!reconfn)
        {
            x265_log(param, X265_LOG_ERROR, "recon file must be specified to get VMAF score, try --help for help\n");
            return true;
        }
        const char *str = strrchr(info.filename, '.');

        if (!strcmp(str, ".y4m"))
        {
            x265_log(param, X265_LOG_ERROR, "VMAF supports YUV file format only.\n");
            return true;
        }
        if (param->internalCsp == X265_CSP_I420 || param->internalCsp == X265_CSP_I422 || param->internalCsp == X265_CSP_I444)
        {
            vmafData->reference_file = x265_fopen(inputfn, "rb");
            vmafData->distorted_file = x265_fopen(reconfn, "rb");
        }
        else
        {
            x265_log(param, X265_LOG_ERROR, "VMAF will support only yuv420p, yu422p, yu444p, yuv420p10le, yuv422p10le, yuv444p10le formats.\n");
            return true;
        }
#endif
        this->output = OutputFile::open(outputfn, info);
        if (this->output->isFail())
        {
            x265_log_file(param, X265_LOG_ERROR, "failed to open output file <%s> for writing\n", outputfn);
            return true;
        }
        general_log_file(param, this->output->getName(), X265_LOG_INFO, "output file: %s\n", outputfn);
        return false;
    }

    bool CLIOptions::parseQPFile(x265_picture &pic_org)
    {
        int32_t num = -1, qp, ret;
        char type;
        uint32_t filePos;
        pic_org.forceqp = 0;
        pic_org.sliceType = X265_TYPE_AUTO;
        while (num < pic_org.poc)
        {
            filePos = ftell(qpfile);
            qp = -1;
            ret = fscanf(qpfile, "%d %c%*[ \t]%d\n", &num, &type, &qp);

            if (num > pic_org.poc || ret == EOF)
            {
                fseek(qpfile, filePos, SEEK_SET);
                break;
            }
            if (num < pic_org.poc && ret >= 2)
                continue;
            if (ret == 3 && qp >= 0)
                pic_org.forceqp = qp + 1;
            if (type == 'I') pic_org.sliceType = X265_TYPE_IDR;
            else if (type == 'i') pic_org.sliceType = X265_TYPE_I;
            else if (type == 'K') pic_org.sliceType = param->bOpenGOP ? X265_TYPE_I : X265_TYPE_IDR;
            else if (type == 'P') pic_org.sliceType = X265_TYPE_P;
            else if (type == 'B') pic_org.sliceType = X265_TYPE_BREF;
            else if (type == 'b') pic_org.sliceType = X265_TYPE_B;
            else ret = 0;
            if (ret < 2 || qp < -1 || qp > 51)
                return 0;
        }
        return 1;
    }

    bool CLIOptions::parseZoneFile()
    {
        char line[256];
        char* argLine;
        param->rc.zonefileCount = 0;

        while (fgets(line, sizeof(line), zoneFile))
        {
            if (!((*line == '#') || (strcmp(line, "\r\n") == 0)))
                param->rc.zonefileCount++;
        }

        rewind(zoneFile);
        param->rc.zones = X265_MALLOC(x265_zone, param->rc.zonefileCount);
        for (int i = 0; i < param->rc.zonefileCount; i++)
        {
            while (fgets(line, sizeof(line), zoneFile))
            {
                if (*line == '#' || (strcmp(line, "\r\n") == 0))
                    continue;
                param->rc.zones[i].zoneParam = X265_MALLOC(x265_param, 1);
                int index = (int)strcspn(line, "\r\n");
                line[index] = '\0';
                argLine = line;
                while (isspace((unsigned char)*argLine)) argLine++;
                char* start = strchr(argLine, ' ');
                start++;
                param->rc.zones[i].startFrame = atoi(argLine);
                int argCount = 0;
                char **args = (char**)malloc(256 * sizeof(char *));
                // Adding a dummy string to avoid file parsing error
                args[argCount++] = (char *)"x265";
                char* token = strtok(start, " ");
                while (token)
                {
                    args[argCount++] = token;
                    token = strtok(NULL, " ");
                }
                args[argCount] = NULL;
                CLIOptions cliopt;
                if (cliopt.parseZoneParam(argCount, args, param, i))
                {
                    cliopt.destroy();
                    if (cliopt.api)
                        cliopt.api->param_free(cliopt.param);
                    exit(1);
                }
                break;
            }
        }
        return 1;
    }

    /* Parse the RPU file and extract the RPU corresponding to the current picture
    * and fill the rpu field of the input picture */
    int CLIOptions::rpuParser(x265_picture * pic)
    {
        uint8_t byteVal;
        uint32_t code = 0;
        int bytesRead = 0;
        pic->rpu.payloadSize = 0;

        if (!pic->pts)
        {
            while (bytesRead++ < 4 && fread(&byteVal, sizeof(uint8_t), 1, dolbyVisionRpu))
                code = (code << 8) | byteVal;

            if (code != START_CODE)
            {
                x265_log(NULL, X265_LOG_ERROR, "Invalid Dolby Vision RPU startcode in POC %d\n", pic->pts);
                return 1;
            }
        }

        bytesRead = 0;
        while (fread(&byteVal, sizeof(uint8_t), 1, dolbyVisionRpu))
        {
            code = (code << 8) | byteVal;
            if (bytesRead++ < 3)
                continue;
            if (bytesRead >= 1024)
            {
                x265_log(NULL, X265_LOG_ERROR, "Invalid Dolby Vision RPU size in POC %d\n", pic->pts);
                return 1;
            }

            if (code != START_CODE)
                pic->rpu.payload[pic->rpu.payloadSize++] = (code >> (3 * 8)) & 0xFF;
            else
                return 0;
        }

        int ShiftBytes = START_CODE_BYTES - (bytesRead - pic->rpu.payloadSize);
        int bytesLeft = bytesRead - pic->rpu.payloadSize;
        code = (code << ShiftBytes * 8);
        for (int i = 0; i < bytesLeft; i++)
        {
            pic->rpu.payload[pic->rpu.payloadSize++] = (code >> (3 * 8)) & 0xFF;
            code = (code << 8);
        }
        if (!pic->rpu.payloadSize)
            x265_log(NULL, X265_LOG_WARNING, "Dolby Vision RPU not found for POC %d\n", pic->pts);
        return 0;
    }

#ifdef __cplusplus
}
#endif