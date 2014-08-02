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

    pixel *pbuf1, *pbuf2, *pbuf3, *pbuf4, **pixel_test_buff;
    int *ibuf1, **int_test_buff;
    int8_t *psbuf1;
    int16_t *sbuf1, *sbuf2, *sbuf3, **short_test_buff, **short_test_buff1, **short_test_buff2;
    uint16_t **ushort_test_buff;
    uint8_t **uchar_test_buff;
    bool check_pixelcmp(pixelcmp_t ref, pixelcmp_t opt);
    bool check_pixelcmp_sp(pixelcmp_sp_t ref, pixelcmp_sp_t opt);
    bool check_pixelcmp_ss(pixelcmp_ss_t ref, pixelcmp_ss_t opt);
    bool check_pixelcmp_x3(pixelcmp_x3_t ref, pixelcmp_x3_t opt);
    bool check_pixelcmp_x4(pixelcmp_x4_t ref, pixelcmp_x4_t opt);
    bool check_blockcopy_pp(blockcpy_pp_t ref, blockcpy_pp_t opt);
    bool check_blockcopy_ps(blockcpy_ps_t ref, blockcpy_ps_t opt);
    bool check_copy_pp(copy_pp_t ref, copy_pp_t opt);
    bool check_copy_sp(copy_sp_t ref, copy_sp_t opt);
    bool check_copy_ps(copy_ps_t ref, copy_ps_t opt);
    bool check_copy_ss(copy_ss_t ref, copy_ss_t opt);
    bool check_pixelavg_pp(pixelavg_pp_t ref, pixelavg_pp_t opt);
    bool check_pixel_sub_ps(pixel_sub_ps_t ref, pixel_sub_ps_t opt);
    bool check_pixel_add_ps(pixel_add_ps_t ref, pixel_add_ps_t opt);
    bool check_pixeladd_ss(pixeladd_ss_t ref, pixeladd_ss_t opt);
    bool check_scale_pp(scale_t ref, scale_t opt);
    bool check_ssd_s(pixel_ssd_s_t ref, pixel_ssd_s_t opt);
    bool check_blockfill_s(blockfill_s_t ref, blockfill_s_t opt);
    bool check_calresidual(calcresidual_t ref, calcresidual_t opt);
    bool check_calcrecon(calcrecon_t ref, calcrecon_t opt);
    bool check_transpose(transpose_t ref, transpose_t opt);
    bool check_weightp(weightp_pp_t ref, weightp_pp_t opt);
    bool check_weightp(weightp_sp_t ref, weightp_sp_t opt);
    bool check_downscale_t(downscale_t ref, downscale_t opt);
    bool check_cvt32to16_shr_t(cvt32to16_shr_t ref, cvt32to16_shr_t opt);
    bool check_cvt16to32_shl_t(cvt16to32_shl_t ref, cvt16to32_shl_t opt);
    bool check_cvt16to32_cnt_t(cvt16to32_cnt_t ref, cvt16to32_cnt_t opt);
    bool check_pixel_var(var_t ref, var_t opt);
    bool check_ssim_4x4x2_core(ssim_4x4x2_core_t ref, ssim_4x4x2_core_t opt);
    bool check_ssim_end(ssim_end4_t ref, ssim_end4_t opt);
    bool check_addAvg(addAvg_t, addAvg_t);
    bool check_saoCuOrgE0_t(saoCuOrgE0_t ref, saoCuOrgE0_t opt);
    bool check_planecopy_sp(planecopy_sp_t ref, planecopy_sp_t opt);
    bool check_planecopy_cp(planecopy_cp_t ref, planecopy_cp_t opt);

public:

    PixelHarness();

    virtual ~PixelHarness();

    const char *getName() const { return "pixel"; }

    bool testCorrectness(const EncoderPrimitives& ref, const EncoderPrimitives& opt);
    bool testPartition(int part, const EncoderPrimitives& ref, const EncoderPrimitives& opt);

    void measureSpeed(const EncoderPrimitives& ref, const EncoderPrimitives& opt);
    void measurePartition(int part, const EncoderPrimitives& ref, const EncoderPrimitives& opt);
};

#endif // ifndef _PIXELHARNESS_H_1
