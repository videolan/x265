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

.macro SAD_X_START_8 x
    ld1             {v0.8b}, [x0], x9
.if \x == 3
    ld1             {v1.8b}, [x1], x4
    ld1             {v2.8b}, [x2], x4
    ld1             {v3.8b}, [x3], x4
.elseif \x == 4
    ld1             {v1.8b}, [x1], x5
    ld1             {v2.8b}, [x2], x5
    ld1             {v3.8b}, [x3], x5
    ld1             {v4.8b}, [x4], x5
.endif
    uabdl           v16.8h, v0.8b, v1.8b
    uabdl           v17.8h, v0.8b, v2.8b
    uabdl           v18.8h, v0.8b, v3.8b
.if \x == 4
    uabdl           v19.8h, v0.8b, v4.8b
.endif
.endm

.macro SAD_X_8 x
    ld1             {v0.8b}, [x0], x9
.if \x == 3
    ld1             {v1.8b}, [x1], x4
    ld1             {v2.8b}, [x2], x4
    ld1             {v3.8b}, [x3], x4
.elseif \x == 4
    ld1             {v1.8b}, [x1], x5
    ld1             {v2.8b}, [x2], x5
    ld1             {v3.8b}, [x3], x5
    ld1             {v4.8b}, [x4], x5
.endif
    uabal           v16.8h, v0.8b, v1.8b
    uabal           v17.8h, v0.8b, v2.8b
    uabal           v18.8h, v0.8b, v3.8b
.if \x == 4
    uabal           v19.8h, v0.8b, v4.8b
.endif
.endm

.macro SAD_X_8xN x, h
function x265_sad_x\x\()_8x\h\()_neon
    mov             x9, #FENC_STRIDE
    SAD_X_START_8 \x
.rept \h - 1
    SAD_X_8 \x
.endr
    uaddlv          s0, v16.8h
    uaddlv          s1, v17.8h
    uaddlv          s2, v18.8h
.if \x == 4
    uaddlv          s3, v19.8h
.endif

.if \x == 3
    stp             s0, s1, [x5]
    str             s2, [x5, #8]
.elseif \x == 4
    stp             s0, s1, [x6]
    stp             s2, s3, [x6, #8]
.endif
    ret
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
