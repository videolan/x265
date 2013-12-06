;*****************************************************************************
;* Copyright (C) 2013 x265 project
;*
;* Authors: Dnyaneshwar Gorade <dnyaneshwar@multicorewareinc.com>
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

cextern pw_4096


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

