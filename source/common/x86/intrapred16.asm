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
;* For more information, contact us at license @ x265.com.
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

const shuf_mode_13_23,      db  0,  0, 14, 15,  6,  7,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0
const shuf_mode_14_22,      db 14, 15, 10, 11,  4,  5,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0
const shuf_mode_15_21,      db 12, 13,  8,  9,  4,  5,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0
const shuf_mode_16_20,      db  2,  3,  0,  1, 14, 15, 12, 13,  8,  9,  6,  7,  2,  3,  0,  1
const shuf_mode_17_19,      db  0,  1, 14, 15, 12, 13, 10, 11,  6,  7,  4,  5,  2,  3,  0,  1
const shuf_mode32_18,       db 14, 15, 12, 13, 10, 11,  8,  9,  6,  7,  4,  5,  2,  3,  0,  1
const pw_punpcklwd,         db  0,  1,  2,  3,  2,  3,  4,  5,  4,  5,  6,  7,  6,  7,  8,  9
const c_mode32_10_0,        db  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1

const pw_unpackwdq, times 8 db 0,1
const pw_ang8_12,   db 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 13, 0, 1
const pw_ang8_13,   db 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14, 15, 8, 9, 0, 1
const pw_ang8_14,   db 0, 0, 0, 0, 0, 0, 0, 0, 14, 15, 10, 11, 4, 5, 0, 1
const pw_ang8_15,   db 0, 0, 0, 0, 0, 0, 0, 0, 12, 13, 8, 9, 4, 5, 0, 1
const pw_ang8_16,   db 0, 0, 0, 0, 0, 0, 12, 13, 10, 11, 6, 7, 4, 5, 0, 1
const pw_ang8_17,   db 0, 0, 14, 15, 12, 13, 10, 11, 8, 9, 4, 5, 2, 3, 0, 1
const pw_swap16,    db 14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1

const pw_ang16_13,   db 14, 15, 8, 9, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
const pw_ang16_16,   db 0, 0, 0, 0, 0, 0, 10, 11, 8, 9, 6, 7, 2, 3, 0, 1

SECTION .text

cextern pw_1
cextern pw_8
cextern pw_1023
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

.end:
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

.end:
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

;-----------------------------------------------------------------------------------------------------------
; void intra_pred_planar(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-----------------------------------------------------------------------------------------------------------
INIT_XMM sse4
%if (ARCH_X86_64 == 1)
cglobal intra_pred_planar32, 4,7,8+8, 0-4*mmsize
    %define bottomRow0  m8
    %define bottomRow1  m9
    %define bottomRow2  m10
    %define bottomRow3  m11
    %define bottomRow4  m12
    %define bottomRow5  m13
    %define bottomRow6  m14
    %define bottomRow7  m15
    %define tmp0        [rsp + 0*mmsize]
    %define tmp1        [rsp + 1*mmsize]
    %define tmp2        [rsp + 2*mmsize]
    %define tmp3        [rsp + 3*mmsize]
%else
cglobal intra_pred_planar32, 4,7,8, 0-12*mmsize
    %define bottomRow0  [rsp + 0*mmsize]
    %define bottomRow1  [rsp + 1*mmsize]
    %define bottomRow2  [rsp + 2*mmsize]
    %define bottomRow3  [rsp + 3*mmsize]
    %define bottomRow4  [rsp + 4*mmsize]
    %define bottomRow5  [rsp + 5*mmsize]
    %define bottomRow6  [rsp + 6*mmsize]
    %define bottomRow7  [rsp + 7*mmsize]
    %define tmp0        [rsp + 8*mmsize]
    %define tmp1        [rsp + 9*mmsize]
    %define tmp2        [rsp + 10*mmsize]
    %define tmp3        [rsp + 11*mmsize]
%endif

    add             r2, 2
    add             r3, 2
    add             r1, r1

    pxor            m0, m0

    ; bottomRow
    movzx           r4d, word [r2 + 32*2]
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
    psubd           m4, m1, m2
    mova            bottomRow3, m4

    movu            m2, [r3 + 32]
    pmovzxwd        m3, m2
    punpckhwd       m2, m0
    psubd           m4, m1, m3
    mova            bottomRow4, m4
    psubd           m4, m1, m2
    mova            bottomRow5, m4
    movu            m2, [r3 + 48]
    pmovzxwd        m3, m2
    punpckhwd       m2, m0
    psubd           m4, m1, m3
    mova            bottomRow6, m4
    psubd           m1, m2
    mova            bottomRow7, m1

    ; topRow
    pmovzxwd        m0, [r3 + 0*8]
    pslld           m0, 5
    pmovzxwd        m1, [r3 + 1*8]
    pslld           m1, 5
    pmovzxwd        m2, [r3 + 2*8]
    pslld           m2, 5
    pmovzxwd        m3, [r3 + 3*8]
    pslld           m3, 5

    pmovzxwd        m4, [r3 + 4*8]
    pslld           m4, 5
    mova            tmp0, m4
    pmovzxwd        m4, [r3 + 5*8]
    pslld           m4, 5
    mova            tmp1, m4
    pmovzxwd        m4, [r3 + 6*8]
    pslld           m4, 5
    mova            tmp2, m4
    pmovzxwd        m4, [r3 + 7*8]
    pslld           m4, 5
    mova            tmp3, m4

    xor             r6, r6
.loopH:
    movzx           r4d, word [r2 + r6*2]
    movzx           r5d, word [r3 + 32*2]       ; r5 = topRight
    sub             r5d, r4d
    movd            m5, r5d
    pshuflw         m5, m5, 0
    pmullw          m5, [multiL]
    pmovsxwd        m5, m5                      ; m5 = rightCol
    shl             r4d, 5
    add             r4d, 32
    movd            m4, r4d
    pshufd          m4, m4, 0                   ; m4 = horPred
    paddd           m4, m5
    pshufd          m6, m5, 0xFF                ; m6 = [4 4 4 4]

    ; 0-3
    paddd           m0, bottomRow0
    paddd           m5, m0, m4
    psrad           m5, 6
    packusdw        m5, m5
    movh            [r0 + 0*8], m5

    ; 4-7
    paddd           m4, m6
    paddd           m1, bottomRow1
    paddd           m5, m1, m4
    psrad           m5, 6
    packusdw        m5, m5
    movh            [r0 + 1*8], m5

    ; 8-11
    paddd           m4, m6
    paddd           m2, bottomRow2
    paddd           m5, m2, m4
    psrad           m5, 6
    packusdw        m5, m5
    movh            [r0 + 2*8], m5

    ; 12-15
    paddd           m4, m6
    paddd           m3, bottomRow3
    paddd           m5, m3, m4
    psrad           m5, 6
    packusdw        m5, m5
    movh            [r0 + 3*8], m5

    ; 16-19
    paddd           m4, m6
    mova            m7, tmp0
    paddd           m7, bottomRow4
    mova            tmp0, m7
    paddd           m7, m4
    psrad           m7, 6
    packusdw        m7, m7
    movh            [r0 + 4*8], m7

    ; 20-23
    paddd           m4, m6
    mova            m7, tmp1
    paddd           m7, bottomRow5
    mova            tmp1, m7
    paddd           m7, m4
    psrad           m7, 6
    packusdw        m7, m7
    movh            [r0 + 5*8], m7

    ; 24-27
    paddd           m4, m6
    mova            m7, tmp2
    paddd           m7, bottomRow6
    mova            tmp2, m7
    paddd           m7, m4
    psrad           m7, 6
    packusdw        m7, m7
    movh            [r0 + 6*8], m7

    ; 28-31
    paddd           m4, m6
    mova            m7, tmp3
    paddd           m7, bottomRow7
    mova            tmp3, m7
    paddd           m7, m4
    psrad           m7, 6
    packusdw        m7, m7
    movh            [r0 + 7*8], m7

    add             r0, r1
    inc             r6d
    cmp             r6d, 32
    jnz            .loopH

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
    pxor        m1,             m1
    pmaxsw      m0,             m1
    pminsw      m0,             [pw_1023]

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
    pxor        m1,             m1
    pmaxsw      m0,             m1
    pminsw      m0,             [pw_1023]

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
.load:
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
.load:
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
.load:
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
.load:
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
.load:
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
    punpcklwd   m4, m6          ; [2 1 1 0 0 x x y]

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

;-----------------------------------------------------------------------------
; void intraPredAng8(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal intra_pred_ang8_2, 3,4,3
    cmp         r4m,           byte 34
    cmove       r2,            r3mp
    add         r1,            r1
    lea         r3,            [r1 * 3]
    movu        m0,            [r2 + 4]
    movu        m1,            [r2 + 20]
    movu        [r0],          m0
    palignr     m2,            m1, m0, 2
    movu        [r0 + r1],     m2
    palignr     m2,            m1, m0, 4
    movu        [r0 + r1 * 2], m2
    palignr     m2,            m1, m0, 6
    movu        [r0 + r3],     m2
    lea         r0,            [r0 + r1 * 4]
    palignr     m2,            m1, m0, 8
    movu        [r0],          m2
    palignr     m2,            m1, m0, 10
    movu        [r0 + r1],     m2
    palignr     m2,            m1, m0, 12
    movu        [r0 + r1 * 2], m2
    palignr     m1,            m0, 14
    movu        [r0 + r3],     m1
    RET

INIT_XMM sse4
cglobal intra_pred_ang8_3, 3,5,8
    lea         r3,        [ang_table + 14 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]
    punpckhwd   m1,        m4                         ; [x 16 16 15 15 14 14 13]

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 12 * 16]             ; [26]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 12 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    pmaddwd     m2,        [r3 + 6 * 16]              ; [20]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m6,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m6,        [r3 + 6 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    palignr     m6,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m6,        [r3]                       ; [14]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m7,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m7,        [r3]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    pmaddwd     m7,        [r3 - 6 * 16]              ; [ 8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m3,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    pmaddwd     m3,        [r3 - 6 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m7,        m3

    punpckhwd   m3,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m7
    punpcklwd   m6,        m7

    punpckldq   m7,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m3, m2
    punpckhdq   m3,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m7
    movhps      [r0 + r1],       m7
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r2,              [r0 + r1 * 4]
    movh        [r2],            m6
    movhps      [r2 + r1],       m6
    movh        [r2 + r1 * 2],   m3
    movhps      [r2 + r4],       m3

    mova        m4,        m0
    pmaddwd     m4,        [r3 - 12 * 16]             ; [ 2]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m5
    pmaddwd     m2,        [r3 - 12 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m0
    pmaddwd     m2,        [r3 + 14 * 16]             ; [28]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m5
    pmaddwd     m6,        [r3 + 14 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    palignr     m6,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m6,        [r3 + 8 * 16]              ; [22]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m7,        m1, m5, 4                  ; [14 13 13 12 12 11 11 10]
    pmaddwd     m7,        [r3 + 8 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m7,        [r3 + 2 * 16]              ; [16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, 8                      ; [15 14 14 13 13 12 12 11]
    pmaddwd     m1,        [r3 + 2 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    punpckhwd   m3,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m7
    punpcklwd   m6,        m7

    punpckldq   m7,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m3, m2
    punpckhdq   m3,        m2

    movh        [r0 + 8],            m7
    movhps      [r0 + r1 + 8],       m7
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m3
    movhps      [r0 + r4 + 8],       m3

    RET

cglobal intra_pred_ang8_4, 3,6,8
    lea         r3,        [ang_table + 19 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 2 * 16]              ; [21]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 2 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m6,        m2
    pmaddwd     m2,        [r3 - 9 * 16]              ; [10]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    mova        m7,        m1
    pmaddwd     m1,        [r3 - 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 12 * 16]             ; [31]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 12 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m7,        [r3 + 1 * 16]              ; [20]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m1,        [r3 + 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    punpckhwd   m1,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m7
    punpcklwd   m6,        m7

    punpckldq   m7,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m1, m2
    punpckhdq   m1,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m7
    movhps      [r0 + r1],       m7
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r5,              [r0 + r1 * 4]
    movh        [r5],            m6
    movhps      [r5 + r1],       m6
    movh        [r5 + r1 * 2],   m1
    movhps      [r5 + r4],       m1

    palignr     m4,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    mova        m2,        m4
    pmaddwd     m4,        [r3 - 10 * 16]             ; [ 9]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m3,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    mova        m6,        m3
    pmaddwd     m3,        [r3 - 10 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m4,        m3

    pmaddwd     m2,        [r3 + 11 * 16]             ; [30]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    pmaddwd     m6,        [r3 + 11 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    mova        m6,        m0
    pmaddwd     m6,        [r3]                       ; [19]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m7,        m5
    pmaddwd     m7,        [r3]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    movh        m1,        [r2 + 26]                  ; [16 15 14 13]
    palignr     m7,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m7,        [r3 - 11 * 16]             ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, 4                      ; [14 13 13 12 12 11 11 10]
    pmaddwd     m1,        [r3 - 11 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    punpckhwd   m3,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m7
    punpcklwd   m6,        m7

    punpckldq   m7,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m3, m2
    punpckhdq   m3,        m2

    movh        [r0 + 8],            m7
    movhps      [r0 + r1 + 8],       m7
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m3
    movhps      [r0 + r4 + 8],       m3

    RET

cglobal intra_pred_ang8_5, 3,5,8
    lea         r3,        [ang_table + 13 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 4 * 16]              ; [17]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 4 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m6,        m2
    pmaddwd     m2,        [r3 - 11 * 16]             ; [2]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    mova        m7,        m1
    pmaddwd     m1,        [r3 - 11 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 6 * 16]              ; [19]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 6 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m7,        [r3 - 9 * 16]              ; [4]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m1,        [r3 - 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    punpckhwd   m1,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m7
    punpcklwd   m6,        m7

    punpckldq   m7,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m1, m2
    punpckhdq   m1,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m7
    movhps      [r0 + r1],       m7
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r2,              [r0 + r1 * 4]
    movh        [r2],            m6
    movhps      [r2 + r1],       m6
    movh        [r2 + r1 * 2],   m1
    movhps      [r2 + r4],       m1

    palignr     m4,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m4,        [r3 + 8 * 16]              ; [21]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m2,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m2,        [r3 + 8 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    mova        m6,        m2
    pmaddwd     m2,        [r3 - 7 * 16]              ; [6]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m1,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    mova        m7,        m1
    pmaddwd     m1,        [r3 - 7 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 10 * 16]             ; [23]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 10 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    mova        m7,        m0
    pmaddwd     m7,        [r3 - 5 * 16]              ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m5
    pmaddwd     m1,        [r3 - 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    punpckhwd   m3,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m7
    punpcklwd   m6,        m7

    punpckldq   m7,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m3, m2
    punpckhdq   m3,        m2

    movh        [r0 + 8],            m7
    movhps      [r0 + r1 + 8],       m7
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m3
    movhps      [r0 + r4 + 8],       m3

    RET

cglobal intra_pred_ang8_6, 3,5,8
    lea         r3,        [ang_table + 14 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 1 * 16]              ; [13]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 1 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 12 * 16]             ; [26]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    palignr     m6,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m7,        m6
    pmaddwd     m6,        [r3 - 7 * 16]              ; [7]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m1,        [r3 - 7 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m7,        [r3 + 6 * 16]              ; [20]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m1,        [r3 + 6 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    punpckhwd   m1,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m7
    punpcklwd   m6,        m7

    punpckldq   m7,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m1, m2
    punpckhdq   m1,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m7
    movhps      [r0 + r1],       m7
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r2,              [r0 + r1 * 4]
    movh        [r2],            m6
    movhps      [r2 + r1],       m6
    movh        [r2 + r1 * 2],   m1
    movhps      [r2 + r4],       m1

    palignr     m4,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    mova        m6,        m4
    pmaddwd     m4,        [r3 - 13 * 16]             ; [1]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m2,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    mova        m7,        m2
    pmaddwd     m2,        [r3 - 13 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    pmaddwd     m2,        m6, [r3]                   ; [14]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    pmaddwd     m1,        m7, [r3]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 13 * 16]             ; [27]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 13 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    pmaddwd     m7,        [r3 - 6 * 16]              ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m5,        m0, 12                     ; [12 11 11 10 10 9 9 8]
    pmaddwd     m5,        [r3 - 6 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m7,        m5

    punpckhwd   m3,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m7
    punpcklwd   m6,        m7

    punpckldq   m7,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m3, m2
    punpckhdq   m3,        m2

    movh        [r0 + 8],            m7
    movhps      [r0 + r1 + 8],       m7
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m3
    movhps      [r0 + r4 + 8],       m3

    RET

cglobal intra_pred_ang8_7, 3,5,8
    lea         r3,        [ang_table + 18 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 9 * 16]              ; [9]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 9 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3]                       ; [18]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 + 9 * 16]              ; [27]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m7,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    pmaddwd     m7,        [r3 - 14 * 16]             ; [4]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m1,        [r3 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    punpckhwd   m1,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m7
    punpcklwd   m6,        m7

    punpckldq   m7,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m1, m2
    punpckhdq   m1,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m7
    movhps      [r0 + r1],       m7
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r2,              [r0 + r1 * 4]
    movh        [r2],            m6
    movhps      [r2 + r1],       m6
    movh        [r2 + r1 * 2],   m1
    movhps      [r2 + r4],       m1

    palignr     m4,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m6,        m4
    pmaddwd     m4,        [r3 - 5 * 16]              ; [13]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m2,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    mova        m7,        m2
    pmaddwd     m2,        [r3 - 5 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    pmaddwd     m2,        m6, [r3 + 4 * 16]          ; [22]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    pmaddwd     m1,        m7, [r3 + 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 13 * 16]             ; [31]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 13 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m7,        [r3 - 10 * 16]             ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m5,        m0, 8                      ; [11 10 10 9 9 8 8 7]
    pmaddwd     m5,        [r3 - 10 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m7,        m5

    punpckhwd   m3,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m7
    punpcklwd   m6,        m7

    punpckldq   m7,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m3, m2
    punpckhdq   m3,        m2

    movh        [r0 + 8],            m7
    movhps      [r0 + r1 + 8],       m7
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m3
    movhps      [r0 + r4 + 8],       m3

    RET

cglobal intra_pred_ang8_8, 3,6,7
    lea         r3,        [ang_table + 17 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 4]                   ; [9 8 7 6 5 4 3 2]

    punpcklwd   m3,        m0, m1                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m1                         ; [9 8 8 7 7 6 6 5]

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 12 * 16]             ; [5]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 12 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 - 7 * 16]              ; [10]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 7 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 - 2 * 16]              ; [15]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 2 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m5,        m3
    pmaddwd     m5,        [r3 + 3 * 16]              ; [20]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 3 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    punpckhwd   m1,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m5
    punpcklwd   m6,        m5

    punpckldq   m5,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m1, m2
    punpckhdq   m1,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m5
    movhps      [r0 + r1],       m5
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r5,              [r0 + r1 * 4]
    movh        [r5],            m6
    movhps      [r5 + r1],       m6
    movh        [r5 + r1 * 2],   m1
    movhps      [r5 + r4],       m1

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 8 * 16]              ; [25]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 8 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 13 * 16]             ; [30]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 13 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    movh        m1,        [r2 + 18]                  ; [12 11 10 9]

    palignr     m6,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m5,        m6
    pmaddwd     m6,        [r3 - 14 * 16]             ; [3]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m1,        m0, 4                      ; [10 9 9 8 8 7 7 6]
    mova        m3,        m1
    pmaddwd     m1,        [r3 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m5,        [r3 - 9 * 16]              ; [8]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    pmaddwd     m3,        [r3 - 9 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m5,        m3

    punpckhwd   m3,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m5
    punpcklwd   m6,        m5

    punpckldq   m5,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m3, m2
    punpckhdq   m3,        m2

    movh        [r0 + 8],            m5
    movhps      [r0 + r1 + 8],       m5
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m3
    movhps      [r0 + r4 + 8],       m3

    RET

cglobal intra_pred_ang8_9, 3,5,7
    lea         r3,        [ang_table + 9 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 4]                   ; [9 8 7 6 5 4 3 2]

    punpcklwd   m3,        m0, m1                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m1                         ; [9 8 8 7 7 6 6 5]

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 7 * 16]              ; [2]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 7 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 - 5 * 16]              ; [4]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 - 3 * 16]              ; [6]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 3 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m5,        m3
    pmaddwd     m5,        [r3 - 1 * 16]              ; [8]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    punpckhwd   m1,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m5
    punpcklwd   m6,        m5

    punpckldq   m5,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m1, m2
    punpckhdq   m1,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m5
    movhps      [r0 + r1],       m5
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r2,              [r0 + r1 * 4]
    movh        [r2],            m6
    movhps      [r2 + r1],       m6
    movh        [r2 + r1 * 2],   m1
    movhps      [r2 + r4],       m1

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 1 * 16]              ; [10]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 1 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 3 * 16]              ; [12]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 3 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 + 5 * 16]              ; [14]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r3 + 5 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pmaddwd     m3,        [r3 + 7 * 16]              ; [16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r3 + 7 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    punpckhwd   m5,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m3
    punpcklwd   m6,        m3

    punpckldq   m3,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m5, m2
    punpckhdq   m5,        m2

    movh        [r0 + 8],            m3
    movhps      [r0 + r1 + 8],       m3
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m5
    movhps      [r0 + r4 + 8],       m5

    RET

cglobal intra_pred_ang8_10, 4,5,3
    movu        m1,             [r2 + 2]            ; [8 7 6 5 4 3 2 1]
    pshufb      m0,             m1, [pw_unpackwdq]  ; [1 1 1 1 1 1 1 1]
    add         r1,             r1
    lea         r4,             [r1 * 3]

    psrldq      m1,             2
    pshufb      m2,             m1, [pw_unpackwdq]  ; [2 2 2 2 2 2 2 2]
    movu        [r0 + r1],      m2
    psrldq      m1,             2
    pshufb      m2,             m1, [pw_unpackwdq]  ; [3 3 3 3 3 3 3 3]
    movu        [r0 + r1 * 2],  m2
    psrldq      m1,             2
    pshufb      m2,             m1, [pw_unpackwdq]  ; [4 4 4 4 4 4 4 4]
    movu        [r0 + r4],      m2

    lea         r2,             [r0 + r1 *4]
    psrldq      m1,             2
    pshufb      m2,             m1, [pw_unpackwdq]  ; [5 5 5 5 5 5 5 5]
    movu        [r2],           m2
    psrldq      m1,             2
    pshufb      m2,             m1, [pw_unpackwdq]  ; [6 6 6 6 6 6 6 6]
    movu        [r2 + r1],      m2
    psrldq      m1,             2
    pshufb      m2,             m1, [pw_unpackwdq]  ; [7 7 7 7 7 7 7 7]
    movu        [r2 + r1 * 2],  m2
    psrldq      m1,             2
    pshufb      m2,             m1, [pw_unpackwdq]  ; [8 8 8 8 8 8 8 8]
    movu        [r2 + r4],      m2

    cmp         r5m,            byte 0
    jz         .quit

    ; filter

    movh        m1,             [r3]                ; [3 2 1 0]
    pshufb      m2,             m1, [pw_unpackwdq]  ; [0 0 0 0 0 0 0 0]
    movu        m1,             [r3 + 2]            ; [8 7 6 5 4 3 2 1]
    psubw       m1,             m2
    psraw       m1,             1
    paddw       m0,             m1
    pxor        m1,             m1
    pmaxsw      m0,             m1
    pminsw      m0,             [pw_1023]

.quit:
    movu        [r0],           m0
    RET

cglobal intra_pred_ang8_11, 3,5,7
    lea         r3,        [ang_table + 23 * 16]
    add         r1,        r1

    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 7 * 16]              ; [30]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 7 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 5 * 16]              ; [28]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 + 3 * 16]              ; [26]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 3 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m5,        m3
    pmaddwd     m5,        [r3 + 1 * 16]              ; [24]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    punpckhwd   m1,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m5
    punpcklwd   m6,        m5

    punpckldq   m5,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m1, m2
    punpckhdq   m1,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m5
    movhps      [r0 + r1],       m5
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r2,              [r0 + r1 * 4]
    movh        [r2],            m6
    movhps      [r2 + r1],       m6
    movh        [r2 + r1 * 2],   m1
    movhps      [r2 + r4],       m1

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 1 * 16]              ; [22]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 1 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 - 3 * 16]              ; [20]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 3 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 - 5 * 16]              ; [18]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r3 - 5 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pmaddwd     m3,        [r3 - 7 * 16]              ; [16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r3 - 7 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    punpckhwd   m5,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m3
    punpcklwd   m6,        m3

    punpckldq   m3,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m5, m2
    punpckhdq   m5,        m2

    movh        [r0 + 8],            m3
    movhps      [r0 + r1 + 8],       m3
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m5
    movhps      [r0 + r4 + 8],       m5

    RET

cglobal intra_pred_ang8_12, 4,6,7
    lea         r5,        [ang_table + 16 * 16]
    add         r1,        r1

    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 11 * 16]             ; [27]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 11 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5 + 6 * 16]              ; [22]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5 + 6 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r5 + 1 * 16]              ; [17]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5 + 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m5,        m3
    pmaddwd     m5,        [r5 - 4 * 16]              ; [12]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5 - 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    punpckhwd   m1,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m5
    punpcklwd   m6,        m5

    punpckldq   m5,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m1, m2
    punpckhdq   m1,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m5
    movhps      [r0 + r1],       m5
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r2,              [r0 + r1 * 4]
    movh        [r2],            m6
    movhps      [r2 + r1],       m6
    movh        [r2 + r1 * 2],   m1
    movhps      [r2 + r4],       m1

    mova        m4,        m3
    pmaddwd     m4,        [r5 - 9 * 16]              ; [7]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 - 9 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5 - 14 * 16]             ; [2]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    palignr     m0,        m3, 12
    movu        m1,        [r3]
    pshufb      m1,        [pw_ang8_12]
    palignr     m3,        m1, 12

    mova        m6,        m3
    pmaddwd     m6,        [r5 + 13 * 16]             ; [29]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 + 13 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pmaddwd     m3,        [r5 + 8 * 16]              ; [24]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r5 + 8 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    punpckhwd   m5,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m3
    punpcklwd   m6,        m3

    punpckldq   m3,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m5, m2
    punpckhdq   m5,        m2

    movh        [r0 + 8],            m3
    movhps      [r0 + r1 + 8],       m3
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m5
    movhps      [r0 + r4 + 8],       m5

    RET

cglobal intra_pred_ang8_13, 4,6,8
    lea         r5,        [ang_table + 14 * 16]
    add         r1,        r1

    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 9 * 16]              ; [23]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 9 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5]                       ; [14]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 9 * 16]              ; [5]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5 - 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m0,        m3, 12
    movu        m1,        [r3]
    pshufb      m1,        [pw_ang8_13]
    palignr     m3,        m1, 12

    mova        m5,        m3
    pmaddwd     m5,        [r5 + 14 * 16]             ; [28]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m7,        m0
    pmaddwd     m7,        [r5 + 14 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m5,        m7

    punpckhwd   m7,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m5
    punpcklwd   m6,        m5

    punpckldq   m5,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m7, m2
    punpckhdq   m7,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m5
    movhps      [r0 + r1],       m5
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r2,              [r0 + r1 * 4]
    movh        [r2],            m6
    movhps      [r2 + r1],       m6
    movh        [r2 + r1 * 2],   m7
    movhps      [r2 + r4],       m7

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 5 * 16]              ; [19]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 5 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5 - 4 * 16]              ; [10]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 4 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 13 * 16]             ; [1]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 13 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    pmaddwd     m3,        [r5 + 10 * 16]             ; [24]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r5 + 10 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    punpckhwd   m5,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m3
    punpcklwd   m6,        m3

    punpckldq   m3,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m5, m2
    punpckhdq   m5,        m2

    movh        [r0 + 8],            m3
    movhps      [r0 + r1 + 8],       m3
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m5
    movhps      [r0 + r4 + 8],       m5

    RET

cglobal intra_pred_ang8_14, 4,6,8
    lea         r5,        [ang_table + 18 * 16]
    add         r1,        r1

    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 1 * 16]              ; [19]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 1 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5 - 12 * 16]             ; [6]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5 - 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    palignr     m0,        m3, 12
    movu        m1,        [r3]
    pshufb      m1,        [pw_ang8_14]
    palignr     m3,        m1, 12

    mova        m6,        m3
    pmaddwd     m6,        [r5 + 7 * 16]              ; [25]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 + 7 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    mova        m5,        m3
    pmaddwd     m5,        [r5 - 6 * 16]              ; [12]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m7,        m0
    pmaddwd     m7,        [r5 - 6 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m5,        m7

    punpckhwd   m7,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m5
    punpcklwd   m6,        m5

    punpckldq   m5,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m7, m2
    punpckhdq   m7,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m5
    movhps      [r0 + r1],       m5
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r2,              [r0 + r1 * 4]
    movh        [r2],            m6
    movhps      [r2 + r1],       m6
    movh        [r2 + r1 * 2],   m7
    movhps      [r2 + r4],       m7

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 13 * 16]             ; [31]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 13 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5]                       ; [18]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 13 * 16]             ; [5]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 13 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    pmaddwd     m3,        [r5 + 6 * 16]              ; [24]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r5 + 6 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    punpckhwd   m5,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m3
    punpcklwd   m6,        m3

    punpckldq   m3,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m5, m2
    punpckhdq   m5,        m2

    movh        [r0 + 8],            m3
    movhps      [r0 + r1 + 8],       m3
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m5
    movhps      [r0 + r4 + 8],       m5

    RET

cglobal intra_pred_ang8_15, 4,6,8
    lea         r5,        [ang_table + 20 * 16]
    add         r1,        r1

    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r5 - 5 * 16]              ; [15]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 - 5 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m0,        m3, 12
    movu        m1,        [r3]
    pshufb      m1,        [pw_ang8_15]
    palignr     m3,        m1, 12

    mova        m2,        m3
    pmaddwd     m2,        [r5 + 10 * 16]             ; [30]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 + 10 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 7 * 16]              ; [13]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 7 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m5,        m3
    pmaddwd     m5,        [r5 + 8 * 16]              ; [28]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m7,        m0
    pmaddwd     m7,        [r5 + 8 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m5,        m7

    punpckhwd   m7,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m5
    punpcklwd   m6,        m5

    punpckldq   m5,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m7, m2
    punpckhdq   m7,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m5
    movhps      [r0 + r1],       m5
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r2,              [r0 + r1 * 4]
    movh        [r2],            m6
    movhps      [r2 + r1],       m6
    movh        [r2 + r1 * 2],   m7
    movhps      [r2 + r4],       m7

    mova        m4,        m3
    pmaddwd     m4,        [r5 - 9 * 16]              ; [11]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 - 9 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m2,        m3
    pmaddwd     m2,        [r5 + 6 * 16]              ; [26]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 + 6 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 11 * 16]             ; [9]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 11 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12
    pinsrw      m3,        [r3 + 16], 0

    pmaddwd     m3,        [r5 + 4 * 16]              ; [24]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r5 + 4 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    punpckhwd   m5,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m3
    punpcklwd   m6,        m3

    punpckldq   m3,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m5, m2
    punpckhdq   m5,        m2

    movh        [r0 + 8],            m3
    movhps      [r0 + r1 + 8],       m3
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m5
    movhps      [r0 + r4 + 8],       m5

    RET

cglobal intra_pred_ang8_16, 4,6,8
    lea         r5,        [ang_table + 13 * 16]
    add         r1,        r1

    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r5 - 2 * 16]              ; [11]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 - 2 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m0,        m3, 12
    movu        m1,        [r3]
    pshufb      m1,        [pw_ang8_16]
    palignr     m3,        m1, 12

    mova        m2,        m3
    pmaddwd     m2,        [r5 + 9 * 16]              ; [22]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 + 9 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 12 * 16]             ; [1]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 12 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m5,        m3
    pmaddwd     m5,        [r5 - 1 * 16]              ; [12]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m7,        m0
    pmaddwd     m7,        [r5 - 1 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m5,        m7

    punpckhwd   m7,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m5
    punpcklwd   m6,        m5

    punpckldq   m5,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m7, m2
    punpckhdq   m7,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m5
    movhps      [r0 + r1],       m5
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r2,              [r0 + r1 * 4]
    movh        [r2],            m6
    movhps      [r2 + r1],       m6
    movh        [r2 + r1 * 2],   m7
    movhps      [r2 + r4],       m7

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 10 * 16]             ; [23]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 10 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5 - 11 * 16]             ; [2]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 11 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m6,        m3
    pmaddwd     m6,        [r5]                       ; [13]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12
    pinsrw      m3,        [r3 + 16], 0

    pmaddwd     m3,        [r5 + 11 * 16]             ; [24]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r5 + 11 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    punpckhwd   m5,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m3
    punpcklwd   m6,        m3

    punpckldq   m3,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m5, m2
    punpckhdq   m5,        m2

    movh        [r0 + 8],            m3
    movhps      [r0 + r1 + 8],       m3
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m5
    movhps      [r0 + r4 + 8],       m5

    RET

cglobal intra_pred_ang8_17, 4,6,8
    lea         r5,        [ang_table + 17 * 16]
    add         r1,        r1

    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r5 - 11 * 16]             ; [6]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 - 11 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m0,        m3, 12
    movu        m1,        [r3]
    pshufb      m1,        [pw_ang8_17]
    palignr     m3,        m1, 12

    mova        m2,        m3
    pmaddwd     m2,        [r5 - 5 * 16]              ; [12]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 5 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m6,        m3
    pmaddwd     m6,        [r5 + 1 * 16]              ; [18]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 + 1 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m5,        m3
    pmaddwd     m5,        [r5 + 7 * 16]              ; [24]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m7,        m0
    pmaddwd     m7,        [r5 + 7 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m5,        m7

    punpckhwd   m7,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m5
    punpcklwd   m6,        m5

    punpckldq   m5,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m7, m2
    punpckhdq   m7,        m2

    lea         r4,              [r1 * 3]
    movh        [r0],            m5
    movhps      [r0 + r1],       m5
    movh        [r0 + r1 * 2],   m4
    movhps      [r0 + r4],       m4
    lea         r2,              [r0 + r1 * 4]
    movh        [r2],            m6
    movhps      [r2 + r1],       m6
    movh        [r2 + r1 * 2],   m7
    movhps      [r2 + r4],       m7

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 13 * 16]             ; [30]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 13 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5 - 13 * 16]             ; [4]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 13 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 7 * 16]              ; [10]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 7 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    pmaddwd     m3,        [r5 - 1 * 16]              ; [16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r5 - 1 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    punpckhwd   m5,        m4, m2
    punpcklwd   m4,        m2
    punpckhwd   m2,        m6, m3
    punpcklwd   m6,        m3

    punpckldq   m3,        m4, m6
    punpckhdq   m4,        m6
    punpckldq   m6,        m5, m2
    punpckhdq   m5,        m2

    movh        [r0 + 8],            m3
    movhps      [r0 + r1 + 8],       m3
    movh        [r0 + r1 * 2 + 8],   m4
    movhps      [r0 + r4 + 8],       m4
    lea         r0,                  [r0 + r1 * 4]
    movh        [r0 + 8],            m6
    movhps      [r0 + r1 + 8],       m6
    movh        [r0 + r1 * 2 + 8],   m5
    movhps      [r0 + r4 + 8],       m5

    RET

cglobal intra_pred_ang8_18, 4,5,3
    add         r1,              r1
    lea         r4,              [r1 * 3]
    movu        m1,              [r3]
    movu        m0,              [r2 + 2]
    pshufb      m0,              [pw_swap16]
    movu        [r0],            m1
    palignr     m2,              m1, m0, 14
    movu        [r0 + r1],       m2
    palignr     m2,              m1, m0, 12
    movu        [r0 + r1 * 2],   m2
    palignr     m2,              m1, m0, 10
    movu        [r0 + r4],       m2
    lea         r0,              [r0 + r1 * 4]
    palignr     m2,              m1, m0, 8
    movu        [r0],            m2
    palignr     m2,              m1, m0, 6
    movu        [r0 + r1],       m2
    palignr     m2,              m1, m0, 4
    movu        [r0 + r1 * 2],   m2
    palignr     m1,              m0, 2
    movu        [r0 + r4],       m1
    RET

cglobal intra_pred_ang8_19, 4,6,8
    lea         r5,        [ang_table + 17 * 16]
    add         r1,        r1

    movu        m0,        [r3]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r3 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r5 - 11 * 16]             ; [6]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 - 11 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m0,        m3, 12
    movu        m1,        [r2]
    pshufb      m1,        [pw_ang8_17]
    palignr     m3,        m1, 12

    mova        m2,        m3
    pmaddwd     m2,        [r5 - 5 * 16]              ; [12]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 5 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m6,        m3
    pmaddwd     m6,        [r5 + 1 * 16]              ; [18]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 + 1 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m5,        m3
    pmaddwd     m5,        [r5 + 7 * 16]              ; [24]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m7,        m0
    pmaddwd     m7,        [r5 + 7 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m5,        m7

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 13 * 16]             ; [30]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 13 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5 - 13 * 16]             ; [4]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 13 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 7 * 16]              ; [10]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 7 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    pmaddwd     m3,        [r5 - 1 * 16]              ; [16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r5 - 1 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m3

    RET

cglobal intra_pred_ang8_20, 4,6,8
    lea         r5,        [ang_table + 13 * 16]
    add         r1,        r1

    movu        m0,        [r3]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r3 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r5 - 2 * 16]              ; [11]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 - 2 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m0,        m3, 12
    movu        m1,        [r2]
    pshufb      m1,        [pw_ang8_16]
    palignr     m3,        m1, 12

    mova        m2,        m3
    pmaddwd     m2,        [r5 + 9 * 16]              ; [22]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 + 9 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 12 * 16]             ; [1]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 12 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m5,        m3
    pmaddwd     m5,        [r5 - 1 * 16]              ; [12]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m7,        m0
    pmaddwd     m7,        [r5 - 1 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m5,        m7

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 10 * 16]             ; [23]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 10 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5 - 11 * 16]             ; [2]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 11 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m6,        m3
    pmaddwd     m6,        [r5]                       ; [13]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12
    pinsrw      m3,        [r2 + 16], 0

    pmaddwd     m3,        [r5 + 11 * 16]             ; [24]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r5 + 11 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m3

    RET

cglobal intra_pred_ang8_21, 4,6,8
    lea         r5,        [ang_table + 20 * 16]
    add         r1,        r1

    movu        m0,        [r3]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r3 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r5 - 5 * 16]              ; [15]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 - 5 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m0,        m3, 12
    movu        m1,        [r2]
    pshufb      m1,        [pw_ang8_15]
    palignr     m3,        m1, 12

    mova        m2,        m3
    pmaddwd     m2,        [r5 + 10 * 16]             ; [30]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 + 10 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 7 * 16]              ; [13]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 7 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m5,        m3
    pmaddwd     m5,        [r5 + 8 * 16]              ; [28]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m7,        m0
    pmaddwd     m7,        [r5 + 8 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m5,        m7

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m5

    mova        m4,        m3
    pmaddwd     m4,        [r5 - 9 * 16]              ; [11]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 - 9 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m2,        m3
    pmaddwd     m2,        [r5 + 6 * 16]              ; [26]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 + 6 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 11 * 16]             ; [9]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 11 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12
    pinsrw      m3,        [r2 + 16], 0

    pmaddwd     m3,        [r5 + 4 * 16]              ; [24]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r5 + 4 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m3

    RET

cglobal intra_pred_ang8_22, 4,6,8
    lea         r5,        [ang_table + 18 * 16]
    add         r1,        r1

    movu        m0,        [r3]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r3 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 1 * 16]              ; [19]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 1 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5 - 12 * 16]             ; [6]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5 - 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    palignr     m0,        m3, 12
    movu        m1,        [r2]
    pshufb      m1,        [pw_ang8_14]
    palignr     m3,        m1, 12

    mova        m6,        m3
    pmaddwd     m6,        [r5 + 7 * 16]              ; [25]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 + 7 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    mova        m5,        m3
    pmaddwd     m5,        [r5 - 6 * 16]              ; [12]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m7,        m0
    pmaddwd     m7,        [r5 - 6 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m5,        m7

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 13 * 16]             ; [31]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 13 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5]                       ; [18]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 13 * 16]             ; [5]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 13 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    pmaddwd     m3,        [r5 + 6 * 16]              ; [24]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r5 + 6 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m3

    RET

cglobal intra_pred_ang8_23, 4,6,8
    lea         r5,        [ang_table + 14 * 16]
    add         r1,        r1

    movu        m0,        [r3]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r3 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 9 * 16]              ; [23]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 9 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5]                       ; [14]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 9 * 16]              ; [5]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5 - 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m0,        m3, 12
    movu        m1,        [r2]
    pshufb      m1,        [pw_ang8_13]
    palignr     m3,        m1, 12

    mova        m5,        m3
    pmaddwd     m5,        [r5 + 14 * 16]             ; [28]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m7,        m0
    pmaddwd     m7,        [r5 + 14 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m5,        m7

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m5

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 5 * 16]              ; [19]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 5 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5 - 4 * 16]              ; [10]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 4 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m2,        m5

    mova        m6,        m3
    pmaddwd     m6,        [r5 - 13 * 16]             ; [1]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 - 13 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pslldq      m1,        2
    palignr     m0,        m3, 12
    palignr     m3,        m1, 12

    pmaddwd     m3,        [r5 + 10 * 16]             ; [24]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r5 + 10 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m3

    RET

cglobal intra_pred_ang8_24, 4,6,7
    lea         r5,        [ang_table + 16 * 16]
    add         r1,        r1

    movu        m0,        [r3]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r3 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r5 + 11 * 16]             ; [27]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 + 11 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5 + 6 * 16]              ; [22]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5 + 6 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r5 + 1 * 16]              ; [17]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5 + 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m5,        m3
    pmaddwd     m5,        [r5 - 4 * 16]              ; [12]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5 - 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m5

    mova        m4,        m3
    pmaddwd     m4,        [r5 - 9 * 16]              ; [7]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r5 - 9 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r5 - 14 * 16]             ; [2]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r5 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    palignr     m0,        m3, 12
    movu        m1,        [r2]
    pshufb      m1,        [pw_ang8_12]
    palignr     m3,        m1, 12

    mova        m6,        m3
    pmaddwd     m6,        [r5 + 13 * 16]             ; [29]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r5 + 13 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pmaddwd     m3,        [r5 + 8 * 16]              ; [24]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r5 + 8 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m3

    RET

cglobal intra_pred_ang8_25, 3,5,7
    mov         r2,        r3mp
    lea         r3,        [ang_table + 23 * 16]
    add         r1,        r1

    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 7 * 16]              ; [30]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 7 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 5 * 16]              ; [28]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 + 3 * 16]              ; [26]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 3 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m5,        m3
    pmaddwd     m5,        [r3 + 1 * 16]              ; [24]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m5

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 1 * 16]              ; [22]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 1 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 - 3 * 16]              ; [20]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 3 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 - 5 * 16]              ; [18]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r3 - 5 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pmaddwd     m3,        [r3 - 7 * 16]              ; [16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r3 - 7 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m3

    RET

cglobal intra_pred_ang8_26, 4,5,3
    movu        m0,             [r3 + 2]            ; [8 7 6 5 4 3 2 1]
    add         r1,             r1
    lea         r4,             [r1 * 3]

    movu        [r0],           m0
    movu        [r0 + r1],      m0
    movu        [r0 + r1 * 2],  m0
    movu        [r0 + r4],      m0

    lea         r3,             [r0 + r1 *4]
    movu        [r3],           m0
    movu        [r3 + r1],      m0
    movu        [r3 + r1 * 2],  m0
    movu        [r3 + r4],      m0

    cmp         r5m,            byte 0
    jz         .quit

    ; filter

    pshufb      m0,             [pw_unpackwdq]
    movh        m1,             [r2]                ; [3 2 1 0]
    pshufb      m2,             m1, [pw_unpackwdq]  ; [0 0 0 0 0 0 0 0]
    movu        m1,             [r2 + 2]            ; [8 7 6 5 4 3 2 1]
    psubw       m1,             m2
    psraw       m1,             1
    paddw       m0,             m1
    pxor        m1,             m1
    pmaxsw      m0,             m1
    pminsw      m0,             [pw_1023]
    pextrw      [r0],          m0, 0
    pextrw      [r0 + r1],     m0, 1
    pextrw      [r0 + r1 * 2], m0, 2
    pextrw      [r0 + r4],     m0, 3
    pextrw      [r3],          m0, 4
    pextrw      [r3 + r1],     m0, 5
    pextrw      [r3 + r1 * 2], m0, 6
    pextrw      [r3 + r4],     m0, 7

.quit:
    RET

cglobal intra_pred_ang8_27, 3,5,7
    mov         r2,        r3mp
    lea         r3,        [ang_table + 9 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 4]                   ; [9 8 7 6 5 4 3 2]

    punpcklwd   m3,        m0, m1                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m1                         ; [9 8 8 7 7 6 6 5]

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 7 * 16]              ; [2]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 7 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 - 5 * 16]              ; [4]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 - 3 * 16]              ; [6]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 3 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m5,        m3
    pmaddwd     m5,        [r3 - 1 * 16]              ; [8]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m5

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 1 * 16]              ; [10]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 1 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 3 * 16]              ; [12]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 3 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 + 5 * 16]              ; [14]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m5,        m0
    pmaddwd     m5,        [r3 + 5 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m6,        m5

    pmaddwd     m3,        [r3 + 7 * 16]              ; [16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r3 + 7 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m3

    RET

cglobal intra_pred_ang8_28, 3,5,7
    mov         r2,        r3mp
    lea         r3,        [ang_table + 17 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 4]                   ; [9 8 7 6 5 4 3 2]

    punpcklwd   m3,        m0, m1                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m1                         ; [9 8 8 7 7 6 6 5]

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 12 * 16]             ; [5]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 12 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 - 7 * 16]              ; [10]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 7 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 - 2 * 16]              ; [15]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 2 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m5,        m3
    pmaddwd     m5,        [r3 + 3 * 16]              ; [20]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 3 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m5

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 8 * 16]              ; [25]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 8 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 13 * 16]             ; [30]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 13 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    movh        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]

    palignr     m6,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m5,        m6
    pmaddwd     m6,        [r3 - 14 * 16]             ; [3]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m1,        m0, 4                      ; [10 9 9 8 8 7 7 6]
    mova        m3,        m1
    pmaddwd     m1,        [r3 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m5,        [r3 - 9 * 16]              ; [8]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    pmaddwd     m3,        [r3 - 9 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m5,        m3

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m5

    RET

cglobal intra_pred_ang8_29, 3,5,8
    mov         r2,        r3mp
    lea         r3,        [ang_table + 18 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 9 * 16]              ; [9]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 9 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3]                       ; [18]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 + 9 * 16]              ; [27]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m7,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    pmaddwd     m7,        [r3 - 14 * 16]             ; [4]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m1,        [r3 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m7

    palignr     m4,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m6,        m4
    pmaddwd     m4,        [r3 - 5 * 16]              ; [13]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m2,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    mova        m7,        m2
    pmaddwd     m2,        [r3 - 5 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    pmaddwd     m2,        m6, [r3 + 4 * 16]          ; [22]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    pmaddwd     m1,        m7, [r3 + 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 13 * 16]             ; [31]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 13 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m7,        [r3 - 10 * 16]             ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m5,        m0, 8                      ; [11 10 10 9 9 8 8 7]
    pmaddwd     m5,        [r3 - 10 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m7,        m5

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m7

    RET

cglobal intra_pred_ang8_30, 3,5,8
    mov         r2,        r3mp
    lea         r3,        [ang_table + 14 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 1 * 16]              ; [13]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 1 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 12 * 16]             ; [26]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    palignr     m6,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m7,        m6
    pmaddwd     m6,        [r3 - 7 * 16]              ; [7]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m1,        [r3 - 7 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m7,        [r3 + 6 * 16]              ; [20]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m1,        [r3 + 6 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m7

    palignr     m4,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    mova        m6,        m4
    pmaddwd     m4,        [r3 - 13 * 16]             ; [1]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m2,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    mova        m7,        m2
    pmaddwd     m2,        [r3 - 13 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    pmaddwd     m2,        m6, [r3]                   ; [14]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    pmaddwd     m1,        m7, [r3]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 13 * 16]             ; [27]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 13 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    pmaddwd     m7,        [r3 - 6 * 16]              ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m5,        m0, 12                     ; [12 11 11 10 10 9 9 8]
    pmaddwd     m5,        [r3 - 6 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m7,        m5

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m7

    RET

cglobal intra_pred_ang8_31, 3,5,8
    mov         r2,        r3mp
    lea         r3,        [ang_table + 13 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 4 * 16]              ; [17]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 4 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m6,        m2
    pmaddwd     m2,        [r3 - 11 * 16]             ; [2]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    mova        m7,        m1
    pmaddwd     m1,        [r3 - 11 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 6 * 16]              ; [19]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 6 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m7,        [r3 - 9 * 16]              ; [4]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m1,        [r3 - 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m7

    palignr     m4,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m4,        [r3 + 8 * 16]              ; [21]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m2,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m2,        [r3 + 8 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    mova        m6,        m2
    pmaddwd     m2,        [r3 - 7 * 16]              ; [6]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m1,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    mova        m7,        m1
    pmaddwd     m1,        [r3 - 7 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 10 * 16]             ; [23]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 10 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    mova        m7,        m0
    pmaddwd     m7,        [r3 - 5 * 16]              ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m5
    pmaddwd     m1,        [r3 - 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m7

    RET

cglobal intra_pred_ang8_32, 3,6,8
    mov         r2,        r3mp
    lea         r3,        [ang_table + 19 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 2 * 16]              ; [21]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 2 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m6,        m2
    pmaddwd     m2,        [r3 - 9 * 16]              ; [10]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    mova        m7,        m1
    pmaddwd     m1,        [r3 - 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 12 * 16]             ; [31]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 12 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m7,        [r3 + 1 * 16]              ; [20]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m1,        [r3 + 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m7

    palignr     m4,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    mova        m2,        m4
    pmaddwd     m4,        [r3 - 10 * 16]             ; [ 9]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m3,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    mova        m6,        m3
    pmaddwd     m3,        [r3 - 10 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m4,        m3

    pmaddwd     m2,        [r3 + 11 * 16]             ; [30]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    pmaddwd     m6,        [r3 + 11 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    mova        m6,        m0
    pmaddwd     m6,        [r3]                       ; [19]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m7,        m5
    pmaddwd     m7,        [r3]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    movh        m1,        [r2 + 26]                  ; [16 15 14 13]
    palignr     m7,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m7,        [r3 - 11 * 16]             ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, 4                      ; [14 13 13 12 12 11 11 10]
    pmaddwd     m1,        [r3 - 11 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m7

    RET

cglobal intra_pred_ang8_33, 3,5,8
    mov         r2,        r3mp
    lea         r3,        [ang_table + 14 * 16]
    add         r1,        r1

    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]
    punpckhwd   m1,        m4                         ; [x 16 16 15 15 14 14 13]

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 12 * 16]             ; [26]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 12 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    pmaddwd     m2,        [r3 + 6 * 16]              ; [20]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m6,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m6,        [r3 + 6 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    palignr     m6,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m6,        [r3]                       ; [14]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m7,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m7,        [r3]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    pmaddwd     m7,        [r3 - 6 * 16]              ; [ 8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m3,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    pmaddwd     m3,        [r3 - 6 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m7,        m3

    lea         r4,              [r1 * 3]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m7

    mova        m4,        m0
    pmaddwd     m4,        [r3 - 12 * 16]             ; [ 2]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m5
    pmaddwd     m2,        [r3 - 12 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m0
    pmaddwd     m2,        [r3 + 14 * 16]             ; [28]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m5
    pmaddwd     m6,        [r3 + 14 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    palignr     m6,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m6,        [r3 + 8 * 16]              ; [22]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m7,        m1, m5, 4                  ; [14 13 13 12 12 11 11 10]
    pmaddwd     m7,        [r3 + 8 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m7,        [r3 + 2 * 16]              ; [16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, 8                      ; [15 14 14 13 13 12 12 11]
    pmaddwd     m1,        [r3 + 2 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r0,              [r0 + r1 * 4]
    movu        [r0],            m4
    movu        [r0 + r1],       m2
    movu        [r0 + r1 * 2],   m6
    movu        [r0 + r4],       m7

    RET

;-----------------------------------------------------------------------------
; void intraPredAng16(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal intra_pred_ang16_2, 3,4,5
    cmp         r4m,                byte 34
    cmove       r2,                 r3mp
    add         r1,                 r1
    lea         r3,                 [r1 * 3]
    movu        m0,                 [r2 + 4]
    movu        m1,                 [r2 + 20]
    movu        m2,                 [r2 + 36]

    movu        [r0],               m0
    movu        [r0 + 16],          m1
    palignr     m3,                 m1, m0, 2
    palignr     m4,                 m2, m1, 2
    movu        [r0 + r1],          m3
    movu        [r0 + r1 + 16],     m4
    palignr     m3,                 m1, m0, 4
    palignr     m4,                 m2, m1, 4
    movu        [r0 + r1 * 2],      m3
    movu        [r0 + r1 * 2 + 16], m4
    palignr     m3,                 m1, m0, 6
    palignr     m4,                 m2, m1, 6
    movu        [r0 + r3],          m3
    movu        [r0 + r3 + 16],     m4

    lea         r0,                 [r0 + r1 * 4]
    palignr     m3,                 m1, m0, 8
    palignr     m4,                 m2, m1, 8
    movu        [r0],               m3
    movu        [r0 + 16],          m4
    palignr     m3,                 m1, m0, 10
    palignr     m4,                 m2, m1, 10
    movu        [r0 + r1],          m3
    movu        [r0 + r1 + 16],     m4
    palignr     m3,                 m1, m0, 12
    palignr     m4,                 m2, m1, 12
    movu        [r0 + r1 * 2],      m3
    movu        [r0 + r1 * 2 + 16], m4
    palignr     m3,                 m1, m0, 14
    palignr     m4,                 m2, m1, 14
    movu        [r0 + r3],          m3
    movu        [r0 + r3 + 16],     m4

    movu        m0,                 [r2 + 52]
    lea         r0,                 [r0 + r1 * 4]
    movu        [r0],               m1
    movu        [r0 + 16],          m2
    palignr     m3,                 m2, m1, 2
    palignr     m4,                 m0, m2, 2
    movu        [r0 + r1],          m3
    movu        [r0 + r1 + 16],     m4
    palignr     m3,                 m2, m1, 4
    palignr     m4,                 m0, m2, 4
    movu        [r0 + r1 * 2],      m3
    movu        [r0 + r1 * 2 + 16], m4
    palignr     m3,                 m2, m1, 6
    palignr     m4,                 m0, m2, 6
    movu        [r0 + r3],          m3
    movu        [r0 + r3 + 16],     m4

    lea         r0,                 [r0 + r1 * 4]
    palignr     m3,                 m2, m1, 8
    palignr     m4,                 m0, m2, 8
    movu        [r0],               m3
    movu        [r0 + 16],          m4
    palignr     m3,                 m2, m1, 10
    palignr     m4,                 m0, m2, 10
    movu        [r0 + r1],          m3
    movu        [r0 + r1 + 16],     m4
    palignr     m3,                 m2, m1, 12
    palignr     m4,                 m0, m2, 12
    movu        [r0 + r1 * 2],      m3
    movu        [r0 + r1 * 2 + 16], m4
    palignr     m3,                 m2, m1, 14
    palignr     m4,                 m0, m2, 14
    movu        [r0 + r3],          m3
    movu        [r0 + r3 + 16],     m4

    RET

%macro TRANSPOSE_STORE 6
    jnz         .skip%6
    punpckhwd   %5,        %1, %2
    punpcklwd   %1,        %2
    punpckhwd   %2,        %3, %4
    punpcklwd   %3,        %4

    punpckldq   %4,        %1, %3
    punpckhdq   %1,        %3
    punpckldq   %3,        %5, %2
    punpckhdq   %5,        %2

    movh        [r0 + %6],            %4
    movhps      [r0 + r1 + %6],       %4
    movh        [r0 + r1 * 2 + %6],   %1
    movhps      [r0 + r4 + %6],       %1
    lea         r5,                   [r0 + r1 * 4]
    movh        [r5 + %6],            %3
    movhps      [r5 + r1 + %6],       %3
    movh        [r5 + r1 * 2 + %6],   %5
    movhps      [r5 + r4 + %6],       %5
    jmp         .end%6

.skip%6:
    movu        [r5],            %1
    movu        [r5 + r1],       %2
    movu        [r5 + r1 * 2],   %3
    movu        [r5 + r4],       %4
.end%6:
%endmacro

INIT_XMM sse4
cglobal ang16_mode_3_33
    test        r6d,       r6d
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]
    punpckhwd   m1,        m4                         ; [x 16 16 15 15 14 14 13]

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 10 * 16]             ; [26]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 10 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    pmaddwd     m2,        [r3 + 4 * 16]              ; [20]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m6,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m6,        [r3 + 4 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    palignr     m6,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m6,        [r3 - 2 * 16]              ; [14]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m7,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m7,        [r3 - 2 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    pmaddwd     m7,        [r3 - 8 * 16]              ; [ 8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m3,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    pmaddwd     m3,        [r3 - 8 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m7,        m3

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m3, 0

    mova        m4,        m0
    pmaddwd     m4,        [r3 - 14 * 16]             ; [ 2]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m5
    pmaddwd     m2,        [r3 - 14 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m0
    pmaddwd     m2,        [r3 + 12 * 16]             ; [28]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m5
    pmaddwd     m6,        [r3 + 12 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    palignr     m6,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m6,        [r3 + 6 * 16]              ; [22]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m7,        m1, m5, 4                  ; [14 13 13 12 12 11 11 10]
    pmaddwd     m7,        [r3 + 6 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m7,        [r3]                       ; [16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, 8                      ; [15 14 14 13 13 12 12 11]
    pmaddwd     m1,        [r3]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m3, 8

    movu        m1,        [r2 + 26]                  ; [20 19 18 17 16 15 14 13]
    psrldq      m4,        m1, 2                      ; [x 20 19 18 17 16 15 14]

    punpcklwd   m3,        m1, m4                     ; [17 16 16 15 15 14 14 13]
    punpckhwd   m1,        m4                         ; [x 20 20 19 19 18 18 17]

    palignr     m4,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    pmaddwd     m4,        [r3 - 6 * 16]              ; [10]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m2,        m3, m5, 12                 ; [15 16 15 14 14 13 13 12]
    pmaddwd     m2,        [r3 - 6 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m5
    pmaddwd     m2,        [r3 - 12 * 16]             ; [4]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m3
    pmaddwd     m6,        [r3 - 12 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    mova        m6,        m5
    pmaddwd     m6,        [r3 + 14 * 16]             ; [30]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m7,        m3
    pmaddwd     m7,        [r3 + 14 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m3, m5, 4                  ; [14 13 13 12 12 11 11 10]
    pmaddwd     m7,        [r3 + 8 * 16]              ; [24]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m0,        m1, m3, 4                  ; [18 17 17 16 16 15 15 14]
    pmaddwd     m0,        [r3 + 8 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m7,        m0

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m0, 16

    palignr     m4,        m3, m5, 8                  ; [15 14 14 13 13 12 12 11]
    pmaddwd     m4,        [r3 + 2 * 16]              ; [18]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m2,        m1, m3, 8                  ; [19 18 18 17 17 16 16 15]
    pmaddwd     m2,        [r3 + 2 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m3, m5, 12                 ; [16 15 15 14 14 13 13 12]
    pmaddwd     m2,        [r3 - 4 * 16]              ; [12]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m6,        m1, m3, 12                 ; [20 19 19 18 18 17 17 16]
    pmaddwd     m6,        [r3 - 4 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    pinsrw      m1,        [r2 + 42], 7
    pmaddwd     m3,        [r3 - 10 * 16]             ; [6]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m1,        [r3 - 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m3,        m1

    movu        m7,        [r2 + 28]

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m3, m7, m0, 24

    ret

cglobal intra_pred_ang16_3, 3,7,8
    xor         r6d,       r6d
    lea         r3,        [ang_table + 16 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_3_33

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + r1 * 8]

    call        ang16_mode_3_33

    RET

cglobal intra_pred_ang16_33, 4,7,8
    xor         r6d,       r6d
    inc         r6d
    mov         r2,        r3
    lea         r3,        [ang_table + 16 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_3_33

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + 16]

    call        ang16_mode_3_33

    RET

cglobal ang16_mode_4_32
    test        r6d,       r6d
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 3 * 16]              ; [21]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 3 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m6,        m2
    pmaddwd     m2,        [r3 - 8 * 16]              ; [10]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    mova        m7,        m1
    pmaddwd     m1,        [r3 - 8 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 13 * 16]             ; [31]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 13 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m7,        [r3 + 2 * 16]              ; [20]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m1,        [r3 + 2 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    palignr     m4,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    mova        m2,        m4
    pmaddwd     m4,        [r3 - 9 * 16]              ; [9]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m7,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    mova        m6,        m7
    pmaddwd     m7,        [r3 - 9 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m4,        m7

    pmaddwd     m2,        [r3 + 12 * 16]             ; [30]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    pmaddwd     m6,        [r3 + 12 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    mova        m6,        m0
    pmaddwd     m6,        [r3 + 1 * 16]              ; [19]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m7,        m5
    pmaddwd     m7,        [r3 + 1 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    movu        m1,        [r2 + 26]                  ; [20 19 18 17 16 15 14 13]

    palignr     m7,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m7,        [r3 - 10 * 16]             ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m3,        m1, m5, 4                  ; [14 13 13 12 12 11 11 10]
    pmaddwd     m3,        [r3 - 10 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m7,        m3

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m3, 8

    psrldq      m4,        m1, 2                      ; [x 20 19 18 17 16 15 14]

    punpcklwd   m3,        m1, m4                     ; [17 16 16 15 15 14 14 13]
    punpckhwd   m1,        m4                         ; [x 20 20 19 19 18 18 17]

    palignr     m4,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m4,        [r3 + 11 * 16]             ; [29]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m2,        m3, m5, 4                  ; [14 13 13 12 12 11 11 10]
    pmaddwd     m2,        [r3 + 11 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m2,        [r3]                       ; [18]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m6,        m3, m5, 8                  ; [15 14 14 13 13 12 12 11]
    pmaddwd     m6,        [r3]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    palignr     m6,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    mova        m7,        m6
    pmaddwd     m6,        [r3 - 11 * 16]             ; [7]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m0,        m3, m5, 12                 ; [15 16 15 14 14 13 13 12]
    pmaddwd     m0,        [r3 - 11 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m6,        m0

    pmaddwd     m7,        [r3 + 10 * 16]             ; [28]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m0,        m3, m5, 12                 ; [15 16 15 14 14 13 13 12]
    pmaddwd     m0,        [r3 + 10 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m7,        m0

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m0, 16

    mova        m4,        m5
    pmaddwd     m4,        [r3 - 1 * 16]              ; [17]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m3
    pmaddwd     m2,        [r3 - 1 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m3, m5, 4                  ; [14 13 13 12 12 11 11 10]
    mova        m7,        m2
    pmaddwd     m2,        [r3 - 12 * 16]             ; [6]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m6,        m1, m3, 4                  ; [18 17 17 16 16 15 15 14]
    mova        m0,        m6
    pmaddwd     m6,        [r3 - 12 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    pmaddwd     m7,        [r3 + 9 * 16]              ; [27]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    pmaddwd     m0,        [r3 + 9 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m7,        m0

    palignr     m0,        m3, m5, 8                  ; [15 14 14 13 13 12 12 11]
    pmaddwd     m0,        [r3 - 2 * 16]              ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    palignr     m1,        m3, 8                      ; [19 18 18 17 17 16 16 15]
    pmaddwd     m1,        [r3 - 2 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m0,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m7, m0, m3, 24

    ret

cglobal intra_pred_ang16_4, 3,7,8
    xor         r6d,       r6d
    lea         r3,        [ang_table + 18 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_4_32

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + r1 * 8]

    call        ang16_mode_4_32

    RET

cglobal intra_pred_ang16_32, 4,7,8
    xor         r6d,       r6d
    inc         r6d
    mov         r2,        r3
    lea         r3,        [ang_table + 18 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_4_32

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + 16]

    call        ang16_mode_4_32

    RET

cglobal ang16_mode_5_31
    test        r6d,       r6d
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 1 * 16]              ; [17]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 1 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m6,        m2
    pmaddwd     m2,        [r3 - 14 * 16]             ; [2]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    mova        m7,        m1
    pmaddwd     m1,        [r3 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 3 * 16]              ; [19]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 3 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m7,        [r3 - 12 * 16]             ; [4]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m1,        [r3 - 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    palignr     m4,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m4,        [r3 + 5 * 16]              ; [21]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m7,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m7,        [r3 + 5 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m4,        m7

    palignr     m2,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    mova        m6,        m2
    pmaddwd     m2,        [r3 - 10 * 16]             ; [6]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m1,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    mova        m7,        m1
    pmaddwd     m1,        [r3 - 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 7 * 16]              ; [23]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 7 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    mova        m7,        m0
    pmaddwd     m7,        [r3 - 8 * 16]              ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m3,        m5
    pmaddwd     m3,        [r3 - 8 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m7,        m3

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m3, 8

    movu        m1,        [r2 + 26]                  ; [20 19 18 17 16 15 14 13]
    psrldq      m4,        m1, 2                      ; [x 20 19 18 17 16 15 14]

    punpcklwd   m3,        m1, m4                     ; [17 16 16 15 15 14 14 13]

    mova        m4,        m0
    pmaddwd     m4,        [r3 + 9 * 16]              ; [25]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m5
    pmaddwd     m2,        [r3 + 9 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    mova        m6,        m2
    pmaddwd     m2,        [r3 - 6 * 16]              ; [10]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m7,        m3, m5, 4                  ; [14 13 13 12 12 11 11 10]
    mova        m1,        m7
    pmaddwd     m7,        [r3 - 6 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m2,        m7

    pmaddwd     m6,        [r3 + 11 * 16]             ; [27]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m1,        [r3 + 11 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m7,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m7,        [r3 - 4 * 16]              ; [12]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m3, m5, 8                  ; [15 14 14 13 13 12 12 11]
    pmaddwd     m1,        [r3 - 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 16

    palignr     m4,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m4,        [r3 + 13 * 16]             ; [29]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m2,        m3, m5, 8                  ; [15 14 14 13 13 12 12 11]
    pmaddwd     m2,        [r3 + 13 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m2,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    mova        m7,        m2
    pmaddwd     m2,        [r3 - 2 * 16]              ; [14]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    palignr     m6,        m3, m5, 12                 ; [15 16 15 14 14 13 13 12]
    mova        m0,        m6
    pmaddwd     m6,        [r3 - 2 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    pmaddwd     m7,        [r3 + 15 * 16]             ; [31]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    pmaddwd     m0,        [r3 + 15 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m7,        m0

    pmaddwd     m5,        [r3]                       ; [16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    pmaddwd     m3,        [r3]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m5,        m3

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m7, m5, m3, 24

    ret

cglobal intra_pred_ang16_5, 3,7,8
    xor         r6d,       r6d
    lea         r3,        [ang_table + 16 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_5_31

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + r1 * 8]

    call        ang16_mode_5_31

    RET

cglobal intra_pred_ang16_31, 4,7,8
    xor         r6d,       r6d
    inc         r6d
    mov         r2,        r3
    lea         r3,        [ang_table + 16 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_5_31

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + 16]

    call        ang16_mode_5_31

    RET

cglobal ang16_mode_6_30
    test        r6d,       r6d
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 2 * 16]              ; [13]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 2 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 11 * 16]             ; [26]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 11 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    palignr     m6,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m7,        m6
    pmaddwd     m6,        [r3 - 8 * 16]              ; [7]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m1,        [r3 - 8 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m7,        [r3 + 5 * 16]              ; [20]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m1,        [r3 + 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    palignr     m4,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    mova        m6,        m4
    pmaddwd     m4,        [r3 - 14 * 16]             ; [1]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m1,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    mova        m7,        m1
    pmaddwd     m1,        [r3 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m6
    pmaddwd     m2,        [r3 - 1 * 16]              ; [14]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m7
    pmaddwd     m1,        [r3 - 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 12 * 16]             ; [27]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 12 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    pmaddwd     m7,        [r3 - 7 * 16]              ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    pmaddwd     m1,        [r3 - 7 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 8

    palignr     m4,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    pmaddwd     m4,        [r3 + 6 * 16]              ; [21]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m2,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    pmaddwd     m2,        [r3 + 6 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m0
    pmaddwd     m2,        [r3 - 13 * 16]             ; [2]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m7,        m5
    pmaddwd     m7,        [r3 - 13 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m2,        m7

    mova        m6,        m0
    pmaddwd     m6,        [r3]                       ; [15]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m5
    pmaddwd     m1,        [r3]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m7,        m0
    pmaddwd     m7,        [r3 + 13 * 16]             ; [28]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m5
    pmaddwd     m1,        [r3 + 13 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 16

    movh        m3,        [r2 + 26]                  ; [16 15 14 13]

    palignr     m4,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    mova        m2,        m4
    pmaddwd     m4,        [r3 - 6 * 16]              ; [9]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m1,        m3, m5, 4                  ; [14 13 13 12 12 11 11 10]
    mova        m6,        m1
    pmaddwd     m1,        [r3 - 6 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m2,        [r3 + 7 * 16]              ; [22]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m6
    pmaddwd     m1,        [r3 + 7 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    psrldq      m3,        2
    palignr     m7,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    mova        m5,        m7
    pmaddwd     m7,        [r3 - 12 * 16]             ; [3]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m3,        m6, 4                      ; [15 14 14 13 13 12 12 11]
    mova        m1,        m3
    pmaddwd     m3,        [r3 - 12 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m7,        m3

    pmaddwd     m5,        [r3 + 1 * 16]              ; [16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    pmaddwd     m1,        [r3 + 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m7, m5, m3, 24

    ret

cglobal intra_pred_ang16_6, 3,7,8
    xor         r6d,       r6d
    lea         r3,        [ang_table + 15 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_6_30

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + r1 * 8]

    call        ang16_mode_6_30

    RET

cglobal intra_pred_ang16_30, 4,7,8
    xor         r6d,       r6d
    inc         r6d
    mov         r2,        r3
    lea         r3,        [ang_table + 15 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_6_30

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + 16]

    call        ang16_mode_6_30

    RET

cglobal ang16_mode_7_29
    test        r6d,       r6d
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 8 * 16]              ; [9]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 8 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 1 * 16]              ; [18]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 + 10 * 16]             ; [27]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m7,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    pmaddwd     m7,        [r3 - 13 * 16]             ; [4]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m1,        [r3 - 13 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    palignr     m4,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m6,        m4
    pmaddwd     m4,        [r3 - 4 * 16]              ; [13]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    mova        m7,        m1
    pmaddwd     m1,        [r3 - 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m6
    pmaddwd     m2,        [r3 + 5 * 16]              ; [22]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m7
    pmaddwd     m1,        [r3 + 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m6,        [r3 + 14 * 16]             ; [31]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m7,        [r3 + 14 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    pmaddwd     m7,        [r3 - 9 * 16]              ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    pmaddwd     m1,        [r3 - 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 8

    palignr     m4,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    mova        m2,        m4
    pmaddwd     m4,        [r3]                       ; [17]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m1,        m5, m0, 8                  ; [11 10 10 9 9 8 8 7]
    mova        m7,        m1
    pmaddwd     m1,        [r3]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m2,        [r3 + 9 * 16]              ; [26]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    pmaddwd     m7,        [r3 + 9 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m2,        m7

    palignr     m6,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    pmaddwd     m6,        [r3 - 14 * 16]             ; [3]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m1,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    pmaddwd     m1,        [r3 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m7,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    pmaddwd     m7,        [r3 - 5 * 16]             ; [12]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    pmaddwd     m1,        [r3 - 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 16

    palignr     m4,        m0, m3, 12                 ; [8 7 7 6 6 5 5 4]
    mova        m2,        m4
    pmaddwd     m4,        [r3 + 4 * 16]              ; [21]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m1,        m5, m0, 12                 ; [12 11 11 10 10 9 9 8]
    mova        m3,        m1
    pmaddwd     m1,        [r3 + 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m2,        [r3 + 13 * 16]             ; [30]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    pmaddwd     m3,        [r3 + 13 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m2,        m3

    mova        m7,        m0
    pmaddwd     m7,        [r3 - 10 * 16]             ; [7]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m3,        m5
    pmaddwd     m3,        [r3 - 10 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m7,        m3

    pmaddwd     m0,        [r3 - 1 * 16]              ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    pmaddwd     m5,        [r3 - 1 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m0,        m5

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m7, m0, m3, 24

    ret

cglobal intra_pred_ang16_7, 3,7,8
    xor         r6d,       r6d
    lea         r3,        [ang_table + 17 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_7_29

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + r1 * 8]

    call        ang16_mode_7_29

    RET

cglobal intra_pred_ang16_29, 4,7,8
    xor         r6d,       r6d
    inc         r6d
    mov         r2,        r3
    lea         r3,        [ang_table + 17 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_7_29

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + 16]

    call        ang16_mode_7_29

    RET

cglobal ang16_mode_8_28
    test        r6d,       r6d
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m2,        m1, m0, 2                  ; [9 8 7 6 5 4 3 2]
    psrldq      m4,        m1, 2                      ; [x 16 15 14 13 12 11 10]

    punpcklwd   m3,        m0, m2                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m2                         ; [9 8 8 7 7 6 6 5]
    punpcklwd   m5,        m1, m4                     ; [13 12 12 11 11 10 10 9]

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 10 * 16]             ; [5]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 10 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 - 5 * 16]              ; [10]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3]                       ; [15]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r3 + 5 * 16]              ; [20]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 10 * 16]             ; [25]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 15 * 16]             ; [30]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 15 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    palignr     m6,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    pmaddwd     m6,        [r3 - 12 * 16]             ; [3]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    palignr     m7,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m7,        [r3 - 12 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    palignr     m7,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    pmaddwd     m7,        [r3 - 7 * 16]              ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m1,        [r3 - 7 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 8

    palignr     m4,        m0, m3, 4                  ; [6 5 5 4 4 3 3 2]
    mova        m7,        m4
    pmaddwd     m4,        [r3 - 2 *16]               ; [13]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m6,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    mova        m1,        m6
    pmaddwd     m6,        [r3 - 2 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m4,        m6

    mova        m2,        m7
    pmaddwd     m2,        [r3 + 3 * 16]              ; [18]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m1
    pmaddwd     m6,        [r3 + 3 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    mova        m6,        m7
    pmaddwd     m6,        [r3 + 8 * 16]              ; [23]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    pmaddwd     m1,        [r3 + 8 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m7,        [r3 + 13 * 16]             ; [28]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    palignr     m1,        m5, m0, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m1,        [r3 + 13 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 16

    palignr     m1,        m0, m3, 8                  ; [7 6 6 5 5 4 4 3]
    mova        m4,        m1
    pmaddwd     m4,        [r3 - 14 * 16]             ; [1]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    palignr     m5,        m0, 8                      ; [11 10 10 9 9 8 8 7]
    mova        m0,        m5
    pmaddwd     m0,        [r3 - 14 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m4,        m0

    mova        m2,        m1
    pmaddwd     m2,        [r3 - 9 * 16]              ; [6]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m3,        m5
    pmaddwd     m3,        [r3 - 9 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m2,        m3

    mova        m7,        m1
    pmaddwd     m7,        [r3 - 4 * 16]              ; [11]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m3,        m5
    pmaddwd     m3,        [r3 - 4 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    packusdw    m7,        m3

    pmaddwd     m1,        [r3 + 1 * 16]              ; [16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    pmaddwd     m5,        [r3 + 1 * 16]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m1,        m5

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m7, m1, m3, 24

    ret

cglobal intra_pred_ang16_8, 3,7,8
    xor         r6d,       r6d
    lea         r3,        [ang_table + 15 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_8_28

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + r1 * 8]

    call        ang16_mode_8_28

    RET

cglobal intra_pred_ang16_28, 4,7,8
    xor         r6d,       r6d
    inc         r6d
    mov         r2,        r3
    lea         r3,        [ang_table + 15 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_8_28

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + 16]

    call        ang16_mode_8_28

    RET

cglobal ang16_mode_9_27
    test        r6d,       r6d
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m1,        [r2 + 4]                   ; [9 8 7 6 5 4 3 2]

    punpcklwd   m3,        m0, m1                     ; [5 4 4 3 3 2 2 1]
    punpckhwd   m0,        m1                         ; [9 8 8 7 7 6 6 5]

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 14 * 16]             ; [2]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 - 14 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 - 12 * 16]             ; [4]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 - 10 *16]             ; [6]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r3 - 8 * 16]              ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 8 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 6 * 16]              ; [10]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 6 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r3 - 4 * 16]              ; [12]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 - 2 * 16]              ; [14]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m7,        m0
    pmaddwd     m7,        [r3 - 2 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    mova        m7,        m3
    pmaddwd     m7,        [r3]                       ; [16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 8

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 2 *16]               ; [18]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m6,        m0
    pmaddwd     m6,        [r3 + 2 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m4,        m6

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 4 * 16]              ; [20]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m0
    pmaddwd     m6,        [r3 + 4 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    mova        m6,        m3
    pmaddwd     m6,        [r3 + 6 * 16]              ; [22]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 6 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r3 + 8 * 16]              ; [24]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 8 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 16

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 10 * 16]             ; [26]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 12 * 16]             ; [28]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pmaddwd     m3,        [r3 + 14 * 16]             ; [30]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r3 + 14 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    movu        m7,        [r2 + 4]

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m3, m7, m1, 24

    ret

cglobal intra_pred_ang16_9, 3,7,8
    xor         r6d,       r6d
    lea         r3,        [ang_table + 16 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_9_27

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + r1 * 8]

    call        ang16_mode_9_27

    RET

cglobal intra_pred_ang16_27, 4,7,8
    xor         r6d,       r6d
    inc         r6d
    mov         r2,        r3
    lea         r3,        [ang_table + 16 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_9_27

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + 16]

    call        ang16_mode_9_27

    RET

cglobal ang16_mode_11_25
    test        r6d,       r6d
    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 14 * 16]             ; [30]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r3 + 14 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 12 * 16]             ; [28]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 + 10 *16]             ; [26]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r3 + 8 * 16]              ; [24]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 8 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    mova        m4,        m3
    pmaddwd     m4,        [r3 + 6 * 16]              ; [22]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 6 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r3 + 4 * 16]              ; [20]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 + 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r3 + 2 * 16]              ; [18]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m7,        m0
    pmaddwd     m7,        [r3 + 2 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    mova        m7,        m3
    pmaddwd     m7,        [r3]                       ; [16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 8

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 2 *16]               ; [14]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m6,        m0
    pmaddwd     m6,        [r3 - 2 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m4,        m6

    mova        m2,        m3
    pmaddwd     m2,        [r3 - 4 * 16]              ; [12]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m0
    pmaddwd     m6,        [r3 - 4 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    mova        m6,        m3
    pmaddwd     m6,        [r3 - 6 * 16]              ; [10]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 6 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r3 - 8 * 16]              ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 8 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 16

    mova        m4,        m3
    pmaddwd     m4,        [r3 - 10 * 16]             ; [6]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r3 - 12 * 16]             ; [4]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r3 - 14 * 16]             ; [2]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r3 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    movu        m3,        [r2]

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m7, m3, m1, 24

    ret

cglobal intra_pred_ang16_11, 3,7,8
    xor         r6d,       r6d
    lea         r3,        [ang_table + 16 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_11_25

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + r1 * 8]

    call        ang16_mode_11_25

    RET

cglobal intra_pred_ang16_25, 4,7,8
    xor         r6d,       r6d
    inc         r6d
    mov         r2,        r3
    lea         r3,        [ang_table + 16 * 16]
    add         r1,        r1
    lea         r4,        [r1 * 3]

    call        ang16_mode_11_25

    lea         r2,        [r2 + 16]
    lea         r0,        [r0 + 16]

    call        ang16_mode_11_25

    RET

cglobal ang16_mode_12_24
    test        r3d,       r3d
    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r6 + 11 * 16]             ; [27]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r6 + 11 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r6 + 6 * 16]              ; [22]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 6 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r6 + 1 *16]              ; [17]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r6 - 4 * 16]              ; [12]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    mova        m4,        m3
    pmaddwd     m4,        [r6 - 9 * 16]              ; [7]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r6 - 14 * 16]             ; [2]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m6,        m3
    pmaddwd     m6,        [r6 + 13 * 16]             ; [29]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m7,        m0
    pmaddwd     m7,        [r6 + 13 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 8 * 16]              ; [24]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 8 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 8

    mova        m4,        m3
    pmaddwd     m4,        [r6 + 3 *16]               ; [19]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m6,        m0
    pmaddwd     m6,        [r6 + 3 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m4,        m6

    mova        m2,        m3
    pmaddwd     m2,        [r6 - 2 * 16]              ; [14]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m0
    pmaddwd     m6,        [r6 - 2 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    mova        m6,        m3
    pmaddwd     m6,        [r6 - 7 * 16]              ; [9]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 7 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r6 - 12 * 16]             ; [4]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 16

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m4,        m3
    pmaddwd     m4,        [r6 + 15 * 16]             ; [31]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 15 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r6 + 10 * 16]             ; [26]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 5 * 16]              ; [21]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    pmaddwd     m3,        [r6]                       ; [16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r6]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m7, m3, m1, 24

    ret

cglobal intra_pred_ang16_12, 4,7,8
    add         r1,        r1
    lea         r4,        [r1 * 3]
    lea         r6,        [ang_table + 16 * 16]
    movu        m5,        [r3]
    pshufb      m5,        [pw_ang8_12]
    pinsrw      m5,        [r3 + 26], 5
    xor         r3d,       r3d

    call        ang16_mode_12_24

    lea         r0,        [r0 + r1 * 8]
    movu        m5,        [r2 + 2]
    lea         r2,        [r2 + 16]

    call        ang16_mode_12_24

    RET

cglobal intra_pred_ang16_24, 4,7,8
    xchg        r2,        r3
    add         r1,        r1
    lea         r4,        [r1 * 3]
    lea         r6,        [ang_table + 16 * 16]
    movu        m5,        [r3]
    pshufb      m5,        [pw_ang8_12]
    pinsrw      m5,        [r3 + 26], 5
    xor         r3d,       r3d
    inc         r3d

    call        ang16_mode_12_24

    lea         r0,        [r0 + 16]
    movu        m5,        [r2 + 2]
    lea         r2,        [r2 + 16]

    call        ang16_mode_12_24

    RET

cglobal ang16_mode_13_23
    test        r3d,       r3d
    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r6 + 8 * 16]              ; [23]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r6 + 8 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r6 - 1 * 16]              ; [14]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r6 - 10 *16]             ; [5]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 13 * 16]             ; [28]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 13 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    mova        m4,        m3
    pmaddwd     m4,        [r6 + 4 * 16]              ; [19]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r6 - 5 * 16]              ; [10]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r6 - 14 * 16]             ; [1]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m7,        m0
    pmaddwd     m7,        [r6 - 14 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 9 * 16]              ; [24]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 8

    mova        m4,        m3
    pmaddwd     m4,        [r6]                       ; [15]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m6,        m0
    pmaddwd     m6,        [r6]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m4,        m6

    mova        m2,        m3
    pmaddwd     m2,        [r6 - 9 * 16]              ; [6]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m0
    pmaddwd     m6,        [r6 - 9 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m6,        m3
    pmaddwd     m6,        [r6 + 14 * 16]             ; [29]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 5 * 16]              ; [20]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 16

    mova        m4,        m3
    pmaddwd     m4,        [r6 - 4 * 16]              ; [11]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r6 - 13 * 16]             ; [2]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 13 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 10 * 16]             ; [25]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    pmaddwd     m3,        [r6 + 1 * 16]              ; [16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r6 + 1 *16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m7, m3, m1, 24

    ret

cglobal intra_pred_ang16_13, 4,7,8
    add         r1,        r1
    lea         r4,        [r1 * 3]
    lea         r6,        [ang_table + 15 * 16]
    movu        m5,        [r3]
    pshufb      m5,        [pw_ang16_13]
    movu        m6,        [r3 + 14]
    pshufb      m6,        [pw_ang8_13]
    pslldq      m6,        2
    palignr     m5,        m6, 6
    xor         r3d,       r3d

    call        ang16_mode_13_23

    lea         r0,        [r0 + r1 * 8]
    movu        m5,        [r2 + 2]
    lea         r2,        [r2 + 16]

    call        ang16_mode_13_23

    RET

cglobal intra_pred_ang16_23, 4,7,8
    xchg        r2,        r3
    add         r1,        r1
    lea         r4,        [r1 * 3]
    lea         r6,        [ang_table + 15 * 16]
    movu        m5,        [r3]
    pshufb      m5,        [pw_ang16_13]
    movu        m6,        [r3 + 14]
    pshufb      m6,        [pw_ang8_13]
    pslldq      m6,        2
    palignr     m5,        m6, 6
    xor         r3d,       r3d
    inc         r3d

    call        ang16_mode_13_23

    lea         r0,        [r0 + 16]
    movu        m5,        [r2 + 2]
    lea         r2,        [r2 + 16]

    call        ang16_mode_13_23

    RET

cglobal ang16_mode_14_22
    test        r3d,       r3d
    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r6 + 1 * 16]              ; [19]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r6 + 1 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    mova        m2,        m3
    pmaddwd     m2,        [r6 - 12 * 16]             ; [6]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m6,        m3
    pmaddwd     m6,        [r6 + 7 * 16]              ; [25]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 7 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r6 - 6 * 16]              ; [12]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 6 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m4,        m3
    pmaddwd     m4,        [r6 + 13 * 16]             ; [31]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 13 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r6]                       ; [18]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r6 - 13 * 16]             ; [5]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m7,        m0
    pmaddwd     m7,        [r6 - 13 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 6 * 16]              ; [24]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 6 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 8

    mova        m4,        m3
    pmaddwd     m4,        [r6 - 7 * 16]              ; [11]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m6,        m0
    pmaddwd     m6,        [r6 - 7 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m4,        m6

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m2,        m3
    pmaddwd     m2,        [r6 + 12 * 16]             ; [30]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m0
    pmaddwd     m6,        [r6 + 12 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    mova        m6,        m3
    pmaddwd     m6,        [r6 - 1 * 16]              ; [17]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r6 - 14 * 16]             ; [4]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 16

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m4,        m3
    pmaddwd     m4,        [r6 + 5 * 16]              ; [23]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r6 - 8 * 16]              ; [10]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 8 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 11 * 16]             ; [29]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 11 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    pmaddwd     m3,        [r6 - 2 * 16]              ; [16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r6 - 2 *16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m7, m3, m1, 24

    ret

cglobal intra_pred_ang16_14, 4,7,8
    add         r1,        r1
    lea         r4,        [r1 * 3]
    lea         r6,        [ang_table + 18 * 16]
    movu        m6,        [r3]
    pshufb      m6,        [pw_ang8_14]
    movu        m5,        [r3 + 20]
    pshufb      m5,        [pw_ang8_14]
    punpckhqdq  m5,        m6
    xor         r3d,       r3d

    call        ang16_mode_14_22

    lea         r0,        [r0 + r1 * 8]
    movu        m5,        [r2 + 2]
    lea         r2,        [r2 + 16]

    call        ang16_mode_14_22

    RET

cglobal intra_pred_ang16_22, 4,7,8
    xchg        r2,        r3
    add         r1,        r1
    lea         r4,        [r1 * 3]
    lea         r6,        [ang_table + 18 * 16]
    movu        m6,        [r3]
    pshufb      m6,        [pw_ang8_14]
    movu        m5,        [r3 + 20]
    pshufb      m5,        [pw_ang8_14]
    punpckhqdq  m5,        m6
    xor         r3d,       r3d
    inc         r3d

    call        ang16_mode_14_22

    lea         r0,        [r0 + 16]
    movu        m5,        [r2 + 2]
    lea         r2,        [r2 + 16]

    call        ang16_mode_14_22

    RET

cglobal ang16_mode_15_21
    test        r3d,       r3d
    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    palignr     m6,        m0, m5, 2

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r6]                       ; [15]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r6]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m0,        m3, 12
    palignr     m3,        m6, 12

    mova        m2,        m3
    pmaddwd     m2,        [r6 + 15 * 16]             ; [30]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 15 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r6 - 2 * 16]              ; [13]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 2 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 13 * 16]             ; [28]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 13 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    mova        m4,        m3
    pmaddwd     m4,        [r6 - 4 * 16]              ; [11]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m2,        m3
    pmaddwd     m2,        [r6 + 11 * 16]             ; [26]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 11 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r6 - 6 * 16]              ; [9]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m7,        m0
    pmaddwd     m7,        [r6 - 6 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 9 * 16]              ; [24]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 8

    mova        m4,        m3
    pmaddwd     m4,        [r6 - 8 * 16]              ; [7]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m6,        m0
    pmaddwd     m6,        [r6 - 8 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m4,        m6

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m2,        m3
    pmaddwd     m2,        [r6 + 7 * 16]              ; [22]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m0
    pmaddwd     m6,        [r6 + 7 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    mova        m6,        m3
    pmaddwd     m6,        [r6 - 10 * 16]             ; [5]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 5 * 16]              ; [20]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 5 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 16

    mova        m4,        m3
    pmaddwd     m4,        [r6 - 12 * 16]             ; [3]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m2,        m3
    pmaddwd     m2,        [r6 + 3 * 16]              ; [18]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 3 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r6 - 14 * 16]             ; [1]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    pmaddwd     m3,        [r6 + 1 * 16]              ; [16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r6 + 1 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m7, m3, m1, 24

    ret

cglobal intra_pred_ang16_15, 4,7,8
    add         r1,        r1
    lea         r4,        [r1 * 3]
    lea         r6,        [ang_table + 15 * 16]
    movu        m6,        [r3 + 4]
    pshufb      m6,        [pw_ang8_15]
    movu        m5,        [r3 + 18]
    pshufb      m5,        [pw_ang8_15]
    punpckhqdq  m5,        m6
    xor         r3d,       r3d

    call        ang16_mode_15_21

    lea         r0,        [r0 + r1 * 8]
    movu        m5,        [r2]
    lea         r2,        [r2 + 16]

    call        ang16_mode_15_21

    RET

cglobal intra_pred_ang16_21, 4,7,8
    xchg        r2,        r3
    add         r1,        r1
    lea         r4,        [r1 * 3]
    lea         r6,        [ang_table + 15 * 16]
    movu        m6,        [r3 + 4]
    pshufb      m6,        [pw_ang8_15]
    movu        m5,        [r3 + 18]
    pshufb      m5,        [pw_ang8_15]
    punpckhqdq  m5,        m6
    xor         r3d,       r3d
    inc         r3d

    call        ang16_mode_15_21

    lea         r0,        [r0 + 16]
    movu        m5,        [r2]
    lea         r2,        [r2 + 16]

    call        ang16_mode_15_21

    RET

cglobal ang16_mode_16_20
    test        r4d,       r4d
    lea         r4,        [r1 * 3]
    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    palignr     m6,        m0, m5, 2

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r6 - 2 * 16]              ; [11]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r6 - 2 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m0,        m3, 12
    palignr     m3,        m6, 12

    mova        m2,        m3
    pmaddwd     m2,        [r6 + 9 * 16]              ; [22]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m6,        m3
    pmaddwd     m6,        [r6 - 12 * 16]             ; [1]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 - 1 * 16]              ; [12]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 1 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m4,        m3
    pmaddwd     m4,        [r6 + 10 * 16]             ; [23]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r6 - 11 * 16]             ; [2]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 11 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m6,        m3
    pmaddwd     m6,        [r6]                       ; [13]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m7,        m0
    pmaddwd     m7,        [r6]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 11 * 16]             ; [24]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 11 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 8

    mova        m4,        m3
    pmaddwd     m4,        [r6 - 10 * 16]             ; [3]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m6,        m0
    pmaddwd     m6,        [r6 - 10 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m4,        m6

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m2,        m3
    pmaddwd     m2,        [r6 + 1 * 16]              ; [14]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m0
    pmaddwd     m6,        [r6 + 1 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m6,        m3
    pmaddwd     m6,        [r6 + 12 * 16]             ; [25]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r6 - 9 * 16]              ; [4]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 9 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 16

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m4,        m3
    pmaddwd     m4,        [r6 + 2 * 16]              ; [15]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 2 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    movu        m5,        [r3]
    pshufb      m5,        [pw_ang8_16]

    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m2,        m3
    pmaddwd     m2,        [r6 + 13 * 16]             ; [26]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 13 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    mova        m7,        m3
    pmaddwd     m7,        [r6 - 8 * 16]              ; [5]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 8 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    pmaddwd     m3,        [r6 + 3 * 16]              ; [16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r6 + 3 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m7, m3, m1, 24

    ret

cglobal intra_pred_ang16_16, 4,7,8,0-(1*mmsize)
    add         r1,        r1
    lea         r6,        [ang_table + 13 * 16]
    movu        m6,        [r3 + 4]
    pshufb      m6,        [pw_ang16_16]
    movu        m5,        [r3 + 16]
    pshufb      m5,        [pw_ang16_16]
    punpckhqdq  m5,        m6
    mov         [rsp],     r3
    lea         r3,        [r3 + 24]
    xor         r4,        r4

    call        ang16_mode_16_20

    lea         r0,        [r0 + r1 * 8]
    mov         r3,        [rsp]
    movu        m5,        [r2]
    lea         r2,        [r2 + 16]
    xor         r4,        r4

    call        ang16_mode_16_20

    RET

cglobal intra_pred_ang16_20, 4,7,8,0-(1*mmsize)
    xchg        r2,        r3
    add         r1,        r1
    lea         r6,        [ang_table + 13 * 16]
    movu        m6,        [r3 + 4]
    pshufb      m6,        [pw_ang16_16]
    movu        m5,        [r3 + 16]
    pshufb      m5,        [pw_ang16_16]
    punpckhqdq  m5,        m6
    mov         [rsp],     r3
    lea         r3,        [r3 + 24]
    xor         r4,        r4
    inc         r4

    call        ang16_mode_16_20

    lea         r0,        [r0 + 16]
    mov         r3,        [rsp]
    movu        m5,        [r2]
    lea         r2,        [r2 + 16]
    xor         r4,        r4
    inc         r4

    call        ang16_mode_16_20

    RET

cglobal ang16_mode_17_19
    test        r4d,       r4d
    lea         r4,        [r1 * 3]
    movu        m0,        [r2]                       ; [7 6 5 4 3 2 1 0]
    movu        m1,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]

    palignr     m6,        m0, m5, 2

    punpcklwd   m3,        m0, m1                     ; [4 3 3 2 2 1 1 0]
    punpckhwd   m0,        m1                         ; [8 7 7 6 6 5 5 4]

    mova        m4,        m3
    pmaddwd     m4,        [r6 - 10 * 16]             ; [6]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m2,        m0
    pmaddwd     m2,        [r6 - 10 * 16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m4,        m2

    palignr     m0,        m3, 12
    palignr     m3,        m6, 12

    mova        m2,        m3
    pmaddwd     m2,        [r6 - 4 * 16]              ; [12]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m6,        m3
    pmaddwd     m6,        [r6 + 2 * 16]              ; [18]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 2 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 8 * 16]              ; [24]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 8 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    mov         r5,        r0

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 0

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m4,        m3
    pmaddwd     m4,        [r6 + 14 * 16]             ; [30]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    mova        m2,        m3
    pmaddwd     m2,        [r6 - 12 * 16]             ; [4]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 12 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m6,        m3
    pmaddwd     m6,        [r6 - 6 * 16]              ; [10]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m7,        m0
    pmaddwd     m7,        [r6 - 6 * 16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6]                      ; [16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r0 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 8

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m4,        m3
    pmaddwd     m4,        [r6 + 6 * 16]              ; [22]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m6,        m0
    pmaddwd     m6,        [r6 + 6 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m4,        m6

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m2,        m3
    pmaddwd     m2,        [r6 + 12 * 16]             ; [28]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m6,        m0
    pmaddwd     m6,        [r6 + 12 * 16]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m2,        m6

    mova        m6,        m3
    pmaddwd     m6,        [r6 - 14 * 16]             ; [2]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 14 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m5,        [r3]
    pshufb      m5,        [pw_ang8_17]

    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 - 8 * 16]              ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 8 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m6, m7, m1, 16

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m4,        m3
    pmaddwd     m4,        [r6 - 2 * 16]              ; [14]
    paddd       m4,        [pd_16]
    psrld       m4,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 - 2 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m2,        m3
    pmaddwd     m2,        [r6 + 4 * 16]              ; [20]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 4 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m2,        m1

    pslldq      m5,        2
    palignr     m0,        m3, 12
    palignr     m3,        m5, 12

    mova        m7,        m3
    pmaddwd     m7,        [r6 + 10 * 16]             ; [26]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    mova        m1,        m0
    pmaddwd     m1,        [r6 + 10 * 16]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m7,        m1

    pmaddwd     m3,        [r6 - 16 * 16]
    paddd       m3,        [pd_16]
    psrld       m3,        5
    pmaddwd     m0,        [r6 - 16 * 16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m3,        m0

    lea         r5,        [r5 + r1 * 4]

    TRANSPOSE_STORE m4, m2, m7, m3, m1, 24

    ret

cglobal intra_pred_ang16_17, 4,7,8,0-(1*mmsize)
    add         r1,        r1
    lea         r6,        [ang_table + 16 * 16]
    movu        m6,        [r3 + 2]
    pshufb      m6,        [pw_ang16_16]
    movu        m5,        [r3 + 12]
    pshufb      m5,        [pw_ang16_16]
    punpckhqdq  m5,        m6
    mov         [rsp],     r3
    lea         r3,        [r3 + 20]
    xor         r4,        r4

    call        ang16_mode_17_19

    lea         r0,        [r0 + r1 * 8]
    mov         r3,        [rsp]
    movu        m5,        [r2]
    lea         r2,        [r2 + 16]
    xor         r4,        r4

    call        ang16_mode_17_19

    RET

cglobal intra_pred_ang16_19, 4,7,8,0-(1*mmsize)
    xchg        r2,        r3
    add         r1,        r1
    lea         r6,        [ang_table + 16 * 16]
    movu        m6,        [r3 + 2]
    pshufb      m6,        [pw_ang16_16]
    movu        m5,        [r3 + 12]
    pshufb      m5,        [pw_ang16_16]
    punpckhqdq  m5,        m6
    mov         [rsp],     r3
    lea         r3,        [r3 + 20]
    xor         r4,        r4
    inc         r4

    call        ang16_mode_17_19

    lea         r0,        [r0 + 16]
    mov         r3,        [rsp]
    movu        m5,        [r2]
    lea         r2,        [r2 + 16]
    xor         r4,        r4
    inc         r4

    call        ang16_mode_17_19

    RET

cglobal intra_pred_ang16_18, 4,5,4
    add         r1,                  r1
    lea         r4,                  [r1 * 3]
    movu        m1,                  [r3]
    movu        m3,                  [r3 + 16]
    movu        m0,                  [r2 + 2]
    pshufb      m0,                  [pw_swap16]
    movu        [r0],                m1
    movu        [r0 + 16],           m3
    palignr     m2,                  m1, m0, 14
    movu        [r0 + r1],           m2
    palignr     m2,                  m3, m1, 14
    movu        [r0 + r1 + 16],      m2
    palignr     m2,                  m1, m0, 12
    movu        [r0 + r1 * 2],       m2
    palignr     m2,                  m3, m1, 12
    movu        [r0 + r1 * 2 + 16],  m2
    palignr     m2,                  m1, m0, 10
    movu        [r0 + r4],           m2
    palignr     m2,                  m3, m1, 10
    movu        [r0 + r4 + 16],      m2

    lea         r0,                  [r0 + r1 * 4]
    palignr     m2,                  m1, m0, 8
    movu        [r0],                m2
    palignr     m2,                  m3, m1, 8
    movu        [r0 + 16],           m2
    palignr     m2,                  m1, m0, 6
    movu        [r0 + r1],           m2
    palignr     m2,                  m3, m1, 6
    movu        [r0 + r1 + 16],      m2
    palignr     m2,                  m1, m0, 4
    movu        [r0 + r1 * 2],       m2
    palignr     m2,                  m3, m1, 4
    movu        [r0 + r1 * 2 + 16],  m2
    palignr     m2,                  m1, m0, 2
    movu        [r0 + r4],           m2
    palignr     m3,                  m1, 2
    movu        [r0 + r4 + 16],      m3

    lea         r0,                  [r0 + r1 * 4]
    movu        [r0],                m0
    movu        [r0 + 16],           m1
    movu        m3,                  [r2 + 18]
    pshufb      m3,                  [pw_swap16]
    palignr     m2,                  m0, m3, 14
    movu        [r0 + r1],           m2
    palignr     m2,                  m1, m0, 14
    movu        [r0 + r1 + 16],      m2
    palignr     m2,                  m0, m3, 12
    movu        [r0 + r1 * 2],       m2
    palignr     m2,                  m1, m0, 12
    movu        [r0 + r1 * 2 + 16],  m2
    palignr     m2,                  m0, m3, 10
    movu        [r0 + r4],           m2
    palignr     m2,                  m1, m0, 10
    movu        [r0 + r4 + 16],      m2

    lea         r0,                  [r0 + r1 * 4]
    palignr     m2,                  m0, m3, 8
    movu        [r0],                m2
    palignr     m2,                  m1, m0, 8
    movu        [r0 + 16],           m2
    palignr     m2,                  m0, m3, 6
    movu        [r0 + r1],           m2
    palignr     m2,                  m1, m0, 6
    movu        [r0 + r1 + 16],      m2
    palignr     m2,                  m0, m3, 4
    movu        [r0 + r1 * 2],       m2
    palignr     m2,                  m1, m0, 4
    movu        [r0 + r1 * 2 + 16],  m2
    palignr     m2,                  m0, m3, 2
    movu        [r0 + r4],           m2
    palignr     m1,                  m0, 2
    movu        [r0 + r4 + 16],      m1

    RET

cglobal intra_pred_ang16_10, 4,5,4
    movu        m1,                     [r2 + 2]            ; [8 7 6 5 4 3 2 1]
    movu        m3,                     [r2 + 18]           ; [16 15 14 13 12 11 10 9]
    pshufb      m0,                     m1, [pw_unpackwdq]  ; [1 1 1 1 1 1 1 1]
    add         r1,                     r1
    lea         r4,                     [r1 * 3]

    psrldq      m1,                     2
    pshufb      m2,                     m1, [pw_unpackwdq]  ; [2 2 2 2 2 2 2 2]
    movu        [r0 + r1],              m2
    movu        [r0 + r1 + 16],         m2
    psrldq      m1,                     2
    pshufb      m2,                     m1, [pw_unpackwdq]  ; [3 3 3 3 3 3 3 3]
    movu        [r0 + r1 * 2],          m2
    movu        [r0 + r1 * 2 + 16],     m2
    psrldq      m1,                     2
    pshufb      m2,                     m1, [pw_unpackwdq]  ; [4 4 4 4 4 4 4 4]
    movu        [r0 + r4],              m2
    movu        [r0 + r4 + 16],         m2

    lea         r2,                     [r0 + r1 *4]
    psrldq      m1,                     2
    pshufb      m2,                     m1, [pw_unpackwdq]  ; [5 5 5 5 5 5 5 5]
    movu        [r2],                   m2
    movu        [r2 + 16],              m2
    psrldq      m1,                     2
    pshufb      m2,                     m1, [pw_unpackwdq]  ; [6 6 6 6 6 6 6 6]
    movu        [r2 + r1],              m2
    movu        [r2 + r1 + 16],         m2
    psrldq      m1,                     2
    pshufb      m2,                     m1, [pw_unpackwdq]  ; [7 7 7 7 7 7 7 7]
    movu        [r2 + r1 * 2],          m2
    movu        [r2 + r1 * 2 + 16],     m2
    psrldq      m1,                     2
    pshufb      m2,                     m1, [pw_unpackwdq]  ; [8 8 8 8 8 8 8 8]
    movu        [r2 + r4],              m2
    movu        [r2 + r4 + 16],         m2

    lea         r2,                     [r2 + r1 *4]
    pshufb      m2,                     m3, [pw_unpackwdq]  ; [9 9 9 9 9 9 9 9]
    movu        [r2],                   m2
    movu        [r2 + 16],              m2
    psrldq      m3,                     2
    pshufb      m2,                     m3, [pw_unpackwdq]  ; [10 10 10 10 10 10 10 10]
    movu        [r2 + r1],              m2
    movu        [r2 + r1 + 16],         m2
    psrldq      m3,                     2
    pshufb      m2,                     m3, [pw_unpackwdq]  ; [11 11 11 11 11 11 11 11]
    movu        [r2 + r1 * 2],          m2
    movu        [r2 + r1 * 2 + 16],     m2
    psrldq      m3,                     2
    pshufb      m2,                     m3, [pw_unpackwdq]  ; [12 12 12 12 12 12 12 12]
    movu        [r2 + r4],              m2
    movu        [r2 + r4 + 16],         m2

    lea         r2,                     [r2 + r1 *4]
    psrldq      m3,                     2
    pshufb      m2,                     m3, [pw_unpackwdq]  ; [13 13 13 13 13 13 13 13]
    movu        [r2],                   m2
    movu        [r2 + 16],              m2
    psrldq      m3,                     2
    pshufb      m2,                     m3, [pw_unpackwdq]  ; [14 14 14 14 14 14 14 14]
    movu        [r2 + r1],              m2
    movu        [r2 + r1 + 16],         m2
    psrldq      m3,                     2
    pshufb      m2,                     m3, [pw_unpackwdq]  ; [15 15 15 15 15 15 15 15]
    movu        [r2 + r1 * 2],          m2
    movu        [r2 + r1 * 2 + 16],     m2
    psrldq      m3,                     2
    pshufb      m2,                     m3, [pw_unpackwdq]  ; [16 16 16 16 16 16 16 16]
    movu        [r2 + r4],              m2
    movu        [r2 + r4 + 16],         m2
    mova        m3,                     m0

    cmp         r5m,                    byte 0
    jz         .quit

    ; filter

    movh        m1,                     [r3]                ; [3 2 1 0]
    pshufb      m2,                     m1, [pw_unpackwdq]  ; [0 0 0 0 0 0 0 0]
    movu        m1,                     [r3 + 2]            ; [8 7 6 5 4 3 2 1]
    movu        m3,                     [r3 + 18]           ; [16 15 14 13 12 11 10 9]
    psubw       m1,                     m2
    psubw       m3,                     m2
    psraw       m1,                     1
    psraw       m3,                     1
    paddw       m3,                     m0
    paddw       m0,                     m1
    pxor        m1,                     m1
    pmaxsw      m0,                     m1
    pminsw      m0,                     [pw_1023]
    pmaxsw      m3,                     m1
    pminsw      m3,                     [pw_1023]
.quit:
    movu        [r0],                   m0
    movu        [r0 + 16],              m3

    RET

cglobal intra_pred_ang16_26, 4,5,4
    movu        m0,                 [r3 + 2]            ; [8 7 6 5 4 3 2 1]
    movu        m3,                 [r3 + 18]           ; [16 15 14 13 12 11 10 9]
    add         r1,                 r1
    lea         r4,                 [r1 * 3]

    movu        [r0],               m0
    movu        [r0 + 16],          m3
    movu        [r0 + r1],          m0
    movu        [r0 + r1 + 16],     m3
    movu        [r0 + r1 * 2],      m0
    movu        [r0 + r1 * 2 + 16], m3
    movu        [r0 + r4],          m0
    movu        [r0 + r4 + 16],     m3

    lea         r3,                 [r0 + r1 *4]
    movu        [r3],               m0
    movu        [r3 + 16],          m3
    movu        [r3 + r1],          m0
    movu        [r3 + r1 + 16],     m3
    movu        [r3 + r1 * 2],      m0
    movu        [r3 + r1 * 2 + 16], m3
    movu        [r3 + r4],          m0
    movu        [r3 + r4 + 16],     m3

    lea         r3,                 [r3 + r1 *4]
    movu        [r3],               m0
    movu        [r3 + 16],          m3
    movu        [r3 + r1],          m0
    movu        [r3 + r1 + 16],     m3
    movu        [r3 + r1 * 2],      m0
    movu        [r3 + r1 * 2 + 16], m3
    movu        [r3 + r4],          m0
    movu        [r3 + r4 + 16],     m3

    lea         r3,                 [r3 + r1 *4]
    movu        [r3],               m0
    movu        [r3 + 16],          m3
    movu        [r3 + r1],          m0
    movu        [r3 + r1 + 16],     m3
    movu        [r3 + r1 * 2],      m0
    movu        [r3 + r1 * 2 + 16], m3
    movu        [r3 + r4],          m0
    movu        [r3 + r4 + 16],     m3

    cmp         r5m,                byte 0
    jz         .quit

    ; filter

    pshufb      m0,                 [pw_unpackwdq]
    movh        m1,                 [r2]                ; [3 2 1 0]
    pshufb      m2,                 m1, [pw_unpackwdq]  ; [0 0 0 0 0 0 0 0]
    movu        m1,                 [r2 + 2]            ; [8 7 6 5 4 3 2 1]
    movu        m3,                 [r2 + 18]           ; [16 15 14 13 12 11 10 9]
    psubw       m1,                 m2
    psubw       m3,                 m2
    psraw       m1,                 1
    psraw       m3,                 1
    paddw       m3,                 m0
    paddw       m0,                 m1
    pxor        m1,                 m1
    pmaxsw      m0,                 m1
    pminsw      m0,                 [pw_1023]
    pmaxsw      m3,                 m1
    pminsw      m3,                 [pw_1023]
    pextrw      [r0],               m0, 0
    pextrw      [r0 + r1],          m0, 1
    pextrw      [r0 + r1 * 2],      m0, 2
    pextrw      [r0 + r4],          m0, 3
    lea         r0,                 [r0 + r1 * 4]
    pextrw      [r0],               m0, 4
    pextrw      [r0 + r1],          m0, 5
    pextrw      [r0 + r1 * 2],      m0, 6
    pextrw      [r0 + r4],          m0, 7
    lea         r0,                 [r0 + r1 * 4]
    pextrw      [r0],               m3, 0
    pextrw      [r0 + r1],          m3, 1
    pextrw      [r0 + r1 * 2],      m3, 2
    pextrw      [r0 + r4],          m3, 3
    pextrw      [r3],               m3, 4
    pextrw      [r3 + r1],          m3, 5
    pextrw      [r3 + r1 * 2],      m3, 6
    pextrw      [r3 + r4],          m3, 7

.quit:
    RET

%macro MODE_2_34 0
    movu            m0, [r2 + 4]
    movu            m1, [r2 + 20]
    movu            m2, [r2 + 36]
    movu            m3, [r2 + 52]
    movu            m4, [r2 + 68]
    movu            [r0], m0
    movu            [r0 + 16], m1
    movu            [r0 + 32], m2
    movu            [r0 + 48], m3
    palignr         m5, m1, m0, 2
    movu            [r0 + r1], m5
    palignr         m5, m2, m1, 2
    movu            [r0 + r1 + 16], m5
    palignr         m5, m3, m2, 2
    movu            [r0 + r1 + 32], m5
    palignr         m5, m4, m3, 2
    movu            [r0 + r1 + 48], m5
    palignr         m5, m1, m0, 4
    movu            [r0 + r3], m5
    palignr         m5, m2, m1, 4
    movu            [r0 + r3 + 16], m5
    palignr         m5, m3, m2, 4
    movu            [r0 + r3 + 32], m5
    palignr         m5, m4, m3, 4
    movu            [r0 + r3 + 48], m5
    palignr         m5, m1, m0, 6
    movu            [r0 + r4], m5
    palignr         m5, m2, m1, 6
    movu            [r0 + r4 + 16], m5
    palignr         m5, m3, m2, 6
    movu            [r0 + r4 + 32], m5
    palignr         m5, m4, m3, 6
    movu            [r0 + r4 + 48], m5
    lea             r0, [r0 + r1 * 4]
    palignr         m5, m1, m0, 8
    movu            [r0], m5
    palignr         m5, m2, m1, 8
    movu            [r0 + 16], m5
    palignr         m5, m3, m2, 8
    movu            [r0 + 32], m5
    palignr         m5, m4, m3, 8
    movu            [r0 + 48], m5
    palignr         m5, m1, m0, 10
    movu            [r0 + r1], m5
    palignr         m5, m2, m1, 10
    movu            [r0 + r1 + 16], m5
    palignr         m5, m3, m2, 10
    movu            [r0 + r1 + 32], m5
    palignr         m5, m4, m3, 10
    movu            [r0 + r1 + 48], m5
    palignr         m5, m1, m0, 12
    movu            [r0 + r3], m5
    palignr         m5, m2, m1, 12
    movu            [r0 + r3 + 16], m5
    palignr         m5, m3, m2, 12
    movu            [r0 + r3 + 32], m5
    palignr         m5, m4, m3, 12
    movu            [r0 + r3 + 48], m5
    palignr         m5, m1, m0, 14
    movu            [r0 + r4], m5
    palignr         m5, m2, m1, 14
    movu            [r0 + r4 + 16], m5
    palignr         m5, m3, m2, 14
    movu            [r0 + r4 + 32], m5
    palignr         m5, m4, m3, 14
    movu            [r0 + r4 + 48], m5
    lea             r0, [r0 + r1 * 4]
    movu            m0, [r2 + 84]
    movu            [r0], m1
    movu            [r0 + 16], m2
    movu            [r0 + 32], m3
    movu            [r0 + 48], m4
    palignr         m5, m2, m1, 2
    movu            [r0 + r1], m5
    palignr         m5, m3, m2, 2
    movu            [r0 + r1 + 16], m5
    palignr         m5, m4, m3, 2
    movu            [r0 + r1 + 32], m5
    palignr         m5, m0, m4, 2
    movu            [r0 + r1 + 48], m5
    palignr         m5, m2, m1, 4
    movu            [r0 + r3], m5
    palignr         m5, m3, m2, 4
    movu            [r0 + r3 + 16], m5
    palignr         m5, m4, m3, 4
    movu            [r0 + r3 + 32], m5
    palignr         m5, m0, m4, 4
    movu            [r0 + r3 + 48], m5
    palignr         m5, m2, m1, 6
    movu            [r0 + r4], m5
    palignr         m5, m3, m2, 6
    movu            [r0 + r4 + 16], m5
    palignr         m5, m4, m3, 6
    movu            [r0 + r4 + 32], m5
    palignr         m5, m0, m4, 6
    movu            [r0 + r4 + 48], m5
    lea             r0, [r0 + r1 * 4]
    palignr         m5, m2, m1, 8
    movu            [r0], m5
    palignr         m5, m3, m2, 8
    movu            [r0 + 16], m5
    palignr         m5, m4, m3, 8
    movu            [r0 + 32], m5
    palignr         m5, m0, m4, 8
    movu            [r0 + 48], m5
    palignr         m5, m2, m1, 10
    movu            [r0 + r1], m5
    palignr         m5, m3, m2, 10
    movu            [r0 + r1 + 16], m5
    palignr         m5, m4, m3, 10
    movu            [r0 + r1 + 32], m5
    palignr         m5, m0, m4, 10
    movu            [r0 + r1 + 48], m5
    palignr         m5, m2, m1, 12
    movu            [r0 + r3], m5
    palignr         m5, m3, m2, 12
    movu            [r0 + r3 + 16], m5
    palignr         m5, m4, m3, 12
    movu            [r0 + r3 + 32], m5
    palignr         m5, m0, m4, 12
    movu            [r0 + r3 + 48], m5
    palignr         m5, m2, m1, 14
    movu            [r0 + r4], m5
    palignr         m5, m3, m2, 14
    movu            [r0 + r4 + 16], m5
    palignr         m5, m4, m3, 14
    movu            [r0 + r4 + 32], m5
    palignr         m5, m0, m4, 14
    movu            [r0 + r4 + 48], m5
    lea             r0,    [r0 + r1 * 4]
%endmacro
;--------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_2_34(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;--------------------------------------------------------------------------------------------------------------------
INIT_XMM ssse3
cglobal intra_pred_ang32_2, 3,6,6
    cmp             r4m, byte 34
    cmove           r2, r3mp

    add             r1, r1
    lea             r3, [r1 * 2]
    lea             r4, [r1 * 3]
    mov             r5, 2

.loop:
    MODE_2_34
    add             r2, 32
    dec             r5
    jnz             .loop
    RET

%macro TRANSPOSE_STORE_8x8 6
  %if %2 == 1
    ; transpose 4x8 and then store, used by angle BLOCK_16x16 and BLOCK_32x32
    punpckhwd   m0, %3, %4
    punpcklwd   %3, %4
    punpckhwd   %4, %3, m0
    punpcklwd   %3, m0

    punpckhwd   m0, %5, %6
    punpcklwd   %5, %6
    punpckhwd   %6, %5, m0
    punpcklwd   %5, m0

    punpckhqdq  m0, %3, %5
    punpcklqdq  %3, %5
    punpcklqdq  %5, %4, %6
    punpckhqdq  %4, %6

    movu        [r0 + %1], %3
    movu        [r0 + r1 + %1], m0
    movu        [r0 + r1 * 2 + %1], %5
    movu        [r0 + r5 + %1], %4
  %else
    ; store 8x4, used by angle BLOCK_16x16 and BLOCK_32x32
    movh        [r0], %3
    movhps      [r0 + r1], %3
    movh        [r0 + r1 * 2], %4
    movhps      [r0 + r5], %4
    lea         r0, [r0 + r1 * 4]
    movh        [r0], %5
    movhps      [r0 + r1], %5
    movh        [r0 + r1 * 2], %6
    movhps      [r0 + r5], %6
    lea         r0, [r0 + r1 * 4]
  %endif
%endmacro

%macro MODE_3_33 1
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m3,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    mova        m7,        m0

    palignr     m1,        m3, m0, 2                  ; [9 8 7 6 5 4 3 2]
    punpckhwd   m2,        m0, m1                     ; [9 8 8 7 7 6 6 5] xmm2
    punpcklwd   m0,        m1                         ; [5 4 4 3 3 2 2 1] xmm0

    palignr     m1,        m2, m0, 4                  ; [6 5 5 4 4 3 3 2] xmm1
    pmaddwd     m4,        m0, [r3 + 10 * 16]         ; [26]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m5,        m1, [r3 + 4 * 16]          ; [20]
    paddd       m5,        [pd_16]
    psrld       m5,        5
    packusdw    m4,        m5

    palignr     m5,        m2, m0, 8
    pmaddwd     m5,        [r3 - 2 * 16]              ; [14]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    palignr     m6,        m2, m0, 12
    pmaddwd     m6,        [r3 - 8 * 16]              ; [ 8]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m2, [r3 - 14 * 16]         ; [ 2]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m2, [r3 + 12 * 16]         ; [28]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m0,        m3, m2, 4                  ; [10 9 9 8 8 7 7 6]
    pmaddwd     m1,        m0, [r3 + 6 * 16]          ; [22]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    psrldq      m2,        m3, 2   ; [x 16 15 14 13 12 11 10]
    palignr     m2,        m0, 4   ;[11 10 10 9 9 8 8 7]

    pmaddwd     m2,        [r3]                       ; [16]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m1,        m2

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    palignr     m0,        m3, m7, 14                 ; [15 14 13 12 11 10 9 8]
    movu        m3,        [r2 + 32]                  ; [23 22 21 20 19 18 17 16]
    palignr     m1,        m3, m0, 2                  ; [16 15 14 13 12 11 10 9]
    punpckhwd   m7,        m0, m1                     ; [16 15 15 14 14 13 13 12]
    punpcklwd   m0,        m1                         ; [12 11 11 10 10 9 9 8]

    palignr     m5,        m7, m0, 4                  ; [13 12 12 11 11 10 10 9]
    pmaddwd     m4,        m0, [r3 - 6 * 16]          ; [10]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m5, [r3 - 12 * 16]         ; [04]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        [r3 + 14 * 16]             ; [30]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    palignr     m6,        m7, m0, 8                  ; [14 13 13 12 12 11 11 10]
    pmaddwd     m6,        [r3 + 8 * 16]              ; [24]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    palignr     m1,        m7, m0, 12                 ; [15 14 14 13 13 12 12 11]
    pmaddwd     m6,        m1, [r3 + 2 * 16]          ; [18]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m7, [r3 - 4 * 16]          ; [12]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m2,        m3, m7, 4                  ; [17 16 16 15 15 14 14 13]
    pmaddwd     m1,        m2, [r3 - 10 * 16]         ; [6]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    packusdw    m1,        m1
    movhps      m1,        [r2 + 28]                  ; [00]

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    movu        m0,        [r2 + 28]                  ; [35 34 33 32 31 30 29 28]
    palignr     m1,        m0, 2                      ; [ x 35 34 33 32 31 30 29]
    punpckhwd   m2,        m0, m1                     ; [ x 35 35 34 34 33 33 32]
    punpcklwd   m0,        m1                         ; [32 31 31 30 30 29 29 28]

    pmaddwd     m4,        m0, [r3 + 10 * 16]         ; [26]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    palignr     m1,        m2, m0, 4                  ; [33 32 32 31 31 30 30 29]
    pmaddwd     m1,        [r3 + 4 * 16]              ; [20]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    palignr     m5,        m2, m0, 8                  ; [34 33 33 32 32 31 31 30]
    pmaddwd     m5,        [r3 - 2 * 16]              ; [14]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    palignr     m6,        m2, m0, 12                 ; [35 34 34 33 33 32 32 31]
    pmaddwd     m6,        [r3 - 8 * 16]              ; [ 8]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pinsrw      m2,        [r2 + 44], 7               ; [35 34 34 33 33 32 32 31]
    pmaddwd     m6,        m2, [r3 - 14 * 16]         ; [ 2]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m2,        [r3 + 12 * 16]             ; [28]
    paddd       m2,        [pd_16]
    psrld       m2,        5
    packusdw    m6,        m2

    movu        m3,        [r2 + 38]                  ; [45 44 43 42 41 40 39 38]
    palignr     m1,        m3, 2                      ; [ x 45 44 43 42 41 40 39]
    punpckhwd   m2,        m3, m1                     ; [ x 35 35 34 34 33 33 32]
    punpcklwd   m3,        m1                         ; [32 31 31 30 30 29 29 28]

    pmaddwd     m1,        m3, [r3 + 6 * 16]          ; [22]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    palignr     m0,        m2, m3, 4
    pmaddwd     m0,        [r3]                       ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    palignr     m5,        m2, m3, 8
    pmaddwd     m4,        m5, [r3 - 6 * 16]          ; [10]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    palignr     m5,        m2, m3, 12
    pmaddwd     m1,        m5, [r3 - 12 * 16]         ; [04]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        [r3 + 14 * 16]             ; [30]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 46]
    palignr     m1,        m3, 2
    punpckhwd   m2,        m3, m1
    punpcklwd   m3,        m1

    pmaddwd     m6,        m3, [r3 + 8 * 16]          ; [24]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    palignr     m6,        m2, m3, 4
    pmaddwd     m6,        [r3 + 2 * 16]              ; [18]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    palignr     m1,        m2, m3, 8
    pmaddwd     m1,        [r3 - 4 * 16]              ; [12]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m1,        m2, m3, 12
    pmaddwd     m1,        [r3 - 10 * 16]             ; [06]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    packusdw    m1,        m1
    movhps      m1,        [r2 + 54]                  ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_3(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_3, 3,6,8
    lea         r3, [ang_table + 16 * 16]
    mov         r4d, 8
    add         r1, r1
    lea         r5, [r1 * 3]

.loop:
    MODE_3_33 1
    lea         r0, [r0 + r1 * 4 ]
    add         r2, 8
    dec         r4
    jnz         .loop
    RET

%macro MODE_4_32 1
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m3,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m1,        m3, m0, 2                  ; [9 8 7 6 5 4 3 2]
    punpckhwd   m2,        m0, m1                     ; [9 8 8 7 7 6 6 5]
    punpcklwd   m0,        m1                         ; [5 4 4 3 3 2 2 1]

    pmaddwd     m4,        m0, [r3 + 5 * 16]          ; [21]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    palignr     m5,        m2, m0, 4                  ; [6 5 5 4 4 3 3 2]
    pmaddwd     m1,        m5, [r3 - 6 * 16]          ; [10]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        [r3 + 15 * 16]             ; [31]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    palignr     m6,        m2, m0, 8
    pmaddwd     m6,        [r3 + 4 * 16]              ; [ 20]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    palignr     m1,        m2, m0, 12
    pmaddwd     m6,        m1, [r3 - 7 * 16]          ; [ 9]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        [r3 + 14 * 16]             ; [30]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m2, [r3 + 3 * 16]          ; [19]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    palignr     m7,        m3, m2, 4                  ; [10 9 9 8 7 6 5 4]
    pmaddwd     m0,        m7, [r3 - 8 * 16]          ; [8]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddwd     m4,        m7, [r3 + 13 * 16]         ; [29]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m0,        [r2 + 34]                  ; [24 23 22 21 20 19 18 17]

    palignr     m2,        m0, m3, 2                  ; [17 16 15 14 13 12 11 10]
    palignr     m1,        m0, m3, 4                  ; [18 17 16 15 14 13 12 11]
    punpckhwd   m3,        m2, m1                     ; [18 17 17 16 16 15 15 14]
    punpcklwd   m2,        m1                         ; [14 13 13 12 12 11 11 10]

    palignr     m1,        m2, m7, 4                  ; [11 10 10 9 9 8 7 6]
    pmaddwd     m1,        [r3 +  2 * 16]             ; [18]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    palignr     m5,        m2, m7, 8
    mova        m6,        m5
    pmaddwd     m5,        [r3 - 9 * 16]              ; [07]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        [r3 + 12 * 16]             ; [28]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    palignr     m6,        m2, m7, 12
    pmaddwd     m6,        [r3 +      16]             ; [17]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m2, [r3 - 10 * 16]         ; [06]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m2, [r3 + 11 * 16]         ; [27]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    palignr     m7,        m3, m2, 4
    pmaddwd     m7,        [r3]                       ; [16]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m1,        m7
    mova        m7,        m0

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    palignr     m0,        m3, m2, 8
    pmaddwd     m4,        m0, [r3 - 11 * 16]         ; [5]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m0, [r3 + 10 * 16]         ; [26]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    palignr     m5,        m3, m2, 12
    pmaddwd     m5,        [r3 - 16]                  ; [15]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m1,        m3, [r3 - 12 * 16]         ; [4]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    pmaddwd     m6,        m3, [r3 + 9 * 16]          ; [25]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    movu        m0,        [r2 + 50]                  ; [32 31 30 29 28 27 26 25]
    palignr     m2,        m0, m7, 2                  ; [25 24 23 22 21 20 19 18]
    palignr     m1,        m0, m7, 4                  ; [26 25 24 23 22 21 20 19]
    punpckhwd   m7,        m2, m1                     ; [26 25 25 24 24 23 23 22]
    punpcklwd   m2,        m1                         ; [22 21 21 20 20 19 19 18]

    palignr     m1,        m2, m3, 4
    pmaddwd     m1,        [r3 - 2 * 16]              ; [14]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m1,        m2, m3, 8
    mova        m0,        m1
    pmaddwd     m1,        [r3 - 13 * 16]             ; [3]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        [r3 + 8 * 16]              ; [24]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    palignr     m4,        m2, m3, 12
    pmaddwd     m4,        [r3 - 3 * 16]              ; [13]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m2, [r3 - 14 * 16]         ; [2]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m2, [r3 + 7 * 16]          ; [23]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    palignr     m6,        m7, m2, 4
    pmaddwd     m6,        [r3 - 4 * 16]              ; [12]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    palignr     m1,        m7, m2, 8
    pmaddwd     m6,        m1, [r3 - 15 * 16]         ; [1]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        [r3 + 6 * 16]              ; [22]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m1,        m7, m2, 12
    pmaddwd     m1,        [r3 - 5 * 16]              ; [11]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m1,        m1
    movhps      m1,        [r2 + 44]                  ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_4(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_4, 3,6,8
    lea         r3, [ang_table + 16 * 16]
    mov         r4d, 8
    add         r1, r1
    lea         r5, [r1 * 3]

.loop:
    MODE_4_32 1
    lea         r0, [r0 + r1 * 4 ]
    add         r2, 8
    dec         r4
    jnz         .loop
    RET

%macro MODE_5_31 1
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m3,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m1,        m3, m0, 2                  ; [9 8 7 6 5 4 3 2]
    punpckhwd   m2,        m0, m1                     ; [9 8 8 7 7 6 6 5]
    punpcklwd   m0,        m1                         ; [5 4 4 3 3 2 2 1]

    pmaddwd     m4,        m0, [r3 + 16]              ; [17]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    palignr     m1,        m2, m0, 4
    mova        m5,        m1
    pmaddwd     m1,        [r3 - 14 * 16]             ; [2]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        [r3 + 3 * 16]              ; [19]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    palignr     m6,        m2, m0, 8
    mova        m1,        m6
    pmaddwd     m6,        [r3 - 12 * 16]             ; [4]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m1, [r3 + 5 * 16]          ; [21]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    palignr     m1,        m2, m0, 12
    mova        m7,        m1
    pmaddwd     m7,        [r3 - 10 * 16]             ; [6]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    pmaddwd     m1,        [r3 + 7 * 16]              ; [23]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m7,        m2, [r3 - 8 * 16]          ; [8]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m1,        m7

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddwd     m4,        m2, [r3 + 9 * 16]          ; [25]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    palignr     m7,        m3, m2, 4                  ; [10 9 9 8 7 6 5 4]
    pmaddwd     m1,        m7, [r3 - 6 * 16]          ; [10]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m7, [r3 + 11 * 16]         ; [27]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m0,        [r2 + 34]                  ; [24 23 22 21 20 19 18 17]
    palignr     m2,        m0, m3, 2                  ; [17 16 15 14 13 12 11 10]
    palignr     m1,        m0, m3, 4                  ; [18 17 16 15 14 13 12 11]
    punpckhwd   m3,        m2, m1                     ; [18 17 17 16 16 15 15 14]
    punpcklwd   m2,        m1                         ; [14 13 13 12 12 11 11 10]

    palignr     m6,        m2, m7, 4
    pmaddwd     m1,        m6, [r3 - 4 * 16]          ; [12]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    pmaddwd     m6,        [r3 + 13 * 16]             ; [29]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    palignr     m1,        m2, m7, 8
    mova        m0,        m1
    pmaddwd     m1,        [r3 - 2 * 16]              ; [14]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m0, [r3 + 15 * 16]         ; [31]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    palignr     m0,        m2, m7, 12
    pmaddwd     m0,        [r3]                       ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    pmaddwd     m4,        m2, [r3 - 15 * 16]         ; [1]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m2, [r3 + 2 * 16]          ; [18]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    palignr     m1,        m3, m2, 4
    pmaddwd     m5,        m1, [r3 - 13 * 16]         ; [3]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m1,        [r3 + 4 * 16]              ; [20]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    palignr     m1,        m3, m2, 8
    pmaddwd     m6,        m1, [r3 - 11 * 16]         ; [5]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        [r3 + 6 * 16]              ; [22]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m7,        m3, m2, 12
    pmaddwd     m1,        m7, [r3 - 9 * 16]          ; [7]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m7,        [r3 + 8 * 16]              ; [24]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m1,        m7

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 - 7 * 16]          ; [9]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 + 10 * 16]         ; [26]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    movu        m0,        [r2 + 36]                  ; [25 24 23 22 21 20 19 18]
    palignr     m1,        m0, 2                      ; [x 25 24 23 22 21 20 19]
    punpcklwd   m0,        m1                         ; [22 21 21 20 20 19 19 18]

    palignr     m1,        m0, m3, 4
    pmaddwd     m5,        m1, [r3 - 5 * 16]          ; [11]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m1,        [r3 + 12 * 16]             ; [28]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    palignr     m1,        m0, m3, 8
    pmaddwd     m6,        m1, [r3 - 3 * 16]          ; [13]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        [r3 + 14 * 16]             ; [30]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m1,        m0, m3, 12
    pmaddwd     m1,        [r3 - 16]                  ; [15]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m1,        m1
    movhps      m1,        [r2 + 36]                  ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_5(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_5, 3,6,8
    lea         r3, [ang_table + 16 * 16]
    mov         r4d, 8
    add         r1, r1
    lea         r5, [r1 * 3]

.loop:
    MODE_5_31 1
    lea         r0, [r0 + r1 * 4 ]
    add         r2, 8
    dec         r4
    jnz         .loop
    RET

%macro MODE_6_30 1
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movu        m3,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m1,        m3, m0, 2                  ; [9 8 7 6 5 4 3 2]
    punpckhwd   m2,        m0, m1                     ; [9 8 8 7 7 6 6 5]
    punpcklwd   m0,        m1                         ; [5 4 4 3 3 2 2 1]

    pmaddwd     m4,        m0, [r3 - 3 * 16]          ; [13]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m0, [r3 + 10 * 16]         ; [26]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    palignr     m1,        m2, m0, 4
    pmaddwd     m5,        m1, [r3 - 9 * 16]          ; [7]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m1,        [r3 + 4 * 16]              ; [20]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    palignr     m1,        m2, m0, 8
    pmaddwd     m6,        m1, [r3 - 15 * 16]         ; [1]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m7,        m1, [r3 - 2 * 16]          ; [14]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    pmaddwd     m1,        [r3 + 11 * 16]             ; [27]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    palignr     m7,        m2, m0, 12
    pmaddwd     m0,        m7, [r3 - 8 * 16]          ; [8]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddwd     m4,        m7, [r3 +  5 * 16]         ; [21]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m2, [r3 - 14 * 16]         ; [2]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m2, [r3 - 16]              ; [15]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m2, [r3 + 12 * 16]         ; [28]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    palignr     m7,        m3, m2, 4
    pmaddwd     m6,        m7, [r3 - 7 * 16]          ; [9]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m7, [r3 + 6 * 16]          ; [22]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m0,        [r2 + 34]                  ; [24 23 22 21 20 19 18 17]
    palignr     m2,        m0, m3, 2                  ; [17 16 15 14 13 12 11 10]
    palignr     m1,        m0, m3, 4                  ; [18 17 16 15 14 13 12 11]
    punpckhwd   m3,        m2, m1                     ; [18 17 17 16 16 15 15 14]
    punpcklwd   m2,        m1                         ; [14 13 13 12 12 11 11 10]

    palignr     m0,        m2, m7, 4
    pmaddwd     m1,        m0, [r3 - 13 * 16]         ; [3]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        [r3]                       ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    palignr     m4,        m2, m7, 4
    pmaddwd     m4,        [r3 +  13 * 16]            ; [29]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    palignr     m5,        m2, m7, 8
    pmaddwd     m1,        m5, [r3 - 6 * 16]          ; [10]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        [r3 + 7 * 16]              ; [23]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    palignr     m1,        m2, m7, 12
    pmaddwd     m6,        m1, [r3 - 12 * 16]         ; [4]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m1, [r3 + 16]              ; [17]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        [r3 + 14 * 16]             ; [30]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m2, [r3 - 5 * 16]          ; [11]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m2, [r3 + 8 * 16]          ; [24]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    palignr     m5,        m3, m2, 4
    pmaddwd     m4,        m5, [r3 - 11 * 16]         ; [5]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m5, [r3 + 2 * 16]          ; [18]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        [r3 + 15 * 16]             ; [31]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    palignr     m6,        m3, m2, 8
    pmaddwd     m1,        m6, [r3 - 4 * 16]          ; [12]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m5,        m1

    pmaddwd     m6,        [r3 + 9 * 16]              ; [25]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    palignr     m1,        m3, m2, 12
    pmaddwd     m0,        m1, [r3 - 10 * 16]         ; [6]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m6,        m0

    pmaddwd     m1,        [r3 + 3 * 16]              ; [19]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m1,        m1
    movhps      m1,        [r2 + 28]                  ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_6(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_6, 3,6,8
    lea         r3, [ang_table + 16 * 16]
    mov         r4d, 8
    add         r1, r1
    lea         r5, [r1 * 3]

.loop:
    MODE_6_30 1
    lea         r0, [r0 + r1 * 4 ]
    add         r2, 8
    dec         r4
    jnz         .loop
    RET

%macro MODE_7_29 1
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movd        m3,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m1,        m3, m0, 2                  ; [9 8 7 6 5 4 3 2]
    punpckhwd   m2,        m0, m1                     ; [9 8 8 7 7 6 6 5]
    punpcklwd   m0,        m1                         ; [5 4 4 3 3 2 2 1]

    pmaddwd     m4,        m0, [r3 - 7 * 16]          ; [9]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m0, [r3 + 2 * 16]          ; [18]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m0, [r3 + 11 * 16]         ; [27]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    palignr     m1,        m2, m0, 4
    pmaddwd     m6,        m1, [r3 - 12 * 16]         ; [4]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m1, [r3 - 3 * 16]          ; [13]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m7,        m1, [r3 + 6 * 16]          ; [22]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m6,        m7

    pmaddwd     m1,        [r3 + 15 * 16]             ; [31]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    mova        m3,        m0
    palignr     m7,        m2, m0, 8
    pmaddwd     m0,        m7, [r3 - 8 * 16]          ; [8]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddwd     m4,        m7, [r3 + 16]              ; [17]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m7, [r3 + 10 * 16]         ; [26]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    palignr     m1,        m2, m3, 12
    pmaddwd     m5,        m1, [r3 - 13 * 16]         ; [3]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m1, [r3 - 4 * 16]          ; [12]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m1, [r3 + 5 * 16]          ; [21]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        [r3 + 14 * 16]             ; [30]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m2, [r3 - 9 * 16]          ; [7]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m2, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    pmaddwd     m4,        m2, [r3 + 9 * 16]          ; [25]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m7,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m1,        m7, 2                      ; [x 16 15 14 13 12 11 10]
    punpcklwd   m7,        m1                         ; [13 12 12 11 11 10 10 9]

    palignr     m6,        m7, m2, 4
    pmaddwd     m1,        m6, [r3 - 14 * 16]         ; [2]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m6, [r3 - 5 * 16]          ; [11]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m0,        m6, [r3 + 4 * 16]          ; [20]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    pmaddwd     m6,        [r3 + 13 * 16]             ; [29]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    palignr     m0,        m7, m2, 8
    pmaddwd     m1,        m0, [r3 - 10 * 16]         ; [6]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m0, [r3 - 16]              ; [15]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        [r3 + 8 * 16]              ; [24]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    palignr     m0,        m7, m2, 12
    pmaddwd     m4,        m0, [r3 - 15 * 16]         ; [1]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m0, [r3 - 6 * 16]          ; [10]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m0, [r3 + 3 * 16]          ; [19]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m0,        [r3 + 12 * 16]             ; [28]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    pmaddwd     m6,        m7, [r3 - 11 * 16]         ; [5]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m0,        m7, [r3 - 2 * 16]          ; [14]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m6,        m0

    pmaddwd     m1,        m7, [r3 + 7 * 16]          ; [23]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m1,        m1
    movhps      m1,        [r2 + 20]                  ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_7(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_7, 3,6,8
    lea         r3, [ang_table + 16 * 16]
    mov         r4d, 8
    add         r1, r1
    lea         r5, [r1 * 3]

.loop:
    MODE_7_29 1
    lea         r0, [r0 + r1 * 4 ]
    add         r2, 8
    dec         r4
    jnz         .loop
    RET

%macro MODE_8_28 1
    movu        m0,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    movd        m3,        [r2 + 18]                  ; [16 15 14 13 12 11 10 9]
    palignr     m1,        m3, m0, 2                  ; [9 8 7 6 5 4 3 2]
    punpckhwd   m2,        m0, m1                     ; [9 8 8 7 7 6 6 5]
    punpcklwd   m0,        m1                         ; [5 4 4 3 3 2 2 1]

    pmaddwd     m4,        m0, [r3 - 11 * 16]         ; [5]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m0, [r3 - 6 * 16]          ; [10]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m0, [r3 - 16]              ; [15]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m0, [r3 + 4 * 16]          ; [20]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m0, [r3 + 9 * 16]          ; [25]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m0, [r3 + 14 * 16]         ; [30]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    palignr     m7,        m2, m0, 4
    pmaddwd     m1,        m7, [r3 - 13 * 16]         ; [3]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    mova        m3,        m0
    pmaddwd     m0,        m7, [r3 - 8 * 16]          ; [8]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddwd     m4,        m7, [r3 - 3 * 16]          ; [13]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m7, [r3 + 2 * 16]          ; [18]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m7, [r3 + 7 * 16]          ; [23]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m7, [r3 + 12 * 16]         ; [28]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    palignr     m7,        m2, m3, 8
    pmaddwd     m6,        m7, [r3 - 15 * 16]         ; [1]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m7, [r3 - 10 * 16]         ; [6]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m7, [r3 - 5 * 16]          ; [11]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m7, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    pmaddwd     m4,        m7, [r3 + 5 * 16]          ; [21]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m7, [r3 + 10 * 16]         ; [26]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m7, [r3 + 15 * 16]         ; [31]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    palignr     m7,        m2, m3, 12
    pmaddwd     m0,        m7, [r3 - 12 * 16]         ; [4]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    pmaddwd     m6,        m7, [r3 - 7 * 16]          ; [9]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m7, [r3 - 2 * 16]          ; [14]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m7, [r3 + 3 * 16]          ; [19]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m7, [r3 + 8 * 16]          ; [24]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    pmaddwd     m4,        m7, [r3 + 13 * 16]         ; [29]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m2, [r3 - 14 * 16]         ; [2]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m2, [r3 - 9 * 16]          ; [7]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m0,        m2, [r3 - 4 * 16]          ; [12]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    pmaddwd     m6,        m2, [r3 + 16]              ; [17]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m0,        m2, [r3 + 6 * 16]          ; [22]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m6,        m0

    pmaddwd     m1,        m2, [r3 + 11 * 16]         ; [27]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m1,        m1
    movhps      m1,        [r2 + 12]                  ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_8(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_8, 3,6,8
    lea         r3, [ang_table + 16 * 16]
    mov         r4d, 8
    add         r1, r1
    lea         r5, [r1 * 3]

.loop:
    MODE_8_28 1
    lea         r0, [r0 + r1 * 4 ]
    add         r2, 8
    dec         r4
    jnz         .loop
    RET

%macro MODE_9_27 1
    movu        m3,        [r2 + 2]                   ; [8 7 6 5 4 3 2 1]
    palignr     m1,        m3, 2                      ; [9 8 7 6 5 4 3 2]
    punpckhwd   m2,        m3, m1                     ; [9 8 8 7 7 6 6 5]
    punpcklwd   m3,        m1                         ; [5 4 4 3 3 2 2 1]

    pmaddwd     m4,        m3, [r3 - 14 * 16]         ; [2]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 - 12 * 16]         ; [4]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 10 * 16]         ; [6]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 - 8 * 16]          ; [8]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m3, [r3 - 6 * 16]          ; [10]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 4 * 16]          ; [12]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 - 2 * 16]          ; [14]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m3, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 + 2 * 16]          ; [18]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 + 4 * 16]          ; [20]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 + 6 * 16]          ; [22]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 + 8 * 16]          ; [24]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m3, [r3 + 10 * 16]         ; [26]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 + 12 * 16]         ; [28]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 + 14 * 16]         ; [30]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    packusdw    m1,        m1
    movhps      m1,        [r2 + 4]                   ; [00]

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    palignr     m7,        m2, m3, 4
    pmaddwd     m4,        m7, [r3 - 14 * 16]         ; [2]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m7, [r3 - 12 * 16]         ; [4]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m7, [r3 - 10 * 16]         ; [6]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m0,        m7, [r3 - 8 * 16]          ; [8]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    pmaddwd     m6,        m7, [r3 - 6 * 16]          ; [10]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m7, [r3 - 4 * 16]          ; [12]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m7, [r3 - 2 * 16]          ; [14]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m7, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    pmaddwd     m4,        m7, [r3 + 2 * 16]          ; [18]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m7, [r3 + 4 * 16]          ; [20]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m7, [r3 + 6 * 16]          ; [22]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m0,        m7, [r3 + 8 * 16]          ; [24]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    pmaddwd     m6,        m7, [r3 + 10 * 16]         ; [26]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m0,        m7, [r3 + 12 * 16]         ; [28]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m6,        m0

    pmaddwd     m7,        [r3 + 14 * 16]             ; [30]
    paddd       m7,        [pd_16]
    psrld       m7,        5
    packusdw    m7,        m7
    movhps      m7,        [r2 + 6]                   ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m7
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_9(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_9, 3,6,8
    lea         r3, [ang_table + 16 * 16]
    mov         r4d, 8
    add         r1, r1
    lea         r5, [r1 * 3]

.loop:
    MODE_9_27 1
    lea         r0, [r0 + r1 * 4 ]
    add         r2, 8
    dec         r4
    jnz         .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_10(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_10, 4,7,8
    mov         r6d, 4
    add         r1, r1
    lea         r5, [r1 * 3]
    lea         r4, [r1 * 2]
    lea         r3, [r1 * 4]
    mova        m7, [c_mode32_10_0]

.loop:
    movu        m0, [r2 + 2]
    pshufb      m1, m0, m7
    movu        [r0], m1
    movu        [r0 + 16], m1
    movu        [r0 + 32], m1
    movu        [r0 + 48], m1

    palignr     m1, m0, 2
    pshufb      m1, m7
    movu        [r0 + r1], m1
    movu        [r0 + r1 + 16], m1
    movu        [r0 + r1 + 32], m1
    movu        [r0 + r1 + 48], m1

    palignr     m1, m0, 4
    pshufb      m1, m7
    movu        [r0 + r4], m1
    movu        [r0 + r4 + 16], m1
    movu        [r0 + r4 + 32], m1
    movu        [r0 + r4 + 48], m1

    palignr     m1, m0, 6
    pshufb      m1, m7
    movu        [r0 + r5], m1
    movu        [r0 + r5 + 16], m1
    movu        [r0 + r5 + 32], m1
    movu        [r0 + r5 + 48], m1

    add         r0, r3

    palignr     m1, m0, 8
    pshufb      m1, m7
    movu        [r0], m1
    movu        [r0 + 16], m1
    movu        [r0 + 32], m1
    movu        [r0 + 48], m1

    palignr     m1, m0, 10
    pshufb      m1, m7
    movu        [r0 + r1], m1
    movu        [r0 + r1 + 16], m1
    movu        [r0 + r1 + 32], m1
    movu        [r0 + r1 + 48], m1

    palignr     m1, m0, 12
    pshufb      m1, m7
    movu        [r0 + r4], m1
    movu        [r0 + r4 + 16], m1
    movu        [r0 + r4 + 32], m1
    movu        [r0 + r4 + 48], m1

    palignr     m1, m0, 14
    pshufb      m1, m7
    movu        [r0 + r5], m1
    movu        [r0 + r5 + 16], m1
    movu        [r0 + r5 + 32], m1
    movu        [r0 + r5 + 48], m1

    add         r0, r3
    add         r2, 16
    dec         r6d
    jnz         .loop
    RET

%macro MODE_11_25 1
    movu        m3,        [r2 + 2]                   ; [7 6 5 4 3 2 1 0]
    pshufb      m3,        [pw_punpcklwd]             ; [4 3 3 2 2 1 1 0]

    pmaddwd     m4,        m3, [r3 + 14 * 16]         ; [30]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 + 12 * 16]         ; [28]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 + 10 * 16]         ; [26]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 + 8 * 16]          ; [24]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m3, [r3 + 6 * 16]          ; [22]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 + 4 * 16]          ; [20]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 + 2 * 16]          ; [18]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m3, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 - 2 * 16]          ; [14]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 - 4 * 16]          ; [12]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 6 * 16]          ; [10]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 - 8 * 16]          ; [8]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m3, [r3 - 10 * 16]         ; [6]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 12 * 16]         ; [4]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 - 14 * 16]         ; [2]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    packusdw    m1,        m1
    movhps      m1,        [r2 + 2]                   ; [00]

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    movu        m3,        [r2]                       ; [6 5 4 3 2 1 0 16]
    pshufb      m3,        [pw_punpcklwd]             ; [3 2 2 1 1 0 0 16]

    pmaddwd     m4,        m3, [r3 + 14 * 16]         ; [30]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 + 12 * 16]         ; [28]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 + 10 * 16]         ; [26]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m0,        m3, [r3 + 8 * 16]          ; [24]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    pmaddwd     m6,        m3, [r3 + 6 * 16]          ; [22]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 + 4 * 16]          ; [20]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 + 2 * 16]          ; [18]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m3, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 - 2 * 16]          ; [14]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 - 4 * 16]          ; [12]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 6 * 16]          ; [10]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 - 8 * 16]          ; [8]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m3, [r3 - 10 * 16]         ; [6]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 12 * 16]         ; [4]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 - 14 * 16]         ; [2]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    packusdw    m1,        m1
    movhps      m1,        [r2]                       ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_11(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_11, 4,6,7,0-(4*mmsize+4)
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 0*mmsize + 2], m0
    movu     [rsp + 1*mmsize + 2], m1
    movu     [rsp + 2*mmsize + 2], m2
    movu     [rsp + 3*mmsize + 2], m3
    mov      r4w, [r3+32]
    mov      [rsp], r4w
    mov      r4w, [r2+64]
    mov      [rsp+66], r4w

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]

.loop:
    MODE_11_25 1
    lea      r0, [r0 + r1 * 4 ]
    add      r2, 8
    dec      r4
    jnz      .loop
    RET

%macro MODE_12_24 1
    movu        m3,        [r2 + 8]                   ; [7 6 5 4 3 2 1 0]
    pshufb      m3,        m2                         ; [4 3 3 2 2 1 1 0]

    pmaddwd     m4,        m3, [r3 + 11 * 16]         ; [27]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 + 6 * 16]          ; [22]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 + 16]              ; [17]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 - 4 * 16]          ; [12]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m3, [r3 - 9 * 16]          ; [7]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 14 * 16]         ; [2]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2 + 6]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 13 * 16]         ; [29]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m3, [r3 + 8 * 16]          ; [24]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 + 3 * 16]          ; [19]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 - 2 * 16]          ; [14]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 7 * 16]          ; [9]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 - 12 * 16]         ; [4]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    movu        m3,        [r2 + 4]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 15 * 16]         ; [31]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 + 10 * 16]         ; [26]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 + 5 * 16]          ; [21]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m3, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 - 5 * 16]          ; [11]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 - 10 * 16]         ; [6]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 15 * 16]         ; [1]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 2]
    pshufb      m3,        m2

    pmaddwd     m0,        m3, [r3 + 12 * 16]         ; [28]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    pmaddwd     m6,        m3, [r3 + 7 * 16]          ; [23]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 + 2 * 16]          ; [18]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 - 3 * 16]          ; [13]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m3, [r3 - 8 * 16]          ; [8]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 - 13 * 16]         ; [3]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 14 * 16]         ; [30]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 + 9 * 16]          ; [25]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 + 4 * 16]          ; [20]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m3, [r3 - 16]              ; [15]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 6 * 16]          ; [10]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 - 11 * 16]         ; [5]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    packusdw    m1,        m1
    movhps      m1,        [r2]                       ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_12(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_12, 4,6,7,0-(4*mmsize+10)
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 0*mmsize + 8], m0
    movu     [rsp + 1*mmsize + 8], m1
    movu     [rsp + 2*mmsize + 8], m2
    movu     [rsp + 3*mmsize + 8], m3

    mov      r4w, [r2+64]
    mov      [rsp+72], r4w
    mov      r4w, [r3+12]
    mov      [rsp+6], r4w
    mov      r4w, [r3+26]
    mov      [rsp+4], r4w
    mov      r4w, [r3+38]
    mov      [rsp+2], r4w
    mov      r4w, [r3+52]
    mov      [rsp], r4w

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]
    mova     m2, [pw_punpcklwd]

.loop:
    MODE_12_24 1
    lea      r0, [r0 + r1 * 4 ]
    add      r2, 8
    dec      r4
    jnz      .loop
    RET

%macro MODE_13_23 1
    movu        m3,        [r2 + 16]                  ; [7 6 5 4 3 2 1 0]
    pshufb      m3,        m2                         ; [4 3 3 2 2 1 1 0]

    pmaddwd     m4,        m3, [r3 + 7 * 16]          ; [23]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 - 2 * 16]          ; [14]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 11 * 16]         ; [05]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 14]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 12 * 16]         ; [28]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m3, [r3 + 3 * 16]          ; [19]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 6 * 16]          ; [10]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 - 15 * 16]         ; [01]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    movu        m3,        [r2 + 12]
    pshufb      m3,        m2

    pmaddwd     m0,        m3, [r3 + 8 * 16]          ; [24]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 - 16]              ; [15]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 - 10 * 16]         ; [06]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    movu        m3,        [r2 + 10]
    pshufb      m3,        m2

    pmaddwd     m5,        m3, [r3 + 13 * 16]         ; [29]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 + 4 * 16]          ; [20]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m3, [r3 - 5 * 16]          ; [11]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 14 * 16]         ; [02]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2 + 8]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 9 * 16]          ; [25]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m3, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 - 9 * 16]          ; [07]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2 + 6]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 14 * 16]         ; [30]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 + 5 * 16]          ; [21]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m0,        m3, [r3 - 4 * 16]          ; [12]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    pmaddwd     m6,        m3, [r3 - 13 * 16]         ; [03]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    movu        m3,        [r2 + 4]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 10 * 16]         ; [26]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 + 16]              ; [17]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m3, [r3 - 8 * 16]          ; [08]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    movu        m3,        [r2 + 2]
    pshufb      m3,        m2

    pmaddwd     m4,        m3, [r3 + 15 * 16]         ; [31]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 + 6 * 16]          ; [22]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 3 * 16]          ; [13]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 - 12 * 16]         ; [04]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    movu        m3,        [r2]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 11 * 16]         ; [27]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 + 2 * 16]          ; [18]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 - 7 * 16]          ; [09]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    packusdw    m1,        m1
    movhps      m1,        [r2]                       ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_13(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_13, 4,6,7,0-(5*mmsize+2)
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 1*mmsize], m0
    movu     [rsp + 2*mmsize], m1
    movu     [rsp + 3*mmsize], m2
    movu     [rsp + 4*mmsize], m3

    mov      r4w, [r2+64]
    mov      [rsp+80], r4w
    movu     m0, [r3 + 8]
    movu     m1, [r3 + 36]
    pshufb   m0, [shuf_mode_13_23]
    pshufb   m1, [shuf_mode_13_23]
    movh     [rsp + 8], m0
    movh     [rsp], m1
    mov      r4w, [r3+28]
    mov      [rsp+8], r4w
    mov      r4w, [r3+56]
    mov      [rsp], r4w

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]
    mova     m2, [pw_punpcklwd]

.loop:
    MODE_13_23 1
    lea      r0, [r0 + r1 * 4 ]
    add      r2, 8
    dec      r4
    jnz     .loop
    RET

%macro MODE_14_22 1
    movu        m3,        [r2 + 24]                  ; [7 6 5 4 3 2 1 0]
    pshufb      m3,        m2                         ; [4 3 3 2 2 1 1 0]

    pmaddwd     m4,        m3, [r3 + 3 * 16]          ; [19]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 - 10 * 16]         ; [06]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    movu        m3,        [r2 + 22]
    pshufb      m3,        m2

    pmaddwd     m5,        m3, [r3 + 9 * 16]          ; [25]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 - 4 * 16]          ; [12]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    movu        m3,        [r2 + 20]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 15 * 16]         ; [31]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 + 2 * 16]          ; [18]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 - 11 * 16]         ; [05]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    movu        m3,        [r2 + 18]
    pshufb      m3,        m2

    pmaddwd     m0,        m3, [r3 + 8 * 16]          ; [24]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 - 5 * 16]          ; [11]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2 + 16]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 14 * 16]         ; [30]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 + 16]              ; [17]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 - 12 * 16]         ; [04]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    movu        m3,        [r2 + 14]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 7 * 16]          ; [23]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 6 * 16]          ; [10]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2 + 12]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 13 * 16]         ; [29]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m3, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 - 13 * 16]         ; [03]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2 + 10]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 6 * 16]          ; [22]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 7 * 16]          ; [09]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 8]
    pshufb      m3,        m2

    pmaddwd     m0,        m3, [r3 + 12 * 16]         ; [28]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    pmaddwd     m6,        m3, [r3 - 16]              ; [15]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 14 * 16]         ; [02]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2 + 6]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 5 * 16]          ; [21]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m3, [r3 - 8 * 16]          ; [08]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    movu        m3,        [r2 + 4]
    pshufb      m3,        m2

    pmaddwd     m4,        m3, [r3 + 11 * 16]         ; [27]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 - 2 * 16]          ; [14]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 15 * 16]         ; [01]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 2]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 4 * 16]          ; [20]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m3, [r3 - 9 * 16]          ; [07]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    movu        m3,        [r2]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 10 * 16]         ; [26]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 - 3 * 16]          ; [13]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    packusdw    m1,        m1
    movhps      m1,        [r2]                       ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_14(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_14, 4,6,7,0-(5*mmsize+10)
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 1*mmsize + 8], m0
    movu     [rsp + 2*mmsize + 8], m1
    movu     [rsp + 3*mmsize + 8], m2
    movu     [rsp + 4*mmsize + 8], m3

    mov      r4w, [r2 + 64]
    mov      [rsp + 88], r4w
    mov      r4w, [r3+4]
    mov      [rsp+22], r4w
    movu     m0, [r3 + 10]
    movu     m1, [r3 + 30]
    movu     m2, [r3 + 50]
    pshufb   m0, [shuf_mode_14_22]
    pshufb   m1, [shuf_mode_14_22]
    pshufb   m2, [shuf_mode_14_22]
    movh     [rsp + 14], m0
    movh     [rsp + 6], m1
    movh     [rsp - 2], m2

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]
    mova     m2, [pw_punpcklwd]

.loop:
    MODE_14_22 1
    lea      r0, [r0 + r1 * 4 ]
    add      r2, 8
    dec      r4
    jnz     .loop
    RET

%macro MODE_15_21 1
    movu        m3,        [r2 + 32]                  ; [7 6 5 4 3 2 1 0]
    pshufb      m3,        m2                         ; [4 3 3 2 2 1 1 0]

    pmaddwd     m4,        m3, [r3 - 16]              ; [15]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2 + 30]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 14 * 16]         ; [30]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 3 * 16]          ; [13]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 28]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 12 * 16]         ; [28]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m3, [r3 - 5 * 16]          ; [11]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    movu        m3,        [r2 + 26]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 10 * 16]         ; [26]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 - 7 * 16]          ; [09]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    movu        m3,        [r2 + 24]
    pshufb      m3,        m2

    pmaddwd     m0,        m3, [r3 + 8 * 16]          ; [24]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 - 9 * 16]          ; [07]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2 + 22]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 6 * 16]          ; [22]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 11 * 16]         ; [05]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 20]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 4 * 16]          ; [20]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    pmaddwd     m6,        m3, [r3 - 13 * 16]         ; [03]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    movu        m3,        [r2 + 18]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 2 * 16]          ; [18]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 - 15 * 16]         ; [01]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    movu        m3,        [r2 + 16]
    pshufb      m3,        m2

    pmaddwd     m0,        m3, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    movu        m3,        [r2 + 14]
    pshufb      m3,        m2

    pmaddwd     m4,        m3, [r3 + 15 * 16]         ; [31]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 - 2 * 16]          ; [14]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    movu        m3,        [r2 + 12]
    pshufb      m3,        m2

    pmaddwd     m5,        m3, [r3 + 13 * 16]         ; [29]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m0,        m3, [r3 - 4 * 16]          ; [12]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    movu        m3,        [r2 + 10]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 11 * 16]         ; [27]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 6 * 16]          ; [10]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2 + 8]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 9 * 16]          ; [25]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m3, [r3 - 8 * 16]          ; [08]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    movu        m3,        [r2 + 6]
    pshufb      m3,        m2

    pmaddwd     m4,        m3, [r3 + 7 * 16]          ; [23]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 - 10 * 16]         ; [06]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    movu        m3,        [r2 + 4]
    pshufb      m3,        m2

    pmaddwd     m5,        m3, [r3 + 5 * 16]          ; [21]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 - 12 * 16]         ; [04]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    movu        m3,        [r2 + 2]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 3 * 16]          ; [19]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 14 * 16]         ; [02]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 16]              ; [17]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    packusdw    m1,        m1
    movhps      m1,        [r2]                       ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_15(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_15, 4,6,7,0-(6*mmsize+2)
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 2*mmsize], m0
    movu     [rsp + 3*mmsize], m1
    movu     [rsp + 4*mmsize], m2
    movu     [rsp + 5*mmsize], m3

    mov      r4w, [r2 + 64]
    mov      [rsp + 96], r4w
    movu     m0, [r3 + 4]
    movu     m1, [r3 + 18]
    movu     m2, [r3 + 34]
    movu     m3, [r3 + 48]
    pshufb   m0, [shuf_mode_15_21]
    pshufb   m1, [shuf_mode_15_21]
    pshufb   m2, [shuf_mode_15_21]
    pshufb   m3, [shuf_mode_15_21]
    movh     [rsp + 24], m0
    movh     [rsp + 16], m1
    movh     [rsp + 8], m2
    movh     [rsp], m3

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]
    mova     m2, [pw_punpcklwd]

.loop:
    MODE_15_21 1
    lea      r0, [r0 + r1 * 4 ]
    add      r2, 8
    dec      r4
    jnz     .loop
    RET

%macro MODE_16_20 1
    movu        m3,        [r2 + 40]                  ; [7 6 5 4 3 2 1 0]
    pshufb      m3,        m2                         ; [4 3 3 2 2 1 1 0]

    pmaddwd     m4,        m3, [r3 - 5 * 16]          ; [11]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2 + 38]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 6 * 16]          ; [22]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 15 * 16]         ; [01]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 36]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 - 4 * 16]          ; [12]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    movu        m3,        [r2 + 34]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 7 * 16]          ; [23]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 14 * 16]         ; [02]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2 + 32]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 - 3 * 16]          ; [13]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    movu        m3,        [r2 + 30]
    pshufb      m3,        m2

    pmaddwd     m0,        m3, [r3 + 8 * 16]          ; [24]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddwd     m4,        m3, [r3 - 13 * 16]         ; [03]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2 + 28]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 - 2 * 16]          ; [14]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    movu        m3,        [r2 + 26]
    pshufb      m3,        m2

    pmaddwd     m5,        m3, [r3 + 9 * 16]          ; [25]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    pmaddwd     m6,        m3, [r3 - 12 * 16]         ; [04]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    movu        m3,        [r2 + 24]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 - 16]              ; [15]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    movu        m3,        [r2 + 22]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 10 * 16]         ; [26]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    pmaddwd     m1,        m3, [r3 - 11 * 16]         ; [05]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    movu        m3,        [r2 + 20]
    pshufb      m3,        m2

    pmaddwd     m0,        m3, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    movu        m3,        [r2 + 18]
    pshufb      m3,        m2

    pmaddwd     m4,        m3, [r3 + 11 * 16]         ; [27]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    pmaddwd     m1,        m3, [r3 - 10 * 16]         ; [06]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    movu        m3,        [r2 + 16]
    pshufb      m3,        m2

    pmaddwd     m5,        m3, [r3 + 16]              ; [17]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 14]
    pshufb      m3,        m2

    pmaddwd     m0,        m3, [r3 + 12 * 16]         ; [28]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    pmaddwd     m6,        m3, [r3 - 9 * 16]          ; [07]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    movu        m3,        [r2 + 12]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 2 * 16]          ; [18]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2 + 10]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 13 * 16]         ; [29]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    pmaddwd     m0,        m3, [r3 - 8 * 16]          ; [08]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    movu        m3,        [r2 + 8]
    pshufb      m3,        m2

    pmaddwd     m4,        m3, [r3 + 3 * 16]          ; [19]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2 + 6]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 14 * 16]         ; [30]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 7 * 16]          ; [09]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 4]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 4 * 16]          ; [20]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    movu        m3,        [r2 + 2]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 15 * 16]         ; [31]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 6 * 16]          ; [10]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 5 * 16]          ; [21]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    packusdw    m1,        m1
    movhps      m1,        [r2]                       ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_16(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_16, 4,6,7,0-(6*mmsize+10)
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 2*mmsize + 8], m0
    movu     [rsp + 3*mmsize + 8], m1
    movu     [rsp + 4*mmsize + 8], m2
    movu     [rsp + 5*mmsize + 8], m3

    mov      r4w, [r2 + 64]
    mov      [rsp + 104], r4w
    movu     m0, [r3 + 4]
    movu     m1, [r3 + 22]
    movu     m2, [r3 + 40]
    movd     m3, [r3 + 58]
    pshufb   m0, [shuf_mode_16_20]
    pshufb   m1, [shuf_mode_16_20]
    pshufb   m2, [shuf_mode_16_20]
    pshufb   m3, [shuf_mode_16_20]
    movu     [rsp + 24], m0
    movu     [rsp + 12], m1
    movu     [rsp], m2
    movd     [rsp], m3

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]
    mova     m2, [pw_punpcklwd]

.loop:
    MODE_16_20 1
    lea      r0, [r0 + r1 * 4 ]
    add      r2, 8
    dec      r4
    jnz     .loop
    RET

%macro MODE_17_19 1
    movu        m3,        [r2 + 50]                  ; [7 6 5 4 3 2 1 0]
    pshufb      m3,        m2                         ; [4 3 3 2 2 1 1 0]

    pmaddwd     m4,        m3, [r3 - 10 * 16]         ; [06]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2 + 48]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 - 4 * 16]          ; [12]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    movu        m3,        [r2 + 46]
    pshufb      m3,        m2

    pmaddwd     m5,        m3, [r3 + 2 * 16]          ; [18]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 44]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 8 * 16]          ; [24]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    movu        m3,        [r2 + 42]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 14 * 16]         ; [30]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 12 * 16]         ; [04]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2 + 40]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 - 6 * 16]          ; [10]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    movu        m3,        [r2 + 38]
    pshufb      m3,        m2

    pmaddwd     m0,        m3, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    movu        m3,        [r2 + 36]
    pshufb      m3,        m2

    pmaddwd     m4,        m3, [r3 + 6 * 16]          ; [22]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2 + 34]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 12 * 16]         ; [28]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 14 * 16]         ; [02]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 32]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 - 8 * 16]          ; [08]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    movu        m3,        [r2 + 30]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 - 2 * 16]          ; [14]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    movu        m3,        [r2 + 28]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 4 * 16]          ; [20]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2 + 26]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 10 * 16]         ; [26]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    packusdw    m1,        m1
    movhps      m1,        [r2 + 26]                  ; [00]

    TRANSPOSE_STORE_8x8 16, %1, m4, m5, m6, m1

    movu        m3,        [r2 + 24]
    pshufb      m3,        m2

    pmaddwd     m4,        m3, [r3 - 10 * 16]         ; [06]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2 + 22]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 - 4 * 16]          ; [12]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    movu        m3,        [r2 + 20]
    pshufb      m3,        m2

    pmaddwd     m5,        m3, [r3 + 2 * 16]          ; [18]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 18]
    pshufb      m3,        m2

    pmaddwd     m0,        m3, [r3 + 8 * 16]          ; [24]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m5,        m0

    movu        m3,        [r2 + 16]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 + 14 * 16]         ; [30]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    pmaddwd     m1,        m3, [r3 - 12 * 16]         ; [04]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2 + 14]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 - 6 * 16]          ; [10]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    movu        m3,        [r2 + 12]
    pshufb      m3,        m2

    pmaddwd     m0,        m3, [r3]                   ; [16]
    paddd       m0,        [pd_16]
    psrld       m0,        5
    packusdw    m1,        m0

    TRANSPOSE_STORE_8x8 32, %1, m4, m5, m6, m1

    movu        m3,        [r2 + 10]
    pshufb      m3,        m2

    pmaddwd     m4,        m3, [r3 + 6 * 16]          ; [22]
    paddd       m4,        [pd_16]
    psrld       m4,        5

    movu        m3,        [r2 + 8]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 12 * 16]         ; [28]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m4,        m1

    pmaddwd     m5,        m3, [r3 - 14 * 16]         ; [02]
    paddd       m5,        [pd_16]
    psrld       m5,        5

    movu        m3,        [r2 + 6]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 - 8 * 16]          ; [08]
    paddd       m6,        [pd_16]
    psrld       m6,        5
    packusdw    m5,        m6

    movu        m3,        [r2 + 4]
    pshufb      m3,        m2

    pmaddwd     m6,        m3, [r3 - 2 * 16]          ; [14]
    paddd       m6,        [pd_16]
    psrld       m6,        5

    movu        m3,        [r2 + 2]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 4 * 16]          ; [20]
    paddd       m1,        [pd_16]
    psrld       m1,        5
    packusdw    m6,        m1

    movu        m3,        [r2]
    pshufb      m3,        m2

    pmaddwd     m1,        m3, [r3 + 10 * 16]         ; [26]
    paddd       m1,        [pd_16]
    psrld       m1,        5

    packusdw    m1,        m1
    movhps      m1,        [r2]                       ; [00]

    TRANSPOSE_STORE_8x8 48, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_17(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_17, 4,6,7,0-(7*mmsize+4)
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 3*mmsize + 2], m0
    movu     [rsp + 4*mmsize + 2], m1
    movu     [rsp + 5*mmsize + 2], m2
    movu     [rsp + 6*mmsize + 2], m3

    mov      r4w, [r2 + 64]
    mov      [rsp + 114], r4w
    movu     m0, [r3 + 8]
    movu     m1, [r3 + 30]
    movu     m2, [r3 + 50]
    movd     m3, [r3 + 2]
    pshufb   m0, [shuf_mode_17_19]
    pshufb   m1, [shuf_mode_17_19]
    pshufb   m2, [shuf_mode_17_19]
    pshufb   m3, [shuf_mode_16_20]
    movd     [rsp + 46], m3
    movu     [rsp + 30], m0
    movu     [rsp + 12], m1
    movu     [rsp - 4], m2
    mov      r4w, [r3 + 24]
    mov      [rsp + 30], r4w
    mov      r4w, [r3 + 28]
    mov      [rsp + 28], r4w
    mov      r4w, [r3 + 46]
    mov      [rsp + 12], r4w

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]
    mova     m2, [pw_punpcklwd]

.loop:
    MODE_17_19 1
    lea      r0, [r0 + r1 * 4 ]
    add      r2, 8
    dec      r4
    jnz     .loop
    RET

;-------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_18(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_18, 4,7,8
    movu        m0, [r3]               ; [7 6 5 4 3 2 1 0]
    movu        m1, [r3 + 16]          ; [15 14 13 12 11 10 9 8]
    movu        m2, [r3 + 32]          ; [23 22 21 20 19 18 17 16]
    movu        m3, [r3 + 48]          ; [31 30 29 28 27 26 25 24]
    movu        m4, [r2 + 2]           ; [8 7 6 5 4 3 2 1]
    movu        m5, [r2 + 18]          ; [16 15 14 13 12 11 10 9]

    add         r1, r1
    lea         r6, [r1 * 2]
    lea         r3, [r1 * 3]
    lea         r4, [r1 * 4]

    movu        [r0], m0
    movu        [r0 + 16], m1
    movu        [r0 + 32], m2
    movu        [r0 + 48], m3

    pshufb      m4, [shuf_mode32_18]   ; [1 2 3 4 5 6 7 8]
    pshufb      m5, [shuf_mode32_18]   ; [9 10 11 12 13 14 15 16]

    palignr     m6, m0, m4, 14
    movu        [r0 + r1], m6
    palignr     m6, m1, m0, 14
    movu        [r0 + r1 + 16], m6
    palignr     m6, m2, m1, 14
    movu        [r0 + r1 + 32], m6
    palignr     m6, m3, m2, 14
    movu        [r0 + r1 + 48], m6

    palignr     m6, m0, m4, 12
    movu        [r0 + r6], m6
    palignr     m6, m1, m0, 12
    movu        [r0 + r6 + 16], m6
    palignr     m6, m2, m1, 12
    movu        [r0 + r6 + 32], m6
    palignr     m6, m3, m2, 12
    movu        [r0 + r6 + 48], m6

    palignr     m6, m0, m4, 10
    movu        [r0 + r3], m6
    palignr     m6, m1, m0, 10
    movu        [r0 + r3 + 16], m6
    palignr     m6, m2, m1, 10
    movu        [r0 + r3 + 32], m6
    palignr     m6, m3, m2, 10
    movu        [r0 + r3 + 48], m6

    add         r0, r4

    palignr     m6, m0, m4, 8
    movu        [r0], m6
    palignr     m6, m1, m0, 8
    movu        [r0 + 16], m6
    palignr     m6, m2, m1, 8
    movu        [r0 + 32], m6
    palignr     m6, m3, m2, 8
    movu        [r0 + 48], m6

    palignr     m6, m0, m4, 6
    movu        [r0 + r1], m6
    palignr     m6, m1, m0, 6
    movu        [r0 + r1 + 16], m6
    palignr     m6, m2, m1, 6
    movu        [r0 + r1 + 32], m6
    palignr     m6, m3, m2, 6
    movu        [r0 + r1 + 48], m6

    palignr     m6, m0, m4, 4
    movu        [r0 + r6], m6
    palignr     m6, m1, m0, 4
    movu        [r0 + r6 + 16], m6
    palignr     m6, m2, m1, 4
    movu        [r0 + r6 + 32], m6
    palignr     m6, m3, m2, 4
    movu        [r0 + r6 + 48], m6

    palignr     m6, m0, m4, 2
    movu        [r0 + r3], m6
    palignr     m6, m1, m0, 2
    movu        [r0 + r3 + 16], m6
    palignr     m6, m2, m1, 2
    movu        [r0 + r3 + 32], m6
    palignr     m6, m3, m2, 2
    movu        [r0 + r3 + 48], m6

    add         r0, r4

    movu        [r0], m4
    movu        [r0 + 16], m0
    movu        [r0 + 32], m1
    movu        [r0 + 48], m2

    palignr     m6, m4, m5, 14
    movu        [r0 + r1], m6
    palignr     m6, m0, m4, 14
    movu        [r0 + r1 + 16], m6
    palignr     m6, m1, m0, 14
    movu        [r0 + r1 + 32], m6
    palignr     m6, m2, m1, 14
    movu        [r0 + r1 + 48], m6

    palignr     m6, m4, m5, 12
    movu        [r0 + r6], m6
    palignr     m6, m0, m4, 12
    movu        [r0 + r6 + 16], m6
    palignr     m6, m1, m0, 12
    movu        [r0 + r6 + 32], m6
    palignr     m6, m2, m1, 12
    movu        [r0 + r6 + 48], m6

    palignr     m6, m4, m5, 10
    movu        [r0 + r3], m6
    palignr     m6, m0, m4, 10
    movu        [r0 + r3 + 16], m6
    palignr     m6, m1, m0, 10
    movu        [r0 + r3 + 32], m6
    palignr     m6, m2, m1, 10
    movu        [r0 + r3 + 48], m6

    add         r0, r4

    palignr     m6, m4, m5, 8
    movu        [r0], m6
    palignr     m6, m0, m4, 8
    movu        [r0 + 16], m6
    palignr     m6, m1, m0, 8
    movu        [r0 + 32], m6
    palignr     m6, m2, m1, 8
    movu        [r0 + 48], m6

    palignr     m6, m4, m5, 6
    movu        [r0 + r1], m6
    palignr     m6, m0, m4, 6
    movu        [r0 + r1 + 16], m6
    palignr     m6, m1, m0, 6
    movu        [r0 + r1 + 32], m6
    palignr     m6, m2, m1, 6
    movu        [r0 + r1 + 48], m6

    palignr     m6, m4, m5, 4
    movu        [r0 + r6], m6
    palignr     m6, m0, m4, 4
    movu        [r0 + r6 + 16], m6
    palignr     m6, m1, m0, 4
    movu        [r0 + r6 + 32], m6
    palignr     m6, m2, m1, 4
    movu        [r0 + r6 + 48], m6

    palignr     m6, m4, m5, 2
    movu        [r0 + r3], m6
    palignr     m6, m0, m4, 2
    movu        [r0 + r3 + 16], m6
    palignr     m6, m1, m0, 2
    movu        [r0 + r3 + 32], m6
    palignr     m6, m2, m1, 2
    movu        [r0 + r3 + 48], m6

    add         r0, r4

    movu        m2, [r2 + 34]
    movu        m3, [r2 + 50]
    pshufb      m2, [shuf_mode32_18]
    pshufb      m3, [shuf_mode32_18]

    movu        [r0], m5
    movu        [r0 + 16], m4
    movu        [r0 + 32], m0
    movu        [r0 + 48], m1

    palignr     m6, m5, m2, 14
    movu        [r0 + r1], m6
    palignr     m6, m4, m5, 14
    movu        [r0 + r1 + 16], m6
    palignr     m6, m0, m4, 14
    movu        [r0 + r1 + 32], m6
    palignr     m6, m1, m0, 14
    movu        [r0 + r1 + 48], m6

    palignr     m6, m5, m2, 12
    movu        [r0 + r6], m6
    palignr     m6, m4, m5, 12
    movu        [r0 + r6 + 16], m6
    palignr     m6, m0, m4, 12
    movu        [r0 + r6 + 32], m6
    palignr     m6, m1, m0, 12
    movu        [r0 + r6 + 48], m6

    palignr     m6, m5, m2, 10
    movu        [r0 + r3], m6
    palignr     m6, m4, m5, 10
    movu        [r0 + r3 + 16], m6
    palignr     m6, m0, m4, 10
    movu        [r0 + r3 + 32], m6
    palignr     m6, m1, m0, 10
    movu        [r0 + r3 + 48], m6

    add         r0, r4

    palignr     m6, m5, m2, 8
    movu        [r0], m6
    palignr     m6, m4, m5, 8
    movu        [r0 + 16], m6
    palignr     m6, m0, m4, 8
    movu        [r0 + 32], m6
    palignr     m6, m1, m0, 8
    movu        [r0 + 48], m6

    palignr     m6, m5, m2, 6
    movu        [r0 + r1], m6
    palignr     m6, m4, m5, 6
    movu        [r0 + r1 + 16], m6
    palignr     m6, m0, m4, 6
    movu        [r0 + r1 + 32], m6
    palignr     m6, m1, m0, 6
    movu        [r0 + r1 + 48], m6

    palignr     m6, m5, m2, 4
    movu        [r0 + r6], m6
    palignr     m6, m4, m5, 4
    movu        [r0 + r6 + 16], m6
    palignr     m6, m0, m4, 4
    movu        [r0 + r6 + 32], m6
    palignr     m6, m1, m0, 4
    movu        [r0 + r6 + 48], m6

    palignr     m6, m5, m2, 2
    movu        [r0 + r3], m6
    palignr     m6, m4, m5, 2
    movu        [r0 + r3 + 16], m6
    palignr     m6, m0, m4, 2
    movu        [r0 + r3 + 32], m6
    palignr     m6, m1, m0, 2
    movu        [r0 + r3 + 48], m6

    add         r0, r4

    movu        [r0], m2
    movu        [r0 + 16], m5
    movu        [r0 + 32], m4
    movu        [r0 + 48], m0

    palignr     m6, m2, m3, 14
    movu        [r0 + r1], m6
    palignr     m6, m5, m2, 14
    movu        [r0 + r1 + 16], m6
    palignr     m6, m4, m5, 14
    movu        [r0 + r1 + 32], m6
    palignr     m6, m0, m4, 14
    movu        [r0 + r1 + 48], m6

    palignr     m6, m2, m3, 12
    movu        [r0 + r6], m6
    palignr     m6, m5, m2, 12
    movu        [r0 + r6 + 16], m6
    palignr     m6, m4, m5, 12
    movu        [r0 + r6 + 32], m6
    palignr     m6, m0, m4, 12
    movu        [r0 + r6 + 48], m6

    palignr     m6, m2, m3, 10
    movu        [r0 + r3], m6
    palignr     m6, m5, m2, 10
    movu        [r0 + r3 + 16], m6
    palignr     m6, m4, m5, 10
    movu        [r0 + r3 + 32], m6
    palignr     m6, m0, m4, 10
    movu        [r0 + r3 + 48], m6

    add         r0, r4

    palignr     m6, m2, m3, 8
    movu        [r0], m6
    palignr     m6, m5, m2, 8
    movu        [r0 + 16], m6
    palignr     m6, m4, m5, 8
    movu        [r0 + 32], m6
    palignr     m6, m0, m4, 8
    movu        [r0 + 48], m6

    palignr     m6, m2, m3, 6
    movu        [r0 + r1], m6
    palignr     m6, m5, m2, 6
    movu        [r0 + r1 + 16], m6
    palignr     m6, m4, m5, 6
    movu        [r0 + r1 + 32], m6
    palignr     m6, m0, m4, 6
    movu        [r0 + r1 + 48], m6

    palignr     m6, m2, m3, 4
    movu        [r0 + r6], m6
    palignr     m6, m5, m2, 4
    movu        [r0 + r6 + 16], m6
    palignr     m6, m4, m5, 4
    movu        [r0 + r6 + 32], m6
    palignr     m6, m0, m4, 4
    movu        [r0 + r6 + 48], m6

    palignr     m6, m2, m3, 2
    movu        [r0 + r3], m6
    palignr     m6, m5, m2, 2
    movu        [r0 + r3 + 16], m6
    palignr     m6, m4, m5, 2
    movu        [r0 + r3 + 32], m6
    palignr     m6, m0, m4, 2
    movu        [r0 + r3 + 48], m6
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_19(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_19, 4,7,7,0-(7*mmsize+4)
    xchg     r2, r3
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 3*mmsize + 2], m0
    movu     [rsp + 4*mmsize + 2], m1
    movu     [rsp + 5*mmsize + 2], m2
    movu     [rsp + 6*mmsize + 2], m3

    mov      r4w, [r2 + 64]
    mov      [rsp + 114], r4w
    movu     m0, [r3 + 8]
    movu     m1, [r3 + 30]
    movu     m2, [r3 + 50]
    movd     m3, [r3 + 2]
    pshufb   m0, [shuf_mode_17_19]
    pshufb   m1, [shuf_mode_17_19]
    pshufb   m2, [shuf_mode_17_19]
    pshufb   m3, [shuf_mode_16_20]
    movd     [rsp + 46], m3
    movu     [rsp + 30], m0
    movu     [rsp + 12], m1
    movu     [rsp - 4], m2
    mov      r4w, [r3 + 24]
    mov      [rsp + 30], r4w
    mov      r4w, [r3 + 28]
    mov      [rsp + 28], r4w
    mov      r4w, [r3 + 46]
    mov      [rsp + 12], r4w

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]
    mova     m2, [pw_punpcklwd]
    mov      r6, r0

.loop:
    MODE_17_19 0
    add      r6, 8
    mov      r0, r6
    add      r2, 8
    dec      r4
    jnz     .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_20(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_20, 4,7,7,0-(6*mmsize+10)
    xchg     r2, r3
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 2*mmsize + 8], m0
    movu     [rsp + 3*mmsize + 8], m1
    movu     [rsp + 4*mmsize + 8], m2
    movu     [rsp + 5*mmsize + 8], m3

    mov      r4w, [r2 + 64]
    mov      [rsp + 104], r4w
    movu     m0, [r3 + 4]
    movu     m1, [r3 + 22]
    movu     m2, [r3 + 40]
    movd     m3, [r3 + 58]
    pshufb   m0, [shuf_mode_16_20]
    pshufb   m1, [shuf_mode_16_20]
    pshufb   m2, [shuf_mode_16_20]
    pshufb   m3, [shuf_mode_16_20]
    movu     [rsp + 24], m0
    movu     [rsp + 12], m1
    movu     [rsp], m2
    movd     [rsp], m3

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]
    mova     m2, [pw_punpcklwd]
    mov      r6, r0

.loop:
    MODE_16_20 0
    add      r6, 8
    mov      r0, r6
    add      r2, 8
    dec      r4
    jnz     .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_21(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_21, 4,7,7,0-(6*mmsize+2)
    xchg     r2, r3
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 2*mmsize], m0
    movu     [rsp + 3*mmsize], m1
    movu     [rsp + 4*mmsize], m2
    movu     [rsp + 5*mmsize], m3

    mov      r4w, [r2 + 64]
    mov      [rsp + 96], r4w
    movu     m0, [r3 + 4]
    movu     m1, [r3 + 18]
    movu     m2, [r3 + 34]
    movu     m3, [r3 + 48]
    pshufb   m0, [shuf_mode_15_21]
    pshufb   m1, [shuf_mode_15_21]
    pshufb   m2, [shuf_mode_15_21]
    pshufb   m3, [shuf_mode_15_21]
    movh     [rsp + 24], m0
    movh     [rsp + 16], m1
    movh     [rsp + 8], m2
    movh     [rsp], m3

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]
    mova     m2, [pw_punpcklwd]
    mov      r6, r0

.loop:
    MODE_15_21 0
    add      r6, 8
    mov      r0, r6
    add      r2, 8
    dec      r4
    jnz     .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_22(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_22, 4,7,7,0-(5*mmsize+10)
    xchg     r2, r3
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 1*mmsize + 8], m0
    movu     [rsp + 2*mmsize + 8], m1
    movu     [rsp + 3*mmsize + 8], m2
    movu     [rsp + 4*mmsize + 8], m3

    mov      r4w, [r2 + 64]
    mov      [rsp + 88], r4w
    mov      r4w, [r3+4]
    mov      [rsp+22], r4w
    movu     m0, [r3 + 10]
    movu     m1, [r3 + 30]
    movu     m2, [r3 + 50]
    pshufb   m0, [shuf_mode_14_22]
    pshufb   m1, [shuf_mode_14_22]
    pshufb   m2, [shuf_mode_14_22]
    movh     [rsp + 14], m0
    movh     [rsp + 6], m1
    movh     [rsp - 2], m2

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]
    mova     m2, [pw_punpcklwd]
    mov      r6, r0

.loop:
    MODE_14_22 0
    add      r6, 8
    mov      r0, r6
    add      r2, 8
    dec      r4
    jnz     .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_23(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_23, 4,7,7,0-(5*mmsize+2)
    xchg     r2, r3
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 1*mmsize], m0
    movu     [rsp + 2*mmsize], m1
    movu     [rsp + 3*mmsize], m2
    movu     [rsp + 4*mmsize], m3

    mov      r4w, [r2+64]
    mov      [rsp+80], r4w
    movu     m0, [r3 + 8]
    movu     m1, [r3 + 36]
    pshufb   m0, [shuf_mode_13_23]
    pshufb   m1, [shuf_mode_13_23]
    movh     [rsp + 8], m0
    movh     [rsp], m1
    mov      r4w, [r3+28]
    mov      [rsp+8], r4w
    mov      r4w, [r3+56]
    mov      [rsp], r4w

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]
    mova     m2, [pw_punpcklwd]
    mov      r6, r0

.loop:
    MODE_13_23 0
    add      r6, 8
    mov      r0, r6
    add      r2, 8
    dec      r4
    jnz     .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_24(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_24, 4,7,7,0-(4*mmsize+10)
    xchg     r2, r3
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]

    movu     [rsp + 0*mmsize + 8], m0
    movu     [rsp + 1*mmsize + 8], m1
    movu     [rsp + 2*mmsize + 8], m2
    movu     [rsp + 3*mmsize + 8], m3

    mov      r4w, [r2+64]
    mov      [rsp+72], r4w
    mov      r4w, [r3+12]
    mov      [rsp+6], r4w
    mov      r4w, [r3+26]
    mov      [rsp+4], r4w
    mov      r4w, [r3+38]
    mov      [rsp+2], r4w
    mov      r4w, [r3+52]
    mov      [rsp], r4w

    lea      r3, [ang_table + 16 * 16]
    mov      r4d, 8
    mov      r2, rsp
    add      r1, r1
    lea      r5, [r1 * 3]
    mov     r6, r0
    mova     m2, [pw_punpcklwd]

.loop:
    MODE_12_24 0
    add      r6, 8
    mov      r0, r6
    add      r2, 8
    dec      r4
    jnz      .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_25(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_25, 4,7,7,0-(4*mmsize+4)
    xchg     r2, r3
    movu     m0, [r2 + 0*mmsize]
    movu     m1, [r2 + 1*mmsize]
    movu     m2, [r2 + 2*mmsize]
    movu     m3, [r2 + 3*mmsize]
    movu     [rsp + 0*mmsize + 2], m0
    movu     [rsp + 1*mmsize + 2], m1
    movu     [rsp + 2*mmsize + 2], m2
    movu     [rsp + 3*mmsize + 2], m3
    mov      r4w, [r3+32]
    mov      [rsp], r4w
    mov      r4w, [r2+64]
    mov      [rsp+66], r4w

    lea         r3, [ang_table + 16 * 16]
    mov         r4d, 8
    mov         r2, rsp
    add         r1, r1
    lea         r5, [r1 * 3]
    mov         r6, r0

.loop:
    MODE_11_25 0
    add         r6, 8
    mov         r0, r6
    add         r2, 8
    dec         r4
    jnz         .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_26(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_26, 4,7,5
    mov         r6d, 4
    add         r1, r1
    lea         r2, [r1 * 2]
    lea         r4, [r1 * 3]
    lea         r5, [r1 * 4]
    mova        m4, [c_mode32_10_0]

    movu        m0, [r3 + 2]
    movu        m1, [r3 + 18]
    movu        m2, [r3 + 34]
    movu        m3, [r3 + 50]

.loop:
    movu        [r0], m0
    movu        [r0 + 16], m1
    movu        [r0 + 32], m2
    movu        [r0 + 48], m3

    movu        [r0 + r1], m0
    movu        [r0 + r1 + 16], m1
    movu        [r0 + r1 + 32], m2
    movu        [r0 + r1 + 48], m3

    movu        [r0 + r2], m0
    movu        [r0 + r2 + 16], m1
    movu        [r0 + r2 + 32], m2
    movu        [r0 + r2 + 48], m3

    movu        [r0 + r4], m0
    movu        [r0 + r4 + 16], m1
    movu        [r0 + r4 + 32], m2
    movu        [r0 + r4 + 48], m3

    add         r0, r5

    movu        [r0], m0
    movu        [r0 + 16], m1
    movu        [r0 + 32], m2
    movu        [r0 + 48], m3

    movu        [r0 + r1], m0
    movu        [r0 + r1 + 16], m1
    movu        [r0 + r1 + 32], m2
    movu        [r0 + r1 + 48], m3

    movu        [r0 + r2], m0
    movu        [r0 + r2 + 16], m1
    movu        [r0 + r2 + 32], m2
    movu        [r0 + r2 + 48], m3

    movu        [r0 + r4], m0
    movu        [r0 + r4 + 16], m1
    movu        [r0 + r4 + 32], m2
    movu        [r0 + r4 + 48], m3

    add         r0, r5
    dec         r6d
    jnz         .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_27(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_27, 4,7,8
    xchg   r2, r3mp
    lea    r3, [ang_table + 16 * 16]
    add    r1, r1
    lea    r5, [r1 * 3]
    mov    r6, r0
    mov    r4d, 8

.loop:
    MODE_9_27 0
    add    r6, 8
    mov    r0, r6
    add    r2, 8
    dec    r4
    jnz    .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_28(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_28, 4,7,8
    xchg   r2, r3mp
    lea    r3, [ang_table + 16 * 16]
    add    r1, r1
    lea    r5, [r1 * 3]
    mov    r6, r0
    mov    r4d, 8

.loop:
    MODE_8_28 0
    add    r6, 8
    mov    r0, r6
    add    r2, 8
    dec    r4
    jnz    .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_29(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_29, 4,7,8
    xchg   r2, r3mp
    lea    r3, [ang_table + 16 * 16]
    add    r1, r1
    lea    r5, [r1 * 3]
    mov    r6, r0
    mov    r4d, 8

.loop:
    MODE_7_29 0
    add    r6, 8
    mov    r0, r6
    add    r2, 8
    dec    r4
    jnz    .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_30(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_30, 4,7,8
    xchg   r2, r3mp
    lea    r3, [ang_table + 16 * 16]
    add    r1, r1
    lea    r5, [r1 * 3]
    mov    r6, r0
    mov    r4d, 8

.loop:
    MODE_6_30 0
    add    r6, 8
    mov    r0, r6
    add    r2, 8
    dec    r4
    jnz    .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_31(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_31, 4,7,8
    xchg   r2, r3mp
    lea    r3, [ang_table + 16 * 16]
    add    r1, r1
    lea    r5, [r1 * 3]
    mov    r6, r0
    mov    r4d, 8

.loop:
    MODE_5_31 0
    add    r6, 8
    mov    r0, r6
    add    r2, 8
    dec    r4
    jnz    .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_32(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_32, 4,7,8
    xchg   r2, r3mp
    lea    r3, [ang_table + 16 * 16]
    add    r1, r1
    lea    r5, [r1 * 3]
    mov    r6, r0
    mov    r4d, 8

.loop:
    MODE_4_32 0
    add    r6, 8
    mov    r0, r6
    add    r2, 8
    dec    r4
    jnz    .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_33(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_33, 4,7,8
    xchg   r2, r3mp
    lea    r3, [ang_table + 16 * 16]
    add    r1, r1
    lea    r5, [r1 * 3]
    mov    r6, r0
    mov    r4d, 8
.loop:
    MODE_3_33 0
    add    r6, 8
    mov    r0, r6
    add    r2, 8
    dec    r4
    jnz    .loop
    RET
