;*****************************************************************************
;* Copyright (C) 2013-2017 MulticoreWare, Inc
;*
;* Authors: Min Chen <chenm003@163.com>
;*          Nabajit Deka <nabajit@multicorewareinc.com>
;*          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
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

const h_tabw_LumaCoeff,  dw   0, 0,  0,  64,  0,   0,  0,  0
                       dw  -1, 4, -10, 58,  17, -5,  1,  0
                       dw  -1, 4, -11, 40,  40, -11, 4, -1
                       dw   0, 1, -5,  17,  58, -10, 4, -1

SECTION .text

cextern pw_32
cextern pw_2000

%macro FILTER_H8_W8_sse2 0
    movh        m1, [r0 + x - 3]
    movh        m4, [r0 + x - 2]
    punpcklbw   m1, m6
    punpcklbw   m4, m6
    movh        m5, [r0 + x - 1]
    movh        m0, [r0 + x]
    punpcklbw   m5, m6
    punpcklbw   m0, m6
    pmaddwd     m1, m3
    pmaddwd     m4, m3
    pmaddwd     m5, m3
    pmaddwd     m0, m3
    packssdw    m1, m4
    packssdw    m5, m0
    pshuflw     m4, m1, q2301
    pshufhw     m4, m4, q2301
    pshuflw     m0, m5, q2301
    pshufhw     m0, m0, q2301
    paddw       m1, m4
    paddw       m5, m0
    psrldq      m1, 2
    psrldq      m5, 2
    pshufd      m1, m1, q3120
    pshufd      m5, m5, q3120
    punpcklqdq  m1, m5
    movh        m7, [r0 + x + 1]
    movh        m4, [r0 + x + 2]
    punpcklbw   m7, m6
    punpcklbw   m4, m6
    movh        m5, [r0 + x + 3]
    movh        m0, [r0 + x + 4]
    punpcklbw   m5, m6
    punpcklbw   m0, m6
    pmaddwd     m7, m3
    pmaddwd     m4, m3
    pmaddwd     m5, m3
    pmaddwd     m0, m3
    packssdw    m7, m4
    packssdw    m5, m0
    pshuflw     m4, m7, q2301
    pshufhw     m4, m4, q2301
    pshuflw     m0, m5, q2301
    pshufhw     m0, m0, q2301
    paddw       m7, m4
    paddw       m5, m0
    psrldq      m7, 2
    psrldq      m5, 2
    pshufd      m7, m7, q3120
    pshufd      m5, m5, q3120
    punpcklqdq  m7, m5
    pshuflw     m4, m1, q2301
    pshufhw     m4, m4, q2301
    pshuflw     m0, m7, q2301
    pshufhw     m0, m0, q2301
    paddw       m1, m4
    paddw       m7, m0
    psrldq      m1, 2
    psrldq      m7, 2
    pshufd      m1, m1, q3120
    pshufd      m7, m7, q3120
    punpcklqdq  m1, m7
%endmacro

%macro FILTER_H8_W4_sse2 0
    movh        m1, [r0 + x - 3]
    movh        m0, [r0 + x - 2]
    punpcklbw   m1, m6
    punpcklbw   m0, m6
    movh        m4, [r0 + x - 1]
    movh        m5, [r0 + x]
    punpcklbw   m4, m6
    punpcklbw   m5, m6
    pmaddwd     m1, m3
    pmaddwd     m0, m3
    pmaddwd     m4, m3
    pmaddwd     m5, m3
    packssdw    m1, m0
    packssdw    m4, m5
    pshuflw     m0, m1, q2301
    pshufhw     m0, m0, q2301
    pshuflw     m5, m4, q2301
    pshufhw     m5, m5, q2301
    paddw       m1, m0
    paddw       m4, m5
    psrldq      m1, 2
    psrldq      m4, 2
    pshufd      m1, m1, q3120
    pshufd      m4, m4, q3120
    punpcklqdq  m1, m4
    pshuflw     m0, m1, q2301
    pshufhw     m0, m0, q2301
    paddw       m1, m0
    psrldq      m1, 2
    pshufd      m1, m1, q3120
%endmacro

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
%macro IPFILTER_LUMA_sse2 3
INIT_XMM sse2
cglobal interp_8tap_horiz_%3_%1x%2, 4,6,8
    mov       r4d, r4m
    add       r4d, r4d
    pxor      m6, m6

%ifidn %3, ps
    add       r3d, r3d
    cmp       r5m, byte 0
%endif

%ifdef PIC
    lea       r5, [h_tabw_LumaCoeff]
    movu      m3, [r5 + r4 * 8]
%else
    movu      m3, [h_tabw_LumaCoeff + r4 * 8]
%endif

    mov       r4d, %2

%ifidn %3, pp
    mova      m2, [pw_32]
%else
    mova      m2, [pw_2000]
    je        .loopH
    lea       r5, [r1 + 2 * r1]
    sub       r0, r5
    add       r4d, 7
%endif

.loopH:
%assign x 0
%rep %1 / 8
    FILTER_H8_W8_sse2
  %ifidn %3, pp
    paddw     m1, m2
    psraw     m1, 6
    packuswb  m1, m1
    movh      [r2 + x], m1
  %else
    psubw     m1, m2
    movu      [r2 + 2 * x], m1
  %endif
%assign x x+8
%endrep

%rep (%1 % 8) / 4
    FILTER_H8_W4_sse2
  %ifidn %3, pp
    paddw     m1, m2
    psraw     m1, 6
    packuswb  m1, m1
    movd      [r2 + x], m1
  %else
    psubw     m1, m2
    movh      [r2 + 2 * x], m1
  %endif
%endrep

    add       r0, r1
    add       r2, r3

    dec       r4d
    jnz       .loopH
    RET

%endmacro

;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
    IPFILTER_LUMA_sse2 4, 4, pp
    IPFILTER_LUMA_sse2 4, 8, pp
    IPFILTER_LUMA_sse2 8, 4, pp
    IPFILTER_LUMA_sse2 8, 8, pp
    IPFILTER_LUMA_sse2 16, 16, pp
    IPFILTER_LUMA_sse2 16, 8, pp
    IPFILTER_LUMA_sse2 8, 16, pp
    IPFILTER_LUMA_sse2 16, 12, pp
    IPFILTER_LUMA_sse2 12, 16, pp
    IPFILTER_LUMA_sse2 16, 4, pp
    IPFILTER_LUMA_sse2 4, 16, pp
    IPFILTER_LUMA_sse2 32, 32, pp
    IPFILTER_LUMA_sse2 32, 16, pp
    IPFILTER_LUMA_sse2 16, 32, pp
    IPFILTER_LUMA_sse2 32, 24, pp
    IPFILTER_LUMA_sse2 24, 32, pp
    IPFILTER_LUMA_sse2 32, 8, pp
    IPFILTER_LUMA_sse2 8, 32, pp
    IPFILTER_LUMA_sse2 64, 64, pp
    IPFILTER_LUMA_sse2 64, 32, pp
    IPFILTER_LUMA_sse2 32, 64, pp
    IPFILTER_LUMA_sse2 64, 48, pp
    IPFILTER_LUMA_sse2 48, 64, pp
    IPFILTER_LUMA_sse2 64, 16, pp
    IPFILTER_LUMA_sse2 16, 64, pp

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_ps_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
    IPFILTER_LUMA_sse2 4, 4, ps
    IPFILTER_LUMA_sse2 8, 8, ps
    IPFILTER_LUMA_sse2 8, 4, ps
    IPFILTER_LUMA_sse2 4, 8, ps
    IPFILTER_LUMA_sse2 16, 16, ps
    IPFILTER_LUMA_sse2 16, 8, ps
    IPFILTER_LUMA_sse2 8, 16, ps
    IPFILTER_LUMA_sse2 16, 12, ps
    IPFILTER_LUMA_sse2 12, 16, ps
    IPFILTER_LUMA_sse2 16, 4, ps
    IPFILTER_LUMA_sse2 4, 16, ps
    IPFILTER_LUMA_sse2 32, 32, ps
    IPFILTER_LUMA_sse2 32, 16, ps
    IPFILTER_LUMA_sse2 16, 32, ps
    IPFILTER_LUMA_sse2 32, 24, ps
    IPFILTER_LUMA_sse2 24, 32, ps
    IPFILTER_LUMA_sse2 32, 8, ps
    IPFILTER_LUMA_sse2 8, 32, ps
    IPFILTER_LUMA_sse2 64, 64, ps
    IPFILTER_LUMA_sse2 64, 32, ps
    IPFILTER_LUMA_sse2 32, 64, ps
    IPFILTER_LUMA_sse2 64, 48, ps
    IPFILTER_LUMA_sse2 48, 64, ps
    IPFILTER_LUMA_sse2 64, 16, ps
    IPFILTER_LUMA_sse2 16, 64, ps

