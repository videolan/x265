/*****************************************************************************
 * Copyright (C) 2020 MulticoreWare, Inc
 *
 * Authors: Hongbin Liu <liuhongbin1@huawei.com>
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

.section .rodata

.align 4

.text

.macro pixel_avg_pp_4xN_neon h
function x265_pixel_avg_pp_4x\h\()_neon
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
function x265_pixel_avg_pp_8x\h\()_neon
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
