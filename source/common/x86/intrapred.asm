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
