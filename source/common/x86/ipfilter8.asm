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
           db 8, 9,10,11, 9,10,11,12,10,11,12,13,11,12,13, 14

tab_Lm:    db 0, 1, 2, 3, 4,  5,  6,  7,  1, 2, 3, 4,  5,  6,  7,  8
           db 2, 3, 4, 5, 6,  7,  8,  9,  3, 4, 5, 6,  7,  8,  9,  10
           db 4, 5, 6, 7, 8,  9,  10, 11, 5, 6, 7, 8,  9,  10, 11, 12
           db 6, 7, 8, 9, 10, 11, 12, 13, 7, 8, 9, 10, 11, 12, 13, 14

tab_Vm:    db 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1
           db 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3

tab_Cm:    db 0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3

tab_c_512:      times 8 dw 512
tab_c_526336:   times 4 dd 8192*64+2048

tab_ChromaCoeff: db  0, 64,  0,  0
                 db -2, 58, 10, -2
                 db -4, 54, 16, -2
                 db -6, 46, 28, -4
                 db -4, 36, 36, -4
                 db -4, 28, 46, -6
                 db -2, 16, 54, -4
                 db -2, 10, 58, -2

tab_ChromaCoeffV: times 4 dw 0, 64
                  times 4 dw 0, 0

                  times 4 dw -2, 58
                  times 4 dw 10, -2

                  times 4 dw -4, 54
                  times 4 dw 16, -2

                  times 4 dw -6, 46 
                  times 4 dw 28, -4

                  times 4 dw -4, 36
                  times 4 dw 36, -4

                  times 4 dw -4, 28
                  times 4 dw 46, -6

                  times 4 dw -2, 16
                  times 4 dw 54, -4

                  times 4 dw -2, 10
                  times 4 dw 58, -2

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

tab_LumaCoeffVer: times 8 db 0, 0
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

tab_c_128:      times 16 db 0x80
tab_c_64_n64:   times 8 db 64, -64


SECTION .text

cextern pw_512
cextern pw_2000

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

%macro FILTER_H8_W4 2
    movu        %1, [r0 - 3 + r5]
    pshufb      %2, %1, [tab_Lm]
    pmaddubsw   %2, m3
    pshufb      m7, %1, [tab_Lm + 16]
    pmaddubsw   m7, m3
    phaddw      %2, m7
    phaddw      %2, %2
%endmacro

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
%macro IPFILTER_LUMA 3
INIT_XMM sse4
cglobal interp_8tap_horiz_%3_%1x%2, 4,7,6

    mov       r4d, r4m

%ifdef PIC
    lea       r6, [tab_LumaCoeff]
    movh      m3, [r6 + r4 * 8]
%else
    movh      m3, [tab_LumaCoeff + r4 * 8]
%endif
    punpcklqdq  m3, m3

%ifidn %3, pp 
    mova      m2, [tab_c_512]
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

.loopH
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

;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
    IPFILTER_LUMA 4, 4, pp
    IPFILTER_LUMA 4, 8, pp
    IPFILTER_LUMA 12, 16, pp
    IPFILTER_LUMA 4, 16, pp


;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro IPFILTER_LUMA_PP_W8 2
INIT_XMM sse4
cglobal interp_8tap_horiz_pp_%1x%2, 4,6,7
    mov         r4d, r4m

%ifdef PIC
    lea         r5, [tab_LumaCoeff]
    movh        m3, [r5 + r4 * 8]
%else
    movh        m3, [tab_LumaCoeff + r4 * 8]
%endif
    pshufd      m0, m3, 0                       ; m0 = coeff-L
    pshufd      m1, m3, 0x55                    ; m1 = coeff-H
    lea         r5, [tab_Tm]                    ; r5 = shuffle
    mova        m2, [pw_512]                    ; m2 = 512

    mov         r4d, %2
.loopH
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

    movq        [r2],       m3
    movq        [r2 + r3],  m4

    lea         r0,         [r0 + 16 * 2]
    lea         r2,         [r2 + r3 * 2]

    inc         r6
    cmp         r6,         8/2
    jnz         .loopV

    RET

;-----------------------------------------------------------------------------
;void interp_4tap_vert_pp_2x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_pp_2x4, 4, 7, 8

mov         r4d,       r4m
sub         r0,        r1

%ifdef PIC
lea         r5,        [tab_ChromaCoeff]
movd        m0,        [r5 + r4 * 4]
%else
movd        m0,        [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m0,        [tab_Cm]

mova        m1,        [tab_c_512]

movd        m2,        [r0]
movd        m3,        [r0 + r1]
movd        m4,        [r0 + 2 * r1]
lea         r5,        [r0 + 2 * r1]
movd        m5,        [r5 + r1]

punpcklbw   m2,        m3
punpcklbw   m6,        m4,        m5
punpcklbw   m2,        m6

pmaddubsw   m2,        m0

movd        m6,        [r0 + 4 * r1]

punpcklbw   m3,        m4
punpcklbw   m7,        m5,        m6
punpcklbw   m3,        m7

pmaddubsw   m3,        m0

phaddw      m2,        m3

pmulhrsw    m2,        m1
packuswb    m2,        m2

pextrw      [r2],      m2,  0
pextrw      [r2 + r3], m2,  2

lea         r5,        [r0 + 4 * r1]
movd        m2,        [r5 + r1]

punpcklbw   m4,        m5
punpcklbw   m3,        m6,        m2
punpcklbw   m4,        m3

pmaddubsw   m4,        m0

movd        m3,        [r5 + 2 * r1]

punpcklbw   m5,        m6
punpcklbw   m2,        m3
punpcklbw   m5,        m2

pmaddubsw   m5,        m0

phaddw      m4,        m5

pmulhrsw    m4,        m1
packuswb    m4,        m4

pextrw      [r2 + 2 * r3],    m4,    0
lea         r6,               [r2 + 2 * r3]
pextrw      [r6 + r3],        m4,    2

RET

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_2x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W2_H4 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_2x8, 4, 7, 8

mov         r4d,       r4m
sub         r0,        r1

%ifdef PIC
lea         r5,        [tab_ChromaCoeff]
movd        m0,        [r5 + r4 * 4]
%else
movd        m0,        [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m0,        [tab_Cm]

mova        m1,        [tab_c_512]

mov         r4d,       %2

.loop
movd        m2,        [r0]
movd        m3,        [r0 + r1]
movd        m4,        [r0 + 2 * r1]
lea         r5,        [r0 + 2 * r1]
movd        m5,        [r5 + r1]

punpcklbw   m2,        m3
punpcklbw   m6,        m4,        m5
punpcklbw   m2,        m6

pmaddubsw   m2,        m0

movd        m6,        [r0 + 4 * r1]

punpcklbw   m3,        m4
punpcklbw   m7,        m5,        m6
punpcklbw   m3,        m7

pmaddubsw   m3,        m0

phaddw      m2,        m3

pmulhrsw    m2,        m1
packuswb    m2,        m2

pextrw      [r2],      m2,  0
pextrw      [r2 + r3], m2,  2

lea         r5,        [r0 + 4 * r1]
movd        m2,        [r5 + r1]

punpcklbw   m4,        m5
punpcklbw   m3,        m6,        m2
punpcklbw   m4,        m3

pmaddubsw   m4,        m0

movd        m3,        [r5 + 2 * r1]

punpcklbw   m5,        m6
punpcklbw   m2,        m3
punpcklbw   m5,        m2

pmaddubsw   m5,        m0

phaddw      m4,        m5

pmulhrsw    m4,        m1
packuswb    m4,        m4

pextrw      [r2 + 2 * r3],    m4,    0
lea         r6,               [r2 + 2 * r3]
pextrw      [r6 + r3],        m4,    2

lea         r0,        [r0 + 4 * r1]
lea         r2,        [r2 + 4 * r3]

sub         r4,        4
jnz        .loop
RET
%endmacro

FILTER_V4_W2_H4 2, 8

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_4x2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_pp_4x2, 4, 6, 8

mov         r4d,       r4m
sub         r0,        r1

%ifdef PIC
lea         r5,        [tab_ChromaCoeff]
movd        m0,        [r5 + r4 * 4]
%else
movd        m0,        [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m0,        [tab_Cm]

mova        m1,        [tab_c_512]

movd        m2,        [r0]
movd        m3,        [r0 + r1]
movd        m4,        [r0 + 2 * r1]
lea         r5,        [r0 + 2 * r1]
movd        m5,        [r5 + r1]

punpcklbw   m2,        m3
punpcklbw   m6,        m4,        m5
punpcklbw   m2,        m6

pmaddubsw   m2,        m0

movd        m6,        [r0 + 4 * r1]

punpcklbw   m3,        m4
punpcklbw   m5,        m6
punpcklbw   m3,        m5

pmaddubsw   m3,        m0

phaddw      m2,        m3

pmulhrsw    m2,        m1
packuswb    m2,        m2
movd        [r2],      m2
pextrd      [r2 + r3], m2,  1

RET

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_4x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_pp_4x4, 4, 7, 8

mov         r4d,       r4m
sub         r0,        r1

%ifdef PIC
lea         r5,        [tab_ChromaCoeff]
movd        m0,        [r5 + r4 * 4]
%else
movd        m0,        [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m0,        [tab_Cm]

mova        m1,        [tab_c_512]

movd        m2,        [r0]
movd        m3,        [r0 + r1]
movd        m4,        [r0 + 2 * r1]
lea         r5,        [r0 + 2 * r1]
movd        m5,        [r5 + r1]

punpcklbw   m2,        m3
punpcklbw   m6,        m4,        m5
punpcklbw   m2,        m6

pmaddubsw   m2,        m0

movd        m6,        [r0 + 4 * r1]

punpcklbw   m3,        m4
punpcklbw   m7,        m5,        m6
punpcklbw   m3,        m7

pmaddubsw   m3,        m0

phaddw      m2,        m3

pmulhrsw    m2,        m1
packuswb    m2,        m2
movd        [r2],      m2
pextrd      [r2 + r3], m2,  1

lea         r5,        [r0 + 4 * r1]
movd        m2,        [r5 + r1]

punpcklbw   m4,        m5
punpcklbw   m3,        m6,        m2
punpcklbw   m4,        m3

pmaddubsw   m4,        m0

movd        m3,        [r5 + 2 * r1]

punpcklbw   m5,        m6
punpcklbw   m2,        m3
punpcklbw   m5,        m2

pmaddubsw   m5,        m0

phaddw      m4,        m5

pmulhrsw    m4,        m1
packuswb    m4,        m4
movd        [r2 + 2 * r3],      m4
lea         r6,        [r2 + 2 * r3]
pextrd      [r6 + r3], m4,  1

RET

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W4_H4 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_%1x%2, 4, 7, 8

mov         r4d,       r4m
sub         r0,        r1

%ifdef PIC
lea         r5,        [tab_ChromaCoeff]
movd        m0,        [r5 + r4 * 4]
%else
movd        m0,        [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m0,        [tab_Cm]

mova        m1,        [tab_c_512]

mov         r4d,       %2

.loop
movd        m2,        [r0]
movd        m3,        [r0 + r1]
movd        m4,        [r0 + 2 * r1]
lea         r5,        [r0 + 2 * r1]
movd        m5,        [r5 + r1]

punpcklbw   m2,        m3
punpcklbw   m6,        m4,        m5
punpcklbw   m2,        m6

pmaddubsw   m2,        m0

movd        m6,        [r0 + 4 * r1]

punpcklbw   m3,        m4
punpcklbw   m7,        m5,        m6
punpcklbw   m3,        m7

pmaddubsw   m3,        m0

phaddw      m2,        m3

pmulhrsw    m2,        m1
packuswb    m2,        m2
movd        [r2],      m2
pextrd      [r2 + r3], m2,  1

lea         r5,        [r0 + 4 * r1]
movd        m2,        [r5 + r1]

punpcklbw   m4,        m5
punpcklbw   m3,        m6,        m2
punpcklbw   m4,        m3

pmaddubsw   m4,        m0

movd        m3,        [r5 + 2 * r1]

punpcklbw   m5,        m6
punpcklbw   m2,        m3
punpcklbw   m5,        m2

pmaddubsw   m5,        m0

phaddw      m4,        m5

pmulhrsw    m4,        m1
packuswb    m4,        m4
movd        [r2 + 2 * r3],      m4
lea         r6,        [r2 + 2 * r3]
pextrd      [r6 + r3], m4,  1

lea         r0,        [r0 + 4 * r1]
lea         r2,        [r2 + 4 * r3]

sub         r4,        4
jnz        .loop
RET
%endmacro

FILTER_V4_W4_H4 4,  8
FILTER_V4_W4_H4 4, 16

%macro FILTER_V4_W8_H2 0
punpcklbw   m1,        m2
punpcklbw   m7,        m3,        m0

pmaddubsw   m1,        m6
pmaddubsw   m7,        m5

paddw       m1,        m7

pmulhrsw    m1,        m4
packuswb    m1,        m1
%endmacro

%macro FILTER_V4_W8_H3 0
punpcklbw   m2,        m3
punpcklbw   m7,        m0,        m1

pmaddubsw   m2,        m6
pmaddubsw   m7,        m5

paddw       m2,        m7

pmulhrsw    m2,        m4
packuswb    m2,        m2
%endmacro

%macro FILTER_V4_W8_H4 0
punpcklbw   m3,        m0
punpcklbw   m7,        m1,        m2

pmaddubsw   m3,        m6
pmaddubsw   m7,        m5

paddw       m3,        m7

pmulhrsw    m3,        m4
packuswb    m3,        m3
%endmacro

%macro FILTER_V4_W8_H5 0
punpcklbw   m0,        m1
punpcklbw   m7,        m2,        m3

pmaddubsw   m0,        m6
pmaddubsw   m7,        m5

paddw       m0,        m7

pmulhrsw    m0,        m4
packuswb    m0,        m0
%endmacro

%macro FILTER_V4_W8_8x2 2
FILTER_V4_W8 %1, %2
movq        m0,        [r0 + 4 * r1]

FILTER_V4_W8_H2

movh        [r2 + r3], m1
%endmacro

%macro FILTER_V4_W8_8x4 2
FILTER_V4_W8_8x2 %1, %2
;8x3
lea         r6,        [r0 + 4 * r1]
movq        m1,        [r6 + r1]

FILTER_V4_W8_H3

movh        [r2 + 2 * r3], m2

;8x4
movq        m2,        [r6 + 2 * r1]

FILTER_V4_W8_H4

lea         r5,        [r2 + 2 * r3]
movh        [r5 + r3], m3
%endmacro

%macro FILTER_V4_W8_8x6 2
FILTER_V4_W8_8x4 %1, %2
;8x5
lea         r6,        [r6 + 2 * r1]
movq        m3,        [r6 + r1]

FILTER_V4_W8_H5

movh        [r2 + 4 * r3], m0

;8x6
movq        m0,        [r0 + 8 * r1]

FILTER_V4_W8_H2

lea         r5,        [r2 + 4 * r3]
movh        [r5 + r3], m1
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W8 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_%1x%2, 4, 7, 8

mov         r4d,       r4m

sub         r0,        r1
movq        m0,        [r0]
movq        m1,        [r0 + r1]
movq        m2,        [r0 + 2 * r1]
lea         r5,        [r0 + 2 * r1]
movq        m3,        [r5 + r1]

punpcklbw   m0,        m1
punpcklbw   m4,        m2,          m3

%ifdef PIC
lea         r6,        [tab_ChromaCoeff]
movd        m5,        [r6 + r4 * 4]
%else
movd        m5,        [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m6,        m5,       [tab_Vm]
pmaddubsw   m0,        m6

pshufb      m5,        [tab_Vm + 16]
pmaddubsw   m4,        m5

paddw       m0,        m4

mova        m4,        [tab_c_512]

pmulhrsw    m0,        m4
packuswb    m0,        m0
movh        [r2],      m0
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_8x2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
FILTER_V4_W8_8x2 8, 2

RET

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_8x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
FILTER_V4_W8_8x4 8, 4

RET

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_8x6(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
FILTER_V4_W8_8x6 8, 6

RET

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_4x2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_ps_4x2, 4, 6, 8

mov         r4d, r4m
sub         r0, r1
add         r3d, r3d

%ifdef PIC
lea         r5, [tab_ChromaCoeff]
movd        m0, [r5 + r4 * 4]
%else
movd        m0, [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m0, [tab_Cm]

mova        m1, [pw_2000]

movd        m2, [r0]
movd        m3, [r0 + r1]
movd        m4, [r0 + 2 * r1]
lea         r5, [r0 + 2 * r1]
movd        m5, [r5 + r1]

punpcklbw   m2, m3
punpcklbw   m6, m4, m5
punpcklbw   m2, m6

pmaddubsw   m2, m0

movd        m6, [r0 + 4 * r1]

punpcklbw   m3, m4
punpcklbw   m5, m6
punpcklbw   m3, m5

pmaddubsw   m3, m0

phaddw      m2, m3

psubw       m2, m1
movlps      [r2], m2
movhps      [r2 + r3], m2

RET

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_4x4(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_ps_4x4, 4, 7, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m0, [tab_Cm]

    mova       m1, [pw_2000]

    movd       m2, [r0]
    movd       m3, [r0 + r1]
    movd       m4, [r0 + 2 * r1]
    lea        r5, [r0 + 2 * r1]
    movd       m5, [r5 + r1]

    punpcklbw  m2, m3
    punpcklbw  m6, m4, m5
    punpcklbw  m2, m6

    pmaddubsw  m2, m0

    movd       m6, [r0 + 4 * r1]

    punpcklbw  m3, m4
    punpcklbw  m7, m5, m6
    punpcklbw  m3, m7

    pmaddubsw  m3, m0

    phaddw     m2, m3

    psubw      m2, m1
    movlps     [r2], m2
    movhps     [r2 + r3], m2

    lea        r5, [r0 + 4 * r1]
    movd       m2, [r5 + r1]

    punpcklbw  m4, m5
    punpcklbw  m3, m6, m2
    punpcklbw  m4, m3

    pmaddubsw  m4, m0

    movd       m3, [r5 + 2 * r1]

    punpcklbw  m5, m6
    punpcklbw  m2, m3
    punpcklbw  m5, m2

    pmaddubsw  m5, m0

    phaddw     m4, m5

    psubw      m4, m1
    movlps     [r2 + 2 * r3], m4
    lea        r6, [r2 + 2 * r3]
    movhps     [r6 + r3], m4

    RET

;---------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_%1x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------
%macro FILTER_V_PS_W4_H4 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_%1x%2, 4, 7, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m0, [tab_Cm]

    mova       m1, [pw_2000]

    mov        r4d, %2/4

.loop
    movd       m2, [r0]
    movd       m3, [r0 + r1]
    movd       m4, [r0 + 2 * r1]
    lea        r5, [r0 + 2 * r1]
    movd       m5, [r5 + r1]

    punpcklbw  m2, m3
    punpcklbw  m6, m4, m5
    punpcklbw  m2, m6

    pmaddubsw  m2, m0

    movd       m6, [r0 + 4 * r1]

    punpcklbw  m3, m4
    punpcklbw  m7, m5, m6
    punpcklbw  m3, m7

    pmaddubsw  m3, m0

    phaddw     m2, m3

    psubw      m2, m1
    movlps     [r2], m2
    movhps     [r2 + r3], m2

    lea        r5, [r0 + 4 * r1]
    movd       m2, [r5 + r1]

    punpcklbw  m4, m5
    punpcklbw  m3, m6, m2
    punpcklbw  m4, m3

    pmaddubsw  m4, m0

    movd       m3, [r5 + 2 * r1]

    punpcklbw  m5, m6
    punpcklbw  m2, m3
    punpcklbw  m5, m2

    pmaddubsw  m5, m0

    phaddw     m4, m5

    psubw      m4, m1
    movlps     [r2 + 2 * r3], m4
    lea        r6, [r2 + 2 * r3]
    movhps     [r6 + r3], m4

    lea        r0, [r0 + 4 * r1]
    lea        r2, [r2 + 4 * r3]

    dec        r4d
    jnz        .loop
    RET
%endmacro

FILTER_V_PS_W4_H4 4, 8
FILTER_V_PS_W4_H4 4, 16

;--------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_8x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_V_PS_W8_H8_H16_H2 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_%1x%2, 4, 7, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r6, [tab_ChromaCoeff]
    movd       m5, [r6 + r4 * 4]
%else
    movd       m5, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m6, m5, [tab_Vm]
    pshufb     m5, [tab_Vm + 16]
    mova       m4, [pw_2000]

    mov        r4d, %2/2

.loopH
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    movq       m2, [r0 + 2 * r1]
    lea        r5, [r0 + 2 * r1]
    movq       m3, [r5 + r1]

    punpcklbw  m0, m1
    punpcklbw  m7, m2, m3

    pmaddubsw  m0, m6
    pmaddubsw  m7, m5

    paddw      m0, m7

    psubw      m0, m4
    movu       [r2], m0

    movq       m0, [r0 + 4 * r1]

    punpcklbw  m1, m2
    punpcklbw  m7, m3, m0

    pmaddubsw  m1, m6
    pmaddubsw  m7, m5

    paddw      m1, m7
    psubw      m1, m4

    movu       [r2 + r3], m1

    lea        r0, [r0 + 2 * r1]
    lea        r2, [r2 + 2 * r3]

    dec        r4d
    jnz       .loopH

    RET
%endmacro

FILTER_V_PS_W8_H8_H16_H2 8, 2
FILTER_V_PS_W8_H8_H16_H2 8, 4
FILTER_V_PS_W8_H8_H16_H2 8, 6

;--------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_8x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_V_PS_W8_H8_H16_H32 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_%1x%2, 4, 7, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r6, [tab_ChromaCoeff]
    movd       m5, [r6 + r4 * 4]
%else
    movd       m5, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m6, m5, [tab_Vm]
    pshufb     m5, [tab_Vm + 16]
    mova       m4, [pw_2000]

    mov        r4d, %2/4

.loop
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    movq       m2, [r0 + 2 * r1]
    lea        r5, [r0 + 2 * r1]
    movq       m3, [r5 + r1]

    punpcklbw  m0, m1
    punpcklbw  m7, m2, m3

    pmaddubsw  m0, m6
    pmaddubsw  m7, m5

    paddw      m0, m7

    psubw       m0, m4
    movu       [r2], m0

    movq       m0, [r0 + 4 * r1]

    punpcklbw  m1, m2
    punpcklbw  m7, m3, m0

    pmaddubsw  m1, m6
    pmaddubsw  m7, m5

    paddw      m1, m7

    psubw      m1, m4
    movu       [r2 + r3], m1

    lea        r6, [r0 + 4 * r1]
    movq       m1, [r6 + r1]

    punpcklbw  m2, m3
    punpcklbw  m7, m0, m1

    pmaddubsw  m2, m6
    pmaddubsw  m7, m5

    paddw      m2, m7

    psubw      m2, m4
    movu       [r2 + 2 * r3], m2

    movq       m2, [r6 + 2 * r1]

    punpcklbw  m3, m0
    punpcklbw  m1, m2

    pmaddubsw  m3, m6
    pmaddubsw  m1, m5

    paddw      m3, m1
    psubw      m3, m4

    lea        r5, [r2 + 2 * r3]
    movu       [r5 + r3], m3

    lea        r0, [r0 + 4 * r1]
    lea        r2, [r2 + 4 * r3]

    dec        r4d
    jnz        .loop
    RET
%endmacro

FILTER_V_PS_W8_H8_H16_H32 8,  8
FILTER_V_PS_W8_H8_H16_H32 8, 16
FILTER_V_PS_W8_H8_H16_H32 8, 32

;------------------------------------------------------------------------------------------------------------
;void interp_4tap_vert_ps_6x8(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_ps_6x8, 4, 7, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r6, [tab_ChromaCoeff]
    movd       m5, [r6 + r4 * 4]
%else
    movd       m5, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m6, m5, [tab_Vm]
    pshufb     m5, [tab_Vm + 16]
    mova       m4, [pw_2000]

    mov        r4d, 2

.loop
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    movq       m2, [r0 + 2 * r1]
    lea        r5, [r0 + 2 * r1]
    movq       m3, [r5 + r1]

    punpcklbw  m0, m1
    punpcklbw  m7, m2, m3

    pmaddubsw  m0, m6
    pmaddubsw  m7, m5

    paddw      m0, m7

    psubw      m0, m4
    movh       [r2], m0
    pshufd     m0, m0, 2
    movd       [r2 + 8], m0

    movq       m0, [r0 + 4 * r1]

    punpcklbw  m1, m2
    punpcklbw  m7, m3, m0

    pmaddubsw  m1, m6
    pmaddubsw  m7, m5

    paddw      m1, m7
    psubw      m1, m4

    movh       [r2 + r3], m1
    pshufd     m1, m1, 2
    movd       [r2 + r3 + 8], m1

    lea        r6, [r0 + 4 * r1]
    movq       m1, [r6 + r1]

    punpcklbw  m2, m3
    punpcklbw  m7, m0, m1

    pmaddubsw  m2, m6
    pmaddubsw  m7, m5

    paddw      m2, m7
    psubw      m2, m4

    movh       [r2 + 2 * r3], m2
    pshufd     m2, m2, 2
    movd       [r2 + 2 * r3 + 8], m2

    movq       m2,[r6 + 2 * r1]

    punpcklbw  m3, m0
    punpcklbw  m1, m2

    pmaddubsw  m3, m6
    pmaddubsw  m1, m5

    paddw      m3, m1
    psubw      m3, m4

    lea        r5,[r2 + 2 * r3]
    movh       [r5 + r3], m3
    pshufd     m3, m3, 2
    movd       [r5 + r3 + 8], m3

    lea        r0, [r0 + 4 * r1]
    lea        r2, [r2 + 4 * r3]

    dec        r4d
    jnz        .loop
    RET

;---------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_12x16(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_ps_12x16, 4, 6, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m1, m0, [tab_Vm]
    pshufb     m0, [tab_Vm + 16]

    mova       m7, [pw_2000]

    mov        r4d, 16/2

.loop
    movu       m2, [r0]
    movu       m3, [r0 + r1]

    punpcklbw  m4, m2, m3,
    punpckhbw  m2, m3,

    pmaddubsw  m4, m1
    pmaddubsw  m2, m1

    movu       m5, [r0 + 2 * r1]
    lea        r5, [r0 + 2 * r1]
    movu       m3, [r5 + r1]

    punpcklbw  m6, m5, m3,
    punpckhbw  m5, m3,

    pmaddubsw  m6, m0
    pmaddubsw  m5, m0

    paddw      m4, m6
    paddw      m2, m5

    psubw      m4, m7
    psubw      m2, m7

    movu       [r2], m4
    movh       [r2 + 16], m2

    movu       m2, [r0 + r1]
    movu       m3, [r0 + 2 * r1]

    punpcklbw  m4, m2, m3,
    punpckhbw  m2, m3,

    pmaddubsw  m4, m1
    pmaddubsw  m2, m1

    lea        r5, [r0 + 2 * r1]
    movu       m5, [r5 + r1]
    movu       m3, [r5 + 2 * r1]

    punpcklbw  m6, m5, m3,
    punpckhbw  m5, m3,

    pmaddubsw  m6, m0
    pmaddubsw  m5, m0

    paddw      m4, m6
    paddw      m2, m5

    psubw      m4, m7
    psubw      m2, m7

    movu       [r2 + r3], m4
    movh       [r2 + r3 + 16], m2

    lea        r0, [r0 + 2 * r1]
    lea        r2, [r2 + 2 * r3]

    dec        r4d
    jnz        .loop
    RET

;---------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_16x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------
%macro FILTER_V_PS_W16 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_%1x%2, 4, 6, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m1, m0, [tab_Vm]
    pshufb     m0, [tab_Vm + 16]

    mov        r4d, %2/2

.loop
    movu       m2, [r0]
    movu       m3, [r0 + r1]

    punpcklbw  m4, m2, m3
    punpckhbw  m5, m2, m3

    pmaddubsw  m4, m1
    pmaddubsw  m5, m1

    movu       m2, [r0 + 2 * r1]
    lea        r5, [r0 + 2 * r1]
    movu       m3, [r5 + r1]

    punpcklbw  m6, m2, m3
    punpckhbw  m7, m2, m3

    pmaddubsw  m6, m0
    pmaddubsw  m7, m0

    paddw      m4, m6
    paddw      m5, m7

    mova       m6, [pw_2000]

    psubw      m4, m6
    psubw      m5, m6

    movu       [r2], m4
    movu       [r2 + 16], m5

    movu       m2, [r0 + r1]
    movu       m3, [r0 + 2 * r1]

    punpcklbw  m4, m2, m3
    punpckhbw  m5, m2, m3

    pmaddubsw  m4, m1
    pmaddubsw  m5, m1

    lea        r5, [r0 + 2 * r1]
    movu       m2, [r5 + r1]
    movu       m3, [r5 + 2 * r1]

    punpcklbw  m6, m2, m3,
    punpckhbw  m7, m2, m3,

    pmaddubsw  m6, m0
    pmaddubsw  m7, m0

    paddw      m4, m6
    paddw      m5, m7

    mova       m6, [pw_2000]

    psubw      m4, m6
    psubw      m5, m6

    movu       [r2 + r3], m4
    movu       [r2 + r3 + 16], m5

    lea        r0, [r0 + 2 * r1]
    lea        r2, [r2 + 2 * r3]

    dec        r4d
    jnz        .loop
    RET
%endmacro

FILTER_V_PS_W16 16,  4
FILTER_V_PS_W16 16,  8
FILTER_V_PS_W16 16, 12
FILTER_V_PS_W16 16, 16
FILTER_V_PS_W16 16, 32

;--------------------------------------------------------------------------------------------------------------
;void interp_4tap_vert_ps_24x32(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_ps_24x32, 4, 6, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m1, m0, [tab_Vm]
    pshufb     m0, [tab_Vm + 16]

    mova       m7, [pw_2000]

    mov        r4d, 32/2

.loop
    movu       m2, [r0]
    movu       m3, [r0 + r1]

    punpcklbw  m4, m2, m3,
    punpckhbw  m2, m3,

    pmaddubsw  m4, m1
    pmaddubsw  m2, m1

    movu       m5, [r0 + 2 * r1]
    lea        r5, [r0 + 2 * r1]
    movu       m3, [r5 + r1]

    punpcklbw  m6, m5, m3,
    punpckhbw  m5, m3

    pmaddubsw  m6, m0
    pmaddubsw  m5, m0

    paddw      m4, m6
    paddw      m2, m5

    psubw      m4, m7
    psubw      m2, m7

    movu       [r2], m4
    movu       [r2 + 16], m2

    movq       m2, [r0 + 16]
    movq       m3, [r0 + r1 + 16]
    movq       m4, [r0 + 2 * r1 + 16]
    movq       m5, [r5 + r1 + 16]

    punpcklbw  m2, m3
    punpcklbw  m4, m5

    pmaddubsw  m2, m1
    pmaddubsw  m4, m0

    paddw      m2, m4
    psubw      m2, m7

    movu       [r2 + 32], m2

    movu       m2, [r0 + r1]
    movu       m3, [r0 + 2 * r1]

    punpcklbw  m4, m2, m3,
    punpckhbw  m2, m3,

    pmaddubsw  m4, m1
    pmaddubsw  m2, m1

    lea        r5, [r0 + 2 * r1]
    movu       m5, [r5 +  r1]
    movu       m3, [r5 + 2 * r1]

    punpcklbw  m6, m5, m3,
    punpckhbw  m5, m3

    pmaddubsw  m6, m0
    pmaddubsw  m5, m0

    paddw      m4, m6
    paddw      m2, m5

    psubw      m4, m7
    psubw      m2, m7

    movu       [r2 + r3], m4
    movu       [r2 + r3 + 16], m2

    movq       m2, [r0 + r1 + 16]
    movq       m3, [r0 + 2 * r1 + 16]
    movq       m4, [r5 + r1 + 16]
    movq       m5, [r5 + 2 * r1 + 16]

    punpcklbw  m2, m3
    punpcklbw  m4, m5

    pmaddubsw  m2, m1
    pmaddubsw  m4, m0

    paddw      m2, m4

    psubw      m2,  m7
    movu       [r2 + r3 + 32], m2

    lea        r0, [r0 + 2 * r1]
    lea        r2, [r2 + 2 * r3]

    dec        r4d
    jnz        .loop
    RET

;---------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_32x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------
%macro FILTER_V_PS_W32 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_%1x%2, 4, 6, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m1, m0, [tab_Vm]
    pshufb     m0, [tab_Vm + 16]

    mova       m7, [pw_2000]

    mov        r4d, %2

.loop
    movu       m2, [r0]
    movu       m3, [r0 + r1]

    punpcklbw  m4, m2, m3,
    punpckhbw  m2, m3,

    pmaddubsw  m4, m1
    pmaddubsw  m2, m1

    movu       m3, [r0 + 2 * r1]
    lea        r5, [r0 + 2 * r1]
    movu       m5, [r5 + r1]

    punpcklbw  m6, m3, m5
    punpckhbw  m3, m5,

    pmaddubsw  m6, m0
    pmaddubsw  m3, m0

    paddw      m4, m6
    paddw      m2, m3

    psubw      m4, m7
    psubw      m2, m7

    movu       [r2], m4
    movu       [r2 + 16], m2

    movu       m2, [r0 + 16]
    movu       m3, [r0 + r1 + 16]

    punpcklbw  m4, m2, m3,
    punpckhbw  m2, m3,

    pmaddubsw  m4, m1
    pmaddubsw  m2, m1

    movu       m3, [r0 + 2 * r1 + 16]
    movu       m5, [r5 + r1 + 16]

    punpcklbw  m6, m3, m5
    punpckhbw  m3, m5,

    pmaddubsw  m6, m0
    pmaddubsw  m3, m0

    paddw      m4, m6
    paddw      m2, m3

    psubw      m4, m7
    psubw      m2, m7

    movu       [r2 + 32], m4
    movu       [r2 + 48], m2

    lea        r0, [r0 + r1]
    lea        r2, [r2 + r3]

    dec        r4d
    jnz        .loop
    RET
%endmacro

FILTER_V_PS_W32 32,  8
FILTER_V_PS_W32 32, 16
FILTER_V_PS_W32 32, 24
FILTER_V_PS_W32 32, 32

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W8_H8_H16_H32 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_%1x%2, 4, 7, 8

mov         r4d,       r4m
sub         r0,        r1

%ifdef PIC
lea         r6,        [tab_ChromaCoeff]
movd        m5,        [r6 + r4 * 4]
%else
movd        m5,        [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m6,        m5,       [tab_Vm]
pshufb      m5,        [tab_Vm + 16]
mova        m4,        [tab_c_512]

mov         r4d,       %2

.loop
movq        m0,        [r0]
movq        m1,        [r0 + r1]
movq        m2,        [r0 + 2 * r1]
lea         r5,        [r0 + 2 * r1]
movq        m3,        [r5 + r1]

punpcklbw   m0,        m1
punpcklbw   m7,        m2,        m3

pmaddubsw   m0,        m6
pmaddubsw   m7,        m5

paddw       m0,        m7

pmulhrsw    m0,        m4
packuswb    m0,        m0
movh        [r2],      m0

movq        m0,        [r0 + 4 * r1]

punpcklbw   m1,        m2
punpcklbw   m7,        m3,        m0

pmaddubsw   m1,        m6
pmaddubsw   m7,        m5

paddw       m1,        m7

pmulhrsw    m1,        m4
packuswb    m1,        m1
movh        [r2 + r3], m1

lea         r6,        [r0 + 4 * r1]
movq        m1,        [r6 + r1]

punpcklbw   m2,        m3
punpcklbw   m7,        m0,        m1

pmaddubsw   m2,        m6
pmaddubsw   m7,        m5

paddw       m2,        m7

pmulhrsw    m2,        m4
packuswb    m2,        m2
movh        [r2 + 2 * r3], m2

movq        m2,        [r6 + 2 * r1]

punpcklbw   m3,        m0
punpcklbw   m1,        m2

pmaddubsw   m3,        m6
pmaddubsw   m1,        m5

paddw       m3,        m1

pmulhrsw    m3,        m4
packuswb    m3,        m3

lea         r5,        [r2 + 2 * r3]
movh        [r5 + r3], m3

lea         r0,        [r0 + 4 * r1]
lea         r2,        [r2 + 4 * r3]

sub         r4,         4
jnz        .loop
RET
%endmacro

FILTER_V4_W8_H8_H16_H32 8,  8
FILTER_V4_W8_H8_H16_H32 8, 16
FILTER_V4_W8_H8_H16_H32 8, 32

;-----------------------------------------------------------------------------
;void interp_4tap_vert_pp_6x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W6_H4 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_6x8, 4, 7, 8

mov         r4d,       r4m
sub         r0,        r1

%ifdef PIC
lea         r6,        [tab_ChromaCoeff]
movd        m5,        [r6 + r4 * 4]
%else
movd        m5,        [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m6,        m5,       [tab_Vm]
pshufb      m5,        [tab_Vm + 16]
mova        m4,        [tab_c_512]

mov         r4d,       %2

.loop
movq        m0,        [r0]
movq        m1,        [r0 + r1]
movq        m2,        [r0 + 2 * r1]
lea         r5,        [r0 + 2 * r1]
movq        m3,        [r5 + r1]

punpcklbw   m0,        m1
punpcklbw   m7,        m2,        m3

pmaddubsw   m0,        m6
pmaddubsw   m7,        m5

paddw       m0,        m7

pmulhrsw    m0,        m4
packuswb    m0,        m0
movd        [r2],      m0
pextrw      [r2 + 4],  m0,    2

movq        m0,        [r0 + 4 * r1]

punpcklbw   m1,        m2
punpcklbw   m7,        m3,        m0

pmaddubsw   m1,        m6
pmaddubsw   m7,        m5

paddw       m1,        m7

pmulhrsw    m1,        m4
packuswb    m1,        m1
movd        [r2 + r3],      m1
pextrw      [r2 + r3 + 4],  m1,    2

lea         r6,        [r0 + 4 * r1]
movq        m1,        [r6 + r1]

punpcklbw   m2,        m3
punpcklbw   m7,        m0,        m1

pmaddubsw   m2,        m6
pmaddubsw   m7,        m5

paddw       m2,        m7

pmulhrsw    m2,        m4
packuswb    m2,        m2
movd        [r2 + 2 * r3],     m2
pextrw      [r2 + 2 * r3 + 4], m2,    2

movq        m2,        [r6 + 2 * r1]

punpcklbw   m3,        m0
punpcklbw   m1,        m2

pmaddubsw   m3,        m6
pmaddubsw   m1,        m5

paddw       m3,        m1

pmulhrsw    m3,        m4
packuswb    m3,        m3

lea         r5,               [r2 + 2 * r3]
movd        [r5 + r3],        m3
pextrw      [r5 + r3 + 4],    m3,    2

lea         r0,        [r0 + 4 * r1]
lea         r2,        [r2 + 4 * r3]

sub         r4,         4
jnz        .loop
RET
%endmacro

FILTER_V4_W6_H4 6, 8

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_12x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W12_H2 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_12x16, 4, 6, 8

mov         r4d,       r4m
sub         r0,        r1

%ifdef PIC
lea         r5,        [tab_ChromaCoeff]
movd        m0,        [r5 + r4 * 4]
%else
movd        m0,        [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m1,        m0,       [tab_Vm]
pshufb      m0,        [tab_Vm + 16]

mova        m7,        [tab_c_512]

mov          r4d,       %2

.loop
movu        m2,        [r0]
movu        m3,        [r0 + r1]

punpcklbw   m4,        m2,        m3,
punpckhbw   m2,        m3,

pmaddubsw   m4,        m1
pmaddubsw   m2,        m1

movu        m5,        [r0 + 2 * r1]
lea         r5,        [r0 + 2 * r1]
movu        m3,        [r5 + r1]

punpcklbw   m6,        m5,        m3,
punpckhbw   m5,        m3,

pmaddubsw   m6,        m0
pmaddubsw   m5,        m0

paddw       m4,        m6
paddw       m2,        m5

pmulhrsw    m4,        m7
pmulhrsw    m2,        m7

packuswb    m4,        m2

movh         [r2],     m4
pextrd       [r2 + 8], m4,  2

movu        m2,        [r0 + r1]
movu        m3,        [r0 + 2 * r1]

punpcklbw   m4,        m2,        m3,
punpckhbw   m2,        m3,

pmaddubsw   m4,        m1
pmaddubsw   m2,        m1

lea         r5,        [r0 + 2 * r1]
movu        m5,        [r5 + r1]
movu        m3,        [r5 + 2 * r1]

punpcklbw   m6,        m5,        m3,
punpckhbw   m5,        m3,

pmaddubsw   m6,        m0
pmaddubsw   m5,        m0

paddw       m4,        m6
paddw       m2,        m5

pmulhrsw    m4,        m7
pmulhrsw    m2,        m7

packuswb    m4,        m2

movh        [r2 + r3],      m4
pextrd      [r2 + r3 + 8],  m4,  2

lea         r0,        [r0 + 2 * r1]
lea         r2,        [r2 + 2 * r3]

sub         r4,        2
jnz        .loop
RET
%endmacro

FILTER_V4_W12_H2 12, 16

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W16_H2 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_%1x%2, 4, 6, 8

mov         r4d,       r4m
sub         r0,        r1

%ifdef PIC
lea         r5,        [tab_ChromaCoeff]
movd        m0,        [r5 + r4 * 4]
%else
movd        m0,        [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m1,        m0,       [tab_Vm]
pshufb      m0,        [tab_Vm + 16]

mov         r4d,       %2

.loop
movu        m2,        [r0]
movu        m3,        [r0 + r1]

punpcklbw   m4,        m2,        m3,
punpckhbw   m5,        m2,        m3,

pmaddubsw   m4,        m1
pmaddubsw   m5,        m1

movu        m2,        [r0 + 2 * r1]
lea         r5,        [r0 + 2 * r1]
movu        m3,        [r5 + r1]

punpcklbw   m6,        m2,        m3,
punpckhbw   m7,        m2,        m3,

pmaddubsw   m6,        m0
pmaddubsw   m7,        m0

paddw       m4,        m6;
paddw       m5,        m7;

mova        m6,        [tab_c_512]

pmulhrsw    m4,        m6
pmulhrsw    m5,        m6

packuswb    m4,        m5

movu        [r2],      m4

movu        m2,        [r0 + r1]
movu        m3,        [r0 + 2 * r1]

punpcklbw   m4,        m2,        m3,
punpckhbw   m5,        m2,        m3,

pmaddubsw   m4,        m1
pmaddubsw   m5,        m1

lea         r5,        [r0 + 2 * r1]
movu        m2,        [r5 + r1]
movu        m3,        [r5 + 2 * r1]

punpcklbw   m6,        m2,        m3,
punpckhbw   m7,        m2,        m3,

pmaddubsw   m6,        m0
pmaddubsw   m7,        m0

paddw       m4,        m6
paddw       m5,        m7

mova        m6,        [tab_c_512]

pmulhrsw    m4,        m6
pmulhrsw    m5,        m6

packuswb    m4,        m5

movu        [r2 + r3],      m4

lea         r0,        [r0 + 2 * r1]
lea         r2,        [r2 + 2 * r3]

sub         r4,        2
jnz        .loop
RET
%endmacro

FILTER_V4_W16_H2 16,  4
FILTER_V4_W16_H2 16,  8
FILTER_V4_W16_H2 16, 12
FILTER_V4_W16_H2 16, 16
FILTER_V4_W16_H2 16, 32

;-----------------------------------------------------------------------------
;void interp_4tap_vert_pp_24x32(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W24 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_24x32, 4, 6, 8

mov         r4d,       r4m
sub         r0,        r1

%ifdef PIC
lea         r5,        [tab_ChromaCoeff]
movd        m0,        [r5 + r4 * 4]
%else
movd        m0,        [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m1,        m0,       [tab_Vm]
pshufb      m0,        [tab_Vm + 16]

mova        m7,        [tab_c_512]

mov         r4d,       %2

.loop
movu        m2,        [r0]
movu        m3,        [r0 + r1]

punpcklbw   m4,        m2,        m3,
punpckhbw   m2,        m3,

pmaddubsw   m4,        m1
pmaddubsw   m2,        m1

movu        m5,        [r0 + 2 * r1]
lea         r5,        [r0 + 2 * r1]
movu        m3,        [r5 + r1]

punpcklbw   m6,        m5,        m3,
punpckhbw   m5,        m3

pmaddubsw   m6,        m0
pmaddubsw   m5,        m0

paddw       m4,        m6
paddw       m2,        m5

pmulhrsw    m4,        m7
pmulhrsw    m2,        m7

packuswb    m4,        m2

movu        [r2],      m4

movq        m2,        [r0 + 16]
movq        m3,        [r0 + r1 + 16]
movq        m4,        [r0 + 2 * r1 + 16]
movq        m5,        [r5 + r1 + 16]

punpcklbw   m2,        m3
punpcklbw   m4,        m5

pmaddubsw   m2,        m1
pmaddubsw   m4,        m0

paddw       m2,        m4

pmulhrsw    m2,        m7
packuswb    m2,        m2
movh        [r2 + 16], m2

movu        m2,        [r0 + r1]
movu        m3,        [r0 + 2 * r1]

punpcklbw   m4,        m2,        m3,
punpckhbw   m2,        m3,

pmaddubsw   m4,        m1
pmaddubsw   m2,        m1

lea         r5,        [r0 + 2 * r1]
movu        m5,        [r5 +  r1]
movu        m3,        [r5 + 2 * r1]

punpcklbw   m6,        m5,        m3,
punpckhbw   m5,        m3

pmaddubsw   m6,        m0
pmaddubsw   m5,        m0

paddw       m4,        m6
paddw       m2,        m5

pmulhrsw    m4,        m7
pmulhrsw    m2,        m7

packuswb    m4,        m2

movu        [r2 + r3],      m4

movq        m2,        [r0 + r1 + 16]
movq        m3,        [r0 + 2 * r1 + 16]
movq        m4,        [r5 + r1 + 16]
movq        m5,        [r5 + 2 * r1 + 16]

punpcklbw   m2,        m3
punpcklbw   m4,        m5

pmaddubsw   m2,        m1
pmaddubsw   m4,        m0

paddw       m2,        m4

pmulhrsw    m2,        m7
packuswb    m2,        m2
movh        [r2 + r3 + 16], m2

lea         r0,        [r0 + 2 * r1]
lea         r2,        [r2 + 2 * r3]

sub         r4,        2
jnz        .loop
RET
%endmacro

FILTER_V4_W24 24, 32

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W32 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_%1x%2, 4, 6, 8

mov         r4d,       r4m
sub         r0,        r1

%ifdef PIC
lea         r5,        [tab_ChromaCoeff]
movd        m0,        [r5 + r4 * 4]
%else
movd        m0,        [tab_ChromaCoeff + r4 * 4]
%endif

pshufb      m1,        m0,       [tab_Vm]
pshufb      m0,        [tab_Vm + 16]

mova        m7,        [tab_c_512]

mov         r4d,       %2

.loop
movu        m2,        [r0]
movu        m3,        [r0 + r1]

punpcklbw   m4,        m2,        m3,
punpckhbw   m2,        m3,

pmaddubsw   m4,        m1
pmaddubsw   m2,        m1

movu        m3,        [r0 + 2 * r1]
lea         r5,        [r0 + 2 * r1]
movu        m5,        [r5 + r1]

punpcklbw   m6,        m3,        m5
punpckhbw   m3,        m5,

pmaddubsw   m6,        m0
pmaddubsw   m3,        m0

paddw       m4,        m6
paddw       m2,        m3

pmulhrsw    m4,        m7
pmulhrsw    m2,        m7

packuswb    m4,        m2

movu        [r2],      m4

movu        m2,        [r0 + 16]
movu        m3,        [r0 + r1 + 16]

punpcklbw   m4,        m2,        m3,
punpckhbw   m2,        m3,

pmaddubsw   m4,        m1
pmaddubsw   m2,        m1

movu        m3,        [r0 + 2 * r1 + 16]
movu        m5,        [r5 + r1 + 16]

punpcklbw   m6,        m3,        m5
punpckhbw   m3,        m5,

pmaddubsw   m6,        m0
pmaddubsw   m3,        m0

paddw       m4,        m6
paddw       m2,        m3

pmulhrsw    m4,        m7
pmulhrsw    m2,        m7

packuswb    m4,        m2

movu        [r2 + 16], m4

lea         r0,        [r0 + r1]
lea         r2,        [r2 + r3]

dec         r4
jnz        .loop
RET
%endmacro

FILTER_V4_W32 32,  8
FILTER_V4_W32 32, 16
FILTER_V4_W32 32, 24
FILTER_V4_W32 32, 32


;-----------------------------------------------------------------------------
; void filterConvertPelToShort(pixel *src, intptr_t srcStride, int16_t *dst, int width, int height)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal luma_p2s, 3, 7, 8

    ; load width and height
    mov         r3d, r3m
    mov         r4d, r4m

    ; load constant
    mova        m6, [tab_c_128]
    mova        m7, [tab_c_64_n64]

.loopH:

    xor         r5d, r5d
.loopW:
    lea         r6, [r0 + r5]

    movh        m0, [r6]
    punpcklbw   m0, m6
    pmaddubsw   m0, m7

    movh        m1, [r6 + r1]
    punpcklbw   m1, m6
    pmaddubsw   m1, m7

    movh        m2, [r6 + r1 * 2]
    punpcklbw   m2, m6
    pmaddubsw   m2, m7

    lea         r6, [r6 + r1 * 2]
    movh        m3, [r6 + r1]
    punpcklbw   m3, m6
    pmaddubsw   m3, m7

    add         r5, 8
    cmp         r5, r3
    jg          .width4
    movu        [r2 + r5 * 2 + FENC_STRIDE * 0 - 16], m0
    movu        [r2 + r5 * 2 + FENC_STRIDE * 2 - 16], m1
    movu        [r2 + r5 * 2 + FENC_STRIDE * 4 - 16], m2
    movu        [r2 + r5 * 2 + FENC_STRIDE * 6 - 16], m3
    je          .nextH
    jmp         .loopW

.width4:
    movh        [r2 + r5 * 2 + FENC_STRIDE * 0 - 16], m0
    movh        [r2 + r5 * 2 + FENC_STRIDE * 2 - 16], m1
    movh        [r2 + r5 * 2 + FENC_STRIDE * 4 - 16], m2
    movh        [r2 + r5 * 2 + FENC_STRIDE * 6 - 16], m3

.nextH:
    lea         r0, [r0 + r1 * 4]
    add         r2, FENC_STRIDE * 8

    sub         r4d, 4
    jnz         .loopH

    RET

%macro PROCESS_LUMA_W4_4R 0
    movd        m0, [r0]
    movd        m1, [r0 + r1]
    punpcklbw   m2, m0, m1                     ; m2=[0 1]

    movd        m0, [r0 + 2 * r1]
    punpcklbw   m1, m0                         ; m1=[1 2]
    punpcklqdq  m2, m1                         ; m2=[0 1 1 2]
    pmaddubsw   m4, m2, [r6 + 0 * 16]          ; m4=[0+1 1+2]

    lea         r0, [r0 + 2 * r1]
    movd        m1, [r0 + r1]
    punpcklbw   m5, m0, m1                     ; m2=[2 3]
    movd        m0, [r0 + 2 * r1]
    punpcklbw   m1, m0                         ; m1=[3 4]
    punpcklqdq  m5, m1                         ; m5=[2 3 3 4]
    pmaddubsw   m2, m5, [r6 + 1 * 16]          ; m2=[2+3 3+4]
    paddw       m4, m2                         ; m4=[0+1+2+3 1+2+3+4]      Row1-2
    pmaddubsw   m5, [r6 + 0 * 16]              ; m5=[2+3 3+4]            Row3-4

    lea         r0, [r0 + 2 * r1]
    movd        m1, [r0 + r1]
    punpcklbw   m2, m0, m1                     ; m2=[4 5]
    movd        m0, [r0 + 2 * r1]
    punpcklbw   m1, m0                         ; m1=[5 6]
    punpcklqdq  m2, m1                         ; m2=[4 5 5 6]
    pmaddubsw   m1, m2, [r6 + 2 * 16]          ; m1=[4+5 5+6]
    paddw       m4, m1                         ; m4=[0+1+2+3+4+5 1+2+3+4+5+6]      Row1-2
    pmaddubsw   m2, [r6 + 1 * 16]              ; m2=[4+5 5+6]
    paddw       m5, m2                         ; m5=[2+3+4+5 3+4+5+6]              Row3-4

    lea         r0, [r0 + 2 * r1]
    movd        m1, [r0 + r1]
    punpcklbw   m2, m0, m1                     ; m2=[6 7]
    movd        m0, [r0 + 2 * r1]
    punpcklbw   m1, m0                         ; m1=[7 8]
    punpcklqdq  m2, m1                         ; m2=[6 7 7 8]
    pmaddubsw   m1, m2, [r6 + 3 * 16]          ; m1=[6+7 7+8]
    paddw       m4, m1                         ; m4=[0+1+2+3+4+5+6+7 1+2+3+4+5+6+7+8]   Row1-2 end
    pmaddubsw   m2, [r6 + 2 * 16]              ; m2=[6+7 7+8]
    paddw       m5, m2                         ; m5=[2+3+4+5+6+7 3+4+5+6+7+8]           Row3-4

    lea         r0, [r0 + 2 * r1]
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
    pmaddubsw  m7, m0, [r6 + 0 *16]            ;m7=[0+1]  Row1

    movq       m0, [r0 + 2 * r1]
    punpcklbw  m1, m0
    pmaddubsw  m6, m1, [r6 + 0 *16]            ;m6=[1+2]  Row2

    lea        r0, [r0 + 2 * r1]
    movq       m1, [r0 + r1]
    punpcklbw  m0, m1
    pmaddubsw  m5, m0, [r6 + 0 *16]            ;m5=[2+3]  Row3
    pmaddubsw  m0, [r6 + 1 * 16]
    paddw      m7, m0                          ;m7=[0+1+2+3]  Row1

    movq       m0, [r0 + 2 * r1]
    punpcklbw  m1, m0
    pmaddubsw  m4, m1, [r6 + 0 *16]            ;m4=[3+4]  Row4
    pmaddubsw  m1, [r6 + 1 * 16]
    paddw      m6, m1                          ;m6 = [1+2+3+4]  Row2

    lea        r0, [r0 + 2 * r1]
    movq       m1, [r0 + r1]
    punpcklbw  m0, m1
    pmaddubsw  m2, m0, [r6 + 1 * 16]
    pmaddubsw  m0, [r6 + 2 * 16]
    paddw      m7, m0                          ;m7=[0+1+2+3+4+5]  Row1
    paddw      m5, m2                          ;m5=[2+3+4+5]  Row3

    movq       m0, [r0 + 2 * r1]
    punpcklbw  m1, m0
    pmaddubsw  m2, m1, [r6 + 1 * 16]
    pmaddubsw  m1, [r6 + 2 * 16]
    paddw      m6, m1                          ;m6=[1+2+3+4+5+6]  Row2
    paddw      m4, m2                          ;m4=[3+4+5+6]  Row4

    lea        r0, [r0 + 2 * r1]
    movq       m1, [r0 + r1]
    punpcklbw  m0, m1
    pmaddubsw  m2, m0, [r6 + 2 * 16]
    pmaddubsw  m0, [r6 + 3 * 16]
    paddw      m7, m0                          ;m7=[0+1+2+3+4+5+6+7]  Row1 end
    paddw      m5, m2                          ;m5=[2+3+4+5+6+7]  Row3

    movq       m0, [r0 + 2 * r1]
    punpcklbw  m1, m0
    pmaddubsw  m2, m1, [r6 + 2 * 16]
    pmaddubsw  m1, [r6 + 3 * 16]
    paddw      m6, m1                          ;m6=[1+2+3+4+5+6+7+8]  Row2 end
    paddw      m4, m2                          ;m4=[3+4+5+6+7+8]  Row4

    lea        r0, [r0 + 2 * r1]
    movq       m1, [r0 + r1]
    punpcklbw  m0, m1
    pmaddubsw  m0, [r6 + 3 * 16]
    paddw      m5, m0                          ;m5=[2+3+4+5+6+7+8+9]  Row3 end

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
    lea       r5, [r1 + 2 * r1]
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
    mova      m3, [tab_c_512]
%else
    mova      m3, [pw_2000]
%endif

    mov       r4d, %2/4

.loopH
    PROCESS_LUMA_W4_4R

%ifidn %3,pp
    pmulhrsw  m4, m3
    pmulhrsw  m5, m3

    packuswb  m4, m4
    packuswb  m5, m5

    movd      [r2], m4
    pshufd    m4, m4, 1
    movd      [r2 + r3], m4
    movd      [r2 + 2 * r3], m5
    pshufd    m5, m5, 1
    lea       r5, [r3 + 2 * r3]
    movd      [r2 + r5], m5
%else
    psubw     m4, m3
    psubw     m5, m3

    movlps    [r2], m4
    movhps    [r2 + r3], m4
    movlps    [r2 + 2 * r3], m5
    lea       r5, [r3 + 2 * r3]
    movhps    [r2 + r5], m5
%endif

    lea       r5, [4 * r1]
    sub       r0, r5
    lea       r2, [r2 + 4 * r3]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_4x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_4xN 4, 4, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_4x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_4xN 4, 8, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_4x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_4xN 4, 16, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_4x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_4xN 4, 4, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_4x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_4xN 4, 8, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_4x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_4xN 4, 16, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_%3_8x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_8xN 3
INIT_XMM sse4
cglobal interp_8tap_vert_%3_%1x%2, 5, 7, 8
    lea       r5, [r1 + 2 * r1]
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
    mova      m3, [tab_c_512]
%else
    mova      m3, [pw_2000]
%endif

    mov       r4d, %2/4

.loopH
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
    movlps    [r2 + 2 * r3], m5
    lea       r5, [r3 + 2 * r3]
    movhps    [r2 + r5], m5
%else
    psubw     m7, m3
    psubw     m6, m3
    psubw     m5, m3
    psubw     m4, m3

    movu      [r2], m7
    movu      [r2 + r3], m6
    movu      [r2 + 2 * r3], m5
    lea       r5, [r3 + 2 * r3]
    movu      [r2 + r5], m4
%endif

    lea       r5, [4 * r1]
    sub       r0, r5
    lea       r2, [r2 + 4 * r3]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_8x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_8xN 8, 4, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_8x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_8xN 8, 8, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_8x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_8xN 8, 16, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_8x32(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_8xN 8, 32, pp

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_8x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_8xN 8, 4, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_8x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_8xN 8, 8, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_8x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_8xN 8, 16, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_8x32(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
FILTER_VER_LUMA_8xN 8, 32, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_%3_12x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_12xN 3
INIT_XMM sse4
cglobal interp_8tap_vert_%3_%1x%2, 5, 7, 8
    lea       r5, [r1 + 2 * r1]
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
    mova      m3, [tab_c_512]
%else
    mova      m3, [pw_2000]
%endif

    mov       r4d, %2/4

.loopH
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
    movlps    [r2 + 2 * r3], m5
    lea       r5, [r3 + 2 * r3]
    movhps    [r2 + r5], m5
%else
    psubw     m7, m3
    psubw     m6, m3
    psubw     m5, m3
    psubw     m4, m3

    movu      [r2], m7
    movu      [r2 + r3], m6
    movu      [r2 + 2 * r3], m5
    lea       r5, [r3 + 2 * r3]
    movu      [r2 + r5], m4
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

    packuswb  m4, m4
    packuswb  m5, m5

    movd      [r2], m4
    pshufd    m4, m4, 1
    movd      [r2 + r3], m4
    movd      [r2 + 2 * r3], m5
    pshufd    m5, m5, 1
    lea       r5, [r3 + 2 * r3]
    movd      [r2 + r5], m5
%else
    psubw     m4, m3
    psubw     m5, m3

    movlps    [r2], m4
    movhps    [r2 + r3], m4
    movlps    [r2 + 2 * r3], m5
    lea       r5, [r3 + 2 * r3]
    movhps    [r2 + r5], m5
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

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA 3
INIT_XMM sse4
cglobal interp_8tap_vert_%3_%1x%2, 5, 7, 8 ,0-1
    lea       r5, [r1 + 2 * r1]
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
    mova      m3, [tab_c_512]
%else
    mova      m3, [pw_2000]
%endif
    mov       byte [rsp], %2/4

.loopH
    mov       r4d, (%1/8)
.loopW
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
    movlps    [r2 + 2 * r3], m5
    lea       r5, [r3 + 2 * r3]
    movhps    [r2 + r5], m5
%else
    psubw     m7, m3
    psubw     m6, m3
    psubw     m5, m3
    psubw     m4, m3

    movu      [r2], m7
    movu      [r2 + r3], m6
    movu      [r2 + 2 * r3], m5
    lea       r5, [r3 + 2 * r3]
    movu      [r2 + r5], m4
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

    dec       byte [rsp]
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

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m1, m4                          ;m1=[1 2]
    pmaddwd    m1, [r6 + 0 *16]                ;m1=[1+2]  Row2

    lea        r0, [r0 + 2 * r1]
    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[2 3]
    pmaddwd    m2, m4, [r6 + 0 *16]            ;m2=[2+3]  Row3
    pmaddwd    m4, [r6 + 1 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3]  Row1

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[3 4]
    pmaddwd    m3, m5, [r6 + 0 *16]            ;m3=[3+4]  Row4
    pmaddwd    m5, [r6 + 1 * 16]
    paddd      m1, m5                          ;m1 = [1+2+3+4]  Row2

    lea        r0, [r0 + 2 * r1]
    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[4 5]
    pmaddwd    m6, m4, [r6 + 1 * 16]
    paddd      m2, m6                          ;m2=[2+3+4+5]  Row3
    pmaddwd    m4, [r6 + 2 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3+4+5]  Row1

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[5 6]
    pmaddwd    m6, m5, [r6 + 1 * 16]
    paddd      m3, m6                          ;m3=[3+4+5+6]  Row4
    pmaddwd    m5, [r6 + 2 * 16]
    paddd      m1, m5                          ;m1=[1+2+3+4+5+6]  Row2

    lea        r0, [r0 + 2 * r1]
    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[6 7]
    pmaddwd    m6, m4, [r6 + 2 * 16]
    paddd      m2, m6                          ;m2=[2+3+4+5+6+7]  Row3
    pmaddwd    m4, [r6 + 3 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3+4+5+6+7]  Row1 end

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[7 8]
    pmaddwd    m6, m5, [r6 + 2 * 16]
    paddd      m3, m6                          ;m3=[3+4+5+6+7+8]  Row4
    pmaddwd    m5, [r6 + 3 * 16]
    paddd      m1, m5                          ;m1=[1+2+3+4+5+6+7+8]  Row2 end

    lea        r0, [r0 + 2 * r1]
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
INIT_XMM ssse3
cglobal interp_8tap_vert_sp_%1x%2, 5, 7, 8 ,0-1

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

    mova      m7, [tab_c_526336]

    mov       byte [rsp], %2/4
.loopH
    mov       r4d, (%1/4)
.loopW
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

    packuswb  m0, m0
    packuswb  m2, m2

    movd      [r2], m0
    pshufd    m0, m0, 1
    movd      [r2 + r3], m0
    movd      [r2 + 2 * r3], m2
    pshufd    m2, m2, 1
    lea       r5, [r3 + 2 * r3]
    movd      [r2 + r5], m2

    lea       r5, [8 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 4

    dec       r4d
    jnz       .loopW

    lea       r0, [r0 + 4 * r1 - 2 * %1]
    lea       r2, [r2 + 4 * r3 - %1]

    dec       byte [rsp]
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

; TODO: combin of U and V is more performance, but need more register
; TODO: use two path for height alignment to 4 and otherwise may improvement 10% performance, but code is more complex, so I disable it
INIT_XMM ssse3
cglobal chroma_p2s, 3, 7, 6

    ; load width and height
    mov         r3d, r3m
    mov         r4d, r4m

    ; load constant
    mova        m4, [tab_c_128]
    mova        m5, [tab_c_64_n64]

.loopH:

    xor         r5d, r5d
.loopW:
    lea         r6, [r0 + r5]

    movh        m0, [r6]
    punpcklbw   m0, m4
    pmaddubsw   m0, m5

    movh        m1, [r6 + r1]
    punpcklbw   m1, m4
    pmaddubsw   m1, m5

    add         r5d, 8
    cmp         r5d, r3d
    lea         r6, [r2 + r5 * 2]
    jg          .width4
    movu        [r6 + FENC_STRIDE / 2 * 0 - 16], m0
    movu        [r6 + FENC_STRIDE / 2 * 2 - 16], m1
    je          .nextH
    jmp         .loopW

.width4:
    test        r3d, 4
    jz          .width2
    test        r3d, 2
    movh        [r6 + FENC_STRIDE / 2 * 0 - 16], m0
    movh        [r6 + FENC_STRIDE / 2 * 2 - 16], m1
    lea         r6, [r6 + 8]
    pshufd      m0, m0, 2
    pshufd      m1, m1, 2
    jz          .nextH

.width2:
    movd        [r6 + FENC_STRIDE / 2 * 0 - 16], m0
    movd        [r6 + FENC_STRIDE / 2 * 2 - 16], m1

.nextH:
    lea         r0, [r0 + r1 * 2]
    add         r2, FENC_STRIDE / 2 * 4

    sub         r4d, 2
    jnz         .loopH

    RET


;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_v_ss(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int width, int height, const int coefIdx)
;-------------------------------------------------------------------------------------------------------------
INIT_XMM sse2

%if ARCH_X86_64
cglobal interp_8tap_v_ss, 4, 7+2, 8
%define tmp_r4d     r7d
%define tmp_r5d     r8d
%else
cglobal interp_8tap_v_ss, 4, 7, 8, 0-2*4
%define tmp_r4d     dword [rsp + 0*4]
%define tmp_r5d     dword [rsp + 1*4]
%endif

    ; load width, height and filterIdx
    mov         r4d, r4m
    mov         r5d, r5m
    mov         r6d, r6m

    ; convert to word stride
    add         r1, r1
    add         r3, r3

    ; stort to temporary memory or register
    shr         r4d, 2
    mov         tmp_r4d, r4d
    shr         r5d, 2
    mov         tmp_r5d, r5d

    shl         r6d, 6
%ifdef PIC
    lea         r5, [tab_LumaCoeffV]
    lea         r6, [r5 + r6]
%else
    lea         r6, [tab_LumaCoeffV + r6]
%endif

    lea         r4, [r1 * 3]
    sub         r0, r4

.loopH:
    ; load width
    mov         r4d, tmp_r4d

.loopW:

    movh        m0, [r0]                    ; m0 = [0]
    movh        m1, [r0 + r1]               ; m1 = [1]
    lea         r0, [r0 + r1 * 2]
    punpcklwd   m0, m1
    pmaddwd     m0, [r6 + 0 * 16]           ; m0 = [0+1]            = R0

    movh        m2, [r0]                    ; m2 = [2]
    movh        m3, [r0 + r1]               ; m3 = [3]
    lea         r0, [r0 + r1 * 2]
    punpcklwd   m1, m2
    pmaddwd     m1, [r6 + 0 * 16]           ; m1 = [1+2]            = R1
    punpcklwd   m2, m3                      ; m2 = [2 3]
    pmaddwd     m7, m2, [r6 + 1 * 16]       ;
    paddd       m0, m7                      ; m0 = [0+1+2+3]        = R0
    pmaddwd     m2, [r6 + 0 * 16]           ; m2 = [2+3]            = R2

    movh        m4, [r0]                    ; m4 = [4]
    movh        m5, [r0 + r1]               ; m5 = [5]
    lea         r0, [r0 + r1 * 2]
    punpcklwd   m3, m4                      ; m3 = [3 4]
    pmaddwd     m7, m3, [r6 + 1 * 16]
    paddd       m1, m7                      ; m1 = [1+2+3+4]        = R1
    pmaddwd     m3, [r6 + 0 * 16]           ; m3 = [3+4]            = R3
    punpcklwd   m4, m5                      ; m4 = [4 5]
    pmaddwd     m7, m4, [r6 + 2 * 16]
    paddd       m0, m7                      ; m0 = [0+1+2+3+4+5]    = R0
    pmaddwd     m4, [r6 + 1 * 16]
    paddd       m2, m4                      ; m2 = [2+3+4+5]        = R2

    movh        m6, [r0]                    ; m6 = [6]
    movh        m7, [r0 + r1]               ; m7 = [7]
    lea         r0, [r0 + r1 * 2]
    punpcklwd   m5, m6                      ; m5 = [5 6]
    pmaddwd     m4, m5, [r6 + 2 * 16]
    paddd       m1, m4                      ; m1 = [1+2+3+4+5+6]    = R1
    pmaddwd     m5, [r6 + 1 * 16]
    paddd       m3, m5                      ; m3 = [3+4+5+6]        = R3
    punpcklwd   m6, m7                      ; m6 = [6 7]
    pmaddwd     m4, m6, [r6 + 3 * 16]
    paddd       m0, m4                      ; m0 = [0+1+2+3+4+5+6+7]= R0
    pmaddwd     m6, [r6 + 2 * 16]
    paddd       m2, m6                      ; m2 = [2+3+4+5+6+7]    = R2
    psrad       m0, 6
    packssdw    m0, m0
    movh        [r2], m0                    ; store [0]

    movh        m4, [r0]                    ; m4 = [8]
    movh        m5, [r0 + r1]               ; m5 = [9]
    punpcklwd   m7, m4                      ; m7 = [7 8]
    pmaddwd     m6, m7, [r6 + 3 * 16]
    paddd       m1, m6                      ; m1 = [1+2+3+4+5+6+7+8]= R1
    pmaddwd     m7, [r6 + 2 * 16]
    paddd       m3, m7                      ; m3 = [3+4+5+6+7+8]    = R3
    psrad       m1, 6
    packssdw    m1, m1
    movh        [r2 + r3], m1               ; store [1]
    punpcklwd   m4, m5                      ; m4 = [8 9]
    pmaddwd     m4, [r6 + 3 * 16]
    paddd       m2, m4                      ; m2 = [2+3+4+5+6+7+8+9]= R2
    psrad       m2, 6
    packssdw    m2, m2
    movh        [r2 + r3 * 2], m2           ; store [2]
    lea         r2, [r2 + r3 * 2]

    movh        m4, [r0 + r1 * 2]           ; m4 = [10]
    punpcklwd   m5, m4                      ; m5 = [9 10]
    pmaddwd     m5, [r6 + 3 * 16]
    paddd       m3, m5                      ; m3 = [3+4+5+6+7+8+9+10]=R3
    psrad       m3, 6
    packssdw    m3, m3
    movh        [r2 + r3], m3               ; store [3]

    lea         r5, [r1 * 8 - 8]
    sub         r0, r5
    lea         r5, [r3 * 2 - 8]
    sub         r2, r5

    dec         r4d
    jnz         .loopW

    ; move to next row
    mov         r4d, tmp_r4d
    shl         r4d, 3
    lea         r0, [r0 + r1 * 4]
    sub         r0, r4
    lea         r2, [r2 + r3 * 4]
    sub         r2, r4

    dec         tmp_r5d
    jnz         .loopH

    RET

%macro PROCESS_CHROMA_SP_W4_4R 0
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r6 + 0 *16]                ;m0=[0+1]  Row1

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m1, m4                          ;m1=[1 2]
    pmaddwd    m1, [r6 + 0 *16]                ;m1=[1+2]  Row2

    lea        r0, [r0 + 2 * r1]
    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[2 3]
    pmaddwd    m2, m4, [r6 + 0 *16]            ;m2=[2+3]  Row3
    pmaddwd    m4, [r6 + 1 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3]  Row1 done

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[3 4]
    pmaddwd    m3, m5, [r6 + 0 *16]            ;m3=[3+4]  Row4
    pmaddwd    m5, [r6 + 1 * 16]
    paddd      m1, m5                          ;m1 = [1+2+3+4]  Row2

    lea        r0, [r0 + 2 * r1]
    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[4 5]
    pmaddwd    m4, [r6 + 1 * 16]
    paddd      m2, m4                          ;m2=[2+3+4+5]  Row3   

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[5 6]
    pmaddwd    m5, [r6 + 1 * 16]
    paddd      m3, m5                          ;m3=[3+4+5+6]  Row4
%endmacro

;--------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_sp_%1x%2(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SP 2
INIT_XMM ssse3
cglobal interp_4tap_vert_sp_%1x%2, 5, 7, 7 ,0-1

    add       r1d, r1d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_ChromaCoeffV + r4]
%endif

    mova      m6, [tab_c_526336]

    mov       byte [rsp], %2/4

.loopH
    mov       r4d, (%1/4)
.loopW
    PROCESS_CHROMA_SP_W4_4R

    paddd     m0, m6
    paddd     m1, m6
    paddd     m2, m6
    paddd     m3, m6

    psrad     m0, 12
    psrad     m1, 12
    psrad     m2, 12
    psrad     m3, 12

    packssdw  m0, m1
    packssdw  m2, m3

    packuswb  m0, m0
    packuswb  m2, m2

    movd      [r2], m0
    pshufd    m0, m0, 1
    movd      [r2 + r3], m0
    movd      [r2 + 2 * r3], m2
    pshufd    m2, m2, 1
    lea       r5, [r3 + 2 * r3]
    movd      [r2 + r5], m2

    lea       r5, [4 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 4

    dec       r4d
    jnz       .loopW

    lea       r0, [r0 + 4 * r1 - 2 * %1]
    lea       r2, [r2 + 4 * r3 - %1]

    dec       byte [rsp]
    jnz       .loopH

    RET
%endmacro

    FILTER_VER_CHROMA_SP 4, 4
    FILTER_VER_CHROMA_SP 4, 8
    FILTER_VER_CHROMA_SP 16, 16
    FILTER_VER_CHROMA_SP 16, 8
    FILTER_VER_CHROMA_SP 16, 12
    FILTER_VER_CHROMA_SP 12, 16
    FILTER_VER_CHROMA_SP 16, 4
    FILTER_VER_CHROMA_SP 4, 16
    FILTER_VER_CHROMA_SP 32, 32
    FILTER_VER_CHROMA_SP 32, 16
    FILTER_VER_CHROMA_SP 16, 32
    FILTER_VER_CHROMA_SP 32, 24
    FILTER_VER_CHROMA_SP 24, 32
    FILTER_VER_CHROMA_SP 32, 8


%macro PROCESS_CHROMA_SP_W2_4R 0
    movd       m0, [r0]
    movd       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]

    movd       m2, [r0 + 2 * r1]
    punpcklwd  m1, m2                          ;m1=[1 2]
    punpcklqdq m0, m1                          ;m0=[0 1 1 2]
    pmaddwd    m0, [r6 + 0 *16]                ;m0=[0+1 1+2] Row 1-2

    lea        r0, [r0 + 2 * r1]
    movd       m1, [r0 + r1]
    punpcklwd  m2, m1                          ;m2=[2 3]

    movd       m3, [r0 + 2 * r1]
    punpcklwd  m1, m3                          ;m2=[3 4]
    punpcklqdq m2, m1                          ;m2=[2 3 3 4]

    pmaddwd    m4, m2, [r6 + 1 * 16]           ;m4=[2+3 3+4] Row 1-2
    pmaddwd    m2, [r6 + 0 * 16]               ;m2=[2+3 3+4] Row 3-4
    paddd      m0, m4                          ;m0=[0+1+2+3 1+2+3+4] Row 1-2

    lea        r0, [r0 + 2 * r1]
    movd       m1, [r0 + r1]
    punpcklwd  m3, m1                          ;m3=[4 5]

    movd       m4, [r0 + 2 * r1]
    punpcklwd  m1, m4                          ;m1=[5 6]
    punpcklqdq m3, m1                          ;m2=[4 5 5 6]
    pmaddwd    m3, [r6 + 1 * 16]               ;m3=[4+5 5+6] Row 3-4
    paddd      m2, m3                          ;m2=[2+3+4+5 3+4+5+6] Row 3-4
%endmacro

;-------------------------------------------------------------------------------------------------------------------
; void interp_4tap_vertical_sp_%1x%2(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SP_W2_4R 2
INIT_XMM sse4
cglobal interp_4tap_vert_sp_%1x%2, 5, 7, 6

    add       r1d, r1d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_ChromaCoeffV + r4]
%endif

    mova      m5, [tab_c_526336]

    mov       r4d, (%2/4)

.loopH
    PROCESS_CHROMA_SP_W2_4R

    paddd     m0, m5
    paddd     m2, m5

    psrad     m0, 12
    psrad     m2, 12

    packssdw  m0, m2
    packuswb  m0, m0

    pextrw    [r2], m0, 0
    pextrw    [r2 + r3], m0, 1
    pextrw    [r2 + 2 * r3], m0, 2
    lea       r2, [r2 + 2 * r3]
    pextrw    [r2 + r3], m0, 3

    lea       r2, [r2 + 2 * r3]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

FILTER_VER_CHROMA_SP_W2_4R 2, 4
FILTER_VER_CHROMA_SP_W2_4R 2, 8

;--------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_sp_4x2(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
INIT_XMM ssse3
cglobal interp_4tap_vert_sp_4x2, 5, 6, 5

    add        r1d, r1d
    sub        r0, r1
    shl        r4d, 5

%ifdef PIC
    lea        r5, [tab_ChromaCoeffV]
    lea        r5, [r5 + r4]
%else
    lea        r5, [tab_ChromaCoeffV + r4]
%endif

    mova       m4, [tab_c_526336]

    movq       m0, [r0]
    movq       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r5 + 0 *16]                ;m0=[0+1]  Row1

    movq       m2, [r0 + 2 * r1]
    punpcklwd  m1, m2                          ;m1=[1 2]
    pmaddwd    m1, [r5 + 0 *16]                ;m1=[1+2]  Row2

    lea        r0, [r0 + 2 * r1]
    movq       m3, [r0 + r1]
    punpcklwd  m2, m3                          ;m4=[2 3]
    pmaddwd    m2, [r5 + 1 * 16]
    paddd      m0, m2                          ;m0=[0+1+2+3]  Row1 done
    paddd      m0, m4
    psrad      m0, 12

    movq       m2, [r0 + 2 * r1]
    punpcklwd  m3, m2                          ;m5=[3 4]
    pmaddwd    m3, [r5 + 1 * 16]
    paddd      m1, m3                          ;m1 = [1+2+3+4]  Row2 done
    paddd      m1, m4
    psrad      m1, 12

    packssdw   m0, m1
    packuswb   m0, m0

    movd       [r2], m0
    pshufd     m0, m0, 1
    movd       [r2 + r3], m0

    RET

;-------------------------------------------------------------------------------------------------------------------
; void interp_4tap_vertical_sp_6x8(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_sp_6x8, 5, 7, 7

    add       r1d, r1d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_ChromaCoeffV + r4]
%endif

    mova      m6, [tab_c_526336]

    mov       r4d, 8/4

.loopH
    PROCESS_CHROMA_SP_W4_4R

    paddd     m0, m6
    paddd     m1, m6
    paddd     m2, m6
    paddd     m3, m6

    psrad     m0, 12
    psrad     m1, 12
    psrad     m2, 12
    psrad     m3, 12

    packssdw  m0, m1
    packssdw  m2, m3

    packuswb  m0, m0
    packuswb  m2, m2

    movd      [r2], m0
    pshufd    m0, m0, 1
    movd      [r2 + r3], m0
    movd      [r2 + 2 * r3], m2
    pshufd    m2, m2, 1
    lea       r5, [r3 + 2 * r3]
    movd      [r2 + r5], m2

    lea       r5, [4 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 4

    PROCESS_CHROMA_SP_W2_4R

    paddd     m0, m6
    paddd     m2, m6

    psrad     m0, 12
    psrad     m2, 12

    packssdw  m0, m2
    packuswb  m0, m0

    pextrw    [r2], m0, 0
    pextrw    [r2 + r3], m0, 1
    pextrw    [r2 + 2 * r3], m0, 2
    lea       r2, [r2 + 2 * r3]
    pextrw    [r2 + r3], m0, 3

    sub       r0, 2 * 4
    lea       r2, [r2 + 2 * r3 - 4]

    dec       r4d
    jnz       .loopH

    RET

%macro PROCESS_CHROMA_SP_W8_2R 0
    movu       m1, [r0]
    movu       m3, [r0 + r1]
    punpcklwd  m0, m1, m3
    pmaddwd    m0, [r5 + 0 * 16]                ;m0 = [0l+1l]  Row1l
    punpckhwd  m1, m3
    pmaddwd    m1, [r5 + 0 * 16]                ;m1 = [0h+1h]  Row1h

    movu       m4, [r0 + 2 * r1]
    punpcklwd  m2, m3, m4
    pmaddwd    m2, [r5 + 0 * 16]                ;m2 = [1l+2l]  Row2l
    punpckhwd  m3, m4
    pmaddwd    m3, [r5 + 0 * 16]                ;m3 = [1h+2h]  Row2h

    lea        r0, [r0 + 2 * r1]
    movu       m5, [r0 + r1]
    punpcklwd  m6, m4, m5
    pmaddwd    m6, [r5 + 1 * 16]                ;m6 = [2l+3l]  Row1l
    paddd      m0, m6                           ;m0 = [0l+1l+2l+3l]  Row1l sum
    punpckhwd  m4, m5
    pmaddwd    m4, [r5 + 1 * 16]                ;m6 = [2h+3h]  Row1h
    paddd      m1, m4                           ;m1 = [0h+1h+2h+3h]  Row1h sum

    movu       m4, [r0 + 2 * r1]
    punpcklwd  m6, m5, m4
    pmaddwd    m6, [r5 + 1 * 16]                ;m6 = [3l+4l]  Row2l
    paddd      m2, m6                           ;m2 = [1l+2l+3l+4l]  Row2l sum
    punpckhwd  m5, m4
    pmaddwd    m5, [r5 + 1 * 16]                ;m1 = [3h+4h]  Row2h
    paddd      m3, m5                           ;m3 = [1h+2h+3h+4h]  Row2h sum
%endmacro

;--------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_sp_8x%2(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SP_W8_H2 2
INIT_XMM ssse3
cglobal interp_4tap_vert_sp_%1x%2, 5, 6, 8

    add       r1d, r1d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif

    mova      m7, [tab_c_526336]

    mov       r4d, %2/2
.loopH
    PROCESS_CHROMA_SP_W8_2R

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

    movlps    [r2], m0
    movhps    [r2 + r3], m0

    lea       r2, [r2 + 2 * r3]

    dec r4d
    jnz .loopH

    RET
%endmacro

FILTER_VER_CHROMA_SP_W8_H2 8, 2
FILTER_VER_CHROMA_SP_W8_H2 8, 4
FILTER_VER_CHROMA_SP_W8_H2 8, 6
FILTER_VER_CHROMA_SP_W8_H2 8, 8
FILTER_VER_CHROMA_SP_W8_H2 8, 16
FILTER_VER_CHROMA_SP_W8_H2 8, 32

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
    lea        r6, [tab_ChromaCoeff]
    movd       coef2, [r6 + r4 * 4]
%else
    movd       coef2, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufd     coef2, coef2, 0
    mova       t1, [pw_2000]
    mova       Tm0, [tab_Tm]

    mov        r4d, %2
    cmp        r5m, byte 0
    je         .loopH
    sub        srcq, srcstrideq
    add        r4d, 3

.loopH
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
    lea        r6, [tab_ChromaCoeff]
    movd       coef2, [r6 + r4 * 4]
%else
    movd       coef2, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufd     coef2, coef2, 0
    mova       t1, [pw_2000]
    mova       Tm0, [tab_Tm]

    mov        r4d, %2
    cmp        r5m, byte 0
    je         .loopH
    sub        srcq, srcstrideq
    add        r4d, 3

.loopH
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
    lea     r6, [tab_ChromaCoeff]
    movd    coef2, [r6 + r4 * 4]
%else
    movd    coef2, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufd  coef2, coef2, 0
    mova    t2, [pw_2000]
    mova    Tm0, [tab_Tm]
    mova    Tm1, [tab_Tm + 16]

    mov     r4d, %2
    cmp     r5m, byte 0
    je      .loopH
    sub     srcq, srcstrideq
    add     r4d, 3

.loopH
    PROCESS_CHROMA_W%1  t0, t1, t2
    add     srcq, srcstrideq
    add     dstq, dststrideq

    dec     r4d
    jnz     .loopH

    RET
%endmacro

FILTER_HORIZ_CHROMA 6, 8
FILTER_HORIZ_CHROMA 12, 16

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
    lea     r6, [tab_ChromaCoeff]
    movd    coef2, [r6 + r4 * 4]
%else
    movd    coef2, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufd  coef2, coef2, 0
    mova    t2, [pw_2000]
    mova    Tm0, [tab_Tm]
    mova    Tm1, [tab_Tm + 16]

    mov     r4d, %2
    cmp     r5m, byte 0
    je      .loopH
    sub     srcq, srcstrideq
    add     r4d, 3

.loopH
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
    lea     r6, [tab_ChromaCoeff]
    movd    coef2, [r6 + r4 * 4]
%else
    movd    coef2, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufd  coef2, coef2, 0
    mova    t2, [pw_2000]
    mova    Tm0, [tab_Tm]
    mova    Tm1, [tab_Tm + 16]

    mov     r4d, %2
    cmp     r5m, byte 0
    je      .loopH
    sub     srcq, srcstrideq
    add     r4d, 3

.loopH
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

;------------------------------------------------------------------------------------------------------------
;void interp_4tap_vert_ps_2x4(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_ps_2x4, 4, 7, 8

    mov         r4d, r4m
    sub         r0, r1
    add         r3d, r3d

%ifdef PIC
    lea         r5, [tab_ChromaCoeff]
    movd        m0, [r5 + r4 * 4]
%else
    movd        m0, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m0, [tab_Cm]

    mova        m1, [pw_2000]

    movd        m2, [r0]
    movd        m3, [r0 + r1]
    movd        m4, [r0 + 2 * r1]
    lea         r5, [r0 + 2 * r1]
    movd        m5, [r5 + r1]

    punpcklbw   m2, m3
    punpcklbw   m6, m4, m5
    punpcklbw   m2, m6

    pmaddubsw   m2, m0

    movd        m6, [r0 + 4 * r1]

    punpcklbw   m3, m4
    punpcklbw   m7, m5, m6
    punpcklbw   m3, m7

    pmaddubsw   m3, m0
    phaddw      m2, m3
    psubw       m2, m1

    movd        [r2], m2
    pshufd      m2, m2 , 2
    movd        [r2 + r3], m2

    lea         r5, [r0 + 4 * r1]
    movd        m2, [r5 + r1]

    punpcklbw   m4, m5
    punpcklbw   m3, m6, m2
    punpcklbw   m4, m3

    pmaddubsw   m4, m0

    movd        m3, [r5 + 2 * r1]

    punpcklbw   m5, m6
    punpcklbw   m2, m3
    punpcklbw   m5, m2

    pmaddubsw   m5, m0
    phaddw      m4, m5
    psubw       m4, m1

    movd        [r2 + 2 * r3], m4
    pshufd      m4, m4, 2
    lea         r6, [r2 + 2 * r3]
    movd        [r6 + r3], m4

    RET

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_2x8(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_ps_2x8, 4, 7, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m0, [tab_Cm]

    mova       m1, [pw_2000]

    mov        r4d, 2
.loop
    movd       m2, [r0]
    movd       m3, [r0 + r1]
    movd       m4, [r0 + 2 * r1]
    lea        r5, [r0 + 2 * r1]
    movd       m5, [r5 + r1]

    punpcklbw  m2, m3
    punpcklbw  m6, m4, m5
    punpcklbw  m2, m6

    pmaddubsw  m2, m0

    movd       m6, [r0 + 4 * r1]

    punpcklbw  m3, m4
    punpcklbw  m7, m5, m6
    punpcklbw  m3, m7

    pmaddubsw  m3, m0

    phaddw     m2, m3
    psubw      m2, m1


    movd       [r2], m2
    pshufd     m2, m2, 2
    movd       [r2 + r3], m2

    lea        r5, [r0 + 4 * r1]
    movd       m2, [r5 + r1]

    punpcklbw  m4, m5
    punpcklbw  m3, m6, m2
    punpcklbw  m4, m3

    pmaddubsw  m4, m0

    movd       m3, [r5 + 2 * r1]

    punpcklbw  m5, m6
    punpcklbw  m2, m3
    punpcklbw  m5, m2

    pmaddubsw  m5, m0

    phaddw     m4, m5

    psubw      m4, m1

    movd       [r2 + 2 * r3], m4
    lea        r6, [r2 + 2 * r3]
    pshufd     m4 , m4 ,2
    movd       [r6 + r3], m4

    lea        r0, [r0 + 4 * r1]
    lea        r2, [r2 + 4 * r3]

    dec        r4d
    jnz        .loop

RET

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ss_%1x%2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SS 2
INIT_XMM sse2
cglobal interp_4tap_vert_ss_%1x%2, 5, 7, 6 ,0-1

    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_ChromaCoeffV + r4]
%endif

    mov       byte [rsp], %2/4

.loopH
    mov       r4d, (%1/4)
.loopW
    PROCESS_CHROMA_SP_W4_4R

    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3

    movlps    [r2], m0
    movhps    [r2 + r3], m0
    movlps    [r2 + 2 * r3], m2
    lea       r5, [r3 + 2 * r3]
    movhps    [r2 + r5], m2

    lea       r5, [4 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 2 * 4

    dec       r4d
    jnz       .loopW

    lea       r0, [r0 + 4 * r1 - 2 * %1]
    lea       r2, [r2 + 4 * r3 - 2 * %1]

    dec       byte [rsp]
    jnz       .loopH

    RET
%endmacro

    FILTER_VER_CHROMA_SS 4, 4
    FILTER_VER_CHROMA_SS 4, 8
    FILTER_VER_CHROMA_SS 16, 16
    FILTER_VER_CHROMA_SS 16, 8
    FILTER_VER_CHROMA_SS 16, 12
    FILTER_VER_CHROMA_SS 12, 16
    FILTER_VER_CHROMA_SS 16, 4
    FILTER_VER_CHROMA_SS 4, 16
    FILTER_VER_CHROMA_SS 32, 32
    FILTER_VER_CHROMA_SS 32, 16
    FILTER_VER_CHROMA_SS 16, 32
    FILTER_VER_CHROMA_SS 32, 24
    FILTER_VER_CHROMA_SS 24, 32
    FILTER_VER_CHROMA_SS 32, 8

;---------------------------------------------------------------------------------------------------------------------
; void interp_4tap_vertical_ss_%1x%2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SS_W2_4R 2
INIT_XMM sse2
cglobal interp_4tap_vert_ss_%1x%2, 5, 7, 5

    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_ChromaCoeffV + r4]
%endif

    mov       r4d, (%2/4)

.loopH
    PROCESS_CHROMA_SP_W2_4R

    psrad     m0, 6
    psrad     m2, 6

    packssdw  m0, m0
    packssdw  m2, m2

    movd      [r2], m0
    pshufd    m0, m0, 1
    movd      [r2 + r3], m0
    lea       r2, [r2 + 2 * r3]
    movd      [r2], m2
    pshufd    m2, m2, 1
    movd      [r2 + r3], m2

    lea       r2, [r2 + 2 * r3]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

FILTER_VER_CHROMA_SS_W2_4R 2, 4
FILTER_VER_CHROMA_SS_W2_4R 2, 8

;---------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ss_4x2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------
INIT_XMM sse2
cglobal interp_4tap_vert_ss_4x2, 5, 6, 4

    add        r1d, r1d
    add        r3d, r3d
    sub        r0, r1
    shl        r4d, 5

%ifdef PIC
    lea        r5, [tab_ChromaCoeffV]
    lea        r5, [r5 + r4]
%else
    lea        r5, [tab_ChromaCoeffV + r4]
%endif

    movq       m0, [r0]
    movq       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r5 + 0 *16]                ;m0=[0+1]  Row1

    movq       m2, [r0 + 2 * r1]
    punpcklwd  m1, m2                          ;m1=[1 2]
    pmaddwd    m1, [r5 + 0 *16]                ;m1=[1+2]  Row2

    lea        r0, [r0 + 2 * r1]
    movq       m3, [r0 + r1]
    punpcklwd  m2, m3                          ;m4=[2 3]
    pmaddwd    m2, [r5 + 1 * 16]
    paddd      m0, m2                          ;m0=[0+1+2+3]  Row1 done
    psrad      m0, 6

    movq       m2, [r0 + 2 * r1]
    punpcklwd  m3, m2                          ;m5=[3 4]
    pmaddwd    m3, [r5 + 1 * 16]
    paddd      m1, m3                          ;m1=[1+2+3+4]  Row2 done
    psrad      m1, 6

    packssdw   m0, m1

    movlps     [r2], m0
    movhps     [r2 + r3], m0

    RET

;-------------------------------------------------------------------------------------------------------------------
; void interp_4tap_vertical_ss_6x8(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------------
INIT_XMM sse2
cglobal interp_4tap_vert_ss_6x8, 5, 7, 6

    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_ChromaCoeffV + r4]
%endif

    mov       r4d, 8/4

.loopH
    PROCESS_CHROMA_SP_W4_4R

    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3

    movlps    [r2], m0
    movhps    [r2 + r3], m0
    movlps    [r2 + 2 * r3], m2
    lea       r5, [r3 + 2 * r3]
    movhps    [r2 + r5], m2

    lea       r5, [4 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 2 * 4

    PROCESS_CHROMA_SP_W2_4R

    psrad     m0, 6
    psrad     m2, 6

    packssdw  m0, m0
    packssdw  m2, m2

    movd      [r2], m0
    pshufd    m0, m0, 1
    movd      [r2 + r3], m0
    lea       r2, [r2 + 2 * r3]
    movd      [r2], m2
    pshufd    m2, m2, 1
    movd      [r2 + r3], m2

    sub       r0, 2 * 4
    lea       r2, [r2 + 2 * r3 - 2 * 4]

    dec       r4d
    jnz       .loopH

    RET

;----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ss_8x%2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SS_W8_H2 2
INIT_XMM sse2
cglobal interp_4tap_vert_ss_%1x%2, 5, 6, 7

    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif

    mov       r4d, %2/2
.loopH
    PROCESS_CHROMA_SP_W8_2R

    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3

    movu      [r2], m0
    movu      [r2 + r3], m2

    lea       r2, [r2 + 2 * r3]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

FILTER_VER_CHROMA_SS_W8_H2 8, 2
FILTER_VER_CHROMA_SS_W8_H2 8, 4
FILTER_VER_CHROMA_SS_W8_H2 8, 6
FILTER_VER_CHROMA_SS_W8_H2 8, 8
FILTER_VER_CHROMA_SS_W8_H2 8, 16
FILTER_VER_CHROMA_SS_W8_H2 8, 32

;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ss_%1x%2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_SS 2
INIT_XMM sse2
cglobal interp_8tap_vert_ss_%1x%2, 5, 7, 7 ,0-1

    add        r1d, r1d
    add        r3d, r3d
    lea        r5, [r1 + 2 * r1]
    sub        r0, r5
    shl        r4d, 6

%ifdef PIC
    lea        r5, [tab_LumaCoeffV]
    lea        r6, [r5 + r4]
%else
    lea        r6, [tab_LumaCoeffV + r4]
%endif

    mov        byte [rsp], %2/4
.loopH
    mov        r4d, (%1/4)
.loopW
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r6 + 0 *16]                ;m0=[0+1]  Row1

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m1, m4                          ;m1=[1 2]
    pmaddwd    m1, [r6 + 0 *16]                ;m1=[1+2]  Row2

    lea        r0, [r0 + 2 * r1]
    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[2 3]
    pmaddwd    m2, m4, [r6 + 0 *16]            ;m2=[2+3]  Row3
    pmaddwd    m4, [r6 + 1 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3]  Row1

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[3 4]
    pmaddwd    m3, m5, [r6 + 0 *16]            ;m3=[3+4]  Row4
    pmaddwd    m5, [r6 + 1 * 16]
    paddd      m1, m5                          ;m1 = [1+2+3+4]  Row2

    lea        r0, [r0 + 2 * r1]
    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[4 5]
    pmaddwd    m6, m4, [r6 + 1 * 16]
    paddd      m2, m6                          ;m2=[2+3+4+5]  Row3
    pmaddwd    m4, [r6 + 2 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3+4+5]  Row1

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[5 6]
    pmaddwd    m6, m5, [r6 + 1 * 16]
    paddd      m3, m6                          ;m3=[3+4+5+6]  Row4
    pmaddwd    m5, [r6 + 2 * 16]
    paddd      m1, m5                          ;m1=[1+2+3+4+5+6]  Row2

    lea        r0, [r0 + 2 * r1]
    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[6 7]
    pmaddwd    m6, m4, [r6 + 2 * 16]
    paddd      m2, m6                          ;m2=[2+3+4+5+6+7]  Row3
    pmaddwd    m4, [r6 + 3 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3+4+5+6+7]  Row1 end
    psrad      m0, 6

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[7 8]
    pmaddwd    m6, m5, [r6 + 2 * 16]
    paddd      m3, m6                          ;m3=[3+4+5+6+7+8]  Row4
    pmaddwd    m5, [r6 + 3 * 16]
    paddd      m1, m5                          ;m1=[1+2+3+4+5+6+7+8]  Row2 end
    psrad      m1, 6

    packssdw   m0, m1

    movlps     [r2], m0
    movhps     [r2 + r3], m0

    lea        r0, [r0 + 2 * r1]
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
    lea        r5, [r3 + 2 * r3]
    movhps     [r2 + r5], m2

    lea        r5, [8 * r1 - 2 * 4]
    sub        r0, r5
    add        r2, 2 * 4

    dec        r4d
    jnz        .loopW

    lea        r0, [r0 + 4 * r1 - 2 * %1]
    lea        r2, [r2 + 4 * r3 - 2 * %1]

    dec        byte [rsp]
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
