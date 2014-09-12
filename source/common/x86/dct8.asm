;*****************************************************************************
;* Copyright (C) 2013 x265 project
;*
;* Authors: Nabajit Deka <nabajit@multicorewareinc.com>
;*          Min Chen <chenm003@163.com> <min.chen@multicorewareinc.com>
;*          Li Cao <li@multicorewareinc.com>
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

;TO-DO : Further optimize the routines.

%include "x86inc.asm"
%include "x86util.asm"

SECTION_RODATA 32
tab_dct16_1:    dw 64, 64, 64, 64, 64, 64, 64, 64
                dw 90, 87, 80, 70, 57, 43, 25,  9
                dw 89, 75, 50, 18, -18, -50, -75, -89
                dw 87, 57,  9, -43, -80, -90, -70, -25
                dw 83, 36, -36, -83, -83, -36, 36, 83
                dw 80,  9, -70, -87, -25, 57, 90, 43
                dw 75, -18, -89, -50, 50, 89, 18, -75
                dw 70, -43, -87,  9, 90, 25, -80, -57
                dw 64, -64, -64, 64, 64, -64, -64, 64
                dw 57, -80, -25, 90, -9, -87, 43, 70
                dw 50, -89, 18, 75, -75, -18, 89, -50
                dw 43, -90, 57, 25, -87, 70,  9, -80
                dw 36, -83, 83, -36, -36, 83, -83, 36
                dw 25, -70, 90, -80, 43,  9, -57, 87
                dw 18, -50, 75, -89, 89, -75, 50, -18
                dw  9, -25, 43, -57, 70, -80, 87, -90


tab_dct16_2:    dw 64, 64, 64, 64, 64, 64, 64, 64
                dw -9, -25, -43, -57, -70, -80, -87, -90
                dw -89, -75, -50, -18, 18, 50, 75, 89
                dw 25, 70, 90, 80, 43, -9, -57, -87
                dw 83, 36, -36, -83, -83, -36, 36, 83
                dw -43, -90, -57, 25, 87, 70, -9, -80
                dw -75, 18, 89, 50, -50, -89, -18, 75
                dw 57, 80, -25, -90, -9, 87, 43, -70
                dw 64, -64, -64, 64, 64, -64, -64, 64
                dw -70, -43, 87,  9, -90, 25, 80, -57
                dw -50, 89, -18, -75, 75, 18, -89, 50
                dw 80, -9, -70, 87, -25, -57, 90, -43
                dw 36, -83, 83, -36, -36, 83, -83, 36
                dw -87, 57, -9, -43, 80, -90, 70, -25
                dw -18, 50, -75, 89, -89, 75, -50, 18
                dw 90, -87, 80, -70, 57, -43, 25, -9

dct16_shuf1:     times 2 db 14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1

dct16_shuf2:    times 2 db 0, 1, 14, 15, 2, 3, 12, 13, 4, 5, 10, 11, 6, 7, 8, 9

avx2_dct4:      dw 64, 64, 64, 64, 64, 64, 64, 64, 64, -64, 64, -64, 64, -64, 64, -64
                dw 83, 36, 83, 36, 83, 36, 83, 36, 36, -83, 36, -83, 36, -83, 36, -83

tab_dct4:       times 4 dw 64, 64
                times 4 dw 83, 36
                times 4 dw 64, -64
                times 4 dw 36, -83

dct4_shuf:      db 0, 1, 2, 3, 8, 9, 10, 11, 6, 7, 4, 5, 14, 15, 12, 13

tab_dst4:       times 2 dw 29, 55, 74, 84
                times 2 dw 74, 74,  0, -74
                times 2 dw 84, -29, -74, 55
                times 2 dw 55, -84, 74, -29

tab_idst4:      times 4 dw 29, +84
                times 4 dw +74, +55
                times 4 dw 55, -29
                times 4 dw +74, -84
                times 4 dw 74, -74
                times 4 dw 0, +74
                times 4 dw 84, +55
                times 4 dw -74, -29

tab_dct8_1:     times 2 dw 89, 50, 75, 18
                times 2 dw 75, -89, -18, -50
                times 2 dw 50, 18, -89, 75
                times 2 dw 18, 75, -50, -89

tab_dct8_2:     times 2 dd 83, 36
                times 2 dd 36, 83
                times 1 dd 89, 75, 50, 18
                times 1 dd 75, -18, -89, -50
                times 1 dd 50, -89, 18, 75
                times 1 dd 18, -50, 75, -89

tab_idct8_3:    times 4 dw 89, 75
                times 4 dw 50, 18
                times 4 dw 75, -18
                times 4 dw -89, -50
                times 4 dw 50, -89
                times 4 dw 18, 75
                times 4 dw 18, -50
                times 4 dw 75, -89

pb_unpackhlw1:  db 0,1,8,9,2,3,10,11,4,5,12,13,6,7,14,15

pb_idct8even:   db 0, 1, 8, 9, 4, 5, 12, 13, 0, 1, 8, 9, 4, 5, 12, 13

tab_idct8_1:    times 1 dw 64, -64, 36, -83, 64, 64, 83, 36

tab_idct8_2:    times 1 dw 89, 75, 50, 18, 75, -18, -89, -50
                times 1 dw 50, -89, 18, 75, 18, -50, 75, -89

pb_idct8odd:    db 2, 3, 6, 7, 10, 11, 14, 15, 2, 3, 6, 7, 10, 11, 14, 15

SECTION .text
cextern pd_1
cextern pd_2
cextern pd_4
cextern pd_8
cextern pd_16
cextern pd_32
cextern pd_64
cextern pd_128
cextern pd_256
cextern pd_512
cextern pd_2048
cextern pw_ppppmmmm

;------------------------------------------------------
;void dct4(int16_t *src, int32_t *dst, intptr_t stride)
;------------------------------------------------------
INIT_XMM sse2
cglobal dct4, 3, 4, 8
%if BIT_DEPTH == 10
  %define       DCT_SHIFT 3
  mova          m7, [pd_4]
%elif BIT_DEPTH == 8
  %define       DCT_SHIFT 1
  mova          m7, [pd_1]
%else
  %error Unsupported BIT_DEPTH!
%endif
    add         r2d, r2d
    lea         r3, [tab_dct4]

    mova        m4, [r3 + 0 * 16]
    mova        m5, [r3 + 1 * 16]
    mova        m6, [r3 + 2 * 16]
    movh        m0, [r0 + 0 * r2]
    movh        m1, [r0 + 1 * r2]
    punpcklqdq  m0, m1
    pshufd      m0, m0, 0xD8
    pshufhw     m0, m0, 0xB1

    lea         r0, [r0 + 2 * r2]
    movh        m1, [r0]
    movh        m2, [r0 + r2]
    punpcklqdq  m1, m2
    pshufd      m1, m1, 0xD8
    pshufhw     m1, m1, 0xB1

    punpcklqdq  m2, m0, m1
    punpckhqdq  m0, m1

    paddw       m1, m2, m0
    psubw       m2, m0
    pmaddwd     m0, m1, m4
    paddd       m0, m7
    psrad       m0, DCT_SHIFT
    pmaddwd     m3, m2, m5
    paddd       m3, m7
    psrad       m3, DCT_SHIFT
    packssdw    m0, m3
    pshufd      m0, m0, 0xD8
    pshufhw     m0, m0, 0xB1
    pmaddwd     m1, m6
    paddd       m1, m7
    psrad       m1, DCT_SHIFT
    pmaddwd     m2, [r3 + 3 * 16]
    paddd       m2, m7
    psrad       m2, DCT_SHIFT
    packssdw    m1, m2
    pshufd      m1, m1, 0xD8
    pshufhw     m1, m1, 0xB1

    punpcklqdq  m2, m0, m1
    punpckhqdq  m0, m1

    mova        m7, [pd_128]

    pmaddwd     m1, m2, m4
    pmaddwd     m3, m0, m4
    paddd       m1, m3
    paddd       m1, m7
    psrad       m1, 8
    movu        [r1 + 0 * 16], m1

    pmaddwd     m1, m2, m5
    pmaddwd     m3, m0, m5
    psubd       m1, m3
    paddd       m1, m7
    psrad       m1, 8
    movu        [r1 + 1 * 16], m1

    pmaddwd     m1, m2, m6
    pmaddwd     m3, m0, m6
    paddd       m1, m3
    paddd       m1, m7
    psrad       m1, 8
    movu        [r1 + 2 * 16], m1

    pmaddwd     m2, [r3 + 3 * 16]
    pmaddwd     m0, [r3 + 3 * 16]
    psubd       m2, m0
    paddd       m2, m7
    psrad       m2, 8
    movu        [r1 + 3 * 16], m2
    RET

; DCT 4x4
;
; Input parameters:
; - r0:     source
; - r1:     destination
; - r2:     source stride
INIT_YMM avx2
cglobal dct4, 3, 4, 8, src, dst, srcStride
%if BIT_DEPTH == 10
    %define DCT_SHIFT 3
    vbroadcasti128 m7, [pd_4]
%elif BIT_DEPTH == 8
    %define DCT_SHIFT 1
    vbroadcasti128 m7, [pd_1]
%else
    %error Unsupported BIT_DEPTH!
%endif
    add             r2d, r2d
    lea             r3, [avx2_dct4]

    vbroadcasti128  m4, [dct4_shuf]
    mova            m5, [r3]
    mova            m6, [r3 + 32]
    movq            xm0, [r0]
    movhps          xm0, [r0 + r2]
    lea             r0, [r0 + 2 * r2]
    movq            xm1, [r0]
    movhps          xm1, [r0 + r2]

    vinserti128     m0, m0, xm1, 1
    pshufb          m0, m4
    vpermq          m1, m0, 11011101b
    vpermq          m0, m0, 10001000b
    paddw           m2, m0, m1
    psubw           m0, m1

    pmaddwd         m2, m5
    paddd           m2, m7
    psrad           m2, DCT_SHIFT

    pmaddwd         m0, m6
    paddd           m0, m7
    psrad           m0, DCT_SHIFT

    packssdw        m2, m0
    pshufb          m2, m4
    vpermq          m1, m2, 11011101b
    vpermq          m2, m2, 10001000b
    vbroadcasti128  m7, [pd_128]

    pmaddwd         m0, m2, m5
    pmaddwd         m3, m1, m5
    paddd           m3, m0
    paddd           m3, m7
    psrad           m3, 8

    pmaddwd         m2, m6
    pmaddwd         m1, m6
    psubd           m2, m1
    paddd           m2, m7
    psrad           m2, 8

    movu            [r1], xm3
    movu            [r1 + mmsize/2], m2
    vextracti128    [r1 + mmsize], m3, 1
    vextracti128    [r1 + mmsize + mmsize/2], m2, 1
    RET

;-------------------------------------------------------
;void idct4(int32_t *src, int16_t *dst, intptr_t stride)
;-------------------------------------------------------
INIT_XMM sse2
cglobal idct4, 3, 4, 7
%if BIT_DEPTH == 8
  %define IDCT4_OFFSET  [pd_2048]
  %define IDCT4_SHIFT   12
%elif BIT_DEPTH == 10
  %define IDCT4_OFFSET  [pd_512]
  %define IDCT4_SHIFT   10
%else
  %error Unsupported BIT_DEPTH!
%endif
    add         r2d, r2d
    lea         r3, [tab_dct4]

    mova        m6, [pd_64]

    movu        m0, [r0 + 0 * 16]
    movu        m1, [r0 + 1 * 16]
    packssdw    m0, m1

    movu        m1, [r0 + 2 * 16]
    movu        m2, [r0 + 3 * 16]
    packssdw    m1, m2

    punpcklwd   m2, m0, m1
    pmaddwd     m3, m2, [r3 + 0 * 16]       ; m3 = E1
    paddd       m3, m6

    pmaddwd     m2, [r3 + 2 * 16]           ; m2 = E2
    paddd       m2, m6

    punpckhwd   m0, m1
    pmaddwd     m1, m0, [r3 + 1 * 16]       ; m1 = O1
    pmaddwd     m0, [r3 + 3 * 16]           ; m0 = O2

    paddd       m4, m3, m1
    psrad       m4, 7                       ; m4 = m128iA
    paddd       m5, m2, m0
    psrad       m5, 7
    packssdw    m4, m5                      ; m4 = m128iA

    psubd       m2, m0
    psrad       m2, 7
    psubd       m3, m1
    psrad       m3, 7
    packssdw    m2, m3                      ; m2 = m128iD

    punpcklwd   m1, m4, m2                  ; m1 = S0
    punpckhwd   m4, m2                      ; m4 = S8

    punpcklwd   m0, m1, m4                  ; m0 = m128iA
    punpckhwd   m1, m4                      ; m1 = m128iD

    mova        m6, IDCT4_OFFSET

    punpcklwd   m2, m0, m1
    pmaddwd     m3, m2, [r3 + 0 * 16]
    paddd       m3, m6                      ; m3 = E1

    pmaddwd     m2, [r3 + 2 * 16]
    paddd       m2, m6                      ; m2 = E2

    punpckhwd   m0, m1
    pmaddwd     m1, m0, [r3 + 1 * 16]       ; m1 = O1
    pmaddwd     m0, [r3 + 3 * 16]           ; m0 = O2

    paddd       m4, m3, m1
    psrad       m4, IDCT4_SHIFT             ; m4 = m128iA
    paddd       m5, m2, m0
    psrad       m5, IDCT4_SHIFT
    packssdw    m4, m5                      ; m4 = m128iA

    psubd       m2, m0
    psrad       m2, IDCT4_SHIFT
    psubd       m3, m1
    psrad       m3, IDCT4_SHIFT
    packssdw    m2, m3                      ; m2 = m128iD

    punpcklwd   m1, m4, m2
    punpckhwd   m4, m2

    punpcklwd   m0, m1, m4
    movlps      [r1 + 0 * r2], m0
    movhps      [r1 + 1 * r2], m0

    punpckhwd   m1, m4
    movlps      [r1 + 2 * r2], m1
    lea         r1, [r1 + 2 * r2]
    movhps      [r1 + r2], m1

    RET

;------------------------------------------------------
;void dst4(int16_t *src, int32_t *dst, intptr_t stride)
;------------------------------------------------------
INIT_XMM ssse3
%if ARCH_X86_64
cglobal dst4, 3, 4, 8+2
  %define       coef2   m8
  %define       coef3   m9
%else ; ARCH_X86_64 = 0
cglobal dst4, 3, 4, 8
  %define       coef2   [r3 + 2 * 16]
  %define       coef3   [r3 + 3 * 16]
%endif ; ARCH_X86_64
%define         coef0   m6
%define         coef1   m7

%if BIT_DEPTH == 8
  %define       DST_SHIFT 1
  mova          m5, [pd_1]
%elif BIT_DEPTH == 10
  %define       DST_SHIFT 3
  mova          m5, [pd_4]
%endif
    add         r2d, r2d
    lea         r3, [tab_dst4]
    mova        coef0, [r3 + 0 * 16]
    mova        coef1, [r3 + 1 * 16]
%if ARCH_X86_64
    mova        coef2, [r3 + 2 * 16]
    mova        coef3, [r3 + 3 * 16]
%endif
    movh        m0, [r0 + 0 * r2]            ; load
    movh        m1, [r0 + 1 * r2]
    punpcklqdq  m0, m1
    lea         r0, [r0 + 2 * r2]
    movh        m1, [r0]
    movh        m2, [r0 + r2]
    punpcklqdq  m1, m2
    pmaddwd     m2, m0, coef0                ; DST1
    pmaddwd     m3, m1, coef0
    phaddd      m2, m3
    paddd       m2, m5
    psrad       m2, DST_SHIFT
    pmaddwd     m3, m0, coef1
    pmaddwd     m4, m1, coef1
    phaddd      m3, m4
    paddd       m3, m5
    psrad       m3, DST_SHIFT
    packssdw    m2, m3                       ; m2 = T70
    pmaddwd     m3, m0, coef2
    pmaddwd     m4, m1, coef2
    phaddd      m3, m4
    paddd       m3, m5
    psrad       m3, DST_SHIFT
    pmaddwd     m0, coef3
    pmaddwd     m1, coef3
    phaddd      m0, m1
    paddd       m0, m5
    psrad       m0, DST_SHIFT
    packssdw    m3, m0                       ; m3 = T71
    mova        m5, [pd_128]

    pmaddwd     m0, m2, coef0                ; DST2
    pmaddwd     m1, m3, coef0
    phaddd      m0, m1
    paddd       m0, m5
    psrad       m0, 8
    movu        [r1 + 0 * 16], m0

    pmaddwd     m0, m2, coef1
    pmaddwd     m1, m3, coef1
    phaddd      m0, m1
    paddd       m0, m5
    psrad       m0, 8
    movu        [r1 + 1 * 16], m0

    pmaddwd     m0, m2, coef2
    pmaddwd     m1, m3, coef2
    phaddd      m0, m1
    paddd       m0, m5
    psrad       m0, 8
    movu        [r1 + 2 * 16], m0

    pmaddwd     m2, coef3
    pmaddwd     m3, coef3
    phaddd      m2, m3
    paddd       m2, m5
    psrad       m2, 8
    movu        [r1 + 3 * 16], m2

    RET

;-------------------------------------------------------
;void idst4(int32_t *src, int16_t *dst, intptr_t stride)
;-------------------------------------------------------
INIT_XMM sse2
cglobal idst4, 3, 4, 7
%if BIT_DEPTH == 8
  mova m6, [pd_2048]
  %define IDCT4_SHIFT 12
%elif BIT_DEPTH == 10
  mova m6, [pd_512]
  %define IDCT4_SHIFT 10
%else
  %error Unsupported BIT_DEPTH!
%endif
    add         r2d, r2d
    lea         r3, [tab_idst4]
    mova        m5, [pd_64]

    movu        m0, [r0 + 0 * 16]
    movu        m1, [r0 + 1 * 16]
    packssdw    m0, m1

    movu        m1, [r0 + 2 * 16]
    movu        m2, [r0 + 3 * 16]
    packssdw    m1, m2

    punpcklwd   m2, m0, m1                  ; m2 = m128iAC
    punpckhwd   m0, m1                      ; m0 = m128iBD

    pmaddwd     m1, m2, [r3 + 0 * 16]
    pmaddwd     m3, m0, [r3 + 1 * 16]
    paddd       m1, m3
    paddd       m1, m5
    psrad       m1, 7                       ; m1 = S0

    pmaddwd     m3, m2, [r3 + 2 * 16]
    pmaddwd     m4, m0, [r3 + 3 * 16]
    paddd       m3, m4
    paddd       m3, m5
    psrad       m3, 7                       ; m3 = S8
    packssdw    m1, m3                      ; m1 = m128iA

    pmaddwd     m3, m2, [r3 + 4 * 16]
    pmaddwd     m4, m0, [r3 + 5 * 16]
    paddd       m3, m4
    paddd       m3, m5
    psrad       m3, 7                       ; m3 = S0

    pmaddwd     m2, [r3 + 6 * 16]
    pmaddwd     m0, [r3 + 7 * 16]
    paddd       m2, m0
    paddd       m2, m5
    psrad       m2, 7                       ; m2 = S8
    packssdw    m3, m2                      ; m3 = m128iD

    punpcklwd   m0, m1, m3
    punpckhwd   m1, m3

    punpcklwd   m2, m0, m1
    punpckhwd   m0, m1
    punpcklwd   m1, m2, m0
    punpckhwd   m2, m0
    pmaddwd     m0, m1, [r3 + 0 * 16]
    pmaddwd     m3, m2, [r3 + 1 * 16]
    paddd       m0, m3
    paddd       m0, m6
    psrad       m0, IDCT4_SHIFT             ; m0 = S0
    pmaddwd     m3, m1, [r3 + 2 * 16]
    pmaddwd     m4, m2, [r3 + 3 * 16]
    paddd       m3, m4
    paddd       m3, m6
    psrad       m3, IDCT4_SHIFT             ; m3 = S8
    packssdw    m0, m3                      ; m0 = m128iA
    pmaddwd     m3, m1, [r3 + 4 * 16]
    pmaddwd     m4, m2, [r3 + 5 * 16]
    paddd       m3, m4
    paddd       m3, m6
    psrad       m3, IDCT4_SHIFT             ; m3 = S0
    pmaddwd     m1, [r3 + 6 * 16]
    pmaddwd     m2, [r3 + 7 * 16]
    paddd       m1, m2
    paddd       m1, m6
    psrad       m1, IDCT4_SHIFT             ; m1 = S8
    packssdw    m3, m1                      ; m3 = m128iD
    punpcklwd   m1, m0, m3
    punpckhwd   m0, m3

    punpcklwd   m2, m1, m0
    movlps      [r1 + 0 * r2], m2
    movhps      [r1 + 1 * r2], m2

    punpckhwd   m1, m0
    movlps      [r1 + 2 * r2], m1
    lea         r1, [r1 + 2 * r2]
    movhps      [r1 + r2], m1
    RET


;-------------------------------------------------------
; void dct8(int16_t *src, int32_t *dst, intptr_t stride)
;-------------------------------------------------------
INIT_XMM sse4
cglobal dct8, 3,6,7,0-16*mmsize
    ;------------------------
    ; Stack Mapping(dword)
    ;------------------------
    ; Row0[0-3] Row1[0-3]
    ; ...
    ; Row6[0-3] Row7[0-3]
    ; Row0[0-3] Row7[0-3]
    ; ...
    ; Row6[4-7] Row7[4-7]
    ;------------------------
%if BIT_DEPTH == 10
  %define       DCT_SHIFT 4
  mova          m6, [pd_8]
%elif BIT_DEPTH == 8
  %define       DCT_SHIFT 2
  mova          m6, [pd_2]
%else
  %error Unsupported BIT_DEPTH!
%endif

    add         r2, r2
    lea         r3, [r2 * 3]
    mov         r5, rsp
%assign x 0
%rep 2
    movu        m0, [r0]
    movu        m1, [r0 + r2]
    movu        m2, [r0 + r2 * 2]
    movu        m3, [r0 + r3]

    punpcklwd   m4, m0, m1
    punpckhwd   m0, m1
    punpcklwd   m5, m2, m3
    punpckhwd   m2, m3
    punpckldq   m1, m4, m5          ; m1 = [1 0]
    punpckhdq   m4, m5              ; m4 = [3 2]
    punpckldq   m3, m0, m2
    punpckhdq   m0, m2
    pshufd      m2, m3, 0x4E        ; m2 = [4 5]
    pshufd      m0, m0, 0x4E        ; m0 = [6 7]

    paddw       m3, m1, m0
    psubw       m1, m0              ; m1 = [d1 d0]
    paddw       m0, m4, m2
    psubw       m4, m2              ; m4 = [d3 d2]
    punpcklqdq  m2, m3, m0          ; m2 = [s2 s0]
    punpckhqdq  m3, m0
    pshufd      m3, m3, 0x4E        ; m3 = [s1 s3]

    punpcklwd   m0, m1, m4          ; m0 = [d2/d0]
    punpckhwd   m1, m4              ; m1 = [d3/d1]
    punpckldq   m4, m0, m1          ; m4 = [d3 d1 d2 d0]
    punpckhdq   m0, m1              ; m0 = [d3 d1 d2 d0]

    ; odd
    lea         r4, [tab_dct8_1]
    pmaddwd     m1, m4, [r4 + 0*16]
    pmaddwd     m5, m0, [r4 + 0*16]
    phaddd      m1, m5
    paddd       m1, m6
    psrad       m1, DCT_SHIFT
  %if x == 1
    pshufd      m1, m1, 0x1B
  %endif
    mova        [r5 + 1*2*mmsize], m1 ; Row 1

    pmaddwd     m1, m4, [r4 + 1*16]
    pmaddwd     m5, m0, [r4 + 1*16]
    phaddd      m1, m5
    paddd       m1, m6
    psrad       m1, DCT_SHIFT
  %if x == 1
    pshufd      m1, m1, 0x1B
  %endif
    mova        [r5 + 3*2*mmsize], m1 ; Row 3

    pmaddwd     m1, m4, [r4 + 2*16]
    pmaddwd     m5, m0, [r4 + 2*16]
    phaddd      m1, m5
    paddd       m1, m6
    psrad       m1, DCT_SHIFT
  %if x == 1
    pshufd      m1, m1, 0x1B
  %endif
    mova        [r5 + 5*2*mmsize], m1 ; Row 5

    pmaddwd     m4, [r4 + 3*16]
    pmaddwd     m0, [r4 + 3*16]
    phaddd      m4, m0
    paddd       m4, m6
    psrad       m4, DCT_SHIFT
  %if x == 1
    pshufd      m4, m4, 0x1B
  %endif
    mova        [r5 + 7*2*mmsize], m4; Row 7

    ; even
    lea         r4, [tab_dct4]
    paddw       m0, m2, m3          ; m0 = [EE1 EE0]
    pshufb      m0, [pb_unpackhlw1]
    psubw       m2, m3              ; m2 = [EO1 EO0]
    psignw      m2, [pw_ppppmmmm]
    pshufb      m2, [pb_unpackhlw1]
    pmaddwd     m3, m0, [r4 + 0*16]
    paddd       m3, m6
    psrad       m3, DCT_SHIFT
  %if x == 1
    pshufd      m3, m3, 0x1B
  %endif
    mova        [r5 + 0*2*mmsize], m3 ; Row 0
    pmaddwd     m0, [r4 + 2*16]
    paddd       m0, m6
    psrad       m0, DCT_SHIFT
  %if x == 1
    pshufd      m0, m0, 0x1B
  %endif
    mova        [r5 + 4*2*mmsize], m0 ; Row 4
    pmaddwd     m3, m2, [r4 + 1*16]
    paddd       m3, m6
    psrad       m3, DCT_SHIFT
  %if x == 1
    pshufd      m3, m3, 0x1B
  %endif
    mova        [r5 + 2*2*mmsize], m3 ; Row 2
    pmaddwd     m2, [r4 + 3*16]
    paddd       m2, m6
    psrad       m2, DCT_SHIFT
  %if x == 1
    pshufd      m2, m2, 0x1B
  %endif
    mova        [r5 + 6*2*mmsize], m2 ; Row 6

  %if x != 1
    lea         r0, [r0 + r2 * 4]
    add         r5, mmsize
  %endif
%assign x x+1
%endrep

    mov         r2, 2
    mov         r0, rsp                 ; r0 = pointer to Low Part
    lea         r4, [tab_dct8_2]
    mova        m6, [pd_256]

.pass2:
%rep 2
    mova        m0, [r0 + 0*2*mmsize]     ; [3 2 1 0]
    mova        m1, [r0 + 1*2*mmsize]
    paddd       m2, m0, [r0 + (0*2+1)*mmsize]
    pshufd      m2, m2, 0x9C            ; m2 = [s2 s1 s3 s0]
    paddd       m3, m1, [r0 + (1*2+1)*mmsize]
    pshufd      m3, m3, 0x9C            ; m3 = ^^
    psubd       m0, [r0 + (0*2+1)*mmsize]     ; m0 = [d3 d2 d1 d0]
    psubd       m1, [r0 + (1*2+1)*mmsize]     ; m1 = ^^

    ; even
    phaddd      m4, m2, m3              ; m4 = [EE1 EE0 EE1 EE0]
    phsubd      m2, m3                  ; m2 = [EO1 EO0 EO1 EO0]

    pslld       m4, 6                   ; m4 = [64*EE1 64*EE0]
    pmulld      m5, m2, [r4 + 0*16]     ; m5 = [36*EO1 83*EO0]
    pmulld      m2, [r4 + 1*16]         ; m2 = [83*EO1 36*EO0]

    phaddd      m3, m4, m5              ; m3 = [Row2 Row0]
    paddd       m3, m6
    psrad       m3, 9
    phsubd      m4, m2                  ; m4 = [Row6 Row4]
    paddd       m4, m6
    psrad       m4, 9
    movh        [r1 + 0*2*mmsize], m3
    movhps      [r1 + 2*2*mmsize], m3
    movh        [r1 + 4*2*mmsize], m4
    movhps      [r1 + 6*2*mmsize], m4

    ; odd
    pmulld      m2, m0, [r4 + 2*16]
    pmulld      m3, m1, [r4 + 2*16]
    pmulld      m4, m0, [r4 + 3*16]
    pmulld      m5, m1, [r4 + 3*16]
    phaddd      m2, m3
    phaddd      m4, m5
    phaddd      m2, m4                  ; m2 = [Row3 Row1]
    paddd       m2, m6
    psrad       m2, 9
    movh        [r1 + 1*2*mmsize], m2
    movhps      [r1 + 3*2*mmsize], m2

    pmulld      m2, m0, [r4 + 4*16]
    pmulld      m3, m1, [r4 + 4*16]
    pmulld      m4, m0, [r4 + 5*16]
    pmulld      m5, m1, [r4 + 5*16]
    phaddd      m2, m3
    phaddd      m4, m5
    phaddd      m2, m4                  ; m2 = [Row7 Row5]
    paddd       m2, m6
    psrad       m2, 9
    movh        [r1 + 5*2*mmsize], m2
    movhps      [r1 + 7*2*mmsize], m2

    add         r1, mmsize/2
    add         r0, 2*2*mmsize
%endrep

    dec         r2
    jnz        .pass2
    RET

;-------------------------------------------------------
; void idct8(int32_t *src, int16_t *dst, intptr_t stride)
;-------------------------------------------------------
INIT_XMM ssse3

cglobal patial_butterfly_inverse_internal_pass1
    movu        m0, [r0]
    movu        m1, [r0 + 4 * 32]
    movu        m2, [r0 + 2 * 32]
    movu        m3, [r0 + 6 * 32]
    packssdw    m0, m2
    packssdw    m1, m3
    punpckhwd   m2, m0, m1                  ; [2 6]
    punpcklwd   m0, m1                      ; [0 4]
    pmaddwd     m1, m0, [r6]                ; EE[0]
    pmaddwd     m0, [r6 + 32]               ; EE[1]
    pmaddwd     m3, m2, [r6 + 16]           ; EO[0]
    pmaddwd     m2, [r6 + 48]               ; EO[1]

    paddd       m4, m1, m3                  ; E[0]
    psubd       m1, m3                      ; E[3]
    paddd       m3, m0, m2                  ; E[1]
    psubd       m0, m2                      ; E[2]

    ;E[K] = E[k] + add
    mova        m5, [pd_64]
    paddd       m0, m5
    paddd       m1, m5
    paddd       m3, m5
    paddd       m4, m5

    movu        m2, [r0 + 32]
    movu        m5, [r0 + 5 * 32]
    packssdw    m2, m5
    movu        m5, [r0 + 3 * 32]
    movu        m6, [r0 + 7 * 32]
    packssdw    m5, m6
    punpcklwd   m6, m2, m5                  ;[1 3]
    punpckhwd   m2, m5                      ;[5 7]

    pmaddwd     m5, m6, [r4]
    pmaddwd     m7, m2, [r4 + 16]
    paddd       m5, m7                      ; O[0]

    paddd       m7, m4, m5
    psrad       m7, 7

    psubd       m4, m5
    psrad       m4, 7

    packssdw    m7, m4
    movh        [r5 + 0 * 16], m7
    movhps      [r5 + 7 * 16], m7

    pmaddwd     m5, m6, [r4 + 32]
    pmaddwd     m4, m2, [r4 + 48]
    paddd       m5, m4                      ; O[1]

    paddd       m4, m3, m5
    psrad       m4, 7

    psubd       m3, m5
    psrad       m3, 7

    packssdw    m4, m3
    movh        [r5 + 1 * 16], m4
    movhps      [r5 + 6 * 16], m4

    pmaddwd     m5, m6, [r4 + 64]
    pmaddwd     m4, m2, [r4 + 80]
    paddd       m5, m4                      ; O[2]

    paddd       m4, m0, m5
    psrad       m4, 7

    psubd       m0, m5
    psrad       m0, 7

    packssdw    m4, m0
    movh        [r5 + 2 * 16], m4
    movhps      [r5 + 5 * 16], m4

    pmaddwd     m5, m6, [r4 + 96]
    pmaddwd     m4, m2, [r4 + 112]
    paddd       m5, m4                      ; O[3]

    paddd       m4, m1, m5
    psrad       m4, 7

    psubd       m1, m5
    psrad       m1, 7

    packssdw    m4, m1
    movh        [r5 + 3 * 16], m4
    movhps      [r5 + 4 * 16], m4

    ret

%macro PARTIAL_BUTTERFLY_PROCESS_ROW 1
%if BIT_DEPTH == 10
    %define     IDCT_SHIFT 10
%elif BIT_DEPTH == 8
    %define     IDCT_SHIFT 12
%else
    %error Unsupported BIT_DEPTH!
%endif
    pshufb      m4, %1, [pb_idct8even]
    pmaddwd     m4, [tab_idct8_1]
    phsubd      m5, m4
    pshufd      m4, m4, 0x4E
    phaddd      m4, m4
    punpckhqdq  m4, m5                      ;m4 = dd e[ 0 1 2 3]
    paddd       m4, m6

    pshufb      %1, %1, [r6]
    pmaddwd     m5, %1, [r4]
    pmaddwd     %1, [r4 + 16]
    phaddd      m5, %1                      ; m5 = dd O[0, 1, 2, 3]

    paddd       %1, m4, m5
    psrad       %1, IDCT_SHIFT

    psubd       m4, m5
    psrad       m4, IDCT_SHIFT
    pshufd      m4, m4, 0x1B

    packssdw    %1, m4
%undef IDCT_SHIFT
%endmacro

cglobal patial_butterfly_inverse_internal_pass2

    mova        m0, [r5]
    PARTIAL_BUTTERFLY_PROCESS_ROW m0
    movu        [r1], m0

    mova        m2, [r5 + 16]
    PARTIAL_BUTTERFLY_PROCESS_ROW m2
    movu        [r1 + r2], m2

    mova        m1, [r5 + 32]
    PARTIAL_BUTTERFLY_PROCESS_ROW m1
    movu        [r1 + 2 * r2], m1

    mova        m3, [r5 + 48]
    PARTIAL_BUTTERFLY_PROCESS_ROW m3
    movu        [r1 + r3], m3

    ret

cglobal idct8, 3,7,8 ;,0-16*mmsize
    ; alignment stack to 64-bytes
    mov         r5, rsp
    sub         rsp, 16*mmsize + gprsize
    and         rsp, ~(64-1)
    mov         [rsp + 16*mmsize], r5
    mov         r5, rsp

    lea         r4, [tab_idct8_3]
    lea         r6, [tab_dct4]

    call        patial_butterfly_inverse_internal_pass1

    add         r0, 16
    add         r5, 8

    call        patial_butterfly_inverse_internal_pass1

%if BIT_DEPTH == 10
    mova        m6, [pd_512]
%elif BIT_DEPTH == 8
    mova        m6, [pd_2048]
%else
  %error Unsupported BIT_DEPTH!
%endif
    add         r2, r2
    lea         r3, [r2 * 3]
    lea         r4, [tab_idct8_2]
    lea         r6, [pb_idct8odd]
    sub         r5, 8

    call        patial_butterfly_inverse_internal_pass2

    lea         r1, [r1 + 4 * r2]
    add         r5, 64

    call        patial_butterfly_inverse_internal_pass2

    ; restore origin stack pointer
    mov         rsp, [rsp + 16*mmsize]
    RET


; TODO: split into two version after coeff_t changed
%if 1 ;HIGH_BIT_DEPTH
;-----------------------------------------------------------------------------
; void denoise_dct( int32_t *dct, uint32_t *sum, uint32_t *offset, int size )
;-----------------------------------------------------------------------------
%macro DENOISE_DCT 0
cglobal denoise_dct, 4,4,6
    pxor      m5, m5
    movsxdifnidn r3, r3d
.loop:
    mova      m2, [r0+r3*4-2*mmsize]
    mova      m3, [r0+r3*4-1*mmsize]
    ABSD      m0, m2
    ABSD      m1, m3
    paddd     m4, m0, [r1+r3*4-2*mmsize]
    psubd     m0, [r2+r3*4-2*mmsize]
    mova      [r1+r3*4-2*mmsize], m4
    paddd     m4, m1, [r1+r3*4-1*mmsize]
    psubd     m1, [r2+r3*4-1*mmsize]
    mova      [r1+r3*4-1*mmsize], m4
    pcmpgtd   m4, m0, m5
    pand      m0, m4
    pcmpgtd   m4, m1, m5
    pand      m1, m4
    PSIGND    m0, m2
    PSIGND    m1, m3
    mova      [r0+r3*4-2*mmsize], m0
    mova      [r0+r3*4-1*mmsize], m1
    sub      r3d, mmsize/2
    jg .loop
    RET
%endmacro

%if ARCH_X86_64 == 0
INIT_MMX mmx
DENOISE_DCT
%endif
INIT_XMM sse2
DENOISE_DCT
INIT_XMM ssse3
DENOISE_DCT
INIT_XMM avx
DENOISE_DCT
INIT_YMM avx2
DENOISE_DCT

%else ; !HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; void denoise_dct( int16_t *dct, uint32_t *sum, uint16_t *offset, int size )
;-----------------------------------------------------------------------------
%macro DENOISE_DCT 0
cglobal denoise_dct, 4,4,7
    pxor      m6, m6
    movsxdifnidn r3, r3d
.loop:
    mova      m2, [r0+r3*2-2*mmsize]
    mova      m3, [r0+r3*2-1*mmsize]
    ABSW      m0, m2, sign
    ABSW      m1, m3, sign
    psubusw   m4, m0, [r2+r3*2-2*mmsize]
    psubusw   m5, m1, [r2+r3*2-1*mmsize]
    PSIGNW    m4, m2
    PSIGNW    m5, m3
    mova      [r0+r3*2-2*mmsize], m4
    mova      [r0+r3*2-1*mmsize], m5
    punpcklwd m2, m0, m6
    punpcklwd m3, m1, m6
    punpckhwd m0, m6
    punpckhwd m1, m6
    paddd     m2, [r1+r3*4-4*mmsize]
    paddd     m0, [r1+r3*4-3*mmsize]
    paddd     m3, [r1+r3*4-2*mmsize]
    paddd     m1, [r1+r3*4-1*mmsize]
    mova      [r1+r3*4-4*mmsize], m2
    mova      [r1+r3*4-3*mmsize], m0
    mova      [r1+r3*4-2*mmsize], m3
    mova      [r1+r3*4-1*mmsize], m1
    sub       r3, mmsize
    jg .loop
%if (mmsize == 8)
    EMMS
%endif
    RET
%endmacro

%if ARCH_X86_64 == 0
INIT_MMX mmx
DENOISE_DCT
%endif
INIT_XMM sse2
DENOISE_DCT
INIT_XMM ssse3
DENOISE_DCT
INIT_XMM avx
DENOISE_DCT

INIT_YMM avx2
cglobal denoise_dct, 4,4,4
    pxor      m3, m3
    movsxdifnidn r3, r3d
.loop:
    mova      m1, [r0+r3*2-mmsize]
    pabsw     m0, m1
    psubusw   m2, m0, [r2+r3*2-mmsize]
    vpermq    m0, m0, q3120
    psignw    m2, m1
    mova [r0+r3*2-mmsize], m2
    punpcklwd m1, m0, m3
    punpckhwd m0, m3
    paddd     m1, [r1+r3*4-2*mmsize]
    paddd     m0, [r1+r3*4-1*mmsize]
    mova      [r1+r3*4-2*mmsize], m1
    mova      [r1+r3*4-1*mmsize], m0
    sub       r3, mmsize/2
    jg .loop
    RET

%endif ; !HIGH_BIT_DEPTH

%macro DCT16_PASS_1_E 2
    vpbroadcastq    m7,                [r7 + %1]

    pmaddwd         m4,                m0, m7
    pmaddwd         m6,                m2, m7
    phaddd          m4,                m6

    paddd           m4,                m9
    psrad           m4,                DCT_SHIFT

    packssdw        m4,                m4
    vpermq          m4,                m4, 0x08

    mova            [r5 + %2],         xm4
%endmacro

%macro DCT16_PASS_1_O 2
    vbroadcasti128  m7,                [r7 + %1]

    pmaddwd         m10,               m0, m7
    pmaddwd         m11,               m2, m7
    phaddd          m10,               m11                 ; [d0 d0 d1 d1 d4 d4 d5 d5]

    pmaddwd         m11,               m4, m7
    pmaddwd         m12,               m6, m7
    phaddd          m11,               m12                 ; [d2 d2 d3 d3 d6 d6 d7 d7]

    phaddd          m10,               m11                 ; [d0 d1 d2 d3 d4 d5 d6 d7]

    paddd           m10,               m9
    psrad           m10,               DCT_SHIFT

    packssdw        m10,               m10                 ; [w0 w1 w2 w3 - - - - w4 w5 w6 w7 - - - -]
    vpermq          m10,               m10, 0x08

    mova            [r5 + %2],         xm10
%endmacro

%macro DCT16_PASS_2 1
    vbroadcasti128  m8,                [r7 + %1]
    vbroadcasti128  m13,               [r8 + %1]

    pmaddwd         m10,               m0, m8
    pmaddwd         m11,               m1, m13
    paddd           m10,               m11

    pmaddwd         m11,               m2, m8
    pmaddwd         m12,               m3, m13
    paddd           m11,               m12
    phaddd          m10,               m11

    pmaddwd         m11,               m4, m8
    pmaddwd         m12,               m5, m13
    paddd           m11,               m12

    pmaddwd         m12,               m6, m8
    pmaddwd         m13,               m7, m13
    paddd           m12,               m13
    phaddd          m11,               m12

    phaddd          m10,               m11
    paddd           m10,               m9
    psrad           m10,               DCT_SHIFT2
%endmacro

%if ARCH_X86_64 == 1
INIT_YMM avx2
cglobal dct16, 3, 9, 15, 0-16*mmsize
%if BIT_DEPTH == 10
    %define         DCT_SHIFT          5
    vpbroadcastd    m9,                [pd_16]
%elif BIT_DEPTH == 8
    %define         DCT_SHIFT          3
    vpbroadcastd    m9,                [pd_4]
%else
    %error Unsupported BIT_DEPTH!
%endif
%define             DCT_SHIFT2         10

    add             r2d,               r2d

    mova            m13,               [dct16_shuf1]
    mova            m14,               [dct16_shuf2]
    lea             r7,                [tab_dct16_1 + 8 * 16]
    lea             r8,                [tab_dct16_2 + 8 * 16]
    lea             r3,                [r2 * 3]
    mov             r5,                rsp
    mov             r4d,               2                   ; Each iteration process 8 rows, so 16/8 iterations

.pass1:
    lea             r6,                [r0 + r2 * 4]

    mova            m2,                [r0]
    mova            m1,                [r6]
    vperm2i128      m0,                m2, m1, 0x20        ; [row0lo  row4lo]
    vperm2i128      m1,                m2, m1, 0x31        ; [row0hi  row4hi]

    mova            m4,                [r0 + r2]
    mova            m3,                [r6 + r2]
    vperm2i128      m2,                m4, m3, 0x20        ; [row1lo  row5lo]
    vperm2i128      m3,                m4, m3, 0x31        ; [row1hi  row5hi]

    mova            m6,                [r0 + r2 * 2]
    mova            m5,                [r6 + r2 * 2]
    vperm2i128      m4,                m6, m5, 0x20        ; [row2lo  row6lo]
    vperm2i128      m5,                m6, m5, 0x31        ; [row2hi  row6hi]

    mova            m8,                [r0 + r3]
    mova            m7,                [r6 + r3]
    vperm2i128      m6,                m8, m7, 0x20        ; [row3lo  row7lo]
    vperm2i128      m7,                m8, m7, 0x31        ; [row3hi  row7hi]

    pshufb          m1,                m13
    pshufb          m3,                m13
    pshufb          m5,                m13
    pshufb          m7,                m13

    paddw           m8,                m0, m1              ;E
    psubw           m0,                m1                  ;O

    paddw           m1,                m2, m3              ;E
    psubw           m2,                m3                  ;O

    paddw           m3,                m4, m5              ;E
    psubw           m4,                m5                  ;O

    paddw           m5,                m6, m7              ;E
    psubw           m6,                m7                  ;O

    DCT16_PASS_1_O  -7 * 16,           1 * 32
    DCT16_PASS_1_O  -5 * 16,           3 * 32
    DCT16_PASS_1_O  -3 * 16,           1 * 32 + 16
    DCT16_PASS_1_O  -1 * 16,           3 * 32 + 16
    DCT16_PASS_1_O  1 * 16,            5 * 32
    DCT16_PASS_1_O  3 * 16,            7 * 32
    DCT16_PASS_1_O  5 * 16,            5 * 32 + 16
    DCT16_PASS_1_O  7 * 16,            7 * 32 + 16

    pshufb          m8,                m14
    pshufb          m1,                m14
    phaddw          m0,                m8, m1

    pshufb          m3,                m14
    pshufb          m5,                m14
    phaddw          m2,                m3, m5

    DCT16_PASS_1_E  -8 * 16,           0 * 32
    DCT16_PASS_1_E  -4 * 16,           0 * 32 + 16
    DCT16_PASS_1_E  0 * 16,            4 * 32
    DCT16_PASS_1_E  4 * 16,            4 * 32 + 16

    phsubw          m0,                m8, m1
    phsubw          m2,                m3, m5

    DCT16_PASS_1_E  -6 * 16,           2 * 32
    DCT16_PASS_1_E  -2 * 16,           2 * 32 + 16
    DCT16_PASS_1_E  2 * 16,            6 * 32
    DCT16_PASS_1_E  6 * 16,            6 * 32 + 16

    lea             r0,                [r0 + 8 * r2]
    add             r5,                256

    dec             r4d
    jnz             .pass1

    mov             r5,                rsp
    mov             r4d,               2
    add             r2d,               r2d
    lea             r3,                [r2 * 3]
    vpbroadcastd    m9,                [pd_512]

.pass2:
    mova            m0,                [r5 + 0 * 32]        ; [row0lo  row4lo]
    mova            m1,                [r5 + 8 * 32]        ; [row0hi  row4hi]

    mova            m2,                [r5 + 1 * 32]        ; [row1lo  row5lo]
    mova            m3,                [r5 + 9 * 32]        ; [row1hi  row5hi]

    mova            m4,                [r5 + 2 * 32]        ; [row2lo  row6lo]
    mova            m5,                [r5 + 10 * 32]       ; [row2hi  row6hi]

    mova            m6,                [r5 + 3 * 32]        ; [row3lo  row7lo]
    mova            m7,                [r5 + 11 * 32]       ; [row3hi  row7hi]

    DCT16_PASS_2    -8 * 16
    mova            [r1],              m10
    DCT16_PASS_2    -7 * 16
    mova            [r1 + r2],         m10
    DCT16_PASS_2    -6 * 16
    mova            [r1 + r2 * 2],     m10
    DCT16_PASS_2    -5 * 16
    mova            [r1 + r3],         m10

    lea             r6,                [r1 + r2 * 4]
    DCT16_PASS_2    -4 * 16
    mova            [r6],              m10
    DCT16_PASS_2    -3 * 16
    mova            [r6 + r2],         m10
    DCT16_PASS_2    -2 * 16
    mova            [r6 + r2 * 2],     m10
    DCT16_PASS_2    -1 * 16
    mova            [r6 + r3],         m10

    lea             r6,                [r6 + r2 * 4]
    DCT16_PASS_2    0 * 16
    mova            [r6],              m10
    DCT16_PASS_2    1 * 16
    mova            [r6 + r2],         m10
    DCT16_PASS_2    2 * 16
    mova            [r6 + r2 * 2],     m10
    DCT16_PASS_2    3 * 16
    mova            [r6 + r3],         m10

    lea             r6,                [r6 + r2 * 4]
    DCT16_PASS_2    4 * 16
    mova            [r6],              m10
    DCT16_PASS_2    5 * 16
    mova            [r6 + r2],         m10
    DCT16_PASS_2    6 * 16
    mova            [r6 + r2 * 2],     m10
    DCT16_PASS_2    7 * 16
    mova            [r6 + r3],         m10

    add             r1,                32
    add             r5,                128

    dec             r4d
    jnz             .pass2
    RET
%endif
