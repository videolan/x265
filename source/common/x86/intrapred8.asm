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

c_trans_4x4 db 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15
pb_0_8          times 8 db 0, 8
pb_unpackbw1    times 2 db 1, 8, 2, 8, 3, 8, 4, 8

tab_Si:  db 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7

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
; void intraPredAng(pixel* dst, intptr_t dstStride, pixel *refLeft, pixel *refAbove, int dirMode, int bFilter)
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
