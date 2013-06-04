/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Mandar Gurav <mandar@multicorewareinc.com>
 *          Mahesh Pittala <mahesh@multicorewareinc.com>
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

// Vector class versions of pixel comparison performance primitives

namespace {

/* File for pixels type-neutral code */

void CDECL convert16to32(short *psOrg, int *piDst, int num)
{
    int i;
    for( i=0; i<num; i+=8 )
    {
        Vec8s im16;
        Vec4i im32L, im32H;

        im16.load(psOrg);
        im32L = extend_low (im16);
        im32H = extend_high(im16);
        im32L.store(piDst  );
        im32H.store(piDst+4);

        psOrg += 8;
        piDst += 8;
    }
}

void CDECL convert16to32_shl(int *piDst, short *psOrg, int shift, int num)
{
    int i;
    for( i=0; i<num; i+=8 )
    {
        Vec8s im16;
        Vec4i im32;

        im16 = load_partial(const_int(8), psOrg);
        im32 = extend_low(im16) << shift;
        im32.store(piDst);

        psOrg += 4;
        piDst += 4;
    }
}

void CDECL convert32to16(int *psOrg, short *piDst, int num)
{
    int i;
    for( i=0; i<num; i+=8 )
    {
        Vec4i im32L, im32H;
        Vec8s im16;

        im32L.load(psOrg  );
        im32H.load(psOrg+4);
        im16 = compress_saturated(im32L, im32H);
        im16.store(piDst);

        psOrg += 8;
        piDst += 8;
    }
}

void CDECL convert32to16_shr(short *piDst, int *psOrg, int shift, int num)
{
    int i;
    Vec4i round = _mm_set1_epi32(1 << (shift-1));
    for( i=0; i<num; i+=4 )
    {
        Vec4i im32;
        Vec8s im16;

        im32.load(psOrg);
        im32 = (im32 + round) >> shift;
        im16 = compress_saturated(im32, im32);
        store_partial(const_int(8), piDst, im16);

        psOrg += 4;
        piDst += 4;
    }
}
}

#include "utils.h"

namespace x265 {
// private x265 namespace

void NAME(Setup_Vec_PixelPrimitives)(EncoderPrimitives &p)
{
    // sad
    p.sad[PARTITION_4x4] = sad_4<4>;
    p.sad[PARTITION_4x8] = sad_4<8>;
    p.sad[PARTITION_4x12] = sad_4<12>;
    p.sad[PARTITION_4x16] = sad_4<16>;
    p.sad[PARTITION_4x24] = sad_4<24>;
    p.sad[PARTITION_4x32] = sad_4<32>;
    p.sad[PARTITION_4x48] = sad_4<48>;
    p.sad[PARTITION_4x64] = sad_4<64>;

    p.sad[PARTITION_8x4] = sad_8<4>;
    p.sad[PARTITION_8x8] = sad_8<8>;
    p.sad[PARTITION_8x12] = sad_8<12>;
    p.sad[PARTITION_8x16] = sad_8<16>;
    p.sad[PARTITION_8x24] = sad_8<24>;
    p.sad[PARTITION_8x32] = sad_8<32>;
    p.sad[PARTITION_8x48] = sad_8<48>;
    p.sad[PARTITION_8x64] = sad_8<64>;

    p.sad[PARTITION_12x4] = sad_12<4>;
    p.sad[PARTITION_12x8] = sad_12<8>;
    p.sad[PARTITION_12x12] = sad_12<12>;
    p.sad[PARTITION_12x16] = sad_12<16>;
    p.sad[PARTITION_12x24] = sad_12<24>;
    p.sad[PARTITION_12x32] = sad_12<32>;
    p.sad[PARTITION_12x48] = sad_12<48>;
    p.sad[PARTITION_12x64] = sad_12<64>;

    p.sad[PARTITION_16x4] = sad_16<4>;
    p.sad[PARTITION_16x8] = sad_16<8>;
    p.sad[PARTITION_16x12] = sad_16<12>;
    p.sad[PARTITION_16x16] = sad_16<16>;
    p.sad[PARTITION_16x24] = sad_16<24>;
    p.sad[PARTITION_16x32] = sad_16<32>;
    p.sad[PARTITION_16x48] = sad_16<48>;
    p.sad[PARTITION_16x64] = sad_16<64>;

    p.sad[PARTITION_24x4] = sad_24<4>;
    p.sad[PARTITION_24x8] = sad_24<8>;
    p.sad[PARTITION_24x12] = sad_24<12>;
    p.sad[PARTITION_24x16] = sad_24<16>;
    p.sad[PARTITION_24x24] = sad_24<24>;
    p.sad[PARTITION_24x32] = sad_24<32>;
    p.sad[PARTITION_24x48] = sad_24<48>;
    p.sad[PARTITION_24x64] = sad_24<64>;

    p.sad[PARTITION_32x4] = sad_32<4>;
    p.sad[PARTITION_32x8] = sad_32<8>;
    p.sad[PARTITION_32x12] = sad_32<12>;
    p.sad[PARTITION_32x16] = sad_32<16>;
    p.sad[PARTITION_32x24] = sad_32<24>;
    p.sad[PARTITION_32x32] = sad_32<32>;
    p.sad[PARTITION_32x48] = sad_32<48>;
    p.sad[PARTITION_32x64] = sad_32<64>;

    p.sad[PARTITION_48x4] = sad_48<4>;
    p.sad[PARTITION_48x8] = sad_48<8>;
    p.sad[PARTITION_48x12] = sad_48<12>;
    p.sad[PARTITION_48x16] = sad_48<16>;
    p.sad[PARTITION_48x24] = sad_48<24>;
    p.sad[PARTITION_48x32] = sad_48<32>;
    p.sad[PARTITION_48x48] = sad_48<48>;
    p.sad[PARTITION_48x64] = sad_48<64>;

    p.sad[PARTITION_64x4] = sad_64<4>;
    p.sad[PARTITION_64x8] = sad_64<8>;
    p.sad[PARTITION_64x12] = sad_64<12>;
    p.sad[PARTITION_64x16] = sad_64<16>;
    p.sad[PARTITION_64x24] = sad_64<24>;
    p.sad[PARTITION_64x32] = sad_64<32>;
    p.sad[PARTITION_64x48] = sad_64<48>;
    p.sad[PARTITION_64x64] = sad_64<64>;

    //sad_x3
    p.sad_x3[PARTITION_4x4] = sad_4_x3<4>;
    p.sad_x3[PARTITION_4x8] = sad_4_x3<8>;
    p.sad_x3[PARTITION_4x12] = sad_4_x3<12>;
    p.sad_x3[PARTITION_4x16] = sad_4_x3<16>;
    p.sad_x3[PARTITION_4x24] = sad_4_x3<24>;
    p.sad_x3[PARTITION_4x32] = sad_4_x3<32>;
    p.sad_x3[PARTITION_4x48] = sad_4_x3<48>;
    p.sad_x3[PARTITION_4x64] = sad_4_x3<64>;

    p.sad_x3[PARTITION_8x4] = sad_8_x3<4>;
    p.sad_x3[PARTITION_8x8] = sad_8_x3<8>;
    p.sad_x3[PARTITION_8x12] = sad_8_x3<12>;
    p.sad_x3[PARTITION_8x16] = sad_8_x3<16>;
    p.sad_x3[PARTITION_8x24] = sad_8_x3<24>;
    p.sad_x3[PARTITION_8x32] = sad_8_x3<32>;
    p.sad_x3[PARTITION_8x48] = sad_8_x3<48>;
    p.sad_x3[PARTITION_8x64] = sad_8_x3<64>;

    p.sad_x3[PARTITION_12x4] = sad_12_x3<4>;
    p.sad_x3[PARTITION_12x8] = sad_12_x3<8>;
    p.sad_x3[PARTITION_12x12] = sad_12_x3<12>;
    p.sad_x3[PARTITION_12x16] = sad_12_x3<16>;
    p.sad_x3[PARTITION_12x24] = sad_12_x3<24>;
    p.sad_x3[PARTITION_12x32] = sad_12_x3<32>;
    p.sad_x3[PARTITION_12x48] = sad_12_x3<48>;
    p.sad_x3[PARTITION_12x64] = sad_12_x3<64>;

    p.sad_x3[PARTITION_16x4] = sad_16_x3<4>;
    p.sad_x3[PARTITION_16x8] = sad_16_x3<8>;
    p.sad_x3[PARTITION_16x12] = sad_16_x3<12>;
    p.sad_x3[PARTITION_16x16] = sad_16_x3<16>;
    p.sad_x3[PARTITION_16x24] = sad_16_x3<24>;
    p.sad_x3[PARTITION_16x32] = sad_16_x3<32>;
    p.sad_x3[PARTITION_16x48] = sad_16_x3<48>;
    p.sad_x3[PARTITION_16x64] = sad_16_x3<64>;

    p.sad_x3[PARTITION_24x4] = sad_24_x3<4>;
    p.sad_x3[PARTITION_24x8] = sad_24_x3<8>;
    p.sad_x3[PARTITION_24x12] = sad_24_x3<12>;
    p.sad_x3[PARTITION_24x16] = sad_24_x3<16>;
    p.sad_x3[PARTITION_24x24] = sad_24_x3<24>;
    p.sad_x3[PARTITION_24x32] = sad_24_x3<32>;
    p.sad_x3[PARTITION_24x48] = sad_24_x3<48>;
    p.sad_x3[PARTITION_24x64] = sad_24_x3<64>;

    p.sad_x3[PARTITION_32x4] = sad_32_x3<4>;
    p.sad_x3[PARTITION_32x8] = sad_32_x3<8>;
    p.sad_x3[PARTITION_32x12] = sad_32_x3<12>;
    p.sad_x3[PARTITION_32x16] = sad_32_x3<16>;
    p.sad_x3[PARTITION_32x24] = sad_32_x3<24>;
    p.sad_x3[PARTITION_32x32] = sad_32_x3<32>;
    p.sad_x3[PARTITION_32x48] = sad_32_x3<48>;
    p.sad_x3[PARTITION_32x64] = sad_32_x3<64>;

    p.sad_x3[PARTITION_48x4] = sad_48_x3<4>;
    p.sad_x3[PARTITION_48x8] = sad_48_x3<8>;
    p.sad_x3[PARTITION_48x12] = sad_48_x3<12>;
    p.sad_x3[PARTITION_48x16] = sad_48_x3<16>;
    p.sad_x3[PARTITION_48x24] = sad_48_x3<24>;
    p.sad_x3[PARTITION_48x32] = sad_48_x3<32>;
    p.sad_x3[PARTITION_48x48] = sad_48_x3<48>;
    p.sad_x3[PARTITION_48x64] = sad_48_x3<64>;

    p.sad_x3[PARTITION_64x4] = sad_64_x3<4>;
    p.sad_x3[PARTITION_64x8] = sad_64_x3<8>;
    p.sad_x3[PARTITION_64x12] = sad_64_x3<12>;
    p.sad_x3[PARTITION_64x16] = sad_64_x3<16>;
    p.sad_x3[PARTITION_64x24] = sad_64_x3<24>;
    p.sad_x3[PARTITION_64x32] = sad_64_x3<32>;
    p.sad_x3[PARTITION_64x48] = sad_64_x3<48>;
    p.sad_x3[PARTITION_64x64] = sad_64_x3<64>;

    //// sad_x4
    p.sad_x4[PARTITION_4x4] = sad_4_x4<4>;
    p.sad_x4[PARTITION_4x8] = sad_4_x4<8>;
    p.sad_x4[PARTITION_4x12] = sad_4_x4<12>;
    p.sad_x4[PARTITION_4x16] = sad_4_x4<16>;
    p.sad_x4[PARTITION_4x24] = sad_4_x4<24>;
    p.sad_x4[PARTITION_4x32] = sad_4_x4<32>;
    p.sad_x4[PARTITION_4x48] = sad_4_x4<48>;
    p.sad_x4[PARTITION_4x64] = sad_4_x4<64>;

    p.sad_x4[PARTITION_8x4] = sad_8_x4<4>;
    p.sad_x4[PARTITION_8x8] = sad_8_x4<8>;
    p.sad_x4[PARTITION_8x12] = sad_8_x4<12>;
    p.sad_x4[PARTITION_8x16] = sad_8_x4<16>;
    p.sad_x4[PARTITION_8x24] = sad_8_x4<24>;
    p.sad_x4[PARTITION_8x32] = sad_8_x4<32>;
    p.sad_x4[PARTITION_8x48] = sad_8_x4<48>;
    p.sad_x4[PARTITION_8x64] = sad_8_x4<64>;

    p.sad_x4[PARTITION_12x4] = sad_12_x4<4>;
    p.sad_x4[PARTITION_12x8] = sad_12_x4<8>;
    p.sad_x4[PARTITION_12x12] = sad_12_x4<12>;
    p.sad_x4[PARTITION_12x16] = sad_12_x4<16>;
    p.sad_x4[PARTITION_12x24] = sad_12_x4<24>;
    p.sad_x4[PARTITION_12x32] = sad_12_x4<32>;
    p.sad_x4[PARTITION_12x48] = sad_12_x4<48>;
    p.sad_x4[PARTITION_12x64] = sad_12_x4<64>;

    p.sad_x4[PARTITION_16x4] = sad_16_x4<4>;
    p.sad_x4[PARTITION_16x8] = sad_16_x4<8>;
    p.sad_x4[PARTITION_16x12] = sad_16_x4<12>;
    p.sad_x4[PARTITION_16x16] = sad_16_x4<16>;
    p.sad_x4[PARTITION_16x24] = sad_16_x4<24>;
    p.sad_x4[PARTITION_16x32] = sad_16_x4<32>;
    p.sad_x4[PARTITION_16x48] = sad_16_x4<48>;
    p.sad_x4[PARTITION_16x64] = sad_16_x4<64>;

    p.sad_x4[PARTITION_24x4] = sad_24_x4<4>;
    p.sad_x4[PARTITION_24x8] = sad_24_x4<8>;
    p.sad_x4[PARTITION_24x12] = sad_24_x4<12>;
    p.sad_x4[PARTITION_24x16] = sad_24_x4<16>;
    p.sad_x4[PARTITION_24x24] = sad_24_x4<24>;
    p.sad_x4[PARTITION_24x32] = sad_24_x4<32>;
    p.sad_x4[PARTITION_24x48] = sad_24_x4<48>;
    p.sad_x4[PARTITION_24x64] = sad_24_x4<64>;

    p.sad_x4[PARTITION_32x4] = sad_32_x4<4>;
    p.sad_x4[PARTITION_32x8] = sad_32_x4<8>;
    p.sad_x4[PARTITION_32x12] = sad_32_x4<12>;
    p.sad_x4[PARTITION_32x16] = sad_32_x4<16>;
    p.sad_x4[PARTITION_32x24] = sad_32_x4<24>;
    p.sad_x4[PARTITION_32x32] = sad_32_x4<32>;
    p.sad_x4[PARTITION_32x48] = sad_32_x4<48>;
    p.sad_x4[PARTITION_32x64] = sad_32_x4<64>;

    p.sad_x4[PARTITION_48x4] = sad_48_x4<4>;
    p.sad_x4[PARTITION_48x8] = sad_48_x4<8>;
    p.sad_x4[PARTITION_48x12] = sad_48_x4<12>;
    p.sad_x4[PARTITION_48x16] = sad_48_x4<16>;
    p.sad_x4[PARTITION_48x24] = sad_48_x4<24>;
    p.sad_x4[PARTITION_48x32] = sad_48_x4<32>;
    p.sad_x4[PARTITION_48x48] = sad_48_x4<48>;
    p.sad_x4[PARTITION_48x64] = sad_48_x4<64>;

    p.sad_x4[PARTITION_64x4] = sad_64_x4<4>;
    p.sad_x4[PARTITION_64x8] = sad_64_x4<8>;
    p.sad_x4[PARTITION_64x12] = sad_64_x4<12>;
    p.sad_x4[PARTITION_64x16] = sad_64_x4<16>;
    p.sad_x4[PARTITION_64x24] = sad_64_x4<24>;
    p.sad_x4[PARTITION_64x32] = sad_64_x4<32>;
    p.sad_x4[PARTITION_64x48] = sad_64_x4<48>;
    p.sad_x4[PARTITION_64x64] = sad_64_x4<64>;

    //sse
    p.sse[PARTITION_4x4] = sse_4<4>;
    p.sse[PARTITION_4x8] = sse_4<8>;
    p.sse[PARTITION_4x12] = sse_4<12>;
    p.sse[PARTITION_4x16] = sse_4<16>;
    p.sse[PARTITION_4x24] = sse_4<24>;
    p.sse[PARTITION_4x32] = sse_4<32>;
    p.sse[PARTITION_4x48] = sse_4<48>;
    p.sse[PARTITION_4x64] = sse_4<64>;

    p.sse[PARTITION_8x4] = sse_8<4>;
    p.sse[PARTITION_8x8] = sse_8<8>;
    p.sse[PARTITION_8x12] = sse_8<12>;
    p.sse[PARTITION_8x16] = sse_8<16>;
    p.sse[PARTITION_8x24] = sse_8<24>;
    p.sse[PARTITION_8x32] = sse_8<32>;
    p.sse[PARTITION_8x48] = sse_8<48>;
    p.sse[PARTITION_8x64] = sse_8<64>;

    /*p.sse[PARTITION_12x4] = sse_12<4>;
    p.sse[PARTITION_12x8] = sse_12<8>;
    p.sse[PARTITION_12x12] = sse_12<12>;
    p.sse[PARTITION_12x16] = sse_12<16>;
    p.sse[PARTITION_12x24] = sse_12<24>;
    p.sse[PARTITION_12x32] = sse_12<32>;
    p.sse[PARTITION_12x48] = sse_12<48>;
    p.sse[PARTITION_12x64] = sse_12<64>;*/

    p.sse[PARTITION_16x4] = sse_16<4>;
    p.sse[PARTITION_16x8] = sse_16<8>;
    p.sse[PARTITION_16x12] = sse_16<12>;
    p.sse[PARTITION_16x16] = sse_16<16>;
    p.sse[PARTITION_16x24] = sse_16<24>;
    p.sse[PARTITION_16x32] = sse_16<32>;
    p.sse[PARTITION_16x48] = sse_16<48>;
    p.sse[PARTITION_16x64] = sse_16<64>;

    /*p.sse[PARTITION_24x4] = sse_24<4>;
    p.sse[PARTITION_24x8] = sse_24<8>;
    p.sse[PARTITION_24x12] = sse_24<12>;
    p.sse[PARTITION_24x16] = sse_24<16>;
    p.sse[PARTITION_24x24] = sse_24<24>;
    p.sse[PARTITION_24x32] = sse_24<32>;
    p.sse[PARTITION_24x48] = sse_24<48>;
    p.sse[PARTITION_24x64] = sse_24<64>;*/

    p.sse[PARTITION_32x4] = sse_32<4>;
    p.sse[PARTITION_32x8] = sse_32<8>;
    p.sse[PARTITION_32x12] = sse_32<12>;
    p.sse[PARTITION_32x16] = sse_32<16>;
    p.sse[PARTITION_32x24] = sse_32<24>;
    p.sse[PARTITION_32x32] = sse_32<32>;
    p.sse[PARTITION_32x48] = sse_32<48>;
    p.sse[PARTITION_32x64] = sse_32<64>;

    p.sse[PARTITION_48x4] = sse_48<4>;
    p.sse[PARTITION_48x8] = sse_48<8>;
    p.sse[PARTITION_48x12] = sse_48<12>;
    p.sse[PARTITION_48x16] = sse_48<16>;
    p.sse[PARTITION_48x24] = sse_48<24>;
    p.sse[PARTITION_48x32] = sse_48<32>;
    p.sse[PARTITION_48x48] = sse_48<48>;
    p.sse[PARTITION_48x64] = sse_48<64>;

    p.sse[PARTITION_64x4] = sse_64<4>;
    p.sse[PARTITION_64x8] = sse_64<8>;
    p.sse[PARTITION_64x12] = sse_64<12>;
    p.sse[PARTITION_64x16] = sse_64<16>;
    p.sse[PARTITION_64x24] = sse_64<24>;
    p.sse[PARTITION_64x32] = sse_64<32>;
    p.sse[PARTITION_64x48] = sse_64<48>;
    p.sse[PARTITION_64x64] = sse_64<64>;

#if HIGH_BIT_DEPTH
    // satd
    p.satd[PARTITION_4x4] = satd_4x4;
    p.satd[PARTITION_4x8] = satd<4, 8>;
    p.satd[PARTITION_16x4] = satd<16, 4>;
    //p.sa8d_8x8 = sa8d_8x8;
#endif
    p.cvt16to32     = convert16to32;
    p.cvt16to32_shl = convert16to32_shl;
    p.cvt32to16     = convert32to16;
    p.cvt32to16_shr = convert32to16_shr;
}
}