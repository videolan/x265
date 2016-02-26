/*****************************************************************************
 * Copyright (C) 2016 x265 project
 *
 * Authors: Radhakrishnan VR <radhakrishnan@multicorewareinc.com>
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

/* void blockcopy_sp(pixel* a, intptr_t stridea, const int16_t* b, intptr_t strideb)
 *
 * r0   - a
 * r1   - stridea
 * r2   - b
 * r3   - strideb */
function x265_blockcopy_sp_4x4_neon
    lsl             r3, #1
.rept 2
    vld1.u16        {q0}, [r2], r3
    vld1.u16        {q1}, [r2], r3
    vmovn.u16       d0, q0
    vmovn.u16       d1, q1
    vst1.u32        {d0[0]}, [r0], r1
    vst1.u32        {d1[0]}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_sp_8x8_neon
    lsl             r3, #1
.rept 4
    vld1.u16        {q0}, [r2], r3
    vld1.u16        {q1}, [r2], r3
    vmovn.u16       d0, q0
    vmovn.u16       d1, q1
    vst1.u8         {d0}, [r0], r1
    vst1.u8         {d1}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_sp_16x16_neon
    lsl             r3, #1
.rept 8
    vld1.u16        {q0, q1}, [r2], r3
    vld1.u16        {q2, q3}, [r2], r3
    vmovn.u16       d0, q0
    vmovn.u16       d1, q1
    vmovn.u16       d2, q2
    vmovn.u16       d3, q3
    vst1.u8         {q0}, [r0], r1
    vst1.u8         {q1}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_sp_32x32_neon
    mov             r12, #4
    lsl             r3, #1
    sub             r3, #32
loop_csp32:
    subs            r12, #1
.rept 4
    vld1.u16        {q0, q1}, [r2]!
    vld1.u16        {q2, q3}, [r2], r3
    vld1.u16        {q8, q9}, [r2]!
    vld1.u16        {q10, q11}, [r2], r3

    vmovn.u16       d0, q0
    vmovn.u16       d1, q1
    vmovn.u16       d2, q2
    vmovn.u16       d3, q3

    vmovn.u16       d4, q8
    vmovn.u16       d5, q9
    vmovn.u16       d6, q10
    vmovn.u16       d7, q11

    vst1.u8         {q0, q1}, [r0], r1
    vst1.u8         {q2, q3}, [r0], r1
.endr
    bne             loop_csp32
    bx              lr
endfunc

function x265_blockcopy_sp_64x64_neon
    mov             r12, #16
    lsl             r3, #1
    sub             r3, #96
    sub             r1, #32
loop_csp64:
    subs            r12, #1
.rept 4
    vld1.u16        {q0, q1}, [r2]!
    vld1.u16        {q2, q3}, [r2]!
    vld1.u16        {q8, q9}, [r2]!
    vld1.u16        {q10, q11}, [r2], r3

    vmovn.u16       d0, q0
    vmovn.u16       d1, q1
    vmovn.u16       d2, q2
    vmovn.u16       d3, q3

    vmovn.u16       d4, q8
    vmovn.u16       d5, q9
    vmovn.u16       d6, q10
    vmovn.u16       d7, q11

    vst1.u8         {q0, q1}, [r0]!
    vst1.u8         {q2, q3}, [r0], r1
.endr
    bne             loop_csp64
    bx              lr
endfunc

// void blockcopy_ps(int16_t* a, intptr_t stridea, const pixel* b, intptr_t strideb)
function x265_blockcopy_ps_4x4_neon
    lsl             r1, #1
.rept 2
    vld1.u8         {d0}, [r2], r3
    vld1.u8         {d1}, [r2], r3
    vmovl.u8        q1, d0
    vmovl.u8        q2, d1
    vst1.u16        {d2}, [r0], r1
    vst1.u16        {d4}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_ps_8x8_neon
    lsl             r1, #1
.rept 4
    vld1.u8         {d0}, [r2], r3
    vld1.u8         {d1}, [r2], r3
    vmovl.u8        q1, d0
    vmovl.u8        q2, d1
    vst1.u16        {q1}, [r0], r1
    vst1.u16        {q2}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_ps_16x16_neon
    lsl             r1, #1
.rept 8
    vld1.u8         {q0}, [r2], r3
    vld1.u8         {q1}, [r2], r3
    vmovl.u8        q8, d0
    vmovl.u8        q9, d1
    vmovl.u8        q10, d2
    vmovl.u8        q11, d3
    vst1.u16        {q8, q9}, [r0], r1
    vst1.u16        {q10, q11}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_ps_32x32_neon
    lsl             r1, #1
    sub             r1, #32
    mov             r12, #4
loop_cps32:
    subs            r12, #1
.rept 4
    vld1.u8         {q0, q1}, [r2], r3
    vld1.u8         {q2, q3}, [r2], r3
    vmovl.u8        q8, d0
    vmovl.u8        q9, d1
    vmovl.u8        q10, d2
    vmovl.u8        q11, d3

    vmovl.u8        q12, d4
    vmovl.u8        q13, d5
    vmovl.u8        q14, d6
    vmovl.u8        q15, d7

    vst1.u16        {q8, q9}, [r0]!
    vst1.u16        {q10, q11}, [r0], r1
    vst1.u16        {q12, q13}, [r0]!
    vst1.u16        {q14, q15}, [r0], r1
.endr
    bne             loop_cps32
    bx              lr
endfunc

function x265_blockcopy_ps_64x64_neon
    lsl             r1, #1
    sub             r1, #96
    sub             r3, #32
    mov             r12, #16
loop_cps64:
    subs            r12, #1
.rept 4
    vld1.u8         {q0, q1}, [r2]!
    vld1.u8         {q2, q3}, [r2], r3
    vmovl.u8        q8, d0
    vmovl.u8        q9, d1
    vmovl.u8        q10, d2
    vmovl.u8        q11, d3

    vmovl.u8        q12, d4
    vmovl.u8        q13, d5
    vmovl.u8        q14, d6
    vmovl.u8        q15, d7

    vst1.u16        {q8, q9}, [r0]!
    vst1.u16        {q10, q11}, [r0]!
    vst1.u16        {q12, q13}, [r0]!
    vst1.u16        {q14, q15}, [r0], r1
.endr
    bne             loop_cps64
    bx              lr
endfunc

// void x265_blockcopy_ss(int16_t* a, intptr_t stridea, const int16_t* b, intptr_t strideb)
function x265_blockcopy_ss_4x4_neon
    lsl             r1, #1
    lsl             r3, #1
.rept 2
    vld1.u16        {d0}, [r2], r3
    vld1.u16        {d1}, [r2], r3
    vst1.u16        {d0}, [r0], r1
    vst1.u16        {d1}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_ss_8x8_neon
    lsl             r1, #1
    lsl             r3, #1
.rept 4
    vld1.u16        {q0}, [r2], r3
    vld1.u16        {q1}, [r2], r3
    vst1.u16        {q0}, [r0], r1
    vst1.u16        {q1}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_ss_16x16_neon
    lsl             r1, #1
    lsl             r3, #1
.rept 8
    vld1.u16        {q0, q1}, [r2], r3
    vld1.u16        {q2, q3}, [r2], r3
    vst1.u16        {q0, q1}, [r0], r1
    vst1.u16        {q2, q3}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_ss_32x32_neon
    lsl             r1, #1
    lsl             r3, #1
    mov             r12, #4
    sub             r1, #32
    sub             r3, #32
loop_css32:
    subs            r12, #1
.rept 8
    vld1.u16        {q0, q1}, [r2]!
    vld1.u16        {q2, q3}, [r2], r3
    vst1.u16        {q0, q1}, [r0]!
    vst1.u16        {q2, q3}, [r0], r1
.endr
    bne             loop_css32
    bx              lr
endfunc

function x265_blockcopy_ss_64x64_neon
    lsl             r1, #1
    lsl             r3, #1
    mov             r12, #8
    sub             r1, #96
    sub             r3, #96
loop_css64:
    subs            r12, #1
.rept 8
    vld1.u16        {q0, q1}, [r2]!
    vld1.u16        {q2, q3}, [r2]!
    vld1.u16        {q8, q9}, [r2]!
    vld1.u16        {q10, q11}, [r2], r3

    vst1.u16        {q0, q1}, [r0]!
    vst1.u16        {q2, q3}, [r0]!
    vst1.u16        {q8, q9}, [r0]!
    vst1.u16        {q10, q11}, [r0], r1
.endr
    bne             loop_css64
    bx              lr
endfunc
