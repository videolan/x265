/*****************************************************************************
 * Copyright (C) 2013-2017 MulticoreWare, Inc
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


function x265_pixel_sse_pp_4x4_neon
    vld1.32     {d16[]}, [r0], r1
    vld1.32     {d17[]}, [r2], r3
    vsubl.u8    q2, d16, d17
    vld1.32     {d16[]}, [r0], r1
    vmull.s16   q0, d4, d4
    vld1.32     {d17[]}, [r2], r3

    vsubl.u8    q2, d16, d17
    vld1.32     {d16[]}, [r0], r1
    vmlal.s16   q0, d4, d4
    vld1.32     {d17[]}, [r2], r3

    vsubl.u8    q2, d16, d17
    vld1.32     {d16[]}, [r0], r1
    vmlal.s16   q0, d4, d4
    vld1.32     {d17[]}, [r2], r3

    vsubl.u8    q2, d16, d17
    vmlal.s16   q0, d4, d4
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_sse_pp_8x8_neon
    vld1.64     {d16}, [r0], r1
    vld1.64     {d17}, [r2], r3
    vsubl.u8    q2, d16, d17
    vld1.64     {d16}, [r0], r1
    vmull.s16   q0, d4, d4
    vmlal.s16   q0, d5, d5
    vld1.64     {d17}, [r2], r3

.rept 6
    vsubl.u8    q2, d16, d17
    vld1.64     {d16}, [r0], r1
    vmlal.s16   q0, d4, d4
    vmlal.s16   q0, d5, d5
    vld1.64     {d17}, [r2], r3
.endr
    vsubl.u8    q2, d16, d17
    vmlal.s16   q0, d4, d4
    vmlal.s16   q0, d5, d5
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_sse_pp_16x16_neon
    vld1.64     {d16-d17}, [r0], r1
    vld1.64     {d18-d19}, [r2], r3
    vsubl.u8    q2, d16, d18
    vsubl.u8    q3, d17, d19
    vld1.64     {d16-d17}, [r0], r1
    vmull.s16   q0, d4, d4
    vmlal.s16   q0, d5, d5
    vld1.64     {d18-d19}, [r2], r3
    vmlal.s16   q0, d6, d6
    vmlal.s16   q0, d7, d7

.rept 14
    vsubl.u8    q2, d16, d18
    vsubl.u8    q3, d17, d19
    vld1.64     {d16-d17}, [r0], r1
    vmlal.s16   q0, d4, d4
    vmlal.s16   q0, d5, d5
    vld1.64     {d18-d19}, [r2], r3
    vmlal.s16   q0, d6, d6
    vmlal.s16   q0, d7, d7
.endr
    vsubl.u8    q2, d16, d18
    vsubl.u8    q3, d17, d19
    vmlal.s16   q0, d4, d4
    vmlal.s16   q0, d5, d5
    vmlal.s16   q0, d6, d6
    vmlal.s16   q0, d7, d7
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_sse_pp_32x32_neon
    mov         r12, #8
    veor.u8     q0, q0
    veor.u8     q1, q1

.loop_sse_pp_32:
    subs        r12, #1
.rept 4
    vld1.64     {q8-q9}, [r0], r1
    vld1.64     {q10-q11}, [r2], r3
    vsubl.u8    q2, d16, d20
    vsubl.u8    q3, d17, d21
    vsubl.u8    q12, d18, d22
    vsubl.u8    q13, d19, d23
    vmlal.s16   q0, d4, d4
    vmlal.s16   q1, d5, d5
    vmlal.s16   q0, d6, d6
    vmlal.s16   q1, d7, d7
    vmlal.s16   q0, d24, d24
    vmlal.s16   q1, d25, d25
    vmlal.s16   q0, d26, d26
    vmlal.s16   q1, d27, d27
.endr
    bne         .loop_sse_pp_32
    vadd.s32    q0, q1
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_sse_pp_64x64_neon
    sub         r1, #32
    sub         r3, #32
    mov         r12, #16
    veor.u8     q0, q0
    veor.u8     q1, q1

.loop_sse_pp_64:
    subs        r12, #1
.rept 4
    vld1.64     {q8-q9}, [r0]!
    vld1.64     {q10-q11}, [r2]!
    vsubl.u8    q2, d16, d20
    vsubl.u8    q3, d17, d21
    vsubl.u8    q12, d18, d22
    vsubl.u8    q13, d19, d23
    vmlal.s16   q0, d4, d4
    vmlal.s16   q1, d5, d5
    vmlal.s16   q0, d6, d6
    vmlal.s16   q1, d7, d7
    vmlal.s16   q0, d24, d24
    vmlal.s16   q1, d25, d25
    vmlal.s16   q0, d26, d26
    vmlal.s16   q1, d27, d27

    vld1.64     {q8-q9}, [r0], r1
    vld1.64     {q10-q11}, [r2], r3
    vsubl.u8    q2, d16, d20
    vsubl.u8    q3, d17, d21
    vsubl.u8    q12, d18, d22
    vsubl.u8    q13, d19, d23
    vmlal.s16   q0, d4, d4
    vmlal.s16   q1, d5, d5
    vmlal.s16   q0, d6, d6
    vmlal.s16   q1, d7, d7
    vmlal.s16   q0, d24, d24
    vmlal.s16   q1, d25, d25
    vmlal.s16   q0, d26, d26
    vmlal.s16   q1, d27, d27
.endr
    bne         .loop_sse_pp_64
    vadd.s32    q0, q1
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_sse_ss_4x4_neon
    add         r1, r1
    add         r3, r3

    vld1.s16    {d16}, [r0], r1
    vld1.s16    {d18}, [r2], r3
    vsub.s16    q2, q8, q9
    vld1.s16    {d16}, [r0], r1
    vmull.s16   q0, d4, d4
    vld1.s16    {d18}, [r2], r3

    vsub.s16    q2, q8, q9
    vld1.s16    {d16}, [r0], r1
    vmlal.s16   q0, d4, d4
    vld1.s16    {d18}, [r2], r3

    vsub.s16    q2, q8, q9
    vld1.s16    {d16}, [r0], r1
    vmlal.s16   q0, d4, d4
    vld1.s16    {d18}, [r2], r3

    vsub.s16    q2, q8, q9
    vmlal.s16   q0, d4, d4

    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_sse_ss_8x8_neon
    add         r1, r1
    add         r3, r3

    vld1.s16    {q8}, [r0], r1
    vld1.s16    {q9}, [r2], r3
    vsub.s16    q8, q9
    vmull.s16   q0, d16, d16
    vmull.s16   q1, d17, d17

.rept 7
    vld1.s16    {q8}, [r0], r1
    vld1.s16    {q9}, [r2], r3
    vsub.s16    q8, q9
    vmlal.s16   q0, d16, d16
    vmlal.s16   q1, d17, d17
.endr
    vadd.s32    q0, q1
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_sse_ss_16x16_neon
    add         r1, r1
    add         r3, r3

    mov         r12, #4
    veor.u8     q0, q0
    veor.u8     q1, q1

.loop_sse_ss_16:
    subs        r12, #1
.rept 4
    vld1.s16    {q8-q9}, [r0], r1
    vld1.s16    {q10-q11}, [r2], r3
    vsub.s16    q8, q10
    vsub.s16    q9, q11
    vmlal.s16   q0, d16, d16
    vmlal.s16   q1, d17, d17
    vmlal.s16   q0, d18, d18
    vmlal.s16   q1, d19, d19
.endr
    bne         .loop_sse_ss_16
    vadd.s32    q0, q1
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_sse_ss_32x32_neon
    add         r1, r1
    add         r3, r3
    sub         r1, #32
    sub         r3, #32
    mov         r12, #8
    veor.u8     q0, q0
    veor.u8     q1, q1

.loop_sse_ss_32:
    subs        r12, #1
.rept 4
    vld1.s16    {q8-q9}, [r0]!
    vld1.s16    {q10-q11}, [r2]!
    vsub.s16    q8, q10
    vsub.s16    q9, q11
    vmlal.s16   q0, d16, d16
    vmlal.s16   q1, d17, d17
    vmlal.s16   q0, d18, d18
    vmlal.s16   q1, d19, d19

    vld1.s16    {q8-q9}, [r0], r1
    vld1.s16    {q10-q11}, [r2], r3
    vsub.s16    q8, q10
    vsub.s16    q9, q11
    vmlal.s16   q0, d16, d16
    vmlal.s16   q1, d17, d17
    vmlal.s16   q0, d18, d18
    vmlal.s16   q1, d19, d19
.endr
    bne         .loop_sse_ss_32
    vadd.s32    q0, q1
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_sse_ss_64x64_neon
    add         r1, r1
    add         r3, r3
    sub         r1, #96
    sub         r3, #96
    mov         r12, #32
    veor.u8     q0, q0
    veor.u8     q1, q1

.loop_sse_ss_64:
    subs        r12, #1
.rept 2
    vld1.s16    {q8-q9}, [r0]!
    vld1.s16    {q10-q11}, [r2]!
    vsub.s16    q8, q10
    vsub.s16    q9, q11
    vmlal.s16   q0, d16, d16
    vmlal.s16   q1, d17, d17
    vmlal.s16   q0, d18, d18
    vmlal.s16   q1, d19, d19

    vld1.s16    {q8-q9}, [r0]!
    vld1.s16    {q10-q11}, [r2]!
    vsub.s16    q8, q10
    vsub.s16    q9, q11
    vmlal.s16   q0, d16, d16
    vmlal.s16   q1, d17, d17
    vmlal.s16   q0, d18, d18
    vmlal.s16   q1, d19, d19

    vld1.s16    {q8-q9}, [r0]!
    vld1.s16    {q10-q11}, [r2]!
    vsub.s16    q8, q10
    vsub.s16    q9, q11
    vmlal.s16   q0, d16, d16
    vmlal.s16   q1, d17, d17
    vmlal.s16   q0, d18, d18
    vmlal.s16   q1, d19, d19

    vld1.s16    {q8-q9}, [r0], r1
    vld1.s16    {q10-q11}, [r2], r3
    vsub.s16    q8, q10
    vsub.s16    q9, q11
    vmlal.s16   q0, d16, d16
    vmlal.s16   q1, d17, d17
    vmlal.s16   q0, d18, d18
    vmlal.s16   q1, d19, d19
.endr
    bne         .loop_sse_ss_64
    vadd.s32    q0, q1
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_ssd_s_4x4_neon
    add         r1, r1
    vld1.s16    {d4}, [r0], r1
    vld1.s16    {d5}, [r0], r1
    vld1.s16    {d6}, [r0], r1
    vld1.s16    {d7}, [r0]
    vmull.s16   q0, d4, d4
    vmull.s16   q1, d5, d5
    vmlal.s16   q0, d6, d6
    vmlal.s16   q1, d7, d7
    vadd.s32    q0, q1
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_ssd_s_8x8_neon
    add         r1, r1
    vld1.s16    {q8}, [r0], r1
    vld1.s16    {q9}, [r0], r1
    vmull.s16   q0, d16, d16
    vmull.s16   q1, d17, d17
    vmlal.s16   q0, d18, d18
    vmlal.s16   q1, d19, d19
.rept 3
    vld1.s16    {q8}, [r0], r1
    vld1.s16    {q9}, [r0], r1
    vmlal.s16   q0, d16, d16
    vmlal.s16   q1, d17, d17
    vmlal.s16   q0, d18, d18
    vmlal.s16   q1, d19, d19
.endr
    vadd.s32    q0, q1
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_ssd_s_16x16_neon
    add         r1, r1
    mov         r12, #4
    veor.u8     q0, q0
    veor.u8     q1, q1

.loop_ssd_s_16:
    subs        r12, #1
.rept 2
    vld1.s16    {q8-q9}, [r0], r1
    vld1.s16    {q10-q11}, [r0], r1
    vmlal.s16   q0, d16, d16
    vmlal.s16   q1, d17, d17
    vmlal.s16   q0, d18, d18
    vmlal.s16   q1, d19, d19
    vmlal.s16   q0, d20, d20
    vmlal.s16   q1, d21, d21
    vmlal.s16   q0, d22, d22
    vmlal.s16   q1, d23, d23
.endr
    bne         .loop_ssd_s_16
    vadd.s32    q0, q1
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc

function x265_pixel_ssd_s_32x32_neon
    add         r1, r1
    sub         r1, #32
    mov         r12, #8
    veor.u8     q0, q0
    veor.u8     q1, q1

.loop_ssd_s_32:
    subs        r12, #1
.rept 4
    vld1.s16    {q8-q9}, [r0]!
    vld1.s16    {q10-q11}, [r0], r1
    vmlal.s16   q0, d16, d16
    vmlal.s16   q1, d17, d17
    vmlal.s16   q0, d18, d18
    vmlal.s16   q1, d19, d19
    vmlal.s16   q0, d20, d20
    vmlal.s16   q1, d21, d21
    vmlal.s16   q0, d22, d22
    vmlal.s16   q1, d23, d23
.endr
    bne         .loop_ssd_s_32
    vadd.s32    q0, q1
    vadd.s32    d0, d0, d1
    vpadd.s32   d0, d0, d0
    vmov.32     r0, d0[0]
    bx          lr
endfunc
