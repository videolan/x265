/*****************************************************************************
 * Copyright (C) 2016 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
 *          Min Chen <chenm003@163.com> <min.chen@multicorewareinc.com>
 *          Dnyaneshwar Gorade <dnyaneshwar@multicorewareinc.com>
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
#include "blockcopy8.h"
#include "pixel.h"
#include "pixel-util.h"
}

namespace X265_NS {
// private x265 namespace

void setupAssemblyPrimitives(EncoderPrimitives &p, int cpuMask)
{
    if (cpuMask & X265_CPU_NEON)
    {
        // Blockcopy_sp
        p.cu[BLOCK_4x4].copy_sp   = PFX(blockcopy_sp_4x4_neon);
        p.cu[BLOCK_8x8].copy_sp   = PFX(blockcopy_sp_8x8_neon);
        p.cu[BLOCK_16x16].copy_sp = PFX(blockcopy_sp_16x16_neon);
        p.cu[BLOCK_32x32].copy_sp = PFX(blockcopy_sp_32x32_neon);
        p.cu[BLOCK_64x64].copy_sp = PFX(blockcopy_sp_64x64_neon);

        // Blockcopy_ps
        p.cu[BLOCK_4x4].copy_ps   = PFX(blockcopy_ps_4x4_neon);
        p.cu[BLOCK_8x8].copy_ps   = PFX(blockcopy_ps_8x8_neon);
        p.cu[BLOCK_16x16].copy_ps = PFX(blockcopy_ps_16x16_neon);
        p.cu[BLOCK_32x32].copy_ps = PFX(blockcopy_ps_32x32_neon);
        p.cu[BLOCK_64x64].copy_ps = PFX(blockcopy_ps_64x64_neon);

        // pixel_add_ps
        p.cu[BLOCK_4x4].add_ps   = PFX(pixel_add_ps_4x4_neon);
        p.cu[BLOCK_8x8].add_ps   = PFX(pixel_add_ps_8x8_neon);
        p.cu[BLOCK_16x16].add_ps = PFX(pixel_add_ps_16x16_neon);
        p.cu[BLOCK_32x32].add_ps = PFX(pixel_add_ps_32x32_neon);
        p.cu[BLOCK_64x64].add_ps = PFX(pixel_add_ps_64x64_neon);

        // cpy2Dto1D_shr
        p.cu[BLOCK_4x4].cpy2Dto1D_shr   = PFX(cpy2Dto1D_shr_4x4_neon);
        p.cu[BLOCK_8x8].cpy2Dto1D_shr   = PFX(cpy2Dto1D_shr_8x8_neon);
        p.cu[BLOCK_16x16].cpy2Dto1D_shr = PFX(cpy2Dto1D_shr_16x16_neon);
        p.cu[BLOCK_32x32].cpy2Dto1D_shr = PFX(cpy2Dto1D_shr_32x32_neon);

        // ssd_s
        p.cu[BLOCK_4x4].ssd_s   = PFX(pixel_ssd_s_4x4_neon);
        p.cu[BLOCK_8x8].ssd_s   = PFX(pixel_ssd_s_8x8_neon);
        p.cu[BLOCK_16x16].ssd_s = PFX(pixel_ssd_s_16x16_neon);
        p.cu[BLOCK_32x32].ssd_s = PFX(pixel_ssd_s_32x32_neon);

        // sse_ss
        p.cu[BLOCK_4x4].sse_ss   = PFX(pixel_sse_ss_4x4_neon);
        p.cu[BLOCK_8x8].sse_ss   = PFX(pixel_sse_ss_8x8_neon);
        p.cu[BLOCK_16x16].sse_ss = PFX(pixel_sse_ss_16x16_neon);
        p.cu[BLOCK_32x32].sse_ss = PFX(pixel_sse_ss_32x32_neon);
        p.cu[BLOCK_64x64].sse_ss = PFX(pixel_sse_ss_64x64_neon);

        // pixel_sub_ps
        p.cu[BLOCK_4x4].sub_ps   = PFX(pixel_sub_ps_4x4_neon);
        p.cu[BLOCK_8x8].sub_ps   = PFX(pixel_sub_ps_8x8_neon);
        p.cu[BLOCK_16x16].sub_ps = PFX(pixel_sub_ps_16x16_neon);
        p.cu[BLOCK_32x32].sub_ps = PFX(pixel_sub_ps_32x32_neon);
        p.cu[BLOCK_64x64].sub_ps = PFX(pixel_sub_ps_64x64_neon);

        // calc_Residual
        p.cu[BLOCK_4x4].calcresidual   = PFX(getResidual4_neon);
        p.cu[BLOCK_8x8].calcresidual   = PFX(getResidual8_neon);
        p.cu[BLOCK_16x16].calcresidual = PFX(getResidual16_neon);
        p.cu[BLOCK_32x32].calcresidual = PFX(getResidual32_neon);

        // sse_pp
        p.cu[BLOCK_4x4].sse_pp   = PFX(pixel_sse_pp_4x4_neon);
        p.cu[BLOCK_8x8].sse_pp   = PFX(pixel_sse_pp_8x8_neon);
        p.cu[BLOCK_16x16].sse_pp = PFX(pixel_sse_pp_16x16_neon);
        p.cu[BLOCK_32x32].sse_pp = PFX(pixel_sse_pp_32x32_neon);
        p.cu[BLOCK_64x64].sse_pp = PFX(pixel_sse_pp_64x64_neon);

        // pixel_var
        p.cu[BLOCK_8x8].var   = PFX(pixel_var_8x8_neon);
        p.cu[BLOCK_16x16].var = PFX(pixel_var_16x16_neon);
        p.cu[BLOCK_32x32].var = PFX(pixel_var_32x32_neon);
        p.cu[BLOCK_64x64].var = PFX(pixel_var_64x64_neon);

        // blockcopy
        p.pu[LUMA_16x16].copy_pp = PFX(blockcopy_pp_16x16_neon);
        p.pu[LUMA_8x4].copy_pp   = PFX(blockcopy_pp_8x4_neon);
        p.pu[LUMA_8x8].copy_pp   = PFX(blockcopy_pp_8x8_neon);
        p.pu[LUMA_8x16].copy_pp  = PFX(blockcopy_pp_8x16_neon);
        p.pu[LUMA_8x32].copy_pp  = PFX(blockcopy_pp_8x32_neon);
        p.pu[LUMA_12x16].copy_pp = PFX(blockcopy_pp_12x16_neon);
        p.pu[LUMA_4x4].copy_pp   = PFX(blockcopy_pp_4x4_neon);
        p.pu[LUMA_4x8].copy_pp   = PFX(blockcopy_pp_4x8_neon);
        p.pu[LUMA_4x16].copy_pp  = PFX(blockcopy_pp_4x16_neon);
        p.pu[LUMA_16x4].copy_pp  = PFX(blockcopy_pp_16x4_neon);
        p.pu[LUMA_16x8].copy_pp  = PFX(blockcopy_pp_16x8_neon);
        p.pu[LUMA_16x12].copy_pp = PFX(blockcopy_pp_16x12_neon);
        p.pu[LUMA_16x32].copy_pp = PFX(blockcopy_pp_16x32_neon);
        p.pu[LUMA_16x64].copy_pp = PFX(blockcopy_pp_16x64_neon);
        p.pu[LUMA_24x32].copy_pp = PFX(blockcopy_pp_24x32_neon);
        p.pu[LUMA_32x8].copy_pp  = PFX(blockcopy_pp_32x8_neon);
        p.pu[LUMA_32x16].copy_pp = PFX(blockcopy_pp_32x16_neon);
        p.pu[LUMA_32x24].copy_pp = PFX(blockcopy_pp_32x24_neon);
        p.pu[LUMA_32x32].copy_pp = PFX(blockcopy_pp_32x32_neon);
        p.pu[LUMA_32x64].copy_pp = PFX(blockcopy_pp_32x64_neon);
        p.pu[LUMA_48x64].copy_pp = PFX(blockcopy_pp_48x64_neon);
        p.pu[LUMA_64x16].copy_pp = PFX(blockcopy_pp_64x16_neon);
        p.pu[LUMA_64x32].copy_pp = PFX(blockcopy_pp_64x32_neon);
        p.pu[LUMA_64x48].copy_pp = PFX(blockcopy_pp_64x48_neon);
        p.pu[LUMA_64x64].copy_pp = PFX(blockcopy_pp_64x64_neon);

        // sad
        p.pu[LUMA_8x4].sad    = PFX(pixel_sad_8x4_neon);
        p.pu[LUMA_8x8].sad    = PFX(pixel_sad_8x8_neon);
        p.pu[LUMA_8x16].sad   = PFX(pixel_sad_8x16_neon);
        p.pu[LUMA_8x32].sad   = PFX(pixel_sad_8x32_neon);
        p.pu[LUMA_16x4].sad   = PFX(pixel_sad_16x4_neon);
        p.pu[LUMA_16x8].sad   = PFX(pixel_sad_16x8_neon);
        p.pu[LUMA_16x16].sad  = PFX(pixel_sad_16x16_neon);
        p.pu[LUMA_16x12].sad  = PFX(pixel_sad_16x12_neon);
        p.pu[LUMA_16x32].sad  = PFX(pixel_sad_16x32_neon);
        p.pu[LUMA_16x64].sad  = PFX(pixel_sad_16x64_neon);
        p.pu[LUMA_32x8].sad   = PFX(pixel_sad_32x8_neon);
        p.pu[LUMA_32x16].sad  = PFX(pixel_sad_32x16_neon);
        p.pu[LUMA_32x32].sad  = PFX(pixel_sad_32x32_neon);
        p.pu[LUMA_32x64].sad  = PFX(pixel_sad_32x64_neon);
        p.pu[LUMA_32x24].sad  = PFX(pixel_sad_32x24_neon);
        p.pu[LUMA_64x16].sad  = PFX(pixel_sad_64x16_neon);
        p.pu[LUMA_64x32].sad  = PFX(pixel_sad_64x32_neon);
        p.pu[LUMA_64x64].sad  = PFX(pixel_sad_64x64_neon);
        p.pu[LUMA_64x48].sad  = PFX(pixel_sad_64x48_neon);
        p.pu[LUMA_12x16].sad  = PFX(pixel_sad_12x16_neon);
        p.pu[LUMA_24x32].sad  = PFX(pixel_sad_24x32_neon);
        p.pu[LUMA_48x64].sad  = PFX(pixel_sad_48x64_neon);

        // sad_x3
        p.pu[LUMA_4x4].sad_x3   = PFX(sad_x3_4x4_neon);
        p.pu[LUMA_4x8].sad_x3   = PFX(sad_x3_4x8_neon);
        p.pu[LUMA_4x16].sad_x3  = PFX(sad_x3_4x16_neon);
        p.pu[LUMA_8x4].sad_x3   = PFX(sad_x3_8x4_neon);
        p.pu[LUMA_8x8].sad_x3   = PFX(sad_x3_8x8_neon);
        p.pu[LUMA_8x16].sad_x3  = PFX(sad_x3_8x16_neon);
        p.pu[LUMA_8x32].sad_x3  = PFX(sad_x3_8x32_neon);
        p.pu[LUMA_12x16].sad_x3 = PFX(sad_x3_12x16_neon);
        p.pu[LUMA_16x4].sad_x3  = PFX(sad_x3_16x4_neon);
        p.pu[LUMA_16x8].sad_x3  = PFX(sad_x3_16x8_neon);
        p.pu[LUMA_16x12].sad_x3 = PFX(sad_x3_16x12_neon);
        p.pu[LUMA_16x16].sad_x3 = PFX(sad_x3_16x16_neon);
        p.pu[LUMA_16x32].sad_x3 = PFX(sad_x3_16x32_neon);
        p.pu[LUMA_16x64].sad_x3 = PFX(sad_x3_16x64_neon);
        p.pu[LUMA_24x32].sad_x3 = PFX(sad_x3_24x32_neon);
        p.pu[LUMA_32x8].sad_x3  = PFX(sad_x3_32x8_neon);
        p.pu[LUMA_32x16].sad_x3 = PFX(sad_x3_32x16_neon);
        p.pu[LUMA_32x24].sad_x3 = PFX(sad_x3_32x24_neon);
        p.pu[LUMA_32x32].sad_x3 = PFX(sad_x3_32x32_neon);
        p.pu[LUMA_32x64].sad_x3 = PFX(sad_x3_32x64_neon);
        p.pu[LUMA_48x64].sad_x3 = PFX(sad_x3_48x64_neon);
        p.pu[LUMA_64x16].sad_x3 = PFX(sad_x3_64x16_neon);
        p.pu[LUMA_64x32].sad_x3 = PFX(sad_x3_64x32_neon);
        p.pu[LUMA_64x48].sad_x3 = PFX(sad_x3_64x48_neon);
        p.pu[LUMA_64x64].sad_x3 = PFX(sad_x3_64x64_neon);

        // sad_x4
        p.pu[LUMA_4x4].sad_x4   = PFX(sad_x4_4x4_neon);
        p.pu[LUMA_4x8].sad_x4   = PFX(sad_x4_4x8_neon);
        p.pu[LUMA_4x16].sad_x4  = PFX(sad_x4_4x16_neon);
        p.pu[LUMA_8x4].sad_x4   = PFX(sad_x4_8x4_neon);
        p.pu[LUMA_8x8].sad_x4   = PFX(sad_x4_8x8_neon);
        p.pu[LUMA_8x16].sad_x4  = PFX(sad_x4_8x16_neon);
        p.pu[LUMA_8x32].sad_x4  = PFX(sad_x4_8x32_neon);
        p.pu[LUMA_12x16].sad_x4 = PFX(sad_x4_12x16_neon);
        p.pu[LUMA_16x4].sad_x4  = PFX(sad_x4_16x4_neon);
        p.pu[LUMA_16x8].sad_x4  = PFX(sad_x4_16x8_neon);
        p.pu[LUMA_16x12].sad_x4 = PFX(sad_x4_16x12_neon);
        p.pu[LUMA_16x16].sad_x4 = PFX(sad_x4_16x16_neon);
        p.pu[LUMA_16x32].sad_x4 = PFX(sad_x4_16x32_neon);
        p.pu[LUMA_16x64].sad_x4 = PFX(sad_x4_16x64_neon);
        p.pu[LUMA_24x32].sad_x4 = PFX(sad_x4_24x32_neon);
        p.pu[LUMA_32x8].sad_x4  = PFX(sad_x4_32x8_neon);
        p.pu[LUMA_32x16].sad_x4 = PFX(sad_x4_32x16_neon);
        p.pu[LUMA_32x24].sad_x4 = PFX(sad_x4_32x24_neon);
        p.pu[LUMA_32x32].sad_x4 = PFX(sad_x4_32x32_neon);
        p.pu[LUMA_32x64].sad_x4 = PFX(sad_x4_32x64_neon);
        p.pu[LUMA_48x64].sad_x4 = PFX(sad_x4_48x64_neon);
        p.pu[LUMA_64x16].sad_x4 = PFX(sad_x4_64x16_neon);
        p.pu[LUMA_64x32].sad_x4 = PFX(sad_x4_64x32_neon);
        p.pu[LUMA_64x48].sad_x4 = PFX(sad_x4_64x48_neon);
        p.pu[LUMA_64x64].sad_x4 = PFX(sad_x4_64x64_neon);

        // pixel_avg_pp
        p.pu[LUMA_4x4].pixelavg_pp   = PFX(pixel_avg_pp_4x4_neon);
        p.pu[LUMA_4x8].pixelavg_pp   = PFX(pixel_avg_pp_4x8_neon);
        p.pu[LUMA_4x16].pixelavg_pp  = PFX(pixel_avg_pp_4x16_neon);
        p.pu[LUMA_8x4].pixelavg_pp   = PFX(pixel_avg_pp_8x4_neon);
        p.pu[LUMA_8x8].pixelavg_pp   = PFX(pixel_avg_pp_8x8_neon);
        p.pu[LUMA_8x16].pixelavg_pp  = PFX(pixel_avg_pp_8x16_neon);
        p.pu[LUMA_8x32].pixelavg_pp  = PFX(pixel_avg_pp_8x32_neon);
        p.pu[LUMA_12x16].pixelavg_pp = PFX(pixel_avg_pp_12x16_neon);
        p.pu[LUMA_16x4].pixelavg_pp  = PFX(pixel_avg_pp_16x4_neon);
        p.pu[LUMA_16x8].pixelavg_pp  = PFX(pixel_avg_pp_16x8_neon);
        p.pu[LUMA_16x12].pixelavg_pp = PFX(pixel_avg_pp_16x12_neon);
        p.pu[LUMA_16x16].pixelavg_pp = PFX(pixel_avg_pp_16x16_neon);
        p.pu[LUMA_16x32].pixelavg_pp = PFX(pixel_avg_pp_16x32_neon);
        p.pu[LUMA_16x64].pixelavg_pp = PFX(pixel_avg_pp_16x64_neon);
        p.pu[LUMA_24x32].pixelavg_pp = PFX(pixel_avg_pp_24x32_neon);
        p.pu[LUMA_32x8].pixelavg_pp  = PFX(pixel_avg_pp_32x8_neon);
        p.pu[LUMA_32x16].pixelavg_pp = PFX(pixel_avg_pp_32x16_neon);
        p.pu[LUMA_32x24].pixelavg_pp = PFX(pixel_avg_pp_32x24_neon);
        p.pu[LUMA_32x32].pixelavg_pp = PFX(pixel_avg_pp_32x32_neon);
        p.pu[LUMA_32x64].pixelavg_pp = PFX(pixel_avg_pp_32x64_neon);
        p.pu[LUMA_48x64].pixelavg_pp = PFX(pixel_avg_pp_48x64_neon);
        p.pu[LUMA_64x16].pixelavg_pp = PFX(pixel_avg_pp_64x16_neon);
        p.pu[LUMA_64x32].pixelavg_pp = PFX(pixel_avg_pp_64x32_neon);
        p.pu[LUMA_64x48].pixelavg_pp = PFX(pixel_avg_pp_64x48_neon);
        p.pu[LUMA_64x64].pixelavg_pp = PFX(pixel_avg_pp_64x64_neon);
    }
    if (cpuMask & X265_CPU_ARMV6)
    {
        p.pu[LUMA_4x4].sad = PFX(pixel_sad_4x4_armv6);
        p.pu[LUMA_4x8].sad = PFX(pixel_sad_4x8_armv6);
        p.pu[LUMA_4x16].sad=PFX(pixel_sad_4x16_armv6);
    }
}
} // namespace X265_NS
