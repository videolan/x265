/*****************************************************************************
 * Copyright (C) 2013-2017 MulticoreWare, Inc
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

/* uses registers q4 - q7 for temp values */
.macro tr4 r0, r1, r2, r3
    vsub.s32    q8, \r0, \r3    // EO0
    vadd.s32    q9, \r0, \r3    // EE0
    vadd.s32    q10, \r1, \r2   // EE1
    vsub.s32    q11, \r1, \r2   // EO1

    vmul.s32    \r1, q8, d0[0]  // 83 * EO0
    vmul.s32    \r3, q8, d0[1]  // 36 * EO0
    vshl.s32    q9, q9, #6      // 64 * EE0
    vshl.s32    q10, q10, #6    // 64 * EE1
    vmla.s32    \r1, q11, d0[1] // 83 * EO0 + 36 * EO1
    vmls.s32    \r3, q11, d0[0] // 36 * EO0 - 83 * EO1
    vadd.s32    \r0, q9, q10    // 64 * (EE0 + EE1)
    vsub.s32    \r2, q9, q10    // 64 * (EE0 - EE1)
.endm


.macro tr8 r0, r1, r2, r3
    vmul.s32  q12, \r0, d1[1]   //  89 * src1
    vmul.s32  q13, \r0, d1[0]   //  75 * src1
    vmul.s32  q14, \r0, d2[1]   //  50 * src1
    vmul.s32  q15, \r0, d2[0]   //  18 * src1

    vmla.s32  q12, \r1, d1[0]   //  75 * src3
    vmls.s32  q13, \r1, d2[0]   // -18 * src3
    vmls.s32  q14, \r1, d1[1]   // -89 * src3
    vmls.s32  q15, \r1, d2[1]   // -50 * src3

    vmla.s32  q12, \r2, d2[1]   //  50 * src5
    vmls.s32  q13, \r2, d1[1]   // -89 * src5
    vmla.s32  q14, \r2, d2[0]   //  18 * src5
    vmla.s32  q15, \r2, d1[0]   //  75 * src5

    vmla.s32  q12, \r3, d2[0]   //  18 * src7
    vmls.s32  q13, \r3, d2[1]   // -50 * src7
    vmla.s32  q14, \r3, d1[0]   //  75 * src7
    vmls.s32  q15, \r3, d1[1]   // -89 * src7
.endm


// TODO: in the DCT-2D stage, I spending 4x8=32 LD/ST operators because I haven't temporary buffer
/* void dct8_c(const int16_t* src, int16_t* dst, intptr_t srcStride) */
function x265_dct_8x8_neon
    vpush {q4-q7}

    mov r2, r2, lsl #1

    adr r3, ctr4
    vld1.16 {d0-d2}, [r3]
    mov r3, r1

    // DCT-1D
    // top half
    vld1.16 {q12}, [r0], r2
    vld1.16 {q13}, [r0], r2
    vld1.16 {q14}, [r0], r2
    vld1.16 {q15}, [r0], r2

    TRANSPOSE4x4x2_16 d24, d26, d28, d30,  d25, d27, d29, d31

    // |--|
    // |24|
    // |26|
    // |28|
    // |30|
    // |25|
    // |27|
    // |29|
    // |31|
    // |--|

    vaddl.s16 q4, d28, d27
    vaddl.s16 q5, d30, d25
    vaddl.s16 q2, d24, d31
    vaddl.s16 q3, d26, d29

    tr4 q2, q3, q4, q5

    vqrshrn.s32 d20, q3, 2
    vqrshrn.s32 d16, q2, 2
    vqrshrn.s32 d17, q4, 2
    vqrshrn.s32 d21, q5, 2

    vsubl.s16 q2, d24, d31
    vsubl.s16 q3, d26, d29
    vsubl.s16 q4, d28, d27
    vsubl.s16 q5, d30, d25

    tr8 q2, q3, q4, q5

    vqrshrn.s32 d18, q12, 2
    vqrshrn.s32 d22, q13, 2
    vqrshrn.s32 d19, q14, 2
    vqrshrn.s32 d23, q15, 2
    vstm r1!, {d16-d23}
    // bottom half
    vld1.16 {q12}, [r0], r2
    vld1.16 {q13}, [r0], r2
    vld1.16 {q14}, [r0], r2
    vld1.16 {q15}, [r0], r2
    mov r2, #8*2

    TRANSPOSE4x4x2_16 d24, d26, d28, d30,  d25, d27, d29, d31

    // |--|
    // |24|
    // |26|
    // |28|
    // |30|
    // |25|
    // |27|
    // |29|
    // |31|
    // |--|

    vaddl.s16 q4, d28, d27
    vaddl.s16 q5, d30, d25
    vaddl.s16 q2, d24, d31
    vaddl.s16 q3, d26, d29

    tr4 q2, q3, q4, q5

    vqrshrn.s32 d20, q3, 2
    vqrshrn.s32 d16, q2, 2
    vqrshrn.s32 d17, q4, 2
    vqrshrn.s32 d21, q5, 2

    vsubl.s16 q2, d24, d31
    vsubl.s16 q3, d26, d29
    vsubl.s16 q4, d28, d27
    vsubl.s16 q5, d30, d25

    tr8 q2, q3, q4, q5

    vqrshrn.s32 d18, q12, 2
    vqrshrn.s32 d22, q13, 2
    vqrshrn.s32 d19, q14, 2
    vqrshrn.s32 d23, q15, 2
    vstm r1, {d16-d23}
    mov r1, r3
    // DCT-2D
    // left half
    vld1.16 {d24}, [r1], r2
    vld1.16 {d26}, [r1], r2
    vld1.16 {d28}, [r1], r2
    vld1.16 {d30}, [r1], r2
    vld1.16 {d25}, [r1], r2
    vld1.16 {d27}, [r1], r2
    vld1.16 {d29}, [r1], r2
    vld1.16 {d31}, [r1], r2
    mov r1, r3

    TRANSPOSE4x4x2_16 d24, d26, d28, d30,  d25, d27, d29, d31

    // |--|
    // |24|
    // |26|
    // |28|
    // |30|
    // |25|
    // |27|
    // |29|
    // |31|
    // |--|

    vaddl.s16 q4, d28, d27
    vaddl.s16 q5, d30, d25
    vaddl.s16 q2, d24, d31
    vaddl.s16 q3, d26, d29

    tr4 q2, q3, q4, q5

    vqrshrn.s32 d18, q3, 9
    vqrshrn.s32 d16, q2, 9
    vqrshrn.s32 d20, q4, 9
    vqrshrn.s32 d22, q5, 9

    vsubl.s16 q2, d24, d31
    vsubl.s16 q3, d26, d29
    vsubl.s16 q4, d28, d27
    vsubl.s16 q5, d30, d25

    tr8 q2, q3, q4, q5

    vqrshrn.s32 d17, q12, 9
    vqrshrn.s32 d19, q13, 9
    vqrshrn.s32 d21, q14, 9
    vqrshrn.s32 d23, q15, 9

    add r3, #8
    vst1.16 {d16}, [r1], r2
    vst1.16 {d17}, [r1], r2
    vst1.16 {d18}, [r1], r2
    vst1.16 {d19}, [r1], r2
    vst1.16 {d20}, [r1], r2
    vst1.16 {d21}, [r1], r2
    vst1.16 {d22}, [r1], r2
    vst1.16 {d23}, [r1], r2
    mov r1, r3


    // right half
    vld1.16 {d24}, [r1], r2
    vld1.16 {d26}, [r1], r2
    vld1.16 {d28}, [r1], r2
    vld1.16 {d30}, [r1], r2
    vld1.16 {d25}, [r1], r2
    vld1.16 {d27}, [r1], r2
    vld1.16 {d29}, [r1], r2
    vld1.16 {d31}, [r1], r2
    mov r1, r3

    TRANSPOSE4x4x2_16 d24, d26, d28, d30,  d25, d27, d29, d31

    // |--|
    // |24|
    // |26|
    // |28|
    // |30|
    // |25|
    // |27|
    // |29|
    // |31|
    // |--|

    vaddl.s16 q4, d28, d27
    vaddl.s16 q5, d30, d25
    vaddl.s16 q2, d24, d31
    vaddl.s16 q3, d26, d29

    tr4 q2, q3, q4, q5

    vqrshrn.s32 d18, q3, 9
    vqrshrn.s32 d16, q2, 9
    vqrshrn.s32 d20, q4, 9
    vqrshrn.s32 d22, q5, 9

    vsubl.s16 q2, d24, d31
    vsubl.s16 q3, d26, d29
    vsubl.s16 q4, d28, d27
    vsubl.s16 q5, d30, d25

    tr8 q2, q3, q4, q5

    vqrshrn.s32 d17, q12, 9
    vqrshrn.s32 d19, q13, 9
    vqrshrn.s32 d21, q14, 9
    vqrshrn.s32 d23, q15, 9

    vst1.16 {d16}, [r1], r2
    vst1.16 {d17}, [r1], r2
    vst1.16 {d18}, [r1], r2
    vst1.16 {d19}, [r1], r2
    vst1.16 {d20}, [r1], r2
    vst1.16 {d21}, [r1], r2
    vst1.16 {d22}, [r1], r2
    vst1.16 {d23}, [r1], r2

    vpop {q4-q7}
    bx lr
endfunc


.align 8
pw_tr16: .hword 90, 87, 80, 70,  57, 43, 25,  9     // q0 = [ 9 25 43 57 70 80 87 90]
         .hword 83, 36, 75, 89,  18, 50, 00, 00     // q1 = [ x  x 50 18 89 75 36 83]

.align 8
ctr4:
    .word 83            // d0[0] = 83
    .word 36            // d0[1] = 36
ctr8:
    .word 75            // d1[0] = 75
    .word 89            // d1[1] = 89
    .word 18            // d2[0] = 18
    .word 50            // d2[1] = 50
ctr16:
    .word 90, 87        // d0
    .word 80, 70        // d1
    .word 57, 43        // d2
    .word 25,  9        // d3

/* void dct16_c(const int16_t* src, int16_t* dst, intptr_t srcStride) */
function x265_dct_16x16_neon
    push {lr}

    // fill 3 of pipeline stall cycles (dependency link on SP)
    add r2, r2
    adr r3, pw_tr16
    mov r12, #16/4

    vpush {q4-q7}

    // TODO: 16x16 transpose buffer (may share with input buffer in future)
    sub sp, #16*16*2

    vld1.16 {d0-d3}, [r3]
    mov r3, sp
    mov lr, #4*16*2

    // DCT-1D
.loop1:
    // Row[0-3]
    vld1.16 {q8-q9}, [r0, :64], r2      // q8  = [07 06 05 04 03 02 01 00], q9  = [0F 0E 0D 0C 0B 0A 09 08]
    vld1.16 {q10-q11}, [r0, :64], r2    // q10 = [17 16 15 14 13 12 11 10], q11 = [1F 1E 1D 1C 1B 1A 19 18]
    vld1.16 {q12-q13}, [r0, :64], r2    // q12 = [27 26 25 24 23 22 21 20], q13 = [2F 2E 2D 2C 2B 2A 29 28]
    vld1.16 {q14-q15}, [r0, :64], r2    // q14 = [37 36 35 34 33 32 31 30], q15 = [3F 3E 3D 3C 3B 3A 39 38]

    // Register map
    // | 16 17 18 19 |
    // | 20 21 22 23 |
    // | 24 25 26 27 |
    // | 28 29 30 31 |

    // Transpose 16x4
    vtrn.32 q8, q12                     // q8  = [25 24 05 04 21 20 01 00], q12 = [27 26 07 06 23 22 03 02]
    vtrn.32 q10, q14                    // q10 = [35 34 15 14 31 30 11 10], q14 = [37 36 17 16 33 32 13 12]
    vtrn.32 q9, q13                     // q9  = [2D 2C 0D 0C 29 28 09 08], q13 = [2F 2E 0F 0E 2B 2A 0B 0A]
    vtrn.32 q11, q15                    // q11 = [3D 3C 1D 1C 39 38 19 18], q15 = [3F 3E 1F 1E 3B 3A 1B 1A]

    vtrn.16 q8, q10                     // q8  = [34 24 14 04 30 20 10 00], q10 = [35 25 15 05 31 21 11 01]
    vtrn.16 q12, q14                    // q12 = [36 26 16 06 32 22 12 02], q14 = [37 27 17 07 33 23 13 03]
    vtrn.16 q13, q15                    // q13 = [3E 2E 1E 0E 3A 2A 1A 0A], q15 = [3F 2F 1F 0F 3B 2B 1B 0B]
    vtrn.16 q9, q11                     // q9  = [3C 2C 1C 0C 38 28 18 08], q11 = [3D 2D 1D 0D 39 29 19 09]

    vswp d26, d27                       // q13 = [3A 2A 1A 0A 3E 2E 1E 0E]
    vswp d30, d31                       // q15 = [3B 2B 1B 0B 3F 2F 1F 0F]
    vswp d18, d19                       // q9  = [38 28 18 08 3C 2C 1C 0C]
    vswp d22, d23                       // q11 = [39 29 19 09 3D 2D 1D 0D]

    // E[0-7] - 10 bits
    vadd.s16 q4, q8, q15                // q4  = [E4 E0]
    vadd.s16 q5, q10, q13               // q5  = [E5 E1]
    vadd.s16 q6, q12, q11               // q6  = [E6 E2]
    vadd.s16 q7, q14, q9                // q7  = [E7 E3]

    // O[0-7] - 10 bits
    vsub.s16 q8, q8, q15                // q8  = [O4 O0]
    vsub.s16 q9, q14, q9                // q9  = [O7 O3]
    vsub.s16 q10, q10, q13              // q10 = [O5 O1]
    vsub.s16 q11, q12, q11              // q11 = [O6 O2]

    // reorder Ex for EE/EO
    vswp d9, d14                        // q4  = [E3 E0], q7  = [E7 E4]
    vswp d11, d12                       // q5  = [E2 E1], q6  = [E6 E5]
    vswp d14, d15                       // q7  = [E4 E7]
    vswp d12, d13                       // q6  = [E5 E6]

    // EE[0-3] - 11 bits
    vadd.s16 q2, q4, q7                 // q2  = [EE3 EE0]
    vadd.s16 q3, q5, q6                 // q3  = [EE2 EE1]

    // EO[0-3] - 11 bits
    vsub.s16 q4, q4, q7                 // q4  = [EO3 EO0]
    vsub.s16 q5, q5, q6                 // q5  = [EO2 EO1]

    // EEx[0-1] - 12 bits
    vadd.s16 d12, d4, d5                // q6  = [EEE1 EEE0]
    vadd.s16 d13, d6, d7
    vsub.s16 d14, d4, d5                // q7  = [EEO1 EEO0]
    vsub.s16 d15, d6, d7

    // NEON Register map
    // Ex -> [q4, q5, q6, q7], Ox -> [q8, q9, q10, q11], Const -> [q0, q1], Free -> [q2, q3, q12, q13, q14, q15]

    // ODD[4,12]
    vmull.s16 q14, d14, d2[0]           // q14 = EEO0 * 83
    vmull.s16 q15, d14, d2[1]           // q15 = EEO0 * 36
    vmlal.s16 q14, d15, d2[1]           // q14+= EEO1 * 36
    vmlsl.s16 q15, d15, d2[0]           // q15+= EEO1 *-83

    vadd.s16 d4, d12, d13               // d4  = (EEE0 + EEE1)
    vsub.s16 d12, d13                   // d12 = (EEE0 - EEE1)

    // Row
    vmull.s16 q12, d16, d0[0]           // q12 =  O0 * 90
    vmull.s16 q13, d8, d2[3]            // q13 = EO0 * 89
    vqrshrn.s32 d14, q14, 3
    vqrshrn.s32 d15, q15, 3             // q7  = [12 4]     -> [12  4]
    vmull.s16 q14, d16, d0[1]           // q14 =  O0 * 87
    vmull.s16 q15, d16, d0[2]           // q15 =  O0 * 80
    vshll.s16 q2, d4, #6                // q2  = (EEE0 + EEE1) * 64 -> [ 0]
    vshll.s16 q6, d12, #6               // q6  = (EEE0 - EEE1) * 64 -> [ 8]

    vmlal.s16 q12, d20, d0[1]           // q12+=  O1 * 87
    vmlal.s16 q13, d10, d2[2]           // q13+= EO1 * 75
    vmlal.s16 q14, d20, d1[0]           // q14+=  O1 * 57
    vmlal.s16 q15, d20, d1[3]           // q15+=  O1 *  9
    vqrshrn.s32 d4, q2, 3               // q2  = [- 0]
    vqrshrn.s32 d12, q6, 3              // q6  = [- 8]

    vmlal.s16 q12, d22, d0[2]           // q12+=  O2 * 80
    vmlal.s16 q13, d11, d3[1]           // q13+= EO2 * 50
    vmlal.s16 q14, d22, d1[3]           // q14+=  O2 *  9
    vmlsl.s16 q15, d22, d0[3]           // q15+=  O2 *-70

    vmlal.s16 q12, d18, d0[3]           // q12+=  O3 * 70
    vmlal.s16 q13, d9,  d3[0]           // q13+= EO3 * 18   -> [ 2]
    vmlsl.s16 q14, d18, d1[1]           // q14+=  O3 *-43
    vmlsl.s16 q15, d18, d0[1]           // q15+=  O3 *-87

    vmlal.s16 q12, d17, d1[0]           // q12+=  O4 * 57
    vmlsl.s16 q14, d17, d0[2]           // q14+=  O4 *-80
    vmlsl.s16 q15, d17, d1[2]           // q15+=  O4 *-25
    vqrshrn.s32 d6, q13, 3              // q3  = [- 2]
    vmull.s16 q13, d8,  d2[2]           // q13 = EO0 * 75

    vmlal.s16 q12, d21, d1[1]           // q12+=  O5 * 43
    vmlsl.s16 q13, d10, d3[0]           // q13+= EO1 *-18
    vmlsl.s16 q14, d21, d0[0]           // q14+=  O5 *-90
    vmlal.s16 q15, d21, d1[0]           // q15+=  O5 * 57

    vmlal.s16 q12, d23, d1[2]           // q12+=  O6 * 25
    vmlsl.s16 q13, d11, d2[3]           // q13+= EO2 *-89
    vmlsl.s16 q14, d23, d0[3]           // q14+=  O6 *-70
    vmlal.s16 q15, d23, d0[0]           // q15+=  O6 * 90

    vmlal.s16 q12, d19, d1[3]           // q12+=  O7 *  9   -> [ 1]
    vmlsl.s16 q13, d9,  d3[1]           // q13+= EO3 *-50   -> [ 6]
    vmlsl.s16 q14, d19, d1[2]           // q14+=  O7 *-25   -> [ 3]
    vmlal.s16 q15, d19, d1[1]           // q15+=  O7 * 43   -> [ 5]
    vqrshrn.s32 d5, q12, 3              // q2  = [1 0]

    vmull.s16 q12, d16, d0[3]           // q12 =  O0 * 70
    vqrshrn.s32 d7, q14, 3              // q3  = [3 2]
    vmull.s16 q14, d16, d1[0]           // q14 =  O0 * 57

    vmlsl.s16 q12, d20, d1[1]           // q12+=  O1 *-43
    vmlsl.s16 q14, d20, d0[2]           // q14+=  O1 *-80

    vmlsl.s16 q12, d22, d0[1]           // q12+=  O2 *-87
    vmlsl.s16 q14, d22, d1[2]           // q14+=  O2 *-25

    vmlal.s16 q12, d18, d1[3]           // q12+=  O3 *  9
    vmlal.s16 q14, d18, d0[0]           // q14+=  O3 * 90

    // Row[0-3]
    vst4.16 {d4-d7}, [r3], lr

    vqrshrn.s32 d5, q15, 3              // q2  = [5 -]
    vqrshrn.s32 d6, q13, 3              // q3  = [- 6]
    vmull.s16 q13, d8,  d3[1]           // q13 = EO0 * 50
    vmlal.s16 q12, d17, d0[0]           // q12+=  O4 * 90
    vmlsl.s16 q14, d17, d1[3]           // q14+=  O4 *-9
    vmull.s16 q15, d16, d1[1]           // q15 =  O0 * 43

    vmlsl.s16 q13, d10, d2[3]           // q13+= EO1 *-89
    vmlal.s16 q12, d21, d1[2]           // q12+=  O5 * 25
    vmlsl.s16 q14, d21, d0[1]           // q14+=  O5 *-87
    vmlsl.s16 q15, d20, d0[0]           // q15+=  O1 *-90

    vmlal.s16 q13, d11, d3[0]           // q13+= EO2 * 18
    vmlsl.s16 q12, d23, d0[2]           // q12+=  O6 *-80
    vmlal.s16 q14, d23, d1[1]           // q14+=  O6 * 43
    vmlal.s16 q15, d22, d1[0]           // q15+=  O2 * 57

    vmlal.s16 q13, d9,  d2[2]           // q13+= EO3 * 75   -> [10]
    vmlsl.s16 q12, d19, d1[0]           // q12+=  O7 *-57   -> [ 7]
    vmlal.s16 q14, d19, d0[3]           // q14+=  O7 * 70   -> [ 9]
    vmlal.s16 q15, d18, d1[2]           // q15+=  O3 * 25
    vmlsl.s16 q15, d17, d0[1]           // q15+=  O4 *-87
    vmlal.s16 q15, d21, d0[3]           // q15+=  O5 * 70
    vmlal.s16 q15, d23, d1[3]           // q15+=  O6 *  9
    vmlsl.s16 q15, d19, d0[2]           // q15+=  O7 *-80   -> [11]
    vmov d4, d14                        // q2  = [5 4]
    vqrshrn.s32 d14, q13, 3             // q7  = [12 10]
    vmull.s16 q13, d8,  d3[0]           // q13 = EO0 * 18
    vqrshrn.s32 d7, q12, 3              // q3  = [7 6]
    vmull.s16 q12, d16, d1[2]           // q12 =  O0 * 25
    vmlsl.s16 q13, d9,  d2[3]           // q13 = EO3 *-89
    vmull.s16 q4, d16, d1[3]            // q4  =  O0 *  9
    vmlsl.s16 q12, d20, d0[3]           // q12+=  O1 *-70
    vmlsl.s16 q13, d10, d3[1]           // q13 = EO1 *-50
    vmlsl.s16 q4, d20, d1[2]            // q4 +=  O1 *-25
    vmlal.s16 q12, d22, d0[0]           // q12+=  O2 * 90
    vmlal.s16 q13, d11, d2[2]           // q13 = EO2 * 75   -> [14]
    vmlal.s16 q4, d22, d1[1]            // q4 +=  O2 * 43
    vmlsl.s16 q12, d18, d0[2]           // q12+=  O3 *-80
    vmlsl.s16 q4, d18, d1[0]            // q4 +=  O3 *-57
    vmlal.s16 q12, d17, d1[1]           // q12+=  O4 * 43
    vqrshrn.s32 d13, q14, 3             // q6  = [9 8]
    vmov d28, d15                       // q14 = [- 12]
    vqrshrn.s32 d15, q15, 3             // q7  = [11 10]
    vqrshrn.s32 d30, q13, 3             // q15 = [- 14]
    vmlal.s16 q4, d17, d0[3]            // q4 +=  O4 * 70
    vmlal.s16 q12, d21, d1[3]           // q12+=  O5 *  9
    vmlsl.s16 q4, d21, d0[2]            // q4 +=  O5 *-80
    vmlsl.s16 q12, d23, d1[0]           // q12+=  O6 *-57
    vmlal.s16 q4, d23, d0[1]            // q4 +=  O6 * 87
    vmlal.s16 q12, d19, d0[1]           // q12+=  O7 * 87   -> [13]
    vmlsl.s16 q4, d19, d0[0]            // q4 +=  O7 *-90   -> [15]

    // Row[4-7]
    vst4.16 {d4-d7}, [r3], lr
    vqrshrn.s32 d29, q12, 3             // q14 = [13 12]
    vqrshrn.s32 d31, q4, 3              // q15 = [15 14]

    // Row[8-11]
    vst4.16 {d12-d15}, [r3], lr

    // Row[12-15]
    vst4.16 {d28-d31}, [r3]!


    // loop into next process group
    sub r3, #3*4*16*2
    subs r12, #1
    bgt .loop1


    // DCT-2D
    // r[0,2,3,12,lr], q[2-15] are free here
    mov r2, sp                          // r3 -> internal temporary buffer
    mov r3, #16*2*2
    mov r12, #16/4                      // Process 4 rows every loop

.loop2:
    vldm r2, {q8-q15}

    // d16 = [30 20 10 00]
    // d17 = [31 21 11 01]
    // q18 = [32 22 12 02]
    // d19 = [33 23 13 03]
    // d20 = [34 24 14 04]
    // d21 = [35 25 15 05]
    // q22 = [36 26 16 06]
    // d23 = [37 27 17 07]
    // d24 = [38 28 18 08]
    // d25 = [39 29 19 09]
    // q26 = [3A 2A 1A 0A]
    // d27 = [3B 2B 1B 0B]
    // d28 = [3C 2C 1C 0C]
    // d29 = [3D 2D 1D 0D]
    // q30 = [3E 2E 1E 0E]
    // d31 = [3F 2F 1F 0F]

    // NOTE: the ARM haven't enough SIMD registers, so I have to process Even & Odd part series.

    // Process Even

    // E
    vaddl.s16 q2,  d16, d31             // q2  = [E30 E20 E10 E00]
    vaddl.s16 q3,  d17, d30             // q3  = [E31 E21 E11 E01]
    vaddl.s16 q4,  d18, d29             // q4  = [E32 E22 E12 E02]
    vaddl.s16 q5,  d19, d28             // q5  = [E33 E23 E13 E03]
    vaddl.s16 q9,  d23, d24             // q9  = [E37 E27 E17 E07]
    vaddl.s16 q8,  d22, d25             // q8  = [E36 E26 E16 E06]
    vaddl.s16 q7,  d21, d26             // q7  = [E35 E25 E15 E05]
    vaddl.s16 q6,  d20, d27             // q6  = [E34 E24 E14 E04]

    // EE & EO
    vadd.s32 q13, q2, q9                // q13 = [EE30 EE20 EE10 EE00]
    vsub.s32 q9, q2, q9                 // q9  = [EO30 EO20 EO10 EO00]

    vadd.s32 q2, q5, q6                 // q2  = [EE33 EE23 EE13 EE03]
    vsub.s32 q12, q5, q6                // q12 = [EO33 EO23 EO13 EO03]

    vadd.s32 q14, q3, q8                // q14 = [EE31 EE21 EE11 EE01]
    vsub.s32 q10, q3, q8                // q10 = [EO31 EO21 EO11 EO01]

    vadd.s32 q15, q4, q7                // q15 = [EE32 EE22 EE12 EE02]
    vsub.s32 q11, q4, q7                // q11 = [EO32 EO22 EO12 EO02]

    // Free=[3,4,5,6,7,8]

    // EEE & EEO
    vadd.s32 q5, q13, q2                // q5  = [EEE30 EEE20 EEE10 EEE00]
    vadd.s32 q6, q14, q15               // q6  = [EEE31 EEE21 EEE11 EEE01]
    vsub.s32 q7, q13, q2                // q7  = [EEO30 EEO20 EEO10 EEO00]
    vsub.s32 q8, q14, q15               // q8  = [EEO31 EEO21 EEO11 EEO01]

    // Convert Const for Dct EE to 32-bits
    adr r0, ctr4
    vld1.32 {d0-d3}, [r0, :64]

    // Register Map (Qx)
    // Free=[2,3,4,13,14,15], Const=[0,1], EEEx=[5,6,7,8], EO=[9,10,11,12]

    vadd.s32 q15, q5, q6                // q15 = EEE0 + EEE1    ->  0
    vmul.s32 q2, q9, d1[1]              // q2  = EO0 * 89       ->  2
    vmul.s32 q3, q7, d0[0]              // q3  = EEO0 * 83      ->  4
    vmul.s32 q4, q9, d1[0]              // q4  = EO0 * 75       ->  6
    vmul.s32 q14, q9, d2[1]             // q14 = EO0 * 50       -> 10

    vshl.s32 q15, #6                    // q15                  -> [ 0]'
    vmla.s32 q2, q10, d1[0]             // q2 += EO1 * 75
    vmla.s32 q3, q8, d0[1]              // q3 += EEO1 * 36      -> [ 4]'
    vmls.s32 q4, q10, d2[0]             // q4 += EO1 *-18
    vmls.s32 q14, q10, d1[1]            // q14+= EO1 *-89
    vmul.s32 q13, q7, d0[1]             // q13 = EEO0 * 36      -> 12 

    vqrshrn.s32 d30, q15, 10            // d30                  -> [ 0]
    vqrshrn.s32 d31, q3, 10             // d31                  -> [ 4]
    vmls.s32 q4, q11, d1[1]             // q4 += EO2 *-89
    vsub.s32 q3, q5, q6                 // q3  = EEE0 - EEE1    ->  8
    vmla.s32 q2, q11, d2[1]             // q2 += EO2 * 50
    vmla.s32 q14, q11, d2[0]            // q14+= EO2 * 18
    vmls.s32 q13, q8, d0[0]             // q13+= EEO1 *-83      -> [12]'
    vst1.16 {d30}, [r1], r3             // Stroe [ 0]

    vshl.s32 q3, #6                     // q3                   -> [ 8]'
    vmls.s32 q4, q12, d2[1]             // q4 += EO3 *-50       -> [ 6]'
    vmla.s32 q2, q12, d2[0]             // q2 += EO3 * 18       -> [ 2]'
    vqrshrn.s32 d26, q13, 10            // d26                  -> [12]
    vmla.s32 q14, q12, d1[0]            // q14+= EO3 * 75       -> [10]'

    vqrshrn.s32 d30, q3, 10             // d30                  -> [ 8]
    vmul.s32 q3, q9, d2[0]              // q3  = EO0 * 18       -> 14
    vqrshrn.s32 d4, q2, 10              // d4                   -> [ 2]
    vmls.s32 q3, q10, d2[1]             // q3 += EO1 *-50
    vqrshrn.s32 d5, q4, 10              // d30                  -> [ 6]
    vmla.s32 q3, q11, d1[0]             // q3 += EO2 * 75
    vqrshrn.s32 d27, q14, 10            // d27                  -> [10]
    vmls.s32 q3, q12, d1[1]             // q3 += EO3 *-89       -> [14]'

    vst1.16 {d4 }, [r1], r3             // Stroe [ 2]
    vst1.16 {d31}, [r1], r3             // Stroe [ 4]
    vst1.16 {d5 }, [r1], r3             // Stroe [ 6]
    vst1.16 {d30}, [r1], r3             // Stroe [ 8]
    vqrshrn.s32 d30, q3, 10             // d30                  -> [14]
    vst1.16 {d27}, [r1], r3             // Stroe [10]
    vst1.16 {d26}, [r1], r3             // Stroe [12]
    vst1.16 {d30}, [r1], r3             // Stroe [14]

    // Process Odd
    sub r1, #(15*16)*2
    vldm r2!, {q8-q15}

    // d8  = [30 20 10 00]
    // d9  = [31 21 11 01]
    // q10 = [32 22 12 02]
    // d11 = [33 23 13 03]
    // d12 = [34 24 14 04]
    // d13 = [35 25 15 05]
    // q14 = [36 26 16 06]
    // d15 = [37 27 17 07]
    // d16 = [38 28 18 08]
    // d17 = [39 29 19 09]
    // q18 = [3A 2A 1A 0A]
    // d19 = [3B 2B 1B 0B]
    // d20 = [3C 2C 1C 0C]
    // d21 = [3D 2D 1D 0D]
    // q22 = [3E 2E 1E 0E]
    // d23 = [3F 2F 1F 0F]

    // O
    vsubl.s16 q2,  d16, d31             // q2  = [O30 O20 O10 O00]
    vsubl.s16 q3,  d17, d30             // q3  = [O31 O21 O11 O01]
    vsubl.s16 q4,  d18, d29             // q4  = [O32 O22 O12 O02]
    vsubl.s16 q5,  d19, d28             // q5  = [O33 O23 O13 O03]
    vsubl.s16 q9,  d23, d24             // q9  = [O37 O27 O17 O07]
    vsubl.s16 q8,  d22, d25             // q8  = [O36 O26 O16 O06]
    vsubl.s16 q7,  d21, d26             // q7  = [O35 O25 O15 O05]
    vsubl.s16 q6,  d20, d27             // q6  = [O34 O24 O14 O04]

    // Load DCT Ox Constant
    adr r0, ctr16
    vld1.32 {d0-d3}, [r0]

    // Register Map (Qx)
    // Free=[10,11,12,13,14,15], Const=[0,1], O=[2,3,4,5,6,7,8,9]

    vmul.s32 q10, q2, d0[0]             // q10 = O0 * 90        ->  1
    vmul.s32 q11, q2, d0[1]             // q11 = O0 * 87        ->  3
    vmul.s32 q12, q2, d1[0]             // q12 = O0 * 80        ->  5
    vmul.s32 q13, q2, d1[1]             // q13 = O0 * 70        ->  7
    vmul.s32 q14, q2, d2[0]             // q14 = O0 * 57        ->  9
    vmul.s32 q15, q2, d2[1]             // q15 = O0 * 43        -> 11

    vmla.s32 q10, q3, d0[1]             // q10+= O1 * 87
    vmla.s32 q11, q3, d2[0]             // q11+= O1 * 57
    vmla.s32 q12, q3, d3[1]             // q12+= O1 *  9
    vmls.s32 q13, q3, d2[1]             // q13+= O1 *-43
    vmls.s32 q14, q3, d1[0]             // q14+= O1 *-80
    vmls.s32 q15, q3, d0[0]             // q15+= O1 *-90

    vmla.s32 q10, q4, d1[0]             // q10+= O2 * 80
    vmla.s32 q11, q4, d3[1]             // q11+= O2 *  9
    vmls.s32 q12, q4, d1[1]             // q12+= O2 *-70
    vmls.s32 q13, q4, d0[1]             // q13+= O2 *-87
    vmls.s32 q14, q4, d3[0]             // q14+= O2 *-25
    vmla.s32 q15, q4, d2[0]             // q15+= O2 * 57

    vmla.s32 q10, q5, d1[1]             // q10+= O3 * 70
    vmls.s32 q11, q5, d2[1]             // q11+= O3 *-43
    vmls.s32 q12, q5, d0[1]             // q12+= O3 *-87
    vmla.s32 q13, q5, d3[1]             // q13+= O3 *  9
    vmla.s32 q14, q5, d0[0]             // q14+= O3 * 90
    vmla.s32 q15, q5, d3[0]             // q15+= O3 * 25

    vmla.s32 q10, q6, d2[0]             // q10+= O4 * 57
    vmls.s32 q11, q6, d1[0]             // q11+= O4 *-80
    vmls.s32 q12, q6, d3[0]             // q12+= O4 *-25
    vmla.s32 q13, q6, d0[0]             // q13+= O4 * 90
    vmls.s32 q14, q6, d3[1]             // q14+= O4 *-9
    vmls.s32 q15, q6, d0[1]             // q15+= O4 *-87

    vmla.s32 q10, q7, d2[1]             // q10+= O5 * 43
    vmls.s32 q11, q7, d0[0]             // q11+= O5 *-90
    vmla.s32 q12, q7, d2[0]             // q12+= O5 * 57
    vmla.s32 q13, q7, d3[0]             // q13+= O5 * 25
    vmls.s32 q14, q7, d0[1]             // q14+= O5 *-87
    vmla.s32 q15, q7, d1[1]             // q15+= O5 * 70

    vmla.s32 q10, q8, d3[0]             // q10+= O6 * 25
    vmls.s32 q11, q8, d1[1]             // q11+= O6 *-70
    vmla.s32 q12, q8, d0[0]             // q12+= O6 * 90
    vmls.s32 q13, q8, d1[0]             // q13+= O6 *-80
    vmla.s32 q14, q8, d2[1]             // q14+= O6 * 43
    vmla.s32 q15, q8, d3[1]             // q15+= O6 *  9

    vmla.s32 q10, q9, d3[1]             // q10+= O7 *  9        -> [ 1]'
    vmls.s32 q11, q9, d3[0]             // q11+= O7 *-25        -> [ 3]'
    vmla.s32 q12, q9, d2[1]             // q12+= O7 * 43        -> [ 5]'
    vqrshrn.s32 d20, q10, 10            // d20                  -> [ 1]
    vmls.s32 q13, q9, d2[0]             // q13+= O7 *-57        -> [ 7]'
    vqrshrn.s32 d21, q11, 10            // d21                  -> [ 3]

    vmul.s32 q11, q2, d3[0]             // q11 = O0 * 25        -> 13
    vmul.s32 q2,  q2, d3[1]             // q2  = O0 *  9        -> 15

    vst1.16 {d20}, [r1], r3             // Stroe [ 1]
    vst1.16 {d21}, [r1], r3             // Stroe [ 3]

    vmls.s32 q11, q3, d1[1]             // q11+= O1 *-70
    vmls.s32 q2,  q3, d3[0]             // q2 += O1 *-25

    vmla.s32 q14, q9, d1[1]             // q14+= O7 * 70        -> [ 9]'
    vmls.s32 q15, q9, d1[0]             // q15+= O7 *-80        -> [11]'

    vqrshrn.s32 d24, q12, 10            // d24                  -> [ 5]

    vqrshrn.s32 d25, q13, 10            // d25                  -> [ 7]
    vqrshrn.s32 d28, q14, 10            // d28                  -> [ 9]
    vqrshrn.s32 d29, q15, 10            // d29                  -> [11]

    vst1.16 {d24}, [r1], r3             // Stroe [ 5]
    vst1.16 {d25}, [r1], r3             // Stroe [ 7]
    vst1.16 {d28}, [r1], r3             // Stroe [ 9]
    vst1.16 {d29}, [r1], r3             // Stroe [11]

    vmla.s32 q11, q4, d0[0]             // q11+= O2 * 90
    vmla.s32 q2,  q4, d2[1]             // q2 += O2 * 43

    vmls.s32 q11, q5, d1[0]             // q11+= O3 *-80
    vmls.s32 q2,  q5, d2[0]             // q2 += O3 *-57

    vmla.s32 q11, q6, d2[1]             // q11+= O4 * 43
    vmla.s32 q2,  q6, d1[1]             // q2 += O4 * 70

    vmla.s32 q11, q7, d3[1]             // q11+= O5 *  9
    vmls.s32 q2,  q7, d1[0]             // q2 += O5 *-80

    vmls.s32 q11, q8, d2[0]             // q11+= O6 *-57
    vmla.s32 q2,  q8, d0[1]             // q2 += O6 * 87

    vmla.s32 q11, q9, d0[1]             // q11+= O7 * 87        -> [13]'
    vmls.s32 q2,  q9, d0[0]             // q2 += O7 *-90        -> [15]'

    vqrshrn.s32 d6, q11, 10             // d6                   -> [13]
    vqrshrn.s32 d7, q2, 10              // d7                   -> [15]
    vst1.16 {d6}, [r1], r3              // Stroe [13]
    vst1.16 {d7}, [r1], r3              // Stroe [15]

    sub r1, #(17*16-4)*2
    subs r12, #1
    bgt .loop2

    add sp, #16*16*2
    vpop {q4-q7}
    pop {pc}
endfunc

