/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
 *          Min Chen <chenm003@163.com> <min.chen@multicorewareinc.com>
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
#include "primitives.h"
#include "x265.h"
#include "cpu.h"

extern "C" {
#include "pixel.h"
#include "pixel-util.h"
#include "mc.h"
#include "ipfilter8.h"
#include "loopfilter.h"
#include "blockcopy8.h"
#include "intrapred.h"
#include "dct8.h"
}

template<int size>
void interp_8tap_hv_pp_cpu(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int idxX, int idxY)
{
    ALIGN_VAR_32(int16_t, immed[MAX_CU_SIZE * (MAX_CU_SIZE + NTAPS_LUMA)]);
    const int filterSize = NTAPS_LUMA;
    const int halfFilterSize = filterSize >> 1;

    x265::primitives.pu[size].luma_hps(src, srcStride, immed, MAX_CU_SIZE, idxX, 1);
    x265::primitives.pu[size].luma_vsp(immed + (halfFilterSize - 1) * MAX_CU_SIZE, MAX_CU_SIZE, dst, dstStride, idxY);
}

#define INIT2_NAME(name1, name2, cpu) \
    p.pu[LUMA_16x16].name1 = x265_pixel_ ## name2 ## _16x16 ## cpu; \
    p.pu[LUMA_16x8].name1  = x265_pixel_ ## name2 ## _16x8 ## cpu;
#define INIT4_NAME(name1, name2, cpu) \
    INIT2_NAME(name1, name2, cpu) \
    p.pu[LUMA_8x16].name1  = x265_pixel_ ## name2 ## _8x16 ## cpu; \
    p.pu[LUMA_8x8].name1   = x265_pixel_ ## name2 ## _8x8 ## cpu;
#define INIT5_NAME(name1, name2, cpu) \
    INIT4_NAME(name1, name2, cpu) \
    p.pu[LUMA_8x4].name1   = x265_pixel_ ## name2 ## _8x4 ## cpu;
#define INIT6_NAME(name1, name2, cpu) \
    INIT5_NAME(name1, name2, cpu) \
    p.pu[LUMA_4x8].name1   = x265_pixel_ ## name2 ## _4x8 ## cpu;
#define INIT7_NAME(name1, name2, cpu) \
    INIT6_NAME(name1, name2, cpu) \
    p.pu[LUMA_4x4].name1   = x265_pixel_ ## name2 ## _4x4 ## cpu;
#define INIT8_NAME(name1, name2, cpu) \
    INIT7_NAME(name1, name2, cpu) \
    p.pu[LUMA_4x16].name1  = x265_pixel_ ## name2 ## _4x16 ## cpu;
#define INIT2(name, cpu) INIT2_NAME(name, name, cpu)
#define INIT4(name, cpu) INIT4_NAME(name, name, cpu)
#define INIT5(name, cpu) INIT5_NAME(name, name, cpu)
#define INIT6(name, cpu) INIT6_NAME(name, name, cpu)
#define INIT7(name, cpu) INIT7_NAME(name, name, cpu)
#define INIT8(name, cpu) INIT8_NAME(name, name, cpu)

#define HEVC_SATD(cpu) \
    p.pu[LUMA_4x8].satd   = x265_pixel_satd_4x8_ ## cpu; \
    p.pu[LUMA_4x16].satd  = x265_pixel_satd_4x16_ ## cpu; \
    p.pu[LUMA_8x4].satd   = x265_pixel_satd_8x4_ ## cpu; \
    p.pu[LUMA_8x8].satd   = x265_pixel_satd_8x8_ ## cpu; \
    p.pu[LUMA_8x16].satd  = x265_pixel_satd_8x16_ ## cpu; \
    p.pu[LUMA_8x32].satd  = x265_pixel_satd_8x32_ ## cpu; \
    p.pu[LUMA_12x16].satd = x265_pixel_satd_12x16_ ## cpu; \
    p.pu[LUMA_16x4].satd  = x265_pixel_satd_16x4_ ## cpu; \
    p.pu[LUMA_16x8].satd  = x265_pixel_satd_16x8_ ## cpu; \
    p.pu[LUMA_16x12].satd = x265_pixel_satd_16x12_ ## cpu; \
    p.pu[LUMA_16x16].satd = x265_pixel_satd_16x16_ ## cpu; \
    p.pu[LUMA_16x32].satd = x265_pixel_satd_16x32_ ## cpu; \
    p.pu[LUMA_16x64].satd = x265_pixel_satd_16x64_ ## cpu; \
    p.pu[LUMA_24x32].satd = x265_pixel_satd_24x32_ ## cpu; \
    p.pu[LUMA_32x8].satd  = x265_pixel_satd_32x8_ ## cpu; \
    p.pu[LUMA_32x16].satd = x265_pixel_satd_32x16_ ## cpu; \
    p.pu[LUMA_32x24].satd = x265_pixel_satd_32x24_ ## cpu; \
    p.pu[LUMA_32x32].satd = x265_pixel_satd_32x32_ ## cpu; \
    p.pu[LUMA_32x64].satd = x265_pixel_satd_32x64_ ## cpu; \
    p.pu[LUMA_48x64].satd = x265_pixel_satd_48x64_ ## cpu; \
    p.pu[LUMA_64x16].satd = x265_pixel_satd_64x16_ ## cpu; \
    p.pu[LUMA_64x32].satd = x265_pixel_satd_64x32_ ## cpu; \
    p.pu[LUMA_64x48].satd = x265_pixel_satd_64x48_ ## cpu; \
    p.pu[LUMA_64x64].satd = x265_pixel_satd_64x64_ ## cpu;

#define SAD_X3(cpu) \
    p.pu[LUMA_16x8].sad_x3  = x265_pixel_sad_x3_16x8_ ## cpu; \
    p.pu[LUMA_16x12].sad_x3 = x265_pixel_sad_x3_16x12_ ## cpu; \
    p.pu[LUMA_16x16].sad_x3 = x265_pixel_sad_x3_16x16_ ## cpu; \
    p.pu[LUMA_16x32].sad_x3 = x265_pixel_sad_x3_16x32_ ## cpu; \
    p.pu[LUMA_16x64].sad_x3 = x265_pixel_sad_x3_16x64_ ## cpu; \
    p.pu[LUMA_32x8].sad_x3  = x265_pixel_sad_x3_32x8_ ## cpu; \
    p.pu[LUMA_32x16].sad_x3 = x265_pixel_sad_x3_32x16_ ## cpu; \
    p.pu[LUMA_32x24].sad_x3 = x265_pixel_sad_x3_32x24_ ## cpu; \
    p.pu[LUMA_32x32].sad_x3 = x265_pixel_sad_x3_32x32_ ## cpu; \
    p.pu[LUMA_32x64].sad_x3 = x265_pixel_sad_x3_32x64_ ## cpu; \
    p.pu[LUMA_24x32].sad_x3 = x265_pixel_sad_x3_24x32_ ## cpu; \
    p.pu[LUMA_48x64].sad_x3 = x265_pixel_sad_x3_48x64_ ## cpu; \
    p.pu[LUMA_64x16].sad_x3 = x265_pixel_sad_x3_64x16_ ## cpu; \
    p.pu[LUMA_64x32].sad_x3 = x265_pixel_sad_x3_64x32_ ## cpu; \
    p.pu[LUMA_64x48].sad_x3 = x265_pixel_sad_x3_64x48_ ## cpu; \
    p.pu[LUMA_64x64].sad_x3 = x265_pixel_sad_x3_64x64_ ## cpu

#define SAD_X4(cpu) \
    p.pu[LUMA_16x8].sad_x4  = x265_pixel_sad_x4_16x8_ ## cpu; \
    p.pu[LUMA_16x12].sad_x4 = x265_pixel_sad_x4_16x12_ ## cpu; \
    p.pu[LUMA_16x16].sad_x4 = x265_pixel_sad_x4_16x16_ ## cpu; \
    p.pu[LUMA_16x32].sad_x4 = x265_pixel_sad_x4_16x32_ ## cpu; \
    p.pu[LUMA_16x64].sad_x4 = x265_pixel_sad_x4_16x64_ ## cpu; \
    p.pu[LUMA_32x8].sad_x4  = x265_pixel_sad_x4_32x8_ ## cpu; \
    p.pu[LUMA_32x16].sad_x4 = x265_pixel_sad_x4_32x16_ ## cpu; \
    p.pu[LUMA_32x24].sad_x4 = x265_pixel_sad_x4_32x24_ ## cpu; \
    p.pu[LUMA_32x32].sad_x4 = x265_pixel_sad_x4_32x32_ ## cpu; \
    p.pu[LUMA_32x64].sad_x4 = x265_pixel_sad_x4_32x64_ ## cpu; \
    p.pu[LUMA_24x32].sad_x4 = x265_pixel_sad_x4_24x32_ ## cpu; \
    p.pu[LUMA_48x64].sad_x4 = x265_pixel_sad_x4_48x64_ ## cpu; \
    p.pu[LUMA_64x16].sad_x4 = x265_pixel_sad_x4_64x16_ ## cpu; \
    p.pu[LUMA_64x32].sad_x4 = x265_pixel_sad_x4_64x32_ ## cpu; \
    p.pu[LUMA_64x48].sad_x4 = x265_pixel_sad_x4_64x48_ ## cpu; \
    p.pu[LUMA_64x64].sad_x4 = x265_pixel_sad_x4_64x64_ ## cpu

#define SAD(cpu) \
    p.pu[LUMA_8x32].sad  = x265_pixel_sad_8x32_ ## cpu; \
    p.pu[LUMA_16x4].sad  = x265_pixel_sad_16x4_ ## cpu; \
    p.pu[LUMA_16x12].sad = x265_pixel_sad_16x12_ ## cpu; \
    p.pu[LUMA_16x32].sad = x265_pixel_sad_16x32_ ## cpu; \
    p.pu[LUMA_16x64].sad = x265_pixel_sad_16x64_ ## cpu; \
    p.pu[LUMA_32x8].sad  = x265_pixel_sad_32x8_ ## cpu; \
    p.pu[LUMA_32x16].sad = x265_pixel_sad_32x16_ ## cpu; \
    p.pu[LUMA_32x24].sad = x265_pixel_sad_32x24_ ## cpu; \
    p.pu[LUMA_32x32].sad = x265_pixel_sad_32x32_ ## cpu; \
    p.pu[LUMA_32x64].sad = x265_pixel_sad_32x64_ ## cpu; \
    p.pu[LUMA_64x16].sad = x265_pixel_sad_64x16_ ## cpu; \
    p.pu[LUMA_64x32].sad = x265_pixel_sad_64x32_ ## cpu; \
    p.pu[LUMA_64x48].sad = x265_pixel_sad_64x48_ ## cpu; \
    p.pu[LUMA_64x64].sad = x265_pixel_sad_64x64_ ## cpu; \
    p.pu[LUMA_48x64].sad = x265_pixel_sad_48x64_ ## cpu; \
    p.pu[LUMA_24x32].sad = x265_pixel_sad_24x32_ ## cpu; \
    p.pu[LUMA_12x16].sad = x265_pixel_sad_12x16_ ## cpu

#define ASSGN_SSE(cpu) \
    p.cu[BLOCK_8x8].sse_pp   = x265_pixel_ssd_8x8_ ## cpu; \
    p.cu[BLOCK_16x16].sse_pp = x265_pixel_ssd_16x16_ ## cpu; \
    p.cu[BLOCK_32x32].sse_pp = x265_pixel_ssd_32x32_ ## cpu; \
    p.chroma[X265_CSP_I422].cu[BLOCK_32x32].sse_pp = x265_pixel_ssd_16x32_ ## cpu; \
    p.chroma[X265_CSP_I422].cu[BLOCK_16x16].sse_pp = x265_pixel_ssd_8x16_ ## cpu; \
    p.chroma[X265_CSP_I422].cu[BLOCK_64x64].sse_pp = x265_pixel_ssd_32x64_ ## cpu;

#define ASSGN_SSE_SS(cpu) \
    p.cu[BLOCK_4x4].sse_ss   = x265_pixel_ssd_ss_4x4_ ## cpu; \
    p.cu[BLOCK_8x8].sse_ss   = x265_pixel_ssd_ss_8x8_ ## cpu; \
    p.cu[BLOCK_16x16].sse_ss = x265_pixel_ssd_ss_16x16_ ## cpu; \
    p.cu[BLOCK_32x32].sse_ss = x265_pixel_ssd_ss_32x32_ ## cpu; \
    p.cu[BLOCK_64x64].sse_ss = x265_pixel_ssd_ss_64x64_ ## cpu;

#define SA8D_INTER_FROM_BLOCK(cpu) \
    p.cu[BLOCK_8x8].sa8d = x265_pixel_sa8d_8x8_ ## cpu; \
    p.cu[BLOCK_16x16].sa8d = x265_pixel_sa8d_16x16_ ## cpu; \
    p.cu[BLOCK_32x32].sa8d = x265_pixel_sa8d_32x32_ ## cpu; \
    p.cu[BLOCK_64x64].sa8d = x265_pixel_sa8d_64x64_ ## cpu; \
    p.chroma[X265_CSP_I422].cu[BLOCK_16x16].sa8d = x265_pixel_sa8d_8x16_ ## cpu; \
    p.chroma[X265_CSP_I422].cu[BLOCK_32x32].sa8d = x265_pixel_sa8d_16x32_ ## cpu; \
    p.chroma[X265_CSP_I422].cu[BLOCK_64x64].sa8d = x265_pixel_sa8d_32x64_ ## cpu;

#define PIXEL_AVG(cpu) \
    p.pu[LUMA_64x64].pixelavg_pp = x265_pixel_avg_64x64_ ## cpu; \
    p.pu[LUMA_64x48].pixelavg_pp = x265_pixel_avg_64x48_ ## cpu; \
    p.pu[LUMA_64x32].pixelavg_pp = x265_pixel_avg_64x32_ ## cpu; \
    p.pu[LUMA_64x16].pixelavg_pp = x265_pixel_avg_64x16_ ## cpu; \
    p.pu[LUMA_48x64].pixelavg_pp = x265_pixel_avg_48x64_ ## cpu; \
    p.pu[LUMA_32x64].pixelavg_pp = x265_pixel_avg_32x64_ ## cpu; \
    p.pu[LUMA_32x32].pixelavg_pp = x265_pixel_avg_32x32_ ## cpu; \
    p.pu[LUMA_32x24].pixelavg_pp = x265_pixel_avg_32x24_ ## cpu; \
    p.pu[LUMA_32x16].pixelavg_pp = x265_pixel_avg_32x16_ ## cpu; \
    p.pu[LUMA_32x8].pixelavg_pp  = x265_pixel_avg_32x8_ ## cpu; \
    p.pu[LUMA_24x32].pixelavg_pp = x265_pixel_avg_24x32_ ## cpu; \
    p.pu[LUMA_16x64].pixelavg_pp = x265_pixel_avg_16x64_ ## cpu; \
    p.pu[LUMA_16x32].pixelavg_pp = x265_pixel_avg_16x32_ ## cpu; \
    p.pu[LUMA_16x16].pixelavg_pp = x265_pixel_avg_16x16_ ## cpu; \
    p.pu[LUMA_16x12].pixelavg_pp = x265_pixel_avg_16x12_ ## cpu; \
    p.pu[LUMA_16x8].pixelavg_pp  = x265_pixel_avg_16x8_ ## cpu; \
    p.pu[LUMA_16x4].pixelavg_pp  = x265_pixel_avg_16x4_ ## cpu; \
    p.pu[LUMA_12x16].pixelavg_pp = x265_pixel_avg_12x16_ ## cpu; \
    p.pu[LUMA_8x32].pixelavg_pp  = x265_pixel_avg_8x32_ ## cpu; \
    p.pu[LUMA_8x16].pixelavg_pp  = x265_pixel_avg_8x16_ ## cpu; \
    p.pu[LUMA_8x8].pixelavg_pp   = x265_pixel_avg_8x8_ ## cpu; \
    p.pu[LUMA_8x4].pixelavg_pp   = x265_pixel_avg_8x4_ ## cpu;

#define PIXEL_AVG_W4(cpu) \
    p.pu[LUMA_4x4].pixelavg_pp  = x265_pixel_avg_4x4_ ## cpu; \
    p.pu[LUMA_4x8].pixelavg_pp  = x265_pixel_avg_4x8_ ## cpu; \
    p.pu[LUMA_4x16].pixelavg_pp = x265_pixel_avg_4x16_ ## cpu;

#define SETUP_CHROMA_FUNC_DEF_420(W, H, cpu) \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].filter_hpp = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].filter_hps = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].filter_vpp = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].filter_vps = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_FUNC_DEF_422(W, H, cpu) \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].filter_hpp = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].filter_hps = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].filter_vpp = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].filter_vps = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_FUNC_DEF_444(W, H, cpu) \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_hpp = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_hps = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vpp = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vps = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SP_FUNC_DEF_420(W, H, cpu) \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].filter_vsp = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SP_FUNC_DEF_422(W, H, cpu) \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].filter_vsp = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SP_FUNC_DEF_444(W, H, cpu) \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vsp = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SS_FUNC_DEF_420(W, H, cpu) \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].filter_vss = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SS_FUNC_DEF_422(W, H, cpu) \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].filter_vss = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SS_FUNC_DEF_444(W, H, cpu) \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vss = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu;

#define CHROMA_FILTERS_420(cpu) \
    SETUP_CHROMA_FUNC_DEF_420(4, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(4, 2, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(2, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(8, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(8, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(4, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(8, 6, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(6, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(8, 2, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(2, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(16, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(16, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(8, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(16, 12, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(12, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(16, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(4, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(32, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(32, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(16, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(32, 24, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(24, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(32, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_420(8, 32, cpu);

#define CHROMA_FILTERS_422(cpu) \
    SETUP_CHROMA_FUNC_DEF_422(4, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(4, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(2, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(8, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(8, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(4, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(8, 12, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(6, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(8, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(2, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(16, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(16, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(8, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(16, 24, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(12, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(16, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(4, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(32, 64, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(32, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(16, 64, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(32, 48, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(24, 64, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(32, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_422(8, 64, cpu);

#define CHROMA_FILTERS_444(cpu) \
    SETUP_CHROMA_FUNC_DEF_444(8, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(8, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(4, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(16, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(16, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(8, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(16, 12, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(12, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(16, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(4, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(32, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(32, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(16, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(32, 24, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(24, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(32, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(8, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(64, 64, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(64, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(32, 64, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(64, 48, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(48, 64, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(64, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF_444(16, 64, cpu);

#define CHROMA_SP_FILTERS_SSE4_420(cpu) \
    SETUP_CHROMA_SP_FUNC_DEF_420(4, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(4, 2, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(2, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(4, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(6, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(2, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(16, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(16, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(16, 12, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(12, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(16, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(4, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(32, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(32, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(16, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(32, 24, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(24, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(32, 8, cpu);

#define CHROMA_SP_FILTERS_420(cpu) \
    SETUP_CHROMA_SP_FUNC_DEF_420(8, 2, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(8, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(8, 6, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(8, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(8, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_420(8, 32, cpu);

#define CHROMA_SP_FILTERS_SSE4_422(cpu) \
    SETUP_CHROMA_SP_FUNC_DEF_422(4, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(4, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(2, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(4, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(6, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(2, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(16, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(16, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(16, 24, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(12, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(16, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(4, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(32, 64, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(32, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(16, 64, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(32, 48, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(24, 64, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(32, 16, cpu);

#define CHROMA_SP_FILTERS_422(cpu) \
    SETUP_CHROMA_SP_FUNC_DEF_422(8, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(8, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(8, 12, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(8, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(8, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_422(8, 64, cpu);

#define CHROMA_SP_FILTERS_SSE4_444(cpu) \
    SETUP_CHROMA_SP_FUNC_DEF_444(4, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(16, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(16, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(16, 12, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(12, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(16, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(4, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(32, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(32, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(16, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(32, 24, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(24, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(32, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(64, 64, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(64, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(32, 64, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(64, 48, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(48, 64, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(64, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(16, 64, cpu);

#define CHROMA_SP_FILTERS_444(cpu) \
    SETUP_CHROMA_SP_FUNC_DEF_444(8, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(8, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(8, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF_444(8, 32, cpu);

#define CHROMA_SS_FILTERS_420(cpu) \
    SETUP_CHROMA_SS_FUNC_DEF_420(4, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(4, 2, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(8, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(8, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(4, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(8, 6, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(8, 2, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(16, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(16, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(8, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(16, 12, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(12, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(16, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(4, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(32, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(32, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(16, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(32, 24, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(24, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(32, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(8, 32, cpu);

#define CHROMA_SS_FILTERS_SSE4_420(cpu) \
    SETUP_CHROMA_SS_FUNC_DEF_420(2, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(2, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_420(6, 8, cpu);

#define CHROMA_SS_FILTERS_422(cpu) \
    SETUP_CHROMA_SS_FUNC_DEF_422(4, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(4, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(8, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(8, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(4, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(8, 12, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(8, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(16, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(16, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(8, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(16, 24, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(12, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(16, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(4, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(32, 64, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(32, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(16, 64, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(32, 48, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(24, 64, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(32, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(8, 64, cpu);

#define CHROMA_SS_FILTERS_SSE4_422(cpu) \
    SETUP_CHROMA_SS_FUNC_DEF_422(2, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(2, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_422(6, 16, cpu);

#define CHROMA_SS_FILTERS_444(cpu) \
    SETUP_CHROMA_SS_FUNC_DEF_444(8, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(8, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(4, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(16, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(16, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(8, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(16, 12, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(12, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(16, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(4, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(32, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(32, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(16, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(32, 24, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(24, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(32, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(8, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(64, 64, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(64, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(32, 64, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(64, 48, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(48, 64, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(64, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF_444(16, 64, cpu);

#if HIGH_BIT_DEPTH    // temporary, until all 10bit functions are completed
#define SETUP_LUMA_FUNC_DEF(W, H, cpu) \
    p.pu[LUMA_ ## W ## x ## H].luma_hpp = x265_interp_8tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.pu[LUMA_ ## W ## x ## H].luma_hps = x265_interp_8tap_horiz_ps_ ## W ## x ## H ## cpu; \
    p.pu[LUMA_ ## W ## x ## H].luma_vpp = x265_interp_8tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.pu[LUMA_ ## W ## x ## H].luma_vps = x265_interp_8tap_vert_ps_ ## W ## x ## H ## cpu; \
    p.pu[LUMA_ ## W ## x ## H].luma_vsp = x265_interp_8tap_vert_sp_ ## W ## x ## H ## cpu; \
    p.pu[LUMA_ ## W ## x ## H].luma_hvpp = interp_8tap_hv_pp_cpu<LUMA_ ## W ## x ## H>;
#else
#define SETUP_LUMA_FUNC_DEF(W, H, cpu) \
    p.pu[LUMA_ ## W ## x ## H].luma_hpp = x265_interp_8tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.pu[LUMA_ ## W ## x ## H].luma_hps = x265_interp_8tap_horiz_ps_ ## W ## x ## H ## cpu; \
    p.pu[LUMA_ ## W ## x ## H].luma_vpp = x265_interp_8tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.pu[LUMA_ ## W ## x ## H].luma_vps = x265_interp_8tap_vert_ps_ ## W ## x ## H ## cpu; \
    p.pu[LUMA_ ## W ## x ## H].luma_vsp = x265_interp_8tap_vert_sp_ ## W ## x ## H ## cpu; \
    p.pu[LUMA_ ## W ## x ## H].luma_hvpp = interp_8tap_hv_pp_cpu<LUMA_ ## W ## x ## H>;
#endif // if HIGH_BIT_DEPTH

#define SETUP_LUMA_SUB_FUNC_DEF(W, H, cpu) \
    p.cu[LUMA_ ## W ## x ## H].sub_ps = x265_pixel_sub_ps_ ## W ## x ## H ## cpu; \
    p.cu[LUMA_ ## W ## x ## H].add_ps = x265_pixel_add_ps_ ## W ## x ## H ## cpu;

#define SETUP_LUMA_SP_FUNC_DEF(W, H, cpu) \
    p.pu[LUMA_ ## W ## x ## H].luma_vsp = x265_interp_8tap_vert_sp_ ## W ## x ## H ## cpu;

#define SETUP_LUMA_SS_FUNC_DEF(W, H, cpu) \
    p.pu[LUMA_ ## W ## x ## H].luma_vss = x265_interp_8tap_vert_ss_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_BLOCKCOPY(type, W, H, cpu) \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].copy_ ## type = x265_blockcopy_ ## type ## _ ## W ## x ## H ## cpu;

#define CHROMA_BLOCKCOPY(type, cpu) \
    SETUP_CHROMA_BLOCKCOPY(type, 2,  4,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 2,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 4,  2,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 4,  4,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 4,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 4,  16, cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 6,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 8,  2,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 8,  4,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 8,  6,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 8,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 8,  16, cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 8,  32, cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 12, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 16, 4,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 16, 8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 16, 12, cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 16, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 16, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 24, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 32, 8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 32, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 32, 24, cpu); \
    SETUP_CHROMA_BLOCKCOPY(type, 32, 32, cpu);

#define SETUP_CHROMA_CU_BLOCKCOPY(type, W, H, cpu) \
    p.chroma[X265_CSP_I420].cu[CHROMA_ ## W ## x ## H].copy_ ## type = x265_blockcopy_ ## type ## _ ## W ## x ## H ## cpu;

#define CHROMA_CU_BLOCKCOPY(type, cpu) \
    SETUP_CHROMA_CU_BLOCKCOPY(type, 4,  4,  cpu); \
    SETUP_CHROMA_CU_BLOCKCOPY(type, 8,  8,  cpu); \
    SETUP_CHROMA_CU_BLOCKCOPY(type, 16, 16, cpu); \
    SETUP_CHROMA_CU_BLOCKCOPY(type, 32, 32, cpu);

#define SETUP_CHROMA_BLOCKCOPY_422(type, W, H, cpu) \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].copy_ ## type = x265_blockcopy_ ## type ## _ ## W ## x ## H ## cpu;

#define CHROMA_BLOCKCOPY_422(type, cpu) \
    SETUP_CHROMA_BLOCKCOPY_422(type, 2,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 2, 16,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 4,  4,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 4,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 4, 16,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 4, 32,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 6, 16,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 8,  4,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 8,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 8, 12,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 8, 16,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 8, 32,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 8, 64,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 12, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 16,  8, cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 16, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 16, 24, cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 16, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 16, 64, cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 24, 64, cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 32, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 32, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 32, 48, cpu); \
    SETUP_CHROMA_BLOCKCOPY_422(type, 32, 64, cpu);

#define SETUP_CHROMA_CU_BLOCKCOPY_422(type, W, H, cpu) \
    p.chroma[X265_CSP_I422].cu[CHROMA422_ ## W ## x ## H].copy_ ## type = x265_blockcopy_ ## type ## _ ## W ## x ## H ## cpu;

#define CHROMA_CU_BLOCKCOPY_422(type, cpu) \
    SETUP_CHROMA_CU_BLOCKCOPY_422(type, 4,  8,  cpu); \
    SETUP_CHROMA_CU_BLOCKCOPY_422(type, 8, 16,  cpu); \
    SETUP_CHROMA_CU_BLOCKCOPY_422(type, 16, 32, cpu); \
    SETUP_CHROMA_CU_BLOCKCOPY_422(type, 32, 64, cpu);

#define SETUP_LUMA_BLOCKCOPY(type, W, H, cpu) \
    p.pu[LUMA_ ## W ## x ## H].copy_ ## type = x265_blockcopy_ ## type ## _ ## W ## x ## H ## cpu;

#define LUMA_BLOCKCOPY(type, cpu) \
    SETUP_LUMA_BLOCKCOPY(type, 4,   4, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 8,   8, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 8,   4, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 4,   8, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 16, 16, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 16,  8, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 8,  16, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 16, 12, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 12, 16, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 16,  4, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 4,  16, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 32, 32, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 32, 16, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 16, 32, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 32, 24, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 24, 32, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 32,  8, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 8,  32, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 64, 64, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 64, 32, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 32, 64, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 64, 48, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 48, 64, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 64, 16, cpu); \
    SETUP_LUMA_BLOCKCOPY(type, 16, 64, cpu);

#define SETUP_LUMA_CU_BLOCKCOPY(type, W, H, cpu) \
    p.cu[BLOCK_ ## W ## x ## H].copy_ ## type = x265_blockcopy_ ## type ## _ ## W ## x ## H ## cpu;

#define LUMA_CU_BLOCKCOPY(type, cpu) \
    SETUP_LUMA_CU_BLOCKCOPY(type, 4,   4, cpu); \
    SETUP_LUMA_CU_BLOCKCOPY(type, 8,   8, cpu); \
    SETUP_LUMA_CU_BLOCKCOPY(type, 16, 16, cpu); \
    SETUP_LUMA_CU_BLOCKCOPY(type, 32, 32, cpu); \
    SETUP_LUMA_CU_BLOCKCOPY(type, 64, 64, cpu); \

#define SETUP_CHROMA_PIXELSUB(W, H, cpu) \
    p.chroma[X265_CSP_I420].cu[CHROMA_ ## W ## x ## H].sub_ps = x265_pixel_sub_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].cu[CHROMA_ ## W ## x ## H].add_ps = x265_pixel_add_ps_ ## W ## x ## H ## cpu;

#define CHROMA_PIXELSUB_PS(cpu) \
    SETUP_CHROMA_PIXELSUB(4,  4,  cpu); \
    SETUP_CHROMA_PIXELSUB(8,  8,  cpu); \
    SETUP_CHROMA_PIXELSUB(16, 16, cpu); \
    SETUP_CHROMA_PIXELSUB(32, 32, cpu);

#define SETUP_CHROMA_PIXELSUB_422(W, H, cpu) \
    p.chroma[X265_CSP_I422].cu[CHROMA422_ ## W ## x ## H].sub_ps = x265_pixel_sub_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].cu[CHROMA422_ ## W ## x ## H].add_ps = x265_pixel_add_ps_ ## W ## x ## H ## cpu;

#define CHROMA_PIXELSUB_PS_422(cpu) \
    SETUP_CHROMA_PIXELSUB_422(4,  8,  cpu); \
    SETUP_CHROMA_PIXELSUB_422(8, 16,  cpu); \
    SETUP_CHROMA_PIXELSUB_422(16, 32, cpu); \
    SETUP_CHROMA_PIXELSUB_422(32, 64, cpu);

#define LUMA_FILTERS(cpu) \
    SETUP_LUMA_FUNC_DEF(4,   4, cpu); \
    SETUP_LUMA_FUNC_DEF(8,   8, cpu); \
    SETUP_LUMA_FUNC_DEF(8,   4, cpu); \
    SETUP_LUMA_FUNC_DEF(4,   8, cpu); \
    SETUP_LUMA_FUNC_DEF(16, 16, cpu); \
    SETUP_LUMA_FUNC_DEF(16,  8, cpu); \
    SETUP_LUMA_FUNC_DEF(8,  16, cpu); \
    SETUP_LUMA_FUNC_DEF(16, 12, cpu); \
    SETUP_LUMA_FUNC_DEF(12, 16, cpu); \
    SETUP_LUMA_FUNC_DEF(16,  4, cpu); \
    SETUP_LUMA_FUNC_DEF(4,  16, cpu); \
    SETUP_LUMA_FUNC_DEF(32, 32, cpu); \
    SETUP_LUMA_FUNC_DEF(32, 16, cpu); \
    SETUP_LUMA_FUNC_DEF(16, 32, cpu); \
    SETUP_LUMA_FUNC_DEF(32, 24, cpu); \
    SETUP_LUMA_FUNC_DEF(24, 32, cpu); \
    SETUP_LUMA_FUNC_DEF(32,  8, cpu); \
    SETUP_LUMA_FUNC_DEF(8,  32, cpu); \
    SETUP_LUMA_FUNC_DEF(64, 64, cpu); \
    SETUP_LUMA_FUNC_DEF(64, 32, cpu); \
    SETUP_LUMA_FUNC_DEF(32, 64, cpu); \
    SETUP_LUMA_FUNC_DEF(64, 48, cpu); \
    SETUP_LUMA_FUNC_DEF(48, 64, cpu); \
    SETUP_LUMA_FUNC_DEF(64, 16, cpu); \
    SETUP_LUMA_FUNC_DEF(16, 64, cpu);

#define LUMA_PIXELSUB(cpu) \
    SETUP_LUMA_SUB_FUNC_DEF(4,   4, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(8,   8, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(16, 16, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(32, 32, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(64, 64, cpu);

#define LUMA_SP_FILTERS(cpu) \
    SETUP_LUMA_SP_FUNC_DEF(4,   4, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(8,   8, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(8,   4, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(4,   8, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(16, 16, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(16,  8, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(8,  16, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(16, 12, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(12, 16, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(16,  4, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(4,  16, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(32, 32, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(32, 16, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(16, 32, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(32, 24, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(24, 32, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(32,  8, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(8,  32, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(64, 64, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(64, 32, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(32, 64, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(64, 48, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(48, 64, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(64, 16, cpu); \
    SETUP_LUMA_SP_FUNC_DEF(16, 64, cpu);

#define LUMA_SS_FILTERS(cpu) \
    SETUP_LUMA_SS_FUNC_DEF(4,   4, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(8,   8, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(8,   4, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(4,   8, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(16, 16, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(16,  8, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(8,  16, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(16, 12, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(12, 16, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(16,  4, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(4,  16, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(32, 32, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(32, 16, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(16, 32, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(32, 24, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(24, 32, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(32,  8, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(8,  32, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(64, 64, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(64, 32, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(32, 64, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(64, 48, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(48, 64, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(64, 16, cpu); \
    SETUP_LUMA_SS_FUNC_DEF(16, 64, cpu);

#define SETUP_PIXEL_VAR_DEF(W, H, cpu) \
    p.cu[BLOCK_ ## W ## x ## H].var = x265_pixel_var_ ## W ## x ## H ## cpu;

#define LUMA_VAR(cpu) \
    SETUP_PIXEL_VAR_DEF(8,   8, cpu); \
    SETUP_PIXEL_VAR_DEF(16, 16, cpu); \
    SETUP_PIXEL_VAR_DEF(32, 32, cpu); \
    SETUP_PIXEL_VAR_DEF(64, 64, cpu);

#define SETUP_LUMA_ADDAVG_FUNC_DEF(W, H, cpu) \
    p.pu[LUMA_ ## W ## x ## H].addAvg = x265_addAvg_ ## W ## x ## H ## cpu;

#define LUMA_ADDAVG(cpu) \
    SETUP_LUMA_ADDAVG_FUNC_DEF(4,  4,  cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(4,  8,  cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(4,  16, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(8,  4,  cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(8,  8,  cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(8,  16, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(8,  32, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(12, 16, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(16, 4,  cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(16, 8,  cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(16, 12, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(16, 16, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(16, 32, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(24, 32, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(16, 64, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(32, 8,  cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(32, 16, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(32, 24, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(32, 32, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(32, 64, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(48, 64, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(64, 16, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(64, 32, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(64, 48, cpu); \
    SETUP_LUMA_ADDAVG_FUNC_DEF(64, 64, cpu); \

#define SETUP_CHROMA_ADDAVG_FUNC_DEF(W, H, cpu) \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].addAvg = x265_addAvg_ ## W ## x ## H ## cpu;

#define CHROMA_ADDAVG(cpu) \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(2,  4,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(2,  8,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(4,  2,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(4,  4,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(4,  8,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(4,  16, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(6,  8,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(8,  2,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(8,  4,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(8,  6,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(8,  8,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(8,  16, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(8,  32, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(16, 4,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(16, 8,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(32, 8,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF(32, 32, cpu);

#define SETUP_CHROMA_ADDAVG_FUNC_DEF_422(W, H, cpu) \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].addAvg = x265_addAvg_ ## W ## x ## H ## cpu;

#define CHROMA_ADDAVG_422(cpu) \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(2,  8,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(2, 16,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(4,  4,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(4,  8,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(4, 16,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(4, 32,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(6, 16,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(8,  4,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(8,  8,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(8, 12,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(8, 16,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(8, 32,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(8, 64,  cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(12, 32, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(16,  8, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(16, 16, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(16, 24, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(16, 32, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(16, 64, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(24, 64, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(32, 16, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(32, 32, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(32, 48, cpu); \
    SETUP_CHROMA_ADDAVG_FUNC_DEF_422(32, 64, cpu);

#define SETUP_INTRA_ANG_COMMON(mode, fno, cpu) \
    p.intra_pred[mode][BLOCK_4x4] = x265_intra_pred_ang4_ ## fno ## _ ## cpu; \
    p.intra_pred[mode][BLOCK_8x8] = x265_intra_pred_ang8_ ## fno ## _ ## cpu; \
    p.intra_pred[mode][BLOCK_16x16] = x265_intra_pred_ang16_ ## fno ## _ ## cpu; \
    p.intra_pred[mode][BLOCK_32x32] = x265_intra_pred_ang32_ ## fno ## _ ## cpu;

#define SETUP_INTRA_ANG4(mode, fno, cpu) \
    p.intra_pred[mode][BLOCK_4x4] = x265_intra_pred_ang4_ ## fno ## _ ## cpu;

#define SETUP_INTRA_ANG16_32(mode, fno, cpu) \
    p.intra_pred[mode][BLOCK_16x16] = x265_intra_pred_ang16_ ## fno ## _ ## cpu; \
    p.intra_pred[mode][BLOCK_32x32] = x265_intra_pred_ang32_ ## fno ## _ ## cpu;

#define SETUP_INTRA_ANG4_8(mode, fno, cpu) \
    p.intra_pred[mode][BLOCK_4x4] = x265_intra_pred_ang4_ ## fno ## _ ## cpu; \
    p.intra_pred[mode][BLOCK_8x8] = x265_intra_pred_ang8_ ## fno ## _ ## cpu;

#define INTRA_ANG_SSSE3(cpu) \
    SETUP_INTRA_ANG_COMMON(2, 2, cpu); \
    SETUP_INTRA_ANG_COMMON(34, 2, cpu);

#define INTRA_ANG_SSE4_COMMON(cpu) \
    SETUP_INTRA_ANG_COMMON(3,  3,  cpu); \
    SETUP_INTRA_ANG_COMMON(4,  4,  cpu); \
    SETUP_INTRA_ANG_COMMON(5,  5,  cpu); \
    SETUP_INTRA_ANG_COMMON(6,  6,  cpu); \
    SETUP_INTRA_ANG_COMMON(7,  7,  cpu); \
    SETUP_INTRA_ANG_COMMON(8,  8,  cpu); \
    SETUP_INTRA_ANG_COMMON(9,  9,  cpu); \
    SETUP_INTRA_ANG_COMMON(10, 10, cpu); \
    SETUP_INTRA_ANG_COMMON(11, 11, cpu); \
    SETUP_INTRA_ANG_COMMON(12, 12, cpu); \
    SETUP_INTRA_ANG_COMMON(13, 13, cpu); \
    SETUP_INTRA_ANG_COMMON(14, 14, cpu); \
    SETUP_INTRA_ANG_COMMON(15, 15, cpu); \
    SETUP_INTRA_ANG_COMMON(16, 16, cpu); \
    SETUP_INTRA_ANG_COMMON(17, 17, cpu); \
    SETUP_INTRA_ANG_COMMON(18, 18, cpu);

#define SETUP_INTRA_ANG_HIGH(mode, fno, cpu) \
    p.intra_pred[mode][BLOCK_8x8] = x265_intra_pred_ang8_ ## fno ## _ ## cpu; \
    p.intra_pred[mode][BLOCK_16x16] = x265_intra_pred_ang16_ ## fno ## _ ## cpu; \
    p.intra_pred[mode][BLOCK_32x32] = x265_intra_pred_ang32_ ## fno ## _ ## cpu;

#define INTRA_ANG_SSE4_HIGH(cpu) \
    SETUP_INTRA_ANG_HIGH(19, 19, cpu); \
    SETUP_INTRA_ANG_HIGH(20, 20, cpu); \
    SETUP_INTRA_ANG_HIGH(21, 21, cpu); \
    SETUP_INTRA_ANG_HIGH(22, 22, cpu); \
    SETUP_INTRA_ANG_HIGH(23, 23, cpu); \
    SETUP_INTRA_ANG_HIGH(24, 24, cpu); \
    SETUP_INTRA_ANG_HIGH(25, 25, cpu); \
    SETUP_INTRA_ANG_HIGH(26, 26, cpu); \
    SETUP_INTRA_ANG_HIGH(27, 27, cpu); \
    SETUP_INTRA_ANG_HIGH(28, 28, cpu); \
    SETUP_INTRA_ANG_HIGH(29, 29, cpu); \
    SETUP_INTRA_ANG_HIGH(30, 30, cpu); \
    SETUP_INTRA_ANG_HIGH(31, 31, cpu); \
    SETUP_INTRA_ANG_HIGH(32, 32, cpu); \
    SETUP_INTRA_ANG_HIGH(33, 33, cpu); \
    SETUP_INTRA_ANG4(19, 17, cpu); \
    SETUP_INTRA_ANG4(20, 16, cpu); \
    SETUP_INTRA_ANG4(21, 15, cpu); \
    SETUP_INTRA_ANG4(22, 14, cpu); \
    SETUP_INTRA_ANG4(23, 13, cpu); \
    SETUP_INTRA_ANG4(24, 12, cpu); \
    SETUP_INTRA_ANG4(25, 11, cpu); \
    SETUP_INTRA_ANG4(26, 26, cpu); \
    SETUP_INTRA_ANG4(27, 9, cpu); \
    SETUP_INTRA_ANG4(28, 8, cpu); \
    SETUP_INTRA_ANG4(29, 7, cpu); \
    SETUP_INTRA_ANG4(30, 6, cpu); \
    SETUP_INTRA_ANG4(31, 5, cpu); \
    SETUP_INTRA_ANG4(32, 4, cpu); \
    SETUP_INTRA_ANG4(33, 3, cpu);

#define INTRA_ANG_SSE4(cpu) \
    SETUP_INTRA_ANG4_8(19, 17, cpu); \
    SETUP_INTRA_ANG4_8(20, 16, cpu); \
    SETUP_INTRA_ANG4_8(21, 15, cpu); \
    SETUP_INTRA_ANG4_8(22, 14, cpu); \
    SETUP_INTRA_ANG4_8(23, 13, cpu); \
    SETUP_INTRA_ANG4_8(24, 12, cpu); \
    SETUP_INTRA_ANG4_8(25, 11, cpu); \
    SETUP_INTRA_ANG4_8(26, 26, cpu); \
    SETUP_INTRA_ANG4_8(27, 9, cpu); \
    SETUP_INTRA_ANG4_8(28, 8, cpu); \
    SETUP_INTRA_ANG4_8(29, 7, cpu); \
    SETUP_INTRA_ANG4_8(30, 6, cpu); \
    SETUP_INTRA_ANG4_8(31, 5, cpu); \
    SETUP_INTRA_ANG4_8(32, 4, cpu); \
    SETUP_INTRA_ANG4_8(33, 3, cpu); \
    SETUP_INTRA_ANG16_32(19, 19, cpu); \
    SETUP_INTRA_ANG16_32(20, 20, cpu); \
    SETUP_INTRA_ANG16_32(21, 21, cpu); \
    SETUP_INTRA_ANG16_32(22, 22, cpu); \
    SETUP_INTRA_ANG16_32(23, 23, cpu); \
    SETUP_INTRA_ANG16_32(24, 24, cpu); \
    SETUP_INTRA_ANG16_32(25, 25, cpu); \
    SETUP_INTRA_ANG16_32(26, 26, cpu); \
    SETUP_INTRA_ANG16_32(27, 27, cpu); \
    SETUP_INTRA_ANG16_32(28, 28, cpu); \
    SETUP_INTRA_ANG16_32(29, 29, cpu); \
    SETUP_INTRA_ANG16_32(30, 30, cpu); \
    SETUP_INTRA_ANG16_32(31, 31, cpu); \
    SETUP_INTRA_ANG16_32(32, 32, cpu); \
    SETUP_INTRA_ANG16_32(33, 33, cpu);

#define SETUP_CHROMA_VERT_FUNC_DEF(W, H, cpu) \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].filter_vss = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].filter_vpp = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].filter_vps = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].filter_vsp = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

#define CHROMA_VERT_FILTERS(cpu) \
    SETUP_CHROMA_VERT_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(8, 6, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(8, 2, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(8, 32, cpu);

#define CHROMA_VERT_FILTERS_SSE4(cpu) \
    SETUP_CHROMA_VERT_FUNC_DEF(2, 4, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(4, 2, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF(6, 8, cpu);

#define SETUP_CHROMA_VERT_FUNC_DEF_422(W, H, cpu) \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].filter_vss = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].filter_vpp = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].filter_vps = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].filter_vsp = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

#define CHROMA_VERT_FILTERS_422(cpu) \
    SETUP_CHROMA_VERT_FUNC_DEF_422(4, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(8, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(8, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(4, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(8, 12, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(8, 4, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(16, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(16, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(8, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(16, 24, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(12, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(16, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(4, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(32, 64, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(32, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(16, 64, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(32, 48, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(24, 64, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(32, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(8, 64, cpu);

#define CHROMA_VERT_FILTERS_SSE4_422(cpu) \
    SETUP_CHROMA_VERT_FUNC_DEF_422(2, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(2, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(4, 4, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_422(6, 16, cpu);

#define SETUP_CHROMA_VERT_FUNC_DEF_444(W, H, cpu) \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vss = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vpp = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vps = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_vsp = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

#define CHROMA_VERT_FILTERS_444(cpu) \
    SETUP_CHROMA_VERT_FUNC_DEF_444(8, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(8, 4, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(4, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(16, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(16, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(8, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(16, 12, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(12, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(16, 4, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(4, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(32, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(32, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(16, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(32, 24, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(24, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(32, 8, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(8, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(64, 64, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(64, 32, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(32, 64, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(64, 48, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(48, 64, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(64, 16, cpu); \
    SETUP_CHROMA_VERT_FUNC_DEF_444(16, 64, cpu);

#define SETUP_CHROMA_HORIZ_FUNC_DEF(W, H, cpu) \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].filter_hpp = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].pu[CHROMA_ ## W ## x ## H].filter_hps = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu;

#define CHROMA_HORIZ_FILTERS(cpu) \
    SETUP_CHROMA_HORIZ_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(4, 2, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(2, 4, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(8, 6, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(6, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(8, 2, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF(8, 32, cpu);

#define SETUP_CHROMA_HORIZ_FUNC_DEF_422(W, H, cpu) \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].filter_hpp = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].pu[CHROMA422_ ## W ## x ## H].filter_hps = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu;

#define CHROMA_HORIZ_FILTERS_422(cpu) \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(4, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(4, 4, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(2, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(8, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(8, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(4, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(8, 12, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(6, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(8, 4, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(2, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(16, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(16, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(8, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(16, 24, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(12, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(16, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(4, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(32, 64, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(32, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(16, 64, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(32, 48, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(24, 64, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(32, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_422(8, 64, cpu);

#define SETUP_CHROMA_HORIZ_FUNC_DEF_444(W, H, cpu) \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_hpp = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].pu[LUMA_ ## W ## x ## H].filter_hps = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu;

#define CHROMA_HORIZ_FILTERS_444(cpu) \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(8, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(8, 4, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(4, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(16, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(16, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(8, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(16, 12, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(12, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(16, 4, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(4, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(32, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(32, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(16, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(32, 24, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(24, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(32, 8, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(8, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(64, 64, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(64, 32, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(32, 64, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(64, 48, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(48, 64, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(64, 16, cpu); \
    SETUP_CHROMA_HORIZ_FUNC_DEF_444(16, 64, cpu);

namespace x265 {
// private x265 namespace

void setupAssemblyPrimitives(EncoderPrimitives &p, int cpuMask)
{
#if HIGH_BIT_DEPTH
    if (cpuMask & X265_CPU_SSE2)
    {
        INIT8(sad, _mmx2);
        INIT2(sad, _sse2);
        SAD(sse2);

        INIT6(satd, _sse2);
        HEVC_SATD(sse2);
        p.pu[LUMA_4x4].satd = x265_pixel_satd_4x4_mmx2;

        SA8D_INTER_FROM_BLOCK(sse2);

        p.cu[BLOCK_4x4].sse_ss = x265_pixel_ssd_ss_4x4_mmx2;
        p.cu[BLOCK_8x8].sse_ss   = x265_pixel_ssd_ss_8x8_sse2;
        p.cu[BLOCK_16x16].sse_ss = x265_pixel_ssd_ss_16x16_sse2;
        p.cu[BLOCK_32x32].sse_ss = x265_pixel_ssd_ss_32x32_sse2;
        p.cu[BLOCK_64x64].sse_ss = x265_pixel_ssd_ss_64x64_sse2;

        p.cu[BLOCK_4x4].transpose   = x265_transpose4_sse2;
        p.cu[BLOCK_8x8].transpose   = x265_transpose8_sse2;
        p.cu[BLOCK_16x16].transpose = x265_transpose16_sse2;
        p.cu[BLOCK_32x32].transpose = x265_transpose32_sse2;
        p.cu[BLOCK_64x64].transpose = x265_transpose64_sse2;

        p.ssim_4x4x2_core = x265_pixel_ssim_4x4x2_core_sse2;
        p.ssim_end_4 = x265_pixel_ssim_end4_sse2;
        PIXEL_AVG(sse2);
        PIXEL_AVG_W4(mmx2);
        LUMA_VAR(_sse2);

        SAD_X3(sse2);
        p.pu[LUMA_4x4].sad_x3   = x265_pixel_sad_x3_4x4_mmx2;
        p.pu[LUMA_4x8].sad_x3   = x265_pixel_sad_x3_4x8_mmx2;
        p.pu[LUMA_4x16].sad_x3  = x265_pixel_sad_x3_4x16_mmx2;
        p.pu[LUMA_8x4].sad_x3   = x265_pixel_sad_x3_8x4_sse2;
        p.pu[LUMA_8x8].sad_x3   = x265_pixel_sad_x3_8x8_sse2;
        p.pu[LUMA_8x16].sad_x3  = x265_pixel_sad_x3_8x16_sse2;
        p.pu[LUMA_8x32].sad_x3  = x265_pixel_sad_x3_8x32_sse2;
        p.pu[LUMA_16x4].sad_x3  = x265_pixel_sad_x3_16x4_sse2;
        p.pu[LUMA_12x16].sad_x3 = x265_pixel_sad_x3_12x16_mmx2;

        SAD_X4(sse2);
        p.pu[LUMA_4x4].sad_x4   = x265_pixel_sad_x4_4x4_mmx2;
        p.pu[LUMA_4x8].sad_x4   = x265_pixel_sad_x4_4x8_mmx2;
        p.pu[LUMA_4x16].sad_x4  = x265_pixel_sad_x4_4x16_mmx2;
        p.pu[LUMA_8x4].sad_x4   = x265_pixel_sad_x4_8x4_sse2;
        p.pu[LUMA_8x8].sad_x4   = x265_pixel_sad_x4_8x8_sse2;
        p.pu[LUMA_8x16].sad_x4  = x265_pixel_sad_x4_8x16_sse2;
        p.pu[LUMA_8x32].sad_x4  = x265_pixel_sad_x4_8x32_sse2;
        p.pu[LUMA_16x4].sad_x4  = x265_pixel_sad_x4_16x4_sse2;
        p.pu[LUMA_12x16].sad_x4 = x265_pixel_sad_x4_12x16_mmx2;

        p.cu[BLOCK_4x4].cpy2Dto1D_shl   = x265_cpy2Dto1D_shl_4_sse2;
        p.cu[BLOCK_8x8].cpy2Dto1D_shl   = x265_cpy2Dto1D_shl_8_sse2;
        p.cu[BLOCK_16x16].cpy2Dto1D_shl = x265_cpy2Dto1D_shl_16_sse2;
        p.cu[BLOCK_32x32].cpy2Dto1D_shl = x265_cpy2Dto1D_shl_32_sse2;
        p.cu[BLOCK_4x4].cpy2Dto1D_shr   = x265_cpy2Dto1D_shr_4_sse2;
        p.cu[BLOCK_8x8].cpy2Dto1D_shr   = x265_cpy2Dto1D_shr_8_sse2;
        p.cu[BLOCK_16x16].cpy2Dto1D_shr = x265_cpy2Dto1D_shr_16_sse2;
        p.cu[BLOCK_32x32].cpy2Dto1D_shr = x265_cpy2Dto1D_shr_32_sse2;
        p.cu[BLOCK_4x4].cpy1Dto2D_shl   = x265_cpy1Dto2D_shl_4_sse2;
        p.cu[BLOCK_8x8].cpy1Dto2D_shl   = x265_cpy1Dto2D_shl_8_sse2;
        p.cu[BLOCK_16x16].cpy1Dto2D_shl = x265_cpy1Dto2D_shl_16_sse2;
        p.cu[BLOCK_32x32].cpy1Dto2D_shl = x265_cpy1Dto2D_shl_32_sse2;
        p.cu[BLOCK_4x4].cpy1Dto2D_shr   = x265_cpy1Dto2D_shr_4_sse2;
        p.cu[BLOCK_8x8].cpy1Dto2D_shr   = x265_cpy1Dto2D_shr_8_sse2;
        p.cu[BLOCK_16x16].cpy1Dto2D_shr = x265_cpy1Dto2D_shr_16_sse2;
        p.cu[BLOCK_32x32].cpy1Dto2D_shr = x265_cpy1Dto2D_shr_32_sse2;

        CHROMA_PIXELSUB_PS(_sse2);
        CHROMA_PIXELSUB_PS_422(_sse2);
        LUMA_PIXELSUB(_sse2);

        CHROMA_CU_BLOCKCOPY(ss, _sse2);
        CHROMA_CU_BLOCKCOPY_422(ss, _sse2);
        LUMA_CU_BLOCKCOPY(ss, _sse2);

        CHROMA_VERT_FILTERS(_sse2);
        CHROMA_VERT_FILTERS_422(_sse2);
        CHROMA_VERT_FILTERS_444(_sse2);

        p.luma_p2s = x265_luma_p2s_sse2;
        p.chroma[X265_CSP_I420].p2s = x265_chroma_p2s_sse2;
        p.chroma[X265_CSP_I422].p2s = x265_chroma_p2s_sse2;

        p.cu[BLOCK_4x4].blockfill_s = x265_blockfill_s_4x4_sse2;
        p.cu[BLOCK_8x8].blockfill_s = x265_blockfill_s_8x8_sse2;
        p.cu[BLOCK_16x16].blockfill_s = x265_blockfill_s_16x16_sse2;
        p.cu[BLOCK_32x32].blockfill_s = x265_blockfill_s_32x32_sse2;

        // TODO: overflow on 12-bits mode!
        p.cu[BLOCK_4x4].ssd_s   = x265_pixel_ssd_s_4_sse2;
        p.cu[BLOCK_8x8].ssd_s   = x265_pixel_ssd_s_8_sse2;
        p.cu[BLOCK_16x16].ssd_s = x265_pixel_ssd_s_16_sse2;
        p.cu[BLOCK_32x32].ssd_s = x265_pixel_ssd_s_32_sse2;

        p.cu[BLOCK_4x4].calcresidual = x265_getResidual4_sse2;
        p.cu[BLOCK_8x8].calcresidual = x265_getResidual8_sse2;
        p.cu[BLOCK_16x16].calcresidual = x265_getResidual16_sse2;
        p.cu[BLOCK_32x32].calcresidual = x265_getResidual32_sse2;

        p.cu[BLOCK_4x4].dct = x265_dct4_sse2;
        p.cu[BLOCK_4x4].idct = x265_idct4_sse2;
#if X86_64
        p.cu[BLOCK_8x8].idct = x265_idct8_sse2;
#endif
        p.idst4x4 = x265_idst4_sse2;

        LUMA_SS_FILTERS(_sse2);
    }
    if (cpuMask & X265_CPU_SSSE3)
    {
        p.scale1D_128to64 = x265_scale1D_128to64_ssse3;
        p.scale2D_64to32 = x265_scale2D_64to32_ssse3;

        INTRA_ANG_SSSE3(ssse3);

        p.dst4x4 = x265_dst4_ssse3;
        p.cu[BLOCK_8x8].idct = x265_idct8_ssse3;
        p.count_nonzero = x265_count_nonzero_ssse3;
    }
    if (cpuMask & X265_CPU_SSE4)
    {
        LUMA_ADDAVG(_sse4);
        CHROMA_ADDAVG(_sse4);
        CHROMA_ADDAVG_422(_sse4);
        LUMA_FILTERS(_sse4);
        CHROMA_HORIZ_FILTERS(_sse4);
        CHROMA_VERT_FILTERS_SSE4(_sse4);
        CHROMA_HORIZ_FILTERS_422(_sse4);
        CHROMA_VERT_FILTERS_SSE4_422(_sse4);
        CHROMA_HORIZ_FILTERS_444(_sse4);

        p.cu[BLOCK_8x8].dct = x265_dct8_sse4;
        p.quant = x265_quant_sse4;
        p.nquant = x265_nquant_sse4;
        p.dequant_normal = x265_dequant_normal_sse4;

        p.intra_pred[0][BLOCK_4x4] = x265_intra_pred_planar4_sse4;
        p.intra_pred[0][BLOCK_8x8] = x265_intra_pred_planar8_sse4;
        p.intra_pred[0][BLOCK_16x16] = x265_intra_pred_planar16_sse4;
        p.intra_pred[0][BLOCK_32x32] = x265_intra_pred_planar32_sse4;

        p.intra_pred[1][BLOCK_4x4] = x265_intra_pred_dc4_sse4;
        p.intra_pred[1][BLOCK_8x8] = x265_intra_pred_dc8_sse4;
        p.intra_pred[1][BLOCK_16x16] = x265_intra_pred_dc16_sse4;
        p.intra_pred[1][BLOCK_32x32] = x265_intra_pred_dc32_sse4;

        p.planecopy_cp = x265_upShift_8_sse4;

        INTRA_ANG_SSE4_COMMON(sse4);
        INTRA_ANG_SSE4_HIGH(sse4);

        p.cu[BLOCK_4x4].psy_cost_pp = x265_psyCost_pp_4x4_sse4;
#if X86_64
        p.cu[BLOCK_8x8].psy_cost_pp = x265_psyCost_pp_8x8_sse4;
        p.cu[BLOCK_16x16].psy_cost_pp = x265_psyCost_pp_16x16_sse4;
        p.cu[BLOCK_32x32].psy_cost_pp = x265_psyCost_pp_32x32_sse4;
        p.cu[BLOCK_64x64].psy_cost_pp = x265_psyCost_pp_64x64_sse4;

        p.cu[BLOCK_8x8].psy_cost_ss = x265_psyCost_ss_8x8_sse4;
#endif
        p.cu[BLOCK_4x4].psy_cost_ss = x265_psyCost_ss_4x4_sse4;
    }
    if (cpuMask & X265_CPU_AVX)
    {
        p.pu[LUMA_64x64].copy_pp = (copy_pp_t)x265_blockcopy_ss_64x64_avx;
    }
    if (cpuMask & X265_CPU_XOP)
    {
        p.frameInitLowres = x265_frame_init_lowres_core_xop;
        SA8D_INTER_FROM_BLOCK(xop);
        INIT7(satd, _xop);
        HEVC_SATD(xop);
    }
    if (cpuMask & X265_CPU_AVX2)
    {
        p.cu[BLOCK_4x4].dct = x265_dct4_avx2;
        p.quant = x265_quant_avx2;
        p.nquant = x265_nquant_avx2;
        p.dequant_normal  = x265_dequant_normal_avx2;
        p.scale1D_128to64= x265_scale1D_128to64_avx2;
        p.cu[BLOCK_4x4].cpy1Dto2D_shl = x265_cpy1Dto2D_shl_4_avx2;
        p.cu[BLOCK_8x8].cpy1Dto2D_shl = x265_cpy1Dto2D_shl_8_avx2;
        p.cu[BLOCK_16x16].cpy1Dto2D_shl = x265_cpy1Dto2D_shl_16_avx2;
        p.cu[BLOCK_32x32].cpy1Dto2D_shl = x265_cpy1Dto2D_shl_32_avx2;
        p.cu[BLOCK_4x4].cpy1Dto2D_shr = x265_cpy1Dto2D_shr_4_avx2;
        p.cu[BLOCK_8x8].cpy1Dto2D_shr = x265_cpy1Dto2D_shr_8_avx2;
        p.cu[BLOCK_16x16].cpy1Dto2D_shr = x265_cpy1Dto2D_shr_16_avx2;
        p.cu[BLOCK_32x32].cpy1Dto2D_shr = x265_cpy1Dto2D_shr_32_avx2;
#if X86_64
        p.cu[BLOCK_8x8].dct   = x265_dct8_avx2;
        p.cu[BLOCK_16x16].dct = x265_dct16_avx2;
        p.cu[BLOCK_32x32].dct = x265_dct32_avx2;
        p.cu[BLOCK_4x4].idct  = x265_idct4_avx2;
        p.cu[BLOCK_8x8].idct  = x265_idct8_avx2;
        p.cu[BLOCK_16x16].idct = x265_idct16_avx2;
        p.cu[BLOCK_32x32].idct = x265_idct32_avx2;
        p.cu[BLOCK_8x8].transpose = x265_transpose8_avx2;
        p.cu[BLOCK_16x16].transpose = x265_transpose16_avx2;
        p.cu[BLOCK_32x32].transpose = x265_transpose32_avx2;
        p.cu[BLOCK_64x64].transpose = x265_transpose64_avx2;
#endif
        p.pu[LUMA_64x16].copy_pp = (copy_pp_t)x265_blockcopy_ss_64x16_avx;
        p.pu[LUMA_64x32].copy_pp = (copy_pp_t)x265_blockcopy_ss_64x32_avx;
        p.pu[LUMA_64x48].copy_pp = (copy_pp_t)x265_blockcopy_ss_64x48_avx;
        p.pu[LUMA_64x64].copy_pp = (copy_pp_t)x265_blockcopy_ss_64x64_avx;
    }

    /* at HIGH_BIT_DEPTH, pixel == short so we can reuse a number of primitives */
    for (int i = 0; i < NUM_SQUARE_BLOCKS; i++)
    {
        p.cu[i].sse_ss  = (pixelcmp_ss_t)p.cu[i].sse_pp;
        p.cu[i].copy_ps = (copy_ps_t)p.pu[i].copy_pp;
        p.cu[i].copy_sp = (copy_sp_t)p.pu[i].copy_pp;
        p.cu[i].copy_ss = (copy_ss_t)p.pu[i].copy_pp;

        p.chroma[X265_CSP_I420].cu[i].copy_ps = (copy_ps_t)p.chroma[X265_CSP_I420].cu[i].copy_ss;
        p.chroma[X265_CSP_I420].cu[i].copy_sp = (copy_sp_t)p.chroma[X265_CSP_I420].cu[i].copy_ss;
        p.chroma[X265_CSP_I420].pu[i].copy_pp = (copy_pp_t)p.chroma[X265_CSP_I420].cu[i].copy_ss;

        p.chroma[X265_CSP_I422].cu[i].copy_ps = (copy_ps_t)p.chroma[X265_CSP_I422].cu[i].copy_ss;
        p.chroma[X265_CSP_I422].cu[i].copy_sp = (copy_sp_t)p.chroma[X265_CSP_I422].cu[i].copy_ss;
        p.chroma[X265_CSP_I422].pu[i].copy_pp = (copy_pp_t)p.chroma[X265_CSP_I422].cu[i].copy_ss;
    }

#else // if HIGH_BIT_DEPTH
    if (cpuMask & X265_CPU_SSE2)
    {
        INIT8(sad, _mmx2);
        INIT8(sad_x3, _mmx2);
        INIT8(sad_x4, _mmx2);
        p.pu[LUMA_4x4].satd = x265_pixel_satd_4x4_mmx2;
        p.frameInitLowres = x265_frame_init_lowres_core_mmx2;

        p.cu[BLOCK_4x4].sse_pp = x265_pixel_ssd_4x4_mmx;
        p.cu[BLOCK_8x8].sse_pp = x265_pixel_ssd_8x8_mmx;
        p.cu[BLOCK_16x16].sse_pp = x265_pixel_ssd_16x16_mmx;

        PIXEL_AVG(sse2);
        PIXEL_AVG_W4(mmx2);

        LUMA_VAR(_sse2);

        ASSGN_SSE(sse2);
        ASSGN_SSE_SS(sse2);
        INIT2(sad, _sse2);
        SAD(sse2);
        INIT2(sad_x3, _sse2);
        INIT2(sad_x4, _sse2);
        HEVC_SATD(sse2);

        CHROMA_BLOCKCOPY(pp, _sse2);
        LUMA_BLOCKCOPY(pp, _sse2);
        LUMA_CU_BLOCKCOPY(ss, _sse2);
        CHROMA_BLOCKCOPY_422(pp, _sse2);

        CHROMA_CU_BLOCKCOPY(ss, _sse2);
        CHROMA_CU_BLOCKCOPY_422(ss, _sse2);
        CHROMA_CU_BLOCKCOPY(sp, _sse2);
        CHROMA_CU_BLOCKCOPY_422(sp, _sse2);
        LUMA_CU_BLOCKCOPY(sp, _sse2);

        CHROMA_SS_FILTERS_420(_sse2);
        CHROMA_SS_FILTERS_422(_sse2);
        CHROMA_SS_FILTERS_444(_sse2);
        CHROMA_SP_FILTERS_420(_sse2);
        CHROMA_SP_FILTERS_422(_sse2);
        CHROMA_SP_FILTERS_444(_sse2);
        LUMA_SS_FILTERS(_sse2);

        // This function pointer initialization is temporary will be removed
        // later with macro definitions.  It is used to avoid linker errors
        // until all partitions are coded and commit smaller patches, easier to
        // review.

        p.cu[BLOCK_4x4].blockfill_s = x265_blockfill_s_4x4_sse2;
        p.cu[BLOCK_8x8].blockfill_s = x265_blockfill_s_8x8_sse2;
        p.cu[BLOCK_16x16].blockfill_s = x265_blockfill_s_16x16_sse2;
        p.cu[BLOCK_32x32].blockfill_s = x265_blockfill_s_32x32_sse2;

        p.cu[BLOCK_4x4].ssd_s = x265_pixel_ssd_s_4_sse2;
        p.cu[BLOCK_8x8].ssd_s = x265_pixel_ssd_s_8_sse2;
        p.cu[BLOCK_16x16].ssd_s = x265_pixel_ssd_s_16_sse2;
        p.cu[BLOCK_32x32].ssd_s = x265_pixel_ssd_s_32_sse2;

        p.frameInitLowres = x265_frame_init_lowres_core_sse2;
        SA8D_INTER_FROM_BLOCK(sse2);

        p.cu[BLOCK_4x4].cpy2Dto1D_shl   = x265_cpy2Dto1D_shl_4_sse2;
        p.cu[BLOCK_8x8].cpy2Dto1D_shl   = x265_cpy2Dto1D_shl_8_sse2;
        p.cu[BLOCK_16x16].cpy2Dto1D_shl = x265_cpy2Dto1D_shl_16_sse2;
        p.cu[BLOCK_32x32].cpy2Dto1D_shl = x265_cpy2Dto1D_shl_32_sse2;
        p.cu[BLOCK_4x4].cpy2Dto1D_shr   = x265_cpy2Dto1D_shr_4_sse2;
        p.cu[BLOCK_8x8].cpy2Dto1D_shr   = x265_cpy2Dto1D_shr_8_sse2;
        p.cu[BLOCK_16x16].cpy2Dto1D_shr = x265_cpy2Dto1D_shr_16_sse2;
        p.cu[BLOCK_32x32].cpy2Dto1D_shr = x265_cpy2Dto1D_shr_32_sse2;
        p.cu[BLOCK_4x4].cpy1Dto2D_shl   = x265_cpy1Dto2D_shl_4_sse2;
        p.cu[BLOCK_8x8].cpy1Dto2D_shl   = x265_cpy1Dto2D_shl_8_sse2;
        p.cu[BLOCK_16x16].cpy1Dto2D_shl = x265_cpy1Dto2D_shl_16_sse2;
        p.cu[BLOCK_32x32].cpy1Dto2D_shl = x265_cpy1Dto2D_shl_32_sse2;
        p.cu[BLOCK_4x4].cpy1Dto2D_shr   = x265_cpy1Dto2D_shr_4_sse2;
        p.cu[BLOCK_8x8].cpy1Dto2D_shr   = x265_cpy1Dto2D_shr_8_sse2;
        p.cu[BLOCK_16x16].cpy1Dto2D_shr = x265_cpy1Dto2D_shr_16_sse2;
        p.cu[BLOCK_32x32].cpy1Dto2D_shr = x265_cpy1Dto2D_shr_32_sse2;

        p.cu[BLOCK_4x4].calcresidual = x265_getResidual4_sse2;
        p.cu[BLOCK_8x8].calcresidual = x265_getResidual8_sse2;
        p.cu[BLOCK_4x4].transpose = x265_transpose4_sse2;
        p.cu[BLOCK_8x8].transpose = x265_transpose8_sse2;
        p.cu[BLOCK_16x16].transpose = x265_transpose16_sse2;
        p.cu[BLOCK_32x32].transpose = x265_transpose32_sse2;
        p.cu[BLOCK_64x64].transpose = x265_transpose64_sse2;
        p.ssim_4x4x2_core = x265_pixel_ssim_4x4x2_core_sse2;
        p.ssim_end_4 = x265_pixel_ssim_end4_sse2;

        p.cu[BLOCK_4x4].dct = x265_dct4_sse2;
        p.cu[BLOCK_4x4].idct = x265_idct4_sse2;
#if X86_64
        p.cu[BLOCK_8x8].idct = x265_idct8_sse2;
#endif
        p.idst4x4 = x265_idst4_sse2;

        p.planecopy_sp = x265_downShift_16_sse2;
    }
    if (cpuMask & X265_CPU_SSSE3)
    {
        p.frameInitLowres = x265_frame_init_lowres_core_ssse3;
        SA8D_INTER_FROM_BLOCK(ssse3);
        ASSGN_SSE(ssse3);
        PIXEL_AVG(ssse3);
        PIXEL_AVG_W4(ssse3);
        p.cu[BLOCK_4x4].sse_pp = x265_pixel_ssd_4x4_ssse3;

        INTRA_ANG_SSSE3(ssse3);

        p.scale1D_128to64 = x265_scale1D_128to64_ssse3;
        p.scale2D_64to32 = x265_scale2D_64to32_ssse3;
        SAD_X3(ssse3);
        SAD_X4(ssse3);
        p.pu[LUMA_8x4].sad_x4  = x265_pixel_sad_x4_8x4_ssse3;
        p.pu[LUMA_8x8].sad_x4  = x265_pixel_sad_x4_8x8_ssse3;
        p.pu[LUMA_8x16].sad_x3 = x265_pixel_sad_x3_8x16_ssse3;
        p.pu[LUMA_8x16].sad_x4 = x265_pixel_sad_x4_8x16_ssse3;
        p.pu[LUMA_8x32].sad_x3 = x265_pixel_sad_x3_8x32_ssse3;
        p.pu[LUMA_8x32].sad_x4 = x265_pixel_sad_x4_8x32_ssse3;

        p.pu[LUMA_12x16].sad_x3 = x265_pixel_sad_x3_12x16_ssse3;
        p.pu[LUMA_12x16].sad_x4 = x265_pixel_sad_x4_12x16_ssse3;

        p.luma_p2s = x265_luma_p2s_ssse3;
        p.chroma[X265_CSP_I420].p2s = x265_chroma_p2s_ssse3;
        p.chroma[X265_CSP_I422].p2s = x265_chroma_p2s_ssse3;

        p.dst4x4 = x265_dst4_ssse3;
        p.cu[BLOCK_8x8].idct = x265_idct8_ssse3;
        p.count_nonzero = x265_count_nonzero_ssse3;
    }
    if (cpuMask & X265_CPU_SSE4)
    {
        p.sign = x265_calSign_sse4;
        p.saoCuOrgE0 = x265_saoCuOrgE0_sse4;
        p.saoCuOrgE1 = x265_saoCuOrgE1_sse4;
        p.saoCuOrgE2 = x265_saoCuOrgE2_sse4;
        p.saoCuOrgE3 = x265_saoCuOrgE3_sse4;
        p.saoCuOrgB0 = x265_saoCuOrgB0_sse4;

        LUMA_ADDAVG(_sse4);
        CHROMA_ADDAVG(_sse4);
        CHROMA_ADDAVG_422(_sse4);

        // TODO: check POPCNT flag!
        p.cu[BLOCK_4x4].copy_cnt = x265_copy_cnt_4_sse4;
        p.cu[BLOCK_8x8].copy_cnt = x265_copy_cnt_8_sse4;
        p.cu[BLOCK_16x16].copy_cnt = x265_copy_cnt_16_sse4;
        p.cu[BLOCK_32x32].copy_cnt = x265_copy_cnt_32_sse4;

        HEVC_SATD(sse4);
        SA8D_INTER_FROM_BLOCK(sse4);

        p.cu[BLOCK_64x64].sse_pp = x265_pixel_ssd_64x64_sse4;

        CHROMA_PIXELSUB_PS(_sse4);
        CHROMA_PIXELSUB_PS_422(_sse4);
        LUMA_PIXELSUB(_sse4);

        CHROMA_FILTERS_420(_sse4);
        CHROMA_FILTERS_422(_sse4);
        CHROMA_FILTERS_444(_sse4);
        CHROMA_SS_FILTERS_SSE4_420(_sse4);
        CHROMA_SS_FILTERS_SSE4_422(_sse4);
        CHROMA_SP_FILTERS_SSE4_420(_sse4);
        CHROMA_SP_FILTERS_SSE4_422(_sse4);
        CHROMA_SP_FILTERS_SSE4_444(_sse4);
        LUMA_SP_FILTERS(_sse4);
        LUMA_FILTERS(_sse4);
        ASSGN_SSE_SS(sse4);

        // MUST be done after LUMA_FILTERS() to overwrite default version
        p.pu[LUMA_8x8].luma_hvpp = x265_interp_8tap_hv_pp_8x8_sse4;

        CHROMA_CU_BLOCKCOPY(ps, _sse4);
        CHROMA_CU_BLOCKCOPY_422(ps, _sse4);
        LUMA_CU_BLOCKCOPY(ps, _sse4);

        p.cu[BLOCK_16x16].calcresidual = x265_getResidual16_sse4;
        p.cu[BLOCK_32x32].calcresidual = x265_getResidual32_sse4;
        p.quant = x265_quant_sse4;
        p.nquant = x265_nquant_sse4;
        p.dequant_normal = x265_dequant_normal_sse4;
        p.weight_pp = x265_weight_pp_sse4;
        p.weight_sp = x265_weight_sp_sse4;

        p.intra_pred[0][BLOCK_4x4] = x265_intra_pred_planar4_sse4;
        p.intra_pred[0][BLOCK_8x8] = x265_intra_pred_planar8_sse4;
        p.intra_pred[0][BLOCK_16x16] = x265_intra_pred_planar16_sse4;
        p.intra_pred[0][BLOCK_32x32] = x265_intra_pred_planar32_sse4;

        p.intra_pred_allangs[BLOCK_4x4] = x265_all_angs_pred_4x4_sse4;
        p.intra_pred_allangs[BLOCK_8x8] = x265_all_angs_pred_8x8_sse4;
        p.intra_pred_allangs[BLOCK_16x16] = x265_all_angs_pred_16x16_sse4;
        p.intra_pred_allangs[BLOCK_32x32] = x265_all_angs_pred_32x32_sse4;

        p.intra_pred[1][BLOCK_4x4] = x265_intra_pred_dc4_sse4;
        p.intra_pred[1][BLOCK_8x8] = x265_intra_pred_dc8_sse4;
        p.intra_pred[1][BLOCK_16x16] = x265_intra_pred_dc16_sse4;
        p.intra_pred[1][BLOCK_32x32] = x265_intra_pred_dc32_sse4;

        INTRA_ANG_SSE4_COMMON(sse4);
        INTRA_ANG_SSE4(sse4);

        p.cu[BLOCK_8x8].dct = x265_dct8_sse4;
        p.denoiseDct = x265_denoise_dct_sse4;
        p.cu[BLOCK_4x4].psy_cost_pp = x265_psyCost_pp_4x4_sse4;
#if X86_64
        p.cu[BLOCK_8x8].psy_cost_pp = x265_psyCost_pp_8x8_sse4;
        p.cu[BLOCK_16x16].psy_cost_pp = x265_psyCost_pp_16x16_sse4;
        p.cu[BLOCK_32x32].psy_cost_pp = x265_psyCost_pp_32x32_sse4;
        p.cu[BLOCK_64x64].psy_cost_pp = x265_psyCost_pp_64x64_sse4;

        p.cu[BLOCK_8x8].psy_cost_ss = x265_psyCost_ss_8x8_sse4;
#endif
        p.cu[BLOCK_4x4].psy_cost_ss = x265_psyCost_ss_4x4_sse4;
    }
    if (cpuMask & X265_CPU_AVX)
    {
        p.frameInitLowres = x265_frame_init_lowres_core_avx;
        HEVC_SATD(avx);
        SA8D_INTER_FROM_BLOCK(avx);
        ASSGN_SSE(avx);

        ASSGN_SSE_SS(avx);
        SAD_X3(avx);
        SAD_X4(avx);
        p.pu[LUMA_12x16].sad_x3 = x265_pixel_sad_x3_12x16_avx;
        p.pu[LUMA_12x16].sad_x4 = x265_pixel_sad_x4_12x16_avx;
        p.pu[LUMA_16x4].sad_x3  = x265_pixel_sad_x3_16x4_avx;
        p.pu[LUMA_16x4].sad_x4  = x265_pixel_sad_x4_16x4_avx;

        p.ssim_4x4x2_core = x265_pixel_ssim_4x4x2_core_avx;
        p.ssim_end_4 = x265_pixel_ssim_end4_avx;
        p.cu[BLOCK_64x64].copy_ss = x265_blockcopy_ss_64x64_avx;

        p.chroma[X265_CSP_I420].pu[CHROMA_32x8].copy_pp = x265_blockcopy_pp_32x8_avx;
        p.pu[LUMA_32x8].copy_pp = x265_blockcopy_pp_32x8_avx;

        p.chroma[X265_CSP_I420].pu[CHROMA_32x16].copy_pp = x265_blockcopy_pp_32x16_avx;
        p.chroma[X265_CSP_I422].pu[CHROMA422_32x16].copy_pp = x265_blockcopy_pp_32x16_avx;
        p.pu[LUMA_32x16].copy_pp = x265_blockcopy_pp_32x16_avx;

        p.chroma[X265_CSP_I420].pu[CHROMA_32x24].copy_pp = x265_blockcopy_pp_32x24_avx;
        p.pu[LUMA_32x24].copy_pp = x265_blockcopy_pp_32x24_avx;

        p.chroma[X265_CSP_I420].pu[CHROMA_32x32].copy_pp = x265_blockcopy_pp_32x32_avx;
        p.chroma[X265_CSP_I422].pu[CHROMA422_32x32].copy_pp = x265_blockcopy_pp_32x32_avx;
        p.pu[LUMA_32x32].copy_pp  = x265_blockcopy_pp_32x32_avx;

        p.chroma[X265_CSP_I422].pu[CHROMA422_32x48].copy_pp = x265_blockcopy_pp_32x48_avx;

        p.chroma[X265_CSP_I422].pu[CHROMA422_32x64].copy_pp = x265_blockcopy_pp_32x64_avx;
        p.pu[LUMA_32x64].copy_pp = x265_blockcopy_pp_32x64_avx;
    }
    if (cpuMask & X265_CPU_XOP)
    {
        p.frameInitLowres = x265_frame_init_lowres_core_xop;
        SA8D_INTER_FROM_BLOCK(xop);
        INIT7(satd, _xop);
        HEVC_SATD(xop);
        p.cu[BLOCK_8x8].sse_pp = x265_pixel_ssd_8x8_xop;
        p.cu[BLOCK_16x16].sse_pp = x265_pixel_ssd_16x16_xop;
    }
    if (cpuMask & X265_CPU_AVX2)
    {
        INIT2(sad_x4, _avx2);
        INIT4(satd, _avx2);
        p.pu[LUMA_16x12].sad_x4 = x265_pixel_sad_x4_16x12_avx2;
        p.pu[LUMA_16x32].sad_x4 = x265_pixel_sad_x4_16x32_avx2;

        p.cu[BLOCK_16x16].sse_pp = x265_pixel_ssd_16x16_avx2;
        p.cu[BLOCK_32x32].ssd_s = x265_pixel_ssd_s_32_avx2;

        p.cu[BLOCK_8x8].copy_cnt = x265_copy_cnt_8_avx2;
        p.cu[BLOCK_16x16].copy_cnt = x265_copy_cnt_16_avx2;
        p.cu[BLOCK_32x32].copy_cnt = x265_copy_cnt_32_avx2;

        p.cu[BLOCK_16x16].blockfill_s = x265_blockfill_s_16x16_avx2;
        p.cu[BLOCK_32x32].blockfill_s = x265_blockfill_s_32x32_avx2;

        p.cu[BLOCK_4x4].cpy1Dto2D_shl   = x265_cpy1Dto2D_shl_4_avx2;
        p.cu[BLOCK_8x8].cpy1Dto2D_shl   = x265_cpy1Dto2D_shl_8_avx2;
        p.cu[BLOCK_16x16].cpy1Dto2D_shl = x265_cpy1Dto2D_shl_16_avx2;
        p.cu[BLOCK_32x32].cpy1Dto2D_shl = x265_cpy1Dto2D_shl_32_avx2;

        p.cu[BLOCK_4x4].cpy1Dto2D_shr   = x265_cpy1Dto2D_shr_4_avx2;
        p.cu[BLOCK_8x8].cpy1Dto2D_shr   = x265_cpy1Dto2D_shr_8_avx2;
        p.cu[BLOCK_16x16].cpy1Dto2D_shr = x265_cpy1Dto2D_shr_16_avx2;
        p.cu[BLOCK_32x32].cpy1Dto2D_shr = x265_cpy1Dto2D_shr_32_avx2;

        p.denoiseDct = x265_denoise_dct_avx2;
        p.cu[BLOCK_4x4].dct = x265_dct4_avx2;
        p.quant = x265_quant_avx2;
        p.nquant = x265_nquant_avx2;
        p.dequant_normal = x265_dequant_normal_avx2;

        p.chroma[X265_CSP_I420].cu[CHROMA_16x16].copy_ss = x265_blockcopy_ss_16x16_avx;
        p.chroma[X265_CSP_I420].cu[CHROMA_16x32].copy_ss = x265_blockcopy_ss_16x32_avx;
        p.chroma[X265_CSP_I422].cu[CHROMA422_16x32].copy_ss = x265_blockcopy_ss_16x32_avx;
        p.scale1D_128to64 = x265_scale1D_128to64_avx2;

        p.weight_pp = x265_weight_pp_avx2;

#if X86_64

        p.cu[BLOCK_8x8].dct    = x265_dct8_avx2;
        p.cu[BLOCK_16x16].dct  = x265_dct16_avx2;
        p.cu[BLOCK_32x32].dct  = x265_dct32_avx2;
        p.cu[BLOCK_4x4].idct   = x265_idct4_avx2;
        p.cu[BLOCK_8x8].idct   = x265_idct8_avx2;
        p.cu[BLOCK_16x16].idct = x265_idct16_avx2;
        p.cu[BLOCK_32x32].idct = x265_idct32_avx2;

        p.cu[BLOCK_8x8].transpose   = x265_transpose8_avx2;
        p.cu[BLOCK_16x16].transpose = x265_transpose16_avx2;
        p.cu[BLOCK_32x32].transpose = x265_transpose32_avx2;
        p.cu[BLOCK_64x64].transpose = x265_transpose64_avx2;

        p.pu[LUMA_12x16].luma_vpp = x265_interp_8tap_vert_pp_12x16_avx2;

        p.pu[LUMA_16x4].luma_vpp  = x265_interp_8tap_vert_pp_16x4_avx2;
        p.pu[LUMA_16x8].luma_vpp  = x265_interp_8tap_vert_pp_16x8_avx2;
        p.pu[LUMA_16x12].luma_vpp = x265_interp_8tap_vert_pp_16x12_avx2;
        p.pu[LUMA_16x16].luma_vpp = x265_interp_8tap_vert_pp_16x16_avx2;
        p.pu[LUMA_16x32].luma_vpp = x265_interp_8tap_vert_pp_16x32_avx2;
        p.pu[LUMA_16x64].luma_vpp = x265_interp_8tap_vert_pp_16x64_avx2;

        p.pu[LUMA_24x32].luma_vpp = x265_interp_8tap_vert_pp_24x32_avx2;

        p.pu[LUMA_32x8].luma_vpp  = x265_interp_8tap_vert_pp_32x8_avx2;
        p.pu[LUMA_32x16].luma_vpp = x265_interp_8tap_vert_pp_32x16_avx2;
        p.pu[LUMA_32x24].luma_vpp = x265_interp_8tap_vert_pp_32x24_avx2;
        p.pu[LUMA_32x32].luma_vpp = x265_interp_8tap_vert_pp_32x32_avx2;
        p.pu[LUMA_32x64].luma_vpp = x265_interp_8tap_vert_pp_32x64_avx2;

        p.pu[LUMA_48x64].luma_vpp = x265_interp_8tap_vert_pp_48x64_avx2;

        p.pu[LUMA_64x16].luma_vpp = x265_interp_8tap_vert_pp_64x16_avx2;
        p.pu[LUMA_64x32].luma_vpp = x265_interp_8tap_vert_pp_64x32_avx2;
        p.pu[LUMA_64x48].luma_vpp = x265_interp_8tap_vert_pp_64x48_avx2;
        p.pu[LUMA_64x64].luma_vpp = x265_interp_8tap_vert_pp_64x64_avx2;
#endif
        p.pu[LUMA_4x4].luma_hpp = x265_interp_8tap_horiz_pp_4x4_avx2;

        p.pu[LUMA_8x4].luma_hpp = x265_interp_8tap_horiz_pp_8x4_avx2;
        p.pu[LUMA_8x8].luma_hpp = x265_interp_8tap_horiz_pp_8x8_avx2;
        p.pu[LUMA_8x16].luma_hpp = x265_interp_8tap_horiz_pp_8x16_avx2;
        p.pu[LUMA_8x32].luma_hpp = x265_interp_8tap_horiz_pp_8x32_avx2;

        p.pu[LUMA_16x4].luma_hpp = x265_interp_8tap_horiz_pp_16x4_avx2;
        p.pu[LUMA_16x8].luma_hpp = x265_interp_8tap_horiz_pp_16x8_avx2;
        p.pu[LUMA_16x12].luma_hpp = x265_interp_8tap_horiz_pp_16x12_avx2;
        p.pu[LUMA_16x16].luma_hpp = x265_interp_8tap_horiz_pp_16x16_avx2;
        p.pu[LUMA_16x32].luma_hpp = x265_interp_8tap_horiz_pp_16x32_avx2;
        p.pu[LUMA_16x64].luma_hpp = x265_interp_8tap_horiz_pp_16x64_avx2;

        p.pu[LUMA_32x8].luma_hpp  = x265_interp_8tap_horiz_pp_32x8_avx2;
        p.pu[LUMA_32x16].luma_hpp = x265_interp_8tap_horiz_pp_32x16_avx2;
        p.pu[LUMA_32x24].luma_hpp = x265_interp_8tap_horiz_pp_32x24_avx2;
        p.pu[LUMA_32x32].luma_hpp = x265_interp_8tap_horiz_pp_32x32_avx2;
        p.pu[LUMA_32x64].luma_hpp = x265_interp_8tap_horiz_pp_32x64_avx2;

        p.pu[LUMA_64x64].luma_hpp = x265_interp_8tap_horiz_pp_64x64_avx2;
        p.pu[LUMA_64x48].luma_hpp = x265_interp_8tap_horiz_pp_64x48_avx2;
        p.pu[LUMA_64x32].luma_hpp = x265_interp_8tap_horiz_pp_64x32_avx2;
        p.pu[LUMA_64x16].luma_hpp = x265_interp_8tap_horiz_pp_64x16_avx2;

        p.pu[LUMA_48x64].luma_hpp = x265_interp_8tap_horiz_pp_48x64_avx2;

        p.chroma[X265_CSP_I420].pu[CHROMA_8x8].filter_hpp = x265_interp_4tap_horiz_pp_8x8_avx2;
        p.chroma[X265_CSP_I420].pu[CHROMA_4x4].filter_hpp = x265_interp_4tap_horiz_pp_4x4_avx2;
        p.chroma[X265_CSP_I420].pu[CHROMA_32x32].filter_hpp = x265_interp_4tap_horiz_pp_32x32_avx2;
        p.chroma[X265_CSP_I420].pu[CHROMA_16x16].filter_hpp = x265_interp_4tap_horiz_pp_16x16_avx2;

        p.pu[LUMA_4x4].luma_vps = x265_interp_8tap_vert_ps_4x4_avx2;
        p.pu[LUMA_4x4].luma_vpp = x265_interp_8tap_vert_pp_4x4_avx2;
        p.pu[LUMA_8x4].luma_vpp = x265_interp_8tap_vert_pp_8x4_avx2;
        p.pu[LUMA_8x8].luma_vpp = x265_interp_8tap_vert_pp_8x8_avx2;
        p.pu[LUMA_8x16].luma_vpp = x265_interp_8tap_vert_pp_8x16_avx2;
        p.pu[LUMA_8x32].luma_vpp = x265_interp_8tap_vert_pp_8x32_avx2;

        // color space i420
        p.chroma[X265_CSP_I420].pu[CHROMA_4x4].filter_vpp = x265_interp_4tap_vert_pp_4x4_avx2;
        p.chroma[X265_CSP_I420].pu[CHROMA_8x8].filter_vpp = x265_interp_4tap_vert_pp_8x8_avx2;

        // color space i422
        p.chroma[X265_CSP_I422].pu[CHROMA422_4x4].filter_vpp = x265_interp_4tap_vert_pp_4x4_avx2;

#if X86_64
        p.chroma[X265_CSP_I420].pu[CHROMA_16x16].filter_vpp = x265_interp_4tap_vert_pp_16x16_avx2;
        p.chroma[X265_CSP_I420].pu[CHROMA_32x32].filter_vpp = x265_interp_4tap_vert_pp_32x32_avx2;
#endif
    }
#endif // if HIGH_BIT_DEPTH
}
}

extern "C" {
#ifdef __INTEL_COMPILER

/* Agner's patch to Intel's CPU dispatcher from pages 131-132 of
 * http://agner.org/optimize/optimizing_cpp.pdf (2011-01-30)
 * adapted to x265's cpu schema. */

// Global variable indicating cpu
int __intel_cpu_indicator = 0;
// CPU dispatcher function
void x265_intel_cpu_indicator_init(void)
{
    uint32_t cpu = x265::cpu_detect();

    if (cpu & X265_CPU_AVX)
        __intel_cpu_indicator = 0x20000;
    else if (cpu & X265_CPU_SSE42)
        __intel_cpu_indicator = 0x8000;
    else if (cpu & X265_CPU_SSE4)
        __intel_cpu_indicator = 0x2000;
    else if (cpu & X265_CPU_SSSE3)
        __intel_cpu_indicator = 0x1000;
    else if (cpu & X265_CPU_SSE3)
        __intel_cpu_indicator = 0x800;
    else if (cpu & X265_CPU_SSE2 && !(cpu & X265_CPU_SSE2_IS_SLOW))
        __intel_cpu_indicator = 0x200;
    else if (cpu & X265_CPU_SSE)
        __intel_cpu_indicator = 0x80;
    else if (cpu & X265_CPU_MMX2)
        __intel_cpu_indicator = 8;
    else
        __intel_cpu_indicator = 1;
}

/* __intel_cpu_indicator_init appears to have a non-standard calling convention that
 * assumes certain registers aren't preserved, so we'll route it through a function
 * that backs up all the registers. */
void __intel_cpu_indicator_init(void)
{
    x265_safe_intel_cpu_indicator_init();
}

#else // ifdef __INTEL_COMPILER
void x265_intel_cpu_indicator_init(void) {}

#endif // ifdef __INTEL_COMPILER
}
