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

SECTION_RODATA 32

const h_tabw_LumaCoeff,  dw   0, 0,  0,  64,  0,   0,  0,  0
                       dw  -1, 4, -10, 58,  17, -5,  1,  0
                       dw  -1, 4, -11, 40,  40, -11, 4, -1
                       dw   0, 1, -5,  17,  58, -10, 4, -1

const h_tabw_ChromaCoeff, dw  0, 64,  0,  0
                        dw -2, 58, 10, -2
                        dw -4, 54, 16, -2
                        dw -6, 46, 28, -4
                        dw -4, 36, 36, -4
                        dw -4, 28, 46, -6
                        dw -2, 16, 54, -4
                        dw -2, 10, 58, -2

const h_tab_ChromaCoeff, db  0, 64,  0,  0
                       db -2, 58, 10, -2
                       db -4, 54, 16, -2
                       db -6, 46, 28, -4
                       db -4, 36, 36, -4
                       db -4, 28, 46, -6
                       db -2, 16, 54, -4
                       db -2, 10, 58, -2

const h_tab_LumaCoeff,   db   0, 0,  0,  64,  0,   0,  0,  0
                       db  -1, 4, -10, 58,  17, -5,  1,  0
                       db  -1, 4, -11, 40,  40, -11, 4, -1
                       db   0, 1, -5,  17,  58, -10, 4, -1

const h_tab_Tm,    db 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6
                 db 4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10
                 db 8, 9,10,11, 9,10,11,12,10,11,12,13,11,12,13, 14


const h_tab_Lm,    db 0, 1, 2, 3, 4,  5,  6,  7,  1, 2, 3, 4,  5,  6,  7,  8
                 db 2, 3, 4, 5, 6,  7,  8,  9,  3, 4, 5, 6,  7,  8,  9,  10
                 db 4, 5, 6, 7, 8,  9,  10, 11, 5, 6, 7, 8,  9,  10, 11, 12
                 db 6, 7, 8, 9, 10, 11, 12, 13, 7, 8, 9, 10, 11, 12, 13, 14

const interp4_shuf, times 2 db 0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 10, 11, 6, 7, 14, 15

const h_interp8_hps_shuf,     dd 0, 4, 1, 5, 2, 6, 3, 7

const interp4_hpp_shuf,     times 2 db 0, 1, 2, 3, 1, 2, 3, 4, 8, 9, 10, 11, 9, 10, 11, 12

const interp4_horiz_shuf1,  db 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6
                            db 8, 9, 10, 11, 9, 10, 11, 12, 10, 11, 12, 13, 11, 12, 13, 14

const h_pd_526336, times 8 dd 8192*64+2048

const pb_LumaCoeffVer,  times 16 db 0, 0
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

const h_pw_LumaCoeffVer,  times 8 dw 0, 0
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

const pb_8tap_hps_0, times 2 db 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8
                     times 2 db 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,10
                     times 2 db 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,10,10,11,11,12
                     times 2 db 6, 7, 7, 8, 8, 9, 9,10,10,11,11,12,12,13,13,14

ALIGN 32
interp4_hps_shuf: times 2 db 0, 1, 2, 3, 1, 2, 3, 4, 8, 9, 10, 11, 9, 10, 11, 12

SECTION .text

cextern pw_1
cextern pw_32
cextern pw_2000
cextern pw_512

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
    mova            m5, [h_interp8_hps_shuf]
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

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
%macro IPFILTER_LUMA_sse2 3
INIT_XMM sse2
cglobal interp_8tap_horiz_%3_%1x%2, 4,6,8
    mov       r4d, r4m
    add       r4d, r4d
    pxor      m6, m6

%ifidn %3, ps
    add       r3d, r3d
    cmp       r5m, byte 0
%endif

%ifdef PIC
    lea       r5, [h_tabw_LumaCoeff]
    movu      m3, [r5 + r4 * 8]
%else
    movu      m3, [h_tabw_LumaCoeff + r4 * 8]
%endif

    mov       r4d, %2

%ifidn %3, pp
    mova      m2, [pw_32]
%else
    mova      m2, [pw_2000]
    je        .loopH
    lea       r5, [r1 + 2 * r1]
    sub       r0, r5
    add       r4d, 7
%endif

.loopH:
%assign x 0
%rep %1 / 8
    FILTER_H8_W8_sse2
  %ifidn %3, pp
    paddw     m1, m2
    psraw     m1, 6
    packuswb  m1, m1
    movh      [r2 + x], m1
  %else
    psubw     m1, m2
    movu      [r2 + 2 * x], m1
  %endif
%assign x x+8
%endrep

%rep (%1 % 8) / 4
    FILTER_H8_W4_sse2
  %ifidn %3, pp
    paddw     m1, m2
    psraw     m1, 6
    packuswb  m1, m1
    movd      [r2 + x], m1
  %else
    psubw     m1, m2
    movh      [r2 + 2 * x], m1
  %endif
%endrep

    add       r0, r1
    add       r2, r3

    dec       r4d
    jnz       .loopH
    RET

%endmacro

;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
    IPFILTER_LUMA_sse2 4, 4, pp
    IPFILTER_LUMA_sse2 4, 8, pp
    IPFILTER_LUMA_sse2 8, 4, pp
    IPFILTER_LUMA_sse2 8, 8, pp
    IPFILTER_LUMA_sse2 16, 16, pp
    IPFILTER_LUMA_sse2 16, 8, pp
    IPFILTER_LUMA_sse2 8, 16, pp
    IPFILTER_LUMA_sse2 16, 12, pp
    IPFILTER_LUMA_sse2 12, 16, pp
    IPFILTER_LUMA_sse2 16, 4, pp
    IPFILTER_LUMA_sse2 4, 16, pp
    IPFILTER_LUMA_sse2 32, 32, pp
    IPFILTER_LUMA_sse2 32, 16, pp
    IPFILTER_LUMA_sse2 16, 32, pp
    IPFILTER_LUMA_sse2 32, 24, pp
    IPFILTER_LUMA_sse2 24, 32, pp
    IPFILTER_LUMA_sse2 32, 8, pp
    IPFILTER_LUMA_sse2 8, 32, pp
    IPFILTER_LUMA_sse2 64, 64, pp
    IPFILTER_LUMA_sse2 64, 32, pp
    IPFILTER_LUMA_sse2 32, 64, pp
    IPFILTER_LUMA_sse2 64, 48, pp
    IPFILTER_LUMA_sse2 48, 64, pp
    IPFILTER_LUMA_sse2 64, 16, pp
    IPFILTER_LUMA_sse2 16, 64, pp

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_ps_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
    IPFILTER_LUMA_sse2 4, 4, ps
    IPFILTER_LUMA_sse2 8, 8, ps
    IPFILTER_LUMA_sse2 8, 4, ps
    IPFILTER_LUMA_sse2 4, 8, ps
    IPFILTER_LUMA_sse2 16, 16, ps
    IPFILTER_LUMA_sse2 16, 8, ps
    IPFILTER_LUMA_sse2 8, 16, ps
    IPFILTER_LUMA_sse2 16, 12, ps
    IPFILTER_LUMA_sse2 12, 16, ps
    IPFILTER_LUMA_sse2 16, 4, ps
    IPFILTER_LUMA_sse2 4, 16, ps
    IPFILTER_LUMA_sse2 32, 32, ps
    IPFILTER_LUMA_sse2 32, 16, ps
    IPFILTER_LUMA_sse2 16, 32, ps
    IPFILTER_LUMA_sse2 32, 24, ps
    IPFILTER_LUMA_sse2 24, 32, ps
    IPFILTER_LUMA_sse2 32, 8, ps
    IPFILTER_LUMA_sse2 8, 32, ps
    IPFILTER_LUMA_sse2 64, 64, ps
    IPFILTER_LUMA_sse2 64, 32, ps
    IPFILTER_LUMA_sse2 32, 64, ps
    IPFILTER_LUMA_sse2 64, 48, ps
    IPFILTER_LUMA_sse2 48, 64, ps
    IPFILTER_LUMA_sse2 64, 16, ps
    IPFILTER_LUMA_sse2 16, 64, ps

%macro FILTER_H4_w2_2_sse2 0
    pxor        m3, m3
    movd        m0, [srcq - 1]
    movd        m2, [srcq]
    punpckldq   m0, m2
    punpcklbw   m0, m3
    movd        m1, [srcq + srcstrideq - 1]
    movd        m2, [srcq + srcstrideq]
    punpckldq   m1, m2
    punpcklbw   m1, m3
    pmaddwd     m0, m4
    pmaddwd     m1, m4
    packssdw    m0, m1
    pshuflw     m1, m0, q2301
    pshufhw     m1, m1, q2301
    paddw       m0, m1
    psrld       m0, 16
    packssdw    m0, m0
    paddw       m0, m5
    psraw       m0, 6
    packuswb    m0, m0
    movd        r4d, m0
    mov         [dstq], r4w
    shr         r4, 16
    mov         [dstq + dststrideq], r4w
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_2xN(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_H4_W2xN_sse3 1
INIT_XMM sse3
cglobal interp_4tap_horiz_pp_2x%1, 4, 6, 6, src, srcstride, dst, dststride
    mov         r4d,    r4m
    mova        m5,     [pw_32]

%ifdef PIC
    lea         r5,     [h_tabw_ChromaCoeff]
    movddup     m4,     [r5 + r4 * 8]
%else
    movddup     m4,     [h_tabw_ChromaCoeff + r4 * 8]
%endif

%assign x 1
%rep %1/2
    FILTER_H4_w2_2_sse2
%if x < %1/2
    lea         srcq,   [srcq + srcstrideq * 2]
    lea         dstq,   [dstq + dststrideq * 2]
%endif
%assign x x+1
%endrep

    RET

%endmacro

    FILTER_H4_W2xN_sse3 4
    FILTER_H4_W2xN_sse3 8
    FILTER_H4_W2xN_sse3 16

%macro FILTER_H4_w4_2_sse2 0
    pxor        m5, m5
    movd        m0, [srcq - 1]
    movd        m6, [srcq]
    punpckldq   m0, m6
    punpcklbw   m0, m5
    movd        m1, [srcq + 1]
    movd        m6, [srcq + 2]
    punpckldq   m1, m6
    punpcklbw   m1, m5
    movd        m2, [srcq + srcstrideq - 1]
    movd        m6, [srcq + srcstrideq]
    punpckldq   m2, m6
    punpcklbw   m2, m5
    movd        m3, [srcq + srcstrideq + 1]
    movd        m6, [srcq + srcstrideq + 2]
    punpckldq   m3, m6
    punpcklbw   m3, m5
    pmaddwd     m0, m4
    pmaddwd     m1, m4
    pmaddwd     m2, m4
    pmaddwd     m3, m4
    packssdw    m0, m1
    packssdw    m2, m3
    pshuflw     m1, m0, q2301
    pshufhw     m1, m1, q2301
    pshuflw     m3, m2, q2301
    pshufhw     m3, m3, q2301
    paddw       m0, m1
    paddw       m2, m3
    psrld       m0, 16
    psrld       m2, 16
    packssdw    m0, m2
    paddw       m0, m7
    psraw       m0, 6
    packuswb    m0, m2
    movd        [dstq], m0
    psrldq      m0, 4
    movd        [dstq + dststrideq], m0
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_4x32(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_H4_W4xN_sse3 1
INIT_XMM sse3
cglobal interp_4tap_horiz_pp_4x%1, 4, 6, 8, src, srcstride, dst, dststride
    mov         r4d,    r4m
    mova        m7,     [pw_32]

%ifdef PIC
    lea         r5,     [h_tabw_ChromaCoeff]
    movddup     m4,     [r5 + r4 * 8]
%else
    movddup     m4,     [h_tabw_ChromaCoeff + r4 * 8]
%endif

%assign x 1
%rep %1/2
    FILTER_H4_w4_2_sse2
%if x < %1/2
    lea         srcq,   [srcq + srcstrideq * 2]
    lea         dstq,   [dstq + dststrideq * 2]
%endif
%assign x x+1
%endrep

    RET

%endmacro

    FILTER_H4_W4xN_sse3 2
    FILTER_H4_W4xN_sse3 4
    FILTER_H4_W4xN_sse3 8
    FILTER_H4_W4xN_sse3 16
    FILTER_H4_W4xN_sse3 32

%macro FILTER_H4_w6_sse2 0
    pxor        m4, m4
    movh        m0, [srcq - 1]
    movh        m5, [srcq]
    punpckldq   m0, m5
    movhlps     m2, m0
    punpcklbw   m0, m4
    punpcklbw   m2, m4
    movd        m1, [srcq + 1]
    movd        m5, [srcq + 2]
    punpckldq   m1, m5
    punpcklbw   m1, m4
    pmaddwd     m0, m6
    pmaddwd     m1, m6
    pmaddwd     m2, m6
    packssdw    m0, m1
    packssdw    m2, m2
    pshuflw     m1, m0, q2301
    pshufhw     m1, m1, q2301
    pshuflw     m3, m2, q2301
    paddw       m0, m1
    paddw       m2, m3
    psrld       m0, 16
    psrld       m2, 16
    packssdw    m0, m2
    paddw       m0, m7
    psraw       m0, 6
    packuswb    m0, m0
    movd        [dstq], m0
    pextrw      r4d, m0, 2
    mov         [dstq + 4], r4w
%endmacro

%macro FILH4W8_sse2 1
    movh        m0, [srcq - 1 + %1]
    movh        m5, [srcq + %1]
    punpckldq   m0, m5
    movhlps     m2, m0
    punpcklbw   m0, m4
    punpcklbw   m2, m4
    movh        m1, [srcq + 1 + %1]
    movh        m5, [srcq + 2 + %1]
    punpckldq   m1, m5
    movhlps     m3, m1
    punpcklbw   m1, m4
    punpcklbw   m3, m4
    pmaddwd     m0, m6
    pmaddwd     m1, m6
    pmaddwd     m2, m6
    pmaddwd     m3, m6
    packssdw    m0, m1
    packssdw    m2, m3
    pshuflw     m1, m0, q2301
    pshufhw     m1, m1, q2301
    pshuflw     m3, m2, q2301
    pshufhw     m3, m3, q2301
    paddw       m0, m1
    paddw       m2, m3
    psrld       m0, 16
    psrld       m2, 16
    packssdw    m0, m2
    paddw       m0, m7
    psraw       m0, 6
    packuswb    m0, m0
    movh        [dstq + %1], m0
%endmacro

%macro FILTER_H4_w8_sse2 0
    FILH4W8_sse2 0
%endmacro

%macro FILTER_H4_w12_sse2 0
    FILH4W8_sse2 0
    movd        m1, [srcq - 1 + 8]
    movd        m3, [srcq + 8]
    punpckldq   m1, m3
    punpcklbw   m1, m4
    movd        m2, [srcq + 1 + 8]
    movd        m3, [srcq + 2 + 8]
    punpckldq   m2, m3
    punpcklbw   m2, m4
    pmaddwd     m1, m6
    pmaddwd     m2, m6
    packssdw    m1, m2
    pshuflw     m2, m1, q2301
    pshufhw     m2, m2, q2301
    paddw       m1, m2
    psrld       m1, 16
    packssdw    m1, m1
    paddw       m1, m7
    psraw       m1, 6
    packuswb    m1, m1
    movd        [dstq + 8], m1
%endmacro

%macro FILTER_H4_w16_sse2 0
    FILH4W8_sse2 0
    FILH4W8_sse2 8
%endmacro

%macro FILTER_H4_w24_sse2 0
    FILH4W8_sse2 0
    FILH4W8_sse2 8
    FILH4W8_sse2 16
%endmacro

%macro FILTER_H4_w32_sse2 0
    FILH4W8_sse2 0
    FILH4W8_sse2 8
    FILH4W8_sse2 16
    FILH4W8_sse2 24
%endmacro

%macro FILTER_H4_w48_sse2 0
    FILH4W8_sse2 0
    FILH4W8_sse2 8
    FILH4W8_sse2 16
    FILH4W8_sse2 24
    FILH4W8_sse2 32
    FILH4W8_sse2 40
%endmacro

%macro FILTER_H4_w64_sse2 0
    FILH4W8_sse2 0
    FILH4W8_sse2 8
    FILH4W8_sse2 16
    FILH4W8_sse2 24
    FILH4W8_sse2 32
    FILH4W8_sse2 40
    FILH4W8_sse2 48
    FILH4W8_sse2 56
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro IPFILTER_CHROMA_sse3 2
INIT_XMM sse3
cglobal interp_4tap_horiz_pp_%1x%2, 4, 6, 8, src, srcstride, dst, dststride
    mov         r4d,        r4m
    mova        m7,         [pw_32]
    pxor        m4,         m4

%ifdef PIC
    lea         r5,          [h_tabw_ChromaCoeff]
    movddup     m6,       [r5 + r4 * 8]
%else
    movddup     m6,       [h_tabw_ChromaCoeff + r4 * 8]
%endif

%assign x 1
%rep %2
    FILTER_H4_w%1_sse2
%if x < %2
    add         srcq,        srcstrideq
    add         dstq,        dststrideq
%endif
%assign x x+1
%endrep

    RET

%endmacro

    IPFILTER_CHROMA_sse3 6,   8
    IPFILTER_CHROMA_sse3 8,   2
    IPFILTER_CHROMA_sse3 8,   4
    IPFILTER_CHROMA_sse3 8,   6
    IPFILTER_CHROMA_sse3 8,   8
    IPFILTER_CHROMA_sse3 8,  16
    IPFILTER_CHROMA_sse3 8,  32
    IPFILTER_CHROMA_sse3 12, 16

    IPFILTER_CHROMA_sse3 6,  16
    IPFILTER_CHROMA_sse3 8,  12
    IPFILTER_CHROMA_sse3 8,  64
    IPFILTER_CHROMA_sse3 12, 32

    IPFILTER_CHROMA_sse3 16,  4
    IPFILTER_CHROMA_sse3 16,  8
    IPFILTER_CHROMA_sse3 16, 12
    IPFILTER_CHROMA_sse3 16, 16
    IPFILTER_CHROMA_sse3 16, 32
    IPFILTER_CHROMA_sse3 32,  8
    IPFILTER_CHROMA_sse3 32, 16
    IPFILTER_CHROMA_sse3 32, 24
    IPFILTER_CHROMA_sse3 24, 32
    IPFILTER_CHROMA_sse3 32, 32

    IPFILTER_CHROMA_sse3 16, 24
    IPFILTER_CHROMA_sse3 16, 64
    IPFILTER_CHROMA_sse3 32, 48
    IPFILTER_CHROMA_sse3 24, 64
    IPFILTER_CHROMA_sse3 32, 64

    IPFILTER_CHROMA_sse3 64, 64
    IPFILTER_CHROMA_sse3 64, 32
    IPFILTER_CHROMA_sse3 64, 48
    IPFILTER_CHROMA_sse3 48, 64
    IPFILTER_CHROMA_sse3 64, 16

%macro FILTER_2 2
    movd        m3,     [srcq + %1]
    movd        m4,     [srcq + 1 + %1]
    punpckldq   m3,     m4
    punpcklbw   m3,     m0
    pmaddwd     m3,     m1
    packssdw    m3,     m3
    pshuflw     m4,     m3, q2301
    paddw       m3,     m4
    psrldq      m3,     2
    psubw       m3,     m2
    movd        [dstq + %2], m3
%endmacro

%macro FILTER_4 2
    movd        m3,     [srcq + %1]
    movd        m4,     [srcq + 1 + %1]
    punpckldq   m3,     m4
    punpcklbw   m3,     m0
    pmaddwd     m3,     m1
    movd        m4,     [srcq + 2 + %1]
    movd        m5,     [srcq + 3 + %1]
    punpckldq   m4,     m5
    punpcklbw   m4,     m0
    pmaddwd     m4,     m1
    packssdw    m3,     m4
    pshuflw     m4,     m3, q2301
    pshufhw     m4,     m4, q2301
    paddw       m3,     m4
    psrldq      m3,     2
    pshufd      m3,     m3,     q3120
    psubw       m3,     m2
    movh        [dstq + %2], m3
%endmacro

%macro FILTER_4TAP_HPS_sse3 2
INIT_XMM sse3
cglobal interp_4tap_horiz_ps_%1x%2, 4, 7, 6, src, srcstride, dst, dststride
    mov         r4d,    r4m
    add         dststrided, dststrided
    mova        m2,     [pw_2000]
    pxor        m0,     m0

%ifdef PIC
    lea         r6,     [h_tabw_ChromaCoeff]
    movddup     m1,     [r6 + r4 * 8]
%else
    movddup     m1,     [h_tabw_ChromaCoeff + r4 * 8]
%endif

    mov        r4d,     %2
    cmp        r5m,     byte 0
    je         .loopH
    sub        srcq,    srcstrideq
    add        r4d,     3

.loopH:
%assign x -1
%assign y 0
%rep %1/4
    FILTER_4 x,y
%assign x x+4
%assign y y+8
%endrep
%rep (%1 % 4)/2
    FILTER_2 x,y
%endrep
    add         srcq,   srcstrideq
    add         dstq,   dststrideq

    dec         r4d
    jnz         .loopH
    RET

%endmacro

    FILTER_4TAP_HPS_sse3 2, 4
    FILTER_4TAP_HPS_sse3 2, 8
    FILTER_4TAP_HPS_sse3 2, 16
    FILTER_4TAP_HPS_sse3 4, 2
    FILTER_4TAP_HPS_sse3 4, 4
    FILTER_4TAP_HPS_sse3 4, 8
    FILTER_4TAP_HPS_sse3 4, 16
    FILTER_4TAP_HPS_sse3 4, 32
    FILTER_4TAP_HPS_sse3 6, 8
    FILTER_4TAP_HPS_sse3 6, 16
    FILTER_4TAP_HPS_sse3 8, 2
    FILTER_4TAP_HPS_sse3 8, 4
    FILTER_4TAP_HPS_sse3 8, 6
    FILTER_4TAP_HPS_sse3 8, 8
    FILTER_4TAP_HPS_sse3 8, 12
    FILTER_4TAP_HPS_sse3 8, 16
    FILTER_4TAP_HPS_sse3 8, 32
    FILTER_4TAP_HPS_sse3 8, 64
    FILTER_4TAP_HPS_sse3 12, 16
    FILTER_4TAP_HPS_sse3 12, 32
    FILTER_4TAP_HPS_sse3 16, 4
    FILTER_4TAP_HPS_sse3 16, 8
    FILTER_4TAP_HPS_sse3 16, 12
    FILTER_4TAP_HPS_sse3 16, 16
    FILTER_4TAP_HPS_sse3 16, 24
    FILTER_4TAP_HPS_sse3 16, 32
    FILTER_4TAP_HPS_sse3 16, 64
    FILTER_4TAP_HPS_sse3 24, 32
    FILTER_4TAP_HPS_sse3 24, 64
    FILTER_4TAP_HPS_sse3 32,  8
    FILTER_4TAP_HPS_sse3 32, 16
    FILTER_4TAP_HPS_sse3 32, 24
    FILTER_4TAP_HPS_sse3 32, 32
    FILTER_4TAP_HPS_sse3 32, 48
    FILTER_4TAP_HPS_sse3 32, 64
    FILTER_4TAP_HPS_sse3 48, 64
    FILTER_4TAP_HPS_sse3 64, 16
    FILTER_4TAP_HPS_sse3 64, 32
    FILTER_4TAP_HPS_sse3 64, 48
    FILTER_4TAP_HPS_sse3 64, 64

%macro FILTER_H4_w2_2 3
    movh        %2, [srcq - 1]
    pshufb      %2, %2, Tm0
    movh        %1, [srcq + srcstrideq - 1]
    pshufb      %1, %1, Tm0
    punpcklqdq  %2, %1
    pmaddubsw   %2, coef2
    phaddw      %2, %2
    pmulhrsw    %2, %3
    packuswb    %2, %2
    movd        r4d, %2
    mov         [dstq], r4w
    shr         r4, 16
    mov         [dstq + dststrideq], r4w
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_2x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_2x4, 4, 6, 5, src, srcstride, dst, dststride
%define coef2       m4
%define Tm0         m3
%define t2          m2
%define t1          m1
%define t0          m0

    mov         r4d,        r4m

%ifdef PIC
    lea         r5,          [h_tab_ChromaCoeff]
    movd        coef2,       [r5 + r4 * 4]
%else
    movd        coef2,       [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd      coef2,       coef2,      0
    mova        t2,          [pw_512]
    mova        Tm0,         [h_tab_Tm]

%rep 2
    FILTER_H4_w2_2   t0, t1, t2
    lea         srcq,       [srcq + srcstrideq * 2]
    lea         dstq,       [dstq + dststrideq * 2]
%endrep

    RET

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_2x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_2x8, 4, 6, 5, src, srcstride, dst, dststride
%define coef2       m4
%define Tm0         m3
%define t2          m2
%define t1          m1
%define t0          m0

    mov         r4d,        r4m

%ifdef PIC
    lea         r5,          [h_tab_ChromaCoeff]
    movd        coef2,       [r5 + r4 * 4]
%else
    movd        coef2,       [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd      coef2,       coef2,      0
    mova        t2,          [pw_512]
    mova        Tm0,         [h_tab_Tm]

%rep 4
    FILTER_H4_w2_2   t0, t1, t2
    lea         srcq,       [srcq + srcstrideq * 2]
    lea         dstq,       [dstq + dststrideq * 2]
%endrep

    RET

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_2x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_2x16, 4, 6, 5, src, srcstride, dst, dststride
%define coef2       m4
%define Tm0         m3
%define t2          m2
%define t1          m1
%define t0          m0

    mov         r4d,        r4m

%ifdef PIC
    lea         r5,          [h_tab_ChromaCoeff]
    movd        coef2,       [r5 + r4 * 4]
%else
    movd        coef2,       [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd      coef2,       coef2,      0
    mova        t2,          [pw_512]
    mova        Tm0,         [h_tab_Tm]

    mov         r5d,        16/2

.loop:
    FILTER_H4_w2_2   t0, t1, t2
    lea         srcq,       [srcq + srcstrideq * 2]
    lea         dstq,       [dstq + dststrideq * 2]
    dec         r5d
    jnz         .loop

    RET

%macro FILTER_H4_w4_2 3
    movh        %2, [srcq - 1]
    pshufb      %2, %2, Tm0
    pmaddubsw   %2, coef2
    movh        %1, [srcq + srcstrideq - 1]
    pshufb      %1, %1, Tm0
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    pmulhrsw    %2, %3
    packuswb    %2, %2
    movd        [dstq], %2
    palignr     %2, %2, 4
    movd        [dstq + dststrideq], %2
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_4x2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_4x2, 4, 6, 5, src, srcstride, dst, dststride
%define coef2       m4
%define Tm0         m3
%define t2          m2
%define t1          m1
%define t0          m0

    mov         r4d,        r4m

%ifdef PIC
    lea         r5,          [h_tab_ChromaCoeff]
    movd        coef2,       [r5 + r4 * 4]
%else
    movd        coef2,       [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd      coef2,       coef2,      0
    mova        t2,          [pw_512]
    mova        Tm0,         [h_tab_Tm]

    FILTER_H4_w4_2   t0, t1, t2

    RET

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_4x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_4x4, 4, 6, 5, src, srcstride, dst, dststride
%define coef2       m4
%define Tm0         m3
%define t2          m2
%define t1          m1
%define t0          m0

    mov         r4d,        r4m

%ifdef PIC
    lea         r5,          [h_tab_ChromaCoeff]
    movd        coef2,       [r5 + r4 * 4]
%else
    movd        coef2,       [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd      coef2,       coef2,      0
    mova        t2,          [pw_512]
    mova        Tm0,         [h_tab_Tm]

%rep 2
    FILTER_H4_w4_2   t0, t1, t2
    lea         srcq,       [srcq + srcstrideq * 2]
    lea         dstq,       [dstq + dststrideq * 2]
%endrep

    RET

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_4x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_4x8, 4, 6, 5, src, srcstride, dst, dststride
%define coef2       m4
%define Tm0         m3
%define t2          m2
%define t1          m1
%define t0          m0

    mov         r4d,        r4m

%ifdef PIC
    lea         r5,          [h_tab_ChromaCoeff]
    movd        coef2,       [r5 + r4 * 4]
%else
    movd        coef2,       [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd      coef2,       coef2,      0
    mova        t2,          [pw_512]
    mova        Tm0,         [h_tab_Tm]

%rep 4
    FILTER_H4_w4_2   t0, t1, t2
    lea         srcq,       [srcq + srcstrideq * 2]
    lea         dstq,       [dstq + dststrideq * 2]
%endrep

    RET

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_4x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_4x16, 4, 6, 5, src, srcstride, dst, dststride
%define coef2       m4
%define Tm0         m3
%define t2          m2
%define t1          m1
%define t0          m0

    mov         r4d,        r4m

%ifdef PIC
    lea         r5,          [h_tab_ChromaCoeff]
    movd        coef2,       [r5 + r4 * 4]
%else
    movd        coef2,       [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd      coef2,       coef2,      0
    mova        t2,          [pw_512]
    mova        Tm0,         [h_tab_Tm]

%rep 8
    FILTER_H4_w4_2   t0, t1, t2
    lea         srcq,       [srcq + srcstrideq * 2]
    lea         dstq,       [dstq + dststrideq * 2]
%endrep

    RET

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_4x32(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_4x32, 4, 6, 5, src, srcstride, dst, dststride
%define coef2       m4
%define Tm0         m3
%define t2          m2
%define t1          m1
%define t0          m0

    mov         r4d,        r4m

%ifdef PIC
    lea         r5,          [h_tab_ChromaCoeff]
    movd        coef2,       [r5 + r4 * 4]
%else
    movd        coef2,       [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd      coef2,       coef2,      0
    mova        t2,          [pw_512]
    mova        Tm0,         [h_tab_Tm]

    mov         r5d,        32/2

.loop:
    FILTER_H4_w4_2   t0, t1, t2
    lea         srcq,       [srcq + srcstrideq * 2]
    lea         dstq,       [dstq + dststrideq * 2]
    dec         r5d
    jnz         .loop

    RET

ALIGN 32
const interp_4tap_8x8_horiz_shuf,   dd 0, 4, 1, 5, 2, 6, 3, 7

%macro FILTER_H4_w6 3
    movu        %1, [srcq - 1]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    pmulhrsw    %2, %3
    packuswb    %2, %2
    movd        [dstq],      %2
    pextrw      [dstq + 4], %2, 2
%endmacro

%macro FILTER_H4_w8 3
    movu        %1, [srcq - 1]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    pmulhrsw    %2, %3
    packuswb    %2, %2
    movh        [dstq],      %2
%endmacro

%macro FILTER_H4_w12 3
    movu        %1, [srcq - 1]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    pmulhrsw    %2, %3
    movu        %1, [srcq - 1 + 8]
    pshufb      %1, %1, Tm0
    pmaddubsw   %1, coef2
    phaddw      %1, %1
    pmulhrsw    %1, %3
    packuswb    %2, %1
    movh        [dstq],      %2
    pextrd      [dstq + 8], %2, 2
%endmacro

%macro FILTER_H4_w16 4
    movu        %1, [srcq - 1]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    movu        %1, [srcq - 1 + 8]
    pshufb      %4, %1, Tm0
    pmaddubsw   %4, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %4, %1
    pmulhrsw    %2, %3
    pmulhrsw    %4, %3
    packuswb    %2, %4
    movu        [dstq],      %2
%endmacro

%macro FILTER_H4_w24 4
    movu        %1, [srcq - 1]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    movu        %1, [srcq - 1 + 8]
    pshufb      %4, %1, Tm0
    pmaddubsw   %4, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %4, %1
    pmulhrsw    %2, %3
    pmulhrsw    %4, %3
    packuswb    %2, %4
    movu        [dstq],          %2
    movu        %1, [srcq - 1 + 16]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    pmulhrsw    %2, %3
    packuswb    %2, %2
    movh        [dstq + 16],     %2
%endmacro

%macro FILTER_H4_w32 4
    movu        %1, [srcq - 1]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    movu        %1, [srcq - 1 + 8]
    pshufb      %4, %1, Tm0
    pmaddubsw   %4, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %4, %1
    pmulhrsw    %2, %3
    pmulhrsw    %4, %3
    packuswb    %2, %4
    movu        [dstq],      %2
    movu        %1, [srcq - 1 + 16]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    movu        %1, [srcq - 1 + 24]
    pshufb      %4, %1, Tm0
    pmaddubsw   %4, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %4, %1
    pmulhrsw    %2, %3
    pmulhrsw    %4, %3
    packuswb    %2, %4
    movu        [dstq + 16],      %2
%endmacro

%macro FILTER_H4_w16o 5
    movu        %1, [srcq + %5 - 1]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    movu        %1, [srcq + %5 - 1 + 8]
    pshufb      %4, %1, Tm0
    pmaddubsw   %4, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %4, %1
    pmulhrsw    %2, %3
    pmulhrsw    %4, %3
    packuswb    %2, %4
    movu        [dstq + %5],      %2
%endmacro

%macro FILTER_H4_w48 4
    FILTER_H4_w16o %1, %2, %3, %4, 0
    FILTER_H4_w16o %1, %2, %3, %4, 16
    FILTER_H4_w16o %1, %2, %3, %4, 32
%endmacro

%macro FILTER_H4_w64 4
    FILTER_H4_w16o %1, %2, %3, %4, 0
    FILTER_H4_w16o %1, %2, %3, %4, 16
    FILTER_H4_w16o %1, %2, %3, %4, 32
    FILTER_H4_w16o %1, %2, %3, %4, 48
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro IPFILTER_CHROMA 2
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_%1x%2, 4, 6, 6, src, srcstride, dst, dststride
%define coef2       m5
%define Tm0         m4
%define Tm1         m3
%define t2          m2
%define t1          m1
%define t0          m0

    mov         r4d,        r4m

%ifdef PIC
    lea         r5,          [h_tab_ChromaCoeff]
    movd        coef2,       [r5 + r4 * 4]
%else
    movd        coef2,       [h_tab_ChromaCoeff + r4 * 4]
%endif

    mov           r5d,       %2

    pshufd      coef2,       coef2,      0
    mova        t2,          [pw_512]
    mova        Tm0,         [h_tab_Tm]
    mova        Tm1,         [h_tab_Tm + 16]

.loop:
    FILTER_H4_w%1   t0, t1, t2
    add         srcq,        srcstrideq
    add         dstq,        dststrideq

    dec         r5d
    jnz        .loop

    RET
%endmacro


    IPFILTER_CHROMA 6,   8
    IPFILTER_CHROMA 8,   2
    IPFILTER_CHROMA 8,   4
    IPFILTER_CHROMA 8,   6
    IPFILTER_CHROMA 8,   8
    IPFILTER_CHROMA 8,  16
    IPFILTER_CHROMA 8,  32
    IPFILTER_CHROMA 12, 16

    IPFILTER_CHROMA 6,  16
    IPFILTER_CHROMA 8,  12
    IPFILTER_CHROMA 8,  64
    IPFILTER_CHROMA 12, 32

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro IPFILTER_CHROMA_W 2
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_%1x%2, 4, 6, 7, src, srcstride, dst, dststride
%define coef2       m6
%define Tm0         m5
%define Tm1         m4
%define t3          m3
%define t2          m2
%define t1          m1
%define t0          m0

    mov         r4d,         r4m

%ifdef PIC
    lea         r5,          [h_tab_ChromaCoeff]
    movd        coef2,       [r5 + r4 * 4]
%else
    movd        coef2,       [h_tab_ChromaCoeff + r4 * 4]
%endif

    mov         r5d,          %2

    pshufd      coef2,       coef2,      0
    mova        t2,          [pw_512]
    mova        Tm0,         [h_tab_Tm]
    mova        Tm1,         [h_tab_Tm + 16]

.loop:
    FILTER_H4_w%1   t0, t1, t2, t3
    add         srcq,        srcstrideq
    add         dstq,        dststrideq

    dec         r5d
    jnz        .loop

    RET
%endmacro

    IPFILTER_CHROMA_W 16,  4
    IPFILTER_CHROMA_W 16,  8
    IPFILTER_CHROMA_W 16, 12
    IPFILTER_CHROMA_W 16, 16
    IPFILTER_CHROMA_W 16, 32
    IPFILTER_CHROMA_W 32,  8
    IPFILTER_CHROMA_W 32, 16
    IPFILTER_CHROMA_W 32, 24
    IPFILTER_CHROMA_W 24, 32
    IPFILTER_CHROMA_W 32, 32

    IPFILTER_CHROMA_W 16, 24
    IPFILTER_CHROMA_W 16, 64
    IPFILTER_CHROMA_W 32, 48
    IPFILTER_CHROMA_W 24, 64
    IPFILTER_CHROMA_W 32, 64

    IPFILTER_CHROMA_W 64, 64
    IPFILTER_CHROMA_W 64, 32
    IPFILTER_CHROMA_W 64, 48
    IPFILTER_CHROMA_W 48, 64
    IPFILTER_CHROMA_W 64, 16

%macro FILTER_H8_W8 7-8   ; t0, t1, t2, t3, coef, c512, src, dst
    movu        %1, %7
    pshufb      %2, %1, [h_tab_Lm +  0]
    pmaddubsw   %2, %5
    pshufb      %3, %1, [h_tab_Lm + 16]
    pmaddubsw   %3, %5
    phaddw      %2, %3
    pshufb      %4, %1, [h_tab_Lm + 32]
    pmaddubsw   %4, %5
    pshufb      %1, %1, [h_tab_Lm + 48]
    pmaddubsw   %1, %5
    phaddw      %4, %1
    phaddw      %2, %4
  %if %0 == 8
    pmulhrsw    %2, %6
    packuswb    %2, %2
    movh        %8, %2
  %endif
%endmacro

%macro FILTER_H8_W4 2
    movu        %1, [r0 - 3 + r5]
    pshufb      %2, %1, [h_tab_Lm]
    pmaddubsw   %2, m3
    pshufb      m7, %1, [h_tab_Lm + 16]
    pmaddubsw   m7, m3
    phaddw      %2, m7
    phaddw      %2, %2
%endmacro

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
%macro IPFILTER_LUMA 3
INIT_XMM sse4
cglobal interp_8tap_horiz_%3_%1x%2, 4,7,8

    mov       r4d, r4m

%ifdef PIC
    lea       r6, [h_tab_LumaCoeff]
    movh      m3, [r6 + r4 * 8]
%else
    movh      m3, [h_tab_LumaCoeff + r4 * 8]
%endif
    punpcklqdq  m3, m3

%ifidn %3, pp
    mova      m2, [pw_512]
%else
    mova      m2, [pw_2000]
%endif

    mov       r4d, %2
%ifidn %3, ps
    add       r3, r3
    cmp       r5m, byte 0
    je        .loopH
    lea       r6, [r1 + 2 * r1]
    sub       r0, r6
    add       r4d, 7
%endif

.loopH:
    xor       r5, r5
%rep %1 / 8
  %ifidn %3, pp
    FILTER_H8_W8  m0, m1, m4, m5, m3, m2, [r0 - 3 + r5], [r2 + r5]
  %else
    FILTER_H8_W8  m0, m1, m4, m5, m3, UNUSED, [r0 - 3 + r5]
    psubw     m1, m2
    movu      [r2 + 2 * r5], m1
  %endif
    add       r5, 8
%endrep

%rep (%1 % 8) / 4
    FILTER_H8_W4  m0, m1
  %ifidn %3, pp
    pmulhrsw  m1, m2
    packuswb  m1, m1
    movd      [r2 + r5], m1
  %else
    psubw     m1, m2
    movh      [r2 + 2 * r5], m1
  %endif
%endrep

    add       r0, r1
    add       r2, r3

    dec       r4d
    jnz       .loopH
    RET
%endmacro

INIT_YMM avx2
cglobal interp_8tap_horiz_pp_4x4, 4,6,6
    mov             r4d, r4m

%ifdef PIC
    lea             r5, [h_tab_LumaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h_tab_LumaCoeff + r4 * 8]
%endif

    mova            m1, [h_tab_Lm]
    vpbroadcastd    m2, [pw_1]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    sub             r0, 3
    ; Row 0-1
    vbroadcasti128  m3, [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m3, m1
    pmaddubsw       m3, m0
    pmaddwd         m3, m2
    vbroadcasti128  m4, [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m4, m1
    pmaddubsw       m4, m0
    pmaddwd         m4, m2
    phaddd          m3, m4                          ; DWORD [R1D R1C R0D R0C R1B R1A R0B R0A]

    ; Row 2-3
    lea             r0, [r0 + r1 * 2]
    vbroadcasti128  m4, [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m4, m1
    pmaddubsw       m4, m0
    pmaddwd         m4, m2
    vbroadcasti128  m5, [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m5, m1
    pmaddubsw       m5, m0
    pmaddwd         m5, m2
    phaddd          m4, m5                          ; DWORD [R3D R3C R2D R2C R3B R3A R2B R2A]

    packssdw        m3, m4                          ; WORD [R3D R3C R2D R2C R1D R1C R0D R0C R3B R3A R2B R2A R1B R1A R0B R0A]
    pmulhrsw        m3, [pw_512]
    vextracti128    xm4, m3, 1
    packuswb        xm3, xm4                        ; BYTE [R3D R3C R2D R2C R1D R1C R0D R0C R3B R3A R2B R2A R1B R1A R0B R0A]
    pshufb          xm3, [interp4_shuf]             ; [row3 row1 row2 row0]

    lea             r0, [r3 * 3]
    movd            [r2], xm3
    pextrd          [r2+r3], xm3, 2
    pextrd          [r2+r3*2], xm3, 1
    pextrd          [r2+r0], xm3, 3
    RET

%macro FILTER_HORIZ_LUMA_AVX2_4xN 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_horiz_pp_4x%1, 4, 6, 9
    mov             r4d, r4m

%ifdef PIC
    lea             r5, [h_tab_LumaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h_tab_LumaCoeff + r4 * 8]
%endif

    mova            m1, [h_tab_Lm]
    mova            m2, [pw_1]
    mova            m7, [h_interp8_hps_shuf]
    mova            m8, [pw_512]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1
    lea             r4, [r1 * 3]
    lea             r5, [r3 * 3]
    sub             r0, 3
%rep %1 / 8
    ; Row 0-1
    vbroadcasti128  m3, [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m3, m1
    pmaddubsw       m3, m0
    pmaddwd         m3, m2
    vbroadcasti128  m4, [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m4, m1
    pmaddubsw       m4, m0
    pmaddwd         m4, m2
    phaddd          m3, m4                          ; DWORD [R1D R1C R0D R0C R1B R1A R0B R0A]

    ; Row 2-3
    vbroadcasti128  m4, [r0 + r1 * 2]               ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m4, m1
    pmaddubsw       m4, m0
    pmaddwd         m4, m2
    vbroadcasti128  m5, [r0 + r4]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m5, m1
    pmaddubsw       m5, m0
    pmaddwd         m5, m2
    phaddd          m4, m5                          ; DWORD [R3D R3C R2D R2C R3B R3A R2B R2A]

    packssdw        m3, m4                          ; WORD [R3D R3C R2D R2C R1D R1C R0D R0C R3B R3A R2B R2A R1B R1A R0B R0A]
    lea             r0, [r0 + r1 * 4]
    ; Row 4-5
    vbroadcasti128  m5, [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m5, m1
    pmaddubsw       m5, m0
    pmaddwd         m5, m2
    vbroadcasti128  m4, [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m4, m1
    pmaddubsw       m4, m0
    pmaddwd         m4, m2
    phaddd          m5, m4                          ; DWORD [R5D R5C R4D R4C R5B R5A R4B R4A]

    ; Row 6-7
    vbroadcasti128  m4, [r0 + r1 * 2]               ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m4, m1
    pmaddubsw       m4, m0
    pmaddwd         m4, m2
    vbroadcasti128  m6, [r0 + r4]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m6, m1
    pmaddubsw       m6, m0
    pmaddwd         m6, m2
    phaddd          m4, m6                          ; DWORD [R7D R7C R6D R6C R7B R7A R6B R6A]

    packssdw        m5, m4                          ; WORD [R7D R7C R6D R6C R5D R5C R4D R4C R7B R7A R6B R6A R5B R5A R4B R4A]
    vpermd          m3, m7, m3
    vpermd          m5, m7, m5
    pmulhrsw        m3, m8
    pmulhrsw        m5, m8
    packuswb        m3, m5
    vextracti128    xm5, m3, 1

    movd            [r2], xm3
    pextrd          [r2 + r3], xm3, 1
    movd            [r2 + r3 * 2], xm5
    pextrd          [r2 + r5], xm5, 1
    lea             r2, [r2 + r3 * 4]
    pextrd          [r2], xm3, 2
    pextrd          [r2 + r3], xm3, 3
    pextrd          [r2 + r3 * 2], xm5, 2
    pextrd          [r2 + r5], xm5, 3
    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]
%endrep
    RET
%endif
%endmacro

    FILTER_HORIZ_LUMA_AVX2_4xN 8
    FILTER_HORIZ_LUMA_AVX2_4xN 16

INIT_YMM avx2
cglobal interp_8tap_horiz_pp_8x4, 4, 6, 7
    mov             r4d, r4m

%ifdef PIC
    lea             r5, [h_tab_LumaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h_tab_LumaCoeff + r4 * 8]
%endif

    mova            m1, [h_tab_Lm]
    mova            m2, [h_tab_Lm + 32]

    ; register map
    ; m0     - interpolate coeff
    ; m1, m2 - shuffle order table

    sub             r0, 3
    lea             r5, [r1 * 3]
    lea             r4, [r3 * 3]

    ; Row 0
    vbroadcasti128  m3, [r0]                        ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m4, m3, m2
    pshufb          m3, m1
    pmaddubsw       m3, m0
    pmaddubsw       m4, m0
    phaddw          m3, m4
    ; Row 1
    vbroadcasti128  m4, [r0 + r1]                   ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m5, m4, m2
    pshufb          m4, m1
    pmaddubsw       m4, m0
    pmaddubsw       m5, m0
    phaddw          m4, m5

    phaddw          m3, m4                          ; WORD [R1H R1G R1D R1C R0H R0G R0D R0C R1F R1E R1B R1A R0F R0E R0B R0A]
    pmulhrsw        m3, [pw_512]

    ; Row 2
    vbroadcasti128  m4, [r0 + r1 * 2]               ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m5, m4, m2
    pshufb          m4, m1
    pmaddubsw       m4, m0
    pmaddubsw       m5, m0
    phaddw          m4, m5
    ; Row 3
    vbroadcasti128  m5, [r0 + r5]                   ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m6, m5, m2
    pshufb          m5, m1
    pmaddubsw       m5, m0
    pmaddubsw       m6, m0
    phaddw          m5, m6

    phaddw          m4, m5                          ; WORD [R3H R3G R3D R3C R2H R2G R2D R2C R3F R3E R3B R3A R2F R2E R2B R2A]
    pmulhrsw        m4, [pw_512]

    packuswb        m3, m4
    vextracti128    xm4, m3, 1
    punpcklwd       xm5, xm3, xm4

    movq            [r2], xm5
    movhps          [r2 + r3], xm5

    punpckhwd       xm5, xm3, xm4
    movq            [r2 + r3 * 2], xm5
    movhps          [r2 + r4], xm5
    RET

%macro IPFILTER_LUMA_AVX2_8xN 2
INIT_YMM avx2
cglobal interp_8tap_horiz_pp_%1x%2, 4, 7, 7
    mov             r4d, r4m

%ifdef PIC
    lea             r5, [h_tab_LumaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h_tab_LumaCoeff + r4 * 8]
%endif

    mova            m1, [h_tab_Lm]
    mova            m2, [h_tab_Lm + 32]

    ; register map
    ; m0     - interpolate coeff
    ; m1, m2 - shuffle order table

    sub             r0, 3
    lea             r5, [r1 * 3]
    lea             r6, [r3 * 3]
    mov             r4d, %2 / 4
.loop:
    ; Row 0
    vbroadcasti128  m3, [r0]                        ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m4, m3, m2
    pshufb          m3, m1
    pmaddubsw       m3, m0
    pmaddubsw       m4, m0
    phaddw          m3, m4
    ; Row 1
    vbroadcasti128  m4, [r0 + r1]                   ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m5, m4, m2
    pshufb          m4, m1
    pmaddubsw       m4, m0
    pmaddubsw       m5, m0
    phaddw          m4, m5

    phaddw          m3, m4                          ; WORD [R1H R1G R1D R1C R0H R0G R0D R0C R1F R1E R1B R1A R0F R0E R0B R0A]
    pmulhrsw        m3, [pw_512]

    ; Row 2
    vbroadcasti128  m4, [r0 + r1 * 2]               ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m5, m4, m2
    pshufb          m4, m1
    pmaddubsw       m4, m0
    pmaddubsw       m5, m0
    phaddw          m4, m5
    ; Row 3
    vbroadcasti128  m5, [r0 + r5]                   ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb          m6, m5, m2
    pshufb          m5, m1
    pmaddubsw       m5, m0
    pmaddubsw       m6, m0
    phaddw          m5, m6

    phaddw          m4, m5                          ; WORD [R3H R3G R3D R3C R2H R2G R2D R2C R3F R3E R3B R3A R2F R2E R2B R2A]
    pmulhrsw        m4, [pw_512]

    packuswb        m3, m4
    vextracti128    xm4, m3, 1
    punpcklwd       xm5, xm3, xm4

    movq            [r2], xm5
    movhps          [r2 + r3], xm5

    punpckhwd       xm5, xm3, xm4
    movq            [r2 + r3 * 2], xm5
    movhps          [r2 + r6], xm5

    lea             r0, [r0 + r1 * 4]
    lea             r2, [r2 + r3 * 4]
    dec             r4d
    jnz             .loop
    RET
%endmacro

    IPFILTER_LUMA_AVX2_8xN 8, 8
    IPFILTER_LUMA_AVX2_8xN 8, 16
    IPFILTER_LUMA_AVX2_8xN 8, 32

%macro IPFILTER_LUMA_AVX2 2
INIT_YMM avx2
cglobal interp_8tap_horiz_pp_%1x%2, 4,6,8
    sub               r0,        3
    mov               r4d,       r4m
%ifdef PIC
    lea               r5,        [h_tab_LumaCoeff]
    vpbroadcastd      m0,        [r5 + r4 * 8]
    vpbroadcastd      m1,        [r5 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,         [h_tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,         [h_tab_LumaCoeff + r4 * 8 + 4]
%endif
    movu              m3,         [h_tab_Tm + 16]
    vpbroadcastd      m7,         [pw_1]

    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m2  shuffle order table
    ; m7 - pw_1

    mov               r4d,        %2/2
.loop:
    ; Row 0
    vbroadcasti128    m4,         [r0]                        ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,         m4,     m3
    pshufb            m4,         [h_tab_Tm]
    pmaddubsw         m4,         m0
    pmaddubsw         m5,         m1
    paddw             m4,         m5
    pmaddwd           m4,         m7
    vbroadcasti128    m5,         [r0 + 8]                    ; second 8 elements in Row0
    pshufb            m6,         m5,     m3
    pshufb            m5,         [h_tab_Tm]
    pmaddubsw         m5,         m0
    pmaddubsw         m6,         m1
    paddw             m5,         m6
    pmaddwd           m5,         m7
    packssdw          m4,         m5                          ; [17 16 15 14 07 06 05 04 13 12 11 10 03 02 01 00]
    pmulhrsw          m4,         [pw_512]
    vbroadcasti128    m2,         [r0 + r1]                        ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,         m2,     m3
    pshufb            m2,         [h_tab_Tm]
    pmaddubsw         m2,         m0
    pmaddubsw         m5,         m1
    paddw             m2,         m5
    pmaddwd           m2,         m7
    vbroadcasti128    m5,         [r0 + r1 + 8]                    ; second 8 elements in Row0
    pshufb            m6,         m5,     m3
    pshufb            m5,         [h_tab_Tm]
    pmaddubsw         m5,         m0
    pmaddubsw         m6,         m1
    paddw             m5,         m6
    pmaddwd           m5,         m7
    packssdw          m2,         m5                          ; [17 16 15 14 07 06 05 04 13 12 11 10 03 02 01 00]
    pmulhrsw          m2,         [pw_512]
    packuswb          m4,         m2
    vpermq            m4,         m4,     11011000b
    vextracti128      xm5,        m4,     1
    pshufd            xm4,        xm4,    11011000b
    pshufd            xm5,        xm5,    11011000b
    movu              [r2],       xm4
    movu              [r2+r3],    xm5
    lea               r0,         [r0 + r1 * 2]
    lea               r2,         [r2 + r3 * 2]
    dec               r4d
    jnz              .loop
    RET
%endmacro

%macro IPFILTER_LUMA_32x_avx2 2
INIT_YMM avx2
cglobal interp_8tap_horiz_pp_%1x%2, 4,6,8
    sub               r0,         3
    mov               r4d,        r4m
%ifdef PIC
    lea               r5,         [h_tab_LumaCoeff]
    vpbroadcastd      m0,         [r5 + r4 * 8]
    vpbroadcastd      m1,         [r5 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,         [h_tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,         [h_tab_LumaCoeff + r4 * 8 + 4]
%endif
    movu              m3,         [h_tab_Tm + 16]
    vpbroadcastd      m7,         [pw_1]

    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m2  shuffle order table
    ; m7 - pw_1

    mov               r4d,        %2
.loop:
    ; Row 0
    vbroadcasti128    m4,         [r0]                        ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,         m4,     m3
    pshufb            m4,         [h_tab_Tm]
    pmaddubsw         m4,         m0
    pmaddubsw         m5,         m1
    paddw             m4,         m5
    pmaddwd           m4,         m7
    vbroadcasti128    m5,         [r0 + 8]
    pshufb            m6,         m5,     m3
    pshufb            m5,         [h_tab_Tm]
    pmaddubsw         m5,         m0
    pmaddubsw         m6,         m1
    paddw             m5,         m6
    pmaddwd           m5,         m7
    packssdw          m4,         m5                          ; [17 16 15 14 07 06 05 04 13 12 11 10 03 02 01 00]
    pmulhrsw          m4,         [pw_512]
    vbroadcasti128    m2,         [r0 + 16]
    pshufb            m5,         m2,     m3
    pshufb            m2,         [h_tab_Tm]
    pmaddubsw         m2,         m0
    pmaddubsw         m5,         m1
    paddw             m2,         m5
    pmaddwd           m2,         m7
    vbroadcasti128    m5,         [r0 + 24]
    pshufb            m6,         m5,     m3
    pshufb            m5,         [h_tab_Tm]
    pmaddubsw         m5,         m0
    pmaddubsw         m6,         m1
    paddw             m5,         m6
    pmaddwd           m5,         m7
    packssdw          m2,         m5
    pmulhrsw          m2,         [pw_512]
    packuswb          m4,         m2
    vpermq            m4,         m4,     11011000b
    vextracti128      xm5,        m4,     1
    pshufd            xm4,        xm4,    11011000b
    pshufd            xm5,        xm5,    11011000b
    movu              [r2],       xm4
    movu              [r2 + 16],  xm5
    lea               r0,         [r0 + r1]
    lea               r2,         [r2 + r3]
    dec               r4d
    jnz               .loop
    RET
%endmacro
%macro IPFILTER_LUMA_64x_avx2 2
INIT_YMM avx2
cglobal interp_8tap_horiz_pp_%1x%2, 4,6,8
    sub               r0,    3
    mov               r4d,   r4m
%ifdef PIC
    lea               r5,        [h_tab_LumaCoeff]
    vpbroadcastd      m0,        [r5 + r4 * 8]
    vpbroadcastd      m1,        [r5 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,        [h_tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,        [h_tab_LumaCoeff + r4 * 8 + 4]
%endif
    movu              m3,        [h_tab_Tm + 16]
    vpbroadcastd      m7,        [pw_1]

    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m2  shuffle order table
    ; m7 - pw_1

    mov               r4d,   %2
.loop:
    ; Row 0
    vbroadcasti128    m4,        [r0]                        ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,        m4,    m3
    pshufb            m4,        [h_tab_Tm]
    pmaddubsw         m4,        m0
    pmaddubsw         m5,        m1
    paddw             m4,        m5
    pmaddwd           m4,        m7
    vbroadcasti128    m5,        [r0 + 8]
    pshufb            m6,        m5,    m3
    pshufb            m5,        [h_tab_Tm]
    pmaddubsw         m5,        m0
    pmaddubsw         m6,        m1
    paddw             m5,        m6
    pmaddwd           m5,        m7
    packssdw          m4,        m5                          ; [17 16 15 14 07 06 05 04 13 12 11 10 03 02 01 00]
    pmulhrsw          m4,        [pw_512]
    vbroadcasti128    m2,        [r0 + 16]
    pshufb            m5,        m2,    m3
    pshufb            m2,        [h_tab_Tm]
    pmaddubsw         m2,        m0
    pmaddubsw         m5,        m1
    paddw             m2,        m5
    pmaddwd           m2,        m7
    vbroadcasti128    m5,        [r0 + 24]
    pshufb            m6,        m5,    m3
    pshufb            m5,        [h_tab_Tm]
    pmaddubsw         m5,        m0
    pmaddubsw         m6,        m1
    paddw             m5,        m6
    pmaddwd           m5,        m7
    packssdw          m2,        m5
    pmulhrsw          m2,        [pw_512]
    packuswb          m4,        m2
    vpermq            m4,        m4,    11011000b
    vextracti128      xm5,       m4,    1
    pshufd            xm4,       xm4,   11011000b
    pshufd            xm5,       xm5,   11011000b
    movu              [r2],      xm4
    movu              [r2 + 16], xm5

    vbroadcasti128    m4,        [r0 + 32]
    pshufb            m5,        m4,    m3
    pshufb            m4,        [h_tab_Tm]
    pmaddubsw         m4,        m0
    pmaddubsw         m5,        m1
    paddw             m4,        m5
    pmaddwd           m4,        m7
    vbroadcasti128    m5,        [r0 + 40]
    pshufb            m6,        m5,    m3
    pshufb            m5,        [h_tab_Tm]
    pmaddubsw         m5,        m0
    pmaddubsw         m6,        m1
    paddw             m5,        m6
    pmaddwd           m5,        m7
    packssdw          m4,        m5
    pmulhrsw          m4,        [pw_512]
    vbroadcasti128    m2,        [r0 + 48]
    pshufb            m5,        m2,    m3
    pshufb            m2,        [h_tab_Tm]
    pmaddubsw         m2,        m0
    pmaddubsw         m5,        m1
    paddw             m2,        m5
    pmaddwd           m2,        m7
    vbroadcasti128    m5,        [r0 + 56]
    pshufb            m6,        m5,    m3
    pshufb            m5,        [h_tab_Tm]
    pmaddubsw         m5,        m0
    pmaddubsw         m6,        m1
    paddw             m5,        m6
    pmaddwd           m5,        m7
    packssdw          m2,        m5
    pmulhrsw          m2,        [pw_512]
    packuswb          m4,        m2
    vpermq            m4,        m4,    11011000b
    vextracti128      xm5,       m4,    1
    pshufd            xm4,       xm4,   11011000b
    pshufd            xm5,       xm5,   11011000b
    movu              [r2 +32],  xm4
    movu              [r2 + 48], xm5

    lea               r0,        [r0 + r1]
    lea               r2,        [r2 + r3]
    dec               r4d
    jnz               .loop
    RET
%endmacro

INIT_YMM avx2
cglobal interp_8tap_horiz_pp_48x64, 4,6,8
    sub               r0,         3
    mov               r4d,        r4m
%ifdef PIC
    lea               r5,         [h_tab_LumaCoeff]
    vpbroadcastd      m0,         [r5 + r4 * 8]
    vpbroadcastd      m1,         [r5 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,         [h_tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,         [h_tab_LumaCoeff + r4 * 8 + 4]
%endif
    movu              m3,         [h_tab_Tm + 16]
    vpbroadcastd      m7,         [pw_1]

    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m2  shuffle order table
    ; m7 - pw_1

    mov               r4d,        64
.loop:
    ; Row 0
    vbroadcasti128    m4,         [r0]                        ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,         m4,     m3
    pshufb            m4,         [h_tab_Tm]
    pmaddubsw         m4,         m0
    pmaddubsw         m5,         m1
    paddw             m4,         m5
    pmaddwd           m4,         m7
    vbroadcasti128    m5,         [r0 + 8]
    pshufb            m6,         m5,     m3
    pshufb            m5,         [h_tab_Tm]
    pmaddubsw         m5,         m0
    pmaddubsw         m6,         m1
    paddw             m5,         m6
    pmaddwd           m5,         m7
    packssdw          m4,         m5                          ; [17 16 15 14 07 06 05 04 13 12 11 10 03 02 01 00]
    pmulhrsw          m4,         [pw_512]

    vbroadcasti128    m2,         [r0 + 16]
    pshufb            m5,         m2,     m3
    pshufb            m2,         [h_tab_Tm]
    pmaddubsw         m2,         m0
    pmaddubsw         m5,         m1
    paddw             m2,         m5
    pmaddwd           m2,         m7
    vbroadcasti128    m5,         [r0 + 24]
    pshufb            m6,         m5,     m3
    pshufb            m5,         [h_tab_Tm]
    pmaddubsw         m5,         m0
    pmaddubsw         m6,         m1
    paddw             m5,         m6
    pmaddwd           m5,         m7
    packssdw          m2,         m5
    pmulhrsw          m2,         [pw_512]
    packuswb          m4,         m2
    vpermq            m4,         m4,     11011000b
    vextracti128      xm5,        m4,     1
    pshufd            xm4,        xm4,    11011000b
    pshufd            xm5,        xm5,    11011000b
    movu              [r2],       xm4
    movu              [r2 + 16],  xm5

    vbroadcasti128    m4,         [r0 + 32]
    pshufb            m5,         m4,     m3
    pshufb            m4,         [h_tab_Tm]
    pmaddubsw         m4,         m0
    pmaddubsw         m5,         m1
    paddw             m4,         m5
    pmaddwd           m4,         m7
    vbroadcasti128    m5,         [r0 + 40]
    pshufb            m6,         m5,     m3
    pshufb            m5,         [h_tab_Tm]
    pmaddubsw         m5,         m0
    pmaddubsw         m6,         m1
    paddw             m5,         m6
    pmaddwd           m5,         m7
    packssdw          m4,         m5
    pmulhrsw          m4,         [pw_512]
    packuswb          m4,         m4
    vpermq            m4,         m4,     11011000b
    pshufd            xm4,        xm4,    11011000b
    movu              [r2 + 32],  xm4

    lea               r0,         [r0 + r1]
    lea               r2,         [r2 + r3]
    dec               r4d
    jnz               .loop
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_4x4, 4,6,6
    mov             r4d, r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    vpbroadcastd      m2,           [pw_1]
    vbroadcasti128    m1,           [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec                r0

    ; Row 0-1
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    vinserti128       m3,           m3,      [r0 + r1],     1
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    ; Row 2-3
    lea               r0,           [r0 + r1 * 2]
    vbroadcasti128    m4,           [r0]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    vinserti128       m4,           m4,      [r0 + r1],     1
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    pmulhrsw          m3,           [pw_512]
    vextracti128      xm4,          m3,     1
    packuswb          xm3,          xm4

    lea               r0,           [r3 * 3]
    movd              [r2],         xm3
    pextrd            [r2+r3],      xm3,     2
    pextrd            [r2+r3*2],    xm3,     1
    pextrd            [r2+r0],      xm3,     3
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_2x4, 4, 6, 3
    mov               r4d,           r4m

%ifdef PIC
    lea               r5,            [h_tab_ChromaCoeff]
    vpbroadcastd      m0,            [r5 + r4 * 4]
%else
    vpbroadcastd      m0,            [h_tab_ChromaCoeff + r4 * 4]
%endif

    dec               r0
    lea               r4,            [r1 * 3]
    movq              xm1,           [r0]
    movhps            xm1,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m1,            m1,          xm2,          1
    pshufb            m1,            [interp4_hpp_shuf]
    pmaddubsw         m1,            m0
    pmaddwd           m1,            [pw_1]
    vextracti128      xm2,           m1,          1
    packssdw          xm1,           xm2
    pmulhrsw          xm1,           [pw_512]
    packuswb          xm1,           xm1

    lea               r4,            [r3 * 3]
    pextrw            [r2],          xm1,         0
    pextrw            [r2 + r3],     xm1,         1
    pextrw            [r2 + r3 * 2], xm1,         2
    pextrw            [r2 + r4],     xm1,         3
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_2x8, 4, 6, 6
    mov               r4d,           r4m

%ifdef PIC
    lea               r5,            [h_tab_ChromaCoeff]
    vpbroadcastd      m0,            [r5 + r4 * 4]
%else
    vpbroadcastd      m0,            [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m4,            [interp4_hpp_shuf]
    mova              m5,            [pw_1]
    dec               r0
    lea               r4,            [r1 * 3]
    movq              xm1,           [r0]
    movhps            xm1,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m1,            m1,          xm2,          1
    lea               r0,            [r0 + r1 * 4]
    movq              xm3,           [r0]
    movhps            xm3,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m3,            m3,          xm2,          1

    pshufb            m1,            m4
    pshufb            m3,            m4
    pmaddubsw         m1,            m0
    pmaddubsw         m3,            m0
    pmaddwd           m1,            m5
    pmaddwd           m3,            m5
    packssdw          m1,            m3
    pmulhrsw          m1,            [pw_512]
    vextracti128      xm2,           m1,          1
    packuswb          xm1,           xm2

    lea               r4,            [r3 * 3]
    pextrw            [r2],          xm1,         0
    pextrw            [r2 + r3],     xm1,         1
    pextrw            [r2 + r3 * 2], xm1,         4
    pextrw            [r2 + r4],     xm1,         5
    lea               r2,            [r2 + r3 * 4]
    pextrw            [r2],          xm1,         2
    pextrw            [r2 + r3],     xm1,         3
    pextrw            [r2 + r3 * 2], xm1,         6
    pextrw            [r2 + r4],     xm1,         7
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_32x32, 4,6,7
    mov             r4d, r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m1,           [interp4_horiz_shuf1]
    vpbroadcastd      m2,           [pw_1]
    mova              m6,           [pw_512]
    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
    mov               r4d,          32

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 4]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           m6

    vbroadcasti128    m4,           [r0 + 16]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    vbroadcasti128    m5,           [r0 + 20]
    pshufb            m5,           m1
    pmaddubsw         m5,           m0
    pmaddwd           m5,           m2
    packssdw          m4,           m5
    pmulhrsw          m4,           m6

    packuswb          m3,           m4
    vpermq            m3,           m3,      11011000b

    movu              [r2],         m3
    lea               r2,           [r2 + r3]
    lea               r0,           [r0 + r1]
    dec               r4d
    jnz               .loop
    RET


INIT_YMM avx2
cglobal interp_4tap_horiz_pp_16x16, 4, 6, 7
    mov               r4d,          r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m6,           [pw_512]
    mova              m1,           [interp4_horiz_shuf1]
    vpbroadcastd      m2,           [pw_1]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
    mov               r4d,          8

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 4]                    ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           m6

    ; Row 1
    vbroadcasti128    m4,           [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    vbroadcasti128    m5,           [r0 + r1 + 4]               ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,           m1
    pmaddubsw         m5,           m0
    pmaddwd           m5,           m2
    packssdw          m4,           m5
    pmulhrsw          m4,           m6

    packuswb          m3,           m4
    vpermq            m3,           m3,      11011000b

    vextracti128      xm4,          m3,       1
    movu              [r2],         xm3
    movu              [r2 + r3],    xm4
    lea               r2,           [r2 + r3 * 2]
    lea               r0,           [r0 + r1 * 2]
    dec               r4d
    jnz               .loop
    RET

;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
    IPFILTER_LUMA 4, 4, pp
    IPFILTER_LUMA 4, 8, pp
    IPFILTER_LUMA 12, 16, pp
    IPFILTER_LUMA 4, 16, pp

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_8x8, 4,6,6
    mov               r4d,    r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    movu              m1,           [h_tab_Tm]
    vpbroadcastd      m2,           [pw_1]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    sub               r0,           1
    mov               r4d,          2

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    ; Row 1
    vbroadcasti128    m4,           [r0 + r1]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           [pw_512]
    lea               r0,           [r0 + r1 * 2]

    ; Row 2
    vbroadcasti128    m4,           [r0 ]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    ; Row 3
    vbroadcasti128    m5,           [r0 + r1]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,           m1
    pmaddubsw         m5,           m0
    pmaddwd           m5,           m2
    packssdw          m4,           m5
    pmulhrsw          m4,           [pw_512]

    packuswb          m3,           m4
    mova              m5,           [interp_4tap_8x8_horiz_shuf]
    vpermd            m3,           m5,     m3
    vextracti128      xm4,          m3,     1
    movq              [r2],         xm3
    movhps            [r2 + r3],    xm3
    lea               r2,           [r2 + r3 * 2]
    movq              [r2],         xm4
    movhps            [r2 + r3],    xm4
    lea               r2,           [r2 + r3 * 2]
    lea               r0,           [r0 + r1*2]
    dec               r4d
    jnz               .loop
    RET

    IPFILTER_LUMA_AVX2 16, 4
    IPFILTER_LUMA_AVX2 16, 8
    IPFILTER_LUMA_AVX2 16, 12
    IPFILTER_LUMA_AVX2 16, 16
    IPFILTER_LUMA_AVX2 16, 32
    IPFILTER_LUMA_AVX2 16, 64

    IPFILTER_LUMA_32x_avx2 32 , 8
    IPFILTER_LUMA_32x_avx2 32 , 16
    IPFILTER_LUMA_32x_avx2 32 , 24
    IPFILTER_LUMA_32x_avx2 32 , 32
    IPFILTER_LUMA_32x_avx2 32 , 64

    IPFILTER_LUMA_64x_avx2 64 , 64
    IPFILTER_LUMA_64x_avx2 64 , 48
    IPFILTER_LUMA_64x_avx2 64 , 32
    IPFILTER_LUMA_64x_avx2 64 , 16

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_8x2, 4, 6, 5
    mov               r4d,          r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m1,           [h_tab_Tm]
    mova              m2,           [pw_1]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    ; Row 1
    vbroadcasti128    m4,           [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           [pw_512]
    vextracti128      xm4,          m3,          1
    packuswb          xm3,          xm4
    pshufd            xm3,          xm3,         11011000b
    movq              [r2],         xm3
    movhps            [r2 + r3],    xm3
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_8x6, 4, 6, 7
    mov               r4d,           r4m

%ifdef PIC
    lea               r5,            [h_tab_ChromaCoeff]
    vpbroadcastd      m0,            [r5 + r4 * 4]
%else
    vpbroadcastd      m0,            [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m1,            [h_tab_Tm]
    mova              m2,            [pw_1]
    mova              m6,            [pw_512]
    lea               r4,            [r1 * 3]
    lea               r5,            [r3 * 3]
    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
    ; Row 0
    vbroadcasti128    m3,            [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,            m1
    pmaddubsw         m3,            m0
    pmaddwd           m3,            m2

    ; Row 1
    vbroadcasti128    m4,            [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,            m1
    pmaddubsw         m4,            m0
    pmaddwd           m4,            m2
    packssdw          m3,            m4
    pmulhrsw          m3,            m6

    ; Row 2
    vbroadcasti128    m4,            [r0 + r1 * 2]               ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,            m1
    pmaddubsw         m4,            m0
    pmaddwd           m4,            m2

    ; Row 3
    vbroadcasti128    m5,            [r0 + r4]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,            m1
    pmaddubsw         m5,            m0
    pmaddwd           m5,            m2
    packssdw          m4,            m5
    pmulhrsw          m4,            m6

    packuswb          m3,            m4
    mova              m5,            [h_interp8_hps_shuf]
    vpermd            m3,            m5,          m3
    vextracti128      xm4,           m3,          1
    movq              [r2],          xm3
    movhps            [r2 + r3],     xm3
    movq              [r2 + r3 * 2], xm4
    movhps            [r2 + r5],     xm4
    lea               r2,            [r2 + r3 * 4]
    lea               r0,            [r0 + r1 * 4]
    ; Row 4
    vbroadcasti128    m3,            [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,            m1
    pmaddubsw         m3,            m0
    pmaddwd           m3,            m2

    ; Row 5
    vbroadcasti128    m4,            [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,            m1
    pmaddubsw         m4,            m0
    pmaddwd           m4,            m2
    packssdw          m3,            m4
    pmulhrsw          m3,            m6
    vextracti128      xm4,           m3,          1
    packuswb          xm3,           xm4
    pshufd            xm3,           xm3,         11011000b
    movq              [r2],          xm3
    movhps            [r2 + r3],     xm3
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_6x8, 4, 6, 7
    mov               r4d,               r4m

%ifdef PIC
    lea               r5,                [h_tab_ChromaCoeff]
    vpbroadcastd      m0,                [r5 + r4 * 4]
%else
    vpbroadcastd      m0,                [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m1,                [h_tab_Tm]
    mova              m2,                [pw_1]
    mova              m6,                [pw_512]
    lea               r4,                [r1 * 3]
    lea               r5,                [r3 * 3]
    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
%rep 2
    ; Row 0
    vbroadcasti128    m3,                [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,                m1
    pmaddubsw         m3,                m0
    pmaddwd           m3,                m2

    ; Row 1
    vbroadcasti128    m4,                [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,                m1
    pmaddubsw         m4,                m0
    pmaddwd           m4,                m2
    packssdw          m3,                m4
    pmulhrsw          m3,                m6

    ; Row 2
    vbroadcasti128    m4,                [r0 + r1 * 2]               ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,                m1
    pmaddubsw         m4,                m0
    pmaddwd           m4,                m2

    ; Row 3
    vbroadcasti128    m5,                [r0 + r4]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,                m1
    pmaddubsw         m5,                m0
    pmaddwd           m5,                m2
    packssdw          m4,                m5
    pmulhrsw          m4,                m6

    packuswb          m3,                m4
    vextracti128      xm4,               m3,          1
    movd              [r2],              xm3
    pextrw            [r2 + 4],          xm4,         0
    pextrd            [r2 + r3],         xm3,         1
    pextrw            [r2 + r3 + 4],     xm4,         2
    pextrd            [r2 + r3 * 2],     xm3,         2
    pextrw            [r2 + r3 * 2 + 4], xm4,         4
    pextrd            [r2 + r5],         xm3,         3
    pextrw            [r2 + r5 + 4],     xm4,         6
    lea               r2,                [r2 + r3 * 4]
    lea               r0,                [r0 + r1 * 4]
%endrep
    RET

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_64xN(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------;
%macro IPFILTER_CHROMA_HPS_64xN 1
INIT_YMM avx2
cglobal interp_4tap_horiz_ps_64x%1, 4,7,6
    mov             r4d, r4m
    mov             r5d, r5m
    add             r3d, r3d

%ifdef PIC
    lea               r6,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r6 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti128     m2,           [pw_1]
    vbroadcasti128     m5,           [pw_2000]
    mova               m1,           [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1
    mov                r6d,         %1
    dec                r0
    test                r5d,      r5d
    je                 .loop
    sub                r0 ,         r1
    add                r6d ,        3

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 8]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           m5
    vpermq            m3,           m3,          11011000b
    movu              [r2],         m3

    vbroadcasti128    m3,           [r0 + 16]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 24]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           m5
    vpermq            m3,           m3,          11011000b
    movu              [r2 + 32],    m3

    vbroadcasti128    m3,           [r0 + 32]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 40]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           m5
    vpermq            m3,           m3,          11011000b
    movu              [r2 + 64],    m3

    vbroadcasti128    m3,           [r0 + 48]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 56]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           m5
    vpermq            m3,           m3,          11011000b
    movu              [r2 + 96],    m3

    add                r2,           r3
    add                r0,           r1
    dec                r6d
    jnz                .loop
    RET
%endmacro

   IPFILTER_CHROMA_HPS_64xN 64
   IPFILTER_CHROMA_HPS_64xN 32
   IPFILTER_CHROMA_HPS_64xN 48
   IPFILTER_CHROMA_HPS_64xN 16

;-----------------------------------------------------------------------------------------------------------------------------
;void interp_horiz_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------

%macro IPFILTER_LUMA_PS_4xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_horiz_ps_4x%1, 6,7,6
    mov                         r5d,               r5m
    mov                         r4d,               r4m
%ifdef PIC
    lea                         r6,                [h_tab_LumaCoeff]
    vpbroadcastq                m0,                [r6 + r4 * 8]
%else
    vpbroadcastq                m0,                [h_tab_LumaCoeff + r4 * 8]
%endif
    mova                        m1,                [h_tab_Lm]
    add                         r3d,               r3d
    vbroadcasti128              m2,                [pw_2000]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - pw_2000

    sub                         r0,                3
    test                        r5d,               r5d
    mov                         r5d,               %1                           ; loop count variable - height
    jz                         .preloop
    lea                         r6,                [r1 * 3]                     ; r8 = (N / 2 - 1) * srcStride
    sub                         r0,                r6                           ; r0(src) - 3 * srcStride
    add                         r5d,               7                            ; need extra 7 rows, just set a specially flag here, blkheight += N - 1  (7 - 3 = 4 ; since the last three rows not in loop)

.preloop:
    lea                         r6,                [r3 * 3]
.loop:
    ; Row 0-1
    vbroadcasti128              m3,                [r0]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m3,                m1                           ; shuffled based on the col order tab_Lm
    pmaddubsw                   m3,                m0
    vbroadcasti128              m4,                [r0 + r1]                    ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m4,                m1
    pmaddubsw                   m4,                m0
    phaddw                      m3,                m4                           ; DWORD [R1D R1C R0D R0C R1B R1A R0B R0A]

    ; Row 2-3
    lea                         r0,                [r0 + r1 * 2]                ;3rd row(i.e 2nd row)
    vbroadcasti128              m4,                [r0]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m4,                m1
    pmaddubsw                   m4,                m0
    vbroadcasti128              m5,                [r0 + r1]                    ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m5,                m1
    pmaddubsw                   m5,                m0
    phaddw                      m4,                m5                           ; DWORD [R3D R3C R2D R2C R3B R3A R2B R2A]
    phaddw                      m3,                m4                           ; all rows and col completed.

    mova                        m5,                [h_interp8_hps_shuf]
    vpermd                      m3,                m5,               m3
    psubw                       m3,                m2

    vextracti128                xm4,               m3,               1
    movq                        [r2],              xm3                          ;row 0
    movhps                      [r2 + r3],         xm3                          ;row 1
    movq                        [r2 + r3 * 2],     xm4                          ;row 2
    movhps                      [r2 + r6],         xm4                          ;row 3

    lea                         r0,                [r0 + r1 * 2]                ; first loop src ->5th row(i.e 4)
    lea                         r2,                [r2 + r3 * 4]                ; first loop dst ->5th row(i.e 4)
    sub                         r5d,               4
    jz                         .end
    cmp                         r5d,               4
    jge                        .loop

    ; Row 8-9
    vbroadcasti128              m3,                [r0]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m3,                m1
    pmaddubsw                   m3,                m0
    vbroadcasti128              m4,                [r0 + r1]                    ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m4,                m1
    pmaddubsw                   m4,                m0
    phaddw                      m3,                m4                           ; DWORD [R1D R1C R0D R0C R1B R1A R0B R0A]

    ; Row 10
    vbroadcasti128              m4,                [r0 + r1 * 2]                ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m4,                m1
    pmaddubsw                   m4,                m0
    phaddw                      m4,                m4                           ; DWORD [R3D R3C R2D R2C R3B R3A R2B R2A]
    phaddw                      m3,                m4

    vpermd                      m3,                m5,            m3            ; m5 don't broken in above
    psubw                       m3,                m2

    vextracti128                xm4,               m3,            1
    movq                        [r2],              xm3
    movhps                      [r2 + r3],         xm3
    movq                        [r2 + r3 * 2],     xm4
.end:
    RET
%endif
%endmacro

    IPFILTER_LUMA_PS_4xN_AVX2 4
    IPFILTER_LUMA_PS_4xN_AVX2 8
    IPFILTER_LUMA_PS_4xN_AVX2 16

%macro IPFILTER_LUMA_PS_8xN_AVX2 1
; TODO: verify and enable on X86 mode
%if ARCH_X86_64 == 1
; void filter_hps(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
INIT_YMM avx2
cglobal interp_8tap_horiz_ps_8x%1, 4,7,6
    mov                         r5d,        r5m
    mov                         r4d,        r4m
    shl                         r4d,        7
%ifdef PIC
    lea                         r6,         [pb_LumaCoeffVer]
    add                         r6,         r4
%else
    lea                         r6,         [pb_LumaCoeffVer + r4]
%endif
    add                         r3d,        r3d
    vpbroadcastd                m0,         [pw_2000]
    sub                         r0,         3
    lea                         r4,         [pb_8tap_hps_0]
    vbroadcasti128              m5,         [r4 + 0 * mmsize]

    ; check row count extend for interpolateHV
    test                        r5d,        r5d;
    mov                         r5d,        %1
    jz                         .enter_loop
    lea                         r4,         [r1 * 3]                        ; r8 = (N / 2 - 1) * srcStride
    sub                         r0,         r4                              ; r0(src)-r8
    add                         r5d,        8-1-2                           ; blkheight += N - 1  (7 - 3 = 4 ; since the last three rows not in loop)

.enter_loop:
    lea                         r4,         [pb_8tap_hps_0]

    ; ***** register map *****
    ; m0 - pw_2000
    ; r4 - base pointer of shuffle order table
    ; r5 - count of loop
    ; r6 - point to LumaCoeff
.loop:

    ; Row 0-1
    movu                        xm1,        [r0]
    movu                        xm2,        [r0 + r1]
    vinserti128                 m1,         m1,         xm2, 1
    pshufb                      m2,         m1,         m5                  ; [0 1 1 2 2 3 3 4 ...]
    pshufb                      m3,         m1,         [r4 + 1 * mmsize]   ; [2 3 3 4 4 5 5 6 ...]
    pshufb                      m4,         m1,         [r4 + 2 * mmsize]   ; [4 5 5 6 6 7 7 8 ...]
    pshufb                      m1,         m1,         [r4 + 3 * mmsize]   ; [6 7 7 8 8 9 9 A ...]
    pmaddubsw                   m2,         [r6 + 0 * mmsize]
    pmaddubsw                   m3,         [r6 + 1 * mmsize]
    pmaddubsw                   m4,         [r6 + 2 * mmsize]
    pmaddubsw                   m1,         [r6 + 3 * mmsize]
    paddw                       m2,         m3
    paddw                       m1,         m4
    paddw                       m1,         m2
    psubw                       m1,         m0

    vextracti128                xm2,        m1,         1
    movu                        [r2],       xm1                             ; row 0
    movu                        [r2 + r3],  xm2                             ; row 1

    lea                         r0,         [r0 + r1 * 2]                   ; first loop src ->5th row(i.e 4)
    lea                         r2,         [r2 + r3 * 2]                   ; first loop dst ->5th row(i.e 4)
    sub                         r5d,        2
    jg                         .loop
    jz                         .end

    ; last row
    movu                        xm1,        [r0]
    pshufb                      xm2,        xm1,         xm5                ; [0 1 1 2 2 3 3 4 ...]
    pshufb                      xm3,        xm1,         [r4 + 1 * mmsize]  ; [2 3 3 4 4 5 5 6 ...]
    pshufb                      xm4,        xm1,         [r4 + 2 * mmsize]  ; [4 5 5 6 6 7 7 8 ...]
    pshufb                      xm1,        xm1,         [r4 + 3 * mmsize]  ; [6 7 7 8 8 9 9 A ...]
    pmaddubsw                   xm2,        [r6 + 0 * mmsize]
    pmaddubsw                   xm3,        [r6 + 1 * mmsize]
    pmaddubsw                   xm4,        [r6 + 2 * mmsize]
    pmaddubsw                   xm1,        [r6 + 3 * mmsize]
    paddw                       xm2,        xm3
    paddw                       xm1,        xm4
    paddw                       xm1,        xm2
    psubw                       xm1,        xm0
    movu                        [r2],       xm1                          ;row 0
.end:
    RET
%endif
%endmacro ; IPFILTER_LUMA_PS_8xN_AVX2

    IPFILTER_LUMA_PS_8xN_AVX2  4
    IPFILTER_LUMA_PS_8xN_AVX2  8
    IPFILTER_LUMA_PS_8xN_AVX2 16
    IPFILTER_LUMA_PS_8xN_AVX2 32


%macro IPFILTER_LUMA_PS_16x_AVX2 2
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_horiz_ps_%1x%2, 6, 10, 7
    mov                         r5d,               r5m
    mov                         r4d,               r4m
%ifdef PIC
    lea                         r6,                [h_tab_LumaCoeff]
    vpbroadcastq                m0,                [r6 + r4 * 8]
%else
    vpbroadcastq                m0,                [h_tab_LumaCoeff + r4 * 8]
%endif
    mova                        m6,                [h_tab_Lm + 32]
    mova                        m1,                [h_tab_Lm]
    mov                         r9,                %2                           ;height
    add                         r3d,               r3d
    vbroadcasti128              m2,                [pw_2000]

    ; register map
    ; m0      - interpolate coeff
    ; m1 , m6 - shuffle order table
    ; m2      - pw_2000

    xor                         r7,                r7                          ; loop count variable
    sub                         r0,                3
    test                        r5d,               r5d
    jz                          .label
    lea                         r8,                [r1 * 3]                     ; r8 = (N / 2 - 1) * srcStride
    sub                         r0,                r8                           ; r0(src)-r8
    add                         r9,                7                            ; blkheight += N - 1  (7 - 1 = 6 ; since the last one row not in loop)

.label:
    ; Row 0
    vbroadcasti128              m3,                [r0]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m4,                m3,             m6           ; row 0 (col 4 to 7)
    pshufb                      m3,                m1                           ; shuffled based on the col order tab_Lm row 0 (col 0 to 3)
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    phaddw                      m3,                m4                           ; DWORD [R1D R1C R0D R0C R1B R1A R0B R0A]

    vbroadcasti128              m4,                [r0 + 8]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m5,                m4,            m6            ;row 1 (col 4 to 7)
    pshufb                      m4,                m1                           ;row 1 (col 0 to 3)
    pmaddubsw                   m4,                m0
    pmaddubsw                   m5,                m0
    phaddw                      m4,                m5                           ; DWORD [R3D R3C R2D R2C R3B R3A R2B R2A]
    phaddw                      m3,                m4                           ; all rows and col completed.

    mova                        m5,                [h_interp8_hps_shuf]
    vpermd                      m3,                m5,               m3
    psubw                       m3,                m2

    movu                        [r2],              m3                          ;row 0

    lea                         r0,                [r0 + r1]                ; first loop src ->5th row(i.e 4)
    lea                         r2,                [r2 + r3]                ; first loop dst ->5th row(i.e 4)
    dec                         r9d
    jnz                         .label

    RET
%endif
%endmacro


    IPFILTER_LUMA_PS_16x_AVX2 16 , 16
    IPFILTER_LUMA_PS_16x_AVX2 16 , 8
    IPFILTER_LUMA_PS_16x_AVX2 16 , 12
    IPFILTER_LUMA_PS_16x_AVX2 16 , 4
    IPFILTER_LUMA_PS_16x_AVX2 16 , 32
    IPFILTER_LUMA_PS_16x_AVX2 16 , 64


;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro IPFILTER_LUMA_PP_W8 2
INIT_XMM sse4
cglobal interp_8tap_horiz_pp_%1x%2, 4,6,7
    mov         r4d, r4m

%ifdef PIC
    lea         r5, [h_tab_LumaCoeff]
    movh        m3, [r5 + r4 * 8]
%else
    movh        m3, [h_tab_LumaCoeff + r4 * 8]
%endif
    pshufd      m0, m3, 0                       ; m0 = coeff-L
    pshufd      m1, m3, 0x55                    ; m1 = coeff-H
    lea         r5, [h_tab_Tm]                    ; r5 = shuffle
    mova        m2, [pw_512]                    ; m2 = 512

    mov         r4d, %2
.loopH:
%assign x 0
%rep %1 / 8
    movu        m3, [r0 - 3 + x]                ; m3 = [F E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb      m4, m3, [r5 + 0*16]             ; m4 = [6 5 4 3 5 4 3 2 4 3 2 1 3 2 1 0]
    pshufb      m5, m3, [r5 + 1*16]             ; m5 = [A 9 8 7 9 8 7 6 8 7 6 5 7 6 5 4]
    pshufb          m3, [r5 + 2*16]             ; m3 = [E D C B D C B A C B A 9 B A 9 8]
    pmaddubsw   m4, m0
    pmaddubsw   m6, m5, m1
    pmaddubsw   m5, m0
    pmaddubsw   m3, m1
    paddw       m4, m6
    paddw       m5, m3
    phaddw      m4, m5
    pmulhrsw    m4, m2
    packuswb    m4, m4
    movh        [r2 + x], m4
%assign x x+8
%endrep

    add       r0, r1
    add       r2, r3

    dec       r4d
    jnz      .loopH
    RET
%endmacro

    IPFILTER_LUMA_PP_W8      8,  4
    IPFILTER_LUMA_PP_W8      8,  8
    IPFILTER_LUMA_PP_W8      8, 16
    IPFILTER_LUMA_PP_W8      8, 32
    IPFILTER_LUMA_PP_W8     16,  4
    IPFILTER_LUMA_PP_W8     16,  8
    IPFILTER_LUMA_PP_W8     16, 12
    IPFILTER_LUMA_PP_W8     16, 16
    IPFILTER_LUMA_PP_W8     16, 32
    IPFILTER_LUMA_PP_W8     16, 64
    IPFILTER_LUMA_PP_W8     24, 32
    IPFILTER_LUMA_PP_W8     32,  8
    IPFILTER_LUMA_PP_W8     32, 16
    IPFILTER_LUMA_PP_W8     32, 24
    IPFILTER_LUMA_PP_W8     32, 32
    IPFILTER_LUMA_PP_W8     32, 64
    IPFILTER_LUMA_PP_W8     48, 64
    IPFILTER_LUMA_PP_W8     64, 16
    IPFILTER_LUMA_PP_W8     64, 32
    IPFILTER_LUMA_PP_W8     64, 48
    IPFILTER_LUMA_PP_W8     64, 64

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_ps_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
    IPFILTER_LUMA 4, 4, ps
    IPFILTER_LUMA 8, 8, ps
    IPFILTER_LUMA 8, 4, ps
    IPFILTER_LUMA 4, 8, ps
    IPFILTER_LUMA 16, 16, ps
    IPFILTER_LUMA 16, 8, ps
    IPFILTER_LUMA 8, 16, ps
    IPFILTER_LUMA 16, 12, ps
    IPFILTER_LUMA 12, 16, ps
    IPFILTER_LUMA 16, 4, ps
    IPFILTER_LUMA 4, 16, ps
    IPFILTER_LUMA 32, 32, ps
    IPFILTER_LUMA 32, 16, ps
    IPFILTER_LUMA 16, 32, ps
    IPFILTER_LUMA 32, 24, ps
    IPFILTER_LUMA 24, 32, ps
    IPFILTER_LUMA 32, 8, ps
    IPFILTER_LUMA 8, 32, ps
    IPFILTER_LUMA 64, 64, ps
    IPFILTER_LUMA 64, 32, ps
    IPFILTER_LUMA 32, 64, ps
    IPFILTER_LUMA 64, 48, ps
    IPFILTER_LUMA 48, 64, ps
    IPFILTER_LUMA 64, 16, ps
    IPFILTER_LUMA 16, 64, ps

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_2x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------
%macro FILTER_HORIZ_CHROMA_2xN 2
INIT_XMM sse4
cglobal interp_4tap_horiz_ps_%1x%2, 4, 7, 4, src, srcstride, dst, dststride
%define coef2  m3
%define Tm0    m2
%define t1     m1
%define t0     m0

    dec        srcq
    mov        r4d, r4m
    add        dststrided, dststrided

%ifdef PIC
    lea        r6, [h_tab_ChromaCoeff]
    movd       coef2, [r6 + r4 * 4]
%else
    movd       coef2, [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd     coef2, coef2, 0
    mova       t1, [pw_2000]
    mova       Tm0, [h_tab_Tm]

    mov        r4d, %2
    cmp        r5m, byte 0
    je         .loopH
    sub        srcq, srcstrideq
    add        r4d, 3

.loopH:
    movh       t0, [srcq]
    pshufb     t0, t0, Tm0
    pmaddubsw  t0, coef2
    phaddw     t0, t0
    psubw      t0, t1
    movd       [dstq], t0

    lea        srcq, [srcq + srcstrideq]
    lea        dstq, [dstq + dststrideq]

    dec        r4d
    jnz        .loopH

    RET
%endmacro

    FILTER_HORIZ_CHROMA_2xN 2, 4
    FILTER_HORIZ_CHROMA_2xN 2, 8

    FILTER_HORIZ_CHROMA_2xN 2, 16

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_4x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------
%macro FILTER_HORIZ_CHROMA_4xN 2
INIT_XMM sse4
cglobal interp_4tap_horiz_ps_%1x%2, 4, 7, 4, src, srcstride, dst, dststride
%define coef2  m3
%define Tm0    m2
%define t1     m1
%define t0     m0

    dec        srcq
    mov        r4d, r4m
    add        dststrided, dststrided

%ifdef PIC
    lea        r6, [h_tab_ChromaCoeff]
    movd       coef2, [r6 + r4 * 4]
%else
    movd       coef2, [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd     coef2, coef2, 0
    mova       t1, [pw_2000]
    mova       Tm0, [h_tab_Tm]

    mov        r4d, %2
    cmp        r5m, byte 0
    je         .loopH
    sub        srcq, srcstrideq
    add        r4d, 3

.loopH:
    movh       t0, [srcq]
    pshufb     t0, t0, Tm0
    pmaddubsw  t0, coef2
    phaddw     t0, t0
    psubw      t0, t1
    movlps     [dstq], t0

    lea        srcq, [srcq + srcstrideq]
    lea        dstq, [dstq + dststrideq]

    dec        r4d
    jnz        .loopH
    RET
%endmacro

    FILTER_HORIZ_CHROMA_4xN 4, 2
    FILTER_HORIZ_CHROMA_4xN 4, 4
    FILTER_HORIZ_CHROMA_4xN 4, 8
    FILTER_HORIZ_CHROMA_4xN 4, 16

    FILTER_HORIZ_CHROMA_4xN 4, 32

%macro PROCESS_CHROMA_W6 3
    movu       %1, [srcq]
    pshufb     %2, %1, Tm0
    pmaddubsw  %2, coef2
    pshufb     %1, %1, Tm1
    pmaddubsw  %1, coef2
    phaddw     %2, %1
    psubw      %2, %3
    movh       [dstq], %2
    pshufd     %2, %2, 2
    movd       [dstq + 8], %2
%endmacro

%macro PROCESS_CHROMA_W12 3
    movu       %1, [srcq]
    pshufb     %2, %1, Tm0
    pmaddubsw  %2, coef2
    pshufb     %1, %1, Tm1
    pmaddubsw  %1, coef2
    phaddw     %2, %1
    psubw      %2, %3
    movu       [dstq], %2
    movu       %1, [srcq + 8]
    pshufb     %1, %1, Tm0
    pmaddubsw  %1, coef2
    phaddw     %1, %1
    psubw      %1, %3
    movh       [dstq + 16], %1
%endmacro

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_6x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------
%macro FILTER_HORIZ_CHROMA 2
INIT_XMM sse4
cglobal interp_4tap_horiz_ps_%1x%2, 4, 7, 6, src, srcstride, dst, dststride
%define coef2    m5
%define Tm0      m4
%define Tm1      m3
%define t2       m2
%define t1       m1
%define t0       m0

    dec     srcq
    mov     r4d, r4m
    add     dststrided, dststrided

%ifdef PIC
    lea     r6, [h_tab_ChromaCoeff]
    movd    coef2, [r6 + r4 * 4]
%else
    movd    coef2, [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd  coef2, coef2, 0
    mova    t2, [pw_2000]
    mova    Tm0, [h_tab_Tm]
    mova    Tm1, [h_tab_Tm + 16]

    mov     r4d, %2
    cmp     r5m, byte 0
    je      .loopH
    sub     srcq, srcstrideq
    add     r4d, 3

.loopH:
    PROCESS_CHROMA_W%1  t0, t1, t2
    add     srcq, srcstrideq
    add     dstq, dststrideq

    dec     r4d
    jnz     .loopH

    RET
%endmacro

    FILTER_HORIZ_CHROMA 6, 8
    FILTER_HORIZ_CHROMA 12, 16

    FILTER_HORIZ_CHROMA 6, 16
    FILTER_HORIZ_CHROMA 12, 32

%macro PROCESS_CHROMA_W8 3
    movu        %1, [srcq]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    psubw       %2, %3
    movu        [dstq], %2
%endmacro

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_8x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------
%macro FILTER_HORIZ_CHROMA_8xN 2
INIT_XMM sse4
cglobal interp_4tap_horiz_ps_%1x%2, 4, 7, 6, src, srcstride, dst, dststride
%define coef2    m5
%define Tm0      m4
%define Tm1      m3
%define t2       m2
%define t1       m1
%define t0       m0

    dec     srcq
    mov     r4d, r4m
    add     dststrided, dststrided

%ifdef PIC
    lea     r6, [h_tab_ChromaCoeff]
    movd    coef2, [r6 + r4 * 4]
%else
    movd    coef2, [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd  coef2, coef2, 0
    mova    t2, [pw_2000]
    mova    Tm0, [h_tab_Tm]
    mova    Tm1, [h_tab_Tm + 16]

    mov     r4d, %2
    cmp     r5m, byte 0
    je      .loopH
    sub     srcq, srcstrideq
    add     r4d, 3

.loopH:
    PROCESS_CHROMA_W8  t0, t1, t2
    add     srcq, srcstrideq
    add     dstq, dststrideq

    dec     r4d
    jnz     .loopH

    RET
%endmacro

    FILTER_HORIZ_CHROMA_8xN 8, 2
    FILTER_HORIZ_CHROMA_8xN 8, 4
    FILTER_HORIZ_CHROMA_8xN 8, 6
    FILTER_HORIZ_CHROMA_8xN 8, 8
    FILTER_HORIZ_CHROMA_8xN 8, 16
    FILTER_HORIZ_CHROMA_8xN 8, 32

    FILTER_HORIZ_CHROMA_8xN 8, 12
    FILTER_HORIZ_CHROMA_8xN 8, 64

%macro PROCESS_CHROMA_W16 4
    movu        %1, [srcq]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    movu        %1, [srcq + 8]
    pshufb      %4, %1, Tm0
    pmaddubsw   %4, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %4, %1
    psubw       %2, %3
    psubw       %4, %3
    movu        [dstq], %2
    movu        [dstq + 16], %4
%endmacro

%macro PROCESS_CHROMA_W24 4
    movu        %1, [srcq]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    movu        %1, [srcq + 8]
    pshufb      %4, %1, Tm0
    pmaddubsw   %4, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %4, %1
    psubw       %2, %3
    psubw       %4, %3
    movu        [dstq], %2
    movu        [dstq + 16], %4
    movu        %1, [srcq + 16]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    psubw       %2, %3
    movu        [dstq + 32], %2
%endmacro

%macro PROCESS_CHROMA_W32 4
    movu        %1, [srcq]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    movu        %1, [srcq + 8]
    pshufb      %4, %1, Tm0
    pmaddubsw   %4, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %4, %1
    psubw       %2, %3
    psubw       %4, %3
    movu        [dstq], %2
    movu        [dstq + 16], %4
    movu        %1, [srcq + 16]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    movu        %1, [srcq + 24]
    pshufb      %4, %1, Tm0
    pmaddubsw   %4, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %4, %1
    psubw       %2, %3
    psubw       %4, %3
    movu        [dstq + 32], %2
    movu        [dstq + 48], %4
%endmacro

%macro PROCESS_CHROMA_W16o 5
    movu        %1, [srcq + %5]
    pshufb      %2, %1, Tm0
    pmaddubsw   %2, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    movu        %1, [srcq + %5 + 8]
    pshufb      %4, %1, Tm0
    pmaddubsw   %4, coef2
    pshufb      %1, %1, Tm1
    pmaddubsw   %1, coef2
    phaddw      %4, %1
    psubw       %2, %3
    psubw       %4, %3
    movu        [dstq + %5 * 2], %2
    movu        [dstq + %5 * 2 + 16], %4
%endmacro

%macro PROCESS_CHROMA_W48 4
    PROCESS_CHROMA_W16o %1, %2, %3, %4, 0
    PROCESS_CHROMA_W16o %1, %2, %3, %4, 16
    PROCESS_CHROMA_W16o %1, %2, %3, %4, 32
%endmacro

%macro PROCESS_CHROMA_W64 4
    PROCESS_CHROMA_W16o %1, %2, %3, %4, 0
    PROCESS_CHROMA_W16o %1, %2, %3, %4, 16
    PROCESS_CHROMA_W16o %1, %2, %3, %4, 32
    PROCESS_CHROMA_W16o %1, %2, %3, %4, 48
%endmacro

;------------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_%1x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;------------------------------------------------------------------------------------------------------------------------------
%macro FILTER_HORIZ_CHROMA_WxN 2
INIT_XMM sse4
cglobal interp_4tap_horiz_ps_%1x%2, 4, 7, 7, src, srcstride, dst, dststride
%define coef2    m6
%define Tm0      m5
%define Tm1      m4
%define t3       m3
%define t2       m2
%define t1       m1
%define t0       m0

    dec     srcq
    mov     r4d, r4m
    add     dststrided, dststrided

%ifdef PIC
    lea     r6, [h_tab_ChromaCoeff]
    movd    coef2, [r6 + r4 * 4]
%else
    movd    coef2, [h_tab_ChromaCoeff + r4 * 4]
%endif

    pshufd  coef2, coef2, 0
    mova    t2, [pw_2000]
    mova    Tm0, [h_tab_Tm]
    mova    Tm1, [h_tab_Tm + 16]

    mov     r4d, %2
    cmp     r5m, byte 0
    je      .loopH
    sub     srcq, srcstrideq
    add     r4d, 3

.loopH:
    PROCESS_CHROMA_W%1   t0, t1, t2, t3
    add     srcq, srcstrideq
    add     dstq, dststrideq

    dec     r4d
    jnz     .loopH

    RET
%endmacro

    FILTER_HORIZ_CHROMA_WxN 16, 4
    FILTER_HORIZ_CHROMA_WxN 16, 8
    FILTER_HORIZ_CHROMA_WxN 16, 12
    FILTER_HORIZ_CHROMA_WxN 16, 16
    FILTER_HORIZ_CHROMA_WxN 16, 32
    FILTER_HORIZ_CHROMA_WxN 24, 32
    FILTER_HORIZ_CHROMA_WxN 32,  8
    FILTER_HORIZ_CHROMA_WxN 32, 16
    FILTER_HORIZ_CHROMA_WxN 32, 24
    FILTER_HORIZ_CHROMA_WxN 32, 32

    FILTER_HORIZ_CHROMA_WxN 16, 24
    FILTER_HORIZ_CHROMA_WxN 16, 64
    FILTER_HORIZ_CHROMA_WxN 24, 64
    FILTER_HORIZ_CHROMA_WxN 32, 48
    FILTER_HORIZ_CHROMA_WxN 32, 64

    FILTER_HORIZ_CHROMA_WxN 64, 64
    FILTER_HORIZ_CHROMA_WxN 64, 32
    FILTER_HORIZ_CHROMA_WxN 64, 48
    FILTER_HORIZ_CHROMA_WxN 48, 64
    FILTER_HORIZ_CHROMA_WxN 64, 16

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_32x32(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------;
INIT_YMM avx2
cglobal interp_4tap_horiz_ps_32x32, 4,6,8
    mov             r4d, r4m
    add             r3d, r3d
    dec             r0

    ; check isRowExt
    cmp             r5m, byte 0

    lea             r5, [h_tab_ChromaCoeff]
    vpbroadcastw    m0, [r5 + r4 * 4 + 0]
    vpbroadcastw    m1, [r5 + r4 * 4 + 2]
    mova            m7, [pw_2000]

    ; register map
    ; m0 - interpolate coeff Low
    ; m1 - interpolate coeff High
    ; m7 - constant pw_2000
    mov             r4d, 32
    je             .loop
    sub             r0, r1
    add             r4d, 3

.loop:
    ; Row 0
    movu            m2, [r0]
    movu            m3, [r0 + 1]
    punpckhbw       m4, m2, m3
    punpcklbw       m2, m3
    pmaddubsw       m4, m0
    pmaddubsw       m2, m0

    movu            m3, [r0 + 2]
    movu            m5, [r0 + 3]
    punpckhbw       m6, m3, m5
    punpcklbw       m3, m5
    pmaddubsw       m6, m1
    pmaddubsw       m3, m1

    paddw           m4, m6
    paddw           m2, m3
    psubw           m4, m7
    psubw           m2, m7
    vperm2i128      m3, m2, m4, 0x20
    vperm2i128      m5, m2, m4, 0x31
    movu            [r2], m3
    movu            [r2 + mmsize], m5

    add             r2, r3
    add             r0, r1
    dec             r4d
    jnz            .loop
    RET

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_16x16(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------;
INIT_YMM avx2
cglobal interp_4tap_horiz_ps_16x16, 4,7,6
    mov             r4d, r4m
    mov             r5d, r5m
    add             r3d, r3d

%ifdef PIC
    lea               r6,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r6 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti128     m2,           [pw_1]
    vbroadcasti128     m5,           [pw_2000]
    mova               m1,           [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1
    mov                r6d,         16
    dec                r0
    test                r5d,        r5d
    je                 .loop
    sub                r0 ,         r1
    add                r6d ,        3

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 8]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           m5
    vpermq            m3,           m3,          11011000b
    movu              [r2],         m3

    add                r2,          r3
    add                r0,          r1
    dec                r6d
    jnz                .loop
    RET

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_16xN(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PS_16xN_AVX2 2
INIT_YMM avx2
cglobal interp_4tap_horiz_ps_%1x%2, 4,7,6
    mov                    r4d,        r4m
    mov                    r5d,        r5m
    add                    r3d,        r3d

%ifdef PIC
    lea                    r6,         [h_tab_ChromaCoeff]
    vpbroadcastd           m0,         [r6 + r4 * 4]
%else
    vpbroadcastd           m0,         [h_tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti128         m2,         [pw_1]
    vbroadcasti128         m5,         [pw_2000]
    mova                   m1,         [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1
    mov                    r6d,        %2
    dec                    r0
    test                   r5d,        r5d
    je                     .loop
    sub                    r0 ,        r1
    add                    r6d ,       3

.loop:
    ; Row 0
    vbroadcasti128         m3,         [r0]
    pshufb                 m3,         m1
    pmaddubsw              m3,         m0
    pmaddwd                m3,         m2
    vbroadcasti128         m4,         [r0 + 8]
    pshufb                 m4,         m1
    pmaddubsw              m4,         m0
    pmaddwd                m4,         m2

    packssdw               m3,         m4
    psubw                  m3,         m5

    vpermq                 m3,         m3,          11011000b
    movu                   [r2],       m3

    add                    r2,         r3
    add                    r0,         r1
    dec                    r6d
    jnz                    .loop
    RET
%endmacro

    IPFILTER_CHROMA_PS_16xN_AVX2  16 , 32
    IPFILTER_CHROMA_PS_16xN_AVX2  16 , 12
    IPFILTER_CHROMA_PS_16xN_AVX2  16 , 8
    IPFILTER_CHROMA_PS_16xN_AVX2  16 , 4
    IPFILTER_CHROMA_PS_16xN_AVX2  16 , 24
    IPFILTER_CHROMA_PS_16xN_AVX2  16 , 64

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_32xN(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PS_32xN_AVX2 2
INIT_YMM avx2
cglobal interp_4tap_horiz_ps_%1x%2, 4,7,6
    mov                r4d,          r4m
    mov                r5d,          r5m
    add                r3d,          r3d

%ifdef PIC
    lea                r6,           [h_tab_ChromaCoeff]
    vpbroadcastd       m0,           [r6 + r4 * 4]
%else
    vpbroadcastd       m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti128     m2,           [pw_1]
    vbroadcasti128     m5,           [pw_2000]
    mova               m1,           [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1
    mov                r6d,          %2
    dec                r0
    test               r5d,          r5d
    je                 .loop
    sub                r0 ,          r1
    add                r6d ,         3

.loop:
    ; Row 0
    vbroadcasti128     m3,           [r0]
    pshufb             m3,           m1
    pmaddubsw          m3,           m0
    pmaddwd            m3,           m2
    vbroadcasti128     m4,           [r0 + 8]
    pshufb             m4,           m1
    pmaddubsw          m4,           m0
    pmaddwd            m4,           m2

    packssdw           m3,           m4
    psubw              m3,           m5

    vpermq             m3,           m3,          11011000b
    movu              [r2],          m3

    vbroadcasti128     m3,           [r0 + 16]
    pshufb             m3,           m1
    pmaddubsw          m3,           m0
    pmaddwd            m3,           m2
    vbroadcasti128     m4,           [r0 + 24]
    pshufb             m4,           m1
    pmaddubsw          m4,           m0
    pmaddwd            m4,           m2

    packssdw           m3,           m4
    psubw              m3,           m5

    vpermq             m3,           m3,          11011000b
    movu               [r2 + 32],    m3

    add                r2,           r3
    add                r0,           r1
    dec                r6d
    jnz                .loop
    RET
%endmacro

    IPFILTER_CHROMA_PS_32xN_AVX2  32 , 16
    IPFILTER_CHROMA_PS_32xN_AVX2  32 , 24
    IPFILTER_CHROMA_PS_32xN_AVX2  32 , 8
    IPFILTER_CHROMA_PS_32xN_AVX2  32 , 64
    IPFILTER_CHROMA_PS_32xN_AVX2  32 , 48

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_4x4(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
cglobal interp_4tap_horiz_ps_4x4, 4,7,5
    mov             r4d, r4m
    mov             r5d, r5m
    add             r3d, r3d

%ifdef PIC
    lea               r6,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r6 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti128     m2,           [pw_1]
    vbroadcasti128     m1,           [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec                r0
    test                r5d,       r5d
    je                 .label
    sub                r0 , r1

.label:
    ; Row 0-1
    movu              xm3,           [r0]
    vinserti128       m3,           m3,      [r0 + r1],     1
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    ; Row 2-3
    lea               r0,           [r0 + r1 * 2]
    movu              xm4,           [r0]
    vinserti128       m4,           m4,      [r0 + r1],     1
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           [pw_2000]
    vextracti128      xm4,          m3,     1
    movq              [r2],         xm3
    movq              [r2+r3],      xm4
    lea               r2,           [r2 + r3 * 2]
    movhps            [r2],         xm3
    movhps            [r2 + r3],    xm4

    test                r5d,        r5d
    jz                .end
    lea               r2,           [r2 + r3 * 2]
    lea               r0,           [r0 + r1 * 2]

    ;Row 5-6
    movu              xm3,          [r0]
    vinserti128       m3,           m3,      [r0 + r1],     1
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    ; Row 7
    lea               r0,           [r0 + r1 * 2]
    vbroadcasti128    m4,           [r0]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           [pw_2000]

    vextracti128      xm4,          m3,     1
    movq              [r2],         xm3
    movq              [r2+r3],      xm4
    lea               r2,           [r2 + r3 * 2]
    movhps            [r2],         xm3
.end:
    RET

cglobal interp_4tap_horiz_ps_4x2, 4,7,5
    mov             r4d, r4m
    mov             r5d, r5m
    add             r3d, r3d

%ifdef PIC
    lea               r6,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r6 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti128     m2,           [pw_1]
    vbroadcasti128     m1,           [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec                r0
    test                r5d,       r5d
    je                 .label
    sub                r0 , r1

.label:
    ; Row 0-1
    movu              xm3,           [r0]
    vinserti128       m3,           m3,      [r0 + r1],     1
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    packssdw          m3,           m3
    psubw             m3,           [pw_2000]
    vextracti128      xm4,          m3,     1
    movq              [r2],         xm3
    movq              [r2+r3],      xm4

    test              r5d,          r5d
    jz                .end
    lea               r2,           [r2 + r3 * 2]
    lea               r0,           [r0 + r1 * 2]

    ;Row 2-3
    movu              xm3,          [r0]
    vinserti128       m3,           m3,      [r0 + r1],     1
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    ; Row 5
    lea               r0,           [r0 + r1 * 2]
    vbroadcasti128    m4,           [r0]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           [pw_2000]

    vextracti128      xm4,          m3,     1
    movq              [r2],         xm3
    movq              [r2+r3],      xm4
    lea               r2,           [r2 + r3 * 2]
    movhps            [r2],         xm3
.end:
    RET

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_4xN(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------;
%macro IPFILTER_CHROMA_PS_4xN_AVX2 2
INIT_YMM avx2
cglobal interp_4tap_horiz_ps_%1x%2, 4,7,5
    mov             r4d, r4m
    mov             r5d, r5m
    add             r3d, r3d

%ifdef PIC
    lea               r6,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r6 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti128     m2,           [pw_1]
    vbroadcasti128     m1,           [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    mov              r4,                %2
    dec              r0
    test             r5d,       r5d
    je               .loop
    sub              r0 ,               r1


.loop:
    sub              r4d,           4
    ; Row 0-1
    movu              xm3,          [r0]
    vinserti128       m3,           m3,      [r0 + r1],     1
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    ; Row 2-3
    lea               r0,           [r0 + r1 * 2]
    movu              xm4,          [r0]
    vinserti128       m4,           m4,      [r0 + r1],     1
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           [pw_2000]
    vextracti128      xm4,          m3,     1
    movq              [r2],         xm3
    movq              [r2+r3],      xm4
    lea               r2,           [r2 + r3 * 2]
    movhps            [r2],         xm3
    movhps            [r2 + r3],    xm4

    lea               r2,           [r2 + r3 * 2]
    lea               r0,           [r0 + r1 * 2]

    test              r4d,          r4d
    jnz               .loop
    test                r5d,        r5d
    jz                .end

    ;Row 5-6
    movu              xm3,          [r0]
    vinserti128       m3,           m3,      [r0 + r1],     1
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    ; Row 7
    lea               r0,           [r0 + r1 * 2]
    vbroadcasti128    m4,           [r0]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           [pw_2000]

    vextracti128      xm4,          m3,     1
    movq              [r2],         xm3
    movq              [r2+r3],      xm4
    lea               r2,           [r2 + r3 * 2]
    movhps            [r2],         xm3
.end:
    RET
%endmacro

    IPFILTER_CHROMA_PS_4xN_AVX2  4 , 8
    IPFILTER_CHROMA_PS_4xN_AVX2  4 , 16

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_8x8(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------;
INIT_YMM avx2
cglobal interp_4tap_horiz_ps_8x8, 4,7,6
    mov             r4d, r4m
    mov             r5d, r5m
    add             r3d, r3d

%ifdef PIC
    lea               r6,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r6 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti128     m2,           [pw_1]
    vbroadcasti128     m5,           [pw_2000]
    mova               m1,           [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    mov                r6d,      4
    dec                r0
    test                r5d,     r5d
    je                 .loop
    sub                r0 ,      r1
    add                r6d ,     1

.loop:
     dec               r6d
    ; Row 0
    vbroadcasti128    m3,           [r0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    ; Row 1
    vbroadcasti128    m4,           [r0 + r1]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           m5

    vpermq            m3,           m3,          11011000b
    vextracti128      xm4,          m3,     1
    movu             [r2],         xm3
    movu             [r2 + r3],    xm4

    lea               r2,           [r2 + r3 * 2]
    lea               r0,           [r0 + r1 * 2]
    test               r6d,          r6d
    jnz               .loop
    test              r5d,         r5d
    je                .end

    ;Row 11
    vbroadcasti128    m3,           [r0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    packssdw          m3,           m3
    psubw             m3,           m5
    vpermq            m3,           m3,          11011000b
    movu             [r2],         xm3
.end:
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_4x2, 4,6,4
    mov             r4d, r4m
%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti128    m1,           [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table

    ; Row 0-1
    movu              xm2,          [r0 - 1]
    vinserti128       m2,           m2,      [r0 + r1 - 1],     1
    pshufb            m2,           m1
    pmaddubsw         m2,           m0
    pmaddwd           m2,           [pw_1]

    packssdw          m2,           m2
    pmulhrsw          m2,           [pw_512]
    vextracti128      xm3,          m2,     1
    packuswb          xm2,          xm3

    movd              [r2],         xm2
    pextrd            [r2+r3],      xm2,     2
    RET

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp_32xN(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PP_32xN_AVX2 2
INIT_YMM avx2
cglobal interp_4tap_horiz_pp_%1x%2, 4,6,7
    mov             r4d, r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m1,           [interp4_horiz_shuf1]
    vpbroadcastd      m2,           [pw_1]
    mova              m6,           [pw_512]
    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
    mov               r4d,          %2

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 4]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           m6

    vbroadcasti128    m4,           [r0 + 16]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    vbroadcasti128    m5,           [r0 + 20]
    pshufb            m5,           m1
    pmaddubsw         m5,           m0
    pmaddwd           m5,           m2
    packssdw          m4,           m5
    pmulhrsw          m4,           m6

    packuswb          m3,           m4
    vpermq            m3,           m3,      11011000b

    movu              [r2],         m3
    add               r2,           r3
    add               r0,           r1
    dec               r4d
    jnz               .loop
    RET
%endmacro

    IPFILTER_CHROMA_PP_32xN_AVX2 32, 16
    IPFILTER_CHROMA_PP_32xN_AVX2 32, 24
    IPFILTER_CHROMA_PP_32xN_AVX2 32, 8
    IPFILTER_CHROMA_PP_32xN_AVX2 32, 64
    IPFILTER_CHROMA_PP_32xN_AVX2 32, 48

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp_8xN(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PP_8xN_AVX2 2
INIT_YMM avx2
cglobal interp_4tap_horiz_pp_%1x%2, 4,6,6
    mov               r4d,    r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    movu              m1,           [h_tab_Tm]
    vpbroadcastd      m2,           [pw_1]
    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    sub               r0,           1
    mov               r4d,          %2

.loop:
    sub               r4d,          4
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    ; Row 1
    vbroadcasti128    m4,           [r0 + r1]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           [pw_512]
    lea               r0,           [r0 + r1 * 2]

    ; Row 2
    vbroadcasti128    m4,           [r0 ]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    ; Row 3
    vbroadcasti128    m5,           [r0 + r1]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,           m1
    pmaddubsw         m5,           m0
    pmaddwd           m5,           m2
    packssdw          m4,           m5
    pmulhrsw          m4,           [pw_512]

    packuswb          m3,           m4
    mova              m5,           [interp_4tap_8x8_horiz_shuf]
    vpermd            m3,           m5,     m3
    vextracti128      xm4,          m3,     1
    movq              [r2],         xm3
    movhps            [r2 + r3],    xm3
    lea               r2,           [r2 + r3 * 2]
    movq              [r2],         xm4
    movhps            [r2 + r3],    xm4
    lea               r2,           [r2 + r3 * 2]
    lea               r0,           [r0 + r1*2]
    test              r4d,          r4d
    jnz               .loop
    RET
%endmacro

    IPFILTER_CHROMA_PP_8xN_AVX2   8 , 16
    IPFILTER_CHROMA_PP_8xN_AVX2   8 , 32
    IPFILTER_CHROMA_PP_8xN_AVX2   8 , 4
    IPFILTER_CHROMA_PP_8xN_AVX2   8 , 64
    IPFILTER_CHROMA_PP_8xN_AVX2   8 , 12
    
;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp_4xN(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PP_4xN_AVX2 2
INIT_YMM avx2
cglobal interp_4tap_horiz_pp_%1x%2, 4,6,6
    mov             r4d, r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    vpbroadcastd      m2,           [pw_1]
    vbroadcasti128    m1,           [h_tab_Tm]
    mov               r4d,          %2
    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec                r0

.loop:
    sub               r4d,          4
    ; Row 0-1
    movu              xm3,          [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    vinserti128       m3,           m3,      [r0 + r1],     1
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2

    ; Row 2-3
    lea               r0,           [r0 + r1 * 2]
    movu              xm4,          [r0]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    vinserti128       m4,           m4,      [r0 + r1],     1
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    pmulhrsw          m3,           [pw_512]
    vextracti128      xm4,          m3,                     1
    packuswb          xm3,          xm4

    movd              [r2],         xm3
    pextrd            [r2+r3],      xm3,                    2
    lea               r2,           [r2 + r3 * 2]
    pextrd            [r2],         xm3,                    1
    pextrd            [r2+r3],      xm3,                    3

    lea               r0,           [r0 + r1 * 2]
    lea               r2,           [r2 + r3 * 2]
    test              r4d,          r4d
    jnz               .loop
    RET
%endmacro

    IPFILTER_CHROMA_PP_4xN_AVX2  4 , 8
    IPFILTER_CHROMA_PP_4xN_AVX2  4 , 16

%macro IPFILTER_LUMA_PS_32xN_AVX2 2
INIT_YMM avx2
cglobal interp_8tap_horiz_ps_%1x%2, 4, 7, 8
    mov                         r5d,               r5m
    mov                         r4d,               r4m
%ifdef PIC
    lea                         r6,                [h_tab_LumaCoeff]
    vpbroadcastq                m0,                [r6 + r4 * 8]
%else
    vpbroadcastq                m0,                [h_tab_LumaCoeff + r4 * 8]
%endif
    mova                        m6,                [h_tab_Lm + 32]
    mova                        m1,                [h_tab_Lm]
    mov                         r4d,                %2                           ;height
    add                         r3d,               r3d
    vbroadcasti128              m2,                [pw_1]
    mova                        m7,                [h_interp8_hps_shuf]

    ; register map
    ; m0      - interpolate coeff
    ; m1 , m6 - shuffle order table
    ; m2      - pw_1


    sub                         r0,                3
    test                        r5d,               r5d
    jz                          .label
    lea                         r6,                [r1 * 3]                     ; r8 = (N / 2 - 1) * srcStride
    sub                         r0,                r6
    add                         r4d,                7

.label:
    lea                         r6,                 [pw_2000]
.loop:
    ; Row 0
    vbroadcasti128              m3,                [r0]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m4,                m3,             m6           ; row 0 (col 4 to 7)
    pshufb                      m3,                m1                           ; shuffled based on the col order tab_Lm row 0 (col 0 to 3)
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4


    vbroadcasti128              m4,                [r0 + 8]
    pshufb                      m5,                m4,            m6            ;row 0 (col 12 to 15)
    pshufb                      m4,                m1                           ;row 0 (col 8 to 11)
    pmaddubsw                   m4,                m0
    pmaddubsw                   m5,                m0
    pmaddwd                     m4,                m2
    pmaddwd                     m5,                m2
    packssdw                    m4,                m5

    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4
    vpermd                      m3,                m7,               m3
    psubw                       m3,                [r6]

    movu                        [r2],              m3                          ;row 0

    vbroadcasti128              m3,                [r0 + 16]
    pshufb                      m4,                m3,             m6           ; row 0 (col 20 to 23)
    pshufb                      m3,                m1                           ; row 0 (col 16 to 19)
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4

    vbroadcasti128              m4,                [r0 + 24]
    pshufb                      m5,                m4,            m6            ;row 0 (col 28 to 31)
    pshufb                      m4,                m1                           ;row 0 (col 24 to 27)
    pmaddubsw                   m4,                m0
    pmaddubsw                   m5,                m0
    pmaddwd                     m4,                m2
    pmaddwd                     m5,                m2
    packssdw                    m4,                m5

    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4
    vpermd                      m3,                m7,               m3
    psubw                       m3,                [r6]

    movu                        [r2 + 32],         m3                          ;row 0

    add                         r0,                r1
    add                         r2,                r3
    dec                         r4d
    jnz                         .loop
    RET
%endmacro

    IPFILTER_LUMA_PS_32xN_AVX2 32 , 32
    IPFILTER_LUMA_PS_32xN_AVX2 32 , 16
    IPFILTER_LUMA_PS_32xN_AVX2 32 , 24
    IPFILTER_LUMA_PS_32xN_AVX2 32 , 8
    IPFILTER_LUMA_PS_32xN_AVX2 32 , 64

INIT_YMM avx2
cglobal interp_8tap_horiz_ps_48x64, 4, 7, 8
    mov                         r5d,               r5m
    mov                         r4d,               r4m
%ifdef PIC
    lea                         r6,                [h_tab_LumaCoeff]
    vpbroadcastq                m0,                [r6 + r4 * 8]
%else
    vpbroadcastq                m0,                [h_tab_LumaCoeff + r4 * 8]
%endif
    mova                        m6,                [h_tab_Lm + 32]
    mova                        m1,                [h_tab_Lm]
    mov                         r4d,               64                           ;height
    add                         r3d,               r3d
    vbroadcasti128              m2,                [pw_2000]
    mova                        m7,                [pw_1]

    ; register map
    ; m0      - interpolate coeff
    ; m1 , m6 - shuffle order table
    ; m2      - pw_2000

    sub                         r0,                3
    test                        r5d,               r5d
    jz                          .label
    lea                         r6,                [r1 * 3]                     ; r6 = (N / 2 - 1) * srcStride
    sub                         r0,                r6                           ; r0(src)-r6
    add                         r4d,                7                            ; blkheight += N - 1  (7 - 1 = 6 ; since the last one row not in loop)

.label:
    lea                         r6,                [h_interp8_hps_shuf]
.loop:
    ; Row 0
    vbroadcasti128              m3,                [r0]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m4,                m3,             m6           ; row 0 (col 4 to 7)
    pshufb                      m3,                m1                           ; shuffled based on the col order tab_Lm row 0 (col 0 to 3)
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m7
    pmaddwd                     m4,                m7
    packssdw                    m3,                m4

    vbroadcasti128              m4,                [r0 + 8]
    pshufb                      m5,                m4,             m6            ;row 0 (col 12 to 15)
    pshufb                      m4,                m1                           ;row 0 (col 8 to 11)
    pmaddubsw                   m4,                m0
    pmaddubsw                   m5,                m0
    pmaddwd                     m4,                m7
    pmaddwd                     m5,                m7
    packssdw                    m4,                m5
    pmaddwd                     m3,                m7
    pmaddwd                     m4,                m7
    packssdw                    m3,                m4
    mova                        m5,                [r6]
    vpermd                      m3,                m5,             m3
    psubw                       m3,                m2
    movu                        [r2],              m3                          ;row 0

    vbroadcasti128              m3,                [r0 + 16]
    pshufb                      m4,                m3,             m6           ; row 0 (col 20 to 23)
    pshufb                      m3,                m1                           ; row 0 (col 16 to 19)
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m7
    pmaddwd                     m4,                m7
    packssdw                    m3,                m4

    vbroadcasti128              m4,                [r0 + 24]
    pshufb                      m5,                m4,             m6            ;row 0 (col 28 to 31)
    pshufb                      m4,                m1                           ;row 0 (col 24 to 27)
    pmaddubsw                   m4,                m0
    pmaddubsw                   m5,                m0
    pmaddwd                     m4,                m7
    pmaddwd                     m5,                m7
    packssdw                    m4,                m5
    pmaddwd                     m3,                m7
    pmaddwd                     m4,                m7
    packssdw                    m3,                m4
    mova                        m5,                [r6]
    vpermd                      m3,                m5,               m3
    psubw                       m3,                m2
    movu                        [r2 + 32],         m3                          ;row 0

    vbroadcasti128              m3,                [r0 + 32]
    pshufb                      m4,                m3,             m6           ; row 0 (col 36 to 39)
    pshufb                      m3,                m1                           ; row 0 (col 32 to 35)
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m7
    pmaddwd                     m4,                m7
    packssdw                    m3,                m4

    vbroadcasti128              m4,                [r0 + 40]
    pshufb                      m5,                m4,            m6            ;row 0 (col 44 to 47)
    pshufb                      m4,                m1                           ;row 0 (col 40 to 43)
    pmaddubsw                   m4,                m0
    pmaddubsw                   m5,                m0
    pmaddwd                     m4,                m7
    pmaddwd                     m5,                m7
    packssdw                    m4,                m5
    pmaddwd                     m3,                m7
    pmaddwd                     m4,                m7
    packssdw                    m3,                m4
    mova                        m5,                [r6]
    vpermd                      m3,                m5,               m3
    psubw                       m3,                m2
    movu                        [r2 + 64],         m3                          ;row 0

    add                         r0,                r1
    add                         r2,                r3
    dec                         r4d
    jnz                         .loop
    RET

INIT_YMM avx2
cglobal interp_8tap_horiz_pp_24x32, 4,6,8
    sub               r0,         3
    mov               r4d,        r4m
%ifdef PIC
    lea               r5,         [h_tab_LumaCoeff]
    vpbroadcastd      m0,         [r5 + r4 * 8]
    vpbroadcastd      m1,         [r5 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,         [h_tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,         [h_tab_LumaCoeff + r4 * 8 + 4]
%endif
    movu              m3,         [h_tab_Tm + 16]
    vpbroadcastd      m7,         [pw_1]
    lea               r5,         [h_tab_Tm]

    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m2  shuffle order table
    ; m7 - pw_1

    mov               r4d,        32
.loop:
    ; Row 0
    vbroadcasti128    m4,         [r0]                        ; [x E D C B A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,         m4,     m3
    pshufb            m4,         [r5]
    pmaddubsw         m4,         m0
    pmaddubsw         m5,         m1
    paddw             m4,         m5
    pmaddwd           m4,         m7

    vbroadcasti128    m5,         [r0 + 8]
    pshufb            m6,         m5,     m3
    pshufb            m5,         [r5]
    pmaddubsw         m5,         m0
    pmaddubsw         m6,         m1
    paddw             m5,         m6
    pmaddwd           m5,         m7
    packssdw          m4,         m5                          ; [17 16 15 14 07 06 05 04 13 12 11 10 03 02 01 00]
    pmulhrsw          m4,         [pw_512]

    vbroadcasti128    m2,         [r0 + 16]
    pshufb            m5,         m2,     m3
    pshufb            m2,         [r5]
    pmaddubsw         m2,         m0
    pmaddubsw         m5,         m1
    paddw             m2,         m5
    pmaddwd           m2,         m7

    packssdw          m2,         m2
    pmulhrsw          m2,         [pw_512]
    packuswb          m4,         m2
    vpermq            m4,         m4,     11011000b
    vextracti128      xm5,        m4,     1
    pshufd            xm4,        xm4,    11011000b
    pshufd            xm5,        xm5,    11011000b

    movu              [r2],       xm4
    movq              [r2 + 16],  xm5
    add               r0,         r1
    add               r2,         r3
    dec               r4d
    jnz               .loop
    RET

INIT_YMM avx2
cglobal interp_8tap_horiz_pp_12x16, 4,6,8
    sub               r0,        3
    mov               r4d,       r4m
%ifdef PIC
    lea               r5,        [h_tab_LumaCoeff]
    vpbroadcastd      m0,        [r5 + r4 * 8]
    vpbroadcastd      m1,        [r5 + r4 * 8 + 4]
%else
    vpbroadcastd      m0,         [h_tab_LumaCoeff + r4 * 8]
    vpbroadcastd      m1,         [h_tab_LumaCoeff + r4 * 8 + 4]
%endif
    movu              m3,         [h_tab_Tm + 16]
    vpbroadcastd      m7,         [pw_1]
    lea               r5,         [h_tab_Tm]

    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m2  shuffle order table
    ; m7 - pw_1

    mov               r4d,        8
.loop:
    ; Row 0
    vbroadcasti128    m4,         [r0]                        ;first 8 element
    pshufb            m5,         m4,     m3
    pshufb            m4,         [r5]
    pmaddubsw         m4,         m0
    pmaddubsw         m5,         m1
    paddw             m4,         m5
    pmaddwd           m4,         m7

    vbroadcasti128    m5,         [r0 + 8]                    ; element 8 to 11
    pshufb            m6,         m5,     m3
    pshufb            m5,         [r5]
    pmaddubsw         m5,         m0
    pmaddubsw         m6,         m1
    paddw             m5,         m6
    pmaddwd           m5,         m7

    packssdw          m4,         m5                          ; [17 16 15 14 07 06 05 04 13 12 11 10 03 02 01 00]
    pmulhrsw          m4,         [pw_512]

    ;Row 1
    vbroadcasti128    m2,         [r0 + r1]
    pshufb            m5,         m2,     m3
    pshufb            m2,         [r5]
    pmaddubsw         m2,         m0
    pmaddubsw         m5,         m1
    paddw             m2,         m5
    pmaddwd           m2,         m7

    vbroadcasti128    m5,         [r0 + r1 + 8]
    pshufb            m6,         m5,     m3
    pshufb            m5,         [r5]
    pmaddubsw         m5,         m0
    pmaddubsw         m6,         m1
    paddw             m5,         m6
    pmaddwd           m5,         m7

    packssdw          m2,         m5
    pmulhrsw          m2,         [pw_512]
    packuswb          m4,         m2
    vpermq            m4,         m4,     11011000b
    vextracti128      xm5,        m4,     1
    pshufd            xm4,        xm4,    11011000b
    pshufd            xm5,        xm5,    11011000b

    movq              [r2],       xm4
    pextrd            [r2+8],     xm4,    2
    movq              [r2 + r3],  xm5
    pextrd            [r2+r3+8],  xm5,    2
    lea               r0,         [r0 + r1 * 2]
    lea               r2,         [r2 + r3 * 2]
    dec               r4d
    jnz              .loop
    RET

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp_16xN(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PP_16xN_AVX2 2
INIT_YMM avx2
cglobal interp_4tap_horiz_pp_%1x%2, 4, 6, 7
    mov               r4d,          r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m6,           [pw_512]
    mova              m1,           [interp4_horiz_shuf1]
    vpbroadcastd      m2,           [pw_1]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
    mov               r4d,          %2/2

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 4]                    ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           m6

    ; Row 1
    vbroadcasti128    m4,           [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    vbroadcasti128    m5,           [r0 + r1 + 4]               ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,           m1
    pmaddubsw         m5,           m0
    pmaddwd           m5,           m2
    packssdw          m4,           m5
    pmulhrsw          m4,           m6

    packuswb          m3,           m4
    vpermq            m3,           m3,      11011000b

    vextracti128      xm4,          m3,       1
    movu              [r2],         xm3
    movu              [r2 + r3],    xm4
    lea               r2,           [r2 + r3 * 2]
    lea               r0,           [r0 + r1 * 2]
    dec               r4d
    jnz               .loop
    RET
%endmacro

    IPFILTER_CHROMA_PP_16xN_AVX2 16 , 8
    IPFILTER_CHROMA_PP_16xN_AVX2 16 , 32
    IPFILTER_CHROMA_PP_16xN_AVX2 16 , 12
    IPFILTER_CHROMA_PP_16xN_AVX2 16 , 4
    IPFILTER_CHROMA_PP_16xN_AVX2 16 , 64
    IPFILTER_CHROMA_PP_16xN_AVX2 16 , 24

%macro IPFILTER_LUMA_PS_64xN_AVX2 1
INIT_YMM avx2
cglobal interp_8tap_horiz_ps_64x%1, 4, 7, 8
    mov                         r5d,               r5m
    mov                         r4d,               r4m
%ifdef PIC
    lea                         r6,                [h_tab_LumaCoeff]
    vpbroadcastq                m0,                [r6 + r4 * 8]
%else
    vpbroadcastq                m0,                [h_tab_LumaCoeff + r4 * 8]
%endif
    mova                        m6,                [h_tab_Lm + 32]
    mova                        m1,                [h_tab_Lm]
    mov                         r4d,               %1                           ;height
    add                         r3d,               r3d
    vbroadcasti128              m2,                [pw_1]
    mova                        m7,                [h_interp8_hps_shuf]

    ; register map
    ; m0      - interpolate coeff
    ; m1 , m6 - shuffle order table
    ; m2      - pw_2000

    sub                         r0,                3
    test                        r5d,               r5d
    jz                          .label
    lea                         r6,                [r1 * 3]
    sub                         r0,                r6                           ; r0(src)-r6
    add                         r4d,               7                            ; blkheight += N - 1

.label:
    lea                         r6,                [pw_2000]
.loop:
    ; Row 0
    vbroadcasti128              m3,                [r0]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m4,                m3,             m6           ; row 0 (col 4 to 7)
    pshufb                      m3,                m1                           ; shuffled based on the col order tab_Lm row 0 (col 0 to 3)
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4

    vbroadcasti128              m4,                [r0 + 8]
    pshufb                      m5,                m4,            m6            ;row 0 (col 12 to 15)
    pshufb                      m4,                m1                           ;row 0 (col 8 to 11)
    pmaddubsw                   m4,                m0
    pmaddubsw                   m5,                m0
    pmaddwd                     m4,                m2
    pmaddwd                     m5,                m2
    packssdw                    m4,                m5
    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4
    vpermd                      m3,                m7,               m3
    psubw                       m3,                [r6]
    movu                        [r2],              m3                          ;row 0

    vbroadcasti128              m3,                [r0 + 16]
    pshufb                      m4,                m3,             m6           ; row 0 (col 20 to 23)
    pshufb                      m3,                m1                           ; row 0 (col 16 to 19)
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4

    vbroadcasti128              m4,                [r0 + 24]
    pshufb                      m5,                m4,            m6            ;row 0 (col 28 to 31)
    pshufb                      m4,                m1                           ;row 0 (col 24 to 27)
    pmaddubsw                   m4,                m0
    pmaddubsw                   m5,                m0
    pmaddwd                     m4,                m2
    pmaddwd                     m5,                m2
    packssdw                    m4,                m5
    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4
    vpermd                      m3,                m7,               m3
    psubw                       m3,                [r6]
    movu                        [r2 + 32],         m3                          ;row 0

    vbroadcasti128              m3,                [r0 + 32]
    pshufb                      m4,                m3,             m6           ; row 0 (col 36 to 39)
    pshufb                      m3,                m1                           ; row 0 (col 32 to 35)
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4

    vbroadcasti128              m4,                [r0 + 40]
    pshufb                      m5,                m4,            m6            ;row 0 (col 44 to 47)
    pshufb                      m4,                m1                           ;row 0 (col 40 to 43)
    pmaddubsw                   m4,                m0
    pmaddubsw                   m5,                m0
    pmaddwd                     m4,                m2
    pmaddwd                     m5,                m2
    packssdw                    m4,                m5
    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4
    vpermd                      m3,                m7,               m3
    psubw                       m3,                [r6]
    movu                        [r2 + 64],         m3                          ;row 0
    vbroadcasti128              m3,                [r0 + 48]
    pshufb                      m4,                m3,             m6           ; row 0 (col 52 to 55)
    pshufb                      m3,                m1                           ; row 0 (col 48 to 51)
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4

    vbroadcasti128              m4,                [r0 + 56]
    pshufb                      m5,                m4,            m6            ;row 0 (col 60 to 63)
    pshufb                      m4,                m1                           ;row 0 (col 56 to 59)
    pmaddubsw                   m4,                m0
    pmaddubsw                   m5,                m0
    pmaddwd                     m4,                m2
    pmaddwd                     m5,                m2
    packssdw                    m4,                m5
    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4
    vpermd                      m3,                m7,               m3
    psubw                       m3,                [r6]
    movu                        [r2 + 96],         m3                          ;row 0

    add                          r0,                r1
    add                          r2,                r3
    dec                          r4d
    jnz                         .loop
    RET
%endmacro

    IPFILTER_LUMA_PS_64xN_AVX2 64
    IPFILTER_LUMA_PS_64xN_AVX2 48
    IPFILTER_LUMA_PS_64xN_AVX2 32
    IPFILTER_LUMA_PS_64xN_AVX2 16

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_8xN(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PS_8xN_AVX2 1
INIT_YMM avx2
cglobal interp_4tap_horiz_ps_8x%1, 4,7,6
    mov                r4d,             r4m
    mov                r5d,             r5m
    add                r3d,             r3d

%ifdef PIC
    lea                r6,              [h_tab_ChromaCoeff]
    vpbroadcastd       m0,              [r6 + r4 * 4]
%else
    vpbroadcastd       m0,              [h_tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti128     m2,              [pw_1]
    vbroadcasti128     m5,              [pw_2000]
    mova               m1,              [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    mov                r6d,             %1/2
    dec                r0
    test               r5d,             r5d
    jz                 .loop
    sub                r0 ,             r1
    inc                r6d

.loop:
    ; Row 0
    vbroadcasti128     m3,              [r0]
    pshufb             m3,              m1
    pmaddubsw          m3,              m0
    pmaddwd            m3,              m2

    ; Row 1
    vbroadcasti128     m4,              [r0 + r1]
    pshufb             m4,              m1
    pmaddubsw          m4,              m0
    pmaddwd            m4,              m2
    packssdw           m3,              m4
    psubw              m3,              m5
    vpermq             m3,              m3,          11011000b
    vextracti128       xm4,             m3,          1
    movu               [r2],            xm3
    movu               [r2 + r3],       xm4

    lea                r2,              [r2 + r3 * 2]
    lea                r0,              [r0 + r1 * 2]
    dec                r6d
    jnz                .loop
    test               r5d,             r5d
    jz                 .end

    ;Row 11
    vbroadcasti128     m3,              [r0]
    pshufb             m3,              m1
    pmaddubsw          m3,              m0
    pmaddwd            m3,              m2
    packssdw           m3,              m3
    psubw              m3,              m5
    vpermq             m3,              m3,          11011000b
    movu               [r2],            xm3
.end:
    RET
%endmacro

    IPFILTER_CHROMA_PS_8xN_AVX2  2
    IPFILTER_CHROMA_PS_8xN_AVX2  32
    IPFILTER_CHROMA_PS_8xN_AVX2  16
    IPFILTER_CHROMA_PS_8xN_AVX2  6
    IPFILTER_CHROMA_PS_8xN_AVX2  4
    IPFILTER_CHROMA_PS_8xN_AVX2  12
    IPFILTER_CHROMA_PS_8xN_AVX2  64

INIT_YMM avx2
cglobal interp_4tap_horiz_ps_2x4, 4, 7, 3
    mov                r4d,            r4m
    mov                r5d,            r5m
    add                r3d,            r3d
%ifdef PIC
    lea                r6,             [h_tab_ChromaCoeff]
    vpbroadcastd       m0,             [r6 + r4 * 4]
%else
    vpbroadcastd       m0,             [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova               xm3,            [pw_2000]
    dec                r0
    test               r5d,            r5d
    jz                 .label
    sub                r0,             r1

.label:
    lea                r6,             [r1 * 3]
    movq               xm1,            [r0]
    movhps             xm1,            [r0 + r1]
    movq               xm2,            [r0 + r1 * 2]
    movhps             xm2,            [r0 + r6]

    vinserti128        m1,             m1,          xm2,          1
    pshufb             m1,             [interp4_hpp_shuf]
    pmaddubsw          m1,             m0
    pmaddwd            m1,             [pw_1]
    vextracti128       xm2,            m1,          1
    packssdw           xm1,            xm2
    psubw              xm1,            xm3

    lea                r4,             [r3 * 3]
    movd               [r2],           xm1
    pextrd             [r2 + r3],      xm1,         1
    pextrd             [r2 + r3 * 2],  xm1,         2
    pextrd             [r2 + r4],      xm1,         3

    test               r5d,            r5d
    jz                .end
    lea                r2,             [r2 + r3 * 4]
    lea                r0,             [r0 + r1 * 4]

    movq               xm1,            [r0]
    movhps             xm1,            [r0 + r1]
    movq               xm2,            [r0 + r1 * 2]
    vinserti128        m1,             m1,          xm2,          1
    pshufb             m1,             [interp4_hpp_shuf]
    pmaddubsw          m1,             m0
    pmaddwd            m1,             [pw_1]
    vextracti128       xm2,            m1,          1
    packssdw           xm1,            xm2
    psubw              xm1,            xm3

    movd               [r2],           xm1
    pextrd             [r2 + r3],      xm1,         1
    pextrd             [r2 + r3 * 2],  xm1,         2
.end:
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_ps_2x8, 4, 7, 7
    mov               r4d,           r4m
    mov               r5d,           r5m
    add               r3d,           r3d

%ifdef PIC
    lea               r6,            [h_tab_ChromaCoeff]
    vpbroadcastd      m0,            [r6 + r4 * 4]
%else
    vpbroadcastd      m0,            [h_tab_ChromaCoeff + r4 * 4]
%endif
    vbroadcasti128    m6,            [pw_2000]
    test              r5d,            r5d
    jz                .label
    sub               r0,             r1

.label:
    mova              m4,            [interp4_hpp_shuf]
    mova              m5,            [pw_1]
    dec               r0
    lea               r4,            [r1 * 3]
    movq              xm1,           [r0]                                   ;row 0
    movhps            xm1,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m1,            m1,          xm2,          1
    lea               r0,            [r0 + r1 * 4]
    movq              xm3,           [r0]
    movhps            xm3,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m3,            m3,          xm2,          1

    pshufb            m1,            m4
    pshufb            m3,            m4
    pmaddubsw         m1,            m0
    pmaddubsw         m3,            m0
    pmaddwd           m1,            m5
    pmaddwd           m3,            m5
    packssdw          m1,            m3
    psubw             m1,            m6

    lea               r4,            [r3 * 3]
    vextracti128      xm2,           m1,          1

    movd              [r2],          xm1
    pextrd            [r2 + r3],     xm1,         1
    movd              [r2 + r3 * 2], xm2
    pextrd            [r2 + r4],     xm2,         1
    lea               r2,            [r2 + r3 * 4]
    pextrd            [r2],          xm1,         2
    pextrd            [r2 + r3],     xm1,         3
    pextrd            [r2 + r3 * 2], xm2,         2
    pextrd            [r2 + r4],     xm2,         3
    test              r5d,            r5d
    jz                .end

    lea               r0,            [r0 + r1 * 4]
    lea               r2,            [r2 + r3 * 4]
    movq              xm1,           [r0]                                   ;row 0
    movhps            xm1,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    vinserti128       m1,            m1,          xm2,          1
    pshufb            m1,            m4
    pmaddubsw         m1,            m0
    pmaddwd           m1,            m5
    packssdw          m1,            m1
    psubw             m1,            m6
    vextracti128      xm2,           m1,          1

    movd              [r2],          xm1
    pextrd            [r2 + r3],     xm1,         1
    movd              [r2 + r3 * 2], xm2
.end:
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_12x16, 4, 6, 7
    mov               r4d,          r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m6,           [pw_512]
    mova              m1,           [interp4_horiz_shuf1]
    vpbroadcastd      m2,           [pw_1]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
    mov               r4d,          8

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 4]                    ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           m6

    ; Row 1
    vbroadcasti128    m4,           [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    vbroadcasti128    m5,           [r0 + r1 + 4]               ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,           m1
    pmaddubsw         m5,           m0
    pmaddwd           m5,           m2
    packssdw          m4,           m5
    pmulhrsw          m4,           m6

    packuswb          m3,           m4
    vpermq            m3,           m3,      11011000b

    vextracti128      xm4,          m3,       1
    movq              [r2],         xm3
    pextrd            [r2+8],       xm3,      2
    movq              [r2 + r3],    xm4
    pextrd            [r2 + r3 + 8],xm4,      2
    lea               r2,           [r2 + r3 * 2]
    lea               r0,           [r0 + r1 * 2]
    dec               r4d
    jnz               .loop
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_24x32, 4,6,7
    mov              r4d,           r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m1,           [interp4_horiz_shuf1]
    vpbroadcastd      m2,           [pw_1]
    mova              m6,           [pw_512]
    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
    mov               r4d,          32

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 4]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           m6

    vbroadcasti128    m4,           [r0 + 16]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    vbroadcasti128    m5,           [r0 + 20]
    pshufb            m5,           m1
    pmaddubsw         m5,           m0
    pmaddwd           m5,           m2
    packssdw          m4,           m5
    pmulhrsw          m4,           m6

    packuswb          m3,           m4
    vpermq            m3,           m3,      11011000b

    vextracti128      xm4,          m3,       1
    movu              [r2],         xm3
    movq              [r2 + 16],    xm4
    add               r2,           r3
    add               r0,           r1
    dec               r4d
    jnz               .loop
    RET

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_6x8(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------;
INIT_YMM avx2
cglobal interp_4tap_horiz_ps_6x8, 4,7,6
    mov                r4d,            r4m
    mov                r5d,            r5m
    add                r3d,            r3d

%ifdef PIC
    lea                r6,             [h_tab_ChromaCoeff]
    vpbroadcastd       m0,             [r6 + r4 * 4]
%else
    vpbroadcastd       m0,             [h_tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti128     m2,             [pw_1]
    vbroadcasti128     m5,             [pw_2000]
    mova               m1,             [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    mov               r6d,             8/2
    dec               r0
    test              r5d,             r5d
    jz                .loop
    sub               r0 ,             r1
    inc               r6d

.loop:
    ; Row 0
    vbroadcasti128    m3,              [r0]
    pshufb            m3,              m1
    pmaddubsw         m3,              m0
    pmaddwd           m3,              m2

    ; Row 1
    vbroadcasti128    m4,              [r0 + r1]
    pshufb            m4,              m1
    pmaddubsw         m4,              m0
    pmaddwd           m4,              m2
    packssdw          m3,              m4
    psubw             m3,              m5
    vpermq            m3,              m3,          11011000b
    vextracti128      xm4,             m3,          1
    movq              [r2],            xm3
    pextrd            [r2 + 8],        xm3,         2
    movq              [r2 + r3],       xm4
    pextrd            [r2 + r3 + 8],   xm4,         2
    lea               r2,              [r2 + r3 * 2]
    lea               r0,              [r0 + r1 * 2]
    dec               r6d
    jnz              .loop
    test              r5d,             r5d
    jz               .end

    ;Row 11
    vbroadcasti128    m3,              [r0]
    pshufb            m3,              m1
    pmaddubsw         m3,              m0
    pmaddwd           m3,              m2
    packssdw          m3,              m3
    psubw             m3,              m5
    vextracti128      xm4,             m3,          1
    movq              [r2],            xm3
    movd              [r2+8],          xm4
.end:
    RET

INIT_YMM avx2
cglobal interp_8tap_horiz_ps_12x16, 6, 7, 8
    mov                         r5d,               r5m
    mov                         r4d,               r4m
%ifdef PIC
    lea                         r6,                [h_tab_LumaCoeff]
    vpbroadcastq                m0,                [r6 + r4 * 8]
%else
    vpbroadcastq                m0,                [h_tab_LumaCoeff + r4 * 8]
%endif
    mova                        m6,                [h_tab_Lm + 32]
    mova                        m1,                [h_tab_Lm]
    add                         r3d,               r3d
    vbroadcasti128              m2,                [pw_2000]
    mov                         r4d,                16
    vbroadcasti128              m7,                [pw_1]
    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - pw_2000

    mova                        m5,                [h_interp8_hps_shuf]
    sub                         r0,                3
    test                        r5d,               r5d
    jz                          .loop
    lea                         r6,                [r1 * 3]                     ; r6 = (N / 2 - 1) * srcStride
    sub                         r0,                r6                           ; r0(src)-r6
    add                         r4d,                7
.loop:

    ; Row 0

    vbroadcasti128              m3,                [r0]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m4,                m3,        m6
    pshufb                      m3,                m1                           ; shuffled based on the col order tab_Lm
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m7
    pmaddwd                     m4,                m7
    packssdw                    m3,                m4

    vbroadcasti128              m4,                [r0 + 8]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m4,                m1
    pmaddubsw                   m4,                m0
    pmaddwd                     m4,                m7
    packssdw                    m4,                m4

    pmaddwd                     m3,                m7
    pmaddwd                     m4,                m7
    packssdw                    m3,                m4

    vpermd                      m3,                m5,               m3
    psubw                       m3,                m2

    vextracti128                xm4,               m3,               1
    movu                        [r2],              xm3                          ;row 0
    movq                        [r2 + 16],         xm4                          ;row 1

    add                         r0,                r1
    add                         r2,                r3
    dec                         r4d
    jnz                         .loop
    RET

INIT_YMM avx2
cglobal interp_8tap_horiz_ps_24x32, 4, 7, 8
    mov                         r5d,               r5m
    mov                         r4d,               r4m
%ifdef PIC
    lea                         r6,                [h_tab_LumaCoeff]
    vpbroadcastq                m0,                [r6 + r4 * 8]
%else
    vpbroadcastq                m0,                [h_tab_LumaCoeff + r4 * 8]
%endif
    mova                        m6,                [h_tab_Lm + 32]
    mova                        m1,                [h_tab_Lm]
    mov                         r4d,               32                           ;height
    add                         r3d,               r3d
    vbroadcasti128              m2,                [pw_2000]
    vbroadcasti128              m7,                [pw_1]

    ; register map
    ; m0      - interpolate coeff
    ; m1 , m6 - shuffle order table
    ; m2      - pw_2000

    sub                         r0,                3
    test                        r5d,               r5d
    jz                          .label
    lea                         r6,                [r1 * 3]                     ; r6 = (N / 2 - 1) * srcStride
    sub                         r0,                r6                           ; r0(src)-r6
    add                         r4d,               7                            ; blkheight += N - 1  (7 - 1 = 6 ; since the last one row not in loop)

.label:
    lea                         r6,                [h_interp8_hps_shuf]
.loop:
    ; Row 0
    vbroadcasti128              m3,                [r0]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m4,                m3,             m6           ; row 0 (col 4 to 7)
    pshufb                      m3,                m1                           ; shuffled based on the col order tab_Lm row 0 (col 0 to 3)
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m7
    pmaddwd                     m4,                m7
    packssdw                    m3,                m4

    vbroadcasti128              m4,                [r0 + 8]                     ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m5,                m4,            m6            ;row 1 (col 4 to 7)
    pshufb                      m4,                m1                           ;row 1 (col 0 to 3)
    pmaddubsw                   m4,                m0
    pmaddubsw                   m5,                m0
    pmaddwd                     m4,                m7
    pmaddwd                     m5,                m7
    packssdw                    m4,                m5
    pmaddwd                     m3,                m7
    pmaddwd                     m4,                m7
    packssdw                    m3,                m4
    mova                        m5,                [r6]
    vpermd                      m3,                m5,               m3
    psubw                       m3,                m2
    movu                        [r2],              m3                          ;row 0

    vbroadcasti128              m3,                [r0 + 16]
    pshufb                      m4,                m3,          m6
    pshufb                      m3,                m1
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m7
    pmaddwd                     m4,                m7
    packssdw                    m3,                m4
    pmaddwd                     m3,                m7
    pmaddwd                     m4,                m7
    packssdw                    m3,                m4
    mova                        m4,                [r6]
    vpermd                      m3,                m4,            m3
    psubw                       m3,                m2
    movu                        [r2 + 32],         xm3                          ;row 0

    add                         r0,                r1
    add                         r2,                r3
    dec                         r4d
    jnz                         .loop
    RET

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_24x32(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
cglobal interp_4tap_horiz_ps_24x32, 4,7,6
    mov                r4d,            r4m
    mov                r5d,            r5m
    add                r3d,            r3d
%ifdef PIC
    lea                r6,             [h_tab_ChromaCoeff]
    vpbroadcastd       m0,             [r6 + r4 * 4]
%else
    vpbroadcastd       m0,             [h_tab_ChromaCoeff + r4 * 4]
%endif
    vbroadcasti128     m2,             [pw_1]
    vbroadcasti128     m5,             [pw_2000]
    mova               m1,             [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1
    mov                r6d,            32
    dec                r0
    test               r5d,            r5d
    je                 .loop
    sub                r0 ,            r1
    add                r6d ,           3

.loop:
    ; Row 0
    vbroadcasti128     m3,             [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb             m3,             m1
    pmaddubsw          m3,             m0
    pmaddwd            m3,             m2
    vbroadcasti128     m4,             [r0 + 8]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb             m4,             m1
    pmaddubsw          m4,             m0
    pmaddwd            m4,             m2
    packssdw           m3,             m4
    psubw              m3,             m5
    vpermq             m3,             m3,          11011000b
    movu               [r2],           m3

    vbroadcasti128     m3,             [r0 + 16]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb             m3,             m1
    pmaddubsw          m3,             m0
    pmaddwd            m3,             m2
    packssdw           m3,             m3
    psubw              m3,             m5
    vpermq             m3,             m3,          11011000b
    movu               [r2 + 32],      xm3

    add                r2,             r3
    add                r0,             r1
    dec                r6d
    jnz                .loop
    RET

;-----------------------------------------------------------------------------------------------------------------------
;macro FILTER_H8_W8_16N_AVX2
;-----------------------------------------------------------------------------------------------------------------------
%macro  FILTER_H8_W8_16N_AVX2 0
    vbroadcasti128              m3,                [r0]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m4,                m3,             m6           ; row 0 (col 4 to 7)
    pshufb                      m3,                m1                           ; shuffled based on the col order tab_Lm row 0 (col 0 to 3)
    pmaddubsw                   m3,                m0
    pmaddubsw                   m4,                m0
    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4                         ; DWORD [R1D R1C R0D R0C R1B R1A R0B R0A]

    vbroadcasti128              m4,                [r0 + 8]                         ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb                      m5,                m4,            m6            ;row 1 (col 4 to 7)
    pshufb                      m4,                m1                           ;row 1 (col 0 to 3)
    pmaddubsw                   m4,                m0
    pmaddubsw                   m5,                m0
    pmaddwd                     m4,                m2
    pmaddwd                     m5,                m2
    packssdw                    m4,                m5                         ; DWORD [R3D R3C R2D R2C R3B R3A R2B R2A]

    pmaddwd                     m3,                m2
    pmaddwd                     m4,                m2
    packssdw                    m3,                m4                         ; all rows and col completed.

    mova                        m5,                [h_interp8_hps_shuf]
    vpermd                      m3,                m5,               m3
    psubw                       m3,                m8

    vextracti128                xm4,               m3,               1
    mova                        [r4],              xm3
    mova                        [r4 + 16],         xm4
    %endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp_64xN(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
%macro IPFILTER_CHROMA_PP_64xN_AVX2 1
INIT_YMM avx2
cglobal interp_4tap_horiz_pp_64x%1, 4,6,7
    mov             r4d, r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m1,           [interp4_horiz_shuf1]
    vpbroadcastd      m2,           [pw_1]
    mova              m6,           [pw_512]
    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
    mov               r4d,          %1

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 4]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           m6

    vbroadcasti128    m4,           [r0 + 16]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    vbroadcasti128    m5,           [r0 + 20]
    pshufb            m5,           m1
    pmaddubsw         m5,           m0
    pmaddwd           m5,           m2
    packssdw          m4,           m5
    pmulhrsw          m4,           m6
    packuswb          m3,           m4
    vpermq            m3,           m3,      11011000b
    movu              [r2],         m3

    vbroadcasti128    m3,           [r0 + 32]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 36]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           m6

    vbroadcasti128    m4,           [r0 + 48]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    vbroadcasti128    m5,           [r0 + 52]
    pshufb            m5,           m1
    pmaddubsw         m5,           m0
    pmaddwd           m5,           m2
    packssdw          m4,           m5
    pmulhrsw          m4,           m6
    packuswb          m3,           m4
    vpermq            m3,           m3,      11011000b
    movu              [r2 + 32],         m3

    add               r2,           r3
    add               r0,           r1
    dec               r4d
    jnz               .loop
    RET
%endmacro

    IPFILTER_CHROMA_PP_64xN_AVX2  64
    IPFILTER_CHROMA_PP_64xN_AVX2  32
    IPFILTER_CHROMA_PP_64xN_AVX2  48
    IPFILTER_CHROMA_PP_64xN_AVX2  16

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp_48x64(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
cglobal interp_4tap_horiz_pp_48x64, 4,6,7
    mov             r4d, r4m

%ifdef PIC
    lea               r5,            [h_tab_ChromaCoeff]
    vpbroadcastd      m0,            [r5 + r4 * 4]
%else
    vpbroadcastd      m0,            [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m1,            [interp4_horiz_shuf1]
    vpbroadcastd      m2,            [pw_1]
    mova              m6,            [pw_512]
    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
    mov               r4d,           64

.loop:
    ; Row 0
    vbroadcasti128    m3,            [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,            m1
    pmaddubsw         m3,            m0
    pmaddwd           m3,            m2
    vbroadcasti128    m4,            [r0 + 4]
    pshufb            m4,            m1
    pmaddubsw         m4,            m0
    pmaddwd           m4,            m2
    packssdw          m3,            m4
    pmulhrsw          m3,            m6

    vbroadcasti128    m4,            [r0 + 16]
    pshufb            m4,            m1
    pmaddubsw         m4,            m0
    pmaddwd           m4,            m2
    vbroadcasti128    m5,            [r0 + 20]
    pshufb            m5,            m1
    pmaddubsw         m5,            m0
    pmaddwd           m5,            m2
    packssdw          m4,            m5
    pmulhrsw          m4,            m6

    packuswb          m3,            m4
    vpermq            m3,            m3,      q3120

    movu              [r2],          m3

    vbroadcasti128    m3,            [r0 + mmsize]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,            m1
    pmaddubsw         m3,            m0
    pmaddwd           m3,            m2
    vbroadcasti128    m4,            [r0 + mmsize + 4]
    pshufb            m4,            m1
    pmaddubsw         m4,            m0
    pmaddwd           m4,            m2
    packssdw          m3,            m4
    pmulhrsw          m3,            m6

    vbroadcasti128    m4,            [r0 + mmsize + 16]
    pshufb            m4,            m1
    pmaddubsw         m4,            m0
    pmaddwd           m4,            m2
    vbroadcasti128    m5,            [r0 + mmsize + 20]
    pshufb            m5,            m1
    pmaddubsw         m5,            m0
    pmaddwd           m5,            m2
    packssdw          m4,            m5
    pmulhrsw          m4,            m6

    packuswb          m3,            m4
    vpermq            m3,            m3,      q3120
    movu              [r2 + mmsize], xm3

    add               r2,            r3
    add               r0,            r1
    dec               r4d
    jnz               .loop
    RET

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_48x64(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------;

INIT_YMM avx2
cglobal interp_4tap_horiz_ps_48x64, 4,7,6
    mov             r4d, r4m
    mov             r5d, r5m
    add             r3d, r3d

%ifdef PIC
    lea               r6,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r6 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    vbroadcasti128     m2,          [pw_1]
    vbroadcasti128     m5,          [pw_2000]
    mova               m1,          [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1
    mov               r6d,          64
    dec               r0
    test              r5d,          r5d
    je                .loop
    sub               r0 ,          r1
    add               r6d ,         3

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                           ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 8]                       ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           m5
    vpermq            m3,           m3,          q3120
    movu              [r2],         m3

    vbroadcasti128    m3,           [r0 + 16]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 24]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           m5
    vpermq            m3,           m3,          q3120
    movu              [r2 + 32],    m3

    vbroadcasti128    m3,           [r0 + 32]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 40]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2

    packssdw          m3,           m4
    psubw             m3,           m5
    vpermq            m3,           m3,          q3120
    movu              [r2 + 64],    m3

    add               r2,          r3
    add               r0,          r1
    dec               r6d
    jnz               .loop
    RET

;-----------------------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_ps_24x64(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
cglobal interp_4tap_horiz_ps_24x64, 4,7,6
    mov                r4d,            r4m
    mov                r5d,            r5m
    add                r3d,            r3d
%ifdef PIC
    lea                r6,             [h_tab_ChromaCoeff]
    vpbroadcastd       m0,             [r6 + r4 * 4]
%else
    vpbroadcastd       m0,             [h_tab_ChromaCoeff + r4 * 4]
%endif
    vbroadcasti128     m2,             [pw_1]
    vbroadcasti128     m5,             [pw_2000]
    mova               m1,             [h_tab_Tm]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1
    mov                r6d,            64
    dec                r0
    test               r5d,            r5d
    je                 .loop
    sub                r0 ,            r1
    add                r6d ,           3

.loop:
    ; Row 0
    vbroadcasti128     m3,             [r0]                          ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb             m3,             m1
    pmaddubsw          m3,             m0
    pmaddwd            m3,             m2
    vbroadcasti128     m4,             [r0 + 8]                      ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb             m4,             m1
    pmaddubsw          m4,             m0
    pmaddwd            m4,             m2
    packssdw           m3,             m4
    psubw              m3,             m5
    vpermq             m3,             m3,          q3120
    movu               [r2],           m3

    vbroadcasti128     m3,             [r0 + 16]                     ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb             m3,             m1
    pmaddubsw          m3,             m0
    pmaddwd            m3,             m2
    packssdw           m3,             m3
    psubw              m3,             m5
    vpermq             m3,             m3,          q3120
    movu               [r2 + 32],      xm3

    add                r2,             r3
    add                r0,             r1
    dec                r6d
    jnz                .loop
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_ps_2x16, 4, 7, 7
    mov               r4d,           r4m
    mov               r5d,           r5m
    add               r3d,           r3d

%ifdef PIC
    lea               r6,            [h_tab_ChromaCoeff]
    vpbroadcastd      m0,            [r6 + r4 * 4]
%else
    vpbroadcastd      m0,            [h_tab_ChromaCoeff + r4 * 4]
%endif
    vbroadcasti128    m6,            [pw_2000]
    test              r5d,            r5d
    jz                .label
    sub               r0,             r1

.label:
    mova              m4,            [interp4_hps_shuf]
    mova              m5,            [pw_1]
    dec               r0
    lea               r4,            [r1 * 3]
    movq              xm1,           [r0]                                   ;row 0
    movhps            xm1,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m1,            m1,           xm2,          1
    lea               r0,            [r0 + r1 * 4]
    movq              xm3,           [r0]
    movhps            xm3,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m3,            m3,           xm2,          1

    pshufb            m1,            m4
    pshufb            m3,            m4
    pmaddubsw         m1,            m0
    pmaddubsw         m3,            m0
    pmaddwd           m1,            m5
    pmaddwd           m3,            m5
    packssdw          m1,            m3
    psubw             m1,            m6

    lea               r4,            [r3 * 3]
    vextracti128      xm2,           m1,           1

    movd              [r2],          xm1
    pextrd            [r2 + r3],     xm1,          1
    movd              [r2 + r3 * 2], xm2
    pextrd            [r2 + r4],     xm2,          1
    lea               r2,            [r2 + r3 * 4]
    pextrd            [r2],          xm1,          2
    pextrd            [r2 + r3],     xm1,          3
    pextrd            [r2 + r3 * 2], xm2,          2
    pextrd            [r2 + r4],     xm2,          3

    lea               r0,            [r0 + r1 * 4]
    lea               r2,            [r2 + r3 * 4]
    lea               r4,            [r1 * 3]
    movq              xm1,           [r0]
    movhps            xm1,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m1,            m1,          xm2,           1
    lea               r0,            [r0 + r1 * 4]
    movq              xm3,           [r0]
    movhps            xm3,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m3,            m3,          xm2,           1

    pshufb            m1,            m4
    pshufb            m3,            m4
    pmaddubsw         m1,            m0
    pmaddubsw         m3,            m0
    pmaddwd           m1,            m5
    pmaddwd           m3,            m5
    packssdw          m1,            m3
    psubw             m1,            m6

    lea               r4,            [r3 * 3]
    vextracti128      xm2,           m1,           1

    movd              [r2],          xm1
    pextrd            [r2 + r3],     xm1,          1
    movd              [r2 + r3 * 2], xm2
    pextrd            [r2 + r4],     xm2,          1
    lea               r2,            [r2 + r3 * 4]
    pextrd            [r2],          xm1,          2
    pextrd            [r2 + r3],     xm1,          3
    pextrd            [r2 + r3 * 2], xm2,          2
    pextrd            [r2 + r4],     xm2,          3

    test              r5d,            r5d
    jz                .end

    lea               r0,            [r0 + r1 * 4]
    lea               r2,            [r2 + r3 * 4]
    movq              xm1,           [r0]
    movhps            xm1,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    vinserti128       m1,            m1,          xm2,           1
    pshufb            m1,            m4
    pmaddubsw         m1,            m0
    pmaddwd           m1,            m5
    packssdw          m1,            m1
    psubw             m1,            m6
    vextracti128      xm2,           m1,           1

    movd              [r2],          xm1
    pextrd            [r2 + r3],     xm1,          1
    movd              [r2 + r3 * 2], xm2
.end:
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_6x16, 4, 6, 7
    mov               r4d,               r4m

%ifdef PIC
    lea               r5,                [h_tab_ChromaCoeff]
    vpbroadcastd      m0,                [r5 + r4 * 4]
%else
    vpbroadcastd      m0,                [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m1,                [h_tab_Tm]
    mova              m2,                [pw_1]
    mova              m6,                [pw_512]
    lea               r4,                [r1 * 3]
    lea               r5,                [r3 * 3]
    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
%rep 4
    ; Row 0
    vbroadcasti128    m3,                [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,                m1
    pmaddubsw         m3,                m0
    pmaddwd           m3,                m2

    ; Row 1
    vbroadcasti128    m4,                [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,                m1
    pmaddubsw         m4,                m0
    pmaddwd           m4,                m2
    packssdw          m3,                m4
    pmulhrsw          m3,                m6

    ; Row 2
    vbroadcasti128    m4,                [r0 + r1 * 2]               ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,                m1
    pmaddubsw         m4,                m0
    pmaddwd           m4,                m2

    ; Row 3
    vbroadcasti128    m5,                [r0 + r4]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,                m1
    pmaddubsw         m5,                m0
    pmaddwd           m5,                m2
    packssdw          m4,                m5
    pmulhrsw          m4,                m6

    packuswb          m3,                m4
    vextracti128      xm4,               m3,          1
    movd              [r2],              xm3
    pextrw            [r2 + 4],          xm4,         0
    pextrd            [r2 + r3],         xm3,         1
    pextrw            [r2 + r3 + 4],     xm4,         2
    pextrd            [r2 + r3 * 2],     xm3,         2
    pextrw            [r2 + r3 * 2 + 4], xm4,         4
    pextrd            [r2 + r5],         xm3,         3
    pextrw            [r2 + r5 + 4],     xm4,         6
    lea               r2,                [r2 + r3 * 4]
    lea               r0,                [r0 + r1 * 4]
%endrep
    RET

;-----------------------------------------------------------------------------
; void interp_8tap_hv_pp_16x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int idxX, int idxY)
;-----------------------------------------------------------------------------
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_hv_pp_16x16, 4, 10, 15, 0-31*32
%define stk_buf1    rsp
    mov                         r4d,               r4m
    mov                         r5d,               r5m
%ifdef PIC
    lea                         r6,                [h_tab_LumaCoeff]
    vpbroadcastq                m0,                [r6 + r4 * 8]
%else
    vpbroadcastq                m0,                [h_tab_LumaCoeff + r4 * 8]
%endif

    xor                         r6,                 r6
    mov                         r4,                 rsp
    mova                        m6,                [h_tab_Lm + 32]
    mova                        m1,                [h_tab_Lm]
    mov                         r8,                16                           ;height
    vbroadcasti128              m8,                [pw_2000]
    vbroadcasti128              m2,                [pw_1]
    sub                         r0,                3
    lea                         r7,                [r1 * 3]                     ; r7 = (N / 2 - 1) * srcStride
    sub                         r0,                r7                           ; r0(src)-r7
    add                         r8,                7

.loopH:
    FILTER_H8_W8_16N_AVX2
    add                         r0,                r1
    add                         r4,                32
    inc                         r6
    cmp                         r6,                16+7
    jnz                        .loopH

; vertical phase
    xor                         r6,                r6
    xor                         r1,                r1
.loopV:

;load necessary variables
    mov                         r4d,               r5d          ;coeff here for vertical is r5m
    shl                         r4d,               7
    mov                         r1d,               16
    add                         r1d,               r1d

 ; load intermedia buffer
    mov                         r0,                stk_buf1

    ; register mapping
    ; r0 - src
    ; r5 - coeff
    ; r6 - loop_i

; load coeff table
%ifdef PIC
    lea                          r5,                [h_pw_LumaCoeffVer]
    add                          r5,                r4
%else
    lea                          r5,                [h_pw_LumaCoeffVer + r4]
%endif

    lea                          r4,                [r1*3]
    mova                         m14,               [h_pd_526336]
    lea                          r6,                [r3 * 3]
    mov                          r9d,               16 / 8

.loopW:
    PROCESS_LUMA_AVX2_W8_16R sp
    add                          r2,                 8
    add                          r0,                 16
    dec                          r9d
    jnz                          .loopW
    RET
%endif

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_12x32, 4, 6, 7
    mov               r4d,          r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m6,           [pw_512]
    mova              m1,           [interp4_horiz_shuf1]
    vpbroadcastd      m2,           [pw_1]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
    mov               r4d,          16

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 4]                    ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           m6

    ; Row 1
    vbroadcasti128    m4,           [r0 + r1]                   ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    vbroadcasti128    m5,           [r0 + r1 + 4]               ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m5,           m1
    pmaddubsw         m5,           m0
    pmaddwd           m5,           m2
    packssdw          m4,           m5
    pmulhrsw          m4,           m6

    packuswb          m3,           m4
    vpermq            m3,           m3,      11011000b

    vextracti128      xm4,          m3,       1
    movq              [r2],         xm3
    pextrd            [r2+8],       xm3,      2
    movq              [r2 + r3],    xm4
    pextrd            [r2 + r3 + 8],xm4,      2
    lea               r2,           [r2 + r3 * 2]
    lea               r0,           [r0 + r1 * 2]
    dec               r4d
    jnz               .loop
    RET

INIT_YMM avx2
cglobal interp_4tap_horiz_pp_24x64, 4,6,7
    mov              r4d,           r4m

%ifdef PIC
    lea               r5,           [h_tab_ChromaCoeff]
    vpbroadcastd      m0,           [r5 + r4 * 4]
%else
    vpbroadcastd      m0,           [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m1,           [interp4_horiz_shuf1]
    vpbroadcastd      m2,           [pw_1]
    mova              m6,           [pw_512]
    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - constant word 1

    dec               r0
    mov               r4d,          64

.loop:
    ; Row 0
    vbroadcasti128    m3,           [r0]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    pshufb            m3,           m1
    pmaddubsw         m3,           m0
    pmaddwd           m3,           m2
    vbroadcasti128    m4,           [r0 + 4]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    packssdw          m3,           m4
    pmulhrsw          m3,           m6

    vbroadcasti128    m4,           [r0 + 16]
    pshufb            m4,           m1
    pmaddubsw         m4,           m0
    pmaddwd           m4,           m2
    vbroadcasti128    m5,           [r0 + 20]
    pshufb            m5,           m1
    pmaddubsw         m5,           m0
    pmaddwd           m5,           m2
    packssdw          m4,           m5
    pmulhrsw          m4,           m6

    packuswb          m3,           m4
    vpermq            m3,           m3,      11011000b

    vextracti128      xm4,          m3,       1
    movu              [r2],         xm3
    movq              [r2 + 16],    xm4
    add               r2,           r3
    add               r0,           r1
    dec               r4d
    jnz               .loop
    RET


INIT_YMM avx2
cglobal interp_4tap_horiz_pp_2x16, 4, 6, 6
    mov               r4d,           r4m

%ifdef PIC
    lea               r5,            [h_tab_ChromaCoeff]
    vpbroadcastd      m0,            [r5 + r4 * 4]
%else
    vpbroadcastd      m0,            [h_tab_ChromaCoeff + r4 * 4]
%endif

    mova              m4,            [interp4_hpp_shuf]
    mova              m5,            [pw_1]
    dec               r0
    lea               r4,            [r1 * 3]
    movq              xm1,           [r0]
    movhps            xm1,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m1,            m1,          xm2,          1
    lea               r0,            [r0 + r1 * 4]
    movq              xm3,           [r0]
    movhps            xm3,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m3,            m3,          xm2,          1

    pshufb            m1,            m4
    pshufb            m3,            m4
    pmaddubsw         m1,            m0
    pmaddubsw         m3,            m0
    pmaddwd           m1,            m5
    pmaddwd           m3,            m5
    packssdw          m1,            m3
    pmulhrsw          m1,            [pw_512]
    vextracti128      xm2,           m1,          1
    packuswb          xm1,           xm2

    lea               r4,            [r3 * 3]
    pextrw            [r2],          xm1,         0
    pextrw            [r2 + r3],     xm1,         1
    pextrw            [r2 + r3 * 2], xm1,         4
    pextrw            [r2 + r4],     xm1,         5
    lea               r2,            [r2 + r3 * 4]
    pextrw            [r2],          xm1,         2
    pextrw            [r2 + r3],     xm1,         3
    pextrw            [r2 + r3 * 2], xm1,         6
    pextrw            [r2 + r4],     xm1,         7
    lea               r2,            [r2 + r3 * 4]
    lea               r0,            [r0 + r1 * 4]

    lea               r4,            [r1 * 3]
    movq              xm1,           [r0]
    movhps            xm1,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m1,            m1,          xm2,          1
    lea               r0,            [r0 + r1 * 4]
    movq              xm3,           [r0]
    movhps            xm3,           [r0 + r1]
    movq              xm2,           [r0 + r1 * 2]
    movhps            xm2,           [r0 + r4]
    vinserti128       m3,            m3,          xm2,          1

    pshufb            m1,            m4
    pshufb            m3,            m4
    pmaddubsw         m1,            m0
    pmaddubsw         m3,            m0
    pmaddwd           m1,            m5
    pmaddwd           m3,            m5
    packssdw          m1,            m3
    pmulhrsw          m1,            [pw_512]
    vextracti128      xm2,           m1,          1
    packuswb          xm1,           xm2

    lea               r4,            [r3 * 3]
    pextrw            [r2],          xm1,         0
    pextrw            [r2 + r3],     xm1,         1
    pextrw            [r2 + r3 * 2], xm1,         4
    pextrw            [r2 + r4],     xm1,         5
    lea               r2,            [r2 + r3 * 4]
    pextrw            [r2],          xm1,         2
    pextrw            [r2 + r3],     xm1,         3
    pextrw            [r2 + r3 * 2], xm1,         6
    pextrw            [r2 + r4],     xm1,         7
    RET
