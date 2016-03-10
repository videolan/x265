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

// void pixel_sub_ps_neon(int16_t* a, intptr_t dstride, const pixel* b0, const pixel* b1, intptr_t sstride0, intptr_t sstride1)
function x265_pixel_sub_ps_4x4_neon
    push            {r4}
    lsl             r1, r1, #1
    ldr             r4, [sp, #4]
    ldr             r12, [sp, #8]
.rept 2
    vld1.u8         {d0}, [r2], r4
    vld1.u8         {d1}, [r3], r12
    vld1.u8         {d2}, [r2], r4
    vld1.u8         {d3}, [r3], r12
    vsubl.u8        q2, d0, d1
    vsubl.u8        q3, d2, d3
    vst1.s16        {d4}, [r0], r1
    vst1.s16        {d6}, [r0], r1
.endr
    pop             {r4}
    bx              lr
endfunc

function x265_pixel_sub_ps_8x8_neon
    push            {r4}
    lsl             r1, r1, #1
    ldr             r4, [sp, #4]
    ldr             r12, [sp, #8]
.rept 4
    vld1.u8         {d0}, [r2], r4
    vld1.u8         {d1}, [r3], r12
    vld1.u8         {d2}, [r2], r4
    vld1.u8         {d3}, [r3], r12
    vsubl.u8        q2, d0, d1
    vsubl.u8        q3, d2, d3
    vst1.s16        {q2}, [r0], r1
    vst1.s16        {q3}, [r0], r1
.endr
    pop             {r4}
    bx              lr
endfunc

function x265_pixel_sub_ps_16x16_neon
    push            {r4, r5}
    lsl             r1, r1, #1
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    mov             r5, #2
loop_sub16:
    subs            r5, r5, #1
.rept 4
    vld1.u8         {q0}, [r2], r4
    vld1.u8         {q1}, [r3], r12
    vld1.u8         {q2}, [r2], r4
    vld1.u8         {q3}, [r3], r12
    vsubl.u8        q8, d0, d2
    vsubl.u8        q9, d1, d3
    vsubl.u8        q10, d4, d6
    vsubl.u8        q11, d5, d7
    vst1.s16        {q8, q9}, [r0], r1
    vst1.s16        {q10, q11}, [r0], r1
.endr
    bne             loop_sub16
    pop             {r4, r5}
    bx              lr
endfunc

function x265_pixel_sub_ps_32x32_neon
    push            {r4, r5}
    lsl             r1, r1, #1
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    sub             r1, #32
    mov             r5, #8
loop_sub32:
    subs            r5, r5, #1
.rept 4
    vld1.u8         {q0, q1}, [r2], r4
    vld1.u8         {q2, q3}, [r3], r12
    vsubl.u8        q8, d0, d4
    vsubl.u8        q9, d1, d5
    vsubl.u8        q10, d2, d6
    vsubl.u8        q11, d3, d7
    vst1.s16        {q8, q9}, [r0]!
    vst1.s16        {q10, q11}, [r0], r1
.endr
    bne             loop_sub32
    pop             {r4, r5}
    bx              lr
endfunc

function x265_pixel_sub_ps_64x64_neon
    push            {r4, r5}
    lsl             r1, r1, #1
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    sub             r1, #96
    sub             r4, #32
    sub             r12, #32
    mov             r5, #32
loop_sub64:
    subs            r5, r5, #1
.rept 2
    vld1.u8         {q0, q1}, [r2]!
    vld1.u8         {q2, q3}, [r2], r4
    vld1.u8         {q8, q9}, [r3]!
    vld1.u8         {q10, q11}, [r3], r12
    vsubl.u8        q12, d0, d16
    vsubl.u8        q13, d1, d17
    vsubl.u8        q14, d2, d18
    vsubl.u8        q15, d3, d19
    vsubl.u8        q0, d4, d20
    vsubl.u8        q1, d5, d21
    vsubl.u8        q2, d6, d22
    vsubl.u8        q3, d7, d23
    vst1.s16        {q12, q13}, [r0]!
    vst1.s16        {q14, q15}, [r0]!
    vst1.s16        {q0, q1}, [r0]!
    vst1.s16        {q2, q3}, [r0], r1
.endr
    bne             loop_sub64
    pop             {r4, r5}
    bx              lr
endfunc

// void x265_pixel_add_ps_neon(pixel* a, intptr_t dstride, const pixel* b0, const int16_t* b1, intptr_t sstride0, intptr_t sstride1);
function x265_pixel_add_ps_4x4_neon
    push            {r4}
    ldr             r4, [sp, #4]
    ldr             r12, [sp, #8]
    lsl             r12, #1
    vmov.u16        q10, #255
    veor.u16        q11, q11
    veor.u16        d3, d3
    veor.u16        d5, d5
.rept 2
    vld1.u8         {d0}, [r2], r4
    vld1.u8         {d1}, [r2], r4
    vld1.s16        {d2}, [r3], r12
    vld1.s16        {d4}, [r3], r12
    vmovl.u8        q8, d0
    vmovl.u8        q9, d1
    vadd.s16        q1, q1, q8
    vadd.s16        q2, q2, q9
    vqmovun.s16     d0, q1
    vqmovun.s16     d1, q2
    vst1.32         {d0[0]}, [r0], r1
    vst1.32         {d1[0]}, [r0], r1
.endr
    pop             {r4}
    bx              lr
endfunc

function x265_pixel_add_ps_8x8_neon
    push            {r4}
    ldr             r4, [sp, #4]
    ldr             r12, [sp, #8]
    lsl             r12, #1
    vmov.u16        q10, #255
    veor.u16        q11, q11
.rept 4
    vld1.u8         {d0}, [r2], r4
    vld1.u8         {d1}, [r2], r4
    vld1.s16        {q8}, [r3], r12
    vld1.s16        {q9}, [r3], r12
    vmovl.u8        q1, d0
    vmovl.u8        q2, d1
    vadd.s16        q1, q1, q8
    vadd.s16        q2, q2, q9
    vqmovun.s16     d0, q1
    vqmovun.s16     d1, q2
    vst1.8          {d0}, [r0], r1
    vst1.8          {d1}, [r0], r1
.endr
    pop             {r4}
    bx              lr
endfunc

function x265_pixel_add_ps_16x16_neon
    push            {r4, r5}
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    lsl             r12, #1
    vmov.u16        q10, #255
    veor.u16        q11, q11
    mov             r5, #2
loop_addps16:
    subs            r5, #1
.rept 4
    vld1.u8         {q0}, [r2], r4
    vld1.u8         {q1}, [r2], r4
    vld1.s16        {q8, q9}, [r3], r12
    vld1.s16        {q12, q13}, [r3], r12

    vmovl.u8        q2, d0
    vmovl.u8        q3, d1
    vmovl.u8        q0, d2
    vmovl.u8        q1, d3

    vadd.s16        q2, q2, q8
    vadd.s16        q3, q3, q9
    vadd.s16        q0, q0, q12
    vadd.s16        q1, q1, q13

    vqmovun.s16     d4, q2
    vqmovun.s16     d5, q3
    vqmovun.s16     d0, q0
    vqmovun.s16     d1, q1
    vst1.8          {d4, d5}, [r0], r1
    vst1.8          {d0, d1}, [r0], r1
.endr
    bne             loop_addps16
    pop             {r4, r5}
    bx              lr
endfunc

 function x265_pixel_add_ps_32x32_neon
    push            {r4, r5}
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    lsl             r12, #1
    vmov.u16        q10, #255
    veor.u16        q11, q11
    mov             r5, #4
    sub             r12, #32
loop_addps32:
    subs            r5, #1
.rept 8
    vld1.u8         {q0, q1}, [r2], r4
    vld1.s16        {q8, q9}, [r3]!
    vld1.s16        {q12, q13}, [r3], r12

    vmovl.u8        q2, d0
    vmovl.u8        q3, d1
    vmovl.u8        q14, d2
    vmovl.u8        q15, d3

    vadd.s16        q2, q2, q8
    vadd.s16        q3, q3, q9
    vadd.s16        q14, q14, q12
    vadd.s16        q15, q15, q13

    vqmovun.s16     d0, q2
    vqmovun.s16     d1, q3
    vqmovun.s16     d2, q14
    vqmovun.s16     d3, q15
    vst1.8          {q0, q1}, [r0], r1
.endr
    bne             loop_addps32
    pop             {r4, r5}
    bx              lr
endfunc

function x265_pixel_add_ps_64x64_neon
    push            {r4, r5}
    vpush           {q4, q5, q6, q7}
    ldr             r4, [sp, #72]
    ldr             r12, [sp, #76]
    lsl             r12, #1
    vmov.u16        q2, #255
    veor.u16        q3, q3
    mov             r5, #32
    sub             r1, #32
    sub             r4, #32
    sub             r12, #96
loop_addps64:
    subs            r5, #1
.rept 2
    vld1.u8         {q0, q1}, [r2]!
    vld1.s16        {q8, q9}, [r3]!
    vld1.s16        {q10, q11}, [r3]!
    vld1.s16        {q12, q13}, [r3]!
    vld1.s16        {q14, q15}, [r3], r12

    vmovl.u8        q4, d0
    vmovl.u8        q5, d1
    vmovl.u8        q6, d2
    vmovl.u8        q7, d3

    vadd.s16        q4, q4, q8
    vadd.s16        q5, q5, q9
    vadd.s16        q6, q6, q10
    vadd.s16        q7, q7, q11

    vqmovun.s16     d0, q4
    vqmovun.s16     d1, q5
    vqmovun.s16     d2, q6
    vqmovun.s16     d3, q7

    vst1.u8         {q0, q1}, [r0]!
    vld1.u8         {q0, q1}, [r2], r4
    vmovl.u8        q4, d0
    vmovl.u8        q5, d1
    vmovl.u8        q6, d2
    vmovl.u8        q7, d3

    vadd.s16        q4, q4, q12
    vadd.s16        q5, q5, q13
    vadd.s16        q6, q6, q14
    vadd.s16        q7, q7, q15

    vqmovun.s16     d0, q4
    vqmovun.s16     d1, q5
    vqmovun.s16     d2, q6
    vqmovun.s16     d3, q7
    vst1.u8         {q0, q1}, [r0], r1
.endr
    bne             loop_addps64
    vpop            {q4, q5, q6, q7}
    pop             {r4, r5}
    bx              lr
endfunc

function x265_pixel_planecopy_cp_neon
    push            {r4, r5, r6, r7}
    ldr             r4, [sp, #4 * 4]
    ldr             r5, [sp, #4 * 4 + 4]
    ldr             r12, [sp, #4 * 4 + 8]
    vdup.8          q2, r12
    sub             r5, #1

.loop_h:
    mov             r6, r0
    mov             r12, r2
    eor             r7, r7
.loop_w:
    vld1.u8         {q0}, [r6]!
    vshl.u8         q0, q0, q2
    vst1.u8         {q0}, [r12]!

    add             r7, #16
    cmp             r7, r4
    blt             .loop_w

    add             r0, r1
    add             r2, r3

    subs             r5, #1
    bgt             .loop_h

// handle last row
    mov             r5, r4
    lsr             r5, #3

.loopW8:
    vld1.u8         d0, [r0]!
    vshl.u8         d0, d0, d4
    vst1.u8         d0, [r2]!
    subs            r4, r4, #8
    subs            r5, #1
    bgt             .loopW8

    mov             r5,#8
    sub             r5, r4
    sub             r0, r5
    sub             r2, r5
    vld1.u8         d0, [r0]
    vshl.u8         d0, d0, d4
    vst1.u8         d0, [r2]

    pop             {r4, r5, r6, r7}
    bx              lr
endfunc

