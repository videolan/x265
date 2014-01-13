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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#include "primitives.h"
#include "x265.h"
#include "cpu.h"

extern "C" {
#include "pixel.h"
#include "pixel-util.h"
#include "mc.h"
#include "ipfilter8.h"
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
    p.satd[LUMA_32x32] = x265_pixel_satd_32x32_ ## cpu; \
    p.satd[LUMA_24x32] = x265_pixel_satd_24x32_ ## cpu; \
    p.satd[LUMA_64x64] = x265_pixel_satd_64x64_ ## cpu; \
    p.satd[LUMA_64x32] = x265_pixel_satd_64x32_ ## cpu; \
    p.satd[LUMA_32x64] = x265_pixel_satd_32x64_ ## cpu; \
    p.satd[LUMA_64x48] = x265_pixel_satd_64x48_ ## cpu; \
    p.satd[LUMA_48x64] = x265_pixel_satd_48x64_ ## cpu; \
    p.satd[LUMA_64x16] = x265_pixel_satd_64x16_ ## cpu

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
    p.sa8d_inter[LUMA_4x8]  = x265_pixel_satd_4x8_ ## cpu; \
    p.sa8d_inter[LUMA_8x4]  = x265_pixel_satd_8x4_ ## cpu; \
    p.sa8d_inter[LUMA_4x16]  = x265_pixel_satd_4x16_ ## cpu; \
    p.sa8d_inter[LUMA_16x4]  = x265_pixel_satd_16x4_ ## cpu; \
    p.sa8d_inter[LUMA_12x16]  = x265_pixel_satd_12x16_ ## cpu; \
    p.sa8d_inter[LUMA_16x12]  = x265_pixel_satd_16x12_ ## cpu; \
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

#define SETUP_CHROMA_FUNC_DEF(W, H, cpu) \
    p.chroma[X265_CSP_I420].filter_hpp[CHROMA_ ## W ## x ## H] = x265_interp_4tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].filter_hps[CHROMA_ ## W ## x ## H] = x265_interp_4tap_horiz_ps_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].filter_vpp[CHROMA_ ## W ## x ## H] = x265_interp_4tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.chroma[X265_CSP_I420].filter_vps[CHROMA_ ## W ## x ## H] = x265_interp_4tap_vert_ps_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SP_FUNC_DEF(W, H, cpu) \
    p.chroma[X265_CSP_I420].filter_vsp[CHROMA_ ## W ## x ## H] = x265_interp_4tap_vert_sp_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_SS_FUNC_DEF(W, H, cpu) \
    p.chroma[X265_CSP_I420].filter_vss[CHROMA_ ## W ## x ## H] = x265_interp_4tap_vert_ss_ ## W ## x ## H ## cpu;

#define CHROMA_FILTERS(cpu) \
    SETUP_CHROMA_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF(4, 2, cpu); \
    SETUP_CHROMA_FUNC_DEF(2, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 6, cpu); \
    SETUP_CHROMA_FUNC_DEF(6, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 2, cpu); \
    SETUP_CHROMA_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_FUNC_DEF(8, 32, cpu);

#define CHROMA_SP_FILTERS(cpu) \
    SETUP_CHROMA_SP_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 2, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 6, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 2, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_SP_FUNC_DEF(8, 32, cpu);

#define CHROMA_SS_FILTERS(cpu) \
    SETUP_CHROMA_SS_FUNC_DEF(4, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(4, 2, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(2, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(4, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 6, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(6, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 2, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(2, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 12, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(12, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 4, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(4, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 16, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(16, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 24, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(24, 32, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(32, 8, cpu); \
    SETUP_CHROMA_SS_FUNC_DEF(8, 32, cpu);

#define SETUP_LUMA_FUNC_DEF(W, H, cpu) \
    p.luma_hpp[LUMA_ ## W ## x ## H] = x265_interp_8tap_horiz_pp_ ## W ## x ## H ## cpu; \
    p.luma_hps[LUMA_ ## W ## x ## H] = x265_interp_8tap_horiz_ps_ ## W ## x ## H ## cpu; \
    p.luma_vpp[LUMA_ ## W ## x ## H] = x265_interp_8tap_vert_pp_ ## W ## x ## H ## cpu; \
    p.luma_vps[LUMA_ ## W ## x ## H] = x265_interp_8tap_vert_ps_ ## W ## x ## H ## cpu; \
    p.luma_copy_ps[LUMA_ ## W ## x ## H] = x265_blockcopy_ps_ ## W ## x ## H ## cpu;

#define SETUP_LUMA_SUB_FUNC_DEF(W, H, cpu) \
    p.luma_sub_ps[LUMA_ ## W ## x ## H] = x265_pixel_sub_ps_ ## W ## x ## H ## cpu; \
    p.luma_add_ps[LUMA_ ## W ## x ## H] = x265_pixel_add_ps_ ## W ## x ## H ## cpu;

#define SETUP_LUMA_SP_FUNC_DEF(W, H, cpu) \
    p.luma_vsp[LUMA_ ## W ## x ## H] = x265_interp_8tap_vert_sp_ ## W ## x ## H ## cpu;

#define SETUP_LUMA_SS_FUNC_DEF(W, H, cpu) \
    p.luma_vss[LUMA_ ## W ## x ## H] = x265_interp_8tap_vert_ss_ ## W ## x ## H ## cpu;

#define SETUP_LUMA_BLOCKCOPY_FUNC_DEF(W, H, cpu) \
    p.luma_copy_pp[LUMA_ ## W ## x ## H] = x265_blockcopy_pp_ ## W ## x ## H ## cpu;

#define SETUP_CHROMA_FROM_LUMA(W1, H1, W2, H2, cpu) \
    p.chroma[X265_CSP_I420].copy_pp[LUMA_ ## W1 ## x ## H1] = x265_blockcopy_pp_ ## W2 ## x ## H2 ## cpu;

// For X265_CSP_I420 chroma width and height will be half of luma width and height
#define CHROMA_BLOCKCOPY(cpu) \
    SETUP_CHROMA_FROM_LUMA(8,   8, 4,  4,  cpu); \
    SETUP_CHROMA_FROM_LUMA(8,   4, 4,  2,  cpu); \
    SETUP_CHROMA_FROM_LUMA(4,   8, 2,  4,  cpu); \
    SETUP_CHROMA_FROM_LUMA(16, 16, 8,  8,  cpu); \
    SETUP_CHROMA_FROM_LUMA(16,  8, 8,  4,  cpu); \
    SETUP_CHROMA_FROM_LUMA(8,  16, 4,  8,  cpu); \
    SETUP_CHROMA_FROM_LUMA(16, 12, 8,  6,  cpu); \
    SETUP_CHROMA_FROM_LUMA(12, 16, 6,  8,  cpu); \
    SETUP_CHROMA_FROM_LUMA(16,  4, 8,  2,  cpu); \
    SETUP_CHROMA_FROM_LUMA(4,  16, 2,  8,  cpu); \
    SETUP_CHROMA_FROM_LUMA(32, 32, 16, 16, cpu); \
    SETUP_CHROMA_FROM_LUMA(32, 16, 16, 8,  cpu); \
    SETUP_CHROMA_FROM_LUMA(16, 32, 8,  16, cpu); \
    SETUP_CHROMA_FROM_LUMA(32, 24, 16, 12, cpu); \
    SETUP_CHROMA_FROM_LUMA(24, 32, 12, 16, cpu); \
    SETUP_CHROMA_FROM_LUMA(32,  8, 16, 4,  cpu); \
    SETUP_CHROMA_FROM_LUMA(8,  32, 4,  16, cpu); \
    SETUP_CHROMA_FROM_LUMA(64, 64, 32, 32, cpu); \
    SETUP_CHROMA_FROM_LUMA(64, 32, 32, 16, cpu); \
    SETUP_CHROMA_FROM_LUMA(32, 64, 16, 32, cpu); \
    SETUP_CHROMA_FROM_LUMA(64, 48, 32, 24, cpu); \
    SETUP_CHROMA_FROM_LUMA(48, 64, 24, 32, cpu); \
    SETUP_CHROMA_FROM_LUMA(64, 16, 32, 8,  cpu); \
    SETUP_CHROMA_FROM_LUMA(16, 64, 8,  32, cpu);

#define SETUP_CHROMA_LUMA(W1, H1, W2, H2, cpu) \
    p.chroma[X265_CSP_I420].sub_ps[LUMA_ ## W1 ## x ## H1] = x265_pixel_sub_ps_ ## W2 ## x ## H2 ## cpu; \
    p.chroma[X265_CSP_I420].add_ps[LUMA_ ## W1 ## x ## H1] = x265_pixel_add_ps_ ## W2 ## x ## H2 ## cpu;

#define CHROMA_PIXELSUB_PS(cpu) \
    SETUP_CHROMA_LUMA(8,   8, 4,  4,  cpu); \
    SETUP_CHROMA_LUMA(8,   4, 4,  2,  cpu); \
    SETUP_CHROMA_LUMA(4,   8, 2,  4,  cpu); \
    SETUP_CHROMA_LUMA(16, 16, 8,  8,  cpu); \
    SETUP_CHROMA_LUMA(16,  8, 8,  4,  cpu); \
    SETUP_CHROMA_LUMA(8,  16, 4,  8,  cpu); \
    SETUP_CHROMA_LUMA(16, 12, 8,  6,  cpu); \
    SETUP_CHROMA_LUMA(12, 16, 6,  8,  cpu); \
    SETUP_CHROMA_LUMA(16,  4, 8,  2,  cpu); \
    SETUP_CHROMA_LUMA(4,  16, 2,  8,  cpu); \
    SETUP_CHROMA_LUMA(32, 32, 16, 16, cpu); \
    SETUP_CHROMA_LUMA(32, 16, 16, 8,  cpu); \
    SETUP_CHROMA_LUMA(16, 32, 8,  16, cpu); \
    SETUP_CHROMA_LUMA(32, 24, 16, 12, cpu); \
    SETUP_CHROMA_LUMA(24, 32, 12, 16, cpu); \
    SETUP_CHROMA_LUMA(32,  8, 16, 4,  cpu); \
    SETUP_CHROMA_LUMA(8,  32, 4,  16, cpu); \
    SETUP_CHROMA_LUMA(64, 64, 32, 32, cpu); \
    SETUP_CHROMA_LUMA(64, 32, 32, 16, cpu); \
    SETUP_CHROMA_LUMA(32, 64, 16, 32, cpu); \
    SETUP_CHROMA_LUMA(64, 48, 32, 24, cpu); \
    SETUP_CHROMA_LUMA(48, 64, 24, 32, cpu); \
    SETUP_CHROMA_LUMA(64, 16, 32, 8,  cpu); \
    SETUP_CHROMA_LUMA(16, 64, 8,  32, cpu);

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
    SETUP_LUMA_SUB_FUNC_DEF(8,   4, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(4,   8, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(16, 16, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(16,  8, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(8,  16, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(16, 12, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(12, 16, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(16,  4, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(4,  16, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(32, 32, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(32, 16, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(16, 32, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(32, 24, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(24, 32, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(32,  8, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(8,  32, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(64, 64, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(64, 32, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(32, 64, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(64, 48, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(48, 64, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(64, 16, cpu); \
    SETUP_LUMA_SUB_FUNC_DEF(16, 64, cpu);

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

#define LUMA_BLOCKCOPY(cpu) \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(4,   4, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(8,   8, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(8,   4, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(4,   8, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(16, 16, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(16,  8, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(8,  16, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(16, 12, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(12, 16, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(16,  4, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(4,  16, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(32, 32, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(32, 16, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(16, 32, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(32, 24, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(24, 32, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(32,  8, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(8,  32, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(64, 64, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(64, 32, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(32, 64, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(64, 48, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(48, 64, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(64, 16, cpu); \
    SETUP_LUMA_BLOCKCOPY_FUNC_DEF(16, 64, cpu);

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

#define SETUP_INTRA_ANG4(mode, fno, cpu) \
    p.intra_pred[BLOCK_4x4][mode] = x265_intra_pred_ang4_ ## fno ## _ ## cpu;
#define SETUP_INTRA_ANG8(mode, fno, cpu) \
    p.intra_pred[BLOCK_8x8][mode] = x265_intra_pred_ang8_ ## fno ## _ ## cpu;
#define SETUP_INTRA_ANG16(mode, fno, cpu) \
    p.intra_pred[BLOCK_16x16][mode] = x265_intra_pred_ang16_ ## fno ## _ ## cpu;

namespace x265 {
// private x265 namespace
void Setup_Assembly_Primitives(EncoderPrimitives &p, int cpuMask)
{
#if HIGH_BIT_DEPTH
    if (cpuMask & X265_CPU_SSE2)
    {
        INIT6(satd, _sse2);
        HEVC_SATD(sse2);
        p.satd[LUMA_4x4] = x265_pixel_satd_4x4_mmx2;
        p.satd[LUMA_4x16] = x265_pixel_satd_4x16_sse2;
        p.satd[LUMA_8x32] = x265_pixel_satd_8x32_sse2;
        p.satd[LUMA_16x4] = x265_pixel_satd_16x4_sse2;
        p.satd[LUMA_16x12] = x265_pixel_satd_16x12_sse2;
        p.satd[LUMA_16x32] = x265_pixel_satd_16x32_sse2;
        p.satd[LUMA_16x64] = x265_pixel_satd_16x64_sse2;
        p.satd[LUMA_12x16] = x265_pixel_satd_12x16_sse2;
        p.satd[LUMA_32x8] = x265_pixel_satd_32x8_sse2;
        p.satd[LUMA_32x16] = x265_pixel_satd_32x16_sse2;
        p.satd[LUMA_32x24] = x265_pixel_satd_32x24_sse2;

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

        INIT8(sad, _mmx2);
        p.sad[LUMA_8x32]  = x265_pixel_sad_8x32_sse2;
        p.sad[LUMA_16x4]  = x265_pixel_sad_16x4_sse2;
        p.sad[LUMA_16x12] = x265_pixel_sad_16x12_sse2;
        p.sad[LUMA_16x32] = x265_pixel_sad_16x32_sse2;

        p.sad[LUMA_32x8]  = x265_pixel_sad_32x8_sse2;
        p.sad[LUMA_32x16] = x265_pixel_sad_32x16_sse2;
        p.sad[LUMA_32x24] = x265_pixel_sad_32x24_sse2;
        p.sad[LUMA_32x32] = x265_pixel_sad_32x32_sse2;
        p.sad[LUMA_32x64] = x265_pixel_sad_32x64_sse2;

        p.sad[LUMA_64x16] = x265_pixel_sad_64x16_sse2;
        p.sad[LUMA_64x32] = x265_pixel_sad_64x32_sse2;
        p.sad[LUMA_64x48] = x265_pixel_sad_64x48_sse2;
        p.sad[LUMA_64x64] = x265_pixel_sad_64x64_sse2;

        p.sad[LUMA_48x64] = x265_pixel_sad_48x64_sse2;
        p.sad[LUMA_24x32] = x265_pixel_sad_24x32_sse2;
        p.sad[LUMA_12x16] = x265_pixel_sad_12x16_sse2;

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

        p.cvt32to16_shr = x265_cvt32to16_shr_sse2;
        p.cvt16to32_shl = x265_cvt16to32_shl_sse2;

        CHROMA_PIXELSUB_PS(_sse2);
        LUMA_PIXELSUB(_sse2);

        CHROMA_BLOCKCOPY(_sse2);
        LUMA_BLOCKCOPY(_sse2);

        p.blockfill_s[BLOCK_4x4] = x265_blockfill_s_4x4_sse2;
        p.blockfill_s[BLOCK_8x8] = x265_blockfill_s_8x8_sse2;
        p.blockfill_s[BLOCK_16x16] = x265_blockfill_s_16x16_sse2;
        p.blockfill_s[BLOCK_32x32] = x265_blockfill_s_32x32_sse2;

        p.calcresidual[BLOCK_4x4] = x265_getResidual4_sse2;
        p.calcresidual[BLOCK_8x8] = x265_getResidual8_sse2;
        p.calcresidual[BLOCK_16x16] = x265_getResidual16_sse2;
        p.calcresidual[BLOCK_32x32] = x265_getResidual32_sse2;

        p.calcrecon[BLOCK_4x4] = x265_calcRecons4_sse2;
        p.calcrecon[BLOCK_8x8] = x265_calcRecons8_sse2;
        p.calcrecon[BLOCK_16x16] = x265_calcRecons16_sse2;
        p.calcrecon[BLOCK_32x32] = x265_calcRecons32_sse2;
    }
    if (cpuMask & X265_CPU_SSSE3)
    {
        p.scale1D_128to64 = x265_scale1D_128to64_ssse3;
        p.scale2D_64to32 = x265_scale2D_64to32_ssse3;

        SETUP_INTRA_ANG4(2, 2, ssse3);
        SETUP_INTRA_ANG4(34, 2, ssse3);
    }
    if (cpuMask & X265_CPU_SSE4)
    {
        p.intra_pred[BLOCK_4x4][0] = x265_intra_pred_planar4_sse4;
        p.intra_pred[BLOCK_8x8][0] = x265_intra_pred_planar8_sse4;
        p.intra_pred[BLOCK_16x16][0] = x265_intra_pred_planar16_sse4;
        p.intra_pred[BLOCK_32x32][0] = x265_intra_pred_planar32_sse4;

        p.intra_pred[BLOCK_4x4][1] = x265_intra_pred_dc4_sse4;
        p.intra_pred[BLOCK_8x8][1] = x265_intra_pred_dc8_sse4;
        p.intra_pred[BLOCK_16x16][1] = x265_intra_pred_dc16_sse4;
        p.intra_pred[BLOCK_32x32][1] = x265_intra_pred_dc32_sse4;

        SETUP_INTRA_ANG4(3, 3, sse4);
        SETUP_INTRA_ANG4(4, 4, sse4);
        SETUP_INTRA_ANG4(5, 5, sse4);
        SETUP_INTRA_ANG4(6, 6, sse4);
        SETUP_INTRA_ANG4(7, 7, sse4);
        SETUP_INTRA_ANG4(8, 8, sse4);
        SETUP_INTRA_ANG4(9, 9, sse4);
        SETUP_INTRA_ANG4(10, 10, sse4);
        SETUP_INTRA_ANG4(11, 11, sse4);
        SETUP_INTRA_ANG4(12, 12, sse4);
        SETUP_INTRA_ANG4(13, 13, sse4);
        SETUP_INTRA_ANG4(14, 14, sse4);
        SETUP_INTRA_ANG4(15, 15, sse4);
        SETUP_INTRA_ANG4(16, 16, sse4);
        SETUP_INTRA_ANG4(17, 17, sse4);
        SETUP_INTRA_ANG4(18, 18, sse4);
        SETUP_INTRA_ANG4(19, 17, sse4);
        SETUP_INTRA_ANG4(20, 16, sse4);
        SETUP_INTRA_ANG4(21, 15, sse4);
        SETUP_INTRA_ANG4(22, 14, sse4);
        SETUP_INTRA_ANG4(23, 13, sse4);
        SETUP_INTRA_ANG4(24, 12, sse4);
        SETUP_INTRA_ANG4(25, 11, sse4);
        SETUP_INTRA_ANG4(26, 26, sse4);
        SETUP_INTRA_ANG4(27, 9, sse4);
        SETUP_INTRA_ANG4(28, 8, sse4);
        SETUP_INTRA_ANG4(29, 7, sse4);
        SETUP_INTRA_ANG4(30, 6, sse4);
        SETUP_INTRA_ANG4(31, 5, sse4);
        SETUP_INTRA_ANG4(32, 4, sse4);
        SETUP_INTRA_ANG4(33, 3, sse4);
    }
    if (cpuMask & X265_CPU_XOP)
    {
    }
    if (cpuMask & X265_CPU_AVX2)
    {
    }

    /* at HIGH_BIT_DEPTH, pixel == short so we can reuse a number of primitives */
    for (int i = 0; i < NUM_LUMA_PARTITIONS; i++)
    {
        p.sse_pp[i] = (pixelcmp_t)p.sse_ss[i];
        p.sse_sp[i] = (pixelcmp_sp_t)p.sse_ss[i];
    }

  for (int i = 0; i < NUM_LUMA_PARTITIONS; i++)
    {
        p.luma_copy_ps[i] = (copy_ps_t)p.luma_copy_pp[i];
        p.luma_copy_sp[i] = (copy_sp_t)p.luma_copy_pp[i];
    }

    for (int i = 0; i < NUM_CHROMA_PARTITIONS; i++)
    {
        p.chroma[X265_CSP_I420].copy_ps[i] = (copy_ps_t)p.chroma[X265_CSP_I420].copy_pp[i];
        p.chroma[X265_CSP_I420].copy_sp[i] = (copy_sp_t)p.chroma[X265_CSP_I420].copy_pp[i];
    }

#else // if HIGH_BIT_DEPTH
    if (cpuMask & X265_CPU_SSE2)
    {
        INIT8_NAME(sse_pp, ssd, _mmx);
        INIT8(sad, _mmx2);
        INIT8(sad_x3, _mmx2);
        INIT8(sad_x4, _mmx2);
        INIT8(satd, _mmx2);
        p.sa8d_inter[LUMA_4x4]  = x265_pixel_satd_4x4_mmx2;
        p.satd[LUMA_8x32] = x265_pixel_satd_8x32_sse2;
        p.satd[LUMA_12x16] = x265_pixel_satd_12x16_sse2;
        p.satd[LUMA_16x4] = x265_pixel_satd_16x4_sse2;
        p.satd[LUMA_16x12] = x265_pixel_satd_16x12_sse2;
        p.satd[LUMA_16x32] = x265_pixel_satd_16x32_sse2;
        p.satd[LUMA_16x64] = x265_pixel_satd_16x64_sse2;
        p.satd[LUMA_32x8]  = x265_pixel_satd_32x8_sse2;
        p.satd[LUMA_32x16] = x265_pixel_satd_32x16_sse2;
        p.satd[LUMA_32x24] = x265_pixel_satd_32x24_sse2;
        p.frame_init_lowres_core = x265_frame_init_lowres_core_mmx2;

        PIXEL_AVG(sse2);
        PIXEL_AVG_W4(mmx2);

        LUMA_VAR(_sse2);

        p.sad[LUMA_8x32]  = x265_pixel_sad_8x32_sse2;
        p.sad[LUMA_16x4]  = x265_pixel_sad_16x4_sse2;
        p.sad[LUMA_16x12] = x265_pixel_sad_16x12_sse2;
        p.sad[LUMA_16x32] = x265_pixel_sad_16x32_sse2;
        p.sad[LUMA_16x64] = x265_pixel_sad_16x64_sse2;

        p.sad[LUMA_32x8]  = x265_pixel_sad_32x8_sse2;
        p.sad[LUMA_32x16] = x265_pixel_sad_32x16_sse2;
        p.sad[LUMA_32x24] = x265_pixel_sad_32x24_sse2;
        p.sad[LUMA_32x32] = x265_pixel_sad_32x32_sse2;
        p.sad[LUMA_32x64] = x265_pixel_sad_32x64_sse2;

        p.sad[LUMA_64x16] = x265_pixel_sad_64x16_sse2;
        p.sad[LUMA_64x32] = x265_pixel_sad_64x32_sse2;
        p.sad[LUMA_64x48] = x265_pixel_sad_64x48_sse2;
        p.sad[LUMA_64x64] = x265_pixel_sad_64x64_sse2;

        p.sad[LUMA_48x64] = x265_pixel_sad_48x64_sse2;
        p.sad[LUMA_24x32] = x265_pixel_sad_24x32_sse2;
        p.sad[LUMA_12x16] = x265_pixel_sad_12x16_sse2;

        ASSGN_SSE(sse2);
        INIT2(sad, _sse2);
        INIT2(sad_x3, _sse2);
        INIT2(sad_x4, _sse2);
        INIT6(satd, _sse2);
        HEVC_SATD(sse2);

        CHROMA_BLOCKCOPY(_sse2);
        LUMA_BLOCKCOPY(_sse2);

        CHROMA_SS_FILTERS(_sse2);
        LUMA_SS_FILTERS(_sse2);

        // This function pointer initialization is temporary will be removed
        // later with macro definitions.  It is used to avoid linker errors
        // until all partitions are coded and commit smaller patches, easier to
        // review.

        p.chroma[X265_CSP_I420].copy_sp[CHROMA_4x2] = x265_blockcopy_sp_4x2_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_4x4] = x265_blockcopy_sp_4x4_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_4x8] = x265_blockcopy_sp_4x8_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_4x16] = x265_blockcopy_sp_4x16_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_8x2] = x265_blockcopy_sp_8x2_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_8x4] = x265_blockcopy_sp_8x4_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_8x6] = x265_blockcopy_sp_8x6_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_8x8] = x265_blockcopy_sp_8x8_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_8x16] = x265_blockcopy_sp_8x16_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_12x16] = x265_blockcopy_sp_12x16_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_16x4] = x265_blockcopy_sp_16x4_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_16x8] = x265_blockcopy_sp_16x8_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_16x12] = x265_blockcopy_sp_16x12_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_16x16] = x265_blockcopy_sp_16x16_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_16x32] = x265_blockcopy_sp_16x32_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_24x32] = x265_blockcopy_sp_24x32_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_32x8] = x265_blockcopy_sp_32x8_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_32x16] = x265_blockcopy_sp_32x16_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_32x24] = x265_blockcopy_sp_32x24_sse2;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_32x32] = x265_blockcopy_sp_32x32_sse2;

        p.luma_copy_sp[LUMA_32x64] = x265_blockcopy_sp_32x64_sse2;
        p.luma_copy_sp[LUMA_16x64] = x265_blockcopy_sp_16x64_sse2;
        p.luma_copy_sp[LUMA_48x64] = x265_blockcopy_sp_48x64_sse2;
        p.luma_copy_sp[LUMA_64x16] = x265_blockcopy_sp_64x16_sse2;
        p.luma_copy_sp[LUMA_64x32] = x265_blockcopy_sp_64x32_sse2;
        p.luma_copy_sp[LUMA_64x48] = x265_blockcopy_sp_64x48_sse2;
        p.luma_copy_sp[LUMA_64x64] = x265_blockcopy_sp_64x64_sse2;
        p.blockfill_s[BLOCK_4x4] = x265_blockfill_s_4x4_sse2;
        p.blockfill_s[BLOCK_8x8] = x265_blockfill_s_8x8_sse2;
        p.blockfill_s[BLOCK_16x16] = x265_blockfill_s_16x16_sse2;
        p.blockfill_s[BLOCK_32x32] = x265_blockfill_s_32x32_sse2;

        p.frame_init_lowres_core = x265_frame_init_lowres_core_sse2;
        SA8D_INTER_FROM_BLOCK(sse2);

        p.cvt32to16_shr = x265_cvt32to16_shr_sse2;
        p.cvt16to32_shl = x265_cvt16to32_shl_sse2;
        p.ipfilter_ss[FILTER_V_S_S_8] = x265_interp_8tap_v_ss_sse2;
        p.calcrecon[BLOCK_4x4] = x265_calcRecons4_sse2;
        p.calcrecon[BLOCK_8x8] = x265_calcRecons8_sse2;
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
    }
    if (cpuMask & X265_CPU_SSSE3)
    {
        p.frame_init_lowres_core = x265_frame_init_lowres_core_ssse3;
        SA8D_INTER_FROM_BLOCK(ssse3);
        p.sse_pp[LUMA_4x4] = x265_pixel_ssd_4x4_ssse3;
        ASSGN_SSE(ssse3);
        PIXEL_AVG(ssse3);
        PIXEL_AVG_W4(ssse3);

        SETUP_INTRA_ANG4(2, 2, ssse3);
        SETUP_INTRA_ANG4(34, 2, ssse3);
        SETUP_INTRA_ANG8(2, 2, ssse3);
        SETUP_INTRA_ANG8(34, 2, ssse3);
        SETUP_INTRA_ANG16(2, 2, ssse3);
        SETUP_INTRA_ANG16(34, 2, ssse3);

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
        p.chroma_p2s = x265_chroma_p2s_ssse3;

        CHROMA_SP_FILTERS(_ssse3);
        LUMA_SP_FILTERS(_ssse3);

        p.dct[DST_4x4] = x265_dst4_ssse3;
    }
    if (cpuMask & X265_CPU_SSE4)
    {
        p.satd[LUMA_4x16]   = x265_pixel_satd_4x16_sse4;
        p.satd[LUMA_12x16]  = x265_pixel_satd_12x16_sse4;
        p.satd[LUMA_32x8] = x265_pixel_satd_32x8_sse4;
        p.satd[LUMA_32x16] = x265_pixel_satd_32x16_sse4;
        p.satd[LUMA_32x24] = x265_pixel_satd_32x24_sse4;
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
        LUMA_PIXELSUB(_sse4);

        CHROMA_FILTERS(_sse4);
        LUMA_FILTERS(_sse4);
        HEVC_SATD(sse4);
        ASSGN_SSE_SS(sse4);
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_2x4] = x265_blockcopy_sp_2x4_sse4;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_2x8] = x265_blockcopy_sp_2x8_sse4;
        p.chroma[X265_CSP_I420].copy_sp[CHROMA_6x8] = x265_blockcopy_sp_6x8_sse4;

        p.chroma[X265_CSP_I420].filter_vsp[CHROMA_2x4] = x265_interp_4tap_vert_sp_2x4_sse4;
        p.chroma[X265_CSP_I420].filter_vsp[CHROMA_2x8] = x265_interp_4tap_vert_sp_2x8_sse4;
        p.chroma[X265_CSP_I420].filter_vsp[CHROMA_6x8] = x265_interp_4tap_vert_sp_6x8_sse4;

        p.calcrecon[BLOCK_16x16] = x265_calcRecons16_sse4;
        p.calcrecon[BLOCK_32x32] = x265_calcRecons32_sse4;
        p.calcresidual[BLOCK_16x16] = x265_getResidual16_sse4;
        p.calcresidual[BLOCK_32x32] = x265_getResidual32_sse4;
        p.quant = x265_quant_sse4;
        p.dequant_normal = x265_dequant_normal_sse4;
        p.weight_pp = x265_weight_pp_sse4;
        p.weight_sp = x265_weight_sp_sse4;
        p.intra_pred[BLOCK_4x4][0] = x265_intra_pred_planar4_sse4;
        p.intra_pred[BLOCK_8x8][0] = x265_intra_pred_planar8_sse4;
        p.intra_pred[BLOCK_16x16][0] = x265_intra_pred_planar16_sse4;
        p.intra_pred[BLOCK_32x32][0] = x265_intra_pred_planar32_sse4;

        p.intra_pred_allangs[BLOCK_4x4] = x265_all_angs_pred_4x4_sse4;
        p.intra_pred_allangs[BLOCK_8x8] = x265_all_angs_pred_8x8_sse4;

        p.intra_pred[BLOCK_4x4][1] = x265_intra_pred_dc4_sse4;
        p.intra_pred[BLOCK_8x8][1] = x265_intra_pred_dc8_sse4;
        p.intra_pred[BLOCK_16x16][1] = x265_intra_pred_dc16_sse4;
        p.intra_pred[BLOCK_32x32][1] = x265_intra_pred_dc32_sse4;
        SETUP_INTRA_ANG4(3, 3, sse4);
        SETUP_INTRA_ANG4(4, 4, sse4);
        SETUP_INTRA_ANG4(5, 5, sse4);
        SETUP_INTRA_ANG4(6, 6, sse4);
        SETUP_INTRA_ANG4(7, 7, sse4);
        SETUP_INTRA_ANG4(8, 8, sse4);
        SETUP_INTRA_ANG4(9, 9, sse4);
        SETUP_INTRA_ANG4(10, 10, sse4);
        SETUP_INTRA_ANG4(11, 11, sse4);
        SETUP_INTRA_ANG4(12, 12, sse4);
        SETUP_INTRA_ANG4(13, 13, sse4);
        SETUP_INTRA_ANG4(14, 14, sse4);
        SETUP_INTRA_ANG4(15, 15, sse4);
        SETUP_INTRA_ANG4(16, 16, sse4);
        SETUP_INTRA_ANG4(17, 17, sse4);
        SETUP_INTRA_ANG4(18, 18, sse4);
        SETUP_INTRA_ANG4(19, 17, sse4);
        SETUP_INTRA_ANG4(20, 16, sse4);
        SETUP_INTRA_ANG4(21, 15, sse4);
        SETUP_INTRA_ANG4(22, 14, sse4);
        SETUP_INTRA_ANG4(23, 13, sse4);
        SETUP_INTRA_ANG4(24, 12, sse4);
        SETUP_INTRA_ANG4(25, 11, sse4);
        SETUP_INTRA_ANG4(26, 26, sse4);
        SETUP_INTRA_ANG4(27, 9, sse4);
        SETUP_INTRA_ANG4(28, 8, sse4);
        SETUP_INTRA_ANG4(29, 7, sse4);
        SETUP_INTRA_ANG4(30, 6, sse4);
        SETUP_INTRA_ANG4(31, 5, sse4);
        SETUP_INTRA_ANG4(32, 4, sse4);
        SETUP_INTRA_ANG4(33, 3, sse4);

        p.dct[DCT_8x8] = x265_dct8_sse4;
    }
    if (cpuMask & X265_CPU_AVX)
    {
        p.frame_init_lowres_core = x265_frame_init_lowres_core_avx;
        p.satd[LUMA_4x16]   = x265_pixel_satd_4x16_avx;
        p.satd[LUMA_12x16]  = x265_pixel_satd_12x16_avx;
        p.satd[LUMA_32x8] = x265_pixel_satd_32x8_avx;
        p.satd[LUMA_32x16] = x265_pixel_satd_32x16_avx;
        p.satd[LUMA_32x24] = x265_pixel_satd_32x24_avx;
        SA8D_INTER_FROM_BLOCK(avx);
        ASSGN_SSE(avx);
        HEVC_SATD(avx);
        ASSGN_SSE_SS(avx);
        SAD_X3(avx);
        SAD_X3(avx);
        p.sad_x3[LUMA_12x16] = x265_pixel_sad_x3_12x16_avx;
        p.sad_x4[LUMA_12x16] = x265_pixel_sad_x4_12x16_avx;
        p.sad_x3[LUMA_16x4]  = x265_pixel_sad_x3_16x4_avx;
        p.sad_x4[LUMA_16x4]  = x265_pixel_sad_x4_16x4_avx;

        p.ssim_4x4x2_core = x265_pixel_ssim_4x4x2_core_avx;
        p.ssim_end_4 = x265_pixel_ssim_end4_avx;
    }
    if (cpuMask & X265_CPU_XOP)
    {
        p.frame_init_lowres_core = x265_frame_init_lowres_core_xop;
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
