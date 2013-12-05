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
; void intra_pred_dc(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int filter)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_dc4, 5,6,3
    pxor        m0, m0
    movd        m1, [r0]
    movd        m2, [r1]
    punpckldq   m1, m2
    psadbw      m1, m0              ; m1 = sum

    test        r4d, r4d

    mov         r4d, 4096
    movd        m2, r4d
    pmulhrsw    m1, m2              ; m1 = (sum + 4) / 8
    movd        r4d, m1             ; r4d = dc_val
    pshufb      m1, m0              ; m1 = byte [dc_val ...]

    ; store DC 4x4
    lea         r5, [r3 * 3]
    movd        [r2], m1
    movd        [r2 + r3], m1
    movd        [r2 + r3 * 2], m1
    movd        [r2 + r5], m1

    ; do DC filter
    jz         .end
    lea         r5d, [r4d * 2 + 2]  ; r5d = DC * 2 + 2
    add         r4d, r5d            ; r4d = DC * 3 + 2
    movd        m1, r4d
    pshuflw     m1, m1, 0           ; m1 = pixDCx3

    ; filter top
    pmovzxbw    m2, [r0]
    paddw       m2, m1
    psraw       m2, 2
    packuswb    m2, m2
    movd        [r2], m2            ; overwrite top-left pixel, we will update it later

    ; filter top-left
    movzx       r0d, byte [r0]
    add         r5d, r0d
    movzx       r0d, byte [r1]
    add         r0d, r5d
    shr         r0d, 2
    mov         [r2], r0b

    ; filter left
    add         r2, r3
    pmovzxbw    m2, [r1 + 1]
    paddw       m2, m1
    psraw       m2, 2
    packuswb    m2, m2
    movd        r0d, m2
    mov         [r2], r0b
    shr         r0d, 8
    mov         [r2 + r3], r0b
    shr         r0d, 8
    mov         [r2 + r3 * 2], r0b

.end:

    RET


;-------------------------------------------------------------------------------------------
; void intra_pred_dc(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int filter)
;-------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_dc8, 5, 7, 3, above, left, dst, dstStride, filter

    pxor            m0,            m0
    movh            m1,            [r0]
    movh            m2,            [r1]
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
    mov             r6,            r2
    movh            [r2],          m1
    movh            [r2 + r3],     m1
    lea             r2,            [r2 + 2 * r3]
    movh            [r2],          m1
    movh            [r2 + r3],     m1
    lea             r2,            [r2 + 2 * r3]
    movh            [r2],          m1
    movh            [r2 + r3],     m1
    lea             r2,            [r2 + 2 * r3]
    movh            [r2],          m1
    movh            [r2 + r3],     m1

    ; Do DC Filter
    jz              .end
    lea             r4d,           [r5d * 2 + 2]  ; r4d = DC * 2 + 2
    add             r5d,           r4d            ; r5d = DC * 3 + 2
    movd            m1,            r5d
    pshuflw         m1,            m1, 0          ; m1 = pixDCx3
    pshufd          m1,            m1, 0

    ; filter top
    pmovzxbw        m2,            [r0]
    paddw           m2,            m1
    psraw           m2,            2
    packuswb        m2,            m2
    movh            [r6],          m2

    ; filter top-left
    movzx           r0d, byte      [r0]
    add             r4d,           r0d
    movzx           r0d, byte      [r1]
    add             r0d,           r4d
    shr             r0d,           2
    mov             [r6],          r0b

    ; filter left
    add             r6,            r3
    pmovzxbw        m2,            [r1 + 1]
    paddw           m2,            m1
    psraw           m2,            2
    packuswb        m2,            m2
    pextrb          [r6],          m2, 0
    pextrb          [r6 + r3],     m2, 1
    pextrb          [r6 + 2 * r3], m2, 2
    lea             r6,            [r6 + r3 * 2]
    pextrb          [r6 + r3],     m2, 3
    pextrb          [r6 + 2 * r3], m2, 4
    pextrb          [r6 + 4 * r3], m2, 6
    lea             r3,            [r3 * 3]
    pextrb          [r6 + r3],     m2, 5

.end
    RET

;-------------------------------------------------------------------------------------------
; void intra_pred_dc(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int filter)
;-------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_dc16, 5, 7, 4, above, left, dst, dstStride, filter

    pxor            m0,            m0
    movu            m1,            [r0]
    movu            m2,            [r1]
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
    mov             r6,            r2
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1

    ; Do DC Filter
    jz              .end
    lea             r4d,           [r5d * 2 + 2]  ; r4d = DC * 2 + 2
    add             r5d,           r4d            ; r5d = DC * 3 + 2
    movd            m1,            r5d
    pshuflw         m1,            m1, 0          ; m1 = pixDCx3
    pshufd          m1,            m1, 0

    ; filter top
    pmovzxbw        m2,            [r0]
    paddw           m2,            m1
    psraw           m2,            2
    packuswb        m2,            m2
    movh            [r6],          m2
    pmovzxbw        m3,            [r0 + 8]
    paddw           m3,            m1
    psraw           m3,            2
    packuswb        m3,            m3
    movh            [r6 + 8],      m3

    ; filter top-left
    movzx           r0d, byte      [r0]
    add             r4d,           r0d
    movzx           r0d, byte      [r1]
    add             r0d,           r4d
    shr             r0d,           2
    mov             [r6],          r0b

    ; filter left
    add             r6,            r3
    pmovzxbw        m2,            [r1 + 1]
    paddw           m2,            m1
    psraw           m2,            2
    packuswb        m2,            m2
    pextrb          [r6],          m2, 0
    pextrb          [r6 + r3],     m2, 1
    pextrb          [r6 + r3 * 2], m2, 2
    lea             r6,            [r6 + r3 * 2]
    add             r6,            r3
    pextrb          [r6],          m2, 3
    add             r6,            r3
    pextrb          [r6],          m2, 4
    pextrb          [r6 + r3],     m2, 5
    pextrb          [r6 + r3 * 2], m2, 6
    lea             r6,            [r6 + r3 * 2]
    add             r6,            r3
    pextrb          [r6],          m2, 7

    add             r6,            r3
    pmovzxbw        m3,            [r1 + 9]
    paddw           m3,            m1
    psraw           m3,            2
    packuswb        m3,            m3
    pextrb          [r6],          m3, 0
    pextrb          [r6 + r3],     m3, 1
    pextrb          [r6 + r3 * 2], m3, 2
    lea             r6,            [r6 + r3 * 2]
    add             r6,            r3
    pextrb          [r6],          m3, 3
    add             r6,            r3
    pextrb          [r6],          m3, 4
    pextrb          [r6 + r3],     m3, 5
    pextrb          [r6 + r3 * 2], m3, 6

.end
    RET

;-------------------------------------------------------------------------------------------
; void intra_pred_dc(pixel* above, pixel* left, pixel* dst, intptr_t dstStride, int filter)
;-------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_dc32, 4, 5, 5, above, left, dst, dstStride, filter

    pxor            m0,            m0
    movu            m1,            [r0]
    movu            m2,            [r0 + 16]
    movu            m3,            [r1]
    movu            m4,            [r1 + 16]
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
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    movu            [r2 + 16],     m1
    movu            [r2 + r3 + 16],m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    movu            [r2 + 16],     m1
    movu            [r2 + r3 + 16],m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    movu            [r2 + 16],     m1
    movu            [r2 + r3 + 16],m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    movu            [r2 + 16],     m1
    movu            [r2 + r3 + 16],m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    movu            [r2 + 16],     m1
    movu            [r2 + r3 + 16],m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    movu            [r2 + 16],     m1
    movu            [r2 + r3 + 16],m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    movu            [r2 + 16],     m1
    movu            [r2 + r3 + 16],m1
    lea             r2,            [r2 + 2 * r3]
    movu            [r2],          m1
    movu            [r2 + r3],     m1
    movu            [r2 + 16],     m1
    movu            [r2 + r3 + 16],m1
    lea             r2,            [r2 + 2 * r3]
%endrep

    RET

;----------------------------------------------------------------------------------------
; void intra_pred_planar4_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
;----------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_planar4, 4,7,5, above, left, dst, dstStride

    pmovzxbw        m0,         [r0]      ; topRow[i] = above[i];
    punpcklqdq      m0,         m0

    pxor            m1,         m1
    movd            m2,         [r1 + 4]  ; bottomLeft = left[4]
    movzx           r6d, byte   [r0 + 4]  ; topRight   = above[4];
    pshufb          m2,         m1
    punpcklbw       m2,         m1
    psubw           m2,         m0        ; bottomRow[i] = bottomLeft - topRow[i]
    psllw           m0,         2
    punpcklqdq      m3,         m2, m1
    psubw           m0,         m3
    paddw           m2,         m2

%macro COMP_PRED_PLANAR_2ROW 1
    movzx           r4d, byte   [r1 + %1]
    lea             r4d,        [r4d * 4 + 4]
    movd            m3,         r4d
    pshuflw         m3,         m3, 0

    movzx           r4d, byte   [r1 + %1 + 1]
    lea             r4d,        [r4d * 4 + 4]
    movd            m4,         r4d
    pshuflw         m4,         m4, 0
    punpcklqdq      m3,         m4        ; horPred

    movzx           r4d, byte   [r1 + %1]
    mov             r5d,        r6d
    sub             r5d,        r4d
    movd            m4,         r5d
    pshuflw         m4,         m4, 0

    movzx           r4d, byte   [r1 + %1 + 1]
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

    movd            [r2],       m3
    pshufd          m3,         m3, 0x55
    movd            [r2 + r3],  m3
    lea             r2,         [r2 + 2 * r3]
%endmacro

    COMP_PRED_PLANAR_2ROW 0
    COMP_PRED_PLANAR_2ROW 2

    RET

;----------------------------------------------------------------------------------------
; void intra_pred_planar8_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
;----------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_planar8, 4,4,7, above, left, dst, dstStride

    pxor            m0,     m0
    pmovzxbw        m1,     [r0]     ; v_topRow
    pmovzxbw        m2,     [r1]     ; v_leftColumn

    movd            m3,     [r0 + 8] ; topRight   = above[8];
    movd            m4,     [r1 + 8] ; bottomLeft = left[8];

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

    movh            [r2],   m5
    lea             r2,     [r2 + r3]

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


;----------------------------------------------------------------------------------------
; void intra_pred_planar16_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
;----------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal intra_pred_planar16, 4,6,8, above, left, dst, dstStride

    pxor            m0,         m0
    pmovzxbw        m1,         [r0]       ; topRow[0-7]
    pmovzxbw        m2,         [r0 + 8]   ; topRow[8-15]

    movd            m3,         [r1 + 16]
    pshufb          m3,         m0
    punpcklbw       m3,         m0         ; v_bottomLeft = left[16]
    movzx           r4d, byte   [r0 + 16]  ; topRight     = above[16]

    psubw           m4,         m3, m1     ; v_bottomRow[0]
    psubw           m5,         m3, m2     ; v_bottomRow[1]

    psllw           m1,         4
    psllw           m2,         4

%macro PRED_PLANAR_ROW16 1
    movzx           r5d, byte   [r1 + %1]
    add             r5d,        r5d
    lea             r5d,        [r5d * 8 + 16]
    movd            m3,         r5d
    pshuflw         m3,         m3, 0
    pshufd          m3,         m3, 0      ; horPred

    movzx           r5d, byte   [r1 + %1]
    mov             r0d,        r4d
    sub             r0d,        r5d
    movd            m6,         r0d
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
    movu            [r2],       m7
    lea             r2,         [r2 + r3]
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


;----------------------------------------------------------------------------------------
; void intra_pred_planar32_sse4(pixel* above, pixel* left, pixel* dst, intptr_t dstStride)
;----------------------------------------------------------------------------------------
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

    pxor            m3,         m3
    movd            m0,         [r1 + 32]
    pshufb          m0,         m3
    punpcklbw       m0,         m3          ; v_bottomLeft = left[32]
    movzx           r4d, byte   [r0 + 32]   ; topRight     = above[32]

    pmovzxbw        m1,         [r0 + 0]    ; topRow[0]
    pmovzxbw        m2,         [r0 + 8]    ; topRow[1]
    pmovzxbw        m3,         [r0 +16]    ; topRow[2]
    pmovzxbw        m4,         [r0 +24]    ; topRow[3]

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
    movzx           r5d,   byte [r1]
    shl             r5d,        5
    add             r5d,        32
    movd            m5,         r5d
    pshuflw         m5,         m5, 0
    pshufd          m5,         m5, 0      ; horPred

    movzx           r5d,   byte [r1]
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
    movu            [r2 + %1],  m7
%endmacro

    mov r0,         32
.loop
    COMP_PRED_PLANAR_ROW 0
    COMP_PRED_PLANAR_ROW 16
    inc             r1
    lea             r2,         [r2 + r3]

    dec             r0
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
    pextrd      [r0], m0, 0
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
