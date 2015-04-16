;*****************************************************************************
;* Copyright (C) 2013 x265 project
;*
;* Authors: Nabajit Deka <nabajit@multicorewareinc.com>
;*          Murugan Vairavel <murugan@multicorewareinc.com>
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

tab_c_32:         times 4 dd 32
tab_c_n32768:     times 4 dd -32768
tab_c_524800:     times 4 dd 524800
tab_c_n8192:      times 8 dw -8192
pd_524800:        times 8 dd 524800

tab_Tm16:         db 0, 1, 2, 3, 4,  5,  6, 7, 2, 3, 4,  5, 6, 7, 8, 9

tab_ChromaCoeff:  dw  0, 64,  0,  0
                  dw -2, 58, 10, -2
                  dw -4, 54, 16, -2
                  dw -6, 46, 28, -4
                  dw -4, 36, 36, -4
                  dw -4, 28, 46, -6
                  dw -2, 16, 54, -4
                  dw -2, 10, 58, -2

tab_ChromaCoeffV: times 4 dw 0, 64
                  times 4 dw 0, 0

                  times 4 dw -2, 58
                  times 4 dw 10, -2

                  times 4 dw -4, 54
                  times 4 dw 16, -2

                  times 4 dw -6, 46 
                  times 4 dw 28, -4

                  times 4 dw -4, 36
                  times 4 dw 36, -4

                  times 4 dw -4, 28
                  times 4 dw 46, -6

                  times 4 dw -2, 16
                  times 4 dw 54, -4

                  times 4 dw -2, 10
                  times 4 dw 58, -2

tab_LumaCoeff:    dw   0, 0,  0,  64,  0,   0,  0,  0
                  dw  -1, 4, -10, 58,  17, -5,  1,  0
                  dw  -1, 4, -11, 40,  40, -11, 4, -1
                  dw   0, 1, -5,  17,  58, -10, 4, -1

tab_LumaCoeffV:   times 4 dw 0, 0
                  times 4 dw 0, 64
                  times 4 dw 0, 0
                  times 4 dw 0, 0

                  times 4 dw -1, 4
                  times 4 dw -10, 58
                  times 4 dw 17, -5
                  times 4 dw 1, 0

                  times 4 dw -1, 4
                  times 4 dw -11, 40
                  times 4 dw 40, -11
                  times 4 dw 4, -1

                  times 4 dw 0, 1
                  times 4 dw -5, 17
                  times 4 dw 58, -10
                  times 4 dw 4, -1
ALIGN 32
tab_LumaCoeffVer: times 8 dw 0, 0
                  times 8 dw 0, 64
                  times 8 dw 0, 0
                  times 8 dw 0, 0

                  times 8 dw -1, 4
                  times 8 dw -10, 58
                  times 8 dw 17, -5
                  times 8 dw 1, 0

                  times 8 dw -1, 4
                  times 8 dw -11, 40
                  times 8 dw 40, -11
                  times 8 dw 4, -1

                  times 8 dw 0, 1
                  times 8 dw -5, 17
                  times 8 dw 58, -10
                  times 8 dw 4, -1

SECTION .text
cextern pd_32
cextern pw_pixel_max
cextern pd_n32768
cextern pw_2000

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

.loopH:
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

.loopH:
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

.loopH:
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

.loopH:
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

.loopH:
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

%macro FILTER_W2_2 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + r1]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1
%ifidn %1, pp
    psrad       m3,         6
    packusdw    m3,         m3
    CLIPW       m3,         m7,    m6
%else
    psrad       m3,         2
    packssdw    m3,         m3
%endif
    movd        [r2],       m3
    pextrd      [r2 + r3],  m3, 1
%endmacro

%macro FILTER_W4_2 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + r1]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + r1 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m7,    m6
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2],       m3
    movhps      [r2 + r3],  m3
%endmacro

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_CHROMA_H 6
INIT_XMM sse4
cglobal interp_4tap_horiz_%3_%1x%2, 4, %4, %5

    add         r3,       r3
    add         r1,       r1
    sub         r0,       2
    mov         r4d,      r4m
    add         r4d,      r4d

%ifdef PIC
    lea         r%6,      [tab_ChromaCoeff]
    movh        m0,       [r%6 + r4 * 4]
%else
    movh        m0,       [tab_ChromaCoeff + r4 * 4]
%endif

    punpcklqdq  m0,       m0
    mova        m2,       [tab_Tm16]

%ifidn %3, ps
    mova        m1,       [tab_c_n32768]
    cmp         r5m, byte 0
    je          .skip
    sub         r0, r1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0

    %if %1 == 4
        movu        m4,         [r0 + 4]
        pshufb      m4,         m4, m2
        pmaddwd     m4,         m0
        phaddd      m3,         m4
    %else
        phaddd      m3,         m3
    %endif

    paddd       m3,         m1
    psrad       m3,         2
    packssdw    m3,         m3

    %if %1 == 2
        movd        [r2],       m3
    %else
        movh        [r2],       m3
    %endif

    add         r0, r1
    add         r2, r3
    FILTER_W%1_2 %3
    lea         r0,       [r0 + 2 * r1]
    lea         r2,       [r2 + 2 * r3]

.skip:

%else     ;%ifidn %3, ps
    pxor        m7,       m7
    mova        m6,       [pw_pixel_max]
    mova        m1,       [tab_c_32]
%endif    ;%ifidn %3, ps

    FILTER_W%1_2 %3

%rep (%2/2) - 1
    lea         r0,       [r0 + 2 * r1]
    lea         r2,       [r2 + 2 * r3]
    FILTER_W%1_2 %3
%endrep

RET
%endmacro

FILTER_CHROMA_H 2, 4, pp, 6, 8, 5
FILTER_CHROMA_H 2, 8, pp, 6, 8, 5
FILTER_CHROMA_H 4, 2, pp, 6, 8, 5
FILTER_CHROMA_H 4, 4, pp, 6, 8, 5
FILTER_CHROMA_H 4, 8, pp, 6, 8, 5
FILTER_CHROMA_H 4, 16, pp, 6, 8, 5

FILTER_CHROMA_H 2, 4, ps, 7, 5, 6
FILTER_CHROMA_H 2, 8, ps, 7, 5, 6
FILTER_CHROMA_H 4, 2, ps, 7, 6, 6
FILTER_CHROMA_H 4, 4, ps, 7, 6, 6
FILTER_CHROMA_H 4, 8, ps, 7, 6, 6
FILTER_CHROMA_H 4, 16, ps, 7, 6, 6

FILTER_CHROMA_H 2, 16, pp, 6, 8, 5
FILTER_CHROMA_H 4, 32, pp, 6, 8, 5
FILTER_CHROMA_H 2, 16, ps, 7, 5, 6
FILTER_CHROMA_H 4, 32, ps, 7, 6, 6


%macro FILTER_W6_1 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m4,         [r0 + 8]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m4,         m4
    paddd       m4,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m4,         6
    packusdw    m3,         m4
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m4,         2
    packssdw    m3,         m4
%endif
    movh        [r2],       m3
    pextrd      [r2 + 8],   m3, 2
%endmacro

cglobal chroma_filter_pp_6x1_internal
    FILTER_W6_1 pp
    ret

cglobal chroma_filter_ps_6x1_internal
    FILTER_W6_1 ps
    ret

%macro FILTER_W8_1 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 8]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 12]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2],       m3
    movhps      [r2 + 8],   m3
%endmacro

cglobal chroma_filter_pp_8x1_internal
    FILTER_W8_1 pp
    ret

cglobal chroma_filter_ps_8x1_internal
    FILTER_W8_1 ps
    ret

%macro FILTER_W12_1 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 8]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 12]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2],       m3
    movhps      [r2 + 8],   m3

    movu        m3,         [r0 + 16]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 20]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

%ifidn %1, pp
    psrad       m3,         6
    packusdw    m3,         m3
    CLIPW       m3,         m6, m7
%else
    psrad       m3,         2
    packssdw    m3,         m3
%endif
    movh        [r2 + 16],  m3
%endmacro

cglobal chroma_filter_pp_12x1_internal
    FILTER_W12_1 pp
    ret

cglobal chroma_filter_ps_12x1_internal
    FILTER_W12_1 ps
    ret

%macro FILTER_W16_1 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 8]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 12]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2],       m3
    movhps      [r2 + 8],   m3

    movu        m3,         [r0 + 16]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 20]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 24]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 28]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2 + 16],  m3
    movhps      [r2 + 24],  m3
%endmacro

cglobal chroma_filter_pp_16x1_internal
    FILTER_W16_1 pp
    ret

cglobal chroma_filter_ps_16x1_internal
    FILTER_W16_1 ps
    ret

%macro FILTER_W24_1 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 8]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 12]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2],       m3
    movhps      [r2 + 8],   m3

    movu        m3,         [r0 + 16]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 20]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 24]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 28]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2 + 16],  m3
    movhps      [r2 + 24],  m3

    movu        m3,         [r0 + 32]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 36]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 40]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 44]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2 + 32],  m3
    movhps      [r2 + 40],  m3
%endmacro

cglobal chroma_filter_pp_24x1_internal
    FILTER_W24_1 pp
    ret

cglobal chroma_filter_ps_24x1_internal
    FILTER_W24_1 ps
    ret

%macro FILTER_W32_1 1
    movu        m3,         [r0]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 8]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 12]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2],       m3
    movhps      [r2 + 8],   m3

    movu        m3,         [r0 + 16]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 20]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 24]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 28]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2 + 16],  m3
    movhps      [r2 + 24],  m3

    movu        m3,         [r0 + 32]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 36]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 40]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 44]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2 + 32],  m3
    movhps      [r2 + 40],  m3

    movu        m3,         [r0 + 48]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + 52]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + 56]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + 60]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2 + 48],  m3
    movhps      [r2 + 56],  m3
%endmacro

cglobal chroma_filter_pp_32x1_internal
    FILTER_W32_1 pp
    ret

cglobal chroma_filter_ps_32x1_internal
    FILTER_W32_1 ps
    ret

%macro FILTER_W8o_1 2
    movu        m3,         [r0 + %2]
    pshufb      m3,         m3, m2
    pmaddwd     m3,         m0
    movu        m4,         [r0 + %2 + 4]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m3,         m4
    paddd       m3,         m1

    movu        m5,         [r0 + %2 + 8]
    pshufb      m5,         m5, m2
    pmaddwd     m5,         m0
    movu        m4,         [r0 + %2 + 12]
    pshufb      m4,         m4, m2
    pmaddwd     m4,         m0
    phaddd      m5,         m4
    paddd       m5,         m1
%ifidn %1, pp
    psrad       m3,         6
    psrad       m5,         6
    packusdw    m3,         m5
    CLIPW       m3,         m6,    m7
%else
    psrad       m3,         2
    psrad       m5,         2
    packssdw    m3,         m5
%endif
    movh        [r2 + %2],       m3
    movhps      [r2 + %2 + 8],   m3
%endmacro

%macro FILTER_W48_1 1
    FILTER_W8o_1 %1, 0
    FILTER_W8o_1 %1, 16
    FILTER_W8o_1 %1, 32
    FILTER_W8o_1 %1, 48
    FILTER_W8o_1 %1, 64
    FILTER_W8o_1 %1, 80
%endmacro

cglobal chroma_filter_pp_48x1_internal
    FILTER_W48_1 pp
    ret

cglobal chroma_filter_ps_48x1_internal
    FILTER_W48_1 ps
    ret

%macro FILTER_W64_1 1
    FILTER_W8o_1 %1, 0
    FILTER_W8o_1 %1, 16
    FILTER_W8o_1 %1, 32
    FILTER_W8o_1 %1, 48
    FILTER_W8o_1 %1, 64
    FILTER_W8o_1 %1, 80
    FILTER_W8o_1 %1, 96
    FILTER_W8o_1 %1, 112
%endmacro

cglobal chroma_filter_pp_64x1_internal
    FILTER_W64_1 pp
    ret

cglobal chroma_filter_ps_64x1_internal
    FILTER_W64_1 ps
    ret


;-----------------------------------------------------------------------------
; void interp_4tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------

INIT_XMM sse4
%macro IPFILTER_CHROMA 6
cglobal interp_4tap_horiz_%3_%1x%2, 4, %5, %6

    add         r3,        r3
    add         r1,        r1
    sub         r0,         2
    mov         r4d,        r4m
    add         r4d,        r4d

%ifdef PIC
    lea         r%4,       [tab_ChromaCoeff]
    movh        m0,       [r%4 + r4 * 4]
%else
    movh        m0,       [tab_ChromaCoeff + r4 * 4]
%endif

    punpcklqdq  m0,       m0
    mova        m2,       [tab_Tm16]

%ifidn %3, ps
    mova        m1,       [tab_c_n32768]
    cmp         r5m, byte 0
    je          .skip
    sub         r0, r1
    call chroma_filter_%3_%1x1_internal
    add         r0, r1
    add         r2, r3
    call chroma_filter_%3_%1x1_internal
    add         r0, r1
    add         r2, r3
    call chroma_filter_%3_%1x1_internal
    add         r0, r1
    add         r2, r3
.skip:
%else
    mova        m1,         [tab_c_32]
    pxor        m6,         m6
    mova        m7,         [pw_pixel_max]
%endif

    call chroma_filter_%3_%1x1_internal
%rep %2 - 1
    add         r0,       r1
    add         r2,       r3
    call chroma_filter_%3_%1x1_internal
%endrep
RET
%endmacro
IPFILTER_CHROMA 6, 8, pp, 5, 6, 8
IPFILTER_CHROMA 8, 2, pp, 5, 6, 8
IPFILTER_CHROMA 8, 4, pp, 5, 6, 8
IPFILTER_CHROMA 8, 6, pp, 5, 6, 8
IPFILTER_CHROMA 8, 8, pp, 5, 6, 8
IPFILTER_CHROMA 8, 16, pp, 5, 6, 8
IPFILTER_CHROMA 8, 32, pp, 5, 6, 8
IPFILTER_CHROMA 12, 16, pp, 5, 6, 8
IPFILTER_CHROMA 16, 4, pp, 5, 6, 8
IPFILTER_CHROMA 16, 8, pp, 5, 6, 8
IPFILTER_CHROMA 16, 12, pp, 5, 6, 8
IPFILTER_CHROMA 16, 16, pp, 5, 6, 8
IPFILTER_CHROMA 16, 32, pp, 5, 6, 8
IPFILTER_CHROMA 24, 32, pp, 5, 6, 8
IPFILTER_CHROMA 32, 8, pp, 5, 6, 8
IPFILTER_CHROMA 32, 16, pp, 5, 6, 8
IPFILTER_CHROMA 32, 24, pp, 5, 6, 8
IPFILTER_CHROMA 32, 32, pp, 5, 6, 8

IPFILTER_CHROMA 6, 8, ps, 6, 7, 6
IPFILTER_CHROMA 8, 2, ps, 6, 7, 6
IPFILTER_CHROMA 8, 4, ps, 6, 7, 6
IPFILTER_CHROMA 8, 6, ps, 6, 7, 6
IPFILTER_CHROMA 8, 8, ps, 6, 7, 6
IPFILTER_CHROMA 8, 16, ps, 6, 7, 6
IPFILTER_CHROMA 8, 32, ps, 6, 7, 6
IPFILTER_CHROMA 12, 16, ps, 6, 7, 6
IPFILTER_CHROMA 16, 4, ps, 6, 7, 6
IPFILTER_CHROMA 16, 8, ps, 6, 7, 6
IPFILTER_CHROMA 16, 12, ps, 6, 7, 6
IPFILTER_CHROMA 16, 16, ps, 6, 7, 6
IPFILTER_CHROMA 16, 32, ps, 6, 7, 6
IPFILTER_CHROMA 24, 32, ps, 6, 7, 6
IPFILTER_CHROMA 32, 8, ps, 6, 7, 6
IPFILTER_CHROMA 32, 16, ps, 6, 7, 6
IPFILTER_CHROMA 32, 24, ps, 6, 7, 6
IPFILTER_CHROMA 32, 32, ps, 6, 7, 6

IPFILTER_CHROMA 6, 16, pp, 5, 6, 8
IPFILTER_CHROMA 8, 12, pp, 5, 6, 8
IPFILTER_CHROMA 8, 64, pp, 5, 6, 8
IPFILTER_CHROMA 12, 32, pp, 5, 6, 8
IPFILTER_CHROMA 16, 24, pp, 5, 6, 8
IPFILTER_CHROMA 16, 64, pp, 5, 6, 8
IPFILTER_CHROMA 24, 64, pp, 5, 6, 8
IPFILTER_CHROMA 32, 48, pp, 5, 6, 8
IPFILTER_CHROMA 32, 64, pp, 5, 6, 8
IPFILTER_CHROMA 6, 16, ps, 6, 7, 6
IPFILTER_CHROMA 8, 12, ps, 6, 7, 6
IPFILTER_CHROMA 8, 64, ps, 6, 7, 6
IPFILTER_CHROMA 12, 32, ps, 6, 7, 6
IPFILTER_CHROMA 16, 24, ps, 6, 7, 6
IPFILTER_CHROMA 16, 64, ps, 6, 7, 6
IPFILTER_CHROMA 24, 64, ps, 6, 7, 6
IPFILTER_CHROMA 32, 48, ps, 6, 7, 6
IPFILTER_CHROMA 32, 64, ps, 6, 7, 6

IPFILTER_CHROMA 48, 64, pp, 5, 6, 8
IPFILTER_CHROMA 64, 48, pp, 5, 6, 8
IPFILTER_CHROMA 64, 64, pp, 5, 6, 8
IPFILTER_CHROMA 64, 32, pp, 5, 6, 8
IPFILTER_CHROMA 64, 16, pp, 5, 6, 8
IPFILTER_CHROMA 48, 64, ps, 6, 7, 6
IPFILTER_CHROMA 64, 48, ps, 6, 7, 6
IPFILTER_CHROMA 64, 64, ps, 6, 7, 6
IPFILTER_CHROMA 64, 32, ps, 6, 7, 6
IPFILTER_CHROMA 64, 16, ps, 6, 7, 6


%macro PROCESS_CHROMA_SP_W4_4R 0
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r6 + 0 *16]                ;m0=[0+1]         Row1

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m1, m4                          ;m1=[1 2]
    pmaddwd    m1, [r6 + 0 *16]                ;m1=[1+2]         Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[2 3]
    pmaddwd    m2, m4, [r6 + 0 *16]            ;m2=[2+3]         Row3
    pmaddwd    m4, [r6 + 1 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3]     Row1 done

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m5, m4                          ;m5=[3 4]
    pmaddwd    m3, m5, [r6 + 0 *16]            ;m3=[3+4]         Row4
    pmaddwd    m5, [r6 + 1 * 16]
    paddd      m1, m5                          ;m1 = [1+2+3+4]   Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[4 5]
    pmaddwd    m4, [r6 + 1 * 16]
    paddd      m2, m4                          ;m2=[2+3+4+5]     Row3

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[5 6]
    pmaddwd    m5, [r6 + 1 * 16]
    paddd      m3, m5                          ;m3=[3+4+5+6]     Row4
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_%3_%1x%2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_SS 4
INIT_XMM sse2
cglobal interp_4tap_vert_%3_%1x%2, 5, 7, %4 ,0-gprsize

    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_ChromaCoeffV + r4]
%endif

    mov       dword [rsp], %2/4

%ifnidn %3, ss
    %ifnidn %3, ps
        mova      m7, [pw_pixel_max]
        %ifidn %3, pp
            mova      m6, [tab_c_32]
        %else
            mova      m6, [tab_c_524800]
        %endif
    %else
        mova      m6, [tab_c_n32768]
    %endif
%endif

.loopH:
    mov       r4d, (%1/4)
.loopW:
    PROCESS_CHROMA_SP_W4_4R

%ifidn %3, ss
    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3
%elifidn %3, ps
    paddd     m0, m6
    paddd     m1, m6
    paddd     m2, m6
    paddd     m3, m6
    psrad     m0, 2
    psrad     m1, 2
    psrad     m2, 2
    psrad     m3, 2

    packssdw  m0, m1
    packssdw  m2, m3
%else
    paddd     m0, m6
    paddd     m1, m6
    paddd     m2, m6
    paddd     m3, m6
    %ifidn %3, pp
        psrad     m0, 6
        psrad     m1, 6
        psrad     m2, 6
        psrad     m3, 6
    %else
        psrad     m0, 10
        psrad     m1, 10
        psrad     m2, 10
        psrad     m3, 10
    %endif
    packssdw  m0, m1
    packssdw  m2, m3
    pxor      m5, m5
    CLIPW2    m0, m2, m5, m7
%endif

    movh      [r2], m0
    movhps    [r2 + r3], m0
    lea       r5, [r2 + 2 * r3]
    movh      [r5], m2
    movhps    [r5 + r3], m2

    lea       r5, [4 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 2 * 4

    dec       r4d
    jnz       .loopW

    lea       r0, [r0 + 4 * r1 - 2 * %1]
    lea       r2, [r2 + 4 * r3 - 2 * %1]

    dec       dword [rsp]
    jnz       .loopH

    RET
%endmacro

    FILTER_VER_CHROMA_SS 4, 4, ss, 6
    FILTER_VER_CHROMA_SS 4, 8, ss, 6
    FILTER_VER_CHROMA_SS 16, 16, ss, 6
    FILTER_VER_CHROMA_SS 16, 8, ss, 6
    FILTER_VER_CHROMA_SS 16, 12, ss, 6
    FILTER_VER_CHROMA_SS 12, 16, ss, 6
    FILTER_VER_CHROMA_SS 16, 4, ss, 6
    FILTER_VER_CHROMA_SS 4, 16, ss, 6
    FILTER_VER_CHROMA_SS 32, 32, ss, 6
    FILTER_VER_CHROMA_SS 32, 16, ss, 6
    FILTER_VER_CHROMA_SS 16, 32, ss, 6
    FILTER_VER_CHROMA_SS 32, 24, ss, 6
    FILTER_VER_CHROMA_SS 24, 32, ss, 6
    FILTER_VER_CHROMA_SS 32, 8, ss, 6

    FILTER_VER_CHROMA_SS 4, 4, ps, 7
    FILTER_VER_CHROMA_SS 4, 8, ps, 7
    FILTER_VER_CHROMA_SS 16, 16, ps, 7
    FILTER_VER_CHROMA_SS 16, 8, ps, 7
    FILTER_VER_CHROMA_SS 16, 12, ps, 7
    FILTER_VER_CHROMA_SS 12, 16, ps, 7
    FILTER_VER_CHROMA_SS 16, 4, ps, 7
    FILTER_VER_CHROMA_SS 4, 16, ps, 7
    FILTER_VER_CHROMA_SS 32, 32, ps, 7
    FILTER_VER_CHROMA_SS 32, 16, ps, 7
    FILTER_VER_CHROMA_SS 16, 32, ps, 7
    FILTER_VER_CHROMA_SS 32, 24, ps, 7
    FILTER_VER_CHROMA_SS 24, 32, ps, 7
    FILTER_VER_CHROMA_SS 32, 8, ps, 7

    FILTER_VER_CHROMA_SS 4, 4, sp, 8
    FILTER_VER_CHROMA_SS 4, 8, sp, 8
    FILTER_VER_CHROMA_SS 16, 16, sp, 8
    FILTER_VER_CHROMA_SS 16, 8, sp, 8
    FILTER_VER_CHROMA_SS 16, 12, sp, 8
    FILTER_VER_CHROMA_SS 12, 16, sp, 8
    FILTER_VER_CHROMA_SS 16, 4, sp, 8
    FILTER_VER_CHROMA_SS 4, 16, sp, 8
    FILTER_VER_CHROMA_SS 32, 32, sp, 8
    FILTER_VER_CHROMA_SS 32, 16, sp, 8
    FILTER_VER_CHROMA_SS 16, 32, sp, 8
    FILTER_VER_CHROMA_SS 32, 24, sp, 8
    FILTER_VER_CHROMA_SS 24, 32, sp, 8
    FILTER_VER_CHROMA_SS 32, 8, sp, 8

    FILTER_VER_CHROMA_SS 4, 4, pp, 8
    FILTER_VER_CHROMA_SS 4, 8, pp, 8
    FILTER_VER_CHROMA_SS 16, 16, pp, 8
    FILTER_VER_CHROMA_SS 16, 8, pp, 8
    FILTER_VER_CHROMA_SS 16, 12, pp, 8
    FILTER_VER_CHROMA_SS 12, 16, pp, 8
    FILTER_VER_CHROMA_SS 16, 4, pp, 8
    FILTER_VER_CHROMA_SS 4, 16, pp, 8
    FILTER_VER_CHROMA_SS 32, 32, pp, 8
    FILTER_VER_CHROMA_SS 32, 16, pp, 8
    FILTER_VER_CHROMA_SS 16, 32, pp, 8
    FILTER_VER_CHROMA_SS 32, 24, pp, 8
    FILTER_VER_CHROMA_SS 24, 32, pp, 8
    FILTER_VER_CHROMA_SS 32, 8, pp, 8


    FILTER_VER_CHROMA_SS 16, 24, ss, 6
    FILTER_VER_CHROMA_SS 12, 32, ss, 6
    FILTER_VER_CHROMA_SS 4, 32, ss, 6
    FILTER_VER_CHROMA_SS 32, 64, ss, 6
    FILTER_VER_CHROMA_SS 16, 64, ss, 6
    FILTER_VER_CHROMA_SS 32, 48, ss, 6
    FILTER_VER_CHROMA_SS 24, 64, ss, 6

    FILTER_VER_CHROMA_SS 16, 24, ps, 7
    FILTER_VER_CHROMA_SS 12, 32, ps, 7
    FILTER_VER_CHROMA_SS 4, 32, ps, 7
    FILTER_VER_CHROMA_SS 32, 64, ps, 7
    FILTER_VER_CHROMA_SS 16, 64, ps, 7
    FILTER_VER_CHROMA_SS 32, 48, ps, 7
    FILTER_VER_CHROMA_SS 24, 64, ps, 7

    FILTER_VER_CHROMA_SS 16, 24, sp, 8
    FILTER_VER_CHROMA_SS 12, 32, sp, 8
    FILTER_VER_CHROMA_SS 4, 32, sp, 8
    FILTER_VER_CHROMA_SS 32, 64, sp, 8
    FILTER_VER_CHROMA_SS 16, 64, sp, 8
    FILTER_VER_CHROMA_SS 32, 48, sp, 8
    FILTER_VER_CHROMA_SS 24, 64, sp, 8

    FILTER_VER_CHROMA_SS 16, 24, pp, 8
    FILTER_VER_CHROMA_SS 12, 32, pp, 8
    FILTER_VER_CHROMA_SS 4, 32, pp, 8
    FILTER_VER_CHROMA_SS 32, 64, pp, 8
    FILTER_VER_CHROMA_SS 16, 64, pp, 8
    FILTER_VER_CHROMA_SS 32, 48, pp, 8
    FILTER_VER_CHROMA_SS 24, 64, pp, 8


    FILTER_VER_CHROMA_SS 48, 64, ss, 6
    FILTER_VER_CHROMA_SS 64, 48, ss, 6
    FILTER_VER_CHROMA_SS 64, 64, ss, 6
    FILTER_VER_CHROMA_SS 64, 32, ss, 6
    FILTER_VER_CHROMA_SS 64, 16, ss, 6

    FILTER_VER_CHROMA_SS 48, 64, ps, 7
    FILTER_VER_CHROMA_SS 64, 48, ps, 7
    FILTER_VER_CHROMA_SS 64, 64, ps, 7
    FILTER_VER_CHROMA_SS 64, 32, ps, 7
    FILTER_VER_CHROMA_SS 64, 16, ps, 7

    FILTER_VER_CHROMA_SS 48, 64, sp, 8
    FILTER_VER_CHROMA_SS 64, 48, sp, 8
    FILTER_VER_CHROMA_SS 64, 64, sp, 8
    FILTER_VER_CHROMA_SS 64, 32, sp, 8
    FILTER_VER_CHROMA_SS 64, 16, sp, 8

    FILTER_VER_CHROMA_SS 48, 64, pp, 8
    FILTER_VER_CHROMA_SS 64, 48, pp, 8
    FILTER_VER_CHROMA_SS 64, 64, pp, 8
    FILTER_VER_CHROMA_SS 64, 32, pp, 8
    FILTER_VER_CHROMA_SS 64, 16, pp, 8


%macro PROCESS_CHROMA_SP_W2_4R 1
    movd       m0, [r0]
    movd       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]

    lea        r0, [r0 + 2 * r1]
    movd       m2, [r0]
    punpcklwd  m1, m2                          ;m1=[1 2]
    punpcklqdq m0, m1                          ;m0=[0 1 1 2]
    pmaddwd    m0, [%1 + 0 *16]                ;m0=[0+1 1+2] Row 1-2

    movd       m1, [r0 + r1]
    punpcklwd  m2, m1                          ;m2=[2 3]

    lea        r0, [r0 + 2 * r1]
    movd       m3, [r0]
    punpcklwd  m1, m3                          ;m2=[3 4]
    punpcklqdq m2, m1                          ;m2=[2 3 3 4]

    pmaddwd    m4, m2, [%1 + 1 * 16]           ;m4=[2+3 3+4] Row 1-2
    pmaddwd    m2, [%1 + 0 * 16]               ;m2=[2+3 3+4] Row 3-4
    paddd      m0, m4                          ;m0=[0+1+2+3 1+2+3+4] Row 1-2

    movd       m1, [r0 + r1]
    punpcklwd  m3, m1                          ;m3=[4 5]

    movd       m4, [r0 + 2 * r1]
    punpcklwd  m1, m4                          ;m1=[5 6]
    punpcklqdq m3, m1                          ;m2=[4 5 5 6]
    pmaddwd    m3, [%1 + 1 * 16]               ;m3=[4+5 5+6] Row 3-4
    paddd      m2, m3                          ;m2=[2+3+4+5 3+4+5+6] Row 3-4
%endmacro

;---------------------------------------------------------------------------------------------------------------------
; void interp_4tap_vertical_%2_2x%1(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W2 3
INIT_XMM sse4
cglobal interp_4tap_vert_%2_2x%1, 5, 6, %3

    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif

    mov       r4d, (%1/4)
%ifnidn %2, ss
    %ifnidn %2, ps
        pxor      m7, m7
        mova      m6, [pw_pixel_max]
        %ifidn %2, pp
            mova      m5, [tab_c_32]
        %else
            mova      m5, [tab_c_524800]
        %endif
    %else
        mova      m5, [tab_c_n32768]
    %endif
%endif

.loopH:
    PROCESS_CHROMA_SP_W2_4R r5
%ifidn %2, ss
    psrad     m0, 6
    psrad     m2, 6
    packssdw  m0, m2
%elifidn %2, ps
    paddd     m0, m5
    paddd     m2, m5
    psrad     m0, 2
    psrad     m2, 2
    packssdw  m0, m2
%else
    paddd     m0, m5
    paddd     m2, m5
    %ifidn %2, pp
        psrad     m0, 6
        psrad     m2, 6
    %else
        psrad     m0, 10
        psrad     m2, 10
    %endif
    packusdw  m0, m2
    CLIPW     m0, m7,    m6
%endif

    movd      [r2], m0
    pextrd    [r2 + r3], m0, 1
    lea       r2, [r2 + 2 * r3]
    pextrd    [r2], m0, 2
    pextrd    [r2 + r3], m0, 3

    lea       r2, [r2 + 2 * r3]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

FILTER_VER_CHROMA_W2 4, ss, 5
FILTER_VER_CHROMA_W2 8, ss, 5

FILTER_VER_CHROMA_W2 4, pp, 8
FILTER_VER_CHROMA_W2 8, pp, 8

FILTER_VER_CHROMA_W2 4, ps, 6
FILTER_VER_CHROMA_W2 8, ps, 6

FILTER_VER_CHROMA_W2 4, sp, 8
FILTER_VER_CHROMA_W2 8, sp, 8

FILTER_VER_CHROMA_W2 16, ss, 5
FILTER_VER_CHROMA_W2 16, pp, 8
FILTER_VER_CHROMA_W2 16, ps, 6
FILTER_VER_CHROMA_W2 16, sp, 8


;---------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_%1_4x2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W4 3
INIT_XMM sse4
cglobal interp_4tap_vert_%2_4x%1, 5, 6, %3

    add        r1d, r1d
    add        r3d, r3d
    sub        r0, r1
    shl        r4d, 5

%ifdef PIC
    lea        r5, [tab_ChromaCoeffV]
    lea        r5, [r5 + r4]
%else
    lea        r5, [tab_ChromaCoeffV + r4]
%endif

%ifnidn %2, 2
    mov        r4d, %1/2
%endif

%ifnidn %2, ss
    %ifnidn %2, ps
        pxor      m6, m6
        mova      m5, [pw_pixel_max]
        %ifidn %2, pp
            mova      m4, [tab_c_32]
        %else
            mova      m4, [tab_c_524800]
        %endif
    %else
        mova      m4, [tab_c_n32768]
    %endif
%endif

%ifnidn %2, 2
.loop:
%endif

    movh       m0, [r0]
    movh       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r5 + 0 *16]                ;m0=[0+1]  Row1

    lea        r0, [r0 + 2 * r1]
    movh       m2, [r0]
    punpcklwd  m1, m2                          ;m1=[1 2]
    pmaddwd    m1, [r5 + 0 *16]                ;m1=[1+2]  Row2

    movh       m3, [r0 + r1]
    punpcklwd  m2, m3                          ;m4=[2 3]
    pmaddwd    m2, [r5 + 1 * 16]
    paddd      m0, m2                          ;m0=[0+1+2+3]  Row1 done

    movh       m2, [r0 + 2 * r1]
    punpcklwd  m3, m2                          ;m5=[3 4]
    pmaddwd    m3, [r5 + 1 * 16]
    paddd      m1, m3                          ;m1=[1+2+3+4]  Row2 done

%ifidn %2, ss
    psrad     m0, 6
    psrad     m1, 6
    packssdw  m0, m1
%elifidn %2, ps
    paddd     m0, m4
    paddd     m1, m4
    psrad     m0, 2
    psrad     m1, 2
    packssdw  m0, m1
%else
    paddd     m0, m4
    paddd     m1, m4
    %ifidn %2, pp
        psrad     m0, 6
        psrad     m1, 6
    %else
        psrad     m0, 10
        psrad     m1, 10
    %endif
    packusdw  m0, m1
    CLIPW     m0, m6,    m5
%endif

    movh       [r2], m0
    movhps     [r2 + r3], m0

%ifnidn %2, 2
    lea        r2, [r2 + r3 * 2]
    dec        r4d
    jnz        .loop
%endif

    RET
%endmacro

FILTER_VER_CHROMA_W4 2, ss, 4
FILTER_VER_CHROMA_W4 2, pp, 7
FILTER_VER_CHROMA_W4 2, ps, 5
FILTER_VER_CHROMA_W4 2, sp, 7

FILTER_VER_CHROMA_W4 4, ss, 4
FILTER_VER_CHROMA_W4 4, pp, 7
FILTER_VER_CHROMA_W4 4, ps, 5
FILTER_VER_CHROMA_W4 4, sp, 7

;-------------------------------------------------------------------------------------------------------------------
; void interp_4tap_vertical_%1_6x8(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W6 3
INIT_XMM sse4
cglobal interp_4tap_vert_%2_6x%1, 5, 7, %3

    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_ChromaCoeffV + r4]
%endif

    mov       r4d, %1/4

%ifnidn %2, ss
    %ifnidn %2, ps
        mova      m7, [pw_pixel_max]
        %ifidn %2, pp
            mova      m6, [tab_c_32]
        %else
            mova      m6, [tab_c_524800]
        %endif
    %else
        mova      m6, [tab_c_n32768]
    %endif
%endif

.loopH:
    PROCESS_CHROMA_SP_W4_4R

%ifidn %2, ss
    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3
%elifidn %2, ps
    paddd     m0, m6
    paddd     m1, m6
    paddd     m2, m6
    paddd     m3, m6
    psrad     m0, 2
    psrad     m1, 2
    psrad     m2, 2
    psrad     m3, 2

    packssdw  m0, m1
    packssdw  m2, m3
%else
    paddd     m0, m6
    paddd     m1, m6
    paddd     m2, m6
    paddd     m3, m6
    %ifidn %2, pp
        psrad     m0, 6
        psrad     m1, 6
        psrad     m2, 6
        psrad     m3, 6
    %else
        psrad     m0, 10
        psrad     m1, 10
        psrad     m2, 10
        psrad     m3, 10
    %endif
    packssdw  m0, m1
    packssdw  m2, m3
    pxor      m5, m5
    CLIPW2    m0, m2, m5, m7
%endif

    movh      [r2], m0
    movhps    [r2 + r3], m0
    lea       r5, [r2 + 2 * r3]
    movh      [r5], m2
    movhps    [r5 + r3], m2

    lea       r5, [4 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 2 * 4

    PROCESS_CHROMA_SP_W2_4R r6

%ifidn %2, ss
    psrad     m0, 6
    psrad     m2, 6
    packssdw  m0, m2
%elifidn %2, ps
    paddd     m0, m6
    paddd     m2, m6
    psrad     m0, 2
    psrad     m2, 2
    packssdw  m0, m2
%else
    paddd     m0, m6
    paddd     m2, m6
    %ifidn %2, pp
        psrad     m0, 6
        psrad     m2, 6
    %else
        psrad     m0, 10
        psrad     m2, 10
    %endif
    packusdw  m0, m2
    CLIPW     m0, m5,    m7
%endif

    movd      [r2], m0
    pextrd    [r2 + r3], m0, 1
    lea       r2, [r2 + 2 * r3]
    pextrd    [r2], m0, 2
    pextrd    [r2 + r3], m0, 3

    sub       r0, 2 * 4
    lea       r2, [r2 + 2 * r3 - 2 * 4]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

FILTER_VER_CHROMA_W6 8, ss, 6
FILTER_VER_CHROMA_W6 8, ps, 7
FILTER_VER_CHROMA_W6 8, sp, 8
FILTER_VER_CHROMA_W6 8, pp, 8

FILTER_VER_CHROMA_W6 16, ss, 6
FILTER_VER_CHROMA_W6 16, ps, 7
FILTER_VER_CHROMA_W6 16, sp, 8
FILTER_VER_CHROMA_W6 16, pp, 8

%macro PROCESS_CHROMA_SP_W8_2R 0
    movu       m1, [r0]
    movu       m3, [r0 + r1]
    punpcklwd  m0, m1, m3
    pmaddwd    m0, [r5 + 0 * 16]                ;m0 = [0l+1l]  Row1l
    punpckhwd  m1, m3
    pmaddwd    m1, [r5 + 0 * 16]                ;m1 = [0h+1h]  Row1h

    movu       m4, [r0 + 2 * r1]
    punpcklwd  m2, m3, m4
    pmaddwd    m2, [r5 + 0 * 16]                ;m2 = [1l+2l]  Row2l
    punpckhwd  m3, m4
    pmaddwd    m3, [r5 + 0 * 16]                ;m3 = [1h+2h]  Row2h

    lea        r0, [r0 + 2 * r1]
    movu       m5, [r0 + r1]
    punpcklwd  m6, m4, m5
    pmaddwd    m6, [r5 + 1 * 16]                ;m6 = [2l+3l]  Row1l
    paddd      m0, m6                           ;m0 = [0l+1l+2l+3l]  Row1l sum
    punpckhwd  m4, m5
    pmaddwd    m4, [r5 + 1 * 16]                ;m6 = [2h+3h]  Row1h
    paddd      m1, m4                           ;m1 = [0h+1h+2h+3h]  Row1h sum

    movu       m4, [r0 + 2 * r1]
    punpcklwd  m6, m5, m4
    pmaddwd    m6, [r5 + 1 * 16]                ;m6 = [3l+4l]  Row2l
    paddd      m2, m6                           ;m2 = [1l+2l+3l+4l]  Row2l sum
    punpckhwd  m5, m4
    pmaddwd    m5, [r5 + 1 * 16]                ;m1 = [3h+4h]  Row2h
    paddd      m3, m5                           ;m3 = [1h+2h+3h+4h]  Row2h sum
%endmacro

;----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert_%3_%1x%2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_CHROMA_W8 4
INIT_XMM sse2
cglobal interp_4tap_vert_%3_%1x%2, 5, 6, %4

    add       r1d, r1d
    add       r3d, r3d
    sub       r0, r1
    shl       r4d, 5

%ifdef PIC
    lea       r5, [tab_ChromaCoeffV]
    lea       r5, [r5 + r4]
%else
    lea       r5, [tab_ChromaCoeffV + r4]
%endif

    mov       r4d, %2/2

%ifidn %3, pp
    mova      m7, [tab_c_32]
%elifidn %3, sp
    mova      m7, [tab_c_524800]
%elifidn %3, ps
    mova      m7, [tab_c_n32768]
%endif

.loopH:
    PROCESS_CHROMA_SP_W8_2R

%ifidn %3, ss
    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3
%elifidn %3, ps
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
    psrad     m0, 2
    psrad     m1, 2
    psrad     m2, 2
    psrad     m3, 2

    packssdw  m0, m1
    packssdw  m2, m3
%else
    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7
    %ifidn %3, pp
        psrad     m0, 6
        psrad     m1, 6
        psrad     m2, 6
        psrad     m3, 6
    %else
        psrad     m0, 10
        psrad     m1, 10
        psrad     m2, 10
        psrad     m3, 10
    %endif
    packssdw  m0, m1
    packssdw  m2, m3
    pxor      m5, m5
    mova      m6, [pw_pixel_max]
    CLIPW2    m0, m2, m5, m6
%endif

    movu      [r2], m0
    movu      [r2 + r3], m2

    lea       r2, [r2 + 2 * r3]

    dec       r4d
    jnz       .loopH

    RET
%endmacro

FILTER_VER_CHROMA_W8 8, 2, ss, 7
FILTER_VER_CHROMA_W8 8, 4, ss, 7
FILTER_VER_CHROMA_W8 8, 6, ss, 7
FILTER_VER_CHROMA_W8 8, 8, ss, 7
FILTER_VER_CHROMA_W8 8, 16, ss, 7
FILTER_VER_CHROMA_W8 8, 32, ss, 7

FILTER_VER_CHROMA_W8 8, 2, sp, 8
FILTER_VER_CHROMA_W8 8, 4, sp, 8
FILTER_VER_CHROMA_W8 8, 6, sp, 8
FILTER_VER_CHROMA_W8 8, 8, sp, 8
FILTER_VER_CHROMA_W8 8, 16, sp, 8
FILTER_VER_CHROMA_W8 8, 32, sp, 8

FILTER_VER_CHROMA_W8 8, 2, ps, 8
FILTER_VER_CHROMA_W8 8, 4, ps, 8
FILTER_VER_CHROMA_W8 8, 6, ps, 8
FILTER_VER_CHROMA_W8 8, 8, ps, 8
FILTER_VER_CHROMA_W8 8, 16, ps, 8
FILTER_VER_CHROMA_W8 8, 32, ps, 8

FILTER_VER_CHROMA_W8 8, 2, pp, 8
FILTER_VER_CHROMA_W8 8, 4, pp, 8
FILTER_VER_CHROMA_W8 8, 6, pp, 8
FILTER_VER_CHROMA_W8 8, 8, pp, 8
FILTER_VER_CHROMA_W8 8, 16, pp, 8
FILTER_VER_CHROMA_W8 8, 32, pp, 8

FILTER_VER_CHROMA_W8 8, 12, ss, 7
FILTER_VER_CHROMA_W8 8, 64, ss, 7
FILTER_VER_CHROMA_W8 8, 12, sp, 8
FILTER_VER_CHROMA_W8 8, 64, sp, 8
FILTER_VER_CHROMA_W8 8, 12, ps, 8
FILTER_VER_CHROMA_W8 8, 64, ps, 8
FILTER_VER_CHROMA_W8 8, 12, pp, 8
FILTER_VER_CHROMA_W8 8, 64, pp, 8


INIT_XMM sse2
cglobal chroma_p2s, 3, 7, 3

    ; load width and height
    mov         r3d, r3m
    mov         r4d, r4m
    add         r1, r1

    ; load constant
    mova        m2, [tab_c_n8192]

.loopH:

    xor         r5d, r5d
.loopW:
    lea         r6, [r0 + r5 * 2]

    movu        m0, [r6]
    psllw       m0, 4
    paddw       m0, m2

    movu        m1, [r6 + r1]
    psllw       m1, 4
    paddw       m1, m2

    add         r5d, 8
    cmp         r5d, r3d
    lea         r6, [r2 + r5 * 2]
    jg          .width4
    movu        [r6 + FENC_STRIDE / 2 * 0 - 16], m0
    movu        [r6 + FENC_STRIDE / 2 * 2 - 16], m1
    je          .nextH
    jmp         .loopW

.width4:
    test        r3d, 4
    jz          .width2
    test        r3d, 2
    movh        [r6 + FENC_STRIDE / 2 * 0 - 16], m0
    movh        [r6 + FENC_STRIDE / 2 * 2 - 16], m1
    lea         r6, [r6 + 8]
    pshufd      m0, m0, 2
    pshufd      m1, m1, 2
    jz          .nextH

.width2:
    movd        [r6 + FENC_STRIDE / 2 * 0 - 16], m0
    movd        [r6 + FENC_STRIDE / 2 * 2 - 16], m1

.nextH:
    lea         r0, [r0 + r1 * 2]
    add         r2, FENC_STRIDE / 2 * 4

    sub         r4d, 2
    jnz         .loopH

    RET

%macro PROCESS_LUMA_VER_W4_4R 0
    movq       m0, [r0]
    movq       m1, [r0 + r1]
    punpcklwd  m0, m1                          ;m0=[0 1]
    pmaddwd    m0, [r6 + 0 *16]                ;m0=[0+1]  Row1

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m1, m4                          ;m1=[1 2]
    pmaddwd    m1, [r6 + 0 *16]                ;m1=[1+2]  Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[2 3]
    pmaddwd    m2, m4, [r6 + 0 *16]            ;m2=[2+3]  Row3
    pmaddwd    m4, [r6 + 1 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3]  Row1

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m5, m4                          ;m5=[3 4]
    pmaddwd    m3, m5, [r6 + 0 *16]            ;m3=[3+4]  Row4
    pmaddwd    m5, [r6 + 1 * 16]
    paddd      m1, m5                          ;m1 = [1+2+3+4]  Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[4 5]
    pmaddwd    m6, m4, [r6 + 1 * 16]
    paddd      m2, m6                          ;m2=[2+3+4+5]  Row3
    pmaddwd    m4, [r6 + 2 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3+4+5]  Row1

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m5, m4                          ;m5=[5 6]
    pmaddwd    m6, m5, [r6 + 1 * 16]
    paddd      m3, m6                          ;m3=[3+4+5+6]  Row4
    pmaddwd    m5, [r6 + 2 * 16]
    paddd      m1, m5                          ;m1=[1+2+3+4+5+6]  Row2

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[6 7]
    pmaddwd    m6, m4, [r6 + 2 * 16]
    paddd      m2, m6                          ;m2=[2+3+4+5+6+7]  Row3
    pmaddwd    m4, [r6 + 3 * 16]
    paddd      m0, m4                          ;m0=[0+1+2+3+4+5+6+7]  Row1 end

    lea        r0, [r0 + 2 * r1]
    movq       m4, [r0]
    punpcklwd  m5, m4                          ;m5=[7 8]
    pmaddwd    m6, m5, [r6 + 2 * 16]
    paddd      m3, m6                          ;m3=[3+4+5+6+7+8]  Row4
    pmaddwd    m5, [r6 + 3 * 16]
    paddd      m1, m5                          ;m1=[1+2+3+4+5+6+7+8]  Row2 end

    movq       m5, [r0 + r1]
    punpcklwd  m4, m5                          ;m4=[8 9]
    pmaddwd    m4, [r6 + 3 * 16]
    paddd      m2, m4                          ;m2=[2+3+4+5+6+7+8+9]  Row3 end

    movq       m4, [r0 + 2 * r1]
    punpcklwd  m5, m4                          ;m5=[9 10]
    pmaddwd    m5, [r6 + 3 * 16]
    paddd      m3, m5                          ;m3=[3+4+5+6+7+8+9+10]  Row4 end
%endmacro

;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_PP 2
INIT_XMM sse4
cglobal interp_8tap_vert_pp_%1x%2, 5, 7, 8 ,0-gprsize

    add       r1d, r1d
    add       r3d, r3d
    lea       r5, [r1 + 2 * r1]
    sub       r0, r5
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_LumaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_LumaCoeffV + r4]
%endif

    mova      m7, [pd_32]

    mov       dword [rsp], %2/4
.loopH:
    mov       r4d, (%1/4)
.loopW:
    PROCESS_LUMA_VER_W4_4R

    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7

    psrad     m0, 6
    psrad     m1, 6
    psrad     m2, 6
    psrad     m3, 6

    packssdw  m0, m1
    packssdw  m2, m3

    pxor      m1, m1
    CLIPW2    m0, m2, m1, [pw_pixel_max]

    movh      [r2], m0
    movhps    [r2 + r3], m0
    lea       r5, [r2 + 2 * r3]
    movh      [r5], m2
    movhps    [r5 + r3], m2

    lea       r5, [8 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 2 * 4

    dec       r4d
    jnz       .loopW

    lea       r0, [r0 + 4 * r1 - 2 * %1]
    lea       r2, [r2 + 4 * r3 - 2 * %1]

    dec       dword [rsp]
    jnz       .loopH

    RET
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_PP 4, 4
    FILTER_VER_LUMA_PP 8, 8
    FILTER_VER_LUMA_PP 8, 4
    FILTER_VER_LUMA_PP 4, 8
    FILTER_VER_LUMA_PP 16, 16
    FILTER_VER_LUMA_PP 16, 8
    FILTER_VER_LUMA_PP 8, 16
    FILTER_VER_LUMA_PP 16, 12
    FILTER_VER_LUMA_PP 12, 16
    FILTER_VER_LUMA_PP 16, 4
    FILTER_VER_LUMA_PP 4, 16
    FILTER_VER_LUMA_PP 32, 32
    FILTER_VER_LUMA_PP 32, 16
    FILTER_VER_LUMA_PP 16, 32
    FILTER_VER_LUMA_PP 32, 24
    FILTER_VER_LUMA_PP 24, 32
    FILTER_VER_LUMA_PP 32, 8
    FILTER_VER_LUMA_PP 8, 32
    FILTER_VER_LUMA_PP 64, 64
    FILTER_VER_LUMA_PP 64, 32
    FILTER_VER_LUMA_PP 32, 64
    FILTER_VER_LUMA_PP 64, 48
    FILTER_VER_LUMA_PP 48, 64
    FILTER_VER_LUMA_PP 64, 16
    FILTER_VER_LUMA_PP 16, 64

%macro FILTER_VER_LUMA_AVX2_4x4 1
INIT_YMM avx2
cglobal interp_8tap_vert_%1_4x4, 4, 6, 7
    mov             r4d, r4m
    add             r1d, r1d
    add             r3d, r3d
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4

%ifidn %1,pp
    vbroadcasti128  m6, [pd_32]
%elifidn %1, sp
    mova            m6, [pd_524800]
%else
    vbroadcasti128  m6, [pd_n32768]
%endif

    movq            xm0, [r0]
    movq            xm1, [r0 + r1]
    punpcklwd       xm0, xm1
    movq            xm2, [r0 + r1 * 2]
    punpcklwd       xm1, xm2
    vinserti128     m0, m0, xm1, 1                  ; m0 = [2 1 1 0]
    pmaddwd         m0, [r5]
    movq            xm3, [r0 + r4]
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm4, [r0]
    punpcklwd       xm3, xm4
    vinserti128     m2, m2, xm3, 1                  ; m2 = [4 3 3 2]
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    pmaddwd         m2, [r5]
    paddd           m0, m5
    movq            xm3, [r0 + r1]
    punpcklwd       xm4, xm3
    movq            xm1, [r0 + r1 * 2]
    punpcklwd       xm3, xm1
    vinserti128     m4, m4, xm3, 1                  ; m4 = [6 5 5 4]
    pmaddwd         m5, m4, [r5 + 2 * mmsize]
    pmaddwd         m4, [r5 + 1 * mmsize]
    paddd           m0, m5
    paddd           m2, m4
    movq            xm3, [r0 + r4]
    punpcklwd       xm1, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm4, [r0]
    punpcklwd       xm3, xm4
    vinserti128     m1, m1, xm3, 1                  ; m1 = [8 7 7 6]
    pmaddwd         m5, m1, [r5 + 3 * mmsize]
    pmaddwd         m1, [r5 + 2 * mmsize]
    paddd           m0, m5
    paddd           m2, m1
    movq            xm3, [r0 + r1]
    punpcklwd       xm4, xm3
    movq            xm1, [r0 + 2 * r1]
    punpcklwd       xm3, xm1
    vinserti128     m4, m4, xm3, 1                  ; m4 = [A 9 9 8]
    pmaddwd         m4, [r5 + 3 * mmsize]
    paddd           m2, m4

%ifidn %1,ss
    psrad           m0, 6
    psrad           m2, 6
%else
    paddd           m0, m6
    paddd           m2, m6
%ifidn %1,pp
    psrad           m0, 6
    psrad           m2, 6
%elifidn %1, sp
    psrad           m0, 10
    psrad           m2, 10
%else
    psrad           m0, 2
    psrad           m2, 2
%endif
%endif

    packssdw        m0, m2
    pxor            m1, m1
%ifidn %1,pp
    CLIPW           m0, m1, [pw_pixel_max]
%elifidn %1, sp
    CLIPW           m0, m1, [pw_pixel_max]
%endif

    vextracti128    xm2, m0, 1
    lea             r4, [r3 * 3]
    movq            [r2], xm0
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r4], xm2
    RET
%endmacro

FILTER_VER_LUMA_AVX2_4x4 pp
FILTER_VER_LUMA_AVX2_4x4 ps
FILTER_VER_LUMA_AVX2_4x4 sp
FILTER_VER_LUMA_AVX2_4x4 ss

%macro FILTER_VER_LUMA_AVX2_8x8 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_8x8, 4, 6, 12
    mov             r4d, r4m
    add             r1d, r1d
    add             r3d, r3d
    shl             r4d, 7

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4

%ifidn %1,pp
    vbroadcasti128  m11, [pd_32]
%elifidn %1, sp
    mova            m11, [pd_524800]
%else
    vbroadcasti128  m11, [pd_n32768]
%endif

    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    pmaddwd         m2, [r5]
    paddd           m0, m4
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    pmaddwd         m3, [r5]
    paddd           m1, m5
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 2 * mmsize]
    paddd           m0, m6
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 2 * mmsize]
    paddd           m1, m7
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    pmaddwd         m5, [r5]
    paddd           m3, m7
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 3 * mmsize]
    paddd           m0, m8
    pmaddwd         m8, m6, [r5 + 2 * mmsize]
    paddd           m2, m8
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    pmaddwd         m6, [r5]
    paddd           m4, m8
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 3 * mmsize]
    paddd           m1, m9
    pmaddwd         m9, m7, [r5 + 2 * mmsize]
    paddd           m3, m9
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    pmaddwd         m7, [r5]
    paddd           m5, m9
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 3 * mmsize]
    paddd           m2, m10
    pmaddwd         m10, m8, [r5 + 2 * mmsize]
    pmaddwd         m8, [r5 + 1 * mmsize]
    paddd           m4, m10
    paddd           m6, m8
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm8, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm8, 1
    pmaddwd         m8, m9, [r5 + 3 * mmsize]
    paddd           m3, m8
    pmaddwd         m8, m9, [r5 + 2 * mmsize]
    pmaddwd         m9, [r5 + 1 * mmsize]
    paddd           m5, m8
    paddd           m7, m9
    movu            xm8, [r0 + r4]                  ; m8 = row 11
    punpckhwd       xm9, xm10, xm8
    punpcklwd       xm10, xm8
    vinserti128     m10, m10, xm9, 1
    pmaddwd         m9, m10, [r5 + 3 * mmsize]
    pmaddwd         m10, [r5 + 2 * mmsize]
    paddd           m4, m9
    paddd           m6, m10

    lea             r4, [r3 * 3]
%ifidn %1,ss
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
%else
    paddd           m0, m11
    paddd           m1, m11
    paddd           m2, m11
    paddd           m3, m11
%ifidn %1,pp
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
%elifidn %1, sp
    psrad           m0, 10
    psrad           m1, 10
    psrad           m2, 10
    psrad           m3, 10
%else
    psrad           m0, 2
    psrad           m1, 2
    psrad           m2, 2
    psrad           m3, 2
%endif
%endif

    packssdw        m0, m1
    packssdw        m2, m3
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    pxor            m10, m10
    mova            m9, [pw_pixel_max]
%ifidn %1,pp
    CLIPW           m0, m10, m9
    CLIPW           m2, m10, m9
%elifidn %1, sp
    CLIPW           m0, m10, m9
    CLIPW           m2, m10, m9
%endif

    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r4], xm3

    lea             r0, [r0 + r1 * 4]
    movu            xm2, [r0]                       ; m2 = row 12
    punpckhwd       xm3, xm8, xm2
    punpcklwd       xm8, xm2
    vinserti128     m8, m8, xm3, 1
    pmaddwd         m3, m8, [r5 + 3 * mmsize]
    pmaddwd         m8, [r5 + 2 * mmsize]
    paddd           m5, m3
    paddd           m7, m8
    movu            xm3, [r0 + r1]                  ; m3 = row 13
    punpckhwd       xm0, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm0, 1
    pmaddwd         m2, [r5 + 3 * mmsize]
    paddd           m6, m2
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhwd       xm1, xm3, xm0
    punpcklwd       xm3, xm0
    vinserti128     m3, m3, xm1, 1
    pmaddwd         m3, [r5 + 3 * mmsize]
    paddd           m7, m3

%ifidn %1,ss
    psrad           m4, 6
    psrad           m5, 6
    psrad           m6, 6
    psrad           m7, 6
%else
    paddd           m4, m11
    paddd           m5, m11
    paddd           m6, m11
    paddd           m7, m11
%ifidn %1,pp
    psrad           m4, 6
    psrad           m5, 6
    psrad           m6, 6
    psrad           m7, 6
%elifidn %1, sp
    psrad           m4, 10
    psrad           m5, 10
    psrad           m6, 10
    psrad           m7, 10
%else
    psrad           m4, 2
    psrad           m5, 2
    psrad           m6, 2
    psrad           m7, 2
%endif
%endif

    packssdw        m4, m5
    packssdw        m6, m7
    vpermq          m4, m4, 11011000b
    vpermq          m6, m6, 11011000b
%ifidn %1,pp
    CLIPW           m4, m10, m9
    CLIPW           m6, m10, m9
%elifidn %1, sp
    CLIPW           m4, m10, m9
    CLIPW           m6, m10, m9
%endif
    vextracti128    xm5, m4, 1
    vextracti128    xm7, m6, 1
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm4
    movu            [r2 + r3], xm5
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r4], xm7
    RET
%endif
%endmacro

FILTER_VER_LUMA_AVX2_8x8 pp
FILTER_VER_LUMA_AVX2_8x8 ps
FILTER_VER_LUMA_AVX2_8x8 sp
FILTER_VER_LUMA_AVX2_8x8 ss

%macro PROCESS_LUMA_AVX2_W8_16R 1
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m2, [r5]
    lea             r7, [r0 + r1 * 4]
    movu            xm4, [r7]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    paddd           m1, m5
    pmaddwd         m3, [r5]
    movu            xm5, [r7 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 2 * mmsize]
    paddd           m0, m6
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r7 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 2 * mmsize]
    paddd           m1, m7
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    paddd           m3, m7
    pmaddwd         m5, [r5]
    movu            xm7, [r7 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 3 * mmsize]
    paddd           m0, m8
    pmaddwd         m8, m6, [r5 + 2 * mmsize]
    paddd           m2, m8
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    paddd           m4, m8
    pmaddwd         m6, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm8, [r7]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 3 * mmsize]
    paddd           m1, m9
    pmaddwd         m9, m7, [r5 + 2 * mmsize]
    paddd           m3, m9
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    paddd           m5, m9
    pmaddwd         m7, [r5]
    movu            xm9, [r7 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 3 * mmsize]
    paddd           m2, m10
    pmaddwd         m10, m8, [r5 + 2 * mmsize]
    paddd           m4, m10
    pmaddwd         m10, m8, [r5 + 1 * mmsize]
    paddd           m6, m10
    pmaddwd         m8, [r5]
    movu            xm10, [r7 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm11, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddwd         m11, m9, [r5 + 3 * mmsize]
    paddd           m3, m11
    pmaddwd         m11, m9, [r5 + 2 * mmsize]
    paddd           m5, m11
    pmaddwd         m11, m9, [r5 + 1 * mmsize]
    paddd           m7, m11
    pmaddwd         m9, [r5]
    movu            xm11, [r7 + r4]                 ; m11 = row 11
    punpckhwd       xm12, xm10, xm11
    punpcklwd       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddwd         m12, m10, [r5 + 3 * mmsize]
    paddd           m4, m12
    pmaddwd         m12, m10, [r5 + 2 * mmsize]
    paddd           m6, m12
    pmaddwd         m12, m10, [r5 + 1 * mmsize]
    paddd           m8, m12
    pmaddwd         m10, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm12, [r7]                      ; m12 = row 12
    punpckhwd       xm13, xm11, xm12
    punpcklwd       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddwd         m13, m11, [r5 + 3 * mmsize]
    paddd           m5, m13
    pmaddwd         m13, m11, [r5 + 2 * mmsize]
    paddd           m7, m13
    pmaddwd         m13, m11, [r5 + 1 * mmsize]
    paddd           m9, m13
    pmaddwd         m11, [r5]

%ifidn %1,ss
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
    psrad           m4, 6
    psrad           m5, 6
%else
    paddd           m0, m14
    paddd           m1, m14
    paddd           m2, m14
    paddd           m3, m14
    paddd           m4, m14
    paddd           m5, m14
%ifidn %1,pp
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
    psrad           m4, 6
    psrad           m5, 6
%elifidn %1, sp
    psrad           m0, 10
    psrad           m1, 10
    psrad           m2, 10
    psrad           m3, 10
    psrad           m4, 10
    psrad           m5, 10
%else
    psrad           m0, 2
    psrad           m1, 2
    psrad           m2, 2
    psrad           m3, 2
    psrad           m4, 2
    psrad           m5, 2
%endif
%endif

    packssdw        m0, m1
    packssdw        m2, m3
    packssdw        m4, m5
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vpermq          m4, m4, 11011000b
    pxor            m5, m5
    mova            m3, [pw_pixel_max]
%ifidn %1,pp
    CLIPW           m0, m5, m3
    CLIPW           m2, m5, m3
    CLIPW           m4, m5, m3
%elifidn %1, sp
    CLIPW           m0, m5, m3
    CLIPW           m2, m5, m3
    CLIPW           m4, m5, m3
%endif

    vextracti128    xm1, m0, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    vextracti128    xm1, m2, 1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm1
    lea             r8, [r2 + r3 * 4]
    vextracti128    xm1, m4, 1
    movu            [r8], xm4
    movu            [r8 + r3], xm1

    movu            xm13, [r7 + r1]                 ; m13 = row 13
    punpckhwd       xm0, xm12, xm13
    punpcklwd       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddwd         m0, m12, [r5 + 3 * mmsize]
    paddd           m6, m0
    pmaddwd         m0, m12, [r5 + 2 * mmsize]
    paddd           m8, m0
    pmaddwd         m0, m12, [r5 + 1 * mmsize]
    paddd           m10, m0
    pmaddwd         m12, [r5]
    movu            xm0, [r7 + r1 * 2]              ; m0 = row 14
    punpckhwd       xm1, xm13, xm0
    punpcklwd       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddwd         m1, m13, [r5 + 3 * mmsize]
    paddd           m7, m1
    pmaddwd         m1, m13, [r5 + 2 * mmsize]
    paddd           m9, m1
    pmaddwd         m1, m13, [r5 + 1 * mmsize]
    paddd           m11, m1
    pmaddwd         m13, [r5]

%ifidn %1,ss
    psrad           m6, 6
    psrad           m7, 6
%else
    paddd           m6, m14
    paddd           m7, m14
%ifidn %1,pp
    psrad           m6, 6
    psrad           m7, 6
%elifidn %1, sp
    psrad           m6, 10
    psrad           m7, 10
%else
    psrad           m6, 2
    psrad           m7, 2
%endif
%endif

    packssdw        m6, m7
    vpermq          m6, m6, 11011000b
%ifidn %1,pp
    CLIPW           m6, m5, m3
%elifidn %1, sp
    CLIPW           m6, m5, m3
%endif
    vextracti128    xm7, m6, 1
    movu            [r8 + r3 * 2], xm6
    movu            [r8 + r6], xm7

    movu            xm1, [r7 + r4]                  ; m1 = row 15
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m2, m0, [r5 + 3 * mmsize]
    paddd           m8, m2
    pmaddwd         m2, m0, [r5 + 2 * mmsize]
    paddd           m10, m2
    pmaddwd         m2, m0, [r5 + 1 * mmsize]
    paddd           m12, m2
    pmaddwd         m0, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm2, [r7]                       ; m2 = row 16
    punpckhwd       xm6, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm6, 1
    pmaddwd         m6, m1, [r5 + 3 * mmsize]
    paddd           m9, m6
    pmaddwd         m6, m1, [r5 + 2 * mmsize]
    paddd           m11, m6
    pmaddwd         m6, m1, [r5 + 1 * mmsize]
    paddd           m13, m6
    pmaddwd         m1, [r5]
    movu            xm6, [r7 + r1]                  ; m6 = row 17
    punpckhwd       xm4, xm2, xm6
    punpcklwd       xm2, xm6
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 3 * mmsize]
    paddd           m10, m4
    pmaddwd         m4, m2, [r5 + 2 * mmsize]
    paddd           m12, m4
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m0, m2
    movu            xm4, [r7 + r1 * 2]              ; m4 = row 18
    punpckhwd       xm2, xm6, xm4
    punpcklwd       xm6, xm4
    vinserti128     m6, m6, xm2, 1
    pmaddwd         m2, m6, [r5 + 3 * mmsize]
    paddd           m11, m2
    pmaddwd         m2, m6, [r5 + 2 * mmsize]
    paddd           m13, m2
    pmaddwd         m6, [r5 + 1 * mmsize]
    paddd           m1, m6
    movu            xm2, [r7 + r4]                  ; m2 = row 19
    punpckhwd       xm6, xm4, xm2
    punpcklwd       xm4, xm2
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 3 * mmsize]
    paddd           m12, m6
    pmaddwd         m4, [r5 + 2 * mmsize]
    paddd           m0, m4
    lea             r7, [r7 + r1 * 4]
    movu            xm6, [r7]                       ; m6 = row 20
    punpckhwd       xm7, xm2, xm6
    punpcklwd       xm2, xm6
    vinserti128     m2, m2, xm7, 1
    pmaddwd         m7, m2, [r5 + 3 * mmsize]
    paddd           m13, m7
    pmaddwd         m2, [r5 + 2 * mmsize]
    paddd           m1, m2
    movu            xm7, [r7 + r1]                  ; m7 = row 21
    punpckhwd       xm2, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm2, 1
    pmaddwd         m6, [r5 + 3 * mmsize]
    paddd           m0, m6
    movu            xm2, [r7 + r1 * 2]              ; m2 = row 22
    punpckhwd       xm6, xm7, xm2
    punpcklwd       xm7, xm2
    vinserti128     m7, m7, xm6, 1
    pmaddwd         m7, [r5 + 3 * mmsize]
    paddd           m1, m7

%ifidn %1,ss
    psrad           m8, 6
    psrad           m9, 6
    psrad           m10, 6
    psrad           m11, 6
    psrad           m12, 6
    psrad           m13, 6
    psrad           m0, 6
    psrad           m1, 6
%else
    paddd           m8, m14
    paddd           m9, m14
    paddd           m10, m14
    paddd           m11, m14
    paddd           m12, m14
    paddd           m13, m14
    paddd           m0, m14
    paddd           m1, m14
%ifidn %1,pp
    psrad           m8, 6
    psrad           m9, 6
    psrad           m10, 6
    psrad           m11, 6
    psrad           m12, 6
    psrad           m13, 6
    psrad           m0, 6
    psrad           m1, 6
%elifidn %1, sp
    psrad           m8, 10
    psrad           m9, 10
    psrad           m10, 10
    psrad           m11, 10
    psrad           m12, 10
    psrad           m13, 10
    psrad           m0, 10
    psrad           m1, 10
%else
    psrad           m8, 2
    psrad           m9, 2
    psrad           m10, 2
    psrad           m11, 2
    psrad           m12, 2
    psrad           m13, 2
    psrad           m0, 2
    psrad           m1, 2
%endif
%endif

    packssdw        m8, m9
    packssdw        m10, m11
    packssdw        m12, m13
    packssdw        m0, m1
    vpermq          m8, m8, 11011000b
    vpermq          m10, m10, 11011000b
    vpermq          m12, m12, 11011000b
    vpermq          m0, m0, 11011000b
%ifidn %1,pp
    CLIPW           m8, m5, m3
    CLIPW           m10, m5, m3
    CLIPW           m12, m5, m3
    CLIPW           m0, m5, m3
%elifidn %1, sp
    CLIPW           m8, m5, m3
    CLIPW           m10, m5, m3
    CLIPW           m12, m5, m3
    CLIPW           m0, m5, m3
%endif
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    vextracti128    xm13, m12, 1
    vextracti128    xm1, m0, 1
    lea             r8, [r8 + r3 * 4]
    movu            [r8], xm8
    movu            [r8 + r3], xm9
    movu            [r8 + r3 * 2], xm10
    movu            [r8 + r6], xm11
    lea             r8, [r8 + r3 * 4]
    movu            [r8], xm12
    movu            [r8 + r3], xm13
    movu            [r8 + r3 * 2], xm0
    movu            [r8 + r6], xm1
%endmacro

%macro FILTER_VER_LUMA_AVX2_Nx16 2
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_%2x16, 4, 10, 15
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    vbroadcasti128  m14, [pd_32]
%elifidn %1, sp
    mova            m14, [pd_524800]
%else
    vbroadcasti128  m14, [pd_n32768]
%endif
    lea             r6, [r3 * 3]
    mov             r9d, %2 / 8
.loopW:
    PROCESS_LUMA_AVX2_W8_16R %1
    add             r2, 16
    add             r0, 16
    dec             r9d
    jnz             .loopW
    RET
%endif
%endmacro

FILTER_VER_LUMA_AVX2_Nx16 pp, 16
FILTER_VER_LUMA_AVX2_Nx16 pp, 32
FILTER_VER_LUMA_AVX2_Nx16 pp, 64
FILTER_VER_LUMA_AVX2_Nx16 ps, 16
FILTER_VER_LUMA_AVX2_Nx16 ps, 32
FILTER_VER_LUMA_AVX2_Nx16 ps, 64
FILTER_VER_LUMA_AVX2_Nx16 sp, 16
FILTER_VER_LUMA_AVX2_Nx16 sp, 32
FILTER_VER_LUMA_AVX2_Nx16 sp, 64
FILTER_VER_LUMA_AVX2_Nx16 ss, 16
FILTER_VER_LUMA_AVX2_Nx16 ss, 32
FILTER_VER_LUMA_AVX2_Nx16 ss, 64

%macro FILTER_VER_LUMA_AVX2_NxN 3
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%3_%1x%2, 4, 12, 15
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4

%ifidn %3,pp
    vbroadcasti128  m14, [pd_32]
%elifidn %3, sp
    mova            m14, [pd_524800]
%else
    vbroadcasti128  m14, [pd_n32768]
%endif

    lea             r6, [r3 * 3]
    lea             r11, [r1 * 4]
    mov             r9d, %2 / 16
.loopH:
    mov             r10d, %1 / 8
.loopW:
    PROCESS_LUMA_AVX2_W8_16R %3
    add             r2, 16
    add             r0, 16
    dec             r10d
    jnz             .loopW
    sub             r7, r11
    lea             r0, [r7 - 2 * %1 + 16]
    lea             r2, [r8 + r3 * 4 - 2 * %1 + 16]
    dec             r9d
    jnz             .loopH
    RET
%endif
%endmacro

FILTER_VER_LUMA_AVX2_NxN 16, 32, pp
FILTER_VER_LUMA_AVX2_NxN 16, 64, pp
FILTER_VER_LUMA_AVX2_NxN 24, 32, pp
FILTER_VER_LUMA_AVX2_NxN 32, 32, pp
FILTER_VER_LUMA_AVX2_NxN 32, 64, pp
FILTER_VER_LUMA_AVX2_NxN 48, 64, pp
FILTER_VER_LUMA_AVX2_NxN 64, 32, pp
FILTER_VER_LUMA_AVX2_NxN 64, 48, pp
FILTER_VER_LUMA_AVX2_NxN 64, 64, pp
FILTER_VER_LUMA_AVX2_NxN 16, 32, ps
FILTER_VER_LUMA_AVX2_NxN 16, 64, ps
FILTER_VER_LUMA_AVX2_NxN 24, 32, ps
FILTER_VER_LUMA_AVX2_NxN 32, 32, ps
FILTER_VER_LUMA_AVX2_NxN 32, 64, ps
FILTER_VER_LUMA_AVX2_NxN 48, 64, ps
FILTER_VER_LUMA_AVX2_NxN 64, 32, ps
FILTER_VER_LUMA_AVX2_NxN 64, 48, ps
FILTER_VER_LUMA_AVX2_NxN 64, 64, ps
FILTER_VER_LUMA_AVX2_NxN 16, 32, sp
FILTER_VER_LUMA_AVX2_NxN 16, 64, sp
FILTER_VER_LUMA_AVX2_NxN 24, 32, sp
FILTER_VER_LUMA_AVX2_NxN 32, 32, sp
FILTER_VER_LUMA_AVX2_NxN 32, 64, sp
FILTER_VER_LUMA_AVX2_NxN 48, 64, sp
FILTER_VER_LUMA_AVX2_NxN 64, 32, sp
FILTER_VER_LUMA_AVX2_NxN 64, 48, sp
FILTER_VER_LUMA_AVX2_NxN 64, 64, sp
FILTER_VER_LUMA_AVX2_NxN 16, 32, ss
FILTER_VER_LUMA_AVX2_NxN 16, 64, ss
FILTER_VER_LUMA_AVX2_NxN 24, 32, ss
FILTER_VER_LUMA_AVX2_NxN 32, 32, ss
FILTER_VER_LUMA_AVX2_NxN 32, 64, ss
FILTER_VER_LUMA_AVX2_NxN 48, 64, ss
FILTER_VER_LUMA_AVX2_NxN 64, 32, ss
FILTER_VER_LUMA_AVX2_NxN 64, 48, ss
FILTER_VER_LUMA_AVX2_NxN 64, 64, ss

%macro FILTER_VER_LUMA_AVX2_8xN 2
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_8x%2, 4, 9, 15
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    vbroadcasti128  m14, [pd_32]
%elifidn %1, sp
    mova            m14, [pd_524800]
%else
    vbroadcasti128  m14, [pd_n32768]
%endif
    lea             r6, [r3 * 3]
    lea             r7, [r1 * 4]
    mov             r8d, %2 / 16
.loopH:
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    paddd           m1, m5
    pmaddwd         m3, [r5]
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 2 * mmsize]
    paddd           m0, m6
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 2 * mmsize]
    paddd           m1, m7
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    paddd           m3, m7
    pmaddwd         m5, [r5]
    movu            xm7, [r0 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 3 * mmsize]
    paddd           m0, m8
    pmaddwd         m8, m6, [r5 + 2 * mmsize]
    paddd           m2, m8
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    paddd           m4, m8
    pmaddwd         m6, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm8, [r0]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 3 * mmsize]
    paddd           m1, m9
    pmaddwd         m9, m7, [r5 + 2 * mmsize]
    paddd           m3, m9
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    paddd           m5, m9
    pmaddwd         m7, [r5]
    movu            xm9, [r0 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 3 * mmsize]
    paddd           m2, m10
    pmaddwd         m10, m8, [r5 + 2 * mmsize]
    paddd           m4, m10
    pmaddwd         m10, m8, [r5 + 1 * mmsize]
    paddd           m6, m10
    pmaddwd         m8, [r5]
    movu            xm10, [r0 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm11, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddwd         m11, m9, [r5 + 3 * mmsize]
    paddd           m3, m11
    pmaddwd         m11, m9, [r5 + 2 * mmsize]
    paddd           m5, m11
    pmaddwd         m11, m9, [r5 + 1 * mmsize]
    paddd           m7, m11
    pmaddwd         m9, [r5]
    movu            xm11, [r0 + r4]                 ; m11 = row 11
    punpckhwd       xm12, xm10, xm11
    punpcklwd       xm10, xm11
    vinserti128     m10, m10, xm12, 1
    pmaddwd         m12, m10, [r5 + 3 * mmsize]
    paddd           m4, m12
    pmaddwd         m12, m10, [r5 + 2 * mmsize]
    paddd           m6, m12
    pmaddwd         m12, m10, [r5 + 1 * mmsize]
    paddd           m8, m12
    pmaddwd         m10, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm12, [r0]                      ; m12 = row 12
    punpckhwd       xm13, xm11, xm12
    punpcklwd       xm11, xm12
    vinserti128     m11, m11, xm13, 1
    pmaddwd         m13, m11, [r5 + 3 * mmsize]
    paddd           m5, m13
    pmaddwd         m13, m11, [r5 + 2 * mmsize]
    paddd           m7, m13
    pmaddwd         m13, m11, [r5 + 1 * mmsize]
    paddd           m9, m13
    pmaddwd         m11, [r5]

%ifidn %1,ss
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
    psrad           m4, 6
    psrad           m5, 6
%else
    paddd           m0, m14
    paddd           m1, m14
    paddd           m2, m14
    paddd           m3, m14
    paddd           m4, m14
    paddd           m5, m14
%ifidn %1,pp
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
    psrad           m4, 6
    psrad           m5, 6
%elifidn %1, sp
    psrad           m0, 10
    psrad           m1, 10
    psrad           m2, 10
    psrad           m3, 10
    psrad           m4, 10
    psrad           m5, 10
%else
    psrad           m0, 2
    psrad           m1, 2
    psrad           m2, 2
    psrad           m3, 2
    psrad           m4, 2
    psrad           m5, 2
%endif
%endif

    packssdw        m0, m1
    packssdw        m2, m3
    packssdw        m4, m5
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vpermq          m4, m4, 11011000b
    pxor            m5, m5
    mova            m3, [pw_pixel_max]
%ifidn %1,pp
    CLIPW           m0, m5, m3
    CLIPW           m2, m5, m3
    CLIPW           m4, m5, m3
%elifidn %1, sp
    CLIPW           m0, m5, m3
    CLIPW           m2, m5, m3
    CLIPW           m4, m5, m3
%endif

    vextracti128    xm1, m0, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    vextracti128    xm1, m2, 1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm1
    lea             r2, [r2 + r3 * 4]
    vextracti128    xm1, m4, 1
    movu            [r2], xm4
    movu            [r2 + r3], xm1

    movu            xm13, [r0 + r1]                 ; m13 = row 13
    punpckhwd       xm0, xm12, xm13
    punpcklwd       xm12, xm13
    vinserti128     m12, m12, xm0, 1
    pmaddwd         m0, m12, [r5 + 3 * mmsize]
    paddd           m6, m0
    pmaddwd         m0, m12, [r5 + 2 * mmsize]
    paddd           m8, m0
    pmaddwd         m0, m12, [r5 + 1 * mmsize]
    paddd           m10, m0
    pmaddwd         m12, [r5]
    movu            xm0, [r0 + r1 * 2]              ; m0 = row 14
    punpckhwd       xm1, xm13, xm0
    punpcklwd       xm13, xm0
    vinserti128     m13, m13, xm1, 1
    pmaddwd         m1, m13, [r5 + 3 * mmsize]
    paddd           m7, m1
    pmaddwd         m1, m13, [r5 + 2 * mmsize]
    paddd           m9, m1
    pmaddwd         m1, m13, [r5 + 1 * mmsize]
    paddd           m11, m1
    pmaddwd         m13, [r5]

%ifidn %1,ss
    psrad           m6, 6
    psrad           m7, 6
%else
    paddd           m6, m14
    paddd           m7, m14
%ifidn %1,pp
    psrad           m6, 6
    psrad           m7, 6
%elifidn %1, sp
    psrad           m6, 10
    psrad           m7, 10
%else
    psrad           m6, 2
    psrad           m7, 2
%endif
%endif

    packssdw        m6, m7
    vpermq          m6, m6, 11011000b
%ifidn %1,pp
    CLIPW           m6, m5, m3
%elifidn %1, sp
    CLIPW           m6, m5, m3
%endif
    vextracti128    xm7, m6, 1
    movu            [r2 + r3 * 2], xm6
    movu            [r2 + r6], xm7

    movu            xm1, [r0 + r4]                  ; m1 = row 15
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m2, m0, [r5 + 3 * mmsize]
    paddd           m8, m2
    pmaddwd         m2, m0, [r5 + 2 * mmsize]
    paddd           m10, m2
    pmaddwd         m2, m0, [r5 + 1 * mmsize]
    paddd           m12, m2
    pmaddwd         m0, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm2, [r0]                       ; m2 = row 16
    punpckhwd       xm6, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm6, 1
    pmaddwd         m6, m1, [r5 + 3 * mmsize]
    paddd           m9, m6
    pmaddwd         m6, m1, [r5 + 2 * mmsize]
    paddd           m11, m6
    pmaddwd         m6, m1, [r5 + 1 * mmsize]
    paddd           m13, m6
    pmaddwd         m1, [r5]
    movu            xm6, [r0 + r1]                  ; m6 = row 17
    punpckhwd       xm4, xm2, xm6
    punpcklwd       xm2, xm6
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 3 * mmsize]
    paddd           m10, m4
    pmaddwd         m4, m2, [r5 + 2 * mmsize]
    paddd           m12, m4
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m0, m2
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 18
    punpckhwd       xm2, xm6, xm4
    punpcklwd       xm6, xm4
    vinserti128     m6, m6, xm2, 1
    pmaddwd         m2, m6, [r5 + 3 * mmsize]
    paddd           m11, m2
    pmaddwd         m2, m6, [r5 + 2 * mmsize]
    paddd           m13, m2
    pmaddwd         m6, [r5 + 1 * mmsize]
    paddd           m1, m6
    movu            xm2, [r0 + r4]                  ; m2 = row 19
    punpckhwd       xm6, xm4, xm2
    punpcklwd       xm4, xm2
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 3 * mmsize]
    paddd           m12, m6
    pmaddwd         m4, [r5 + 2 * mmsize]
    paddd           m0, m4
    lea             r0, [r0 + r1 * 4]
    movu            xm6, [r0]                       ; m6 = row 20
    punpckhwd       xm7, xm2, xm6
    punpcklwd       xm2, xm6
    vinserti128     m2, m2, xm7, 1
    pmaddwd         m7, m2, [r5 + 3 * mmsize]
    paddd           m13, m7
    pmaddwd         m2, [r5 + 2 * mmsize]
    paddd           m1, m2
    movu            xm7, [r0 + r1]                  ; m7 = row 21
    punpckhwd       xm2, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm2, 1
    pmaddwd         m6, [r5 + 3 * mmsize]
    paddd           m0, m6
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 22
    punpckhwd       xm6, xm7, xm2
    punpcklwd       xm7, xm2
    vinserti128     m7, m7, xm6, 1
    pmaddwd         m7, [r5 + 3 * mmsize]
    paddd           m1, m7

%ifidn %1,ss
    psrad           m8, 6
    psrad           m9, 6
    psrad           m10, 6
    psrad           m11, 6
    psrad           m12, 6
    psrad           m13, 6
    psrad           m0, 6
    psrad           m1, 6
%else
    paddd           m8, m14
    paddd           m9, m14
    paddd           m10, m14
    paddd           m11, m14
    paddd           m12, m14
    paddd           m13, m14
    paddd           m0, m14
    paddd           m1, m14
%ifidn %1,pp
    psrad           m8, 6
    psrad           m9, 6
    psrad           m10, 6
    psrad           m11, 6
    psrad           m12, 6
    psrad           m13, 6
    psrad           m0, 6
    psrad           m1, 6
%elifidn %1, sp
    psrad           m8, 10
    psrad           m9, 10
    psrad           m10, 10
    psrad           m11, 10
    psrad           m12, 10
    psrad           m13, 10
    psrad           m0, 10
    psrad           m1, 10
%else
    psrad           m8, 2
    psrad           m9, 2
    psrad           m10, 2
    psrad           m11, 2
    psrad           m12, 2
    psrad           m13, 2
    psrad           m0, 2
    psrad           m1, 2
%endif
%endif

    packssdw        m8, m9
    packssdw        m10, m11
    packssdw        m12, m13
    packssdw        m0, m1
    vpermq          m8, m8, 11011000b
    vpermq          m10, m10, 11011000b
    vpermq          m12, m12, 11011000b
    vpermq          m0, m0, 11011000b
%ifidn %1,pp
    CLIPW           m8, m5, m3
    CLIPW           m10, m5, m3
    CLIPW           m12, m5, m3
    CLIPW           m0, m5, m3
%elifidn %1, sp
    CLIPW           m8, m5, m3
    CLIPW           m10, m5, m3
    CLIPW           m12, m5, m3
    CLIPW           m0, m5, m3
%endif
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    vextracti128    xm13, m12, 1
    vextracti128    xm1, m0, 1
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm8
    movu            [r2 + r3], xm9
    movu            [r2 + r3 * 2], xm10
    movu            [r2 + r6], xm11
    lea             r2, [r2 + r3 * 4]
    movu            [r2], xm12
    movu            [r2 + r3], xm13
    movu            [r2 + r3 * 2], xm0
    movu            [r2 + r6], xm1
    lea             r2, [r2 + r3 * 4]
    sub             r0, r7
    dec             r8d
    jnz             .loopH
    RET
%endif
%endmacro

FILTER_VER_LUMA_AVX2_8xN pp, 16
FILTER_VER_LUMA_AVX2_8xN pp, 32
FILTER_VER_LUMA_AVX2_8xN ps, 16
FILTER_VER_LUMA_AVX2_8xN ps, 32
FILTER_VER_LUMA_AVX2_8xN sp, 16
FILTER_VER_LUMA_AVX2_8xN sp, 32
FILTER_VER_LUMA_AVX2_8xN ss, 16
FILTER_VER_LUMA_AVX2_8xN ss, 32

%macro PROCESS_LUMA_AVX2_W8_8R 1
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m2, [r5]
    lea             r7, [r0 + r1 * 4]
    movu            xm4, [r7]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    paddd           m1, m5
    pmaddwd         m3, [r5]
    movu            xm5, [r7 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 2 * mmsize]
    paddd           m0, m6
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r7 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 2 * mmsize]
    paddd           m1, m7
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    paddd           m3, m7
    pmaddwd         m5, [r5]
    movu            xm7, [r7 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 3 * mmsize]
    paddd           m0, m8
    pmaddwd         m8, m6, [r5 + 2 * mmsize]
    paddd           m2, m8
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    paddd           m4, m8
    pmaddwd         m6, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm8, [r7]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 3 * mmsize]
    paddd           m1, m9
    pmaddwd         m9, m7, [r5 + 2 * mmsize]
    paddd           m3, m9
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    paddd           m5, m9
    pmaddwd         m7, [r5]
    movu            xm9, [r7 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 3 * mmsize]
    paddd           m2, m10
    pmaddwd         m10, m8, [r5 + 2 * mmsize]
    paddd           m4, m10
    pmaddwd         m8, [r5 + 1 * mmsize]
    paddd           m6, m8
    movu            xm10, [r7 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm8, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm8, 1
    pmaddwd         m8, m9, [r5 + 3 * mmsize]
    paddd           m3, m8
    pmaddwd         m8, m9, [r5 + 2 * mmsize]
    paddd           m5, m8
    pmaddwd         m9, [r5 + 1 * mmsize]
    paddd           m7, m9
    movu            xm8, [r7 + r4]                  ; m8 = row 11
    punpckhwd       xm9, xm10, xm8
    punpcklwd       xm10, xm8
    vinserti128     m10, m10, xm9, 1
    pmaddwd         m9, m10, [r5 + 3 * mmsize]
    paddd           m4, m9
    pmaddwd         m10, [r5 + 2 * mmsize]
    paddd           m6, m10
    lea             r7, [r7 + r1 * 4]
    movu            xm9, [r7]                       ; m9 = row 12
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 3 * mmsize]
    paddd           m5, m10
    pmaddwd         m8, [r5 + 2 * mmsize]
    paddd           m7, m8

%ifidn %1,ss
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
    psrad           m4, 6
    psrad           m5, 6
%else
    paddd           m0, m11
    paddd           m1, m11
    paddd           m2, m11
    paddd           m3, m11
    paddd           m4, m11
    paddd           m5, m11
%ifidn %1,pp
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
    psrad           m4, 6
    psrad           m5, 6
%elifidn %1, sp
    psrad           m0, 10
    psrad           m1, 10
    psrad           m2, 10
    psrad           m3, 10
    psrad           m4, 10
    psrad           m5, 10
%else
    psrad           m0, 2
    psrad           m1, 2
    psrad           m2, 2
    psrad           m3, 2
    psrad           m4, 2
    psrad           m5, 2
%endif
%endif

    packssdw        m0, m1
    packssdw        m2, m3
    packssdw        m4, m5
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    vpermq          m4, m4, 11011000b
    pxor            m8, m8
%ifidn %1,pp
    CLIPW           m0, m8, m12
    CLIPW           m2, m8, m12
    CLIPW           m4, m8, m12
%elifidn %1, sp
    CLIPW           m0, m8, m12
    CLIPW           m2, m8, m12
    CLIPW           m4, m8, m12
%endif

    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    vextracti128    xm5, m4, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3
    lea             r8, [r2 + r3 * 4]
    movu            [r8], xm4
    movu            [r8 + r3], xm5

    movu            xm10, [r7 + r1]                 ; m10 = row 13
    punpckhwd       xm0, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm0, 1
    pmaddwd         m9, [r5 + 3 * mmsize]
    paddd           m6, m9
    movu            xm0, [r7 + r1 * 2]              ; m0 = row 14
    punpckhwd       xm1, xm10, xm0
    punpcklwd       xm10, xm0
    vinserti128     m10, m10, xm1, 1
    pmaddwd         m10, [r5 + 3 * mmsize]
    paddd           m7, m10

%ifidn %1,ss
    psrad           m6, 6
    psrad           m7, 6
%else
    paddd           m6, m11
    paddd           m7, m11
%ifidn %1,pp
    psrad           m6, 6
    psrad           m7, 6
%elifidn %1, sp
    psrad           m6, 10
    psrad           m7, 10
%else
    psrad           m6, 2
    psrad           m7, 2
%endif
%endif

    packssdw        m6, m7
    vpermq          m6, m6, 11011000b
%ifidn %1,pp
    CLIPW           m6, m8, m12
%elifidn %1, sp
    CLIPW           m6, m8, m12
%endif
    vextracti128    xm7, m6, 1
    movu            [r8 + r3 * 2], xm6
    movu            [r8 + r6], xm7
%endmacro

%macro FILTER_VER_LUMA_AVX2_Nx8 2
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_%2x8, 4, 10, 13
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    vbroadcasti128  m11, [pd_32]
%elifidn %1, sp
    mova            m11, [pd_524800]
%else
    vbroadcasti128  m11, [pd_n32768]
%endif
    mova            m12, [pw_pixel_max]
    lea             r6, [r3 * 3]
    mov             r9d, %2 / 8
.loopW:
    PROCESS_LUMA_AVX2_W8_8R %1
    add             r2, 16
    add             r0, 16
    dec             r9d
    jnz             .loopW
    RET
%endif
%endmacro

FILTER_VER_LUMA_AVX2_Nx8 pp, 32
FILTER_VER_LUMA_AVX2_Nx8 pp, 16
FILTER_VER_LUMA_AVX2_Nx8 ps, 32
FILTER_VER_LUMA_AVX2_Nx8 ps, 16
FILTER_VER_LUMA_AVX2_Nx8 sp, 32
FILTER_VER_LUMA_AVX2_Nx8 sp, 16
FILTER_VER_LUMA_AVX2_Nx8 ss, 32
FILTER_VER_LUMA_AVX2_Nx8 ss, 16

%macro FILTER_VER_LUMA_AVX2_32x24 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_32x24, 4, 10, 15
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    vbroadcasti128  m14, [pd_32]
%elifidn %1, sp
    mova            m14, [pd_524800]
%else
    vbroadcasti128  m14, [pd_n32768]
%endif
    lea             r6, [r3 * 3]
    mov             r9d, 4
.loopW:
    PROCESS_LUMA_AVX2_W8_16R %1
    add             r2, 16
    add             r0, 16
    dec             r9d
    jnz             .loopW
    lea             r9, [r1 * 4]
    sub             r7, r9
    lea             r0, [r7 - 48]
    lea             r2, [r8 + r3 * 4 - 48]
    mova            m11, m14
    mova            m12, m3
    mov             r9d, 4
.loop:
    PROCESS_LUMA_AVX2_W8_8R %1
    add             r2, 16
    add             r0, 16
    dec             r9d
    jnz             .loop
    RET
%endif
%endmacro

FILTER_VER_LUMA_AVX2_32x24 pp
FILTER_VER_LUMA_AVX2_32x24 ps
FILTER_VER_LUMA_AVX2_32x24 sp
FILTER_VER_LUMA_AVX2_32x24 ss

%macro PROCESS_LUMA_AVX2_W8_4R 1
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m2, [r5]
    lea             r0, [r0 + r1 * 4]
    movu            xm4, [r0]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    paddd           m1, m5
    pmaddwd         m3, [r5]
    movu            xm5, [r0 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 2 * mmsize]
    paddd           m0, m6
    pmaddwd         m4, [r5 + 1 * mmsize]
    paddd           m2, m4
    movu            xm6, [r0 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm4, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm4, 1
    pmaddwd         m4, m5, [r5 + 2 * mmsize]
    paddd           m1, m4
    pmaddwd         m5, [r5 + 1 * mmsize]
    paddd           m3, m5
    movu            xm4, [r0 + r4]                  ; m4 = row 7
    punpckhwd       xm5, xm6, xm4
    punpcklwd       xm6, xm4
    vinserti128     m6, m6, xm5, 1
    pmaddwd         m5, m6, [r5 + 3 * mmsize]
    paddd           m0, m5
    pmaddwd         m6, [r5 + 2 * mmsize]
    paddd           m2, m6
    lea             r0, [r0 + r1 * 4]
    movu            xm5, [r0]                       ; m5 = row 8
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 3 * mmsize]
    paddd           m1, m6
    pmaddwd         m4, [r5 + 2 * mmsize]
    paddd           m3, m4
    movu            xm6, [r0 + r1]                  ; m6 = row 9
    punpckhwd       xm4, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm4, 1
    pmaddwd         m5, [r5 + 3 * mmsize]
    paddd           m2, m5
    movu            xm4, [r0 + r1 * 2]              ; m4 = row 10
    punpckhwd       xm5, xm6, xm4
    punpcklwd       xm6, xm4
    vinserti128     m6, m6, xm5, 1
    pmaddwd         m6, [r5 + 3 * mmsize]
    paddd           m3, m6

%ifidn %1,ss
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
%else
    paddd           m0, m7
    paddd           m1, m7
    paddd           m2, m7
    paddd           m3, m7
%ifidn %1,pp
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
%elifidn %1, sp
    psrad           m0, 10
    psrad           m1, 10
    psrad           m2, 10
    psrad           m3, 10
%else
    psrad           m0, 2
    psrad           m1, 2
    psrad           m2, 2
    psrad           m3, 2
%endif
%endif

    packssdw        m0, m1
    packssdw        m2, m3
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
    pxor            m4, m4
%ifidn %1,pp
    CLIPW           m0, m4, [pw_pixel_max]
    CLIPW           m2, m4, [pw_pixel_max]
%elifidn %1, sp
    CLIPW           m0, m4, [pw_pixel_max]
    CLIPW           m2, m4, [pw_pixel_max]
%endif

    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
%endmacro

%macro FILTER_VER_LUMA_AVX2_16x4 1
INIT_YMM avx2
cglobal interp_8tap_vert_%1_16x4, 4, 7, 8, 0 - gprsize
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    vbroadcasti128  m7, [pd_32]
%elifidn %1, sp
    mova            m7, [pd_524800]
%else
    vbroadcasti128  m7, [pd_n32768]
%endif
    mov             dword [rsp], 2
.loopW:
    PROCESS_LUMA_AVX2_W8_4R %1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    lea             r6, [r3 * 3]
    movu            [r2 + r6], xm3
    add             r2, 16
    lea             r6, [8 * r1 - 16]
    sub             r0, r6
    dec             dword [rsp]
    jnz             .loopW
    RET
%endmacro

FILTER_VER_LUMA_AVX2_16x4 pp
FILTER_VER_LUMA_AVX2_16x4 ps
FILTER_VER_LUMA_AVX2_16x4 sp
FILTER_VER_LUMA_AVX2_16x4 ss

%macro FILTER_VER_LUMA_AVX2_8x4 1
INIT_YMM avx2
cglobal interp_8tap_vert_%1_8x4, 4, 6, 8
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    vbroadcasti128  m7, [pd_32]
%elifidn %1, sp
    mova            m7, [pd_524800]
%else
    vbroadcasti128  m7, [pd_n32768]
%endif

    PROCESS_LUMA_AVX2_W8_4R %1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    lea             r4, [r3 * 3]
    movu            [r2 + r4], xm3
    RET
%endmacro

FILTER_VER_LUMA_AVX2_8x4 pp
FILTER_VER_LUMA_AVX2_8x4 ps
FILTER_VER_LUMA_AVX2_8x4 sp
FILTER_VER_LUMA_AVX2_8x4 ss

%macro FILTER_VER_LUMA_AVX2_16x12 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_16x12, 4, 10, 15
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    vbroadcasti128  m14, [pd_32]
%elifidn %1, sp
    mova            m14, [pd_524800]
%else
    vbroadcasti128  m14, [pd_n32768]
%endif
    mova            m13, [pw_pixel_max]
    pxor            m12, m12
    lea             r6, [r3 * 3]
    mov             r9d, 2
.loopW:
    movu            xm0, [r0]                       ; m0 = row 0
    movu            xm1, [r0 + r1]                  ; m1 = row 1
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m0, [r5]
    movu            xm2, [r0 + r1 * 2]              ; m2 = row 2
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m1, [r5]
    movu            xm3, [r0 + r4]                  ; m3 = row 3
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m4, m2, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m2, [r5]
    lea             r7, [r0 + r1 * 4]
    movu            xm4, [r7]                       ; m4 = row 4
    punpckhwd       xm5, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm5, 1
    pmaddwd         m5, m3, [r5 + 1 * mmsize]
    paddd           m1, m5
    pmaddwd         m3, [r5]
    movu            xm5, [r7 + r1]                  ; m5 = row 5
    punpckhwd       xm6, xm4, xm5
    punpcklwd       xm4, xm5
    vinserti128     m4, m4, xm6, 1
    pmaddwd         m6, m4, [r5 + 2 * mmsize]
    paddd           m0, m6
    pmaddwd         m6, m4, [r5 + 1 * mmsize]
    paddd           m2, m6
    pmaddwd         m4, [r5]
    movu            xm6, [r7 + r1 * 2]              ; m6 = row 6
    punpckhwd       xm7, xm5, xm6
    punpcklwd       xm5, xm6
    vinserti128     m5, m5, xm7, 1
    pmaddwd         m7, m5, [r5 + 2 * mmsize]
    paddd           m1, m7
    pmaddwd         m7, m5, [r5 + 1 * mmsize]
    paddd           m3, m7
    pmaddwd         m5, [r5]
    movu            xm7, [r7 + r4]                  ; m7 = row 7
    punpckhwd       xm8, xm6, xm7
    punpcklwd       xm6, xm7
    vinserti128     m6, m6, xm8, 1
    pmaddwd         m8, m6, [r5 + 3 * mmsize]
    paddd           m0, m8
    pmaddwd         m8, m6, [r5 + 2 * mmsize]
    paddd           m2, m8
    pmaddwd         m8, m6, [r5 + 1 * mmsize]
    paddd           m4, m8
    pmaddwd         m6, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm8, [r7]                       ; m8 = row 8
    punpckhwd       xm9, xm7, xm8
    punpcklwd       xm7, xm8
    vinserti128     m7, m7, xm9, 1
    pmaddwd         m9, m7, [r5 + 3 * mmsize]
    paddd           m1, m9
    pmaddwd         m9, m7, [r5 + 2 * mmsize]
    paddd           m3, m9
    pmaddwd         m9, m7, [r5 + 1 * mmsize]
    paddd           m5, m9
    pmaddwd         m7, [r5]
    movu            xm9, [r7 + r1]                  ; m9 = row 9
    punpckhwd       xm10, xm8, xm9
    punpcklwd       xm8, xm9
    vinserti128     m8, m8, xm10, 1
    pmaddwd         m10, m8, [r5 + 3 * mmsize]
    paddd           m2, m10
    pmaddwd         m10, m8, [r5 + 2 * mmsize]
    paddd           m4, m10
    pmaddwd         m10, m8, [r5 + 1 * mmsize]
    paddd           m6, m10
    pmaddwd         m8, [r5]
    movu            xm10, [r7 + r1 * 2]             ; m10 = row 10
    punpckhwd       xm11, xm9, xm10
    punpcklwd       xm9, xm10
    vinserti128     m9, m9, xm11, 1
    pmaddwd         m11, m9, [r5 + 3 * mmsize]
    paddd           m3, m11
    pmaddwd         m11, m9, [r5 + 2 * mmsize]
    paddd           m5, m11
    pmaddwd         m11, m9, [r5 + 1 * mmsize]
    paddd           m7, m11
    pmaddwd         m9, [r5]

%ifidn %1,ss
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
%else
    paddd           m0, m14
    paddd           m1, m14
    paddd           m2, m14
    paddd           m3, m14
%ifidn %1,pp
    psrad           m0, 6
    psrad           m1, 6
    psrad           m2, 6
    psrad           m3, 6
%elifidn %1, sp
    psrad           m0, 10
    psrad           m1, 10
    psrad           m2, 10
    psrad           m3, 10
%else
    psrad           m0, 2
    psrad           m1, 2
    psrad           m2, 2
    psrad           m3, 2
%endif
%endif

    packssdw        m0, m1
    packssdw        m2, m3
    vpermq          m0, m0, 11011000b
    vpermq          m2, m2, 11011000b
%ifidn %1,pp
    CLIPW           m0, m12, m13
    CLIPW           m2, m12, m13
%elifidn %1, sp
    CLIPW           m0, m12, m13
    CLIPW           m2, m12, m13
%endif

    vextracti128    xm1, m0, 1
    vextracti128    xm3, m2, 1
    movu            [r2], xm0
    movu            [r2 + r3], xm1
    movu            [r2 + r3 * 2], xm2
    movu            [r2 + r6], xm3

    movu            xm11, [r7 + r4]                 ; m11 = row 11
    punpckhwd       xm0, xm10, xm11
    punpcklwd       xm10, xm11
    vinserti128     m10, m10, xm0, 1
    pmaddwd         m0, m10, [r5 + 3 * mmsize]
    paddd           m4, m0
    pmaddwd         m0, m10, [r5 + 2 * mmsize]
    paddd           m6, m0
    pmaddwd         m0, m10, [r5 + 1 * mmsize]
    paddd           m8, m0
    pmaddwd         m10, [r5]
    lea             r7, [r7 + r1 * 4]
    movu            xm0, [r7]                      ; m0 = row 12
    punpckhwd       xm1, xm11, xm0
    punpcklwd       xm11, xm0
    vinserti128     m11, m11, xm1, 1
    pmaddwd         m1, m11, [r5 + 3 * mmsize]
    paddd           m5, m1
    pmaddwd         m1, m11, [r5 + 2 * mmsize]
    paddd           m7, m1
    pmaddwd         m1, m11, [r5 + 1 * mmsize]
    paddd           m9, m1
    pmaddwd         m11, [r5]
    movu            xm2, [r7 + r1]                 ; m2 = row 13
    punpckhwd       xm1, xm0, xm2
    punpcklwd       xm0, xm2
    vinserti128     m0, m0, xm1, 1
    pmaddwd         m1, m0, [r5 + 3 * mmsize]
    paddd           m6, m1
    pmaddwd         m1, m0, [r5 + 2 * mmsize]
    paddd           m8, m1
    pmaddwd         m0, [r5 + 1 * mmsize]
    paddd           m10, m0
    movu            xm0, [r7 + r1 * 2]              ; m0 = row 14
    punpckhwd       xm1, xm2, xm0
    punpcklwd       xm2, xm0
    vinserti128     m2, m2, xm1, 1
    pmaddwd         m1, m2, [r5 + 3 * mmsize]
    paddd           m7, m1
    pmaddwd         m1, m2, [r5 + 2 * mmsize]
    paddd           m9, m1
    pmaddwd         m2, [r5 + 1 * mmsize]
    paddd           m11, m2

%ifidn %1,ss
    psrad           m4, 6
    psrad           m5, 6
    psrad           m6, 6
    psrad           m7, 6
%else
    paddd           m4, m14
    paddd           m5, m14
    paddd           m6, m14
    paddd           m7, m14
%ifidn %1,pp
    psrad           m4, 6
    psrad           m5, 6
    psrad           m6, 6
    psrad           m7, 6
%elifidn %1, sp
    psrad           m4, 10
    psrad           m5, 10
    psrad           m6, 10
    psrad           m7, 10
%else
    psrad           m4, 2
    psrad           m5, 2
    psrad           m6, 2
    psrad           m7, 2
%endif
%endif

    packssdw        m4, m5
    packssdw        m6, m7
    vpermq          m4, m4, 11011000b
    vpermq          m6, m6, 11011000b
%ifidn %1,pp
    CLIPW           m4, m12, m13
    CLIPW           m6, m12, m13
%elifidn %1, sp
    CLIPW           m4, m12, m13
    CLIPW           m6, m12, m13
%endif
    lea             r8, [r2 + r3 * 4]
    vextracti128    xm1, m4, 1
    vextracti128    xm7, m6, 1
    movu            [r8], xm4
    movu            [r8 + r3], xm1
    movu            [r8 + r3 * 2], xm6
    movu            [r8 + r6], xm7

    movu            xm1, [r7 + r4]                  ; m1 = row 15
    punpckhwd       xm2, xm0, xm1
    punpcklwd       xm0, xm1
    vinserti128     m0, m0, xm2, 1
    pmaddwd         m2, m0, [r5 + 3 * mmsize]
    paddd           m8, m2
    pmaddwd         m0, [r5 + 2 * mmsize]
    paddd           m10, m0
    lea             r7, [r7 + r1 * 4]
    movu            xm2, [r7]                       ; m2 = row 16
    punpckhwd       xm3, xm1, xm2
    punpcklwd       xm1, xm2
    vinserti128     m1, m1, xm3, 1
    pmaddwd         m3, m1, [r5 + 3 * mmsize]
    paddd           m9, m3
    pmaddwd         m1, [r5 + 2 * mmsize]
    paddd           m11, m1
    movu            xm3, [r7 + r1]                  ; m3 = row 17
    punpckhwd       xm4, xm2, xm3
    punpcklwd       xm2, xm3
    vinserti128     m2, m2, xm4, 1
    pmaddwd         m2, [r5 + 3 * mmsize]
    paddd           m10, m2
    movu            xm4, [r7 + r1 * 2]              ; m4 = row 18
    punpckhwd       xm2, xm3, xm4
    punpcklwd       xm3, xm4
    vinserti128     m3, m3, xm2, 1
    pmaddwd         m3, [r5 + 3 * mmsize]
    paddd           m11, m3

%ifidn %1,ss
    psrad           m8, 6
    psrad           m9, 6
    psrad           m10, 6
    psrad           m11, 6
%else
    paddd           m8, m14
    paddd           m9, m14
    paddd           m10, m14
    paddd           m11, m14
%ifidn %1,pp
    psrad           m8, 6
    psrad           m9, 6
    psrad           m10, 6
    psrad           m11, 6
%elifidn %1, sp
    psrad           m8, 10
    psrad           m9, 10
    psrad           m10, 10
    psrad           m11, 10
%else
    psrad           m8, 2
    psrad           m9, 2
    psrad           m10, 2
    psrad           m11, 2
%endif
%endif

    packssdw        m8, m9
    packssdw        m10, m11
    vpermq          m8, m8, 11011000b
    vpermq          m10, m10, 11011000b
%ifidn %1,pp
    CLIPW           m8, m12, m13
    CLIPW           m10, m12, m13
%elifidn %1, sp
    CLIPW           m8, m12, m13
    CLIPW           m10, m12, m13
%endif
    vextracti128    xm9, m8, 1
    vextracti128    xm11, m10, 1
    lea             r8, [r8 + r3 * 4]
    movu            [r8], xm8
    movu            [r8 + r3], xm9
    movu            [r8 + r3 * 2], xm10
    movu            [r8 + r6], xm11
    add             r2, 16
    add             r0, 16
    dec             r9d
    jnz             .loopW
    RET
%endif
%endmacro

FILTER_VER_LUMA_AVX2_16x12 pp
FILTER_VER_LUMA_AVX2_16x12 ps
FILTER_VER_LUMA_AVX2_16x12 sp
FILTER_VER_LUMA_AVX2_16x12 ss

%macro FILTER_VER_LUMA_AVX2_4x8 1
INIT_YMM avx2
cglobal interp_8tap_vert_%1_4x8, 4, 7, 8
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4

%ifidn %1,pp
    vbroadcasti128  m7, [pd_32]
%elifidn %1, sp
    mova            m7, [pd_524800]
%else
    vbroadcasti128  m7, [pd_n32768]
%endif
    lea             r6, [r3 * 3]

    movq            xm0, [r0]
    movq            xm1, [r0 + r1]
    punpcklwd       xm0, xm1
    movq            xm2, [r0 + r1 * 2]
    punpcklwd       xm1, xm2
    vinserti128     m0, m0, xm1, 1                  ; m0 = [2 1 1 0]
    pmaddwd         m0, [r5]
    movq            xm3, [r0 + r4]
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm4, [r0]
    punpcklwd       xm3, xm4
    vinserti128     m2, m2, xm3, 1                  ; m2 = [4 3 3 2]
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    pmaddwd         m2, [r5]
    paddd           m0, m5
    movq            xm3, [r0 + r1]
    punpcklwd       xm4, xm3
    movq            xm1, [r0 + r1 * 2]
    punpcklwd       xm3, xm1
    vinserti128     m4, m4, xm3, 1                  ; m4 = [6 5 5 4]
    pmaddwd         m5, m4, [r5 + 2 * mmsize]
    paddd           m0, m5
    pmaddwd         m5, m4, [r5 + 1 * mmsize]
    paddd           m2, m5
    pmaddwd         m4, [r5]
    movq            xm3, [r0 + r4]
    punpcklwd       xm1, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm6, [r0]
    punpcklwd       xm3, xm6
    vinserti128     m1, m1, xm3, 1                  ; m1 = [8 7 7 6]
    pmaddwd         m5, m1, [r5 + 3 * mmsize]
    paddd           m0, m5
    pmaddwd         m5, m1, [r5 + 2 * mmsize]
    paddd           m2, m5
    pmaddwd         m5, m1, [r5 + 1 * mmsize]
    paddd           m4, m5
    pmaddwd         m1, [r5]
    movq            xm3, [r0 + r1]
    punpcklwd       xm6, xm3
    movq            xm5, [r0 + 2 * r1]
    punpcklwd       xm3, xm5
    vinserti128     m6, m6, xm3, 1                  ; m6 = [A 9 9 8]
    pmaddwd         m3, m6, [r5 + 3 * mmsize]
    paddd           m2, m3
    pmaddwd         m3, m6, [r5 + 2 * mmsize]
    paddd           m4, m3
    pmaddwd         m6, [r5 + 1 * mmsize]
    paddd           m1, m6

%ifidn %1,ss
    psrad           m0, 6
    psrad           m2, 6
%else
    paddd           m0, m7
    paddd           m2, m7
%ifidn %1,pp
    psrad           m0, 6
    psrad           m2, 6
%elifidn %1, sp
    psrad           m0, 10
    psrad           m2, 10
%else
    psrad           m0, 2
    psrad           m2, 2
%endif
%endif

    packssdw        m0, m2
    pxor            m6, m6
    mova            m3, [pw_pixel_max]
%ifidn %1,pp
    CLIPW           m0, m6, m3
%elifidn %1, sp
    CLIPW           m0, m6, m3
%endif

    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r6], xm2

    movq            xm2, [r0 + r4]
    punpcklwd       xm5, xm2
    lea             r0, [r0 + 4 * r1]
    movq            xm0, [r0]
    punpcklwd       xm2, xm0
    vinserti128     m5, m5, xm2, 1                  ; m5 = [C B B A]
    pmaddwd         m2, m5, [r5 + 3 * mmsize]
    paddd           m4, m2
    pmaddwd         m5, [r5 + 2 * mmsize]
    paddd           m1, m5
    movq            xm2, [r0 + r1]
    punpcklwd       xm0, xm2
    movq            xm5, [r0 + 2 * r1]
    punpcklwd       xm2, xm5
    vinserti128     m0, m0, xm2, 1                  ; m0 = [E D D C]
    pmaddwd         m0, [r5 + 3 * mmsize]
    paddd           m1, m0

%ifidn %1,ss
    psrad           m4, 6
    psrad           m1, 6
%else
    paddd           m4, m7
    paddd           m1, m7
%ifidn %1,pp
    psrad           m4, 6
    psrad           m1, 6
%elifidn %1, sp
    psrad           m4, 10
    psrad           m1, 10
%else
    psrad           m4, 2
    psrad           m1, 2
%endif
%endif

    packssdw        m4, m1
%ifidn %1,pp
    CLIPW           m4, m6, m3
%elifidn %1, sp
    CLIPW           m4, m6, m3
%endif

    vextracti128    xm1, m4, 1
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm4
    movq            [r2 + r3], xm1
    movhps          [r2 + r3 * 2], xm4
    movhps          [r2 + r6], xm1
    RET
%endmacro

FILTER_VER_LUMA_AVX2_4x8 pp
FILTER_VER_LUMA_AVX2_4x8 ps
FILTER_VER_LUMA_AVX2_4x8 sp
FILTER_VER_LUMA_AVX2_4x8 ss

%macro PROCESS_LUMA_AVX2_W4_16R 1
    movq            xm0, [r0]
    movq            xm1, [r0 + r1]
    punpcklwd       xm0, xm1
    movq            xm2, [r0 + r1 * 2]
    punpcklwd       xm1, xm2
    vinserti128     m0, m0, xm1, 1                  ; m0 = [2 1 1 0]
    pmaddwd         m0, [r5]
    movq            xm3, [r0 + r4]
    punpcklwd       xm2, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm4, [r0]
    punpcklwd       xm3, xm4
    vinserti128     m2, m2, xm3, 1                  ; m2 = [4 3 3 2]
    pmaddwd         m5, m2, [r5 + 1 * mmsize]
    pmaddwd         m2, [r5]
    paddd           m0, m5
    movq            xm3, [r0 + r1]
    punpcklwd       xm4, xm3
    movq            xm1, [r0 + r1 * 2]
    punpcklwd       xm3, xm1
    vinserti128     m4, m4, xm3, 1                  ; m4 = [6 5 5 4]
    pmaddwd         m5, m4, [r5 + 2 * mmsize]
    paddd           m0, m5
    pmaddwd         m5, m4, [r5 + 1 * mmsize]
    paddd           m2, m5
    pmaddwd         m4, [r5]
    movq            xm3, [r0 + r4]
    punpcklwd       xm1, xm3
    lea             r0, [r0 + 4 * r1]
    movq            xm6, [r0]
    punpcklwd       xm3, xm6
    vinserti128     m1, m1, xm3, 1                  ; m1 = [8 7 7 6]
    pmaddwd         m5, m1, [r5 + 3 * mmsize]
    paddd           m0, m5
    pmaddwd         m5, m1, [r5 + 2 * mmsize]
    paddd           m2, m5
    pmaddwd         m5, m1, [r5 + 1 * mmsize]
    paddd           m4, m5
    pmaddwd         m1, [r5]
    movq            xm3, [r0 + r1]
    punpcklwd       xm6, xm3
    movq            xm5, [r0 + 2 * r1]
    punpcklwd       xm3, xm5
    vinserti128     m6, m6, xm3, 1                  ; m6 = [10 9 9 8]
    pmaddwd         m3, m6, [r5 + 3 * mmsize]
    paddd           m2, m3
    pmaddwd         m3, m6, [r5 + 2 * mmsize]
    paddd           m4, m3
    pmaddwd         m3, m6, [r5 + 1 * mmsize]
    paddd           m1, m3
    pmaddwd         m6, [r5]

%ifidn %1,ss
    psrad           m0, 6
    psrad           m2, 6
%else
    paddd           m0, m7
    paddd           m2, m7
%ifidn %1,pp
    psrad           m0, 6
    psrad           m2, 6
%elifidn %1, sp
    psrad           m0, 10
    psrad           m2, 10
%else
    psrad           m0, 2
    psrad           m2, 2
%endif
%endif

    packssdw        m0, m2
    pxor            m3, m3
%ifidn %1,pp
    CLIPW           m0, m3, [pw_pixel_max]
%elifidn %1, sp
    CLIPW           m0, m3, [pw_pixel_max]
%endif

    vextracti128    xm2, m0, 1
    movq            [r2], xm0
    movq            [r2 + r3], xm2
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r6], xm2

    movq            xm2, [r0 + r4]
    punpcklwd       xm5, xm2
    lea             r0, [r0 + 4 * r1]
    movq            xm0, [r0]
    punpcklwd       xm2, xm0
    vinserti128     m5, m5, xm2, 1                  ; m5 = [12 11 11 10]
    pmaddwd         m2, m5, [r5 + 3 * mmsize]
    paddd           m4, m2
    pmaddwd         m2, m5, [r5 + 2 * mmsize]
    paddd           m1, m2
    pmaddwd         m2, m5, [r5 + 1 * mmsize]
    paddd           m6, m2
    pmaddwd         m5, [r5]
    movq            xm2, [r0 + r1]
    punpcklwd       xm0, xm2
    movq            xm3, [r0 + 2 * r1]
    punpcklwd       xm2, xm3
    vinserti128     m0, m0, xm2, 1                  ; m0 = [14 13 13 12]
    pmaddwd         m2, m0, [r5 + 3 * mmsize]
    paddd           m1, m2
    pmaddwd         m2, m0, [r5 + 2 * mmsize]
    paddd           m6, m2
    pmaddwd         m2, m0, [r5 + 1 * mmsize]
    paddd           m5, m2
    pmaddwd         m0, [r5]

%ifidn %1,ss
    psrad           m4, 6
    psrad           m1, 6
%else
    paddd           m4, m7
    paddd           m1, m7
%ifidn %1,pp
    psrad           m4, 6
    psrad           m1, 6
%elifidn %1, sp
    psrad           m4, 10
    psrad           m1, 10
%else
    psrad           m4, 2
    psrad           m1, 2
%endif
%endif

    packssdw        m4, m1
    pxor            m2, m2
%ifidn %1,pp
    CLIPW           m4, m2, [pw_pixel_max]
%elifidn %1, sp
    CLIPW           m4, m2, [pw_pixel_max]
%endif

    vextracti128    xm1, m4, 1
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm4
    movq            [r2 + r3], xm1
    movhps          [r2 + r3 * 2], xm4
    movhps          [r2 + r6], xm1

    movq            xm4, [r0 + r4]
    punpcklwd       xm3, xm4
    lea             r0, [r0 + 4 * r1]
    movq            xm1, [r0]
    punpcklwd       xm4, xm1
    vinserti128     m3, m3, xm4, 1                  ; m3 = [16 15 15 14]
    pmaddwd         m4, m3, [r5 + 3 * mmsize]
    paddd           m6, m4
    pmaddwd         m4, m3, [r5 + 2 * mmsize]
    paddd           m5, m4
    pmaddwd         m4, m3, [r5 + 1 * mmsize]
    paddd           m0, m4
    pmaddwd         m3, [r5]
    movq            xm4, [r0 + r1]
    punpcklwd       xm1, xm4
    movq            xm2, [r0 + 2 * r1]
    punpcklwd       xm4, xm2
    vinserti128     m1, m1, xm4, 1                  ; m1 = [18 17 17 16]
    pmaddwd         m4, m1, [r5 + 3 * mmsize]
    paddd           m5, m4
    pmaddwd         m4, m1, [r5 + 2 * mmsize]
    paddd           m0, m4
    pmaddwd         m1, [r5 + 1 * mmsize]
    paddd           m3, m1

%ifidn %1,ss
    psrad           m6, 6
    psrad           m5, 6
%else
    paddd           m6, m7
    paddd           m5, m7
%ifidn %1,pp
    psrad           m6, 6
    psrad           m5, 6
%elifidn %1, sp
    psrad           m6, 10
    psrad           m5, 10
%else
    psrad           m6, 2
    psrad           m5, 2
%endif
%endif

    packssdw        m6, m5
    pxor            m1, m1
%ifidn %1,pp
    CLIPW           m6, m1, [pw_pixel_max]
%elifidn %1, sp
    CLIPW           m6, m1, [pw_pixel_max]
%endif

    vextracti128    xm5, m6, 1
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm6
    movq            [r2 + r3], xm5
    movhps          [r2 + r3 * 2], xm6
    movhps          [r2 + r6], xm5

    movq            xm4, [r0 + r4]
    punpcklwd       xm2, xm4
    lea             r0, [r0 + 4 * r1]
    movq            xm6, [r0]
    punpcklwd       xm4, xm6
    vinserti128     m2, m2, xm4, 1                  ; m2 = [20 19 19 18]
    pmaddwd         m4, m2, [r5 + 3 * mmsize]
    paddd           m0, m4
    pmaddwd         m2, [r5 + 2 * mmsize]
    paddd           m3, m2
    movq            xm4, [r0 + r1]
    punpcklwd       xm6, xm4
    movq            xm2, [r0 + 2 * r1]
    punpcklwd       xm4, xm2
    vinserti128     m6, m6, xm4, 1                  ; m6 = [22 21 21 20]
    pmaddwd         m6, [r5 + 3 * mmsize]
    paddd           m3, m6

%ifidn %1,ss
    psrad           m0, 6
    psrad           m3, 6
%else
    paddd           m0, m7
    paddd           m3, m7
%ifidn %1,pp
    psrad           m0, 6
    psrad           m3, 6
%elifidn %1, sp
    psrad           m0, 10
    psrad           m3, 10
%else
    psrad           m0, 2
    psrad           m3, 2
%endif
%endif
  
    packssdw        m0, m3
%ifidn %1,pp
    CLIPW           m0, m1, [pw_pixel_max]
%elifidn %1, sp
    CLIPW           m0, m1, [pw_pixel_max]
%endif

    vextracti128    xm3, m0, 1
    lea             r2, [r2 + r3 * 4]
    movq            [r2], xm0
    movq            [r2 + r3], xm3
    movhps          [r2 + r3 * 2], xm0
    movhps          [r2 + r6], xm3
%endmacro

%macro FILTER_VER_LUMA_AVX2_4x16 1
INIT_YMM avx2
cglobal interp_8tap_vert_%1_4x16, 4, 7, 8
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    vbroadcasti128  m7, [pd_32]
%elifidn %1, sp
    mova            m7, [pd_524800]
%else
    vbroadcasti128  m7, [pd_n32768]
%endif
    lea             r6, [r3 * 3]
    PROCESS_LUMA_AVX2_W4_16R %1
    RET
%endmacro

FILTER_VER_LUMA_AVX2_4x16 pp
FILTER_VER_LUMA_AVX2_4x16 ps
FILTER_VER_LUMA_AVX2_4x16 sp
FILTER_VER_LUMA_AVX2_4x16 ss

%macro FILTER_VER_LUMA_AVX2_12x16 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_vert_%1_12x16, 4, 9, 15
    mov             r4d, r4m
    shl             r4d, 7
    add             r1d, r1d
    add             r3d, r3d

%ifdef PIC
    lea             r5, [tab_LumaCoeffVer]
    add             r5, r4
%else
    lea             r5, [tab_LumaCoeffVer + r4]
%endif

    lea             r4, [r1 * 3]
    sub             r0, r4
%ifidn %1,pp
    vbroadcasti128  m14, [pd_32]
%elifidn %1, sp
    mova            m14, [pd_524800]
%else
    vbroadcasti128  m14, [pd_n32768]
%endif
    lea             r6, [r3 * 3]
    PROCESS_LUMA_AVX2_W8_16R %1
    add             r2, 16
    add             r0, 16
    mova            m7, m14
    PROCESS_LUMA_AVX2_W4_16R %1
    RET
%endif
%endmacro

FILTER_VER_LUMA_AVX2_12x16 pp
FILTER_VER_LUMA_AVX2_12x16 ps
FILTER_VER_LUMA_AVX2_12x16 sp
FILTER_VER_LUMA_AVX2_12x16 ss

;---------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_%1x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_PS 2
INIT_XMM sse4
cglobal interp_8tap_vert_ps_%1x%2, 5, 7, 8 ,0-gprsize

    add       r1d, r1d
    add       r3d, r3d
    lea       r5, [r1 + 2 * r1]
    sub       r0, r5
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_LumaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_LumaCoeffV + r4]
%endif

    mova      m7, [pd_n32768]

    mov       dword [rsp], %2/4
.loopH:
    mov       r4d, (%1/4)
.loopW:
    PROCESS_LUMA_VER_W4_4R

    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7

    psrad     m0, 2
    psrad     m1, 2
    psrad     m2, 2
    psrad     m3, 2

    packssdw  m0, m1
    packssdw  m2, m3

    movh      [r2], m0
    movhps    [r2 + r3], m0
    lea       r5, [r2 + 2 * r3]
    movh      [r5], m2
    movhps    [r5 + r3], m2

    lea       r5, [8 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 2 * 4

    dec       r4d
    jnz       .loopW

    lea       r0, [r0 + 4 * r1 - 2 * %1]
    lea       r2, [r2 + 4 * r3 - 2 * %1]

    dec       dword [rsp]
    jnz       .loopH

    RET
%endmacro

;---------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_%1x%2(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;---------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_PS 4, 4
    FILTER_VER_LUMA_PS 8, 8
    FILTER_VER_LUMA_PS 8, 4
    FILTER_VER_LUMA_PS 4, 8
    FILTER_VER_LUMA_PS 16, 16
    FILTER_VER_LUMA_PS 16, 8
    FILTER_VER_LUMA_PS 8, 16
    FILTER_VER_LUMA_PS 16, 12
    FILTER_VER_LUMA_PS 12, 16
    FILTER_VER_LUMA_PS 16, 4
    FILTER_VER_LUMA_PS 4, 16
    FILTER_VER_LUMA_PS 32, 32
    FILTER_VER_LUMA_PS 32, 16
    FILTER_VER_LUMA_PS 16, 32
    FILTER_VER_LUMA_PS 32, 24
    FILTER_VER_LUMA_PS 24, 32
    FILTER_VER_LUMA_PS 32, 8
    FILTER_VER_LUMA_PS 8, 32
    FILTER_VER_LUMA_PS 64, 64
    FILTER_VER_LUMA_PS 64, 32
    FILTER_VER_LUMA_PS 32, 64
    FILTER_VER_LUMA_PS 64, 48
    FILTER_VER_LUMA_PS 48, 64
    FILTER_VER_LUMA_PS 64, 16
    FILTER_VER_LUMA_PS 16, 64

;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_sp_%1x%2(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_SP 2
INIT_XMM sse4
cglobal interp_8tap_vert_sp_%1x%2, 5, 7, 8 ,0-gprsize

    add       r1d, r1d
    add       r3d, r3d
    lea       r5, [r1 + 2 * r1]
    sub       r0, r5
    shl       r4d, 6

%ifdef PIC
    lea       r5, [tab_LumaCoeffV]
    lea       r6, [r5 + r4]
%else
    lea       r6, [tab_LumaCoeffV + r4]
%endif

    mova      m7, [tab_c_524800]

    mov       dword [rsp], %2/4
.loopH:
    mov       r4d, (%1/4)
.loopW:
    PROCESS_LUMA_VER_W4_4R

    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7

    psrad     m0, 10
    psrad     m1, 10
    psrad     m2, 10
    psrad     m3, 10

    packssdw  m0, m1
    packssdw  m2, m3

    pxor      m1, m1
    CLIPW2    m0, m2, m1, [pw_pixel_max]

    movh      [r2], m0
    movhps    [r2 + r3], m0
    lea       r5, [r2 + 2 * r3]
    movh      [r5], m2
    movhps    [r5 + r3], m2

    lea       r5, [8 * r1 - 2 * 4]
    sub       r0, r5
    add       r2, 2 * 4

    dec       r4d
    jnz       .loopW

    lea       r0, [r0 + 4 * r1 - 2 * %1]
    lea       r2, [r2 + 4 * r3 - 2 * %1]

    dec       dword [rsp]
    jnz       .loopH

    RET
%endmacro

;--------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_sp_%1x%2(int16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_SP 4, 4
    FILTER_VER_LUMA_SP 8, 8
    FILTER_VER_LUMA_SP 8, 4
    FILTER_VER_LUMA_SP 4, 8
    FILTER_VER_LUMA_SP 16, 16
    FILTER_VER_LUMA_SP 16, 8
    FILTER_VER_LUMA_SP 8, 16
    FILTER_VER_LUMA_SP 16, 12
    FILTER_VER_LUMA_SP 12, 16
    FILTER_VER_LUMA_SP 16, 4
    FILTER_VER_LUMA_SP 4, 16
    FILTER_VER_LUMA_SP 32, 32
    FILTER_VER_LUMA_SP 32, 16
    FILTER_VER_LUMA_SP 16, 32
    FILTER_VER_LUMA_SP 32, 24
    FILTER_VER_LUMA_SP 24, 32
    FILTER_VER_LUMA_SP 32, 8
    FILTER_VER_LUMA_SP 8, 32
    FILTER_VER_LUMA_SP 64, 64
    FILTER_VER_LUMA_SP 64, 32
    FILTER_VER_LUMA_SP 32, 64
    FILTER_VER_LUMA_SP 64, 48
    FILTER_VER_LUMA_SP 48, 64
    FILTER_VER_LUMA_SP 64, 16
    FILTER_VER_LUMA_SP 16, 64

;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ss_%1x%2(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_SS 2
INIT_XMM sse2
cglobal interp_8tap_vert_ss_%1x%2, 5, 7, 7 ,0-gprsize

    add        r1d, r1d
    add        r3d, r3d
    lea        r5, [3 * r1]
    sub        r0, r5
    shl        r4d, 6

%ifdef PIC
    lea        r5, [tab_LumaCoeffV]
    lea        r6, [r5 + r4]
%else
    lea        r6, [tab_LumaCoeffV + r4]
%endif

    mov        dword [rsp], %2/4
.loopH:
    mov        r4d, (%1/4)
.loopW:
    PROCESS_LUMA_VER_W4_4R

    psrad      m0, 6
    psrad      m1, 6
    packssdw   m0, m1
    movlps     [r2], m0
    movhps     [r2 + r3], m0

    psrad      m2, 6
    psrad      m3, 6
    packssdw   m2, m3
    movlps     [r2 + 2 * r3], m2
    lea        r5, [3 * r3]
    movhps     [r2 + r5], m2

    lea        r5, [8 * r1 - 2 * 4]
    sub        r0, r5
    add        r2, 2 * 4

    dec        r4d
    jnz        .loopW

    lea        r0, [r0 + 4 * r1 - 2 * %1]
    lea        r2, [r2 + 4 * r3 - 2 * %1]

    dec        dword [rsp]
    jnz        .loopH

    RET
%endmacro

    FILTER_VER_LUMA_SS 4, 4
    FILTER_VER_LUMA_SS 8, 8
    FILTER_VER_LUMA_SS 8, 4
    FILTER_VER_LUMA_SS 4, 8
    FILTER_VER_LUMA_SS 16, 16
    FILTER_VER_LUMA_SS 16, 8
    FILTER_VER_LUMA_SS 8, 16
    FILTER_VER_LUMA_SS 16, 12
    FILTER_VER_LUMA_SS 12, 16
    FILTER_VER_LUMA_SS 16, 4
    FILTER_VER_LUMA_SS 4, 16
    FILTER_VER_LUMA_SS 32, 32
    FILTER_VER_LUMA_SS 32, 16
    FILTER_VER_LUMA_SS 16, 32
    FILTER_VER_LUMA_SS 32, 24
    FILTER_VER_LUMA_SS 24, 32
    FILTER_VER_LUMA_SS 32, 8
    FILTER_VER_LUMA_SS 8, 32
    FILTER_VER_LUMA_SS 64, 64
    FILTER_VER_LUMA_SS 64, 32
    FILTER_VER_LUMA_SS 32, 64
    FILTER_VER_LUMA_SS 64, 48
    FILTER_VER_LUMA_SS 48, 64
    FILTER_VER_LUMA_SS 64, 16
    FILTER_VER_LUMA_SS 16, 64

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_4xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_4x%1, 3, 6, 2
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    mova       m1, [pw_2000]

%rep %1/4
    movh       m0, [r0]
    movhps     m0, [r0 + r1]
    psllw      m0, 4
    psubw      m0, m1
    movh       [r2 + r3 * 0], m0
    movhps     [r2 + r3 * 1], m0

    movh       m0, [r0 + r1 * 2]
    movhps     m0, [r0 + r5]
    psllw      m0, 4
    psubw      m0, m1
    movh       [r2 + r3 * 2], m0
    movhps     [r2 + r4], m0

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    RET
%endmacro
P2S_H_4xN 4
P2S_H_4xN 8
P2S_H_4xN 16
P2S_H_4xN 32

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal filterPixelToShort_4x2, 3, 4, 1
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d

    movh       m0, [r0]
    movhps     m0, [r0 + r1]
    psllw      m0, 4
    psubw      m0, [pw_2000]
    movh       [r2 + r3 * 0], m0
    movhps     [r2 + r3 * 1], m0

    RET

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_6xN 1
INIT_XMM sse4
cglobal filterPixelToShort_6x%1, 3, 7, 3
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load height
    mov        r6d, %1/4

    ; load constant
    mova       m2, [pw_2000]

.loop
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    psllw      m0, 4
    psubw      m0, m2
    psllw      m1, 4
    psubw      m1, m2

    movh       [r2 + r3 * 0], m0
    pextrd     [r2 + r3 * 0 + 8], m0, 2
    movh       [r2 + r3 * 1], m1
    pextrd     [r2 + r3 * 1 + 8], m1, 2

    movu       m0, [r0 + r1 * 2]
    movu       m1, [r0 + r5]
    psllw      m0, 4
    psubw      m0, m2
    psllw      m1, 4
    psubw      m1, m2

    movh       [r2 + r3 * 2], m0
    pextrd     [r2 + r3 * 2 + 8], m0, 2
    movh       [r2 + r4], m1
    pextrd     [r2 + r4 + 8], m1, 2

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET
%endmacro
P2S_H_6xN 8
P2S_H_6xN 16

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_8xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_8x%1, 3, 7, 2
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load height
    mov        r6d, %1/4

    ; load constant
    mova       m1, [pw_2000]

.loop
    movu       m0, [r0]
    psllw      m0, 4
    psubw      m0, m1
    movu       [r2 + r3 * 0], m0

    movu       m0, [r0 + r1]
    psllw      m0, 4
    psubw      m0, m1
    movu       [r2 + r3 * 1], m0

    movu       m0, [r0 + r1 * 2]
    psllw      m0, 4
    psubw      m0, m1
    movu       [r2 + r3 * 2], m0

    movu       m0, [r0 + r5]
    psllw      m0, 4
    psubw      m0, m1
    movu       [r2 + r4], m0

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET
%endmacro
P2S_H_8xN 8
P2S_H_8xN 4
P2S_H_8xN 16
P2S_H_8xN 32
P2S_H_8xN 12
P2S_H_8xN 64

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal filterPixelToShort_8x2, 3, 4, 2
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d

    movu       m0, [r0]
    movu       m1, [r0 + r1]

    psllw      m0, 4
    psubw      m0, [pw_2000]
    psllw      m1, 4
    psubw      m1, [pw_2000]

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1

    RET

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal filterPixelToShort_8x6, 3, 7, 4
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r1 * 3]
    lea        r5, [r1 * 5]
    lea        r6, [r3 * 3]

    ; load constant
    mova       m3, [pw_2000]

    movu       m0, [r0]
    movu       m1, [r0 + r1]
    movu       m2, [r0 + r1 * 2]

    psllw      m0, 4
    psubw      m0, m3
    psllw      m1, 4
    psubw      m1, m3
    psllw      m2, 4
    psubw      m2, m3

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1
    movu       [r2 + r3 * 2], m2

    movu       m0, [r0 + r4]
    movu       m1, [r0 + r1 * 4]
    movu       m2, [r0 + r5 ]

    psllw      m0, 4
    psubw      m0, m3
    psllw      m1, 4
    psubw      m1, m3
    psllw      m2, 4
    psubw      m2, m3

    movu       [r2 + r6], m0
    movu       [r2 + r3 * 4], m1
    lea        r2, [r2 + r3 * 4]
    movu       [r2 + r3], m2

    RET

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_16xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_16x%1, 3, 7, 3
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load height
    mov        r6d, %1/4

    ; load constant
    mova       m2, [pw_2000]

.loop
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    psllw      m0, 4
    psubw      m0, m2
    psllw      m1, 4
    psubw      m1, m2

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1

    movu       m0, [r0 + r1 * 2]
    movu       m1, [r0 + r5]
    psllw      m0, 4
    psubw      m0, m2
    psllw      m1, 4
    psubw      m1, m2

    movu       [r2 + r3 * 2], m0
    movu       [r2 + r4], m1

    movu       m0, [r0 + 16]
    movu       m1, [r0 + r1 + 16]
    psllw      m0, 4
    psubw      m0, m2
    psllw      m1, 4
    psubw      m1, m2

    movu       [r2 + r3 * 0 + 16], m0
    movu       [r2 + r3 * 1 + 16], m1

    movu       m0, [r0 + r1 * 2 + 16]
    movu       m1, [r0 + r5 + 16]
    psllw      m0, 4
    psubw      m0, m2
    psllw      m1, 4
    psubw      m1, m2

    movu       [r2 + r3 * 2 + 16], m0
    movu       [r2 + r4 + 16], m1

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET
%endmacro
P2S_H_16xN 16
P2S_H_16xN 4
P2S_H_16xN 8
P2S_H_16xN 12
P2S_H_16xN 32
P2S_H_16xN 64
P2S_H_16xN 24

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_32xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_32x%1, 3, 7, 5
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load height
    mov        r6d, %1/4

    ; load constant
    mova       m4, [pw_2000]

.loop
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    movu       m2, [r0 + r1 * 2]
    movu       m3, [r0 + r5]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1
    movu       [r2 + r3 * 2], m2
    movu       [r2 + r4], m3

    movu       m0, [r0 + 16]
    movu       m1, [r0 + r1 + 16]
    movu       m2, [r0 + r1 * 2 + 16]
    movu       m3, [r0 + r5 + 16]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 16], m0
    movu       [r2 + r3 * 1 + 16], m1
    movu       [r2 + r3 * 2 + 16], m2
    movu       [r2 + r4 + 16], m3

    movu       m0, [r0 + 32]
    movu       m1, [r0 + r1 + 32]
    movu       m2, [r0 + r1 * 2 + 32]
    movu       m3, [r0 + r5 + 32]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 32], m0
    movu       [r2 + r3 * 1 + 32], m1
    movu       [r2 + r3 * 2 + 32], m2
    movu       [r2 + r4 + 32], m3

    movu       m0, [r0 + 48]
    movu       m1, [r0 + r1 + 48]
    movu       m2, [r0 + r1 * 2 + 48]
    movu       m3, [r0 + r5 + 48]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 48], m0
    movu       [r2 + r3 * 1 + 48], m1
    movu       [r2 + r3 * 2 + 48], m2
    movu       [r2 + r4 + 48], m3

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET
%endmacro
P2S_H_32xN 32
P2S_H_32xN 8
P2S_H_32xN 16
P2S_H_32xN 24
P2S_H_32xN 64
P2S_H_32xN 48

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_64xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_64x%1, 3, 7, 5
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load height
    mov        r6d, %1/4

    ; load constant
    mova       m4, [pw_2000]

.loop
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    movu       m2, [r0 + r1 * 2]
    movu       m3, [r0 + r5]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1
    movu       [r2 + r3 * 2], m2
    movu       [r2 + r4], m3

    movu       m0, [r0 + 16]
    movu       m1, [r0 + r1 + 16]
    movu       m2, [r0 + r1 * 2 + 16]
    movu       m3, [r0 + r5 + 16]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 16], m0
    movu       [r2 + r3 * 1 + 16], m1
    movu       [r2 + r3 * 2 + 16], m2
    movu       [r2 + r4 + 16], m3

    movu       m0, [r0 + 32]
    movu       m1, [r0 + r1 + 32]
    movu       m2, [r0 + r1 * 2 + 32]
    movu       m3, [r0 + r5 + 32]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 32], m0
    movu       [r2 + r3 * 1 + 32], m1
    movu       [r2 + r3 * 2 + 32], m2
    movu       [r2 + r4 + 32], m3

    movu       m0, [r0 + 48]
    movu       m1, [r0 + r1 + 48]
    movu       m2, [r0 + r1 * 2 + 48]
    movu       m3, [r0 + r5 + 48]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 48], m0
    movu       [r2 + r3 * 1 + 48], m1
    movu       [r2 + r3 * 2 + 48], m2
    movu       [r2 + r4 + 48], m3

    movu       m0, [r0 + 64]
    movu       m1, [r0 + r1 + 64]
    movu       m2, [r0 + r1 * 2 + 64]
    movu       m3, [r0 + r5 + 64]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 64], m0
    movu       [r2 + r3 * 1 + 64], m1
    movu       [r2 + r3 * 2 + 64], m2
    movu       [r2 + r4 + 64], m3

    movu       m0, [r0 + 80]
    movu       m1, [r0 + r1 + 80]
    movu       m2, [r0 + r1 * 2 + 80]
    movu       m3, [r0 + r5 + 80]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 80], m0
    movu       [r2 + r3 * 1 + 80], m1
    movu       [r2 + r3 * 2 + 80], m2
    movu       [r2 + r4 + 80], m3

    movu       m0, [r0 + 96]
    movu       m1, [r0 + r1 + 96]
    movu       m2, [r0 + r1 * 2 + 96]
    movu       m3, [r0 + r5 + 96]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 96], m0
    movu       [r2 + r3 * 1 + 96], m1
    movu       [r2 + r3 * 2 + 96], m2
    movu       [r2 + r4 + 96], m3

    movu       m0, [r0 + 112]
    movu       m1, [r0 + r1 + 112]
    movu       m2, [r0 + r1 * 2 + 112]
    movu       m3, [r0 + r5 + 112]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 112], m0
    movu       [r2 + r3 * 1 + 112], m1
    movu       [r2 + r3 * 2 + 112], m2
    movu       [r2 + r4 + 112], m3

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET
%endmacro
P2S_H_64xN 64
P2S_H_64xN 16
P2S_H_64xN 32
P2S_H_64xN 48

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_24xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_24x%1, 3, 7, 5
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load height
    mov        r6d, %1/4

    ; load constant
    mova       m4, [pw_2000]

.loop
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    movu       m2, [r0 + r1 * 2]
    movu       m3, [r0 + r5]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1
    movu       [r2 + r3 * 2], m2
    movu       [r2 + r4], m3

    movu       m0, [r0 + 16]
    movu       m1, [r0 + r1 + 16]
    movu       m2, [r0 + r1 * 2 + 16]
    movu       m3, [r0 + r5 + 16]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 16], m0
    movu       [r2 + r3 * 1 + 16], m1
    movu       [r2 + r3 * 2 + 16], m2
    movu       [r2 + r4 + 16], m3

    movu       m0, [r0 + 32]
    movu       m1, [r0 + r1 + 32]
    movu       m2, [r0 + r1 * 2 + 32]
    movu       m3, [r0 + r5 + 32]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 32], m0
    movu       [r2 + r3 * 1 + 32], m1
    movu       [r2 + r3 * 2 + 32], m2
    movu       [r2 + r4 + 32], m3

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET
%endmacro
P2S_H_24xN 32
P2S_H_24xN 64

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
%macro P2S_H_12xN 1
INIT_XMM ssse3
cglobal filterPixelToShort_12x%1, 3, 7, 3
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load height
    mov        r6d, %1/4

    ; load constant
    mova       m2, [pw_2000]

.loop
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    psllw      m0, 4
    psubw      m0, m2
    psllw      m1, 4
    psubw      m1, m2

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1

    movu       m0, [r0 + r1 * 2]
    movu       m1, [r0 + r5]
    psllw      m0, 4
    psubw      m0, m2
    psllw      m1, 4
    psubw      m1, m2

    movu       [r2 + r3 * 2], m0
    movu       [r2 + r4], m1

    movh       m0, [r0 + 16]
    movhps     m0, [r0 + r1 + 16]
    psllw      m0, 4
    psubw      m0, m2

    movh       [r2 + r3 * 0 + 16], m0
    movhps     [r2 + r3 * 1 + 16], m0

    movh       m0, [r0 + r1 * 2 + 16]
    movhps     m0, [r0 + r5 + 16]
    psllw      m0, 4
    psubw      m0, m2

    movh       [r2 + r3 * 2 + 16], m0
    movhps     [r2 + r4 + 16], m0

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET
%endmacro
P2S_H_12xN 16
P2S_H_12xN 32

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
INIT_XMM ssse3
cglobal filterPixelToShort_48x64, 3, 7, 5
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load height
    mov        r6d, 16

    ; load constant
    mova       m4, [pw_2000]

.loop
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    movu       m2, [r0 + r1 * 2]
    movu       m3, [r0 + r5]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1
    movu       [r2 + r3 * 2], m2
    movu       [r2 + r4], m3

    movu       m0, [r0 + 16]
    movu       m1, [r0 + r1 + 16]
    movu       m2, [r0 + r1 * 2 + 16]
    movu       m3, [r0 + r5 + 16]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 16], m0
    movu       [r2 + r3 * 1 + 16], m1
    movu       [r2 + r3 * 2 + 16], m2
    movu       [r2 + r4 + 16], m3

    movu       m0, [r0 + 32]
    movu       m1, [r0 + r1 + 32]
    movu       m2, [r0 + r1 * 2 + 32]
    movu       m3, [r0 + r5 + 32]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 32], m0
    movu       [r2 + r3 * 1 + 32], m1
    movu       [r2 + r3 * 2 + 32], m2
    movu       [r2 + r4 + 32], m3

    movu       m0, [r0 + 48]
    movu       m1, [r0 + r1 + 48]
    movu       m2, [r0 + r1 * 2 + 48]
    movu       m3, [r0 + r5 + 48]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 48], m0
    movu       [r2 + r3 * 1 + 48], m1
    movu       [r2 + r3 * 2 + 48], m2
    movu       [r2 + r4 + 48], m3

    movu       m0, [r0 + 64]
    movu       m1, [r0 + r1 + 64]
    movu       m2, [r0 + r1 * 2 + 64]
    movu       m3, [r0 + r5 + 64]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 64], m0
    movu       [r2 + r3 * 1 + 64], m1
    movu       [r2 + r3 * 2 + 64], m2
    movu       [r2 + r4 + 64], m3

    movu       m0, [r0 + 80]
    movu       m1, [r0 + r1 + 80]
    movu       m2, [r0 + r1 * 2 + 80]
    movu       m3, [r0 + r5 + 80]
    psllw      m0, 4
    psubw      m0, m4
    psllw      m1, 4
    psubw      m1, m4
    psllw      m2, 4
    psubw      m2, m4
    psllw      m3, 4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 80], m0
    movu       [r2 + r3 * 1 + 80], m1
    movu       [r2 + r3 * 2 + 80], m2
    movu       [r2 + r4 + 80], m3

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET
