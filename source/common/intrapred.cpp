/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Min Chen <chenm003@163.com>
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

#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/TComRom.h"
#include "primitives.h"
#include <cstring>
#include <assert.h>

namespace x265 {
unsigned char IntraFilterType[][35] =
{
    //  Index:    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34
    /*  4x4  */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /*  8x8  */ { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
    /* 16x16 */ { 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
    /* 32x32 */ { 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    /* 64x64 */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};
}

using namespace x265;

namespace {
pixel predIntraGetPredValDC(pixel* above, pixel* left, intptr_t width)
{
    int w, sum = 0;
    pixel pDcVal;

    for (w = 0; w < width; w++)
    {
        sum += above[w];
    }

    for (w = 0; w < width; w++)
    {
        sum += left[w];
    }

    pDcVal = (pixel)((sum + width) / (width + width));

    return pDcVal;
}

void DCPredFiltering(pixel* above, pixel* left, pixel* dst, intptr_t dststride, int width, int height)
{
    intptr_t x, y, dstride2;

    // boundary pixels processing
    dst[0] = (pixel)((above[0] + left[0] + 2 * dst[0] + 2) >> 2);

    for (x = 1; x < width; x++)
    {
        dst[x] = (pixel)((above[x] +  3 * dst[x] + 2) >> 2);
    }

    for (y = 1, dstride2 = dststride; y < height; y++, dstride2 += dststride)
    {
        dst[dstride2] = (pixel)((left[y] + 3 * dst[dstride2] + 2) >> 2);
    }
}

void PredIntraDC(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int width, int bFilter)
{
    int k, l;
    int blkSize = width;

    // Do the DC prediction
    pixel dcval = (pixel)predIntraGetPredValDC(above, left, width);

    for (k = 0; k < blkSize; k++)
    {
        for (l = 0; l < blkSize; l++)
        {
            dst[k * dstStride + l] = dcval;
        }
    }

    if (bFilter)
    {
        DCPredFiltering(above, left, dst, dstStride, width, width);
    }
}

void PredIntraPlanar(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int width)
{
    //assert(width == height);

    int k, l;
    pixel bottomLeft, topRight;
    int horPred;
    // OPT_ME: when width is 64, the shift1D is 8, then the dynamic range is 17 bits or [-65280, 65280], so we have to use 32 bits here
    int32_t leftColumn[MAX_CU_SIZE + 1], topRow[MAX_CU_SIZE + 1];
    // CHECK_ME: dynamic range is 9 bits or 15 bits(I assume max input bit_depth is 14 bits)
    int16_t bottomRow[MAX_CU_SIZE], rightColumn[MAX_CU_SIZE];
    int blkSize = width;
    int offset2D = width;
    int shift1D = g_convertToBit[width] + 2;
    int shift2D = shift1D + 1;

    // Get left and above reference column and row
    for (k = 0; k < blkSize + 1; k++)
    {
        topRow[k] = above[k];
        leftColumn[k] = left[k];
    }

    // Prepare intermediate variables used in interpolation
    bottomLeft = (pixel)leftColumn[blkSize];
    topRight   = (pixel)topRow[blkSize];
    for (k = 0; k < blkSize; k++)
    {
        bottomRow[k]   = (int16_t)(bottomLeft - topRow[k]);
        rightColumn[k] = (int16_t)(topRight   - leftColumn[k]);
        topRow[k]      <<= shift1D;
        leftColumn[k]  <<= shift1D;
    }

    // Generate prediction signal
    for (k = 0; k < blkSize; k++)
    {
        horPred = leftColumn[k] + offset2D;
        for (l = 0; l < blkSize; l++)
        {
            horPred += rightColumn[k];
            topRow[l] += bottomRow[l];
            dst[k * dstStride + l] = (pixel)((horPred + topRow[l]) >> shift2D);
        }
    }
}

void PredIntraAngBufRef(pixel* dst, int dstStride, int width, int dirMode, bool bFilter, pixel *refLeft, pixel *refAbove)
{
    int k, l;
    int blkSize  = width;

    // Map the mode index to main prediction direction and angle
    assert(dirMode > 1); //no planar and dc
    bool modeHor       = (dirMode < 18);
    bool modeVer       = !modeHor;
    int intraPredAngle = modeVer ? (int)dirMode - VER_IDX : modeHor ? -((int)dirMode - HOR_IDX) : 0;
    int absAng         = abs(intraPredAngle);
    int signAng        = intraPredAngle < 0 ? -1 : 1;

    // Set bitshifts and scale the angle parameter to block size
    int angTable[9]    = { 0,    2,    5,   9,  13,  17,  21,  26,  32 };
    int invAngTable[9] = { 0, 4096, 1638, 910, 630, 482, 390, 315, 256 }; // (256 * 32) / Angle
    int invAngle       = invAngTable[absAng];
    absAng             = angTable[absAng];
    intraPredAngle     = signAng * absAng;

    // Do angular predictions
    {
        pixel* refMain;
        pixel* refSide;

        // Initialise the Main and Left reference array.
        if (intraPredAngle < 0)
        {
            refMain = (modeVer ? refAbove : refLeft); // + (blkSize - 1);
            refSide = (modeVer ? refLeft : refAbove); // + (blkSize - 1);

            // Extend the Main reference to the left.
            int invAngleSum    = 128; // rounding for (shift by 8)
            for (k = -1; k > blkSize * intraPredAngle >> 5; k--)
            {
                invAngleSum += invAngle;
                refMain[k] = refSide[invAngleSum >> 8];
            }
        }
        else
        {
            refMain = modeVer ? refAbove : refLeft;
            refSide = modeVer ? refLeft  : refAbove;
        }

        if (intraPredAngle == 0)
        {
            for (k = 0; k < blkSize; k++)
            {
                for (l = 0; l < blkSize; l++)
                {
                    dst[k * dstStride + l] = refMain[l + 1];
                }
            }

            if (bFilter)
            {
                for (k = 0; k < blkSize; k++)
                {
                    dst[k * dstStride] = (pixel)Clip3((int16_t)0, (int16_t)((1 << X265_DEPTH) - 1), static_cast<int16_t>((dst[k * dstStride]) + ((refSide[k + 1] - refSide[0]) >> 1)));
                }
            }
        }
        else
        {
            int deltaPos = 0;
            int deltaInt;
            int deltaFract;
            int refMainIndex;

            for (k = 0; k < blkSize; k++)
            {
                deltaPos += intraPredAngle;
                deltaInt   = deltaPos >> 5;
                deltaFract = deltaPos & (32 - 1);

                if (deltaFract)
                {
                    // Do linear filtering
                    for (l = 0; l < blkSize; l++)
                    {
                        refMainIndex        = l + deltaInt + 1;
                        dst[k * dstStride + l] = (pixel)(((32 - deltaFract) * refMain[refMainIndex] + deltaFract * refMain[refMainIndex + 1] + 16) >> 5);
                    }
                }
                else
                {
                    // Just copy the integer samples
                    for (l = 0; l < blkSize; l++)
                    {
                        dst[k * dstStride + l] = refMain[l + deltaInt + 1];
                    }
                }
            }
        }

        // Flip the block if this is the horizontal mode
        if (modeHor)
        {
            pixel  tmp;
            for (k = 0; k < blkSize - 1; k++)
            {
                for (l = k + 1; l < blkSize; l++)
                {
                    tmp                 = dst[k * dstStride + l];
                    dst[k * dstStride + l] = dst[l * dstStride + k];
                    dst[l * dstStride + k] = tmp;
                }
            }
        }
    }
}

template<int size>
void PredIntraAngs_C(pixel *Dst0, pixel *pAbove0, pixel *pLeft0, pixel *pAbove1, pixel *pLeft1, bool bLuma)
{
    int iMode;

    for (iMode = 2; iMode <= 34; iMode++)
    {
        pixel *pLeft = (IntraFilterType[(int)g_convertToBit[size]][iMode] ? pLeft1 : pLeft0);
        pixel *pAbove = (IntraFilterType[(int)g_convertToBit[size]][iMode] ? pAbove1 : pAbove0);
        pixel *dst = Dst0 + (iMode - 2) * (size * size);

        PredIntraAngBufRef(dst, size, size, iMode, bLuma, pLeft, pAbove);

        // Optimize code don't flip buffer
        bool modeHor = (iMode < 18);
        // Flip the block if this is the horizontal mode
        if (modeHor)
        {
            pixel  tmp;
            for (int k = 0; k < size - 1; k++)
            {
                for (int l = k + 1; l < size; l++)
                {
                    tmp                = dst[k * size + l];
                    dst[k * size + l] = dst[l * size + k];
                    dst[l * size + k] = tmp;
                }
            }
        }
    }
}
}

namespace x265 {
// x265 private namespace

void Setup_C_IPredPrimitives(EncoderPrimitives& p)
{
    p.intra_pred_dc = PredIntraDC;
    p.intra_pred_planar = PredIntraPlanar;
    p.intra_pred_ang = PredIntraAngBufRef;
    p.intra_pred_allangs[0] = PredIntraAngs_C<4>;
    p.intra_pred_allangs[1] = PredIntraAngs_C<8>;
    p.intra_pred_allangs[2] = PredIntraAngs_C<16>;
    p.intra_pred_allangs[3] = PredIntraAngs_C<32>;
    p.intra_pred_allangs[4] = PredIntraAngs_C<64>;
}
}
