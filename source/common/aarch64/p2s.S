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

#if HIGH_BIT_DEPTH
# if BIT_DEPTH == 10
#  define P2S_SHIFT 4
# elif BIT_DEPTH == 12
#  define P2S_SHIFT 2
# endif
.macro p2s_start
    add             x3, x3, x3
    add             x1, x1, x1
    movi            v31.8h, #0xe0, lsl #8
.endm

#else // if !HIGH_BIT_DEPTH
# define P2S_SHIFT 6
.macro p2s_start
    add             x3, x3, x3
    movi            v31.8h, #0xe0, lsl #8
.endm
#endif // HIGH_BIT_DEPTH

.macro p2s_2x2
#if HIGH_BIT_DEPTH
    ld1             {v0.s}[0], [x0], x1
    ld1             {v0.s}[1], [x0], x1
    shl             v3.8h, v0.8h, #P2S_SHIFT
#else
    ldrh            w10, [x0]
    add             x0, x0, x1
    ldrh            w11, [x0]
    orr             w10, w10, w11, lsl #16
    add             x0, x0, x1
    dup             v0.4s, w10
    ushll           v3.8h, v0.8b, #P2S_SHIFT
#endif
    add             v3.8h, v3.8h, v31.8h
    st1             {v3.s}[0], [x2], x3
    st1             {v3.s}[1], [x2], x3
.endm

// filterPixelToShort(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride)
.macro p2s_2xN h
function PFX(filterPixelToShort_2x\h\()_neon)
    p2s_start
.rept \h / 2
    p2s_2x2
.endr
    ret
endfunc
.endm

p2s_2xN 4
p2s_2xN 8
p2s_2xN 16

.macro p2s_6x2
#if HIGH_BIT_DEPTH
    ld1             {v0.d}[0], [x0], #8
    ld1             {v1.s}[0], [x0], x1
    ld1             {v0.d}[1], [x0], #8
    ld1             {v1.s}[1], [x0], x1
    shl             v3.8h, v0.8h, #P2S_SHIFT
    shl             v4.8h, v1.8h, #P2S_SHIFT
#else
    ldr             s0, [x0]
    ldrh            w10, [x0, #4]
    add             x0, x0, x1
    ld1             {v0.s}[1], [x0]
    ldrh            w11, [x0, #4]
    add             x0, x0, x1
    orr             w10, w10, w11, lsl #16
    dup             v1.4s, w10
    ushll           v3.8h, v0.8b, #P2S_SHIFT
    ushll           v4.8h, v1.8b, #P2S_SHIFT
#endif
    add             v3.8h, v3.8h, v31.8h
    add             v4.8h, v4.8h, v31.8h
    st1             {v3.d}[0], [x2], #8
    st1             {v4.s}[0], [x2], x3
    st1             {v3.d}[1], [x2], #8
    st1             {v4.s}[1], [x2], x3
.endm

.macro p2s_6xN h
function PFX(filterPixelToShort_6x\h\()_neon)
    p2s_start
    sub             x3, x3, #8
#if HIGH_BIT_DEPTH
    sub             x1, x1, #8
#endif
.rept \h / 2
    p2s_6x2
.endr
    ret
endfunc
.endm

p2s_6xN 8
p2s_6xN 16

function PFX(filterPixelToShort_4x2_neon)
    p2s_start
#if HIGH_BIT_DEPTH
    ld1             {v0.d}[0], [x0], x1
    ld1             {v0.d}[1], [x0], x1
    shl             v3.8h, v0.8h, #P2S_SHIFT
#else
    ld1             {v0.s}[0], [x0], x1
    ld1             {v0.s}[1], [x0], x1
    ushll           v3.8h, v0.8b, #P2S_SHIFT
#endif
    add             v3.8h, v3.8h, v31.8h
    st1             {v3.d}[0], [x2], x3
    st1             {v3.d}[1], [x2], x3
    ret
endfunc

function PFX(filterPixelToShort_4x4_neon)
    p2s_start
#if HIGH_BIT_DEPTH
    ld1             {v0.d}[0], [x0], x1
    ld1             {v0.d}[1], [x0], x1
    shl             v3.8h, v0.8h, #P2S_SHIFT
#else
    ld1             {v0.s}[0], [x0], x1
    ld1             {v0.s}[1], [x0], x1
    ushll           v3.8h, v0.8b, #P2S_SHIFT
#endif
    add             v3.8h, v3.8h, v31.8h
    st1             {v3.d}[0], [x2], x3
    st1             {v3.d}[1], [x2], x3
#if HIGH_BIT_DEPTH
    ld1             {v1.d}[0], [x0], x1
    ld1             {v1.d}[1], [x0], x1
    shl             v4.8h, v1.8h, #P2S_SHIFT
#else
    ld1             {v1.s}[0], [x0], x1
    ld1             {v1.s}[1], [x0], x1
    ushll           v4.8h, v1.8b, #P2S_SHIFT
#endif
    add             v4.8h, v4.8h, v31.8h
    st1             {v4.d}[0], [x2], x3
    st1             {v4.d}[1], [x2], x3
    ret
endfunc

.macro p2s_4xN h
function PFX(filterPixelToShort_4x\h\()_neon)
    p2s_start
.rept \h / 2
#if HIGH_BIT_DEPTH
    ld1             {v0.16b}, [x0], x1
    shl             v0.8h, v0.8h, #P2S_SHIFT
#else
    ld1             {v0.8b}, [x0], x1
    ushll           v0.8h, v0.8b, #P2S_SHIFT
#endif
    add             v2.4h, v0.4h, v31.4h
    st1             {v2.4h}, [x2], x3
#if HIGH_BIT_DEPTH
    ld1             {v1.16b}, [x0], x1
    shl             v1.8h, v1.8h, #P2S_SHIFT
#else
    ld1             {v1.8b}, [x0], x1
    ushll           v1.8h, v1.8b, #P2S_SHIFT
#endif
    add             v3.4h, v1.4h, v31.4h
    st1             {v3.4h}, [x2], x3
.endr
    ret
endfunc
.endm

p2s_4xN 8
p2s_4xN 16
p2s_4xN 32

.macro p2s_8xN h
function PFX(filterPixelToShort_8x\h\()_neon)
    p2s_start
.rept \h / 2
#if HIGH_BIT_DEPTH
    ld1             {v0.16b}, [x0], x1
    ld1             {v1.16b}, [x0], x1
    shl             v0.8h, v0.8h, #P2S_SHIFT
    shl             v1.8h, v1.8h, #P2S_SHIFT
#else
    ld1             {v0.8b}, [x0], x1
    ld1             {v1.8b}, [x0], x1
    ushll           v0.8h, v0.8b, #P2S_SHIFT
    ushll           v1.8h, v1.8b, #P2S_SHIFT
#endif
    add             v2.8h, v0.8h, v31.8h
    st1             {v2.8h}, [x2], x3
    add             v3.8h, v1.8h, v31.8h
    st1             {v3.8h}, [x2], x3
.endr
    ret
endfunc
.endm

p2s_8xN 2
p2s_8xN 4
p2s_8xN 6
p2s_8xN 8
p2s_8xN 12
p2s_8xN 16
p2s_8xN 32
p2s_8xN 64

.macro p2s_12xN h
function PFX(filterPixelToShort_12x\h\()_neon)
    p2s_start
    sub             x3, x3, #16
.rept \h
#if HIGH_BIT_DEPTH
    ld1             {v0.16b-v1.16b}, [x0], x1
    shl             v2.8h, v0.8h, #P2S_SHIFT
    shl             v3.8h, v1.8h, #P2S_SHIFT
#else
    ld1             {v0.16b}, [x0], x1
    ushll           v2.8h, v0.8b,  #P2S_SHIFT
    ushll2          v3.8h, v0.16b, #P2S_SHIFT
#endif
    add             v2.8h, v2.8h, v31.8h
    add             v3.8h, v3.8h, v31.8h
    st1             {v2.16b}, [x2], #16
    st1             {v3.8b}, [x2], x3
.endr
    ret
endfunc
.endm

p2s_12xN 16
p2s_12xN 32

.macro p2s_16xN h
function PFX(filterPixelToShort_16x\h\()_neon)
    p2s_start
.rept \h
#if HIGH_BIT_DEPTH
    ld1             {v0.16b-v1.16b}, [x0], x1
    shl             v2.8h, v0.8h, #P2S_SHIFT
    shl             v3.8h, v1.8h, #P2S_SHIFT
#else
    ld1             {v0.16b}, [x0], x1
    ushll           v2.8h, v0.8b,  #P2S_SHIFT
    ushll2          v3.8h, v0.16b, #P2S_SHIFT
#endif
    add             v2.8h, v2.8h, v31.8h
    add             v3.8h, v3.8h, v31.8h
    st1             {v2.16b-v3.16b}, [x2], x3
.endr
    ret
endfunc
.endm

p2s_16xN 4
p2s_16xN 8
p2s_16xN 12
p2s_16xN 16
p2s_16xN 24
p2s_16xN 32
p2s_16xN 64

.macro p2s_24xN h
function PFX(filterPixelToShort_24x\h\()_neon)
    p2s_start
.rept \h
#if HIGH_BIT_DEPTH
    ld1             {v0.16b-v2.16b}, [x0], x1
    shl             v3.8h, v0.8h, #P2S_SHIFT
    shl             v4.8h, v1.8h, #P2S_SHIFT
    shl             v5.8h, v2.8h, #P2S_SHIFT
#else
    ld1             {v0.8b-v2.8b}, [x0], x1
    ushll           v3.8h, v0.8b, #P2S_SHIFT
    ushll           v4.8h, v1.8b, #P2S_SHIFT
    ushll           v5.8h, v2.8b, #P2S_SHIFT
#endif
    add             v3.8h, v3.8h, v31.8h
    add             v4.8h, v4.8h, v31.8h
    add             v5.8h, v5.8h, v31.8h
    st1             {v3.16b-v5.16b}, [x2], x3
.endr
    ret
endfunc
.endm

p2s_24xN 32
p2s_24xN 64

.macro p2s_32xN h
function PFX(filterPixelToShort_32x\h\()_neon)
    p2s_start
    mov             x9, #\h
.loop_filterP2S_32x\h:
    sub             x9, x9, #1
#if HIGH_BIT_DEPTH
    ld1             {v0.16b-v3.16b}, [x0], x1
    shl             v22.8h, v0.8h, #P2S_SHIFT
    shl             v23.8h, v1.8h, #P2S_SHIFT
    shl             v24.8h, v2.8h, #P2S_SHIFT
    shl             v25.8h, v3.8h, #P2S_SHIFT
#else
    ld1             {v0.16b-v1.16b}, [x0], x1
    ushll           v22.8h, v0.8b,  #P2S_SHIFT
    ushll2          v23.8h, v0.16b, #P2S_SHIFT
    ushll           v24.8h, v1.8b,  #P2S_SHIFT
    ushll2          v25.8h, v1.16b, #P2S_SHIFT
#endif
    add             v22.8h, v22.8h, v31.8h
    add             v23.8h, v23.8h, v31.8h
    add             v24.8h, v24.8h, v31.8h
    add             v25.8h, v25.8h, v31.8h
    st1             {v22.16b-v25.16b}, [x2], x3
    cbnz            x9, .loop_filterP2S_32x\h
    ret
endfunc
.endm

p2s_32xN 8
p2s_32xN 16
p2s_32xN 24
p2s_32xN 32
p2s_32xN 48
p2s_32xN 64

.macro p2s_64xN h
function PFX(filterPixelToShort_64x\h\()_neon)
    p2s_start
#if HIGH_BIT_DEPTH
    sub             x1, x1, #64
#endif
    sub             x3, x3, #64
    mov             x9, #\h
.loop_filterP2S_64x\h:
    sub             x9, x9, #1
#if HIGH_BIT_DEPTH
    ld1             {v0.16b-v3.16b}, [x0], #64
    ld1             {v4.16b-v7.16b}, [x0], x1
    shl             v16.8h, v0.8h, #P2S_SHIFT
    shl             v17.8h, v1.8h, #P2S_SHIFT
    shl             v18.8h, v2.8h, #P2S_SHIFT
    shl             v19.8h, v3.8h, #P2S_SHIFT
    shl             v20.8h, v4.8h, #P2S_SHIFT
    shl             v21.8h, v5.8h, #P2S_SHIFT
    shl             v22.8h, v6.8h, #P2S_SHIFT
    shl             v23.8h, v7.8h, #P2S_SHIFT
#else
    ld1             {v0.16b-v3.16b}, [x0], x1
    ushll           v16.8h, v0.8b,  #P2S_SHIFT
    ushll2          v17.8h, v0.16b, #P2S_SHIFT
    ushll           v18.8h, v1.8b,  #P2S_SHIFT
    ushll2          v19.8h, v1.16b, #P2S_SHIFT
    ushll           v20.8h, v2.8b,  #P2S_SHIFT
    ushll2          v21.8h, v2.16b, #P2S_SHIFT
    ushll           v22.8h, v3.8b,  #P2S_SHIFT
    ushll2          v23.8h, v3.16b, #P2S_SHIFT
#endif
    add             v16.8h, v16.8h, v31.8h
    add             v17.8h, v17.8h, v31.8h
    add             v18.8h, v18.8h, v31.8h
    add             v19.8h, v19.8h, v31.8h
    add             v20.8h, v20.8h, v31.8h
    add             v21.8h, v21.8h, v31.8h
    add             v22.8h, v22.8h, v31.8h
    add             v23.8h, v23.8h, v31.8h
    st1             {v16.16b-v19.16b}, [x2], #64
    st1             {v20.16b-v23.16b}, [x2], x3
    cbnz            x9, .loop_filterP2S_64x\h
    ret
endfunc
.endm

p2s_64xN 16
p2s_64xN 32
p2s_64xN 48
p2s_64xN 64

function PFX(filterPixelToShort_48x64_neon)
    p2s_start
#if HIGH_BIT_DEPTH
    sub             x1, x1, #64
#endif
    sub             x3, x3, #64
    mov             x9, #64
.loop_filterP2S_48x64:
    sub            x9, x9, #1
#if HIGH_BIT_DEPTH
    ld1             {v0.16b-v3.16b}, [x0], #64
    ld1             {v4.16b-v5.16b}, [x0], x1
    shl             v16.8h, v0.8h, #P2S_SHIFT
    shl             v17.8h, v1.8h, #P2S_SHIFT
    shl             v18.8h, v2.8h, #P2S_SHIFT
    shl             v19.8h, v3.8h, #P2S_SHIFT
    shl             v20.8h, v4.8h, #P2S_SHIFT
    shl             v21.8h, v5.8h, #P2S_SHIFT
#else
    ld1             {v0.16b-v2.16b}, [x0], x1
    ushll           v16.8h, v0.8b,  #P2S_SHIFT
    ushll2          v17.8h, v0.16b, #P2S_SHIFT
    ushll           v18.8h, v1.8b,  #P2S_SHIFT
    ushll2          v19.8h, v1.16b, #P2S_SHIFT
    ushll           v20.8h, v2.8b,  #P2S_SHIFT
    ushll2          v21.8h, v2.16b, #P2S_SHIFT
#endif
    add             v16.8h, v16.8h, v31.8h
    add             v17.8h, v17.8h, v31.8h
    add             v18.8h, v18.8h, v31.8h
    add             v19.8h, v19.8h, v31.8h
    add             v20.8h, v20.8h, v31.8h
    add             v21.8h, v21.8h, v31.8h
    st1             {v16.16b-v19.16b}, [x2], #64
    st1             {v20.16b-v21.16b}, [x2], x3
    cbnz            x9, .loop_filterP2S_48x64
    ret
endfunc
