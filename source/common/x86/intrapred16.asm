;*****************************************************************************
;* Copyright (C) 2013 x265 project
;*
;* Authors: Dnyaneshwar Gorade <dnyaneshwar@multicorewareinc.com>
;*          Yuvaraj Venkatesh <yuvaraj@multicorewareinc.com>
;*          Min Chen <chenm003@163.com> <min.chen@multicorewareinc.com>
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

const ang_table
%assign x 0
%rep 32
    times 4 dw (32-x), x
%assign x x+1
%endrep

const pw_unpack0wd, times 4 db 0,1,8,8

SECTION .text

cextern pw_1
cextern pw_8
cextern pd_16
cextern pd_32
cextern pw_4096
cextern multiL
cextern multiH
cextern multi_2Row
cextern pw_swap
cextern pb_unpackwq1
cextern pb_unpackwq2

;-------------------------------------------------------------------------------------------------------
; void intra_pred_dc(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_dc4, 4,6,2
    mov         r4d,            r5m
    add         r2,             2
    add         r3,             2

    movh        m0,             [r3]           ; sumAbove
    movh        m1,             [r2]           ; sumLeft

    paddw       m0,             m1
    pshufd      m1,             m0, 1
    paddw       m0,             m1
    phaddw      m0,             m0             ; m0 = sum

    test        r4d,            r4d

    pmulhrsw    m0,             [pw_4096]      ; m0 = (sum + 4) / 8
    movd        r4d,            m0             ; r4d = dc_val
    movzx       r4d,            r4w
    pshuflw     m0,             m0, 0          ; m0 = word [dc_val ...]

    ; store DC 4x4
    movh        [r0],           m0
    movh        [r0 + r1 * 2],  m0
    movh        [r0 + r1 * 4],  m0
    lea         r5,             [r0 + r1 * 4]
    movh        [r5 + r1 * 2],  m0

    ; do DC filter
    jz          .end
    lea         r5d,            [r4d * 2 + 2]  ; r5d = DC * 2 + 2
    add         r4d,            r5d            ; r4d = DC * 3 + 2
    movd        m0,             r4d
    pshuflw     m0,             m0, 0          ; m0 = pixDCx3

    ; filter top
    movu        m1,             [r3]
    paddw       m1,             m0
    psraw       m1,             2
    movh        [r0],           m1             ; overwrite top-left pixel, we will update it later

    ; filter top-left
    movzx       r3d, word       [r3]
    add         r5d,            r3d
    movzx       r3d, word       [r2]
    add         r3d,            r5d
    shr         r3d,            2
    mov         [r0],           r3w

    ; filter left
    lea         r0,             [r0 + r1 * 2]
    movu        m1,             [r2 + 2]
    paddw       m1,             m0
    psraw       m1,             2
    movd        r3d,            m1
    mov         [r0],           r3w
    shr         r3d,            16
    mov         [r0 + r1 * 2],  r3w
    pextrw      [r0 + r1 * 4],  m1, 2

.end:

    RET



;-------------------------------------------------------------------------------------------------------
; void intra_pred_dc(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_dc8, 4, 7, 2
    mov             r4d,           r5m
    add             r2,            2
    add             r3,            2
    add             r1,            r1
    movu            m0,            [r3]
    movu            m1,            [r2]

    paddw           m0,            m1
    movhlps         m1,            m0
    paddw           m0,            m1
    phaddw          m0,            m0
    pmaddwd         m0,            [pw_1]

    movd            r5d,           m0
    add             r5d,           8
    shr             r5d,           4              ; sum = sum / 16
    movd            m1,            r5d
    pshuflw         m1,            m1, 0          ; m1 = word [dc_val ...]
    pshufd          m1,            m1, 0

    test            r4d,           r4d

    ; store DC 8x8
    mov             r6,            r0
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    movu            [r0 + r1 * 2], m1
    lea             r0,            [r0 + r1 * 2]
    movu            [r0 + r1],     m1
    movu            [r0 + r1 * 2], m1
    lea             r0,            [r0 + r1 * 2]
    movu            [r0 + r1],     m1
    movu            [r0 + r1 * 2], m1
    lea             r0,            [r0 + r1 * 2]
    movu            [r0 + r1],     m1

    ; Do DC Filter
    jz              .end
    lea             r4d,           [r5d * 2 + 2]  ; r4d = DC * 2 + 2
    add             r5d,           r4d            ; r5d = DC * 3 + 2
    movd            m1,            r5d
    pshuflw         m1,            m1, 0          ; m1 = pixDCx3
    pshufd          m1,            m1, 0

    ; filter top
    movu            m0,            [r3]
    paddw           m0,            m1
    psraw           m0,            2
    movu            [r6],          m0

    ; filter top-left
    movzx           r3d, word      [r3]
    add             r4d,           r3d
    movzx           r3d, word      [r2]
    add             r3d,           r4d
    shr             r3d,           2
    mov             [r6],          r3w

    ; filter left
    add             r6,            r1
    movu            m0,            [r2 + 2]
    paddw           m0,            m1
    psraw           m0,            2
    pextrw          [r6],          m0, 0
    pextrw          [r6 + r1],     m0, 1
    pextrw          [r6 + r1 * 2], m0, 2
    lea             r6,            [r6 + r1 * 2]
    pextrw          [r6 + r1],     m0, 3
    pextrw          [r6 + r1 * 2], m0, 4
    lea             r6,            [r6 + r1 * 2]
    pextrw          [r6 + r1],     m0, 5
    pextrw          [r6 + r1 * 2], m0, 6

.end
    RET


;-------------------------------------------------------------------------------------------------------
; void intra_pred_dc(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_dc16, 4, 7, 4
    mov             r4d,                 r5m
    add             r2,                  2
    add             r3,                  2
    add             r1,                  r1
    movu            m0,                  [r3]
    movu            m1,                  [r3 + 16]
    movu            m2,                  [r2]
    movu            m3,                  [r2 + 16]

    paddw           m0,                  m1
    paddw           m2,                  m3
    paddw           m0,                  m2
    movhlps         m1,                  m0
    paddw           m0,                  m1
    phaddw          m0,                  m0
    pmaddwd         m0,                  [pw_1]

    movd            r5d,                 m0
    add             r5d,                 16
    shr             r5d,                 5     ; sum = sum / 16
    movd            m1,                  r5d
    pshuflw         m1,                  m1, 0 ; m1 = word [dc_val ...]
    pshufd          m1,                  m1, 0

    test            r4d,                 r4d

    ; store DC 16x16
    mov             r6,                  r0
    movu            [r0],                m1
    movu            [r0 + 16],           m1
    movu            [r0 + r1],           m1
    movu            [r0 + 16 + r1],      m1
    lea             r0,                  [r0 + r1 * 2]
    movu            [r0],                m1
    movu            [r0 + 16],           m1
    movu            [r0 + r1],           m1
    movu            [r0 + 16 + r1],      m1
    lea             r0,                  [r0 + r1 * 2]
    movu            [r0],                m1
    movu            [r0 + 16],           m1
    movu            [r0 + r1],           m1
    movu            [r0 + 16 + r1],      m1
    lea             r0,                  [r0 + r1 * 2]
    movu            [r0],                m1
    movu            [r0 + 16],           m1
    movu            [r0 + r1],           m1
    movu            [r0 + 16 + r1],      m1
    lea             r0,                  [r0 + r1 * 2]
    movu            [r0],                m1
    movu            [r0 + 16],           m1
    movu            [r0 + r1],           m1
    movu            [r0 + 16 + r1],      m1
    lea             r0,                  [r0 + r1 * 2]
    movu            [r0],                m1
    movu            [r0 + 16],           m1
    movu            [r0 + r1],           m1
    movu            [r0 + 16 + r1],      m1
    lea             r0,                  [r0 + r1 * 2]
    movu            [r0],                m1
    movu            [r0 + 16],           m1
    movu            [r0 + r1],           m1
    movu            [r0 + 16 + r1],      m1
    lea             r0,                  [r0 + r1 * 2]
    movu            [r0],                m1
    movu            [r0 + 16],           m1
    movu            [r0 + r1],           m1
    movu            [r0 + 16 + r1],      m1

    ; Do DC Filter
    jz              .end
    lea             r4d,                 [r5d * 2 + 2]  ; r4d = DC * 2 + 2
    add             r5d,                 r4d            ; r5d = DC * 3 + 2
    movd            m1,                  r5d
    pshuflw         m1,                  m1, 0          ; m1 = pixDCx3
    pshufd          m1,                  m1, 0

    ; filter top
    movu            m2,                  [r3]
    paddw           m2,                  m1
    psraw           m2,                  2
    movu            [r6],                m2
    movu            m3,                  [r3 + 16]
    paddw           m3,                  m1
    psraw           m3,                  2
    movu            [r6 + 16],           m3

    ; filter top-left
    movzx           r3d, word            [r3]
    add             r4d,                 r3d
    movzx           r3d, word            [r2]
    add             r3d,                 r4d
    shr             r3d,                 2
    mov             [r6],                r3w

    ; filter left
    add             r6,                  r1
    movu            m2,                  [r2 + 2]
    paddw           m2,                  m1
    psraw           m2,                  2

    pextrw          [r6],                m2, 0
    pextrw          [r6 + r1],           m2, 1
    lea             r6,                  [r6 + r1 * 2]
    pextrw          [r6],                m2, 2
    pextrw          [r6 + r1],           m2, 3
    lea             r6,                  [r6 + r1 * 2]
    pextrw          [r6],                m2, 4
    pextrw          [r6 + r1],           m2, 5
    lea             r6,                  [r6 + r1 * 2]
    pextrw          [r6],                m2, 6
    pextrw          [r6 + r1],           m2, 7

    lea             r6,                  [r6 + r1 * 2]
    movu            m3,                  [r2 + 18]
    paddw           m3,                  m1
    psraw           m3,                  2

    pextrw          [r6],                m3, 0
    pextrw          [r6 + r1],           m3, 1
    lea             r6,                  [r6 + r1 * 2]
    pextrw          [r6],                m3, 2
    pextrw          [r6 + r1],           m3, 3
    lea             r6,                  [r6 + r1 * 2]
    pextrw          [r6],                m3, 4
    pextrw          [r6 + r1],           m3, 5
    lea             r6,                  [r6 + r1 * 2]
    pextrw          [r6],                m3, 6

.end
    RET


;-------------------------------------------------------------------------------------------
; void intra_pred_dc(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int filter)
;-------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_dc32, 4, 5, 6
    mov             r4d,                 r5m
    add             r2,                  2
    add             r3,                  2
    add             r1,                  r1
    movu            m0,                  [r3]
    movu            m1,                  [r3 + 16]
    movu            m2,                  [r3 + 32]
    movu            m3,                  [r3 + 48]
    paddw           m0,                  m1
    paddw           m2,                  m3
    paddw           m0,                  m2
    movu            m1,                  [r2]
    movu            m3,                  [r2 + 16]
    movu            m4,                  [r2 + 32]
    movu            m5,                  [r2 + 48]
    paddw           m1,                  m3
    paddw           m4,                  m5
    paddw           m1,                  m4
    paddw           m0,                  m1
    movhlps         m1,                  m0
    paddw           m0,                  m1
    phaddw          m0,                  m0
    pmaddwd         m0,                  [pw_1]

    paddd           m0,                  [pd_32]     ; sum = sum + 32
    psrld           m0,                  6           ; sum = sum / 64
    pshuflw         m0,                  m0, 0
    pshufd          m0,                  m0, 0

    lea             r2,                 [r1 * 3]
    mov             r3d,                4
.loop:
    ; store DC 32x32
    movu            [r0 +  0],          m0
    movu            [r0 + 16],          m0
    movu            [r0 + 32],          m0
    movu            [r0 + 48],          m0
    movu            [r0 + r1 +  0],     m0
    movu            [r0 + r1 + 16],     m0
    movu            [r0 + r1 + 32],     m0
    movu            [r0 + r1 + 48],     m0
    movu            [r0 + r1 * 2 +  0], m0
    movu            [r0 + r1 * 2 + 16], m0
    movu            [r0 + r1 * 2 + 32], m0
    movu            [r0 + r1 * 2 + 48], m0
    movu            [r0 + r2 +  0],     m0
    movu            [r0 + r2 + 16],     m0
    movu            [r0 + r2 + 32],     m0
    movu            [r0 + r2 + 48],     m0
    lea             r0, [r0 + r1 * 4]
    movu            [r0 +  0],          m0
    movu            [r0 + 16],          m0
    movu            [r0 + 32],          m0
    movu            [r0 + 48],          m0
    movu            [r0 + r1 +  0],     m0
    movu            [r0 + r1 + 16],     m0
    movu            [r0 + r1 + 32],     m0
    movu            [r0 + r1 + 48],     m0
    movu            [r0 + r1 * 2 +  0], m0
    movu            [r0 + r1 * 2 + 16], m0
    movu            [r0 + r1 * 2 + 32], m0
    movu            [r0 + r1 * 2 + 48], m0
    movu            [r0 + r2 +  0],     m0
    movu            [r0 + r2 + 16],     m0
    movu            [r0 + r2 + 32],     m0
    movu            [r0 + r2 + 48],     m0
    lea             r0, [r0 + r1 * 4]
    dec             r3d
    jnz            .loop
    RET

;-----------------------------------------------------------------------------------------------------------
; void intra_pred_planar(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-----------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_planar4, 4,7,5
    add             r2,         2
    add             r3,         2
    add             r1,         r1
    movh            m0,         [r3]      ; topRow[i] = above[i];
    punpcklqdq      m0,         m0

    pxor            m1,         m1
    movd            m2,         [r2 + 8]  ; bottomLeft = left[4]
    movzx           r6d, word   [r3 + 8]  ; topRight   = above[4];
    pshuflw         m2,         m2, 0
    pshufd          m2,         m2, 0

    psubw           m2,         m0        ; bottomRow[i] = bottomLeft - topRow[i]
    psllw           m0,         2
    punpcklqdq      m3,         m2, m1
    psubw           m0,         m3
    paddw           m2,         m2

%macro COMP_PRED_PLANAR_2ROW 1
    movzx           r4d, word   [r2 + %1]
    lea             r4d,        [r4d * 4 + 4]
    movd            m3,         r4d
    pshuflw         m3,         m3, 0

    movzx           r4d, word   [r2 + %1 + 2]
    lea             r4d,        [r4d * 4 + 4]
    movd            m4,         r4d
    pshuflw         m4,         m4, 0
    punpcklqdq      m3,         m4        ; horPred

    movzx           r4d, word   [r2 + %1]
    mov             r5d,        r6d
    sub             r5d,        r4d
    movd            m4,         r5d
    pshuflw         m4,         m4, 0

    movzx           r4d, word   [r2 + %1 + 2]
    mov             r5d,        r6d
    sub             r5d,        r4d
    movd            m1,         r5d
    pshuflw         m1,         m1, 0
    punpcklqdq      m4,         m1        ; rightColumnN

    pmullw          m4,         [multi_2Row]
    paddw           m3,         m4
    paddw           m0,         m2
    paddw           m3,         m0
    psraw           m3,         3

    movh            [r0],       m3
    pshufd          m3,         m3, 0xAE
    movh            [r0 + r1],  m3
    lea             r0,         [r0 + 2 * r1]
%endmacro

    COMP_PRED_PLANAR_2ROW 0
    COMP_PRED_PLANAR_2ROW 4
%undef COMP_PRED_PLANAR_2ROW
    RET

;-----------------------------------------------------------------------------------------------------------
; void intra_pred_planar(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-----------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_planar8, 4,4,7
    add             r2,     2
    add             r3,     2
    add             r1,     r1
    movu            m1,     [r3]      ; v_topRow
    movu            m2,     [r2]      ; v_leftColumn

    movd            m3,     [r3 + 16] ; topRight   = above[8];
    movd            m4,     [r2 + 16] ; bottomLeft = left[8];

    pshuflw         m3,     m3, 0
    pshufd          m3,     m3, 0
    pshuflw         m4,     m4, 0
    pshufd          m4,     m4, 0

    psubw           m4,     m1        ; v_bottomRow
    psubw           m3,     m2        ; v_rightColumn

    psllw           m1,     3         ; v_topRow
    psllw           m2,     3         ; v_leftColumn

    paddw           m6,     m2, [pw_8]

%macro PRED_PLANAR_ROW8 1
    %if (%1 < 4)
        pshuflw     m5,     m6, 0x55 * %1
        pshufd      m5,     m5, 0
        pshuflw     m2,     m3, 0x55 * %1
        pshufd      m2,     m2, 0
    %else
        pshufhw     m5,     m6, 0x55 * (%1 - 4)
        pshufd      m5,     m5, 0xAA
        pshufhw     m2,     m3, 0x55 * (%1 - 4)
        pshufd      m2,     m2, 0xAA
    %endif

    pmullw          m2,     [multiL]
    paddw           m5,     m2
    paddw           m1,     m4
    paddw           m5,     m1
    psraw           m5,     4

    movu            [r0],   m5
    add             r0,     r1

%endmacro

    PRED_PLANAR_ROW8 0
    PRED_PLANAR_ROW8 1
    PRED_PLANAR_ROW8 2
    PRED_PLANAR_ROW8 3
    PRED_PLANAR_ROW8 4
    PRED_PLANAR_ROW8 5
    PRED_PLANAR_ROW8 6
    PRED_PLANAR_ROW8 7

%undef PRED_PLANAR_ROW8
    RET


;-----------------------------------------------------------------------------------------------------------
; void intra_pred_planar(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-----------------------------------------------------------------------------------------------------------
INIT_XMM sse4
%if (BIT_DEPTH == 12)

%if (ARCH_X86_64 == 1)
cglobal intra_pred_planar16, 4,7,8+3
%define bottomRow0  m7
%define bottomRow1  m8
%define bottomRow2  m9
%define bottomRow3  m10
%else
cglobal intra_pred_planar16, 4,7,8, 0-3*mmsize
%define bottomRow0  [rsp + 0*mmsize]
%define bottomRow1  [rsp + 1*mmsize]
%define bottomRow2  [rsp + 2*mmsize]
%define bottomRow3  m7
%endif

    add             r2, 2
    add             r3, 2
    add             r1, r1

    pxor            m0, m0

    ; bottomRow
    movzx           r4d, word [r2 + 16*2]
    movd            m1, r4d
    pshufd          m1, m1, 0               ; m1 = bottomLeft
    movu            m2, [r3]
    pmovzxwd        m3, m2
    punpckhwd       m2, m0
    psubd           m4, m1, m3
    mova            bottomRow0, m4
    psubd           m4, m1, m2
    mova            bottomRow1, m4
    movu            m2, [r3 + 16]
    pmovzxwd        m3, m2
    punpckhwd       m2, m0
    psubd           m4, m1, m3
    mova            bottomRow2, m4
    psubd           m1, m2
    mova            bottomRow3, m1

    ; topRow
    pmovzxwd        m0, [r3 + 0*8]
    pslld           m0, 4
    pmovzxwd        m1, [r3 + 1*8]
    pslld           m1, 4
    pmovzxwd        m2, [r3 + 2*8]
    pslld           m2, 4
    pmovzxwd        m3, [r3 + 3*8]
    pslld           m3, 4

    xor             r6, r6
.loopH:
    movzx           r4d, word [r2 + r6*2]
    movzx           r5d, word [r3 + 16*2]       ; r5 = topRight
    sub             r5d, r4d
    movd            m5, r5d
    pshuflw         m5, m5, 0
    pmullw          m5, [multiL]
    pmovsxwd        m5, m5                      ; m5 = rightCol
    add             r4d, r4d
    lea             r4d, [r4d * 8 + 16]
    movd            m4, r4d
    pshufd          m4, m4, 0                   ; m4 = horPred
    paddd           m4, m5
    pshufd          m6, m5, 0xFF                ; m6 = [4 4 4 4]

    ; 0-3
    paddd           m0, bottomRow0
    paddd           m5, m0, m4
    psrad           m5, 5
    packusdw        m5, m5
    movh            [r0 + 0*8], m5

    ; 4-7
    paddd           m4, m6
    paddd           m1, bottomRow1
    paddd           m5, m1, m4
    psrad           m5, 5
    packusdw        m5, m5
    movh            [r0 + 1*8], m5

    ; 8-11
    paddd           m4, m6
    paddd           m2, bottomRow2
    paddd           m5, m2, m4
    psrad           m5, 5
    packusdw        m5, m5
    movh            [r0 + 2*8], m5

    ; 12-15
    paddd           m4, m6
    paddd           m3, bottomRow3
    paddd           m5, m3, m4
    psrad           m5, 5
    packusdw        m5, m5
    movh            [r0 + 3*8], m5

    add             r0, r1
    inc             r6d
    cmp             r6d, 16
    jnz            .loopH
    RET

%else ; BIT_DEPTH == 10
INIT_XMM sse4
cglobal intra_pred_planar16, 4,6,7
    add             r2,         2
    add             r3,         2
    add             r1,         r1

    movu            m1,         [r3]        ; topRow[0-7]
    movu            m2,         [r3 + 16]   ; topRow[8-15]

    movd            m3,         [r2 + 32]
    pshuflw         m3,         m3, 0
    pshufd          m3,         m3, 0
    movzx           r4d, word   [r3 + 32]   ; topRight = above[16]

    psubw           m4,         m3, m1      ; v_bottomRow[0]
    psubw           m3,         m2          ; v_bottomRow[1]

    psllw           m1,         4
    psllw           m2,         4

%macro PRED_PLANAR_ROW16 1
    movzx           r5d, word   [r2 + %1 * 2]
    add             r5d,        r5d
    lea             r5d,        [r5d * 8 + 16]
    movd            m5,         r5d
    pshuflw         m5,         m5, 0
    pshufd          m5,         m5, 0       ; horPred

    movzx           r5d, word   [r2 + %1 * 2]
    mov             r3d,        r4d
    sub             r3d,        r5d
    movd            m0,         r3d
    pshuflw         m0,         m0, 0
    pshufd          m0,         m0, 0

    pmullw          m6,         m0, [multiL]
    paddw           m6,         m5
    paddw           m1,         m4
    paddw           m6,         m1
    psraw           m6,         5

    pmullw          m0,         m0, [multiH]
    paddw           m5,         m0
    paddw           m2,         m3
    paddw           m5,         m2
    psraw           m5,         5

    movu            [r0],       m6
    movu            [r0 + 16],  m5
    add             r0,         r1
%endmacro

    PRED_PLANAR_ROW16 0
    PRED_PLANAR_ROW16 1
    PRED_PLANAR_ROW16 2
    PRED_PLANAR_ROW16 3
    PRED_PLANAR_ROW16 4
    PRED_PLANAR_ROW16 5
    PRED_PLANAR_ROW16 6
    PRED_PLANAR_ROW16 7
    PRED_PLANAR_ROW16 8
    PRED_PLANAR_ROW16 9
    PRED_PLANAR_ROW16 10
    PRED_PLANAR_ROW16 11
    PRED_PLANAR_ROW16 12
    PRED_PLANAR_ROW16 13
    PRED_PLANAR_ROW16 14
    PRED_PLANAR_ROW16 15
%undef PRED_PLANAR_ROW16
    RET
%endif


;-----------------------------------------------------------------------------
; void intraPredAng(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal intra_pred_ang4_2, 3,3,4
    cmp         r4m,           byte 34
    cmove       r2,            r3mp
    add         r1,            r1
    movu        m0,            [r2 + 4]
    movh        [r0],          m0
    palignr     m1,            m0, 2
    movh        [r0 + r1],     m1
    palignr     m2,            m0, 4
    movh        [r0 + r1 * 2], m2
    lea         r1,            [r1 * 3]
    psrldq      m0,            6
    movh        [r0 + r1],     m0
    RET

INIT_XMM sse4
cglobal intra_pred_ang4_3, 3,4,8
    cmp         r4m, byte 33
    cmove       r2, r3mp
    lea         r3, [ang_table + 20 * 16]
    movu        m0, [r2 + 2]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 2       ; [x 8 7 6 5 4 3 2]
    punpcklwd   m2, m0, m1      ; [5 4 4 3 3 2 2 1]
    palignr     m5, m0, 4       ; [x x 8 7 6 5 4 3]
    punpcklwd   m3, m1, m5      ; [6 5 5 4 4 3 3 2]
    palignr     m1, m0, 6       ; [x x x 8 7 6 5 4]
    punpcklwd   m4, m5 ,m1      ; [7 6 6 5 5 4 4 3]
    movhlps     m0, m0          ; [x x x x 8 7 6 5]
    punpcklwd   m5, m1, m0      ; [8 7 7 6 6 5 5 4]

    mova        m0, [r3 + 6 * 16]   ; [26]
    mova        m1, [r3]            ; [20]
    mova        m6, [r3 - 6 * 16]   ; [14]
    mova        m7, [r3 - 12 * 16]  ; [ 8]
    jmp        .do_filter4x4

ALIGN 16
.do_filter4x4:
    pmaddwd m2, m0
    paddd   m2, [pd_16]
    psrld   m2, 5

    pmaddwd m3, m1
    paddd   m3, [pd_16]
    psrld   m3, 5
    packusdw m2, m3

    pmaddwd m4, m6
    paddd   m4, [pd_16]
    psrld   m4, 5

    pmaddwd m5, m7
    paddd   m5, [pd_16]
    psrld   m5, 5
    packusdw m4, m5

    jz         .store

    ; transpose 4x4
    punpckhwd    m0, m2, m4
    punpcklwd    m2, m4
    punpckhwd    m4, m2, m0
    punpcklwd    m2, m0

.store:
    add         r1, r1
    movh        [r0], m2
    movhps      [r0 + r1], m2
    movh        [r0 + r1 * 2], m4
    lea         r1, [r1 * 3]
    movhps      [r0 + r1], m4
    RET

cglobal intra_pred_ang4_4, 3,4,8
    cmp         r4m, byte 32
    cmove       r2, r3mp
    lea         r3, [ang_table + 18 * 16]
    movu        m0, [r2 + 2]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 2       ; [x 8 7 6 5 4 3 2]
    punpcklwd   m2, m0, m1      ; [5 4 4 3 3 2 2 1]
    palignr     m6, m0, 4       ; [x x 8 7 6 5 4 3]
    punpcklwd   m3, m1, m6      ; [6 5 5 4 4 3 3 2]
    mova        m4, m3
    palignr     m7, m0, 6       ; [x x x 8 7 6 5 4]
    punpcklwd   m5, m6, m7      ; [7 6 6 5 5 4 4 3]

    mova        m0, [r3 +  3 * 16]  ; [21]
    mova        m1, [r3 -  8 * 16]  ; [10]
    mova        m6, [r3 + 13 * 16]  ; [31]
    mova        m7, [r3 +  2 * 16]  ; [20]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)

cglobal intra_pred_ang4_5, 3,4,8
    cmp         r4m, byte 31
    cmove       r2, r3mp
    lea         r3, [ang_table + 10 * 16]
    movu        m0, [r2 + 2]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 2       ; [x 8 7 6 5 4 3 2]
    punpcklwd   m2, m0, m1      ; [5 4 4 3 3 2 2 1]
    palignr     m6, m0, 4       ; [x x 8 7 6 5 4 3]
    punpcklwd   m3, m1, m6      ; [6 5 5 4 4 3 3 2]
    mova        m4, m3
    palignr     m7, m0, 6       ; [x x x 8 7 6 5 4]
    punpcklwd   m5, m6, m7      ; [7 6 6 5 5 4 4 3]

    mova        m0, [r3 +  7 * 16]  ; [17]
    mova        m1, [r3 -  8 * 16]  ; [ 2]
    mova        m6, [r3 +  9 * 16]  ; [19]
    mova        m7, [r3 -  6 * 16]  ; [ 4]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)

cglobal intra_pred_ang4_6, 3,4,8
    cmp         r4m, byte 30
    cmove       r2, r3mp
    lea         r3, [ang_table + 19 * 16]
    movu        m0, [r2 + 2]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 2       ; [x 8 7 6 5 4 3 2]
    punpcklwd   m2, m0, m1      ; [5 4 4 3 3 2 2 1]
    mova        m3, m2
    palignr     m6, m0, 4       ; [x x 8 7 6 5 4 3]
    punpcklwd   m4, m1, m6      ; [6 5 5 4 4 3 3 2]
    mova        m5, m4

    mova        m0, [r3 -  6 * 16]  ; [13]
    mova        m1, [r3 +  7 * 16]  ; [26]
    mova        m6, [r3 - 12 * 16]  ; [ 7]
    mova        m7, [r3 +  1 * 16]  ; [20]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)

cglobal intra_pred_ang4_7, 3,4,8
    cmp         r4m, byte 29
    cmove       r2, r3mp
    lea         r3, [ang_table + 20 * 16]
    movu        m0, [r2 + 2]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 2       ; [x 8 7 6 5 4 3 2]
    punpcklwd   m2, m0, m1      ; [5 4 4 3 3 2 2 1]
    mova        m3, m2
    mova        m4, m2
    palignr     m6, m0, 4       ; [x x 8 7 6 5 4 3]
    punpcklwd   m5, m1, m6      ; [6 5 5 4 4 3 3 2]

    mova        m0, [r3 - 11 * 16]  ; [ 9]
    mova        m1, [r3 -  2 * 16]  ; [18]
    mova        m6, [r3 +  7 * 16]  ; [27]
    mova        m7, [r3 - 16 * 16]  ; [ 4]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)

cglobal intra_pred_ang4_8, 3,4,8
    cmp         r4m, byte 28
    cmove       r2, r3mp
    lea         r3, [ang_table + 13 * 16]
    movu        m0, [r2 + 2]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 2       ; [x 8 7 6 5 4 3 2]
    punpcklwd   m2, m0, m1      ; [5 4 4 3 3 2 2 1]
    mova        m3, m2
    mova        m4, m2
    mova        m5, m2

    mova        m0, [r3 -  8 * 16]  ; [ 5]
    mova        m1, [r3 -  3 * 16]  ; [10]
    mova        m6, [r3 +  2 * 16]  ; [15]
    mova        m7, [r3 +  7 * 16]  ; [20]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_9, 3,4,8
    cmp         r4m, byte 27
    cmove       r2, r3mp
    lea         r3, [ang_table + 4 * 16]
    movu        m0, [r2 + 2]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 2       ; [x 8 7 6 5 4 3 2]
    punpcklwd   m2, m0, m1      ; [5 4 4 3 3 2 2 1]
    mova        m3, m2
    mova        m4, m2
    mova        m5, m2

    mova        m0, [r3 -  2 * 16]  ; [ 2]
    mova        m1, [r3 -  0 * 16]  ; [ 4]
    mova        m6, [r3 +  2 * 16]  ; [ 6]
    mova        m7, [r3 +  4 * 16]  ; [ 8]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)

cglobal intra_pred_ang4_10, 3,3,4
    movh        m0,             [r2 + 2]            ; [4 3 2 1]
    pshufb      m2,             m0, [pb_unpackwq2]  ; [4 4 4 4 3 3 3 3]
    pshufb      m0,             [pb_unpackwq1]      ; [2 2 2 2 1 1 1 1]
    add         r1,             r1
    movhlps     m1,             m0                  ; [2 2 2 2]
    movhlps     m3,             m2                  ; [4 4 4 4]
    movh        [r0 + r1],      m1
    movh        [r0 + r1 * 2],  m2
    lea         r1,             [r1 * 3]
    movh        [r0 + r1],      m3

    cmp         r5m,            byte 0
    jz         .quit

    ; filter
    mov         r2,             r3mp
    movu        m1,             [r2]                ; [7 6 5 4 3 2 1 0]
    pshufb      m2,             m1, [pb_unpackwq1]  ; [0 0 0 0]
    palignr     m1,             m1, 2               ; [4 3 2 1]
    psubw       m1,             m2
    psraw       m1,             1
    paddw       m0,             m1
    pmovsxwd    m0,             m0
    packusdw    m0,             m0

.quit:
    movh        [r0],           m0
    RET

cglobal intra_pred_ang4_26, 4,4,3
    movh        m0,             [r3 + 2]            ; [8 7 6 5 4 3 2 1]
    add         r1,             r1
    ; store
    movh        [r0],           m0
    movh        [r0 + r1],      m0
    movh        [r0 + r1 * 2],  m0
    lea         r3,             [r1 * 3]
    movh        [r0 + r3],      m0

    ; filter
    cmp         r5m,            byte 0
    jz         .quit

    pshufb      m0,             [pb_unpackwq1]      ; [2 2 2 2 1 1 1 1]
    movu        m1,             [r2]                ; [7 6 5 4 3 2 1 0]
    pshufb      m2,             m1, [pb_unpackwq1]  ; [0 0 0 0]
    palignr     m1,             m1, 2               ; [4 3 2 1]
    psubw       m1,             m2
    psraw       m1,             1
    paddw       m0,             m1
    pmovsxwd    m0,             m0
    packusdw    m0,             m0

    pextrw      [r0],           m0, 0
    pextrw      [r0 + r1],      m0, 1
    pextrw      [r0 + r1 * 2],  m0, 2
    pextrw      [r0 + r3],      m0, 3

.quit:
    RET

cglobal intra_pred_ang4_11, 3,4,8
    cmp         r4m, byte 25
    cmove       r2, r3mp
    lea         r3, [ang_table + 24 * 16]
    movu        m2, [r2]        ; [x x x 4 3 2 1 0]
    palignr     m1, m2, 2       ; [x x x x 4 3 2 1]
    punpcklwd   m2, m1          ; [4 3 3 2 2 1 1 0]
    mova        m3, m2
    mova        m4, m2
    mova        m5, m2

    mova        m0, [r3 +  6 * 16]  ; [24]
    mova        m1, [r3 +  4 * 16]  ; [26]
    mova        m6, [r3 +  2 * 16]  ; [28]
    mova        m7, [r3 +  0 * 16]  ; [30]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_12, 3,4,8
    cmp         r4m, byte 24
    cmove       r2, r3mp
    lea         r3, [ang_table + 20 * 16]
    movu        m2, [r2]        ; [x x x 4 3 2 1 0]
    palignr     m1, m2, 2       ; [x x x x 4 3 2 1]
    punpcklwd   m2, m1          ; [4 3 3 2 2 1 1 0]
    mova        m3, m2
    mova        m4, m2
    mova        m5, m2

    mova        m0, [r3 +  7 * 16]  ; [27]
    mova        m1, [r3 +  2 * 16]  ; [22]
    mova        m6, [r3 -  3 * 16]  ; [17]
    mova        m7, [r3 -  8 * 16]  ; [12]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_13, 4,4,8
    cmp         r4m, byte 23
    jnz        .load
    xchg        r2, r3
.load
    movu        m5, [r2 - 2]    ; [x x 4 3 2 1 0 x]
    palignr     m2, m5, 2       ; [x x x 4 3 2 1 0]
    palignr     m0, m5, 4       ; [x x x x 4 3 2 1]
    pinsrw      m5, [r3 + 8], 0
    punpcklwd   m5, m2          ; [3 2 2 1 1 0 0 x]
    punpcklwd   m2, m0          ; [4 3 3 2 2 1 1 0]
    mova        m3, m2
    mova        m4, m2

    lea         r3, [ang_table + 21 * 16]
    mova        m0, [r3 +  2 * 16]  ; [23]
    mova        m1, [r3 -  7 * 16]  ; [14]
    mova        m6, [r3 - 16 * 16]  ; [ 5]
    mova        m7, [r3 +  7 * 16]  ; [28]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)

cglobal intra_pred_ang4_14, 4,4,8
    cmp         r4m, byte 22
    jnz        .load
    xchg        r2, r3
.load
    movu        m5, [r2 - 2]    ; [x x 4 3 2 1 0 x]
    palignr     m2, m5, 2       ; [x x x 4 3 2 1 0]
    palignr     m0, m5, 4       ; [x x x x 4 3 2 1]
    pinsrw      m5, [r3 + 4], 0
    punpcklwd   m5, m2          ; [3 2 2 1 1 0 0 x]
    punpcklwd   m2, m0          ; [4 3 3 2 2 1 1 0]
    mova        m3, m2
    mova        m4, m5

    lea         r3, [ang_table + 19 * 16]
    mova        m0, [r3 +  0 * 16]  ; [19]
    mova        m1, [r3 - 13 * 16]  ; [ 6]
    mova        m6, [r3 +  6 * 16]  ; [25]
    mova        m7, [r3 -  7 * 16]  ; [12]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_15, 4,4,8
    cmp         r4m, byte 21
    jnz        .load
    xchg        r2, r3
.load
    movu        m3, [r2 - 2]    ; [x x 4 3 2 1 0 x]
    palignr     m2, m3, 2       ; [x x x 4 3 2 1 0]
    palignr     m0, m3, 4       ; [x x x x 4 3 2 1]
    pinsrw      m3, [r3 + 4], 0
    pslldq      m5, m3, 2       ; [x 4 3 2 1 0 x y]
    pinsrw      m5, [r3 + 8], 0
    punpcklwd   m5, m3          ; [2 1 1 0 0 x x y]
    punpcklwd   m3, m2          ; [3 2 2 1 1 0 0 x]
    punpcklwd   m2, m0          ; [4 3 3 2 2 1 1 0]
    mova        m4, m3

    lea         r3, [ang_table + 23 * 16]
    mova        m0, [r3 -  8 * 16]  ; [15]
    mova        m1, [r3 +  7 * 16]  ; [30]
    mova        m6, [r3 - 10 * 16]  ; [13]
    mova        m7, [r3 +  5 * 16]  ; [28]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_16, 4,4,8
    cmp         r4m, byte 20
    jnz        .load
    xchg        r2, r3
.load
    movu        m3, [r2 - 2]    ; [x x 4 3 2 1 0 x]
    palignr     m2, m3, 2       ; [x x x 4 3 2 1 0]
    palignr     m0, m3, 4       ; [x x x x 4 3 2 1]
    pinsrw      m3, [r3 + 4], 0
    pslldq      m5, m3, 2       ; [x 4 3 2 1 0 x y]
    pinsrw      m5, [r3 + 6], 0
    punpcklwd   m5, m3          ; [2 1 1 0 0 x x y]
    punpcklwd   m3, m2          ; [3 2 2 1 1 0 0 x]
    punpcklwd   m2, m0          ; [4 3 3 2 2 1 1 0]
    mova        m4, m3

    lea         r3, [ang_table + 19 * 16]
    mova        m0, [r3 -  8 * 16]  ; [11]
    mova        m1, [r3 +  3 * 16]  ; [22]
    mova        m6, [r3 - 18 * 16]  ; [ 1]
    mova        m7, [r3 -  7 * 16]  ; [12]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)

cglobal intra_pred_ang4_17, 4,4,8
    cmp         r4m, byte 19
    jnz        .load
    xchg        r2, r3
.load
    movu        m6, [r2 - 2]    ; [- - 4 3 2 1 0 x]
    palignr     m2, m6, 2       ; [- - - 4 3 2 1 0]
    palignr     m1, m6, 4       ; [- - - - 4 3 2 1]
    mova        m4, m2
    punpcklwd   m2, m1          ; [4 3 3 2 2 1 1 0]

    pinsrw      m6, [r3 + 2], 0
    punpcklwd   m3, m6, m4      ; [3 2 2 1 1 0 0 x]

    pslldq      m4, m6, 2       ; [- 4 3 2 1 0 x y]
    pinsrw      m4, [r3 + 4], 0
    pslldq      m5, m4, 2       ; [4 3 2 1 0 x y z]
    pinsrw      m5, [r3 + 8], 0
    punpcklwd   m5, m4          ; [1 0 0 x x y y z]
    punpcklwd   m4, m3          ; [2 1 1 0 0 x x y]

    lea         r3, [ang_table + 14 * 16]
    mova        m0, [r3 -  8 * 16]  ; [ 6]
    mova        m1, [r3 -  2 * 16]  ; [12]
    mova        m6, [r3 +  4 * 16]  ; [18]
    mova        m7, [r3 + 10 * 16]  ; [24]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_18, 4,4,1
    movh        m0, [r2]
    pshufb      m0, [pw_swap]
    movhps      m0, [r3 + 2]
    add         r1, r1
    lea         r2, [r1 * 3]
    movh        [r0 + r2], m0
    psrldq      m0, 2
    movh        [r0 + r1 * 2], m0
    psrldq      m0, 2
    movh        [r0 + r1], m0
    psrldq      m0, 2
    movh        [r0], m0
    RET
