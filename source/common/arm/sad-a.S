/*****************************************************************************
 * Copyright (C) 2016 x265 project
 *
 * Authors: David Conrad <lessen42@gmail.com>
 *          Janne Grunau <janne-x264@jannau.net>
 *          Dnyaneshwar G <dnyaneshwar@multicorewareinc.com>
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
sad12_mask:
.byte 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 0

.text

/* sad4x4(pixel* dst, intptr_t dstStride, const pixel* src, intptr_t srcStride)
 *
 * r0   - dst
 * r1   - dstStride
 * r2   - src
 * r3   - srcStride */

.macro SAD4_ARMV6 h
function x265_pixel_sad_4x\h\()_armv6
    push        {r4-r6,lr}
    ldr         r4, [r2], r3
    ldr         r5, [r0], r1
    ldr         r6, [r2], r3
    ldr         lr, [r0], r1
    usad8       ip, r4, r5
.rept (\h - 2)/2
    ldr         r4, [r2], r3
    ldr         r5, [r0], r1
    usada8      ip, r6, lr, ip
    ldr         r6, [r2], r3
    ldr         lr, [r0], r1
    usada8      ip, r4, r5, ip
.endr
    usada8      r0, r6, lr, ip
    pop         {r4-r6,pc}
endfunc
.endm

SAD4_ARMV6 4
SAD4_ARMV6 8

// SAD_X3 and SAD_X4 code start

.macro SAD_X_START_4 x
    vld1.32         {d0[]}, [r0], r12
    vld1.32         {d1[]}, [r1], r4
    vld1.32         {d2[]}, [r2], r4
    vld1.32         {d3[]}, [r3], r4
.if \x == 4
    vld1.32         {d4[]}, [lr], r4
.endif
    vabdl.u8        q8, d0, d1
    vabdl.u8        q9, d0, d2
    vabdl.u8        q10, d0, d3
.if \x == 4
    vabdl.u8        q11, d0, d4
.endif
.endm

.macro SAD_X_4 x
    vld1.32         {d0[]}, [r0], r12
    vld1.32         {d1[]}, [r1], r4
    vld1.32         {d2[]}, [r2], r4
    vld1.32         {d3[]}, [r3], r4
.if \x == 4
    vld1.32         {d4[]}, [lr], r4
.endif
    vabal.u8        q8, d0, d1
    vabal.u8        q9, d0, d2
    vabal.u8        q10, d0, d3
.if \x == 4
    vabal.u8        q11, d0, d4
.endif
.endm

.macro SAD_X_4xN x, h
function x265_sad_x\x\()_4x\h\()_neon
    push            {r4, r5, lr}
.if \x == 3
    ldrd            r4, r5, [sp, #12]
.else
    ldr             lr, [sp, #12]
    ldrd            r4, r5, [sp, #16]
.endif
    mov             r12, #FENC_STRIDE

    SAD_X_START_4 \x
.rept \h - 1
    SAD_X_4 \x 
.endr
    vpadd.u16       d0, d16, d18
    vpadd.u16       d1, d20, d22
    vpaddl.u16      q0, q0
.if \x == 3
    vst1.32         {d0}, [r5]!
    vst1.32         {d1[0]}, [r5, :32]
.else
    vst1.32         {d0-d1}, [r5]
.endif
    pop             {r4, r5, lr}
    bx              lr
endfunc
.endm

SAD_X_4xN 3 4
SAD_X_4xN 3 8
SAD_X_4xN 3 16

SAD_X_4xN 4 4
SAD_X_4xN 4 8
SAD_X_4xN 4 16

.macro SAD_X_START_8 x
    vld1.8          {d0}, [r0], r12
    vld1.8          {d1}, [r1], r4
    vld1.8          {d2}, [r2], r4
    vld1.8          {d3}, [r3], r4
.if \x == 4
    vld1.8          {d4}, [lr], r4
.endif
    vabdl.u8        q8, d0, d1
    vabdl.u8        q9, d0, d2
    vabdl.u8        q10, d0, d3
.if \x == 4
    vabdl.u8        q11, d0, d4
.endif
.endm

.macro SAD_X_8 x
    vld1.8          {d0}, [r0], r12
    vld1.8          {d1}, [r1], r4
    vld1.8          {d2}, [r2], r4
    vld1.8          {d3}, [r3], r4
.if \x == 4
    vld1.8          {d4}, [lr], r4
.endif
    vabal.u8        q8, d0, d1
    vabal.u8        q9, d0, d2
    vabal.u8        q10, d0, d3
.if \x == 4
    vabal.u8        q11, d0, d4
.endif
.endm

.macro SAD_X_8xN x, h
function x265_sad_x\x\()_8x\h\()_neon
    push            {r4, r5, lr}
.if \x == 3
    ldrd            r4, r5, [sp, #12]
.else
    ldr             lr, [sp, #12]
    ldrd            r4, r5, [sp, #16]
.endif
    mov             r12, #FENC_STRIDE
    SAD_X_START_8 \x
.rept \h - 1
    SAD_X_8 \x
.endr
    vadd.u16        d16, d16, d17
    vadd.u16        d18, d18, d19
    vadd.u16        d20, d20, d21
    vadd.u16        d22, d22, d23

    vpadd.u16       d0, d16, d18
    vpadd.u16       d1, d20, d22
    vpaddl.u16      q0, q0
.if \x == 3
    vst1.32         {d0}, [r5]!
    vst1.32         {d1[0]}, [r5, :32]
.else
    vst1.32         {d0-d1}, [r5]
.endif
    pop             {r4, r5, lr}
    bx              lr
endfunc
.endm

SAD_X_8xN 3 4
SAD_X_8xN 3 8
SAD_X_8xN 3 16
SAD_X_8xN 3 32

SAD_X_8xN 4 4
SAD_X_8xN 4 8
SAD_X_8xN 4 16
SAD_X_8xN 4 32

.macro SAD_X_START_16 x
    vld1.8          {q0}, [r0], r12
    vld1.8          {q1}, [r1], r4
    vld1.8          {q2}, [r2], r4
    vld1.8          {q3}, [r3], r4
    vabdl.u8        q8, d0, d2
    vabdl.u8        q9, d1, d3
    vabdl.u8        q10, d0, d4
    vabdl.u8        q11, d1, d5
    vabdl.u8        q12, d0, d6
    vabdl.u8        q13, d1, d7
.if \x == 4
    vld1.8          {q3}, [lr], r4
    vabdl.u8        q14, d0, d6
    vabdl.u8        q15, d1, d7 
.endif
.endm

.macro SAD_X_16 x
    vld1.8          {q0}, [r0], r12
    vld1.8          {q1}, [r1], r4
    vld1.8          {q2}, [r2], r4
    vld1.8          {q3}, [r3], r4
    vabal.u8        q8, d0, d2
    vabal.u8        q9, d1, d3
    vabal.u8        q10, d0, d4
    vabal.u8        q11, d1, d5
    vabal.u8        q12, d0, d6
    vabal.u8        q13, d1, d7
.if \x == 4
    vld1.8          {q3}, [lr], r4
    vabal.u8        q14, d0, d6
    vabal.u8        q15, d1, d7
.endif
.endm

.macro SAD_X_16xN x, h
function x265_sad_x\x\()_16x\h\()_neon
    push            {r4, r5, lr}
.if \x == 3
    ldrd            r4, r5, [sp, #12]
.else
    ldr             lr, [sp, #12]
    ldrd            r4, r5, [sp, #16]
.endif
    mov             r12, #FENC_STRIDE
    SAD_X_START_16 \x
.rept \h - 1
    SAD_X_16 \x
.endr
    vadd.u16        q8, q8, q9
    vadd.u16        q10, q10, q11
    vadd.u16        q12, q12, q13
.if \x == 4
    vadd.u16        q14, q14, q15
.endif
    vadd.u16        d16, d16, d17
    vadd.u16        d20, d20, d21
    vadd.u16        d24, d24, d25
.if \x == 4
    vadd.u16        d28, d28, d29
.endif

.if \h <= 32
    vpadd.u16       d0, d16, d20
    vpadd.u16       d1, d24, d28
    vpaddl.u16      q0, q0
  .if \x == 3
    vst1.32         {d0}, [r5]!
    vst1.32         {d1[0]}, [r5, :32]
  .else
    vst1.32         {d0-d1}, [r5]
  .endif
.else
    vpaddl.u16      d16, d16
    vpaddl.u16      d20, d20
    vpaddl.u16      d24, d24
  .if \x == 4
    vpaddl.u16      d28, d28
  .endif
    vpaddl.u32      d16, d16
    vpaddl.u32      d20, d20
    vpaddl.u32      d24, d24
  .if \x == 4
    vpaddl.u32      d28, d28
  .endif
    vst1.32         {d16[0]}, [r5]!
    vst1.32         {d20[0]}, [r5]!
  .if \x == 3
    vst1.32         {d24[0]}, [r5]
  .endif
  .if \x == 4
    vst1.32         {d24[0]}, [r5]!
    vst1.32         {d28[0]}, [r5]
  .endif
.endif
    pop             {r4, r5, lr}
    bx              lr
endfunc
.endm

SAD_X_16xN 3 4
SAD_X_16xN 3 12

SAD_X_16xN 4 4
SAD_X_16xN 4 12

.macro SAD_X_16xN_LOOP x, h
function x265_sad_x\x\()_16x\h\()_neon
    push            {r4-r6, lr}
.if \x == 3
    ldrd            r4, r5, [sp, #16]
.else
    ldr             lr, [sp, #16]
    ldrd            r4, r5, [sp, #20]
.endif
    mov             r12, #FENC_STRIDE
    mov             r6, #\h/8
    veor.u8         q8, q8
    veor.u8         q9, q9
    veor.u8         q10, q10
    veor.u8         q11, q11
    veor.u8         q12, q12
    veor.u8         q13, q13
.if \x == 4
    veor.u8         q14, q14
    veor.u8         q15, q15
.endif

.loop_sad_x\x\()_16x\h:
.rept 8
    SAD_X_16 \x
.endr
    subs            r6, #1
    bne             .loop_sad_x\x\()_16x\h

    vadd.u16        q8, q8, q9
    vadd.u16        q10, q10, q11
    vadd.u16        q12, q12, q13
.if \x == 4
    vadd.u16        q14, q14, q15
.endif
    vadd.u16        d16, d16, d17
    vadd.u16        d20, d20, d21
    vadd.u16        d24, d24, d25
.if \x == 4
    vadd.u16        d28, d28, d29
.endif

.if \h <= 32
    vpadd.u16       d0, d16, d20
    vpadd.u16       d1, d24, d28
    vpaddl.u16      q0, q0
  .if \x == 3
    vst1.32         {d0}, [r5]!
    vst1.32         {d1[0]}, [r5, :32]
  .else
    vst1.32         {d0-d1}, [r5]
  .endif
.else
    vpaddl.u16      d16, d16
    vpaddl.u16      d20, d20
    vpaddl.u16      d24, d24
  .if \x == 4
    vpaddl.u16      d28, d28
  .endif
    vpaddl.u32      d16, d16
    vpaddl.u32      d20, d20
    vpaddl.u32      d24, d24
  .if \x == 4
    vpaddl.u32      d28, d28
  .endif
    vst1.32         {d16[0]}, [r5]!
    vst1.32         {d20[0]}, [r5]!
  .if \x == 3
    vst1.32         {d24[0]}, [r5]
  .endif
  .if \x == 4
    vst1.32         {d24[0]}, [r5]!
    vst1.32         {d28[0]}, [r5]
  .endif
.endif
    pop             {r4-r6, lr}
    bx              lr
endfunc
.endm

SAD_X_16xN_LOOP 3 8
SAD_X_16xN_LOOP 3 16
SAD_X_16xN_LOOP 3 32
SAD_X_16xN_LOOP 3 64

SAD_X_16xN_LOOP 4 8
SAD_X_16xN_LOOP 4 16
SAD_X_16xN_LOOP 4 32
SAD_X_16xN_LOOP 4 64

.macro SAD_X_32 x
    vld1.8          {q0}, [r0]!
    vld1.8          {q1}, [r1]!
    vld1.8          {q2}, [r2]!
    vld1.8          {q3}, [r3]!
    vabal.u8        q8, d0, d2
    vabal.u8        q9, d1, d3
    vabal.u8        q10, d0, d4
    vabal.u8        q11, d1, d5
    vabal.u8        q12, d0, d6
    vabal.u8        q13, d1, d7
.if \x == 4
    vld1.8          {q3}, [lr]!
    vabal.u8        q14, d0, d6
    vabal.u8        q15, d1, d7
.endif
    vld1.8          {q0}, [r0], r12
    vld1.8          {q1}, [r1], r4
    vld1.8          {q2}, [r2], r4
    vld1.8          {q3}, [r3], r4
    vabal.u8        q8, d0, d2
    vabal.u8        q9, d1, d3
    vabal.u8        q10, d0, d4
    vabal.u8        q11, d1, d5
    vabal.u8        q12, d0, d6
    vabal.u8        q13, d1, d7
.if \x == 4
    vld1.8          {q3}, [lr], r4
    vabal.u8        q14, d0, d6
    vabal.u8        q15, d1, d7
.endif
.endm

.macro SAD_X_32xN x, h
function x265_sad_x\x\()_32x\h\()_neon
    push            {r4-r6, lr}
.if \x == 3
    ldrd            r4, r5, [sp, #16]
.else
    ldr             lr, [sp, #16]
    ldrd            r4, r5, [sp, #20]
.endif
    mov             r12, #FENC_STRIDE
    sub             r12, #16
    sub             r4, #16
    mov             r6, #\h/8
    veor.u8         q8, q8
    veor.u8         q9, q9
    veor.u8         q10, q10
    veor.u8         q11, q11
    veor.u8         q12, q12
    veor.u8         q13, q13
.if \x == 4
    veor.u8         q14, q14
    veor.u8         q15, q15
.endif

loop_sad_x\x\()_32x\h:
.rept 8
    SAD_X_32 \x
.endr
    subs            r6, #1
    bgt             loop_sad_x\x\()_32x\h

.if \h <= 32
    vadd.u16        q8, q8, q9
    vadd.u16        q10, q10, q11
    vadd.u16        q12, q12, q13
  .if \x == 4
    vadd.u16        q14, q14, q15
  .endif
    vadd.u16        d16, d16, d17
    vadd.u16        d20, d20, d21
    vadd.u16        d24, d24, d25
  .if \x == 4
    vadd.u16        d28, d28, d29
  .endif
.else
    vpaddl.u16      q8, q8
    vpaddl.u16      q9, q9
    vpaddl.u16      q10, q10
    vpaddl.u16      q11, q11
    vpaddl.u16      q12, q12
    vpaddl.u16      q13, q13
  .if \x == 4
    vpaddl.u16      q14, q14
    vpaddl.u16      q15, q15
  .endif
    vadd.u32        q8, q8, q9
    vadd.u32        q10, q10, q11
    vadd.u32        q12, q12, q13
  .if \x == 4
    vadd.u32        q14, q14, q15
  .endif
    vadd.u32        d16, d16, d17
    vadd.u32        d20, d20, d21
    vadd.u32        d24, d24, d25
  .if \x == 4
    vadd.u32        d28, d28, d29
  .endif
.endif

.if \h <= 16
    vpadd.u16       d0, d16, d20
    vpadd.u16       d1, d24, d28
    vpaddl.u16      q0, q0
  .if \x == 3
    vst1.32         {d0}, [r5]!
    vst1.32         {d1[0]}, [r5, :32]
  .else
    vst1.32         {d0-d1}, [r5]
  .endif
.elseif \h <= 32
    vpaddl.u16      d16, d16
    vpaddl.u16      d20, d20
    vpaddl.u16      d24, d24
  .if \x == 4
    vpaddl.u16      d28, d28
  .endif
    vpaddl.u32      d16, d16
    vpaddl.u32      d20, d20
    vpaddl.u32      d24, d24
  .if \x == 4
    vpaddl.u32      d28, d28
  .endif
    vst1.32         {d16[0]}, [r5]!
    vst1.32         {d20[0]}, [r5]!
  .if \x == 3
    vst1.32         {d24[0]}, [r5]
  .endif
  .if \x == 4
    vst1.32         {d24[0]}, [r5]!
    vst1.32         {d28[0]}, [r5]
  .endif
.elseif \h <= 64
    vpaddl.u32      d16, d16
    vpaddl.u32      d20, d20
    vpaddl.u32      d24, d24
  .if \x == 4
    vpaddl.u32      d28, d28
  .endif
    vst1.32         {d16[0]}, [r5]!
    vst1.32         {d20[0]}, [r5]!
  .if \x == 3
    vst1.32         {d24[0]}, [r5]
  .endif
  .if \x == 4
    vst1.32         {d24[0]}, [r5]!
    vst1.32         {d28[0]}, [r5]
  .endif
.endif
    pop             {r4-r6, lr}
    bx              lr
endfunc
.endm

SAD_X_32xN 3 8
SAD_X_32xN 3 16
SAD_X_32xN 3 24
SAD_X_32xN 3 32
SAD_X_32xN 3 64

SAD_X_32xN 4 8
SAD_X_32xN 4 16
SAD_X_32xN 4 24
SAD_X_32xN 4 32
SAD_X_32xN 4 64

.macro SAD_X_64 x
.rept 3
    vld1.8          {q0}, [r0]!
    vld1.8          {q1}, [r1]!
    vld1.8          {q2}, [r2]!
    vld1.8          {q3}, [r3]!
    vabal.u8        q8, d0, d2
    vabal.u8        q9, d1, d3
    vabal.u8        q10, d0, d4
    vabal.u8        q11, d1, d5
    vabal.u8        q12, d0, d6
    vabal.u8        q13, d1, d7
.if \x == 4
    vld1.8          {q3}, [lr]!
    vabal.u8        q14, d0, d6
    vabal.u8        q15, d1, d7
.endif
.endr
    vld1.8          {q0}, [r0], r12
    vld1.8          {q1}, [r1], r4
    vld1.8          {q2}, [r2], r4
    vld1.8          {q3}, [r3], r4
    vabal.u8        q8, d0, d2
    vabal.u8        q9, d1, d3
    vabal.u8        q10, d0, d4
    vabal.u8        q11, d1, d5
    vabal.u8        q12, d0, d6
    vabal.u8        q13, d1, d7
.if \x == 4
    vld1.8          {q3}, [lr], r4
    vabal.u8        q14, d0, d6
    vabal.u8        q15, d1, d7
.endif
.endm

.macro SAD_X_64xN x, h
function x265_sad_x\x\()_64x\h\()_neon
    push            {r4-r6, lr}
.if \x == 3
    ldrd            r4, r5, [sp, #16]
.else
    ldr             lr, [sp, #16]
    ldrd            r4, r5, [sp, #20]
.endif
    mov             r12, #FENC_STRIDE
    sub             r12, #48
    sub             r4, #48
    mov             r6, #\h/8
    veor.u8         q8, q8
    veor.u8         q9, q9
    veor.u8         q10, q10
    veor.u8         q11, q11
    veor.u8         q12, q12
    veor.u8         q13, q13
.if \x == 4
    veor.u8         q14, q14
    veor.u8         q15, q15
.endif
.loop_sad_x\x\()_64x\h:
.rept 8
    SAD_X_64 \x
.endr
    subs            r6, #1
    bne             .loop_sad_x\x\()_64x\h

.if \h <= 16
    vadd.u16        q8, q8, q9
    vadd.u16        q10, q10, q11
    vadd.u16        q12, q12, q13
  .if \x == 4
    vadd.u16        q14, q14, q15
  .endif
    vadd.u16        d16, d16, d17
    vadd.u16        d20, d20, d21
    vadd.u16        d24, d24, d25
  .if \x == 4
    vadd.u16        d28, d28, d29
  .endif
.else
    vpaddl.u16      q8, q8
    vpaddl.u16      q9, q9
    vpaddl.u16      q10, q10
    vpaddl.u16      q11, q11
    vpaddl.u16      q12, q12
    vpaddl.u16      q13, q13
  .if \x == 4
    vpaddl.u16      q14, q14
    vpaddl.u16      q15, q15
  .endif
    vadd.u32        q8, q8, q9
    vadd.u32        q10, q10, q11
    vadd.u32        q12, q12, q13
  .if \x == 4
    vadd.u32        q14, q14, q15
  .endif
    vadd.u32        d16, d16, d17
    vadd.u32        d20, d20, d21
    vadd.u32        d24, d24, d25
  .if \x == 4
    vadd.u32        d28, d28, d29
  .endif
.endif

.if \h <= 16
    vpaddl.u16      d16, d16
    vpaddl.u16      d20, d20
    vpaddl.u16      d24, d24
  .if \x == 4
    vpaddl.u16      d28, d28
  .endif 
.endif
    vpaddl.u32      d16, d16
    vpaddl.u32      d20, d20
    vpaddl.u32      d24, d24
.if \x == 4
    vpaddl.u32      d28, d28
.endif
    vst1.32         {d16[0]}, [r5]!
    vst1.32         {d20[0]}, [r5]!
.if \x == 3
    vst1.32         {d24[0]}, [r5]
.endif
.if \x == 4
    vst1.32         {d24[0]}, [r5]!
    vst1.32         {d28[0]}, [r5]
.endif
    pop             {r4-r6, lr}
    bx              lr
endfunc
.endm

SAD_X_64xN 3 16
SAD_X_64xN 3 32
SAD_X_64xN 3 48
SAD_X_64xN 3 64

SAD_X_64xN 4 16
SAD_X_64xN 4 32
SAD_X_64xN 4 48
SAD_X_64xN 4 64

.macro SAD_X_48 x
.rept 2
    vld1.8          {q0}, [r0]!
    vld1.8          {q1}, [r1]!
    vld1.8          {q2}, [r2]!
    vld1.8          {q3}, [r3]!
    vabal.u8        q8, d0, d2
    vabal.u8        q9, d1, d3
    vabal.u8        q10, d0, d4
    vabal.u8        q11, d1, d5
    vabal.u8        q12, d0, d6
    vabal.u8        q13, d1, d7
.if \x == 4
    vld1.8          {q3}, [lr]!
    vabal.u8        q14, d0, d6
    vabal.u8        q15, d1, d7
.endif
.endr
    vld1.8          {q0}, [r0], r12
    vld1.8          {q1}, [r1], r4
    vld1.8          {q2}, [r2], r4
    vld1.8          {q3}, [r3], r4
    vabal.u8        q8, d0, d2
    vabal.u8        q9, d1, d3
    vabal.u8        q10, d0, d4
    vabal.u8        q11, d1, d5
    vabal.u8        q12, d0, d6
    vabal.u8        q13, d1, d7
.if \x == 4
    vld1.8          {q3}, [lr], r4
    vabal.u8        q14, d0, d6
    vabal.u8        q15, d1, d7
.endif
.endm

.macro SAD_X_48x64 x
function x265_sad_x\x\()_48x64_neon
    push            {r4-r6, lr}
.if \x == 3
    ldrd            r4, r5, [sp, #16]
.else
    ldr             lr, [sp, #16]
    ldrd            r4, r5, [sp, #20]
.endif
    mov             r12, #FENC_STRIDE
    sub             r12, #32
    sub             r4, #32
    mov             r6, #8
    veor.u8         q8, q8
    veor.u8         q9, q9
    veor.u8         q10, q10
    veor.u8         q11, q11
    veor.u8         q12, q12
    veor.u8         q13, q13
.if \x == 4
    veor.u8         q14, q14
    veor.u8         q15, q15
.endif

.loop_sad_x\x\()_48x64:
.rept 8
    SAD_X_48 \x
.endr
    subs            r6, #1
    bne             .loop_sad_x\x\()_48x64

    vpaddl.u16      q8, q8
    vpaddl.u16      q9, q9
    vpaddl.u16      q10, q10
    vpaddl.u16      q11, q11
    vpaddl.u16      q12, q12
    vpaddl.u16      q13, q13
.if \x == 4
    vpaddl.u16      q14, q14
    vpaddl.u16      q15, q15
.endif
    vadd.u32        q8, q8, q9
    vadd.u32        q10, q10, q11
    vadd.u32        q12, q12, q13
.if \x == 4
    vadd.u32        q14, q14, q15
.endif
    vadd.u32        d16, d16, d17
    vadd.u32        d20, d20, d21
    vadd.u32        d24, d24, d25
.if \x == 4
    vadd.u32        d28, d28, d29
.endif
    vpaddl.u32      d16, d16
    vpaddl.u32      d20, d20
    vpaddl.u32      d24, d24
    vpaddl.u32      d28, d28
.if \x == 4
    vpaddl.u32      d28, d28
.endif
    vst1.32         {d16[0]}, [r5]!
    vst1.32         {d20[0]}, [r5]!
.if \x == 3
    vst1.32         {d24[0]}, [r5]
.endif
.if \x == 4
    vst1.32         {d24[0]}, [r5]!
    vst1.32         {d28[0]}, [r5]
.endif
    pop             {r4-r6, lr}
    bx              lr
endfunc
.endm

SAD_X_48x64 3
SAD_X_48x64 4

.macro SAD_X_24 x
    vld1.8          {q0}, [r0]!
    vld1.8          {q1}, [r1]!
    vld1.8          {q2}, [r2]!
    vld1.8          {q3}, [r3]!
    vabal.u8        q8, d0, d2
    vabal.u8        q9, d1, d3
    vabal.u8        q10, d0, d4
    vabal.u8        q11, d1, d5
    vabal.u8        q12, d0, d6
    vabal.u8        q13, d1, d7
.if \x == 4
    vld1.8          {q3}, [lr]!
    vabal.u8        q14, d0, d6
    vabal.u8        q15, d1, d7
.endif
    vld1.8          {d0}, [r0], r12
    vld1.8          {d1}, [r1], r4
    vld1.8          {d2}, [r2], r4
    vld1.8          {d3}, [r3], r4
.if \x == 4
    vld1.8          {d8}, [lr], r4
.endif
    vabal.u8        q8, d0, d1
    vabal.u8        q10, d0, d2
    vabal.u8        q12, d0, d3
.if \x == 4
    vabal.u8        q14, d0, d8
.endif
.endm

.macro SAD_X_24x32 x
function x265_sad_x\x\()_24x32_neon
     push           {r4-r6, lr}
.if \x == 3
    ldrd            r4, r5, [sp, #16]
.else
    ldr             lr, [sp, #16]
    ldrd            r4, r5, [sp, #20]
.endif
    mov             r12, #FENC_STRIDE
    sub             r12, #16
    sub             r4, #16
    mov             r6, #4
    veor.u8         q8, q8
    veor.u8         q9, q9
    veor.u8         q10, q10
    veor.u8         q11, q11
    veor.u8         q12, q12
    veor.u8         q13, q13
.if \x == 4
    veor.u8         q14, q14
    veor.u8         q15, q15
.endif

.loop_sad_x\x\()_24x32:
.rept 8
    SAD_X_24 \x
.endr
    subs            r6, #1
    bne             .loop_sad_x\x\()_24x32

    vadd.u16        q8, q8, q9
    vadd.u16        q10, q10, q11
    vadd.u16        q12, q12, q13
.if \x == 4
    vadd.u16        q14, q14, q15
.endif
    vadd.u16        d16, d16, d17
    vadd.u16        d20, d20, d21
    vadd.u16        d24, d24, d25
.if \x == 4
    vadd.u16        d28, d28, d29
.endif
    vpaddl.u16      d16, d16
    vpaddl.u16      d20, d20
    vpaddl.u16      d24, d24
.if \x == 4
    vpaddl.u16      d28, d28
.endif
    vpaddl.u32      d16, d16
    vpaddl.u32      d20, d20
    vpaddl.u32      d24, d24
.if \x == 4
    vpaddl.u32      d28, d28
.endif
.if \x == 4
    vpaddl.u32      d28, d28
.endif
    vst1.32         {d16[0]}, [r5]!
    vst1.32         {d20[0]}, [r5]!
.if \x == 3
    vst1.32         {d24[0]}, [r5]
.endif
.if \x == 4
    vst1.32         {d24[0]}, [r5]!
    vst1.32         {d28[0]}, [r5]
.endif
    pop             {r4-r6, lr}
    bx              lr
endfunc
.endm

SAD_X_24x32 3
SAD_X_24x32 4

// SAD_X3 and SAD_X4 code end

.macro SAD_X_START_12 x
    vld1.8          {q0}, [r0], r12
    vld1.8          {q1}, [r1], r4
    vld1.8          {q2}, [r2], r4
    vld1.8          {q3}, [r3], r4
    vand.u8         q0, q15
    vand.u8         q1, q15
    vand.u8         q2, q15
    vand.u8         q3, q15
    vabdl.u8        q5, d0, d2
    vabdl.u8        q8, d1, d3
    vabdl.u8        q9, d0, d4
    vabdl.u8        q10, d1, d5
    vabdl.u8        q11, d0, d6
    vabdl.u8        q12, d1, d7
.if \x == 4
    vld1.8          {q3}, [lr], r4
    vand.u8         q3, q15
    vabdl.u8        q13, d0, d6
    vabdl.u8        q14, d1, d7
.endif
.endm

.macro SAD_X_12 x
    vld1.8          {q0}, [r0], r12
    vld1.8          {q1}, [r1], r4
    vld1.8          {q2}, [r2], r4
    vld1.8          {q3}, [r3], r4
    vand.u8         q0, q15
    vand.u8         q1, q15
    vand.u8         q2, q15
    vand.u8         q3, q15
    vabal.u8        q5, d0, d2
    vabal.u8        q8, d1, d3
    vabal.u8        q9, d0, d4
    vabal.u8        q10, d1, d5
    vabal.u8        q11, d0, d6
    vabal.u8        q12, d1, d7
.if \x == 4
    vld1.8          {q3}, [lr], r4
    vand.u8         q3, q15
    vabal.u8        q13, d0, d6
    vabal.u8        q14, d1, d7
.endif
.endm

.macro SAD_X_12x16 x
function x265_sad_x\x\()_12x16_neon
    push            {r4-r5, lr}
    vpush           {q5}
.if \x == 3
    ldrd            r4, r5, [sp, #28]
.else
    ldr             lr, [sp, #28]
    ldrd            r4, r5, [sp, #32]
.endif
    movrel          r12, sad12_mask
    vld1.8          {q15}, [r12]
    mov             r12, #FENC_STRIDE

    SAD_X_START_12 \x    
.rept 15
    SAD_X_12 \x
.endr
    vadd.u16        q5, q5, q8
    vadd.u16        q9, q9, q10
    vadd.u16        q11, q11, q12
.if \x == 4
    vadd.u16        q13, q13, q14
.endif
    vadd.u16        d10, d10, d11
    vadd.u16        d18, d18, d19
    vadd.u16        d22, d22, d23
.if \x == 4
    vadd.u16        d26, d26, d27
.endif
    vpadd.u16       d0, d10, d18
    vpadd.u16       d1, d22, d26
    vpaddl.u16      q0, q0
.if \x == 3
    vst1.32         {d0}, [r5]!
    vst1.32         {d1[0]}, [r5, :32]
.else
    vst1.32         {d0-d1}, [r5]
.endif
    vpop            {q5}
    pop             {r4-r5, lr}
    bx              lr
endfunc
.endm

SAD_X_12x16 3
SAD_X_12x16 4
