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
