;*****************************************************************************
;* Copyright (C) 2013 x265 project
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
;* For more information, contact us at licensing@multicorewareinc.com.
;*****************************************************************************/

%include "x86inc.asm"
%include "x86util.asm"

%if ARCH_X86_64 == 0

SECTION_RODATA 32
tab_leftmask:   db -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0

tab_Tm:     db 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8

tab_c_512:  times 8 dw 512

SECTION .text

%macro FILTER_H4 2
    movu        %1, [src + col - 1]
    pshufb      %1, Tm4
    pmaddubsw   %1, coef2
    movu        %2, [src + col + 1]
    pshufb      %2, Tm4
    pmaddubsw   %2, coef3
    paddw       %1, %2
    pmulhrsw    %1, c512
    packuswb    %1, %1
%endmacro

;-----------------------------------------------------------------------------
; void filterHorizontal_p_p_4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height, short const *coeff)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal filterHorizontal_p_p_4, 0, 7, 8
%define src         r0
%define dst         r1
%define row         r2
%define col         r3
%define width       r4
%define widthleft   r5
%define mask_offset r6
%define coef2       m7
%define coef3       m6
%define Tm4         m5
%define c512        m4
%define x2          m3
%define x1          m2
%define x0          m1
%define leftmask    m0
%define tmp         r0
%define tmp1        r1
 
    mov         tmp,        r6m
    movd        coef2,      [tmp    ]
    movd        coef3,      [tmp + 4]
    pshufd      coef2,      coef2,  0
    pshufd      coef3,      coef3,  0
    packsswb    coef2,      coef2
    packsswb    coef3,      coef3

    mov         width,      r4m
    mov         widthleft,  width
    and         width,      ~7
    and         widthleft,  7
    mov         mask_offset,  widthleft
    neg         mask_offset

    movq        leftmask,   [tab_leftmask + (7 + mask_offset)]
    mova        Tm4,        [tab_Tm]
    mova        c512,       [tab_c_512]

    mov         src,        r0m
    mov         dst,        r2m
    mov         row,        r5m

_loop_row:
    xor         col,        col
 
_loop_col:
    cmp         col,        width
    jge         _end_col

    FILTER_H4   x0, x1
    movh        [dst + col], x0

    add         col,         8
    jmp         _loop_col

_end_col:
    test        widthleft,  widthleft
    jz          _next_row

    movq        x2, [dst + col]
    FILTER_H4   x0, x1
    pblendvb    x2, x2, x0, leftmask
    movh        [dst + col], x2

_next_row:
    add         src,        r1m
    add         dst,        r3m
    dec         row

    test        row,        row
    jz          _end_row

    jmp         _loop_row

_end_row:

    RET

%endif  ; ARCH_X86_64 == 0
