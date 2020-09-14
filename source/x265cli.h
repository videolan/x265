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

#ifndef X265CLI_H
#define X265CLI_H 1

#include "common.h"
#include "param.h"
#include "input/input.h"
#include "output/output.h"
#include "output/reconplay.h"

#include <getopt.h>

#define CONSOLE_TITLE_SIZE 200
#ifdef _WIN32
#include <windows.h>
#define SetThreadExecutionState(es)
static char orgConsoleTitle[CONSOLE_TITLE_SIZE] = "";
#else
#define GetConsoleTitle(t, n)
#define SetConsoleTitle(t)
#define SetThreadExecutionState(es)
#endif

#ifdef __cplusplus
namespace X265_NS {
#endif

static const char short_options[] = "o:D:P:p:f:F:r:I:i:b:s:t:q:m:hwV?";
static const struct option long_options[] =
{
    { "help",                 no_argument, NULL, 'h' },
    { "fullhelp",             no_argument, NULL, 0 },
    { "version",              no_argument, NULL, 'V' },
    { "asm",            required_argument, NULL, 0 },
    { "no-asm",               no_argument, NULL, 0 },
    { "pools",          required_argument, NULL, 0 },
    { "numa-pools",     required_argument, NULL, 0 },
    { "preset",         required_argument, NULL, 'p' },
    { "tune",           required_argument, NULL, 't' },
    { "frame-threads",  required_argument, NULL, 'F' },
    { "no-pmode",             no_argument, NULL, 0 },
    { "pmode",                no_argument, NULL, 0 },
    { "no-pme",               no_argument, NULL, 0 },
    { "pme",                  no_argument, NULL, 0 },
    { "log-level",      required_argument, NULL, 0 },
    { "profile",        required_argument, NULL, 'P' },
    { "level-idc",      required_argument, NULL, 0 },
    { "high-tier",            no_argument, NULL, 0 },
    { "uhd-bd",               no_argument, NULL, 0 },
    { "no-high-tier",         no_argument, NULL, 0 },
    { "allow-non-conformance",no_argument, NULL, 0 },
    { "no-allow-non-conformance",no_argument, NULL, 0 },
    { "csv",            required_argument, NULL, 0 },
    { "csv-log-level",  required_argument, NULL, 0 },
    { "no-cu-stats",          no_argument, NULL, 0 },
    { "cu-stats",             no_argument, NULL, 0 },
    { "y4m",                  no_argument, NULL, 0 },
    { "no-progress",          no_argument, NULL, 0 },
    { "output",         required_argument, NULL, 'o' },
    { "output-depth",   required_argument, NULL, 'D' },
    { "input",          required_argument, NULL, 0 },
    { "input-depth",    required_argument, NULL, 0 },
    { "input-res",      required_argument, NULL, 0 },
    { "input-csp",      required_argument, NULL, 0 },
    { "interlace",      required_argument, NULL, 0 },
    { "no-interlace",         no_argument, NULL, 0 },
    { "field",                no_argument, NULL, 0 },
    { "no-field",             no_argument, NULL, 0 },
    { "fps",            required_argument, NULL, 0 },
    { "seek",           required_argument, NULL, 0 },
    { "frame-skip",     required_argument, NULL, 0 },
    { "frames",         required_argument, NULL, 'f' },
    { "recon",          required_argument, NULL, 'r' },
    { "recon-depth",    required_argument, NULL, 0 },
    { "no-wpp",               no_argument, NULL, 0 },
    { "wpp",                  no_argument, NULL, 0 },
    { "ctu",            required_argument, NULL, 's' },
    { "min-cu-size",    required_argument, NULL, 0 },
    { "max-tu-size",    required_argument, NULL, 0 },
    { "tu-intra-depth", required_argument, NULL, 0 },
    { "tu-inter-depth", required_argument, NULL, 0 },
    { "limit-tu",       required_argument, NULL, 0 },
    { "me",             required_argument, NULL, 0 },
    { "subme",          required_argument, NULL, 'm' },
    { "merange",        required_argument, NULL, 0 },
    { "max-merge",      required_argument, NULL, 0 },
    { "no-temporal-mvp",      no_argument, NULL, 0 },
    { "temporal-mvp",         no_argument, NULL, 0 },
    { "hme",                  no_argument, NULL, 0 },
    { "no-hme",               no_argument, NULL, 0 },
    { "hme-search",     required_argument, NULL, 0 },
    { "rdpenalty",      required_argument, NULL, 0 },
    { "no-rect",              no_argument, NULL, 0 },
    { "rect",                 no_argument, NULL, 0 },
    { "no-amp",               no_argument, NULL, 0 },
    { "amp",                  no_argument, NULL, 0 },
    { "no-early-skip",        no_argument, NULL, 0 },
    { "early-skip",           no_argument, NULL, 0 },
    { "rskip",                required_argument, NULL, 0 },
    { "rskip-edge-threshold", required_argument, NULL, 0 },
    { "no-fast-cbf",          no_argument, NULL, 0 },
    { "fast-cbf",             no_argument, NULL, 0 },
    { "no-tskip",             no_argument, NULL, 0 },
    { "tskip",                no_argument, NULL, 0 },
    { "no-tskip-fast",        no_argument, NULL, 0 },
    { "tskip-fast",           no_argument, NULL, 0 },
    { "cu-lossless",          no_argument, NULL, 0 },
    { "no-cu-lossless",       no_argument, NULL, 0 },
    { "no-constrained-intra", no_argument, NULL, 0 },
    { "constrained-intra",    no_argument, NULL, 0 },
    { "cip",                  no_argument, NULL, 0 },
    { "no-cip",               no_argument, NULL, 0 },
    { "fast-intra",           no_argument, NULL, 0 },
    { "no-fast-intra",        no_argument, NULL, 0 },
    { "no-open-gop",          no_argument, NULL, 0 },
    { "open-gop",             no_argument, NULL, 0 },
    { "keyint",         required_argument, NULL, 'I' },
    { "min-keyint",     required_argument, NULL, 'i' },
    { "gop-lookahead",  required_argument, NULL, 0 },
    { "scenecut",       required_argument, NULL, 0 },
    { "no-scenecut",          no_argument, NULL, 0 },
    { "scenecut-bias",  required_argument, NULL, 0 },
    { "hist-scenecut",        no_argument, NULL, 0},
    { "no-hist-scenecut",     no_argument, NULL, 0},
    { "hist-threshold", required_argument, NULL, 0},
    { "fades",                no_argument, NULL, 0 },
    { "no-fades",             no_argument, NULL, 0 },
    { "scenecut-aware-qp",    no_argument, NULL, 0 },
    { "no-scenecut-aware-qp", no_argument, NULL, 0 },
    { "scenecut-window",required_argument, NULL, 0 },
    { "qp-delta-ref",   required_argument, NULL, 0 },
    { "qp-delta-nonref",required_argument, NULL, 0 },
    { "radl",           required_argument, NULL, 0 },
    { "ctu-info",       required_argument, NULL, 0 },
    { "intra-refresh",        no_argument, NULL, 0 },
    { "rc-lookahead",   required_argument, NULL, 0 },
    { "lookahead-slices", required_argument, NULL, 0 },
    { "lookahead-threads", required_argument, NULL, 0 },
    { "bframes",        required_argument, NULL, 'b' },
    { "bframe-bias",    required_argument, NULL, 0 },
    { "b-adapt",        required_argument, NULL, 0 },
    { "no-b-adapt",           no_argument, NULL, 0 },
    { "no-b-pyramid",         no_argument, NULL, 0 },
    { "b-pyramid",            no_argument, NULL, 0 },
    { "ref",            required_argument, NULL, 0 },
    { "limit-refs",     required_argument, NULL, 0 },
    { "no-limit-modes",       no_argument, NULL, 0 },
    { "limit-modes",          no_argument, NULL, 0 },
    { "no-weightp",           no_argument, NULL, 0 },
    { "weightp",              no_argument, NULL, 'w' },
    { "no-weightb",           no_argument, NULL, 0 },
    { "weightb",              no_argument, NULL, 0 },
    { "crf",            required_argument, NULL, 0 },
    { "crf-max",        required_argument, NULL, 0 },
    { "crf-min",        required_argument, NULL, 0 },
    { "vbv-maxrate",    required_argument, NULL, 0 },
    { "vbv-bufsize",    required_argument, NULL, 0 },
    { "vbv-init",       required_argument, NULL, 0 },
    { "vbv-end",        required_argument, NULL, 0 },
    { "vbv-end-fr-adj", required_argument, NULL, 0 },
    { "chunk-start",    required_argument, NULL, 0 },
    { "chunk-end",      required_argument, NULL, 0 },
    { "bitrate",        required_argument, NULL, 0 },
    { "qp",             required_argument, NULL, 'q' },
    { "aq-mode",        required_argument, NULL, 0 },
    { "aq-strength",    required_argument, NULL, 0 },
    { "rc-grain",             no_argument, NULL, 0 },
    { "no-rc-grain",          no_argument, NULL, 0 },
    { "ipratio",        required_argument, NULL, 0 },
    { "pbratio",        required_argument, NULL, 0 },
    { "qcomp",          required_argument, NULL, 0 },
    { "qpstep",         required_argument, NULL, 0 },
    { "qpmin",          required_argument, NULL, 0 },
    { "qpmax",          required_argument, NULL, 0 },
    { "const-vbv",            no_argument, NULL, 0 },
    { "no-const-vbv",         no_argument, NULL, 0 },
    { "ratetol",        required_argument, NULL, 0 },
    { "cplxblur",       required_argument, NULL, 0 },
    { "qblur",          required_argument, NULL, 0 },
    { "cbqpoffs",       required_argument, NULL, 0 },
    { "crqpoffs",       required_argument, NULL, 0 },
    { "rd",             required_argument, NULL, 0 },
    { "rdoq-level",     required_argument, NULL, 0 },
    { "no-rdoq-level",        no_argument, NULL, 0 },
    { "dynamic-rd",     required_argument, NULL, 0 },
    { "psy-rd",         required_argument, NULL, 0 },
    { "psy-rdoq",       required_argument, NULL, 0 },
    { "no-psy-rd",            no_argument, NULL, 0 },
    { "no-psy-rdoq",          no_argument, NULL, 0 },
    { "rd-refine",            no_argument, NULL, 0 },
    { "no-rd-refine",         no_argument, NULL, 0 },
    { "scaling-list",   required_argument, NULL, 0 },
    { "lossless",             no_argument, NULL, 0 },
    { "no-lossless",          no_argument, NULL, 0 },
    { "no-signhide",          no_argument, NULL, 0 },
    { "signhide",             no_argument, NULL, 0 },
    { "no-lft",               no_argument, NULL, 0 }, /* DEPRECATED */
    { "lft",                  no_argument, NULL, 0 }, /* DEPRECATED */
    { "no-deblock",           no_argument, NULL, 0 },
    { "deblock",        required_argument, NULL, 0 },
    { "no-sao",               no_argument, NULL, 0 },
    { "selective-sao",  required_argument, NULL, 0 },
    { "sao",                  no_argument, NULL, 0 },
    { "no-sao-non-deblock",   no_argument, NULL, 0 },
    { "sao-non-deblock",      no_argument, NULL, 0 },
    { "no-ssim",              no_argument, NULL, 0 },
    { "ssim",                 no_argument, NULL, 0 },
    { "no-psnr",              no_argument, NULL, 0 },
    { "psnr",                 no_argument, NULL, 0 },
    { "hash",           required_argument, NULL, 0 },
    { "no-strong-intra-smoothing", no_argument, NULL, 0 },
    { "strong-intra-smoothing",    no_argument, NULL, 0 },
    { "no-cutree",                 no_argument, NULL, 0 },
    { "cutree",                    no_argument, NULL, 0 },
    { "no-hrd",               no_argument, NULL, 0 },
    { "hrd",                  no_argument, NULL, 0 },
    { "sar",            required_argument, NULL, 0 },
    { "overscan",       required_argument, NULL, 0 },
    { "videoformat",    required_argument, NULL, 0 },
    { "range",          required_argument, NULL, 0 },
    { "colorprim",      required_argument, NULL, 0 },
    { "transfer",       required_argument, NULL, 0 },
    { "colormatrix",    required_argument, NULL, 0 },
    { "chromaloc",      required_argument, NULL, 0 },
    { "display-window", required_argument, NULL, 0 },
    { "crop-rect",      required_argument, NULL, 0 }, /* DEPRECATED */
    { "master-display", required_argument, NULL, 0 },
    { "max-cll",        required_argument, NULL, 0 },
    { "min-luma",       required_argument, NULL, 0 },
    { "max-luma",       required_argument, NULL, 0 },
    { "log2-max-poc-lsb", required_argument, NULL, 8 },
    { "vui-timing-info",      no_argument, NULL, 0 },
    { "no-vui-timing-info",   no_argument, NULL, 0 },
    { "vui-hrd-info",         no_argument, NULL, 0 },
    { "no-vui-hrd-info",      no_argument, NULL, 0 },
    { "opt-qp-pps",           no_argument, NULL, 0 },
    { "no-opt-qp-pps",        no_argument, NULL, 0 },
    { "opt-ref-list-length-pps",         no_argument, NULL, 0 },
    { "no-opt-ref-list-length-pps",      no_argument, NULL, 0 },
    { "opt-cu-delta-qp",      no_argument, NULL, 0 },
    { "no-opt-cu-delta-qp",   no_argument, NULL, 0 },
    { "no-dither",            no_argument, NULL, 0 },
    { "dither",               no_argument, NULL, 0 },
    { "no-repeat-headers",    no_argument, NULL, 0 },
    { "repeat-headers",       no_argument, NULL, 0 },
    { "aud",                  no_argument, NULL, 0 },
    { "no-aud",               no_argument, NULL, 0 },
    { "info",                 no_argument, NULL, 0 },
    { "no-info",              no_argument, NULL, 0 },
    { "zones",          required_argument, NULL, 0 },
    { "qpfile",         required_argument, NULL, 0 },
    { "zonefile",       required_argument, NULL, 0 },
    { "lambda-file",    required_argument, NULL, 0 },
    { "b-intra",              no_argument, NULL, 0 },
    { "no-b-intra",           no_argument, NULL, 0 },
    { "nr-intra",       required_argument, NULL, 0 },
    { "nr-inter",       required_argument, NULL, 0 },
    { "stats",          required_argument, NULL, 0 },
    { "pass",           required_argument, NULL, 0 },
    { "multi-pass-opt-analysis", no_argument, NULL, 0 },
    { "no-multi-pass-opt-analysis",    no_argument, NULL, 0 },
    { "multi-pass-opt-distortion",     no_argument, NULL, 0 },
    { "no-multi-pass-opt-distortion",  no_argument, NULL, 0 },
    { "vbv-live-multi-pass",           no_argument, NULL, 0 },
    { "no-vbv-live-multi-pass",        no_argument, NULL, 0 },
    { "slow-firstpass",       no_argument, NULL, 0 },
    { "no-slow-firstpass",    no_argument, NULL, 0 },
    { "multi-pass-opt-rps",   no_argument, NULL, 0 },
    { "no-multi-pass-opt-rps", no_argument, NULL, 0 },
    { "analysis-reuse-mode", required_argument, NULL, 0 }, /* DEPRECATED */
    { "analysis-reuse-file", required_argument, NULL, 0 },
    { "analysis-reuse-level", required_argument, NULL, 0 }, /* DEPRECATED */
    { "analysis-save-reuse-level", required_argument, NULL, 0 },
    { "analysis-load-reuse-level", required_argument, NULL, 0 },
    { "analysis-save",  required_argument, NULL, 0 },
    { "analysis-load",  required_argument, NULL, 0 },
    { "scale-factor",   required_argument, NULL, 0 },
    { "refine-intra",   required_argument, NULL, 0 },
    { "refine-inter",   required_argument, NULL, 0 },
    { "dynamic-refine",       no_argument, NULL, 0 },
    { "no-dynamic-refine",    no_argument, NULL, 0 },
    { "strict-cbr",           no_argument, NULL, 0 },
    { "temporal-layers",      no_argument, NULL, 0 },
    { "no-temporal-layers",   no_argument, NULL, 0 },
    { "qg-size",        required_argument, NULL, 0 },
    { "recon-y4m-exec", required_argument, NULL, 0 },
    { "analyze-src-pics", no_argument, NULL, 0 },
    { "no-analyze-src-pics", no_argument, NULL, 0 },
    { "slices",         required_argument, NULL, 0 },
    { "aq-motion",            no_argument, NULL, 0 },
    { "no-aq-motion",         no_argument, NULL, 0 },
    { "ssim-rd",              no_argument, NULL, 0 },
    { "no-ssim-rd",           no_argument, NULL, 0 },
    { "hdr",                  no_argument, NULL, 0 },
    { "no-hdr",               no_argument, NULL, 0 },
    { "hdr10",                no_argument, NULL, 0 },
    { "no-hdr10",             no_argument, NULL, 0 },
    { "hdr-opt",              no_argument, NULL, 0 },
    { "no-hdr-opt",           no_argument, NULL, 0 },
    { "hdr10-opt",            no_argument, NULL, 0 },
    { "no-hdr10-opt",         no_argument, NULL, 0 },
    { "limit-sao",            no_argument, NULL, 0 },
    { "no-limit-sao",         no_argument, NULL, 0 },
    { "dhdr10-info",    required_argument, NULL, 0 },
    { "dhdr10-opt",           no_argument, NULL, 0},
    { "no-dhdr10-opt",        no_argument, NULL, 0},
    { "dolby-vision-profile",  required_argument, NULL, 0 },
    { "refine-mv",      required_argument, NULL, 0 },
    { "refine-ctu-distortion", required_argument, NULL, 0 },
    { "force-flush",    required_argument, NULL, 0 },
    { "splitrd-skip",         no_argument, NULL, 0 },
    { "no-splitrd-skip",      no_argument, NULL, 0 },
    { "lowpass-dct",          no_argument, NULL, 0 },
    { "refine-analysis-type", required_argument, NULL, 0 },
    { "copy-pic",             no_argument, NULL, 0 },
    { "no-copy-pic",          no_argument, NULL, 0 },
    { "max-ausize-factor", required_argument, NULL, 0 },
    { "idr-recovery-sei",     no_argument, NULL, 0 },
    { "no-idr-recovery-sei",  no_argument, NULL, 0 },
    { "single-sei", no_argument, NULL, 0 },
    { "no-single-sei", no_argument, NULL, 0 },
    { "atc-sei", required_argument, NULL, 0 },
    { "pic-struct", required_argument, NULL, 0 },
    { "nalu-file", required_argument, NULL, 0 },
    { "dolby-vision-rpu", required_argument, NULL, 0 },
    { "hrd-concat",          no_argument, NULL, 0},
    { "no-hrd-concat",       no_argument, NULL, 0 },
    { "hevc-aq", no_argument, NULL, 0 },
    { "no-hevc-aq", no_argument, NULL, 0 },
    { "qp-adaptation-range", required_argument, NULL, 0 },
    { "frame-dup",            no_argument, NULL, 0 },
    { "no-frame-dup", no_argument, NULL, 0 },
    { "dup-threshold", required_argument, NULL, 0 },
#ifdef SVT_HEVC
    { "svt",     no_argument, NULL, 0 },
    { "no-svt",  no_argument, NULL, 0 },
    { "svt-hme",     no_argument, NULL, 0 },
    { "no-svt-hme",  no_argument, NULL, 0 },
    { "svt-search-width",      required_argument, NULL, 0 },
    { "svt-search-height",     required_argument, NULL, 0 },
    { "svt-compressed-ten-bit-format",    no_argument, NULL, 0 },
    { "no-svt-compressed-ten-bit-format", no_argument, NULL, 0 },
    { "svt-speed-control",     no_argument  , NULL, 0 },
    { "no-svt-speed-control",  no_argument  , NULL, 0 },
    { "svt-preset-tuner",  required_argument  , NULL, 0 },
    { "svt-hierarchical-level",  required_argument  , NULL, 0 },
    { "svt-base-layer-switch-mode",  required_argument  , NULL, 0 },
    { "svt-pred-struct",  required_argument  , NULL, 0 },
    { "svt-fps-in-vps",  no_argument  , NULL, 0 },
    { "no-svt-fps-in-vps",  no_argument  , NULL, 0 },
#endif
    { "cll", no_argument, NULL, 0 },
    { "no-cll", no_argument, NULL, 0 },
    { "hme-range", required_argument, NULL, 0 },
    { "abr-ladder", required_argument, NULL, 0 },
    { "min-vbv-fullness", required_argument, NULL, 0 },
    { "max-vbv-fullness", required_argument, NULL, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 }
};

    struct CLIOptions
    {
        InputFile* input;
        ReconFile* recon;
        OutputFile* output;
        FILE*       qpfile;
        FILE*       zoneFile;
        FILE*    dolbyVisionRpu;    /* File containing Dolby Vision BL RPU metadata */
        const char* reconPlayCmd;
        const x265_api* api;
        x265_param* param;
        x265_vmaf_data* vmafData;
        bool bProgress;
        bool bForceY4m;
        bool bDither;
        uint32_t seek;              // number of frames to skip from the beginning
        uint32_t framesToBeEncoded; // number of frames to encode
        uint64_t totalbytes;
        int64_t startTime;
        int64_t prevUpdateTime;

        int argCnt;
        char** argString;

        /* ABR ladder settings */
        bool isAbrLadderConfig;
        bool enableScaler;
        char*    encName;
        char*    reuseName;
        uint32_t encId;
        int      refId;
        uint32_t loadLevel;
        uint32_t saveLevel;
        uint32_t numRefs;

        /* in microseconds */
        static const int UPDATE_INTERVAL = 250000;
        CLIOptions()
        {
            input = NULL;
            recon = NULL;
            output = NULL;
            qpfile = NULL;
            zoneFile = NULL;
            dolbyVisionRpu = NULL;
            reconPlayCmd = NULL;
            api = NULL;
            param = NULL;
            vmafData = NULL;
            framesToBeEncoded = seek = 0;
            totalbytes = 0;
            bProgress = true;
            bForceY4m = false;
            startTime = x265_mdate();
            prevUpdateTime = 0;
            bDither = false;
            isAbrLadderConfig = false;
            enableScaler = false;
            encName = NULL;
            reuseName = NULL;
            encId = 0;
            refId = -1;
            loadLevel = 0;
            saveLevel = 0;
            numRefs = 0;
            argCnt = 0;
        }

        void destroy();
        void printStatus(uint32_t frameNum);
        bool parse(int argc, char **argv);
        bool parseZoneParam(int argc, char **argv, x265_param* globalParam, int zonefileCount);
        bool parseQPFile(x265_picture &pic_org);
        bool parseZoneFile();
        int rpuParser(x265_picture * pic);
    };
#ifdef __cplusplus
}
#endif

#endif
