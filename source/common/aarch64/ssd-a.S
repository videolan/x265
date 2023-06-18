/*****************************************************************************
 * Copyright (C) 2021 MulticoreWare, Inc
 *
 * Authors: Sebastian Pop <spop@amazon.com>
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

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

.macro ret_v0_w0
    trn2            v1.2d, v0.2d, v0.2d
    add             v0.2s, v0.2s, v1.2s
    addp            v0.2s, v0.2s, v0.2s
    fmov            w0, s0
    ret
.endm

function PFX(pixel_sse_pp_4x4_neon)
    ld1             {v16.s}[0], [x0], x1
    ld1             {v17.s}[0], [x2], x3
    ld1             {v18.s}[0], [x0], x1
    ld1             {v19.s}[0], [x2], x3
    ld1             {v20.s}[0], [x0], x1
    ld1             {v21.s}[0], [x2], x3
    ld1             {v22.s}[0], [x0], x1
    ld1             {v23.s}[0], [x2], x3

    usubl           v1.8h, v16.8b, v17.8b
    usubl           v2.8h, v18.8b, v19.8b
    usubl           v3.8h, v20.8b, v21.8b
    usubl           v4.8h, v22.8b, v23.8b

    smull           v0.4s, v1.4h, v1.4h
    smlal           v0.4s, v2.4h, v2.4h
    smlal           v0.4s, v3.4h, v3.4h
    smlal           v0.4s, v4.4h, v4.4h
    ret_v0_w0
endfunc

function PFX(pixel_sse_pp_4x8_neon)
    ld1             {v16.s}[0], [x0], x1
    ld1             {v17.s}[0], [x2], x3
    usubl           v1.8h, v16.8b, v17.8b
    ld1             {v16.s}[0], [x0], x1
    ld1             {v17.s}[0], [x2], x3
    smull           v0.4s, v1.4h, v1.4h
.rept 6
    usubl           v1.8h, v16.8b, v17.8b
    ld1             {v16.s}[0], [x0], x1
    smlal           v0.4s, v1.4h, v1.4h
    ld1             {v17.s}[0], [x2], x3
.endr
    usubl           v1.8h, v16.8b, v17.8b
    smlal           v0.4s, v1.4h, v1.4h
    ret_v0_w0
endfunc

function PFX(pixel_sse_pp_8x8_neon)
    ld1             {v16.8b}, [x0], x1
    ld1             {v17.8b}, [x2], x3
    usubl           v1.8h, v16.8b, v17.8b
    ld1             {v16.8b}, [x0], x1
    smull           v0.4s, v1.4h, v1.4h
    smlal2          v0.4s, v1.8h, v1.8h
    ld1             {v17.8b}, [x2], x3

.rept 6
    usubl           v1.8h, v16.8b, v17.8b
    ld1             {v16.8b}, [x0], x1
    smlal           v0.4s, v1.4h, v1.4h
    smlal2          v0.4s, v1.8h, v1.8h
    ld1             {v17.8b}, [x2], x3
.endr
    usubl           v1.8h, v16.8b, v17.8b
    smlal           v0.4s, v1.4h, v1.4h
    smlal2          v0.4s, v1.8h, v1.8h
    ret_v0_w0
endfunc

function PFX(pixel_sse_pp_8x16_neon)
    ld1             {v16.8b}, [x0], x1
    ld1             {v17.8b}, [x2], x3
    usubl           v1.8h, v16.8b, v17.8b
    ld1             {v16.8b}, [x0], x1
    smull           v0.4s, v1.4h, v1.4h
    smlal2          v0.4s, v1.8h, v1.8h
    ld1             {v17.8b}, [x2], x3

.rept 14
    usubl           v1.8h, v16.8b, v17.8b
    ld1             {v16.8b}, [x0], x1
    smlal           v0.4s, v1.4h, v1.4h
    smlal2          v0.4s, v1.8h, v1.8h
    ld1             {v17.8b}, [x2], x3
.endr
    usubl           v1.8h, v16.8b, v17.8b
    smlal           v0.4s, v1.4h, v1.4h
    smlal2          v0.4s, v1.8h, v1.8h
    ret_v0_w0
endfunc

.macro sse_pp_16xN h
function PFX(pixel_sse_pp_16x\h\()_neon)
    ld1             {v16.16b}, [x0], x1
    ld1             {v17.16b}, [x2], x3
    usubl           v1.8h, v16.8b, v17.8b
    usubl2          v2.8h, v16.16b, v17.16b
    ld1             {v16.16b}, [x0], x1
    ld1             {v17.16b}, [x2], x3
    smull           v0.4s, v1.4h, v1.4h
    smlal2          v0.4s, v1.8h, v1.8h
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v0.4s, v2.8h, v2.8h
.rept \h - 2
    usubl           v1.8h, v16.8b, v17.8b
    usubl2          v2.8h, v16.16b, v17.16b
    ld1             {v16.16b}, [x0], x1
    smlal           v0.4s, v1.4h, v1.4h
    smlal2          v0.4s, v1.8h, v1.8h
    ld1             {v17.16b}, [x2], x3
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v0.4s, v2.8h, v2.8h
.endr
    usubl           v1.8h, v16.8b, v17.8b
    usubl2          v2.8h, v16.16b, v17.16b
    smlal           v0.4s, v1.4h, v1.4h
    smlal2          v0.4s, v1.8h, v1.8h
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v0.4s, v2.8h, v2.8h
    ret_v0_w0
endfunc
.endm

sse_pp_16xN 16
sse_pp_16xN 32

function PFX(pixel_sse_pp_32x32_neon)
    mov             w12, #8
    movi            v0.16b, #0
    movi            v1.16b, #0
.loop_sse_pp_32:
    sub             w12, w12, #1
.rept 4
    ld1             {v16.16b,v17.16b}, [x0], x1
    ld1             {v18.16b,v19.16b}, [x2], x3
    usubl           v2.8h, v16.8b, v18.8b
    usubl2          v3.8h, v16.16b, v18.16b
    usubl           v4.8h, v17.8b, v19.8b
    usubl2          v5.8h, v17.16b, v19.16b
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v1.4s, v2.8h, v2.8h
    smlal           v0.4s, v3.4h, v3.4h
    smlal2          v1.4s, v3.8h, v3.8h
    smlal           v0.4s, v4.4h, v4.4h
    smlal2          v1.4s, v4.8h, v4.8h
    smlal           v0.4s, v5.4h, v5.4h
    smlal2          v1.4s, v5.8h, v5.8h
.endr
    cbnz            w12, .loop_sse_pp_32
    add             v0.4s, v0.4s, v1.4s
    ret_v0_w0
endfunc

function PFX(pixel_sse_pp_32x64_neon)
    mov             w12, #16
    movi            v0.16b, #0
    movi            v1.16b, #0
.loop_sse_pp_32x64:
    sub             w12, w12, #1
.rept 4
    ld1             {v16.16b,v17.16b}, [x0], x1
    ld1             {v18.16b,v19.16b}, [x2], x3
    usubl           v2.8h, v16.8b, v18.8b
    usubl2          v3.8h, v16.16b, v18.16b
    usubl           v4.8h, v17.8b, v19.8b
    usubl2          v5.8h, v17.16b, v19.16b
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v1.4s, v2.8h, v2.8h
    smlal           v0.4s, v3.4h, v3.4h
    smlal2          v1.4s, v3.8h, v3.8h
    smlal           v0.4s, v4.4h, v4.4h
    smlal2          v1.4s, v4.8h, v4.8h
    smlal           v0.4s, v5.4h, v5.4h
    smlal2          v1.4s, v5.8h, v5.8h
.endr
    cbnz            w12, .loop_sse_pp_32x64
    add             v0.4s, v0.4s, v1.4s
    ret_v0_w0
endfunc

function PFX(pixel_sse_pp_64x64_neon)
    mov             w12, #16
    movi            v0.16b, #0
    movi            v1.16b, #0

.loop_sse_pp_64:
    sub             w12, w12, #1
.rept 4
    ld1             {v16.16b-v19.16b}, [x0], x1
    ld1             {v20.16b-v23.16b}, [x2], x3

    usubl           v2.8h, v16.8b, v20.8b
    usubl2          v3.8h, v16.16b, v20.16b
    usubl           v4.8h, v17.8b, v21.8b
    usubl2          v5.8h, v17.16b, v21.16b
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v1.4s, v2.8h, v2.8h
    smlal           v0.4s, v3.4h, v3.4h
    smlal2          v1.4s, v3.8h, v3.8h
    smlal           v0.4s, v4.4h, v4.4h
    smlal2          v1.4s, v4.8h, v4.8h
    smlal           v0.4s, v5.4h, v5.4h
    smlal2          v1.4s, v5.8h, v5.8h

    usubl           v2.8h, v18.8b, v22.8b
    usubl2          v3.8h, v18.16b, v22.16b
    usubl           v4.8h, v19.8b, v23.8b
    usubl2          v5.8h, v19.16b, v23.16b
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v1.4s, v2.8h, v2.8h
    smlal           v0.4s, v3.4h, v3.4h
    smlal2          v1.4s, v3.8h, v3.8h
    smlal           v0.4s, v4.4h, v4.4h
    smlal2          v1.4s, v4.8h, v4.8h
    smlal           v0.4s, v5.4h, v5.4h
    smlal2          v1.4s, v5.8h, v5.8h
.endr
    cbnz            w12, .loop_sse_pp_64
    add             v0.4s, v0.4s, v1.4s
    ret_v0_w0
endfunc

function PFX(pixel_sse_ss_4x4_neon)
    add             x1, x1, x1
    add             x3, x3, x3
    ld1             {v16.8b}, [x0], x1
    ld1             {v17.8b}, [x2], x3
    sub             v2.4h, v16.4h, v17.4h
    ld1             {v16.8b}, [x0], x1
    ld1             {v17.8b}, [x2], x3
    smull           v0.4s, v2.4h, v2.4h
    sub             v2.4h, v16.4h, v17.4h
    ld1             {v16.8b}, [x0], x1
    ld1             {v17.8b}, [x2], x3
    smlal           v0.4s, v2.4h, v2.4h
    sub             v2.4h, v16.4h, v17.4h
    ld1             {v16.8b}, [x0], x1
    smlal           v0.4s, v2.4h, v2.4h
    ld1             {v17.8b}, [x2], x3
    sub             v2.4h, v16.4h, v17.4h
    smlal           v0.4s, v2.4h, v2.4h
    ret_v0_w0
endfunc

function PFX(pixel_sse_ss_8x8_neon)
    add             x1, x1, x1
    add             x3, x3, x3
    ld1             {v16.16b}, [x0], x1
    ld1             {v17.16b}, [x2], x3
    sub             v2.8h, v16.8h, v17.8h
    ld1             {v16.16b}, [x0], x1
    ld1             {v17.16b}, [x2], x3
    smull           v0.4s, v2.4h, v2.4h
    smull2          v1.4s, v2.8h, v2.8h
    sub             v2.8h, v16.8h, v17.8h
.rept 6
    ld1             {v16.16b}, [x0], x1
    ld1             {v17.16b}, [x2], x3
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v1.4s, v2.8h, v2.8h
    sub             v2.8h, v16.8h, v17.8h
.endr
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v1.4s, v2.8h, v2.8h
    add             v0.4s, v0.4s, v1.4s
    ret_v0_w0
endfunc

function PFX(pixel_sse_ss_16x16_neon)
    add             x1, x1, x1
    add             x3, x3, x3
    mov             w12, #4
    movi            v0.16b, #0
    movi            v1.16b, #0
.loop_sse_ss_16:
    sub             w12, w12, #1
.rept 4
    ld1             {v16.16b, v17.16b}, [x0], x1
    ld1             {v18.16b, v19.16b}, [x2], x3
    sub             v2.8h, v16.8h, v18.8h
    sub             v3.8h, v17.8h, v19.8h
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v1.4s, v2.8h, v2.8h
    smlal           v0.4s, v3.4h, v3.4h
    smlal2          v1.4s, v3.8h, v3.8h
.endr
    cbnz            w12, .loop_sse_ss_16
    add             v0.4s, v0.4s, v1.4s
    ret_v0_w0
endfunc

function PFX(pixel_sse_ss_32x32_neon)
    add             x1, x1, x1
    add             x3, x3, x3

    mov             w12, #8
    movi            v0.16b, #0
    movi            v1.16b, #0
.loop_sse_ss_32:
    sub             w12, w12, #1
.rept 4
    ld1             {v16.16b-v19.16b}, [x0], x1
    ld1             {v20.16b-v23.16b}, [x2], x3
    sub             v2.8h, v16.8h, v20.8h
    sub             v3.8h, v17.8h, v21.8h
    sub             v4.8h, v18.8h, v22.8h
    sub             v5.8h, v19.8h, v23.8h
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v1.4s, v2.8h, v2.8h
    smlal           v0.4s, v3.4h, v3.4h
    smlal2          v1.4s, v3.8h, v3.8h
    smlal           v0.4s, v4.4h, v4.4h
    smlal2          v1.4s, v4.8h, v4.8h
    smlal           v0.4s, v5.4h, v5.4h
    smlal2          v1.4s, v5.8h, v5.8h
.endr
    cbnz            w12, .loop_sse_ss_32
    add             v0.4s, v0.4s, v1.4s
    ret_v0_w0
endfunc

function PFX(pixel_sse_ss_64x64_neon)
    add             x1, x1, x1
    add             x3, x3, x3
    sub             x1, x1, #64
    sub             x3, x3, #64

    mov             w12, #32
    movi            v0.16b, #0
    movi            v1.16b, #0
.loop_sse_ss_64:
    sub             w12, w12, #1
.rept 2
    ld1             {v16.16b-v19.16b}, [x0], #64
    ld1             {v20.16b-v23.16b}, [x2], #64
    sub             v2.8h, v16.8h, v20.8h
    sub             v3.8h, v17.8h, v21.8h
    sub             v4.8h, v18.8h, v22.8h
    sub             v5.8h, v19.8h, v23.8h
    ld1             {v16.16b-v19.16b}, [x0], x1
    ld1             {v20.16b-v23.16b}, [x2], x3
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v1.4s, v2.8h, v2.8h
    smlal           v0.4s, v3.4h, v3.4h
    smlal2          v1.4s, v3.8h, v3.8h
    smlal           v0.4s, v4.4h, v4.4h
    smlal2          v1.4s, v4.8h, v4.8h
    smlal           v0.4s, v5.4h, v5.4h
    smlal2          v1.4s, v5.8h, v5.8h
    sub             v2.8h, v16.8h, v20.8h
    sub             v3.8h, v17.8h, v21.8h
    sub             v4.8h, v18.8h, v22.8h
    sub             v5.8h, v19.8h, v23.8h
    smlal           v0.4s, v2.4h, v2.4h
    smlal2          v1.4s, v2.8h, v2.8h
    smlal           v0.4s, v3.4h, v3.4h
    smlal2          v1.4s, v3.8h, v3.8h
    smlal           v0.4s, v4.4h, v4.4h
    smlal2          v1.4s, v4.8h, v4.8h
    smlal           v0.4s, v5.4h, v5.4h
    smlal2          v1.4s, v5.8h, v5.8h
.endr
    cbnz            w12, .loop_sse_ss_64
    add             v0.4s, v0.4s, v1.4s
    ret_v0_w0
endfunc

function PFX(pixel_ssd_s_4x4_neon)
    add             x1, x1, x1
    ld1             {v4.8b}, [x0], x1
    ld1             {v5.8b}, [x0], x1
    ld1             {v6.8b}, [x0], x1
    ld1             {v7.8b}, [x0]
    smull           v0.4s, v4.4h, v4.4h
    smull           v1.4s, v5.4h, v5.4h
    smlal           v0.4s, v6.4h, v6.4h
    smlal           v1.4s, v7.4h, v7.4h
    add             v0.4s, v0.4s, v1.4s
    ret_v0_w0
endfunc

function PFX(pixel_ssd_s_8x8_neon)
    add             x1, x1, x1
    ld1             {v4.16b}, [x0], x1
    ld1             {v5.16b}, [x0], x1
    smull           v0.4s, v4.4h, v4.4h
    smull2          v1.4s, v4.8h, v4.8h
    smlal           v0.4s, v5.4h, v5.4h
    smlal2          v1.4s, v5.8h, v5.8h
.rept 3
    ld1             {v4.16b}, [x0], x1
    ld1             {v5.16b}, [x0], x1
    smlal           v0.4s, v4.4h, v4.4h
    smlal2          v1.4s, v4.8h, v4.8h
    smlal           v0.4s, v5.4h, v5.4h
    smlal2          v1.4s, v5.8h, v5.8h
.endr
    add             v0.4s, v0.4s, v1.4s
    ret_v0_w0
endfunc

function PFX(pixel_ssd_s_16x16_neon)
    add             x1, x1, x1
    mov             w12, #4
    movi            v0.16b, #0
    movi            v1.16b, #0
.loop_ssd_s_16:
    sub             w12, w12, #1
.rept 2
    ld1             {v4.16b,v5.16b}, [x0], x1
    ld1             {v6.16b,v7.16b}, [x0], x1
    smlal           v0.4s, v4.4h, v4.4h
    smlal2          v1.4s, v4.8h, v4.8h
    smlal           v0.4s, v5.4h, v5.4h
    smlal2          v1.4s, v5.8h, v5.8h
    smlal           v0.4s, v6.4h, v6.4h
    smlal2          v1.4s, v6.8h, v6.8h
    smlal           v0.4s, v7.4h, v7.4h
    smlal2          v1.4s, v7.8h, v7.8h
.endr
    cbnz            w12, .loop_ssd_s_16
    add             v0.4s, v0.4s, v1.4s
    ret_v0_w0
endfunc

function PFX(pixel_ssd_s_32x32_neon)
    add             x1, x1, x1
    mov             w12, #8
    movi            v0.16b, #0
    movi            v1.16b, #0
.loop_ssd_s_32:
    sub             w12, w12, #1
.rept 4
    ld1             {v4.16b-v7.16b}, [x0], x1
    smlal           v0.4s, v4.4h, v4.4h
    smlal2          v1.4s, v4.8h, v4.8h
    smlal           v0.4s, v5.4h, v5.4h
    smlal2          v1.4s, v5.8h, v5.8h
    smlal           v0.4s, v6.4h, v6.4h
    smlal2          v1.4s, v6.8h, v6.8h
    smlal           v0.4s, v7.4h, v7.4h
    smlal2          v1.4s, v7.8h, v7.8h
.endr
    cbnz            w12, .loop_ssd_s_32
    add             v0.4s, v0.4s, v1.4s
    ret_v0_w0
endfunc
