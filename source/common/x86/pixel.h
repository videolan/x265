/*****************************************************************************
 * pixel.h: x86 pixel metrics
 *****************************************************************************
 * Copyright (C) 2003-2013 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
 *          Jason Garrett-Glaser <darkshikari@gmail.com>
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

#ifndef X265_I386_PIXEL_H
#define X265_I386_PIXEL_H

#define DECL_PIXELS(ret, name, suffix, args) \
    ret x265_pixel_ ## name ## _16x64_ ## suffix args; \
    ret x265_pixel_ ## name ## _16x32_ ## suffix args; \
    ret x265_pixel_ ## name ## _16x16_ ## suffix args; \
    ret x265_pixel_ ## name ## _16x12_ ## suffix args; \
    ret x265_pixel_ ## name ## _16x8_ ## suffix args; \
    ret x265_pixel_ ## name ## _16x4_ ## suffix args; \
    ret x265_pixel_ ## name ## _8x32_ ## suffix args; \
    ret x265_pixel_ ## name ## _8x16_ ## suffix args; \
    ret x265_pixel_ ## name ## _8x8_ ## suffix args; \
    ret x265_pixel_ ## name ## _8x4_ ## suffix args; \
    ret x265_pixel_ ## name ## _4x16_ ## suffix args; \
    ret x265_pixel_ ## name ## _4x8_ ## suffix args; \
    ret x265_pixel_ ## name ## _4x4_ ## suffix args; \
    ret x265_pixel_ ## name ## _32x8_ ## suffix args; \
    ret x265_pixel_ ## name ## _32x16_ ## suffix args; \
    ret x265_pixel_ ## name ## _32x24_ ## suffix args; \
    ret x265_pixel_ ## name ## _24x32_ ## suffix args; \
    ret x265_pixel_ ## name ## _32x32_ ## suffix args; \
    ret x265_pixel_ ## name ## _32x64_ ## suffix args; \
    ret x265_pixel_ ## name ## _64x16_ ## suffix args; \
    ret x265_pixel_ ## name ## _64x32_ ## suffix args; \
    ret x265_pixel_ ## name ## _64x48_ ## suffix args; \
    ret x265_pixel_ ## name ## _64x64_ ## suffix args; \
    ret x265_pixel_ ## name ## _48x64_ ## suffix args; \
    ret x265_pixel_ ## name ## _24x32_ ## suffix args; \
    ret x265_pixel_ ## name ## _12x16_ ## suffix args; \

#define DECL_X1(name, suffix) \
    DECL_PIXELS(int, name, suffix, (pixel *, intptr_t, pixel *, intptr_t))

#define DECL_X1_SS(name, suffix) \
    DECL_PIXELS(int, name, suffix, (int16_t *, intptr_t, int16_t *, intptr_t))

#define DECL_X1_SP(name, suffix) \
    DECL_PIXELS(int, name, suffix, (int16_t *, intptr_t, pixel *, intptr_t))

#define DECL_X4(name, suffix) \
    DECL_PIXELS(void, name ## _x3, suffix, (pixel *, pixel *, pixel *, pixel *, intptr_t, int *)) \
    DECL_PIXELS(void, name ## _x4, suffix, (pixel *, pixel *, pixel *, pixel *, pixel *, intptr_t, int *))

DECL_X1(sad, mmx2)
DECL_X1(sad, sse2)
DECL_X4(sad, sse2_misalign)
DECL_X1(sad, sse3)
DECL_X1(sad, sse2_aligned)
DECL_X1(sad, ssse3)
DECL_X1(sad, ssse3_aligned)
DECL_X1(sad, avx2)
DECL_X1(sad, avx2_aligned)
DECL_X4(sad, mmx2)
DECL_X4(sad, sse2)
DECL_X4(sad, sse3)
DECL_X4(sad, ssse3)
DECL_X4(sad, avx)
DECL_X4(sad, avx2)
DECL_X1(ssd, mmx)
DECL_X1(ssd, mmx2)
DECL_X1(ssd, sse2slow)
DECL_X1(ssd, sse2)
DECL_X1(ssd, ssse3)
DECL_X1(ssd, avx)
DECL_X1(ssd, xop)
DECL_X1(ssd, avx2)
DECL_X1_SS(ssd_ss, mmx)
DECL_X1_SS(ssd_ss, mmx2)
DECL_X1_SS(ssd_ss, sse2slow)
DECL_X1_SS(ssd_ss, sse2)
DECL_X1_SS(ssd_ss, ssse3)
DECL_X1_SS(ssd_ss, sse4)
DECL_X1_SS(ssd_ss, avx)
DECL_X1_SS(ssd_ss, xop)
DECL_X1_SS(ssd_ss, avx2)
DECL_X1_SP(ssd_sp, sse4)
DECL_X1(satd, mmx2)
DECL_X1(satd, sse2)
DECL_X1(satd, ssse3)
DECL_X1(satd, ssse3_atom)
DECL_X1(satd, sse4)
DECL_X1(satd, avx)
DECL_X1(satd, xop)
DECL_X1(satd, avx2)
DECL_X1(sa8d, mmx2)
DECL_X1(sa8d, sse2)
DECL_X1(sa8d, ssse3)
DECL_X1(sa8d, ssse3_atom)
DECL_X1(sa8d, sse4)
DECL_X1(sa8d, avx)
DECL_X1(sa8d, xop)
DECL_X1(sa8d, avx2)
DECL_X1(sad, cache32_mmx2);
DECL_X1(sad, cache64_mmx2);
DECL_X1(sad, cache64_sse2);
DECL_X1(sad, cache64_ssse3);
DECL_X4(sad, cache32_mmx2);
DECL_X4(sad, cache64_mmx2);
DECL_X4(sad, cache64_sse2);
DECL_X4(sad, cache64_ssse3);

int x265_pixel_satd_8x32_sse2(pixel *, intptr_t, pixel *, intptr_t);
int x265_pixel_satd_16x4_sse2(pixel *, intptr_t, pixel *, intptr_t);
int x265_pixel_satd_16x12_sse2(pixel *, intptr_t, pixel *, intptr_t);
int x265_pixel_satd_16x32_sse2(pixel *, intptr_t, pixel *, intptr_t);
int x265_pixel_satd_16x64_sse2(pixel *, intptr_t, pixel *, intptr_t);
void x265_scale1D_128to64_ssse3(pixel *, pixel *, intptr_t);
void x265_scale2D_64to32_ssse3(pixel *, pixel *, intptr_t);

#define DECL_HEVC_SSD(suffix) \
    int x265_pixel_ssd_32x64_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_16x64_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_32x32_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_32x16_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_16x32_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_32x24_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_24x32_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_32x8_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_8x32_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_16x16_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_16x8_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_8x16_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_16x12_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_16x4_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_8x8_ ## suffix(pixel *, intptr_t, pixel *, intptr_t); \
    int x265_pixel_ssd_8x4_ ## suffix(pixel *, intptr_t, pixel *, intptr_t);
DECL_HEVC_SSD(sse2)
DECL_HEVC_SSD(ssse3)
DECL_HEVC_SSD(avx)

int x265_pixel_ssd_12x16_sse4(pixel *, intptr_t, pixel *, intptr_t);
int x265_pixel_ssd_24x32_sse4(pixel *, intptr_t, pixel *, intptr_t);
int x265_pixel_ssd_48x64_sse4(pixel *, intptr_t, pixel *, intptr_t);
int x265_pixel_ssd_64x16_sse4(pixel *, intptr_t, pixel *, intptr_t);
int x265_pixel_ssd_64x32_sse4(pixel *, intptr_t, pixel *, intptr_t);
int x265_pixel_ssd_64x48_sse4(pixel *, intptr_t, pixel *, intptr_t);
int x265_pixel_ssd_64x64_sse4(pixel *, intptr_t, pixel *, intptr_t);

#define DECL_SUF(func, args) \
    void func ## _mmx2 args; \
    void func ## _sse2 args; \
    void func ## _ssse3 args;
DECL_SUF(x265_pixel_avg_64x64, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_64x48, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_64x16, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_48x64, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_32x64, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_32x32, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_32x24, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_32x16, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_32x8,  (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_24x32, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_16x64, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_16x32, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_16x16, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_16x12, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_16x8,  (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_16x4,  (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_12x16, (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_8x32,  (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_8x16,  (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_8x8,   (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_8x4,   (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_4x16,  (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_4x8,   (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))
DECL_SUF(x265_pixel_avg_4x4,   (pixel *, intptr_t, pixel *, intptr_t, pixel *, intptr_t, int))

#define SETUP_CHROMA_PIXELSUB_PS_FUNC(W, H, cpu) \
    void x265_pixel_sub_ps_ ## W ## x ## H ## cpu(int16_t * dest, intptr_t destride, pixel * src0, pixel * src1, intptr_t srcstride0, intptr_t srcstride1); \
    void x265_pixel_add_ps_ ## W ## x ## H ## cpu(pixel * dest, intptr_t destride, pixel * src0, int16_t * scr1, intptr_t srcStride0, intptr_t srcStride1);

#define CHROMA_PIXELSUB_DEF(cpu) \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(4, 4, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(4, 2, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(2, 4, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(8, 8, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(8, 4, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(4, 8, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(8, 6, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(6, 8, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(8, 2, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(2, 8, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(16, 16, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(16, 8, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(8, 16, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(16, 12, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(12, 16, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(16, 4, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(4, 16, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(32, 32, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(32, 16, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(16, 32, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(32, 24, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(24, 32, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(32, 8, cpu); \
    SETUP_CHROMA_PIXELSUB_PS_FUNC(8, 32, cpu);

#define SETUP_LUMA_PIXELSUB_PS_FUNC(W, H, cpu) \
    void x265_pixel_sub_ps_ ## W ## x ## H ## cpu(int16_t * dest, intptr_t destride, pixel * src0, pixel * src1, intptr_t srcstride0, intptr_t srcstride1); \
    void x265_pixel_add_ps_ ## W ## x ## H ## cpu(pixel * dest, intptr_t destride, pixel * src0, int16_t * scr1, intptr_t srcStride0, intptr_t srcStride1);

#define LUMA_PIXELSUB_DEF(cpu) \
    SETUP_LUMA_PIXELSUB_PS_FUNC(4,   4, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(8,   8, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(8,   4, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(4,   8, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(16, 16, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(16,  8, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(8,  16, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(16, 12, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(12, 16, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(16,  4, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(4,  16, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(32, 32, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(32, 16, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(16, 32, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(32, 24, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(24, 32, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(32,  8, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(8,  32, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(64, 64, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(64, 32, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(32, 64, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(64, 48, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(48, 64, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(64, 16, cpu); \
    SETUP_LUMA_PIXELSUB_PS_FUNC(16, 64, cpu);

CHROMA_PIXELSUB_DEF(_sse4);
LUMA_PIXELSUB_DEF(_sse4);

#define SETUP_LUMA_PIXELVAR_FUNC(W, H, cpu) \
    uint64_t x265_pixel_var_ ## W ## x ## H ## cpu(pixel * pix, intptr_t pixstride);

#define LUMA_PIXELVAR_DEF(cpu) \
    SETUP_LUMA_PIXELVAR_FUNC(8,   8, cpu); \
    SETUP_LUMA_PIXELVAR_FUNC(16, 16, cpu); \
    SETUP_LUMA_PIXELVAR_FUNC(32, 32, cpu); \
    SETUP_LUMA_PIXELVAR_FUNC(64, 64, cpu);

LUMA_PIXELVAR_DEF(_sse2);

#undef DECL_PIXELS
#undef DECL_SUF
#undef DECL_HEVC_SSD
#undef DECL_X1
#undef DECL_X4
#undef SETUP_CHROMA_PIXELSUB_PS_FUNC
#undef SETUP_LUMA_PIXELSUB_PS_FUNC
#undef CHROMA_PIXELSUB_DEF
#undef LUMA_PIXELSUB_DEF
#undef LUMA_PIXELVAR_DEF
#undef SETUP_LUMA_PIXELVAR_FUNC

#endif // ifndef X265_I386_PIXEL_H
