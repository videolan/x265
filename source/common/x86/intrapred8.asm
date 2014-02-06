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

pb_0_8        times 8 db  0,  8
pb_unpackbw1  times 2 db  1,  8,  2,  8,  3,  8,  4,  8
pb_swap8:     times 2 db  7,  6,  5,  4,  3,  2,  1,  0
c_trans_4x4           db  0,  4,  8, 12,  1,  5,  9, 13,  2,  6, 10, 14,  3,  7, 11, 15
tab_Si:               db  0,  1,  2,  3,  4,  5,  6,  7,  0,  1,  2,  3,  4,  5,  6,  7
pb_fact0:             db  0,  2,  4,  6,  8, 10, 12, 14,  0,  0,  0,  0,  0,  0,  0,  0
c_mode32_12_0:        db  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 13,  7,  0
c_mode32_13_0:        db  3,  6, 10, 13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
c_mode32_13_shuf:     db  0,  0,  0,  0,  0,  0,  0,  0,  7,  6,  5,  4,  3,  2,  1,  0
c_mode32_14_shuf:     db 15, 14, 13,  0,  2,  3,  4,  5,  6,  7, 10, 11, 12, 13, 14, 15
c_mode32_14_0:        db 15, 12, 10,  7,  5,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
c_mode32_15_0:        db 15, 13, 11,  9,  8,  6,  4,  2,  0,  0,  0,  0,  0,  0,  0,  0
c_mode32_16_0:        db 15, 14, 12, 11,  9,  8,  6,  5,  3,  2,  0,  0,  0,  0,  0,  0
c_mode32_17_0:        db 15, 14, 12, 11, 10,  9,  7,  6,  5,  4,  2,  1,  0,  0,  0,  0
c_mode32_18_0:        db 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0
c_shuf8_0:            db  0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,  8
c_deinterval8:        db  0,  8,  1,  9,  2, 10,  3, 11,  4, 12,  5, 13,  6, 14,  7, 15
tab_S1:               db 15, 14, 12, 11, 10,  9,  7,  6,  5,  4,  2,  1,  0,  0,  0,  0
pb_unpackbq:          db  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1
c_mode16_15:          db  0,  0,  0,  0,  0,  0,  0,  0, 15, 13, 11,  9,  8,  6,  4,  2
c_mode16_16:          db  8,  6,  5,  3,  2,  0, 15, 14, 12, 11,  9,  8,  6,  5,  3,  2
c_mode16_17:          db  4,  2,  1,  0, 15, 14, 12, 11, 10,  9,  7,  6,  5,  4,  2,  1

const ang_table
%assign x 0
%rep 32
    times 8 db (32-x), x
%assign x x+1
%endrep

SECTION .text

cextern pw_8
cextern pw_1024
cextern pb_unpackbd1
cextern multiL
cextern multiH
cextern multiH2
cextern multiH3
cextern multi_2Row

;-----------------------------------------------------------------------------
; void intra_pred_dc(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_dc4, 4,6,3
    mov         r4d, r5m
    inc         r2
    inc         r3
    pxor        m0, m0
    movd        m1, [r2]
    movd        m2, [r3]
    punpckldq   m1, m2
    psadbw      m1, m0              ; m1 = sum

    test        r4d, r4d

    mov         r4d, 4096
    movd        m2, r4d
    pmulhrsw    m1, m2              ; m1 = (sum + 4) / 8
    movd        r4d, m1             ; r4d = dc_val
    pshufb      m1, m0              ; m1 = byte [dc_val ...]

    ; store DC 4x4
    lea         r5, [r1 * 3]
    movd        [r0], m1
    movd        [r0 + r1], m1
    movd        [r0 + r1 * 2], m1
    movd        [r0 + r5], m1

    ; do DC filter
    jz         .end
    lea         r5d, [r4d * 2 + 2]  ; r5d = DC * 2 + 2
    add         r4d, r5d            ; r4d = DC * 3 + 2
    movd        m1, r4d
    pshuflw     m1, m1, 0           ; m1 = pixDCx3

    ; filter top
    pmovzxbw    m2, [r3]
    paddw       m2, m1
    psraw       m2, 2
    packuswb    m2, m2
    movd        [r0], m2            ; overwrite top-left pixel, we will update it later

    ; filter top-left
    movzx       r3d, byte [r3]
    add         r5d, r3d
    movzx       r3d, byte [r2]
    add         r3d, r5d
    shr         r3d, 2
    mov         [r0], r3b

    ; filter left
    add         r0, r1
    pmovzxbw    m2, [r2 + 1]
    paddw       m2, m1
    psraw       m2, 2
    packuswb    m2, m2
    pextrb      [r0], m2, 0
    pextrb      [r0 + r1], m2, 1
    pextrb      [r0 + r1 * 2], m2, 2

.end:
    RET


;-------------------------------------------------------------------------------------------
; void intra_pred_dc(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_dc8, 4, 7, 3
    mov             r4d,           r5m
    inc             r2
    inc             r3
    pxor            m0,            m0
    movh            m1,            [r2]
    movh            m2,            [r3]
    punpcklqdq      m1,            m2
    psadbw          m1,            m0
    pshufd          m2,            m1, 2
    paddw           m1,            m2

    movd            r5d,           m1
    add             r5d,           8
    shr             r5d,           4     ; sum = sum / 16
    movd            m1,            r5d
    pshufb          m1,            m0    ; m1 = byte [dc_val ...]

    test            r4d,           r4d

    ; store DC 8x8
    mov             r6,            r0
    movh            [r0],          m1
    movh            [r0 + r1],     m1
    lea             r0,            [r0 + r1 * 2]
    movh            [r0],          m1
    movh            [r0 + r1],     m1
    lea             r0,            [r0 + r1 * 2]
    movh            [r0],          m1
    movh            [r0 + r1],     m1
    lea             r0,            [r0 + r1 * 2]
    movh            [r0],          m1
    movh            [r0 + r1],     m1

    ; Do DC Filter
    jz              .end
    lea             r4d,           [r5d * 2 + 2]  ; r4d = DC * 2 + 2
    add             r5d,           r4d            ; r5d = DC * 3 + 2
    movd            m1,            r5d
    pshuflw         m1,            m1, 0          ; m1 = pixDCx3
    pshufd          m1,            m1, 0

    ; filter top
    pmovzxbw        m2,            [r3]
    paddw           m2,            m1
    psraw           m2,            2
    packuswb        m2,            m2
    movh            [r6],          m2

    ; filter top-left
    movzx           r3d, byte      [r3]
    add             r4d,           r3d
    movzx           r3d, byte      [r2]
    add             r3d,           r4d
    shr             r3d,           2
    mov             [r6],          r3b

    ; filter left
    add             r6,            r1
    pmovzxbw        m2,            [r2 + 1]
    paddw           m2,            m1
    psraw           m2,            2
    packuswb        m2,            m2
    pextrb          [r6],          m2, 0
    pextrb          [r6 + r1],     m2, 1
    pextrb          [r6 + 2 * r1], m2, 2
    lea             r6,            [r6 + r1 * 2]
    pextrb          [r6 + r1],     m2, 3
    pextrb          [r6 + r1 * 2], m2, 4
    pextrb          [r6 + r1 * 4], m2, 6
    lea             r1,            [r1 * 3]
    pextrb          [r6 + r1],     m2, 5

.end
    RET

;-------------------------------------------------------------------------------------------
; void intra_pred_dc(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_dc16, 5, 7, 4
    mov             r4d,           r5m
    inc             r2
    inc             r3
    pxor            m0,            m0
    movu            m1,            [r2]
    movu            m2,            [r3]
    psadbw          m1,            m0
    psadbw          m2,            m0
    paddw           m1,            m2
    pshufd          m2,            m1, 2
    paddw           m1,            m2

    movd            r5d,           m1
    add             r5d,           16
    shr             r5d,           5     ; sum = sum / 32
    movd            m1,            r5d
    pshufb          m1,            m0    ; m1 = byte [dc_val ...]

    test            r4d,           r4d

    ; store DC 16x16
    mov             r6,            r0
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    lea             r0,            [r0 + r1 * 2]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    lea             r0,            [r0 + r1 * 2]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    lea             r0,            [r0 + r1 * 2]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    lea             r0,            [r0 + r1 * 2]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    lea             r0,            [r0 + r1 * 2]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    lea             r0,            [r0 + r1 * 2]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    lea             r0,            [r0 + r1 * 2]
    movu            [r0],          m1
    movu            [r0 + r1],     m1

    ; Do DC Filter
    jz              .end
    lea             r4d,           [r5d * 2 + 2]  ; r4d = DC * 2 + 2
    add             r5d,           r4d            ; r5d = DC * 3 + 2
    movd            m1,            r5d
    pshuflw         m1,            m1, 0          ; m1 = pixDCx3
    pshufd          m1,            m1, 0

    ; filter top
    pmovzxbw        m2,            [r3]
    paddw           m2,            m1
    psraw           m2,            2
    packuswb        m2,            m2
    movh            [r6],          m2
    pmovzxbw        m3,            [r3 + 8]
    paddw           m3,            m1
    psraw           m3,            2
    packuswb        m3,            m3
    movh            [r6 + 8],      m3

    ; filter top-left
    movzx           r3d, byte      [r3]
    add             r4d,           r3d
    movzx           r3d, byte      [r2]
    add             r3d,           r4d
    shr             r3d,           2
    mov             [r6],          r3b

    ; filter left
    add             r6,            r1
    pmovzxbw        m2,            [r2 + 1]
    paddw           m2,            m1
    psraw           m2,            2
    packuswb        m2,            m2
    pextrb          [r6],          m2, 0
    pextrb          [r6 + r1],     m2, 1
    pextrb          [r6 + r1 * 2], m2, 2
    lea             r6,            [r6 + r1 * 2]
    pextrb          [r6 + r1],     m2, 3
    pextrb          [r6 + r1 * 2], m2, 4
    lea             r6,            [r6 + r1 * 2]
    pextrb          [r6 + r1],     m2, 5
    pextrb          [r6 + r1 * 2], m2, 6
    lea             r6,            [r6 + r1 * 2]
    pextrb          [r6 + r1],     m2, 7

    pmovzxbw        m3,            [r2 + 9]
    paddw           m3,            m1
    psraw           m3,            2
    packuswb        m3,            m3
    pextrb          [r6 + r1 * 2], m3, 0
    lea             r6,            [r6 + r1 * 2]
    pextrb          [r6 + r1],     m3, 1
    pextrb          [r6 + r1 * 2], m3, 2
    lea             r6,            [r6 + r1 * 2]
    pextrb          [r6 + r1],     m3, 3
    pextrb          [r6 + r1 * 2], m3, 4
    lea             r6,            [r6 + r1 * 2]
    pextrb          [r6 + r1],     m3, 5
    pextrb          [r6 + r1 * 2], m3, 6

.end
    RET

;-------------------------------------------------------------------------------------------
; void intra_pred_dc(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_dc32, 4, 5, 5
    inc             r2
    inc             r3
    pxor            m0,            m0
    movu            m1,            [r2]
    movu            m2,            [r2 + 16]
    movu            m3,            [r3]
    movu            m4,            [r3 + 16]
    psadbw          m1,            m0
    psadbw          m2,            m0
    psadbw          m3,            m0
    psadbw          m4,            m0
    paddw           m1,            m2
    paddw           m3,            m4
    paddw           m1,            m3
    pshufd          m2,            m1, 2
    paddw           m1,            m2

    movd            r4d,           m1
    add             r4d,           32
    shr             r4d,           6     ; sum = sum / 64
    movd            m1,            r4d
    pshufb          m1,            m0    ; m1 = byte [dc_val ...]

%rep 2
    ; store DC 16x16
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    movu            [r0 + 16],     m1
    movu            [r0 + r1 + 16],m1
    lea             r0,            [r0 + 2 * r1]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    movu            [r0 + 16],     m1
    movu            [r0 + r1 + 16],m1
    lea             r0,            [r0 + 2 * r1]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    movu            [r0 + 16],     m1
    movu            [r0 + r1 + 16],m1
    lea             r0,            [r0 + 2 * r1]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    movu            [r0 + 16],     m1
    movu            [r0 + r1 + 16],m1
    lea             r0,            [r0 + 2 * r1]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    movu            [r0 + 16],     m1
    movu            [r0 + r1 + 16],m1
    lea             r0,            [r0 + 2 * r1]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    movu            [r0 + 16],     m1
    movu            [r0 + r1 + 16],m1
    lea             r0,            [r0 + 2 * r1]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    movu            [r0 + 16],     m1
    movu            [r0 + r1 + 16],m1
    lea             r0,            [r0 + 2 * r1]
    movu            [r0],          m1
    movu            [r0 + r1],     m1
    movu            [r0 + 16],     m1
    movu            [r0 + r1 + 16],m1
    lea             r0,            [r0 + 2 * r1]
%endrep

    RET

;-----------------------------------------------------------------------------------------------------------
; void intra_pred_planar(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-----------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_planar4, 4,7,5
    inc             r2
    inc             r3
    pmovzxbw        m0,         [r3]      ; topRow[i] = above[i];
    punpcklqdq      m0,         m0

    pxor            m1,         m1
    movd            m2,         [r2 + 4]  ; bottomLeft = left[4]
    movzx           r6d, byte   [r3 + 4]  ; topRight   = above[4];
    pshufb          m2,         m1
    punpcklbw       m2,         m1
    psubw           m2,         m0        ; bottomRow[i] = bottomLeft - topRow[i]
    psllw           m0,         2
    punpcklqdq      m3,         m2, m1
    psubw           m0,         m3
    paddw           m2,         m2

%macro COMP_PRED_PLANAR_2ROW 1
    movzx           r4d, byte   [r2 + %1]
    lea             r4d,        [r4d * 4 + 4]
    movd            m3,         r4d
    pshuflw         m3,         m3, 0

    movzx           r4d, byte   [r2 + %1 + 1]
    lea             r4d,        [r4d * 4 + 4]
    movd            m4,         r4d
    pshuflw         m4,         m4, 0
    punpcklqdq      m3,         m4        ; horPred

    movzx           r4d, byte   [r2 + %1]
    mov             r5d,        r6d
    sub             r5d,        r4d
    movd            m4,         r5d
    pshuflw         m4,         m4, 0

    movzx           r4d, byte   [r2 + %1 + 1]
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
    packuswb        m3,         m3

    movd            [r0],       m3
    pshufd          m3,         m3, 0x55
    movd            [r0 + r1],  m3
    lea             r0,         [r0 + 2 * r1]
%endmacro

    COMP_PRED_PLANAR_2ROW 0
    COMP_PRED_PLANAR_2ROW 2

    RET

;-----------------------------------------------------------------------------------------------------------
; void intra_pred_planar(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-----------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_planar8, 4,4,7
    inc             r2
    inc             r3
    pxor            m0,     m0
    pmovzxbw        m1,     [r3]     ; v_topRow
    pmovzxbw        m2,     [r2]     ; v_leftColumn

    movd            m3,     [r3 + 8] ; topRight   = above[8];
    movd            m4,     [r2 + 8] ; bottomLeft = left[8];

    pshufb          m3,     m0
    pshufb          m4,     m0
    punpcklbw       m3,     m0       ; v_topRight
    punpcklbw       m4,     m0       ; v_bottomLeft

    psubw           m4,     m1       ; v_bottomRow
    psubw           m3,     m2       ; v_rightColumn

    psllw           m1,     3        ; v_topRow
    psllw           m2,     3        ; v_leftColumn

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
    packuswb        m5,     m5

    movh            [r0],   m5
    lea             r0,     [r0 + r1]

%endmacro

    PRED_PLANAR_ROW8 0
    PRED_PLANAR_ROW8 1
    PRED_PLANAR_ROW8 2
    PRED_PLANAR_ROW8 3
    PRED_PLANAR_ROW8 4
    PRED_PLANAR_ROW8 5
    PRED_PLANAR_ROW8 6
    PRED_PLANAR_ROW8 7

    RET


;-----------------------------------------------------------------------------------------------------------
; void intra_pred_planar(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-----------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_planar16, 4,6,8
    inc             r2
    inc             r3
    pxor            m0,         m0
    pmovzxbw        m1,         [r3]       ; topRow[0-7]
    pmovzxbw        m2,         [r3 + 8]   ; topRow[8-15]

    movd            m3,         [r2 + 16]
    pshufb          m3,         m0
    punpcklbw       m3,         m0         ; v_bottomLeft = left[16]
    movzx           r4d, byte   [r3 + 16]  ; topRight     = above[16]

    psubw           m4,         m3, m1     ; v_bottomRow[0]
    psubw           m5,         m3, m2     ; v_bottomRow[1]

    psllw           m1,         4
    psllw           m2,         4

%macro PRED_PLANAR_ROW16 1
    movzx           r5d, byte   [r2 + %1]
    add             r5d,        r5d
    lea             r5d,        [r5d * 8 + 16]
    movd            m3,         r5d
    pshuflw         m3,         m3, 0
    pshufd          m3,         m3, 0      ; horPred

    movzx           r5d, byte   [r2 + %1]
    mov             r3d,        r4d
    sub             r3d,        r5d
    movd            m6,         r3d
    pshuflw         m6,         m6, 0
    pshufd          m6,         m6, 0

    pmullw          m7,         m6, [multiL]
    paddw           m7,         m3
    paddw           m1,         m4
    paddw           m7,         m1
    psraw           m7,         5

    pmullw          m6,         m6, [multiH]
    paddw           m3,         m6
    paddw           m2,         m5
    paddw           m3,         m2
    psraw           m3,         5

    packuswb        m7,         m3
    movu            [r0],       m7
    lea             r0,         [r0 + r1]
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

    RET


;-----------------------------------------------------------------------------------------------------------
; void intra_pred_planar(pixel* dst, intptr_t dstStride, pixel* left, pixel* above, int dirMode, int filter)
;-----------------------------------------------------------------------------------------------------------
INIT_XMM sse4
%if ARCH_X86_64 == 1
cglobal intra_pred_planar32, 4,7,12
  %define bottomRow0    m8
  %define bottomRow1    m9
  %define bottomRow2    m10
  %define bottomRow3    m11
%else
cglobal intra_pred_planar32, 4,7,8,0-(4*mmsize)
  %define bottomRow0    [rsp + 0 * mmsize]
  %define bottomRow1    [rsp + 1 * mmsize]
  %define bottomRow2    [rsp + 2 * mmsize]
  %define bottomRow3    [rsp + 3 * mmsize]
%endif
    inc             r2
    inc             r3
    pxor            m3,         m3
    movd            m0,         [r2 + 32]
    pshufb          m0,         m3
    punpcklbw       m0,         m3          ; v_bottomLeft = left[32]
    movzx           r4d, byte   [r3 + 32]   ; topRight     = above[32]

    pmovzxbw        m1,         [r3 + 0]    ; topRow[0]
    pmovzxbw        m2,         [r3 + 8]    ; topRow[1]
    pmovzxbw        m3,         [r3 +16]    ; topRow[2]
    pmovzxbw        m4,         [r3 +24]    ; topRow[3]

    psubw           m5,         m0, m1      ; v_bottomRow[0]
    psubw           m6,         m0, m2      ; v_bottomRow[1]
    psubw           m7,         m0, m3      ; v_bottomRow[2]
    psubw           m0,         m4          ; v_bottomRow[3]

    mova            bottomRow0, m5
    mova            bottomRow1, m6
    mova            bottomRow2, m7
    mova            bottomRow3, m0

    psllw           m1,         5
    psllw           m2,         5
    psllw           m3,         5
    psllw           m4,         5

%macro COMP_PRED_PLANAR_ROW 1
    movzx           r5d,   byte [r2]
    shl             r5d,        5
    add             r5d,        32
    movd            m5,         r5d
    pshuflw         m5,         m5, 0
    pshufd          m5,         m5, 0      ; horPred

    movzx           r5d,   byte [r2]
    mov             r6d,        r4d
    sub             r6d,        r5d
    movd            m6,         r6d
    pshuflw         m6,         m6, 0
    pshufd          m6,         m6, 0

%if (%1 == 0)
    pmullw          m7,         m6, [multiL]
%else
    pmullw          m7,         m6, [multiH2]
%endif

    paddw           m7,         m5
%if (%1 == 0)
    paddw           m1,         bottomRow0
    paddw           m7,         m1
%else
    paddw           m3,         bottomRow2
    paddw           m7,         m3
%endif
    psraw           m7,         6

%if (%1 == 0)
    pmullw          m6,        [multiH]
%else
    pmullw          m6,        [multiH3]
%endif
    paddw           m6,         m5
%if (%1 == 0)
    paddw           m2,         bottomRow1
    paddw           m6,         m2
%else
    paddw           m4,         bottomRow3
    paddw           m6,         m4
%endif
    psraw           m6,         6

    packuswb        m7,         m6
    movu            [r0 + %1],  m7
%endmacro

    mov r3,         32
.loop
    COMP_PRED_PLANAR_ROW 0
    COMP_PRED_PLANAR_ROW 16
    inc             r2
    lea             r0,         [r0 + r1]

    dec             r3
    jnz .loop
%undef COMP_PRED_PLANAR_ROW

    RET

;-----------------------------------------------------------------------------
; void intraPredAng(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal intra_pred_ang4_2, 3,3,4
    cmp         r4m, byte 34
    cmove       r2, r3mp
    movh        m0, [r2 + 2]
    movd        [r0], m0
    palignr     m1, m0, 1
    movd        [r0 + r1], m1
    palignr     m2, m0, 2
    movd        [r0 + r1 * 2], m2
    lea         r1, [r1 * 3]
    psrldq      m0, 3
    movd        [r0 + r1], m0
    RET


INIT_XMM sse4
cglobal intra_pred_ang4_3, 3,4,5
    cmp         r4m, byte 33
    cmove       r2, r3mp
    lea         r3, [ang_table + 20 * 16]
    movh        m0, [r2 + 1]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 1       ; [x 8 7 6 5 4 3 2]
    punpcklbw   m0, m1          ; [x 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    palignr     m1, m0, 2       ; [x x x x x x x x 6 5 5 4 4 3 3 2]
    palignr     m2, m0, 4       ; [x x x x x x x x 7 6 6 5 5 4 4 3]
    palignr     m3, m0, 6       ; [x x x x x x x x 8 7 7 6 6 5 5 4]
    punpcklqdq  m0, m1
    punpcklqdq  m2, m3

    movh        m3, [r3 + 6 * 16]   ; [26]
    movhps      m3, [r3]            ; [20]
    movh        m4, [r3 - 6 * 16]   ; [14]
    movhps      m4, [r3 - 12 * 16]  ; [ 8]
    jmp        .do_filter4x4

    ; NOTE: share path, input is m0=[1 0], m2=[3 2], m3,m4=coef, flag_z=no_transpose
ALIGN 16
.do_filter4x4:
    mova        m1, [pw_1024]

    pmaddubsw   m0, m3
    pmulhrsw    m0, m1
    pmaddubsw   m2, m4
    pmulhrsw    m2, m1
    packuswb    m0, m2

    ; NOTE: mode 33 doesn't reorde, UNSAFE but I don't use any instruction that affect eflag register before
    jz         .store

    ; transpose 4x4
    pshufb      m0, [c_trans_4x4]

.store:
    ; TODO: use pextrd here after intrinsic ssse3 removed
    movd        [r0], m0
    pextrd      [r0 + r1], m0, 1
    pextrd      [r0 + r1 * 2], m0, 2
    lea         r1, [r1 * 3]
    pextrd      [r0 + r1], m0, 3
    RET


cglobal intra_pred_ang4_4, 3,4,5
    cmp         r4m, byte 32
    cmove       r2, r3mp
    lea         r3, [ang_table + 18 * 16]
    movh        m0, [r2 + 1]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 1       ; [x 8 7 6 5 4 3 2]
    punpcklbw   m0, m1          ; [x 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    palignr     m1, m0, 2       ; [x x x x x x x x 6 5 5 4 4 3 3 2]
    palignr     m3, m0, 4       ; [x x x x x x x x 7 6 6 5 5 4 4 3]
    punpcklqdq  m0, m1
    punpcklqdq  m2, m1, m3

    movh        m3, [r3 +  3 * 16]  ; [21]
    movhps      m3, [r3 -  8 * 16]  ; [10]
    movh        m4, [r3 + 13 * 16]  ; [31]
    movhps      m4, [r3 +  2 * 16]  ; [20]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_5, 3,4,5
    cmp         r4m, byte 31
    cmove       r2, r3mp
    lea         r3, [ang_table + 10 * 16]
    movh        m0, [r2 + 1]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 1       ; [x 8 7 6 5 4 3 2]
    punpcklbw   m0, m1          ; [x 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    palignr     m1, m0, 2       ; [x x x x x x x x 6 5 5 4 4 3 3 2]
    palignr     m3, m0, 4       ; [x x x x x x x x 7 6 6 5 5 4 4 3]
    punpcklqdq  m0, m1
    punpcklqdq  m2, m1, m3

    movh        m3, [r3 +  7 * 16]  ; [17]
    movhps      m3, [r3 -  8 * 16]  ; [ 2]
    movh        m4, [r3 +  9 * 16]  ; [19]
    movhps      m4, [r3 -  6 * 16]  ; [ 4]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_6, 3,4,5
    cmp         r4m, byte 30
    cmove       r2, r3mp
    lea         r3, [ang_table + 19 * 16]
    movh        m0, [r2 + 1]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 1       ; [x 8 7 6 5 4 3 2]
    punpcklbw   m0, m1          ; [x 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    palignr     m2, m0, 2       ; [x x x x x x x x 6 5 5 4 4 3 3 2]
    punpcklqdq  m0, m0
    punpcklqdq  m2, m2

    movh        m3, [r3 -  6 * 16]  ; [13]
    movhps      m3, [r3 +  7 * 16]  ; [26]
    movh        m4, [r3 - 12 * 16]  ; [ 7]
    movhps      m4, [r3 +  1 * 16]  ; [20]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_7, 3,4,5
    cmp         r4m, byte 29
    cmove       r2, r3mp
    lea         r3, [ang_table + 20 * 16]
    movh        m0, [r2 + 1]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 1       ; [x 8 7 6 5 4 3 2]
    punpcklbw   m0, m1          ; [x 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    palignr     m3, m0, 2       ; [x x x x x x x x 6 5 5 4 4 3 3 2]
    punpcklqdq  m2, m0, m3
    punpcklqdq  m0, m0

    movh        m3, [r3 - 11 * 16]  ; [ 9]
    movhps      m3, [r3 -  2 * 16]  ; [18]
    movh        m4, [r3 +  7 * 16]  ; [27]
    movhps      m4, [r3 - 16 * 16]  ; [ 4]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_8, 3,4,5
    cmp         r4m, byte 28
    cmove       r2, r3mp
    lea         r3, [ang_table + 13 * 16]
    movh        m0, [r2 + 1]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 1       ; [x 8 7 6 5 4 3 2]
    punpcklbw   m0, m1          ; [x 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    punpcklqdq  m0, m0
    mova        m2, m0

    movh        m3, [r3 -  8 * 16]  ; [ 5]
    movhps      m3, [r3 -  3 * 16]  ; [10]
    movh        m4, [r3 +  2 * 16]  ; [15]
    movhps      m4, [r3 +  7 * 16]  ; [20]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_9, 3,4,5
    cmp         r4m, byte 27
    cmove       r2, r3mp
    lea         r3, [ang_table + 4 * 16]
    movh        m0, [r2 + 1]    ; [8 7 6 5 4 3 2 1]
    palignr     m1, m0, 1       ; [x 8 7 6 5 4 3 2]
    punpcklbw   m0, m1          ; [x 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    punpcklqdq  m0, m0
    mova        m2, m0

    movh        m3, [r3 -  2 * 16]  ; [ 2]
    movhps      m3, [r3 -  0 * 16]  ; [ 4]
    movh        m4, [r3 +  2 * 16]  ; [ 6]
    movhps      m4, [r3 +  4 * 16]  ; [ 8]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_10, 3,3,4
    movd        m0, [r2 + 1]            ; [8 7 6 5 4 3 2 1]
    pshufb      m0, [pb_unpackbd1]

    pshufd      m1, m0, 1
    movhlps     m2, m0
    pshufd      m3, m0, 3
    movd        [r0 + r1], m1
    movd        [r0 + r1 * 2], m2
    lea         r1, [r1 * 3]
    movd        [r0 + r1], m3

    cmp         r5m, byte 0
    jz         .quit

    ; filter
    mov         r2, r3mp
    pmovzxbw    m0, m0                  ; [-1 -1 -1 -1]
    movh        m1, [r2]                ; [4 3 2 1 0]
    pshufb      m2, m1, [pb_0_8]        ; [0 0 0 0]
    pshufb      m1, [pb_unpackbw1]      ; [4 3 2 1]
    psubw       m1, m2
    psraw       m1, 1
    paddw       m0, m1
    packuswb    m0, m0

.quit:
    movd        [r0], m0
    RET


INIT_XMM sse4
cglobal intra_pred_ang4_26, 4,4,3
    movd        m0, [r3 + 1]            ; [8 7 6 5 4 3 2 1]

    ; store
    movd        [r0], m0
    movd        [r0 + r1], m0
    movd        [r0 + r1 * 2], m0
    lea         r3, [r1 * 3]
    movd        [r0 + r3], m0

    ; filter
    cmp         r5m, byte 0
    jz         .quit

    pshufb      m0, [pb_0_8]            ; [ 1  1  1  1]
    movh        m1, [r2]                ; [-4 -3 -2 -1 0]
    pshufb      m2, m1, [pb_0_8]        ; [0 0 0 0]
    pshufb      m1, [pb_unpackbw1]      ; [-4 -3 -2 -1]
    psubw       m1, m2
    psraw       m1, 1
    paddw       m0, m1
    packuswb    m0, m0

    pextrb      [r0], m0, 0
    pextrb      [r0 + r1], m0, 1
    pextrb      [r0 + r1 * 2], m0, 2
    pextrb      [r0 + r3], m0, 3

.quit:
    RET


cglobal intra_pred_ang4_11, 3,4,5
    cmp         r4m, byte 25
    cmove       r2, r3mp
    lea         r3, [ang_table + 24 * 16]
    movh        m0, [r2]        ; [x x x 4 3 2 1 0]
    palignr     m1, m0, 1       ; [x x x x 4 3 2 1]
    punpcklbw   m0, m1          ; [x x x x x x x x 4 3 3 2 2 1 1 0]
    punpcklqdq  m0, m0
    mova        m2, m0

    movh        m3, [r3 +  6 * 16]  ; [24]
    movhps      m3, [r3 +  4 * 16]  ; [26]
    movh        m4, [r3 +  2 * 16]  ; [28]
    movhps      m4, [r3 +  0 * 16]  ; [30]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_12, 3,4,5
    cmp         r4m, byte 24
    cmove       r2, r3mp
    lea         r3, [ang_table + 20 * 16]
    movh        m0, [r2]        ; [x x x 4 3 2 1 0]
    palignr     m1, m0, 1       ; [x x x x 4 3 2 1]
    punpcklbw   m0, m1          ; [x x x x x x x x 4 3 3 2 2 1 1 0]
    punpcklqdq  m0, m0
    mova        m2, m0

    movh        m3, [r3 +  7 * 16]  ; [27]
    movhps      m3, [r3 +  2 * 16]  ; [22]
    movh        m4, [r3 -  3 * 16]  ; [17]
    movhps      m4, [r3 -  8 * 16]  ; [12]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_13, 4,4,5
    cmp         r4m, byte 23
    jnz        .load
    xchg        r2, r3
.load
    movh        m1, [r2 - 1]    ; [x x 4 3 2 1 0 x]
    palignr     m0, m1, 1       ; [x x x 4 3 2 1 0]
    palignr     m2, m1, 2       ; [x x x x 4 3 2 1]
    pinsrb      m1, [r3 + 4], 0
    punpcklbw   m1, m0          ; [3 2 2 1 1 0 0 x]
    punpcklbw   m0, m2          ; [4 3 3 2 2 1 1 0]
    punpcklqdq  m2, m0, m1
    punpcklqdq  m0, m0

    lea         r3, [ang_table + 21 * 16]
    movh        m3, [r3 +  2 * 16]  ; [23]
    movhps      m3, [r3 -  7 * 16]  ; [14]
    movh        m4, [r3 - 16 * 16]  ; [ 5]
    movhps      m4, [r3 +  7 * 16]  ; [28]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_14, 4,4,5
    cmp         r4m, byte 22
    jnz        .load
    xchg        r2, r3
.load
    movh        m2, [r2 - 1]    ; [x x 4 3 2 1 0 x]
    palignr     m0, m2, 1       ; [x x x 4 3 2 1 0]
    palignr     m1, m2, 2       ; [x x x x 4 3 2 1]
    pinsrb      m2, [r3 + 2], 0
    punpcklbw   m2, m0          ; [3 2 2 1 1 0 0 x]
    punpcklbw   m0, m1          ; [4 3 3 2 2 1 1 0]
    punpcklqdq  m0, m0
    punpcklqdq  m2, m2

    lea         r3, [ang_table + 19 * 16]
    movh        m3, [r3 +  0 * 16]  ; [19]
    movhps      m3, [r3 - 13 * 16]  ; [ 6]
    movh        m4, [r3 +  6 * 16]  ; [25]
    movhps      m4, [r3 -  7 * 16]  ; [12]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_15, 4,4,5
    cmp         r4m, byte 21
    jnz        .load
    xchg        r2, r3
.load
    movh        m2, [r2 - 1]    ; [x x 4 3 2 1 0 x]
    palignr     m0, m2, 1       ; [x x x 4 3 2 1 0]
    palignr     m1, m2, 2       ; [x x x x 4 3 2 1]
    pinsrb      m2, [r3 + 2], 0
    pslldq      m3, m2, 1       ; [x 4 3 2 1 0 x y]
    pinsrb      m3, [r3 + 4], 0
    punpcklbw   m4, m3, m2      ; [2 1 1 0 0 x x y]
    punpcklbw   m2, m0          ; [3 2 2 1 1 0 0 x]
    punpcklbw   m0, m1          ; [4 3 3 2 2 1 1 0]
    punpcklqdq  m0, m2
    punpcklqdq  m2, m4

    lea         r3, [ang_table + 23 * 16]
    movh        m3, [r3 -  8 * 16]  ; [15]
    movhps      m3, [r3 +  7 * 16]  ; [30]
    movh        m4, [r3 - 10 * 16]  ; [13]
    movhps      m4, [r3 +  5 * 16]  ; [28]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_16, 4,4,5
    cmp         r4m, byte 20
    jnz        .load
    xchg        r2, r3
.load
    movh        m2, [r2 - 1]    ; [x x 4 3 2 1 0 x]
    palignr     m0, m2, 1       ; [x x x 4 3 2 1 0]
    palignr     m1, m2, 2       ; [x x x x 4 3 2 1]
    pinsrb      m2, [r3 + 2], 0
    pslldq      m3, m2, 1       ; [x 4 3 2 1 0 x y]
    pinsrb      m3, [r3 + 3], 0
    punpcklbw   m4, m3, m2      ; [2 1 1 0 0 x x y]
    punpcklbw   m2, m0          ; [3 2 2 1 1 0 0 x]
    punpcklbw   m0, m1          ; [4 3 3 2 2 1 1 0]
    punpcklqdq  m0, m2
    punpcklqdq  m2, m4

    lea         r3, [ang_table + 19 * 16]
    movh        m3, [r3 -  8 * 16]  ; [11]
    movhps      m3, [r3 +  3 * 16]  ; [22]
    movh        m4, [r3 - 18 * 16]  ; [ 1]
    movhps      m4, [r3 -  7 * 16]  ; [12]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_17, 4,4,5
    cmp         r4m, byte 19
    jnz        .load
    xchg        r2, r3
.load
    movh        m3, [r2 - 1]    ; [- - 4 3 2 1 0 x]
    palignr     m0, m3, 1       ; [- - - 4 3 2 1 0]
    palignr     m1, m3, 2       ; [- - - - 4 3 2 1]
    mova        m4, m0
    punpcklbw   m0, m1          ; [4 3 3 2 2 1 1 0]

    pinsrb      m3, [r3 + 1], 0
    punpcklbw   m1, m3, m4      ; [3 2 2 1 1 0 0 x]
    punpcklqdq  m0, m1

    pslldq      m2, m3, 1       ; [- 4 3 2 1 0 x y]
    pinsrb      m2, [r3 + 2], 0
    pslldq      m1, m2, 1       ; [4 3 2 1 0 x y z]
    pinsrb      m1, [r3 + 4], 0
    punpcklbw   m1, m2          ; [1 0 0 x x y y z]
    punpcklbw   m2, m3          ; [2 1 1 0 0 x x y]
    punpcklqdq  m2, m1

    lea         r3, [ang_table + 14 * 16]
    movh        m3, [r3 -  8 * 16]  ; [ 6]
    movhps      m3, [r3 -  2 * 16]  ; [12]
    movh        m4, [r3 +  4 * 16]  ; [18]
    movhps      m4, [r3 + 10 * 16]  ; [24]
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang4_3 %+ SUFFIX %+ .do_filter4x4)


cglobal intra_pred_ang4_18, 4,4,1
    mov         r2d, [r2]
    bswap       r2d
    movd        m0, r2d
    pinsrd      m0, [r3 + 1], 1     ; [- 3 2 1 0 -1 -2 -3]
    lea         r2, [r1 * 3]
    movd        [r0 + r2], m0
    psrldq      m0, 1
    movd        [r0 + r1 * 2], m0
    psrldq      m0, 1
    movd        [r0 + r1], m0
    psrldq      m0, 1
    movd        [r0], m0
    RET
;-----------------------------------------------------------------------------
; void intraPredAng8(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal intra_pred_ang8_2, 3,5,2
    cmp         r4m,            byte 34
    cmove       r2,             r3mp
    movu        m0,             [r2 + 2]
    lea         r4,             [r1 * 3]

    movh        [r0],           m0
    palignr     m1,             m0, 1
    movh        [r0 + r1],      m1
    palignr     m1,             m0, 2
    movh        [r0 + r1 * 2],  m1
    palignr     m1,             m0, 3
    movh        [r0 + r4],      m1
    palignr     m1,             m0, 4
    lea         r0,             [r0 + r1 * 4]
    movh        [r0],           m1
    palignr     m1,             m0, 5
    movh        [r0 + r1],      m1
    palignr     m1,             m0, 6
    movh        [r0 + r1 * 2],  m1
    palignr     m1,             m0, 7
    movh        [r0 + r4],      m1
    RET

INIT_XMM sse4
cglobal intra_pred_ang8_3, 3,5,7
    cmp         r4m,       byte 33
    cmove       r2,        r3mp
    lea         r3,        [ang_table + 14 * 16]
    mova        m3,        [pw_1024]

    movu        m0,        [r2 + 1]                   ; [16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1]
    palignr     m1,        m0, 1                      ; [x 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2]

    punpckhbw   m2,        m0, m1                     ; [x 16 16 15 15 14 14 13 13 12 12 11 11 10 10 9]
    punpcklbw   m0,        m1                         ; [9 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    palignr     m1,        m2, m0, 2                  ; [10 9 9 8 8 7 7 6 6 5 5 4 4 3 3 2]

    pmaddubsw   m4,        m0, [r3 + 12 * 16]         ; [26]
    pmulhrsw    m4,        m3
    pmaddubsw   m1,        [r3 + 6 * 16]              ; [20]
    pmulhrsw    m1,        m3
    packuswb    m4,        m1

    palignr     m5,        m2, m0, 4                  ; [11 10 10 9 9 8 8 7 7 6 6 5 5 4 4 3]

    pmaddubsw   m5,        [r3]                       ; [14]
    pmulhrsw    m5,        m3

    palignr     m6,        m2, m0, 6                  ; [12 11 11 10 10 9 9 8 8 7 7 6 6 5 5 4]

    pmaddubsw   m6,        [r3 - 6 * 16]              ; [ 8]
    pmulhrsw    m6,        m3
    packuswb    m5,        m6

    palignr     m1,        m2, m0, 8                  ; [13 12 12 11 11 10 10 9 9 8 8 7 7 6 6 5]

    pmaddubsw   m6,        m1, [r3 - 12 * 16]         ; [ 2]
    pmulhrsw    m6,        m3

    pmaddubsw   m1,        [r3 + 14 * 16]             ; [28]
    pmulhrsw    m1,        m3
    packuswb    m6,        m1

    palignr     m1,        m2, m0, 10                 ; [14 13 13 12 12 11 11 10 10 9 9 8 8 7 7 6]

    pmaddubsw   m1,        [r3 + 8 * 16]              ; [22]
    pmulhrsw    m1,        m3

    palignr     m2,        m0, 12                     ; [15 14 14 13 13 12 12 11 11 10 10 9 9 8 8 7]

    pmaddubsw   m2,        [r3 + 2 * 16]              ; [16]
    pmulhrsw    m2,        m3
    packuswb    m1,        m2

.transpose8x8:
    jz         .store

    ; transpose 8x8
    punpckhbw   m0,        m4, m5
    punpcklbw   m4,        m5
    punpckhbw   m2,        m4, m0
    punpcklbw   m4,        m0

    punpckhbw   m0,        m6, m1
    punpcklbw   m6,        m1
    punpckhbw   m1,        m6, m0
    punpcklbw   m6,        m0

    punpckhdq   m5,        m4, m6
    punpckldq   m4,        m6
    punpckldq   m6,        m2, m1
    punpckhdq   m2,        m1
    mova        m1,        m2

.store:
    lea         r4,              [r1 * 3]
    movh        [r0],            m4
    movhps      [r0 + r1],       m4
    movh        [r0 + r1 * 2],   m5
    movhps      [r0 + r4],       m5
    lea         r0,              [r0 + r1 * 4]
    movh        [r0],            m6
    movhps      [r0 + r1],       m6
    movh        [r0 + r1 * 2],   m1
    movhps      [r0 + r4],       m1

    RET

cglobal intra_pred_ang8_4, 3,5,7
    cmp         r4m,       byte 32
    cmove       r2,        r3mp
    lea         r3,        [ang_table + 19 * 16]
    mova        m3,        [pw_1024]

    movu        m0,        [r2 + 1]                   ; [16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1]
    palignr     m1,        m0, 1                      ; [x 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2]

    punpckhbw   m2,        m0, m1                     ; [x 16 16 15 15 14 14 13 13 12 12 11 11 10 10 9]
    punpcklbw   m0,        m1                         ; [9 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    palignr     m1,        m2, m0, 2                  ; [10 9 9 8 8 7 7 6 6 5 5 4 4 3 3 2]
    mova        m5,        m1

    pmaddubsw   m4,        m0, [r3 + 2 * 16]          ; [21]
    pmulhrsw    m4,        m3
    pmaddubsw   m1,        [r3 - 9 * 16]              ; [10]
    pmulhrsw    m1,        m3
    packuswb    m4,        m1

    pmaddubsw   m5,        [r3 + 12 * 16]             ; [31]
    pmulhrsw    m5,        m3

    palignr     m6,        m2, m0, 4                  ; [11 10 10 9 9 8 8 7 7 6 6 5 5 4 4 3]

    pmaddubsw   m6,        [r3 + 1 * 16]              ; [ 20]
    pmulhrsw    m6,        m3
    packuswb    m5,        m6

    palignr     m1,        m2, m0, 6                  ; [12 11 11 10 10 9 9 8 8 7 7 6 6 5 5 4]

    pmaddubsw   m6,        m1, [r3 - 10 * 16]         ; [ 9]
    pmulhrsw    m6,        m3

    pmaddubsw   m1,        [r3 + 11 * 16]             ; [30]
    pmulhrsw    m1,        m3
    packuswb    m6,        m1

    palignr     m1,        m2, m0, 8                  ; [13 12 12 11 11 10 10 9 9 8 8 7 7 6 6 5]

    pmaddubsw   m1,        [r3]                       ; [19]
    pmulhrsw    m1,        m3

    palignr     m2,        m0, 10                     ; [14 13 13 12 12 11 11 10 10 9 9 8 8 7 7 8]

    pmaddubsw   m2,        [r3 - 11 * 16]             ; [8]
    pmulhrsw    m2,        m3
    packuswb    m1,        m2
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_5, 3,5,8
    cmp         r4m,       byte 31
    cmove       r2,        r3mp
    lea         r3,        [ang_table + 13 * 16]
    mova        m3,        [pw_1024]

    movu        m0,        [r2 + 1]                   ; [16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1]
    palignr     m1,        m0, 1                      ; [x 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2]

    punpckhbw   m2,        m0, m1                     ; [x 16 16 15 15 14 14 13 13 12 12 11 11 10 10 9]
    punpcklbw   m0,        m1                         ; [9 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    palignr     m1,        m2, m0, 2                  ; [10 9 9 8 8 7 7 6 6 5 5 4 4 3 3 2]
    mova        m5,        m1

    pmaddubsw   m4,        m0, [r3 + 4 * 16]          ; [17]
    pmulhrsw    m4,        m3
    pmaddubsw   m1,        [r3 - 11 * 16]             ; [2]
    pmulhrsw    m1,        m3
    packuswb    m4,        m1

    pmaddubsw   m5,        [r3 + 6 * 16]              ; [19]
    pmulhrsw    m5,        m3

    palignr     m6,        m2, m0, 4                  ; [11 10 10 9 9 8 8 7 7 6 6 5 5 4 4 3]
    mova        m1,        m6

    pmaddubsw   m1,        [r3 - 9 * 16]              ; [4]
    pmulhrsw    m1,        m3
    packuswb    m5,        m1

    pmaddubsw   m6,        [r3 + 8 * 16]              ; [21]
    pmulhrsw    m6,        m3

    palignr     m1,        m2, m0, 6                  ; [12 11 11 10 10 9 9 8 8 7 7 6 6 5 5 4]

    mova        m7,        m1
    pmaddubsw   m7,        [r3 - 7 * 16]              ; [6]
    pmulhrsw    m7,        m3
    packuswb    m6,        m7

    pmaddubsw   m1,        [r3 + 10 * 16]             ; [23]
    pmulhrsw    m1,        m3

    palignr     m2,        m0, 8                      ; [13 12 12 11 11 10 10 9 9 8 8 7 7 8 8 9]

    pmaddubsw   m2,        [r3 - 5 * 16]              ; [8]
    pmulhrsw    m2,        m3
    packuswb    m1,        m2
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_6, 3,5,8
    cmp         r4m,       byte 30
    cmove       r2,        r3mp
    lea         r3,        [ang_table + 14 * 16]
    mova        m7,        [pw_1024]

    movu        m0,        [r2 + 1]                   ; [16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1]
    palignr     m1,        m0, 1                      ; [x 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2]

    punpckhbw   m2,        m0, m1                     ; [x 16 16 15 15 14 14 13 13 12 12 11 11 10 10 9]
    punpcklbw   m0,        m1                         ; [9 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    mova        m1,        m0

    pmaddubsw   m4,        m0, [r3 - 1 * 16]          ; [13]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 + 12 * 16]             ; [26]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1

    palignr     m6,        m2, m0, 2                  ; [10 9 9 8 8 7 7 6 6 5 5 4 4 3 3 2]

    pmaddubsw   m5,        m6, [r3 - 7 * 16]          ; [7]
    pmulhrsw    m5,        m7

    pmaddubsw   m6,        [r3 + 6 * 16]              ; [20]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    palignr     m1,        m2, m0, 4                  ; [11 10 10 9 9 8 8 7 7 6 6 5 5 4 4 3]

    pmaddubsw   m6,        m1, [r3 - 13 * 16]         ; [1]
    pmulhrsw    m6,        m7

    mova        m3,        m1
    pmaddubsw   m3,        [r3]                       ; [14]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3

    pmaddubsw   m1,        [r3 + 13 * 16]             ; [27]
    pmulhrsw    m1,        m7

    palignr     m2,        m0, 6                      ; [12 11 11 10 10 9 9 8 8 7 7 6 6 5 5 4]

    pmaddubsw   m2,        [r3 - 6 * 16]              ; [8]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_7, 3,5,8
    cmp         r4m,       byte 29
    cmove       r2,        r3mp
    lea         r3,        [ang_table + 18 * 16]
    mova        m7,        [pw_1024]

    movu        m0,        [r2 + 1]                   ; [16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1]
    palignr     m1,        m0, 1                      ; [x 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2]

    punpckhbw   m2,        m0, m1                     ; [x 16 16 15 15 14 14 13 13 12 12 11 11 10 10 9]
    punpcklbw   m0,        m1                         ; [9 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]

    pmaddubsw   m4,        m0, [r3 - 9 * 16]          ; [9]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m0, [r3]                   ; [18]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3

    pmaddubsw   m5,        m0, [r3 + 9 * 16]          ; [27]
    pmulhrsw    m5,        m7

    palignr     m1,        m2, m0, 2                  ; [10 9 9 8 8 7 7 6 6 5 5 4 4 3 3 2]

    pmaddubsw   m6,        m1, [r3 - 14 * 16]         ; [4]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    pmaddubsw   m6,        m1, [r3 - 5 * 16]          ; [13]
    pmulhrsw    m6,        m7

    mova        m3,        m1
    pmaddubsw   m3,        [r3 + 4 * 16]              ; [22]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3

    pmaddubsw   m1,        [r3 + 13 * 16]             ; [31]
    pmulhrsw    m1,        m7

    palignr     m2,        m0, 4                      ; [11 10 10 9 9 8 8 7 7 6 6 5 5 4 4 3]

    pmaddubsw   m2,        [r3 - 10 * 16]             ; [8]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_8, 3,5,8
    cmp         r4m,       byte 28
    cmove       r2,        r3mp
    lea         r3,        [ang_table + 17 * 16]
    mova        m7,        [pw_1024]

    movu        m0,        [r2 + 1]                   ; [16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1]
    palignr     m1,        m0, 1                      ; [x 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2]

    punpckhbw   m2,        m0, m1                     ; [x 16 16 15 15 14 14 13 13 12 12 11 11 10 10 9]
    punpcklbw   m0,        m1                         ; [9 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    palignr     m2,        m0, 2                      ; [10 9 9 8 8 7 7 6 6 5 5 4 4 3 3 2]

    pmaddubsw   m4,        m0, [r3 - 12 * 16]         ; [5]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m0, [r3 - 7 * 16]          ; [10]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3

    pmaddubsw   m5,        m0, [r3 - 2 * 16]          ; [15]
    pmulhrsw    m5,        m7

    pmaddubsw   m6,        m0, [r3 + 3 * 16]          ; [20]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    pmaddubsw   m6,        m0, [r3 + 8 * 16]          ; [25]
    pmulhrsw    m6,        m7

    pmaddubsw   m0,        [r3 + 13 * 16]             ; [30]
    pmulhrsw    m0,        m7
    packuswb    m6,        m0

    pmaddubsw   m1,        m2, [r3 - 14 * 16]         ; [3]
    pmulhrsw    m1,        m7

    pmaddubsw   m2,        [r3 - 9 * 16]              ; [8]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_9, 3,5,8
    cmp         r4m,       byte 27
    cmove       r2,        r3mp
    lea         r3,        [ang_table + 9 * 16]
    mova        m7,        [pw_1024]

    movu        m0,        [r2 + 1]                   ; [16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1]
    palignr     m1,        m0, 1                      ; [x 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2]

    punpcklbw   m0,        m1                         ; [9 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]

    pmaddubsw   m4,        m0, [r3 - 7 * 16]          ; [2]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m0, [r3 - 5 * 16]          ; [4]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3

    pmaddubsw   m5,        m0, [r3 - 3 * 16]          ; [6]
    pmulhrsw    m5,        m7

    pmaddubsw   m6,        m0, [r3 - 1 * 16]          ; [8]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    pmaddubsw   m6,        m0, [r3 + 1 * 16]          ; [10]
    pmulhrsw    m6,        m7

    pmaddubsw   m2,        m0, [r3 + 3 * 16]          ; [12]
    pmulhrsw    m2,        m7
    packuswb    m6,        m2

    pmaddubsw   m1,        m0, [r3 + 5 * 16]          ; [14]
    pmulhrsw    m1,        m7

    pmaddubsw   m0,        [r3 + 7 * 16]              ; [16]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_10, 4,5,5
    movh        m0,        [r2 + 1]
    mova        m4,        [pb_unpackbq]
    palignr     m1,        m0, 2
    pshufb      m1,        m4
    palignr     m2,        m0, 4
    pshufb      m2,        m4
    palignr     m3,        m0, 6
    pshufb      m3,        m4
    pshufb      m0,        m4

    lea         r4,             [r1 * 3]
    movhps      [r0 + r1],      m0
    movh        [r0 + r1 * 2],  m1
    movhps      [r0 + r4],      m1
    lea         r2,             [r0 + r1 * 4]
    movh        [r2],           m2
    movhps      [r2 + r1],      m2
    movh        [r2 + r1 * 2],  m3
    movhps      [r2 + r4],      m3

; filter
    cmp         r5m, byte 0
    jz         .quit

    pmovzxbw    m0,        m0
    movu        m1,        [r3]
    palignr     m2,        m1, 1
    pshufb      m1,        m4
    pmovzxbw    m1,        m1
    pmovzxbw    m2,        m2
    psubw       m2,        m1
    psraw       m2,        1
    paddw       m0,        m2
    packuswb    m0,        m0

.quit:
    movh        [r0],      m0
    RET

cglobal intra_pred_ang8_26, 4,5,3
    movh        m0,        [r3 + 1]

    lea         r4,             [r1 * 3]
    movh        [r0],           m0
    movh        [r0 + r1],      m0
    movh        [r0 + r1 * 2],  m0
    movh        [r0 + r4],      m0
    lea         r3,             [r0 + r1 * 4]
    movh        [r3],           m0
    movh        [r3 + r1],      m0
    movh        [r3 + r1 * 2],  m0
    movh        [r3 + r4],      m0

; filter
    cmp         r5m, byte 0
    jz         .quit

    pshufb      m0,        [pb_unpackbq]
    pmovzxbw    m0,        m0
    movu        m1,        [r2]
    palignr     m2,        m1, 1
    pshufb      m1,        [pb_unpackbq]
    pmovzxbw    m1,        m1
    pmovzxbw    m2,        m2
    psubw       m2,        m1
    psraw       m2,        1
    paddw       m0,        m2
    packuswb    m0,        m0
    pextrb      [r0],          m0, 0
    pextrb      [r0 + r1],     m0, 1
    pextrb      [r0 + r1 * 2], m0, 2
    pextrb      [r0 + r4],     m0, 3
    pextrb      [r3],          m0, 4
    pextrb      [r3 + r1],     m0, 5
    pextrb      [r3 + r1 * 2], m0, 6
    pextrb      [r3 + r4],     m0, 7

.quit:
    RET

cglobal intra_pred_ang8_11, 3,5,8
    cmp         r4m,       byte 25
    cmove       r2,        r3mp
    lea         r3,        [ang_table + 23 * 16]
    mova        m7,        [pw_1024]

    movu        m0,        [r2]                       ; [15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    palignr     m1,        m0, 1                      ; [x 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1]

    punpcklbw   m0,        m1                         ; [8 7 7 6 6 5 5 4 4 3 3 2 2 1 1 0]

    pmaddubsw   m4,        m0, [r3 + 7 * 16]          ; [30]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m0, [r3 + 5 * 16]          ; [28]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3

    pmaddubsw   m5,        m0, [r3 + 3 * 16]          ; [26]
    pmulhrsw    m5,        m7

    pmaddubsw   m6,        m0, [r3 + 1 * 16]          ; [24]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    pmaddubsw   m6,        m0, [r3 - 1 * 16]          ; [22]
    pmulhrsw    m6,        m7

    pmaddubsw   m2,        m0, [r3 - 3 * 16]          ; [20]
    pmulhrsw    m2,        m7
    packuswb    m6,        m2

    pmaddubsw   m1,        m0, [r3 - 5 * 16]          ; [18]
    pmulhrsw    m1,        m7

    pmaddubsw   m0,        [r3 - 7 * 16]              ; [16]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_12, 4,5,8
    cmp         r4m,       byte 24
    jnz         .skip
    xchg        r2,        r3

.skip:
    lea         r4,        [ang_table + 16 * 16]
    mova        m7,        [pw_1024]

    movu        m1,        [r2]                       ; [15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    pslldq      m0,        m1, 1                      ; [14 13 12 11 10 9 8 7 6 5 4 3 2 1 0 a]
    pinsrb      m0,        [r3 + 6], 0
    punpckhbw   m2,        m0, m1                     ; [15 14 14 13 13 12 12 11 11 10 10 9 9 8 8 7]
    punpcklbw   m0,        m1                         ; [7 6 6 5 5 4 4 3 3 2 2 1 1 0 0 a]
    palignr     m2,        m0, 2                      ; [8 7 7 6 6 5 5 4 4 3 3 2 2 1 1 0]

    pmaddubsw   m4,        m2, [r4 + 11 * 16]         ; [27]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m2, [r4 + 6 * 16]          ; [22]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3

    pmaddubsw   m5,        m2, [r4 + 1 * 16]          ; [17]
    pmulhrsw    m5,        m7

    pmaddubsw   m6,        m2, [r4 - 4 * 16]          ; [12]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    pmaddubsw   m6,        m2, [r4 - 9 * 16]          ; [7]
    pmulhrsw    m6,        m7

    pmaddubsw   m2,        [r4 - 14 * 16]             ; [2]
    pmulhrsw    m2,        m7
    packuswb    m6,        m2

    pmaddubsw   m1,        m0, [r4 + 13 * 16]         ; [29]
    pmulhrsw    m1,        m7

    pmaddubsw   m0,        [r4 + 8 * 16]              ; [24]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_13, 4,5,8
    cmp         r4m,       byte 23
    jnz         .skip
    xchg        r2,        r3

.skip:
    lea         r4,        [ang_table + 14 * 16]
    mova        m7,        [pw_1024]

    movu        m1,        [r2]                       ; [15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    pslldq      m1,        1                          ; [14 13 12 11 10 9 8 7 6 5 4 3 2 1 0 a]
    pinsrb      m1,        [r3 + 4], 0
    pslldq      m0,        m1, 1                      ; [13 12 11 10 9 8 7 6 5 4 3 2 1 0 a b]
    pinsrb      m0,        [r3 + 7], 0
    punpckhbw   m5,        m0, m1                     ; [14 13 13 12 12 11 11 10 10 9 9 8 8 7 7 6]
    punpcklbw   m0,        m1                         ; [6 5 5 4 4 3 3 2 2 1 1 0 0 a a b]
    palignr     m1,        m5, m0, 2                  ; [7 6 6 5 5 4 4 3 3 2 2 1 1 0 0 a]
    palignr     m5,        m0, 4                      ; [8 7 7 6 6 5 5 4 4 3 3 2 2 1 1 0]

    pmaddubsw   m4,        m5, [r4 + 9 * 16]          ; [23]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m5, [r4]                   ; [14]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3

    pmaddubsw   m5,        [r4 - 9 * 16]              ; [5]
    pmulhrsw    m5,        m7

    pmaddubsw   m6,        m1, [r4 + 14 * 16]         ; [28]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    pmaddubsw   m6,        m1, [r4 + 5 * 16]          ; [19]
    pmulhrsw    m6,        m7

    pmaddubsw   m2,        m1, [r4 - 4 * 16]          ; [10]
    pmulhrsw    m2,        m7
    packuswb    m6,        m2

    pmaddubsw   m1,        [r4 - 13 * 16]             ; [1]
    pmulhrsw    m1,        m7

    pmaddubsw   m0,        [r4 + 10 * 16]             ; [24]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_14, 4,5,7
    cmp         r4m,       byte 22
    jnz         .skip
    xchg        r2,        r3

.skip:
    lea         r4,        [ang_table + 18 * 16]
    mova        m3,        [pw_1024]

    movu        m1,        [r2]                       ; [15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    pslldq      m1,        2                          ; [13 12 11 10 9 8 7 6 5 4 3 2 1 0 a b]
    pinsrb      m1,        [r3 + 2], 1
    pinsrb      m1,        [r3 + 5], 0
    pslldq      m0,        m1, 1                      ; [12 11 10 9 8 7 6 5 4 3 2 1 0 a b c]
    pinsrb      m0,        [r3 + 7], 0
    punpckhbw   m2,        m0, m1                     ; [13 12 12 11 11 10 10 9 9 8 8 7 7 6 6 5]
    punpcklbw   m0,        m1                         ; [5 4 4 3 3 2 2 1 1 0 0 a a b b c]
    palignr     m1,        m2, m0, 2                  ; [6 5 5 4 4 3 3 2 2 1 1 0 0 a a b]
    palignr     m6,        m2, m0, 4                  ; [7 6 6 5 5 4 4 3 3 2 2 1 1 0 0 a]
    palignr     m2,        m0, 6                      ; [8 7 7 6 6 5 5 4 4 3 3 2 2 1 1 0]

    pmaddubsw   m4,        m2, [r4 + 1 * 16]          ; [19]
    pmulhrsw    m4,        m3
    pmaddubsw   m2,        [r4 - 12 * 16]             ; [6]
    pmulhrsw    m2,        m3
    packuswb    m4,        m2

    pmaddubsw   m5,        m6, [r4 + 7 * 16]          ; [25]
    pmulhrsw    m5,        m3

    pmaddubsw   m6,        [r4 - 6 * 16]              ; [12]
    pmulhrsw    m6,        m3
    packuswb    m5,        m6

    pmaddubsw   m6,        m1, [r4 + 13 * 16]         ; [31]
    pmulhrsw    m6,        m3

    pmaddubsw   m2,        m1, [r4]                   ; [18]
    pmulhrsw    m2,        m3
    packuswb    m6,        m2

    pmaddubsw   m1,        [r4 - 13 * 16]             ; [5]
    pmulhrsw    m1,        m3

    pmaddubsw   m0,        [r4 + 6 * 16]              ; [24]
    pmulhrsw    m0,        m3
    packuswb    m1,        m0
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_15, 4,5,7
    cmp         r4m,       byte 21
    jnz         .skip
    xchg        r2,        r3

.skip:
    lea         r4,        [ang_table + 20 * 16]
    mova        m3,        [pw_1024]

    movu        m1,        [r2]                       ; [15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    movu        m2,        [r3]
    pshufb      m2,        [c_mode16_15]
    palignr     m1,        m2, 13                     ; [12 11 10 9 8 7 6 5 4 3 2 1 0 a b c]
    pslldq      m0,        m1, 1                      ; [11 10 9 8 7 6 5 4 3 2 1 0 a b c d]
    pinsrb      m0,        [r3 + 8], 0
    punpckhbw   m4,        m0, m1                     ; [12 11 11 10 10 9 9 8 8 7 7 6 6 5 5 4]
    punpcklbw   m0,        m1                         ; [4 3 3 2 2 1 1 0 0 a a b b c c d]
    palignr     m1,        m4, m0, 2                  ; [5 4 4 3 3 2 2 1 1 0 0 a a b b c]
    palignr     m6,        m4, m0, 4                  ; [6 5 5 4 4 3 3 2 2 1 1 0 0 a a b]
    palignr     m5,        m4, m0, 6                  ; [7 6 6 5 5 4 4 3 3 2 2 1 1 0 0 a]
    palignr     m4,        m0, 8                      ; [8 7 7 6 6 5 5 4 4 3 3 2 2 1 1 0]

    pmaddubsw   m4,        [r4 - 5 * 16]              ; [15]
    pmulhrsw    m4,        m3

    pmaddubsw   m2,        m5, [r4 + 10 * 16]         ; [30]
    pmulhrsw    m2,        m3
    packuswb    m4,        m2

    pmaddubsw   m5,        [r4 - 7 * 16]              ; [13]
    pmulhrsw    m5,        m3

    pmaddubsw   m2,        m6, [r4 + 8 * 16]          ; [28]
    pmulhrsw    m2,        m3
    packuswb    m5,        m2

    pmaddubsw   m6,        [r4 - 9 * 16]              ; [11]
    pmulhrsw    m6,        m3

    pmaddubsw   m2,        m1, [r4 + 6 * 16]          ; [26]
    pmulhrsw    m2,        m3
    packuswb    m6,        m2

    pmaddubsw   m1,        [r4 - 11 * 16]             ; [9]
    pmulhrsw    m1,        m3

    pmaddubsw   m0,        [r4 + 4 * 16]              ; [24]
    pmulhrsw    m0,        m3
    packuswb    m1,        m0
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_16, 4,5,8
    cmp         r4m,       byte 20
    jnz         .skip
    xchg        r2,        r3

.skip:
    lea         r4,        [ang_table + 13 * 16]
    mova        m7,        [pw_1024]

    movu        m1,        [r2]                       ; [15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    movu        m2,        [r3]
    pshufb      m2,        [c_mode16_16]
    palignr     m1,        m2, 12                     ; [11 10 9 8 7 6 5 4 3 2 1 0 a b c d]
    pslldq      m0,        m1, 1                      ; [10 9 8 7 6 5 4 3 2 1 0 a b c d e]
    pinsrb      m0,        [r3 + 8], 0
    punpckhbw   m4,        m0, m1                     ; [11 10 10 9 9 8 8 7 7 6 6 5 5 4 4 3]
    punpcklbw   m0,        m1                         ; [3 2 2 1 1 0 0 a a b b c c d d e]
    palignr     m1,        m4, m0, 2                  ; [4 3 3 2 2 1 1 0 0 a a b b c c d]
    palignr     m6,        m4, m0, 4                  ; [5 4 4 3 3 2 2 1 1 0 0 a a b b c]
    palignr     m2,        m4, m0, 6                  ; [6 5 5 4 4 3 3 2 2 1 1 0 0 a a b]
    palignr     m5,        m4, m0, 8                  ; [7 6 6 5 5 4 4 3 3 2 2 1 1 0 0 a]
    palignr     m4,        m0, 10                     ; [8 7 7 6 6 5 5 4 4 3 3 2 2 1 1 0]

    pmaddubsw   m4,        [r4 - 2 * 16]              ; [11]
    pmulhrsw    m4,        m7

    pmaddubsw   m3,        m5, [r4 + 9 * 16]          ; [22]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3

    pmaddubsw   m5,        [r4 - 12 * 16]             ; [1]
    pmulhrsw    m5,        m7

    pmaddubsw   m2,        [r4 - 1 * 16]              ; [12]
    pmulhrsw    m2,        m7
    packuswb    m5,        m2

    mova        m2,        m6
    pmaddubsw   m6,        [r4 + 10 * 16]             ; [23]
    pmulhrsw    m6,        m7

    pmaddubsw   m2,        [r4 - 11 * 16]             ; [2]
    pmulhrsw    m2,        m7
    packuswb    m6,        m2

    pmaddubsw   m1,        [r4]                       ; [13]
    pmulhrsw    m1,        m7

    pmaddubsw   m0,        [r4 + 11 * 16]             ; [24]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_17, 4,5,7
    cmp         r4m,       byte 19
    jnz         .skip
    xchg        r2,        r3

.skip:
    lea         r4,        [ang_table + 17 * 16]
    mova        m3,        [pw_1024]

    movu        m2,        [r2]                       ; [15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    movu        m1,        [r3]
    pshufb      m1,        [c_mode16_17]
    palignr     m2,        m1, 11                     ; [10 9 8 7 6 5 4 3 2 1 0 a b c d e]
    pslldq      m0,        m2, 1                      ; [9 8 7 6 5 4 3 2 1 0 a b c d e f]
    pinsrb      m0,        [r3 + 7], 0
    punpckhbw   m1,        m0, m2                     ; [10 9 9 8 8 7 7 6 6 5 5 4 4 3 3 2]
    punpcklbw   m0,        m2                         ; [2 1 1 0 0 a a b b c c d d e e f]

    palignr     m5,        m1, m0, 8                  ; [6 5 5 4 4 3 3 2 2 1 1 0 0 a a b]
    palignr     m2,        m1, m0, 10                 ; [7 6 6 5 5 4 4 3 3 2 2 1 1 0 0 a]
    palignr     m4,        m1, m0, 12                 ; [8 7 7 6 6 5 5 4 4 3 3 2 2 1 1 0]

    pmaddubsw   m4,        [r4 - 11 * 16]             ; [6]
    pmulhrsw    m4,        m3

    pmaddubsw   m2,        [r4 - 5 * 16]              ; [12]
    pmulhrsw    m2,        m3
    packuswb    m4,        m2

    pmaddubsw   m5,        [r4 + 1 * 16]              ; [18]
    pmulhrsw    m5,        m3

    palignr     m2,        m1, m0, 6                  ; [5 4 4 3 3 2 2 1 1 0 0 a a b b c]
    pmaddubsw   m2,        [r4 + 7 * 16]              ; [24]
    pmulhrsw    m2,        m3
    packuswb    m5,        m2

    palignr     m6,        m1, m0, 4                  ; [4 3 3 2 2 1 1 0 0 a a b b c c d]
    mova        m2,        m6
    pmaddubsw   m6,        [r4 + 13 * 16]             ; [30]
    pmulhrsw    m6,        m3

    pmaddubsw   m2,        [r4 - 13 * 16]             ; [4]
    pmulhrsw    m2,        m3
    packuswb    m6,        m2

    palignr     m1,        m0, 2                      ; [3 2 2 1 1 0 0 a a b b c c d d e]
    pmaddubsw   m1,        [r4 - 7 * 16]              ; [10]
    pmulhrsw    m1,        m3

    pmaddubsw   m0,        [r4 - 1 * 16]              ; [16]
    pmulhrsw    m0,        m3
    packuswb    m1,        m0
    jmp         mangle(private_prefix %+ _ %+ intra_pred_ang8_3 %+ SUFFIX %+ .transpose8x8)

cglobal intra_pred_ang8_18, 4,4,1
    movu        m0, [r2]
    pshufb      m0, [pb_swap8]
    movhps      m0, [r3 + 1]
    lea         r2, [r0 + r1 * 4]
    lea         r3, [r1 * 3]
    movh        [r2 + r3], m0
    psrldq      m0, 1
    movh        [r2 + r1 * 2], m0
    psrldq      m0, 1
    movh        [r2 + r1], m0
    psrldq      m0, 1
    movh        [r2], m0
    psrldq      m0, 1
    movh        [r0 + r3], m0
    psrldq      m0, 1
    movh        [r0 + r1 * 2], m0
    psrldq      m0, 1
    movh        [r0 + r1], m0
    psrldq      m0, 1
    movh        [r0], m0
    RET


;-----------------------------------------------------------------------------
; void intraPredAng16(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal intra_pred_ang16_2, 3,3,3
    cmp             r4m, byte 34
    cmove           r2, r3mp
    movu            m0, [r2 + 2]
    movu            m1, [r2 + 18]
    movu            [r0], m0
    palignr         m2, m1, m0, 1
    movu            [r0 + r1], m2
    lea             r0, [r0 + r1 * 2]
    palignr         m2, m1, m0, 2
    movu            [r0], m2
    palignr         m2, m1, m0, 3
    movu            [r0 + r1], m2
    lea             r0, [r0 + r1 * 2]
    palignr         m2, m1, m0, 4
    movu            [r0], m2
    palignr         m2, m1, m0, 5
    movu            [r0 + r1], m2
    lea             r0, [r0 + r1 * 2]
    palignr         m2, m1, m0, 6
    movu            [r0], m2
    palignr         m2, m1, m0, 7
    movu            [r0 + r1], m2
    lea             r0, [r0 + r1 * 2]
    palignr         m2, m1, m0, 8
    movu            [r0], m2
    palignr         m2, m1, m0, 9
    movu            [r0 + r1], m2
    lea             r0, [r0 + r1 * 2]
    palignr         m2, m1, m0, 10
    movu            [r0], m2
    palignr         m2, m1, m0, 11
    movu            [r0 + r1], m2
    lea             r0, [r0 + r1 * 2]
    palignr         m2, m1, m0, 12
    movu            [r0], m2
    palignr         m2, m1, m0, 13
    movu            [r0 + r1], m2
    lea             r0, [r0 + r1 * 2]
    palignr         m2, m1, m0, 14
    movu            [r0], m2
    palignr         m2, m1, m0, 15
    movu            [r0 + r1], m2
    RET

%macro TRANSPOSE_STORE_8x8 6
  %if %2 == 1
    ; transpose 8x8 and then store, used by angle BLOCK_16x16 and BLOCK_32x32
    punpckhbw   m0,        %3, %4
    punpcklbw   %3,        %4
    punpckhbw   %4,        %3, m0
    punpcklbw   %3,        m0

    punpckhbw   m0,        %5, m1
    punpcklbw   %5,        %6
    punpckhbw   %6,        %5, m0
    punpcklbw   %5,        m0

    punpckhdq   m0,        %3, %5
    punpckldq   %3,        %5
    punpckldq   %5,        %4, %6
    punpckhdq   %4,        %6

    movh        [r0 +       + %1 * 8], %3
    movhps      [r0 +  r1   + %1 * 8], %3
    movh        [r0 +  r1*2 + %1 * 8], m0
    movhps      [r0 +  r5   + %1 * 8], m0
    movh        [r6         + %1 * 8], %5
    movhps      [r6 +  r1   + %1 * 8], %5
    movh        [r6 +  r1*2 + %1 * 8], %4
    movhps      [r6 +  r5   + %1 * 8], %4
  %else
    ; store 8x8, used by angle BLOCK_16x16 and BLOCK_32x32
    movh        [r0         ], %3
    movhps      [r0 + r1    ], %3
    movh        [r0 + r1 * 2], %4
    movhps      [r0 + r5    ], %4
    lea         r0, [r0 + r1 * 4]
    movh        [r0         ], %5
    movhps      [r0 + r1    ], %5
    movh        [r0 + r1 * 2], %6
    movhps      [r0 + r5    ], %6
    lea         r0, [r0 + r1 * 4]
  %endif
%endmacro

INIT_XMM sse4
cglobal intra_pred_ang16_3, 3,7,8

    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       2
    lea         r5,        [r1 * 3]                   ; r5 -> 3 * stride
    lea         r6,        [r0 + r1 * 4]              ; r6 -> 4 * stride
    mova        m7,        [pw_1024]

.loop:
    movu        m0,        [r2 + 1]
    palignr     m1,        m0, 1

    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m1,        m2, m0, 2

    pmaddubsw   m4,        m0, [r3 + 10 * 16]         ; [26]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 + 4 * 16]              ; [20]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1

    palignr     m5,        m2, m0, 4

    pmaddubsw   m5,        [r3 - 2 * 16]              ; [14]
    pmulhrsw    m5,        m7

    palignr     m6,        m2, m0, 6

    pmaddubsw   m6,        [r3 - 8 * 16]              ; [ 8]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    palignr     m1,        m2, m0, 8

    pmaddubsw   m6,        m1, [r3 - 14 * 16]         ; [ 2]
    pmulhrsw    m6,        m7

    pmaddubsw   m1,        [r3 + 12 * 16]             ; [28]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1

    palignr     m1,        m2, m0, 10

    pmaddubsw   m1,        [r3 + 6 * 16]              ; [22]
    pmulhrsw    m1,        m7

    palignr     m2,        m0, 12

    pmaddubsw   m2,        [r3]                       ; [16]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 0, 1, m4, m5, m6, m1

    movu        m0,        [r2 + 8]
    palignr     m1,        m0, 1

    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m5,        m2, m0, 2

    pmaddubsw   m4,        m0, [r3 - 6 * 16]          ; [10]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        m5, [r3 - 12 * 16]         ; [04]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1

    pmaddubsw   m5,        [r3 + 14 * 16]             ; [30]
    pmulhrsw    m5,        m7

    palignr     m6,        m2, m0, 4

    pmaddubsw   m6,        [r3 + 8 * 16]              ; [24]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    palignr     m1,        m2, m0, 6

    pmaddubsw   m6,        m1, [r3 + 2 * 16]          ; [18]
    pmulhrsw    m6,        m7

    palignr     m1,        m2, m0, 8

    pmaddubsw   m1,        [r3 - 4 * 16]              ; [12]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1

    palignr     m1,        m2, m0, 10

    pmaddubsw   m1,        [r3 - 10 * 16]             ; [06]
    pmulhrsw    m1,        m7
    packuswb    m1,        m1

    movhps      m1,        [r2 + 14]                  ; [00]

    TRANSPOSE_STORE_8x8 1, 1, m4, m5, m6, m1

    lea         r0,        [r6 + r1 * 4]
    lea         r6,        [r6 + r1 * 8]
    add         r2,        8
    dec         r4
    jnz        .loop

    RET

INIT_XMM sse4
cglobal intra_pred_ang16_33, 3,7,8
    mov         r2,        r3mp
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       2
    lea         r5,        [r1 * 3]
    mov         r6,        r0
    mova        m7,        [pw_1024]

.loop:
    movu        m0,        [r2 + 1]
    palignr     m1,        m0, 1

    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m1,        m2, m0, 2

    pmaddubsw   m4,        m0, [r3 + 10 * 16]         ; [26]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 + 4 * 16]              ; [20]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1

    palignr     m5,        m2, m0, 4

    pmaddubsw   m5,        [r3 - 2 * 16]              ; [14]
    pmulhrsw    m5,        m7

    palignr     m6,        m2, m0, 6

    pmaddubsw   m6,        [r3 - 8 * 16]              ; [ 8]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    palignr     m1,        m2, m0, 8

    pmaddubsw   m6,        m1, [r3 - 14 * 16]         ; [ 2]
    pmulhrsw    m6,        m7

    pmaddubsw   m1,        [r3 + 12 * 16]             ; [28]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1

    palignr     m1,        m2, m0, 10

    pmaddubsw   m1,        [r3 + 6 * 16]              ; [22]
    pmulhrsw    m1,        m7

    palignr     m2,        m0, 12

    pmaddubsw   m2,        [r3]                       ; [16]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 0, 0, m4, m5, m6, m1

    movu        m0,        [r2 + 8]
    palignr     m1,        m0, 1

    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m5,        m2, m0, 2

    pmaddubsw   m4,        m0, [r3 - 6 * 16]          ; [10]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        m5, [r3 - 12 * 16]         ; [04]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1

    pmaddubsw   m5,        [r3 + 14 * 16]             ; [30]
    pmulhrsw    m5,        m7

    palignr     m6,        m2, m0, 4

    pmaddubsw   m6,        [r3 + 8 * 16]              ; [24]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    palignr     m1,        m2, m0, 6

    pmaddubsw   m6,        m1, [r3 + 2 * 16]          ; [18]
    pmulhrsw    m6,        m7

    palignr     m1,        m2, m0, 8

    pmaddubsw   m1,        [r3 - 4 * 16]              ; [12]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1

    palignr     m1,        m2, m0, 10

    pmaddubsw   m1,        [r3 - 10 * 16]             ; [06]
    pmulhrsw    m1,        m7
    packuswb    m1,        m1

    movh        m2,        [r2 + 14]                  ; [00]

    movh        [r0         ], m4
    movhps      [r0 + r1    ], m4
    movh        [r0 + r1 * 2], m5
    movhps      [r0 + r5    ], m5
    lea         r0, [r0 + r1 * 4]
    movh        [r0         ], m6
    movhps      [r0 + r1    ], m6
    movh        [r0 + r1 * 2], m1
    movh        [r0 + r5    ], m2

    lea         r0,        [r6 + 8]
    add         r2,        8
    dec         r4
    jnz        .loop

    RET

INIT_XMM sse4
cglobal intra_pred_ang16_4, 3,7,8

    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       2
    lea         r5,        [r1 * 3]                   ; r5 -> 3 * stride
    lea         r6,        [r0 + r1 * 4]              ; r6 -> 4 * stride
    mova        m7,        [pw_1024]

.loop:
    movu        m0,        [r2 + 1]
    palignr     m1,        m0, 1

    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m1,        m2, m0, 2
    mova        m5,        m1

    pmaddubsw   m4,        m0, [r3 + 5 * 16]          ; [21]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 - 6 * 16]              ; [10]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1

    pmaddubsw   m5,        [r3 + 15 * 16]             ; [31]
    pmulhrsw    m5,        m7

    palignr     m6,        m2, m0, 4

    pmaddubsw   m6,        [r3 + 4 * 16]              ; [ 20]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    palignr     m1,        m2, m0, 6

    pmaddubsw   m6,        m1, [r3 - 7 * 16]          ; [ 9]
    pmulhrsw    m6,        m7

    pmaddubsw   m1,        [r3 + 14 * 16]             ; [30]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1

    palignr     m1,        m2, m0, 8

    pmaddubsw   m1,        [r3 + 3 * 16]              ; [19]
    pmulhrsw    m1,        m7

    palignr     m2,        m0, 10

    pmaddubsw   m3,        m2, [r3 - 8 * 16]          ; [8]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3

    TRANSPOSE_STORE_8x8 0, 1, m4, m5, m6, m1

    pmaddubsw   m4,        m2, [r3 + 13 * 16]         ; [29]
    pmulhrsw    m4,        m7

    movu        m0,        [r2 + 6]
    palignr     m1,        m0, 1

    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m1,        m2, m0, 2

    pmaddubsw   m1,        [r3 +  2 * 16]             ; [18]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1

    palignr     m5,        m2, m0, 4
    mova        m6,        m5

    pmaddubsw   m5,        [r3 - 9 * 16]              ; [07]
    pmulhrsw    m5,        m7

    pmaddubsw   m6,        [r3 + 12 * 16]             ; [28]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    palignr     m6,        m2, m0, 6

    pmaddubsw   m6,        [r3 +      16]             ; [17]
    pmulhrsw    m6,        m7

    palignr     m1,        m2, m0, 8
    palignr     m2,        m0, 10

    pmaddubsw   m3,        m1, [r3 - 10 * 16]         ; [06]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3

    pmaddubsw   m1,        [r3 + 11 * 16]             ; [27]
    pmulhrsw    m1,        m7

    pmaddubsw   m2,        [r3]                       ; [16]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 1, 1, m4, m5, m6, m1

    lea         r0,        [r6 + r1 * 4]
    lea         r6,        [r6 + r1 * 8]
    add         r2,        8
    dec         r4
    jnz        .loop

    RET

INIT_XMM sse4
cglobal intra_pred_ang16_32, 3,7,8
    mov         r2,        r3mp
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       2
    lea         r5,        [r1 * 3]                   ; r5 -> 3 * stride
    mov         r6,        r0
    mova        m7,        [pw_1024]

.loop:
    movu        m0,        [r2 + 1]
    palignr     m1,        m0, 1

    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m1,        m2, m0, 2
    mova        m5,        m1


    pmaddubsw   m4,        m0, [r3 + 5 * 16]          ; [21]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 - 6 * 16]              ; [10]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1

    pmaddubsw   m5,        [r3 + 15 * 16]             ; [31]
    pmulhrsw    m5,        m7

    palignr     m6,        m2, m0, 4

    pmaddubsw   m6,        [r3 + 4 * 16]              ; [ 20]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    palignr     m1,        m2, m0, 6

    pmaddubsw   m6,        m1, [r3 - 7 * 16]          ; [ 9]
    pmulhrsw    m6,        m7

    pmaddubsw   m1,        [r3 + 14 * 16]             ; [30]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1

    palignr     m1,        m2, m0, 8

    pmaddubsw   m1,        [r3 + 3 * 16]              ; [19]
    pmulhrsw    m1,        m7

    palignr     m2,        m0, 10

    pmaddubsw   m3,        m2, [r3 - 8 * 16]          ; [8]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3

    TRANSPOSE_STORE_8x8 0, 0, m4, m5, m6, m1

    pmaddubsw   m4,        m2, [r3 + 13 * 16]         ; [29]
    pmulhrsw    m4,        m7

    movu        m0,        [r2 + 6]
    palignr     m1,        m0, 1

    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m1,        m2, m0, 2

    pmaddubsw   m1,        [r3 +  2 * 16]             ; [18]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1

    palignr     m5,        m2, m0, 4
    mova        m6,        m5

    pmaddubsw   m5,        [r3 - 9 * 16]              ; [07]
    pmulhrsw    m5,        m7

    pmaddubsw   m6,        [r3 + 12 * 16]             ; [28]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6

    palignr     m6,        m2, m0, 6

    pmaddubsw   m6,        [r3 +      16]             ; [17]
    pmulhrsw    m6,        m7

    palignr     m1,        m2, m0, 8
    palignr     m2,        m0, 10

    pmaddubsw   m3,        m1, [r3 - 10 * 16]         ; [06]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3

    pmaddubsw   m1,        [r3 + 11 * 16]             ; [27]
    pmulhrsw    m1,        m7

    pmaddubsw   m2,        [r3]                       ; [16]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 1, 0, m4, m5, m6, m1

    lea         r0,        [r6 + 8]
    add         r2,        8
    dec         r4
    jnz        .loop

    RET

;---------------------------------------------------------------------------------------------------------------
; void intraPredAng32(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;---------------------------------------------------------------------------------------------------------------
INIT_XMM ssse3
cglobal intra_pred_ang32_2, 3,4,4
    cmp             r4m, byte 34
    cmove           r2, r3mp
    movu            m0, [r2 + 2]
    movu            m1, [r2 + 18]
    movu            m3, [r2 + 34]

    lea             r3, [r1 * 3]

    movu            [r0], m0
    movu            [r0 + 16], m1
    palignr         m2, m1, m0, 1
    movu            [r0 + r1], m2
    palignr         m2, m3, m1, 1
    movu            [r0 + r1 + 16], m2
    palignr         m2, m1, m0, 2
    movu            [r0 + r1 * 2], m2
    palignr         m2, m3, m1, 2
    movu            [r0 + r1 * 2 + 16], m2
    palignr         m2, m1, m0, 3
    movu            [r0 + r3], m2
    palignr         m2, m3, m1, 3
    movu            [r0 + r3 + 16], m2

    lea             r0, [r0 + r1 * 4]

    palignr         m2, m1, m0, 4
    movu            [r0], m2
    palignr         m2, m3, m1, 4
    movu            [r0 + 16], m2
    palignr         m2, m1, m0, 5
    movu            [r0 + r1], m2
    palignr         m2, m3, m1, 5
    movu            [r0 + r1 + 16], m2
    palignr         m2, m1, m0, 6
    movu            [r0 + r1 * 2], m2
    palignr         m2, m3, m1, 6
    movu            [r0 + r1 * 2 + 16], m2
    palignr         m2, m1, m0, 7
    movu            [r0 + r3], m2
    palignr         m2, m3, m1, 7
    movu            [r0 + r3 + 16], m2

    lea             r0, [r0 + r1 * 4]

    palignr         m2, m1, m0, 8
    movu            [r0], m2
    palignr         m2, m3, m1, 8
    movu            [r0 + 16], m2
    palignr         m2, m1, m0, 9
    movu            [r0 + r1], m2
    palignr         m2, m3, m1, 9
    movu            [r0 + r1 + 16], m2
    palignr         m2, m1, m0, 10
    movu            [r0 + r1 * 2], m2
    palignr         m2, m3, m1, 10
    movu            [r0 + r1 * 2 + 16], m2
    palignr         m2, m1, m0, 11
    movu            [r0 + r3], m2
    palignr         m2, m3, m1, 11
    movu            [r0 + r3 + 16], m2

    lea             r0, [r0 + r1 * 4]

    palignr         m2, m1, m0, 12
    movu            [r0], m2
    palignr         m2, m3, m1, 12
    movu            [r0 + 16], m2
    palignr         m2, m1, m0, 13
    movu            [r0 + r1], m2
    palignr         m2, m3, m1, 13
    movu            [r0 + r1 + 16], m2
    palignr         m2, m1, m0, 14
    movu            [r0 + r1 * 2], m2
    palignr         m2, m3, m1, 14
    movu            [r0 + r1 * 2 + 16], m2
    palignr         m2, m1, m0, 15
    movu            [r0 + r3], m2
    palignr         m2, m3, m1, 15
    movu            [r0 + r3 + 16], m2

    lea             r0, [r0 + r1 * 4]

    movu            [r0], m1
    movu            m0, [r2 + 50]
    movu            [r0 + 16], m3
    palignr         m2, m3, m1, 1
    movu            [r0 + r1], m2
    palignr         m2, m0, m3, 1
    movu            [r0 + r1 + 16], m2
    palignr         m2, m3, m1, 2
    movu            [r0 + r1 * 2], m2
    palignr         m2, m0, m3, 2
    movu            [r0 + r1 * 2 + 16], m2
    palignr         m2, m3, m1, 3
    movu            [r0 + r3], m2
    palignr         m2, m0, m3, 3
    movu            [r0 + r3 + 16], m2

    lea             r0, [r0 + r1 * 4]

    palignr         m2, m3, m1, 4
    movu            [r0], m2
    palignr         m2, m0, m3, 4
    movu            [r0 + 16], m2
    palignr         m2, m3, m1, 5
    movu            [r0 + r1], m2
    palignr         m2, m0, m3, 5
    movu            [r0 + r1 + 16], m2
    palignr         m2, m3, m1, 6
    movu            [r0 + r1 * 2], m2
    palignr         m2, m0, m3, 6
    movu            [r0 + r1 * 2 + 16], m2
    palignr         m2, m3, m1, 7
    movu            [r0 + r3], m2
    palignr         m2, m0, m3, 7
    movu            [r0 + r3 + 16], m2

    lea             r0, [r0 + r1 * 4]

    palignr         m2, m3, m1, 8
    movu            [r0], m2
    palignr         m2, m0, m3, 8
    movu            [r0 + 16], m2
    palignr         m2, m3, m1, 9
    movu            [r0 + r1], m2
    palignr         m2, m0, m3, 9
    movu            [r0 + r1 + 16], m2
    palignr         m2, m3, m1, 10
    movu            [r0 + r1 * 2], m2
    palignr         m2, m0, m3, 10
    movu            [r0 + r1 * 2 + 16], m2
    palignr         m2, m3, m1, 11
    movu            [r0 + r3], m2
    palignr         m2, m0, m3, 11
    movu            [r0 + r3 + 16], m2

    lea             r0, [r0 + r1 * 4]

    palignr         m2, m3, m1, 12
    movu            [r0], m2
    palignr         m2, m0, m3, 12
    movu            [r0 + 16], m2
    palignr         m2, m3, m1, 13
    movu            [r0 + r1], m2
    palignr         m2, m0, m3, 13
    movu            [r0 + r1 + 16], m2
    palignr         m2, m3, m1, 14
    movu            [r0 + r1 * 2], m2
    palignr         m2, m0, m3, 14
    movu            [r0 + r1 * 2 + 16], m2
    palignr         m2, m3, m1, 15
    movu            [r0 + r3], m2
    palignr         m2, m0, m3, 15
    movu            [r0 + r3 + 16], m2
    RET

; Process Intra32x32, input 8x8 in [m0, m1, m2, m3, m4, m5, m6, m7], output 8x8
%macro PROC32_8x8 10  ; col4, transpose[0/1] c0, c1, c2, c3, c4, c5, c6, c7
  %if %3 == 0
  %else
    pshufb      m0, [r3]
    pmaddubsw   m0, [r4 + %3 * 16]
    pmulhrsw    m0, [pw_1024]
  %endif
  %if %4 == 0
    pmovzxbw    m1, m1
  %else
    pshufb      m1, [r3]
    pmaddubsw   m1, [r4 + %4 * 16]
    pmulhrsw    m1, [pw_1024]
  %endif
  %if %3 == 0
    packuswb    m1, m1
    movlhps     m0, m1
  %else
    packuswb    m0, m1
  %endif
    mova        m1, [pw_1024]
  %if %5 == 0
  %else
    pshufb      m2, [r3]
    pmaddubsw   m2, [r4 + %5 * 16]
    pmulhrsw    m2, m1
  %endif
  %if %6 == 0
    pmovzxbw    m3, m3
  %else
    pshufb      m3, [r3]
    pmaddubsw   m3, [r4 + %6 * 16]
    pmulhrsw    m3, m1
  %endif
  %if %5 == 0
    packuswb    m3, m3
    movlhps     m2, m3
  %else
    packuswb    m2, m3
  %endif
  %if %7 == 0
  %else
    pshufb      m4, [r3]
    pmaddubsw   m4, [r4 + %7 * 16]
    pmulhrsw    m4, m1
  %endif
  %if %8 == 0
    pmovzxbw    m5, m5
  %else
    pshufb      m5, [r3]
    pmaddubsw   m5, [r4 + %8 * 16]
    pmulhrsw    m5, m1
  %endif
  %if %7 == 0
    packuswb    m5, m5
    movlhps     m4, m5
  %else
    packuswb    m4, m5
  %endif
  %if %9 == 0
  %else
    pshufb      m6, [r3]
    pmaddubsw   m6, [r4 + %9 * 16]
    pmulhrsw    m6, m1
  %endif
  %if %10 == 0
    pmovzxbw    m7, m7
  %else
    pshufb      m7, [r3]
    pmaddubsw   m7, [r4 + %10 * 16]
    pmulhrsw    m7, m1
  %endif
  %if %9 == 0
    packuswb    m7, m7
    movlhps     m6, m7
  %else
    packuswb    m6, m7
  %endif

  %if %2 == 1
    ; transpose
    punpckhbw   m1,        m0, m2
    punpcklbw   m0,        m2
    punpckhbw   m3,        m0, m1
    punpcklbw   m0,        m1

    punpckhbw   m1,        m4, m6
    punpcklbw   m4,        m6
    punpckhbw   m6,        m4, m1
    punpcklbw   m4,        m1

    punpckhdq   m2,        m0, m4
    punpckldq   m0,        m4
    punpckldq   m4,        m3, m6
    punpckhdq   m3,        m6

    movh        [r0 +       + %1 * 8], m0
    movhps      [r0 +  r1   + %1 * 8], m0
    movh        [r0 +  r1*2 + %1 * 8], m2
    movhps      [r0 +  r5   + %1 * 8], m2
    movh        [r6         + %1 * 8], m4
    movhps      [r6 +  r1   + %1 * 8], m4
    movh        [r6 +  r1*2 + %1 * 8], m3
    movhps      [r6 +  r5   + %1 * 8], m3
  %else
    movh        [r0         ], m0
    movhps      [r0 + r1    ], m0
    movh        [r0 + r1 * 2], m2
    movhps      [r0 + r5    ], m2
    lea         r0, [r0 + r1 * 4]
    movh        [r0         ], m4
    movhps      [r0 + r1    ], m4
    movh        [r0 + r1 * 2], m6
    movhps      [r0 + r5    ], m6
  %endif
%endmacro

%macro MODE_3_33 1
    movu        m0,        [r2 + 1]                   ; [16 15 14 13 12 11 10 9  8 7 6 5 4 3 2 1]
    palignr     m1,        m0, 1                      ; [ x 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2]
    punpckhbw   m2,        m0, m1                     ; [x 16 16 15 15 14 14 13 13 12 12 11 11 10 10 9]
    punpcklbw   m0,        m1                         ; [9 8 8 7 7 6 6 5 5 4 4 3 3 2 2 1]
    palignr     m1,        m2, m0, 2                  ; [10 9 9 8 8 7 7 6 6 5 5 4 4 3 3 2]
    pmaddubsw   m4,        m0, [r3 + 10 * 16]         ; [26]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 + 4 * 16]              ; [20]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    palignr     m5,        m2, m0, 4
    pmaddubsw   m5,        [r3 - 2 * 16]              ; [14]
    pmulhrsw    m5,        m7
    palignr     m6,        m2, m0, 6
    pmaddubsw   m6,        [r3 - 8 * 16]              ; [ 8]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    palignr     m1,        m2, m0, 8
    pmaddubsw   m6,        m1, [r3 - 14 * 16]         ; [ 2]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        [r3 + 12 * 16]             ; [28]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    palignr     m1,        m2, m0, 10
    pmaddubsw   m1,        [r3 + 6 * 16]              ; [22]
    pmulhrsw    m1,        m7
    palignr     m2,        m0, 12
    pmaddubsw   m2,        [r3]                       ; [16]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    movu        m0,        [r2 + 8]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m5,        m2, m0, 2
    pmaddubsw   m4,        m0, [r3 - 6 * 16]          ; [10]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        m5, [r3 - 12 * 16]         ; [04]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    pmaddubsw   m5,        [r3 + 14 * 16]             ; [30]
    pmulhrsw    m5,        m7
    palignr     m6,        m2, m0, 4
    pmaddubsw   m6,        [r3 + 8 * 16]              ; [24]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    palignr     m1,        m2, m0, 6
    pmaddubsw   m6,        m1, [r3 + 2 * 16]          ; [18]
    pmulhrsw    m6,        m7
    palignr     m1,        m2, m0, 8
    pmaddubsw   m1,        [r3 - 4 * 16]              ; [12]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    palignr     m1,        m2, m0, 10
    pmaddubsw   m1,        [r3 - 10 * 16]             ; [06]
    pmulhrsw    m1,        m7
    packuswb    m1,        m1
    movhps      m1,        [r2 + 14]                  ; [00]

    TRANSPOSE_STORE_8x8 1, %1, m4, m5, m6, m1

    movu        m0,        [r2 + 14]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m1,        m2, m0, 2
    pmaddubsw   m4,        m0, [r3 + 10 * 16]         ; [26]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 + 4 * 16]              ; [20]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    palignr     m5,        m2, m0, 4
    pmaddubsw   m5,        [r3 - 2 * 16]              ; [14]
    pmulhrsw    m5,        m7
    palignr     m6,        m2, m0, 6
    pmaddubsw   m6,        [r3 - 8 * 16]              ; [ 8]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    palignr     m1,        m2, m0, 8
    pmaddubsw   m6,        m1, [r3 - 14 * 16]         ; [ 2]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        [r3 + 12 * 16]             ; [28]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    palignr     m1,        m2, m0, 10
    pmaddubsw   m1,        [r3 + 6 * 16]              ; [22]
    pmulhrsw    m1,        m7
    palignr     m2,        m0, 12
    pmaddubsw   m2,        [r3]                       ; [16]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 2, %1, m4, m5, m6, m1

    movu        m0,        [r2 + 21]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m5,        m2, m0, 2
    pmaddubsw   m4,        m0, [r3 - 6 * 16]          ; [10]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        m5, [r3 - 12 * 16]         ; [04]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    pmaddubsw   m5,        [r3 + 14 * 16]             ; [30]
    pmulhrsw    m5,        m7
    palignr     m6,        m2, m0, 4
    pmaddubsw   m6,        [r3 + 8 * 16]              ; [24]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    palignr     m1,        m2, m0, 6
    pmaddubsw   m6,        m1, [r3 + 2 * 16]          ; [18]
    pmulhrsw    m6,        m7
    palignr     m1,        m2, m0, 8
    pmaddubsw   m1,        [r3 - 4 * 16]              ; [12]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    palignr     m1,        m2, m0, 10
    pmaddubsw   m1,        [r3 - 10 * 16]             ; [06]
    pmulhrsw    m1,        m7
    packuswb    m1,        m1
    movhps      m1,        [r2 + 27]                  ; [00]

    TRANSPOSE_STORE_8x8 3, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_3(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_3, 3,7,8
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       4
    lea         r5,        [r1 * 3]                   ; r5 -> 3 * stride
    lea         r6,        [r0 + r1 * 4]              ; r6 -> 4 * stride
    mova        m7,        [pw_1024]
.loop:
    MODE_3_33 1
    lea         r0, [r6 + r1 * 4]
    lea         r6, [r6 + r1 * 8]
    add         r2, 8
    dec         r4
    jnz        .loop
    RET

%macro MODE_4_32 1
    movu        m0,        [r2 + 1]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m1,        m2, m0, 2
    mova        m5,        m1
    pmaddubsw   m4,        m0, [r3 + 5 * 16]          ; [21]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 - 6 * 16]              ; [10]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    pmaddubsw   m5,        [r3 + 15 * 16]             ; [31]
    pmulhrsw    m5,        m7
    palignr     m6,        m2, m0, 4
    pmaddubsw   m6,        [r3 + 4 * 16]              ; [ 20]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    palignr     m1,        m2, m0, 6
    pmaddubsw   m6,        m1, [r3 - 7 * 16]          ; [ 9]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        [r3 + 14 * 16]             ; [30]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    palignr     m1,        m2, m0, 8
    pmaddubsw   m1,        [r3 + 3 * 16]              ; [19]
    pmulhrsw    m1,        m7
    palignr     m2,        m0, 10
    pmaddubsw   m3,        m2, [r3 - 8 * 16]          ; [8]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddubsw   m4,        m2, [r3 + 13 * 16]         ; [29]
    pmulhrsw    m4,        m7
    movu        m0,        [r2 + 6]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m1,        m2, m0, 2
    pmaddubsw   m1,        [r3 +  2 * 16]             ; [18]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    palignr     m5,        m2, m0, 4
    mova        m6,        m5
    pmaddubsw   m5,        [r3 - 9 * 16]              ; [07]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        [r3 + 12 * 16]             ; [28]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    palignr     m6,        m2, m0, 6
    pmaddubsw   m6,        [r3 +      16]             ; [17]
    pmulhrsw    m6,        m7
    palignr     m1,        m2, m0, 8
    pmaddubsw   m3,        m1, [r3 - 10 * 16]         ; [06]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3
    pmaddubsw   m1,        [r3 + 11 * 16]             ; [27]
    pmulhrsw    m1,        m7
    palignr     m2,        m0, 10
    pmaddubsw   m2,        [r3]                       ; [16]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 1, %1, m4, m5, m6, m1

    movu        m0,        [r2 + 12]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    mova        m1,        m0
    pmaddubsw   m4,        m0, [r3 - 11 * 16]         ; [5]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 + 10 * 16]             ; [26]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    palignr     m5,        m2, m0, 2
    pmaddubsw   m5,        [r3 - 16]                  ; [15]
    pmulhrsw    m5,        m7
    palignr     m6,        m2, m0, 4
    mova        m1,        m6
    pmaddubsw   m1,        [r3 - 12 * 16]             ; [4]
    pmulhrsw    m1,        m7
    packuswb    m5,        m1
    pmaddubsw   m6,        [r3 + 9 * 16]              ; [25]
    pmulhrsw    m6,        m7
    palignr     m1,        m2, m0, 6
    pmaddubsw   m1,        [r3 - 2 * 16]              ; [14]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    palignr     m1,        m2, m0, 8
    mova        m2,        m1
    pmaddubsw   m1,        [r3 - 13 * 16]             ; [3]
    pmulhrsw    m1,        m7
    pmaddubsw   m2,        [r3 + 8 * 16]              ; [24]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 2, %1, m4, m5, m6, m1

    movu        m0,        [r2 + 17]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    pmaddubsw   m4,        m0, [r3 - 3 * 16]          ; [13]
    pmulhrsw    m4,        m7
    palignr     m5,        m2, m0, 2
    pmaddubsw   m1,        m5, [r3 - 14 * 16]         ; [2]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    pmaddubsw   m5,        [r3 + 7 * 16]              ; [23]
    pmulhrsw    m5,        m7
    palignr     m6,        m2, m0, 4
    pmaddubsw   m6,        [r3 - 4 * 16]              ; [12]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    palignr     m6,        m2, m0, 6
    mova        m1,        m6
    pmaddubsw   m6,        [r3 - 15 * 16]             ; [1]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        [r3 + 6 * 16]              ; [22]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    palignr     m1,        m2, m0, 8
    pmaddubsw   m1,        [r3 - 5 * 16]              ; [11]
    pmulhrsw    m1,        m7
    packuswb    m1,        m1
    movhps      m1,        [r2 + 22]                  ; [00]

    TRANSPOSE_STORE_8x8 3, %1, m4, m5, m6, m1
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void intraPredAng32_4(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-----------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_4, 3,7,8
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       4
    lea         r5,        [r1 * 3]                    ; r5 -> 3 * stride
    lea         r6,        [r0 + r1 * 4]               ; r6 -> 4 * stride
    mova        m7,        [pw_1024]
.loop:
    MODE_4_32 1
    lea         r0,        [r6 + r1 * 4]
    lea         r6,        [r6 + r1 * 8]
    add         r2,        8
    dec         r4
    jnz        .loop
    RET

%macro MODE_5_31 1
    movu        m0,        [r2 + 1]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m1,        m2, m0, 2
    mova        m5,        m1
    pmaddubsw   m4,        m0, [r3 +      16]          ; [17]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 - 14 * 16]              ; [2]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    pmaddubsw   m5,        [r3 + 3 * 16]               ; [19]
    pmulhrsw    m5,        m7
    palignr     m6,        m2, m0, 4
    mova        m1,        m6
    pmaddubsw   m6,        [r3 - 12 * 16]              ; [4]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m1, [r3 + 5 * 16]               ; [21]
    pmulhrsw    m6,        m7
    palignr     m1,        m2, m0, 6
    mova        m3,        m1
    pmaddubsw   m3,        [r3 - 10 * 16]              ; [6]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3
    pmaddubsw   m1,        [r3 + 7 * 16]               ; [23]
    pmulhrsw    m1,        m7
    palignr     m2,        m0, 8
    pmaddubsw   m2,        [r3 - 8 * 16]               ; [8]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    movu        m0,        [r2 + 5]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m1,        m2, m0, 2
    mova        m5,        m1
    pmaddubsw   m4,        m0, [r3 + 9 * 16]           ; [25]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 - 6 * 16]               ; [10]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    pmaddubsw   m5,        [r3 + 11 * 16]              ; [27]
    pmulhrsw    m5,        m7
    palignr     m6,        m2, m0, 4
    mova        m1,        m6
    pmaddubsw   m6,        [r3 - 4 * 16]               ; [12]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m1, [r3 + 13 * 16]          ; [29]
    pmulhrsw    m6,        m7
    palignr     m1,        m2, m0, 6
    mova        m3,        m1
    pmaddubsw   m3,        [r3 - 2 * 16]               ; [14]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3
    pmaddubsw   m1,        [r3 + 15 * 16]              ; [31]
    pmulhrsw    m1,        m7
    palignr     m2,        m0, 8
    pmaddubsw   m2,        [r3]                        ; [16]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 1, %1, m4, m5, m6, m1

    movu        m0,        [r2 + 10]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    mova        m1,        m0
    pmaddubsw   m4,        m0, [r3 - 15 * 16]          ; [1]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 + 2 * 16]               ; [18]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    palignr     m5,        m2, m0, 2
    mova        m1,        m5
    pmaddubsw   m5,        [r3 - 13 * 16]              ; [3]
    pmulhrsw    m5,        m7
    pmaddubsw   m1,        [r3 + 4 * 16]               ; [20]
    pmulhrsw    m1,        m7
    packuswb    m5,        m1
    palignr     m1,        m2, m0, 4
    pmaddubsw   m6,        m1, [r3 - 11 * 16]          ; [5]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        [r3 + 6 * 16]               ; [22]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    palignr     m2,        m0, 6
    pmaddubsw   m1,        m2, [r3 - 9 * 16]           ; [7]
    pmulhrsw    m1,        m7
    pmaddubsw   m2,        [r3 + 8 * 16]               ; [24]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 2, %1, m4, m5, m6, m1

    movu        m0,        [r2 + 14]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    mova        m1,        m0
    pmaddubsw   m4,        m0, [r3 - 7 * 16]           ; [9]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 + 10 * 16]              ; [26]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    palignr     m5,        m2, m0, 2
    mova        m1,        m5
    pmaddubsw   m5,        [r3 - 5 * 16]               ; [11]
    pmulhrsw    m5,        m7
    pmaddubsw   m1,        [r3 + 12 * 16]              ; [28]
    pmulhrsw    m1,        m7
    packuswb    m5,        m1
    palignr     m1,        m2, m0, 4
    pmaddubsw   m6,        m1, [r3 - 3 * 16]           ; [13]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        [r3 + 14 * 16]              ; [30]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    palignr     m2,        m0, 6
    pmaddubsw   m1,        m2, [r3 - 16]               ; [15]
    pmulhrsw    m1,        m7
    packuswb    m1,        m1
    movhps      m1,        [r2 + 18]                   ; [00]

    TRANSPOSE_STORE_8x8 3, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_5(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_5, 3,7,8
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       4
    lea         r5,        [r1 * 3]                   ; r5 -> 3 * stride
    lea         r6,        [r0 + r1 * 4]              ; r6 -> 4 * stride
    mova        m7,        [pw_1024]
.loop:
    MODE_5_31 1
    lea         r0,        [r6 + r1 * 4]
    lea         r6,        [r6 + r1 * 8]
    add         r2,        8
    dec         r4
    jnz        .loop
    RET

%macro MODE_6_30 1
    movu        m0,        [r2 + 1]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    mova        m1,        m0
    pmaddubsw   m4,        m0, [r3 - 3 * 16]          ; [13]
    pmulhrsw    m4,        m7
    pmaddubsw   m1,        [r3 + 10 * 16]             ; [26]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    palignr     m6,        m2, m0, 2
    pmaddubsw   m5,        m6, [r3 - 9 * 16]          ; [7]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        [r3 + 4 * 16]              ; [20]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    palignr     m1,        m2, m0, 4
    pmaddubsw   m6,        m1, [r3 - 15 * 16]         ; [1]
    pmulhrsw    m6,        m7
    pmaddubsw   m3,        m1, [r3 - 2 * 16]          ; [14]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3
    pmaddubsw   m1,        [r3 + 11 * 16]             ; [27]
    pmulhrsw    m1,        m7
    palignr     m2,        m0, 6
    pmaddubsw   m3,        m2, [r3 - 8 * 16]          ; [8]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddubsw   m4,        m2, [r3 +  5 * 16]         ; [21]
    pmulhrsw    m4,        m7
    movu        m0,        [r2 + 5]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    mova        m6,        m0
    pmaddubsw   m1,        m6, [r3 - 14 * 16]         ; [2]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    pmaddubsw   m5,        m6, [r3 - 16]              ; [15]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        [r3 + 12 * 16]             ; [28]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    palignr     m3,        m2, m0, 2
    pmaddubsw   m6,        m3, [r3 - 7 * 16]          ; [9]
    pmulhrsw    m6,        m7
    pmaddubsw   m3,        [r3 + 6 * 16]              ; [22]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3
    palignr     m2,        m0, 4
    pmaddubsw   m1,        m2, [r3 - 13 * 16]         ; [3]
    pmulhrsw    m1,        m7
    pmaddubsw   m3,        m2, [r3]                   ; [16]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3

    TRANSPOSE_STORE_8x8 1, %1, m4, m5, m6, m1

    pmaddubsw   m4,        m2, [r3 +  13 * 16]        ; [29]
    pmulhrsw    m4,        m7
    movu        m0,        [r2 + 7]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m5,        m2, m0, 2
    pmaddubsw   m1,        m5, [r3 - 6 * 16]          ; [10]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    pmaddubsw   m5,        [r3 + 7 * 16]              ; [23]
    pmulhrsw    m5,        m7
    palignr     m1,        m2, m0, 4
    pmaddubsw   m6,        m1, [r3 - 12 * 16]         ; [4]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m1, [r3 + 16]              ; [17]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        [r3 + 14 * 16]             ; [30]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    palignr     m2,        m2, m0, 6
    pmaddubsw   m1,        m2, [r3 - 5 * 16]          ; [11]
    pmulhrsw    m1,        m7
    pmaddubsw   m2,        m2, [r3 + 8 * 16]          ; [24]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 2, %1, m4, m5, m6, m1

    movu        m0,        [r2 + 11]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    mova        m5,        m0
    pmaddubsw   m4,        m0, [r3 - 11 * 16]         ; [5]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m5, [r3 + 2 * 16]          ; [18]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        [r3 + 15 * 16]             ; [31]
    pmulhrsw    m5,        m7
    palignr     m6,        m2, m0, 2
    pmaddubsw   m1,        m6, [r3 - 4 * 16]          ; [12]
    pmulhrsw    m1,        m7
    packuswb    m5,        m1
    pmaddubsw   m6,        [r3 + 9 * 16]              ; [25]
    pmulhrsw    m6,        m7
    palignr     m1,        m2, m0, 4
    pmaddubsw   m2,        m1, [r3 - 10 * 16]         ; [6]
    pmulhrsw    m2,        m7
    packuswb    m6,        m2
    pmaddubsw   m1,        [r3 + 3 * 16]              ; [19]
    pmulhrsw    m1,        m7
    packuswb    m1,        m1
    movhps      m1,        [r2 + 14]                  ; [00]

    TRANSPOSE_STORE_8x8 3, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_6(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_6, 3,7,8
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       4
    lea         r5,        [r1 * 3]                  ; r5 -> 3 * stride
    lea         r6,        [r0 + r1 * 4]             ; r6 -> 4 * stride
    mova        m7,        [pw_1024]
.loop:
    MODE_6_30 1
    lea         r0,        [r6 + r1 * 4]
    lea         r6,        [r6 + r1 * 8]
    add         r2,        8
    dec         r4
    jnz        .loop
    RET

%macro MODE_7_29 1
    movu        m0,        [r2 + 1]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    mova        m5,        m0
    pmaddubsw   m4,        m0, [r3 - 7 * 16]         ; [9]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m5, [r3 + 2 * 16]         ; [18]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        [r3 + 11 * 16]            ; [27]
    pmulhrsw    m5,        m7
    palignr     m1,        m2, m0, 2
    palignr     m2,        m0, 4
    pmaddubsw   m6,        m1, [r3 - 12 * 16]        ; [4]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m1, [r3 - 3 * 16]         ; [13]
    pmulhrsw    m6,        m7
    pmaddubsw   m0,        m1, [r3 + 6 * 16]         ; [22]
    pmulhrsw    m0,        m7
    packuswb    m6,        m0
    pmaddubsw   m1,        [r3 + 15 * 16]            ; [31]
    pmulhrsw    m1,        m7
    pmaddubsw   m0,        m2, [r3 - 8 * 16]         ; [8]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddubsw   m4,        m2, [r3 + 16]             ; [17]
    pmulhrsw    m4,        m7
    pmaddubsw   m2,        [r3 + 10 * 16]            ; [26]
    pmulhrsw    m2,        m7
    packuswb    m4,        m2
    movu        m0,        [r2 + 4]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m2,        m0, 2
    pmaddubsw   m5,        m0, [r3 - 13 * 16]        ; [03]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m0, [r3 - 4 * 16]         ; [12]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m0, [r3 + 5 * 16]         ; [21]
    pmulhrsw    m6,        m7
    pmaddubsw   m0,        [r3 + 14 * 16]            ; [30]
    pmulhrsw    m0,        m7
    packuswb    m6,        m0
    pmaddubsw   m1,        m2, [r3 - 9 * 16]         ; [07]
    pmulhrsw    m1,        m7
    pmaddubsw   m3,        m2, [r3]                  ; [16]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3

    TRANSPOSE_STORE_8x8 1, %1, m4, m5, m6, m1

    pmaddubsw   m4,        m2, [r3 + 9 * 16]         ; [25]
    pmulhrsw    m4,        m7
    movu        m0,        [r2 + 6]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m2,        m0, 2
    pmaddubsw   m1,        m0, [r3 - 14 * 16]        ; [2]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    pmaddubsw   m5,        m0, [r3 - 5 * 16]         ; [11]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m0, [r3 + 4 * 16]         ; [20]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m0, [r3 + 13 * 16]        ; [29]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m2, [r3 - 10 * 16]        ; [6]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pmaddubsw   m1,        m2, [r3 - 16]             ; [15]
    pmulhrsw    m1,        m7
    pmaddubsw   m2,        m2, [r3 + 8 * 16]         ; [24]
    pmulhrsw    m2,        m7
    packuswb    m1,        m2

    TRANSPOSE_STORE_8x8 2, %1, m4, m5, m6, m1

    movu        m0,        [r2 + 8]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    pmaddubsw   m4,        m0, [r3 - 15 * 16]        ; [1]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m0, [r3 - 6 * 16]         ; [10]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        m0, [r3 + 3 * 16]         ; [19]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m0, [r3 + 12 * 16]        ; [28]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    palignr     m2,        m0, 2
    pmaddubsw   m6,        m2, [r3 - 11 * 16]        ; [5]
    pmulhrsw    m6,        m7
    pmaddubsw   m0,        m2, [r3 - 2 * 16]         ; [14]
    pmulhrsw    m0,        m7
    packuswb    m6,        m0
    pmaddubsw   m1,        m2, [r3 + 7 * 16]         ; [23]
    pmulhrsw    m1,        m7
    packuswb    m1,        m1
    movhps      m1,        [r2 + 10]                 ; [0]

    TRANSPOSE_STORE_8x8 3, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_7(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_7, 3,7,8
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       4
    lea         r5,        [r1 * 3]               ; r5 -> 3 * stride
    lea         r6,        [r0 + r1 * 4]          ; r6 -> 4 * stride
    mova        m7,        [pw_1024]
.loop:
    MODE_7_29 1
    lea         r0,        [r6 + r1 * 4]
    lea         r6,        [r6 + r1 * 8]
    add         r2,        8
    dec         r4
    jnz        .loop
    RET

%macro MODE_8_28 1
    movu        m0,        [r2 + 1]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m2,        m0, 2
    pmaddubsw   m4,        m0, [r3 - 11 * 16]     ; [5]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m0, [r3 - 6 * 16]      ; [10]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        m0, [r3 - 1 * 16]      ; [15]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m0, [r3 + 4 * 16]      ; [20]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m0, [r3 + 9 * 16]      ; [25]
    pmulhrsw    m6,        m7
    pmaddubsw   m0,        [r3 + 14 * 16]         ; [30]
    pmulhrsw    m0,        m7
    packuswb    m6,        m0
    pmaddubsw   m1,        m2, [r3 - 13 * 16]     ; [3]
    pmulhrsw    m1,        m7
    pmaddubsw   m0,        m2, [r3 - 8 * 16]      ; [8]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddubsw   m4,        m2, [r3 - 3 * 16]      ; [13]
    pmulhrsw    m4,        m7
    pmaddubsw   m5,        m2, [r3 + 2 * 16]      ; [18]
    pmulhrsw    m5,        m7
    packuswb    m4,        m5
    pmaddubsw   m5,        m2, [r3 + 7 * 16]      ; [23]
    pmulhrsw    m5,        m7
    pmaddubsw   m2,        [r3 + 12 * 16]         ; [28]
    pmulhrsw    m2,        m7
    packuswb    m5,        m2
    movu        m0,        [r2 + 3]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    pmaddubsw   m6,        m0, [r3 - 15 * 16]     ; [01]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m0, [r3 - 10 * 16]     ; [06]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pmaddubsw   m1,        m0, [r3 - 5 * 16]      ; [11]
    pmulhrsw    m1,        m7
    mova        m2,        m0
    pmaddubsw   m0,        [r3]                   ; [16]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0

    TRANSPOSE_STORE_8x8 1, %1, m4, m5, m6, m1

    pmaddubsw   m4,        m2, [r3 + 5 * 16]      ; [21]
    pmulhrsw    m4,        m7
    pmaddubsw   m5,        m2, [r3 + 10 * 16]     ; [26]
    pmulhrsw    m5,        m7
    packuswb    m4,        m5
    pmaddubsw   m5,        m2, [r3 + 15 * 16]     ; [31]
    pmulhrsw    m5,        m7
    movu        m0,        [r2 + 4]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    pmaddubsw   m2,        m0, [r3 - 12 * 16]     ; [4]
    pmulhrsw    m2,        m7
    packuswb    m5,        m2
    pmaddubsw   m6,        m0, [r3 - 7 * 16]      ; [9]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m0, [r3 - 2 * 16]      ; [14]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pmaddubsw   m1,        m0, [r3 + 3 * 16]      ; [19]
    pmulhrsw    m1,        m7
    mova        m2,        m0
    pmaddubsw   m0,        [r3 + 8 * 16]          ; [24]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0

    TRANSPOSE_STORE_8x8 2, %1, m4, m5, m6, m1

    pmaddubsw   m4,        m2, [r3 + 13 * 16]     ; [29]
    pmulhrsw    m4,        m7
    movu        m0,        [r2 + 5]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    pmaddubsw   m1,        m0, [r3 - 14 * 16]     ; [2]
    pmulhrsw    m1,        m7
    packuswb    m4,        m1
    pmaddubsw   m5,        m0, [r3 - 9 * 16]      ; [7]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m0, [r3 - 4 * 16]      ; [12]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m0, [r3 + 16]          ; [17]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m0, [r3 + 6 * 16]      ; [22]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pmaddubsw   m1,        m0, [r3 + 11 * 16]         ; [27]
    pmulhrsw    m1,        m7
    packuswb    m1,        m1
    movhps      m1,        [r2 + 6]               ; [00]

    TRANSPOSE_STORE_8x8 3, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_8(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_8, 3,7,8
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       4
    lea         r5,        [r1 * 3]            ; r5 -> 3 * stride
    lea         r6,        [r0 + r1 * 4]       ; r6 -> 4 * stride
    mova        m7,        [pw_1024]
.loop:
    MODE_8_28 1
    lea         r0,        [r6 + r1 * 4]
    lea         r6,        [r6 + r1 * 8]
    add         r2,        8
    dec         r4
    jnz        .loop
    RET

%macro MODE_9_27 1
    movu        m2,        [r2 + 1]
    palignr     m1,        m2, 1
    punpckhbw   m0,        m2, m1
    punpcklbw   m2,        m1
    pmaddubsw   m4,        m2, [r3 - 14 * 16]   ; [2]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m2, [r3 - 12 * 16]   ; [4]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        m2, [r3 - 10 * 16]   ; [6]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r3 - 8 * 16]    ; [8]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r3 - 6 * 16]    ; [10]
    pmulhrsw    m6,        m7
    pmaddubsw   m3,        m2, [r3 - 4 * 16]    ; [12]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3
    pmaddubsw   m1,        m2, [r3 - 2 * 16]    ; [14]
    pmulhrsw    m1,        m7
    pmaddubsw   m0,        m2, [r3]             ; [16]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0

    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1

    pmaddubsw   m4,        m2, [r3 + 2 * 16]    ; [18]
    pmulhrsw    m4,        m7
    pmaddubsw   m5,        m2, [r3 + 4 * 16]    ; [20]
    pmulhrsw    m5,        m7
    packuswb    m4,        m5
    pmaddubsw   m5,        m2, [r3 + 6 * 16]    ; [22]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r3 + 8 * 16]    ; [24]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r3 + 10 * 16]   ; [26]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m2, [r3 + 12 * 16]   ; [28]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pmaddubsw   m1,        m2, [r3 + 14 * 16]   ; [30]
    pmulhrsw    m1,        m7
    packuswb    m1,        m1
    movhps      m1,        [r2 + 2]             ; [00]

    TRANSPOSE_STORE_8x8 1, %1, m4, m5, m6, m1

    movu        m2,        [r2 + 2]
    palignr     m1,        m2, 1
    punpcklbw   m2,        m1
    pmaddubsw   m4,        m2, [r3 - 14 * 16]   ; [2]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m2, [r3 - 12 * 16]   ; [4]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        m2, [r3 - 10 * 16]   ; [6]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r3 - 8 * 16]    ; [8]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r3 - 6 * 16]    ; [10]
    pmulhrsw    m6,        m7
    pmaddubsw   m0,        m2, [r3 - 4 * 16]    ; [12]
    pmulhrsw    m0,        m7
    packuswb    m6,        m0
    pmaddubsw   m1,        m2, [r3 - 2 * 16]    ; [14]
    pmulhrsw    m1,        m7
    pmaddubsw   m0,        m2, [r3]             ; [16]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0

    TRANSPOSE_STORE_8x8 2, %1, m4, m5, m6, m1

    movu        m2,        [r2 + 2]
    palignr     m1,        m2, 1
    punpcklbw   m2,        m1
    pmaddubsw   m4,        m2, [r3 + 2 * 16]    ; [18]
    pmulhrsw    m4,        m7
    pmaddubsw   m5,        m2, [r3 + 4 * 16]    ; [20]
    pmulhrsw    m5,        m7
    packuswb    m4,        m5
    pmaddubsw   m5,        m2, [r3 + 6 * 16]    ; [22]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r3 + 8 * 16]    ; [24]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r3 + 10 * 16]   ; [26]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m2, [r3 + 12 * 16]   ; [28]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pmaddubsw   m1,        m2, [r3 + 14 * 16]   ; [30]
    pmulhrsw    m1,        m7
    packuswb    m1,        m1
    movhps      m1,        [r2 + 3]             ; [00]

     TRANSPOSE_STORE_8x8 3, %1, m4, m5, m6, m1
%endmacro
;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_9(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_9, 3,7,8
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       4
    lea         r5,        [r1 * 3]         ; r5 -> 3 * stride
    lea         r6,        [r0 + r1 * 4]    ; r6 -> 4 * stride
    mova        m7,        [pw_1024]
.loop:
    MODE_9_27 1
    lea         r0,        [r6 + r1 * 4]
    lea         r6,        [r6 + r1 * 8]
    add         r2,        8
    dec         r4
    jnz        .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_10(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_10, 6,7,8,0-(2*mmsize)
%define m8 [rsp + 0 * mmsize]
%define m9 [rsp + 1 * mmsize]
    lea         r4, [r1 * 3]
    pxor        m7, m7
    mov         r6, 2
    movu        m0, [r3]
    movu        m1, [r3 + 1]
    mova        m8, m0
    mova        m9, m1
    mov         r3d, r5d

.loop:
    movu        m0, [r2 + 1]
    palignr     m1, m0, 1
    pshufb      m1, m7
    palignr     m2, m0, 2
    pshufb      m2, m7
    palignr     m3, m0, 3
    pshufb      m3, m7
    palignr     m4, m0, 4
    pshufb      m4, m7
    palignr     m5, m0, 5
    pshufb      m5, m7
    palignr     m6, m0, 6
    pshufb      m6, m7

    movu        [r0 + r1], m1
    movu        [r0 + r1 + 16], m1
    movu        [r0 + r1 * 2], m2
    movu        [r0 + r1 * 2 + 16], m2
    movu        [r0 + r4], m3
    movu        [r0 + r4 + 16], m3
    lea         r5, [r0 + r1 * 4]
    movu        [r5], m4
    movu        [r5 + 16], m4
    movu        [r5 + r1], m5
    movu        [r5 + r1 + 16], m5
    movu        [r5 + r1 * 2], m6
    movu        [r5 + r1 * 2 + 16], m6

    palignr     m1, m0, 7
    pshufb      m1, m7
    movhlps     m2, m0
    pshufb      m2, m7
    palignr     m3, m0, 9
    pshufb      m3, m7
    palignr     m4, m0, 10
    pshufb      m4, m7
    palignr     m5, m0, 11
    pshufb      m5, m7
    palignr     m6, m0, 12
    pshufb      m6, m7

    movu        [r5 + r4], m1
    movu        [r5 + r4 + 16], m1
    lea         r5, [r5 + r1 * 4]
    movu        [r5], m2
    movu        [r5 + 16], m2
    movu        [r5 + r1], m3
    movu        [r5 + r1 + 16], m3
    movu        [r5 + r1 * 2], m4
    movu        [r5 + r1 * 2 + 16], m4
    movu        [r5 + r4], m5
    movu        [r5 + r4 + 16], m5
    lea         r5, [r5 + r1 * 4]
    movu        [r5], m6
    movu        [r5 + 16], m6

    palignr     m1, m0, 13
    pshufb      m1, m7
    palignr     m2, m0, 14
    pshufb      m2, m7
    palignr     m3, m0, 15
    pshufb      m3, m7
    pshufb      m0, m7

    movu        [r5 + r1], m1
    movu        [r5 + r1 + 16], m1
    movu        [r5 + r1 * 2], m2
    movu        [r5 + r1 * 2 + 16], m2
    movu        [r5 + r4], m3
    movu        [r5 + r4 + 16], m3

; filter
    cmp         r3d, byte 0
    jz         .quit
    movhlps     m1, m0
    pmovzxbw    m0, m0
    mova        m1, m0
    movu        m2, m8
    movu        m3, m9

    pshufb      m2, m7
    pmovzxbw    m2, m2
    movhlps     m4, m3
    pmovzxbw    m3, m3
    pmovzxbw    m4, m4
    psubw       m3, m2
    psubw       m4, m2
    psraw       m3, 1
    psraw       m4, 1
    paddw       m0, m3
    paddw       m1, m4
    packuswb    m0, m1

.quit:
    movu        [r0], m0
    movu        [r0 + 16], m0
    dec         r6
    lea         r0, [r5 + r1 * 4]
    lea         r2, [r2 + 16]
    jnz         .loop
    RET

;-------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_11(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_11, 4,7,8
    ; NOTE: alignment stack to 64 bytes, so all of local data in same cache line

    mov         r6, rsp
    sub         rsp, 64+gprsize
    and         rsp, ~63
    mov         [rsp+64], r6

    ; collect reference pixel
    movu        m0, [r3 + 16]
    pxor        m1, m1
    pshufb      m0, m1                   ; [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
    mova        [rsp], m0
    movu        m0, [r2]
    movu        m1, [r2 + 16]
    movu        m2, [r2 + 32]
    movu        [rsp + 1], m0
    movu        [rsp + 1 + 16], m1
    movu        [rsp + 1 + 32], m2
    mov         [rsp + 63], byte 4

    ; filter
    lea         r2, [rsp + 1]            ; r2 -> [0]
    lea         r3, [c_shuf8_0]          ; r3 -> shuffle8
    lea         r4, [ang_table]          ; r4 -> ang_table
    lea         r5, [r1 * 3]             ; r5 -> 3 * stride
    lea         r6, [r0 + r1 * 4]        ; r6 -> 4 * stride
    mova        m5, [pw_1024]            ; m5 -> 1024
    mova        m6, [c_deinterval8]      ; m6 -> c_deinterval8

.loop:
    ; Row[0 - 7]
    movu        m7, [r2]
    mova        m0, m7
    mova        m1, m7
    mova        m2, m7
    mova        m3, m7
    mova        m4, m7
    mova        m5, m7
    mova        m6, m7
    PROC32_8x8  0, 1, 30,28,26,24,22,20,18,16

    ; Row[8 - 15]
    movu        m7, [r2]
    mova        m0, m7
    mova        m1, m7
    mova        m2, m7
    mova        m3, m7
    mova        m4, m7
    mova        m5, m7
    mova        m6, m7
    PROC32_8x8  1, 1, 14,12,10,8,6,4,2,0

    ; Row[16 - 23]
    movu        m7, [r2 - 1]
    mova        m0, m7
    mova        m1, m7
    mova        m2, m7
    mova        m3, m7
    mova        m4, m7
    mova        m5, m7
    mova        m6, m7
    PROC32_8x8  2, 1, 30,28,26,24,22,20,18,16

    ; Row[24 - 31]
    movu        m7, [r2 - 1]
    mova        m0, m7
    mova        m1, m7
    mova        m2, m7
    mova        m3, m7
    mova        m4, m7
    mova        m5, m7
    mova        m6, m7
    PROC32_8x8  3, 1, 14,12,10,8,6,4,2,0

    lea         r0, [r6 + r1 * 4]
    lea         r6, [r6 + r1 * 8]
    add         r2, 8
    dec         byte [rsp + 63]
    jnz        .loop
    mov         rsp, [rsp+64]
    RET

%macro MODE_12_24_ROW0 1
    movu        m0,        [r3 + 6]
    pshufb      m0,        [c_mode32_12_0]
    pinsrb      m0,        [r3 + 26], 12
    mova        above,     m0
    movu        m2,        [r2]
    palignr     m1,        m2, 1
    punpcklbw   m2,        m1
    pmaddubsw   m4,        m2, [r4 + 11 * 16]         ; [27]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m2, [r4 + 6 * 16]          ; [22]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        m2, [r4 + 16]              ; [17]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r4 - 4 * 16]          ; [12]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r4 - 9 * 16]          ; [7]
    pmulhrsw    m6,        m7
    pmaddubsw   m3,        m2, [r4 - 14 * 16]         ; [2]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3
    movu        m1,        [r2]                       ; [15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    palignr     m2,        m1, above, 15              ; [14 13 12 11 10 9 8 7 6 5 4 3 2 1 0 a]
    punpcklbw   m2,        m1                         ; [7 6 6 5 5 4 4 3 3 2 2 1 1 0 0 a]
    pmaddubsw   m1,        m2, [r4 + 13 * 16]             ; [29]
    pmulhrsw    m1,        m7
    pmaddubsw   m3,        m2, [r4 + 8 * 16]          ; [24]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3
    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1
    pmaddubsw   m4,        m2, [r4 + 3 * 16]          ; [19]
    pmulhrsw    m4,        m7
    pmaddubsw   m5,        m2, [r4 - 2 * 16]          ; [14]
    pmulhrsw    m5,        m7
    packuswb    m4,        m5
    pmaddubsw   m5,        m2, [r4 - 7 * 16]          ; [09]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r4 - 12 * 16]         ; [04]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    palignr     m2,        above, 14                  ;[6 5 5 4 4 3 3 2 2 1 1 0 0 a a b]
    pmaddubsw   m6,        m2, [r4 + 15 * 16]         ; [31]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m2, [r4 + 10 * 16]         ; [26]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pmaddubsw   m1,        m2, [r4 + 5 * 16]          ; [21]
    pmulhrsw    m1,        m7
    pmaddubsw   m3,        m2, [r4]                   ; [16]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3
    TRANSPOSE_STORE_8x8 1, %1, m4, m5, m6, m1
    pmaddubsw   m4,        m2, [r4 - 5 * 16]          ; [11]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m2, [r4 - 10 * 16]         ; [06]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        m2, [r4 - 15 * 16]         ; [1]
    pmulhrsw    m5,        m7
    pslldq      m1,        above, 1
    palignr     m2,        m1, 14
    pmaddubsw   m6,        m2, [r4 + 12 * 16]         ; [28]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r4 + 7 * 16]          ; [23]
    pmulhrsw    m6,        m7
    pmaddubsw   m3,        m2, [r4 + 2 * 16]          ; [18]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3
    pmaddubsw   m1,        m2, [r4 - 3 * 16]          ; [13]
    pmulhrsw    m1,        m7
    pmaddubsw   m3,        m2, [r4 - 8 * 16]          ; [8]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3
    TRANSPOSE_STORE_8x8 2, %1, m4, m5, m6, m1
    pmaddubsw   m4,        m2, [r4 - 13 * 16]         ; [3]
    pmulhrsw    m4,        m7
    pslldq      m1,        above, 2
    palignr     m2,        m1, 14
    pmaddubsw   m5,        m2, [r4 + 14 * 16]         ; [30]
    pmulhrsw    m5,        m7
    packuswb    m4,        m5
    pmaddubsw   m5,        m2, [r4 + 9 * 16]          ; [25]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r4 + 4 * 16]          ; [20]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r4 - 16]              ; [15]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m2, [r4 - 6 * 16]          ; [10]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pmaddubsw   m1,        m2, [r4 - 11 * 16]         ; [05]
    pmulhrsw    m1,        m7
    movu        m0,        [pb_fact0]
    pshufb      m2,        m0
    pmovzxbw    m2,        m2
    packuswb    m1,        m2
    TRANSPOSE_STORE_8x8 3, %1, m4, m5, m6, m1
%endmacro

%macro MODE_12_24 1
    movu        m2,        [r2]
    palignr     m1,        m2, 1
    punpckhbw   m0,        m2, m1
    punpcklbw   m2,        m1
    palignr     m0,        m2, 2
    pmaddubsw   m4,        m0, [r4 + 11 * 16]         ; [27]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m0, [r4 + 6 * 16]          ; [22]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        m0, [r4 + 16]              ; [17]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m0, [r4 - 4 * 16]          ; [12]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m0, [r4 - 9 * 16]          ; [7]
    pmulhrsw    m6,        m7
    pmaddubsw   m3,        m0, [r4 - 14 * 16]         ; [2]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3
    pmaddubsw   m1,        m2, [r4 + 13 * 16]         ; [29]
    pmulhrsw    m1,        m7
    pmaddubsw   m3,        m2, [r4 + 8 * 16]          ; [24]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3
    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1
    pmaddubsw   m4,        m2, [r4 + 3 * 16]          ; [19]
    pmulhrsw    m4,        m7
    pmaddubsw   m5,        m2, [r4 - 2 * 16]          ; [14]
    pmulhrsw    m5,        m7
    packuswb    m4,        m5
    pmaddubsw   m5,        m2, [r4 - 7 * 16]          ; [09]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r4 - 12 * 16]         ; [04]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    movu        m0,        [r2 - 2]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m2,        m0, 2
    pmaddubsw   m6,        m2, [r4 + 15 * 16]         ; [31]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m2, [r4 + 10 * 16]         ; [26]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pmaddubsw   m1,        m2, [r4 + 5 * 16]          ; [21]
    pmulhrsw    m1,        m7
    pmaddubsw   m3,        m2, [r4]                   ; [16]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3
    TRANSPOSE_STORE_8x8 1, %1, m4, m5, m6, m1
    pmaddubsw   m4,        m2, [r4 - 5 * 16]          ; [11]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m2, [r4 - 10 * 16]         ; [06]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        m2, [r4 - 15 * 16]         ; [1]
    pmulhrsw    m5,        m7
    movu        m0,        [r2 - 3]
    palignr     m1,        m0, 1
    punpckhbw   m2,        m0, m1
    punpcklbw   m0,        m1
    palignr     m2,        m0, 2
    pmaddubsw   m6,        m2, [r4 + 12 * 16]         ; [28]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r4 + 7 * 16]          ; [23]
    pmulhrsw    m6,        m7
    pmaddubsw   m3,        m2, [r4 + 2 * 16]          ; [18]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3
    pmaddubsw   m1,        m2, [r4 - 3 * 16]          ; [13]
    pmulhrsw    m1,        m7
    pmaddubsw   m3,        m2, [r4 - 8 * 16]          ; [8]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3
    TRANSPOSE_STORE_8x8 2, %1, m4, m5, m6, m1
    pmaddubsw   m4,        m2, [r4 - 13 * 16]         ; [3]
    pmulhrsw    m4,        m7
    movu        m2,        [r2 - 4]
    palignr     m1,        m2, 1
    punpckhbw   m0,        m2, m1
    punpcklbw   m2,        m1
    palignr     m0,        m2, 2
    pmaddubsw   m5,        m0, [r4 + 14 * 16]         ; [30]
    pmulhrsw    m5,        m7
    packuswb    m4,        m5
    pmaddubsw   m5,        m0, [r4 + 9 * 16]          ; [25]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m0, [r4 + 4 * 16]          ; [20]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m0, [r4 - 16]              ; [15]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m0, [r4 - 6 * 16]          ; [10]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pmaddubsw   m1,        m0, [r4 - 11 * 16]         ; [05]
    pmulhrsw    m1,        m7
    movu        m2,        [pb_fact0]
    pshufb      m0,        m2
    pmovzxbw    m0,        m0
    packuswb    m1,        m0
    TRANSPOSE_STORE_8x8 3, %1, m4, m5, m6, m1
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void intraPredAng32_12(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-----------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_12, 4,7,8,0-(1*mmsize)
  %define above    [rsp + 0 * mmsize]

    lea         r4,        [ang_table + 16 * 16]
    lea         r5,        [r1 * 3]                   ; r5 -> 3 * stride
    lea         r6,        [r0 + r1 * 4]              ; r6 -> 4 * stride
    mova        m7,        [pw_1024]

    MODE_12_24_ROW0 1
    lea         r0,        [r6 + r1 * 4]
    lea         r6,        [r6 + r1 * 8]
    add         r2,        7
    mov         r3,        3
.loop:
    MODE_12_24 1
    lea         r0,        [r6 + r1 * 4]
    lea         r6,        [r6 + r1 * 8]
    add         r2,        8
    dec         r3
    jnz         .loop
    RET

%macro MODE_13_23_ROW0 1
    movu        m0,        [r3 + 1]
    movu        m1,        [r3 + 15]
    pshufb      m0,        [c_mode32_13_0]
    pshufb      m1,        [c_mode32_13_0]
    punpckldq   m0,        m1
    pshufb      m0,        [c_mode32_13_shuf]
    mova        above,     m0
    movu        m2,        [r2]
    palignr     m1,        m2, 1
    punpcklbw   m2,        m1
    pmaddubsw   m4,        m2, [r4 + 7 * 16]         ; [23]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m2, [r4 - 2 * 16]         ; [14]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        m2, [r4 - 11 * 16]        ; [5]
    pmulhrsw    m5,        m7
    movu        m1,        [r2]                      ; [15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    palignr     m2,        m1, above, 15             ; [14 13 12 11 10 9 8 7 6 5 4 3 2 1 0 a]
    punpcklbw   m2,        m1                        ; [7 6 6 5 5 4 4 3 3 2 2 1 1 0 0]
    pmaddubsw   m6,        m2, [r4 + 12 * 16]        ; [28]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r4 + 3 * 16]         ; [19]
    pmulhrsw    m6,        m7
    pmaddubsw   m0,        m2, [r4 - 6 * 16]         ; [10]
    pmulhrsw    m0,        m7
    packuswb    m6,        m0
    pmaddubsw   m1,        m2, [r4 - 15 * 16]        ; [1]
    pmulhrsw    m1,        m7
    palignr     m2,        above, 14
    pmaddubsw   m3,        m2, [r4 + 8 * 16]         ; [24]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3
    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1
    pmaddubsw   m4,        m2, [r4 - 16]             ; [15]
    pmulhrsw    m4,        m7
    pmaddubsw   m5,        m2, [r4 - 10 * 16]        ; [6]
    pmulhrsw    m5,        m7
    packuswb    m4,        m5
    pslldq      m0,        above, 1
    palignr     m2,        m0, 14
    pmaddubsw   m5,        m2, [r4 + 13 * 16]        ; [29]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r4 + 4 * 16]         ; [20]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r4 - 5 * 16]         ; [11]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m2, [r4 - 14 * 16]        ; [2]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pslldq      m0,        1
    palignr     m2,        m0, 14
    pmaddubsw   m1,        m2, [r4 + 9 * 16]         ; [25]
    pmulhrsw    m1,        m7
    pmaddubsw   m0,        m2, [r4]                  ; [16]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0
    TRANSPOSE_STORE_8x8 1, %1, m4, m5, m6, m1
    pmaddubsw   m4,        m2, [r4 - 9 * 16]         ; [7]
    pmulhrsw    m4,        m7
    pslldq      m0,        above, 3
    palignr     m2,        m0, 14
    pmaddubsw   m3,        m2, [r4 + 14 * 16]        ; [30]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        m2, [r4 + 5 * 16]         ; [21]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r4 - 4 * 16]         ; [12]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r4 - 13 * 16]        ; [3]
    pmulhrsw    m6,        m7
    pslldq      m0,        1
    palignr     m2,        m0, 14
    pmaddubsw   m0,        m2, [r4 + 10 * 16]        ; [26]
    pmulhrsw    m0,        m7
    packuswb    m6,        m0
    pmaddubsw   m1,        m2, [r4 + 16]             ; [17]
    pmulhrsw    m1,        m7
    pmaddubsw   m0,        m2, [r4 - 8 * 16]         ; [8]
    pmulhrsw    m0,        m7
    packuswb    m1,        m0
    TRANSPOSE_STORE_8x8 2, %1, m4, m5, m6, m1
    pslldq      m0,        above, 5
    palignr     m2,        m0, 14
    pmaddubsw   m4,        m2, [r4 + 15 * 16]        ; [31]
    pmulhrsw    m4,        m7
    pmaddubsw   m5,        m2, [r4 + 6 * 16]         ; [22]
    pmulhrsw    m5,        m7
    packuswb    m4,        m5
    pmaddubsw   m5,        m2, [r4 - 3 * 16]         ; [13]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r4 - 12 * 16]        ; [04]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pslldq      m0,        1
    palignr     m2,        m0, 14
    pmaddubsw   m6,        m2, [r4 + 11 * 16]        ; [27]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m2, [r4 + 2 * 16]         ; [18]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pmaddubsw   m1,        m2, [r4 - 7 * 16]         ; [09]
    pmulhrsw    m1,        m7
    pmaddubsw   m3,        m2, [r4 - 16 * 16]        ; [00]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3
    TRANSPOSE_STORE_8x8 3, %1, m4, m5, m6, m1
%endmacro

%macro MODE_13_23 1
    movu        m2,        [r2]                      ; [15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]
    palignr     m1,        m2, 1                     ; [x ,15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
    punpckhbw   m0,        m2, m1                    ; [x, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8]
    punpcklbw   m2,        m1                        ; [8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0]
    palignr     m0,        m2, 2                     ; [9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1]
    pmaddubsw   m4,        m0, [r4 + 7 * 16]         ; [23]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m0, [r4 - 2 * 16]         ; [14]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        m0, [r4 - 11 * 16]        ; [05]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r4 + 12 * 16]        ; [28]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r4 + 3 * 16]         ; [19]
    pmulhrsw    m6,        m7
    pmaddubsw   m3,        m2, [r4 - 6 * 16]         ; [10]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3
    pmaddubsw   m1,        m2, [r4 - 15 * 16]        ; [1]
    pmulhrsw    m1,        m7
    movu        m2,        [r2 - 2]                  ; [14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, -1]
    palignr     m3,        m2, 1                     ; [x, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]
    punpckhbw   m0,        m2, m3
    punpcklbw   m2,        m3
    palignr     m0,        m2, 2
    pmaddubsw   m3,        m0, [r4 + 8 * 16]         ; [24]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3
    mova        m3,        m0
    TRANSPOSE_STORE_8x8 0, %1, m4, m5, m6, m1
    pmaddubsw   m4,        m3, [r4 - 16]             ; [15]
    pmulhrsw    m4,        m7
    pmaddubsw   m5,        m3, [r4 - 10 * 16]        ; [6]
    pmulhrsw    m5,        m7
    packuswb    m4,        m5
    pmaddubsw   m5,        m2, [r4 + 13 * 16]        ; [29]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r4 + 4 * 16]         ; [20]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r4 - 5 * 16]         ; [11]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m2, [r4 - 14 * 16]        ; [2]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    movu        m2,        [r2 - 4]                  ; [15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]
    palignr     m1,        m2, 1                     ; [x ,15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
    punpckhbw   m0,        m2, m1                    ; [x, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8]
    punpcklbw   m2,        m1                        ; [8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0]
    palignr     m0,        m2, 2                     ; [9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1]
    pmaddubsw   m1,        m0, [r4 + 9 * 16]         ; [25]
    pmulhrsw    m1,        m7
    pmaddubsw   m3,        m0, [r4]                  ; [16]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3
    mova        m3,        m0
    TRANSPOSE_STORE_8x8 1, %1, m4, m5, m6, m1
    pmaddubsw   m4,        m3, [r4 - 9 * 16]         ; [7]
    pmulhrsw    m4,        m7
    pmaddubsw   m3,        m2, [r4 + 14 * 16]        ; [30]
    pmulhrsw    m3,        m7
    packuswb    m4,        m3
    pmaddubsw   m5,        m2, [r4 + 5 * 16]         ; [21]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r4 - 4 * 16]         ; [12]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    pmaddubsw   m6,        m2, [r4 - 13 * 16]        ; [3]
    pmulhrsw    m6,        m7
    movu        m2,        [r2 - 6]                  ; [15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]
    palignr     m1,        m2, 1                     ; [x ,15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
    punpckhbw   m0,        m2, m1                    ; [x, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8]
    punpcklbw   m2,        m1                        ; [8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0]
    palignr     m0,        m2, 2                     ; [9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1]
    pmaddubsw   m3,        m0, [r4 + 10 * 16]        ; [26]
    pmulhrsw    m3,        m7
    packuswb    m6,        m3
    pmaddubsw   m1,        m0, [r4 + 16]             ; [17]
    pmulhrsw    m1,        m7
    pmaddubsw   m3,        m0, [r4 - 8 * 16]         ; [8]
    pmulhrsw    m3,        m7
    packuswb    m1,        m3
    TRANSPOSE_STORE_8x8 2, %1, m4, m5, m6, m1
    pmaddubsw   m4,        m2, [r4 + 15 * 16]        ; [31]
    pmulhrsw    m4,        m7
    pmaddubsw   m5,        m2, [r4 + 6 * 16]         ; [22]
    pmulhrsw    m5,        m7
    packuswb    m4,        m5
    pmaddubsw   m5,        m2, [r4 - 3 * 16]         ; [13]
    pmulhrsw    m5,        m7
    pmaddubsw   m6,        m2, [r4 - 12 * 16]        ; [04]
    pmulhrsw    m6,        m7
    packuswb    m5,        m6
    movu        m2,        [r2 - 7]                  ; [15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]
    palignr     m1,        m2, 1                     ; [x ,15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
    punpcklbw   m2,        m1                        ; [8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0]
    pmaddubsw   m6,        m2, [r4 + 11 * 16]        ; [27]
    pmulhrsw    m6,        m7
    pmaddubsw   m1,        m2, [r4 + 2 * 16]         ; [18]
    pmulhrsw    m1,        m7
    packuswb    m6,        m1
    pmaddubsw   m1,        m2, [r4 - 7 * 16]         ; [09]
    pmulhrsw    m1,        m7
    movu        m0,        [pb_fact0]
    pshufb      m2,        m0
    pmovzxbw    m2,        m2
    packuswb    m1,        m2
    TRANSPOSE_STORE_8x8 3, %1, m4, m5, m6, m1
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void intraPredAng32_13(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-----------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_13, 4,7,8,0-(1*mmsize)
%define above [rsp + 0 * mmsize]
    lea         r4,        [ang_table + 16 * 16]
    lea         r5,        [r1 * 3]                  ; r5 -> 3 * stride
    lea         r6,        [r0 + r1 * 4]             ; r6 -> 4 * stride
    mova        m7,        [pw_1024]

    MODE_13_23_ROW0 1
    lea         r0,        [r6 + r1 * 4]
    lea         r6,        [r6 + r1 * 8]
    add         r2,        7
    mov         r3,        3
.loop:
    MODE_13_23 1
    lea         r0,        [r6 + r1 * 4]
    lea         r6,        [r6 + r1 * 8]
    add         r2,        8
    dec         r3
    jnz         .loop
    RET

;-------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_14(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_14, 4,7,8
    ; NOTE: alignment stack to 64 bytes, so all of local data in same cache line
    mov         r6, rsp
    sub         rsp, 64+gprsize
    and         rsp, ~63
    mov         [rsp+64], r6

    ; collect reference pixel
    movu        m0, [r3]
    movu        m1, [r3 + 15]
    pshufb      m0, [c_mode32_14_0]      ; [x x x x x x x x x 0 2 5 7 10 12 15]
    pshufb      m1, [c_mode32_14_0]      ; [x x x x x x x x x 15 17 20 22 25 27 30]
    pslldq      m1, 10                   ; [17 20 22 25 27 30 x x x x x x x x x x x]
    palignr     m0, m1, 10               ; [x x x 0 2 5 7 10 12 15 17 20 22 25 27 30]
    mova        [rsp], m0
    movu        m0, [r2 + 1]
    movu        m1, [r2 + 1 + 16]
    movu        [rsp + 13], m0
    movu        [rsp + 13 + 16], m1
    mov         [rsp + 63], byte 4

    ; filter
    lea         r2, [rsp + 13]           ; r2 -> [0]
    lea         r3, [c_shuf8_0]          ; r3 -> shuffle8
    lea         r4, [ang_table]          ; r4 -> ang_table
    lea         r5, [r1 * 3]             ; r5 -> 3 * stride
    lea         r6, [r0 + r1 * 4]        ; r6 -> 4 * stride
    mova        m5, [pw_1024]            ; m5 -> 1024
    mova        m6, [c_deinterval8]      ; m6 -> c_deinterval8

.loop:
    ; Row[0 - 7]
    movu        m7, [r2 - 4]
    palignr     m0, m7, 3
    mova        m1, m0
    palignr     m2, m7, 2
    mova        m3, m2
    palignr     m4, m7, 1
    mova        m5, m4
    mova        m6, m4
    PROC32_8x8  0, 1, 19,6,25,12,31,18,5,24

    ; Row[8 - 15]
    movu        m7, [r2 - 7]
    palignr     m0, m7, 3
    palignr     m1, m7, 2
    mova        m2, m1
    mova        m3, m1
    palignr     m4, m7, 1
    mova        m5, m4
    mova        m6, m7
    PROC32_8x8  1, 1, 11,30,17,4,23,10,29,16

    ; Row[16 - 23]
    movu        m7, [r2 - 10]
    palignr     m0, m7, 3
    palignr     m1, m7, 2
    mova        m2, m1
    palignr     m3, m7, 1
    mova        m4, m3
    mova        m5, m3
    mova        m6, m7
    PROC32_8x8  2, 1, 3,22,9,28,15,2,21,8

    ; Row[24 - 31]
    movu        m7, [r2 - 13]
    palignr     m0, m7, 2
    mova        m1, m0
    mova        m2, m0
    palignr     m3, m7, 1
    mova        m4, m3
    mova        m5, m7
    mova        m6, m7
    PROC32_8x8  3, 1, 27,14,1,20,7,26,13,0

    lea         r0, [r6 + r1 * 4]
    lea         r6, [r6 + r1 * 8]
    add         r2, 8
    dec         byte [rsp + 63]
    jnz        .loop
    mov         rsp, [rsp+64]
    RET

;-------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_15(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_15, 4,7,8
    ; NOTE: alignment stack to 64 bytes, so all of local data in same cache line
    mov         r6, rsp
    sub         rsp, 64+gprsize
    and         rsp, ~63
    mov         [rsp+64], r6

    ; collect reference pixel
    movu        m0, [r3]
    movu        m1, [r3 + 15]
    pshufb      m0, [c_mode32_15_0]      ; [x x x x x x x 0 2 4 6 8 9 11 13 15]
    pshufb      m1, [c_mode32_15_0]      ; [x x x x x x x 15 17 19 21 23 24 26 28 30]
    mova        [rsp], m1
    movu        [rsp + 8], m0
    movu        m0, [r2 + 1]
    movu        m1, [r2 + 1 + 16]
    movu        [rsp + 17], m0
    movu        [rsp + 17 + 16], m1
    mov         [rsp + 63], byte 4

    ; filter
    lea         r2, [rsp + 17]           ; r2 -> [0]
    lea         r3, [c_shuf8_0]          ; r3 -> shuffle8
    lea         r4, [ang_table]          ; r4 -> ang_table
    lea         r5, [r1 * 3]             ; r5 -> 3 * stride
    lea         r6, [r0 + r1 * 4]        ; r6 -> 4 * stride
    mova        m5, [pw_1024]            ; m5 -> 1024
    mova        m6, [c_deinterval8]      ; m6 -> c_deinterval8

.loop:
    ; Row[0 - 7]
    movu        m7, [r2 - 5]
    palignr     m0, m7, 4
    palignr     m1, m7, 3
    mova        m2, m1
    palignr     m3, m7, 2
    mova        m4, m3
    palignr     m5, m7, 1
    mova        m6, m5
    PROC32_8x8  0, 1, 15,30,13,28,11,26,9,24

    ; Row[8 - 15]
    movu        m7, [r2 - 9]
    palignr     m0, m7, 4
    palignr     m1, m7, 3
    mova        m2, m1
    palignr     m3, m7, 2
    mova        m4, m3
    palignr     m5, m7, 1
    mova        m6, m5
    PROC32_8x8  1, 1, 7,22,5,20,3,18,1,16

    ; Row[16 - 23]
    movu        m7, [r2 - 13]
    palignr     m0, m7, 3
    mova        m1, m0
    palignr     m2, m7, 2
    mova        m3, m2
    palignr     m4, m7, 1
    mova        m5, m4
    mova        m6, m7
    PROC32_8x8  2, 1, 31,14,29,12,27,10,25,8

    ; Row[24 - 31]
    movu        m7, [r2 - 17]
    palignr     m0, m7, 3
    mova        m1, m0
    palignr     m2, m7, 2
    mova        m3, m2
    palignr     m4, m7, 1
    mova        m5, m4
    mova        m6, m7
    PROC32_8x8  3, 1, 23,6,21,4,19,2,17,0

    lea         r0, [r6 + r1 * 4]
    lea         r6, [r6 + r1 * 8]
    add         r2, 8
    dec         byte [rsp + 63]
    jnz        .loop
    mov         rsp, [rsp+64]
    RET

;-------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_16(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_16, 4,7,8
    ; NOTE: alignment stack to 64 bytes, so all of local data in same cache line
    mov         r6, rsp
    sub         rsp, 64+gprsize
    and         rsp, ~63
    mov         [rsp+64], r6

    ; collect reference pixel
    movu        m0, [r3]
    movu        m1, [r3 + 15]
    pshufb      m0, [c_mode32_16_0]      ; [x x x x x 0 2 3 5 6 8 9 11 12 14 15]
    pshufb      m1, [c_mode32_16_0]      ; [x x x x x 15 17 18 20 21 23 24 26 27 29 30]
    mova        [rsp], m1
    movu        [rsp + 10], m0
    movu        m0, [r2 + 1]
    movu        m1, [r2 + 1 + 16]
    movu        [rsp + 21], m0
    movu        [rsp + 21 + 16], m1
    mov         [rsp + 63], byte 4

    ; filter
    lea         r2, [rsp + 21]           ; r2 -> [0]
    lea         r3, [c_shuf8_0]          ; r3 -> shuffle8
    lea         r4, [ang_table]          ; r4 -> ang_table
    lea         r5, [r1 * 3]             ; r5 -> 3 * stride
    lea         r6, [r0 + r1 * 4]        ; r6 -> 4 * stride
    mova        m5, [pw_1024]            ; m5 -> 1024
    mova        m6, [c_deinterval8]      ; m6 -> c_deinterval8

.loop:
    ; Row[0 - 7]
    movu        m7, [r2 - 6]
    palignr     m0, m7, 5
    palignr     m1, m7, 4
    mova        m2, m1
    palignr     m3, m7, 3
    palignr     m4, m7, 2
    mova        m5, m4
    palignr     m6, m7, 1
    PROC32_8x8  0, 1, 11,22,1,12,23,2,13,24

    ; Row[8 - 15]
    movu        m7, [r2 - 11]
    palignr     m0, m7, 5
    palignr     m1, m7, 4
    palignr     m2, m7, 3
    mova        m3, m2
    palignr     m4, m7, 2
    palignr     m5, m7, 1
    mova        m6, m5
    PROC32_8x8  1, 1, 3,14,25,4,15,26,5,16

    ; Row[16 - 23]
    movu        m7, [r2 - 16]
    palignr     m0, m7, 4
    mova        m1, m0
    palignr     m2, m7, 3
    palignr     m3, m7, 2
    mova        m4, m3
    palignr     m5, m7, 1
    mova        m6, m7
    PROC32_8x8  2, 1, 27,6,17,28,7,18,29,8

    ; Row[24 - 31]
    movu        m7, [r2 - 21]
    palignr     m0, m7, 4
    palignr     m1, m7, 3
    mova        m2, m1
    palignr     m3, m7, 2
    palignr     m4, m7, 1
    mova        m5, m4
    mova        m6, m7
    PROC32_8x8  3, 1, 19,30,9,20,31,10,21,0

    lea         r0, [r6 + r1 * 4]
    lea         r6, [r6 + r1 * 8]
    add         r2, 8
    dec         byte [rsp + 63]
    jnz        .loop
    mov         rsp, [rsp+64]
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_17(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_17, 4,7,8
    ; NOTE: alignment stack to 64 bytes, so all of local data in same cache line
    mov         r6, rsp
    sub         rsp, 64+gprsize
    and         rsp, ~63
    mov         [rsp+64], r6

    ; collect reference pixel
    movu        m0, [r3]
    movu        m1, [r3 + 16]
    pshufb      m0, [c_mode32_17_0]
    pshufb      m1, [c_mode32_17_0]
    mova        [rsp     ], m1
    movu        [rsp + 13], m0
    movu        m0, [r2 + 1]
    movu        m1, [r2 + 1 + 16]
    movu        [rsp + 26], m0
    movu        [rsp + 26 + 16], m1
    mov         [rsp + 63], byte 4

    ; filter
    lea         r2, [rsp + 25]          ; r2 -> [0]
    lea         r3, [c_shuf8_0]         ; r3 -> shuffle8
    lea         r4, [ang_table]         ; r4 -> ang_table
    lea         r5, [r1 * 3]            ; r5 -> 3 * stride
    lea         r6, [r0 + r1 * 4]       ; r6 -> 4 * stride
    mova        m5, [pw_1024]           ; m5 -> 1024
    mova        m6, [c_deinterval8]     ; m6 -> c_deinterval8

.loop:
    ; Row[0 - 7]
    movu        m7, [r2 - 6]
    palignr     m0, m7, 6
    palignr     m1, m7, 5
    palignr     m2, m7, 4
    palignr     m3, m7, 3
    palignr     m4, m7, 2
    mova        m5, m4
    palignr     m6, m7, 1
    PROC32_8x8  0, 1, 6,12,18,24,30,4,10,16

    ; Row[7 - 15]
    movu        m7, [r2 - 12]
    palignr     m0, m7, 5
    palignr     m1, m7, 4
    mova        m2, m1
    palignr     m3, m7, 3
    palignr     m4, m7, 2
    palignr     m5, m7, 1
    mova        m6, m7
    PROC32_8x8  1, 1, 22,28,2,8,14,20,26,0

    ; Row[16 - 23]
    movu        m7, [r2 - 19]
    palignr     m0, m7, 6
    palignr     m1, m7, 5
    palignr     m2, m7, 4
    palignr     m3, m7, 3
    palignr     m4, m7, 2
    mova        m5, m4
    palignr     m6, m7, 1
    PROC32_8x8  2, 1, 6,12,18,24,30,4,10,16

    ; Row[24 - 31]
    movu        m7, [r2 - 25]
    palignr     m0, m7, 5
    palignr     m1, m7, 4
    mova        m2, m1
    palignr     m3, m7, 3
    palignr     m4, m7, 2
    palignr     m5, m7, 1
    mova        m6, m7
    PROC32_8x8  3, 1, 22,28,2,8,14,20,26,0

    lea         r0, [r6 + r1 * 4]
    lea         r6, [r6 + r1 * 8]
    add         r2, 8
    dec         byte [rsp + 63]
    jnz        .loop
    mov         rsp, [rsp+64]

    RET

;-------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_18(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_18, 4,5,5
    movu        m0, [r3]               ; [15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0]
    movu        m1, [r3 + 16]          ; [31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16]
    movu        m2, [r2 + 1]           ; [16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1]
    movu        m3, [r2 + 17]          ; [32 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17]

    lea         r2, [r1 * 2]
    lea         r3, [r1 * 3]
    lea         r4, [r1 * 4]

    movu        [r0], m0
    movu        [r0 + 16], m1

    pshufb      m2, [c_mode32_18_0]    ; [1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16]
    pshufb      m3, [c_mode32_18_0]    ; [17 18 19 20 21 22 23 24 25 26 27 28 19 30 31 32]

    palignr     m4, m0, m2, 15
    movu        [r0 + r1], m4
    palignr     m4, m1, m0, 15
    movu        [r0 + r1 + 16], m4
    palignr     m4, m0, m2, 14
    movu        [r0 + r2], m4
    palignr     m4, m1, m0, 14
    movu        [r0 + r2 + 16], m4
    palignr     m4, m0, m2, 13
    movu        [r0 + r3], m4
    palignr     m4, m1, m0, 13
    movu        [r0 + r3 + 16], m4

    lea         r0, [r0 + r4]

    palignr     m4, m0, m2, 12
    movu        [r0], m4
    palignr     m4, m1, m0, 12
    movu        [r0 + 16], m4
    palignr     m4, m0, m2, 11
    movu        [r0 + r1], m4
    palignr     m4, m1, m0, 11
    movu        [r0 + r1 + 16], m4
    palignr     m4, m0, m2, 10
    movu        [r0 + r2], m4
    palignr     m4, m1, m0, 10
    movu        [r0 + r2 + 16], m4
    palignr     m4, m0, m2, 9
    movu        [r0 + r3], m4
    palignr     m4, m1, m0, 9
    movu        [r0 + r3 + 16], m4

    lea         r0, [r0 + r4]

    palignr     m4, m0, m2, 8
    movu        [r0], m4
    palignr     m4, m1, m0, 8
    movu        [r0 + 16], m4
    palignr     m4, m0, m2, 7
    movu        [r0 + r1], m4
    palignr     m4, m1, m0, 7
    movu        [r0 + r1 + 16], m4
    palignr     m4, m0, m2, 6
    movu        [r0 + r2], m4
    palignr     m4, m1, m0, 6
    movu        [r0 + r2 + 16], m4
    palignr     m4, m0, m2, 5
    movu        [r0 + r3], m4
    palignr     m4, m1, m0, 5
    movu        [r0 + r3 + 16], m4

    lea         r0, [r0 + r4]

    palignr     m4, m0, m2, 4
    movu        [r0], m4
    palignr     m4, m1, m0, 4
    movu        [r0 + 16], m4
    palignr     m4, m0, m2, 3
    movu        [r0 + r1], m4
    palignr     m4, m1, m0, 3
    movu        [r0 + r1 + 16], m4
    palignr     m4, m0, m2, 2
    movu        [r0 + r2], m4
    palignr     m4, m1, m0, 2
    movu        [r0 + r2 + 16], m4
    palignr     m4, m0, m2, 1
    movu        [r0 + r3], m4
    palignr     m4, m1, m0, 1
    movu        [r0 + r3 + 16], m4

    lea         r0, [r0 + r4]

    movu        [r0], m2
    movu        [r0 + 16], m0
    palignr     m4, m2, m3, 15
    movu        [r0 + r1], m4
    palignr     m4, m0, m2, 15
    movu        [r0 + r1 + 16], m4
    palignr     m4, m2, m3, 14
    movu        [r0 + r2], m4
    palignr     m4, m0, m2, 14
    movu        [r0 + r2 + 16], m4
    palignr     m4, m2, m3, 13
    movu        [r0 + r3], m4
    palignr     m4, m0, m2, 13
    movu        [r0 + r3 + 16], m4

    lea         r0, [r0 + r4]

    palignr     m4, m2, m3, 12
    movu        [r0], m4
    palignr     m4, m0, m2, 12
    movu        [r0 + 16], m4
    palignr     m4, m2, m3, 11
    movu        [r0 + r1], m4
    palignr     m4, m0, m2, 11
    movu        [r0 + r1 + 16], m4
    palignr     m4, m2, m3, 10
    movu        [r0 + r2], m4
    palignr     m4, m0, m2, 10
    movu        [r0 + r2 + 16], m4
    palignr     m4, m2, m3, 9
    movu        [r0 + r3], m4
    palignr     m4, m0, m2, 9
    movu        [r0 + r3 + 16], m4

    lea         r0, [r0 + r4]

    palignr     m4, m2, m3, 8
    movu        [r0], m4
    palignr     m4, m0, m2, 8
    movu        [r0 + 16], m4
    palignr     m4, m2, m3, 7
    movu        [r0 + r1], m4
    palignr     m4, m0, m2, 7
    movu        [r0 + r1 + 16], m4
    palignr     m4, m2, m3, 6
    movu        [r0 + r2], m4
    palignr     m4, m0, m2, 6
    movu        [r0 + r2 + 16], m4
    palignr     m4, m2, m3, 5
    movu        [r0 + r3], m4
    palignr     m4, m0, m2, 5
    movu        [r0 + r3 + 16], m4

    lea         r0, [r0 + r4]

    palignr     m4, m2, m3, 4
    movu        [r0], m4
    palignr     m4, m0, m2, 4
    movu        [r0 + 16], m4
    palignr     m4, m2, m3, 3
    movu        [r0 + r1], m4
    palignr     m4, m0, m2, 3
    movu        [r0 + r1 + 16], m4
    palignr     m4, m2, m3, 2
    movu        [r0 + r2], m4
    palignr     m4, m0, m2, 2
    movu        [r0 + r2 + 16], m4
    palignr     m4, m2, m3, 1
    movu        [r0 + r3], m4
    palignr     m4, m0, m2, 1
    movu        [r0 + r3 + 16], m4
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_19(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_19, 4,7,8
    ; NOTE: alignment stack to 64 bytes, so all of local data in same cache line
    xchg        r2, r3
    mov         r6, rsp
    sub         rsp, 64+gprsize
    and         rsp, ~63
    mov         [rsp+64], r6

    ; collect reference pixel
    movu        m0, [r3]
    movu        m1, [r3 + 16]
    pshufb      m0, [c_mode32_17_0]
    pshufb      m1, [c_mode32_17_0]
    mova        [rsp     ], m1
    movu        [rsp + 13], m0
    movu        m0, [r2 + 1]
    movu        m1, [r2 + 1 + 16]
    movu        [rsp + 26], m0
    movu        [rsp + 26 + 16], m1
    mov         [rsp + 63], byte 4

    ; filter
    lea         r2, [rsp + 25]          ; r2 -> [0]
    lea         r3, [c_shuf8_0]         ; r3 -> shuffle8
    lea         r4, [ang_table]         ; r4 -> ang_table
    lea         r5, [r1 * 3]            ; r5 -> 3 * stride
    lea         r6, [r0]                ; r6 -> r0
    mova        m5, [pw_1024]           ; m5 -> 1024
    mova        m6, [c_deinterval8]     ; m6 -> c_deinterval8

.loop:
    ; Row[0 - 7]
    movu        m7, [r2 - 6]
    palignr     m0, m7, 6
    palignr     m1, m7, 5
    palignr     m2, m7, 4
    palignr     m3, m7, 3
    palignr     m4, m7, 2
    mova        m5, m4
    palignr     m6, m7, 1
    PROC32_8x8  0, 0, 6,12,18,24,30,4,10,16

    ; Row[7 - 15]
    movu        m7, [r2 - 12]
    palignr     m0, m7, 5
    palignr     m1, m7, 4
    mova        m2, m1
    palignr     m3, m7, 3
    palignr     m4, m7, 2
    palignr     m5, m7, 1
    mova        m6, m7
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  1, 0, 22,28,2,8,14,20,26,0

    ; Row[16 - 23]
    movu        m7, [r2 - 19]
    palignr     m0, m7, 6
    palignr     m1, m7, 5
    palignr     m2, m7, 4
    palignr     m3, m7, 3
    palignr     m4, m7, 2
    mova        m5, m4
    palignr     m6, m7, 1
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  2, 0, 6,12,18,24,30,4,10,16

    ; Row[24 - 31]
    movu        m7, [r2 - 25]
    palignr     m0, m7, 5
    palignr     m1, m7, 4
    mova        m2, m1
    palignr     m3, m7, 3
    palignr     m4, m7, 2
    palignr     m5, m7, 1
    mova        m6, m7
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  3, 0, 22,28,2,8,14,20,26,0

    add         r6, 8
    mov         r0, r6
    add         r2, 8
    dec         byte [rsp + 63]
    jnz        .loop
    mov         rsp, [rsp+64]
    RET

;-------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_20(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_20, 4,7,8
    ; NOTE: alignment stack to 64 bytes, so all of local data in same cache line
    xchg        r2, r3
    mov         r6, rsp
    sub         rsp, 64+gprsize
    and         rsp, ~63
    mov         [rsp+64], r6

    ; collect reference pixel
    movu        m0, [r3]
    movu        m1, [r3 + 15]
    pshufb      m0, [c_mode32_16_0]      ; [x x x x x 0 2 3 5 6 8 9 11 12 14 15]
    pshufb      m1, [c_mode32_16_0]      ; [x x x x x 15 17 18 20 21 23 24 26 27 29 30]
    mova        [rsp], m1
    movu        [rsp + 10], m0
    movu        m0, [r2 + 1]
    movu        m1, [r2 + 1 + 16]
    movu        [rsp + 21], m0
    movu        [rsp + 21 + 16], m1
    mov         [rsp + 63], byte 4

    ; filter
    lea         r2, [rsp + 21]           ; r2 -> [0]
    lea         r3, [c_shuf8_0]          ; r3 -> shuffle8
    lea         r4, [ang_table]          ; r4 -> ang_table
    lea         r5, [r1 * 3]             ; r5 -> 3 * stride
    lea         r6, [r0]                 ; r6 -> r0
    mova        m5, [pw_1024]            ; m5 -> 1024
    mova        m6, [c_deinterval8]      ; m6 -> c_deinterval8

.loop:
    ; Row[0 - 7]
    movu        m7, [r2 - 6]
    palignr     m0, m7, 5
    palignr     m1, m7, 4
    mova        m2, m1
    palignr     m3, m7, 3
    palignr     m4, m7, 2
    mova        m5, m4
    palignr     m6, m7, 1
    PROC32_8x8  0, 0, 11,22,1,12,23,2,13,24

    ; Row[8 - 15]
    movu        m7, [r2 - 11]
    palignr     m0, m7, 5
    palignr     m1, m7, 4
    palignr     m2, m7, 3
    mova        m3, m2
    palignr     m4, m7, 2
    palignr     m5, m7, 1
    mova        m6, m5
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  1, 0, 3,14,25,4,15,26,5,16

    ; Row[16 - 23]
    movu        m7, [r2 - 16]
    palignr     m0, m7, 4
    mova        m1, m0
    palignr     m2, m7, 3
    palignr     m3, m7, 2
    mova        m4, m3
    palignr     m5, m7, 1
    mova        m6, m7
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  2, 0, 27,6,17,28,7,18,29,8

    ; Row[24 - 31]
    movu        m7, [r2 - 21]
    palignr     m0, m7, 4
    palignr     m1, m7, 3
    mova        m2, m1
    palignr     m3, m7, 2
    palignr     m4, m7, 1
    mova        m5, m4
    mova        m6, m7
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  3, 0, 19,30,9,20,31,10,21,0

    add         r6, 8
    mov         r0, r6
    add         r2, 8
    dec         byte [rsp + 63]
    jnz        .loop
    mov         rsp, [rsp+64]
    RET

;-------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_21(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_21, 4,7,8
    ; NOTE: alignment stack to 64 bytes, so all of local data in same cache line
    xchg        r2, r3
    mov         r6, rsp
    sub         rsp, 64+gprsize
    and         rsp, ~63
    mov         [rsp+64], r6

    ; collect reference pixel
    movu        m0, [r3]
    movu        m1, [r3 + 15]
    pshufb      m0, [c_mode32_15_0]      ; [x x x x x x x 0 2 4 6 8 9 11 13 15]
    pshufb      m1, [c_mode32_15_0]      ; [x x x x x x x 15 17 19 21 23 24 26 28 30]
    mova        [rsp], m1
    movu        [rsp + 8], m0
    movu        m0, [r2 + 1]
    movu        m1, [r2 + 1 + 16]
    movu        [rsp + 17], m0
    movu        [rsp + 17 + 16], m1
    mov         [rsp + 63], byte 4

    ; filter
    lea         r2, [rsp + 17]           ; r2 -> [0]
    lea         r3, [c_shuf8_0]          ; r3 -> shuffle8
    lea         r4, [ang_table]          ; r4 -> ang_table
    lea         r5, [r1 * 3]             ; r5 -> 3 * stride
    lea         r6, [r0]                 ; r6 -> r0
    mova        m5, [pw_1024]            ; m5 -> 1024
    mova        m6, [c_deinterval8]      ; m6 -> c_deinterval8

.loop:
    ; Row[0 - 7]
    movu        m7, [r2 - 5]
    palignr     m0, m7, 4
    palignr     m1, m7, 3
    mova        m2, m1
    palignr     m3, m7, 2
    mova        m4, m3
    palignr     m5, m7, 1
    mova        m6, m5
    PROC32_8x8  0, 0, 15,30,13,28,11,26,9,24

    ; Row[8 - 15]
    movu        m7, [r2 - 9]
    palignr     m0, m7, 4
    palignr     m1, m7, 3
    mova        m2, m1
    palignr     m3, m7, 2
    mova        m4, m3
    palignr     m5, m7, 1
    mova        m6, m5
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  1, 0, 7,22,5,20,3,18,1,16

    ; Row[16 - 23]
    movu        m7, [r2 - 13]
    palignr     m0, m7, 3
    mova        m1, m0
    palignr     m2, m7, 2
    mova        m3, m2
    palignr     m4, m7, 1
    mova        m5, m4
    mova        m6, m7
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  2, 0, 31,14,29,12,27,10,25,8

    ; Row[24 - 31]
    movu        m7, [r2 - 17]
    palignr     m0, m7, 3
    mova        m1, m0
    palignr     m2, m7, 2
    mova        m3, m2
    palignr     m4, m7, 1
    mova        m5, m4
    mova        m6, m7
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  3, 0, 23,6,21,4,19,2,17,0

    add         r6, 8
    mov         r0, r6
    add         r2, 8
    dec         byte [rsp + 63]
    jnz        .loop
    mov         rsp, [rsp+64]
    RET

;-------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_22(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_22, 4,7,8
    ; NOTE: alignment stack to 64 bytes, so all of local data in same cache line

    xchg        r2, r3
    mov         r6, rsp
    sub         rsp, 64+gprsize
    and         rsp, ~63
    mov         [rsp+64], r6

    ; collect reference pixel
    movu        m0, [r3]
    movu        m1, [r3 + 15]
    pshufb      m0, [c_mode32_14_0]      ; [x x x x x x x x x 0 2 5 7 10 12 15]
    pshufb      m1, [c_mode32_14_0]      ; [x x x x x x x x x 15 17 20 22 25 27 30]
    pslldq      m1, 10                   ; [17 20 22 25 27 30 x x x x x x x x x x x]
    palignr     m0, m1, 10               ; [x x x 0 2 5 7 10 12 15 17 20 22 25 27 30]
    mova        [rsp], m0
    movu        m0, [r2 + 1]
    movu        m1, [r2 + 1 + 16]
    movu        [rsp + 13], m0
    movu        [rsp + 13 + 16], m1
    mov         [rsp + 63], byte 4

    ; filter
    lea         r2, [rsp + 13]           ; r2 -> [0]
    lea         r3, [c_shuf8_0]          ; r3 -> shuffle8
    lea         r4, [ang_table]          ; r4 -> ang_table
    lea         r5, [r1 * 3]             ; r5 -> 3 * stride
    lea         r6, [r0]                 ; r6 -> r0
    mova        m5, [pw_1024]            ; m5 -> 1024
    mova        m6, [c_deinterval8]      ; m6 -> c_deinterval8

.loop:
    ; Row[0 - 7]
    movu        m7, [r2 - 4]
    palignr     m0, m7, 3
    mova        m1, m0
    palignr     m2, m7, 2
    mova        m3, m2
    palignr     m4, m7, 1
    mova        m5, m4
    mova        m6, m4
    PROC32_8x8  0, 0, 19,6,25,12,31,18,5,24

    ; Row[8 - 15]
    movu        m7, [r2 - 7]
    palignr     m0, m7, 3
    palignr     m1, m7, 2
    mova        m2, m1
    mova        m3, m1
    palignr     m4, m7, 1
    mova        m5, m4
    mova        m6, m7
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  1, 0, 11,30,17,4,23,10,29,16

    ; Row[16 - 23]
    movu        m7, [r2 - 10]
    palignr     m0, m7, 3
    palignr     m1, m7, 2
    mova        m2, m1
    palignr     m3, m7, 1
    mova        m4, m3
    mova        m5, m3
    mova        m6, m7
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  2, 0, 3,22,9,28,15,2,21,8

    ; Row[24 - 31]
    movu        m7, [r2 - 13]
    palignr     m0, m7, 2
    mova        m1, m0
    mova        m2, m0
    palignr     m3, m7, 1
    mova        m4, m3
    mova        m5, m7
    mova        m6, m7
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  3, 0, 27,14,1,20,7,26,13,0

    add         r6, 8
    mov         r0, r6
    add         r2, 8
    dec         byte [rsp + 63]
    jnz        .loop
    mov         rsp, [rsp+64]
    RET

;-----------------------------------------------------------------------------------------------------------------
; void intraPredAng32_23(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-----------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_23, 4,7,8,0-(1*mmsize)
%define above [rsp + 0 * mmsize]
    xchg        r2,        r3
    lea         r4,        [ang_table + 16 * 16]
    lea         r5,        [r1 * 3]            ; r5 -> 3 * stride
    mov         r6,        r0
    mova        m7,        [pw_1024]

    MODE_13_23_ROW0 0
    add         r6,        8
    mov         r0,        r6
    add         r2,        7
    mov         r3,        3
.loop:
    MODE_13_23 0
    add         r6,        8
    mov         r0,        r6
    add         r2,        8
    dec         r3
    jnz         .loop
    RET

;-----------------------------------------------------------------------------------------------------------------
; void intraPredAng32_24(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-----------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_24, 4,7,8,0-(1*mmsize)
  %define above    [rsp + 0 * mmsize]
    xchg        r2,        r3
    lea         r4,        [ang_table + 16 * 16]
    lea         r5,        [r1 * 3]            ; r5 -> 3 * stride
    mov         r6,        r0
    mova        m7,        [pw_1024]

    MODE_12_24_ROW0 0
    add         r6,        8
    mov         r0,        r6
    add         r2,        7
    mov         r3,        3
.loop:
    MODE_12_24 0
    add         r6,        8
    mov         r0,        r6
    add         r2,        8
    dec         r3
    jnz         .loop
    RET

;-------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_11(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_25, 4,7,8
    ; NOTE: alignment stack to 64 bytes, so all of local data in same cache line
    xchg        r2, r3
    mov         r6, rsp
    sub         rsp, 64+gprsize
    and         rsp, ~63
    mov         [rsp+64], r6

    ; collect reference pixel
    movu        m0, [r3 + 16]
    pxor        m1, m1
    pshufb      m0, m1                   ; [0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
    mova        [rsp], m0
    movu        m0, [r2]
    movu        m1, [r2 + 16]
    movu        m2, [r2 + 32]
    movu        [rsp + 1], m0
    movu        [rsp + 1 + 16], m1
    movu        [rsp + 1 + 32], m2
    mov         [rsp + 63], byte 4

    ; filter
    lea         r2, [rsp + 1]            ; r2 -> [0]
    lea         r3, [c_shuf8_0]          ; r3 -> shuffle8
    lea         r4, [ang_table]          ; r4 -> ang_table
    lea         r5, [r1 * 3]             ; r5 -> 3 * stride
    lea         r6, [r0]                 ; r6 -> r0
    mova        m5, [pw_1024]            ; m5 -> 1024
    mova        m6, [c_deinterval8]      ; m6 -> c_deinterval8

.loop:
    ; Row[0 - 7]
    movu        m7, [r2]
    mova        m0, m7
    mova        m1, m7
    mova        m2, m7
    mova        m3, m7
    mova        m4, m7
    mova        m5, m7
    mova        m6, m7
    PROC32_8x8  0, 0, 30,28,26,24,22,20,18,16

    ; Row[8 - 15]
    movu        m7, [r2]
    mova        m0, m7
    mova        m1, m7
    mova        m2, m7
    mova        m3, m7
    mova        m4, m7
    mova        m5, m7
    mova        m6, m7
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  1, 0, 14,12,10,8,6,4,2,0

    ; Row[16 - 23]
    movu        m7, [r2 - 1]
    mova        m0, m7
    mova        m1, m7
    mova        m2, m7
    mova        m3, m7
    mova        m4, m7
    mova        m5, m7
    mova        m6, m7
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  2, 0, 30,28,26,24,22,20,18,16

    ; Row[24 - 31]
    movu        m7, [r2 - 1]
    mova        m0, m7
    mova        m1, m7
    mova        m2, m7
    mova        m3, m7
    mova        m4, m7
    mova        m5, m7
    mova        m6, m7
    lea         r0, [r0 + r1 * 4]
    PROC32_8x8  3, 0, 14,12,10,8,6,4,2,0

    add         r6, 8
    mov         r0, r6
    add         r2, 8
    dec         byte [rsp + 63]
    jnz        .loop
    mov         rsp, [rsp+64]
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_26(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_26, 6,7,7,0-(2*mmsize)
%define m8 [rsp + 0 * mmsize]
%define m9 [rsp + 1 * mmsize]
    lea         r4,             [r1 * 3]
    mov         r6,             2
    movu        m0,             [r2]
    movu        m1,             [r2 + 1]
    mova        m8,             m0
    mova        m9,             m1
    mov         r2d,            r5d

.loop:
    movu        m0,             [r3 + 1]

    movu        [r0],           m0
    movu        [r0 + r1],      m0
    movu        [r0 + r1 * 2],  m0
    movu        [r0 + r4],      m0
    lea         r5,             [r0 + r1 * 4]
    movu        [r5],           m0
    movu        [r5 + r1],      m0
    movu        [r5 + r1 * 2],  m0
    movu        [r5 + r4],      m0
    lea         r5,             [r5 + r1 * 4]
    movu        [r5],           m0
    movu        [r5 + r1],      m0
    movu        [r5 + r1 * 2],  m0
    movu        [r5 + r4],      m0
    lea         r5,             [r5 + r1 * 4]
    movu        [r5],           m0
    movu        [r5 + r1],      m0
    movu        [r5 + r1 * 2],  m0
    movu        [r5 + r4],      m0
    lea         r5,             [r0 + r1 * 4]
    movu        [r5],           m0
    movu        [r5 + r1],      m0
    movu        [r5 + r1 * 2],  m0
    movu        [r5 + r4],      m0
    lea         r5,             [r5 + r1 * 4]
    movu        [r5],           m0
    movu        [r5 + r1],      m0
    movu        [r5 + r1 * 2],  m0
    movu        [r5 + r4],      m0
    lea         r5,             [r5 + r1 * 4]
    movu        [r5],           m0
    movu        [r5 + r1],      m0
    movu        [r5 + r1 * 2],  m0
    movu        [r5 + r4],      m0
    lea         r5,             [r5 + r1 * 4]
    movu        [r5],           m0
    movu        [r5 + r1],      m0
    movu        [r5 + r1 * 2],  m0
    movu        [r5 + r4],      m0
    lea         r5,             [r5 + r1 * 4]
    movu        [r5],           m0
    movu        [r5 + r1],      m0
    movu        [r5 + r1 * 2],  m0
    movu        [r5 + r4],      m0
    lea         r5,             [r5 + r1 * 4]
    movu        [r5],           m0
    movu        [r5 + r1],      m0
    movu        [r5 + r1 * 2],  m0
    movu        [r5 + r4],      m0
    lea         r5,             [r5 + r1 * 4]
    movu        [r5],           m0
    movu        [r5 + r1],      m0
    movu        [r5 + r1 * 2],  m0
    movu        [r5 + r4],      m0

; filter
    cmp         r2d, byte 0
    jz         .quit

    pxor        m4,        m4
    pshufb      m0,        m4
    pmovzxbw    m0,        m0
    mova        m1,        m0
    movu        m2,        m8
    movu        m3,        m9

    pshufb      m2,        m4
    pmovzxbw    m2,        m2
    movhlps     m4,        m3
    pmovzxbw    m3,        m3
    pmovzxbw    m4,        m4
    psubw       m3,        m2
    psubw       m4,        m2
    psraw       m3,        1
    psraw       m4,        1
    paddw       m0,        m3
    paddw       m1,        m4
    packuswb    m0,        m1

    pextrb      [r0],           m0, 0
    pextrb      [r0 + r1],      m0, 1
    pextrb      [r0 + r1 * 2],  m0, 2
    pextrb      [r0 + r4],      m0, 3
    lea         r5,             [r0 + r1 * 4]
    pextrb      [r5],           m0, 4
    pextrb      [r5 + r1],      m0, 5
    pextrb      [r5 + r1 * 2],  m0, 6
    pextrb      [r5 + r4],      m0, 7
    lea         r5,             [r5 + r1 * 4]
    pextrb      [r5],           m0, 8
    pextrb      [r5 + r1],      m0, 9
    pextrb      [r5 + r1 * 2],  m0, 10
    pextrb      [r5 + r4],      m0, 11
    lea         r5,             [r5 + r1 * 4]
    pextrb      [r5],           m0, 12
    pextrb      [r5 + r1],      m0, 13
    pextrb      [r5 + r1 * 2],  m0, 14
    pextrb      [r5 + r4],      m0, 15

.quit:
    lea         r3, [r3 + 16]
    add         r0, 16
    dec         r6d
    jnz         .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_27(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_27, 3,7,8
    mov         r2,        r3mp
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       4
    lea         r5,        [r1 * 3]
    mov         r6,        r0
    mova        m7,        [pw_1024]
.loop:
    MODE_9_27 0
    add         r6,        8
    mov         r0,        r6
    add         r2,        8
    dec         r4
    jnz        .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_28(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_28, 3,7,8
    mov         r2,        r3mp
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       4
    lea         r5,        [r1 * 3]
    mov         r6,        r0
    mova        m7,        [pw_1024]
.loop:
    MODE_8_28 0
    add         r6,        8
    mov         r0,        r6
    add         r2,        8
    dec         r4
    jnz        .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_29(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_29, 3,7,8
    mov         r2,        r3mp
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       4
    lea         r5,        [r1 * 3]
    mov         r6,        r0
    mova        m7,        [pw_1024]
.loop:
    MODE_7_29 0
    add         r6,        8
    mov         r0,        r6
    add         r2,        8
    dec         r4
    jnz        .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_30(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_30, 3,7,8
    mov         r2,        r3mp
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       4
    lea         r5,        [r1 * 3]
    mov         r6,        r0
    mova        m7,        [pw_1024]
.loop:
    MODE_6_30 0
    add         r6,        8
    mov         r0,        r6
    add         r2,        8
    dec         r4
    jnz        .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_31(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_31, 3,7,8
    mov         r2,        r3mp
    lea         r3,        [ang_table + 16 * 16]
    mov         r4d,       4
    lea         r5,        [r1 * 3]
    mov         r6,        r0
    mova        m7,        [pw_1024]
.loop:
    MODE_5_31 0
    add         r6,        8
    mov         r0,        r6
    add         r2,        8
    dec         r4
    jnz        .loop
    RET

;-----------------------------------------------------------------------------------------------------------------
; void intraPredAng32_32(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;-----------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_32, 3,7,8
    mov         r2,     r3mp
    lea         r3,     [ang_table + 16 * 16]
    mov         r4d,    4
    lea         r5,     [r1 * 3]
    mov         r6,     r0
    mova        m7,     [pw_1024]
.loop:
    MODE_4_32 0
    add         r6,      8
    mov         r0,     r6
    add         r2,     8
    dec         r4
    jnz        .loop
    RET

;------------------------------------------------------------------------------------------------------------------
; void intraPredAng32_33(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
;------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_ang32_33, 3,7,8
    xchg        r2,    r3mp
    lea         r3,    [ang_table + 16 * 16]
    mov         r4d,   4
    lea         r5,    [r1 * 3]
    mov         r6,    r0
    mova        m7,    [pw_1024]
.loop:
    MODE_3_33 0
    add         r6,    8
    mov         r0,    r6
    add         r2,    8
    dec         r4
    jnz        .loop
    RET

;-----------------------------------------------------------------------------
; void all_angs_pred_4x4(pixel *dest, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool bLuma)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal all_angs_pred_4x4, 6, 6, 8

; mode 2

movh      m0,         [r2 + 2]
movd      [r0],       m0

palignr   m1,         m0,      1
movd      [r0 + 4],   m1

palignr   m1,         m0,      2
movd      [r0 + 8],   m1

psrldq     m0,        3
movd      [r0 + 12],  m0

; mode 3

mova          m0,        [pw_1024]

movh          m1,        [r2 + 1]

palignr       m2,        m1,        1
punpcklbw     m1,        m2

lea           r5,        [ang_table]

pmaddubsw     m5,        m1,        [r5 + 26 * 16]
pmulhrsw      m5,        m0
packuswb      m5,        m5
movd          [r0 + 16], m5

palignr       m2,        m1,        2

mova          m7,        [r5 + 20 * 16]

pmaddubsw     m6,        m2,        m7
pmulhrsw      m6,        m0
packuswb      m6,        m6
movd          [r0 + 20], m6

palignr        m3,        m1,       4

pmaddubsw     m4,        m3,        [r5 + 14 * 16]
pmulhrsw      m4,        m0
packuswb      m4,        m4
movd          [r0 + 24], m4

palignr       m4,        m1,        6

pmaddubsw     m4,        [r5 + 8 * 16]
pmulhrsw      m4,        m0
packuswb      m4,        m4
movd          [r0 + 28], m4

; mode 4

pmaddubsw     m4,        m1,        [r5 + 21 * 16]
pmulhrsw      m4,        m0
packuswb      m4,        m4
movd          [r0 + 32], m4

pmaddubsw     m4,        m2,        [r5 + 10 * 16]
pmulhrsw      m4,        m0
packuswb      m4,        m4
movd          [r0 + 36], m4

pmaddubsw     m4,        m2,        [r5 + 31 * 16]
pmulhrsw      m4,        m0
packuswb      m4,        m4
movd          [r0 + 40], m4

pmaddubsw     m4,        m3,        m7
pmulhrsw      m4,        m0
packuswb      m4,        m4
movd          [r0 + 44], m4

; mode 5

pmaddubsw     m4,        m1,        [r5 + 17 * 16]
pmulhrsw      m4,        m0
packuswb      m4,        m4
movd          [r0 + 48], m4

pmaddubsw     m4,        m2,        [r5 + 2 * 16]
pmulhrsw      m4,        m0
packuswb      m4,        m4
movd          [r0 + 52], m4

pmaddubsw     m4,        m2,        [r5 + 19 * 16]
pmulhrsw      m4,        m0
packuswb      m4,        m4
movd          [r0 + 56], m4

pmaddubsw     m3,        [r5 + 4 * 16]
pmulhrsw      m3,        m0
packuswb      m3,        m3
movd          [r0 + 60], m3

; mode 6

pmaddubsw     m3,        m1,        [r5 + 13 * 16]
pmulhrsw      m3,        m0
packuswb      m3,        m3
movd          [r0 + 64], m3

movd          [r0 + 68], m5

pmaddubsw     m3,        m2,        [r5 + 7 * 16]
pmulhrsw      m3,        m0
packuswb      m3,        m3
movd          [r0 + 72], m3

movd          [r0 + 76], m6

; mode 7

pmaddubsw     m3,        m1,        [r5 + 9 * 16]
pmulhrsw      m3,        m0
packuswb      m3,        m3
movd          [r0 + 80], m3

pmaddubsw     m3,        m1,        [r5 + 18 * 16]
pmulhrsw      m3,        m0
packuswb      m3,        m3
movd          [r0 + 84], m3

pmaddubsw     m3,        m1,        [r5 + 27 * 16]
pmulhrsw      m3,        m0
packuswb      m3,        m3
movd          [r0 + 88], m3

pmaddubsw     m2,        [r5 + 4 * 16]
pmulhrsw      m2,        m0
packuswb      m2,        m2
movd          [r0 + 92], m2

; mode 8

pmaddubsw     m2,         m1,       [r5 + 5 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 96],  m2

pmaddubsw     m2,         m1,       [r5 + 10 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 100], m2

pmaddubsw     m2,         m1,       [r5 + 15 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 104], m2

pmaddubsw     m2,         m1,       m7
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 108], m2

; mode 9

pmaddubsw     m2,         m1,       [r5 + 2 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 112], m2

pmaddubsw     m2,         m1,       [r5 + 4 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 116], m2

pmaddubsw     m2,         m1,       [r5 + 6 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 120], m2

pmaddubsw     m1,         [r5 + 8 * 16]
pmulhrsw      m1,         m0
packuswb      m1,         m1
movd          [r0 + 124], m1

; mode 10

movh         m1,         [r2]
palignr      m2,         m1,        1
pshufd       m3,         m2,        0
movu         [r0 + 128], m3

pxor         m3,          m3

pshufb       m4,          m2,       m3
punpcklbw    m4,          m3

movh         m5,          [r1]

pshufb       m6,          m5,       m3
punpcklbw    m6,          m3

psrldq       m5,          1
punpcklbw    m5,          m3

psubw        m5,          m6
psraw        m5,          1

paddw        m4,          m5

packuswb     m4,          m3

pextrb       [r0 + 128],  m4,    0
pextrb       [r0 + 132],  m4,    1
pextrb       [r0 + 136],  m4,    2
pextrb       [r0 + 140],  m4,    3

; mode 11

palignr       m2,         m1,        1
punpcklbw     m1,         m2

pmaddubsw     m2,         m1,        [r5 + 30 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 144], m2

pmaddubsw     m2,         m1,        [r5 + 28 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 148], m2

pmaddubsw     m2,         m1,        [r5 + 26 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 152], m2

pmaddubsw     m2,         m1,        [r5 + 24 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 156], m2

; mode 12

pmaddubsw     m2,         m1,        [r5 + 27 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 160], m2

pmaddubsw     m2,         m1,        [r5 + 22 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 164], m2

pmaddubsw     m2,         m1,        [r5 + 17 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 168], m2

pmaddubsw     m2,         m1,        [r5 + 12 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 172], m2

; mode 13

pmaddubsw     m2,         m1,        [r5 + 23 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 176], m2

pmaddubsw     m2,         m1,        [r5 + 14 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 180], m2

pmaddubsw     m2,         m1,        [r5 + 5 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 184], m2

pslldq        m2,         m1,         2
pinsrb        m2,         [r1 + 0],   1
pinsrb        m2,         [r1 + 4],   0

pmaddubsw     m3,         m2,         [r5 + 28 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 188], m3

; mode 14

pmaddubsw     m3,         m1,        [r5 + 19 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 192], m3

pmaddubsw     m5,         m1,        [r5 + 6 * 16]
pmulhrsw      m5,         m0
packuswb      m5,         m5
movd          [r0 + 196], m5

pinsrb        m2,         [r1 + 2],  0

pmaddubsw     m3,         m2,        [r5 + 25 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 200], m3

pmaddubsw     m3,         m2,        [r5 + 12 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 204], m3

; mode 15

pmaddubsw     m3,         m1,        [r5 + 15 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 208], m3

pmaddubsw     m3,         m2,        [r5 + 30 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 212], m3

pmaddubsw     m3,         m2,        [r5 + 13 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 216], m3

pslldq        m3,         m2,         2
pinsrb        m3,         [r1 + 2],   1
pinsrb        m3,         [r1 + 4],   0

pmaddubsw     m4,         m3,         [r5 + 28 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 220], m4

; mode 16

pmaddubsw     m4,         m1,        [r5 + 11 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 224], m4

pmaddubsw     m4,         m2,        [r5 + 22 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 228], m4

pmaddubsw     m4,         m2,        [r5 + 1 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 232], m4

pinsrb        m3,         [r1 + 3],  0

pmaddubsw     m3,         [r5 + 12 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 236], m3

; mode 17

movd          [r0 + 240],  m5

pslldq        m1,         2
pinsrb        m1,         [r1 + 1],  0
pinsrb        m1,         [r1 + 0],  1

pmaddubsw     m2,         m1,        [r5 + 12 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 244], m2

pslldq        m1,         2
pinsrb        m1,         [r1 + 2],  0
pinsrb        m1,         [r1 + 1],  1

pmaddubsw     m2,         m1,        [r5 + 18 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 248], m2

pslldq        m1,         2
pinsrb        m1,         [r1 + 4],  0
pinsrb        m1,         [r1 + 2],  1

pmaddubsw     m1,         [r5 + 24 * 16]
pmulhrsw      m1,         m0
packuswb      m1,         m1
movd          [r0 + 252], m1

; mode 18

movh          m1,         [r1]
movd          [r0 + 256], m1

pslldq        m2,         m1,         1
pinsrb        m2,         [r2 + 1],   0
movd          [r0 + 260], m2

pslldq        m3,         m2,         1
pinsrb        m3,         [r2 + 2],   0
movd          [r0 + 264], m3

pslldq        m4,         m3,         1
pinsrb        m4,         [r2 + 3],   0
movd          [r0 + 268], m4

; mode 19

palignr       m4,         m1,        1
punpcklbw     m1,         m4

pmaddubsw     m5,         m1,        [r5 + 6 * 16]
pmulhrsw      m5,         m0
packuswb      m5,         m5
movd          [r0 + 272], m5

pslldq        m2,         m1,         2
pinsrb        m2,         [r2 + 1],   0
pinsrb        m2,         [r2],       1

pmaddubsw     m3,         m2,         [r5 + 12 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 276], m3

pslldq        m3,         m2,         2
pinsrb        m3,         [r2 + 1],   1
pinsrb        m3,         [r2 + 2],   0

pmaddubsw     m4,         m3,         [r5 + 18 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 280], m4

pslldq        m3,         2
pinsrb        m3,         [r2 + 2],   1
pinsrb        m3,         [r2 + 4],   0

pmaddubsw     m3,         [r5 + 24 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 284], m3

; mode 20

pmaddubsw     m3,         m1,        [r5 + 11 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 288], m3

pinsrb        m2,         [r2 + 2],  0

pmaddubsw     m3,         m2,        [r5 + 22 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 292], m3

pmaddubsw     m3,         m2,        [r5 + 1 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 296], m3

pslldq        m3,         m2,        2
pinsrb        m3,         [r2 + 2],  1
pinsrb        m3,         [r2 + 3],  0

pmaddubsw     m4,         m3,        [r5 + 12 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 300], m4

; mode 21

pmaddubsw     m4,         m1,         [r5 + 15 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 304], m4

pmaddubsw     m4,         m2,         [r5 + 30 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 308], m4

pmaddubsw     m4,         m2,         [r5 + 13 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 312], m4

pinsrb        m3,         [r2 + 4],   0

pmaddubsw     m3,         [r5 + 28 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 316], m3

; mode 22

pmaddubsw     m3,         m1,         [r5 + 19 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 320], m3

movd          [r0 + 324], m5

pmaddubsw     m3,         m2,         [r5 + 25 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 328], m3

pmaddubsw     m3,         m2,         [r5 + 12 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 332], m3

; mode 23

pmaddubsw     m3,         m1,         [r5 + 23 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 336], m3

pmaddubsw     m3,         m1,         [r5 + 14 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 340], m3

pmaddubsw     m3,         m1,         [r5 + 5 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 344], m3

pinsrb         m2,        [r2 + 4],   0

pmaddubsw     m2,         [r5 + 28 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 348], m2

; mode 24

pmaddubsw     m2,         m1,         [r5 + 27 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 352], m2

pmaddubsw     m2,         m1,         [r5 + 22 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 356], m2

pmaddubsw     m2,         m1,         [r5 + 17 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 360], m2

pmaddubsw     m2,         m1,         [r5 + 12 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 364], m2

; mode 25

pmaddubsw     m2,         m1,         [r5 + 30 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 368], m2

pmaddubsw     m2,         m1,         [r5 + 28 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 372], m2

pmaddubsw     m2,         m1,         [r5 + 26 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 376], m2

pmaddubsw     m2,         m1,         [r5 + 24 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 380], m2

; mode 26

movh         m1,         [r1 + 1]
pshufd       m2,         m1,        0
movu         [r0 + 384], m2

pxor         m2,         m2

pshufb       m3,          m1,       m2
punpcklbw    m3,          m2

movh         m4,          [r2]

pshufb       m5,          m4,       m2
punpcklbw    m5,          m2

psrldq       m4,          1
punpcklbw    m4,          m2

psubw        m4,          m5
psraw        m4,          1

paddw        m3,          m4

packuswb     m3,          m2

pextrb       [r0 + 384],  m3,    0
pextrb       [r0 + 388],  m3,    1
pextrb       [r0 + 392],  m3,    2
pextrb       [r0 + 396],  m3,    3

; mode 27

palignr       m2,         m1,     1
punpcklbw     m1,         m2

pmaddubsw     m2,         m1,     [r5 + 2 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 400], m2

pmaddubsw     m2,         m1,     [r5 + 4 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 404], m2

pmaddubsw     m2,         m1,     [r5 + 6 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 408], m2

pmaddubsw     m2,         m1,     [r5 + 8 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 412], m2

; mode 28

pmaddubsw     m2,         m1,     [r5 + 5 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 416], m2

pmaddubsw     m2,         m1,     [r5 + 10 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 420], m2

pmaddubsw     m2,         m1,     [r5 + 15 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 424], m2

pmaddubsw     m2,         m1,     m7
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 428], m2

; mode 29

pmaddubsw     m2,         m1,     [r5 + 9 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 432], m2

pmaddubsw     m2,         m1,     [r5 + 18 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 436], m2

pmaddubsw     m2,         m1,     [r5 + 27 * 16]
pmulhrsw      m2,         m0
packuswb      m2,         m2
movd          [r0 + 440], m2

palignr       m2,         m1,     2

pmaddubsw     m3,         m2,     [r5 + 4 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 444], m3

; mode 30

pmaddubsw     m3,         m1,     [r5 + 13 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 448], m3

pmaddubsw     m6,         m1,     [r5 + 26 * 16]
pmulhrsw      m6,         m0
packuswb      m6,         m6
movd          [r0 + 452], m6

pmaddubsw     m3,         m2,     [r5 + 7 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 456], m3

pmaddubsw     m5,         m2,     m7
pmulhrsw      m5,         m0
packuswb      m5,         m5
movd          [r0 + 460], m5

; mode 31

pmaddubsw     m3,         m1,     [r5 + 17 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 464], m3

pmaddubsw     m3,         m2,     [r5 + 2 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 468], m3

pmaddubsw     m3,         m2,     [r5 + 19 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 472], m3

palignr       m3,         m2,     2

pmaddubsw     m4,         m3,     [r5 + 4 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 476], m4

; mode 32

pmaddubsw     m4,         m1,     [r5 + 21 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 480], m4

pmaddubsw     m4,         m2,     [r5 + 10 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 484], m4

pmaddubsw     m4,         m2,     [r5 + 31 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 488], m4

pmaddubsw     m4,         m3,     m7
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 492], m4

; mode 33

movd          [r0 + 496], m6

movd          [r0 + 500], m5

pmaddubsw     m4,         m3,         [r5 + 14 * 16]
pmulhrsw      m4,         m0
packuswb      m4,         m4
movd          [r0 + 504], m4

psrldq        m3,         2

pmaddubsw     m3,         [r5 + 8 * 16]
pmulhrsw      m3,         m0
packuswb      m3,         m3
movd          [r0 + 508], m3

; mode 34

movh      m0,             [r1 + 2]
movd      [r0 + 512],     m0

palignr   m1,             m0,      1
movd      [r0 + 516],     m1

palignr   m1,             m0,      2
movd      [r0 + 520],     m1

palignr   m1,             m0,      3
movd      [r0 + 524],     m1

RET

;-----------------------------------------------------------------------------
; void all_angs_pred_8x8(pixel *dest, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool bLuma)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal all_angs_pred_8x8, 6, 6, 8, dest, above0, left0, above1, left1, bLuma

; mode 2

movu          m0,         [r4 + 2]

palignr      m1,          m0,          1
punpcklqdq   m2,          m0,          m1
movu         [r0],        m2

palignr      m1,          m0,          2
palignr      m2,          m0,          3
punpcklqdq   m1,          m2
movu         [r0 + 16],   m1

palignr      m1,          m0,          4
palignr      m2,          m0,          5
punpcklqdq   m1,          m2
movu         [r0 + 32],   m1

palignr      m1,          m0,          6
palignr      m2,          m0,          7
punpcklqdq   m1,          m2
movu         [r0 + 48],   m1

; mode 3 [row 0, 1]

mova          m7,         [pw_1024]
lea           r5,         [ang_table]

movu          m0,         [r2 + 1]

palignr       m1,         m0,               1
palignr       m2,         m0,               2

punpcklbw     m3,         m0,               m1
pmaddubsw     m4,         m3,               [r5 + 26 * 16]
pmulhrsw      m4,         m7

punpcklbw     m1,         m2
pmaddubsw     m5,         m1,               [r5 + 20 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5

movu          [r0 + 64],  m4

; mode 6 [row 1]

movh          [r0 + 264], m4

; mode 6 [row 3]

movhps        [r0 + 280], m4

; mode 4 [row 0, 1]

pmaddubsw     m4,         m3,               [r5 + 21 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m1,               [r5 + 10 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 128], m4

; mode 5 [row 0, 1]

pmaddubsw     m4,         m3,               [r5 + 17 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m1,               [r5 + 2 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 192], m4

; mode 6 [row 0]

pmaddubsw     m4,         m3,               [r5 + 13 * 16]
pmulhrsw      m4,         m7

pxor          m5,         m5

packuswb      m4,         m5
movh          [r0 + 256], m4

; mode 7 [row 0, 1]

pmaddubsw     m4,         m3,               [r5 + 9 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m3,               [r5 + 18 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 320], m4

; mode 8 [row 0, 1]

pmaddubsw     m4,         m3,               [r5 + 5 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m3,               [r5 + 10 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 384], m4

; mode 8 [row 2, 3]

pmaddubsw     m4,         m3,               [r5 + 15 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m3,               [r5 + 20 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 400], m4

; mode 8 [row 4, 5]

pmaddubsw     m4,         m3,               [r5 + 25 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m3,               [r5 + 30 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 416], m4

; mode 8 [row 6, 7]

pmaddubsw     m4,         m1,               [r5 + 3 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m1,               [r5 + 8 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 432], m4

; mode 9 [row 0, 1]

pmaddubsw     m4,         m3,               [r5 + 2 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m3,               [r5 + 4 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 448], m4

; mode 9 [row 2, 3]

pmaddubsw     m4,         m3,               [r5 + 6 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m3,               [r5 + 8 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 464], m4

; mode 9 [row 4, 5]

pmaddubsw     m4,         m3,               [r5 + 10 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m3,               [r5 + 12 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 480], m4

; mode 9 [row 6, 7]

pmaddubsw     m4,         m3,               [r5 + 14 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m3,               [r5 + 16 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 496], m4

; mode 7 [row 2, 3]

pmaddubsw     m4,         m3,               [r5 + 27 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m1,               [r5 + 4 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 336], m4

; mode 7 [row 4, 5]

pmaddubsw     m4,         m1,               [r5 + 13 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m1,               [r5 + 22 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 352], m4

; mode 6 [row 2]

pmaddubsw     m4,         m1,               [r5 + 7 * 16]
pmulhrsw      m4,         m7

pxor           m5,         m5

packuswb      m4,         m5
movh          [r0 + 272], m4

; mode 3 [row 2, 3]

palignr       m1,         m0,               3
palignr       m3,         m0,               4

punpcklbw     m2,         m1
pmaddubsw     m5,         m2,               [r5 + 14 * 16]
pmulhrsw      m5,         m7

punpcklbw     m1,         m3
pmaddubsw     m6,         m1,               [r5 + 8 * 16]
pmulhrsw      m6,         m7

packuswb      m5,         m6
movu          [r0 + 80],  m5

; mode 6 [row 7]

movhps        [r0 + 312], m5

; mode 6 [row 5]

movh          [r0 + 296], m5

; mode 4 [calculate and store row 4, 5]

pmaddubsw     m4,         m1,               [r5 + 9 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m1,               [r5 + 30 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 160], m4

; mode 5 [row 4, 5]

pmaddubsw     m4,         m2,               [r5 + 21 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m1,               [r5 + 6 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 224], m4

; mode 6 [row 4, 5]

pmaddubsw     m5,         m2,               [r5 + 1 * 16]
pmulhrsw      m5,         m7

pxor           m6,        m6

packuswb      m5,         m6
movh          [r0 + 288], m5

; mode 6 [row 6, 7]

pmaddubsw     m5,         m2,               [r5 + 27 * 16]
pmulhrsw      m5,         m7

pxor          m6,         m6

packuswb      m5,         m6
movh          [r0 + 304], m5

; mode 5 [calculate row 6]

pmaddubsw     m6,         m1,               [r5 + 23 * 16]
pmulhrsw      m6,         m7

; mode 3 [row 4, 5]

palignr       m1,         m0,               5

punpcklbw     m3,         m1
pmaddubsw     m4,         m3,               [r5 + 2 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m3,               [r5 + 28 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 96],  m4

; mode 4 [calculate row 7]

pmaddubsw     m5,         m3,               [r5 + 19 * 16]
pmulhrsw      m5,         m7

; mode 5 [calculate row 6]

pmaddubsw     m4,         m3,               [r5 + 8 * 16]
pmulhrsw      m4,         m7

packuswb      m6,         m4
movu          [r0 + 240], m6

; mode 3 [row 6, 7]

palignr       m2,         m0,               6
palignr       m3,         m0,               7

punpcklbw     m1,         m2
pmaddubsw     m4,         m1,               [r5 + 22 * 16]
pmulhrsw      m4,         m7

punpcklbw     m2,         m3
pmaddubsw     m2,         [r5 + 16 * 16]
pmulhrsw      m2,         m7

packuswb      m4,         m2
movu          [r0 + 112], m4

; mode 4 [calculate row 7]

pmaddubsw     m2,         m1,               [r5 + 8 * 16]
pmulhrsw      m2,         m7

; mode 4 [store row 6 and 7]

packuswb      m5,         m2
movu          [r0 + 176], m5

; mode 4 [row 2, 3]

palignr       m1,         m0,               1
palignr       m2,         m0,               2
palignr       m3,         m0,               3

punpcklbw     m1,         m2
pmaddubsw     m4,         m1,               [r5 + 31 * 16]
pmulhrsw      m4,         m7

punpcklbw     m2,         m3
pmaddubsw     m5,         m2,               [r5 + 20 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 144], m4

; mode 5 [row 2, 3]

pmaddubsw     m4,         m1,               [r5 + 19 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m2,               [r5 + 4 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 208], m4

; mode 7 [row 6, 7]

pmaddubsw     m4,         m1,               [r5 + 31 * 16]
pmulhrsw      m4,         m7

pmaddubsw     m5,         m2,               [r5 + 8 * 16]
pmulhrsw      m5,         m7

packuswb      m4,         m5
movu          [r0 + 368], m4

; mode 10

pshufb       m1,          m0,          [tab_Si]
movu         [r0 + 512],  m1
movu         [r0 + 528],  m1
movu         [r0 + 544],  m1
movu         [r0 + 560],  m1

pxor         m0,          m0

pshufb       m1,          m1,          m0
punpcklbw    m1,          m0

movu         m2,          [r1]

pshufb       m3,          m2,          m0
punpcklbw    m3,          m0

psrldq       m4,          m2,          1
punpcklbw    m4,          m0

movu         m2,          [r1 + 9]
punpcklbw    m2,          m0

psubw        m4,          m3
psubw        m2,          m3

psraw        m4,          1
psraw        m2,          1

paddw        m4,          m1
paddw        m2,          m1

packuswb     m4,          m2

pextrb       [r0 + 512],  m4,          0
pextrb       [r0 + 520],  m4,          1
pextrb       [r0 + 528],  m4,          2
pextrb       [r0 + 536],  m4,          3
pextrb       [r0 + 544],  m4,          4
pextrb       [r0 + 552],  m4,          5
pextrb       [r0 + 560],  m4,          6
pextrb       [r0 + 568],  m4,          7

; mode 11 [row 0, 1]

movu         m0,         [r2]
palignr      m1,         m0,          1
punpcklbw    m2,         m0,          m1

pmaddubsw    m3,         m2,          [r5 + 30 * 16]
pmulhrsw     m3,         m7

pmaddubsw    m4,         m2,          [r5 + 28 * 16]
pmulhrsw     m4,         m7

packuswb     m3,         m4
movu         [r0 + 576], m3

; mode 11 [row 2, 3]

pmaddubsw    m3,         m2,          [r5 + 26 * 16]
pmulhrsw     m3,         m7

pmaddubsw    m4,         m2,          [r5 + 24 * 16]
pmulhrsw     m4,         m7

packuswb     m3,         m4
movu         [r0 + 592], m3

; mode 11 [row 4, 5]

pmaddubsw    m3,         m2,          [r5 + 22 * 16]
pmulhrsw     m3,         m7

pmaddubsw    m4,         m2,          [r5 + 20 * 16]
pmulhrsw     m4,         m7

packuswb     m5,         m3,         m4
movu         [r0 + 608], m5

; mode 12 [row 0, 1]

pmaddubsw    m4,         m2,          [r5 + 27 * 16]
pmulhrsw     m4,         m7

packuswb     m4,         m3
movu         [r0 + 640], m4

; mode 11 [row 6, 7]

pmaddubsw    m3,         m2,          [r5 + 18 * 16]
pmulhrsw     m3,         m7

pmaddubsw    m4,         m2,          [r5 + 16 * 16]
pmulhrsw     m4,         m7

packuswb     m3,         m4
movu         [r0 + 624], m3

; mode 12 [row 2, 3]

pmaddubsw    m3,         m2,          [r5 + 17 * 16]
pmulhrsw     m3,         m7

pmaddubsw    m4,         m2,          [r5 + 12 * 16]
pmulhrsw     m4,         m7

packuswb     m3,         m4
movu         [r0 + 656], m3

; mode 12 [row 4, 5]

pmaddubsw    m3,         m2,          [r5 + 7 * 16]
pmulhrsw     m3,         m7

pmaddubsw    m4,         m2,          [r5 + 2 * 16]
pmulhrsw     m4,         m7

packuswb     m3,         m4
movu         [r0 + 672], m3

; mode 12 [row 6, 7]

pslldq       m3,         m2,          2
pinsrb       m3,         [r1 + 0],    1
pinsrb       m3,         [r1 + 6],    0

pmaddubsw    m4,         m3,          [r5 + 29 * 16]
pmulhrsw     m4,         m7

pmaddubsw    m5,         m3,          [r5 + 24 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 688], m4

; mode 13 [row 0, 1]

pmaddubsw    m4,         m2,          [r5 + 23 * 16]
pmulhrsw     m4,         m7

pmaddubsw    m5,         m2,          [r5 + 14 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 704], m4

; mode 13 [row 2, 3]

pmaddubsw    m4,         m2,          [r5 + 5 * 16]
pmulhrsw     m4,         m7

pinsrb       m3,         [r1 + 4],    0
pmaddubsw    m5,         m3,          [r5 + 28 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 720], m4

; mode 13 [row 4, 5]

pmaddubsw    m4,         m3,          [r5 + 19 * 16]
pmulhrsw     m4,         m7

pmaddubsw    m5,         m3,          [r5 + 10 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 736], m4

; mode 13 [row 6, 7]

pmaddubsw    m4,         m3,          [r5 + 1 * 16]
pmulhrsw     m4,         m7

pslldq       m5,         m3,          2
pinsrb       m5,         [r1 + 4],    1
pinsrb       m5,         [r1 + 7],    0

pmaddubsw    m5,         [r5 + 24 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 752], m4

; mode 14 [row 0, 1]

pmaddubsw    m4,         m2,          [r5 + 19 * 16]
pmulhrsw     m4,         m7

pmaddubsw    m5,         m2,          [r5 + 6 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 768], m4

; mode 14 [row 2, 3]

pinsrb       m3,         [r1 + 2],    0

pmaddubsw    m4,         m3,          [r5 + 25 * 16]
pmulhrsw     m4,         m7

pmaddubsw    m5,         m3,          [r5 + 12 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 784], m4

; mode 14 [row 4, 5]

pslldq       m1,         m3,          2
pinsrb       m1,         [r1 + 2],    1
pinsrb       m1,         [r1 + 5],    0

pmaddubsw    m4,         m1,          [r5 + 31 * 16]
pmulhrsw     m4,         m7

pmaddubsw    m5,         m1,          [r5 + 18 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 800], m4

; mode 14 [row 6, 7]

pmaddubsw    m4,         m1,          [r5 + 5 * 16]
pmulhrsw     m4,         m7

pslldq       m1,         2
pinsrb       m1,         [r1 + 5],    1
pinsrb       m1,         [r1 + 7],    0

pmaddubsw    m5,         m1,          [r5 + 24 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 816], m4

; mode 15 [row 0, 1]

pmaddubsw    m4,         m2,          [r5 + 15 * 16]
pmulhrsw     m4,         m7

pmaddubsw    m5,         m3,          [r5 + 30 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 832], m4

; mode 15 [row 2, 3]

pmaddubsw    m4,         m3,          [r5 + 13 * 16]
pmulhrsw     m4,         m7

pslldq       m1,         m3,          2
pinsrb       m1,         [r1 + 2],    1
pinsrb       m1,         [r1 + 4],    0

pmaddubsw    m5,         m1,          [r5 + 28 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 848], m4

; mode 15 [row 4, 5]

pmaddubsw    m4,         m1,          [r5 + 11 * 16]
pmulhrsw     m4,         m7

pslldq       m1,         2
pinsrb       m1,         [r1 + 4],    1
pinsrb       m1,         [r1 + 6],    0

pmaddubsw    m5,         m1,          [r5 + 26 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 864], m4

; mode 15 [row 6, 7]

pmaddubsw    m4,         m1,          [r5 + 9 * 16]
pmulhrsw     m4,         m7

pslldq       m1,         2
pinsrb       m1,         [r1 + 6],    1
pinsrb       m1,         [r1 + 8],    0

pmaddubsw    m1,          [r5 + 24 * 16]
pmulhrsw     m1,         m7

packuswb     m4,         m1
movu         [r0 + 880], m4

; mode 16 [row 0, 1]

pmaddubsw    m4,         m2,          [r5 + 11 * 16]
pmulhrsw     m4,         m7

pmaddubsw    m5,         m3,          [r5 + 22 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 896], m4

; mode 16 [row 2, 3]

pmaddubsw    m4,         m3,          [r5 + 1 * 16]
pmulhrsw     m4,         m7

pslldq       m3,         2
pinsrb       m3,         [r1 + 2],    1
pinsrb       m3,         [r1 + 3],    0

pmaddubsw    m5,         m3,          [r5 + 12 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 912], m4

; mode 16 [row 4, 5]

pslldq       m3,         2
pinsrb       m3,         [r1 + 3],    1
pinsrb       m3,         [r1 + 5],    0

pmaddubsw    m4,         m3,          [r5 + 23 * 16]
pmulhrsw     m4,         m7

pmaddubsw    m5,         m3,          [r5 + 2 * 16]
pmulhrsw     m5,         m7

packuswb     m4,         m5
movu         [r0 + 928], m4

; mode 16 [row 6, 7]

pslldq       m3,         2
pinsrb       m3,         [r1 + 5],    1
pinsrb       m3,         [r1 + 6],    0

pmaddubsw    m4,         m3,          [r5 + 13 * 16]
pmulhrsw     m4,         m7

pslldq       m3,         2
pinsrb       m3,         [r1 + 6],    1
pinsrb       m3,         [r1 + 8],    0

pmaddubsw    m3,         [r5 + 24 * 16]
pmulhrsw     m3,         m7

packuswb     m4,         m3
movu         [r0 + 944], m4

; mode 17 [row 0, 1]

pmaddubsw    m4,         m2,          [r5 + 6 * 16]
pmulhrsw     m4,         m7

pslldq       m2,         2
pinsrb       m2,         [r1 + 0],    1
pinsrb       m2,         [r1 + 1],    0

pmaddubsw    m3,         m2,          [r5 + 12 * 16]
pmulhrsw     m3,         m7

packuswb     m4,         m3
movu         [r0 + 960], m4

; mode 17 [row 2, 3]

pslldq       m2,         2
pinsrb       m2,         [r1 + 1],    1
pinsrb       m2,         [r1 + 2],    0

pmaddubsw    m4,         m2,          [r5 + 18 * 16]
pmulhrsw     m4,         m7

pslldq       m2,         2
pinsrb       m2,         [r1 + 2],    1
pinsrb       m2,         [r1 + 4],    0

pmaddubsw    m3,         m2,          [r5 + 24 * 16]
pmulhrsw     m3,         m7

packuswb     m4,         m3
movu         [r0 + 976], m4

; mode 17 [row 4, 5]

pslldq       m2,         2
pinsrb       m2,         [r1 + 4],    1
pinsrb       m2,         [r1 + 5],    0

pmaddubsw    m4,         m2,          [r5 + 30 * 16]
pmulhrsw     m4,         m7

pmaddubsw    m3,         m2,          [r5 + 4 * 16]
pmulhrsw     m3,         m7

packuswb     m4,         m3
movu         [r0 + 992], m4

; mode 17 [row 6, 7]

pslldq       m2,          2
pinsrb       m2,          [r1 + 5],    1
pinsrb       m2,          [r1 + 6],    0

pmaddubsw    m4,          m2,          [r5 + 10 * 16]
pmulhrsw     m4,          m7

pslldq       m2,          2
pinsrb       m2,          [r1 + 6],    1
pinsrb       m2,          [r1 + 7],    0

pmaddubsw    m3,          m2,          [r5 + 16 * 16]
pmulhrsw     m3,          m7

packuswb     m4,          m3
movu         [r0 + 1008], m4

; mode 18 [row 0, 1, 2, 3, 4, 5, 6, 7]

movh          m1,          [r3]
movh          [r0 + 1024], m1

pslldq        m2,          m1,         1
pinsrb        m2,          [r4 + 1],   0
movh          [r0 + 1032], m2

pslldq        m2,          1
pinsrb        m2,          [r4 + 2],   0
movh          [r0 + 1040], m2

pslldq        m2,          1
pinsrb        m2,          [r4 + 3],   0
movh          [r0 + 1048], m2

pslldq        m2,          1
pinsrb        m2,          [r4 + 4],   0
movh          [r0 + 1056], m2

pslldq        m2,          1
pinsrb        m2,          [r4 + 5],   0
movh          [r0 + 1064], m2

pslldq        m2,          1
pinsrb        m2,          [r4 + 6],   0
movh          [r0 + 1072], m2

pslldq        m2,          1
pinsrb        m2,          [r4 + 7],   0
movh          [r0 + 1080], m2

; mode 19 [row 0, 1]

movu         m0,          [r1]
palignr      m1,          m0,          1
punpcklbw    m0,          m1

pmaddubsw    m1,          m0,          [r5 + 6 * 16]
pmulhrsw     m1,          m7

pslldq       m2,          m0,          2
pinsrb       m2,          [r2 + 0],    1
pinsrb       m2,          [r2 + 1],    0

pmaddubsw    m3,          m2,          [r5 + 12 * 16]
pmulhrsw     m3,          m7

packuswb     m1,          m3
movu         [r0 + 1088], m1

; mode 19 [row 2, 3]

pslldq       m2,          2
pinsrb       m2,          [r2 + 1],    1
pinsrb       m2,          [r2 + 2],    0

pmaddubsw    m4,          m2,          [r5 + 18 * 16]
pmulhrsw     m4,          m7

pslldq       m2,          2
pinsrb       m2,          [r2 + 2],    1
pinsrb       m2,          [r2 + 4],    0

pmaddubsw    m5,          m2,          [r5 + 24 * 16]
pmulhrsw     m5,          m7

packuswb     m4,          m5
movu         [r0 + 1104], m4

; mode 19 [row 4, 5]

pslldq       m2,          2
pinsrb       m2,          [r2 + 4],    1
pinsrb       m2,          [r2 + 5],    0

pmaddubsw    m4,          m2,          [r5 + 30 * 16]
pmulhrsw     m4,          m7

pmaddubsw    m5,          m2,          [r5 + 4 * 16]
pmulhrsw     m5,          m7

packuswb     m4,          m5
movu         [r0 + 1120], m4

; mode 19 [row 6, 7]

pslldq       m2,          2
pinsrb       m2,          [r2 + 5],    1
pinsrb       m2,          [r2 + 6],    0

pmaddubsw    m4,          m2,          [r5 + 10 * 16]
pmulhrsw     m4,          m7

pslldq       m2,          2
pinsrb       m2,          [r2 + 6],    1
pinsrb       m2,          [r2 + 7],    0

pmaddubsw    m2,          [r5 + 16 * 16]
pmulhrsw     m2,          m7

packuswb     m4,          m2
movu         [r0 + 1136], m4

; mode 20 [row 0, 1]

pmaddubsw    m3,          m0,          [r5 + 11 * 16]
pmulhrsw     m3,          m7

pslldq       m1,          m0,          2
pinsrb       m1,          [r2 + 0],    1
pinsrb       m1,          [r2 + 2],    0

pmaddubsw    m4,          m1,          [r5 + 22 * 16]
pmulhrsw     m4,          m7

packuswb     m3,          m4
movu         [r0 + 1152], m3

; mode 20 [row 2, 3]

pmaddubsw    m3,          m1,          [r5 + 1 * 16]
pmulhrsw     m3,          m7

pslldq       m2,          m1,          2
pinsrb       m2,          [r2 + 2],    1
pinsrb       m2,          [r2 + 3],    0

pmaddubsw    m4,          m2,          [r5 + 12 * 16]
pmulhrsw     m4,          m7

packuswb     m3,          m4
movu         [r0 + 1168], m3

; mode 20 [row 4, 5]

pslldq       m2,          2
pinsrb       m2,          [r2 + 3],    1
pinsrb       m2,          [r2 + 5],    0

pmaddubsw    m3,          m2,          [r5 + 23 * 16]
pmulhrsw     m3,          m7

pmaddubsw    m4,          m2,          [r5 + 2 * 16]
pmulhrsw     m4,          m7

packuswb     m3,          m4
movu         [r0 + 1184], m3

; mode 20 [row 6, 7]

pslldq       m2,          2
pinsrb       m2,          [r2 + 5],    1
pinsrb       m2,          [r2 + 6],    0

pmaddubsw    m3,          m2,          [r5 + 13 * 16]
pmulhrsw     m3,          m7

pslldq       m2,          2
pinsrb       m2,          [r2 + 6],    1
pinsrb       m2,          [r2 + 8],    0

pmaddubsw    m4,          m2,          [r5 + 24 * 16]
pmulhrsw     m4,          m7

packuswb     m3,          m4
movu         [r0 + 1200], m3

; mode 21 [row 0, 1]

pmaddubsw    m2,          m0,          [r5 + 15 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m1,          [r5 + 30 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1216], m2

; mode 21 [row 2, 3]

pmaddubsw    m2,          m1,          [r5 + 13 * 16]
pmulhrsw     m2,          m7

pslldq       m3,          m1,          2
pinsrb       m3,          [r2 + 2],    1
pinsrb       m3,          [r2 + 4],    0

pmaddubsw    m4,          m3,          [r5 + 28 * 16]
pmulhrsw     m4,          m7

packuswb     m2,          m4
movu         [r0 + 1232], m2

; mode 21 [row 4, 5]

pmaddubsw    m2,          m3,          [r5 + 11 * 16]
pmulhrsw     m2,          m7

pslldq       m3,          2
pinsrb       m3,          [r2 + 4],    1
pinsrb       m3,          [r2 + 6],    0

pmaddubsw    m4,          m3,          [r5 + 26 * 16]
pmulhrsw     m4,          m7

packuswb     m2,          m4
movu         [r0 + 1248], m2

; mode 21 [row 6, 7]

pmaddubsw    m2,          m3,          [r5 + 9 * 16]
pmulhrsw     m2,          m7

pslldq       m3,          2
pinsrb       m3,          [r2 + 6],    1
pinsrb       m3,          [r2 + 8],    0

pmaddubsw    m4,          m3,          [r5 + 24 * 16]
pmulhrsw     m4,          m7

packuswb     m2,          m4
movu         [r0 + 1264], m2

; mode 22 [row 0, 1]

pmaddubsw    m2,          m0,          [r5 + 19 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m4,          m0,          [r5 + 6 * 16]
pmulhrsw     m4,          m7

packuswb     m2,          m4
movu         [r0 + 1280], m2

; mode 22 [row 2, 3]

pmaddubsw    m2,          m1,          [r5 + 25 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m1,          [r5 + 12 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1296], m2

; mode 22 [row 4, 5]

pslldq       m1,          2
pinsrb       m1,          [r2 + 5],    0
pinsrb       m1,          [r2 + 2],    1

pmaddubsw    m2,          m1,          [r5 + 31 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m1,          [r5 + 18 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1312], m2

; mode 22 [row 6, 7]

pmaddubsw    m2,          m1,          [r5 + 5 * 16]
pmulhrsw     m2,          m7

pslldq       m1,          2
pinsrb       m1,          [r2 + 5],    1
pinsrb       m1,          [r2 + 7],    0

pmaddubsw    m1,          [r5 + 24 * 16]
pmulhrsw     m1,          m7

packuswb     m2,          m1
movu         [r0 + 1328], m2

; mode 23 [row 0, 1]

pmaddubsw    m2,          m0,          [r5 + 23 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m0,          [r5 + 14 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1344], m2

; mode 23 [row 2, 3]

pmaddubsw    m2,          m0,          [r5 + 5 * 16]
pmulhrsw     m2,          m7

pslldq       m1,          m0,          2
pinsrb       m1,          [r2 + 0],    1
pinsrb       m1,          [r2 + 4],    0

pmaddubsw    m3,          m1,          [r5 + 28 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1360], m2

; mode 23 [row 4, 5]

pmaddubsw    m2,          m1,          [r5 + 19 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m1,          [r5 + 10 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1376], m2

; mode 23 [row 6, 7]

pmaddubsw    m2,          m1,          [r5 + 1 * 16]
pmulhrsw     m2,          m7

pslldq       m3,          m1,          2
pinsrb       m3,          [r2 + 4],    1
pinsrb       m3,          [r2 + 7],    0

pmaddubsw    m3,          [r5 + 24 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1392], m2

; mode 24 [row 0, 1]

pmaddubsw    m2,          m0,          [r5 + 27 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m5,          m0,          [r5 + 22 * 16]
pmulhrsw     m5,          m7

packuswb     m2,          m5
movu         [r0 + 1408], m2

; mode 24 [row 2, 3]

pmaddubsw    m2,          m0,          [r5 + 17 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m0,          [r5 + 12 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1424], m2

; mode 24 [row 4, 5]

pmaddubsw    m2,          m0,          [r5 + 7 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m0,          [r5 + 2 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1440], m2

; mode 24 [row 6, 7]

pinsrb       m1,          [r2 + 6],    0

pmaddubsw    m2,          m1,          [r5 + 29 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m1,          [r5 + 24 * 16]
pmulhrsw     m1,          m7

packuswb     m2,          m1
movu         [r0 + 1456], m2

; mode 25 [row 0, 1]

pmaddubsw    m2,          m0,          [r5 + 30 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m1,          m0,          [r5 + 28 * 16]
pmulhrsw     m1,          m7

packuswb     m2,          m1
movu         [r0 + 1472], m2

; mode 25 [row 2, 3]

pmaddubsw    m2,          m0,          [r5 + 26 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m1,          m0,          [r5 + 24 * 16]
pmulhrsw     m1,          m7

packuswb     m2,          m1
movu         [r0 + 1488], m2

; mode 25 [row 4, 5]

pmaddubsw    m1,          m0,          [r5 + 20 * 16]
pmulhrsw     m1,          m7

packuswb     m5,          m1
movu         [r0 + 1504], m5

; mode 25 [row 6, 7]

pmaddubsw    m2,          m0,          [r5 + 18 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m1,          m0,          [r5 + 16 * 16]
pmulhrsw     m1,          m7

packuswb     m2,          m1
movu         [r0 + 1520], m2

; mode 26

movu         m0,          [r1 + 1]

pshufb       m1,          m0,          [tab_Si]
movu         [r0 + 1536], m1
movu         [r0 + 1552], m1
movu         [r0 + 1568], m1
movu         [r0 + 1584], m1

pxor         m5,          m5

pshufb       m1,          m1,          m5
punpcklbw    m1,          m5

movu         m2,          [r2]

pshufb       m3,          m2,          m5
punpcklbw    m3,          m5

psrldq       m4,          m2,          1
punpcklbw    m4,          m5

movu         m2,          [r2 + 9]
punpcklbw    m2,          m5

psubw        m4,          m3
psubw        m2,          m3

psraw        m4,          1
psraw        m2,          1

paddw        m4,          m1
paddw        m2,          m1

packuswb     m4,          m2

pextrb       [r0 + 1536], m4,          0
pextrb       [r0 + 1544], m4,          1
pextrb       [r0 + 1552], m4,          2
pextrb       [r0 + 1560], m4,          3
pextrb       [r0 + 1568], m4,          4
pextrb       [r0 + 1576], m4,          5
pextrb       [r0 + 1584], m4,          6
pextrb       [r0 + 1592], m4,          7

; mode 27 [row 0, 1]

palignr      m6,          m0,          1
punpcklbw    m4,          m0,          m6

pmaddubsw    m1,          m4,          [r5 + 2 * 16]
pmulhrsw     m1,          m7

pmaddubsw    m2,          m4,          [r5 + 4 * 16]
pmulhrsw     m2,          m7

packuswb     m1,          m2
movu         [r0 + 1600], m1

; mode 27 [row 2, 3]

pmaddubsw    m1,          m4,          [r5 + 6 * 16]
pmulhrsw     m1,          m7

pmaddubsw    m2,          m4,          [r5 + 8 * 16]
pmulhrsw     m2,          m7

packuswb     m1,          m2
movu         [r0 + 1616], m1

; mode 27 [row 4, 5]

pmaddubsw    m3,          m4,          [r5 + 10 * 16]
pmulhrsw     m3,          m7

pmaddubsw    m2,          m4,          [r5 + 12 * 16]
pmulhrsw     m2,          m7

packuswb     m1,          m3,          m2
movu         [r0 + 1632], m1

; mode 27 [row 6, 7]

pmaddubsw    m1,          m4,          [r5 + 14 * 16]
pmulhrsw     m1,          m7

pmaddubsw    m2,          m4,          [r5 + 16 * 16]
pmulhrsw     m2,          m7

packuswb     m1,          m2
movu         [r0 + 1648], m1

; mode 28 [row 0, 1]

pmaddubsw    m1,          m4,          [r5 + 5 * 16]
pmulhrsw     m1,          m7

packuswb     m1,          m3
movu         [r0 + 1664], m1

; mode 28 [row 2, 3]

pmaddubsw    m1,          m4,          [r5 + 15 * 16]
pmulhrsw     m1,          m7

pmaddubsw    m2,          m4,          [r5 + 20 * 16]
pmulhrsw     m2,          m7

packuswb     m1,          m2
movu         [r0 + 1680], m1

; mode 28 [row 4, 5]

pmaddubsw    m1,          m4,          [r5 + 25 * 16]
pmulhrsw     m1,          m7

pmaddubsw    m2,          m4,          [r5 + 30 * 16]
pmulhrsw     m2,          m7

packuswb     m1,          m2
movu         [r0 + 1696], m1

; mode 28 [row 6, 7]

palignr      m1,          m0,          2
punpcklbw    m5,          m6,          m1

pmaddubsw    m2,          m5,          [r5 + 3 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m5,          [r5 + 8 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1712], m2

; mode 29 [row 0, 1]

pmaddubsw    m2,          m4,          [r5 + 9 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m4,          [r5 + 18 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1728], m2

; mode 29 [row 2, 3]

pmaddubsw    m2,          m4,          [r5 + 27 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m5,          [r5 + 4 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1744], m2

; mode 29 [row 4, 5]

pmaddubsw    m2,          m5,          [r5 + 13 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m5,          [r5 + 22 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1760], m2

; mode 29 [row 6, 7]

pmaddubsw    m2,          m5,          [r5 + 31 * 16]
pmulhrsw     m2,          m7

palignr      m6,          m0,          3
punpcklbw    m1,          m6

pmaddubsw    m3,          m1,          [r5 + 8 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1776], m2

; mode 32 [row 2]

movh         [r0 + 1936], m2

; mode 30 [row 0, 1]

pmaddubsw    m2,          m4,          [r5 + 13 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m4,          [r5 + 26 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1792], m2

; mode 30 [row 2, 3]

pmaddubsw    m2,          m5,          [r5 + 7 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m5,          [r5 + 20 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1808], m2

; mode 33 [row 1]

movhps       [r0 + 1992], m2

; mode 30 [row 4, 5]

pmaddubsw    m2,          m1,          [r5 + 1 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m1,          [r5 + 14 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1824], m2

; mode 33 [row 2]

movhps       [r0 + 2000], m2

; mode 30 [row 6, 7]

pmaddubsw    m2,          m1,          [r5 + 27 * 16]
pmulhrsw     m2,          m7

psrldq       m0,          4
punpcklbw    m6,          m0

pmaddubsw    m3,          m6,          [r5 + 8 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1840], m2

; mode 33 [row 3]

movhps       [r0 + 2008], m2

; mode 31 [row 0, 1]

pmaddubsw    m2,          m4,          [r5 + 17 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m5,          [r5 + 2 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1856], m2

; mode 31 [row 2, 3]

pmaddubsw    m2,          m5,          [r5 + 19 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m1,          [r5 + 4 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1872], m2

; mode 31 [row 4, 5]

pmaddubsw    m2,          m1,          [r5 + 21 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m6,          [r5 + 6 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1888], m2

; mode 31 [row 6, 7]

pmaddubsw    m2,          m6,          [r5 + 23 * 16]
pmulhrsw     m2,          m7

movu         m3,          [r1 + 6]
punpcklbw    m0,          m3

pmaddubsw    m3,          m0,          [r5 + 8 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1904], m2

; mode 32 [row 0, 1]

pmaddubsw    m2,          m4,          [r5 + 21 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m5,          [r5 + 10 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1920], m2

; mode 32 [row 3]

pmaddubsw    m2,          m1,          [r5 + 20 * 16]
pmulhrsw     m2,          m7

pxor         m3,          m3

packuswb     m2,          m3
movh         [r0 + 1944], m2

; mode 32 [row 4, 5]

pmaddubsw    m2,          m6,          [r5 + 9 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m6,          [r5 + 30 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1952], m2

; mode 33 [row 4, 5]

pmaddubsw    m2,          m0,          [r5 + 2 * 16]
pmulhrsw     m2,          m7

pmaddubsw    m3,          m0,          [r5 + 28 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 2016], m2

; mode 32 [row 6]

pmaddubsw    m2,          m0,          [r5 + 19 * 16]
pmulhrsw     m2,          m7

; mode 32 [row 7]

movu         m0,          [r1 + 6]
palignr      m3,          m0,          1
punpcklbw    m0,          m3

pmaddubsw    m3,          m0,          [r5 + 8 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 1968], m2

; mode 33 [row 6, 7]

pmaddubsw    m2,          m0,          [r5 + 22 * 16]
pmulhrsw     m2,          m7

movu         m0,          [r1 + 7]
palignr      m3,          m0,          1
punpcklbw    m0,          m3

pmaddubsw    m3,          m0,          [r5 + 16 * 16]
pmulhrsw     m3,          m7

packuswb     m2,          m3
movu         [r0 + 2032], m2

; mode 33 [row 0]

pmaddubsw    m2,          m4,          [r5 + 26 * 16]
pmulhrsw     m2,          m7

pxor         m3,          m3

packuswb     m2,          m3
movh         [r0 + 1984], m2

; mode 34 [row 0, 1, 2, 3, 4, 5, 6, 7]

movu         m0,          [r3 + 2]
palignr      m1,          m0,          1
punpcklqdq   m2,          m0,          m1
movu         [r0 + 2048], m2

palignr      m1,          m0,          2
palignr      m2,          m0,          3
punpcklqdq   m1,          m2
movu         [r0 + 2064], m1

palignr      m1,          m0,          4
palignr      m2,          m0,          5
punpcklqdq   m1,          m2
movu         [r0 + 2080], m1

palignr      m1,          m0,          6
palignr      m2,          m0,          7
punpcklqdq   m1,          m2
movu         [r0 + 2096], m1

RET

;-----------------------------------------------------------------------------
; void all_angs_pred_16x16(pixel *dest, pixel *above0, pixel *left0, pixel *above1, pixel *left1, bool bLuma)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal all_angs_pred_16x16, 6, 6, 8, dest, above0, left0, above1, left1, bLuma

movu     m0,               [r4 + 2]
movu     [r0 + 0 * 16],    m0

movu     m1,               m0

movu      m6,              [r4 + 18]
palignr   m5,              m6,             m0,    1
movu     [r0 + 1 * 16],    m5

movu     m4,               m5

palignr   m5,              m6,             m0,    2
movu      [r0 + 2 * 16],   m5
palignr   m5,              m6,             m0,    3
movu      [r0 + 3 * 16],   m5
palignr   m5,              m6,             m0,    4
movu      [r0 + 4 * 16],   m5
palignr   m5,              m6,             m0,    5
movu      [r0 + 5 * 16],   m5
palignr   m5,              m6,             m0,    6
movu      [r0 + 6 * 16],   m5
palignr   m5,              m6,             m0,    7
movu      [r0 + 7 * 16],   m5

movu     m7,               m5

palignr   m5,              m6,             m0,    8
movu      [r0 + 8 * 16],   m5

movu      m2,              m5

palignr   m5,              m6,             m0,    9
movu      [r0 + 9 * 16],   m5

palignr   m3,              m6,             m0,    10
movu      [r0 + 10 * 16],  m3
palignr   m3,              m6,             m0,    11
movu      [r0 + 11 * 16],  m3
palignr   m3,              m6,             m0,    12
movu      [r0 + 12 * 16],  m3

; mode 3 [row 15]
movu          [r0 + (3-2)*16*16 + 15 * 16], m3

palignr   m3,              m6,             m0,    13
movu     [r0 + 13 * 16],   m3
palignr   m3,              m6,             m0,    14
movu     [r0 + 14 * 16],   m3
palignr   m3,              m6,             m0,    15
movu     [r0 + 15 * 16],   m3

; mode 3 [row 0]
lea           r5,    [ang_table]
movu          m3,    [pw_1024]
movu          m0,    [r4 + 1]
punpcklbw     m0,    m1

; mode 17 [row 8 - second half]
pmaddubsw     m1,                   m0,    [r5 + 22 * 16]
pmulhrsw      m1,                   m3
packuswb      m1,                   m1
movh          [r0 + 248 * 16 + 8],  m1
; mode 17 [row 8 - second half] end

pmaddubsw     m1,    m0,        [r5 + 26 * 16]
pmulhrsw      m1,    m3
punpcklbw     m7,    m2
pmaddubsw     m2,    m7,        [r5 + 26 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 16 * 16],   m1

;mode 6 [row 1]
movu          [r0 + 65 * 16],   m1

; mode 4 [row 0]
pmaddubsw     m1,             m0,         [r5 + 21 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m2,             m7,         [r5 + 21 * 16]
pmulhrsw      m2,             m3
packuswb      m1,             m2
movu          [r0 + 32 * 16], m1

; mode 5 [row 0]
pmaddubsw     m1,             m0,         [r5 + 17 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m2,             m7,         [r5 + 17 * 16]
pmulhrsw      m2,             m3
packuswb      m1,             m2
movu          [r0 + 48 * 16], m1

; mode 6 [row 0]
pmaddubsw     m1,             m0,         [r5 + 13 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m2,             m7,         [r5 + 13 * 16]
pmulhrsw      m2,             m3
packuswb      m1,             m2
movu          [r0 + 64 * 16], m1

; mode 7 [row 0]
pmaddubsw     m1,             m0,        [r5 + 9 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m2,             m7,        [r5 + 9 * 16]
pmulhrsw      m2,             m3
packuswb      m1,             m2
movu          [r0 + 80 * 16], m1

; mode 7 [row 1]
pmaddubsw     m1,             m0,         [r5 + 18 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m2,             m7,         [r5 + 18 * 16]
pmulhrsw      m2,             m3
packuswb      m1,             m2
movu          [r0 + 81 * 16], m1

; mode 7 [row 2]
pmaddubsw     m1,             m0,         [r5 + 27 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m2,             m7,         [r5 + 27 * 16]
pmulhrsw      m2,             m3
packuswb      m1,             m2
movu          [r0 + 82 * 16], m1

; mode 8 [row 0]
pmaddubsw     m1,             m0,        [r5 + 5 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m2,             m7,        [r5 + 5 * 16]
pmulhrsw      m2,             m3
packuswb      m1,             m2
movu          [r0 + 96 * 16], m1

; mode 8 [row 1]
pmaddubsw     m1,             m0,         [r5 + 10 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m2,             m7,         [r5 + 10 * 16]
pmulhrsw      m2,             m3
packuswb      m1,             m2
movu          [r0 + 97 * 16], m1

; mode 8 [row 2]
pmaddubsw     m1,             m0,         [r5 + 15 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m2,             m7,         [r5 + 15 * 16]
pmulhrsw      m2,             m3
packuswb      m1,             m2
movu          [r0 + 98 * 16], m1

; mode 8 [row 3]
pmaddubsw     m1,             m0,         [r5 + 20 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m2,             m7,         [r5 + 20 * 16]
pmulhrsw      m2,             m3
packuswb      m1,             m2
movu          [r0 + 99 * 16], m1

; mode 8 [row 4]
pmaddubsw     m1,              m0,         [r5 + 25 * 16]
pmulhrsw      m1,              m3
pmaddubsw     m2,              m7,         [r5 + 25 * 16]
pmulhrsw      m2,              m3
packuswb      m1,              m2
movu          [r0 + 100 * 16], m1

; mode 8 [row 5]
pmaddubsw     m1,              m0,         [r5 + 30 * 16]
pmulhrsw      m1,              m3
pmaddubsw     m2,              m7,         [r5 + 30 * 16]
pmulhrsw      m2,              m3
packuswb      m1,              m2
movu          [r0 + 101 * 16], m1

; mode 15 [row 13 - second half]
pmaddubsw     m1,                  m0,     [r5 + 18 * 16]
pmulhrsw      m1,                  m3
packuswb      m1,                  m1
movh          [r0 + 221 * 16 + 8], m1
; mode 15 [row 13 - second half] end

; mode 15 [row 14 - second half]
pmaddubsw     m1,                  m0,     [r5 + 1 * 16]
pmulhrsw      m1,                  m3
packuswb      m1,                  m1
movh          [r0 + 222 * 16 + 8], m1
; mode 15 [row 14 - second half] end

; mode 16 [row 10 - second half]
pmaddubsw     m1,                  m0,    [r5 + 25 * 16]
pmulhrsw      m1,                  m3
packuswb      m1,                  m1
movh          [r0 + 234 * 16 + 8], m1
; mode 16 [row 10 - second half] end

; mode 16 [row 11 - second half]
pmaddubsw     m1,                  m0,    [r5 + 4 * 16]
pmulhrsw      m1,                  m3
packuswb      m1,                  m1
movh          [r0 + 235 * 16 + 8], m1
; mode 16 [row 11 - second half] end

; mode 3 [row 1]
movu          m6,    [r5 + 20 * 16]
movu          m0,    [r4 + 2]
punpcklbw     m0,    m4

; mode 17 [row 7 - second half]
pmaddubsw     m1,     m0,          [r5 + 16 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                  m1
movh          [r0 + 247 * 16 + 8], m1

; mode 17 [row 7 - second half] end
pmaddubsw     m1,             m0,          m6
pmulhrsw      m1,             m3
movu          m2,             [r4 + 10]
punpcklbw     m2,             m5
pmaddubsw     m4,             m2,          m6
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 17 * 16], m1

;mode 6 [row 3]
movu          [r0 + 67 * 16], m1

; mode 4 row [row 1]
pmaddubsw     m1,             m0,         [r5 + 10 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,         [r5 + 10 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 33 * 16], m1

; mode 4 row [row 2]
pmaddubsw     m1,             m0,         [r5 + 31 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,         [r5 + 31 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 34 * 16], m1

; mode 7 [row 6]
movu          [r0 + 86 * 16], m1

; mode 5 row [row 1]
pmaddubsw     m1,             m0,        [r5 + 2 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,        [r5 + 2 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 49 * 16], m1

; mode 5 row [row 2]
pmaddubsw     m1,             m0,         [r5 + 19 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,         [r5 + 19 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 50 * 16], m1

; mode 6 [row 2]
pmaddubsw     m1,             m0,        [r5 + 7 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,        [r5 + 7 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 66 * 16], m1

; mode 7 [row 3]
pmaddubsw     m1,             m0,        [r5 + 4 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,        [r5 + 4 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 83 * 16], m1

; mode 7 [row 4]
pmaddubsw     m1,             m0,         [r5 + 13 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,         [r5 + 13 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 84 * 16], m1

; mode 8 [row 8]
movu          [r0 + 104 * 16], m1

; mode 7 [row 5]
pmaddubsw     m1,             m0,         [r5 + 22 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,         [r5 + 22 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 85 * 16], m1

; mode 8 [row 6]
pmaddubsw     m1,              m0,      [r5 + 3 * 16]
pmulhrsw      m1,              m3
pmaddubsw     m4,              m2,      [r5 + 3 * 16]
pmulhrsw      m4,              m3
packuswb      m1,              m4
movu          [r0 + 102 * 16], m1

; mode 8 [row 7]
pmaddubsw     m1,              m0,        [r5 + 8 * 16]
pmulhrsw      m1,              m3
pmaddubsw     m4,              m2,        [r5 + 8 * 16]
pmulhrsw      m4,              m3
packuswb      m1,              m4
movu          [r0 + 103 * 16], m1

; mode 8 [row 9]
pmaddubsw     m1,              m0,         [r5 + 18 * 16]
pmulhrsw      m1,              m3
pmaddubsw     m4,              m2,         [r5 + 18 * 16]
pmulhrsw      m4,              m3
packuswb      m1,              m4
movu          [r0 + 105 * 16], m1

; mode 8 [row 10]
pmaddubsw     m1,              m0,         [r5 + 23 * 16]
pmulhrsw      m1,              m3
pmaddubsw     m4,              m2,         [r5 + 23 * 16]
pmulhrsw      m4,              m3
packuswb      m1,              m4
movu          [r0 + 106 * 16], m1

; mode 8 [row 11]
pmaddubsw     m1,              m0,         [r5 + 28 * 16]
pmulhrsw      m1,              m3
pmaddubsw     m4,              m2,         [r5 + 28 * 16]
pmulhrsw      m4,              m3
packuswb      m1,              m4
movu          [r0 + 107 * 16], m1

; mode 3 [row 2]
movu          m0,    [r4 + 3]
movd          m1,    [r4 + 19]
palignr       m1,    m0,          1
punpcklbw     m0,    m1

; mode 17 [row 6 - second half]
pmaddubsw     m1,                  m0,     [r5 + 10 * 16]
pmulhrsw      m1,                  m3
packuswb      m1,                  m1
movh          [r0 + 246 * 16 + 8], m1
; mode 17 [row 6 - second half] end

pmaddubsw     m1,             m0,          [r5 + 14 * 16]
pmulhrsw      m1,             m3
movu          m2,             [r4 + 11]
movd          m4,             [r4 + 27]
palignr       m4,             m2,          1
punpcklbw     m2,             m4
pmaddubsw     m4,             m2,          [r5 + 14 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 18 * 16], m1

; mode 6 [row 5]
movu          [r0 + 69 * 16], m1

; mode 4 row [row 3]
pmaddubsw     m1,             m0,         [r5 + 20 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,         [r5 + 20 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 35 * 16], m1

; mode 5 row [row 3]
pmaddubsw     m1,             m0,        [r5 + 4 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,        [r5 + 4 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 51 * 16], m1

; mode 5 row [row 4]
pmaddubsw     m1,             m0,         [r5 + 21 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,         [r5 + 21 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 52 * 16], m1

; mode 6 [row 4]
pmaddubsw     m1,             m0,        [r5 + 1 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,        [r5 + 1 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 68 * 16], m1

; mode 6 [row 6]
pmaddubsw     m1,             m0,      [r5 + 27 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,      [r5 + 27 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 70 * 16], m1

; mode 7 [row 7]
pmaddubsw     m1,             m0,        [r5 + 8 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,        [r5 + 8 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 87 * 16], m1

; mode 7 [row 8]
pmaddubsw     m1,             m0,         [r5 + 17 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,         [r5 + 17 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 88 * 16], m1

; mode 7 [row 9]
pmaddubsw     m1,             m0,       [r5 + 26 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,       [r5 + 26 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 89 * 16], m1

; mode 8 [row 12]
pmaddubsw     m1,              m0,        [r5 + 1 * 16]
pmulhrsw      m1,              m3
pmaddubsw     m4,              m2,        [r5 + 1 * 16]
pmulhrsw      m4,              m3
packuswb      m1,              m4
movu          [r0 + 108 * 16], m1

; mode 8 [row 13]
pmaddubsw     m1,              m0,      [r5 + 6 * 16]
pmulhrsw      m1,              m3
pmaddubsw     m4,              m2,      [r5 + 6 * 16]
pmulhrsw      m4,              m3
packuswb      m1,              m4
movu          [r0 + 109 * 16], m1

; mode 8 [row 14]
pmaddubsw     m1,              m0,         [r5 + 11 * 16]
pmulhrsw      m1,              m3
pmaddubsw     m4,              m2,         [r5 + 11 * 16]
pmulhrsw      m4,              m3
packuswb      m1,              m4
movu          [r0 + 110 * 16], m1

; mode 8 [row 15]
pmaddubsw     m1,              m0,         [r5 + 16 * 16]
pmulhrsw      m1,              m3
pmaddubsw     m4,              m2,         [r5 + 16 * 16]
pmulhrsw      m4,              m3
packuswb      m1,              m4
movu          [r0 + 111 * 16], m1

; mode 3 [row 3]
movu          m0,              [r4 + 4]
movd          m1,              [r4 + 20]
palignr       m1,              m0,          1
punpcklbw     m0,              m1

; mode 17 [row 4 - second half]
pmaddubsw     m1,                  m0,    [r5 + 30 * 16]
pmulhrsw      m1,                  m3
packuswb      m1,                  m1
movh          [r0 + 244 * 16 + 8], m1
; mode 17 [row 4 - second half] end

; mode 17 [row 5 - second half]
pmaddubsw     m1,                  m0,    [r5 + 4 * 16]
pmulhrsw      m1,                  m3
packuswb      m1,                  m1
movh          [r0 + 245 * 16 + 8], m1
; mode 17 [row 5 - second half] end

pmaddubsw     m1,             m0,          [r5 + 8 * 16]
pmulhrsw      m1,             m3
movu          m2,             [r4 + 12]
movd          m4,             [r4 + 28]
palignr       m4,             m2,          1
punpcklbw     m2,             m4
pmaddubsw     m4,             m2,          [r5 + 8 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 19 * 16], m1

; mode 6 [row 7]
movu          [r0 + 71 * 16], m1

; mode 4 row [row 4]
pmaddubsw     m1,             m0,        [r5 + 9 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,        [r5 + 9 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 36 * 16], m1

; mode 4 row [row 5]
pmaddubsw     m1,             m0,        [r5 + 30 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,         [r5 + 30 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 37 * 16], m1

; mode 7 row [row 13]
movu          [r0 + 93 * 16], m1

; mode 5 row [row 5]
pmaddubsw     m1,             m0,        [r5 + 6 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,        [r5 + 6 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 53 * 16], m1

; mode 5 row [row 6]
pmaddubsw     m1,             m0,         [r5 + 23 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,         [r5 + 23 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 54 * 16], m1

; mode 6 [row 8]
pmaddubsw     m1,             m0,         [r5 + 21 * 16]
pmulhrsw      m1,             m3
pmaddubsw     m4,             m2,         [r5 + 21 * 16]
pmulhrsw      m4,             m3
packuswb      m1,             m4
movu          [r0 + 72 * 16], m1

; mode 7 [row 12]
movu          [r0 + 92 * 16], m1

; mode 7 [row 10]
pmaddubsw     m1,    m0,      [r5 + 3 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 3 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 90 * 16], m1

; mode 7 [row 11]
pmaddubsw     m1,    m0,      [r5 + 12 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 12 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 91 * 16], m1

; mode 3 [row 4]
movu          m0,    [r4 + 5]
movd          m1,    [r4 + 20]
palignr       m1,    m0,         1
punpcklbw     m0,    m1

; mode 17 [row 3 - second half]
pmaddubsw     m1,     m0,           [r5 + 24 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 243 * 16 + 8],  m1

; mode 17 [row 3 - second half] end
pmaddubsw     m1,    m0,          [r5 + 2 * 16]
pmulhrsw      m1,    m3
movu          m2,    [r4 + 13]
movd          m4,    [r4 + 29]
palignr       m4,    m2,          1
punpcklbw     m2,    m4
pmaddubsw     m4,    m2,          [r5 + 2 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 20 * 16], m1

;mode 6 [row 9]
movu          [r0 + 73 * 16], m1

; mode 4 row [row 6]
movu          m6,    [r5 + 19 * 16]
pmaddubsw     m1,    m0,      m6
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      m6
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 38 * 16], m1

; mode 3 [row 5]
pmaddubsw     m1,    m0,      [r5 + 28 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 28 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 21 * 16], m1

;mode 6 [row 11]
movu          [r0 + 75 * 16], m1

; mode 5 row [row 7]
pmaddubsw     m1,    m0,      [r5 + 8 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 8 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 55 * 16], m1

; mode 5 row [row 8]
pmaddubsw     m1,    m0,      [r5 + 25 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 25 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 56 * 16], m1

; mode 6 [row 10]
pmaddubsw     m1,    m0,      [r5 + 15 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 15 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 74 * 16], m1

; mode 7 [row 14]
pmaddubsw     m1,    m0,      [r5 + 7 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 7 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 94 * 16], m1

; mode 7 [row 15]
pmaddubsw     m1,    m0,      [r5 + 16 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 95 * 16], m1

; mode 3 [row 6]
movu          m0,    [r4 + 6]
movd          m1,    [r4 + 22]
palignr       m1,    m0,          1
punpcklbw     m0,    m1

; mode 17 [row 2 - second half]
pmaddubsw     m1,     m0,          [r5 + 18 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 242 * 16 + 8],  m1
; mode 17 [row 2 - second half] end

pmaddubsw     m1,    m0,          [r5 + 22 * 16]
pmulhrsw      m1,    m3
movu          m2,    [r4 + 14]
movd          m4,    [r4 + 30]
palignr       m4,    m2,          1
punpcklbw     m2,    m4
pmaddubsw     m4,    m2,          [r5 + 22 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 22 * 16], m1

; mode 6 [row 13]
movu          [r0 + 77 * 16], m1

; mode 4 row [row 7]
pmaddubsw     m1,    m0,      [r5 + 8 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 8 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 39 * 16], m1

; mode 4 row [row 8]
pmaddubsw     m1,    m0,       [r5 + 29 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,       [r5 + 29 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 40 * 16], m1

; mode 5 row [row 9]
pmaddubsw     m1,    m0,      [r5 + 10 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 10 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 57 * 16], m1

; mode 5 row [row 10]
pmaddubsw     m1,    m0,      [r5 + 27 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 27 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 58 * 16], m1

; mode 6 [row 12]
pmaddubsw     m1,    m0,      [r5 + 9 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 9 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 76 * 16], m1

; mode 3 [row 7]
movu          m0,    [r4 + 7]
movd          m1,    [r4 + 27]
palignr       m1,    m0,          1
punpcklbw     m0,    m1

; mode 17 [row 1 - second half]
pmaddubsw     m1,     m0,           [r5 + 12 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 241 * 16 + 8],  m1
; mode 17 [row 1 - second half] end

pmaddubsw     m1,    m0,          [r5 + 16 * 16]
pmulhrsw      m1,    m3
movu          m2,    [r4 + 15]
movd          m4,    [r4 + 25]
palignr       m4,    m2,          1
punpcklbw     m2,    m4
pmaddubsw     m4,    m2,          [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 23 * 16], m1

; mode 6 [row 15]
movu          [r0 + 79 * 16], m1

; mode 4 row [row 9]
pmaddubsw     m1,    m0,      [r5 + 18 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 18 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 41 * 16], m1

; mode 5 row [row 11]
pmaddubsw     m1,    m0,      [r5 + 12 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 12 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 59 * 16], m1

; mode 5 row [row 12]
pmaddubsw     m1,    m0,      [r5 + 29 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 29 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 60 * 16], m1

; mode 6 [row 14]
pmaddubsw     m1,    m0,      [r5 + 3 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 3 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 78 * 16], m1

; mode 3 [row 8]
movu          m0,    [r4 + 8]
movd          m1,    [r4 + 24]
palignr       m1,    m0,          1
punpcklbw     m0,    m1
pmaddubsw     m1,    m0,          [r5 + 10 * 16]
pmulhrsw      m1,    m3
movu          m2,    [r4 + 16]
psrldq        m4,    m2,         1
pinsrb        m4,    [r4 + 32],  15
punpcklbw     m2,    m4
pmaddubsw     m4,    m2,          [r5 + 10 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 24 * 16], m1

; mode 4 row [row 10]
pmaddubsw     m1,    m0,      [r5 + 7 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 7 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 42 * 16], m1

; mode 4 row [row 11]
pmaddubsw     m1,    m0,      [r5 + 28 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 28 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 43 * 16], m1

; mode 5 row [row 13]
pmaddubsw     m1,    m0,      [r5 + 14 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 14 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 61 * 16], m1

; mode 5 row [row 14]
pmaddubsw     m1,    m0,      [r5 + 31 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 31 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 62 * 16], m1

; mode 3 [row 9]
movu          m0,    [r4 +  9]
movd          m1,    [r4 + 16]
palignr       m1,    m0,         1
punpcklbw     m0,    m1
pmaddubsw     m1,    m0,         [r5 + 4 * 16]
pmulhrsw      m1,    m3
movu          m2,    [r4 + 17]
movd          m4,    [r4 + 33]
palignr       m4,    m2,         1
punpcklbw     m2,    m4
pmaddubsw     m4,    m2,         [r5 + 4 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 25 * 16], m1

; mode 4 row [row 12]
pmaddubsw     m1,    m0,      [r5 + 17 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 17 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 44 * 16], m1

; mode 3 [row 10]
pmaddubsw     m1,    m0,          [r5 + 30 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,          [r5 + 30 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 26 * 16], m1

; mode 5 row [row 15]
pmaddubsw     m1,    m0,      [r5 + 16 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 63 * 16], m1

; mode 3 [row 11]
movu          m0,    [r4 + 10]
movd          m1,    [r4 + 26]
palignr       m1,    m0,          1
punpcklbw     m0,    m1
pmaddubsw     m1,    m0,          [r5 + 24 * 16]
pmulhrsw      m1,    m3
movu          m2,    [r4 + 18]
movd          m4,    [r4 + 34]
palignr       m4,    m2,         1
punpcklbw     m2,    m4
pmaddubsw     m4,    m2,         [r5 + 24 * 16]
pmulhrsw      m4,    m3
packuswb      m1,                 m4
movu          [r0 + 27 * 16],     m1

; mode 4 row [row 13]
pmaddubsw     m1,    m0,      [r5 + 6 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 6 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 45 * 16], m1

; mode 4 row [row 14]
pmaddubsw     m1,    m0,      [r5 + 27 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 27 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 46 * 16], m1

; mode 3 [row 12]
movu          m0,    [r4 + 11]
movd          m1,    [r4 + 27]
palignr       m1,    m0,          1
punpcklbw     m0,    m1
pmaddubsw     m1,    m0,          [r5 + 18 * 16]
pmulhrsw      m1,    m3
movu          m2,    [r4 + 19]
movd          m4,    [r4 + 35]
palignr       m4,    m2,          1
punpcklbw     m2,    m4
pmaddubsw     m4,    m2,          [r5 + 18 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 28 * 16], m1

; mode 4 row [row 15]
pmaddubsw     m1,    m0,      [r5 + 16 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m2,      [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 47 * 16], m1

; mode 3 [row 13]
movu          m0,    [r4 + 12]
movd          m1,    [r4 + 28]
palignr       m1,    m0,          1
punpcklbw     m0,    m1
pmaddubsw     m1,    m0,          [r5 + 12 * 16]
pmulhrsw      m1,    m3
movu          m2,    [r4 + 20]
movd          m4,    [r4 + 36]
palignr       m4,    m2,          1
punpcklbw     m2,    m4
pmaddubsw     m4,    m2,          [r5 + 12 * 16]
pmulhrsw      m4,    m3
packuswb      m1,             m4
movu          [r0 + 29 * 16], m1

; mode 3 [row 14]
movu          m0,    [r4 + 13]
movd          m1,    [r4 + 29]
palignr       m1,    m0,         1
punpcklbw     m0,    m1
pmaddubsw     m1,    m0,         [r5 + 6 * 16]
pmulhrsw      m1,    m3
movu          m2,    [r4 + 21]
movd          m4,    [r4 + 37]
palignr       m4,    m2,         1
punpcklbw     m2,    m4
pmaddubsw     m4,    m2,         [r5 + 6 * 16]
pmulhrsw      m4,    m3
packuswb      m1,                m4
movu          [r0 + 30 * 16],    m1

; mode 9
movu          m0,    [r2 + 1]
movd          m1,    [r2 + 17]
palignr       m1,    m0,         1

; mode 9 [row 15]
movu          [r0 + 127 * 16],  m1

; mode 9 [row 0]
punpcklbw     m0,    m1
pmaddubsw     m1,    m0,        [r5 + 2 * 16]
pmulhrsw      m1,    m3
movu          m7,    [r2 +  9]
movd          m4,    [r4 + 25]
palignr       m2,    m7,        1
punpcklbw     m7,    m2
pmaddubsw     m2,    m7,        [r5 + 2 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 112 * 16],  m1

; mode 9 [row 1]
pmaddubsw     m1,    m0,        [r5 + 4 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 4 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 113 * 16],  m1

; mode 9 [row 2]
pmaddubsw     m1,    m0,        [r5 + 6 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 6 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 114 * 16],  m1

; mode 9 [row 3]
pmaddubsw     m1,    m0,        [r5 + 8 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 8 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 115 * 16],  m1

; mode 9 [row 4]
pmaddubsw     m1,    m0,        [r5 + 10 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 10 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 116 * 16],  m1

; mode 9 [row 5]
pmaddubsw     m1,    m0,        [r5 + 12 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 12 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 117 * 16],  m1

; mode 9 [row 6]
pmaddubsw     m1,    m0,        [r5 + 14 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 14 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 118 * 16],  m1

; mode 9 [row 7]
pmaddubsw     m1,    m0,        [r5 + 16 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 16 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 119 * 16],  m1

; mode 9 [row 8]
pmaddubsw     m1,    m0,        [r5 + 18 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 18 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 120 * 16],  m1

; mode 9 [row 9]
pmaddubsw     m1,    m0,        [r5 + 20 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 20 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 121 * 16],  m1

; mode 9 [row 10]
pmaddubsw     m1,    m0,        [r5 + 22 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 22 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 122 * 16],  m1

; mode 9 [row 11]
pmaddubsw     m1,    m0,        [r5 + 24 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 24 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 123 * 16],  m1

; mode 9 [row 12]
pmaddubsw     m1,    m0,        [r5 + 26 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 26 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 124 * 16],  m1

; mode 9 [row 13]
pmaddubsw     m1,    m0,         [r5 + 28 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,         [r5 + 28 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 125 * 16],  m1

; mode 9 [row 14]
pmaddubsw     m1,    m0,        [r5 + 30 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 30 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 126 * 16],  m1

; mode 10
movu         m1,               [r2 + 1]
movu         [r0 + 128 * 16],  m1
movu         [r0 + 129 * 16],  m1
movu         [r0 + 130 * 16],  m1
movu         [r0 + 131 * 16],  m1
movu         [r0 + 132 * 16],  m1
movu         [r0 + 133 * 16],  m1
movu         [r0 + 134 * 16],  m1
movu         [r0 + 135 * 16],  m1
movu         [r0 + 136 * 16],  m1
movu         [r0 + 137 * 16],  m1
movu         [r0 + 138 * 16],  m1
movu         [r0 + 139 * 16],  m1
movu         [r0 + 140 * 16],  m1
movu         [r0 + 141 * 16],  m1
movu         [r0 + 142 * 16],  m1
movu         [r0 + 143 * 16],  m1

pxor         m0,          m0
pshufb       m1,          m1,         m0
punpcklbw    m1,          m0
movu         m2,          [r1]
pshufb       m2,          m2,         m0
punpcklbw    m2,          m0
movu         m4,          [r1 + 1]
punpcklbw    m5,          m4,         m0
punpckhbw    m4,          m0
psubw        m5,          m2
psubw        m4,          m2
psraw        m5,          1
psraw        m4,          1
paddw        m5,          m1
paddw        m4,          m1
packuswb     m5,          m4

pextrb       [r0 + 128 * 16],  m5,          0
pextrb       [r0 + 129 * 16],  m5,          1
pextrb       [r0 + 130 * 16],  m5,          2
pextrb       [r0 + 131 * 16],  m5,          3
pextrb       [r0 + 132 * 16],  m5,          4
pextrb       [r0 + 133 * 16],  m5,          5
pextrb       [r0 + 134 * 16],  m5,          6
pextrb       [r0 + 135 * 16],  m5,          7
pextrb       [r0 + 136 * 16],  m5,          8
pextrb       [r0 + 137 * 16],  m5,          9
pextrb       [r0 + 138 * 16],  m5,          10
pextrb       [r0 + 139 * 16],  m5,          11
pextrb       [r0 + 140 * 16],  m5,          12
pextrb       [r0 + 141 * 16],  m5,          13
pextrb       [r0 + 142 * 16],  m5,          14
pextrb       [r0 + 143 * 16],  m5,          15

; mode 11
movu          m0,               [r2]

; mode 11 [row 15]
movu          [r0 + 159 * 16],  m0

; mode 11 [row 0]
movu          m1,    [r2 + 1]
punpcklbw     m0,    m1
pmaddubsw     m1,    m0,        [r5 + 30 * 16]
pmulhrsw      m1,    m3
movu          m7,    [r2 + 8]
movu          m2,    [r2 + 9]
punpcklbw     m7,    m2
pmaddubsw     m2,    m7,        [r5 + 30 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 144 * 16],  m1

; mode 11 [row 1]
pmaddubsw     m1,    m0,        [r5 + 28 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 28 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 145 * 16],  m1

; mode 11 [row 2]
pmaddubsw     m1,    m0,        [r5 + 26 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 26 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 146 * 16],  m1

; mode 11 [row 3]
pmaddubsw     m1,    m0,         [r5 + 24 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 24 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 147 * 16],  m1

; mode 11 [row 4]
pmaddubsw     m1,    m0,        [r5 + 22 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 22 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 148 * 16],  m1

; mode 11 [row 5]
pmaddubsw     m1,    m0,        [r5 + 20 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 20 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 149 * 16],  m1

; mode 11 [row 6]
pmaddubsw     m1,    m0,        [r5 + 18 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 18 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 150 * 16],  m1

; mode 11 [row 7]
pmaddubsw     m1,    m0,        [r5 + 16 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 16 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 151 * 16],  m1

; mode 11 [row 8]
pmaddubsw     m1,    m0,        [r5 + 14 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 14 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 152 * 16],  m1

; mode 11 [row 9]
pmaddubsw     m1,    m0,        [r5 + 12 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 12 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 153 * 16],  m1

; mode 11 [row 10]
pmaddubsw     m1,    m0,        [r5 + 10 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 10 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 154 * 16],  m1

; mode 11 [row 11]
pmaddubsw     m1,    m0,        [r5 + 8 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 8 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 155 * 16],  m1

; mode 11 [row 12]
pmaddubsw     m1,    m0,        [r5 + 6 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 6 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 156 * 16],  m1

; mode 11 [row 13]
pmaddubsw     m1,    m0,        [r5 + 4 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 4 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 157 * 16],  m1

; mode 11 [row 14]
pmaddubsw     m1,    m0,        [r5 + 2 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 2 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 158 * 16],  m1

; mode 12 [row 0]
movu          m0,    [r4]
movu          m1,    [r4 + 1]
punpcklbw     m0,    m1
pmaddubsw     m1,    m0,        [r5 + 27 * 16]
pmulhrsw      m1,    m3
movu          m7,    [r4 + 8]
movd          m2,    [r4 + 24]
palignr       m2,    m7,        1
punpcklbw     m7,    m2
pmaddubsw     m2,    m7,        [r5 + 27 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 160 * 16],  m1

; mode 12 [row 1]
pmaddubsw     m1,    m0,        [r5 + 22 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 22 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 161 * 16],  m1

; mode 12 [row 2]
pmaddubsw     m1,    m0,        [r5 + 17 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 17 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 162 * 16],  m1

; mode 12 [row 3]
pmaddubsw     m1,    m0,        [r5 + 12 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 12 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 163 * 16],  m1

; mode 12 [row 4]
pmaddubsw     m1,    m0,        [r5 + 7 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 7 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 164 * 16],  m1

; mode 12 [row 5]
pmaddubsw     m1,    m0,        [r5 + 2 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 2 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 165 * 16],  m1

; mode 13 [row 0]
pmaddubsw     m1,    m0,        [r5 + 23 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 23 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 176 * 16],  m1

; mode 13 [row 1]
pmaddubsw     m1,    m0,        [r5 + 14 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 14 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 177 * 16],  m1

; mode 13 [row 2]
pmaddubsw     m1,    m0,        [r5 + 5 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 5 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 178 * 16],  m1

; mode 14 [row 0]
pmaddubsw     m1,    m0,        [r5 + 19 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 19 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 192 * 16],  m1

; mode 14 [row 1]
pmaddubsw     m1,    m0,        [r5 + 6 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 6 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 193 * 16],  m1

; mode 17 [row 0]
movu          [r0 + 240 * 16],  m1

; mode 15 [row 0]
pmaddubsw     m1,    m0,        [r5 + 15 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 15 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 208 * 16],  m1

; mode 15 [row 15 - second half]
pmaddubsw     m1,    m0,           [r5 + 16 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                  m1
movh          [r0 + 223 * 16 + 8], m1
; mode 15 [row 15 - second half] end

; mode 16 [row 0]
pmaddubsw     m1,    m0,        [r5 + 11 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m2,    m7,        [r5 + 11 * 16]
pmulhrsw      m2,    m3
packuswb      m1,               m2
movu          [r0 + 224 * 16],  m1

; mode 17 [row 9 - second half]
pmaddubsw     m1,     m0,          [r5 + 28 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 249 * 16 + 8],  m1
; mode 17 [row 9 - second half] end

; mode 17 [row 10 - second half]
pmaddubsw     m1,     m0,          [r5 + 2 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 250 * 16 + 8],  m1
; mode 17 [row 10 - second half] end

; mode 17 [row 1 - first half]
pslldq        m6,     m0,          2
pinsrb        m6,     [r3 + 0],    1
pinsrb        m6,     [r3 + 1],    0
pmaddubsw     m1,     m6,          [r5 + 12 * 16]
pmulhrsw      m1,     m3
packuswb      m1,               m1
movh          [r0 + 241 * 16],  m1

; mode 17 [row 11 - second half]
pmaddubsw     m1,     m6,          [r5 + 8 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 251 * 16 + 8],  m1
; mode 17 [row 11 - second half] end

; mode 17 [row 2 - first half]
pslldq        m6,     2
pinsrb        m6,     [r3 + 1],    1
pinsrb        m6,     [r3 + 2],    0
pmaddubsw     m1,     m6,          [r5 + 18 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                  m1
movh          [r0 + 242 * 16],     m1

; mode 17 [row 12 - second half]
pmaddubsw     m1,     m6,           [r5 + 14 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 252 * 16 + 8],  m1
; mode 17 [row 12 - second half] end

; mode 17 [row 3 - first half]
pslldq        m6,     2
pinsrb        m6,     [r3 + 2],    1
pinsrb        m6,     [r3 + 4],    0
pmaddubsw     m1,     m6,          [r5 + 24 * 16]
pmulhrsw      m1,     m3
packuswb      m1,               m1
movh          [r0 + 243 * 16],  m1

; mode 17 [row 13 - first half]
pmaddubsw     m1,     m6,           [r5 + 20 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 253 * 16 + 8],  m1

; mode 17 [row 4 - first half]
pslldq        m6,     2
pinsrb        m6,     [r3 + 4],    1
pinsrb        m6,     [r3 + 5],    0
pmaddubsw     m1,     m6,          [r5 + 30 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                  m1
movh          [r0 + 244 * 16],     m1

; mode 17 [row 5 - first half]
pmaddubsw     m1,     m6,          [r5 + 4 * 16]
pmulhrsw      m1,     m3
packuswb      m1,               m1
movh          [r0 + 245 * 16],  m1

; mode 17 [row 14 - second half]
pmaddubsw     m1,     m6,          [r5 + 26 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                  m1
movh          [r0 + 254 * 16 + 8], m1
; mode 17 [row 14 - second half] end

; mode 17 [row 6 - first half]
pslldq        m6,     2
pinsrb        m6,     [r3 + 5],    1
pinsrb        m6,     [r3 + 6],    0
pmaddubsw     m1,     m6,          [r5 + 10 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                  m1
movh          [r0 + 246 * 16],     m1

; mode 17 [row 7 - first half]
pslldq        m6,     2
pinsrb        m6,     [r3 + 6],    1
pinsrb        m6,     [r3 + 7],    0
pmaddubsw     m1,     m6,          [r5 + 16 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                  m1
movh          [r0 + 247 * 16],     m1

; mode 17 [row 8 - first half]
pslldq        m6,     2
pinsrb        m6,     [r3 + 7],    1
pinsrb        m6,     [r3 + 9],    0
pmaddubsw     m1,     m6,          [r5 + 22 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                  m1
movh          [r0 + 248 * 16],     m1

; mode 17 [row 9 - first half]
pslldq        m6,     2
pinsrb        m6,     [r3 +  9],    1
pinsrb        m6,     [r3 + 10],    0
pmaddubsw     m1,     m6,           [r5 + 28 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 249 * 16],      m1

; mode 17 [row 10 - first half]
pmaddubsw     m1,     m6,          [r5 + 2 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                  m1
movh          [r0 + 250 * 16],     m1

; mode 17 [row 11 - first half]
pslldq        m6,     2
pinsrb        m6,     [r3 + 10],    1
pinsrb        m6,     [r3 + 11],    0
pmaddubsw     m1,     m6,           [r5 + 8 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 251 * 16],      m1

; mode 17 [row 12 - first half]
pslldq        m6,     2
pinsrb        m6,     [r3 + 11],    1
pinsrb        m6,     [r3 + 12],    0
pmaddubsw     m1,     m6,           [r5 + 14 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 252 * 16],      m1

; mode 17 [row 13 - first half]
pslldq        m6,     2
pinsrb        m6,     [r3 + 12],    1
pinsrb        m6,     [r3 + 14],    0
pmaddubsw     m1,     m6,           [r5 + 20 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 253 * 16],      m1

; mode 17 [row 14 - first half]
pslldq        m6,     2
pinsrb        m6,     [r3 + 14],    1
pinsrb        m6,     [r3 + 15],    0
pmaddubsw     m1,     m6,           [r5 + 26 * 16]
pmulhrsw      m1,     m3
packuswb      m1,                   m1
movh          [r0 + 254 * 16],      m1

; mode 16 [row 12 -  second half]
pmaddubsw     m1,    m0,            [r5 + 15 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                   m1
movh          [r0 + 236 * 16 + 8],  m1
; mode 16 [row 12 -  second half]

; mode 12 [row 6]
pslldq        m2,    m0,            2
pinsrb        m2,    [r3 + 0],      1
pinsrb        m2,    [r3 + 6],      0
pmaddubsw     m1,    m2,            [r5 + 29 * 16]
pmulhrsw      m1,    m3
movu          m0,    [r4 + 7]
psrldq        m4,    m0,            1
punpcklbw     m0,    m4
pmaddubsw     m4,    m0,            [r5 + 29 * 16]
pmulhrsw      m4,    m3
packuswb      m1,                   m4
movu          [r0 + 166 * 16],      m1

; mode 12 [row 7]
pmaddubsw     m1,    m2,        [r5 + 24 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 24 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 167 * 16],  m1

; mode 12 [row 8]
pmaddubsw     m1,    m2,        [r5 + 19 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 19 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 168 * 16],  m1

; mode 12 [row 9]
pmaddubsw     m1,    m2,        [r5 + 14 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 14 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 169 * 16],  m1

; mode 12 [row 10]
pmaddubsw     m1,    m2,        [r5 + 9 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 9 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 170 * 16],  m1

; mode 12 [row 11]
pmaddubsw     m1,    m2,        [r5 + 4 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 4 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 171 * 16],  m1

; mode 13 [row 3]
pinsrb        m7,    m2,            [r3 +  4],   0
pmaddubsw     m1,    m7,        [r5 + 28 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 28 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 179 * 16],  m1

; mode 13 [row 4]
pmaddubsw     m1,    m7,        [r5 + 19 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 19 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 180 * 16],  m1

; mode 13 [row 5]
pmaddubsw     m1,    m7,        [r5 + 10 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 10 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 181 * 16],  m1

; mode 13 [row 6]
pmaddubsw     m1,    m7,        [r5 + 1 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 1 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 182 * 16],  m1

; mode 14 [row 2]
pinsrb        m5,    m7,        [r3 +  2],   0
pmaddubsw     m1,    m5,        [r5 + 25 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 25 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 194 * 16],  m1

; mode 14 [row 3]
pmaddubsw     m1,    m5,        [r5 + 12 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 12 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 195 * 16],  m1

; mode 15 [row 1]
pmaddubsw     m1,    m5,        [r5 + 30 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 30 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 209 * 16],  m1

; mode 15 [row 2]
pmaddubsw     m1,    m5,        [r5 + 13 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 13 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 210 * 16],  m1

; mode 16 [row 1]
pmaddubsw     m1,    m5,        [r5 + 22 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 22 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 225 * 16],  m1

; mode 16 [row 2]
pmaddubsw     m1,    m5,        [r5 + 1 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m4,    m0,        [r5 + 1 * 16]
pmulhrsw      m4,    m3
packuswb      m1,               m4
movu          [r0 + 226 * 16],  m1

; mode 16 [row 13 - second half]
pmaddubsw     m1,    m5,           [r5 + 26 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                  m1
movh          [r0 + 237 * 16 + 8], m1
; mode 16 [row 13 - second half]

; mode 16 [row 14 - second half]
pmaddubsw     m1,    m5,           [r5 + 5 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                  m1
movh          [r0 + 238 * 16 + 8], m1
; mode 16 [row 14 - second half]

; mode 16 [row 3]
pslldq        m6,    m5,         2
pinsrb        m6,    [r3 + 2],   1
pinsrb        m6,    [r3 + 3],   0
pmaddubsw     m1,    m6,         [r5 + 12 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 227 * 16],   m1

; mode 16 [row 15 - second half]
pmaddubsw     m1,    m6,          [r5 + 16 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                  m1
movh          [r0 + 239 * 16 + 8], m1
; mode 16 [row 15 - second half] end

; mode 16 [row 4- first half]
pslldq        m6,    2
pinsrb        m6,    [r3 + 3],   1
pinsrb        m6,    [r3 + 5],   0
pmaddubsw     m1,    m6,         [r5 + 23 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 228 * 16],   m1

; mode 16 [row 5- first half]
pmaddubsw     m1,    m6,        [r5 + 2 * 16]
pmulhrsw      m1,    m3
packuswb      m1,               m1
movh          [r0 + 229 * 16],  m1

; mode 16 [row 6- first half]
pslldq        m6,    2
pinsrb        m6,    [r3 + 5],   1
pinsrb        m6,    [r3 + 6],   0
pmaddubsw     m1,    m6,         [r5 + 13 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 230 * 16],   m1

; mode 16 [row 7- first half]
pslldq        m6,    2
pinsrb        m6,    [r3 + 6],   1
pinsrb        m6,    [r3 + 8],   0
pmaddubsw     m1,    m6,         [r5 + 24 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 231 * 16],   m1

; mode 16 [row 8- first half]
pmaddubsw     m1,    m6,        [r5 + 3 * 16]
pmulhrsw      m1,    m3
packuswb      m1,               m1
movh          [r0 + 232 * 16],  m1
; mode 19 [row 0 - second half] end

; mode 16 [row 9- first half]
pslldq        m6,    2
pinsrb        m6,    [r3 + 8],   1
pinsrb        m6,    [r3 + 9],   0
pmaddubsw     m1,    m6,        [r5 + 14 * 16]
pmulhrsw      m1,    m3
packuswb      m1,               m1
movh          [r0 + 233 * 16],  m1

; mode 16 [row 10 - first half]
pslldq        m6,    2
pinsrb        m6,    [r3 +  9], 1
pinsrb        m6,    [r3 + 11], 0
pmaddubsw     m1,    m6,        [r5 + 25 * 16]
pmulhrsw      m1,    m3
packuswb      m1,               m1
movh          [r0 + 234 * 16],  m1

; mode 16 [row 11 - first half]
pmaddubsw     m1,    m6,        [r5 + 4 * 16]
pmulhrsw      m1,    m3
packuswb      m1,               m1
movh          [r0 + 235 * 16],  m1

; mode 16 [row 12 - first half]
pslldq        m6,    2
pinsrb        m6,    [r3 + 11], 1
pinsrb        m6,    [r3 + 12], 0
pmaddubsw     m1,    m6,        [r5 + 15 * 16]
pmulhrsw      m1,    m3
packuswb      m1,               m1
movh          [r0 + 236 * 16],  m1

; mode 16 [row 13 - first half]
pslldq        m6,    2
pinsrb        m6,    [r3 + 12],   1
pinsrb        m6,    [r3 + 14],   0
pmaddubsw     m1,    m6,        [r5 + 26 * 16]
pmulhrsw      m1,    m3
packuswb      m1,               m1
movh          [r0 + 237 * 16],  m1

; mode 16 [row 14 - first half]
pmaddubsw     m1,    m6,        [r5 + 5 * 16]
pmulhrsw      m1,    m3
packuswb      m1,               m1
movh          [r0 + 238 * 16],  m1

; mode 16 [row 15 - first half]
pslldq        m6,    2
pinsrb        m6,    [r3 + 14],   1
pinsrb        m6,    [r3 + 15],   0
pmaddubsw     m1,    m6,          [r5 + 16 * 16]
pmulhrsw      m1,    m3
packuswb      m1,               m1
movh          [r0 + 239 * 16],  m1

; mode 14 [row 4]
pslldq        m5,    2
pinsrb        m5,    [r3 + 2],   1
pinsrb        m5,    [r3 + 5],   0
movu          m4,    [r4 + 6]
psrldq        m0,    m4,         1
punpcklbw     m4,    m0

; mode 16 [row 3 - second half]
pmaddubsw     m1,    m4,        [r5 + 12 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                  m1
movh          [r0 + 227 * 16 + 8], m1

; mode 16 [row 3 - second half] end
pmaddubsw     m1,    m5,        [r5 + 31 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m0,    m4,        [r5 + 31 * 16]
pmulhrsw      m0,    m3
packuswb      m1,               m0
movu          [r0 + 196 * 16],  m1

; mode 14 [row 5]
pmaddubsw     m1,    m5,        [r5 + 18 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m0,    m4,        [r5 + 18 * 16]
pmulhrsw      m0,    m3
packuswb      m1,               m0
movu          [r0 + 197 * 16],  m1

; mode 14 [row 6]
pmaddubsw     m1,    m5,         [r5 + 5 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m0,    m4,         [r5 + 5 * 16]
pmulhrsw      m0,    m3
packuswb      m1,               m0
movu          [r0 + 198 * 16],  m1

; mode 15 [row 3]
movu          m6,    m5
pinsrb        m6,    [r3 + 4],   0
pmaddubsw     m1,    m6,         [r5 + 28 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m0,    m4,         [r5 + 28 * 16]
pmulhrsw      m0,    m3
packuswb      m1,                m0
movu          [r0 + 211 * 16],   m1

; mode 15 [row 4]
pmaddubsw     m1,    m6,         [r5 + 11 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m0,    m4,         [r5 + 11 * 16]
pmulhrsw      m0,    m3
packuswb      m1,                m0
movu          [r0 + 212 * 16],   m1

; mode 15 [row 5 - first half]
pslldq        m6,    2
pinsrb        m6,    [r3 + 4],   1
pinsrb        m6,    [r3 + 6],   0
pmaddubsw     m1,    m6,         [r5 + 26 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 213 * 16],   m1

; mode 15 [row 6 - first half]
pmaddubsw     m1,    m6,         [r5 + 9 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 214 * 16],   m1

; mode 15 [row 7 - first half]
pslldq        m6,    2
pinsrb        m6,    [r3 + 6],   1
pinsrb        m6,    [r3 + 8],   0
pmaddubsw     m1,    m6,         [r5 + 24 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 215 * 16],   m1

; mode 15 [row 8 - first half]
pmaddubsw     m1,    m6,         [r5 + 7 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 216 * 16],   m1

; mode 15 [row 9 - first half]
pslldq        m6,    2
pinsrb        m6,    [r3 + 8],   1
pinsrb        m6,    [r3 + 9],   0
pmaddubsw     m1,    m6,         [r5 + 22 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 217 * 16],   m1

; mode 15 [row 10 - first half]
pmaddubsw     m1,    m6,         [r5 + 5 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 218 * 16],   m1

; mode 15 [row 11 - first half]
pslldq        m6,    2
pinsrb        m6,    [r3 +  9],   1
pinsrb        m6,    [r3 + 11],   0
pmaddubsw     m1,    m6,         [r5 + 20 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 219 * 16],   m1

; mode 15 [row 12 - first half]
pmaddubsw     m1,    m6,         [r5 + 3 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 220 * 16],   m1

; mode 15 [row 13 - first half]
pslldq        m6,    2
pinsrb        m6,    [r3 + 11],   1
pinsrb        m6,    [r3 + 13],   0
pmaddubsw     m1,    m6,         [r5 + 18 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 221 * 16],   m1

; mode 15 [row 14 - first half]
pmaddubsw     m1,    m6,         [r5 + 1 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 222 * 16],   m1

; mode 15 [row 15 - first half]
pslldq        m6,    2
pinsrb        m6,    [r3 + 13],   1
pinsrb        m6,    [r3 + 15],   0
pmaddubsw     m1,    m6,         [r5 + 16 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                m1
movh          [r0 + 223 * 16],   m1

; mode 14 [row 7]
pslldq        m5,    2
pinsrb        m5,    [r3 + 5],   1
pinsrb        m5,    [r3 + 7],   0
movu          m0,    [r4 + 5]
psrldq        m6,    m0,          1
punpcklbw     m0,    m6

; mode 15 [row 5 - second half]
pmaddubsw     m1,    m0,           [r5 + 26 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                  m1
movh          [r0 + 213 * 16 + 8], m1
; mode 15 [row 5 - second half] end

; mode 15 [row 6 - second half]
pmaddubsw     m1,    m0,           [r5 + 9 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                  m1
movh          [r0 + 214 * 16 + 8], m1
; mode 15 [row 6 - second half] end

; mode 16 [row 4 - second half]
pmaddubsw     m1,    m0,        [r5 + 23 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                  m1
movh          [r0 + 228 * 16 + 8], m1
; mode 16 [row 4 - second half] end

; mode 16 [row 5 - second half]
pmaddubsw     m1,    m0,        [r5 + 2 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                  m1
movh          [r0 + 229 * 16 + 8], m1

; mode 16 [row 5 - second half] end
pmaddubsw     m1,    m5,        [r5 + 24 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m6,    m0,        [r5 + 24 * 16]
pmulhrsw      m6,    m3
packuswb      m1,               m6
movu          [r0 + 199 * 16],  m1

; mode 14 [row 8]
pmaddubsw     m1,    m5,        [r5 + 11 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m6,    m0,        [r5 + 11 * 16]
pmulhrsw      m6,    m3
packuswb      m1,               m6
movu          [r0 + 200 * 16],  m1

; mode 14 [row 9]
pslldq        m5,    2
pinsrb        m5,    [r3 + 7],    1
pinsrb        m5,    [r3 + 10],   0
movu          m0,    [r4 + 4]
psrldq        m6,    m0,          1
punpcklbw     m0,    m6

; mode 15 [row 7 - second half]
pmaddubsw     m1,    m0,             [r5 + 24 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                    m1
movh          [r0 + 215 * 16 + 8],   m1
; mode 15 [row 7 - second half] end

; mode 15 [row 8 - second half]
pmaddubsw     m1,    m0,             [r5 + 7 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                    m1
movh          [r0 + 216 * 16 + 8],   m1
; mode 15 [row 8 - second half] end

; mode 16 [row 6 - second half]
pmaddubsw     m1,    m0,             [r5 + 13 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                    m1
movh          [r0 + 230 * 16 + 8],   m1
; mode 16 [row 6 - second half] end

; mode 15 [row 6 - second half] end
pmaddubsw     m1,    m5,        [r5 + 30 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m6,    m0,        [r5 + 30 * 16]
pmulhrsw      m6,    m3
packuswb      m1,               m6
movu          [r0 + 201 * 16],  m1

; mode 14 [row 10]
pmaddubsw     m1,    m5,        [r5 + 17 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m6,    m0,        [r5 + 17 * 16]
pmulhrsw      m6,    m3
packuswb      m1,               m6
movu          [r0 + 202 * 16],  m1

; mode 14 [row 11]
pmaddubsw     m1,    m5,        [r5 + 4 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m6,    m0,        [r5 + 4 * 16]
pmulhrsw      m6,    m3
packuswb      m1,               m6
movu          [r0 + 203 * 16],  m1

; mode 14 [row 12]
pslldq        m5,    2
pinsrb        m5,    [r3 + 10],   1
pinsrb        m5,    [r3 + 12],   0
movu          m0,    [r4 + 3]
psrldq        m6,    m0,          1
punpcklbw     m0,    m6

; mode 15 [row 9 - second half]
pmaddubsw     m1,    m0,             [r5 + 22 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                    m1
movh          [r0 + 217 * 16 + 8],   m1
; mode 15 [row 9 - second half] end

; mode 15 [row 10 - second half]
pmaddubsw     m1,    m0,             [r5 + 5 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                    m1
movh          [r0 + 218 * 16 + 8],   m1
; mode 15 [row 10 - second half] end

; mode 16 [row 7 - second half]
pmaddubsw     m1,    m0,             [r5 + 24 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                    m1
movh          [r0 + 231 * 16 + 8],   m1
; mode 16 [row 7 - second half] end

; mode 16 [row 8 - second half]
pmaddubsw     m1,    m0,             [r5 + 3 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                    m1
movh          [r0 + 232 * 16 + 8],   m1
; mode 16 [row 8 - second half] end

pmaddubsw     m1,    m5,          [r5 + 23 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m6,    m0,          [r5 + 23 * 16]
pmulhrsw      m6,    m3
packuswb      m1,                 m6
movu          [r0 + 204 * 16],    m1

; mode 14 [row 13]
pmaddubsw     m1,    m5,          [r5 + 10 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m6,    m0,          [r5 + 10 * 16]
pmulhrsw      m6,    m3
packuswb      m1,                 m6
movu          [r0 + 205 * 16],    m1

; mode 14 [row 14]
pslldq        m5,    2
pinsrb        m5,    [r3 + 12],   1
pinsrb        m5,    [r3 + 15],   0
movu          m0,    [r4 + 2]
psrldq        m6,    m0,          1
punpcklbw     m0,    m6

; mode 15 [row 11 - second half]
pmaddubsw     m1,    m0,             [r5 + 20 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                    m1
movh          [r0 + 219 * 16 + 8],   m1
; mode 15 [row 11 - second half] end

; mode 15 [row 12 - second half]
pmaddubsw     m1,    m0,             [r5 + 3 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                    m1
movh          [r0 + 220 * 16 + 8],   m1
; mode 15 [row 12 - second half] end

; mode 16 [row 9 - second half]
pmaddubsw     m1,    m0,             [r5 + 14 * 16]
pmulhrsw      m1,    m3
packuswb      m1,                    m1
movh          [r0 + 233 * 16 + 8],   m1

; mode 16 [row 9 - second half] end
pmaddubsw     m1,    m5,          [r5 + 29 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m6,    m0,          [r5 + 29 * 16]
pmulhrsw      m6,    m3
packuswb      m1,                 m6
movu          [r0 + 206 * 16],    m1

; mode 14 [row 15]
pmaddubsw     m1,    m5,          [r5 + 16 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m6,    m0,          [r5 + 16 * 16]
pmulhrsw      m6,    m3
packuswb      m1,                 m6
movu          [r0 + 207 * 16],    m1

; mode 12 [row 12]
pslldq        m0,    m2,          2
pinsrb        m0,    [r3 +  6],   1
pinsrb        m0,    [r3 + 13],   0
pmaddubsw     m1,    m0,          [r5 + 31 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m5,    m4,          [r5 + 31 * 16]
pmulhrsw      m5,    m3
packuswb      m1,                 m5
movu          [r0 + 172 * 16],    m1

; mode 12 [row 13]
pmaddubsw     m1,    m0,          [r5 + 26 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m5,    m4,          [r5 + 26 * 16]
pmulhrsw      m5,    m3
packuswb      m1,                 m5
movu          [r0 + 173 * 16],    m1

; mode 12 [row 14]
pmaddubsw     m1,    m0,          [r5 + 21 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m5,    m4,          [r5 + 21 * 16]
pmulhrsw      m5,    m3
packuswb      m1,                 m5
movu          [r0 + 174 * 16],    m1

; mode 12 [row 15]
pmaddubsw     m1,    m0,          [r5 + 16 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m5,    m4,          [r5 + 16 * 16]
pmulhrsw      m5,    m3
packuswb      m1,                 m5
movu          [r0 + 175 * 16],    m1

; mode 13 [row 7]
pslldq        m7,    2
pinsrb        m7,    [r3 +  4],   1
pinsrb        m7,    [r3 +  7],   0
pmaddubsw     m1,    m7,          [r5 + 24 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m5,    m4,          [r5 + 24 * 16]
pmulhrsw      m5,    m3
packuswb      m1,                 m5
movu          [r0 + 183 * 16],    m1

; mode 13 [row 8]
pmaddubsw     m1,    m7,          [r5 + 15 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m5,    m4,          [r5 + 15 * 16]
pmulhrsw      m5,    m3
packuswb      m1,                 m5
movu          [r0 + 184 * 16],    m1

; mode 13 [row 9]
pmaddubsw     m1,    m7,          [r5 + 6 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m5,    m4,          [r5 + 6 * 16]
pmulhrsw      m5,    m3
packuswb      m1,                 m5
movu          [r0 + 185 * 16],    m1

; mode 13 [row 10]
pslldq        m7,    2
pinsrb        m7,    [r3 +  7],   1
pinsrb        m7,    [r3 + 11],   0
pmaddubsw     m1,    m7,          [r5 + 29 * 16]
pmulhrsw      m1,    m3
movu          m4,    [r4 + 5]
psrldq        m5,    m4,         1
punpcklbw     m4,    m5
pmaddubsw     m5,    m4,          [r5 + 29 * 16]
pmulhrsw      m5,    m3
packuswb      m1,    m5
movu          [r0 + 186 * 16],    m1

; mode 13 [row 11]
pmaddubsw     m1,    m7,          [r5 + 20 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m5,    m4,          [r5 + 20 * 16]
pmulhrsw      m5,    m3
packuswb      m1,                 m5
movu          [r0 + 187 * 16],    m1

; mode 13 [row 12]
pmaddubsw     m1,    m7,          [r5 + 11 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m5,    m4,          [r5 + 11 * 16]
pmulhrsw      m5,    m3
packuswb      m1,                 m5
movu          [r0 + 188 * 16],    m1

; mode 13 [row 13]
pmaddubsw     m1,    m7,          [r5 + 2 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m5,    m4,          [r5 + 2 * 16]
pmulhrsw      m5,    m3
packuswb      m1,                 m5
movu          [r0 + 189 * 16],    m1

; mode 13 [row 14]
pslldq        m7,    2
pinsrb        m7,    [r3 + 11],   1
pinsrb        m7,    [r3 + 14],   0
pmaddubsw     m1,    m7,          [r5 + 25 * 16]
pmulhrsw      m1,    m3
movu          m4,    [r4 + 4]
psrldq        m5,    m4,          1
punpcklbw     m4,    m5
pmaddubsw     m5,    m4,          [r5 + 25 * 16]
pmulhrsw      m5,    m3
packuswb      m1,                 m5
movu          [r0 + 190 * 16],    m1

; mode 13 [row 15]
pmaddubsw     m1,    m7,          [r5 + 16 * 16]
pmulhrsw      m1,    m3
pmaddubsw     m5,    m4,          [r5 + 16 * 16]
pmulhrsw      m5,    m3
packuswb      m1,                 m5
movu          [r0 + 191 * 16],    m1

; mode 17 [row 15]
movu         m0,                   [r3]
pshufb       m1,                   m0,       [tab_S1]
movu         [r0 + 255 * 16],      m1
movu         m2,                   [r4]
movd         [r0 + 255 * 16 + 12], m2

; mode 18 [row 0]
movu         [r0 + 256 * 16],      m0

; mode 18 [row 1]
pslldq        m4,              m0,         1
pinsrb        m4,              [r4 + 1],   0
movu          [r0 + 257 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 2],   0
movu          [r0 + 258 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 3],   0
movu          [r0 + 259 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 4],   0
movu          [r0 + 260 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 5],   0
movu          [r0 + 261 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 6],   0
movu          [r0 + 262 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 7],   0
movu          [r0 + 263 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 8],   0
movu          [r0 + 264 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 9],   0
movu          [r0 + 265 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 10],   0
movu          [r0 + 266 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 11],   0
movu          [r0 + 267 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 12],   0
movu          [r0 + 268 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 13],   0
movu          [r0 + 269 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 14],   0
movu          [r0 + 270 * 16], m4
pslldq        m4,              1
pinsrb        m4,              [r4 + 15],   0
movu          [r0 + 271 * 16], m4

; mode 19 [row 0]
psrldq        m2,    m0,           1
punpcklbw     m0,    m2
movu          m5,    [r3 + 8]
psrldq        m6,    m5,           1
punpcklbw     m5,    m6
pmaddubsw     m4,    m0,           [r5 + 6 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,           [r5 + 6 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 272 * 16],     m4

; mode 20 [row 0]
pmaddubsw     m4,    m0,           [r5 + 11 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,           [r5 + 11 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 288 * 16],     m4

; mode 21 [row 0]
pmaddubsw     m4,    m0,            [r5 + 15 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,            [r5 + 15 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 304 * 16],     m4

; mode 22 [row 0]
pmaddubsw     m4,    m0,           [r5 + 19 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,           [r5 + 19 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 320 * 16],     m4

; mode 22 [row 1]
pmaddubsw     m4,    m0,           [r5 + 6 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,           [r5 + 6 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 321 * 16],     m4

; mode 23 [row 0]
pmaddubsw     m4,    m0,           [r5 + 23 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,           [r5 + 23 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 336 * 16],     m4

; mode 23 [row 1]
pmaddubsw     m4,    m0,           [r5 + 14 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,           [r5 + 14 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 337 * 16],     m4

; mode 23 [row 2]
pmaddubsw     m4,    m0,           [r5 + 5 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,           [r5 + 5 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 338 * 16],     m4

; mode 24 [row 0]
pmaddubsw     m4,    m0,           [r5 + 27 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,           [r5 + 27 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 352 * 16],     m4

; mode 24 [row 1]
pmaddubsw     m4,    m0,            [r5 + 22 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,            [r5 + 22 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 353 * 16],     m4

; mode 24 [row 2]
pmaddubsw     m4,    m0,           [r5 + 17 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,           [r5 + 17 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 354 * 16],     m4

; mode 24 [row 3]
pmaddubsw     m4,    m0,           [r5 + 12 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,           [r5 + 12 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 355 * 16],     m4

; mode 24 [row 4]
pmaddubsw     m4,    m0,            [r5 + 7 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,            [r5 + 7 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 356 * 16],     m4

; mode 24 [row 5]
pmaddubsw     m4,    m0,           [r5 + 2 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,           [r5 + 2 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                  m6
movu          [r0 + 357 * 16],     m4

; mode 24 [row 6 - first half]
pslldq        m7,    m0,    2
pinsrb        m7,    [r4 + 0],     1
pinsrb        m7,    [r4 + 6],     0
pmaddubsw     m4,    m7,           [r5 + 29 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 358 * 16],     m4

; mode 24 [row 7 - first half]
pmaddubsw     m4,    m7,           [r5 + 24 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 359 * 16],     m4

; mode 24 [row 8 - first half]
pmaddubsw     m4,    m7,           [r5 + 19 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 360 * 16],     m4

; mode 24 [row 9 - first half]
pmaddubsw     m4,    m7,           [r5 + 14 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 361 * 16],     m4

; mode 24 [row 10 - first half]
pmaddubsw     m4,    m7,           [r5 + 9 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 362 * 16],     m4

; mode 24 [row 11 - first half]
pmaddubsw     m4,    m7,           [r5 + 4 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 363 * 16],     m4

; mode 24 [row 12 - first half]
pslldq        m7,    2
pinsrb        m7,    [r4 +  6],    1
pinsrb        m7,    [r4 + 13],    0
pmaddubsw     m4,    m7,           [r5 + 31 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 364 * 16],     m4

; mode 24 [row 13 - first half]
pmaddubsw     m4,    m7,           [r5 + 26 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 365 * 16],     m4

; mode 24 [row 14 - first half]
pmaddubsw     m4,    m7,           [r5 + 21 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 366 * 16],     m4

; mode 24 [row 15 - first half]
pmaddubsw     m4,    m7,           [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 367 * 16],     m4

; mode 23 [row 3 - first half]
pslldq        m7,    m0,    2
pinsrb        m7,    [r4 + 0],     1
pinsrb        m7,    [r4 + 4],     0
pmaddubsw     m4,    m7,           [r5 + 28 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 339 * 16],     m4

; mode 23 [row 4 - first half]
pmaddubsw     m4,    m7,           [r5 + 19 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 340 * 16],     m4

; mode 23 [row 5 - first half]
pmaddubsw     m4,    m7,           [r5 + 10 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 341 * 16],     m4

; mode 23 [row 6 - first half]
pmaddubsw     m4,    m7,           [r5 + 1 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 342 * 16],     m4

; mode 23 [row 7 - first half]
pslldq        m7,    2
pinsrb        m7,    [r4 + 4],     1
pinsrb        m7,    [r4 + 7],     0
pmaddubsw     m4,    m7,            [r5 + 24 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 343 * 16],     m4

; mode 23 [row 8 - first half]
pmaddubsw     m4,    m7,           [r5 + 15 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 344 * 16],     m4

; mode 23 [row 9 - first half]
pmaddubsw     m4,    m7,           [r5 + 6 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 345 * 16],     m4

; mode 23 [row 10 - first half]
pslldq        m7,    2
pinsrb        m7,    [r4 +  7],    1
pinsrb        m7,    [r4 + 11],    0
pmaddubsw     m4,    m7,           [r5 + 29 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 346 * 16],     m4

; mode 23 [row 11 - first half]
pmaddubsw     m4,    m7,           [r5 + 20 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 347 * 16],     m4

; mode 23 [row 12 - first half]
pmaddubsw     m4,    m7,           [r5 + 11 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 348 * 16],     m4

; mode 23 [row 13 - first half]
pmaddubsw     m4,    m7,           [r5 + 2 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 349 * 16],     m4

; mode 23 [row 14 - first half]
pslldq        m7,    2
pinsrb        m7,    [r4 + 11],   1
pinsrb        m7,    [r4 + 14],   0
pmaddubsw     m4,    m7,           [r5 + 25 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 350 * 16],     m4

; mode 23 [row 15 - first half]
pmaddubsw     m4,    m7,           [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 351 * 16],     m4

; mode 21 [row 15 - first half]
pmaddubsw     m4,    m0,         [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 319 * 16 + 8], m4
; mode 21 [row 15 - second half] end

; mode 20 [row 1 - first half]
pslldq        m7,    m0,    2
pinsrb        m7,    [r4 + 0],   1
pinsrb        m7,    [r4 + 2],   0
pmaddubsw     m4,    m7,           [r5 + 22 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 289 * 16],     m4

; mode 20 [row 2 - first half]
pmaddubsw     m4,    m7,           [r5 + 1 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 290 * 16],     m4

; mode 21 [row 1 - first half]
pmaddubsw     m4,    m7,           [r5 + 30 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 305 * 16],     m4

; mode 21 [row 2 - first half]
pmaddubsw     m4,    m7,           [r5 + 13 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 306 * 16],     m4

; mode 22 [row 2 - first half]
pmaddubsw     m4,    m7,           [r5 + 25 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 322 * 16],     m4

; mode 22 [row 3 - first half]
pmaddubsw     m4,    m7,           [r5 + 12 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 323 * 16],     m4

; mode 22 [row 4 - first half]
pslldq        m1,    m7,    2
pinsrb        m1,    [r4 + 2],     1
pinsrb        m1,    [r4 + 5],     0
pmaddubsw     m4,    m1,           [r5 + 31 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 324 * 16],     m4

; mode 22 [row 5 - first half]
pmaddubsw     m4,    m1,           [r5 + 18 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 325 * 16],     m4

; mode 22 [row 6 - first half]
pmaddubsw     m4,    m1,           [r5 + 5 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 326 * 16],     m4

; mode 22 [row 7 - first half]
pslldq        m1,    2
pinsrb        m1,    [r4 + 5],     1
pinsrb        m1,    [r4 + 7],     0
pmaddubsw     m4,    m1,           [r5 + 24 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 327 * 16],     m4

; mode 22 [row 8 - first half]
pmaddubsw     m4,    m1,           [r5 + 11 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 328 * 16],     m4

; mode 22 [row 9 - first half]
pslldq        m1,    2
pinsrb        m1,    [r4 +  7],    1
pinsrb        m1,    [r4 + 10],    0
pmaddubsw     m4,    m1,           [r5 + 30 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 329 * 16],     m4

; mode 22 [row 10 - first half]
pmaddubsw     m4,    m1,           [r5 + 17 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 330 * 16],     m4

; mode 22 [row 11 - first half]
pmaddubsw     m4,    m1,           [r5 + 4 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 331 * 16],     m4

; mode 22 [row 12 - first half]
pslldq        m1,    2
pinsrb        m1,    [r4 + 10],    1
pinsrb        m1,    [r4 + 12],    0
pmaddubsw     m4,    m1,           [r5 + 23 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 332 * 16],     m4

; mode 22 [row 13 - first half]
pmaddubsw     m4,    m1,           [r5 + 10 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 333 * 16],     m4

; mode 22 [row 14 - first half]
pslldq        m1,    2
pinsrb        m1,    [r4 + 12],   1
pinsrb        m1,    [r4 + 15],   0
pmaddubsw     m4,    m1,          [r5 + 29 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 334 * 16],     m4

; mode 22 [row 15 - first half]
pmaddubsw     m4,    m1,           [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 335 * 16],     m4

; mode 21 [row 3 - first half]
pslldq        m6,    m7,    2
pinsrb        m6,    [r4 + 2],     1
pinsrb        m6,    [r4 + 4],     0
pmaddubsw     m4,    m6,           [r5 + 28 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 307 * 16],     m4

; mode 21 [row 4 - first half]
pmaddubsw     m4,    m6,            [r5 + 11 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 308 * 16],     m4

; mode 21 [row 5 - first half]
pslldq        m6,    2
pinsrb        m6,    [r4 + 4],     1
pinsrb        m6,    [r4 + 6],     0
pmaddubsw     m4,    m6,           [r5 + 26 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 309 * 16],     m4

; mode 21 [row 6 - first half]
pmaddubsw     m4,    m6,           [r5 + 9 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 310 * 16],     m4

; mode 21 [row 7 - first half]
pslldq        m6,    2
pinsrb        m6,    [r4 + 6],     1
pinsrb        m6,    [r4 + 8],     0
pmaddubsw     m4,    m6,           [r5 + 24 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 311 * 16],     m4

; mode 21 [row 8 - first half]
pmaddubsw     m4,    m6,           [r5 + 7 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 312 * 16],     m4

; mode 21 [row 9 - first half]
pslldq        m6,    2
pinsrb        m6,    [r4 + 8],     1
pinsrb        m6,    [r4 + 9],     0
pmaddubsw     m4,    m6,            [r5 + 22 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 313 * 16],     m4

; mode 21 [row 10 - first half]
pmaddubsw     m4,    m6,            [r5 + 5 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 314 * 16],     m4

; mode 21 [row 11 - first half]
pslldq        m6,    2
pinsrb        m6,    [r4 +  9],    1
pinsrb        m6,    [r4 + 11],    0
pmaddubsw     m4,    m6,           [r5 + 20 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 315 * 16],     m4

; mode 21 [row 12 - first half]
pmaddubsw     m4,    m6,           [r5 + 3 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 316 * 16],     m4

; mode 21 [row 13 - first half]
pslldq        m6,    2
pinsrb        m6,    [r4 + 11],    1
pinsrb        m6,    [r4 + 13],    0
pmaddubsw     m4,    m6,           [r5 + 18 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 317 * 16],     m4

; mode 21 [row 14 - first half]
pmaddubsw     m4,    m6,           [r5 + 1 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 318 * 16],     m4

; mode 21 [row 15 - first half]
pslldq        m6,    2
pinsrb        m6,    [r4 + 13],    1
pinsrb        m6,    [r4 + 15],    0
pmaddubsw     m4,    m6,           [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 319 * 16],     m4

; mode 20 [row 13 - second half]
pmaddubsw     m4,    m7,           [r5 + 26 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 301 * 16 + 8], m4
; mode 20 [row 13 - second half]

; mode 20 [row 14 - second half]
pmaddubsw     m4,    m7,           [r5 + 5 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 302 * 16 + 8], m4
; mode 20 [row 14 - second half]

; mode 20 [row 3 - first half]
pslldq        m7,    2
pinsrb        m7,    [r4 + 2],    1
pinsrb        m7,    [r4 + 3],    0
pmaddubsw     m4,    m7,           [r5 + 12 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 291 * 16],     m4

; mode 20 [row 15 - second half]
pmaddubsw     m4,    m7,           [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 303 * 16 + 8], m4
; mode 20 [row 15 - second half]

; mode 20 [row 4 - first half]
pslldq        m7,    2
pinsrb        m7,    [r4 + 3],     1
pinsrb        m7,    [r4 + 5],     0
pmaddubsw     m4,    m7,           [r5 + 23 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 292 * 16],     m4

; mode 20 [row 5 - first half]
pmaddubsw     m4,    m7,           [r5 + 2 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 293 * 16],     m4

; mode 20 [row 6 - first half]
pslldq        m7,    2
pinsrb        m7,    [r4 + 5],     1
pinsrb        m7,    [r4 + 6],     0
pmaddubsw     m4,    m7,           [r5 + 13 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 294 * 16],     m4

; mode 20 [row 7 - first half]
pslldq        m7,    2
pinsrb        m7,    [r4 + 6],   1
pinsrb        m7,    [r4 + 8],   0
pmaddubsw     m4,    m7,           [r5 + 24 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 295 * 16],     m4

; mode 20 [row 8 - first half]
pmaddubsw     m4,    m7,           [r5 + 3 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 296 * 16],     m4

; mode 20 [row 9 - first half]
pslldq        m7,    2
pinsrb        m7,    [r4 + 8],   1
pinsrb        m7,    [r4 + 9],   0
pmaddubsw     m4,    m7,           [r5 + 14 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 297 * 16],     m4

; mode 20 [row 10 - first half]
pslldq        m7,    2
pinsrb        m7,    [r4 +  9],   1
pinsrb        m7,    [r4 + 11],   0
pmaddubsw     m4,    m7,           [r5 + 25 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 298 * 16],     m4

; mode 20 [row 11 - first half]
pmaddubsw     m4,    m7,           [r5 + 4 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 299 * 16],     m4

; mode 20 [row 12 - first half]
movu          m1,    [r5 + 15 * 16]
pslldq        m7,    2
pinsrb        m7,    [r4 + 11],   1
pinsrb        m7,    [r4 + 12],   0
pmaddubsw     m4,    m7,           [r5 + 15 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 300 * 16],     m4

; mode 20 [row 13 - first half]
pslldq        m7,    2
pinsrb        m7,    [r4 + 12],   1
pinsrb        m7,    [r4 + 14],   0
pmaddubsw     m4,    m7,           [r5 + 26 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 301 * 16],     m4

; mode 20 [row 14 - first half]
pmaddubsw     m4,    m7,           [r5 + 5 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 302 * 16],     m4

; mode 20 [row 15 - first half]
pslldq        m7,    2
pinsrb        m7,    [r4 + 14],    1
pinsrb        m7,    [r4 + 15],    0
pmaddubsw     m4,    m7,           [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 303 * 16],     m4

; mode 19 [row 1]
pslldq        m0,    2
pinsrb        m0,    [r4 + 0],   1
pinsrb        m0,    [r4 + 1],   0
pslldq        m5,    2
pinsrb        m5,    [r3 + 8],   1
pinsrb        m5,    [r3 + 7],   0

; mode 20 [row 1 - second half]
pmaddubsw     m4,    m5,           [r5 + 22 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 289 * 16 + 8], m4
; mode 20 [row 1 - second half] end

; mode 20 [row 2 - second half]
pmaddubsw     m4,    m5,           [r5 + 1 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 290 * 16 + 8], m4
; mode 20 [row 2 - second half] end

; mode 21 [row 2 - second half]
pmaddubsw     m4,    m5,           [r5 + 30 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 305 * 16 + 8], m4
; mode 21 [row 2 - second half] end

; mode 21 [row 3 - second half]
pmaddubsw     m4,    m5,           [r5 + 13 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 306 * 16 + 8], m4
; mode 21 [row 3 - second half] end

; mode 21 [row 4 - second half]
pmaddubsw     m4,    m5,           [r5 + 11 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 307 * 16 + 8], m4
; mode 21 [row 4 - second half] end

; mode 22 [row 2 - second half]
pmaddubsw     m4,    m5,           [r5 + 25 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 322 * 16 + 8], m4
; mode 22 [row 2 - second half] end

; mode 22 [row 3 - second half]
pmaddubsw     m4,    m5,           [r5 + 12 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 323 * 16 + 8], m4
; mode 22 [row 3 - second half] end

; mode 23 [row 3 - second half]
pmaddubsw     m4,    m5,          [r5 + 28 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 339 * 16 + 8], m4
; mode 23 [row 3 - second half] end

; mode 23 [row 4 - second half]
pmaddubsw     m4,    m5,          [r5 + 19 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 340 * 16 + 8], m4
; mode 23 [row 4 - second half] end

; mode 23 [row 5 - second half]
pmaddubsw     m4,    m5,          [r5 + 10 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 341 * 16 + 8], m4
; mode 23 [row 5 - second half] end

; mode 23 [row 6 - second half]
pmaddubsw     m4,    m5,          [r5 + 1 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 342 * 16 + 8], m4
; mode 23 [row 6 - second half] end

; mode 24 [row 6 - second half]
pmaddubsw     m4,    m5,           [r5 + 29 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 358 * 16 + 8], m4
; mode 24 [row 6 - second half] end

; mode 24 [row 7 - second half]
pmaddubsw     m4,    m5,           [r5 + 24 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 359 * 16 + 8], m4
; mode 24 [row 7 - second half] end

; mode 24 [row 8 - second half]
pmaddubsw     m4,    m5,           [r5 + 19 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 360 * 16 + 8], m4
; mode 24 [row 8 - second half] end

; mode 24 [row 9 - second half]
pmaddubsw     m4,    m5,           [r5 + 14 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 361 * 16 + 8], m4
; mode 24 [row 9 - second half] end

; mode 24 [row 10 - second half]
pmaddubsw     m4,    m5,           [r5 + 9 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 362 * 16 + 8], m4
; mode 24 [row 10 - second half] end

; mode 24 [row 11 - second half]
pmaddubsw     m4,    m5,           [r5 + 4 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 363 * 16 + 8], m4
; mode 24 [row 11 - second half] end

pmaddubsw     m4,    m0,         [r5 + 12 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,         [r5 + 12 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 273 * 16],   m4

; mode 19 [row 2]
pslldq        m0,    2
pinsrb        m0,    [r4 + 1],   1
pinsrb        m0,    [r4 + 2],   0
pslldq        m5,    2
pinsrb        m5,    [r3 + 7],   1
pinsrb        m5,    [r3 + 6],   0

; mode 20 [row 3 - second half]
pmaddubsw     m4,    m5,            [r5 + 12 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                   m4
movh          [r0 + 291 * 16 + 8], m4
; mode 20 [row 3 - second half] end

; mode 21 [row 3 - second half]
pmaddubsw     m4,    m5,           [r5 + 28 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 307 * 16 + 8], m4
; mode 21 [row 3 - second half] end

; mode 21 [row 4 - second half]
pmaddubsw     m4,    m5,           [r5 + 11 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 308 * 16 + 8], m4
; mode 21 [row 4 - second half] end

; mode 22 [row 4 - second half]
pmaddubsw     m4,    m5,           [r5 + 31 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 324 * 16 + 8], m4
; mode 22 [row 4 - second half] end

; mode 22 [row 5 - second half]
pmaddubsw     m4,    m5,           [r5 + 18 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 325 * 16 + 8], m4
; mode 22 [row 5 - second half] end

; mode 22 [row 6 - second half]
pmaddubsw     m4,    m5,           [r5 + 5 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 326 * 16 + 8], m4
; mode 22 [row 6 - second half] end

; mode 23 [row 7 - second half]
pmaddubsw     m4,    m5,           [r5 + 24 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 343 * 16 + 8], m4
; mode 23 [row 7 - second half] end

; mode 23 [row 8 - second half]
pmaddubsw     m4,    m5,           [r5 + 15 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 344 * 16 + 8], m4
; mode 23 [row 8 - second half] end

; mode 23 [row 9 - second half]
pmaddubsw     m4,    m5,           [r5 + 6 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 345 * 16 + 8], m4
; mode 23 [row 9 - second half] end

; mode 24 [row 12 - second half]
pmaddubsw     m4,    m5,          [r5 + 31 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 364 * 16 + 8], m4
; mode 24 [row 12 - second half] end

; mode 24 [row 13 - second half]
pmaddubsw     m4,    m5,          [r5 + 26 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 365 * 16 + 8], m4
; mode 24 [row 13 - second half] end

; mode 24 [row 14 - second half]
pmaddubsw     m4,    m5,          [r5 + 21 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 366 * 16 + 8], m4
; mode 24 [row 14 - second half] end

; mode 24 [row 15 - second half]
pmaddubsw     m4,    m5,          [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 367 * 16 + 8], m4
; mode 24 [row 15 - second half] end

pmaddubsw     m4,    m0,         [r5 + 18 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,         [r5 + 18 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 274 * 16],   m4

; mode 19 [row 3]
pslldq        m0,    2
pinsrb        m0,    [r4 + 2],   1
pinsrb        m0,    [r4 + 4],   0
pslldq        m5,    2
pinsrb        m5,    [r3 + 6],   1
pinsrb        m5,    [r3 + 5],   0

; mode 20 [row 4 - second half]
pmaddubsw     m4,    m5,           [r5 + 23 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 292 * 16 + 8], m4
; mode 20 [row 4 - second half] end

; mode 20 [row 5 - second half]
pmaddubsw     m4,    m5,           [r5 + 2 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 293 * 16 + 8], m4
; mode 20 [row 5 - second half] end

; mode 21 [row 5 - second half]
pmaddubsw     m4,    m5,          [r5 + 26 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 309 * 16 + 8], m4
; mode 21 [row 5 - second half] end

; mode 21 [row 6 - second half]
pmaddubsw     m4,    m5,          [r5 + 9 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 310 * 16 + 8], m4
; mode 21 [row 6 - second half] end

; mode 22 [row 7 - second half]
pmaddubsw     m4,    m5,           [r5 + 24 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 327 * 16 + 8], m4
; mode 22 [row 7 - second half] end

; mode 22 [row 8 - second half]
pmaddubsw     m4,    m5,           [r5 + 11 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 328 * 16 + 8], m4
; mode 22 [row 7 - second half] end

; mode 23 [row 10 - second half]
pmaddubsw     m4,    m5,           [r5 + 29 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 346 * 16 + 8], m4
; mode 23 [row 10 - second half] end

; mode 23 [row 11 - second half]
pmaddubsw     m4,    m5,           [r5 + 20 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 347 * 16 + 8], m4
; mode 23 [row 11 - second half] end

; mode 23 [row 12 - second half]
pmaddubsw     m4,    m5,           [r5 + 11 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 348 * 16 + 8], m4
; mode 23 [row 12 - second half] end

; mode 23 [row 13 - second half]
pmaddubsw     m4,    m5,           [r5 + 2 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 349 * 16 + 8], m4
; mode 23 [row 13 - second half] end

pmaddubsw     m4,    m0,         [r5 + 24 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,         [r5 + 24 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 275 * 16],   m4

; mode 19 [row 4]
pslldq        m0,    2
pinsrb        m0,    [r4 + 4],   1
pinsrb        m0,    [r4 + 5],   0
pslldq        m5,    2
pinsrb        m5,    [r3 + 5],   1
pinsrb        m5,    [r3 + 4],   0

; mode 20 [row 6 - second half]
pmaddubsw     m4,    m5,           [r5 + 13 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 294 * 16 + 8], m4
; mode 20 [row 6 - second half] end

; mode 21 [row 7 - second half]
pmaddubsw     m4,    m5,          [r5 + 24 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 311 * 16 + 8], m4
; mode 21 [row 7 - second half] end

; mode 21 [row 8 - second half]
pmaddubsw     m4,    m5,          [r5 + 7 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 312 * 16 + 8], m4
; mode 21 [row 8 - second half] end

; mode 22 [row 9 - second half]
pmaddubsw     m4,    m5,           [r5 + 30 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 329 * 16 + 8], m4
; mode 22 [row 9 - second half] end

; mode 22 [row 10 - second half]
pmaddubsw     m4,    m5,           [r5 + 17 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 330 * 16 + 8], m4
; mode 22 [row 10 - second half] end

; mode 22 [row 11 - second half]
pmaddubsw     m4,    m5,           [r5 + 4 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 331 * 16 + 8], m4
; mode 22 [row 11 - second half] end

; mode 23 [row 14 - second half]
pmaddubsw     m4,    m5,           [r5 + 25 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 350 * 16 + 8], m4
; mode 23 [row 14 - second half] end

; mode 23 [row 15 - second half]
pmaddubsw     m4,    m5,           [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 351 * 16 + 8], m4

; mode 23 [row 15 - second half] end
pmaddubsw     m4,    m0,         [r5 + 30 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,         [r5 + 30 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 276 * 16],   m4

; mode 19 [row 5]
pmaddubsw     m4,    m0,         [r5 + 4 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,         [r5 + 4 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 277 * 16],   m4

; mode 19 [row 6]
pslldq        m0,    2
pinsrb        m0,    [r4 + 5],   1
pinsrb        m0,    [r4 + 6],   0
pslldq        m5,    2
pinsrb        m5,    [r3 + 4],   1
pinsrb        m5,    [r3 + 3],   0

; mode 20 [row 7 - second half]
pmaddubsw     m4,    m5,           [r5 + 24 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 295 * 16 + 8], m4
; mode 20 [row 7 - second half] end

; mode 20 [row 8 - second half]
pmaddubsw     m4,    m5,           [r5 + 3 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 296 * 16 + 8], m4
; mode 20 [row 8 - second half] end

; mode 21 [row 9 - second half]
pmaddubsw     m4,    m5,           [r5 + 22 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 313 * 16 + 8], m4
; mode 21 [row 9 - second half] end

; mode 21 [row 10 - second half]
pmaddubsw     m4,    m5,           [r5 + 5 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 314 * 16 + 8], m4
; mode 21 [row 10 - second half] end

; mode 22 [row 12 - second half]
pmaddubsw     m4,    m5,           [r5 + 23 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 332 * 16 + 8], m4
; mode 22 [row 12 - second half] end

; mode 22 [row 12 - second half]
pmaddubsw     m4,    m5,           [r5 + 10 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 333 * 16 + 8], m4
; mode 22 [row 12 - second half] end

pmaddubsw     m4,    m0,          [r5 + 10 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,          [r5 + 10 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 278 * 16],   m4

; mode 19 [row 7]
pslldq        m0,    2
pinsrb        m0,    [r4 + 6],   1
pinsrb        m0,    [r4 + 7],   0
pslldq        m5,    2
pinsrb        m5,    [r3 + 3],   1
pinsrb        m5,    [r3 + 2],   0

; mode 20 [row 9 - second half]
pmaddubsw     m4,    m5,           [r5 + 14 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 297 * 16 + 8], m4
; mode 20 [row 9 - second half]

; mode 21 [row 11 - second half]
pmaddubsw     m4,    m5,           [r5 + 20 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 315 * 16 + 8], m4
; mode 21 [row 11 - second half] end

; mode 21 [row 12 - second half]
pmaddubsw     m4,    m5,           [r5 + 3 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 316 * 16 + 8], m4
; mode 21 [row 12 - second half] end

; mode 22 [row 14 - second half]
pmaddubsw     m4,    m5,           [r5 + 29 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 334 * 16 + 8], m4
; mode 22 [row 14 - second half] end

; mode 22 [row 15 - second half]
pmaddubsw     m4,    m5,           [r5 + 16 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 335 * 16 + 8], m4
; mode 22 [row 15 - second half] end

pmaddubsw     m4,    m0,         [r5 + 16 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,         [r5 + 16 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 279 * 16],   m4

; mode 19 [row 8]
pslldq        m0,    2
pinsrb        m0,    [r4 + 7],   1
pinsrb        m0,    [r4 + 9],   0
pslldq        m5,    2
pinsrb        m5,    [r3 + 2],   1
pinsrb        m5,    [r3 + 1],   0

; mode 20 [row 10 - second half]
pmaddubsw     m4,    m5,           [r5 + 25 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 298 * 16 + 8], m4
; mode 20 [row 10 - second half] end

; mode 20 [row 11 - second half]
pmaddubsw     m4,    m5,           [r5 + 4 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 299 * 16 + 8], m4
; mode 20 [row 11 - second half] end

; mode 21 [row 13 - second half]
pmaddubsw     m4,    m5,           [r5 + 18 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 317 * 16 + 8], m4
; mode 21 [row 13 - second half] end

; mode 21 [row 14 - second half]
pmaddubsw     m4,    m5,           [r5 + 1 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 318 * 16 + 8], m4
; mode 21 [row 14 - second half] end

pmaddubsw     m4,    m0,         [r5 + 22 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,         [r5 + 22 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 280 * 16],   m4

; mode 19 [row 9]
pslldq        m0,    2
pinsrb        m0,    [r4 + 9],   1
pinsrb        m0,    [r4 + 10],   0
pslldq        m5,    2
pinsrb        m5,    [r3 + 1],   1
pinsrb        m5,    [r3 + 0],   0

; mode 20 [row 12 - second half]
pmaddubsw     m4,    m5,           [r5 + 15 * 16]
pmulhrsw      m4,    m3
packuswb      m4,                  m4
movh          [r0 + 300 * 16 + 8], m4

; mode 20 [row 12 - second half] end
pmaddubsw     m4,    m0,          [r5 + 28 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,          [r5 + 28 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 281 * 16],   m4

; mode 19 [row 10]
pmaddubsw     m4,    m0,         [r5 + 2 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,         [r5 + 2 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 282 * 16],   m4

; mode 19 [row 11]
pslldq        m0,    2
pinsrb        m0,    [r4 + 10],   1
pinsrb        m0,    [r4 + 11],   0
pmaddubsw     m4,    m0,         [r5 + 8 * 16]
pmulhrsw      m4,    m3
pslldq        m5,    2
pinsrb        m5,    [r4 + 0],   1
pinsrb        m5,    [r4 + 1],   0
pmaddubsw     m6,    m5,         [r5 + 8 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 283 * 16],   m4

; mode 19 [row 12]
pslldq        m0,    2
pinsrb        m0,    [r4 + 11],   1
pinsrb        m0,    [r4 + 12],   0
pslldq        m5,    2
pinsrb        m5,    [r4 + 1],   1
pinsrb        m5,    [r4 + 2],   0
pmaddubsw     m4,    m0,         [r5 + 14 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m6,    m5,         [r5 + 14 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 284 * 16],   m4

; mode 19 [row 13]
pslldq        m0,    2
pinsrb        m0,    [r4 + 12],   1
pinsrb        m0,    [r4 + 14],   0
pmaddubsw     m4,    m0,         [r5 + 20 * 16]
pmulhrsw      m4,    m3
pslldq        m5,    2
pinsrb        m5,    [r4 + 2],   1
pinsrb        m5,    [r4 + 4],   0
pmaddubsw     m6,    m5,         [r5 + 20 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 285 * 16],   m4

; mode 19 [row 14]
pslldq        m0,    2
pinsrb        m0,    [r4 + 14],   1
pinsrb        m0,    [r4 + 15],   0
pmaddubsw     m4,    m0,         [r5 + 26 * 16]
pmulhrsw      m4,    m3
pslldq        m5,    2
pinsrb        m5,    [r4 + 4],   1
pinsrb        m5,    [r4 + 5],   0
pmaddubsw     m6,    m5,         [r5 + 26 * 16]
pmulhrsw      m6,    m3
packuswb      m4,                m6
movu          [r0 + 286 * 16],   m4

; mode 19 [row 15]
movu         m0,                   [r4]
pshufb       m0,                   [tab_S1]
movu         [r0 + 287 * 16],      m0
movd         m1,                   [r3]
movd         [r0 + 287 * 16 + 12], m1

; mode 25
movu          m1,    [r1]

; mode 26 [all rows]
psrldq        m6,    m1,         1
pinsrb        m6,    [r1 + 16], 15
movu          m7,    m6
movu         [r0 + 384 * 16],   m6
movu         [r0 + 385 * 16],   m6
movu         [r0 + 386 * 16],   m6
movu         [r0 + 387 * 16],   m6
movu         [r0 + 388 * 16],   m6
movu         [r0 + 389 * 16],   m6
movu         [r0 + 390 * 16],   m6
movu         [r0 + 391 * 16],   m6
movu         [r0 + 392 * 16],   m6
movu         [r0 + 393 * 16],   m6
movu         [r0 + 394 * 16],   m6
movu         [r0 + 395 * 16],   m6
movu         [r0 + 396 * 16],   m6
movu         [r0 + 397 * 16],   m6
movu         [r0 + 398 * 16],   m6
movu         [r0 + 399 * 16],   m6

pxor         m0,          m0
pshufb       m6,          m6,         m0
punpcklbw    m6,          m0
movu         m2,          [r2]
pshufb       m2,          m2,         m0
punpcklbw    m2,          m0
movu         m4,          [r2 + 1]
punpcklbw    m5,          m4,         m0
punpckhbw    m4,          m0
psubw        m5,          m2
psubw        m4,          m2
psraw        m5,          1
psraw        m4,          1
paddw        m5,          m6
paddw        m4,          m6
packuswb     m5,          m4

pextrb       [r0 + 384 * 16],  m5,          0
pextrb       [r0 + 385 * 16],  m5,          1
pextrb       [r0 + 386 * 16],  m5,          2
pextrb       [r0 + 387 * 16],  m5,          3
pextrb       [r0 + 388 * 16],  m5,          4
pextrb       [r0 + 389 * 16],  m5,          5
pextrb       [r0 + 390 * 16],  m5,          6
pextrb       [r0 + 391 * 16],  m5,          7
pextrb       [r0 + 392 * 16],  m5,          8
pextrb       [r0 + 393 * 16],  m5,          9
pextrb       [r0 + 394 * 16],  m5,          10
pextrb       [r0 + 395 * 16],  m5,          11
pextrb       [r0 + 396 * 16],  m5,          12
pextrb       [r0 + 397 * 16],  m5,          13
pextrb       [r0 + 398 * 16],  m5,          14
pextrb       [r0 + 399 * 16],  m5,          15

; mode 25 [row 15]
movu          [r0 + 383 * 16],     m1

; mode 25 [row 0]
psrldq        m2,    m1,           1
punpcklbw     m1,    m2
movu          m2,    [r1 + 8]
psrldq        m4,    m2,           1
punpcklbw     m2,    m4
pmaddubsw     m4,    m1,           [r5 + 30 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,           [r5 + 30 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 368 * 16],     m4

; mode 25 [row 1]
pmaddubsw     m4,    m1,            [r5 + 28 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,            [r5 + 28 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 369 * 16],     m4

; mode 25 [row 2]
pmaddubsw     m4,    m1,            [r5 + 26 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,            [r5 + 26 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 370 * 16],     m4

; mode 25 [row 3]
pmaddubsw     m4,    m1,            [r5 + 24 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,            [r5 + 24 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 371 * 16],     m4

; mode 25 [row 4]
pmaddubsw     m4,    m1,           [r5 + 22 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,           [r5 + 22 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 372 * 16],     m4

; mode 25 [row 5]
pmaddubsw     m4,    m1,           [r5 + 20 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,           [r5 + 20 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 373 * 16],     m4

; mode 25 [row 6]
pmaddubsw     m4,    m1,            [r5 + 18 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,            [r5 + 18 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 374 * 16],     m4

; mode 25 [row 7]
pmaddubsw     m4,    m1,            [r5 + 16 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,            [r5 + 16 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 375 * 16],     m4

; mode 25 [row 8]
pmaddubsw     m4,    m1,           [r5 + 14 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,           [r5 + 14 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 376 * 16],     m4

; mode 25 [row 9]
pmaddubsw     m4,    m1,            [r5 + 12 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,            [r5 + 12 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 377 * 16],     m4

; mode 25 [row 10]
pmaddubsw     m4,    m1,           [r5 + 10 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,           [r5 + 10 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 378 * 16],     m4

; mode 25 [row 11]
pmaddubsw     m4,    m1,             [r5 + 8 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,             [r5 + 8 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 379 * 16],     m4

; mode 25 [row 12]
pmaddubsw     m4,    m1,            [r5 + 6 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,            [r5 + 6 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 380 * 16],     m4

; mode 25 [row 13]
pmaddubsw     m4,    m1,           [r5 + 4 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,           [r5 + 4 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 381 * 16],     m4

; mode 25 [row 14]
pmaddubsw     m4,    m1,           [r5 + 2 * 16]
pmulhrsw      m4,    m3
pmaddubsw     m5,    m2,           [r5 + 2 * 16]
pmulhrsw      m5,    m3
packuswb      m4,                  m5
movu          [r0 + 382 * 16],     m4

; mode 27 [row 15]
psrldq        m6,    m7,           1
punpcklbw     m7,    m6
pinsrb        m6,    [r1 + 17],    15
movu          [r0 + 415 * 16],     m6

; mode 27 [row 0]
movu          m4,    [r1 + 9]
psrldq        m5,    m4,           1
punpcklbw     m4,    m5
pmaddubsw     m6,    m7,           [r5 + 2 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 2 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 400 * 16],     m6

; mode 27 [row 1]
pmaddubsw     m6,    m7,           [r5 + 4 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 4 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 401 * 16],     m6

; mode 27 [row 2]
pmaddubsw     m6,    m7,           [r5 + 6 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 6 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 402 * 16],     m6

; mode 27 [row 3]
pmaddubsw     m6,    m7,           [r5 + 8 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 8 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 403 * 16],     m6

; mode 27 [row 4]
pmaddubsw     m6,    m7,           [r5 + 10 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 10 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 404 * 16],     m6

; mode 27 [row 5]
pmaddubsw     m6,    m7,           [r5 + 12 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 12 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 405 * 16],     m6

; mode 27 [row 6]
pmaddubsw     m6,    m7,           [r5 + 14 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 14 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 406 * 16],     m6

; mode 27 [row 7]
pmaddubsw     m6,    m7,           [r5 + 16 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 16 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 407 * 16],     m6

; mode 27 [row 8]
pmaddubsw     m6,    m7,            [r5 + 18 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,            [r5 + 18 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 408 * 16],     m6

; mode 27 [row 9]
pmaddubsw     m6,    m7,           [r5 + 20 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 20 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 409 * 16],     m6

; mode 27 [row 10]
pmaddubsw     m6,    m7,           [r5 + 22 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 22 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 410 * 16],     m6

; mode 27 [row 11]
pmaddubsw     m6,    m7,           [r5 + 24 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 24 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 411 * 16],     m6

; mode 27 [row 12]
pmaddubsw     m6,    m7,           [r5 + 26 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 26 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 412 * 16],     m6

; mode 27 [row 13]
pmaddubsw     m6,    m7,           [r5 + 28 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 28 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 413 * 16],     m6

; mode 27 [row 14]
pmaddubsw     m6,    m7,           [r5 + 30 * 16]
pmulhrsw      m6,    m3
pmaddubsw     m5,    m4,           [r5 + 30 * 16]
pmulhrsw      m5,    m3
packuswb      m6,                  m5
movu          [r0 + 414 * 16],     m6

; mode 28 [row 0]
movu          m1,    [r3 + 1]
psrldq        m2,    m1,           1
punpcklbw     m1,    m2
movu          m4,    [r3 + 9]
psrldq        m5,    m4,           1
punpcklbw     m4,    m5
pmaddubsw     m2,    m1,           [r5 + 5 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 5 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 416 * 16],     m2

; mode 28 [row 0]
pmaddubsw     m2,    m1,            [r5 + 5 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,            [r5 + 5 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 416 * 16],     m2

; mode 28 [row 1]
pmaddubsw     m2,    m1,           [r5 + 10 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 10 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 417 * 16],     m2

; mode 28 [row 2]
pmaddubsw     m2,    m1,            [r5 + 15 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,            [r5 + 15 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 418 * 16],     m2

; mode 28 [row 3]
pmaddubsw     m2,    m1,           [r5 + 20 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 20 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 419 * 16],     m2

; mode 28 [row 4]
pmaddubsw     m2,    m1,           [r5 + 25 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 25 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 420 * 16],     m2

; mode 28 [row 5]
pmaddubsw     m2,    m1,           [r5 + 30 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 30 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 421 * 16],     m2

; mode 29 [row 0]
pmaddubsw     m2,    m1,           [r5 + 9 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 9 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 432 * 16],     m2

; mode 29 [row 1]
pmaddubsw     m2,    m1,           [r5 + 18 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 18 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 433 * 16],     m2

; mode 29 [row 2]
pmaddubsw     m2,    m1,           [r5 + 27 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 27 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 434 * 16],     m2

; mode 30 [row 0]
pmaddubsw     m2,    m1,           [r5 + 13 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 13 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 448 * 16],     m2

; mode 30 [row 1]
pmaddubsw     m2,    m1,           [r5 + 26 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 26 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 449 * 16],     m2

; mode 33 [row 0]
movu     [r0 + 496 * 16],     m2

; mode 31 [row 0]
pmaddubsw     m2,    m1,           [r5 + 17 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 17 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 464 * 16],     m2

; mode 32 [row 0]
pmaddubsw     m2,    m1,           [r5 + 21 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 21 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                  m5
movu          [r0 + 480 * 16],     m2

; mode 28 [row 6]
movd           m7,   [r3 + 9]
palignr        m7,   m1,          2
pmaddubsw     m2,    m7,          [r5 + 3 * 16]
pmulhrsw      m2,    m3
movd          m6,     [r3 + 17]
palignr       m6,     m4,         2
pmaddubsw     m5,    m6,          [r5 + 3 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 422 * 16],   m2

; mode 28 [row 7]
pmaddubsw     m2,    m7,         [r5 + 8 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 8 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 423 * 16],   m2

; mode 28 [row 8]
pmaddubsw     m2,    m7,         [r5 + 13 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 13 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 424 * 16],   m2

; mode 28 [row 9]
pmaddubsw     m2,    m7,         [r5 + 18 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 18 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 425 * 16],   m2

; mode 28 [row 10]
pmaddubsw     m2,    m7,         [r5 + 23 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 23 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 426 * 16],   m2

; mode 29 [row 3]
pmaddubsw     m2,    m7,         [r5 + 4 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 4 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 435 * 16],   m2

; mode 29 [row 4]
pmaddubsw     m2,    m7,         [r5 + 13 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 13 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 436 * 16],   m2

; mode 29 [row 5]
pmaddubsw     m2,    m7,         [r5 + 22 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 22 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 437 * 16],   m2

; mode 29 [row 6]
pmaddubsw     m2,    m7,         [r5 + 31 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 31 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 438 * 16],   m2

; mode 32 [row 2]
movu          [r0 + 482 * 16],   m2

; mode 30 [row 2]
pmaddubsw     m2,    m7,         [r5 + 7 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 7 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 450 * 16],   m2

; mode 30 [row 3]
pmaddubsw     m2,    m7,         [r5 + 20 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 20 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 451 * 16],   m2

; mode 33 [row 1]
movu          [r0 + 497 * 16],   m2

; mode 31 [row 1]
pmaddubsw     m2,    m7,         [r5 + 2 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 2 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 465 * 16],   m2

; mode 31 [row 2]
pmaddubsw     m2,    m7,         [r5 + 19 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 19 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 466 * 16],   m2

; mode 32 [row 1]
pmaddubsw     m2,    m7,         [r5 + 10 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 10 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 481 * 16],   m2

; mode 28 [row 11]
pmaddubsw     m2,    m7,         [r5 + 28 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 28 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 427 * 16],   m2

; mode 28 [row 12]
movd          m1,     [r3 + 10]
palignr       m1,     m7,        2
pmaddubsw     m2,    m1,         [r5 + 1 * 16]
pmulhrsw      m2,    m3
movd          m4,     [r3 + 18]
palignr       m4,     m6,        2
pmaddubsw     m5,    m4,         [r5 + 1 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 428 * 16],   m2

; mode 30 [row 4]
movu          [r0 + 452 * 16],   m2

; mode 28 [row 13]
pmaddubsw     m2,    m1,         [r5 + 6 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 6 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 429 * 16],   m2

; mode 28 [row 14]
pmaddubsw     m2,    m1,         [r5 + 11 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 11 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 430 * 16],   m2

; mode 28 [row 15]
pmaddubsw     m2,    m1,           [r5 + 16 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,           [r5 + 16 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 431 * 16],   m2

; mode 29 [row 7]
pmaddubsw     m2,    m1,         [r5 + 8 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 8 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 439 * 16],   m2

; mode 29 [row 8]
pmaddubsw     m2,    m1,         [r5 + 17 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 17 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 440 * 16],   m2

; mode 29 [row 9]
pmaddubsw     m2,    m1,          [r5 + 26 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,          [r5 + 26 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 441 * 16],   m2

; mode 30 [row 5]
pmaddubsw     m2,    m1,         [r5 + 14 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 14 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 453 * 16],   m2

; mode 33 [row 2]
movu          [r0 + 498 * 16],   m2

; mode 30 [row 6]
pmaddubsw     m2,    m1,         [r5 + 27 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 27 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 454 * 16],   m2

; mode 31 [row 3]
pmaddubsw     m2,    m1,         [r5 + 4 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 4 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 467 * 16],   m2

; mode 31 [row 4]
pmaddubsw     m2,    m1,         [r5 + 21 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 21 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 468 * 16],   m2

; mode 32 [row 3]
pmaddubsw     m2,    m1,         [r5 + 20 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 20 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 483 * 16],   m2

; mode 29 [row 10]
movd          m7,     [r3 + 11]
palignr       m7,     m1,        2
pmaddubsw     m2,    m7,         [r5 + 3 * 16]
pmulhrsw      m2,    m3
movd          m6,     [r3 + 19]
palignr       m6,     m4,        2
pmaddubsw     m5,    m6,         [r5 + 3 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 442 * 16],   m2

; mode 29 [row 11]
pmaddubsw     m2,    m7,         [r5 + 12 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 12 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 443 * 16],   m2

; mode 29 [row 12]
pmaddubsw     m2,    m7,         [r5 + 21 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 21 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 444 * 16],   m2

; mode 30 [row 8]
movu          [r0 + 456 * 16],   m2

; mode 29 [row 13]
pmaddubsw     m2,    m7,         [r5 + 30 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 30 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 445 * 16],   m2

; mode 32 [row 5]
movu          [r0 + 485 * 16],   m2

; mode 30 [row 7]
pmaddubsw     m2,    m7,         [r5 + 8 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 8 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 455 * 16],   m2

; mode 33 [row 3]
movu          [r0 + 499 * 16],   m2

; mode 31 [row 5]
pmaddubsw     m2,    m7,         [r5 + 6 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 6 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 469 * 16],   m2

; mode 31 [row 6]
pmaddubsw     m2,    m7,         [r5 + 23 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 23 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 470 * 16],   m2

; mode 32 [row 4]
pmaddubsw     m2,    m7,          [r5 + 9 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,          [r5 + 9 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 484 * 16],   m2

movu          m1,        m7
movu          m4,        m6

; mode 29 [row 14]
movu          m1,    [r3 + 12]
palignr       m1,    m7,         2
pmaddubsw     m2,    m1,         [r5 + 7 * 16]
pmulhrsw      m2,    m3
movd          m4,     [r3 + 20]
palignr       m4,     m6,        2
pmaddubsw     m5,    m4,         [r5 + 7 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 446 * 16],   m2

; mode 29 [row 15]
pmaddubsw     m2,    m1,         [r5 + 16 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 16 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 447 * 16],   m2

; mode 30 [row 9]
pmaddubsw     m2,    m1,         [r5 + 2 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 2 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 457 * 16],   m2

; mode 33 [row 4]
movu          [r0 + 500 * 16],   m2

; mode 30 [row 10]
pmaddubsw     m2,    m1,         [r5 + 15 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 15 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 458 * 16],   m2

; mode 30 [row 11]
pmaddubsw     m2,    m1,          [r5 + 28 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,          [r5 + 28 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 459 * 16],   m2

; mode 33 [row 5]
movu          [r0 + 501 * 16],   m2

; mode 31 [row 7]
pmaddubsw     m2,    m1,         [r5 + 8 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 8 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 471 * 16],   m2

; mode 31 [row 8]
pmaddubsw     m2,    m1,         [r5 + 25 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 25 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 472 * 16],   m2

; mode 32 [row 6]
pmaddubsw     m2,    m1,         [r5 + 19 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 19 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 486 * 16],   m2

; mode 30 [row 12]
movd          m7,    [r3 + 13]
palignr       m7,    m1,         2
pmaddubsw     m2,    m7,         [r5 + 9 * 16]
pmulhrsw      m2,    m3
movd          m6,    [r3 + 21]
palignr       m6,    m4,         2
pmaddubsw     m5,    m6,         [r5 + 9 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 460 * 16],   m2

; mode 30 [row 13]
pmaddubsw     m2,    m7,          [r5 + 22 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,          [r5 + 22 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 461 * 16],   m2

; mode 33 [row 6]
movu          [r0 + 502 * 16],   m2

; mode 31 [row 9]
pmaddubsw     m2,    m7,          [r5 + 10 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,          [r5 + 10 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 473 * 16],   m2

; mode 31 [row 10]
pmaddubsw     m2,    m7,         [r5 + 27 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 27 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 474 * 16],   m2

; mode 32 [row 7]
pmaddubsw     m2,    m7,         [r5 + 8 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 8 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 487 * 16],   m2

; mode 32 [row 8]
pmaddubsw     m2,    m7,         [r5 + 29 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 29 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 488 * 16],   m2


movu          m1,                m7
movu          m4,                m6

; mode 30 [row 14]
movd          m1,    [r3 + 14]
palignr       m1,    m7,        2
pmaddubsw     m2,    m1,        [r5 + 3 * 16]
pmulhrsw      m2,    m3
movd          m4,    [r3 + 22]
palignr       m4,    m6,        2
pmaddubsw     m5,    m4,        [r5 + 3 * 16]
pmulhrsw      m5,    m3
packuswb      m2,               m5
movu          [r0 + 462 * 16],  m2

; mode 30 [row 15]
pmaddubsw     m2,    m1,         [r5 + 16 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 16 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 463 * 16],   m2

; mode 33 [row 7]
movu          [r0 + 503 * 16],   m2

; mode 31 [row 11]
pmaddubsw     m2,    m1,          [r5 + 12 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,          [r5 + 12 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 475 * 16],   m2

; mode 31 [row 12]
pmaddubsw     m2,    m1,         [r5 + 29 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 29 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 476 * 16],   m2

; mode 32 [row 9]
pmaddubsw     m2,    m1,         [r5 + 18 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 18 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 489 * 16],   m2

; mode 31 [row 13]
movd          m7,    [r3 + 15]
palignr       m7,    m1,         2
pmaddubsw     m2,    m7,         [r5 + 14 * 16]
pmulhrsw      m2,    m3
movd          m6,    [r3 + 23]
palignr       m6,    m4,         2
pmaddubsw     m5,    m6,         [r5 + 14 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 477 * 16],   m2

; mode 31 [row 14]
pmaddubsw     m2,    m7,         [r5 + 31 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 31 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 478 * 16],   m2

; mode 32 [row 10]
pmaddubsw     m2,    m7,         [r5 + 7 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 7 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 490 * 16],   m2

; mode 32 [row 11]
pmaddubsw     m2,    m7,         [r5 + 28 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 28 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 491 * 16],   m2

; mode 33 [row 8]
pmaddubsw     m2,    m7,         [r5 + 10 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 10 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 504 * 16],   m2

; mode 31 [row 15]
movd          m1,    [r3 + 16]
palignr       m1,    m7,         2
pmaddubsw     m2,    m1,          [r5 + 16 * 16]
pmulhrsw      m2,    m3
movd          m4,    [r3 + 24]
palignr       m4,    m6,         2
pmaddubsw     m5,    m4,          [r5 + 16 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 479 * 16],   m2

; mode 32 [row 12]
pmaddubsw     m2,    m1,          [r5 + 17 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,          [r5 + 17 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 492 * 16],   m2

; mode 33 [row 9]
pmaddubsw     m2,    m1,         [r5 + 4 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 4 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 505 * 16],   m2

; mode 33 [row 10]
pmaddubsw     m2,    m1,          [r5 + 30 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,          [r5 + 30 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 506 * 16],   m2

; mode 33 [row 10]
pmaddubsw     m2,    m1,          [r5 + 4 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,          [r5 + 4 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 505 * 16],   m2

; mode 32 [row 13]
movd          m7,    [r3 + 17]
palignr       m7,    m1,         2
pmaddubsw     m2,    m7,         [r5 + 6 * 16]
pmulhrsw      m2,    m3

movd          m6,    [r3 + 25]
palignr       m6,    m4,         2
pmaddubsw     m5,    m6,         [r5 + 6 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 493 * 16],   m2

; mode 32 [row 14]
pmaddubsw     m2,    m7,         [r5 + 27 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 27 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 494 * 16],   m2

; mode 33 [row 11]
pmaddubsw     m2,    m7,         [r5 + 24 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m6,         [r5 + 24 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 507 * 16],   m2

; mode 32 [row 15]
movd          m1,    [r3 + 18]
palignr       m1,    m7,         2
pmaddubsw     m2,    m1,         [r5 + 16 * 16]
pmulhrsw      m2,    m3
psrldq        m4,    2
pinsrb        m4,    [r3 + 26],  14
pinsrb        m4,    [r3 + 27],  15
movd          m4,    [r3 + 26]
palignr       m4,    m6,         2
pmaddubsw     m5,    m4,         [r5 + 16 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 495 * 16],   m2

; mode 33 [row 12]
pmaddubsw     m2,    m1,         [r5 + 18 * 16]
pmulhrsw      m2,    m3
pmaddubsw     m5,    m4,         [r5 + 18 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 508 * 16],   m2

; mode 33 [row 13]
movd          m7,    [r3 + 19]
palignr       m7,    m1,         2
pmaddubsw     m2,    m7,         [r5 + 12 * 16]
pmulhrsw      m2,    m3
movd          m6,    [r3 + 27]
palignr       m6,    m4,         2
pmaddubsw     m5,    m6,         [r5 + 12 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 509 * 16],   m2

; mode 33 [row 14]
movd          m1,    [r3 + 20]
palignr       m1,    m7,         2
pmaddubsw     m2,    m1,         [r5 + 6 * 16]
pmulhrsw      m2,    m3
movd          m4,    [r3 + 28]
palignr       m4,    m6,         2
pmaddubsw     m5,    m4,         [r5 + 6 * 16]
pmulhrsw      m5,    m3
packuswb      m2,                m5
movu          [r0 + 510 * 16],   m2

; mode 34 [row 0]
movu          m1,                [r3 + 2]
movu          [r0 + 512 * 16],   m1
movu          m2,                [r3 + 18]
palignr       m3,                m2,     m1,    1
movu          [r0 + 513 * 16],   m3
palignr       m3,                m2,     m1,    2
movu          [r0 + 514 * 16],   m3
palignr       m3,                m2,     m1,    3
movu          [r0 + 515 * 16],   m3
palignr       m3,                m2,     m1,    4
movu          [r0 + 516 * 16],   m3
palignr       m3,                m2,     m1,    5
movu          [r0 + 517 * 16],   m3
palignr       m3,                m2,     m1,    6
movu          [r0 + 518 * 16],   m3
palignr       m3,                m2,     m1,    7
movu          [r0 + 519 * 16],   m3
palignr       m3,                m2,     m1,    8
movu          [r0 + 520 * 16],   m3
palignr       m3,                m2,     m1,    9
movu          [r0 + 521 * 16],   m3
palignr       m3,                m2,     m1,   10
movu          [r0 + 522 * 16],   m3
palignr       m3,                m2,     m1,   11
movu          [r0 + 523 * 16],   m3
palignr       m3,                m2,     m1,   12
movu          [r0 + 524 * 16],   m3

; mode 33 [row 15]
movu          [r0 + 511 * 16],   m3

; mode 34
palignr       m3,                m2,     m1,   13
movu          [r0 + 525 * 16],   m3
palignr       m3,                m2,     m1,   14
movu          [r0 + 526 * 16],   m3
palignr       m3,                m2,     m1,   15
movu          [r0 + 527 * 16],   m3

RET
