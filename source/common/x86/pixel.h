/*****************************************************************************
 * pixel.h: x86 pixel metrics
 *****************************************************************************
 * Copyright (C) 2003-2013 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
 *          Fiona Glaser <fiona@x264.com>
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
    DECL_PIXELS(int, name, suffix, (const pixel*, intptr_t, const pixel*, intptr_t))

#define DECL_X1_SS(name, suffix) \
    DECL_PIXELS(int, name, suffix, (const int16_t*, intptr_t, const int16_t*, intptr_t))

#define DECL_X1_SP(name, suffix) \
    DECL_PIXELS(int, name, suffix, (const int16_t*, intptr_t, const pixel*, intptr_t))

#define DECL_X4(name, suffix) \
    DECL_PIXELS(void, name ## _x3, suffix, (const pixel*, const pixel*, const pixel*, const pixel*, intptr_t, int32_t*)) \
    DECL_PIXELS(void, name ## _x4, suffix, (const pixel*, const pixel*, const pixel*, const pixel*, const pixel*, intptr_t, int32_t*))

/* sad-a.asm */
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
DECL_X1(sad, cache32_mmx2);
DECL_X1(sad, cache64_mmx2);
DECL_X1(sad, cache64_sse2);
DECL_X1(sad, cache64_ssse3);
DECL_X4(sad, cache32_mmx2);
DECL_X4(sad, cache64_mmx2);
DECL_X4(sad, cache64_sse2);
DECL_X4(sad, cache64_ssse3);

/* pixel-a.asm */
DECL_X1(satd, mmx2)
DECL_X1(satd, sse2)
DECL_X1(satd, ssse3)
DECL_X1(satd, ssse3_atom)
DECL_X1(satd, sse4)
DECL_X1(satd, avx)
DECL_X1(satd, xop)
DECL_X1(satd, avx2)
int x265_pixel_satd_16x24_avx(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_satd_32x48_avx(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_satd_24x64_avx(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_satd_8x64_avx(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_satd_8x12_avx(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_satd_12x32_avx(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_satd_4x32_avx(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_satd_8x32_sse2(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_satd_16x4_sse2(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_satd_16x12_sse2(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_satd_16x32_sse2(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_satd_16x64_sse2(const pixel*, intptr_t, const pixel*, intptr_t);

DECL_X1(sa8d, mmx2)
DECL_X1(sa8d, sse2)
DECL_X1(sa8d, ssse3)
DECL_X1(sa8d, ssse3_atom)
DECL_X1(sa8d, sse4)
DECL_X1(sa8d, avx)
DECL_X1(sa8d, xop)
DECL_X1(sa8d, avx2)

/* ssd-a.asm */
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
#define DECL_HEVC_SSD(suffix) \
    int x265_pixel_ssd_32x64_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_16x64_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_32x32_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_32x16_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_16x32_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_32x24_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_24x32_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_32x8_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_8x32_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_16x16_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_16x8_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_8x16_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_16x12_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_16x4_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_8x8_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t); \
    int x265_pixel_ssd_8x4_ ## suffix(const pixel*, intptr_t, const pixel*, intptr_t);
DECL_HEVC_SSD(sse2)
DECL_HEVC_SSD(ssse3)
DECL_HEVC_SSD(avx)

int x265_pixel_ssd_12x16_sse4(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_ssd_24x32_sse4(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_ssd_48x64_sse4(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_ssd_64x16_sse4(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_ssd_64x32_sse4(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_ssd_64x48_sse4(const pixel*, intptr_t, const pixel*, intptr_t);
int x265_pixel_ssd_64x64_sse4(const pixel*, intptr_t, const pixel*, intptr_t);

int x265_pixel_ssd_s_4_sse2(const int16_t*, intptr_t);
int x265_pixel_ssd_s_8_sse2(const int16_t*, intptr_t);
int x265_pixel_ssd_s_16_sse2(const int16_t*, intptr_t);
int x265_pixel_ssd_s_32_sse2(const int16_t*, intptr_t);
int x265_pixel_ssd_s_16_avx2(const int16_t*, intptr_t);
int x265_pixel_ssd_s_32_avx2(const int16_t*, intptr_t);

#define ADDAVG(func)  \
    void x265_ ## func ## _sse4(const int16_t*, const int16_t*, pixel*, intptr_t, intptr_t, intptr_t); \
    void x265_ ## func ## _avx2(const int16_t*, const int16_t*, pixel*, intptr_t, intptr_t, intptr_t);
ADDAVG(addAvg_2x4)
ADDAVG(addAvg_2x8)
ADDAVG(addAvg_4x2);
ADDAVG(addAvg_4x4)
ADDAVG(addAvg_4x8)
ADDAVG(addAvg_4x16)
ADDAVG(addAvg_6x8)
ADDAVG(addAvg_8x2)
ADDAVG(addAvg_8x4)
ADDAVG(addAvg_8x6)
ADDAVG(addAvg_8x8)
ADDAVG(addAvg_8x16)
ADDAVG(addAvg_8x32)
ADDAVG(addAvg_12x16)
ADDAVG(addAvg_16x4)
ADDAVG(addAvg_16x8)
ADDAVG(addAvg_16x12)
ADDAVG(addAvg_16x16)
ADDAVG(addAvg_16x32)
ADDAVG(addAvg_16x64)
ADDAVG(addAvg_24x32)
ADDAVG(addAvg_32x8)
ADDAVG(addAvg_32x16)
ADDAVG(addAvg_32x24)
ADDAVG(addAvg_32x32)
ADDAVG(addAvg_32x64)
ADDAVG(addAvg_48x64)
ADDAVG(addAvg_64x16)
ADDAVG(addAvg_64x32)
ADDAVG(addAvg_64x48)
ADDAVG(addAvg_64x64)

ADDAVG(addAvg_2x16)
ADDAVG(addAvg_4x32)
ADDAVG(addAvg_6x16)
ADDAVG(addAvg_8x12)
ADDAVG(addAvg_8x64)
ADDAVG(addAvg_12x32)
ADDAVG(addAvg_16x24)
ADDAVG(addAvg_24x64)
ADDAVG(addAvg_32x48)

void x265_downShift_16_sse2(const uint16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int width, int height, int shift, uint16_t mask);
void x265_upShift_8_sse4(const uint8_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int width, int height, int shift);
int x265_psyCost_pp_4x4_sse4(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);
int x265_psyCost_pp_8x8_sse4(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);
int x265_psyCost_pp_16x16_sse4(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);
int x265_psyCost_pp_32x32_sse4(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);
int x265_psyCost_pp_64x64_sse4(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);
int x265_psyCost_ss_4x4_sse4(const int16_t* source, intptr_t sstride, const int16_t* recon, intptr_t rstride);
int x265_psyCost_ss_8x8_sse4(const int16_t* source, intptr_t sstride, const int16_t* recon, intptr_t rstride);
int x265_psyCost_ss_16x16_sse4(const int16_t* source, intptr_t sstride, const int16_t* recon, intptr_t rstride);
int x265_psyCost_ss_32x32_sse4(const int16_t* source, intptr_t sstride, const int16_t* recon, intptr_t rstride);
int x265_psyCost_ss_64x64_sse4(const int16_t* source, intptr_t sstride, const int16_t* recon, intptr_t rstride);
void x265_pixel_avg_16x4_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_16x8_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_16x12_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_16x16_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_16x32_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_16x64_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_32x64_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_32x32_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_32x24_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_32x16_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_32x8_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_64x64_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_64x48_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_64x32_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);
void x265_pixel_avg_64x16_avx2(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int);

void x265_pixel_add_ps_16x16_avx2(pixel* a, intptr_t dstride, const pixel* b0, const int16_t* b1, intptr_t sstride0, intptr_t sstride1);
void x265_pixel_add_ps_32x32_avx2(pixel* a, intptr_t dstride, const pixel* b0, const int16_t* b1, intptr_t sstride0, intptr_t sstride1);
void x265_pixel_add_ps_64x64_avx2(pixel* a, intptr_t dstride, const pixel* b0, const int16_t* b1, intptr_t sstride0, intptr_t sstride1);

void x265_pixel_sub_ps_16x16_avx2(int16_t* a, intptr_t dstride, const pixel* b0, const pixel* b1, intptr_t sstride0, intptr_t sstride1);
void x265_pixel_sub_ps_32x32_avx2(int16_t* a, intptr_t dstride, const pixel* b0, const pixel* b1, intptr_t sstride0, intptr_t sstride1);
void x265_pixel_sub_ps_64x64_avx2(int16_t* a, intptr_t dstride, const pixel* b0, const pixel* b1, intptr_t sstride0, intptr_t sstride1);

int x265_psyCost_pp_4x4_avx2(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);
int x265_psyCost_pp_8x8_avx2(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);
int x265_psyCost_pp_16x16_avx2(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);
int x265_psyCost_pp_32x32_avx2(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);
int x265_psyCost_pp_64x64_avx2(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride);

int x265_psyCost_ss_4x4_avx2(const int16_t* source, intptr_t sstride, const int16_t* recon, intptr_t rstride);
int x265_psyCost_ss_8x8_avx2(const int16_t* source, intptr_t sstride, const int16_t* recon, intptr_t rstride);
int x265_psyCost_ss_16x16_avx2(const int16_t* source, intptr_t sstride, const int16_t* recon, intptr_t rstride);
int x265_psyCost_ss_32x32_avx2(const int16_t* source, intptr_t sstride, const int16_t* recon, intptr_t rstride);
int x265_psyCost_ss_64x64_avx2(const int16_t* source, intptr_t sstride, const int16_t* recon, intptr_t rstride);

#undef DECL_PIXELS
#undef DECL_HEVC_SSD
#undef DECL_X1
#undef DECL_X4

#endif // ifndef X265_I386_PIXEL_H
