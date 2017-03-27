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

// chroma sub_ps
function x265_pixel_sub_ps_4x8_neon
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
    vst1.s16        {d4}, [r0], r1
    vst1.s16        {d6}, [r0], r1
.endr
    pop             {r4}
    bx              lr
endfunc

function x265_pixel_sub_ps_8x16_neon
    push            {r4}
    lsl             r1, r1, #1
    ldr             r4, [sp, #4]
    ldr             r12, [sp, #8]
.rept 8
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

function x265_pixel_sub_ps_16x32_neon
    push            {r4, r5}
    lsl             r1, r1, #1
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    mov             r5, #4
loop_sub_16x32:
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
    bne             loop_sub_16x32
    pop             {r4, r5}
    bx              lr
endfunc

function x265_pixel_sub_ps_32x64_neon
    push            {r4, r5}
    lsl             r1, r1, #1
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    sub             r1, #32
    mov             r5, #16
loop_sub_32x64:
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
    bne             loop_sub_32x64
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

.macro pixel_add_ps_16xN_neon h i
function x265_pixel_add_ps_16x\h\()_neon
    push            {r4, r5}
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    lsl             r12, #1
    vmov.u16        q10, #255
    veor.u16        q11, q11
    mov             r5, #\i
loop_addps_16x\h\():
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
    bne             loop_addps_16x\h
    pop             {r4, r5}
    bx              lr
endfunc
.endm

pixel_add_ps_16xN_neon 16 2
pixel_add_ps_16xN_neon 32 4

.macro pixel_add_ps_32xN_neon h i
 function x265_pixel_add_ps_32x\h\()_neon
    push            {r4, r5}
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    lsl             r12, #1
    vmov.u16        q10, #255
    veor.u16        q11, q11
    mov             r5, #\i
    sub             r12, #32
loop_addps_32x\h\():
    subs            r5, #1
.rept 4
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
    bne             loop_addps_32x\h
    pop             {r4, r5}
    bx              lr
endfunc
.endm

pixel_add_ps_32xN_neon 32 8
pixel_add_ps_32xN_neon 64 16

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

// Chroma add_ps
function x265_pixel_add_ps_4x8_neon
    push            {r4}
    ldr             r4, [sp, #4]
    ldr             r12, [sp, #8]
    lsl             r12, #1
    vmov.u16        q10, #255
    veor.u16        q11, q11
    veor.u16        d3, d3
    veor.u16        d5, d5
.rept 4
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

function x265_pixel_add_ps_8x16_neon
    push            {r4, r5}
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    lsl             r12, #1
    vmov.u16        q10, #255
    veor.u16        q11, q11
    mov             r5, #2
loop_add_8x16:
    subs            r5, #1
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
    bne             loop_add_8x16
    pop             {r4, r5}
    bx              lr
endfunc

// void scale1D_128to64(pixel *dst, const pixel *src)
function x265_scale1D_128to64_neon 
    mov             r12, #32
.rept 2
    vld2.u8         {q8, q9}, [r1]!
    vld2.u8         {q10, q11}, [r1]!
    vld2.u8         {q12, q13}, [r1]!
    vld2.u8         {q14, q15}, [r1], r12

    vrhadd.u8       q0, q8, q9
    vrhadd.u8       q1, q10, q11
    vrhadd.u8       q2, q12, q13
    vrhadd.u8       q3, q14, q15

    vst1.u8         {q0, q1}, [r0]!
    vst1.u8         {q2, q3}, [r0], r12
.endr
    bx              lr
endfunc

// void scale2D_64to32(pixel* dst, const pixel* src, intptr_t stride)
function x265_scale2D_64to32_neon
    sub             r2, #32
    mov             r3, #16
loop_scale2D:
    subs            r3, #1
.rept 2
    vld2.8          {q8, q9}, [r1]!
    vld2.8          {q10, q11}, [r1], r2
    vld2.8          {q12, q13}, [r1]!
    vld2.8          {q14, q15}, [r1], r2

    vaddl.u8        q0, d16, d18
    vaddl.u8        q1, d17, d19
    vaddl.u8        q2, d20, d22
    vaddl.u8        q3, d21, d23

    vaddl.u8        q8, d24, d26
    vaddl.u8        q9, d25, d27
    vaddl.u8        q10, d28, d30
    vaddl.u8        q11, d29, d31

    vadd.u16        q0, q8
    vadd.u16        q1, q9
    vadd.u16        q2, q10
    vadd.u16        q3, q11

    vrshrn.u16      d16, q0, #2
    vrshrn.u16      d17, q1, #2
    vrshrn.u16      d18, q2, #2
    vrshrn.u16      d19, q3, #2
    vst1.8          {q8, q9}, [r0]!
.endr
    bne             loop_scale2D
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

//******* satd *******
.macro satd_4x4_neon
    vld1.32         {d1[]}, [r2], r3
    vld1.32         {d0[]}, [r0,:32], r1
    vld1.32         {d3[]}, [r2], r3
    vld1.32         {d2[]}, [r0,:32], r1
    vld1.32         {d1[1]}, [r2], r3
    vld1.32         {d0[1]}, [r0,:32], r1
    vld1.32         {d3[1]}, [r2], r3
    vld1.32         {d2[1]}, [r0,:32], r1
    vsubl.u8        q0, d0, d1
    vsubl.u8        q1, d2, d3
    SUMSUB_AB       q2, q3, q0, q1
    SUMSUB_ABCD     d0, d2, d1, d3, d4, d5, d6, d7
    HADAMARD        1, sumsub, q2, q3, q0, q1
    HADAMARD        2, amax, q0,, q2, q3
    HORIZ_ADD       d0, d0, d1
.endm

function x265_pixel_satd_4x4_neon
    satd_4x4_neon
    vmov.32         r0, d0[0]
    bx              lr
endfunc

.macro LOAD_DIFF_8x4_1 q0 q1 q2 q3
    vld1.32         {d1}, [r2], r3
    vld1.32         {d0}, [r0,:64], r1
    vsubl.u8        \q0, d0, d1
    vld1.32         {d3}, [r2], r3
    vld1.32         {d2}, [r0,:64], r1
    vsubl.u8        \q1, d2, d3
    vld1.32         {d5}, [r2], r3
    vld1.32         {d4}, [r0,:64], r1
    vsubl.u8        \q2, d4, d5
    vld1.32         {d7}, [r2], r3
    vld1.32         {d6}, [r0,:64], r1
    vsubl.u8        \q3, d6, d7
.endm

.macro x265_satd_4x8_8x4_end_neon
    vadd.s16        q0, q8, q10
    vadd.s16        q1, q9, q11
    vsub.s16        q2, q8, q10
    vsub.s16        q3, q9, q11

    vtrn.16         q0, q1
    vadd.s16        q8, q0, q1
    vtrn.16         q2, q3
    vsub.s16        q9, q0, q1
    vadd.s16        q10, q2, q3
    vsub.s16        q11, q2, q3
    vtrn.32         q8, q10
    vabs.s16        q8, q8
    vtrn.32         q9, q11
    vabs.s16        q10, q10
    vabs.s16        q9, q9
    vabs.s16        q11, q11
    vmax.u16        q0, q8, q10
    vmax.u16        q1, q9, q11
    vadd.u16        q0, q0, q1
    HORIZ_ADD       d0, d0, d1
.endm

.macro pixel_satd_4x8_neon
    vld1.32         {d1[]}, [r2], r3
    vld1.32         {d0[]}, [r0,:32], r1
    vld1.32         {d3[]}, [r2], r3
    vld1.32         {d2[]}, [r0,:32], r1
    vld1.32         {d5[]}, [r2], r3
    vld1.32         {d4[]}, [r0,:32], r1
    vld1.32         {d7[]}, [r2], r3
    vld1.32         {d6[]}, [r0,:32], r1

    vld1.32         {d1[1]}, [r2], r3
    vld1.32         {d0[1]}, [r0,:32], r1
    vsubl.u8        q0, d0, d1
    vld1.32         {d3[1]}, [r2], r3
    vld1.32         {d2[1]}, [r0,:32], r1
    vsubl.u8        q1, d2, d3
    vld1.32         {d5[1]}, [r2], r3
    vld1.32         {d4[1]}, [r0,:32], r1
    vsubl.u8        q2, d4, d5
    vld1.32         {d7[1]}, [r2], r3
    SUMSUB_AB       q8, q9, q0, q1
    vld1.32         {d6[1]}, [r0,:32], r1
    vsubl.u8        q3, d6, d7
    SUMSUB_AB       q10, q11, q2, q3
    x265_satd_4x8_8x4_end_neon
.endm

function x265_pixel_satd_4x8_neon
    pixel_satd_4x8_neon
    vmov.32         r0, d0[0]
    bx              lr
endfunc

function x265_pixel_satd_4x16_neon
    push            {r4, r5}
    eor             r4, r4
    pixel_satd_4x8_neon
    vmov.32         r5, d0[0]
    add             r4, r5
    pixel_satd_4x8_neon
    vmov.32         r5, d0[0]
    add             r0, r5, r4
    pop             {r4, r5}
    bx              lr
endfunc

function x265_pixel_satd_4x32_neon
    push            {r4, r5}
    eor             r4, r4
.rept 4
    pixel_satd_4x8_neon
    vmov.32         r5, d0[0]
    add             r4, r5
.endr
    mov             r0, r4
    pop             {r4, r5}
    bx              lr
endfunc

function x265_pixel_satd_12x16_neon
    push            {r4, r5, r6, r7}
    vpush           {d8-d11}
    mov             ip, lr
    mov             r4, r0
    mov             r5, r2
    eor             r7, r7
    pixel_satd_4x8_neon
    vmov.32         r6, d0[0]
    add             r7, r6
    pixel_satd_4x8_neon
    vmov.32         r6, d0[0]
    add             r7, r6

    add             r0, r4, #4
    add             r2, r5, #4
    pixel_satd_4x8_neon
    vmov.32         r6, d0[0]
    add             r7, r6
    pixel_satd_4x8_neon
    vmov.32         r6, d0[0]
    add             r7, r6

    add             r0, r4, #8
    add             r2, r5, #8
    pixel_satd_4x8_neon
    vmov.32         r6, d0[0]
    add             r7, r6
    pixel_satd_4x8_neon
    vmov.32         r6, d0[0]
    add             r0, r7, r6
    vpop            {d8-d11}
    pop             {r4, r5, r6, r7}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_12x32_neon
    push            {r4, r5, r6, r7}
    vpush           {d8-d11}
    mov             ip, lr
    mov             r4, r0
    mov             r5, r2
    eor             r7, r7
.rept 4
    pixel_satd_4x8_neon
    vmov.32         r6, d0[0]
    add             r7, r6
.endr

    add             r0, r4, #4
    add             r2, r5, #4
.rept 4
    pixel_satd_4x8_neon
    vmov.32         r6, d0[0]
    add             r7, r6
.endr

    add             r0, r4, #8
    add             r2, r5, #8
.rept 4
    pixel_satd_4x8_neon
    vmov.32         r6, d0[0]
    add             r7, r6
.endr

    mov             r0, r7
    vpop            {d8-d11}
    pop             {r4, r5, r6, r7}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_8x4_neon
    push            {r4, r5, r6}
    mov             r4, r0
    mov             r5, r2
    satd_4x4_neon
    add             r0, r4, #4
    add             r2, r5, #4
    vmov.32         r6, d0[0]
    satd_4x4_neon
    vmov.32         r0, d0[0]
    add             r0, r0, r6
    pop             {r4, r5, r6}
    bx              lr
endfunc

function x265_pixel_satd_8x8_neon
    mov             ip, lr
    push            {r4, r5, r6, r7}
    eor             r4, r4
    mov             r6, r0
    mov             r7, r2
    pixel_satd_4x8_neon
    vmov.32         r5, d0[0]
    add             r4, r5
    add             r0, r6, #4
    add             r2, r7, #4
    pixel_satd_4x8_neon
    vmov.32         r5, d0[0]
    add             r0, r4, r5
    pop             {r4, r5, r6, r7}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_8x12_neon
    push            {r4, r5, r6, r7}
    mov             r4, r0
    mov             r5, r2
    eor             r7, r7
    satd_4x4_neon
    vmov.32         r6, d0[0]
    add             r7, r6
    add             r0, r4, #4
    add             r2, r5, #4
    satd_4x4_neon
    vmov.32         r6, d0[0]
    add             r7, r6
.rept 2
    sub             r0, #4
    sub             r2, #4
    mov             r4, r0
    mov             r5, r2
    satd_4x4_neon
    vmov.32         r6, d0[0]
    add             r7, r6
    add             r0, r4, #4
    add             r2, r5, #4
    satd_4x4_neon
    vmov.32         r6, d0[0]
    add             r7, r6
.endr
    mov             r0, r7
    pop             {r4, r5, r6, r7}
    bx              lr
endfunc

function x265_pixel_satd_8x16_neon
    vpush           {d8-d11}
    mov             ip, lr
    bl              x265_satd_8x8_neon
    vadd.u16        q4, q12, q13
    vadd.u16        q5, q14, q15

    bl              x265_satd_8x8_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15

    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vpop            {d8-d11}
    mov             lr, ip
    vmov.32         r0, d0[0]
    bx              lr
endfunc

function x265_pixel_satd_8x32_neon
    vpush           {d8-d11}
    mov             ip, lr
    bl              x265_satd_8x8_neon
    vadd.u16        q4, q12, q13
    vadd.u16        q5, q14, q15
.rept 3
    bl              x265_satd_8x8_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15
.endr
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vpop            {d8-d11}
    mov             lr, ip
    vmov.32         r0, d0[0]
    bx              lr
endfunc

function x265_pixel_satd_8x64_neon
    vpush           {d8-d11}
    mov             ip, lr
    bl              x265_satd_8x8_neon
    vadd.u16        q4, q12, q13
    vadd.u16        q5, q14, q15
.rept 7
    bl              x265_satd_8x8_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15
.endr
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vpop            {d8-d11}
    mov             lr, ip
    vmov.32         r0, d0[0]
    bx              lr
endfunc

function x265_satd_8x8_neon
    LOAD_DIFF_8x4_1   q8, q9, q10, q11
    vld1.64         {d7}, [r2], r3
    vld1.64         {d6}, [r0,:64], r1
    vsubl.u8        q12, d6, d7
    SUMSUB_AB       q0, q1, q8, q9

    vld1.64         {d17}, [r2], r3
    vld1.64         {d16}, [r0,:64], r1
    vsubl.u8        q13, d16, d17
    SUMSUB_AB       q2, q3, q10, q11

    vld1.64         {d19}, [r2], r3
    vld1.64         {d18}, [r0,:64], r1
    vsubl.u8        q14, d18, d19
    SUMSUB_AB       q8, q10, q0, q2

    vld1.64         {d1}, [r2], r3
    vld1.64         {d0}, [r0,:64], r1
    vsubl.u8        q15, d0, d1
    SUMSUB_AB       q9, q11, q1, q3
endfunc

// one vertical hadamard pass and two horizontal
function x265_satd_8x4v_8x8h_neon, export=0
    SUMSUB_ABCD     q0, q1, q2, q3, q12, q13, q14, q15
    SUMSUB_AB       q12, q14, q0, q2
    SUMSUB_AB       q13, q15, q1, q3
    vtrn.16         q8, q9
    vtrn.16         q10, q11

    SUMSUB_AB       q0, q1, q8, q9
    SUMSUB_AB       q2, q3, q10, q11
    vtrn.16         q12, q13
    vtrn.16         q14, q15

    SUMSUB_AB       q8, q9, q12, q13
    SUMSUB_AB       q10, q11, q14, q15
    vtrn.32         q0, q2
    vtrn.32         q1, q3
    ABS2            q0, q2
    ABS2            q1, q3

    vtrn.32         q8, q10
    vtrn.32         q9, q11
    ABS2            q8, q10
    ABS2            q9, q11

    vmax.s16        q12, q0, q2
    vmax.s16        q13, q1, q3
    vmax.s16        q14, q8, q10
    vmax.s16        q15, q9, q11
    bx              lr
endfunc

function x265_satd_16x4_neon, export=0
    vld1.64         {d2-d3}, [r2], r3
    vld1.64         {d0-d1}, [r0,:128], r1
    vsubl.u8        q8, d0, d2
    vsubl.u8        q12, d1, d3

    vld1.64         {d6-d7}, [r2], r3
    vld1.64         {d4-d5}, [r0,:128], r1
    vsubl.u8        q9, d4, d6
    vsubl.u8        q13, d5, d7

    vld1.64         {d2-d3}, [r2], r3
    vld1.64         {d0-d1}, [r0,:128], r1
    vsubl.u8        q10, d0, d2
    vsubl.u8        q14, d1, d3

    vld1.64         {d6-d7}, [r2], r3
    vld1.64         {d4-d5}, [r0,:128], r1
    vsubl.u8        q11, d4, d6
    vsubl.u8        q15, d5, d7

    vadd.s16        q0, q8, q9
    vsub.s16        q1, q8, q9
    SUMSUB_AB       q2, q3, q10, q11
    SUMSUB_ABCD     q8, q10, q9, q11, q0, q2, q1, q3
    b               x265_satd_8x4v_8x8h_neon
endfunc

function x265_pixel_satd_16x4_neon
    vpush           {d8-d11}
    mov             ip, lr
    bl              x265_satd_16x4_neon
    vadd.u16        q4, q12, q13
    vadd.u16        q5, q14, q15
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vpop            {d8-d11}
    mov             lr, ip
    vmov.32         r0, d0[0]
    bx              lr
endfunc

function x265_pixel_satd_16x8_neon
    vpush           {d8-d11}
    mov             ip, lr
    bl              x265_satd_16x4_neon
    vadd.u16        q4, q12, q13
    vadd.u16        q5, q14, q15

    bl              x265_satd_16x4_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15

    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vpop            {d8-d11}
    mov             lr, ip
    vmov.32         r0, d0[0]
    bx              lr
endfunc

function x265_pixel_satd_16x12_neon
    vpush           {d8-d11}
    mov             ip, lr
    bl              x265_satd_16x4_neon
    vadd.u16        q4, q12, q13
    vadd.u16        q5, q14, q15
.rept 2
    bl              x265_satd_16x4_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15
.endr
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vpop            {d8-d11}
    mov             lr, ip
    vmov.32         r0, d0[0]
    bx              lr
endfunc

function x265_pixel_satd_16x16_neon
    vpush           {d8-d11}
    mov             ip, lr
    bl              x265_satd_16x4_neon
    vadd.u16        q4, q12, q13
    vadd.u16        q5, q14, q15
.rept 3
    bl              x265_satd_16x4_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15
.endr
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vpop            {d8-d11}
    mov             lr, ip
    vmov.32         r0, d0[0]
    bx              lr
endfunc

function x265_pixel_satd_16x24_neon
    vpush           {d8-d11}
    mov             ip, lr
    bl              x265_satd_16x4_neon
    vadd.u16        q4, q12, q13
    vadd.u16        q5, q14, q15
.rept 5
    bl              x265_satd_16x4_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15
.endr
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vpop            {d8-d11}
    mov             lr, ip
    vmov.32         r0, d0[0]
    bx              lr
endfunc

.macro pixel_satd_16x32_neon
    bl              x265_satd_16x4_neon
    vadd.u16        q4, q12, q13
    vadd.u16        q5, q14, q15
.rept 7
    bl              x265_satd_16x4_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15
.endr
.endm

function x265_pixel_satd_16x32_neon
    vpush           {d8-d11}
    mov             ip, lr
    pixel_satd_16x32_neon
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vpop            {d8-d11}
    mov             lr, ip
    vmov.32         r0, d0[0]
    bx              lr
endfunc

function x265_pixel_satd_16x64_neon
    push            {r6, r7}
    vpush           {d8-d11}
    mov             ip, lr
    eor             r7, r7
    pixel_satd_16x32_neon
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vmov.32         r6, d0[0]
    add             r7, r6

    veor            q4, q5
    veor            q5, q5
    pixel_satd_16x32_neon
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vmov.32         r6, d0[0]
    add             r0, r7, r6
    vpop            {d8-d11}
    pop             {r6, r7}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_24x32_neon
    push            {r4, r5, r6, r7}
    vpush           {d8-d11}
    mov             ip, lr
    eor             r7, r7
    mov             r4, r0
    mov             r5, r2
.rept 3
    veor            q4, q4
    veor            q5, q5
.rept 4
    bl              x265_satd_8x8_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15
.endr
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vmov.32         r6, d0[0]
    add             r7, r6
    add             r4, #8
    add             r5, #8
    mov             r0, r4
    mov             r2, r5
.endr
    mov             r0, r7
    vpop            {d8-d11}
    pop             {r4, r5, r6, r7}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_24x64_neon
    push            {r4, r5, r6, r7}
    vpush           {d8-d11}
    mov             ip, lr
    eor             r7, r7
    mov             r4, r0
    mov             r5, r2
.rept 3
    veor            q4, q4
    veor            q5, q5
.rept 4
    bl              x265_satd_8x8_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15
.endr
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vmov.32         r6, d0[0]
    add             r7, r6
    add             r4, #8
    add             r5, #8
    mov             r0, r4
    mov             r2, r5
.endr

    sub             r4, #24
    sub             r5, #24
    add             r0, r4, r1, lsl #5
    add             r2, r5, r3, lsl #5
    mov             r4, r0
    mov             r5, r2
.rept 3
    veor            q4, q4
    veor            q5, q5
.rept 4
    bl              x265_satd_8x8_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15
.endr
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vmov.32         r6, d0[0]
    add             r7, r6
    add             r4, #8
    add             r5, #8
    mov             r0, r4
    mov             r2, r5
.endr
    mov             r0, r7
    vpop            {d8-d11}
    pop             {r4, r5, r6, r7}
    mov             lr, ip
    bx              lr
endfunc

.macro pixel_satd_32x8
    mov             r4, r0
    mov             r5, r2
    bl              x265_satd_16x4_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15

    bl              x265_satd_16x4_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15

    add             r0, r4, #16
    add             r2, r5, #16
    bl              x265_satd_16x4_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15

    bl              x265_satd_16x4_neon
    vadd.u16        q4, q4, q12
    vadd.u16        q5, q5, q13
    vadd.u16        q4, q4, q14
    vadd.u16        q5, q5, q15
.endm

function x265_pixel_satd_32x8_neon
    push            {r4, r5}
    vpush           {d8-d11}
    mov             ip, lr
    veor            q4, q4
    veor            q5, q5
    pixel_satd_32x8
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vmov.32         r0, d0[0]
    vpop            {d8-d11}
    pop             {r4, r5}
    mov             lr, ip
    bx              lr
endfunc

.macro satd_32x16_neon
    veor            q4, q4
    veor            q5, q5
    pixel_satd_32x8
    sub             r0, #16
    sub             r2, #16
    pixel_satd_32x8
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vmov.32         r6, d0[0]
.endm

function x265_pixel_satd_32x16_neon
    push            {r4, r5, r6}
    vpush           {d8-d11}
    mov             ip, lr
    satd_32x16_neon
    mov             r0, r6
    vpop            {d8-d11}
    pop             {r4, r5, r6}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_32x24_neon
    push            {r4, r5, r6}
    vpush           {d8-d11}
    mov             ip, lr
    satd_32x16_neon
    veor            q4, q4
    veor            q5, q5
    sub             r0, #16
    sub             r2, #16
    pixel_satd_32x8
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vmov.32         r0, d0[0]
    add             r0, r6
    vpop            {d8-d11}
    pop             {r4, r5, r6}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_32x32_neon
    push            {r4, r5, r6, r7}
    vpush           {d8-d11}
    mov             ip, lr
    eor             r7, r7
    satd_32x16_neon
    sub             r0, #16
    sub             r2, #16
    add             r7, r6
    satd_32x16_neon
    add             r0, r7, r6
    vpop            {d8-d11}
    pop             {r4, r5, r6, r7}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_32x48_neon
    push            {r4, r5, r6, r7}
    vpush           {d8-d11}
    mov             ip, lr
    eor             r7, r7
.rept 2
    satd_32x16_neon
    sub             r0, #16
    sub             r2, #16
    add             r7, r6
.endr
    satd_32x16_neon
    add             r0, r7, r6
    vpop            {d8-d11}
    pop             {r4, r5, r6, r7}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_32x64_neon
    push            {r4, r5, r6, r7}
    vpush           {d8-d11}
    mov             ip, lr
    eor             r7, r7
.rept 3
    satd_32x16_neon
    sub             r0, #16
    sub             r2, #16
    add             r7, r6
.endr
    satd_32x16_neon
    add             r0, r7, r6
    vpop            {d8-d11}
    pop             {r4, r5, r6, r7}
    mov             lr, ip
    bx              lr
endfunc

.macro satd_64x16_neon
    mov             r8, r0
    mov             r9, r2
    satd_32x16_neon
    add             r7, r6
    add             r0, r8, #32
    add             r2, r9, #32
    satd_32x16_neon
    add             r7, r6
.endm

function x265_pixel_satd_64x16_neon
    push            {r4, r5, r6, r7, r8, r9}
    vpush           {d8-d11}
    mov             ip, lr
    eor             r7, r7
    satd_64x16_neon
    mov             r0, r7
    vpop            {d8-d11}
    pop             {r4, r5, r6, r7, r8, r9}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_64x32_neon
    push            {r4, r5, r6, r7, r8, r9}
    vpush           {d8-d11}
    mov             ip, lr
    eor             r7, r7
    satd_64x16_neon
    sub             r0, #48
    sub             r2, #48
    satd_64x16_neon
    mov             r0, r7
    vpop            {d8-d11}
    pop             {r4, r5, r6, r7, r8, r9}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_64x48_neon
    push            {r4, r5, r6, r7, r8, r9}
    vpush           {d8-d11}
    mov             ip, lr
    eor             r7, r7
    satd_64x16_neon
    sub             r0, #48
    sub             r2, #48
    satd_64x16_neon
    sub             r0, #48
    sub             r2, #48
    satd_64x16_neon
    mov             r0, r7
    vpop            {d8-d11}
    pop             {r4, r5, r6, r7, r8, r9}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_64x64_neon
    push            {r4, r5, r6, r7, r8, r9}
    vpush           {d8-d11}
    mov             ip, lr
    eor             r7, r7
    satd_64x16_neon
    sub             r0, #48
    sub             r2, #48
    satd_64x16_neon
    sub             r0, #48
    sub             r2, #48
    satd_64x16_neon
    sub             r0, #48
    sub             r2, #48
    satd_64x16_neon
    mov             r0, r7
    vpop            {d8-d11}
    pop             {r4, r5, r6, r7, r8, r9}
    mov             lr, ip
    bx              lr
endfunc

function x265_pixel_satd_48x64_neon
    push            {r4, r5, r6, r7, r8, r9}
    vpush           {d8-d11}
    mov             ip, lr
    eor             r7, r7
    mov             r8, r0
    mov             r9, r2
.rept 3
    satd_32x16_neon
    sub             r0, #16
    sub             r2, #16
    add             r7, r6
.endr
    satd_32x16_neon
    add             r7, r6

    add             r0, r8, #32
    add             r2, r9, #32
    pixel_satd_16x32_neon
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vmov.32         r6, d0[0]
    add             r7, r6

    veor            q4, q5
    veor            q5, q5
    pixel_satd_16x32_neon
    vadd.u16        q0, q4, q5
    HORIZ_ADD       d0, d0, d1
    vmov.32         r6, d0[0]
    add             r0, r7, r6

    vpop            {d8-d11}
    pop             {r4, r5, r6, r7, r8, r9}
    mov             lr, ip
    bx              lr
endfunc

.macro LOAD_DIFF_8x4 q0 q1 q2 q3
    vld1.32         {d1}, [r2], r3
    vld1.32         {d0}, [r0,:64], r1
    vsubl.u8        \q0, d0, d1
    vld1.32         {d3}, [r2], r3
    vld1.32         {d2}, [r0,:64], r1
    vsubl.u8        \q1, d2, d3
    vld1.32         {d5}, [r2], r3
    vld1.32         {d4}, [r0,:64], r1
    vsubl.u8        \q2, d4, d5
    vld1.32         {d7}, [r2], r3
    vld1.32         {d6}, [r0,:64], r1
    vsubl.u8        \q3, d6, d7
.endm

.macro HADAMARD4_V r1, r2, r3, r4, t1, t2, t3, t4
    SUMSUB_ABCD \t1, \t2, \t3, \t4, \r1, \r2, \r3, \r4
    SUMSUB_ABCD \r1, \r3, \r2, \r4, \t1, \t3, \t2, \t4
.endm

.macro sa8d_satd_8x8 satd=
function x265_sa8d_\satd\()8x8_neon, export=0
    LOAD_DIFF_8x4   q8,  q9,  q10, q11
    vld1.64         {d7}, [r2], r3
    SUMSUB_AB       q0,  q1,  q8,  q9
    vld1.64         {d6}, [r0,:64], r1
    vsubl.u8        q12, d6,  d7
    vld1.64         {d17}, [r2], r3
    SUMSUB_AB       q2,  q3,  q10, q11
    vld1.64         {d16}, [r0,:64], r1
    vsubl.u8        q13, d16, d17
    vld1.64         {d19}, [r2], r3
    SUMSUB_AB       q8,  q10, q0,  q2
    vld1.64         {d18}, [r0,:64], r1
    vsubl.u8        q14, d18, d19
    vld1.64         {d1}, [r2], r3
    SUMSUB_AB       q9,  q11, q1,  q3
    vld1.64         {d0}, [r0,:64], r1
    vsubl.u8        q15, d0,  d1

    HADAMARD4_V     q12, q13, q14, q15,  q0,  q1,  q2,  q3

    SUMSUB_ABCD     q0,  q8,  q1,  q9,   q8,  q12, q9,  q13
    SUMSUB_AB       q2,  q10, q10, q14
    vtrn.16         q8,  q9
    SUMSUB_AB       q3,  q11, q11, q15
    vtrn.16         q0,  q1
    SUMSUB_AB       q12, q13, q8,  q9
    vtrn.16         q10, q11
    SUMSUB_AB       q8,  q9,  q0,  q1
    vtrn.16         q2,  q3
    SUMSUB_AB       q14, q15, q10, q11
    vadd.i16        q10, q2,  q3
    vtrn.32         q12, q14
    vsub.i16        q11, q2,  q3
    vtrn.32         q13, q15
    SUMSUB_AB       q0,  q2,  q12, q14
    vtrn.32         q8,  q10
    SUMSUB_AB       q1,  q3,  q13, q15
    vtrn.32         q9,  q11
    SUMSUB_AB       q12, q14, q8,  q10
    SUMSUB_AB       q13, q15, q9,  q11

    vswp            d1,  d24
    ABS2            q0,  q12
    vswp            d3,  d26
    ABS2            q1,  q13
    vswp            d5,  d28
    ABS2            q2,  q14
    vswp            d7,  d30
    ABS2            q3,  q15
    vmax.s16        q8,  q0,  q12
    vmax.s16        q9,  q1,  q13
    vmax.s16        q10, q2,  q14
    vmax.s16        q11, q3,  q15
    vadd.i16        q8,  q8,  q9
    vadd.i16        q9,  q10, q11

    bx              lr
endfunc
.endm

sa8d_satd_8x8

function x265_pixel_sa8d_8x8_neon
    mov             ip,  lr
    bl              x265_sa8d_8x8_neon
    vadd.u16        q0,  q8,  q9
    HORIZ_ADD       d0,  d0,  d1
    mov             lr,  ip
    vmov.32         r0,  d0[0]
    add             r0,  r0,  #1
    lsr             r0,  r0,  #1
    bx              lr
endfunc

function x265_pixel_sa8d_8x16_neon
    push            {r4, r5}
    mov             ip,  lr
    bl              x265_sa8d_8x8_neon
    vadd.u16        q0,  q8,  q9
    HORIZ_ADD       d0,  d0,  d1
    vmov.32         r5, d0[0]
    add             r5,  r5,  #1
    lsr             r5,  r5,  #1
    bl              x265_sa8d_8x8_neon
    vadd.u16        q0,  q8,  q9
    HORIZ_ADD       d0,  d0,  d1
    vmov.32         r4, d0[0]
    add             r4,  r4,  #1
    lsr             r4,  r4,  #1
    add             r0, r4, r5
    mov             lr,  ip
    pop             {r4, r5}
    bx              lr
endfunc

function x265_pixel_sa8d_16x16_neon
    vpush           {d8 - d11}
    mov             ip,  lr
    bl              x265_sa8d_8x8_neon
    vpaddl.u16      q4,  q8
    vpaddl.u16      q5,  q9
    bl              x265_sa8d_8x8_neon
    vpadal.u16      q4,  q8
    vpadal.u16      q5,  q9
    sub             r0,  r0,  r1,  lsl #4
    sub             r2,  r2,  r3,  lsl #4
    add             r0,  r0,  #8
    add             r2,  r2,  #8
    bl              x265_sa8d_8x8_neon
    vpadal.u16      q4,  q8
    vpadal.u16      q5,  q9
    bl              x265_sa8d_8x8_neon
    vpaddl.u16      q8,  q8
    vpaddl.u16      q9,  q9
    vadd.u32        q0,  q4,  q8
    vadd.u32        q1,  q5,  q9
    vadd.u32        q0,  q0,  q1
    vadd.u32        d0,  d0,  d1
    vpadd.u32       d0,  d0,  d0
    vpop            {d8-d11}
    mov             lr,  ip
    vmov.32         r0,  d0[0]
    add             r0,  r0,  #1
    lsr             r0,  r0,  #1
    bx              lr
endfunc

function x265_quant_neon
    push            {r4-r6}
    ldr             r4, [sp, #3* 4]
    mov             r12, #1
    lsl             r12, r4
    vdup.s32        d0, r12                // q0 = 2^qbits
    neg             r12, r4
    vdup.s32        q1, r12                // q1= -qbits
    add             r12, #8
    vdup.s32        q2, r12                // q2= -qbits+8
    ldr             r4, [sp, #3* 4 + 4]
    vdup.s32        q3, r4                 // q3= add
    ldr             r4, [sp, #3* 4 + 8]    // r4= numcoeff

    lsr             r4, r4 ,#2
    veor.s32        q4, q4                 // q4= accumulate numsig
    eor             r5, r5
    veor.s32        q12, q12

.loop_quant:

    vld1.s16        d16, [r0]!
    vmovl.s16       q9, d16                // q9= coef[blockpos]

    vclt.s32        q8, q9, #0             // q8= sign

    vabs.s32        q9, q9                 // q9= level=abs(coef[blockpos])
    vld1.s32        {q10}, [r1]!           // q10= quantCoeff[blockpos]
    vmul.i32        q9, q9, q10            // q9 = tmplevel = abs(level) * quantCoeff[blockpos];

    vadd.s32        q10, q9, q3            // q10= tmplevel+add
    vshl.s32        q10, q10, q1           // q10= level =(tmplevel+add) >> qbits

    vmls.s32        q9, q10, d0[0]         // q10= tmplevel - (level << qBits)
    vshl.s32        q11, q9, q2            // q11= ((tmplevel - (level << qBits)) >> qBits8)
    vst1.s32        {q11}, [r2]!           // store deltaU

    // numsig
    vceq.s32        q11, q10, q12
    vadd.s32        q4, q11
    add             r5, #4

    veor.s32        q11, q10, q8
    vsub.s32        q11, q11, q8
    vqmovn.s32      d16, q11
    vst1.s16        d16, [r3]!

    subs            r4, #1
    bne             .loop_quant

    vadd.u32        d8, d9
    vpadd.u32       d8, d8
    vmov.32         r12, d8[0]
    add             r0, r5, r12

    pop             {r4-r6}
    bx              lr
endfunc

function x265_nquant_neon
    push            {r4}
    neg             r12, r3
    vdup.s32        q0, r12                 // q0= -qbits
    ldr             r3, [sp, #1* 4]
    vdup.s32        q1, r3                  // add
    ldr             r3, [sp, #1* 4 + 4]     // numcoeff

    lsr             r3, r3 ,#2
    veor.s32        q4, q4                 // q4= accumulate numsig
    eor             r4, r4
    veor.s32        q12, q12

.loop_nquant:

    vld1.s16        d16, [r0]!
    vmovl.s16       q9, d16                // q9= coef[blockpos]

    vclt.s32        q8, q9, #0             // q8= sign

    vabs.s32        q9, q9                 // q9= level=abs(coef[blockpos])
    vld1.s32        {q10}, [r1]!           // q10= quantCoeff[blockpos]
    vmul.i32        q9, q9, q10            // q9 = tmplevel = abs(level) * quantCoeff[blockpos];

    vadd.s32        q10, q9, q1            // q10= tmplevel+add
    vshl.s32        q10, q10, q0           // q10= level =(tmplevel+add) >> qbits

    // numsig
    vceq.s32        q11, q10, q12
    vadd.s32        q4, q11
    add             r4, #4

    veor.s32        q11, q10, q8
    vsub.s32        q11, q11, q8
    vqmovn.s32      d16, q11
    vabs.s16        d17, d16
    vst1.s16        d17, [r2]!

    subs            r3, #1
    bne             .loop_nquant

    vadd.u32        d8, d9
    vpadd.u32       d8, d8
    vmov.32         r12, d8[0]
    add             r0, r4, r12

    pop             {r4}
    bx              lr
endfunc
.macro sa8d_16x16 reg
    bl              x265_sa8d_8x8_neon
    vpaddl.u16      q4,  q8
    vpaddl.u16      q5,  q9
    bl              x265_sa8d_8x8_neon
    vpadal.u16      q4,  q8
    vpadal.u16      q5,  q9
    sub             r0,  r0,  r1,  lsl #4
    sub             r2,  r2,  r3,  lsl #4
    add             r0,  r0,  #8
    add             r2,  r2,  #8
    bl              x265_sa8d_8x8_neon
    vpadal.u16      q4,  q8
    vpadal.u16      q5,  q9
    bl              x265_sa8d_8x8_neon
    vpaddl.u16      q8,  q8
    vpaddl.u16      q9,  q9
    vadd.u32        q0,  q4,  q8
    vadd.u32        q1,  q5,  q9
    vadd.u32        q0,  q0,  q1
    vadd.u32        d0,  d0,  d1
    vpadd.u32       d0,  d0,  d0
    vmov.32         \reg,  d0[0]
    add             \reg,  \reg,  #1
    lsr             \reg,  \reg,  #1
.endm

function x265_pixel_sa8d_16x32_neon
    push            {r4, r5}
    vpush           {d8 - d11}
    mov             ip,  lr

    sa8d_16x16 r4

    sub             r0,  r0,  #8
    sub             r2,  r2,  #8

    sa8d_16x16 r5

    add             r0, r4, r5
    vpop            {d8 - d11}
    pop             {r4, r5}
    mov             lr,  ip
    bx              lr
endfunc

function x265_pixel_sa8d_32x32_neon
    push            {r4 - r7}
    vpush           {d8 - d11}
    mov             ip,  lr

    sa8d_16x16 r4

    sub             r0,  r0,  r1,  lsl #4
    sub             r2,  r2,  r3,  lsl #4
    add             r0,  r0,  #8
    add             r2,  r2,  #8

    sa8d_16x16 r5

    sub             r0,  r0,  #24
    sub             r2,  r2,  #24

   sa8d_16x16 r6

    sub             r0,  r0,  r1,  lsl #4
    sub             r2,  r2,  r3,  lsl #4
    add             r0,  r0,  #8
    add             r2,  r2,  #8

   sa8d_16x16 r7

    add             r4, r4, r5
    add             r6, r6, r7
    add             r0, r4, r6
    vpop            {d8 - d11}
    pop             {r4 - r7}
    mov             lr,  ip
    bx              lr
endfunc

function x265_pixel_sa8d_32x64_neon
    push            {r4 - r10}
    vpush           {d8 - d11}
    mov             ip,  lr

    mov             r10, #4
    eor             r9, r9

.loop_32:

    sa8d_16x16 r4

    sub             r0,  r0,  r1,  lsl #4
    sub             r2,  r2,  r3,  lsl #4
    add             r0,  r0,  #8
    add             r2,  r2,  #8

    sa8d_16x16 r5

    add             r4, r4, r5
    add             r9, r9, r4

    sub             r0,  r0,  #24
    sub             r2,  r2,  #24

    subs            r10, #1
    bgt            .loop_32

    mov             r0, r9
    vpop            {d8-d11}
    pop             {r4-r10}
    mov             lr,  ip
    bx              lr
endfunc

function x265_pixel_sa8d_64x64_neon
    push            {r4-r10}
    vpush           {d8-d11}
    mov             ip,  lr

    mov             r10, #4
    eor             r9, r9

.loop_1:

    sa8d_16x16 r4

    sub             r0,  r0,  r1,  lsl #4
    sub             r2,  r2,  r3,  lsl #4
    add             r0,  r0,  #8
    add             r2,  r2,  #8

    sa8d_16x16 r5

    sub             r0,  r0,  r1,  lsl #4
    sub             r2,  r2,  r3,  lsl #4
    add             r0,  r0,  #8
    add             r2,  r2,  #8

   sa8d_16x16 r6

    sub             r0,  r0,  r1,  lsl #4
    sub             r2,  r2,  r3,  lsl #4
    add             r0,  r0,  #8
    add             r2,  r2,  #8

   sa8d_16x16 r7

    add             r4, r4, r5
    add             r6, r6, r7
    add             r8, r4, r6
    add             r9, r9, r8

    sub             r0,  r0,  #56
    sub             r2,  r2,  #56

    subs            r10, #1
    bgt            .loop_1

    mov             r0, r9
    vpop            {d8-d11}
    pop             {r4-r10}
    mov             lr,  ip
    bx              lr
endfunc

/***** dequant_scaling*****/
// void dequant_scaling_c(const int16_t* quantCoef, const int32_t* deQuantCoef, int16_t* coef, int num, int per, int shift)
function x265_dequant_scaling_neon
    push            {r4, r5, r6, r7}
    ldr             r4, [sp, #16]       // per
    ldr             r5, [sp, #20]       //.shift
    add             r5, #4              // shift + 4
    lsr             r3, #3              // num / 8
    cmp             r5, r4
    blt             skip

    mov             r12, #1
    sub             r6, r5, r4          // shift - per
    sub             r6, #1              // shift - per - 1
    lsl             r6, r12, r6         // 1 << shift - per - 1 (add)
    vdup.32         q0, r6
    sub             r7, r4, r5          // per - shift
    vdup.32         q3, r7

dequant_loop1:
    vld1.16         {q9}, [r0]!          // quantCoef
    vld1.32         {q2}, [r1]!          // deQuantCoef
    vld1.32         {q10}, [r1]!
    vmovl.s16       q1, d18
    vmovl.s16       q9, d19

    vmul.s32        q1, q2              // quantCoef * deQuantCoef
    vmul.s32        q9, q10
    vadd.s32        q1, q0              // quantCoef * deQuantCoef + add
    vadd.s32        q9, q0

    vshl.s32        q1, q3
    vshl.s32        q9, q3
    vqmovn.s32      d16, q1             // x265_clip3
    vqmovn.s32      d17, q9
    subs            r3, #1
    vst1.16         {q8}, [r2]!
    bne             dequant_loop1
    b               1f

skip:
    sub             r6, r4, r5          // per - shift
    vdup.16         q0, r6

dequant_loop2:
    vld1.16         {q9}, [r0]!          // quantCoef
    vld1.32         {q2}, [r1]!          // deQuantCoef
    vld1.32         {q10}, [r1]!
    vmovl.s16       q1, d18
    vmovl.s16       q9, d19

    vmul.s32        q1, q2              // quantCoef * deQuantCoef
    vmul.s32        q9, q10
    vqmovn.s32      d16, q1             // x265_clip3
    vqmovn.s32      d17, q9

    vqshl.s16       q8, q0             // coefQ << per - shift
    subs            r3, #1
    vst1.16         {q8}, [r2]!
    bne             dequant_loop2
1:
    pop             {r4, r5, r6, r7}
    bx              lr
endfunc

// void dequant_normal_c(const int16_t* quantCoef, int16_t* coef, int num, int scale, int shift)
function x265_dequant_normal_neon
    ldr             r12, [sp]            // shift
#if HIGH_BIT_DEPTH  // NEVER TEST path
    cmp             r3, #32768
    lsrlt           r3, #(BIT_DEPTH - 8)
    sublt           r12, #(BIT_DEPTH - 8)
#endif
    lsr             r2, #4              // num / 16

    neg             r12, r12
    vdup.16         q0, r3
    vdup.32         q1, r12

.dqn_loop1:
    vld1.16         {d4-d7}, [r0]!

    vmull.s16       q8, d4, d0
    vmull.s16       q9, d5, d0
    vmull.s16       q10, d6, d0
    vmull.s16       q11, d7, d0

    vrshl.s32       q8, q1
    vrshl.s32       q9, q1
    vrshl.s32       q10, q1
    vrshl.s32       q11, q1
    vqmovn.s32      d16, q8
    vqmovn.s32      d17, q9
    vqmovn.s32      d18, q10
    vqmovn.s32      d19, q11

    subs            r2, #1
    vst1.16         {d16-d19}, [r1]!
    bgt            .dqn_loop1
    bx              lr
endfunc

/********* ssim ***********/
// void x265_ssim_4x4x2_core_neon(const pixel* pix1, intptr_t stride1, const pixel* pix2, intptr_t stride2, int sums[2][4]);
function x265_ssim_4x4x2_core_neon
    ldr             r12, [sp]

    vld1.64         {d0}, [r0], r1
    vld1.64         {d1}, [r0], r1
    vld1.64         {d2}, [r0], r1
    vld1.64         {d3}, [r0], r1

    vld1.64         {d4}, [r2], r3
    vld1.64         {d5}, [r2], r3
    vld1.64         {d6}, [r2], r3
    vld1.64         {d7}, [r2], r3

    vpaddl.u8       q8, q0
    vpadal.u8       q8, q1
    vpaddl.u8       q9, q2
    vpadal.u8       q9, q3
    vadd.u16        d16, d17
    vpaddl.u16      d16, d16
    vadd.u16        d18, d19
    vpaddl.u16      d17, d18

    vmull.u8        q10, d0, d0
    vmull.u8        q11, d1, d1
    vmull.u8        q12, d2, d2
    vmull.u8        q13, d3, d3
    vpaddl.u16      q10, q10
    vpadal.u16      q10, q11
    vpadal.u16      q10, q12
    vpadal.u16      q10, q13

    vmull.u8        q9, d4, d4
    vmull.u8        q11, d5, d5
    vmull.u8        q12, d6, d6
    vmull.u8        q13, d7, d7
    vpadal.u16      q10, q9
    vpadal.u16      q10, q11
    vpadal.u16      q10, q12
    vpadal.u16      q10, q13
    vpadd.u32       d18, d20, d21

    vmull.u8        q10, d0, d4
    vmull.u8        q11, d1, d5
    vmull.u8        q12, d2, d6
    vmull.u8        q13, d3, d7
    vpaddl.u16      q10, q10
    vpadal.u16      q10, q11
    vpadal.u16      q10, q12
    vpadal.u16      q10, q13
    vpadd.u32       d19, d20, d21

    vst4.32         {d16-d19}, [r12]
    bx              lr
endfunc

// int psyCost_pp(const pixel* source, intptr_t sstride, const pixel* recon, intptr_t rstride)
function x265_psyCost_4x4_neon
    vld1.32         {d16[]}, [r0,:32], r1                   // d16 = [A03 A02 A01 A00 A03 A02 A01 A00]
    vld1.32         {d17[]}, [r0,:32], r1                   // d17 = [A13 A12 A11 A10 A13 A12 A11 A10]
    vld1.32         {d16[1]}, [r0,:32], r1                  // d16 = [A23 A22 A21 A20 A03 A02 A01 A00]
    vld1.32         {d17[1]}, [r0,:32], r1                  // d17 = [A33 A32 A31 A30 A13 A12 A11 A10]

    vld1.32         {d18[]}, [r2,:32], r3                   // d18 = [B03 B02 B01 B00 B03 B02 B01 B00]
    vld1.32         {d19[]}, [r2,:32], r3                   // d19 = [B13 B12 B11 B10 B13 B12 B11 B10]
    vld1.32         {d18[1]}, [r2,:32], r3                  // d18 = [B23 B22 B21 B20 B03 B02 B01 B00]
    vld1.32         {d19[1]}, [r2,:32], r3                  // d19 = [B33 B32 B31 B30 B13 B12 B11 B10]

    vaddl.u8        q2, d16, d17                            // q2 = [2+3 0+1]
    vsubl.u8        q3, d16, d17                            // q3 = [2-3 0-1]
    vaddl.u8        q12, d18, d19
    vsubl.u8        q13, d18, d19

    SUMSUB_ABCD     d0, d2, d1, d3, d4, d5, d6, d7          // q0 = [(0-1)+(2-3) (0+1)+(2+3)], q1 = [(0-1)-(2-3) (0+1)-(2+3)]
    SUMSUB_ABCD     d20, d22, d21, d23, d24, d25, d26, d27

    // Hadamard-1D
    vtrn.16         q0, q1
    vtrn.16         q10, q11
    SUMSUB_AB       q2, q3, q0, q1                          // q2 = [((0-1)-(2-3))+((0-1)+(2-3)) ((0+1)-(2+3))+((0+1)+(2+3))], q3 = [((0-1)-(2-3))-((0-1)+(2-3)) ((0+1)-(2+3))-((0+1)+(2+3))]
    SUMSUB_AB       q12, q13, q10, q11

    // SAD Stage-0
    vaddl.u8        q14, d16, d17                           // q14 = [S23x4 S01x4]
    vaddl.u8        q15, d18, d19

    // Hadamard-2D
    vtrn.32         q2, q3
    vtrn.32         q12, q13
    vabs.s16        q2, q2
    vabs.s16        q12, q12
    vabs.s16        q3, q3
    vabs.s16        q13, q13

    // SAD Stage-1
    vadd.u16        d28, d29                                // SAD: reduce to 4 elements
    vadd.u16        d30, d31

    vmax.s16        q0, q2, q3
    vmax.s16        q10, q12, q13

    // SAD Stage-2
    vpadd.u16       d28, d30                                // SAD: reduce to 2 elements

    // SAD & SATD Final Stage
    vswp            d1, d20
    vadd.u16        q0, q10
    vpaddl.u16      d28, d28                                // d28 = SAD_DWORD[B A]
    vpadd.u16       d0, d1
    vshr.u32        d28, #2                                 // d28 = SAD_DWORD[B A] >> 2
    vpaddl.u16      d0, d0                                  // d0 = SATD_DWORD[B A]
    vsub.s32        d0, d28                                 // d0 = SATD - SAD
    vmov.32         r0, d0[0]
    vmov.32         r1, d0[1]
    subs            r0, r1
    rsbmi           r0, r0, #0

    bx              lr
endfunc

function x265_psyCost_8x8_neon

    vpush           {q4-q7}

    vld1.8          {d0}, [r0], r1
    vld1.8          {d1}, [r0], r1
    vmovl.u8        q8, d0
    vld1.8          {d2}, [r0], r1
    vmovl.u8        q9, d1
    vld1.8          {d3}, [r0], r1
    vmovl.u8        q10, d2
    vld1.8          {d4}, [r0], r1
    vmovl.u8        q11, d3
    vld1.8          {d5}, [r0], r1
    vmovl.u8        q12, d4
    vld1.8          {d6}, [r0], r1
    vmovl.u8        q13, d5
    vld1.8          {d7}, [r0], r1
    vmovl.u8        q14, d6
    vmovl.u8        q15, d7

    // SAD Stage-0
    vadd.u16        q4, q8, q9
    vadd.u16        q5, q10, q11
    vadd.u16        q6, q12, q13
    vadd.u16        q7, q14, q15

    // SAD Stage-1
    vadd.u16        q4, q5
    vadd.u16        q6, q7
    vadd.u16        q4, q6
    vpadd.u16       d8, d9
    vpaddl.u16      d8, d8
    vpadd.u32       d8, d8
    vshr.u32        d8, #2

    // sa8d
    SUMSUB_AB       q0,  q1,  q8,  q9
    SUMSUB_AB       q2,  q3,  q10, q11
    SUMSUB_AB       q8,  q10, q0,  q2
    SUMSUB_AB       q9,  q11, q1,  q3

    HADAMARD4_V     q12, q13, q14, q15,  q0,  q1,  q2,  q3

    SUMSUB_ABCD     q0,  q8,  q1,  q9,   q8,  q12, q9,  q13
    SUMSUB_AB       q2,  q10, q10, q14
    vtrn.16         q8,  q9
    SUMSUB_AB       q3,  q11, q11, q15
    vtrn.16         q0,  q1
    SUMSUB_AB       q12, q13, q8,  q9
    vtrn.16         q10, q11
    SUMSUB_AB       q8,  q9,  q0,  q1
    vtrn.16         q2,  q3
    SUMSUB_AB       q14, q15, q10, q11
    vadd.i16        q10, q2,  q3
    vtrn.32         q12, q14
    vsub.i16        q11, q2,  q3
    vtrn.32         q13, q15
    SUMSUB_AB       q0,  q2,  q12, q14
    vtrn.32         q8,  q10
    SUMSUB_AB       q1,  q3,  q13, q15
    vtrn.32         q9,  q11
    SUMSUB_AB       q12, q14, q8,  q10
    SUMSUB_AB       q13, q15, q9,  q11

    vswp            d1,  d24
    ABS2            q0,  q12
    vswp            d3,  d26
    ABS2            q1,  q13
    vswp            d5,  d28
    ABS2            q2,  q14
    vswp            d7,  d30
    ABS2            q3,  q15
    vmax.s16        q8,  q0,  q12
    vmax.s16        q9,  q1,  q13
    vmax.s16        q10, q2,  q14
    vmax.s16        q11, q3,  q15
    vadd.i16        q8,  q8,  q9
    vadd.i16        q9,  q10, q11
    vadd.u16        q0, q8, q9
    vadd.u16        d0, d1
    vpaddl.u16      d0, d0
    vpadd.u32       d0, d0
    vmov.32         r0, d0[0]
    add             r0, r0, #1
    lsr             r0, r0, #1
//-------------------------------------------------------------
    vld1.8          d0, [r2], r3
    vld1.8          d1, [r2], r3
    vmovl.u8        q8, d0
    vld1.8          d2, [r2], r3
    vmovl.u8        q9, d1
    vld1.8          d3, [r2], r3
    vmovl.u8        q10, d2
    vld1.8          d4, [r2], r3
    vmovl.u8        q11, d3
    vld1.8          d5, [r2], r3
    vmovl.u8        q12, d4
    vld1.8          d6, [r2], r3
    vmovl.u8        q13, d5
    vld1.8          d7, [r2], r3
    vmovl.u8        q14, d6
    vmovl.u8        q15, d7

    // SAD Stage-0
    vadd.u16       q5, q8, q9
    vadd.u16       q6, q10, q11
    vadd.u16       q7, q12, q13
    vadd.u16       q0, q14, q15

    // SAD Stage-1
    vadd.u16        q5, q6
    vadd.u16        q7, q0
    vadd.u16        q5, q7
    vadd.u16        d10, d11
    vpaddl.u16      d10, d10
    vpadd.u32       d10, d10
    vshr.u32        d10, #2

    // sa8d
    SUMSUB_AB       q0,  q1,  q8,  q9
    SUMSUB_AB       q2,  q3,  q10, q11
    SUMSUB_AB       q8,  q10, q0,  q2
    SUMSUB_AB       q9,  q11, q1,  q3

    HADAMARD4_V     q12, q13, q14, q15,  q0,  q1,  q2,  q3

    SUMSUB_ABCD     q0,  q8,  q1,  q9,   q8,  q12, q9,  q13
    SUMSUB_AB       q2,  q10, q10, q14
    vtrn.16         q8,  q9
    SUMSUB_AB       q3,  q11, q11, q15
    vtrn.16         q0,  q1
    SUMSUB_AB       q12, q13, q8,  q9
    vtrn.16         q10, q11
    SUMSUB_AB       q8,  q9,  q0,  q1
    vtrn.16         q2,  q3
    SUMSUB_AB       q14, q15, q10, q11
    vadd.i16        q10, q2,  q3
    vtrn.32         q12, q14
    vsub.i16        q11, q2,  q3
    vtrn.32         q13, q15
    SUMSUB_AB       q0,  q2,  q12, q14
    vtrn.32         q8,  q10
    SUMSUB_AB       q1,  q3,  q13, q15
    vtrn.32         q9,  q11
    SUMSUB_AB       q12, q14, q8,  q10
    SUMSUB_AB       q13, q15, q9,  q11

    vswp            d1,  d24
    ABS2            q0,  q12
    vswp            d3,  d26
    ABS2            q1,  q13
    vswp            d5,  d28
    ABS2            q2,  q14
    vswp            d7,  d30
    ABS2            q3,  q15
    vmax.s16        q8,  q0,  q12
    vmax.s16        q9,  q1,  q13
    vmax.s16        q10, q2,  q14
    vmax.s16        q11, q3,  q15
    vadd.i16        q8,  q8,  q9
    vadd.i16        q9,  q10, q11
    vadd.u16        q0, q8, q9
    vadd.u16        d0, d1
    vpaddl.u16      d0, d0
    vpadd.u32       d0, d0
    vmov.32         r2, d0[0]
    add             r2, r2, #1
    lsr             r2, r2, #1

    // SAD & SA8D Final Stage
    vmov.32         r1, d8[0]
    sub             r0, r1
    vmov.32         r3, d10[0]
    sub             r2, r3
    cmp             r0, r2
    bgt             subr0
    sub             r0, r2, r0
    b               end
subr0:
    sub             r0, r2
end:

    vpop            {q4-q7}
    bx              lr
endfunc


