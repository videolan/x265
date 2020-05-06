/*****************************************************************************
 * Copyright (C) 2020 MulticoreWare, Inc
 *
 * Authors: Hongbin Liu <liuhongbin1@huawei.com>
 *          Yimeng Su <yimeng.su@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "primitives.h"
#include "x265.h"
#include "cpu.h"


#if defined(__GNUC__)
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#define GCC_4_9_0 40900
#define GCC_5_1_0 50100

extern "C" {
#include "pixel.h"
#include "pixel-util.h"
#include "ipfilter8.h"
}

namespace X265_NS {
// private x265 namespace


template<int size>
void interp_8tap_hv_pp_cpu(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int idxX, int idxY)
{
    ALIGN_VAR_32(int16_t, immed[MAX_CU_SIZE * (MAX_CU_SIZE + NTAPS_LUMA - 1)]);
    const int halfFilterSize = NTAPS_LUMA >> 1;
    const int immedStride = MAX_CU_SIZE;

    primitives.pu[size].luma_hps(src, srcStride, immed, immedStride, idxX, 1);
    primitives.pu[size].luma_vsp(immed + (halfFilterSize - 1) * immedStride, immedStride, dst, dstStride, idxY);
}


/* Temporary workaround because luma_vsp assembly primitive has not been completed
 * but interp_8tap_hv_pp_cpu uses mixed C primitive and assembly primitive.
 * Otherwise, segment fault occurs. */
void setupAliasCPrimitives(EncoderPrimitives &cp, EncoderPrimitives &asmp, int cpuMask)
{
    if (cpuMask & X265_CPU_NEON)
    {
        asmp.pu[LUMA_8x4].luma_vsp   = cp.pu[LUMA_8x4].luma_vsp;
        asmp.pu[LUMA_8x8].luma_vsp   = cp.pu[LUMA_8x8].luma_vsp;
        asmp.pu[LUMA_8x16].luma_vsp  = cp.pu[LUMA_8x16].luma_vsp;
        asmp.pu[LUMA_8x32].luma_vsp  = cp.pu[LUMA_8x32].luma_vsp;
        asmp.pu[LUMA_12x16].luma_vsp = cp.pu[LUMA_12x16].luma_vsp;
#if !AUTO_VECTORIZE || GCC_VERSION < GCC_5_1_0 /* gcc_version < gcc-5.1.0 */
        asmp.pu[LUMA_16x4].luma_vsp  = cp.pu[LUMA_16x4].luma_vsp;
        asmp.pu[LUMA_16x8].luma_vsp  = cp.pu[LUMA_16x8].luma_vsp;
        asmp.pu[LUMA_16x12].luma_vsp = cp.pu[LUMA_16x12].luma_vsp;
        asmp.pu[LUMA_16x16].luma_vsp = cp.pu[LUMA_16x16].luma_vsp;
        asmp.pu[LUMA_16x32].luma_vsp = cp.pu[LUMA_16x32].luma_vsp;
        asmp.pu[LUMA_16x64].luma_vsp = cp.pu[LUMA_16x64].luma_vsp;
        asmp.pu[LUMA_32x16].luma_vsp = cp.pu[LUMA_32x16].luma_vsp;
        asmp.pu[LUMA_32x24].luma_vsp = cp.pu[LUMA_32x24].luma_vsp;
        asmp.pu[LUMA_32x32].luma_vsp = cp.pu[LUMA_32x32].luma_vsp;
        asmp.pu[LUMA_32x64].luma_vsp = cp.pu[LUMA_32x64].luma_vsp;
        asmp.pu[LUMA_48x64].luma_vsp = cp.pu[LUMA_48x64].luma_vsp;
        asmp.pu[LUMA_64x16].luma_vsp = cp.pu[LUMA_64x16].luma_vsp;
        asmp.pu[LUMA_64x32].luma_vsp = cp.pu[LUMA_64x32].luma_vsp;
        asmp.pu[LUMA_64x48].luma_vsp = cp.pu[LUMA_64x48].luma_vsp;
        asmp.pu[LUMA_64x64].luma_vsp = cp.pu[LUMA_64x64].luma_vsp;    
#if !AUTO_VECTORIZE || GCC_VERSION < GCC_4_9_0 /* gcc_version < gcc-4.9.0 */
        asmp.pu[LUMA_4x4].luma_vsp   = cp.pu[LUMA_4x4].luma_vsp;
        asmp.pu[LUMA_4x8].luma_vsp   = cp.pu[LUMA_4x8].luma_vsp;
        asmp.pu[LUMA_4x16].luma_vsp  = cp.pu[LUMA_4x16].luma_vsp;
        asmp.pu[LUMA_24x32].luma_vsp = cp.pu[LUMA_24x32].luma_vsp;
        asmp.pu[LUMA_32x8].luma_vsp  = cp.pu[LUMA_32x8].luma_vsp;
#endif
#endif
    }
}


void setupAssemblyPrimitives(EncoderPrimitives &p, int cpuMask) 
{
    if (cpuMask & X265_CPU_NEON)
    {
        p.pu[LUMA_4x4].satd   = PFX(pixel_satd_4x4_neon);
        p.pu[LUMA_4x8].satd   = PFX(pixel_satd_4x8_neon);
        p.pu[LUMA_4x16].satd  = PFX(pixel_satd_4x16_neon);
        p.pu[LUMA_8x4].satd   = PFX(pixel_satd_8x4_neon);
        p.pu[LUMA_8x8].satd   = PFX(pixel_satd_8x8_neon);
        p.pu[LUMA_12x16].satd = PFX(pixel_satd_12x16_neon);
        
        p.chroma[X265_CSP_I420].pu[CHROMA_420_4x4].satd    = PFX(pixel_satd_4x4_neon);
        p.chroma[X265_CSP_I420].pu[CHROMA_420_4x8].satd    = PFX(pixel_satd_4x8_neon);
        p.chroma[X265_CSP_I420].pu[CHROMA_420_4x16].satd   = PFX(pixel_satd_4x16_neon);
        p.chroma[X265_CSP_I420].pu[CHROMA_420_8x4].satd    = PFX(pixel_satd_8x4_neon);
        p.chroma[X265_CSP_I420].pu[CHROMA_420_8x8].satd    = PFX(pixel_satd_8x8_neon);
        p.chroma[X265_CSP_I420].pu[CHROMA_420_12x16].satd  = PFX(pixel_satd_12x16_neon);
        
        p.chroma[X265_CSP_I422].pu[CHROMA_422_4x4].satd    = PFX(pixel_satd_4x4_neon);
        p.chroma[X265_CSP_I422].pu[CHROMA_422_4x8].satd    = PFX(pixel_satd_4x8_neon);
        p.chroma[X265_CSP_I422].pu[CHROMA_422_4x16].satd   = PFX(pixel_satd_4x16_neon);
        p.chroma[X265_CSP_I422].pu[CHROMA_422_4x32].satd   = PFX(pixel_satd_4x32_neon);
        p.chroma[X265_CSP_I422].pu[CHROMA_422_8x4].satd    = PFX(pixel_satd_8x4_neon);
        p.chroma[X265_CSP_I422].pu[CHROMA_422_8x8].satd    = PFX(pixel_satd_8x8_neon);
        p.chroma[X265_CSP_I422].pu[CHROMA_422_12x32].satd  = PFX(pixel_satd_12x32_neon);

        p.pu[LUMA_4x4].pixelavg_pp[NONALIGNED]   = PFX(pixel_avg_pp_4x4_neon);
        p.pu[LUMA_4x8].pixelavg_pp[NONALIGNED]   = PFX(pixel_avg_pp_4x8_neon);
        p.pu[LUMA_4x16].pixelavg_pp[NONALIGNED]  = PFX(pixel_avg_pp_4x16_neon);
        p.pu[LUMA_8x4].pixelavg_pp[NONALIGNED]   = PFX(pixel_avg_pp_8x4_neon);
        p.pu[LUMA_8x8].pixelavg_pp[NONALIGNED]   = PFX(pixel_avg_pp_8x8_neon);
        p.pu[LUMA_8x16].pixelavg_pp[NONALIGNED]  = PFX(pixel_avg_pp_8x16_neon);
        p.pu[LUMA_8x32].pixelavg_pp[NONALIGNED]  = PFX(pixel_avg_pp_8x32_neon);

        p.pu[LUMA_4x4].pixelavg_pp[ALIGNED]   = PFX(pixel_avg_pp_4x4_neon);
        p.pu[LUMA_4x8].pixelavg_pp[ALIGNED]   = PFX(pixel_avg_pp_4x8_neon);
        p.pu[LUMA_4x16].pixelavg_pp[ALIGNED]  = PFX(pixel_avg_pp_4x16_neon);
        p.pu[LUMA_8x4].pixelavg_pp[ALIGNED]   = PFX(pixel_avg_pp_8x4_neon);
        p.pu[LUMA_8x8].pixelavg_pp[ALIGNED]   = PFX(pixel_avg_pp_8x8_neon);
        p.pu[LUMA_8x16].pixelavg_pp[ALIGNED]  = PFX(pixel_avg_pp_8x16_neon);
        p.pu[LUMA_8x32].pixelavg_pp[ALIGNED]  = PFX(pixel_avg_pp_8x32_neon);

        p.pu[LUMA_8x4].sad_x3   = PFX(sad_x3_8x4_neon);
        p.pu[LUMA_8x8].sad_x3   = PFX(sad_x3_8x8_neon);
        p.pu[LUMA_8x16].sad_x3  = PFX(sad_x3_8x16_neon);
        p.pu[LUMA_8x32].sad_x3  = PFX(sad_x3_8x32_neon);

        p.pu[LUMA_8x4].sad_x4   = PFX(sad_x4_8x4_neon);
        p.pu[LUMA_8x8].sad_x4   = PFX(sad_x4_8x8_neon);
        p.pu[LUMA_8x16].sad_x4  = PFX(sad_x4_8x16_neon);
        p.pu[LUMA_8x32].sad_x4  = PFX(sad_x4_8x32_neon);

        // quant
        p.quant = PFX(quant_neon);
        // luma_hps
        p.pu[LUMA_4x4].luma_hps   = PFX(interp_8tap_horiz_ps_4x4_neon);
        p.pu[LUMA_4x8].luma_hps   = PFX(interp_8tap_horiz_ps_4x8_neon);
        p.pu[LUMA_4x16].luma_hps  = PFX(interp_8tap_horiz_ps_4x16_neon);
        p.pu[LUMA_8x4].luma_hps   = PFX(interp_8tap_horiz_ps_8x4_neon);
        p.pu[LUMA_8x8].luma_hps   = PFX(interp_8tap_horiz_ps_8x8_neon);
        p.pu[LUMA_8x16].luma_hps  = PFX(interp_8tap_horiz_ps_8x16_neon);
        p.pu[LUMA_8x32].luma_hps  = PFX(interp_8tap_horiz_ps_8x32_neon);
        p.pu[LUMA_12x16].luma_hps = PFX(interp_8tap_horiz_ps_12x16_neon);
        p.pu[LUMA_24x32].luma_hps = PFX(interp_8tap_horiz_ps_24x32_neon);
#if !AUTO_VECTORIZE || GCC_VERSION < GCC_5_1_0 /* gcc_version < gcc-5.1.0 */
        p.pu[LUMA_16x4].luma_hps  = PFX(interp_8tap_horiz_ps_16x4_neon);
        p.pu[LUMA_16x8].luma_hps  = PFX(interp_8tap_horiz_ps_16x8_neon);
        p.pu[LUMA_16x12].luma_hps = PFX(interp_8tap_horiz_ps_16x12_neon);
        p.pu[LUMA_16x16].luma_hps = PFX(interp_8tap_horiz_ps_16x16_neon);
        p.pu[LUMA_16x32].luma_hps = PFX(interp_8tap_horiz_ps_16x32_neon);
        p.pu[LUMA_16x64].luma_hps = PFX(interp_8tap_horiz_ps_16x64_neon);
        p.pu[LUMA_32x8].luma_hps  = PFX(interp_8tap_horiz_ps_32x8_neon);
        p.pu[LUMA_32x16].luma_hps = PFX(interp_8tap_horiz_ps_32x16_neon);
        p.pu[LUMA_32x24].luma_hps = PFX(interp_8tap_horiz_ps_32x24_neon);
        p.pu[LUMA_32x32].luma_hps = PFX(interp_8tap_horiz_ps_32x32_neon);
        p.pu[LUMA_32x64].luma_hps = PFX(interp_8tap_horiz_ps_32x64_neon);
        p.pu[LUMA_48x64].luma_hps = PFX(interp_8tap_horiz_ps_48x64_neon);
        p.pu[LUMA_64x16].luma_hps = PFX(interp_8tap_horiz_ps_64x16_neon);
        p.pu[LUMA_64x32].luma_hps = PFX(interp_8tap_horiz_ps_64x32_neon);
        p.pu[LUMA_64x48].luma_hps = PFX(interp_8tap_horiz_ps_64x48_neon);
        p.pu[LUMA_64x64].luma_hps = PFX(interp_8tap_horiz_ps_64x64_neon);
#endif

        p.pu[LUMA_8x4].luma_hvpp   =  interp_8tap_hv_pp_cpu<LUMA_8x4>;
        p.pu[LUMA_8x8].luma_hvpp   =  interp_8tap_hv_pp_cpu<LUMA_8x8>;
        p.pu[LUMA_8x16].luma_hvpp  =  interp_8tap_hv_pp_cpu<LUMA_8x16>;
        p.pu[LUMA_8x32].luma_hvpp  =  interp_8tap_hv_pp_cpu<LUMA_8x32>;
        p.pu[LUMA_12x16].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_12x16>;
#if !AUTO_VECTORIZE || GCC_VERSION < GCC_5_1_0 /* gcc_version < gcc-5.1.0 */
        p.pu[LUMA_16x4].luma_hvpp  =  interp_8tap_hv_pp_cpu<LUMA_16x4>;
        p.pu[LUMA_16x8].luma_hvpp  =  interp_8tap_hv_pp_cpu<LUMA_16x8>;
        p.pu[LUMA_16x12].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_16x12>;
        p.pu[LUMA_16x16].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_16x16>;
        p.pu[LUMA_16x32].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_16x32>;
        p.pu[LUMA_16x64].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_16x64>;
        p.pu[LUMA_32x16].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_32x16>;
        p.pu[LUMA_32x24].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_32x24>;
        p.pu[LUMA_32x32].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_32x32>;
        p.pu[LUMA_32x64].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_32x64>;
        p.pu[LUMA_48x64].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_48x64>;
        p.pu[LUMA_64x16].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_64x16>;
        p.pu[LUMA_64x32].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_64x32>;
        p.pu[LUMA_64x48].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_64x48>;
        p.pu[LUMA_64x64].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_64x64>;
#if !AUTO_VECTORIZE || GCC_VERSION < GCC_4_9_0 /* gcc_version < gcc-4.9.0 */
        p.pu[LUMA_4x4].luma_hvpp   =  interp_8tap_hv_pp_cpu<LUMA_4x4>;
        p.pu[LUMA_4x8].luma_hvpp   =  interp_8tap_hv_pp_cpu<LUMA_4x8>;
        p.pu[LUMA_4x16].luma_hvpp  =  interp_8tap_hv_pp_cpu<LUMA_4x16>;
        p.pu[LUMA_24x32].luma_hvpp =  interp_8tap_hv_pp_cpu<LUMA_24x32>;
        p.pu[LUMA_32x8].luma_hvpp  =  interp_8tap_hv_pp_cpu<LUMA_32x8>;
#endif
#endif

#if !HIGH_BIT_DEPTH
        p.cu[BLOCK_4x4].psy_cost_pp = PFX(psyCost_4x4_neon);
#endif // !HIGH_BIT_DEPTH

    }
}
} // namespace X265_NS
