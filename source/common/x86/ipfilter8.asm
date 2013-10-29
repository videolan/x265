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

tab_c_512:      times 8 dw 512
tab_c_8192:     times 8 dw 8192
tab_c_526336:   times 4 dd 8192*64+2048

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

tab_LumaCoeffV: times 4 dw 0, 0
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

SECTION .text

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
    movd        r4, %2
    mov         [dstq], r4w
    shr         r4, 16
    mov         [dstq + dststrideq], r4w
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
    FILTER_H8_W8  m0, m1, m4, m5, m3, m2, [r0 - 3 + r5], [r2 + r5]
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
%endmacro ; FILTER_HV8_START

; Round and Saturate
%macro FILTER_HV8_END 4 ; output in [1, 3]
    paddd       %1, [tab_c_526336]
    paddd       %2, [tab_c_526336]
    paddd       %3, [tab_c_526336]
    paddd       %4, [tab_c_526336]
    psrad       %1, 12
    psrad       %2, 12
    psrad       %3, 12
    psrad       %4, 12
    packssdw    %1, %2
    packssdw    %3, %4

    ; TODO: is merge better? I think this way is short dependency link
    packuswb    %1, %1
    packuswb    %3, %3
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
    FILTER_H8_W8 m0, m1, m2, m3, coef, [tab_c_512], [r0 - 3]
    psubw       m1,         [tab_c_8192]
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

    movq        [r2],       m3
    movq        [r2 + r3],  m4

    lea         r0,         [r0 + 16 * 2]
    lea         r2,         [r2 + r3 * 2]

    inc         r6
    cmp         r6,         8/2
    jnz         .loopV

    RET


;-----------------------------------------------------------------------------
; void interp_8tap_v_sp(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height, const int coeffIdx);
;-----------------------------------------------------------------------------
INIT_XMM ssse3

%if ARCH_X86_64
cglobal interp_8tap_v_sp, 4, 7+5, 8
%define tmp_r0      r7
%define tmp_r2      r8
%define tmp_r3      r9
%define tmp_r4d     r10d
%define tmp_6rows   r11

%else ; ARCH_X86_64 = 0

cglobal interp_8tap_v_sp, 4, 7, 8, 0-(5*4)
%define tmp_r0      [(rsp + 0 * 4)]
%define tmp_r2      [(rsp + 1 * 4)]
%define tmp_r3      [(rsp + 2 * 4)]
%define tmp_r4d     [(rsp + 3 * 4)]
%define tmp_6rows   [(rsp + 4 * 4)]
%endif ; ARCH_X86_64

    mov         r4d,        r4m
    mov         r5d,        r5m

    mov         tmp_r4d, r4d
    mov         tmp_r2, r2

    ; load coeff table
    mov         r6d,        r6m
    shl         r6,         6
    lea         r4,         [tab_LumaCoeffV]
    lea         r6,         [r4 + r6]

    ; move to -3
    lea         r1, [r1 * 2]
    lea         r4, [r1 + r1 * 2]
    sub         r0, r4
    lea         r4, [r4 * 2]
    mov         tmp_6rows, r4

.loopH:

    ; load width
    mov         r4d, tmp_r4d

    ; save old src
    mov         tmp_r0, r0

.loopW:

    movu        m0, [r0]
    movu        m1, [r0 + r1]
    lea         r0, [r0 + r1 * 2]
    punpcklwd   m2, m0, m1
    pmaddwd     m2, [r6 + 0 * 16]
    punpckhwd   m0, m1
    pmaddwd     m0, [r6 + 0 * 16]

    movu        m3, [r0]
    movu        m4, [r0 + r1]
    lea         r0, [r0 + r1 * 2]
    punpcklwd   m1, m3, m4
    pmaddwd     m1, [r6 + 1 * 16]
    paddd       m2, m1
    punpckhwd   m3, m4
    pmaddwd     m3, [r6 + 1 * 16]
    paddd       m0, m3

    movu        m3, [r0]
    movu        m4, [r0 + r1]
    lea         r0, [r0 + r1 * 2]
    punpcklwd   m1, m3, m4
    pmaddwd     m1, [r6 + 2 * 16]
    paddd       m2, m1
    punpckhwd   m3, m4
    pmaddwd     m3, [r6 + 2 * 16]
    paddd       m0, m3

    movu        m3, [r0]
    movu        m4, [r0 + r1]
    punpcklwd   m1, m3, m4
    pmaddwd     m1, [r6 + 3 * 16]
    paddd       m2, m1
    punpckhwd   m3, m4
    pmaddwd     m3, [r6 + 3 * 16]
    paddd       m0, m3

    paddd       m2, [tab_c_526336]
    paddd       m0, [tab_c_526336]
    psrad       m2, 12
    psrad       m0, 12
    packssdw    m2, m0
    packuswb    m2, m2

    ; move to next 8 col
    sub         r0, tmp_6rows

    sub         r4, 8
    jl          .width4
    movq        [r2], m2
    je          .nextH
    lea         r0, [r0 + 16]
    lea         r2, [r2 + 8]
    jmp         .loopW

.width4:
    movd        [r2], m2
    lea         r0, [r0 + 4]

.nextH:
    ; move to next row
    mov         r0, tmp_r0
    lea         r0, [r0 + r1]
    add         tmp_r2, r3
    mov         r2, tmp_r2

    dec         r5d
    jnz         .loopH

    RET
