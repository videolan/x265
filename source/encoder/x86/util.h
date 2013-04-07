/*****************************************************************************
 * util.h: x86 inline asm
 *****************************************************************************
 * Copyright (C) 2008-2013 x264 project
 *
 * Authors: Jason Garrett-Glaser <darkshikari@gmail.com>
 *          Loren Merritt <lorenm@u.washington.edu>
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
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

#ifndef X264_X86_UTIL_H
#define X264_X86_UTIL_H

#ifdef __SSE__
#include <xmmintrin.h>

#undef M128_ZERO
#define M128_ZERO ((__m128){0,0,0,0})
#define x264_union128_t x264_union128_sse_t
typedef union { __m128 i; uint64_t a[2]; uint32_t b[4]; uint16_t c[8]; uint8_t d[16]; } MAY_ALIAS x264_union128_sse_t;
#if HAVE_VECTOREXT
typedef uint32_t v4si __attribute__((vector_size (16)));
#endif
#endif // __SSE__

#if HAVE_X86_INLINE_ASM && HAVE_MMX

#define x264_median_mv x264_median_mv_mmx2
static ALWAYS_INLINE void x264_median_mv_mmx2( int16_t *dst, int16_t *a, int16_t *b, int16_t *c )
{
    asm(
        "movd   %1,    %%mm0 \n"
        "movd   %2,    %%mm1 \n"
        "movq   %%mm0, %%mm3 \n"
        "movd   %3,    %%mm2 \n"
        "pmaxsw %%mm1, %%mm0 \n"
        "pminsw %%mm3, %%mm1 \n"
        "pminsw %%mm2, %%mm0 \n"
        "pmaxsw %%mm1, %%mm0 \n"
        "movd   %%mm0, %0    \n"
        :"=m"(*(x264_union32_t*)dst)
        :"m"(M32( a )), "m"(M32( b )), "m"(M32( c ))
    );
}

#define x264_predictor_difference x264_predictor_difference_mmx2
static ALWAYS_INLINE int x264_predictor_difference_mmx2( int16_t (*mvc)[2], intptr_t i_mvc )
{
    int sum;
    static const uint64_t pw_1 = 0x0001000100010001ULL;

    asm(
        "pxor    %%mm4, %%mm4 \n"
        "test    $1, %1       \n"
        "jnz 3f               \n"
        "movd    -8(%2,%1,4), %%mm0 \n"
        "movd    -4(%2,%1,4), %%mm3 \n"
        "psubw   %%mm3, %%mm0 \n"
        "jmp 2f               \n"
        "3:                   \n"
        "dec     %1           \n"
        "1:                   \n"
        "movq    -8(%2,%1,4), %%mm0 \n"
        "psubw   -4(%2,%1,4), %%mm0 \n"
        "2:                   \n"
        "sub     $2,    %1    \n"
        "pxor    %%mm2, %%mm2 \n"
        "psubw   %%mm0, %%mm2 \n"
        "pmaxsw  %%mm2, %%mm0 \n"
        "paddusw %%mm0, %%mm4 \n"
        "jg 1b                \n"
        "pmaddwd %4, %%mm4    \n"
        "pshufw $14, %%mm4, %%mm0 \n"
        "paddd   %%mm0, %%mm4 \n"
        "movd    %%mm4, %0    \n"
        :"=r"(sum), "+r"(i_mvc)
        :"r"(mvc), "m"(M64( mvc )), "m"(pw_1)
    );
    return sum;
}

#define x264_cabac_mvd_sum x264_cabac_mvd_sum_mmx2
static ALWAYS_INLINE uint16_t x264_cabac_mvd_sum_mmx2(uint8_t *mvdleft, uint8_t *mvdtop)
{
    static const uint64_t pb_2    = 0x0202020202020202ULL;
    static const uint64_t pb_32   = 0x2020202020202020ULL;
    static const uint64_t pb_33   = 0x2121212121212121ULL;
    int amvd;
    asm(
        "movd         %1, %%mm0 \n"
        "movd         %2, %%mm1 \n"
        "paddusb   %%mm1, %%mm0 \n"
        "pminub       %5, %%mm0 \n"
        "pxor      %%mm2, %%mm2 \n"
        "movq      %%mm0, %%mm1 \n"
        "pcmpgtb      %3, %%mm0 \n"
        "pcmpgtb      %4, %%mm1 \n"
        "psubb     %%mm0, %%mm2 \n"
        "psubb     %%mm1, %%mm2 \n"
        "movd      %%mm2, %0    \n"
        :"=r"(amvd)
        :"m"(M16( mvdleft )),"m"(M16( mvdtop )),
         "m"(pb_2),"m"(pb_32),"m"(pb_33)
    );
    return amvd;
}

#define x264_predictor_roundclip x264_predictor_roundclip_mmx2
static void ALWAYS_INLINE x264_predictor_roundclip_mmx2( int16_t (*dst)[2], int16_t (*mvc)[2], int i_mvc, int mv_x_min, int mv_x_max, int mv_y_min, int mv_y_max )
{
    uint32_t mv_min = pack16to32_mask( mv_x_min, mv_y_min );
    uint32_t mv_max = pack16to32_mask( mv_x_max, mv_y_max );
    static const uint64_t pw_2 = 0x0002000200020002ULL;
    intptr_t i = i_mvc;
    asm(
        "movd    %2, %%mm5       \n"
        "movd    %3, %%mm6       \n"
        "movq    %4, %%mm7       \n"
        "punpckldq %%mm5, %%mm5  \n"
        "punpckldq %%mm6, %%mm6  \n"
        "test $1, %0             \n"
        "jz 1f                   \n"
        "movd -4(%6,%0,4), %%mm0 \n"
        "paddw %%mm7, %%mm0      \n"
        "psraw $2, %%mm0         \n"
        "pmaxsw %%mm5, %%mm0     \n"
        "pminsw %%mm6, %%mm0     \n"
        "movd %%mm0, -4(%5,%0,4) \n"
        "dec %0                  \n"
        "jz 2f                   \n"
        "1:                      \n"
        "movq -8(%6,%0,4), %%mm0 \n"
        "paddw %%mm7, %%mm0      \n"
        "psraw $2, %%mm0         \n"
        "pmaxsw %%mm5, %%mm0     \n"
        "pminsw %%mm6, %%mm0     \n"
        "movq %%mm0, -8(%5,%0,4) \n"
        "sub $2, %0              \n"
        "jnz 1b                  \n"
        "2:                      \n"
        :"+r"(i), "=m"(M64( dst ))
        :"g"(mv_min), "g"(mv_max), "m"(pw_2), "r"(dst), "r"(mvc), "m"(M64( mvc ))
    );
}

#endif

#endif
