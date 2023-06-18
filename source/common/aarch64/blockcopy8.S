/*****************************************************************************
 * Copyright (C) 2021 MulticoreWare, Inc
 *
 * Authors: Sebastian Pop <spop@amazon.com>
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

#include "asm.S"

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

/* void blockcopy_sp(pixel* a, intptr_t stridea, const int16_t* b, intptr_t strideb)
 *
 * r0   - a
 * r1   - stridea
 * r2   - b
 * r3   - strideb */
function PFX(blockcopy_sp_4x4_neon)
    lsl             x3, x3, #1
.rept 2
    ld1             {v0.8h}, [x2], x3
    ld1             {v1.8h}, [x2], x3
    xtn             v0.8b, v0.8h
    xtn             v1.8b, v1.8h
    st1             {v0.s}[0], [x0], x1
    st1             {v1.s}[0], [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_sp_8x8_neon)
    lsl             x3, x3, #1
.rept 4
    ld1             {v0.8h}, [x2], x3
    ld1             {v1.8h}, [x2], x3
    xtn             v0.8b, v0.8h
    xtn             v1.8b, v1.8h
    st1             {v0.d}[0], [x0], x1
    st1             {v1.d}[0], [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_sp_16x16_neon)
    lsl             x3, x3, #1
    movrel          x11, xtn_xtn2_table
    ld1             {v31.16b}, [x11]
.rept 8
    ld1             {v0.8h-v1.8h}, [x2], x3
    ld1             {v2.8h-v3.8h}, [x2], x3
    tbl             v0.16b, {v0.16b,v1.16b}, v31.16b
    tbl             v1.16b, {v2.16b,v3.16b}, v31.16b
    st1             {v0.16b}, [x0], x1
    st1             {v1.16b}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_sp_32x32_neon)
    mov             w12, #4
    lsl             x3, x3, #1
    movrel          x11, xtn_xtn2_table
    ld1             {v31.16b}, [x11]
.loop_csp32:
    sub             w12, w12, #1
.rept 4
    ld1             {v0.8h-v3.8h}, [x2], x3
    ld1             {v4.8h-v7.8h}, [x2], x3
    tbl             v0.16b, {v0.16b,v1.16b}, v31.16b
    tbl             v1.16b, {v2.16b,v3.16b}, v31.16b
    tbl             v2.16b, {v4.16b,v5.16b}, v31.16b
    tbl             v3.16b, {v6.16b,v7.16b}, v31.16b
    st1             {v0.16b-v1.16b}, [x0], x1
    st1             {v2.16b-v3.16b}, [x0], x1
.endr
    cbnz            w12, .loop_csp32
    ret
endfunc

function PFX(blockcopy_sp_64x64_neon)
    mov             w12, #16
    lsl             x3, x3, #1
    sub             x3, x3, #64
    movrel          x11, xtn_xtn2_table
    ld1             {v31.16b}, [x11]
.loop_csp64:
    sub             w12, w12, #1
.rept 4
    ld1             {v0.8h-v3.8h}, [x2], #64
    ld1             {v4.8h-v7.8h}, [x2], x3
    tbl             v0.16b, {v0.16b,v1.16b}, v31.16b
    tbl             v1.16b, {v2.16b,v3.16b}, v31.16b
    tbl             v2.16b, {v4.16b,v5.16b}, v31.16b
    tbl             v3.16b, {v6.16b,v7.16b}, v31.16b
    st1             {v0.16b-v3.16b}, [x0], x1
.endr
    cbnz            w12, .loop_csp64
    ret
endfunc

// void blockcopy_ps(int16_t* a, intptr_t stridea, const pixel* b, intptr_t strideb)
function PFX(blockcopy_ps_4x4_neon)
    lsl             x1, x1, #1
.rept 2
    ld1             {v0.8b}, [x2], x3
    ld1             {v1.8b}, [x2], x3
    uxtl            v0.8h, v0.8b
    uxtl            v1.8h, v1.8b
    st1             {v0.4h}, [x0], x1
    st1             {v1.4h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_ps_8x8_neon)
    lsl             x1, x1, #1
.rept 4
    ld1             {v0.8b}, [x2], x3
    ld1             {v1.8b}, [x2], x3
    uxtl            v0.8h, v0.8b
    uxtl            v1.8h, v1.8b
    st1             {v0.8h}, [x0], x1
    st1             {v1.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_ps_16x16_neon)
    lsl             x1, x1, #1
.rept 8
    ld1             {v4.16b}, [x2], x3
    ld1             {v5.16b}, [x2], x3
    uxtl            v0.8h, v4.8b
    uxtl2           v1.8h, v4.16b
    uxtl            v2.8h, v5.8b
    uxtl2           v3.8h, v5.16b
    st1             {v0.8h-v1.8h}, [x0], x1
    st1             {v2.8h-v3.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_ps_32x32_neon)
    lsl             x1, x1, #1
    mov             w12, #4
.loop_cps32:
    sub             w12, w12, #1
.rept 4
    ld1             {v16.16b-v17.16b}, [x2], x3
    ld1             {v18.16b-v19.16b}, [x2], x3
    uxtl            v0.8h, v16.8b
    uxtl2           v1.8h, v16.16b
    uxtl            v2.8h, v17.8b
    uxtl2           v3.8h, v17.16b
    uxtl            v4.8h, v18.8b
    uxtl2           v5.8h, v18.16b
    uxtl            v6.8h, v19.8b
    uxtl2           v7.8h, v19.16b
    st1             {v0.8h-v3.8h}, [x0], x1
    st1             {v4.8h-v7.8h}, [x0], x1
.endr
    cbnz            w12, .loop_cps32
    ret
endfunc

function PFX(blockcopy_ps_64x64_neon)
    lsl             x1, x1, #1
    sub             x1, x1, #64
    mov             w12, #16
.loop_cps64:
    sub             w12, w12, #1
.rept 4
    ld1             {v16.16b-v19.16b}, [x2], x3
    uxtl            v0.8h, v16.8b
    uxtl2           v1.8h, v16.16b
    uxtl            v2.8h, v17.8b
    uxtl2           v3.8h, v17.16b
    uxtl            v4.8h, v18.8b
    uxtl2           v5.8h, v18.16b
    uxtl            v6.8h, v19.8b
    uxtl2           v7.8h, v19.16b
    st1             {v0.8h-v3.8h}, [x0], #64
    st1             {v4.8h-v7.8h}, [x0], x1
.endr
    cbnz            w12, .loop_cps64
    ret
endfunc

// void x265_blockcopy_ss(int16_t* a, intptr_t stridea, const int16_t* b, intptr_t strideb)
function PFX(blockcopy_ss_4x4_neon)
    lsl             x1, x1, #1
    lsl             x3, x3, #1
.rept 2
    ld1             {v0.8b}, [x2], x3
    ld1             {v1.8b}, [x2], x3
    st1             {v0.8b}, [x0], x1
    st1             {v1.8b}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_ss_8x8_neon)
    lsl             x1, x1, #1
    lsl             x3, x3, #1
.rept 4
    ld1             {v0.8h}, [x2], x3
    ld1             {v1.8h}, [x2], x3
    st1             {v0.8h}, [x0], x1
    st1             {v1.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_ss_16x16_neon)
    lsl             x1, x1, #1
    lsl             x3, x3, #1
.rept 8
    ld1             {v0.8h-v1.8h}, [x2], x3
    ld1             {v2.8h-v3.8h}, [x2], x3
    st1             {v0.8h-v1.8h}, [x0], x1
    st1             {v2.8h-v3.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_ss_32x32_neon)
    lsl             x1, x1, #1
    lsl             x3, x3, #1
    mov             w12, #4
.loop_css32:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.8h-v3.8h}, [x2], x3
    st1             {v0.8h-v3.8h}, [x0], x1
.endr
    cbnz            w12, .loop_css32
    ret
endfunc

function PFX(blockcopy_ss_64x64_neon)
    lsl             x1, x1, #1
    sub             x1, x1, #64
    lsl             x3, x3, #1
    sub             x3, x3, #64
    mov             w12, #8
.loop_css64:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.8h-v3.8h}, [x2], #64
    ld1             {v4.8h-v7.8h}, [x2], x3
    st1             {v0.8h-v3.8h}, [x0], #64
    st1             {v4.8h-v7.8h}, [x0], x1
.endr
    cbnz            w12, .loop_css64
    ret
endfunc

/******** Chroma blockcopy********/
function PFX(blockcopy_ss_4x8_neon)
    lsl             x1, x1, #1
    lsl             x3, x3, #1
.rept 4
    ld1             {v0.8b}, [x2], x3
    ld1             {v1.8b}, [x2], x3
    st1             {v0.8b}, [x0], x1
    st1             {v1.8b}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_ss_8x16_neon)
    lsl             x1, x1, #1
    lsl             x3, x3, #1
.rept 8
    ld1             {v0.8h}, [x2], x3
    ld1             {v1.8h}, [x2], x3
    st1             {v0.8h}, [x0], x1
    st1             {v1.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_ss_16x32_neon)
    lsl             x1, x1, #1
    lsl             x3, x3, #1
.rept 16
    ld1             {v0.8h-v1.8h}, [x2], x3
    ld1             {v2.8h-v3.8h}, [x2], x3
    st1             {v0.8h-v1.8h}, [x0], x1
    st1             {v2.8h-v3.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_ss_32x64_neon)
    lsl             x1, x1, #1
    lsl             x3, x3, #1
    mov             w12, #8
.loop_css32x64:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.8h-v3.8h}, [x2], x3
    st1             {v0.8h-v3.8h}, [x0], x1
.endr
    cbnz            w12, .loop_css32x64
    ret
endfunc

// chroma blockcopy_ps
function PFX(blockcopy_ps_4x8_neon)
    lsl             x1, x1, #1
.rept 4
    ld1             {v0.8b}, [x2], x3
    ld1             {v1.8b}, [x2], x3
    uxtl            v0.8h, v0.8b
    uxtl            v1.8h, v1.8b
    st1             {v0.4h}, [x0], x1
    st1             {v1.4h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_ps_8x16_neon)
    lsl             x1, x1, #1
.rept 8
    ld1             {v0.8b}, [x2], x3
    ld1             {v1.8b}, [x2], x3
    uxtl            v0.8h, v0.8b
    uxtl            v1.8h, v1.8b
    st1             {v0.8h}, [x0], x1
    st1             {v1.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_ps_16x32_neon)
    lsl             x1, x1, #1
.rept 16
    ld1             {v4.16b}, [x2], x3
    ld1             {v5.16b}, [x2], x3
    uxtl            v0.8h, v4.8b
    uxtl2           v1.8h, v4.16b
    uxtl            v2.8h, v5.8b
    uxtl2           v3.8h, v5.16b
    st1             {v0.8h-v1.8h}, [x0], x1
    st1             {v2.8h-v3.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_ps_32x64_neon)
    lsl             x1, x1, #1
    mov             w12, #8
.loop_cps32x64:
    sub             w12, w12, #1
.rept 4
    ld1             {v16.16b-v17.16b}, [x2], x3
    ld1             {v18.16b-v19.16b}, [x2], x3
    uxtl            v0.8h, v16.8b
    uxtl2           v1.8h, v16.16b
    uxtl            v2.8h, v17.8b
    uxtl2           v3.8h, v17.16b
    uxtl            v4.8h, v18.8b
    uxtl2           v5.8h, v18.16b
    uxtl            v6.8h, v19.8b
    uxtl2           v7.8h, v19.16b
    st1             {v0.8h-v3.8h}, [x0], x1
    st1             {v4.8h-v7.8h}, [x0], x1
.endr
    cbnz            w12, .loop_cps32x64
    ret
endfunc

// chroma blockcopy_sp
function PFX(blockcopy_sp_4x8_neon)
    lsl             x3, x3, #1
.rept 4
    ld1             {v0.8h}, [x2], x3
    ld1             {v1.8h}, [x2], x3
    xtn             v0.8b, v0.8h
    xtn             v1.8b, v1.8h
    st1             {v0.s}[0], [x0], x1
    st1             {v1.s}[0], [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_sp_8x16_neon)
    lsl             x3, x3, #1
.rept 8
    ld1             {v0.8h}, [x2], x3
    ld1             {v1.8h}, [x2], x3
    xtn             v0.8b, v0.8h
    xtn             v1.8b, v1.8h
    st1             {v0.d}[0], [x0], x1
    st1             {v1.d}[0], [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_sp_16x32_neon)
    lsl             x3, x3, #1
    movrel          x11, xtn_xtn2_table
    ld1             {v31.16b}, [x11]
.rept 16
    ld1             {v0.8h-v1.8h}, [x2], x3
    ld1             {v2.8h-v3.8h}, [x2], x3
    tbl             v0.16b, {v0.16b,v1.16b}, v31.16b
    tbl             v1.16b, {v2.16b,v3.16b}, v31.16b
    st1             {v0.16b}, [x0], x1
    st1             {v1.16b}, [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_sp_32x64_neon)
    mov             w12, #8
    lsl             x3, x3, #1
    movrel          x11, xtn_xtn2_table
    ld1             {v31.16b}, [x11]
.loop_csp32x64:
    sub             w12, w12, #1
.rept 4
    ld1             {v0.8h-v3.8h}, [x2], x3
    ld1             {v4.8h-v7.8h}, [x2], x3
    tbl             v0.16b, {v0.16b,v1.16b}, v31.16b
    tbl             v1.16b, {v2.16b,v3.16b}, v31.16b
    tbl             v2.16b, {v4.16b,v5.16b}, v31.16b
    tbl             v3.16b, {v6.16b,v7.16b}, v31.16b
    st1             {v0.16b-v1.16b}, [x0], x1
    st1             {v2.16b-v3.16b}, [x0], x1
.endr
    cbnz            w12, .loop_csp32x64
    ret
endfunc

/* blockcopy_pp(pixel* dst, intptr_t dstStride, const pixel* src, intptr_t srcStride) */

function PFX(blockcopy_pp_2x4_neon)
    ldrh            w9, [x2]
    add             x4, x1, x1
    add             x14, x3, x3
    strh            w9, [x0]
    ldrh            w10, [x2, x3]
    add             x5, x4, x1
    add             x15, x14, x3
    strh            w10, [x0, x1]
    ldrh            w11, [x2, x14]
    strh            w11, [x0, x4]
    ldrh            w12, [x2, x15]
    strh            w12, [x0, x5]
    ret
endfunc

.macro blockcopy_pp_2xN_neon h
function PFX(blockcopy_pp_2x\h\()_neon)
    add             x4, x1, x1
    add             x5, x4, x1
    add             x6, x5, x1

    add             x14, x3, x3
    add             x15, x14, x3
    add             x16, x15, x3

.rept \h / 4
    ldrh            w9, [x2]
    strh            w9, [x0]
    ldrh            w10, [x2, x3]
    strh            w10, [x0, x1]
    ldrh            w11, [x2, x14]
    strh            w11, [x0, x4]
    ldrh            w12, [x2, x15]
    strh            w12, [x0, x5]
    add             x2, x2, x16
    add             x0, x0, x6
.endr
    ret
endfunc
.endm

blockcopy_pp_2xN_neon 8
blockcopy_pp_2xN_neon 16

function PFX(blockcopy_pp_4x2_neon)
    ldr             w9, [x2]
    str             w9, [x0]
    ldr             w10, [x2, x3]
    str             w10, [x0, x1]
    ret
endfunc

function PFX(blockcopy_pp_4x4_neon)
    ldr             w9, [x2]
    add             x4, x1, x1
    add             x14, x3, x3
    str             w9, [x0]
    ldr             w10, [x2, x3]
    add             x5, x4, x1
    add             x15, x14, x3
    str             w10, [x0, x1]
    ldr             w11, [x2, x14]
    str             w11, [x0, x4]
    ldr             w12, [x2, x15]
    str             w12, [x0, x5]
    ret
endfunc

.macro blockcopy_pp_4xN_neon h
function PFX(blockcopy_pp_4x\h\()_neon)
    add             x4, x1, x1
    add             x5, x4, x1
    add             x6, x5, x1

    add             x14, x3, x3
    add             x15, x14, x3
    add             x16, x15, x3

.rept \h / 4
    ldr             w9, [x2]
    str             w9, [x0]
    ldr             w10, [x2, x3]
    str             w10, [x0, x1]
    ldr             w11, [x2, x14]
    str             w11, [x0, x4]
    ldr             w12, [x2, x15]
    str             w12, [x0, x5]
    add             x2, x2, x16
    add             x0, x0, x6
.endr
    ret
endfunc
.endm

blockcopy_pp_4xN_neon 8
blockcopy_pp_4xN_neon 16
blockcopy_pp_4xN_neon 32

.macro blockcopy_pp_6xN_neon h
function PFX(blockcopy_pp_6x\h\()_neon)
    sub             x1, x1, #4
.rept \h
    ld1             {v0.8b}, [x2], x3
    st1             {v0.s}[0], [x0], #4
    st1             {v0.h}[2], [x0], x1
.endr
    ret
endfunc
.endm

blockcopy_pp_6xN_neon 8
blockcopy_pp_6xN_neon 16

.macro blockcopy_pp_8xN_neon h
function PFX(blockcopy_pp_8x\h\()_neon)
.rept \h
    ld1             {v0.4h}, [x2], x3
    st1             {v0.4h}, [x0], x1
.endr
    ret
endfunc
.endm

blockcopy_pp_8xN_neon 2
blockcopy_pp_8xN_neon 4
blockcopy_pp_8xN_neon 6
blockcopy_pp_8xN_neon 8
blockcopy_pp_8xN_neon 12
blockcopy_pp_8xN_neon 16
blockcopy_pp_8xN_neon 32

function PFX(blockcopy_pp_8x64_neon)
    mov             w12, #4
.loop_pp_8x64:
    sub             w12, w12, #1
.rept 16
    ld1             {v0.4h}, [x2], x3
    st1             {v0.4h}, [x0], x1
.endr
    cbnz            w12, .loop_pp_8x64
    ret
endfunc

.macro blockcopy_pp_16xN_neon h
function PFX(blockcopy_pp_16x\h\()_neon)
.rept \h
    ld1             {v0.8h}, [x2], x3
    st1             {v0.8h}, [x0], x1
.endr
    ret
endfunc
.endm

blockcopy_pp_16xN_neon 4
blockcopy_pp_16xN_neon 8
blockcopy_pp_16xN_neon 12
blockcopy_pp_16xN_neon 16

.macro blockcopy_pp_16xN1_neon h
function PFX(blockcopy_pp_16x\h\()_neon)
    mov             w12, #\h / 8
.loop_16x\h\():
.rept 8
    ld1             {v0.8h}, [x2], x3
    st1             {v0.8h}, [x0], x1
.endr
    sub             w12, w12, #1
    cbnz            w12, .loop_16x\h
    ret
endfunc
.endm

blockcopy_pp_16xN1_neon 24
blockcopy_pp_16xN1_neon 32
blockcopy_pp_16xN1_neon 64

function PFX(blockcopy_pp_12x16_neon)
    sub             x1, x1, #8
.rept 16
    ld1             {v0.16b}, [x2], x3
    str             d0, [x0], #8
    st1             {v0.s}[2], [x0], x1
.endr
    ret
endfunc

function PFX(blockcopy_pp_12x32_neon)
    sub             x1, x1, #8
    mov             w12, #4
.loop_pp_12x32:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.16b}, [x2], x3
    str             d0, [x0], #8
    st1             {v0.s}[2], [x0], x1
.endr
    cbnz            w12, .loop_pp_12x32
    ret
endfunc

function PFX(blockcopy_pp_24x32_neon)
    mov             w12, #4
.loop_24x32:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.8b-v2.8b}, [x2], x3
    st1             {v0.8b-v2.8b}, [x0], x1
.endr
    cbnz            w12, .loop_24x32
    ret
endfunc

function PFX(blockcopy_pp_24x64_neon)
    mov             w12, #4
.loop_24x64:
    sub             w12, w12, #1
.rept 16
    ld1             {v0.8b-v2.8b}, [x2], x3
    st1             {v0.8b-v2.8b}, [x0], x1
.endr
    cbnz            w12, .loop_24x64
    ret
endfunc

function PFX(blockcopy_pp_32x8_neon)
.rept 8
    ld1             {v0.16b-v1.16b}, [x2], x3
    st1             {v0.16b-v1.16b}, [x0], x1
.endr
    ret
endfunc

.macro blockcopy_pp_32xN_neon h
function PFX(blockcopy_pp_32x\h\()_neon)
    mov             w12, #\h / 8
.loop_32x\h\():
    sub             w12, w12, #1
.rept 8
    ld1             {v0.16b-v1.16b}, [x2], x3
    st1             {v0.16b-v1.16b}, [x0], x1
.endr
    cbnz            w12, .loop_32x\h
    ret
endfunc
.endm

blockcopy_pp_32xN_neon 16
blockcopy_pp_32xN_neon 24
blockcopy_pp_32xN_neon 32
blockcopy_pp_32xN_neon 64
blockcopy_pp_32xN_neon 48

function PFX(blockcopy_pp_48x64_neon)
    mov             w12, #8
.loop_48x64:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.16b-v2.16b}, [x2], x3
    st1             {v0.16b-v2.16b}, [x0], x1
.endr
    cbnz            w12, .loop_48x64
    ret
endfunc

.macro blockcopy_pp_64xN_neon h
function PFX(blockcopy_pp_64x\h\()_neon)
    mov             w12, #\h / 4
.loop_64x\h\():
    sub             w12, w12, #1
.rept 4
    ld1             {v0.16b-v3.16b}, [x2], x3
    st1             {v0.16b-v3.16b}, [x0], x1
.endr
    cbnz            w12, .loop_64x\h
    ret
endfunc
.endm

blockcopy_pp_64xN_neon 16
blockcopy_pp_64xN_neon 32
blockcopy_pp_64xN_neon 48
blockcopy_pp_64xN_neon 64

// void x265_blockfill_s_neon(int16_t* dst, intptr_t dstride, int16_t val)
function PFX(blockfill_s_4x4_neon)
    dup             v0.4h, w2
    lsl             x1, x1, #1
.rept 4
    st1             {v0.4h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockfill_s_8x8_neon)
    dup             v0.8h, w2
    lsl             x1, x1, #1
.rept 8
    st1             {v0.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockfill_s_16x16_neon)
    dup             v0.8h, w2
    mov             v1.16b, v0.16b
    lsl             x1, x1, #1
.rept 16
    stp             q0, q1, [x0]
    add             x0, x0, x1
.endr
    ret
endfunc

function PFX(blockfill_s_32x32_neon)
    dup             v0.8h, w2
    mov             v1.16b, v0.16b
    mov             v2.16b, v0.16b
    mov             v3.16b, v0.16b
    lsl             x1, x1, #1
.rept 32
    st1             {v0.8h-v3.8h}, [x0], x1
.endr
    ret
endfunc

function PFX(blockfill_s_64x64_neon)
    dup             v0.8h, w2
    mov             v1.16b, v0.16b
    mov             v2.16b, v0.16b
    mov             v3.16b, v0.16b
    lsl             x1, x1, #1
    sub             x1, x1, #64
.rept 64
    st1             {v0.8h-v3.8h}, [x0], #64
    st1             {v0.8h-v3.8h}, [x0], x1
.endr
    ret
endfunc

// uint32_t copy_count(int16_t* coeff, const int16_t* residual, intptr_t resiStride)
function PFX(copy_cnt_4_neon)
    lsl             x2, x2, #1
    movi            v4.8b, #0
.rept 2
    ld1             {v0.8b}, [x1], x2
    ld1             {v1.8b}, [x1], x2
    stp             d0, d1, [x0], #16
    cmeq            v0.4h, v0.4h, #0
    cmeq            v1.4h, v1.4h, #0
    add             v4.4h, v4.4h, v0.4h
    add             v4.4h, v4.4h, v1.4h
.endr
    saddlv          s4, v4.4h
    fmov            w12, s4
    add             w0, w12, #16
    ret
endfunc

function PFX(copy_cnt_8_neon)
    lsl             x2, x2, #1
    movi            v4.8b, #0
.rept 4
    ld1             {v0.16b}, [x1], x2
    ld1             {v1.16b}, [x1], x2
    stp             q0, q1, [x0], #32
    cmeq            v0.8h, v0.8h, #0
    cmeq            v1.8h, v1.8h, #0
    add             v4.8h, v4.8h, v0.8h
    add             v4.8h, v4.8h, v1.8h
.endr
    saddlv          s4, v4.8h
    fmov            w12, s4
    add             w0, w12, #64
    ret
endfunc

function PFX(copy_cnt_16_neon)
    lsl             x2, x2, #1
    movi            v4.8b, #0
.rept 16
    ld1             {v0.16b-v1.16b}, [x1], x2
    st1             {v0.16b-v1.16b}, [x0], #32
    cmeq            v0.8h, v0.8h, #0
    cmeq            v1.8h, v1.8h, #0
    add             v4.8h, v4.8h, v0.8h
    add             v4.8h, v4.8h, v1.8h
.endr
    saddlv          s4, v4.8h
    fmov            w12, s4
    add             w0, w12, #256
    ret
endfunc

function PFX(copy_cnt_32_neon)
    lsl             x2, x2, #1
    movi            v4.8b, #0
.rept 32
    ld1             {v0.16b-v3.16b}, [x1], x2
    st1             {v0.16b-v3.16b}, [x0], #64
    cmeq            v0.8h, v0.8h, #0
    cmeq            v1.8h, v1.8h, #0
    cmeq            v2.8h, v2.8h, #0
    cmeq            v3.8h, v3.8h, #0
    add             v0.8h, v0.8h, v1.8h
    add             v2.8h, v2.8h, v3.8h
    add             v4.8h, v4.8h, v0.8h
    add             v4.8h, v4.8h, v2.8h
.endr
    saddlv          s4, v4.8h
    fmov            w12, s4
    add             w0, w12, #1024
    ret
endfunc

// int  count_nonzero_c(const int16_t* quantCoeff)
function PFX(count_nonzero_4_neon)
    movi            v16.16b, #1
    movi            v17.16b, #0
    trn1            v16.16b, v16.16b, v17.16b
    ldp             q0, q1, [x0]
    cmhi            v0.8h, v0.8h, v17.8h
    cmhi            v1.8h, v1.8h, v17.8h
    and             v0.16b, v0.16b, v16.16b
    and             v1.16b, v1.16b, v16.16b
    add             v0.8h, v0.8h, v1.8h
    uaddlv          s0, v0.8h
    fmov            w0, s0
    ret
endfunc

.macro COUNT_NONZERO_8
    ld1             {v0.16b-v3.16b}, [x0], #64
    ld1             {v4.16b-v7.16b}, [x0], #64
    cmhi            v0.8h, v0.8h, v17.8h
    cmhi            v1.8h, v1.8h, v17.8h
    cmhi            v2.8h, v2.8h, v17.8h
    cmhi            v3.8h, v3.8h, v17.8h
    cmhi            v4.8h, v4.8h, v17.8h
    cmhi            v5.8h, v5.8h, v17.8h
    cmhi            v6.8h, v6.8h, v17.8h
    cmhi            v7.8h, v7.8h, v17.8h
    and             v0.16b, v0.16b, v16.16b
    and             v1.16b, v1.16b, v16.16b
    and             v2.16b, v2.16b, v16.16b
    and             v3.16b, v3.16b, v16.16b
    and             v4.16b, v4.16b, v16.16b
    and             v5.16b, v5.16b, v16.16b
    and             v6.16b, v6.16b, v16.16b
    and             v7.16b, v7.16b, v16.16b
    add             v0.8h, v0.8h, v1.8h
    add             v2.8h, v2.8h, v3.8h
    add             v4.8h, v4.8h, v5.8h
    add             v6.8h, v6.8h, v7.8h
    add             v0.8h, v0.8h, v2.8h
    add             v4.8h, v4.8h, v6.8h
    add             v0.8h, v0.8h, v4.8h
.endm

function PFX(count_nonzero_8_neon)
    movi            v16.16b, #1
    movi            v17.16b, #0
    trn1            v16.16b, v16.16b, v17.16b
    COUNT_NONZERO_8
    uaddlv          s0, v0.8h
    fmov            w0, s0
    ret
endfunc

function PFX(count_nonzero_16_neon)
    movi            v16.16b, #1
    movi            v17.16b, #0
    trn1            v16.16b, v16.16b, v17.16b
    movi            v18.16b, #0
.rept 4
    COUNT_NONZERO_8
    add             v18.16b, v18.16b, v0.16b
.endr
    uaddlv          s0, v18.8h
    fmov            w0, s0
    ret
endfunc

function PFX(count_nonzero_32_neon)
    movi            v16.16b, #1
    movi            v17.16b, #0
    trn1            v16.16b, v16.16b, v17.16b
    movi            v18.16b, #0
    mov             w12, #16
.loop_count_nonzero_32:
    sub             w12, w12, #1
    COUNT_NONZERO_8
    add             v18.16b, v18.16b, v0.16b
    cbnz            w12, .loop_count_nonzero_32

    uaddlv          s0, v18.8h
    fmov            w0, s0
    ret
endfunc

// void cpy2Dto1D_shl(int16_t* dst, const int16_t* src, intptr_t srcStride, int shift)
.macro cpy2Dto1D_shl_start
    add             x2, x2, x2
    dup             v0.8h, w3
.endm

function PFX(cpy2Dto1D_shl_4x4_neon)
    cpy2Dto1D_shl_start
    ld1             {v2.d}[0], [x1], x2
    ld1             {v2.d}[1], [x1], x2
    ld1             {v3.d}[0], [x1], x2
    ld1             {v3.d}[1], [x1], x2
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    st1             {v2.16b-v3.16b}, [x0]
    ret
endfunc

function PFX(cpy2Dto1D_shl_8x8_neon)
    cpy2Dto1D_shl_start
.rept 4
    ld1             {v2.16b}, [x1], x2
    ld1             {v3.16b}, [x1], x2
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    st1             {v2.16b-v3.16b}, [x0], #32
.endr
    ret
endfunc

function PFX(cpy2Dto1D_shl_16x16_neon)
    cpy2Dto1D_shl_start
    mov             w12, #4
.loop_cpy2Dto1D_shl_16:
    sub             w12, w12, #1
.rept 4
    ld1             {v2.16b-v3.16b}, [x1], x2
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    st1             {v2.16b-v3.16b}, [x0], #32
.endr
    cbnz            w12, .loop_cpy2Dto1D_shl_16
    ret
endfunc

function PFX(cpy2Dto1D_shl_32x32_neon)
    cpy2Dto1D_shl_start
    mov             w12, #16
.loop_cpy2Dto1D_shl_32:
    sub             w12, w12, #1
.rept 2
    ld1             {v2.16b-v5.16b}, [x1], x2
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    sshl            v4.8h, v4.8h, v0.8h
    sshl            v5.8h, v5.8h, v0.8h
    st1             {v2.16b-v5.16b}, [x0], #64
.endr
    cbnz            w12, .loop_cpy2Dto1D_shl_32
    ret
endfunc

function PFX(cpy2Dto1D_shl_64x64_neon)
    cpy2Dto1D_shl_start
    mov             w12, #32
    sub             x2, x2, #64
.loop_cpy2Dto1D_shl_64:
    sub             w12, w12, #1
.rept 2
    ld1             {v2.16b-v5.16b}, [x1], #64
    ld1             {v16.16b-v19.16b}, [x1], x2
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    sshl            v4.8h, v4.8h, v0.8h
    sshl            v5.8h, v5.8h, v0.8h
    sshl            v16.8h, v16.8h, v0.8h
    sshl            v17.8h, v17.8h, v0.8h
    sshl            v18.8h, v18.8h, v0.8h
    sshl            v19.8h, v19.8h, v0.8h
    st1             {v2.16b-v5.16b}, [x0], #64
    st1             {v16.16b-v19.16b}, [x0], #64
.endr
    cbnz            w12, .loop_cpy2Dto1D_shl_64
    ret
endfunc

// void cpy2Dto1D_shr(int16_t* dst, const int16_t* src, intptr_t srcStride, int shift)
.macro cpy2Dto1D_shr_start
    add             x2, x2, x2
    dup             v0.8h, w3
    cmeq            v1.8h, v1.8h, v1.8h
    sshl            v1.8h, v1.8h, v0.8h
    sri             v1.8h, v1.8h, #1
    neg             v0.8h, v0.8h
.endm

function PFX(cpy2Dto1D_shr_4x4_neon)
    cpy2Dto1D_shr_start
    ld1             {v2.d}[0], [x1], x2
    ld1             {v2.d}[1], [x1], x2
    ld1             {v3.d}[0], [x1], x2
    ld1             {v3.d}[1], [x1], x2
    sub             v2.8h, v2.8h, v1.8h
    sub             v3.8h, v3.8h, v1.8h
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    stp             q2, q3, [x0]
    ret
endfunc

function PFX(cpy2Dto1D_shr_8x8_neon)
    cpy2Dto1D_shr_start
.rept 4
    ld1             {v2.16b}, [x1], x2
    ld1             {v3.16b}, [x1], x2
    sub             v2.8h, v2.8h, v1.8h
    sub             v3.8h, v3.8h, v1.8h
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    stp             q2, q3, [x0], #32
.endr
    ret
endfunc

function PFX(cpy2Dto1D_shr_16x16_neon)
    cpy2Dto1D_shr_start
    mov             w12, #4
.loop_cpy2Dto1D_shr_16:
    sub             w12, w12, #1
.rept 4
    ld1             {v2.8h-v3.8h}, [x1], x2
    sub             v2.8h, v2.8h, v1.8h
    sub             v3.8h, v3.8h, v1.8h
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    st1             {v2.8h-v3.8h}, [x0], #32
.endr
    cbnz            w12, .loop_cpy2Dto1D_shr_16
    ret
endfunc

function PFX(cpy2Dto1D_shr_32x32_neon)
    cpy2Dto1D_shr_start
    mov             w12, #16
.loop_cpy2Dto1D_shr_32:
    sub             w12, w12, #1
.rept 2
    ld1             {v2.8h-v5.8h}, [x1], x2
    sub             v2.8h, v2.8h, v1.8h
    sub             v3.8h, v3.8h, v1.8h
    sub             v4.8h, v4.8h, v1.8h
    sub             v5.8h, v5.8h, v1.8h
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    sshl            v4.8h, v4.8h, v0.8h
    sshl            v5.8h, v5.8h, v0.8h
    st1             {v2.8h-v5.8h}, [x0], #64
.endr
    cbnz            w12, .loop_cpy2Dto1D_shr_32
    ret
endfunc

// void cpy1Dto2D_shl(int16_t* dst, const int16_t* src, intptr_t dstStride, int shift)
.macro cpy1Dto2D_shl_start
    add             x2, x2, x2
    dup             v0.8h, w3
.endm

function PFX(cpy1Dto2D_shl_4x4_neon)
    cpy1Dto2D_shl_start
    ld1             {v2.16b-v3.16b}, [x1]
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    st1             {v2.d}[0], [x0], x2
    st1             {v2.d}[1], [x0], x2
    st1             {v3.d}[0], [x0], x2
    st1             {v3.d}[1], [x0], x2
    ret
endfunc

function PFX(cpy1Dto2D_shl_8x8_neon)
    cpy1Dto2D_shl_start
.rept 4
    ld1             {v2.16b-v3.16b}, [x1], #32
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    st1             {v2.16b}, [x0], x2
    st1             {v3.16b}, [x0], x2
.endr
    ret
endfunc

function PFX(cpy1Dto2D_shl_16x16_neon)
    cpy1Dto2D_shl_start
    mov             w12, #4
.loop_cpy1Dto2D_shl_16:
    sub             w12, w12, #1
.rept 4
    ld1             {v2.16b-v3.16b}, [x1], #32
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    st1             {v2.16b-v3.16b}, [x0], x2
.endr
    cbnz            w12, .loop_cpy1Dto2D_shl_16
    ret
endfunc

function PFX(cpy1Dto2D_shl_32x32_neon)
    cpy1Dto2D_shl_start
    mov             w12, #16
.loop_cpy1Dto2D_shl_32:
    sub             w12, w12, #1
.rept 2
    ld1             {v2.16b-v5.16b}, [x1], #64
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    sshl            v4.8h, v4.8h, v0.8h
    sshl            v5.8h, v5.8h, v0.8h
    st1             {v2.16b-v5.16b}, [x0], x2
.endr
    cbnz            w12, .loop_cpy1Dto2D_shl_32
    ret
endfunc

function PFX(cpy1Dto2D_shl_64x64_neon)
    cpy1Dto2D_shl_start
    mov             w12, #32
    sub             x2, x2, #64
.loop_cpy1Dto2D_shl_64:
    sub             w12, w12, #1
.rept 2
    ld1             {v2.16b-v5.16b}, [x1], #64
    ld1             {v16.16b-v19.16b}, [x1], #64
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    sshl            v4.8h, v4.8h, v0.8h
    sshl            v5.8h, v5.8h, v0.8h
    sshl            v16.8h, v16.8h, v0.8h
    sshl            v17.8h, v17.8h, v0.8h
    sshl            v18.8h, v18.8h, v0.8h
    sshl            v19.8h, v19.8h, v0.8h
    st1             {v2.16b-v5.16b}, [x0], #64
    st1             {v16.16b-v19.16b}, [x0], x2
.endr
    cbnz            w12, .loop_cpy1Dto2D_shl_64
    ret
endfunc

// void cpy1Dto2D_shr(int16_t* dst, const int16_t* src, intptr_t dstStride, int shift)
.macro cpy1Dto2D_shr_start
    add             x2, x2, x2
    dup             v0.8h, w3
    cmeq            v1.8h, v1.8h, v1.8h
    sshl            v1.8h, v1.8h, v0.8h
    sri             v1.8h, v1.8h, #1
    neg             v0.8h, v0.8h
.endm

function PFX(cpy1Dto2D_shr_4x4_neon)
    cpy1Dto2D_shr_start
    ld1             {v2.16b-v3.16b}, [x1]
    sub             v2.8h, v2.8h, v1.8h
    sub             v3.8h, v3.8h, v1.8h
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    st1             {v2.d}[0], [x0], x2
    st1             {v2.d}[1], [x0], x2
    st1             {v3.d}[0], [x0], x2
    st1             {v3.d}[1], [x0], x2
    ret
endfunc

function PFX(cpy1Dto2D_shr_8x8_neon)
    cpy1Dto2D_shr_start
.rept 4
    ld1             {v2.16b-v3.16b}, [x1], #32
    sub             v2.8h, v2.8h, v1.8h
    sub             v3.8h, v3.8h, v1.8h
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    st1             {v2.16b}, [x0], x2
    st1             {v3.16b}, [x0], x2
.endr
    ret
endfunc

function PFX(cpy1Dto2D_shr_16x16_neon)
    cpy1Dto2D_shr_start
    mov             w12, #4
.loop_cpy1Dto2D_shr_16:
    sub             w12, w12, #1
.rept 4
    ld1             {v2.8h-v3.8h}, [x1], #32
    sub             v2.8h, v2.8h, v1.8h
    sub             v3.8h, v3.8h, v1.8h
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    st1             {v2.8h-v3.8h}, [x0], x2
.endr
    cbnz            w12, .loop_cpy1Dto2D_shr_16
    ret
endfunc

function PFX(cpy1Dto2D_shr_32x32_neon)
    cpy1Dto2D_shr_start
    mov             w12, #16
.loop_cpy1Dto2D_shr_32:
    sub             w12, w12, #1
.rept 2
    ld1             {v2.16b-v5.16b}, [x1], #64
    sub             v2.8h, v2.8h, v1.8h
    sub             v3.8h, v3.8h, v1.8h
    sub             v4.8h, v4.8h, v1.8h
    sub             v5.8h, v5.8h, v1.8h
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    sshl            v4.8h, v4.8h, v0.8h
    sshl            v5.8h, v5.8h, v0.8h
    st1             {v2.16b-v5.16b}, [x0], x2
.endr
    cbnz            w12, .loop_cpy1Dto2D_shr_32
    ret
endfunc

function PFX(cpy1Dto2D_shr_64x64_neon)
    cpy1Dto2D_shr_start
    mov             w12, #32
    sub             x2, x2, #64
.loop_cpy1Dto2D_shr_64:
    sub             w12, w12, #1
.rept 2
    ld1             {v2.16b-v5.16b}, [x1], #64
    ld1             {v16.16b-v19.16b}, [x1], #64
    sub             v2.8h, v2.8h, v1.8h
    sub             v3.8h, v3.8h, v1.8h
    sub             v4.8h, v4.8h, v1.8h
    sub             v5.8h, v5.8h, v1.8h
    sub             v16.8h, v16.8h, v1.8h
    sub             v17.8h, v17.8h, v1.8h
    sub             v18.8h, v18.8h, v1.8h
    sub             v19.8h, v19.8h, v1.8h
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    sshl            v4.8h, v4.8h, v0.8h
    sshl            v5.8h, v5.8h, v0.8h
    sshl            v16.8h, v16.8h, v0.8h
    sshl            v17.8h, v17.8h, v0.8h
    sshl            v18.8h, v18.8h, v0.8h
    sshl            v19.8h, v19.8h, v0.8h
    st1             {v2.16b-v5.16b}, [x0], #64
    st1             {v16.16b-v19.16b}, [x0], x2
.endr
    cbnz            w12, .loop_cpy1Dto2D_shr_64
    ret
endfunc

const xtn_xtn2_table, align=4
.byte    0, 2, 4, 6, 8, 10, 12, 14
.byte    16, 18, 20, 22, 24, 26, 28, 30
endconst
