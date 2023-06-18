/*****************************************************************************
 * Copyright (C) 2020-2021 MulticoreWare, Inc
 *
 * Authors: Hongbin Liu <liuhongbin1@huawei.com>
 *          Sebastian Pop <spop@amazon.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "asm.S"

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

.macro pixel_avg_pp_4xN_neon h
function PFX(pixel_avg_pp_4x\h\()_neon)
.rept \h
    ld1             {v0.s}[0], [x2], x3
    ld1             {v1.s}[0], [x4], x5
    urhadd          v2.8b, v0.8b, v1.8b
    st1             {v2.s}[0], [x0], x1
.endr
    ret
endfunc
.endm

pixel_avg_pp_4xN_neon 4
pixel_avg_pp_4xN_neon 8
pixel_avg_pp_4xN_neon 16

.macro pixel_avg_pp_8xN_neon h
function PFX(pixel_avg_pp_8x\h\()_neon)
.rept \h
    ld1             {v0.8b}, [x2], x3
    ld1             {v1.8b}, [x4], x5
    urhadd          v2.8b, v0.8b, v1.8b
    st1             {v2.8b}, [x0], x1
.endr
    ret
endfunc
.endm

pixel_avg_pp_8xN_neon 4
pixel_avg_pp_8xN_neon 8
pixel_avg_pp_8xN_neon 16
pixel_avg_pp_8xN_neon 32

function PFX(pixel_avg_pp_12x16_neon)
    sub             x1, x1, #4
    sub             x3, x3, #4
    sub             x5, x5, #4
.rept 16
    ld1             {v0.s}[0], [x2], #4
    ld1             {v1.8b}, [x2], x3
    ld1             {v2.s}[0], [x4], #4
    ld1             {v3.8b}, [x4], x5
    urhadd          v4.8b, v0.8b, v2.8b
    urhadd          v5.8b, v1.8b, v3.8b
    st1             {v4.s}[0], [x0], #4
    st1             {v5.8b}, [x0], x1
.endr
    ret
endfunc

.macro pixel_avg_pp_16xN_neon h
function PFX(pixel_avg_pp_16x\h\()_neon)
.rept \h
    ld1             {v0.16b}, [x2], x3
    ld1             {v1.16b}, [x4], x5
    urhadd          v2.16b, v0.16b, v1.16b
    st1             {v2.16b}, [x0], x1
.endr
    ret
endfunc
.endm

pixel_avg_pp_16xN_neon 4
pixel_avg_pp_16xN_neon 8
pixel_avg_pp_16xN_neon 12
pixel_avg_pp_16xN_neon 16
pixel_avg_pp_16xN_neon 32

function PFX(pixel_avg_pp_16x64_neon)
    mov             w12, #8
.lpavg_16x64:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.16b}, [x2], x3
    ld1             {v1.16b}, [x4], x5
    urhadd          v2.16b, v0.16b, v1.16b
    st1             {v2.16b}, [x0], x1
.endr
    cbnz            w12, .lpavg_16x64
    ret
endfunc

function PFX(pixel_avg_pp_24x32_neon)
    sub             x1, x1, #16
    sub             x3, x3, #16
    sub             x5, x5, #16
    mov             w12, #4
.lpavg_24x32:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.16b}, [x2], #16
    ld1             {v1.8b}, [x2], x3
    ld1             {v2.16b}, [x4], #16
    ld1             {v3.8b}, [x4], x5
    urhadd          v0.16b, v0.16b, v2.16b
    urhadd          v1.8b, v1.8b, v3.8b
    st1             {v0.16b}, [x0], #16
    st1             {v1.8b}, [x0], x1
.endr
    cbnz            w12, .lpavg_24x32
    ret
endfunc

.macro pixel_avg_pp_32xN_neon h
function PFX(pixel_avg_pp_32x\h\()_neon)
.rept \h
    ld1             {v0.16b-v1.16b}, [x2], x3
    ld1             {v2.16b-v3.16b}, [x4], x5
    urhadd          v0.16b, v0.16b, v2.16b
    urhadd          v1.16b, v1.16b, v3.16b
    st1             {v0.16b-v1.16b}, [x0], x1
.endr
    ret
endfunc
.endm

pixel_avg_pp_32xN_neon 8
pixel_avg_pp_32xN_neon 16
pixel_avg_pp_32xN_neon 24

.macro pixel_avg_pp_32xN1_neon h
function PFX(pixel_avg_pp_32x\h\()_neon)
    mov             w12, #\h / 8
.lpavg_32x\h\():
    sub             w12, w12, #1
.rept 8
    ld1             {v0.16b-v1.16b}, [x2], x3
    ld1             {v2.16b-v3.16b}, [x4], x5
    urhadd          v0.16b, v0.16b, v2.16b
    urhadd          v1.16b, v1.16b, v3.16b
    st1             {v0.16b-v1.16b}, [x0], x1
.endr
    cbnz            w12, .lpavg_32x\h
    ret
endfunc
.endm

pixel_avg_pp_32xN1_neon 32
pixel_avg_pp_32xN1_neon 64

function PFX(pixel_avg_pp_48x64_neon)
    mov             w12, #8
.lpavg_48x64:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.16b-v2.16b}, [x2], x3
    ld1             {v3.16b-v5.16b}, [x4], x5
    urhadd          v0.16b, v0.16b, v3.16b
    urhadd          v1.16b, v1.16b, v4.16b
    urhadd          v2.16b, v2.16b, v5.16b
    st1             {v0.16b-v2.16b}, [x0], x1
.endr
    cbnz            w12, .lpavg_48x64
    ret
endfunc

.macro pixel_avg_pp_64xN_neon h
function PFX(pixel_avg_pp_64x\h\()_neon)
    mov             w12, #\h / 4
.lpavg_64x\h\():
    sub             w12, w12, #1
.rept 4
    ld1             {v0.16b-v3.16b}, [x2], x3
    ld1             {v4.16b-v7.16b}, [x4], x5
    urhadd          v0.16b, v0.16b, v4.16b
    urhadd          v1.16b, v1.16b, v5.16b
    urhadd          v2.16b, v2.16b, v6.16b
    urhadd          v3.16b, v3.16b, v7.16b
    st1             {v0.16b-v3.16b}, [x0], x1
.endr
    cbnz            w12, .lpavg_64x\h
    ret
endfunc
.endm

pixel_avg_pp_64xN_neon 16
pixel_avg_pp_64xN_neon 32
pixel_avg_pp_64xN_neon 48
pixel_avg_pp_64xN_neon 64

// void addAvg(const int16_t* src0, const int16_t* src1, pixel* dst, intptr_t src0Stride, intptr_t src1Stride, intptr_t dstStride)
.macro addAvg_start
    lsl             x3, x3, #1
    lsl             x4, x4, #1
    mov             w11, #0x40
    dup             v30.16b, w11
.endm

.macro addAvg_2xN h
function PFX(addAvg_2x\h\()_neon)
    addAvg_start
.rept \h / 2
    ldr             w10, [x0]
    ldr             w11, [x1]
    add             x0, x0, x3
    add             x1, x1, x4
    ldr             w12, [x0]
    ldr             w13, [x1]
    add             x0, x0, x3
    add             x1, x1, x4
    dup             v0.2s, w10
    dup             v1.2s, w11
    dup             v2.2s, w12
    dup             v3.2s, w13
    add             v0.4h, v0.4h, v1.4h
    add             v2.4h, v2.4h, v3.4h
    saddl           v0.4s, v0.4h, v30.4h
    saddl           v2.4s, v2.4h, v30.4h
    shrn            v0.4h, v0.4s, #7
    shrn2           v0.8h, v2.4s, #7
    sqxtun          v0.8b, v0.8h
    st1             {v0.h}[0], [x2], x5
    st1             {v0.h}[2], [x2], x5
.endr
    ret
endfunc
.endm

addAvg_2xN 4
addAvg_2xN 8
addAvg_2xN 16

.macro addAvg_4xN h
function PFX(addAvg_4x\h\()_neon)
    addAvg_start
.rept \h / 2
    ld1             {v0.8b}, [x0], x3
    ld1             {v1.8b}, [x1], x4
    ld1             {v2.8b}, [x0], x3
    ld1             {v3.8b}, [x1], x4
    add             v0.4h, v0.4h, v1.4h
    add             v2.4h, v2.4h, v3.4h
    saddl           v0.4s, v0.4h, v30.4h
    saddl           v2.4s, v2.4h, v30.4h
    shrn            v0.4h, v0.4s, #7
    shrn2           v0.8h, v2.4s, #7
    sqxtun          v0.8b, v0.8h
    st1             {v0.s}[0], [x2], x5
    st1             {v0.s}[1], [x2], x5
.endr
    ret
endfunc
.endm

addAvg_4xN 2
addAvg_4xN 4
addAvg_4xN 8
addAvg_4xN 16
addAvg_4xN 32

.macro addAvg_6xN h
function PFX(addAvg_6x\h\()_neon)
    addAvg_start
    mov             w12, #\h / 2
    sub             x5, x5, #4
.loop_addavg_6x\h:
    sub             w12, w12, #1
    ld1             {v0.16b}, [x0], x3
    ld1             {v1.16b}, [x1], x4
    ld1             {v2.16b}, [x0], x3
    ld1             {v3.16b}, [x1], x4
    add             v0.8h, v0.8h, v1.8h
    add             v2.8h, v2.8h, v3.8h
    saddl           v16.4s, v0.4h, v30.4h
    saddl2          v17.4s, v0.8h, v30.8h
    saddl           v18.4s, v2.4h, v30.4h
    saddl2          v19.4s, v2.8h, v30.8h
    shrn            v0.4h, v16.4s, #7
    shrn2           v0.8h, v17.4s, #7
    shrn            v1.4h, v18.4s, #7
    shrn2           v1.8h, v19.4s, #7
    sqxtun          v0.8b, v0.8h
    sqxtun          v1.8b, v1.8h
    str             s0, [x2], #4
    st1             {v0.h}[2], [x2], x5
    str             s1, [x2], #4
    st1             {v1.h}[2], [x2], x5
    cbnz            w12, .loop_addavg_6x\h
    ret
endfunc
.endm

addAvg_6xN 8
addAvg_6xN 16

.macro addAvg_8xN h
function PFX(addAvg_8x\h\()_neon)
    addAvg_start
.rept \h / 2
    ld1             {v0.16b}, [x0], x3
    ld1             {v1.16b}, [x1], x4
    ld1             {v2.16b}, [x0], x3
    ld1             {v3.16b}, [x1], x4
    add             v0.8h, v0.8h, v1.8h
    add             v2.8h, v2.8h, v3.8h
    saddl           v16.4s, v0.4h, v30.4h
    saddl2          v17.4s, v0.8h, v30.8h
    saddl           v18.4s, v2.4h, v30.4h
    saddl2          v19.4s, v2.8h, v30.8h
    shrn            v0.4h, v16.4s, #7
    shrn2           v0.8h, v17.4s, #7
    shrn            v1.4h, v18.4s, #7
    shrn2           v1.8h, v19.4s, #7
    sqxtun          v0.8b, v0.8h
    sqxtun          v1.8b, v1.8h
    st1             {v0.8b}, [x2], x5
    st1             {v1.8b}, [x2], x5
.endr
    ret
endfunc
.endm

.macro addAvg_8xN1 h
function PFX(addAvg_8x\h\()_neon)
    addAvg_start
    mov             w12, #\h / 2
.loop_addavg_8x\h:
    sub             w12, w12, #1
    ld1             {v0.16b}, [x0], x3
    ld1             {v1.16b}, [x1], x4
    ld1             {v2.16b}, [x0], x3
    ld1             {v3.16b}, [x1], x4
    add             v0.8h, v0.8h, v1.8h
    add             v2.8h, v2.8h, v3.8h
    saddl           v16.4s, v0.4h, v30.4h
    saddl2          v17.4s, v0.8h, v30.8h
    saddl           v18.4s, v2.4h, v30.4h
    saddl2          v19.4s, v2.8h, v30.8h
    shrn            v0.4h, v16.4s, #7
    shrn2           v0.8h, v17.4s, #7
    shrn            v1.4h, v18.4s, #7
    shrn2           v1.8h, v19.4s, #7
    sqxtun          v0.8b, v0.8h
    sqxtun          v1.8b, v1.8h
    st1             {v0.8b}, [x2], x5
    st1             {v1.8b}, [x2], x5
    cbnz            w12, .loop_addavg_8x\h
    ret
endfunc
.endm

addAvg_8xN 2
addAvg_8xN 4
addAvg_8xN 6
addAvg_8xN 8
addAvg_8xN 12
addAvg_8xN 16
addAvg_8xN1 32
addAvg_8xN1 64

.macro addAvg_12xN h
function PFX(addAvg_12x\h\()_neon)
    addAvg_start
    sub             x3, x3, #16
    sub             x4, x4, #16
    sub             x5, x5, #8
    mov             w12, #\h
.loop_addAvg_12X\h\():
    sub             w12, w12, #1
    ld1             {v0.16b}, [x0], #16
    ld1             {v1.16b}, [x1], #16
    ld1             {v2.8b}, [x0], x3
    ld1             {v3.8b}, [x1], x4
    add             v0.8h, v0.8h, v1.8h
    add             v2.4h, v2.4h, v3.4h
    saddl           v16.4s, v0.4h, v30.4h
    saddl2          v17.4s, v0.8h, v30.8h
    saddl           v18.4s, v2.4h, v30.4h
    shrn            v0.4h, v16.4s, #7
    shrn2           v0.8h, v17.4s, #7
    shrn            v1.4h, v18.4s, #7
    sqxtun          v0.8b, v0.8h
    sqxtun          v1.8b, v1.8h
    st1             {v0.8b}, [x2], #8
    st1             {v1.s}[0], [x2], x5
    cbnz            w12, .loop_addAvg_12X\h
    ret
endfunc
.endm

addAvg_12xN 16
addAvg_12xN 32

.macro addavg_1 v0, v1
    add             \v0\().8h, \v0\().8h, \v1\().8h
    saddl           v16.4s, \v0\().4h, v30.4h
    saddl2          v17.4s, \v0\().8h, v30.8h
    shrn            \v0\().4h, v16.4s, #7
    shrn2           \v0\().8h, v17.4s, #7
.endm

.macro addAvg_16xN h
function PFX(addAvg_16x\h\()_neon)
    addAvg_start
    mov             w12, #\h
.loop_addavg_16x\h:
    sub             w12, w12, #1
    ld1             {v0.8h-v1.8h}, [x0], x3
    ld1             {v2.8h-v3.8h}, [x1], x4
    addavg_1        v0, v2
    addavg_1        v1, v3
    sqxtun          v0.8b, v0.8h
    sqxtun2         v0.16b, v1.8h
    st1             {v0.16b}, [x2], x5
    cbnz            w12, .loop_addavg_16x\h
    ret
endfunc
.endm

addAvg_16xN 4
addAvg_16xN 8
addAvg_16xN 12
addAvg_16xN 16
addAvg_16xN 24
addAvg_16xN 32
addAvg_16xN 64

.macro addAvg_24xN h
function PFX(addAvg_24x\h\()_neon)
    addAvg_start
    mov             w12, #\h
.loop_addavg_24x\h\():
    sub             w12, w12, #1
    ld1             {v0.16b-v2.16b}, [x0], x3
    ld1             {v3.16b-v5.16b}, [x1], x4
    addavg_1        v0, v3
    addavg_1        v1, v4
    addavg_1        v2, v5
    sqxtun          v0.8b, v0.8h
    sqxtun          v1.8b, v1.8h
    sqxtun          v2.8b, v2.8h
    st1             {v0.8b-v2.8b}, [x2], x5
    cbnz            w12, .loop_addavg_24x\h
    ret
endfunc
.endm

addAvg_24xN 32
addAvg_24xN 64

.macro addAvg_32xN h
function PFX(addAvg_32x\h\()_neon)
    addAvg_start
    mov             w12, #\h
.loop_addavg_32x\h\():
    sub             w12, w12, #1
    ld1             {v0.8h-v3.8h}, [x0], x3
    ld1             {v4.8h-v7.8h}, [x1], x4
    addavg_1        v0, v4
    addavg_1        v1, v5
    addavg_1        v2, v6
    addavg_1        v3, v7
    sqxtun          v0.8b, v0.8h
    sqxtun          v1.8b, v1.8h
    sqxtun          v2.8b, v2.8h
    sqxtun          v3.8b, v3.8h
    st1             {v0.8b-v3.8b}, [x2], x5
    cbnz            w12, .loop_addavg_32x\h
    ret
endfunc
.endm

addAvg_32xN 8
addAvg_32xN 16
addAvg_32xN 24
addAvg_32xN 32
addAvg_32xN 48
addAvg_32xN 64

function PFX(addAvg_48x64_neon)
    addAvg_start
    sub             x3, x3, #64
    sub             x4, x4, #64
    mov             w12, #64
.loop_addavg_48x64:
    sub             w12, w12, #1
    ld1             {v0.8h-v3.8h}, [x0], #64
    ld1             {v4.8h-v7.8h}, [x1], #64
    ld1             {v20.8h-v21.8h}, [x0], x3
    ld1             {v22.8h-v23.8h}, [x1], x4
    addavg_1        v0, v4
    addavg_1        v1, v5
    addavg_1        v2, v6
    addavg_1        v3, v7
    addavg_1        v20, v22
    addavg_1        v21, v23
    sqxtun          v0.8b, v0.8h
    sqxtun2         v0.16b, v1.8h
    sqxtun          v1.8b, v2.8h
    sqxtun2         v1.16b, v3.8h
    sqxtun          v2.8b, v20.8h
    sqxtun2         v2.16b, v21.8h
    st1             {v0.16b-v2.16b}, [x2], x5
    cbnz            w12, .loop_addavg_48x64
    ret
endfunc

.macro addAvg_64xN h
function PFX(addAvg_64x\h\()_neon)
    addAvg_start
    mov             w12, #\h
    sub             x3, x3, #64
    sub             x4, x4, #64
.loop_addavg_64x\h\():
    sub             w12, w12, #1
    ld1             {v0.8h-v3.8h}, [x0], #64
    ld1             {v4.8h-v7.8h}, [x1], #64
    ld1             {v20.8h-v23.8h}, [x0], x3
    ld1             {v24.8h-v27.8h}, [x1], x4
    addavg_1        v0, v4
    addavg_1        v1, v5
    addavg_1        v2, v6
    addavg_1        v3, v7
    addavg_1        v20, v24
    addavg_1        v21, v25
    addavg_1        v22, v26
    addavg_1        v23, v27
    sqxtun          v0.8b, v0.8h
    sqxtun2         v0.16b, v1.8h
    sqxtun          v1.8b, v2.8h
    sqxtun2         v1.16b, v3.8h
    sqxtun          v2.8b, v20.8h
    sqxtun2         v2.16b, v21.8h
    sqxtun          v3.8b, v22.8h
    sqxtun2         v3.16b, v23.8h
    st1             {v0.16b-v3.16b}, [x2], x5
    cbnz            w12, .loop_addavg_64x\h
    ret
endfunc
.endm

addAvg_64xN 16
addAvg_64xN 32
addAvg_64xN 48
addAvg_64xN 64
