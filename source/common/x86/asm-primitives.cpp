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

#define LOWRES(cpu)\
    void x265_frame_init_lowres_core_##cpu( pixel *src0, pixel *dst0, pixel *dsth, pixel *dstv, pixel *dstc,\
    intptr_t src_stride, intptr_t dst_stride, int width, int height );
LOWRES(mmx2)
LOWRES(cache32_mmx2)
LOWRES(sse2)
LOWRES(ssse3)
LOWRES(avx)
LOWRES(xop)

#define DECL_SUF( func, args )\
    void func##_mmx2 args;\
    void func##_sse2 args;\
    void func##_ssse3 args;
DECL_SUF( x265_pixel_avg_16x16, ( pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int ))
DECL_SUF( x265_pixel_avg_16x8,  ( pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int ))
DECL_SUF( x265_pixel_avg_8x16,  ( pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int ))
DECL_SUF( x265_pixel_avg_8x8,   ( pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int ))
DECL_SUF( x265_pixel_avg_8x4,   ( pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int ))
DECL_SUF( x265_pixel_avg_4x16,  ( pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int ))
DECL_SUF( x265_pixel_avg_4x8,   ( pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int ))
DECL_SUF( x265_pixel_avg_4x4,   ( pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int ))

void x265_filterHorizontal_p_p_4_sse4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height, short const *coeff);
}

bool hasXOP(void); // instr_detect.cpp

using namespace x265;

namespace {
// file private anonymous namespace

/* template for building arbitrary partition sizes from full optimized primitives */
template<int lx, int ly, int dx, int dy, pixelcmp_t compare>
int cmp(pixel * piOrg, intptr_t strideOrg, pixel * piCur, intptr_t strideCur)
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

#define ASSGN_SSE(cpu) \
    p.sse_pp[PARTITION_8x8]   = x265_pixel_ssd_8x8_##cpu; \
    p.sse_pp[PARTITION_8x4]   = x265_pixel_ssd_8x4_##cpu; \
    p.sse_pp[PARTITION_16x16] = x265_pixel_ssd_16x16_##cpu; \
    p.sse_pp[PARTITION_16x4]  = x265_pixel_ssd_16x4_##cpu; \
    p.sse_pp[PARTITION_16x8]  = x265_pixel_ssd_16x8_##cpu; \
    p.sse_pp[PARTITION_8x16]  = x265_pixel_ssd_8x16_##cpu; \
    p.sse_pp[PARTITION_16x12] = x265_pixel_ssd_16x12_##cpu; \
    p.sse_pp[PARTITION_32x32] = x265_pixel_ssd_32x32_##cpu; \
    p.sse_pp[PARTITION_32x16] = x265_pixel_ssd_32x16_##cpu; \
    p.sse_pp[PARTITION_16x32] = x265_pixel_ssd_16x32_##cpu; \
    p.sse_pp[PARTITION_8x32]  = x265_pixel_ssd_8x32_##cpu; \
    p.sse_pp[PARTITION_32x8]  = x265_pixel_ssd_32x8_##cpu; \
    p.sse_pp[PARTITION_32x24] = x265_pixel_ssd_32x24_##cpu; \
    p.sse_pp[PARTITION_32x64] = x265_pixel_ssd_32x64_##cpu; \
    p.sse_pp[PARTITION_16x64] = x265_pixel_ssd_16x64_##cpu

#define SA8D_INTER_FROM_BLOCK8(cpu) \
    p.sa8d_inter[PARTITION_16x8]  = cmp<16, 8, 8, 8, x265_pixel_sa8d_8x8_##cpu>; \
    p.sa8d_inter[PARTITION_8x16]  = cmp<8, 16, 8, 8, x265_pixel_sa8d_8x8_##cpu>; \
    p.sa8d_inter[PARTITION_32x24] = cmp<32, 24, 8, 8, x265_pixel_sa8d_8x8_##cpu>; \
    p.sa8d_inter[PARTITION_24x32] = cmp<24, 32, 8, 8, x265_pixel_sa8d_8x8_##cpu>; \
    p.sa8d_inter[PARTITION_32x8]  = cmp<32, 8, 8, 8, x265_pixel_sa8d_8x8_##cpu>; \
    p.sa8d_inter[PARTITION_8x32]  = cmp<8, 32, 8, 8, x265_pixel_sa8d_8x8_##cpu>
#define SA8D_INTER_FROM_BLOCK(cpu) \
    SA8D_INTER_FROM_BLOCK8(cpu); \
    p.sa8d[BLOCK_32x32] = cmp<32, 32, 16, 16, x265_pixel_sa8d_16x16_##cpu>; \
    p.sa8d[BLOCK_64x64] = cmp<64, 64, 16, 16, x265_pixel_sa8d_16x16_##cpu>; \
    p.sa8d_inter[PARTITION_32x32] = cmp<32, 32, 16, 16, x265_pixel_sa8d_16x16_##cpu>; \
    p.sa8d_inter[PARTITION_32x16] = cmp<32, 16, 16, 16, x265_pixel_sa8d_16x16_##cpu>; \
    p.sa8d_inter[PARTITION_16x32] = cmp<16, 32, 16, 16, x265_pixel_sa8d_16x16_##cpu>; \
    p.sa8d_inter[PARTITION_64x64] = cmp<64, 64, 16, 16, x265_pixel_sa8d_16x16_##cpu>; \
    p.sa8d_inter[PARTITION_64x32] = cmp<64, 32, 16, 16, x265_pixel_sa8d_16x16_##cpu>; \
    p.sa8d_inter[PARTITION_32x64] = cmp<32, 64, 16, 16, x265_pixel_sa8d_16x16_##cpu>; \
    p.sa8d_inter[PARTITION_64x48] = cmp<64, 48, 16, 16, x265_pixel_sa8d_16x16_##cpu>; \
    p.sa8d_inter[PARTITION_48x64] = cmp<48, 64, 16, 16, x265_pixel_sa8d_16x16_##cpu>; \
    p.sa8d_inter[PARTITION_64x16] = cmp<64, 16, 16, 16, x265_pixel_sa8d_16x16_##cpu>; \
    p.sa8d_inter[PARTITION_16x64] = cmp<16, 64, 16, 16, x265_pixel_sa8d_16x16_##cpu>

#define PIXEL_AVE(cpu) \
    p.pixelavg_pp[PARTITION_16x16] = x265_pixel_avg_16x16_##cpu; \
    p.pixelavg_pp[PARTITION_16x8]  = x265_pixel_avg_16x8_##cpu; \
    p.pixelavg_pp[PARTITION_8x16]  = x265_pixel_avg_8x16_##cpu; \
    p.pixelavg_pp[PARTITION_8x8]   = x265_pixel_avg_8x8_##cpu; \
    p.pixelavg_pp[PARTITION_8x4]   = x265_pixel_avg_8x4_##cpu;

void Setup_Assembly_Primitives(EncoderPrimitives &p, int cpuMask)
{
#if HIGH_BIT_DEPTH
    if (cpuMask & (1 << X265_CPU_LEVEL_SSE2)) p.sa8d[0] = p.sa8d[0];
#else
    if (cpuMask & (1 << X265_CPU_LEVEL_SSE2))
    {
        INIT8_NAME( sse_pp, ssd, _mmx );
        INIT8( sad, _mmx2 );
        INIT7( sad_x3, _mmx2 );
        INIT7( sad_x4, _mmx2 );
        INIT8( satd, _mmx2 );

        p.frame_init_lowres_core = x265_frame_init_lowres_core_mmx2;
        p.pixelavg_pp[PARTITION_4x16] = x265_pixel_avg_4x16_mmx2;
        p.pixelavg_pp[PARTITION_4x8]  = x265_pixel_avg_4x8_mmx2;
        p.pixelavg_pp[PARTITION_4x4]  = x265_pixel_avg_4x4_mmx2;

        p.sa8d[BLOCK_4x4] = x265_pixel_satd_4x4_mmx2;

        p.satd[PARTITION_12x16] = cmp<12, 16, 4, 16, x265_pixel_satd_4x16_mmx2>;
        p.satd[PARTITION_16x12] = cmp<16, 12, 8, 4, x265_pixel_satd_8x4_mmx2>;

        p.satd[PARTITION_32x32] = cmp<32, 32, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_32x16] = cmp<32, 16, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_16x32] = cmp<16, 32, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_32x24] = cmp<32, 24, 16, 8, x265_pixel_satd_16x8_mmx2>;
        p.satd[PARTITION_24x32] = cmp<24, 32, 8, 16, x265_pixel_satd_8x16_mmx2>;
        p.satd[PARTITION_32x8] = cmp<32, 8, 16, 8, x265_pixel_satd_16x8_mmx2>;
        p.satd[PARTITION_8x32] = cmp<8, 32, 8, 16, x265_pixel_satd_8x16_mmx2>;

        p.satd[PARTITION_64x64] = cmp<64, 64, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_64x32] = cmp<64, 32, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_32x64] = cmp<32, 64, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_64x48] = cmp<64, 48, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_48x64] = cmp<48, 64, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_64x16] = cmp<64, 16, 16, 16, x265_pixel_satd_16x16_mmx2>;
        p.satd[PARTITION_16x64] = cmp<16, 64, 16, 16, x265_pixel_satd_16x16_mmx2>;

        INIT2( sad, _sse2 );
        INIT2( sad_x3, _sse2 );
        INIT2( sad_x4, _sse2 );
        INIT6( satd, _sse2 );
        PIXEL_AVE(sse2);
        ASSGN_SSE(sse2);

        p.frame_init_lowres_core = x265_frame_init_lowres_core_sse2;

        p.sa8d[BLOCK_8x8]   = x265_pixel_sa8d_8x8_sse2;
        p.sa8d[BLOCK_16x16] = x265_pixel_sa8d_16x16_sse2;
        SA8D_INTER_FROM_BLOCK(sse2);

#if X86_64
        p.satd[PARTITION_8x32] = x265_pixel_satd_8x32_sse2;
        p.satd[PARTITION_16x4] = x265_pixel_satd_16x4_sse2;
        p.satd[PARTITION_16x32] = x265_pixel_satd_16x32_sse2;
        p.satd[PARTITION_16x64] = x265_pixel_satd_16x64_sse2;
#else
        p.satd[PARTITION_8x32] = cmp<8, 32, 8, 16, x265_pixel_satd_8x16_sse2>;
        p.satd[PARTITION_16x4] = cmp<16, 4, 8, 4, x265_pixel_satd_8x4_sse2>;
        p.satd[PARTITION_16x32] = cmp<16, 32, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_16x64] = cmp<16, 64, 16, 16, x265_pixel_satd_16x16_sse2>;
#endif
        p.satd[PARTITION_4x16] = x265_pixel_satd_4x16_sse2;
        p.satd[PARTITION_24x32] = cmp<24, 32, 8, 16, x265_pixel_satd_8x16_sse2>;
        p.satd[PARTITION_32x32] = cmp<32, 32, 16, 16, x265_pixel_satd_16x16_sse2>;

        p.satd[PARTITION_64x64] = cmp<64, 64, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_64x16] = cmp<64, 16, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_64x32] = cmp<64, 32, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_64x48] = cmp<64, 48, 16, 16, x265_pixel_satd_16x16_sse2>;
        p.satd[PARTITION_48x64] = cmp<48, 64, 16, 16, x265_pixel_satd_16x16_sse2>;
    }
    if (cpuMask & (1 << X265_CPU_LEVEL_SSSE3))
    {
        p.frame_init_lowres_core = x265_frame_init_lowres_core_ssse3;
        p.sa8d[BLOCK_8x8]   = x265_pixel_sa8d_8x8_ssse3;
        p.sa8d[BLOCK_16x16] = x265_pixel_sa8d_16x16_ssse3;
        SA8D_INTER_FROM_BLOCK(ssse3);
        p.sse_pp[PARTITION_4x4] = x265_pixel_ssd_4x4_ssse3;
        ASSGN_SSE(ssse3);
        PIXEL_AVE(ssse3);

        p.sad_x4[PARTITION_8x4] = x265_pixel_sad_x4_8x4_ssse3;
        p.sad_x4[PARTITION_8x8] = x265_pixel_sad_x4_8x8_ssse3;
        p.sad_x4[PARTITION_8x16] = x265_pixel_sad_x4_8x16_ssse3;
    }
    if (cpuMask & (1 << X265_CPU_LEVEL_SSE41))
    {
        p.satd[PARTITION_4x16] = x265_pixel_satd_4x16_sse4;
        p.sa8d[BLOCK_8x8]   = x265_pixel_sa8d_8x8_sse4;
        p.sa8d[BLOCK_16x16] = x265_pixel_sa8d_16x16_sse4;
        SA8D_INTER_FROM_BLOCK(sse4);

#if !defined(X86_64)
        p.ipfilter_pp[FILTER_H_P_P_4] = x265_filterHorizontal_p_p_4_sse4;
#endif
    }
    if (cpuMask & (1 << X265_CPU_LEVEL_AVX))
    {
        p.frame_init_lowres_core = x265_frame_init_lowres_core_avx;
        p.satd[PARTITION_4x16] = x265_pixel_satd_4x16_avx;
        p.sa8d[BLOCK_8x8]   = x265_pixel_sa8d_8x8_avx;
        p.sa8d[BLOCK_16x16] = x265_pixel_sa8d_16x16_avx;
        SA8D_INTER_FROM_BLOCK(avx);
        ASSGN_SSE(avx);
    }
    if ((cpuMask & (1 << X265_CPU_LEVEL_AVX)) && hasXOP())
    {
        p.frame_init_lowres_core = x265_frame_init_lowres_core_xop;
        p.sa8d[BLOCK_8x8]   = x265_pixel_sa8d_8x8_xop;
        p.sa8d[BLOCK_16x16] = x265_pixel_sa8d_16x16_xop;
        SA8D_INTER_FROM_BLOCK(xop);

        INIT7( satd, _xop );
        INIT5_NAME( sse_pp, ssd, _xop );

#if !X86_64
        // We have x64 SSE2 native versions of these functions, better than wrapper versions
        p.satd[PARTITION_8x32] = cmp<8, 32, 8, 16, x265_pixel_satd_8x16_xop>;
        p.satd[PARTITION_16x4] = cmp<16, 4, 8, 4, x265_pixel_satd_8x4_xop>;
        p.satd[PARTITION_16x32] = cmp<16, 32, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_16x64] = cmp<16, 64, 16, 16, x265_pixel_satd_16x16_xop>;
#endif

        p.satd[PARTITION_32x32] = cmp<32, 32, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_32x16] = cmp<32, 16, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_32x8] = cmp<32, 8, 16, 8, x265_pixel_satd_16x8_xop>;
        p.satd[PARTITION_32x24] = cmp<32, 24, 16, 8, x265_pixel_satd_16x8_xop>;
        p.satd[PARTITION_24x32] = cmp<24, 32, 8, 16, x265_pixel_satd_8x16_xop>;

        p.satd[PARTITION_64x64] = cmp<64, 64, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_64x32] = cmp<64, 32, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_32x64] = cmp<32, 64, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_64x16] = cmp<64, 16, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_64x48] = cmp<64, 48, 16, 16, x265_pixel_satd_16x16_xop>;
        p.satd[PARTITION_48x64] = cmp<48, 64, 16, 16, x265_pixel_satd_16x16_xop>;
    }
    if (cpuMask & (1 << X265_CPU_LEVEL_AVX2))
    {
        INIT2( sad_x4, _avx2 );
        INIT4( satd, _avx2 );
        INIT2_NAME( sse_pp, ssd, _avx2 );
        p.sa8d[BLOCK_8x8] = x265_pixel_sa8d_8x8_avx2;
        SA8D_INTER_FROM_BLOCK8(avx2);
    }
#endif
}
}
