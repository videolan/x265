;*****************************************************************************
;* Copyright (C) 2013-2017 MulticoreWare, Inc
;*
;* Authors: Nabajit Deka <nabajit@multicorewareinc.com>
;*          Murugan Vairavel <murugan@multicorewareinc.com>
;*          Min Chen <chenm003@163.com>
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


%define INTERP_OFFSET_PP        pd_32
%define INTERP_SHIFT_PP         6

%if BIT_DEPTH == 10
    %define INTERP_SHIFT_PS         2
    %define INTERP_OFFSET_PS        pd_n32768
    %define INTERP_SHIFT_SP         10
    %define INTERP_OFFSET_SP        h4_pd_524800
%elif BIT_DEPTH == 12
    %define INTERP_SHIFT_PS         4
    %define INTERP_OFFSET_PS        pd_n131072
    %define INTERP_SHIFT_SP         8
    %define INTERP_OFFSET_SP        pd_524416
%else
    %error Unsupport bit depth!
%endif


SECTION_RODATA 32

tab_c_32:         times 8 dd 32
h4_pd_524800:        times 8 dd 524800

tab_Tm16:         db 0, 1, 2, 3, 4,  5,  6, 7, 2, 3, 4,  5, 6, 7, 8, 9

h4_tab_ChromaCoeff:  dw  0, 64,  0,  0
                  dw -2, 58, 10, -2
                  dw -4, 54, 16, -2
                  dw -6, 46, 28, -4
                  dw -4, 36, 36, -4
                  dw -4, 28, 46, -6
                  dw -2, 16, 54, -4
                  dw -2, 10, 58, -2

const h4_interp8_hpp_shuf,     db 0, 1, 2, 3, 4, 5, 6, 7, 2, 3, 4, 5, 6, 7, 8, 9
                            db 4, 5, 6, 7, 8, 9, 10, 11, 6, 7, 8, 9, 10, 11, 12, 13

SECTION .text
cextern pd_8
cextern pd_32
cextern pw_pixel_max
cextern pd_524416
cextern pd_n32768
cextern pd_n131072
cextern pw_2000
cextern idct8_shuf2

%macro FILTERH_W2_4_sse3 2
    movh        m3,     [r0 + %1]
    movhps      m3,     [r0 + %1 + 2]
    pmaddwd     m3,     m0
    movh        m4,     [r0 + r1 + %1]
    movhps      m4,     [r0 + r1 + %1 + 2]
    pmaddwd     m4,     m0
    pshufd      m2,     m3,     q2301
    paddd       m3,     m2
    pshufd      m2,     m4,     q2301
    paddd       m4,     m2
    pshufd      m3,     m3,     q3120
    pshufd      m4,     m4,     q3120
    punpcklqdq  m3,     m4
    paddd       m3,     m1
    movh        m5,     [r0 + 2 * r1 + %1]
    movhps      m5,     [r0 + 2 * r1 + %1 + 2]
    pmaddwd     m5,     m0
    movh        m4,     [r0 + r4 + %1]
    movhps      m4,     [r0 + r4 + %1 + 2]
    pmaddwd     m4,     m0
    pshufd      m2,     m5,     q2301
    paddd       m5,     m2
    pshufd      m2,     m4,     q2301
    paddd       m4,     m2
    pshufd      m5,     m5,     q3120
    pshufd      m4,     m4,     q3120
    punpcklqdq  m5,     m4
    paddd       m5,     m1
%ifidn %2, pp
    psrad       m3,     6
    psrad       m5,     6
    packssdw    m3,     m5
    CLIPW       m3,     m7,     m6
%else
    psrad       m3,     INTERP_SHIFT_PS
    psrad       m5,     INTERP_SHIFT_PS
    packssdw    m3,     m5
%endif
    movd        [r2 + %1], m3
    psrldq      m3,     4
    movd        [r2 + r3 + %1], m3
    psrldq      m3,     4
    movd        [r2 + r3 * 2 + %1], m3
    psrldq      m3,     4
    movd        [r2 + r5 + %1], m3
%endmacro

%macro FILTERH_W2_3_sse3 1
    movh        m3,     [r0 + %1]
    movhps      m3,     [r0 + %1 + 2]
    pmaddwd     m3,     m0
    movh        m4,     [r0 + r1 + %1]
    movhps      m4,     [r0 + r1 + %1 + 2]
    pmaddwd     m4,     m0
    pshufd      m2,     m3,     q2301
    paddd       m3,     m2
    pshufd      m2,     m4,     q2301
    paddd       m4,     m2
    pshufd      m3,     m3,     q3120
    pshufd      m4,     m4,     q3120
    punpcklqdq  m3,     m4
    paddd       m3,     m1

    movh        m5,     [r0 + 2 * r1 + %1]
    movhps      m5,     [r0 + 2 * r1 + %1 + 2]
    pmaddwd     m5,     m0

    pshufd      m2,     m5,     q2301
    paddd       m5,     m2
    pshufd      m5,     m5,     q3120
    paddd       m5,     m1

    psrad       m3,     INTERP_SHIFT_PS
    psrad       m5,     INTERP_SHIFT_PS
    packssdw    m3,     m5

    movd        [r2 + %1], m3
    psrldq      m3,     4
    movd        [r2 + r3 + %1], m3
    psrldq      m3,     4
    movd        [r2 + r3 * 2 + %1], m3
%endmacro

%macro FILTERH_W4_2_sse3 2
    movh        m3,     [r0 + %1]
    movhps      m3,     [r0 + %1 + 2]
    pmaddwd     m3,     m0
    movh        m4,     [r0 + %1 + 4]
    movhps      m4,     [r0 + %1 + 6]
    pmaddwd     m4,     m0
    pshufd      m2,     m3,     q2301
    paddd       m3,     m2
    pshufd      m2,     m4,     q2301
    paddd       m4,     m2
    pshufd      m3,     m3,     q3120
    pshufd      m4,     m4,     q3120
    punpcklqdq  m3,     m4
    paddd       m3,     m1

    movh        m5,     [r0 + r1 + %1]
    movhps      m5,     [r0 + r1 + %1 + 2]
    pmaddwd     m5,     m0
    movh        m4,     [r0 + r1 + %1 + 4]
    movhps      m4,     [r0 + r1 + %1 + 6]
    pmaddwd     m4,     m0
    pshufd      m2,     m5,     q2301
    paddd       m5,     m2
    pshufd      m2,     m4,     q2301
    paddd       m4,     m2
    pshufd      m5,     m5,     q3120
    pshufd      m4,     m4,     q3120
    punpcklqdq  m5,     m4
    paddd       m5,     m1
%ifidn %2, pp
    psrad       m3,     6
    psrad       m5,     6
    packssdw    m3,     m5
    CLIPW       m3,     m7,     m6
%else
    psrad       m3,     INTERP_SHIFT_PS
    psrad       m5,     INTERP_SHIFT_PS
    packssdw    m3,     m5
%endif
    movh        [r2 + %1], m3
    movhps      [r2 + r3 + %1], m3
%endmacro

%macro FILTERH_W4_1_sse3 1
    movh        m3,     [r0 + 2 * r1 + %1]
    movhps      m3,     [r0 + 2 * r1 + %1 + 2]
    pmaddwd     m3,     m0
    movh        m4,     [r0 + 2 * r1 + %1 + 4]
    movhps      m4,     [r0 + 2 * r1 + %1 + 6]
    pmaddwd     m4,     m0
    pshufd      m2,     m3,     q2301
    paddd       m3,     m2
    pshufd      m2,     m4,     q2301
    paddd       m4,     m2
    pshufd      m3,     m3,     q3120
    pshufd      m4,     m4,     q3120
    punpcklqdq  m3,     m4
    paddd       m3,     m1

    psrad       m3,     INTERP_SHIFT_PS
    packssdw    m3,     m3
    movh        [r2 + r3 * 2 + %1], m3
%endmacro

%macro FILTERH_W8_1_sse3 2
    movh        m3,     [r0 + %1]
    movhps      m3,     [r0 + %1 + 2]
    pmaddwd     m3,     m0
    movh        m4,     [r0 + %1 + 4]
    movhps      m4,     [r0 + %1 + 6]
    pmaddwd     m4,     m0
    pshufd      m2,     m3,     q2301
    paddd       m3,     m2
    pshufd      m2,     m4,     q2301
    paddd       m4,     m2
    pshufd      m3,     m3,     q3120
    pshufd      m4,     m4,     q3120
    punpcklqdq  m3,     m4
    paddd       m3,     m1

    movh        m5,     [r0 + %1 + 8]
    movhps      m5,     [r0 + %1 + 10]
    pmaddwd     m5,     m0
    movh        m4,     [r0 + %1 + 12]
    movhps      m4,     [r0 + %1 + 14]
    pmaddwd     m4,     m0
    pshufd      m2,     m5,     q2301
    paddd       m5,     m2
    pshufd      m2,     m4,     q2301
    paddd       m4,     m2
    pshufd      m5,     m5,     q3120
    pshufd      m4,     m4,     q3120
    punpcklqdq  m5,     m4
    paddd       m5,     m1
%ifidn %2, pp
    psrad       m3,     6
    psrad       m5,     6
    packssdw    m3,     m5
    CLIPW       m3,     m7,     m6
%else
    psrad       m3,     INTERP_SHIFT_PS
    psrad       m5,     INTERP_SHIFT_PS
    packssdw    m3,     m5
%endif
    movdqu      [r2 + %1], m3
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_HOR_CHROMA_sse3 3
INIT_XMM sse3
cglobal interp_4tap_horiz_%3_%1x%2, 4, 7, 8
    add         r3,     r3
    add         r1,     r1
    sub         r0,     2
    mov         r4d,    r4m
    add         r4d,    r4d

%ifdef PIC
    lea         r6,     [h4_tab_ChromaCoeff]
    movddup     m0,     [r6 + r4 * 4]
%else
    movddup     m0,     [h4_tab_ChromaCoeff + r4 * 4]
%endif

%ifidn %3, ps
    mova        m1,     [INTERP_OFFSET_PS]
    cmp         r5m,    byte 0
%if %1 <= 6
    lea         r4,     [r1 * 3]
    lea         r5,     [r3 * 3]
%endif
    je          .skip
    sub         r0,     r1
%if %1 <= 6
%assign y 1
%else
%assign y 3
%endif
%assign z 0
%rep y
%assign x 0
%rep %1/8
    FILTERH_W8_1_sse3 x, %3
%assign x x+16
%endrep
%if %1 == 4 || (%1 == 6 && z == 0) || (%1 == 12 && z == 0)
    FILTERH_W4_2_sse3 x, %3
    FILTERH_W4_1_sse3 x
%assign x x+8
%endif
%if %1 == 2 || (%1 == 6 && z == 0)
    FILTERH_W2_3_sse3 x
%endif
%if %1 <= 6
    lea         r0,     [r0 + r4]
    lea         r2,     [r2 + r5]
%else
    lea         r0,     [r0 + r1]
    lea         r2,     [r2 + r3]
%endif
%assign z z+1
%endrep
.skip:
%elifidn %3, pp
    pxor        m7,     m7
    mova        m6,     [pw_pixel_max]
    mova        m1,     [tab_c_32]
%if %1 == 2 || %1 == 6
    lea         r4,     [r1 * 3]
    lea         r5,     [r3 * 3]
%endif
%endif

%if %1 == 2
%assign y %2/4
%elif %1 <= 6
%assign y %2/2
%else
%assign y %2
%endif
%assign z 0
%rep y
%assign x 0
%rep %1/8
    FILTERH_W8_1_sse3 x, %3
%assign x x+16
%endrep
%if %1 == 4 || %1 == 6 || (%1 == 12 && (z % 2) == 0)
    FILTERH_W4_2_sse3 x, %3
%assign x x+8
%endif
%if %1 == 2 || (%1 == 6 && (z % 2) == 0)
    FILTERH_W2_4_sse3 x, %3
%endif
%assign z z+1
%if z < y
%if %1 == 2
    lea         r0,     [r0 + 4 * r1]
    lea         r2,     [r2 + 4 * r3]
%elif %1 <= 6
    lea         r0,     [r0 + 2 * r1]
    lea         r2,     [r2 + 2 * r3]
%else
    lea         r0,     [r0 + r1]
    lea         r2,     [r2 + r3]
%endif
%endif ;z < y
%endrep

    RET
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------

%if ARCH_X86_64
FILTER_HOR_CHROMA_sse3 2, 4, pp
FILTER_HOR_CHROMA_sse3 2, 8, pp
FILTER_HOR_CHROMA_sse3 2, 16, pp
FILTER_HOR_CHROMA_sse3 4, 2, pp
FILTER_HOR_CHROMA_sse3 4, 4, pp
FILTER_HOR_CHROMA_sse3 4, 8, pp
FILTER_HOR_CHROMA_sse3 4, 16, pp
FILTER_HOR_CHROMA_sse3 4, 32, pp
FILTER_HOR_CHROMA_sse3 6, 8, pp
FILTER_HOR_CHROMA_sse3 6, 16, pp
FILTER_HOR_CHROMA_sse3 8, 2, pp
FILTER_HOR_CHROMA_sse3 8, 4, pp
FILTER_HOR_CHROMA_sse3 8, 6, pp
FILTER_HOR_CHROMA_sse3 8, 8, pp
FILTER_HOR_CHROMA_sse3 8, 12, pp
FILTER_HOR_CHROMA_sse3 8, 16, pp
FILTER_HOR_CHROMA_sse3 8, 32, pp
FILTER_HOR_CHROMA_sse3 8, 64, pp
FILTER_HOR_CHROMA_sse3 12, 16, pp
FILTER_HOR_CHROMA_sse3 12, 32, pp
FILTER_HOR_CHROMA_sse3 16, 4, pp
FILTER_HOR_CHROMA_sse3 16, 8, pp
FILTER_HOR_CHROMA_sse3 16, 12, pp
FILTER_HOR_CHROMA_sse3 16, 16, pp
FILTER_HOR_CHROMA_sse3 16, 24, pp
FILTER_HOR_CHROMA_sse3 16, 32, pp
FILTER_HOR_CHROMA_sse3 16, 64, pp
FILTER_HOR_CHROMA_sse3 24, 32, pp
FILTER_HOR_CHROMA_sse3 24, 64, pp
FILTER_HOR_CHROMA_sse3 32, 8, pp
FILTER_HOR_CHROMA_sse3 32, 16, pp
FILTER_HOR_CHROMA_sse3 32, 24, pp
FILTER_HOR_CHROMA_sse3 32, 32, pp
FILTER_HOR_CHROMA_sse3 32, 48, pp
FILTER_HOR_CHROMA_sse3 32, 64, pp
FILTER_HOR_CHROMA_sse3 48, 64, pp
FILTER_HOR_CHROMA_sse3 64, 16, pp
FILTER_HOR_CHROMA_sse3 64, 32, pp
FILTER_HOR_CHROMA_sse3 64, 48, pp
FILTER_HOR_CHROMA_sse3 64, 64, pp

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_ps_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------

FILTER_HOR_CHROMA_sse3 2, 4, ps
FILTER_HOR_CHROMA_sse3 2, 8, ps
FILTER_HOR_CHROMA_sse3 2, 16, ps
FILTER_HOR_CHROMA_sse3 4, 2, ps
FILTER_HOR_CHROMA_sse3 4, 4, ps
FILTER_HOR_CHROMA_sse3 4, 8, ps
FILTER_HOR_CHROMA_sse3 4, 16, ps
FILTER_HOR_CHROMA_sse3 4, 32, ps
FILTER_HOR_CHROMA_sse3 6, 8, ps
FILTER_HOR_CHROMA_sse3 6, 16, ps
FILTER_HOR_CHROMA_sse3 8, 2, ps
FILTER_HOR_CHROMA_sse3 8, 4, ps
FILTER_HOR_CHROMA_sse3 8, 6, ps
FILTER_HOR_CHROMA_sse3 8, 8, ps
FILTER_HOR_CHROMA_sse3 8, 12, ps
FILTER_HOR_CHROMA_sse3 8, 16, ps
FILTER_HOR_CHROMA_sse3 8, 32, ps
FILTER_HOR_CHROMA_sse3 8, 64, ps
FILTER_HOR_CHROMA_sse3 12, 16, ps
FILTER_HOR_CHROMA_sse3 12, 32, ps
FILTER_HOR_CHROMA_sse3 16, 4, ps
FILTER_HOR_CHROMA_sse3 16, 8, ps
FILTER_HOR_CHROMA_sse3 16, 12, ps
FILTER_HOR_CHROMA_sse3 16, 16, ps
FILTER_HOR_CHROMA_sse3 16, 24, ps
FILTER_HOR_CHROMA_sse3 16, 32, ps
FILTER_HOR_CHROMA_sse3 16, 64, ps
FILTER_HOR_CHROMA_sse3 24, 32, ps
FILTER_HOR_CHROMA_sse3 24, 64, ps
FILTER_HOR_CHROMA_sse3 32, 8, ps
FILTER_HOR_CHROMA_sse3 32, 16, ps
FILTER_HOR_CHROMA_sse3 32, 24, ps
FILTER_HOR_CHROMA_sse3 32, 32, ps
FILTER_HOR_CHROMA_sse3 32, 48, ps
FILTER_HOR_CHROMA_sse3 32, 64, ps
FILTER_HOR_CHROMA_sse3 48, 64, ps
FILTER_HOR_CHROMA_sse3 64, 16, ps
FILTER_HOR_CHROMA_sse3 64, 32, ps
FILTER_HOR_CHROMA_sse3 64, 48, ps
FILTER_HOR_CHROMA_sse3 64, 64, ps
%endif

%macro FILTER_W2_2 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + r1]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    packusdw    m3,         m3
    CLIPW       m3,         m7,    m6
%else
    psrad       m3,         INTERP_SHIFT_PS
    packssdw    m3,         m3
%endif
    movd        [r2],       m3
    pextrd      [r2 + r3],  m3, 1
%endmacro

%macro FILTER_W4_2 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + r1]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + r1 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m7,    m6
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2],       m3
    movhps      [r2 + r3],  m3
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_CHROMA_H 6
INIT_XMM sse4
cglobal interp_4tap_horiz_%3_%1x%2, 4, %4, %5

    add         r3,       r3
    add         r1,       r1
    sub         r0,       2
    mov         r4d,      r4m
    add         r4d,      r4d

%ifdef PIC
    lea         r%6,      [h4_tab_ChromaCoeff]
    movh        m0,       [r%6 + r4 * 4]
%else
    movh        m0,       [h4_tab_ChromaCoeff + r4 * 4]
%endif

    punpcklqdq  m0,       m0
    mova        m2,       [tab_Tm16]

%ifidn %3, ps
    mova        m1,       [INTERP_OFFSET_PS]
    cmp         r5m, byte 0
    je          .skip
    sub         r0,       r1
    movu        m3,       [r0]
    pshufb      m3,       m3, m2
    pmaddwd     m3,       m0

  %if %1 == 4
    movu        m4,       [r0 + 4]
    pshufb      m4,       m4, m2
    pmaddwd     m4,       m0
    phaddd      m3,       m4
  %else
    phaddd      m3,       m3
  %endif

    paddd       m3,       m1
    psrad       m3,       INTERP_SHIFT_PS
    packssdw    m3,       m3

  %if %1 == 2
    movd        [r2],     m3
  %else
    movh        [r2],     m3
  %endif

    add         r0,       r1
    add         r2,       r3
    FILTER_W%1_2 %3
    lea         r0,       [r0 + 2 * r1]
    lea         r2,       [r2 + 2 * r3]

.skip:

%else     ;%ifidn %3, ps
    pxor        m7,       m7
    mova        m6,       [pw_pixel_max]
    mova        m1,       [tab_c_32]
%endif    ;%ifidn %3, ps

    FILTER_W%1_2 %3

%rep (%2/2) - 1
    lea         r0,       [r0 + 2 * r1]
    lea         r2,       [r2 + 2 * r3]
    FILTER_W%1_2 %3
%endrep
    RET
%endmacro

FILTER_CHROMA_H 2, 4, pp, 6, 8, 5
FILTER_CHROMA_H 2, 8, pp, 6, 8, 5
FILTER_CHROMA_H 4, 2, pp, 6, 8, 5
FILTER_CHROMA_H 4, 4, pp, 6, 8, 5
FILTER_CHROMA_H 4, 8, pp, 6, 8, 5
FILTER_CHROMA_H 4, 16, pp, 6, 8, 5

FILTER_CHROMA_H 2, 4, ps, 7, 5, 6
FILTER_CHROMA_H 2, 8, ps, 7, 5, 6
FILTER_CHROMA_H 4, 2, ps, 7, 6, 6
FILTER_CHROMA_H 4, 4, ps, 7, 6, 6
FILTER_CHROMA_H 4, 8, ps, 7, 6, 6
FILTER_CHROMA_H 4, 16, ps, 7, 6, 6

FILTER_CHROMA_H 2, 16, pp, 6, 8, 5
FILTER_CHROMA_H 4, 32, pp, 6, 8, 5
FILTER_CHROMA_H 2, 16, ps, 7, 5, 6
FILTER_CHROMA_H 4, 32, ps, 7, 6, 6


%macro FILTER_W6_1 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m4,         [r0 + 8]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m4,         m4
    paddd       m4,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m4,         INTERP_SHIFT_PP
    packusdw    m3,         m4
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m4,         INTERP_SHIFT_PS
    packssdw    m3,         m4
%endif
    movh        [r2],       m3
    pextrd      [r2 + 8],   m3, 2
%endmacro

cglobal chroma_filter_pp_6x1_internal
    FILTER_W6_1 pp
    ret

cglobal chroma_filter_ps_6x1_internal
    FILTER_W6_1 ps
    ret

%macro FILTER_W8_1 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 8]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 12]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2],       m3
    movhps      [r2 + 8],   m3
%endmacro

cglobal chroma_filter_pp_8x1_internal
    FILTER_W8_1 pp
    ret

cglobal chroma_filter_ps_8x1_internal
    FILTER_W8_1 ps
    ret

%macro FILTER_W12_1 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 8]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 12]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2],       m3
    movhps      [r2 + 8],   m3

    movu        m3,         [r0 + 16]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 20]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    packusdw    m3,         m3
    CLIPW       m3,         m6, m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    packssdw    m3,         m3
%endif
    movh        [r2 + 16],  m3
%endmacro

cglobal chroma_filter_pp_12x1_internal
    FILTER_W12_1 pp
    ret

cglobal chroma_filter_ps_12x1_internal
    FILTER_W12_1 ps
    ret

%macro FILTER_W16_1 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 8]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 12]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2],       m3
    movhps      [r2 + 8],   m3

    movu        m3,         [r0 + 16]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 20]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 24]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 28]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2 + 16],  m3
    movhps      [r2 + 24],  m3
%endmacro

cglobal chroma_filter_pp_16x1_internal
    FILTER_W16_1 pp
    ret

cglobal chroma_filter_ps_16x1_internal
    FILTER_W16_1 ps
    ret

%macro FILTER_W24_1 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 8]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 12]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2],       m3
    movhps      [r2 + 8],   m3

    movu        m3,         [r0 + 16]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 20]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 24]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 28]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2 + 16],  m3
    movhps      [r2 + 24],  m3

    movu        m3,         [r0 + 32]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 36]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 40]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 44]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2 + 32],  m3
    movhps      [r2 + 40],  m3
%endmacro

cglobal chroma_filter_pp_24x1_internal
    FILTER_W24_1 pp
    ret

cglobal chroma_filter_ps_24x1_internal
    FILTER_W24_1 ps
    ret

%macro FILTER_W32_1 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 8]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 12]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2],       m3
    movhps      [r2 + 8],   m3

    movu        m3,         [r0 + 16]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 20]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 24]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 28]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2 + 16],  m3
    movhps      [r2 + 24],  m3

    movu        m3,         [r0 + 32]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 36]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 40]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 44]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2 + 32],  m3
    movhps      [r2 + 40],  m3

    movu        m3,         [r0 + 48]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 52]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 56]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 60]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2 + 48],  m3
    movhps      [r2 + 56],  m3
%endmacro

cglobal chroma_filter_pp_32x1_internal
    FILTER_W32_1 pp
    ret

cglobal chroma_filter_ps_32x1_internal
    FILTER_W32_1 ps
    ret

%macro FILTER_W8o_1 2
    movu        m3,         [r0 + %2]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + %2 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + %2 + 8]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + %2 + 12]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         INTERP_SHIFT_PP
    psrad       m5,         INTERP_SHIFT_PP
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         INTERP_SHIFT_PS
    psrad       m5,         INTERP_SHIFT_PS
    packssdw    m3,         m5
%endif
    movh        [r2 + %2],       m3
    movhps      [r2 + %2 + 8],   m3
%endmacro

%macro FILTER_W48_1 1
    FILTER_W8o_1 %1, 0
    FILTER_W8o_1 %1, 16
    FILTER_W8o_1 %1, 32
    FILTER_W8o_1 %1, 48
    FILTER_W8o_1 %1, 64
    FILTER_W8o_1 %1, 80
%endmacro

cglobal chroma_filter_pp_48x1_internal
    FILTER_W48_1 pp
    ret

cglobal chroma_filter_ps_48x1_internal
    FILTER_W48_1 ps
    ret

%macro FILTER_W64_1 1
    FILTER_W8o_1 %1, 0
    FILTER_W8o_1 %1, 16
    FILTER_W8o_1 %1, 32
    FILTER_W8o_1 %1, 48
    FILTER_W8o_1 %1, 64
    FILTER_W8o_1 %1, 80
    FILTER_W8o_1 %1, 96
    FILTER_W8o_1 %1, 112
%endmacro

cglobal chroma_filter_pp_64x1_internal
    FILTER_W64_1 pp
    ret

cglobal chroma_filter_ps_64x1_internal
    FILTER_W64_1 ps
    ret
;-----------------------------------------------------------------------------
; void interp_4tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------

INIT_XMM sse4
%macro IPFILTER_CHROMA 6
cglobal interp_4tap_horiz_%3_%1x%2, 4, %5, %6

    add         r3,        r3
    add         r1,        r1
    sub         r0,         2
    mov         r4d,        r4m
    add         r4d,        r4d

%ifdef PIC
    lea         r%4,       [h4_tab_ChromaCoeff]
    movh        m0,       [r%4 + r4 * 4]
%else
    movh        m0,       [h4_tab_ChromaCoeff + r4 * 4]
%endif

    punpcklqdq  m0,       m0
    mova        m2,       [tab_Tm16]

%ifidn %3, ps
    mova        m1,       [INTERP_OFFSET_PS]
    cmp         r5m, byte 0
    je          .skip
    sub         r0, r1
    call chroma_filter_%3_%1x1_internal
    add         r0, r1
    add         r2, r3
    call chroma_filter_%3_%1x1_internal
    add         r0, r1
    add         r2, r3
    call chroma_filter_%3_%1x1_internal
    add         r0, r1
    add         r2, r3
.skip:
%else
    mova        m1,         [tab_c_32]
    pxor        m6,         m6
    mova        m7,         [pw_pixel_max]
%endif

    call chroma_filter_%3_%1x1_internal
%rep %2 - 1
    add         r0,       r1
    add         r2,       r3
    call chroma_filter_%3_%1x1_internal
%endrep
RET
%endmacro
IPFILTER_CHROMA 6, 8, pp, 5, 6, 8
IPFILTER_CHROMA 8, 2, pp, 5, 6, 8
IPFILTER_CHROMA 8, 4, pp, 5, 6, 8
IPFILTER_CHROMA 8, 6, pp, 5, 6, 8
IPFILTER_CHROMA 8, 8, pp, 5, 6, 8
IPFILTER_CHROMA 8, 16, pp, 5, 6, 8
IPFILTER_CHROMA 8, 32, pp, 5, 6, 8
IPFILTER_CHROMA 12, 16, pp, 5, 6, 8
IPFILTER_CHROMA 16, 4, pp, 5, 6, 8
IPFILTER_CHROMA 16, 8, pp, 5, 6, 8
IPFILTER_CHROMA 16, 12, pp, 5, 6, 8
IPFILTER_CHROMA 16, 16, pp, 5, 6, 8
IPFILTER_CHROMA 16, 32, pp, 5, 6, 8
IPFILTER_CHROMA 24, 32, pp, 5, 6, 8
IPFILTER_CHROMA 32, 8, pp, 5, 6, 8
IPFILTER_CHROMA 32, 16, pp, 5, 6, 8
IPFILTER_CHROMA 32, 24, pp, 5, 6, 8
IPFILTER_CHROMA 32, 32, pp, 5, 6, 8

IPFILTER_CHROMA 6, 8, ps, 6, 7, 6
IPFILTER_CHROMA 8, 2, ps, 6, 7, 6
IPFILTER_CHROMA 8, 4, ps, 6, 7, 6
IPFILTER_CHROMA 8, 6, ps, 6, 7, 6
IPFILTER_CHROMA 8, 8, ps, 6, 7, 6
IPFILTER_CHROMA 8, 16, ps, 6, 7, 6
IPFILTER_CHROMA 8, 32, ps, 6, 7, 6
IPFILTER_CHROMA 12, 16, ps, 6, 7, 6
IPFILTER_CHROMA 16, 4, ps, 6, 7, 6
IPFILTER_CHROMA 16, 8, ps, 6, 7, 6
IPFILTER_CHROMA 16, 12, ps, 6, 7, 6
IPFILTER_CHROMA 16, 16, ps, 6, 7, 6
IPFILTER_CHROMA 16, 32, ps, 6, 7, 6
IPFILTER_CHROMA 24, 32, ps, 6, 7, 6
IPFILTER_CHROMA 32, 8, ps, 6, 7, 6
IPFILTER_CHROMA 32, 16, ps, 6, 7, 6
IPFILTER_CHROMA 32, 24, ps, 6, 7, 6
IPFILTER_CHROMA 32, 32, ps, 6, 7, 6

IPFILTER_CHROMA 6, 16, pp, 5, 6, 8
IPFILTER_CHROMA 8, 12, pp, 5, 6, 8
IPFILTER_CHROMA 8, 64, pp, 5, 6, 8
IPFILTER_CHROMA 12, 32, pp, 5, 6, 8
IPFILTER_CHROMA 16, 24, pp, 5, 6, 8
IPFILTER_CHROMA 16, 64, pp, 5, 6, 8
IPFILTER_CHROMA 24, 64, pp, 5, 6, 8
IPFILTER_CHROMA 32, 48, pp, 5, 6, 8
IPFILTER_CHROMA 32, 64, pp, 5, 6, 8
IPFILTER_CHROMA 6, 16, ps, 6, 7, 6
IPFILTER_CHROMA 8, 12, ps, 6, 7, 6
IPFILTER_CHROMA 8, 64, ps, 6, 7, 6
IPFILTER_CHROMA 12, 32, ps, 6, 7, 6
IPFILTER_CHROMA 16, 24, ps, 6, 7, 6
IPFILTER_CHROMA 16, 64, ps, 6, 7, 6
IPFILTER_CHROMA 24, 64, ps, 6, 7, 6
IPFILTER_CHROMA 32, 48, ps, 6, 7, 6
IPFILTER_CHROMA 32, 64, ps, 6, 7, 6

IPFILTER_CHROMA 48, 64, pp, 5, 6, 8
IPFILTER_CHROMA 64, 48, pp, 5, 6, 8
IPFILTER_CHROMA 64, 64, pp, 5, 6, 8
IPFILTER_CHROMA 64, 32, pp, 5, 6, 8
IPFILTER_CHROMA 64, 16, pp, 5, 6, 8
IPFILTER_CHROMA 48, 64, ps, 6, 7, 6
IPFILTER_CHROMA 64, 48, ps, 6, 7, 6
IPFILTER_CHROMA 64, 64, ps, 6, 7, 6
IPFILTER_CHROMA 64, 32, ps, 6, 7, 6
IPFILTER_CHROMA 64, 16, ps, 6, 7, 6

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
%macro IPFILTER_CHROMA_avx2_6xN 1
cglobal interp_4tap_horiz_pp_6x%1, 5,6,8
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [h4_tab_ChromaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova            m1, [h4_interp8_hpp_shuf]
    vpbroadcastd    m2, [pd_32]
    pxor            m5, m5
    mova            m6, [idct8_shuf2]
    mova            m7, [pw_pixel_max]

    mov             r4d, %1/2
.loop:
    vbroadcasti128  m3, [r0]
    vbroadcasti128  m4, [r0 + 8]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, INTERP_SHIFT_PP           ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m3, m3
    vpermq          m3, m3, q2020
    pshufb          xm3, xm6                      ; m3 = WORD[7 6 5 4 3 2 1 0]
    CLIPW           xm3, xm5, xm7
    movq            [r2], xm3
    pextrd          [r2 + 8], xm3, 2

    vbroadcasti128  m3, [r0 + r1]
    vbroadcasti128  m4, [r0 + r1 + 8]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, INTERP_SHIFT_PP           ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m3, m3
    vpermq          m3, m3, q2020
    pshufb          xm3, xm6                      ; m3 = WORD[7 6 5 4 3 2 1 0]
    CLIPW           xm3, xm5, xm7
    movq            [r2 + r3], xm3
    pextrd          [r2 + r3 + 8], xm3, 2

    lea             r0, [r0 + r1 * 2]
    lea             r2, [r2 + r3 * 2]
    dec             r4d
    jnz             .loop
    RET
%endmacro
IPFILTER_CHROMA_avx2_6xN 8
IPFILTER_CHROMA_avx2_6xN 16

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
cglobal interp_4tap_horiz_pp_8x2, 5,6,8
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [h4_tab_ChromaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova            m1, [h4_interp8_hpp_shuf]
    vpbroadcastd    m2, [pd_32]
    pxor            m5, m5
    mova            m6, [idct8_shuf2]
    mova            m7, [pw_pixel_max]

    vbroadcasti128  m3, [r0]
    vbroadcasti128  m4, [r0 + 8]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, INTERP_SHIFT_PP          ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m3, m3
    vpermq          m3, m3,q2020
    pshufb          xm3, xm6                     ; m3 = WORD[7 6 5 4 3 2 1 0]
    CLIPW           xm3, xm5, xm7
    movu            [r2], xm3

    vbroadcasti128  m3, [r0 + r1]
    vbroadcasti128  m4, [r0 + r1 + 8]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, INTERP_SHIFT_PP           ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m3, m3
    vpermq          m3, m3,q2020
    pshufb          xm3, xm6                      ; m3 = WORD[7 6 5 4 3 2 1 0]
    CLIPW           xm3, xm5, xm7
    movu            [r2 + r3], xm3
    RET

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
cglobal interp_4tap_horiz_pp_8x4, 5,6,8
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [h4_tab_ChromaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova            m1, [h4_interp8_hpp_shuf]
    vpbroadcastd    m2, [pd_32]
    pxor            m5, m5
    mova            m6, [idct8_shuf2]
    mova            m7, [pw_pixel_max]

%rep 2
    vbroadcasti128  m3, [r0]
    vbroadcasti128  m4, [r0 + 8]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, 6                       ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m3, m3
    vpermq          m3, m3,q2020
    pshufb          xm3, xm6                    ; m3 = WORD[7 6 5 4 3 2 1 0]
    CLIPW           xm3, xm5, xm7
    movu            [r2], xm3

    vbroadcasti128  m3, [r0 + r1]
    vbroadcasti128  m4, [r0 + r1 + 8]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, 6                       ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m3, m3
    vpermq          m3, m3,q2020
    pshufb          xm3, xm6                    ; m3 = WORD[7 6 5 4 3 2 1 0]
    CLIPW           xm3, xm5, xm7
    movu            [r2 + r3], xm3

    lea             r0, [r0 + r1 * 2]
    lea             r2, [r2 + r3 * 2]
%endrep
    RET

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
%macro IPFILTER_CHROMA_avx2_8xN 1
cglobal interp_4tap_horiz_pp_8x%1, 5,6,8
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [h4_tab_ChromaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova            m1, [h4_interp8_hpp_shuf]
    vpbroadcastd    m2, [pd_32]
    pxor            m5, m5
    mova            m6, [idct8_shuf2]
    mova            m7, [pw_pixel_max]

    mov             r4d, %1/2
.loop:
    vbroadcasti128  m3, [r0]
    vbroadcasti128  m4, [r0 + 8]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, 6                         ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m3, m3
    vpermq          m3, m3, q2020
    pshufb          xm3, xm6                      ; m3 = WORD[7 6 5 4 3 2 1 0]
    CLIPW           xm3, xm5, xm7
    movu            [r2], xm3

    vbroadcasti128  m3, [r0 + r1]
    vbroadcasti128  m4, [r0 + r1 + 8]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, 6                         ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m3, m3
    vpermq          m3, m3, q2020
    pshufb          xm3, xm6                      ; m3 = WORD[7 6 5 4 3 2 1 0]
    CLIPW           xm3, xm5, xm7
    movu            [r2 + r3], xm3

    lea             r0, [r0 + r1 * 2]
    lea             r2, [r2 + r3 * 2]
    dec             r4d
    jnz             .loop
    RET
%endmacro
IPFILTER_CHROMA_avx2_8xN 6
IPFILTER_CHROMA_avx2_8xN 8
IPFILTER_CHROMA_avx2_8xN 12
IPFILTER_CHROMA_avx2_8xN 16
IPFILTER_CHROMA_avx2_8xN 32
IPFILTER_CHROMA_avx2_8xN 64

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
%macro IPFILTER_CHROMA_avx2_16xN 1
%if ARCH_X86_64
cglobal interp_4tap_horiz_pp_16x%1, 5,6,9
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [h4_tab_ChromaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova            m1, [h4_interp8_hpp_shuf]
    vpbroadcastd    m2, [pd_32]
    pxor            m5, m5
    mova            m6, [idct8_shuf2]
    mova            m7, [pw_pixel_max]

    mov             r4d, %1
.loop:
    vbroadcasti128  m3, [r0]
    vbroadcasti128  m4, [r0 + 8]

    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, 6                       ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m3, m3
    vpermq          m3, m3, q2020
    pshufb          xm3, xm6                     ; m3 = WORD[7 6 5 4 3 2 1 0]

    vbroadcasti128  m4, [r0 + 16]
    vbroadcasti128  m8, [r0 + 24]

    pshufb          m4, m1
    pshufb          m8, m1

    pmaddwd         m4, m0
    pmaddwd         m8, m0
    phaddd          m4, m8
    paddd           m4, m2
    psrad           m4, 6                       ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m4, m4
    vpermq          m4, m4, q2020
    pshufb          xm4, xm6                    ; m3 = WORD[7 6 5 4 3 2 1 0]
    vinserti128     m3, m3, xm4, 1
    CLIPW           m3, m5, m7
    movu            [r2], m3

    add             r0, r1
    add             r2, r3
    dec             r4d
    jnz             .loop
    RET
%endif
%endmacro
IPFILTER_CHROMA_avx2_16xN 4
IPFILTER_CHROMA_avx2_16xN 8
IPFILTER_CHROMA_avx2_16xN 12
IPFILTER_CHROMA_avx2_16xN 16
IPFILTER_CHROMA_avx2_16xN 24
IPFILTER_CHROMA_avx2_16xN 32
IPFILTER_CHROMA_avx2_16xN 64

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
%macro IPFILTER_CHROMA_avx2_32xN 1
%if ARCH_X86_64
cglobal interp_4tap_horiz_pp_32x%1, 5,6,9
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [h4_tab_ChromaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova            m1, [h4_interp8_hpp_shuf]
    vpbroadcastd    m2, [pd_32]
    pxor            m5, m5
    mova            m6, [idct8_shuf2]
    mova            m7, [pw_pixel_max]

    mov             r6d, %1
.loop:
%assign x 0
%rep 2
    vbroadcasti128  m3, [r0 + x]
    vbroadcasti128  m4, [r0 + 8 + x]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, 6                       ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m3, m3
    vpermq          m3, m3, q2020
    pshufb          xm3, xm6                     ; m3 = WORD[7 6 5 4 3 2 1 0]

    vbroadcasti128  m4, [r0 + 16 + x]
    vbroadcasti128  m8, [r0 + 24 + x]
    pshufb          m4, m1
    pshufb          m8, m1

    pmaddwd         m4, m0
    pmaddwd         m8, m0
    phaddd          m4, m8
    paddd           m4, m2
    psrad           m4, 6                       ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m4, m4
    vpermq          m4, m4, q2020
    pshufb          xm4, xm6                    ; m3 = WORD[7 6 5 4 3 2 1 0]
    vinserti128     m3, m3, xm4, 1
    CLIPW           m3, m5, m7
    movu            [r2 + x], m3
    %assign x x+32
    %endrep

    add             r0, r1
    add             r2, r3
    dec             r6d
    jnz             .loop
    RET
%endif
%endmacro
IPFILTER_CHROMA_avx2_32xN 8
IPFILTER_CHROMA_avx2_32xN 16
IPFILTER_CHROMA_avx2_32xN 24
IPFILTER_CHROMA_avx2_32xN 32
IPFILTER_CHROMA_avx2_32xN 48
IPFILTER_CHROMA_avx2_32xN 64

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
%macro IPFILTER_CHROMA_avx2_12xN 1
%if ARCH_X86_64
cglobal interp_4tap_horiz_pp_12x%1, 5,6,8
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [h4_tab_ChromaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova            m1, [h4_interp8_hpp_shuf]
    vpbroadcastd    m2, [pd_32]
    pxor            m5, m5
    mova            m6, [idct8_shuf2]
    mova            m7, [pw_pixel_max]

    mov             r4d, %1
.loop:
    vbroadcasti128  m3, [r0]
    vbroadcasti128  m4, [r0 + 8]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, 6                       ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m3, m3
    vpermq          m3, m3, q2020
    pshufb          xm3, xm6                     ; m3 = WORD[7 6 5 4 3 2 1 0]
    CLIPW           xm3, xm5, xm7
    movu            [r2], xm3

    vbroadcasti128  m3, [r0 + 16]
    vbroadcasti128  m4, [r0 + 24]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, 6                       ; m3 = DWORD[7 6 3 2 5 4 1 0]

    packusdw        m3, m3
    vpermq          m3, m3, q2020
    pshufb          xm3, xm6                    ; m3 = WORD[7 6 5 4 3 2 1 0]
    CLIPW           xm3, xm5, xm7
    movq            [r2 + 16], xm3

    add             r0, r1
    add             r2, r3
    dec             r4d
    jnz             .loop
    RET
%endif
%endmacro
IPFILTER_CHROMA_avx2_12xN 16
IPFILTER_CHROMA_avx2_12xN 32

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
%macro IPFILTER_CHROMA_avx2_24xN 1
%if ARCH_X86_64
cglobal interp_4tap_horiz_pp_24x%1, 5,6,9
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [h4_tab_ChromaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova            m1, [h4_interp8_hpp_shuf]
    vpbroadcastd    m2, [pd_32]
    pxor            m5, m5
    mova            m6, [idct8_shuf2]
    mova            m7, [pw_pixel_max]

    mov             r4d, %1
.loop:
    vbroadcasti128  m3, [r0]
    vbroadcasti128  m4, [r0 + 8]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, 6

    vbroadcasti128  m4, [r0 + 16]
    vbroadcasti128  m8, [r0 + 24]
    pshufb          m4, m1
    pshufb          m8, m1

    pmaddwd         m4, m0
    pmaddwd         m8, m0
    phaddd          m4, m8
    paddd           m4, m2
    psrad           m4, 6

    packusdw        m3, m4
    vpermq          m3, m3, q3120
    pshufb          m3, m6
    CLIPW           m3, m5, m7
    movu            [r2], m3

    vbroadcasti128  m3, [r0 + 32]
    vbroadcasti128  m4, [r0 + 40]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, 6

    packusdw        m3, m3
    vpermq          m3, m3, q2020
    pshufb          xm3, xm6
    CLIPW           xm3, xm5, xm7
    movu            [r2 + 32], xm3

    add             r0, r1
    add             r2, r3
    dec             r4d
    jnz             .loop
    RET
%endif
%endmacro
IPFILTER_CHROMA_avx2_24xN 32
IPFILTER_CHROMA_avx2_24xN 64

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
%macro IPFILTER_CHROMA_avx2_64xN 1
%if ARCH_X86_64
cglobal interp_4tap_horiz_pp_64x%1, 5,6,9
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [h4_tab_ChromaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova            m1, [h4_interp8_hpp_shuf]
    vpbroadcastd    m2, [pd_32]
    pxor            m5, m5
    mova            m6, [idct8_shuf2]
    mova            m7, [pw_pixel_max]

    mov             r6d, %1
.loop:
%assign x 0
%rep 4
    vbroadcasti128  m3, [r0 + x]
    vbroadcasti128  m4, [r0 + 8 + x]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, 6

    vbroadcasti128  m4, [r0 + 16 + x]
    vbroadcasti128  m8, [r0 + 24 + x]
    pshufb          m4, m1
    pshufb          m8, m1

    pmaddwd         m4, m0
    pmaddwd         m8, m0
    phaddd          m4, m8
    paddd           m4, m2
    psrad           m4, 6

    packusdw        m3, m4
    vpermq          m3, m3, q3120
    pshufb          m3, m6
    CLIPW           m3, m5, m7
    movu            [r2 + x], m3
    %assign x x+32
    %endrep

    add             r0, r1
    add             r2, r3
    dec             r6d
    jnz             .loop
    RET
%endif
%endmacro
IPFILTER_CHROMA_avx2_64xN 16
IPFILTER_CHROMA_avx2_64xN 32
IPFILTER_CHROMA_avx2_64xN 48
IPFILTER_CHROMA_avx2_64xN 64

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
%if ARCH_X86_64
cglobal interp_4tap_horiz_pp_48x64, 5,6,9
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [h4_tab_ChromaCoeff]
    vpbroadcastq    m0, [r5 + r4 * 8]
%else
    vpbroadcastq    m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova            m1, [h4_interp8_hpp_shuf]
    vpbroadcastd    m2, [pd_32]
    pxor            m5, m5
    mova            m6, [idct8_shuf2]
    mova            m7, [pw_pixel_max]

    mov             r4d, 64
.loop:
%assign x 0
%rep 3
    vbroadcasti128  m3, [r0 + x]
    vbroadcasti128  m4, [r0 + 8 + x]
    pshufb          m3, m1
    pshufb          m4, m1

    pmaddwd         m3, m0
    pmaddwd         m4, m0
    phaddd          m3, m4
    paddd           m3, m2
    psrad           m3, 6

    vbroadcasti128  m4, [r0 + 16 + x]
    vbroadcasti128  m8, [r0 + 24 + x]
    pshufb          m4, m1
    pshufb          m8, m1

    pmaddwd         m4, m0
    pmaddwd         m8, m0
    phaddd          m4, m8
    paddd           m4, m2
    psrad           m4, 6

    packusdw        m3, m4
    vpermq          m3, m3, q3120
    pshufb          m3, m6
    CLIPW           m3, m5, m7
    movu            [r2 + x], m3
%assign x x+32
%endrep

    add             r0, r1
    add             r2, r3
    dec             r4d
    jnz             .loop
    RET
%endif

%macro IPFILTER_CHROMA_PS_8xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_horiz_ps_8x%1, 4, 7, 6
    add                 r1d, r1d
    add                 r3d, r3d
    mov                 r4d, r4m
    mov                 r5d, r5m

%ifdef PIC
    lea                 r6, [h4_tab_ChromaCoeff]
    vpbroadcastq        m0, [r6 + r4 * 8]
%else
    vpbroadcastq        m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova                m3, [h4_interp8_hpp_shuf]
    vbroadcasti128      m2, [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 2
    test                r5d, r5d
    mov                 r4d, %1
    jz                  .loop0
    sub                 r0, r1
    add                 r4d, 3

.loop0:
    vbroadcasti128      m4, [r0]
    vbroadcasti128      m5, [r0 + 8]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2], xm4
    add                 r2, r3
    add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif
%endmacro

    IPFILTER_CHROMA_PS_8xN_AVX2 4
    IPFILTER_CHROMA_PS_8xN_AVX2 8
    IPFILTER_CHROMA_PS_8xN_AVX2 16
    IPFILTER_CHROMA_PS_8xN_AVX2 32
    IPFILTER_CHROMA_PS_8xN_AVX2 6
    IPFILTER_CHROMA_PS_8xN_AVX2 2
    IPFILTER_CHROMA_PS_8xN_AVX2 12
    IPFILTER_CHROMA_PS_8xN_AVX2 64

%macro IPFILTER_CHROMA_PS_16xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_horiz_ps_16x%1, 4, 7, 6
    add                 r1d, r1d
    add                 r3d, r3d
    mov                 r4d, r4m
    mov                 r5d, r5m

%ifdef PIC
    lea                 r6, [h4_tab_ChromaCoeff]
    vpbroadcastq        m0, [r6 + r4 * 8]
%else
    vpbroadcastq        m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova                m3, [h4_interp8_hpp_shuf]
    vbroadcasti128      m2, [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 2
    test                r5d, r5d
    mov                 r4d, %1
    jz                  .loop0
    sub                 r0, r1
    add                 r4d, 3

.loop0:
    vbroadcasti128      m4, [r0]
    vbroadcasti128      m5, [r0 + 8]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2], xm4

    vbroadcasti128      m4, [r0 + 16]
    vbroadcasti128      m5, [r0 + 24]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 16], xm4

    add                 r2, r3
    add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif
%endmacro

IPFILTER_CHROMA_PS_16xN_AVX2 16
IPFILTER_CHROMA_PS_16xN_AVX2 8
IPFILTER_CHROMA_PS_16xN_AVX2 32
IPFILTER_CHROMA_PS_16xN_AVX2 12
IPFILTER_CHROMA_PS_16xN_AVX2 4
IPFILTER_CHROMA_PS_16xN_AVX2 64
IPFILTER_CHROMA_PS_16xN_AVX2 24

%macro IPFILTER_CHROMA_PS_24xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_horiz_ps_24x%1, 4, 7, 6
    add                 r1d, r1d
    add                 r3d, r3d
    mov                 r4d, r4m
    mov                 r5d, r5m

%ifdef PIC
    lea                 r6, [h4_tab_ChromaCoeff]
    vpbroadcastq        m0, [r6 + r4 * 8]
%else
    vpbroadcastq        m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova                m3, [h4_interp8_hpp_shuf]
    vbroadcasti128      m2, [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 2
    test                r5d, r5d
    mov                 r4d, %1
    jz                  .loop0
    sub                 r0, r1
    add                 r4d, 3

.loop0:
    vbroadcasti128      m4, [r0]
    vbroadcasti128      m5, [r0 + 8]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2], xm4

    vbroadcasti128      m4, [r0 + 16]
    vbroadcasti128      m5, [r0 + 24]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 16], xm4

    vbroadcasti128      m4, [r0 + 32]
    vbroadcasti128      m5, [r0 + 40]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 32], xm4

    add                 r2, r3
    add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif
%endmacro

IPFILTER_CHROMA_PS_24xN_AVX2 32
IPFILTER_CHROMA_PS_24xN_AVX2 64

%macro IPFILTER_CHROMA_PS_12xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_horiz_ps_12x%1, 4, 7, 6
    add                 r1d, r1d
    add                 r3d, r3d
    mov                 r4d, r4m
    mov                 r5d, r5m

%ifdef PIC
    lea                 r6, [h4_tab_ChromaCoeff]
    vpbroadcastq        m0, [r6 + r4 * 8]
%else
    vpbroadcastq        m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova                m3, [h4_interp8_hpp_shuf]
    vbroadcasti128      m2, [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 2
    test                r5d, r5d
    mov                 r4d, %1
    jz                  .loop0
    sub                 r0, r1
    add                 r4d, 3

.loop0:
    vbroadcasti128      m4, [r0]
    vbroadcasti128      m5, [r0 + 8]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2], xm4

    vbroadcasti128      m4, [r0 + 16]
    pshufb              m4, m3
    pmaddwd             m4, m0
    phaddd              m4, m4
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movq                [r2 + 16], xm4

    add                 r2, r3
    add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif
%endmacro

IPFILTER_CHROMA_PS_12xN_AVX2 16
IPFILTER_CHROMA_PS_12xN_AVX2 32

%macro IPFILTER_CHROMA_PS_32xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_horiz_ps_32x%1, 4, 7, 6
    add                 r1d, r1d
    add                 r3d, r3d
    mov                 r4d, r4m
    mov                 r5d, r5m

%ifdef PIC
    lea                 r6, [h4_tab_ChromaCoeff]
    vpbroadcastq        m0, [r6 + r4 * 8]
%else
    vpbroadcastq        m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova                m3, [h4_interp8_hpp_shuf]
    vbroadcasti128      m2, [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 2
    test                r5d, r5d
    mov                 r4d, %1
    jz                  .loop0
    sub                 r0, r1
    add                 r4d, 3

.loop0:
    vbroadcasti128      m4, [r0]
    vbroadcasti128      m5, [r0 + 8]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2], xm4

    vbroadcasti128      m4, [r0 + 16]
    vbroadcasti128      m5, [r0 + 24]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 16], xm4

    vbroadcasti128      m4, [r0 + 32]
    vbroadcasti128      m5, [r0 + 40]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 32], xm4

    vbroadcasti128      m4, [r0 + 48]
    vbroadcasti128      m5, [r0 + 56]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 48], xm4

    add                 r2, r3
    add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif
%endmacro

IPFILTER_CHROMA_PS_32xN_AVX2 32
IPFILTER_CHROMA_PS_32xN_AVX2 16
IPFILTER_CHROMA_PS_32xN_AVX2 24
IPFILTER_CHROMA_PS_32xN_AVX2 8
IPFILTER_CHROMA_PS_32xN_AVX2 64
IPFILTER_CHROMA_PS_32xN_AVX2 48


%macro IPFILTER_CHROMA_PS_64xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_horiz_ps_64x%1, 4, 7, 6
    add                 r1d, r1d
    add                 r3d, r3d
    mov                 r4d, r4m
    mov                 r5d, r5m

%ifdef PIC
    lea                 r6, [h4_tab_ChromaCoeff]
    vpbroadcastq        m0, [r6 + r4 * 8]
%else
    vpbroadcastq        m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova                m3, [h4_interp8_hpp_shuf]
    vbroadcasti128      m2, [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 2
    test                r5d, r5d
    mov                 r4d, %1
    jz                  .loop0
    sub                 r0, r1
    add                 r4d, 3

.loop0:
    vbroadcasti128      m4, [r0]
    vbroadcasti128      m5, [r0 + 8]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2], xm4

    vbroadcasti128      m4, [r0 + 16]
    vbroadcasti128      m5, [r0 + 24]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 16], xm4

    vbroadcasti128      m4, [r0 + 32]
    vbroadcasti128      m5, [r0 + 40]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 32], xm4

    vbroadcasti128      m4, [r0 + 48]
    vbroadcasti128      m5, [r0 + 56]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 48], xm4

    vbroadcasti128      m4, [r0 + 64]
    vbroadcasti128      m5, [r0 + 72]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 64], xm4

    vbroadcasti128      m4, [r0 + 80]
    vbroadcasti128      m5, [r0 + 88]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 80], xm4

    vbroadcasti128      m4, [r0 + 96]
    vbroadcasti128      m5, [r0 + 104]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 96], xm4

    vbroadcasti128      m4, [r0 + 112]
    vbroadcasti128      m5, [r0 + 120]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 112], xm4

    add                 r2, r3
    add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif
%endmacro

IPFILTER_CHROMA_PS_64xN_AVX2 64
IPFILTER_CHROMA_PS_64xN_AVX2 48
IPFILTER_CHROMA_PS_64xN_AVX2 32
IPFILTER_CHROMA_PS_64xN_AVX2 16

INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_horiz_ps_48x64, 4, 7, 6
    add                 r1d, r1d
    add                 r3d, r3d
    mov                 r4d, r4m
    mov                 r5d, r5m

%ifdef PIC
    lea                 r6, [h4_tab_ChromaCoeff]
    vpbroadcastq        m0, [r6 + r4 * 8]
%else
    vpbroadcastq        m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova                m3, [h4_interp8_hpp_shuf]
    vbroadcasti128      m2, [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 2
    test                r5d, r5d
    mov                 r4d, 64
    jz                  .loop0
    sub                 r0, r1
    add                 r4d, 3

.loop0:
    vbroadcasti128      m4, [r0]
    vbroadcasti128      m5, [r0 + 8]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2], xm4

    vbroadcasti128      m4, [r0 + 16]
    vbroadcasti128      m5, [r0 + 24]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 16], xm4

    vbroadcasti128      m4, [r0 + 32]
    vbroadcasti128      m5, [r0 + 40]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 32], xm4

    vbroadcasti128      m4, [r0 + 48]
    vbroadcasti128      m5, [r0 + 56]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 48], xm4

    vbroadcasti128      m4, [r0 + 64]
    vbroadcasti128      m5, [r0 + 72]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 64], xm4

    vbroadcasti128      m4, [r0 + 80]
    vbroadcasti128      m5, [r0 + 88]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movu                [r2 + 80], xm4

    add                 r2, r3
    add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif

%macro IPFILTER_CHROMA_PS_6xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_horiz_ps_6x%1, 4, 7, 6
    add                 r1d, r1d
    add                 r3d, r3d
    mov                 r4d, r4m
    mov                 r5d, r5m

%ifdef PIC
    lea                 r6, [h4_tab_ChromaCoeff]
    vpbroadcastq        m0, [r6 + r4 * 8]
%else
    vpbroadcastq        m0, [h4_tab_ChromaCoeff + r4 * 8]
%endif
    mova                m3, [h4_interp8_hpp_shuf]
    vbroadcasti128      m2, [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 2
    test                r5d, r5d
    mov                 r4d, %1
    jz                  .loop0
    sub                 r0, r1
    add                 r4d, 3

.loop0:
    vbroadcasti128      m4, [r0]
    vbroadcasti128      m5, [r0 + 8]
    pshufb              m4, m3
    pshufb              m5, m3
    pmaddwd             m4, m0
    pmaddwd             m5, m0
    phaddd              m4, m5
    paddd               m4, m2
    vpermq              m4, m4, q3120
    psrad               m4, INTERP_SHIFT_PS
    vextracti128        xm5, m4, 1
    packssdw            xm4, xm5
    movq                [r2], xm4
    pextrd              [r2 + 8], xm4, 2
    add                 r2, r3
    add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif
%endmacro

    IPFILTER_CHROMA_PS_6xN_AVX2 8
    IPFILTER_CHROMA_PS_6xN_AVX2 16
