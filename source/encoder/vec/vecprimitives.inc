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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

/* This header is included into the per-architecture CPP files.  Each
 * one will define ARCH to a different value. */

#include "primitives.h"

#define PASTER(name, val) name ## _ ## val
#define EVALUATOR(x, y)   PASTER(x, y)
#define NAME(fun)        EVALUATOR(fun, ARCH)

namespace {
// place vector functions in anonymous namespace (file static)

#if HIGH_BIT_DEPTH /* 16-bit pixels */

int CDECL NAME(sad_8x8) (pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    // NOTE: This is completely untested, and ignores alignment issues
    int uiSum = 0;

    for (int Row = 0; Row < 8; Row++)
    {
        Vec8s m1, n1;
        m1.load(piOrg);
        n1.load(piCur);
        m1 = m1 - n1;
        m1 = abs(m1);
        uiSum += horizontal_add_x(m1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    return uiSum;
}

int CDECL NAME(satd_4x4) (pixel * piCur, intptr_t iStrideCur, pixel * piOrg, intptr_t iStrideOrg)
{
    int satd = 0;

    Vec8s v1, v2, m1, m2;

    {
        Vec8s temp1, temp2, temp3, temp4, piOrg_v, piCur_v;
        temp1.load(piOrg);
        temp2.load(piCur);
        piCur += iStrideCur;
        piOrg += iStrideOrg;

        temp3.load(piOrg);
        temp4.load(piCur);
        piCur += iStrideCur;
        piOrg += iStrideOrg;

        piOrg_v = blend2q<0, 2>((Vec2q)temp1, (Vec2q)temp3);
        piCur_v = blend2q<0, 2>((Vec2q)temp2, (Vec2q)temp4);

        temp1.load(piOrg);
        temp2.load(piCur);
        piCur += iStrideCur;
        piOrg += iStrideOrg;

        temp3.load(piOrg);
        temp4.load(piCur);
        piCur += iStrideCur;
        piOrg += iStrideOrg;

        v1 = piOrg_v - piCur_v; //diff

        piOrg_v = blend2q<0, 2>((Vec2q)temp3, (Vec2q)temp1);
        piCur_v = blend2q<0, 2>((Vec2q)temp4, (Vec2q)temp2);
        v2 = piOrg_v - piCur_v; //diff
    }

    for (int i = 0; i < 2; i++)
    {
        m1 = v1 + v2;
        m2 = v1 - v2;

        v1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(m1, m2);
        v2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(m1, m2);
    }

    v2 = permute2q<1, 0>((Vec2q)v2);

    m1 = v1 + v2;
    m2 = v1 - v2;

    v1 = blend8s<0, 8, 1, 9, 2, 10, 3, 11>(m1, m2);
    v2 = blend8s<4, 12, 5, 13, 6, 14, 7, 15>(m1, m2);

    m1 = v1 + v2;
    m2 = v1 - v2;

    v1 = abs(m1);
    v2 = abs(m2);
    v1 = v1 + v2;
    satd = horizontal_add_x(v1);

    satd = ((satd + 1) >> 1);

    return satd;
}

void CDECL NAME(inversedst) (short *tmp, short *block, int shift)  // input tmp, output block
{
  int rnd_factor = 1<<(shift-1);
 
  Vec8s tmp0, tmp1;
  tmp0.load (tmp);
  tmp1.load (tmp + 8);
  
  Vec4i c0 = extend_low (tmp0);
  Vec4i c1 = extend_high (tmp0);
  Vec4i c2 = extend_low (tmp1);
  Vec4i c3 = extend_high (tmp1);

  Vec4i c0_total = c0 + c2;
  Vec4i c1_total = c2 + c3;
  Vec4i c2_total = c0 - c3;
  Vec4i c3_total = 74 * c1;

  Vec4i c4 = (c0 - c2 + c3);

  Vec4i c0_final = ( 29 * c0_total + 55 * c1_total + c3_total + rnd_factor ) >> shift;
  Vec4i c1_final = ( 55 * c2_total - 29 * c1_total + c3_total + rnd_factor ) >> shift;
  Vec4i c2_final = (74 * c4 + rnd_factor) >> shift;
  Vec4i c3_final = (55 * c0_total + 29 * c2_total - c3_total + rnd_factor) >> shift;

  Vec4i first_arg (-32768);
  Vec4i second_arg (32767);

  Vec4i max_number1 = max (first_arg, c0_final);
  Vec4i mid_number1 = min (max_number1, second_arg);
  Vec4i max_number2 = max (first_arg, c1_final);
  Vec4i mid_number2 = min (max_number2, second_arg);
  Vec8s half = compress (mid_number1, mid_number2);

  block[0] = half[0];
  block[4] = half[1];
  block[8] = half[2];
  block[12] = half[3];
  block[1] = half[4];
  block[5] = half[5];
  block[9] = half[6];
  block[13] = half[7];

  max_number1 = max (first_arg, c2_final);
  mid_number1 = min (max_number1, second_arg);
  max_number2 = max (first_arg, c3_final);
  mid_number2 = min (max_number2, second_arg);
  half = compress (mid_number1, mid_number2);

  block[2] = half[0];
  block[6] = half[1];
  block[10] = half[2];
  block[14] = half[3];
  block[3] = half[4];
  block[7] = half[5];
  block[11] = half[6];
  block[15] = half[7];
  
}

#else /* 8-bit pixels */

int CDECL NAME(sad_8x8) (pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    // NOTE: This is completely untested, likely broken, and ignores alignment issues
    int uiSum = 0;

    for (int Row = 0; Row < 8; Row++)
    {
        Vec8s m1, n1;
        m1.load(piOrg);
        n1.load(piCur);
        m1 = m1 - n1;
        m1 = abs(m1);
        uiSum += horizontal_add_x(m1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    return uiSum;
}

#endif /* if HIGH_BIT_DEPTH */
}  // end anonymous namespace

namespace x265 {
// private x265 namespace

/* It should initialize the function table for this vector architecture.  */
void NAME(Setup_Vec_Primitives) (EncoderPrimitives & p)
{
#if HIGH_BIT_DEPTH /* 16-bit pixels */
    p.sad[PARTITION_8x8] = NAME(sad_8x8);
    p.satd[PARTITION_4x4] = NAME(satd_4x4);
    p.inversedst = NAME(inversedst);
#else
    p.sad[PARTITION_8x8] = NAME(sad_8x8);
#endif
}
};
