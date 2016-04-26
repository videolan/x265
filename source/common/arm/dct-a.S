/*****************************************************************************
 * Copyright (C) 2016 x265 project
 *
 * Authors: Min Chen <chenm003@163.com>
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

.align 4

//        dst[0 * line] = ((64 * E[0] + 64 * E[1] + add) >> shift);
//        dst[2 * line] = ((64 * E[0] - 64 * E[1] + add) >> shift);
//        dst[1 * line] = ((83 * O[0] + 36 * O[1] + add) >> shift);
//        dst[3 * line] = ((36 * O[0] - 83 * O[1] + add) >> shift);

/* void dct4_c(const int16_t* src, int16_t* dst, intptr_t srcStride) */
function x265_dct_4x4_neon
    mov             r2, r2, lsl #1
    vld1.16         {d0}, [r0, :64], r2                     // d0  = [03 02 01 00]
    vld1.16         {d1}, [r0, :64], r2                     // d1  = [13 12 11 10]
    vld1.16         {d2}, [r0, :64], r2                     // d2  = [23 22 21 20]
    vld1.16         {d3}, [r0, :64]                         // d3  = [33 32 31 30]

    vtrn.32         q0, q1                                  // q0  = [31 30 11 10 21 20 01 00], q1 = [33 32 13 12 23 22 03 02]
    vrev32.16       q1, q1                                  // q1  = [32 33 12 13 22 23 02 03]

    movconst        r0, 0x00240053
    movconst        r2, 0xFFAD0024

    // DCT-1D
    vadd.s16        q2, q0, q1                              // q2  = [E31 E30 E11 E10 E21 E20 E01 E00]
    vsub.s16        q3, q0, q1                              // q3  = [O31 O30 O11 O10 O21 O20 O01 O00]
    vdup.32         d16, r0                                 // d16 = [ 36  83]
    vdup.32         d17, r2                                 // d17 = [-83  36]
    vtrn.16         d4, d5                                  // d4  = [E30 E20 E10 E00], d5 = [E31 E21 E11 E01]
    vtrn.32         d6, d7                                  // q3  = [O31 O30 O21 O20 O11 O10 O01 O00]

    vmull.s16       q9, d6, d16
    vmull.s16       q10, d7, d16                            // [q9, q10] = [ 36*O1 83*O0] -> [1]
    vmull.s16       q11, d6, d17
    vmull.s16       q12, d7, d17                            // [q11,q12] = [-83*O1 36*O0] -> [3]

    vadd.s16        d0, d4, d5                              // d0 = [E0 + E1]
    vsub.s16        d1, d4, d5                              // d1 = [E0 - E1]

    vpadd.s32       d18, d18, d19                           // q9  = [1]
    vpadd.s32       d19, d20, d21
    vpadd.s32       d20, d22, d23                           // q10 = [3]
    vpadd.s32       d21, d24, d25

    vshll.s16       q1, d0, #6                              // q1  = 64 * [0]
    vshll.s16       q2, d1, #6                              // q2  = 64 * [2]

    // TODO: Dynamic Range is 11+6-1 bits
    vqrshrn.s32     d25, q9, 1                              // d25 = R[13 12 11 10]
    vqrshrn.s32     d24, q1, 1                              // d24 = R[03 02 01 00]
    vqrshrn.s32     d26, q2, 1                              // q26 = R[23 22 21 20]
    vqrshrn.s32     d27, q10, 1                             // d27 = R[33 32 31 30]


    // DCT-2D
    vmovl.s16       q0, d16                                // q14 = [ 36  83]

    vtrn.32         q12, q13                                // q12 = [31 30 11 10 21 20 01 00], q13 = [33 32 13 12 23 22 03 02]
    vrev32.16       q13, q13                                // q13 = [32 33 12 13 22 23 02 03]

    vaddl.s16       q1, d24, d26                            // q0  = [E21 E20 E01 E00]
    vaddl.s16       q2, d25, d27                            // q1  = [E31 E30 E11 E10]
    vsubl.s16       q3, d24, d26                            // q2  = [O21 O20 O01 O00]
    vsubl.s16       q8, d25, d27                            // q3  = [O31 O30 O11 O10]

    vtrn.32         q1, q2                                  // q1  = [E30 E20 E10 E00], q2  = [E31 E21 E11 E01]
    vtrn.32         q3, q8                                  // q3  = [O30 O20 O10 O00], q8  = [O31 O21 O11 O01]

    vmul.s32        q9, q3, d0[0]                           // q9  = [83*O30 83*O20 83*O10 83*O00]
    vmul.s32        q10, q8, d0[1]                          // q10 = [36*O31 36*O21 36*O11 36*O01]
    vmul.s32        q11, q3, d0[1]                          // q11 = [36*O30 36*O20 36*O10 36*O00]
    vmul.s32        q12, q8, d0[0]                          // q12 = [83*O31 83*O21 83*O11 83*O01]

    vadd.s32        q0, q1, q2                              // d0 = [E0 + E1]
    vsub.s32        q1, q1, q2                              // d1 = [E0 - E1]

    vadd.s32        q9, q9, q10
    vsub.s32        q10, q11, q12

    vshl.s32        q0, q0, #6                              // q1  = 64 * [0]
    vshl.s32        q1, q1, #6                              // q2  = 64 * [2]

    vqrshrn.s32     d25, q9, 8                              // d25 = R[13 12 11 10]
    vqrshrn.s32     d27, q10, 8                             // d27 = R[33 32 31 30]

    vqrshrn.s32     d24, q0, 8                              // d24 = R[03 02 01 00]
    vqrshrn.s32     d26, q1, 8                              // q26 = R[23 22 21 20]

    vst1.16         {d24-d27}, [r1]

    bx              lr
endfunc

