/*****************************************************************************
 * Copyright (C) 2016 x265 project
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
    push            {r4}
.rept \h
    ldr             r4, [r2], r3
    str             r4, [r0], r1
.endr
    pop             {r4}
    bx              lr
endfunc
.endm

blockcopy_pp_4xN_neon 4
blockcopy_pp_4xN_neon 8
blockcopy_pp_4xN_neon 16

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

.macro blockcopy_pp_16xN1_neon h i
function x265_blockcopy_pp_16x\h\()_neon
    push            {r4}
    mov             r4, #\i
loop_16x\h\():
.rept 8
    vld1.8          {q0}, [r2], r3
    vst1.8          {q0}, [r0], r1
.endr
    subs            r4, r4, #1
    bne             loop_16x\h
    pop             {r4}
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

function x265_blockcopy_pp_12x16_neon
    push            {r4, r5}
    mov             r5, #8
    sub             r3, r5
    sub             r1, r5
.rept 16
    vld1.8          {d0}, [r2]!
    ldr             r4, [r2], r3
    vst1.8          {d0}, [r0]!
    str             r4, [r0], r1
.endr
    pop            {r4, r5}
    bx              lr
endfunc

function x265_blockcopy_pp_24x32_neon
    push            {r4}
    mov             r4, #4
loop_24x32:
.rept 8
    vld1.8          {d0, d1, d2}, [r2], r3
    vst1.8          {d0, d1, d2}, [r0], r1
.endr
    subs            r4, r4, #1
    bne             loop_24x32
    pop             {r4}
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
    push            {r4}
    mov             r4, #\i
loop_32x\h\():
.rept 8
    vld1.8          {q0, q1}, [r2], r3
    vst1.8          {q0, q1}, [r0], r1
.endr
    subs            r4, r4, #1
    bne             loop_32x\h
    pop             {r4}
    bx              lr
endfunc
.endm

blockcopy_pp_32xN_neon 16 2
blockcopy_pp_32xN_neon 24 3
blockcopy_pp_32xN_neon 32 4
blockcopy_pp_32xN_neon 64 8

function x265_blockcopy_pp_48x64_neon
    push            {r4, r5}
    mov             r4, #8
    mov             r5, #32
    sub             r3, r5
    sub             r1, r5
loop_48x64:
.rept 8
    vld1.8          {q0, q1}, [r2]!
    vld1.8          {q2}, [r2], r3
    vst1.8          {q0, q1}, [r0]!
    vst1.8          {q2}, [r0], r1
.endr
    subs            r4, r4, #1
    bne             loop_48x64
    pop             {r4, r5}
    bx              lr
endfunc

.macro blockcopy_pp_64xN_neon h i
function x265_blockcopy_pp_64x\h\()_neon
    push            {r4, r5}
    mov             r4, #\i
    mov             r5, #32
    sub             r3, r5
    sub             r1, r5
loop_64x\h\():
.rept 4
    vld1.8          {q0, q1}, [r2]!
    vld1.8          {q2, q3}, [r2], r3
    vst1.8          {q0, q1}, [r0]!
    vst1.8          {q2, q3}, [r0], r1
.endr
    subs            r4, r4, #1
    bne             loop_64x\h
    pop             {r4, r5}
    bx              lr
endfunc
.endm

blockcopy_pp_64xN_neon 16 4
blockcopy_pp_64xN_neon 32 8
blockcopy_pp_64xN_neon 48 12
blockcopy_pp_64xN_neon 64 16

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
