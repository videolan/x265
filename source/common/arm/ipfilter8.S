/*****************************************************************************
 * Copyright (C) 2016 x265 project
 *
 * Authors: Dnyaneshwar G <dnyaneshwar@multicorewareinc.com>
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

.section .rodata

.align 4

.text

// filterPixelToShort(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride)
function x265_filterPixelToShort_4x4_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 2
    vld1.u8     {d0}, [r0], r1
    vld1.u8     {d2}, [r0], r1
    vmovl.u8    q0, d0
    vmovl.u8    q1, d2
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {d4}, [r2], r3
    vst1.16     {d6}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_4x8_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 4
    vld1.u8     {d0}, [r0], r1
    vld1.u8     {d2}, [r0], r1
    vmovl.u8    q0, d0
    vmovl.u8    q1, d2
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {d4}, [r2], r3
    vst1.16     {d6}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_4x16_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 8
    vld1.u8     {d0}, [r0], r1
    vld1.u8     {d2}, [r0], r1
    vmovl.u8    q0, d0
    vmovl.u8    q1, d2
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {d4}, [r2], r3
    vst1.16     {d6}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_8x4_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 2
    vld1.u8     {d0}, [r0], r1
    vld1.u8     {d2}, [r0], r1
    vmovl.u8    q0, d0
    vmovl.u8    q1, d2
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {q2}, [r2], r3
    vst1.16     {q3}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_8x8_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 4
    vld1.u8     {d0}, [r0], r1
    vld1.u8     {d2}, [r0], r1
    vmovl.u8    q0, d0
    vmovl.u8    q1, d2
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {q2}, [r2], r3
    vst1.16     {q3}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_8x16_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 8
    vld1.u8     {d0}, [r0], r1
    vld1.u8     {d2}, [r0], r1
    vmovl.u8    q0, d0
    vmovl.u8    q1, d2
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {q2}, [r2], r3
    vst1.16     {q3}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_8x32_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 16
    vld1.u8     {d0}, [r0], r1
    vld1.u8     {d2}, [r0], r1
    vmovl.u8    q0, d0
    vmovl.u8    q1, d2
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {q2}, [r2], r3
    vst1.16     {q3}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_12x16_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 16
    vld1.u8     {d2-d3}, [r0], r1
    vmovl.u8    q0, d2
    vmovl.u8    q1, d3
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {d4, d5, d6}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_16x4_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 4
    vld1.u8     {d2-d3}, [r0], r1
    vmovl.u8    q0, d2
    vmovl.u8    q1, d3
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {q2-q3}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_16x8_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 8
    vld1.u8     {d2-d3}, [r0], r1
    vmovl.u8    q0, d2
    vmovl.u8    q1, d3
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {q2-q3}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_16x12_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 12
    vld1.u8     {d2-d3}, [r0], r1
    vmovl.u8    q0, d2
    vmovl.u8    q1, d3
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {q2-q3}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_16x16_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 16
    vld1.u8     {d2-d3}, [r0], r1
    vmovl.u8    q0, d2
    vmovl.u8    q1, d3
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {q2-q3}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_16x32_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 32
    vld1.u8     {d2-d3}, [r0], r1
    vmovl.u8    q0, d2
    vmovl.u8    q1, d3
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {q2-q3}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_16x64_neon
    add         r3, r3
    vmov.u16    q8, #64
    vmov.u16    q9, #8192
    vneg.s16    q9, q9
.rept 64
    vld1.u8     {d2-d3}, [r0], r1
    vmovl.u8    q0, d2
    vmovl.u8    q1, d3
    vmov        q2, q9
    vmov        q3, q9
    vmla.s16    q2, q0, q8
    vmla.s16    q3, q1, q8
    vst1.16     {q2-q3}, [r2], r3
.endr   
    bx          lr
endfunc

function x265_filterPixelToShort_24x32_neon
    add         r3, r3
    sub         r3, #32
    vmov.u16    q0, #64
    vmov.u16    q1, #8192
    vneg.s16    q1, q1
.rept 32
    vld1.u8     {d18, d19, d20}, [r0], r1
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmla.s16    q2, q11, q0
    vst1.16     {q2}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_32x8_neon
    add         r3, r3
    sub         r3, #32
    vmov.u16    q0, #64
    vmov.u16    q1, #8192
    vneg.s16    q1, q1
.rept 8
    vld1.u8     {q9-q10}, [r0], r1
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2], r3
.endr
    bx          lr
endfunc

function x265_filterPixelToShort_32x16_neon
    add         r3, r3
    sub         r3, #32
    vmov.u16    q0, #64
    vmov.u16    q1, #8192
    vneg.s16    q1, q1
    mov         r12, #8
.loop_filterP2S_32x16:
    subs        r12, #1
.rept 2
    vld1.u8     {q9-q10}, [r0], r1
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2], r3
.endr
    bgt         .loop_filterP2S_32x16
    bx          lr
endfunc

function x265_filterPixelToShort_32x24_neon
    add         r3, r3
    sub         r3, #32
    vmov.u16    q0, #64
    vmov.u16    q1, #8192
    vneg.s16    q1, q1
    mov         r12, #12
.loop_filterP2S_32x24:
    subs        r12, #1
.rept 2
    vld1.u8     {q9-q10}, [r0], r1
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2], r3
.endr
    bgt         .loop_filterP2S_32x24
    bx          lr
endfunc

function x265_filterPixelToShort_32x32_neon
    add         r3, r3
    sub         r3, #32
    vmov.u16    q0, #64
    vmov.u16    q1, #8192
    vneg.s16    q1, q1
    mov         r12, #16
.loop_filterP2S_32x32:
    subs        r12, #1
.rept 2
    vld1.u8     {q9-q10}, [r0], r1
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2], r3
.endr
    bgt         .loop_filterP2S_32x32
    bx          lr
endfunc

function x265_filterPixelToShort_32x64_neon
    add         r3, r3
    sub         r3, #32
    vmov.u16    q0, #64
    vmov.u16    q1, #8192
    vneg.s16    q1, q1
    mov         r12, #32
.loop_filterP2S_32x64:
    subs        r12, #1
.rept 2
    vld1.u8     {q9-q10}, [r0], r1
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2], r3
.endr
    bgt         .loop_filterP2S_32x64
    bx          lr
endfunc

function x265_filterPixelToShort_64x16_neon
    add         r3, r3
    sub         r1, #32
    sub         r3, #96
    vmov.u16    q0, #64
    vmov.u16    q1, #8192
    vneg.s16    q1, q1
    mov         r12, #8
.loop_filterP2S_64x16:
    subs        r12, #1
.rept 2
    vld1.u8     {q9-q10}, [r0]!
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2]!

    vld1.u8     {q9-q10}, [r0], r1
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2], r3
.endr
    bgt         .loop_filterP2S_64x16
    bx          lr
endfunc

function x265_filterPixelToShort_64x32_neon
    add         r3, r3
    sub         r1, #32
    sub         r3, #96
    vmov.u16    q0, #64
    vmov.u16    q1, #8192
    vneg.s16    q1, q1
    mov         r12, #16
.loop_filterP2S_64x32:
    subs        r12, #1
.rept 2
    vld1.u8     {q9-q10}, [r0]!
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2]!

    vld1.u8     {q9-q10}, [r0], r1
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2], r3
.endr
    bgt         .loop_filterP2S_64x32
    bx          lr
endfunc

function x265_filterPixelToShort_64x48_neon
    add         r3, r3
    sub         r1, #32
    sub         r3, #96
    vmov.u16    q0, #64
    vmov.u16    q1, #8192
    vneg.s16    q1, q1
    mov         r12, #24
.loop_filterP2S_64x48:
    subs        r12, #1
.rept 2
    vld1.u8     {q9-q10}, [r0]!
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2]!

    vld1.u8     {q9-q10}, [r0], r1
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2], r3
.endr
    bgt         .loop_filterP2S_64x48
    bx          lr
endfunc

function x265_filterPixelToShort_64x64_neon
    add         r3, r3
    sub         r1, #32
    sub         r3, #96
    vmov.u16    q0, #64
    vmov.u16    q1, #8192
    vneg.s16    q1, q1
    mov         r12, #32
.loop_filterP2S_64x64:
    subs        r12, #1
.rept 2
    vld1.u8     {q9-q10}, [r0]!
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2]!

    vld1.u8     {q9-q10}, [r0], r1
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2], r3
.endr
    bgt         .loop_filterP2S_64x64
    bx          lr
endfunc

function x265_filterPixelToShort_48x64_neon
    add         r3, r3
    sub         r1, #32
    sub         r3, #64
    vmov.u16    q0, #64
    vmov.u16    q1, #8192
    vneg.s16    q1, q1
    mov         r12, #32
.loop_filterP2S_48x64:
    subs        r12, #1
.rept 2
    vld1.u8     {q9-q10}, [r0]!
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmovl.u8    q11, d20
    vmovl.u8    q10, d21
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2]!
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q11, q0
    vmla.s16    q3, q10, q0
    vst1.16     {q2-q3}, [r2]!

    vld1.u8     {q9}, [r0], r1
    vmovl.u8    q8, d18
    vmovl.u8    q9, d19
    vmov        q2, q1
    vmov        q3, q1
    vmla.s16    q2, q8, q0
    vmla.s16    q3, q9, q0
    vst1.16     {q2-q3}, [r2], r3
.endr
    bgt         .loop_filterP2S_48x64
    bx          lr
endfunc
