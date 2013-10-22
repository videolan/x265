;*****************************************************************************
;* Copyright (C) 2013 x265 project
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
;* For more information, contact us at licensing@multicorewareinc.com.
;*****************************************************************************/

%include "x86inc.asm"
%include "x86util.asm"

SECTION_RODATA 32
tab_Tm:    db 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 5, 6
           db 4, 5, 6, 7, 5, 6, 7, 8, 6, 7, 8, 9, 7, 8, 9, 10

tab_Lm:    db 0, 1, 2, 3, 4,  5,  6,  7,  1, 2, 3, 4,  5,  6,  7,  8
           db 2, 3, 4, 5, 6,  7,  8,  9,  3, 4, 5, 6,  7,  8,  9,  10
           db 4, 5, 6, 7, 8,  9,  10, 11, 5, 6, 7, 8,  9,  10, 11, 12
           db 6, 7, 8, 9, 10, 11, 12, 13, 7, 8, 9, 10, 11, 12, 13, 14

tab_c_512:  times 8 dw 512

tab_ChromaCoeff: db  0, 64,  0,  0
                 db -2, 58, 10, -2
                 db -4, 54, 16, -2
                 db -6, 46, 28, -4
                 db -4, 36, 36, -4
                 db -4, 28, 46, -6
                 db -2, 16, 54, -4
                 db -2, 10, 58, -2

tab_LumaCoeff:   db   0, 0,  0,  64,  0,   0,  0,  0
                 db  -1, 4, -10, 58,  17, -5,  1,  0
                 db  -1, 4, -11, 40,  40, -11, 4, -1
                 db   0, 1, -5,  17,  58, -10, 4, -1


SECTION .text

%macro FILTER_H4_w2_2 3
    movu        %2, [srcq - 1]
    pshufb      %2, %2, Tm0
    pmaddubsw   %2, coef2
    movu        %1, [srcq + srcstrideq - 1]
    pshufb      %1, %1, Tm0
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    pmulhrsw    %2, %3
    packuswb    %2, %2
    pextrw      [dstq], %2, 0
    pextrw      [dstq + dststrideq], %2, 2
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_2x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_2x4, 4, 6, 6, src, srcstride, dst, dststride
%define coef2       m5
%define Tm0         m4
%define Tm1         m3
%define t2          m2
%define t1          m1
%define t0          m0

mov         r4d,        r4m

%ifdef PIC
lea         r5,          [tab_ChromaCoeff]
movd        coef2,       [r5 + r4 * 4]
%else
movd        coef2,       [tab_ChromaCoeff + r4 * 4]
%endif

pshufd      coef2,       coef2,      0
mova        t2,          [tab_c_512]
mova        Tm0,         [tab_Tm]
mova        Tm1,         [tab_Tm + 16]

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
cglobal interp_4tap_horiz_pp_2x8, 4, 6, 6, src, srcstride, dst, dststride
%define coef2       m5
%define Tm0         m4
%define Tm1         m3
%define t2          m2
%define t1          m1
%define t0          m0

mov         r4d,        r4m

%ifdef PIC
lea         r5,          [tab_ChromaCoeff]
movd        coef2,       [r5 + r4 * 4]
%else
movd        coef2,       [tab_ChromaCoeff + r4 * 4]
%endif

pshufd      coef2,       coef2,      0
mova        t2,          [tab_c_512]
mova        Tm0,         [tab_Tm]
mova        Tm1,         [tab_Tm + 16]

%rep 4
FILTER_H4_w2_2   t0, t1, t2
lea         srcq,       [srcq + srcstrideq * 2]
lea         dstq,       [dstq + dststrideq * 2]
%endrep

RET

%macro FILTER_H4_w4_2 3
    movu        %2, [srcq - 1]
    pshufb      %2, %2, Tm0
    pmaddubsw   %2, coef2
    movu        %1, [srcq + srcstrideq - 1]
    pshufb      %1, %1, Tm0
    pmaddubsw   %1, coef2
    phaddw      %2, %1
    pmulhrsw    %2, %3
    packuswb    %2, %2
    movd        [dstq],      %2
    pextrd      [dstq + dststrideq], %2,  1
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_4x2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_4x2, 4, 6, 6, src, srcstride, dst, dststride
%define coef2       m5
%define Tm0         m4
%define Tm1         m3
%define t2          m2
%define t1          m1
%define t0          m0

mov         r4d,        r4m

%ifdef PIC
lea         r5,          [tab_ChromaCoeff]
movd        coef2,       [r5 + r4 * 4]
%else
movd        coef2,       [tab_ChromaCoeff + r4 * 4]
%endif

pshufd      coef2,       coef2,      0
mova        t2,          [tab_c_512]
mova        Tm0,         [tab_Tm]
mova        Tm1,         [tab_Tm + 16]

FILTER_H4_w4_2   t0, t1, t2

RET

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_4x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_horiz_pp_4x4, 4, 6, 6, src, srcstride, dst, dststride
%define coef2       m5
%define Tm0         m4
%define Tm1         m3
%define t2          m2
%define t1          m1
%define t0          m0

mov         r4d,        r4m

%ifdef PIC
lea         r5,          [tab_ChromaCoeff]
movd        coef2,       [r5 + r4 * 4]
%else
movd        coef2,       [tab_ChromaCoeff + r4 * 4]
%endif

pshufd      coef2,       coef2,      0
mova        t2,          [tab_c_512]
mova        Tm0,         [tab_Tm]
mova        Tm1,         [tab_Tm + 16]

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
cglobal interp_4tap_horiz_pp_4x8, 4, 6, 6, src, srcstride, dst, dststride
%define coef2       m5
%define Tm0         m4
%define Tm1         m3
%define t2          m2
%define t1          m1
%define t0          m0

mov         r4d,        r4m

%ifdef PIC
lea         r5,          [tab_ChromaCoeff]
movd        coef2,       [r5 + r4 * 4]
%else
movd        coef2,       [tab_ChromaCoeff + r4 * 4]
%endif

pshufd      coef2,       coef2,      0
mova        t2,          [tab_c_512]
mova        Tm0,         [tab_Tm]
mova        Tm1,         [tab_Tm + 16]

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
cglobal interp_4tap_horiz_pp_4x16, 4, 6, 6, src, srcstride, dst, dststride
%define coef2       m5
%define Tm0         m4
%define Tm1         m3
%define t2          m2
%define t1          m1
%define t0          m0

mov         r4d,        r4m

%ifdef PIC
lea         r5,          [tab_ChromaCoeff]
movd        coef2,       [r5 + r4 * 4]
%else
movd        coef2,       [tab_ChromaCoeff + r4 * 4]
%endif

pshufd      coef2,       coef2,      0
mova        t2,          [tab_c_512]
mova        Tm0,         [tab_Tm]
mova        Tm1,         [tab_Tm + 16]

%rep 8
FILTER_H4_w4_2   t0, t1, t2
lea         srcq,       [srcq + srcstrideq * 2]
lea         dstq,       [dstq + dststrideq * 2]
%endrep

RET

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
lea         r5,          [tab_ChromaCoeff]
movd        coef2,       [r5 + r4 * 4]
%else
movd        coef2,       [tab_ChromaCoeff + r4 * 4]
%endif

mov           r5d,       %2

pshufd      coef2,       coef2,      0
mova        t2,          [tab_c_512]
mova        Tm0,         [tab_Tm]
mova        Tm1,         [tab_Tm + 16]

.loop
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
lea         r5,          [tab_ChromaCoeff]
movd        coef2,       [r5 + r4 * 4]
%else
movd        coef2,       [tab_ChromaCoeff + r4 * 4]
%endif

mov         r5d,          %2

pshufd      coef2,       coef2,      0
mova        t2,          [tab_c_512]
mova        Tm0,         [tab_Tm]
mova        Tm1,         [tab_Tm + 16]

.loop
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


%macro FILTER_H8_W8 3
    movu        %1, [r0 - 3 + r5]
    pshufb      %2, %1, [tab_Lm]
    pmaddubsw   %2, m3
    pshufb      m7, %1, [tab_Lm + 16]
    pmaddubsw   m7, m3
    phaddw      %2, m7
    pshufb      m7, %1, [tab_Lm + 32]
    pmaddubsw   m7, m3
    pshufb      %1, %1, [tab_Lm + 48]
    pmaddubsw   %1, m3
    phaddw      m7, %1
    phaddw      %2, m7
    pmulhrsw    %2, %3
    packuswb    %2, %2
    movh        [r2 + r5], %2
%endmacro

%macro FILTER_H8_W4 3
    movu        %1, [r0 - 3 + r5]
    pshufb      %2, %1, [tab_Lm]
    pmaddubsw   %2, m3
    pshufb      m7, %1, [tab_Lm + 16]
    pmaddubsw   m7, m3
    phaddw      %2, m7
    phaddw      %2, %2
    pmulhrsw    %2, %3
    packuswb    %2, %2
    movd       [r2 + r5], %2
%endmacro

%macro FILTER_H8_W1 3
    movu        %1, [r0 - 3 + r5]
    pshufb      %2, %1, [tab_Lm]
    pmaddubsw   %2, m3
    phaddw      %2, %2
    phaddw      %2, %2
    pmulhrsw    %2, %3
    packuswb    %2, %2
    pextrb      [r2 + r5], %2, 0
%endmacro

;-----------------------------------------------------------------------------
; void interp_8tap_horiz_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro IPFILTER_LUMA 2
cglobal interp_8tap_horiz_pp_%1x%2, 4, 6, 5

mov         r4d,        r4m

%ifdef PIC
lea         r5,       [tab_LumaCoeff]
movh        m3,       [r5 + r4 * 8]
%else
movh        m3,       [tab_LumaCoeff + r4 * 8]
%endif

punpcklqdq  m3,       m3
mova        m2,       [tab_c_512]
mov         r4,       %2

.loop
    xor    r5,    r5
%rep %1 / 8
    FILTER_H8_W8  m0, m1, m2
    add    r5,    8
%endrep

%rep (%1 % 8) / 4
    FILTER_H8_W4  m0, m1, m2
    add     r5,   4
%endrep

 %rep(%1 % 4)
    FILTER_H8_W1  m0, m1, m2
    add    r5,    1
 %endrep

    add    r0,    r1
    add    r2,    r3

    dec    r4d
    jnz   .loop
    RET
%endmacro

    IPFILTER_LUMA 4,   4
    IPFILTER_LUMA 8,   8
    IPFILTER_LUMA 8,   4
    IPFILTER_LUMA 4,   8
    IPFILTER_LUMA 16, 16
    IPFILTER_LUMA 16,  8
    IPFILTER_LUMA 8,  16
    IPFILTER_LUMA 16, 12
    IPFILTER_LUMA 12, 16
    IPFILTER_LUMA 16,  4
    IPFILTER_LUMA 4,  16
    IPFILTER_LUMA 32, 32
    IPFILTER_LUMA 32, 16
    IPFILTER_LUMA 16, 32
    IPFILTER_LUMA 32, 24
    IPFILTER_LUMA 24, 32
    IPFILTER_LUMA 32,  8
    IPFILTER_LUMA 8,  32
    IPFILTER_LUMA 64, 64
    IPFILTER_LUMA 64, 32
    IPFILTER_LUMA 32, 64
    IPFILTER_LUMA 64, 48
    IPFILTER_LUMA 48, 64
    IPFILTER_LUMA 64, 16
    IPFILTER_LUMA 16, 64
