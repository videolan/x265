;*****************************************************************************
;* Copyright (C) 2013 x265 project
;*
;* Authors: Nabajit Deka <nabajit@multicorewareinc.com>
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

tab_dct4:       times 4 dw 64, 64
                times 4 dw 83, 36
                times 4 dw 64, -64
                times 4 dw 36, -83

SECTION .text

cextern pd_1
cextern pd_128

;------------------------------------------------------
;void dct4(int16_t *src, int32_t *dst, intptr_t stride)
;------------------------------------------------------
INIT_XMM sse2
cglobal dct4, 3, 4, 8

    add         r2d, r2d
    lea         r3, [tab_dct4]

    mova        m4, [r3 + 0 * 16]
    mova        m5, [r3 + 1 * 16]
    mova        m6, [r3 + 2 * 16]

    mova        m7, [pd_1]

    movh        m0, [r0 + 0 * r2]
    movh        m1, [r0 + 1 * r2]
    punpcklqdq  m0, m1
    pshufd      m0, m0, 0xD8
    pshufhw     m0, m0, 0xB1

    lea         r0, [r0 + 2 * r2]
    movh        m1, [r0]
    movh        m2, [r0 + r2]
    punpcklqdq  m1, m2
    pshufd      m1, m1, 0xD8
    pshufhw     m1, m1, 0xB1

    punpcklqdq  m2, m0, m1
    punpckhqdq  m0, m1

    paddw       m1, m2, m0
    psubw       m2, m0

    pmaddwd     m0, m1, m4
    paddd       m0, m7
    psrad       m0, 1

    pmaddwd     m3, m2, m5
    paddd       m3, m7
    psrad       m3, 1

    packssdw    m0, m3
    pshufd      m0, m0, 0xD8
    pshufhw     m0, m0, 0xB1

    pmaddwd     m1, m6
    paddd       m1, m7
    psrad       m1, 1

    pmaddwd     m2, [r3 + 3 * 16]
    paddd       m2, m7
    psrad       m2, 1

    packssdw    m1, m2
    pshufd      m1, m1, 0xD8
    pshufhw     m1, m1, 0xB1

    punpcklqdq  m2, m0, m1
    punpckhqdq  m0, m1

    mova        m7, [pd_128]

    pmaddwd     m1, m2, m4
    pmaddwd     m3, m0, m4
    paddd       m1, m3
    paddd       m1, m7
    psrad       m1, 8
    movu        [r1 + 0 * 16], m1

    pmaddwd     m1, m2, m5
    pmaddwd     m3, m0, m5
    psubd       m1, m3
    paddd       m1, m7
    psrad       m1, 8
    movu        [r1 + 1 * 16], m1

    pmaddwd     m1, m2, m6
    pmaddwd     m3, m0, m6
    paddd       m1, m3
    paddd       m1, m7
    psrad       m1, 8
    movu        [r1 + 2 * 16], m1

    pmaddwd     m2, [r3 + 3 * 16]
    pmaddwd     m0, [r3 + 3 * 16]
    psubd       m2, m0
    paddd       m2, m7
    psrad       m2, 8
    movu        [r1 + 3 * 16], m2

    RET
