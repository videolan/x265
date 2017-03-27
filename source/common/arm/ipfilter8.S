/*****************************************************************************
 * Copyright (C) 2013-2017 MulticoreWare, Inc
 *
 * Authors: Dnyaneshwar G <dnyaneshwar@multicorewareinc.com>
 *          Radhakrishnan VR <radhakrishnan@multicorewareinc.com>
 *          Min Chen <min.chen@multicorewareinc.com>
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
g_chromaFilter:
.word 0, 0, 64, 64, 0, 0, 0, 0
.word -2, -2, 58, 58, 10, 10, -2, -2
.word -4, -4, 54, 54, 16, 16, -2, -2
.word -6, -6, 46, 46, 28, 28, -4, -4
.word -4, -4, 36, 36, 36, 36, -4 ,-4
.word -4, -4, 28, 28, 46, 46, -6, -6
.word -2, -2, 16, 16, 54, 54, -4 ,-4
.word -2, -2, 10, 10, 58, 58, -2, -2


.text

// filterPixelToShort(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride)
function x265_filterPixelToShort_4x4_neon
    vld1.u32    {d0[]}, [r0], r1
    vld1.u32    {d0[1]}, [r0], r1
    vld1.u32    {d1[]}, [r0], r1
    vld1.u32    {d1[1]}, [r0], r1

    // avoid load pipeline stall
    vmov.i16    q1, #0xE000

    vshll.u8    q2, d0, #6
    vshll.u8    q3, d1, #6
    vadd.i16    q2, q1
    vadd.i16    q3, q1

    add         r3, r3
    vst1.16     {d4}, [r2], r3
    vst1.16     {d5}, [r2], r3
    vst1.16     {d6}, [r2], r3
    vst1.16     {d7}, [r2], r3

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

//**************luma_vpp************
.align 8
// TODO: I don't like S16 in here, but the VMUL with scalar doesn't support (U8 x U8)
g_luma_s16:
.hword   0, 0,   0, 64,  0,   0, 0,  0
.hword  -1, 4, -10, 58, 17,  -5, 1,  0
.hword  -1, 4, -11, 40, 40, -11, 4, -1
.hword   0, 1,  -5, 17, 58, -10, 4, -1

.macro LUMA_VPP_4xN h
function x265_interp_8tap_vert_pp_4x\h\()_neon
    ldr         r12, [sp]
    push        {lr}
    adr         lr, g_luma_s16
    sub         r0, r1
    sub         r0, r0, r1, lsl #1          // src -= 3 * srcStride
    add         lr, lr, r12, lsl #4
    vld1.16     {q0}, [lr, :64]             // q8 = luma interpolate coeff
    vdup.s16    d24, d0[0]
    vdup.s16    d25, d0[1]
    vdup.s16    d26, d0[2]
    vdup.s16    d27, d0[3]
    vdup.s16    d28, d1[0]
    vdup.s16    d29, d1[1]
    vdup.s16    d30, d1[2]
    vdup.s16    d31, d1[3]

    mov         r12, #\h

    // prepare to load 8 lines
    vld1.u32    {d0[0]}, [r0], r1
    vld1.u32    {d0[1]}, [r0], r1
    vld1.u32    {d2[0]}, [r0], r1
    vld1.u32    {d2[1]}, [r0], r1
    vld1.u32    {d4[0]}, [r0], r1
    vld1.u32    {d4[1]}, [r0], r1
    vld1.u32    {d6[0]}, [r0], r1
    vld1.u32    {d6[1]}, [r0], r1
    vmovl.u8    q0, d0
    vmovl.u8    q1, d2
    vmovl.u8    q2, d4
    vmovl.u8    q3, d6

.loop_4x\h:
    // TODO: read extra 1 row for speed optimize, may made crash on OS X platform!
    vld1.u32    {d16[0]}, [r0], r1
    vld1.u32    {d16[1]}, [r0], r1
    vmovl.u8    q8, d16

    // row[0-1]
    vmul.s16    q9, q0, q12
    vext.64     q11, q0, q1, 1
    vmul.s16    q10, q11, q12
    vmov        q0, q1

    // row[2-3]
    vmla.s16    q9, q1, q13
    vext.64     q11, q1, q2, 1
    vmla.s16    q10, q11, q13
    vmov        q1, q2

    // row[4-5]
    vmla.s16    q9, q2, q14
    vext.64     q11, q2, q3, 1
    vmla.s16    q10, q11, q14
    vmov        q2, q3

    // row[6-7]
    vmla.s16    q9, q3, q15
    vext.64     q11, q3, q8, 1
    vmla.s16    q10, q11, q15
    vmov        q3, q8

    // sum row[0-7]
    vadd.s16    d18, d18, d19
    vadd.s16    d19, d20, d21

    vqrshrun.s16 d18, q9, #6
    vst1.u32    {d18[0]}, [r2], r3
    vst1.u32    {d18[1]}, [r2], r3

    subs        r12, #2
    bne        .loop_4x4

    pop         {pc}
    .ltorg
endfunc
.endm

LUMA_VPP_4xN 4
LUMA_VPP_4xN 8
LUMA_VPP_4xN 16

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

function x265_interp_8tap_vert_pp_12x16_neon
    push            {r4, r5, r6, r7}
    ldr             r5, [sp, #4 * 4]
    mov             r4, r1, lsl #2
    sub             r4, r1
    sub             r0, r4

    mov             r4, #16
.loop_vpp_12x16:

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
0:
    qpel_filter_0_32b
    b            5f
1:
    qpel_filter_1_32b
    b            5f
2:
    qpel_filter_2_32b
    b            5f
3:
    qpel_filter_3_32b
    b            5f
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
0:
    qpel_filter_0_32b
    b            5f
1:
    qpel_filter_1_32b
    b            5f
2:
    qpel_filter_2_32b
    b            5f
3:
    qpel_filter_3_32b
    b            5f
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
    bne             .loop_vpp_12x16

    pop             {r4, r5, r6, r7}
    bx              lr
endfunc
//**************luma_vsp************
.macro LUMA_VSP_4xN h
function x265_interp_8tap_vert_sp_4x\h\()_neon
    push            {r4, r5, r6}
    ldr             r4, [sp, #4 * 3]
    mov             r5, r4, lsl #6
    lsl             r1, #1
    mov             r4, r1, lsl #2
    sub             r4, r1
    sub             r0, r4

    mov             r12, #1
    lsl             r12, #19
    add             r12, #2048
    vdup.32         q8, r12
    mov             r4, #\h
.loop_vsp_4x\h:
    movrel          r12, g_lumaFilter
    add             r12, r5
    mov             r6, r0

    pld [r6]
    vld1.u16         d0, [r6], r1
    pld [r6]
    vld1.u16         d1, [r6], r1
    pld [r6]
    vld1.u16         d2, [r6], r1
    pld [r6]
    vld1.u16         d3, [r6], r1
    pld [r6]
    vld1.u16         d4, [r6], r1
    pld [r6]
    vld1.u16         d5, [r6], r1
    pld [r6]
    vld1.u16         d6, [r6], r1
    pld [r6]
    vld1.u16         d7, [r6], r1

    veor.u8         q9, q9

    vmovl.s16       q11, d0
    vld1.s32        d24, [r12]!
    vmov.s32        d25, d24
    vmla.s32        q9, q12, q11

    vmovl.s16       q11, d1
    vld1.s32        d24, [r12]!
    vmov.s32        d25, d24
    vmla.s32        q9, q12, q11

    vmovl.s16       q11, d2
    vld1.s32        d24, [r12]!
    vmov.s32        d25, d24
    vmla.s32        q9, q12, q11

    vmovl.s16       q11, d3
    vld1.s32        d24, [r12]!
    vmov.s32        d25, d24
    vmla.s32        q9, q12, q11

    vmovl.s16       q11, d4
    vld1.s32        d24, [r12]!
    vmov.s32        d25, d24
    vmla.s32        q9, q12, q11

    vmovl.s16       q11, d5
    vld1.s32        d24, [r12]!
    vmov.s32        d25, d24
    vmla.s32        q9, q12, q11

    vmovl.s16       q11, d6
    vld1.s32        d24, [r12]!
    vmov.s32        d25, d24
    vmla.s32        q9, q12, q11

    vmovl.s16       q11, d7
    vld1.s32        d24, [r12]!
    vmov.s32        d25, d24
    vmla.s32        q9, q12, q11


    vadd.s32        q9, q8
    vqshrun.s32     d0, q9, #12
    vqmovn.u16      d0, q0
    vst1.u32        d0[0], [r2], r3

    add             r0, r1
    subs            r4, #1
    bne             .loop_vsp_4x\h
    pop             {r4, r5, r6}
    bx              lr
    .ltorg
endfunc
.endm

LUMA_VSP_4xN 4
LUMA_VSP_4xN 8
LUMA_VSP_4xN 16

.macro qpel_filter_0_32b_1
    vmov.i16        d17, #64
    vmull.s16       q9, d6, d17    // 64*d0
    vmull.s16       q10, d7, d17   // 64*d1
.endm

.macro qpel_filter_1_32b_1
    vmov.i16        d16, #58
    vmov.i16        d17, #10
    vmull.s16       q9, d6, d16    // 58 * d0
    vmull.s16       q10, d7, d16   // 58 * d1
    vmov.i16        d16, #17
    vmull.s16       q11, d4, d17   // 10 * c0
    vmull.s16       q12, d5, d17   // 10 * c1
    vmov.i16        d17, #5
    vmull.s16       q13, d8, d16   // 17 * e0
    vmull.s16       q14, d9, d16   // 17 * e1
    vmull.s16       q15, d10, d17  //  5 * f0
    vmull.s16       q8, d11, d17   //  5 * f1
    vsub.s32        q9, q11        // 58 * d0 - 10 * c0
    vsub.s32        q10, q12       // 58 * d1 - 10 * c1
    vshll.s16       q11, d2, #2    // 4 * b0
    vshll.s16       q12, d3, #2    // 4 * b1
    vadd.s32        q9, q13        // 58 * d0 - 10 * c0 + 17 * e0
    vadd.s32        q10, q14       // 58 * d1 - 10 * c1 + 17 * e1
    vsubl.s16       q13, d12, d0   // g0 - a0
    vsubl.s16       q14, d13, d1   // g1 - a1
    vadd.s32        q9, q11        // 58 * d0 - 10 * c0 + 17 * e0 + 4 * b0
    vadd.s32        q10, q12       // 58 * d1 - 10 * c1 + 17 * e1 + 4 * b1
    vsub.s32        q13, q15       // g0 - a0 - 5 * f0
    vsub.s32        q14, q8        // g1 - a1 - 5 * f1
    vadd.s32        q9, q13        // 58 * d0 - 10 * c0 + 17 * e0 + 4 * b0 + g0 - a0 - 5 * f0
    vadd.s32        q10, q14       // 58 * d1 - 10 * c1 + 17 * e1 + 4 * b1 + g1 - a1 - 5 * f1
.endm

.macro qpel_filter_2_32b_1
    vmov.i32        q8, #11
    vaddl.s16       q9, d6, d8    // d0 + e0
    vaddl.s16       q10, d7, d9   // d1 + e1
    vaddl.s16       q11, d4, d10  // c0 + f0
    vaddl.s16       q12, d5, d11  // c1 + f1
    vmul.s32        q11, q8       // 11 * (c0 + f0)
    vmul.s32        q12, q8       // 11 * (c1 + f1)
    vmov.i32        q8, #40
    vaddl.s16       q15, d2, d12  // b0 + g0
    vmul.s32        q9, q8        // 40 * (d0 + e0)
    vmul.s32        q10, q8       // 40 * (d1 + e1)
    vaddl.s16       q8, d3, d13   // b1 + g1
    vaddl.s16       q13, d0, d14  // a0 + h0
    vaddl.s16       q14, d1, d15  // a1 + h1
    vshl.s32        q15, #2       // 4*(b0+g0)
    vshl.s32        q8, #2        // 4*(b1+g1)
    vadd.s32        q11, q13      // 11 * (c0 + f0) + a0 + h0
    vadd.s32        q12, q14      // 11 * (c1 + f1) + a1 + h1
    vadd.s32        q9, q15       // 40 * (d0 + e0) + 4*(b0+g0)
    vadd.s32        q10, q8       // 40 * (d1 + e1) + 4*(b1+g1)
    vsub.s32        q9, q11       // 40 * (d0 + e0) + 4*(b0+g0) - (11 * (c0 + f0) + a0 + h0)
    vsub.s32        q10, q12      // 40 * (d1 + e1) + 4*(b1+g1) - (11 * (c1 + f1) + a1 + h1) 
.endm

.macro qpel_filter_3_32b_1
    vmov.i16        d16, #17
    vmov.i16        d17, #5
    vmull.s16       q9, d6, d16   // 17 * d0
    vmull.s16       q10, d7, d16  // 17 * d1
    vmull.s16       q11, d4, d17  // 5 * c0
    vmull.s16       q12, d5, d17  // 5* c1
    vmov.i16        d16, #58
    vmull.s16       q13, d8, d16  // 58 * e0
    vmull.s16       q14, d9, d16  // 58 * e1
    vmov.i16        d17, #10
    vmull.s16       q15, d10, d17 // 10 * f0
    vmull.s16       q8, d11, d17  // 10 * f1
    vsub.s32        q9, q11       // 17 * d0 - 5 * c0
    vsub.s32        q10, q12      // 17 * d1 - 5 * c1
    vshll.s16       q11, d12, #2  // 4 * g0
    vshll.s16       q12, d13, #2  // 4 * g1
    vadd.s32        q9, q13       // 17 * d0 - 5 * c0+ 58 * e0
    vadd.s32        q10, q14      // 17 * d1 - 5 * c1 + 58 * e1
    vsubl.s16       q13, d2, d14  // b0 - h0
    vsubl.s16       q14, d3, d15  // b1 - h1
    vadd.s32        q9, q11       // 17 * d0 - 5 * c0+ 58 * e0 +4 * g0
    vadd.s32        q10, q12      // 17 * d1 - 5 * c1 + 58 * e1+4 * g1
    vsub.s32        q13, q15      // 17 * d0 - 5 * c0+ 58 * e0 +4 * g0 -10 * f0
    vsub.s32        q14, q8       // 17 * d1 - 5 * c1 + 58 * e1+4 * g1 - 10*f1
    vadd.s32        q9, q13       //  17 * d0 - 5 * c0+ 58 * e0 +4 * g0 -10 * f0 +b0 - h0
    vadd.s32        q10, q14      // 17 * d1 - 5 * c1 + 58 * e1+4 * g1 - 10*f1 + b1 - h1
.endm

.macro FILTER_VSP a b filterv

    vpush           { q4 - q7}
.loop_\filterv\()_\a\()x\b:

    mov             r7, r2
    mov             r6, r0
    eor             r8, r8

.loop_w8_\filterv\()_\a\()x\b:

    add             r6, r0, r8

    pld [r6]
    vld1.u16         {q0}, [r6], r1
    pld [r6]
    vld1.u16         {q1}, [r6], r1
    pld [r6]
    vld1.u16         {q2}, [r6], r1
    pld [r6]
    vld1.u16         {q3}, [r6], r1
    pld [r6]
    vld1.u16         {q4}, [r6], r1
    pld [r6]
    vld1.u16         {q5}, [r6], r1
    pld [r6]
    vld1.u16         {q6}, [r6], r1
    pld [r6]
    vld1.u16         {q7}, [r6], r1

    veor.u8         q9, q9
    veor.u8         q10, q10

   \filterv

    mov             r12,#1
    lsl             r12, #19
    add             r12, #2048
    vdup.32         q8, r12
    vadd.s32        q9, q8
    vqshrun.s32     d0, q9, #12
    vadd.s32        q10, q8
    vqshrun.s32     d1, q10, #12
    vqmovn.u16      d0, q0
    vst1.u8         d0, [r7]!


    add             r8, #16
    mov             r12, #\a
    lsl             r12, #1
    cmp             r8, r12
    blt             .loop_w8_\filterv\()_\a\()x\b

    add             r0, r1
    add             r2, r3
    subs            r4, #1
    bne             .loop_\filterv\()_\a\()x\b

    vpop            { q4 - q7}

.endm 

.macro LUMA_VSP  w h
function x265_interp_8tap_vert_sp_\w\()x\h\()_neon

    push            {r4, r5, r6, r7, r8}
    ldr             r5, [sp, #4 * 5]
    lsl             r1, #1
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
    FILTER_VSP  \w \h qpel_filter_0_32b_1
    b            5f
1:
    FILTER_VSP  \w \h qpel_filter_1_32b_1
    b            5f
2:
    FILTER_VSP  \w \h qpel_filter_2_32b_1
    b            5f
3:
    FILTER_VSP  \w \h qpel_filter_3_32b_1
    b            5f
5:
    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc
.endm


LUMA_VSP 8 4
LUMA_VSP 8 8
LUMA_VSP 8 16
LUMA_VSP 8 32
LUMA_VSP 16 4
LUMA_VSP 16 8
LUMA_VSP 16 16
LUMA_VSP 16 32
LUMA_VSP 16 64
LUMA_VSP 16 12
LUMA_VSP 32 8
LUMA_VSP 32 16
LUMA_VSP 32 32
LUMA_VSP 32 64
LUMA_VSP 32 24
LUMA_VSP 64 16
LUMA_VSP 64 32
LUMA_VSP 64 64
LUMA_VSP 64 48
LUMA_VSP 24 32
LUMA_VSP 48 64

function x265_interp_8tap_vert_sp_12x16_neon
    push            {r4, r5, r6, r7}
    ldr             r5, [sp, #4 * 4]
    lsl             r1, #1
    mov             r4, r1, lsl #2
    sub             r4, r1
    sub             r0, r4

    mov             r4, #16
    vpush           { q4 - q7}
.loop1_12x16:

    mov             r6, r0
    mov             r7, r2

    pld [r6]
    vld1.u16         {q0}, [r6], r1
    pld [r6]
    vld1.u16         {q1}, [r6], r1
    pld [r6]
    vld1.u8          {q2}, [r6], r1
    pld [r6]
    vld1.u16         {q3}, [r6], r1
    pld [r6]
    vld1.u16         {q4}, [r6], r1
    pld [r6]
    vld1.u16         {q5}, [r6], r1
    pld [r6]
    vld1.u16         {q6}, [r6], r1
    pld [r6]
    vld1.u16         {q7}, [r6], r1

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
0:
    qpel_filter_0_32b_1
    b            5f
1:
    qpel_filter_1_32b_1
    b            5f
2:
    qpel_filter_2_32b_1
    b            5f
3:
    qpel_filter_3_32b_1
    b            5f
5:
    mov             r12,#1
    lsl             r12, #19
    add             r12, #2048
    vdup.32         q8, r12
    vadd.s32        q9, q8
    vqshrun.s32     d0, q9, #12
    vadd.s32        q10, q8
    vqshrun.s32     d1, q10, #12
    vqmovn.u16      d0, q0
    vst1.u8         d0, [r7]!

    add             r6, r0, #16

    pld [r6]
    vld1.u16         {q0}, [r6], r1
    pld [r6]
    vld1.u16         {q1}, [r6], r1
    pld [r6]
    vld1.u8          {q2}, [r6], r1
    pld [r6]
    vld1.u16         {q3}, [r6], r1
    pld [r6]
    vld1.u16         {q4}, [r6], r1
    pld [r6]
    vld1.u16         {q5}, [r6], r1
    pld [r6]
    vld1.u16         {q6}, [r6], r1
    pld [r6]
    vld1.u16         {q7}, [r6], r1

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
0:
    qpel_filter_0_32b_1
    b            5f
1:
    qpel_filter_1_32b_1
    b            5f
2:
    qpel_filter_2_32b_1
    b            5f
3:
    qpel_filter_3_32b_1
    b            5f
5:
    mov             r12,#1
    lsl             r12, #19
    add             r12, #2048
    vdup.32         q8, r12
    vadd.s32        q9, q8
    vqshrun.s32     d0, q9, #12
    vadd.s32        q10, q8
    vqshrun.s32     d1, q10, #12
    vqmovn.u16      d0, q0
    vst1.u32        d0[0], [r7]!

    add             r0, r1
    add             r2, r3
    subs            r4, #1
    bne             .loop1_12x16
    vpop            { q4 - q7}
    pop             {r4, r5, r6, r7}
    bx              lr
endfunc
//**************luma_vps*****************
.macro LUMA_VPS_4xN h
function x265_interp_8tap_vert_ps_4x\h\()_neon
    push           {r4, r5, r6}
    ldr             r4, [sp, #4 * 3]
    lsl             r3, #1
    mov             r5, r4, lsl #6
    mov             r4, r1, lsl #2
    sub             r4, r1
    sub             r0, r4

    mov             r4, #8192
    vdup.32         q8, r4
    mov             r4, #\h

.loop_vps_4x\h:
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

    vsub.s32        q9, q8
    vqmovn.s32      d0, q9
    vst1.u16        d0, [r2], r3

    add             r0, r1
    subs            r4, #1
    bne             .loop_vps_4x\h

    pop             {r4, r5, r6}
    bx              lr
    .ltorg
endfunc
.endm

LUMA_VPS_4xN 4
LUMA_VPS_4xN 8
LUMA_VPS_4xN 16


.macro FILTER_VPS a b filterv

.loop_ps_\filterv\()_\a\()x\b:

    mov             r7, r2
    mov             r6, r0
    eor             r8, r8

.loop_ps_w8_\filterv\()_\a\()x\b:

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

    mov             r12,#8192
    vdup.32         q8, r12
    vsub.s32        q9, q8
    vqmovn.s32      d0, q9
    vsub.s32        q10, q8
    vqmovn.s32      d1, q10
    vst1.u16         {q0}, [r7]!

    add             r8, #8
    cmp             r8, #\a
    blt             .loop_ps_w8_\filterv\()_\a\()x\b

    add             r0, r1
    add             r2, r3
    subs            r4, #1
    bne             .loop_ps_\filterv\()_\a\()x\b 

.endm 

.macro LUMA_VPS  w h
function x265_interp_8tap_vert_ps_\w\()x\h\()_neon

    push            {r4, r5, r6, r7, r8}
    ldr             r5, [sp, #4 * 5]
    lsl             r3, #1
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
    FILTER_VPS  \w \h qpel_filter_0_32b
    b            5f
1:
    FILTER_VPS  \w \h qpel_filter_1_32b
    b            5f
2:
    FILTER_VPS  \w \h qpel_filter_2_32b
    b            5f
3:
    FILTER_VPS  \w \h qpel_filter_3_32b
    b            5f
5:
    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc
.endm

LUMA_VPS 8 4
LUMA_VPS 8 8
LUMA_VPS 8 16
LUMA_VPS 8 32
LUMA_VPS 16 4
LUMA_VPS 16 8
LUMA_VPS 16 16
LUMA_VPS 16 32
LUMA_VPS 16 64
LUMA_VPS 16 12
LUMA_VPS 32 8
LUMA_VPS 32 16
LUMA_VPS 32 32
LUMA_VPS 32 64
LUMA_VPS 32 24
LUMA_VPS 64 16
LUMA_VPS 64 32
LUMA_VPS 64 64
LUMA_VPS 64 48
LUMA_VPS 24 32
LUMA_VPS 48 64

function x265_interp_8tap_vert_ps_12x16_neon
    push            {r4, r5, r6, r7}
    lsl             r3, #1
    ldr             r5, [sp, #4 * 4]
    mov             r4, r1, lsl #2
    sub             r4, r1
    sub             r0, r4

    mov             r4, #16
.loop_vps_12x16:

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
0:
    qpel_filter_0_32b
    b            5f
1:
    qpel_filter_1_32b
    b            5f
2:
    qpel_filter_2_32b
    b            5f
3:
    qpel_filter_3_32b
    b            5f
5:
    mov             r12,#8192
    vdup.32         q8, r12
    vsub.s32        q9, q8
    vqmovn.s32      d0, q9
    vsub.s32        q10, q8
    vqmovn.s32      d1, q10
    vst1.u8         {q0}, [r7]!

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
0:
    qpel_filter_0_32b
    b            5f
1:
    qpel_filter_1_32b
    b            5f
2:
    qpel_filter_2_32b
    b            5f
3:
    qpel_filter_3_32b
    b            5f
5:
    mov             r12,#8192
    vdup.32         q8, r12
    vsub.s32        q9, q8
    vqmovn.s32      d0, q9
    vst1.u8         d0, [r7]!

    add             r0, r1
    add             r2, r3
    subs            r4, #1
    bne             .loop_vps_12x16

    pop             {r4, r5, r6, r7}
    bx              lr
endfunc

//************chroma_vpp************

.macro qpel_filter_chroma_0_32b
    vmov.i16        d16, #64
    vmull.s16       q6, d6, d16    // 64*b0
    vmull.s16       q7, d7, d16   // 64*b1
.endm

.macro qpel_filter_chroma_1_32b
    vmov.i16        d16, #58
    vmov.i16        d17, #10
    vmull.s16       q9, d6, d16     // 58*b0
    vmull.s16       q10, d7, d16     // 58*b1
    vmull.s16       q11, d8, d17    // 10*c0
    vmull.s16       q12, d9, d17    // 10*c1
    vadd.s16        q2, q5          //a +d
    vshll.s16       q13, d4, #1     // 2 * (a0+d0)
    vshll.s16       q14, d5, #1     // 2 * (a1+d1)
    vsub.s32        q9, q13         // 58*b0 - 2 * (a0+d0)
    vsub.s32        q10, q14         // 58*b1 - 2 * (a1+d1)
    vadd.s32        q6, q9, q11         // 58*b0 - 2 * (a0+d0) +10*c0
    vadd.s32        q7, q10, q12         // 58*b1 - 2 * (a1+d1) +10*c1
.endm

.macro qpel_filter_chroma_2_32b
    vmov.i16        d16, #54
    vmull.s16       q9, d6, d16     // 54*b0
    vmull.s16       q10, d7, d16     // 54*b1
    vshll.s16       q11, d4, #2     // 4 * a0
    vshll.s16       q12, d5, #2     // 4 * a1
    vshll.s16       q13, d8, #4     // 16 * c0
    vshll.s16       q14, d9, #4     // 16 * c1
    vshll.s16       q15, d10, #1     // 2 * d0
    vshll.s16       q8, d11, #1     // 2 * d1

    vadd.s32        q9, q13         // 54*b0 + 16 * c0
    vadd.s32        q10, q14        // 54*b1 + 16 * c1
    vadd.s32        q11, q15         // 4 * a0 +2 * d0
    vadd.s32        q12, q8         // 4 * a1 +2 * d1
    vsub.s32        q6, q9, q11     // 54*b0 + 16 * c0 - ( 4 * a0 +2 * d0)
    vsub.s32        q7, q10, q12     // 54*b0 + 16 * c0 - ( 4 * a0 +2 * d0)
.endm

.macro qpel_filter_chroma_3_32b
    vmov.i16        d16, #46
    vmov.i16        d17, #28
    vmull.s16       q9, d6, d16     // 46*b0
    vmull.s16       q10, d7, d16     // 46*b1
    vmull.s16       q11, d8, d17    // 28*c0
    vmull.s16       q12, d9, d17    // 28*c1
    vmov.i16        d17, #6
    vshll.s16       q13, d10, #2     // 4 * d0
    vshll.s16       q14, d11, #2     // 4 * d1
    vmull.s16       q15, d4, d17    // 6*a0
    vmull.s16       q8, d5, d17    // 6*a1
    vadd.s32        q9, q11         // 46*b0 + 28*c0
    vadd.s32        q10, q12        //  46*b1 + 28*c1
    vadd.s32        q13, q15         // 4 * d0 + 6*a0
    vadd.s32        q14, q8        // 4 * d1 + 6*a1
    vsub.s32        q6, q9, q13         // 46*b0 + 28*c0 -(4 * d0 + 6*a0)
    vsub.s32        q7, q10, q14         //  46*b1 + 28*c1 -(4 * d1 + 6*a1)
.endm

.macro qpel_filter_chroma_4_32b
    vmov.i16        d16, #36
    vadd.s16        q2, q5          // a +d
    vadd.s16        q3, q4          // b+c
    vmull.s16       q9, d6, d16     // 36*(b0 + c0)
    vmull.s16       q10, d7, d16     // 36*(b1 + c1)
    vshll.s16       q11, d4, #2     // 4 * (a0+d0)
    vshll.s16       q12, d5, #2     // 4 * (a1+d1)
    vsub.s32        q6, q9, q11         // 36*(b0 + c0) - ( 4 * (a0+d0))
    vsub.s32        q7, q10, q12         // 36*(b1 + c1) - ( 4 * (a1+d1))
.endm

.macro qpel_filter_chroma_5_32b
    vmov.i16        d16, #46
    vmov.i16        d17, #28
    vmull.s16       q9, d6, d17     // 28*b0
    vmull.s16       q10, d7, d17     // 28*b1
    vmull.s16       q11, d8, d16    // 46*c0
    vmull.s16       q12, d9, d16    // 46*c1
    vmov.i16        d17, #6
    vshll.s16       q13, d4, #2     // 4 * a0
    vshll.s16       q14, d5, #2     // 4 * a1
    vmull.s16       q15, d10, d17    // 6*d0
    vmull.s16       q8, d11, d17    // 6*d1
    vadd.s32        q9, q11         // 28*b0 + 46*c0
    vadd.s32        q10, q12        //  28*b1 + 46*c1
    vadd.s32        q13, q15         // 4 * a0 + 6*d0
    vadd.s32        q14, q8        //  4 * a1 + 6*d1
    vsub.s32        q6, q9, q13         // 28*b0 + 46*c0- (4 * a0 + 6*d0)
    vsub.s32        q7, q10, q14         //   28*b1 + 46*c1- (4 * a1 + 6*d1)
.endm

.macro qpel_filter_chroma_6_32b
    vmov.i16        d16, #54
    vmull.s16       q9, d8, d16     // 54*c0
    vmull.s16       q10, d9, d16     // 54*c1
    vshll.s16       q11, d4, #1     // 2 * a0
    vshll.s16       q12, d5, #1     // 2 * a1
    vshll.s16       q13, d6, #4     // 16 * b0
    vshll.s16       q14, d7, #4     // 16 * b1
    vshll.s16       q15, d10, #2     // 4 * d0
    vshll.s16       q8, d11, #2     // 4 * d1
    vadd.s32        q9, q13         // 54*c0 + 16 * b0
    vadd.s32        q10, q14        // 54*c1 + 16 * b1
    vadd.s32        q11, q15         // 2 * a0 + 4 * d0
    vadd.s32        q12, q8         // 2 * a1 + 4 * d1
    vsub.s32        q6, q9, q11     // 54*c0 + 16 * b0 - ( 2 * a0 + 4 * d0)
    vsub.s32        q7, q10, q12     //  54*c1 + 16 * b1 - ( 2 * a1 + 4 * d1)
.endm

.macro qpel_filter_chroma_7_32b
    vmov.i16        d16, #10
    vmov.i16        d17, #58
    vmull.s16       q9, d6, d16     // 10*b0
    vmull.s16       q10, d7, d16     // 10*b1
    vmull.s16       q11, d8, d17    // 58*c0
    vmull.s16       q12, d9, d17    // 58*c1
    vadd.s16        q2, q5          //a +d
    vshll.s16       q13, d4, #1     // 2 * (a0+d0)
    vshll.s16       q14, d5, #1     // 2 * (a1+d1)
    vsub.s32        q9, q13         // 58*c0 - 2 * (a0+d0)
    vsub.s32        q10, q14         // 58*c1 - 2 * (a1+d1)
    vadd.s32        q6, q9, q11         // 58*c0 - 2 * (a0+d0) +10*b0
    vadd.s32        q7, q10, q12         // 58*c1 - 2 * (a1+d1) +10*b1
.endm

.macro FILTER_CHROMA_VPP a b filterv

    vpush           {q4-q7}

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

    vmovl.u8        q2, d0
    vmovl.u8        q3, d1
    vmovl.u8        q4, d2
    vmovl.u8        q5, d3

    veor.u8         q6, q6
    veor.u8         q7, q7

   \filterv

    mov             r12,#32
    vdup.32         q8, r12
    vadd.s32        q6, q8
    vqshrun.s32     d0, q6, #6
    vadd.s32        q7, q8
    vqshrun.s32     d1, q7, #6
    vqmovn.u16      d0, q0
    vst1.u8         d0, [r7]!

    add             r8, #8
    cmp             r8, #\a
    blt             .loop_w8_\filterv\()_\a\()x\b

    add             r0, r1
    add             r2, r3
    subs            r4, #1
    bne             .loop_\filterv\()_\a\()x\b 
    vpop            {q4-q7}
.endm 

.macro CHROMA_VPP  w h
function x265_interp_4tap_vert_pp_\w\()x\h\()_neon

    push            {r4, r5, r6, r7, r8}
    ldr             r5, [sp, #4 * 5]
    sub             r0, r1
    mov             r4, #\h

    cmp             r5, #0
    beq              0f
    cmp             r5, #1
    beq              1f
    cmp             r5, #2
    beq              2f
    cmp             r5, #3
    beq              3f
    cmp             r5, #4
    beq              4f
    cmp             r5, #5
    beq              5f
    cmp             r5, #6
    beq              6f
    cmp             r5, #7
    beq              7f
0:
    FILTER_CHROMA_VPP  \w \h qpel_filter_chroma_0_32b
    b            8f
1:
    FILTER_CHROMA_VPP  \w \h qpel_filter_chroma_1_32b
    b            8f
2:
    FILTER_CHROMA_VPP  \w \h qpel_filter_chroma_2_32b
    b            8f
3:
    FILTER_CHROMA_VPP  \w \h qpel_filter_chroma_3_32b
    b            8f
4:
    FILTER_CHROMA_VPP  \w \h qpel_filter_chroma_4_32b
    b            8f
5:
    FILTER_CHROMA_VPP  \w \h qpel_filter_chroma_5_32b
    b            8f
6:
    FILTER_CHROMA_VPP  \w \h qpel_filter_chroma_6_32b
    b            8f
7:
    FILTER_CHROMA_VPP  \w \h qpel_filter_chroma_7_32b
    b            8f
8:
    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc
.endm

CHROMA_VPP 8 2
CHROMA_VPP 8 4
CHROMA_VPP 8 6
CHROMA_VPP 8 8
CHROMA_VPP 8 16
CHROMA_VPP 8 32
CHROMA_VPP 8 12
CHROMA_VPP 8 64
CHROMA_VPP 16 4
CHROMA_VPP 16 8
CHROMA_VPP 16 12
CHROMA_VPP 16 16
CHROMA_VPP 16 32
CHROMA_VPP 16 64
CHROMA_VPP 16 24
CHROMA_VPP 32 8
CHROMA_VPP 32 16
CHROMA_VPP 32 24
CHROMA_VPP 32 32
CHROMA_VPP 32 64
CHROMA_VPP 32 48
CHROMA_VPP 24 32
CHROMA_VPP 24 64
CHROMA_VPP 64 16
CHROMA_VPP 64 32
CHROMA_VPP 64 48
CHROMA_VPP 64 64
CHROMA_VPP 48 64

.macro FILTER_CHROMA_VPS a b filterv

    vpush           {q4-q7}

.loop_vps_\filterv\()_\a\()x\b:

    mov             r7, r2
    mov             r6, r0
    eor             r8, r8

.loop_vps_w8_\filterv\()_\a\()x\b:

    add             r6, r0, r8

    pld [r6]
    vld1.u8         d0, [r6], r1
    pld [r6]
    vld1.u8         d1, [r6], r1
    pld [r6]
    vld1.u8         d2, [r6], r1
    pld [r6]
    vld1.u8         d3, [r6], r1

    vmovl.u8        q2, d0
    vmovl.u8        q3, d1
    vmovl.u8        q4, d2
    vmovl.u8        q5, d3

    veor.u8         q6, q6
    veor.u8         q7, q7

   \filterv

    mov             r12,#8192
    vdup.32         q8, r12
    vsub.s32        q6, q8
    vqmovn.s32      d0, q6
    vsub.s32        q7, q8
    vqmovn.s32      d1, q7
    vst1.u16         {q0}, [r7]!

    add             r8, #8
    cmp             r8, #\a
    blt             .loop_vps_w8_\filterv\()_\a\()x\b

    add             r0, r1
    add             r2, r3
    subs            r4, #1
    bne             .loop_vps_\filterv\()_\a\()x\b 
    vpop            {q4-q7}
.endm 

.macro CHROMA_VPS  w h
function x265_interp_4tap_vert_ps_\w\()x\h\()_neon

    push            {r4, r5, r6, r7, r8}
    ldr             r5, [sp, #4 * 5]
    lsl             r3, #1
    sub             r0, r1
    mov             r4, #\h

    cmp             r5, #0
    beq              0f
    cmp             r5, #1
    beq              1f
    cmp             r5, #2
    beq              2f
    cmp             r5, #3
    beq              3f
    cmp             r5, #4
    beq              4f
    cmp             r5, #5
    beq              5f
    cmp             r5, #6
    beq              6f
    cmp             r5, #7
    beq              7f
0:
    FILTER_CHROMA_VPS  \w \h qpel_filter_chroma_0_32b
    b            8f
1:
    FILTER_CHROMA_VPS  \w \h qpel_filter_chroma_1_32b
    b            8f
2:
    FILTER_CHROMA_VPS  \w \h qpel_filter_chroma_2_32b
    b            8f
3:
    FILTER_CHROMA_VPS  \w \h qpel_filter_chroma_3_32b
    b            8f
4:
    FILTER_CHROMA_VPS  \w \h qpel_filter_chroma_4_32b
    b            8f
5:
    FILTER_CHROMA_VPS  \w \h qpel_filter_chroma_5_32b
    b            8f
6:
    FILTER_CHROMA_VPS  \w \h qpel_filter_chroma_6_32b
    b            8f
7:
    FILTER_CHROMA_VPS  \w \h qpel_filter_chroma_7_32b
    b            8f
8:
    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc
.endm

CHROMA_VPS 8 2
CHROMA_VPS 8 4
CHROMA_VPS 8 6
CHROMA_VPS 8 8
CHROMA_VPS 8 16
CHROMA_VPS 8 32
CHROMA_VPS 8 12
CHROMA_VPS 8 64
CHROMA_VPS 16 4
CHROMA_VPS 16 8
CHROMA_VPS 16 12
CHROMA_VPS 16 16
CHROMA_VPS 16 32
CHROMA_VPS 16 64
CHROMA_VPS 16 24
CHROMA_VPS 32 8
CHROMA_VPS 32 16
CHROMA_VPS 32 24
CHROMA_VPS 32 32
CHROMA_VPS 32 64
CHROMA_VPS 32 48
CHROMA_VPS 24 32
CHROMA_VPS 24 64
CHROMA_VPS 64 16
CHROMA_VPS 64 32
CHROMA_VPS 64 48
CHROMA_VPS 64 64
CHROMA_VPS 48 64

.macro FILTER_CHROMA_VSP a b filterv

    vpush           {q4-q7}

.loop_vsp_\filterv\()_\a\()x\b:

    mov             r7, r2
    mov             r6, r0
    eor             r8, r8

.loop_vsp_w8_\filterv\()_\a\()x\b:

    add             r6, r0, r8

    pld [r6]
    vld1.u16         {q2}, [r6], r1
    pld [r6]
    vld1.u16         {q3}, [r6], r1
    pld [r6]
    vld1.u16         {q4}, [r6], r1
    pld [r6]
    vld1.u16         {q5}, [r6], r1

    veor.u8         q6, q6
    veor.u8         q7, q7

   \filterv

    mov             r12,#1
    lsl             r12, #19
    add             r12, #2048
    vdup.32         q8, r12
    vadd.s32        q6, q8
    vqshrun.s32     d0, q6, #12
    vadd.s32        q7, q8
    vqshrun.s32     d1, q7, #12
    vqmovn.u16      d0, q0
    vst1.u8         d0, [r7]!

    add             r8, #16
    mov             r12, #\a
    lsl             r12, #1
    cmp             r8, r12
    blt             .loop_vsp_w8_\filterv\()_\a\()x\b

    add             r0, r1
    add             r2, r3
    subs            r4, #1
    bne             .loop_vsp_\filterv\()_\a\()x\b 
    vpop            {q4-q7}
.endm 

.macro CHROMA_VSP  w h
function x265_interp_4tap_vert_sp_\w\()x\h\()_neon

    push            {r4, r5, r6, r7, r8}
    ldr             r5, [sp, #4 * 5]
    lsl             r1, #1
    sub             r0, r1
    mov             r4, #\h

    cmp             r5, #0
    beq              0f
    cmp             r5, #1
    beq              1f
    cmp             r5, #2
    beq              2f
    cmp             r5, #3
    beq              3f
    cmp             r5, #4
    beq              4f
    cmp             r5, #5
    beq              5f
    cmp             r5, #6
    beq              6f
    cmp             r5, #7
    beq              7f
0:
    FILTER_CHROMA_VSP  \w \h qpel_filter_chroma_0_32b
    b            8f
1:
    FILTER_CHROMA_VSP  \w \h qpel_filter_chroma_1_32b
    b            8f
2:
    FILTER_CHROMA_VSP  \w \h qpel_filter_chroma_2_32b
    b            8f
3:
    FILTER_CHROMA_VSP  \w \h qpel_filter_chroma_3_32b
    b            8f
4:
    FILTER_CHROMA_VSP  \w \h qpel_filter_chroma_4_32b
    b            8f
5:
    FILTER_CHROMA_VSP  \w \h qpel_filter_chroma_5_32b
    b            8f
6:
    FILTER_CHROMA_VSP  \w \h qpel_filter_chroma_6_32b
    b            8f
7:
    FILTER_CHROMA_VSP  \w \h qpel_filter_chroma_7_32b
    b            8f
8:
    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc
.endm

CHROMA_VSP 8 2
CHROMA_VSP 8 4
CHROMA_VSP 8 6
CHROMA_VSP 8 8
CHROMA_VSP 8 16
CHROMA_VSP 8 32
CHROMA_VSP 8 12
CHROMA_VSP 8 64
CHROMA_VSP 16 4
CHROMA_VSP 16 8
CHROMA_VSP 16 12
CHROMA_VSP 16 16
CHROMA_VSP 16 32
CHROMA_VSP 16 64
CHROMA_VSP 16 24
CHROMA_VSP 32 8
CHROMA_VSP 32 16
CHROMA_VSP 32 24
CHROMA_VSP 32 32
CHROMA_VSP 32 64
CHROMA_VSP 32 48
CHROMA_VSP 24 32
CHROMA_VSP 24 64
CHROMA_VSP 64 16
CHROMA_VSP 64 32
CHROMA_VSP 64 48
CHROMA_VSP 64 64
CHROMA_VSP 48 64

 // void interp_horiz_pp_c(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx)
.macro vextin8
    pld             [r5]
    vld1.8          {q3}, [r5]!
    vext.8          d0, d6, d7, #1
    vext.8          d1, d6, d7, #2
    vext.8          d2, d6, d7, #3
    vext.8          d3, d6, d7, #4
    vext.8          d4, d6, d7, #5
    vext.8          d5, d6, d7, #6
    vext.8          d6, d6, d7, #7
.endm

.macro HPP_FILTER a b filterhpp
    mov             r12,#32
    mov             r6, #\b
    sub             r3, #\a
    mov             r8, #\a
    cmp             r8, #4
    beq             4f
    cmp             r8, #12
    beq             12f
    b               6f
4:
    HPP_FILTER_4 \a \b \filterhpp
    b               5f
12:
    HPP_FILTER_12 \a \b \filterhpp
    b               5f
6:
loop2_hpp_\filterhpp\()_\a\()x\b:
    mov             r7, #\a
    lsr             r7, #3
    mov             r5, r0
    sub             r5, #4
loop3_hpp_\filterhpp\()_\a\()x\b:
    vextin8
    \filterhpp
    vdup.32         q8, r12
    vadd.s32        q9, q8
    vqshrun.s32     d0, q9, #6
    vadd.s32        q10, q8
    vqshrun.s32     d1, q10, #6
    vqmovn.u16      d0, q0
    vst1.u8         d0, [r2]!
    subs            r7, #1
    sub             r5, #8
    bne             loop3_hpp_\filterhpp\()_\a\()x\b
    subs            r6, #1
    add             r0, r1
    add             r2, r3
    bne             loop2_hpp_\filterhpp\()_\a\()x\b
5:
.endm

.macro HPP_FILTER_4 w h filterhpp
loop4_hpp_\filterhpp\()_\w\()x\h:
    mov             r5, r0
    sub             r5, #4
    vextin8
    \filterhpp
    vdup.32         q8, r12
    vadd.s32        q9, q8
    vqshrun.s32     d0, q9, #6
    vadd.s32        q10, q8
    vqshrun.s32     d1, q10, #6
    vqmovn.u16      d0, q0
    vst1.u32        {d0[0]}, [r2]!
    sub             r5, #8
    subs            r6, #1
    add             r0, r1
    add             r2, r3
    bne             loop4_hpp_\filterhpp\()_\w\()x\h
.endm

.macro HPP_FILTER_12 w h filterhpp
loop12_hpp_\filterhpp\()_\w\()x\h:
    mov             r5, r0
    sub             r5, #4
    vextin8
    \filterhpp
    vdup.32         q8, r12
    vadd.s32        q9, q8
    vqshrun.s32     d0, q9, #6
    vadd.s32        q10, q8
    vqshrun.s32     d1, q10, #6
    vqmovn.u16      d0, q0
    vst1.u8         {d0}, [r2]!
    sub             r5, #8

    vextin8
    \filterhpp
    vdup.32         q8, r12
    vadd.s32        q9, q8
    vqshrun.s32     d0, q9, #6
    vadd.s32        q10, q8
    vqshrun.s32     d1, q10, #6
    vqmovn.u16      d0, q0
    vst1.u32        {d0[0]}, [r2]!
    add             r2, r3
    subs            r6, #1
    add             r0, r1
    bne             loop12_hpp_\filterhpp\()_\w\()x\h
.endm

.macro LUMA_HPP w h
function x265_interp_horiz_pp_\w\()x\h\()_neon
    push            {r4, r5, r6, r7, r8}
    ldr             r4, [sp, #20]
    cmp             r4, #0
    beq             0f
    cmp             r4, #1
    beq             1f
    cmp             r4, #2
    beq             2f
    cmp             r4, #3
    beq             3f
0:
    HPP_FILTER  \w \h qpel_filter_0_32b
    b               5f
1:
    HPP_FILTER  \w \h qpel_filter_1_32b
    b               5f
2:
    HPP_FILTER  \w \h qpel_filter_2_32b
    b               5f
3:
    HPP_FILTER  \w \h qpel_filter_3_32b
    b               5f
5:
    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc
.endm

LUMA_HPP    4 4
LUMA_HPP    4 8
LUMA_HPP    4 16
LUMA_HPP    8 4
LUMA_HPP    8 8
LUMA_HPP    8 16
LUMA_HPP    8 32
LUMA_HPP    12 16
LUMA_HPP    16 4
LUMA_HPP    16 8
LUMA_HPP    16 12
LUMA_HPP    16 16
LUMA_HPP    16 32
LUMA_HPP    16 64
LUMA_HPP    24 32
LUMA_HPP    32 8
LUMA_HPP    32 16
LUMA_HPP    32 24
LUMA_HPP    32 32
LUMA_HPP    32 64
LUMA_HPP    48 64
LUMA_HPP    64 16
LUMA_HPP    64 32
LUMA_HPP    64 48
LUMA_HPP    64 64

// void interp_horiz_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
.macro HPS_FILTER a b filterhps
    mov             r12, #8192
    mov             r6, r10
    sub             r3, #\a
    lsl             r3, #1

    mov             r8, #\a
    cmp             r8, #4
    beq             14f
    cmp             r8, #12
    beq             15f
    b               7f
14:
    HPS_FILTER_4 \a \b \filterhps
    b               10f
15:
    HPS_FILTER_12 \a \b \filterhps
    b               10f
7:
    cmp             r9, #0
    beq             8f
    cmp             r9, #1
    beq             9f
8:
loop1_hps_\filterhps\()_\a\()x\b\()_rowext0:
    mov             r7, #\a
    lsr             r7, #3
    mov             r5, r0
    sub             r5, #4
loop2_hps_\filterhps\()_\a\()x\b\()_rowext0:
    vextin8
    \filterhps
    vdup.32         q8, r12
    vsub.s32        q9, q8
    vsub.s32        q10, q8
    vmovn.u32       d0, q9
    vmovn.u32       d1, q10
    vst1.s16        {q0}, [r2]!
    subs            r7, #1
    sub             r5, #8
    bne             loop2_hps_\filterhps\()_\a\()x\b\()_rowext0
    subs            r6, #1
    add             r0, r1
    add             r2, r3
    bne             loop1_hps_\filterhps\()_\a\()x\b\()_rowext0
    b               10f
9:
loop3_hps_\filterhps\()_\a\()x\b\()_rowext1:
    mov             r7, #\a
    lsr             r7, #3
    mov             r5, r0
    sub             r5, #4
loop4_hps_\filterhps\()_\a\()x\b\()_rowext1:
    vextin8
    \filterhps
    vdup.32         q8, r12
    vsub.s32        q9, q8
    vsub.s32        q10, q8
    vmovn.u32       d0, q9
    vmovn.u32       d1, q10
    vst1.s16        {q0}, [r2]!
    subs            r7, #1
    sub             r5, #8
    bne             loop4_hps_\filterhps\()_\a\()x\b\()_rowext1
    subs            r6, #1
    add             r0, r1
    add             r2, r3
    bne             loop3_hps_\filterhps\()_\a\()x\b\()_rowext1
10:
.endm

.macro HPS_FILTER_4 w h filterhps
    cmp             r9, #0
    beq             11f
    cmp             r9, #1
    beq             12f
11:
loop4_hps_\filterhps\()_\w\()x\h\()_rowext0:
    mov             r5, r0
    sub             r5, #4
    vextin8
    \filterhps
    vdup.32         q8, r12
    vsub.s32        q9, q8
    vmovn.u32       d0, q9
    vst1.s16        {d0}, [r2]!
    sub             r5, #8
    subs            r6, #1
    add             r0, r1
    add             r2, r3
    bne             loop4_hps_\filterhps\()_\w\()x\h\()_rowext0
    b               13f
12:
loop5_hps_\filterhps\()_\w\()x\h\()_rowext1:
    mov             r5, r0
    sub             r5, #4
    vextin8
    \filterhps
    vdup.32         q8, r12
    vsub.s32        q9, q8
    vmovn.u32       d0, q9
    vst1.s16        {d0}, [r2]!
    sub             r5, #8
    subs            r6, #1
    add             r0, r1
    add             r2, r3
    bne             loop5_hps_\filterhps\()_\w\()x\h\()_rowext1
13:
.endm

.macro HPS_FILTER_12 w h filterhps
    cmp             r9, #0
    beq             14f
    cmp             r9, #1
    beq             15f
14:
loop12_hps_\filterhps\()_\w\()x\h\()_rowext0:
    mov             r5, r0
    sub             r5, #4
    vextin8
    \filterhps
    vdup.32         q8, r12
    vsub.s32        q9, q8
    vsub.s32        q10, q8
    vmovn.u32       d0, q9
    vmovn.u32       d1, q10
    vst1.s16        {q0}, [r2]!
    sub             r5, #8

    vextin8
    \filterhps
    vdup.32         q8, r12
    vsub.s32        q9, q8
    vmovn.u32       d0, q9
    vst1.s16        {d0}, [r2]!
    add             r2, r3
    subs            r6, #1
    add             r0, r1
    bne             loop12_hps_\filterhps\()_\w\()x\h\()_rowext0
    b               16f
15:
loop12_hps_\filterhps\()_\w\()x\h\()_rowext1:
    mov             r5, r0
    sub             r5, #4
    vextin8
    \filterhps
    vdup.32         q8, r12
    vsub.s32        q9, q8
    vsub.s32        q10, q8
    vmovn.u32       d0, q9
    vmovn.u32       d1, q10
    vst1.s16        {q0}, [r2]!
    sub             r5, #8

    vextin8
    \filterhps
    vdup.32         q8, r12
    vsub.s32        q9, q8
    vmovn.u32       d0, q9
    vst1.s16        {d0}, [r2]!
    add             r2, r3
    subs            r6, #1
    add             r0, r1
    bne             loop12_hps_\filterhps\()_\w\()x\h\()_rowext1
16:
.endm

.macro LUMA_HPS w h
function x265_interp_horiz_ps_\w\()x\h\()_neon
    push            {r4, r5, r6, r7, r8, r9, r10}
    ldr             r4, [sp, #28]
    ldr             r9, [sp, #32]
    mov             r10, #\h
    cmp             r9, #0
    beq             6f
    sub             r0, r0, r1, lsl #2
    add             r0, r1
    add             r10, #7
6:
    cmp             r4, #0
    beq             0f
    cmp             r4, #1
    beq             1f
    cmp             r4, #2
    beq             2f
    cmp             r4, #3
    beq             3f
0:
    HPS_FILTER  \w \h qpel_filter_0_32b
    b               5f
1:
    HPS_FILTER  \w \h qpel_filter_1_32b
    b               5f
2:
    HPS_FILTER  \w \h qpel_filter_2_32b
    b               5f
3:
    HPS_FILTER  \w \h qpel_filter_3_32b
    b               5f
5:
    pop             {r4, r5, r6, r7, r8, r9, r10}
    bx              lr
endfunc
.endm

LUMA_HPS    4 4
LUMA_HPS    4 8
LUMA_HPS    4 16
LUMA_HPS    8 4
LUMA_HPS    8 8
LUMA_HPS    8 16
LUMA_HPS    8 32
LUMA_HPS    12 16
LUMA_HPS    16 4
LUMA_HPS    16 8
LUMA_HPS    16 12
LUMA_HPS    16 16
LUMA_HPS    16 32
LUMA_HPS    16 64
LUMA_HPS    24 32
LUMA_HPS    32 8
LUMA_HPS    32 16
LUMA_HPS    32 24
LUMA_HPS    32 32
LUMA_HPS    32 64
LUMA_HPS    48 64
LUMA_HPS    64 16
LUMA_HPS    64 32
LUMA_HPS    64 48
LUMA_HPS    64 64

// ******* Chroma_hpp *******
.macro vextin8_chroma
    pld             [r5]
    vld1.8          {q3}, [r5]!
    vext.8          d0, d6, d7, #1
    vext.8          d1, d6, d7, #2
    vext.8          d2, d6, d7, #3
    vext.8          d3, d6, d7, #4

    vmovl.u8        q2, d0
    vmovl.u8        q3, d1
    vmovl.u8        q4, d2
    vmovl.u8        q5, d3
.endm

.macro FILTER_CHROMA_HPP a b filterhpp
    vpush           {q4-q7}
    mov             r12,#32
    mov             r6, #\b
    sub             r3, #\a
    mov             r8, #\a
    cmp             r8, #4
    beq             11f
    cmp             r8, #12
    beq             12f
    b               13f
11:
    FILTER_CHROMA_HPP_4 \a \b \filterhpp
    b               14f
12:
    FILTER_CHROMA_HPP_12 \a \b \filterhpp
    b               14f
13:
    veor            q6, q6
    veor            q7, q7

loop2_hpp_\filterhpp\()_\a\()x\b:
    mov             r7, #\a
    lsr             r7, #3
    mov             r5, r0
    sub             r5, #2
loop3_hpp_\filterhpp\()_\a\()x\b:
    vextin8_chroma
    \filterhpp
    vdup.32         q8, r12
    vadd.s32        q6, q8
    vqshrun.s32     d0, q6, #6
    vadd.s32        q7, q8
    vqshrun.s32     d1, q7, #6
    vqmovn.u16      d0, q0
    vst1.u8         d0, [r2]!
    subs            r7, #1
    sub             r5, #8
    bne             loop3_hpp_\filterhpp\()_\a\()x\b
    subs            r6, #1
    add             r0, r1
    add             r2, r3
    bne             loop2_hpp_\filterhpp\()_\a\()x\b
14:
    vpop            {q4-q7}
.endm

.macro FILTER_CHROMA_HPP_4 w h filterhpp
loop4_hpp_\filterhpp\()_\w\()x\h:
    mov             r5, r0
    sub             r5, #2
    vextin8_chroma
    \filterhpp
    vdup.32         q8, r12
    vadd.s32        q6, q8
    vqshrun.s32     d0, q6, #6
    vadd.s32        q7, q8
    vqshrun.s32     d1, q7, #6
    vqmovn.u16      d0, q0
    vst1.u32        {d0[0]}, [r2]!
    sub             r5, #8
    subs            r6, #1
    add             r0, r1
    add             r2, r3
    bne             loop4_hpp_\filterhpp\()_\w\()x\h
.endm

.macro FILTER_CHROMA_HPP_12 w h filterhpp
loop12_hpp_\filterhpp\()_\w\()x\h:
    mov             r5, r0
    sub             r5, #2
    vextin8_chroma
    \filterhpp
    vdup.32         q8, r12
    vadd.s32        q6, q8
    vqshrun.s32     d0, q6, #6
    vadd.s32        q7, q8
    vqshrun.s32     d1, q7, #6
    vqmovn.u16      d0, q0
    vst1.u8         {d0}, [r2]!
    sub             r5, #8

    vextin8_chroma
    \filterhpp
    vdup.32         q8, r12
    vadd.s32        q6, q8
    vqshrun.s32     d0, q6, #6
    vadd.s32        q7, q8
    vqshrun.s32     d1, q7, #6
    vqmovn.u16      d0, q0
    vst1.u32        {d0[0]}, [r2]!
    add             r2, r3
    subs            r6, #1
    add             r0, r1
    bne             loop12_hpp_\filterhpp\()_\w\()x\h
.endm

.macro CHROMA_HPP  w h
function x265_interp_4tap_horiz_pp_\w\()x\h\()_neon

    push            {r4, r5, r6, r7, r8}
    ldr             r4, [sp, #4 * 5]

    cmp             r4, #0
    beq              0f
    cmp             r4, #1
    beq              1f
    cmp             r4, #2
    beq              2f
    cmp             r4, #3
    beq              3f
    cmp             r4, #4
    beq              4f
    cmp             r4, #5
    beq              5f
    cmp             r4, #6
    beq              6f
    cmp             r4, #7
    beq              7f
0:
    FILTER_CHROMA_HPP  \w \h qpel_filter_chroma_0_32b
    b            8f
1:
    FILTER_CHROMA_HPP  \w \h qpel_filter_chroma_1_32b
    b            8f
2:
    FILTER_CHROMA_HPP  \w \h qpel_filter_chroma_2_32b
    b            8f
3:
    FILTER_CHROMA_HPP  \w \h qpel_filter_chroma_3_32b
    b            8f
4:
    FILTER_CHROMA_HPP  \w \h qpel_filter_chroma_4_32b
    b            8f
5:
    FILTER_CHROMA_HPP  \w \h qpel_filter_chroma_5_32b
    b            8f
6:
    FILTER_CHROMA_HPP  \w \h qpel_filter_chroma_6_32b
    b            8f
7:
    FILTER_CHROMA_HPP  \w \h qpel_filter_chroma_7_32b

8:
    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc
.endm

CHROMA_HPP 4 2
CHROMA_HPP 4 4
CHROMA_HPP 4 8
CHROMA_HPP 4 16
CHROMA_HPP 4 32
CHROMA_HPP 8 2
CHROMA_HPP 8 4
CHROMA_HPP 8 6
CHROMA_HPP 8 8
CHROMA_HPP 8 12
CHROMA_HPP 8 16
CHROMA_HPP 8 32
CHROMA_HPP 8 64
CHROMA_HPP 12 16
CHROMA_HPP 12 32
CHROMA_HPP 16 4
CHROMA_HPP 16 8
CHROMA_HPP 16 12
CHROMA_HPP 16 16
CHROMA_HPP 16 24
CHROMA_HPP 16 32
CHROMA_HPP 16 64
CHROMA_HPP 24 32
CHROMA_HPP 24 64
CHROMA_HPP 32 8
CHROMA_HPP 32 16
CHROMA_HPP 32 24
CHROMA_HPP 32 32
CHROMA_HPP 32 48
CHROMA_HPP 32 64
CHROMA_HPP 48 64
CHROMA_HPP 64 16
CHROMA_HPP 64 32
CHROMA_HPP 64 48
CHROMA_HPP 64 64
// ***** Chroma_hps *****
.macro FILTER_CHROMA_HPS a b filterhps
    vpush           {q4-q7}
    mov             r12, #8192
    mov             r6, r10
    sub             r3, #\a
    lsl             r3, #1

    mov             r8, #\a
    cmp             r8, #4
    beq             14f
    cmp             r8, #12
    beq             15f
    b               16f
14:
    FILTER_CHROMA_HPS_4 \a \b \filterhps
    b               10f
15:
    FILTER_CHROMA_HPS_12 \a \b \filterhps
    b               10f
16:
    cmp             r9, #0
    beq             17f
    cmp             r9, #1
    beq             18f
17:
loop1_hps_\filterhps\()_\a\()x\b\()_rowext0:
    mov             r7, #\a
    lsr             r7, #3
    mov             r5, r0
    sub             r5, #2
loop2_hps_\filterhps\()_\a\()x\b\()_rowext0:
    vextin8_chroma
    \filterhps
    vdup.32         q8, r12
    vsub.s32        q6, q8
    vsub.s32        q7, q8
    vmovn.u32       d0, q6
    vmovn.u32       d1, q7
    vst1.s16        {q0}, [r2]!
    subs            r7, #1
    sub             r5, #8
    bne             loop2_hps_\filterhps\()_\a\()x\b\()_rowext0
    subs            r6, #1
    add             r0, r1
    add             r2, r3
    bne             loop1_hps_\filterhps\()_\a\()x\b\()_rowext0
    b               10f
18:
loop3_hps_\filterhps\()_\a\()x\b\()_rowext1:
    mov             r7, #\a
    lsr             r7, #3
    mov             r5, r0
    sub             r5, #2
loop4_hps_\filterhps\()_\a\()x\b\()_rowext1:
    vextin8_chroma
    \filterhps
    vdup.32         q8, r12
    vsub.s32        q6, q8
    vsub.s32        q7, q8
    vmovn.u32       d0, q6
    vmovn.u32       d1, q7
    vst1.s16        {q0}, [r2]!
    subs            r7, #1
    sub             r5, #8
    bne             loop4_hps_\filterhps\()_\a\()x\b\()_rowext1
    subs            r6, #1
    add             r0, r1
    add             r2, r3
    bne             loop3_hps_\filterhps\()_\a\()x\b\()_rowext1
10:
    vpop            {q4-q7}
.endm

.macro FILTER_CHROMA_HPS_4 w h filterhps
    cmp             r9, #0
    beq             19f
    cmp             r9, #1
    beq             20f
19:
loop4_hps_\filterhps\()_\w\()x\h\()_rowext0:
    mov             r5, r0
    sub             r5, #2
    vextin8_chroma
    \filterhps
    vdup.32         q8, r12
    vsub.s32        q6, q8
    vmovn.u32       d0, q6
    vst1.s16        {d0}, [r2]!
    sub             r5, #8
    subs            r6, #1
    add             r0, r1
    add             r2, r3
    bne             loop4_hps_\filterhps\()_\w\()x\h\()_rowext0
    b               21f
20:
loop5_hps_\filterhps\()_\w\()x\h\()_rowext1:
    mov             r5, r0
    sub             r5, #2
    vextin8_chroma
    \filterhps
    vdup.32         q8, r12
    vsub.s32        q6, q8
    vmovn.u32       d0, q6
    vst1.s16        {d0}, [r2]!
    sub             r5, #8
    subs            r6, #1
    add             r0, r1
    add             r2, r3
    bne             loop5_hps_\filterhps\()_\w\()x\h\()_rowext1
21:
.endm

.macro FILTER_CHROMA_HPS_12 w h filterhpp
    cmp             r9, #0
    beq             22f
    cmp             r9, #1
    beq             23f
22:
loop12_hps_\filterhpp\()_\w\()x\h\()_rowext0:
    mov             r5, r0
    sub             r5, #2
    vextin8_chroma
    \filterhpp
    vdup.32         q8, r12
    vsub.s32        q6, q8
    vsub.s32        q7, q8
    vmovn.u32       d0, q6
    vmovn.u32       d1, q7
    vst1.s16        {q0}, [r2]!
    sub             r5, #8

    vextin8_chroma
    \filterhpp
    vdup.32         q8, r12
    vsub.s32        q6, q8
    vmovn.u32       d0, q6
    vst1.s16        {d0}, [r2]!
    add             r2, r3
    subs            r6, #1
    add             r0, r1
    bne             loop12_hps_\filterhpp\()_\w\()x\h\()_rowext0
    b               24f
23:
loop12_hps_\filterhpp\()_\w\()x\h\()_rowext1:
    mov             r5, r0
    sub             r5, #2
    vextin8_chroma
    \filterhpp
    vdup.32         q8, r12
    vsub.s32        q6, q8
    vsub.s32        q7, q8
    vmovn.u32       d0, q6
    vmovn.u32       d1, q7
    vst1.s16        {q0}, [r2]!
    sub             r5, #8

    vextin8_chroma
    \filterhpp
    vdup.32         q8, r12
    vsub.s32        q6, q8
    vmovn.u32       d0, q6
    vst1.s16        {d0}, [r2]!
    add             r2, r3
    subs            r6, #1
    add             r0, r1
    bne             loop12_hps_\filterhpp\()_\w\()x\h\()_rowext1
24:
.endm

.macro CHROMA_HPS w h
function x265_interp_4tap_horiz_ps_\w\()x\h\()_neon
    push            {r4, r5, r6, r7, r8, r9, r10}
    ldr             r4, [sp, #28]
    ldr             r9, [sp, #32]
    mov             r10, #\h
    cmp             r9, #0
    beq             9f
    sub             r0, r1
    add             r10, #3
9:
    cmp             r4, #0
    beq              0f
    cmp             r4, #1
    beq              1f
    cmp             r4, #2
    beq              2f
    cmp             r4, #3
    beq              3f
    cmp             r4, #4
    beq              4f
    cmp             r4, #5
    beq              5f
    cmp             r4, #6
    beq              6f
    cmp             r4, #7
    beq              7f
0:
    FILTER_CHROMA_HPS  \w \h qpel_filter_chroma_0_32b
    b            8f
1:
    FILTER_CHROMA_HPS  \w \h qpel_filter_chroma_1_32b
    b            8f
2:
    FILTER_CHROMA_HPS  \w \h qpel_filter_chroma_2_32b
    b            8f
3:
    FILTER_CHROMA_HPS  \w \h qpel_filter_chroma_3_32b
    b            8f
4:
    FILTER_CHROMA_HPS  \w \h qpel_filter_chroma_4_32b
    b            8f
5:
    FILTER_CHROMA_HPS  \w \h qpel_filter_chroma_5_32b
    b            8f
6:
    FILTER_CHROMA_HPS  \w \h qpel_filter_chroma_6_32b
    b            8f
7:
    FILTER_CHROMA_HPS  \w \h qpel_filter_chroma_7_32b

8:
    pop             {r4, r5, r6, r7, r8, r9, r10}
    bx              lr
endfunc
.endm

CHROMA_HPS 4 2
CHROMA_HPS 4 4
CHROMA_HPS 4 8
CHROMA_HPS 4 16
CHROMA_HPS 4 32
CHROMA_HPS 8 2
CHROMA_HPS 8 4
CHROMA_HPS 8 6
CHROMA_HPS 8 8
CHROMA_HPS 8 12
CHROMA_HPS 8 16
CHROMA_HPS 8 32
CHROMA_HPS 8 64
CHROMA_HPS 12 16
CHROMA_HPS 12 32
CHROMA_HPS 16 4
CHROMA_HPS 16 8
CHROMA_HPS 16 12
CHROMA_HPS 16 16
CHROMA_HPS 16 24
CHROMA_HPS 16 32
CHROMA_HPS 16 64
CHROMA_HPS 24 32
CHROMA_HPS 24 64
CHROMA_HPS 32 8
CHROMA_HPS 32 16
CHROMA_HPS 32 24
CHROMA_HPS 32 32
CHROMA_HPS 32 48
CHROMA_HPS 32 64
CHROMA_HPS 48 64
CHROMA_HPS 64 16
CHROMA_HPS 64 32
CHROMA_HPS 64 48
CHROMA_HPS 64 64
