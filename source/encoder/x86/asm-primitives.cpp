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
void x264_intel_cpu_indicator_init( void ) {}
}

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
    p.name1[PARTITION_16x16] = x264_pixel_##name2##_16x16##cpu;\
    p.name1[PARTITION_16x8]  = x264_pixel_##name2##_16x8##cpu;
#define INIT4_NAME( name1, name2, cpu ) \
    INIT2_NAME( name1, name2, cpu ) \
    p.name1[PARTITION_8x16]  = x264_pixel_##name2##_8x16##cpu;\
    p.name1[PARTITION_8x8]   = x264_pixel_##name2##_8x8##cpu;
#define INIT5_NAME( name1, name2, cpu ) \
    INIT4_NAME( name1, name2, cpu ) \
    p.name1[PARTITION_8x4]   = x264_pixel_##name2##_8x4##cpu;
#define INIT6_NAME( name1, name2, cpu ) \
    INIT5_NAME( name1, name2, cpu ) \
    p.name1[PARTITION_4x8]   = x264_pixel_##name2##_4x8##cpu;
#define INIT7_NAME( name1, name2, cpu ) \
    INIT6_NAME( name1, name2, cpu ) \
    p.name1[PARTITION_4x4]   = x264_pixel_##name2##_4x4##cpu;
#define INIT8_NAME( name1, name2, cpu ) \
    INIT7_NAME( name1, name2, cpu ) \
    p.name1[PARTITION_4x16]  = x264_pixel_##name2##_4x16##cpu;
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
#if 0
    if (cpuid >= 1)
    {
        INIT7( sad, _mmx2 );
        INIT8( satd, _mmx2 );

        // Intra predictions max out at 32x32 (but subpel refine can use larger blocks)
        p.satd[PARTITION_16x4]  = cmp<16, 4, 8, 4, x264_pixel_satd_8x4_mmx2>;
        p.satd[PARTITION_32x8]  = cmp<32, 8, 16, 8, x264_pixel_satd_16x8_mmx2>;
        p.satd[PARTITION_32x32] = cmp<32, 32, 16, 16, x264_pixel_satd_16x16_mmx2>;

        // For large CU motion search
        p.sad[PARTITION_32x32]  = cmp<32, 32, 16, 16, x264_pixel_sad_16x16_mmx2>;
        p.sad[PARTITION_64x32]  = cmp<64, 32, 16, 16, x264_pixel_sad_16x16_mmx2>;
        p.sad[PARTITION_32x64]  = cmp<32, 64, 16, 16, x264_pixel_sad_16x16_mmx2>;
        p.sad[PARTITION_64x64]  = cmp<64, 64, 16, 16, x264_pixel_sad_16x16_mmx2>;
    }
    if (cpuid >= 2)
    {
        p.satd[PARTITION_4x16] = x264_pixel_satd_4x16_sse2;
        p.sa8d_8x8 = x264_pixel_sa8d_8x8_sse2;
        p.sa8d_16x16 = x264_pixel_sa8d_16x16_sse2;
        p.sad[PARTITION_16x16] = x264_pixel_sad_16x16_sse2;
        p.sad[PARTITION_16x8]  = x264_pixel_sad_16x8_sse2;
        p.sad[PARTITION_8x16]  = x264_pixel_sad_8x16_sse2;

        // For large CU motion search
        p.sad[PARTITION_32x32]  = cmp<32, 32, 16, 16, x264_pixel_sad_16x16_sse2>;
        p.sad[PARTITION_64x32]  = cmp<64, 32, 16, 16, x264_pixel_sad_16x16_sse2>;
        p.sad[PARTITION_32x64]  = cmp<32, 64, 16, 16, x264_pixel_sad_16x16_sse2>;
        p.sad[PARTITION_64x64]  = cmp<64, 64, 16, 16, x264_pixel_sad_16x16_sse2>;
    }
    if (cpuid >= 3)
    {
        p.sa8d_8x8 = x264_pixel_sa8d_8x8_ssse3;
        p.sa8d_16x16 = x264_pixel_sa8d_16x16_ssse3;
    }
    if (cpuid >= 4)
    {
        p.satd[PARTITION_4x16] = x264_pixel_satd_4x16_sse4;
        p.sa8d_8x8 = x264_pixel_sa8d_8x8_sse4;
        p.sa8d_16x16 = x264_pixel_sa8d_16x16_sse4;
    }
    if (cpuid == 7)
    {
        p.satd[PARTITION_4x16] = x264_pixel_satd_4x16_avx;
    }
    if (cpuid >= 8)
    {
        // our x264 assembly is too old for AVX2 (not for long)
    }
#endif
}

}
