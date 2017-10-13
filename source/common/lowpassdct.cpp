/*****************************************************************************
 * Copyright (C) 2017 Montez Claros 
 *
 * Authors: Montez Claros <mont3z.claros@gmail.com>
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
//#include "contexts.h"   // costCoeffNxN_c
//#include "threading.h"  // CLZ

using namespace X265_NS;

/* original set of encoder primitives */
static EncoderPrimitives s_rootPrimitives;

static void lowPassDct8_c(const int16_t* src, int16_t* dst, intptr_t srcStride)
{
    ALIGN_VAR_32(int16_t, coef[4 * 4]);
    ALIGN_VAR_32(int16_t, avgBlock[4 * 4]);
    int16_t totalSum = 0;
    int16_t sum = 0;
    
	// Calculate average of 2x2 cells
	for (int i = 0; i < 4; i++)
        for (int j =0; j < 4; j++)
        {
            sum = src[2*i*srcStride + 2*j] + src[2*i*srcStride + 2*j + 1]
                    + src[(2*i+1)*srcStride + 2*j] + src[(2*i+1)*srcStride + 2*j + 1];
            totalSum += sum;
            avgBlock[i*4 + j] = sum >> 2;
        }

	//dct4
	s_rootPrimitives.cu[BLOCK_4x4].dct(avgBlock, coef, 4);
    memset(dst, 0, 64 * sizeof(int16_t));
    for (int i = 0; i < 4; i++)
    {
        memcpy(&dst[i * 8], &coef[i * 4], 4 * sizeof(int16_t));
    }

	// fix first coef with total block average
    dst[0] = totalSum << 1;
}

static void lowPassDct16_c(const int16_t* src, int16_t* dst, intptr_t srcStride)
{
    ALIGN_VAR_32(int16_t, coef[8 * 8]);
    ALIGN_VAR_32(int16_t, avgBlock[8 * 8]);
    int32_t totalSum = 0;
    int16_t sum = 0;
    for (int i = 0; i < 8; i++)
        for (int j =0; j < 8; j++)
        {
            sum = src[2*i*srcStride + 2*j] + src[2*i*srcStride + 2*j + 1]
                    + src[(2*i+1)*srcStride + 2*j] + src[(2*i+1)*srcStride + 2*j + 1];
            totalSum += sum;
            avgBlock[i*8 + j] = sum >> 2;
        }

	// dct8
	s_rootPrimitives.cu[BLOCK_8x8].dct(avgBlock, coef, 8);
    memset(dst, 0, 256 * sizeof(int16_t));
    for (int i = 0; i < 8; i++)
    {
        memcpy(&dst[i * 16], &coef[i * 8], 8 * sizeof(int16_t));
    }
    dst[0] = static_cast<int16_t>(totalSum >> 1);
}

static void lowPassDct32_c(const int16_t* src, int16_t* dst, intptr_t srcStride)
{
    ALIGN_VAR_32(int16_t, coef[16 * 16]);
    ALIGN_VAR_32(int16_t, avgBlock[16 * 16]);
    int32_t totalSum = 0;
    int16_t sum = 0;
    for (int i = 0; i < 16; i++)
        for (int j =0; j < 16; j++)
        {
            sum = src[2*i*srcStride + 2*j] + src[2*i*srcStride + 2*j + 1]
                    + src[(2*i+1)*srcStride + 2*j] + src[(2*i+1)*srcStride + 2*j + 1];
            totalSum += sum;
            avgBlock[i*16 + j] = sum >> 2;
        }

	// dct16
	s_rootPrimitives.cu[BLOCK_16x16].dct(avgBlock, coef, 16);
    memset(dst, 0, 1024 * sizeof(int16_t));
    for (int i = 0; i < 16; i++)
    {
        memcpy(&dst[i * 32], &coef[i * 16], 16 * sizeof(int16_t));
    }
    dst[0] = static_cast<int16_t>(totalSum >> 3);
}

/*
static void dequant_normal_c(const int16_t* quantCoef, int16_t* coef, int num, int scale, int shift)
{
#if HIGH_BIT_DEPTH
    X265_CHECK(scale < 32768 || ((scale & 3) == 0 && shift > (X265_DEPTH - 8)), "dequant invalid scale %d\n", scale);
#else
    // NOTE: maximum of scale is (72 * 256)
    X265_CHECK(scale < 32768, "dequant invalid scale %d\n", scale);
#endif
    X265_CHECK(num <= 32 * 32, "dequant num %d too large\n", num);
    X265_CHECK((num % 8) == 0, "dequant num %d not multiple of 8\n", num);
    X265_CHECK(shift <= 10, "shift too large %d\n", shift);
    X265_CHECK(((intptr_t)coef & 31) == 0, "dequant coef buffer not aligned\n");

    int add, coeffQ;

    add = 1 << (shift - 1);

    for (int n = 0; n < num; n++)
    {
        coeffQ = (quantCoef[n] * scale + add) >> shift;
        coef[n] = (int16_t)x265_clip3(-32768, 32767, coeffQ);
    }
}

static void dequant_scaling_c(const int16_t* quantCoef, const int32_t* deQuantCoef, int16_t* coef, int num, int per, int shift)
{
    X265_CHECK(num <= 32 * 32, "dequant num %d too large\n", num);

    int add, coeffQ;

    shift += 4;

    if (shift > per)
    {
        add = 1 << (shift - per - 1);

        for (int n = 0; n < num; n++)
        {
            coeffQ = ((quantCoef[n] * deQuantCoef[n]) + add) >> (shift - per);
            coef[n] = (int16_t)x265_clip3(-32768, 32767, coeffQ);
        }
    }
    else
    {
        for (int n = 0; n < num; n++)
        {
            coeffQ   = x265_clip3(-32768, 32767, quantCoef[n] * deQuantCoef[n]);
            coef[n] = (int16_t)x265_clip3(-32768, 32767, coeffQ << (per - shift));
        }
    }
}

static uint32_t quant_c(const int16_t* coef, const int32_t* quantCoeff, int32_t* deltaU, int16_t* qCoef, int qBits, int add, int numCoeff)
{
    X265_CHECK(qBits >= 8, "qBits less than 8\n");
    X265_CHECK((numCoeff % 16) == 0, "numCoeff must be multiple of 16\n");
    int qBits8 = qBits - 8;
    uint32_t numSig = 0;

    for (int blockpos = 0; blockpos < numCoeff; blockpos++)
    {
        int level = coef[blockpos];
        int sign  = (level < 0 ? -1 : 1);

        int tmplevel = abs(level) * quantCoeff[blockpos];
        level = ((tmplevel + add) >> qBits);
        deltaU[blockpos] = ((tmplevel - (level << qBits)) >> qBits8);
        if (level)
            ++numSig;
        level *= sign;
        qCoef[blockpos] = (int16_t)x265_clip3(-32768, 32767, level);
    }

    return numSig;
}

static uint32_t nquant_c(const int16_t* coef, const int32_t* quantCoeff, int16_t* qCoef, int qBits, int add, int numCoeff)
{
    X265_CHECK((numCoeff % 16) == 0, "number of quant coeff is not multiple of 4x4\n");
    X265_CHECK((uint32_t)add < ((uint32_t)1 << qBits), "2 ^ qBits less than add\n");
    X265_CHECK(((intptr_t)quantCoeff & 31) == 0, "quantCoeff buffer not aligned\n");

    uint32_t numSig = 0;

    for (int blockpos = 0; blockpos < numCoeff; blockpos++)
    {
        int level = coef[blockpos];
        int sign  = (level < 0 ? -1 : 1);

        int tmplevel = abs(level) * quantCoeff[blockpos];
        level = ((tmplevel + add) >> qBits);
        if (level)
            ++numSig;
        level *= sign;

        // TODO: when we limit range to [-32767, 32767], we can get more performance with output change
        //       But nquant is a little percent in rdoQuant, so I keep old dynamic range for compatible
        qCoef[blockpos] = (int16_t)abs(x265_clip3(-32768, 32767, level));
    }

    return numSig;
}
*/

namespace X265_NS {
// x265 private namespace

void setupLowPassPrimitives(EncoderPrimitives& p)
{
	s_rootPrimitives = p;

    //p.dequant_scaling = dequant_scaling_c;
    //p.dequant_normal = dequant_normal_c;
    //p.quant = quant_c;
    //p.nquant = nquant_c;
    p.cu[BLOCK_8x8].dct   = lowPassDct8_c;
    p.cu[BLOCK_16x16].dct = lowPassDct16_c;
    p.cu[BLOCK_32x32].dct = lowPassDct32_c;
}
}
