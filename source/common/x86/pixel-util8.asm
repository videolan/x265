;*****************************************************************************
;* Copyright (C) 2013 x265 project
;*
;* Authors: Min Chen <chenm003@163.com> <min.chen@multicorewareinc.com>
;*          Nabajit Deka <nabajit@multicorewareinc.com>
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

c_d_4:             dd 4, 4, 4, 4
c_d_1234:          dd 1, 2, 3, 4
%if BIT_DEPTH == 10
ssim_c1:   times 4 dd 6697.7856    ; .01*.01*1023*1023*64
ssim_c2:   times 4 dd 3797644.4352 ; .03*.03*1023*1023*64*63
pf_64:     times 4 dd 64.0
pf_128:    times 4 dd 128.0
%elif BIT_DEPTH == 9
ssim_c1:   times 4 dd 1671         ; .01*.01*511*511*64
ssim_c2:   times 4 dd 947556       ; .03*.03*511*511*64*63
%else ; 8-bit
ssim_c1:   times 4 dd 416          ; .01*.01*255*255*64
ssim_c2:   times 4 dd 235963       ; .03*.03*255*255*64*63
%endif
mask_ff:   times 16 db 0xff
           times 16 db 0
deinterleave_shuf: db 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15
deinterleave_word_shuf: db 0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 15, 15
hmul_16p:  times 16 db 1
           times 8 db 1, -1
hmulw_16p:  times 8 dw 1
            times 4 dw 1, -1

SECTION .text

cextern pw_1
cextern pw_00ff
cextern pw_2000
cextern pw_pixel_max

;-----------------------------------------------------------------------------
; void calcrecon(pixel* pred, int16_t* residual, pixel* recon, int16_t* reconqt, pixel *reconipred, int stride, int strideqt, int strideipred)
;-----------------------------------------------------------------------------
INIT_XMM sse2
cglobal calcRecons4
%if HIGH_BIT_DEPTH
%if ARCH_X86_64 == 1
    DECLARE_REG_TMP 0,1,2,3,4,5,6,7,8
    PROLOGUE 6,9,6
%else
    DECLARE_REG_TMP 0,1,2,3,4,5
    PROLOGUE 6,7,6
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
    add         t7, t7
%endif

    pxor        m4, m4
    mova        m5, [pw_pixel_max]
    add         t5, t5
    mov         t8d, 4/2
.loop:
    movh        m0, [t0]
    movh        m1, [t0 + t5]
    punpcklqdq  m0, m1
    movh        m2, [t1]
    movh        m3, [t1 + t5]
    punpcklqdq  m2, m3
    paddw       m0, m2
    CLIPW       m0, m4, m5

    ; store recon[] and recipred[]
    movh        [t2], m0
    movh        [t4], m0
%if ARCH_X86_64 == 0
    add         t4, t7
    add         t4, t7
    movhps      [t4], m0
    add         t4, t7
    add         t4, t7
%else
    movhps      [t4 + t7], m0
    lea         t4, [t4 + t7 * 2]
%endif
    movhps      [t2 + t5], m0

    ; store recqt[]
    movh        [t3], m0
    add         t3, t6
    movhps      [t3], m0
    add         t3, t6

    lea         t0, [t0 + t5 * 2]
    lea         t1, [t1 + t5 * 2]
    lea         t2, [t2 + t5 * 2]

    dec         t8d
    jnz        .loop

%else          ;HIGH_BIT_DEPTH
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
%endif          ;HIGH_BIT_DEPTH
    RET


INIT_XMM sse2
cglobal calcRecons8
%if HIGH_BIT_DEPTH
%if ARCH_X86_64 == 1
    DECLARE_REG_TMP 0,1,2,3,4,5,6,7,8
    PROLOGUE 6,9,6
%else
    DECLARE_REG_TMP 0,1,2,3,4,5
    PROLOGUE 6,7,6
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
    add         t7, t7
%endif

    pxor        m4, m4
    mova        m5, [pw_pixel_max]
    add         t5, t5
    mov         t8d, 8/2
.loop:
    movu        m0, [t0]
    movu        m1, [t0 + t5]
    movu        m2, [t1]
    movu        m3, [t1 + t5]
    paddw       m0, m2
    paddw       m1, m3
    CLIPW       m0, m4, m5
    CLIPW       m1, m4, m5

    ; store recon[] and recipred[]
    movu        [t2], m0
    movu        [t2 + t5], m1
    movu        [t4], m0
%if ARCH_X86_64 == 0
    add         t4, t7
    add         t4, t7
    movu        [t4], m1
    add         t4, t7
    add         t4, t7
%else
    movu        [t4 + t7], m1
    lea         t4, [t4 + t7 * 2]
%endif

    ; store recqt[]
    movu        [t3], m0
    add         t3, t6
    movu        [t3], m1
    add         t3, t6

    lea         t0, [t0 + t5 * 2]
    lea         t1, [t1 + t5 * 2]
    lea         t2, [t2 + t5 * 2]

    dec         t8d
    jnz        .loop
%else          ;HIGH_BIT_DEPTH

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
%endif          ;HIGH_BIT_DEPTH
    RET



%if HIGH_BIT_DEPTH
INIT_XMM sse2
cglobal calcRecons16
%if ARCH_X86_64 == 1
    DECLARE_REG_TMP 0,1,2,3,4,5,6,7,8
    PROLOGUE 6,9,6
%else
    DECLARE_REG_TMP 0,1,2,3,4,5
    PROLOGUE 6,7,6
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
    add         t7, t7
%endif

    pxor        m4, m4
    mova        m5, [pw_pixel_max]
    add         t5, t5
    mov         t8d, 16/2
.loop:
    movu        m0, [t0]
    movu        m1, [t0 + 16]
    movu        m2, [t1]
    movu        m3, [t1 + 16]
    paddw       m0, m2
    paddw       m1, m3
    CLIPW       m0, m4, m5
    CLIPW       m1, m4, m5

    ; store recon[] and recipred[]
    movu        [t2], m0
    movu        [t2 + 16], m1
    movu        [t4], m0
    movu        [t4 + 16], m1
%if ARCH_X86_64 == 0
    add         t4, t7
    add         t4, t7
%endif

    ; store recqt[]
    movu        [t3], m0
    movu        [t3 + 16], m1
    add         t3, t6

    movu        m0, [t0 + t5]
    movu        m1, [t0 + t5 + 16]
    movu        m2, [t1 + t5]
    movu        m3, [t1 + t5 + 16]
    paddw       m0, m2
    paddw       m1, m3
    CLIPW       m0, m4, m5
    CLIPW       m1, m4, m5

    ; store recon[] and recipred[]
    movu        [t2 + t5], m0
    movu        [t2 + t5 + 16], m1
%if ARCH_X86_64 == 0
    movu        [t4], m0
    movu        [t4 + 16], m1
    add         t4, t7
    add         t4, t7
%else
    movu        [t4 + t7], m0
    movu        [t4 + t7 + 16], m1
    lea         t4, [t4 + t7 * 2]
%endif

    ; store recqt[]
    movu        [t3], m0
    movu        [t3 + 16], m1
    add         t3, t6

    lea         t0, [t0 + t5 * 2]
    lea         t1, [t1 + t5 * 2]
    lea         t2, [t2 + t5 * 2]

    dec         t8d
    jnz        .loop
%else          ;HIGH_BIT_DEPTH
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
%endif          ;HIGH_BIT_DEPTH
    RET

%if HIGH_BIT_DEPTH
INIT_XMM sse2
cglobal calcRecons32
%if ARCH_X86_64 == 1
    DECLARE_REG_TMP 0,1,2,3,4,5,6,7,8
    PROLOGUE 6,9,6
%else
    DECLARE_REG_TMP 0,1,2,3,4,5
    PROLOGUE 6,7,6
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
    add         t7, t7
%endif

    pxor        m4, m4
    mova        m5, [pw_pixel_max]
    add         t5, t5
    mov         t8d, 32/2
.loop:

    movu        m0, [t0]
    movu        m1, [t0 + 16]
    movu        m2, [t1]
    movu        m3, [t1 + 16]
    paddw       m0, m2
    paddw       m1, m3
    CLIPW       m0, m4, m5
    CLIPW       m1, m4, m5

    ; store recon[] and recipred[]
    movu        [t2], m0
    movu        [t2 + 16], m1
    movu        [t4], m0
    movu        [t4 + 16], m1

    ; store recqt[]
    movu        [t3], m0
    movu        [t3 + 16], m1

    movu        m0, [t0 + 32]
    movu        m1, [t0 + 48]
    movu        m2, [t1 + 32]
    movu        m3, [t1 + 48]
    paddw       m0, m2
    paddw       m1, m3
    CLIPW       m0, m4, m5
    CLIPW       m1, m4, m5

    ; store recon[] and recipred[]
    movu        [t2 + 32], m0
    movu        [t2 + 48], m1
    movu        [t4 + 32], m0
    movu        [t4 + 48], m1
%if ARCH_X86_64 == 0
    add         t4, t7
    add         t4, t7
%endif

    ; store recqt[]
    movu        [t3 + 32], m0
    movu        [t3 + 48], m1
    add         t3, t6

    movu        m0, [t0 + t5]
    movu        m1, [t0 + t5 + 16]
    movu        m2, [t1 + t5]
    movu        m3, [t1 + t5 + 16]
    paddw       m0, m2
    paddw       m1, m3
    CLIPW       m0, m4, m5
    CLIPW       m1, m4, m5

    ; store recon[] and recipred[]
    movu        [t2 + t5], m0
    movu        [t2 + t5 + 16], m1
%if ARCH_X86_64 == 0
    movu        [t4], m0
    movu        [t4 + 16], m1
%else
    movu        [t4 + t7], m0
    movu        [t4 + t7 + 16], m1
%endif

    ; store recqt[]
    movu        [t3], m0
    movu        [t3 + 16], m1

    movu        m0, [t0 + t5 + 32]
    movu        m1, [t0 + t5 + 48]
    movu        m2, [t1 + t5 + 32]
    movu        m3, [t1 + t5 + 48]
    paddw       m0, m2
    paddw       m1, m3
    CLIPW       m0, m4, m5
    CLIPW       m1, m4, m5

    ; store recon[] and recipred[]
    movu        [t2 + t5 + 32], m0
    movu        [t2 + t5 + 48], m1
%if ARCH_X86_64 == 0
    movu        [t4 + 32], m0
    movu        [t4 + 48], m1
    add         t4, t7
    add         t4, t7
%else
    movu        [t4 + t7 + 32], m0
    movu        [t4 + t7 + 48], m1
    lea         t4, [t4 + t7 * 2]
%endif

    ; store recqt[]
    movu        [t3 + 32], m0
    movu        [t3 + 48], m1
    add         t3, t6

    lea         t0, [t0 + t5 * 2]
    lea         t1, [t1 + t5 * 2]
    lea         t2, [t2 + t5 * 2]

    dec         t8d
    jnz        .loop
%else          ;HIGH_BIT_DEPTH
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
%endif          ;HIGH_BIT_DEPTH
    RET


;-----------------------------------------------------------------------------
; void getResidual(pixel *fenc, pixel *pred, int16_t *residual, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_XMM sse2
%if HIGH_BIT_DEPTH
cglobal getResidual4, 4,4,4
    add      r3,    r3

    ; row 0-1
    movh         m0, [r0]
    movh         m1, [r0 + r3]
    movh         m2, [r1]
    movh         m3, [r1 + r3]
    punpcklqdq   m0, m1
    punpcklqdq   m2, m3
    psubw        m0, m2

    movh         [r2], m0
    movhps       [r2 + r3], m0
    lea          r0, [r0 + r3 * 2]
    lea          r1, [r1 + r3 * 2]
    lea          r2, [r2 + r3 * 2]

    ; row 2-3
    movh         m0, [r0]
    movh         m1, [r0 + r3]
    movh         m2, [r1]
    movh         m3, [r1 + r3]
    punpcklqdq   m0, m1
    punpcklqdq   m2, m3
    psubw        m0, m2

    movh        [r2], m0
    movhps      [r2 + r3], m0
%else
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
%endif
    RET


INIT_XMM sse2
%if HIGH_BIT_DEPTH
cglobal getResidual8, 4,4,4
    add      r3,    r3

%assign x 0
%rep 8/2
    ; row 0-1
    movu        m1, [r0]
    movu        m2, [r0 + r3]
    movu        m3, [r1]
    movu        m4, [r1 + r3]
    psubw       m1, m3
    psubw       m2, m4
    movu        [r2], m1
    movu        [r2 + r3], m2
%assign x x+1
%if (x != 4)
    lea         r0, [r0 + r3 * 2]
    lea         r1, [r1 + r3 * 2]
    lea         r2, [r2 + r3 * 2]
%endif
%endrep
%else
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
%endif
    RET

%if HIGH_BIT_DEPTH
INIT_XMM sse2
cglobal getResidual16, 4,5,6
    add         r3, r3
    mov         r4d, 16/4
.loop:
    ; row 0-1
    movu        m0, [r0]
    movu        m1, [r0 + 16]
    movu        m2, [r0 + r3]
    movu        m3, [r0 + r3 + 16]
    movu        m4, [r1]
    movu        m5, [r1 + 16]
    psubw       m0, m4
    psubw       m1, m5
    movu        m4, [r1 + r3]
    movu        m5, [r1 + r3 + 16]
    psubw       m2, m4
    psubw       m3, m5
    lea         r0, [r0 + r3 * 2]
    lea         r1, [r1 + r3 * 2]

    movu        [r2], m0
    movu        [r2 + 16], m1
    movu        [r2 + r3], m2
    movu        [r2 + r3 + 16], m3
    lea         r2, [r2 + r3 * 2]

    ; row 2-3
    movu        m0, [r0]
    movu        m1, [r0 + 16]
    movu        m2, [r0 + r3]
    movu        m3, [r0 + r3 + 16]
    movu        m4, [r1]
    movu        m5, [r1 + 16]
    psubw       m0, m4
    psubw       m1, m5
    movu        m4, [r1 + r3]
    movu        m5, [r1 + r3 + 16]
    psubw       m2, m4
    psubw       m3, m5

    movu        [r2], m0
    movu        [r2 + 16], m1
    movu        [r2 + r3], m2
    movu        [r2 + r3 + 16], m3

    dec         r4d

    lea         r0, [r0 + r3 * 2]
    lea         r1, [r1 + r3 * 2]
    lea         r2, [r2 + r3 * 2]

    jnz        .loop
%else

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
%endif

    RET

%if HIGH_BIT_DEPTH
INIT_XMM sse2
cglobal getResidual32, 4,5,6
    add         r3, r3
    mov         r4d, 32/2
.loop:
    ; row 0
    movu        m0, [r0]
    movu        m1, [r0 + 16]
    movu        m2, [r0 + 32]
    movu        m3, [r0 + 48]
    movu        m4, [r1]
    movu        m5, [r1 + 16]
    psubw       m0, m4
    psubw       m1, m5
    movu        m4, [r1 + 32]
    movu        m5, [r1 + 48]
    psubw       m2, m4
    psubw       m3, m5

    movu        [r2], m0
    movu        [r2 + 16], m1
    movu        [r2 + 32], m2
    movu        [r2 + 48], m3

    ; row 1
    movu        m0, [r0 + r3]
    movu        m1, [r0 + r3 + 16]
    movu        m2, [r0 + r3 + 32]
    movu        m3, [r0 + r3 + 48]
    movu        m4, [r1 + r3]
    movu        m5, [r1 + r3 + 16]
    psubw       m0, m4
    psubw       m1, m5
    movu        m4, [r1 + r3 + 32]
    movu        m5, [r1 + r3 + 48]
    psubw       m2, m4
    psubw       m3, m5

    movu        [r2 + r3], m0
    movu        [r2 + r3 + 16], m1
    movu        [r2 + r3 + 32], m2
    movu        [r2 + r3 + 48], m3

    dec         r4d

    lea         r0, [r0 + r3 * 2]
    lea         r1, [r1 + r3 * 2]
    lea         r2, [r2 + r3 * 2]

    jnz        .loop

%else
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
%endif
    RET


;-----------------------------------------------------------------------------
; uint32_t quant(int32_t *coef, int32_t *quantCoeff, int32_t *deltaU, int32_t *qCoef, int qBits, int add, int numCoeff, int32_t* lastPos);
;-----------------------------------------------------------------------------
INIT_XMM sse4
%if ARCH_X86_64 == 1
cglobal quant, 5,6,11
  %define addVec    m8
  %define qbits     m9
  %define qbits8    m10
%else
cglobal quant, 5,6,8, 0-(3*mmsize)
  %define addVec    [rsp + 0 * mmsize]
  %define qbits     [rsp + 1 * mmsize]
  %define qbits8    [rsp + 2 * mmsize]
%endif

    ; fill qbits-8
    movd        m0, r4d
    mova        qbits, m0

    ; fill qbits-8
    sub         r4d, 8
    movd        m0, r4d
    mova        qbits8, m0

    ; fill offset
    mov         r4d, r5m
    movd        m0, r4d
    pshufd      m0, m0, 0
    mova        addVec, m0

    mov         r4d, r6m
    shr         r4d, 3
    pxor        m7, m7          ; m7 = acSum4
    mova        m6, [c_d_1234]  ; m6 = last4
    pxor        m5, m5          ; m5 = count
    mova        m4, [c_d_4]     ; m4 = [4 4 4 4]
.loop:
    ; 4 coeff
    movu        m0, [r0]        ; m1 = level
    pxor        m1, m1
    pcmpgtd     m1, m0          ; m2 = sign
    movu        m2, [r1]        ; m3 = qcoeff
    pabsd       m0, m0
    pmulld      m0, m2          ; m1 = tmpLevel1
    paddd       m2, m0, addVec
    psrad       m2, qbits       ; m3 = level1
    paddd       m7, m2
    pslld       m3, m2, qbits
    psubd       m0, m3
    psrad       m0, qbits8      ; m1 = deltaU1
    movu        [r2], m0

    pxor        m0, m0
    pcmpeqd     m0, m2          ; m0 = mask4
    pand        m5, m0
    pandn       m0, m6
    por         m5, m0
    paddd       m6, m4

    pxor        m2, m1
    psubd       m2, m1
    packssdw    m2, m2
    pmovsxwd    m2, m2
    movu        [r3], m2

    ; 4 coeff
    movu        m0, [r0 + 16]   ; m1 = level
    pxor        m1, m1
    pcmpgtd     m1, m0          ; m2 = sign
    movu        m2, [r1 + 16]   ; m3 = qcoeff
    pabsd       m0, m0
    pmulld      m0, m2          ; m1 = tmpLevel1
    paddd       m2, m0, addVec
    psrad       m2, qbits       ; m3 = level1
    paddd       m7, m2
    pslld       m3, m2, qbits
    psubd       m0, m3
    psrad       m0, qbits8      ; m1 = deltaU1
    movu        [r2 + 16], m0

    pxor        m0, m0
    pcmpeqd     m0, m2          ; m0 = mask4
    pand        m5, m0
    pandn       m0, m6
    por         m5, m0
    paddd       m6, m4

    pxor        m2, m1
    psubd       m2, m1
    packssdw    m2, m2
    pmovsxwd    m2, m2
    movu        [r3 + 16], m2

    add         r0, 32
    add         r1, 32
    add         r2, 32
    add         r3, 32

    dec         r4d
    jnz        .loop

    movhlps     m4, m5
    pmaxud      m4, m5
    pshufd      m5, m4, 1
    pmaxud      m4, m5

    mov         r4, r7m
    movd        [r4], m4
    dec         dword [r4]

    phaddd      m7, m7
    phaddd      m7, m7
    movd        eax, m7

    RET


;-----------------------------------------------------------------------------
; void dequant_normal(const int32_t* quantCoef, int32_t* coef, int num, int scale, int shift)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal dequant_normal, 2,5,8
    movd        m1, r3m             ; m1 = word [scale]
    mov         r4d, r4m
    movd        m0, r4d             ; m0 = shift
    xor         r3d, r3d
    dec         r4d
    bts         r3d, r4d
    movd        m2, r3d
    punpcklwd   m1, m2
    pshufd      m1, m1, 0           ; m1 = dword [add scale]
    mova        m2, [pw_1]
    mov         r2d, r2m

    ; m0 = shift
    ; m1 = scale
    ; m2 = word [1]
.loop:
    movu        m3, [r0]
    movu        m4, [r0 + 16]
    packssdw    m3, m4              ; m3 = clipQCoef
    punpckhwd   m4, m3, m2
    punpcklwd   m3, m2
    pmaddwd     m3, m1              ; m3 = dword (clipQCoef * scale + add)
    pmaddwd     m4, m1
    psrad       m3, m0
    psrad       m4, m0
    packssdw    m3, m3              ; OPT_ME: store must be 32 bits
    pmovsxwd    m3, m3
    packssdw    m4, m4
    pmovsxwd    m4, m4
    movu        [r1], m3
    movu        [r1 + 16], m4

    add         r0, 32
    add         r1, 32

    sub         r2d, 8
    jnz        .loop
    RET

;-----------------------------------------------------------------------------------------------------------------------------------------------
;void weight_pp(pixel *src, pixel *dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset)
;-----------------------------------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal weight_pp, 6, 7, 6

    mov         r6d, r6m
    shl         r6d, 6
    movd        m0, r6d         ; m0 = [w0<<6]

    movd        m1, r7m         ; m1 = [round]
    punpcklwd   m0, m1          ; assuming both (w0<<6) and round are using maximum of 16 bits each.
    pshufd      m0, m0, 0       ; m0 = [w0<<6 round]

    movd        m1, r8m

    movd        m2, r9m
    pshufd      m2, m2, 0

    mova        m5, [pw_1]

    sub         r2d, r4d
    sub         r3d, r4d

.loopH
    mov         r6d, r4d
    shr         r6d, 4
.loopW:
    movh        m4, [r0]
    pmovzxbw    m4, m4

    punpcklwd   m3, m4, m5
    pmaddwd     m3, m0
    psrad       m3, m1
    paddd       m3, m2

    punpckhwd   m4, m5
    pmaddwd     m4, m0
    psrad       m4, m1
    paddd       m4, m2

    packssdw    m3, m4
    packuswb    m3, m3

    movh        [r1], m3

    movh        m4, [r0 + 8]
    pmovzxbw    m4, m4

    punpcklwd   m3, m4, m5
    pmaddwd     m3, m0
    psrad       m3, m1
    paddd       m3, m2

    punpckhwd   m4, m5
    pmaddwd     m4, m0
    psrad       m4, m1
    paddd       m4, m2

    packssdw    m3, m4
    packuswb    m3, m3

    movh        [r1 + 8], m3

    add         r0, 16
    add         r1, 16

    dec         r6d
    jnz         .loopW

    lea         r0, [r0 + r2]
    lea         r1, [r1 + r3]

    dec         r5d
    jnz         .loopH

    RET

;-------------------------------------------------------------------------------------------------------------------------------------------------
;void weight_sp(int16_t *src, pixel *dst, intptr_t srcStride, intptr_t dstStride, int width, int height, int w0, int round, int shift, int offset)
;-------------------------------------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
%if ARCH_X86_64
cglobal weight_sp, 6, 7+2, 7
    %define tmp_r0      r7
    %define tmp_r1      r8
%else ; ARCH_X86_64 = 0
cglobal weight_sp, 6, 7, 7, 0-(2*4)
    %define tmp_r0      [(rsp + 0 * 4)]
    %define tmp_r1      [(rsp + 1 * 4)]
%endif ; ARCH_X86_64

    movd        m0, r6m         ; m0 = [w0]

    movd        m1, r7m         ; m1 = [round]
    punpcklwd   m0, m1
    pshufd      m0, m0, 0       ; m0 = [w0 round]

    movd        m1, r8m         ; m1 = [shift]

    movd        m2, r9m
    pshufd      m2, m2, 0       ; m2 =[offset]

    mova        m3, [pw_1]
    mova        m4, [pw_2000]

    add         r2d, r2d

.loopH
    mov         r6d, r4d

    ; save old src and dst
    mov         tmp_r0, r0
    mov         tmp_r1, r1
.loopW:
    movu        m5, [r0]
    paddw       m5, m4

    punpcklwd   m6,m5, m3
    pmaddwd     m6, m0
    psrad       m6, m1
    paddd       m6, m2

    punpckhwd   m5, m3
    pmaddwd     m5, m0
    psrad       m5, m1
    paddd       m5, m2

    packssdw    m6, m5
    packuswb    m6, m6

    sub         r6d, 8
    jl          .width4
    movh        [r1], m6
    je          .nextH
    add         r0, 16
    add         r1, 8

    jmp         .loopW

.width4
    cmp         r6d, -4
    jl          .width2
    movd        [r1], m6
    je          .nextH
    add         r1, 4
    pshufd      m6, m6, 1

.width2
    pextrw      [r1], m6, 0

.nextH
    mov         r0, tmp_r0
    mov         r1, tmp_r1
    lea         r0, [r0 + r2]
    lea         r1, [r1 + r3]

    dec         r5d
    jnz         .loopH

    RET

;-----------------------------------------------------------------
; void transpose_4x4(pixel *dst, pixel *src, intptr_t stride)
;-----------------------------------------------------------------
INIT_XMM sse2
cglobal transpose4, 3, 3, 4, dest, src, stride
%if HIGH_BIT_DEPTH
    add          r2,    r2
    movh         m0,    [r1]
    movh         m1,    [r1 + r2]
    movh         m2,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movh         m3,    [r1 + r2]
    punpcklwd    m0,    m1
    punpcklwd    m2,    m3
    punpckhdq    m1,    m0,    m2
    punpckldq    m0,    m2
    movu         [r0],       m0
    movu         [r0 + 16],  m1
%else
    movd         m0,    [r1]
    movd         m1,    [r1 + r2]
    movd         m2,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movd         m3,    [r1 + r2]

    punpcklbw    m0,    m1
    punpcklbw    m2,    m3
    punpcklwd    m0,    m2
    movu         [r0],    m0
%endif
    RET

;-----------------------------------------------------------------
; void transpose_8x8(pixel *dst, pixel *src, intptr_t stride)
;-----------------------------------------------------------------
INIT_XMM sse2
%if HIGH_BIT_DEPTH
%macro TRANSPOSE_4x4 1
    movh         m0,    [r1]
    movh         m1,    [r1 + r2]
    movh         m2,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movh         m3,    [r1 + r2]
    punpcklwd    m0,    m1
    punpcklwd    m2,    m3
    punpckhdq    m1,    m0,    m2
    punpckldq    m0,    m2
    movlps         [r0],             m0
    movhps         [r0 + %1],        m0
    movlps         [r0 + 2 * %1],    m1
    lea            r0,               [r0 + 2 * %1]
    movhps         [r0 + %1],        m1
%endmacro
cglobal transpose8_internal
    TRANSPOSE_4x4 r5
    lea    r1,    [r1 + 2 * r2]
    lea    r0,    [r3 + 8]
    TRANSPOSE_4x4 r5
    lea    r1,    [r1 + 2 * r2]
    neg    r2
    lea    r1,    [r1 + r2 * 8 + 8]
    neg    r2
    lea    r0,    [r3 + 4 * r5]
    TRANSPOSE_4x4 r5
    lea    r1,    [r1 + 2 * r2]
    lea    r0,    [r3 + 8 + 4 * r5]
    TRANSPOSE_4x4 r5
    ret
cglobal transpose8, 3, 6, 4, dest, src, stride
    add    r2,    r2
    mov    r3,    r0
    mov    r5,    16
    call   transpose8_internal
%else
cglobal transpose8, 3, 5, 8, dest, src, stride
    lea          r3,    [2 * r2]
    lea          r4,    [3 * r2]
    movh         m0,    [r1]
    movh         m1,    [r1 + r2]
    movh         m2,    [r1 + r3]
    movh         m3,    [r1 + r4]
    movh         m4,    [r1 + 4 * r2]
    lea          r1,    [r1 + 4 * r2]
    movh         m5,    [r1 + r2]
    movh         m6,    [r1 + r3]
    movh         m7,    [r1 + r4]

    punpcklbw    m0,    m1
    punpcklbw    m2,    m3
    punpcklbw    m4,    m5
    punpcklbw    m6,    m7

    punpckhwd    m1,    m0,    m2
    punpcklwd    m0,    m2
    punpckhwd    m5,    m4,    m6
    punpcklwd    m4,    m6
    punpckhdq    m2,    m0,    m4
    punpckldq    m0,    m4
    punpckhdq    m3,    m1,    m5
    punpckldq    m1,    m5

    movu         [r0],         m0
    movu         [r0 + 16],    m2
    movu         [r0 + 32],    m1
    movu         [r0 + 48],    m3
%endif
    RET

%macro TRANSPOSE_8x8 1

    movh         m0,    [r1]
    movh         m1,    [r1 + r2]
    movh         m2,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movh         m3,    [r1 + r2]
    movh         m4,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movh         m5,    [r1 + r2]
    movh         m6,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movh         m7,    [r1 + r2]

    punpcklbw    m0,    m1
    punpcklbw    m2,    m3
    punpcklbw    m4,    m5
    punpcklbw    m6,    m7

    punpckhwd    m1,    m0,    m2
    punpcklwd    m0,    m2
    punpckhwd    m5,    m4,    m6
    punpcklwd    m4,    m6
    punpckhdq    m2,    m0,    m4
    punpckldq    m0,    m4
    punpckhdq    m3,    m1,    m5
    punpckldq    m1,    m5

    movlps         [r0],             m0
    movhps         [r0 + %1],        m0
    movlps         [r0 + 2 * %1],    m2
    lea            r0,               [r0 + 2 * %1]
    movhps         [r0 + %1],        m2
    movlps         [r0 + 2 * %1],    m1
    lea            r0,               [r0 + 2 * %1]
    movhps         [r0 + %1],        m1
    movlps         [r0 + 2 * %1],    m3
    lea            r0,               [r0 + 2 * %1]
    movhps         [r0 + %1],        m3

%endmacro


;-----------------------------------------------------------------
; void transpose_16x16(pixel *dst, pixel *src, intptr_t stride)
;-----------------------------------------------------------------
INIT_XMM sse2
%if HIGH_BIT_DEPTH
cglobal transpose16, 3, 7, 4, dest, src, stride
    add    r2,    r2
    mov    r3,    r0
    mov    r4,    r1
    mov    r5,    32
    mov    r6,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r4 + 16]
    lea    r0,    [r6 + 8 * r5]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 8 * r5 + 16]
    mov    r3,    r0
    call   transpose8_internal

%else
cglobal transpose16, 3, 5, 8, dest, src, stride
    mov    r3,    r0
    mov    r4,    r1
    TRANSPOSE_8x8 16
    lea    r1,    [r1 + 2 * r2]
    lea    r0,    [r3 + 8]
    TRANSPOSE_8x8 16
    lea    r1,    [r4 + 8]
    lea    r0,    [r3 + 8 * 16]
    TRANSPOSE_8x8 16
    lea    r1,    [r1 + 2 * r2]
    lea    r0,    [r3 + 8 * 16 + 8]
    TRANSPOSE_8x8 16
%endif
    RET

cglobal transpose16_internal
    TRANSPOSE_8x8 r6
    lea    r1,    [r1 + 2 * r2]
    lea    r0,    [r5 + 8]
    TRANSPOSE_8x8 r6
    lea    r1,    [r1 + 2 * r2]
    neg    r2
    lea    r1,    [r1 + r2 * 8]
    lea    r1,    [r1 + r2 * 8 + 8]
    neg    r2
    lea    r0,    [r5 + 8 * r6]
    TRANSPOSE_8x8 r6
    lea    r1,    [r1 + 2 * r2]
    lea    r0,    [r5 + 8 * r6 + 8]
    TRANSPOSE_8x8 r6
    ret

;-----------------------------------------------------------------
; void transpose_32x32(pixel *dst, pixel *src, intptr_t stride)
;-----------------------------------------------------------------
INIT_XMM sse2
%if HIGH_BIT_DEPTH
cglobal transpose32, 3, 7, 4, dest, src, stride
    add    r2,    r2
    mov    r3,    r0
    mov    r4,    r1
    mov    r5,    64
    mov    r6,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 32]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 48]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r4 + 16]
    lea    r0,    [r6 + 8 * 64]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 8 * 64 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 8 * 64 + 32]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 8 * 64 + 48]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r4 + 32]
    lea    r0,    [r6 + 16 * 64]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16 * 64 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16 * 64 + 32]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16 * 64 + 48]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r4 + 48]
    lea    r0,    [r6 + 24 * 64]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 24 * 64 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 24 * 64 + 32]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 24 * 64 + 48]
    mov    r3,    r0
    call   transpose8_internal
%else
cglobal transpose32, 3, 7, 8, dest, src, stride
    mov    r3,    r0
    mov    r4,    r1
    mov    r5,    r0
    mov    r6,    32
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 16]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r4 + 16]
    lea    r0,    [r3 + 16 * 32]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 16 * 32 + 16]
    mov    r5,    r0
    call   transpose16_internal
%endif
    RET

;-----------------------------------------------------------------
; void transpose_64x64(pixel *dst, pixel *src, intptr_t stride)
;-----------------------------------------------------------------
INIT_XMM sse2
%if HIGH_BIT_DEPTH
cglobal transpose64, 3, 7, 4, dest, src, stride
    add    r2,    r2
    mov    r3,    r0
    mov    r4,    r1
    mov    r5,    128
    mov    r6,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 32]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 48]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 64]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 80]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 96]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 112]
    mov    r3,    r0
    call   transpose8_internal

    lea    r1,    [r4 + 16]
    lea    r0,    [r6 + 8 * 128]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 8 * 128 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 8 * 128 + 32]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 8 * 128 + 48]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 8 * 128 + 64]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 8 * 128 + 80]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 8 * 128 + 96]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 8 * 128 + 112]
    mov    r3,    r0
    call   transpose8_internal

    lea    r1,    [r4 + 32]
    lea    r0,    [r6 + 16 * 128]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16 * 128 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16 * 128 + 32]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16 * 128 + 48]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16 * 128 + 64]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16 * 128 + 80]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16 * 128 + 96]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 16 * 128 + 112]
    mov    r3,    r0
    call   transpose8_internal

    lea    r1,    [r4 + 48]
    lea    r0,    [r6 + 24 * 128]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 24 * 128 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 24 * 128 + 32]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 24 * 128 + 48]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 24 * 128 + 64]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 24 * 128 + 80]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 24 * 128 + 96]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 24 * 128 + 112]
    mov    r3,    r0
    call   transpose8_internal

    lea    r1,    [r4 + 64]
    lea    r0,    [r6 + 32 * 128]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 32 * 128 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 32 * 128 + 32]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 32 * 128 + 48]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 32 * 128 + 64]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 32 * 128 + 80]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 32 * 128 + 96]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 32 * 128 + 112]
    mov    r3,    r0
    call   transpose8_internal

    lea    r1,    [r4 + 80]
    lea    r0,    [r6 + 40 * 128]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 40 * 128 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 40 * 128 + 32]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 40 * 128 + 48]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 40 * 128 + 64]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 40 * 128 + 80]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 40 * 128 + 96]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 40 * 128 + 112]
    mov    r3,    r0
    call   transpose8_internal

    lea    r1,    [r4 + 96]
    lea    r0,    [r6 + 48 * 128]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 48 * 128 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 48 * 128 + 32]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 48 * 128 + 48]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 48 * 128 + 64]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 48 * 128 + 80]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 48 * 128 + 96]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 48 * 128 + 112]
    mov    r3,    r0
    call   transpose8_internal

    lea    r1,    [r4 + 112]
    lea    r0,    [r6 + 56 * 128]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 56 * 128 + 16]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 56 * 128 + 32]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 56 * 128 + 48]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 56 * 128 + 64]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 56 * 128 + 80]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 56 * 128 + 96]
    mov    r3,    r0
    call   transpose8_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r6 + 56 * 128 + 112]
    mov    r3,    r0
    call   transpose8_internal
%else
cglobal transpose64, 3, 7, 8, dest, src, stride
    mov    r3,    r0
    mov    r4,    r1
    mov    r5,    r0
    mov    r6,    64
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 16]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 32]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 48]
    mov    r5,    r0
    call   transpose16_internal

    lea    r1,    [r4 + 16]
    lea    r0,    [r3 + 16 * 64]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 16 * 64 + 16]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 16 * 64 + 32]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 16 * 64 + 48]
    mov    r5,    r0
    call   transpose16_internal

    lea    r1,    [r4 + 32]
    lea    r0,    [r3 + 32 * 64]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 32 * 64 + 16]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 32 * 64 + 32]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 32 * 64 + 48]
    mov    r5,    r0
    call   transpose16_internal

    lea    r1,    [r4 + 48]
    lea    r0,    [r3 + 48 * 64]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 48 * 64 + 16]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 48 * 64 + 32]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 48 * 64 + 48]
    mov    r5,    r0
    call   transpose16_internal
%endif
    RET

;=============================================================================
; SSIM
;=============================================================================

;-----------------------------------------------------------------------------
; void pixel_ssim_4x4x2_core( const uint8_t *pix1, intptr_t stride1,
;                             const uint8_t *pix2, intptr_t stride2, int sums[2][4] )
;-----------------------------------------------------------------------------
%macro SSIM_ITER 1
%if HIGH_BIT_DEPTH
    movdqu    m5, [r0+(%1&1)*r1]
    movdqu    m6, [r2+(%1&1)*r3]
%else
    movq      m5, [r0+(%1&1)*r1]
    movq      m6, [r2+(%1&1)*r3]
    punpcklbw m5, m0
    punpcklbw m6, m0
%endif
%if %1==1
    lea       r0, [r0+r1*2]
    lea       r2, [r2+r3*2]
%endif
%if %1==0
    movdqa    m1, m5
    movdqa    m2, m6
%else
    paddw     m1, m5
    paddw     m2, m6
%endif
    pmaddwd   m7, m5, m6
    pmaddwd   m5, m5
    pmaddwd   m6, m6
    ACCUM  paddd, 3, 5, %1
    ACCUM  paddd, 4, 7, %1
    paddd     m3, m6
%endmacro

%macro SSIM 0
cglobal pixel_ssim_4x4x2_core, 4,4,8
    FIX_STRIDES r1, r3
    pxor      m0, m0
    SSIM_ITER 0
    SSIM_ITER 1
    SSIM_ITER 2
    SSIM_ITER 3
    ; PHADDW m1, m2
    ; PHADDD m3, m4
    movdqa    m7, [pw_1]
    pshufd    m5, m3, q2301
    pmaddwd   m1, m7
    pmaddwd   m2, m7
    pshufd    m6, m4, q2301
    packssdw  m1, m2
    paddd     m3, m5
    pshufd    m1, m1, q3120
    paddd     m4, m6
    pmaddwd   m1, m7
    punpckhdq m5, m3, m4
    punpckldq m3, m4

%if UNIX64
    %define t0 r4
%else
    %define t0 rax
    mov t0, r4mp
%endif

    movq      [t0+ 0], m1
    movq      [t0+ 8], m3
    movhps    [t0+16], m1
    movq      [t0+24], m5
    RET

;-----------------------------------------------------------------------------
; float pixel_ssim_end( int sum0[5][4], int sum1[5][4], int width )
;-----------------------------------------------------------------------------
cglobal pixel_ssim_end4, 3,3,7
    movdqa    m0, [r0+ 0]
    movdqa    m1, [r0+16]
    movdqa    m2, [r0+32]
    movdqa    m3, [r0+48]
    movdqa    m4, [r0+64]
    paddd     m0, [r1+ 0]
    paddd     m1, [r1+16]
    paddd     m2, [r1+32]
    paddd     m3, [r1+48]
    paddd     m4, [r1+64]
    paddd     m0, m1
    paddd     m1, m2
    paddd     m2, m3
    paddd     m3, m4
    movdqa    m5, [ssim_c1]
    movdqa    m6, [ssim_c2]
    TRANSPOSE4x4D  0, 1, 2, 3, 4

;   s1=m0, s2=m1, ss=m2, s12=m3
%if BIT_DEPTH == 10
    cvtdq2ps  m0, m0
    cvtdq2ps  m1, m1
    cvtdq2ps  m2, m2
    cvtdq2ps  m3, m3
    mulps     m2, [pf_64] ; ss*64
    mulps     m3, [pf_128] ; s12*128
    movdqa    m4, m1
    mulps     m4, m0      ; s1*s2
    mulps     m1, m1      ; s2*s2
    mulps     m0, m0      ; s1*s1
    addps     m4, m4      ; s1*s2*2
    addps     m0, m1      ; s1*s1 + s2*s2
    subps     m2, m0      ; vars
    subps     m3, m4      ; covar*2
    addps     m4, m5      ; s1*s2*2 + ssim_c1
    addps     m0, m5      ; s1*s1 + s2*s2 + ssim_c1
    addps     m2, m6      ; vars + ssim_c2
    addps     m3, m6      ; covar*2 + ssim_c2
%else
    pmaddwd   m4, m1, m0  ; s1*s2
    pslld     m1, 16
    por       m0, m1
    pmaddwd   m0, m0  ; s1*s1 + s2*s2
    pslld     m4, 1
    pslld     m3, 7
    pslld     m2, 6
    psubd     m3, m4  ; covar*2
    psubd     m2, m0  ; vars
    paddd     m0, m5
    paddd     m4, m5
    paddd     m3, m6
    paddd     m2, m6
    cvtdq2ps  m0, m0  ; (float)(s1*s1 + s2*s2 + ssim_c1)
    cvtdq2ps  m4, m4  ; (float)(s1*s2*2 + ssim_c1)
    cvtdq2ps  m3, m3  ; (float)(covar*2 + ssim_c2)
    cvtdq2ps  m2, m2  ; (float)(vars + ssim_c2)
%endif
    mulps     m4, m3
    mulps     m0, m2
    divps     m4, m0  ; ssim

    cmp       r2d, 4
    je .skip ; faster only if this is the common case; remove branch if we use ssim on a macroblock level
    neg       r2
%ifdef PIC
    lea       r3, [mask_ff + 16]
    movdqu    m1, [r3 + r2*4]
%else
    movdqu    m1, [mask_ff + r2*4 + 16]
%endif
    pand      m4, m1
.skip:
    movhlps   m0, m4
    addps     m0, m4
    pshuflw   m4, m0, q0032
    addss     m0, m4
%if ARCH_X86_64 == 0
    movd     r0m, m0
    fld     dword r0m
%endif
    RET
%endmacro ; SSIM

INIT_XMM sse2
SSIM
INIT_XMM avx
SSIM

;-----------------------------------------------------------------
; void scale1D_128to64(pixel *dst, pixel *src, intptr_t /*stride*/)
;-----------------------------------------------------------------
INIT_XMM ssse3
cglobal scale1D_128to64, 2, 2, 8, dest, src1, stride
%if HIGH_BIT_DEPTH
    mova        m7,      [deinterleave_word_shuf]

    movu        m0,      [r1]
    palignr     m1,      m0,    2
    movu        m2,      [r1 + 16]
    palignr     m3,      m2,    2
    movu        m4,      [r1 + 32]
    palignr     m5,      m4,    2
    movu        m6,      [r1 + 48]
    pavgw       m0,      m1
    palignr     m1,      m6,    2
    pavgw       m2,      m3
    pavgw       m4,      m5
    pavgw       m6,      m1
    pshufb      m0,      m0,    m7
    pshufb      m2,      m2,    m7
    pshufb      m4,      m4,    m7
    pshufb      m6,      m6,    m7
    punpcklqdq    m0,           m2
    movu          [r0],         m0
    punpcklqdq    m4,           m6
    movu          [r0 + 16],    m4



    movu        m0,      [r1 + 64]
    palignr     m1,      m0,    2
    movu        m2,      [r1 + 80]
    palignr     m3,      m2,    2
    movu        m4,      [r1 + 96]
    palignr     m5,      m4,    2
    movu        m6,      [r1 + 112]
    pavgw       m0,      m1
    palignr     m1,      m6,    2
    pavgw       m2,      m3
    pavgw       m4,      m5
    pavgw       m6,      m1
    pshufb      m0,      m0,    m7
    pshufb      m2,      m2,    m7
    pshufb      m4,      m4,    m7
    pshufb      m6,      m6,    m7
    punpcklqdq    m0,           m2
    movu          [r0 + 32],    m0
    punpcklqdq    m4,           m6
    movu          [r0 + 48],    m4

    movu        m0,      [r1 + 128]
    palignr     m1,      m0,    2
    movu        m2,      [r1 + 144]
    palignr     m3,      m2,    2
    movu        m4,      [r1 + 160]
    palignr     m5,      m4,    2
    movu        m6,      [r1 + 176]
    pavgw       m0,      m1
    palignr     m1,      m6,    2
    pavgw       m2,      m3
    pavgw       m4,      m5
    pavgw       m6,      m1
    pshufb      m0,      m0,    m7
    pshufb      m2,      m2,    m7
    pshufb      m4,      m4,    m7
    pshufb      m6,      m6,    m7

    punpcklqdq    m0,           m2
    movu          [r0 + 64],    m0
    punpcklqdq    m4,           m6
    movu          [r0 + 80],    m4

    movu        m0,      [r1 + 192]
    palignr     m1,      m0,    2
    movu        m2,      [r1 + 208]
    palignr     m3,      m2,    2
    movu        m4,      [r1 + 224]
    palignr     m5,      m4,    2
    movu        m6,      [r1 + 240]
    pavgw       m0,      m1
    palignr     m1,      m6,    2
    pavgw       m2,      m3
    pavgw       m4,      m5
    pavgw       m6,      m1
    pshufb      m0,      m0,    m7
    pshufb      m2,      m2,    m7
    pshufb      m4,      m4,    m7
    pshufb      m6,      m6,    m7

    punpcklqdq    m0,           m2
    movu          [r0 + 96],    m0
    punpcklqdq    m4,           m6
    movu          [r0 + 112],    m4

%else
    mova        m7,      [deinterleave_shuf]

    movu        m0,      [r1]
    palignr     m1,      m0,    1
    movu        m2,      [r1 + 16]
    palignr     m3,      m2,    1
    movu        m4,      [r1 + 32]
    palignr     m5,      m4,    1
    movu        m6,      [r1 + 48]

    pavgb       m0,      m1

    palignr     m1,      m6,    1

    pavgb       m2,      m3
    pavgb       m4,      m5
    pavgb       m6,      m1

    pshufb      m0,      m0,    m7
    pshufb      m2,      m2,    m7
    pshufb      m4,      m4,    m7
    pshufb      m6,      m6,    m7

    punpcklqdq    m0,           m2
    movu          [r0],         m0
    punpcklqdq    m4,           m6
    movu          [r0 + 16],    m4

    movu        m0,      [r1 + 64]
    palignr     m1,      m0,    1
    movu        m2,      [r1 + 80]
    palignr     m3,      m2,    1
    movu        m4,      [r1 + 96]
    palignr     m5,      m4,    1
    movu        m6,      [r1 + 112]

    pavgb       m0,      m1

    palignr     m1,      m6,    1

    pavgb       m2,      m3
    pavgb       m4,      m5
    pavgb       m6,      m1

    pshufb      m0,      m0,    m7
    pshufb      m2,      m2,    m7
    pshufb      m4,      m4,    m7
    pshufb      m6,      m6,    m7

    punpcklqdq    m0,           m2
    movu          [r0 + 32],    m0
    punpcklqdq    m4,           m6
    movu          [r0 + 48],    m4
%endif
RET

;-----------------------------------------------------------------
; void scale2D_64to32(pixel *dst, pixel *src, intptr_t stride)
;-----------------------------------------------------------------
%if HIGH_BIT_DEPTH
INIT_XMM ssse3
cglobal scale2D_64to32, 3, 4, 8, dest, src, stride
    mov       r3d,    32
    mova      m7,    [deinterleave_word_shuf]
    add       r2,    r2
.loop
    movu      m0,    [r1]                  ;i
    psrld     m1,    m0,    16             ;j
    movu      m2,    [r1 + r2]             ;k
    psrld     m3,    m2,    16             ;l
    movu      m4,    m0
    movu      m5,    m2
    pxor      m4,    m1                    ;i^j
    pxor      m5,    m3                    ;k^l
    por       m4,    m5                    ;ij|kl
    pavgw     m0,    m1                    ;s
    pavgw     m2,    m3                    ;t
    movu      m5,    m0
    pavgw     m0,    m2                    ;(s+t+1)/2
    pxor      m5,    m2                    ;s^t
    pand      m4,    m5                    ;(ij|kl)&st
    pand      m4,    [hmulw_16p]
    psubw     m0,    m4                    ;Result
    movu      m1,    [r1 + 16]             ;i
    psrld     m2,    m1,    16             ;j
    movu      m3,    [r1 + r2 + 16]        ;k
    psrld     m4,    m3,    16             ;l
    movu      m5,    m1
    movu      m6,    m3
    pxor      m5,    m2                    ;i^j
    pxor      m6,    m4                    ;k^l
    por       m5,    m6                    ;ij|kl
    pavgw     m1,    m2                    ;s
    pavgw     m3,    m4                    ;t
    movu      m6,    m1
    pavgw     m1,    m3                    ;(s+t+1)/2
    pxor      m6,    m3                    ;s^t
    pand      m5,    m6                    ;(ij|kl)&st
    pand      m5,    [hmulw_16p]
    psubw     m1,    m5                    ;Result
    pshufb    m0,    m7
    pshufb    m1,    m7

    punpcklqdq    m0,       m1
    movu          [r0],     m0

    movu      m0,    [r1 + 32]             ;i
    psrld     m1,    m0,    16             ;j
    movu      m2,    [r1 + r2 + 32]        ;k
    psrld     m3,    m2,    16             ;l
    movu      m4,    m0
    movu      m5,    m2
    pxor      m4,    m1                    ;i^j
    pxor      m5,    m3                    ;k^l
    por       m4,    m5                    ;ij|kl
    pavgw     m0,    m1                    ;s
    pavgw     m2,    m3                    ;t
    movu      m5,    m0
    pavgw     m0,    m2                    ;(s+t+1)/2
    pxor      m5,    m2                    ;s^t
    pand      m4,    m5                    ;(ij|kl)&st
    pand      m4,    [hmulw_16p]
    psubw     m0,    m4                    ;Result
    movu      m1,    [r1 + 48]             ;i
    psrld     m2,    m1,    16             ;j
    movu      m3,    [r1 + r2 + 48]        ;k
    psrld     m4,    m3,    16             ;l
    movu      m5,    m1
    movu      m6,    m3
    pxor      m5,    m2                    ;i^j
    pxor      m6,    m4                    ;k^l
    por       m5,    m6                    ;ij|kl
    pavgw     m1,    m2                    ;s
    pavgw     m3,    m4                    ;t
    movu      m6,    m1
    pavgw     m1,    m3                    ;(s+t+1)/2
    pxor      m6,    m3                    ;s^t
    pand      m5,    m6                    ;(ij|kl)&st
    pand      m5,    [hmulw_16p]
    psubw     m1,    m5                    ;Result
    pshufb    m0,    m7
    pshufb    m1,    m7

    punpcklqdq    m0,           m1
    movu          [r0 + 16],    m0

    movu      m0,    [r1 + 64]             ;i
    psrld     m1,    m0,    16             ;j
    movu      m2,    [r1 + r2 + 64]        ;k
    psrld     m3,    m2,    16             ;l
    movu      m4,    m0
    movu      m5,    m2
    pxor      m4,    m1                    ;i^j
    pxor      m5,    m3                    ;k^l
    por       m4,    m5                    ;ij|kl
    pavgw     m0,    m1                    ;s
    pavgw     m2,    m3                    ;t
    movu      m5,    m0
    pavgw     m0,    m2                    ;(s+t+1)/2
    pxor      m5,    m2                    ;s^t
    pand      m4,    m5                    ;(ij|kl)&st
    pand      m4,    [hmulw_16p]
    psubw     m0,    m4                    ;Result
    movu      m1,    [r1 + 80]             ;i
    psrld     m2,    m1,    16             ;j
    movu      m3,    [r1 + r2 + 80]        ;k
    psrld     m4,    m3,    16             ;l
    movu      m5,    m1
    movu      m6,    m3
    pxor      m5,    m2                    ;i^j
    pxor      m6,    m4                    ;k^l
    por       m5,    m6                    ;ij|kl
    pavgw     m1,    m2                    ;s
    pavgw     m3,    m4                    ;t
    movu      m6,    m1
    pavgw     m1,    m3                    ;(s+t+1)/2
    pxor      m6,    m3                    ;s^t
    pand      m5,    m6                    ;(ij|kl)&st
    pand      m5,    [hmulw_16p]
    psubw     m1,    m5                    ;Result
    pshufb    m0,    m7
    pshufb    m1,    m7

    punpcklqdq    m0,           m1
    movu          [r0 + 32],    m0

    movu      m0,    [r1 + 96]             ;i
    psrld     m1,    m0,    16             ;j
    movu      m2,    [r1 + r2 + 96]        ;k
    psrld     m3,    m2,    16             ;l
    movu      m4,    m0
    movu      m5,    m2
    pxor      m4,    m1                    ;i^j
    pxor      m5,    m3                    ;k^l
    por       m4,    m5                    ;ij|kl
    pavgw     m0,    m1                    ;s
    pavgw     m2,    m3                    ;t
    movu      m5,    m0
    pavgw     m0,    m2                    ;(s+t+1)/2
    pxor      m5,    m2                    ;s^t
    pand      m4,    m5                    ;(ij|kl)&st
    pand      m4,    [hmulw_16p]
    psubw     m0,    m4                    ;Result
    movu      m1,    [r1 + 112]            ;i
    psrld     m2,    m1,    16             ;j
    movu      m3,    [r1 + r2 + 112]       ;k
    psrld     m4,    m3,    16             ;l
    movu      m5,    m1
    movu      m6,    m3
    pxor      m5,    m2                    ;i^j
    pxor      m6,    m4                    ;k^l
    por       m5,    m6                    ;ij|kl
    pavgw     m1,    m2                    ;s
    pavgw     m3,    m4                    ;t
    movu      m6,    m1
    pavgw     m1,    m3                    ;(s+t+1)/2
    pxor      m6,    m3                    ;s^t
    pand      m5,    m6                    ;(ij|kl)&st
    pand      m5,    [hmulw_16p]
    psubw     m1,    m5                    ;Result
    pshufb    m0,    m7
    pshufb    m1,    m7

    punpcklqdq    m0,           m1
    movu          [r0 + 48],    m0
    lea    r0,    [r0 + 64]
    lea    r1,    [r1 + 2 * r2]
    dec    r3d
    jnz    .loop
    RET
%else

INIT_XMM ssse3
cglobal scale2D_64to32, 3, 4, 8, dest, src, stride
    mov       r3d,    32
    mova        m7,      [deinterleave_shuf]
.loop

    movu        m0,      [r1]                  ;i
    psrlw       m1,      m0,    8              ;j
    movu        m2,      [r1 + r2]             ;k
    psrlw       m3,      m2,    8              ;l
    movu        m4,      m0
    movu        m5,      m2

    pxor        m4,      m1                    ;i^j
    pxor        m5,      m3                    ;k^l
    por         m4,      m5                    ;ij|kl

    pavgb       m0,      m1                    ;s
    pavgb       m2,      m3                    ;t
    movu        m5,      m0
    pavgb       m0,      m2                    ;(s+t+1)/2
    pxor        m5,      m2                    ;s^t
    pand        m4,      m5                    ;(ij|kl)&st
    pand        m4,      [hmul_16p]
    psubb       m0,      m4                    ;Result

    movu        m1,      [r1 + 16]             ;i
    psrlw       m2,      m1,    8              ;j
    movu        m3,      [r1 + r2 + 16]        ;k
    psrlw       m4,      m3,    8              ;l
    movu        m5,      m1
    movu        m6,      m3

    pxor        m5,      m2                    ;i^j
    pxor        m6,      m4                    ;k^l
    por         m5,      m6                    ;ij|kl

    pavgb       m1,      m2                    ;s
    pavgb       m3,      m4                    ;t
    movu        m6,      m1
    pavgb       m1,      m3                    ;(s+t+1)/2
    pxor        m6,      m3                    ;s^t
    pand        m5,      m6                    ;(ij|kl)&st
    pand        m5,      [hmul_16p]
    psubb       m1,      m5                    ;Result

    pshufb      m0,      m0,    m7
    pshufb      m1,      m1,    m7

    punpcklqdq    m0,           m1
    movu          [r0],         m0

    movu        m0,      [r1 + 32]             ;i
    psrlw       m1,      m0,    8              ;j
    movu        m2,      [r1 + r2 + 32]        ;k
    psrlw       m3,      m2,    8              ;l
    movu        m4,      m0
    movu        m5,      m2

    pxor        m4,      m1                    ;i^j
    pxor        m5,      m3                    ;k^l
    por         m4,      m5                    ;ij|kl

    pavgb       m0,      m1                    ;s
    pavgb       m2,      m3                    ;t
    movu        m5,      m0
    pavgb       m0,      m2                    ;(s+t+1)/2
    pxor        m5,      m2                    ;s^t
    pand        m4,      m5                    ;(ij|kl)&st
    pand        m4,      [hmul_16p]
    psubb       m0,      m4                    ;Result

    movu        m1,      [r1 + 48]             ;i
    psrlw       m2,      m1,    8              ;j
    movu        m3,      [r1 + r2 + 48]        ;k
    psrlw       m4,      m3,    8              ;l
    movu        m5,      m1
    movu        m6,      m3

    pxor        m5,      m2                    ;i^j
    pxor        m6,      m4                    ;k^l
    por         m5,      m6                    ;ij|kl

    pavgb       m1,      m2                    ;s
    pavgb       m3,      m4                    ;t
    movu        m6,      m1
    pavgb       m1,      m3                    ;(s+t+1)/2
    pxor        m6,      m3                    ;s^t
    pand        m5,      m6                    ;(ij|kl)&st
    pand        m5,      [hmul_16p]
    psubb       m1,      m5                    ;Result

    pshufb      m0,      m0,    m7
    pshufb      m1,      m1,    m7

    punpcklqdq    m0,           m1
    movu          [r0 + 16],    m0

    lea    r0,    [r0 + 32]
    lea    r1,    [r1 + 2 * r2]
    dec    r3d
    jnz    .loop
    RET
%endif


;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_2x4(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%if HIGH_BIT_DEPTH
INIT_XMM sse2
cglobal pixel_sub_ps_2x4, 6, 6, 8, dest, deststride, src0, src1, srcstride0, srcstride1
    add      r1,    r1
    add      r4,    r4
    add      r5,    r5
    movd     m0,    [r2]
    movd     m1,    [r3]
    movd     m2,    [r2 + r4]
    movd     m3,    [r3 + r5]
    movd     m4,    [r2 + 2 * r4]
    movd     m5,    [r3 + 2 * r5]
    lea      r2,    [r2 + 2 * r4]
    lea      r3,    [r3 + 2 * r5]
    movd     m6,    [r2 + r4]
    movd     m7,    [r3 + r5]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    psubw    m6,    m7

    movd     [r0],           m0
    movd     [r0 + r1],      m2
    movd     [r0 + 2 * r1],  m4
    lea      r0,             [r0 + 2 * r1]
    movd     [r0 + r1],      m6
%else
INIT_XMM sse4
%if ARCH_X86_64
    cglobal pixel_sub_ps_2x4, 6, 8, 0

    %define tmp_r1     r1
    DECLARE_REG_TMP    6, 7
%else
    cglobal pixel_sub_ps_2x4, 6, 7, 0, 0-4

    %define tmp_r1     dword [rsp]
    DECLARE_REG_TMP    6, 1
%endif ; ARCH_X86_64

    add    r1,         r1

%if ARCH_X86_64 == 0
    mov    tmp_r1,     r1

%endif

movzx    t0d,      byte [r2]
movzx    t1d,      byte [r3]

sub      t0d,      t1d

mov      [r0],     t0w

movzx    t0d,      byte [r2 + 1]
movzx    t1d,      byte [r3 + 1]

sub      t0d,      t1d

mov      [r0 + 2], t0w

add      r0,       tmp_r1

movzx    t0d,      byte [r2 + r4]
movzx    t1d,      byte [r3 + r5]

sub      t0d,      t1d

mov      [r0],     t0w

movzx    t0d,      byte [r2 + r4 + 1]
movzx    t1d,      byte [r3 + r5 + 1]

sub      t0d,      t1d

mov      [r0 + 2], t0w

add      r0,       tmp_r1

movzx    t0d,      byte [r2 + r4 * 2]
movzx    t1d,      byte [r3 + r5 * 2]

sub      t0d,      t1d

mov      [r0],     t0w

movzx    t0d,      byte [r2 + r4 * 2 + 1]
movzx    t1d,      byte [r3 + r5 * 2 + 1]

sub      t0d,      t1d

mov      [r0 + 2], t0w

add      r0,       tmp_r1

lea      r2,       [r2 + r4 * 2]
lea      r3,       [r3 + r5 * 2]

movzx    t0d,      byte [r2 + r4]
movzx    t1d,      byte [r3 + r5]

sub      t0d,      t1d

mov      [r0],     t0w

movzx    t0d,      byte [r2 + r4 + 1]
movzx    t1d,      byte [r3 + r5 + 1]

sub      t0d,      t1d

mov      [r0 + 2], t0w
%endif
RET

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_2x8(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%if HIGH_BIT_DEPTH
INIT_XMM sse2
cglobal pixel_sub_ps_2x8, 6, 6, 8, dest, deststride, src0, src1, srcstride0, srcstride1
    add      r1,    r1
    add      r4,    r4
    add      r5,    r5
    movd     m0,    [r2]
    movd     m1,    [r3]
    movd     m2,    [r2 + r4]
    movd     m3,    [r3 + r5]
    movd     m4,    [r2 + 2 * r4]
    movd     m5,    [r3 + 2 * r5]
    lea      r2,    [r2 + 2 * r4]
    lea      r3,    [r3 + 2 * r5]
    movd     m6,    [r2 + r4]
    movd     m7,    [r3 + r5]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    psubw    m6,    m7

    movd     [r0],           m0
    movd     [r0 + r1],      m2
    movd     [r0 + 2 * r1],  m4
    lea      r0,             [r0 + 2 * r1]
    movd     [r0 + r1],      m6

    movd     m0,    [r2 + 2 * r4]
    movd     m1,    [r3 + 2 * r5]
    lea      r2,    [r2 + 2 * r4]
    lea      r3,    [r3 + 2 * r5]
    movd     m2,    [r2 + r4]
    movd     m3,    [r3 + r5]
    movd     m4,    [r2 + 2 * r4]
    movd     m5,    [r3 + 2 * r5]
    lea      r2,    [r2 + 2 * r4]
    lea      r3,    [r3 + 2 * r5]
    movd     m6,    [r2 + r4]
    movd     m7,    [r3 + r5]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    psubw    m6,    m7

    movd     [r0 + 2 * r1],  m0
    lea      r0,             [r0 + 2 * r1]
    movd     [r0 + r1],      m2
    movd     [r0 + 2 * r1],  m4
    lea      r0,             [r0 + 2 * r1]
    movd     [r0 + r1],      m6
%else
INIT_XMM sse4
%if ARCH_X86_64
    cglobal pixel_sub_ps_2x8, 6, 8, 0

    %define tmp_r1     r1
    DECLARE_REG_TMP    6, 7
%else
    cglobal pixel_sub_ps_2x8, 6, 7, 0, 0-4

    %define tmp_r1     dword [rsp]
    DECLARE_REG_TMP    6, 1
%endif ; ARCH_X86_64

    add    r1,         r1

%if ARCH_X86_64 == 0
    mov    tmp_r1,     r1

%endif

    movzx    t0d,      byte [r2]
    movzx    t1d,      byte [r3]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + 1]
    movzx    t1d,      byte [r3 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    movzx    t0d,      byte [r2 + r4]
    movzx    t1d,      byte [r3 + r5]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 + 1]
    movzx    t1d,      byte [r3 + r5 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    movzx    t0d,      byte [r2 + r4 * 2]
    movzx    t1d,      byte [r3 + r5 * 2]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 * 2 + 1]
    movzx    t1d,      byte [r3 + r5 * 2 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    lea      r2,       [r2 + r4 * 2]
    lea      r3,       [r3 + r5 * 2]

    movzx    t0d,      byte [r2 + r4]
    movzx    t1d,      byte [r3 + r5]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 + 1]
    movzx    t1d,      byte [r3 + r5 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    movzx    t0d,      byte [r2 + r4 * 2]
    movzx    t1d,      byte [r3 + r5 * 2]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 * 2 + 1]
    movzx    t1d,      byte [r3 + r5 * 2 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    lea      r2,       [r2 + r4 * 2]
    lea      r3,       [r3 + r5 * 2]

    movzx    t0d,      byte [r2 + r4]
    movzx    t1d,      byte [r3 + r5]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 + 1]
    movzx    t1d,      byte [r3 + r5 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    movzx    t0d,      byte [r2 + r4 * 2]
    movzx    t1d,      byte [r3 + r5 * 2]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 * 2 + 1]
    movzx    t1d,      byte [r3 + r5 * 2 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    lea      r2,       [r2 + r4 * 2]
    lea      r3,       [r3 + r5 * 2]

    movzx    t0d,      byte [r2 + r4]
    movzx    t1d,      byte [r3 + r5]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 + 1]
    movzx    t1d,      byte [r3 + r5 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w
%endif
RET

;-----------------------------------------------------------------------------
; void pixel_sub_sp_c_4x2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%if HIGH_BIT_DEPTH
INIT_XMM sse2
cglobal pixel_sub_ps_4x2, 6, 6, 8, dest, deststride, src0, src1, srcstride0, srcstride1
    add      r1,    r1
    add      r4,    r4
    add      r5,    r5
    movh     m0,    [r2]
    movh     m1,    [r3]
    movh     m2,    [r2 + r4]
    movh     m3,    [r3 + r5]
    psubw    m0,    m1
    psubw    m2,    m3

    movh     [r0],         m0
    movh     [r0 + r1],    m2
%else
INIT_XMM sse4
cglobal pixel_sub_ps_4x2, 6, 6, 4, dest, deststride, src0, src1, srcstride0, srcstride1

add          r1,    r1

movd         m0,    [r2]
movd         m1,    [r3]

movd         m2,    [r2 + r4]
movd         m3,    [r3 + r5]

punpckldq    m0,    m2
punpckldq    m1,    m3
pmovzxbw     m0,    m0
pmovzxbw     m1,    m1

psubw        m0,    m1

movlps    [r0],         m0
movhps    [r0 + r1],    m0
%endif
RET

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W4_H4 2
cglobal pixel_sub_ps_%1x%2, 6, 7, 8, dest, deststride, src0, src1, srcstride0, srcstride1
    mov    r6d,    %2/4
    add          r1,    r1
%if HIGH_BIT_DEPTH
    add      r4,    r4
    add      r5,    r5
.loop
    movh     m0,    [r2]
    movh     m1,    [r3]
    movh     m2,    [r2 + r4]
    movh     m3,    [r3 + r5]
    movh     m4,    [r2 + 2 * r4]
    movh     m5,    [r3 + 2 * r5]
    lea      r2,    [r2 + 2 * r4]
    lea      r3,    [r3 + 2 * r5]
    movh     m6,    [r2 + r4]
    movh     m7,    [r3 + r5]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    psubw    m6,    m7

    movh     [r0],           m0
    movh     [r0 + r1],      m2
    movh     [r0 + 2 * r1],  m4
    lea      r0,             [r0 + 2 * r1]
    movh     [r0 + r1],      m6
%else
.loop
    movd         m0,    [r2]
    movd         m1,    [r3]
    movd         m2,    [r2 + r4]
    movd         m3,    [r3 + r5]
    movd         m4,    [r2 + 2 * r4]
    movd         m5,    [r3 + 2 * r5]
    lea          r2,    [r2 + 2 * r4]
    lea          r3,    [r3 + 2 * r5]
    movd         m6,    [r2 + r4]
    movd         m7,    [r3 + r5]
    punpckldq    m0,    m2
    punpckldq    m1,    m3
    punpckldq    m4,    m6
    punpckldq    m5,    m7
    pmovzxbw     m0,    m0
    pmovzxbw     m1,    m1
    pmovzxbw     m4,    m4
    pmovzxbw     m5,    m5
    psubw        m0,    m1
    psubw        m4,    m5

    movlps    [r0],             m0
    movhps    [r0 + r1],        m0
    movlps    [r0 + 2 * r1],    m4
    lea       r0,               [r0 + 2 * r1]
    movhps    [r0 + r1],        m4
%endif
    dec    r6d
    lea    r2,    [r2 + 2 * r4]
    lea    r3,    [r3 + 2 * r5]
    lea    r0,    [r0 + 2 * r1]
    jnz    .loop
RET
%endmacro
%if HIGH_BIT_DEPTH
INIT_XMM sse2
PIXELSUB_PS_W4_H4 4, 4
PIXELSUB_PS_W4_H4 4, 8
PIXELSUB_PS_W4_H4 4, 16
%else
INIT_XMM sse4
PIXELSUB_PS_W4_H4 4, 4
PIXELSUB_PS_W4_H4 4, 8
PIXELSUB_PS_W4_H4 4, 16
%endif
;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W6_H4 2
cglobal pixel_sub_ps_%1x%2, 6, 7, 8, dest, deststride, src0, src1, srcstride0, srcstride1
    add    r1,     r1
    mov    r6d,    %2/4
%if HIGH_BIT_DEPTH
    add         r4,    r4
    add         r5,    r5
.loop
    movu        m0,    [r2]
    movu        m1,    [r3]
    movu        m2,    [r2 + r4]
    movu        m3,    [r3 + r5]
    mova        m4,    m0
    mova        m5,    m1
    mova        m6,    m2
    mova        m7,    m3
    punpckhqdq  m4,    m4
    punpckhqdq  m5,    m5
    punpckhqdq  m6,    m6
    punpckhqdq  m7,    m7
    psubw       m0,    m1
    psubw       m2,    m3
    psubw       m4,    m5
    psubw       m6,    m7

    lea      r2,    [r2 + 2 * r4]
    lea      r3,    [r3 + 2 * r5]
    movh     [r0],             m0
    movd     [r0 + 8],         m4
    movh     [r0 + r1],        m2
    movd     [r0 + r1 + 8],    m6

    movu        m0,    [r2]
    movu        m1,    [r3]
    movu        m2,    [r2 + r4]
    movu        m3,    [r3 + r5]
    mova        m4,    m0
    mova        m5,    m1
    mova        m6,    m2
    mova        m7,    m3
    punpckhqdq  m4,    m4
    punpckhqdq  m5,    m5
    punpckhqdq  m6,    m6
    punpckhqdq  m7,    m7
    psubw       m0,    m1
    psubw       m2,    m3
    psubw       m4,    m5
    psubw       m6,    m7
    lea      r0,    [r0 + 2 * r1]
    movh     [r0],             m0
    movd     [r0 + 8],         m4
    movh     [r0 + r1],        m2
    movd     [r0 + r1 + 8],    m6
%else
.loop
    movh        m0,    [r2]
    movh        m1,    [r3]
    movh        m2,    [r2 + r4]
    movh        m3,    [r3 + r5]
    movh        m4,    [r2 + 2 * r4]
    movh        m5,    [r3 + 2 * r5]
    lea         r2,    [r2 + 2 * r4]
    lea         r3,    [r3 + 2 * r5]
    movh        m6,    [r2 + r4]
    movh        m7,    [r3 + r5]
    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5
    pmovzxbw    m6,    m6
    pmovzxbw    m7,    m7
    psubw       m0,    m1
    psubw       m2,    m3
    psubw       m4,    m5
    psubw       m6,    m7

    movh      [r0],                 m0
    pextrd    [r0 + 8],             m0,    2
    movh      [r0 + r1],            m2
    pextrd    [r0 + r1 + 8],        m2,    2
    movh      [r0 + 2* r1],         m4
    pextrd    [r0 + 2 * r1 + 8],    m4,    2
    lea       r0,                   [r0 + 2 * r1]
    movh      [r0 + r1],            m6
    pextrd    [r0 + r1 + 8],        m6,    2
%endif
    dec    r6d
    lea    r2,    [r2 + 2 * r4]
    lea    r3,    [r3 + 2 * r5]
    lea    r0,    [r0 + 2 * r1]
jnz    .loop
RET
%endmacro

%if HIGH_BIT_DEPTH
INIT_XMM sse2
PIXELSUB_PS_W6_H4 6, 8
%else
INIT_XMM sse4
PIXELSUB_PS_W6_H4 6, 8
%endif
;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_8x2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%if HIGH_BIT_DEPTH
INIT_XMM sse2
cglobal pixel_sub_ps_8x2, 6, 6, 4, dest, deststride, src0, src1, srcstride0, srcstride1
    add     r1,    r1
    add     r4,    r4
    add     r5,    r5
    movu    m0,    [r2]
    movu    m1,    [r3]
    movu    m2,    [r2 + r4]
    movu    m3,    [r3 + r5]
%else
INIT_XMM sse4
cglobal pixel_sub_ps_8x2, 6, 6, 4, dest, deststride, src0, src1, srcstride0, srcstride1
    add         r1,    r1
    movh        m0,    [r2]
    movh        m1,    [r3]
    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    movh        m2,    [r2 + r4]
    movh        m3,    [r3 + r5]
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
%endif
    psubw       m0,    m1
    psubw       m2,    m3

    movu    [r0],         m0
    movu    [r0 + r1],    m2
    RET

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_8x6(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%if HIGH_BIT_DEPTH
INIT_XMM sse2
cglobal pixel_sub_ps_8x6, 6, 6, 8, dest, deststride, src0, src1, srcstride0, srcstride1
    add      r1,    r1
    add      r4,    r4
    add      r5,    r5
    movu     m0,    [r2]
    movu     m1,    [r3]
    movu     m2,    [r2 + r4]
    movu     m3,    [r3 + r5]
    lea      r2,    [r2 + 2 * r4]
    lea      r3,    [r3 + 2 * r5]
    movu     m4,    [r2]
    movu     m5,    [r3]
    movu     m6,    [r2 + r4]
    movu     m7,    [r3 + r5]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    psubw    m6,    m7

    movu    [r0],             m0
    movu    [r0 + r1],        m2
    lea     r0,               [r0 + 2 * r1]
    movu    [r0],             m4
    movu    [r0 + r1],        m6

    lea      r2,    [r2 + 2 * r4]
    lea      r3,    [r3 + 2 * r5]
    movu     m0,    [r2]
    movu     m1,    [r3]
    movu     m2,    [r2 + r4]
    movu     m3,    [r3 + r5]
    psubw    m0,    m1
    psubw    m2,    m3

    lea     r0,               [r0 + 2 * r1]
    movu    [r0],             m0
    movu    [r0 + r1],        m2
%else

INIT_XMM sse4
cglobal pixel_sub_ps_8x6, 6, 6, 8, dest, deststride, src0, src1, srcstride0, srcstride1
    add         r1,    r1
    movh        m0,    [r2]
    movh        m1,    [r3]
    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    movh        m2,    [r2 + r4]
    movh        m3,    [r3 + r5]
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
    movh        m4,    [r2 + 2 * r4]
    movh        m5,    [r3 + 2 * r5]
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5
    psubw       m0,    m1
    psubw       m2,    m3
    psubw       m4,    m5
    lea         r2,    [r2 + 2 * r4]
    lea         r3,    [r3 + 2 * r5]
    movh        m6,    [r2 + r4]
    movh        m7,    [r3 + r5]
    pmovzxbw    m6,    m6
    pmovzxbw    m7,    m7
    movh        m1,    [r2 + 2 * r4]
    movh        m3,    [r3 + 2 * r5]
    pmovzxbw    m1,    m1
    pmovzxbw    m3,    m3
    psubw       m6,    m7
    psubw       m1,    m3
    lea         r2,    [r2 + 2 * r4]
    lea         r3,    [r3 + 2 * r5]
    movh        m3,    [r2 + r4]
    movh        m5,    [r3 + r5]
    pmovzxbw    m3,    m3
    pmovzxbw    m5,    m5
    psubw       m3,     m5

    movu    [r0],             m0
    movu    [r0 + r1],        m2
    movu    [r0 + 2 * r1],    m4
    lea     r0,               [r0 + 2 * r1]
    movu    [r0 + r1],        m6
    movu    [r0 + 2 * r1],    m1
    lea     r0,               [r0 + 2 * r1]
    movu    [r0 + r1],        m3
%endif
    RET

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W8_H4 2
cglobal pixel_sub_ps_%1x%2, 6, 7, 8, dest, deststride, src0, src1, srcstride0, srcstride1
    add    r1,     r1
    mov    r6d,    %2/4
%if HIGH_BIT_DEPTH
    add     r4,    r4
    add     r5,    r5
.loop
    movu    m0,    [r2]
    movu    m1,    [r3]
    movu    m2,    [r2 + r4]
    movu    m3,    [r3 + r5]
    lea     r2,    [r2 + 2 * r4]
    lea     r3,    [r3 + 2 * r5]
    movu    m4,    [r2]
    movu    m5,    [r3]
    movu    m6,    [r2 + r4]
    movu    m7,    [r3 + r5]
%else

.loop
    movh        m0,    [r2]
    movh        m1,    [r3]
    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    movh        m2,    [r2 + r4]
    movh        m3,    [r3 + r5]
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
    movh        m4,    [r2 + 2 * r4]
    movh        m5,    [r3 + 2 * r5]
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5
    lea         r2,    [r2 + 2 * r4]
    lea         r3,    [r3 + 2 * r5]
    movh        m6,    [r2 + r4]
    movh        m7,    [r3 + r5]
    pmovzxbw    m6,    m6
    pmovzxbw    m7,    m7
%endif
    psubw       m0,    m1
    psubw       m2,    m3
    psubw       m4,    m5
    psubw       m6,    m7

    movu    [r0],             m0
    movu    [r0 + r1],        m2
    movu    [r0 + 2 * r1],    m4
    lea     r0,               [r0 + 2 * r1]
    movu    [r0 + r1],        m6

    dec     r6d
    lea     r2,               [r2 + 2 * r4]
    lea     r3,               [r3 + 2 * r5]
    lea     r0,               [r0 + 2 * r1]
    jnz    .loop
    RET
%endmacro

%if HIGH_BIT_DEPTH
INIT_XMM sse2
PIXELSUB_PS_W8_H4 8, 4
PIXELSUB_PS_W8_H4 8, 8
PIXELSUB_PS_W8_H4 8, 16
PIXELSUB_PS_W8_H4 8, 32
%else
INIT_XMM sse4
PIXELSUB_PS_W8_H4 8, 4
PIXELSUB_PS_W8_H4 8, 8
PIXELSUB_PS_W8_H4 8, 16
PIXELSUB_PS_W8_H4 8, 32
%endif

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W12_H4 2
cglobal pixel_sub_ps_%1x%2, 6, 7, 8, dest, deststride, src0, src1, srcstride0, srcstride1
    add    r1,     r1
    mov    r6d,    %2/4
%if HIGH_BIT_DEPTH
    add     r4,    r4
    add     r5,    r5
.loop
    movu     m0,    [r2]
    movu     m1,    [r3]
    movh     m2,    [r2 + 16]
    movh     m3,    [r3 + 16]
    movu     m4,    [r2 + r4]
    movu     m5,    [r3 + r5]
    movh     m6,    [r2 + r4 + 16]
    movh     m7,    [r3 + r5 + 16]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    psubw    m6,    m7
    lea      r2,    [r2 + 2 * r4]
    lea      r3,    [r3 + 2 * r5]

    movu    [r0],              m0
    movh    [r0 + 16],         m2
    movu    [r0 + r1],         m4
    movh    [r0 + r1 + 16],    m6

    movu     m0,    [r2]
    movu     m1,    [r3]
    movh     m2,    [r2 + 16]
    movh     m3,    [r3 + 16]
    movu     m4,    [r2 + r4]
    movu     m5,    [r3 + r5]
    movh     m6,    [r2 + r4 + 16]
    movh     m7,    [r3 + r5 + 16]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    psubw    m6,    m7
    lea     r0,     [r0 + 2 * r1]

    movu    [r0],              m0
    movh    [r0 + 16],         m2
    movu    [r0 + r1],         m4
    movh    [r0 + r1 + 16],    m6
%else
.loop
    movu        m0,    [r2]
    movu        m1,    [r3]
    movu        m2,    [r2 + r4]
    movu        m3,    [r3 + r5]
    mova        m4,    m0
    mova        m5,    m1
    punpckhdq   m4,    m2
    punpckhdq   m5,    m3
    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5
    psubw       m0,    m1
    psubw       m2,    m3
    psubw       m4,    m5

    movu      [r0],              m0
    movlps    [r0 + 16],         m4
    movu      [r0 + r1],         m2
    movhps    [r0 + r1 + 16],    m4
    movu      m0,    [r2 + 2 * r4]
    movu      m1,    [r3 + 2 * r5]
    lea       r2,    [r2 + 2 * r4]
    lea       r3,    [r3 + 2 * r5]

    movu         m2,    [r2 + r4]
    movu         m3,    [r3 + r5]
    mova         m4,    m0
    mova         m5,    m1
    punpckhdq    m4,    m2
    punpckhdq    m5,    m3
    pmovzxbw     m0,    m0
    pmovzxbw     m1,    m1
    pmovzxbw     m2,    m2
    pmovzxbw     m3,    m3
    pmovzxbw     m4,    m4
    pmovzxbw     m5,    m5
    psubw        m0,    m1
    psubw        m2,    m3
    psubw        m4,    m5

    movu      [r0 + 2 * r1],         m0
    movlps    [r0 + 2 * r1 + 16],    m4
    lea       r0,                    [r0 + 2 * r1]
    movu      [r0 + r1],             m2
    movhps    [r0 + r1 + 16],        m4
%endif
    dec    r6d
    lea    r2,    [r2 + 2 * r4]
    lea    r3,    [r3 + 2 * r5]
    lea    r0,    [r0 + 2 * r1]
    jnz    .loop
    RET
%endmacro

%if HIGH_BIT_DEPTH
INIT_XMM sse2
PIXELSUB_PS_W12_H4 12, 16
%else
INIT_XMM sse4
PIXELSUB_PS_W12_H4 12, 16
%endif

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W16_H4 2
%if HIGH_BIT_DEPTH
cglobal pixel_sub_ps_%1x%2, 6, 7, 6, dest, deststride, src0, src1, srcstride0, srcstride1
    add    r1,     r1
    mov    r6d,    %2/4
    add     r4,    r4
    add     r5,    r5
.loop
    movu     m0,    [r2]
    movu     m1,    [r3]
    movu     m2,    [r2 + 16]
    movu     m3,    [r3 + 16]
    psubw    m0,    m1
    psubw    m2,    m3
    movu     m4,    [r2 + r4]
    movu     m5,    [r3 + r5]
    movu     m1,    [r2 + r4 + 16]
    movu     m3,    [r3 + r5 + 16]
    psubw    m4,    m5
    psubw    m1,    m3
    lea      r2,    [r2 + 2 * r4]
    lea      r3,    [r3 + 2 * r5]

    movu    [r0],              m0
    movu    [r0 + 16],         m2
    movu    [r0 + r1],         m4
    movu    [r0 + r1 + 16],    m1

    movu     m0,    [r2]
    movu     m1,    [r3]
    movu     m2,    [r2 + 16]
    movu     m3,    [r3 + 16]
    psubw    m0,    m1
    psubw    m2,    m3
    movu     m4,    [r2 + r4]
    movu     m5,    [r3 + r5]
    movu     m1,    [r2 + r4 + 16]
    movu     m3,    [r3 + r5 + 16]
    psubw    m4,    m5
    psubw    m1,    m3
    lea     r0,     [r0 + 2 * r1]

    movu    [r0],              m0
    movu    [r0 + 16],         m2
    movu    [r0 + r1],         m4
    movu    [r0 + r1 + 16],    m1
%else
cglobal pixel_sub_ps_%1x%2, 6, 7, 7, dest, deststride, src0, src1, srcstride0, srcstride1
    add    r1,     r1
    mov    r6d,    %2/4
    pxor   m6,     m6
.loop
    movu         m1,    [r2]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    psubw        m0,    m2
    psubw        m1,    m3

    movu         m5,    [r2 + r4]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    movu         m3,    [r3 + r5]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    psubw        m4,    m2
    psubw        m5,    m3

    movu    [r0],              m0
    movu    [r0 + 16],         m1
    movu    [r0 + r1],         m4
    movu    [r0 + r1 + 16],    m5

    movu         m1,    [r2 + 2 * r4]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3 + 2 * r5]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    lea          r2,    [r2 + 2 * r4]
    lea          r3,    [r3 + 2 * r5]
    psubw        m0,    m2
    psubw        m1,    m3
    movu         m5,    [r2 + r4]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    movu         m3,    [r3 + r5]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    psubw        m4,    m2
    psubw        m5,    m3

    movu    [r0 + 2 * r1],         m0
    movu    [r0 + 2 * r1 + 16],    m1
    lea     r0,                    [r0 + 2 * r1]
    movu    [r0 + r1],             m4
    movu    [r0 + r1 + 16],        m5
%endif
    dec    r6d
    lea    r2,    [r2 + 2 * r4]
    lea    r3,    [r3 + 2 * r5]
    lea    r0,    [r0 + 2 * r1]
    jnz    .loop

RET
%endmacro

%if HIGH_BIT_DEPTH
INIT_XMM sse2
PIXELSUB_PS_W16_H4 16, 4
PIXELSUB_PS_W16_H4 16, 8
PIXELSUB_PS_W16_H4 16, 12
PIXELSUB_PS_W16_H4 16, 16
PIXELSUB_PS_W16_H4 16, 32
PIXELSUB_PS_W16_H4 16, 64
%else
INIT_XMM sse4
PIXELSUB_PS_W16_H4 16, 4
PIXELSUB_PS_W16_H4 16, 8
PIXELSUB_PS_W16_H4 16, 12
PIXELSUB_PS_W16_H4 16, 16
PIXELSUB_PS_W16_H4 16, 32
PIXELSUB_PS_W16_H4 16, 64
%endif

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W24_H2 2
%if HIGH_BIT_DEPTH
cglobal pixel_sub_ps_%1x%2, 6, 7, 6, dest, deststride, src0, src1, srcstride0, srcstride1
    add     r1,     r1
    mov     r6d,    %2/2
    add     r4,     r4
    add     r5,     r5
.loop
    movu     m0,    [r2]
    movu     m1,    [r3]
    movu     m2,    [r2 + 16]
    movu     m3,    [r3 + 16]
    movu     m4,    [r2 + 32]
    movu     m5,    [r3 + 32]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5

    movu    [r0],              m0
    movu    [r0 + 16],         m2
    movu    [r0 + 32],         m4

    movu     m0,    [r2 + r4]
    movu     m1,    [r3 + r5]
    movu     m2,    [r2 + r4 + 16]
    movu     m3,    [r3 + r5 + 16]
    movu     m4,    [r2 + r4 + 32]
    movu     m5,    [r3 + r5 + 32]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5

    movu    [r0 + r1],         m0
    movu    [r0 + r1 + 16],    m2
    movu    [r0 + r1 + 32],    m4
%else
cglobal pixel_sub_ps_%1x%2, 6, 7, 7, dest, deststride, src0, src1, srcstride0, srcstride1
    add    r1,     r1
    mov    r6d,    %2/2
    pxor   m6,     m6
.loop
    movu         m1,    [r2]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movh         m2,    [r2 + 16]
    pmovzxbw     m2,    m2
    movu         m4,    [r3]
    pmovzxbw     m3,    m4
    punpckhbw    m4,    m6
    movh         m5,    [r3 + 16]
    pmovzxbw     m5,    m5
    psubw        m0,    m3
    psubw        m1,    m4
    psubw        m2,    m5

    movu    [r0],         m0
    movu    [r0 + 16],    m1
    movu    [r0 + 32],    m2

    movu         m1,    [r2 + r4]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movh         m2,    [r2 + r4 + 16]
    pmovzxbw     m2,    m2
    movu         m4,    [r3 + r5]
    pmovzxbw     m3,    m4
    punpckhbw    m4,    m6
    movh         m5,    [r3 + r5 + 16]
    pmovzxbw     m5,    m5
    psubw        m0,    m3
    psubw        m1,    m4
    psubw        m2,    m5

    movu    [r0 + r1],         m0
    movu    [r0 + r1 + 16],    m1
    movu    [r0 + r1 + 32],    m2
%endif

    dec    r6d
    lea    r2,    [r2 + 2 * r4]
    lea    r3,    [r3 + 2 * r5]
    lea    r0,    [r0 + 2 * r1]
    jnz    .loop

RET
%endmacro

%if HIGH_BIT_DEPTH
INIT_XMM sse2
PIXELSUB_PS_W24_H2 24, 32
%else
INIT_XMM sse4
PIXELSUB_PS_W24_H2 24, 32
%endif

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W32_H2 2
cglobal pixel_sub_ps_%1x%2, 6, 7, 6, dest, deststride, src0, src1, srcstride0, srcstride1
    add     r1,     r1
    mov     r6d,    %2/2
%if HIGH_BIT_DEPTH
    add     r4,     r4
    add     r5,     r5
.loop
    movu     m0,    [r2]
    movu     m1,    [r3]
    movu     m2,    [r2 + 16]
    movu     m3,    [r3 + 16]
    movu     m4,    [r2 + 32]
    movu     m5,    [r3 + 32]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    movu     m3,    [r2 + 48]
    movu     m5,    [r3 + 48]
    psubw    m3,    m5

    movu    [r0],         m0
    movu    [r0 + 16],    m2
    movu    [r0 + 32],    m4
    movu    [r0 + 48],    m3

    movu     m0,    [r2 + r4]
    movu     m1,    [r3 + r5]
    movu     m2,    [r2 + r4 + 16]
    movu     m3,    [r3 + r5 + 16]
    movu     m4,    [r2 + r4 + 32]
    movu     m5,    [r3 + r5 + 32]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    movu     m3,    [r2 + r4 + 48]
    movu     m5,    [r3 + r5 + 48]
    psubw    m3,    m5

    movu    [r0 + r1],         m0
    movu    [r0 + r1 + 16],    m2
    movu    [r0 + r1 + 32],    m4
    movu    [r0 + r1 + 48],    m3
%else
.loop
    movh        m0,    [r2]
    movh        m1,    [r2 + 8]
    movh        m2,    [r2 + 16]
    movh        m3,    [r3]
    movh        m4,    [r3 + 8]
    movh        m5,    [r3 + 16]
    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5
    psubw       m0,    m3
    psubw       m1,    m4
    psubw       m2,    m5
    movh        m3,    [r2 + 24]
    movh        m4,    [r3 + 24]
    pmovzxbw    m4,    m4
    pmovzxbw    m3,    m3
    psubw       m3,    m4

    movu    [r0],         m0
    movu    [r0 + 16],    m1
    movu    [r0 + 32],    m2
    movu    [r0 + 48],    m3

    movh        m0,    [r2 + r4]
    movh        m1,    [r2 + r4 + 8]
    movh        m2,    [r2 + r4 + 16]
    movh        m3,    [r3 + r5]
    movh        m4,    [r3 + r5 + 8]
    movh        m5,    [r3 + r5 + 16]
    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5
    psubw       m0,    m3
    psubw       m1,    m4
    psubw       m2,    m5
    movh        m3,    [r2 + r4 + 24]
    movh        m4,    [r3 + r5 + 24]
    pmovzxbw    m3,    m3
    pmovzxbw    m4,    m4
    psubw       m3,    m4

    movu    [r0 + r1],         m0
    movu    [r0 + r1 + 16],    m1
    movu    [r0 + r1 + 32],    m2
    movu    [r0 + r1 + 48],    m3
%endif
    dec    r6d
    lea    r2,    [r2 + 2 * r4]
    lea    r3,    [r3 + 2 * r5]
    lea    r0,    [r0 + 2 * r1]
    jnz    .loop
    RET
%endmacro

%if HIGH_BIT_DEPTH
INIT_XMM sse2
PIXELSUB_PS_W32_H2 32, 8
PIXELSUB_PS_W32_H2 32, 16
PIXELSUB_PS_W32_H2 32, 24
PIXELSUB_PS_W32_H2 32, 32
PIXELSUB_PS_W32_H2 32, 64
%else
INIT_XMM sse4
PIXELSUB_PS_W32_H2 32, 8
PIXELSUB_PS_W32_H2 32, 16
PIXELSUB_PS_W32_H2 32, 24
PIXELSUB_PS_W32_H2 32, 32
PIXELSUB_PS_W32_H2 32, 64
%endif

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W48_H2 2
%if HIGH_BIT_DEPTH
cglobal pixel_sub_ps_%1x%2, 6, 7, 6, dest, deststride, src0, src1, srcstride0, srcstride1
    add     r1,     r1
    mov     r6d,    %2/2
    add     r4,     r4
    add     r5,     r5
.loop
    movu     m0,    [r2]
    movu     m1,    [r3]
    movu     m2,    [r2 + 16]
    movu     m3,    [r3 + 16]
    movu     m4,    [r2 + 32]
    movu     m5,    [r3 + 32]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5

    movu    [r0],         m0
    movu    [r0 + 16],    m2
    movu    [r0 + 32],    m4

    movu     m0,    [r2 + 48]
    movu     m1,    [r3 + 48]
    movu     m2,    [r2 + 64]
    movu     m3,    [r3 + 64]
    movu     m4,    [r2 + 80]
    movu     m5,    [r3 + 80]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5

    movu    [r0 + 48],    m0
    movu    [r0 + 64],    m2
    movu    [r0 + 80],    m4

    movu     m0,    [r2 + r4]
    movu     m1,    [r3 + r5]
    movu     m2,    [r2 + r4 + 16]
    movu     m3,    [r3 + r5 + 16]
    movu     m4,    [r2 + r4 + 32]
    movu     m5,    [r3 + r5 + 32]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5

    movu    [r0 + r1],         m0
    movu    [r0 + r1 + 16],    m2
    movu    [r0 + r1 + 32],    m4

    movu     m0,    [r2 + r4 + 48]
    movu     m1,    [r3 + r5 + 48]
    movu     m2,    [r2 + r4 + 64]
    movu     m3,    [r3 + r5 + 64]
    movu     m4,    [r2 + r4 + 80]
    movu     m5,    [r3 + r5 + 80]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5

    movu    [r0 + r1 + 48],    m0
    movu    [r0 + r1 + 64],    m2
    movu    [r0 + r1 + 80],    m4
%else

cglobal pixel_sub_ps_%1x%2, 6, 7, 7, dest, deststride, src0, src1, srcstride0, srcstride1
    add     r1,     r1
    mov     r6d,    %2/2
    pxor    m6,     m6
.loop
    movu         m1,    [r2]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    movu         m5,    [r2 + 16]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0],         m0
    movu    [r0 + 16],    m1

    movu         m3,    [r3 + 16]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    psubw        m4,    m2
    psubw        m5,    m3

    movu    [r0 + 32],    m4
    movu    [r0 + 48],    m5

    movu         m1,    [r2 + 32]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3 + 32]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0 + 64],    m0
    movu    [r0 + 80],    m1

    movu         m1,    [r2 + r4]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3 + r5]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    movu         m5,    [r2 + r5 + 16]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0 + r1],         m0
    movu    [r0 + r1 + 16],    m1

    movu         m3,    [r3 + r4 + 16]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    psubw        m4,    m2
    psubw        m5,    m3

    movu    [r0 + r1 + 32],    m4
    movu    [r0 + r1 + 48],    m5

    movu         m1,    [r2 + r4 + 32]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3 + r5 + 32]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0 + r1 + 64],    m0
    movu    [r0 + r1 + 80],    m1
%endif
    dec    r6d
    lea    r2,    [r2 + 2 * r4]
    lea    r3,    [r3 + 2 * r5]
    lea    r0,    [r0 + 2 * r1]
    jnz    .loop

RET
%endmacro

%if HIGH_BIT_DEPTH
INIT_XMM sse2
PIXELSUB_PS_W48_H2 48, 64
%else
INIT_XMM sse4
PIXELSUB_PS_W48_H2 48, 64
%endif

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W64_H2 2
%if HIGH_BIT_DEPTH
cglobal pixel_sub_ps_%1x%2, 6, 7, 6, dest, deststride, src0, src1, srcstride0, srcstride1
    add     r1,     r1
    mov     r6d,    %2/2
    add     r4,     r4
    add     r5,     r5
.loop
    movu     m0,    [r2]
    movu     m1,    [r3]
    movu     m2,    [r2 + 16]
    movu     m3,    [r3 + 16]
    movu     m4,    [r2 + 32]
    movu     m5,    [r3 + 32]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    movu     m3,    [r2 + 48]
    movu     m5,    [r3 + 48]
    psubw    m3,    m5

    movu    [r0],         m0
    movu    [r0 + 16],    m2
    movu    [r0 + 32],    m4
    movu    [r0 + 48],    m3

    movu     m0,    [r2 + 64]
    movu     m1,    [r3 + 64]
    movu     m2,    [r2 + 80]
    movu     m3,    [r3 + 80]
    movu     m4,    [r2 + 96]
    movu     m5,    [r3 + 96]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    movu     m3,    [r2 + 112]
    movu     m5,    [r3 + 112]
    psubw    m3,    m5

    movu    [r0 + 64],     m0
    movu    [r0 + 80],     m2
    movu    [r0 + 96],     m4
    movu    [r0 + 112],    m3

    movu     m0,    [r2 + r4]
    movu     m1,    [r3 + r5]
    movu     m2,    [r2 + r4 + 16]
    movu     m3,    [r3 + r5 + 16]
    movu     m4,    [r2 + r4 + 32]
    movu     m5,    [r3 + r5 + 32]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    movu     m3,    [r2 + r4 + 48]
    movu     m5,    [r3 + r5 + 48]
    psubw    m3,    m5

    movu    [r0 + r1],         m0
    movu    [r0 + r1 + 16],    m2
    movu    [r0 + r1 + 32],    m4
    movu    [r0 + r1 + 48],    m3

    movu     m0,    [r2 + r4 + 64]
    movu     m1,    [r3 + r5 + 64]
    movu     m2,    [r2 + r4 + 80]
    movu     m3,    [r3 + r5 + 80]
    movu     m4,    [r2 + r4 + 96]
    movu     m5,    [r3 + r5 + 96]
    psubw    m0,    m1
    psubw    m2,    m3
    psubw    m4,    m5
    movu     m3,    [r2 + r4 + 112]
    movu     m5,    [r3 + r5 + 112]
    psubw    m3,    m5

    movu    [r0 + r1 + 64],     m0
    movu    [r0 + r1 + 80],     m2
    movu    [r0 + r1 + 96],     m4
    movu    [r0 + r1 + 112],    m3

%else

cglobal pixel_sub_ps_%1x%2, 6, 7, 7, dest, deststride, src0, src1, srcstride0, srcstride1
    add     r1,     r1
    mov     r6d,    %2/2
    pxor    m6,     m6
.loop
    movu         m1,    [r2]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    movu         m5,    [r2 + 16]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0],         m0
    movu    [r0 + 16],    m1

    movu         m1,    [r3 + 16]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r2 + 32]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    psubw        m4,    m0
    psubw        m5,    m1

    movu    [r0 + 32],    m4
    movu    [r0 + 48],    m5

    movu         m5,    [r3 + 32]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    movu         m1,    [r2 + 48]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    psubw        m2,    m4
    psubw        m3,    m5

    movu    [r0 + 64],    m2
    movu    [r0 + 80],    m3

    movu         m3,    [r3 + 48]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    movu         m5,    [r2 + r4]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0 + 96],     m0
    movu    [r0 + 112],    m1

    movu         m1,    [r3 + r5]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r2 + r4 + 16]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    psubw        m4,    m0
    psubw        m5,    m1

    movu    [r0 + r1],         m4
    movu    [r0 + r1 + 16],    m5

    movu         m5,    [r3 + r5 + 16]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    movu         m1,    [r2 + r4 + 32]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    psubw        m2,    m4
    psubw        m3,    m5

    movu    [r0 + r1 + 32],    m2
    movu    [r0 + r1 + 48],    m3

    movu         m3,    [r3 + r5 + 32]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    movu         m5,    [r2 + r4 + 48]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0 + r1 + 64],    m0
    movu    [r0 + r1 + 80],    m1

    movu         m1,    [r3 + r5 + 48]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    psubw        m4,    m0
    psubw        m5,    m1

    movu    [r0 + r1 + 96],     m4
    movu    [r0 + r1 + 112],    m5
%endif
    dec    r6d
    lea    r2,    [r2 + 2 * r4]
    lea    r3,    [r3 + 2 * r5]
    lea    r0,    [r0 + 2 * r1]
    jnz    .loop
    RET
%endmacro

%if HIGH_BIT_DEPTH
INIT_XMM sse2
PIXELSUB_PS_W64_H2 64, 16
PIXELSUB_PS_W64_H2 64, 32
PIXELSUB_PS_W64_H2 64, 48
PIXELSUB_PS_W64_H2 64, 64
%else
INIT_XMM sse4
PIXELSUB_PS_W64_H2 64, 16
PIXELSUB_PS_W64_H2 64, 32
PIXELSUB_PS_W64_H2 64, 48
PIXELSUB_PS_W64_H2 64, 64
%endif

;=============================================================================
; variance
;=============================================================================

%macro VAR_START 1
    pxor  m5, m5    ; sum
    pxor  m6, m6    ; sum squared
%if HIGH_BIT_DEPTH == 0
%if %1
    mova  m7, [pw_00ff]
%elif mmsize < 32
    pxor  m7, m7    ; zero
%endif
%endif ; !HIGH_BIT_DEPTH
%endmacro

%macro VAR_END 2
%if HIGH_BIT_DEPTH
%if mmsize == 8 && %1*%2 == 256
    HADDUW  m5, m2
%else
%if %1 >= 32
    HADDW     m5,    m2
    movd      m7,    r4d
    paddd     m5,    m7
%else
    HADDW   m5, m2
%endif
%endif
%else ; !HIGH_BIT_DEPTH
%if %1 == 64
    HADDW     m5,    m2
    movd      m7,    r4d
    paddd     m5,    m7
%else
    HADDW   m5, m2
%endif
%endif ; HIGH_BIT_DEPTH
    HADDD   m6, m1
%if ARCH_X86_64
    punpckldq m5, m6
    movq   rax, m5
%else
    movd   eax, m5
    movd   edx, m6
%endif
    RET
%endmacro

%macro VAR_CORE 0
    paddw     m5, m0
    paddw     m5, m3
    paddw     m5, m1
    paddw     m5, m4
    pmaddwd   m0, m0
    pmaddwd   m3, m3
    pmaddwd   m1, m1
    pmaddwd   m4, m4
    paddd     m6, m0
    paddd     m6, m3
    paddd     m6, m1
    paddd     m6, m4
%endmacro

%macro VAR_2ROW 3
    mov      r2d, %2
.loop%3:
%if HIGH_BIT_DEPTH
    movu      m0, [r0]
    movu      m1, [r0+mmsize]
    movu      m3, [r0+%1]
    movu      m4, [r0+%1+mmsize]
%else ; !HIGH_BIT_DEPTH
    mova      m0, [r0]
    punpckhbw m1, m0, m7
    mova      m3, [r0+%1]
    mova      m4, m3
    punpcklbw m0, m7
%endif ; HIGH_BIT_DEPTH
%ifidn %1, r1
    lea       r0, [r0+%1*2]
%else
    add       r0, r1
%endif
%if HIGH_BIT_DEPTH == 0
    punpcklbw m3, m7
    punpckhbw m4, m7
%endif ; !HIGH_BIT_DEPTH
    VAR_CORE
    dec r2d
    jg .loop%3
%endmacro

;-----------------------------------------------------------------------------
; int pixel_var_wxh( uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
INIT_MMX mmx2
cglobal pixel_var_16x16, 2,3
    FIX_STRIDES r1
    VAR_START 0
    VAR_2ROW 8*SIZEOF_PIXEL, 16, 1
    VAR_END 16, 16

cglobal pixel_var_8x8, 2,3
    FIX_STRIDES r1
    VAR_START 0
    VAR_2ROW r1, 4, 1
    VAR_END 8, 8

%if HIGH_BIT_DEPTH
%macro VAR 0
cglobal pixel_var_16x16, 2,3,8
    FIX_STRIDES r1
    VAR_START 0
    VAR_2ROW r1, 8, 1
    VAR_END 16, 16

cglobal pixel_var_8x8, 2,3,8
    lea       r2, [r1*3]
    VAR_START 0
    movu      m0, [r0]
    movu      m1, [r0+r1*2]
    movu      m3, [r0+r1*4]
    movu      m4, [r0+r2*2]
    lea       r0, [r0+r1*8]
    VAR_CORE
    movu      m0, [r0]
    movu      m1, [r0+r1*2]
    movu      m3, [r0+r1*4]
    movu      m4, [r0+r2*2]
    VAR_CORE
    VAR_END 8, 8

cglobal pixel_var_32x32, 2,6,8
    FIX_STRIDES r1
    mov       r3,    r0
    VAR_START 0
    VAR_2ROW  r1,    8, 1
    HADDW      m5,    m2
    movd       r4d,   m5
    pxor       m5,    m5
    VAR_2ROW  r1,    8, 2
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    lea       r0,    [r3 + 32]
    VAR_2ROW  r1,    8, 3
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    VAR_2ROW  r1,    8, 4
    VAR_END   32,    32

cglobal pixel_var_64x64, 2,6,8
    FIX_STRIDES r1
    mov       r3,    r0
    VAR_START 0
    VAR_2ROW  r1,    8, 1
    HADDW      m5,    m2
    movd       r4d,   m5
    pxor       m5,    m5
    VAR_2ROW  r1,    8, 2
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    VAR_2ROW  r1,    8, 3
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    VAR_2ROW  r1,    8, 4
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    lea       r0,    [r3 + 32]
    VAR_2ROW  r1,    8, 5
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    VAR_2ROW  r1,    8, 6
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    VAR_2ROW  r1,    8, 7
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    VAR_2ROW  r1,    8, 8
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    lea       r0,    [r3 + 64]
    VAR_2ROW  r1,    8, 9
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    VAR_2ROW  r1,    8, 10
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    VAR_2ROW  r1,    8, 11
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    VAR_2ROW  r1,    8, 12
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    lea       r0,    [r3 + 96]
    VAR_2ROW  r1,    8, 13
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    VAR_2ROW  r1,    8, 14
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    VAR_2ROW  r1,    8, 15
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    VAR_2ROW  r1,    8, 16
    VAR_END   64,    64
%endmacro ; VAR

INIT_XMM sse2
VAR
INIT_XMM avx
VAR
INIT_XMM xop
VAR
%endif ; HIGH_BIT_DEPTH

%if HIGH_BIT_DEPTH == 0
%macro VAR 0
cglobal pixel_var_8x8, 2,3,8
    VAR_START 1
    lea       r2,    [r1 * 3]
    movh      m0,    [r0]
    movh      m3,    [r0 + r1]
    movhps    m0,    [r0 + r1 * 2]
    movhps    m3,    [r0 + r2]
    DEINTB    1, 0, 4, 3, 7
    lea       r0,    [r0 + r1 * 4]
    VAR_CORE
    movh      m0,    [r0]
    movh      m3,    [r0 + r1]
    movhps    m0,    [r0 + r1 * 2]
    movhps    m3,    [r0 + r2]
    DEINTB    1, 0, 4, 3, 7
    VAR_CORE
    VAR_END 8, 8

cglobal pixel_var_16x16_internal
    movu      m0,    [r0]
    movu      m3,    [r0 + r1]
    DEINTB    1, 0, 4, 3, 7
    VAR_CORE
    movu      m0,    [r0 + 2 * r1]
    movu      m3,    [r0 + r2]
    DEINTB    1, 0, 4, 3, 7
    lea       r0,    [r0 + r1 * 4]
    VAR_CORE
    movu      m0,    [r0]
    movu      m3,    [r0 + r1]
    DEINTB    1, 0, 4, 3, 7
    VAR_CORE
    movu      m0,    [r0 + 2 * r1]
    movu      m3,    [r0 + r2]
    DEINTB    1, 0, 4, 3, 7
    lea       r0,    [r0 + r1 * 4]
    VAR_CORE
    movu      m0,    [r0]
    movu      m3,    [r0 + r1]
    DEINTB    1, 0, 4, 3, 7
    VAR_CORE
    movu      m0,    [r0 + 2 * r1]
    movu      m3,    [r0 + r2]
    DEINTB    1, 0, 4, 3, 7
    lea       r0,    [r0 + r1 * 4]
    VAR_CORE
    movu      m0,    [r0]
    movu      m3,    [r0 + r1]
    DEINTB    1, 0, 4, 3, 7
    VAR_CORE
    movu      m0,    [r0 + 2 * r1]
    movu      m3,    [r0 + r2]
    DEINTB    1, 0, 4, 3, 7
    VAR_CORE
    ret

cglobal pixel_var_16x16, 2,3,8
    VAR_START 1
    lea     r2,    [r1 * 3]
    call    pixel_var_16x16_internal
    VAR_END 16, 16

cglobal pixel_var_32x32, 2,4,8
    VAR_START 1
    lea     r2,    [r1 * 3]
    mov     r3,    r0
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    lea       r0,    [r3 + 16]
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    VAR_END 32, 32

cglobal pixel_var_64x64, 2,6,8
    VAR_START 1
    lea     r2,    [r1 * 3]
    mov     r3,    r0
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    HADDW     m5,    m2
    movd      r4d,   m5
    pxor      m5,    m5
    lea       r0,    [r3 + 16]
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    lea       r0,    [r3 + 32]
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    lea       r0,    [r3 + 48]
    HADDW     m5,    m2
    movd      r5d,   m5
    add       r4,    r5
    pxor      m5,    m5
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    lea       r0,    [r0 + r1 * 4]
    call    pixel_var_16x16_internal
    VAR_END 64, 64
%endmacro ; VAR

INIT_XMM sse2
VAR
INIT_XMM avx
VAR
INIT_XMM xop
VAR

INIT_YMM avx2
cglobal pixel_var_16x16, 2,4,7
    VAR_START 0
    mov      r2d, 4
    lea       r3, [r1*3]
.loop:
    pmovzxbw  m0, [r0]
    pmovzxbw  m3, [r0+r1]
    pmovzxbw  m1, [r0+r1*2]
    pmovzxbw  m4, [r0+r3]
    lea       r0, [r0+r1*4]
    VAR_CORE
    dec r2d
    jg .loop
    vextracti128 xm0, m5, 1
    vextracti128 xm1, m6, 1
    paddw  xm5, xm0
    paddd  xm6, xm1
    HADDW  xm5, xm2
    HADDD  xm6, xm1
%if ARCH_X86_64
    punpckldq xm5, xm6
    movq   rax, xm5
%else
    movd   eax, xm5
    movd   edx, xm6
%endif
    RET
%endif ; !HIGH_BIT_DEPTH

%macro VAR2_END 3
    HADDW   %2, xm1
    movd   r1d, %2
    imul   r1d, r1d
    HADDD   %3, xm1
    shr    r1d, %1
    movd   eax, %3
    movd  [r4], %3
    sub    eax, r1d  ; sqr - (sum * sum >> shift)
    RET
%endmacro

