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

g_lumaFilter:
.word 0,0,0,0,0,0,64,64,0,0,0,0,0,0,0,0
.word -1,-1,4,4,-10,-10,58,58,17,17,-5,-5,1,1,0,0
.word -1,-1,4,4,-11,-11,40,40,40,40,-11,-11,4,4,-1,-1
.word 0,0,1,1,-5,-5,17,17,58,58,-10,-10,4,4,-1,-1 

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

.macro qpel_filter_0_32b
    vmov.i16        d17, #64
    vmovl.u8        q11, d3
    vmull.s16       q9, d22, d17    // 64*d0
    vmull.s16       q10, d23, d17   // 64*d1
.endm

.macro qpel_filter_1_32b
    vmov.i16        d16, #58
    vmovl.u8        q11, d3
    vmull.s16       q9, d22, d16        // 58 * d0
    vmull.s16       q10, d23, d16       // 58 * d1

    vmov.i16        d17, #10
    vmovl.u8        q13, d2
    vmull.s16       q11, d26, d17       // 10 * c0
    vmull.s16       q12, d27, d17       // 10 * c1

    vmov.i16        d16, #17
    vmovl.u8        q15, d4
    vmull.s16       q13, d30, d16       // 17 * e0
    vmull.s16       q14, d31, d16       // 17 * e1

    vmov.i16        d17, #5
    vmovl.u8        q1, d5
    vmull.s16       q15, d2, d17        //  5 * f0
    vmull.s16       q8, d3, d17         //  5 * f1

    vsub.s32        q9, q11             // 58 * d0 - 10 * c0
    vsub.s32        q10, q12            // 58 * d1 - 10 * c1

    vmovl.u8       q1, d1
    vshll.s16      q11, d2, #2         // 4 * b0
    vshll.s16      q12, d3, #2         // 4 * b1

    vadd.s32       q9, q13             // 58 * d0 - 10 * c0 + 17 * e0
    vadd.s32       q10, q14            // 58 * d1 - 10 * c1 + 17 * e1

    vmovl.u8       q1, d0
    vmovl.u8       q2, d6
    vsubl.s16      q13, d4, d2         // g0 - a0
    vsubl.s16      q14, d5, d3         // g1 - a1

    vadd.s32       q9, q11             // 58 * d0 - 10 * c0 + 17 * e0 + 4 * b0
    vadd.s32       q10, q12            // 58 * d1 - 10 * c1 + 17 * e1 + 4 * b1
    vsub.s32       q13, q15            // g0 - a0 - 5 * f0
    vsub.s32       q14, q8             // g1 - a1 - 5 * f1
    vadd.s32       q9, q13             // 58 * d0 - 10 * c0 + 17 * e0 + 4 * b0 + g0 - a0 - 5 * f0
    vadd.s32       q10, q14            // 58 * d1 - 10 * c1 + 17 * e1 + 4 * b1 + g1 - a1 - 5 * f1
.endm

.macro qpel_filter_2_32b
    vmov.i32        q8, #11
    vmovl.u8        q11, d3
    vmovl.u8        q12, d4
    vaddl.s16       q9, d22,d24        // d0 + e0
    vaddl.s16       q10, d23, d25      // d1 + e1

    vmovl.u8        q13, d2            //c
    vmovl.u8        q14, d5            //f
    vaddl.s16       q11, d26, d28      // c0 + f0
    vaddl.s16       q12, d27, d29      // c1 + f1

    vmul.s32        q11, q8            // 11 * (c0 + f0)
    vmul.s32        q12, q8            // 11 * (c1 + f1)

    vmov.i32        q8, #40
    vmul.s32        q9, q8             // 40 * (d0 + e0)
    vmul.s32        q10, q8            // 40 * (d1 + e1)

    vmovl.u8        q13, d1            //b
    vmovl.u8        q14, d6            //g
    vaddl.s16       q15, d26, d28      // b0 + g0
    vaddl.s16       q8, d27, d29       // b1 + g1

    vmovl.u8        q1, d0             //a
    vmovl.u8        q2, d7             //h
    vaddl.s16       q13, d2, d4        // a0 + h0
    vaddl.s16       q14, d3, d5        // a1 + h1

    vshl.s32        q15, #2            // 4*(b0+g0)
    vshl.s32        q8, #2             // 4*(b1+g1)

    vadd.s32        q11, q13           // 11 * (c0 + f0) + a0 + h0
    vadd.s32        q12, q14           // 11 * (c1 + f1) + a1 + h1
    vadd.s32        q9, q15            // 40 * (d0 + e0) + 4*(b0+g0)
    vadd.s32        q10, q8            // 40 * (d1 + e1) + 4*(b1+g1)
    vsub.s32        q9, q11            // 40 * (d0 + e0) + 4*(b0+g0) - (11 * (c0 + f0) + a0 + h0)
    vsub.s32        q10, q12           // 40 * (d1 + e1) + 4*(b1+g1) - (11 * (c1 + f1) + a1 + h1)
.endm

.macro qpel_filter_3_32b

    vmov.i16        d16, #17
    vmov.i16        d17, #5

    vmovl.u8        q11, d3
    vmull.s16       q9, d22, d16       // 17 * d0
    vmull.s16       q10, d23, d16      // 17 * d1

    vmovl.u8        q13, d2
    vmull.s16       q11, d26, d17      // 5 * c0
    vmull.s16       q12, d27, d17      // 5* c1

    vmov.i16        d16, #58
    vmovl.u8        q15, d4
    vmull.s16       q13, d30, d16      // 58 * e0
    vmull.s16       q14, d31, d16      // 58 * e1

    vmov.i16        d17, #10
    vmovl.u8        q1, d5
    vmull.s16       q15, d2, d17       // 10 * f0
    vmull.s16       q8, d3, d17        // 10 * f1

    vsub.s32        q9, q11            // 17 * d0 - 5 * c0
    vsub.s32        q10, q12           // 17 * d1 - 5 * c1

    vmovl.u8        q1, d6
    vshll.s16       q11, d2, #2        // 4 * g0
    vshll.s16       q12, d3, #2        // 4 * g1

    vadd.s32        q9, q13            // 17 * d0 - 5 * c0+ 58 * e0
    vadd.s32        q10, q14           // 17 * d1 - 5 * c1 + 58 * e1

    vmovl.u8        q1, d1
    vmovl.u8        q2, d7
    vsubl.s16      q13, d2, d4         // b0 - h0
    vsubl.s16      q14, d3, d5         // b1 - h1

    vadd.s32        q9, q11            // 17 * d0 - 5 * c0+ 58 * e0 +4 * g0
    vadd.s32        q10, q12           // 17 * d1 - 5 * c1 + 58 * e1+4 * g1
    vsub.s32        q13, q15           // 17 * d0 - 5 * c0+ 58 * e0 +4 * g0 -10 * f0
    vsub.s32        q14, q8            // 17 * d1 - 5 * c1 + 58 * e1+4 * g1 - 10*f1
    vadd.s32        q9, q13            //  17 * d0 - 5 * c0+ 58 * e0 +4 * g0 -10 * f0 +b0 - h0
    vadd.s32        q10, q14           // 17 * d1 - 5 * c1 + 58 * e1+4 * g1 - 10*f1 + b1 - h1
.endm

.macro FILTER_VPP a b filterv

.loop_\filterv\()_\a\()x\b:

    mov             r7, r2
    mov             r6, r0
    eor             r8, r8

.loop_w8_\filterv\()_\a\()x\b:

    add             r6, r0, r8

    pld [r6]
    vld1.u8         d0, [r6], r1
    pld [r6]
    vld1.u8         d1, [r6], r1
    pld [r6]
    vld1.u8         d2, [r6], r1
    pld [r6]
    vld1.u8         d3, [r6], r1
    pld [r6]
    vld1.u8         d4, [r6], r1
    pld [r6]
    vld1.u8         d5, [r6], r1
    pld [r6]
    vld1.u8         d6, [r6], r1
    pld [r6]
    vld1.u8         d7, [r6], r1

    veor.u8         q9, q9
    veor.u8         q10, q10

   \filterv

    mov             r12,#32
    vdup.32         q8, r12
    vadd.s32        q9, q8
    vqshrun.s32     d0, q9, #6
    vadd.s32        q10, q8
    vqshrun.s32     d1, q10, #6
    vqmovn.u16      d0, q0
    vst1.u8         d0, [r7]!

    add             r8, #8
    cmp             r8, #\a
    blt             .loop_w8_\filterv\()_\a\()x\b

    add             r0, r1
    add             r2, r3
    subs            r4, #1
    bne             .loop_\filterv\()_\a\()x\b 

.endm 

.macro LUMA_VPP  w h
function x265_interp_8tap_vert_pp_\w\()x\h\()_neon

    push            {r4, r5, r6, r7, r8}
    ldr             r5, [sp, #4 * 5]
    mov             r4, r1, lsl #2
    sub             r4, r1
    sub             r0, r4
    mov             r4, #\h

    cmp             r5, #0
    beq              0f
    cmp             r5, #1
    beq              1f
    cmp             r5, #2
    beq              2f
    cmp             r5, #3
    beq              3f
0:
    FILTER_VPP  \w \h qpel_filter_0_32b
    b            5f
1:
    FILTER_VPP  \w \h qpel_filter_1_32b
    b            5f
2:
    FILTER_VPP  \w \h qpel_filter_2_32b
    b            5f
3:
    FILTER_VPP  \w \h qpel_filter_3_32b
    b            5f
5:
    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc
.endm

LUMA_VPP 8 4
LUMA_VPP 8 8
LUMA_VPP 8 16
LUMA_VPP 8 32
LUMA_VPP 16 4
LUMA_VPP 16 8
LUMA_VPP 16 16
LUMA_VPP 16 32
LUMA_VPP 16 64
LUMA_VPP 16 12
LUMA_VPP 32 8
LUMA_VPP 32 16
LUMA_VPP 32 32
LUMA_VPP 32 64
LUMA_VPP 32 24
LUMA_VPP 64 16
LUMA_VPP 64 32
LUMA_VPP 64 64
LUMA_VPP 64 48
LUMA_VPP 24 32
LUMA_VPP 48 64

.macro LUMA_VPP_4xN h
function x265_interp_8tap_vert_pp_4x\h\()_neon
    push           {r4, r5, r6}
    ldr             r4, [sp, #4 * 3]
    mov             r5, r4, lsl #6
    mov             r4, r1, lsl #2
    sub             r4, r1
    sub             r0, r4

    mov             r4, #32
    vdup.32         q8, r4
    mov             r4, #\h

.loop_4x\h:
    movrel          r12, g_lumaFilter
    add             r12, r5
    mov             r6, r0

    pld [r6]
    vld1.u32        d0[0], [r6], r1
    pld [r6]
    vld1.u32        d0[1], [r6], r1
    pld [r6]
    vld1.u32        d1[0], [r6], r1
    pld [r6]
    vld1.u32        d1[1], [r6], r1
    pld [r6]
    vld1.u32        d2[0], [r6], r1
    pld [r6]
    vld1.u32        d2[1], [r6], r1
    pld [r6]
    vld1.u32        d3[0], [r6], r1
    pld [r6]
    vld1.u32        d3[1], [r6], r1

    veor.u8         q9, q9

    vmovl.u8        q11, d0
    vmovl.u16       q12, d22
    vmovl.u16       q13, d23
    vld1.s32        d20, [r12]!
    vmov.s32        d21, d20
    vmla.s32        q9, q12, q10
    vld1.s32        d20, [r12]!
    vmov.s32        d21, d20
    vmla.s32        q9, q13, q10

    vmovl.u8        q11, d1
    vmovl.u16       q12, d22
    vmovl.u16       q13, d23
    vld1.s32        d20, [r12]!
    vmov.s32        d21, d20
    vmla.s32        q9, q12, q10
    vld1.s32        d20, [r12]!
    vmov.s32        d21, d20
    vmla.s32        q9, q13, q10

    vmovl.u8        q11, d2
    vmovl.u16       q12, d22
    vmovl.u16       q13, d23
    vld1.s32        d20, [r12]!
    vmov.s32        d21, d20
    vmla.s32        q9, q12, q10
    vld1.s32        d20, [r12]!
    vmov.s32        d21, d20
    vmla.s32        q9, q13, q10

    vmovl.u8        q11, d3
    vmovl.u16       q12, d22
    vmovl.u16       q13, d23
    vld1.s32        d20, [r12]!
    vmov.s32        d21, d20
    vmla.s32        q9, q12, q10
    vld1.s32        d20, [r12]!
    vmov.s32        d21, d20
    vmla.s32        q9, q13, q10

    vadd.s32        q9, q8
    vqshrun.s32     d0, q9, #6
    vqmovn.u16      d0, q0
    vst1.u32        d0[0], [r2], r3

    add             r0, r1
    subs            r4, #1
    bne             .loop_4x\h

    pop             {r4, r5, r6}
    bx              lr
endfunc
.endm

LUMA_VPP_4xN 4
LUMA_VPP_4xN 8
LUMA_VPP_4xN 16

function x265_interp_8tap_vert_pp_12x16_neon
    push            {r4, r5, r6, r7}
    ldr             r5, [sp, #4 * 4]
    mov             r4, r1, lsl #2
    sub             r4, r1
    sub             r0, r4

    mov             r4, #16
.loop_12x16:

    mov             r6, r0
    mov             r7, r2

    pld [r6]
    vld1.u8         d0, [r6], r1
    pld [r6]
    vld1.u8         d1, [r6], r1
    pld [r6]
    vld1.u8         d2, [r6], r1
    pld [r6]
    vld1.u8         d3, [r6], r1
    pld [r6]
    vld1.u8         d4, [r6], r1
    pld [r6]
    vld1.u8         d5, [r6], r1
    pld [r6]
    vld1.u8         d6, [r6], r1
    pld [r6]
    vld1.u8         d7, [r6], r1

    veor.u8         q9, q9
    veor.u8         q10, q10

    cmp             r5,#0
    beq              0f
    cmp             r5,#1
    beq              1f
    cmp             r5,#2
    beq              2f
    cmp             r5,#3
    beq              3f
1:
    qpel_filter_1_32b
    b            5f
2:
    qpel_filter_2_32b
    b            5f
3:
    qpel_filter_3_32b
    b            5f
0:
    vmov.i16        d17, #64
    vmovl.u8        q11, d3
    vmull.s16       q9, d22, d17    // 64*d0
    vmull.s16       q10, d23, d17   // 64*d1
5:
    mov             r12,#32
    vdup.32         q8, r12
    vadd.s32        q9, q8
    vqshrun.s32     d0, q9, #6
    vadd.s32        q10, q8
    vqshrun.s32     d1, q10, #6
    vqmovn.u16      d0, q0
    vst1.u8         d0, [r7]!

    add             r6, r0, #8

    pld [r6]
    vld1.u8         d0, [r6], r1
    pld [r6]
    vld1.u8         d1, [r6], r1
    pld [r6]
    vld1.u8         d2, [r6], r1
    pld [r6]
    vld1.u8         d3, [r6], r1
    pld [r6]
    vld1.u8         d4, [r6], r1
    pld [r6]
    vld1.u8         d5, [r6], r1
    pld [r6]
    vld1.u8         d6, [r6], r1
    pld [r6]
    vld1.u8         d7, [r6], r1

    veor.u8         q9, q9
    veor.u8         q10, q10

    cmp             r5,#0
    beq              0f
    cmp             r5,#1
    beq              1f
    cmp             r5,#2
    beq              2f
    cmp             r5,#3
    beq              3f
1:
    qpel_filter_1_32b
    b            5f
2:
    qpel_filter_2_32b
    b            5f
3:
    qpel_filter_3_32b
    b            5f
0:
    vmov.i16        d17, #64
    vmovl.u8        q11, d3
    vmull.s16       q9, d22, d17    // 64*d0
    vmull.s16       q10, d23, d17   // 64*d1
5:
    mov             r12,#32
    vdup.32         q8, r12
    vadd.s32        q9, q8
    vqshrun.s32     d0, q9, #6
    vadd.s32        q10, q8
    vqshrun.s32     d1, q10, #6
    vqmovn.u16      d0, q0
    vst1.u32        d0[0], [r7]!

    add             r0, r1
    add             r2, r3
    subs            r4, #1
    bne             .loop_12x16

    pop             {r4, r5, r6, r7}
    bx              lr
endfunc
