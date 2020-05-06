/*****************************************************************************
 * Copyright (C) 2020 MulticoreWare, Inc
 *
 * Authors: Yimeng Su <yimeng.su@huawei.com>
 *          Hongbin Liu <liuhongbin1@huawei.com>
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

#ifndef X265_PIXEL_UTIL_AARCH64_H
#define X265_PIXEL_UTIL_AARCH64_H

int x265_pixel_satd_4x4_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_4x8_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_4x16_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_4x32_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_8x4_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_8x8_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_12x16_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);
int x265_pixel_satd_12x32_neon(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2);

uint32_t x265_quant_neon(const int16_t* coef, const int32_t* quantCoeff, int32_t* deltaU, int16_t* qCoef, int qBits, int add, int numCoeff);
int PFX(psyCost_4x4_neon)(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);

#endif // ifndef X265_PIXEL_UTIL_AARCH64_H
