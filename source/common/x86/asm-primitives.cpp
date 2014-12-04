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

#define INIT2_NAME(name1, name2, cpu) \
    p.name1[LUMA_16x16] = x265_pixel_ ## name2 ## _16x16 ## cpu; \
    p.name1[LUMA_16x8]  = x265_pixel_ ## name2 ## _16x8 ## cpu;
#define INIT4_NAME(name1, name2, cpu) \
    INIT2_NAME(name1, name2, cpu) \
    p.name1[LUMA_8x16]  = x265_pixel_ ## name2 ## _8x16 ## cpu; \
    p.name1[LUMA_8x8]   = x265_pixel_ ## name2 ## _8x8 ## cpu;
#define INIT5_NAME(name1, name2, cpu) \
    INIT4_NAME(name1, name2, cpu) \
    p.name1[LUMA_8x4]   = x265_pixel_ ## name2 ## _8x4 ## cpu;
#define INIT6_NAME(name1, name2, cpu) \
    INIT5_NAME(name1, name2, cpu) \
    p.name1[LUMA_4x8]   = x265_pixel_ ## name2 ## _4x8 ## cpu;
#define INIT7_NAME(name1, name2, cpu) \
    INIT6_NAME(name1, name2, cpu) \
    p.name1[LUMA_4x4]   = x265_pixel_ ## name2 ## _4x4 ## cpu;
#define INIT8_NAME(name1, name2, cpu) \
    INIT7_NAME(name1, name2, cpu) \
    p.name1[LUMA_4x16]  = x265_pixel_ ## name2 ## _4x16 ## cpu;
#define INIT2(name, cpu) INIT2_NAME(name, name, cpu)
#define INIT4(name, cpu) INIT4_NAME(name, name, cpu)
#define INIT5(name, cpu) INIT5_NAME(name, name, cpu)
#define INIT6(name, cpu) INIT6_NAME(name, name, cpu)
#define INIT7(name, cpu) INIT7_NAME(name, name, cpu)
#define INIT8(name, cpu) INIT8_NAME(name, name, cpu)

#define HEVC_SATD(cpu) \
    p.satd[LUMA_4x8]   = x265_pixel_satd_4x8_ ## cpu; \
    p.satd[LUMA_4x16]   = x265_pixel_satd_4x16_ ## cpu; \
    p.satd[LUMA_8x4]   = x265_pixel_satd_8x4_ ## cpu; \
    p.satd[LUMA_8x8]   = x265_pixel_satd_8x8_ ## cpu; \
    p.satd[LUMA_8x16]   = x265_pixel_satd_8x16_ ## cpu; \
    p.satd[LUMA_8x32]   = x265_pixel_satd_8x32_ ## cpu; \
    p.satd[LUMA_12x16]   = x265_pixel_satd_12x16_ ## cpu; \
    p.satd[LUMA_16x4]   = x265_pixel_satd_16x4_ ## cpu; \
    p.satd[LUMA_16x8]   = x265_pixel_satd_16x8_ ## cpu; \
    p.satd[LUMA_16x12]   = x265_pixel_satd_16x12_ ## cpu; \
    p.satd[LUMA_16x16]   = x265_pixel_satd_16x16_ ## cpu; \
    p.satd[LUMA_16x32]   = x265_pixel_satd_16x32_ ## cpu; \
    p.satd[LUMA_16x64]   = x265_pixel_satd_16x64_ ## cpu; \
    p.satd[LUMA_24x32]   = x265_pixel_satd_24x32_ ## cpu; \
    p.satd[LUMA_32x8]   = x265_pixel_satd_32x8_ ## cpu; \
    p.satd[LUMA_32x16]   = x265_pixel_satd_32x16_ ## cpu; \
    p.satd[LUMA_32x24]   = x265_pixel_satd_32x24_ ## cpu; \
    p.satd[LUMA_32x32]   = x265_pixel_satd_32x32_ ## cpu; \
    p.satd[LUMA_32x64]   = x265_pixel_satd_32x64_ ## cpu; \
    p.satd[LUMA_48x64]   = x265_pixel_satd_48x64_ ## cpu; \
    p.satd[LUMA_64x16]   = x265_pixel_satd_64x16_ ## cpu; \
    p.satd[LUMA_64x32]   = x265_pixel_satd_64x32_ ## cpu; \
    p.satd[LUMA_64x48]   = x265_pixel_satd_64x48_ ## cpu; \
    p.satd[LUMA_64x64]   = x265_pixel_satd_64x64_ ## cpu;

#define SAD_X3(cpu) \
    p.sad_x3[LUMA_16x8] = x265_pixel_sad_x3_16x8_ ## cpu; \
    p.sad_x3[LUMA_16x12] = x265_pixel_sad_x3_16x12_ ## cpu; \
    p.sad_x3[LUMA_16x16] = x265_pixel_sad_x3_16x16_ ## cpu; \
    p.sad_x3[LUMA_16x32] = x265_pixel_sad_x3_16x32_ ## cpu; \
    p.sad_x3[LUMA_16x64] = x265_pixel_sad_x3_16x64_ ## cpu; \
    p.sad_x3[LUMA_32x8] = x265_pixel_sad_x3_32x8_ ## cpu; \
    p.sad_x3[LUMA_32x16] = x265_pixel_sad_x3_32x16_ ## cpu; \
    p.sad_x3[LUMA_32x24] = x265_pixel_sad_x3_32x24_ ## cpu; \
    p.sad_x3[LUMA_32x32] = x265_pixel_sad_x3_32x32_ ## cpu; \
    p.sad_x3[LUMA_32x64] = x265_pixel_sad_x3_32x64_ ## cpu; \
    p.sad_x3[LUMA_24x32] = x265_pixel_sad_x3_24x32_ ## cpu; \
    p.sad_x3[LUMA_48x64] = x265_pixel_sad_x3_48x64_ ## cpu; \
    p.sad_x3[LUMA_64x16] = x265_pixel_sad_x3_64x16_ ## cpu; \
    p.sad_x3[LUMA_64x32] = x265_pixel_sad_x3_64x32_ ## cpu; \
    p.sad_x3[LUMA_64x48] = x265_pixel_sad_x3_64x48_ ## cpu; \
    p.sad_x3[LUMA_64x64] = x265_pixel_sad_x3_64x64_ ## cpu

#define SAD_X4(cpu) \
    p.sad_x4[LUMA_16x8] = x265_pixel_sad_x4_16x8_ ## cpu; \
    p.sad_x4[LUMA_16x12] = x265_pixel_sad_x4_16x12_ ## cpu; \
    p.sad_x4[LUMA_16x16] = x265_pixel_sad_x4_16x16_ ## cpu; \
    p.sad_x4[LUMA_16x32] = x265_pixel_sad_x4_16x32_ ## cpu; \
    p.sad_x4[LUMA_16x64] = x265_pixel_sad_x4_16x64_ ## cpu; \
    p.sad_x4[LUMA_32x8] = x265_pixel_sad_x4_32x8_ ## cpu; \
    p.sad_x4[LUMA_32x16] = x265_pixel_sad_x4_32x16_ ## cpu; \
    p.sad_x4[LUMA_32x24] = x265_pixel_sad_x4_32x24_ ## cpu; \
    p.sad_x4[LUMA_32x32] = x265_pixel_sad_x4_32x32_ ## cpu; \
    p.sad_x4[LUMA_32x64] = x265_pixel_sad_x4_32x64_ ## cpu; \
    p.sad_x4[LUMA_24x32] = x265_pixel_sad_x4_24x32_ ## cpu; \
    p.sad_x4[LUMA_48x64] = x265_pixel_sad_x4_48x64_ ## cpu; \
    p.sad_x4[LUMA_64x16] = x265_pixel_sad_x4_64x16_ ## cpu; \
    p.sad_x4[LUMA_64x32] = x265_pixel_sad_x4_64x32_ ## cpu; \
    p.sad_x4[LUMA_64x48] = x265_pixel_sad_x4_64x48_ ## cpu; \
    p.sad_x4[LUMA_64x64] = x265_pixel_sad_x4_64x64_ ## cpu

#define SAD(cpu) \
    p.sad[LUMA_8x32]  = x265_pixel_sad_8x32_ ## cpu; \
    p.sad[LUMA_16x4]  = x265_pixel_sad_16x4_ ## cpu; \
    p.sad[LUMA_16x12] = x265_pixel_sad_16x12_ ## cpu; \
    p.sad[LUMA_16x32] = x265_pixel_sad_16x32_ ## cpu; \
    p.sad[LUMA_16x64] = x265_pixel_sad_16x64_ ## cpu; \
    p.sad[LUMA_32x8]  = x265_pixel_sad_32x8_ ## cpu; \
    p.sad[LUMA_32x16] = x265_pixel_sad_32x16_ ## cpu; \
    p.sad[LUMA_32x24] = x265_pixel_sad_32x24_ ## cpu; \
    p.sad[LUMA_32x32] = x265_pixel_sad_32x32_ ## cpu; \
    p.sad[LUMA_32x64] = x265_pixel_sad_32x64_ ## cpu; \
    p.sad[LUMA_64x16] = x265_pixel_sad_64x16_ ## cpu; \
    p.sad[LUMA_64x32] = x265_pixel_sad_64x32_ ## cpu; \
    p.sad[LUMA_64x48] = x265_pixel_sad_64x48_ ## cpu; \
    p.sad[LUMA_64x64] = x265_pixel_sad_64x64_ ## cpu; \
    p.sad[LUMA_48x64] = x265_pixel_sad_48x64_ ## cpu; \
    p.sad[LUMA_24x32] = x265_pixel_sad_24x32_ ## cpu; \
    p.sad[LUMA_12x16] = x265_pixel_sad_12x16_ ## cpu

#define ASSGN_SSE(cpu) \
    p.sse_pp[LUMA_8x8]   = x265_pixel_ssd_8x8_ ## cpu; \
    p.sse_pp[LUMA_8x4]   = x265_pixel_ssd_8x4_ ## cpu; \
    p.sse_pp[LUMA_16x16] = x265_pixel_ssd_16x16_ ## cpu; \
    p.sse_pp[LUMA_16x4]  = x265_pixel_ssd_16x4_ ## cpu; \
    p.sse_pp[LUMA_16x8]  = x265_pixel_ssd_16x8_ ## cpu; \
    p.sse_pp[LUMA_8x16]  = x265_pixel_ssd_8x16_ ## cpu; \
    p.sse_pp[LUMA_16x12] = x265_pixel_ssd_16x12_ ## cpu; \
    p.sse_pp[LUMA_32x32] = x265_pixel_ssd_32x32_ ## cpu; \
    p.sse_pp[LUMA_32x16] = x265_pixel_ssd_32x16_ ## cpu; \
    p.sse_pp[LUMA_16x32] = x265_pixel_ssd_16x32_ ## cpu; \
    p.sse_pp[LUMA_8x32]  = x265_pixel_ssd_8x32_ ## cpu; \
    p.sse_pp[LUMA_32x8]  = x265_pixel_ssd_32x8_ ## cpu; \
    p.sse_pp[LUMA_32x24] = x265_pixel_ssd_32x24_ ## cpu; \
    p.sse_pp[LUMA_32x64] = x265_pixel_ssd_32x64_ ## cpu; \
    p.sse_pp[LUMA_16x64] = x265_pixel_ssd_16x64_ ## cpu

#define ASSGN_SSE_SS(cpu) \
    p.sse_ss[LUMA_4x4]   = x265_pixel_ssd_ss_4x4_ ## cpu; \
    p.sse_ss[LUMA_4x8]   = x265_pixel_ssd_ss_4x8_ ## cpu; \
    p.sse_ss[LUMA_4x16]   = x265_pixel_ssd_ss_4x16_ ## cpu; \
    p.sse_ss[LUMA_8x4]   = x265_pixel_ssd_ss_8x4_ ## cpu; \
    p.sse_ss[LUMA_8x8]   = x265_pixel_ssd_ss_8x8_ ## cpu; \
    p.sse_ss[LUMA_8x16]   = x265_pixel_ssd_ss_8x16_ ## cpu; \
    p.sse_ss[LUMA_8x32]   = x265_pixel_ssd_ss_8x32_ ## cpu; \
    p.sse_ss[LUMA_12x16]   = x265_pixel_ssd_ss_12x16_ ## cpu; \
    p.sse_ss[LUMA_16x4]   = x265_pixel_ssd_ss_16x4_ ## cpu; \
    p.sse_ss[LUMA_16x8]   = x265_pixel_ssd_ss_16x8_ ## cpu; \
    p.sse_ss[LUMA_16x12]   = x265_pixel_ssd_ss_16x12_ ## cpu; \
    p.sse_ss[LUMA_16x16]   = x265_pixel_ssd_ss_16x16_ ## cpu; \
    p.sse_ss[LUMA_16x32]   = x265_pixel_ssd_ss_16x32_ ## cpu; \
    p.sse_ss[LUMA_16x64]   = x265_pixel_ssd_ss_16x64_ ## cpu; \
    p.sse_ss[LUMA_24x32]   = x265_pixel_ssd_ss_24x32_ ## cpu; \
    p.sse_ss[LUMA_32x8]   = x265_pixel_ssd_ss_32x8_ ## cpu; \
    p.sse_ss[LUMA_32x16]   = x265_pixel_ssd_ss_32x16_ ## cpu; \
    p.sse_ss[LUMA_32x24]   = x265_pixel_ssd_ss_32x24_ ## cpu; \
    p.sse_ss[LUMA_32x32]   = x265_pixel_ssd_ss_32x32_ ## cpu; \
    p.sse_ss[LUMA_32x64]   = x265_pixel_ssd_ss_32x64_ ## cpu; \
    p.sse_ss[LUMA_48x64]   = x265_pixel_ssd_ss_48x64_ ## cpu; \
    p.sse_ss[LUMA_64x16]   = x265_pixel_ssd_ss_64x16_ ## cpu; \
    p.sse_ss[LUMA_64x32]   = x265_pixel_ssd_ss_64x32_ ## cpu; \
    p.sse_ss[LUMA_64x48]   = x265_pixel_ssd_ss_64x48_ ## cpu; \
    p.sse_ss[LUMA_64x64]   = x265_pixel_ssd_ss_64x64_ ## cpu;

#define SA8D_INTER_FROM_BLOCK(cpu) \
    p.sa8d_inter[LUMA_4x8]   = x265_pixel_satd_4x8_ ## cpu; \
    p.sa8d_inter[LUMA_8x4]   = x265_pixel_satd_8x4_ ## cpu; \
    p.sa8d_inter[LUMA_4x16]  = x265_pixel_satd_4x16_ ## cpu; \
    p.sa8d_inter[LUMA_16x4]  = x265_pixel_satd_16x4_ ## cpu; \
    p.sa8d_inter[LUMA_12x16] = x265_pixel_satd_12x16_ ## cpu; \
    p.sa8d_inter[LUMA_8x8]   = x265_pixel_sa8d_8x8_ ## cpu; \
    p.sa8d_inter[LUMA_16x16] = x265_pixel_sa8d_16x16_ ## cpu; \
    p.sa8d_inter[LUMA_16x12] = x265_pixel_satd_16x12_ ## cpu; \
    p.sa8d_inter[LUMA_16x8]  = x265_pixel_sa8d_16x8_ ## cpu; \
    p.sa8d_inter[LUMA_8x16]  = x265_pixel_sa8d_8x16_ ## cpu; \
    p.sa8d_inter[LUMA_32x24] = x265_pixel_sa8d_32x24_ ## cpu; \
    p.sa8d_inter[LUMA_24x32] = x265_pixel_sa8d_24x32_ ## cpu; \
    p.sa8d_inter[LUMA_32x8]  = x265_pixel_sa8d_32x8_ ## cpu; \
    p.sa8d_inter[LUMA_8x32]  = x265_pixel_sa8d_8x32_ ## cpu; \
    p.sa8d_inter[LUMA_32x32] = x265_pixel_sa8d_32x32_ ## cpu; \
    p.sa8d_inter[LUMA_32x16] = x265_pixel_sa8d_32x16_ ## cpu; \
    p.sa8d_inter[LUMA_16x32] = x265_pixel_sa8d_16x32_ ## cpu; \
    p.sa8d_inter[LUMA_64x64] = x265_pixel_sa8d_64x64_ ## cpu; \
    p.sa8d_inter[LUMA_64x32] = x265_pixel_sa8d_64x32_ ## cpu; \
    p.sa8d_inter[LUMA_32x64] = x265_pixel_sa8d_32x64_ ## cpu; \
    p.sa8d_inter[LUMA_64x48] = x265_pixel_sa8d_64x48_ ## cpu; \
    p.sa8d_inter[LUMA_48x64] = x265_pixel_sa8d_48x64_ ## cpu; \
    p.sa8d_inter[LUMA_64x16] = x265_pixel_sa8d_64x16_ ## cpu; \
    p.sa8d_inter[LUMA_16x64] = x265_pixel_sa8d_16x64_ ## cpu;

#define PIXEL_AVG(cpu) \
    p.pixelavg_pp[LUMA_64x64] = x265_pixel_avg_64x64_ ## cpu; \
    p.pixelavg_pp[LUMA_64x48] = x265_pixel_avg_64x48_ ## cpu; \
    p.pixelavg_pp[LUMA_64x32] = x265_pixel_avg_64x32_ ## cpu; \
    p.pixelavg_pp[LUMA_64x16] = x265_pixel_avg_64x16_ ## cpu; \
    p.pixelavg_pp[LUMA_48x64] = x265_pixel_avg_48x64_ ## cpu; \
    p.pixelavg_pp[LUMA_32x64] = x265_pixel_avg_32x64_ ## cpu; \
    p.pixelavg_pp[LUMA_32x32] = x265_pixel_avg_32x32_ ## cpu; \
    p.pixelavg_pp[LUMA_32x24] = x265_pixel_avg_32x24_ ## cpu; \
    p.pixelavg_pp[LUMA_32x16] = x265_pixel_avg_32x16_ ## cpu; \
    p.pixelavg_pp[LUMA_32x8] = x265_pixel_avg_32x8_ ## cpu; \
    p.pixelavg_pp[LUMA_24x32] = x265_pixel_avg_24x32_ ## cpu; \
    p.pixelavg_pp[LUMA_16x64] = x265_pixel_avg_16x64_ ## cpu; \
    p.pixelavg_pp[LUMA_16x32] = x265_pixel_avg_16x32_ ## cpu; \
    p.pixelavg_pp[LUMA_16x16] = x265_pixel_avg_16x16_ ## cpu; \
    p.pixelavg_pp[LUMA_16x12]  = x265_pixel_avg_16x12_ ## cpu; \
    p.pixelavg_pp[LUMA_16x8]  = x265_pixel_avg_16x8_ ## cpu; \
    p.pixelavg_pp[LUMA_16x4]  = x265_pixel_avg_16x4_ ## cpu; \
    p.pixelavg_pp[LUMA_12x16] = x265_pixel_avg_12x16_ ## cpu; \
    p.pixelavg_pp[LUMA_8x32]  = x265_pixel_avg_8x32_ ## cpu; \
    p.pixelavg_pp[LUMA_8x16]  = x265_pixel_avg_8x16_ ## cpu; \
    p.pixelavg_pp[LUMA_8x8]   = x265_pixel_avg_8x8_ ## cpu; \
    p.pixelavg_pp[LUMA_8x4]   = x265_pixel_avg_8x4_ ## cpu;

#define PIXEL_AVG_W4(cpu) \
    p.pixelavg_pp[LUMA_4x4]  = x265_pixel_avg_4x4_ ## cpu; \
    p.pixelavg_pp[LUMA_4x8]  = x265_pixel_avg_4x8_ ## cpu; \
    p.pixelavg_pp[LUMA_4x16] = x265_pixel_avg_4x16_ ## cpu;

#define SETUP_CHROMA_FUNC_DEF_420(W, H, cpu) \
    p.chroma[X265_CSP_I420].filter_hpp[CHROMA_ ## W ## x ## H] = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].filter_hps[CHROMA_ ## W ## x ## H] = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].filter_vpp[CHROMA_ ## W ## x ## H] = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].filter_vps[CHROMA_ ## W ## x ## H] = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_FUNC_DEF_422(W, H, cpu) \
    p.chroma[X265_CSP_I422].filter_hpp[CHROMA422_ ## W ## x ## H] = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].filter_hps[CHROMA422_ ## W ## x ## H] = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].filter_vpp[CHROMA422_ ## W ## x ## H] = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].filter_vps[CHROMA422_ ## W ## x ## H] = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_FUNC_DEF_444(W, H, cpu) \
    p.chroma[X265_CSP_I444].filter_hpp[LUMA_ ## W ## x ## H] = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].filter_hps[LUMA_ ## W ## x ## H] = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].filter_vpp[LUMA_ ## W ## x ## H] = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].filter_vps[LUMA_ ## W ## x ## H] = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SP_FUNC_DEF_420(W, H, cpu) \
    p.chroma[X265_CSP_I420].filter_vsp[CHROMA_ ## W ## x ## H] = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SP_FUNC_DEF_422(W, H, cpu) \
    p.chroma[X265_CSP_I422].filter_vsp[CHROMA422_ ## W ## x ## H] = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SP_FUNC_DEF_444(W, H, cpu) \
    p.chroma[X265_CSP_I444].filter_vsp[LUMA_ ## W ## x ## H] = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SS_FUNC_DEF_420(W, H, cpu) \
    p.chroma[X265_CSP_I420].filter_vss[CHROMA_ ## W ## x ## H] = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SS_FUNC_DEF_422(W, H, cpu) \
    p.chroma[X265_CSP_I422].filter_vss[CHROMA422_ ## W ## x ## H] = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SS_FUNC_DEF_444(W, H, cpu) \
    p.chroma[X265_CSP_I444].filter_vss[LUMA_ ## W ## x ## H] = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu;

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
    p.luma_hpp[LUMA_ ## W ## x ## H] = x265_interp_8tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.luma_hps[LUMA_ ## W ## x ## H] = x265_interp_8tap_horiz_ps_ ## W ## x ## H ## cpu; \
    p.luma_vpp[LUMA_ ## W ## x ## H] = x265_interp_8tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.luma_vps[LUMA_ ## W ## x ## H] = x265_interp_8tap_vert_ps_ ## W ## x ## H ## cpu; \
    p.luma_vsp[LUMA_ ## W ## x ## H] = x265_interp_8tap_vert_sp_ ## W ## x ## H ## cpu;
#else
#define SETUP_LUMA_FUNC_DEF(W, H, cpu) \
    p.luma_hpp[LUMA_ ## W ## x ## H] = x265_interp_8tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.luma_hps[LUMA_ ## W ## x ## H] = x265_interp_8tap_horiz_ps_ ## W ## x ## H ## cpu; \
    p.luma_vpp[LUMA_ ## W ## x ## H] = x265_interp_8tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.luma_vps[LUMA_ ## W ## x ## H] = x265_interp_8tap_vert_ps_ ## W ## x ## H ## cpu;
#endif // if HIGH_BIT_DEPTH

#define SETUP_LUMA_SUB_FUNC_DEF(W, H, cpu) \
    p.luma_sub_ps[LUMA_ ## W ## x ## H] = x265_pixel_sub_ps_ ## W ## x ## H ## cpu; \
    p.luma_add_ps[LUMA_ ## W ## x ## H] = x265_pixel_add_ps_ ## W ## x ## H ## cpu;

#define SETUP_LUMA_SP_FUNC_DEF(W, H, cpu) \
    p.luma_vsp[LUMA_ ## W ## x ## H] = x265_interp_8tap_vert_sp_ ## W ## x ## H ## cpu;

#define SETUP_LUMA_SS_FUNC_DEF(W, H, cpu) \
    p.luma_vss[LUMA_ ## W ## x ## H] = x265_interp_8tap_vert_ss_ ## W ## x ## H ## cpu;

#define SETUP_LUMA_BLOCKCOPY(type, W, H, cpu) \
    p.luma_copy_ ## type[LUMA_ ## W ## x ## H] = x265_blockcopy_ ## type ## _ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_BLOCKCOPY(type, W, H, cpu) \
    p.chroma[X265_CSP_I420].copy_ ## type[CHROMA_ ## W ## x ## H] = x265_blockcopy_ ## type ## _ ## W ## x ## H ## cpu;

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

#define SETUP_CHROMA_BLOCKCOPY_422(type, W, H, cpu) \
    p.chroma[X265_CSP_I422].copy_ ## type[CHROMA422_ ## W ## x ## H] = x265_blockcopy_ ## type ## _ ## W ## x ## H ## cpu;

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

#define SETUP_CHROMA_BLOCKCOPY_SP(W, H, cpu) \
    p.chroma[X265_CSP_I420].copy_sp[CHROMA_ ## W ## x ## H] = x265_blockcopy_sp_ ## W ## x ## H ## cpu;

#define CHROMA_BLOCKCOPY_SP(cpu) \
    SETUP_CHROMA_BLOCKCOPY_SP(2,  4,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(2,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(4,  2,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(4,  4,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(4,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(4,  16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(6,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(8,  2,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(8,  4,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(8,  6,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(8,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(8,  16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(8,  32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(12, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(16, 4,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(16, 8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(16, 12, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(16, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(16, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(24, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(32, 8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(32, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(32, 24, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP(32, 32, cpu);

#define SETUP_CHROMA_BLOCKCOPY_SP_422(W, H, cpu) \
    p.chroma[X265_CSP_I422].copy_sp[CHROMA422_ ## W ## x ## H] = x265_blockcopy_sp_ ## W ## x ## H ## cpu;

#define CHROMA_BLOCKCOPY_SP_422(cpu) \
    SETUP_CHROMA_BLOCKCOPY_SP_422(2,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(2, 16,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(4,  4,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(4,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(4, 16,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(4,  32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(6, 16,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(8,  4,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(8,  8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(8, 12,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(8, 16,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(8,  32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(8,  64, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(12, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(16, 8,  cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(16, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(16, 24, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(16, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(16, 64, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(24, 64, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(32, 16, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(32, 32, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(32, 48, cpu); \
    SETUP_CHROMA_BLOCKCOPY_SP_422(32, 64, cpu);

#define SETUP_CHROMA_PIXELSUB(W, H, cpu) \
    p.chroma[X265_CSP_I420].sub_ps[CHROMA_ ## W ## x ## H] = x265_pixel_sub_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].add_ps[CHROMA_ ## W ## x ## H] = x265_pixel_add_ps_ ## W ## x ## H ## cpu;

#define CHROMA_PIXELSUB_PS(cpu) \
    SETUP_CHROMA_PIXELSUB(4,  4,  cpu); \
    SETUP_CHROMA_PIXELSUB(8,  8,  cpu); \
    SETUP_CHROMA_PIXELSUB(16, 16, cpu); \
    SETUP_CHROMA_PIXELSUB(32, 32, cpu);

#define SETUP_CHROMA_PIXELSUB_422(W, H, cpu) \
    p.chroma[X265_CSP_I422].sub_ps[CHROMA422_ ## W ## x ## H] = x265_pixel_sub_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].add_ps[CHROMA422_ ## W ## x ## H] = x265_pixel_add_ps_ ## W ## x ## H ## cpu;

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
    p.var[BLOCK_ ## W ## x ## H] = x265_pixel_var_ ## W ## x ## H ## cpu;

#define LUMA_VAR(cpu) \
    SETUP_PIXEL_VAR_DEF(8,   8, cpu); \
    SETUP_PIXEL_VAR_DEF(16, 16, cpu); \
    SETUP_PIXEL_VAR_DEF(32, 32, cpu); \
    SETUP_PIXEL_VAR_DEF(64, 64, cpu);

#define SETUP_PIXEL_SSE_SP_DEF(W, H, cpu) \
    p.sse_sp[LUMA_ ## W ## x ## H] = x265_pixel_ssd_sp_ ## W ## x ## H ## cpu;

#define LUMA_SSE_SP(cpu) \
    SETUP_PIXEL_SSE_SP_DEF(4,   4, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(8,   8, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(8,   4, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(4,   8, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(16, 16, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(16,  8, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(8,  16, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(16, 12, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(12, 16, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(16,  4, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(4,  16, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(32, 32, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(32, 16, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(16, 32, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(32, 24, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(24, 32, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(32,  8, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(8,  32, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(64, 64, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(64, 32, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(32, 64, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(64, 48, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(48, 64, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(64, 16, cpu); \
    SETUP_PIXEL_SSE_SP_DEF(16, 64, cpu);

#define SETUP_LUMA_ADDAVG_FUNC_DEF(W, H, cpu) \
    p.luma_addAvg[LUMA_ ## W ## x ## H] = x265_addAvg_ ## W ## x ## H ## cpu;

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
    p.chroma[X265_CSP_I420].addAvg[CHROMA_ ## W ## x ## H] = x265_addAvg_ ## W ## x ## H ## cpu;

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
    p.chroma[X265_CSP_I422].addAvg[CHROMA422_ ## W ## x ## H] = x265_addAvg_ ## W ## x ## H ## cpu;

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

#define SETUP_INTRA_ANG(mode, fno, cpu) \
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

#define INTRA_ANG_SSE4_HIGH(cpu) \
    SETUP_INTRA_ANG(19, 19, cpu); \
    SETUP_INTRA_ANG(20, 20, cpu); \
    SETUP_INTRA_ANG(21, 21, cpu); \
    SETUP_INTRA_ANG(22, 22, cpu); \
    SETUP_INTRA_ANG(23, 23, cpu); \
    SETUP_INTRA_ANG(24, 24, cpu); \
    SETUP_INTRA_ANG(25, 25, cpu); \
    SETUP_INTRA_ANG(26, 26, cpu); \
    SETUP_INTRA_ANG(27, 27, cpu); \
    SETUP_INTRA_ANG(28, 28, cpu); \
    SETUP_INTRA_ANG(29, 29, cpu); \
    SETUP_INTRA_ANG(30, 30, cpu); \
    SETUP_INTRA_ANG(31, 31, cpu); \
    SETUP_INTRA_ANG(32, 32, cpu); \
    SETUP_INTRA_ANG(33, 33, cpu); \
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
    p.chroma[X265_CSP_I420].filter_vss[CHROMA_ ## W ## x ## H] = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].filter_vpp[CHROMA_ ## W ## x ## H] = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].filter_vps[CHROMA_ ## W ## x ## H] = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].filter_vsp[CHROMA_ ## W ## x ## H] = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

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
    p.chroma[X265_CSP_I422].filter_vss[CHROMA422_ ## W ## x ## H] = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].filter_vpp[CHROMA422_ ## W ## x ## H] = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].filter_vps[CHROMA422_ ## W ## x ## H] = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].filter_vsp[CHROMA422_ ## W ## x ## H] = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

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
    p.chroma[X265_CSP_I444].filter_vss[LUMA_ ## W ## x ## H] = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].filter_vpp[LUMA_ ## W ## x ## H] = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].filter_vps[LUMA_ ## W ## x ## H] = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].filter_vsp[LUMA_ ## W ## x ## H] = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

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
    p.chroma[X265_CSP_I420].filter_hpp[CHROMA_ ## W ## x ## H] = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].filter_hps[CHROMA_ ## W ## x ## H] = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu;

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
    p.chroma[X265_CSP_I422].filter_hpp[CHROMA422_ ## W ## x ## H] = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I422].filter_hps[CHROMA422_ ## W ## x ## H] = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu;

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
    p.chroma[X265_CSP_I444].filter_hpp[LUMA_ ## W ## x ## H] = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I444].filter_hps[LUMA_ ## W ## x ## H] = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu;

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

#if HIGH_BIT_DEPTH
/* Very similar to CRef in intrapred.cpp, except it uses optimized primitives */
template<int log2Size>
void intra_allangs(pixel *dest, pixel *above0, pixel *left0, pixel *above1, pixel *left1, int bLuma)
{
    const int size = 1 << log2Size;
    const int sizeIdx = log2Size - 2;
    ALIGN_VAR_32(pixel, buffer[32 * 32]);

    for (int mode = 2; mode <= 34; mode++)
    {
        pixel *left  = (g_intraFilterFlags[mode] & size ? left1  : left0);
        pixel *above = (g_intraFilterFlags[mode] & size ? above1 : above0);
        pixel *out = dest + ((mode - 2) << (log2Size * 2));

        if (mode < 18)
        {
            primitives.intra_pred[mode][sizeIdx](buffer, size, left, above, mode, bLuma);
            primitives.transpose[sizeIdx](out, buffer, size);
        }
        else
            primitives.intra_pred[mode][sizeIdx](out, size, left, above, mode, bLuma);
    }
}
#endif

void Setup_Assembly_Primitives(EncoderPrimitives &p, int cpuMask)
{
#if HIGH_BIT_DEPTH
    if (cpuMask & X265_CPU_SSE2)
    {
        INIT8(sad, _mmx2);
        INIT2(sad, _sse2);
        SAD(sse2);

        INIT6(satd, _sse2);
        HEVC_SATD(sse2);
        p.satd[LUMA_4x4] = x265_pixel_satd_4x4_mmx2;

        p.sa8d_inter[LUMA_4x4]  = x265_pixel_satd_4x4_mmx2;
        SA8D_INTER_FROM_BLOCK(sse2);
        p.sa8d_inter[LUMA_8x8] = x265_pixel_sa8d_8x8_sse2;
        p.sa8d_inter[LUMA_16x16] = x265_pixel_sa8d_16x16_sse2;

        p.sse_ss[LUMA_4x4] = x265_pixel_ssd_ss_4x4_mmx2;
        p.sse_ss[LUMA_4x8] = x265_pixel_ssd_ss_4x8_mmx2;
        p.sse_ss[LUMA_4x16] = x265_pixel_ssd_ss_4x16_mmx2;
        p.sse_ss[LUMA_8x4] = x265_pixel_ssd_ss_8x4_sse2;
        p.sse_ss[LUMA_8x8] = x265_pixel_ssd_ss_8x8_sse2;
        p.sse_ss[LUMA_8x16] = x265_pixel_ssd_ss_8x16_sse2;
        p.sse_ss[LUMA_8x32] = x265_pixel_ssd_ss_8x32_sse2;
        p.sse_ss[LUMA_12x16] = x265_pixel_ssd_ss_12x16_sse2;
        p.sse_ss[LUMA_16x4] = x265_pixel_ssd_ss_16x4_sse2;
        p.sse_ss[LUMA_16x8] = x265_pixel_ssd_ss_16x8_sse2;
        p.sse_ss[LUMA_16x12] = x265_pixel_ssd_ss_16x12_sse2;
        p.sse_ss[LUMA_16x16] = x265_pixel_ssd_ss_16x16_sse2;
        p.sse_ss[LUMA_16x32] = x265_pixel_ssd_ss_16x32_sse2;
        p.sse_ss[LUMA_16x64] = x265_pixel_ssd_ss_16x64_sse2;
        p.sse_ss[LUMA_24x32] = x265_pixel_ssd_ss_24x32_sse2;
        p.sse_ss[LUMA_32x8] = x265_pixel_ssd_ss_32x8_sse2;
        p.sse_ss[LUMA_32x16] = x265_pixel_ssd_ss_32x16_sse2;
        p.sse_ss[LUMA_32x24] = x265_pixel_ssd_ss_32x24_sse2;
        p.sse_ss[LUMA_32x32] = x265_pixel_ssd_ss_32x32_sse2;
        p.sse_ss[LUMA_32x64] = x265_pixel_ssd_ss_32x64_sse2;
        p.sse_ss[LUMA_48x64] = x265_pixel_ssd_ss_48x64_sse2;
        p.sse_ss[LUMA_64x16] = x265_pixel_ssd_ss_64x16_sse2;
        p.sse_ss[LUMA_64x32] = x265_pixel_ssd_ss_64x32_sse2;
        p.sse_ss[LUMA_64x48] = x265_pixel_ssd_ss_64x48_sse2;
        p.sse_ss[LUMA_64x64] = x265_pixel_ssd_ss_64x64_sse2;

        p.transpose[BLOCK_4x4] = x265_transpose4_sse2;
        p.transpose[BLOCK_8x8] = x265_transpose8_sse2;
        p.transpose[BLOCK_16x16] = x265_transpose16_sse2;
        p.transpose[BLOCK_32x32] = x265_transpose32_sse2;
        p.transpose[BLOCK_64x64] = x265_transpose64_sse2;

        p.ssim_4x4x2_core = x265_pixel_ssim_4x4x2_core_sse2;
        p.ssim_end_4 = x265_pixel_ssim_end4_sse2;
        PIXEL_AVG(sse2);
        PIXEL_AVG_W4(mmx2);
        LUMA_VAR(_sse2);

        SAD_X3(sse2);
        p.sad_x3[LUMA_4x4] = x265_pixel_sad_x3_4x4_mmx2;
        p.sad_x3[LUMA_4x8] = x265_pixel_sad_x3_4x8_mmx2;
        p.sad_x3[LUMA_4x16] = x265_pixel_sad_x3_4x16_mmx2;
        p.sad_x3[LUMA_8x4] = x265_pixel_sad_x3_8x4_sse2;
        p.sad_x3[LUMA_8x8] = x265_pixel_sad_x3_8x8_sse2;
        p.sad_x3[LUMA_8x16] = x265_pixel_sad_x3_8x16_sse2;
        p.sad_x3[LUMA_8x32] = x265_pixel_sad_x3_8x32_sse2;
        p.sad_x3[LUMA_16x4] = x265_pixel_sad_x3_16x4_sse2;
        p.sad_x3[LUMA_12x16] = x265_pixel_sad_x3_12x16_mmx2;

        SAD_X4(sse2);
        p.sad_x4[LUMA_4x4] = x265_pixel_sad_x4_4x4_mmx2;
        p.sad_x4[LUMA_4x8] = x265_pixel_sad_x4_4x8_mmx2;
        p.sad_x4[LUMA_4x16] = x265_pixel_sad_x4_4x16_mmx2;
        p.sad_x4[LUMA_8x4] = x265_pixel_sad_x4_8x4_sse2;
        p.sad_x4[LUMA_8x8] = x265_pixel_sad_x4_8x8_sse2;
        p.sad_x4[LUMA_8x16] = x265_pixel_sad_x4_8x16_sse2;
        p.sad_x4[LUMA_8x32] = x265_pixel_sad_x4_8x32_sse2;
        p.sad_x4[LUMA_16x4] = x265_pixel_sad_x4_16x4_sse2;
        p.sad_x4[LUMA_12x16] = x265_pixel_sad_x4_12x16_mmx2;

        p.cpy2Dto1D_shl[BLOCK_4x4] = x265_cpy2Dto1D_shl_4_sse2;
        p.cpy2Dto1D_shl[BLOCK_8x8] = x265_cpy2Dto1D_shl_8_sse2;
        p.cpy2Dto1D_shl[BLOCK_16x16] = x265_cpy2Dto1D_shl_16_sse2;
        p.cpy2Dto1D_shl[BLOCK_32x32] = x265_cpy2Dto1D_shl_32_sse2;
        p.cpy2Dto1D_shr[BLOCK_4x4] = x265_cpy2Dto1D_shr_4_sse2;
        p.cpy2Dto1D_shr[BLOCK_8x8] = x265_cpy2Dto1D_shr_8_sse2;
        p.cpy2Dto1D_shr[BLOCK_16x16] = x265_cpy2Dto1D_shr_16_sse2;
        p.cpy2Dto1D_shr[BLOCK_32x32] = x265_cpy2Dto1D_shr_32_sse2;
        p.cpy1Dto2D_shl[BLOCK_4x4] = x265_cpy1Dto2D_shl_4_sse2;
        p.cpy1Dto2D_shl[BLOCK_8x8] = x265_cpy1Dto2D_shl_8_sse2;
        p.cpy1Dto2D_shl[BLOCK_16x16] = x265_cpy1Dto2D_shl_16_sse2;
        p.cpy1Dto2D_shl[BLOCK_32x32] = x265_cpy1Dto2D_shl_32_sse2;
        p.cpy1Dto2D_shr[BLOCK_4x4] = x265_cpy1Dto2D_shr_4_sse2;
        p.cpy1Dto2D_shr[BLOCK_8x8] = x265_cpy1Dto2D_shr_8_sse2;
        p.cpy1Dto2D_shr[BLOCK_16x16] = x265_cpy1Dto2D_shr_16_sse2;
        p.cpy1Dto2D_shr[BLOCK_32x32] = x265_cpy1Dto2D_shr_32_sse2;

        CHROMA_PIXELSUB_PS(_sse2);
        CHROMA_PIXELSUB_PS_422(_sse2);
        LUMA_PIXELSUB(_sse2);

        CHROMA_BLOCKCOPY(ss, _sse2);
        CHROMA_BLOCKCOPY_422(ss, _sse2);
        LUMA_BLOCKCOPY(ss, _sse2);

        CHROMA_VERT_FILTERS(_sse2);
        CHROMA_VERT_FILTERS_422(_sse2);
        CHROMA_VERT_FILTERS_444(_sse2);
        p.luma_p2s = x265_luma_p2s_sse2;
        p.chroma_p2s[X265_CSP_I420] = x265_chroma_p2s_sse2;
        p.chroma_p2s[X265_CSP_I422] = x265_chroma_p2s_sse2;
        p.chroma_p2s[X265_CSP_I444] = x265_luma_p2s_sse2; // for i444 , chroma_p2s can be replaced by luma_p2s

        p.blockfill_s[BLOCK_4x4] = x265_blockfill_s_4x4_sse2;
        p.blockfill_s[BLOCK_8x8] = x265_blockfill_s_8x8_sse2;
        p.blockfill_s[BLOCK_16x16] = x265_blockfill_s_16x16_sse2;
        p.blockfill_s[BLOCK_32x32] = x265_blockfill_s_32x32_sse2;

        // TODO: overflow on 12-bits mode!
        p.ssd_s[BLOCK_4x4] = x265_pixel_ssd_s_4_sse2;
        p.ssd_s[BLOCK_8x8] = x265_pixel_ssd_s_8_sse2;
        p.ssd_s[BLOCK_16x16] = x265_pixel_ssd_s_16_sse2;
        p.ssd_s[BLOCK_32x32] = x265_pixel_ssd_s_32_sse2;

        p.calcresidual[BLOCK_4x4] = x265_getResidual4_sse2;
        p.calcresidual[BLOCK_8x8] = x265_getResidual8_sse2;
        p.calcresidual[BLOCK_16x16] = x265_getResidual16_sse2;
        p.calcresidual[BLOCK_32x32] = x265_getResidual32_sse2;

        p.dct[DCT_4x4] = x265_dct4_sse2;
        p.idct[IDCT_4x4] = x265_idct4_sse2;
        p.idct[IDST_4x4] = x265_idst4_sse2;

        LUMA_SS_FILTERS(_sse2);
    }
    if (cpuMask & X265_CPU_SSSE3)
    {
        p.scale1D_128to64 = x265_scale1D_128to64_ssse3;
        p.scale2D_64to32 = x265_scale2D_64to32_ssse3;

        INTRA_ANG_SSSE3(ssse3);

        p.dct[DST_4x4] = x265_dst4_ssse3;
        p.idct[IDCT_8x8] = x265_idct8_ssse3;
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

        p.dct[DCT_8x8] = x265_dct8_sse4;
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
        p.dct[DCT_4x4] = x265_dct4_avx2;
        p.quant = x265_quant_avx2;
        p.nquant = x265_nquant_avx2;
        p.dequant_normal = x265_dequant_normal_avx2;
        p.scale1D_128to64 = x265_scale1D_128to64_avx2;
        p.cpy1Dto2D_shl[BLOCK_4x4] = x265_cpy1Dto2D_shl_4_avx2;
        p.cpy1Dto2D_shl[BLOCK_8x8] = x265_cpy1Dto2D_shl_8_avx2;
        p.cpy1Dto2D_shl[BLOCK_16x16] = x265_cpy1Dto2D_shl_16_avx2;
        p.cpy1Dto2D_shl[BLOCK_32x32] = x265_cpy1Dto2D_shl_32_avx2;
        p.cpy1Dto2D_shr[BLOCK_4x4] = x265_cpy1Dto2D_shr_4_avx2;
        p.cpy1Dto2D_shr[BLOCK_8x8] = x265_cpy1Dto2D_shr_8_avx2;
        p.cpy1Dto2D_shr[BLOCK_16x16] = x265_cpy1Dto2D_shr_16_avx2;
        p.cpy1Dto2D_shr[BLOCK_32x32] = x265_cpy1Dto2D_shr_32_avx2;
#if X86_64
        p.dct[DCT_8x8] = x265_dct8_avx2;
        p.dct[DCT_16x16] = x265_dct16_avx2;
        p.dct[DCT_32x32] = x265_dct32_avx2;
        p.idct[IDCT_4x4] = x265_idct4_avx2;
        p.idct[IDCT_8x8] = x265_idct8_avx2;
        p.idct[IDCT_16x16] = x265_idct16_avx2;
        p.idct[IDCT_32x32] = x265_idct32_avx2;
        p.transpose[BLOCK_8x8] = x265_transpose8_avx2;
        p.transpose[BLOCK_16x16] = x265_transpose16_avx2;
        p.transpose[BLOCK_32x32] = x265_transpose32_avx2;
        p.transpose[BLOCK_64x64] = x265_transpose64_avx2;
#endif
    }
    /* at HIGH_BIT_DEPTH, pixel == short so we can reuse a number of primitives */
    for (int i = 0; i < NUM_LUMA_PARTITIONS; i++)
    {
        p.sse_pp[i] = (pixelcmp_t)p.sse_ss[i];
        p.sse_sp[i] = (pixelcmp_sp_t)p.sse_ss[i];
    }

    for (int i = 0; i < NUM_LUMA_PARTITIONS; i++)
    {
        p.luma_copy_ps[i] = (copy_ps_t)p.luma_copy_ss[i];
        p.luma_copy_sp[i] = (copy_sp_t)p.luma_copy_ss[i];
        p.luma_copy_pp[i] = (copy_pp_t)p.luma_copy_ss[i];
    }

    for (int i = 0; i < NUM_CHROMA_PARTITIONS; i++)
    {
        p.chroma[X265_CSP_I420].copy_ps[i] = (copy_ps_t)p.chroma[X265_CSP_I420].copy_ss[i];
        p.chroma[X265_CSP_I420].copy_sp[i] = (copy_sp_t)p.chroma[X265_CSP_I420].copy_ss[i];
        p.chroma[X265_CSP_I420].copy_pp[i] = (copy_pp_t)p.chroma[X265_CSP_I420].copy_ss[i];
    }

    for (int i = 0; i < NUM_CHROMA_PARTITIONS; i++)
    {
        p.chroma[X265_CSP_I422].copy_ps[i] = (copy_ps_t)p.chroma[X265_CSP_I422].copy_ss[i];
        p.chroma[X265_CSP_I422].copy_sp[i] = (copy_sp_t)p.chroma[X265_CSP_I422].copy_ss[i];
        p.chroma[X265_CSP_I422].copy_pp[i] = (copy_pp_t)p.chroma[X265_CSP_I422].copy_ss[i];
    }

    if (p.intra_pred[0][0] && p.transpose[0])
    {
        p.intra_pred_allangs[BLOCK_4x4] = intra_allangs<2>;
        p.intra_pred_allangs[BLOCK_8x8] = intra_allangs<3>;
        p.intra_pred_allangs[BLOCK_16x16] = intra_allangs<4>;
        p.intra_pred_allangs[BLOCK_32x32] = intra_allangs<5>;
    }

#else // if HIGH_BIT_DEPTH
    if (cpuMask & X265_CPU_SSE2)
    {
        INIT8_NAME(sse_pp, ssd, _mmx);
        INIT8(sad, _mmx2);
        INIT8(sad_x3, _mmx2);
        INIT8(sad_x4, _mmx2);
        p.satd[LUMA_4x4] = x265_pixel_satd_4x4_mmx2;
        p.sa8d_inter[LUMA_4x4]  = x265_pixel_satd_4x4_mmx2;
        p.frameInitLowres = x265_frame_init_lowres_core_mmx2;

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

        CHROMA_BLOCKCOPY(ss, _sse2);
        CHROMA_BLOCKCOPY(pp, _sse2);
        CHROMA_BLOCKCOPY_422(ss, _sse2);
        CHROMA_BLOCKCOPY_422(pp, _sse2);
        LUMA_BLOCKCOPY(ss, _sse2);
        LUMA_BLOCKCOPY(pp, _sse2);
        LUMA_BLOCKCOPY(sp, _sse2);
        CHROMA_BLOCKCOPY_SP(_sse2);
        CHROMA_BLOCKCOPY_SP_422(_sse2);

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

        p.blockfill_s[BLOCK_4x4] = x265_blockfill_s_4x4_sse2;
        p.blockfill_s[BLOCK_8x8] = x265_blockfill_s_8x8_sse2;
        p.blockfill_s[BLOCK_16x16] = x265_blockfill_s_16x16_sse2;
        p.blockfill_s[BLOCK_32x32] = x265_blockfill_s_32x32_sse2;

        p.ssd_s[BLOCK_4x4] = x265_pixel_ssd_s_4_sse2;
        p.ssd_s[BLOCK_8x8] = x265_pixel_ssd_s_8_sse2;
        p.ssd_s[BLOCK_16x16] = x265_pixel_ssd_s_16_sse2;
        p.ssd_s[BLOCK_32x32] = x265_pixel_ssd_s_32_sse2;

        p.frameInitLowres = x265_frame_init_lowres_core_sse2;
        SA8D_INTER_FROM_BLOCK(sse2);

        p.cpy2Dto1D_shl[BLOCK_4x4] = x265_cpy2Dto1D_shl_4_sse2;
        p.cpy2Dto1D_shl[BLOCK_8x8] = x265_cpy2Dto1D_shl_8_sse2;
        p.cpy2Dto1D_shl[BLOCK_16x16] = x265_cpy2Dto1D_shl_16_sse2;
        p.cpy2Dto1D_shl[BLOCK_32x32] = x265_cpy2Dto1D_shl_32_sse2;
        p.cpy2Dto1D_shr[BLOCK_4x4] = x265_cpy2Dto1D_shr_4_sse2;
        p.cpy2Dto1D_shr[BLOCK_8x8] = x265_cpy2Dto1D_shr_8_sse2;
        p.cpy2Dto1D_shr[BLOCK_16x16] = x265_cpy2Dto1D_shr_16_sse2;
        p.cpy2Dto1D_shr[BLOCK_32x32] = x265_cpy2Dto1D_shr_32_sse2;
        p.cpy1Dto2D_shl[BLOCK_4x4] = x265_cpy1Dto2D_shl_4_sse2;
        p.cpy1Dto2D_shl[BLOCK_8x8] = x265_cpy1Dto2D_shl_8_sse2;
        p.cpy1Dto2D_shl[BLOCK_16x16] = x265_cpy1Dto2D_shl_16_sse2;
        p.cpy1Dto2D_shl[BLOCK_32x32] = x265_cpy1Dto2D_shl_32_sse2;
        p.cpy1Dto2D_shr[BLOCK_4x4] = x265_cpy1Dto2D_shr_4_sse2;
        p.cpy1Dto2D_shr[BLOCK_8x8] = x265_cpy1Dto2D_shr_8_sse2;
        p.cpy1Dto2D_shr[BLOCK_16x16] = x265_cpy1Dto2D_shr_16_sse2;
        p.cpy1Dto2D_shr[BLOCK_32x32] = x265_cpy1Dto2D_shr_32_sse2;

        p.calcresidual[BLOCK_4x4] = x265_getResidual4_sse2;
        p.calcresidual[BLOCK_8x8] = x265_getResidual8_sse2;
        p.transpose[BLOCK_4x4] = x265_transpose4_sse2;
        p.transpose[BLOCK_8x8] = x265_transpose8_sse2;
        p.transpose[BLOCK_16x16] = x265_transpose16_sse2;
        p.transpose[BLOCK_32x32] = x265_transpose32_sse2;
        p.transpose[BLOCK_64x64] = x265_transpose64_sse2;
        p.ssim_4x4x2_core = x265_pixel_ssim_4x4x2_core_sse2;
        p.ssim_end_4 = x265_pixel_ssim_end4_sse2;

        p.dct[DCT_4x4] = x265_dct4_sse2;
        p.idct[IDCT_4x4] = x265_idct4_sse2;
        p.idct[IDST_4x4] = x265_idst4_sse2;

        p.planecopy_sp = x265_downShift_16_sse2;
    }
    if (cpuMask & X265_CPU_SSSE3)
    {
        p.frameInitLowres = x265_frame_init_lowres_core_ssse3;
        SA8D_INTER_FROM_BLOCK(ssse3);
        p.sse_pp[LUMA_4x4] = x265_pixel_ssd_4x4_ssse3;
        ASSGN_SSE(ssse3);
        PIXEL_AVG(ssse3);
        PIXEL_AVG_W4(ssse3);

        INTRA_ANG_SSSE3(ssse3);

        p.scale1D_128to64 = x265_scale1D_128to64_ssse3;
        p.scale2D_64to32 = x265_scale2D_64to32_ssse3;
        SAD_X3(ssse3);
        SAD_X4(ssse3);
        p.sad_x4[LUMA_8x4] = x265_pixel_sad_x4_8x4_ssse3;
        p.sad_x4[LUMA_8x8] = x265_pixel_sad_x4_8x8_ssse3;
        p.sad_x3[LUMA_8x16] = x265_pixel_sad_x3_8x16_ssse3;
        p.sad_x4[LUMA_8x16] = x265_pixel_sad_x4_8x16_ssse3;
        p.sad_x3[LUMA_8x32]  = x265_pixel_sad_x3_8x32_ssse3;
        p.sad_x4[LUMA_8x32]  = x265_pixel_sad_x4_8x32_ssse3;

        p.sad_x3[LUMA_12x16] = x265_pixel_sad_x3_12x16_ssse3;
        p.sad_x4[LUMA_12x16] = x265_pixel_sad_x4_12x16_ssse3;

        p.luma_hvpp[LUMA_8x8] = x265_interp_8tap_hv_pp_8x8_ssse3;
        p.luma_p2s = x265_luma_p2s_ssse3;
        p.chroma[X265_CSP_I420].p2s = x265_chroma_p2s_ssse3;
        p.chroma[X265_CSP_I422].p2s = x265_chroma_p2s_ssse3;
        p.chroma[X265_CSP_I444].p2s = x265_luma_p2s_ssse3; // for i444, chroma_p2s can use luma_p2s

        p.dct[DST_4x4] = x265_dst4_ssse3;
        p.idct[IDCT_8x8] = x265_idct8_ssse3;
        p.count_nonzero = x265_count_nonzero_ssse3;
    }
    if (cpuMask & X265_CPU_SSE4)
    {
        p.saoCuOrgE0 = x265_saoCuOrgE0_sse4;

        LUMA_ADDAVG(_sse4);
        CHROMA_ADDAVG(_sse4);
        CHROMA_ADDAVG_422(_sse4);

        // TODO: check POPCNT flag!
        p.copy_cnt[BLOCK_4x4] = x265_copy_cnt_4_sse4;
        p.copy_cnt[BLOCK_8x8] = x265_copy_cnt_8_sse4;
        p.copy_cnt[BLOCK_16x16] = x265_copy_cnt_16_sse4;
        p.copy_cnt[BLOCK_32x32] = x265_copy_cnt_32_sse4;

        HEVC_SATD(sse4);
        SA8D_INTER_FROM_BLOCK(sse4);

        p.sse_pp[LUMA_12x16] = x265_pixel_ssd_12x16_sse4;
        p.sse_pp[LUMA_24x32] = x265_pixel_ssd_24x32_sse4;
        p.sse_pp[LUMA_48x64] = x265_pixel_ssd_48x64_sse4;
        p.sse_pp[LUMA_64x16] = x265_pixel_ssd_64x16_sse4;
        p.sse_pp[LUMA_64x32] = x265_pixel_ssd_64x32_sse4;
        p.sse_pp[LUMA_64x48] = x265_pixel_ssd_64x48_sse4;
        p.sse_pp[LUMA_64x64] = x265_pixel_ssd_64x64_sse4;

        LUMA_SSE_SP(_sse4);

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

        p.chroma[X265_CSP_I420].copy_sp[CHROMA_2x4] = x265_blockcopy_sp_2x4_sse4;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_2x8] = x265_blockcopy_sp_2x8_sse4;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_6x8] = x265_blockcopy_sp_6x8_sse4;
        CHROMA_BLOCKCOPY(ps, _sse4);
        CHROMA_BLOCKCOPY_422(ps, _sse4);
        LUMA_BLOCKCOPY(ps, _sse4);

        p.calcresidual[BLOCK_16x16] = x265_getResidual16_sse4;
        p.calcresidual[BLOCK_32x32] = x265_getResidual32_sse4;
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

        p.dct[DCT_8x8] = x265_dct8_sse4;
//        p.denoiseDct = x265_denoise_dct_sse4;
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
        p.sad_x3[LUMA_12x16] = x265_pixel_sad_x3_12x16_avx;
        p.sad_x4[LUMA_12x16] = x265_pixel_sad_x4_12x16_avx;
        p.sad_x3[LUMA_16x4]  = x265_pixel_sad_x3_16x4_avx;
        p.sad_x4[LUMA_16x4]  = x265_pixel_sad_x4_16x4_avx;

        p.ssim_4x4x2_core = x265_pixel_ssim_4x4x2_core_avx;
        p.ssim_end_4 = x265_pixel_ssim_end4_avx;
        p.luma_copy_ss[LUMA_64x16] = x265_blockcopy_ss_64x16_avx;
        p.luma_copy_ss[LUMA_64x32] = x265_blockcopy_ss_64x32_avx;
        p.luma_copy_ss[LUMA_64x48] = x265_blockcopy_ss_64x48_avx;
        p.luma_copy_ss[LUMA_64x64] = x265_blockcopy_ss_64x64_avx;

        p.chroma[X265_CSP_I420].copy_pp[CHROMA_32x8] = x265_blockcopy_pp_32x8_avx;
        p.luma_copy_pp[LUMA_32x8] = x265_blockcopy_pp_32x8_avx;

        p.chroma[X265_CSP_I420].copy_pp[CHROMA_32x16] = x265_blockcopy_pp_32x16_avx;
        p.chroma[X265_CSP_I422].copy_pp[CHROMA422_32x16] = x265_blockcopy_pp_32x16_avx;
        p.luma_copy_pp[LUMA_32x16] = x265_blockcopy_pp_32x16_avx;

        p.chroma[X265_CSP_I420].copy_pp[CHROMA_32x24] = x265_blockcopy_pp_32x24_avx;
        p.luma_copy_pp[LUMA_32x24] = x265_blockcopy_pp_32x24_avx;

        p.chroma[X265_CSP_I420].copy_pp[CHROMA_32x32] = x265_blockcopy_pp_32x32_avx;
        p.chroma[X265_CSP_I422].copy_pp[CHROMA422_32x32] = x265_blockcopy_pp_32x32_avx;
        p.luma_copy_pp[LUMA_32x32]  = x265_blockcopy_pp_32x32_avx;

        p.chroma[X265_CSP_I422].copy_pp[CHROMA422_32x48] = x265_blockcopy_pp_32x48_avx;

        p.chroma[X265_CSP_I422].copy_pp[CHROMA422_32x64] = x265_blockcopy_pp_32x64_avx;
        p.luma_copy_pp[LUMA_32x64]  = x265_blockcopy_pp_32x64_avx;
    }
    if (cpuMask & X265_CPU_XOP)
    {
        p.frameInitLowres = x265_frame_init_lowres_core_xop;
        SA8D_INTER_FROM_BLOCK(xop);
        INIT7(satd, _xop);
        INIT5_NAME(sse_pp, ssd, _xop);
        HEVC_SATD(xop);
    }
    if (cpuMask & X265_CPU_AVX2)
    {
        INIT2(sad_x4, _avx2);
        INIT4(satd, _avx2);
        INIT2_NAME(sse_pp, ssd, _avx2);
        p.sad_x4[LUMA_16x12] = x265_pixel_sad_x4_16x12_avx2;
        p.sad_x4[LUMA_16x32] = x265_pixel_sad_x4_16x32_avx2;
        p.ssd_s[BLOCK_32x32] = x265_pixel_ssd_s_32_avx2;

        /* Need to update assembly code as per changed interface of the copy_cnt primitive, once
         * code is updated, avx2 version will be enabled */

        p.copy_cnt[BLOCK_8x8] = x265_copy_cnt_8_avx2;
        p.copy_cnt[BLOCK_16x16] = x265_copy_cnt_16_avx2;
        p.copy_cnt[BLOCK_32x32] = x265_copy_cnt_32_avx2;

        p.blockfill_s[BLOCK_16x16] = x265_blockfill_s_16x16_avx2;
        p.blockfill_s[BLOCK_32x32] = x265_blockfill_s_32x32_avx2;

        p.cpy1Dto2D_shl[BLOCK_4x4] = x265_cpy1Dto2D_shl_4_avx2;
        p.cpy1Dto2D_shl[BLOCK_8x8] = x265_cpy1Dto2D_shl_8_avx2;
        p.cpy1Dto2D_shl[BLOCK_16x16] = x265_cpy1Dto2D_shl_16_avx2;
        p.cpy1Dto2D_shl[BLOCK_32x32] = x265_cpy1Dto2D_shl_32_avx2;
        p.cpy1Dto2D_shr[BLOCK_4x4] = x265_cpy1Dto2D_shr_4_avx2;
        p.cpy1Dto2D_shr[BLOCK_8x8] = x265_cpy1Dto2D_shr_8_avx2;
        p.cpy1Dto2D_shr[BLOCK_16x16] = x265_cpy1Dto2D_shr_16_avx2;
        p.cpy1Dto2D_shr[BLOCK_32x32] = x265_cpy1Dto2D_shr_32_avx2;

//        p.denoiseDct = x265_denoise_dct_avx2;
        p.dct[DCT_4x4] = x265_dct4_avx2;
        p.quant = x265_quant_avx2;
        p.nquant = x265_nquant_avx2;
        p.dequant_normal = x265_dequant_normal_avx2;

        p.chroma[X265_CSP_I420].copy_ss[CHROMA_16x4] = x265_blockcopy_ss_16x4_avx;
        p.chroma[X265_CSP_I420].copy_ss[CHROMA_16x12] = x265_blockcopy_ss_16x12_avx;
        p.chroma[X265_CSP_I420].copy_ss[CHROMA_16x8] = x265_blockcopy_ss_16x8_avx;
        p.chroma[X265_CSP_I420].copy_ss[CHROMA_16x16] = x265_blockcopy_ss_16x16_avx;
        p.chroma[X265_CSP_I420].copy_ss[CHROMA_16x32] = x265_blockcopy_ss_16x32_avx;
        p.chroma[X265_CSP_I422].copy_ss[CHROMA422_16x8] = x265_blockcopy_ss_16x8_avx;
        p.chroma[X265_CSP_I422].copy_ss[CHROMA422_16x16] = x265_blockcopy_ss_16x16_avx;
        p.chroma[X265_CSP_I422].copy_ss[CHROMA422_16x24] = x265_blockcopy_ss_16x24_avx;
        p.chroma[X265_CSP_I422].copy_ss[CHROMA422_16x32] = x265_blockcopy_ss_16x32_avx;
        p.chroma[X265_CSP_I422].copy_ss[CHROMA422_16x64] = x265_blockcopy_ss_16x64_avx;
        p.scale1D_128to64 = x265_scale1D_128to64_avx2;

        p.weight_pp = x265_weight_pp_avx2;

#if X86_64

        p.dct[DCT_8x8] = x265_dct8_avx2;
        p.dct[DCT_16x16] = x265_dct16_avx2;
        p.dct[DCT_32x32] = x265_dct32_avx2;
        p.idct[IDCT_4x4] = x265_idct4_avx2;
        p.idct[IDCT_8x8] = x265_idct8_avx2;
        p.idct[IDCT_16x16] = x265_idct16_avx2;
        p.idct[IDCT_32x32] = x265_idct32_avx2;

        p.transpose[BLOCK_8x8] = x265_transpose8_avx2;
        p.transpose[BLOCK_16x16] = x265_transpose16_avx2;
        p.transpose[BLOCK_32x32] = x265_transpose32_avx2;
        p.transpose[BLOCK_64x64] = x265_transpose64_avx2;

        p.luma_vpp[LUMA_12x16] = x265_interp_8tap_vert_pp_12x16_avx2;

        p.luma_vpp[LUMA_16x4] = x265_interp_8tap_vert_pp_16x4_avx2;
        p.luma_vpp[LUMA_16x8] = x265_interp_8tap_vert_pp_16x8_avx2;
        p.luma_vpp[LUMA_16x12] = x265_interp_8tap_vert_pp_16x12_avx2;
        p.luma_vpp[LUMA_16x16] = x265_interp_8tap_vert_pp_16x16_avx2;
        p.luma_vpp[LUMA_16x32] = x265_interp_8tap_vert_pp_16x32_avx2;
        p.luma_vpp[LUMA_16x64] = x265_interp_8tap_vert_pp_16x64_avx2;

        p.luma_vpp[LUMA_24x32] = x265_interp_8tap_vert_pp_24x32_avx2;

        p.luma_vpp[LUMA_32x8] = x265_interp_8tap_vert_pp_32x8_avx2;
        p.luma_vpp[LUMA_32x16] = x265_interp_8tap_vert_pp_32x16_avx2;
        p.luma_vpp[LUMA_32x24] = x265_interp_8tap_vert_pp_32x24_avx2;
        p.luma_vpp[LUMA_32x32] = x265_interp_8tap_vert_pp_32x32_avx2;
        p.luma_vpp[LUMA_32x64] = x265_interp_8tap_vert_pp_32x64_avx2;

        p.luma_vpp[LUMA_48x64] = x265_interp_8tap_vert_pp_48x64_avx2;

        p.luma_vpp[LUMA_64x16] = x265_interp_8tap_vert_pp_64x16_avx2;
        p.luma_vpp[LUMA_64x32] = x265_interp_8tap_vert_pp_64x32_avx2;
        p.luma_vpp[LUMA_64x48] = x265_interp_8tap_vert_pp_64x48_avx2;
        p.luma_vpp[LUMA_64x64] = x265_interp_8tap_vert_pp_64x64_avx2;
#endif
        p.luma_hpp[LUMA_4x4] = x265_interp_8tap_horiz_pp_4x4_avx2;

        p.luma_hpp[LUMA_8x4] = x265_interp_8tap_horiz_pp_8x4_avx2;
        p.luma_hpp[LUMA_8x8] = x265_interp_8tap_horiz_pp_8x8_avx2;
        p.luma_hpp[LUMA_8x16] = x265_interp_8tap_horiz_pp_8x16_avx2;
        p.luma_hpp[LUMA_8x32] = x265_interp_8tap_horiz_pp_8x32_avx2;

        p.luma_hpp[LUMA_16x4] = x265_interp_8tap_horiz_pp_16x4_avx2;
        p.luma_hpp[LUMA_16x8] = x265_interp_8tap_horiz_pp_16x8_avx2;
        p.luma_hpp[LUMA_16x12] = x265_interp_8tap_horiz_pp_16x12_avx2;
        p.luma_hpp[LUMA_16x16] = x265_interp_8tap_horiz_pp_16x16_avx2;
        p.luma_hpp[LUMA_16x32] = x265_interp_8tap_horiz_pp_16x32_avx2;
        p.luma_hpp[LUMA_16x64] = x265_interp_8tap_horiz_pp_16x64_avx2;

        p.luma_hpp[LUMA_32x8] = x265_interp_8tap_horiz_pp_32x8_avx2;
        p.luma_hpp[LUMA_32x16] = x265_interp_8tap_horiz_pp_32x16_avx2;
        p.luma_hpp[LUMA_32x24] = x265_interp_8tap_horiz_pp_32x24_avx2;
        p.luma_hpp[LUMA_32x32] = x265_interp_8tap_horiz_pp_32x32_avx2;
        p.luma_hpp[LUMA_32x64] = x265_interp_8tap_horiz_pp_32x64_avx2;

        p.luma_hpp[LUMA_64x64] = x265_interp_8tap_horiz_pp_64x64_avx2;
        p.luma_hpp[LUMA_64x48] = x265_interp_8tap_horiz_pp_64x48_avx2;
        p.luma_hpp[LUMA_64x32] = x265_interp_8tap_horiz_pp_64x32_avx2;
        p.luma_hpp[LUMA_64x16] = x265_interp_8tap_horiz_pp_64x16_avx2;

        p.luma_hpp[LUMA_48x64] = x265_interp_8tap_horiz_pp_48x64_avx2;

        p.luma_vpp[LUMA_4x4] = x265_interp_8tap_vert_pp_4x4_avx2;

        p.luma_vpp[LUMA_8x4] = x265_interp_8tap_vert_pp_8x4_avx2;
        p.luma_vpp[LUMA_8x8] = x265_interp_8tap_vert_pp_8x8_avx2;
        p.luma_vpp[LUMA_8x16] = x265_interp_8tap_vert_pp_8x16_avx2;
        p.luma_vpp[LUMA_8x32] = x265_interp_8tap_vert_pp_8x32_avx2;
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
