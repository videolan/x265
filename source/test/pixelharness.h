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

#ifndef _PIXELHARNESS_H_1
#define _PIXELHARNESS_H_1 1

#include "testharness.h"
#include "primitives.h"

class PixelHarness : public TestHarness
{
protected:

    enum { INCR = 32 };
    enum { STRIDE = 64 };
    enum { ITERS = 100 };
    enum { MAX_HEIGHT = 64 };
    enum { PAD_ROWS = 64 };
    enum { BUFFSIZE = STRIDE * (MAX_HEIGHT + PAD_ROWS) + INCR * ITERS };
    enum { TEST_CASES = 3 };
    enum { SMAX = 1 << 12 };
    enum { SMIN = -1 << 12 };

    ALIGN_VAR_32(pixel, pbuf1[BUFFSIZE]);
    pixel    pbuf2[BUFFSIZE];
    pixel    pbuf3[BUFFSIZE];
    pixel    pbuf4[BUFFSIZE];
    int      ibuf1[BUFFSIZE];
    int8_t   psbuf1[BUFFSIZE];
    int8_t   psbuf2[BUFFSIZE];
    int8_t   psbuf3[BUFFSIZE];
    int8_t   psbuf4[BUFFSIZE];
    int8_t   psbuf5[BUFFSIZE];

    int16_t  sbuf1[BUFFSIZE];
    int16_t  sbuf2[BUFFSIZE];
    int16_t  sbuf3[BUFFSIZE];

    pixel    pixel_test_buff[TEST_CASES][BUFFSIZE];
    int16_t  short_test_buff[TEST_CASES][BUFFSIZE];
    int16_t  short_test_buff1[TEST_CASES][BUFFSIZE];
    int16_t  short_test_buff2[TEST_CASES][BUFFSIZE];
    int      int_test_buff[TEST_CASES][BUFFSIZE];
    uint16_t ushort_test_buff[TEST_CASES][BUFFSIZE];
    uint8_t  uchar_test_buff[TEST_CASES][BUFFSIZE];
    double   double_test_buff[TEST_CASES][BUFFSIZE];

    bool check_pixelcmp(pixelcmp_t ref, pixelcmp_t opt);
    bool check_pixel_sse(pixel_sse_t ref, pixel_sse_t opt);
    bool check_pixel_sse_ss(pixel_sse_ss_t ref, pixel_sse_ss_t opt);
    bool check_pixelcmp_x3(pixelcmp_x3_t ref, pixelcmp_x3_t opt);
    bool check_pixelcmp_x4(pixelcmp_x4_t ref, pixelcmp_x4_t opt);
    bool check_copy_pp(copy_pp_t ref, copy_pp_t opt);
    bool check_copy_sp(copy_sp_t ref, copy_sp_t opt);
    bool check_copy_ps(copy_ps_t ref, copy_ps_t opt);
    bool check_copy_ss(copy_ss_t ref, copy_ss_t opt);
    bool check_pixelavg_pp(pixelavg_pp_t ref, pixelavg_pp_t opt);
    bool check_pixel_sub_ps(pixel_sub_ps_t ref, pixel_sub_ps_t opt);
    bool check_pixel_add_ps(pixel_add_ps_t ref, pixel_add_ps_t opt);
    bool check_scale1D_pp(scale1D_t ref, scale1D_t opt);
    bool check_scale2D_pp(scale2D_t ref, scale2D_t opt);
    bool check_ssd_s(pixel_ssd_s_t ref, pixel_ssd_s_t opt);
    bool check_blockfill_s(blockfill_s_t ref, blockfill_s_t opt);
    bool check_calresidual(calcresidual_t ref, calcresidual_t opt);
    bool check_transpose(transpose_t ref, transpose_t opt);
    bool check_weightp(weightp_pp_t ref, weightp_pp_t opt);
    bool check_weightp(weightp_sp_t ref, weightp_sp_t opt);
    bool check_downscale_t(downscale_t ref, downscale_t opt);
    bool check_cpy2Dto1D_shl_t(cpy2Dto1D_shl_t ref, cpy2Dto1D_shl_t opt);
    bool check_cpy2Dto1D_shr_t(cpy2Dto1D_shr_t ref, cpy2Dto1D_shr_t opt);
    bool check_cpy1Dto2D_shl_t(cpy1Dto2D_shl_t ref, cpy1Dto2D_shl_t opt);
    bool check_cpy1Dto2D_shr_t(cpy1Dto2D_shr_t ref, cpy1Dto2D_shr_t opt);
    bool check_copy_cnt_t(copy_cnt_t ref, copy_cnt_t opt);
    bool check_pixel_var(var_t ref, var_t opt);
    bool check_ssim_4x4x2_core(ssim_4x4x2_core_t ref, ssim_4x4x2_core_t opt);
    bool check_ssim_end(ssim_end4_t ref, ssim_end4_t opt);
    bool check_addAvg(addAvg_t, addAvg_t);
    bool check_saoCuOrgE0_t(saoCuOrgE0_t ref, saoCuOrgE0_t opt);
    bool check_saoCuOrgE1_t(saoCuOrgE1_t ref, saoCuOrgE1_t opt);
    bool check_saoCuOrgE2_t(saoCuOrgE2_t ref[], saoCuOrgE2_t opt[]);
    bool check_saoCuOrgE3_t(saoCuOrgE3_t ref, saoCuOrgE3_t opt);
    bool check_saoCuOrgE3_32_t(saoCuOrgE3_t ref, saoCuOrgE3_t opt);
    bool check_saoCuOrgB0_t(saoCuOrgB0_t ref, saoCuOrgB0_t opt);
    bool check_saoCuStatsBO_t(saoCuStatsBO_t ref, saoCuStatsBO_t opt);
    bool check_saoCuStatsE0_t(saoCuStatsE0_t ref, saoCuStatsE0_t opt);
    bool check_saoCuStatsE1_t(saoCuStatsE1_t ref, saoCuStatsE1_t opt);
    bool check_saoCuStatsE2_t(saoCuStatsE2_t ref, saoCuStatsE2_t opt);
    bool check_saoCuStatsE3_t(saoCuStatsE3_t ref, saoCuStatsE3_t opt);
    bool check_planecopy_sp(planecopy_sp_t ref, planecopy_sp_t opt);
    bool check_planecopy_cp(planecopy_cp_t ref, planecopy_cp_t opt);
    bool check_cutree_propagate_cost(cutree_propagate_cost ref, cutree_propagate_cost opt);
    bool check_psyCost_pp(pixelcmp_t ref, pixelcmp_t opt);
    bool check_psyCost_ss(pixelcmp_ss_t ref, pixelcmp_ss_t opt);
    bool check_calSign(sign_t ref, sign_t opt);
    bool check_scanPosLast(scanPosLast_t ref, scanPosLast_t opt);
    bool check_findPosFirstLast(findPosFirstLast_t ref, findPosFirstLast_t opt);
    bool check_costCoeffNxN(costCoeffNxN_t ref, costCoeffNxN_t opt);
    bool check_costCoeffRemain(costCoeffRemain_t ref, costCoeffRemain_t opt);
    bool check_costC1C2Flag(costC1C2Flag_t ref, costC1C2Flag_t opt);
    bool check_planeClipAndMax(planeClipAndMax_t ref, planeClipAndMax_t opt);

public:

    PixelHarness();

    const char *getName() const { return "pixel"; }

    bool testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt);
    bool testPU(int part, const EncoderPrimitives& ref, const EncoderPrimitives& opt);

    void measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt);
    void measurePartition(int part, const EncoderPrimitives& ref, const EncoderPrimitives& opt);
};

#endif // ifndef _PIXELHARNESS_H_1
