;*****************************************************************************
;* Copyright (C) 2013 x265 project
;*
;* Authors: Dnyaneshwar Gorade <dnyaneshwar@multicorewareinc.com>
;*          Yuvaraj Venkatesh <yuvaraj@multicorewareinc.com>
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


SECTION .text

cextern pw_1
cextern pw_8
cextern pd_16
cextern pd_32
cextern pw_4096
cextern multiL
cextern multi_2Row


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

%rep 4
    ; store DC 16x16
    movu            [r0 +  0],           m0
    movu            [r0 + 16],           m0
    movu            [r0 + 32],           m0
    movu            [r0 + 48],           m0
    add             r0,                  r1
    movu            [r0 +  0],           m0
    movu            [r0 + 16],           m0
    movu            [r0 + 32],           m0
    movu            [r0 + 48],           m0
    add             r0,                  r1
    movu            [r0 +  0],           m0
    movu            [r0 + 16],           m0
    movu            [r0 + 32],           m0
    movu            [r0 + 48],           m0
    add             r0,                  r1
    movu            [r0 +  0],           m0
    movu            [r0 + 16],           m0
    movu            [r0 + 32],           m0
    movu            [r0 + 48],           m0
    add             r0,                  r1
    movu            [r0 +  0],           m0
    movu            [r0 + 16],           m0
    movu            [r0 + 32],           m0
    movu            [r0 + 48],           m0
    add             r0,                  r1
    movu            [r0 +  0],           m0
    movu            [r0 + 16],           m0
    movu            [r0 + 32],           m0
    movu            [r0 + 48],           m0
    add             r0,                  r1
    movu            [r0 +  0],           m0
    movu            [r0 + 16],           m0
    movu            [r0 + 32],           m0
    movu            [r0 + 48],           m0
    add             r0,                  r1
    movu            [r0 +  0],           m0
    movu            [r0 + 16],           m0
    movu            [r0 + 32],           m0
    movu            [r0 + 48],           m0
    add             r0,                  r1
%endrep
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
