/*****************************************************************************
 * Copyright (C) 2013-2017 MulticoreWare, Inc
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

/******** Chroma blockcopy********/
function x265_blockcopy_ss_4x8_neon
    lsl             r1, #1
    lsl             r3, #1
.rept 4
    vld1.u16        {d0}, [r2], r3
    vld1.u16        {d1}, [r2], r3
    vst1.u16        {d0}, [r0], r1
    vst1.u16        {d1}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_ss_8x16_neon
    lsl             r1, #1
    lsl             r3, #1
.rept 8
    vld1.u16        {q0}, [r2], r3
    vld1.u16        {q1}, [r2], r3
    vst1.u16        {q0}, [r0], r1
    vst1.u16        {q1}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_ss_16x32_neon
    lsl             r1, #1
    lsl             r3, #1
.rept 16
    vld1.u16        {q0, q1}, [r2], r3
    vld1.u16        {q2, q3}, [r2], r3
    vst1.u16        {q0, q1}, [r0], r1
    vst1.u16        {q2, q3}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_ss_32x64_neon
    lsl             r1, #1
    lsl             r3, #1
    mov             r12, #8
    sub             r1, #32
    sub             r3, #32
loop_css_32x64:
    subs            r12, #1
.rept 8
    vld1.u16        {q0, q1}, [r2]!
    vld1.u16        {q2, q3}, [r2], r3
    vst1.u16        {q0, q1}, [r0]!
    vst1.u16        {q2, q3}, [r0], r1
.endr
    bne             loop_css_32x64
    bx              lr
endfunc

// chroma blockcopy_ps
function x265_blockcopy_ps_4x8_neon
    lsl             r1, #1
.rept 4
    vld1.u8         {d0}, [r2], r3
    vld1.u8         {d1}, [r2], r3
    vmovl.u8        q1, d0
    vmovl.u8        q2, d1
    vst1.u16        {d2}, [r0], r1
    vst1.u16        {d4}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_ps_8x16_neon
    lsl             r1, #1
.rept 8
    vld1.u8         {d0}, [r2], r3
    vld1.u8         {d1}, [r2], r3
    vmovl.u8        q1, d0
    vmovl.u8        q2, d1
    vst1.u16        {q1}, [r0], r1
    vst1.u16        {q2}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_ps_16x32_neon
    lsl             r1, #1
    mov             r12, #4
loop_cps_16x32:
    subs            r12, #1
.rept 4
    vld1.u8         {q0}, [r2], r3
    vld1.u8         {q1}, [r2], r3
    vmovl.u8        q8, d0
    vmovl.u8        q9, d1
    vmovl.u8        q10, d2
    vmovl.u8        q11, d3
    vst1.u16        {q8, q9}, [r0], r1
    vst1.u16        {q10, q11}, [r0], r1
.endr
    bne             loop_cps_16x32
    bx              lr
endfunc

function x265_blockcopy_ps_32x64_neon
    lsl             r1, #1
    sub             r1, #32
    mov             r12, #8
loop_cps_32x64:
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
    bne             loop_cps_32x64
    bx              lr
endfunc

// chroma blockcopy_sp
function x265_blockcopy_sp_4x8_neon
    lsl             r3, #1
.rept 4
    vld1.u16        {q0}, [r2], r3
    vld1.u16        {q1}, [r2], r3
    vmovn.u16       d0, q0
    vmovn.u16       d1, q1
    vst1.u32        {d0[0]}, [r0], r1
    vst1.u32        {d1[0]}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_sp_8x16_neon
    lsl             r3, #1
.rept 8
    vld1.u16        {q0}, [r2], r3
    vld1.u16        {q1}, [r2], r3
    vmovn.u16       d0, q0
    vmovn.u16       d1, q1
    vst1.u8         {d0}, [r0], r1
    vst1.u8         {d1}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_sp_16x32_neon
    lsl             r3, #1
    mov             r12, #4
loop_csp_16x32:
    subs            r12, #1
.rept 4
    vld1.u16        {q0, q1}, [r2], r3
    vld1.u16        {q2, q3}, [r2], r3
    vmovn.u16       d0, q0
    vmovn.u16       d1, q1
    vmovn.u16       d2, q2
    vmovn.u16       d3, q3
    vst1.u8         {q0}, [r0], r1
    vst1.u8         {q1}, [r0], r1
.endr
    bne             loop_csp_16x32
    bx              lr
endfunc

function x265_blockcopy_sp_32x64_neon
    mov             r12, #8
    lsl             r3, #1
    sub             r3, #32
loop_csp_32x64:
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
    bne             loop_csp_32x64
    bx              lr
endfunc

// void x265_blockfill_s_neon(int16_t* dst, intptr_t dstride, int16_t val)
function x265_blockfill_s_4x4_neon
    vdup.u16        d0, r2
    lsl             r1, #1
.rept 4
    vst1.16         {d0}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockfill_s_8x8_neon
    vdup.u16        q0, r2
    lsl             r1, #1
.rept 8
    vst1.16         {q0}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockfill_s_16x16_neon
    vdup.u16        q0, r2
    vmov            q1, q0
    lsl             r1, #1
.rept 16
    vst1.16         {q0, q1}, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockfill_s_32x32_neon
    vdup.u16        q0, r2
    vmov            q1, q0
    lsl             r1, #1
    sub             r1, #32
.rept 32
    vst1.16         {q0, q1}, [r0]!
    vst1.16         {q0, q1}, [r0], r1
.endr
    bx              lr
endfunc

// uint32_t copy_count(int16_t* coeff, const int16_t* residual, intptr_t resiStride)
function x265_copy_cnt_4_neon
    lsl             r2, #1
    mov             r12, #8
    veor            d4, d4
.rept 2
    vld1.s16        {d0}, [r1], r2
    vld1.s16        {d1}, [r1], r2
    vclz.i16        d2, d0
    vclz.i16        d3, d1
    vshr.u16        q1, #4
    vadd.u16        d2, d3
    vadd.u16        d4, d2
    vst1.s16        {d0}, [r0], r12
    vst1.s16        {d1}, [r0], r12
.endr
    vpadd.u16       d4, d4
    vpadd.u16       d4, d4
    vmov.u16        r12, d4[0]
    rsb             r0, r12, #16
    bx              lr
endfunc

function x265_copy_cnt_8_neon
    lsl             r2, #1
    mov             r12, #16
    veor            q8, q8
.rept 4
    vld1.s16        {q0}, [r1], r2
    vld1.s16        {q1}, [r1], r2
    vclz.i16        q2, q0
    vclz.i16        q3, q1
    vshr.u16        q2, #4
    vshr.u16        q3, #4
    vadd.u16        q2, q3
    vadd.u16        q8, q2
    vst1.s16        {q0}, [r0], r12
    vst1.s16        {q1}, [r0], r12
.endr
    vadd.u16        d16, d17
    vpadd.u16       d16, d16
    vpadd.u16       d16, d16
    vmov.u16        r12, d16[0]
    rsb             r0, r12, #64
    bx              lr
endfunc

function x265_copy_cnt_16_neon
    lsl             r2, #1
    mov             r12, #32
    veor            q2, q2
.rept 16
    vld1.s16        {q0, q1}, [r1], r2
    vst1.s16        {q0, q1}, [r0], r12
    vclz.i16        q8, q0
    vclz.i16        q9, q1
    vshr.u16        q8, #4
    vshr.u16        q9, #4
    vadd.u16        q8, q9
    vadd.u16        q2, q8
.endr
    vadd.u16        d4, d5
    vpadd.u16       d4, d4
    vpadd.u16       d4, d4

    vmov.u16        r12, d4[0]
    rsb             r0, r12, #256
    bx              lr
endfunc

function x265_copy_cnt_32_neon
    lsl             r2, #1
    sub             r2, #32
    mov             r12, #32
    veor            q12, q12
.rept 32
    vld1.s16        {q0, q1}, [r1]!
    vld1.s16        {q2, q3}, [r1], r2
    vst1.s16        {q0, q1}, [r0]!
    vst1.s16        {q2, q3}, [r0], r12

    vclz.i16        q8, q0
    vclz.i16        q9, q1
    vclz.i16        q10, q2
    vclz.i16        q11, q3

    vshr.u16        q8, #4
    vshr.u16        q9, #4
    vshr.u16        q10, #4
    vshr.u16        q11, #4

    vadd.u16        q8, q9
    vadd.u16        q10, q11
    vadd.u16        q8, q10
    vadd.u16        q12, q8
.endr
    vadd.u16        d24, d25
    vpadd.u16       d24, d24
    vpadd.u16       d24, d24

    vmov.u16        r12, d24[0]
    rsb             r0, r12, #1024
    bx              lr
endfunc

// int  count_nonzero_c(const int16_t* quantCoeff)
function x265_count_nonzero_4_neon
    vld1.s16        {d0-d3}, [r0]
    vceq.u16        q0, #0
    vceq.u16        q1, #0
    eor             r1, r1
    vtrn.8          q0, q1

    vshr.u8         q0, #7

    vadd.u8         d0, d1
    vshr.u64        d1, d0, #32
    vadd.u8         d0, d1
    vmov.u32        r0, d0[0]
    usad8           r0, r0, r1
    rsb             r0, #16
    bx              lr
endfunc

function x265_count_nonzero_8_neon
    vldm            r0, {q8-q15}
    eor             r1, r1
    vceq.u16        q8, #0
    vceq.u16        q9, #0
    vceq.u16        q10, #0
    vceq.u16        q11, #0
    vceq.u16        q12, #0
    vceq.u16        q13, #0
    vceq.u16        q14, #0
    vceq.u16        q15, #0

    vtrn.8          q8, q9
    vtrn.8          q10, q11
    vtrn.8          q12, q13
    vtrn.8          q14, q15

    vadd.s8         q8, q10
    vadd.s8         q12, q14
    vadd.s8         q8, q12

    vadd.s8         d16, d17
    vshr.u64        d17, d16, #32
    vadd.s8         d16, d17
    vabs.s8         d16, d16

    vmov.u32        r0, d16[0]
    usad8           r0, r0, r1
    rsb             r0, #64
    bx              lr
endfunc

function x265_count_nonzero_16_neon
    vldm            r0!, {q8-q15}
    eor             r1, r1
    vceq.u16        q8, #0
    vceq.u16        q9, #0
    vceq.u16        q10, #0
    vceq.u16        q11, #0
    vceq.u16        q12, #0
    vceq.u16        q13, #0
    vceq.u16        q14, #0
    vceq.u16        q15, #0

    vtrn.8          q8, q9
    vtrn.8          q10, q11
    vtrn.8          q12, q13
    vtrn.8          q14, q15

    vmov            q0, q8
    vmov            q1, q10
    vmov            q2, q12
    vmov            q3, q14

.rept 3
    vldm            r0!, {q8-q15}
    vceq.u16        q8, #0
    vceq.u16        q9, #0
    vceq.u16        q10, #0
    vceq.u16        q11, #0
    vceq.u16        q12, #0
    vceq.u16        q13, #0
    vceq.u16        q14, #0
    vceq.u16        q15, #0

    vtrn.8          q8, q9
    vtrn.8          q10, q11
    vtrn.8          q12, q13
    vtrn.8          q14, q15

    vadd.s8         q0, q8
    vadd.s8         q1, q10
    vadd.s8         q2, q12
    vadd.s8         q3, q14
.endr

    vadd.s8         q0, q1
    vadd.s8         q2, q3
    vadd.s8         q0, q2                      // dynamic range is 4+1 bits

    vadd.s8         d0, d1
    vshr.u64        d1, d0, #32
    vadd.s8         d0, d1
    vabs.s8         d0, d0                      // maximum value of each element are 64

    vmov.u32        r0, d0[0]
    usad8           r0, r0, r1
    rsb             r0, #256
    bx              lr
endfunc

function x265_count_nonzero_32_neon
    vldm            r0!, {q8-q15}
    vceq.u16        q8, #0
    vceq.u16        q9, #0
    vceq.u16        q10, #0
    vceq.u16        q11, #0
    vceq.u16        q12, #0
    vceq.u16        q13, #0
    vceq.u16        q14, #0
    vceq.u16        q15, #0

    vtrn.8          q8, q9
    vtrn.8          q10, q11
    vtrn.8          q12, q13
    vtrn.8          q14, q15

    mov             r1, #15

    vmov            q0, q8
    vmov            q1, q10
    vmov            q2, q12
    vmov            q3, q14

.loop:    
    vldm            r0!, {q8-q15}
    subs            r1, #1

    vceq.u16        q8, #0
    vceq.u16        q9, #0
    vceq.u16        q10, #0
    vceq.u16        q11, #0
    vceq.u16        q12, #0
    vceq.u16        q13, #0
    vceq.u16        q14, #0
    vceq.u16        q15, #0

    vtrn.8          q8, q9
    vtrn.8          q10, q11
    vtrn.8          q12, q13
    vtrn.8          q14, q15

    vadd.s8         q0, q8
    vadd.s8         q1, q10
    vadd.s8         q2, q12
    vadd.s8         q3, q14
    bgt            .loop

    // sum
    vadd.s8         q0, q1
    vadd.s8         q2, q3
    vadd.s8         q0, q2                      // dynamic range is 6+1 bits

    vaddl.s8        q0, d0, d1
    vadd.s16        d0, d1
    vshr.u64        d1, d0, #32
    vadd.s16        d0, d1
    vabs.s16        d0, d0                      //  maximum value of each element are 512

    vmov.u32        r0, d0[0]
    uasx            r0, r0, r0
    mov             r0, r0, lsr 16
    rsb             r0, #1024
    bx              lr
endfunc
