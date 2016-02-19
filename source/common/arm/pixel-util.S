/*****************************************************************************
 * Copyright (C) 2016 x265 project
 *
 * Authors: Dnyaneshwar G <dnyaneshwar@multicorewareinc.com>
 *          Radhakrishnan VR <radhakrishnan@multicorewareinc.com>
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

.macro VAR_SQR_SUM qsqr_sum, qsqr_last, qsqr_temp, dsrc, num=0, vpadal=vpadal.u16
    vmull.u8        \qsqr_temp, \dsrc, \dsrc
    vaddw.u8        q\num, q\num, \dsrc
    \vpadal         \qsqr_sum, \qsqr_last
.endm

function x265_pixel_var_8x8_neon
    vld1.u8         {d16}, [r0], r1
    vmull.u8        q1, d16, d16
    vmovl.u8        q0, d16
    vld1.u8         {d18}, [r0], r1
    vmull.u8        q2, d18, d18
    vaddw.u8        q0, q0, d18

    vld1.u8         {d20}, [r0], r1
    VAR_SQR_SUM     q1, q1, q3, d20, 0, vpaddl.u16
    vld1.u8         {d22}, [r0], r1
    VAR_SQR_SUM     q2, q2, q8, d22, 0, vpaddl.u16

    vld1.u8         {d24}, [r0], r1
    VAR_SQR_SUM     q1, q3, q9, d24
    vld1.u8         {d26}, [r0], r1
    VAR_SQR_SUM     q2, q8, q10, d26
    vld1.u8         {d24}, [r0], r1
    VAR_SQR_SUM     q1, q9, q14, d24
    vld1.u8         {d26}, [r0], r1
    VAR_SQR_SUM     q2, q10, q15, d26

    vpaddl.u16      q8, q14
    vpaddl.u16      q9, q15
    vadd.u32        q1, q1, q8
    vadd.u16        d0, d0, d1
    vadd.u32        q1, q1, q9
    vadd.u32        q1, q1, q2
    vpaddl.u16      d0, d0
    vadd.u32        d2, d2, d3
    vpadd.u32       d0, d0, d2

    vmov            r0, r1, d0
    bx              lr
endfunc

function x265_pixel_var_16x16_neon
    veor.u8         q0, q0
    veor.u8         q1, q1
    veor.u8         q2, q2
    veor.u8         q14, q14
    veor.u8         q15, q15
    mov             ip, #4

.var16_loop:
    subs            ip, ip, #1
    vld1.u8         {q8}, [r0], r1
    VAR_SQR_SUM     q1, q14, q12, d16
    VAR_SQR_SUM     q2, q15, q13, d17

    vld1.u8         {q9}, [r0], r1
    VAR_SQR_SUM     q1, q12, q14, d18
    VAR_SQR_SUM     q2, q13, q15, d19

    vld1.u8         {q8}, [r0], r1
    VAR_SQR_SUM     q1, q14, q12, d16
    VAR_SQR_SUM     q2, q15, q13, d17

    vld1.u8         {q9}, [r0], r1
    VAR_SQR_SUM     q1, q12, q14, d18
    VAR_SQR_SUM     q2, q13, q15, d19
    bgt             .var16_loop

    vpaddl.u16      q8, q14
    vpaddl.u16      q9, q15
    vadd.u32        q1, q1, q8
    vadd.u16        d0, d0, d1
    vadd.u32        q1, q1, q9
    vadd.u32        q1, q1, q2
    vpaddl.u16      d0, d0
    vadd.u32        d2, d2, d3
    vpadd.u32       d0, d0, d2

    vmov            r0, r1, d0
    bx              lr
endfunc

function x265_pixel_var_32x32_neon
    veor.u8         q0, q0
    veor.u8         q1, q1
    veor.u8         q2, q2
    veor.u8         q14, q14
    veor.u8         q15, q15
    mov             ip, #8

.var32_loop:
    subs            ip, ip, #1
    vld1.u8         {q8-q9}, [r0], r1
    VAR_SQR_SUM     q1, q14, q12, d16
    VAR_SQR_SUM     q2, q15, q13, d17
    VAR_SQR_SUM     q1, q12, q14, d18
    VAR_SQR_SUM     q2, q13, q15, d19

    vld1.u8         {q8-q9}, [r0], r1
    VAR_SQR_SUM     q1, q14, q12, d16
    VAR_SQR_SUM     q2, q15, q13, d17
    VAR_SQR_SUM     q1, q12, q14, d18
    VAR_SQR_SUM     q2, q13, q15, d19

    vld1.u8         {q8-q9}, [r0], r1
    VAR_SQR_SUM     q1, q14, q12, d16
    VAR_SQR_SUM     q2, q15, q13, d17
    VAR_SQR_SUM     q1, q12, q14, d18
    VAR_SQR_SUM     q2, q13, q15, d19

    vld1.u8         {q8-q9}, [r0], r1
    VAR_SQR_SUM     q1, q14, q12, d16
    VAR_SQR_SUM     q2, q15, q13, d17
    VAR_SQR_SUM     q1, q12, q14, d18
    VAR_SQR_SUM     q2, q13, q15, d19
    bgt             .var32_loop

    vpaddl.u16      q8, q14
    vpaddl.u16      q9, q15
    vadd.u32        q1, q1, q8
    vadd.u16        d0, d0, d1
    vadd.u32        q1, q1, q9
    vadd.u32        q1, q1, q2
    vpaddl.u16      d0, d0
    vadd.u32        d2, d2, d3
    vpadd.u32       d0, d0, d2

    vmov            r0, r1, d0
    bx              lr
endfunc

function x265_pixel_var_64x64_neon
    sub             r1, #32
    veor.u8         q0, q0
    veor.u8         q1, q1
    veor.u8         q2, q2
    veor.u8         q3, q3
    veor.u8         q14, q14
    veor.u8         q15, q15
    mov             ip, #16

.var64_loop:
    subs            ip, ip, #1
    vld1.u8         {q8-q9}, [r0]!
    VAR_SQR_SUM     q1, q14, q12, d16
    VAR_SQR_SUM     q2, q15, q13, d17
    VAR_SQR_SUM     q1, q12, q14, d18
    VAR_SQR_SUM     q2, q13, q15, d19

    vld1.u8         {q8-q9}, [r0], r1
    VAR_SQR_SUM     q1, q14, q12, d16, 3
    VAR_SQR_SUM     q2, q15, q13, d17, 3
    VAR_SQR_SUM     q1, q12, q14, d18, 3
    VAR_SQR_SUM     q2, q13, q15, d19, 3

    vld1.u8         {q8-q9}, [r0]!
    VAR_SQR_SUM     q1, q14, q12, d16
    VAR_SQR_SUM     q2, q15, q13, d17
    VAR_SQR_SUM     q1, q12, q14, d18
    VAR_SQR_SUM     q2, q13, q15, d19

    vld1.u8         {q8-q9}, [r0], r1
    VAR_SQR_SUM     q1, q14, q12, d16, 3
    VAR_SQR_SUM     q2, q15, q13, d17, 3
    VAR_SQR_SUM     q1, q12, q14, d18, 3
    VAR_SQR_SUM     q2, q13, q15, d19, 3

    vld1.u8         {q8-q9}, [r0]!
    VAR_SQR_SUM     q1, q14, q12, d16
    VAR_SQR_SUM     q2, q15, q13, d17
    VAR_SQR_SUM     q1, q12, q14, d18
    VAR_SQR_SUM     q2, q13, q15, d19

    vld1.u8         {q8-q9}, [r0], r1
    VAR_SQR_SUM     q1, q14, q12, d16, 3
    VAR_SQR_SUM     q2, q15, q13, d17, 3
    VAR_SQR_SUM     q1, q12, q14, d18, 3
    VAR_SQR_SUM     q2, q13, q15, d19, 3

    vld1.u8         {q8-q9}, [r0]!
    VAR_SQR_SUM     q1, q14, q12, d16
    VAR_SQR_SUM     q2, q15, q13, d17
    VAR_SQR_SUM     q1, q12, q14, d18
    VAR_SQR_SUM     q2, q13, q15, d19

    vld1.u8         {q8-q9}, [r0], r1
    VAR_SQR_SUM     q1, q14, q12, d16, 3
    VAR_SQR_SUM     q2, q15, q13, d17, 3
    VAR_SQR_SUM     q1, q12, q14, d18, 3
    VAR_SQR_SUM     q2, q13, q15, d19, 3
    bgt             .var64_loop

    vpaddl.u16      q8, q14
    vpaddl.u16      q9, q15
    vadd.u32        q1, q1, q8
    vadd.u32        q1, q1, q9
    vadd.u32        q1, q1, q2
    vpaddl.u16      d0, d0
    vpaddl.u16      d1, d1
    vpaddl.u16      d6, d6
    vpaddl.u16      d7, d7
    vadd.u32        d0, d1
    vadd.u32        d6, d7
    vadd.u32        d0, d6
    vadd.u32        d2, d2, d3
    vpadd.u32       d0, d0, d2

    vmov            r0, r1, d0
    bx              lr
endfunc

/* void getResidual4_neon(const pixel* fenc, const pixel* pred, int16_t* residual, intptr_t stride);
 * r0   - fenc
 * r1   - pred
 * r2   - residual
 * r3   - Stride */
function x265_getResidual4_neon
    lsl             r12, r3, #1
.rept 2
    vld1.u8         {d0}, [r0], r3
    vld1.u8         {d1}, [r1], r3
    vld1.u8         {d2}, [r0], r3
    vld1.u8         {d3}, [r1], r3
    vsubl.u8        q2, d0, d1
    vsubl.u8        q3, d2, d3
    vst1.s16        {d4}, [r2], r12
    vst1.s16        {d6}, [r2], r12
.endr
    bx              lr
endfunc

function x265_getResidual8_neon
    lsl             r12, r3, #1
.rept 4
    vld1.u8         {d0}, [r0], r3
    vld1.u8         {d1}, [r1], r3
    vld1.u8         {d2}, [r0], r3
    vld1.u8         {d3}, [r1], r3
    vsubl.u8        q2, d0, d1
    vsubl.u8        q3, d2, d3
    vst1.s16        {q2}, [r2], r12
    vst1.s16        {q3}, [r2], r12
.endr
    bx              lr
endfunc

function x265_getResidual16_neon
    lsl             r12, r3, #1
.rept 8
    vld1.u8         {d0, d1}, [r0], r3
    vld1.u8         {d2, d3}, [r1], r3
    vld1.u8         {d4, d5}, [r0], r3
    vld1.u8         {d6, d7}, [r1], r3
    vsubl.u8        q8, d0, d2
    vsubl.u8        q9, d1, d3
    vsubl.u8        q10, d4, d6
    vsubl.u8        q11, d5, d7
    vst1.s16        {q8, q9}, [r2], r12
    vst1.s16        {q10, q11}, [r2], r12
.endr
    bx              lr
endfunc

function x265_getResidual32_neon
    push            {r4}
    lsl             r12, r3, #1
    sub             r12, #32
    mov             r4, #4
loop_res32:
    subs            r4, r4, #1
.rept 8
    vld1.u8         {q0, q1}, [r0], r3
    vld1.u8         {q2, q3}, [r1], r3
    vsubl.u8        q8, d0, d4
    vsubl.u8        q9, d1, d5
    vsubl.u8        q10, d2, d6
    vsubl.u8        q11, d3, d7
    vst1.s16        {q8, q9}, [r2]!
    vst1.s16        {q10, q11}, [r2], r12
.endr
    bne             loop_res32
    pop             {r4}
    bx              lr
endfunc
