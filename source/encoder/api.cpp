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
#include "bitstream.h"
#include "param.h"

#include "encoder.h"
#include "entropy.h"
#include "level.h"
#include "nal.h"
#include "bitcost.h"

/* multilib namespace reflectors */
#if LINKED_8BIT
namespace x265_8bit {
const x265_api* x265_api_get(int bitDepth);
const x265_api* x265_api_query(int bitDepth, int apiVersion, int* err);
}
#endif

#if LINKED_10BIT
namespace x265_10bit {
const x265_api* x265_api_get(int bitDepth);
const x265_api* x265_api_query(int bitDepth, int apiVersion, int* err);
}
#endif

#if LINKED_12BIT
namespace x265_12bit {
const x265_api* x265_api_get(int bitDepth);
const x265_api* x265_api_query(int bitDepth, int apiVersion, int* err);
}
#endif

#if EXPORT_C_API
/* these functions are exported as C functions (default) */
using namespace X265_NS;
extern "C" {
#else
/* these functions exist within private namespace (multilib) */
namespace X265_NS {
#endif

x265_encoder *x265_encoder_open(x265_param *p)
{
    if (!p)
        return NULL;

#if _MSC_VER
#pragma warning(disable: 4127) // conditional expression is constant, yes I know
#endif

#if HIGH_BIT_DEPTH
    if (X265_DEPTH != 10 && X265_DEPTH != 12)
#else
    if (X265_DEPTH != 8)
#endif
    {
        x265_log(p, X265_LOG_ERROR, "Build error, internal bit depth mismatch\n");
        return NULL;
    }

    Encoder* encoder = NULL;
    x265_param* param = PARAM_NS::x265_param_alloc();
    x265_param* latestParam = PARAM_NS::x265_param_alloc();
    if (!param || !latestParam)
        goto fail;

    memcpy(param, p, sizeof(x265_param));
    x265_log(param, X265_LOG_INFO, "HEVC encoder version %s\n", PFX(version_str));
    x265_log(param, X265_LOG_INFO, "build info %s\n", PFX(build_info_str));

    x265_setup_primitives(param);

    if (x265_check_params(param))
        goto fail;

    if (x265_set_globals(param))
        goto fail;

    encoder = new Encoder;
    if (!param->rc.bEnableSlowFirstPass)
        PARAM_NS::x265_param_apply_fastfirstpass(param);

    // may change params for auto-detect, etc
    encoder->configure(param);
    // may change rate control and CPB params
    if (!enforceLevel(*param, encoder->m_vps))
        goto fail;

    // will detect and set profile/tier/level in VPS
    determineLevel(*param, encoder->m_vps);

    if (!param->bAllowNonConformance && encoder->m_vps.ptl.profileIdc == Profile::NONE)
    {
        x265_log(param, X265_LOG_INFO, "non-conformant bitstreams not allowed (--allow-non-conformance)\n");
        goto fail;
    }

    encoder->create();
    encoder->m_latestParam = latestParam;
    memcpy(latestParam, param, sizeof(x265_param));
    if (encoder->m_aborted)
        goto fail;

    x265_print_params(param);
    return encoder;

fail:
    delete encoder;
    PARAM_NS::x265_param_free(param);
    PARAM_NS::x265_param_free(latestParam);
    return NULL;
}

int x265_encoder_headers(x265_encoder *enc, x265_nal **pp_nal, uint32_t *pi_nal)
{
    if (pp_nal && enc)
    {
        Encoder *encoder = static_cast<Encoder*>(enc);
        Entropy sbacCoder;
        Bitstream bs;
        encoder->getStreamHeaders(encoder->m_nalList, sbacCoder, bs);
        *pp_nal = &encoder->m_nalList.m_nal[0];
        if (pi_nal) *pi_nal = encoder->m_nalList.m_numNal;
        return encoder->m_nalList.m_occupancy;
    }

    return -1;
}

void x265_encoder_parameters(x265_encoder *enc, x265_param *out)
{
    if (enc && out)
    {
        Encoder *encoder = static_cast<Encoder*>(enc);
        memcpy(out, encoder->m_param, sizeof(x265_param));
    }
}

int x265_encoder_reconfig(x265_encoder* enc, x265_param* param_in)
{
    if (!enc || !param_in)
        return -1;

    x265_param save;
    Encoder* encoder = static_cast<Encoder*>(enc);
    if (encoder->m_reconfigure) /* Reconfigure in progress */
        return -1;
    memcpy(&save, encoder->m_latestParam, sizeof(x265_param));
    int ret = encoder->reconfigureParam(encoder->m_latestParam, param_in);
    if (ret)
        /* reconfigure failed, recover saved param set */
        memcpy(encoder->m_latestParam, &save, sizeof(x265_param));
    else
    {
        encoder->m_reconfigure = true;
        x265_print_reconfigured_params(&save, encoder->m_latestParam);
    }
    return ret;
}

int x265_encoder_encode(x265_encoder *enc, x265_nal **pp_nal, uint32_t *pi_nal, x265_picture *pic_in, x265_picture *pic_out)
{
    if (!enc)
        return -1;

    Encoder *encoder = static_cast<Encoder*>(enc);
    int numEncoded;

    // While flushing, we cannot return 0 until the entire stream is flushed
    do
    {
        numEncoded = encoder->encode(pic_in, pic_out);
    }
    while (numEncoded == 0 && !pic_in && encoder->m_numDelayedPic);

    // do not allow reuse of these buffers for more than one picture. The
    // encoder now owns these analysisData buffers.
    if (pic_in)
    {
        pic_in->analysisData.intraData = NULL;
        pic_in->analysisData.interData = NULL;
    }

    if (pp_nal && numEncoded > 0)
    {
        *pp_nal = &encoder->m_nalList.m_nal[0];
        if (pi_nal) *pi_nal = encoder->m_nalList.m_numNal;
    }
    else if (pi_nal)
        *pi_nal = 0;

    return numEncoded;
}

void x265_encoder_get_stats(x265_encoder *enc, x265_stats *outputStats, uint32_t statsSizeBytes)
{
    if (enc && outputStats)
    {
        Encoder *encoder = static_cast<Encoder*>(enc);
        encoder->fetchStats(outputStats, statsSizeBytes);
    }
}

void x265_encoder_log(x265_encoder* enc, int, char **)
{
    if (enc)
    {
        Encoder *encoder = static_cast<Encoder*>(enc);
        x265_log(encoder->m_param, X265_LOG_WARNING, "x265_encoder_log is now deprecated\n");
    }
}

void x265_encoder_close(x265_encoder *enc)
{
    if (enc)
    {
        Encoder *encoder = static_cast<Encoder*>(enc);

        encoder->stopJobs();
        encoder->printSummary();
        encoder->destroy();
        delete encoder;
        ATOMIC_DEC(&g_ctuSizeConfigured);
    }
}

int x265_encoder_intra_refresh(x265_encoder *enc)
{
    if (!enc)
        return -1;

    Encoder *encoder = static_cast<Encoder*>(enc);
    encoder->m_bQueuedIntraRefresh = 1;
    return 0;
}

void x265_cleanup(void)
{
    if (!g_ctuSizeConfigured)
    {
        BitCost::destroy();
        CUData::s_partSet[0] = NULL; /* allow CUData to adjust to new CTU size */
    }
}

x265_picture *x265_picture_alloc()
{
    return (x265_picture*)x265_malloc(sizeof(x265_picture));
}

void x265_picture_init(x265_param *param, x265_picture *pic)
{
    memset(pic, 0, sizeof(x265_picture));

    pic->bitDepth = param->internalBitDepth;
    pic->colorSpace = param->internalCsp;
    pic->forceqp = X265_QP_AUTO;
    pic->quantOffsets = NULL;
    if (param->analysisMode)
    {
        uint32_t widthInCU       = (param->sourceWidth  + g_maxCUSize - 1) >> g_maxLog2CUSize;
        uint32_t heightInCU      = (param->sourceHeight + g_maxCUSize - 1) >> g_maxLog2CUSize;

        uint32_t numCUsInFrame   = widthInCU * heightInCU;
        pic->analysisData.numCUsInFrame = numCUsInFrame;
        pic->analysisData.numPartitions = NUM_4x4_PARTITIONS;
    }
}

void x265_picture_free(x265_picture *p)
{
    return x265_free(p);
}

static const x265_api libapi =
{
    X265_MAJOR_VERSION,
    X265_BUILD,
    sizeof(x265_param),
    sizeof(x265_picture),
    sizeof(x265_analysis_data),
    sizeof(x265_zone),
    sizeof(x265_stats),

    PFX(max_bit_depth),
    PFX(version_str),
    PFX(build_info_str),

    &PARAM_NS::x265_param_alloc,
    &PARAM_NS::x265_param_free,
    &PARAM_NS::x265_param_default,
    &PARAM_NS::x265_param_parse,
    &PARAM_NS::x265_param_apply_profile,
    &PARAM_NS::x265_param_default_preset,
    &x265_picture_alloc,
    &x265_picture_free,
    &x265_picture_init,
    &x265_encoder_open,
    &x265_encoder_parameters,
    &x265_encoder_reconfig,
    &x265_encoder_headers,
    &x265_encoder_encode,
    &x265_encoder_get_stats,
    &x265_encoder_log,
    &x265_encoder_close,
    &x265_cleanup,

    sizeof(x265_frame_stats),
    &x265_encoder_intra_refresh,
};

typedef const x265_api* (*api_get_func)(int bitDepth);
typedef const x265_api* (*api_query_func)(int bitDepth, int apiVersion, int* err);

#define xstr(s) str(s)
#define str(s) #s

#if _WIN32
#define ext ".dll"
#elif MACOS
#include <dlfcn.h>
#define ext ".dylib"
#else
#include <dlfcn.h>
#define ext ".so"
#endif

static int g_recursion /* = 0 */;

const x265_api* x265_api_get(int bitDepth)
{
    if (bitDepth && bitDepth != X265_DEPTH)
    {
#if LINKED_8BIT
        if (bitDepth == 8) return x265_8bit::x265_api_get(0);
#endif
#if LINKED_10BIT
        if (bitDepth == 10) return x265_10bit::x265_api_get(0);
#endif
#if LINKED_12BIT
        if (bitDepth == 12) return x265_12bit::x265_api_get(0);
#endif

        const char* libname = NULL;
        const char* method = "x265_api_get_" xstr(X265_BUILD);
        const char* multilibname = "libx265" ext;

        if (bitDepth == 12)
            libname = "libx265_main12" ext;
        else if (bitDepth == 10)
            libname = "libx265_main10" ext;
        else if (bitDepth == 8)
            libname = "libx265_main" ext;
        else
            return NULL;

        const x265_api* api = NULL;
        int reqDepth = 0;

        if (g_recursion > 1)
            return NULL;
        else
            g_recursion++;

#if _WIN32
        HMODULE h = LoadLibraryA(libname);
        if (!h)
        {
            h = LoadLibraryA(multilibname);
            reqDepth = bitDepth;
        }
        if (h)
        {
            api_get_func get = (api_get_func)GetProcAddress(h, method);
            if (get)
                api = get(reqDepth);
        }
#else
        void* h = dlopen(libname, RTLD_LAZY | RTLD_LOCAL);
        if (!h)
        {
            h = dlopen(multilibname, RTLD_LAZY | RTLD_LOCAL);
            reqDepth = bitDepth;
        }
        if (h)
        {
            api_get_func get = (api_get_func)dlsym(h, method);
            if (get)
                api = get(reqDepth);
        }
#endif

        g_recursion--;

        if (api && bitDepth != api->bit_depth)
        {
            x265_log(NULL, X265_LOG_WARNING, "%s does not support requested bitDepth %d\n", libname, bitDepth);
            return NULL;
        }

        return api;
    }

    return &libapi;
}

const x265_api* x265_api_query(int bitDepth, int apiVersion, int* err)
{
    if (apiVersion < 51)
    {
        /* builds before 1.6 had re-ordered public structs */
        if (err) *err = X265_API_QUERY_ERR_VER_REFUSED;
        return NULL;
    }

    if (err) *err = X265_API_QUERY_ERR_NONE;

    if (bitDepth && bitDepth != X265_DEPTH)
    {
#if LINKED_8BIT
        if (bitDepth == 8) return x265_8bit::x265_api_query(0, apiVersion, err);
#endif
#if LINKED_10BIT
        if (bitDepth == 10) return x265_10bit::x265_api_query(0, apiVersion, err);
#endif
#if LINKED_12BIT
        if (bitDepth == 12) return x265_12bit::x265_api_query(0, apiVersion, err);
#endif

        const char* libname = NULL;
        const char* method = "x265_api_query";
        const char* multilibname = "libx265" ext;

        if (bitDepth == 12)
            libname = "libx265_main12" ext;
        else if (bitDepth == 10)
            libname = "libx265_main10" ext;
        else if (bitDepth == 8)
            libname = "libx265_main" ext;
        else
        {
            if (err) *err = X265_API_QUERY_ERR_LIB_NOT_FOUND;
            return NULL;
        }

        const x265_api* api = NULL;
        int reqDepth = 0;
        int e = X265_API_QUERY_ERR_LIB_NOT_FOUND;

        if (g_recursion > 1)
        {
            if (err) *err = X265_API_QUERY_ERR_LIB_NOT_FOUND;
            return NULL;
        }
        else
            g_recursion++;

#if _WIN32
        HMODULE h = LoadLibraryA(libname);
        if (!h)
        {
            h = LoadLibraryA(multilibname);
            reqDepth = bitDepth;
        }
        if (h)
        {
            e = X265_API_QUERY_ERR_FUNC_NOT_FOUND;
            api_query_func query = (api_query_func)GetProcAddress(h, method);
            if (query)
                api = query(reqDepth, apiVersion, err);
        }
#else
        void* h = dlopen(libname, RTLD_LAZY | RTLD_LOCAL);
        if (!h)
        {
            h = dlopen(multilibname, RTLD_LAZY | RTLD_LOCAL);
            reqDepth = bitDepth;
        }
        if (h)
        {
            e = X265_API_QUERY_ERR_FUNC_NOT_FOUND;
            api_query_func query = (api_query_func)dlsym(h, method);
            if (query)
                api = query(reqDepth, apiVersion, err);
        }
#endif

        g_recursion--;

        if (api && bitDepth != api->bit_depth)
        {
            x265_log(NULL, X265_LOG_WARNING, "%s does not support requested bitDepth %d\n", libname, bitDepth);
            if (err) *err = X265_API_QUERY_ERR_WRONG_BITDEPTH;
            return NULL;
        }

        if (err) *err = api ? X265_API_QUERY_ERR_NONE : e;
        return api;
    }

    return &libapi;
}

} /* end namespace or extern "C" */
