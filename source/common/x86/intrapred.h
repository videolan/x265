/*****************************************************************************
 * intrapred.h: Intra Prediction metrics
 *****************************************************************************
 * Copyright (C) 2003-2013 x264 project
 *
 * Authors: Min Chen <chenm003@163.com> <min.chen@multicorewareinc.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
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

#ifndef X265_INTRAPRED_H
#define X265_INTRAPRED_H

void x265_intra_pred_dc4_sse2(pixel* dst, intptr_t dstStride, const pixel*srcPix, int, int filter);
void x265_intra_pred_dc8_sse2(pixel* dst, intptr_t dstStride, const pixel*srcPix, int, int filter);
void x265_intra_pred_dc16_sse2(pixel* dst, intptr_t dstStride, const pixel*srcPix, int, int filter);
void x265_intra_pred_dc32_sse2(pixel* dst, intptr_t dstStride, const pixel*srcPix, int, int filter);
void x265_intra_pred_dc4_sse4(pixel* dst, intptr_t dstStride, const pixel*srcPix, int, int filter);
void x265_intra_pred_dc8_sse4(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int filter);
void x265_intra_pred_dc16_sse4(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int filter);
void x265_intra_pred_dc32_sse4(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int filter);
void x265_intra_pred_dc32_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int filter);

void x265_intra_pred_planar4_sse2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int);
void x265_intra_pred_planar8_sse2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int);
void x265_intra_pred_planar16_sse2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int);
void x265_intra_pred_planar32_sse2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int);
void x265_intra_pred_planar4_sse4(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int);
void x265_intra_pred_planar8_sse4(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int);
void x265_intra_pred_planar16_sse4(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int);
void x265_intra_pred_planar32_sse4(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int);
void x265_intra_pred_planar16_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int);
void x265_intra_pred_planar32_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int, int);

#define DECL_ANG(bsize, mode, cpu) \
    void x265_intra_pred_ang ## bsize ## _ ## mode ## _ ## cpu(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);

DECL_ANG(4, 2, sse2);
DECL_ANG(4, 3, sse2);
DECL_ANG(4, 4, sse2);
DECL_ANG(4, 5, sse2);
DECL_ANG(4, 6, sse2);
DECL_ANG(4, 7, sse2);
DECL_ANG(4, 8, sse2);
DECL_ANG(4, 9, sse2);
DECL_ANG(4, 10, sse2);
DECL_ANG(4, 11, sse2);
DECL_ANG(4, 12, sse2);
DECL_ANG(4, 13, sse2);
DECL_ANG(4, 14, sse2);
DECL_ANG(4, 15, sse2);
DECL_ANG(4, 16, sse2);
DECL_ANG(4, 17, sse2);
DECL_ANG(4, 18, sse2);
DECL_ANG(4, 26, sse2);

DECL_ANG(4, 2, ssse3);
DECL_ANG(4, 3, sse4);
DECL_ANG(4, 4, sse4);
DECL_ANG(4, 5, sse4);
DECL_ANG(4, 6, sse4);
DECL_ANG(4, 7, sse4);
DECL_ANG(4, 8, sse4);
DECL_ANG(4, 9, sse4);
DECL_ANG(4, 10, sse4);
DECL_ANG(4, 11, sse4);
DECL_ANG(4, 12, sse4);
DECL_ANG(4, 13, sse4);
DECL_ANG(4, 14, sse4);
DECL_ANG(4, 15, sse4);
DECL_ANG(4, 16, sse4);
DECL_ANG(4, 17, sse4);
DECL_ANG(4, 18, sse4);
DECL_ANG(4, 26, sse4);
DECL_ANG(8, 2, ssse3);
DECL_ANG(8, 3, sse4);
DECL_ANG(8, 4, sse4);
DECL_ANG(8, 5, sse4);
DECL_ANG(8, 6, sse4);
DECL_ANG(8, 7, sse4);
DECL_ANG(8, 8, sse4);
DECL_ANG(8, 9, sse4);
DECL_ANG(8, 10, sse4);
DECL_ANG(8, 11, sse4);
DECL_ANG(8, 12, sse4);
DECL_ANG(8, 13, sse4);
DECL_ANG(8, 14, sse4);
DECL_ANG(8, 15, sse4);
DECL_ANG(8, 16, sse4);
DECL_ANG(8, 17, sse4);
DECL_ANG(8, 18, sse4);
DECL_ANG(8, 19, sse4);
DECL_ANG(8, 20, sse4);
DECL_ANG(8, 21, sse4);
DECL_ANG(8, 22, sse4);
DECL_ANG(8, 23, sse4);
DECL_ANG(8, 24, sse4);
DECL_ANG(8, 25, sse4);
DECL_ANG(8, 26, sse4);
DECL_ANG(8, 27, sse4);
DECL_ANG(8, 28, sse4);
DECL_ANG(8, 29, sse4);
DECL_ANG(8, 30, sse4);
DECL_ANG(8, 31, sse4);
DECL_ANG(8, 32, sse4);
DECL_ANG(8, 33, sse4);

DECL_ANG(16, 2, ssse3);
DECL_ANG(16, 3, sse4);
DECL_ANG(16, 4, sse4);
DECL_ANG(16, 5, sse4);
DECL_ANG(16, 6, sse4);
DECL_ANG(16, 7, sse4);
DECL_ANG(16, 8, sse4);
DECL_ANG(16, 9, sse4);
DECL_ANG(16, 10, sse4);
DECL_ANG(16, 11, sse4);
DECL_ANG(16, 12, sse4);
DECL_ANG(16, 13, sse4);
DECL_ANG(16, 14, sse4);
DECL_ANG(16, 15, sse4);
DECL_ANG(16, 16, sse4);
DECL_ANG(16, 17, sse4);
DECL_ANG(16, 18, sse4);
DECL_ANG(16, 19, sse4);
DECL_ANG(16, 20, sse4);
DECL_ANG(16, 21, sse4);
DECL_ANG(16, 22, sse4);
DECL_ANG(16, 23, sse4);
DECL_ANG(16, 24, sse4);
DECL_ANG(16, 25, sse4);
DECL_ANG(16, 26, sse4);
DECL_ANG(16, 27, sse4);
DECL_ANG(16, 28, sse4);
DECL_ANG(16, 29, sse4);
DECL_ANG(16, 30, sse4);
DECL_ANG(16, 31, sse4);
DECL_ANG(16, 32, sse4);
DECL_ANG(16, 33, sse4);

DECL_ANG(32, 2, ssse3);
DECL_ANG(32, 3, sse4);
DECL_ANG(32, 4, sse4);
DECL_ANG(32, 5, sse4);
DECL_ANG(32, 6, sse4);
DECL_ANG(32, 7, sse4);
DECL_ANG(32, 8, sse4);
DECL_ANG(32, 9, sse4);
DECL_ANG(32, 10, sse4);
DECL_ANG(32, 11, sse4);
DECL_ANG(32, 12, sse4);
DECL_ANG(32, 13, sse4);
DECL_ANG(32, 14, sse4);
DECL_ANG(32, 15, sse4);
DECL_ANG(32, 16, sse4);
DECL_ANG(32, 17, sse4);
DECL_ANG(32, 18, sse4);
DECL_ANG(32, 19, sse4);
DECL_ANG(32, 20, sse4);
DECL_ANG(32, 21, sse4);
DECL_ANG(32, 22, sse4);
DECL_ANG(32, 23, sse4);
DECL_ANG(32, 24, sse4);
DECL_ANG(32, 25, sse4);
DECL_ANG(32, 26, sse4);
DECL_ANG(32, 27, sse4);
DECL_ANG(32, 28, sse4);
DECL_ANG(32, 29, sse4);
DECL_ANG(32, 30, sse4);
DECL_ANG(32, 31, sse4);
DECL_ANG(32, 32, sse4);
DECL_ANG(32, 33, sse4);

#undef DECL_ANG
void x265_intra_pred_ang4_3_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_4_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_5_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_6_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_7_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_8_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_9_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_11_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_12_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_13_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_14_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_15_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_16_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_17_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_19_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_20_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_21_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_22_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_23_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_24_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_25_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_27_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_28_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_29_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_30_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_31_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_32_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang4_33_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_3_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_33_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_4_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_32_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_5_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_31_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_6_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_30_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_7_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_29_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_8_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_28_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_9_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_27_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_25_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_12_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_24_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_11_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_13_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_14_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang8_15_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_3_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_4_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_5_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_6_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_7_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_8_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_9_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_12_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_11_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_13_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_25_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_28_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_27_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_29_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_30_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_31_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_32_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_33_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_24_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_23_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang16_22_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_34_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_2_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_26_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_27_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_28_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_29_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_30_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_31_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_32_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_33_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_25_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_24_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_23_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_22_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_intra_pred_ang32_21_avx2(pixel* dst, intptr_t dstStride, const pixel* srcPix, int dirMode, int bFilter);
void x265_all_angs_pred_4x4_sse4(pixel *dest, pixel *refPix, pixel *filtPix, int bLuma);
void x265_all_angs_pred_8x8_sse4(pixel *dest, pixel *refPix, pixel *filtPix, int bLuma);
void x265_all_angs_pred_16x16_sse4(pixel *dest, pixel *refPix, pixel *filtPix, int bLuma);
void x265_all_angs_pred_32x32_sse4(pixel *dest, pixel *refPix, pixel *filtPix, int bLuma);
#endif // ifndef X265_INTRAPRED_H
