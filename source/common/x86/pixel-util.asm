;*****************************************************************************
;* Copyright (C) 2013 x265 project
;*
;* Authors: Min Chen <chenm003@163.com> <min.chen@multicorewareinc.com>
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

SECTION .text


;-----------------------------------------------------------------------------
; void cvt32to16_shr(short *dst, int *src, intptr_t stride, int shift, int size)
;-----------------------------------------------------------------------------
INIT_XMM sse2
cglobal cvt32to16_shr, 5, 7, 1, dst, src, stride
%define rnd     m7
%define shift   m6

    ; make shift
    mov         r5d, r3m
    movd        shift, r5d

    ; make round
    dec         r5
    xor         r6, r6
    bts         r6, r5
    
    movd        rnd, r6d
    pshufd      rnd, rnd, 0

    ; register alloc
    ; r0 - dst
    ; r1 - src
    ; r2 - stride * 2 (short*)
    ; r3 - lx
    ; r4 - size
    ; r5 - ly
    ; r6 - diff
    lea         r2, [r2 * 2]

    mov         r4d, r4m
    mov         r5, r4
    mov         r6, r2
    sub         r6, r4
    lea         r6, [r6 * 2]

    shr         r5, 1
.loop_row:

    mov         r3, r4
    shr         r3, 2
.loop_col:
    ; row 0
    movu        m0, [r1]
    paddd       m0, rnd
    psrad       m0, shift
    packssdw    m0, m0
    movh        [r0], m0

    ; row 1
    movu        m0, [r1 + r4 * 4]
    paddd       m0, rnd
    psrad       m0, shift
    packssdw    m0, m0
    movh        [r0 + r2], m0

    ; move col pointer
    add         r1, 16
    add         r0, 8

    dec         r3
    jg          .loop_col

    ; update pointer
    lea         r1, [r1 + r4 * 4]
    add         r0, r6

    ; end of loop_row
    dec         r5
    jg         .loop_row
    
    RET


;-----------------------------------------------------------------------------
; void calcrecon(pixel* pred, int16_t* residual, pixel* recon, int16_t* reconqt, pixel *reconipred, int stride, int strideqt, int strideipred)
;-----------------------------------------------------------------------------
INIT_XMM sse2
cglobal calcRecons4
%if ARCH_X86_64 == 1
    DECLARE_REG_TMP 0,1,2,3,4,5,6,7,8
    PROLOGUE 6,9,4
%else
    DECLARE_REG_TMP 0,1,2,3,4,5
    PROLOGUE 6,7,4
    %define t6      r6m
    %define t6d     r6d
    %define t7      r7m
    %define t8d     r6d
%endif

    mov         t6d, r6m
%if ARCH_X86_64 == 0
    add         t6d, t6d
    mov         r6m, t6d
%else
    mov         r5d, r5m
    mov         r7d, r7m
    add         t6d, t6d
%endif

    pxor        m0, m0
    mov         t8d, 4/2
.loop:
    movd        m1, [t0]
    movd        m2, [t0 + t5]
    punpckldq   m1, m2
    punpcklbw   m1, m0
    movh        m2, [t1]
    movh        m3, [t1 + t5 * 2]
    punpcklqdq  m2, m3
    paddw       m1, m2
    packuswb    m1, m1

    ; store recon[] and recipred[]
    movd        [t2], m1
    movd        [t4], m1
    add         t4, t7
    pshufd      m2, m1, 1
    movd        [t2 + t5], m2
    movd        [t4], m2
    add         t4, t7

    ; store recqt[]
    punpcklbw   m1, m0
    movlps      [t3], m1
    add         t3, t6
    movhps      [t3], m1
    add         t3, t6

    lea         t0, [t0 + t5 * 2]
    lea         t1, [t1 + t5 * 4]
    lea         t2, [t2 + t5 * 2]

    dec         t8d
    jnz        .loop
    RET


INIT_XMM sse2
cglobal calcRecons8
%if ARCH_X86_64 == 1
    DECLARE_REG_TMP 0,1,2,3,4,5,6,7,8
    PROLOGUE 6,9,5
%else
    DECLARE_REG_TMP 0,1,2,3,4,5
    PROLOGUE 6,7,5
    %define t6      r6m
    %define t6d     r6d
    %define t7      r7m
    %define t8d     r6d
%endif

    mov         t6d, r6m
%if ARCH_X86_64 == 0
    add         t6d, t6d
    mov         r6m, t6d
%else
    mov         r5d, r5m
    mov         r7d, r7m
    add         t6d, t6d
%endif

    pxor        m0, m0
    mov         t8d, 8/2
.loop:
    movh        m1, [t0]
    movh        m2, [t0 + t5]
    punpcklbw   m1, m0
    punpcklbw   m2, m0
    movu        m3, [t1]
    movu        m4, [t1 + t5 * 2]
    paddw       m1, m3
    paddw       m2, m4
    packuswb    m1, m2

    ; store recon[] and recipred[]
    movlps      [t2], m1
    movhps      [t2 + t5], m1
    movlps      [t4], m1
%if ARCH_X86_64 == 0
    add         t4, t7
    movhps      [t4], m1
    add         t4, t7
%else
    movhps      [t4 + t7], m1
    lea         t4, [t4 + t7 * 2]
%endif

    ; store recqt[]
    punpcklbw   m2, m1, m0
    punpckhbw   m1, m0
    movu        [t3], m2
    add         t3, t6
    movu        [t3], m1
    add         t3, t6

    lea         t0, [t0 + t5 * 2]
    lea         t1, [t1 + t5 * 4]
    lea         t2, [t2 + t5 * 2]

    dec         t8d
    jnz        .loop
    RET


INIT_XMM sse4
cglobal calcRecons16
%if ARCH_X86_64 == 1
    DECLARE_REG_TMP 0,1,2,3,4,5,6,7,8
    PROLOGUE 6,9,3
%else
    DECLARE_REG_TMP 0,1,2,3,4,5
    PROLOGUE 6,7,3
    %define t6      r6m
    %define t6d     r6d
    %define t7      r7m
    %define t8d     r6d
%endif

    mov         t6d, r6m
%if ARCH_X86_64 == 0
    add         t6d, t6d
    mov         r6m, t6d
%else
    mov         r5d, r5m
    mov         r7d, r7m
    add         t6d, t6d
%endif

    pxor        m0, m0
    mov         t8d, 16
.loop:
    movu        m2, [t0]
    pmovzxbw    m1, m2
    punpckhbw   m2, m0
    paddw       m1, [t1]
    paddw       m2, [t1 + 16]
    packuswb    m1, m2

    ; store recon[] and recipred[]
    movu        [t2], m1
    movu        [t4], m1

    ; store recqt[]
    pmovzxbw    m2, m1
    punpckhbw   m1, m0
    movu        [t3], m2
    movu        [t3 + 16], m1

    add         t3, t6
    add         t4, t7
    add         t0, t5
    lea         t1, [t1 + t5 * 2]
    add         t2, t5

    dec         t8d
    jnz        .loop
    RET


INIT_XMM sse4
cglobal calcRecons32
%if ARCH_X86_64 == 1
    DECLARE_REG_TMP 0,1,2,3,4,5,6,7,8
    PROLOGUE 6,9,5
%else
    DECLARE_REG_TMP 0,1,2,3,4,5
    PROLOGUE 6,7,5
    %define t6      r6m
    %define t6d     r6d
    %define t7      r7m
    %define t8d     r6d
%endif

    mov         t6d, r6m
%if ARCH_X86_64 == 0
    add         t6d, t6d
    mov         r6m, t6d
%else
    mov         r5d, r5m
    mov         r7d, r7m
    add         t6d, t6d
%endif

    pxor        m0, m0
    mov         t8d, 32
.loop:
    movu        m2, [t0]
    movu        m4, [t0 + 16]
    pmovzxbw    m1, m2
    punpckhbw   m2, m0
    pmovzxbw    m3, m4
    punpckhbw   m4, m0

    paddw       m1, [t1 + 0 * 16]
    paddw       m2, [t1 + 1 * 16]
    packuswb    m1, m2

    paddw       m3, [t1 + 2 * 16]
    paddw       m4, [t1 + 3 * 16]
    packuswb    m3, m4

    ; store recon[] and recipred[]
    movu        [t2], m1
    movu        [t2 + 16], m3
    movu        [t4], m1
    movu        [t4 + 16], m3

    ; store recqt[]
    pmovzxbw    m2, m1
    punpckhbw   m1, m0
    movu        [t3 + 0 * 16], m2
    movu        [t3 + 1 * 16], m1
    pmovzxbw    m4, m3
    punpckhbw   m3, m0
    movu        [t3 + 2 * 16], m4
    movu        [t3 + 3 * 16], m3

    add         t3, t6
    add         t4, t7
    add         t0, t5
    lea         t1, [t1 + t5 * 2]
    add         t2, t5

    dec         t8d
    jnz        .loop
    RET


;-----------------------------------------------------------------------------
; void getResidual(pixel *fenc, pixel *pred, int16_t *residual, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_XMM sse2
cglobal getResidual4, 4,4,5
    pxor        m0, m0

    ; row 0-1
    movd        m1, [r0]
    movd        m2, [r0 + r3]
    movd        m3, [r1]
    movd        m4, [r1 + r3]
    punpckldq   m1, m2
    punpcklbw   m1, m0
    punpckldq   m3, m4
    punpcklbw   m3, m0
    psubw       m1, m3
    movlps      [r2], m1
    movhps      [r2 + r3 * 2], m1
    lea         r0, [r0 + r3 * 2]
    lea         r1, [r1 + r3 * 2]
    lea         r2, [r2 + r3 * 4]

    ; row 2-3
    movd        m1, [r0]
    movd        m2, [r0 + r3]
    movd        m3, [r1]
    movd        m4, [r1 + r3]
    punpckldq   m1, m2
    punpcklbw   m1, m0
    punpckldq   m3, m4
    punpcklbw   m3, m0
    psubw       m1, m3
    movlps      [r2], m1
    movhps      [r2 + r3 * 2], m1

    RET


INIT_XMM sse2
cglobal getResidual8, 4,4,5
    pxor        m0, m0

%assign x 0
%rep 8/2
    ; row 0-1
    movh        m1, [r0]
    movh        m2, [r0 + r3]
    movh        m3, [r1]
    movh        m4, [r1 + r3]
    punpcklbw   m1, m0
    punpcklbw   m2, m0
    punpcklbw   m3, m0
    punpcklbw   m4, m0
    psubw       m1, m3
    psubw       m2, m4
    movu        [r2], m1
    movu        [r2 + r3 * 2], m2
%assign x x+1
%if (x != 4)
    lea         r0, [r0 + r3 * 2]
    lea         r1, [r1 + r3 * 2]
    lea         r2, [r2 + r3 * 4]
%endif
%endrep
    RET


INIT_XMM sse4
cglobal getResidual16, 4,5,8
    mov         r4d, 16/4
    pxor        m0, m0
.loop:
    ; row 0-1
    movu        m1, [r0]
    movu        m2, [r0 + r3]
    movu        m3, [r1]
    movu        m4, [r1 + r3]
    pmovzxbw    m5, m1
    punpckhbw   m1, m0
    pmovzxbw    m6, m2
    punpckhbw   m2, m0
    pmovzxbw    m7, m3
    punpckhbw   m3, m0
    psubw       m5, m7
    psubw       m1, m3
    pmovzxbw    m7, m4
    punpckhbw   m4, m0
    psubw       m6, m7
    psubw       m2, m4

    movu        [r2], m5
    movu        [r2 + 16], m1
    movu        [r2 + r3 * 2], m6
    movu        [r2 + r3 * 2 + 16], m2

    lea         r0, [r0 + r3 * 2]
    lea         r1, [r1 + r3 * 2]
    lea         r2, [r2 + r3 * 4]

    ; row 2-3
    movu        m1, [r0]
    movu        m2, [r0 + r3]
    movu        m3, [r1]
    movu        m4, [r1 + r3]
    pmovzxbw    m5, m1
    punpckhbw   m1, m0
    pmovzxbw    m6, m2
    punpckhbw   m2, m0
    pmovzxbw    m7, m3
    punpckhbw   m3, m0
    psubw       m5, m7
    psubw       m1, m3
    pmovzxbw    m7, m4
    punpckhbw   m4, m0
    psubw       m6, m7
    psubw       m2, m4

    movu        [r2], m5
    movu        [r2 + 16], m1
    movu        [r2 + r3 * 2], m6
    movu        [r2 + r3 * 2 + 16], m2

    dec         r4d

    lea         r0, [r0 + r3 * 2]
    lea         r1, [r1 + r3 * 2]
    lea         r2, [r2 + r3 * 4]

    jnz        .loop
    RET


INIT_XMM sse4
cglobal getResidual32, 4,5,7
    mov         r4d, 32/2
    pxor        m0, m0
.loop:
    movu        m1, [r0]
    movu        m2, [r0 + 16]
    movu        m3, [r1]
    movu        m4, [r1 + 16]
    pmovzxbw    m5, m1
    punpckhbw   m1, m0
    pmovzxbw    m6, m3
    punpckhbw   m3, m0
    psubw       m5, m6
    psubw       m1, m3
    movu        [r2 + 0 * 16], m5
    movu        [r2 + 1 * 16], m1

    pmovzxbw    m5, m2
    punpckhbw   m2, m0
    pmovzxbw    m6, m4
    punpckhbw   m4, m0
    psubw       m5, m6
    psubw       m2, m4
    movu        [r2 + 2 * 16], m5
    movu        [r2 + 3 * 16], m2

    movu        m1, [r0 + r3]
    movu        m2, [r0 + r3 + 16]
    movu        m3, [r1 + r3]
    movu        m4, [r1 + r3 + 16]
    pmovzxbw    m5, m1
    punpckhbw   m1, m0
    pmovzxbw    m6, m3
    punpckhbw   m3, m0
    psubw       m5, m6
    psubw       m1, m3
    movu        [r2 + r3 * 2 + 0 * 16], m5
    movu        [r2 + r3 * 2 + 1 * 16], m1

    pmovzxbw    m5, m2
    punpckhbw   m2, m0
    pmovzxbw    m6, m4
    punpckhbw   m4, m0
    psubw       m5, m6
    psubw       m2, m4
    movu        [r2 + r3 * 2 + 2 * 16], m5
    movu        [r2 + r3 * 2 + 3 * 16], m2

    dec         r4d

    lea         r0, [r0 + r3 * 2]
    lea         r1, [r1 + r3 * 2]
    lea         r2, [r2 + r3 * 4]

    jnz        .loop
    RET
