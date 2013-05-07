/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Mandar Gurav <mandar@multicorewareinc.com>
 *          Deepthi Devaki Akkoorath <deepthidevaki@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
 *          Rajesh Paulraj <rajesh@multicorewareinc.com>
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

#include "primitives.h"
#include "Lib/TLibCommon/CommonDef.h"
#include "butterfly.h"
#include <algorithm>

/* Used for filter */
#define IF_INTERNAL_PREC 14 ///< Number of bits for internal precision
#define IF_FILTER_PREC    6 ///< Log2 of sum of filter taps
#define IF_INTERNAL_OFFS (1 << (IF_INTERNAL_PREC - 1)) ///< Offset used internally

#if _MSC_VER
#pragma warning(disable: 4127) // conditional expression is constant, typical for templated functions
#endif

namespace {
// anonymous file-static namespace

void CDECL inversedst(short *tmp, short *block, int shift)  // input tmp, output block
{
    int i, c[4];
    int rnd_factor = 1 << (shift - 1);

    for (i = 0; i < 4; i++)
    {
        // Intermediate Variables
        c[0] = tmp[i] + tmp[8 + i];
        c[1] = tmp[8 + i] + tmp[12 + i];
        c[2] = tmp[i] - tmp[12 + i];
        c[3] = 74 * tmp[4 + i];

        block[4 * i + 0] = (short)Clip3(-32768, 32767, (29 * c[0] + 55 * c[1]     + c[3]               + rnd_factor) >> shift);
        block[4 * i + 1] = (short)Clip3(-32768, 32767, (55 * c[2] - 29 * c[1]     + c[3]               + rnd_factor) >> shift);
        block[4 * i + 2] = (short)Clip3(-32768, 32767, (74 * (tmp[i] - tmp[8 + i]  + tmp[12 + i])      + rnd_factor) >> shift);
        block[4 * i + 3] = (short)Clip3(-32768, 32767, (55 * c[0] + 29 * c[2]     - c[3]               + rnd_factor) >> shift);
    }
}

void CDECL partialButterfly16(short *src, short *dst, int shift, int line)
{
    int j, k;
    int E[8], O[8];
    int EE[4], EO[4];
    int EEE[2], EEO[2];
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* E and O */
        for (k = 0; k < 8; k++)
        {
            E[k] = src[k] + src[15 - k];
            O[k] = src[k] - src[15 - k];
        }

        /* EE and EO */
        for (k = 0; k < 4; k++)
        {
            EE[k] = E[k] + E[7 - k];
            EO[k] = E[k] - E[7 - k];
        }

        /* EEE and EEO */
        EEE[0] = EE[0] + EE[3];
        EEO[0] = EE[0] - EE[3];
        EEE[1] = EE[1] + EE[2];
        EEO[1] = EE[1] - EE[2];

        dst[0] = (short)((g_aiT16[0][0] * EEE[0] + g_aiT16[0][1] * EEE[1] + add) >> shift);
        dst[8 * line] = (short)((g_aiT16[8][0] * EEE[0] + g_aiT16[8][1] * EEE[1] + add) >> shift);
        dst[4 * line] = (short)((g_aiT16[4][0] * EEO[0] + g_aiT16[4][1] * EEO[1] + add) >> shift);
        dst[12 * line] = (short)((g_aiT16[12][0] * EEO[0] + g_aiT16[12][1] * EEO[1] + add) >> shift);

        for (k = 2; k < 16; k += 4)
        {
            dst[k * line] = (short)((g_aiT16[k][0] * EO[0] + g_aiT16[k][1] * EO[1] + g_aiT16[k][2] * EO[2] +
                                     g_aiT16[k][3] * EO[3] + add) >> shift);
        }

        for (k = 1; k < 16; k += 2)
        {
            dst[k * line] =  (short)((g_aiT16[k][0] * O[0] + g_aiT16[k][1] * O[1] + g_aiT16[k][2] * O[2] + g_aiT16[k][3] * O[3] +
                                      g_aiT16[k][4] * O[4] + g_aiT16[k][5] * O[5] + g_aiT16[k][6] * O[6] + g_aiT16[k][7] * O[7] +
                                      add) >> shift);
        }

        src += 16;
        dst++;
    }
}

void CDECL partialButterfly32(short *src, short *dst, int shift, int line)
{
    int j, k;
    int E[16], O[16];
    int EE[8], EO[8];
    int EEE[4], EEO[4];
    int EEEE[2], EEEO[2];
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* E and O*/
        for (k = 0; k < 16; k++)
        {
            E[k] = src[k] + src[31 - k];
            O[k] = src[k] - src[31 - k];
        }

        /* EE and EO */
        for (k = 0; k < 8; k++)
        {
            EE[k] = E[k] + E[15 - k];
            EO[k] = E[k] - E[15 - k];
        }

        /* EEE and EEO */
        for (k = 0; k < 4; k++)
        {
            EEE[k] = EE[k] + EE[7 - k];
            EEO[k] = EE[k] - EE[7 - k];
        }

        /* EEEE and EEEO */
        EEEE[0] = EEE[0] + EEE[3];
        EEEO[0] = EEE[0] - EEE[3];
        EEEE[1] = EEE[1] + EEE[2];
        EEEO[1] = EEE[1] - EEE[2];

        dst[0] = (short)((g_aiT32[0][0] * EEEE[0] + g_aiT32[0][1] * EEEE[1] + add) >> shift);
        dst[16 * line] = (short)((g_aiT32[16][0] * EEEE[0] + g_aiT32[16][1] * EEEE[1] + add) >> shift);
        dst[8 * line] = (short)((g_aiT32[8][0] * EEEO[0] + g_aiT32[8][1] * EEEO[1] + add) >> shift);
        dst[24 * line] = (short)((g_aiT32[24][0] * EEEO[0] + g_aiT32[24][1] * EEEO[1] + add) >> shift);
        for (k = 4; k < 32; k += 8)
        {
            dst[k * line] = (short)((g_aiT32[k][0] * EEO[0] + g_aiT32[k][1] * EEO[1] + g_aiT32[k][2] * EEO[2] +
                                     g_aiT32[k][3] * EEO[3] + add) >> shift);
        }

        for (k = 2; k < 32; k += 4)
        {
            dst[k * line] = (short)((g_aiT32[k][0] * EO[0] + g_aiT32[k][1] * EO[1] + g_aiT32[k][2] * EO[2] +
                                     g_aiT32[k][3] * EO[3] + g_aiT32[k][4] * EO[4] + g_aiT32[k][5] * EO[5] +
                                     g_aiT32[k][6] * EO[6] + g_aiT32[k][7] * EO[7] + add) >> shift);
        }

        for (k = 1; k < 32; k += 2)
        {
            dst[k * line] = (short)((g_aiT32[k][0] * O[0] + g_aiT32[k][1] * O[1] + g_aiT32[k][2] * O[2] + g_aiT32[k][3] * O[3] +
                                     g_aiT32[k][4] * O[4] + g_aiT32[k][5] * O[5] + g_aiT32[k][6] * O[6] + g_aiT32[k][7] * O[7] +
                                     g_aiT32[k][8] * O[8] + g_aiT32[k][9] * O[9] + g_aiT32[k][10] * O[10] + g_aiT32[k][11] *
                                     O[11] + g_aiT32[k][12] * O[12] + g_aiT32[k][13] * O[13] + g_aiT32[k][14] * O[14] +
                                     g_aiT32[k][15] * O[15] + add) >> shift);
        }

        src += 32;
        dst++;
    }
}

void CDECL partialButterfly8(Short *src, Short *dst, Int shift, Int line)
{
    Int j, k;
    Int E[4], O[4];
    Int EE[2], EO[2];
    Int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* E and O*/
        for (k = 0; k < 4; k++)
        {
            E[k] = src[k] + src[7 - k];
            O[k] = src[k] - src[7 - k];
        }

        /* EE and EO */
        EE[0] = E[0] + E[3];
        EO[0] = E[0] - E[3];
        EE[1] = E[1] + E[2];
        EO[1] = E[1] - E[2];

        dst[0] = (short)((g_aiT8[0][0] * EE[0] + g_aiT8[0][1] * EE[1] + add) >> shift);
        dst[4 * line] = (short)((g_aiT8[4][0] * EE[0] + g_aiT8[4][1] * EE[1] + add) >> shift);
        dst[2 * line] = (short)((g_aiT8[2][0] * EO[0] + g_aiT8[2][1] * EO[1] + add) >> shift);
        dst[6 * line] = (short)((g_aiT8[6][0] * EO[0] + g_aiT8[6][1] * EO[1] + add) >> shift);

        dst[line] = (short)((g_aiT8[1][0] * O[0] + g_aiT8[1][1] * O[1] + g_aiT8[1][2] * O[2] + g_aiT8[1][3] * O[3] + add) >> shift);
        dst[3 * line] = (short)((g_aiT8[3][0] * O[0] + g_aiT8[3][1] * O[1] + g_aiT8[3][2] * O[2] + g_aiT8[3][3] * O[3] + add) >> shift);
        dst[5 * line] = (short)((g_aiT8[5][0] * O[0] + g_aiT8[5][1] * O[1] + g_aiT8[5][2] * O[2] + g_aiT8[5][3] * O[3] + add) >> shift);
        dst[7 * line] = (short)((g_aiT8[7][0] * O[0] + g_aiT8[7][1] * O[1] + g_aiT8[7][2] * O[2] + g_aiT8[7][3] * O[3] + add) >> shift);

        src += 8;
        dst++;
    }
}

void CDECL partialButterflyInverse4(Short *src, Short *dst, Int shift, Int line)
{
    Int j;
    Int E[2], O[2];
    Int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
        O[0] = g_aiT4[1][0] * src[line] + g_aiT4[3][0] * src[3 * line];
        O[1] = g_aiT4[1][1] * src[line] + g_aiT4[3][1] * src[3 * line];
        E[0] = g_aiT4[0][0] * src[0] + g_aiT4[2][0] * src[2 * line];
        E[1] = g_aiT4[0][1] * src[0] + g_aiT4[2][1] * src[2 * line];

        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        dst[0] = (short)(Clip3(-32768, 32767, (E[0] + O[0] + add) >> shift));
        dst[1] = (short)(Clip3(-32768, 32767, (E[1] + O[1] + add) >> shift));
        dst[2] = (short)(Clip3(-32768, 32767, (E[1] - O[1] + add) >> shift));
        dst[3] = (short)(Clip3(-32768, 32767, (E[0] - O[0] + add) >> shift));

        src++;
        dst += 4;
    }
}

void CDECL partialButterflyInverse8(Short *src, Short *dst, Int shift, Int line)
{
    Int j, k;
    Int E[4], O[4];
    Int EE[2], EO[2];
    Int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
        for (k = 0; k < 4; k++)
        {
            O[k] = g_aiT8[1][k] * src[line] + g_aiT8[3][k] * src[3 * line] + g_aiT8[5][k] * src[5 * line] + g_aiT8[7][k] * src[7 * line];
        }

        EO[0] = g_aiT8[2][0] * src[2 * line] + g_aiT8[6][0] * src[6 * line];
        EO[1] = g_aiT8[2][1] * src[2 * line] + g_aiT8[6][1] * src[6 * line];
        EE[0] = g_aiT8[0][0] * src[0] + g_aiT8[4][0] * src[4 * line];
        EE[1] = g_aiT8[0][1] * src[0] + g_aiT8[4][1] * src[4 * line];

        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        E[0] = EE[0] + EO[0];
        E[3] = EE[0] - EO[0];
        E[1] = EE[1] + EO[1];
        E[2] = EE[1] - EO[1];
        for (k = 0; k < 4; k++)
        {
            dst[k] = (short)Clip3(-32768, 32767, (E[k] + O[k] + add) >> shift);
            dst[k + 4] = (short)Clip3(-32768, 32767, (E[3 - k] - O[3 - k] + add) >> shift);
        }

        src++;
        dst += 8;
    }
}

void CDECL partialButterflyInverse16(short *src, short *dst, int shift, int line)
{
    Int j, k;
    Int E[8], O[8];
    Int EE[4], EO[4];
    Int EEE[2], EEO[2];
    Int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
        for (k = 0; k < 8; k++)
        {
            O[k] = g_aiT16[1][k] * src[line] + g_aiT16[3][k] * src[3 * line] + g_aiT16[5][k] * src[5 * line] + g_aiT16[7][k] * src[7 * line] +
                g_aiT16[9][k] * src[9 * line] + g_aiT16[11][k] * src[11 * line] + g_aiT16[13][k] * src[13 * line] + g_aiT16[15][k] * src[15 * line];
        }

        for (k = 0; k < 4; k++)
        {
            EO[k] = g_aiT16[2][k] * src[2 * line] + g_aiT16[6][k] * src[6 * line] + g_aiT16[10][k] * src[10 * line] + g_aiT16[14][k] * src[14 * line];
        }

        EEO[0] = g_aiT16[4][0] * src[4 * line] + g_aiT16[12][0] * src[12 * line];
        EEE[0] = g_aiT16[0][0] * src[0] + g_aiT16[8][0] * src[8 * line];
        EEO[1] = g_aiT16[4][1] * src[4 * line] + g_aiT16[12][1] * src[12 * line];
        EEE[1] = g_aiT16[0][1] * src[0] + g_aiT16[8][1] * src[8 * line];

        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        for (k = 0; k < 2; k++)
        {
            EE[k] = EEE[k] + EEO[k];
            EE[k + 2] = EEE[1 - k] - EEO[1 - k];
        }

        for (k = 0; k < 4; k++)
        {
            E[k] = EE[k] + EO[k];
            E[k + 4] = EE[3 - k] - EO[3 - k];
        }

        for (k = 0; k < 8; k++)
        {
            dst[k]   = (short)Clip3(-32768, 32767, (E[k] + O[k] + add) >> shift);
            dst[k + 8] = (short)Clip3(-32768, 32767, (E[7 - k] - O[7 - k] + add) >> shift);
        }

        src++;
        dst += 16;
    }
}

void CDECL partialButterflyInverse32(Short *src, Short *dst, Int shift, Int line)
{
    int j, k;
    int E[16], O[16];
    int EE[8], EO[8];
    int EEE[4], EEO[4];
    int EEEE[2], EEEO[2];
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
        for (k = 0; k < 16; k++)
        {
            O[k] = g_aiT32[1][k] * src[line] + g_aiT32[3][k] * src[3 * line] + g_aiT32[5][k] * src[5 * line] + g_aiT32[7][k] * src[7 * line] +
                g_aiT32[9][k] * src[9 * line] + g_aiT32[11][k] * src[11 * line] + g_aiT32[13][k] * src[13 * line] + g_aiT32[15][k] * src[15 * line] +
                g_aiT32[17][k] * src[17 * line] + g_aiT32[19][k] * src[19 * line] + g_aiT32[21][k] * src[21 * line] + g_aiT32[23][k] * src[23 * line] +
                g_aiT32[25][k] * src[25 * line] + g_aiT32[27][k] * src[27 * line] + g_aiT32[29][k] * src[29 * line] + g_aiT32[31][k] * src[31 * line];
        }

        for (k = 0; k < 8; k++)
        {
            EO[k] = g_aiT32[2][k] * src[2 * line] + g_aiT32[6][k] * src[6 * line] + g_aiT32[10][k] * src[10 * line] + g_aiT32[14][k] * src[14 * line] +
                g_aiT32[18][k] * src[18 * line] + g_aiT32[22][k] * src[22 * line] + g_aiT32[26][k] * src[26 * line] + g_aiT32[30][k] * src[30 * line];
        }

        for (k = 0; k < 4; k++)
        {
            EEO[k] = g_aiT32[4][k] * src[4 * line] + g_aiT32[12][k] * src[12 * line] + g_aiT32[20][k] * src[20 * line] + g_aiT32[28][k] * src[28 * line];
        }

        EEEO[0] = g_aiT32[8][0] * src[8 * line] + g_aiT32[24][0] * src[24 * line];
        EEEO[1] = g_aiT32[8][1] * src[8 * line] + g_aiT32[24][1] * src[24 * line];
        EEEE[0] = g_aiT32[0][0] * src[0] + g_aiT32[16][0] * src[16 * line];
        EEEE[1] = g_aiT32[0][1] * src[0] + g_aiT32[16][1] * src[16 * line];

        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        EEE[0] = EEEE[0] + EEEO[0];
        EEE[3] = EEEE[0] - EEEO[0];
        EEE[1] = EEEE[1] + EEEO[1];
        EEE[2] = EEEE[1] - EEEO[1];
        for (k = 0; k < 4; k++)
        {
            EE[k] = EEE[k] + EEO[k];
            EE[k + 4] = EEE[3 - k] - EEO[3 - k];
        }

        for (k = 0; k < 8; k++)
        {
            E[k] = EE[k] + EO[k];
            E[k + 8] = EE[7 - k] - EO[7 - k];
        }

        for (k = 0; k < 16; k++)
        {
            dst[k] = (short)Clip3(-32768, 32767, (E[k] + O[k] + add) >> shift);
            dst[k + 16] = (short)Clip3(-32768, 32767, (E[15 - k] - O[15 - k] + add) >> shift);
        }

        src++;
        dst += 32;
    }
}
}  // closing - anonymous file-static namespace

namespace x265 {
// x265 private namespace

void Setup_C_MacroblockPrimitives(EncoderPrimitives& p)
{
    p.inversedst = inversedst;

    p.partial_butterfly[BUTTERFLY_16] = partialButterfly16;
    p.partial_butterfly[BUTTERFLY_32] = partialButterfly32;
    p.partial_butterfly[BUTTERFLY_8] = partialButterfly8;
    p.partial_butterfly[BUTTERFLY_INVERSE_4] = partialButterflyInverse4;
    p.partial_butterfly[BUTTERFLY_INVERSE_8] = partialButterflyInverse8;
    p.partial_butterfly[BUTTERFLY_INVERSE_16] = partialButterflyInverse16;
    p.partial_butterfly[BUTTERFLY_INVERSE_32] = partialButterflyInverse32;
}
}
