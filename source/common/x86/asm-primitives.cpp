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

#include "primitives.h"

extern "C" {
#include "pixel.h"
void x265_intel_cpu_indicator_init( void ) {}
}

bool hasXOP(void);

namespace {

/* template for building arbitrary partition sizes from full optimized primitives */
template<int lx, int ly, int dx, int dy, x265::pixelcmp compare>
int CDECL cmp(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
{
    int sum = 0;
    for (int row = 0; row < ly; row += dy)
        for (int col = 0; col < lx; col += dx)
            sum += compare(piOrg + row * strideOrg + col, strideOrg,
                           piCur + row * strideCur + col, strideCur);
    return sum;
}

}

namespace x265 {
// private x265 namespace

#define INIT2_NAME( name1, name2, cpu ) \
    p.name1[PARTITION_16x16] = x265_pixel_##name2##_16x16##cpu;\
    p.name1[PARTITION_16x8]  = x265_pixel_##name2##_16x8##cpu;
#define INIT4_NAME( name1, name2, cpu ) \
    INIT2_NAME( name1, name2, cpu ) \
    p.name1[PARTITION_8x16]  = x265_pixel_##name2##_8x16##cpu;\
    p.name1[PARTITION_8x8]   = x265_pixel_##name2##_8x8##cpu;
#define INIT5_NAME( name1, name2, cpu ) \
    INIT4_NAME( name1, name2, cpu ) \
    p.name1[PARTITION_8x4]   = x265_pixel_##name2##_8x4##cpu;
#define INIT6_NAME( name1, name2, cpu ) \
    INIT5_NAME( name1, name2, cpu ) \
    p.name1[PARTITION_4x8]   = x265_pixel_##name2##_4x8##cpu;
#define INIT7_NAME( name1, name2, cpu ) \
    INIT6_NAME( name1, name2, cpu ) \
    p.name1[PARTITION_4x4]   = x265_pixel_##name2##_4x4##cpu;
#define INIT8_NAME( name1, name2, cpu ) \
    INIT7_NAME( name1, name2, cpu ) \
    p.name1[PARTITION_4x16]  = x265_pixel_##name2##_4x16##cpu;
#define INIT2( name, cpu ) INIT2_NAME( name, name, cpu )
#define INIT4( name, cpu ) INIT4_NAME( name, name, cpu )
#define INIT5( name, cpu ) INIT5_NAME( name, name, cpu )
#define INIT6( name, cpu ) INIT6_NAME( name, name, cpu )
#define INIT7( name, cpu ) INIT7_NAME( name, name, cpu )
#define INIT8( name, cpu ) INIT8_NAME( name, name, cpu )

#if _MSC_VER
#pragma warning(disable: 4100) // unused param, temporary issue until alignment problems are resolved
#endif
void Setup_Assembly_Primitives(EncoderPrimitives &p, int cpuid)
{
#if !HIGH_BIT_DEPTH
    if (cpuid >= 1)
    {
        INIT8( sad, _mmx2 );
        INIT7( sad_x3, _mmx2 );
        INIT7( sad_x4, _mmx2 );
        INIT8( satd, _mmx2 );

        p.satd[PARTITION_4x12] = cmp<4, 12, 4, 4, x265_pixel_satd_4x4_mmx2>;
        p.satd[PARTITION_4x24] = cmp<4, 24, 4, 8, x265_pixel_satd_4x8_mmx2>;
        p.satd[PARTITION_4x32] = cmp<4, 32, 4, 16, x265_pixel_satd_4x16_mmx2>;
        p.satd[PARTITION_4x48] = cmp<4, 48, 4, 16, x265_pixel_satd_4x16_mmx2>;
        p.satd[PARTITION_4x64] = cmp<4, 64, 4, 16, x265_pixel_satd_4x16_mmx2>;

        p.satd[PARTITION_8x12] = cmp<8, 12, 8, 4, x265_pixel_satd_8x4_mmx2>;
        p.satd[PARTITION_8x24] = cmp<8, 24, 8, 8, x265_pixel_satd_8x8_mmx2>;
        p.satd[PARTITION_8x32] = cmp<8, 32, 8, 16, x265_pixel_satd_8x16_mmx2>;
        p.satd[PARTITION_8x48] = cmp<8, 48, 8, 16, x265_pixel_satd_8x16_mmx2>;
        p.satd[PARTITION_8x64] = cmp<8, 64, 8, 16, x265_pixel_satd_8x16_mmx2>;

        p.satd[PARTITION_12x4] = cmp<12, 4, 4, 4, x265_pixel_satd_4x4_mmx2>;
        p.satd[PARTITION_12x8] = cmp<12, 8, 4, 8, x265_pixel_satd_4x8_mmx2>;
        p.satd[PARTITION_12x12] = cmp<12, 12, 4, 4, x265_pixel_satd_4x4_mmx2>;
        p.satd[PARTITION_12x16] = cmp<12, 16, 4, 16, x265_pixel_satd_4x16_mmx2>;
        p.satd[PARTITION_12x24] = cmp<12, 24, 4, 8, x265_pixel_satd_4x8_mmx2>;
        p.satd[PARTITION_12x32] = cmp<12, 32, 4, 16, x265_pixel_satd_4x16_mmx2>;
        p.satd[PARTITION_12x48] = cmp<12, 48, 4, 16, x265_pixel_satd_4x16_mmx2>;
        p.satd[PARTITION_12x64] = cmp<12, 64, 4, 16, x265_pixel_satd_4x16_mmx2>;

        p.satd[PARTITION_16x4] = cmp<16, 4, 8, 4, x265_pixel_satd_8x4_mmx2>;
        p.satd[PARTITION_16x12] = cmp<16, 12, 8, 4, x265_pixel_satd_8x4_mmx2>;
        p.satd[PARTITION_16x24] = cmp<16, 24, 16, 8, x265_pixel_satd_16x8_mmx2>;
        p.satd[PARTITION_16x32] = cmp<16, 32, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_16x48] = cmp<16, 48, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_16x64] = cmp<16, 64, 16, 16, x265_pixel_satd_16x16_mmx2>;

        p.satd[PARTITION_24x4] = cmp<24, 4, 8, 4, x265_pixel_satd_8x4_mmx2>;
        p.satd[PARTITION_24x8] = cmp<24, 8, 8, 8, x265_pixel_satd_8x8_mmx2>;
        p.satd[PARTITION_24x12] = cmp<24, 12, 8, 4, x265_pixel_satd_8x4_mmx2>;
        p.satd[PARTITION_24x16] = cmp<24, 16, 8, 16, x265_pixel_satd_8x16_mmx2>;
        p.satd[PARTITION_24x24] = cmp<24, 24, 8, 8, x265_pixel_satd_8x8_mmx2>;
        p.satd[PARTITION_24x32] = cmp<24, 32, 8, 16, x265_pixel_satd_8x16_mmx2>;
        p.satd[PARTITION_24x48] = cmp<24, 48, 8, 16, x265_pixel_satd_8x16_mmx2>;
        p.satd[PARTITION_24x64] = cmp<24, 64, 8, 16, x265_pixel_satd_8x16_mmx2>;

        p.satd[PARTITION_32x4] = cmp<32, 4, 8, 4, x265_pixel_satd_8x4_mmx2>;
        p.satd[PARTITION_32x8] = cmp<32, 8, 16, 8, x265_pixel_satd_16x8_mmx2>;
        p.satd[PARTITION_32x12] = cmp<32, 12, 8, 4, x265_pixel_satd_8x4_mmx2>;
        p.satd[PARTITION_32x16] = cmp<32, 16, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_32x24] = cmp<32, 24, 16, 8, x265_pixel_satd_16x8_mmx2>;
        p.satd[PARTITION_32x32] = cmp<32, 32, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_32x48] = cmp<32, 48, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_32x64] = cmp<32, 64, 16, 16, x265_pixel_satd_16x16_mmx2>;

        p.satd[PARTITION_48x4] = cmp<48, 4, 8, 4, x265_pixel_satd_8x4_mmx2>;
        p.satd[PARTITION_48x8] = cmp<48, 8, 16, 8, x265_pixel_satd_16x8_mmx2>;
        p.satd[PARTITION_48x12] = cmp<48, 12, 8, 4, x265_pixel_satd_8x4_mmx2>;
        p.satd[PARTITION_48x16] = cmp<48, 16, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_48x24] = cmp<48, 24, 16, 8, x265_pixel_satd_16x8_mmx2>;
        p.satd[PARTITION_48x32] = cmp<48, 32, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_48x48] = cmp<48, 48, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_48x64] = cmp<48, 64, 16, 16, x265_pixel_satd_16x16_mmx2>;

        p.satd[PARTITION_64x4] = cmp<64, 4, 8, 4, x265_pixel_satd_8x4_mmx2>;
        p.satd[PARTITION_64x8] = cmp<64, 8, 16, 8, x265_pixel_satd_16x8_mmx2>;
        p.satd[PARTITION_64x12] = cmp<64, 12, 8, 4, x265_pixel_satd_8x4_mmx2>;
        p.satd[PARTITION_64x16] = cmp<64, 16, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_64x24] = cmp<64, 24, 16, 8, x265_pixel_satd_16x8_mmx2>;
        p.satd[PARTITION_64x32] = cmp<64, 32, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_64x48] = cmp<64, 48, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_64x64] = cmp<64, 64, 16, 16, x265_pixel_satd_16x16_mmx2>;
    }
    if (cpuid >= 2)
    {
        INIT2( sad, _sse2 );
        INIT2( sad_x3, _sse2 );
        INIT2( sad_x4, _sse2 );
        INIT6( satd, _sse2 );
        p.satd[PARTITION_4x16] = x265_pixel_satd_4x16_sse2;

        p.sa8d_8x8 = x265_pixel_sa8d_8x8_sse2;
        p.sa8d_16x16 = x265_pixel_sa8d_16x16_sse2;
        p.sa8d_32x32 = cmp<32, 32, 16, 16, x265_pixel_sa8d_16x16_sse2>;
        p.sa8d_64x64 = cmp<64, 64, 16, 16, x265_pixel_sa8d_16x16_sse2>;

        p.satd[PARTITION_4x32] = cmp<4, 32, 4, 16, x265_pixel_satd_4x16_sse2>;
        p.satd[PARTITION_4x48] = cmp<4, 48, 4, 16, x265_pixel_satd_4x16_sse2>;
        p.satd[PARTITION_4x64] = cmp<4, 64, 4, 16, x265_pixel_satd_4x16_sse2>;

        p.satd[PARTITION_8x12] = cmp<8, 12, 8, 4, x265_pixel_satd_8x4_sse2>;
        p.satd[PARTITION_8x24] = cmp<8, 24, 8, 8, x265_pixel_satd_8x8_sse2>;
        p.satd[PARTITION_8x32] = cmp<8, 32, 8, 16, x265_pixel_satd_8x16_sse2>;
        p.satd[PARTITION_8x48] = cmp<8, 48, 8, 16, x265_pixel_satd_8x16_sse2>;
        p.satd[PARTITION_8x64] = cmp<8, 64, 8, 16, x265_pixel_satd_8x16_sse2>;

        p.satd[PARTITION_12x8] = cmp<12, 8, 4, 8, x265_pixel_satd_4x8_sse2>;
        p.satd[PARTITION_12x16] = cmp<12, 16, 4, 16, x265_pixel_satd_4x16_sse2>;
        p.satd[PARTITION_12x24] = cmp<12, 24, 4, 8, x265_pixel_satd_4x8_sse2>;
        p.satd[PARTITION_12x32] = cmp<12, 32, 4, 16, x265_pixel_satd_4x16_sse2>;
        p.satd[PARTITION_12x48] = cmp<12, 48, 4, 16, x265_pixel_satd_4x16_sse2>;
        p.satd[PARTITION_12x64] = cmp<12, 64, 4, 16, x265_pixel_satd_4x16_sse2>;

        p.satd[PARTITION_16x4] = cmp<16, 4, 8, 4, x265_pixel_satd_8x4_sse2>;
        p.satd[PARTITION_16x12] = cmp<16, 12, 8, 4, x265_pixel_satd_8x4_sse2>;
        p.satd[PARTITION_16x24] = cmp<16, 24, 16, 8, x265_pixel_satd_16x8_sse2>;
        p.satd[PARTITION_16x32] = cmp<16, 32, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_16x48] = cmp<16, 48, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_16x64] = cmp<16, 64, 16, 16, x265_pixel_satd_16x16_sse2>;

        p.satd[PARTITION_24x4] = cmp<24, 4, 8, 4, x265_pixel_satd_8x4_sse2>;
        p.satd[PARTITION_24x8] = cmp<24, 8, 8, 8, x265_pixel_satd_8x8_sse2>;
        p.satd[PARTITION_24x12] = cmp<24, 12, 8, 4, x265_pixel_satd_8x4_sse2>;
        p.satd[PARTITION_24x16] = cmp<24, 16, 8, 16, x265_pixel_satd_8x16_sse2>;
        p.satd[PARTITION_24x24] = cmp<24, 24, 8, 8, x265_pixel_satd_8x8_sse2>;
        p.satd[PARTITION_24x32] = cmp<24, 32, 8, 16, x265_pixel_satd_8x16_sse2>;
        p.satd[PARTITION_24x48] = cmp<24, 48, 8, 16, x265_pixel_satd_8x16_sse2>;
        p.satd[PARTITION_24x64] = cmp<24, 64, 8, 16, x265_pixel_satd_8x16_sse2>;

        p.satd[PARTITION_32x4] = cmp<32, 4, 8, 4, x265_pixel_satd_8x4_sse2>;
        p.satd[PARTITION_32x8] = cmp<32, 8, 16, 8, x265_pixel_satd_16x8_sse2>;
        p.satd[PARTITION_32x12] = cmp<32, 12, 8, 4, x265_pixel_satd_8x4_sse2>;
        p.satd[PARTITION_32x16] = cmp<32, 16, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_32x24] = cmp<32, 24, 16, 8, x265_pixel_satd_16x8_sse2>;
        p.satd[PARTITION_32x32] = cmp<32, 32, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_32x48] = cmp<32, 48, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_32x64] = cmp<32, 64, 16, 16, x265_pixel_satd_16x16_sse2>;

        p.satd[PARTITION_48x4] = cmp<48, 4, 8, 4, x265_pixel_satd_8x4_sse2>;
        p.satd[PARTITION_48x8] = cmp<48, 8, 16, 8, x265_pixel_satd_16x8_sse2>;
        p.satd[PARTITION_48x12] = cmp<48, 12, 8, 4, x265_pixel_satd_8x4_sse2>;
        p.satd[PARTITION_48x16] = cmp<48, 16, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_48x24] = cmp<48, 24, 16, 8, x265_pixel_satd_16x8_sse2>;
        p.satd[PARTITION_48x32] = cmp<48, 32, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_48x48] = cmp<48, 48, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_48x64] = cmp<48, 64, 16, 16, x265_pixel_satd_16x16_sse2>;

        p.satd[PARTITION_64x4] = cmp<64, 4, 8, 4, x265_pixel_satd_8x4_sse2>;
        p.satd[PARTITION_64x8] = cmp<64, 8, 16, 8, x265_pixel_satd_16x8_sse2>;
        p.satd[PARTITION_64x12] = cmp<64, 12, 8, 4, x265_pixel_satd_8x4_sse2>;
        p.satd[PARTITION_64x16] = cmp<64, 16, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_64x24] = cmp<64, 24, 16, 8, x265_pixel_satd_16x8_sse2>;
        p.satd[PARTITION_64x32] = cmp<64, 32, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_64x48] = cmp<64, 48, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_64x64] = cmp<64, 64, 16, 16, x265_pixel_satd_16x16_sse2>;
    }
    if (cpuid >= 3)
    {
        p.sa8d_8x8 = x265_pixel_sa8d_8x8_ssse3;
        p.sa8d_16x16 = x265_pixel_sa8d_16x16_ssse3;
        p.sa8d_32x32 = cmp<32, 32, 16, 16, x265_pixel_sa8d_16x16_ssse3>;
        p.sa8d_64x64 = cmp<64, 64, 16, 16, x265_pixel_sa8d_16x16_ssse3>;
        p.sad_x4[PARTITION_8x4] = x265_pixel_sad_x4_8x4_ssse3;
        p.sad_x4[PARTITION_8x8] = x265_pixel_sad_x4_8x8_ssse3;
        p.sad_x4[PARTITION_8x16] = x265_pixel_sad_x4_8x16_ssse3;
    }
    if (cpuid >= 4)
    {
        p.sa8d_8x8 = x265_pixel_sa8d_8x8_sse4;
        p.sa8d_16x16 = x265_pixel_sa8d_16x16_sse4;
        p.sa8d_32x32 = cmp<32, 32, 16, 16, x265_pixel_sa8d_16x16_sse4>;
        p.sa8d_64x64 = cmp<64, 64, 16, 16, x265_pixel_sa8d_16x16_sse4>;
        p.satd[PARTITION_4x16] = x265_pixel_satd_4x16_sse4;
        p.satd[PARTITION_4x32] = cmp<4, 32, 4, 16, x265_pixel_satd_4x16_sse4>;
        p.satd[PARTITION_4x48] = cmp<4, 48, 4, 16, x265_pixel_satd_4x16_sse4>;
        p.satd[PARTITION_4x64] = cmp<4, 64, 4, 16, x265_pixel_satd_4x16_sse4>;
        p.satd[PARTITION_12x16] = cmp<12, 16, 4, 16, x265_pixel_satd_4x16_sse4>;
        p.satd[PARTITION_12x32] = cmp<12, 32, 4, 16, x265_pixel_satd_4x16_sse4>;
        p.satd[PARTITION_12x48] = cmp<12, 48, 4, 16, x265_pixel_satd_4x16_sse4>;
        p.satd[PARTITION_12x64] = cmp<12, 64, 4, 16, x265_pixel_satd_4x16_sse4>;
    }
    if (cpuid == 7)
    {
        p.sa8d_8x8 = x265_pixel_sa8d_8x8_avx;
        p.sa8d_16x16 = x265_pixel_sa8d_16x16_avx;
        p.sa8d_32x32 = cmp<32, 32, 16, 16, x265_pixel_sa8d_16x16_avx>;
        p.sa8d_64x64 = cmp<64, 64, 16, 16, x265_pixel_sa8d_16x16_avx>;
        p.satd[PARTITION_4x16] = x265_pixel_satd_4x16_avx;
        p.satd[PARTITION_4x32] = cmp<4, 32, 4, 16, x265_pixel_satd_4x16_avx>;
        p.satd[PARTITION_4x48] = cmp<4, 48, 4, 16, x265_pixel_satd_4x16_avx>;
        p.satd[PARTITION_4x64] = cmp<4, 64, 4, 16, x265_pixel_satd_4x16_avx>;
        p.satd[PARTITION_12x16] = cmp<12, 16, 4, 16, x265_pixel_satd_4x16_avx>;
        p.satd[PARTITION_12x32] = cmp<12, 32, 4, 16, x265_pixel_satd_4x16_avx>;
        p.satd[PARTITION_12x48] = cmp<12, 48, 4, 16, x265_pixel_satd_4x16_avx>;
        p.satd[PARTITION_12x64] = cmp<12, 64, 4, 16, x265_pixel_satd_4x16_avx>;
    }
    if (hasXOP())
    {
        p.sa8d_8x8 = x265_pixel_sa8d_8x8_xop;
        p.sa8d_16x16 = x265_pixel_sa8d_16x16_xop;
        p.sa8d_32x32 = cmp<32, 32, 16, 16, x265_pixel_sa8d_16x16_xop>;
        p.sa8d_64x64 = cmp<64, 64, 16, 16, x265_pixel_sa8d_16x16_xop>;

        INIT7( satd, _xop );
        p.satd[PARTITION_4x12] = cmp<4, 12, 4, 4, x265_pixel_satd_4x4_xop>;
        p.satd[PARTITION_4x24] = cmp<4, 24, 4, 8, x265_pixel_satd_4x8_xop>;

        p.satd[PARTITION_8x12] = cmp<8, 12, 8, 4, x265_pixel_satd_8x4_xop>;
        p.satd[PARTITION_8x24] = cmp<8, 24, 8, 8, x265_pixel_satd_8x8_xop>;
        p.satd[PARTITION_8x32] = cmp<8, 32, 8, 16, x265_pixel_satd_8x16_xop>;
        p.satd[PARTITION_8x48] = cmp<8, 48, 8, 16, x265_pixel_satd_8x16_xop>;
        p.satd[PARTITION_8x64] = cmp<8, 64, 8, 16, x265_pixel_satd_8x16_xop>;

        p.satd[PARTITION_12x4] = cmp<12, 4, 4, 4, x265_pixel_satd_4x4_xop>;
        p.satd[PARTITION_12x8] = cmp<12, 8, 4, 8, x265_pixel_satd_4x8_xop>;
        p.satd[PARTITION_12x12] = cmp<12, 12, 4, 4, x265_pixel_satd_4x4_xop>;
        p.satd[PARTITION_12x24] = cmp<12, 24, 4, 8, x265_pixel_satd_4x8_xop>;

        p.satd[PARTITION_16x4] = cmp<16, 4, 8, 4, x265_pixel_satd_8x4_xop>;
        p.satd[PARTITION_16x12] = cmp<16, 12, 8, 4, x265_pixel_satd_8x4_xop>;
        p.satd[PARTITION_16x24] = cmp<16, 24, 16, 8, x265_pixel_satd_16x8_xop>;
        p.satd[PARTITION_16x32] = cmp<16, 32, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_16x48] = cmp<16, 48, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_16x64] = cmp<16, 64, 16, 16, x265_pixel_satd_16x16_xop>;

        p.satd[PARTITION_24x4] = cmp<24, 4, 8, 4, x265_pixel_satd_8x4_xop>;
        p.satd[PARTITION_24x8] = cmp<24, 8, 8, 8, x265_pixel_satd_8x8_xop>;
        p.satd[PARTITION_24x12] = cmp<24, 12, 8, 4, x265_pixel_satd_8x4_xop>;
        p.satd[PARTITION_24x16] = cmp<24, 16, 8, 16, x265_pixel_satd_8x16_xop>;
        p.satd[PARTITION_24x24] = cmp<24, 24, 8, 8, x265_pixel_satd_8x8_xop>;
        p.satd[PARTITION_24x32] = cmp<24, 32, 8, 16, x265_pixel_satd_8x16_xop>;
        p.satd[PARTITION_24x48] = cmp<24, 48, 8, 16, x265_pixel_satd_8x16_xop>;
        p.satd[PARTITION_24x64] = cmp<24, 64, 8, 16, x265_pixel_satd_8x16_xop>;

        p.satd[PARTITION_32x4] = cmp<32, 4, 8, 4, x265_pixel_satd_8x4_xop>;
        p.satd[PARTITION_32x8] = cmp<32, 8, 16, 8, x265_pixel_satd_16x8_xop>;
        p.satd[PARTITION_32x12] = cmp<32, 12, 8, 4, x265_pixel_satd_8x4_xop>;
        p.satd[PARTITION_32x16] = cmp<32, 16, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_32x24] = cmp<32, 24, 16, 8, x265_pixel_satd_16x8_xop>;
        p.satd[PARTITION_32x32] = cmp<32, 32, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_32x48] = cmp<32, 48, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_32x64] = cmp<32, 64, 16, 16, x265_pixel_satd_16x16_xop>;

        p.satd[PARTITION_48x4] = cmp<48, 4, 8, 4, x265_pixel_satd_8x4_xop>;
        p.satd[PARTITION_48x8] = cmp<48, 8, 16, 8, x265_pixel_satd_16x8_xop>;
        p.satd[PARTITION_48x12] = cmp<48, 12, 8, 4, x265_pixel_satd_8x4_xop>;
        p.satd[PARTITION_48x16] = cmp<48, 16, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_48x24] = cmp<48, 24, 16, 8, x265_pixel_satd_16x8_xop>;
        p.satd[PARTITION_48x32] = cmp<48, 32, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_48x48] = cmp<48, 48, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_48x64] = cmp<48, 64, 16, 16, x265_pixel_satd_16x16_xop>;

        p.satd[PARTITION_64x4] = cmp<64, 4, 8, 4, x265_pixel_satd_8x4_xop>;
        p.satd[PARTITION_64x8] = cmp<64, 8, 16, 8, x265_pixel_satd_16x8_xop>;
        p.satd[PARTITION_64x12] = cmp<64, 12, 8, 4, x265_pixel_satd_8x4_xop>;
        p.satd[PARTITION_64x16] = cmp<64, 16, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_64x24] = cmp<64, 24, 16, 8, x265_pixel_satd_16x8_xop>;
        p.satd[PARTITION_64x32] = cmp<64, 32, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_64x48] = cmp<64, 48, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_64x64] = cmp<64, 64, 16, 16, x265_pixel_satd_16x16_xop>;
    }
    if (cpuid >= 8)
    {
        INIT2( sad_x3, _avx2 );
        INIT2( sad_x4, _avx2 );
        INIT4( satd, _avx2 );
        p.sa8d_8x8 = x265_pixel_sa8d_8x8_avx2;
    }
#endif
}

}
