/*****************************************************************************
 * intrapred.h: Intra Prediction metrics
 *****************************************************************************
 * Copyright (C) 2003-2013 x264 project
 *
 * Authors: Min Chen <chenm003@163.com> <min.chen@multicorewareinc.com>
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
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

#ifndef X265_INTRAPRED_H
#define X265_INTRAPRED_H

void x265_intra_pred_dc4_sse4(pixel* dst, intptr_t dstStride, pixel* above, pixel* left, int, int filter);
void x265_intra_pred_dc8_sse4(pixel* dst, intptr_t dstStride, pixel* above, pixel* left, int, int filter);
void x265_intra_pred_dc16_sse4(pixel* dst, intptr_t dstStride, pixel* above, pixel* left, int, int filter);
void x265_intra_pred_dc32_sse4(pixel* dst, intptr_t dstStride, pixel* above, pixel* left, int, int filter);

void x265_intra_pred_planar4_sse4(pixel* dst, intptr_t dstStride, pixel* above, pixel* left, int, int filter);
void x265_intra_pred_planar8_sse4(pixel* dst, intptr_t dstStride, pixel* above, pixel* left, int, int filter);
void x265_intra_pred_planar16_sse4(pixel* dst, intptr_t dstStride, pixel* above, pixel* left, int, int filter);
void x265_intra_pred_planar32_sse4(pixel* dst, intptr_t dstStride, pixel* above, pixel* left, int, int filter);

#define DECL_ANG(bsize, mode, cpu) \
    void x265_intra_pred_ang ## bsize ## _ ## mode ## _ ## cpu(pixel * dst, intptr_t dstStride, pixel * refLeft, pixel * refAbove, int dirMode, int bFilter);

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
DECL_ANG(8, 26, sse4);

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
DECL_ANG(16, 26, sse4);

#undef DECL_ANG
void x265_all_angs_pred_4x4_sse4(pixel *dest, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool bLuma);
void x265_all_angs_pred_8x8_sse4(pixel *dest, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool bLuma);

#endif // ifndef X265_INTRAPRED_H
