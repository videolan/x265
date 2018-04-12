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

const v4_pd_526336, times 8 dd 8192*64+2048

const tab_Vm,    db 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1
                 db 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3

const tab_Cm,    db 0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3, 0, 2, 1, 3

const interp_vert_shuf, times 2 db 0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8, 7, 9
                        times 2 db 4, 6, 5, 7, 6, 8, 7, 9, 8, 10, 9, 11, 10, 12, 11, 13

const v4_interp4_vpp_shuf, times 2 db 0, 4, 1, 5, 2, 6, 3, 7, 8, 12, 9, 13, 10, 14, 11, 15

const v4_interp4_vpp_shuf1, dd 0, 1, 1, 2, 2, 3, 3, 4
                         dd 2, 3, 3, 4, 4, 5, 5, 6

const v4_tab_ChromaCoeff, db  0, 64,  0,  0
                       db -2, 58, 10, -2
                       db -4, 54, 16, -2
                       db -6, 46, 28, -4
                       db -4, 36, 36, -4
                       db -4, 28, 46, -6
                       db -2, 16, 54, -4
                       db -2, 10, 58, -2

const tabw_ChromaCoeff, dw  0, 64,  0,  0
                        dw -2, 58, 10, -2
                        dw -4, 54, 16, -2
                        dw -6, 46, 28, -4
                        dw -4, 36, 36, -4
                        dw -4, 28, 46, -6
                        dw -2, 16, 54, -4
                        dw -2, 10, 58, -2

const tab_ChromaCoeffV, times 4 dw 0, 64
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

const tab_ChromaCoeff_V, times 8 db 0, 64
                         times 8 db 0,  0

                         times 8 db -2, 58
                         times 8 db 10, -2

                         times 8 db -4, 54
                         times 8 db 16, -2

                         times 8 db -6, 46
                         times 8 db 28, -4

                         times 8 db -4, 36
                         times 8 db 36, -4

                         times 8 db -4, 28
                         times 8 db 46, -6

                         times 8 db -2, 16
                         times 8 db 54, -4

                         times 8 db -2, 10
                         times 8 db 58, -2

const tab_ChromaCoeffVer_32,    times 16 db 0, 64
                                times 16 db 0, 0

                                times 16 db -2, 58
                                times 16 db 10, -2

                                times 16 db -4, 54
                                times 16 db 16, -2

                                times 16 db -6, 46
                                times 16 db 28, -4

                                times 16 db -4, 36
                                times 16 db 36, -4

                                times 16 db -4, 28
                                times 16 db 46, -6

                                times 16 db -2, 16
                                times 16 db 54, -4

                                times 16 db -2, 10
                                times 16 db 58, -2

const pw_ChromaCoeffV,  times 8 dw 0, 64
                        times 8 dw 0, 0

                        times 8 dw -2, 58
                        times 8 dw 10, -2

                        times 8 dw -4, 54
                        times 8 dw 16, -2

                        times 8 dw -6, 46
                        times 8 dw 28, -4

                        times 8 dw -4, 36
                        times 8 dw 36, -4

                        times 8 dw -4, 28
                        times 8 dw 46, -6

                        times 8 dw -2, 16
                        times 8 dw 54, -4

                        times 8 dw -2, 10
                        times 8 dw 58, -2

const v4_interp8_hps_shuf,     dd 0, 4, 1, 5, 2, 6, 3, 7

SECTION .text

cextern pw_32
cextern pw_512
cextern pw_2000

%macro  WORD_TO_DOUBLE 1
%if ARCH_X86_64
    punpcklbw   %1,     m8
%else
    punpcklbw   %1,     %1
    psrlw       %1,     8
%endif
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_vert_%1_2x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W2_H4_sse2 2
INIT_XMM sse2
%if ARCH_X86_64
cglobal interp_4tap_vert_%1_2x%2, 4, 6, 9
    pxor        m8,        m8
%else
cglobal interp_4tap_vert_%1_2x%2, 4, 6, 8
%endif
    mov         r4d,       r4m
    sub         r0,        r1

%ifidn %1,pp
    mova        m1,        [pw_32]
%elifidn %1,ps
    mova        m1,        [pw_2000]
    add         r3d,       r3d
%endif

%ifdef PIC
    lea         r5,        [tabw_ChromaCoeff]
    movh        m0,        [r5 + r4 * 8]
%else
    movh        m0,        [tabw_ChromaCoeff + r4 * 8]
%endif

    punpcklqdq  m0,        m0
    lea         r5,        [3 * r1]

%assign x 1
%rep %2/4
    movd        m2,        [r0]
    movd        m3,        [r0 + r1]
    movd        m4,        [r0 + 2 * r1]
    movd        m5,        [r0 + r5]

    punpcklbw   m2,        m3
    punpcklbw   m6,        m4,        m5
    punpcklwd   m2,        m6

    WORD_TO_DOUBLE         m2
    pmaddwd     m2,        m0

    lea         r0,        [r0 + 4 * r1]
    movd        m6,        [r0]

    punpcklbw   m3,        m4
    punpcklbw   m7,        m5,        m6
    punpcklwd   m3,        m7

    WORD_TO_DOUBLE         m3
    pmaddwd     m3,        m0

    packssdw    m2,        m3
    pshuflw     m3,        m2,          q2301
    pshufhw     m3,        m3,          q2301
    paddw       m2,        m3

    movd        m7,        [r0 + r1]

    punpcklbw   m4,        m5
    punpcklbw   m3,        m6,        m7
    punpcklwd   m4,        m3

    WORD_TO_DOUBLE         m4
    pmaddwd     m4,        m0

    movd        m3,        [r0 + 2 * r1]

    punpcklbw   m5,        m6
    punpcklbw   m7,        m3
    punpcklwd   m5,        m7

    WORD_TO_DOUBLE         m5
    pmaddwd     m5,        m0

    packssdw    m4,        m5
    pshuflw     m5,        m4,          q2301
    pshufhw     m5,        m5,          q2301
    paddw       m4,        m5

%ifidn %1,pp
    psrld       m2,        16
    psrld       m4,        16
    packssdw    m2,        m4
    paddw       m2,        m1
    psraw       m2,        6
    packuswb    m2,        m2

%if ARCH_X86_64
    movq        r4,        m2
    mov         [r2],      r4w
    shr         r4,        16
    mov         [r2 + r3], r4w
    lea         r2,        [r2 + 2 * r3]
    shr         r4,        16
    mov         [r2],      r4w
    shr         r4,        16
    mov         [r2 + r3], r4w
%else
    movd        r4,        m2
    mov         [r2],      r4w
    shr         r4,        16
    mov         [r2 + r3], r4w
    lea         r2,        [r2 + 2 * r3]
    psrldq      m2,        4
    movd        r4,        m2
    mov         [r2],      r4w
    shr         r4,        16
    mov         [r2 + r3], r4w
%endif
%elifidn %1,ps
    psrldq      m2,        2
    psrldq      m4,        2
    pshufd      m2,        m2, q3120
    pshufd      m4,        m4, q3120
    psubw       m4,        m1
    psubw       m2,        m1

    movd        [r2],      m2
    psrldq      m2,        4
    movd        [r2 + r3], m2
    lea         r2,        [r2 + 2 * r3]
    movd        [r2],      m4
    psrldq      m4,        4
    movd        [r2 + r3], m4
%endif

%if x < %2/4
    lea         r2,        [r2 + 2 * r3]
%endif
%assign x x+1
%endrep
    RET

%endmacro

    FILTER_V4_W2_H4_sse2 pp, 4
    FILTER_V4_W2_H4_sse2 pp, 8
    FILTER_V4_W2_H4_sse2 pp, 16

    FILTER_V4_W2_H4_sse2 ps, 4
    FILTER_V4_W2_H4_sse2 ps, 8
    FILTER_V4_W2_H4_sse2 ps, 16

;-----------------------------------------------------------------------------
; void interp_4tap_vert_%1_4x2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro  FILTER_V2_W4_H4_sse2 1
INIT_XMM sse2
cglobal interp_4tap_vert_%1_4x2, 4, 6, 8
    mov         r4d,       r4m
    sub         r0,        r1
    pxor        m7,        m7

%ifdef PIC
    lea         r5,        [tabw_ChromaCoeff]
    movh        m0,        [r5 + r4 * 8]
%else
    movh        m0,        [tabw_ChromaCoeff + r4 * 8]
%endif

    lea         r5,        [r0 + 2 * r1]
    punpcklqdq  m0,        m0
    movd        m2,        [r0]
    movd        m3,        [r0 + r1]
    movd        m4,        [r5]
    movd        m5,        [r5 + r1]

    punpcklbw   m2,        m3
    punpcklbw   m1,        m4,        m5
    punpcklwd   m2,        m1

    movhlps     m6,        m2
    punpcklbw   m2,        m7
    punpcklbw   m6,        m7
    pmaddwd     m2,        m0
    pmaddwd     m6,        m0
    packssdw    m2,        m6

    movd        m1,        [r0 + 4 * r1]

    punpcklbw   m3,        m4
    punpcklbw   m5,        m1
    punpcklwd   m3,        m5

    movhlps     m6,        m3
    punpcklbw   m3,        m7
    punpcklbw   m6,        m7
    pmaddwd     m3,        m0
    pmaddwd     m6,        m0
    packssdw    m3,        m6

    pshuflw     m4,        m2,        q2301
    pshufhw     m4,        m4,        q2301
    paddw       m2,        m4
    pshuflw     m5,        m3,        q2301
    pshufhw     m5,        m5,        q2301
    paddw       m3,        m5

%ifidn %1, pp
    psrld       m2,        16
    psrld       m3,        16
    packssdw    m2,        m3

    paddw       m2,        [pw_32]
    psraw       m2,        6
    packuswb    m2,        m2

    movd        [r2],      m2
    psrldq      m2,        4
    movd        [r2 + r3], m2
%elifidn %1, ps
    psrldq      m2,        2
    psrldq      m3,        2
    pshufd      m2,        m2, q3120
    pshufd      m3,        m3, q3120
    punpcklqdq  m2, m3

    add         r3d,       r3d
    psubw       m2,        [pw_2000]
    movh        [r2],      m2
    movhps      [r2 + r3], m2
%endif
    RET

%endmacro

    FILTER_V2_W4_H4_sse2 pp
    FILTER_V2_W4_H4_sse2 ps

;-----------------------------------------------------------------------------
; void interp_4tap_vert_%1_4x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W4_H4_sse2 2
INIT_XMM sse2
%if ARCH_X86_64
cglobal interp_4tap_vert_%1_4x%2, 4, 6, 9
    pxor        m8,        m8
%else
cglobal interp_4tap_vert_%1_4x%2, 4, 6, 8
%endif

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [tabw_ChromaCoeff]
    movh        m0,        [r5 + r4 * 8]
%else
    movh        m0,        [tabw_ChromaCoeff + r4 * 8]
%endif

%ifidn %1,pp
    mova        m1,        [pw_32]
%elifidn %1,ps
    add         r3d,       r3d
    mova        m1,        [pw_2000]
%endif

    lea         r5,        [3 * r1]
    lea         r4,        [3 * r3]
    punpcklqdq  m0,        m0

%assign x 1
%rep %2/4
    movd        m2,        [r0]
    movd        m3,        [r0 + r1]
    movd        m4,        [r0 + 2 * r1]
    movd        m5,        [r0 + r5]

    punpcklbw   m2,        m3
    punpcklbw   m6,        m4,        m5
    punpcklwd   m2,        m6

    movhlps     m6,        m2
    WORD_TO_DOUBLE         m2
    WORD_TO_DOUBLE         m6
    pmaddwd     m2,        m0
    pmaddwd     m6,        m0
    packssdw    m2,        m6

    lea         r0,        [r0 + 4 * r1]
    movd        m6,        [r0]

    punpcklbw   m3,        m4
    punpcklbw   m7,        m5,        m6
    punpcklwd   m3,        m7

    movhlps     m7,        m3
    WORD_TO_DOUBLE         m3
    WORD_TO_DOUBLE         m7
    pmaddwd     m3,        m0
    pmaddwd     m7,        m0
    packssdw    m3,        m7

    pshuflw     m7,        m2,        q2301
    pshufhw     m7,        m7,        q2301
    paddw       m2,        m7
    pshuflw     m7,        m3,        q2301
    pshufhw     m7,        m7,        q2301
    paddw       m3,        m7

%ifidn %1,pp
    psrld       m2,        16
    psrld       m3,        16
    packssdw    m2,        m3
    paddw       m2,        m1
    psraw       m2,        6
%elifidn %1,ps
    psrldq      m2,        2
    psrldq      m3,        2
    pshufd      m2,        m2, q3120
    pshufd      m3,        m3, q3120
    punpcklqdq  m2,        m3

    psubw       m2,        m1
    movh        [r2],      m2
    movhps      [r2 + r3], m2
%endif

    movd        m7,        [r0 + r1]

    punpcklbw   m4,        m5
    punpcklbw   m3,        m6,        m7
    punpcklwd   m4,        m3

    movhlps     m3,        m4
    WORD_TO_DOUBLE         m4
    WORD_TO_DOUBLE         m3
    pmaddwd     m4,        m0
    pmaddwd     m3,        m0
    packssdw    m4,        m3

    movd        m3,        [r0 + 2 * r1]

    punpcklbw   m5,        m6
    punpcklbw   m7,        m3
    punpcklwd   m5,        m7

    movhlps     m3,        m5
    WORD_TO_DOUBLE         m5
    WORD_TO_DOUBLE         m3
    pmaddwd     m5,        m0
    pmaddwd     m3,        m0
    packssdw    m5,        m3

    pshuflw     m7,        m4,        q2301
    pshufhw     m7,        m7,        q2301
    paddw       m4,        m7
    pshuflw     m7,        m5,        q2301
    pshufhw     m7,        m7,        q2301
    paddw       m5,        m7

%ifidn %1,pp
    psrld       m4,        16
    psrld       m5,        16
    packssdw    m4,        m5

    paddw       m4,        m1
    psraw       m4,        6
    packuswb    m2,        m4

    movd        [r2],      m2
    psrldq      m2,        4
    movd        [r2 + r3], m2
    psrldq      m2,        4
    movd        [r2 + 2 * r3],      m2
    psrldq      m2,        4
    movd        [r2 + r4], m2
%elifidn %1,ps
    psrldq      m4,        2
    psrldq      m5,        2
    pshufd      m4,        m4, q3120
    pshufd      m5,        m5, q3120
    punpcklqdq  m4,        m5
    psubw       m4,        m1
    movh        [r2 + 2 * r3],      m4
    movhps      [r2 + r4], m4
%endif

%if x < %2/4
    lea         r2,        [r2 + 4 * r3]
%endif

%assign x x+1
%endrep
    RET

%endmacro

    FILTER_V4_W4_H4_sse2 pp, 4
    FILTER_V4_W4_H4_sse2 pp, 8
    FILTER_V4_W4_H4_sse2 pp, 16
    FILTER_V4_W4_H4_sse2 pp, 32

    FILTER_V4_W4_H4_sse2 ps, 4
    FILTER_V4_W4_H4_sse2 ps, 8
    FILTER_V4_W4_H4_sse2 ps, 16
    FILTER_V4_W4_H4_sse2 ps, 32

;-----------------------------------------------------------------------------
;void interp_4tap_vert_%1_6x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W6_H4_sse2 2
INIT_XMM sse2
cglobal interp_4tap_vert_%1_6x%2, 4, 7, 10
    mov         r4d,       r4m
    sub         r0,        r1
    shl         r4d,       5
    pxor        m9,        m9

%ifdef PIC
    lea         r5,        [tab_ChromaCoeffV]
    mova        m6,        [r5 + r4]
    mova        m5,        [r5 + r4 + 16]
%else
    mova        m6,        [tab_ChromaCoeffV + r4]
    mova        m5,        [tab_ChromaCoeffV + r4 + 16]
%endif

%ifidn %1,pp
    mova        m4,        [pw_32]
%elifidn %1,ps
    mova        m4,        [pw_2000]
    add         r3d,       r3d
%endif
    lea         r5,        [3 * r1]

%assign x 1
%rep %2/4
    movq        m0,        [r0]
    movq        m1,        [r0 + r1]
    movq        m2,        [r0 + 2 * r1]
    movq        m3,        [r0 + r5]

    punpcklbw   m0,        m1
    punpcklbw   m1,        m2
    punpcklbw   m2,        m3

    movhlps     m7,        m0
    punpcklbw   m0,        m9
    punpcklbw   m7,        m9
    pmaddwd     m0,        m6
    pmaddwd     m7,        m6
    packssdw    m0,        m7

    movhlps     m8,        m2
    movq        m7,        m2
    punpcklbw   m8,        m9
    punpcklbw   m7,        m9
    pmaddwd     m8,        m5
    pmaddwd     m7,        m5
    packssdw    m7,        m8

    paddw       m0,        m7

%ifidn %1,pp
    paddw       m0,        m4
    psraw       m0,        6
    packuswb    m0,        m0

    movd        [r2],      m0
    pextrw      r6d,       m0,        2
    mov         [r2 + 4],  r6w
%elifidn %1,ps
    psubw       m0,        m4
    movh        [r2],      m0
    pshufd      m0,        m0,        2
    movd        [r2 + 8],  m0
%endif

    lea         r0,        [r0 + 4 * r1]

    movq        m0,        [r0]
    punpcklbw   m3,        m0

    movhlps     m8,        m1
    punpcklbw   m1,        m9
    punpcklbw   m8,        m9
    pmaddwd     m1,        m6
    pmaddwd     m8,        m6
    packssdw    m1,        m8

    movhlps     m8,        m3
    movq        m7,        m3
    punpcklbw   m8,        m9
    punpcklbw   m7,        m9
    pmaddwd     m8,        m5
    pmaddwd     m7,        m5
    packssdw    m7,        m8

    paddw       m1,        m7

%ifidn %1,pp
    paddw       m1,        m4
    psraw       m1,        6
    packuswb    m1,        m1

    movd        [r2 + r3], m1
    pextrw      r6d,       m1,        2
    mov         [r2 + r3 + 4], r6w
%elifidn %1,ps
    psubw       m1,        m4
    movh        [r2 + r3], m1
    pshufd      m1,        m1,        2
    movd        [r2 + r3 + 8],  m1
%endif

    movq        m1,        [r0 + r1]
    punpcklbw   m7,        m0,        m1

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m6
    pmaddwd     m8,        m6
    packssdw    m2,        m8

    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m5
    pmaddwd     m8,        m5
    packssdw    m7,        m8

    paddw       m2,        m7
    lea         r2,        [r2 + 2 * r3]

%ifidn %1,pp
    paddw       m2,        m4
    psraw       m2,        6
    packuswb    m2,        m2
    movd        [r2],      m2
    pextrw      r6d,       m2,    2
    mov         [r2 + 4],  r6w
%elifidn %1,ps
    psubw       m2,        m4
    movh        [r2],      m2
    pshufd      m2,        m2,        2
    movd        [r2 + 8],  m2
%endif

    movq        m2,        [r0 + 2 * r1]
    punpcklbw   m1,        m2

    movhlps     m8,        m3
    punpcklbw   m3,        m9
    punpcklbw   m8,        m9
    pmaddwd     m3,        m6
    pmaddwd     m8,        m6
    packssdw    m3,        m8

    movhlps     m8,        m1
    punpcklbw   m1,        m9
    punpcklbw   m8,        m9
    pmaddwd     m1,        m5
    pmaddwd     m8,        m5
    packssdw    m1,        m8

    paddw       m3,        m1

%ifidn %1,pp
    paddw       m3,        m4
    psraw       m3,        6
    packuswb    m3,        m3

    movd        [r2 + r3], m3
    pextrw      r6d,       m3,    2
    mov         [r2 + r3 + 4], r6w
%elifidn %1,ps
    psubw       m3,        m4
    movh        [r2 + r3], m3
    pshufd      m3,        m3,        2
    movd        [r2 + r3 + 8],  m3
%endif

%if x < %2/4
    lea         r2,        [r2 + 2 * r3]
%endif

%assign x x+1
%endrep
    RET

%endmacro

%if ARCH_X86_64
    FILTER_V4_W6_H4_sse2 pp, 8
    FILTER_V4_W6_H4_sse2 pp, 16
    FILTER_V4_W6_H4_sse2 ps, 8
    FILTER_V4_W6_H4_sse2 ps, 16
%endif

;-----------------------------------------------------------------------------
; void interp_4tap_vert_%1_8x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W8_sse2 2
INIT_XMM sse2
cglobal interp_4tap_vert_%1_8x%2, 4, 7, 12
    mov         r4d,       r4m
    sub         r0,        r1
    shl         r4d,       5
    pxor        m9,        m9

%ifidn %1,pp
    mova        m4,        [pw_32]
%elifidn %1,ps
    mova        m4,        [pw_2000]
    add         r3d,       r3d
%endif

%ifdef PIC
    lea         r6,        [tab_ChromaCoeffV]
    mova        m6,        [r6 + r4]
    mova        m5,        [r6 + r4 + 16]
%else
    mova        m6,        [tab_ChromaCoeffV + r4]
    mova        m5,        [tab_ChromaCoeffV + r4 + 16]
%endif

    movq        m0,        [r0]
    movq        m1,        [r0 + r1]
    movq        m2,        [r0 + 2 * r1]
    lea         r5,        [r0 + 2 * r1]
    movq        m3,        [r5 + r1]

    punpcklbw   m0,        m1
    punpcklbw   m7,        m2,          m3

    movhlps     m8,        m0
    punpcklbw   m0,        m9
    punpcklbw   m8,        m9
    pmaddwd     m0,        m6
    pmaddwd     m8,        m6
    packssdw    m0,        m8

    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m5
    pmaddwd     m8,        m5
    packssdw    m7,        m8

    paddw       m0,        m7

%ifidn %1,pp
    paddw       m0,        m4
    psraw       m0,        6
%elifidn %1,ps
    psubw       m0,        m4
    movu        [r2],      m0
%endif

    movq        m11,        [r0 + 4 * r1]

    punpcklbw   m1,        m2
    punpcklbw   m7,        m3,        m11

    movhlps     m8,        m1
    punpcklbw   m1,        m9
    punpcklbw   m8,        m9
    pmaddwd     m1,        m6
    pmaddwd     m8,        m6
    packssdw    m1,        m8

    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m5
    pmaddwd     m8,        m5
    packssdw    m7,        m8

    paddw       m1,        m7

%ifidn %1,pp
    paddw       m1,        m4
    psraw       m1,        6
    packuswb    m1,        m0

    movhps      [r2],      m1
    movh        [r2 + r3], m1
%elifidn %1,ps
    psubw       m1,        m4
    movu        [r2 + r3], m1
%endif
%if %2 == 2     ;end of 8x2
    RET

%else
    lea         r6,        [r0 + 4 * r1]
    movq        m1,        [r6 + r1]

    punpcklbw   m2,        m3
    punpcklbw   m7,        m11,        m1

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m6
    pmaddwd     m8,        m6
    packssdw    m2,        m8

    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m5
    pmaddwd     m8,        m5
    packssdw    m7,        m8

    paddw       m2,        m7

%ifidn %1,pp
    paddw       m2,        m4
    psraw       m2,        6
%elifidn %1,ps
    psubw       m2,        m4
    movu        [r2 + 2 * r3], m2
%endif

    movq        m10,        [r6 + 2 * r1]

    punpcklbw   m3,        m11
    punpcklbw   m7,        m1,        m10

    movhlps     m8,        m3
    punpcklbw   m3,        m9
    punpcklbw   m8,        m9
    pmaddwd     m3,        m6
    pmaddwd     m8,        m6
    packssdw    m3,        m8

    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m5
    pmaddwd     m8,        m5
    packssdw    m7,        m8

    paddw       m3,        m7
    lea         r5,        [r2 + 2 * r3]

%ifidn %1,pp
    paddw       m3,        m4
    psraw       m3,        6
    packuswb    m3,        m2

    movhps      [r2 + 2 * r3], m3
    movh        [r5 + r3], m3
%elifidn %1,ps
    psubw       m3,        m4
    movu        [r5 + r3], m3
%endif
%if %2 == 4     ;end of 8x4
    RET

%else
    lea         r6,        [r6 + 2 * r1]
    movq        m3,        [r6 + r1]

    punpcklbw   m11,        m1
    punpcklbw   m7,        m10,        m3

    movhlps     m8,        m11
    punpcklbw   m11,        m9
    punpcklbw   m8,        m9
    pmaddwd     m11,        m6
    pmaddwd     m8,        m6
    packssdw    m11,        m8

    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m5
    pmaddwd     m8,        m5
    packssdw    m7,        m8

    paddw       m11,       m7

%ifidn %1, pp
    paddw       m11,       m4
    psraw       m11,       6
%elifidn %1,ps
    psubw       m11,       m4
    movu        [r2 + 4 * r3], m11
%endif

    movq        m7,        [r0 + 8 * r1]

    punpcklbw   m1,        m10
    punpcklbw   m3,        m7

    movhlps     m8,        m1
    punpcklbw   m1,        m9
    punpcklbw   m8,        m9
    pmaddwd     m1,        m6
    pmaddwd     m8,        m6
    packssdw    m1,        m8

    movhlps     m8,        m3
    punpcklbw   m3,        m9
    punpcklbw   m8,        m9
    pmaddwd     m3,        m5
    pmaddwd     m8,        m5
    packssdw    m3,        m8

    paddw       m1,        m3
    lea         r5,        [r2 + 4 * r3]

%ifidn %1,pp
    paddw       m1,        m4
    psraw       m1,        6
    packuswb    m1,        m11

    movhps      [r2 + 4 * r3], m1
    movh        [r5 + r3], m1
%elifidn %1,ps
    psubw       m1,        m4
    movu        [r5 + r3], m1
%endif
%if %2 == 6
    RET

%else
  %error INVALID macro argument, only 2, 4 or 6!
%endif
%endif
%endif
%endmacro

%if ARCH_X86_64
    FILTER_V4_W8_sse2 pp, 2
    FILTER_V4_W8_sse2 pp, 4
    FILTER_V4_W8_sse2 pp, 6
    FILTER_V4_W8_sse2 ps, 2
    FILTER_V4_W8_sse2 ps, 4
    FILTER_V4_W8_sse2 ps, 6
%endif

;-----------------------------------------------------------------------------
; void interp_4tap_vert_%1_8x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W8_H8_H16_H32_sse2 2
INIT_XMM sse2
cglobal interp_4tap_vert_%1_8x%2, 4, 6, 11
    mov         r4d,       r4m
    sub         r0,        r1
    shl         r4d,       5
    pxor        m9,        m9

%ifdef PIC
    lea         r5,        [tab_ChromaCoeffV]
    mova        m6,        [r5 + r4]
    mova        m5,        [r5 + r4 + 16]
%else
    mova        m6,        [v4_tab_ChromaCoeff + r4]
    mova        m5,        [v4_tab_ChromaCoeff + r4 + 16]
%endif

%ifidn %1,pp
    mova        m4,        [pw_32]
%elifidn %1,ps
    mova        m4,        [pw_2000]
    add         r3d,       r3d
%endif

    lea         r5,        [r1 * 3]

%assign x 1
%rep %2/4
    movq        m0,        [r0]
    movq        m1,        [r0 + r1]
    movq        m2,        [r0 + 2 * r1]
    movq        m3,        [r0 + r5]

    punpcklbw   m0,        m1
    punpcklbw   m1,        m2
    punpcklbw   m2,        m3

    movhlps     m7,        m0
    punpcklbw   m0,        m9
    punpcklbw   m7,        m9
    pmaddwd     m0,        m6
    pmaddwd     m7,        m6
    packssdw    m0,        m7

    movhlps     m8,        m2
    movq        m7,        m2
    punpcklbw   m8,        m9
    punpcklbw   m7,        m9
    pmaddwd     m8,        m5
    pmaddwd     m7,        m5
    packssdw    m7,        m8

    paddw       m0,        m7

%ifidn %1,pp
    paddw       m0,        m4
    psraw       m0,        6
%elifidn %1,ps
    psubw       m0,        m4
    movu        [r2],      m0
%endif

    lea         r0,        [r0 + 4 * r1]
    movq        m10,       [r0]
    punpcklbw   m3,        m10

    movhlps     m8,        m1
    punpcklbw   m1,        m9
    punpcklbw   m8,        m9
    pmaddwd     m1,        m6
    pmaddwd     m8,        m6
    packssdw    m1,        m8

    movhlps     m8,        m3
    movq        m7,        m3
    punpcklbw   m8,        m9
    punpcklbw   m7,        m9
    pmaddwd     m8,        m5
    pmaddwd     m7,        m5
    packssdw    m7,        m8

    paddw       m1,        m7

%ifidn %1,pp
    paddw       m1,        m4
    psraw       m1,        6

    packuswb    m0,        m1
    movh        [r2],      m0
    movhps      [r2 + r3], m0
%elifidn %1,ps
    psubw       m1,        m4
    movu        [r2 + r3], m1
%endif

    movq        m1,        [r0 + r1]
    punpcklbw   m10,       m1

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m6
    pmaddwd     m8,        m6
    packssdw    m2,        m8

    movhlps     m8,        m10
    punpcklbw   m10,       m9
    punpcklbw   m8,        m9
    pmaddwd     m10,       m5
    pmaddwd     m8,        m5
    packssdw    m10,       m8

    paddw       m2,        m10
    lea         r2,        [r2 + 2 * r3]

%ifidn %1,pp
    paddw       m2,        m4
    psraw       m2,        6
%elifidn %1,ps
    psubw       m2,        m4
    movu        [r2],      m2
%endif

    movq        m7,        [r0 + 2 * r1]
    punpcklbw   m1,        m7

    movhlps     m8,        m3
    punpcklbw   m3,        m9
    punpcklbw   m8,        m9
    pmaddwd     m3,        m6
    pmaddwd     m8,        m6
    packssdw    m3,        m8

    movhlps     m8,        m1
    punpcklbw   m1,        m9
    punpcklbw   m8,        m9
    pmaddwd     m1,        m5
    pmaddwd     m8,        m5
    packssdw    m1,        m8

    paddw       m3,        m1

%ifidn %1,pp
    paddw       m3,        m4
    psraw       m3,        6

    packuswb    m2,        m3
    movh        [r2],      m2
    movhps      [r2 + r3], m2
%elifidn %1,ps
    psubw       m3,        m4
    movu        [r2 + r3], m3
%endif

%if x < %2/4
    lea         r2,        [r2 + 2 * r3]
%endif
%endrep
    RET
%endmacro

%if ARCH_X86_64
    FILTER_V4_W8_H8_H16_H32_sse2 pp,  8
    FILTER_V4_W8_H8_H16_H32_sse2 pp, 16
    FILTER_V4_W8_H8_H16_H32_sse2 pp, 32

    FILTER_V4_W8_H8_H16_H32_sse2 pp, 12
    FILTER_V4_W8_H8_H16_H32_sse2 pp, 64

    FILTER_V4_W8_H8_H16_H32_sse2 ps,  8
    FILTER_V4_W8_H8_H16_H32_sse2 ps, 16
    FILTER_V4_W8_H8_H16_H32_sse2 ps, 32

    FILTER_V4_W8_H8_H16_H32_sse2 ps, 12
    FILTER_V4_W8_H8_H16_H32_sse2 ps, 64
%endif

;-----------------------------------------------------------------------------
; void interp_4tap_vert_%1_12x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W12_H2_sse2 2
INIT_XMM sse2
cglobal interp_4tap_vert_%1_12x%2, 4, 6, 11
    mov         r4d,       r4m
    sub         r0,        r1
    shl         r4d,       5
    pxor        m9,        m9

%ifidn %1,pp
    mova        m6,        [pw_32]
%elifidn %1,ps
    mova        m6,        [pw_2000]
    add         r3d,       r3d
%endif

%ifdef PIC
    lea         r5,        [tab_ChromaCoeffV]
    mova        m1,        [r5 + r4]
    mova        m0,        [r5 + r4 + 16]
%else
    mova        m1,        [tab_ChromaCoeffV + r4]
    mova        m0,        [tab_ChromaCoeffV + r4 + 16]
%endif

%assign x 1
%rep %2/2
    movu        m2,        [r0]
    movu        m3,        [r0 + r1]

    punpcklbw   m4,        m2,        m3
    punpckhbw   m2,        m3

    movhlps     m8,        m4
    punpcklbw   m4,        m9
    punpcklbw   m8,        m9
    pmaddwd     m4,        m1
    pmaddwd     m8,        m1
    packssdw    m4,        m8

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m1
    pmaddwd     m8,        m1
    packssdw    m2,        m8

    lea         r0,        [r0 + 2 * r1]
    movu        m5,        [r0]
    movu        m7,        [r0 + r1]

    punpcklbw   m10,       m5,        m7
    movhlps     m8,        m10
    punpcklbw   m10,       m9
    punpcklbw   m8,        m9
    pmaddwd     m10,       m0
    pmaddwd     m8,        m0
    packssdw    m10,       m8

    paddw       m4,        m10

    punpckhbw   m10,       m5,        m7
    movhlps     m8,        m10
    punpcklbw   m10,       m9
    punpcklbw   m8,        m9
    pmaddwd     m10,       m0
    pmaddwd     m8,        m0
    packssdw    m10,       m8

    paddw       m2,        m10

%ifidn %1,pp
    paddw       m4,        m6
    psraw       m4,        6
    paddw       m2,        m6
    psraw       m2,        6

    packuswb    m4,        m2
    movh        [r2],      m4
    psrldq      m4,        8
    movd        [r2 + 8],  m4
%elifidn %1,ps
    psubw       m4,        m6
    psubw       m2,        m6
    movu        [r2],      m4
    movh        [r2 + 16], m2
%endif

    punpcklbw   m4,        m3,        m5
    punpckhbw   m3,        m5

    movhlps     m8,        m4
    punpcklbw   m4,        m9
    punpcklbw   m8,        m9
    pmaddwd     m4,        m1
    pmaddwd     m8,        m1
    packssdw    m4,        m8

    movhlps     m8,        m4
    punpcklbw   m3,        m9
    punpcklbw   m8,        m9
    pmaddwd     m3,        m1
    pmaddwd     m8,        m1
    packssdw    m3,        m8

    movu        m5,        [r0 + 2 * r1]
    punpcklbw   m2,        m7,        m5
    punpckhbw   m7,        m5

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m0
    pmaddwd     m8,        m0
    packssdw    m2,        m8

    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m0
    pmaddwd     m8,        m0
    packssdw    m7,        m8

    paddw       m4,        m2
    paddw       m3,        m7

%ifidn %1,pp
    paddw       m4,        m6
    psraw       m4,        6
    paddw       m3,        m6
    psraw       m3,        6

    packuswb    m4,        m3
    movh        [r2 + r3], m4
    psrldq      m4,        8
    movd        [r2 + r3 + 8], m4
%elifidn %1,ps
    psubw       m4,        m6
    psubw       m3,        m6
    movu        [r2 + r3], m4
    movh        [r2 + r3 + 16], m3
%endif

%if x < %2/2
    lea         r2,        [r2 + 2 * r3]
%endif
%assign x x+1
%endrep
    RET

%endmacro

%if ARCH_X86_64
    FILTER_V4_W12_H2_sse2 pp, 16
    FILTER_V4_W12_H2_sse2 pp, 32
    FILTER_V4_W12_H2_sse2 ps, 16
    FILTER_V4_W12_H2_sse2 ps, 32
%endif

;-----------------------------------------------------------------------------
; void interp_4tap_vert_%1_16x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W16_H2_sse2 2
INIT_XMM sse2
cglobal interp_4tap_vert_%1_16x%2, 4, 6, 11
    mov         r4d,       r4m
    sub         r0,        r1
    shl         r4d,       5
    pxor        m9,        m9

%ifidn %1,pp
    mova        m6,        [pw_32]
%elifidn %1,ps
    mova        m6,        [pw_2000]
    add         r3d,       r3d
%endif

%ifdef PIC
    lea         r5,        [tab_ChromaCoeffV]
    mova        m1,        [r5 + r4]
    mova        m0,        [r5 + r4 + 16]
%else
    mova        m1,        [tab_ChromaCoeffV + r4]
    mova        m0,        [tab_ChromaCoeffV + r4 + 16]
%endif

%assign x 1
%rep %2/2
    movu        m2,        [r0]
    movu        m3,        [r0 + r1]

    punpcklbw   m4,        m2,        m3
    punpckhbw   m2,        m3

    movhlps     m8,        m4
    punpcklbw   m4,        m9
    punpcklbw   m8,        m9
    pmaddwd     m4,        m1
    pmaddwd     m8,        m1
    packssdw    m4,        m8

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m1
    pmaddwd     m8,        m1
    packssdw    m2,        m8

    lea         r0,        [r0 + 2 * r1]
    movu        m5,        [r0]
    movu        m10,       [r0 + r1]

    punpckhbw   m7,        m5,        m10
    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m0
    pmaddwd     m8,        m0
    packssdw    m7,        m8
    paddw       m2,        m7

    punpcklbw   m7,        m5,        m10
    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m0
    pmaddwd     m8,        m0
    packssdw    m7,        m8
    paddw       m4,        m7

%ifidn %1,pp
    paddw       m4,        m6
    psraw       m4,        6
    paddw       m2,        m6
    psraw       m2,        6

    packuswb    m4,        m2
    movu        [r2],      m4
%elifidn %1,ps
    psubw       m4,        m6
    psubw       m2,        m6
    movu        [r2],      m4
    movu        [r2 + 16], m2
%endif

    punpcklbw   m4,        m3,        m5
    punpckhbw   m3,        m5

    movhlps     m8,        m4
    punpcklbw   m4,        m9
    punpcklbw   m8,        m9
    pmaddwd     m4,        m1
    pmaddwd     m8,        m1
    packssdw    m4,        m8

    movhlps     m8,        m3
    punpcklbw   m3,        m9
    punpcklbw   m8,        m9
    pmaddwd     m3,        m1
    pmaddwd     m8,        m1
    packssdw    m3,        m8

    movu        m5,        [r0 + 2 * r1]

    punpcklbw   m2,        m10,       m5
    punpckhbw   m10,       m5

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m0
    pmaddwd     m8,        m0
    packssdw    m2,        m8

    movhlps     m8,        m10
    punpcklbw   m10,       m9
    punpcklbw   m8,        m9
    pmaddwd     m10,       m0
    pmaddwd     m8,        m0
    packssdw    m10,       m8

    paddw       m4,        m2
    paddw       m3,        m10

%ifidn %1,pp
    paddw       m4,        m6
    psraw       m4,        6
    paddw       m3,        m6
    psraw       m3,        6

    packuswb    m4,        m3
    movu        [r2 + r3], m4
%elifidn %1,ps
    psubw       m4,        m6
    psubw       m3,        m6
    movu        [r2 + r3], m4
    movu        [r2 + r3 + 16], m3
%endif

%if x < %2/2
    lea         r2,        [r2 + 2 * r3]
%endif
%assign x x+1
%endrep
    RET

%endmacro

%if ARCH_X86_64
    FILTER_V4_W16_H2_sse2 pp, 4
    FILTER_V4_W16_H2_sse2 pp, 8
    FILTER_V4_W16_H2_sse2 pp, 12
    FILTER_V4_W16_H2_sse2 pp, 16
    FILTER_V4_W16_H2_sse2 pp, 32

    FILTER_V4_W16_H2_sse2 pp, 24
    FILTER_V4_W16_H2_sse2 pp, 64

    FILTER_V4_W16_H2_sse2 ps, 4
    FILTER_V4_W16_H2_sse2 ps, 8
    FILTER_V4_W16_H2_sse2 ps, 12
    FILTER_V4_W16_H2_sse2 ps, 16
    FILTER_V4_W16_H2_sse2 ps, 32

    FILTER_V4_W16_H2_sse2 ps, 24
    FILTER_V4_W16_H2_sse2 ps, 64
%endif

;-----------------------------------------------------------------------------
;void interp_4tap_vert_%1_24%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W24_sse2 2
INIT_XMM sse2
cglobal interp_4tap_vert_%1_24x%2, 4, 6, 11
    mov         r4d,       r4m
    sub         r0,        r1
    shl         r4d,       5
    pxor        m9,        m9

%ifidn %1,pp
    mova        m6,        [pw_32]
%elifidn %1,ps
    mova        m6,        [pw_2000]
    add         r3d,       r3d
%endif

%ifdef PIC
    lea         r5,        [tab_ChromaCoeffV]
    mova        m1,        [r5 + r4]
    mova        m0,        [r5 + r4 + 16]
%else
    mova        m1,        [tab_ChromaCoeffV + r4]
    mova        m0,        [tab_ChromaCoeffV + r4 + 16]
%endif

%assign x 1
%rep %2/2
    movu        m2,        [r0]
    movu        m3,        [r0 + r1]

    punpcklbw   m4,        m2,        m3
    punpckhbw   m2,        m3

    movhlps     m8,        m4
    punpcklbw   m4,        m9
    punpcklbw   m8,        m9
    pmaddwd     m4,        m1
    pmaddwd     m8,        m1
    packssdw    m4,        m8

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m1
    pmaddwd     m8,        m1
    packssdw    m2,        m8

    lea         r5,        [r0 + 2 * r1]
    movu        m5,        [r5]
    movu        m10,       [r5 + r1]
    punpcklbw   m7,        m5,        m10

    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m0
    pmaddwd     m8,        m0
    packssdw    m7,        m8
    paddw       m4,        m7

    punpckhbw   m7,        m5,        m10

    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m0
    pmaddwd     m8,        m0
    packssdw    m7,        m8

    paddw       m2,        m7

%ifidn %1,pp
    paddw       m4,        m6
    psraw       m4,        6
    paddw       m2,        m6
    psraw       m2,        6

    packuswb    m4,        m2
    movu        [r2],      m4
%elifidn %1,ps
    psubw       m4,        m6
    psubw       m2,        m6
    movu        [r2],      m4
    movu        [r2 + 16], m2
%endif

    punpcklbw   m4,        m3,        m5
    punpckhbw   m3,        m5

    movhlps     m8,        m4
    punpcklbw   m4,        m9
    punpcklbw   m8,        m9
    pmaddwd     m4,        m1
    pmaddwd     m8,        m1
    packssdw    m4,        m8

    movhlps     m8,        m3
    punpcklbw   m3,        m9
    punpcklbw   m8,        m9
    pmaddwd     m3,        m1
    pmaddwd     m8,        m1
    packssdw    m3,        m8

    movu        m2,        [r5 + 2 * r1]

    punpcklbw   m5,        m10,        m2
    punpckhbw   m10,       m2

    movhlps     m8,        m5
    punpcklbw   m5,        m9
    punpcklbw   m8,        m9
    pmaddwd     m5,        m0
    pmaddwd     m8,        m0
    packssdw    m5,        m8

    movhlps     m8,        m10
    punpcklbw   m10,       m9
    punpcklbw   m8,        m9
    pmaddwd     m10,       m0
    pmaddwd     m8,        m0
    packssdw    m10,       m8

    paddw       m4,        m5
    paddw       m3,        m10

%ifidn %1,pp
    paddw       m4,        m6
    psraw       m4,        6
    paddw       m3,        m6
    psraw       m3,        6

    packuswb    m4,        m3
    movu        [r2 + r3], m4
%elifidn %1,ps
    psubw       m4,        m6
    psubw       m3,        m6
    movu        [r2 + r3], m4
    movu        [r2 + r3 + 16], m3
%endif

    movq        m2,        [r0 + 16]
    movq        m3,        [r0 + r1 + 16]
    movq        m4,        [r5 + 16]
    movq        m5,        [r5 + r1 + 16]

    punpcklbw   m2,        m3
    punpcklbw   m4,        m5

    movhlps     m8,        m4
    punpcklbw   m4,        m9
    punpcklbw   m8,        m9
    pmaddwd     m4,        m0
    pmaddwd     m8,        m0
    packssdw    m4,        m8

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m1
    pmaddwd     m8,        m1
    packssdw    m2,        m8

    paddw       m2,        m4

%ifidn %1,pp
    paddw       m2,        m6
    psraw       m2,        6
%elifidn %1,ps
    psubw       m2,        m6
    movu        [r2 + 32], m2
%endif

    movq        m3,        [r0 + r1 + 16]
    movq        m4,        [r5 + 16]
    movq        m5,        [r5 + r1 + 16]
    movq        m7,        [r5 + 2 * r1 + 16]

    punpcklbw   m3,        m4
    punpcklbw   m5,        m7

    movhlps     m8,        m5
    punpcklbw   m5,        m9
    punpcklbw   m8,        m9
    pmaddwd     m5,        m0
    pmaddwd     m8,        m0
    packssdw    m5,        m8

    movhlps     m8,        m3
    punpcklbw   m3,        m9
    punpcklbw   m8,        m9
    pmaddwd     m3,        m1
    pmaddwd     m8,        m1
    packssdw    m3,        m8

    paddw       m3,        m5

%ifidn %1,pp
    paddw       m3,        m6
    psraw       m3,        6

    packuswb    m2,        m3
    movh        [r2 + 16], m2
    movhps      [r2 + r3 + 16], m2
%elifidn %1,ps
    psubw       m3,        m6
    movu        [r2 + r3 + 32], m3
%endif

%if x < %2/2
    mov         r0,        r5
    lea         r2,        [r2 + 2 * r3]
%endif
%assign x x+1
%endrep
    RET

%endmacro

%if ARCH_X86_64
    FILTER_V4_W24_sse2 pp, 32
    FILTER_V4_W24_sse2 pp, 64
    FILTER_V4_W24_sse2 ps, 32
    FILTER_V4_W24_sse2 ps, 64
%endif

;-----------------------------------------------------------------------------
; void interp_4tap_vert_%1_32x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W32_sse2 2
INIT_XMM sse2
cglobal interp_4tap_vert_%1_32x%2, 4, 6, 10
    mov         r4d,       r4m
    sub         r0,        r1
    shl         r4d,       5
    pxor        m9,        m9

%ifidn %1,pp
    mova        m6,        [pw_32]
%elifidn %1,ps
    mova        m6,        [pw_2000]
    add         r3d,       r3d
%endif

%ifdef PIC
    lea         r5,        [tab_ChromaCoeffV]
    mova        m1,        [r5 + r4]
    mova        m0,        [r5 + r4 + 16]
%else
    mova        m1,        [tab_ChromaCoeffV + r4]
    mova        m0,        [tab_ChromaCoeffV + r4 + 16]
%endif

    mov         r4d,       %2

.loop:
    movu        m2,        [r0]
    movu        m3,        [r0 + r1]

    punpcklbw   m4,        m2,        m3
    punpckhbw   m2,        m3

    movhlps     m8,        m4
    punpcklbw   m4,        m9
    punpcklbw   m8,        m9
    pmaddwd     m4,        m1
    pmaddwd     m8,        m1
    packssdw    m4,        m8

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m1
    pmaddwd     m8,        m1
    packssdw    m2,        m8

    lea         r5,        [r0 + 2 * r1]
    movu        m3,        [r5]
    movu        m5,        [r5 + r1]

    punpcklbw   m7,        m3,        m5
    punpckhbw   m3,        m5

    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m0
    pmaddwd     m8,        m0
    packssdw    m7,        m8

    movhlps     m8,        m3
    punpcklbw   m3,        m9
    punpcklbw   m8,        m9
    pmaddwd     m3,        m0
    pmaddwd     m8,        m0
    packssdw    m3,        m8

    paddw       m4,        m7
    paddw       m2,        m3

%ifidn %1,pp
    paddw       m4,        m6
    psraw       m4,        6
    paddw       m2,        m6
    psraw       m2,        6

    packuswb    m4,        m2
    movu        [r2],      m4
%elifidn %1,ps
    psubw       m4,        m6
    psubw       m2,        m6
    movu        [r2],      m4
    movu        [r2 + 16], m2
%endif

    movu        m2,        [r0 + 16]
    movu        m3,        [r0 + r1 + 16]

    punpcklbw   m4,        m2,        m3
    punpckhbw   m2,        m3

    movhlps     m8,        m4
    punpcklbw   m4,        m9
    punpcklbw   m8,        m9
    pmaddwd     m4,        m1
    pmaddwd     m8,        m1
    packssdw    m4,        m8

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m1
    pmaddwd     m8,        m1
    packssdw    m2,        m8

    movu        m3,        [r5 + 16]
    movu        m5,        [r5 + r1 + 16]

    punpcklbw   m7,        m3,        m5
    punpckhbw   m3,        m5

    movhlps     m8,        m7
    punpcklbw   m7,        m9
    punpcklbw   m8,        m9
    pmaddwd     m7,        m0
    pmaddwd     m8,        m0
    packssdw    m7,        m8

    movhlps     m8,        m3
    punpcklbw   m3,        m9
    punpcklbw   m8,        m9
    pmaddwd     m3,        m0
    pmaddwd     m8,        m0
    packssdw    m3,        m8

    paddw       m4,        m7
    paddw       m2,        m3

%ifidn %1,pp
    paddw       m4,        m6
    psraw       m4,        6
    paddw       m2,        m6
    psraw       m2,        6

    packuswb    m4,        m2
    movu        [r2 + 16], m4
%elifidn %1,ps
    psubw       m4,        m6
    psubw       m2,        m6
    movu        [r2 + 32], m4
    movu        [r2 + 48], m2
%endif

    lea         r0,        [r0 + r1]
    lea         r2,        [r2 + r3]
    dec         r4
    jnz        .loop
    RET

%endmacro

%if ARCH_X86_64
    FILTER_V4_W32_sse2 pp, 8
    FILTER_V4_W32_sse2 pp, 16
    FILTER_V4_W32_sse2 pp, 24
    FILTER_V4_W32_sse2 pp, 32

    FILTER_V4_W32_sse2 pp, 48
    FILTER_V4_W32_sse2 pp, 64

    FILTER_V4_W32_sse2 ps, 8
    FILTER_V4_W32_sse2 ps, 16
    FILTER_V4_W32_sse2 ps, 24
    FILTER_V4_W32_sse2 ps, 32

    FILTER_V4_W32_sse2 ps, 48
    FILTER_V4_W32_sse2 ps, 64
%endif

;-----------------------------------------------------------------------------
; void interp_4tap_vert_%1_%2x%3(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W16n_H2_sse2 3
INIT_XMM sse2
cglobal interp_4tap_vert_%1_%2x%3, 4, 7, 11
    mov         r4d,       r4m
    sub         r0,        r1
    shl         r4d,       5
    pxor        m9,        m9

%ifidn %1,pp
    mova        m7,        [pw_32]
%elifidn %1,ps
    mova        m7,        [pw_2000]
    add         r3d,       r3d
%endif

%ifdef PIC
    lea         r5,        [tab_ChromaCoeffV]
    mova        m1,        [r5 + r4]
    mova        m0,        [r5 + r4 + 16]
%else
    mova        m1,        [tab_ChromaCoeffV + r4]
    mova        m0,        [tab_ChromaCoeffV + r4 + 16]
%endif

    mov         r4d,       %3/2

.loop:

    mov         r6d,       %2/16

.loopW:

    movu        m2,        [r0]
    movu        m3,        [r0 + r1]

    punpcklbw   m4,        m2,        m3
    punpckhbw   m2,        m3

    movhlps     m8,        m4
    punpcklbw   m4,        m9
    punpcklbw   m8,        m9
    pmaddwd     m4,        m1
    pmaddwd     m8,        m1
    packssdw    m4,        m8

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m1
    pmaddwd     m8,        m1
    packssdw    m2,        m8

    lea         r5,        [r0 + 2 * r1]
    movu        m5,        [r5]
    movu        m6,        [r5 + r1]

    punpckhbw   m10,        m5,        m6
    movhlps     m8,        m10
    punpcklbw   m10,       m9
    punpcklbw   m8,        m9
    pmaddwd     m10,       m0
    pmaddwd     m8,        m0
    packssdw    m10,       m8
    paddw       m2,        m10

    punpcklbw   m10,        m5,        m6
    movhlps     m8,        m10
    punpcklbw   m10,       m9
    punpcklbw   m8,        m9
    pmaddwd     m10,       m0
    pmaddwd     m8,        m0
    packssdw    m10,       m8
    paddw       m4,        m10

%ifidn %1,pp
    paddw       m4,        m7
    psraw       m4,        6
    paddw       m2,        m7
    psraw       m2,        6

    packuswb    m4,        m2
    movu        [r2],      m4
%elifidn %1,ps
    psubw       m4,        m7
    psubw       m2,        m7
    movu        [r2],      m4
    movu        [r2 + 16], m2
%endif

    punpcklbw   m4,        m3,        m5
    punpckhbw   m3,        m5

    movhlps     m8,        m4
    punpcklbw   m4,        m9
    punpcklbw   m8,        m9
    pmaddwd     m4,        m1
    pmaddwd     m8,        m1
    packssdw    m4,        m8

    movhlps     m8,        m3
    punpcklbw   m3,        m9
    punpcklbw   m8,        m9
    pmaddwd     m3,        m1
    pmaddwd     m8,        m1
    packssdw    m3,        m8

    movu        m5,        [r5 + 2 * r1]

    punpcklbw   m2,        m6,        m5
    punpckhbw   m6,        m5

    movhlps     m8,        m2
    punpcklbw   m2,        m9
    punpcklbw   m8,        m9
    pmaddwd     m2,        m0
    pmaddwd     m8,        m0
    packssdw    m2,        m8

    movhlps     m8,        m6
    punpcklbw   m6,        m9
    punpcklbw   m8,        m9
    pmaddwd     m6,        m0
    pmaddwd     m8,        m0
    packssdw    m6,        m8

    paddw       m4,        m2
    paddw       m3,        m6

%ifidn %1,pp
    paddw       m4,        m7
    psraw       m4,        6
    paddw       m3,        m7
    psraw       m3,        6

    packuswb    m4,        m3
    movu        [r2 + r3], m4
    add         r2,        16
%elifidn %1,ps
    psubw       m4,        m7
    psubw       m3,        m7
    movu        [r2 + r3], m4
    movu        [r2 + r3 + 16], m3
    add         r2,        32
%endif

    add         r0,        16
    dec         r6d
    jnz         .loopW

    lea         r0,        [r0 + r1 * 2 - %2]

%ifidn %1,pp
    lea         r2,        [r2 + r3 * 2 - %2]
%elifidn %1,ps
    lea         r2,        [r2 + r3 * 2 - (%2 * 2)]
%endif

    dec         r4d
    jnz        .loop
    RET

%endmacro

%if ARCH_X86_64
    FILTER_V4_W16n_H2_sse2 pp, 64, 64
    FILTER_V4_W16n_H2_sse2 pp, 64, 32
    FILTER_V4_W16n_H2_sse2 pp, 64, 48
    FILTER_V4_W16n_H2_sse2 pp, 48, 64
    FILTER_V4_W16n_H2_sse2 pp, 64, 16
    FILTER_V4_W16n_H2_sse2 ps, 64, 64
    FILTER_V4_W16n_H2_sse2 ps, 64, 32
    FILTER_V4_W16n_H2_sse2 ps, 64, 48
    FILTER_V4_W16n_H2_sse2 ps, 48, 64
    FILTER_V4_W16n_H2_sse2 ps, 64, 16
%endif

;-----------------------------------------------------------------------------
;void interp_4tap_vert_pp_2x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_pp_2x4, 4, 6, 8

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [v4_tab_ChromaCoeff]
    movd        m0,        [r5 + r4 * 4]
%else
    movd        m0,        [v4_tab_ChromaCoeff + r4 * 4]
%endif
    lea         r4,        [r1 * 3]
    lea         r5,        [r0 + 4 * r1]
    pshufb      m0,        [tab_Cm]
    mova        m1,        [pw_512]

    movd        m2,        [r0]
    movd        m3,        [r0 + r1]
    movd        m4,        [r0 + 2 * r1]
    movd        m5,        [r0 + r4]

    punpcklbw   m2,        m3
    punpcklbw   m6,        m4,        m5
    punpcklbw   m2,        m6

    pmaddubsw   m2,        m0

    movd        m6,        [r5]

    punpcklbw   m3,        m4
    punpcklbw   m7,        m5,        m6
    punpcklbw   m3,        m7

    pmaddubsw   m3,        m0

    phaddw      m2,        m3

    pmulhrsw    m2,        m1

    movd        m7,        [r5 + r1]

    punpcklbw   m4,        m5
    punpcklbw   m3,        m6,        m7
    punpcklbw   m4,        m3

    pmaddubsw   m4,        m0

    movd        m3,        [r5 + 2 * r1]

    punpcklbw   m5,        m6
    punpcklbw   m7,        m3
    punpcklbw   m5,        m7

    pmaddubsw   m5,        m0

    phaddw      m4,        m5

    pmulhrsw    m4,        m1
    packuswb    m2,        m4

    pextrw      [r2],      m2, 0
    pextrw      [r2 + r3], m2, 2
    lea         r2,        [r2 + 2 * r3]
    pextrw      [r2],      m2, 4
    pextrw      [r2 + r3], m2, 6

    RET

%macro FILTER_VER_CHROMA_AVX2_2x4 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_2x4, 4, 6, 2
    mov             r4d, r4m
    shl             r4d, 5
    sub             r0, r1

%ifdef PIC
    lea             r5, [tab_ChromaCoeff_V]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeff_V + r4]
%endif

    lea             r4, [r1 * 3]

    pinsrw          xm1, [r0], 0
    pinsrw          xm1, [r0 + r1], 1
    pinsrw          xm1, [r0 + r1 * 2], 2
    pinsrw          xm1, [r0 + r4], 3
    lea             r0, [r0 + r1 * 4]
    pinsrw          xm1, [r0], 4
    pinsrw          xm1, [r0 + r1], 5
    pinsrw          xm1, [r0 + r1 * 2], 6

    pshufb          xm0, xm1, [interp_vert_shuf]
    pshufb          xm1, [interp_vert_shuf + 32]
    vinserti128     m0, m0, xm1, 1
    pmaddubsw       m0, [r5]
    vextracti128    xm1, m0, 1
    paddw           xm0, xm1
%ifidn %1,pp
    pmulhrsw        xm0, [pw_512]
    packuswb        xm0, xm0
    lea             r4, [r3 * 3]
    pextrw          [r2], xm0, 0
    pextrw          [r2 + r3], xm0, 1
    pextrw          [r2 + r3 * 2], xm0, 2
    pextrw          [r2 + r4], xm0, 3
%else
    add             r3d, r3d
    lea             r4, [r3 * 3]
    psubw           xm0, [pw_2000]
    movd            [r2], xm0
    pextrd          [r2 + r3], xm0, 1
    pextrd          [r2 + r3 * 2], xm0, 2
    pextrd          [r2 + r4], xm0, 3
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_2x4 pp
    FILTER_VER_CHROMA_AVX2_2x4 ps

%macro FILTER_VER_CHROMA_AVX2_2x8 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_2x8, 4, 6, 2
    mov             r4d, r4m
    shl             r4d, 6
    sub             r0, r1

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]

    pinsrw          xm1, [r0], 0
    pinsrw          xm1, [r0 + r1], 1
    pinsrw          xm1, [r0 + r1 * 2], 2
    pinsrw          xm1, [r0 + r4], 3
    lea             r0, [r0 + r1 * 4]
    pinsrw          xm1, [r0], 4
    pinsrw          xm1, [r0 + r1], 5
    pinsrw          xm1, [r0 + r1 * 2], 6
    pinsrw          xm1, [r0 + r4], 7
    movhlps         xm0, xm1
    lea             r0, [r0 + r1 * 4]
    pinsrw          xm0, [r0], 4
    pinsrw          xm0, [r0 + r1], 5
    pinsrw          xm0, [r0 + r1 * 2], 6
    vinserti128     m1, m1, xm0, 1

    pshufb          m0, m1, [interp_vert_shuf]
    pshufb          m1, [interp_vert_shuf + 32]
    pmaddubsw       m0, [r5]
    pmaddubsw       m1, [r5 + 1 * mmsize]
    paddw           m0, m1
%ifidn %1,pp
    pmulhrsw        m0, [pw_512]
    vextracti128    xm1, m0, 1
    packuswb        xm0, xm1
    lea             r4, [r3 * 3]
    pextrw          [r2], xm0, 0
    pextrw          [r2 + r3], xm0, 1
    pextrw          [r2 + r3 * 2], xm0, 2
    pextrw          [r2 + r4], xm0, 3
    lea             r2, [r2 + r3 * 4]
    pextrw          [r2], xm0, 4
    pextrw          [r2 + r3], xm0, 5
    pextrw          [r2 + r3 * 2], xm0, 6
    pextrw          [r2 + r4], xm0, 7
%else
    add             r3d, r3d
    lea             r4, [r3 * 3]
    psubw           m0, [pw_2000]
    vextracti128    xm1, m0, 1
    movd            [r2], xm0
    pextrd          [r2 + r3], xm0, 1
    pextrd          [r2 + r3 * 2], xm0, 2
    pextrd          [r2 + r4], xm0, 3
    lea             r2, [r2 + r3 * 4]
    movd            [r2], xm1
    pextrd          [r2 + r3], xm1, 1
    pextrd          [r2 + r3 * 2], xm1, 2
    pextrd          [r2 + r4], xm1, 3
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_2x8 pp
    FILTER_VER_CHROMA_AVX2_2x8 ps

%macro FILTER_VER_CHROMA_AVX2_2x16 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_2x16, 4, 6, 3
    mov             r4d, r4m
    shl             r4d, 6
    sub             r0,  r1

%ifdef PIC
    lea             r5,  [tab_ChromaCoeffVer_32]
    add             r5,  r4
%else
    lea             r5,  [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4,  [r1 * 3]

    movd            xm1, [r0]
    pinsrw          xm1, [r0 + r1], 1
    pinsrw          xm1, [r0 + r1 * 2], 2
    pinsrw          xm1, [r0 + r4], 3
    lea             r0,  [r0 + r1 * 4]
    pinsrw          xm1, [r0], 4
    pinsrw          xm1, [r0 + r1], 5
    pinsrw          xm1, [r0 + r1 * 2], 6
    pinsrw          xm1, [r0 + r4], 7
    lea             r0,  [r0 + r1 * 4]
    pinsrw          xm0, [r0], 4
    pinsrw          xm0, [r0 + r1], 5
    pinsrw          xm0, [r0 + r1 * 2], 6
    pinsrw          xm0, [r0 + r4], 7
    punpckhqdq      xm0, xm1, xm0
    vinserti128     m1,  m1,  xm0,  1

    pshufb          m2,  m1,  [interp_vert_shuf]
    pshufb          m1,  [interp_vert_shuf + 32]
    pmaddubsw       m2,  [r5]
    pmaddubsw       m1,  [r5 + 1 * mmsize]
    paddw           m2,  m1

    lea             r0,  [r0 + r1 * 4]
    pinsrw          xm1, [r0], 4
    pinsrw          xm1, [r0 + r1], 5
    pinsrw          xm1, [r0 + r1 * 2], 6
    pinsrw          xm1, [r0 + r4], 7
    punpckhqdq      xm1, xm0, xm1
    lea             r0,  [r0 + r1 * 4]
    pinsrw          xm0, [r0], 4
    pinsrw          xm0, [r0 + r1], 5
    pinsrw          xm0, [r0 + r1 * 2], 6
    punpckhqdq      xm0, xm1, xm0
    vinserti128     m1,  m1,  xm0,  1

    pshufb          m0,  m1,  [interp_vert_shuf]
    pshufb          m1,  [interp_vert_shuf + 32]
    pmaddubsw       m0,  [r5]
    pmaddubsw       m1,  [r5 + 1 * mmsize]
    paddw           m0,  m1
%ifidn %1,pp
    mova            m1,  [pw_512]
    pmulhrsw        m2,  m1
    pmulhrsw        m0,  m1
    packuswb        m2,  m0
    lea             r4,  [r3 * 3]
    pextrw          [r2], xm2, 0
    pextrw          [r2 + r3], xm2, 1
    pextrw          [r2 + r3 * 2], xm2, 2
    pextrw          [r2 + r4], xm2, 3
    vextracti128    xm0, m2, 1
    lea             r2,  [r2 + r3 * 4]
    pextrw          [r2], xm0, 0
    pextrw          [r2 + r3], xm0, 1
    pextrw          [r2 + r3 * 2], xm0, 2
    pextrw          [r2 + r4], xm0, 3
    lea             r2,  [r2 + r3 * 4]
    pextrw          [r2], xm2, 4
    pextrw          [r2 + r3], xm2, 5
    pextrw          [r2 + r3 * 2], xm2, 6
    pextrw          [r2 + r4], xm2, 7
    lea             r2,  [r2 + r3 * 4]
    pextrw          [r2], xm0, 4
    pextrw          [r2 + r3], xm0, 5
    pextrw          [r2 + r3 * 2], xm0, 6
    pextrw          [r2 + r4], xm0, 7
%else
    add             r3d, r3d
    lea             r4,  [r3 * 3]
    vbroadcasti128  m1,  [pw_2000]
    psubw           m2,  m1
    psubw           m0,  m1
    vextracti128    xm1, m2, 1
    movd            [r2], xm2
    pextrd          [r2 + r3], xm2, 1
    pextrd          [r2 + r3 * 2], xm2, 2
    pextrd          [r2 + r4], xm2, 3
    lea             r2, [r2 + r3 * 4]
    movd            [r2], xm1
    pextrd          [r2 + r3], xm1, 1
    pextrd          [r2 + r3 * 2], xm1, 2
    pextrd          [r2 + r4], xm1, 3
    vextracti128    xm1, m0, 1
    lea             r2,  [r2 + r3 * 4]
    movd            [r2], xm0
    pextrd          [r2 + r3], xm0, 1
    pextrd          [r2 + r3 * 2], xm0, 2
    pextrd          [r2 + r4], xm0, 3
    lea             r2,  [r2 + r3 * 4]
    movd            [r2], xm1
    pextrd          [r2 + r3], xm1, 1
    pextrd          [r2 + r3 * 2], xm1, 2
    pextrd          [r2 + r4], xm1, 3
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_2x16 pp
    FILTER_VER_CHROMA_AVX2_2x16 ps

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_2x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W2_H4 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_2x%2, 4, 6, 8

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [v4_tab_ChromaCoeff]
    movd        m0,        [r5 + r4 * 4]
%else
    movd        m0,        [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m0,        [tab_Cm]

    mova        m1,        [pw_512]

    mov         r4d,       %2
    lea         r5,        [3 * r1]

.loop:
    movd        m2,        [r0]
    movd        m3,        [r0 + r1]
    movd        m4,        [r0 + 2 * r1]
    movd        m5,        [r0 + r5]

    punpcklbw   m2,        m3
    punpcklbw   m6,        m4,        m5
    punpcklbw   m2,        m6

    pmaddubsw   m2,        m0

    lea         r0,        [r0 + 4 * r1]
    movd        m6,        [r0]

    punpcklbw   m3,        m4
    punpcklbw   m7,        m5,        m6
    punpcklbw   m3,        m7

    pmaddubsw   m3,        m0

    phaddw      m2,        m3

    pmulhrsw    m2,        m1

    movd        m7,        [r0 + r1]

    punpcklbw   m4,        m5
    punpcklbw   m3,        m6,        m7
    punpcklbw   m4,        m3

    pmaddubsw   m4,        m0

    movd        m3,        [r0 + 2 * r1]

    punpcklbw   m5,        m6
    punpcklbw   m7,        m3
    punpcklbw   m5,        m7

    pmaddubsw   m5,        m0

    phaddw      m4,        m5

    pmulhrsw    m4,        m1
    packuswb    m2,        m4

    pextrw      [r2],      m2, 0
    pextrw      [r2 + r3], m2, 2
    lea         r2,        [r2 + 2 * r3]
    pextrw      [r2],      m2, 4
    pextrw      [r2 + r3], m2, 6

    lea         r2,        [r2 + 2 * r3]

    sub         r4,        4
    jnz        .loop
    RET
%endmacro

    FILTER_V4_W2_H4 2, 8
    FILTER_V4_W2_H4 2, 16

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_4x2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_pp_4x2, 4, 6, 6

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [v4_tab_ChromaCoeff]
    movd        m0,        [r5 + r4 * 4]
%else
    movd        m0,        [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m0,        [tab_Cm]
    lea         r5,        [r0 + 2 * r1]

    movd        m2,        [r0]
    movd        m3,        [r0 + r1]
    movd        m4,        [r5]
    movd        m5,        [r5 + r1]

    punpcklbw   m2,        m3
    punpcklbw   m1,        m4,        m5
    punpcklbw   m2,        m1

    pmaddubsw   m2,        m0

    movd        m1,        [r0 + 4 * r1]

    punpcklbw   m3,        m4
    punpcklbw   m5,        m1
    punpcklbw   m3,        m5

    pmaddubsw   m3,        m0

    phaddw      m2,        m3

    pmulhrsw    m2,        [pw_512]
    packuswb    m2,        m2
    movd        [r2],      m2
    pextrd      [r2 + r3], m2,  1

    RET

%macro FILTER_VER_CHROMA_AVX2_4x2 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x2, 4, 6, 4
    mov             r4d, r4m
    shl             r4d, 5
    sub             r0, r1

%ifdef PIC
    lea             r5, [tab_ChromaCoeff_V]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeff_V + r4]
%endif

    lea             r4, [r1 * 3]

    movd            xm1, [r0]
    movd            xm2, [r0 + r1]
    punpcklbw       xm1, xm2
    movd            xm3, [r0 + r1 * 2]
    punpcklbw       xm2, xm3
    movlhps         xm1, xm2
    movd            xm0, [r0 + r4]
    punpcklbw       xm3, xm0
    movd            xm2, [r0 + r1 * 4]
    punpcklbw       xm0, xm2
    movlhps         xm3, xm0
    vinserti128     m1, m1, xm3, 1                          ; m1 = row[x x x 4 3 2 1 0]

    pmaddubsw       m1, [r5]
    vextracti128    xm3, m1, 1
    paddw           xm1, xm3
%ifidn %1,pp
    pmulhrsw        xm1, [pw_512]
    packuswb        xm1, xm1
    movd            [r2], xm1
    pextrd          [r2 + r3], xm1, 1
%else
    add             r3d, r3d
    psubw           xm1, [pw_2000]
    movq            [r2], xm1
    movhps          [r2 + r3], xm1
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_4x2 pp
    FILTER_VER_CHROMA_AVX2_4x2 ps

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_4x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_pp_4x4, 4, 6, 8

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [v4_tab_ChromaCoeff]
    movd        m0,        [r5 + r4 * 4]
%else
    movd        m0,        [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m0,        [tab_Cm]
    mova        m1,        [pw_512]
    lea         r5,        [r0 + 4 * r1]
    lea         r4,        [r1 * 3]

    movd        m2,        [r0]
    movd        m3,        [r0 + r1]
    movd        m4,        [r0 + 2 * r1]
    movd        m5,        [r0 + r4]

    punpcklbw   m2,        m3
    punpcklbw   m6,        m4,        m5
    punpcklbw   m2,        m6

    pmaddubsw   m2,        m0

    movd        m6,        [r5]

    punpcklbw   m3,        m4
    punpcklbw   m7,        m5,        m6
    punpcklbw   m3,        m7

    pmaddubsw   m3,        m0

    phaddw      m2,        m3

    pmulhrsw    m2,        m1

    movd        m7,        [r5 + r1]

    punpcklbw   m4,        m5
    punpcklbw   m3,        m6,        m7
    punpcklbw   m4,        m3

    pmaddubsw   m4,        m0

    movd        m3,        [r5 + 2 * r1]

    punpcklbw   m5,        m6
    punpcklbw   m7,        m3
    punpcklbw   m5,        m7

    pmaddubsw   m5,        m0

    phaddw      m4,        m5

    pmulhrsw    m4,        m1

    packuswb    m2,        m4
    movd        [r2],      m2
    pextrd      [r2 + r3], m2, 1
    lea         r2,        [r2 + 2 * r3]
    pextrd      [r2],      m2, 2
    pextrd      [r2 + r3], m2, 3
    RET

%macro FILTER_VER_CHROMA_AVX2_4x4 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x4, 4, 6, 3
    mov             r4d, r4m
    shl             r4d, 6
    sub             r0, r1

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]

    movd            xm1, [r0]
    pinsrd          xm1, [r0 + r1], 1
    pinsrd          xm1, [r0 + r1 * 2], 2
    pinsrd          xm1, [r0 + r4], 3                       ; m1 = row[3 2 1 0]
    lea             r0, [r0 + r1 * 4]
    movd            xm2, [r0]
    pinsrd          xm2, [r0 + r1], 1
    pinsrd          xm2, [r0 + r1 * 2], 2                   ; m2 = row[x 6 5 4]
    vinserti128     m1, m1, xm2, 1                          ; m1 = row[x 6 5 4 3 2 1 0]
    mova            m2, [v4_interp4_vpp_shuf1]
    vpermd          m0, m2, m1                              ; m0 = row[4 3 3 2 2 1 1 0]
    mova            m2, [v4_interp4_vpp_shuf1 + mmsize]
    vpermd          m1, m2, m1                              ; m1 = row[6 5 5 4 4 3 3 2]

    mova            m2, [v4_interp4_vpp_shuf]
    pshufb          m0, m0, m2
    pshufb          m1, m1, m2
    pmaddubsw       m0, [r5]
    pmaddubsw       m1, [r5 + mmsize]
    paddw           m0, m1                                  ; m0 = WORD ROW[3 2 1 0]
%ifidn %1,pp
    pmulhrsw        m0, [pw_512]
    vextracti128    xm1, m0, 1
    packuswb        xm0, xm1
    lea             r5, [r3 * 3]
    movd            [r2], xm0
    pextrd          [r2 + r3], xm0, 1
    pextrd          [r2 + r3 * 2], xm0, 2
    pextrd          [r2 + r5], xm0, 3
%else
    add             r3d, r3d
    psubw           m0, [pw_2000]
    vextracti128    xm1, m0, 1
    lea             r5, [r3 * 3]
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm1
    movhps          [r2 + r5], xm1
%endif
    RET
%endmacro
    FILTER_VER_CHROMA_AVX2_4x4 pp
    FILTER_VER_CHROMA_AVX2_4x4 ps

%macro FILTER_VER_CHROMA_AVX2_4x8 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x8, 4, 6, 5
    mov             r4d, r4m
    shl             r4d, 6
    sub             r0, r1

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]

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
    mova            m3, [v4_interp4_vpp_shuf1]
    vpermd          m0, m3, m1                              ; m0 = row[4 3 3 2 2 1 1 0]
    vpermd          m4, m3, m2                              ; m4 = row[8 7 7 6 6 5 5 4]
    mova            m3, [v4_interp4_vpp_shuf1 + mmsize]
    vpermd          m1, m3, m1                              ; m1 = row[6 5 5 4 4 3 3 2]
    vpermd          m2, m3, m2                              ; m2 = row[10 9 9 8 8 7 7 6]

    mova            m3, [v4_interp4_vpp_shuf]
    pshufb          m0, m0, m3
    pshufb          m1, m1, m3
    pshufb          m2, m2, m3
    pshufb          m4, m4, m3
    pmaddubsw       m0, [r5]
    pmaddubsw       m4, [r5]
    pmaddubsw       m1, [r5 + mmsize]
    pmaddubsw       m2, [r5 + mmsize]
    paddw           m0, m1                                  ; m0 = WORD ROW[3 2 1 0]
    paddw           m4, m2                                  ; m4 = WORD ROW[7 6 5 4]
%ifidn %1,pp
    pmulhrsw        m0, [pw_512]
    pmulhrsw        m4, [pw_512]
    packuswb        m0, m4
    vextracti128    xm1, m0, 1
    lea             r5, [r3 * 3]
    movd            [r2], xm0
    pextrd          [r2 + r3], xm0, 1
    movd            [r2 + r3 * 2], xm1
    pextrd          [r2 + r5], xm1, 1
    lea             r2, [r2 + r3 * 4]
    pextrd          [r2], xm0, 2
    pextrd          [r2 + r3], xm0, 3
    pextrd          [r2 + r3 * 2], xm1, 2
    pextrd          [r2 + r5], xm1, 3
%else
    add             r3d, r3d
    psubw           m0, [pw_2000]
    psubw           m4, [pw_2000]
    vextracti128    xm1, m0, 1
    vextracti128    xm2, m4, 1
    lea             r5, [r3 * 3]
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm1
    movhps          [r2 + r5], xm1
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm4
    movhps          [r2 + r3], xm4
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r5], xm2
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_4x8 pp
    FILTER_VER_CHROMA_AVX2_4x8 ps

%macro FILTER_VER_CHROMA_AVX2_4xN 2
%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x%2, 4, 6, 12
    mov             r4d, r4m
    shl             r4d, 6
    sub             r0, r1

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    mova            m10, [r5]
    mova            m11, [r5 + mmsize]
%ifidn %1,pp
    mova            m9, [pw_512]
%else
    add             r3d, r3d
    mova            m9, [pw_2000]
%endif
    lea             r5, [r3 * 3]
%rep %2 / 16
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
    pinsrd          xm4, [r0 + r1 * 2], 2
    pinsrd          xm4, [r0 + r4], 3                       ; m4 = row[15 14 13 12]
    vinserti128     m3, m3, xm4, 1                          ; m3 = row[15 14 13 12 11 10 9 8]
    lea             r0, [r0 + r1 * 4]
    movd            xm5, [r0]
    pinsrd          xm5, [r0 + r1], 1
    pinsrd          xm5, [r0 + r1 * 2], 2                   ; m5 = row[x 18 17 16]
    vinserti128     m4, m4, xm5, 1                          ; m4 = row[x 18 17 16 15 14 13 12]
    mova            m5, [v4_interp4_vpp_shuf1]
    vpermd          m0, m5, m1                              ; m0 = row[4 3 3 2 2 1 1 0]
    vpermd          m6, m5, m2                              ; m6 = row[8 7 7 6 6 5 5 4]
    vpermd          m7, m5, m3                              ; m7 = row[12 11 11 10 10 9 9 8]
    vpermd          m8, m5, m4                              ; m8 = row[16 15 15 14 14 13 13 12]
    mova            m5, [v4_interp4_vpp_shuf1 + mmsize]
    vpermd          m1, m5, m1                              ; m1 = row[6 5 5 4 4 3 3 2]
    vpermd          m2, m5, m2                              ; m2 = row[10 9 9 8 8 7 7 6]
    vpermd          m3, m5, m3                              ; m3 = row[14 13 13 12 12 11 11 10]
    vpermd          m4, m5, m4                              ; m4 = row[18 17 17 16 16 15 15 14]

    mova            m5, [v4_interp4_vpp_shuf]
    pshufb          m0, m0, m5
    pshufb          m1, m1, m5
    pshufb          m2, m2, m5
    pshufb          m4, m4, m5
    pshufb          m3, m3, m5
    pshufb          m6, m6, m5
    pshufb          m7, m7, m5
    pshufb          m8, m8, m5
    pmaddubsw       m0, m10
    pmaddubsw       m6, m10
    pmaddubsw       m7, m10
    pmaddubsw       m8, m10
    pmaddubsw       m1, m11
    pmaddubsw       m2, m11
    pmaddubsw       m3, m11
    pmaddubsw       m4, m11
    paddw           m0, m1                                  ; m0 = WORD ROW[3 2 1 0]
    paddw           m6, m2                                  ; m6 = WORD ROW[7 6 5 4]
    paddw           m7, m3                                  ; m7 = WORD ROW[11 10 9 8]
    paddw           m8, m4                                  ; m8 = WORD ROW[15 14 13 12]
%ifidn %1,pp
    pmulhrsw        m0, m9
    pmulhrsw        m6, m9
    pmulhrsw        m7, m9
    pmulhrsw        m8, m9
    packuswb        m0, m6
    packuswb        m7, m8
    vextracti128    xm1, m0, 1
    vextracti128    xm2, m7, 1
    movd            [r2], xm0
    pextrd          [r2 + r3], xm0, 1
    movd            [r2 + r3 * 2], xm1
    pextrd          [r2 + r5], xm1, 1
    lea             r2, [r2 + r3 * 4]
    pextrd          [r2], xm0, 2
    pextrd          [r2 + r3], xm0, 3
    pextrd          [r2 + r3 * 2], xm1, 2
    pextrd          [r2 + r5], xm1, 3
    lea             r2, [r2 + r3 * 4]
    movd            [r2], xm7
    pextrd          [r2 + r3], xm7, 1
    movd            [r2 + r3 * 2], xm2
    pextrd          [r2 + r5], xm2, 1
    lea             r2, [r2 + r3 * 4]
    pextrd          [r2], xm7, 2
    pextrd          [r2 + r3], xm7, 3
    pextrd          [r2 + r3 * 2], xm2, 2
    pextrd          [r2 + r5], xm2, 3
%else
    psubw           m0, m9
    psubw           m6, m9
    psubw           m7, m9
    psubw           m8, m9
    vextracti128    xm1, m0, 1
    vextracti128    xm2, m6, 1
    vextracti128    xm3, m7, 1
    vextracti128    xm4, m8, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm1
    movhps          [r2 + r5], xm1
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm6
    movhps          [r2 + r3], xm6
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r5], xm2
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm7
    movhps          [r2 + r3], xm7
    movq            [r2 + r3 * 2], xm3
    movhps          [r2 + r5], xm3
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm8
    movhps          [r2 + r3], xm8
    movq            [r2 + r3 * 2], xm4
    movhps          [r2 + r5], xm4
%endif
    lea             r2, [r2 + r3 * 4]
%endrep
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_AVX2_4xN pp, 16
    FILTER_VER_CHROMA_AVX2_4xN ps, 16
    FILTER_VER_CHROMA_AVX2_4xN pp, 32
    FILTER_VER_CHROMA_AVX2_4xN ps, 32

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W4_H4 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_%1x%2, 4, 6, 8

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [v4_tab_ChromaCoeff]
    movd        m0,        [r5 + r4 * 4]
%else
    movd        m0,        [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m0,        [tab_Cm]

    mova        m1,        [pw_512]

    mov         r4d,       %2

    lea         r5,        [3 * r1]

.loop:
    movd        m2,        [r0]
    movd        m3,        [r0 + r1]
    movd        m4,        [r0 + 2 * r1]
    movd        m5,        [r0 + r5]

    punpcklbw   m2,        m3
    punpcklbw   m6,        m4,        m5
    punpcklbw   m2,        m6

    pmaddubsw   m2,        m0

    lea         r0,        [r0 + 4 * r1]
    movd        m6,        [r0]

    punpcklbw   m3,        m4
    punpcklbw   m7,        m5,        m6
    punpcklbw   m3,        m7

    pmaddubsw   m3,        m0

    phaddw      m2,        m3

    pmulhrsw    m2,        m1

    movd        m7,        [r0 + r1]

    punpcklbw   m4,        m5
    punpcklbw   m3,        m6,        m7
    punpcklbw   m4,        m3

    pmaddubsw   m4,        m0

    movd        m3,        [r0 + 2 * r1]

    punpcklbw   m5,        m6
    punpcklbw   m7,        m3
    punpcklbw   m5,        m7

    pmaddubsw   m5,        m0

    phaddw      m4,        m5

    pmulhrsw    m4,        m1
    packuswb    m2,        m4
    movd        [r2],      m2
    pextrd      [r2 + r3], m2,  1
    lea         r2,        [r2 + 2 * r3]
    pextrd      [r2],      m2, 2
    pextrd      [r2 + r3], m2, 3

    lea         r2,        [r2 + 2 * r3]

    sub         r4,        4
    jnz        .loop
    RET
%endmacro

    FILTER_V4_W4_H4 4,  8
    FILTER_V4_W4_H4 4, 16

    FILTER_V4_W4_H4 4, 32

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
    lea         r6,        [v4_tab_ChromaCoeff]
    movd        m5,        [r6 + r4 * 4]
%else
    movd        m5,        [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m6,        m5,       [tab_Vm]
    pmaddubsw   m0,        m6

    pshufb      m5,        [tab_Vm + 16]
    pmaddubsw   m4,        m5

    paddw       m0,        m4

    mova        m4,        [pw_512]

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
cglobal interp_4tap_vert_ps_4x2, 4, 6, 6

    mov         r4d, r4m
    sub         r0, r1
    add         r3d, r3d

%ifdef PIC
    lea         r5, [v4_tab_ChromaCoeff]
    movd        m0, [r5 + r4 * 4]
%else
    movd        m0, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m0, [tab_Cm]

    movd        m2, [r0]
    movd        m3, [r0 + r1]
    lea         r5, [r0 + 2 * r1]
    movd        m4, [r5]
    movd        m5, [r5 + r1]

    punpcklbw   m2, m3
    punpcklbw   m1, m4, m5
    punpcklbw   m2, m1

    pmaddubsw   m2, m0

    movd        m1, [r0 + 4 * r1]

    punpcklbw   m3, m4
    punpcklbw   m5, m1
    punpcklbw   m3, m5

    pmaddubsw   m3, m0

    phaddw      m2, m3

    psubw       m2, [pw_2000]
    movh        [r2], m2
    movhps      [r2 + r3], m2

    RET

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_4x4(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_ps_4x4, 4, 6, 7

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [v4_tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m0, [tab_Cm]

    lea        r4, [r1 * 3]
    lea        r5, [r0 + 4 * r1]

    movd       m2, [r0]
    movd       m3, [r0 + r1]
    movd       m4, [r0 + 2 * r1]
    movd       m5, [r0 + r4]

    punpcklbw  m2, m3
    punpcklbw  m6, m4, m5
    punpcklbw  m2, m6

    pmaddubsw  m2, m0

    movd       m6, [r5]

    punpcklbw  m3, m4
    punpcklbw  m1, m5, m6
    punpcklbw  m3, m1

    pmaddubsw  m3, m0

    phaddw     m2, m3

    mova       m1, [pw_2000]

    psubw      m2, m1
    movh       [r2], m2
    movhps     [r2 + r3], m2

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
    lea        r2, [r2 + 2 * r3]
    movh       [r2], m4
    movhps     [r2 + r3], m4

    RET

;---------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_%1x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------
%macro FILTER_V_PS_W4_H4 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_%1x%2, 4, 6, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [v4_tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m0, [tab_Cm]

    mova       m1, [pw_2000]

    mov        r4d, %2/4
    lea        r5, [3 * r1]

.loop:
    movd       m2, [r0]
    movd       m3, [r0 + r1]
    movd       m4, [r0 + 2 * r1]
    movd       m5, [r0 + r5]

    punpcklbw  m2, m3
    punpcklbw  m6, m4, m5
    punpcklbw  m2, m6

    pmaddubsw  m2, m0

    lea        r0, [r0 + 4 * r1]
    movd       m6, [r0]

    punpcklbw  m3, m4
    punpcklbw  m7, m5, m6
    punpcklbw  m3, m7

    pmaddubsw  m3, m0

    phaddw     m2, m3

    psubw      m2, m1
    movh       [r2], m2
    movhps     [r2 + r3], m2

    movd       m2, [r0 + r1]

    punpcklbw  m4, m5
    punpcklbw  m3, m6, m2
    punpcklbw  m4, m3

    pmaddubsw  m4, m0

    movd       m3, [r0 + 2 * r1]

    punpcklbw  m5, m6
    punpcklbw  m2, m3
    punpcklbw  m5, m2

    pmaddubsw  m5, m0

    phaddw     m4, m5

    psubw      m4, m1
    lea        r2, [r2 + 2 * r3]
    movh       [r2], m4
    movhps     [r2 + r3], m4

    lea        r2, [r2 + 2 * r3]

    dec        r4d
    jnz        .loop
    RET
%endmacro

    FILTER_V_PS_W4_H4 4, 8
    FILTER_V_PS_W4_H4 4, 16

    FILTER_V_PS_W4_H4 4, 32

;--------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_8x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_V_PS_W8_H8_H16_H2 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_%1x%2, 4, 6, 7

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [v4_tab_ChromaCoeff]
    movd       m5, [r5 + r4 * 4]
%else
    movd       m5, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m6, m5, [tab_Vm]
    pshufb     m5, [tab_Vm + 16]
    mova       m4, [pw_2000]

    mov        r4d, %2/2
    lea        r5, [3 * r1]

.loopH:
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    movq       m2, [r0 + 2 * r1]
    movq       m3, [r0 + r5]

    punpcklbw  m0, m1
    punpcklbw  m1, m2
    punpcklbw  m2, m3

    pmaddubsw  m0, m6
    pmaddubsw  m2, m5

    paddw      m0, m2

    psubw      m0, m4
    movu       [r2], m0

    movq       m0, [r0 + 4 * r1]

    punpcklbw  m3, m0

    pmaddubsw  m1, m6
    pmaddubsw  m3, m5

    paddw      m1, m3
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

    FILTER_V_PS_W8_H8_H16_H2 8, 12
    FILTER_V_PS_W8_H8_H16_H2 8, 64

;--------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_8x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_V_PS_W8_H8_H16_H32 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_%1x%2, 4, 6, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [v4_tab_ChromaCoeff]
    movd       m5, [r5 + r4 * 4]
%else
    movd       m5, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m6, m5, [tab_Vm]
    pshufb     m5, [tab_Vm + 16]
    mova       m4, [pw_2000]

    mov        r4d, %2/4
    lea        r5, [3 * r1]

.loop:
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    movq       m2, [r0 + 2 * r1]
    movq       m3, [r0 + r5]

    punpcklbw  m0, m1
    punpcklbw  m1, m2
    punpcklbw  m2, m3

    pmaddubsw  m0, m6
    pmaddubsw  m7, m2, m5

    paddw      m0, m7

    psubw       m0, m4
    movu       [r2], m0

    lea        r0, [r0 + 4 * r1]
    movq       m0, [r0]

    punpcklbw  m3, m0

    pmaddubsw  m1, m6
    pmaddubsw  m7, m3, m5

    paddw      m1, m7

    psubw      m1, m4
    movu       [r2 + r3], m1

    movq       m1, [r0 + r1]

    punpcklbw  m0, m1

    pmaddubsw  m2, m6
    pmaddubsw  m0, m5

    paddw      m2, m0

    psubw      m2, m4
    lea        r2, [r2 + 2 * r3]
    movu       [r2], m2

    movq       m2, [r0 + 2 * r1]

    punpcklbw  m1, m2

    pmaddubsw  m3, m6
    pmaddubsw  m1, m5

    paddw      m3, m1
    psubw      m3, m4

    movu       [r2 + r3], m3

    lea        r2, [r2 + 2 * r3]

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
%macro FILTER_V_PS_W6 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_6x%2, 4, 6, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [v4_tab_ChromaCoeff]
    movd       m5, [r5 + r4 * 4]
%else
    movd       m5, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m6, m5, [tab_Vm]
    pshufb     m5, [tab_Vm + 16]
    mova       m4, [pw_2000]
    lea        r5, [3 * r1]
    mov        r4d, %2/4

.loop:
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    movq       m2, [r0 + 2 * r1]
    movq       m3, [r0 + r5]

    punpcklbw  m0, m1
    punpcklbw  m1, m2
    punpcklbw  m2, m3

    pmaddubsw  m0, m6
    pmaddubsw  m7, m2, m5

    paddw      m0, m7
    psubw      m0, m4

    movh       [r2], m0
    pshufd     m0, m0, 2
    movd       [r2 + 8], m0

    lea        r0, [r0 + 4 * r1]
    movq       m0, [r0]
    punpcklbw  m3, m0

    pmaddubsw  m1, m6
    pmaddubsw  m7, m3, m5

    paddw      m1, m7
    psubw      m1, m4

    movh       [r2 + r3], m1
    pshufd     m1, m1, 2
    movd       [r2 + r3 + 8], m1

    movq       m1, [r0 + r1]
    punpcklbw  m0, m1

    pmaddubsw  m2, m6
    pmaddubsw  m0, m5

    paddw      m2, m0
    psubw      m2, m4

    lea        r2,[r2 + 2 * r3]
    movh       [r2], m2
    pshufd     m2, m2, 2
    movd       [r2 + 8], m2

    movq       m2,[r0 + 2 * r1]
    punpcklbw  m1, m2

    pmaddubsw  m3, m6
    pmaddubsw  m1, m5

    paddw      m3, m1
    psubw      m3, m4

    movh       [r2 + r3], m3
    pshufd     m3, m3, 2
    movd       [r2 + r3 + 8], m3

    lea        r2, [r2 + 2 * r3]

    dec        r4d
    jnz        .loop
    RET
%endmacro

    FILTER_V_PS_W6 6, 8
    FILTER_V_PS_W6 6, 16

;---------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_12x16(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------
%macro FILTER_V_PS_W12 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_12x%2, 4, 6, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [v4_tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m1, m0, [tab_Vm]
    pshufb     m0, [tab_Vm + 16]

    mov        r4d, %2/2

.loop:
    movu       m2, [r0]
    movu       m3, [r0 + r1]

    punpcklbw  m4, m2, m3
    punpckhbw  m2, m3

    pmaddubsw  m4, m1
    pmaddubsw  m2, m1

    lea        r0, [r0 + 2 * r1]
    movu       m5, [r0]
    movu       m7, [r0 + r1]

    punpcklbw  m6, m5, m7
    pmaddubsw  m6, m0
    paddw      m4, m6

    punpckhbw  m6, m5, m7
    pmaddubsw  m6, m0
    paddw      m2, m6

    mova       m6, [pw_2000]

    psubw      m4, m6
    psubw      m2, m6

    movu       [r2], m4
    movh       [r2 + 16], m2

    punpcklbw  m4, m3, m5
    punpckhbw  m3, m5

    pmaddubsw  m4, m1
    pmaddubsw  m3, m1

    movu       m2, [r0 + 2 * r1]

    punpcklbw  m5, m7, m2
    punpckhbw  m7, m2

    pmaddubsw  m5, m0
    pmaddubsw  m7, m0

    paddw      m4, m5
    paddw      m3, m7

    psubw      m4, m6
    psubw      m3, m6

    movu       [r2 + r3], m4
    movh       [r2 + r3 + 16], m3

    lea        r2, [r2 + 2 * r3]

    dec        r4d
    jnz        .loop
    RET
%endmacro

    FILTER_V_PS_W12 12, 16
    FILTER_V_PS_W12 12, 32

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
    lea        r5, [v4_tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m1, m0, [tab_Vm]
    pshufb     m0, [tab_Vm + 16]
    mov        r4d, %2/2

.loop:
    movu       m2, [r0]
    movu       m3, [r0 + r1]

    punpcklbw  m4, m2, m3
    punpckhbw  m2, m3

    pmaddubsw  m4, m1
    pmaddubsw  m2, m1

    lea        r0, [r0 + 2 * r1]
    movu       m5, [r0]
    movu       m7, [r0 + r1]

    punpcklbw  m6, m5, m7
    pmaddubsw  m6, m0
    paddw      m4, m6

    punpckhbw  m6, m5, m7
    pmaddubsw  m6, m0
    paddw      m2, m6

    mova       m6, [pw_2000]

    psubw      m4, m6
    psubw      m2, m6

    movu       [r2], m4
    movu       [r2 + 16], m2

    punpcklbw  m4, m3, m5
    punpckhbw  m3, m5

    pmaddubsw  m4, m1
    pmaddubsw  m3, m1

    movu       m5, [r0 + 2 * r1]

    punpcklbw  m2, m7, m5
    punpckhbw  m7, m5

    pmaddubsw  m2, m0
    pmaddubsw  m7, m0

    paddw      m4, m2
    paddw      m3, m7

    psubw      m4, m6
    psubw      m3, m6

    movu       [r2 + r3], m4
    movu       [r2 + r3 + 16], m3

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

    FILTER_V_PS_W16 16, 24
    FILTER_V_PS_W16 16, 64

;--------------------------------------------------------------------------------------------------------------
;void interp_4tap_vert_ps_24x32(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_V4_PS_W24 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_24x%2, 4, 6, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [v4_tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m1, m0, [tab_Vm]
    pshufb     m0, [tab_Vm + 16]

    mov        r4d, %2/2

.loop:
    movu       m2, [r0]
    movu       m3, [r0 + r1]

    punpcklbw  m4, m2, m3
    punpckhbw  m2, m3

    pmaddubsw  m4, m1
    pmaddubsw  m2, m1

    lea        r5, [r0 + 2 * r1]

    movu       m5, [r5]
    movu       m7, [r5 + r1]

    punpcklbw  m6, m5, m7
    pmaddubsw  m6, m0
    paddw      m4, m6

    punpckhbw  m6, m5, m7
    pmaddubsw  m6, m0
    paddw      m2, m6

    mova       m6, [pw_2000]

    psubw      m4, m6
    psubw      m2, m6

    movu       [r2], m4
    movu       [r2 + 16], m2

    punpcklbw  m4, m3, m5
    punpckhbw  m3, m5

    pmaddubsw  m4, m1
    pmaddubsw  m3, m1

    movu       m2, [r5 + 2 * r1]

    punpcklbw  m5, m7, m2
    punpckhbw  m7, m2

    pmaddubsw  m5, m0
    pmaddubsw  m7, m0

    paddw      m4, m5
    paddw      m3, m7

    psubw      m4, m6
    psubw      m3, m6

    movu       [r2 + r3], m4
    movu       [r2 + r3 + 16], m3

    movq       m2, [r0 + 16]
    movq       m3, [r0 + r1 + 16]
    movq       m4, [r5 + 16]
    movq       m5, [r5 + r1 + 16]

    punpcklbw  m2, m3
    punpcklbw  m7, m4, m5

    pmaddubsw  m2, m1
    pmaddubsw  m7, m0

    paddw      m2, m7
    psubw      m2, m6

    movu       [r2 + 32], m2

    movq       m2, [r5 + 2 * r1 + 16]

    punpcklbw  m3, m4
    punpcklbw  m5, m2

    pmaddubsw  m3, m1
    pmaddubsw  m5, m0

    paddw      m3, m5
    psubw      m3,  m6

    movu       [r2 + r3 + 32], m3

    mov        r0, r5
    lea        r2, [r2 + 2 * r3]

    dec        r4d
    jnz        .loop
    RET
%endmacro

    FILTER_V4_PS_W24 24, 32

    FILTER_V4_PS_W24 24, 64

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
    lea        r5, [v4_tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m1, m0, [tab_Vm]
    pshufb     m0, [tab_Vm + 16]

    mova       m7, [pw_2000]

    mov        r4d, %2

.loop:
    movu       m2, [r0]
    movu       m3, [r0 + r1]

    punpcklbw  m4, m2, m3
    punpckhbw  m2, m3

    pmaddubsw  m4, m1
    pmaddubsw  m2, m1

    lea        r5, [r0 + 2 * r1]
    movu       m3, [r5]
    movu       m5, [r5 + r1]

    punpcklbw  m6, m3, m5
    punpckhbw  m3, m5

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

    punpcklbw  m4, m2, m3
    punpckhbw  m2, m3

    pmaddubsw  m4, m1
    pmaddubsw  m2, m1

    movu       m3, [r5 + 16]
    movu       m5, [r5 + r1 + 16]

    punpcklbw  m6, m3, m5
    punpckhbw  m3, m5

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

    FILTER_V_PS_W32 32, 48
    FILTER_V_PS_W32 32, 64

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W8_H8_H16_H32 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_%1x%2, 4, 6, 8

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [v4_tab_ChromaCoeff]
    movd        m5,        [r5 + r4 * 4]
%else
    movd        m5,        [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m6,        m5,       [tab_Vm]
    pshufb      m5,        [tab_Vm + 16]
    mova        m4,        [pw_512]
    lea         r5,        [r1 * 3]

    mov         r4d,       %2

.loop:
    movq        m0,        [r0]
    movq        m1,        [r0 + r1]
    movq        m2,        [r0 + 2 * r1]
    movq        m3,        [r0 + r5]

    punpcklbw   m0,        m1
    punpcklbw   m1,        m2
    punpcklbw   m2,        m3

    pmaddubsw   m0,        m6
    pmaddubsw   m7,        m2, m5

    paddw       m0,        m7

    pmulhrsw    m0,        m4
    packuswb    m0,        m0
    movh        [r2],      m0

    lea         r0,        [r0 + 4 * r1]
    movq        m0,        [r0]

    punpcklbw   m3,        m0

    pmaddubsw   m1,        m6
    pmaddubsw   m7,        m3, m5

    paddw       m1,        m7

    pmulhrsw    m1,        m4
    packuswb    m1,        m1
    movh        [r2 + r3], m1

    movq        m1,        [r0 + r1]

    punpcklbw   m0,        m1

    pmaddubsw   m2,        m6
    pmaddubsw   m0,        m5

    paddw       m2,        m0

    pmulhrsw    m2,        m4

    movq        m7,        [r0 + 2 * r1]
    punpcklbw   m1,        m7

    pmaddubsw   m3,        m6
    pmaddubsw   m1,        m5

    paddw       m3,        m1

    pmulhrsw    m3,        m4
    packuswb    m2,        m3

    lea         r2,        [r2 + 2 * r3]
    movh        [r2],      m2
    movhps      [r2 + r3], m2

    lea         r2,        [r2 + 2 * r3]

    sub         r4,         4
    jnz        .loop
    RET
%endmacro

    FILTER_V4_W8_H8_H16_H32 8,  8
    FILTER_V4_W8_H8_H16_H32 8, 16
    FILTER_V4_W8_H8_H16_H32 8, 32

    FILTER_V4_W8_H8_H16_H32 8, 12
    FILTER_V4_W8_H8_H16_H32 8, 64

%macro PROCESS_CHROMA_AVX2_W8_8R 0
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
    pmaddubsw       m0, m1, [r5 + 1 * mmsize]
    paddw           m2, m0
    pmaddubsw       m1, [r5]
    movq            xm3, [r0 + r4]                  ; m3 = row 7
    punpcklbw       xm4, xm3                        ; m4 = [77 67 76 66 75 65 74 64 73 63 72 62 71 61 70 60]
    lea             r0, [r0 + r1 * 4]
    movq            xm0, [r0]                       ; m0 = row 8
    punpcklbw       xm3, xm0                        ; m3 = [87 77 86 76 85 75 84 74 83 73 82 72 81 71 80 70]
    vinserti128     m4, m4, xm3, 1                  ; m4 = [87 77 86 76 85 75 84 74 83 73 82 72 81 71 80 70] - [77 67 76 66 75 65 74 64 73 63 72 62 71 61 70 60]
    pmaddubsw       m3, m4, [r5 + 1 * mmsize]
    paddw           m1, m3
    pmaddubsw       m4, [r5]
    movq            xm3, [r0 + r1]                  ; m3 = row 9
    punpcklbw       xm0, xm3                        ; m0 = [97 87 96 86 95 85 94 84 93 83 92 82 91 81 90 80]
    movq            xm6, [r0 + r1 * 2]              ; m6 = row 10
    punpcklbw       xm3, xm6                        ; m3 = [A7 97 A6 96 A5 95 A4 94 A3 93 A2 92 A1 91 A0 90]
    vinserti128     m0, m0, xm3, 1                  ; m0 = [A7 97 A6 96 A5 95 A4 94 A3 93 A2 92 A1 91 A0 90] - [97 87 96 86 95 85 94 84 93 83 92 82 91 81 90 80]
    pmaddubsw       m0, [r5 + 1 * mmsize]
    paddw           m4, m0
%endmacro

%macro FILTER_VER_CHROMA_AVX2_8x8 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x8, 4, 6, 7
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
    PROCESS_CHROMA_AVX2_W8_8R
%ifidn %1,pp
    lea             r4, [r3 * 3]
    mova            m3, [pw_512]
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
    add             r3d, r3d
    vbroadcasti128  m3, [pw_2000]
    lea             r4, [r3 * 3]
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

    FILTER_VER_CHROMA_AVX2_8x8 pp
    FILTER_VER_CHROMA_AVX2_8x8 ps

%macro FILTER_VER_CHROMA_AVX2_8x6 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x6, 4, 6, 6
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1

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
    pmaddubsw       m0, m1, [r5 + 1 * mmsize]
    paddw           m2, m0
    pmaddubsw       m1, [r5]
    movq            xm3, [r0 + r4]                  ; m3 = row 7
    punpcklbw       xm4, xm3                        ; m4 = [77 67 76 66 75 65 74 64 73 63 72 62 71 61 70 60]
    lea             r0, [r0 + r1 * 4]
    movq            xm0, [r0]                       ; m0 = row 8
    punpcklbw       xm3, xm0                        ; m3 = [87 77 86 76 85 75 84 74 83 73 82 72 81 71 80 70]
    vinserti128     m4, m4, xm3, 1                  ; m4 = [87 77 86 76 85 75 84 74 83 73 82 72 81 71 80 70] - [77 67 76 66 75 65 74 64 73 63 72 62 71 61 70 60]
    pmaddubsw       m4, [r5 + 1 * mmsize]
    paddw           m1, m4
%ifidn %1,pp
    lea             r4, [r3 * 3]
    mova            m3, [pw_512]
    pmulhrsw        m5, m3                          ; m5 = word: row 0, row 1
    pmulhrsw        m2, m3                          ; m2 = word: row 2, row 3
    pmulhrsw        m1, m3                          ; m1 = word: row 4, row 5
    packuswb        m5, m2
    packuswb        m1, m1
    vextracti128    xm2, m5, 1
    vextracti128    xm4, m1, 1
    movq            [r2], xm5
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm5
    movhps          [r2 + r4], xm2
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm1
    movq            [r2 + r3], xm4
%else
    add             r3d, r3d
    mova            m3, [pw_2000]
    lea             r4, [r3 * 3]
    psubw           m5, m3                          ; m5 = word: row 0, row 1
    psubw           m2, m3                          ; m2 = word: row 2, row 3
    psubw           m1, m3                          ; m1 = word: row 4, row 5
    vextracti128    xm4, m5, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm0, m1, 1
    movu            [r2], xm5
    movu            [r2 + r3], xm4
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r4], xm3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm1
    movu            [r2 + r3], xm0
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_8x6 pp
    FILTER_VER_CHROMA_AVX2_8x6 ps

%macro PROCESS_CHROMA_AVX2_W8_16R 1
    movq            xm1, [r0]                       ; m1 = row 0
    movq            xm2, [r0 + r1]                  ; m2 = row 1
    punpcklbw       xm1, xm2
    movq            xm3, [r0 + r1 * 2]              ; m3 = row 2
    punpcklbw       xm2, xm3
    vinserti128     m5, m1, xm2, 1
    pmaddubsw       m5, [r5]
    movq            xm4, [r0 + r4]                  ; m4 = row 3
    punpcklbw       xm3, xm4
    lea             r0, [r0 + r1 * 4]
    movq            xm1, [r0]                       ; m1 = row 4
    punpcklbw       xm4, xm1
    vinserti128     m2, m3, xm4, 1
    pmaddubsw       m0, m2, [r5 + 1 * mmsize]
    paddw           m5, m0
    pmaddubsw       m2, [r5]
    movq            xm3, [r0 + r1]                  ; m3 = row 5
    punpcklbw       xm1, xm3
    movq            xm4, [r0 + r1 * 2]              ; m4 = row 6
    punpcklbw       xm3, xm4
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m0, m1, [r5 + 1 * mmsize]
    paddw           m2, m0
    pmaddubsw       m1, [r5]
    movq            xm3, [r0 + r4]                  ; m3 = row 7
    punpcklbw       xm4, xm3
    lea             r0, [r0 + r1 * 4]
    movq            xm0, [r0]                       ; m0 = row 8
    punpcklbw       xm3, xm0
    vinserti128     m4, m4, xm3, 1
    pmaddubsw       m3, m4, [r5 + 1 * mmsize]
    paddw           m1, m3
    pmaddubsw       m4, [r5]
    movq            xm3, [r0 + r1]                  ; m3 = row 9
    punpcklbw       xm0, xm3
    movq            xm6, [r0 + r1 * 2]              ; m6 = row 10
    punpcklbw       xm3, xm6
    vinserti128     m0, m0, xm3, 1
    pmaddubsw       m3, m0, [r5 + 1 * mmsize]
    paddw           m4, m3
    pmaddubsw       m0, [r5]
%ifidn %1,pp
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
    movhps          [r2 + r3 * 2], xm5
    movhps          [r2 + r6], xm2
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm1
    movq            [r2 + r3], xm4
    movhps          [r2 + r3 * 2], xm1
    movhps          [r2 + r6], xm4
%else
    psubw           m5, m7                          ; m5 = word: row 0, row 1
    psubw           m2, m7                          ; m2 = word: row 2, row 3
    psubw           m1, m7                          ; m1 = word: row 4, row 5
    psubw           m4, m7                          ; m4 = word: row 6, row 7
    vextracti128    xm3, m5, 1
    movu            [r2], xm5
    movu            [r2 + r3], xm3
    vextracti128    xm3, m2, 1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
    lea             r2, [r2 + r3 * 4]
    vextracti128    xm5, m1, 1
    vextracti128    xm3, m4, 1
    movu            [r2], xm1
    movu            [r2 + r3], xm5
    movu            [r2 + r3 * 2], xm4
    movu            [r2 + r6], xm3
%endif
    movq            xm3, [r0 + r4]                  ; m3 = row 11
    punpcklbw       xm6, xm3
    lea             r0, [r0 + r1 * 4]
    movq            xm5, [r0]                       ; m5 = row 12
    punpcklbw       xm3, xm5
    vinserti128     m6, m6, xm3, 1
    pmaddubsw       m3, m6, [r5 + 1 * mmsize]
    paddw           m0, m3
    pmaddubsw       m6, [r5]
    movq            xm3, [r0 + r1]                  ; m3 = row 13
    punpcklbw       xm5, xm3
    movq            xm2, [r0 + r1 * 2]              ; m2 = row 14
    punpcklbw       xm3, xm2
    vinserti128     m5, m5, xm3, 1
    pmaddubsw       m3, m5, [r5 + 1 * mmsize]
    paddw           m6, m3
    pmaddubsw       m5, [r5]
    movq            xm3, [r0 + r4]                  ; m3 = row 15
    punpcklbw       xm2, xm3
    lea             r0, [r0 + r1 * 4]
    movq            xm1, [r0]                       ; m1 = row 16
    punpcklbw       xm3, xm1
    vinserti128     m2, m2, xm3, 1
    pmaddubsw       m3, m2, [r5 + 1 * mmsize]
    paddw           m5, m3
    pmaddubsw       m2, [r5]
    movq            xm3, [r0 + r1]                  ; m3 = row 17
    punpcklbw       xm1, xm3
    movq            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpcklbw       xm3, xm4
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m1, [r5 + 1 * mmsize]
    paddw           m2, m1
    lea             r2, [r2 + r3 * 4]
%ifidn %1,pp
    pmulhrsw        m0, m7                          ; m0 = word: row 8, row 9
    pmulhrsw        m6, m7                          ; m6 = word: row 10, row 11
    pmulhrsw        m5, m7                          ; m5 = word: row 12, row 13
    pmulhrsw        m2, m7                          ; m2 = word: row 14, row 15
    packuswb        m0, m6
    packuswb        m5, m2
    vextracti128    xm6, m0, 1
    vextracti128    xm2, m5, 1
    movq            [r2], xm0
    movq            [r2 + r3], xm6
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r6], xm6
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm5
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm5
    movhps          [r2 + r6], xm2
%else
    psubw           m0, m7                          ; m0 = word: row 8, row 9
    psubw           m6, m7                          ; m6 = word: row 10, row 11
    psubw           m5, m7                          ; m5 = word: row 12, row 13
    psubw           m2, m7                          ; m2 = word: row 14, row 15
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m6, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r6], xm3
    lea             r2, [r2 + r3 * 4]
    vextracti128    xm1, m5, 1
    vextracti128    xm3, m2, 1
    movu            [r2], xm5
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
%endif
%endmacro

%macro FILTER_VER_CHROMA_AVX2_8x16 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x16, 4, 7, 8
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m7, [pw_512]
%else
    add             r3d, r3d
    mova            m7, [pw_2000]
%endif
    lea             r6, [r3 * 3]
    PROCESS_CHROMA_AVX2_W8_16R %1
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_8x16 pp
    FILTER_VER_CHROMA_AVX2_8x16 ps

%macro FILTER_VER_CHROMA_AVX2_8x12 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x12, 4, 7, 8
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1, pp
    mova            m7, [pw_512]
%else
    add             r3d, r3d
    mova            m7, [pw_2000]
%endif
    lea             r6, [r3 * 3]
    movq            xm1, [r0]                       ; m1 = row 0
    movq            xm2, [r0 + r1]                  ; m2 = row 1
    punpcklbw       xm1, xm2
    movq            xm3, [r0 + r1 * 2]              ; m3 = row 2
    punpcklbw       xm2, xm3
    vinserti128     m5, m1, xm2, 1
    pmaddubsw       m5, [r5]
    movq            xm4, [r0 + r4]                  ; m4 = row 3
    punpcklbw       xm3, xm4
    lea             r0, [r0 + r1 * 4]
    movq            xm1, [r0]                       ; m1 = row 4
    punpcklbw       xm4, xm1
    vinserti128     m2, m3, xm4, 1
    pmaddubsw       m0, m2, [r5 + 1 * mmsize]
    paddw           m5, m0
    pmaddubsw       m2, [r5]
    movq            xm3, [r0 + r1]                  ; m3 = row 5
    punpcklbw       xm1, xm3
    movq            xm4, [r0 + r1 * 2]              ; m4 = row 6
    punpcklbw       xm3, xm4
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m0, m1, [r5 + 1 * mmsize]
    paddw           m2, m0
    pmaddubsw       m1, [r5]
    movq            xm3, [r0 + r4]                  ; m3 = row 7
    punpcklbw       xm4, xm3
    lea             r0, [r0 + r1 * 4]
    movq            xm0, [r0]                       ; m0 = row 8
    punpcklbw       xm3, xm0
    vinserti128     m4, m4, xm3, 1
    pmaddubsw       m3, m4, [r5 + 1 * mmsize]
    paddw           m1, m3
    pmaddubsw       m4, [r5]
    movq            xm3, [r0 + r1]                  ; m3 = row 9
    punpcklbw       xm0, xm3
    movq            xm6, [r0 + r1 * 2]              ; m6 = row 10
    punpcklbw       xm3, xm6
    vinserti128     m0, m0, xm3, 1
    pmaddubsw       m3, m0, [r5 + 1 * mmsize]
    paddw           m4, m3
    pmaddubsw       m0, [r5]
%ifidn %1, pp
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
    movhps          [r2 + r3 * 2], xm5
    movhps          [r2 + r6], xm2
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm1
    movq            [r2 + r3], xm4
    movhps          [r2 + r3 * 2], xm1
    movhps          [r2 + r6], xm4
%else
    psubw           m5, m7                          ; m5 = word: row 0, row 1
    psubw           m2, m7                          ; m2 = word: row 2, row 3
    psubw           m1, m7                          ; m1 = word: row 4, row 5
    psubw           m4, m7                          ; m4 = word: row 6, row 7
    vextracti128    xm3, m5, 1
    movu            [r2], xm5
    movu            [r2 + r3], xm3
    vextracti128    xm3, m2, 1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
    lea             r2, [r2 + r3 * 4]
    vextracti128    xm5, m1, 1
    vextracti128    xm3, m4, 1
    movu            [r2], xm1
    movu            [r2 + r3], xm5
    movu            [r2 + r3 * 2], xm4
    movu            [r2 + r6], xm3
%endif
    movq            xm3, [r0 + r4]                  ; m3 = row 11
    punpcklbw       xm6, xm3
    lea             r0, [r0 + r1 * 4]
    movq            xm5, [r0]                       ; m5 = row 12
    punpcklbw       xm3, xm5
    vinserti128     m6, m6, xm3, 1
    pmaddubsw       m3, m6, [r5 + 1 * mmsize]
    paddw           m0, m3
    pmaddubsw       m6, [r5]
    movq            xm3, [r0 + r1]                  ; m3 = row 13
    punpcklbw       xm5, xm3
    movq            xm2, [r0 + r1 * 2]              ; m2 = row 14
    punpcklbw       xm3, xm2
    vinserti128     m5, m5, xm3, 1
    pmaddubsw       m3, m5, [r5 + 1 * mmsize]
    paddw           m6, m3
    lea             r2, [r2 + r3 * 4]
%ifidn %1, pp
    pmulhrsw        m0, m7                          ; m0 = word: row 8, row 9
    pmulhrsw        m6, m7                          ; m6 = word: row 10, row 11
    packuswb        m0, m6
    vextracti128    xm6, m0, 1
    movq            [r2], xm0
    movq            [r2 + r3], xm6
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r6], xm6
%else
    psubw           m0, m7                          ; m0 = word: row 8, row 9
    psubw           m6, m7                          ; m6 = word: row 10, row 11
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m6, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r6], xm3
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_8x12 pp
    FILTER_VER_CHROMA_AVX2_8x12 ps

%macro FILTER_VER_CHROMA_AVX2_8xN 2
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x%2, 4, 7, 8
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m7, [pw_512]
%else
    add             r3d, r3d
    mova            m7, [pw_2000]
%endif
    lea             r6, [r3 * 3]
%rep %2 / 16
    PROCESS_CHROMA_AVX2_W8_16R %1
    lea             r2, [r2 + r3 * 4]
%endrep
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_8xN pp, 32
    FILTER_VER_CHROMA_AVX2_8xN ps, 32
    FILTER_VER_CHROMA_AVX2_8xN pp, 64
    FILTER_VER_CHROMA_AVX2_8xN ps, 64

%macro PROCESS_CHROMA_AVX2_W8_4R 0
    movq            xm1, [r0]                       ; m1 = row 0
    movq            xm2, [r0 + r1]                  ; m2 = row 1
    punpcklbw       xm1, xm2                        ; m1 = [17 07 16 06 15 05 14 04 13 03 12 02 11 01 10 00]
    movq            xm3, [r0 + r1 * 2]              ; m3 = row 2
    punpcklbw       xm2, xm3                        ; m2 = [27 17 26 16 25 15 24 14 23 13 22 12 21 11 20 10]
    vinserti128     m0, m1, xm2, 1                  ; m0 = [27 17 26 16 25 15 24 14 23 13 22 12 21 11 20 10] - [17 07 16 06 15 05 14 04 13 03 12 02 11 01 10 00]
    pmaddubsw       m0, [r5]
    movq            xm4, [r0 + r4]                  ; m4 = row 3
    punpcklbw       xm3, xm4                        ; m3 = [37 27 36 26 35 25 34 24 33 23 32 22 31 21 30 20]
    lea             r0, [r0 + r1 * 4]
    movq            xm1, [r0]                       ; m1 = row 4
    punpcklbw       xm4, xm1                        ; m4 = [47 37 46 36 45 35 44 34 43 33 42 32 41 31 40 30]
    vinserti128     m2, m3, xm4, 1                  ; m2 = [47 37 46 36 45 35 44 34 43 33 42 32 41 31 40 30] - [37 27 36 26 35 25 34 24 33 23 32 22 31 21 30 20]
    pmaddubsw       m4, m2, [r5 + 1 * mmsize]
    paddw           m0, m4
    pmaddubsw       m2, [r5]
    movq            xm3, [r0 + r1]                  ; m3 = row 5
    punpcklbw       xm1, xm3                        ; m1 = [57 47 56 46 55 45 54 44 53 43 52 42 51 41 50 40]
    movq            xm4, [r0 + r1 * 2]              ; m4 = row 6
    punpcklbw       xm3, xm4                        ; m3 = [67 57 66 56 65 55 64 54 63 53 62 52 61 51 60 50]
    vinserti128     m1, m1, xm3, 1                  ; m1 = [67 57 66 56 65 55 64 54 63 53 62 52 61 51 60 50] - [57 47 56 46 55 45 54 44 53 43 52 42 51 41 50 40]
    pmaddubsw       m1, [r5 + 1 * mmsize]
    paddw           m2, m1
%endmacro

%macro FILTER_VER_CHROMA_AVX2_8x4 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x4, 4, 6, 5
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
    PROCESS_CHROMA_AVX2_W8_4R
%ifidn %1,pp
    lea             r4, [r3 * 3]
    mova            m3, [pw_512]
    pmulhrsw        m0, m3                          ; m0 = word: row 0, row 1
    pmulhrsw        m2, m3                          ; m2 = word: row 2, row 3
    packuswb        m0, m2
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r4], xm2
%else
    add             r3d, r3d
    vbroadcasti128  m3, [pw_2000]
    lea             r4, [r3 * 3]
    psubw           m0, m3                          ; m0 = word: row 0, row 1
    psubw           m2, m3                          ; m2 = word: row 2, row 3
    vextracti128    xm1, m0, 1
    vextracti128    xm4, m2, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r4], xm4
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_8x4 pp
    FILTER_VER_CHROMA_AVX2_8x4 ps

%macro FILTER_VER_CHROMA_AVX2_8x2 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x2, 4, 6, 4
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1

    movq            xm1, [r0]                       ; m1 = row 0
    movq            xm2, [r0 + r1]                  ; m2 = row 1
    punpcklbw       xm1, xm2                        ; m1 = [17 07 16 06 15 05 14 04 13 03 12 02 11 01 10 00]
    movq            xm3, [r0 + r1 * 2]              ; m3 = row 2
    punpcklbw       xm2, xm3                        ; m2 = [27 17 26 16 25 15 24 14 23 13 22 12 21 11 20 10]
    vinserti128     m1, m1, xm2, 1                  ; m1 = [27 17 26 16 25 15 24 14 23 13 22 12 21 11 20 10] - [17 07 16 06 15 05 14 04 13 03 12 02 11 01 10 00]
    pmaddubsw       m1, [r5]
    movq            xm2, [r0 + r4]                  ; m2 = row 3
    punpcklbw       xm3, xm2                        ; m3 = [37 27 36 26 35 25 34 24 33 23 32 22 31 21 30 20]
    movq            xm0, [r0 + r1 * 4]              ; m0 = row 4
    punpcklbw       xm2, xm0                        ; m2 = [47 37 46 36 45 35 44 34 43 33 42 32 41 31 40 30]
    vinserti128     m3, m3, xm2, 1                  ; m3 = [47 37 46 36 45 35 44 34 43 33 42 32 41 31 40 30] - [37 27 36 26 35 25 34 24 33 23 32 22 31 21 30 20]
    pmaddubsw       m3, [r5 + 1 * mmsize]
    paddw           m1, m3
%ifidn %1,pp
    pmulhrsw        m1, [pw_512]                    ; m1 = word: row 0, row 1
    packuswb        m1, m1
    vextracti128    xm0, m1, 1
    movq            [r2], xm1
    movq            [r2 + r3], xm0
%else
    add             r3d, r3d
    psubw           m1, [pw_2000]                   ; m1 = word: row 0, row 1
    vextracti128    xm0, m1, 1
    movu            [r2], xm1
    movu            [r2 + r3], xm0
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_8x2 pp
    FILTER_VER_CHROMA_AVX2_8x2 ps

%macro FILTER_VER_CHROMA_AVX2_6x8 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_6x8, 4, 6, 7
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
    PROCESS_CHROMA_AVX2_W8_8R
%ifidn %1,pp
    lea             r4, [r3 * 3]
    mova            m3, [pw_512]
    pmulhrsw        m5, m3                          ; m5 = word: row 0, row 1
    pmulhrsw        m2, m3                          ; m2 = word: row 2, row 3
    pmulhrsw        m1, m3                          ; m1 = word: row 4, row 5
    pmulhrsw        m4, m3                          ; m4 = word: row 6, row 7
    packuswb        m5, m2
    packuswb        m1, m4
    vextracti128    xm2, m5, 1
    vextracti128    xm4, m1, 1
    movd            [r2], xm5
    pextrw          [r2 + 4], xm5, 2
    movd            [r2 + r3], xm2
    pextrw          [r2 + r3 + 4], xm2, 2
    pextrd          [r2 + r3 * 2], xm5, 2
    pextrw          [r2 + r3 * 2 + 4], xm5, 6
    pextrd          [r2 + r4], xm2, 2
    pextrw          [r2 + r4 + 4], xm2, 6
    lea             r2, [r2 + r3 * 4]
    movd            [r2], xm1
    pextrw          [r2 + 4], xm1, 2
    movd            [r2 + r3], xm4
    pextrw          [r2 + r3 + 4], xm4, 2
    pextrd          [r2 + r3 * 2], xm1, 2
    pextrw          [r2 + r3 * 2 + 4], xm1, 6
    pextrd          [r2 + r4], xm4, 2
    pextrw          [r2 + r4 + 4], xm4, 6
%else
    add             r3d, r3d
    vbroadcasti128  m3, [pw_2000]
    lea             r4, [r3 * 3]
    psubw           m5, m3                          ; m5 = word: row 0, row 1
    psubw           m2, m3                          ; m2 = word: row 2, row 3
    psubw           m1, m3                          ; m1 = word: row 4, row 5
    psubw           m4, m3                          ; m4 = word: row 6, row 7
    vextracti128    xm6, m5, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm0, m1, 1
    movq            [r2], xm5
    pextrd          [r2 + 8], xm5, 2
    movq            [r2 + r3], xm6
    pextrd          [r2 + r3 + 8], xm6, 2
    movq            [r2 + r3 * 2], xm2
    pextrd          [r2 + r3 * 2 + 8], xm2, 2
    movq            [r2 + r4], xm3
    pextrd          [r2 + r4 + 8], xm3, 2
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm1
    pextrd          [r2 + 8], xm1, 2
    movq            [r2 + r3], xm0
    pextrd          [r2 + r3 + 8], xm0, 2
    movq            [r2 + r3 * 2], xm4
    pextrd          [r2 + r3 * 2 + 8], xm4, 2
    vextracti128    xm4, m4, 1
    movq            [r2 + r4], xm4
    pextrd          [r2 + r4 + 8], xm4, 2
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_6x8 pp
    FILTER_VER_CHROMA_AVX2_6x8 ps

;-----------------------------------------------------------------------------
;void interp_4tap_vert_pp_6x8(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W6_H4 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_6x%2, 4, 6, 8

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [v4_tab_ChromaCoeff]
    movd        m5,        [r5 + r4 * 4]
%else
    movd        m5,        [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m6,        m5,       [tab_Vm]
    pshufb      m5,        [tab_Vm + 16]
    mova        m4,        [pw_512]

    mov         r4d,       %2
    lea         r5,        [3 * r1]

.loop:
    movq        m0,        [r0]
    movq        m1,        [r0 + r1]
    movq        m2,        [r0 + 2 * r1]
    movq        m3,        [r0 + r5]

    punpcklbw   m0,        m1
    punpcklbw   m1,        m2
    punpcklbw   m2,        m3

    pmaddubsw   m0,        m6
    pmaddubsw   m7,        m2, m5

    paddw       m0,        m7

    pmulhrsw    m0,        m4
    packuswb    m0,        m0
    movd        [r2],      m0
    pextrw      [r2 + 4],  m0,    2

    lea         r0,        [r0 + 4 * r1]

    movq        m0,        [r0]
    punpcklbw   m3,        m0

    pmaddubsw   m1,        m6
    pmaddubsw   m7,        m3, m5

    paddw       m1,        m7

    pmulhrsw    m1,        m4
    packuswb    m1,        m1
    movd        [r2 + r3],      m1
    pextrw      [r2 + r3 + 4],  m1,    2

    movq        m1,        [r0 + r1]
    punpcklbw   m7,        m0,        m1

    pmaddubsw   m2,        m6
    pmaddubsw   m7,        m5

    paddw       m2,        m7

    pmulhrsw    m2,        m4
    packuswb    m2,        m2
    lea         r2,        [r2 + 2 * r3]
    movd        [r2],      m2
    pextrw      [r2 + 4],  m2,    2

    movq        m2,        [r0 + 2 * r1]
    punpcklbw   m1,        m2

    pmaddubsw   m3,        m6
    pmaddubsw   m1,        m5

    paddw       m3,        m1

    pmulhrsw    m3,        m4
    packuswb    m3,        m3

    movd        [r2 + r3],        m3
    pextrw      [r2 + r3 + 4],    m3,    2

    lea         r2,        [r2 + 2 * r3]

    sub         r4,         4
    jnz        .loop
    RET
%endmacro

    FILTER_V4_W6_H4 6, 8

    FILTER_V4_W6_H4 6, 16

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_12x16(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W12_H2 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_12x%2, 4, 6, 8

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [v4_tab_ChromaCoeff]
    movd        m0,        [r5 + r4 * 4]
%else
    movd        m0,        [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m1,        m0,       [tab_Vm]
    pshufb      m0,        [tab_Vm + 16]

    mov         r4d,       %2

.loop:
    movu        m2,        [r0]
    movu        m3,        [r0 + r1]

    punpcklbw   m4,        m2,        m3
    punpckhbw   m2,        m3

    pmaddubsw   m4,        m1
    pmaddubsw   m2,        m1

    lea         r0,        [r0 + 2 * r1]
    movu        m5,        [r0]
    movu        m7,        [r0 + r1]

    punpcklbw   m6,        m5,        m7
    pmaddubsw   m6,        m0
    paddw       m4,        m6

    punpckhbw   m6,        m5,        m7
    pmaddubsw   m6,        m0
    paddw       m2,        m6

    mova        m6,        [pw_512]

    pmulhrsw    m4,        m6
    pmulhrsw    m2,        m6

    packuswb    m4,        m2

    movh         [r2],     m4
    pextrd       [r2 + 8], m4,  2

    punpcklbw   m4,        m3,        m5
    punpckhbw   m3,        m5

    pmaddubsw   m4,        m1
    pmaddubsw   m3,        m1

    movu        m5,        [r0 + 2 * r1]

    punpcklbw   m2,        m7,        m5
    punpckhbw   m7,        m5

    pmaddubsw   m2,        m0
    pmaddubsw   m7,        m0

    paddw       m4,        m2
    paddw       m3,        m7

    pmulhrsw    m4,        m6
    pmulhrsw    m3,        m6

    packuswb    m4,        m3

    movh        [r2 + r3],      m4
    pextrd      [r2 + r3 + 8],  m4,  2

    lea         r2,        [r2 + 2 * r3]

    sub         r4,        2
    jnz        .loop
    RET
%endmacro

    FILTER_V4_W12_H2 12, 16

    FILTER_V4_W12_H2 12, 32

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_16x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W16_H2 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_16x%2, 4, 6, 8

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [v4_tab_ChromaCoeff]
    movd        m0,        [r5 + r4 * 4]
%else
    movd        m0,        [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m1,        m0,       [tab_Vm]
    pshufb      m0,        [tab_Vm + 16]

    mov         r4d,       %2/2

.loop:
    movu        m2,        [r0]
    movu        m3,        [r0 + r1]

    punpcklbw   m4,        m2,        m3
    punpckhbw   m2,        m3

    pmaddubsw   m4,        m1
    pmaddubsw   m2,        m1

    lea         r0,        [r0 + 2 * r1]
    movu        m5,        [r0]
    movu        m6,        [r0 + r1]

    punpckhbw   m7,        m5,        m6
    pmaddubsw   m7,        m0
    paddw       m2,        m7

    punpcklbw   m7,        m5,        m6
    pmaddubsw   m7,        m0
    paddw       m4,        m7

    mova        m7,        [pw_512]

    pmulhrsw    m4,        m7
    pmulhrsw    m2,        m7

    packuswb    m4,        m2

    movu        [r2],      m4

    punpcklbw   m4,        m3,        m5
    punpckhbw   m3,        m5

    pmaddubsw   m4,        m1
    pmaddubsw   m3,        m1

    movu        m5,        [r0 + 2 * r1]

    punpcklbw   m2,        m6,        m5
    punpckhbw   m6,        m5

    pmaddubsw   m2,        m0
    pmaddubsw   m6,        m0

    paddw       m4,        m2
    paddw       m3,        m6

    pmulhrsw    m4,        m7
    pmulhrsw    m3,        m7

    packuswb    m4,        m3

    movu        [r2 + r3],      m4

    lea         r2,        [r2 + 2 * r3]

    dec         r4d
    jnz        .loop
    RET
%endmacro

    FILTER_V4_W16_H2 16,  4
    FILTER_V4_W16_H2 16,  8
    FILTER_V4_W16_H2 16, 12
    FILTER_V4_W16_H2 16, 16
    FILTER_V4_W16_H2 16, 32

    FILTER_V4_W16_H2 16, 24
    FILTER_V4_W16_H2 16, 64

%macro FILTER_VER_CHROMA_AVX2_16x16 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%1_16x16, 4, 6, 15
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    mova            m12, [r5]
    mova            m13, [r5 + mmsize]
    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m14, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m14, [pw_2000]
%endif
    lea             r5, [r3 * 3]

    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m0, m12
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m1, m12
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, m13
    paddw           m0, m4
    pmaddubsw       m2, m12
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, m13
    paddw           m1, m5
    pmaddubsw       m3, m12
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, m13
    paddw           m2, m6
    pmaddubsw       m4, m12
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, m13
    paddw           m3, m7
    pmaddubsw       m5, m12
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhbw       xm8, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddubsw       m8, m6, m13
    paddw           m4, m8
    pmaddubsw       m6, m12
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhbw       xm9, xm7, xm8
    punpcklbw       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddubsw       m9, m7, m13
    paddw           m5, m9
    pmaddubsw       m7, m12
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhbw       xm10, xm8, xm9
    punpcklbw       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddubsw       m10, m8, m13
    paddw           m6, m10
    pmaddubsw       m8, m12
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhbw       xm11, xm9, xm10
    punpcklbw       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddubsw       m11, m9, m13
    paddw           m7, m11
    pmaddubsw       m9, m12

%ifidn %1,pp
    pmulhrsw        m0, m14                         ; m0 = word: row 0
    pmulhrsw        m1, m14                         ; m1 = word: row 1
    pmulhrsw        m2, m14                         ; m2 = word: row 2
    pmulhrsw        m3, m14                         ; m3 = word: row 3
    pmulhrsw        m4, m14                         ; m4 = word: row 4
    pmulhrsw        m5, m14                         ; m5 = word: row 5
    pmulhrsw        m6, m14                         ; m6 = word: row 6
    pmulhrsw        m7, m14                         ; m7 = word: row 7
    packuswb        m0, m1
    packuswb        m2, m3
    packuswb        m4, m5
    packuswb        m6, m7
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vpermq          m4, m4, 11011000b
    vpermq          m6, m6, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm5, m4, 1
    vextracti128    xm7, m6, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r5], xm3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm4
    movu            [r2 + r3], xm5
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r5], xm7
%else
    psubw           m0, m14                         ; m0 = word: row 0
    psubw           m1, m14                         ; m1 = word: row 1
    psubw           m2, m14                         ; m2 = word: row 2
    psubw           m3, m14                         ; m3 = word: row 3
    psubw           m4, m14                         ; m4 = word: row 4
    psubw           m5, m14                         ; m5 = word: row 5
    psubw           m6, m14                         ; m6 = word: row 6
    psubw           m7, m14                         ; m7 = word: row 7
    movu            [r2], m0
    movu            [r2 + r3], m1
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r5], m3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], m4
    movu            [r2 + r3], m5
    movu            [r2 + r3 * 2], m6
    movu            [r2 + r5], m7
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm11, [r0 + r4]                 ; m11 = row 11
    punpckhbw       xm6, xm10, xm11
    punpcklbw       xm10, xm11
    vinserti128     m10, m10, xm6, 1
    pmaddubsw       m6, m10, m13
    paddw           m8, m6
    pmaddubsw       m10, m12
    lea             r0, [r0 + r1 * 4]
    movu            xm6, [r0]                       ; m6 = row 12
    punpckhbw       xm7, xm11, xm6
    punpcklbw       xm11, xm6
    vinserti128     m11, m11, xm7, 1
    pmaddubsw       m7, m11, m13
    paddw           m9, m7
    pmaddubsw       m11, m12

    movu            xm7, [r0 + r1]                  ; m7 = row 13
    punpckhbw       xm0, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm0, 1
    pmaddubsw       m0, m6, m13
    paddw           m10, m0
    pmaddubsw       m6, m12
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhbw       xm1, xm7, xm0
    punpcklbw       xm7, xm0
    vinserti128     m7, m7, xm1, 1
    pmaddubsw       m1, m7, m13
    paddw           m11, m1
    pmaddubsw       m7, m12
    movu            xm1, [r0 + r4]                  ; m1 = row 15
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m2, m0, m13
    paddw           m6, m2
    pmaddubsw       m0, m12
    lea             r0, [r0 + r1 * 4]
    movu            xm2, [r0]                       ; m2 = row 16
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m3, m1, m13
    paddw           m7, m3
    pmaddubsw       m1, m12
    movu            xm3, [r0 + r1]                  ; m3 = row 17
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m2, m13
    paddw           m0, m2
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m3, m13
    paddw           m1, m3

%ifidn %1,pp
    pmulhrsw        m8, m14                         ; m8 = word: row 8
    pmulhrsw        m9, m14                         ; m9 = word: row 9
    pmulhrsw        m10, m14                        ; m10 = word: row 10
    pmulhrsw        m11, m14                        ; m11 = word: row 11
    pmulhrsw        m6, m14                         ; m6 = word: row 12
    pmulhrsw        m7, m14                         ; m7 = word: row 13
    pmulhrsw        m0, m14                         ; m0 = word: row 14
    pmulhrsw        m1, m14                         ; m1 = word: row 15
    packuswb        m8, m9
    packuswb        m10, m11
    packuswb        m6, m7
    packuswb        m0, m1
    vpermq          m8, m8, 11011000b
    vpermq          m10, m10, 11011000b
    vpermq          m6, m6, 11011000b
    vpermq          m0, m0, 11011000b
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    vextracti128    xm7, m6, 1
    vextracti128    xm1, m0, 1
    movu            [r2], xm8
    movu            [r2 + r3], xm9
    movu            [r2 + r3 * 2], xm10
    movu            [r2 + r5], xm11
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm6
    movu            [r2 + r3], xm7
    movu            [r2 + r3 * 2], xm0
    movu            [r2 + r5], xm1
%else
    psubw           m8, m14                         ; m8 = word: row 8
    psubw           m9, m14                         ; m9 = word: row 9
    psubw           m10, m14                        ; m10 = word: row 10
    psubw           m11, m14                        ; m11 = word: row 11
    psubw           m6, m14                         ; m6 = word: row 12
    psubw           m7, m14                         ; m7 = word: row 13
    psubw           m0, m14                         ; m0 = word: row 14
    psubw           m1, m14                         ; m1 = word: row 15
    movu            [r2], m8
    movu            [r2 + r3], m9
    movu            [r2 + r3 * 2], m10
    movu            [r2 + r5], m11
    lea             r2, [r2 + r3 * 4]
    movu            [r2], m6
    movu            [r2 + r3], m7
    movu            [r2 + r3 * 2], m0
    movu            [r2 + r5], m1
%endif
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_AVX2_16x16 pp
    FILTER_VER_CHROMA_AVX2_16x16 ps
%macro FILTER_VER_CHROMA_AVX2_16x8 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_16x8, 4, 7, 7
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m6, [pw_512]
%else
    add             r3d, r3d
    mova            m6, [pw_2000]
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
    pmaddubsw       m4, m2, [r5 + mmsize]
    paddw           m0, m4
    pmaddubsw       m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, [r5 + mmsize]
    paddw           m1, m5
    pmaddubsw       m3, [r5]
%ifidn %1,pp
    pmulhrsw        m0, m6                          ; m0 = word: row 0
    pmulhrsw        m1, m6                          ; m1 = word: row 1
    packuswb        m0, m1
    vpermq          m0, m0, 11011000b
    vextracti128    xm1, m0, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
%else
    psubw           m0, m6                          ; m0 = word: row 0
    psubw           m1, m6                          ; m1 = word: row 1
    movu            [r2], m0
    movu            [r2 + r3], m1
%endif

    movu            xm0, [r0 + r1]                  ; m0 = row 5
    punpckhbw       xm1, xm4, xm0
    punpcklbw       xm4, xm0
    vinserti128     m4, m4, xm1, 1
    pmaddubsw       m1, m4, [r5 + mmsize]
    paddw           m2, m1
    pmaddubsw       m4, [r5]
    movu            xm1, [r0 + r1 * 2]              ; m1 = row 6
    punpckhbw       xm5, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm5, 1
    pmaddubsw       m5, m0, [r5 + mmsize]
    paddw           m3, m5
    pmaddubsw       m0, [r5]
%ifidn %1,pp
    pmulhrsw        m2, m6                          ; m2 = word: row 2
    pmulhrsw        m3, m6                          ; m3 = word: row 3
    packuswb        m2, m3
    vpermq          m2, m2, 11011000b
    vextracti128    xm3, m2, 1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
%else
    psubw           m2, m6                          ; m2 = word: row 2
    psubw           m3, m6                          ; m3 = word: row 3
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r6], m3
%endif

    movu            xm2, [r0 + r4]                  ; m2 = row 7
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m3, m1, [r5 + mmsize]
    paddw           m4, m3
    pmaddubsw       m1, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm3, [r0]                       ; m3 = row 8
    punpckhbw       xm5, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm5, 1
    pmaddubsw       m5, m2, [r5 + mmsize]
    paddw           m0, m5
    pmaddubsw       m2, [r5]
    lea             r2, [r2 + r3 * 4]
%ifidn %1,pp
    pmulhrsw        m4, m6                          ; m4 = word: row 4
    pmulhrsw        m0, m6                          ; m0 = word: row 5
    packuswb        m4, m0
    vpermq          m4, m4, 11011000b
    vextracti128    xm0, m4, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm0
%else
    psubw           m4, m6                          ; m4 = word: row 4
    psubw           m0, m6                          ; m0 = word: row 5
    movu            [r2], m4
    movu            [r2 + r3], m0
%endif

    movu            xm5, [r0 + r1]                  ; m5 = row 9
    punpckhbw       xm4, xm3, xm5
    punpcklbw       xm3, xm5
    vinserti128     m3, m3, xm4, 1
    pmaddubsw       m3, [r5 + mmsize]
    paddw           m1, m3
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 10
    punpckhbw       xm0, xm5, xm4
    punpcklbw       xm5, xm4
    vinserti128     m5, m5, xm0, 1
    pmaddubsw       m5, [r5 + mmsize]
    paddw           m2, m5
%ifidn %1,pp
    pmulhrsw        m1, m6                          ; m1 = word: row 6
    pmulhrsw        m2, m6                          ; m2 = word: row 7
    packuswb        m1, m2
    vpermq          m1, m1, 11011000b
    vextracti128    xm2, m1, 1
    movu            [r2 + r3 * 2], xm1
    movu            [r2 + r6], xm2
%else
    psubw           m1, m6                          ; m1 = word: row 6
    psubw           m2, m6                          ; m2 = word: row 7
    movu            [r2 + r3 * 2], m1
    movu            [r2 + r6], m2
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_16x8 pp
    FILTER_VER_CHROMA_AVX2_16x8 ps

%macro FILTER_VER_CHROMA_AVX2_16x12 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%1_16x12, 4, 6, 10
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    mova            m8, [r5]
    mova            m9, [r5 + mmsize]
    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m7, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m7, [pw_2000]
%endif
    lea             r5, [r3 * 3]

    movu            xm0, [r0]
    vinserti128     m0, m0, [r0 + r1 * 2], 1
    movu            xm1, [r0 + r1]
    vinserti128     m1, m1, [r0 + r4], 1

    punpcklbw       m2, m0, m1
    punpckhbw       m3, m0, m1
    vperm2i128      m4, m2, m3, 0x20
    vperm2i128      m2, m2, m3, 0x31
    pmaddubsw       m4, m8
    pmaddubsw       m3, m2, m9
    paddw           m4, m3
    pmaddubsw       m2, m8

    vextracti128    xm0, m0, 1
    lea             r0, [r0 + r1 * 4]
    vinserti128     m0, m0, [r0], 1

    punpcklbw       m5, m1, m0
    punpckhbw       m3, m1, m0
    vperm2i128      m6, m5, m3, 0x20
    vperm2i128      m5, m5, m3, 0x31
    pmaddubsw       m6, m8
    pmaddubsw       m3, m5, m9
    paddw           m6, m3
    pmaddubsw       m5, m8
%ifidn %1,pp
    pmulhrsw        m4, m7                         ; m4 = word: row 0
    pmulhrsw        m6, m7                         ; m6 = word: row 1
    packuswb        m4, m6
    vpermq          m4, m4, 11011000b
    vextracti128    xm6, m4, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm6
%else
    psubw           m4, m7                         ; m4 = word: row 0
    psubw           m6, m7                         ; m6 = word: row 1
    movu            [r2], m4
    movu            [r2 + r3], m6
%endif

    movu            xm4, [r0 + r1 * 2]
    vinserti128     m4, m4, [r0 + r1], 1
    vextracti128    xm1, m4, 1
    vinserti128     m0, m0, xm1, 0

    punpcklbw       m6, m0, m4
    punpckhbw       m1, m0, m4
    vperm2i128      m0, m6, m1, 0x20
    vperm2i128      m6, m6, m1, 0x31
    pmaddubsw       m1, m0, m9
    paddw           m5, m1
    pmaddubsw       m0, m8
    pmaddubsw       m1, m6, m9
    paddw           m2, m1
    pmaddubsw       m6, m8

%ifidn %1,pp
    pmulhrsw        m2, m7                         ; m2 = word: row 2
    pmulhrsw        m5, m7                         ; m5 = word: row 3
    packuswb        m2, m5
    vpermq          m2, m2, 11011000b
    vextracti128    xm5, m2, 1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r5], xm5
%else
    psubw           m2, m7                         ; m2 = word: row 2
    psubw           m5, m7                         ; m5 = word: row 3
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r5], m5
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm1, [r0 + r4]
    lea             r0, [r0 + r1 * 4]
    vinserti128     m1, m1, [r0], 1
    vinserti128     m4, m4, xm1, 1

    punpcklbw       m2, m4, m1
    punpckhbw       m5, m4, m1
    vperm2i128      m3, m2, m5, 0x20
    vperm2i128      m2, m2, m5, 0x31
    pmaddubsw       m5, m3, m9
    paddw           m6, m5
    pmaddubsw       m3, m8
    pmaddubsw       m5, m2, m9
    paddw           m0, m5
    pmaddubsw       m2, m8

%ifidn %1,pp
    pmulhrsw        m6, m7                         ; m6 = word: row 4
    pmulhrsw        m0, m7                         ; m0 = word: row 5
    packuswb        m6, m0
    vpermq          m6, m6, 11011000b
    vextracti128    xm0, m6, 1
    movu            [r2], xm6
    movu            [r2 + r3], xm0
%else
    psubw           m6, m7                         ; m6 = word: row 4
    psubw           m0, m7                         ; m0 = word: row 5
    movu            [r2], m6
    movu            [r2 + r3], m0
%endif

    movu            xm6, [r0 + r1 * 2]
    vinserti128     m6, m6, [r0 + r1], 1
    vextracti128    xm0, m6, 1
    vinserti128     m1, m1, xm0, 0

    punpcklbw       m4, m1, m6
    punpckhbw       m5, m1, m6
    vperm2i128      m0, m4, m5, 0x20
    vperm2i128      m5, m4, m5, 0x31
    pmaddubsw       m4, m0, m9
    paddw           m2, m4
    pmaddubsw       m0, m8
    pmaddubsw       m4, m5, m9
    paddw           m3, m4
    pmaddubsw       m5, m8

%ifidn %1,pp
    pmulhrsw        m3, m7                         ; m3 = word: row 6
    pmulhrsw        m2, m7                         ; m2 = word: row 7
    packuswb        m3, m2
    vpermq          m3, m3, 11011000b
    vextracti128    xm2, m3, 1
    movu            [r2 + r3 * 2], xm3
    movu            [r2 + r5], xm2
%else
    psubw           m3, m7                         ; m3 = word: row 6
    psubw           m2, m7                         ; m2 = word: row 7
    movu            [r2 + r3 * 2], m3
    movu            [r2 + r5], m2
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm3, [r0 + r4]
    lea             r0, [r0 + r1 * 4]
    vinserti128     m3, m3, [r0], 1
    vinserti128     m6, m6, xm3, 1

    punpcklbw       m2, m6, m3
    punpckhbw       m1, m6, m3
    vperm2i128      m4, m2, m1, 0x20
    vperm2i128      m2, m2, m1, 0x31
    pmaddubsw       m1, m4, m9
    paddw           m5, m1
    pmaddubsw       m4, m8
    pmaddubsw       m1, m2, m9
    paddw           m0, m1
    pmaddubsw       m2, m8

%ifidn %1,pp
    pmulhrsw        m5, m7                         ; m5 = word: row 8
    pmulhrsw        m0, m7                         ; m0 = word: row 9
    packuswb        m5, m0
    vpermq          m5, m5, 11011000b
    vextracti128    xm0, m5, 1
    movu            [r2], xm5
    movu            [r2 + r3], xm0
%else
    psubw           m5, m7                         ; m5 = word: row 8
    psubw           m0, m7                         ; m0 = word: row 9
    movu            [r2], m5
    movu            [r2 + r3], m0
%endif

    movu            xm5, [r0 + r1 * 2]
    vinserti128     m5, m5, [r0 + r1], 1
    vextracti128    xm0, m5, 1
    vinserti128     m3, m3, xm0, 0

    punpcklbw       m1, m3, m5
    punpckhbw       m0, m3, m5
    vperm2i128      m6, m1, m0, 0x20
    vperm2i128      m0, m1, m0, 0x31
    pmaddubsw       m1, m6, m9
    paddw           m2, m1
    pmaddubsw       m1, m0, m9
    paddw           m4, m1

%ifidn %1,pp
    pmulhrsw        m4, m7                         ; m4 = word: row 10
    pmulhrsw        m2, m7                         ; m2 = word: row 11
    packuswb        m4, m2
    vpermq          m4, m4, 11011000b
    vextracti128    xm2, m4, 1
    movu            [r2 + r3 * 2], xm4
    movu            [r2 + r5], xm2
%else
    psubw           m4, m7                         ; m4 = word: row 10
    psubw           m2, m7                         ; m2 = word: row 11
    movu            [r2 + r3 * 2], m4
    movu            [r2 + r5], m2
%endif
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_AVX2_16x12 pp
    FILTER_VER_CHROMA_AVX2_16x12 ps

%macro FILTER_VER_CHROMA_AVX2_16xN 2
%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_16x%2, 4, 8, 8
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m7, [pw_512]
%else
    add             r3d, r3d
    mova            m7, [pw_2000]
%endif
    lea             r6, [r3 * 3]
    mov             r7d, %2 / 16
.loopH:
    movu            xm0, [r0]
    vinserti128     m0, m0, [r0 + r1 * 2], 1
    movu            xm1, [r0 + r1]
    vinserti128     m1, m1, [r0 + r4], 1

    punpcklbw       m2, m0, m1
    punpckhbw       m3, m0, m1
    vperm2i128      m4, m2, m3, 0x20
    vperm2i128      m2, m2, m3, 0x31
    pmaddubsw       m4, [r5]
    pmaddubsw       m3, m2, [r5 + mmsize]
    paddw           m4, m3
    pmaddubsw       m2, [r5]

    vextracti128    xm0, m0, 1
    lea             r0, [r0 + r1 * 4]
    vinserti128     m0, m0, [r0], 1

    punpcklbw       m5, m1, m0
    punpckhbw       m3, m1, m0
    vperm2i128      m6, m5, m3, 0x20
    vperm2i128      m5, m5, m3, 0x31
    pmaddubsw       m6, [r5]
    pmaddubsw       m3, m5, [r5 + mmsize]
    paddw           m6, m3
    pmaddubsw       m5, [r5]
%ifidn %1,pp
    pmulhrsw        m4, m7                         ; m4 = word: row 0
    pmulhrsw        m6, m7                         ; m6 = word: row 1
    packuswb        m4, m6
    vpermq          m4, m4, 11011000b
    vextracti128    xm6, m4, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm6
%else
    psubw           m4, m7                         ; m4 = word: row 0
    psubw           m6, m7                         ; m6 = word: row 1
    movu            [r2], m4
    movu            [r2 + r3], m6
%endif

    movu            xm4, [r0 + r1 * 2]
    vinserti128     m4, m4, [r0 + r1], 1
    vextracti128    xm1, m4, 1
    vinserti128     m0, m0, xm1, 0

    punpcklbw       m6, m0, m4
    punpckhbw       m1, m0, m4
    vperm2i128      m0, m6, m1, 0x20
    vperm2i128      m6, m6, m1, 0x31
    pmaddubsw       m1, m0, [r5 + mmsize]
    paddw           m5, m1
    pmaddubsw       m0, [r5]
    pmaddubsw       m1, m6, [r5 + mmsize]
    paddw           m2, m1
    pmaddubsw       m6, [r5]

%ifidn %1,pp
    pmulhrsw        m2, m7                         ; m2 = word: row 2
    pmulhrsw        m5, m7                         ; m5 = word: row 3
    packuswb        m2, m5
    vpermq          m2, m2, 11011000b
    vextracti128    xm5, m2, 1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm5
%else
    psubw           m2, m7                         ; m2 = word: row 2
    psubw           m5, m7                         ; m5 = word: row 3
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r6], m5
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm1, [r0 + r4]
    lea             r0, [r0 + r1 * 4]
    vinserti128     m1, m1, [r0], 1
    vinserti128     m4, m4, xm1, 1

    punpcklbw       m2, m4, m1
    punpckhbw       m5, m4, m1
    vperm2i128      m3, m2, m5, 0x20
    vperm2i128      m2, m2, m5, 0x31
    pmaddubsw       m5, m3, [r5 + mmsize]
    paddw           m6, m5
    pmaddubsw       m3, [r5]
    pmaddubsw       m5, m2, [r5 + mmsize]
    paddw           m0, m5
    pmaddubsw       m2, [r5]

%ifidn %1,pp
    pmulhrsw        m6, m7                         ; m6 = word: row 4
    pmulhrsw        m0, m7                         ; m0 = word: row 5
    packuswb        m6, m0
    vpermq          m6, m6, 11011000b
    vextracti128    xm0, m6, 1
    movu            [r2], xm6
    movu            [r2 + r3], xm0
%else
    psubw           m6, m7                         ; m6 = word: row 4
    psubw           m0, m7                         ; m0 = word: row 5
    movu            [r2], m6
    movu            [r2 + r3], m0
%endif

    movu            xm6, [r0 + r1 * 2]
    vinserti128     m6, m6, [r0 + r1], 1
    vextracti128    xm0, m6, 1
    vinserti128     m1, m1, xm0, 0

    punpcklbw       m4, m1, m6
    punpckhbw       m5, m1, m6
    vperm2i128      m0, m4, m5, 0x20
    vperm2i128      m5, m4, m5, 0x31
    pmaddubsw       m4, m0, [r5 + mmsize]
    paddw           m2, m4
    pmaddubsw       m0, [r5]
    pmaddubsw       m4, m5, [r5 + mmsize]
    paddw           m3, m4
    pmaddubsw       m5, [r5]

%ifidn %1,pp
    pmulhrsw        m3, m7                         ; m3 = word: row 6
    pmulhrsw        m2, m7                         ; m2 = word: row 7
    packuswb        m3, m2
    vpermq          m3, m3, 11011000b
    vextracti128    xm2, m3, 1
    movu            [r2 + r3 * 2], xm3
    movu            [r2 + r6], xm2
%else
    psubw           m3, m7                         ; m3 = word: row 6
    psubw           m2, m7                         ; m2 = word: row 7
    movu            [r2 + r3 * 2], m3
    movu            [r2 + r6], m2
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm3, [r0 + r4]
    lea             r0, [r0 + r1 * 4]
    vinserti128     m3, m3, [r0], 1
    vinserti128     m6, m6, xm3, 1

    punpcklbw       m2, m6, m3
    punpckhbw       m1, m6, m3
    vperm2i128      m4, m2, m1, 0x20
    vperm2i128      m2, m2, m1, 0x31
    pmaddubsw       m1, m4, [r5 + mmsize]
    paddw           m5, m1
    pmaddubsw       m4, [r5]
    pmaddubsw       m1, m2, [r5 + mmsize]
    paddw           m0, m1
    pmaddubsw       m2, [r5]

%ifidn %1,pp
    pmulhrsw        m5, m7                         ; m5 = word: row 8
    pmulhrsw        m0, m7                         ; m0 = word: row 9
    packuswb        m5, m0
    vpermq          m5, m5, 11011000b
    vextracti128    xm0, m5, 1
    movu            [r2], xm5
    movu            [r2 + r3], xm0
%else
    psubw           m5, m7                         ; m5 = word: row 8
    psubw           m0, m7                         ; m0 = word: row 9
    movu            [r2], m5
    movu            [r2 + r3], m0
%endif

    movu            xm5, [r0 + r1 * 2]
    vinserti128     m5, m5, [r0 + r1], 1
    vextracti128    xm0, m5, 1
    vinserti128     m3, m3, xm0, 0

    punpcklbw       m1, m3, m5
    punpckhbw       m0, m3, m5
    vperm2i128      m6, m1, m0, 0x20
    vperm2i128      m0, m1, m0, 0x31
    pmaddubsw       m1, m6, [r5 + mmsize]
    paddw           m2, m1
    pmaddubsw       m6, [r5]
    pmaddubsw       m1, m0, [r5 + mmsize]
    paddw           m4, m1
    pmaddubsw       m0, [r5]

%ifidn %1,pp
    pmulhrsw        m4, m7                         ; m4 = word: row 10
    pmulhrsw        m2, m7                         ; m2 = word: row 11
    packuswb        m4, m2
    vpermq          m4, m4, 11011000b
    vextracti128    xm2, m4, 1
    movu            [r2 + r3 * 2], xm4
    movu            [r2 + r6], xm2
%else
    psubw           m4, m7                         ; m4 = word: row 10
    psubw           m2, m7                         ; m2 = word: row 11
    movu            [r2 + r3 * 2], m4
    movu            [r2 + r6], m2
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm3, [r0 + r4]
    lea             r0, [r0 + r1 * 4]
    vinserti128     m3, m3, [r0], 1
    vinserti128     m5, m5, xm3, 1

    punpcklbw       m2, m5, m3
    punpckhbw       m1, m5, m3
    vperm2i128      m4, m2, m1, 0x20
    vperm2i128      m2, m2, m1, 0x31
    pmaddubsw       m1, m4, [r5 + mmsize]
    paddw           m0, m1
    pmaddubsw       m4, [r5]
    pmaddubsw       m1, m2, [r5 + mmsize]
    paddw           m6, m1
    pmaddubsw       m2, [r5]

%ifidn %1,pp
    pmulhrsw        m0, m7                         ; m0 = word: row 12
    pmulhrsw        m6, m7                         ; m6 = word: row 13
    packuswb        m0, m6
    vpermq          m0, m0, 11011000b
    vextracti128    xm6, m0, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm6
%else
    psubw           m0, m7                         ; m0 = word: row 12
    psubw           m6, m7                         ; m6 = word: row 13
    movu            [r2], m0
    movu            [r2 + r3], m6
%endif

    movu            xm5, [r0 + r1 * 2]
    vinserti128     m5, m5, [r0 + r1], 1
    vextracti128    xm0, m5, 1
    vinserti128     m3, m3, xm0, 0

    punpcklbw       m1, m3, m5
    punpckhbw       m0, m3, m5
    vperm2i128      m6, m1, m0, 0x20
    vperm2i128      m0, m1, m0, 0x31
    pmaddubsw       m6, [r5 + mmsize]
    paddw           m2, m6
    pmaddubsw       m0, [r5 + mmsize]
    paddw           m4, m0

%ifidn %1,pp
    pmulhrsw        m4, m7                         ; m4 = word: row 14
    pmulhrsw        m2, m7                         ; m2 = word: row 15
    packuswb        m4, m2
    vpermq          m4, m4, 11011000b
    vextracti128    xm2, m4, 1
    movu            [r2 + r3 * 2], xm4
    movu            [r2 + r6], xm2
%else
    psubw           m4, m7                         ; m4 = word: row 14
    psubw           m2, m7                         ; m2 = word: row 15
    movu            [r2 + r3 * 2], m4
    movu            [r2 + r6], m2
%endif
    lea             r2, [r2 + r3 * 4]
    dec             r7d
    jnz             .loopH
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_AVX2_16xN pp, 32
    FILTER_VER_CHROMA_AVX2_16xN ps, 32
    FILTER_VER_CHROMA_AVX2_16xN pp, 64
    FILTER_VER_CHROMA_AVX2_16xN ps, 64

%macro FILTER_VER_CHROMA_AVX2_16x24 1
%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_16x24, 4, 6, 15
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    mova            m12, [r5]
    mova            m13, [r5 + mmsize]
    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m14, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m14, [pw_2000]
%endif
    lea             r5, [r3 * 3]

    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m0, m12
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m1, m12
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, m13
    paddw           m0, m4
    pmaddubsw       m2, m12
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, m13
    paddw           m1, m5
    pmaddubsw       m3, m12
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, m13
    paddw           m2, m6
    pmaddubsw       m4, m12
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, m13
    paddw           m3, m7
    pmaddubsw       m5, m12
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhbw       xm8, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddubsw       m8, m6, m13
    paddw           m4, m8
    pmaddubsw       m6, m12
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhbw       xm9, xm7, xm8
    punpcklbw       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddubsw       m9, m7, m13
    paddw           m5, m9
    pmaddubsw       m7, m12
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhbw       xm10, xm8, xm9
    punpcklbw       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddubsw       m10, m8, m13
    paddw           m6, m10
    pmaddubsw       m8, m12
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhbw       xm11, xm9, xm10
    punpcklbw       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddubsw       m11, m9, m13
    paddw           m7, m11
    pmaddubsw       m9, m12

%ifidn %1,pp
    pmulhrsw        m0, m14                         ; m0 = word: row 0
    pmulhrsw        m1, m14                         ; m1 = word: row 1
    pmulhrsw        m2, m14                         ; m2 = word: row 2
    pmulhrsw        m3, m14                         ; m3 = word: row 3
    pmulhrsw        m4, m14                         ; m4 = word: row 4
    pmulhrsw        m5, m14                         ; m5 = word: row 5
    pmulhrsw        m6, m14                         ; m6 = word: row 6
    pmulhrsw        m7, m14                         ; m7 = word: row 7
    packuswb        m0, m1
    packuswb        m2, m3
    packuswb        m4, m5
    packuswb        m6, m7
    vpermq          m0, m0, q3120
    vpermq          m2, m2, q3120
    vpermq          m4, m4, q3120
    vpermq          m6, m6, q3120
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm5, m4, 1
    vextracti128    xm7, m6, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r5], xm3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm4
    movu            [r2 + r3], xm5
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r5], xm7
%else
    psubw           m0, m14                         ; m0 = word: row 0
    psubw           m1, m14                         ; m1 = word: row 1
    psubw           m2, m14                         ; m2 = word: row 2
    psubw           m3, m14                         ; m3 = word: row 3
    psubw           m4, m14                         ; m4 = word: row 4
    psubw           m5, m14                         ; m5 = word: row 5
    psubw           m6, m14                         ; m6 = word: row 6
    psubw           m7, m14                         ; m7 = word: row 7
    movu            [r2], m0
    movu            [r2 + r3], m1
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r5], m3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], m4
    movu            [r2 + r3], m5
    movu            [r2 + r3 * 2], m6
    movu            [r2 + r5], m7
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm11, [r0 + r4]                 ; m11 = row 11
    punpckhbw       xm6, xm10, xm11
    punpcklbw       xm10, xm11
    vinserti128     m10, m10, xm6, 1
    pmaddubsw       m6, m10, m13
    paddw           m8, m6
    pmaddubsw       m10, m12
    lea             r0, [r0 + r1 * 4]
    movu            xm6, [r0]                       ; m6 = row 12
    punpckhbw       xm7, xm11, xm6
    punpcklbw       xm11, xm6
    vinserti128     m11, m11, xm7, 1
    pmaddubsw       m7, m11, m13
    paddw           m9, m7
    pmaddubsw       m11, m12

    movu            xm7, [r0 + r1]                  ; m7 = row 13
    punpckhbw       xm0, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm0, 1
    pmaddubsw       m0, m6, m13
    paddw           m10, m0
    pmaddubsw       m6, m12
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhbw       xm1, xm7, xm0
    punpcklbw       xm7, xm0
    vinserti128     m7, m7, xm1, 1
    pmaddubsw       m1, m7, m13
    paddw           m11, m1
    pmaddubsw       m7, m12
    movu            xm1, [r0 + r4]                  ; m1 = row 15
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m2, m0, m13
    paddw           m6, m2
    pmaddubsw       m0, m12
    lea             r0, [r0 + r1 * 4]
    movu            xm2, [r0]                       ; m2 = row 16
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m3, m1, m13
    paddw           m7, m3
    pmaddubsw       m1, m12
    movu            xm3, [r0 + r1]                  ; m3 = row 17
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, m13
    paddw           m0, m4
    pmaddubsw       m2, m12
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, m13
    paddw           m1, m5
    pmaddubsw       m3, m12

%ifidn %1,pp
    pmulhrsw        m8, m14                         ; m8 = word: row 8
    pmulhrsw        m9, m14                         ; m9 = word: row 9
    pmulhrsw        m10, m14                        ; m10 = word: row 10
    pmulhrsw        m11, m14                        ; m11 = word: row 11
    pmulhrsw        m6, m14                         ; m6 = word: row 12
    pmulhrsw        m7, m14                         ; m7 = word: row 13
    pmulhrsw        m0, m14                         ; m0 = word: row 14
    pmulhrsw        m1, m14                         ; m1 = word: row 15
    packuswb        m8, m9
    packuswb        m10, m11
    packuswb        m6, m7
    packuswb        m0, m1
    vpermq          m8, m8, q3120
    vpermq          m10, m10, q3120
    vpermq          m6, m6, q3120
    vpermq          m0, m0, q3120
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    vextracti128    xm7, m6, 1
    vextracti128    xm1, m0, 1
    movu            [r2], xm8
    movu            [r2 + r3], xm9
    movu            [r2 + r3 * 2], xm10
    movu            [r2 + r5], xm11
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm6
    movu            [r2 + r3], xm7
    movu            [r2 + r3 * 2], xm0
    movu            [r2 + r5], xm1
%else
    psubw           m8, m14                         ; m8 = word: row 8
    psubw           m9, m14                         ; m9 = word: row 9
    psubw           m10, m14                        ; m10 = word: row 10
    psubw           m11, m14                        ; m11 = word: row 11
    psubw           m6, m14                         ; m6 = word: row 12
    psubw           m7, m14                         ; m7 = word: row 13
    psubw           m0, m14                         ; m0 = word: row 14
    psubw           m1, m14                         ; m1 = word: row 15
    movu            [r2], m8
    movu            [r2 + r3], m9
    movu            [r2 + r3 * 2], m10
    movu            [r2 + r5], m11
    lea             r2, [r2 + r3 * 4]
    movu            [r2], m6
    movu            [r2 + r3], m7
    movu            [r2 + r3 * 2], m0
    movu            [r2 + r5], m1
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm5, [r0 + r4]                  ; m5 = row 19
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, m13
    paddw           m2, m6
    pmaddubsw       m4, m12
    lea             r0, [r0 + r1 * 4]
    movu            xm6, [r0]                       ; m6 = row 20
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m7, m5, m13
    paddw           m3, m7
    pmaddubsw       m5, m12
    movu            xm7, [r0 + r1]                  ; m7 = row 21
    punpckhbw       xm0, xm6, xm7
    punpcklbw       xm6, xm7
    vinserti128     m6, m6, xm0, 1
    pmaddubsw       m0, m6, m13
    paddw           m4, m0
    pmaddubsw       m6, m12
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 22
    punpckhbw       xm1, xm7, xm0
    punpcklbw       xm7, xm0
    vinserti128     m7, m7, xm1, 1
    pmaddubsw       m1, m7, m13
    paddw           m5, m1
    pmaddubsw       m7, m12
    movu            xm1, [r0 + r4]                  ; m1 = row 23
    punpckhbw       xm8, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm8, 1
    pmaddubsw       m8, m0, m13
    paddw           m6, m8
    pmaddubsw       m0, m12
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 24
    punpckhbw       xm9, xm1, xm8
    punpcklbw       xm1, xm8
    vinserti128     m1, m1, xm9, 1
    pmaddubsw       m9, m1, m13
    paddw           m7, m9
    pmaddubsw       m1, m12
    movu            xm9, [r0 + r1]                  ; m9 = row 25
    punpckhbw       xm10, xm8, xm9
    punpcklbw       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddubsw       m8, m13
    paddw           m0, m8
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 26
    punpckhbw       xm11, xm9, xm10
    punpcklbw       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddubsw       m9, m13
    paddw           m1, m9

%ifidn %1,pp
    pmulhrsw        m2, m14                         ; m2 = word: row 16
    pmulhrsw        m3, m14                         ; m3 = word: row 17
    pmulhrsw        m4, m14                         ; m4 = word: row 18
    pmulhrsw        m5, m14                         ; m5 = word: row 19
    pmulhrsw        m6, m14                         ; m6 = word: row 20
    pmulhrsw        m7, m14                         ; m7 = word: row 21
    pmulhrsw        m0, m14                         ; m0 = word: row 22
    pmulhrsw        m1, m14                         ; m1 = word: row 23
    packuswb        m2, m3
    packuswb        m4, m5
    packuswb        m6, m7
    packuswb        m0, m1
    vpermq          m2, m2, q3120
    vpermq          m4, m4, q3120
    vpermq          m6, m6, q3120
    vpermq          m0, m0, q3120
    vextracti128    xm3, m2, 1
    vextracti128    xm5, m4, 1
    vextracti128    xm7, m6, 1
    vextracti128    xm1, m0, 1
    movu            [r2], xm2
    movu            [r2 + r3], xm3
    movu            [r2 + r3 * 2], xm4
    movu            [r2 + r5], xm5
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm6
    movu            [r2 + r3], xm7
    movu            [r2 + r3 * 2], xm0
    movu            [r2 + r5], xm1
%else
    psubw           m2, m14                         ; m2 = word: row 16
    psubw           m3, m14                         ; m3 = word: row 17
    psubw           m4, m14                         ; m4 = word: row 18
    psubw           m5, m14                         ; m5 = word: row 19
    psubw           m6, m14                         ; m6 = word: row 20
    psubw           m7, m14                         ; m7 = word: row 21
    psubw           m0, m14                         ; m0 = word: row 22
    psubw           m1, m14                         ; m1 = word: row 23
    movu            [r2], m2
    movu            [r2 + r3], m3
    movu            [r2 + r3 * 2], m4
    movu            [r2 + r5], m5
    lea             r2, [r2 + r3 * 4]
    movu            [r2], m6
    movu            [r2 + r3], m7
    movu            [r2 + r3 * 2], m0
    movu            [r2 + r5], m1
%endif
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_AVX2_16x24 pp
    FILTER_VER_CHROMA_AVX2_16x24 ps

%macro FILTER_VER_CHROMA_AVX2_24x32 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%1_24x32, 4, 9, 10
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    mova            m8, [r5]
    mova            m9, [r5 + mmsize]
    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m7, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m7, [pw_2000]
%endif
    lea             r6, [r3 * 3]
    mov             r5d, 2
.loopH:
    movu            xm0, [r0]
    vinserti128     m0, m0, [r0 + r1 * 2], 1
    movu            xm1, [r0 + r1]
    vinserti128     m1, m1, [r0 + r4], 1

    punpcklbw       m2, m0, m1
    punpckhbw       m3, m0, m1
    vperm2i128      m4, m2, m3, 0x20
    vperm2i128      m2, m2, m3, 0x31
    pmaddubsw       m4, m8
    pmaddubsw       m3, m2, m9
    paddw           m4, m3
    pmaddubsw       m2, m8

    vextracti128    xm0, m0, 1
    lea             r7, [r0 + r1 * 4]
    vinserti128     m0, m0, [r7], 1

    punpcklbw       m5, m1, m0
    punpckhbw       m3, m1, m0
    vperm2i128      m6, m5, m3, 0x20
    vperm2i128      m5, m5, m3, 0x31
    pmaddubsw       m6, m8
    pmaddubsw       m3, m5, m9
    paddw           m6, m3
    pmaddubsw       m5, m8
%ifidn %1,pp
    pmulhrsw        m4, m7                         ; m4 = word: row 0
    pmulhrsw        m6, m7                         ; m6 = word: row 1
    packuswb        m4, m6
    vpermq          m4, m4, 11011000b
    vextracti128    xm6, m4, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm6
%else
    psubw           m4, m7                         ; m4 = word: row 0
    psubw           m6, m7                         ; m6 = word: row 1
    movu            [r2], m4
    movu            [r2 + r3], m6
%endif

    movu            xm4, [r7 + r1 * 2]
    vinserti128     m4, m4, [r7 + r1], 1
    vextracti128    xm1, m4, 1
    vinserti128     m0, m0, xm1, 0

    punpcklbw       m6, m0, m4
    punpckhbw       m1, m0, m4
    vperm2i128      m0, m6, m1, 0x20
    vperm2i128      m6, m6, m1, 0x31
    pmaddubsw       m1, m0, m9
    paddw           m5, m1
    pmaddubsw       m0, m8
    pmaddubsw       m1, m6, m9
    paddw           m2, m1
    pmaddubsw       m6, m8

%ifidn %1,pp
    pmulhrsw        m2, m7                         ; m2 = word: row 2
    pmulhrsw        m5, m7                         ; m5 = word: row 3
    packuswb        m2, m5
    vpermq          m2, m2, 11011000b
    vextracti128    xm5, m2, 1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm5
%else
    psubw           m2, m7                         ; m2 = word: row 2
    psubw           m5, m7                         ; m5 = word: row 3
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r6], m5
%endif
    lea             r8, [r2 + r3 * 4]

    movu            xm1, [r7 + r4]
    lea             r7, [r7 + r1 * 4]
    vinserti128     m1, m1, [r7], 1
    vinserti128     m4, m4, xm1, 1

    punpcklbw       m2, m4, m1
    punpckhbw       m5, m4, m1
    vperm2i128      m3, m2, m5, 0x20
    vperm2i128      m2, m2, m5, 0x31
    pmaddubsw       m5, m3, m9
    paddw           m6, m5
    pmaddubsw       m3, m8
    pmaddubsw       m5, m2, m9
    paddw           m0, m5
    pmaddubsw       m2, m8

%ifidn %1,pp
    pmulhrsw        m6, m7                         ; m6 = word: row 4
    pmulhrsw        m0, m7                         ; m0 = word: row 5
    packuswb        m6, m0
    vpermq          m6, m6, 11011000b
    vextracti128    xm0, m6, 1
    movu            [r8], xm6
    movu            [r8 + r3], xm0
%else
    psubw           m6, m7                         ; m6 = word: row 4
    psubw           m0, m7                         ; m0 = word: row 5
    movu            [r8], m6
    movu            [r8 + r3], m0
%endif

    movu            xm6, [r7 + r1 * 2]
    vinserti128     m6, m6, [r7 + r1], 1
    vextracti128    xm0, m6, 1
    vinserti128     m1, m1, xm0, 0

    punpcklbw       m4, m1, m6
    punpckhbw       m5, m1, m6
    vperm2i128      m0, m4, m5, 0x20
    vperm2i128      m5, m4, m5, 0x31
    pmaddubsw       m4, m0, m9
    paddw           m2, m4
    pmaddubsw       m0, m8
    pmaddubsw       m4, m5, m9
    paddw           m3, m4
    pmaddubsw       m5, m8

%ifidn %1,pp
    pmulhrsw        m3, m7                         ; m3 = word: row 6
    pmulhrsw        m2, m7                         ; m2 = word: row 7
    packuswb        m3, m2
    vpermq          m3, m3, 11011000b
    vextracti128    xm2, m3, 1
    movu            [r8 + r3 * 2], xm3
    movu            [r8 + r6], xm2
%else
    psubw           m3, m7                         ; m3 = word: row 6
    psubw           m2, m7                         ; m2 = word: row 7
    movu            [r8 + r3 * 2], m3
    movu            [r8 + r6], m2
%endif
    lea             r8, [r8 + r3 * 4]

    movu            xm3, [r7 + r4]
    lea             r7, [r7 + r1 * 4]
    vinserti128     m3, m3, [r7], 1
    vinserti128     m6, m6, xm3, 1

    punpcklbw       m2, m6, m3
    punpckhbw       m1, m6, m3
    vperm2i128      m4, m2, m1, 0x20
    vperm2i128      m2, m2, m1, 0x31
    pmaddubsw       m1, m4, m9
    paddw           m5, m1
    pmaddubsw       m4, m8
    pmaddubsw       m1, m2, m9
    paddw           m0, m1
    pmaddubsw       m2, m8

%ifidn %1,pp
    pmulhrsw        m5, m7                         ; m5 = word: row 8
    pmulhrsw        m0, m7                         ; m0 = word: row 9
    packuswb        m5, m0
    vpermq          m5, m5, 11011000b
    vextracti128    xm0, m5, 1
    movu            [r8], xm5
    movu            [r8 + r3], xm0
%else
    psubw           m5, m7                         ; m5 = word: row 8
    psubw           m0, m7                         ; m0 = word: row 9
    movu            [r8], m5
    movu            [r8 + r3], m0
%endif

    movu            xm5, [r7 + r1 * 2]
    vinserti128     m5, m5, [r7 + r1], 1
    vextracti128    xm0, m5, 1
    vinserti128     m3, m3, xm0, 0

    punpcklbw       m1, m3, m5
    punpckhbw       m0, m3, m5
    vperm2i128      m6, m1, m0, 0x20
    vperm2i128      m0, m1, m0, 0x31
    pmaddubsw       m1, m6, m9
    paddw           m2, m1
    pmaddubsw       m6, m8
    pmaddubsw       m1, m0, m9
    paddw           m4, m1
    pmaddubsw       m0, m8

%ifidn %1,pp
    pmulhrsw        m4, m7                         ; m4 = word: row 10
    pmulhrsw        m2, m7                         ; m2 = word: row 11
    packuswb        m4, m2
    vpermq          m4, m4, 11011000b
    vextracti128    xm2, m4, 1
    movu            [r8 + r3 * 2], xm4
    movu            [r8 + r6], xm2
%else
    psubw           m4, m7                         ; m4 = word: row 10
    psubw           m2, m7                         ; m2 = word: row 11
    movu            [r8 + r3 * 2], m4
    movu            [r8 + r6], m2
%endif
    lea             r8, [r8 + r3 * 4]

    movu            xm3, [r7 + r4]
    lea             r7, [r7 + r1 * 4]
    vinserti128     m3, m3, [r7], 1
    vinserti128     m5, m5, xm3, 1

    punpcklbw       m2, m5, m3
    punpckhbw       m1, m5, m3
    vperm2i128      m4, m2, m1, 0x20
    vperm2i128      m2, m2, m1, 0x31
    pmaddubsw       m1, m4, m9
    paddw           m0, m1
    pmaddubsw       m4, m8
    pmaddubsw       m1, m2, m9
    paddw           m6, m1
    pmaddubsw       m2, m8

%ifidn %1,pp
    pmulhrsw        m0, m7                         ; m0 = word: row 12
    pmulhrsw        m6, m7                         ; m6 = word: row 13
    packuswb        m0, m6
    vpermq          m0, m0, 11011000b
    vextracti128    xm6, m0, 1
    movu            [r8], xm0
    movu            [r8 + r3], xm6
%else
    psubw           m0, m7                         ; m0 = word: row 12
    psubw           m6, m7                         ; m6 = word: row 13
    movu            [r8], m0
    movu            [r8 + r3], m6
%endif

    movu            xm5, [r7 + r1 * 2]
    vinserti128     m5, m5, [r7 + r1], 1
    vextracti128    xm0, m5, 1
    vinserti128     m3, m3, xm0, 0

    punpcklbw       m1, m3, m5
    punpckhbw       m0, m3, m5
    vperm2i128      m6, m1, m0, 0x20
    vperm2i128      m0, m1, m0, 0x31
    pmaddubsw       m6, m9
    paddw           m2, m6
    pmaddubsw       m0, m9
    paddw           m4, m0

%ifidn %1,pp
    pmulhrsw        m4, m7                         ; m4 = word: row 14
    pmulhrsw        m2, m7                         ; m2 = word: row 15
    packuswb        m4, m2
    vpermq          m4, m4, 11011000b
    vextracti128    xm2, m4, 1
    movu            [r8 + r3 * 2], xm4
    movu            [r8 + r6], xm2
    add             r2, 16
%else
    psubw           m4, m7                         ; m4 = word: row 14
    psubw           m2, m7                         ; m2 = word: row 15
    movu            [r8 + r3 * 2], m4
    movu            [r8 + r6], m2
    add             r2, 32
%endif
    add             r0, 16
    movq            xm1, [r0]                       ; m1 = row 0
    movq            xm2, [r0 + r1]                  ; m2 = row 1
    punpcklbw       xm1, xm2
    movq            xm3, [r0 + r1 * 2]              ; m3 = row 2
    punpcklbw       xm2, xm3
    vinserti128     m5, m1, xm2, 1
    pmaddubsw       m5, m8
    movq            xm4, [r0 + r4]                  ; m4 = row 3
    punpcklbw       xm3, xm4
    lea             r7, [r0 + r1 * 4]
    movq            xm1, [r7]                       ; m1 = row 4
    punpcklbw       xm4, xm1
    vinserti128     m2, m3, xm4, 1
    pmaddubsw       m0, m2, m9
    paddw           m5, m0
    pmaddubsw       m2, m8
    movq            xm3, [r7 + r1]                  ; m3 = row 5
    punpcklbw       xm1, xm3
    movq            xm4, [r7 + r1 * 2]              ; m4 = row 6
    punpcklbw       xm3, xm4
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m0, m1, m9
    paddw           m2, m0
    pmaddubsw       m1, m8
    movq            xm3, [r7 + r4]                  ; m3 = row 7
    punpcklbw       xm4, xm3
    lea             r7, [r7 + r1 * 4]
    movq            xm0, [r7]                       ; m0 = row 8
    punpcklbw       xm3, xm0
    vinserti128     m4, m4, xm3, 1
    pmaddubsw       m3, m4, m9
    paddw           m1, m3
    pmaddubsw       m4, m8
    movq            xm3, [r7 + r1]                  ; m3 = row 9
    punpcklbw       xm0, xm3
    movq            xm6, [r7 + r1 * 2]              ; m6 = row 10
    punpcklbw       xm3, xm6
    vinserti128     m0, m0, xm3, 1
    pmaddubsw       m3, m0, m9
    paddw           m4, m3
    pmaddubsw       m0, m8

%ifidn %1,pp
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
    movhps          [r2 + r3 * 2], xm5
    movhps          [r2 + r6], xm2
    lea             r8, [r2 + r3 * 4]
    movq            [r8], xm1
    movq            [r8 + r3], xm4
    movhps          [r8 + r3 * 2], xm1
    movhps          [r8 + r6], xm4
%else
    psubw           m5, m7                          ; m5 = word: row 0, row 1
    psubw           m2, m7                          ; m2 = word: row 2, row 3
    psubw           m1, m7                          ; m1 = word: row 4, row 5
    psubw           m4, m7                          ; m4 = word: row 6, row 7
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
%endif
    lea             r8, [r8 + r3 * 4]

    movq            xm3, [r7 + r4]                  ; m3 = row 11
    punpcklbw       xm6, xm3
    lea             r7, [r7 + r1 * 4]
    movq            xm5, [r7]                       ; m5 = row 12
    punpcklbw       xm3, xm5
    vinserti128     m6, m6, xm3, 1
    pmaddubsw       m3, m6, m9
    paddw           m0, m3
    pmaddubsw       m6, m8
    movq            xm3, [r7 + r1]                  ; m3 = row 13
    punpcklbw       xm5, xm3
    movq            xm2, [r7 + r1 * 2]              ; m2 = row 14
    punpcklbw       xm3, xm2
    vinserti128     m5, m5, xm3, 1
    pmaddubsw       m3, m5, m9
    paddw           m6, m3
    pmaddubsw       m5, m8
    movq            xm3, [r7 + r4]                  ; m3 = row 15
    punpcklbw       xm2, xm3
    lea             r7, [r7 + r1 * 4]
    movq            xm1, [r7]                       ; m1 = row 16
    punpcklbw       xm3, xm1
    vinserti128     m2, m2, xm3, 1
    pmaddubsw       m3, m2, m9
    paddw           m5, m3
    pmaddubsw       m2, m8
    movq            xm3, [r7 + r1]                  ; m3 = row 17
    punpcklbw       xm1, xm3
    movq            xm4, [r7 + r1 * 2]              ; m4 = row 18
    punpcklbw       xm3, xm4
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m3, m1, m9
    paddw           m2, m3
%ifidn %1,pp
    pmulhrsw        m0, m7                          ; m0 = word: row 8, row 9
    pmulhrsw        m6, m7                          ; m6 = word: row 10, row 11
    pmulhrsw        m5, m7                          ; m5 = word: row 12, row 13
    pmulhrsw        m2, m7                          ; m2 = word: row 14, row 15
    packuswb        m0, m6
    packuswb        m5, m2
    vextracti128    xm6, m0, 1
    vextracti128    xm2, m5, 1
    movq            [r8], xm0
    movq            [r8 + r3], xm6
    movhps          [r8 + r3 * 2], xm0
    movhps          [r8 + r6], xm6
    lea             r8, [r8 + r3 * 4]
    movq            [r8], xm5
    movq            [r8 + r3], xm2
    movhps          [r8 + r3 * 2], xm5
    movhps          [r8 + r6], xm2
    lea             r2, [r8 + r3 * 4 - 16]
%else
    psubw           m0, m7                          ; m0 = word: row 8, row 9
    psubw           m6, m7                          ; m6 = word: row 10, row 11
    psubw           m5, m7                          ; m5 = word: row 12, row 13
    psubw           m2, m7                          ; m2 = word: row 14, row 15
    vextracti128    xm3, m0, 1
    movu            [r8], xm0
    movu            [r8 + r3], xm3
    vextracti128    xm3, m6, 1
    movu            [r8 + r3 * 2], xm6
    movu            [r8 + r6], xm3
    vextracti128    xm3, m5, 1
    lea             r8, [r8 + r3 * 4]
    movu            [r8], xm5
    movu            [r8 + r3], xm3
    vextracti128    xm3, m2, 1
    movu            [r8 + r3 * 2], xm2
    movu            [r8 + r6], xm3
    lea             r2, [r8 + r3 * 4 - 32]
%endif
    lea             r0, [r7 - 16]
    dec             r5d
    jnz             .loopH
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_AVX2_24x32 pp
    FILTER_VER_CHROMA_AVX2_24x32 ps

%macro FILTER_VER_CHROMA_AVX2_24x64 1
%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_24x64, 4, 7, 13
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    mova            m10, [r5]
    mova            m11, [r5 + mmsize]
    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m12, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m12, [pw_2000]
%endif
    lea             r5, [r3 * 3]
    mov             r6d, 16
.loopH:
    movu            m0, [r0]                        ; m0 = row 0
    movu            m1, [r0 + r1]                   ; m1 = row 1
    punpcklbw       m2, m0, m1
    punpckhbw       m3, m0, m1
    pmaddubsw       m2, m10
    pmaddubsw       m3, m10
    movu            m0, [r0 + r1 * 2]               ; m0 = row 2
    punpcklbw       m4, m1, m0
    punpckhbw       m5, m1, m0
    pmaddubsw       m4, m10
    pmaddubsw       m5, m10
    movu            m1, [r0 + r4]                   ; m1 = row 3
    punpcklbw       m6, m0, m1
    punpckhbw       m7, m0, m1
    pmaddubsw       m8, m6, m11
    pmaddubsw       m9, m7, m11
    pmaddubsw       m6, m10
    pmaddubsw       m7, m10
    paddw           m2, m8
    paddw           m3, m9
%ifidn %1,pp
    pmulhrsw        m2, m12
    pmulhrsw        m3, m12
    packuswb        m2, m3
    movu            [r2], xm2
    vextracti128    xm2, m2, 1
    movq            [r2 + 16], xm2
%else
    psubw           m2, m12
    psubw           m3, m12
    vperm2i128      m0, m2, m3, 0x20
    vperm2i128      m2, m2, m3, 0x31
    movu            [r2], m0
    movu            [r2 + mmsize], xm2
%endif
    lea             r0, [r0 + r1 * 4]
    movu            m0, [r0]                        ; m0 = row 4
    punpcklbw       m2, m1, m0
    punpckhbw       m3, m1, m0
    pmaddubsw       m8, m2, m11
    pmaddubsw       m9, m3, m11
    pmaddubsw       m2, m10
    pmaddubsw       m3, m10
    paddw           m4, m8
    paddw           m5, m9
%ifidn %1,pp
    pmulhrsw        m4, m12
    pmulhrsw        m5, m12
    packuswb        m4, m5
    movu            [r2 + r3], xm4
    vextracti128    xm4, m4, 1
    movq            [r2 + r3 + 16], xm4
%else
    psubw           m4, m12
    psubw           m5, m12
    vperm2i128      m1, m4, m5, 0x20
    vperm2i128      m4, m4, m5, 0x31
    movu            [r2 + r3], m1
    movu            [r2 + r3 + mmsize], xm4
%endif

    movu            m1, [r0 + r1]                   ; m1 = row 5
    punpcklbw       m4, m0, m1
    punpckhbw       m5, m0, m1
    pmaddubsw       m4, m11
    pmaddubsw       m5, m11
    paddw           m6, m4
    paddw           m7, m5
%ifidn %1,pp
    pmulhrsw        m6, m12
    pmulhrsw        m7, m12
    packuswb        m6, m7
    movu            [r2 + r3 * 2], xm6
    vextracti128    xm6, m6, 1
    movq            [r2 + r3 * 2 + 16], xm6
%else
    psubw           m6, m12
    psubw           m7, m12
    vperm2i128      m0, m6, m7, 0x20
    vperm2i128      m6, m6, m7, 0x31
    movu            [r2 + r3 * 2], m0
    movu            [r2 + r3 * 2 + mmsize], xm6
%endif

    movu            m0, [r0 + r1 * 2]               ; m0 = row 6
    punpcklbw       m6, m1, m0
    punpckhbw       m7, m1, m0
    pmaddubsw       m6, m11
    pmaddubsw       m7, m11
    paddw           m2, m6
    paddw           m3, m7
%ifidn %1,pp
    pmulhrsw        m2, m12
    pmulhrsw        m3, m12
    packuswb        m2, m3
    movu            [r2 + r5], xm2
    vextracti128    xm2, m2, 1
    movq            [r2 + r5 + 16], xm2
%else
    psubw           m2, m12
    psubw           m3, m12
    vperm2i128      m0, m2, m3, 0x20
    vperm2i128      m2, m2, m3, 0x31
    movu            [r2 + r5], m0
    movu            [r2 + r5 + mmsize], xm2
%endif
    lea             r2, [r2 + r3 * 4]
    dec             r6d
    jnz             .loopH
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_AVX2_24x64 pp
    FILTER_VER_CHROMA_AVX2_24x64 ps

%macro FILTER_VER_CHROMA_AVX2_16x4 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_16x4, 4, 6, 8
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m7, [pw_512]
%else
    add             r3d, r3d
    mova            m7, [pw_2000]
%endif

    movu            xm0, [r0]
    vinserti128     m0, m0, [r0 + r1 * 2], 1
    movu            xm1, [r0 + r1]
    vinserti128     m1, m1, [r0 + r4], 1

    punpcklbw       m2, m0, m1
    punpckhbw       m3, m0, m1
    vperm2i128      m4, m2, m3, 0x20
    vperm2i128      m2, m2, m3, 0x31
    pmaddubsw       m4, [r5]
    pmaddubsw       m3, m2, [r5 + mmsize]
    paddw           m4, m3
    pmaddubsw       m2, [r5]

    vextracti128    xm0, m0, 1
    lea             r0, [r0 + r1 * 4]
    vinserti128     m0, m0, [r0], 1

    punpcklbw       m5, m1, m0
    punpckhbw       m3, m1, m0
    vperm2i128      m6, m5, m3, 0x20
    vperm2i128      m5, m5, m3, 0x31
    pmaddubsw       m6, [r5]
    pmaddubsw       m3, m5, [r5 + mmsize]
    paddw           m6, m3
    pmaddubsw       m5, [r5]
%ifidn %1,pp
    pmulhrsw        m4, m7                          ; m4 = word: row 0
    pmulhrsw        m6, m7                          ; m6 = word: row 1
    packuswb        m4, m6
    vpermq          m4, m4, 11011000b
    vextracti128    xm6, m4, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm6
%else
    psubw           m4, m7                          ; m4 = word: row 0
    psubw           m6, m7                          ; m6 = word: row 1
    movu            [r2], m4
    movu            [r2 + r3], m6
%endif
    lea             r2, [r2 + r3 * 2]

    movu            xm4, [r0 + r1 * 2]
    vinserti128     m4, m4, [r0 + r1], 1
    vextracti128    xm1, m4, 1
    vinserti128     m0, m0, xm1, 0

    punpcklbw       m6, m0, m4
    punpckhbw       m1, m0, m4
    vperm2i128      m0, m6, m1, 0x20
    vperm2i128      m6, m6, m1, 0x31
    pmaddubsw       m0, [r5 + mmsize]
    paddw           m5, m0
    pmaddubsw       m6, [r5 + mmsize]
    paddw           m2, m6

%ifidn %1,pp
    pmulhrsw        m2, m7                          ; m2 = word: row 2
    pmulhrsw        m5, m7                          ; m5 = word: row 3
    packuswb        m2, m5
    vpermq          m2, m2, 11011000b
    vextracti128    xm5, m2, 1
    movu            [r2], xm2
    movu            [r2 + r3], xm5
%else
    psubw           m2, m7                          ; m2 = word: row 2
    psubw           m5, m7                          ; m5 = word: row 3
    movu            [r2], m2
    movu            [r2 + r3], m5
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_16x4 pp
    FILTER_VER_CHROMA_AVX2_16x4 ps

%macro FILTER_VER_CHROMA_AVX2_12xN 2
INIT_YMM avx2
cglobal interp_4tap_vert_%1_12x%2, 4, 7, 8
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m7, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m7, [pw_2000]
%endif
    lea             r6, [r3 * 3]
%rep %2 / 16
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
%ifidn %1,pp
    pmulhrsw        m0, m7                          ; m0 = word: row 0
    pmulhrsw        m1, m7                          ; m1 = word: row 1
    packuswb        m0, m1
    vextracti128    xm1, m0, 1
    movq            [r2], xm0
    movd            [r2 + 8], xm1
    movhps          [r2 + r3], xm0
    pextrd          [r2 + r3 + 8], xm1, 2
%else
    psubw           m0, m7                          ; m0 = word: row 0
    psubw           m1, m7                          ; m1 = word: row 1
    movu            [r2], xm0
    vextracti128    xm0, m0, 1
    movq            [r2 + 16], xm0
    movu            [r2 + r3], xm1
    vextracti128    xm1, m1, 1
    movq            [r2 + r3 + 16], xm1
%endif

    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m6, m4, [r5 + 1 * mmsize]
    paddw           m2, m6
    pmaddubsw       m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhbw       xm0, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm0, 1
    pmaddubsw       m0, m5, [r5 + 1 * mmsize]
    paddw           m3, m0
    pmaddubsw       m5, [r5]
%ifidn %1,pp
    pmulhrsw        m2, m7                          ; m2 = word: row 2
    pmulhrsw        m3, m7                          ; m3 = word: row 3
    packuswb        m2, m3
    vextracti128    xm3, m2, 1
    movq            [r2 + r3 * 2], xm2
    movd            [r2 + r3 * 2 + 8], xm3
    movhps          [r2 + r6], xm2
    pextrd          [r2 + r6 + 8], xm3, 2
%else
    psubw           m2, m7                          ; m2 = word: row 2
    psubw           m3, m7                          ; m3 = word: row 3
    movu            [r2 + r3 * 2], xm2
    vextracti128    xm2, m2, 1
    movq            [r2 + r3 * 2 + 16], xm2
    movu            [r2 + r6], xm3
    vextracti128    xm3, m3, 1
    movq            [r2 + r6 + 16], xm3
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm0, [r0 + r4]                  ; m0 = row 7
    punpckhbw       xm3, xm6, xm0
    punpcklbw       xm6, xm0
    vinserti128     m6, m6, xm3, 1
    pmaddubsw       m3, m6, [r5 + 1 * mmsize]
    paddw           m4, m3
    pmaddubsw       m6, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm3, [r0]                       ; m3 = row 8
    punpckhbw       xm1, xm0, xm3
    punpcklbw       xm0, xm3
    vinserti128     m0, m0, xm1, 1
    pmaddubsw       m1, m0, [r5 + 1 * mmsize]
    paddw           m5, m1
    pmaddubsw       m0, [r5]
%ifidn %1,pp
    pmulhrsw        m4, m7                          ; m4 = word: row 4
    pmulhrsw        m5, m7                          ; m5 = word: row 5
    packuswb        m4, m5
    vextracti128    xm5, m4, 1
    movq            [r2], xm4
    movd            [r2 + 8], xm5
    movhps          [r2 + r3], xm4
    pextrd          [r2 + r3 + 8], xm5, 2
%else
    psubw           m4, m7                          ; m4 = word: row 4
    psubw           m5, m7                          ; m5 = word: row 5
    movu            [r2], xm4
    vextracti128    xm4, m4, 1
    movq            [r2 + 16], xm4
    movu            [r2 + r3], xm5
    vextracti128    xm5, m5, 1
    movq            [r2 + r3 + 16], xm5
%endif

    movu            xm1, [r0 + r1]                  ; m1 = row 9
    punpckhbw       xm2, xm3, xm1
    punpcklbw       xm3, xm1
    vinserti128     m3, m3, xm2, 1
    pmaddubsw       m2, m3, [r5 + 1 * mmsize]
    paddw           m6, m2
    pmaddubsw       m3, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 10
    punpckhbw       xm4, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm4, 1
    pmaddubsw       m4, m1, [r5 + 1 * mmsize]
    paddw           m0, m4
    pmaddubsw       m1, [r5]

%ifidn %1,pp
    pmulhrsw        m6, m7                          ; m6 = word: row 6
    pmulhrsw        m0, m7                          ; m0 = word: row 7
    packuswb        m6, m0
    vextracti128    xm0, m6, 1
    movq            [r2 + r3 * 2], xm6
    movd            [r2 + r3 * 2 + 8], xm0
    movhps          [r2 + r6], xm6
    pextrd          [r2 + r6 + 8], xm0, 2
%else
    psubw           m6, m7                          ; m6 = word: row 6
    psubw           m0, m7                          ; m0 = word: row 7
    movu            [r2 + r3 * 2], xm6
    vextracti128    xm6, m6, 1
    movq            [r2 + r3 * 2 + 16], xm6
    movu            [r2 + r6], xm0
    vextracti128    xm0, m0, 1
    movq            [r2 + r6 + 16], xm0
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm4, [r0 + r4]                  ; m4 = row 11
    punpckhbw       xm6, xm2, xm4
    punpcklbw       xm2, xm4
    vinserti128     m2, m2, xm6, 1
    pmaddubsw       m6, m2, [r5 + 1 * mmsize]
    paddw           m3, m6
    pmaddubsw       m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm6, [r0]                       ; m6 = row 12
    punpckhbw       xm0, xm4, xm6
    punpcklbw       xm4, xm6
    vinserti128     m4, m4, xm0, 1
    pmaddubsw       m0, m4, [r5 + 1 * mmsize]
    paddw           m1, m0
    pmaddubsw       m4, [r5]
%ifidn %1,pp
    pmulhrsw        m3, m7                          ; m3 = word: row 8
    pmulhrsw        m1, m7                          ; m1 = word: row 9
    packuswb        m3, m1
    vextracti128    xm1, m3, 1
    movq            [r2], xm3
    movd            [r2 + 8], xm1
    movhps          [r2 + r3], xm3
    pextrd          [r2 + r3 + 8], xm1, 2
%else
    psubw           m3, m7                          ; m3 = word: row 8
    psubw           m1, m7                          ; m1 = word: row 9
    movu            [r2], xm3
    vextracti128    xm3, m3, 1
    movq            [r2 + 16], xm3
    movu            [r2 + r3], xm1
    vextracti128    xm1, m1, 1
    movq            [r2 + r3 + 16], xm1
%endif

    movu            xm0, [r0 + r1]                  ; m0 = row 13
    punpckhbw       xm1, xm6, xm0
    punpcklbw       xm6, xm0
    vinserti128     m6, m6, xm1, 1
    pmaddubsw       m1, m6, [r5 + 1 * mmsize]
    paddw           m2, m1
    pmaddubsw       m6, [r5]
    movu            xm1, [r0 + r1 * 2]              ; m1 = row 14
    punpckhbw       xm5, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm5, 1
    pmaddubsw       m5, m0, [r5 + 1 * mmsize]
    paddw           m4, m5
    pmaddubsw       m0, [r5]
%ifidn %1,pp
    pmulhrsw        m2, m7                          ; m2 = word: row 10
    pmulhrsw        m4, m7                          ; m4 = word: row 11
    packuswb        m2, m4
    vextracti128    xm4, m2, 1
    movq            [r2 + r3 * 2], xm2
    movd            [r2 + r3 * 2 + 8], xm4
    movhps          [r2 + r6], xm2
    pextrd          [r2 + r6 + 8], xm4, 2
%else
    psubw           m2, m7                          ; m2 = word: row 10
    psubw           m4, m7                          ; m4 = word: row 11
    movu            [r2 + r3 * 2], xm2
    vextracti128    xm2, m2, 1
    movq            [r2 + r3 * 2 + 16], xm2
    movu            [r2 + r6], xm4
    vextracti128    xm4, m4, 1
    movq            [r2 + r6 + 16], xm4
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm5, [r0 + r4]                  ; m5 = row 15
    punpckhbw       xm2, xm1, xm5
    punpcklbw       xm1, xm5
    vinserti128     m1, m1, xm2, 1
    pmaddubsw       m2, m1, [r5 + 1 * mmsize]
    paddw           m6, m2
    pmaddubsw       m1, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm2, [r0]                       ; m2 = row 16
    punpckhbw       xm3, xm5, xm2
    punpcklbw       xm5, xm2
    vinserti128     m5, m5, xm3, 1
    pmaddubsw       m3, m5, [r5 + 1 * mmsize]
    paddw           m0, m3
    pmaddubsw       m5, [r5]
    movu            xm3, [r0 + r1]                  ; m3 = row 17
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m2, [r5 + 1 * mmsize]
    paddw           m1, m2
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpckhbw       xm2, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm2, 1
    pmaddubsw       m3, [r5 + 1 * mmsize]
    paddw           m5, m3

%ifidn %1,pp
    pmulhrsw        m6, m7                          ; m6 = word: row 12
    pmulhrsw        m0, m7                          ; m0 = word: row 13
    pmulhrsw        m1, m7                          ; m1 = word: row 14
    pmulhrsw        m5, m7                          ; m5 = word: row 15
    packuswb        m6, m0
    packuswb        m1, m5
    vextracti128    xm0, m6, 1
    vextracti128    xm5, m1, 1
    movq            [r2], xm6
    movd            [r2 + 8], xm0
    movhps          [r2 + r3], xm6
    pextrd          [r2 + r3 + 8], xm0, 2
    movq            [r2 + r3 * 2], xm1
    movd            [r2 + r3 * 2 + 8], xm5
    movhps          [r2 + r6], xm1
    pextrd          [r2 + r6 + 8], xm5, 2
%else
    psubw           m6, m7                          ; m6 = word: row 12
    psubw           m0, m7                          ; m0 = word: row 13
    psubw           m1, m7                          ; m1 = word: row 14
    psubw           m5, m7                          ; m5 = word: row 15
    movu            [r2], xm6
    vextracti128    xm6, m6, 1
    movq            [r2 + 16], xm6
    movu            [r2 + r3], xm0
    vextracti128    xm0, m0, 1
    movq            [r2 + r3 + 16], xm0
    movu            [r2 + r3 * 2], xm1
    vextracti128    xm1, m1, 1
    movq            [r2 + r3 * 2 + 16], xm1
    movu            [r2 + r6], xm5
    vextracti128    xm5, m5, 1
    movq            [r2 + r6 + 16], xm5
%endif
    lea             r2, [r2 + r3 * 4]
%endrep
    RET
%endmacro

    FILTER_VER_CHROMA_AVX2_12xN pp, 16
    FILTER_VER_CHROMA_AVX2_12xN ps, 16
    FILTER_VER_CHROMA_AVX2_12xN pp, 32
    FILTER_VER_CHROMA_AVX2_12xN ps, 32

;-----------------------------------------------------------------------------
;void interp_4tap_vert_pp_24x32(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W24 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_24x%2, 4, 6, 8

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [v4_tab_ChromaCoeff]
    movd        m0,        [r5 + r4 * 4]
%else
    movd        m0,        [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m1,        m0,       [tab_Vm]
    pshufb      m0,        [tab_Vm + 16]

    mov         r4d,       %2

.loop:
    movu        m2,        [r0]
    movu        m3,        [r0 + r1]

    punpcklbw   m4,        m2,        m3
    punpckhbw   m2,        m3

    pmaddubsw   m4,        m1
    pmaddubsw   m2,        m1

    lea         r5,        [r0 + 2 * r1]
    movu        m5,        [r5]
    movu        m7,        [r5 + r1]

    punpcklbw   m6,        m5,        m7
    pmaddubsw   m6,        m0
    paddw       m4,        m6

    punpckhbw   m6,        m5,        m7
    pmaddubsw   m6,        m0
    paddw       m2,        m6

    mova        m6,        [pw_512]

    pmulhrsw    m4,        m6
    pmulhrsw    m2,        m6

    packuswb    m4,        m2

    movu        [r2],      m4

    punpcklbw   m4,        m3,        m5
    punpckhbw   m3,        m5

    pmaddubsw   m4,        m1
    pmaddubsw   m3,        m1

    movu        m2,        [r5 + 2 * r1]

    punpcklbw   m5,        m7,        m2
    punpckhbw   m7,        m2

    pmaddubsw   m5,        m0
    pmaddubsw   m7,        m0

    paddw       m4,        m5
    paddw       m3,        m7

    pmulhrsw    m4,        m6
    pmulhrsw    m3,        m6

    packuswb    m4,        m3

    movu        [r2 + r3],      m4

    movq        m2,        [r0 + 16]
    movq        m3,        [r0 + r1 + 16]
    movq        m4,        [r5 + 16]
    movq        m5,        [r5 + r1 + 16]

    punpcklbw   m2,        m3
    punpcklbw   m4,        m5

    pmaddubsw   m2,        m1
    pmaddubsw   m4,        m0

    paddw       m2,        m4

    pmulhrsw    m2,        m6

    movq        m3,        [r0 + r1 + 16]
    movq        m4,        [r5 + 16]
    movq        m5,        [r5 + r1 + 16]
    movq        m7,        [r5 + 2 * r1 + 16]

    punpcklbw   m3,        m4
    punpcklbw   m5,        m7

    pmaddubsw   m3,        m1
    pmaddubsw   m5,        m0

    paddw       m3,        m5

    pmulhrsw    m3,        m6
    packuswb    m2,        m3

    movh        [r2 + 16], m2
    movhps      [r2 + r3 + 16], m2

    mov         r0,        r5
    lea         r2,        [r2 + 2 * r3]

    sub         r4,        2
    jnz        .loop
    RET
%endmacro

    FILTER_V4_W24 24, 32

    FILTER_V4_W24 24, 64

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_32x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W32 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_%1x%2, 4, 6, 8

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [v4_tab_ChromaCoeff]
    movd        m0,        [r5 + r4 * 4]
%else
    movd        m0,        [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m1,        m0,       [tab_Vm]
    pshufb      m0,        [tab_Vm + 16]

    mova        m7,        [pw_512]

    mov         r4d,       %2

.loop:
    movu        m2,        [r0]
    movu        m3,        [r0 + r1]

    punpcklbw   m4,        m2,        m3
    punpckhbw   m2,        m3

    pmaddubsw   m4,        m1
    pmaddubsw   m2,        m1

    lea         r5,        [r0 + 2 * r1]
    movu        m3,        [r5]
    movu        m5,        [r5 + r1]

    punpcklbw   m6,        m3,        m5
    punpckhbw   m3,        m5

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

    punpcklbw   m4,        m2,        m3
    punpckhbw   m2,        m3

    pmaddubsw   m4,        m1
    pmaddubsw   m2,        m1

    movu        m3,        [r5 + 16]
    movu        m5,        [r5 + r1 + 16]

    punpcklbw   m6,        m3,        m5
    punpckhbw   m3,        m5

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

    FILTER_V4_W32 32, 48
    FILTER_V4_W32 32, 64

%macro FILTER_VER_CHROMA_AVX2_32xN 2
%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_32x%2, 4, 7, 13
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    mova            m10, [r5]
    mova            m11, [r5 + mmsize]
    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m12, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m12, [pw_2000]
%endif
    lea             r5, [r3 * 3]
    mov             r6d, %2 / 4
.loopW:
    movu            m0, [r0]                        ; m0 = row 0
    movu            m1, [r0 + r1]                   ; m1 = row 1
    punpcklbw       m2, m0, m1
    punpckhbw       m3, m0, m1
    pmaddubsw       m2, m10
    pmaddubsw       m3, m10
    movu            m0, [r0 + r1 * 2]               ; m0 = row 2
    punpcklbw       m4, m1, m0
    punpckhbw       m5, m1, m0
    pmaddubsw       m4, m10
    pmaddubsw       m5, m10
    movu            m1, [r0 + r4]                   ; m1 = row 3
    punpcklbw       m6, m0, m1
    punpckhbw       m7, m0, m1
    pmaddubsw       m8, m6, m11
    pmaddubsw       m9, m7, m11
    pmaddubsw       m6, m10
    pmaddubsw       m7, m10
    paddw           m2, m8
    paddw           m3, m9
%ifidn %1,pp
    pmulhrsw        m2, m12
    pmulhrsw        m3, m12
    packuswb        m2, m3
    movu            [r2], m2
%else
    psubw           m2, m12
    psubw           m3, m12
    vperm2i128      m0, m2, m3, 0x20
    vperm2i128      m2, m2, m3, 0x31
    movu            [r2], m0
    movu            [r2 + mmsize], m2
%endif
    lea             r0, [r0 + r1 * 4]
    movu            m0, [r0]                        ; m0 = row 4
    punpcklbw       m2, m1, m0
    punpckhbw       m3, m1, m0
    pmaddubsw       m8, m2, m11
    pmaddubsw       m9, m3, m11
    pmaddubsw       m2, m10
    pmaddubsw       m3, m10
    paddw           m4, m8
    paddw           m5, m9
%ifidn %1,pp
    pmulhrsw        m4, m12
    pmulhrsw        m5, m12
    packuswb        m4, m5
    movu            [r2 + r3], m4
%else
    psubw           m4, m12
    psubw           m5, m12
    vperm2i128      m1, m4, m5, 0x20
    vperm2i128      m4, m4, m5, 0x31
    movu            [r2 + r3], m1
    movu            [r2 + r3 + mmsize], m4
%endif

    movu            m1, [r0 + r1]                   ; m1 = row 5
    punpcklbw       m4, m0, m1
    punpckhbw       m5, m0, m1
    pmaddubsw       m4, m11
    pmaddubsw       m5, m11
    paddw           m6, m4
    paddw           m7, m5
%ifidn %1,pp
    pmulhrsw        m6, m12
    pmulhrsw        m7, m12
    packuswb        m6, m7
    movu            [r2 + r3 * 2], m6
%else
    psubw           m6, m12
    psubw           m7, m12
    vperm2i128      m0, m6, m7, 0x20
    vperm2i128      m6, m6, m7, 0x31
    movu            [r2 + r3 * 2], m0
    movu            [r2 + r3 * 2 + mmsize], m6
%endif

    movu            m0, [r0 + r1 * 2]               ; m0 = row 6
    punpcklbw       m6, m1, m0
    punpckhbw       m7, m1, m0
    pmaddubsw       m6, m11
    pmaddubsw       m7, m11
    paddw           m2, m6
    paddw           m3, m7
%ifidn %1,pp
    pmulhrsw        m2, m12
    pmulhrsw        m3, m12
    packuswb        m2, m3
    movu            [r2 + r5], m2
%else
    psubw           m2, m12
    psubw           m3, m12
    vperm2i128      m0, m2, m3, 0x20
    vperm2i128      m2, m2, m3, 0x31
    movu            [r2 + r5], m0
    movu            [r2 + r5 + mmsize], m2
%endif
    lea             r2, [r2 + r3 * 4]
    dec             r6d
    jnz             .loopW
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_AVX2_32xN pp, 64
    FILTER_VER_CHROMA_AVX2_32xN pp, 48
    FILTER_VER_CHROMA_AVX2_32xN pp, 32
    FILTER_VER_CHROMA_AVX2_32xN pp, 24
    FILTER_VER_CHROMA_AVX2_32xN pp, 16
    FILTER_VER_CHROMA_AVX2_32xN pp, 8
    FILTER_VER_CHROMA_AVX2_32xN ps, 64
    FILTER_VER_CHROMA_AVX2_32xN ps, 48
    FILTER_VER_CHROMA_AVX2_32xN ps, 32
    FILTER_VER_CHROMA_AVX2_32xN ps, 24
    FILTER_VER_CHROMA_AVX2_32xN ps, 16
    FILTER_VER_CHROMA_AVX2_32xN ps, 8

%macro FILTER_VER_CHROMA_AVX2_48x64 1
%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_48x64, 4, 8, 13
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    mova            m10, [r5]
    mova            m11, [r5 + mmsize]
    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m12, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m12, [pw_2000]
%endif
    lea             r5, [r3 * 3]
    lea             r7, [r1 * 4]
    mov             r6d, 16
.loopH:
    movu            m0, [r0]                        ; m0 = row 0
    movu            m1, [r0 + r1]                   ; m1 = row 1
    punpcklbw       m2, m0, m1
    punpckhbw       m3, m0, m1
    pmaddubsw       m2, m10
    pmaddubsw       m3, m10
    movu            m0, [r0 + r1 * 2]               ; m0 = row 2
    punpcklbw       m4, m1, m0
    punpckhbw       m5, m1, m0
    pmaddubsw       m4, m10
    pmaddubsw       m5, m10
    movu            m1, [r0 + r4]                   ; m1 = row 3
    punpcklbw       m6, m0, m1
    punpckhbw       m7, m0, m1
    pmaddubsw       m8, m6, m11
    pmaddubsw       m9, m7, m11
    pmaddubsw       m6, m10
    pmaddubsw       m7, m10
    paddw           m2, m8
    paddw           m3, m9
%ifidn %1,pp
    pmulhrsw        m2, m12
    pmulhrsw        m3, m12
    packuswb        m2, m3
    movu            [r2], m2
%else
    psubw           m2, m12
    psubw           m3, m12
    vperm2i128      m0, m2, m3, 0x20
    vperm2i128      m2, m2, m3, 0x31
    movu            [r2], m0
    movu            [r2 + mmsize], m2
%endif
    lea             r0, [r0 + r1 * 4]
    movu            m0, [r0]                        ; m0 = row 4
    punpcklbw       m2, m1, m0
    punpckhbw       m3, m1, m0
    pmaddubsw       m8, m2, m11
    pmaddubsw       m9, m3, m11
    pmaddubsw       m2, m10
    pmaddubsw       m3, m10
    paddw           m4, m8
    paddw           m5, m9
%ifidn %1,pp
    pmulhrsw        m4, m12
    pmulhrsw        m5, m12
    packuswb        m4, m5
    movu            [r2 + r3], m4
%else
    psubw           m4, m12
    psubw           m5, m12
    vperm2i128      m1, m4, m5, 0x20
    vperm2i128      m4, m4, m5, 0x31
    movu            [r2 + r3], m1
    movu            [r2 + r3 + mmsize], m4
%endif

    movu            m1, [r0 + r1]                   ; m1 = row 5
    punpcklbw       m4, m0, m1
    punpckhbw       m5, m0, m1
    pmaddubsw       m4, m11
    pmaddubsw       m5, m11
    paddw           m6, m4
    paddw           m7, m5
%ifidn %1,pp
    pmulhrsw        m6, m12
    pmulhrsw        m7, m12
    packuswb        m6, m7
    movu            [r2 + r3 * 2], m6
%else
    psubw           m6, m12
    psubw           m7, m12
    vperm2i128      m0, m6, m7, 0x20
    vperm2i128      m6, m6, m7, 0x31
    movu            [r2 + r3 * 2], m0
    movu            [r2 + r3 * 2 + mmsize], m6
%endif

    movu            m0, [r0 + r1 * 2]               ; m0 = row 6
    punpcklbw       m6, m1, m0
    punpckhbw       m7, m1, m0
    pmaddubsw       m6, m11
    pmaddubsw       m7, m11
    paddw           m2, m6
    paddw           m3, m7
%ifidn %1,pp
    pmulhrsw        m2, m12
    pmulhrsw        m3, m12
    packuswb        m2, m3
    movu            [r2 + r5], m2
    add             r2, 32
%else
    psubw           m2, m12
    psubw           m3, m12
    vperm2i128      m0, m2, m3, 0x20
    vperm2i128      m2, m2, m3, 0x31
    movu            [r2 + r5], m0
    movu            [r2 + r5 + mmsize], m2
    add             r2, 64
%endif
    sub             r0, r7

    movu            xm0, [r0 + 32]                  ; m0 = row 0
    movu            xm1, [r0 + r1 + 32]             ; m1 = row 1
    punpckhbw       xm2, xm0, xm1
    punpcklbw       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddubsw       m0, m10
    movu            xm2, [r0 + r1 * 2 + 32]         ; m2 = row 2
    punpckhbw       xm3, xm1, xm2
    punpcklbw       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddubsw       m1, m10
    movu            xm3, [r0 + r4 + 32]             ; m3 = row 3
    punpckhbw       xm4, xm2, xm3
    punpcklbw       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddubsw       m4, m2, m11
    paddw           m0, m4
    pmaddubsw       m2, m10
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0 + 32]                  ; m4 = row 4
    punpckhbw       xm5, xm3, xm4
    punpcklbw       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddubsw       m5, m3, m11
    paddw           m1, m5
    pmaddubsw       m3, m10
    movu            xm5, [r0 + r1 + 32]             ; m5 = row 5
    punpckhbw       xm6, xm4, xm5
    punpcklbw       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddubsw       m4, m11
    paddw           m2, m4
    movu            xm6, [r0 + r1 * 2 + 32]         ; m6 = row 6
    punpckhbw       xm7, xm5, xm6
    punpcklbw       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddubsw       m5, m11
    paddw           m3, m5
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
    movu            [r2 + r5], xm3
    lea             r2, [r2 + r3 * 4 - 32]
%else
    psubw           m0, m12                         ; m0 = word: row 0
    psubw           m1, m12                         ; m1 = word: row 1
    psubw           m2, m12                         ; m2 = word: row 2
    psubw           m3, m12                         ; m3 = word: row 3
    movu            [r2], m0
    movu            [r2 + r3], m1
    movu            [r2 + r3 * 2], m2
    movu            [r2 + r5], m3
    lea             r2, [r2 + r3 * 4 - 64]
%endif
    dec             r6d
    jnz             .loopH
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_AVX2_48x64 pp
    FILTER_VER_CHROMA_AVX2_48x64 ps

%macro FILTER_VER_CHROMA_AVX2_64xN 2
%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_64x%2, 4, 8, 13
    mov             r4d, r4m
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer_32]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer_32 + r4]
%endif

    mova            m10, [r5]
    mova            m11, [r5 + mmsize]
    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    mova            m12, [pw_512]
%else
    add             r3d, r3d
    vbroadcasti128  m12, [pw_2000]
%endif
    lea             r5, [r3 * 3]
    lea             r7, [r1 * 4]
    mov             r6d, %2 / 4
.loopH:
%assign x 0
%rep 2
    movu            m0, [r0 + x]                    ; m0 = row 0
    movu            m1, [r0 + r1 + x]               ; m1 = row 1
    punpcklbw       m2, m0, m1
    punpckhbw       m3, m0, m1
    pmaddubsw       m2, m10
    pmaddubsw       m3, m10
    movu            m0, [r0 + r1 * 2 + x]           ; m0 = row 2
    punpcklbw       m4, m1, m0
    punpckhbw       m5, m1, m0
    pmaddubsw       m4, m10
    pmaddubsw       m5, m10
    movu            m1, [r0 + r4 + x]               ; m1 = row 3
    punpcklbw       m6, m0, m1
    punpckhbw       m7, m0, m1
    pmaddubsw       m8, m6, m11
    pmaddubsw       m9, m7, m11
    pmaddubsw       m6, m10
    pmaddubsw       m7, m10
    paddw           m2, m8
    paddw           m3, m9
%ifidn %1,pp
    pmulhrsw        m2, m12
    pmulhrsw        m3, m12
    packuswb        m2, m3
    movu            [r2], m2
%else
    psubw           m2, m12
    psubw           m3, m12
    vperm2i128      m0, m2, m3, 0x20
    vperm2i128      m2, m2, m3, 0x31
    movu            [r2], m0
    movu            [r2 + mmsize], m2
%endif
    lea             r0, [r0 + r1 * 4]
    movu            m0, [r0 + x]                    ; m0 = row 4
    punpcklbw       m2, m1, m0
    punpckhbw       m3, m1, m0
    pmaddubsw       m8, m2, m11
    pmaddubsw       m9, m3, m11
    pmaddubsw       m2, m10
    pmaddubsw       m3, m10
    paddw           m4, m8
    paddw           m5, m9
%ifidn %1,pp
    pmulhrsw        m4, m12
    pmulhrsw        m5, m12
    packuswb        m4, m5
    movu            [r2 + r3], m4
%else
    psubw           m4, m12
    psubw           m5, m12
    vperm2i128      m1, m4, m5, 0x20
    vperm2i128      m4, m4, m5, 0x31
    movu            [r2 + r3], m1
    movu            [r2 + r3 + mmsize], m4
%endif

    movu            m1, [r0 + r1 + x]               ; m1 = row 5
    punpcklbw       m4, m0, m1
    punpckhbw       m5, m0, m1
    pmaddubsw       m4, m11
    pmaddubsw       m5, m11
    paddw           m6, m4
    paddw           m7, m5
%ifidn %1,pp
    pmulhrsw        m6, m12
    pmulhrsw        m7, m12
    packuswb        m6, m7
    movu            [r2 + r3 * 2], m6
%else
    psubw           m6, m12
    psubw           m7, m12
    vperm2i128      m0, m6, m7, 0x20
    vperm2i128      m6, m6, m7, 0x31
    movu            [r2 + r3 * 2], m0
    movu            [r2 + r3 * 2 + mmsize], m6
%endif

    movu            m0, [r0 + r1 * 2 + x]           ; m0 = row 6
    punpcklbw       m6, m1, m0
    punpckhbw       m7, m1, m0
    pmaddubsw       m6, m11
    pmaddubsw       m7, m11
    paddw           m2, m6
    paddw           m3, m7
%ifidn %1,pp
    pmulhrsw        m2, m12
    pmulhrsw        m3, m12
    packuswb        m2, m3
    movu            [r2 + r5], m2
    add             r2, 32
%else
    psubw           m2, m12
    psubw           m3, m12
    vperm2i128      m0, m2, m3, 0x20
    vperm2i128      m2, m2, m3, 0x31
    movu            [r2 + r5], m0
    movu            [r2 + r5 + mmsize], m2
    add             r2, 64
%endif
    sub             r0, r7
%assign x x+32
%endrep
%ifidn %1,pp
    lea             r2, [r2 + r3 * 4 - 64]
%else
    lea             r2, [r2 + r3 * 4 - 128]
%endif
    add             r0, r7
    dec             r6d
    jnz             .loopH
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_AVX2_64xN pp, 64
    FILTER_VER_CHROMA_AVX2_64xN pp, 48
    FILTER_VER_CHROMA_AVX2_64xN pp, 32
    FILTER_VER_CHROMA_AVX2_64xN pp, 16
    FILTER_VER_CHROMA_AVX2_64xN ps, 64
    FILTER_VER_CHROMA_AVX2_64xN ps, 48
    FILTER_VER_CHROMA_AVX2_64xN ps, 32
    FILTER_VER_CHROMA_AVX2_64xN ps, 16

;-----------------------------------------------------------------------------
; void interp_4tap_vert_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_V4_W16n_H2 2
INIT_XMM sse4
cglobal interp_4tap_vert_pp_%1x%2, 4, 7, 8

    mov         r4d,       r4m
    sub         r0,        r1

%ifdef PIC
    lea         r5,        [v4_tab_ChromaCoeff]
    movd        m0,        [r5 + r4 * 4]
%else
    movd        m0,        [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m1,        m0,       [tab_Vm]
    pshufb      m0,        [tab_Vm + 16]

    mov         r4d,       %2/2

.loop:

    mov         r6d,       %1/16

.loopW:

    movu        m2,        [r0]
    movu        m3,        [r0 + r1]

    punpcklbw   m4,        m2,        m3
    punpckhbw   m2,        m3

    pmaddubsw   m4,        m1
    pmaddubsw   m2,        m1

    lea         r5,        [r0 + 2 * r1]
    movu        m5,        [r5]
    movu        m6,        [r5 + r1]

    punpckhbw   m7,        m5,        m6
    pmaddubsw   m7,        m0
    paddw       m2,        m7

    punpcklbw   m7,        m5,        m6
    pmaddubsw   m7,        m0
    paddw       m4,        m7

    mova        m7,        [pw_512]

    pmulhrsw    m4,        m7
    pmulhrsw    m2,        m7

    packuswb    m4,        m2

    movu        [r2],      m4

    punpcklbw   m4,        m3,        m5
    punpckhbw   m3,        m5

    pmaddubsw   m4,        m1
    pmaddubsw   m3,        m1

    movu        m5,        [r5 + 2 * r1]

    punpcklbw   m2,        m6,        m5
    punpckhbw   m6,        m5

    pmaddubsw   m2,        m0
    pmaddubsw   m6,        m0

    paddw       m4,        m2
    paddw       m3,        m6

    pmulhrsw    m4,        m7
    pmulhrsw    m3,        m7

    packuswb    m4,        m3

    movu        [r2 + r3],      m4

    add         r0,        16
    add         r2,        16
    dec         r6d
    jnz         .loopW

    lea         r0,        [r0 + r1 * 2 - %1]
    lea         r2,        [r2 + r3 * 2 - %1]

    dec         r4d
    jnz        .loop
    RET
%endmacro

    FILTER_V4_W16n_H2 64, 64
    FILTER_V4_W16n_H2 64, 32
    FILTER_V4_W16n_H2 64, 48
    FILTER_V4_W16n_H2 48, 64
    FILTER_V4_W16n_H2 64, 16

%macro PROCESS_CHROMA_SP_W4_4R 0
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r6 + 0 *16]                ;m0=[0+1]         Row1

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m1, m4                          ;m1=[1 2]
    pmaddwd    m1, [r6 + 0 *16]                ;m1=[1+2]         Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[2 3]
    pmaddwd    m2, m4, [r6 + 0 *16]            ;m2=[2+3]         Row3
    pmaddwd    m4, [r6 + 1 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3]     Row1 done

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m5, m4                          ;m5=[3 4]
    pmaddwd    m3, m5, [r6 + 0 *16]            ;m3=[3+4]         Row4
    pmaddwd    m5, [r6 + 1 * 16]
    paddd      m1, m5                          ;m1 = [1+2+3+4]   Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[4 5]
    pmaddwd    m4, [r6 + 1 * 16]
    paddd      m2, m4                          ;m2=[2+3+4+5]     Row3

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[5 6]
    pmaddwd    m5, [r6 + 1 * 16]
    paddd      m3, m5                          ;m3=[3+4+5+6]     Row4
%endmacro

;--------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_sp_%1x%2(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SP 2
INIT_XMM sse4
cglobal interp_4tap_vert_sp_%1x%2, 5, 7, 7 ,0-gprsize

    add       r1d, r1d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_ChromaCoeffV + r4]
%endif

    mova      m6, [v4_pd_526336]

    mov       dword [rsp], %2/4

.loopH:
    mov       r4d, (%1/4)
.loopW:
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

    packuswb  m0, m2

    movd      [r2], m0
    pextrd    [r2 + r3], m0, 1
    lea       r5, [r2 + 2 * r3]
    pextrd    [r5], m0, 2
    pextrd    [r5 + r3], m0, 3

    lea       r5, [4 * r1 - 2 * 4]
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

    FILTER_VER_CHROMA_SP 16, 24
    FILTER_VER_CHROMA_SP 16, 64
    FILTER_VER_CHROMA_SP 12, 32
    FILTER_VER_CHROMA_SP 4, 32
    FILTER_VER_CHROMA_SP 32, 64
    FILTER_VER_CHROMA_SP 32, 48
    FILTER_VER_CHROMA_SP 24, 64

    FILTER_VER_CHROMA_SP 64, 64
    FILTER_VER_CHROMA_SP 64, 32
    FILTER_VER_CHROMA_SP 64, 48
    FILTER_VER_CHROMA_SP 48, 64
    FILTER_VER_CHROMA_SP 64, 16


%macro PROCESS_CHROMA_SP_W2_4R 1
    movd       m0, [r0]
    movd       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]

    lea        r0, [r0 + 2 * r1]
    movd       m2, [r0]
    punpcklwd  m1, m2                          ;m1=[1 2]
    punpcklqdq m0, m1                          ;m0=[0 1 1 2]
    pmaddwd    m0, [%1 + 0 *16]                ;m0=[0+1 1+2] Row 1-2

    movd       m1, [r0 + r1]
    punpcklwd  m2, m1                          ;m2=[2 3]

    lea        r0, [r0 + 2 * r1]
    movd       m3, [r0]
    punpcklwd  m1, m3                          ;m2=[3 4]
    punpcklqdq m2, m1                          ;m2=[2 3 3 4]

    pmaddwd    m4, m2, [%1 + 1 * 16]           ;m4=[2+3 3+4] Row 1-2
    pmaddwd    m2, [%1 + 0 * 16]               ;m2=[2+3 3+4] Row 3-4
    paddd      m0, m4                          ;m0=[0+1+2+3 1+2+3+4] Row 1-2

    movd       m1, [r0 + r1]
    punpcklwd  m3, m1                          ;m3=[4 5]

    movd       m4, [r0 + 2 * r1]
    punpcklwd  m1, m4                          ;m1=[5 6]
    punpcklqdq m3, m1                          ;m2=[4 5 5 6]
    pmaddwd    m3, [%1 + 1 * 16]               ;m3=[4+5 5+6] Row 3-4
    paddd      m2, m3                          ;m2=[2+3+4+5 3+4+5+6] Row 3-4
%endmacro

;-------------------------------------------------------------------------------------------------------------------
; void interp_4tap_vertical_sp_%1x%2(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SP_W2_4R 2
INIT_XMM sse4
cglobal interp_4tap_vert_sp_%1x%2, 5, 6, 6

    add       r1d, r1d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif

    mova      m5, [v4_pd_526336]

    mov       r4d, (%2/4)

.loopH:
    PROCESS_CHROMA_SP_W2_4R r5

    paddd     m0, m5
    paddd     m2, m5

    psrad     m0, 12
    psrad     m2, 12

    packssdw  m0, m2
    packuswb  m0, m0

    pextrw    [r2], m0, 0
    pextrw    [r2 + r3], m0, 1
    lea       r2, [r2 + 2 * r3]
    pextrw    [r2], m0, 2
    pextrw    [r2 + r3], m0, 3

    lea       r2, [r2 + 2 * r3]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

    FILTER_VER_CHROMA_SP_W2_4R 2, 4
    FILTER_VER_CHROMA_SP_W2_4R 2, 8

    FILTER_VER_CHROMA_SP_W2_4R 2, 16

;--------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_sp_4x2(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
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

    mova       m4, [v4_pd_526336]

    movq       m0, [r0]
    movq       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r5 + 0 *16]                ;m0=[0+1]  Row1

    lea        r0, [r0 + 2 * r1]
    movq       m2, [r0]
    punpcklwd  m1, m2                          ;m1=[1 2]
    pmaddwd    m1, [r5 + 0 *16]                ;m1=[1+2]  Row2

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
    pextrd     [r2 + r3], m0, 1

    RET

;-------------------------------------------------------------------------------------------------------------------
; void interp_4tap_vertical_sp_6x%2(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SP_W6_H4 2
INIT_XMM sse4
cglobal interp_4tap_vert_sp_6x%2, 5, 7, 7

    add       r1d, r1d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_ChromaCoeffV + r4]
%endif

    mova      m6, [v4_pd_526336]

    mov       r4d, %2/4

.loopH:
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

    packuswb  m0, m2

    movd      [r2], m0
    pextrd    [r2 + r3], m0, 1
    lea       r5, [r2 + 2 * r3]
    pextrd    [r5], m0, 2
    pextrd    [r5 + r3], m0, 3

    lea       r5, [4 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 4

    PROCESS_CHROMA_SP_W2_4R r6

    paddd     m0, m6
    paddd     m2, m6

    psrad     m0, 12
    psrad     m2, 12

    packssdw  m0, m2
    packuswb  m0, m0

    pextrw    [r2], m0, 0
    pextrw    [r2 + r3], m0, 1
    lea       r2, [r2 + 2 * r3]
    pextrw    [r2], m0, 2
    pextrw    [r2 + r3], m0, 3

    sub       r0, 2 * 4
    lea       r2, [r2 + 2 * r3 - 4]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

    FILTER_VER_CHROMA_SP_W6_H4 6, 8

    FILTER_VER_CHROMA_SP_W6_H4 6, 16

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
INIT_XMM sse2
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

    mova      m7, [v4_pd_526336]

    mov       r4d, %2/2
.loopH:
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

    FILTER_VER_CHROMA_SP_W8_H2 8, 12
    FILTER_VER_CHROMA_SP_W8_H2 8, 64


;---------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_%1x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------
%macro FILTER_V_PS_W16n 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_%1x%2, 4, 7, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [v4_tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m1, m0, [tab_Vm]
    pshufb     m0, [tab_Vm + 16]
    mov        r4d, %2/2

.loop:

    mov         r6d,       %1/16

.loopW:

    movu       m2, [r0]
    movu       m3, [r0 + r1]

    punpcklbw  m4, m2, m3
    punpckhbw  m2, m3

    pmaddubsw  m4, m1
    pmaddubsw  m2, m1

    lea        r5, [r0 + 2 * r1]
    movu       m5, [r5]
    movu       m7, [r5 + r1]

    punpcklbw  m6, m5, m7
    pmaddubsw  m6, m0
    paddw      m4, m6

    punpckhbw  m6, m5, m7
    pmaddubsw  m6, m0
    paddw      m2, m6

    mova       m6, [pw_2000]

    psubw      m4, m6
    psubw      m2, m6

    movu       [r2], m4
    movu       [r2 + 16], m2

    punpcklbw  m4, m3, m5
    punpckhbw  m3, m5

    pmaddubsw  m4, m1
    pmaddubsw  m3, m1

    movu       m5, [r5 + 2 * r1]

    punpcklbw  m2, m7, m5
    punpckhbw  m7, m5

    pmaddubsw  m2, m0
    pmaddubsw  m7, m0

    paddw      m4, m2
    paddw      m3, m7

    psubw      m4, m6
    psubw      m3, m6

    movu       [r2 + r3], m4
    movu       [r2 + r3 + 16], m3

    add         r0,        16
    add         r2,        32
    dec         r6d
    jnz         .loopW

    lea         r0,        [r0 + r1 * 2 - %1]
    lea         r2,        [r2 + r3 * 2 - %1 * 2]

    dec        r4d
    jnz        .loop
    RET
%endmacro

    FILTER_V_PS_W16n 64, 64
    FILTER_V_PS_W16n 64, 32
    FILTER_V_PS_W16n 64, 48
    FILTER_V_PS_W16n 48, 64
    FILTER_V_PS_W16n 64, 16


;------------------------------------------------------------------------------------------------------------
;void interp_4tap_vert_ps_2x4(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal interp_4tap_vert_ps_2x4, 4, 6, 7

    mov         r4d, r4m
    sub         r0, r1
    add         r3d, r3d

%ifdef PIC
    lea         r5, [v4_tab_ChromaCoeff]
    movd        m0, [r5 + r4 * 4]
%else
    movd        m0, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb      m0, [tab_Cm]

    lea         r5, [3 * r1]

    movd        m2, [r0]
    movd        m3, [r0 + r1]
    movd        m4, [r0 + 2 * r1]
    movd        m5, [r0 + r5]

    punpcklbw   m2, m3
    punpcklbw   m6, m4, m5
    punpcklbw   m2, m6

    pmaddubsw   m2, m0

    lea         r0, [r0 + 4 * r1]
    movd        m6, [r0]

    punpcklbw   m3, m4
    punpcklbw   m1, m5, m6
    punpcklbw   m3, m1

    pmaddubsw   m3, m0
    phaddw      m2, m3

    mova        m1, [pw_2000]

    psubw       m2, m1

    movd        [r2], m2
    pextrd      [r2 + r3], m2, 2

    movd        m2, [r0 + r1]

    punpcklbw   m4, m5
    punpcklbw   m3, m6, m2
    punpcklbw   m4, m3

    pmaddubsw   m4, m0

    movd        m3, [r0 + 2 * r1]

    punpcklbw   m5, m6
    punpcklbw   m2, m3
    punpcklbw   m5, m2

    pmaddubsw   m5, m0
    phaddw      m4, m5
    psubw       m4, m1

    lea         r2, [r2 + 2 * r3]
    movd        [r2], m4
    pextrd      [r2 + r3], m4, 2

    RET

;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ps_2x8(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
%macro FILTER_V_PS_W2 2
INIT_XMM sse4
cglobal interp_4tap_vert_ps_2x%2, 4, 6, 8

    mov        r4d, r4m
    sub        r0, r1
    add        r3d, r3d

%ifdef PIC
    lea        r5, [v4_tab_ChromaCoeff]
    movd       m0, [r5 + r4 * 4]
%else
    movd       m0, [v4_tab_ChromaCoeff + r4 * 4]
%endif

    pshufb     m0, [tab_Cm]

    mova       m1, [pw_2000]
    lea        r5, [3 * r1]
    mov        r4d, %2/4
.loop:
    movd       m2, [r0]
    movd       m3, [r0 + r1]
    movd       m4, [r0 + 2 * r1]
    movd       m5, [r0 + r5]

    punpcklbw  m2, m3
    punpcklbw  m6, m4, m5
    punpcklbw  m2, m6

    pmaddubsw  m2, m0

    lea        r0, [r0 + 4 * r1]
    movd       m6, [r0]

    punpcklbw  m3, m4
    punpcklbw  m7, m5, m6
    punpcklbw  m3, m7

    pmaddubsw  m3, m0

    phaddw     m2, m3
    psubw      m2, m1


    movd       [r2], m2
    pshufd     m2, m2, 2
    movd       [r2 + r3], m2

    movd       m2, [r0 + r1]

    punpcklbw  m4, m5
    punpcklbw  m3, m6, m2
    punpcklbw  m4, m3

    pmaddubsw  m4, m0

    movd       m3, [r0 + 2 * r1]

    punpcklbw  m5, m6
    punpcklbw  m2, m3
    punpcklbw  m5, m2

    pmaddubsw  m5, m0

    phaddw     m4, m5

    psubw      m4, m1

    lea        r2, [r2 + 2 * r3]
    movd       [r2], m4
    pshufd     m4 , m4 ,2
    movd       [r2 + r3], m4

    lea        r2, [r2 + 2 * r3]

    dec        r4d
    jnz        .loop

    RET
%endmacro

    FILTER_V_PS_W2 2, 8

    FILTER_V_PS_W2 2, 16

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_ss_%1x%2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SS 2
INIT_XMM sse2
cglobal interp_4tap_vert_ss_%1x%2, 5, 7, 6 ,0-gprsize

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

    mov       dword [rsp], %2/4

.loopH:
    mov       r4d, (%1/4)
.loopW:
    PROCESS_CHROMA_SP_W4_4R

    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3

    movlps    [r2], m0
    movhps    [r2 + r3], m0
    lea       r5, [r2 + 2 * r3]
    movlps    [r5], m2
    movhps    [r5 + r3], m2

    lea       r5, [4 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 2 * 4

    dec       r4d
    jnz       .loopW

    lea       r0, [r0 + 4 * r1 - 2 * %1]
    lea       r2, [r2 + 4 * r3 - 2 * %1]

    dec       dword [rsp]
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

    FILTER_VER_CHROMA_SS 16, 24
    FILTER_VER_CHROMA_SS 12, 32
    FILTER_VER_CHROMA_SS 4, 32
    FILTER_VER_CHROMA_SS 32, 64
    FILTER_VER_CHROMA_SS 16, 64
    FILTER_VER_CHROMA_SS 32, 48
    FILTER_VER_CHROMA_SS 24, 64

    FILTER_VER_CHROMA_SS 64, 64
    FILTER_VER_CHROMA_SS 64, 32
    FILTER_VER_CHROMA_SS 64, 48
    FILTER_VER_CHROMA_SS 48, 64
    FILTER_VER_CHROMA_SS 64, 16

%macro FILTER_VER_CHROMA_S_AVX2_4x4 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x4, 4, 6, 7
    mov             r4d, r4m
    add             r1d, r1d
    shl             r4d, 6
    sub             r0, r1

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
%ifidn %1,sp
    mova            m6, [v4_pd_526336]
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
    pmaddwd         m4, [r5 + 1 * mmsize]
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

    FILTER_VER_CHROMA_S_AVX2_4x4 sp
    FILTER_VER_CHROMA_S_AVX2_4x4 ss

%macro FILTER_VER_CHROMA_S_AVX2_4x8 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x8, 4, 6, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    sub             r0, r1

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
%ifidn %1,sp
    mova            m7, [v4_pd_526336]
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
    pmaddwd         m5, m4, [r5 + 1 * mmsize]
    paddd           m2, m5
    pmaddwd         m4, [r5]
    movq            xm3, [r0 + r4]
    punpcklwd       xm1, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm6, [r0]
    punpcklwd       xm3, xm6
    vinserti128     m1, m1, xm3, 1                  ; m1 = [8 7 7 6]
    pmaddwd         m5, m1, [r5 + 1 * mmsize]
    paddd           m4, m5
    pmaddwd         m1, [r5]
    movq            xm3, [r0 + r1]
    punpcklwd       xm6, xm3
    movq            xm5, [r0 + 2 * r1]
    punpcklwd       xm3, xm5
    vinserti128     m6, m6, xm3, 1                  ; m6 = [A 9 9 8]
    pmaddwd         m6, [r5 + 1 * mmsize]
    paddd           m1, m6
    lea             r4, [r3 * 3]

%ifidn %1,sp
    paddd           m0, m7
    paddd           m2, m7
    paddd           m4, m7
    paddd           m1, m7
    psrad           m0, 12
    psrad           m2, 12
    psrad           m4, 12
    psrad           m1, 12
%else
    psrad           m0, 6
    psrad           m2, 6
    psrad           m4, 6
    psrad           m1, 6
%endif
    packssdw        m0, m2
    packssdw        m4, m1
%ifidn %1,sp
    packuswb        m0, m4
    vextracti128    xm2, m0, 1
    movd            [r2], xm0
    movd            [r2 + r3], xm2
    pextrd          [r2 + r3 * 2], xm0, 1
    pextrd          [r2 + r4], xm2, 1
    lea             r2, [r2 + r3 * 4]
    pextrd          [r2], xm0, 2
    pextrd          [r2 + r3], xm2, 2
    pextrd          [r2 + r3 * 2], xm0, 3
    pextrd          [r2 + r4], xm2, 3
%else
    vextracti128    xm2, m0, 1
    vextracti128    xm1, m4, 1
    movq            [r2], xm0
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r4], xm2
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm4
    movq            [r2 + r3], xm1
    movhps          [r2 + r3 * 2], xm4
    movhps          [r2 + r4], xm1
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_S_AVX2_4x8 sp
    FILTER_VER_CHROMA_S_AVX2_4x8 ss

%macro PROCESS_CHROMA_AVX2_W4_16R 1
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
    pmaddwd         m5, m4, [r5 + 1 * mmsize]
    paddd           m2, m5
    pmaddwd         m4, [r5]
    movq            xm3, [r0 + r4]
    punpcklwd       xm1, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm6, [r0]
    punpcklwd       xm3, xm6
    vinserti128     m1, m1, xm3, 1                  ; m1 = [8 7 7 6]
    pmaddwd         m5, m1, [r5 + 1 * mmsize]
    paddd           m4, m5
    pmaddwd         m1, [r5]
    movq            xm3, [r0 + r1]
    punpcklwd       xm6, xm3
    movq            xm5, [r0 + 2 * r1]
    punpcklwd       xm3, xm5
    vinserti128     m6, m6, xm3, 1                  ; m6 = [10 9 9 8]
    pmaddwd         m3, m6, [r5 + 1 * mmsize]
    paddd           m1, m3
    pmaddwd         m6, [r5]

%ifidn %1,sp
    paddd           m0, m7
    paddd           m2, m7
    paddd           m4, m7
    paddd           m1, m7
    psrad           m4, 12
    psrad           m1, 12
    psrad           m0, 12
    psrad           m2, 12
%else
    psrad           m0, 6
    psrad           m2, 6
    psrad           m4, 6
    psrad           m1, 6
%endif
    packssdw        m0, m2
    packssdw        m4, m1
%ifidn %1,sp
    packuswb        m0, m4
    vextracti128    xm4, m0, 1
    movd            [r2], xm0
    movd            [r2 + r3], xm4
    pextrd          [r2 + r3 * 2], xm0, 1
    pextrd          [r2 + r6], xm4, 1
    lea             r2, [r2 + r3 * 4]
    pextrd          [r2], xm0, 2
    pextrd          [r2 + r3], xm4, 2
    pextrd          [r2 + r3 * 2], xm0, 3
    pextrd          [r2 + r6], xm4, 3
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

    movq            xm2, [r0 + r4]
    punpcklwd       xm5, xm2
    lea             r0, [r0 + 4 * r1]
    movq            xm0, [r0]
    punpcklwd       xm2, xm0
    vinserti128     m5, m5, xm2, 1                  ; m5 = [12 11 11 10]
    pmaddwd         m2, m5, [r5 + 1 * mmsize]
    paddd           m6, m2
    pmaddwd         m5, [r5]
    movq            xm2, [r0 + r1]
    punpcklwd       xm0, xm2
    movq            xm3, [r0 + 2 * r1]
    punpcklwd       xm2, xm3
    vinserti128     m0, m0, xm2, 1                  ; m0 = [14 13 13 12]
    pmaddwd         m2, m0, [r5 + 1 * mmsize]
    paddd           m5, m2
    pmaddwd         m0, [r5]
    movq            xm4, [r0 + r4]
    punpcklwd       xm3, xm4
    lea             r0, [r0 + 4 * r1]
    movq            xm1, [r0]
    punpcklwd       xm4, xm1
    vinserti128     m3, m3, xm4, 1                  ; m3 = [16 15 15 14]
    pmaddwd         m4, m3, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m3, [r5]
    movq            xm4, [r0 + r1]
    punpcklwd       xm1, xm4
    movq            xm2, [r0 + 2 * r1]
    punpcklwd       xm4, xm2
    vinserti128     m1, m1, xm4, 1                  ; m1 = [18 17 17 16]
    pmaddwd         m1, [r5 + 1 * mmsize]
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

%macro FILTER_VER_CHROMA_S_AVX2_4x16 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x16, 4, 7, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    sub             r0, r1

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
%ifidn %1,sp
    mova            m7, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
    PROCESS_CHROMA_AVX2_W4_16R %1
    RET
%endmacro

    FILTER_VER_CHROMA_S_AVX2_4x16 sp
    FILTER_VER_CHROMA_S_AVX2_4x16 ss

%macro FILTER_VER_CHROMA_S_AVX2_4x32 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x32, 4, 7, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    sub             r0, r1

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
%ifidn %1,sp
    mova            m7, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
%rep 2
    PROCESS_CHROMA_AVX2_W4_16R %1
    lea             r2, [r2 + r3 * 4]
%endrep
    RET
%endmacro

    FILTER_VER_CHROMA_S_AVX2_4x32 sp
    FILTER_VER_CHROMA_S_AVX2_4x32 ss

%macro FILTER_VER_CHROMA_S_AVX2_4x2 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x2, 4, 6, 6
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    sub             r0, r1

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
%ifidn %1,sp
    mova            m5, [v4_pd_526336]
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
    movq            xm4, [r0 + 4 * r1]
    punpcklwd       xm3, xm4
    vinserti128     m2, m2, xm3, 1                  ; m2 = [4 3 3 2]
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m0, m2
%ifidn %1,sp
    paddd           m0, m5
    psrad           m0, 12
%else
    psrad           m0, 6
%endif
    vextracti128    xm1, m0, 1
    packssdw        xm0, xm1
%ifidn %1,sp
    packuswb        xm0, xm0
    movd            [r2], xm0
    pextrd          [r2 + r3], xm0, 1
%else
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_S_AVX2_4x2 sp
    FILTER_VER_CHROMA_S_AVX2_4x2 ss

%macro FILTER_VER_CHROMA_S_AVX2_2x4 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_2x4, 4, 6, 6
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    sub             r0, r1

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
%ifidn %1,sp
    mova            m5, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    movd            xm0, [r0]
    movd            xm1, [r0 + r1]
    punpcklwd       xm0, xm1
    movd            xm2, [r0 + r1 * 2]
    punpcklwd       xm1, xm2
    punpcklqdq      xm0, xm1                        ; m0 = [2 1 1 0]
    movd            xm3, [r0 + r4]
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movd            xm4, [r0]
    punpcklwd       xm3, xm4
    punpcklqdq      xm2, xm3                        ; m2 = [4 3 3 2]
    vinserti128     m0, m0, xm2, 1                  ; m0 = [4 3 3 2 2 1 1 0]
    movd            xm1, [r0 + r1]
    punpcklwd       xm4, xm1
    movd            xm3, [r0 + r1 * 2]
    punpcklwd       xm1, xm3
    punpcklqdq      xm4, xm1                        ; m4 = [6 5 5 4]
    vinserti128     m2, m2, xm4, 1                  ; m2 = [6 5 5 4 4 3 3 2]
    pmaddwd         m0, [r5]
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m0, m2
%ifidn %1,sp
    paddd           m0, m5
    psrad           m0, 12
%else
    psrad           m0, 6
%endif
    vextracti128    xm1, m0, 1
    packssdw        xm0, xm1
    lea             r4, [r3 * 3]
%ifidn %1,sp
    packuswb        xm0, xm0
    pextrw          [r2], xm0, 0
    pextrw          [r2 + r3], xm0, 1
    pextrw          [r2 + 2 * r3], xm0, 2
    pextrw          [r2 + r4], xm0, 3
%else
    movd            [r2], xm0
    pextrd          [r2 + r3], xm0, 1
    pextrd          [r2 + 2 * r3], xm0, 2
    pextrd          [r2 + r4], xm0, 3
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_S_AVX2_2x4 sp
    FILTER_VER_CHROMA_S_AVX2_2x4 ss

%macro FILTER_VER_CHROMA_S_AVX2_8x8 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x8, 4, 6, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m7, [v4_pd_526336]
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
%ifidn %1,sp
    paddd           m0, m7
    paddd           m1, m7
    psrad           m0, 12
    psrad           m1, 12
%else
    psrad           m0, 6
    psrad           m1, 6
%endif
    packssdw        m0, m1

    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm1, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm1, 1
    pmaddwd         m1, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m1
%ifidn %1,sp
    paddd           m2, m7
    paddd           m3, m7
    psrad           m2, 12
    psrad           m3, 12
%else
    psrad           m2, 6
    psrad           m3, 6
%endif
    packssdw        m2, m3

    movu            xm1, [r0 + r4]                  ; m1 = row 7
    punpckhwd       xm3, xm6, xm1
    punpcklwd       xm6, xm1
    vinserti128     m6, m6, xm3, 1
    pmaddwd         m3, m6, [r5 + 1 * mmsize]
    pmaddwd         m6, [r5]
    paddd           m4, m3

    lea             r4, [r3 * 3]
%ifidn %1,sp
    packuswb        m0, m2
    mova            m3, [v4_interp8_hps_shuf]
    vpermd          m0, m3, m0
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r4], xm2
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    movu            [r2], xm0
    vextracti128    xm0, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2 + r3], xm0
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r4], xm3
%endif
    lea             r2, [r2 + r3 * 4]
    lea             r0, [r0 + r1 * 4]
    movu            xm0, [r0]                       ; m0 = row 8
    punpckhwd       xm2, xm1, xm0
    punpcklwd       xm1, xm0
    vinserti128     m1, m1, xm2, 1
    pmaddwd         m2, m1, [r5 + 1 * mmsize]
    pmaddwd         m1, [r5]
    paddd           m5, m2
%ifidn %1,sp
    paddd           m4, m7
    paddd           m5, m7
    psrad           m4, 12
    psrad           m5, 12
%else
    psrad           m4, 6
    psrad           m5, 6
%endif
    packssdw        m4, m5

    movu            xm2, [r0 + r1]                  ; m2 = row 9
    punpckhwd       xm5, xm0, xm2
    punpcklwd       xm0, xm2
    vinserti128     m0, m0, xm5, 1
    pmaddwd         m0, [r5 + 1 * mmsize]
    paddd           m6, m0
    movu            xm5, [r0 + r1 * 2]              ; m5 = row 10
    punpckhwd       xm0, xm2, xm5
    punpcklwd       xm2, xm5
    vinserti128     m2, m2, xm0, 1
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m1, m2

%ifidn %1,sp
    paddd           m6, m7
    paddd           m1, m7
    psrad           m6, 12
    psrad           m1, 12
%else
    psrad           m6, 6
    psrad           m1, 6
%endif
    packssdw        m6, m1
%ifidn %1,sp
    packuswb        m4, m6
    vpermd          m4, m3, m4
    vextracti128    xm6, m4, 1
    movq            [r2], xm4
    movhps          [r2 + r3], xm4
    movq            [r2 + r3 * 2], xm6
    movhps          [r2 + r4], xm6
%else
    vpermq          m4, m4, 11011000b
    vpermq          m6, m6, 11011000b
    vextracti128    xm5, m4, 1
    vextracti128    xm1, m6, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm5
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r4], xm1
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_S_AVX2_8x8 sp
    FILTER_VER_CHROMA_S_AVX2_8x8 ss

%macro PROCESS_CHROMA_S_AVX2_W8_16R 1
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
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r7 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    paddd           m3, m7
    pmaddwd         m5, [r5]
%ifidn %1,sp
    paddd           m0, m9
    paddd           m1, m9
    paddd           m2, m9
    paddd           m3, m9
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
    mova            m3, [v4_interp8_hps_shuf]
    vpermd          m0, m3, m0
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

    movu            xm7, [r7 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    paddd           m4, m8
    pmaddwd         m6, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm8, [r7]                       ; m8 = row 8
    punpckhwd       xm0, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm0, 1
    pmaddwd         m0, m7, [r5 + 1 * mmsize]
    paddd           m5, m0
    pmaddwd         m7, [r5]
    movu            xm0, [r7 + r1]                  ; m0 = row 9
    punpckhwd       xm1, xm8, xm0
    punpcklwd       xm8, xm0
    vinserti128     m8, m8, xm1, 1
    pmaddwd         m1, m8, [r5 + 1 * mmsize]
    paddd           m6, m1
    pmaddwd         m8, [r5]
    movu            xm1, [r7 + r1 * 2]              ; m1 = row 10
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m2, m0, [r5 + 1 * mmsize]
    paddd           m7, m2
    pmaddwd         m0, [r5]
%ifidn %1,sp
    paddd           m4, m9
    paddd           m5, m9
    psrad           m4, 12
    psrad           m5, 12
    paddd           m6, m9
    paddd           m7, m9
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
    lea             r8, [r2 + r3 * 4]
%ifidn %1,sp
    packuswb        m4, m6
    vpermd          m4, m3, m4
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

    movu            xm2, [r7 + r4]                  ; m2 = row 11
    punpckhwd       xm4, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm4, 1
    pmaddwd         m4, m1, [r5 + 1 * mmsize]
    paddd           m8, m4
    pmaddwd         m1, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm4, [r7]                       ; m4 = row 12
    punpckhwd       xm5, xm2, xm4
    punpcklwd       xm2, xm4
    vinserti128     m2, m2, xm5, 1
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    paddd           m0, m5
    pmaddwd         m2, [r5]
    movu            xm5, [r7 + r1]                  ; m5 = row 13
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m1, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r7 + r1 * 2]              ; m6 = row 14
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    paddd           m2, m7
    pmaddwd         m5, [r5]
%ifidn %1,sp
    paddd           m8, m9
    paddd           m0, m9
    paddd           m1, m9
    paddd           m2, m9
    psrad           m8, 12
    psrad           m0, 12
    psrad           m1, 12
    psrad           m2, 12
%else
    psrad           m8, 6
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
%endif
    packssdw        m8, m0
    packssdw        m1, m2
    lea             r8, [r8 + r3 * 4]
%ifidn %1,sp
    packuswb        m8, m1
    vpermd          m8, m3, m8
    vextracti128    xm1, m8, 1
    movq            [r8], xm8
    movhps          [r8 + r3], xm8
    movq            [r8 + r3 * 2], xm1
    movhps          [r8 + r6], xm1
%else
    vpermq          m8, m8, 11011000b
    vpermq          m1, m1, 11011000b
    vextracti128    xm0, m8, 1
    vextracti128    xm2, m1, 1
    movu            [r8], xm8
    movu            [r8 + r3], xm0
    movu            [r8 + r3 * 2], xm1
    movu            [r8 + r6], xm2
%endif
    lea             r8, [r8 + r3 * 4]

    movu            xm7, [r7 + r4]                  ; m7 = row 15
    punpckhwd       xm2, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm2, 1
    pmaddwd         m2, m6, [r5 + 1 * mmsize]
    paddd           m4, m2
    pmaddwd         m6, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm2, [r7]                       ; m2 = row 16
    punpckhwd       xm1, xm7, xm2
    punpcklwd       xm7, xm2
    vinserti128     m7, m7, xm1, 1
    pmaddwd         m1, m7, [r5 + 1 * mmsize]
    paddd           m5, m1
    pmaddwd         m7, [r5]
    movu            xm1, [r7 + r1]                  ; m1 = row 17
    punpckhwd       xm0, xm2, xm1
    punpcklwd       xm2, xm1
    vinserti128     m2, m2, xm0, 1
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m6, m2
    movu            xm0, [r7 + r1 * 2]              ; m0 = row 18
    punpckhwd       xm2, xm1, xm0
    punpcklwd       xm1, xm0
    vinserti128     m1, m1, xm2, 1
    pmaddwd         m1, [r5 + 1 * mmsize]
    paddd           m7, m1

%ifidn %1,sp
    paddd           m4, m9
    paddd           m5, m9
    paddd           m6, m9
    paddd           m7, m9
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
%ifidn %1,sp
    packuswb        m4, m6
    vpermd          m4, m3, m4
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

%macro FILTER_VER_CHROMA_S_AVX2_Nx16 2
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%1_%2x16, 4, 10, 10
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif
    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m9, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
    mov             r9d, %2 / 8
.loopW:
    PROCESS_CHROMA_S_AVX2_W8_16R %1
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

    FILTER_VER_CHROMA_S_AVX2_Nx16 sp, 16
    FILTER_VER_CHROMA_S_AVX2_Nx16 sp, 32
    FILTER_VER_CHROMA_S_AVX2_Nx16 sp, 64
    FILTER_VER_CHROMA_S_AVX2_Nx16 ss, 16
    FILTER_VER_CHROMA_S_AVX2_Nx16 ss, 32
    FILTER_VER_CHROMA_S_AVX2_Nx16 ss, 64

%macro FILTER_VER_CHROMA_S_AVX2_NxN 3
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%3_%1x%2, 4, 11, 10
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif
    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %3,sp
    mova            m9, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
    mov             r9d, %2 / 16
.loopH:
    mov             r10d, %1 / 8
.loopW:
    PROCESS_CHROMA_S_AVX2_W8_16R %3
%ifidn %3,sp
    add             r2, 8
%else
    add             r2, 16
%endif
    add             r0, 16
    dec             r10d
    jnz             .loopW
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

    FILTER_VER_CHROMA_S_AVX2_NxN 16, 32, sp
    FILTER_VER_CHROMA_S_AVX2_NxN 24, 32, sp
    FILTER_VER_CHROMA_S_AVX2_NxN 32, 32, sp
    FILTER_VER_CHROMA_S_AVX2_NxN 16, 32, ss
    FILTER_VER_CHROMA_S_AVX2_NxN 24, 32, ss
    FILTER_VER_CHROMA_S_AVX2_NxN 32, 32, ss
    FILTER_VER_CHROMA_S_AVX2_NxN 16, 64, sp
    FILTER_VER_CHROMA_S_AVX2_NxN 24, 64, sp
    FILTER_VER_CHROMA_S_AVX2_NxN 32, 64, sp
    FILTER_VER_CHROMA_S_AVX2_NxN 32, 48, sp
    FILTER_VER_CHROMA_S_AVX2_NxN 32, 48, ss
    FILTER_VER_CHROMA_S_AVX2_NxN 16, 64, ss
    FILTER_VER_CHROMA_S_AVX2_NxN 24, 64, ss
    FILTER_VER_CHROMA_S_AVX2_NxN 32, 64, ss
    FILTER_VER_CHROMA_S_AVX2_NxN 64, 64, sp
    FILTER_VER_CHROMA_S_AVX2_NxN 64, 32, sp
    FILTER_VER_CHROMA_S_AVX2_NxN 64, 48, sp
    FILTER_VER_CHROMA_S_AVX2_NxN 48, 64, sp
    FILTER_VER_CHROMA_S_AVX2_NxN 64, 64, ss
    FILTER_VER_CHROMA_S_AVX2_NxN 64, 32, ss
    FILTER_VER_CHROMA_S_AVX2_NxN 64, 48, ss
    FILTER_VER_CHROMA_S_AVX2_NxN 48, 64, ss

%macro PROCESS_CHROMA_S_AVX2_W8_4R 1
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
    pmaddwd         m4, [r5 + 1 * mmsize]
    paddd           m2, m4
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm4, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm4, 1
    pmaddwd         m5, [r5 + 1 * mmsize]
    paddd           m3, m5
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
    mova            m3, [v4_interp8_hps_shuf]
    vpermd          m0, m3, m0
    vextracti128    xm2, m0, 1
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
%endif
%endmacro

%macro FILTER_VER_CHROMA_S_AVX2_8x4 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x4, 4, 6, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m7, [v4_pd_526336]
%else
    add             r3d, r3d
%endif

    PROCESS_CHROMA_S_AVX2_W8_4R %1
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

    FILTER_VER_CHROMA_S_AVX2_8x4 sp
    FILTER_VER_CHROMA_S_AVX2_8x4 ss

%macro FILTER_VER_CHROMA_S_AVX2_12x16 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%1_12x16, 4, 9, 10
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m9, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
    PROCESS_CHROMA_S_AVX2_W8_16R %1
%ifidn %1,sp
    add             r2, 8
%else
    add             r2, 16
%endif
    add             r0, 16
    mova            m7, m9
    PROCESS_CHROMA_AVX2_W4_16R %1
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_S_AVX2_12x16 sp
    FILTER_VER_CHROMA_S_AVX2_12x16 ss

%macro FILTER_VER_CHROMA_S_AVX2_12x32 1
%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_12x32, 4, 9, 10
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1, sp
    mova            m9, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
%rep 2
    PROCESS_CHROMA_S_AVX2_W8_16R %1
%ifidn %1, sp
    add             r2, 8
%else
    add             r2, 16
%endif
    add             r0, 16
    mova            m7, m9
    PROCESS_CHROMA_AVX2_W4_16R %1
    sub             r0, 16
%ifidn %1, sp
    lea             r2, [r2 + r3 * 4 - 8]
%else
    lea             r2, [r2 + r3 * 4 - 16]
%endif
%endrep
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_S_AVX2_12x32 sp
    FILTER_VER_CHROMA_S_AVX2_12x32 ss

%macro FILTER_VER_CHROMA_S_AVX2_16x12 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%1_16x12, 4, 9, 9
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m8, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
%rep 2
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
%ifidn %1,sp
    paddd           m0, m8
    paddd           m1, m8
    psrad           m0, 12
    psrad           m1, 12
%else
    psrad           m0, 6
    psrad           m1, 6
%endif
    packssdw        m0, m1

    movu            xm5, [r7 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r7 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm1, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm1, 1
    pmaddwd         m1, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m1
%ifidn %1,sp
    paddd           m2, m8
    paddd           m3, m8
    psrad           m2, 12
    psrad           m3, 12
%else
    psrad           m2, 6
    psrad           m3, 6
%endif
    packssdw        m2, m3
%ifidn %1,sp
    packuswb        m0, m2
    mova            m3, [v4_interp8_hps_shuf]
    vpermd          m0, m3, m0
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r6], xm2
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    movu            [r2], xm0
    vextracti128    xm0, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2 + r3], xm0
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
%endif
    lea             r8, [r2 + r3 * 4]

    movu            xm1, [r7 + r4]                  ; m1 = row 7
    punpckhwd       xm0, xm6, xm1
    punpcklwd       xm6, xm1
    vinserti128     m6, m6, xm0, 1
    pmaddwd         m0, m6, [r5 + 1 * mmsize]
    pmaddwd         m6, [r5]
    paddd           m4, m0
    lea             r7, [r7 + r1 * 4]
    movu            xm0, [r7]                       ; m0 = row 8
    punpckhwd       xm2, xm1, xm0
    punpcklwd       xm1, xm0
    vinserti128     m1, m1, xm2, 1
    pmaddwd         m2, m1, [r5 + 1 * mmsize]
    pmaddwd         m1, [r5]
    paddd           m5, m2
%ifidn %1,sp
    paddd           m4, m8
    paddd           m5, m8
    psrad           m4, 12
    psrad           m5, 12
%else
    psrad           m4, 6
    psrad           m5, 6
%endif
    packssdw        m4, m5

    movu            xm2, [r7 + r1]                  ; m2 = row 9
    punpckhwd       xm5, xm0, xm2
    punpcklwd       xm0, xm2
    vinserti128     m0, m0, xm5, 1
    pmaddwd         m5, m0, [r5 + 1 * mmsize]
    paddd           m6, m5
    pmaddwd         m0, [r5]
    movu            xm5, [r7 + r1 * 2]              ; m5 = row 10
    punpckhwd       xm7, xm2, xm5
    punpcklwd       xm2, xm5
    vinserti128     m2, m2, xm7, 1
    pmaddwd         m7, m2, [r5 + 1 * mmsize]
    paddd           m1, m7
    pmaddwd         m2, [r5]

%ifidn %1,sp
    paddd           m6, m8
    paddd           m1, m8
    psrad           m6, 12
    psrad           m1, 12
%else
    psrad           m6, 6
    psrad           m1, 6
%endif
    packssdw        m6, m1
%ifidn %1,sp
    packuswb        m4, m6
    vpermd          m4, m3, m4
    vextracti128    xm6, m4, 1
    movq            [r8], xm4
    movhps          [r8 + r3], xm4
    movq            [r8 + r3 * 2], xm6
    movhps          [r8 + r6], xm6
%else
    vpermq          m4, m4, 11011000b
    vpermq          m6, m6, 11011000b
    vextracti128    xm7, m4, 1
    vextracti128    xm1, m6, 1
    movu            [r8], xm4
    movu            [r8 + r3], xm7
    movu            [r8 + r3 * 2], xm6
    movu            [r8 + r6], xm1
%endif
    lea             r8, [r8 + r3 * 4]

    movu            xm7, [r7 + r4]                  ; m7 = row 11
    punpckhwd       xm1, xm5, xm7
    punpcklwd       xm5, xm7
    vinserti128     m5, m5, xm1, 1
    pmaddwd         m1, m5, [r5 + 1 * mmsize]
    paddd           m0, m1
    pmaddwd         m5, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm1, [r7]                       ; m1 = row 12
    punpckhwd       xm4, xm7, xm1
    punpcklwd       xm7, xm1
    vinserti128     m7, m7, xm4, 1
    pmaddwd         m4, m7, [r5 + 1 * mmsize]
    paddd           m2, m4
    pmaddwd         m7, [r5]
%ifidn %1,sp
    paddd           m0, m8
    paddd           m2, m8
    psrad           m0, 12
    psrad           m2, 12
%else
    psrad           m0, 6
    psrad           m2, 6
%endif
    packssdw        m0, m2

    movu            xm4, [r7 + r1]                  ; m4 = row 13
    punpckhwd       xm2, xm1, xm4
    punpcklwd       xm1, xm4
    vinserti128     m1, m1, xm2, 1
    pmaddwd         m1, [r5 + 1 * mmsize]
    paddd           m5, m1
    movu            xm2, [r7 + r1 * 2]              ; m2 = row 14
    punpckhwd       xm6, xm4, xm2
    punpcklwd       xm4, xm2
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m4, [r5 + 1 * mmsize]
    paddd           m7, m4
%ifidn %1,sp
    paddd           m5, m8
    paddd           m7, m8
    psrad           m5, 12
    psrad           m7, 12
%else
    psrad           m5, 6
    psrad           m7, 6
%endif
    packssdw        m5, m7
%ifidn %1,sp
    packuswb        m0, m5
    vpermd          m0, m3, m0
    vextracti128    xm5, m0, 1
    movq            [r8], xm0
    movhps          [r8 + r3], xm0
    movq            [r8 + r3 * 2], xm5
    movhps          [r8 + r6], xm5
    add             r2, 8
%else
    vpermq          m0, m0, 11011000b
    vpermq          m5, m5, 11011000b
    vextracti128    xm7, m0, 1
    vextracti128    xm6, m5, 1
    movu            [r8], xm0
    movu            [r8 + r3], xm7
    movu            [r8 + r3 * 2], xm5
    movu            [r8 + r6], xm6
    add             r2, 16
%endif
    add             r0, 16
%endrep
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_S_AVX2_16x12 sp
    FILTER_VER_CHROMA_S_AVX2_16x12 ss

%macro FILTER_VER_CHROMA_S_AVX2_8x12 1
%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x12, 4, 7, 9
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m8, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
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
%ifidn %1,sp
    paddd           m0, m8
    paddd           m1, m8
    psrad           m0, 12
    psrad           m1, 12
%else
    psrad           m0, 6
    psrad           m1, 6
%endif
    packssdw        m0, m1

    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm1, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm1, 1
    pmaddwd         m1, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m1
%ifidn %1,sp
    paddd           m2, m8
    paddd           m3, m8
    psrad           m2, 12
    psrad           m3, 12
%else
    psrad           m2, 6
    psrad           m3, 6
%endif
    packssdw        m2, m3
%ifidn %1,sp
    packuswb        m0, m2
    mova            m3, [v4_interp8_hps_shuf]
    vpermd          m0, m3, m0
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r6], xm2
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    movu            [r2], xm0
    vextracti128    xm0, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2 + r3], xm0
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm1, [r0 + r4]                  ; m1 = row 7
    punpckhwd       xm0, xm6, xm1
    punpcklwd       xm6, xm1
    vinserti128     m6, m6, xm0, 1
    pmaddwd         m0, m6, [r5 + 1 * mmsize]
    pmaddwd         m6, [r5]
    paddd           m4, m0
    lea             r0, [r0 + r1 * 4]
    movu            xm0, [r0]                       ; m0 = row 8
    punpckhwd       xm2, xm1, xm0
    punpcklwd       xm1, xm0
    vinserti128     m1, m1, xm2, 1
    pmaddwd         m2, m1, [r5 + 1 * mmsize]
    pmaddwd         m1, [r5]
    paddd           m5, m2
%ifidn %1,sp
    paddd           m4, m8
    paddd           m5, m8
    psrad           m4, 12
    psrad           m5, 12
%else
    psrad           m4, 6
    psrad           m5, 6
%endif
    packssdw        m4, m5

    movu            xm2, [r0 + r1]                  ; m2 = row 9
    punpckhwd       xm5, xm0, xm2
    punpcklwd       xm0, xm2
    vinserti128     m0, m0, xm5, 1
    pmaddwd         m5, m0, [r5 + 1 * mmsize]
    paddd           m6, m5
    pmaddwd         m0, [r5]
    movu            xm5, [r0 + r1 * 2]              ; m5 = row 10
    punpckhwd       xm7, xm2, xm5
    punpcklwd       xm2, xm5
    vinserti128     m2, m2, xm7, 1
    pmaddwd         m7, m2, [r5 + 1 * mmsize]
    paddd           m1, m7
    pmaddwd         m2, [r5]

%ifidn %1,sp
    paddd           m6, m8
    paddd           m1, m8
    psrad           m6, 12
    psrad           m1, 12
%else
    psrad           m6, 6
    psrad           m1, 6
%endif
    packssdw        m6, m1
%ifidn %1,sp
    packuswb        m4, m6
    vpermd          m4, m3, m4
    vextracti128    xm6, m4, 1
    movq            [r2], xm4
    movhps          [r2 + r3], xm4
    movq            [r2 + r3 * 2], xm6
    movhps          [r2 + r6], xm6
%else
    vpermq          m4, m4, 11011000b
    vpermq          m6, m6, 11011000b
    vextracti128    xm7, m4, 1
    vextracti128    xm1, m6, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm7
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r6], xm1
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm7, [r0 + r4]                  ; m7 = row 11
    punpckhwd       xm1, xm5, xm7
    punpcklwd       xm5, xm7
    vinserti128     m5, m5, xm1, 1
    pmaddwd         m1, m5, [r5 + 1 * mmsize]
    paddd           m0, m1
    pmaddwd         m5, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm1, [r0]                       ; m1 = row 12
    punpckhwd       xm4, xm7, xm1
    punpcklwd       xm7, xm1
    vinserti128     m7, m7, xm4, 1
    pmaddwd         m4, m7, [r5 + 1 * mmsize]
    paddd           m2, m4
    pmaddwd         m7, [r5]
%ifidn %1,sp
    paddd           m0, m8
    paddd           m2, m8
    psrad           m0, 12
    psrad           m2, 12
%else
    psrad           m0, 6
    psrad           m2, 6
%endif
    packssdw        m0, m2

    movu            xm4, [r0 + r1]                  ; m4 = row 13
    punpckhwd       xm2, xm1, xm4
    punpcklwd       xm1, xm4
    vinserti128     m1, m1, xm2, 1
    pmaddwd         m1, [r5 + 1 * mmsize]
    paddd           m5, m1
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 14
    punpckhwd       xm6, xm4, xm2
    punpcklwd       xm4, xm2
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m4, [r5 + 1 * mmsize]
    paddd           m7, m4
%ifidn %1,sp
    paddd           m5, m8
    paddd           m7, m8
    psrad           m5, 12
    psrad           m7, 12
%else
    psrad           m5, 6
    psrad           m7, 6
%endif
    packssdw        m5, m7
%ifidn %1,sp
    packuswb        m0, m5
    vpermd          m0, m3, m0
    vextracti128    xm5, m0, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm5
    movhps          [r2 + r6], xm5
%else
    vpermq          m0, m0, 11011000b
    vpermq          m5, m5, 11011000b
    vextracti128    xm7, m0, 1
    vextracti128    xm6, m5, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm7
    movu            [r2 + r3 * 2], xm5
    movu            [r2 + r6], xm6
%endif
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_S_AVX2_8x12 sp
    FILTER_VER_CHROMA_S_AVX2_8x12 ss

%macro FILTER_VER_CHROMA_S_AVX2_16x4 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_16x4, 4, 7, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m7, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
%rep 2
    PROCESS_CHROMA_S_AVX2_W8_4R %1
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
    lea             r6, [4 * r1 - 16]
    sub             r0, r6
%endrep
    RET
%endmacro

    FILTER_VER_CHROMA_S_AVX2_16x4 sp
    FILTER_VER_CHROMA_S_AVX2_16x4 ss

%macro PROCESS_CHROMA_S_AVX2_W8_8R 1
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
%ifidn %1,sp
    paddd           m0, m7
    paddd           m1, m7
    psrad           m0, 12
    psrad           m1, 12
%else
    psrad           m0, 6
    psrad           m1, 6
%endif
    packssdw        m0, m1

    movu            xm5, [r7 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r7 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm1, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm1, 1
    pmaddwd         m1, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m1
%ifidn %1,sp
    paddd           m2, m7
    paddd           m3, m7
    psrad           m2, 12
    psrad           m3, 12
%else
    psrad           m2, 6
    psrad           m3, 6
%endif
    packssdw        m2, m3
%ifidn %1,sp
    packuswb        m0, m2
    mova            m3, [v4_interp8_hps_shuf]
    vpermd          m0, m3, m0
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r6], xm2
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    movu            [r2], xm0
    vextracti128    xm0, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2 + r3], xm0
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
%endif
    lea             r8, [r2 + r3 * 4]

    movu            xm1, [r7 + r4]                  ; m1 = row 7
    punpckhwd       xm0, xm6, xm1
    punpcklwd       xm6, xm1
    vinserti128     m6, m6, xm0, 1
    pmaddwd         m0, m6, [r5 + 1 * mmsize]
    pmaddwd         m6, [r5]
    paddd           m4, m0
    lea             r7, [r7 + r1 * 4]
    movu            xm0, [r7]                       ; m0 = row 8
    punpckhwd       xm2, xm1, xm0
    punpcklwd       xm1, xm0
    vinserti128     m1, m1, xm2, 1
    pmaddwd         m2, m1, [r5 + 1 * mmsize]
    pmaddwd         m1, [r5]
    paddd           m5, m2
%ifidn %1,sp
    paddd           m4, m7
    paddd           m5, m7
    psrad           m4, 12
    psrad           m5, 12
%else
    psrad           m4, 6
    psrad           m5, 6
%endif
    packssdw        m4, m5

    movu            xm2, [r7 + r1]                  ; m2 = row 9
    punpckhwd       xm5, xm0, xm2
    punpcklwd       xm0, xm2
    vinserti128     m0, m0, xm5, 1
    pmaddwd         m0, [r5 + 1 * mmsize]
    paddd           m6, m0
    movu            xm5, [r7 + r1 * 2]              ; m5 = row 10
    punpckhwd       xm0, xm2, xm5
    punpcklwd       xm2, xm5
    vinserti128     m2, m2, xm0, 1
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m1, m2

%ifidn %1,sp
    paddd           m6, m7
    paddd           m1, m7
    psrad           m6, 12
    psrad           m1, 12
%else
    psrad           m6, 6
    psrad           m1, 6
%endif
    packssdw        m6, m1
%ifidn %1,sp
    packuswb        m4, m6
    vpermd          m4, m3, m4
    vextracti128    xm6, m4, 1
    movq            [r8], xm4
    movhps          [r8 + r3], xm4
    movq            [r8 + r3 * 2], xm6
    movhps          [r8 + r6], xm6
%else
    vpermq          m4, m4, 11011000b
    vpermq          m6, m6, 11011000b
    vextracti128    xm7, m4, 1
    vextracti128    xm1, m6, 1
    movu            [r8], xm4
    movu            [r8 + r3], xm7
    movu            [r8 + r3 * 2], xm6
    movu            [r8 + r6], xm1
%endif
%endmacro

%macro FILTER_VER_CHROMA_S_AVX2_Nx8 2
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%1_%2x8, 4, 9, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m7, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
%rep %2 / 8
    PROCESS_CHROMA_S_AVX2_W8_8R %1
%ifidn %1,sp
    add             r2, 8
%else
    add             r2, 16
%endif
    add             r0, 16
%endrep
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_S_AVX2_Nx8 sp, 32
    FILTER_VER_CHROMA_S_AVX2_Nx8 sp, 16
    FILTER_VER_CHROMA_S_AVX2_Nx8 ss, 32
    FILTER_VER_CHROMA_S_AVX2_Nx8 ss, 16

%macro FILTER_VER_CHROMA_S_AVX2_8x2 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x2, 4, 6, 6
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m5, [v4_pd_526336]
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
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m0, m2
    movu            xm4, [r0 + r1 * 4]              ; m4 = row 4
    punpckhwd       xm2, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm2, 1
    pmaddwd         m3, [r5 + 1 * mmsize]
    paddd           m1, m3
%ifidn %1,sp
    paddd           m0, m5
    paddd           m1, m5
    psrad           m0, 12
    psrad           m1, 12
%else
    psrad           m0, 6
    psrad           m1, 6
%endif
    packssdw        m0, m1
%ifidn %1,sp
    vextracti128    xm1, m0, 1
    packuswb        xm0, xm1
    pshufd          xm0, xm0, 11011000b
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
%else
    vpermq          m0, m0, 11011000b
    vextracti128    xm1, m0, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_S_AVX2_8x2 sp
    FILTER_VER_CHROMA_S_AVX2_8x2 ss

%macro FILTER_VER_CHROMA_S_AVX2_8x6 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x6, 4, 6, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m7, [v4_pd_526336]
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
%ifidn %1,sp
    paddd           m0, m7
    paddd           m1, m7
    psrad           m0, 12
    psrad           m1, 12
%else
    psrad           m0, 6
    psrad           m1, 6
%endif
    packssdw        m0, m1

    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm1, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm1, 1
    pmaddwd         m1, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m1
%ifidn %1,sp
    paddd           m2, m7
    paddd           m3, m7
    psrad           m2, 12
    psrad           m3, 12
%else
    psrad           m2, 6
    psrad           m3, 6
%endif
    packssdw        m2, m3

    movu            xm1, [r0 + r4]                  ; m1 = row 7
    punpckhwd       xm3, xm6, xm1
    punpcklwd       xm6, xm1
    vinserti128     m6, m6, xm3, 1
    pmaddwd         m6, [r5 + 1 * mmsize]
    paddd           m4, m6
    movu            xm6, [r0 + r1 * 4]              ; m6 = row 8
    punpckhwd       xm3, xm1, xm6
    punpcklwd       xm1, xm6
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5 + 1 * mmsize]
    paddd           m5, m1
%ifidn %1,sp
    paddd           m4, m7
    paddd           m5, m7
    psrad           m4, 12
    psrad           m5, 12
%else
    psrad           m4, 6
    psrad           m5, 6
%endif
    packssdw        m4, m5
    lea             r4, [r3 * 3]
%ifidn %1,sp
    packuswb        m0, m2
    mova            m3, [v4_interp8_hps_shuf]
    vpermd          m0, m3, m0
    vextracti128    xm2, m0, 1
    vextracti128    xm5, m4, 1
    packuswb        xm4, xm5
    pshufd          xm4, xm4, 11011000b
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r4], xm2
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm4
    movhps          [r2 + r3], xm4
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vpermq          m4, m4, 11011000b
    movu            [r2], xm0
    vextracti128    xm0, m0, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm5, m4, 1
    movu            [r2 + r3], xm0
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r4], xm3
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm4
    movu            [r2 + r3], xm5
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_S_AVX2_8x6 sp
    FILTER_VER_CHROMA_S_AVX2_8x6 ss

%macro FILTER_VER_CHROMA_S_AVX2_8xN 2
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%1_8x%2, 4, 7, 9
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m8, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
%rep %2 / 16
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
%ifidn %1,sp
    paddd           m0, m8
    paddd           m1, m8
    psrad           m0, 12
    psrad           m1, 12
%else
    psrad           m0, 6
    psrad           m1, 6
%endif
    packssdw        m0, m1

    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm1, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm1, 1
    pmaddwd         m1, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m1
%ifidn %1,sp
    paddd           m2, m8
    paddd           m3, m8
    psrad           m2, 12
    psrad           m3, 12
%else
    psrad           m2, 6
    psrad           m3, 6
%endif
    packssdw        m2, m3
%ifidn %1,sp
    packuswb        m0, m2
    mova            m3, [v4_interp8_hps_shuf]
    vpermd          m0, m3, m0
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r6], xm2
%else
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    movu            [r2], xm0
    vextracti128    xm0, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2 + r3], xm0
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm1, [r0 + r4]                  ; m1 = row 7
    punpckhwd       xm0, xm6, xm1
    punpcklwd       xm6, xm1
    vinserti128     m6, m6, xm0, 1
    pmaddwd         m0, m6, [r5 + 1 * mmsize]
    pmaddwd         m6, [r5]
    paddd           m4, m0
    lea             r0, [r0 + r1 * 4]
    movu            xm0, [r0]                       ; m0 = row 8
    punpckhwd       xm2, xm1, xm0
    punpcklwd       xm1, xm0
    vinserti128     m1, m1, xm2, 1
    pmaddwd         m2, m1, [r5 + 1 * mmsize]
    pmaddwd         m1, [r5]
    paddd           m5, m2
%ifidn %1,sp
    paddd           m4, m8
    paddd           m5, m8
    psrad           m4, 12
    psrad           m5, 12
%else
    psrad           m4, 6
    psrad           m5, 6
%endif
    packssdw        m4, m5

    movu            xm2, [r0 + r1]                  ; m2 = row 9
    punpckhwd       xm5, xm0, xm2
    punpcklwd       xm0, xm2
    vinserti128     m0, m0, xm5, 1
    pmaddwd         m5, m0, [r5 + 1 * mmsize]
    paddd           m6, m5
    pmaddwd         m0, [r5]
    movu            xm5, [r0 + r1 * 2]              ; m5 = row 10
    punpckhwd       xm7, xm2, xm5
    punpcklwd       xm2, xm5
    vinserti128     m2, m2, xm7, 1
    pmaddwd         m7, m2, [r5 + 1 * mmsize]
    paddd           m1, m7
    pmaddwd         m2, [r5]

%ifidn %1,sp
    paddd           m6, m8
    paddd           m1, m8
    psrad           m6, 12
    psrad           m1, 12
%else
    psrad           m6, 6
    psrad           m1, 6
%endif
    packssdw        m6, m1
%ifidn %1,sp
    packuswb        m4, m6
    vpermd          m4, m3, m4
    vextracti128    xm6, m4, 1
    movq            [r2], xm4
    movhps          [r2 + r3], xm4
    movq            [r2 + r3 * 2], xm6
    movhps          [r2 + r6], xm6
%else
    vpermq          m4, m4, 11011000b
    vpermq          m6, m6, 11011000b
    vextracti128    xm7, m4, 1
    vextracti128    xm1, m6, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm7
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r6], xm1
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm7, [r0 + r4]                  ; m7 = row 11
    punpckhwd       xm1, xm5, xm7
    punpcklwd       xm5, xm7
    vinserti128     m5, m5, xm1, 1
    pmaddwd         m1, m5, [r5 + 1 * mmsize]
    paddd           m0, m1
    pmaddwd         m5, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm1, [r0]                       ; m1 = row 12
    punpckhwd       xm4, xm7, xm1
    punpcklwd       xm7, xm1
    vinserti128     m7, m7, xm4, 1
    pmaddwd         m4, m7, [r5 + 1 * mmsize]
    paddd           m2, m4
    pmaddwd         m7, [r5]
%ifidn %1,sp
    paddd           m0, m8
    paddd           m2, m8
    psrad           m0, 12
    psrad           m2, 12
%else
    psrad           m0, 6
    psrad           m2, 6
%endif
    packssdw        m0, m2

    movu            xm4, [r0 + r1]                  ; m4 = row 13
    punpckhwd       xm2, xm1, xm4
    punpcklwd       xm1, xm4
    vinserti128     m1, m1, xm2, 1
    pmaddwd         m2, m1, [r5 + 1 * mmsize]
    paddd           m5, m2
    pmaddwd         m1, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 14
    punpckhwd       xm6, xm4, xm2
    punpcklwd       xm4, xm2
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m7, m6
    pmaddwd         m4, [r5]
%ifidn %1,sp
    paddd           m5, m8
    paddd           m7, m8
    psrad           m5, 12
    psrad           m7, 12
%else
    psrad           m5, 6
    psrad           m7, 6
%endif
    packssdw        m5, m7
%ifidn %1,sp
    packuswb        m0, m5
    vpermd          m0, m3, m0
    vextracti128    xm5, m0, 1
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm5
    movhps          [r2 + r6], xm5
%else
    vpermq          m0, m0, 11011000b
    vpermq          m5, m5, 11011000b
    vextracti128    xm7, m0, 1
    vextracti128    xm6, m5, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm7
    movu            [r2 + r3 * 2], xm5
    movu            [r2 + r6], xm6
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm6, [r0 + r4]                  ; m6 = row 15
    punpckhwd       xm5, xm2, xm6
    punpcklwd       xm2, xm6
    vinserti128     m2, m2, xm5, 1
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    paddd           m1, m5
    pmaddwd         m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm0, [r0]                       ; m0 = row 16
    punpckhwd       xm5, xm6, xm0
    punpcklwd       xm6, xm0
    vinserti128     m6, m6, xm5, 1
    pmaddwd         m5, m6, [r5 + 1 * mmsize]
    paddd           m4, m5
    pmaddwd         m6, [r5]
%ifidn %1,sp
    paddd           m1, m8
    paddd           m4, m8
    psrad           m1, 12
    psrad           m4, 12
%else
    psrad           m1, 6
    psrad           m4, 6
%endif
    packssdw        m1, m4

    movu            xm5, [r0 + r1]                  ; m5 = row 17
    punpckhwd       xm4, xm0, xm5
    punpcklwd       xm0, xm5
    vinserti128     m0, m0, xm4, 1
    pmaddwd         m0, [r5 + 1 * mmsize]
    paddd           m2, m0
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpckhwd       xm0, xm5, xm4
    punpcklwd       xm5, xm4
    vinserti128     m5, m5, xm0, 1
    pmaddwd         m5, [r5 + 1 * mmsize]
    paddd           m6, m5
%ifidn %1,sp
    paddd           m2, m8
    paddd           m6, m8
    psrad           m2, 12
    psrad           m6, 12
%else
    psrad           m2, 6
    psrad           m6, 6
%endif
    packssdw        m2, m6
%ifidn %1,sp
    packuswb        m1, m2
    vpermd          m1, m3, m1
    vextracti128    xm2, m1, 1
    movq            [r2], xm1
    movhps          [r2 + r3], xm1
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r6], xm2
%else
    vpermq          m1, m1, 11011000b
    vpermq          m2, m2, 11011000b
    vextracti128    xm6, m1, 1
    vextracti128    xm4, m2, 1
    movu            [r2], xm1
    movu            [r2 + r3], xm6
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm4
%endif
    lea             r2, [r2 + r3 * 4]
%endrep
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_S_AVX2_8xN sp, 16
    FILTER_VER_CHROMA_S_AVX2_8xN sp, 32
    FILTER_VER_CHROMA_S_AVX2_8xN sp, 64
    FILTER_VER_CHROMA_S_AVX2_8xN ss, 16
    FILTER_VER_CHROMA_S_AVX2_8xN ss, 32
    FILTER_VER_CHROMA_S_AVX2_8xN ss, 64

%macro FILTER_VER_CHROMA_S_AVX2_Nx24 2
%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_%2x24, 4, 10, 10
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m9, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
    mov             r9d, %2 / 8
.loopW:
    PROCESS_CHROMA_S_AVX2_W8_16R %1
%ifidn %1,sp
    add             r2, 8
%else
    add             r2, 16
%endif
    add             r0, 16
    dec             r9d
    jnz             .loopW
%ifidn %1,sp
    lea             r2, [r8 + r3 * 4 - %2 + 8]
%else
    lea             r2, [r8 + r3 * 4 - 2 * %2 + 16]
%endif
    lea             r0, [r7 - 2 * %2 + 16]
    mova            m7, m9
    mov             r9d, %2 / 8
.loop:
    PROCESS_CHROMA_S_AVX2_W8_8R %1
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

    FILTER_VER_CHROMA_S_AVX2_Nx24 sp, 32
    FILTER_VER_CHROMA_S_AVX2_Nx24 sp, 16
    FILTER_VER_CHROMA_S_AVX2_Nx24 ss, 32
    FILTER_VER_CHROMA_S_AVX2_Nx24 ss, 16

%macro FILTER_VER_CHROMA_S_AVX2_2x8 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_2x8, 4, 6, 7
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    sub             r0, r1

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
%ifidn %1,sp
    mova            m6, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    movd            xm0, [r0]
    movd            xm1, [r0 + r1]
    punpcklwd       xm0, xm1
    movd            xm2, [r0 + r1 * 2]
    punpcklwd       xm1, xm2
    punpcklqdq      xm0, xm1                        ; m0 = [2 1 1 0]
    movd            xm3, [r0 + r4]
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movd            xm4, [r0]
    punpcklwd       xm3, xm4
    punpcklqdq      xm2, xm3                        ; m2 = [4 3 3 2]
    vinserti128     m0, m0, xm2, 1                  ; m0 = [4 3 3 2 2 1 1 0]
    movd            xm1, [r0 + r1]
    punpcklwd       xm4, xm1
    movd            xm3, [r0 + r1 * 2]
    punpcklwd       xm1, xm3
    punpcklqdq      xm4, xm1                        ; m4 = [6 5 5 4]
    vinserti128     m2, m2, xm4, 1                  ; m2 = [6 5 5 4 4 3 3 2]
    pmaddwd         m0, [r5]
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m0, m2
    movd            xm1, [r0 + r4]
    punpcklwd       xm3, xm1
    lea             r0, [r0 + 4 * r1]
    movd            xm2, [r0]
    punpcklwd       xm1, xm2
    punpcklqdq      xm3, xm1                        ; m3 = [8 7 7 6]
    vinserti128     m4, m4, xm3, 1                  ; m4 = [8 7 7 6 6 5 5 4]
    movd            xm1, [r0 + r1]
    punpcklwd       xm2, xm1
    movd            xm5, [r0 + r1 * 2]
    punpcklwd       xm1, xm5
    punpcklqdq      xm2, xm1                        ; m2 = [10 9 9 8]
    vinserti128     m3, m3, xm2, 1                  ; m3 = [10 9 9 8 8 7 7 6]
    pmaddwd         m4, [r5]
    pmaddwd         m3, [r5 + 1 * mmsize]
    paddd           m4, m3
%ifidn %1,sp
    paddd           m0, m6
    paddd           m4, m6
    psrad           m0, 12
    psrad           m4, 12
%else
    psrad           m0, 6
    psrad           m4, 6
%endif
    packssdw        m0, m4
    vextracti128    xm4, m0, 1
    lea             r4, [r3 * 3]
%ifidn %1,sp
    packuswb        xm0, xm4
    pextrw          [r2], xm0, 0
    pextrw          [r2 + r3], xm0, 1
    pextrw          [r2 + 2 * r3], xm0, 4
    pextrw          [r2 + r4], xm0, 5
    lea             r2, [r2 + r3 * 4]
    pextrw          [r2], xm0, 2
    pextrw          [r2 + r3], xm0, 3
    pextrw          [r2 + 2 * r3], xm0, 6
    pextrw          [r2 + r4], xm0, 7
%else
    movd            [r2], xm0
    pextrd          [r2 + r3], xm0, 1
    movd            [r2 + 2 * r3], xm4
    pextrd          [r2 + r4], xm4, 1
    lea             r2, [r2 + r3 * 4]
    pextrd          [r2], xm0, 2
    pextrd          [r2 + r3], xm0, 3
    pextrd          [r2 + 2 * r3], xm4, 2
    pextrd          [r2 + r4], xm4, 3
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_S_AVX2_2x8 sp
    FILTER_VER_CHROMA_S_AVX2_2x8 ss

%macro FILTER_VER_CHROMA_S_AVX2_2x16 1
%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_2x16, 4, 6, 9
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    sub             r0, r1

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
%ifidn %1,sp
    mova            m6, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    movd            xm0, [r0]
    movd            xm1, [r0 + r1]
    punpcklwd       xm0, xm1
    movd            xm2, [r0 + r1 * 2]
    punpcklwd       xm1, xm2
    punpcklqdq      xm0, xm1                        ; m0 = [2 1 1 0]
    movd            xm3, [r0 + r4]
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movd            xm4, [r0]
    punpcklwd       xm3, xm4
    punpcklqdq      xm2, xm3                        ; m2 = [4 3 3 2]
    vinserti128     m0, m0, xm2, 1                  ; m0 = [4 3 3 2 2 1 1 0]
    movd            xm1, [r0 + r1]
    punpcklwd       xm4, xm1
    movd            xm3, [r0 + r1 * 2]
    punpcklwd       xm1, xm3
    punpcklqdq      xm4, xm1                        ; m4 = [6 5 5 4]
    vinserti128     m2, m2, xm4, 1                  ; m2 = [6 5 5 4 4 3 3 2]
    pmaddwd         m0, [r5]
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m0, m2
    movd            xm1, [r0 + r4]
    punpcklwd       xm3, xm1
    lea             r0, [r0 + 4 * r1]
    movd            xm2, [r0]
    punpcklwd       xm1, xm2
    punpcklqdq      xm3, xm1                        ; m3 = [8 7 7 6]
    vinserti128     m4, m4, xm3, 1                  ; m4 = [8 7 7 6 6 5 5 4]
    movd            xm1, [r0 + r1]
    punpcklwd       xm2, xm1
    movd            xm5, [r0 + r1 * 2]
    punpcklwd       xm1, xm5
    punpcklqdq      xm2, xm1                        ; m2 = [10 9 9 8]
    vinserti128     m3, m3, xm2, 1                  ; m3 = [10 9 9 8 8 7 7 6]
    pmaddwd         m4, [r5]
    pmaddwd         m3, [r5 + 1 * mmsize]
    paddd           m4, m3
    movd            xm1, [r0 + r4]
    punpcklwd       xm5, xm1
    lea             r0, [r0 + 4 * r1]
    movd            xm3, [r0]
    punpcklwd       xm1, xm3
    punpcklqdq      xm5, xm1                        ; m5 = [12 11 11 10]
    vinserti128     m2, m2, xm5, 1                  ; m2 = [12 11 11 10 10 9 9 8]
    movd            xm1, [r0 + r1]
    punpcklwd       xm3, xm1
    movd            xm7, [r0 + r1 * 2]
    punpcklwd       xm1, xm7
    punpcklqdq      xm3, xm1                        ; m3 = [14 13 13 12]
    vinserti128     m5, m5, xm3, 1                  ; m5 = [14 13 13 12 12 11 11 10]
    pmaddwd         m2, [r5]
    pmaddwd         m5, [r5 + 1 * mmsize]
    paddd           m2, m5
    movd            xm5, [r0 + r4]
    punpcklwd       xm7, xm5
    lea             r0, [r0 + 4 * r1]
    movd            xm1, [r0]
    punpcklwd       xm5, xm1
    punpcklqdq      xm7, xm5                        ; m7 = [16 15 15 14]
    vinserti128     m3, m3, xm7, 1                  ; m3 = [16 15 15 14 14 13 13 12]
    movd            xm5, [r0 + r1]
    punpcklwd       xm1, xm5
    movd            xm8, [r0 + r1 * 2]
    punpcklwd       xm5, xm8
    punpcklqdq      xm1, xm5                        ; m1 = [18 17 17 16]
    vinserti128     m7, m7, xm1, 1                  ; m7 = [18 17 17 16 16 15 15 14]
    pmaddwd         m3, [r5]
    pmaddwd         m7, [r5 + 1 * mmsize]
    paddd           m3, m7
%ifidn %1,sp
    paddd           m0, m6
    paddd           m4, m6
    paddd           m2, m6
    paddd           m3, m6
    psrad           m0, 12
    psrad           m4, 12
    psrad           m2, 12
    psrad           m3, 12
%else
    psrad           m0, 6
    psrad           m4, 6
    psrad           m2, 6
    psrad           m3, 6
%endif
    packssdw        m0, m4
    packssdw        m2, m3
    lea             r4, [r3 * 3]
%ifidn %1,sp
    packuswb        m0, m2
    vextracti128    xm2, m0, 1
    pextrw          [r2], xm0, 0
    pextrw          [r2 + r3], xm0, 1
    pextrw          [r2 + 2 * r3], xm2, 0
    pextrw          [r2 + r4], xm2, 1
    lea             r2, [r2 + r3 * 4]
    pextrw          [r2], xm0, 2
    pextrw          [r2 + r3], xm0, 3
    pextrw          [r2 + 2 * r3], xm2, 2
    pextrw          [r2 + r4], xm2, 3
    lea             r2, [r2 + r3 * 4]
    pextrw          [r2], xm0, 4
    pextrw          [r2 + r3], xm0, 5
    pextrw          [r2 + 2 * r3], xm2, 4
    pextrw          [r2 + r4], xm2, 5
    lea             r2, [r2 + r3 * 4]
    pextrw          [r2], xm0, 6
    pextrw          [r2 + r3], xm0, 7
    pextrw          [r2 + 2 * r3], xm2, 6
    pextrw          [r2 + r4], xm2, 7
%else
    vextracti128    xm4, m0, 1
    vextracti128    xm3, m2, 1
    movd            [r2], xm0
    pextrd          [r2 + r3], xm0, 1
    movd            [r2 + 2 * r3], xm4
    pextrd          [r2 + r4], xm4, 1
    lea             r2, [r2 + r3 * 4]
    pextrd          [r2], xm0, 2
    pextrd          [r2 + r3], xm0, 3
    pextrd          [r2 + 2 * r3], xm4, 2
    pextrd          [r2 + r4], xm4, 3
    lea             r2, [r2 + r3 * 4]
    movd            [r2], xm2
    pextrd          [r2 + r3], xm2, 1
    movd            [r2 + 2 * r3], xm3
    pextrd          [r2 + r4], xm3, 1
    lea             r2, [r2 + r3 * 4]
    pextrd          [r2], xm2, 2
    pextrd          [r2 + r3], xm2, 3
    pextrd          [r2 + 2 * r3], xm3, 2
    pextrd          [r2 + r4], xm3, 3
%endif
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_S_AVX2_2x16 sp
    FILTER_VER_CHROMA_S_AVX2_2x16 ss

%macro FILTER_VER_CHROMA_S_AVX2_6x8 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_6x8, 4, 6, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m7, [v4_pd_526336]
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
%ifidn %1,sp
    paddd           m0, m7
    paddd           m1, m7
    psrad           m0, 12
    psrad           m1, 12
%else
    psrad           m0, 6
    psrad           m1, 6
%endif
    packssdw        m0, m1

    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm1, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm1, 1
    pmaddwd         m1, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m1
%ifidn %1,sp
    paddd           m2, m7
    paddd           m3, m7
    psrad           m2, 12
    psrad           m3, 12
%else
    psrad           m2, 6
    psrad           m3, 6
%endif
    packssdw        m2, m3

    movu            xm1, [r0 + r4]                  ; m1 = row 7
    punpckhwd       xm3, xm6, xm1
    punpcklwd       xm6, xm1
    vinserti128     m6, m6, xm3, 1
    pmaddwd         m3, m6, [r5 + 1 * mmsize]
    pmaddwd         m6, [r5]
    paddd           m4, m3

    lea             r4, [r3 * 3]
%ifidn %1,sp
    packuswb        m0, m2
    vextracti128    xm2, m0, 1
    movd            [r2], xm0
    pextrw          [r2 + 4], xm2, 0
    pextrd          [r2 + r3], xm0, 1
    pextrw          [r2 + r3 + 4], xm2, 2
    pextrd          [r2 + r3 * 2], xm0, 2
    pextrw          [r2 + r3 * 2 + 4], xm2, 4
    pextrd          [r2 + r4], xm0, 3
    pextrw          [r2 + r4 + 4], xm2, 6
%else
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r4], xm2
    vextracti128    xm0, m0, 1
    vextracti128    xm3, m2, 1
    movd            [r2 + 8], xm0
    pextrd          [r2 + r3 + 8], xm0, 2
    movd            [r2 + r3 * 2 + 8], xm3
    pextrd          [r2 + r4 + 8], xm3, 2
%endif
    lea             r2, [r2 + r3 * 4]
    lea             r0, [r0 + r1 * 4]
    movu            xm0, [r0]                       ; m0 = row 8
    punpckhwd       xm2, xm1, xm0
    punpcklwd       xm1, xm0
    vinserti128     m1, m1, xm2, 1
    pmaddwd         m2, m1, [r5 + 1 * mmsize]
    pmaddwd         m1, [r5]
    paddd           m5, m2
%ifidn %1,sp
    paddd           m4, m7
    paddd           m5, m7
    psrad           m4, 12
    psrad           m5, 12
%else
    psrad           m4, 6
    psrad           m5, 6
%endif
    packssdw        m4, m5

    movu            xm2, [r0 + r1]                  ; m2 = row 9
    punpckhwd       xm5, xm0, xm2
    punpcklwd       xm0, xm2
    vinserti128     m0, m0, xm5, 1
    pmaddwd         m0, [r5 + 1 * mmsize]
    paddd           m6, m0
    movu            xm5, [r0 + r1 * 2]              ; m5 = row 10
    punpckhwd       xm0, xm2, xm5
    punpcklwd       xm2, xm5
    vinserti128     m2, m2, xm0, 1
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m1, m2

%ifidn %1,sp
    paddd           m6, m7
    paddd           m1, m7
    psrad           m6, 12
    psrad           m1, 12
%else
    psrad           m6, 6
    psrad           m1, 6
%endif
    packssdw        m6, m1
%ifidn %1,sp
    packuswb        m4, m6
    vextracti128    xm6, m4, 1
    movd            [r2], xm4
    pextrw          [r2 + 4], xm6, 0
    pextrd          [r2 + r3], xm4, 1
    pextrw          [r2 + r3 + 4], xm6, 2
    pextrd          [r2 + r3 * 2], xm4, 2
    pextrw          [r2 + r3 * 2 + 4], xm6, 4
    pextrd          [r2 + r4], xm4, 3
    pextrw          [r2 + r4 + 4], xm6, 6
%else
    movq            [r2], xm4
    movhps          [r2 + r3], xm4
    movq            [r2 + r3 * 2], xm6
    movhps          [r2 + r4], xm6
    vextracti128    xm5, m4, 1
    vextracti128    xm1, m6, 1
    movd            [r2 + 8], xm5
    pextrd          [r2 + r3 + 8], xm5, 2
    movd            [r2 + r3 * 2 + 8], xm1
    pextrd          [r2 + r4 + 8], xm1, 2
%endif
    RET
%endmacro

    FILTER_VER_CHROMA_S_AVX2_6x8 sp
    FILTER_VER_CHROMA_S_AVX2_6x8 ss

%macro FILTER_VER_CHROMA_S_AVX2_6x16 1
%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal interp_4tap_vert_%1_6x16, 4, 7, 9
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d

%ifdef PIC
    lea             r5, [pw_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [pw_ChromaCoeffV + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,sp
    mova            m8, [v4_pd_526336]
%else
    add             r3d, r3d
%endif
    lea             r6, [r3 * 3]
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
%ifidn %1,sp
    paddd           m0, m8
    paddd           m1, m8
    psrad           m0, 12
    psrad           m1, 12
%else
    psrad           m0, 6
    psrad           m1, 6
%endif
    packssdw        m0, m1

    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm1, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm1, 1
    pmaddwd         m1, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m1
%ifidn %1,sp
    paddd           m2, m8
    paddd           m3, m8
    psrad           m2, 12
    psrad           m3, 12
%else
    psrad           m2, 6
    psrad           m3, 6
%endif
    packssdw        m2, m3
%ifidn %1,sp
    packuswb        m0, m2
    vextracti128    xm2, m0, 1
    movd            [r2], xm0
    pextrw          [r2 + 4], xm2, 0
    pextrd          [r2 + r3], xm0, 1
    pextrw          [r2 + r3 + 4], xm2, 2
    pextrd          [r2 + r3 * 2], xm0, 2
    pextrw          [r2 + r3 * 2 + 4], xm2, 4
    pextrd          [r2 + r6], xm0, 3
    pextrw          [r2 + r6 + 4], xm2, 6
%else
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r6], xm2
    vextracti128    xm0, m0, 1
    vextracti128    xm3, m2, 1
    movd            [r2 + 8], xm0
    pextrd          [r2 + r3 + 8], xm0, 2
    movd            [r2 + r3 * 2 + 8], xm3
    pextrd          [r2 + r6 + 8], xm3, 2
%endif
    lea             r2, [r2 + r3 * 4]
    movu            xm1, [r0 + r4]                  ; m1 = row 7
    punpckhwd       xm0, xm6, xm1
    punpcklwd       xm6, xm1
    vinserti128     m6, m6, xm0, 1
    pmaddwd         m0, m6, [r5 + 1 * mmsize]
    pmaddwd         m6, [r5]
    paddd           m4, m0
    lea             r0, [r0 + r1 * 4]
    movu            xm0, [r0]                       ; m0 = row 8
    punpckhwd       xm2, xm1, xm0
    punpcklwd       xm1, xm0
    vinserti128     m1, m1, xm2, 1
    pmaddwd         m2, m1, [r5 + 1 * mmsize]
    pmaddwd         m1, [r5]
    paddd           m5, m2
%ifidn %1,sp
    paddd           m4, m8
    paddd           m5, m8
    psrad           m4, 12
    psrad           m5, 12
%else
    psrad           m4, 6
    psrad           m5, 6
%endif
    packssdw        m4, m5

    movu            xm2, [r0 + r1]                  ; m2 = row 9
    punpckhwd       xm5, xm0, xm2
    punpcklwd       xm0, xm2
    vinserti128     m0, m0, xm5, 1
    pmaddwd         m5, m0, [r5 + 1 * mmsize]
    paddd           m6, m5
    pmaddwd         m0, [r5]
    movu            xm5, [r0 + r1 * 2]              ; m5 = row 10
    punpckhwd       xm7, xm2, xm5
    punpcklwd       xm2, xm5
    vinserti128     m2, m2, xm7, 1
    pmaddwd         m7, m2, [r5 + 1 * mmsize]
    paddd           m1, m7
    pmaddwd         m2, [r5]

%ifidn %1,sp
    paddd           m6, m8
    paddd           m1, m8
    psrad           m6, 12
    psrad           m1, 12
%else
    psrad           m6, 6
    psrad           m1, 6
%endif
    packssdw        m6, m1
%ifidn %1,sp
    packuswb        m4, m6
    vextracti128    xm6, m4, 1
    movd            [r2], xm4
    pextrw          [r2 + 4], xm6, 0
    pextrd          [r2 + r3], xm4, 1
    pextrw          [r2 + r3 + 4], xm6, 2
    pextrd          [r2 + r3 * 2], xm4, 2
    pextrw          [r2 + r3 * 2 + 4], xm6, 4
    pextrd          [r2 + r6], xm4, 3
    pextrw          [r2 + r6 + 4], xm6, 6
%else
    movq            [r2], xm4
    movhps          [r2 + r3], xm4
    movq            [r2 + r3 * 2], xm6
    movhps          [r2 + r6], xm6
    vextracti128    xm4, m4, 1
    vextracti128    xm1, m6, 1
    movd            [r2 + 8], xm4
    pextrd          [r2 + r3 + 8], xm4, 2
    movd            [r2 + r3 * 2 + 8], xm1
    pextrd          [r2 + r6 + 8], xm1, 2
%endif
    lea             r2, [r2 + r3 * 4]
    movu            xm7, [r0 + r4]                  ; m7 = row 11
    punpckhwd       xm1, xm5, xm7
    punpcklwd       xm5, xm7
    vinserti128     m5, m5, xm1, 1
    pmaddwd         m1, m5, [r5 + 1 * mmsize]
    paddd           m0, m1
    pmaddwd         m5, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm1, [r0]                       ; m1 = row 12
    punpckhwd       xm4, xm7, xm1
    punpcklwd       xm7, xm1
    vinserti128     m7, m7, xm4, 1
    pmaddwd         m4, m7, [r5 + 1 * mmsize]
    paddd           m2, m4
    pmaddwd         m7, [r5]
%ifidn %1,sp
    paddd           m0, m8
    paddd           m2, m8
    psrad           m0, 12
    psrad           m2, 12
%else
    psrad           m0, 6
    psrad           m2, 6
%endif
    packssdw        m0, m2

    movu            xm4, [r0 + r1]                  ; m4 = row 13
    punpckhwd       xm2, xm1, xm4
    punpcklwd       xm1, xm4
    vinserti128     m1, m1, xm2, 1
    pmaddwd         m2, m1, [r5 + 1 * mmsize]
    paddd           m5, m2
    pmaddwd         m1, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 14
    punpckhwd       xm6, xm4, xm2
    punpcklwd       xm4, xm2
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m7, m6
    pmaddwd         m4, [r5]
%ifidn %1,sp
    paddd           m5, m8
    paddd           m7, m8
    psrad           m5, 12
    psrad           m7, 12
%else
    psrad           m5, 6
    psrad           m7, 6
%endif
    packssdw        m5, m7
%ifidn %1,sp
    packuswb        m0, m5
    vextracti128    xm5, m0, 1
    movd            [r2], xm0
    pextrw          [r2 + 4], xm5, 0
    pextrd          [r2 + r3], xm0, 1
    pextrw          [r2 + r3 + 4], xm5, 2
    pextrd          [r2 + r3 * 2], xm0, 2
    pextrw          [r2 + r3 * 2 + 4], xm5, 4
    pextrd          [r2 + r6], xm0, 3
    pextrw          [r2 + r6 + 4], xm5, 6
%else
    movq            [r2], xm0
    movhps          [r2 + r3], xm0
    movq            [r2 + r3 * 2], xm5
    movhps          [r2 + r6], xm5
    vextracti128    xm0, m0, 1
    vextracti128    xm7, m5, 1
    movd            [r2 + 8], xm0
    pextrd          [r2 + r3 + 8], xm0, 2
    movd            [r2 + r3 * 2 + 8], xm7
    pextrd          [r2 + r6 + 8], xm7, 2
%endif
    lea             r2, [r2 + r3 * 4]

    movu            xm6, [r0 + r4]                  ; m6 = row 15
    punpckhwd       xm5, xm2, xm6
    punpcklwd       xm2, xm6
    vinserti128     m2, m2, xm5, 1
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    paddd           m1, m5
    pmaddwd         m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm0, [r0]                       ; m0 = row 16
    punpckhwd       xm5, xm6, xm0
    punpcklwd       xm6, xm0
    vinserti128     m6, m6, xm5, 1
    pmaddwd         m5, m6, [r5 + 1 * mmsize]
    paddd           m4, m5
    pmaddwd         m6, [r5]
%ifidn %1,sp
    paddd           m1, m8
    paddd           m4, m8
    psrad           m1, 12
    psrad           m4, 12
%else
    psrad           m1, 6
    psrad           m4, 6
%endif
    packssdw        m1, m4

    movu            xm5, [r0 + r1]                  ; m5 = row 17
    punpckhwd       xm4, xm0, xm5
    punpcklwd       xm0, xm5
    vinserti128     m0, m0, xm4, 1
    pmaddwd         m0, [r5 + 1 * mmsize]
    paddd           m2, m0
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpckhwd       xm0, xm5, xm4
    punpcklwd       xm5, xm4
    vinserti128     m5, m5, xm0, 1
    pmaddwd         m5, [r5 + 1 * mmsize]
    paddd           m6, m5
%ifidn %1,sp
    paddd           m2, m8
    paddd           m6, m8
    psrad           m2, 12
    psrad           m6, 12
%else
    psrad           m2, 6
    psrad           m6, 6
%endif
    packssdw        m2, m6
%ifidn %1,sp
    packuswb        m1, m2
    vextracti128    xm2, m1, 1
    movd            [r2], xm1
    pextrw          [r2 + 4], xm2, 0
    pextrd          [r2 + r3], xm1, 1
    pextrw          [r2 + r3 + 4], xm2, 2
    pextrd          [r2 + r3 * 2], xm1, 2
    pextrw          [r2 + r3 * 2 + 4], xm2, 4
    pextrd          [r2 + r6], xm1, 3
    pextrw          [r2 + r6 + 4], xm2, 6
%else
    movq            [r2], xm1
    movhps          [r2 + r3], xm1
    movq            [r2 + r3 * 2], xm2
    movhps          [r2 + r6], xm2
    vextracti128    xm4, m1, 1
    vextracti128    xm6, m2, 1
    movd            [r2 + 8], xm4
    pextrd          [r2 + r3 + 8], xm4, 2
    movd            [r2 + r3 * 2 + 8], xm6
    pextrd          [r2 + r6 + 8], xm6, 2
%endif
    RET
%endif
%endmacro

    FILTER_VER_CHROMA_S_AVX2_6x16 sp
    FILTER_VER_CHROMA_S_AVX2_6x16 ss

;---------------------------------------------------------------------------------------------------------------------
; void interp_4tap_vertical_ss_%1x%2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SS_W2_4R 2
INIT_XMM sse4
cglobal interp_4tap_vert_ss_%1x%2, 5, 6, 5

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

    mov       r4d, (%2/4)

.loopH:
    PROCESS_CHROMA_SP_W2_4R r5

    psrad     m0, 6
    psrad     m2, 6

    packssdw  m0, m2

    movd      [r2], m0
    pextrd    [r2 + r3], m0, 1
    lea       r2, [r2 + 2 * r3]
    pextrd    [r2], m0, 2
    pextrd    [r2 + r3], m0, 3

    lea       r2, [r2 + 2 * r3]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

    FILTER_VER_CHROMA_SS_W2_4R 2, 4
    FILTER_VER_CHROMA_SS_W2_4R 2, 8

    FILTER_VER_CHROMA_SS_W2_4R 2, 16

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

    lea        r0, [r0 + 2 * r1]
    movq       m2, [r0]
    punpcklwd  m1, m2                          ;m1=[1 2]
    pmaddwd    m1, [r5 + 0 *16]                ;m1=[1+2]  Row2

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
%macro FILTER_VER_CHROMA_SS_W6_H4 2
INIT_XMM sse4
cglobal interp_4tap_vert_ss_6x%2, 5, 7, 6

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

    mov       r4d, %2/4

.loopH:
    PROCESS_CHROMA_SP_W4_4R

    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3

    movlps    [r2], m0
    movhps    [r2 + r3], m0
    lea       r5, [r2 + 2 * r3]
    movlps    [r5], m2
    movhps    [r5 + r3], m2

    lea       r5, [4 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 2 * 4

    PROCESS_CHROMA_SP_W2_4R r6

    psrad     m0, 6
    psrad     m2, 6

    packssdw  m0, m2

    movd      [r2], m0
    pextrd    [r2 + r3], m0, 1
    lea       r2, [r2 + 2 * r3]
    pextrd    [r2], m0, 2
    pextrd    [r2 + r3], m0, 3

    sub       r0, 2 * 4
    lea       r2, [r2 + 2 * r3 - 2 * 4]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

    FILTER_VER_CHROMA_SS_W6_H4 6, 8

    FILTER_VER_CHROMA_SS_W6_H4 6, 16


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
.loopH:
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

    FILTER_VER_CHROMA_SS_W8_H2 8, 12
    FILTER_VER_CHROMA_SS_W8_H2 8, 64

