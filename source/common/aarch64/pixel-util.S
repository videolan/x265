/*****************************************************************************
 * Copyright (C) 2020 MulticoreWare, Inc
 *
 * Authors: Yimeng Su <yimeng.su@huawei.com>
 *          Hongbin Liu <liuhongbin1@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "asm.S"

.section .rodata

.align 4

.text

.macro x265_satd_4x8_8x4_end_neon
    add             v0.8h, v4.8h, v6.8h
    add             v1.8h, v5.8h, v7.8h
    sub             v2.8h, v4.8h, v6.8h
    sub             v3.8h, v5.8h, v7.8h

    trn1            v16.8h, v0.8h, v1.8h
    trn2            v17.8h, v0.8h, v1.8h
    add             v4.8h, v16.8h, v17.8h
    trn1            v18.8h, v2.8h, v3.8h
    trn2            v19.8h, v2.8h, v3.8h
    sub             v5.8h, v16.8h, v17.8h
    add             v6.8h, v18.8h, v19.8h
    sub             v7.8h, v18.8h, v19.8h
    trn1            v0.4s, v4.4s, v6.4s
    trn2            v2.4s, v4.4s, v6.4s
    abs             v0.8h, v0.8h
    trn1            v1.4s, v5.4s, v7.4s
    trn2            v3.4s, v5.4s, v7.4s
    abs             v2.8h, v2.8h
    abs             v1.8h, v1.8h
    abs             v3.8h, v3.8h
    umax            v0.8h, v0.8h, v2.8h
    umax            v1.8h, v1.8h, v3.8h
    add             v0.8h, v0.8h, v1.8h
    uaddlv          s0, v0.8h
.endm

.macro pixel_satd_4x8_neon
    ld1r             {v1.2s}, [x2], x3
    ld1r            {v0.2s}, [x0], x1
    ld1r            {v3.2s}, [x2], x3
    ld1r            {v2.2s}, [x0], x1
    ld1r            {v5.2s}, [x2], x3
    ld1r            {v4.2s}, [x0], x1
    ld1r            {v7.2s}, [x2], x3
    ld1r            {v6.2s}, [x0], x1

    ld1             {v1.s}[1], [x2], x3
    ld1             {v0.s}[1], [x0], x1
    usubl           v0.8h, v0.8b, v1.8b
    ld1             {v3.s}[1], [x2], x3
    ld1             {v2.s}[1], [x0], x1
    usubl           v1.8h, v2.8b, v3.8b
    ld1             {v5.s}[1], [x2], x3
    ld1             {v4.s}[1], [x0], x1
    usubl           v2.8h, v4.8b, v5.8b
    ld1             {v7.s}[1], [x2], x3
    add             v4.8h, v0.8h, v1.8h
    sub             v5.8h, v0.8h, v1.8h
    ld1             {v6.s}[1], [x0], x1
    usubl           v3.8h, v6.8b, v7.8b
    add         v6.8h, v2.8h, v3.8h
    sub         v7.8h, v2.8h, v3.8h
    x265_satd_4x8_8x4_end_neon
.endm

// template<int w, int h>
// int satd4(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2)
function x265_pixel_satd_4x8_neon
    pixel_satd_4x8_neon
    mov               w0, v0.s[0]
    ret
endfunc

// template<int w, int h>
// int satd4(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2)
function x265_pixel_satd_4x16_neon
    eor             w4, w4, w4
    pixel_satd_4x8_neon
    mov               w5, v0.s[0]
    add             w4, w4, w5
    pixel_satd_4x8_neon
    mov               w5, v0.s[0]
    add             w0, w5, w4
    ret
endfunc

// template<int w, int h>
// int satd4(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2)
function x265_pixel_satd_4x32_neon
    eor             w4, w4, w4
.rept 4
    pixel_satd_4x8_neon
    mov             w5, v0.s[0]
    add             w4, w4, w5
.endr
    mov             w0, w4
    ret
endfunc

// template<int w, int h>
// int satd4(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2)
function x265_pixel_satd_12x16_neon
    mov             x4, x0
    mov             x5, x2
    eor             w7, w7, w7
    pixel_satd_4x8_neon
    mov             w6, v0.s[0]
    add             w7, w7, w6
    pixel_satd_4x8_neon
    mov             w6, v0.s[0]
    add             w7, w7, w6

    add             x0, x4, #4
    add             x2, x5, #4
    pixel_satd_4x8_neon
    mov             w6, v0.s[0]
    add             w7, w7, w6
    pixel_satd_4x8_neon
    mov             w6, v0.s[0]
    add             w7, w7, w6

    add             x0, x4, #8
    add             x2, x5, #8
    pixel_satd_4x8_neon
    mov             w6, v0.s[0]
    add             w7, w7, w6
    pixel_satd_4x8_neon
    mov             w6, v0.s[0]
    add             w0, w7, w6
    ret
endfunc

// template<int w, int h>
// int satd4(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2)
function x265_pixel_satd_12x32_neon
    mov             x4, x0
    mov             x5, x2
    eor             w7, w7, w7
.rept 4
    pixel_satd_4x8_neon
    mov             w6, v0.s[0]
    add             w7, w7, w6
.endr

    add             x0, x4, #4
    add             x2, x5, #4
.rept 4
    pixel_satd_4x8_neon
    mov             w6, v0.s[0]
    add             w7, w7, w6
.endr

    add             x0, x4, #8
    add             x2, x5, #8
.rept 4
    pixel_satd_4x8_neon
    mov             w6, v0.s[0]
    add             w7, w7, w6
.endr

    mov             w0, w7
    ret
endfunc

// template<int w, int h>
// int satd4(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2)
function x265_pixel_satd_8x8_neon
    eor             w4, w4, w4
    mov             x6, x0
    mov             x7, x2
    pixel_satd_4x8_neon
    mov             w5, v0.s[0]
    add             w4, w4, w5
    add             x0, x6, #4
    add             x2, x7, #4
    pixel_satd_4x8_neon
    mov             w5, v0.s[0]
    add             w0, w4, w5
    ret
endfunc

// int psyCost_pp(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride)
function x265_psyCost_4x4_neon
    ld1r            {v4.2s}, [x0], x1
    ld1r            {v5.2s}, [x0], x1
    ld1             {v4.s}[1], [x0], x1
    ld1             {v5.s}[1], [x0], x1

    ld1r            {v6.2s}, [x2], x3
    ld1r            {v7.2s}, [x2], x3
    ld1             {v6.s}[1], [x2], x3
    ld1             {v7.s}[1], [x2], x3

    uaddl           v2.8h, v4.8b, v5.8b
    usubl           v3.8h, v4.8b, v5.8b
    uaddl           v18.8h, v6.8b, v7.8b
    usubl           v19.8h, v6.8b, v7.8b

    mov             v20.d[0], v2.d[1]
    add             v0.4h, v2.4h, v20.4h
    sub             v1.4h, v2.4h, v20.4h
    mov             v21.d[0], v3.d[1]
    add             v22.4h, v3.4h, v21.4h
    sub             v23.4h, v3.4h, v21.4h

    mov             v24.d[0], v18.d[1]
    add             v16.4h, v18.4h, v24.4h
    sub             v17.4h, v18.4h, v24.4h
    mov             v25.d[0], v19.d[1]
    add             v26.4h, v19.4h, v25.4h
    sub             v27.4h, v19.4h, v25.4h

    mov             v0.d[1], v22.d[0]
    mov             v1.d[1], v23.d[0]
    trn1            v22.8h, v0.8h, v1.8h
    trn2            v23.8h, v0.8h, v1.8h
    mov             v16.d[1], v26.d[0]
    mov             v17.d[1], v27.d[0]
    trn1            v26.8h, v16.8h, v17.8h
    trn2            v27.8h, v16.8h, v17.8h

    add             v2.8h, v22.8h, v23.8h
    sub             v3.8h, v22.8h, v23.8h
    add             v18.8h, v26.8h, v27.8h
    sub             v19.8h, v26.8h, v27.8h

    uaddl           v20.8h, v4.8b, v5.8b
    uaddl           v21.8h, v6.8b, v7.8b

    trn1            v0.4s, v2.4s, v3.4s
    trn2            v1.4s, v2.4s, v3.4s
    trn1            v16.4s, v18.4s, v19.4s
    trn2            v17.4s, v18.4s, v19.4s
    abs             v0.8h, v0.8h
    abs             v16.8h, v16.8h
    abs             v1.8h, v1.8h
    abs             v17.8h, v17.8h

    uaddlv          s20, v20.8h
    uaddlv          s21, v21.8h
    mov             v20.s[1], v21.s[0]

    smax            v0.8h, v0.8h, v1.8h
    smax            v16.8h, v16.8h, v17.8h

    trn1            v4.2d, v0.2d, v16.2d
    trn2            v5.2d, v0.2d, v16.2d
    add             v0.8h, v4.8h, v5.8h
    mov             v4.d[0], v0.d[1]
    uaddlv          s0, v0.4h
    uaddlv          s4, v4.4h

    ushr            v20.2s, v20.2s, #2
    mov             v0.s[1], v4.s[0]
    sub             v0.2s, v0.2s, v20.2s
    mov             w0, v0.s[0]
    mov             w1, v0.s[1]
    subs            w0, w0, w1
    cneg            w0, w0, mi

    ret
endfunc

// uint32_t quant_c(const int16_t* coef, const int32_t* quantCoeff, int32_t* deltaU, int16_t* qCoef, int qBits, int add, int numCoeff)
function x265_quant_neon
    mov             w9, #1
    lsl             w9, w9, w4
    dup             v0.2s, w9
    neg             w9, w4
    dup             v1.4s, w9
    add             w9, w9, #8
    dup             v2.4s, w9
    dup             v3.4s, w5

    lsr             w6, w6, #2
    eor             v4.16b, v4.16b, v4.16b
    eor             w10, w10, w10
    eor             v17.16b, v17.16b, v17.16b

.loop_quant:

    ld1             {v18.4h}, [x0], #8
    ld1             {v7.4s}, [x1], #16
    sxtl            v6.4s, v18.4h

    cmlt            v5.4s, v6.4s, #0

    abs             v6.4s, v6.4s


    mul             v6.4s, v6.4s, v7.4s

    add             v7.4s, v6.4s, v3.4s
    sshl            v7.4s, v7.4s, v1.4s

    mls             v6.4s, v7.4s, v0.s[0]
    sshl            v16.4s, v6.4s, v2.4s
    st1             {v16.4s}, [x2], #16

    // numsig
    cmeq            v16.4s, v7.4s, v17.4s
    add             v4.4s, v4.4s, v16.4s
    add             w10, w10, #4

    // level *= sign
    eor             v16.16b, v7.16b, v5.16b
    sub             v16.4s, v16.4s, v5.4s
    sqxtn           v5.4h, v16.4s
    st1             {v5.4h}, [x3], #8

    subs            w6, w6, #1
    b.ne             .loop_quant

    addv            s4, v4.4s
    mov             w9, v4.s[0]
    add             w0, w10, w9
    ret
endfunc

.macro satd_4x4_neon
    ld1             {v1.s}[0], [x2], x3
    ld1             {v0.s}[0], [x0], x1
    ld1             {v3.s}[0], [x2], x3
    ld1             {v2.s}[0], [x0], x1

    ld1             {v1.s}[1], [x2], x3
    ld1             {v0.s}[1], [x0], x1
    ld1             {v3.s}[1], [x2], x3
    ld1             {v2.s}[1], [x0], x1

    usubl           v4.8h, v0.8b, v1.8b
    usubl           v5.8h, v2.8b, v3.8b

    add             v6.8h, v4.8h, v5.8h
    sub             v7.8h, v4.8h, v5.8h

    mov             v4.d[0], v6.d[1]
    add             v0.8h, v6.8h, v4.8h
    sub             v2.8h, v6.8h, v4.8h

    mov             v5.d[0], v7.d[1]
    add             v1.8h, v7.8h, v5.8h
    sub             v3.8h, v7.8h, v5.8h

    trn1            v4.4h, v0.4h, v1.4h
    trn2            v5.4h, v0.4h, v1.4h

    trn1            v6.4h, v2.4h, v3.4h
    trn2            v7.4h, v2.4h, v3.4h

    add             v0.4h, v4.4h, v5.4h
    sub             v1.4h, v4.4h, v5.4h

    add             v2.4h, v6.4h, v7.4h
    sub             v3.4h, v6.4h, v7.4h

    trn1            v4.2s, v0.2s, v1.2s
    trn2            v5.2s, v0.2s, v1.2s

    trn1            v6.2s, v2.2s, v3.2s
    trn2            v7.2s, v2.2s, v3.2s

    abs             v4.4h, v4.4h
    abs             v5.4h, v5.4h
    abs             v6.4h, v6.4h
    abs             v7.4h, v7.4h

    smax            v1.4h, v4.4h, v5.4h
    smax            v2.4h, v6.4h, v7.4h

    add             v0.4h, v1.4h, v2.4h
    uaddlp          v0.2s, v0.4h
    uaddlp          v0.1d, v0.2s
.endm

// int satd_4x4(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2)
function x265_pixel_satd_4x4_neon
    satd_4x4_neon
    umov            x0, v0.d[0]
    ret
endfunc

// int satd_8x4(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2)
function x265_pixel_satd_8x4_neon
    mov             x4, x0
    mov             x5, x2
    satd_4x4_neon
    add             x0, x4, #4
    add             x2, x5, #4
    umov            x6, v0.d[0]
    satd_4x4_neon
    umov            x0, v0.d[0]
    add             x0, x0, x6
    ret
endfunc
