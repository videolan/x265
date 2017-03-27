/*****************************************************************************
 * Copyright (C) 2013-2017 MulticoreWare, Inc
 *
 * Authors: Dnyaneshwar Gorade <dnyaneshwar@multicorewareinc.com>
 *          Radhakrishnan <radhakrishnan@multicorewareinc.com>
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

/* blockcopy_pp_16x16(pixel* dst, intptr_t dstStride, const pixel* src, intptr_t srcStride)
 *
 * r0   - dst
 * r1   - dstStride
 * r2   - src
 * r3   - srcStride */
function x265_blockcopy_pp_16x16_neon
.rept 16
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
.endr
    bx              lr
endfunc

.macro blockcopy_pp_4xN_neon h
function x265_blockcopy_pp_4x\h\()_neon
.rept \h
    ldr             r12, [r2], r3
    str             r12, [r0], r1
.endr
    bx              lr
endfunc
.endm

blockcopy_pp_4xN_neon 4
blockcopy_pp_4xN_neon 8
blockcopy_pp_4xN_neon 16
blockcopy_pp_4xN_neon 2
blockcopy_pp_4xN_neon 32

.macro blockcopy_pp_16xN_neon h
function x265_blockcopy_pp_16x\h\()_neon
.rept \h    
    vld1.8          {q0}, [r2], r3
    vst1.8          {q0}, [r0], r1
.endr
    bx              lr
endfunc
.endm

blockcopy_pp_16xN_neon 4
blockcopy_pp_16xN_neon 8
blockcopy_pp_16xN_neon 12
blockcopy_pp_16xN_neon 24

.macro blockcopy_pp_16xN1_neon h i
function x265_blockcopy_pp_16x\h\()_neon
    mov             r12, #\i
loop_16x\h\():
.rept 8
    vld1.8          {q0}, [r2], r3
    vst1.8          {q0}, [r0], r1
.endr
    subs            r12, r12, #1
    bne             loop_16x\h
    bx              lr
endfunc
.endm

blockcopy_pp_16xN1_neon 32 4
blockcopy_pp_16xN1_neon 64 8

.macro blockcopy_pp_8xN_neon h
function x265_blockcopy_pp_8x\h\()_neon
.rept \h    
    vld1.8          {d0}, [r2], r3
    vst1.8          {d0}, [r0], r1
.endr
    bx              lr
endfunc
.endm

blockcopy_pp_8xN_neon 4
blockcopy_pp_8xN_neon 8
blockcopy_pp_8xN_neon 16
blockcopy_pp_8xN_neon 32
blockcopy_pp_8xN_neon 2
blockcopy_pp_8xN_neon 6
blockcopy_pp_8xN_neon 12

function x265_blockcopy_pp_12x16_neon
    sub             r3, #8
    sub             r1, #8
.rept 16
    vld1.8          {d0}, [r2]!
    ldr             r12, [r2], r3
    vst1.8          {d0}, [r0]!
    str             r12, [r0], r1
.endr
    bx              lr
endfunc

function x265_blockcopy_pp_24x32_neon
    mov             r12, #4
loop_24x32:
.rept 8
    vld1.8          {d0, d1, d2}, [r2], r3
    vst1.8          {d0, d1, d2}, [r0], r1
.endr
    subs            r12, r12, #1
    bne             loop_24x32
    bx              lr
endfunc

function x265_blockcopy_pp_32x8_neon
.rept 8
    vld1.8          {q0, q1}, [r2], r3
    vst1.8          {q0, q1}, [r0], r1
.endr 
    bx              lr
endfunc

.macro blockcopy_pp_32xN_neon h i
function x265_blockcopy_pp_32x\h\()_neon
    mov             r12, #\i
loop_32x\h\():
.rept 8
    vld1.8          {q0, q1}, [r2], r3
    vst1.8          {q0, q1}, [r0], r1
.endr
    subs            r12, r12, #1
    bne             loop_32x\h
    bx              lr
endfunc
.endm

blockcopy_pp_32xN_neon 16 2
blockcopy_pp_32xN_neon 24 3
blockcopy_pp_32xN_neon 32 4
blockcopy_pp_32xN_neon 64 8
blockcopy_pp_32xN_neon 48 6

function x265_blockcopy_pp_48x64_neon
    mov             r12, #8
    sub             r3, #32
    sub             r1, #32
loop_48x64:
.rept 8
    vld1.8          {q0, q1}, [r2]!
    vld1.8          {q2}, [r2], r3
    vst1.8          {q0, q1}, [r0]!
    vst1.8          {q2}, [r0], r1
.endr
    subs            r12, r12, #1
    bne             loop_48x64
    bx              lr
endfunc

.macro blockcopy_pp_64xN_neon h i
function x265_blockcopy_pp_64x\h\()_neon
    mov             r12, #\i
    sub             r3, #32
    sub             r1, #32
loop_64x\h\():
.rept 4
    vld1.8          {q0, q1}, [r2]!
    vld1.8          {q2, q3}, [r2], r3
    vst1.8          {q0, q1}, [r0]!
    vst1.8          {q2, q3}, [r0], r1
.endr
    subs            r12, r12, #1
    bne             loop_64x\h
    bx              lr
endfunc
.endm

blockcopy_pp_64xN_neon 16 4
blockcopy_pp_64xN_neon 32 8
blockcopy_pp_64xN_neon 48 12
blockcopy_pp_64xN_neon 64 16

.macro blockcopy_pp_2xN_neon h
function x265_blockcopy_pp_2x\h\()_neon
.rept \h
    ldrh            r12, [r2], r3
    strh            r12, [r0], r1
.endr
    bx              lr
endfunc
.endm

blockcopy_pp_2xN_neon 4
blockcopy_pp_2xN_neon 8
blockcopy_pp_2xN_neon 16

.macro blockcopy_pp_6xN_neon h i
function x265_blockcopy_pp_6x\h\()_neon
    sub             r1, #4
.rept \i
    vld1.8          {d0}, [r2], r3
    vld1.8          {d1}, [r2], r3
    vst1.32         {d0[0]}, [r0]!
    vst1.16         {d0[2]}, [r0], r1
    vst1.32         {d1[0]}, [r0]!
    vst1.16         {d1[2]}, [r0], r1
.endr
    bx              lr
endfunc
.endm
blockcopy_pp_6xN_neon 8 4
blockcopy_pp_6xN_neon 16 8

function x265_blockcopy_pp_8x64_neon
    mov             r12, #4
loop_pp_8x64:
    subs            r12, #1
.rept 16
    vld1.8          {d0}, [r2], r3
    vst1.8          {d0}, [r0], r1
.endr
    bne             loop_pp_8x64
    bx              lr
endfunc

function x265_blockcopy_pp_12x32_neon
    push            {r4}
    sub             r3, #8
    sub             r1, #8
    mov             r12, #4
loop_pp_12x32:
    subs            r12, #1
.rept 8
    vld1.8          {d0}, [r2]!
    ldr             r4, [r2], r3
    vst1.8          {d0}, [r0]!
    str             r4, [r0], r1
.endr
    bne             loop_pp_12x32
    pop            {r4}
    bx              lr
endfunc

function x265_blockcopy_pp_24x64_neon
    mov             r12, #4
loop_24x64:
.rept 16
    vld1.8          {d0, d1, d2}, [r2], r3
    vst1.8          {d0, d1, d2}, [r0], r1
.endr
    subs            r12, r12, #1
    bne             loop_24x64
    bx              lr
endfunc

// void pixelavg_pp(pixel* dst, intptr_t dstride, const pixel* src0, intptr_t sstride0, const pixel* src1, intptr_t sstride1, int)
.macro pixel_avg_pp_4xN_neon h
function x265_pixel_avg_pp_4x\h\()_neon
    push            {r4}
    ldr             r4, [sp, #4]
    ldr             r12, [sp, #8]
.rept \h
    vld1.32         {d0[]}, [r2], r3
    vld1.32         {d1[]}, [r4], r12
    vrhadd.u8       d2, d0, d1
    vst1.32         {d2[0]}, [r0], r1
.endr
    pop             {r4}
    bx              lr
endfunc
.endm

pixel_avg_pp_4xN_neon 4
pixel_avg_pp_4xN_neon 8
pixel_avg_pp_4xN_neon 16

.macro pixel_avg_pp_8xN_neon h
function x265_pixel_avg_pp_8x\h\()_neon
    push            {r4}
    ldr             r4, [sp, #4]
    ldr             r12, [sp, #8]
.rept \h
    vld1.8          {d0}, [r2], r3
    vld1.8          {d1}, [r4], r12
    vrhadd.u8       d2, d0, d1
    vst1.8          {d2}, [r0], r1
.endr    
    pop             {r4}
    bx              lr
endfunc
.endm

pixel_avg_pp_8xN_neon 4
pixel_avg_pp_8xN_neon 8
pixel_avg_pp_8xN_neon 16
pixel_avg_pp_8xN_neon 32

function x265_pixel_avg_pp_12x16_neon
    push            {r4, r6}
    mov             r6, #8
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    sub             r1, r6
    sub             r3, r6
    sub             r12, r6
.rept 16
    vld1.32         {d0}, [r2]!
    vld1.32         {d1[0]}, [r2], r3
    vld1.32         {d2}, [r4]!
    vld1.32         {d3[0]}, [r4], r12
    vrhadd.u8       d0, d0, d2
    vrhadd.u8       d1, d1, d3
    vst1.8          {d0}, [r0]!
    vst1.32         {d1[0]}, [r0], r1
.endr
    pop            {r4, r6}
    bx              lr
endfunc

.macro pixel_avg_pp_16xN_neon h
function x265_pixel_avg_pp_16x\h\()_neon
    push            {r4}
    ldr             r4, [sp, #4]
    ldr             r12, [sp, #8]
.rept \h
    vld1.8          {q0}, [r2], r3
    vld1.8          {q1}, [r4], r12
    vrhadd.u8       q2, q0, q1
    vst1.8          {q2}, [r0], r1
.endr    
    pop             {r4}
    bx              lr
endfunc
.endm

pixel_avg_pp_16xN_neon 4
pixel_avg_pp_16xN_neon 8
pixel_avg_pp_16xN_neon 12
pixel_avg_pp_16xN_neon 16
pixel_avg_pp_16xN_neon 32

function x265_pixel_avg_pp_16x64_neon
    push            {r4, r6}
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    mov             r6, #8
lpavg_16x64:
.rept 8
    vld1.8          {q0}, [r2], r3
    vld1.8          {q1}, [r4], r12
    vrhadd.u8       q2, q0, q1
    vst1.8          {q2}, [r0], r1
.endr  
    subs            r6, r6, #1
    bne             lpavg_16x64
    pop             {r4 , r6}
    bx              lr
endfunc

function x265_pixel_avg_pp_24x32_neon
    push            {r4, r6}
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    mov             r6, #4
lpavg_24x32:
.rept 8
    vld1.8          {d0, d1, d2}, [r2], r3
    vld1.8          {d3, d4, d5}, [r4], r12
    vrhadd.u8       d0, d0, d3
    vrhadd.u8       d1, d1, d4
    vrhadd.u8       d2, d2, d5
    vst1.8          {d0, d1, d2}, [r0], r1
.endr
    subs            r6, r6, #1
    bne             lpavg_24x32
    pop             {r4, r6}
    bx              lr
endfunc

.macro pixel_avg_pp_32xN_neon h
function x265_pixel_avg_pp_32x\h\()_neon
    push            {r4}
    ldr             r4, [sp, #4]
    ldr             r12, [sp, #8]
.rept \h
    vld1.8          {q0, q1}, [r2], r3
    vld1.8          {q2, q3}, [r4], r12
    vrhadd.u8       q0, q0, q2
    vrhadd.u8       q1, q1, q3
    vst1.8          {q0, q1}, [r0], r1
.endr    
    pop             {r4}
    bx              lr
endfunc
.endm

pixel_avg_pp_32xN_neon 8
pixel_avg_pp_32xN_neon 16
pixel_avg_pp_32xN_neon 24

.macro pixel_avg_pp_32xN1_neon h i
function x265_pixel_avg_pp_32x\h\()_neon
    push            {r4, r6}
    ldr             r4, [sp, #8]
    ldr             r12, [sp, #12]
    mov             r6, #\i
lpavg_32x\h\():
.rept 8
    vld1.8          {q0, q1}, [r2], r3
    vld1.8          {q2, q3}, [r4], r12
    vrhadd.u8       q0, q0, q2
    vrhadd.u8       q1, q1, q3
    vst1.8          {q0, q1}, [r0], r1
.endr  
    subs            r6, r6, #1
    bne             lpavg_32x\h
    pop             {r4, r6}
    bx              lr
endfunc
.endm

pixel_avg_pp_32xN1_neon 32 4 
pixel_avg_pp_32xN1_neon 64 8

function x265_pixel_avg_pp_48x64_neon
    push            {r4, r6, r7}
    ldr             r4, [sp, #12]
    ldr             r12, [sp, #16]
    mov             r6, #8
    mov             r7, #32
    sub             r1, r7
    sub             r3, r7
    sub             r12, r7
lpavg_48x64:
.rept 8
    vld1.8          {q0, q1}, [r2]!
    vld1.8          {q2}, [r2], r3
    vld1.8          {q8, q9}, [r4]!
    vld1.8          {q10}, [r4], r12
    vrhadd.u8       q0, q0, q8
    vrhadd.u8       q1, q1, q9
    vrhadd.u8       q2, q2, q10
    vst1.8          {q0, q1}, [r0]!
    vst1.8          {q2}, [r0], r1
.endr
    subs            r6, r6, #1
    bne             lpavg_48x64
    pop             {r4, r6, r7}
    bx              lr
endfunc

.macro pixel_avg_pp_64xN_neon h i
function x265_pixel_avg_pp_64x\h\()_neon
    push            {r4, r6, r7}
    ldr             r4, [sp, #12]
    ldr             r12, [sp, #16]
    mov             r7, #32
    mov             r6, #\i
    sub             r3, r7
    sub             r12, r7
    sub             r1, r7
lpavg_64x\h\():
.rept 4
    vld1.8          {q0, q1}, [r2]!
    vld1.8          {q2, q3}, [r2], r3
    vld1.8          {q8, q9}, [r4]!
    vld1.8          {q10, q11}, [r4], r12
    vrhadd.u8       q0, q0, q8
    vrhadd.u8       q1, q1, q9
    vrhadd.u8       q2, q2, q10
    vrhadd.u8       q3, q3, q11
    vst1.8          {q0, q1}, [r0]!
    vst1.8          {q2, q3}, [r0], r1
.endr
    subs            r6, r6, #1
    bne             lpavg_64x\h
    pop             {r4, r6, r7}
    bx              lr
endfunc
.endm

pixel_avg_pp_64xN_neon 16 4
pixel_avg_pp_64xN_neon 32 8
pixel_avg_pp_64xN_neon 48 12
pixel_avg_pp_64xN_neon 64 16

// void x265_cpy2Dto1D_shr_4x4_neon(int16_t* dst, const int16_t* src, intptr_t srcStride, int shift)
function x265_cpy2Dto1D_shr_4x4_neon
    add             r2, r2
    vdup.16         q0, r3
    vceq.s16        q1, q1
    vshl.s16        q1, q0
    vsri.s16        q1, #1
    vneg.s16        q0, q0
    vld1.s16        {d4}, [r1], r2
    vld1.s16        {d5}, [r1], r2
    vld1.s16        {d6}, [r1], r2
    vld1.s16        {d7}, [r1], r2
    vsub.s16        q2, q1
    vsub.s16        q3, q1
    vshl.s16        q2, q0
    vshl.s16        q3, q0
    vst1.16         {q2-q3}, [r0]
    bx              lr
endfunc

function x265_cpy2Dto1D_shr_8x8_neon
    add             r2, r2
    vdup.16         q0, r3
    vceq.s16        q1, q1
    vshl.s16        q1, q0
    vsri.s16        q1, #1
    vneg.s16        q0, q0
.rept 4
    vld1.s16        {q2}, [r1], r2
    vld1.s16        {q3}, [r1], r2
    vsub.s16        q2, q1
    vsub.s16        q3, q1
    vshl.s16        q2, q0
    vshl.s16        q3, q0
    vst1.16         {q2-q3}, [r0]!
.endr
    bx              lr
endfunc

function x265_cpy2Dto1D_shr_16x16_neon
    add             r2, r2
    vdup.16         q0, r3
    vceq.s16        q1, q1
    vshl.s16        q1, q0
    vsri.s16        q1, #1
    vneg.s16        q0, q0
    mov             r3, #4
.loop_cpy2Dto1D_shr_16:
    subs            r3, #1
.rept 4
    vld1.s16        {q2-q3}, [r1], r2
    vsub.s16        q2, q1
    vsub.s16        q3, q1
    vshl.s16        q2, q0
    vshl.s16        q3, q0
    vst1.16         {q2-q3}, [r0]!
.endr
    bgt             .loop_cpy2Dto1D_shr_16
    bx              lr
endfunc

function x265_cpy2Dto1D_shr_32x32_neon
    add             r2, r2
    sub             r2, #32
    vdup.16         q0, r3
    vceq.s16        q1, q1
    vshl.s16        q1, q0
    vsri.s16        q1, #1
    vneg.s16        q0, q0
    mov             r3, 16
.loop_cpy2Dto1D_shr_32:
    subs            r3, #1
.rept 2
    vld1.s16        {q2-q3}, [r1]!
    vld1.s16        {q8-q9}, [r1], r2
    vsub.s16        q2, q1
    vsub.s16        q3, q1
    vsub.s16        q8, q1
    vsub.s16        q9, q1
    vshl.s16        q2, q0
    vshl.s16        q3, q0
    vshl.s16        q8, q0
    vshl.s16        q9, q0
    vst1.16         {q2-q3}, [r0]!
    vst1.16         {q8-q9}, [r0]!
.endr
    bgt             .loop_cpy2Dto1D_shr_32
    bx              lr
endfunc

// void addAvg(const int16_t* src0, const int16_t* src1, pixel* dst, intptr_t src0Stride, intptr_t src1Stride, intptr_t dstStride)
.macro addAvg_8xN h i
function x265_addAvg_8x\h\()_neon
    push            {r4, r5, r6}
    ldr             r4, [sp, #12]
    ldr             r5, [sp, #16]
    lsl             r3, #1
    lsl             r4, #1
    mov             r12, #\i
    vmov.i16        d0, #16448

loop_addavg_8x\h:
    subs            r12, #1
    vld1.16         {q1}, [r0], r3        // src1
    vld1.16         {q2}, [r1], r4        // src2
    vld1.16         {q10}, [r0], r3        // src1
    vld1.16         {q11}, [r1], r4        // src2

    vadd.s16        q1, q2
    vaddl.s16       q8, d2, d0
    vaddl.s16       q9, d3, d0
    vadd.s16        q10, q11
    vaddl.s16       q1, d20, d0
    vaddl.s16       q2, d21, d0

    vshrn.s32       d20, q8, #7
    vshrn.s32       d21, q9, #7
    vshrn.s32       d22, q1, #7
    vshrn.s32       d23, q2, #7

    vqmovun.s16     d2, q10
    vqmovun.s16     d3, q11
    vst1.8          {d2}, [r2], r5
    vst1.8          {d3}, [r2], r5

    bne             loop_addavg_8x\h
    pop             {r4, r5, r6}
    bx              lr
endfunc
.endm

addAvg_8xN 4 2
addAvg_8xN 8 4
addAvg_8xN 16 8
addAvg_8xN 32 16
addAvg_8xN 2 1
addAvg_8xN 6 3
addAvg_8xN 12 6
addAvg_8xN 64 32

function x265_addAvg_4x4_neon
    push            {r4, r5, r6}
    ldr             r4, [sp, #12]
    ldr             r5, [sp, #16]
    lsl             r3, #1
    lsl             r4, #1
    vmov.i16        d0, #16448

.rept 2
    vld1.16         {d2}, [r0], r3        // src1
    vld1.16         {d4}, [r0], r3
    vld1.16         {d3}, [r1], r4        // src2
    vld1.16         {d5}, [r1], r4

    vadd.s16        d2, d3
    vadd.s16        d4, d5
    vaddl.s16       q8, d2, d0
    vaddl.s16       q9, d4, d0
    vshrn.s32       d20, q8, #7
    vshrn.s32       d21, q9, #7
    vqmovun.s16     d2, q10

    vst1.32         {d2[0]}, [r2], r5
    vst1.32         {d2[1]}, [r2], r5
.endr
    pop             {r4, r5, r6}
    bx              lr
endfunc

.macro addAvg_4xN h i
function x265_addAvg_4x\h\()_neon
    push            {r4, r5, r6}
    ldr             r4, [sp, #12]
    ldr             r5, [sp, #16]
    lsl             r3, #1
    lsl             r4, #1
    mov             r12, #\i
    vmov.i16        d0, #16448

loop_addavg_4x\h\():
    subs            r12, #1
    vld1.16         {d2}, [r0], r3        // src1
    vld1.16         {d4}, [r0], r3
    vld1.16         {d3}, [r1], r4        // src2
    vld1.16         {d5}, [r1], r4

    vadd.s16        d2, d3
    vadd.s16        d4, d5
    vaddl.s16       q8, d2, d0
    vaddl.s16       q9, d4, d0
    vshrn.s32       d20, q8, #7
    vshrn.s32       d21, q9, #7
    vqmovun.s16     d2, q10

    vst1.32         {d2[0]}, [r2], r5
    vst1.32         {d2[1]}, [r2], r5
    bne             loop_addavg_4x\h
    pop             {r4, r5, r6}
    bx              lr
endfunc
.endm

addAvg_4xN 8 4
addAvg_4xN 16 8
addAvg_4xN 2 1
addAvg_4xN 32 16

.macro addAvg_6xN h i
function x265_addAvg_6x\h\()_neon
    push            {r4, r5, r6}
    ldr             r4, [sp, #12]
    ldr             r5, [sp, #16]
    lsl             r3, #1
    lsl             r4, #1
    sub             r5, #4
    mov             r12, #\i
    vmov.i16        d0, #16448

loop_addavg_6x\h:
    subs            r12, #1
    vld1.16         {q1}, [r0], r3        // src1
    vld1.16         {q2}, [r1], r4        // src2
    vld1.16         {q10}, [r0], r3        // src1
    vld1.16         {q11}, [r1], r4        // src2

    vadd.s16        q1, q2
    vaddl.s16       q8, d2, d0
    vaddl.s16       q9, d3, d0
    vadd.s16        q10, q11
    vaddl.s16       q1, d20, d0
    vaddl.s16       q2, d21, d0

    vshrn.s32       d20, q8, #7
    vshrn.s32       d21, q9, #7
    vshrn.s32       d22, q1, #7
    vshrn.s32       d23, q2, #7

    vqmovun.s16     d2, q10
    vqmovun.s16     d3, q11
    vst1.32         {d2[0]}, [r2]!
    vst1.16         {d2[2]}, [r2], r5
    vst1.32         {d3[0]}, [r2]!
    vst1.16         {d3[2]}, [r2], r5

    bne             loop_addavg_6x\h
    pop             {r4, r5, r6}
    bx              lr
endfunc
.endm

addAvg_6xN 8 4
addAvg_6xN 16 8

function x265_addAvg_12x16_neon
    push            {r4, r5, r6}
    ldr             r4, [sp, #12]
    ldr             r5, [sp, #16]
    lsl             r3, #1
    lsl             r4, #1
    sub             r5, #8
    mov             r12, #16
    vmov.i16        d0, #16448

loop_addAvg_12X16:
    subs            r12, #1
    vld1.16         {d2, d3, d4}, [r0], r3
    vld1.16         {d16, d17, d18}, [r1], r4

    vadd.s16        q1, q8
    vaddl.s16       q11, d2, d0
    vaddl.s16       q10, d3, d0
    vadd.s16        d4, d18
    vaddl.s16       q9, d0, d4

    vshrn.s32       d2, q11, #7
    vshrn.s32       d3, q10, #7
    vshrn.s32       d4, q9, #7
    veor            d5, d5

    vqmovun.s16     d6, q1
    vqmovun.s16     d7, q2
    vst1.8          {d6}, [r2]!
    vst1.32         {d7[0]}, [r2], r5

    bne             loop_addAvg_12X16
    pop             {r4, r5, r6}
    bx              lr
endfunc

function x265_addAvg_12x32_neon
    push            {r4, r5, r6}
    ldr             r4, [sp, #12]
    ldr             r5, [sp, #16]
    lsl             r3, #1
    lsl             r4, #1
    sub             r5, #8
    mov             r12, #32
    vmov.i16        d0, #16448

loop_addAvg_12X32:
    subs            r12, #1
    vld1.16         {d2, d3, d4}, [r0], r3
    vld1.16         {d16, d17, d18}, [r1], r4

    vadd.s16        q1, q8
    vaddl.s16       q11, d2, d0
    vaddl.s16       q10, d3, d0
    vadd.s16        d4, d18
    vaddl.s16       q9, d0, d4

    vshrn.s32       d2, q11, #7
    vshrn.s32       d3, q10, #7
    vshrn.s32       d4, q9, #7
    veor            d5, d5

    vqmovun.s16     d6, q1
    vqmovun.s16     d7, q2
    vst1.8          {d6}, [r2]!
    vst1.32         {d7[0]}, [r2], r5

    bne             loop_addAvg_12X32
    pop             {r4, r5, r6}
    bx              lr
endfunc

.macro addAvg_16xN h
function x265_addAvg_16x\h\()_neon
    push            {r4, r5, r6}
    ldr             r4, [sp, #12]
    ldr             r5, [sp, #16]
    lsl             r3, #1
    lsl             r4, #1
    mov             r12, #\h
    vmov.i16        d0, #16448

loop_addavg_16x\h:
    subs            r12, #1
    vld1.16         {q1, q2}, [r0], r3             // src1
    vld1.16         {q8, q9}, [r1], r4             // src2

    vadd.s16        q1, q8
    vaddl.s16       q10, d2, d0
    vaddl.s16       q11, d3, d0
    vadd.s16        q2, q9
    vaddl.s16       q8, d4, d0
    vaddl.s16       q9, d5, d0

    vshrn.s32       d2, q10, #7
    vshrn.s32       d3, q11, #7
    vshrn.s32       d4, q8, #7
    vshrn.s32       d5, q9, #7

    vqmovun.s16     d6, q1
    vqmovun.s16     d7, q2
    vst1.8          {q3}, [r2], r5

    bne             loop_addavg_16x\h
    pop             {r4, r5, r6}
    bx              lr
endfunc
.endm

addAvg_16xN 4
addAvg_16xN 8
addAvg_16xN 12
addAvg_16xN 16
addAvg_16xN 32
addAvg_16xN 64
addAvg_16xN 24

function x265_addAvg_24x32_neon
    push            {r4, r5, r6}
    ldr             r4, [sp, #12]
    ldr             r5, [sp, #16]
    lsl             r3, #1
    lsl             r4, #1
    sub             r3, #32
    sub             r4, #32
    mov             r12, #32
    vmov.i16        d0, #16448

loop_addavg_24x32:
    subs            r12, #1
    vld1.16         {q1, q2}, [r0]!             // src1
    vld1.16         {q3}, [r0], r3
    vld1.16         {q8, q9}, [r1]!             // src2
    vld1.16         {q10}, [r1], r4

    vadd.s16        q1, q8
    vaddl.s16       q12, d2, d0
    vaddl.s16       q13, d3, d0
    vadd.s16        q2, q9
    vaddl.s16       q8, d4, d0
    vaddl.s16       q9, d5, d0
    vadd.s16        q3, q10
    vaddl.s16       q10, d6, d0
    vaddl.s16       q11, d7, d0

    vshrn.s32       d2, q12, #7
    vshrn.s32       d3, q13, #7
    vshrn.s32       d4, q8, #7
    vshrn.s32       d5, q9, #7
    vshrn.s32       d6, q10, #7
    vshrn.s32       d7, q11, #7

    vqmovun.s16     d16, q1
    vqmovun.s16     d17, q2
    vqmovun.s16     d18, q3
    vst1.8          {d16, d17, d18}, [r2], r5
    bne             loop_addavg_24x32

    pop             {r4, r5, r6}
    bx              lr
endfunc

function x265_addAvg_24x64_neon
    push            {r4, r5, r6}
    ldr             r4, [sp, #12]
    ldr             r5, [sp, #16]
    lsl             r3, #1
    lsl             r4, #1
    sub             r3, #32
    sub             r4, #32
    mov             r12, #64
    vmov.i16        d0, #16448

loop_addavg_24x64:
    subs            r12, #1
    vld1.16         {q1, q2}, [r0]!             // src1
    vld1.16         {q3}, [r0], r3
    vld1.16         {q8, q9}, [r1]!             // src2
    vld1.16         {q10}, [r1], r4

    vadd.s16        q1, q8
    vaddl.s16       q12, d2, d0
    vaddl.s16       q13, d3, d0
    vadd.s16        q2, q9
    vaddl.s16       q8, d4, d0
    vaddl.s16       q9, d5, d0
    vadd.s16        q3, q10
    vaddl.s16       q10, d6, d0
    vaddl.s16       q11, d7, d0

    vshrn.s32       d2, q12, #7
    vshrn.s32       d3, q13, #7
    vshrn.s32       d4, q8, #7
    vshrn.s32       d5, q9, #7
    vshrn.s32       d6, q10, #7
    vshrn.s32       d7, q11, #7

    vqmovun.s16     d16, q1
    vqmovun.s16     d17, q2
    vqmovun.s16     d18, q3
    vst1.8          {d16, d17, d18}, [r2], r5
    bne             loop_addavg_24x64

    pop             {r4, r5, r6}
    bx              lr
endfunc

.macro addAvg32 x y z
    mov             r12, #\y
loop_addavg_\x\()x\y\()_\z:
    subs            r12, #1
    vld1.16         {q8, q9}, [r0]!               // src1
    vld1.16         {q10, q11}, [r0], r3
    vld1.16         {q12, q13}, [r1]!             // src2
    vld1.16         {q14, q15}, [r1], r4

    vadd.s16        q8, q12
    vaddl.s16       q1, d16, d0
    vaddl.s16       q2, d17, d0
    vadd.s16        q9, q13
    vaddl.s16       q12, d18, d0
    vaddl.s16       q13, d19, d0

    vshrn.s32       d6, q1, #7
    vshrn.s32       d7, q2, #7
    vshrn.s32       d2, q12, #7
    vshrn.s32       d3, q13, #7
    vqmovun.s16     d16, q3
    vqmovun.s16     d17, q1

    vadd.s16        q10, q14
    vaddl.s16       q1, d20, d0
    vaddl.s16       q2, d21, d0
    vadd.s16        q11, q15
    vaddl.s16       q12, d22, d0
    vaddl.s16       q13, d23, d0

    vshrn.s32       d6, q1, #7
    vshrn.s32       d7, q2, #7
    vshrn.s32       d2, q12, #7
    vshrn.s32       d3, q13, #7
    vqmovun.s16     d18, q3
    vqmovun.s16     d19, q1
    vst1.8          {q8, q9}, [r2], r5
    bne             loop_addavg_\x\()x\y\()_\z
.endm

.macro addAvg_32xN h
function x265_addAvg_32x\h\()_neon
    push            {r4, r5, r6}
    ldr             r4, [sp, #12]
    ldr             r5, [sp, #16]
    lsl             r3, #1
    lsl             r4, #1
    sub             r3, #32
    sub             r4, #32
    vmov.i16        d0, #16448

    addAvg32 32 \h 1
    pop             {r4, r5, r6}
    bx              lr
endfunc
.endm

addAvg_32xN 8
addAvg_32xN 16
addAvg_32xN 24
addAvg_32xN 32
addAvg_32xN 64
addAvg_32xN 48

function x265_addAvg_48x64_neon
    push            {r4, r5, r6, r7, r8}
    ldr             r4, [sp, #20]
    ldr             r5, [sp, #24]
    lsl             r3, #1
    lsl             r4, #1
    sub             r3, #32
    sub             r4, #32
    vmov.i16        d0, #16448
    mov             r7, r0
    mov             r8, r1

    addAvg32 48 64 1                               // 32x64
    add             r0, r7, #64
    add             r1, r8, #64
    sub             r2, r2, r5, lsl #6
    add             r2, #32
    add             r3, #32
    add             r4, #32

    mov             r12, #64
loop_addavg_16x64_2:                               // 16x64
    subs            r12, #1
    vld1.16         {q1, q2}, [r0], r3             // src1
    vld1.16         {q8, q9}, [r1], r4             // src2

    vadd.s16        q1, q8
    vaddl.s16       q10, d2, d0
    vaddl.s16       q11, d3, d0
    vadd.s16        q2, q9
    vaddl.s16       q8, d4, d0
    vaddl.s16       q9, d5, d0

    vshrn.s32       d2, q10, #7
    vshrn.s32       d3, q11, #7
    vshrn.s32       d4, q8, #7
    vshrn.s32       d5, q9, #7

    vqmovun.s16     d6, q1
    vqmovun.s16     d7, q2
    vst1.8          {q3}, [r2], r5
    bne             loop_addavg_16x64_2

    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc

function x265_addAvg_64x16_neon
    push            {r4, r5, r6, r7, r8}
    ldr             r4, [sp, #20]
    ldr             r5, [sp, #24]
    lsl             r3, #1
    lsl             r4, #1
    sub             r3, #32
    sub             r4, #32
    vmov.i16        d0, #16448
    mov             r7, r0
    mov             r8, r1

    addAvg32 64 16 1
    add             r0, r7, #64
    add             r1, r8, #64
    sub             r2, r2, r5, lsl #4
    add             r2, #32
    addAvg32 64 16 2

    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc

function x265_addAvg_64x32_neon
    push            {r4, r5, r6, r7, r8}
    ldr             r4, [sp, #20]
    ldr             r5, [sp, #24]
    lsl             r3, #1
    lsl             r4, #1
    sub             r3, #32
    sub             r4, #32
    vmov.i16        d0, #16448
    mov             r7, r0
    mov             r8, r1

    addAvg32 64 32 1
    add             r0, r7, #64
    add             r1, r8, #64
    sub             r2, r2, r5, lsl #5
    add             r2, #32
    addAvg32 64 32 2

    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc

function x265_addAvg_64x48_neon
    push            {r4, r5, r6, r7, r8}
    ldr             r4, [sp, #20]
    ldr             r5, [sp, #24]
    lsl             r3, #1
    lsl             r4, #1
    sub             r3, #32
    sub             r4, #32
    vmov.i16        d0, #16448
    mov             r7, r0
    mov             r8, r1

    addAvg32 64 48 1
    add             r0, r7, #64
    add             r1, r8, #64
    sub             r2, r2, r5, lsl #5
    sub             r2, r2, r5, lsl #4
    add             r2, #32
    addAvg32 64 48 2

    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc

function x265_addAvg_64x64_neon
    push            {r4, r5, r6, r7, r8}
    ldr             r4, [sp, #20]
    ldr             r5, [sp, #24]
    lsl             r3, #1
    lsl             r4, #1
    sub             r3, #32
    sub             r4, #32
    vmov.i16        d0, #16448
    mov             r7, r0
    mov             r8, r1

    addAvg32 64 64 1
    add             r0, r7, #64
    add             r1, r8, #64
    sub             r2, r2, r5, lsl #6
    add             r2, #32
    addAvg32 64 64 2

    pop             {r4, r5, r6, r7, r8}
    bx              lr
endfunc
