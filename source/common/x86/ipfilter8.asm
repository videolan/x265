;*****************************************************************************
;* Copyright (C) 2013-2017 MulticoreWare, Inc
;*
;* Authors: Min Chen <chenm003@163.com>
;*          Nabajit Deka <nabajit@multicorewareinc.com>
;*          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;*
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
;*
;* This program is also available under a commercial proprietary license.
;* For more information, contact us at license @ x265.com.
;*****************************************************************************/

%include "x86inc.asm"
%include "x86util.asm"

SECTION_RODATA 64
const tab_Tm,    db 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6
                 db 4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10
                 db 8, 9,10,11, 9,10,11,12,10,11,12,13,11,12,13, 14

const interp4_vpp_shuf, times 2 db 0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15

const interp4_vpp_shuf1, dd 0, 1, 1, 2, 2, 3, 3, 4
                         dd 2, 3, 3, 4, 4, 5, 5, 6

const tab_Lm,    db 0, 1, 2, 3, 4,  5,  6,  7,  1, 2, 3, 4,  5,  6,  7,  8
                 db 2, 3, 4, 5, 6,  7,  8,  9,  3, 4, 5, 6,  7,  8,  9,  10
                 db 4, 5, 6, 7, 8,  9,  10, 11, 5, 6, 7, 8,  9,  10, 11, 12
                 db 6, 7, 8, 9, 10, 11, 12, 13, 7, 8, 9, 10, 11, 12, 13, 14

const pd_526336, times 8 dd 8192*64+2048

const tab_ChromaCoeff, db  0, 64,  0,  0
                       db -2, 58, 10, -2
                       db -4, 54, 16, -2
                       db -6, 46, 28, -4
                       db -4, 36, 36, -4
                       db -4, 28, 46, -6
                       db -2, 16, 54, -4
                       db -2, 10, 58, -2

const tab_LumaCoeff,   db   0, 0,  0,  64,  0,   0,  0,  0
                       db  -1, 4, -10, 58,  17, -5,  1,  0
                       db  -1, 4, -11, 40,  40, -11, 4, -1
                       db   0, 1, -5,  17,  58, -10, 4, -1

const tabw_LumaCoeff,  dw   0, 0,  0,  64,  0,   0,  0,  0
                       dw  -1, 4, -10, 58,  17, -5,  1,  0
                       dw  -1, 4, -11, 40,  40, -11, 4, -1
                       dw   0, 1, -5,  17,  58, -10, 4, -1

const tab_LumaCoeffV,   times 4 dw 0, 0
                        times 4 dw 0, 64
                        times 4 dw 0, 0
                        times 4 dw 0, 0

                        times 4 dw -1, 4
                        times 4 dw -10, 58
                        times 4 dw 17, -5
                        times 4 dw 1, 0

                        times 4 dw -1, 4
                        times 4 dw -11, 40
                        times 4 dw 40, -11
                        times 4 dw 4, -1

                        times 4 dw 0, 1
                        times 4 dw -5, 17
                        times 4 dw 58, -10
                        times 4 dw 4, -1

const pw_LumaCoeffVer,  times 8 dw 0, 0
                        times 8 dw 0, 64
                        times 8 dw 0, 0
                        times 8 dw 0, 0

                        times 8 dw -1, 4
                        times 8 dw -10, 58
                        times 8 dw 17, -5
                        times 8 dw 1, 0

                        times 8 dw -1, 4
                        times 8 dw -11, 40
                        times 8 dw 40, -11
                        times 8 dw 4, -1

                        times 8 dw 0, 1
                        times 8 dw -5, 17
                        times 8 dw 58, -10
                        times 8 dw 4, -1

const tab_LumaCoeffVer, times 8 db 0, 0
                        times 8 db 0, 64
                        times 8 db 0, 0
                        times 8 db 0, 0

                        times 8 db -1, 4
                        times 8 db -10, 58
                        times 8 db 17, -5
                        times 8 db 1, 0

                        times 8 db -1, 4
                        times 8 db -11, 40
                        times 8 db 40, -11
                        times 8 db 4, -1

                        times 8 db 0, 1
                        times 8 db -5, 17
                        times 8 db 58, -10
                        times 8 db 4, -1

const tab_LumaCoeffVer_32,  times 16 db 0, 0
                            times 16 db 0, 64
                            times 16 db 0, 0
                            times 16 db 0, 0

                            times 16 db -1, 4
                            times 16 db -10, 58
                            times 16 db 17, -5
                            times 16 db 1, 0

                            times 16 db -1, 4
                            times 16 db -11, 40
                            times 16 db 40, -11
                            times 16 db 4, -1

                            times 16 db 0, 1
                            times 16 db -5, 17
                            times 16 db 58, -10
                            times 16 db 4, -1

ALIGN 64
const tab_ChromaCoeffVer_32_avx512,     times 32 db 0, 64
                                        times 32 db 0, 0

                                        times 32 db -2, 58
                                        times 32 db 10, -2

                                        times 32 db -4, 54
                                        times 32 db 16, -2

                                        times 32 db -6, 46
                                        times 32 db 28, -4

                                        times 32 db -4, 36
                                        times 32 db 36, -4

                                        times 32 db -4, 28
                                        times 32 db 46, -6

                                        times 32 db -2, 16
                                        times 32 db 54, -4

                                        times 32 db -2, 10
                                        times 32 db 58, -2

ALIGN 64
const pw_ChromaCoeffVer_32_avx512,      times 16 dw 0, 64
                                        times 16 dw 0, 0

                                        times 16 dw -2, 58
                                        times 16 dw 10, -2

                                        times 16 dw -4, 54
                                        times 16 dw 16, -2

                                        times 16 dw -6, 46
                                        times 16 dw 28, -4

                                        times 16 dw -4, 36
                                        times 16 dw 36, -4

                                        times 16 dw -4, 28
                                        times 16 dw 46, -6

                                        times 16 dw -2, 16
                                        times 16 dw 54, -4

                                        times 16 dw -2, 10
                                        times 16 dw 58, -2

ALIGN 64
const pw_LumaCoeffVer_avx512,           times 16 dw 0, 0
                                        times 16 dw 0, 64
                                        times 16 dw 0, 0
                                        times 16 dw 0, 0

                                        times 16 dw -1, 4
                                        times 16 dw -10, 58
                                        times 16 dw 17, -5
                                        times 16 dw 1, 0

                                        times 16 dw -1, 4
                                        times 16 dw -11, 40
                                        times 16 dw 40, -11
                                        times 16 dw 4, -1

                                        times 16 dw 0, 1
                                        times 16 dw -5, 17
                                        times 16 dw 58, -10
                                        times 16 dw 4, -1

ALIGN 64
const tab_LumaCoeffVer_32_avx512,       times 32 db 0, 0
                                        times 32 db 0, 64
                                        times 32 db 0, 0
                                        times 32 db 0, 0

                                        times 32 db -1, 4
                                        times 32 db -10, 58
                                        times 32 db 17, -5
                                        times 32 db 1, 0

                                        times 32 db -1, 4
                                        times 32 db -11, 40
                                        times 32 db 40, -11
                                        times 32 db 4, -1

                                        times 32 db 0, 1
                                        times 32 db -5, 17
                                        times 32 db 58, -10
                                        times 32 db 4, -1

const tab_c_64_n64, times 8 db 64, -64

const interp8_hps_shuf,     dd 0, 4, 1, 5, 2, 6, 3, 7

const interp4_horiz_shuf_load1_avx512,  times 2 db 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6
const interp4_horiz_shuf_load2_avx512,  times 2 db 8, 9, 10, 11, 9, 10, 11, 12, 10, 11, 12, 13, 11, 12, 13, 14
const interp4_horiz_shuf_load3_avx512,  times 2 db 4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10

ALIGN 64
interp4_vps_store1_avx512:   dq 0, 1, 8, 9, 2, 3, 10, 11
interp4_vps_store2_avx512:   dq 4, 5, 12, 13, 6, 7, 14, 15
const interp4_hps_shuf_avx512,  dq 0, 4, 1, 5, 2, 6, 3, 7
const interp4_hps_store_16xN_avx512,  dq 0, 2, 1, 3, 4, 6, 5, 7
const interp8_hps_store_avx512,  dq 0, 1, 4, 5, 2, 3, 6, 7
const interp8_vsp_store_avx512,  dq 0, 2, 4, 6, 1, 3, 5, 7

SECTION .text
cextern pb_128
cextern pw_1
cextern pw_32
cextern pw_512
cextern pw_2000
cextern pw_8192

%macro FILTER_H8_W8_sse2 0
    movh        m1, [r0 + x - 3]
    movh        m4, [r0 + x - 2]
    punpcklbw   m1, m6
    punpcklbw   m4, m6
    movh        m5, [r0 + x - 1]
    movh        m0, [r0 + x]
    punpcklbw   m5, m6
    punpcklbw   m0, m6
    pmaddwd     m1, m3
    pmaddwd     m4, m3
    pmaddwd     m5, m3
    pmaddwd     m0, m3
    packssdw    m1, m4
    packssdw    m5, m0
    pshuflw     m4, m1, q2301
    pshufhw     m4, m4, q2301
    pshuflw     m0, m5, q2301
    pshufhw     m0, m0, q2301
    paddw       m1, m4
    paddw       m5, m0
    psrldq      m1, 2
    psrldq      m5, 2
    pshufd      m1, m1, q3120
    pshufd      m5, m5, q3120
    punpcklqdq  m1, m5
    movh        m7, [r0 + x + 1]
    movh        m4, [r0 + x + 2]
    punpcklbw   m7, m6
    punpcklbw   m4, m6
    movh        m5, [r0 + x + 3]
    movh        m0, [r0 + x + 4]
    punpcklbw   m5, m6
    punpcklbw   m0, m6
    pmaddwd     m7, m3
    pmaddwd     m4, m3
    pmaddwd     m5, m3
    pmaddwd     m0, m3
    packssdw    m7, m4
    packssdw    m5, m0
    pshuflw     m4, m7, q2301
    pshufhw     m4, m4, q2301
    pshuflw     m0, m5, q2301
    pshufhw     m0, m0, q2301
    paddw       m7, m4
    paddw       m5, m0
    psrldq      m7, 2
    psrldq      m5, 2
    pshufd      m7, m7, q3120
    pshufd      m5, m5, q3120
    punpcklqdq  m7, m5
    pshuflw     m4, m1, q2301
    pshufhw     m4, m4, q2301
    pshuflw     m0, m7, q2301
    pshufhw     m0, m0, q2301
    paddw       m1, m4
    paddw       m7, m0
    psrldq      m1, 2
    psrldq      m7, 2
    pshufd      m1, m1, q3120
    pshufd      m7, m7, q3120
    punpcklqdq  m1, m7
%endmacro

%macro FILTER_H8_W4_sse2 0
    movh        m1, [r0 + x - 3]
    movh        m0, [r0 + x - 2]
    punpcklbw   m1, m6
    punpcklbw   m0, m6
    movh        m4, [r0 + x - 1]
    movh        m5, [r0 + x]
    punpcklbw   m4, m6
    punpcklbw   m5, m6
    pmaddwd     m1, m3
    pmaddwd     m0, m3
    pmaddwd     m4, m3
    pmaddwd     m5, m3
    packssdw    m1, m0
    packssdw    m4, m5
    pshuflw     m0, m1, q2301
    pshufhw     m0, m0, q2301
    pshuflw     m5, m4, q2301
    pshufhw     m5, m5, q2301
    paddw       m1, m0
    paddw       m4, m5
    psrldq      m1, 2
    psrldq      m4, 2
    pshufd      m1, m1, q3120
    pshufd      m4, m4, q3120
    punpcklqdq  m1, m4
    pshuflw     m0, m1, q2301
    pshufhw     m0, m0, q2301
    paddw       m1, m0
    psrldq      m1, 2
    pshufd      m1, m1, q3120
%endmacro
%macro PROCESS_LUMA_W4_4R_sse2 0
    movd        m2,     [r0]
    movd        m7,     [r0 + r1]
    punpcklbw   m2,     m7                      ; m2=[0 1]

    lea         r0,     [r0 + 2 * r1]
    movd        m3,     [r0]
    punpcklbw   m7,     m3                      ; m7=[1 2]
    punpcklbw   m2,     m0
    punpcklbw   m7,     m0
    pmaddwd     m2,     [r6 + 0 * 32]
    pmaddwd     m7,     [r6 + 0 * 32]
    packssdw    m2,     m7                      ; m2=[0+1 1+2]

    movd        m7,     [r0 + r1]
    punpcklbw   m3,     m7                      ; m3=[2 3]
    lea         r0,     [r0 + 2 * r1]
    movd        m5,     [r0]
    punpcklbw   m7,     m5                      ; m7=[3 4]
    punpcklbw   m3,     m0
    punpcklbw   m7,     m0
    pmaddwd     m4,     m3,     [r6 + 1 * 32]
    pmaddwd     m6,     m7,     [r6 + 1 * 32]
    packssdw    m4,     m6                      ; m4=[2+3 3+4]
    paddw       m2,     m4                      ; m2=[0+1+2+3 1+2+3+4]                   Row1-2
    pmaddwd     m3,     [r6 + 0 * 32]
    pmaddwd     m7,     [r6 + 0 * 32]
    packssdw    m3,     m7                      ; m3=[2+3 3+4]                           Row3-4

    movd        m7,     [r0 + r1]
    punpcklbw   m5,     m7                      ; m5=[4 5]
    lea         r0,     [r0 + 2 * r1]
    movd        m4,     [r0]
    punpcklbw   m7,     m4                      ; m7=[5 6]
    punpcklbw   m5,     m0
    punpcklbw   m7,     m0
    pmaddwd     m6,     m5,     [r6 + 2 * 32]
    pmaddwd     m8,     m7,     [r6 + 2 * 32]
    packssdw    m6,     m8                      ; m6=[4+5 5+6]
    paddw       m2,     m6                      ; m2=[0+1+2+3+4+5 1+2+3+4+5+6]           Row1-2
    pmaddwd     m5,     [r6 + 1 * 32]
    pmaddwd     m7,     [r6 + 1 * 32]
    packssdw    m5,     m7                      ; m5=[4+5 5+6]
    paddw       m3,     m5                      ; m3=[2+3+4+5 3+4+5+6]                   Row3-4

    movd        m7,     [r0 + r1]
    punpcklbw   m4,     m7                      ; m4=[6 7]
    lea         r0,     [r0 + 2 * r1]
    movd        m5,     [r0]
    punpcklbw   m7,     m5                      ; m7=[7 8]
    punpcklbw   m4,     m0
    punpcklbw   m7,     m0
    pmaddwd     m6,     m4,     [r6 + 3 * 32]
    pmaddwd     m8,     m7,     [r6 + 3 * 32]
    packssdw    m6,     m8                      ; m7=[6+7 7+8]
    paddw       m2,     m6                      ; m2=[0+1+2+3+4+5+6+7 1+2+3+4+5+6+7+8]   Row1-2 end
    pmaddwd     m4,     [r6 + 2 * 32]
    pmaddwd     m7,     [r6 + 2 * 32]
    packssdw    m4,     m7                      ; m4=[6+7 7+8]
    paddw       m3,     m4                      ; m3=[2+3+4+5+6+7 3+4+5+6+7+8]           Row3-4

    movd        m7,     [r0 + r1]
    punpcklbw   m5,     m7                      ; m5=[8 9]
    movd        m4,     [r0 + 2 * r1]
    punpcklbw   m7,     m4                      ; m7=[9 10]
    punpcklbw   m5,     m0
    punpcklbw   m7,     m0
    pmaddwd     m5,     [r6 + 3 * 32]
    pmaddwd     m7,     [r6 + 3 * 32]
    packssdw    m5,     m7                      ; m5=[8+9 9+10]
    paddw       m3,     m5                      ; m3=[2+3+4+5+6+7+8+9 3+4+5+6+7+8+9+10]  Row3-4 end
%endmacro

%macro PROCESS_LUMA_W8_4R_sse2 0
    movq        m7,     [r0]
    movq        m6,     [r0 + r1]
    punpcklbw   m7,     m6
    punpcklbw   m2,     m7,     m0
    punpckhbw   m7,     m0
    pmaddwd     m2,     [r6 + 0 * 32]
    pmaddwd     m7,     [r6 + 0 * 32]
    packssdw    m2,     m7                      ; m2=[0+1]               Row1

    lea         r0,     [r0 + 2 * r1]
    movq        m7,     [r0]
    punpcklbw   m6,     m7
    punpcklbw   m3,     m6,     m0
    punpckhbw   m6,     m0
    pmaddwd     m3,     [r6 + 0 * 32]
    pmaddwd     m6,     [r6 + 0 * 32]
    packssdw    m3,     m6                      ; m3=[1+2]               Row2

    movq        m6,     [r0 + r1]
    punpcklbw   m7,     m6
    punpckhbw   m8,     m7,     m0
    punpcklbw   m7,     m0
    pmaddwd     m4,     m7,     [r6 + 0 * 32]
    pmaddwd     m9,     m8,     [r6 + 0 * 32]
    packssdw    m4,     m9                      ; m4=[2+3]               Row3
    pmaddwd     m7,     [r6 + 1 * 32]
    pmaddwd     m8,     [r6 + 1 * 32]
    packssdw    m7,     m8
    paddw       m2,     m7                      ; m2=[0+1+2+3]           Row1

    lea         r0,     [r0 + 2 * r1]
    movq        m10,    [r0]
    punpcklbw   m6,     m10
    punpckhbw   m8,     m6,     m0
    punpcklbw   m6,     m0
    pmaddwd     m5,     m6,     [r6 + 0 * 32]
    pmaddwd     m9,     m8,     [r6 + 0 * 32]
    packssdw    m5,     m9                      ; m5=[3+4]               Row4
    pmaddwd     m6,     [r6 + 1 * 32]
    pmaddwd     m8,     [r6 + 1 * 32]
    packssdw    m6,     m8
    paddw       m3,     m6                      ; m3 = [1+2+3+4]         Row2

    movq        m6,     [r0 + r1]
    punpcklbw   m10,    m6
    punpckhbw   m8,     m10,    m0
    punpcklbw   m10,    m0
    pmaddwd     m7,     m10,    [r6 + 1 * 32]
    pmaddwd     m9,     m8,     [r6 + 1 * 32]
    packssdw    m7,     m9
    pmaddwd     m10,    [r6 + 2 * 32]
    pmaddwd     m8,     [r6 + 2 * 32]
    packssdw    m10,    m8
    paddw       m2,     m10                     ; m2=[0+1+2+3+4+5]       Row1
    paddw       m4,     m7                      ; m4=[2+3+4+5]           Row3

    lea         r0,     [r0 + 2 * r1]
    movq        m10,    [r0]
    punpcklbw   m6,     m10
    punpckhbw   m8,     m6,     m0
    punpcklbw   m6,     m0
    pmaddwd     m7,     m6,     [r6 + 1 * 32]
    pmaddwd     m9,     m8,     [r6 + 1 * 32]
    packssdw    m7,     m9
    pmaddwd     m6,     [r6 + 2 * 32]
    pmaddwd     m8,     [r6 + 2 * 32]
    packssdw    m6,     m8
    paddw       m3,     m6                      ; m3=[1+2+3+4+5+6]       Row2
    paddw       m5,     m7                      ; m5=[3+4+5+6]           Row4

    movq        m6,     [r0 + r1]
    punpcklbw   m10,    m6
    punpckhbw   m8,     m10,    m0
    punpcklbw   m10,    m0
    pmaddwd     m7,     m10,    [r6 + 2 * 32]
    pmaddwd     m9,     m8,     [r6 + 2 * 32]
    packssdw    m7,     m9
    pmaddwd     m10,    [r6 + 3 * 32]
    pmaddwd     m8,     [r6 + 3 * 32]
    packssdw    m10,    m8
    paddw       m2,     m10                     ; m2=[0+1+2+3+4+5+6+7]   Row1 end
    paddw       m4,     m7                      ; m4=[2+3+4+5+6+7]       Row3

    lea         r0,     [r0 + 2 * r1]
    movq        m10,    [r0]
    punpcklbw   m6,     m10
    punpckhbw   m8,     m6,     m0
    punpcklbw   m6,     m0
    pmaddwd     m7,     m6,     [r6 + 2 * 32]
    pmaddwd     m9,     m8,     [r6 + 2 * 32]
    packssdw    m7,     m9
    pmaddwd     m6,     [r6 + 3 * 32]
    pmaddwd     m8,     [r6 + 3 * 32]
    packssdw    m6,     m8
    paddw       m3,     m6                      ; m3=[1+2+3+4+5+6+7+8]   Row2 end
    paddw       m5,     m7                      ; m5=[3+4+5+6+7+8]       Row4

    movq        m6,     [r0 + r1]
    punpcklbw   m10,    m6
    punpckhbw   m8,     m10,     m0
    punpcklbw   m10,    m0
    pmaddwd     m8,     [r6 + 3 * 32]
    pmaddwd     m10,    [r6 + 3 * 32]
    packssdw    m10,    m8
    paddw       m4,     m10                     ; m4=[2+3+4+5+6+7+8+9]   Row3 end

    movq        m10,    [r0 + 2 * r1]
    punpcklbw   m6,     m10
    punpckhbw   m8,     m6,     m0
    punpcklbw   m6,     m0
    pmaddwd     m8,     [r6 + 3 * 32]
    pmaddwd     m6,     [r6 + 3 * 32]
    packssdw    m6,     m8
    paddw       m5,     m6                      ; m5=[3+4+5+6+7+8+9+10]  Row4 end
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_%3_4x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_sse2 3
INIT_XMM sse2
cglobal interp_8tap_vert_%3_%1x%2, 5, 8, 11
    lea         r5,     [3 * r1]
    sub         r0,     r5
    shl         r4d,    7

%ifdef PIC
    lea         r6,     [pw_LumaCoeffVer]
    add         r6,     r4
%else
    lea         r6,     [pw_LumaCoeffVer + r4]
%endif

%ifidn %3,pp
    mova        m1,     [pw_32]
%else
    mova        m1,     [pw_2000]
    add         r3d,    r3d
%endif

    mov         r4d,    %2/4
    lea         r5,     [3 * r3]
    pxor        m0,     m0

.loopH:
%assign x 0
%rep (%1 / 8)
    PROCESS_LUMA_W8_4R_sse2

%ifidn %3,pp
    paddw       m2,     m1
    paddw       m3,     m1
    paddw       m4,     m1
    paddw       m5,     m1
    psraw       m2,     6
    psraw       m3,     6
    psraw       m4,     6
    psraw       m5,     6

    packuswb    m2,     m3
    packuswb    m4,     m5

    movh        [r2 + x], m2
    movhps      [r2 + r3 + x], m2
    movh        [r2 + 2 * r3 + x], m4
    movhps      [r2 + r5 + x], m4
%else
    psubw       m2,     m1
    psubw       m3,     m1
    psubw       m4,     m1
    psubw       m5,     m1

    movu        [r2 + (2*x)], m2
    movu        [r2 + r3 + (2*x)], m3
    movu        [r2 + 2 * r3 + (2*x)], m4
    movu        [r2 + r5 + (2*x)], m5
%endif
%assign x x+8
%if %1 > 8
    lea         r7,     [8 * r1 - 8]
    sub         r0,     r7
%endif
%endrep

%rep (%1 % 8)/4
    PROCESS_LUMA_W4_4R_sse2

%ifidn %3,pp
    paddw       m2,     m1
    psraw       m2,     6
    paddw       m3,     m1
    psraw       m3,     6

    packuswb    m2,     m3

    movd        [r2 + x], m2
    psrldq      m2,     4
    movd        [r2 + r3 + x], m2
    psrldq      m2,     4
    movd        [r2 + 2 * r3 + x], m2
    psrldq      m2,     4
    movd        [r2 + r5 + x], m2
%else
    psubw       m2,     m1
    psubw       m3,     m1

    movh        [r2 + (2*x)], m2
    movhps      [r2 + r3 + (2*x)], m2
    movh        [r2 + 2 * r3 + (2*x)], m3
    movhps      [r2 + r5 + (2*x)], m3
%endif
%endrep

    lea         r2,     [r2 + 4 * r3]
%if %1 <= 8
    lea         r7,     [4 * r1]
    sub         r0,     r7
%elif %1 == 12
    lea         r7,     [4 * r1 + 8]
    sub         r0,     r7
%else
    lea         r0,     [r0 + 4 * r1 - %1]
%endif

    dec         r4d
    jnz         .loopH

    RET

%endmacro

%if ARCH_X86_64
    FILTER_VER_LUMA_sse2 4, 4, pp
    FILTER_VER_LUMA_sse2 4, 8, pp
    FILTER_VER_LUMA_sse2 4, 16, pp
    FILTER_VER_LUMA_sse2 8, 4, pp
    FILTER_VER_LUMA_sse2 8, 8, pp
    FILTER_VER_LUMA_sse2 8, 16, pp
    FILTER_VER_LUMA_sse2 8, 32, pp
    FILTER_VER_LUMA_sse2 12, 16, pp
    FILTER_VER_LUMA_sse2 16, 4, pp
    FILTER_VER_LUMA_sse2 16, 8, pp
    FILTER_VER_LUMA_sse2 16, 12, pp
    FILTER_VER_LUMA_sse2 16, 16, pp
    FILTER_VER_LUMA_sse2 16, 32, pp
    FILTER_VER_LUMA_sse2 16, 64, pp
    FILTER_VER_LUMA_sse2 24, 32, pp
    FILTER_VER_LUMA_sse2 32, 8, pp
    FILTER_VER_LUMA_sse2 32, 16, pp
    FILTER_VER_LUMA_sse2 32, 24, pp
    FILTER_VER_LUMA_sse2 32, 32, pp
    FILTER_VER_LUMA_sse2 32, 64, pp
    FILTER_VER_LUMA_sse2 48, 64, pp
    FILTER_VER_LUMA_sse2 64, 16, pp
    FILTER_VER_LUMA_sse2 64, 32, pp
    FILTER_VER_LUMA_sse2 64, 48, pp
    FILTER_VER_LUMA_sse2 64, 64, pp

    FILTER_VER_LUMA_sse2 4, 4, ps
    FILTER_VER_LUMA_sse2 4, 8, ps
    FILTER_VER_LUMA_sse2 4, 16, ps
    FILTER_VER_LUMA_sse2 8, 4, ps
    FILTER_VER_LUMA_sse2 8, 8, ps
    FILTER_VER_LUMA_sse2 8, 16, ps
    FILTER_VER_LUMA_sse2 8, 32, ps
    FILTER_VER_LUMA_sse2 12, 16, ps
    FILTER_VER_LUMA_sse2 16, 4, ps
    FILTER_VER_LUMA_sse2 16, 8, ps
    FILTER_VER_LUMA_sse2 16, 12, ps
    FILTER_VER_LUMA_sse2 16, 16, ps
    FILTER_VER_LUMA_sse2 16, 32, ps
    FILTER_VER_LUMA_sse2 16, 64, ps
    FILTER_VER_LUMA_sse2 24, 32, ps
    FILTER_VER_LUMA_sse2 32, 8, ps
    FILTER_VER_LUMA_sse2 32, 16, ps
    FILTER_VER_LUMA_sse2 32, 24, ps
    FILTER_VER_LUMA_sse2 32, 32, ps
    FILTER_VER_LUMA_sse2 32, 64, ps
    FILTER_VER_LUMA_sse2 48, 64, ps
    FILTER_VER_LUMA_sse2 64, 16, ps
    FILTER_VER_LUMA_sse2 64, 32, ps
    FILTER_VER_LUMA_sse2 64, 48, ps
    FILTER_VER_LUMA_sse2 64, 64, ps
%endif

%macro FILTER_P2S_2_4_sse2 1
    movd        m2,     [r0 + %1]
    movd        m3,     [r0 + r1 + %1]
    punpcklwd   m2,     m3
    movd        m3,     [r0 + r1 * 2 + %1]
    movd        m4,     [r0 + r4 + %1]
    punpcklwd   m3,     m4
    punpckldq   m2,     m3
    punpcklbw   m2,     m0
    psllw       m2,     6
    psubw       m2,     m1

    movd        [r2 + r3 * 0 + %1 * 2], m2
    psrldq      m2,     4
    movd        [r2 + r3 * 1 + %1 * 2], m2
    psrldq      m2,     4
    movd        [r2 + r3 * 2 + %1 * 2], m2
    psrldq      m2,     4
    movd        [r2 + r5 + %1 * 2], m2
%endmacro

%macro FILTER_P2S_4_4_sse2 1
    movd        m2,     [r0 + %1]
    movd        m3,     [r0 + r1 + %1]
    movd        m4,     [r0 + r1 * 2 + %1]
    movd        m5,     [r0 + r4 + %1]
    punpckldq   m2,     m3
    punpcklbw   m2,     m0
    punpckldq   m4,     m5
    punpcklbw   m4,     m0
    psllw       m2,     6
    psllw       m4,     6
    psubw       m2,     m1
    psubw       m4,     m1
    movh        [r2 + r3 * 0 + %1 * 2], m2
    movh        [r2 + r3 * 2 + %1 * 2], m4
    movhps      [r2 + r3 * 1 + %1 * 2], m2
    movhps      [r2 + r5 + %1 * 2], m4
%endmacro

%macro FILTER_P2S_4_2_sse2 0
    movd        m2,     [r0]
    movd        m3,     [r0 + r1]
    punpckldq   m2,     m3
    punpcklbw   m2,     m0
    psllw       m2,     6
    psubw       m2,     [pw_8192]
    movh        [r2],   m2
    movhps      [r2 + r3 * 2], m2
%endmacro

%macro FILTER_P2S_8_4_sse2 1
    movh        m2,     [r0 + %1]
    movh        m3,     [r0 + r1 + %1]
    movh        m4,     [r0 + r1 * 2 + %1]
    movh        m5,     [r0 + r4 + %1]
    punpcklbw   m2,     m0
    punpcklbw   m3,     m0
    punpcklbw   m5,     m0
    punpcklbw   m4,     m0
    psllw       m2,     6
    psllw       m3,     6
    psllw       m5,     6
    psllw       m4,     6
    psubw       m2,     m1
    psubw       m3,     m1
    psubw       m4,     m1
    psubw       m5,     m1
    movu        [r2 + r3 * 0 + %1 * 2], m2
    movu        [r2 + r3 * 1 + %1 * 2], m3
    movu        [r2 + r3 * 2 + %1 * 2], m4
    movu        [r2 + r5 + %1 * 2], m5
%endmacro

%macro FILTER_P2S_8_2_sse2 1
    movh        m2,     [r0 + %1]
    movh        m3,     [r0 + r1 + %1]
    punpcklbw   m2,     m0
    punpcklbw   m3,     m0
    psllw       m2,     6
    psllw       m3,     6
    psubw       m2,     m1
    psubw       m3,     m1
    movu        [r2 + r3 * 0 + %1 * 2], m2
    movu        [r2 + r3 * 1 + %1 * 2], m3
%endmacro

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
%macro FILTER_PIX_TO_SHORT_sse2 2
INIT_XMM sse2
cglobal filterPixelToShort_%1x%2, 4, 6, 6
    pxor        m0,     m0
%if %2 == 2
%if %1 == 4
    FILTER_P2S_4_2_sse2
%elif %1 == 8
    add        r3d, r3d
    mova       m1, [pw_8192]
    FILTER_P2S_8_2_sse2 0
%endif
%else
    add        r3d, r3d
    mova       m1, [pw_8192]
    lea        r4, [r1 * 3]
    lea        r5, [r3 * 3]
%assign y 1
%rep %2/4
%assign x 0
%rep %1/8
    FILTER_P2S_8_4_sse2 x
%if %2 == 6
    lea         r0,     [r0 + 4 * r1]
    lea         r2,     [r2 + 4 * r3]
    FILTER_P2S_8_2_sse2 x
%endif
%assign x x+8
%endrep
%rep (%1 % 8)/4
    FILTER_P2S_4_4_sse2 x
%assign x x+4
%endrep
%rep (%1 % 4)/2
    FILTER_P2S_2_4_sse2 x
%endrep
%if y < %2/4
    lea         r0,     [r0 + 4 * r1]
    lea         r2,     [r2 + 4 * r3]
%assign y y+1
%endif
%endrep
%endif
RET
%endmacro

    FILTER_PIX_TO_SHORT_sse2 2, 4
    FILTER_PIX_TO_SHORT_sse2 2, 8
    FILTER_PIX_TO_SHORT_sse2 2, 16
    FILTER_PIX_TO_SHORT_sse2 4, 2
    FILTER_PIX_TO_SHORT_sse2 4, 4
    FILTER_PIX_TO_SHORT_sse2 4, 8
    FILTER_PIX_TO_SHORT_sse2 4, 16
    FILTER_PIX_TO_SHORT_sse2 4, 32
    FILTER_PIX_TO_SHORT_sse2 6, 8
    FILTER_PIX_TO_SHORT_sse2 6, 16
    FILTER_PIX_TO_SHORT_sse2 8, 2
    FILTER_PIX_TO_SHORT_sse2 8, 4
    FILTER_PIX_TO_SHORT_sse2 8, 6
    FILTER_PIX_TO_SHORT_sse2 8, 8
    FILTER_PIX_TO_SHORT_sse2 8, 12
    FILTER_PIX_TO_SHORT_sse2 8, 16
    FILTER_PIX_TO_SHORT_sse2 8, 32
    FILTER_PIX_TO_SHORT_sse2 8, 64
    FILTER_PIX_TO_SHORT_sse2 12, 16
    FILTER_PIX_TO_SHORT_sse2 12, 32
    FILTER_PIX_TO_SHORT_sse2 16, 4
    FILTER_PIX_TO_SHORT_sse2 16, 8
    FILTER_PIX_TO_SHORT_sse2 16, 12
    FILTER_PIX_TO_SHORT_sse2 16, 16
    FILTER_PIX_TO_SHORT_sse2 16, 24
    FILTER_PIX_TO_SHORT_sse2 16, 32
    FILTER_PIX_TO_SHORT_sse2 16, 64
    FILTER_PIX_TO_SHORT_sse2 24, 32
    FILTER_PIX_TO_SHORT_sse2 24, 64
    FILTER_PIX_TO_SHORT_sse2 32, 8
    FILTER_PIX_TO_SHORT_sse2 32, 16
    FILTER_PIX_TO_SHORT_sse2 32, 24
    FILTER_PIX_TO_SHORT_sse2 32, 32
    FILTER_PIX_TO_SHORT_sse2 32, 48
    FILTER_PIX_TO_SHORT_sse2 32, 64
    FILTER_PIX_TO_SHORT_sse2 48, 64
    FILTER_PIX_TO_SHORT_sse2 64, 16
    FILTER_PIX_TO_SHORT_sse2 64, 32
    FILTER_PIX_TO_SHORT_sse2 64, 48
    FILTER_PIX_TO_SHORT_sse2 64, 64

%macro FILTER_H8_W8 7-8   ; t0, t1, t2, t3, coef, c512, src, dst
    movu        %1, %7
    pshufb      %2, %1, [tab_Lm +  0]
    pmaddubsw   %2, %5
    pshufb      %3, %1, [tab_Lm + 16]
    pmaddubsw   %3, %5
    phaddw      %2, %3
    pshufb      %4, %1, [tab_Lm + 32]
    pmaddubsw   %4, %5
    pshufb      %1, %1, [tab_Lm + 48]
    pmaddubsw   %1, %5
    phaddw      %4, %1
    phaddw      %2, %4
  %if %0 == 8
    pmulhrsw    %2, %6
    packuswb    %2, %2
    movh        %8, %2
  %endif
%endmacro

;-----------------------------------------------------------------------------
; Interpolate HV
;-----------------------------------------------------------------------------
%macro FILTER_HV8_START 7 ; (t0, t1, t2, t3, t4, off_src, off_coeff) -> (t3, t5), (t4, t1), [2]
    mova        %5, [r0 +  (%6 + 0) * 16]
    mova        %1, [r0 +  (%6 + 1) * 16]
    mova        %2, [r0 +  (%6 + 2) * 16]
    punpcklwd   %3, %5, %1
    punpckhwd   %5, %1
    pmaddwd     %3, [r5 + (%7) * 16]   ; R3 = L[0+1] -- Row 0
    pmaddwd     %5, [r5 + (%7) * 16]   ; R0 = H[0+1]
    punpcklwd   %4, %1, %2
    punpckhwd   %1, %2
    pmaddwd     %4, [r5 + (%7) * 16]   ; R4 = L[1+2] -- Row 1
    pmaddwd     %1, [r5 + (%7) * 16]   ; R1 = H[1+2]
%endmacro ; FILTER_HV8_START

%macro FILTER_HV8_MID 10 ; (Row3, prevRow, sum0L, sum1L, sum0H, sum1H, t6, t7, off_src, off_coeff) -> [6]
    mova        %8, [r0 +  (%9 + 0) * 16]
    mova        %1, [r0 +  (%9 + 1) * 16]
    punpcklwd   %7, %2, %8
    punpckhwd   %2, %8
    pmaddwd     %7, [r5 + %10 * 16]
    pmaddwd     %2, [r5 + %10 * 16]
    paddd       %3, %7              ; R3 = L[0+1+2+3] -- Row 0
    paddd       %5, %2              ; R0 = H[0+1+2+3]
    punpcklwd   %7, %8, %1
    punpckhwd   %8, %1
    pmaddwd     %7, [r5 + %10 * 16]
    pmaddwd     %8, [r5 + %10 * 16]
    paddd       %4, %7              ; R4 = L[1+2+3+4] -- Row 1
    paddd       %6, %8              ; R1 = H[1+2+3+4]
%endmacro ; FILTER_HV8_MID

; Round and Saturate
%macro FILTER_HV8_END 4 ; output in [1, 3]
    paddd       %1, [pd_526336]
    paddd       %2, [pd_526336]
    paddd       %3, [pd_526336]
    paddd       %4, [pd_526336]
    psrad       %1, 12
    psrad       %2, 12
    psrad       %3, 12
    psrad       %4, 12
    packssdw    %1, %2
    packssdw    %3, %4

    ; TODO: is merge better? I think this way is short dependency link
    packuswb    %1, %3
%endmacro ; FILTER_HV8_END

;-----------------------------------------------------------------------------
; void interp_8tap_hv_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int idxX, int idxY)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal interp_8tap_hv_pp_8x8, 4, 7, 8, 0-15*16
%define coef        m7
%define stk_buf     rsp

    mov         r4d,        r4m
    mov         r5d,        r5m

%ifdef PIC
    lea         r6,         [tab_LumaCoeff]
    movh        coef,       [r6 + r4 * 8]
%else
    movh        coef,       [tab_LumaCoeff + r4 * 8]
%endif
    punpcklqdq  coef,       coef

    ; move to row -3
    lea         r6,         [r1 + r1 * 2]
    sub         r0,         r6

    xor         r6,         r6
    mov         r4,         rsp

.loopH:
    FILTER_H8_W8 m0, m1, m2, m3, coef, [pw_512], [r0 - 3]
    psubw       m1,         [pw_2000]
    mova        [r4],       m1

    add         r0,         r1
    add         r4,         16
    inc         r6
    cmp         r6,         8+7
    jnz         .loopH

    ; ready to phase V
    ; Here all of mN is free

    ; load coeff table
    shl         r5,         6
    lea         r6,         [tab_LumaCoeffV]
    lea         r5,         [r5 + r6]

    ; load intermedia buffer
    mov         r0,         stk_buf

    ; register mapping
    ; r0 - src
    ; r5 - coeff
    ; r6 - loop_i

    ; let's go
    xor         r6,         r6

    ; TODO: this loop have more than 70 instructions, I think it is more than Intel loop decode cache
.loopV:

    FILTER_HV8_START    m1, m2, m3, m4, m0,             0, 0
    FILTER_HV8_MID      m6, m2, m3, m4, m0, m1, m7, m5, 3, 1
    FILTER_HV8_MID      m5, m6, m3, m4, m0, m1, m7, m2, 5, 2
    FILTER_HV8_MID      m6, m5, m3, m4, m0, m1, m7, m2, 7, 3
    FILTER_HV8_END      m3, m0, m4, m1

    movh        [r2],       m3
    movhps      [r2 + r3],  m3

    lea         r0,         [r0 + 16 * 2]
    lea         r2,         [r2 + r3 * 2]

    inc         r6
    cmp         r6,         8/2
    jnz         .loopV

    RET

;-----------------------------------------------------------------------------
; void interp_8tap_hv_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int idxX, int idxY)
;-----------------------------------------------------------------------------
INIT_XMM sse3
cglobal interp_8tap_hv_pp_8x8, 4, 7, 8, 0-15*16
    mov         r4d,        r4m
    mov         r5d,        r5m
    add         r4d,        r4d
    pxor        m6,         m6

%ifdef PIC
    lea         r6,         [tabw_LumaCoeff]
    mova        m3,         [r6 + r4 * 8]
%else
    mova        m3,         [tabw_LumaCoeff + r4 * 8]
%endif

    ; move to row -3
    lea         r6,         [r1 + r1 * 2]
    sub         r0,         r6

    mov         r4,         rsp

%assign x 0     ;needed for FILTER_H8_W8_sse2 macro
%assign y 1
%rep 15
    FILTER_H8_W8_sse2
    psubw       m1,         [pw_2000]
    mova        [r4],       m1

%if y < 15
    add         r0,         r1
    add         r4,         16
%endif
%assign y y+1
%endrep

    ; ready to phase V
    ; Here all of mN is free

    ; load coeff table
    shl         r5,         6
    lea         r6,         [tab_LumaCoeffV]
    lea         r5,         [r5 + r6]

    ; load intermedia buffer
    mov         r0,         rsp

    ; register mapping
    ; r0 - src
    ; r5 - coeff

    ; let's go
%assign y 1
%rep 4
    FILTER_HV8_START    m1, m2, m3, m4, m0,             0, 0
    FILTER_HV8_MID      m6, m2, m3, m4, m0, m1, m7, m5, 3, 1
    FILTER_HV8_MID      m5, m6, m3, m4, m0, m1, m7, m2, 5, 2
    FILTER_HV8_MID      m6, m5, m3, m4, m0, m1, m7, m2, 7, 3
    FILTER_HV8_END      m3, m0, m4, m1

    movh        [r2],       m3
    movhps      [r2 + r3],  m3

%if y < 4
    lea         r0,         [r0 + 16 * 2]
    lea         r2,         [r2 + r3 * 2]
%endif
%assign y y+1
%endrep
    RET

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_2xN 1
INIT_XMM sse4
cglobal filterPixelToShort_2x%1, 3, 4, 3
    mov         r3d, r3m
    add         r3d, r3d

    ; load constant
    mova        m1, [pb_128]
    mova        m2, [tab_c_64_n64]

%rep %1/2
    movd        m0, [r0]
    pinsrd      m0, [r0 + r1], 1
    punpcklbw   m0, m1
    pmaddubsw   m0, m2

    movd        [r2 + r3 * 0], m0
    pextrd      [r2 + r3 * 1], m0, 2

    lea         r0, [r0 + r1 * 2]
    lea         r2, [r2 + r3 * 2]
%endrep
    RET
%endmacro
    P2S_H_2xN 4
    P2S_H_2xN 8
    P2S_H_2xN 16

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_4xN 1
INIT_XMM sse4
cglobal filterPixelToShort_4x%1, 3, 6, 4
    mov         r3d, r3m
    add         r3d, r3d
    lea         r4, [r3 * 3]
    lea         r5, [r1 * 3]

    ; load constant
    mova        m2, [pb_128]
    mova        m3, [tab_c_64_n64]

%assign x 0
%rep %1/4
    movd        m0, [r0]
    pinsrd      m0, [r0 + r1], 1
    punpcklbw   m0, m2
    pmaddubsw   m0, m3

    movd        m1, [r0 + r1 * 2]
    pinsrd      m1, [r0 + r5], 1
    punpcklbw   m1, m2
    pmaddubsw   m1, m3

    movq        [r2 + r3 * 0], m0
    movq        [r2 + r3 * 2], m1
    movhps      [r2 + r3 * 1], m0
    movhps      [r2 + r4], m1
%assign x x+1
%if (x != %1/4)
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endif
%endrep
    RET
%endmacro
    P2S_H_4xN 4
    P2S_H_4xN 8
    P2S_H_4xN 16
    P2S_H_4xN 32

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_6xN 1
INIT_XMM sse4
cglobal filterPixelToShort_6x%1, 3, 7, 6
    mov         r3d, r3m
    add         r3d, r3d
    lea         r4, [r1 * 3]
    lea         r5, [r3 * 3]

    ; load height
    mov         r6d, %1/4

    ; load constant
    mova        m4, [pb_128]
    mova        m5, [tab_c_64_n64]

.loop:
    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r4]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movh        [r2 + r3 * 0], m0
    pextrd      [r2 + r3 * 0 + 8], m0, 2
    movh        [r2 + r3 * 1], m1
    pextrd      [r2 + r3 * 1 + 8], m1, 2
    movh        [r2 + r3 * 2], m2
    pextrd      [r2 + r3 * 2 + 8], m2, 2
    movh        [r2 + r5], m3
    pextrd      [r2 + r5 + 8], m3, 2

    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]

    dec         r6d
    jnz         .loop
    RET
%endmacro
    P2S_H_6xN 8
    P2S_H_6xN 16

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_8xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_8x%1, 3, 7, 6
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load height
    mov         r4d, %1/4

    ; load constant
    mova        m4, [pb_128]
    mova        m5, [tab_c_64_n64]

.loop:
    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0], m0
    movu        [r2 + r3 * 1], m1
    movu        [r2 + r3 * 2], m2
    movu        [r2 + r6 ], m3

    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]

    dec         r4d
    jnz         .loop
    RET
%endmacro
    P2S_H_8xN 8
    P2S_H_8xN 4
    P2S_H_8xN 16
    P2S_H_8xN 32
    P2S_H_8xN 12
    P2S_H_8xN 64

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal filterPixelToShort_8x6, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r4, [r1 * 3]
    lea         r5, [r1 * 5]
    lea         r6, [r3 * 3]

    ; load constant
    mova        m3, [pb_128]
    mova        m4, [tab_c_64_n64]

    movh        m0, [r0]
    punpcklbw   m0, m3
    pmaddubsw   m0, m4

    movh        m1, [r0 + r1]
    punpcklbw   m1, m3
    pmaddubsw   m1, m4

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m3
    pmaddubsw   m2, m4

    movu        [r2 + r3 * 0], m0
    movu        [r2 + r3 * 1], m1
    movu        [r2 + r3 * 2], m2

    movh        m0, [r0 + r4]
    punpcklbw   m0, m3
    pmaddubsw   m0, m4

    movh        m1, [r0 + r1 * 4]
    punpcklbw   m1, m3
    pmaddubsw   m1, m4

    movh        m2, [r0 + r5]
    punpcklbw   m2, m3
    pmaddubsw   m2, m4

    movu        [r2 + r6 ], m0
    movu        [r2 + r3 * 4], m1
    lea         r2, [r2 + r3 * 4]
    movu        [r2 + r3], m2

    RET

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_16xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_16x%1, 3, 7, 6
    mov         r3d, r3m
    add         r3d, r3d
    lea         r4, [r3 * 3]
    lea         r5, [r1 * 3]

   ; load height
    mov         r6d, %1/4

    ; load constant
    mova        m4, [pb_128]
    mova        m5, [tab_c_64_n64]

.loop:
    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0], m0
    movu        [r2 + r3 * 1], m1
    movu        [r2 + r3 * 2], m2
    movu        [r2 + r4], m3

    lea         r0, [r0 + 8]

    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0 + 16], m0
    movu        [r2 + r3 * 1 + 16], m1
    movu        [r2 + r3 * 2 + 16], m2
    movu        [r2 + r4 + 16], m3

    lea         r0, [r0 + r1 * 4 - 8]
    lea         r2, [r2 + r3 * 4]

    dec         r6d
    jnz         .loop
    RET
%endmacro
    P2S_H_16xN 16
    P2S_H_16xN 4
    P2S_H_16xN 8
    P2S_H_16xN 12
    P2S_H_16xN 32
    P2S_H_16xN 64
    P2S_H_16xN 24

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal filterPixelToShort_16x4, 3, 4, 2
    mov             r3d, r3m
    add             r3d, r3d

    ; load constant
    vbroadcasti128  m1, [pw_2000]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    lea             r1, [r1 * 3]
    lea             r3, [r3 * 3]

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0
    RET

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal filterPixelToShort_16x8, 3, 6, 2
    mov             r3d, r3m
    add             r3d, r3d
    lea             r4, [r1 * 3]
    lea             r5, [r3 * 3]

    ; load constant
    vbroadcasti128  m1, [pw_2000]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0
    RET

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal filterPixelToShort_16x12, 3, 6, 2
    mov             r3d, r3m
    add             r3d, r3d
    lea             r4, [r1 * 3]
    lea             r5, [r3 * 3]

    ; load constant
    vbroadcasti128  m1, [pw_2000]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0
    RET

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal filterPixelToShort_16x16, 3, 6, 2
    mov             r3d, r3m
    add             r3d, r3d
    lea             r4, [r1 * 3]
    lea             r5, [r3 * 3]

    ; load constant
    vbroadcasti128  m1, [pw_2000]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0
    RET

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal filterPixelToShort_16x24, 3, 7, 2
    mov             r3d, r3m
    add             r3d, r3d
    lea             r4, [r1 * 3]
    lea             r5, [r3 * 3]
    mov             r6d, 3

    ; load constant
    vbroadcasti128  m1, [pw_2000]
.loop:
    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]

    dec             r6d
    jnz             .loop
    RET

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_16xN_avx2 1
INIT_YMM avx2
cglobal filterPixelToShort_16x%1, 3, 7, 2
    mov             r3d, r3m
    add             r3d, r3d
    lea             r4, [r1 * 3]
    lea             r5, [r3 * 3]
    mov             r6d, %1/16

    ; load constant
    vbroadcasti128  m1, [pw_2000]
.loop:
    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]

    pmovzxbw        m0, [r0]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2], m0

    pmovzxbw        m0, [r0 + r1]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3], m0

    pmovzxbw        m0, [r0 + r1 * 2]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r3 * 2], m0

    pmovzxbw        m0, [r0 + r4]
    psllw           m0, 6
    psubw           m0, m1
    movu            [r2 + r5], m0

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]

    dec             r6d
    jnz             .loop
    RET
%endmacro
P2S_H_16xN_avx2 32
P2S_H_16xN_avx2 64

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_32xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_32x%1, 3, 7, 6
    mov         r3d, r3m
    add         r3d, r3d
    lea         r4, [r3 * 3]
    lea         r5, [r1 * 3]

    ; load height
    mov         r6d, %1/4

    ; load constant
    mova        m4, [pb_128]
    mova        m5, [tab_c_64_n64]

.loop:
    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0], m0
    movu        [r2 + r3 * 1], m1
    movu        [r2 + r3 * 2], m2
    movu        [r2 + r4], m3

    lea         r0, [r0 + 8]

    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0 + 16], m0
    movu        [r2 + r3 * 1 + 16], m1
    movu        [r2 + r3 * 2 + 16], m2
    movu        [r2 + r4 + 16], m3

    lea         r0, [r0 + 8]

    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0 + 32], m0
    movu        [r2 + r3 * 1 + 32], m1
    movu        [r2 + r3 * 2 + 32], m2
    movu        [r2 + r4 + 32], m3

    lea         r0, [r0 + 8]

    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0 + 48], m0
    movu        [r2 + r3 * 1 + 48], m1
    movu        [r2 + r3 * 2 + 48], m2
    movu        [r2 + r4 + 48], m3

    lea         r0, [r0 + r1 * 4 - 24]
    lea         r2, [r2 + r3 * 4]

    dec         r6d
    jnz         .loop
    RET
%endmacro
    P2S_H_32xN 32
    P2S_H_32xN 8
    P2S_H_32xN 16
    P2S_H_32xN 24
    P2S_H_32xN 64
    P2S_H_32xN 48

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_32xN_avx2 1
INIT_YMM avx2
cglobal filterPixelToShort_32x%1, 3, 7, 3
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load height
    mov         r4d, %1/4

    ; load constant
    vpbroadcastd m2, [pw_2000]

.loop:
    pmovzxbw    m0, [r0 + 0 * mmsize/2]
    pmovzxbw    m1, [r0 + 1 * mmsize/2]
    psllw       m0, 6
    psllw       m1, 6
    psubw       m0, m2
    psubw       m1, m2
    movu        [r2 + 0 * mmsize], m0
    movu        [r2 + 1 * mmsize], m1

    pmovzxbw    m0, [r0 + r1 + 0 * mmsize/2]
    pmovzxbw    m1, [r0 + r1 + 1 * mmsize/2]
    psllw       m0, 6
    psllw       m1, 6
    psubw       m0, m2
    psubw       m1, m2
    movu        [r2 + r3 + 0 * mmsize], m0
    movu        [r2 + r3 + 1 * mmsize], m1

    pmovzxbw    m0, [r0 + r1 * 2 + 0 * mmsize/2]
    pmovzxbw    m1, [r0 + r1 * 2 + 1 * mmsize/2]
    psllw       m0, 6
    psllw       m1, 6
    psubw       m0, m2
    psubw       m1, m2
    movu        [r2 + r3 * 2 + 0 * mmsize], m0
    movu        [r2 + r3 * 2 + 1 * mmsize], m1

    pmovzxbw    m0, [r0 + r5 + 0 * mmsize/2]
    pmovzxbw    m1, [r0 + r5 + 1 * mmsize/2]
    psllw       m0, 6
    psllw       m1, 6
    psubw       m0, m2
    psubw       m1, m2
    movu        [r2 + r6 + 0 * mmsize], m0
    movu        [r2 + r6 + 1 * mmsize], m1

    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]

    dec         r4d
    jnz        .loop
    RET
%endmacro
    P2S_H_32xN_avx2 32
    P2S_H_32xN_avx2 8
    P2S_H_32xN_avx2 16
    P2S_H_32xN_avx2 24
    P2S_H_32xN_avx2 64
    P2S_H_32xN_avx2 48

;-----------------------------------------------------------------------------
;p2s and p2s_aligned 32xN avx512 code start
;-----------------------------------------------------------------------------

%macro PROCESS_P2S_32x4_AVX512 0
    pmovzxbw    m0, [r0]
    pmovzxbw    m1, [r0 + r1]
    pmovzxbw    m2, [r0 + r1 * 2]
    pmovzxbw    m3, [r0 + r5]

    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4

    movu        [r2],           m0
    movu        [r2 + r3],      m1
    movu        [r2 + r3 * 2],  m2
    movu        [r2 + r6],      m3
%endmacro

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%if ARCH_X86_64
INIT_ZMM avx512
cglobal filterPixelToShort_32x8, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

    PROCESS_P2S_32x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_32x16, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 3
    PROCESS_P2S_32x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_32x24, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 5
    PROCESS_P2S_32x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_32x32, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 7
    PROCESS_P2S_32x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_32x48, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 11
    PROCESS_P2S_32x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_32x64, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 15
    PROCESS_P2S_32x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_32x4_AVX512
    RET
%endif

%macro PROCESS_P2S_ALIGNED_32x4_AVX512 0
    pmovzxbw    m0, [r0]
    pmovzxbw    m1, [r0 + r1]
    pmovzxbw    m2, [r0 + r1 * 2]
    pmovzxbw    m3, [r0 + r5]

    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4

    mova        [r2],           m0
    mova        [r2 + r3],      m1
    mova        [r2 + r3 * 2],  m2
    mova        [r2 + r6],      m3
%endmacro

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%if ARCH_X86_64
INIT_ZMM avx512
cglobal filterPixelToShort_aligned_32x8, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

    PROCESS_P2S_ALIGNED_32x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_ALIGNED_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_32x16, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 3
    PROCESS_P2S_ALIGNED_32x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_ALIGNED_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_32x24, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 5
    PROCESS_P2S_ALIGNED_32x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_ALIGNED_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_32x32, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 7
    PROCESS_P2S_ALIGNED_32x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_ALIGNED_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_32x48, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 11
    PROCESS_P2S_ALIGNED_32x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_ALIGNED_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_32x64, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 15
    PROCESS_P2S_ALIGNED_32x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_ALIGNED_32x4_AVX512
    RET
%endif
;-----------------------------------------------------------------------------
;p2s and p2s_aligned 32xN avx512 code end
;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_64xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_64x%1, 3, 7, 6
    mov         r3d, r3m
    add         r3d, r3d
    lea         r4, [r3 * 3]
    lea         r5, [r1 * 3]

    ; load height
    mov         r6d, %1/4

    ; load constant
    mova        m4, [pb_128]
    mova        m5, [tab_c_64_n64]

.loop:
    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0], m0
    movu        [r2 + r3 * 1], m1
    movu        [r2 + r3 * 2], m2
    movu        [r2 + r4], m3

    lea         r0, [r0 + 8]

    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0 + 16], m0
    movu        [r2 + r3 * 1 + 16], m1
    movu        [r2 + r3 * 2 + 16], m2
    movu        [r2 + r4 + 16], m3

    lea         r0, [r0 + 8]

    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0 + 32], m0
    movu        [r2 + r3 * 1 + 32], m1
    movu        [r2 + r3 * 2 + 32], m2
    movu        [r2 + r4 + 32], m3

    lea         r0, [r0 + 8]

    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0 + 48], m0
    movu        [r2 + r3 * 1 + 48], m1
    movu        [r2 + r3 * 2 + 48], m2
    movu        [r2 + r4 + 48], m3

    lea         r0, [r0 + 8]

    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0 + 64], m0
    movu        [r2 + r3 * 1 + 64], m1
    movu        [r2 + r3 * 2 + 64], m2
    movu        [r2 + r4 + 64], m3

    lea         r0, [r0 + 8]

    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0 + 80], m0
    movu        [r2 + r3 * 1 + 80], m1
    movu        [r2 + r3 * 2 + 80], m2
    movu        [r2 + r4 + 80], m3

    lea         r0, [r0 + 8]

    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0 + 96], m0
    movu        [r2 + r3 * 1 + 96], m1
    movu        [r2 + r3 * 2 + 96], m2
    movu        [r2 + r4 + 96], m3

    lea         r0, [r0 + 8]

    movh        m0, [r0]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r0 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    movh        m2, [r0 + r1 * 2]
    punpcklbw   m2, m4
    pmaddubsw   m2, m5

    movh        m3, [r0 + r5]
    punpcklbw   m3, m4
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0 + 112], m0
    movu        [r2 + r3 * 1 + 112], m1
    movu        [r2 + r3 * 2 + 112], m2
    movu        [r2 + r4 + 112], m3

    lea         r0, [r0 + r1 * 4 - 56]
    lea         r2, [r2 + r3 * 4]

    dec         r6d
    jnz         .loop
    RET
%endmacro
    P2S_H_64xN 64
    P2S_H_64xN 16
    P2S_H_64xN 32
    P2S_H_64xN 48

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_64xN_avx2 1
INIT_YMM avx2
cglobal filterPixelToShort_64x%1, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load height
    mov         r4d, %1/4

    ; load constant
    vpbroadcastd m4, [pw_2000]

.loop:
    pmovzxbw    m0, [r0 + 0 * mmsize/2]
    pmovzxbw    m1, [r0 + 1 * mmsize/2]
    pmovzxbw    m2, [r0 + 2 * mmsize/2]
    pmovzxbw    m3, [r0 + 3 * mmsize/2]
    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4

    movu        [r2 + 0 * mmsize], m0
    movu        [r2 + 1 * mmsize], m1
    movu        [r2 + 2 * mmsize], m2
    movu        [r2 + 3 * mmsize], m3

    pmovzxbw    m0, [r0 + r1 + 0 * mmsize/2]
    pmovzxbw    m1, [r0 + r1 + 1 * mmsize/2]
    pmovzxbw    m2, [r0 + r1 + 2 * mmsize/2]
    pmovzxbw    m3, [r0 + r1 + 3 * mmsize/2]
    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4

    movu        [r2 + r3 + 0 * mmsize], m0
    movu        [r2 + r3 + 1 * mmsize], m1
    movu        [r2 + r3 + 2 * mmsize], m2
    movu        [r2 + r3 + 3 * mmsize], m3

    pmovzxbw    m0, [r0 + r1 * 2 + 0 * mmsize/2]
    pmovzxbw    m1, [r0 + r1 * 2 + 1 * mmsize/2]
    pmovzxbw    m2, [r0 + r1 * 2 + 2 * mmsize/2]
    pmovzxbw    m3, [r0 + r1 * 2 + 3 * mmsize/2]
    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4

    movu        [r2 + r3 * 2 + 0 * mmsize], m0
    movu        [r2 + r3 * 2 + 1 * mmsize], m1
    movu        [r2 + r3 * 2 + 2 * mmsize], m2
    movu        [r2 + r3 * 2 + 3 * mmsize], m3

    pmovzxbw    m0, [r0 + r5 + 0 * mmsize/2]
    pmovzxbw    m1, [r0 + r5 + 1 * mmsize/2]
    pmovzxbw    m2, [r0 + r5 + 2 * mmsize/2]
    pmovzxbw    m3, [r0 + r5 + 3 * mmsize/2]
    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4

    movu        [r2 + r6 + 0 * mmsize], m0
    movu        [r2 + r6 + 1 * mmsize], m1
    movu        [r2 + r6 + 2 * mmsize], m2
    movu        [r2 + r6 + 3 * mmsize], m3

    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]

    dec         r4d
    jnz        .loop
    RET
%endmacro
    P2S_H_64xN_avx2 64
    P2S_H_64xN_avx2 16
    P2S_H_64xN_avx2 32
    P2S_H_64xN_avx2 48

;-----------------------------------------------------------------------------
;p2s and p2s_aligned 64xN avx512 code start
;-----------------------------------------------------------------------------
%macro PROCESS_P2S_64x4_AVX512 0
    pmovzxbw    m0, [r0]
    pmovzxbw    m1, [r0 + mmsize/2]
    pmovzxbw    m2, [r0 + r1]
    pmovzxbw    m3, [r0 + r1 + mmsize/2]

    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4
    movu        [r2], m0
    movu        [r2 + mmsize], m1
    movu        [r2 + r3], m2
    movu        [r2 + r3 + mmsize], m3

    pmovzxbw    m0, [r0 + r1 * 2]
    pmovzxbw    m1, [r0 + r1 * 2 + mmsize/2]
    pmovzxbw    m2, [r0 + r5]
    pmovzxbw    m3, [r0 + r5 + mmsize/2]

    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4
    movu        [r2 + r3 * 2], m0
    movu        [r2 + r3 * 2 + mmsize], m1
    movu        [r2 + r6], m2
    movu        [r2 + r6 + mmsize], m3
%endmacro

%macro PROCESS_P2S_ALIGNED_64x4_AVX512 0
    pmovzxbw    m0, [r0]
    pmovzxbw    m1, [r0 + mmsize/2]
    pmovzxbw    m2, [r0 + r1]
    pmovzxbw    m3, [r0 + r1 + mmsize/2]

    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4
    mova        [r2], m0
    mova        [r2 + mmsize], m1
    mova        [r2 + r3], m2
    mova        [r2 + r3 + mmsize], m3

    pmovzxbw    m0, [r0 + r1 * 2]
    pmovzxbw    m1, [r0 + r1 * 2 + mmsize/2]
    pmovzxbw    m2, [r0 + r5]
    pmovzxbw    m3, [r0 + r5 + mmsize/2]

    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4
    mova        [r2 + r3 * 2], m0
    mova        [r2 + r3 * 2 + mmsize], m1
    mova        [r2 + r6], m2
    mova        [r2 + r6 + mmsize], m3
%endmacro
;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%if ARCH_X86_64
INIT_ZMM avx512
cglobal filterPixelToShort_64x64, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 15
    PROCESS_P2S_64x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_64x48, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 11
    PROCESS_P2S_64x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_64x32, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 7
    PROCESS_P2S_64x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_64x16, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 3
    PROCESS_P2S_64x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_64x64, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 15
    PROCESS_P2S_ALIGNED_64x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_ALIGNED_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_64x48, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 11
    PROCESS_P2S_ALIGNED_64x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_ALIGNED_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_64x32, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 7
    PROCESS_P2S_ALIGNED_64x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_ALIGNED_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_64x16, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd      m4, [pw_2000]

%rep 3
    PROCESS_P2S_ALIGNED_64x4_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
%endrep
    PROCESS_P2S_ALIGNED_64x4_AVX512
    RET
%endif
;-----------------------------------------------------------------------------
;p2s and p2s_aligned 64xN avx512 code end
;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel src, intptr_t srcStride, int16_t dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_12xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_12x%1, 3, 7, 6
    mov         r3d, r3m
    add         r3d, r3d
    lea         r4, [r1 * 3]
    lea         r6, [r3 * 3]
    mov         r5d, %1/4

    ; load constant
    mova        m4, [pb_128]
    mova        m5, [tab_c_64_n64]

.loop:
    movu        m0, [r0]
    punpcklbw   m1, m0, m4
    punpckhbw   m0, m4
    pmaddubsw   m0, m5
    pmaddubsw   m1, m5

    movu        m2, [r0 + r1]
    punpcklbw   m3, m2, m4
    punpckhbw   m2, m4
    pmaddubsw   m2, m5
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 0], m1
    movu        [r2 + r3 * 1], m3

    movh        [r2 + r3 * 0 + 16], m0
    movh        [r2 + r3 * 1 + 16], m2

    movu        m0, [r0 + r1 * 2]
    punpcklbw   m1, m0, m4
    punpckhbw   m0, m4
    pmaddubsw   m0, m5
    pmaddubsw   m1, m5

    movu        m2, [r0 + r4]
    punpcklbw   m3, m2, m4
    punpckhbw   m2, m4
    pmaddubsw   m2, m5
    pmaddubsw   m3, m5

    movu        [r2 + r3 * 2], m1
    movu        [r2 + r6], m3

    movh        [r2 + r3 * 2 + 16], m0
    movh        [r2 + r6 + 16], m2

    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]

    dec         r5d
    jnz         .loop
    RET
%endmacro
    P2S_H_12xN 16
    P2S_H_12xN 32

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_24xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_24x%1, 3, 7, 5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r4, [r1 * 3]
    lea         r5, [r3 * 3]
    mov         r6d, %1/4

    ; load constant
    mova        m3, [pb_128]
    mova        m4, [tab_c_64_n64]

.loop:
    movu        m0, [r0]
    punpcklbw   m1, m0, m3
    punpckhbw   m0, m3
    pmaddubsw   m0, m4
    pmaddubsw   m1, m4

    movu        m2, [r0 + 16]
    punpcklbw   m2, m3
    pmaddubsw   m2, m4

    movu        [r2 +  r3 * 0], m1
    movu        [r2 +  r3 * 0 + 16], m0
    movu        [r2 +  r3 * 0 + 32], m2

    movu        m0, [r0 + r1]
    punpcklbw   m1, m0, m3
    punpckhbw   m0, m3
    pmaddubsw   m0, m4
    pmaddubsw   m1, m4

    movu        m2, [r0 + r1 + 16]
    punpcklbw   m2, m3
    pmaddubsw   m2, m4

    movu        [r2 +  r3 * 1], m1
    movu        [r2 +  r3 * 1 + 16], m0
    movu        [r2 +  r3 * 1 + 32], m2

    movu        m0, [r0 + r1 * 2]
    punpcklbw   m1, m0, m3
    punpckhbw   m0, m3
    pmaddubsw   m0, m4
    pmaddubsw   m1, m4

    movu        m2, [r0 + r1 * 2 + 16]
    punpcklbw   m2, m3
    pmaddubsw   m2, m4

    movu        [r2 +  r3 * 2], m1
    movu        [r2 +  r3 * 2 + 16], m0
    movu        [r2 +  r3 * 2 + 32], m2

    movu        m0, [r0 + r4]
    punpcklbw   m1, m0, m3
    punpckhbw   m0, m3
    pmaddubsw   m0, m4
    pmaddubsw   m1, m4

    movu        m2, [r0 + r4 + 16]
    punpcklbw   m2, m3
    pmaddubsw   m2, m4
    movu        [r2 +  r5], m1
    movu        [r2 +  r5 + 16], m0
    movu        [r2 +  r5 + 32], m2

    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]

    dec         r6d
    jnz         .loop
    RET
%endmacro
    P2S_H_24xN 32
    P2S_H_24xN 64

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_24xN_avx2 1
INIT_YMM avx2
cglobal filterPixelToShort_24x%1, 3, 7, 4
    mov         r3d, r3m
    add         r3d, r3d
    lea         r4, [r1 * 3]
    lea         r5, [r3 * 3]
    mov         r6d, %1/4

    ; load constant
    vpbroadcastd m1, [pw_2000]
    vpbroadcastd m2, [pb_128]
    vpbroadcastd m3, [tab_c_64_n64]

.loop:
    pmovzxbw    m0, [r0]
    psllw       m0, 6
    psubw       m0, m1
    movu        [r2], m0

    movu        m0, [r0 + mmsize/2]
    punpcklbw   m0, m2
    pmaddubsw   m0, m3
    movu        [r2 +  r3 * 0 + mmsize], xm0

    pmovzxbw    m0, [r0 + r1]
    psllw       m0, 6
    psubw       m0, m1
    movu        [r2 + r3], m0

    movu        m0, [r0 + r1 + mmsize/2]
    punpcklbw   m0, m2
    pmaddubsw   m0, m3
    movu        [r2 +  r3 * 1 + mmsize], xm0

    pmovzxbw    m0, [r0 + r1 * 2]
    psllw       m0, 6
    psubw       m0, m1
    movu        [r2 + r3 * 2], m0

    movu        m0, [r0 + r1 * 2 + mmsize/2]
    punpcklbw   m0, m2
    pmaddubsw   m0, m3
    movu        [r2 +  r3 * 2 + mmsize], xm0

    pmovzxbw    m0, [r0 + r4]
    psllw       m0, 6
    psubw       m0, m1
    movu        [r2 + r5], m0

    movu        m0, [r0 + r4 + mmsize/2]
    punpcklbw   m0, m2
    pmaddubsw   m0, m3
    movu        [r2 + r5 + mmsize], xm0

    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]

    dec         r6d
    jnz         .loop
    RET
%endmacro
    P2S_H_24xN_avx2 32
    P2S_H_24xN_avx2 64

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal filterPixelToShort_48x64, 3, 7, 4
    mov         r3d, r3m
    add         r3d, r3d
    lea         r4, [r1 * 3]
    lea         r5, [r3 * 3]
    mov         r6d, 16

    ; load constant
    mova        m2, [pb_128]
    mova        m3, [tab_c_64_n64]

.loop:
    movu        m0, [r0]
    punpcklbw   m1, m0, m2
    punpckhbw   m0, m2
    pmaddubsw   m0, m3
    pmaddubsw   m1, m3

    movu        [r2 +  r3 * 0], m1
    movu        [r2 +  r3 * 0 + 16], m0

    movu        m0, [r0 + 16]
    punpcklbw   m1, m0, m2
    punpckhbw   m0, m2
    pmaddubsw   m0, m3
    pmaddubsw   m1, m3

    movu        [r2 +  r3 * 0 + 32], m1
    movu        [r2 +  r3 * 0 + 48], m0

    movu        m0, [r0 + 32]
    punpcklbw   m1, m0, m2
    punpckhbw   m0, m2
    pmaddubsw   m0, m3
    pmaddubsw   m1, m3

    movu        [r2 +  r3 * 0 + 64], m1
    movu        [r2 +  r3 * 0 + 80], m0

    movu        m0, [r0 + r1]
    punpcklbw   m1, m0, m2
    punpckhbw   m0, m2
    pmaddubsw   m0, m3
    pmaddubsw   m1, m3

    movu        [r2 +  r3 * 1], m1
    movu        [r2 +  r3 * 1 + 16], m0

    movu        m0, [r0 + r1 + 16]
    punpcklbw   m1, m0, m2
    punpckhbw   m0, m2
    pmaddubsw   m0, m3
    pmaddubsw   m1, m3

    movu        [r2 +  r3 * 1 + 32], m1
    movu        [r2 +  r3 * 1 + 48], m0

    movu        m0, [r0 + r1 + 32]
    punpcklbw   m1, m0, m2
    punpckhbw   m0, m2
    pmaddubsw   m0, m3
    pmaddubsw   m1, m3

    movu        [r2 +  r3 * 1 + 64], m1
    movu        [r2 +  r3 * 1 + 80], m0

    movu        m0, [r0 + r1 * 2]
    punpcklbw   m1, m0, m2
    punpckhbw   m0, m2
    pmaddubsw   m0, m3
    pmaddubsw   m1, m3

    movu        [r2 +  r3 * 2], m1
    movu        [r2 +  r3 * 2 + 16], m0

    movu        m0, [r0 + r1 * 2 + 16]
    punpcklbw   m1, m0, m2
    punpckhbw   m0, m2
    pmaddubsw   m0, m3
    pmaddubsw   m1, m3

    movu        [r2 +  r3 * 2 + 32], m1
    movu        [r2 +  r3 * 2 + 48], m0

    movu        m0, [r0 + r1 * 2 + 32]
    punpcklbw   m1, m0, m2
    punpckhbw   m0, m2
    pmaddubsw   m0, m3
    pmaddubsw   m1, m3

    movu        [r2 +  r3 * 2 + 64], m1
    movu        [r2 +  r3 * 2 + 80], m0

    movu        m0, [r0 + r4]
    punpcklbw   m1, m0, m2
    punpckhbw   m0, m2
    pmaddubsw   m0, m3
    pmaddubsw   m1, m3

    movu        [r2 +  r5], m1
    movu        [r2 +  r5 + 16], m0

    movu        m0, [r0 + r4 + 16]
    punpcklbw   m1, m0, m2
    punpckhbw   m0, m2
    pmaddubsw   m0, m3
    pmaddubsw   m1, m3

    movu        [r2 +  r5 + 32], m1
    movu        [r2 +  r5 + 48], m0

    movu        m0, [r0 + r4 + 32]
    punpcklbw   m1, m0, m2
    punpckhbw   m0, m2
    pmaddubsw   m0, m3
    pmaddubsw   m1, m3

    movu        [r2 +  r5 + 64], m1
    movu        [r2 +  r5 + 80], m0

    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]

    dec         r6d
    jnz         .loop
    RET

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal filterPixelToShort_48x64, 3,7,4
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load height
    mov         r4d, 64/4

    ; load constant
    vpbroadcastd m3, [pw_2000]

    ; just unroll(1) because it is best choice for 48x64
.loop:
    pmovzxbw    m0, [r0 + 0 * mmsize/2]
    pmovzxbw    m1, [r0 + 1 * mmsize/2]
    pmovzxbw    m2, [r0 + 2 * mmsize/2]
    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psubw       m0, m3
    psubw       m1, m3
    psubw       m2, m3
    movu        [r2 + 0 * mmsize], m0
    movu        [r2 + 1 * mmsize], m1
    movu        [r2 + 2 * mmsize], m2

    pmovzxbw    m0, [r0 + r1 + 0 * mmsize/2]
    pmovzxbw    m1, [r0 + r1 + 1 * mmsize/2]
    pmovzxbw    m2, [r0 + r1 + 2 * mmsize/2]
    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psubw       m0, m3
    psubw       m1, m3
    psubw       m2, m3
    movu        [r2 + r3 + 0 * mmsize], m0
    movu        [r2 + r3 + 1 * mmsize], m1
    movu        [r2 + r3 + 2 * mmsize], m2

    pmovzxbw    m0, [r0 + r1 * 2 + 0 * mmsize/2]
    pmovzxbw    m1, [r0 + r1 * 2 + 1 * mmsize/2]
    pmovzxbw    m2, [r0 + r1 * 2 + 2 * mmsize/2]
    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psubw       m0, m3
    psubw       m1, m3
    psubw       m2, m3
    movu        [r2 + r3 * 2 + 0 * mmsize], m0
    movu        [r2 + r3 * 2 + 1 * mmsize], m1
    movu        [r2 + r3 * 2 + 2 * mmsize], m2

    pmovzxbw    m0, [r0 + r5 + 0 * mmsize/2]
    pmovzxbw    m1, [r0 + r5 + 1 * mmsize/2]
    pmovzxbw    m2, [r0 + r5 + 2 * mmsize/2]
    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psubw       m0, m3
    psubw       m1, m3
    psubw       m2, m3
    movu        [r2 + r6 + 0 * mmsize], m0
    movu        [r2 + r6 + 1 * mmsize], m1
    movu        [r2 + r6 + 2 * mmsize], m2

    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]

    dec         r4d
    jnz        .loop
    RET

;-----------------------------------------------------------------------------
;p2s and p2s_aligned 48xN avx512 code start
;-----------------------------------------------------------------------------
%macro PROCESS_P2S_48x8_AVX512 0
    pmovzxbw    m0, [r0]
    pmovzxbw    m1, [r0 + r1]
    pmovzxbw    m2, [r0 + r1 * 2]
    pmovzxbw    m3, [r0 + r5]
    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4
    movu        [r2],           m0
    movu        [r2 + r3],      m1
    movu        [r2 + r3 * 2],  m2
    movu        [r2 + r6],      m3

    pmovzxbw    ym0, [r0 + 32]
    pmovzxbw    ym1, [r0 + r1 + 32]
    pmovzxbw    ym2, [r0 + r1 * 2 + 32]
    pmovzxbw    ym3, [r0 + r5 + 32]
    psllw       ym0, 6
    psllw       ym1, 6
    psllw       ym2, 6
    psllw       ym3, 6
    psubw       ym0, ym4
    psubw       ym1, ym4
    psubw       ym2, ym4
    psubw       ym3, ym4
    movu        [r2 + 64],           ym0
    movu        [r2 + r3 + 64],      ym1
    movu        [r2 + r3 * 2 + 64],  ym2
    movu        [r2 + r6 + 64],      ym3

    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]

    pmovzxbw    m0, [r0]
    pmovzxbw    m1, [r0 + r1]
    pmovzxbw    m2, [r0 + r1 * 2]
    pmovzxbw    m3, [r0 + r5]
    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4
    movu        [r2],           m0
    movu        [r2 + r3],      m1
    movu        [r2 + r3 * 2],  m2
    movu        [r2 + r6],      m3

    pmovzxbw    ym0, [r0 + 32]
    pmovzxbw    ym1, [r0 + r1 + 32]
    pmovzxbw    ym2, [r0 + r1 * 2 + 32]
    pmovzxbw    ym3, [r0 + r5 + 32]
    psllw       ym0, 6
    psllw       ym1, 6
    psllw       ym2, 6
    psllw       ym3, 6
    psubw       ym0, ym4
    psubw       ym1, ym4
    psubw       ym2, ym4
    psubw       ym3, ym4
    movu        [r2 + 64],           ym0
    movu        [r2 + r3 + 64],      ym1
    movu        [r2 + r3 * 2 + 64],  ym2
    movu        [r2 + r6 + 64],      ym3
%endmacro

%macro PROCESS_P2S_ALIGNED_48x8_AVX512 0
    pmovzxbw    m0, [r0]
    pmovzxbw    m1, [r0 + r1]
    pmovzxbw    m2, [r0 + r1 * 2]
    pmovzxbw    m3, [r0 + r5]
    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4
    mova        [r2],           m0
    mova        [r2 + r3],      m1
    mova        [r2 + r3 * 2],  m2
    mova        [r2 + r6],      m3

    pmovzxbw    ym0, [r0 + 32]
    pmovzxbw    ym1, [r0 + r1 + 32]
    pmovzxbw    ym2, [r0 + r1 * 2 + 32]
    pmovzxbw    ym3, [r0 + r5 + 32]
    psllw       ym0, 6
    psllw       ym1, 6
    psllw       ym2, 6
    psllw       ym3, 6
    psubw       ym0, ym4
    psubw       ym1, ym4
    psubw       ym2, ym4
    psubw       ym3, ym4
    mova        [r2 + 64],           ym0
    mova        [r2 + r3 + 64],      ym1
    mova        [r2 + r3 * 2 + 64],  ym2
    mova        [r2 + r6 + 64],      ym3

    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]

    pmovzxbw    m0, [r0]
    pmovzxbw    m1, [r0 + r1]
    pmovzxbw    m2, [r0 + r1 * 2]
    pmovzxbw    m3, [r0 + r5]
    psllw       m0, 6
    psllw       m1, 6
    psllw       m2, 6
    psllw       m3, 6
    psubw       m0, m4
    psubw       m1, m4
    psubw       m2, m4
    psubw       m3, m4
    mova        [r2],           m0
    mova        [r2 + r3],      m1
    mova        [r2 + r3 * 2],  m2
    mova        [r2 + r6],      m3

    pmovzxbw    ym0, [r0 + 32]
    pmovzxbw    ym1, [r0 + r1 + 32]
    pmovzxbw    ym2, [r0 + r1 * 2 + 32]
    pmovzxbw    ym3, [r0 + r5 + 32]
    psllw       ym0, 6
    psllw       ym1, 6
    psllw       ym2, 6
    psllw       ym3, 6
    psubw       ym0, ym4
    psubw       ym1, ym4
    psubw       ym2, ym4
    psubw       ym3, ym4
    mova        [r2 + 64],           ym0
    mova        [r2 + r3 + 64],      ym1
    mova        [r2 + r3 * 2 + 64],  ym2
    mova        [r2 + r6 + 64],      ym3
%endmacro
;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
%if ARCH_X86_64
INIT_ZMM avx512
cglobal filterPixelToShort_48x64, 3,7,5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd m4, [pw_2000]

    PROCESS_P2S_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_48x8_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_48x64, 3,7,5
    mov         r3d, r3m
    add         r3d, r3d
    lea         r5, [r1 * 3]
    lea         r6, [r3 * 3]

    ; load constant
    vpbroadcastd m4, [pw_2000]

    PROCESS_P2S_ALIGNED_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_ALIGNED_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_ALIGNED_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_ALIGNED_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_ALIGNED_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_ALIGNED_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_ALIGNED_48x8_AVX512
    lea         r0, [r0 + r1 * 4]
    lea         r2, [r2 + r3 * 4]
    PROCESS_P2S_ALIGNED_48x8_AVX512
    RET
%endif
;-----------------------------------------------------------------------------
;p2s and p2s_aligned 48xN avx512 code end
;-----------------------------------------------------------------------------

%macro PROCESS_LUMA_W4_4R 0
    movd        m0, [r0]
    movd        m1, [r0 + r1]
    punpcklbw   m2, m0, m1                     ; m2=[0 1]

    lea         r0, [r0 + 2 * r1]
    movd        m0, [r0]
    punpcklbw   m1, m0                         ; m1=[1 2]
    punpcklqdq  m2, m1                         ; m2=[0 1 1 2]
    pmaddubsw   m4, m2, [r6 + 0 * 16]          ; m4=[0+1 1+2]

    movd        m1, [r0 + r1]
    punpcklbw   m5, m0, m1                     ; m2=[2 3]
    lea         r0, [r0 + 2 * r1]
    movd        m0, [r0]
    punpcklbw   m1, m0                         ; m1=[3 4]
    punpcklqdq  m5, m1                         ; m5=[2 3 3 4]
    pmaddubsw   m2, m5, [r6 + 1 * 16]          ; m2=[2+3 3+4]
    paddw       m4, m2                         ; m4=[0+1+2+3 1+2+3+4]                   Row1-2
    pmaddubsw   m5, [r6 + 0 * 16]              ; m5=[2+3 3+4]                           Row3-4

    movd        m1, [r0 + r1]
    punpcklbw   m2, m0, m1                     ; m2=[4 5]
    lea         r0, [r0 + 2 * r1]
    movd        m0, [r0]
    punpcklbw   m1, m0                         ; m1=[5 6]
    punpcklqdq  m2, m1                         ; m2=[4 5 5 6]
    pmaddubsw   m1, m2, [r6 + 2 * 16]          ; m1=[4+5 5+6]
    paddw       m4, m1                         ; m4=[0+1+2+3+4+5 1+2+3+4+5+6]           Row1-2
    pmaddubsw   m2, [r6 + 1 * 16]              ; m2=[4+5 5+6]
    paddw       m5, m2                         ; m5=[2+3+4+5 3+4+5+6]                   Row3-4

    movd        m1, [r0 + r1]
    punpcklbw   m2, m0, m1                     ; m2=[6 7]
    lea         r0, [r0 + 2 * r1]
    movd        m0, [r0]
    punpcklbw   m1, m0                         ; m1=[7 8]
    punpcklqdq  m2, m1                         ; m2=[6 7 7 8]
    pmaddubsw   m1, m2, [r6 + 3 * 16]          ; m1=[6+7 7+8]
    paddw       m4, m1                         ; m4=[0+1+2+3+4+5+6+7 1+2+3+4+5+6+7+8]   Row1-2 end
    pmaddubsw   m2, [r6 + 2 * 16]              ; m2=[6+7 7+8]
    paddw       m5, m2                         ; m5=[2+3+4+5+6+7 3+4+5+6+7+8]           Row3-4

    movd        m1, [r0 + r1]
    punpcklbw   m2, m0, m1                     ; m2=[8 9]
    movd        m0, [r0 + 2 * r1]
    punpcklbw   m1, m0                         ; m1=[9 10]
    punpcklqdq  m2, m1                         ; m2=[8 9 9 10]
    pmaddubsw   m2, [r6 + 3 * 16]              ; m2=[8+9 9+10]
    paddw       m5, m2                         ; m5=[2+3+4+5+6+7+8+9 3+4+5+6+7+8+9+10]  Row3-4 end
%endmacro

%macro PROCESS_LUMA_W8_4R 0
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    punpcklbw  m0, m1
    pmaddubsw  m7, m0, [r6 + 0 *16]            ;m7=[0+1]               Row1

    lea        r0, [r0 + 2 * r1]
    movq       m0, [r0]
    punpcklbw  m1, m0
    pmaddubsw  m6, m1, [r6 + 0 *16]            ;m6=[1+2]               Row2

    movq       m1, [r0 + r1]
    punpcklbw  m0, m1
    pmaddubsw  m5, m0, [r6 + 0 *16]            ;m5=[2+3]               Row3
    pmaddubsw  m0, [r6 + 1 * 16]
    paddw      m7, m0                          ;m7=[0+1+2+3]           Row1

    lea        r0, [r0 + 2 * r1]
    movq       m0, [r0]
    punpcklbw  m1, m0
    pmaddubsw  m4, m1, [r6 + 0 *16]            ;m4=[3+4]               Row4
    pmaddubsw  m1, [r6 + 1 * 16]
    paddw      m6, m1                          ;m6 = [1+2+3+4]         Row2

    movq       m1, [r0 + r1]
    punpcklbw  m0, m1
    pmaddubsw  m2, m0, [r6 + 1 * 16]
    pmaddubsw  m0, [r6 + 2 * 16]
    paddw      m7, m0                          ;m7=[0+1+2+3+4+5]       Row1
    paddw      m5, m2                          ;m5=[2+3+4+5]           Row3

    lea        r0, [r0 + 2 * r1]
    movq       m0, [r0]
    punpcklbw  m1, m0
    pmaddubsw  m2, m1, [r6 + 1 * 16]
    pmaddubsw  m1, [r6 + 2 * 16]
    paddw      m6, m1                          ;m6=[1+2+3+4+5+6]       Row2
    paddw      m4, m2                          ;m4=[3+4+5+6]           Row4

    movq       m1, [r0 + r1]
    punpcklbw  m0, m1
    pmaddubsw  m2, m0, [r6 + 2 * 16]
    pmaddubsw  m0, [r6 + 3 * 16]
    paddw      m7, m0                          ;m7=[0+1+2+3+4+5+6+7]   Row1 end
    paddw      m5, m2                          ;m5=[2+3+4+5+6+7]       Row3

    lea        r0, [r0 + 2 * r1]
    movq       m0, [r0]
    punpcklbw  m1, m0
    pmaddubsw  m2, m1, [r6 + 2 * 16]
    pmaddubsw  m1, [r6 + 3 * 16]
    paddw      m6, m1                          ;m6=[1+2+3+4+5+6+7+8]   Row2 end
    paddw      m4, m2                          ;m4=[3+4+5+6+7+8]       Row4

    movq       m1, [r0 + r1]
    punpcklbw  m0, m1
    pmaddubsw  m0, [r6 + 3 * 16]
    paddw      m5, m0                          ;m5=[2+3+4+5+6+7+8+9]   Row3 end

    movq       m0, [r0 + 2 * r1]
    punpcklbw  m1, m0
    pmaddubsw  m1, [r6 + 3 * 16]
    paddw      m4, m1                          ;m4=[3+4+5+6+7+8+9+10]  Row4 end
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_%3_4x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_4xN 3
INIT_XMM sse4
cglobal interp_8tap_vert_%3_%1x%2, 5, 7, 6
    lea       r5, [3 * r1]
    sub       r0, r5
    shl       r4d, 6
%ifidn %3,ps
    add       r3d, r3d
%endif

%ifdef PIC
    lea       r5, [tab_LumaCoeffVer]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_LumaCoeffVer + r4]
%endif

%ifidn %3,pp
    mova      m3, [pw_512]
%else
    mova      m3, [pw_2000]
%endif

    mov       r4d, %2/4
    lea       r5, [4 * r1]

.loopH:
    PROCESS_LUMA_W4_4R

%ifidn %3,pp
    pmulhrsw  m4, m3
    pmulhrsw  m5, m3

    packuswb  m4, m5

    movd      [r2], m4
    pextrd    [r2 + r3], m4, 1
    lea       r2, [r2 + 2 * r3]
    pextrd    [r2], m4, 2
    pextrd    [r2 + r3], m4, 3
%else
    psubw     m4, m3
    psubw     m5, m3

    movlps    [r2], m4
    movhps    [r2 + r3], m4
    lea       r2, [r2 + 2 * r3]
    movlps    [r2], m5
    movhps    [r2 + r3], m5
%endif

    sub       r0, r5
    lea       r2, [r2 + 2 * r3]

    dec       r4d
    jnz       .loopH

    RET
%endmacro


INIT_YMM avx2
cglobal interp_8tap_vert_pp_4x4, 4,6,8
    mov             r4d, r4m
    lea             r5, [r1 * 3]
    sub             r0, r5

    ; TODO: VPGATHERDD
    movd            xm1, [r0]                       ; m1 = row0
    movd            xm2, [r0 + r1]                  ; m2 = row1
    punpcklbw       xm1, xm2                        ; m1 = [13 03 12 02 11 01 10 00]

    movd            xm3, [r0 + r1 * 2]              ; m3 = row2
    punpcklbw       xm2, xm3                        ; m2 = [23 13 22 12 21 11 20 10]
    movd            xm4, [r0 + r5]
    punpcklbw       xm3, xm4                        ; m3 = [33 23 32 22 31 21 30 20]
    punpcklwd       xm1, xm3                        ; m1 = [33 23 13 03 32 22 12 02 31 21 11 01 30 20 10 00]

    lea             r0, [r0 + r1 * 4]
    movd            xm5, [r0]                       ; m5 = row4
    punpcklbw       xm4, xm5                        ; m4 = [43 33 42 32 41 31 40 30]
    punpcklwd       xm2, xm4                        ; m2 = [43 33 21 13 42 32 22 12 41 31 21 11 40 30 20 10]
    vinserti128     m1, m1, xm2, 1                  ; m1 = [43 33 21 13 42 32 22 12 41 31 21 11 40 30 20 10] - [33 23 13 03 32 22 12 02 31 21 11 01 30 20 10 00]
    movd            xm2, [r0 + r1]                  ; m2 = row5
    punpcklbw       xm5, xm2                        ; m5 = [53 43 52 42 51 41 50 40]
    punpcklwd       xm3, xm5                        ; m3 = [53 43 44 23 52 42 32 22 51 41 31 21 50 40 30 20]
    movd            xm6, [r0 + r1 * 2]              ; m6 = row6
    punpcklbw       xm2, xm6                        ; m2 = [63 53 62 52 61 51 60 50]
    punpcklwd       xm4, xm2                        ; m4 = [63 53 43 33 62 52 42 32 61 51 41 31 60 50 40 30]
    vinserti128     m3, m3, xm4, 1                  ; m3 = [63 53 43 33 62 52 42 32 61 51 41 31 60 50 40 30] - [53 43 44 23 52 42 32 22 51 41 31 21 50 40 30 20]
    movd            xm4, [r0 + r5]                  ; m4 = row7
    punpcklbw       xm6, xm4                        ; m6 = [73 63 72 62 71 61 70 60]
    punpcklwd       xm5, xm6                        ; m5 = [73 63 53 43 72 62 52 42 71 61 51 41 70 60 50 40]

    lea             r0, [r0 + r1 * 4]
    movd            xm7, [r0]                       ; m7 = row8
    punpcklbw       xm4, xm7                        ; m4 = [83 73 82 72 81 71 80 70]
    punpcklwd       xm2, xm4                        ; m2 = [83 73 63 53 82 72 62 52 81 71 61 51 80 70 60 50]
    vinserti128     m5, m5, xm2, 1                  ; m5 = [83 73 63 53 82 72 62 52 81 71 61 51 80 70 60 50] - [73 63 53 43 72 62 52 42 71 61 51 41 70 60 50 40]
    movd            xm2, [r0 + r1]                  ; m2 = row9
    punpcklbw       xm7, xm2                        ; m7 = [93 83 92 82 91 81 90 80]
    punpcklwd       xm6, xm7                        ; m6 = [93 83 73 63 92 82 72 62 91 81 71 61 90 80 70 60]
    movd            xm7, [r0 + r1 * 2]              ; m7 = rowA
    punpcklbw       xm2, xm7                        ; m2 = [A3 93 A2 92 A1 91 A0 90]
    punpcklwd       xm4, xm2                        ; m4 = [A3 93 83 73 A2 92 82 72 A1 91 81 71 A0 90 80 70]
    vinserti128     m6, m6, xm4, 1                  ; m6 = [A3 93 83 73 A2 92 82 72 A1 91 81 71 A0 90 80 70] - [93 83 73 63 92 82 72 62 91 81 71 61 90 80 70 60]

    ; load filter coeff
%ifdef PIC
    lea             r5, [tab_LumaCoeff]
    vpbroadcastd    m0, [r5 + r4 * 8 + 0]
    vpbroadcastd    m2, [r5 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_LumaCoeff + r4 * 8 + 0]
    vpbroadcastd    m2, [tab_LumaCoeff + r4 * 8 + 4]
%endif

    pmaddubsw       m1, m0
    pmaddubsw       m3, m0
    pmaddubsw       m5, m2
    pmaddubsw       m6, m2
    vbroadcasti128  m0, [pw_1]
    pmaddwd         m1, m0
    pmaddwd         m3, m0
    pmaddwd         m5, m0
    pmaddwd         m6, m0
    paddd           m1, m5                          ; m1 = DQWORD ROW[1 0]
    paddd           m3, m6                          ; m3 = DQWORD ROW[3 2]
    packssdw        m1, m3                          ; m1 =  QWORD ROW[3 1 2 0]

    ; TODO: does it overflow?
    pmulhrsw        m1, [pw_512]
    vextracti128    xm2, m1, 1
    packuswb        xm1, xm2                        ; m1 =  DWORD ROW[3 1 2 0]
    movd            [r2], xm1
    pextrd          [r2 + r3], xm1, 2
    pextrd          [r2 + r3 * 2], xm1, 1
    lea             r4, [r3 * 3]
    pextrd          [r2 + r4], xm1, 3
    RET

INIT_YMM avx2
cglobal interp_8tap_vert_ps_4x4, 4, 6, 5
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4

    add             r3d, r3d

    movd            xm1, [r0]
    pinsrd          xm1, [r0 + r1], 1
    pinsrd          xm1, [r0 + r1 * 2], 2
    pinsrd          xm1, [r0 + r4], 3                       ; m1 = row[3 2 1 0]
    lea             r0, [r0 + r1 * 4]
    movd            xm2, [r0]
    pinsrd          xm2, [r0 + r1], 1
    pinsrd          xm2, [r0 + r1 * 2], 2
    pinsrd          xm2, [r0 + r4], 3                       ; m2 = row[7 6 5 4]
    vinserti128     m1, m1, xm2, 1                          ; m1 = row[7 6 5 4 3 2 1 0]
    lea             r0, [r0 + r1 * 4]
    movd            xm3, [r0]
    pinsrd          xm3, [r0 + r1], 1
    pinsrd          xm3, [r0 + r1 * 2], 2                   ; m3 = row[x 10 9 8]
    vinserti128     m2, m2, xm3, 1                          ; m2 = row[x 10 9 8 7 6 5 4]
    mova            m3, [interp4_vpp_shuf1]
    vpermd          m0, m3, m1                              ; m0 = row[4 3 3 2 2 1 1 0]
    vpermd          m4, m3, m2                              ; m4 = row[8 7 7 6 6 5 5 4]
    mova            m3, [interp4_vpp_shuf1 + mmsize]
    vpermd          m1, m3, m1                              ; m1 = row[6 5 5 4 4 3 3 2]
    vpermd          m2, m3, m2                              ; m2 = row[10 9 9 8 8 7 7 6]

    mova            m3, [interp4_vpp_shuf]
    pshufb          m0, m0, m3
    pshufb          m1, m1, m3
    pshufb          m4, m4, m3
    pshufb          m2, m2, m3
    pmaddubsw       m0, [r5]
    pmaddubsw       m1, [r5 + mmsize]
    pmaddubsw       m4, [r5 + 2 * mmsize]
    pmaddubsw       m2, [r5 + 3 * mmsize]
    paddw           m0, m1
    paddw           m0, m4
    paddw           m0, m2                                  ; m0 = WORD ROW[3 2 1 0]

    psubw           m0, [pw_2000]
    vextracti128    xm2, m0, 1
    lea             r5, [r3 * 3]
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r5], xm2
    RET

%macro FILTER_VER_LUMA_AVX2_4xN 3
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%3_%1x%2, 4, 9, 10
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
    lea             r6, [r1 * 4]
%ifidn %3,pp
    mova            m6, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m6, [pw_2000]
%endif
    lea             r8, [r3 * 3]
    mova            m5, [interp4_vpp_shuf]
    mova            m0, [interp4_vpp_shuf1]
    mova            m7, [interp4_vpp_shuf1 + mmsize]
    mov             r7d, %2 / 8
.loop:
    movd            xm1, [r0]
    pinsrd          xm1, [r0 + r1], 1
    pinsrd          xm1, [r0 + r1 * 2], 2
    pinsrd          xm1, [r0 + r4], 3                       ; m1 = row[3 2 1 0]
    lea             r0, [r0 + r1 * 4]
    movd            xm2, [r0]
    pinsrd          xm2, [r0 + r1], 1
    pinsrd          xm2, [r0 + r1 * 2], 2
    pinsrd          xm2, [r0 + r4], 3                       ; m2 = row[7 6 5 4]
    vinserti128     m1, m1, xm2, 1                          ; m1 = row[7 6 5 4 3 2 1 0]
    lea             r0, [r0 + r1 * 4]
    movd            xm3, [r0]
    pinsrd          xm3, [r0 + r1], 1
    pinsrd          xm3, [r0 + r1 * 2], 2
    pinsrd          xm3, [r0 + r4], 3                       ; m3 = row[11 10 9 8]
    vinserti128     m2, m2, xm3, 1                          ; m2 = row[11 10 9 8 7 6 5 4]
    lea             r0, [r0 + r1 * 4]
    movd            xm4, [r0]
    pinsrd          xm4, [r0 + r1], 1
    pinsrd          xm4, [r0 + r1 * 2], 2                   ; m4 = row[x 14 13 12]
    vinserti128     m3, m3, xm4, 1                          ; m3 = row[x 14 13 12 11 10 9 8]
    vpermd          m8, m0, m1                              ; m8 = row[4 3 3 2 2 1 1 0]
    vpermd          m4, m0, m2                              ; m4 = row[8 7 7 6 6 5 5 4]
    vpermd          m1, m7, m1                              ; m1 = row[6 5 5 4 4 3 3 2]
    vpermd          m2, m7, m2                              ; m2 = row[10 9 9 8 8 7 7 6]
    vpermd          m9, m0, m3                              ; m9 = row[12 11 11 10 10 9 9 8]
    vpermd          m3, m7, m3                              ; m3 = row[14 13 13 12 12 11 11 10]

    pshufb          m8, m8, m5
    pshufb          m1, m1, m5
    pshufb          m4, m4, m5
    pshufb          m9, m9, m5
    pshufb          m2, m2, m5
    pshufb          m3, m3, m5
    pmaddubsw       m8, [r5]
    pmaddubsw       m1, [r5 + mmsize]
    pmaddubsw       m9, [r5 + 2 * mmsize]
    pmaddubsw       m3, [r5 + 3 * mmsize]
    paddw           m8, m1
    paddw           m9, m3
    pmaddubsw       m1, m4, [r5 + 2 * mmsize]
    pmaddubsw       m3, m2, [r5 + 3 * mmsize]
    pmaddubsw       m4, [r5]
    pmaddubsw       m2, [r5 + mmsize]
    paddw           m3, m1
    paddw           m2, m4
    paddw           m8, m3                                  ; m8 = WORD ROW[3 2 1 0]
    paddw           m9, m2                                  ; m9 = WORD ROW[7 6 5 4]

%ifidn %3,pp
    pmulhrsw        m8, m6
    pmulhrsw        m9, m6
    packuswb        m8, m9
    vextracti128    xm1, m8, 1
    movd            [r2], xm8
    pextrd          [r2 + r3], xm8, 1
    movd            [r2 + r3 * 2], xm1
    pextrd          [r2 + r8], xm1, 1
    lea             r2, [r2 + r3 * 4]
    pextrd          [r2], xm8, 2
    pextrd          [r2 + r3], xm8, 3
    pextrd          [r2 + r3 * 2], xm1, 2
    pextrd          [r2 + r8], xm1, 3
%else
    psubw           m8, m6
    psubw           m9, m6
    vextracti128    xm1, m8, 1
    vextracti128    xm2, m9, 1
    movq            [r2], xm8
    movhps          [r2 + r3], xm8
    movq            [r2 + r3 * 2], xm1
    movhps          [r2 + r8], xm1
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm9
    movhps          [r2 + r3], xm9
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r8], xm2
%endif
    lea             r2, [r2 + r3 * 4]
    sub             r0, r6
    dec             r7d
    jnz             .loop
    RET
%endif
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_4x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_4xN 4, 4, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_4x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_4xN 4, 8, pp
    FILTER_VER_LUMA_AVX2_4xN 4, 8, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_4x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_4xN 4, 16, pp
    FILTER_VER_LUMA_AVX2_4xN 4, 16, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_4x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_4xN 4, 4, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_4x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_4xN 4, 8, ps
    FILTER_VER_LUMA_AVX2_4xN 4, 8, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_4x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_4xN 4, 16, ps
    FILTER_VER_LUMA_AVX2_4xN 4, 16, ps

%macro PROCESS_LUMA_AVX2_W8_8R 0
    movq            xm1, [r0]                       ; m1 = row 0
    movq            xm2, [r0 + r1]                  ; m2 = row 1
    punpcklbw       xm1, xm2                        ; m1 = [17 07 16 06 15 05 14 04 13 03 12 02 11 01 10 00]
    movq            xm3, [r0 + r1 * 2]              ; m3 = row 2
    punpcklbw       xm2, xm3                        ; m2 = [27 17 26 16 25 15 24 14 23 13 22 12 21 11 20 10]
    vinserti128     m5, m1, xm2, 1                  ; m5 = [27 17 26 16 25 15 24 14 23 13 22 12 21 11 20 10] - [17 07 16 06 15 05 14 04 13 03 12 02 11 01 10 00]
    pmaddubsw       m5, [r5]
    movq            xm4, [r0 + r4]                  ; m4 = row 3
    punpcklbw       xm3, xm4                        ; m3 = [37 27 36 26 35 25 34 24 33 23 32 22 31 21 30 20]
    lea             r0, [r0 + r1 * 4]
    movq            xm1, [r0]                       ; m1 = row 4
    punpcklbw       xm4, xm1                        ; m4 = [47 37 46 36 45 35 44 34 43 33 42 32 41 31 40 30]
    vinserti128     m2, m3, xm4, 1                  ; m2 = [47 37 46 36 45 35 44 34 43 33 42 32 41 31 40 30] - [37 27 36 26 35 25 34 24 33 23 32 22 31 21 30 20]
    pmaddubsw       m0, m2, [r5 + 1 * mmsize]
    paddw           m5, m0
    pmaddubsw       m2, [r5]
    movq            xm3, [r0 + r1]                  ; m3 = row 5
    punpcklbw       xm1, xm3                        ; m1 = [57 47 56 46 55 45 54 44 53 43 52 42 51 41 50 40]
    movq            xm4, [r0 + r1 * 2]              ; m4 = row 6
    punpcklbw       xm3, xm4                        ; m3 = [67 57 66 56 65 55 64 54 63 53 62 52 61 51 60 50]
    vinserti128     m1, m1, xm3, 1                  ; m1 = [67 57 66 56 65 55 64 54 63 53 62 52 61 51 60 50] - [57 47 56 46 55 45 54 44 53 43 52 42 51 41 50 40]
    pmaddubsw       m3, m1, [r5 + 2 * mmsize]
    paddw           m5, m3
    pmaddubsw       m0, m1, [r5 + 1 * mmsize]
    paddw           m2, m0
    pmaddubsw       m1, [r5]
    movq            xm3, [r0 + r4]                  ; m3 = row 7
    punpcklbw       xm4, xm3                        ; m4 = [77 67 76 66 75 65 74 64 73 63 72 62 71 61 70 60]
    lea             r0, [r0 + r1 * 4]
    movq            xm0, [r0]                       ; m0 = row 8
    punpcklbw       xm3, xm0                        ; m3 = [87 77 86 76 85 75 84 74 83 73 82 72 81 71 80 70]
    vinserti128     m4, m4, xm3, 1                  ; m4 = [87 77 86 76 85 75 84 74 83 73 82 72 81 71 80 70] - [77 67 76 66 75 65 74 64 73 63 72 62 71 61 70 60]
    pmaddubsw       m3, m4, [r5 + 3 * mmsize]
    paddw           m5, m3
    pmaddubsw       m3, m4, [r5 + 2 * mmsize]
    paddw           m2, m3
    pmaddubsw       m3, m4, [r5 + 1 * mmsize]
    paddw           m1, m3
    pmaddubsw       m4, [r5]
    movq            xm3, [r0 + r1]                  ; m3 = row 9
    punpcklbw       xm0, xm3                        ; m0 = [97 87 96 86 95 85 94 84 93 83 92 82 91 81 90 80]
    movq            xm6, [r0 + r1 * 2]              ; m6 = row 10
    punpcklbw       xm3, xm6                        ; m3 = [A7 97 A6 96 A5 95 A4 94 A3 93 A2 92 A1 91 A0 90]
    vinserti128     m0, m0, xm3, 1                  ; m0 = [A7 97 A6 96 A5 95 A4 94 A3 93 A2 92 A1 91 A0 90] - [97 87 96 86 95 85 94 84 93 83 92 82 91 81 90 80]
    pmaddubsw       m3, m0, [r5 + 3 * mmsize]
    paddw           m2, m3
    pmaddubsw       m3, m0, [r5 + 2 * mmsize]
    paddw           m1, m3
    pmaddubsw       m0, [r5 + 1 * mmsize]
    paddw           m4, m0

    movq            xm3, [r0 + r4]                  ; m3 = row 11
    punpcklbw       xm6, xm3                        ; m6 = [B7 A7 B6 A6 B5 A5 B4 A4 B3 A3 B2 A2 B1 A1 B0 A0]
    lea             r0, [r0 + r1 * 4]
    movq            xm0, [r0]                       ; m0 = row 12
    punpcklbw       xm3, xm0                        ; m3 = [C7 B7 C6 B6 C5 B5 C4 B4 C3 B3 C2 B2 C1 B1 C0 B0]
    vinserti128     m6, m6, xm3, 1                  ; m6 = [C7 B7 C6 B6 C5 B5 C4 B4 C3 B3 C2 B2 C1 B1 C0 B0] - [B7 A7 B6 A6 B5 A5 B4 A4 B3 A3 B2 A2 B1 A1 B0 A0]
    pmaddubsw       m3, m6, [r5 + 3 * mmsize]
    paddw           m1, m3
    pmaddubsw       m6, [r5 + 2 * mmsize]
    paddw           m4, m6
    movq            xm3, [r0 + r1]                  ; m3 = row 13
    punpcklbw       xm0, xm3                        ; m0 = [D7 C7 D6 C6 D5 C5 D4 C4 D3 C3 D2 C2 D1 C1 D0 C0]
    movq            xm6, [r0 + r1 * 2]              ; m6 = row 14
    punpcklbw       xm3, xm6                        ; m3 = [E7 D7 E6 D6 E5 D5 E4 D4 E3 D3 E2 D2 E1 D1 E0 D0]
    vinserti128     m0, m0, xm3, 1                  ; m0 = [E7 D7 E6 D6 E5 D5 E4 D4 E3 D3 E2 D2 E1 D1 E0 D0] - [D7 C7 D6 C6 D5 C5 D4 C4 D3 C3 D2 C2 D1 C1 D0 C0]
    pmaddubsw       m0, [r5 + 3 * mmsize]
    paddw           m4, m0
%endmacro

%macro PROCESS_LUMA_AVX2_W8_4R 0
    movq            xm1, [r0]                       ; m1 = row 0
    movq            xm2, [r0 + r1]                  ; m2 = row 1
    punpcklbw       xm1, xm2                        ; m1 = [17 07 16 06 15 05 14 04 13 03 12 02 11 01 10 00]
    movq            xm3, [r0 + r1 * 2]              ; m3 = row 2
    punpcklbw       xm2, xm3                        ; m2 = [27 17 26 16 25 15 24 14 23 13 22 12 21 11 20 10]
    vinserti128     m5, m1, xm2, 1                  ; m5 = [27 17 26 16 25 15 24 14 23 13 22 12 21 11 20 10] - [17 07 16 06 15 05 14 04 13 03 12 02 11 01 10 00]
    pmaddubsw       m5, [r5]
    movq            xm4, [r0 + r4]                  ; m4 = row 3
    punpcklbw       xm3, xm4                        ; m3 = [37 27 36 26 35 25 34 24 33 23 32 22 31 21 30 20]
    lea             r0, [r0 + r1 * 4]
    movq            xm1, [r0]                       ; m1 = row 4
    punpcklbw       xm4, xm1                        ; m4 = [47 37 46 36 45 35 44 34 43 33 42 32 41 31 40 30]
    vinserti128     m2, m3, xm4, 1                  ; m2 = [47 37 46 36 45 35 44 34 43 33 42 32 41 31 40 30] - [37 27 36 26 35 25 34 24 33 23 32 22 31 21 30 20]
    pmaddubsw       m0, m2, [r5 + 1 * mmsize]
    paddw           m5, m0
    pmaddubsw       m2, [r5]
    movq            xm3, [r0 + r1]                  ; m3 = row 5
    punpcklbw       xm1, xm3                        ; m1 = [57 47 56 46 55 45 54 44 53 43 52 42 51 41 50 40]
    movq            xm4, [r0 + r1 * 2]              ; m4 = row 6
    punpcklbw       xm3, xm4                        ; m3 = [67 57 66 56 65 55 64 54 63 53 62 52 61 51 60 50]
    vinserti128     m1, m1, xm3, 1                  ; m1 = [67 57 66 56 65 55 64 54 63 53 62 52 61 51 60 50] - [57 47 56 46 55 45 54 44 53 43 52 42 51 41 50 40]
    pmaddubsw       m3, m1, [r5 + 2 * mmsize]
    paddw           m5, m3
    pmaddubsw       m0, m1, [r5 + 1 * mmsize]
    paddw           m2, m0
    movq            xm3, [r0 + r4]                  ; m3 = row 7
    punpcklbw       xm4, xm3                        ; m4 = [77 67 76 66 75 65 74 64 73 63 72 62 71 61 70 60]
    lea             r0, [r0 + r1 * 4]
    movq            xm0, [r0]                       ; m0 = row 8
    punpcklbw       xm3, xm0                        ; m3 = [87 77 86 76 85 75 84 74 83 73 82 72 81 71 80 70]
    vinserti128     m4, m4, xm3, 1                  ; m4 = [87 77 86 76 85 75 84 74 83 73 82 72 81 71 80 70] - [77 67 76 66 75 65 74 64 73 63 72 62 71 61 70 60]
    pmaddubsw       m3, m4, [r5 + 3 * mmsize]
    paddw           m5, m3
    pmaddubsw       m3, m4, [r5 + 2 * mmsize]
    paddw           m2, m3
    movq            xm3, [r0 + r1]                  ; m3 = row 9
    punpcklbw       xm0, xm3                        ; m0 = [97 87 96 86 95 85 94 84 93 83 92 82 91 81 90 80]
    movq            xm6, [r0 + r1 * 2]              ; m6 = row 10
    punpcklbw       xm3, xm6                        ; m3 = [A7 97 A6 96 A5 95 A4 94 A3 93 A2 92 A1 91 A0 90]
    vinserti128     m0, m0, xm3, 1                  ; m0 = [A7 97 A6 96 A5 95 A4 94 A3 93 A2 92 A1 91 A0 90] - [97 87 96 86 95 85 94 84 93 83 92 82 91 81 90 80]
    pmaddubsw       m3, m0, [r5 + 3 * mmsize]
    paddw           m2, m3
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_%3_8x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_8xN 3
INIT_XMM sse4
cglobal interp_8tap_vert_%3_%1x%2, 5, 7, 8
    lea       r5, [3 * r1]
    sub       r0, r5
    shl       r4d, 6

%ifidn %3,ps
    add       r3d, r3d
%endif

%ifdef PIC
    lea       r5, [tab_LumaCoeffVer]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_LumaCoeffVer + r4]
%endif

 %ifidn %3,pp
    mova      m3, [pw_512]
%else
    mova      m3, [pw_2000]
%endif

    mov       r4d, %2/4
    lea       r5, [4 * r1]

.loopH:
    PROCESS_LUMA_W8_4R

%ifidn %3,pp
    pmulhrsw  m7, m3
    pmulhrsw  m6, m3
    pmulhrsw  m5, m3
    pmulhrsw  m4, m3

    packuswb  m7, m6
    packuswb  m5, m4

    movlps    [r2], m7
    movhps    [r2 + r3], m7
    lea       r2, [r2 + 2 * r3]
    movlps    [r2], m5
    movhps    [r2 + r3], m5
%else
    psubw     m7, m3
    psubw     m6, m3
    psubw     m5, m3
    psubw     m4, m3

    movu      [r2], m7
    movu      [r2 + r3], m6
    lea       r2, [r2 + 2 * r3]
    movu      [r2], m5
    movu      [r2 + r3], m4
%endif

    sub       r0, r5
    lea       r2, [r2 + 2 * r3]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

%macro FILTER_VER_LUMA_AVX2_8xN 3
INIT_YMM avx2
cglobal interp_8tap_vert_%3_%1x%2, 4, 7, 8, 0-gprsize
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif
    lea             r4, [r1 * 3]
    sub             r0, r4
    lea             r6, [r1 * 4]
%ifidn %3,pp
    mova            m7, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m7, [pw_2000]
%endif
    mov             word [rsp], %2 / 8

.loop:
    PROCESS_LUMA_AVX2_W8_8R
%ifidn %3,pp
    pmulhrsw        m5, m7                          ; m5 = word: row 0, row 1
    pmulhrsw        m2, m7                          ; m2 = word: row 2, row 3
    pmulhrsw        m1, m7                          ; m1 = word: row 4, row 5
    pmulhrsw        m4, m7                          ; m4 = word: row 6, row 7
    packuswb        m5, m2
    packuswb        m1, m4
    vextracti128    xm2, m5, 1
    vextracti128    xm4, m1, 1
    movq            [r2], xm5
    movq            [r2 + r3], xm2
    lea             r2, [r2 + r3 * 2]
    movhps          [r2], xm5
    movhps          [r2 + r3], xm2
    lea             r2, [r2 + r3 * 2]
    movq            [r2], xm1
    movq            [r2 + r3], xm4
    lea             r2, [r2 + r3 * 2]
    movhps          [r2], xm1
    movhps          [r2 + r3], xm4
%else
    psubw           m5, m7                          ; m5 = word: row 0, row 1
    psubw           m2, m7                          ; m2 = word: row 2, row 3
    psubw           m1, m7                          ; m1 = word: row 4, row 5
    psubw           m4, m7                          ; m4 = word: row 6, row 7
    vextracti128    xm6, m5, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm0, m1, 1
    movu            [r2], xm5
    movu            [r2 + r3], xm6
    lea             r2, [r2 + r3 * 2]
    movu            [r2], xm2
    movu            [r2 + r3], xm3
    lea             r2, [r2 + r3 * 2]
    movu            [r2], xm1
    movu            [r2 + r3], xm0
    lea             r2, [r2 + r3 * 2]
    movu            [r2], xm4
    vextracti128    xm4, m4, 1
    movu            [r2 + r3], xm4
%endif
    lea             r2, [r2 + r3 * 2]
    sub             r0, r6
    dec             word [rsp]
    jnz             .loop
    RET
%endmacro

%macro FILTER_VER_LUMA_AVX2_8x8 1
INIT_YMM avx2
cglobal interp_8tap_vert_%1_8x8, 4, 6, 7
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
    PROCESS_LUMA_AVX2_W8_8R
%ifidn %1,pp
    mova            m3, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m3, [pw_2000]
%endif
    lea             r4, [r3 * 3]
%ifidn %1,pp
    pmulhrsw        m5, m3                          ; m5 = word: row 0, row 1
    pmulhrsw        m2, m3                          ; m2 = word: row 2, row 3
    pmulhrsw        m1, m3                          ; m1 = word: row 4, row 5
    pmulhrsw        m4, m3                          ; m4 = word: row 6, row 7
    packuswb        m5, m2
    packuswb        m1, m4
    vextracti128    xm2, m5, 1
    vextracti128    xm4, m1, 1
    movq            [r2], xm5
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm5
    movhps          [r2 + r4], xm2
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm1
    movq            [r2 + r3], xm4
    movhps          [r2 + r3 * 2], xm1
    movhps          [r2 + r4], xm4
%else
    psubw           m5, m3                          ; m5 = word: row 0, row 1
    psubw           m2, m3                          ; m2 = word: row 2, row 3
    psubw           m1, m3                          ; m1 = word: row 4, row 5
    psubw           m4, m3                          ; m4 = word: row 6, row 7
    vextracti128    xm6, m5, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm0, m1, 1
    movu            [r2], xm5
    movu            [r2 + r3], xm6
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r4], xm3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm1
    movu            [r2 + r3], xm0
    movu            [r2 + r3 * 2], xm4
    vextracti128    xm4, m4, 1
    movu            [r2 + r4], xm4
%endif
    RET
%endmacro

%macro FILTER_VER_LUMA_AVX2_8x4 1
INIT_YMM avx2
cglobal interp_8tap_vert_%1_8x4, 4, 6, 7
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
    PROCESS_LUMA_AVX2_W8_4R
%ifidn %1,pp
    mova            m3, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m3, [pw_2000]
%endif
    lea             r4, [r3 * 3]
%ifidn %1,pp
    pmulhrsw        m5, m3                          ; m5 = word: row 0, row 1
    pmulhrsw        m2, m3                          ; m2 = word: row 2, row 3
    packuswb        m5, m2
    vextracti128    xm2, m5, 1
    movq            [r2], xm5
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm5
    movhps          [r2 + r4], xm2
%else
    psubw           m5, m3                          ; m5 = word: row 0, row 1
    psubw           m2, m3                          ; m2 = word: row 2, row 3
    movu            [r2], xm5
    vextracti128    xm5, m5, 1
    movu            [r2 + r3], xm5
    movu            [r2 + r3 * 2], xm2
    vextracti128    xm2, m2, 1
    movu            [r2 + r4], xm2
%endif
    RET
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_8x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_8xN 8, 4, pp
    FILTER_VER_LUMA_AVX2_8x4 pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_8x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_8xN 8, 8, pp
    FILTER_VER_LUMA_AVX2_8x8 pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_8x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_8xN 8, 16, pp
    FILTER_VER_LUMA_AVX2_8xN 8, 16, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_8x32(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_8xN 8, 32, pp
    FILTER_VER_LUMA_AVX2_8xN 8, 32, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_8x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_8xN 8, 4, ps
    FILTER_VER_LUMA_AVX2_8x4 ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_8x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_8xN 8, 8, ps
    FILTER_VER_LUMA_AVX2_8x8 ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_8x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_8xN 8, 16, ps
    FILTER_VER_LUMA_AVX2_8xN 8, 16, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_8x32(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_8xN 8, 32, ps
    FILTER_VER_LUMA_AVX2_8xN 8, 32, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_%3_12x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_12xN 3
INIT_XMM sse4
cglobal interp_8tap_vert_%3_%1x%2, 5, 7, 8
    lea       r5, [3 * r1]
    sub       r0, r5
    shl       r4d, 6
%ifidn %3,ps
    add       r3d, r3d
%endif

%ifdef PIC
    lea       r5, [tab_LumaCoeffVer]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_LumaCoeffVer + r4]
%endif

 %ifidn %3,pp
    mova      m3, [pw_512]
%else
    mova      m3, [pw_2000]
%endif

    mov       r4d, %2/4

.loopH:
    PROCESS_LUMA_W8_4R

%ifidn %3,pp
    pmulhrsw  m7, m3
    pmulhrsw  m6, m3
    pmulhrsw  m5, m3
    pmulhrsw  m4, m3

    packuswb  m7, m6
    packuswb  m5, m4

    movlps    [r2], m7
    movhps    [r2 + r3], m7
    lea       r5, [r2 + 2 * r3]
    movlps    [r5], m5
    movhps    [r5 + r3], m5
%else
    psubw     m7, m3
    psubw     m6, m3
    psubw     m5, m3
    psubw     m4, m3

    movu      [r2], m7
    movu      [r2 + r3], m6
    lea       r5, [r2 + 2 * r3]
    movu      [r5], m5
    movu      [r5 + r3], m4
%endif

    lea       r5, [8 * r1 - 8]
    sub       r0, r5
%ifidn %3,pp
    add       r2, 8
%else
    add       r2, 16
%endif

    PROCESS_LUMA_W4_4R

%ifidn %3,pp
    pmulhrsw  m4, m3
    pmulhrsw  m5, m3

    packuswb  m4, m5

    movd      [r2], m4
    pextrd    [r2 + r3], m4, 1
    lea       r5, [r2 + 2 * r3]
    pextrd    [r5], m4, 2
    pextrd    [r5 + r3], m4, 3
%else
    psubw     m4, m3
    psubw     m5, m3

    movlps    [r2], m4
    movhps    [r2 + r3], m4
    lea       r5, [r2 + 2 * r3]
    movlps    [r5], m5
    movhps    [r5 + r3], m5
%endif

    lea       r5, [4 * r1 + 8]
    sub       r0, r5
%ifidn %3,pp
    lea       r2, [r2 + 4 * r3 - 8]
%else
    lea       r2, [r2 + 4 * r3 - 16]
%endif

    dec       r4d
    jnz       .loopH

    RET
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_12x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_12xN 12, 16, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_12x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_12xN 12, 16, ps

%macro FILTER_VER_LUMA_AVX2_12x16 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_12x16, 4, 7, 15
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    mova            m14, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m14, [pw_2000]
%endif
    lea             r6, [r3 * 3]

    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 1 * mmsize]
    paddw           m0, m4
    pmaddubsw       m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 1 * mmsize]
    paddw           m1, m5
    pmaddubsw       m3, [r5]
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 2 * mmsize]
    paddw           m0, m6
    pmaddubsw       m6, m4, [r5 + 1 * mmsize]
    paddw           m2, m6
    pmaddubsw       m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, [r5 + 2 * mmsize]
    paddw           m1, m7
    pmaddubsw       m7, m5, [r5 + 1 * mmsize]
    paddw           m3, m7
    pmaddubsw       m5, [r5]
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhbw       xm8, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddubsw       m8, m6, [r5 + 3 * mmsize]
    paddw           m0, m8
    pmaddubsw       m8, m6, [r5 + 2 * mmsize]
    paddw           m2, m8
    pmaddubsw       m8, m6, [r5 + 1 * mmsize]
    paddw           m4, m8
    pmaddubsw       m6, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhbw       xm9, xm7, xm8
    punpcklbw       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddubsw       m9, m7, [r5 + 3 * mmsize]
    paddw           m1, m9
    pmaddubsw       m9, m7, [r5 + 2 * mmsize]
    paddw           m3, m9
    pmaddubsw       m9, m7, [r5 + 1 * mmsize]
    paddw           m5, m9
    pmaddubsw       m7, [r5]
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhbw       xm10, xm8, xm9
    punpcklbw       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddubsw       m10, m8, [r5 + 3 * mmsize]
    paddw           m2, m10
    pmaddubsw       m10, m8, [r5 + 2 * mmsize]
    paddw           m4, m10
    pmaddubsw       m10, m8, [r5 + 1 * mmsize]
    paddw           m6, m10
    pmaddubsw       m8, [r5]
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhbw       xm11, xm9, xm10
    punpcklbw       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddubsw       m11, m9, [r5 + 3 * mmsize]
    paddw           m3, m11
    pmaddubsw       m11, m9, [r5 + 2 * mmsize]
    paddw           m5, m11
    pmaddubsw       m11, m9, [r5 + 1 * mmsize]
    paddw           m7, m11
    pmaddubsw       m9, [r5]
    movu            xm11, [r0 + r4]                 ; m11 = row 11
    punpckhbw       xm12, xm10, xm11
    punpcklbw       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddubsw       m12, m10, [r5 + 3 * mmsize]
    paddw           m4, m12
    pmaddubsw       m12, m10, [r5 + 2 * mmsize]
    paddw           m6, m12
    pmaddubsw       m12, m10, [r5 + 1 * mmsize]
    paddw           m8, m12
    pmaddubsw       m10, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm12, [r0]                      ; m12 = row 12
    punpckhbw       xm13, xm11, xm12
    punpcklbw       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddubsw       m13, m11, [r5 + 3 * mmsize]
    paddw           m5, m13
    pmaddubsw       m13, m11, [r5 + 2 * mmsize]
    paddw           m7, m13
    pmaddubsw       m13, m11, [r5 + 1 * mmsize]
    paddw           m9, m13
    pmaddubsw       m11, [r5]

%ifidn %1,pp
    pmulhrsw        m0, m14                         ; m0 = word: row 0
    pmulhrsw        m1, m14                         ; m1 = word: row 1
    pmulhrsw        m2, m14                         ; m2 = word: row 2
    pmulhrsw        m3, m14                         ; m3 = word: row 3
    pmulhrsw        m4, m14                         ; m4 = word: row 4
    pmulhrsw        m5, m14                         ; m5 = word: row 5
    packuswb        m0, m1
    packuswb        m2, m3
    packuswb        m4, m5
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vpermq          m4, m4, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm5, m4, 1
    movq            [r2], xm0
    pextrd          [r2 + 8], xm0, 2
    movq            [r2 + r3], xm1
    pextrd          [r2 + r3 + 8], xm1, 2
    movq            [r2 + r3 * 2], xm2
    pextrd          [r2 + r3 * 2 + 8], xm2, 2
    movq            [r2 + r6], xm3
    pextrd          [r2 + r6 + 8], xm3, 2
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm4
    pextrd          [r2 + 8], xm4, 2
    movq            [r2 + r3], xm5
    pextrd          [r2 + r3 + 8], xm5, 2
%else
    psubw           m0, m14                         ; m0 = word: row 0
    psubw           m1, m14                         ; m1 = word: row 1
    psubw           m2, m14                         ; m2 = word: row 2
    psubw           m3, m14                         ; m3 = word: row 3
    psubw           m4, m14                         ; m4 = word: row 4
    psubw           m5, m14                         ; m5 = word: row 5
    movu            [r2], xm0
    vextracti128    xm0, m0, 1
    movq            [r2 + 16], xm0
    movu            [r2 + r3], xm1
    vextracti128    xm1, m1, 1
    movq            [r2 + r3 + 16], xm1
    movu            [r2 + r3 * 2], xm2
    vextracti128    xm2, m2, 1
    movq            [r2 + r3 * 2 + 16], xm2
    movu            [r2 + r6], xm3
    vextracti128    xm3, m3, 1
    movq            [r2 + r6 + 16], xm3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm4
    vextracti128    xm4, m4, 1
    movq            [r2 + 16], xm4
    movu            [r2 + r3], xm5
    vextracti128    xm5, m5, 1
    movq            [r2 + r3 + 16], xm5
%endif

    movu            xm13, [r0 + r1]                 ; m13 = row 13
    punpckhbw       xm0, xm12, xm13
    punpcklbw       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddubsw       m0, m12, [r5 + 3 * mmsize]
    paddw           m6, m0
    pmaddubsw       m0, m12, [r5 + 2 * mmsize]
    paddw           m8, m0
    pmaddubsw       m0, m12, [r5 + 1 * mmsize]
    paddw           m10, m0
    pmaddubsw       m12, [r5]
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhbw       xm1, xm13, xm0
    punpcklbw       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddubsw       m1, m13, [r5 + 3 * mmsize]
    paddw           m7, m1
    pmaddubsw       m1, m13, [r5 + 2 * mmsize]
    paddw           m9, m1
    pmaddubsw       m1, m13, [r5 + 1 * mmsize]
    paddw           m11, m1
    pmaddubsw       m13, [r5]

%ifidn %1,pp
    pmulhrsw        m6, m14                         ; m6 = word: row 6
    pmulhrsw        m7, m14                         ; m7 = word: row 7
    packuswb        m6, m7
    vpermq          m6, m6, 11011000b
    vextracti128    xm7, m6, 1
    movq            [r2 + r3 * 2], xm6
    pextrd          [r2 + r3 * 2 + 8], xm6, 2
    movq            [r2 + r6], xm7
    pextrd          [r2 + r6 + 8], xm7, 2
%else
    psubw           m6, m14                         ; m6 = word: row 6
    psubw           m7, m14                         ; m7 = word: row 7
    movu            [r2 + r3 * 2], xm6
    vextracti128    xm6, m6, 1
    movq            [r2 + r3 * 2 + 16], xm6
    movu            [r2 + r6], xm7
    vextracti128    xm7, m7, 1
    movq            [r2 + r6 + 16], xm7
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm1, [r0 + r4]                  ; m1 = row 15
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m2, m0, [r5 + 3 * mmsize]
    paddw           m8, m2
    pmaddubsw       m2, m0, [r5 + 2 * mmsize]
    paddw           m10, m2
    pmaddubsw       m2, m0, [r5 + 1 * mmsize]
    paddw           m12, m2
    pmaddubsw       m0, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm2, [r0]                       ; m2 = row 16
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m3, m1, [r5 + 3 * mmsize]
    paddw           m9, m3
    pmaddubsw       m3, m1, [r5 + 2 * mmsize]
    paddw           m11, m3
    pmaddubsw       m3, m1, [r5 + 1 * mmsize]
    paddw           m13, m3
    pmaddubsw       m1, [r5]
    movu            xm3, [r0 + r1]                  ; m3 = row 17
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 3 * mmsize]
    paddw           m10, m4
    pmaddubsw       m4, m2, [r5 + 2 * mmsize]
    paddw           m12, m4
    pmaddubsw       m2, [r5 + 1 * mmsize]
    paddw           m0, m2
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 3 * mmsize]
    paddw           m11, m5
    pmaddubsw       m5, m3, [r5 + 2 * mmsize]
    paddw           m13, m5
    pmaddubsw       m3, [r5 + 1 * mmsize]
    paddw           m1, m3
    movu            xm5, [r0 + r4]                  ; m5 = row 19
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 3 * mmsize]
    paddw           m12, m6
    pmaddubsw       m4, [r5 + 2 * mmsize]
    paddw           m0, m4
    lea             r0, [r0 + r1 * 4]
    movu            xm6, [r0]                       ; m6 = row 20
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, [r5 + 3 * mmsize]
    paddw           m13, m7
    pmaddubsw       m5, [r5 + 2 * mmsize]
    paddw           m1, m5
    movu            xm7, [r0 + r1]                  ; m7 = row 21
    punpckhbw       xm2, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm2, 1
    pmaddubsw       m6, [r5 + 3 * mmsize]
    paddw           m0, m6
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 22
    punpckhbw       xm3, xm7, xm2
    punpcklbw       xm7, xm2
    vinserti128     m7, m7, xm3, 1
    pmaddubsw       m7, [r5 + 3 * mmsize]
    paddw           m1, m7

%ifidn %1,pp
    pmulhrsw        m8, m14                         ; m8 = word: row 8
    pmulhrsw        m9, m14                         ; m9 = word: row 9
    pmulhrsw        m10, m14                        ; m10 = word: row 10
    pmulhrsw        m11, m14                        ; m11 = word: row 11
    pmulhrsw        m12, m14                        ; m12 = word: row 12
    pmulhrsw        m13, m14                        ; m13 = word: row 13
    pmulhrsw        m0, m14                         ; m0 = word: row 14
    pmulhrsw        m1, m14                         ; m1 = word: row 15
    packuswb        m8, m9
    packuswb        m10, m11
    packuswb        m12, m13
    packuswb        m0, m1
    vpermq          m8, m8, 11011000b
    vpermq          m10, m10, 11011000b
    vpermq          m12, m12, 11011000b
    vpermq          m0, m0, 11011000b
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    vextracti128    xm13, m12, 1
    vextracti128    xm1, m0, 1
    movq            [r2], xm8
    pextrd          [r2 + 8], xm8, 2
    movq            [r2 + r3], xm9
    pextrd          [r2 + r3 + 8], xm9, 2
    movq            [r2 + r3 * 2], xm10
    pextrd          [r2 + r3 * 2 + 8], xm10, 2
    movq            [r2 + r6], xm11
    pextrd          [r2 + r6 + 8], xm11, 2
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm12
    pextrd          [r2 + 8], xm12, 2
    movq            [r2 + r3], xm13
    pextrd          [r2 + r3 + 8], xm13, 2
    movq            [r2 + r3 * 2], xm0
    pextrd          [r2 + r3 * 2 + 8], xm0, 2
    movq            [r2 + r6], xm1
    pextrd          [r2 + r6 + 8], xm1, 2
%else
    psubw           m8, m14                         ; m8 = word: row 8
    psubw           m9, m14                         ; m9 = word: row 9
    psubw           m10, m14                        ; m10 = word: row 10
    psubw           m11, m14                        ; m11 = word: row 11
    psubw           m12, m14                        ; m12 = word: row 12
    psubw           m13, m14                        ; m13 = word: row 13
    psubw           m0, m14                         ; m0 = word: row 14
    psubw           m1, m14                         ; m1 = word: row 15
    movu            [r2], xm8
    vextracti128    xm8, m8, 1
    movq            [r2 + 16], xm8
    movu            [r2 + r3], xm9
    vextracti128    xm9, m9, 1
    movq            [r2 + r3 + 16], xm9
    movu            [r2 + r3 * 2], xm10
    vextracti128    xm10, m10, 1
    movq            [r2 + r3 * 2 + 16], xm10
    movu            [r2 + r6], xm11
    vextracti128    xm11, m11, 1
    movq            [r2 + r6 + 16], xm11
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm12
    vextracti128    xm12, m12, 1
    movq            [r2 + 16], xm12
    movu            [r2 + r3], xm13
    vextracti128    xm13, m13, 1
    movq            [r2 + r3 + 16], xm13
    movu            [r2 + r3 * 2], xm0
    vextracti128    xm0, m0, 1
    movq            [r2 + r3 * 2 + 16], xm0
    movu            [r2 + r6], xm1
    vextracti128    xm1, m1, 1
    movq            [r2 + r6 + 16], xm1
%endif
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_12x16 pp
    FILTER_VER_LUMA_AVX2_12x16 ps

%macro FILTER_VER_LUMA_AVX2_16x16 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_16x16, 4, 7, 15
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    mova            m14, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m14, [pw_2000]
%endif
    lea             r6, [r3 * 3]

    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 1 * mmsize]
    paddw           m0, m4
    pmaddubsw       m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 1 * mmsize]
    paddw           m1, m5
    pmaddubsw       m3, [r5]
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 2 * mmsize]
    paddw           m0, m6
    pmaddubsw       m6, m4, [r5 + 1 * mmsize]
    paddw           m2, m6
    pmaddubsw       m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, [r5 + 2 * mmsize]
    paddw           m1, m7
    pmaddubsw       m7, m5, [r5 + 1 * mmsize]
    paddw           m3, m7
    pmaddubsw       m5, [r5]
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhbw       xm8, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddubsw       m8, m6, [r5 + 3 * mmsize]
    paddw           m0, m8
    pmaddubsw       m8, m6, [r5 + 2 * mmsize]
    paddw           m2, m8
    pmaddubsw       m8, m6, [r5 + 1 * mmsize]
    paddw           m4, m8
    pmaddubsw       m6, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhbw       xm9, xm7, xm8
    punpcklbw       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddubsw       m9, m7, [r5 + 3 * mmsize]
    paddw           m1, m9
    pmaddubsw       m9, m7, [r5 + 2 * mmsize]
    paddw           m3, m9
    pmaddubsw       m9, m7, [r5 + 1 * mmsize]
    paddw           m5, m9
    pmaddubsw       m7, [r5]
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhbw       xm10, xm8, xm9
    punpcklbw       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddubsw       m10, m8, [r5 + 3 * mmsize]
    paddw           m2, m10
    pmaddubsw       m10, m8, [r5 + 2 * mmsize]
    paddw           m4, m10
    pmaddubsw       m10, m8, [r5 + 1 * mmsize]
    paddw           m6, m10
    pmaddubsw       m8, [r5]
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhbw       xm11, xm9, xm10
    punpcklbw       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddubsw       m11, m9, [r5 + 3 * mmsize]
    paddw           m3, m11
    pmaddubsw       m11, m9, [r5 + 2 * mmsize]
    paddw           m5, m11
    pmaddubsw       m11, m9, [r5 + 1 * mmsize]
    paddw           m7, m11
    pmaddubsw       m9, [r5]
    movu            xm11, [r0 + r4]                 ; m11 = row 11
    punpckhbw       xm12, xm10, xm11
    punpcklbw       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddubsw       m12, m10, [r5 + 3 * mmsize]
    paddw           m4, m12
    pmaddubsw       m12, m10, [r5 + 2 * mmsize]
    paddw           m6, m12
    pmaddubsw       m12, m10, [r5 + 1 * mmsize]
    paddw           m8, m12
    pmaddubsw       m10, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm12, [r0]                      ; m12 = row 12
    punpckhbw       xm13, xm11, xm12
    punpcklbw       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddubsw       m13, m11, [r5 + 3 * mmsize]
    paddw           m5, m13
    pmaddubsw       m13, m11, [r5 + 2 * mmsize]
    paddw           m7, m13
    pmaddubsw       m13, m11, [r5 + 1 * mmsize]
    paddw           m9, m13
    pmaddubsw       m11, [r5]

%ifidn %1,pp
    pmulhrsw        m0, m14                         ; m0 = word: row 0
    pmulhrsw        m1, m14                         ; m1 = word: row 1
    pmulhrsw        m2, m14                         ; m2 = word: row 2
    pmulhrsw        m3, m14                         ; m3 = word: row 3
    pmulhrsw        m4, m14                         ; m4 = word: row 4
    pmulhrsw        m5, m14                         ; m5 = word: row 5
    packuswb        m0, m1
    packuswb        m2, m3
    packuswb        m4, m5
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vpermq          m4, m4, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm5, m4, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm4
    movu            [r2 + r3], xm5
%else
    psubw           m0, m14                         ; m0 = word: row 0
    psubw           m1, m14                         ; m1 = word: row 1
    psubw           m2, m14                         ; m2 = word: row 2
    psubw           m3, m14                         ; m3 = word: row 3
    psubw           m4, m14                         ; m4 = word: row 4
    psubw           m5, m14                         ; m5 = word: row 5
    movu            [r2], m0
    movu            [r2 + r3], m1
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r6], m3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], m4
    movu            [r2 + r3], m5
%endif

    movu            xm13, [r0 + r1]                 ; m13 = row 13
    punpckhbw       xm0, xm12, xm13
    punpcklbw       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddubsw       m0, m12, [r5 + 3 * mmsize]
    paddw           m6, m0
    pmaddubsw       m0, m12, [r5 + 2 * mmsize]
    paddw           m8, m0
    pmaddubsw       m0, m12, [r5 + 1 * mmsize]
    paddw           m10, m0
    pmaddubsw       m12, [r5]
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhbw       xm1, xm13, xm0
    punpcklbw       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddubsw       m1, m13, [r5 + 3 * mmsize]
    paddw           m7, m1
    pmaddubsw       m1, m13, [r5 + 2 * mmsize]
    paddw           m9, m1
    pmaddubsw       m1, m13, [r5 + 1 * mmsize]
    paddw           m11, m1
    pmaddubsw       m13, [r5]

%ifidn %1,pp
    pmulhrsw        m6, m14                         ; m6 = word: row 6
    pmulhrsw        m7, m14                         ; m7 = word: row 7
    packuswb        m6, m7
    vpermq          m6, m6, 11011000b
    vextracti128    xm7, m6, 1
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r6], xm7
%else
    psubw           m6, m14                         ; m6 = word: row 6
    psubw           m7, m14                         ; m7 = word: row 7
    movu            [r2 + r3 * 2], m6
    movu            [r2 + r6], m7
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm1, [r0 + r4]                  ; m1 = row 15
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m2, m0, [r5 + 3 * mmsize]
    paddw           m8, m2
    pmaddubsw       m2, m0, [r5 + 2 * mmsize]
    paddw           m10, m2
    pmaddubsw       m2, m0, [r5 + 1 * mmsize]
    paddw           m12, m2
    pmaddubsw       m0, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm2, [r0]                       ; m2 = row 16
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m3, m1, [r5 + 3 * mmsize]
    paddw           m9, m3
    pmaddubsw       m3, m1, [r5 + 2 * mmsize]
    paddw           m11, m3
    pmaddubsw       m3, m1, [r5 + 1 * mmsize]
    paddw           m13, m3
    pmaddubsw       m1, [r5]
    movu            xm3, [r0 + r1]                  ; m3 = row 17
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 3 * mmsize]
    paddw           m10, m4
    pmaddubsw       m4, m2, [r5 + 2 * mmsize]
    paddw           m12, m4
    pmaddubsw       m2, [r5 + 1 * mmsize]
    paddw           m0, m2
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 3 * mmsize]
    paddw           m11, m5
    pmaddubsw       m5, m3, [r5 + 2 * mmsize]
    paddw           m13, m5
    pmaddubsw       m3, [r5 + 1 * mmsize]
    paddw           m1, m3
    movu            xm5, [r0 + r4]                  ; m5 = row 19
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 3 * mmsize]
    paddw           m12, m6
    pmaddubsw       m4, [r5 + 2 * mmsize]
    paddw           m0, m4
    lea             r0, [r0 + r1 * 4]
    movu            xm6, [r0]                       ; m6 = row 20
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, [r5 + 3 * mmsize]
    paddw           m13, m7
    pmaddubsw       m5, [r5 + 2 * mmsize]
    paddw           m1, m5
    movu            xm7, [r0 + r1]                  ; m7 = row 21
    punpckhbw       xm2, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm2, 1
    pmaddubsw       m6, [r5 + 3 * mmsize]
    paddw           m0, m6
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 22
    punpckhbw       xm3, xm7, xm2
    punpcklbw       xm7, xm2
    vinserti128     m7, m7, xm3, 1
    pmaddubsw       m7, [r5 + 3 * mmsize]
    paddw           m1, m7

%ifidn %1,pp
    pmulhrsw        m8, m14                         ; m8 = word: row 8
    pmulhrsw        m9, m14                         ; m9 = word: row 9
    pmulhrsw        m10, m14                        ; m10 = word: row 10
    pmulhrsw        m11, m14                        ; m11 = word: row 11
    pmulhrsw        m12, m14                        ; m12 = word: row 12
    pmulhrsw        m13, m14                        ; m13 = word: row 13
    pmulhrsw        m0, m14                         ; m0 = word: row 14
    pmulhrsw        m1, m14                         ; m1 = word: row 15
    packuswb        m8, m9
    packuswb        m10, m11
    packuswb        m12, m13
    packuswb        m0, m1
    vpermq          m8, m8, 11011000b
    vpermq          m10, m10, 11011000b
    vpermq          m12, m12, 11011000b
    vpermq          m0, m0, 11011000b
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    vextracti128    xm13, m12, 1
    vextracti128    xm1, m0, 1
    movu            [r2], xm8
    movu            [r2 + r3], xm9
    movu            [r2 + r3 * 2], xm10
    movu            [r2 + r6], xm11
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm12
    movu            [r2 + r3], xm13
    movu            [r2 + r3 * 2], xm0
    movu            [r2 + r6], xm1
%else
    psubw           m8, m14                         ; m8 = word: row 8
    psubw           m9, m14                         ; m9 = word: row 9
    psubw           m10, m14                        ; m10 = word: row 10
    psubw           m11, m14                        ; m11 = word: row 11
    psubw           m12, m14                        ; m12 = word: row 12
    psubw           m13, m14                        ; m13 = word: row 13
    psubw           m0, m14                         ; m0 = word: row 14
    psubw           m1, m14                         ; m1 = word: row 15
    movu            [r2], m8
    movu            [r2 + r3], m9
    movu            [r2 + r3 * 2], m10
    movu            [r2 + r6], m11
    lea             r2, [r2 + r3 * 4]
    movu            [r2], m12
    movu            [r2 + r3], m13
    movu            [r2 + r3 * 2], m0
    movu            [r2 + r6], m1
%endif
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_16x16 pp
    FILTER_VER_LUMA_AVX2_16x16 ps

%macro FILTER_VER_LUMA_AVX2_16x12 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_16x12, 4, 7, 15
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    mova            m14, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m14, [pw_2000]
%endif
    lea             r6, [r3 * 3]

    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 1 * mmsize]
    paddw           m0, m4
    pmaddubsw       m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 1 * mmsize]
    paddw           m1, m5
    pmaddubsw       m3, [r5]
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 2 * mmsize]
    paddw           m0, m6
    pmaddubsw       m6, m4, [r5 + 1 * mmsize]
    paddw           m2, m6
    pmaddubsw       m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, [r5 + 2 * mmsize]
    paddw           m1, m7
    pmaddubsw       m7, m5, [r5 + 1 * mmsize]
    paddw           m3, m7
    pmaddubsw       m5, [r5]
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhbw       xm8, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddubsw       m8, m6, [r5 + 3 * mmsize]
    paddw           m0, m8
    pmaddubsw       m8, m6, [r5 + 2 * mmsize]
    paddw           m2, m8
    pmaddubsw       m8, m6, [r5 + 1 * mmsize]
    paddw           m4, m8
    pmaddubsw       m6, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhbw       xm9, xm7, xm8
    punpcklbw       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddubsw       m9, m7, [r5 + 3 * mmsize]
    paddw           m1, m9
    pmaddubsw       m9, m7, [r5 + 2 * mmsize]
    paddw           m3, m9
    pmaddubsw       m9, m7, [r5 + 1 * mmsize]
    paddw           m5, m9
    pmaddubsw       m7, [r5]
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhbw       xm10, xm8, xm9
    punpcklbw       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddubsw       m10, m8, [r5 + 3 * mmsize]
    paddw           m2, m10
    pmaddubsw       m10, m8, [r5 + 2 * mmsize]
    paddw           m4, m10
    pmaddubsw       m10, m8, [r5 + 1 * mmsize]
    paddw           m6, m10
    pmaddubsw       m8, [r5]
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhbw       xm11, xm9, xm10
    punpcklbw       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddubsw       m11, m9, [r5 + 3 * mmsize]
    paddw           m3, m11
    pmaddubsw       m11, m9, [r5 + 2 * mmsize]
    paddw           m5, m11
    pmaddubsw       m11, m9, [r5 + 1 * mmsize]
    paddw           m7, m11
    pmaddubsw       m9, [r5]
    movu            xm11, [r0 + r4]                 ; m11 = row 11
    punpckhbw       xm12, xm10, xm11
    punpcklbw       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddubsw       m12, m10, [r5 + 3 * mmsize]
    paddw           m4, m12
    pmaddubsw       m12, m10, [r5 + 2 * mmsize]
    paddw           m6, m12
    pmaddubsw       m12, m10, [r5 + 1 * mmsize]
    paddw           m8, m12
    pmaddubsw       m10, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm12, [r0]                      ; m12 = row 12
    punpckhbw       xm13, xm11, xm12
    punpcklbw       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddubsw       m13, m11, [r5 + 3 * mmsize]
    paddw           m5, m13
    pmaddubsw       m13, m11, [r5 + 2 * mmsize]
    paddw           m7, m13
    pmaddubsw       m13, m11, [r5 + 1 * mmsize]
    paddw           m9, m13
    pmaddubsw       m11, [r5]

%ifidn %1,pp
    pmulhrsw        m0, m14                         ; m0 = word: row 0
    pmulhrsw        m1, m14                         ; m1 = word: row 1
    pmulhrsw        m2, m14                         ; m2 = word: row 2
    pmulhrsw        m3, m14                         ; m3 = word: row 3
    pmulhrsw        m4, m14                         ; m4 = word: row 4
    pmulhrsw        m5, m14                         ; m5 = word: row 5
    packuswb        m0, m1
    packuswb        m2, m3
    packuswb        m4, m5
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vpermq          m4, m4, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm5, m4, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm4
    movu            [r2 + r3], xm5
%else
    psubw           m0, m14                         ; m0 = word: row 0
    psubw           m1, m14                         ; m1 = word: row 1
    psubw           m2, m14                         ; m2 = word: row 2
    psubw           m3, m14                         ; m3 = word: row 3
    psubw           m4, m14                         ; m4 = word: row 4
    psubw           m5, m14                         ; m5 = word: row 5
    movu            [r2], m0
    movu            [r2 + r3], m1
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r6], m3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], m4
    movu            [r2 + r3], m5
%endif

    movu            xm13, [r0 + r1]                 ; m13 = row 13
    punpckhbw       xm0, xm12, xm13
    punpcklbw       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddubsw       m0, m12, [r5 + 3 * mmsize]
    paddw           m6, m0
    pmaddubsw       m0, m12, [r5 + 2 * mmsize]
    paddw           m8, m0
    pmaddubsw       m0, m12, [r5 + 1 * mmsize]
    paddw           m10, m0
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhbw       xm1, xm13, xm0
    punpcklbw       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddubsw       m1, m13, [r5 + 3 * mmsize]
    paddw           m7, m1
    pmaddubsw       m1, m13, [r5 + 2 * mmsize]
    paddw           m9, m1
    pmaddubsw       m1, m13, [r5 + 1 * mmsize]
    paddw           m11, m1

%ifidn %1,pp
    pmulhrsw        m6, m14                         ; m6 = word: row 6
    pmulhrsw        m7, m14                         ; m7 = word: row 7
    packuswb        m6, m7
    vpermq          m6, m6, 11011000b
    vextracti128    xm7, m6, 1
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r6], xm7
%else
    psubw           m6, m14                         ; m6 = word: row 6
    psubw           m7, m14                         ; m7 = word: row 7
    movu            [r2 + r3 * 2], m6
    movu            [r2 + r6], m7
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm1, [r0 + r4]                  ; m1 = row 15
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m2, m0, [r5 + 3 * mmsize]
    paddw           m8, m2
    pmaddubsw       m2, m0, [r5 + 2 * mmsize]
    paddw           m10, m2
    lea             r0, [r0 + r1 * 4]
    movu            xm2, [r0]                       ; m2 = row 16
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m3, m1, [r5 + 3 * mmsize]
    paddw           m9, m3
    pmaddubsw       m3, m1, [r5 + 2 * mmsize]
    paddw           m11, m3
    movu            xm3, [r0 + r1]                  ; m3 = row 17
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 3 * mmsize]
    paddw           m10, m4
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 3 * mmsize]
    paddw           m11, m5

%ifidn %1,pp
    pmulhrsw        m8, m14                         ; m8 = word: row 8
    pmulhrsw        m9, m14                         ; m9 = word: row 9
    pmulhrsw        m10, m14                        ; m10 = word: row 10
    pmulhrsw        m11, m14                        ; m11 = word: row 11
    packuswb        m8, m9
    packuswb        m10, m11
    vpermq          m8, m8, 11011000b
    vpermq          m10, m10, 11011000b
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    movu            [r2], xm8
    movu            [r2 + r3], xm9
    movu            [r2 + r3 * 2], xm10
    movu            [r2 + r6], xm11
%else
    psubw           m8, m14                         ; m8 = word: row 8
    psubw           m9, m14                         ; m9 = word: row 9
    psubw           m10, m14                        ; m10 = word: row 10
    psubw           m11, m14                        ; m11 = word: row 11
    movu            [r2], m8
    movu            [r2 + r3], m9
    movu            [r2 + r3 * 2], m10
    movu            [r2 + r6], m11
%endif
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_16x12 pp
    FILTER_VER_LUMA_AVX2_16x12 ps

%macro FILTER_VER_LUMA_AVX2_16x8 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_16x8, 4, 6, 15
    mov             r4d, r4m
    shl             r4d, 7
%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif
    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    mova            m14, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m14, [pw_2000]
%endif
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 1 * mmsize]
    paddw           m0, m4
    pmaddubsw       m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 1 * mmsize]
    paddw           m1, m5
    pmaddubsw       m3, [r5]
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 2 * mmsize]
    paddw           m0, m6
    pmaddubsw       m6, m4, [r5 + 1 * mmsize]
    paddw           m2, m6
    pmaddubsw       m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, [r5 + 2 * mmsize]
    paddw           m1, m7
    pmaddubsw       m7, m5, [r5 + 1 * mmsize]
    paddw           m3, m7
    pmaddubsw       m5, [r5]
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhbw       xm8, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddubsw       m8, m6, [r5 + 3 * mmsize]
    paddw           m0, m8
    pmaddubsw       m8, m6, [r5 + 2 * mmsize]
    paddw           m2, m8
    pmaddubsw       m8, m6, [r5 + 1 * mmsize]
    paddw           m4, m8
    pmaddubsw       m6, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhbw       xm9, xm7, xm8
    punpcklbw       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddubsw       m9, m7, [r5 + 3 * mmsize]
    paddw           m1, m9
    pmaddubsw       m9, m7, [r5 + 2 * mmsize]
    paddw           m3, m9
    pmaddubsw       m9, m7, [r5 + 1 * mmsize]
    paddw           m5, m9
    pmaddubsw       m7, [r5]
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhbw       xm10, xm8, xm9
    punpcklbw       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddubsw       m10, m8, [r5 + 3 * mmsize]
    paddw           m2, m10
    pmaddubsw       m10, m8, [r5 + 2 * mmsize]
    paddw           m4, m10
    pmaddubsw       m10, m8, [r5 + 1 * mmsize]
    paddw           m6, m10
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhbw       xm11, xm9, xm10
    punpcklbw       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddubsw       m11, m9, [r5 + 3 * mmsize]
    paddw           m3, m11
    pmaddubsw       m11, m9, [r5 + 2 * mmsize]
    paddw           m5, m11
    pmaddubsw       m11, m9, [r5 + 1 * mmsize]
    paddw           m7, m11
    movu            xm11, [r0 + r4]                 ; m11 = row 11
    punpckhbw       xm12, xm10, xm11
    punpcklbw       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddubsw       m12, m10, [r5 + 3 * mmsize]
    paddw           m4, m12
    pmaddubsw       m12, m10, [r5 + 2 * mmsize]
    paddw           m6, m12
    lea             r0, [r0 + r1 * 4]
    movu            xm12, [r0]                      ; m12 = row 12
    punpckhbw       xm13, xm11, xm12
    punpcklbw       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddubsw       m13, m11, [r5 + 3 * mmsize]
    paddw           m5, m13
    pmaddubsw       m13, m11, [r5 + 2 * mmsize]
    paddw           m7, m13
    lea             r4, [r3 * 3]
%ifidn %1,pp
    pmulhrsw        m0, m14                         ; m0 = word: row 0
    pmulhrsw        m1, m14                         ; m1 = word: row 1
    pmulhrsw        m2, m14                         ; m2 = word: row 2
    pmulhrsw        m3, m14                         ; m3 = word: row 3
    pmulhrsw        m4, m14                         ; m4 = word: row 4
    pmulhrsw        m5, m14                         ; m5 = word: row 5
    packuswb        m0, m1
    packuswb        m2, m3
    packuswb        m4, m5
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vpermq          m4, m4, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm5, m4, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r4], xm3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm4
    movu            [r2 + r3], xm5
%else
    psubw           m0, m14                         ; m0 = word: row 0
    psubw           m1, m14                         ; m1 = word: row 1
    psubw           m2, m14                         ; m2 = word: row 2
    psubw           m3, m14                         ; m3 = word: row 3
    psubw           m4, m14                         ; m4 = word: row 4
    psubw           m5, m14                         ; m5 = word: row 5
    movu            [r2], m0
    movu            [r2 + r3], m1
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r4], m3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], m4
    movu            [r2 + r3], m5
%endif
    movu            xm13, [r0 + r1]                 ; m13 = row 13
    punpckhbw       xm0, xm12, xm13
    punpcklbw       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddubsw       m0, m12, [r5 + 3 * mmsize]
    paddw           m6, m0
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhbw       xm1, xm13, xm0
    punpcklbw       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddubsw       m1, m13, [r5 + 3 * mmsize]
    paddw           m7, m1
%ifidn %1,pp
    pmulhrsw        m6, m14                         ; m6 = word: row 6
    pmulhrsw        m7, m14                         ; m7 = word: row 7
    packuswb        m6, m7
    vpermq          m6, m6, 11011000b
    vextracti128    xm7, m6, 1
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r4], xm7
%else
    psubw           m6, m14                         ; m6 = word: row 6
    psubw           m7, m14                         ; m7 = word: row 7
    movu            [r2 + r3 * 2], m6
    movu            [r2 + r4], m7
%endif
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_16x8 pp
    FILTER_VER_LUMA_AVX2_16x8 ps

%macro FILTER_VER_LUMA_AVX2_16x4 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_16x4, 4, 6, 13
    mov             r4d, r4m
    shl             r4d, 7
%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif
    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    mova            m12, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m12, [pw_2000]
%endif
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 1 * mmsize]
    paddw           m0, m4
    pmaddubsw       m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 1 * mmsize]
    paddw           m1, m5
    pmaddubsw       m3, [r5]
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 2 * mmsize]
    paddw           m0, m6
    pmaddubsw       m6, m4, [r5 + 1 * mmsize]
    paddw           m2, m6
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, [r5 + 2 * mmsize]
    paddw           m1, m7
    pmaddubsw       m7, m5, [r5 + 1 * mmsize]
    paddw           m3, m7
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhbw       xm8, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddubsw       m8, m6, [r5 + 3 * mmsize]
    paddw           m0, m8
    pmaddubsw       m8, m6, [r5 + 2 * mmsize]
    paddw           m2, m8
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhbw       xm9, xm7, xm8
    punpcklbw       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddubsw       m9, m7, [r5 + 3 * mmsize]
    paddw           m1, m9
    pmaddubsw       m9, m7, [r5 + 2 * mmsize]
    paddw           m3, m9
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhbw       xm10, xm8, xm9
    punpcklbw       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddubsw       m10, m8, [r5 + 3 * mmsize]
    paddw           m2, m10
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhbw       xm11, xm9, xm10
    punpcklbw       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddubsw       m11, m9, [r5 + 3 * mmsize]
    paddw           m3, m11
%ifidn %1,pp
    pmulhrsw        m0, m12                         ; m0 = word: row 0
    pmulhrsw        m1, m12                         ; m1 = word: row 1
    pmulhrsw        m2, m12                         ; m2 = word: row 2
    pmulhrsw        m3, m12                         ; m3 = word: row 3
    packuswb        m0, m1
    packuswb        m2, m3
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    lea             r4, [r3 * 3]
    movu            [r2 + r4], xm3
%else
    psubw           m0, m12                         ; m0 = word: row 0
    psubw           m1, m12                         ; m1 = word: row 1
    psubw           m2, m12                         ; m2 = word: row 2
    psubw           m3, m12                         ; m3 = word: row 3
    movu            [r2], m0
    movu            [r2 + r3], m1
    movu            [r2 + r3 * 2], m2
    lea             r4, [r3 * 3]
    movu            [r2 + r4], m3
%endif
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_16x4 pp
    FILTER_VER_LUMA_AVX2_16x4 ps
%macro FILTER_VER_LUMA_AVX2_16xN 3
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%3_%1x%2, 4, 9, 15
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %3,ps
    add             r3d, r3d
    vbroadcasti128  m14, [pw_2000]
%else
    mova            m14, [pw_512]
%endif
    lea             r6, [r3 * 3]
    lea             r7, [r1 * 4]
    mov             r8d, %2 / 16

.loop:
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 1 * mmsize]
    paddw           m0, m4
    pmaddubsw       m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 1 * mmsize]
    paddw           m1, m5
    pmaddubsw       m3, [r5]
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 2 * mmsize]
    paddw           m0, m6
    pmaddubsw       m6, m4, [r5 + 1 * mmsize]
    paddw           m2, m6
    pmaddubsw       m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, [r5 + 2 * mmsize]
    paddw           m1, m7
    pmaddubsw       m7, m5, [r5 + 1 * mmsize]
    paddw           m3, m7
    pmaddubsw       m5, [r5]
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhbw       xm8, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddubsw       m8, m6, [r5 + 3 * mmsize]
    paddw           m0, m8
    pmaddubsw       m8, m6, [r5 + 2 * mmsize]
    paddw           m2, m8
    pmaddubsw       m8, m6, [r5 + 1 * mmsize]
    paddw           m4, m8
    pmaddubsw       m6, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhbw       xm9, xm7, xm8
    punpcklbw       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddubsw       m9, m7, [r5 + 3 * mmsize]
    paddw           m1, m9
    pmaddubsw       m9, m7, [r5 + 2 * mmsize]
    paddw           m3, m9
    pmaddubsw       m9, m7, [r5 + 1 * mmsize]
    paddw           m5, m9
    pmaddubsw       m7, [r5]
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhbw       xm10, xm8, xm9
    punpcklbw       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddubsw       m10, m8, [r5 + 3 * mmsize]
    paddw           m2, m10
    pmaddubsw       m10, m8, [r5 + 2 * mmsize]
    paddw           m4, m10
    pmaddubsw       m10, m8, [r5 + 1 * mmsize]
    paddw           m6, m10
    pmaddubsw       m8, [r5]
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhbw       xm11, xm9, xm10
    punpcklbw       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddubsw       m11, m9, [r5 + 3 * mmsize]
    paddw           m3, m11
    pmaddubsw       m11, m9, [r5 + 2 * mmsize]
    paddw           m5, m11
    pmaddubsw       m11, m9, [r5 + 1 * mmsize]
    paddw           m7, m11
    pmaddubsw       m9, [r5]
    movu            xm11, [r0 + r4]                 ; m11 = row 11
    punpckhbw       xm12, xm10, xm11
    punpcklbw       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddubsw       m12, m10, [r5 + 3 * mmsize]
    paddw           m4, m12
    pmaddubsw       m12, m10, [r5 + 2 * mmsize]
    paddw           m6, m12
    pmaddubsw       m12, m10, [r5 + 1 * mmsize]
    paddw           m8, m12
    pmaddubsw       m10, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm12, [r0]                      ; m12 = row 12
    punpckhbw       xm13, xm11, xm12
    punpcklbw       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddubsw       m13, m11, [r5 + 3 * mmsize]
    paddw           m5, m13
    pmaddubsw       m13, m11, [r5 + 2 * mmsize]
    paddw           m7, m13
    pmaddubsw       m13, m11, [r5 + 1 * mmsize]
    paddw           m9, m13
    pmaddubsw       m11, [r5]

%ifidn %3,pp
    pmulhrsw        m0, m14                         ; m0 = word: row 0
    pmulhrsw        m1, m14                         ; m1 = word: row 1
    pmulhrsw        m2, m14                         ; m2 = word: row 2
    pmulhrsw        m3, m14                         ; m3 = word: row 3
    pmulhrsw        m4, m14                         ; m4 = word: row 4
    pmulhrsw        m5, m14                         ; m5 = word: row 5
    packuswb        m0, m1
    packuswb        m2, m3
    packuswb        m4, m5
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vpermq          m4, m4, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm5, m4, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm4
    movu            [r2 + r3], xm5
%else
    psubw           m0, m14                         ; m0 = word: row 0
    psubw           m1, m14                         ; m1 = word: row 1
    psubw           m2, m14                         ; m2 = word: row 2
    psubw           m3, m14                         ; m3 = word: row 3
    psubw           m4, m14                         ; m4 = word: row 4
    psubw           m5, m14                         ; m5 = word: row 5
    movu            [r2], m0
    movu            [r2 + r3], m1
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r6], m3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], m4
    movu            [r2 + r3], m5
%endif

    movu            xm13, [r0 + r1]                 ; m13 = row 13
    punpckhbw       xm0, xm12, xm13
    punpcklbw       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddubsw       m0, m12, [r5 + 3 * mmsize]
    paddw           m6, m0
    pmaddubsw       m0, m12, [r5 + 2 * mmsize]
    paddw           m8, m0
    pmaddubsw       m0, m12, [r5 + 1 * mmsize]
    paddw           m10, m0
    pmaddubsw       m12, [r5]
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhbw       xm1, xm13, xm0
    punpcklbw       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddubsw       m1, m13, [r5 + 3 * mmsize]
    paddw           m7, m1
    pmaddubsw       m1, m13, [r5 + 2 * mmsize]
    paddw           m9, m1
    pmaddubsw       m1, m13, [r5 + 1 * mmsize]
    paddw           m11, m1
    pmaddubsw       m13, [r5]

%ifidn %3,pp
    pmulhrsw        m6, m14                         ; m6 = word: row 6
    pmulhrsw        m7, m14                         ; m7 = word: row 7
    packuswb        m6, m7
    vpermq          m6, m6, 11011000b
    vextracti128    xm7, m6, 1
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r6], xm7
%else
    psubw           m6, m14                         ; m6 = word: row 6
    psubw           m7, m14                         ; m7 = word: row 7
    movu            [r2 + r3 * 2], m6
    movu            [r2 + r6], m7
%endif

    lea             r2, [r2 + r3 * 4]

    movu            xm1, [r0 + r4]                  ; m1 = row 15
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m2, m0, [r5 + 3 * mmsize]
    paddw           m8, m2
    pmaddubsw       m2, m0, [r5 + 2 * mmsize]
    paddw           m10, m2
    pmaddubsw       m2, m0, [r5 + 1 * mmsize]
    paddw           m12, m2
    pmaddubsw       m0, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm2, [r0]                       ; m2 = row 16
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m3, m1, [r5 + 3 * mmsize]
    paddw           m9, m3
    pmaddubsw       m3, m1, [r5 + 2 * mmsize]
    paddw           m11, m3
    pmaddubsw       m3, m1, [r5 + 1 * mmsize]
    paddw           m13, m3
    pmaddubsw       m1, [r5]
    movu            xm3, [r0 + r1]                  ; m3 = row 17
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 3 * mmsize]
    paddw           m10, m4
    pmaddubsw       m4, m2, [r5 + 2 * mmsize]
    paddw           m12, m4
    pmaddubsw       m2, [r5 + 1 * mmsize]
    paddw           m0, m2
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 3 * mmsize]
    paddw           m11, m5
    pmaddubsw       m5, m3, [r5 + 2 * mmsize]
    paddw           m13, m5
    pmaddubsw       m3, [r5 + 1 * mmsize]
    paddw           m1, m3
    movu            xm5, [r0 + r4]                  ; m5 = row 19
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 3 * mmsize]
    paddw           m12, m6
    pmaddubsw       m4, [r5 + 2 * mmsize]
    paddw           m0, m4
    lea             r0, [r0 + r1 * 4]
    movu            xm6, [r0]                       ; m6 = row 20
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, [r5 + 3 * mmsize]
    paddw           m13, m7
    pmaddubsw       m5, [r5 + 2 * mmsize]
    paddw           m1, m5
    movu            xm7, [r0 + r1]                  ; m7 = row 21
    punpckhbw       xm2, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm2, 1
    pmaddubsw       m6, [r5 + 3 * mmsize]
    paddw           m0, m6
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 22
    punpckhbw       xm3, xm7, xm2
    punpcklbw       xm7, xm2
    vinserti128     m7, m7, xm3, 1
    pmaddubsw       m7, [r5 + 3 * mmsize]
    paddw           m1, m7

%ifidn %3,pp
    pmulhrsw        m8, m14                         ; m8 = word: row 8
    pmulhrsw        m9, m14                         ; m9 = word: row 9
    pmulhrsw        m10, m14                        ; m10 = word: row 10
    pmulhrsw        m11, m14                        ; m11 = word: row 11
    pmulhrsw        m12, m14                        ; m12 = word: row 12
    pmulhrsw        m13, m14                        ; m13 = word: row 13
    pmulhrsw        m0, m14                         ; m0 = word: row 14
    pmulhrsw        m1, m14                         ; m1 = word: row 15
    packuswb        m8, m9
    packuswb        m10, m11
    packuswb        m12, m13
    packuswb        m0, m1
    vpermq          m8, m8, 11011000b
    vpermq          m10, m10, 11011000b
    vpermq          m12, m12, 11011000b
    vpermq          m0, m0, 11011000b
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    vextracti128    xm13, m12, 1
    vextracti128    xm1, m0, 1
    movu            [r2], xm8
    movu            [r2 + r3], xm9
    movu            [r2 + r3 * 2], xm10
    movu            [r2 + r6], xm11
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm12
    movu            [r2 + r3], xm13
    movu            [r2 + r3 * 2], xm0
    movu            [r2 + r6], xm1
%else
    psubw           m8, m14                         ; m8 = word: row 8
    psubw           m9, m14                         ; m9 = word: row 9
    psubw           m10, m14                        ; m10 = word: row 10
    psubw           m11, m14                        ; m11 = word: row 11
    psubw           m12, m14                        ; m12 = word: row 12
    psubw           m13, m14                        ; m13 = word: row 13
    psubw           m0, m14                         ; m0 = word: row 14
    psubw           m1, m14                         ; m1 = word: row 15
    movu            [r2], m8
    movu            [r2 + r3], m9
    movu            [r2 + r3 * 2], m10
    movu            [r2 + r6], m11
    lea             r2, [r2 + r3 * 4]
    movu            [r2], m12
    movu            [r2 + r3], m13
    movu            [r2 + r3 * 2], m0
    movu            [r2 + r6], m1
%endif

    lea             r2, [r2 + r3 * 4]
    sub             r0, r7
    dec             r8d
    jnz             .loop
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_16xN 16, 32, pp
    FILTER_VER_LUMA_AVX2_16xN 16, 64, pp
    FILTER_VER_LUMA_AVX2_16xN 16, 32, ps
    FILTER_VER_LUMA_AVX2_16xN 16, 64, ps

%macro PROCESS_LUMA_AVX2_W16_16R 1
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 1 * mmsize]
    paddw           m0, m4
    pmaddubsw       m2, [r5]
    lea             r7, [r0 + r1 * 4]
    movu            xm4, [r7]                       ; m4 = row 4
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 1 * mmsize]
    paddw           m1, m5
    pmaddubsw       m3, [r5]
    movu            xm5, [r7 + r1]                  ; m5 = row 5
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 2 * mmsize]
    paddw           m0, m6
    pmaddubsw       m6, m4, [r5 + 1 * mmsize]
    paddw           m2, m6
    pmaddubsw       m4, [r5]
    movu            xm6, [r7 + r1 * 2]              ; m6 = row 6
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, [r5 + 2 * mmsize]
    paddw           m1, m7
    pmaddubsw       m7, m5, [r5 + 1 * mmsize]
    paddw           m3, m7
    pmaddubsw       m5, [r5]
    movu            xm7, [r7 + r4]                  ; m7 = row 7
    punpckhbw       xm8, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddubsw       m8, m6, [r5 + 3 * mmsize]
    paddw           m0, m8
    pmaddubsw       m8, m6, [r5 + 2 * mmsize]
    paddw           m2, m8
    pmaddubsw       m8, m6, [r5 + 1 * mmsize]
    paddw           m4, m8
    pmaddubsw       m6, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm8, [r7]                       ; m8 = row 8
    punpckhbw       xm9, xm7, xm8
    punpcklbw       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddubsw       m9, m7, [r5 + 3 * mmsize]
    paddw           m1, m9
    pmaddubsw       m9, m7, [r5 + 2 * mmsize]
    paddw           m3, m9
    pmaddubsw       m9, m7, [r5 + 1 * mmsize]
    paddw           m5, m9
    pmaddubsw       m7, [r5]
    movu            xm9, [r7 + r1]                  ; m9 = row 9
    punpckhbw       xm10, xm8, xm9
    punpcklbw       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddubsw       m10, m8, [r5 + 3 * mmsize]
    paddw           m2, m10
    pmaddubsw       m10, m8, [r5 + 2 * mmsize]
    paddw           m4, m10
    pmaddubsw       m10, m8, [r5 + 1 * mmsize]
    paddw           m6, m10
    pmaddubsw       m8, [r5]
    movu            xm10, [r7 + r1 * 2]             ; m10 = row 10
    punpckhbw       xm11, xm9, xm10
    punpcklbw       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddubsw       m11, m9, [r5 + 3 * mmsize]
    paddw           m3, m11
    pmaddubsw       m11, m9, [r5 + 2 * mmsize]
    paddw           m5, m11
    pmaddubsw       m11, m9, [r5 + 1 * mmsize]
    paddw           m7, m11
    pmaddubsw       m9, [r5]
    movu            xm11, [r7 + r4]                 ; m11 = row 11
    punpckhbw       xm12, xm10, xm11
    punpcklbw       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddubsw       m12, m10, [r5 + 3 * mmsize]
    paddw           m4, m12
    pmaddubsw       m12, m10, [r5 + 2 * mmsize]
    paddw           m6, m12
    pmaddubsw       m12, m10, [r5 + 1 * mmsize]
    paddw           m8, m12
    pmaddubsw       m10, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm12, [r7]                      ; m12 = row 12
    punpckhbw       xm13, xm11, xm12
    punpcklbw       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddubsw       m13, m11, [r5 + 3 * mmsize]
    paddw           m5, m13
    pmaddubsw       m13, m11, [r5 + 2 * mmsize]
    paddw           m7, m13
    pmaddubsw       m13, m11, [r5 + 1 * mmsize]
    paddw           m9, m13
    pmaddubsw       m11, [r5]

%ifidn %1,pp
    pmulhrsw        m0, m14                         ; m0 = word: row 0
    pmulhrsw        m1, m14                         ; m1 = word: row 1
    pmulhrsw        m2, m14                         ; m2 = word: row 2
    pmulhrsw        m3, m14                         ; m3 = word: row 3
    pmulhrsw        m4, m14                         ; m4 = word: row 4
    pmulhrsw        m5, m14                         ; m5 = word: row 5
    packuswb        m0, m1
    packuswb        m2, m3
    packuswb        m4, m5
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vpermq          m4, m4, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm5, m4, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
    lea             r8, [r2 + r3 * 4]
    movu            [r8], xm4
    movu            [r8 + r3], xm5
%else
    psubw           m0, m14                         ; m0 = word: row 0
    psubw           m1, m14                         ; m1 = word: row 1
    psubw           m2, m14                         ; m2 = word: row 2
    psubw           m3, m14                         ; m3 = word: row 3
    psubw           m4, m14                         ; m4 = word: row 4
    psubw           m5, m14                         ; m5 = word: row 5
    movu            [r2], m0
    movu            [r2 + r3], m1
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r6], m3
    lea             r8, [r2 + r3 * 4]
    movu            [r8], m4
    movu            [r8 + r3], m5
%endif

    movu            xm13, [r7 + r1]                 ; m13 = row 13
    punpckhbw       xm0, xm12, xm13
    punpcklbw       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddubsw       m0, m12, [r5 + 3 * mmsize]
    paddw           m6, m0
    pmaddubsw       m0, m12, [r5 + 2 * mmsize]
    paddw           m8, m0
    pmaddubsw       m0, m12, [r5 + 1 * mmsize]
    paddw           m10, m0
    pmaddubsw       m12, [r5]
    movu            xm0, [r7 + r1 * 2]              ; m0 = row 14
    punpckhbw       xm1, xm13, xm0
    punpcklbw       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddubsw       m1, m13, [r5 + 3 * mmsize]
    paddw           m7, m1
    pmaddubsw       m1, m13, [r5 + 2 * mmsize]
    paddw           m9, m1
    pmaddubsw       m1, m13, [r5 + 1 * mmsize]
    paddw           m11, m1
    pmaddubsw       m13, [r5]

%ifidn %1,pp
    pmulhrsw        m6, m14                         ; m6 = word: row 6
    pmulhrsw        m7, m14                         ; m7 = word: row 7
    packuswb        m6, m7
    vpermq          m6, m6, 11011000b
    vextracti128    xm7, m6, 1
    movu            [r8 + r3 * 2], xm6
    movu            [r8 + r6], xm7
%else
    psubw           m6, m14                         ; m6 = word: row 6
    psubw           m7, m14                         ; m7 = word: row 7
    movu            [r8 + r3 * 2], m6
    movu            [r8 + r6], m7
%endif

    lea             r8, [r8 + r3 * 4]

    movu            xm1, [r7 + r4]                  ; m1 = row 15
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m2, m0, [r5 + 3 * mmsize]
    paddw           m8, m2
    pmaddubsw       m2, m0, [r5 + 2 * mmsize]
    paddw           m10, m2
    pmaddubsw       m2, m0, [r5 + 1 * mmsize]
    paddw           m12, m2
    pmaddubsw       m0, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm2, [r7]                       ; m2 = row 16
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m3, m1, [r5 + 3 * mmsize]
    paddw           m9, m3
    pmaddubsw       m3, m1, [r5 + 2 * mmsize]
    paddw           m11, m3
    pmaddubsw       m3, m1, [r5 + 1 * mmsize]
    paddw           m13, m3
    pmaddubsw       m1, [r5]
    movu            xm3, [r7 + r1]                  ; m3 = row 17
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 3 * mmsize]
    paddw           m10, m4
    pmaddubsw       m4, m2, [r5 + 2 * mmsize]
    paddw           m12, m4
    pmaddubsw       m2, [r5 + 1 * mmsize]
    paddw           m0, m2
    movu            xm4, [r7 + r1 * 2]              ; m4 = row 18
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 3 * mmsize]
    paddw           m11, m5
    pmaddubsw       m5, m3, [r5 + 2 * mmsize]
    paddw           m13, m5
    pmaddubsw       m3, [r5 + 1 * mmsize]
    paddw           m1, m3
    movu            xm5, [r7 + r4]                  ; m5 = row 19
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 3 * mmsize]
    paddw           m12, m6
    pmaddubsw       m4, [r5 + 2 * mmsize]
    paddw           m0, m4
    lea             r7, [r7 + r1 * 4]
    movu            xm6, [r7]                       ; m6 = row 20
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, [r5 + 3 * mmsize]
    paddw           m13, m7
    pmaddubsw       m5, [r5 + 2 * mmsize]
    paddw           m1, m5
    movu            xm7, [r7 + r1]                  ; m7 = row 21
    punpckhbw       xm2, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm2, 1
    pmaddubsw       m6, [r5 + 3 * mmsize]
    paddw           m0, m6
    movu            xm2, [r7 + r1 * 2]              ; m2 = row 22
    punpckhbw       xm3, xm7, xm2
    punpcklbw       xm7, xm2
    vinserti128     m7, m7, xm3, 1
    pmaddubsw       m7, [r5 + 3 * mmsize]
    paddw           m1, m7

%ifidn %1,pp
    pmulhrsw        m8, m14                         ; m8 = word: row 8
    pmulhrsw        m9, m14                         ; m9 = word: row 9
    pmulhrsw        m10, m14                        ; m10 = word: row 10
    pmulhrsw        m11, m14                        ; m11 = word: row 11
    pmulhrsw        m12, m14                        ; m12 = word: row 12
    pmulhrsw        m13, m14                        ; m13 = word: row 13
    pmulhrsw        m0, m14                         ; m0 = word: row 14
    pmulhrsw        m1, m14                         ; m1 = word: row 15
    packuswb        m8, m9
    packuswb        m10, m11
    packuswb        m12, m13
    packuswb        m0, m1
    vpermq          m8, m8, 11011000b
    vpermq          m10, m10, 11011000b
    vpermq          m12, m12, 11011000b
    vpermq          m0, m0, 11011000b
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    vextracti128    xm13, m12, 1
    vextracti128    xm1, m0, 1
    movu            [r8], xm8
    movu            [r8 + r3], xm9
    movu            [r8 + r3 * 2], xm10
    movu            [r8 + r6], xm11
    lea             r8, [r8 + r3 * 4]
    movu            [r8], xm12
    movu            [r8 + r3], xm13
    movu            [r8 + r3 * 2], xm0
    movu            [r8 + r6], xm1
%else
    psubw           m8, m14                         ; m8 = word: row 8
    psubw           m9, m14                         ; m9 = word: row 9
    psubw           m10, m14                        ; m10 = word: row 10
    psubw           m11, m14                        ; m11 = word: row 11
    psubw           m12, m14                        ; m12 = word: row 12
    psubw           m13, m14                        ; m13 = word: row 13
    psubw           m0, m14                         ; m0 = word: row 14
    psubw           m1, m14                         ; m1 = word: row 15
    movu            [r8], m8
    movu            [r8 + r3], m9
    movu            [r8 + r3 * 2], m10
    movu            [r8 + r6], m11
    lea             r8, [r8 + r3 * 4]
    movu            [r8], m12
    movu            [r8 + r3], m13
    movu            [r8 + r3 * 2], m0
    movu            [r8 + r6], m1
%endif
%endmacro

%macro PROCESS_LUMA_AVX2_W16_8R 1
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, [r5 + 1 * mmsize]
    paddw           m0, m4
    pmaddubsw       m2, [r5]
    lea             r7, [r0 + r1 * 4]
    movu            xm4, [r7]                       ; m4 = row 4
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + 1 * mmsize]
    paddw           m1, m5
    pmaddubsw       m3, [r5]
    movu            xm5, [r7 + r1]                  ; m5 = row 5
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 2 * mmsize]
    paddw           m0, m6
    pmaddubsw       m6, m4, [r5 + 1 * mmsize]
    paddw           m2, m6
    pmaddubsw       m4, [r5]
    movu            xm6, [r7 + r1 * 2]              ; m6 = row 6
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, [r5 + 2 * mmsize]
    paddw           m1, m7
    pmaddubsw       m7, m5, [r5 + 1 * mmsize]
    paddw           m3, m7
    pmaddubsw       m5, [r5]
    movu            xm7, [r7 + r4]                  ; m7 = row 7
    punpckhbw       xm8, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddubsw       m8, m6, [r5 + 3 * mmsize]
    paddw           m0, m8
    pmaddubsw       m8, m6, [r5 + 2 * mmsize]
    paddw           m2, m8
    pmaddubsw       m8, m6, [r5 + 1 * mmsize]
    paddw           m4, m8
    pmaddubsw       m6, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm8, [r7]                       ; m8 = row 8
    punpckhbw       xm9, xm7, xm8
    punpcklbw       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddubsw       m9, m7, [r5 + 3 * mmsize]
    paddw           m1, m9
    pmaddubsw       m9, m7, [r5 + 2 * mmsize]
    paddw           m3, m9
    pmaddubsw       m9, m7, [r5 + 1 * mmsize]
    paddw           m5, m9
    pmaddubsw       m7, [r5]
    movu            xm9, [r7 + r1]                  ; m9 = row 9
    punpckhbw       xm10, xm8, xm9
    punpcklbw       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddubsw       m10, m8, [r5 + 3 * mmsize]
    paddw           m2, m10
    pmaddubsw       m10, m8, [r5 + 2 * mmsize]
    paddw           m4, m10
    pmaddubsw       m10, m8, [r5 + 1 * mmsize]
    paddw           m6, m10
    movu            xm10, [r7 + r1 * 2]             ; m10 = row 10
    punpckhbw       xm11, xm9, xm10
    punpcklbw       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddubsw       m11, m9, [r5 + 3 * mmsize]
    paddw           m3, m11
    pmaddubsw       m11, m9, [r5 + 2 * mmsize]
    paddw           m5, m11
    pmaddubsw       m11, m9, [r5 + 1 * mmsize]
    paddw           m7, m11
    movu            xm11, [r7 + r4]                 ; m11 = row 11
    punpckhbw       xm12, xm10, xm11
    punpcklbw       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddubsw       m12, m10, [r5 + 3 * mmsize]
    paddw           m4, m12
    pmaddubsw       m12, m10, [r5 + 2 * mmsize]
    paddw           m6, m12
    lea             r7, [r7 + r1 * 4]
    movu            xm12, [r7]                      ; m12 = row 12
    punpckhbw       xm13, xm11, xm12
    punpcklbw       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddubsw       m13, m11, [r5 + 3 * mmsize]
    paddw           m5, m13
    pmaddubsw       m13, m11, [r5 + 2 * mmsize]
    paddw           m7, m13

%ifidn %1,pp
    pmulhrsw        m0, m14                         ; m0 = word: row 0
    pmulhrsw        m1, m14                         ; m1 = word: row 1
    pmulhrsw        m2, m14                         ; m2 = word: row 2
    pmulhrsw        m3, m14                         ; m3 = word: row 3
    pmulhrsw        m4, m14                         ; m4 = word: row 4
    pmulhrsw        m5, m14                         ; m5 = word: row 5
    packuswb        m0, m1
    packuswb        m2, m3
    packuswb        m4, m5
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vpermq          m4, m4, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm5, m4, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
    lea             r8, [r2 + r3 * 4]
    movu            [r8], xm4
    movu            [r8 + r3], xm5
%else
    psubw           m0, m14                         ; m0 = word: row 0
    psubw           m1, m14                         ; m1 = word: row 1
    psubw           m2, m14                         ; m2 = word: row 2
    psubw           m3, m14                         ; m3 = word: row 3
    psubw           m4, m14                         ; m4 = word: row 4
    psubw           m5, m14                         ; m5 = word: row 5
    movu            [r2], m0
    movu            [r2 + r3], m1
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r6], m3
    lea             r8, [r2 + r3 * 4]
    movu            [r8], m4
    movu            [r8 + r3], m5
%endif

    movu            xm13, [r7 + r1]                 ; m13 = row 13
    punpckhbw       xm0, xm12, xm13
    punpcklbw       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddubsw       m0, m12, [r5 + 3 * mmsize]
    paddw           m6, m0
    movu            xm0, [r7 + r1 * 2]              ; m0 = row 14
    punpckhbw       xm1, xm13, xm0
    punpcklbw       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddubsw       m1, m13, [r5 + 3 * mmsize]
    paddw           m7, m1

%ifidn %1,pp
    pmulhrsw        m6, m14                         ; m6 = word: row 6
    pmulhrsw        m7, m14                         ; m7 = word: row 7
    packuswb        m6, m7
    vpermq          m6, m6, 11011000b
    vextracti128    xm7, m6, 1
    movu            [r8 + r3 * 2], xm6
    movu            [r8 + r6], xm7
%else
    psubw           m6, m14                         ; m6 = word: row 6
    psubw           m7, m14                         ; m7 = word: row 7
    movu            [r8 + r3 * 2], m6
    movu            [r8 + r6], m7
%endif
%endmacro

%macro FILTER_VER_LUMA_AVX2_24x32 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_24x32, 4, 11, 15
    mov             r4d, r4m
    shl             r4d, 7
%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif
    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,ps
    add             r3d, r3d
    vbroadcasti128  m14, [pw_2000]
%else
    mova            m14, [pw_512]
%endif
    lea             r6, [r3 * 3]
    lea             r10, [r1 * 4]
    mov             r9d, 2
.loopH:
    PROCESS_LUMA_AVX2_W16_16R %1
%ifidn %1,pp
    add             r2, 16
%else
    add             r2, 32
%endif
    add             r0, 16
    movq            xm1, [r0]                       ; m1 = row 0
    movq            xm2, [r0 + r1]                  ; m2 = row 1
    punpcklbw       xm1, xm2
    movq            xm3, [r0 + r1 * 2]              ; m3 = row 2
    punpcklbw       xm2, xm3
    vinserti128     m5, m1, xm2, 1
    pmaddubsw       m5, [r5]
    movq            xm4, [r0 + r4]                  ; m4 = row 3
    punpcklbw       xm3, xm4
    lea             r7, [r0 + r1 * 4]
    movq            xm1, [r7]                       ; m1 = row 4
    punpcklbw       xm4, xm1
    vinserti128     m2, m3, xm4, 1
    pmaddubsw       m0, m2, [r5 + 1 * mmsize]
    paddw           m5, m0
    pmaddubsw       m2, [r5]
    movq            xm3, [r7 + r1]                  ; m3 = row 5
    punpcklbw       xm1, xm3
    movq            xm4, [r7 + r1 * 2]              ; m4 = row 6
    punpcklbw       xm3, xm4
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m3, m1, [r5 + 2 * mmsize]
    paddw           m5, m3
    pmaddubsw       m0, m1, [r5 + 1 * mmsize]
    paddw           m2, m0
    pmaddubsw       m1, [r5]
    movq            xm3, [r7 + r4]                  ; m3 = row 7
    punpcklbw       xm4, xm3
    lea             r7, [r7 + r1 * 4]
    movq            xm0, [r7]                       ; m0 = row 8
    punpcklbw       xm3, xm0
    vinserti128     m4, m4, xm3, 1
    pmaddubsw       m3, m4, [r5 + 3 * mmsize]
    paddw           m5, m3
    pmaddubsw       m3, m4, [r5 + 2 * mmsize]
    paddw           m2, m3
    pmaddubsw       m3, m4, [r5 + 1 * mmsize]
    paddw           m1, m3
    pmaddubsw       m4, [r5]
    movq            xm3, [r7 + r1]                  ; m3 = row 9
    punpcklbw       xm0, xm3
    movq            xm6, [r7 + r1 * 2]              ; m6 = row 10
    punpcklbw       xm3, xm6
    vinserti128     m0, m0, xm3, 1
    pmaddubsw       m3, m0, [r5 + 3 * mmsize]
    paddw           m2, m3
    pmaddubsw       m3, m0, [r5 + 2 * mmsize]
    paddw           m1, m3
    pmaddubsw       m3, m0, [r5 + 1 * mmsize]
    paddw           m4, m3
    pmaddubsw       m0, [r5]

    movq            xm3, [r7 + r4]                  ; m3 = row 11
    punpcklbw       xm6, xm3
    lea             r7, [r7 + r1 * 4]
    movq            xm7, [r7]                       ; m7 = row 12
    punpcklbw       xm3, xm7
    vinserti128     m6, m6, xm3, 1
    pmaddubsw       m3, m6, [r5 + 3 * mmsize]
    paddw           m1, m3
    pmaddubsw       m3, m6, [r5 + 2 * mmsize]
    paddw           m4, m3
    pmaddubsw       m3, m6, [r5 + 1 * mmsize]
    paddw           m0, m3
    pmaddubsw       m6, [r5]
    movq            xm3, [r7 + r1]                  ; m3 = row 13
    punpcklbw       xm7, xm3
    movq            xm8, [r7 + r1 * 2]              ; m8 = row 14
    punpcklbw       xm3, xm8
    vinserti128     m7, m7, xm3, 1
    pmaddubsw       m3, m7, [r5 + 3 * mmsize]
    paddw           m4, m3
    pmaddubsw       m3, m7, [r5 + 2 * mmsize]
    paddw           m0, m3
    pmaddubsw       m3, m7, [r5 + 1 * mmsize]
    paddw           m6, m3
    pmaddubsw       m7, [r5]
    movq            xm3, [r7 + r4]                  ; m3 = row 15
    punpcklbw       xm8, xm3
    lea             r7, [r7 + r1 * 4]
    movq            xm9, [r7]                       ; m9 = row 16
    punpcklbw       xm3, xm9
    vinserti128     m8, m8, xm3, 1
    pmaddubsw       m3, m8, [r5 + 3 * mmsize]
    paddw           m0, m3
    pmaddubsw       m3, m8, [r5 + 2 * mmsize]
    paddw           m6, m3
    pmaddubsw       m3, m8, [r5 + 1 * mmsize]
    paddw           m7, m3
    pmaddubsw       m8, [r5]
    movq            xm3, [r7 + r1]                  ; m3 = row 17
    punpcklbw       xm9, xm3
    movq            xm10, [r7 + r1 * 2]             ; m10 = row 18
    punpcklbw       xm3, xm10
    vinserti128     m9, m9, xm3, 1
    pmaddubsw       m3, m9, [r5 + 3 * mmsize]
    paddw           m6, m3
    pmaddubsw       m3, m9, [r5 + 2 * mmsize]
    paddw           m7, m3
    pmaddubsw       m3, m9, [r5 + 1 * mmsize]
    paddw           m8, m3
    movq            xm3, [r7 + r4]                  ; m3 = row 19
    punpcklbw       xm10, xm3
    lea             r7, [r7 + r1 * 4]
    movq            xm9, [r7]                       ; m9 = row 20
    punpcklbw       xm3, xm9
    vinserti128     m10, m10, xm3, 1
    pmaddubsw       m3, m10, [r5 + 3 * mmsize]
    paddw           m7, m3
    pmaddubsw       m3, m10, [r5 + 2 * mmsize]
    paddw           m8, m3
    movq            xm3, [r7 + r1]                  ; m3 = row 21
    punpcklbw       xm9, xm3
    movq            xm10, [r7 + r1 * 2]             ; m10 = row 22
    punpcklbw       xm3, xm10
    vinserti128     m9, m9, xm3, 1
    pmaddubsw       m3, m9, [r5 + 3 * mmsize]
    paddw           m8, m3
%ifidn %1,pp
    pmulhrsw        m5, m14                         ; m5 = word: row 0, row 1
    pmulhrsw        m2, m14                         ; m2 = word: row 2, row 3
    pmulhrsw        m1, m14                         ; m1 = word: row 4, row 5
    pmulhrsw        m4, m14                         ; m4 = word: row 6, row 7
    pmulhrsw        m0, m14                         ; m0 = word: row 8, row 9
    pmulhrsw        m6, m14                         ; m6 = word: row 10, row 11
    pmulhrsw        m7, m14                         ; m7 = word: row 12, row 13
    pmulhrsw        m8, m14                         ; m8 = word: row 14, row 15
    packuswb        m5, m2
    packuswb        m1, m4
    packuswb        m0, m6
    packuswb        m7, m8
    vextracti128    xm2, m5, 1
    vextracti128    xm4, m1, 1
    vextracti128    xm6, m0, 1
    vextracti128    xm8, m7, 1
    movq            [r2], xm5
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm5
    movhps          [r2 + r6], xm2
    lea             r8, [r2 + r3 * 4]
    movq            [r8], xm1
    movq            [r8 + r3], xm4
    movhps          [r8 + r3 * 2], xm1
    movhps          [r8 + r6], xm4
    lea             r8, [r8 + r3 * 4]
    movq            [r8], xm0
    movq            [r8 + r3], xm6
    movhps          [r8 + r3 * 2], xm0
    movhps          [r8 + r6], xm6
    lea             r8, [r8 + r3 * 4]
    movq            [r8], xm7
    movq            [r8 + r3], xm8
    movhps          [r8 + r3 * 2], xm7
    movhps          [r8 + r6], xm8
%else
    psubw           m5, m14                         ; m5 = word: row 0, row 1
    psubw           m2, m14                         ; m2 = word: row 2, row 3
    psubw           m1, m14                         ; m1 = word: row 4, row 5
    psubw           m4, m14                         ; m4 = word: row 6, row 7
    psubw           m0, m14                         ; m0 = word: row 8, row 9
    psubw           m6, m14                         ; m6 = word: row 10, row 11
    psubw           m7, m14                         ; m7 = word: row 12, row 13
    psubw           m8, m14                         ; m8 = word: row 14, row 15
    vextracti128    xm3, m5, 1
    movu            [r2], xm5
    movu            [r2 + r3], xm3
    vextracti128    xm3, m2, 1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
    vextracti128    xm3, m1, 1
    lea             r8, [r2 + r3 * 4]
    movu            [r8], xm1
    movu            [r8 + r3], xm3
    vextracti128    xm3, m4, 1
    movu            [r8 + r3 * 2], xm4
    movu            [r8 + r6], xm3
    vextracti128    xm3, m0, 1
    lea             r8, [r8 + r3 * 4]
    movu            [r8], xm0
    movu            [r8 + r3], xm3
    vextracti128    xm3, m6, 1
    movu            [r8 + r3 * 2], xm6
    movu            [r8 + r6], xm3
    vextracti128    xm3, m7, 1
    lea             r8, [r8 + r3 * 4]
    movu            [r8], xm7
    movu            [r8 + r3], xm3
    vextracti128    xm3, m8, 1
    movu            [r8 + r3 * 2], xm8
    movu            [r8 + r6], xm3
%endif
    sub             r7, r10
    lea             r0, [r7 - 16]
%ifidn %1,pp
    lea             r2, [r8 + r3 * 4 - 16]
%else
    lea             r2, [r8 + r3 * 4 - 32]
%endif
    dec             r9d
    jnz             .loopH
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_24x32 pp
    FILTER_VER_LUMA_AVX2_24x32 ps

%macro FILTER_VER_LUMA_AVX2_32xN 3
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%3_%1x%2, 4, 12, 15
    mov             r4d, r4m
    shl             r4d, 7
%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif
    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %3,ps
    add             r3d, r3d
    vbroadcasti128  m14, [pw_2000]
%else
    mova            m14, [pw_512]
%endif
    lea             r6, [r3 * 3]
    lea             r11, [r1 * 4]
    mov             r9d, %2 / 16
.loopH:
    mov             r10d, %1 / 16
.loopW:
    PROCESS_LUMA_AVX2_W16_16R %3
%ifidn %3,pp
    add             r2, 16
%else
    add             r2, 32
%endif
    add             r0, 16
    dec             r10d
    jnz             .loopW
    sub             r7, r11
    lea             r0, [r7 - 16]
%ifidn %3,pp
    lea             r2, [r8 + r3 * 4 - 16]
%else
    lea             r2, [r8 + r3 * 4 - 32]
%endif
    dec             r9d
    jnz             .loopH
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_32xN 32, 32, pp
    FILTER_VER_LUMA_AVX2_32xN 32, 64, pp
    FILTER_VER_LUMA_AVX2_32xN 32, 32, ps
    FILTER_VER_LUMA_AVX2_32xN 32, 64, ps

%macro FILTER_VER_LUMA_AVX2_32x16 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_32x16, 4, 10, 15
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,ps
    add             r3d, r3d
    vbroadcasti128  m14, [pw_2000]
%else
    mova            m14, [pw_512]
%endif
    lea             r6, [r3 * 3]
    mov             r9d, 2
.loopW:
    PROCESS_LUMA_AVX2_W16_16R %1
%ifidn %1,pp
    add             r2, 16
%else
    add             r2, 32
%endif
    add             r0, 16
    dec             r9d
    jnz             .loopW
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_32x16 pp
    FILTER_VER_LUMA_AVX2_32x16 ps

%macro FILTER_VER_LUMA_AVX2_32x24 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_32x24, 4, 10, 15
    mov             r4d, r4m
    shl             r4d, 7
%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif
    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,ps
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
%ifidn %1,pp
    mova            m14, [pw_512]
%else
    vbroadcasti128  m14, [pw_2000]
%endif
    mov             r9d, 2
.loopW:
    PROCESS_LUMA_AVX2_W16_16R %1
%ifidn %1,pp
    add             r2, 16
%else
    add             r2, 32
%endif
    add             r0, 16
    dec             r9d
    jnz             .loopW
    lea             r9, [r1 * 4]
    sub             r7, r9
    lea             r0, [r7 - 16]
%ifidn %1,pp
    lea             r2, [r8 + r3 * 4 - 16]
%else
    lea             r2, [r8 + r3 * 4 - 32]
%endif
    mov             r9d, 2
.loop:
    PROCESS_LUMA_AVX2_W16_8R %1
%ifidn %1,pp
    add             r2, 16
%else
    add             r2, 32
%endif
    add             r0, 16
    dec             r9d
    jnz             .loop
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_32x24 pp
    FILTER_VER_LUMA_AVX2_32x24 ps

%macro FILTER_VER_LUMA_AVX2_32x8 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_32x8, 4, 10, 15
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,ps
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
%ifidn %1,pp
    mova            m14, [pw_512]
%else
    vbroadcasti128  m14, [pw_2000]
%endif
    mov             r9d, 2
.loopW:
    PROCESS_LUMA_AVX2_W16_8R %1
%ifidn %1,pp
    add             r2, 16
%else
    add             r2, 32
%endif
    add             r0, 16
    dec             r9d
    jnz             .loopW
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_32x8 pp
    FILTER_VER_LUMA_AVX2_32x8 ps

%macro FILTER_VER_LUMA_AVX2_48x64 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_48x64, 4, 12, 15
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4

%ifidn %1,ps
    add             r3d, r3d
%endif

    lea             r6, [r3 * 3]
    lea             r11, [r1 * 4]

%ifidn %1,pp
    mova            m14, [pw_512]
%else
    vbroadcasti128  m14, [pw_2000]
%endif

    mov             r9d, 4
.loopH:
    mov             r10d, 3
.loopW:
    PROCESS_LUMA_AVX2_W16_16R %1
%ifidn %1,pp
    add             r2, 16
%else
    add             r2, 32
%endif
    add             r0, 16
    dec             r10d
    jnz             .loopW
    sub             r7, r11
    lea             r0, [r7 - 32]
%ifidn %1,pp
    lea             r2, [r8 + r3 * 4 - 32]
%else
    lea             r2, [r8 + r3 * 4 - 64]
%endif
    dec             r9d
    jnz             .loopH
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_48x64 pp
    FILTER_VER_LUMA_AVX2_48x64 ps

%macro FILTER_VER_LUMA_AVX2_64xN 3
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%3_%1x%2, 4, 12, 15
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4

%ifidn %3,ps
    add             r3d, r3d
%endif

    lea             r6, [r3 * 3]
    lea             r11, [r1 * 4]

%ifidn %3,pp
    mova            m14, [pw_512]
%else
    vbroadcasti128  m14, [pw_2000]
%endif

    mov             r9d, %2 / 16
.loopH:
    mov             r10d, %1 / 16
.loopW:
    PROCESS_LUMA_AVX2_W16_16R %3
%ifidn %3,pp
    add             r2, 16
%else
    add             r2, 32
%endif
    add             r0, 16
    dec             r10d
    jnz             .loopW
    sub             r7, r11
    lea             r0, [r7 - 48]
%ifidn %3,pp
    lea             r2, [r8 + r3 * 4 - 48]
%else
    lea             r2, [r8 + r3 * 4 - 96]
%endif
    dec             r9d
    jnz             .loopH
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_64xN 64, 32, pp
    FILTER_VER_LUMA_AVX2_64xN 64, 48, pp
    FILTER_VER_LUMA_AVX2_64xN 64, 64, pp
    FILTER_VER_LUMA_AVX2_64xN 64, 32, ps
    FILTER_VER_LUMA_AVX2_64xN 64, 48, ps
    FILTER_VER_LUMA_AVX2_64xN 64, 64, ps

%macro FILTER_VER_LUMA_AVX2_64x16 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_64x16, 4, 10, 15
    mov             r4d, r4m
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4

%ifidn %1,ps
    add             r3d, r3d
%endif

    lea             r6, [r3 * 3]

%ifidn %1,pp
    mova            m14, [pw_512]
%else
    vbroadcasti128  m14, [pw_2000]
%endif

    mov             r9d, 4
.loopW:
    PROCESS_LUMA_AVX2_W16_16R %1
%ifidn %1,pp
    add             r2, 16
%else
    add             r2, 32
%endif
    add             r0, 16
    dec             r9d
    jnz             .loopW
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_64x16 pp
    FILTER_VER_LUMA_AVX2_64x16 ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA 3
INIT_XMM sse4
cglobal interp_8tap_vert_%3_%1x%2, 5, 7, 8 ,0-gprsize
    lea       r5, [3 * r1]
    sub       r0, r5
    shl       r4d, 6
%ifidn %3,ps
    add       r3d, r3d
%endif

%ifdef PIC
    lea       r5, [tab_LumaCoeffVer]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_LumaCoeffVer + r4]
%endif

%ifidn %3,pp
    mova      m3, [pw_512]
%else
    mova      m3, [pw_2000]
%endif
    mov       dword [rsp], %2/4

.loopH:
    mov       r4d, (%1/8)
.loopW:
    PROCESS_LUMA_W8_4R
%ifidn %3,pp
    pmulhrsw  m7, m3
    pmulhrsw  m6, m3
    pmulhrsw  m5, m3
    pmulhrsw  m4, m3

    packuswb  m7, m6
    packuswb  m5, m4

    movlps    [r2], m7
    movhps    [r2 + r3], m7
    lea       r5, [r2 + 2 * r3]
    movlps    [r5], m5
    movhps    [r5 + r3], m5
%else
    psubw     m7, m3
    psubw     m6, m3
    psubw     m5, m3
    psubw     m4, m3

    movu      [r2], m7
    movu      [r2 + r3], m6
    lea       r5, [r2 + 2 * r3]
    movu      [r5], m5
    movu      [r5 + r3], m4
%endif

    lea       r5, [8 * r1 - 8]
    sub       r0, r5
%ifidn %3,pp
    add       r2, 8
%else
    add       r2, 16
%endif
    dec       r4d
    jnz       .loopW

    lea       r0, [r0 + 4 * r1 - %1]
%ifidn %3,pp
    lea       r2, [r2 + 4 * r3 - %1]
%else
    lea       r2, [r2 + 4 * r3 - 2 * %1]
%endif

    dec       dword [rsp]
    jnz       .loopH

    RET
%endmacro

    FILTER_VER_LUMA 16, 4, pp
    FILTER_VER_LUMA 16, 8, pp
    FILTER_VER_LUMA 16, 12, pp
    FILTER_VER_LUMA 16, 16, pp
    FILTER_VER_LUMA 16, 32, pp
    FILTER_VER_LUMA 16, 64, pp
    FILTER_VER_LUMA 24, 32, pp
    FILTER_VER_LUMA 32, 8, pp
    FILTER_VER_LUMA 32, 16, pp
    FILTER_VER_LUMA 32, 24, pp
    FILTER_VER_LUMA 32, 32, pp
    FILTER_VER_LUMA 32, 64, pp
    FILTER_VER_LUMA 48, 64, pp
    FILTER_VER_LUMA 64, 16, pp
    FILTER_VER_LUMA 64, 32, pp
    FILTER_VER_LUMA 64, 48, pp
    FILTER_VER_LUMA 64, 64, pp

    FILTER_VER_LUMA 16, 4, ps
    FILTER_VER_LUMA 16, 8, ps
    FILTER_VER_LUMA 16, 12, ps
    FILTER_VER_LUMA 16, 16, ps
    FILTER_VER_LUMA 16, 32, ps
    FILTER_VER_LUMA 16, 64, ps
    FILTER_VER_LUMA 24, 32, ps
    FILTER_VER_LUMA 32, 8, ps
    FILTER_VER_LUMA 32, 16, ps
    FILTER_VER_LUMA 32, 24, ps
    FILTER_VER_LUMA 32, 32, ps
    FILTER_VER_LUMA 32, 64, ps
    FILTER_VER_LUMA 48, 64, ps
    FILTER_VER_LUMA 64, 16, ps
    FILTER_VER_LUMA 64, 32, ps
    FILTER_VER_LUMA 64, 48, ps
    FILTER_VER_LUMA 64, 64, ps

%macro PROCESS_LUMA_SP_W4_4R 0
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r6 + 0 *16]                ;m0=[0+1]  Row1

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m1, m4                          ;m1=[1 2]
    pmaddwd    m1, [r6 + 0 *16]                ;m1=[1+2]  Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[2 3]
    pmaddwd    m2, m4, [r6 + 0 *16]            ;m2=[2+3]  Row3
    pmaddwd    m4, [r6 + 1 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3]  Row1

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m5, m4                          ;m5=[3 4]
    pmaddwd    m3, m5, [r6 + 0 *16]            ;m3=[3+4]  Row4
    pmaddwd    m5, [r6 + 1 * 16]
    paddd      m1, m5                          ;m1 = [1+2+3+4]  Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[4 5]
    pmaddwd    m6, m4, [r6 + 1 * 16]
    paddd      m2, m6                          ;m2=[2+3+4+5]  Row3
    pmaddwd    m4, [r6 + 2 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3+4+5]  Row1

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m5, m4                          ;m5=[5 6]
    pmaddwd    m6, m5, [r6 + 1 * 16]
    paddd      m3, m6                          ;m3=[3+4+5+6]  Row4
    pmaddwd    m5, [r6 + 2 * 16]
    paddd      m1, m5                          ;m1=[1+2+3+4+5+6]  Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[6 7]
    pmaddwd    m6, m4, [r6 + 2 * 16]
    paddd      m2, m6                          ;m2=[2+3+4+5+6+7]  Row3
    pmaddwd    m4, [r6 + 3 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3+4+5+6+7]  Row1 end

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m5, m4                          ;m5=[7 8]
    pmaddwd    m6, m5, [r6 + 2 * 16]
    paddd      m3, m6                          ;m3=[3+4+5+6+7+8]  Row4
    pmaddwd    m5, [r6 + 3 * 16]
    paddd      m1, m5                          ;m1=[1+2+3+4+5+6+7+8]  Row2 end

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[8 9]
    pmaddwd    m4, [r6 + 3 * 16]
    paddd      m2, m4                          ;m2=[2+3+4+5+6+7+8+9]  Row3 end

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[9 10]
    pmaddwd    m5, [r6 + 3 * 16]
    paddd      m3, m5                          ;m3=[3+4+5+6+7+8+9+10]  Row4 end
%endmacro

;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_sp_%1x%2(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_SP 2
INIT_XMM sse4
cglobal interp_8tap_vert_sp_%1x%2, 5, 7, 8 ,0-gprsize

    add       r1d, r1d
    lea       r5, [r1 + 2 * r1]
    sub       r0, r5
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_LumaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_LumaCoeffV + r4]
%endif

    mova      m7, [pd_526336]

    mov       dword [rsp], %2/4
.loopH:
    mov       r4d, (%1/4)
.loopW:
    PROCESS_LUMA_SP_W4_4R

    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7

    psrad     m0, 12
    psrad     m1, 12
    psrad     m2, 12
    psrad     m3, 12

    packssdw  m0, m1
    packssdw  m2, m3

    packuswb  m0, m2

    movd      [r2], m0
    pextrd    [r2 + r3], m0, 1
    lea       r5, [r2 + 2 * r3]
    pextrd    [r5], m0, 2
    pextrd    [r5 + r3], m0, 3

    lea       r5, [8 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 4

    dec       r4d
    jnz       .loopW

    lea       r0, [r0 + 4 * r1 - 2 * %1]
    lea       r2, [r2 + 4 * r3 - %1]

    dec       dword [rsp]
    jnz       .loopH

    RET
%endmacro

;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_sp_%1x%2(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_SP 4, 4
    FILTER_VER_LUMA_SP 8, 8
    FILTER_VER_LUMA_SP 8, 4
    FILTER_VER_LUMA_SP 4, 8
    FILTER_VER_LUMA_SP 16, 16
    FILTER_VER_LUMA_SP 16, 8
    FILTER_VER_LUMA_SP 8, 16
    FILTER_VER_LUMA_SP 16, 12
    FILTER_VER_LUMA_SP 12, 16
    FILTER_VER_LUMA_SP 16, 4
    FILTER_VER_LUMA_SP 4, 16
    FILTER_VER_LUMA_SP 32, 32
    FILTER_VER_LUMA_SP 32, 16
    FILTER_VER_LUMA_SP 16, 32
    FILTER_VER_LUMA_SP 32, 24
    FILTER_VER_LUMA_SP 24, 32
    FILTER_VER_LUMA_SP 32, 8
    FILTER_VER_LUMA_SP 8, 32
    FILTER_VER_LUMA_SP 64, 64
    FILTER_VER_LUMA_SP 64, 32
    FILTER_VER_LUMA_SP 32, 64
    FILTER_VER_LUMA_SP 64, 48
    FILTER_VER_LUMA_SP 48, 64
    FILTER_VER_LUMA_SP 64, 16
    FILTER_VER_LUMA_SP 16, 64

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal filterPixelToShort_4x2, 3, 4, 3
    mov         r3d, r3m
    add         r3d, r3d

    ; load constant
    mova        m1, [pb_128]
    mova        m2, [tab_c_64_n64]

    movd        m0, [r0]
    pinsrd      m0, [r0 + r1], 1
    punpcklbw   m0, m1
    pmaddubsw   m0, m2

    movq        [r2 + r3 * 0], m0
    movhps      [r2 + r3 * 1], m0

    RET

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int16_t dstStride)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal filterPixelToShort_8x2, 3, 4, 3
    mov         r3d, r3m
    add         r3d, r3d

    ; load constant
    mova        m1, [pb_128]
    mova        m2, [tab_c_64_n64]

    movh        m0, [r0]
    punpcklbw   m0, m1
    pmaddubsw   m0, m2
    movu        [r2 + r3 * 0], m0

    movh        m0, [r0 + r1]
    punpcklbw   m0, m1
    pmaddubsw   m0, m2
    movu        [r2 + r3 * 1], m0

    RET

;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ss_%1x%2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_SS 2
INIT_XMM sse2
cglobal interp_8tap_vert_ss_%1x%2, 5, 7, 7 ,0-gprsize

    add        r1d, r1d
    add        r3d, r3d
    lea        r5, [3 * r1]
    sub        r0, r5
    shl        r4d, 6

%ifdef PIC
    lea        r5, [tab_LumaCoeffV]
    lea        r6, [r5 + r4]
%else
    lea        r6, [tab_LumaCoeffV + r4]
%endif

    mov        dword [rsp], %2/4
.loopH:
    mov        r4d, (%1/4)
.loopW:
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r6 + 0 *16]                ;m0=[0+1]  Row1

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m1, m4                          ;m1=[1 2]
    pmaddwd    m1, [r6 + 0 *16]                ;m1=[1+2]  Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[2 3]
    pmaddwd    m2, m4, [r6 + 0 *16]            ;m2=[2+3]  Row3
    pmaddwd    m4, [r6 + 1 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3]  Row1

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m5, m4                          ;m5=[3 4]
    pmaddwd    m3, m5, [r6 + 0 *16]            ;m3=[3+4]  Row4
    pmaddwd    m5, [r6 + 1 * 16]
    paddd      m1, m5                          ;m1 = [1+2+3+4]  Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[4 5]
    pmaddwd    m6, m4, [r6 + 1 * 16]
    paddd      m2, m6                          ;m2=[2+3+4+5]  Row3
    pmaddwd    m4, [r6 + 2 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3+4+5]  Row1

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m5, m4                          ;m5=[5 6]
    pmaddwd    m6, m5, [r6 + 1 * 16]
    paddd      m3, m6                          ;m3=[3+4+5+6]  Row4
    pmaddwd    m5, [r6 + 2 * 16]
    paddd      m1, m5                          ;m1=[1+2+3+4+5+6]  Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[6 7]
    pmaddwd    m6, m4, [r6 + 2 * 16]
    paddd      m2, m6                          ;m2=[2+3+4+5+6+7]  Row3
    pmaddwd    m4, [r6 + 3 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3+4+5+6+7]  Row1 end
    psrad      m0, 6

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m5, m4                          ;m5=[7 8]
    pmaddwd    m6, m5, [r6 + 2 * 16]
    paddd      m3, m6                          ;m3=[3+4+5+6+7+8]  Row4
    pmaddwd    m5, [r6 + 3 * 16]
    paddd      m1, m5                          ;m1=[1+2+3+4+5+6+7+8]  Row2 end
    psrad      m1, 6

    packssdw   m0, m1

    movlps     [r2], m0
    movhps     [r2 + r3], m0

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[8 9]
    pmaddwd    m4, [r6 + 3 * 16]
    paddd      m2, m4                          ;m2=[2+3+4+5+6+7+8+9]  Row3 end
    psrad      m2, 6

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[9 10]
    pmaddwd    m5, [r6 + 3 * 16]
    paddd      m3, m5                          ;m3=[3+4+5+6+7+8+9+10]  Row4 end
    psrad      m3, 6

    packssdw   m2, m3

    movlps     [r2 + 2 * r3], m2
    lea        r5, [3 * r3]
    movhps     [r2 + r5], m2

    lea        r5, [8 * r1 - 2 * 4]
    sub        r0, r5
    add        r2, 2 * 4

    dec        r4d
    jnz        .loopW

    lea        r0, [r0 + 4 * r1 - 2 * %1]
    lea        r2, [r2 + 4 * r3 - 2 * %1]

    dec        dword [rsp]
    jnz        .loopH

    RET
%endmacro

    FILTER_VER_LUMA_SS 4, 4
    FILTER_VER_LUMA_SS 8, 8
    FILTER_VER_LUMA_SS 8, 4
    FILTER_VER_LUMA_SS 4, 8
    FILTER_VER_LUMA_SS 16, 16
    FILTER_VER_LUMA_SS 16, 8
    FILTER_VER_LUMA_SS 8, 16
    FILTER_VER_LUMA_SS 16, 12
    FILTER_VER_LUMA_SS 12, 16
    FILTER_VER_LUMA_SS 16, 4
    FILTER_VER_LUMA_SS 4, 16
    FILTER_VER_LUMA_SS 32, 32
    FILTER_VER_LUMA_SS 32, 16
    FILTER_VER_LUMA_SS 16, 32
    FILTER_VER_LUMA_SS 32, 24
    FILTER_VER_LUMA_SS 24, 32
    FILTER_VER_LUMA_SS 32, 8
    FILTER_VER_LUMA_SS 8, 32
    FILTER_VER_LUMA_SS 64, 64
    FILTER_VER_LUMA_SS 64, 32
    FILTER_VER_LUMA_SS 32, 64
    FILTER_VER_LUMA_SS 64, 48
    FILTER_VER_LUMA_SS 48, 64
    FILTER_VER_LUMA_SS 64, 16
    FILTER_VER_LUMA_SS 16, 64

%macro FILTER_VER_LUMA_AVX2_4x4 1
INIT_YMM avx2
cglobal interp_8tap_vert_%1_4x4, 4, 6, 7
    mov             r4d, r4m
    add             r1d, r1d
    shl             r4d, 7

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4

%ifidn %1,sp
    mova            m6, [pd_526336]
%else
    add             r3d, r3d
%endif

    movq            xm0, [r0]
    movq            xm1, [r0 + r1]
    punpcklwd       xm0, xm1
    movq            xm2, [r0 + r1 * 2]
    punpcklwd       xm1, xm2
    vinserti128     m0, m0, xm1, 1                  ; m0 = [2 1 1 0]
    pmaddwd         m0, [r5]
    movq            xm3, [r0 + r4]
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm4, [r0]
    punpcklwd       xm3, xm4
    vinserti128     m2, m2, xm3, 1                  ; m2 = [4 3 3 2]
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    pmaddwd         m2, [r5]
    paddd           m0, m5
    movq            xm3, [r0 + r1]
    punpcklwd       xm4, xm3
    movq            xm1, [r0 + r1 * 2]
    punpcklwd       xm3, xm1
    vinserti128     m4, m4, xm3, 1                  ; m4 = [6 5 5 4]
    pmaddwd         m5, m4, [r5 + 2 * mmsize]
    pmaddwd         m4, [r5 + 1 * mmsize]
    paddd           m0, m5
    paddd           m2, m4
    movq            xm3, [r0 + r4]
    punpcklwd       xm1, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm4, [r0]
    punpcklwd       xm3, xm4
    vinserti128     m1, m1, xm3, 1                  ; m1 = [8 7 7 6]
    pmaddwd         m5, m1, [r5 + 3 * mmsize]
    pmaddwd         m1, [r5 + 2 * mmsize]
    paddd           m0, m5
    paddd           m2, m1
    movq            xm3, [r0 + r1]
    punpcklwd       xm4, xm3
    movq            xm1, [r0 + 2 * r1]
    punpcklwd       xm3, xm1
    vinserti128     m4, m4, xm3, 1                  ; m4 = [A 9 9 8]
    pmaddwd         m4, [r5 + 3 * mmsize]
    paddd           m2, m4

%ifidn %1,sp
    paddd           m0, m6
    paddd           m2, m6
    psrad           m0, 12
    psrad           m2, 12
%else
    psrad           m0, 6
    psrad           m2, 6
%endif
    packssdw        m0, m2
    vextracti128    xm2, m0, 1
    lea             r4, [r3 * 3]

%ifidn %1,sp
    packuswb        xm0, xm2
    movd            [r2], xm0
    pextrd          [r2 + r3], xm0, 2
    pextrd          [r2 + r3 * 2], xm0, 1
    pextrd          [r2 + r4], xm0, 3
%else
    movq            [r2], xm0
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r4], xm2
%endif
    RET
%endmacro

    FILTER_VER_LUMA_AVX2_4x4 sp
    FILTER_VER_LUMA_AVX2_4x4 ss

%macro FILTER_VER_LUMA_AVX2_4x8 1
INIT_YMM avx2
cglobal interp_8tap_vert_%1_4x8, 4, 7, 8
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4

%ifidn %1,sp
    mova            m7, [pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]

    movq            xm0, [r0]
    movq            xm1, [r0 + r1]
    punpcklwd       xm0, xm1
    movq            xm2, [r0 + r1 * 2]
    punpcklwd       xm1, xm2
    vinserti128     m0, m0, xm1, 1                  ; m0 = [2 1 1 0]
    pmaddwd         m0, [r5]
    movq            xm3, [r0 + r4]
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm4, [r0]
    punpcklwd       xm3, xm4
    vinserti128     m2, m2, xm3, 1                  ; m2 = [4 3 3 2]
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    pmaddwd         m2, [r5]
    paddd           m0, m5
    movq            xm3, [r0 + r1]
    punpcklwd       xm4, xm3
    movq            xm1, [r0 + r1 * 2]
    punpcklwd       xm3, xm1
    vinserti128     m4, m4, xm3, 1                  ; m4 = [6 5 5 4]
    pmaddwd         m5, m4, [r5 + 2 * mmsize]
    paddd           m0, m5
    pmaddwd         m5, m4, [r5 + 1 * mmsize]
    paddd           m2, m5
    pmaddwd         m4, [r5]
    movq            xm3, [r0 + r4]
    punpcklwd       xm1, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm6, [r0]
    punpcklwd       xm3, xm6
    vinserti128     m1, m1, xm3, 1                  ; m1 = [8 7 7 6]
    pmaddwd         m5, m1, [r5 + 3 * mmsize]
    paddd           m0, m5
    pmaddwd         m5, m1, [r5 + 2 * mmsize]
    paddd           m2, m5
    pmaddwd         m5, m1, [r5 + 1 * mmsize]
    paddd           m4, m5
    pmaddwd         m1, [r5]
    movq            xm3, [r0 + r1]
    punpcklwd       xm6, xm3
    movq            xm5, [r0 + 2 * r1]
    punpcklwd       xm3, xm5
    vinserti128     m6, m6, xm3, 1                  ; m6 = [A 9 9 8]
    pmaddwd         m3, m6, [r5 + 3 * mmsize]
    paddd           m2, m3
    pmaddwd         m3, m6, [r5 + 2 * mmsize]
    paddd           m4, m3
    pmaddwd         m6, [r5 + 1 * mmsize]
    paddd           m1, m6

%ifidn %1,sp
    paddd           m0, m7
    paddd           m2, m7
    psrad           m0, 12
    psrad           m2, 12
%else
    psrad           m0, 6
    psrad           m2, 6
%endif
    packssdw        m0, m2

    movq            xm3, [r0 + r4]
    punpcklwd       xm5, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm2, [r0]
    punpcklwd       xm3, xm2
    vinserti128     m5, m5, xm3, 1                  ; m5 = [C B B A]
    pmaddwd         m3, m5, [r5 + 3 * mmsize]
    paddd           m4, m3
    pmaddwd         m5, [r5 + 2 * mmsize]
    paddd           m1, m5
    movq            xm3, [r0 + r1]
    punpcklwd       xm2, xm3
    movq            xm5, [r0 + 2 * r1]
    punpcklwd       xm3, xm5
    vinserti128     m2, m2, xm3, 1                  ; m2 = [E D D C]
    pmaddwd         m2, [r5 + 3 * mmsize]
    paddd           m1, m2

%ifidn %1,sp
    paddd           m4, m7
    paddd           m1, m7
    psrad           m4, 12
    psrad           m1, 12
%else
    psrad           m4, 6
    psrad           m1, 6
%endif
    packssdw        m4, m1

%ifidn %1,sp
    packuswb        m0, m4
    vextracti128    xm2, m0, 1
    movd            [r2], xm0
    movd            [r2 + r3], xm2
    pextrd          [r2 + r3 * 2], xm0, 1
    pextrd          [r2 + r6], xm2, 1
    lea             r2, [r2 + r3 * 4]
    pextrd          [r2], xm0, 2
    pextrd          [r2 + r3], xm2, 2
    pextrd          [r2 + r3 * 2], xm0, 3
    pextrd          [r2 + r6], xm2, 3
%else
    vextracti128    xm2, m0, 1
    vextracti128    xm1, m4, 1
    movq            [r2], xm0
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r6], xm2
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm4
    movq            [r2 + r3], xm1
    movhps          [r2 + r3 * 2], xm4
    movhps          [r2 + r6], xm1
%endif
    RET
%endmacro

    FILTER_VER_LUMA_AVX2_4x8 sp
    FILTER_VER_LUMA_AVX2_4x8 ss

%macro PROCESS_LUMA_AVX2_W4_16R 1
    movq            xm0, [r0]
    movq            xm1, [r0 + r1]
    punpcklwd       xm0, xm1
    movq            xm2, [r0 + r1 * 2]
    punpcklwd       xm1, xm2
    vinserti128     m0, m0, xm1, 1                  ; m0 = [2 1 1 0]
    pmaddwd         m0, [r5]
    movq            xm3, [r0 + r4]
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm4, [r0]
    punpcklwd       xm3, xm4
    vinserti128     m2, m2, xm3, 1                  ; m2 = [4 3 3 2]
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    pmaddwd         m2, [r5]
    paddd           m0, m5
    movq            xm3, [r0 + r1]
    punpcklwd       xm4, xm3
    movq            xm1, [r0 + r1 * 2]
    punpcklwd       xm3, xm1
    vinserti128     m4, m4, xm3, 1                  ; m4 = [6 5 5 4]
    pmaddwd         m5, m4, [r5 + 2 * mmsize]
    paddd           m0, m5
    pmaddwd         m5, m4, [r5 + 1 * mmsize]
    paddd           m2, m5
    pmaddwd         m4, [r5]
    movq            xm3, [r0 + r4]
    punpcklwd       xm1, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm6, [r0]
    punpcklwd       xm3, xm6
    vinserti128     m1, m1, xm3, 1                  ; m1 = [8 7 7 6]
    pmaddwd         m5, m1, [r5 + 3 * mmsize]
    paddd           m0, m5
    pmaddwd         m5, m1, [r5 + 2 * mmsize]
    paddd           m2, m5
    pmaddwd         m5, m1, [r5 + 1 * mmsize]
    paddd           m4, m5
    pmaddwd         m1, [r5]
    movq            xm3, [r0 + r1]
    punpcklwd       xm6, xm3
    movq            xm5, [r0 + 2 * r1]
    punpcklwd       xm3, xm5
    vinserti128     m6, m6, xm3, 1                  ; m6 = [10 9 9 8]
    pmaddwd         m3, m6, [r5 + 3 * mmsize]
    paddd           m2, m3
    pmaddwd         m3, m6, [r5 + 2 * mmsize]
    paddd           m4, m3
    pmaddwd         m3, m6, [r5 + 1 * mmsize]
    paddd           m1, m3
    pmaddwd         m6, [r5]

%ifidn %1,sp
    paddd           m0, m7
    paddd           m2, m7
    psrad           m0, 12
    psrad           m2, 12
%else
    psrad           m0, 6
    psrad           m2, 6
%endif
    packssdw        m0, m2
    vextracti128    xm2, m0, 1
%ifidn %1,sp
    packuswb        xm0, xm2
    movd            [r2], xm0
    pextrd          [r2 + r3], xm0, 2
    pextrd          [r2 + r3 * 2], xm0, 1
    pextrd          [r2 + r6], xm0, 3
%else
    movq            [r2], xm0
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r6], xm2
%endif

    movq            xm2, [r0 + r4]
    punpcklwd       xm5, xm2
    lea             r0, [r0 + 4 * r1]
    movq            xm0, [r0]
    punpcklwd       xm2, xm0
    vinserti128     m5, m5, xm2, 1                  ; m5 = [12 11 11 10]
    pmaddwd         m2, m5, [r5 + 3 * mmsize]
    paddd           m4, m2
    pmaddwd         m2, m5, [r5 + 2 * mmsize]
    paddd           m1, m2
    pmaddwd         m2, m5, [r5 + 1 * mmsize]
    paddd           m6, m2
    pmaddwd         m5, [r5]
    movq            xm2, [r0 + r1]
    punpcklwd       xm0, xm2
    movq            xm3, [r0 + 2 * r1]
    punpcklwd       xm2, xm3
    vinserti128     m0, m0, xm2, 1                  ; m0 = [14 13 13 12]
    pmaddwd         m2, m0, [r5 + 3 * mmsize]
    paddd           m1, m2
    pmaddwd         m2, m0, [r5 + 2 * mmsize]
    paddd           m6, m2
    pmaddwd         m2, m0, [r5 + 1 * mmsize]
    paddd           m5, m2
    pmaddwd         m0, [r5]

%ifidn %1,sp
    paddd           m4, m7
    paddd           m1, m7
    psrad           m4, 12
    psrad           m1, 12
%else
    psrad           m4, 6
    psrad           m1, 6
%endif
    packssdw        m4, m1
    vextracti128    xm1, m4, 1
    lea             r2, [r2 + r3 * 4]
%ifidn %1,sp
    packuswb        xm4, xm1
    movd            [r2], xm4
    pextrd          [r2 + r3], xm4, 2
    pextrd          [r2 + r3 * 2], xm4, 1
    pextrd          [r2 + r6], xm4, 3
%else
    movq            [r2], xm4
    movq            [r2 + r3], xm1
    movhps          [r2 + r3 * 2], xm4
    movhps          [r2 + r6], xm1
%endif

    movq            xm4, [r0 + r4]
    punpcklwd       xm3, xm4
    lea             r0, [r0 + 4 * r1]
    movq            xm1, [r0]
    punpcklwd       xm4, xm1
    vinserti128     m3, m3, xm4, 1                  ; m3 = [16 15 15 14]
    pmaddwd         m4, m3, [r5 + 3 * mmsize]
    paddd           m6, m4
    pmaddwd         m4, m3, [r5 + 2 * mmsize]
    paddd           m5, m4
    pmaddwd         m4, m3, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m3, [r5]
    movq            xm4, [r0 + r1]
    punpcklwd       xm1, xm4
    movq            xm2, [r0 + 2 * r1]
    punpcklwd       xm4, xm2
    vinserti128     m1, m1, xm4, 1                  ; m1 = [18 17 17 16]
    pmaddwd         m4, m1, [r5 + 3 * mmsize]
    paddd           m5, m4
    pmaddwd         m4, m1, [r5 + 2 * mmsize]
    paddd           m0, m4
    pmaddwd         m1, [r5 + 1 * mmsize]
    paddd           m3, m1
    movq            xm4, [r0 + r4]
    punpcklwd       xm2, xm4
    lea             r0, [r0 + 4 * r1]
    movq            xm1, [r0]
    punpcklwd       xm4, xm1
    vinserti128     m2, m2, xm4, 1                  ; m2 = [20 19 19 18]
    pmaddwd         m4, m2, [r5 + 3 * mmsize]
    paddd           m0, m4
    pmaddwd         m2, [r5 + 2 * mmsize]
    paddd           m3, m2
    movq            xm4, [r0 + r1]
    punpcklwd       xm1, xm4
    movq            xm2, [r0 + 2 * r1]
    punpcklwd       xm4, xm2
    vinserti128     m1, m1, xm4, 1                  ; m1 = [22 21 21 20]
    pmaddwd         m1, [r5 + 3 * mmsize]
    paddd           m3, m1

%ifidn %1,sp
    paddd           m6, m7
    paddd           m5, m7
    paddd           m0, m7
    paddd           m3, m7
    psrad           m6, 12
    psrad           m5, 12
    psrad           m0, 12
    psrad           m3, 12
%else
    psrad           m6, 6
    psrad           m5, 6
    psrad           m0, 6
    psrad           m3, 6
%endif
    packssdw        m6, m5
    packssdw        m0, m3
    lea             r2, [r2 + r3 * 4]

%ifidn %1,sp
    packuswb        m6, m0
    vextracti128    xm0, m6, 1
    movd            [r2], xm6
    movd            [r2 + r3], xm0
    pextrd          [r2 + r3 * 2], xm6, 1
    pextrd          [r2 + r6], xm0, 1
    lea             r2, [r2 + r3 * 4]
    pextrd          [r2], xm6, 2
    pextrd          [r2 + r3], xm0, 2
    pextrd          [r2 + r3 * 2], xm6, 3
    pextrd          [r2 + r6], xm0, 3
%else
    vextracti128    xm5, m6, 1
    vextracti128    xm3, m0, 1
    movq            [r2], xm6
    movq            [r2 + r3], xm5
    movhps          [r2 + r3 * 2], xm6
    movhps          [r2 + r6], xm5
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm0
    movq            [r2 + r3], xm3
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r6], xm3
%endif
%endmacro

%macro FILTER_VER_LUMA_AVX2_4x16 1
INIT_YMM avx2
cglobal interp_8tap_vert_%1_4x16, 4, 7, 8
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,sp
    mova            m7, [pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
    PROCESS_LUMA_AVX2_W4_16R %1
    RET
%endmacro

    FILTER_VER_LUMA_AVX2_4x16 sp
    FILTER_VER_LUMA_AVX2_4x16 ss

%macro FILTER_VER_LUMA_S_AVX2_8x8 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_8x8, 4, 6, 12
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4

%ifidn %1,sp
    mova            m11, [pd_526336]
%else
    add             r3d, r3d
%endif

    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    pmaddwd         m2, [r5]
    paddd           m0, m4
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    pmaddwd         m3, [r5]
    paddd           m1, m5
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 2 * mmsize]
    paddd           m0, m6
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 2 * mmsize]
    paddd           m1, m7
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m7
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 3 * mmsize]
    paddd           m0, m8
    pmaddwd         m8, m6, [r5 + 2 * mmsize]
    paddd           m2, m8
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    pmaddwd         m6, [r5]
    paddd           m4, m8
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 3 * mmsize]
    paddd           m1, m9
    pmaddwd         m9, m7, [r5 + 2 * mmsize]
    paddd           m3, m9
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    pmaddwd         m7, [r5]
    paddd           m5, m9
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 3 * mmsize]
    paddd           m2, m10
    pmaddwd         m10, m8, [r5 + 2 * mmsize]
    pmaddwd         m8, [r5 + 1 * mmsize]
    paddd           m4, m10
    paddd           m6, m8
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm8, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm8, 1
    pmaddwd         m8, m9, [r5 + 3 * mmsize]
    paddd           m3, m8
    pmaddwd         m8, m9, [r5 + 2 * mmsize]
    pmaddwd         m9, [r5 + 1 * mmsize]
    paddd           m5, m8
    paddd           m7, m9
    movu            xm8, [r0 + r4]                  ; m8 = row 11
    punpckhwd       xm9, xm10, xm8
    punpcklwd       xm10, xm8
    vinserti128     m10, m10, xm9, 1
    pmaddwd         m9, m10, [r5 + 3 * mmsize]
    pmaddwd         m10, [r5 + 2 * mmsize]
    paddd           m4, m9
    paddd           m6, m10

    lea             r4, [r3 * 3]
%ifidn %1,sp
    paddd           m0, m11
    paddd           m1, m11
    paddd           m2, m11
    paddd           m3, m11
    psrad           m0, 12
    psrad           m1, 12
    psrad           m2, 12
    psrad           m3, 12
%else
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
%endif
    packssdw        m0, m1
    packssdw        m2, m3
%ifidn %1,sp
    packuswb        m0, m2
    mova            m1, [interp8_hps_shuf]
    vpermd          m0, m1, m0
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r4], xm2
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r4], xm3
%endif

    lea             r0, [r0 + r1 * 4]
    movu            xm9, [r0]                       ; m9 = row 12
    punpckhwd       xm3, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm3, 1
    pmaddwd         m3, m8, [r5 + 3 * mmsize]
    pmaddwd         m8, [r5 + 2 * mmsize]
    paddd           m5, m3
    paddd           m7, m8
    movu            xm3, [r0 + r1]                  ; m3 = row 13
    punpckhwd       xm0, xm9, xm3
    punpcklwd       xm9, xm3
    vinserti128     m9, m9, xm0, 1
    pmaddwd         m9, [r5 + 3 * mmsize]
    paddd           m6, m9
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhwd       xm9, xm3, xm0
    punpcklwd       xm3, xm0
    vinserti128     m3, m3, xm9, 1
    pmaddwd         m3, [r5 + 3 * mmsize]
    paddd           m7, m3

%ifidn %1,sp
    paddd           m4, m11
    paddd           m5, m11
    paddd           m6, m11
    paddd           m7, m11
    psrad           m4, 12
    psrad           m5, 12
    psrad           m6, 12
    psrad           m7, 12
%else
    psrad           m4, 6
    psrad           m5, 6
    psrad           m6, 6
    psrad           m7, 6
%endif
    packssdw        m4, m5
    packssdw        m6, m7
    lea             r2, [r2 + r3 * 4]
%ifidn %1,sp
    packuswb        m4, m6
    vpermd          m4, m1, m4
    vextracti128    xm6, m4, 1
    movq            [r2], xm4
    movhps          [r2 + r3], xm4
    movq            [r2 + r3 * 2], xm6
    movhps          [r2 + r4], xm6
%else
    vpermq          m4, m4, 11011000b
    vpermq          m6, m6, 11011000b
    vextracti128    xm5, m4, 1
    vextracti128    xm7, m6, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm5
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r4], xm7
%endif
    RET
%endif
%endmacro

    FILTER_VER_LUMA_S_AVX2_8x8 sp
    FILTER_VER_LUMA_S_AVX2_8x8 ss

%macro FILTER_VER_LUMA_S_AVX2_8xN 2
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_8x%2, 4, 9, 15
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,sp
    mova            m14, [pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
    lea             r7, [r1 * 4]
    mov             r8d, %2 / 16
.loopH:
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    paddd           m1, m5
    pmaddwd         m3, [r5]
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 2 * mmsize]
    paddd           m0, m6
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 2 * mmsize]
    paddd           m1, m7
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    paddd           m3, m7
    pmaddwd         m5, [r5]
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 3 * mmsize]
    paddd           m0, m8
    pmaddwd         m8, m6, [r5 + 2 * mmsize]
    paddd           m2, m8
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    paddd           m4, m8
    pmaddwd         m6, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 3 * mmsize]
    paddd           m1, m9
    pmaddwd         m9, m7, [r5 + 2 * mmsize]
    paddd           m3, m9
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    paddd           m5, m9
    pmaddwd         m7, [r5]
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 3 * mmsize]
    paddd           m2, m10
    pmaddwd         m10, m8, [r5 + 2 * mmsize]
    paddd           m4, m10
    pmaddwd         m10, m8, [r5 + 1 * mmsize]
    paddd           m6, m10
    pmaddwd         m8, [r5]
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm11, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddwd         m11, m9, [r5 + 3 * mmsize]
    paddd           m3, m11
    pmaddwd         m11, m9, [r5 + 2 * mmsize]
    paddd           m5, m11
    pmaddwd         m11, m9, [r5 + 1 * mmsize]
    paddd           m7, m11
    pmaddwd         m9, [r5]
    movu            xm11, [r0 + r4]                 ; m11 = row 11
    punpckhwd       xm12, xm10, xm11
    punpcklwd       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddwd         m12, m10, [r5 + 3 * mmsize]
    paddd           m4, m12
    pmaddwd         m12, m10, [r5 + 2 * mmsize]
    paddd           m6, m12
    pmaddwd         m12, m10, [r5 + 1 * mmsize]
    paddd           m8, m12
    pmaddwd         m10, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm12, [r0]                      ; m12 = row 12
    punpckhwd       xm13, xm11, xm12
    punpcklwd       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddwd         m13, m11, [r5 + 3 * mmsize]
    paddd           m5, m13
    pmaddwd         m13, m11, [r5 + 2 * mmsize]
    paddd           m7, m13
    pmaddwd         m13, m11, [r5 + 1 * mmsize]
    paddd           m9, m13
    pmaddwd         m11, [r5]

%ifidn %1,sp
    paddd           m0, m14
    paddd           m1, m14
    paddd           m2, m14
    paddd           m3, m14
    paddd           m4, m14
    paddd           m5, m14
    psrad           m0, 12
    psrad           m1, 12
    psrad           m2, 12
    psrad           m3, 12
    psrad           m4, 12
    psrad           m5, 12
%else
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
    psrad           m4, 6
    psrad           m5, 6
%endif
    packssdw        m0, m1
    packssdw        m2, m3
    packssdw        m4, m5
%ifidn %1,sp
    packuswb        m0, m2
    mova            m1, [interp8_hps_shuf]
    vpermd          m0, m1, m0
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r6], xm2
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
%endif

    movu            xm13, [r0 + r1]                 ; m13 = row 13
    punpckhwd       xm0, xm12, xm13
    punpcklwd       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddwd         m0, m12, [r5 + 3 * mmsize]
    paddd           m6, m0
    pmaddwd         m0, m12, [r5 + 2 * mmsize]
    paddd           m8, m0
    pmaddwd         m0, m12, [r5 + 1 * mmsize]
    paddd           m10, m0
    pmaddwd         m12, [r5]
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhwd       xm2, xm13, xm0
    punpcklwd       xm13, xm0
    vinserti128     m13, m13, xm2, 1
    pmaddwd         m2, m13, [r5 + 3 * mmsize]
    paddd           m7, m2
    pmaddwd         m2, m13, [r5 + 2 * mmsize]
    paddd           m9, m2
    pmaddwd         m2, m13, [r5 + 1 * mmsize]
    paddd           m11, m2
    pmaddwd         m13, [r5]

%ifidn %1,sp
    paddd           m6, m14
    paddd           m7, m14
    psrad           m6, 12
    psrad           m7, 12
%else
    psrad           m6, 6
    psrad           m7, 6
%endif
    packssdw        m6, m7
    lea             r2, [r2 + r3 * 4]

%ifidn %1,sp
    packuswb        m4, m6
    vpermd          m4, m1, m4
    vextracti128    xm6, m4, 1
    movq            [r2], xm4
    movhps          [r2 + r3], xm4
    movq            [r2 + r3 * 2], xm6
    movhps          [r2 + r6], xm6
%else
    vpermq          m6, m6, 11011000b
    vpermq          m4, m4, 11011000b
    vextracti128    xm1, m4, 1
    vextracti128    xm7, m6, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r6], xm7
%endif

    movu            xm6, [r0 + r4]                  ; m6 = row 15
    punpckhwd       xm5, xm0, xm6
    punpcklwd       xm0, xm6
    vinserti128     m0, m0, xm5, 1
    pmaddwd         m5, m0, [r5 + 3 * mmsize]
    paddd           m8, m5
    pmaddwd         m5, m0, [r5 + 2 * mmsize]
    paddd           m10, m5
    pmaddwd         m5, m0, [r5 + 1 * mmsize]
    paddd           m12, m5
    pmaddwd         m0, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm2, [r0]                       ; m2 = row 16
    punpckhwd       xm3, xm6, xm2
    punpcklwd       xm6, xm2
    vinserti128     m6, m6, xm3, 1
    pmaddwd         m3, m6, [r5 + 3 * mmsize]
    paddd           m9, m3
    pmaddwd         m3, m6, [r5 + 2 * mmsize]
    paddd           m11, m3
    pmaddwd         m3, m6, [r5 + 1 * mmsize]
    paddd           m13, m3
    pmaddwd         m6, [r5]
    movu            xm3, [r0 + r1]                  ; m3 = row 17
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 3 * mmsize]
    paddd           m10, m4
    pmaddwd         m4, m2, [r5 + 2 * mmsize]
    paddd           m12, m4
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m0, m2
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpckhwd       xm2, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm2, 1
    pmaddwd         m2, m3, [r5 + 3 * mmsize]
    paddd           m11, m2
    pmaddwd         m2, m3, [r5 + 2 * mmsize]
    paddd           m13, m2
    pmaddwd         m3, [r5 + 1 * mmsize]
    paddd           m6, m3
    movu            xm2, [r0 + r4]                  ; m2 = row 19
    punpckhwd       xm7, xm4, xm2
    punpcklwd       xm4, xm2
    vinserti128     m4, m4, xm7, 1
    pmaddwd         m7, m4, [r5 + 3 * mmsize]
    paddd           m12, m7
    pmaddwd         m4, [r5 + 2 * mmsize]
    paddd           m0, m4
    lea             r0, [r0 + r1 * 4]
    movu            xm7, [r0]                       ; m7 = row 20
    punpckhwd       xm3, xm2, xm7
    punpcklwd       xm2, xm7
    vinserti128     m2, m2, xm3, 1
    pmaddwd         m3, m2, [r5 + 3 * mmsize]
    paddd           m13, m3
    pmaddwd         m2, [r5 + 2 * mmsize]
    paddd           m6, m2
    movu            xm3, [r0 + r1]                  ; m3 = row 21
    punpckhwd       xm2, xm7, xm3
    punpcklwd       xm7, xm3
    vinserti128     m7, m7, xm2, 1
    pmaddwd         m7, [r5 + 3 * mmsize]
    paddd           m0, m7
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 22
    punpckhwd       xm7, xm3, xm2
    punpcklwd       xm3, xm2
    vinserti128     m3, m3, xm7, 1
    pmaddwd         m3, [r5 + 3 * mmsize]
    paddd           m6, m3

%ifidn %1,sp
    paddd           m8, m14
    paddd           m9, m14
    paddd           m10, m14
    paddd           m11, m14
    paddd           m12, m14
    paddd           m13, m14
    paddd           m0, m14
    paddd           m6, m14
    psrad           m8, 12
    psrad           m9, 12
    psrad           m10, 12
    psrad           m11, 12
    psrad           m12, 12
    psrad           m13, 12
    psrad           m0, 12
    psrad           m6, 12
%else
    psrad           m8, 6
    psrad           m9, 6
    psrad           m10, 6
    psrad           m11, 6
    psrad           m12, 6
    psrad           m13, 6
    psrad           m0, 6
    psrad           m6, 6
%endif
    packssdw        m8, m9
    packssdw        m10, m11
    packssdw        m12, m13
    packssdw        m0, m6
    lea             r2, [r2 + r3 * 4]

%ifidn %1,sp
    packuswb        m8, m10
    packuswb        m12, m0
    vpermd          m8, m1, m8
    vpermd          m12, m1, m12
    vextracti128    xm10, m8, 1
    vextracti128    xm0, m12, 1
    movq            [r2], xm8
    movhps          [r2 + r3], xm8
    movq            [r2 + r3 * 2], xm10
    movhps          [r2 + r6], xm10
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm12
    movhps          [r2 + r3], xm12
    movq            [r2 + r3 * 2], xm0
    movhps          [r2 + r6], xm0
%else
    vpermq          m8, m8, 11011000b
    vpermq          m10, m10, 11011000b
    vpermq          m12, m12, 11011000b
    vpermq          m0, m0, 11011000b
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    vextracti128    xm13, m12, 1
    vextracti128    xm6, m0, 1
    movu            [r2], xm8
    movu            [r2 + r3], xm9
    movu            [r2 + r3 * 2], xm10
    movu            [r2 + r6], xm11
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm12
    movu            [r2 + r3], xm13
    movu            [r2 + r3 * 2], xm0
    movu            [r2 + r6], xm6
%endif

    lea             r2, [r2 + r3 * 4]
    sub             r0, r7
    dec             r8d
    jnz             .loopH
    RET
%endif
%endmacro

    FILTER_VER_LUMA_S_AVX2_8xN sp, 16
    FILTER_VER_LUMA_S_AVX2_8xN sp, 32
    FILTER_VER_LUMA_S_AVX2_8xN ss, 16
    FILTER_VER_LUMA_S_AVX2_8xN ss, 32

%macro PROCESS_LUMA_S_AVX2_W8_4R 1
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    paddd           m1, m5
    pmaddwd         m3, [r5]
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 2 * mmsize]
    paddd           m0, m6
    pmaddwd         m4, [r5 + 1 * mmsize]
    paddd           m2, m4
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm4, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm4, 1
    pmaddwd         m4, m5, [r5 + 2 * mmsize]
    paddd           m1, m4
    pmaddwd         m5, [r5 + 1 * mmsize]
    paddd           m3, m5
    movu            xm4, [r0 + r4]                  ; m4 = row 7
    punpckhwd       xm5, xm6, xm4
    punpcklwd       xm6, xm4
    vinserti128     m6, m6, xm5, 1
    pmaddwd         m5, m6, [r5 + 3 * mmsize]
    paddd           m0, m5
    pmaddwd         m6, [r5 + 2 * mmsize]
    paddd           m2, m6
    lea             r0, [r0 + r1 * 4]
    movu            xm5, [r0]                       ; m5 = row 8
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 3 * mmsize]
    paddd           m1, m6
    pmaddwd         m4, [r5 + 2 * mmsize]
    paddd           m3, m4
    movu            xm6, [r0 + r1]                  ; m6 = row 9
    punpckhwd       xm4, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm4, 1
    pmaddwd         m5, [r5 + 3 * mmsize]
    paddd           m2, m5
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 10
    punpckhwd       xm5, xm6, xm4
    punpcklwd       xm6, xm4
    vinserti128     m6, m6, xm5, 1
    pmaddwd         m6, [r5 + 3 * mmsize]
    paddd           m3, m6

%ifidn %1,sp
    paddd           m0, m7
    paddd           m1, m7
    paddd           m2, m7
    paddd           m3, m7
    psrad           m0, 12
    psrad           m1, 12
    psrad           m2, 12
    psrad           m3, 12
%else
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
%endif
    packssdw        m0, m1
    packssdw        m2, m3
%ifidn %1,sp
    packuswb        m0, m2
    mova            m4, [interp8_hps_shuf]
    vpermd          m0, m4, m0
    vextracti128    xm2, m0, 1
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
%endif
%endmacro

%macro FILTER_VER_LUMA_S_AVX2_8x4 1
INIT_YMM avx2
cglobal interp_8tap_vert_%1_8x4, 4, 6, 8
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,sp
    mova            m7, [pd_526336]
%else
    add             r3d, r3d
%endif

    PROCESS_LUMA_S_AVX2_W8_4R %1
    lea             r4, [r3 * 3]
%ifidn %1,sp
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r4], xm2
%else
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r4], xm3
%endif
    RET
%endmacro

    FILTER_VER_LUMA_S_AVX2_8x4 sp
    FILTER_VER_LUMA_S_AVX2_8x4 ss

%macro PROCESS_LUMA_AVX2_W8_16R 1
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m2, [r5]
    lea             r7, [r0 + r1 * 4]
    movu            xm4, [r7]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    paddd           m1, m5
    pmaddwd         m3, [r5]
    movu            xm5, [r7 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 2 * mmsize]
    paddd           m0, m6
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r7 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 2 * mmsize]
    paddd           m1, m7
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    paddd           m3, m7
    pmaddwd         m5, [r5]
    movu            xm7, [r7 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 3 * mmsize]
    paddd           m0, m8
    pmaddwd         m8, m6, [r5 + 2 * mmsize]
    paddd           m2, m8
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    paddd           m4, m8
    pmaddwd         m6, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm8, [r7]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 3 * mmsize]
    paddd           m1, m9
    pmaddwd         m9, m7, [r5 + 2 * mmsize]
    paddd           m3, m9
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    paddd           m5, m9
    pmaddwd         m7, [r5]
    movu            xm9, [r7 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 3 * mmsize]
    paddd           m2, m10
    pmaddwd         m10, m8, [r5 + 2 * mmsize]
    paddd           m4, m10
    pmaddwd         m10, m8, [r5 + 1 * mmsize]
    paddd           m6, m10
    pmaddwd         m8, [r5]
    movu            xm10, [r7 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm11, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddwd         m11, m9, [r5 + 3 * mmsize]
    paddd           m3, m11
    pmaddwd         m11, m9, [r5 + 2 * mmsize]
    paddd           m5, m11
    pmaddwd         m11, m9, [r5 + 1 * mmsize]
    paddd           m7, m11
    pmaddwd         m9, [r5]
    movu            xm11, [r7 + r4]                 ; m11 = row 11
    punpckhwd       xm12, xm10, xm11
    punpcklwd       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddwd         m12, m10, [r5 + 3 * mmsize]
    paddd           m4, m12
    pmaddwd         m12, m10, [r5 + 2 * mmsize]
    paddd           m6, m12
    pmaddwd         m12, m10, [r5 + 1 * mmsize]
    paddd           m8, m12
    pmaddwd         m10, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm12, [r7]                      ; m12 = row 12
    punpckhwd       xm13, xm11, xm12
    punpcklwd       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddwd         m13, m11, [r5 + 3 * mmsize]
    paddd           m5, m13
    pmaddwd         m13, m11, [r5 + 2 * mmsize]
    paddd           m7, m13
    pmaddwd         m13, m11, [r5 + 1 * mmsize]
    paddd           m9, m13
    pmaddwd         m11, [r5]

%ifidn %1,sp
    paddd           m0, m14
    paddd           m1, m14
    paddd           m2, m14
    paddd           m3, m14
    paddd           m4, m14
    paddd           m5, m14
    psrad           m0, 12
    psrad           m1, 12
    psrad           m2, 12
    psrad           m3, 12
    psrad           m4, 12
    psrad           m5, 12
%else
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
    psrad           m4, 6
    psrad           m5, 6
%endif
    packssdw        m0, m1
    packssdw        m2, m3
    packssdw        m4, m5
%ifidn %1,sp
    packuswb        m0, m2
    mova            m5, [interp8_hps_shuf]
    vpermd          m0, m5, m0
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r6], xm2
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
%endif

    movu            xm13, [r7 + r1]                 ; m13 = row 13
    punpckhwd       xm0, xm12, xm13
    punpcklwd       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddwd         m0, m12, [r5 + 3 * mmsize]
    paddd           m6, m0
    pmaddwd         m0, m12, [r5 + 2 * mmsize]
    paddd           m8, m0
    pmaddwd         m0, m12, [r5 + 1 * mmsize]
    paddd           m10, m0
    pmaddwd         m12, [r5]
    movu            xm0, [r7 + r1 * 2]              ; m0 = row 14
    punpckhwd       xm1, xm13, xm0
    punpcklwd       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddwd         m1, m13, [r5 + 3 * mmsize]
    paddd           m7, m1
    pmaddwd         m1, m13, [r5 + 2 * mmsize]
    paddd           m9, m1
    pmaddwd         m1, m13, [r5 + 1 * mmsize]
    paddd           m11, m1
    pmaddwd         m13, [r5]

%ifidn %1,sp
    paddd           m6, m14
    paddd           m7, m14
    psrad           m6, 12
    psrad           m7, 12
%else
    psrad           m6, 6
    psrad           m7, 6
%endif
    packssdw        m6, m7
    lea             r8, [r2 + r3 * 4]

%ifidn %1,sp
    packuswb        m4, m6
    vpermd          m4, m5, m4
    vextracti128    xm6, m4, 1
    movq            [r8], xm4
    movhps          [r8 + r3], xm4
    movq            [r8 + r3 * 2], xm6
    movhps          [r8 + r6], xm6
%else
    vpermq          m4, m4, 11011000b
    vpermq          m6, m6, 11011000b
    vextracti128    xm1, m4, 1
    vextracti128    xm7, m6, 1
    movu            [r8], xm4
    movu            [r8 + r3], xm1
    movu            [r8 + r3 * 2], xm6
    movu            [r8 + r6], xm7
%endif

    movu            xm1, [r7 + r4]                  ; m1 = row 15
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m2, m0, [r5 + 3 * mmsize]
    paddd           m8, m2
    pmaddwd         m2, m0, [r5 + 2 * mmsize]
    paddd           m10, m2
    pmaddwd         m2, m0, [r5 + 1 * mmsize]
    paddd           m12, m2
    pmaddwd         m0, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm2, [r7]                       ; m2 = row 16
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m3, m1, [r5 + 3 * mmsize]
    paddd           m9, m3
    pmaddwd         m3, m1, [r5 + 2 * mmsize]
    paddd           m11, m3
    pmaddwd         m3, m1, [r5 + 1 * mmsize]
    paddd           m13, m3
    pmaddwd         m1, [r5]
    movu            xm3, [r7 + r1]                  ; m3 = row 17
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 3 * mmsize]
    paddd           m10, m4
    pmaddwd         m4, m2, [r5 + 2 * mmsize]
    paddd           m12, m4
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m0, m2
    movu            xm4, [r7 + r1 * 2]              ; m4 = row 18
    punpckhwd       xm2, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm2, 1
    pmaddwd         m2, m3, [r5 + 3 * mmsize]
    paddd           m11, m2
    pmaddwd         m2, m3, [r5 + 2 * mmsize]
    paddd           m13, m2
    pmaddwd         m3, [r5 + 1 * mmsize]
    paddd           m1, m3
    movu            xm2, [r7 + r4]                  ; m2 = row 19
    punpckhwd       xm6, xm4, xm2
    punpcklwd       xm4, xm2
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 3 * mmsize]
    paddd           m12, m6
    pmaddwd         m4, [r5 + 2 * mmsize]
    paddd           m0, m4
    lea             r7, [r7 + r1 * 4]
    movu            xm6, [r7]                       ; m6 = row 20
    punpckhwd       xm7, xm2, xm6
    punpcklwd       xm2, xm6
    vinserti128     m2, m2, xm7, 1
    pmaddwd         m7, m2, [r5 + 3 * mmsize]
    paddd           m13, m7
    pmaddwd         m2, [r5 + 2 * mmsize]
    paddd           m1, m2
    movu            xm7, [r7 + r1]                  ; m7 = row 21
    punpckhwd       xm2, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm2, 1
    pmaddwd         m6, [r5 + 3 * mmsize]
    paddd           m0, m6
    movu            xm2, [r7 + r1 * 2]              ; m2 = row 22
    punpckhwd       xm3, xm7, xm2
    punpcklwd       xm7, xm2
    vinserti128     m7, m7, xm3, 1
    pmaddwd         m7, [r5 + 3 * mmsize]
    paddd           m1, m7

%ifidn %1,sp
    paddd           m8, m14
    paddd           m9, m14
    paddd           m10, m14
    paddd           m11, m14
    paddd           m12, m14
    paddd           m13, m14
    paddd           m0, m14
    paddd           m1, m14
    psrad           m8, 12
    psrad           m9, 12
    psrad           m10, 12
    psrad           m11, 12
    psrad           m12, 12
    psrad           m13, 12
    psrad           m0, 12
    psrad           m1, 12
%else
    psrad           m8, 6
    psrad           m9, 6
    psrad           m10, 6
    psrad           m11, 6
    psrad           m12, 6
    psrad           m13, 6
    psrad           m0, 6
    psrad           m1, 6
%endif
    packssdw        m8, m9
    packssdw        m10, m11
    packssdw        m12, m13
    packssdw        m0, m1
    lea             r8, [r8 + r3 * 4]

%ifidn %1,sp
    packuswb        m8, m10
    packuswb        m12, m0
    vpermd          m8, m5, m8
    vpermd          m12, m5, m12
    vextracti128    xm10, m8, 1
    vextracti128    xm0, m12, 1
    movq            [r8], xm8
    movhps          [r8 + r3], xm8
    movq            [r8 + r3 * 2], xm10
    movhps          [r8 + r6], xm10
    lea             r8, [r8 + r3 * 4]
    movq            [r8], xm12
    movhps          [r8 + r3], xm12
    movq            [r8 + r3 * 2], xm0
    movhps          [r8 + r6], xm0
%else
    vpermq          m8, m8, 11011000b
    vpermq          m10, m10, 11011000b
    vpermq          m12, m12, 11011000b
    vpermq          m0, m0, 11011000b
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    vextracti128    xm13, m12, 1
    vextracti128    xm1, m0, 1
    movu            [r8], xm8
    movu            [r8 + r3], xm9
    movu            [r8 + r3 * 2], xm10
    movu            [r8 + r6], xm11
    lea             r8, [r8 + r3 * 4]
    movu            [r8], xm12
    movu            [r8 + r3], xm13
    movu            [r8 + r3 * 2], xm0
    movu            [r8 + r6], xm1
%endif
%endmacro

%macro FILTER_VER_LUMA_AVX2_Nx16 2
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_%2x16, 4, 10, 15
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,sp
    mova            m14, [pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
    mov             r9d, %2 / 8
.loopW:
    PROCESS_LUMA_AVX2_W8_16R %1
%ifidn %1,sp
    add             r2, 8
%else
    add             r2, 16
%endif
    add             r0, 16
    dec             r9d
    jnz             .loopW
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_Nx16 sp, 16
    FILTER_VER_LUMA_AVX2_Nx16 sp, 32
    FILTER_VER_LUMA_AVX2_Nx16 sp, 64
    FILTER_VER_LUMA_AVX2_Nx16 ss, 16
    FILTER_VER_LUMA_AVX2_Nx16 ss, 32
    FILTER_VER_LUMA_AVX2_Nx16 ss, 64

%macro FILTER_VER_LUMA_AVX2_NxN 3
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%3_%1x%2, 4, 12, 15
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4

%ifidn %3,sp
    mova            m14, [pd_526336]
%else
    add             r3d, r3d
%endif

    lea             r6, [r3 * 3]
    lea             r11, [r1 * 4]
    mov             r9d, %2 / 16
.loopH:
    mov             r10d, %1 / 8
.loopW:
    PROCESS_LUMA_AVX2_W8_16R %3
%ifidn %3,sp
    add             r2, 8
%else
    add             r2, 16
%endif
    add             r0, 16
    dec             r10d
    jnz             .loopW
    sub             r7, r11
    lea             r0, [r7 - 2 * %1 + 16]
%ifidn %3,sp
    lea             r2, [r8 + r3 * 4 - %1 + 8]
%else
    lea             r2, [r8 + r3 * 4 - 2 * %1 + 16]
%endif
    dec             r9d
    jnz             .loopH
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_NxN 16, 32, sp
    FILTER_VER_LUMA_AVX2_NxN 16, 64, sp
    FILTER_VER_LUMA_AVX2_NxN 24, 32, sp
    FILTER_VER_LUMA_AVX2_NxN 32, 32, sp
    FILTER_VER_LUMA_AVX2_NxN 32, 64, sp
    FILTER_VER_LUMA_AVX2_NxN 48, 64, sp
    FILTER_VER_LUMA_AVX2_NxN 64, 32, sp
    FILTER_VER_LUMA_AVX2_NxN 64, 48, sp
    FILTER_VER_LUMA_AVX2_NxN 64, 64, sp
    FILTER_VER_LUMA_AVX2_NxN 16, 32, ss
    FILTER_VER_LUMA_AVX2_NxN 16, 64, ss
    FILTER_VER_LUMA_AVX2_NxN 24, 32, ss
    FILTER_VER_LUMA_AVX2_NxN 32, 32, ss
    FILTER_VER_LUMA_AVX2_NxN 32, 64, ss
    FILTER_VER_LUMA_AVX2_NxN 48, 64, ss
    FILTER_VER_LUMA_AVX2_NxN 64, 32, ss
    FILTER_VER_LUMA_AVX2_NxN 64, 48, ss
    FILTER_VER_LUMA_AVX2_NxN 64, 64, ss

%macro FILTER_VER_LUMA_S_AVX2_12x16 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_12x16, 4, 9, 15
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,sp
    mova            m14, [pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
    PROCESS_LUMA_AVX2_W8_16R %1
%ifidn %1,sp
    add             r2, 8
%else
    add             r2, 16
%endif
    add             r0, 16
    mova            m7, m14
    PROCESS_LUMA_AVX2_W4_16R %1
    RET
%endif
%endmacro

    FILTER_VER_LUMA_S_AVX2_12x16 sp
    FILTER_VER_LUMA_S_AVX2_12x16 ss

%macro FILTER_VER_LUMA_S_AVX2_16x12 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_16x12, 4, 10, 15
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,sp
    mova            m14, [pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
    mov             r9d, 2
.loopW:
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m2, [r5]
    lea             r7, [r0 + r1 * 4]
    movu            xm4, [r7]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    paddd           m1, m5
    pmaddwd         m3, [r5]
    movu            xm5, [r7 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 2 * mmsize]
    paddd           m0, m6
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r7 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 2 * mmsize]
    paddd           m1, m7
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    paddd           m3, m7
    pmaddwd         m5, [r5]
    movu            xm7, [r7 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 3 * mmsize]
    paddd           m0, m8
    pmaddwd         m8, m6, [r5 + 2 * mmsize]
    paddd           m2, m8
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    paddd           m4, m8
    pmaddwd         m6, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm8, [r7]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 3 * mmsize]
    paddd           m1, m9
    pmaddwd         m9, m7, [r5 + 2 * mmsize]
    paddd           m3, m9
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    paddd           m5, m9
    pmaddwd         m7, [r5]
    movu            xm9, [r7 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 3 * mmsize]
    paddd           m2, m10
    pmaddwd         m10, m8, [r5 + 2 * mmsize]
    paddd           m4, m10
    pmaddwd         m10, m8, [r5 + 1 * mmsize]
    paddd           m6, m10
    pmaddwd         m8, [r5]
    movu            xm10, [r7 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm11, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddwd         m11, m9, [r5 + 3 * mmsize]
    paddd           m3, m11
    pmaddwd         m11, m9, [r5 + 2 * mmsize]
    paddd           m5, m11
    pmaddwd         m11, m9, [r5 + 1 * mmsize]
    paddd           m7, m11
    pmaddwd         m9, [r5]
    movu            xm11, [r7 + r4]                 ; m11 = row 11
    punpckhwd       xm12, xm10, xm11
    punpcklwd       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddwd         m12, m10, [r5 + 3 * mmsize]
    paddd           m4, m12
    pmaddwd         m12, m10, [r5 + 2 * mmsize]
    paddd           m6, m12
    pmaddwd         m12, m10, [r5 + 1 * mmsize]
    paddd           m8, m12
    pmaddwd         m10, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm12, [r7]                      ; m12 = row 12
    punpckhwd       xm13, xm11, xm12
    punpcklwd       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddwd         m13, m11, [r5 + 3 * mmsize]
    paddd           m5, m13
    pmaddwd         m13, m11, [r5 + 2 * mmsize]
    paddd           m7, m13
    pmaddwd         m13, m11, [r5 + 1 * mmsize]
    paddd           m9, m13
    pmaddwd         m11, [r5]

%ifidn %1,sp
    paddd           m0, m14
    paddd           m1, m14
    paddd           m2, m14
    paddd           m3, m14
    paddd           m4, m14
    paddd           m5, m14
    psrad           m0, 12
    psrad           m1, 12
    psrad           m2, 12
    psrad           m3, 12
    psrad           m4, 12
    psrad           m5, 12
%else
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
    psrad           m4, 6
    psrad           m5, 6
%endif
    packssdw        m0, m1
    packssdw        m2, m3
    packssdw        m4, m5

%ifidn %1,sp
    packuswb        m0, m2
    mova            m5, [interp8_hps_shuf]
    vpermd          m0, m5, m0
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r6], xm2
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
%endif

    movu            xm13, [r7 + r1]                 ; m13 = row 13
    punpckhwd       xm0, xm12, xm13
    punpcklwd       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddwd         m0, m12, [r5 + 3 * mmsize]
    paddd           m6, m0
    pmaddwd         m0, m12, [r5 + 2 * mmsize]
    paddd           m8, m0
    pmaddwd         m12, [r5 + 1 * mmsize]
    paddd           m10, m12
    movu            xm0, [r7 + r1 * 2]              ; m0 = row 14
    punpckhwd       xm1, xm13, xm0
    punpcklwd       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddwd         m1, m13, [r5 + 3 * mmsize]
    paddd           m7, m1
    pmaddwd         m1, m13, [r5 + 2 * mmsize]
    paddd           m9, m1
    pmaddwd         m13, [r5 + 1 * mmsize]
    paddd           m11, m13

%ifidn %1,sp
    paddd           m6, m14
    paddd           m7, m14
    psrad           m6, 12
    psrad           m7, 12
%else
    psrad           m6, 6
    psrad           m7, 6
%endif
    packssdw        m6, m7
    lea             r8, [r2 + r3 * 4]

%ifidn %1,sp
    packuswb        m4, m6
    vpermd          m4, m5, m4
    vextracti128    xm6, m4, 1
    movq            [r8], xm4
    movhps          [r8 + r3], xm4
    movq            [r8 + r3 * 2], xm6
    movhps          [r8 + r6], xm6
%else
    vpermq          m4, m4, 11011000b
    vpermq          m6, m6, 11011000b
    vextracti128    xm1, m4, 1
    vextracti128    xm7, m6, 1
    movu            [r8], xm4
    movu            [r8 + r3], xm1
    movu            [r8 + r3 * 2], xm6
    movu            [r8 + r6], xm7
%endif

    movu            xm1, [r7 + r4]                  ; m1 = row 15
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m2, m0, [r5 + 3 * mmsize]
    paddd           m8, m2
    pmaddwd         m0, [r5 + 2 * mmsize]
    paddd           m10, m0
    lea             r7, [r7 + r1 * 4]
    movu            xm2, [r7]                       ; m2 = row 16
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m3, m1, [r5 + 3 * mmsize]
    paddd           m9, m3
    pmaddwd         m1, [r5 + 2 * mmsize]
    paddd           m11, m1
    movu            xm3, [r7 + r1]                  ; m3 = row 17
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m2, [r5 + 3 * mmsize]
    paddd           m10, m2
    movu            xm4, [r7 + r1 * 2]              ; m4 = row 18
    punpckhwd       xm2, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm2, 1
    pmaddwd         m3, [r5 + 3 * mmsize]
    paddd           m11, m3

%ifidn %1,sp
    paddd           m8, m14
    paddd           m9, m14
    paddd           m10, m14
    paddd           m11, m14
    psrad           m8, 12
    psrad           m9, 12
    psrad           m10, 12
    psrad           m11, 12
%else
    psrad           m8, 6
    psrad           m9, 6
    psrad           m10, 6
    psrad           m11, 6
%endif
    packssdw        m8, m9
    packssdw        m10, m11
    lea             r8, [r8 + r3 * 4]

%ifidn %1,sp
    packuswb        m8, m10
    vpermd          m8, m5, m8
    vextracti128    xm10, m8, 1
    movq            [r8], xm8
    movhps          [r8 + r3], xm8
    movq            [r8 + r3 * 2], xm10
    movhps          [r8 + r6], xm10
    add             r2, 8
%else
    vpermq          m8, m8, 11011000b
    vpermq          m10, m10, 11011000b
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    movu            [r8], xm8
    movu            [r8 + r3], xm9
    movu            [r8 + r3 * 2], xm10
    movu            [r8 + r6], xm11
    add             r2, 16
%endif
    add             r0, 16
    dec             r9d
    jnz             .loopW
    RET
%endif
%endmacro

    FILTER_VER_LUMA_S_AVX2_16x12 sp
    FILTER_VER_LUMA_S_AVX2_16x12 ss

%macro FILTER_VER_LUMA_S_AVX2_16x4 1
INIT_YMM avx2
cglobal interp_8tap_vert_%1_16x4, 4, 7, 8, 0 - gprsize
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,sp
    mova            m7, [pd_526336]
%else
    add             r3d, r3d
%endif
    mov             dword [rsp], 2
.loopW:
    PROCESS_LUMA_S_AVX2_W8_4R %1
    lea             r6, [r3 * 3]
%ifidn %1,sp
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r6], xm2
    add             r2, 8
%else
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
    add             r2, 16
%endif
    lea             r6, [8 * r1 - 16]
    sub             r0, r6
    dec             dword [rsp]
    jnz             .loopW
    RET
%endmacro

    FILTER_VER_LUMA_S_AVX2_16x4 sp
    FILTER_VER_LUMA_S_AVX2_16x4 ss

%macro PROCESS_LUMA_S_AVX2_W8_8R 1
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m2, [r5]
    lea             r7, [r0 + r1 * 4]
    movu            xm4, [r7]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    paddd           m1, m5
    pmaddwd         m3, [r5]
    movu            xm5, [r7 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 2 * mmsize]
    paddd           m0, m6
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r7 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 2 * mmsize]
    paddd           m1, m7
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    paddd           m3, m7
    pmaddwd         m5, [r5]
    movu            xm7, [r7 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 3 * mmsize]
    paddd           m0, m8
    pmaddwd         m8, m6, [r5 + 2 * mmsize]
    paddd           m2, m8
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    paddd           m4, m8
    pmaddwd         m6, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm8, [r7]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 3 * mmsize]
    paddd           m1, m9
    pmaddwd         m9, m7, [r5 + 2 * mmsize]
    paddd           m3, m9
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    paddd           m5, m9
    pmaddwd         m7, [r5]
    movu            xm9, [r7 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 3 * mmsize]
    paddd           m2, m10
    pmaddwd         m10, m8, [r5 + 2 * mmsize]
    paddd           m4, m10
    pmaddwd         m8, [r5 + 1 * mmsize]
    paddd           m6, m8
    movu            xm10, [r7 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm8, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm8, 1
    pmaddwd         m8, m9, [r5 + 3 * mmsize]
    paddd           m3, m8
    pmaddwd         m8, m9, [r5 + 2 * mmsize]
    paddd           m5, m8
    pmaddwd         m9, [r5 + 1 * mmsize]
    paddd           m7, m9
    movu            xm8, [r7 + r4]                  ; m8 = row 11
    punpckhwd       xm9, xm10, xm8
    punpcklwd       xm10, xm8
    vinserti128     m10, m10, xm9, 1
    pmaddwd         m9, m10, [r5 + 3 * mmsize]
    paddd           m4, m9
    pmaddwd         m10, [r5 + 2 * mmsize]
    paddd           m6, m10
    lea             r7, [r7 + r1 * 4]
    movu            xm9, [r7]                       ; m9 = row 12
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 3 * mmsize]
    paddd           m5, m10
    pmaddwd         m8, [r5 + 2 * mmsize]
    paddd           m7, m8

%ifidn %1,sp
    paddd           m0, m11
    paddd           m1, m11
    paddd           m2, m11
    paddd           m3, m11
    paddd           m4, m11
    paddd           m5, m11
    psrad           m0, 12
    psrad           m1, 12
    psrad           m2, 12
    psrad           m3, 12
    psrad           m4, 12
    psrad           m5, 12
%else
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
    psrad           m4, 6
    psrad           m5, 6
%endif
    packssdw        m0, m1
    packssdw        m2, m3
    packssdw        m4, m5

%ifidn %1,sp
    packuswb        m0, m2
    mova            m5, [interp8_hps_shuf]
    vpermd          m0, m5, m0
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r6], xm2
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
%endif

    movu            xm10, [r7 + r1]                 ; m10 = row 13
    punpckhwd       xm0, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm0, 1
    pmaddwd         m9, [r5 + 3 * mmsize]
    paddd           m6, m9
    movu            xm0, [r7 + r1 * 2]              ; m0 = row 14
    punpckhwd       xm1, xm10, xm0
    punpcklwd       xm10, xm0
    vinserti128     m10, m10, xm1, 1
    pmaddwd         m10, [r5 + 3 * mmsize]
    paddd           m7, m10

%ifidn %1,sp
    paddd           m6, m11
    paddd           m7, m11
    psrad           m6, 12
    psrad           m7, 12
%else
    psrad           m6, 6
    psrad           m7, 6
%endif
    packssdw        m6, m7
    lea             r8, [r2 + r3 * 4]

%ifidn %1,sp
    packuswb        m4, m6
    vpermd          m4, m5, m4
    vextracti128    xm6, m4, 1
    movq            [r8], xm4
    movhps          [r8 + r3], xm4
    movq            [r8 + r3 * 2], xm6
    movhps          [r8 + r6], xm6
%else
    vpermq          m4, m4, 11011000b
    vpermq          m6, m6, 11011000b
    vextracti128    xm5, m4, 1
    vextracti128    xm7, m6, 1
    movu            [r8], xm4
    movu            [r8 + r3], xm5
    movu            [r8 + r3 * 2], xm6
    movu            [r8 + r6], xm7
%endif
%endmacro

%macro FILTER_VER_LUMA_AVX2_Nx8 2
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_%2x8, 4, 10, 12
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,sp
    mova            m11, [pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
    mov             r9d, %2 / 8
.loopW:
    PROCESS_LUMA_S_AVX2_W8_8R %1
%ifidn %1,sp
    add             r2, 8
%else
    add             r2, 16
%endif
    add             r0, 16
    dec             r9d
    jnz             .loopW
    RET
%endif
%endmacro

    FILTER_VER_LUMA_AVX2_Nx8 sp, 32
    FILTER_VER_LUMA_AVX2_Nx8 sp, 16
    FILTER_VER_LUMA_AVX2_Nx8 ss, 32
    FILTER_VER_LUMA_AVX2_Nx8 ss, 16

%macro FILTER_VER_LUMA_S_AVX2_32x24 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_32x24, 4, 10, 15
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [pw_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,sp
    mova            m14, [pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
    mov             r9d, 4
.loopW:
    PROCESS_LUMA_AVX2_W8_16R %1
%ifidn %1,sp
    add             r2, 8
%else
    add             r2, 16
%endif
    add             r0, 16
    dec             r9d
    jnz             .loopW
    lea             r9, [r1 * 4]
    sub             r7, r9
    lea             r0, [r7 - 48]
%ifidn %1,sp
    lea             r2, [r8 + r3 * 4 - 24]
%else
    lea             r2, [r8 + r3 * 4 - 48]
%endif
    mova            m11, m14
    mov             r9d, 4
.loop:
    PROCESS_LUMA_S_AVX2_W8_8R %1
%ifidn %1,sp
    add             r2, 8
%else
    add             r2, 16
%endif
    add             r0, 16
    dec             r9d
    jnz             .loop
    RET
%endif
%endmacro

    FILTER_VER_LUMA_S_AVX2_32x24 sp
    FILTER_VER_LUMA_S_AVX2_32x24 ss
;-------------------------------------------------------------------------------------------------------------
;ipfilter_chroma_avx512 code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_IPFILTER_CHROMA_PP_64x1_AVX512 0
    ; register map
    ; m0 - interpolate coeff
    ; m1, m2 - shuffle order table
    ; m3 - constant word 1
    ; m4 - constant word 512

    movu               m5,           [r0]
    pshufb             m6,           m5,       m2
    pshufb             m5,           m5,       m1
    pmaddubsw          m5,           m0
    pmaddubsw          m6,           m0
    pmaddwd            m5,           m3
    pmaddwd            m6,           m3

    movu               m7,           [r0 + 4]
    pshufb             m8,           m7,       m2
    pshufb             m7,           m7,       m1
    pmaddubsw          m7,           m0
    pmaddubsw          m8,           m0
    pmaddwd            m7,           m3
    pmaddwd            m8,           m3

    packssdw           m5,           m7
    packssdw           m6,           m8
    pmulhrsw           m5,           m4
    pmulhrsw           m6,           m4
    packuswb           m5,           m6
    movu              [r2],          m5
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PP_32x2_AVX512 0
    ; register map
    ; m0 - interpolate coeff
    ; m1, m2 - shuffle order table
    ; m3 - constant word 1
    ; m4 - constant word 512
    ; m9 - store shuffle order table

    movu              ym5,           [r0]
    vinserti32x8       m5,           [r0 + r1], 1
    movu              ym7,           [r0 + 4]
    vinserti32x8       m7,           [r0 + r1 + 4], 1

    pshufb             m6,           m5,       m2
    pshufb             m5,           m1
    pshufb             m8,           m7,       m2
    pshufb             m7,           m1

    pmaddubsw          m5,           m0
    pmaddubsw          m7,           m0
    pmaddwd            m5,           m3
    pmaddwd            m7,           m3

    pmaddubsw          m6,           m0
    pmaddubsw          m8,           m0
    pmaddwd            m6,           m3
    pmaddwd            m8,           m3

    packssdw           m5,           m7
    packssdw           m6,           m8
    pmulhrsw           m5,           m4
    pmulhrsw           m6,           m4
    packuswb           m5,           m6
    movu             [r2],          ym5
    vextracti32x8    [r2 + r3],      m5,            1
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PP_16x4_AVX512 0
    ; register map
    ; m0 - interpolate coeff
    ; m1, m2 - shuffle order table
    ; m3 - constant word 1
    ; m4 - constant word 512

    movu              xm5,           [r0]
    vinserti32x4       m5,           [r0 + r1],            1
    vinserti32x4       m5,           [r0 + 2 * r1],        2
    vinserti32x4       m5,           [r0 + r6],            3
    pshufb             m6,           m5,       m2
    pshufb             m5,           m1

    movu              xm7,           [r0 + 4]
    vinserti32x4       m7,           [r0 + r1 + 4],        1
    vinserti32x4       m7,           [r0 + 2 * r1 + 4],    2
    vinserti32x4       m7,           [r0 + r6 + 4],        3
    pshufb             m8,           m7,       m2
    pshufb             m7,           m1

    pmaddubsw          m5,           m0
    pmaddubsw          m7,           m0
    pmaddwd            m5,           m3
    pmaddwd            m7,           m3

    pmaddubsw          m6,           m0
    pmaddubsw          m8,           m0
    pmaddwd            m6,           m3
    pmaddwd            m8,           m3

    packssdw           m5,           m7
    packssdw           m6,           m8
    pmulhrsw           m5,           m4
    pmulhrsw           m6,           m4
    packuswb           m5,           m6
    movu              [r2],          xm5
    vextracti32x4     [r2 + r3],     m5,       1
    vextracti32x4     [r2 + 2 * r3], m5,       2
    vextracti32x4     [r2 + r7],     m5,       3
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PP_48x4_AVX512 0
    ; register map
    ; m0 - interpolate coeff
    ; m1, m2 - shuffle order table
    ; m3 - constant word 1
    ; m4 - constant word 512
    movu              ym5,           [r0]
    vinserti32x8       m5,           [r0 + r1], 1
    movu              ym7,           [r0 + 4]
    vinserti32x8       m7,           [r0 + r1 + 4], 1

    pshufb             m6,           m5,           m2
    pshufb             m5,           m1
    pshufb             m8,           m7,           m2
    pshufb             m7,           m1

    pmaddubsw          m5,           m0
    pmaddubsw          m7,           m0
    pmaddwd            m5,           m3
    pmaddwd            m7,           m3

    pmaddubsw          m6,           m0
    pmaddubsw          m8,           m0
    pmaddwd            m6,           m3
    pmaddwd            m8,           m3

    packssdw           m5,           m7
    packssdw           m6,           m8
    pmulhrsw           m5,           m4
    pmulhrsw           m6,           m4
    packuswb           m5,           m6
    movu             [r2],          ym5
    vextracti32x8    [r2 + r3],      m5,            1

    movu              ym5,           [r0 + 2 * r1]
    vinserti32x8       m5,           [r0 + r6], 1
    movu              ym7,           [r0 + 2 * r1 + 4]
    vinserti32x8       m7,           [r0 + r6 + 4], 1

    pshufb             m6,           m5,           m2
    pshufb             m5,           m1
    pshufb             m8,           m7,           m2
    pshufb             m7,           m1

    pmaddubsw          m5,           m0
    pmaddubsw          m7,           m0
    pmaddwd            m5,           m3
    pmaddwd            m7,           m3

    pmaddubsw          m6,           m0
    pmaddubsw          m8,           m0
    pmaddwd            m6,           m3
    pmaddwd            m8,           m3

    packssdw           m5,           m7
    packssdw           m6,           m8
    pmulhrsw           m5,           m4
    pmulhrsw           m6,           m4
    packuswb           m5,           m6
    movu             [r2 + 2 * r3], ym5
    vextracti32x8    [r2 + r7],      m5,            1

    movu              xm5,           [r0 + mmsize/2]
    vinserti32x4       m5,           [r0 + r1 + mmsize/2],            1
    vinserti32x4       m5,           [r0 + 2 * r1 + mmsize/2],        2
    vinserti32x4       m5,           [r0 + r6 + mmsize/2],            3
    pshufb             m6,           m5,       m2
    pshufb             m5,           m1

    movu              xm7,           [r0 + 36]
    vinserti32x4       m7,           [r0 + r1 + 36],        1
    vinserti32x4       m7,           [r0 + 2 * r1 + 36],    2
    vinserti32x4       m7,           [r0 + r6 + 36],        3
    pshufb             m8,           m7,       m2
    pshufb             m7,           m1

    pmaddubsw          m5,           m0
    pmaddubsw          m7,           m0
    pmaddwd            m5,           m3
    pmaddwd            m7,           m3

    pmaddubsw          m6,           m0
    pmaddubsw          m8,           m0
    pmaddwd            m6,           m3
    pmaddwd            m8,           m3

    packssdw           m5,           m7
    packssdw           m6,           m8
    pmulhrsw           m5,           m4
    pmulhrsw           m6,           m4
    packuswb           m5,           m6
    movu              [r2 + mmsize/2],          xm5
    vextracti32x4     [r2 + r3 + mmsize/2],     m5,       1
    vextracti32x4     [r2 + 2 * r3 + mmsize/2], m5,       2
    vextracti32x4     [r2 + r7 + mmsize/2],     m5,       3
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp_64xN(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PP_64xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_pp_64x%1, 4,6,9
    mov               r4d,               r4m

%ifdef PIC
    lea               r5,           [tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti32x8   m1,           [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8   m2,           [interp4_horiz_shuf_load2_avx512]
    vbroadcasti32x8   m3,           [pw_1]
    vbroadcasti32x8   m4,           [pw_512]
    dec               r0

%rep %1 - 1
    PROCESS_IPFILTER_CHROMA_PP_64x1_AVX512
    lea               r2,           [r2 + r3]
    lea               r0,           [r0 + r1]
%endrep
    PROCESS_IPFILTER_CHROMA_PP_64x1_AVX512
    RET
%endmacro

%if ARCH_X86_64
    IPFILTER_CHROMA_PP_64xN_AVX512  64
    IPFILTER_CHROMA_PP_64xN_AVX512  32
    IPFILTER_CHROMA_PP_64xN_AVX512  48
    IPFILTER_CHROMA_PP_64xN_AVX512  16
%endif

%macro IPFILTER_CHROMA_PP_32xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_pp_32x%1, 4,6,9
    mov               r4d,               r4m

%ifdef PIC
    lea               r5,           [tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti32x8   m1,           [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8   m2,           [interp4_horiz_shuf_load2_avx512]
    vbroadcasti32x8   m3,           [pw_1]
    vbroadcasti32x8   m4,           [pw_512]
    dec               r0

%rep %1/2 - 1
    PROCESS_IPFILTER_CHROMA_PP_32x2_AVX512
    lea               r2,           [r2 + 2 * r3]
    lea               r0,           [r0 + 2 * r1]
%endrep
    PROCESS_IPFILTER_CHROMA_PP_32x2_AVX512
    RET
%endmacro

%if ARCH_X86_64
    IPFILTER_CHROMA_PP_32xN_AVX512 16
    IPFILTER_CHROMA_PP_32xN_AVX512 24
    IPFILTER_CHROMA_PP_32xN_AVX512 8
    IPFILTER_CHROMA_PP_32xN_AVX512 32
    IPFILTER_CHROMA_PP_32xN_AVX512 64
    IPFILTER_CHROMA_PP_32xN_AVX512 48
%endif

%macro IPFILTER_CHROMA_PP_16xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_pp_16x%1, 4,8,9
    mov               r4d,          r4m
    lea               r6,           [3 * r1]
    lea               r7,           [3 * r3]
%ifdef PIC
    lea               r5,           [tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti32x8   m1,           [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8   m2,           [interp4_horiz_shuf_load2_avx512]
    vbroadcasti32x8   m3,           [pw_1]
    vbroadcasti32x8   m4,           [pw_512]
    dec               r0

%rep %1/4 - 1
    PROCESS_IPFILTER_CHROMA_PP_16x4_AVX512
    lea               r2,           [r2 + 4 * r3]
    lea               r0,           [r0 + 4 * r1]
%endrep
    PROCESS_IPFILTER_CHROMA_PP_16x4_AVX512
    RET
%endmacro

%if ARCH_X86_64
    IPFILTER_CHROMA_PP_16xN_AVX512 4
    IPFILTER_CHROMA_PP_16xN_AVX512 8
    IPFILTER_CHROMA_PP_16xN_AVX512 12
    IPFILTER_CHROMA_PP_16xN_AVX512 16
    IPFILTER_CHROMA_PP_16xN_AVX512 24
    IPFILTER_CHROMA_PP_16xN_AVX512 32
    IPFILTER_CHROMA_PP_16xN_AVX512 64
%endif

%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_4tap_horiz_pp_48x64, 4,8,9
    mov               r4d,          r4m
    lea               r6,           [3 * r1]
    lea               r7,           [3 * r3]
%ifdef PIC
    lea               r5,           [tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti32x8   m1,           [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8   m2,           [interp4_horiz_shuf_load2_avx512]
    vbroadcasti32x8   m3,           [pw_1]
    vbroadcasti32x8   m4,           [pw_512]
    dec               r0

%rep 15
    PROCESS_IPFILTER_CHROMA_PP_48x4_AVX512
    lea               r2,           [r2 + 4 * r3]
    lea               r0,           [r0 + 4 * r1]
%endrep
    PROCESS_IPFILTER_CHROMA_PP_48x4_AVX512
    RET
%endif

%macro PROCESS_IPFILTER_CHROMA_PS_64x1_AVX512 0
    movu               ym6,          [r0]
    vinserti32x8       m6,           [r0 + 4], 1
    pshufb             m7,           m6,       m2
    pshufb             m6,           m1
    pmaddubsw          m6,           m0
    pmaddubsw          m7,           m0
    pmaddwd            m6,           m3
    pmaddwd            m7,           m3

    movu               ym8,          [r0 + 32]
    vinserti32x8       m8,           [r0 + 36], 1
    pshufb             m9,           m8,       m2
    pshufb             m8,           m1
    pmaddubsw          m8,           m0
    pmaddubsw          m9,           m0
    pmaddwd            m8,           m3
    pmaddwd            m9,           m3

    packssdw           m6,           m7
    packssdw           m8,           m9
    psubw              m6,           m4
    psubw              m8,           m4
    vpermq             m6,           m10,       m6
    vpermq             m8,           m10,       m8
    movu               [r2],         m6
    movu               [r2 + mmsize],m8
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_horiz_ps_64xN(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PS_64xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_ps_64x%1, 4,7,11
    mov             r4d, r4m
    mov             r5d, r5m

%ifdef PIC
    lea               r6,           [tab_ChromaCoeff]
    vpbroadcastd      m0,           [r6 + r4 * 4]
%else
    vpbroadcastd      m0,           [tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti32x8    m1,           [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8    m2,           [interp4_horiz_shuf_load2_avx512]
    vbroadcasti32x8    m3,           [pw_1]
    vbroadcasti32x8    m4,           [pw_2000]
    mova               m10,          [interp4_hps_shuf_avx512]

    ; register map
    ; m0    - interpolate coeff
    ; m1,m2 - load shuffle order table
    ; m3    - constant word 1
    ; m4    - constant word 2000
    ; m10   - store shuffle order table

    mov               r6d,         %1
    dec               r0
    test              r5d,         r5d
    je                .loop
    sub               r0,          r1
    add               r6d,         3

.loop:
    PROCESS_IPFILTER_CHROMA_PS_64x1_AVX512
    lea               r2,           [r2 + 2 * r3]
    lea               r0,           [r0 + r1]
    dec               r6d
    jnz               .loop
    RET
%endmacro

%if ARCH_X86_64
    IPFILTER_CHROMA_PS_64xN_AVX512 64
    IPFILTER_CHROMA_PS_64xN_AVX512 32
    IPFILTER_CHROMA_PS_64xN_AVX512 48
    IPFILTER_CHROMA_PS_64xN_AVX512 16
%endif

%macro PROCESS_IPFILTER_CHROMA_PS_32x1_AVX512 0
    movu               ym6,          [r0]
    vinserti32x8       m6,           [r0 + 4], 1
    pshufb             m7,           m6,       m2
    pshufb             m6,           m6,       m1
    pmaddubsw          m6,           m0
    pmaddubsw          m7,           m0
    pmaddwd            m6,           m3
    pmaddwd            m7,           m3

    packssdw           m6,           m7
    psubw              m6,           m4
    vpermq             m6,           m8,       m6
    movu               [r2],         m6
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_horiz_ps_32xN(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PS_32xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_ps_32x%1, 4,7,9
    mov             r4d, r4m
    mov             r5d, r5m

%ifdef PIC
    lea               r6,           [tab_ChromaCoeff]
    vpbroadcastd      m0,           [r6 + r4 * 4]
%else
    vpbroadcastd      m0,           [tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti32x8    m1,           [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8    m2,           [interp4_horiz_shuf_load2_avx512]
    vbroadcasti32x8    m3,           [pw_1]
    vbroadcasti32x8    m4,           [pw_2000]
    mova               m8,           [interp4_hps_shuf_avx512]

    ; register map
    ; m0    - interpolate coeff
    ; m1,m2 - load shuffle order table
    ; m3    - constant word 1
    ; m4    - constant word 2000
    ; m8   - store shuffle order table

    mov               r6d,         %1
    dec               r0
    test              r5d,         r5d
    je                .loop
    sub               r0,          r1
    add               r6d,         3

.loop:
    PROCESS_IPFILTER_CHROMA_PS_32x1_AVX512
    lea               r2,           [r2 + 2 * r3]
    lea               r0,           [r0 + r1]
    dec               r6d
    jnz               .loop
    RET
%endmacro

%if ARCH_X86_64
    IPFILTER_CHROMA_PS_32xN_AVX512 64
    IPFILTER_CHROMA_PS_32xN_AVX512 48
    IPFILTER_CHROMA_PS_32xN_AVX512 32
    IPFILTER_CHROMA_PS_32xN_AVX512 24
    IPFILTER_CHROMA_PS_32xN_AVX512 16
    IPFILTER_CHROMA_PS_32xN_AVX512 8
%endif

%macro PROCESS_IPFILTER_CHROMA_PS_16x2_AVX512 0
    movu               xm6,         [r0]
    vinserti32x4       m6,          [r0 + 4],      1
    vinserti32x4       m6,          [r0 + r1],     2
    vinserti32x4       m6,          [r0 + r1 + 4], 3

    pshufb             m7,          m6,            m2
    pshufb             m6,          m6,            m1
    pmaddubsw          m6,          m0
    pmaddubsw          m7,          m0
    pmaddwd            m6,          m3
    pmaddwd            m7,          m3

    packssdw           m6,          m7
    psubw              m6,          m4
    vpermq             m6,          m8,            m6
    movu               [r2],        ym6
    vextracti32x8      [r2 + r3],   m6,            1
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PS_16x1_AVX512 0
    movu              xm6,          [r0]
    vinserti32x4      m6,           [r0 + 4],  1

    pshufb            ym7,          ym6,       ym2
    pshufb            ym6,          ym6,       ym1
    pmaddubsw         ym6,          ym0
    pmaddubsw         ym7,          ym0
    pmaddwd           ym6,          ym3
    pmaddwd           ym7,          ym3

    packssdw          ym6,          ym7
    psubw             ym6,          ym4
    vpermq            ym6,          ym8,       ym6
    movu              [r2],         ym6
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_horiz_ps_16xN(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PS_16xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_ps_16x%1, 4,7,9
    mov             r4d, r4m
    mov             r5d, r5m
    add             r3,  r3

%ifdef PIC
    lea               r6,           [tab_ChromaCoeff]
    vpbroadcastd      m0,           [r6 + r4 * 4]
%else
    vpbroadcastd      m0,           [tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti32x8    m1,          [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8    m2,          [interp4_horiz_shuf_load2_avx512]
    vbroadcasti32x8    m3,          [pw_1]
    vbroadcasti32x8    m4,          [pw_2000]
    mova               m8,          [interp4_hps_store_16xN_avx512]

    ; register map
    ; m0    - interpolate coeff
    ; m1,m2 - load shuffle order table
    ; m3    - constant word 1
    ; m4    - constant word 2000
    ; m8   - store shuffle order table

    mov               r6d,          %1
    dec               r0
    test              r5d,          r5d
    je                .loop
    sub               r0,           r1
    add               r6d,          3
    PROCESS_IPFILTER_CHROMA_PS_16x1_AVX512
    lea               r2,           [r2 + r3]
    lea               r0,           [r0 + r1]
    dec               r6d

.loop:
    PROCESS_IPFILTER_CHROMA_PS_16x2_AVX512
    lea               r2,           [r2 + 2 * r3]
    lea               r0,           [r0 + 2 * r1]
    sub               r6d,          2
    jnz               .loop

    RET
%endmacro

%if ARCH_X86_64 == 1
    IPFILTER_CHROMA_PS_16xN_AVX512 64
    IPFILTER_CHROMA_PS_16xN_AVX512 32
    IPFILTER_CHROMA_PS_16xN_AVX512 24
    IPFILTER_CHROMA_PS_16xN_AVX512 16
    IPFILTER_CHROMA_PS_16xN_AVX512 12
    IPFILTER_CHROMA_PS_16xN_AVX512 8
    IPFILTER_CHROMA_PS_16xN_AVX512 4
%endif

%macro PROCESS_IPFILTER_CHROMA_PS_48x1_AVX512 0
    movu               ym6,          [r0]
    vinserti32x8       m6,           [r0 + 4], 1
    pshufb             m7,           m6,       m2
    pshufb             m6,           m6,       m1
    pmaddubsw          m6,           m0
    pmaddubsw          m7,           m0
    pmaddwd            m6,           m3
    pmaddwd            m7,           m3

    packssdw           m6,           m7
    psubw              m6,           m4
    vpermq             m6,           m8,       m6
    movu               [r2],         m6

    movu              xm6,          [r0 + 32]
    vinserti32x4      m6,           [r0 + 36], 1
    pshufb            ym7,          ym6,       ym2
    pshufb            ym6,          ym6,       ym1
    pmaddubsw         ym6,          ym0
    pmaddubsw         ym7,          ym0
    pmaddwd           ym6,          ym3
    pmaddwd           ym7,          ym3

    packssdw          ym6,          ym7
    psubw             ym6,          ym4
    vpermq            ym6,          ym9,       ym6
    movu              [r2 + mmsize],ym6
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_horiz_ps_48xN(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PS_48xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_ps_48x%1, 4,7,10
    mov             r4d, r4m
    mov             r5d, r5m

%ifdef PIC
    lea               r6,           [tab_ChromaCoeff]
    vpbroadcastd      m0,           [r6 + r4 * 4]
%else
    vpbroadcastd      m0,           [tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti32x8    m1,           [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8    m2,           [interp4_horiz_shuf_load2_avx512]
    vbroadcasti32x8    m3,           [pw_1]
    vbroadcasti32x8    m4,           [pw_2000]
    mova               m8,           [interp4_hps_shuf_avx512]
    mova               m9,           [interp4_hps_store_16xN_avx512]

    ; register map
    ; m0    - interpolate coeff
    ; m1,m2 - load shuffle order table
    ; m3    - constant word 1
    ; m4    - constant word 2000
    ; m8   - store shuffle order table

    mov               r6d,         %1
    dec               r0
    test              r5d,         r5d
    je                .loop
    sub               r0,          r1
    add               r6d,         3

.loop:
    PROCESS_IPFILTER_CHROMA_PS_48x1_AVX512
    lea               r2,           [r2 + 2 * r3]
    lea               r0,           [r0 + r1]
    dec               r6d
    jnz               .loop
    RET
%endmacro

%if ARCH_X86_64 == 1
    IPFILTER_CHROMA_PS_48xN_AVX512 64
%endif

;-------------------------------------------------------------------------------------------------------------
;avx512 chroma_vpp and chroma_vps code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_CHROMA_VERT_16x4_AVX512 1
    lea                   r5,                 [r0 + 4 * r1]
    movu                  xm1,                [r0]
    movu                  xm3,                [r0 + r1]
    vinserti32x4          m1,                 [r0 + r1],           1
    vinserti32x4          m3,                 [r0 + 2 * r1],       1
    vinserti32x4          m1,                 [r0 + 2 * r1],       2
    vinserti32x4          m3,                 [r0 + r6],           2
    vinserti32x4          m1,                 [r0 + r6],           3
    vinserti32x4          m3,                 [r0 + 4 * r1],       3

    punpcklbw             m0,                 m1,                  m3
    pmaddubsw             m0,                 m8
    punpckhbw             m1,                 m3
    pmaddubsw             m1,                 m8

    movu                  xm4,                [r0 + 2 * r1]
    movu                  xm5,                [r0 + r6]
    vinserti32x4          m4,                 [r0 + r6],           1
    vinserti32x4          m5,                 [r5],                1
    vinserti32x4          m4,                 [r5],                2
    vinserti32x4          m5,                 [r5 + r1],           2
    vinserti32x4          m4,                 [r5 + r1],           3
    vinserti32x4          m5,                 [r5 + 2 * r1],       3

    punpcklbw             m3,                 m4,                  m5
    pmaddubsw             m3,                 m9
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m9

    paddw                 m0,                 m3
    paddw                 m1,                 m4
%ifidn %1,pp
    pmulhrsw              m0,                 m7
    pmulhrsw              m1,                 m7
    packuswb              m0,                 m1
    movu                  [r2],               xm0
    vextracti32x4         [r2 + r3],          m0,                  1
    vextracti32x4         [r2 + 2 * r3],      m0,                  2
    vextracti32x4         [r2 + r7],          m0,                  3
%else
    psubw                 m0,                 m7
    psubw                 m1,                 m7
    mova                  m2,                 m10
    mova                  m3,                 m11

    vpermi2q              m2,                 m0,                  m1
    vpermi2q              m3,                 m0,                  m1
    
    movu                  [r2],               ym2
    vextracti32x8         [r2 + r3],          m2,                   1
    movu                  [r2 + 2 * r3],      ym3
    vextracti32x8         [r2 + r7],          m3,                   1
%endif
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VERT_CHROMA_16xN_AVX512 2
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_16x%2, 4, 10, 12
    mov                   r4d,                r4m
    shl                   r4d,                7
    sub                   r0,                 r1

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffVer_32_avx512]
    mova                  m8,                 [r5 + r4]
    mova                  m9,                 [r5 + r4 + mmsize]
%else
    mova                  m8,                 [tab_ChromaCoeffVer_32_avx512 + r4]
    mova                  m9,                 [tab_ChromaCoeffVer_32_avx512 + r4 + mmsize]
%endif

%ifidn %1, pp
    vbroadcasti32x8       m7,                 [pw_512]
%else
    shl                   r3d,                1
    vbroadcasti32x8       m7,                 [pw_2000]
    mova                  m10,                [interp4_vps_store1_avx512]
    mova                  m11,                [interp4_vps_store2_avx512]
%endif
    lea                   r6,                 [3 * r1]
    lea                   r7,                 [3 * r3]

%rep %2/4 - 1
    PROCESS_CHROMA_VERT_16x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_16x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VERT_CHROMA_16xN_AVX512 pp, 4
    FILTER_VERT_CHROMA_16xN_AVX512 pp, 8
    FILTER_VERT_CHROMA_16xN_AVX512 pp, 12
    FILTER_VERT_CHROMA_16xN_AVX512 pp, 16
    FILTER_VERT_CHROMA_16xN_AVX512 pp, 24
    FILTER_VERT_CHROMA_16xN_AVX512 pp, 32
    FILTER_VERT_CHROMA_16xN_AVX512 pp, 64

    FILTER_VERT_CHROMA_16xN_AVX512 ps, 4
    FILTER_VERT_CHROMA_16xN_AVX512 ps, 8
    FILTER_VERT_CHROMA_16xN_AVX512 ps, 12
    FILTER_VERT_CHROMA_16xN_AVX512 ps, 16
    FILTER_VERT_CHROMA_16xN_AVX512 ps, 24
    FILTER_VERT_CHROMA_16xN_AVX512 ps, 32
    FILTER_VERT_CHROMA_16xN_AVX512 ps, 64
%endif
%macro PROCESS_CHROMA_VERT_32x4_AVX512 1
    movu                  ym1,                [r0]
    movu                  ym3,                [r0 + r1]
    vinserti32x8          m1,                 [r0 + 2 * r1],       1
    vinserti32x8          m3,                 [r0 + r6],           1
    punpcklbw             m0,                 m1,                  m3
    pmaddubsw             m0,                 m8
    punpckhbw             m1,                 m3
    pmaddubsw             m1,                 m8

    movu                  ym4,                [r0 + 2 * r1]
    vinserti32x8          m4,                 [r0 + 4 * r1],       1
    punpcklbw             m2,                 m3,                  m4
    pmaddubsw             m2,                 m8
    punpckhbw             m3,                 m4
    pmaddubsw             m3,                 m8

    lea                   r0,                 [r0 + 2 * r1]

    movu                  ym5,                [r0 + r1]
    vinserti32x8          m5,                 [r0 + r6],           1
    punpcklbw             m6,                 m4,                  m5
    pmaddubsw             m6,                 m9
    paddw                 m0,                 m6
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m9
    paddw                 m1,                 m4

    movu                  ym4,                [r0 + 2 * r1]
    vinserti32x8          m4,                 [r0 + 4 * r1],       1
    punpcklbw             m6,                 m5,                  m4
    pmaddubsw             m6,                 m9
    paddw                 m2,                 m6
    punpckhbw             m5,                 m4
    pmaddubsw             m5,                 m9
    paddw                 m3,                 m5

%ifidn %1,pp
    pmulhrsw              m0,                 m7
    pmulhrsw              m1,                 m7
    pmulhrsw              m2,                 m7
    pmulhrsw              m3,                 m7
    packuswb              m0,                 m1
    packuswb              m2,                 m3
    movu                  [r2],               ym0
    movu                  [r2 + r3],          ym2
    vextracti32x8         [r2 + 2 * r3],      m0,                  1
    vextracti32x8         [r2 + r7],          m2,                  1
%else
    psubw                 m0,                 m7
    psubw                 m1,                 m7
    psubw                 m2,                 m7
    psubw                 m3,                 m7

    mova                  m4,                 m10
    mova                  m5,                 m11
    vpermi2q              m4,                 m0,                m1
    vpermi2q              m5,                 m0,                m1
    mova                  m6,                 m10
    mova                  m12,                m11
    vpermi2q              m6,                 m2,                m3
    vpermi2q              m12,                 m2,                m3

    movu                  [r2],               m4
    movu                  [r2 + r3],          m6
    movu                  [r2 + 2 * r3],      m5
    movu                  [r2 + r7],          m12
%endif
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VERT_CHROMA_32xN_AVX512 2
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_32x%2, 4, 8, 13
    mov                   r4d,                r4m
    shl                   r4d,                7
    sub                   r0,                 r1

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffVer_32_avx512]
    mova                  m8,                 [r5 + r4]
    mova                  m9,                 [r5 + r4 + mmsize]
%else
    mova                  m8,                 [tab_ChromaCoeffVer_32_avx512 + r4]
    mova                  m9,                 [tab_ChromaCoeffVer_32_avx512 + r4 + mmsize]
%endif

%ifidn %1,pp
    vbroadcasti32x8       m7,                [pw_512]
%else
    shl                   r3d,                1
    vbroadcasti32x8       m7,                [pw_2000]
    mova                  m10,                [interp4_vps_store1_avx512]
    mova                  m11,                [interp4_vps_store2_avx512]
%endif

    lea                   r6,                 [3 * r1]
    lea                   r7,                 [3 * r3]

%rep %2/4 - 1
    PROCESS_CHROMA_VERT_32x4_AVX512 %1
    lea                   r0,                 [r0 + 2 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_32x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VERT_CHROMA_32xN_AVX512 pp, 8
    FILTER_VERT_CHROMA_32xN_AVX512 pp, 16
    FILTER_VERT_CHROMA_32xN_AVX512 pp, 24
    FILTER_VERT_CHROMA_32xN_AVX512 pp, 32
    FILTER_VERT_CHROMA_32xN_AVX512 pp, 48
    FILTER_VERT_CHROMA_32xN_AVX512 pp, 64

    FILTER_VERT_CHROMA_32xN_AVX512 ps, 8
    FILTER_VERT_CHROMA_32xN_AVX512 ps, 16
    FILTER_VERT_CHROMA_32xN_AVX512 ps, 24
    FILTER_VERT_CHROMA_32xN_AVX512 ps, 32
    FILTER_VERT_CHROMA_32xN_AVX512 ps, 48
    FILTER_VERT_CHROMA_32xN_AVX512 ps, 64
%endif
%macro PROCESS_CHROMA_VERT_48x4_AVX512 1
    movu                  ym1,                [r0]
    movu                  ym3,                [r0 + r1]
    vinserti32x8          m1,                 [r0 + 2 * r1],       1
    vinserti32x8          m3,                 [r0 + r6],           1
    punpcklbw             m0,                 m1,                  m3
    pmaddubsw             m0,                 m8
    punpckhbw             m1,                 m3
    pmaddubsw             m1,                 m8

    movu                  ym4,                [r0 + 2 * r1]
    vinserti32x8          m4,                 [r0 + 4 * r1],       1
    punpcklbw             m2,                 m3,                  m4
    pmaddubsw             m2,                 m8
    punpckhbw             m3,                 m4
    pmaddubsw             m3,                 m8

    lea                   r5,                 [r0 + 4 * r1]

    movu                  ym5,                [r0 + r6]
    vinserti32x8          m5,                 [r5 + r1],           1
    punpcklbw             m6,                 m4,                  m5
    pmaddubsw             m6,                 m9
    paddw                 m0,                 m6
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m9
    paddw                 m1,                 m4

    movu                  ym4,                [r0 + 4 * r1]
    vinserti32x8          m4,                 [r5 + 2 * r1],       1
    punpcklbw             m6,                 m5,                  m4
    pmaddubsw             m6,                 m9
    paddw                 m2,                 m6
    punpckhbw             m5,                 m4
    pmaddubsw             m5,                 m9
    paddw                 m3,                 m5
%ifidn %1, pp
    pmulhrsw              m0,                 m7
    pmulhrsw              m1,                 m7
    pmulhrsw              m2,                 m7
    pmulhrsw              m3,                 m7

    packuswb              m0,                 m1
    packuswb              m2,                 m3
    movu                  [r2],               ym0
    movu                  [r2 + r3],          ym2
    vextracti32x8         [r2 + 2 * r3],      m0,                  1
    vextracti32x8         [r2 + r7],          m2,                  1
%else
    psubw                 m0,                 m7
    psubw                 m1,                 m7
    psubw                 m2,                 m7
    psubw                 m3,                 m7

    mova                  m4,                 m10
    mova                  m5,                 m11
    vpermi2q              m4,                 m0,                m1
    vpermi2q              m5,                 m0,                m1
    mova                  m6,                 m10
    mova                  m12,                m11
    vpermi2q              m6,                 m2,                m3
    vpermi2q              m12,                m2,                m3

    movu                  [r2],               m4
    movu                  [r2 + r3],          m6
    movu                  [r2 + 2 * r3],      m5
    movu                  [r2 + r7],          m12
%endif
    movu                  xm1,                [r0 + mmsize/2]
    movu                  xm3,                [r0 + r1 + mmsize/2]
    vinserti32x4          m1,                 [r0 + r1 + mmsize/2],           1
    vinserti32x4          m3,                 [r0 + 2 * r1 + mmsize/2],       1
    vinserti32x4          m1,                 [r0 + 2 * r1 + mmsize/2],       2
    vinserti32x4          m3,                 [r0 + r6 + mmsize/2],           2
    vinserti32x4          m1,                 [r0 + r6 + mmsize/2],           3
    vinserti32x4          m3,                 [r0 + 4 * r1 + mmsize/2],       3

    punpcklbw             m0,                 m1,                  m3
    pmaddubsw             m0,                 m8
    punpckhbw             m1,                 m3
    pmaddubsw             m1,                 m8

    movu                  xm4,                [r0 + 2 * r1 + mmsize/2]
    movu                  xm5,                [r0 + r6 + mmsize/2]
    vinserti32x4          m4,                 [r0 + r6 + mmsize/2],           1
    vinserti32x4          m5,                 [r5 + mmsize/2],                1
    vinserti32x4          m4,                 [r5 + mmsize/2],                2
    vinserti32x4          m5,                 [r5 + r1 + mmsize/2],           2
    vinserti32x4          m4,                 [r5 + r1 + mmsize/2],           3
    vinserti32x4          m5,                 [r5 + 2 * r1 + mmsize/2],       3

    punpcklbw             m3,                 m4,                  m5
    pmaddubsw             m3,                 m9
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m9
    paddw                 m0,                 m3
    paddw                 m1,                 m4
%ifidn %1, pp
    pmulhrsw              m0,                 m7
    pmulhrsw              m1,                 m7
    packuswb              m0,                 m1
    movu                  [r2 + mmsize/2],               xm0
    vextracti32x4         [r2 + r3 + mmsize/2],          m0,                  1
    vextracti32x4         [r2 + 2 * r3 + mmsize/2],      m0,                  2
    vextracti32x4         [r2 + r7 + mmsize/2],          m0,                  3
%else
    psubw                 m0,                 m7
    psubw                 m1,                 m7
    mova                  m2,                m10
    mova                  m3,                m11

    vpermi2q              m2,  m0, m1
    vpermi2q              m3,  m0, m1

    movu                  [r2 + mmsize],               ym2
    vextracti32x8         [r2 + r3 + mmsize],          m2,                  1
    movu                  [r2 + 2 * r3 + mmsize],      ym3
    vextracti32x8         [r2 + r7 + mmsize],          m3,                  1
%endif
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VERT_CHROMA_48x64_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_48x64, 4, 8, 13
    mov                   r4d,                r4m
    shl                   r4d,                7
    sub                   r0,                 r1

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffVer_32_avx512]
    mova                  m8,                 [r5 + r4]
    mova                  m9,                 [r5 + r4 + mmsize]
%else
    mova                  m8,                 [tab_ChromaCoeffVer_32_avx512 + r4]
    mova                  m9,                 [tab_ChromaCoeffVer_32_avx512 + r4 + mmsize]
%endif

%ifidn %1, pp
    vbroadcasti32x8       m7,                 [pw_512]
%else
    shl                   r3d,                1
    vbroadcasti32x8       m7,                 [pw_2000]
    mova                  m10,                [interp4_vps_store1_avx512]
    mova                  m11,                [interp4_vps_store2_avx512]
%endif

    lea                   r6,                 [3 * r1]
    lea                   r7,                 [3 * r3]
%rep 15
    PROCESS_CHROMA_VERT_48x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_48x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VERT_CHROMA_48x64_AVX512 pp
    FILTER_VERT_CHROMA_48x64_AVX512 ps
%endif
%macro PROCESS_CHROMA_VERT_64x4_AVX512 1
    movu              m0,              [r0]                        ; m0 = row 0
    movu              m1,              [r0 + r1]                   ; m1 = row 1
    punpcklbw         m2,              m0,                m1
    punpckhbw         m3,              m0,                m1
    pmaddubsw         m2,              m10
    pmaddubsw         m3,              m10
    movu              m0,              [r0 + r1 * 2]               ; m0 = row 2
    punpcklbw         m4,              m1,                m0
    punpckhbw         m5,              m1,                m0
    pmaddubsw         m4,              m10
    pmaddubsw         m5,              m10
    movu              m1,              [r0 + r4]                   ; m1 = row 3
    punpcklbw         m6,              m0,                m1
    punpckhbw         m7,              m0,                m1
    pmaddubsw         m8,              m6,                m11
    pmaddubsw         m9,              m7,                m11
    pmaddubsw         m6,              m10
    pmaddubsw         m7,              m10
    paddw             m2,              m8
    paddw             m3,              m9

%ifidn %1,pp
    pmulhrsw          m2,              m12
    pmulhrsw          m3,              m12
    packuswb          m2,              m3
    movu              [r2],            m2
%else
    psubw             m2, m12
    psubw             m3, m12
    movu              m8, m13
    movu              m9, m14
    vpermi2q          m8, m2, m3
    vpermi2q          m9, m2, m3
    movu              [r2], m8
    movu              [r2 + mmsize], m9
%endif

    lea               r0,              [r0 + r1 * 4]
    movu              m0,              [r0]                        ; m0 = row 4
    punpcklbw         m2,              m1,                m0
    punpckhbw         m3,              m1,                m0
    pmaddubsw         m8,              m2,                m11
    pmaddubsw         m9,              m3,                m11
    pmaddubsw         m2,              m10
    pmaddubsw         m3,              m10
    paddw             m4,              m8
    paddw             m5,              m9

%ifidn %1,pp
    pmulhrsw          m4,              m12
    pmulhrsw          m5,              m12
    packuswb          m4,              m5
    movu              [r2 + r3],       m4
%else
    psubw             m4, m12
    psubw             m5, m12
    movu              m8, m13
    movu              m9, m14
    vpermi2q          m8, m4, m5
    vpermi2q          m9, m4, m5
    movu              [r2 + r3], m8
    movu              [r2 + r3 + mmsize], m9
%endif

    movu              m1,              [r0 + r1]                   ; m1 = row 5
    punpcklbw         m4,              m0,                m1
    punpckhbw         m5,              m0,                m1
    pmaddubsw         m4,              m11
    pmaddubsw         m5,              m11
    paddw             m6,              m4
    paddw             m7,              m5

%ifidn %1,pp
    pmulhrsw          m6,              m12
    pmulhrsw          m7,              m12
    packuswb          m6,              m7
    movu              [r2 + r3 * 2],   m6
%else
    psubw             m6, m12
    psubw             m7, m12
    movu              m8, m13
    movu              m9, m14
    vpermi2q          m8, m6, m7
    vpermi2q          m9, m6, m7
    movu              [r2 + 2 * r3], m8
    movu              [r2 + 2 * r3 + mmsize], m9
%endif
    movu              m0,              [r0 + r1 * 2]               ; m0 = row 6
    punpcklbw         m6,              m1,                m0
    punpckhbw         m7,              m1,                m0
    pmaddubsw         m6,              m11
    pmaddubsw         m7,              m11
    paddw             m2,              m6
    paddw             m3,              m7

%ifidn %1,pp
    pmulhrsw          m2,              m12
    pmulhrsw          m3,              m12
    packuswb          m2,              m3
    movu              [r2 + r5],       m2
%else
    psubw             m2, m12
    psubw             m3, m12
    movu              m8, m13
    movu              m9, m14
    vpermi2q          m8, m2, m3
    vpermi2q          m9, m2, m3
    movu              [r2 + r5], m8
    movu              [r2 + r5 + mmsize], m9
%endif
%endmacro

%macro FILTER_VER_CHROMA_AVX512_64xN 2
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_64x%2, 4, 6, 15
    mov               r4d,             r4m
    shl               r4d,             7

%ifdef PIC
    lea               r5,              [tab_ChromaCoeffVer_32_avx512]
    mova              m10,             [r5 + r4]
    mova              m11,             [r5 + r4 + mmsize]
%else
    mova              m10,             [tab_ChromaCoeffVer_32_avx512 + r4]
    mova              m11,             [tab_ChromaCoeffVer_32_avx512 + r4 + mmsize]
%endif

%ifidn %1,pp
    vbroadcasti32x8            m12, [pw_512]
%else
    shl                        r3d, 1
    vbroadcasti32x8            m12, [pw_2000]
    mova                       m13, [interp4_vps_store1_avx512]
    mova                       m14, [interp4_vps_store2_avx512]
%endif
    lea               r4,              [r1 * 3]
    sub               r0,              r1
    lea               r5,              [r3 * 3]

%rep %2/4 - 1
    PROCESS_CHROMA_VERT_64x4_AVX512 %1
    lea               r2, [r2 + r3 * 4]
%endrep
    PROCESS_CHROMA_VERT_64x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64 == 1
FILTER_VER_CHROMA_AVX512_64xN pp, 64
FILTER_VER_CHROMA_AVX512_64xN pp, 48
FILTER_VER_CHROMA_AVX512_64xN pp, 32
FILTER_VER_CHROMA_AVX512_64xN pp, 16

FILTER_VER_CHROMA_AVX512_64xN ps, 64
FILTER_VER_CHROMA_AVX512_64xN ps, 48
FILTER_VER_CHROMA_AVX512_64xN ps, 32
FILTER_VER_CHROMA_AVX512_64xN ps, 16
%endif
;-------------------------------------------------------------------------------------------------------------
;avx512 chroma_vpp and chroma_vps code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
;avx512 chroma_vss code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_CHROMA_VERT_SS_8x4_AVX512 0
    lea                   r5,                 [r0 + 4 * r1]
    movu                  xm1,                [r0]
    movu                  xm3,                [r0 + r1]
    vinserti32x4          m1,                 [r0 + r1],           1
    vinserti32x4          m3,                 [r0 + 2 * r1],       1
    vinserti32x4          m1,                 [r0 + 2 * r1],       2
    vinserti32x4          m3,                 [r0 + r6],           2
    vinserti32x4          m1,                 [r0 + r6],           3
    vinserti32x4          m3,                 [r0 + 4 * r1],       3

    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m8
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m8

    movu                  xm4,                [r0 + 2 * r1]
    movu                  xm5,                [r0 + r6]
    vinserti32x4          m4,                 [r0 + r6],           1
    vinserti32x4          m5,                 [r5],                1
    vinserti32x4          m4,                 [r5],                2
    vinserti32x4          m5,                 [r5 + r1],           2
    vinserti32x4          m4,                 [r5 + r1],           3
    vinserti32x4          m5,                 [r5 + 2 * r1],       3

    punpcklwd             m3,                 m4,                  m5
    pmaddwd               m3,                 m9
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m9

    paddd                 m0,                 m3
    paddd                 m1,                 m4

    psrad                 m0,                 6
    psrad                 m1,                 6
    packssdw              m0,                 m1
    movu                  [r2],               xm0
    vextracti32x4         [r2 + r3],          m0,                  1
    vextracti32x4         [r2 + 2 * r3],      m0,                  2
    vextracti32x4         [r2 + r7],          m0,                  3
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_SS_CHROMA_8xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_ss_8x%1, 5, 8, 10
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7
%ifdef PIC
    lea                   r5,                 [pw_ChromaCoeffVer_32_avx512]
    mova                  m8,                 [r5 + r4]
    mova                  m9,                 [r5 + r4 + mmsize]
%else
    lea                   r5,                 [pw_ChromaCoeffVer_32_avx512 + r4]
    mova                  m8,                 [r5]
    mova                  m9,                 [r5 + mmsize]
%endif
    lea                   r6,                 [3 * r1]
    lea                   r7,                 [3 * r3]

%rep %1/4 - 1
    PROCESS_CHROMA_VERT_SS_8x4_AVX512
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_SS_8x4_AVX512
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_SS_CHROMA_8xN_AVX512 4
    FILTER_VER_SS_CHROMA_8xN_AVX512 8
    FILTER_VER_SS_CHROMA_8xN_AVX512 12
    FILTER_VER_SS_CHROMA_8xN_AVX512 16
    FILTER_VER_SS_CHROMA_8xN_AVX512 32
    FILTER_VER_SS_CHROMA_8xN_AVX512 64
%endif

%macro PROCESS_CHROMA_VERT_S_16x4_AVX512 1
    movu                  ym1,                [r0]
    lea                   r6,                 [r0 + 2 * r1]
    vinserti32x8          m1,                 [r6],                1
    movu                  ym3,                [r0 + r1]
    vinserti32x8          m3,                 [r6 + r1],           1
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m7
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m7

    movu                  ym4,                [r0 + 2 * r1]
    vinserti32x8          m4,                 [r6 + 2 * r1],       1
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 m7
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m7

    movu                  ym5,                [r0 + r4]
    vinserti32x8          m5,                 [r6 + r4],           1
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 m8
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m8
    paddd                 m1,                 m4

    movu                  ym4,                [r0 + 4 * r1]
    vinserti32x8          m4,                 [r6 + 4 * r1],       1
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 m8
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m8
    paddd                 m3,                 m5

%ifidn %1, sp
    paddd                m0,                  m9
    paddd                m1,                  m9
    paddd                m2,                  m9
    paddd                m3,                  m9

    psrad                m0,                  12
    psrad                m1,                  12
    psrad                m2,                  12
    psrad                m3,                  12

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    packuswb             m0,                  m2
    vpermq               m0,                  m10,                 m0
    movu                 [r2],                xm0
    vextracti32x4        [r2 + r3],           m0,                  2
    vextracti32x4        [r2 + 2 * r3],       m0,                  1
    vextracti32x4        [r2 + r5],           m0,                  3
%else
    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6
    packssdw              m0,                 m1
    packssdw              m2,                 m3

    movu                  [r2],               ym0
    movu                  [r2 + r3],          ym2
    vextracti32x8         [r2 + 2 * r3],      m0,                  1
    vextracti32x8         [r2 + r5],          m2,                  1
%endif
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_CHROMA_16xN_AVX512 2
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_16x%2, 4, 7, 11
    mov                   r4d,                r4m
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [pw_ChromaCoeffVer_32_avx512]
    mova                  m7,                 [r5 + r4]
    mova                  m8,                 [r5 + r4 + mmsize]
%else
    mova                  m7,                 [pw_ChromaCoeffVer_32_avx512 + r4]
    mova                  m8,                 [pw_ChromaCoeffVer_32_avx512 + r4 + mmsize]
%endif

%ifidn %1, sp
    vbroadcasti32x4       m9,                 [pd_526336]
    mova                  m10,                [interp8_vsp_store_avx512]
%else
    add                   r3d,                r3d
%endif
    add                   r1d,                r1d
    sub                   r0,                 r1
    lea                   r4,                 [r1 * 3]
    lea                   r5,                 [r3 * 3]

%rep %2/4 - 1
    PROCESS_CHROMA_VERT_S_16x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_S_16x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 4
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 8
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 12
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 16
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 24
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 32
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 64
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 4
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 8
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 12
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 16
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 24
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 32
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 64
%endif

%macro PROCESS_CHROMA_VERT_SS_24x8_AVX512 0
    movu                  ym1,                [r0]
    lea                   r6,                 [r0 + 2 * r1]
    lea                   r8,                 [r0 + 4 * r1]
    lea                   r9,                 [r8 + 2 * r1]

    movu                  ym10,               [r8]
    movu                  ym3,                [r0 + r1]
    movu                  ym12,               [r8 + r1]
    vinserti32x8          m1,                 [r6],                1
    vinserti32x8          m10,                [r9],                1
    vinserti32x8          m3,                 [r6 + r1],           1
    vinserti32x8          m12,                [r9 + r1],           1

    punpcklwd             m0,                 m1,                  m3
    punpcklwd             m9,                 m10,                 m12
    pmaddwd               m0,                 m16
    pmaddwd               m9,                 m16
    punpckhwd             m1,                 m3
    punpckhwd             m10,                m12
    pmaddwd               m1,                 m16
    pmaddwd               m10,                m16

    movu                  ym4,                [r0 + 2 * r1]
    movu                  ym13,               [r8 + 2 * r1]
    vinserti32x8          m4,                 [r6 + 2 * r1],       1
    vinserti32x8          m13,                [r9 + 2 * r1],       1
    punpcklwd             m2,                 m3,                  m4
    punpcklwd             m11,                m12,                 m13
    pmaddwd               m2,                 m16
    pmaddwd               m11,                m16
    punpckhwd             m3,                 m4
    punpckhwd             m12,                m13
    pmaddwd               m3,                 m16
    pmaddwd               m12,                m16

    movu                  ym5,                [r0 + r10]
    vinserti32x8          m5,                 [r6 + r10],          1
    movu                  ym14,               [r8 + r10]
    vinserti32x8          m14,                [r9 + r10],          1
    punpcklwd             m6,                 m4,                  m5
    punpcklwd             m15,                m13,                 m14
    pmaddwd               m6,                 m17
    pmaddwd               m15,                m17
    paddd                 m0,                 m6
    paddd                 m9,                 m15
    punpckhwd             m4,                 m5
    punpckhwd             m13,                m14
    pmaddwd               m4,                 m17
    pmaddwd               m13,                m17
    paddd                 m1,                 m4
    paddd                 m10,                m13

    movu                  ym4,                [r0 + 4 * r1]
    vinserti32x8          m4,                 [r6 + 4 * r1],       1
    movu                  ym13,               [r8 + 4 * r1]
    vinserti32x8          m13,                [r9 + 4 * r1],       1
    punpcklwd             m6,                 m5,                  m4
    punpcklwd             m15,                m14,                 m13
    pmaddwd               m6,                 m17
    pmaddwd               m15,                m17
    paddd                 m2,                 m6
    paddd                 m11,                m15
    punpckhwd             m5,                 m4
    punpckhwd             m14,                m13
    pmaddwd               m5,                 m17
    pmaddwd               m14,                m17
    paddd                 m3,                 m5
    paddd                 m12,                m14

    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6
    psrad                 m9,                 6
    psrad                 m10,                6
    psrad                 m11,                6
    psrad                 m12,                6

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packssdw              m9,                 m10
    packssdw              m11,                m12

    movu                  [r2],               ym0
    movu                  [r2 + r3],          ym2
    vextracti32x8         [r2 + 2 * r3],      m0,                  1
    vextracti32x8         [r2 + r7],          m2,                  1
    lea                   r11,                [r2 + 4 * r3]
    movu                  [r11],              ym9
    movu                  [r11 + r3],         ym11
    vextracti32x8         [r11 + 2 * r3],     m9,                  1
    vextracti32x8         [r11 + r7],         m11,                 1

    movu                  xm1,                [r0 + mmsize/2]
    vinserti32x4          m1,                 [r6 + mmsize/2],                1
    vinserti32x4          m1,                 [r8 + mmsize/2],                2
    vinserti32x4          m1,                 [r9 + mmsize/2],                3
    movu                  xm3,                [r0 + r1 + mmsize/2]
    vinserti32x4          m3,                 [r6 + r1 + mmsize/2],           1
    vinserti32x4          m3,                 [r8 + r1 + mmsize/2],           2
    vinserti32x4          m3,                 [r9 + r1 + mmsize/2],           3
    punpcklwd             m0,                 m1,                             m3
    pmaddwd               m0,                 m16
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m16

    movu                  xm4,                [r0 + 2 * r1 + mmsize/2]
    vinserti32x4          m4,                 [r6 + 2 * r1 + mmsize/2],       1
    vinserti32x4          m4,                 [r8 + 2 * r1 + mmsize/2],       2
    vinserti32x4          m4,                 [r9 + 2 * r1 + mmsize/2],       3
    punpcklwd             m2,                 m3,                             m4
    pmaddwd               m2,                 m16
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m16

    movu                  xm5,                [r0 + r10 + mmsize/2]
    vinserti32x4          m5,                 [r6 + r10 + mmsize/2],          1
    vinserti32x4          m5,                 [r8 + r10 + mmsize/2],          2
    vinserti32x4          m5,                 [r9 + r10 + mmsize/2],          3
    punpcklwd             m6,                 m4,                             m5
    pmaddwd               m6,                 m17
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m17
    paddd                 m1,                 m4

    movu                  xm4,                [r0 + 4 * r1 + mmsize/2]
    vinserti32x4          m4,                 [r6 + 4 * r1 + mmsize/2],       1
    vinserti32x4          m4,                 [r8 + 4 * r1 + mmsize/2],       2
    vinserti32x4          m4,                 [r9 + 4 * r1 + mmsize/2],       3
    punpcklwd             m6,                 m5,                             m4
    pmaddwd               m6,                 m17
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m17
    paddd                 m3,                 m5

    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6

    packssdw              m0,                 m1
    packssdw              m2,                 m3

    movu                  [r2 + mmsize/2],               xm0
    movu                  [r2 + r3 + mmsize/2],          xm2
    vextracti32x4         [r2 + 2 * r3 + mmsize/2],      m0,                  1
    vextracti32x4         [r2 + r7 + mmsize/2],          m2,                  1
    lea                   r2,                            [r2 + 4 * r3]
    vextracti32x4         [r2 + mmsize/2],               m0,                  2
    vextracti32x4         [r2 + r3 + mmsize/2],          m2,                  2
    vextracti32x4         [r2 + 2 * r3 + mmsize/2],      m0,                  3
    vextracti32x4         [r2 + r7 + mmsize/2],          m2,                  3
%endmacro

%macro FILTER_VER_SS_CHROMA_24xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_ss_24x%1, 5, 12, 18
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7
%ifdef PIC
    lea                   r5,                 [pw_ChromaCoeffVer_32_avx512]
    mova                  m16,                [r5 + r4]
    mova                  m17,                [r5 + r4 + mmsize]
%else
    lea                   r5,                 [pw_ChromaCoeffVer_32_avx512 + r4]
    mova                  m16,                [r5]
    mova                  m17,                [r5 + mmsize]
%endif
    lea                   r10,                [3 * r1]
    lea                   r7,                 [3 * r3]
%rep %1/8 - 1
    PROCESS_CHROMA_VERT_SS_24x8_AVX512
    lea                   r0,                 [r8 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_SS_24x8_AVX512
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_SS_CHROMA_24xN_AVX512 32
    FILTER_VER_SS_CHROMA_24xN_AVX512 64
%endif
%macro PROCESS_CHROMA_VERT_S_32x2_AVX512 1
    movu                  m1,                 [r0]
    movu                  m3,                 [r0 + r1]
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m7
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m7
    movu                  m4,                 [r0 + 2 * r1]
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 m7
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m7
    movu                  m5,                 [r0 + r4]
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 m8
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m8
    paddd                 m1,                 m4
    movu                  m4,                 [r0 + 4 * r1]
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 m8
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m8
    paddd                 m3,                 m5
%ifidn %1, sp
    paddd                 m0,                 m9
    paddd                 m1,                 m9
    paddd                 m2,                 m9
    paddd                 m3,                 m9

    psrad                 m0,                 12
    psrad                 m1,                 12
    psrad                 m2,                 12
    psrad                 m3,                 12

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packuswb              m0,                 m2
    vpermq                m0,                 m10,                   m0
    movu                  [r2],               ym0
    vextracti32x8         [r2 + r3],          m0,                    1
%else
    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6
    packssdw              m0,                 m1
    packssdw              m2,                 m3
    movu                  [r2],               m0
    movu                  [r2 + r3],          m2
%endif
%endmacro

%macro FILTER_VER_S_CHROMA_32xN_AVX512 2
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_32x%2, 4, 6, 11
    mov               r4d,             r4m
    shl               r4d,             7
%ifdef PIC
    lea               r5,              [pw_ChromaCoeffVer_32_avx512]
    mova              m7,              [r5 + r4]
    mova              m8,              [r5 + r4 + mmsize]
%else
    mova              m7,              [pw_ChromaCoeffVer_32_avx512 + r4]
    mova              m8,              [pw_ChromaCoeffVer_32_avx512 + r4 + mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4   m9,              [pd_526336]
    mova              m10,             [interp8_vsp_store_avx512]
%else
    add               r3d,             r3d
%endif
    add               r1d,             r1d
    sub               r0,              r1
    lea               r4,              [r1 * 3]
    lea               r5,              [r3 * 3]
%rep %2/2 - 1
    PROCESS_CHROMA_VERT_S_32x2_AVX512 %1
    lea               r0,              [r0 + r1 * 2]
    lea               r2,              [r2 + r3 * 2]
%endrep
    PROCESS_CHROMA_VERT_S_32x2_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_CHROMA_32xN_AVX512 ss, 8
    FILTER_VER_S_CHROMA_32xN_AVX512 ss, 16
    FILTER_VER_S_CHROMA_32xN_AVX512 ss, 24
    FILTER_VER_S_CHROMA_32xN_AVX512 ss, 32
    FILTER_VER_S_CHROMA_32xN_AVX512 ss, 48
    FILTER_VER_S_CHROMA_32xN_AVX512 ss, 64
    FILTER_VER_S_CHROMA_32xN_AVX512 sp, 8
    FILTER_VER_S_CHROMA_32xN_AVX512 sp, 16
    FILTER_VER_S_CHROMA_32xN_AVX512 sp, 24
    FILTER_VER_S_CHROMA_32xN_AVX512 sp, 32
    FILTER_VER_S_CHROMA_32xN_AVX512 sp, 48
    FILTER_VER_S_CHROMA_32xN_AVX512 sp, 64
%endif

%macro PROCESS_CHROMA_VERT_S_48x4_AVX512 1
    PROCESS_CHROMA_VERT_S_32x2_AVX512 %1
    lea                   r6,                 [r0 + 2 * r1]

    movu                  m1,                 [r6]
    movu                  m3,                 [r6 + r1]
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m7
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m7
    movu                  m4,                 [r6 + 2 * r1]
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 m7
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m7

    movu                  m5,                 [r6 + r4]
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 m8
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m8
    paddd                 m1,                 m4

    movu                  m4,                 [r6 + 4 * r1]
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 m8
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m8
    paddd                 m3,                 m5

%ifidn %1, sp
    paddd                 m0,                 m9
    paddd                 m1,                 m9
    paddd                 m2,                 m9
    paddd                 m3,                 m9

    psrad                 m0,                 12
    psrad                 m1,                 12
    psrad                 m2,                 12
    psrad                 m3,                 12

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packuswb              m0,                 m2
    vpermq                m0,                 m10,                   m0
    movu                  [r2 + 2 * r3],      ym0
    vextracti32x8         [r2 + r5],          m0,                    1
%else
    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    movu                  [r2 + 2 * r3],      m0
    movu                  [r2 + r5],          m2
%endif

    movu                  ym1,                [r0 + mmsize]
    vinserti32x8          m1,                 [r6 + mmsize],                1
    movu                  ym3,                [r0 + r1 + mmsize]
    vinserti32x8          m3,                 [r6 + r1 + mmsize],           1
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m7
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m7

    movu                  ym4,                [r0 + 2 * r1 + mmsize]
    vinserti32x8          m4,                 [r6 + 2 * r1 + mmsize],       1
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 m7
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m7

    movu                  ym5,                [r0 + r4 + mmsize]
    vinserti32x8          m5,                 [r6 + r4 + mmsize],           1
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 m8
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m8
    paddd                 m1,                 m4

    movu                  ym4,                [r0 + 4 * r1 + mmsize]
    vinserti32x8          m4,                 [r6 + 4 * r1 + mmsize],       1
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 m8
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m8
    paddd                 m3,                 m5

%ifidn %1, sp
    paddd                m0,                  m9
    paddd                m1,                  m9
    paddd                m2,                  m9
    paddd                m3,                  m9

    psrad                m0,                  12
    psrad                m1,                  12
    psrad                m2,                  12
    psrad                m3,                  12

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    packuswb             m0,                  m2
    vpermq               m0,                  m10,                 m0
    movu                 [r2 + mmsize/2],                xm0
    vextracti32x4        [r2 + r3 + mmsize/2],           m0,                  2
    vextracti32x4        [r2 + 2 * r3 + mmsize/2],       m0,                  1
    vextracti32x4        [r2 + r5 + mmsize/2],           m0,                  3
%else
    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6
    packssdw              m0,                 m1
    packssdw              m2,                 m3

    movu                  [r2 + mmsize],               ym0
    movu                  [r2 + r3 + mmsize],          ym2
    vextracti32x8         [r2 + 2 * r3 + mmsize],      m0,                  1
    vextracti32x8         [r2 + r5 + mmsize],          m2,                  1
%endif
%endmacro

%macro FILTER_VER_S_CHROMA_48x64_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_48x64, 4, 7, 11
    mov                   r4d,                r4m
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [pw_ChromaCoeffVer_32_avx512]
    mova                  m7,                 [r5 + r4]
    mova                  m8,                 [r5 + r4 + mmsize]
%else
    mova                  m7,                 [pw_ChromaCoeffVer_32_avx512 + r4]
    mova                  m8,                 [pw_ChromaCoeffVer_32_avx512 + r4 + mmsize]
%endif

%ifidn %1, sp
    vbroadcasti32x4       m9,                 [pd_526336]
    mova                  m10,                [interp8_vsp_store_avx512]
%else
    add                   r3d,                r3d
%endif
    add                   r1d,                r1d
    sub                   r0,                 r1
    lea                   r4,                 [r1 * 3]
    lea                   r5,                 [r3 * 3]

%rep 15
    PROCESS_CHROMA_VERT_S_48x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_S_48x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_CHROMA_48x64_AVX512 ss
    FILTER_VER_S_CHROMA_48x64_AVX512 sp
%endif

%macro PROCESS_CHROMA_VERT_S_64x2_AVX512 1
    PROCESS_CHROMA_VERT_S_32x2_AVX512 %1
    movu                  m1,                 [r0 + mmsize]
    movu                  m3,                 [r0 + r1 + mmsize]
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m7
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m7
    movu                  m4,                 [r0 + 2 * r1 + mmsize]
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 m7
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m7

    movu                  m5,                 [r0 + r4 + mmsize]
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 m8
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m8
    paddd                 m1,                 m4

    movu                  m4,                 [r0 + 4 * r1 + mmsize]
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 m8
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m8
    paddd                 m3,                 m5

%ifidn %1, sp
    paddd                 m0,                 m9
    paddd                 m1,                 m9
    paddd                 m2,                 m9
    paddd                 m3,                 m9

    psrad                 m0,                 12
    psrad                 m1,                 12
    psrad                 m2,                 12
    psrad                 m3,                 12

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packuswb              m0,                 m2
    vpermq                m0,                 m10,                   m0
    movu                  [r2 + mmsize/2],    ym0
    vextracti32x8         [r2 + r3 + mmsize/2], m0,                    1
%else
    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    movu                  [r2 + mmsize],      m0
    movu                  [r2 + r3 + mmsize], m2
%endif
%endmacro

%macro FILTER_VER_S_CHROMA_64xN_AVX512 2
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_64x%2, 4, 6, 11
    mov               r4d,             r4m
    shl               r4d,             7
%ifdef PIC
    lea               r5,              [pw_ChromaCoeffVer_32_avx512]
    mova              m7,              [r5 + r4]
    mova              m8,              [r5 + r4 + mmsize]
%else
    mova              m7,              [pw_ChromaCoeffVer_32_avx512 + r4]
    mova              m8,              [pw_ChromaCoeffVer_32_avx512 + r4 + mmsize]
%endif

%ifidn %1, sp
    vbroadcasti32x4   m9,              [pd_526336]
    mova              m10,             [interp8_vsp_store_avx512]
%else
    add               r3d,             r3d
%endif
    add               r1d,             r1d
    sub               r0,              r1
    lea               r4,              [r1 * 3]
    lea               r5,              [r3 * 3]

%rep %2/2 - 1
    PROCESS_CHROMA_VERT_S_64x2_AVX512 %1
    lea               r0,              [r0 + r1 * 2]
    lea               r2,              [r2 + r3 * 2]
%endrep
    PROCESS_CHROMA_VERT_S_64x2_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_CHROMA_64xN_AVX512 ss, 16
    FILTER_VER_S_CHROMA_64xN_AVX512 ss, 32
    FILTER_VER_S_CHROMA_64xN_AVX512 ss, 48
    FILTER_VER_S_CHROMA_64xN_AVX512 ss, 64
    FILTER_VER_S_CHROMA_64xN_AVX512 sp, 16
    FILTER_VER_S_CHROMA_64xN_AVX512 sp, 32
    FILTER_VER_S_CHROMA_64xN_AVX512 sp, 48
    FILTER_VER_S_CHROMA_64xN_AVX512 sp, 64
%endif
;-------------------------------------------------------------------------------------------------------------
;avx512 chroma_vss code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
;ipfilter_chroma_avx512 code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
;ipfilter_luma_avx512 code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_IPFILTER_LUMA_PP_64x1_AVX512 0
    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m3, m4  shuffle order table
    ; m5 - pw_1
    ; m6 - pw_512

    movu              m7,        [r0]
    movu              m9,        [r0 + 8]

    pshufb            m8,        m7,        m3
    pshufb            m7,        m2
    pshufb            m10,       m9,        m3
    pshufb            m11,       m9,        m4
    pshufb            m9,        m2


    pmaddubsw         m7,        m0
    pmaddubsw         m12,       m8,        m1
    pmaddwd           m7,        m5
    pmaddwd           m12,       m5
    paddd             m7,        m12

    pmaddubsw         m8,        m0
    pmaddubsw         m12,       m9,        m1
    pmaddwd           m8,        m5
    pmaddwd           m12,       m5
    paddd             m8,        m12

    pmaddubsw         m9,        m0
    pmaddubsw         m12,       m10,       m1
    pmaddwd           m9,        m5
    pmaddwd           m12,       m5
    paddd             m9,        m12

    pmaddubsw         m10,       m0
    pmaddubsw         m12,      m11,        m1
    pmaddwd           m10,      m5
    pmaddwd           m12,      m5
    paddd             m10,      m12

    packssdw          m7,       m8
    packssdw          m9,       m10
    pmulhrsw          m7,       m6
    pmulhrsw          m9,       m6
    packuswb          m7,       m9
    movu              [r2],     m7
%endmacro

%macro PROCESS_IPFILTER_LUMA_PP_32x2_AVX512 0
    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m3, m4  shuffle order table
    ; m5 - pw_1
    ; m6 - pw_512

    movu             ym7,        [r0]
    vinserti32x8      m7,        [r0 + r1], 1
    movu             ym9,        [r0 + 8]
    vinserti32x8      m9,        [r0 + r1 + 8], 1

    pshufb            m8,        m7,        m3
    pshufb            m7,        m2
    pshufb            m10,       m9,        m3
    pshufb            m11,       m9,        m4
    pshufb            m9,        m2

    pmaddubsw         m7,        m0
    pmaddubsw         m12,       m8,        m1
    pmaddwd           m7,        m5
    pmaddwd           m12,       m5
    paddd             m7,        m12

    pmaddubsw         m8,        m0
    pmaddubsw         m12,       m9,        m1
    pmaddwd           m8,        m5
    pmaddwd           m12,       m5
    paddd             m8,        m12

    pmaddubsw         m9,        m0
    pmaddubsw         m12,       m10,       m1
    pmaddwd           m9,        m5
    pmaddwd           m12,       m5
    paddd             m9,        m12

    pmaddubsw         m10,       m0
    pmaddubsw         m12,      m11,        m1
    pmaddwd           m10,      m5
    pmaddwd           m12,      m5
    paddd             m10,      m12

    packssdw          m7,       m8
    packssdw          m9,       m10
    pmulhrsw          m7,       m6
    pmulhrsw          m9,       m6
    packuswb          m7,       m9
    movu              [r2],     ym7
    vextracti32x8     [r2 + r3], m7, 1
%endmacro

%macro PROCESS_IPFILTER_LUMA_PP_16x4_AVX512 0
    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m3, m4  shuffle order table
    ; m5 - pw_1
    ; m6 - pw_512

    movu             xm7,        [r0]
    vinserti32x4      m7,        [r0 + r1],          1
    vinserti32x4      m7,        [r0 + 2 * r1],      2
    vinserti32x4      m7,        [r0 + r6],          3

    pshufb            m8,        m7,        m3
    pshufb            m7,        m2

    movu             xm9,        [r0 + 8]
    vinserti32x4      m9,        [r0 + r1 + 8],      1
    vinserti32x4      m9,        [r0 + 2 * r1 + 8],  2
    vinserti32x4      m9,        [r0 + r6 + 8],      3

    pshufb            m10,       m9,        m3
    pshufb            m11,       m9,        m4
    pshufb            m9,        m2

    pmaddubsw         m7,        m0
    pmaddubsw         m12,       m8,        m1
    pmaddwd           m7,        m5
    pmaddwd           m12,       m5
    paddd             m7,        m12

    pmaddubsw         m8,        m0
    pmaddubsw         m12,       m9,        m1
    pmaddwd           m8,        m5
    pmaddwd           m12,       m5
    paddd             m8,        m12

    pmaddubsw         m9,        m0
    pmaddubsw         m12,       m10,       m1
    pmaddwd           m9,        m5
    pmaddwd           m12,       m5
    paddd             m9,        m12

    pmaddubsw         m10,       m0
    pmaddubsw         m12,      m11,        m1
    pmaddwd           m10,      m5
    pmaddwd           m12,      m5
    paddd             m10,      m12

    packssdw          m7,       m8
    packssdw          m9,       m10
    pmulhrsw          m7,       m6
    pmulhrsw          m9,       m6
    packuswb          m7,       m9
    movu              [r2],         xm7
    vextracti32x4     [r2 + r3],     m7,    1
    vextracti32x4     [r2 + 2 * r3], m7,    2
    vextracti32x4     [r2 + r7],     m7,    3
%endmacro

%macro PROCESS_IPFILTER_LUMA_PP_48x4_AVX512 0
    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m3, m4  shuffle order table
    ; m5 - pw_1
    ; m6 - pw_512

    movu             ym7,        [r0]
    vinserti32x8      m7,        [r0 + r1], 1
    movu             ym9,        [r0 + 8]
    vinserti32x8      m9,        [r0 + r1 + 8], 1

    pshufb            m8,        m7,        m3
    pshufb            m7,        m2
    pshufb            m10,       m9,        m3
    pshufb            m11,       m9,        m4
    pshufb            m9,        m2

    pmaddubsw         m7,        m0
    pmaddubsw         m12,       m8,        m1
    pmaddwd           m7,        m5
    pmaddwd           m12,       m5
    paddd             m7,        m12

    pmaddubsw         m8,        m0
    pmaddubsw         m12,       m9,        m1
    pmaddwd           m8,        m5
    pmaddwd           m12,       m5
    paddd             m8,        m12

    pmaddubsw         m9,        m0
    pmaddubsw         m12,       m10,       m1
    pmaddwd           m9,        m5
    pmaddwd           m12,       m5
    paddd             m9,        m12

    pmaddubsw         m10,       m0
    pmaddubsw         m12,      m11,        m1
    pmaddwd           m10,      m5
    pmaddwd           m12,      m5
    paddd             m10,      m12

    packssdw          m7,       m8
    packssdw          m9,       m10
    pmulhrsw          m7,       m6
    pmulhrsw          m9,       m6
    packuswb          m7,       m9
    movu              [r2],     ym7
    vextracti32x8     [r2 + r3], m7, 1

    movu             ym7,        [r0 + 2 * r1]
    vinserti32x8      m7,        [r0 + r6],          1
    movu             ym9,        [r0 + 2 * r1 + 8]
    vinserti32x8      m9,        [r0 + r6 + 8],      1

    pshufb            m8,        m7,        m3
    pshufb            m7,        m2
    pshufb            m10,       m9,        m3
    pshufb            m11,       m9,        m4
    pshufb            m9,        m2

    pmaddubsw         m7,        m0
    pmaddubsw         m12,       m8,        m1
    pmaddwd           m7,        m5
    pmaddwd           m12,       m5
    paddd             m7,        m12

    pmaddubsw         m8,        m0
    pmaddubsw         m12,       m9,        m1
    pmaddwd           m8,        m5
    pmaddwd           m12,       m5
    paddd             m8,        m12

    pmaddubsw         m9,        m0
    pmaddubsw         m12,       m10,       m1
    pmaddwd           m9,        m5
    pmaddwd           m12,       m5
    paddd             m9,        m12

    pmaddubsw         m10,       m0
    pmaddubsw         m12,      m11,        m1
    pmaddwd           m10,      m5
    pmaddwd           m12,      m5
    paddd             m10,      m12

    packssdw          m7,       m8
    packssdw          m9,       m10
    pmulhrsw          m7,       m6
    pmulhrsw          m9,       m6
    packuswb          m7,       m9
    movu              [r2 + 2 * r3],     ym7
    vextracti32x8     [r2 + r7],          m7,    1

    movu             xm7,        [r0 + mmsize/2]
    vinserti32x4      m7,        [r0 + r1 + mmsize/2],          1
    vinserti32x4      m7,        [r0 + 2 * r1 + mmsize/2],      2
    vinserti32x4      m7,        [r0 + r6 + mmsize/2],          3

    pshufb            m8,        m7,        m3
    pshufb            m7,        m2

    movu             xm9,        [r0 + 40]
    vinserti32x4      m9,        [r0 + r1 + 40],      1
    vinserti32x4      m9,        [r0 + 2 * r1 + 40],  2
    vinserti32x4      m9,        [r0 + r6 + 40],      3

    pshufb            m10,       m9,        m3
    pshufb            m11,       m9,        m4
    pshufb            m9,        m2

    pmaddubsw         m7,        m0
    pmaddubsw         m12,       m8,        m1
    pmaddwd           m7,        m5
    pmaddwd           m12,       m5
    paddd             m7,        m12

    pmaddubsw         m8,        m0
    pmaddubsw         m12,       m9,        m1
    pmaddwd           m8,        m5
    pmaddwd           m12,       m5
    paddd             m8,        m12

    pmaddubsw         m9,        m0
    pmaddubsw         m12,       m10,       m1
    pmaddwd           m9,        m5
    pmaddwd           m12,       m5
    paddd             m9,        m12

    pmaddubsw         m10,       m0
    pmaddubsw         m12,      m11,        m1
    pmaddwd           m10,      m5
    pmaddwd           m12,      m5
    paddd             m10,      m12

    packssdw          m7,       m8
    packssdw          m9,       m10
    pmulhrsw          m7,       m6
    pmulhrsw          m9,       m6
    packuswb          m7,       m9
    movu              [r2 + mmsize/2],         xm7
    vextracti32x4     [r2 + r3 + mmsize/2],     m7,    1
    vextracti32x4     [r2 + 2 * r3 + mmsize/2], m7,    2
    vextracti32x4     [r2 + r7 + mmsize/2],     m7,    3
%endmacro

%macro IPFILTER_LUMA_64xN_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_pp_64x%1, 4,6,13
    sub               r0,    3
    mov               r4d,   r4m
%ifdef PIC
    lea               r5,        [tab_LumaCoeff]
    vpbroadcastd      m0,        [r5 + r4 * 8]
    vpbroadcastd      m1,        [r5 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,        [tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,        [tab_LumaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8   m2,        [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8   m3,        [interp4_horiz_shuf_load3_avx512]
    vbroadcasti32x8   m4,        [interp4_horiz_shuf_load2_avx512]
    vpbroadcastd      m5,        [pw_1]
    vbroadcasti32x8   m6,        [pw_512]

%rep %1-1
    PROCESS_IPFILTER_LUMA_PP_64x1_AVX512
    lea               r0,        [r0 + r1]
    lea               r2,        [r2 + r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_64x1_AVX512
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_LUMA_64xN_AVX512 16
IPFILTER_LUMA_64xN_AVX512 32
IPFILTER_LUMA_64xN_AVX512 48
IPFILTER_LUMA_64xN_AVX512 64
%endif

%macro IPFILTER_LUMA_32xN_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_pp_32x%1, 4,6,13
    sub               r0,    3
    mov               r4d,   r4m
%ifdef PIC
    lea               r5,        [tab_LumaCoeff]
    vpbroadcastd      m0,        [r5 + r4 * 8]
    vpbroadcastd      m1,        [r5 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,        [tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,        [tab_LumaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8   m2,        [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8   m3,        [interp4_horiz_shuf_load3_avx512]
    vbroadcasti32x8   m4,        [interp4_horiz_shuf_load2_avx512]
    vpbroadcastd      m5,        [pw_1]
    vbroadcasti32x8   m6,        [pw_512]

%rep %1/2 -1
    PROCESS_IPFILTER_LUMA_PP_32x2_AVX512
    lea               r0,        [r0 + 2 * r1]
    lea               r2,        [r2 + 2 * r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_32x2_AVX512
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_LUMA_32xN_AVX512 8
IPFILTER_LUMA_32xN_AVX512 16
IPFILTER_LUMA_32xN_AVX512 24
IPFILTER_LUMA_32xN_AVX512 32
IPFILTER_LUMA_32xN_AVX512 64
%endif

%macro IPFILTER_LUMA_16xN_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_pp_16x%1, 4,8,14
    sub               r0,    3
    mov               r4d,   r4m
    lea               r6,    [3 * r1]
    lea               r7,    [3 * r3]
%ifdef PIC
    lea               r5,        [tab_LumaCoeff]
    vpbroadcastd      m0,        [r5 + r4 * 8]
    vpbroadcastd      m1,        [r5 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,        [tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,        [tab_LumaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8   m2,        [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8   m3,        [interp4_horiz_shuf_load3_avx512]
    vbroadcasti32x8   m4,        [interp4_horiz_shuf_load2_avx512]
    vpbroadcastd      m5,        [pw_1]
    vbroadcasti32x8   m6,        [pw_512]

%rep %1/4 -1
    PROCESS_IPFILTER_LUMA_PP_16x4_AVX512
    lea               r0,        [r0 + 4 * r1]
    lea               r2,        [r2 + 4 * r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_16x4_AVX512
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_LUMA_16xN_AVX512 4
IPFILTER_LUMA_16xN_AVX512 8
IPFILTER_LUMA_16xN_AVX512 12
IPFILTER_LUMA_16xN_AVX512 16
IPFILTER_LUMA_16xN_AVX512 32
IPFILTER_LUMA_16xN_AVX512 64
%endif

%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_8tap_horiz_pp_48x64, 4,8,14
    sub               r0,    3
    mov               r4d,   r4m
    lea               r6,    [3 * r1]
    lea               r7,    [3 * r3]
%ifdef PIC
    lea               r5,        [tab_LumaCoeff]
    vpbroadcastd      m0,        [r5 + r4 * 8]
    vpbroadcastd      m1,        [r5 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,        [tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,        [tab_LumaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8   m2,        [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8   m3,        [interp4_horiz_shuf_load3_avx512]
    vbroadcasti32x8   m4,        [interp4_horiz_shuf_load2_avx512]
    vpbroadcastd      m5,        [pw_1]
    vbroadcasti32x8   m6,        [pw_512]

%rep 15
    PROCESS_IPFILTER_LUMA_PP_48x4_AVX512
    lea               r0,        [r0 + 4 * r1]
    lea               r2,        [r2 + 4 * r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_48x4_AVX512
    RET
%endif

%macro PROCESS_IPFILTER_LUMA_PS_64x1_AVX512 0
    ; register map
    ; m0 , m1     - interpolate coeff
    ; m2 , m3, m4 - load shuffle order table
    ; m5          - pw_1
    ; m6          - pw_2000
    ; m7          - store shuffle order table

    movu              ym8,           [r0]
    vinserti32x8      m8,            [r0 + 8],            1
    pshufb            m9,            m8,                  m3
    pshufb            m10,           m8,                  m4
    pshufb            m8,             m2

    movu              ym11,          [r0 + mmsize/2]
    vinserti32x8      m11,           [r0 + mmsize/2 + 8], 1
    pshufb            m12,           m11,                 m3
    pshufb            m13,           m11,                 m4
    pshufb            m11,           m2

    pmaddubsw         m8,            m0
    pmaddubsw         m14,           m9,                  m1
    pmaddwd           m8,            m5
    pmaddwd           m14,           m5
    paddd             m8,            m14

    pmaddubsw         m9,            m0
    pmaddubsw         m14,           m10,                 m1
    pmaddwd           m9,            m5
    pmaddwd           m14,           m5
    paddd             m9,            m14

    pmaddubsw         m11,           m0
    pmaddubsw         m14,           m12,                 m1
    pmaddwd           m11,           m5
    pmaddwd           m14,           m5
    paddd             m11,           m14

    pmaddubsw         m12,           m0
    pmaddubsw         m14,           m13,                 m1
    pmaddwd           m12,           m5
    pmaddwd           m14,           m5
    paddd             m12,           m14


    packssdw          m8,            m9
    packssdw          m11,           m12
    psubw             m8,            m6
    psubw             m11,           m6
    vpermq            m8,            m7,                  m8
    vpermq            m11,           m7,                  m11
    movu              [r2],          m8
    movu              [r2 + mmsize], m11
%endmacro

%macro IPFILTER_LUMA_PS_64xN_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_ps_64x%1, 4,7,15
    mov               r4d,   r4m
    mov               r5d,   r5m

%ifdef PIC
    lea               r6,        [tab_LumaCoeff]
    vpbroadcastd      m0,        [r6 + r4 * 8]
    vpbroadcastd      m1,        [r6 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,        [tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,        [tab_LumaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8   m2,        [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8   m3,        [interp4_horiz_shuf_load3_avx512]
    vbroadcasti32x8   m4,        [interp4_horiz_shuf_load2_avx512]
    vpbroadcastd      m5,        [pw_1]
    vbroadcasti32x8   m6,        [pw_2000]
    mova              m7,        [interp8_hps_store_avx512]

    mov               r4d,       %1
    sub               r0,        3
    test              r5d,       r5d
    jz                .loop
    lea               r6,        [r1 * 3]
    sub               r0,        r6                           ; r0(src)-r6
    add               r4d,       7                            ; blkheight += N - 1

.loop:
    PROCESS_IPFILTER_LUMA_PS_64x1_AVX512
    lea               r0,        [r0 + r1]
    lea               r2,        [r2 + 2 * r3]
    dec               r4d
    jnz               .loop
    RET
%endmacro

%if ARCH_X86_64 == 1
    IPFILTER_LUMA_PS_64xN_AVX512 16
    IPFILTER_LUMA_PS_64xN_AVX512 32
    IPFILTER_LUMA_PS_64xN_AVX512 48
    IPFILTER_LUMA_PS_64xN_AVX512 64
%endif

%macro PROCESS_IPFILTER_LUMA_PS_32x1_AVX512 0
    ; register map
    ; m0 , m1     - interpolate coeff
    ; m2 , m3, m4 - load shuffle order table
    ; m5          - pw_1
    ; m6          - pw_2000
    ; m7          - store shuffle order table

    movu              ym8,           [r0]
    vinserti32x8      m8,            [r0 + 8],            1
    pshufb            m9,            m8,                  m3
    pshufb            m10,           m8,                  m4
    pshufb            m8,             m2

    pmaddubsw         m8,            m0
    pmaddubsw         m11,           m9,                  m1
    pmaddwd           m8,            m5
    pmaddwd           m11,           m5
    paddd             m8,            m11

    pmaddubsw         m9,            m0
    pmaddubsw         m11,           m10,                 m1
    pmaddwd           m9,            m5
    pmaddwd           m11,           m5
    paddd             m9,            m11

    packssdw          m8,            m9
    psubw             m8,            m6
    vpermq            m8,            m7,                  m8
    movu              [r2],          m8
%endmacro

%macro IPFILTER_LUMA_PS_32xN_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_ps_32x%1, 4,7,12
    mov               r4d,   r4m
    mov               r5d,   r5m

%ifdef PIC
    lea               r6,        [tab_LumaCoeff]
    vpbroadcastd      m0,        [r6 + r4 * 8]
    vpbroadcastd      m1,        [r6 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,        [tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,        [tab_LumaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8   m2,        [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8   m3,        [interp4_horiz_shuf_load3_avx512]
    vbroadcasti32x8   m4,        [interp4_horiz_shuf_load2_avx512]
    vpbroadcastd      m5,        [pw_1]
    vbroadcasti32x8   m6,        [pw_2000]
    mova              m7,        [interp8_hps_store_avx512]

    mov               r4d,       %1
    sub               r0,        3
    test              r5d,       r5d
    jz                .loop
    lea               r6,        [r1 * 3]
    sub               r0,        r6                           ; r0(src)-r6
    add               r4d,       7                            ; blkheight += N - 1

.loop:
    PROCESS_IPFILTER_LUMA_PS_32x1_AVX512
    lea               r0,        [r0 + r1]
    lea               r2,        [r2 + 2 * r3]
    dec               r4d
    jnz               .loop
    RET
%endmacro

%if ARCH_X86_64 == 1
    IPFILTER_LUMA_PS_32xN_AVX512 8
    IPFILTER_LUMA_PS_32xN_AVX512 16
    IPFILTER_LUMA_PS_32xN_AVX512 24
    IPFILTER_LUMA_PS_32xN_AVX512 32
    IPFILTER_LUMA_PS_32xN_AVX512 64
%endif

%macro PROCESS_IPFILTER_LUMA_PS_8TAP_16x2_AVX512 0
    movu              xm7,           [r0]
    vinserti32x4      m7,            [r0 + 8],            1
    vinserti32x4      m7,            [r0 + r1],           2
    vinserti32x4      m7,            [r0 + r1 + 8],       3
    pshufb            m8,            m7,                  m3
    pshufb            m9,            m7,                  m4
    pshufb            m7,            m2

    pmaddubsw         m7,            m0
    pmaddubsw         m10,           m8,                  m1
    pmaddwd           m7,            m5
    pmaddwd           m10,           m5
    paddd             m7,            m10

    pmaddubsw         m8,            m0
    pmaddubsw         m10,           m9,                  m1
    pmaddwd           m8,            m5
    pmaddwd           m10,           m5
    paddd             m8,            m10

    packssdw          m7,            m8
    psubw             m7,            m6
    movu              [r2],          ym7
    vextracti32x8     [r2 + r3],     m7,                  1
%endmacro

%macro PROCESS_IPFILTER_LUMA_PS_8TAP_16x1_AVX512 0
    movu              xm7,            [r0]
    vinserti32x4      m7,             [r0 + 8],             1
    pshufb            ym8,            ym7,                  ym3
    pshufb            ym9,            ym7,                  ym4
    pshufb            ym7,            ym2

    pmaddubsw         ym7,            ym0
    pmaddubsw         ym10,           ym8,                  ym1
    pmaddwd           ym7,            ym5
    pmaddwd           ym10,           ym5
    paddd             ym7,            ym10

    pmaddubsw         ym8,            ym0
    pmaddubsw         ym10,           ym9,                  ym1
    pmaddwd           ym8,            ym5
    pmaddwd           ym10,           ym5
    paddd             ym8,            ym10

    packssdw          ym7,            ym8
    psubw             ym7,            ym6
    movu              [r2],           ym7
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_horiz_ps_16xN(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-------------------------------------------------------------------------------------------------------------
%macro IPFILTER_LUMA_PS_8TAP_16xN_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_ps_16x%1, 4,7,11
    mov               r4d,   r4m
    mov               r5d,   r5m
    add               r3,    r3

%ifdef PIC
    lea               r6,        [tab_LumaCoeff]
    vpbroadcastd      m0,        [r6 + r4 * 8]
    vpbroadcastd      m1,        [r6 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,        [tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,        [tab_LumaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8   m2,        [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8   m3,        [interp4_horiz_shuf_load3_avx512]
    vbroadcasti32x8   m4,        [interp4_horiz_shuf_load2_avx512]
    vpbroadcastd      m5,        [pw_1]
    vbroadcasti32x8   m6,        [pw_2000]

    ; register map
    ; m0 , m1     - interpolate coeff
    ; m2 , m3, m4 - load shuffle order table
    ; m5          - pw_1
    ; m6          - pw_2000

    mov               r4d,       %1
    sub               r0,        3
    test              r5d,       r5d
    jz                .loop
    lea               r6,        [r1 * 3]
    sub               r0,        r6                           ; r0(src)-r6
    add               r4d,       7                            ; blkheight += N - 1
    PROCESS_IPFILTER_LUMA_PS_8TAP_16x1_AVX512
    lea               r0,        [r0 + r1]
    lea               r2,        [r2 + r3]
    dec               r4d

.loop:
    PROCESS_IPFILTER_LUMA_PS_8TAP_16x2_AVX512
    lea               r0,        [r0 + 2 * r1]
    lea               r2,        [r2 + 2 * r3]
    sub               r4d,       2
    jnz               .loop
    RET
%endmacro

%if ARCH_X86_64 == 1
    IPFILTER_LUMA_PS_8TAP_16xN_AVX512 4
    IPFILTER_LUMA_PS_8TAP_16xN_AVX512 8
    IPFILTER_LUMA_PS_8TAP_16xN_AVX512 12
    IPFILTER_LUMA_PS_8TAP_16xN_AVX512 16
    IPFILTER_LUMA_PS_8TAP_16xN_AVX512 32
    IPFILTER_LUMA_PS_8TAP_16xN_AVX512 64
%endif

%macro PROCESS_IPFILTER_LUMA_PS_48x1_AVX512 0
    ; register map
    ; m0 , m1     - interpolate coeff
    ; m2 , m3, m4 - load shuffle order table
    ; m5          - pw_1
    ; m6          - pw_2000
    ; m7          - store shuffle order table

    movu              ym8,           [r0]
    vinserti32x8      m8,            [r0 + 8],            1
    pshufb            m9,            m8,                  m3
    pshufb            m10,           m8,                  m4
    pshufb            m8,             m2

    pmaddubsw         m8,            m0
    pmaddubsw         m11,           m9,                  m1
    pmaddwd           m8,            m5
    pmaddwd           m11,           m5
    paddd             m8,            m11

    pmaddubsw         m9,            m0
    pmaddubsw         m11,           m10,                 m1
    pmaddwd           m9,            m5
    pmaddwd           m11,           m5
    paddd             m9,            m11

    packssdw          m8,            m9
    psubw             m8,            m6
    vpermq            m8,            m7,                  m8
    movu              [r2],          m8

    movu              ym8,           [r0 + 32]
    vinserti32x4      m8,            [r0 + 40],           1
    pshufb            ym9,           ym8,                 ym3
    pshufb            ym10,           ym8,                ym4
    pshufb            ym8,            ym2

    pmaddubsw         ym8,            ym0
    pmaddubsw         ym11,           ym9,                ym1
    pmaddwd           ym8,            ym5
    pmaddwd           ym11,           ym5
    paddd             ym8,            ym11

    pmaddubsw         ym9,            ym0
    pmaddubsw         ym11,           ym10,               ym1
    pmaddwd           ym9,            ym5
    pmaddwd           ym11,           ym5
    paddd             ym9,            ym11

    packssdw          ym8,            ym9
    psubw             ym8,            ym6
    movu              [r2 + mmsize],  ym8
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_horiz_ps_48xN(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-------------------------------------------------------------------------------------------------------------
%macro IPFILTER_LUMA_PS_48xN_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_ps_48x%1, 4,7,12
    mov               r4d,   r4m
    mov               r5d,   r5m

%ifdef PIC
    lea               r6,        [tab_LumaCoeff]
    vpbroadcastd      m0,        [r6 + r4 * 8]
    vpbroadcastd      m1,        [r6 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,        [tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,        [tab_LumaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8   m2,        [interp4_horiz_shuf_load1_avx512]
    vbroadcasti32x8   m3,        [interp4_horiz_shuf_load3_avx512]
    vbroadcasti32x8   m4,        [interp4_horiz_shuf_load2_avx512]
    vpbroadcastd      m5,        [pw_1]
    vbroadcasti32x8   m6,        [pw_2000]
    mova              m7,        [interp8_hps_store_avx512]

    mov               r4d,       %1
    sub               r0,        3
    test              r5d,       r5d
    jz                .loop
    lea               r6,        [r1 * 3]
    sub               r0,        r6                           ; r0(src)-r6
    add               r4d,       7                            ; blkheight += N - 1

.loop:
    PROCESS_IPFILTER_LUMA_PS_48x1_AVX512
    lea               r0,        [r0 + r1]
    lea               r2,        [r2 + 2 * r3]
    dec               r4d
    jnz               .loop
    RET
%endmacro

%if ARCH_X86_64 == 1
    IPFILTER_LUMA_PS_48xN_AVX512 64
%endif

;-------------------------------------------------------------------------------------------------------------
;avx512 luma_vss code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_LUMA_VERT_SS_8x8_AVX512 0
    lea                  r6,                  [r0 + 4 * r1]
    movu                 xm1,                 [r0]                           ;0 row
    vinserti32x4         m1,                  [r0 + 2 * r1],          1
    vinserti32x4         m1,                  [r0 + 4 * r1],          2
    vinserti32x4         m1,                  [r6 + 2 * r1],          3
    movu                 xm3,                 [r0 + r1]                      ;1 row
    vinserti32x4         m3,                  [r0 + r7],              1
    vinserti32x4         m3,                  [r6 + r1],              2
    vinserti32x4         m3,                  [r6 + r7],              3
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 xm4,                 [r0 + 2 * r1]                  ;2 row
    vinserti32x4         m4,                  [r0 + 4 * r1],          1
    vinserti32x4         m4,                  [r6 + 2 * r1],          2
    vinserti32x4         m4,                  [r6 + 4 * r1],          3
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    lea                  r4,                  [r6 + 4 * r1]
    movu                 xm5,                 [r0 + r7]                      ;3 row
    vinserti32x4         m5,                  [r6 + r1],              1
    vinserti32x4         m5,                  [r6 + r7],              2
    vinserti32x4         m5,                  [r4 + r1],              3
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 xm4,                 [r0 + 4 * r1]                  ;4 row
    vinserti32x4         m4,                  [r6 + 2 * r1],              1
    vinserti32x4         m4,                  [r6 + 4 * r1],              2
    vinserti32x4         m4,                  [r4 + 2 * r1],              3
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 xm11,                [r6 + r1]                      ;5 row
    vinserti32x4         m11,                 [r6 + r7],              1
    vinserti32x4         m11,                 [r4 + r1],              2
    vinserti32x4         m11,                 [r4 + r7],              3
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 xm12,                [r6 + 2 * r1]                  ;6 row
    vinserti32x4         m12,                 [r6 + 4 * r1],          1
    vinserti32x4         m12,                 [r4 + 2 * r1],          2
    vinserti32x4         m12,                 [r4 + 4 * r1],          3
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    lea                  r8,                  [r4 + 4 * r1]
    movu                 xm13,                [r6 + r7]                      ;7 row
    vinserti32x4         m13,                 [r4 + r1],              1
    vinserti32x4         m13,                 [r4 + r7],              2
    vinserti32x4         m13,                 [r8 + r1],              3
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 xm12,                [r6 + 4 * r1]                 ; 8 row
    vinserti32x4         m12,                 [r4 + 2 * r1],          1
    vinserti32x4         m12,                 [r4 + 4 * r1],          2
    vinserti32x4         m12,                 [r8 + 2 * r1],          3
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3

    movu                 [r2],                xm0
    movu                 [r2 + r3],           xm2
    vextracti32x4        [r2 + 2 * r3],       m0,                  1
    vextracti32x4        [r2 + r5],           m2,                  1
    lea                  r2,                  [r2 + 4 * r3]
    vextracti32x4        [r2],                m0,                  2
    vextracti32x4        [r2 + r3],           m2,                  2
    vextracti32x4        [r2 + 2 * r3],       m0,                  3
    vextracti32x4        [r2 + r5],           m2,                  3
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_SS_LUMA_8xN_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_vert_ss_8x%1, 5, 9, 19
    add                   r1d,                r1d
    add                   r3d,                r3d
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [pw_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [pw_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif

    lea                   r5,                 [3 * r3]
%rep %1/8 - 1
    PROCESS_LUMA_VERT_SS_8x8_AVX512
    lea                   r0,                 [r4]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_SS_8x8_AVX512
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_SS_LUMA_8xN_AVX512 8
    FILTER_VER_SS_LUMA_8xN_AVX512 16
    FILTER_VER_SS_LUMA_8xN_AVX512 32
%endif
%macro PROCESS_LUMA_VERT_S_16x4_AVX512 1
    movu                 ym1,                 [r0]
    movu                 ym3,                 [r0 + r1]
    vinserti32x8         m1,                  [r0 + 2 * r1],          1
    vinserti32x8         m3,                  [r0 + r7],              1
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    lea                  r6,                  [r0 + 4 * r1]
    movu                 ym4,                 [r0 + 2 * r1]
    vinserti32x8         m4,                  [r6],                   1
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 ym5,                 [r0 + r7]
    vinserti32x8         m5,                  [r6 + r1],              1
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 ym4,                 [r6]
    vinserti32x8         m4,                  [r6 + 2 * r1],          1
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 ym11,                [r6 + r1]
    vinserti32x8         m11,                 [r6 + r7],              1
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 ym12,                [r6 + 2 * r1]
    vinserti32x8         m12,                 [r6 + 4 * r1],          1
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    lea                  r4,                  [r6 + 4 * r1]
    movu                 ym13,                [r6 + r7]
    vinserti32x8         m13,                 [r4 + r1],              1
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 ym12,                [r6 + 4 * r1]
    vinserti32x8         m12,                 [r4 + 2 * r1],          1
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11
%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  12
    psrad                m1,                  12
    psrad                m2,                  12
    psrad                m3,                  12

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    packuswb             m0,                  m2
    vpermq               m0,                  m20,                   m0
    movu                 [r2],                xm0
    vextracti32x4        [r2 + r3],           m0,                    2
    vextracti32x4        [r2 + 2 * r3],       m0,                    1
    vextracti32x4        [r2 + r5],           m0,                    3
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3

    movu                 [r2],                ym0
    movu                 [r2 + r3],           ym2
    vextracti32x8        [r2 + 2 * r3],       m0,                1
    vextracti32x8        [r2 + r5],           m2,                1
%endif
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_LUMA_16xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_16x%2, 5, 8, 21
    add                   r1d,                r1d
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [pw_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [pw_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m19,                [pd_526336]
    mova                  m20,                [interp8_vsp_store_avx512]
%else
    add                   r3d,                r3d
%endif

    lea                   r5,                 [3 * r3]
%rep %2/4 - 1
    PROCESS_LUMA_VERT_S_16x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_S_16x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_LUMA_16xN_AVX512 ss, 4
    FILTER_VER_S_LUMA_16xN_AVX512 ss, 8
    FILTER_VER_S_LUMA_16xN_AVX512 ss, 12
    FILTER_VER_S_LUMA_16xN_AVX512 ss, 16
    FILTER_VER_S_LUMA_16xN_AVX512 ss, 32
    FILTER_VER_S_LUMA_16xN_AVX512 ss, 64
    FILTER_VER_S_LUMA_16xN_AVX512 sp, 4
    FILTER_VER_S_LUMA_16xN_AVX512 sp, 8
    FILTER_VER_S_LUMA_16xN_AVX512 sp, 12
    FILTER_VER_S_LUMA_16xN_AVX512 sp, 16
    FILTER_VER_S_LUMA_16xN_AVX512 sp, 32
    FILTER_VER_S_LUMA_16xN_AVX512 sp, 64
%endif
%macro PROCESS_LUMA_VERT_SS_24x8_AVX512 0
    PROCESS_LUMA_VERT_S_16x4_AVX512 ss
    lea                  r4,                  [r6 + 4 * r1]
    lea                  r8,                  [r4 + 4 * r1]
    movu                 ym1,                 [r6]
    movu                 ym3,                 [r6 + r1]
    vinserti32x8         m1,                  [r6 + 2 * r1],          1
    vinserti32x8         m3,                  [r6 + r7],              1
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 ym4,                 [r6 + 2 * r1]
    vinserti32x8         m4,                  [r4],                   1
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 ym5,                 [r6 + r7]
    vinserti32x8         m5,                  [r4 + r1],              1
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 ym4,                 [r4]
    vinserti32x8         m4,                  [r4 + 2 * r1],          1
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 ym11,                [r4 + r1]
    vinserti32x8         m11,                 [r4 + r7],              1
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 ym12,                [r4 + 2 * r1]
    vinserti32x8         m12,                 [r4 + 4 * r1],          1
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 ym13,                [r4 + r7]
    vinserti32x8         m13,                 [r8 + r1],              1
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 ym12,                [r4 + 4 * r1]
    vinserti32x8         m12,                 [r8 + 2 * r1],          1
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3

    lea                  r9,                  [r2 + 4 * r3]
    movu                 [r9],                ym0
    movu                 [r9 + r3],           ym2
    vextracti32x8        [r9 + 2 * r3],       m0,                1
    vextracti32x8        [r9 + r5],           m2,                1

    movu                 xm1,                 [r0 + mmsize/2]
    vinserti32x4         m1,                  [r0 + 2 * r1 + mmsize/2],          1
    vinserti32x4         m1,                  [r0 + 4 * r1 + mmsize/2],          2
    vinserti32x4         m1,                  [r6 + 2 * r1 + mmsize/2],          3
    movu                 xm3,                 [r0 + r1 + mmsize/2]
    vinserti32x4         m3,                  [r0 + r7 + mmsize/2],              1
    vinserti32x4         m3,                  [r6 + r1 + mmsize/2],              2
    vinserti32x4         m3,                  [r6 + r7 + mmsize/2],              3
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 xm4,                 [r0 + 2 * r1 + mmsize/2]
    vinserti32x4         m4,                  [r0 + 4 * r1 + mmsize/2],          1
    vinserti32x4         m4,                  [r6 + 2 * r1 + mmsize/2],          2
    vinserti32x4         m4,                  [r6 + 4 * r1 + mmsize/2],          3
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 xm5,                 [r0 + r7 + mmsize/2]
    vinserti32x4         m5,                  [r6 + r1 + mmsize/2],              1
    vinserti32x4         m5,                  [r6 + r7 + mmsize/2],              2
    vinserti32x4         m5,                  [r4 + r1 + mmsize/2],              3
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 xm4,                 [r0 + 4 * r1 + mmsize/2]
    vinserti32x4         m4,                  [r6 + 2 * r1 + mmsize/2],              1
    vinserti32x4         m4,                  [r6 + 4 * r1 + mmsize/2],              2
    vinserti32x4         m4,                  [r4 + 2 * r1 + mmsize/2],              3
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 xm11,                [r6 + r1 + mmsize/2]
    vinserti32x4         m11,                 [r6 + r7 + mmsize/2],              1
    vinserti32x4         m11,                 [r4 + r1 + mmsize/2],              2
    vinserti32x4         m11,                 [r4 + r7 + mmsize/2],              3
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 xm12,                [r6 + 2 * r1 + mmsize/2]
    vinserti32x4         m12,                 [r6 + 4 * r1 + mmsize/2],          1
    vinserti32x4         m12,                 [r4 + 2 * r1 + mmsize/2],          2
    vinserti32x4         m12,                 [r4 + 4 * r1 + mmsize/2],          3
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 xm13,                [r6 + r7 + mmsize/2]
    vinserti32x4         m13,                 [r4 + r1 + mmsize/2],              1
    vinserti32x4         m13,                 [r4 + r7 + mmsize/2],              2
    vinserti32x4         m13,                 [r8 + r1 + mmsize/2],              3
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 xm12,                [r6 + 4 * r1 + mmsize/2]
    vinserti32x4         m12,                 [r4 + 2 * r1 + mmsize/2],          1
    vinserti32x4         m12,                 [r4 + 4 * r1 + mmsize/2],          2
    vinserti32x4         m12,                 [r8 + 2 * r1 + mmsize/2],          3
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3

    movu                 [r2 + mmsize/2],                xm0
    movu                 [r2 + r3 + mmsize/2],           xm2
    vextracti32x4        [r2 + 2 * r3 + mmsize/2],       m0,                  1
    vextracti32x4        [r2 + r5 + mmsize/2],           m2,                  1
    lea                  r2,                             [r2 + 4 * r3]
    vextracti32x4        [r2 + mmsize/2],                m0,                  2
    vextracti32x4        [r2 + r3 + mmsize/2],           m2,                  2
    vextracti32x4        [r2 + 2 * r3 + mmsize/2],       m0,                  3
    vextracti32x4        [r2 + r5 + mmsize/2],           m2,                  3
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_8tap_vert_ss_24x32, 5, 10, 19
    add                   r1d,                r1d
    add                   r3d,                r3d
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [pw_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [pw_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif

    lea                   r5,                 [3 * r3]
%rep 3
    PROCESS_LUMA_VERT_SS_24x8_AVX512
    lea                   r0,                 [r4]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_SS_24x8_AVX512
    RET
%endif

%macro PROCESS_LUMA_VERT_S_32x2_AVX512 1
    movu                 m1,                  [r0]                           ;0 row
    movu                 m3,                  [r0 + r1]                      ;1 row
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 m4,                  [r0 + 2 * r1]                  ;2 row
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 m5,                  [r0 + r7]                      ;3 row
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 m4,                  [r0 + 4 * r1]                  ;4 row
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    lea                  r6,                  [r0 + 4 * r1]

    movu                 m11,                 [r6 + r1]                      ;5 row
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 m12,                 [r6 + 2 * r1]                  ;6 row
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 m13,                 [r6 + r7]                      ;7 row
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18
    paddd                m8,                  m14
    paddd                m4,                  m12
    movu                 m12,                 [r6 + 4 * r1]                 ; 8 row
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18
    paddd                m10,                 m14
    paddd                m11,                 m13

    paddd                m0,                  m8
    paddd                m1,                  m4
    paddd                m2,                  m10
    paddd                m3,                  m11
%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  12
    psrad                m1,                  12
    psrad                m2,                  12
    psrad                m3,                  12

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    packuswb             m0,                  m2
    vpermq               m0,                  m20,                   m0
    movu                 [r2],                ym0
    vextracti32x8        [r2 + r3],           m0,                    1
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    movu                 [r2],                m0
    movu                 [r2 + r3],           m2
%endif
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_LUMA_32xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_32x%2, 5, 8, 21
    add                   r1d,                r1d
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [pw_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [pw_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m19,                [pd_526336]
    mova                  m20,                [interp8_vsp_store_avx512]
%else
    add                   r3d,                r3d
%endif

%rep %2/2 - 1
    PROCESS_LUMA_VERT_S_32x2_AVX512 %1
    lea                   r0,                 [r0 + 2 * r1]
    lea                   r2,                 [r2 + 2 * r3]
%endrep
    PROCESS_LUMA_VERT_S_32x2_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_LUMA_32xN_AVX512 ss, 8
    FILTER_VER_S_LUMA_32xN_AVX512 ss, 16
    FILTER_VER_S_LUMA_32xN_AVX512 ss, 32
    FILTER_VER_S_LUMA_32xN_AVX512 ss, 24
    FILTER_VER_S_LUMA_32xN_AVX512 ss, 64
    FILTER_VER_S_LUMA_32xN_AVX512 sp, 8
    FILTER_VER_S_LUMA_32xN_AVX512 sp, 16
    FILTER_VER_S_LUMA_32xN_AVX512 sp, 32
    FILTER_VER_S_LUMA_32xN_AVX512 sp, 24
    FILTER_VER_S_LUMA_32xN_AVX512 sp, 64
%endif

%macro PROCESS_LUMA_VERT_S_48x4_AVX512 1
    PROCESS_LUMA_VERT_S_32x2_AVX512 %1
    movu                 m1,                  [r0 + 2 * r1]
    movu                 m3,                  [r0 + r7]
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 m4,                  [r0 + 4 * r1]
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 m5,                  [r6 + r1]
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    lea                  r4,                  [r6 + 4 * r1]

    movu                 m4,                  [r6 + 2 * r1]
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 m11,                 [r6 + r7]
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 m12,                 [r4]
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 m13,                 [r4 + r1]
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18
    paddd                m8,                  m14
    paddd                m4,                  m12
    movu                 m12,                 [r4 + 2 * r1]
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18
    paddd                m10,                 m14
    paddd                m11,                 m13

    paddd                m0,                  m8
    paddd                m1,                  m4
    paddd                m2,                  m10
    paddd                m3,                  m11
%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  12
    psrad                m1,                  12
    psrad                m2,                  12
    psrad                m3,                  12

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    packuswb             m0,                  m2
    vpermq               m0,                  m20,                   m0
    movu                 [r2 + 2 * r3],       ym0
    vextracti32x8        [r2 + r5],           m0,                    1
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    movu                 [r2 + 2 * r3],       m0
    movu                 [r2 + r5],           m2
%endif
    movu                 ym1,                 [r0 + mmsize]
    movu                 ym3,                 [r0 + r1 + mmsize]
    vinserti32x8         m1,                  [r0 + 2 * r1 + mmsize], 1
    vinserti32x8         m3,                  [r0 + r7 + mmsize],     1
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 ym4,                 [r0 + 2 * r1 + mmsize]
    vinserti32x8         m4,                  [r6 + mmsize],          1
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 ym5,                 [r0 + r7 + mmsize]
    vinserti32x8         m5,                  [r6 + r1 + mmsize],     1
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 ym4,                 [r6 + mmsize]
    vinserti32x8         m4,                  [r6 + 2 * r1 + mmsize], 1
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 ym11,                [r6 + r1 + mmsize]
    vinserti32x8         m11,                 [r6 + r7 + mmsize],     1
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 ym12,                [r6 + 2 * r1 + mmsize]
    vinserti32x8         m12,                 [r6 + 4 * r1 + mmsize], 1
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 ym13,                [r6 + r7 + mmsize]
    vinserti32x8         m13,                 [r4 + r1 + mmsize],     1
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18
    paddd                m8,                  m14
    paddd                m4,                  m12
    movu                 ym12,                [r6 + 4 * r1 + mmsize]
    vinserti32x8         m12,                 [r4 + 2 * r1 + mmsize], 1
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18
    paddd                m10,                 m14
    paddd                m11,                 m13

    paddd                m0,                  m8
    paddd                m1,                  m4
    paddd                m2,                  m10
    paddd                m3,                  m11
%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  12
    psrad                m1,                  12
    psrad                m2,                  12
    psrad                m3,                  12

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    packuswb             m0,                  m2
    vpermq               m0,                  m20,                   m0
    movu                 [r2 + mmsize/2],                xm0
    vextracti32x4        [r2 + r3 + mmsize/2], m0,                    2
    vextracti32x4        [r2 + 2 * r3 + mmsize/2],       m0,          1
    vextracti32x4        [r2 + r5 + mmsize/2],           m0,          3
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3

    movu                 [r2 + mmsize],                ym0
    movu                 [r2 + r3 + mmsize],           ym2
    vextracti32x8        [r2 + 2 * r3 + mmsize],       m0,                1
    vextracti32x8        [r2 + r5 + mmsize],           m2,                1
%endif
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_LUMA_48x64_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_48x64, 5, 8, 21
    add                   r1d,                r1d
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [pw_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [pw_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m19,                [pd_526336]
    mova                  m20,                [interp8_vsp_store_avx512]
%else
    add                   r3d,                r3d
%endif

    lea                   r5,                 [3 * r3]
%rep 15
    PROCESS_LUMA_VERT_S_48x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_S_48x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_LUMA_48x64_AVX512 ss
    FILTER_VER_S_LUMA_48x64_AVX512 sp
%endif

%macro PROCESS_LUMA_VERT_S_64x2_AVX512 1
    PROCESS_LUMA_VERT_S_32x2_AVX512 %1
    movu                 m1,                  [r0 + mmsize]                  ;0 row
    movu                 m3,                  [r0 + r1 + mmsize]             ;1 row
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 m4,                  [r0 + 2 * r1 + mmsize]         ;2 row
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 m5,                  [r0 + r7 + mmsize]             ;3 row
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 m4,                  [r0 + 4 * r1 + mmsize]         ;4 row
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 m11,                 [r6 + r1 + mmsize]             ;5 row
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 m12,                 [r6 + 2 * r1 + mmsize]         ;6 row
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 m13,                 [r6 + r7 + mmsize]             ;7 row
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18
    paddd                m8,                  m14
    paddd                m4,                  m12
    movu                 m12,                 [r6 + 4 * r1 + mmsize]         ; 8 row
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18
    paddd                m10,                 m14
    paddd                m11,                 m13

    paddd                m0,                  m8
    paddd                m1,                  m4
    paddd                m2,                  m10
    paddd                m3,                  m11
%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  12
    psrad                m1,                  12
    psrad                m2,                  12
    psrad                m3,                  12

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    packuswb             m0,                  m2
    vpermq               m0,                  m20,                   m0
    movu                 [r2 + mmsize/2],     ym0
    vextracti32x8        [r2 + r3 + mmsize/2], m0,                    1
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6
    packssdw             m0,                  m1
    packssdw             m2,                  m3
    movu                 [r2 + mmsize],       m0
    movu                 [r2 + r3 + mmsize],  m2
%endif
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_LUMA_64xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_64x%2, 5, 8, 21
    add                   r1d,                r1d
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [pw_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [pw_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m19,                [pd_526336]
    mova                  m20,                [interp8_vsp_store_avx512]
%else
    add                   r3d,                r3d
%endif

%rep %2/2 - 1
    PROCESS_LUMA_VERT_S_64x2_AVX512 %1
    lea                   r0,                 [r0 + 2 * r1]
    lea                   r2,                 [r2 + 2 * r3]
%endrep
    PROCESS_LUMA_VERT_S_64x2_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_LUMA_64xN_AVX512 ss, 16
    FILTER_VER_S_LUMA_64xN_AVX512 ss, 32
    FILTER_VER_S_LUMA_64xN_AVX512 ss, 48
    FILTER_VER_S_LUMA_64xN_AVX512 ss, 64
    FILTER_VER_S_LUMA_64xN_AVX512 sp, 16
    FILTER_VER_S_LUMA_64xN_AVX512 sp, 32
    FILTER_VER_S_LUMA_64xN_AVX512 sp, 48
    FILTER_VER_S_LUMA_64xN_AVX512 sp, 64
%endif
;-------------------------------------------------------------------------------------------------------------
;avx512 luma_vss code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
;avx512 luma_vpp and luma_vps code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_LUMA_VERT_16x8_AVX512 1
    lea                   r5,                 [r0 + 4 * r1]
    lea                   r4,                 [r5 + 4 * r1]
    movu                  xm1,                [r0]
    vinserti32x4          m1,                 [r0 + 2 * r1],       1
    vinserti32x4          m1,                 [r5],                2
    vinserti32x4          m1,                 [r5 + 2 * r1],       3
    movu                  xm3,                [r0 + r1]
    vinserti32x4          m3,                 [r0 + r6],           1
    vinserti32x4          m3,                 [r5 + r1],           2
    vinserti32x4          m3,                 [r5 + r6],           3
    punpcklbw             m0,                 m1,                  m3
    pmaddubsw             m0,                 m8
    punpckhbw             m1,                 m3
    pmaddubsw             m1,                 m8

    movu                  xm4,                [r0 + 2 * r1]
    vinserti32x4          m4,                 [r0 + 4 * r1],       1
    vinserti32x4          m4,                 [r5 + 2 * r1],       2
    vinserti32x4          m4,                 [r5 + 4 * r1],       3
    punpcklbw             m2,                 m3,                  m4
    pmaddubsw             m2,                 m8
    punpckhbw             m3,                 m4
    pmaddubsw             m3,                 m8

    movu                  xm5,                [r0 + r6]
    vinserti32x4          m5,                 [r5 + r1],           1
    vinserti32x4          m5,                 [r5 + r6],           2
    vinserti32x4          m5,                 [r4 + r1],           3
    punpcklbw             m6,                 m4,                  m5
    pmaddubsw             m6,                 m9
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m9

    paddw                 m0,                 m6
    paddw                 m1,                 m4

    movu                  xm4,                [r0 + 4 * r1]
    vinserti32x4          m4,                 [r5 + 2 * r1],       1
    vinserti32x4          m4,                 [r5 + 4 * r1],       2
    vinserti32x4          m4,                 [r4 + 2 * r1],       3
    punpcklbw             m6,                 m5,                  m4
    pmaddubsw             m6,                 m9
    punpckhbw             m5,                 m4
    pmaddubsw             m5,                 m9

    paddw                 m2,                 m6
    paddw                 m3,                 m5

    movu                  xm15,               [r5 + r1]
    vinserti32x4          m15,                [r5 + r6],           1
    vinserti32x4          m15,                [r4 + r1],           2
    vinserti32x4          m15,                [r4 + r6],           3
    punpcklbw             m12,                m4,                 m15
    pmaddubsw             m12,                m10
    punpckhbw             m13,                m4,                 m15
    pmaddubsw             m13,                m10

    lea                   r8,                 [r4 + 4 * r1]
    movu                  xm4,                [r5 + 2 * r1]
    vinserti32x4          m4,                 [r5 + 4 * r1],       1
    vinserti32x4          m4,                 [r4 + 2 * r1],       2
    vinserti32x4          m4,                 [r4 + 4 * r1],       3
    punpcklbw             m14,                m15,                 m4
    pmaddubsw             m14,                m10
    punpckhbw             m15,                m4
    pmaddubsw             m15,                m10

    movu                  xm5,                [r5 + r6]
    vinserti32x4          m5,                 [r4 + r1],           1
    vinserti32x4          m5,                 [r4 + r6],           2
    vinserti32x4          m5,                 [r8 + r1],           3
    punpcklbw             m6,                 m4,                  m5
    pmaddubsw             m6,                 m11
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m11

    paddw                 m12,                m6
    paddw                 m13,                m4

    movu                  xm4,                [r5 + 4 * r1]
    vinserti32x4          m4,                 [r4 + 2 * r1],       1
    vinserti32x4          m4,                 [r4 + 4 * r1],       2
    vinserti32x4          m4,                 [r8 + 2 * r1],       3
    punpcklbw             m6,                 m5,                  m4
    pmaddubsw             m6,                 m11
    punpckhbw             m5,                 m4
    pmaddubsw             m5,                 m11

    paddw                 m14,                m6
    paddw                 m15,                m5

    paddw                 m0,                 m12
    paddw                 m1,                 m13
    paddw                 m2,                 m14
    paddw                 m3,                 m15
%ifidn %1,pp
    pmulhrsw              m0,                 m7
    pmulhrsw              m1,                 m7
    pmulhrsw              m2,                 m7
    pmulhrsw              m3,                 m7

    packuswb              m0,                 m1
    packuswb              m2,                 m3
    movu                  [r2],               xm0
    movu                  [r2 + r3],          xm2
    vextracti32x4         [r2 + 2 * r3],      m0,                  1
    vextracti32x4         [r2 + r7],          m2,                  1
    lea                   r2,                 [r2 + 4 * r3]
    vextracti32x4         [r2],               m0,                  2
    vextracti32x4         [r2 + r3],          m2,                  2
    vextracti32x4         [r2 + 2 * r3],      m0,                  3
    vextracti32x4         [r2 + r7],          m2,                  3
%else
    psubw                 m0,                 m7
    psubw                 m1,                 m7
    mova                  m12,                 m16
    mova                  m13,                 m17
    vpermi2q              m12,                 m0,                m1
    vpermi2q              m13,                 m0,                m1
    movu                  [r2],               ym12
    vextracti32x8         [r2 + 2 * r3],      m12,                 1

    psubw                 m2,                 m7
    psubw                 m3,                 m7
    mova                  m14,                 m16
    mova                  m15,                 m17
    vpermi2q              m14,                 m2,                m3
    vpermi2q              m15,                 m2,                m3
    movu                  [r2 + r3],          ym14
    vextracti32x8         [r2 + r7],          m14,                 1
    lea                   r2,                 [r2 + 4 * r3]

    movu                  [r2],               ym13
    movu                  [r2 + r3],          ym15
    vextracti32x8         [r2 + 2 * r3],      m13,                 1
    vextracti32x8         [r2 + r7],          m15,                 1
%endif
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VERT_LUMA_16xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_16x%2, 5, 9, 18
    mov                   r4d,                r4m
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_32_avx512]
    mova                  m8,                 [r5 + r4]
    mova                  m9,                 [r5 + r4 + 1 * mmsize]
    mova                  m10,                [r5 + r4 + 2 * mmsize]
    mova                  m11,                [r5 + r4 + 3 * mmsize]
%else
    mova                  m8,                 [tab_LumaCoeffVer_32_avx512 + r4]
    mova                  m9,                 [tab_LumaCoeffVer_32_avx512 + r4 + 1 * mmsize]
    mova                  m10,                [tab_LumaCoeffVer_32_avx512 + r4 + 2 * mmsize]
    mova                  m11,                [tab_LumaCoeffVer_32_avx512 + r4 + 3 * mmsize]
%endif
%ifidn %1, pp
    vbroadcasti32x8       m7,                 [pw_512]
%else
    shl                   r3d,                1
    vbroadcasti32x8       m7,                 [pw_2000]
    mova                  m16,                [interp4_vps_store1_avx512]
    mova                  m17,                [interp4_vps_store2_avx512]
%endif

    lea                   r6,                 [3 * r1]
    lea                   r7,                 [3 * r3]
    sub                   r0,                 r6

%rep %2/8 - 1
    PROCESS_LUMA_VERT_16x8_AVX512 %1
    lea                   r0,                 [r4]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_16x8_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VERT_LUMA_16xN_AVX512 pp, 8
    FILTER_VERT_LUMA_16xN_AVX512 pp, 16
    FILTER_VERT_LUMA_16xN_AVX512 pp, 32
    FILTER_VERT_LUMA_16xN_AVX512 pp, 64

    FILTER_VERT_LUMA_16xN_AVX512 ps, 8
    FILTER_VERT_LUMA_16xN_AVX512 ps, 16
    FILTER_VERT_LUMA_16xN_AVX512 ps, 32
    FILTER_VERT_LUMA_16xN_AVX512 ps, 64
%endif
%macro PROCESS_LUMA_VERT_32x4_AVX512 1
    lea                   r5,                 [r0 + 4 * r1]
    movu                  ym1,                [r0]
    vinserti32x8          m1,                 [r0 + 2 * r1],       1
    movu                  ym3,                [r0 + r1]
    vinserti32x8          m3,                 [r0 + r6],           1
    punpcklbw             m0,                 m1,                  m3
    pmaddubsw             m0,                 m8
    punpckhbw             m1,                 m3
    pmaddubsw             m1,                 m8

    movu                  ym4,                [r0 + 2 * r1]
    vinserti32x8          m4,                 [r0 + 4 * r1],       1
    punpcklbw             m2,                 m3,                  m4
    pmaddubsw             m2,                 m8
    punpckhbw             m3,                 m4
    pmaddubsw             m3,                 m8

    movu                  ym5,                [r0 + r6]
    vinserti32x8          m5,                 [r5 + r1],           1
    punpcklbw             m6,                 m4,                  m5
    pmaddubsw             m6,                 m9
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m9

    paddw                 m0,                 m6
    paddw                 m1,                 m4

    movu                  ym4,                [r0 + 4 * r1]
    vinserti32x8          m4,                 [r5 + 2 * r1],       1
    punpcklbw             m6,                 m5,                  m4
    pmaddubsw             m6,                 m9
    punpckhbw             m5,                 m4
    pmaddubsw             m5,                 m9

    paddw                 m2,                 m6
    paddw                 m3,                 m5

    lea                   r4,                 [r5 + 4 * r1]
    movu                  ym15,               [r5 + r1]
    vinserti32x8          m15,                [r5 + r6],           1
    punpcklbw             m12,                m4,                 m15
    pmaddubsw             m12,                m10
    punpckhbw             m13,                m4,                 m15
    pmaddubsw             m13,                m10

    movu                  ym4,                [r5 + 2 * r1]
    vinserti32x8          m4,                 [r5 + 4 * r1],       1
    punpcklbw             m14,                m15,                 m4
    pmaddubsw             m14,                m10
    punpckhbw             m15,                m4
    pmaddubsw             m15,                m10

    movu                  ym5,                [r5 + r6]
    vinserti32x8          m5,                 [r4 + r1],           1
    punpcklbw             m6,                 m4,                  m5
    pmaddubsw             m6,                 m11
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m11

    paddw                 m12,                m6
    paddw                 m13,                m4

    movu                  ym4,                [r5 + 4 * r1]
    vinserti32x8          m4,                 [r4 + 2 * r1],       1
    punpcklbw             m6,                 m5,                  m4
    pmaddubsw             m6,                 m11
    punpckhbw             m5,                 m4
    pmaddubsw             m5,                 m11

    paddw                 m14,                m6
    paddw                 m15,                m5

    paddw                 m0,                 m12
    paddw                 m1,                 m13
    paddw                 m2,                 m14
    paddw                 m3,                 m15
%ifidn %1,pp
    pmulhrsw              m0,                 m7
    pmulhrsw              m1,                 m7
    pmulhrsw              m2,                 m7
    pmulhrsw              m3,                 m7

    packuswb              m0,                 m1
    packuswb              m2,                 m3
    movu                  [r2],               ym0
    movu                  [r2 + r3],          ym2
    vextracti32x8         [r2 + 2 * r3],      m0,                  1
    vextracti32x8         [r2 + r7],          m2,                  1
%else
    psubw                 m0,                 m7
    psubw                 m1,                 m7
    mova                  m12,                 m16
    mova                  m13,                 m17
    vpermi2q              m12,                 m0,                m1
    vpermi2q              m13,                 m0,                m1
    movu                  [r2],               m12
    movu                  [r2 + 2 * r3],      m13

    psubw                 m2,                 m7
    psubw                 m3,                 m7
    mova                  m14,                 m16
    mova                  m15,                 m17
    vpermi2q              m14,                 m2,                m3
    vpermi2q              m15,                 m2,                m3
    movu                  [r2 + r3],          m14
    movu                  [r2 + r7],          m15
%endif
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VERT_LUMA_32xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_32x%2, 5, 8, 18
    mov                   r4d,                r4m
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_32_avx512]
    mova                  m8,                 [r5 + r4]
    mova                  m9,                 [r5 + r4 + 1 * mmsize]
    mova                  m10,                [r5 + r4 + 2 * mmsize]
    mova                  m11,                [r5 + r4 + 3 * mmsize]
%else
    mova                  m8,                 [tab_LumaCoeffVer_32_avx512 + r4]
    mova                  m9,                 [tab_LumaCoeffVer_32_avx512 + r4 + 1 * mmsize]
    mova                  m10,                [tab_LumaCoeffVer_32_avx512 + r4 + 2 * mmsize]
    mova                  m11,                [tab_LumaCoeffVer_32_avx512 + r4 + 3 * mmsize]
%endif
%ifidn %1, pp
    vbroadcasti32x8       m7,                 [pw_512]
%else
    shl                   r3d,                1
    vbroadcasti32x8       m7,                 [pw_2000]
    mova                  m16,                [interp4_vps_store1_avx512]
    mova                  m17,                [interp4_vps_store2_avx512]
%endif

    lea                   r6,                 [3 * r1]
    lea                   r7,                 [3 * r3]
    sub                   r0,                 r6

%rep %2/4 - 1
    PROCESS_LUMA_VERT_32x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_32x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VERT_LUMA_32xN_AVX512 pp, 8
    FILTER_VERT_LUMA_32xN_AVX512 pp, 16
    FILTER_VERT_LUMA_32xN_AVX512 pp, 24
    FILTER_VERT_LUMA_32xN_AVX512 pp, 32
    FILTER_VERT_LUMA_32xN_AVX512 pp, 64

    FILTER_VERT_LUMA_32xN_AVX512 ps, 8
    FILTER_VERT_LUMA_32xN_AVX512 ps, 16
    FILTER_VERT_LUMA_32xN_AVX512 ps, 24
    FILTER_VERT_LUMA_32xN_AVX512 ps, 32
    FILTER_VERT_LUMA_32xN_AVX512 ps, 64
%endif
%macro PROCESS_LUMA_VERT_48x8_AVX512 1
%ifidn %1, pp
    PROCESS_LUMA_VERT_32x4_AVX512 pp
%else
    PROCESS_LUMA_VERT_32x4_AVX512 ps
%endif
    lea                   r8,                 [r4 + 4 * r1]
    lea                   r9,                 [r2 + 4 * r3]
    movu                  ym1,                [r5]
    vinserti32x8          m1,                 [r5 + 2 * r1],       1
    movu                  ym3,                [r5 + r1]
    vinserti32x8          m3,                 [r5 + r6],           1
    punpcklbw             m0,                 m1,                  m3
    pmaddubsw             m0,                 m8
    punpckhbw             m1,                 m3
    pmaddubsw             m1,                 m8

    movu                  ym4,                [r5 + 2 * r1]
    vinserti32x8          m4,                 [r5 + 4 * r1],       1
    punpcklbw             m2,                 m3,                  m4
    pmaddubsw             m2,                 m8
    punpckhbw             m3,                 m4
    pmaddubsw             m3,                 m8

    movu                  ym5,                [r5 + r6]
    vinserti32x8          m5,                 [r4 + r1],           1
    punpcklbw             m6,                 m4,                  m5
    pmaddubsw             m6,                 m9
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m9

    paddw                 m0,                 m6
    paddw                 m1,                 m4

    movu                  ym4,                [r5 + 4 * r1]
    vinserti32x8          m4,                 [r4 + 2 * r1],       1
    punpcklbw             m6,                 m5,                  m4
    pmaddubsw             m6,                 m9
    punpckhbw             m5,                 m4
    pmaddubsw             m5,                 m9

    paddw                 m2,                 m6
    paddw                 m3,                 m5

    movu                  ym15,               [r4 + r1]
    vinserti32x8          m15,                [r4 + r6],           1
    punpcklbw             m12,                m4,                 m15
    pmaddubsw             m12,                m10
    punpckhbw             m13,                m4,                 m15
    pmaddubsw             m13,                m10

    movu                  ym4,                [r4 + 2 * r1]
    vinserti32x8          m4,                 [r4 + 4 * r1],       1
    punpcklbw             m14,                m15,                 m4
    pmaddubsw             m14,                m10
    punpckhbw             m15,                m4
    pmaddubsw             m15,                m10

    movu                  ym5,                [r4 + r6]
    vinserti32x8          m5,                 [r8 + r1],           1
    punpcklbw             m6,                 m4,                  m5
    pmaddubsw             m6,                 m11
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m11

    paddw                 m12,                m6
    paddw                 m13,                m4

    movu                  ym4,                [r4 + 4 * r1]
    vinserti32x8          m4,                 [r8 + 2 * r1],       1
    punpcklbw             m6,                 m5,                  m4
    pmaddubsw             m6,                 m11
    punpckhbw             m5,                 m4
    pmaddubsw             m5,                 m11

    paddw                 m14,                m6
    paddw                 m15,                m5

    paddw                 m0,                 m12
    paddw                 m1,                 m13
    paddw                 m2,                 m14
    paddw                 m3,                 m15
%ifidn %1,pp
    pmulhrsw              m0,                 m7
    pmulhrsw              m1,                 m7
    pmulhrsw              m2,                 m7
    pmulhrsw              m3,                 m7
    packuswb              m0,                 m1
    packuswb              m2,                 m3

    movu                  [r9],               ym0
    movu                  [r9 + r3],          ym2
    vextracti32x8         [r9 + 2 * r3],      m0,                  1
    vextracti32x8         [r9 + r7],          m2,                  1
%else
    psubw                 m0,                 m7
    psubw                 m1,                 m7
    mova                  m12,                 m16
    mova                  m13,                 m17
    vpermi2q              m12,                 m0,                m1
    vpermi2q              m13,                 m0,                m1
    movu                  [r9],               m12
    movu                  [r9 + 2 * r3],      m13

    psubw                 m2,                 m7
    psubw                 m3,                 m7
    mova                  m14,                 m16
    mova                  m15,                 m17
    vpermi2q              m14,                 m2,                m3
    vpermi2q              m15,                 m2,                m3
    movu                  [r9 + r3],          m14
    movu                  [r9 + r7],          m15
%endif
    movu                  xm1,                [r0 + mmsize/2]
    vinserti32x4          m1,                 [r0 + 2 * r1 + mmsize/2],       1
    vinserti32x4          m1,                 [r5 + mmsize/2],                2
    vinserti32x4          m1,                 [r5 + 2 * r1 + mmsize/2],       3
    movu                  xm3,                [r0 + r1 + mmsize/2]
    vinserti32x4          m3,                 [r0 + r6 + mmsize/2],           1
    vinserti32x4          m3,                 [r5 + r1 + mmsize/2],           2
    vinserti32x4          m3,                 [r5 + r6 + mmsize/2],           3
    punpcklbw             m0,                 m1,                  m3
    pmaddubsw             m0,                 m8
    punpckhbw             m1,                 m3
    pmaddubsw             m1,                 m8

    movu                  xm4,                [r0 + 2 * r1 + mmsize/2]
    vinserti32x4          m4,                 [r0 + 4 * r1 + mmsize/2],       1
    vinserti32x4          m4,                 [r5 + 2 * r1 + mmsize/2],       2
    vinserti32x4          m4,                 [r5 + 4 * r1 + mmsize/2],       3
    punpcklbw             m2,                 m3,                  m4
    pmaddubsw             m2,                 m8
    punpckhbw             m3,                 m4
    pmaddubsw             m3,                 m8

    movu                  xm5,                [r0 + r6 + mmsize/2]
    vinserti32x4          m5,                 [r5 + r1 + mmsize/2],           1
    vinserti32x4          m5,                 [r5 + r6 + mmsize/2],           2
    vinserti32x4          m5,                 [r4 + r1 + mmsize/2],           3
    punpcklbw             m6,                 m4,                  m5
    pmaddubsw             m6,                 m9
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m9

    paddw                 m0,                 m6
    paddw                 m1,                 m4

    movu                  xm4,                [r0 + 4 * r1 + mmsize/2]
    vinserti32x4          m4,                 [r5 + 2 * r1 + mmsize/2],       1
    vinserti32x4          m4,                 [r5 + 4 * r1 + mmsize/2],       2
    vinserti32x4          m4,                 [r4 + 2 * r1 + mmsize/2],       3
    punpcklbw             m6,                 m5,                  m4
    pmaddubsw             m6,                 m9
    punpckhbw             m5,                 m4
    pmaddubsw             m5,                 m9

    paddw                 m2,                 m6
    paddw                 m3,                 m5

    movu                  xm15,               [r5 + r1 + mmsize/2]
    vinserti32x4          m15,                [r5 + r6 + mmsize/2],           1
    vinserti32x4          m15,                [r4 + r1 + mmsize/2],           2
    vinserti32x4          m15,                [r4 + r6 + mmsize/2],           3
    punpcklbw             m12,                m4,                 m15
    pmaddubsw             m12,                m10
    punpckhbw             m13,                m4,                 m15
    pmaddubsw             m13,                m10

    movu                  xm4,                [r5 + 2 * r1 + mmsize/2]
    vinserti32x4          m4,                 [r5 + 4 * r1 + mmsize/2],       1
    vinserti32x4          m4,                 [r4 + 2 * r1 + mmsize/2],       2
    vinserti32x4          m4,                 [r4 + 4 * r1 + mmsize/2],       3
    punpcklbw             m14,                m15,                 m4
    pmaddubsw             m14,                m10
    punpckhbw             m15,                m4
    pmaddubsw             m15,                m10

    movu                  xm5,                [r5 + r6 + mmsize/2]
    vinserti32x4          m5,                 [r4 + r1 + mmsize/2],           1
    vinserti32x4          m5,                 [r4 + r6 + mmsize/2],           2
    vinserti32x4          m5,                 [r8 + r1 + mmsize/2],           3
    punpcklbw             m6,                 m4,                  m5
    pmaddubsw             m6,                 m11
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m11

    paddw                 m12,                m6
    paddw                 m13,                m4

    movu                  xm4,                [r5 + 4 * r1 + mmsize/2]
    vinserti32x4          m4,                 [r4 + 2 * r1 + mmsize/2],       1
    vinserti32x4          m4,                 [r4 + 4 * r1 + mmsize/2],       2
    vinserti32x4          m4,                 [r8 + 2 * r1 + mmsize/2],       3
    punpcklbw             m6,                 m5,                  m4
    pmaddubsw             m6,                 m11
    punpckhbw             m5,                 m4
    pmaddubsw             m5,                 m11

    paddw                 m14,                m6
    paddw                 m15,                m5

    paddw                 m0,                 m12
    paddw                 m1,                 m13
    paddw                 m2,                 m14
    paddw                 m3,                 m15
%ifidn %1, pp
    pmulhrsw              m0,                 m7
    pmulhrsw              m1,                 m7
    pmulhrsw              m2,                 m7
    pmulhrsw              m3,                 m7

    packuswb              m0,                 m1
    packuswb              m2,                 m3
    movu                  [r2 + mmsize/2],               xm0
    movu                  [r2 + r3 + mmsize/2],          xm2
    vextracti32x4         [r2 + 2 * r3 + mmsize/2],      m0,                  1
    vextracti32x4         [r2 + r7 + mmsize/2],          m2,                  1
    lea                   r2,                 [r2 + 4 * r3]
    vextracti32x4         [r2 + mmsize/2],               m0,                  2
    vextracti32x4         [r2 + r3 + mmsize/2],          m2,                  2
    vextracti32x4         [r2 + 2 * r3 + mmsize/2],      m0,                  3
    vextracti32x4         [r2 + r7 + mmsize/2],          m2,                  3
%else
    psubw                 m0,                 m7
    psubw                 m1,                 m7
    mova                  m12,                 m16
    mova                  m13,                 m17
    vpermi2q              m12,                 m0,                m1
    vpermi2q              m13,                 m0,                m1
    movu                  [r2 + mmsize],               ym12
    vextracti32x8         [r2 + 2 * r3 + mmsize],      m12,                 1

    psubw                 m2,                 m7
    psubw                 m3,                 m7
    mova                  m14,                 m16
    mova                  m15,                 m17
    vpermi2q              m14,                 m2,                m3
    vpermi2q              m15,                 m2,                m3
    movu                  [r2 + r3 + mmsize],          ym14
    vextracti32x8         [r2 + r7 + mmsize],          m14,                 1
    lea                   r2,                          [r2 + 4 * r3]

    movu                  [r2 + mmsize],               ym13
    movu                  [r2 + r3 + mmsize],          ym15
    vextracti32x8         [r2 + 2 * r3 + mmsize],      m13,                 1
    vextracti32x8         [r2 + r7 + mmsize],          m15,                 1
%endif
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VERT_LUMA_48x64_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_48x64, 5, 10, 18
    mov                   r4d,                r4m
    shl                   r4d,                8

%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_32_avx512]
    mova                  m8,                 [r5 + r4]
    mova                  m9,                 [r5 + r4 + 1 * mmsize]
    mova                  m10,                [r5 + r4 + 2 * mmsize]
    mova                  m11,                [r5 + r4 + 3 * mmsize]
%else
    mova                  m8,                 [tab_LumaCoeffVer_32_avx512 + r4]
    mova                  m9,                 [tab_LumaCoeffVer_32_avx512 + r4 + 1 * mmsize]
    mova                  m10,                [tab_LumaCoeffVer_32_avx512 + r4 + 2 * mmsize]
    mova                  m11,                [tab_LumaCoeffVer_32_avx512 + r4 + 3 * mmsize]
%endif
%ifidn %1, pp
    vbroadcasti32x8       m7,                 [pw_512]
%else
    shl                   r3d,                1
    vbroadcasti32x8       m7,                 [pw_2000]
    mova                  m16,                [interp4_vps_store1_avx512]
    mova                  m17,                [interp4_vps_store2_avx512]
%endif

    lea                   r6,                 [3 * r1]
    lea                   r7,                 [3 * r3]
    sub                   r0,                 r6

%rep 7
    PROCESS_LUMA_VERT_48x8_AVX512 %1
    lea                   r0,                 [r4]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_48x8_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VERT_LUMA_48x64_AVX512 pp
    FILTER_VERT_LUMA_48x64_AVX512 ps
%endif
%macro PROCESS_LUMA_VERT_64x2_AVX512 1
    lea                   r5,                 [r0 + 4 * r1]
    movu                  m1,                 [r0]
    movu                  m3,                 [r0 + r1]
    punpcklbw             m0,                 m1,                  m3
    pmaddubsw             m0,                 m8
    punpckhbw             m1,                 m3
    pmaddubsw             m1,                 m8

    movu                  m4,                 [r0 + 2 * r1]
    punpcklbw             m2,                 m3,                  m4
    pmaddubsw             m2,                 m8
    punpckhbw             m3,                 m4
    pmaddubsw             m3,                 m8

    movu                  m5,                 [r0 + r6]
    punpcklbw             m6,                 m4,                  m5
    pmaddubsw             m6,                 m9
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m9

    paddw                 m0,                 m6
    paddw                 m1,                 m4

    movu                  m4,                 [r0 + 4 * r1]
    punpcklbw             m6,                 m5,                  m4
    pmaddubsw             m6,                 m9
    punpckhbw             m5,                 m4
    pmaddubsw             m5,                 m9

    paddw                 m2,                 m6
    paddw                 m3,                 m5

    movu                  m15,                [r5 + r1]
    punpcklbw             m12,                m4,                  m15
    pmaddubsw             m12,                m10
    punpckhbw             m13,                m4,                  m15
    pmaddubsw             m13,                m10

    movu                  m4,                 [r5 + 2 * r1]
    punpcklbw             m14,                m15,                 m4
    pmaddubsw             m14,                m10
    punpckhbw             m15,                m4
    pmaddubsw             m15,                m10

    movu                  m5,                 [r5 + r6]
    punpcklbw             m6,                 m4,                  m5
    pmaddubsw             m6,                 m11
    punpckhbw             m4,                 m5
    pmaddubsw             m4,                 m11

    paddw                 m12,                m6
    paddw                 m13,                m4

    movu                  m4,                 [r5 + 4 * r1]
    punpcklbw             m6,                 m5,                  m4
    pmaddubsw             m6,                 m11
    punpckhbw             m5,                 m4
    pmaddubsw             m5,                 m11

    paddw                 m14,                m6
    paddw                 m15,                m5

    paddw                 m0,                 m12
    paddw                 m1,                 m13
    paddw                 m2,                 m14
    paddw                 m3,                 m15
%ifidn %1,pp
    pmulhrsw              m0,                 m7
    pmulhrsw              m1,                 m7
    pmulhrsw              m2,                 m7
    pmulhrsw              m3,                 m7

    packuswb              m0,                 m1
    packuswb              m2,                 m3
    movu                  [r2],               m0
    movu                  [r2 + r3],          m2
%else
    psubw                 m0,                 m7
    psubw                 m1,                 m7
    mova                  m12,                 m16
    mova                  m13,                 m17
    vpermi2q              m12,                 m0,                m1
    vpermi2q              m13,                 m0,                m1
    movu                  [r2],               m12
    movu                  [r2 + mmsize],      m13

    psubw                 m2,                 m7
    psubw                 m3,                 m7
    mova                  m14,                 m16
    mova                  m15,                 m17
    vpermi2q              m14,                 m2,                m3
    vpermi2q              m15,                 m2,                m3
    movu                  [r2 + r3],          m14
    movu                  [r2 + r3 + mmsize], m15
%endif
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VERT_LUMA_64xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_64x%2, 5, 8, 18
    mov                   r4d,                r4m
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_32_avx512]
    mova                  m8,                 [r5 + r4]
    mova                  m9,                 [r5 + r4 + 1 * mmsize]
    mova                  m10,                [r5 + r4 + 2 * mmsize]
    mova                  m11,                [r5 + r4 + 3 * mmsize]
%else
    mova                  m8,                 [tab_LumaCoeffVer_32_avx512 + r4]
    mova                  m9,                 [tab_LumaCoeffVer_32_avx512 + r4 + 1 * mmsize]
    mova                  m10,                [tab_LumaCoeffVer_32_avx512 + r4 + 2 * mmsize]
    mova                  m11,                [tab_LumaCoeffVer_32_avx512 + r4 + 3 * mmsize]
%endif
%ifidn %1, pp
    vbroadcasti32x8       m7,                 [pw_512]
%else
    shl                   r3d,                1
    vbroadcasti32x8       m7,                 [pw_2000]
    mova                  m16,                [interp4_vps_store1_avx512]
    mova                  m17,                [interp4_vps_store2_avx512]
%endif

    lea                   r6,                 [3 * r1]
    sub                   r0,                 r6
    lea                   r7,                 [3 * r3]

%rep %2/2 - 1
    PROCESS_LUMA_VERT_64x2_AVX512 %1
    lea                   r0,                 [r0 + 2 * r1]
    lea                   r2,                 [r2 + 2 * r3]
%endrep
    PROCESS_LUMA_VERT_64x2_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
FILTER_VERT_LUMA_64xN_AVX512 pp, 16
FILTER_VERT_LUMA_64xN_AVX512 pp, 32
FILTER_VERT_LUMA_64xN_AVX512 pp, 48
FILTER_VERT_LUMA_64xN_AVX512 pp, 64

FILTER_VERT_LUMA_64xN_AVX512 ps, 16
FILTER_VERT_LUMA_64xN_AVX512 ps, 32
FILTER_VERT_LUMA_64xN_AVX512 ps, 48
FILTER_VERT_LUMA_64xN_AVX512 ps, 64
%endif
;-------------------------------------------------------------------------------------------------------------
;avx512 luma_vpp and luma_vps code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
;ipfilter_luma_avx512 code end
;-------------------------------------------------------------------------------------------------------------