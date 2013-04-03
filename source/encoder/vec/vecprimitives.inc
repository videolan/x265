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

#define PASTER(name,val) name ## _ ## val
#define EVALUATOR(x,y)   PASTER(x,y)
#define NAME(fun)        EVALUATOR(fun, ARCH)

namespace {
// place vector functions in anonymous namespace (file static)

#if HIGH_BIT_DEPTH /* 16-bit pixels */

int CDECL NAME(sad_8x8)(pixel *piOrg, intptr_t strideOrg, pixel *piCur, intptr_t strideCur)
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

int CDECL NAME(satd_4x4)(pixel *piOrg, intptr_t strideOrg, pixel *piCur, intptr_t strideCur)
{
    int satd = 0;

    /*Assertion istep==1 removed*/

    Vec8s temp1, temp2, temp3, temp4;
    Vec4i v1, v2, v3, v4, m0, m4, m8, m12, diff_v, piOrg_v, piCur_v;
    int satd1, satd2, satd3, satd4;

    temp1.load(piOrg);
    temp2.load(piCur);
    piCur += strideCur;
    piOrg += strideOrg;

    temp3.load(piOrg);
    temp4.load(piCur);

    piOrg_v = extend_low(temp1);
    piCur_v = extend_low(temp2);
    v1 = piOrg_v - piCur_v;

    piOrg_v = extend_low(temp3);
    piCur_v = extend_low(temp4);
    v2 = piOrg_v - piCur_v;

    piCur += strideCur;
    piOrg += strideOrg;

    temp1.load(piOrg);
    temp2.load(piCur);
    piCur += strideCur;
    piOrg += strideOrg;

    temp3.load(piOrg);
    temp4.load(piCur);

    piOrg_v = extend_low(temp1);
    piCur_v = extend_low(temp2);
    v3 = piOrg_v - piCur_v;

    piOrg_v = extend_low(temp3);
    piCur_v = extend_low(temp4);
    v4 = piOrg_v - piCur_v;

    m4 = v2 + v3;
    m8 = v2 - v3;

    m0 = v1 + v4;
    m12 = v1 - v4;

    v1 = m0 + m4;
    v2 = m8 + m12;
    v3 = m0 - m4;
    v4 = m12 - m8;

    Vec4i tv1(v1[0], v1[1], v2[0], v2[1]);
    Vec4i tv2(v1[3], v1[2], v2[3], v2[2]);
    v1 = tv1 + tv2;
    v2 = tv1 - tv2;

    Vec4i tv3(v3[0], v3[1], v4[0], v4[1]);
    Vec4i tv4(v3[3], v3[2], v4[3], v4[2]);
    v3 = tv3 + tv4;
    v4 = tv3 - tv4;

    Vec4i tm1(v1[0], v2[1], v1[2], v2[3]);
    Vec4i tm2(v1[1], v2[0], v1[3], v2[2]);
    v1 = abs(tm1 + tm2);
    v2 = abs(tm1 - tm2);
    satd1 = horizontal_add_x(v1);
    satd2 = horizontal_add_x(v2);

    Vec4i tm3(v3[0], v4[1], v3[2], v4[3]);
    Vec4i tm4(v3[1], v4[0], v3[3], v4[2]);
    v3 = abs(tm3 + tm4);
    v4 = abs(tm3 - tm4);
    satd3 = horizontal_add_x(v3);
    satd4 = horizontal_add_x(v4);

    satd = satd1 + satd2 + satd3 + satd4;

    satd = ((satd + 1) >> 1);

    return satd;
}

#else /* 8-bit pixels */

int CDECL NAME(sad_8x8)(pixel *piOrg, intptr_t strideOrg, pixel *piCur, intptr_t strideCur)
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

#endif

}  // end anonymous namespace

namespace x265 {
// Instantiate an EncoderPrimitives instance for this vector architecture

EncoderPrimitives NAME(primitives_vectorized);

/* Setup() will be called before main().  It should initialize the
 * function table for this vector architecture.
 */
static int Setup()
{
    EncoderPrimitives &p = NAME(primitives_vectorized);

#if HIGH_BIT_DEPTH /* 16-bit pixels */
    p.sad[PARTITION_8x8] = NAME(sad_8x8);
    p.satd[PARTITION_4x4] = NAME(satd_4x4);
#else
    p.sad[PARTITION_8x8] = NAME(sad_8x8);
#endif

    return 1;
}

static int forceinit = Setup();

};
