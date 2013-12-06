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

cextern pw_1
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

