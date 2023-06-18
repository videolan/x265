/*****************************************************************************
 * Copyright (C) 2020-2021 MulticoreWare, Inc
 *
 * Authors: Yimeng Su <yimeng.su@huawei.com>
 *          Hongbin Liu <liuhongbin1@huawei.com>
 *          Sebastian Pop <spop@amazon.com>
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

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

// uint64_t pixel_var(const pixel* pix, intptr_t i_stride)
function PFX(pixel_var_8x8_neon)
    ld1             {v4.8b}, [x0], x1        // pix[x]
    uxtl            v0.8h, v4.8b             // sum = pix[x]
    umull           v1.8h, v4.8b, v4.8b
    uaddlp          v1.4s, v1.8h             // sqr = pix[x] * pix[x]

.rept 7
    ld1             {v4.8b}, [x0], x1        // pix[x]
    umull           v31.8h, v4.8b, v4.8b
    uaddw           v0.8h, v0.8h, v4.8b      // sum += pix[x]
    uadalp          v1.4s, v31.8h            // sqr += pix[x] * pix[x]
.endr
    uaddlv          s0, v0.8h
    uaddlv          d1, v1.4s
    fmov            w0, s0
    fmov            x1, d1
    orr             x0, x0, x1, lsl #32      // return sum + ((uint64_t)sqr << 32);
    ret
endfunc

.macro pixel_var_start
    movi            v0.16b, #0
    movi            v1.16b, #0
    movi            v2.16b, #0
    movi            v3.16b, #0
.endm

.macro pixel_var_1 v
    uaddw           v0.8h, v0.8h, \v\().8b
    umull           v30.8h, \v\().8b, \v\().8b
    uaddw2          v1.8h, v1.8h, \v\().16b
    umull2          v31.8h, \v\().16b, \v\().16b
    uadalp          v2.4s, v30.8h
    uadalp          v3.4s, v31.8h
.endm

.macro pixel_var_end
    uaddlv          s0, v0.8h
    uaddlv          s1, v1.8h
    add             v2.4s, v2.4s, v3.4s
    fadd            s0, s0, s1
    uaddlv          d2, v2.4s
    fmov            w0, s0
    fmov            x2, d2
    orr             x0, x0, x2, lsl #32
.endm

function PFX(pixel_var_16x16_neon)
    pixel_var_start
    mov             w12, #16
.loop_var_16:
    sub             w12, w12, #1
    ld1             {v4.16b}, [x0], x1
    pixel_var_1 v4
    cbnz            w12, .loop_var_16
    pixel_var_end
    ret
endfunc

function PFX(pixel_var_32x32_neon)
    pixel_var_start
    mov             w12, #32
.loop_var_32:
    sub             w12, w12, #1
    ld1             {v4.16b-v5.16b}, [x0], x1
    pixel_var_1 v4
    pixel_var_1 v5
    cbnz            w12, .loop_var_32
    pixel_var_end
    ret
endfunc

function PFX(pixel_var_64x64_neon)
    pixel_var_start
    mov             w12, #64
.loop_var_64:
    sub             w12, w12, #1
    ld1             {v4.16b-v7.16b}, [x0], x1
    pixel_var_1 v4
    pixel_var_1 v5
    pixel_var_1 v6
    pixel_var_1 v7
    cbnz            w12, .loop_var_64
    pixel_var_end
    ret
endfunc

// void getResidual4_neon(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride)
function PFX(getResidual4_neon)
    lsl             x4, x3, #1
.rept 2
    ld1             {v0.8b}, [x0], x3
    ld1             {v1.8b}, [x1], x3
    ld1             {v2.8b}, [x0], x3
    ld1             {v3.8b}, [x1], x3
    usubl           v4.8h, v0.8b, v1.8b
    usubl           v5.8h, v2.8b, v3.8b
    st1             {v4.8b}, [x2], x4
    st1             {v5.8b}, [x2], x4
.endr
    ret
endfunc

function PFX(getResidual8_neon)
    lsl             x4, x3, #1
.rept 4
    ld1             {v0.8b}, [x0], x3
    ld1             {v1.8b}, [x1], x3
    ld1             {v2.8b}, [x0], x3
    ld1             {v3.8b}, [x1], x3
    usubl           v4.8h, v0.8b, v1.8b
    usubl           v5.8h, v2.8b, v3.8b
    st1             {v4.16b}, [x2], x4
    st1             {v5.16b}, [x2], x4
.endr
    ret
endfunc

function PFX(getResidual16_neon)
    lsl             x4, x3, #1
.rept 8
    ld1             {v0.16b}, [x0], x3
    ld1             {v1.16b}, [x1], x3
    ld1             {v2.16b}, [x0], x3
    ld1             {v3.16b}, [x1], x3
    usubl           v4.8h, v0.8b, v1.8b
    usubl2          v5.8h, v0.16b, v1.16b
    usubl           v6.8h, v2.8b, v3.8b
    usubl2          v7.8h, v2.16b, v3.16b
    st1             {v4.8h-v5.8h}, [x2], x4
    st1             {v6.8h-v7.8h}, [x2], x4
.endr
    ret
endfunc

function PFX(getResidual32_neon)
    lsl             x4, x3, #1
    mov             w12, #4
.loop_residual_32:
    sub             w12, w12, #1
.rept 4
    ld1             {v0.16b-v1.16b}, [x0], x3
    ld1             {v2.16b-v3.16b}, [x1], x3
    ld1             {v4.16b-v5.16b}, [x0], x3
    ld1             {v6.16b-v7.16b}, [x1], x3
    usubl           v16.8h, v0.8b, v2.8b
    usubl2          v17.8h, v0.16b, v2.16b
    usubl           v18.8h, v1.8b, v3.8b
    usubl2          v19.8h, v1.16b, v3.16b
    usubl           v20.8h, v4.8b, v6.8b
    usubl2          v21.8h, v4.16b, v6.16b
    usubl           v22.8h, v5.8b, v7.8b
    usubl2          v23.8h, v5.16b, v7.16b
    st1             {v16.8h-v19.8h}, [x2], x4
    st1             {v20.8h-v23.8h}, [x2], x4
.endr
    cbnz            w12, .loop_residual_32
    ret
endfunc

// void pixel_sub_ps_neon(int16_t* a, intptr_t dstride, const pixel* b0, const pixel* b1, intptr_t sstride0, intptr_t sstride1)
function PFX(pixel_sub_ps_4x4_neon)
    lsl             x1, x1, #1
.rept 2
    ld1             {v0.8b}, [x2], x4
    ld1             {v1.8b}, [x3], x5
    ld1             {v2.8b}, [x2], x4
    ld1             {v3.8b}, [x3], x5
    usubl           v4.8h, v0.8b, v1.8b
    usubl           v5.8h, v2.8b, v3.8b
    st1             {v4.4h}, [x0], x1
    st1             {v5.4h}, [x0], x1
.endr
    ret
endfunc

function PFX(pixel_sub_ps_8x8_neon)
    lsl             x1, x1, #1
.rept 4
    ld1             {v0.8b}, [x2], x4
    ld1             {v1.8b}, [x3], x5
    ld1             {v2.8b}, [x2], x4
    ld1             {v3.8b}, [x3], x5
    usubl           v4.8h, v0.8b, v1.8b
    usubl           v5.8h, v2.8b, v3.8b
    st1             {v4.8h}, [x0], x1
    st1             {v5.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(pixel_sub_ps_16x16_neon)
    lsl             x1, x1, #1
.rept 8
    ld1             {v0.16b}, [x2], x4
    ld1             {v1.16b}, [x3], x5
    ld1             {v2.16b}, [x2], x4
    ld1             {v3.16b}, [x3], x5
    usubl           v4.8h, v0.8b, v1.8b
    usubl2          v5.8h, v0.16b, v1.16b
    usubl           v6.8h, v2.8b, v3.8b
    usubl2          v7.8h, v2.16b, v3.16b
    st1             {v4.8h-v5.8h}, [x0], x1
    st1             {v6.8h-v7.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(pixel_sub_ps_32x32_neon)
    lsl             x1, x1, #1
    mov             w12, #4
.loop_sub_ps_32:
    sub             w12, w12, #1
.rept 4
    ld1             {v0.16b-v1.16b}, [x2], x4
    ld1             {v2.16b-v3.16b}, [x3], x5
    ld1             {v4.16b-v5.16b}, [x2], x4
    ld1             {v6.16b-v7.16b}, [x3], x5
    usubl           v16.8h, v0.8b, v2.8b
    usubl2          v17.8h, v0.16b, v2.16b
    usubl           v18.8h, v1.8b, v3.8b
    usubl2          v19.8h, v1.16b, v3.16b
    usubl           v20.8h, v4.8b, v6.8b
    usubl2          v21.8h, v4.16b, v6.16b
    usubl           v22.8h, v5.8b, v7.8b
    usubl2          v23.8h, v5.16b, v7.16b
    st1             {v16.8h-v19.8h}, [x0], x1
    st1             {v20.8h-v23.8h}, [x0], x1
.endr
    cbnz            w12, .loop_sub_ps_32
    ret
endfunc

function PFX(pixel_sub_ps_64x64_neon)
    lsl             x1, x1, #1
    sub             x1, x1, #64
    mov             w12, #16
.loop_sub_ps_64:
    sub             w12, w12, #1
.rept 4
    ld1             {v0.16b-v3.16b}, [x2], x4
    ld1             {v4.16b-v7.16b}, [x3], x5
    usubl           v16.8h, v0.8b, v4.8b
    usubl2          v17.8h, v0.16b, v4.16b
    usubl           v18.8h, v1.8b, v5.8b
    usubl2          v19.8h, v1.16b, v5.16b
    usubl           v20.8h, v2.8b, v6.8b
    usubl2          v21.8h, v2.16b, v6.16b
    usubl           v22.8h, v3.8b, v7.8b
    usubl2          v23.8h, v3.16b, v7.16b
    st1             {v16.8h-v19.8h}, [x0], #64
    st1             {v20.8h-v23.8h}, [x0], x1
.endr
    cbnz            w12, .loop_sub_ps_64
    ret
endfunc

// chroma sub_ps
function PFX(pixel_sub_ps_4x8_neon)
    lsl             x1, x1, #1
.rept 4
    ld1             {v0.8b}, [x2], x4
    ld1             {v1.8b}, [x3], x5
    ld1             {v2.8b}, [x2], x4
    ld1             {v3.8b}, [x3], x5
    usubl           v4.8h, v0.8b, v1.8b
    usubl           v5.8h, v2.8b, v3.8b
    st1             {v4.4h}, [x0], x1
    st1             {v5.4h}, [x0], x1
.endr
    ret
endfunc

function PFX(pixel_sub_ps_8x16_neon)
    lsl             x1, x1, #1
.rept 8
    ld1             {v0.8b}, [x2], x4
    ld1             {v1.8b}, [x3], x5
    ld1             {v2.8b}, [x2], x4
    ld1             {v3.8b}, [x3], x5
    usubl           v4.8h, v0.8b, v1.8b
    usubl           v5.8h, v2.8b, v3.8b
    st1             {v4.8h}, [x0], x1
    st1             {v5.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(pixel_sub_ps_16x32_neon)
    lsl             x1, x1, #1
.rept 16
    ld1             {v0.16b}, [x2], x4
    ld1             {v1.16b}, [x3], x5
    ld1             {v2.16b}, [x2], x4
    ld1             {v3.16b}, [x3], x5
    usubl           v4.8h, v0.8b, v1.8b
    usubl2          v5.8h, v0.16b, v1.16b
    usubl           v6.8h, v2.8b, v3.8b
    usubl2          v7.8h, v2.16b, v3.16b
    st1             {v4.8h-v5.8h}, [x0], x1
    st1             {v6.8h-v7.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(pixel_sub_ps_32x64_neon)
    lsl             x1, x1, #1
    mov             w12, #8
.loop_sub_ps_32x64:
    sub             w12, w12, #1
.rept 4
    ld1             {v0.16b-v1.16b}, [x2], x4
    ld1             {v2.16b-v3.16b}, [x3], x5
    ld1             {v4.16b-v5.16b}, [x2], x4
    ld1             {v6.16b-v7.16b}, [x3], x5
    usubl           v16.8h, v0.8b, v2.8b
    usubl2          v17.8h, v0.16b, v2.16b
    usubl           v18.8h, v1.8b, v3.8b
    usubl2          v19.8h, v1.16b, v3.16b
    usubl           v20.8h, v4.8b, v6.8b
    usubl2          v21.8h, v4.16b, v6.16b
    usubl           v22.8h, v5.8b, v7.8b
    usubl2          v23.8h, v5.16b, v7.16b
    st1             {v16.8h-v19.8h}, [x0], x1
    st1             {v20.8h-v23.8h}, [x0], x1
.endr
    cbnz            w12, .loop_sub_ps_32x64
    ret
endfunc

// void x265_pixel_add_ps_neon(pixel* a, intptr_t dstride, const pixel* b0, const int16_t* b1, intptr_t sstride0, intptr_t sstride1);
function PFX(pixel_add_ps_4x4_neon)
    lsl             x5, x5, #1
.rept 2
    ld1             {v0.8b}, [x2], x4
    ld1             {v1.8b}, [x2], x4
    ld1             {v2.4h}, [x3], x5
    ld1             {v3.4h}, [x3], x5
    uxtl            v0.8h, v0.8b
    uxtl            v1.8h, v1.8b
    add             v4.8h, v0.8h, v2.8h
    add             v5.8h, v1.8h, v3.8h
    sqxtun          v4.8b, v4.8h
    sqxtun          v5.8b, v5.8h
    st1             {v4.s}[0], [x0], x1
    st1             {v5.s}[0], [x0], x1
.endr
    ret
endfunc

function PFX(pixel_add_ps_8x8_neon)
    lsl             x5, x5, #1
.rept 4
    ld1             {v0.8b}, [x2], x4
    ld1             {v1.8b}, [x2], x4
    ld1             {v2.8h}, [x3], x5
    ld1             {v3.8h}, [x3], x5
    uxtl            v0.8h, v0.8b
    uxtl            v1.8h, v1.8b
    add             v4.8h, v0.8h, v2.8h
    add             v5.8h, v1.8h, v3.8h
    sqxtun          v4.8b, v4.8h
    sqxtun          v5.8b, v5.8h
    st1             {v4.8b}, [x0], x1
    st1             {v5.8b}, [x0], x1
.endr
    ret
endfunc

.macro pixel_add_ps_16xN_neon h
function PFX(pixel_add_ps_16x\h\()_neon)
    lsl             x5, x5, #1
    mov             w12, #\h / 8
.loop_add_ps_16x\h\():
    sub             w12, w12, #1
.rept 4
    ld1             {v0.16b}, [x2], x4
    ld1             {v1.16b}, [x2], x4
    ld1             {v16.8h-v17.8h}, [x3], x5
    ld1             {v18.8h-v19.8h}, [x3], x5
    uxtl            v4.8h, v0.8b
    uxtl2           v5.8h, v0.16b
    uxtl            v6.8h, v1.8b
    uxtl2           v7.8h, v1.16b
    add             v24.8h, v4.8h, v16.8h
    add             v25.8h, v5.8h, v17.8h
    add             v26.8h, v6.8h, v18.8h
    add             v27.8h, v7.8h, v19.8h
    sqxtun          v4.8b, v24.8h
    sqxtun2         v4.16b, v25.8h
    sqxtun          v5.8b, v26.8h
    sqxtun2         v5.16b, v27.8h
    st1             {v4.16b}, [x0], x1
    st1             {v5.16b}, [x0], x1
.endr
    cbnz            w12, .loop_add_ps_16x\h
    ret
endfunc
.endm

pixel_add_ps_16xN_neon 16
pixel_add_ps_16xN_neon 32

.macro pixel_add_ps_32xN_neon h
 function PFX(pixel_add_ps_32x\h\()_neon)
    lsl             x5, x5, #1
    mov             w12, #\h / 4
.loop_add_ps_32x\h\():
    sub             w12, w12, #1
.rept 4
    ld1             {v0.16b-v1.16b}, [x2], x4
    ld1             {v16.8h-v19.8h}, [x3], x5
    uxtl            v4.8h, v0.8b
    uxtl2           v5.8h, v0.16b
    uxtl            v6.8h, v1.8b
    uxtl2           v7.8h, v1.16b
    add             v24.8h, v4.8h, v16.8h
    add             v25.8h, v5.8h, v17.8h
    add             v26.8h, v6.8h, v18.8h
    add             v27.8h, v7.8h, v19.8h
    sqxtun          v4.8b, v24.8h
    sqxtun2         v4.16b, v25.8h
    sqxtun          v5.8b, v26.8h
    sqxtun2         v5.16b, v27.8h
    st1             {v4.16b-v5.16b}, [x0], x1
.endr
    cbnz            w12, .loop_add_ps_32x\h
    ret
endfunc
.endm

pixel_add_ps_32xN_neon 32
pixel_add_ps_32xN_neon 64

function PFX(pixel_add_ps_64x64_neon)
    lsl             x5, x5, #1
    sub             x5, x5, #64
    mov             w12, #32
.loop_add_ps_64x64:
    sub             w12, w12, #1
.rept 2
    ld1             {v0.16b-v3.16b}, [x2], x4
    ld1             {v16.8h-v19.8h}, [x3], #64
    ld1             {v20.8h-v23.8h}, [x3], x5
    uxtl            v4.8h, v0.8b
    uxtl2           v5.8h, v0.16b
    uxtl            v6.8h, v1.8b
    uxtl2           v7.8h, v1.16b
    uxtl            v24.8h, v2.8b
    uxtl2           v25.8h, v2.16b
    uxtl            v26.8h, v3.8b
    uxtl2           v27.8h, v3.16b
    add             v0.8h, v4.8h, v16.8h
    add             v1.8h, v5.8h, v17.8h
    add             v2.8h, v6.8h, v18.8h
    add             v3.8h, v7.8h, v19.8h
    add             v4.8h, v24.8h, v20.8h
    add             v5.8h, v25.8h, v21.8h
    add             v6.8h, v26.8h, v22.8h
    add             v7.8h, v27.8h, v23.8h
    sqxtun          v0.8b, v0.8h
    sqxtun2         v0.16b, v1.8h
    sqxtun          v1.8b, v2.8h
    sqxtun2         v1.16b, v3.8h
    sqxtun          v2.8b, v4.8h
    sqxtun2         v2.16b, v5.8h
    sqxtun          v3.8b, v6.8h
    sqxtun2         v3.16b, v7.8h
    st1             {v0.16b-v3.16b}, [x0], x1
.endr
    cbnz            w12, .loop_add_ps_64x64
    ret
endfunc

// Chroma add_ps
function PFX(pixel_add_ps_4x8_neon)
    lsl             x5, x5, #1
.rept 4
    ld1             {v0.8b}, [x2], x4
    ld1             {v1.8b}, [x2], x4
    ld1             {v2.4h}, [x3], x5
    ld1             {v3.4h}, [x3], x5
    uxtl            v0.8h, v0.8b
    uxtl            v1.8h, v1.8b
    add             v4.8h, v0.8h, v2.8h
    add             v5.8h, v1.8h, v3.8h
    sqxtun          v4.8b, v4.8h
    sqxtun          v5.8b, v5.8h
    st1             {v4.s}[0], [x0], x1
    st1             {v5.s}[0], [x0], x1
.endr
    ret
endfunc

function PFX(pixel_add_ps_8x16_neon)
    lsl             x5, x5, #1
.rept 8
    ld1             {v0.8b}, [x2], x4
    ld1             {v1.8b}, [x2], x4
    ld1             {v2.8h}, [x3], x5
    ld1             {v3.8h}, [x3], x5
    uxtl            v0.8h, v0.8b
    uxtl            v1.8h, v1.8b
    add             v4.8h, v0.8h, v2.8h
    add             v5.8h, v1.8h, v3.8h
    sqxtun          v4.8b, v4.8h
    sqxtun          v5.8b, v5.8h
    st1             {v4.8b}, [x0], x1
    st1             {v5.8b}, [x0], x1
.endr
    ret
endfunc

// void scale1D_128to64(pixel *dst, const pixel *src)
function PFX(scale1D_128to64_neon)
.rept 2
    ld2             {v0.16b, v1.16b}, [x1], #32
    ld2             {v2.16b, v3.16b}, [x1], #32
    ld2             {v4.16b, v5.16b}, [x1], #32
    ld2             {v6.16b, v7.16b}, [x1], #32
    urhadd          v0.16b, v0.16b, v1.16b
    urhadd          v1.16b, v2.16b, v3.16b
    urhadd          v2.16b, v4.16b, v5.16b
    urhadd          v3.16b, v6.16b, v7.16b
    st1             {v0.16b-v3.16b}, [x0], #64
.endr
    ret
endfunc

.macro scale2D_1  v0, v1
    uaddlp          \v0\().8h, \v0\().16b
    uaddlp          \v1\().8h, \v1\().16b
    add             \v0\().8h, \v0\().8h, \v1\().8h
.endm

// void scale2D_64to32(pixel* dst, const pixel* src, intptr_t stride)
function PFX(scale2D_64to32_neon)
    mov             w12, #32
.loop_scale2D:
    ld1             {v0.16b-v3.16b}, [x1], x2
    sub             w12, w12, #1
    ld1             {v4.16b-v7.16b}, [x1], x2
    scale2D_1       v0, v4
    scale2D_1       v1, v5
    scale2D_1       v2, v6
    scale2D_1       v3, v7
    uqrshrn         v0.8b, v0.8h, #2
    uqrshrn2        v0.16b, v1.8h, #2
    uqrshrn         v1.8b, v2.8h, #2
    uqrshrn2        v1.16b, v3.8h, #2
    st1             {v0.16b-v1.16b}, [x0], #32
    cbnz            w12, .loop_scale2D
    ret
endfunc

// void planecopy_cp_c(const uint8_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int width, int height, int shift)
function PFX(pixel_planecopy_cp_neon)
    dup             v2.16b, w6
    sub             x5, x5, #1
.loop_h:
    mov             x6, x0
    mov             x12, x2
    mov             x7, #0
.loop_w:
    ldr             q0, [x6], #16
    ushl            v0.16b, v0.16b, v2.16b
    str             q0, [x12], #16
    add             x7, x7, #16
    cmp             x7, x4
    blt             .loop_w

    add             x0, x0, x1
    add             x2, x2, x3
    sub             x5, x5, #1
    cbnz            x5, .loop_h

// handle last row
    mov             x5, x4
    lsr             x5, x5, #3
.loopW8:
    ldr             d0, [x0], #8
    ushl            v0.8b, v0.8b, v2.8b
    str             d0, [x2], #8
    sub             x4, x4, #8
    sub             x5, x5, #1
    cbnz            x5, .loopW8

    mov             x5, #8
    sub             x5, x5, x4
    sub             x0, x0, x5
    sub             x2, x2, x5
    ldr             d0, [x0]
    ushl            v0.8b, v0.8b, v2.8b
    str             d0, [x2]
    ret
endfunc

//******* satd *******
.macro satd_4x4_neon
    ld1             {v0.s}[0], [x0], x1
    ld1             {v0.s}[1], [x0], x1
    ld1             {v1.s}[0], [x2], x3
    ld1             {v1.s}[1], [x2], x3
    ld1             {v2.s}[0], [x0], x1
    ld1             {v2.s}[1], [x0], x1
    ld1             {v3.s}[0], [x2], x3
    ld1             {v3.s}[1], [x2], x3

    usubl           v4.8h, v0.8b, v1.8b
    usubl           v5.8h, v2.8b, v3.8b

    add             v6.8h, v4.8h, v5.8h
    sub             v7.8h, v4.8h, v5.8h

    mov             v4.d[0], v6.d[1]
    add             v0.4h, v6.4h, v4.4h
    sub             v2.4h, v6.4h, v4.4h

    mov             v5.d[0], v7.d[1]
    add             v1.4h, v7.4h, v5.4h
    sub             v3.4h, v7.4h, v5.4h

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
function PFX(pixel_satd_4x4_neon)
    satd_4x4_neon
    fmov            x0, d0
    ret
endfunc

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
    ld1r            {v1.2s}, [x2], x3
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
    add             v6.8h, v2.8h, v3.8h
    sub             v7.8h, v2.8h, v3.8h
    x265_satd_4x8_8x4_end_neon
.endm

// template<int w, int h>
// int satd4(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2)
function PFX(pixel_satd_4x8_neon)
    pixel_satd_4x8_neon
    mov             w0, v0.s[0]
    ret
endfunc

function PFX(pixel_satd_4x16_neon)
    mov             w4, #0
    pixel_satd_4x8_neon
    mov             w5, v0.s[0]
    add             w4, w4, w5
    pixel_satd_4x8_neon
    mov             w5, v0.s[0]
    add             w0, w5, w4
    ret
endfunc

function PFX(pixel_satd_4x32_neon)
    mov             w4, #0
.rept 4
    pixel_satd_4x8_neon
    mov             w5, v0.s[0]
    add             w4, w4, w5
.endr
    mov             w0, w4
    ret
endfunc

function PFX(pixel_satd_12x16_neon)
    mov             x4, x0
    mov             x5, x2
    mov             w7, #0
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

function PFX(pixel_satd_12x32_neon)
    mov             x4, x0
    mov             x5, x2
    mov             w7, #0
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

function PFX(pixel_satd_8x4_neon)
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

.macro LOAD_DIFF_8x4 v0 v1 v2 v3
    ld1             {v0.8b}, [x0], x1
    ld1             {v1.8b}, [x2], x3
    ld1             {v2.8b}, [x0], x1
    ld1             {v3.8b}, [x2], x3
    ld1             {v4.8b}, [x0], x1
    ld1             {v5.8b}, [x2], x3
    ld1             {v6.8b}, [x0], x1
    ld1             {v7.8b}, [x2], x3
    usubl           \v0, v0.8b, v1.8b
    usubl           \v1, v2.8b, v3.8b
    usubl           \v2, v4.8b, v5.8b
    usubl           \v3, v6.8b, v7.8b
.endm

.macro LOAD_DIFF_16x4 v0 v1 v2 v3 v4 v5 v6 v7
    ld1             {v0.16b}, [x0], x1
    ld1             {v1.16b}, [x2], x3
    ld1             {v2.16b}, [x0], x1
    ld1             {v3.16b}, [x2], x3
    ld1             {v4.16b}, [x0], x1
    ld1             {v5.16b}, [x2], x3
    ld1             {v6.16b}, [x0], x1
    ld1             {v7.16b}, [x2], x3
    usubl           \v0, v0.8b, v1.8b
    usubl           \v1, v2.8b, v3.8b
    usubl           \v2, v4.8b, v5.8b
    usubl           \v3, v6.8b, v7.8b
    usubl2          \v4, v0.16b, v1.16b
    usubl2          \v5, v2.16b, v3.16b
    usubl2          \v6, v4.16b, v5.16b
    usubl2          \v7, v6.16b, v7.16b
.endm

function PFX(satd_16x4_neon), export=0
    LOAD_DIFF_16x4  v16.8h, v17.8h, v18.8h, v19.8h, v20.8h, v21.8h, v22.8h, v23.8h
    b               PFX(satd_8x4v_8x8h_neon)
endfunc

function PFX(satd_8x8_neon), export=0
    LOAD_DIFF_8x4   v16.8h, v17.8h, v18.8h, v19.8h
    LOAD_DIFF_8x4   v20.8h, v21.8h, v22.8h, v23.8h
    b               PFX(satd_8x4v_8x8h_neon)
endfunc

// one vertical hadamard pass and two horizontal
function PFX(satd_8x4v_8x8h_neon), export=0
    HADAMARD4_V     v16.8h, v18.8h, v17.8h, v19.8h, v0.8h, v2.8h, v1.8h, v3.8h
    HADAMARD4_V     v20.8h, v21.8h, v22.8h, v23.8h, v0.8h, v1.8h, v2.8h, v3.8h
    trn4            v0.8h, v1.8h, v2.8h, v3.8h, v16.8h, v17.8h, v18.8h, v19.8h
    trn4            v4.8h, v5.8h, v6.8h, v7.8h, v20.8h, v21.8h, v22.8h, v23.8h
    SUMSUB_ABCD     v16.8h, v17.8h, v18.8h, v19.8h, v0.8h, v1.8h, v2.8h, v3.8h
    SUMSUB_ABCD     v20.8h, v21.8h, v22.8h, v23.8h, v4.8h, v5.8h, v6.8h, v7.8h
    trn4            v0.4s, v2.4s, v1.4s, v3.4s, v16.4s, v18.4s, v17.4s, v19.4s
    trn4            v4.4s, v6.4s, v5.4s, v7.4s, v20.4s, v22.4s, v21.4s, v23.4s
    ABS8            v0.8h, v1.8h, v2.8h, v3.8h, v4.8h, v5.8h, v6.8h, v7.8h
    smax            v0.8h, v0.8h, v2.8h
    smax            v1.8h, v1.8h, v3.8h
    smax            v2.8h, v4.8h, v6.8h
    smax            v3.8h, v5.8h, v7.8h
    ret
endfunc

function PFX(pixel_satd_8x8_neon)
    mov             x10, x30
    bl              PFX(satd_8x8_neon)
    add             v0.8h, v0.8h, v1.8h
    add             v1.8h, v2.8h, v3.8h
    add             v0.8h, v0.8h, v1.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    ret             x10
endfunc

function PFX(pixel_satd_8x12_neon)
    mov             x4, x0
    mov             x5, x2
    mov             x7, #0
    satd_4x4_neon
    umov            x6, v0.d[0]
    add             x7, x7, x6
    add             x0, x4, #4
    add             x2, x5, #4
    satd_4x4_neon
    umov            x6, v0.d[0]
    add             x7, x7, x6
.rept 2
    sub             x0, x0, #4
    sub             x2, x2, #4
    mov             x4, x0
    mov             x5, x2
    satd_4x4_neon
    umov            x6, v0.d[0]
    add             x7, x7, x6
    add             x0, x4, #4
    add             x2, x5, #4
    satd_4x4_neon
    umov            x6, v0.d[0]
    add             x7, x7, x6
.endr
    mov             x0, x7
    ret
endfunc

function PFX(pixel_satd_8x16_neon)
    mov             x10, x30
    bl              PFX(satd_8x8_neon)
    add             v30.8h, v0.8h, v1.8h
    add             v31.8h, v2.8h, v3.8h
    bl              PFX(satd_8x8_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    ret             x10
endfunc

function PFX(pixel_satd_8x32_neon)
    mov             x10, x30
    bl              PFX(satd_8x8_neon)
    add             v30.8h, v0.8h, v1.8h
    add             v31.8h, v2.8h, v3.8h
.rept 3
    bl              PFX(satd_8x8_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
.endr
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    ret             x10
endfunc

function PFX(pixel_satd_8x64_neon)
    mov             x10, x30
    bl              PFX(satd_8x8_neon)
    add             v30.8h, v0.8h, v1.8h
    add             v31.8h, v2.8h, v3.8h
.rept 7
    bl              PFX(satd_8x8_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
.endr
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    ret             x10
endfunc

function PFX(pixel_satd_16x4_neon)
    mov             x10, x30
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v0.8h, v1.8h
    add             v31.8h, v2.8h, v3.8h
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    ret             x10
endfunc

function PFX(pixel_satd_16x8_neon)
    mov             x10, x30
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v0.8h, v1.8h
    add             v31.8h, v2.8h, v3.8h
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    ret             x10
endfunc

function PFX(pixel_satd_16x12_neon)
    mov             x10, x30
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v0.8h, v1.8h
    add             v31.8h, v2.8h, v3.8h
.rept 2
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
.endr
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    ret             x10
endfunc

function PFX(pixel_satd_16x16_neon)
    mov             x10, x30
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v0.8h, v1.8h
    add             v31.8h, v2.8h, v3.8h
.rept 3
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
.endr
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    ret             x10
endfunc

function PFX(pixel_satd_16x24_neon)
    mov             x10, x30
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v0.8h, v1.8h
    add             v31.8h, v2.8h, v3.8h
.rept 5
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
.endr
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    ret             x10
endfunc

.macro pixel_satd_16x32_neon
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v0.8h, v1.8h
    add             v31.8h, v2.8h, v3.8h
.rept 7
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
.endr
.endm

function PFX(pixel_satd_16x32_neon)
    mov             x10, x30
    pixel_satd_16x32_neon
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    ret             x10
endfunc

function PFX(pixel_satd_16x64_neon)
    mov             x10, x30
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v0.8h, v1.8h
    add             v31.8h, v2.8h, v3.8h
.rept 15
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
.endr
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    ret             x10
endfunc

function PFX(pixel_satd_24x32_neon)
    mov             x10, x30
    mov             x7, #0
    mov             x4, x0
    mov             x5, x2
.rept 3
    movi            v30.8h, #0
    movi            v31.8h, #0
.rept 4
    bl              PFX(satd_8x8_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
.endr
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w6, v0.s[0]
    add             x7, x7, x6
    add             x4, x4, #8
    add             x5, x5, #8
    mov             x0, x4
    mov             x2, x5
.endr
    mov             x0, x7
    ret             x10
endfunc

function PFX(pixel_satd_24x64_neon)
    mov             x10, x30
    mov             x7, #0
    mov             x4, x0
    mov             x5, x2
.rept 3
    movi            v30.8h, #0
    movi            v31.8h, #0
.rept 4
    bl              PFX(satd_8x8_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
.endr
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w6, v0.s[0]
    add             x7, x7, x6
    add             x4, x4, #8
    add             x5, x5, #8
    mov             x0, x4
    mov             x2, x5
.endr
    sub             x4, x4, #24
    sub             x5, x5, #24
    add             x0, x4, x1, lsl #5
    add             x2, x5, x3, lsl #5
    mov             x4, x0
    mov             x5, x2
.rept 3
    movi            v30.8h, #0
    movi            v31.8h, #0
.rept 4
    bl              PFX(satd_8x8_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
.endr
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w6, v0.s[0]
    add             x7, x7, x6
    add             x4, x4, #8
    add             x5, x5, #8
    mov             x0, x4
    mov             x2, x5
.endr
    mov             x0, x7
    ret             x10
endfunc

.macro pixel_satd_32x8
    mov             x4, x0
    mov             x5, x2
.rept 2
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
.endr
    add             x0, x4, #16
    add             x2, x5, #16
.rept 2
    bl              PFX(satd_16x4_neon)
    add             v30.8h, v30.8h, v0.8h
    add             v31.8h, v31.8h, v1.8h
    add             v30.8h, v30.8h, v2.8h
    add             v31.8h, v31.8h, v3.8h
.endr
.endm

.macro satd_32x16_neon
    movi            v30.8h, #0
    movi            v31.8h, #0
    pixel_satd_32x8
    sub             x0, x0, #16
    sub             x2, x2, #16
    pixel_satd_32x8
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w6, v0.s[0]
.endm

.macro satd_64x16_neon
    mov             x8, x0
    mov             x9, x2
    satd_32x16_neon
    add             x7, x7, x6
    add             x0, x8, #32
    add             x2, x9, #32
    satd_32x16_neon
    add             x7, x7, x6
.endm

function PFX(pixel_satd_32x8_neon)
    mov             x10, x30
    mov             x7, #0
    mov             x4, x0
    mov             x5, x2
    movi            v30.8h, #0
    movi            v31.8h, #0
    pixel_satd_32x8
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    ret             x10
endfunc

function PFX(pixel_satd_32x16_neon)
    mov             x10, x30
    satd_32x16_neon
    mov             x0, x6
    ret             x10
endfunc

function PFX(pixel_satd_32x24_neon)
    mov             x10, x30
    satd_32x16_neon
    movi            v30.8h, #0
    movi            v31.8h, #0
    sub             x0, x0, #16
    sub             x2, x2, #16
    pixel_satd_32x8
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    add             x0, x0, x6
    ret             x10
endfunc

function PFX(pixel_satd_32x32_neon)
    mov             x10, x30
    mov             x7, #0
    satd_32x16_neon
    sub             x0, x0, #16
    sub             x2, x2, #16
    add             x7, x7, x6
    satd_32x16_neon
    add             x0, x7, x6
    ret             x10
endfunc

function PFX(pixel_satd_32x48_neon)
    mov             x10, x30
    mov             x7, #0
.rept 2
    satd_32x16_neon
    sub             x0, x0, #16
    sub             x2, x2, #16
    add             x7, x7, x6
.endr
    satd_32x16_neon
    add             x0, x7, x6
    ret             x10
endfunc

function PFX(pixel_satd_32x64_neon)
    mov             x10, x30
    mov             x7, #0
.rept 3
    satd_32x16_neon
    sub             x0, x0, #16
    sub             x2, x2, #16
    add             x7, x7, x6
.endr
    satd_32x16_neon
    add             x0, x7, x6
    ret             x10
endfunc

function PFX(pixel_satd_64x16_neon)
    mov             x10, x30
    mov             x7, #0
    satd_64x16_neon
    mov             x0, x7
    ret             x10
endfunc

function PFX(pixel_satd_64x32_neon)
    mov             x10, x30
    mov             x7, #0
    satd_64x16_neon
    sub             x0, x0, #48
    sub             x2, x2, #48
    satd_64x16_neon
    mov             x0, x7
    ret             x10
endfunc

function PFX(pixel_satd_64x48_neon)
    mov             x10, x30
    mov             x7, #0
.rept 2
    satd_64x16_neon
    sub             x0, x0, #48
    sub             x2, x2, #48
.endr
    satd_64x16_neon
    mov             x0, x7
    ret             x10
endfunc

function PFX(pixel_satd_64x64_neon)
    mov             x10, x30
    mov             x7, #0
.rept 3
    satd_64x16_neon
    sub             x0, x0, #48
    sub             x2, x2, #48
.endr
    satd_64x16_neon
    mov             x0, x7
    ret             x10
endfunc

function PFX(pixel_satd_48x64_neon)
    mov             x10, x30
    mov             x7, #0
    mov             x8, x0
    mov             x9, x2
.rept 3
    satd_32x16_neon
    sub             x0, x0, #16
    sub             x2, x2, #16
    add             x7, x7, x6
.endr
    satd_32x16_neon
    add             x7, x7, x6

    add             x0, x8, #32
    add             x2, x9, #32
    pixel_satd_16x32_neon
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w6, v0.s[0]
    add             x7, x7, x6

    movi            v30.8h, #0
    movi            v31.8h, #0
    pixel_satd_16x32_neon
    add             v0.8h, v30.8h, v31.8h
    uaddlv          s0, v0.8h
    mov             w6, v0.s[0]
    add             x0, x7, x6
    ret             x10
endfunc

function PFX(sa8d_8x8_neon), export=0
    LOAD_DIFF_8x4   v16.8h, v17.8h, v18.8h, v19.8h
    LOAD_DIFF_8x4   v20.8h, v21.8h, v22.8h, v23.8h
    HADAMARD4_V     v16.8h, v18.8h, v17.8h, v19.8h, v0.8h, v2.8h, v1.8h, v3.8h
    HADAMARD4_V     v20.8h, v21.8h, v22.8h, v23.8h, v0.8h, v1.8h, v2.8h, v3.8h
    SUMSUB_ABCD     v0.8h, v16.8h, v1.8h, v17.8h, v16.8h, v20.8h, v17.8h, v21.8h
    SUMSUB_ABCD     v2.8h, v18.8h, v3.8h, v19.8h, v18.8h, v22.8h, v19.8h, v23.8h
    trn4            v4.8h, v5.8h, v6.8h, v7.8h, v0.8h, v1.8h, v2.8h, v3.8h
    trn4            v20.8h, v21.8h, v22.8h, v23.8h, v16.8h, v17.8h, v18.8h, v19.8h
    SUMSUB_ABCD     v2.8h, v3.8h, v24.8h, v25.8h, v20.8h, v21.8h, v4.8h, v5.8h
    SUMSUB_ABCD     v0.8h, v1.8h, v4.8h, v5.8h, v22.8h, v23.8h, v6.8h, v7.8h
    trn4            v20.4s, v22.4s, v21.4s, v23.4s, v2.4s, v0.4s, v3.4s, v1.4s
    trn4            v16.4s, v18.4s, v17.4s, v19.4s, v24.4s, v4.4s, v25.4s, v5.4s
    SUMSUB_ABCD     v0.8h, v2.8h, v1.8h, v3.8h, v20.8h, v22.8h, v21.8h, v23.8h
    SUMSUB_ABCD     v4.8h, v6.8h, v5.8h, v7.8h, v16.8h, v18.8h, v17.8h, v19.8h
    trn4            v16.2d, v20.2d, v17.2d, v21.2d, v0.2d, v4.2d, v1.2d, v5.2d
    trn4            v18.2d, v22.2d, v19.2d, v23.2d, v2.2d, v6.2d, v3.2d, v7.2d
    ABS8            v16.8h, v17.8h, v18.8h, v19.8h, v20.8h, v21.8h, v22.8h, v23.8h
    smax            v16.8h, v16.8h, v20.8h
    smax            v17.8h, v17.8h, v21.8h
    smax            v18.8h, v18.8h, v22.8h
    smax            v19.8h, v19.8h, v23.8h
    add             v0.8h, v16.8h, v17.8h
    add             v1.8h, v18.8h, v19.8h
    ret
endfunc

function PFX(pixel_sa8d_8x8_neon)
    mov             x10, x30
    bl              PFX(sa8d_8x8_neon)
    add             v0.8h, v0.8h, v1.8h
    uaddlv          s0, v0.8h
    mov             w0, v0.s[0]
    add             w0, w0, #1
    lsr             w0, w0, #1
    ret             x10
endfunc

function PFX(pixel_sa8d_8x16_neon)
    mov             x10, x30
    bl              PFX(sa8d_8x8_neon)
    add             v0.8h, v0.8h, v1.8h
    uaddlv          s0, v0.8h
    mov             w5, v0.s[0]
    add             w5, w5, #1
    lsr             w5, w5, #1
    bl              PFX(sa8d_8x8_neon)
    add             v0.8h, v0.8h, v1.8h
    uaddlv          s0, v0.8h
    mov             w4, v0.s[0]
    add             w4, w4, #1
    lsr             w4, w4, #1
    add             w0, w4, w5
    ret             x10
endfunc

.macro sa8d_16x16 reg
    bl              PFX(sa8d_8x8_neon)
    uaddlp          v30.4s, v0.8h
    uaddlp          v31.4s, v1.8h
    bl              PFX(sa8d_8x8_neon)
    uadalp          v30.4s, v0.8h
    uadalp          v31.4s, v1.8h
    sub             x0, x0, x1, lsl #4
    sub             x2, x2, x3, lsl #4
    add             x0, x0, #8
    add             x2, x2, #8
    bl              PFX(sa8d_8x8_neon)
    uadalp          v30.4s, v0.8h
    uadalp          v31.4s, v1.8h
    bl              PFX(sa8d_8x8_neon)
    uadalp          v30.4s, v0.8h
    uadalp          v31.4s, v1.8h
    add             v0.4s, v30.4s, v31.4s
    addv            s0, v0.4s
    mov             \reg, v0.s[0]
    add             \reg, \reg, #1
    lsr             \reg, \reg, #1
.endm

function PFX(pixel_sa8d_16x16_neon)
    mov             x10, x30
    sa8d_16x16      w0
    ret             x10
endfunc

function PFX(pixel_sa8d_16x32_neon)
    mov             x10, x30
    sa8d_16x16      w4
    sub             x0, x0, #8
    sub             x2, x2, #8
    sa8d_16x16      w5
    add             w0, w4, w5
    ret             x10
endfunc

function PFX(pixel_sa8d_32x32_neon)
    mov             x10, x30
    sa8d_16x16      w4
    sub             x0, x0, x1, lsl #4
    sub             x2, x2, x3, lsl #4
    add             x0, x0, #8
    add             x2, x2, #8
    sa8d_16x16      w5
    sub             x0, x0, #24
    sub             x2, x2, #24
    sa8d_16x16      w6
    sub             x0, x0, x1, lsl #4
    sub             x2, x2, x3, lsl #4
    add             x0, x0, #8
    add             x2, x2, #8
    sa8d_16x16      w7
    add             w4, w4, w5
    add             w6, w6, w7
    add             w0, w4, w6
    ret             x10
endfunc

function PFX(pixel_sa8d_32x64_neon)
    mov             x10, x30
    mov             w11, #4
    mov             w9, #0
.loop_sa8d_32:
    sub             w11, w11, #1
    sa8d_16x16      w4
    sub             x0, x0, x1, lsl #4
    sub             x2, x2, x3, lsl #4
    add             x0, x0, #8
    add             x2, x2, #8
    sa8d_16x16      w5
    add             w4, w4, w5
    add             w9, w9, w4
    sub             x0, x0, #24
    sub             x2, x2, #24
    cbnz            w11, .loop_sa8d_32
    mov             w0, w9
    ret             x10
endfunc

function PFX(pixel_sa8d_64x64_neon)
    mov             x10, x30
    mov             w11, #4
    mov             w9, #0
.loop_sa8d_64:
    sub             w11, w11, #1
    sa8d_16x16      w4
    sub             x0, x0, x1, lsl #4
    sub             x2, x2, x3, lsl #4
    add             x0, x0, #8
    add             x2, x2, #8
    sa8d_16x16      w5
    sub             x0, x0, x1, lsl #4
    sub             x2, x2, x3, lsl #4
    add             x0, x0, #8
    add             x2, x2, #8
    sa8d_16x16      w6
    sub             x0, x0, x1, lsl #4
    sub             x2, x2, x3, lsl #4
    add             x0, x0, #8
    add             x2, x2, #8
    sa8d_16x16      w7
    add             w4, w4, w5
    add             w6, w6, w7
    add             w8, w4, w6
    add             w9, w9, w8

    sub             x0, x0, #56
    sub             x2, x2, #56
    cbnz            w11, .loop_sa8d_64
    mov             w0, w9
    ret             x10
endfunc

/***** dequant_scaling*****/
// void dequant_scaling_c(const int16_t* quantCoef, const int32_t* deQuantCoef, int16_t* coef, int num, int per, int shift)
function PFX(dequant_scaling_neon)
    add             x5, x5, #4              // shift + 4
    lsr             x3, x3, #3              // num / 8
    cmp             x5, x4
    blt             .dequant_skip

    mov             x12, #1
    sub             x6, x5, x4          // shift - per
    sub             x6, x6, #1          // shift - per - 1
    lsl             x6, x12, x6         // 1 << shift - per - 1 (add)
    dup             v0.4s, w6
    sub             x7, x4, x5          // per - shift
    dup             v3.4s, w7

.dequant_loop1:
    ld1             {v19.8h}, [x0], #16 // quantCoef
    ld1             {v2.4s}, [x1], #16  // deQuantCoef
    ld1             {v20.4s}, [x1], #16
    sub             x3, x3, #1
    sxtl            v1.4s, v19.4h
    sxtl2           v19.4s, v19.8h

    mul             v1.4s, v1.4s, v2.4s // quantCoef * deQuantCoef
    mul             v19.4s, v19.4s, v20.4s
    add             v1.4s, v1.4s, v0.4s // quantCoef * deQuantCoef + add
    add             v19.4s, v19.4s, v0.4s

    sshl            v1.4s, v1.4s, v3.4s
    sshl            v19.4s, v19.4s, v3.4s
    sqxtn           v16.4h, v1.4s       // x265_clip3
    sqxtn2          v16.8h, v19.4s
    st1             {v16.8h}, [x2], #16
    cbnz            x3, .dequant_loop1
    ret

.dequant_skip:
    sub             x6, x4, x5          // per - shift
    dup             v0.8h, w6

.dequant_loop2:
    ld1             {v19.8h}, [x0], #16 // quantCoef
    ld1             {v2.4s}, [x1], #16  // deQuantCoef
    ld1             {v20.4s}, [x1], #16
    sub             x3, x3, #1
    sxtl            v1.4s, v19.4h
    sxtl2           v19.4s, v19.8h

    mul             v1.4s, v1.4s, v2.4s // quantCoef * deQuantCoef
    mul             v19.4s, v19.4s, v20.4s
    sqxtn           v16.4h, v1.4s       // x265_clip3
    sqxtn2          v16.8h, v19.4s

    sqshl           v16.8h, v16.8h, v0.8h // coefQ << per - shift
    st1             {v16.8h}, [x2], #16
    cbnz            x3, .dequant_loop2
    ret
endfunc

// void dequant_normal_c(const int16_t* quantCoef, int16_t* coef, int num, int scale, int shift)
function PFX(dequant_normal_neon)
    lsr             w2, w2, #4              // num / 16
    neg             w4, w4
    dup             v0.8h, w3
    dup             v1.4s, w4

.dqn_loop1:
    ld1             {v2.8h, v3.8h}, [x0], #32
    smull           v16.4s, v2.4h, v0.4h
    smull2          v17.4s, v2.8h, v0.8h
    smull           v18.4s, v3.4h, v0.4h
    smull2          v19.4s, v3.8h, v0.8h

    srshl           v16.4s, v16.4s, v1.4s
    srshl           v17.4s, v17.4s, v1.4s
    srshl           v18.4s, v18.4s, v1.4s
    srshl           v19.4s, v19.4s, v1.4s

    sqxtn           v2.4h, v16.4s
    sqxtn2          v2.8h, v17.4s
    sqxtn           v3.4h, v18.4s
    sqxtn2          v3.8h, v19.4s

    sub             w2, w2, #1
    st1             {v2.8h, v3.8h}, [x1], #32
    cbnz            w2, .dqn_loop1
    ret
endfunc

/********* ssim ***********/
// void ssim_4x4x2_core(const pixel* pix1, intptr_t stride1, const pixel* pix2, intptr_t stride2, int sums[2][4])
function PFX(ssim_4x4x2_core_neon)
    ld1             {v0.8b}, [x0], x1
    ld1             {v1.8b}, [x0], x1
    ld1             {v2.8b}, [x0], x1
    ld1             {v3.8b}, [x0], x1

    ld1             {v4.8b}, [x2], x3
    ld1             {v5.8b}, [x2], x3
    ld1             {v6.8b}, [x2], x3
    ld1             {v7.8b}, [x2], x3

    umull           v16.8h, v0.8b, v0.8b
    umull           v17.8h, v1.8b, v1.8b
    umull           v18.8h, v2.8b, v2.8b
    uaddlp          v30.4s, v16.8h
    umull           v19.8h, v3.8b, v3.8b
    umull           v20.8h, v4.8b, v4.8b
    umull           v21.8h, v5.8b, v5.8b
    uadalp          v30.4s, v17.8h
    umull           v22.8h, v6.8b, v6.8b
    umull           v23.8h, v7.8b, v7.8b

    umull           v24.8h, v0.8b, v4.8b
    uadalp          v30.4s, v18.8h
    umull           v25.8h, v1.8b, v5.8b
    umull           v26.8h, v2.8b, v6.8b
    umull           v27.8h, v3.8b, v7.8b
    uadalp          v30.4s, v19.8h

    uaddl           v28.8h, v0.8b, v1.8b
    uaddl           v29.8h, v4.8b, v5.8b
    uadalp          v30.4s, v20.8h
    uaddlp          v31.4s, v24.8h

    uaddw           v28.8h, v28.8h, v2.8b
    uaddw           v29.8h, v29.8h, v6.8b
    uadalp          v30.4s, v21.8h
    uadalp          v31.4s, v25.8h

    uaddw           v28.8h, v28.8h, v3.8b
    uaddw           v29.8h, v29.8h, v7.8b
    uadalp          v30.4s, v22.8h
    uadalp          v31.4s, v26.8h

    uaddlp          v28.4s, v28.8h
    uaddlp          v29.4s, v29.8h
    uadalp          v30.4s, v23.8h
    uadalp          v31.4s, v27.8h

    addp            v28.4s, v28.4s, v28.4s
    addp            v29.4s, v29.4s, v29.4s
    addp            v30.4s, v30.4s, v30.4s
    addp            v31.4s, v31.4s, v31.4s

    st4             {v28.2s, v29.2s, v30.2s, v31.2s}, [x4]
    ret
endfunc

// int psyCost_pp(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride)
function PFX(psyCost_4x4_neon)
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
function PFX(quant_neon)
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

// uint32_t nquant_c(const int16_t* coef, const int32_t* quantCoeff, int16_t* qCoef, int qBits, int add, int numCoeff)
function PFX(nquant_neon)
    neg             x12, x3
    dup             v0.4s, w12             // q0= -qbits
    dup             v1.4s, w4              // add

    lsr             w5, w5, #2
    movi            v4.4s, #0              // v4= accumulate numsig
    mov             x4, #0
    movi            v22.4s, #0

.loop_nquant:
    ld1             {v16.4h}, [x0], #8
    sub             w5, w5, #1
    sxtl            v19.4s, v16.4h         // v19 = coef[blockpos]

    cmlt            v18.4s, v19.4s, #0     // v18 = sign

    abs             v19.4s, v19.4s         // v19 = level=abs(coef[blockpos])
    ld1             {v20.4s}, [x1], #16    // v20 = quantCoeff[blockpos]
    mul             v19.4s, v19.4s, v20.4s // v19 = tmplevel = abs(level) * quantCoeff[blockpos];

    add             v20.4s, v19.4s, v1.4s  // v20 = tmplevel+add
    sshl            v20.4s, v20.4s, v0.4s  // v20 = level =(tmplevel+add) >> qbits

    // numsig
    cmeq            v21.4s, v20.4s, v22.4s
    add             v4.4s, v4.4s, v21.4s
    add             x4, x4, #4

    eor             v21.16b, v20.16b, v18.16b
    sub             v21.4s, v21.4s, v18.4s
    sqxtn           v16.4h, v21.4s
    abs             v17.4h, v16.4h
    st1             {v17.4h}, [x2], #8

    cbnz            w5, .loop_nquant

    uaddlv          d4, v4.4s
    fmov            x12, d4
    add             x0, x4, x12
    ret
endfunc

// void ssimDist_c(const pixel* fenc, uint32_t fStride, const pixel* recon, intptr_t rstride, uint64_t *ssBlock, int shift, uint64_t *ac_k)
.macro ssimDist_start
    movi            v0.16b, #0
    movi            v1.16b, #0
.endm

.macro ssimDist_1  v4 v5
    sub             v20.8h, \v4\().8h, \v5\().8h
    smull           v16.4s, \v4\().4h, \v4\().4h
    smull2          v17.4s, \v4\().8h, \v4\().8h
    smull           v18.4s, v20.4h, v20.4h
    smull2          v19.4s, v20.8h, v20.8h
    add             v0.4s, v0.4s, v16.4s
    add             v0.4s, v0.4s, v17.4s
    add             v1.4s, v1.4s, v18.4s
    add             v1.4s, v1.4s, v19.4s
.endm

.macro ssimDist_end
    uaddlv          d0, v0.4s
    uaddlv          d1, v1.4s
    str             d0, [x6]
    str             d1, [x4]
.endm

function PFX(ssimDist4_neon)
    ssimDist_start
.rept 4
    ld1             {v4.s}[0], [x0], x1
    ld1             {v5.s}[0], [x2], x3
    uxtl            v4.8h, v4.8b
    uxtl            v5.8h, v5.8b
    sub             v2.4h, v4.4h, v5.4h
    smull           v3.4s, v4.4h, v4.4h
    smull           v2.4s, v2.4h, v2.4h
    add             v0.4s, v0.4s, v3.4s
    add             v1.4s, v1.4s, v2.4s
.endr
    ssimDist_end
    ret
endfunc

function PFX(ssimDist8_neon)
    ssimDist_start
.rept 8
    ld1             {v4.8b}, [x0], x1
    ld1             {v5.8b}, [x2], x3
    uxtl            v4.8h, v4.8b
    uxtl            v5.8h, v5.8b
    ssimDist_1      v4, v5
.endr
    ssimDist_end
    ret
endfunc

function PFX(ssimDist16_neon)
    mov w12, #16
    ssimDist_start
.loop_ssimDist16:
    sub             w12, w12, #1
    ld1             {v4.16b}, [x0], x1
    ld1             {v5.16b}, [x2], x3
    uxtl            v6.8h, v4.8b
    uxtl            v7.8h, v5.8b
    uxtl2           v4.8h, v4.16b
    uxtl2           v5.8h, v5.16b
    ssimDist_1      v6, v7
    ssimDist_1      v4, v5
    cbnz            w12, .loop_ssimDist16
    ssimDist_end
    ret
endfunc

function PFX(ssimDist32_neon)
    mov w12, #32
    ssimDist_start
.loop_ssimDist32:
    sub             w12, w12, #1
    ld1             {v4.16b-v5.16b}, [x0], x1
    ld1             {v6.16b-v7.16b}, [x2], x3
    uxtl            v21.8h, v4.8b
    uxtl            v22.8h, v6.8b
    uxtl            v23.8h, v5.8b
    uxtl            v24.8h, v7.8b
    uxtl2           v25.8h, v4.16b
    uxtl2           v26.8h, v6.16b
    uxtl2           v27.8h, v5.16b
    uxtl2           v28.8h, v7.16b
    ssimDist_1      v21, v22
    ssimDist_1      v23, v24
    ssimDist_1      v25, v26
    ssimDist_1      v27, v28
    cbnz            w12, .loop_ssimDist32
    ssimDist_end
    ret
endfunc

function PFX(ssimDist64_neon)
    mov w12, #64
    ssimDist_start
.loop_ssimDist64:
    sub             w12, w12, #1
    ld1             {v4.16b-v7.16b}, [x0], x1
    ld1             {v16.16b-v19.16b}, [x2], x3
    uxtl            v21.8h, v4.8b
    uxtl            v22.8h, v16.8b
    uxtl            v23.8h, v5.8b
    uxtl            v24.8h, v17.8b
    uxtl2           v25.8h, v4.16b
    uxtl2           v26.8h, v16.16b
    uxtl2           v27.8h, v5.16b
    uxtl2           v28.8h, v17.16b
    ssimDist_1      v21, v22
    ssimDist_1      v23, v24
    ssimDist_1      v25, v26
    ssimDist_1      v27, v28
    uxtl            v21.8h, v6.8b
    uxtl            v22.8h, v18.8b
    uxtl            v23.8h, v7.8b
    uxtl            v24.8h, v19.8b
    uxtl2           v25.8h, v6.16b
    uxtl2           v26.8h, v18.16b
    uxtl2           v27.8h, v7.16b
    uxtl2           v28.8h, v19.16b
    ssimDist_1      v21, v22
    ssimDist_1      v23, v24
    ssimDist_1      v25, v26
    ssimDist_1      v27, v28
    cbnz            w12, .loop_ssimDist64
    ssimDist_end
    ret
endfunc

// void normFact_c(const pixel* src, uint32_t blockSize, int shift, uint64_t *z_k)

.macro normFact_start
    movi            v0.16b, #0
.endm

.macro normFact_1  v4
    smull           v16.4s, \v4\().4h, \v4\().4h
    smull2          v17.4s, \v4\().8h, \v4\().8h
    add             v0.4s, v0.4s, v16.4s
    add             v0.4s, v0.4s, v17.4s
.endm

.macro normFact_end
    uaddlv          d0, v0.4s
    str             d0, [x3]
.endm


function PFX(normFact8_neon)
    normFact_start
.rept 8
    ld1             {v4.8b}, [x0], x1
    uxtl            v4.8h, v4.8b
    normFact_1      v4
.endr
    normFact_end
    ret
endfunc

function PFX(normFact16_neon)
    mov w12, #16
    normFact_start
.loop_normFact16:
    sub             w12, w12, #1
    ld1             {v4.16b}, [x0], x1
    uxtl            v5.8h, v4.8b
    uxtl2           v4.8h, v4.16b
    normFact_1      v5
    normFact_1      v4
    cbnz            w12, .loop_normFact16
    normFact_end
    ret
endfunc

function PFX(normFact32_neon)
    mov w12, #32
    normFact_start
.loop_normFact32:
    sub             w12, w12, #1
    ld1             {v4.16b-v5.16b}, [x0], x1
    uxtl            v6.8h, v4.8b
    uxtl2           v4.8h, v4.16b
    uxtl            v7.8h, v5.8b
    uxtl2           v5.8h, v5.16b
    normFact_1      v4
    normFact_1      v5
    normFact_1      v6
    normFact_1      v7
    cbnz            w12, .loop_normFact32
    normFact_end
    ret
endfunc

function PFX(normFact64_neon)
    mov w12, #64
    normFact_start
.loop_normFact64:
    sub             w12, w12, #1
    ld1             {v4.16b-v7.16b}, [x0], x1
    uxtl            v26.8h, v4.8b
    uxtl2           v24.8h, v4.16b
    uxtl            v27.8h, v5.8b
    uxtl2           v25.8h, v5.16b
    normFact_1      v24
    normFact_1      v25
    normFact_1      v26
    normFact_1      v27
    uxtl            v26.8h, v6.8b
    uxtl2           v24.8h, v6.16b
    uxtl            v27.8h, v7.8b
    uxtl2           v25.8h, v7.16b
    normFact_1      v24
    normFact_1      v25
    normFact_1      v26
    normFact_1      v27
    cbnz            w12, .loop_normFact64
    normFact_end
    ret
endfunc

// void weight_pp_c(const pixel* src, pixel* dst, intptr_t stride, int width, int height, int w0, int round, int shift, int offset)
function PFX(weight_pp_neon)
    sub             x2, x2, x3
    ldr             w9, [sp]              // offset
    lsl             w5, w5, #6            // w0 << correction

    // count trailing zeros in w5 and compare against shift right amount.
    rbit            w10, w5
    clz             w10, w10
    cmp             w10, w7
    b.lt            .unfoldedShift

    // shift right only removes trailing zeros: hoist LSR out of the loop.
    lsr             w10, w5, w7           // w0 << correction >> shift
    dup             v25.16b, w10
    lsr             w6, w6, w7            // round >> shift
    add             w6, w6, w9            // round >> shift + offset
    dup             v26.8h, w6

    // Check arithmetic range.
    mov             w11, #255
    madd            w11, w11, w10, w6
    add             w11, w11, w9
    lsr             w11, w11, #16
    cbnz            w11, .widenTo32Bit

    // 16-bit arithmetic is enough.
.loopHpp:
    mov             x12, x3
.loopWpp:
    ldr             q0, [x0], #16
    sub             x12, x12, #16
    umull           v1.8h, v0.8b, v25.8b  // val *= w0 << correction >> shift
    umull2          v2.8h, v0.16b, v25.16b
    add             v1.8h, v1.8h, v26.8h  // val += round >> shift + offset
    add             v2.8h, v2.8h, v26.8h
    sqxtun          v0.8b, v1.8h          // val = x265_clip(val)
    sqxtun2         v0.16b, v2.8h
    str             q0, [x1], #16
    cbnz            x12, .loopWpp
    add             x1, x1, x2
    add             x0, x0, x2
    sub             x4, x4, #1
    cbnz            x4, .loopHpp
    ret

    // 32-bit arithmetic is needed.
.widenTo32Bit:
.loopHpp32:
    mov             x12, x3
.loopWpp32:
    ldr             d0, [x0], #8
    sub             x12, x12, #8
    uxtl            v0.8h, v0.8b
    umull           v1.4s, v0.4h, v25.4h  // val *= w0 << correction >> shift
    umull2          v2.4s, v0.8h, v25.8h
    add             v1.4s, v1.4s, v26.4s  // val += round >> shift + offset
    add             v2.4s, v2.4s, v26.4s
    sqxtn           v0.4h, v1.4s          // val = x265_clip(val)
    sqxtn2          v0.8h, v2.4s
    sqxtun          v0.8b, v0.8h
    str             d0, [x1], #8
    cbnz            x12, .loopWpp32
    add             x1, x1, x2
    add             x0, x0, x2
    sub             x4, x4, #1
    cbnz            x4, .loopHpp32
    ret

    // The shift right cannot be moved out of the loop.
.unfoldedShift:
    dup             v25.8h, w5            // w0 << correction
    dup             v26.4s, w6            // round
    neg             w7, w7                // -shift
    dup             v27.4s, w7
    dup             v29.4s, w9            // offset
.loopHppUS:
    mov             x12, x3
.loopWppUS:
    ldr             d0, [x0], #8
    sub             x12, x12, #8
    uxtl            v0.8h, v0.8b
    umull           v1.4s, v0.4h, v25.4h  // val *= w0
    umull2          v2.4s, v0.8h, v25.8h
    add             v1.4s, v1.4s, v26.4s  // val += round
    add             v2.4s, v2.4s, v26.4s
    sshl            v1.4s, v1.4s, v27.4s  // val >>= shift
    sshl            v2.4s, v2.4s, v27.4s
    add             v1.4s, v1.4s, v29.4s  // val += offset
    add             v2.4s, v2.4s, v29.4s
    sqxtn           v0.4h, v1.4s          // val = x265_clip(val)
    sqxtn2          v0.8h, v2.4s
    sqxtun          v0.8b, v0.8h
    str             d0, [x1], #8
    cbnz            x12, .loopWppUS
    add             x1, x1, x2
    add             x0, x0, x2
    sub             x4, x4, #1
    cbnz            x4, .loopHppUS
    ret
endfunc

// int scanPosLast(
//     const uint16_t *scan,      // x0
//     const coeff_t *coeff,      // x1
//     uint16_t *coeffSign,       // x2
//     uint16_t *coeffFlag,       // x3
//     uint8_t *coeffNum,         // x4
//     int numSig,                // x5
//     const uint16_t* scanCG4x4, // x6
//     const int trSize)          // x7
function PFX(scanPosLast_neon)
    // convert unit of Stride(trSize) to int16_t
    add             x7, x7, x7

    // load scan table and convert to Byte
    ldp             q0, q1, [x6]
    xtn             v0.8b, v0.8h
    xtn2            v0.16b, v1.8h   // v0 - Zigzag scan table

    movrel          x10, g_SPL_and_mask
    ldr             q28, [x10]      // v28 = mask for pmovmskb
    movi            v31.16b, #0     // v31 = {0, ..., 0}
    add             x10, x7, x7     // 2*x7
    add             x11, x10, x7    // 3*x7
    add             x9, x4, #1      // CG count

.loop_spl:
    // position of current CG
    ldrh            w6, [x0], #32
    add             x6, x1, x6, lsl #1

    // loading current CG
    ldr             d2, [x6]
    ldr             d3, [x6, x7]
    ldr             d4, [x6, x10]
    ldr             d5, [x6, x11]
    mov             v2.d[1], v3.d[0]
    mov             v4.d[1], v5.d[0]
    sqxtn           v2.8b, v2.8h
    sqxtn2          v2.16b, v4.8h

    // Zigzag
    tbl             v3.16b, {v2.16b}, v0.16b

    // get sign
    cmhi            v5.16b, v3.16b, v31.16b   // v5 = non-zero
    cmlt            v3.16b, v3.16b, #0        // v3 = negative

    // val - w13 = pmovmskb(v3)
    and             v3.16b, v3.16b, v28.16b
    mov             d4, v3.d[1]
    addv            b23, v3.8b
    addv            b24, v4.8b
    mov             v23.b[1], v24.b[0]
    fmov            w13, s23

    // mask - w15 = pmovmskb(v5)
    and             v5.16b, v5.16b, v28.16b
    mov             d6, v5.d[1]
    addv            b25, v5.8b
    addv            b26, v6.8b
    mov             v25.b[1], v26.b[0]
    fmov            w15, s25

    // coeffFlag = reverse_bit(w15) in 16-bit
    rbit            w12, w15
    lsr             w12, w12, #16
    fmov            s30, w12
    strh            w12, [x3], #2

    // accelerate by preparing w13 = w13 & w15
    and             w13, w13, w15
    mov             x14, xzr
.loop_spl_1:
    cbz             w15, .pext_end
    clz             w6, w15
    lsl             w13, w13, w6
    lsl             w15, w15, w6
    extr            w14, w14, w13, #31
    bfm             w15, wzr, #1, #0
    b               .loop_spl_1
.pext_end:
    strh            w14, [x2], #2

    // compute coeffNum = popcount(coeffFlag)
    cnt             v30.8b, v30.8b
    addp            v30.8b, v30.8b, v30.8b
    fmov            w6, s30
    sub             x5, x5, x6
    strb            w6, [x4], #1

    cbnz            x5, .loop_spl

    // count trailing zeros
    rbit            w13, w12
    clz             w13, w13
    lsr             w12, w12, w13
    strh            w12, [x3, #-2]

    // get last pos
    sub             x9, x4, x9
    lsl             x0, x9, #4
    eor             w13, w13, #15
    add             x0, x0, x13
    ret
endfunc

// uint32_t costCoeffNxN(
//    uint16_t *scan,        // x0
//    coeff_t *coeff,        // x1
//    intptr_t trSize,       // x2
//    uint16_t *absCoeff,    // x3
//    uint8_t *tabSigCtx,    // x4
//    uint16_t scanFlagMask, // x5
//    uint8_t *baseCtx,      // x6
//    int offset,            // x7
//    int scanPosSigOff,     // sp
//    int subPosBase)        // sp + 8
function PFX(costCoeffNxN_neon)
    // abs(coeff)
    add             x2, x2, x2
    ld1             {v1.d}[0], [x1], x2
    ld1             {v1.d}[1], [x1], x2
    ld1             {v2.d}[0], [x1], x2
    ld1             {v2.d}[1], [x1], x2
    abs             v1.8h, v1.8h
    abs             v2.8h, v2.8h

    // WARNING: beyond-bound read here!
    // loading scan table
    ldr             w2, [sp]
    eor             w15, w2, #15
    add             x1, x0, x15, lsl #1
    ldp             q20, q21, [x1]
    uzp1            v20.16b, v20.16b, v21.16b
    movi            v21.16b, #15
    eor             v0.16b, v20.16b, v21.16b

    // reorder coeff
    uzp1           v22.16b, v1.16b, v2.16b
    uzp2           v23.16b, v1.16b, v2.16b
    tbl            v24.16b, {v22.16b}, v0.16b
    tbl            v25.16b, {v23.16b}, v0.16b
    zip1           v2.16b, v24.16b, v25.16b
    zip2           v3.16b, v24.16b, v25.16b

    // loading tabSigCtx (+offset)
    ldr             q1, [x4]
    tbl             v1.16b, {v1.16b}, v0.16b
    dup             v4.16b, w7
    movi            v5.16b, #0
    tbl             v4.16b, {v4.16b}, v5.16b
    add             v1.16b, v1.16b, v4.16b

    // register mapping
    // x0 - sum
    // x1 - entropyStateBits
    // v1 - sigCtx
    // {v3,v2} - abs(coeff)
    // x2 - scanPosSigOff
    // x3 - absCoeff
    // x4 - numNonZero
    // x5 - scanFlagMask
    // x6 - baseCtx
    mov             x0, #0
    movrel          x1, PFX_C(entropyStateBits)
    mov             x4, #0
    mov             x11, #0
    movi            v31.16b, #0
    cbz             x2, .idx_zero
.loop_ccnn:
//   {
//        const uint32_t cnt = tabSigCtx[blkPos] + offset + posOffset;
//        ctxSig = cnt & posZeroMask;
//        const uint32_t mstate = baseCtx[ctxSig];
//        const uint32_t mps = mstate & 1;
//        const uint32_t stateBits = x265_entropyStateBits[mstate ^ sig];
//        uint32_t nextState = (stateBits >> 24) + mps;
//        if ((mstate ^ sig) == 1)
//            nextState = sig;
//        baseCtx[ctxSig] = (uint8_t)nextState;
//        sum += stateBits;
//    }
//    absCoeff[numNonZero] = tmpCoeff[blkPos];
//    numNonZero += sig;
//    scanPosSigOff--;

    add             x13, x3, x4, lsl #1
    sub             x2, x2, #1
    str             h2, [x13]             // absCoeff[numNonZero] = tmpCoeff[blkPos]
    fmov            w14, s1               // x14 = ctxSig
    uxtb            w14, w14
    ubfx            w11, w5, #0, #1       // x11 = sig
    lsr             x5, x5, #1
    add             x4, x4, x11           // numNonZero += sig
    ext             v1.16b, v1.16b, v31.16b, #1
    ext             v2.16b, v2.16b, v3.16b, #2
    ext             v3.16b, v3.16b, v31.16b, #2
    ldrb            w9, [x6, x14]         // mstate = baseCtx[ctxSig]
    and             w10, w9, #1           // mps = mstate & 1
    eor             w9, w9, w11           // x9 = mstate ^ sig
    add             x12, x1, x9, lsl #2
    ldr             w13, [x12]
    add             w0, w0, w13           // sum += x265_entropyStateBits[mstate ^ sig]
    ldrb            w13, [x12, #3]
    add             w10, w10, w13         // nextState = (stateBits >> 24) + mps
    cmp             w9, #1
    csel            w10, w11, w10, eq
    strb            w10, [x6, x14]
    cbnz            x2, .loop_ccnn
.idx_zero:

    add             x13, x3, x4, lsl #1
    add             x4, x4, x15
    str             h2, [x13]              // absCoeff[numNonZero] = tmpCoeff[blkPos]

    ldr             x9, [sp, #8]           // subPosBase
    uxth            w9, w9
    cmp             w9, #0
    cset            x2, eq
    add             x4, x4, x2
    cbz             x4, .exit_ccnn

    sub             w2, w2, #1
    uxtb            w2, w2
    fmov            w3, s1
    and             w2, w2, w3

    ldrb            w3, [x6, x2]         // mstate = baseCtx[ctxSig]
    eor             w4, w5, w3            // x5 = mstate ^ sig
    and             w3, w3, #1            // mps = mstate & 1
    add             x1, x1, x4, lsl #2
    ldr             w11, [x1]
    ldrb            w12, [x1, #3]
    add             w0, w0, w11           // sum += x265_entropyStateBits[mstate ^ sig]
    add             w3, w3, w12           // nextState = (stateBits >> 24) + mps
    cmp             w4, #1
    csel            w3, w5, w3, eq
    strb            w3, [x6, x2]
.exit_ccnn:
    ubfx            w0, w0, #0, #24
    ret
endfunc

const g_SPL_and_mask, align=8
.byte 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80, 0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
endconst
