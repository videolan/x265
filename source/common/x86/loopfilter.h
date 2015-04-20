/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Dnyaneshwar Gorade <dnyaneshwar@multicorewareinc.com>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
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

#ifndef X265_LOOPFILTER_H
#define X265_LOOPFILTER_H

void x265_saoCuOrgE0_sse4(pixel * rec, int8_t * offsetEo, int endX, int8_t* signLeft, intptr_t stride);
void x265_saoCuOrgE0_avx2(pixel * rec, int8_t * offsetEo, int endX, int8_t* signLeft, intptr_t stride);
void x265_saoCuOrgE1_sse4(pixel* rec, int8_t* upBuff1, int8_t* offsetEo, intptr_t stride, int width);
void x265_saoCuOrgE1_avx2(pixel* rec, int8_t* upBuff1, int8_t* offsetEo, intptr_t stride, int width);
void x265_saoCuOrgE1_2Rows_sse4(pixel* rec, int8_t* upBuff1, int8_t* offsetEo, intptr_t stride, int width);
void x265_saoCuOrgE1_2Rows_avx2(pixel* rec, int8_t* upBuff1, int8_t* offsetEo, intptr_t stride, int width);
void x265_saoCuOrgE2_sse4(pixel* rec, int8_t* pBufft, int8_t* pBuff1, int8_t* offsetEo, int lcuWidth, intptr_t stride);
void x265_saoCuOrgE3_sse4(pixel *rec, int8_t *upBuff1, int8_t *m_offsetEo, intptr_t stride, int startX, int endX);
void x265_saoCuOrgE3_avx2(pixel *rec, int8_t *upBuff1, int8_t *m_offsetEo, intptr_t stride, int startX, int endX);
void x265_saoCuOrgE3_2Rows_sse4(pixel *rec, int8_t *upBuff1, int8_t *m_offsetEo, intptr_t stride, int startX, int endX, int8_t* upBuff);
void x265_saoCuOrgB0_sse4(pixel* rec, const int8_t* offsetBo, int ctuWidth, int ctuHeight, intptr_t stride);
void x265_saoCuOrgB0_avx2(pixel* rec, const int8_t* offsetBo, int ctuWidth, int ctuHeight, intptr_t stride);
void x265_calSign_sse4(int8_t *dst, const pixel *src1, const pixel *src2, const int endX);

#endif // ifndef X265_LOOPFILTER_H
