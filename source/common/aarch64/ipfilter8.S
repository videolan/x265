/*****************************************************************************
 * Copyright (C) 2020 MulticoreWare, Inc
 *
 * Authors: Yimeng Su <yimeng.su@huawei.com>
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



.macro qpel_filter_0_32b
    movi            v24.8h, #64
    uxtl            v19.8h, v5.8b
    smull           v17.4s, v19.4h, v24.4h
    smull2          v18.4s, v19.8h, v24.8h
.endm

.macro qpel_filter_1_32b
    movi            v16.8h, #58
    uxtl            v19.8h, v5.8b
    smull           v17.4s, v19.4h, v16.4h
    smull2          v18.4s, v19.8h, v16.8h

    movi            v24.8h, #10
    uxtl            v21.8h, v1.8b
    smull           v19.4s, v21.4h, v24.4h
    smull2          v20.4s, v21.8h, v24.8h

    movi            v16.8h, #17
    uxtl            v23.8h, v2.8b
    smull           v21.4s, v23.4h, v16.4h
    smull2          v22.4s, v23.8h, v16.8h

    movi            v24.8h, #5
    uxtl            v1.8h, v6.8b
    smull           v23.4s, v1.4h, v24.4h
    smull2          v16.4s, v1.8h, v24.8h

    sub             v17.4s, v17.4s, v19.4s
    sub             v18.4s, v18.4s, v20.4s

    uxtl            v1.8h, v4.8b
    sshll           v19.4s, v1.4h, #2
    sshll2          v20.4s, v1.8h, #2

    add             v17.4s, v17.4s, v21.4s
    add             v18.4s, v18.4s, v22.4s

    uxtl            v1.8h, v0.8b
    uxtl            v2.8h, v3.8b
    ssubl           v21.4s, v2.4h, v1.4h
    ssubl2          v22.4s, v2.8h, v1.8h

    add             v17.4s, v17.4s, v19.4s
    add             v18.4s, v18.4s, v20.4s
    sub             v21.4s, v21.4s, v23.4s
    sub             v22.4s, v22.4s, v16.4s
    add             v17.4s, v17.4s, v21.4s
    add             v18.4s, v18.4s, v22.4s
.endm

.macro qpel_filter_2_32b
    movi            v16.4s, #11
    uxtl            v19.8h, v5.8b
    uxtl            v20.8h, v2.8b
    saddl           v17.4s, v19.4h, v20.4h
    saddl2          v18.4s, v19.8h, v20.8h

    uxtl            v21.8h, v1.8b
    uxtl            v22.8h, v6.8b
    saddl           v19.4s, v21.4h, v22.4h
    saddl2          v20.4s, v21.8h, v22.8h

    mul             v19.4s, v19.4s, v16.4s
    mul             v20.4s, v20.4s, v16.4s

    movi            v16.4s, #40
    mul             v17.4s, v17.4s, v16.4s
    mul             v18.4s, v18.4s, v16.4s

    uxtl            v21.8h, v4.8b
    uxtl            v22.8h, v3.8b
    saddl           v23.4s, v21.4h, v22.4h
    saddl2          v16.4s, v21.8h, v22.8h

    uxtl            v1.8h, v0.8b
    uxtl            v2.8h, v7.8b
    saddl           v21.4s, v1.4h, v2.4h
    saddl2          v22.4s, v1.8h, v2.8h

    shl             v23.4s, v23.4s, #2
    shl             v16.4s, v16.4s, #2

    add             v19.4s, v19.4s, v21.4s
    add             v20.4s, v20.4s, v22.4s
    add             v17.4s, v17.4s, v23.4s
    add             v18.4s, v18.4s, v16.4s
    sub             v17.4s, v17.4s, v19.4s
    sub             v18.4s, v18.4s, v20.4s
.endm

.macro qpel_filter_3_32b
    movi            v16.8h, #17
    movi            v24.8h, #5

    uxtl            v19.8h, v5.8b
    smull           v17.4s, v19.4h, v16.4h
    smull2          v18.4s, v19.8h, v16.8h

    uxtl            v21.8h, v1.8b
    smull           v19.4s, v21.4h, v24.4h
    smull2          v20.4s, v21.8h, v24.8h

    movi            v16.8h, #58
    uxtl            v23.8h, v2.8b
    smull           v21.4s, v23.4h, v16.4h
    smull2          v22.4s, v23.8h, v16.8h

    movi            v24.8h, #10
    uxtl            v1.8h, v6.8b
    smull           v23.4s, v1.4h, v24.4h
    smull2          v16.4s, v1.8h, v24.8h

    sub             v17.4s, v17.4s, v19.4s
    sub             v18.4s, v18.4s, v20.4s

    uxtl            v1.8h, v3.8b
    sshll           v19.4s, v1.4h, #2
    sshll2          v20.4s, v1.8h, #2

    add             v17.4s, v17.4s, v21.4s
    add             v18.4s, v18.4s, v22.4s

    uxtl            v1.8h, v4.8b
    uxtl            v2.8h, v7.8b
    ssubl           v21.4s, v1.4h, v2.4h
    ssubl2          v22.4s, v1.8h, v2.8h

    add             v17.4s, v17.4s, v19.4s
    add             v18.4s, v18.4s, v20.4s
    sub             v21.4s, v21.4s, v23.4s
    sub             v22.4s, v22.4s, v16.4s
    add             v17.4s, v17.4s, v21.4s
    add             v18.4s, v18.4s, v22.4s
.endm




.macro vextin8
    ld1             {v3.16b}, [x11], #16
    mov             v7.d[0], v3.d[1]
    ext             v0.8b, v3.8b, v7.8b, #1
    ext             v4.8b, v3.8b, v7.8b, #2
    ext             v1.8b, v3.8b, v7.8b, #3
    ext             v5.8b, v3.8b, v7.8b, #4
    ext             v2.8b, v3.8b, v7.8b, #5
    ext             v6.8b, v3.8b, v7.8b, #6
    ext             v3.8b, v3.8b, v7.8b, #7
.endm



// void interp_horiz_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
.macro HPS_FILTER a b filterhps
    mov             w12, #8192
    mov             w6, w10
    sub             x3, x3, #\a
    lsl             x3, x3, #1
    mov             w9, #\a
    cmp             w9, #4
    b.eq            14f
    cmp             w9, #12
    b.eq            15f
    b               7f
14:
    HPS_FILTER_4 \a \b \filterhps
    b               10f
15:
    HPS_FILTER_12 \a \b \filterhps
    b               10f
7:
    cmp             w5, #0
    b.eq            8f
    cmp             w5, #1
    b.eq            9f
8:
loop1_hps_\filterhps\()_\a\()x\b\()_rowext0:
    mov             w7, #\a
    lsr             w7, w7, #3
    mov             x11, x0
    sub             x11, x11, #4
loop2_hps_\filterhps\()_\a\()x\b\()_rowext0:
    vextin8
    \filterhps
    dup             v16.4s, w12
    sub             v17.4s, v17.4s, v16.4s
    sub             v18.4s, v18.4s, v16.4s
    xtn             v0.4h, v17.4s
    xtn2            v0.8h, v18.4s
    st1             {v0.8h}, [x2], #16
    subs            w7, w7, #1
    sub             x11, x11, #8
    b.ne            loop2_hps_\filterhps\()_\a\()x\b\()_rowext0
    subs            w6, w6, #1
    add             x0, x0, x1
    add             x2, x2, x3
    b.ne            loop1_hps_\filterhps\()_\a\()x\b\()_rowext0
    b               10f
9:
loop3_hps_\filterhps\()_\a\()x\b\()_rowext1:
    mov             w7, #\a
    lsr             w7, w7, #3
    mov             x11, x0
    sub             x11, x11, #4
loop4_hps_\filterhps\()_\a\()x\b\()_rowext1:
    vextin8
    \filterhps
    dup             v16.4s, w12
    sub             v17.4s, v17.4s, v16.4s
    sub             v18.4s, v18.4s, v16.4s
    xtn             v0.4h, v17.4s
    xtn2            v0.8h, v18.4s
    st1             {v0.8h}, [x2], #16
    subs            w7, w7, #1
    sub             x11, x11, #8
    b.ne            loop4_hps_\filterhps\()_\a\()x\b\()_rowext1
    subs            w6, w6, #1
    add             x0, x0, x1
    add             x2, x2, x3
    b.ne            loop3_hps_\filterhps\()_\a\()x\b\()_rowext1
10:
.endm

.macro HPS_FILTER_4 w h filterhps
    cmp             w5, #0
    b.eq            11f
    cmp             w5, #1
    b.eq            12f
11:
loop4_hps_\filterhps\()_\w\()x\h\()_rowext0:
    mov             x11, x0
    sub             x11, x11, #4
    vextin8
    \filterhps
    dup             v16.4s, w12
    sub             v17.4s, v17.4s, v16.4s
    xtn             v0.4h, v17.4s
    st1             {v0.4h}, [x2], #8
    sub             x11, x11, #8
    subs            w6, w6, #1
    add             x0, x0, x1
    add             x2, x2, x3
    b.ne            loop4_hps_\filterhps\()_\w\()x\h\()_rowext0
    b               13f
12:
loop5_hps_\filterhps\()_\w\()x\h\()_rowext1:
    mov             x11, x0
    sub             x11, x11, #4
    vextin8
    \filterhps
    dup             v16.4s, w12
    sub             v17.4s, v17.4s, v16.4s
    xtn             v0.4h, v17.4s
    st1             {v0.4h}, [x2], #8
    sub             x11, x11, #8
    subs            w6, w6, #1
    add             x0, x0, x1
    add             x2, x2, x3
    b.ne            loop5_hps_\filterhps\()_\w\()x\h\()_rowext1
13:
.endm

.macro HPS_FILTER_12 w h filterhps
    cmp             w5, #0
    b.eq            14f
    cmp             w5, #1
    b.eq            15f
14:
loop12_hps_\filterhps\()_\w\()x\h\()_rowext0:
    mov             x11, x0
    sub             x11, x11, #4
    vextin8
    \filterhps
    dup             v16.4s, w12
    sub             v17.4s, v17.4s, v16.4s
    sub             v18.4s, v18.4s, v16.4s
    xtn             v0.4h, v17.4s
    xtn2            v0.8h, v18.4s
    st1             {v0.8h}, [x2], #16
    sub             x11, x11, #8

    vextin8
    \filterhps
    dup             v16.4s, w12
    sub             v17.4s, v17.4s, v16.4s
    xtn             v0.4h, v17.4s
    st1             {v0.4h}, [x2], #8
    add             x2, x2, x3
    subs            w6, w6, #1
    add             x0, x0, x1
    b.ne            loop12_hps_\filterhps\()_\w\()x\h\()_rowext0
    b               16f
15:
loop12_hps_\filterhps\()_\w\()x\h\()_rowext1:
    mov             x11, x0
    sub             x11, x11, #4
    vextin8
    \filterhps
    dup             v16.4s, w12
    sub             v17.4s, v17.4s, v16.4s
    sub             v18.4s, v18.4s, v16.4s
    xtn             v0.4h, v17.4s
    xtn2            v0.8h, v18.4s
    st1             {v0.8h}, [x2], #16
    sub             x11, x11, #8

    vextin8
    \filterhps
    dup             v16.4s, w12
    sub             v17.4s, v17.4s, v16.4s
    xtn             v0.4h, v17.4s
    st1             {v0.4h}, [x2], #8
    add             x2, x2, x3
    subs            w6, w6, #1
    add             x0, x0, x1
    b.ne            loop12_hps_\filterhps\()_\w\()x\h\()_rowext1
16:
.endm

// void interp_horiz_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
.macro LUMA_HPS w h
function x265_interp_8tap_horiz_ps_\w\()x\h\()_neon
    mov             w10, #\h
    cmp             w5, #0
    b.eq            6f
    sub             x0, x0, x1, lsl #2

    add             x0, x0, x1
    add             w10, w10, #7
6:
    cmp             w4, #0
    b.eq            0f
    cmp             w4, #1
    b.eq            1f
    cmp             w4, #2
    b.eq            2f
    cmp             w4, #3
    b.eq            3f
0:
    HPS_FILTER  \w \h qpel_filter_0_32b
    b               5f
1:
    HPS_FILTER  \w \h qpel_filter_1_32b
    b               5f
2:
    HPS_FILTER  \w \h qpel_filter_2_32b
    b               5f
3:
    HPS_FILTER  \w \h qpel_filter_3_32b
    b               5f
5:
    ret
endfunc
.endm

LUMA_HPS    4 4
LUMA_HPS    4 8
LUMA_HPS    4 16
LUMA_HPS    8 4
LUMA_HPS    8 8
LUMA_HPS    8 16
LUMA_HPS    8 32
LUMA_HPS    12 16
LUMA_HPS    16 4
LUMA_HPS    16 8
LUMA_HPS    16 12
LUMA_HPS    16 16
LUMA_HPS    16 32
LUMA_HPS    16 64
LUMA_HPS    24 32
LUMA_HPS    32 8
LUMA_HPS    32 16
LUMA_HPS    32 24
LUMA_HPS    32 32
LUMA_HPS    32 64
LUMA_HPS    48 64
LUMA_HPS    64 16
LUMA_HPS    64 32
LUMA_HPS    64 48
LUMA_HPS    64 64
