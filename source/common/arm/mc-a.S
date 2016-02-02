/*****************************************************************************
 * Copyright (C) 2016 x265 project
 *
 * Authors: Dnyaneshwar Gorade <dnyaneshwar@multicorewareinc.com>
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
 * d3   - srcStride */
function x265_blockcopy_pp_16x16_neon
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    add             r2, r2, r3
    add             r0, r0, r1
    vld1.8          {q0}, [r2]
    vst1.8          {q0}, [r0]
    bx              lr
endfunc
