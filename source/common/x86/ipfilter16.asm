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

tab_LumaCoeff:   dw   0, 0,  0,  64,  0,   0,  0,  0
                 dw  -1, 4, -10, 58,  17, -5,  1,  0
                 dw  -1, 4, -11, 40,  40, -11, 4, -1
                 dw   0, 1, -5,  17,  58, -10, 4, -1

SECTION .text

cextern pd_32
cextern pw_pixel_max
cextern pd_n32768

;------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_4x4(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;------------------------------------------------------------------------------------------------------------
%macro FILTER_HOR_LUMA_W4 3
INIT_XMM sse4
cglobal interp_8tap_horiz_%3_%1x%2, 4, 7, 8

    mov         r4d, r4m
    sub         r0, 6
    shl         r4d, 4
    add         r1, r1
    add         r3, r3

%ifdef PIC
    lea         r6, [tab_LumaCoeff]
    mova        m0, [r6 + r4]
%else
    mova        m0, [tab_LumaCoeff + r4]
%endif

%ifidn %3, pp 
    mova        m1, [pd_32]
    pxor        m6, m6
    mova        m7, [pw_pixel_max]
%else
    mova        m1, [pd_n32768]
%endif

    mov         r4d, %2
%ifidn %3, ps
    cmp         r5m, byte 0
    je          .loopH
    lea         r6, [r1 + 2 * r1]
    sub         r0, r6
    add         r4d, 7
%endif

.loopH
    movu        m2, [r0]                     ; m2 = src[0-7]
    movu        m3, [r0 + 16]                ; m3 = src[8-15]

    pmaddwd     m4, m2, m0
    palignr     m5, m3, m2, 2                ; m5 = src[1-8]
    pmaddwd     m5, m0
    phaddd      m4, m5

    palignr     m5, m3, m2, 4                ; m5 = src[2-9]
    pmaddwd     m5, m0
    palignr     m3, m2, 6                    ; m3 = src[3-10]
    pmaddwd     m3, m0
    phaddd      m5, m3

    phaddd      m4, m5
    paddd       m4, m1
%ifidn %3, pp
    psrad       m4, 6
    packusdw    m4, m4
    CLIPW       m4, m6, m7
%else
    psrad       m4, 2
    packssdw    m4, m4
%endif

    movh        [r2], m4

    add         r0, r1
    add         r2, r3

    dec         r4d
    jnz         .loopH
    RET
%endmacro

;------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_4x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W4 4, 4, pp
FILTER_HOR_LUMA_W4 4, 8, pp
FILTER_HOR_LUMA_W4 4, 16, pp

;---------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_ps_4x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;---------------------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W4 4, 4, ps
FILTER_HOR_LUMA_W4 4, 8, ps
FILTER_HOR_LUMA_W4 4, 16, ps

;------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;------------------------------------------------------------------------------------------------------------
%macro FILTER_HOR_LUMA_W8 3
INIT_XMM sse4
cglobal interp_8tap_horiz_%3_%1x%2, 4, 7, 8

    add         r1, r1
    add         r3, r3
    mov         r4d, r4m
    sub         r0, 6
    shl         r4d, 4

%ifdef PIC
    lea         r6, [tab_LumaCoeff]
    mova        m0, [r6 + r4]
%else
    mova        m0, [tab_LumaCoeff + r4]
%endif

%ifidn %3, pp 
    mova        m1, [pd_32]
    pxor        m7, m7
%else
    mova        m1, [pd_n32768]
%endif

    mov         r4d, %2
%ifidn %3, ps
    cmp         r5m, byte 0
    je          .loopH
    lea         r6, [r1 + 2 * r1]
    sub         r0, r6
    add         r4d, 7
%endif

.loopH
    movu        m2, [r0]                     ; m2 = src[0-7]
    movu        m3, [r0 + 16]                ; m3 = src[8-15]

    pmaddwd     m4, m2, m0
    palignr     m5, m3, m2, 2                ; m5 = src[1-8]
    pmaddwd     m5, m0
    phaddd      m4, m5

    palignr     m5, m3, m2, 4                ; m5 = src[2-9]
    pmaddwd     m5, m0
    palignr     m6, m3, m2, 6                ; m6 = src[3-10]
    pmaddwd     m6, m0
    phaddd      m5, m6
    phaddd      m4, m5
    paddd       m4, m1

    palignr     m5, m3, m2, 8                ; m5 = src[4-11]
    pmaddwd     m5, m0
    palignr     m6, m3, m2, 10               ; m6 = src[5-12]
    pmaddwd     m6, m0
    phaddd      m5, m6

    palignr     m6, m3, m2, 12               ; m6 = src[6-13]
    pmaddwd     m6, m0
    palignr     m3, m2, 14                   ; m3 = src[7-14]
    pmaddwd     m3, m0
    phaddd      m6, m3
    phaddd      m5, m6
    paddd       m5, m1
%ifidn %3, pp 
    psrad       m4, 6
    psrad       m5, 6
    packusdw    m4, m5
    CLIPW       m4, m7, [pw_pixel_max]
%else
    psrad       m4, 2
    psrad       m5, 2
    packssdw    m4, m5
%endif

    movu        [r2], m4

    add         r0, r1
    add         r2, r3

    dec         r4d
    jnz         .loopH
    RET
%endmacro

;------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_8x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W8 8, 4, pp
FILTER_HOR_LUMA_W8 8, 8, pp
FILTER_HOR_LUMA_W8 8, 16, pp
FILTER_HOR_LUMA_W8 8, 32, pp

;---------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_ps_8x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;---------------------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W8 8, 4, ps
FILTER_HOR_LUMA_W8 8, 8, ps
FILTER_HOR_LUMA_W8 8, 16, ps
FILTER_HOR_LUMA_W8 8, 32, ps

;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_HOR_LUMA_W12 3
INIT_XMM sse4
cglobal interp_8tap_horiz_%3_%1x%2, 4, 7, 8

    add         r1, r1
    add         r3, r3
    mov         r4d, r4m
    sub         r0, 6
    shl         r4d, 4

%ifdef PIC
    lea         r6, [tab_LumaCoeff]
    mova        m0, [r6 + r4]
%else
    mova        m0, [tab_LumaCoeff + r4]
%endif
%ifidn %3, pp 
    mova        m1, [pd_32]
%else
    mova        m1, [pd_n32768]
%endif

    mov         r4d, %2
%ifidn %3, ps
    cmp         r5m, byte 0
    je          .loopH
    lea         r6, [r1 + 2 * r1]
    sub         r0, r6
    add         r4d, 7
%endif

.loopH
    movu        m2, [r0]                     ; m2 = src[0-7]
    movu        m3, [r0 + 16]                ; m3 = src[8-15]

    pmaddwd     m4, m2, m0
    palignr     m5, m3, m2, 2                ; m5 = src[1-8]
    pmaddwd     m5, m0
    phaddd      m4, m5

    palignr     m5, m3, m2, 4                ; m5 = src[2-9]
    pmaddwd     m5, m0
    palignr     m6, m3, m2, 6                ; m6 = src[3-10]
    pmaddwd     m6, m0
    phaddd      m5, m6
    phaddd      m4, m5
    paddd       m4, m1

    palignr     m5, m3, m2, 8                ; m5 = src[4-11]
    pmaddwd     m5, m0
    palignr     m6, m3, m2, 10               ; m6 = src[5-12]
    pmaddwd     m6, m0
    phaddd      m5, m6

    palignr     m6, m3, m2, 12               ; m6 = src[6-13]
    pmaddwd     m6, m0
    palignr     m7, m3, m2, 14               ; m2 = src[7-14]
    pmaddwd     m7, m0
    phaddd      m6, m7
    phaddd      m5, m6
    paddd       m5, m1
%ifidn %3, pp 
    psrad       m4, 6
    psrad       m5, 6
    packusdw    m4, m5
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, 2
    psrad       m5, 2
    packssdw    m4, m5
%endif

    movu        [r2], m4

    movu        m2, [r0 + 32]                ; m2 = src[16-23]

    pmaddwd     m4, m3, m0                   ; m3 = src[8-15]
    palignr     m5, m2, m3, 2                ; m5 = src[9-16]
    pmaddwd     m5, m0
    phaddd      m4, m5

    palignr     m5, m2, m3, 4                ; m5 = src[10-17]
    pmaddwd     m5, m0
    palignr     m2, m3, 6                    ; m2 = src[11-18]
    pmaddwd     m2, m0
    phaddd      m5, m2
    phaddd      m4, m5
    paddd       m4, m1
%ifidn %3, pp 
    psrad       m4, 6
    packusdw    m4, m4
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, 2
    packssdw    m4, m4
%endif

    movh        [r2 + 16], m4

    add         r0, r1
    add         r2, r3

    dec         r4d
    jnz         .loopH
    RET
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_12x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W12 12, 16, pp

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_ps_12x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W12 12, 16, ps

;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_HOR_LUMA_W16 3
INIT_XMM sse4
cglobal interp_8tap_horiz_%3_%1x%2, 4, 7, 8

    add         r1, r1
    add         r3, r3
    mov         r4d, r4m
    sub         r0, 6
    shl         r4d, 4

%ifdef PIC
    lea         r6, [tab_LumaCoeff]
    mova        m0, [r6 + r4]
%else
    mova        m0, [tab_LumaCoeff + r4]
%endif

%ifidn %3, pp 
    mova        m1, [pd_32]
%else
    mova        m1, [pd_n32768]
%endif

    mov         r4d, %2
%ifidn %3, ps
    cmp         r5m, byte 0
    je          .loopH
    lea         r6, [r1 + 2 * r1]
    sub         r0, r6
    add         r4d, 7
%endif

.loopH
%assign x 0
%rep %1 / 16
    movu        m2, [r0 + x]                 ; m2 = src[0-7]
    movu        m3, [r0 + 16 + x]            ; m3 = src[8-15]

    pmaddwd     m4, m2, m0
    palignr     m5, m3, m2, 2                ; m5 = src[1-8]
    pmaddwd     m5, m0
    phaddd      m4, m5

    palignr     m5, m3, m2, 4                ; m5 = src[2-9]
    pmaddwd     m5, m0
    palignr     m6, m3, m2, 6                ; m6 = src[3-10]
    pmaddwd     m6, m0
    phaddd      m5, m6
    phaddd      m4, m5
    paddd       m4, m1

    palignr     m5, m3, m2, 8                ; m5 = src[4-11]
    pmaddwd     m5, m0
    palignr     m6, m3, m2, 10               ; m6 = src[5-12]
    pmaddwd     m6, m0
    phaddd      m5, m6

    palignr     m6, m3, m2, 12               ; m6 = src[6-13]
    pmaddwd     m6, m0
    palignr     m7, m3, m2, 14               ; m2 = src[7-14]
    pmaddwd     m7, m0
    phaddd      m6, m7
    phaddd      m5, m6
    paddd       m5, m1
%ifidn %3, pp
    psrad       m4, 6
    psrad       m5, 6
    packusdw    m4, m5
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, 2
    psrad       m5, 2
    packssdw    m4, m5
%endif
    movu        [r2 + x], m4

    movu        m2, [r0 + 32 + x]            ; m2 = src[16-23]

    pmaddwd     m4, m3, m0                   ; m3 = src[8-15]
    palignr     m5, m2, m3, 2                ; m5 = src[9-16]
    pmaddwd     m5, m0
    phaddd      m4, m5

    palignr     m5, m2, m3, 4                ; m5 = src[10-17]
    pmaddwd     m5, m0
    palignr     m6, m2, m3, 6                ; m6 = src[11-18]
    pmaddwd     m6, m0
    phaddd      m5, m6
    phaddd      m4, m5
    paddd       m4, m1

    palignr     m5, m2, m3, 8                ; m5 = src[12-19]
    pmaddwd     m5, m0
    palignr     m6, m2, m3, 10               ; m6 = src[13-20]
    pmaddwd     m6, m0
    phaddd      m5, m6

    palignr     m6, m2, m3, 12               ; m6 = src[14-21]
    pmaddwd     m6, m0
    palignr     m2, m3, 14                   ; m3 = src[15-22]
    pmaddwd     m2, m0
    phaddd      m6, m2
    phaddd      m5, m6
    paddd       m5, m1
%ifidn %3, pp 
    psrad       m4, 6
    psrad       m5, 6
    packusdw    m4, m5
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, 2
    psrad       m5, 2
    packssdw    m4, m5
%endif
    movu        [r2 + 16 + x], m4

%assign x x+32
%endrep

    add         r0, r1
    add         r2, r3

    dec         r4d
    jnz         .loopH
    RET
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_16x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W16 16, 4, pp
FILTER_HOR_LUMA_W16 16, 8, pp
FILTER_HOR_LUMA_W16 16, 12, pp
FILTER_HOR_LUMA_W16 16, 16, pp
FILTER_HOR_LUMA_W16 16, 32, pp
FILTER_HOR_LUMA_W16 16, 64, pp

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_ps_16x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W16 16, 4, ps
FILTER_HOR_LUMA_W16 16, 8, ps
FILTER_HOR_LUMA_W16 16, 12, ps
FILTER_HOR_LUMA_W16 16, 16, ps
FILTER_HOR_LUMA_W16 16, 32, ps
FILTER_HOR_LUMA_W16 16, 64, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_32x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W16 32, 8, pp
FILTER_HOR_LUMA_W16 32, 16, pp
FILTER_HOR_LUMA_W16 32, 24, pp
FILTER_HOR_LUMA_W16 32, 32, pp
FILTER_HOR_LUMA_W16 32, 64, pp

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_ps_32x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W16 32, 8, ps
FILTER_HOR_LUMA_W16 32, 16, ps
FILTER_HOR_LUMA_W16 32, 24, ps
FILTER_HOR_LUMA_W16 32, 32, ps
FILTER_HOR_LUMA_W16 32, 64, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_48x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W16 48, 64, pp

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_ps_48x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W16 48, 64, ps

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_64x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W16 64, 16, pp
FILTER_HOR_LUMA_W16 64, 32, pp
FILTER_HOR_LUMA_W16 64, 48, pp
FILTER_HOR_LUMA_W16 64, 64, pp

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_ps_64x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W16 64, 16, ps
FILTER_HOR_LUMA_W16 64, 32, ps
FILTER_HOR_LUMA_W16 64, 48, ps
FILTER_HOR_LUMA_W16 64, 64, ps

;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_HOR_LUMA_W24 3
INIT_XMM sse4
cglobal interp_8tap_horiz_%3_%1x%2, 4, 7, 8

    add         r1, r1
    add         r3, r3
    mov         r4d, r4m
    sub         r0, 6
    shl         r4d, 4

%ifdef PIC
    lea         r6, [tab_LumaCoeff]
    mova        m0, [r6 + r4]
%else
    mova        m0, [tab_LumaCoeff + r4]
%endif
%ifidn %3, pp 
    mova        m1, [pd_32]
%else
    mova        m1, [pd_n32768]
%endif

    mov         r4d, %2
%ifidn %3, ps
    cmp         r5m, byte 0
    je          .loopH
    lea         r6, [r1 + 2 * r1]
    sub         r0, r6
    add         r4d, 7
%endif

.loopH
    movu        m2, [r0]                     ; m2 = src[0-7]
    movu        m3, [r0 + 16]                ; m3 = src[8-15]

    pmaddwd     m4, m2, m0
    palignr     m5, m3, m2, 2                ; m5 = src[1-8]
    pmaddwd     m5, m0
    phaddd      m4, m5

    palignr     m5, m3, m2, 4                ; m5 = src[2-9]
    pmaddwd     m5, m0
    palignr     m6, m3, m2, 6                ; m6 = src[3-10]
    pmaddwd     m6, m0
    phaddd      m5, m6
    phaddd      m4, m5
    paddd       m4, m1

    palignr     m5, m3, m2, 8                ; m5 = src[4-11]
    pmaddwd     m5, m0
    palignr     m6, m3, m2, 10               ; m6 = src[5-12]
    pmaddwd     m6, m0
    phaddd      m5, m6

    palignr     m6, m3, m2, 12               ; m6 = src[6-13]
    pmaddwd     m6, m0
    palignr     m7, m3, m2, 14               ; m7 = src[7-14]
    pmaddwd     m7, m0
    phaddd      m6, m7
    phaddd      m5, m6
    paddd       m5, m1
%ifidn %3, pp 
    psrad       m4, 6
    psrad       m5, 6
    packusdw    m4, m5
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, 2
    psrad       m5, 2
    packssdw    m4, m5
%endif
    movu        [r2], m4

    movu        m2, [r0 + 32]                ; m2 = src[16-23]

    pmaddwd     m4, m3, m0                   ; m3 = src[8-15]
    palignr     m5, m2, m3, 2                ; m5 = src[1-8]
    pmaddwd     m5, m0
    phaddd      m4, m5

    palignr     m5, m2, m3, 4                ; m5 = src[2-9]
    pmaddwd     m5, m0
    palignr     m6, m2, m3, 6                ; m6 = src[3-10]
    pmaddwd     m6, m0
    phaddd      m5, m6
    phaddd      m4, m5
    paddd       m4, m1

    palignr     m5, m2, m3, 8                ; m5 = src[4-11]
    pmaddwd     m5, m0
    palignr     m6, m2, m3, 10               ; m6 = src[5-12]
    pmaddwd     m6, m0
    phaddd      m5, m6

    palignr     m6, m2, m3, 12               ; m6 = src[6-13]
    pmaddwd     m6, m0
    palignr     m7, m2, m3, 14               ; m7 = src[7-14]
    pmaddwd     m7, m0
    phaddd      m6, m7
    phaddd      m5, m6
    paddd       m5, m1
%ifidn %3, pp 
    psrad       m4, 6
    psrad       m5, 6
    packusdw    m4, m5
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, 2
    psrad       m5, 2
    packssdw    m4, m5
%endif
    movu        [r2 + 16], m4

    movu        m3, [r0 + 48]                ; m3 = src[24-31]

    pmaddwd     m4, m2, m0                   ; m2 = src[16-23]
    palignr     m5, m3, m2, 2                ; m5 = src[1-8]
    pmaddwd     m5, m0
    phaddd      m4, m5

    palignr     m5, m3, m2, 4                ; m5 = src[2-9]
    pmaddwd     m5, m0
    palignr     m6, m3, m2, 6                ; m6 = src[3-10]
    pmaddwd     m6, m0
    phaddd      m5, m6
    phaddd      m4, m5
    paddd       m4, m1

    palignr     m5, m3, m2, 8                ; m5 = src[4-11]
    pmaddwd     m5, m0
    palignr     m6, m3, m2, 10               ; m6 = src[5-12]
    pmaddwd     m6, m0
    phaddd      m5, m6

    palignr     m6, m3, m2, 12               ; m6 = src[6-13]
    pmaddwd     m6, m0
    palignr     m7, m3, m2, 14               ; m7 = src[7-14]
    pmaddwd     m7, m0
    phaddd      m6, m7
    phaddd      m5, m6
    paddd       m5, m1
%ifidn %3, pp 
    psrad       m4, 6
    psrad       m5, 6
    packusdw    m4, m5
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, 2
    psrad       m5, 2
    packssdw    m4, m5
%endif
    movu        [r2 + 32], m4

    add         r0, r1
    add         r2, r3

    dec         r4d
    jnz         .loopH
    RET
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_24x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W24 24, 32, pp

;----------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_ps_24x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;----------------------------------------------------------------------------------------------------------------------------
FILTER_HOR_LUMA_W24 24, 32, ps
