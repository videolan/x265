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

void x265_intra_pred_dc4_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int filter);
void x265_intra_pred_dc8_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int filter);
void x265_intra_pred_dc16_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int filter);
void x265_intra_pred_dc32_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int filter);

void x265_intra_pred_planar4_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride);
void x265_intra_pred_planar8_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride);
void x265_intra_pred_planar16_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride);
void x265_intra_pred_planar32_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride);

void x265_intra_pred_ang4_2_ssse3(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter);
void x265_intra_pred_ang4_3_ssse3(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter);
void x265_intra_pred_ang4_4_ssse3(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter);
void x265_intra_pred_ang4_5_ssse3(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter);
void x265_intra_pred_ang4_6_ssse3(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter);

#endif // ifndef X265_INTRAPRED_H
