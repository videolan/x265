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
;* For more information, contact us at licensing@multicorewareinc.com.
;*****************************************************************************/

;TO-DO : Further optimize the routines.

%include "x86inc.asm"
%include "x86util.asm"

SECTION_RODATA 32

tab_dct4:       times 4 dw 64, 64
                times 4 dw 83, 36
                times 4 dw 64, -64
                times 4 dw 36, -83

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

pb_unpackhlw1:  db 0,1,8,9,2,3,10,11,4,5,12,13,6,7,14,15

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
