/*****************************************************************************
 * Copyright (C) 2016 x265 project
 *
 * Authors:David Conrad <lessen42@gmail.com>
 *          Janne Grunau <janne-x264@jannau.net>
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

/* sad4x4(pixel* dst, intptr_t dstStride, const pixel* src, intptr_t srcStride)
 *
 * r0   - dst
 * r1   - dstStride
 * r2   - src
 * d3   - srcStride */

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

