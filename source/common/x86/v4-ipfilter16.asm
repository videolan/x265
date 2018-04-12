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
    %define INTERP_OFFSET_SP        v4_pd_524800
%elif BIT_DEPTH == 12
    %define INTERP_SHIFT_PS         4
    %define INTERP_OFFSET_PS        pd_n131072
    %define INTERP_SHIFT_SP         8
    %define INTERP_OFFSET_SP        pd_524416
%else
    %error Unsupport bit depth!
%endif


SECTION_RODATA 32

v4_pd_524800:        times 8 dd 524800
tab_c_n8192:      times 8 dw -8192

const tab_ChromaCoeffV,  times 8 dw 0, 64
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
                        
tab_ChromaCoeffVer: times 8 dw 0, 64
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
                        
SECTION .text
cextern pd_8
cextern pd_32
cextern pw_pixel_max
cextern pd_524416
cextern pd_n32768
cextern pd_n131072
cextern pw_2000
cextern idct8_shuf2

%macro PROCESS_CHROMA_SP_W4_4R 0
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r6 + 0 *32]                ;m0=[0+1]         Row1

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m1, m4                          ;m1=[1 2]
    pmaddwd    m1, [r6 + 0 *32]                ;m1=[1+2]         Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[2 3]
    pmaddwd    m2, m4, [r6 + 0 *32]            ;m2=[2+3]         Row3
    pmaddwd    m4, [r6 + 1 * 32]
    paddd      m0, m4                          ;m0=[0+1+2+3]     Row1 done

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m5, m4                          ;m5=[3 4]
    pmaddwd    m3, m5, [r6 + 0 *32]            ;m3=[3+4]         Row4
    pmaddwd    m5, [r6 + 1 * 32]
    paddd      m1, m5                          ;m1 = [1+2+3+4]   Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[4 5]
    pmaddwd    m4, [r6 + 1 * 32]
    paddd      m2, m4                          ;m2=[2+3+4+5]     Row3

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[5 6]
    pmaddwd    m5, [r6 + 1 * 32]
    paddd      m3, m5                          ;m3=[3+4+5+6]     Row4
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_%3_%1x%2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SS 4
INIT_XMM sse2
cglobal interp_4tap_vert_%3_%1x%2, 5, 7, %4 ,0-gprsize

    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_ChromaCoeffV + r4]
%endif

    mov       dword [rsp], %2/4

%ifnidn %3, ss
    %ifnidn %3, ps
        mova      m7, [pw_pixel_max]
        %ifidn %3, pp
            mova      m6, [INTERP_OFFSET_PP]
        %else
            mova      m6, [INTERP_OFFSET_SP]
        %endif
    %else
        mova      m6, [INTERP_OFFSET_PS]
    %endif
%endif

.loopH:
    mov       r4d, (%1/4)
.loopW:
    PROCESS_CHROMA_SP_W4_4R

%ifidn %3, ss
    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3
%elifidn %3, ps
    paddd     m0, m6
    paddd     m1, m6
    paddd     m2, m6
    paddd     m3, m6
    psrad     m0, INTERP_SHIFT_PS
    psrad     m1, INTERP_SHIFT_PS
    psrad     m2, INTERP_SHIFT_PS
    psrad     m3, INTERP_SHIFT_PS

    packssdw  m0, m1
    packssdw  m2, m3
%else
    paddd     m0, m6
    paddd     m1, m6
    paddd     m2, m6
    paddd     m3, m6
    %ifidn %3, pp
        psrad     m0, INTERP_SHIFT_PP
        psrad     m1, INTERP_SHIFT_PP
        psrad     m2, INTERP_SHIFT_PP
        psrad     m3, INTERP_SHIFT_PP
    %else
        psrad     m0, INTERP_SHIFT_SP
        psrad     m1, INTERP_SHIFT_SP
        psrad     m2, INTERP_SHIFT_SP
        psrad     m3, INTERP_SHIFT_SP
    %endif
    packssdw  m0, m1
    packssdw  m2, m3
    pxor      m5, m5
    CLIPW2    m0, m2, m5, m7
%endif

    movh      [r2], m0
    movhps    [r2 + r3], m0
    lea       r5, [r2 + 2 * r3]
    movh      [r5], m2
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

    FILTER_VER_CHROMA_SS 4, 4, ss, 6
    FILTER_VER_CHROMA_SS 4, 8, ss, 6
    FILTER_VER_CHROMA_SS 16, 16, ss, 6
    FILTER_VER_CHROMA_SS 16, 8, ss, 6
    FILTER_VER_CHROMA_SS 16, 12, ss, 6
    FILTER_VER_CHROMA_SS 12, 16, ss, 6
    FILTER_VER_CHROMA_SS 16, 4, ss, 6
    FILTER_VER_CHROMA_SS 4, 16, ss, 6
    FILTER_VER_CHROMA_SS 32, 32, ss, 6
    FILTER_VER_CHROMA_SS 32, 16, ss, 6
    FILTER_VER_CHROMA_SS 16, 32, ss, 6
    FILTER_VER_CHROMA_SS 32, 24, ss, 6
    FILTER_VER_CHROMA_SS 24, 32, ss, 6
    FILTER_VER_CHROMA_SS 32, 8, ss, 6

    FILTER_VER_CHROMA_SS 4, 4, ps, 7
    FILTER_VER_CHROMA_SS 4, 8, ps, 7
    FILTER_VER_CHROMA_SS 16, 16, ps, 7
    FILTER_VER_CHROMA_SS 16, 8, ps, 7
    FILTER_VER_CHROMA_SS 16, 12, ps, 7
    FILTER_VER_CHROMA_SS 12, 16, ps, 7
    FILTER_VER_CHROMA_SS 16, 4, ps, 7
    FILTER_VER_CHROMA_SS 4, 16, ps, 7
    FILTER_VER_CHROMA_SS 32, 32, ps, 7
    FILTER_VER_CHROMA_SS 32, 16, ps, 7
    FILTER_VER_CHROMA_SS 16, 32, ps, 7
    FILTER_VER_CHROMA_SS 32, 24, ps, 7
    FILTER_VER_CHROMA_SS 24, 32, ps, 7
    FILTER_VER_CHROMA_SS 32, 8, ps, 7

    FILTER_VER_CHROMA_SS 4, 4, sp, 8
    FILTER_VER_CHROMA_SS 4, 8, sp, 8
    FILTER_VER_CHROMA_SS 16, 16, sp, 8
    FILTER_VER_CHROMA_SS 16, 8, sp, 8
    FILTER_VER_CHROMA_SS 16, 12, sp, 8
    FILTER_VER_CHROMA_SS 12, 16, sp, 8
    FILTER_VER_CHROMA_SS 16, 4, sp, 8
    FILTER_VER_CHROMA_SS 4, 16, sp, 8
    FILTER_VER_CHROMA_SS 32, 32, sp, 8
    FILTER_VER_CHROMA_SS 32, 16, sp, 8
    FILTER_VER_CHROMA_SS 16, 32, sp, 8
    FILTER_VER_CHROMA_SS 32, 24, sp, 8
    FILTER_VER_CHROMA_SS 24, 32, sp, 8
    FILTER_VER_CHROMA_SS 32, 8, sp, 8

    FILTER_VER_CHROMA_SS 4, 4, pp, 8
    FILTER_VER_CHROMA_SS 4, 8, pp, 8
    FILTER_VER_CHROMA_SS 16, 16, pp, 8
    FILTER_VER_CHROMA_SS 16, 8, pp, 8
    FILTER_VER_CHROMA_SS 16, 12, pp, 8
    FILTER_VER_CHROMA_SS 12, 16, pp, 8
    FILTER_VER_CHROMA_SS 16, 4, pp, 8
    FILTER_VER_CHROMA_SS 4, 16, pp, 8
    FILTER_VER_CHROMA_SS 32, 32, pp, 8
    FILTER_VER_CHROMA_SS 32, 16, pp, 8
    FILTER_VER_CHROMA_SS 16, 32, pp, 8
    FILTER_VER_CHROMA_SS 32, 24, pp, 8
    FILTER_VER_CHROMA_SS 24, 32, pp, 8
    FILTER_VER_CHROMA_SS 32, 8, pp, 8


    FILTER_VER_CHROMA_SS 16, 24, ss, 6
    FILTER_VER_CHROMA_SS 12, 32, ss, 6
    FILTER_VER_CHROMA_SS 4, 32, ss, 6
    FILTER_VER_CHROMA_SS 32, 64, ss, 6
    FILTER_VER_CHROMA_SS 16, 64, ss, 6
    FILTER_VER_CHROMA_SS 32, 48, ss, 6
    FILTER_VER_CHROMA_SS 24, 64, ss, 6

    FILTER_VER_CHROMA_SS 16, 24, ps, 7
    FILTER_VER_CHROMA_SS 12, 32, ps, 7
    FILTER_VER_CHROMA_SS 4, 32, ps, 7
    FILTER_VER_CHROMA_SS 32, 64, ps, 7
    FILTER_VER_CHROMA_SS 16, 64, ps, 7
    FILTER_VER_CHROMA_SS 32, 48, ps, 7
    FILTER_VER_CHROMA_SS 24, 64, ps, 7

    FILTER_VER_CHROMA_SS 16, 24, sp, 8
    FILTER_VER_CHROMA_SS 12, 32, sp, 8
    FILTER_VER_CHROMA_SS 4, 32, sp, 8
    FILTER_VER_CHROMA_SS 32, 64, sp, 8
    FILTER_VER_CHROMA_SS 16, 64, sp, 8
    FILTER_VER_CHROMA_SS 32, 48, sp, 8
    FILTER_VER_CHROMA_SS 24, 64, sp, 8

    FILTER_VER_CHROMA_SS 16, 24, pp, 8
    FILTER_VER_CHROMA_SS 12, 32, pp, 8
    FILTER_VER_CHROMA_SS 4, 32, pp, 8
    FILTER_VER_CHROMA_SS 32, 64, pp, 8
    FILTER_VER_CHROMA_SS 16, 64, pp, 8
    FILTER_VER_CHROMA_SS 32, 48, pp, 8
    FILTER_VER_CHROMA_SS 24, 64, pp, 8


    FILTER_VER_CHROMA_SS 48, 64, ss, 6
    FILTER_VER_CHROMA_SS 64, 48, ss, 6
    FILTER_VER_CHROMA_SS 64, 64, ss, 6
    FILTER_VER_CHROMA_SS 64, 32, ss, 6
    FILTER_VER_CHROMA_SS 64, 16, ss, 6

    FILTER_VER_CHROMA_SS 48, 64, ps, 7
    FILTER_VER_CHROMA_SS 64, 48, ps, 7
    FILTER_VER_CHROMA_SS 64, 64, ps, 7
    FILTER_VER_CHROMA_SS 64, 32, ps, 7
    FILTER_VER_CHROMA_SS 64, 16, ps, 7

    FILTER_VER_CHROMA_SS 48, 64, sp, 8
    FILTER_VER_CHROMA_SS 64, 48, sp, 8
    FILTER_VER_CHROMA_SS 64, 64, sp, 8
    FILTER_VER_CHROMA_SS 64, 32, sp, 8
    FILTER_VER_CHROMA_SS 64, 16, sp, 8

    FILTER_VER_CHROMA_SS 48, 64, pp, 8
    FILTER_VER_CHROMA_SS 64, 48, pp, 8
    FILTER_VER_CHROMA_SS 64, 64, pp, 8
    FILTER_VER_CHROMA_SS 64, 32, pp, 8
    FILTER_VER_CHROMA_SS 64, 16, pp, 8


%macro PROCESS_CHROMA_SP_W2_4R 1
    movd       m0, [r0]
    movd       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]

    lea        r0, [r0 + 2 * r1]
    movd       m2, [r0]
    punpcklwd  m1, m2                          ;m1=[1 2]
    punpcklqdq m0, m1                          ;m0=[0 1 1 2]
    pmaddwd    m0, [%1 + 0 *32]                ;m0=[0+1 1+2] Row 1-2

    movd       m1, [r0 + r1]
    punpcklwd  m2, m1                          ;m2=[2 3]

    lea        r0, [r0 + 2 * r1]
    movd       m3, [r0]
    punpcklwd  m1, m3                          ;m2=[3 4]
    punpcklqdq m2, m1                          ;m2=[2 3 3 4]

    pmaddwd    m4, m2, [%1 + 1 * 32]           ;m4=[2+3 3+4] Row 1-2
    pmaddwd    m2, [%1 + 0 * 32]               ;m2=[2+3 3+4] Row 3-4
    paddd      m0, m4                          ;m0=[0+1+2+3 1+2+3+4] Row 1-2

    movd       m1, [r0 + r1]
    punpcklwd  m3, m1                          ;m3=[4 5]

    movd       m4, [r0 + 2 * r1]
    punpcklwd  m1, m4                          ;m1=[5 6]
    punpcklqdq m3, m1                          ;m2=[4 5 5 6]
    pmaddwd    m3, [%1 + 1 * 32]               ;m3=[4+5 5+6] Row 3-4
    paddd      m2, m3                          ;m2=[2+3+4+5 3+4+5+6] Row 3-4
%endmacro
;---------------------------------------------------------------------------------------------------------------------
; void interp_4tap_vertical_%2_2x%1(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W2 3
INIT_XMM sse4
cglobal interp_4tap_vert_%2_2x%1, 5, 6, %3

    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif

    mov       r4d, (%1/4)
%ifnidn %2, ss
    %ifnidn %2, ps
        pxor      m7, m7
        mova      m6, [pw_pixel_max]
        %ifidn %2, pp
            mova      m5, [INTERP_OFFSET_PP]
        %else
            mova      m5, [INTERP_OFFSET_SP]
        %endif
    %else
        mova      m5, [INTERP_OFFSET_PS]
    %endif
%endif

.loopH:
    PROCESS_CHROMA_SP_W2_4R r5
%ifidn %2, ss
    psrad     m0, 6
    psrad     m2, 6
    packssdw  m0, m2
%elifidn %2, ps
    paddd     m0, m5
    paddd     m2, m5
    psrad     m0, INTERP_SHIFT_PS
    psrad     m2, INTERP_SHIFT_PS
    packssdw  m0, m2
%else
    paddd     m0, m5
    paddd     m2, m5
    %ifidn %2, pp
        psrad     m0, INTERP_SHIFT_PP
        psrad     m2, INTERP_SHIFT_PP
    %else
        psrad     m0, INTERP_SHIFT_SP
        psrad     m2, INTERP_SHIFT_SP
    %endif
    packusdw  m0, m2
    CLIPW     m0, m7,    m6
%endif

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

FILTER_VER_CHROMA_W2 4, ss, 5
FILTER_VER_CHROMA_W2 8, ss, 5

FILTER_VER_CHROMA_W2 4, pp, 8
FILTER_VER_CHROMA_W2 8, pp, 8

FILTER_VER_CHROMA_W2 4, ps, 6
FILTER_VER_CHROMA_W2 8, ps, 6

FILTER_VER_CHROMA_W2 4, sp, 8
FILTER_VER_CHROMA_W2 8, sp, 8

FILTER_VER_CHROMA_W2 16, ss, 5
FILTER_VER_CHROMA_W2 16, pp, 8
FILTER_VER_CHROMA_W2 16, ps, 6
FILTER_VER_CHROMA_W2 16, sp, 8


;---------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_%1_4x2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W4 3
INIT_XMM sse4
cglobal interp_4tap_vert_%2_4x%1, 5, 6, %3
    add        r1d, r1d
    add        r3d, r3d
    sub        r0, r1
    shl        r4d, 6

%ifdef PIC
    lea        r5, [tab_ChromaCoeffV]
    lea        r5, [r5 + r4]
%else
    lea        r5, [tab_ChromaCoeffV + r4]
%endif

%ifnidn %2, 2
    mov        r4d, %1/2
%endif

%ifnidn %2, ss
    %ifnidn %2, ps
        pxor      m6, m6
        mova      m5, [pw_pixel_max]
        %ifidn %2, pp
            mova      m4, [INTERP_OFFSET_PP]
        %else
            mova      m4, [INTERP_OFFSET_SP]
        %endif
    %else
        mova      m4, [INTERP_OFFSET_PS]
    %endif
%endif

%ifnidn %2, 2
.loop:
%endif

    movh       m0, [r0]
    movh       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r5 + 0 *32]                ;m0=[0+1]  Row1

    lea        r0, [r0 + 2 * r1]
    movh       m2, [r0]
    punpcklwd  m1, m2                          ;m1=[1 2]
    pmaddwd    m1, [r5 + 0 *32]                ;m1=[1+2]  Row2

    movh       m3, [r0 + r1]
    punpcklwd  m2, m3                          ;m4=[2 3]
    pmaddwd    m2, [r5 + 1 * 32]
    paddd      m0, m2                          ;m0=[0+1+2+3]  Row1 done

    movh       m2, [r0 + 2 * r1]
    punpcklwd  m3, m2                          ;m5=[3 4]
    pmaddwd    m3, [r5 + 1 * 32]
    paddd      m1, m3                          ;m1=[1+2+3+4]  Row2 done

%ifidn %2, ss
    psrad     m0, 6
    psrad     m1, 6
    packssdw  m0, m1
%elifidn %2, ps
    paddd     m0, m4
    paddd     m1, m4
    psrad     m0, INTERP_SHIFT_PS
    psrad     m1, INTERP_SHIFT_PS
    packssdw  m0, m1
%else
    paddd     m0, m4
    paddd     m1, m4
    %ifidn %2, pp
        psrad     m0, INTERP_SHIFT_PP
        psrad     m1, INTERP_SHIFT_PP
    %else
        psrad     m0, INTERP_SHIFT_SP
        psrad     m1, INTERP_SHIFT_SP
    %endif
    packusdw  m0, m1
    CLIPW     m0, m6,    m5
%endif

    movh       [r2], m0
    movhps     [r2 + r3], m0

%ifnidn %2, 2
    lea        r2, [r2 + r3 * 2]
    dec        r4d
    jnz        .loop
%endif
    RET
%endmacro

FILTER_VER_CHROMA_W4 2, ss, 4
FILTER_VER_CHROMA_W4 2, pp, 7
FILTER_VER_CHROMA_W4 2, ps, 5
FILTER_VER_CHROMA_W4 2, sp, 7

FILTER_VER_CHROMA_W4 4, ss, 4
FILTER_VER_CHROMA_W4 4, pp, 7
FILTER_VER_CHROMA_W4 4, ps, 5
FILTER_VER_CHROMA_W4 4, sp, 7

;-------------------------------------------------------------------------------------------------------------------
; void interp_4tap_vertical_%1_6x8(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W6 3
INIT_XMM sse4
cglobal interp_4tap_vert_%2_6x%1, 5, 7, %3
    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_ChromaCoeffV + r4]
%endif

    mov       r4d, %1/4

%ifnidn %2, ss
    %ifnidn %2, ps
        mova      m7, [pw_pixel_max]
        %ifidn %2, pp
            mova      m6, [INTERP_OFFSET_PP]
        %else
            mova      m6, [INTERP_OFFSET_SP]
        %endif
    %else
        mova      m6, [INTERP_OFFSET_PS]
    %endif
%endif

.loopH:
    PROCESS_CHROMA_SP_W4_4R

%ifidn %2, ss
    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3
%elifidn %2, ps
    paddd     m0, m6
    paddd     m1, m6
    paddd     m2, m6
    paddd     m3, m6
    psrad     m0, INTERP_SHIFT_PS
    psrad     m1, INTERP_SHIFT_PS
    psrad     m2, INTERP_SHIFT_PS
    psrad     m3, INTERP_SHIFT_PS

    packssdw  m0, m1
    packssdw  m2, m3
%else
    paddd     m0, m6
    paddd     m1, m6
    paddd     m2, m6
    paddd     m3, m6
    %ifidn %2, pp
        psrad     m0, INTERP_SHIFT_PP
        psrad     m1, INTERP_SHIFT_PP
        psrad     m2, INTERP_SHIFT_PP
        psrad     m3, INTERP_SHIFT_PP
    %else
        psrad     m0, INTERP_SHIFT_SP
        psrad     m1, INTERP_SHIFT_SP
        psrad     m2, INTERP_SHIFT_SP
        psrad     m3, INTERP_SHIFT_SP
    %endif
    packssdw  m0, m1
    packssdw  m2, m3
    pxor      m5, m5
    CLIPW2    m0, m2, m5, m7
%endif

    movh      [r2], m0
    movhps    [r2 + r3], m0
    lea       r5, [r2 + 2 * r3]
    movh      [r5], m2
    movhps    [r5 + r3], m2

    lea       r5, [4 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 2 * 4

    PROCESS_CHROMA_SP_W2_4R r6

%ifidn %2, ss
    psrad     m0, 6
    psrad     m2, 6
    packssdw  m0, m2
%elifidn %2, ps
    paddd     m0, m6
    paddd     m2, m6
    psrad     m0, INTERP_SHIFT_PS
    psrad     m2, INTERP_SHIFT_PS
    packssdw  m0, m2
%else
    paddd     m0, m6
    paddd     m2, m6
    %ifidn %2, pp
        psrad     m0, INTERP_SHIFT_PP
        psrad     m2, INTERP_SHIFT_PP
    %else
        psrad     m0, INTERP_SHIFT_SP
        psrad     m2, INTERP_SHIFT_SP
    %endif
    packusdw  m0, m2
    CLIPW     m0, m5,    m7
%endif

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

FILTER_VER_CHROMA_W6 8, ss, 6
FILTER_VER_CHROMA_W6 8, ps, 7
FILTER_VER_CHROMA_W6 8, sp, 8
FILTER_VER_CHROMA_W6 8, pp, 8

FILTER_VER_CHROMA_W6 16, ss, 6
FILTER_VER_CHROMA_W6 16, ps, 7
FILTER_VER_CHROMA_W6 16, sp, 8
FILTER_VER_CHROMA_W6 16, pp, 8

%macro PROCESS_CHROMA_SP_W8_2R 0
    movu       m1, [r0]
    movu       m3, [r0 + r1]
    punpcklwd  m0, m1, m3
    pmaddwd    m0, [r5 + 0 * 32]                ;m0 = [0l+1l]  Row1l
    punpckhwd  m1, m3
    pmaddwd    m1, [r5 + 0 * 32]                ;m1 = [0h+1h]  Row1h

    movu       m4, [r0 + 2 * r1]
    punpcklwd  m2, m3, m4
    pmaddwd    m2, [r5 + 0 * 32]                ;m2 = [1l+2l]  Row2l
    punpckhwd  m3, m4
    pmaddwd    m3, [r5 + 0 * 32]                ;m3 = [1h+2h]  Row2h

    lea        r0, [r0 + 2 * r1]
    movu       m5, [r0 + r1]
    punpcklwd  m6, m4, m5
    pmaddwd    m6, [r5 + 1 * 32]                ;m6 = [2l+3l]  Row1l
    paddd      m0, m6                           ;m0 = [0l+1l+2l+3l]  Row1l sum
    punpckhwd  m4, m5
    pmaddwd    m4, [r5 + 1 * 32]                ;m6 = [2h+3h]  Row1h
    paddd      m1, m4                           ;m1 = [0h+1h+2h+3h]  Row1h sum

    movu       m4, [r0 + 2 * r1]
    punpcklwd  m6, m5, m4
    pmaddwd    m6, [r5 + 1 * 32]                ;m6 = [3l+4l]  Row2l
    paddd      m2, m6                           ;m2 = [1l+2l+3l+4l]  Row2l sum
    punpckhwd  m5, m4
    pmaddwd    m5, [r5 + 1 * 32]                ;m1 = [3h+4h]  Row2h
    paddd      m3, m5                           ;m3 = [1h+2h+3h+4h]  Row2h sum
%endmacro

;----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_%3_%1x%2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W8 4
INIT_XMM sse2
cglobal interp_4tap_vert_%3_%1x%2, 5, 6, %4

    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif

    mov       r4d, %2/2

%ifidn %3, pp
    mova      m7, [INTERP_OFFSET_PP]
%elifidn %3, sp
    mova      m7, [INTERP_OFFSET_SP]
%elifidn %3, ps
    mova      m7, [INTERP_OFFSET_PS]
%endif

.loopH:
    PROCESS_CHROMA_SP_W8_2R

%ifidn %3, ss
    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3
%elifidn %3, ps
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
    psrad     m0, INTERP_SHIFT_PS
    psrad     m1, INTERP_SHIFT_PS
    psrad     m2, INTERP_SHIFT_PS
    psrad     m3, INTERP_SHIFT_PS

    packssdw  m0, m1
    packssdw  m2, m3
%else
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
    %ifidn %3, pp
        psrad     m0, INTERP_SHIFT_PP
        psrad     m1, INTERP_SHIFT_PP
        psrad     m2, INTERP_SHIFT_PP
        psrad     m3, INTERP_SHIFT_PP
    %else
        psrad     m0, INTERP_SHIFT_SP
        psrad     m1, INTERP_SHIFT_SP
        psrad     m2, INTERP_SHIFT_SP
        psrad     m3, INTERP_SHIFT_SP
    %endif
    packssdw  m0, m1
    packssdw  m2, m3
    pxor      m5, m5
    mova      m6, [pw_pixel_max]
    CLIPW2    m0, m2, m5, m6
%endif

    movu      [r2], m0
    movu      [r2 + r3], m2

    lea       r2, [r2 + 2 * r3]

    dec       r4d
    jnz       .loopH
    RET
%endmacro

FILTER_VER_CHROMA_W8 8, 2, ss, 7
FILTER_VER_CHROMA_W8 8, 4, ss, 7
FILTER_VER_CHROMA_W8 8, 6, ss, 7
FILTER_VER_CHROMA_W8 8, 8, ss, 7
FILTER_VER_CHROMA_W8 8, 16, ss, 7
FILTER_VER_CHROMA_W8 8, 32, ss, 7

FILTER_VER_CHROMA_W8 8, 2, sp, 8
FILTER_VER_CHROMA_W8 8, 4, sp, 8
FILTER_VER_CHROMA_W8 8, 6, sp, 8
FILTER_VER_CHROMA_W8 8, 8, sp, 8
FILTER_VER_CHROMA_W8 8, 16, sp, 8
FILTER_VER_CHROMA_W8 8, 32, sp, 8

FILTER_VER_CHROMA_W8 8, 2, ps, 8
FILTER_VER_CHROMA_W8 8, 4, ps, 8
FILTER_VER_CHROMA_W8 8, 6, ps, 8
FILTER_VER_CHROMA_W8 8, 8, ps, 8
FILTER_VER_CHROMA_W8 8, 16, ps, 8
FILTER_VER_CHROMA_W8 8, 32, ps, 8

FILTER_VER_CHROMA_W8 8, 2, pp, 8
FILTER_VER_CHROMA_W8 8, 4, pp, 8
FILTER_VER_CHROMA_W8 8, 6, pp, 8
FILTER_VER_CHROMA_W8 8, 8, pp, 8
FILTER_VER_CHROMA_W8 8, 16, pp, 8
FILTER_VER_CHROMA_W8 8, 32, pp, 8

FILTER_VER_CHROMA_W8 8, 12, ss, 7
FILTER_VER_CHROMA_W8 8, 64, ss, 7
FILTER_VER_CHROMA_W8 8, 12, sp, 8
FILTER_VER_CHROMA_W8 8, 64, sp, 8
FILTER_VER_CHROMA_W8 8, 12, ps, 8
FILTER_VER_CHROMA_W8 8, 64, ps, 8
FILTER_VER_CHROMA_W8 8, 12, pp, 8
FILTER_VER_CHROMA_W8 8, 64, pp, 8

%macro PROCESS_CHROMA_VERT_W16_2R 0
    movu       m1, [r0]
    movu       m3, [r0 + r1]
    punpcklwd  m0, m1, m3
    pmaddwd    m0, [r5 + 0 * 32]
    punpckhwd  m1, m3
    pmaddwd    m1, [r5 + 0 * 32]

    movu       m4, [r0 + 2 * r1]
    punpcklwd  m2, m3, m4
    pmaddwd    m2, [r5 + 0 * 32]
    punpckhwd  m3, m4
    pmaddwd    m3, [r5 + 0 * 32]

    lea        r0, [r0 + 2 * r1]
    movu       m5, [r0 + r1]
    punpcklwd  m6, m4, m5
    pmaddwd    m6, [r5 + 1 * 32]
    paddd      m0, m6
    punpckhwd  m4, m5
    pmaddwd    m4, [r5 + 1 * 32]
    paddd      m1, m4

    movu       m4, [r0 + 2 * r1]
    punpcklwd  m6, m5, m4
    pmaddwd    m6, [r5 + 1 * 32]
    paddd      m2, m6
    punpckhwd  m5, m4
    pmaddwd    m5, [r5 + 1 * 32]
    paddd      m3, m5
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_AVX2_6xN 2
INIT_YMM avx2
%if ARCH_X86_64
cglobal interp_4tap_vert_%2_6x%1, 4, 7, 10
    mov             r4d, r4m
    add             r1d, r1d
    add             r3d, r3d
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffV]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffV + r4]
%endif

    sub             r0, r1
    mov             r6d, %1/4

%ifidn %2,pp
    vbroadcasti128  m8, [INTERP_OFFSET_PP]
%elifidn %2, sp
    vbroadcasti128  m8, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m8, [INTERP_OFFSET_PS]
%endif

.loopH:
    movu            xm0, [r0]
    movu            xm1, [r0 + r1]
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]

    movu            xm2, [r0 + r1 * 2]
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]

    lea             r4, [r1 * 3]
    movu            xm3, [r0 + r4]
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    pmaddwd         m2, [r5]
    paddd           m0, m4

    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    pmaddwd         m3, [r5]
    paddd           m1, m5

    movu            xm5, [r0 + r1]
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    pmaddwd         m4, [r5]
    paddd           m2, m6

    movu            xm6, [r0 + r1 * 2]
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m7
    lea             r4, [r3 * 3]
%ifidn %2,ss
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
%else
    paddd           m0, m8
    paddd           m1, m8
    paddd           m2, m8
    paddd           m3, m8
%ifidn %2,pp
    psrad           m0, INTERP_SHIFT_PP
    psrad           m1, INTERP_SHIFT_PP
    psrad           m2, INTERP_SHIFT_PP
    psrad           m3, INTERP_SHIFT_PP
%elifidn %2, sp
    psrad           m0, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
    psrad           m2, INTERP_SHIFT_SP
    psrad           m3, INTERP_SHIFT_SP
%else
    psrad           m0, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
    psrad           m2, INTERP_SHIFT_PS
    psrad           m3, INTERP_SHIFT_PS
%endif
%endif

    packssdw        m0, m1
    packssdw        m2, m3
    vpermq          m0, m0, q3120
    vpermq          m2, m2, q3120
    pxor            m5, m5
    mova            m9, [pw_pixel_max]
%ifidn %2,pp
    CLIPW           m0, m5, m9
    CLIPW           m2, m5, m9
%elifidn %2, sp
    CLIPW           m0, m5, m9
    CLIPW           m2, m5, m9
%endif

    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    movq            [r2], xm0
    pextrd          [r2 + 8], xm0, 2
    movq            [r2 + r3], xm1
    pextrd          [r2 + r3 + 8], xm1, 2
    movq            [r2 + r3 * 2], xm2
    pextrd          [r2 + r3 * 2 + 8], xm2, 2
    movq            [r2 + r4], xm3
    pextrd          [r2 + r4 + 8], xm3, 2

    lea             r2, [r2 + r3 * 4]
    dec r6d
    jnz .loopH
    RET
%endif
%endmacro
FILTER_VER_CHROMA_AVX2_6xN 8, pp
FILTER_VER_CHROMA_AVX2_6xN 8, ps
FILTER_VER_CHROMA_AVX2_6xN 8, ss
FILTER_VER_CHROMA_AVX2_6xN 8, sp
FILTER_VER_CHROMA_AVX2_6xN 16, pp
FILTER_VER_CHROMA_AVX2_6xN 16, ps
FILTER_VER_CHROMA_AVX2_6xN 16, ss
FILTER_VER_CHROMA_AVX2_6xN 16, sp

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W16_16xN_avx2 3
INIT_YMM avx2
cglobal interp_4tap_vert_%2_16x%1, 5, 6, %3
    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif

    mov       r4d, %1/2

%ifidn %2, pp
    vbroadcasti128  m7, [INTERP_OFFSET_PP]
%elifidn %2, sp
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%elifidn %2, ps
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
%endif

.loopH:
    PROCESS_CHROMA_VERT_W16_2R
%ifidn %2, ss
    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3
%elifidn %2, ps
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
    psrad     m0, INTERP_SHIFT_PS
    psrad     m1, INTERP_SHIFT_PS
    psrad     m2, INTERP_SHIFT_PS
    psrad     m3, INTERP_SHIFT_PS

    packssdw  m0, m1
    packssdw  m2, m3
%else
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
 %ifidn %2, pp
    psrad     m0, INTERP_SHIFT_PP
    psrad     m1, INTERP_SHIFT_PP
    psrad     m2, INTERP_SHIFT_PP
    psrad     m3, INTERP_SHIFT_PP
%else
    psrad     m0, INTERP_SHIFT_SP
    psrad     m1, INTERP_SHIFT_SP
    psrad     m2, INTERP_SHIFT_SP
    psrad     m3, INTERP_SHIFT_SP
%endif
    packssdw  m0, m1
    packssdw  m2, m3
    pxor      m5, m5
    CLIPW2    m0, m2, m5, [pw_pixel_max]
%endif

    movu      [r2], m0
    movu      [r2 + r3], m2
    lea       r2, [r2 + 2 * r3]
    dec       r4d
    jnz       .loopH
    RET
%endmacro
    FILTER_VER_CHROMA_W16_16xN_avx2 4, pp, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 8, pp, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 12, pp, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 24, pp, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 16, pp, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 32, pp, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 64, pp, 8

    FILTER_VER_CHROMA_W16_16xN_avx2 4, ps, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 8, ps, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 12, ps, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 24, ps, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 16, ps, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 32, ps, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 64, ps, 8

    FILTER_VER_CHROMA_W16_16xN_avx2 4, ss, 7
    FILTER_VER_CHROMA_W16_16xN_avx2 8, ss, 7
    FILTER_VER_CHROMA_W16_16xN_avx2 12, ss, 7
    FILTER_VER_CHROMA_W16_16xN_avx2 24, ss, 7
    FILTER_VER_CHROMA_W16_16xN_avx2 16, ss, 7
    FILTER_VER_CHROMA_W16_16xN_avx2 32, ss, 7
    FILTER_VER_CHROMA_W16_16xN_avx2 64, ss, 7

    FILTER_VER_CHROMA_W16_16xN_avx2 4, sp, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 8, sp, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 12, sp, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 24, sp, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 16, sp, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 32, sp, 8
    FILTER_VER_CHROMA_W16_16xN_avx2 64, sp, 8

%macro PROCESS_CHROMA_VERT_W32_2R 0
    movu       m1, [r0]
    movu       m3, [r0 + r1]
    punpcklwd  m0, m1, m3
    pmaddwd    m0, [r5 + 0 * mmsize]
    punpckhwd  m1, m3
    pmaddwd    m1, [r5 + 0 * mmsize]

    movu       m9, [r0 + mmsize]
    movu       m11, [r0 + r1 + mmsize]
    punpcklwd  m8, m9, m11
    pmaddwd    m8, [r5 + 0 * mmsize]
    punpckhwd  m9, m11
    pmaddwd    m9, [r5 + 0 * mmsize]

    movu       m4, [r0 + 2 * r1]
    punpcklwd  m2, m3, m4
    pmaddwd    m2, [r5 + 0 * mmsize]
    punpckhwd  m3, m4
    pmaddwd    m3, [r5 + 0 * mmsize]

    movu       m12, [r0 + 2 * r1 + mmsize]
    punpcklwd  m10, m11, m12
    pmaddwd    m10, [r5 + 0 * mmsize]
    punpckhwd  m11, m12
    pmaddwd    m11, [r5 + 0 * mmsize]

    lea        r6, [r0 + 2 * r1]
    movu       m5, [r6 + r1]
    punpcklwd  m6, m4, m5
    pmaddwd    m6, [r5 + 1 * mmsize]
    paddd      m0, m6
    punpckhwd  m4, m5
    pmaddwd    m4, [r5 + 1 * mmsize]
    paddd      m1, m4

    movu       m13, [r6 + r1 + mmsize]
    punpcklwd  m14, m12, m13
    pmaddwd    m14, [r5 + 1 * mmsize]
    paddd      m8, m14
    punpckhwd  m12, m13
    pmaddwd    m12, [r5 + 1 * mmsize]
    paddd      m9, m12

    movu       m4, [r6 + 2 * r1]
    punpcklwd  m6, m5, m4
    pmaddwd    m6, [r5 + 1 * mmsize]
    paddd      m2, m6
    punpckhwd  m5, m4
    pmaddwd    m5, [r5 + 1 * mmsize]
    paddd      m3, m5

    movu       m12, [r6 + 2 * r1 + mmsize]
    punpcklwd  m14, m13, m12
    pmaddwd    m14, [r5 + 1 * mmsize]
    paddd      m10, m14
    punpckhwd  m13, m12
    pmaddwd    m13, [r5 + 1 * mmsize]
    paddd      m11, m13
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W16_32xN_avx2 3
INIT_YMM avx2
%if ARCH_X86_64
cglobal interp_4tap_vert_%2_32x%1, 5, 7, %3
    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif
    mov       r4d, %1/2

%ifidn %2, pp
    vbroadcasti128  m7, [INTERP_OFFSET_PP]
%elifidn %2, sp
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%elifidn %2, ps
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
%endif

.loopH:
    PROCESS_CHROMA_VERT_W32_2R
%ifidn %2, ss
    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    psrad     m8, 6
    psrad     m9, 6
    psrad     m10, 6
    psrad     m11, 6

    packssdw  m0, m1
    packssdw  m2, m3
    packssdw  m8, m9
    packssdw  m10, m11
%elifidn %2, ps
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
    psrad     m0, INTERP_SHIFT_PS
    psrad     m1, INTERP_SHIFT_PS
    psrad     m2, INTERP_SHIFT_PS
    psrad     m3, INTERP_SHIFT_PS
    paddd     m8, m7
    paddd     m9, m7
    paddd     m10, m7
    paddd     m11, m7
    psrad     m8, INTERP_SHIFT_PS
    psrad     m9, INTERP_SHIFT_PS
    psrad     m10, INTERP_SHIFT_PS
    psrad     m11, INTERP_SHIFT_PS

    packssdw  m0, m1
    packssdw  m2, m3
    packssdw  m8, m9
    packssdw  m10, m11
%else
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
    paddd     m8, m7
    paddd     m9, m7
    paddd     m10, m7
    paddd     m11, m7
 %ifidn %2, pp
    psrad     m0, INTERP_SHIFT_PP
    psrad     m1, INTERP_SHIFT_PP
    psrad     m2, INTERP_SHIFT_PP
    psrad     m3, INTERP_SHIFT_PP
    psrad     m8, INTERP_SHIFT_PP
    psrad     m9, INTERP_SHIFT_PP
    psrad     m10, INTERP_SHIFT_PP
    psrad     m11, INTERP_SHIFT_PP
%else
    psrad     m0, INTERP_SHIFT_SP
    psrad     m1, INTERP_SHIFT_SP
    psrad     m2, INTERP_SHIFT_SP
    psrad     m3, INTERP_SHIFT_SP
    psrad     m8, INTERP_SHIFT_SP
    psrad     m9, INTERP_SHIFT_SP
    psrad     m10, INTERP_SHIFT_SP
    psrad     m11, INTERP_SHIFT_SP
%endif
    packssdw  m0, m1
    packssdw  m2, m3
    packssdw  m8, m9
    packssdw  m10, m11
    pxor      m5, m5
    CLIPW2    m0, m2, m5, [pw_pixel_max]
    CLIPW2    m8, m10, m5, [pw_pixel_max]
%endif

    movu      [r2], m0
    movu      [r2 + r3], m2
    movu      [r2 + mmsize], m8
    movu      [r2 + r3 + mmsize], m10
    lea       r2, [r2 + 2 * r3]
    lea       r0, [r0 + 2 * r1]
    dec       r4d
    jnz       .loopH
    RET
%endif
%endmacro
    FILTER_VER_CHROMA_W16_32xN_avx2 8, pp, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 16, pp, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 24, pp, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 32, pp, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 48, pp, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 64, pp, 15

    FILTER_VER_CHROMA_W16_32xN_avx2 8, ps, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 16, ps, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 24, ps, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 32, ps, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 48, ps, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 64, ps, 15

    FILTER_VER_CHROMA_W16_32xN_avx2 8, ss, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 16, ss, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 24, ss, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 32, ss, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 48, ss, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 64, ss, 15

    FILTER_VER_CHROMA_W16_32xN_avx2 8, sp, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 16, sp, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 24, sp, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 32, sp, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 48, sp, 15
    FILTER_VER_CHROMA_W16_32xN_avx2 64, sp, 15

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W16_64xN_avx2 3
INIT_YMM avx2
cglobal interp_4tap_vert_%2_64x%1, 5, 7, %3
    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif
    mov       r4d, %1/2

%ifidn %2, pp
    vbroadcasti128  m7, [INTERP_OFFSET_PP]
%elifidn %2, sp
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%elifidn %2, ps
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
%endif

.loopH:
%assign x 0
%rep 4
    movu       m1, [r0 + x]
    movu       m3, [r0 + r1 + x]
    movu       m5, [r5 + 0 * mmsize]
    punpcklwd  m0, m1, m3
    pmaddwd    m0, m5
    punpckhwd  m1, m3
    pmaddwd    m1, m5

    movu       m4, [r0 + 2 * r1 + x]
    punpcklwd  m2, m3, m4
    pmaddwd    m2, m5
    punpckhwd  m3, m4
    pmaddwd    m3, m5

    lea        r6, [r0 + 2 * r1]
    movu       m5, [r6 + r1 + x]
    punpcklwd  m6, m4, m5
    pmaddwd    m6, [r5 + 1 * mmsize]
    paddd      m0, m6
    punpckhwd  m4, m5
    pmaddwd    m4, [r5 + 1 * mmsize]
    paddd      m1, m4

    movu       m4, [r6 + 2 * r1 + x]
    punpcklwd  m6, m5, m4
    pmaddwd    m6, [r5 + 1 * mmsize]
    paddd      m2, m6
    punpckhwd  m5, m4
    pmaddwd    m5, [r5 + 1 * mmsize]
    paddd      m3, m5

%ifidn %2, ss
    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3
%elifidn %2, ps
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
    psrad     m0, INTERP_SHIFT_PS
    psrad     m1, INTERP_SHIFT_PS
    psrad     m2, INTERP_SHIFT_PS
    psrad     m3, INTERP_SHIFT_PS

    packssdw  m0, m1
    packssdw  m2, m3
%else
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
%ifidn %2, pp
    psrad     m0, INTERP_SHIFT_PP
    psrad     m1, INTERP_SHIFT_PP
    psrad     m2, INTERP_SHIFT_PP
    psrad     m3, INTERP_SHIFT_PP
%else
    psrad     m0, INTERP_SHIFT_SP
    psrad     m1, INTERP_SHIFT_SP
    psrad     m2, INTERP_SHIFT_SP
    psrad     m3, INTERP_SHIFT_SP
%endif
    packssdw  m0, m1
    packssdw  m2, m3
    pxor      m5, m5
    CLIPW2    m0, m2, m5, [pw_pixel_max]
%endif

    movu      [r2 + x], m0
    movu      [r2 + r3 + x], m2
%assign x x+mmsize
%endrep

    lea       r2, [r2 + 2 * r3]
    lea       r0, [r0 + 2 * r1]
    dec       r4d
    jnz       .loopH
    RET
%endmacro
    FILTER_VER_CHROMA_W16_64xN_avx2 16, ss, 7
    FILTER_VER_CHROMA_W16_64xN_avx2 32, ss, 7
    FILTER_VER_CHROMA_W16_64xN_avx2 48, ss, 7
    FILTER_VER_CHROMA_W16_64xN_avx2 64, ss, 7
    FILTER_VER_CHROMA_W16_64xN_avx2 16, sp, 8
    FILTER_VER_CHROMA_W16_64xN_avx2 32, sp, 8
    FILTER_VER_CHROMA_W16_64xN_avx2 48, sp, 8
    FILTER_VER_CHROMA_W16_64xN_avx2 64, sp, 8
    FILTER_VER_CHROMA_W16_64xN_avx2 16, ps, 8
    FILTER_VER_CHROMA_W16_64xN_avx2 32, ps, 8
    FILTER_VER_CHROMA_W16_64xN_avx2 48, ps, 8
    FILTER_VER_CHROMA_W16_64xN_avx2 64, ps, 8
    FILTER_VER_CHROMA_W16_64xN_avx2 16, pp, 8
    FILTER_VER_CHROMA_W16_64xN_avx2 32, pp, 8
    FILTER_VER_CHROMA_W16_64xN_avx2 48, pp, 8
    FILTER_VER_CHROMA_W16_64xN_avx2 64, pp, 8

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W16_12xN_avx2 3
INIT_YMM avx2
cglobal interp_4tap_vert_%2_12x%1, 5, 8, %3
    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif
    mov       r4d, %1/2

%ifidn %2, pp
    vbroadcasti128  m7, [INTERP_OFFSET_PP]
%elifidn %2, sp
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%elifidn %2, ps
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
%endif

.loopH:
    PROCESS_CHROMA_VERT_W16_2R
%ifidn %2, ss
    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3
%elifidn %2, ps
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
    psrad     m0, INTERP_SHIFT_PS
    psrad     m1, INTERP_SHIFT_PS
    psrad     m2, INTERP_SHIFT_PS
    psrad     m3, INTERP_SHIFT_PS

    packssdw  m0, m1
    packssdw  m2, m3
%else
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
 %ifidn %2, pp
    psrad     m0, INTERP_SHIFT_PP
    psrad     m1, INTERP_SHIFT_PP
    psrad     m2, INTERP_SHIFT_PP
    psrad     m3, INTERP_SHIFT_PP
%else
    psrad     m0, INTERP_SHIFT_SP
    psrad     m1, INTERP_SHIFT_SP
    psrad     m2, INTERP_SHIFT_SP
    psrad     m3, INTERP_SHIFT_SP
%endif
    packssdw  m0, m1
    packssdw  m2, m3
    pxor      m5, m5
    CLIPW2    m0, m2, m5, [pw_pixel_max]
%endif

    movu      [r2], xm0
    movu      [r2 + r3], xm2
    vextracti128 xm0, m0, 1
    vextracti128 xm2, m2, 1
    movq      [r2 + 16], xm0
    movq      [r2 + r3 + 16], xm2
    lea       r2, [r2 + 2 * r3]
    dec       r4d
    jnz       .loopH
    RET
%endmacro
    FILTER_VER_CHROMA_W16_12xN_avx2 16, ss, 7
    FILTER_VER_CHROMA_W16_12xN_avx2 16, sp, 8
    FILTER_VER_CHROMA_W16_12xN_avx2 16, ps, 8
    FILTER_VER_CHROMA_W16_12xN_avx2 16, pp, 8
    FILTER_VER_CHROMA_W16_12xN_avx2 32, ss, 7
    FILTER_VER_CHROMA_W16_12xN_avx2 32, sp, 8
    FILTER_VER_CHROMA_W16_12xN_avx2 32, ps, 8
    FILTER_VER_CHROMA_W16_12xN_avx2 32, pp, 8

%macro PROCESS_CHROMA_VERT_W24_2R 0
    movu       m1, [r0]
    movu       m3, [r0 + r1]
    punpcklwd  m0, m1, m3
    pmaddwd    m0, [r5 + 0 * mmsize]
    punpckhwd  m1, m3
    pmaddwd    m1, [r5 + 0 * mmsize]

    movu       xm9, [r0 + mmsize]
    movu       xm11, [r0 + r1 + mmsize]
    punpcklwd  xm8, xm9, xm11
    pmaddwd    xm8, [r5 + 0 * mmsize]
    punpckhwd  xm9, xm11
    pmaddwd    xm9, [r5 + 0 * mmsize]

    movu       m4, [r0 + 2 * r1]
    punpcklwd  m2, m3, m4
    pmaddwd    m2, [r5 + 0 * mmsize]
    punpckhwd  m3, m4
    pmaddwd    m3, [r5 + 0 * mmsize]

    movu       xm12, [r0 + 2 * r1 + mmsize]
    punpcklwd  xm10, xm11, xm12
    pmaddwd    xm10, [r5 + 0 * mmsize]
    punpckhwd  xm11, xm12
    pmaddwd    xm11, [r5 + 0 * mmsize]

    lea        r6, [r0 + 2 * r1]
    movu       m5, [r6 + r1]
    punpcklwd  m6, m4, m5
    pmaddwd    m6, [r5 + 1 * mmsize]
    paddd      m0, m6
    punpckhwd  m4, m5
    pmaddwd    m4, [r5 + 1 * mmsize]
    paddd      m1, m4

    movu       xm13, [r6 + r1 + mmsize]
    punpcklwd  xm14, xm12, xm13
    pmaddwd    xm14, [r5 + 1 * mmsize]
    paddd      xm8, xm14
    punpckhwd  xm12, xm13
    pmaddwd    xm12, [r5 + 1 * mmsize]
    paddd      xm9, xm12

    movu       m4, [r6 + 2 * r1]
    punpcklwd  m6, m5, m4
    pmaddwd    m6, [r5 + 1 * mmsize]
    paddd      m2, m6
    punpckhwd  m5, m4
    pmaddwd    m5, [r5 + 1 * mmsize]
    paddd      m3, m5

    movu       xm12, [r6 + 2 * r1 + mmsize]
    punpcklwd  xm14, xm13, xm12
    pmaddwd    xm14, [r5 + 1 * mmsize]
    paddd      xm10, xm14
    punpckhwd  xm13, xm12
    pmaddwd    xm13, [r5 + 1 * mmsize]
    paddd      xm11, xm13
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W16_24xN_avx2 3
INIT_YMM avx2
%if ARCH_X86_64
cglobal interp_4tap_vert_%2_24x%1, 5, 7, %3
    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif
    mov       r4d, %1/2

%ifidn %2, pp
    vbroadcasti128  m7, [INTERP_OFFSET_PP]
%elifidn %2, sp
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%elifidn %2, ps
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
%endif

.loopH:
    PROCESS_CHROMA_VERT_W24_2R
%ifidn %2, ss
    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    psrad     m8, 6
    psrad     m9, 6
    psrad     m10, 6
    psrad     m11, 6

    packssdw  m0, m1
    packssdw  m2, m3
    packssdw  m8, m9
    packssdw  m10, m11
%elifidn %2, ps
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
    psrad     m0, INTERP_SHIFT_PS
    psrad     m1, INTERP_SHIFT_PS
    psrad     m2, INTERP_SHIFT_PS
    psrad     m3, INTERP_SHIFT_PS
    paddd     m8, m7
    paddd     m9, m7
    paddd     m10, m7
    paddd     m11, m7
    psrad     m8, INTERP_SHIFT_PS
    psrad     m9, INTERP_SHIFT_PS
    psrad     m10, INTERP_SHIFT_PS
    psrad     m11, INTERP_SHIFT_PS

    packssdw  m0, m1
    packssdw  m2, m3
    packssdw  m8, m9
    packssdw  m10, m11
%else
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
    paddd     m8, m7
    paddd     m9, m7
    paddd     m10, m7
    paddd     m11, m7
 %ifidn %2, pp
    psrad     m0, INTERP_SHIFT_PP
    psrad     m1, INTERP_SHIFT_PP
    psrad     m2, INTERP_SHIFT_PP
    psrad     m3, INTERP_SHIFT_PP
    psrad     m8, INTERP_SHIFT_PP
    psrad     m9, INTERP_SHIFT_PP
    psrad     m10, INTERP_SHIFT_PP
    psrad     m11, INTERP_SHIFT_PP
%else
    psrad     m0, INTERP_SHIFT_SP
    psrad     m1, INTERP_SHIFT_SP
    psrad     m2, INTERP_SHIFT_SP
    psrad     m3, INTERP_SHIFT_SP
    psrad     m8, INTERP_SHIFT_SP
    psrad     m9, INTERP_SHIFT_SP
    psrad     m10, INTERP_SHIFT_SP
    psrad     m11, INTERP_SHIFT_SP
%endif
    packssdw  m0, m1
    packssdw  m2, m3
    packssdw  m8, m9
    packssdw  m10, m11
    pxor      m5, m5
    CLIPW2    m0, m2, m5, [pw_pixel_max]
    CLIPW2    m8, m10, m5, [pw_pixel_max]
%endif

    movu      [r2], m0
    movu      [r2 + r3], m2
    movu      [r2 + mmsize], xm8
    movu      [r2 + r3 + mmsize], xm10
    lea       r2, [r2 + 2 * r3]
    lea       r0, [r0 + 2 * r1]
    dec       r4d
    jnz       .loopH
    RET
%endif
%endmacro
    FILTER_VER_CHROMA_W16_24xN_avx2 32, ss, 15
    FILTER_VER_CHROMA_W16_24xN_avx2 32, sp, 15
    FILTER_VER_CHROMA_W16_24xN_avx2 32, ps, 15
    FILTER_VER_CHROMA_W16_24xN_avx2 32, pp, 15
    FILTER_VER_CHROMA_W16_24xN_avx2 64, ss, 15
    FILTER_VER_CHROMA_W16_24xN_avx2 64, sp, 15
    FILTER_VER_CHROMA_W16_24xN_avx2 64, ps, 15
    FILTER_VER_CHROMA_W16_24xN_avx2 64, pp, 15

   
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W16_48x64_avx2 2
INIT_YMM avx2
cglobal interp_4tap_vert_%1_48x64, 5, 7, %2
    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif
    mov       r4d, 32

%ifidn %1, pp
    vbroadcasti128  m7, [INTERP_OFFSET_PP]
%elifidn %1, sp
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%elifidn %1, ps
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
%endif

.loopH:
%assign x 0
%rep 3
    movu       m1, [r0 + x]
    movu       m3, [r0 + r1 + x]
    movu       m5, [r5 + 0 * mmsize]
    punpcklwd  m0, m1, m3
    pmaddwd    m0, m5
    punpckhwd  m1, m3
    pmaddwd    m1, m5

    movu       m4, [r0 + 2 * r1 + x]
    punpcklwd  m2, m3, m4
    pmaddwd    m2, m5
    punpckhwd  m3, m4
    pmaddwd    m3, m5

    lea        r6, [r0 + 2 * r1]
    movu       m5, [r6 + r1 + x]
    punpcklwd  m6, m4, m5
    pmaddwd    m6, [r5 + 1 * mmsize]
    paddd      m0, m6
    punpckhwd  m4, m5
    pmaddwd    m4, [r5 + 1 * mmsize]
    paddd      m1, m4

    movu       m4, [r6 + 2 * r1 + x]
    punpcklwd  m6, m5, m4
    pmaddwd    m6, [r5 + 1 * mmsize]
    paddd      m2, m6
    punpckhwd  m5, m4
    pmaddwd    m5, [r5 + 1 * mmsize]
    paddd      m3, m5

%ifidn %1, ss
    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3
%elifidn %1, ps
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
    psrad     m0, INTERP_SHIFT_PS
    psrad     m1, INTERP_SHIFT_PS
    psrad     m2, INTERP_SHIFT_PS
    psrad     m3, INTERP_SHIFT_PS

    packssdw  m0, m1
    packssdw  m2, m3
%else
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
%ifidn %1, pp
    psrad     m0, INTERP_SHIFT_PP
    psrad     m1, INTERP_SHIFT_PP
    psrad     m2, INTERP_SHIFT_PP
    psrad     m3, INTERP_SHIFT_PP
%else
    psrad     m0, INTERP_SHIFT_SP
    psrad     m1, INTERP_SHIFT_SP
    psrad     m2, INTERP_SHIFT_SP
    psrad     m3, INTERP_SHIFT_SP
%endif
    packssdw  m0, m1
    packssdw  m2, m3
    pxor      m5, m5
    CLIPW2    m0, m2, m5, [pw_pixel_max]
%endif

    movu      [r2 + x], m0
    movu      [r2 + r3 + x], m2
%assign x x+mmsize
%endrep

    lea       r2, [r2 + 2 * r3]
    lea       r0, [r0 + 2 * r1]
    dec       r4d
    jnz       .loopH
    RET
%endmacro

    FILTER_VER_CHROMA_W16_48x64_avx2 pp, 8
    FILTER_VER_CHROMA_W16_48x64_avx2 ps, 8
    FILTER_VER_CHROMA_W16_48x64_avx2 ss, 7
    FILTER_VER_CHROMA_W16_48x64_avx2 sp, 8

INIT_XMM sse2
cglobal chroma_p2s, 3, 7, 3
    ; load width and height
    mov         r3d, r3m
    mov         r4d, r4m
    add         r1, r1

    ; load constant
    mova        m2, [tab_c_n8192]

.loopH:

    xor         r5d, r5d
.loopW:
    lea         r6, [r0 + r5 * 2]

    movu        m0, [r6]
    psllw       m0, (14 - BIT_DEPTH)
    paddw       m0, m2

    movu        m1, [r6 + r1]
    psllw       m1, (14 - BIT_DEPTH)
    paddw       m1, m2

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

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal filterPixelToShort_48x64, 3, 7, 4
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load height
    mov        r6d, 16

    ; load constant
    mova       m3, [pw_2000]

.loop:
    movu       m0, [r0]
    movu       m1, [r0 + 32]
    movu       m2, [r0 + 64]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psubw      m0, m3
    psubw      m1, m3
    psubw      m2, m3
    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 0 + 32], m1
    movu       [r2 + r3 * 0 + 64], m2

    movu       m0, [r0 + r1]
    movu       m1, [r0 + r1 + 32]
    movu       m2, [r0 + r1 + 64]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psubw      m0, m3
    psubw      m1, m3
    psubw      m2, m3
    movu       [r2 + r3 * 1], m0
    movu       [r2 + r3 * 1 + 32], m1
    movu       [r2 + r3 * 1 + 64], m2

    movu       m0, [r0 + r1 * 2]
    movu       m1, [r0 + r1 * 2 + 32]
    movu       m2, [r0 + r1 * 2 + 64]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psubw      m0, m3
    psubw      m1, m3
    psubw      m2, m3
    movu       [r2 + r3 * 2], m0
    movu       [r2 + r3 * 2 + 32], m1
    movu       [r2 + r3 * 2 + 64], m2

    movu       m0, [r0 + r5]
    movu       m1, [r0 + r5 + 32]
    movu       m2, [r0 + r5 + 64]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psubw      m0, m3
    psubw      m1, m3
    psubw      m2, m3
    movu       [r2 + r4], m0
    movu       [r2 + r4 + 32], m1
    movu       [r2 + r4 + 64], m2

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET

   %macro FILTER_VER_CHROMA_AVX2_8xN 2
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%1_8x%2, 4, 9, 15
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    vbroadcasti128  m14, [pd_32]
%elifidn %1, sp
    vbroadcasti128  m14, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m14, [INTERP_OFFSET_PS]
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
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]

    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    paddd           m3, m7
    pmaddwd         m5, [r5]

    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    paddd           m4, m8
    pmaddwd         m6, [r5]

    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    paddd           m5, m9
    pmaddwd         m7, [r5]


    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 1 * mmsize]
    paddd           m6, m10
    pmaddwd         m8, [r5]


    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm11, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddwd         m11, m9, [r5 + 1 * mmsize]
    paddd           m7, m11
    pmaddwd         m9, [r5]

    movu            xm11, [r0 + r4]                 ; m11 = row 11
    punpckhwd       xm12, xm10, xm11
    punpcklwd       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddwd         m12, m10, [r5 + 1 * mmsize]
    paddd           m8, m12
    pmaddwd         m10, [r5]

    lea             r0, [r0 + r1 * 4]
    movu            xm12, [r0]                      ; m12 = row 12
    punpckhwd       xm13, xm11, xm12
    punpcklwd       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddwd         m13, m11, [r5 + 1 * mmsize]
    paddd           m9, m13
    pmaddwd         m11, [r5]

%ifidn %1,ss
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
    psrad           m4, 6
    psrad           m5, 6
%else
    paddd           m0, m14
    paddd           m1, m14
    paddd           m2, m14
    paddd           m3, m14
    paddd           m4, m14
    paddd           m5, m14
%ifidn %1,pp
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
    psrad           m4, 6
    psrad           m5, 6
%elifidn %1, sp
    psrad           m0, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
    psrad           m2, INTERP_SHIFT_SP
    psrad           m3, INTERP_SHIFT_SP
    psrad           m4, INTERP_SHIFT_SP
    psrad           m5, INTERP_SHIFT_SP
%else
    psrad           m0, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
    psrad           m2, INTERP_SHIFT_PS
    psrad           m3, INTERP_SHIFT_PS
    psrad           m4, INTERP_SHIFT_PS
    psrad           m5, INTERP_SHIFT_PS
%endif
%endif

    packssdw        m0, m1
    packssdw        m2, m3
    packssdw        m4, m5
    vpermq          m0, m0, q3120
    vpermq          m2, m2, q3120
    vpermq          m4, m4, q3120
    pxor            m5, m5
    mova            m3, [pw_pixel_max]
%ifidn %1,pp
    CLIPW           m0, m5, m3
    CLIPW           m2, m5, m3
    CLIPW           m4, m5, m3
%elifidn %1, sp
    CLIPW           m0, m5, m3
    CLIPW           m2, m5, m3
    CLIPW           m4, m5, m3
%endif

    vextracti128    xm1, m0, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    vextracti128    xm1, m2, 1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm1
    lea             r2, [r2 + r3 * 4]
    vextracti128    xm1, m4, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm1

    movu            xm13, [r0 + r1]                 ; m13 = row 13
    punpckhwd       xm0, xm12, xm13
    punpcklwd       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddwd         m0, m12, [r5 + 1 * mmsize]
    paddd           m10, m0
    pmaddwd         m12, [r5]

    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhwd       xm1, xm13, xm0
    punpcklwd       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddwd         m1, m13, [r5 + 1 * mmsize]
    paddd           m11, m1
    pmaddwd         m13, [r5]

%ifidn %1,ss
    psrad           m6, 6
    psrad           m7, 6
%else
    paddd           m6, m14
    paddd           m7, m14
%ifidn %1,pp
    psrad           m6, 6
    psrad           m7, 6
%elifidn %1, sp
    psrad           m6, INTERP_SHIFT_SP
    psrad           m7, INTERP_SHIFT_SP
%else
    psrad           m6, INTERP_SHIFT_PS
    psrad           m7, INTERP_SHIFT_PS
%endif
%endif

    packssdw        m6, m7
    vpermq          m6, m6, q3120
%ifidn %1,pp
    CLIPW           m6, m5, m3
%elifidn %1, sp
    CLIPW           m6, m5, m3
%endif
    vextracti128    xm7, m6, 1
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r6], xm7

    movu            xm1, [r0 + r4]                  ; m1 = row 15
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m2, m0, [r5 + 1 * mmsize]
    paddd           m12, m2
    pmaddwd         m0, [r5]

    lea             r0, [r0 + r1 * 4]
    movu            xm2, [r0]                       ; m2 = row 16
    punpckhwd       xm6, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm6, 1
    pmaddwd         m6, m1, [r5 + 1 * mmsize]
    paddd           m13, m6
    pmaddwd         m1, [r5]

    movu            xm6, [r0 + r1]                  ; m6 = row 17
    punpckhwd       xm4, xm2, xm6
    punpcklwd       xm2, xm6
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m0, m2

    movu            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpckhwd       xm2, xm6, xm4
    punpcklwd       xm6, xm4
    vinserti128     m6, m6, xm2, 1
    pmaddwd         m6, [r5 + 1 * mmsize]
    paddd           m1, m6

%ifidn %1,ss
    psrad           m8, 6
    psrad           m9, 6
    psrad           m10, 6
    psrad           m11, 6
    psrad           m12, 6
    psrad           m13, 6
    psrad           m0, 6
    psrad           m1, 6
%else
    paddd           m8, m14
    paddd           m9, m14
    paddd           m10, m14
    paddd           m11, m14
    paddd           m12, m14
    paddd           m13, m14
    paddd           m0, m14
    paddd           m1, m14
%ifidn %1,pp
    psrad           m8, 6
    psrad           m9, 6
    psrad           m10, 6
    psrad           m11, 6
    psrad           m12, 6
    psrad           m13, 6
    psrad           m0, 6
    psrad           m1, 6
%elifidn %1, sp
    psrad           m8, INTERP_SHIFT_SP
    psrad           m9, INTERP_SHIFT_SP
    psrad           m10, INTERP_SHIFT_SP
    psrad           m11, INTERP_SHIFT_SP
    psrad           m12, INTERP_SHIFT_SP
    psrad           m13, INTERP_SHIFT_SP
    psrad           m0, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
%else
    psrad           m8, INTERP_SHIFT_PS
    psrad           m9, INTERP_SHIFT_PS
    psrad           m10, INTERP_SHIFT_PS
    psrad           m11, INTERP_SHIFT_PS
    psrad           m12, INTERP_SHIFT_PS
    psrad           m13, INTERP_SHIFT_PS
    psrad           m0, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
%endif
%endif

    packssdw        m8, m9
    packssdw        m10, m11
    packssdw        m12, m13
    packssdw        m0, m1
    vpermq          m8, m8, q3120
    vpermq          m10, m10, q3120
    vpermq          m12, m12, q3120
    vpermq          m0, m0, q3120
%ifidn %1,pp
    CLIPW           m8, m5, m3
    CLIPW           m10, m5, m3
    CLIPW           m12, m5, m3
    CLIPW           m0, m5, m3
%elifidn %1, sp
    CLIPW           m8, m5, m3
    CLIPW           m10, m5, m3
    CLIPW           m12, m5, m3
    CLIPW           m0, m5, m3
%endif
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    vextracti128    xm13, m12, 1
    vextracti128    xm1, m0, 1
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm8
    movu            [r2 + r3], xm9
    movu            [r2 + r3 * 2], xm10
    movu            [r2 + r6], xm11
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm12
    movu            [r2 + r3], xm13
    movu            [r2 + r3 * 2], xm0
    movu            [r2 + r6], xm1
    lea             r2, [r2 + r3 * 4]
    dec             r8d
    jnz             .loopH
    RET
%endif
%endmacro

FILTER_VER_CHROMA_AVX2_8xN pp, 16
FILTER_VER_CHROMA_AVX2_8xN ps, 16
FILTER_VER_CHROMA_AVX2_8xN ss, 16
FILTER_VER_CHROMA_AVX2_8xN sp, 16
FILTER_VER_CHROMA_AVX2_8xN pp, 32
FILTER_VER_CHROMA_AVX2_8xN ps, 32
FILTER_VER_CHROMA_AVX2_8xN sp, 32
FILTER_VER_CHROMA_AVX2_8xN ss, 32
FILTER_VER_CHROMA_AVX2_8xN pp, 64
FILTER_VER_CHROMA_AVX2_8xN ps, 64
FILTER_VER_CHROMA_AVX2_8xN sp, 64
FILTER_VER_CHROMA_AVX2_8xN ss, 64

%macro PROCESS_CHROMA_AVX2_8x2 3
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
    pmaddwd         m2, m2, [r5 + 1 * mmsize]
    paddd           m0, m2

    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m3, m3, [r5 + 1 * mmsize]
    paddd           m1, m3

%ifnidn %1,ss
    paddd           m0, m7
    paddd           m1, m7
%endif
    psrad           m0, %3
    psrad           m1, %3

    packssdw        m0, m1
    vpermq          m0, m0, q3120
    pxor            m4, m4

%if %2
    CLIPW           m0, m4, [pw_pixel_max]
%endif
    vextracti128    xm1, m0, 1
%endmacro


%macro FILTER_VER_CHROMA_AVX2_8x2 3
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x2, 4, 6, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    vbroadcasti128  m7, [pd_32]
%elifidn %1, sp
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
%endif

    PROCESS_CHROMA_AVX2_8x2 %1, %2, %3
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    RET
%endmacro

FILTER_VER_CHROMA_AVX2_8x2 pp, 1, 6
FILTER_VER_CHROMA_AVX2_8x2 ps, 0, INTERP_SHIFT_PS
FILTER_VER_CHROMA_AVX2_8x2 sp, 1, INTERP_SHIFT_SP
FILTER_VER_CHROMA_AVX2_8x2 ss, 0, 6

%macro FILTER_VER_CHROMA_AVX2_4x2 3
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x2, 4, 6, 7
    mov             r4d, r4m
    add             r1d, r1d
    add             r3d, r3d
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1

%ifidn %1,pp
    vbroadcasti128  m6, [pd_32]
%elifidn %1, sp
    vbroadcasti128  m6, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m6, [INTERP_OFFSET_PS]
%endif

    movq            xm0, [r0]                       ; row 0
    movq            xm1, [r0 + r1]                  ; row 1
    punpcklwd       xm0, xm1

    movq            xm2, [r0 + r1 * 2]              ; row 2
    punpcklwd       xm1, xm2
    vinserti128     m0, m0, xm1, 1                  ; m0 = [2 1 1 0]
    pmaddwd         m0, [r5]

    movq            xm3, [r0 + r4]                  ; row 3
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm4, [r0]                       ; row 4
    punpcklwd       xm3, xm4
    vinserti128     m2, m2, xm3, 1                  ; m2 = [4 3 3 2]
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    paddd           m0, m5

%ifnidn %1, ss
    paddd           m0, m6
%endif
    psrad           m0, %3
    packssdw        m0, m0
    pxor            m1, m1

%if %2
    CLIPW           m0, m1, [pw_pixel_max]
%endif

    vextracti128    xm2, m0, 1
    lea             r4, [r3 * 3]
    movq            [r2], xm0
    movq            [r2 + r3], xm2
    RET
%endmacro

FILTER_VER_CHROMA_AVX2_4x2 pp, 1, 6
FILTER_VER_CHROMA_AVX2_4x2 ps, 0, INTERP_SHIFT_PS
FILTER_VER_CHROMA_AVX2_4x2 sp, 1, INTERP_SHIFT_SP
FILTER_VER_CHROMA_AVX2_4x2 ss, 0, 6

%macro FILTER_VER_CHROMA_AVX2_4x4 3
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x4, 4, 6, 7
    mov             r4d, r4m
    add             r1d, r1d
    add             r3d, r3d
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1

%ifidn %1,pp
   vbroadcasti128  m6, [pd_32]
%elifidn %1, sp
   vbroadcasti128  m6, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m6, [INTERP_OFFSET_PS]
%endif
    movq            xm0, [r0]                       ; row 0
    movq            xm1, [r0 + r1]                  ; row 1
    punpcklwd       xm0, xm1

    movq            xm2, [r0 + r1 * 2]              ; row 2
    punpcklwd       xm1, xm2
    vinserti128     m0, m0, xm1, 1                  ; m0 = [2 1 1 0]
    pmaddwd         m0, [r5]

    movq            xm3, [r0 + r4]                  ; row 3
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm4, [r0]                       ; row 4
    punpcklwd       xm3, xm4
    vinserti128     m2, m2, xm3, 1                  ; m2 = [4 3 3 2]
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    pmaddwd         m2, [r5]
    paddd           m0, m5

    movq            xm3, [r0 + r1]                  ; row 5
    punpcklwd       xm4, xm3
    movq            xm1, [r0 + r1 * 2]              ; row 6
    punpcklwd       xm3, xm1
    vinserti128     m4, m4, xm3, 1                  ; m4 = [6 5 5 4]
    pmaddwd         m4, [r5 + 1 * mmsize]
    paddd           m2, m4

%ifnidn %1,ss
    paddd           m0, m6
    paddd           m2, m6
%endif
    psrad           m0, %3
    psrad           m2, %3

    packssdw        m0, m2
    pxor            m1, m1
%if %2
    CLIPW           m0, m1, [pw_pixel_max]
%endif

    vextracti128    xm2, m0, 1
    lea             r4, [r3 * 3]
    movq            [r2], xm0
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r4], xm2
    RET
%endmacro

FILTER_VER_CHROMA_AVX2_4x4 pp, 1, 6
FILTER_VER_CHROMA_AVX2_4x4 ps, 0, INTERP_SHIFT_PS
FILTER_VER_CHROMA_AVX2_4x4 sp, 1, INTERP_SHIFT_SP
FILTER_VER_CHROMA_AVX2_4x4 ss, 0, 6


%macro FILTER_VER_CHROMA_AVX2_4x8 3
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x8, 4, 7, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1

%ifidn %1,pp
    vbroadcasti128  m7, [pd_32]
%elifidn %1, sp
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
%endif
    lea             r6, [r3 * 3]

    movq            xm0, [r0]                       ; row 0
    movq            xm1, [r0 + r1]                  ; row 1
    punpcklwd       xm0, xm1
    movq            xm2, [r0 + r1 * 2]              ; row 2
    punpcklwd       xm1, xm2
    vinserti128     m0, m0, xm1, 1                  ; m0 = [2 1 1 0]
    pmaddwd         m0, [r5]

    movq            xm3, [r0 + r4]                  ; row 3
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm4, [r0]                       ; row 4
    punpcklwd       xm3, xm4
    vinserti128     m2, m2, xm3, 1                  ; m2 = [4 3 3 2]
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    pmaddwd         m2, [r5]
    paddd           m0, m5

    movq            xm3, [r0 + r1]                  ; row 5
    punpcklwd       xm4, xm3
    movq            xm1, [r0 + r1 * 2]              ; row 6
    punpcklwd       xm3, xm1
    vinserti128     m4, m4, xm3, 1                  ; m4 = [6 5 5 4]
    pmaddwd         m5, m4, [r5 + 1 * mmsize]
    paddd           m2, m5
    pmaddwd         m4, [r5]

    movq            xm3, [r0 + r4]                  ; row 7
    punpcklwd       xm1, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm6, [r0]                       ; row 8
    punpcklwd       xm3, xm6
    vinserti128     m1, m1, xm3, 1                  ; m1 = [8 7 7 6]
    pmaddwd         m5, m1, [r5 + 1 * mmsize]
    paddd           m4, m5
    pmaddwd         m1, [r5]

    movq            xm3, [r0 + r1]                  ; row 9
    punpcklwd       xm6, xm3
    movq            xm5, [r0 + 2 * r1]              ; row 10
    punpcklwd       xm3, xm5
    vinserti128     m6, m6, xm3, 1                  ; m6 = [A 9 9 8]
    pmaddwd         m6, [r5 + 1 * mmsize]
    paddd           m1, m6
%ifnidn %1,ss
    paddd           m0, m7
    paddd           m2, m7
%endif
    psrad           m0, %3
    psrad           m2, %3
    packssdw        m0, m2
    pxor            m6, m6
    mova            m3, [pw_pixel_max]
%if %2
    CLIPW           m0, m6, m3
%endif
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r6], xm2
%ifnidn %1,ss
    paddd           m4, m7
    paddd           m1, m7
%endif
    psrad           m4, %3
    psrad           m1, %3
    packssdw        m4, m1
%if %2
    CLIPW           m4, m6, m3
%endif
    vextracti128    xm1, m4, 1
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm4
    movq            [r2 + r3], xm1
    movhps          [r2 + r3 * 2], xm4
    movhps          [r2 + r6], xm1
    RET
%endmacro

FILTER_VER_CHROMA_AVX2_4x8 pp, 1, 6
FILTER_VER_CHROMA_AVX2_4x8 ps, 0, INTERP_SHIFT_PS
FILTER_VER_CHROMA_AVX2_4x8 sp, 1, INTERP_SHIFT_SP
FILTER_VER_CHROMA_AVX2_4x8 ss, 0 , 6

%macro PROCESS_LUMA_AVX2_W4_16R_4TAP 3
    movq            xm0, [r0]                       ; row 0
    movq            xm1, [r0 + r1]                  ; row 1
    punpcklwd       xm0, xm1
    movq            xm2, [r0 + r1 * 2]              ; row 2
    punpcklwd       xm1, xm2
    vinserti128     m0, m0, xm1, 1                  ; m0 = [2 1 1 0]
    pmaddwd         m0, [r5]
    movq            xm3, [r0 + r4]                  ; row 3
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm4, [r0]                       ; row 4
    punpcklwd       xm3, xm4
    vinserti128     m2, m2, xm3, 1                  ; m2 = [4 3 3 2]
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    pmaddwd         m2, [r5]
    paddd           m0, m5
    movq            xm3, [r0 + r1]                  ; row 5
    punpcklwd       xm4, xm3
    movq            xm1, [r0 + r1 * 2]              ; row 6
    punpcklwd       xm3, xm1
    vinserti128     m4, m4, xm3, 1                  ; m4 = [6 5 5 4]
    pmaddwd         m5, m4, [r5 + 1 * mmsize]
    paddd           m2, m5
    pmaddwd         m4, [r5]
    movq            xm3, [r0 + r4]                  ; row 7
    punpcklwd       xm1, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm6, [r0]                       ; row 8
    punpcklwd       xm3, xm6
    vinserti128     m1, m1, xm3, 1                  ; m1 = [8 7 7 6]
    pmaddwd         m5, m1, [r5 + 1 * mmsize]
    paddd           m4, m5
    pmaddwd         m1, [r5]
    movq            xm3, [r0 + r1]                  ; row 9
    punpcklwd       xm6, xm3
    movq            xm5, [r0 + 2 * r1]              ; row 10
    punpcklwd       xm3, xm5
    vinserti128     m6, m6, xm3, 1                  ; m6 = [10 9 9 8]
    pmaddwd         m3, m6, [r5 + 1 * mmsize]
    paddd           m1, m3
    pmaddwd         m6, [r5]
%ifnidn %1,ss
    paddd           m0, m7
    paddd           m2, m7
%endif
    psrad           m0, %3
    psrad           m2, %3
    packssdw        m0, m2
    pxor            m3, m3
%if %2
    CLIPW           m0, m3, [pw_pixel_max]
%endif
    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r6], xm2
    movq            xm2, [r0 + r4]                  ;row 11
    punpcklwd       xm5, xm2
    lea             r0, [r0 + 4 * r1]
    movq            xm0, [r0]                       ; row 12
    punpcklwd       xm2, xm0
    vinserti128     m5, m5, xm2, 1                  ; m5 = [12 11 11 10]
    pmaddwd         m2, m5, [r5 + 1 * mmsize]
    paddd           m6, m2
    pmaddwd         m5, [r5]
    movq            xm2, [r0 + r1]                  ; row 13
    punpcklwd       xm0, xm2
    movq            xm3, [r0 + 2 * r1]              ; row 14
    punpcklwd       xm2, xm3
    vinserti128     m0, m0, xm2, 1                  ; m0 = [14 13 13 12]
    pmaddwd         m2, m0, [r5 + 1 * mmsize]
    paddd           m5, m2
    pmaddwd         m0, [r5]
%ifnidn %1,ss
    paddd           m4, m7
    paddd           m1, m7
%endif
    psrad           m4, %3
    psrad           m1, %3
    packssdw        m4, m1
    pxor            m2, m2
%if %2
    CLIPW           m4, m2, [pw_pixel_max]
%endif

    vextracti128    xm1, m4, 1
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm4
    movq            [r2 + r3], xm1
    movhps          [r2 + r3 * 2], xm4
    movhps          [r2 + r6], xm1
    movq            xm4, [r0 + r4]                  ; row 15
    punpcklwd       xm3, xm4
    lea             r0, [r0 + 4 * r1]
    movq            xm1, [r0]                       ; row 16
    punpcklwd       xm4, xm1
    vinserti128     m3, m3, xm4, 1                  ; m3 = [16 15 15 14]
    pmaddwd         m4, m3, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m3, [r5]
    movq            xm4, [r0 + r1]                  ; row 17
    punpcklwd       xm1, xm4
    movq            xm2, [r0 + 2 * r1]              ; row 18
    punpcklwd       xm4, xm2
    vinserti128     m1, m1, xm4, 1                  ; m1 = [18 17 17 16]
    pmaddwd         m1, [r5 + 1 * mmsize]
    paddd           m3, m1

%ifnidn %1,ss
    paddd           m6, m7
    paddd           m5, m7
%endif
    psrad           m6, %3
    psrad           m5, %3
    packssdw        m6, m5
    pxor            m1, m1
%if %2
    CLIPW           m6, m1, [pw_pixel_max]
%endif
    vextracti128    xm5, m6, 1
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm6
    movq            [r2 + r3], xm5
    movhps          [r2 + r3 * 2], xm6
    movhps          [r2 + r6], xm5
%ifnidn %1,ss
    paddd           m0, m7
    paddd           m3, m7
%endif
    psrad           m0, %3
    psrad           m3, %3
    packssdw        m0, m3
%if %2
    CLIPW           m0, m1, [pw_pixel_max]
%endif
    vextracti128    xm3, m0, 1
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm0
    movq            [r2 + r3], xm3
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r6], xm3
%endmacro

%macro FILTER_VER_CHROMA_AVX2_4xN 4
INIT_YMM avx2
cglobal interp_4tap_vert_%1_4x%2, 4, 8, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
    mov             r7d, %2 / 16
%ifidn %1,pp
    vbroadcasti128  m7, [pd_32]
%elifidn %1, sp
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
%endif
    lea             r6, [r3 * 3]
.loopH:
    PROCESS_LUMA_AVX2_W4_16R_4TAP %1, %3, %4
    lea             r2, [r2 + r3 * 4]
    dec             r7d
    jnz             .loopH
    RET
%endmacro

%if ARCH_X86_64
FILTER_VER_CHROMA_AVX2_4xN pp, 16, 1, 6
FILTER_VER_CHROMA_AVX2_4xN ps, 16, 0, INTERP_SHIFT_PS
FILTER_VER_CHROMA_AVX2_4xN sp, 16, 1, INTERP_SHIFT_SP
FILTER_VER_CHROMA_AVX2_4xN ss, 16, 0, 6
FILTER_VER_CHROMA_AVX2_4xN pp, 32, 1, 6
FILTER_VER_CHROMA_AVX2_4xN ps, 32, 0, INTERP_SHIFT_PS
FILTER_VER_CHROMA_AVX2_4xN sp, 32, 1, INTERP_SHIFT_SP
FILTER_VER_CHROMA_AVX2_4xN ss, 32, 0, 6
%endif

%macro FILTER_VER_CHROMA_AVX2_8x8 3
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%1_8x8, 4, 6, 12
    mov             r4d, r4m
    add             r1d, r1d
    add             r3d, r3d
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1

%ifidn %1,pp
    vbroadcasti128  m11, [pd_32]
%elifidn %1, sp
    vbroadcasti128  m11, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m11, [INTERP_OFFSET_PS]
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
    paddd           m0, m4                          ; res row0 done(0,1,2,3)
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    pmaddwd         m3, [r5]
    paddd           m1, m5                          ;res row1 done(1, 2, 3, 4)
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    pmaddwd         m4, [r5]
    paddd           m2, m6                          ;res row2 done(2,3,4,5)
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m7                          ;res row3 done(3,4,5,6)
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    pmaddwd         m6, [r5]
    paddd           m4, m8                          ;res row4 done(4,5,6,7)
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    pmaddwd         m7, [r5]
    paddd           m5, m9                          ;res row5 done(5,6,7,8)
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m8, [r5 + 1 * mmsize]
    paddd           m6, m8                          ;res row6 done(6,7,8,9)
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm8, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm8, 1
    pmaddwd         m9, [r5 + 1 * mmsize]
    paddd           m7, m9                          ;res row7 done 7,8,9,10
    lea             r4, [r3 * 3]
%ifnidn %1,ss
    paddd           m0, m11
    paddd           m1, m11
    paddd           m2, m11
    paddd           m3, m11
%endif
    psrad           m0, %3
    psrad           m1, %3
    psrad           m2, %3
    psrad           m3, %3
    packssdw        m0, m1
    packssdw        m2, m3
    vpermq          m0, m0, q3120
    vpermq          m2, m2, q3120
    pxor            m1, m1
    mova            m3, [pw_pixel_max]
%if %2
    CLIPW           m0, m1, m3
    CLIPW           m2, m1, m3
%endif
    vextracti128    xm9, m0, 1
    vextracti128    xm8, m2, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm9
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r4], xm8
%ifnidn %1,ss
    paddd           m4, m11
    paddd           m5, m11
    paddd           m6, m11
    paddd           m7, m11
%endif
    psrad           m4, %3
    psrad           m5, %3
    psrad           m6, %3
    psrad           m7, %3
    packssdw        m4, m5
    packssdw        m6, m7
    vpermq          m4, m4, q3120
    vpermq          m6, m6, q3120
%if %2
    CLIPW           m4, m1, m3
    CLIPW           m6, m1, m3
%endif
    vextracti128    xm5, m4, 1
    vextracti128    xm7, m6, 1
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm4
    movu            [r2 + r3], xm5
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r4], xm7
    RET
%endif
%endmacro

FILTER_VER_CHROMA_AVX2_8x8 pp, 1, 6
FILTER_VER_CHROMA_AVX2_8x8 ps, 0, INTERP_SHIFT_PS
FILTER_VER_CHROMA_AVX2_8x8 sp, 1, INTERP_SHIFT_SP
FILTER_VER_CHROMA_AVX2_8x8 ss, 0, 6

%macro FILTER_VER_CHROMA_AVX2_8x6 3
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%1_8x6, 4, 6, 12
    mov             r4d, r4m
    add             r1d, r1d
    add             r3d, r3d
    shl             r4d, 6

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1

%ifidn %1,pp
    vbroadcasti128  m11, [pd_32]
%elifidn %1, sp
    vbroadcasti128  m11, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m11, [INTERP_OFFSET_PS]
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
    paddd           m0, m4                          ; r0 done(0,1,2,3)
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    pmaddwd         m3, [r5]
    paddd           m1, m5                          ;r1 done(1, 2, 3, 4)
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    pmaddwd         m4, [r5]
    paddd           m2, m6                          ;r2 done(2,3,4,5)
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m7                          ;r3 done(3,4,5,6)
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    paddd           m4, m8                          ;r4 done(4,5,6,7)
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m7, m7, [r5 + 1 * mmsize]
    paddd           m5, m7                          ;r5 done(5,6,7,8)
    lea             r4, [r3 * 3]
%ifnidn %1,ss
    paddd           m0, m11
    paddd           m1, m11
    paddd           m2, m11
    paddd           m3, m11
%endif
    psrad           m0, %3
    psrad           m1, %3
    psrad           m2, %3
    psrad           m3, %3
    packssdw        m0, m1
    packssdw        m2, m3
    vpermq          m0, m0, q3120
    vpermq          m2, m2, q3120
    pxor            m10, m10
    mova            m9, [pw_pixel_max]
%if %2
    CLIPW           m0, m10, m9
    CLIPW           m2, m10, m9
%endif
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r4], xm3
%ifnidn %1,ss
    paddd           m4, m11
    paddd           m5, m11
%endif
    psrad           m4, %3
    psrad           m5, %3
    packssdw        m4, m5
    vpermq          m4, m4, 11011000b
%if %2
    CLIPW           m4, m10, m9
%endif
    vextracti128    xm5, m4, 1
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm4
    movu            [r2 + r3], xm5
    RET
%endif
%endmacro

FILTER_VER_CHROMA_AVX2_8x6 pp, 1, 6
FILTER_VER_CHROMA_AVX2_8x6 ps, 0, INTERP_SHIFT_PS
FILTER_VER_CHROMA_AVX2_8x6 sp, 1, INTERP_SHIFT_SP
FILTER_VER_CHROMA_AVX2_8x6 ss, 0, 6

%macro PROCESS_CHROMA_AVX2 3
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
%ifnidn %1,ss
    paddd           m0, m7
    paddd           m1, m7
    paddd           m2, m7
    paddd           m3, m7
%endif
    psrad           m0, %3
    psrad           m1, %3
    psrad           m2, %3
    psrad           m3, %3
    packssdw        m0, m1
    packssdw        m2, m3
    vpermq          m0, m0, q3120
    vpermq          m2, m2, q3120
    pxor            m4, m4
%if %2
    CLIPW           m0, m4, [pw_pixel_max]
    CLIPW           m2, m4, [pw_pixel_max]
%endif
    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
%endmacro


%macro FILTER_VER_CHROMA_AVX2_8x4 3
INIT_YMM avx2
cglobal interp_4tap_vert_%1_8x4, 4, 6, 8
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    add             r3d, r3d
%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer + r4]
%endif
    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    vbroadcasti128  m7, [pd_32]
%elifidn %1, sp
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
%endif
    PROCESS_CHROMA_AVX2 %1, %2, %3
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    lea             r4, [r3 * 3]
    movu            [r2 + r4], xm3
    RET
%endmacro

FILTER_VER_CHROMA_AVX2_8x4 pp, 1, 6
FILTER_VER_CHROMA_AVX2_8x4 ps, 0, INTERP_SHIFT_PS
FILTER_VER_CHROMA_AVX2_8x4 sp, 1, INTERP_SHIFT_SP
FILTER_VER_CHROMA_AVX2_8x4 ss, 0, 6

%macro FILTER_VER_CHROMA_AVX2_8x12 3
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_4tap_vert_%1_8x12, 4, 7, 15
    mov             r4d, r4m
    shl             r4d, 6
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_ChromaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_ChromaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r1
%ifidn %1,pp
    vbroadcasti128  m14, [pd_32]
%elifidn %1, sp
    vbroadcasti128  m14, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m14, [INTERP_OFFSET_PS]
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
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    paddd           m3, m7
    pmaddwd         m5, [r5]
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    paddd           m4, m8
    pmaddwd         m6, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    paddd           m5, m9
    pmaddwd         m7, [r5]
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 1 * mmsize]
    paddd           m6, m10
    pmaddwd         m8, [r5]
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm11, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddwd         m11, m9, [r5 + 1 * mmsize]
    paddd           m7, m11
    pmaddwd         m9, [r5]
    movu            xm11, [r0 + r4]                 ; m11 = row 11
    punpckhwd       xm12, xm10, xm11
    punpcklwd       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddwd         m12, m10, [r5 + 1 * mmsize]
    paddd           m8, m12
    pmaddwd         m10, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm12, [r0]                      ; m12 = row 12
    punpckhwd       xm13, xm11, xm12
    punpcklwd       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddwd         m13, m11, [r5 + 1 * mmsize]
    paddd           m9, m13
    pmaddwd         m11, [r5]
%ifnidn %1,ss
    paddd           m0, m14
    paddd           m1, m14
    paddd           m2, m14
    paddd           m3, m14
    paddd           m4, m14
    paddd           m5, m14
%endif
    psrad           m0, %3
    psrad           m1, %3
    psrad           m2, %3
    psrad           m3, %3
    psrad           m4, %3
    psrad           m5, %3
    packssdw        m0, m1
    packssdw        m2, m3
    packssdw        m4, m5
    vpermq          m0, m0, q3120
    vpermq          m2, m2, q3120
    vpermq          m4, m4, q3120
    pxor            m5, m5
    mova            m3, [pw_pixel_max]
%if %2
    CLIPW           m0, m5, m3
    CLIPW           m2, m5, m3
    CLIPW           m4, m5, m3
%endif
    vextracti128    xm1, m0, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    vextracti128    xm1, m2, 1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm1
    lea             r2, [r2 + r3 * 4]
    vextracti128    xm1, m4, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm1
    movu            xm13, [r0 + r1]                 ; m13 = row 13
    punpckhwd       xm0, xm12, xm13
    punpcklwd       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddwd         m12, m12, [r5 + 1 * mmsize]
    paddd           m10, m12
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhwd       xm1, xm13, xm0
    punpcklwd       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddwd         m13, m13, [r5 + 1 * mmsize]
    paddd           m11, m13
%ifnidn %1,ss
    paddd           m6, m14
    paddd           m7, m14
    paddd           m8, m14
    paddd           m9, m14
    paddd           m10, m14
    paddd           m11, m14
%endif
    psrad           m6, %3
    psrad           m7, %3
    psrad           m8, %3
    psrad           m9, %3
    psrad           m10, %3
    psrad           m11, %3
    packssdw        m6, m7
    packssdw        m8, m9
    packssdw        m10, m11
    vpermq          m6, m6, q3120
    vpermq          m8, m8, q3120
    vpermq          m10, m10, q3120
%if %2
    CLIPW           m6, m5, m3
    CLIPW           m8, m5, m3
    CLIPW           m10, m5, m3
%endif
    vextracti128    xm7, m6, 1
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r6], xm7
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm8
    movu            [r2 + r3], xm9
    movu            [r2 + r3 * 2], xm10
    movu            [r2 + r6], xm11
    RET
%endif
%endmacro

FILTER_VER_CHROMA_AVX2_8x12 pp, 1, 6
FILTER_VER_CHROMA_AVX2_8x12 ps, 0, INTERP_SHIFT_PS
FILTER_VER_CHROMA_AVX2_8x12 sp, 1, INTERP_SHIFT_SP
FILTER_VER_CHROMA_AVX2_8x12 ss, 0, 6