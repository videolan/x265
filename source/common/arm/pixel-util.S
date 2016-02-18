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
