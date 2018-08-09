;*****************************************************************************
;* Copyright (C) 2013-2017 MulticoreWare, Inc
;*
;* Authors: Nabajit Deka <nabajit@multicorewareinc.com>
;*          Murugan Vairavel <murugan@multicorewareinc.com>
;*          Min Chen <chenm003@163.com>
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


%define INTERP_OFFSET_PP        pd_32
%define INTERP_SHIFT_PP         6

%if BIT_DEPTH == 10
    %define INTERP_SHIFT_PS         2
    %define INTERP_OFFSET_PS        pd_n32768
    %define INTERP_SHIFT_SP         10
    %define INTERP_OFFSET_SP        h_pd_524800
%elif BIT_DEPTH == 12
    %define INTERP_SHIFT_PS         4
    %define INTERP_OFFSET_PS        pd_n131072
    %define INTERP_SHIFT_SP         8
    %define INTERP_OFFSET_SP        pd_524416
%else
    %error Unsupport bit depth!
%endif

SECTION_RODATA 32

h_pd_524800:        times 8 dd 524800
                                    
h_tab_LumaCoeff:    dw   0, 0,  0,  64,  0,   0,  0,  0
                  dw  -1, 4, -10, 58,  17, -5,  1,  0
                  dw  -1, 4, -11, 40,  40, -11, 4, -1
                  dw   0, 1, -5,  17,  58, -10, 4, -1
                  
ALIGN 32
h_tab_LumaCoeffV:   times 4 dw 0, 0
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
                  
const interp8_hps_shuf,     dd 0, 4, 1, 5, 2, 6, 3, 7                 

const interp8_hpp_shuf,     db 0, 1, 2, 3, 4, 5, 6, 7, 2, 3, 4, 5, 6, 7, 8, 9
                            db 4, 5, 6, 7, 8, 9, 10, 11, 6, 7, 8, 9, 10, 11, 12, 13 

const interp8_hpp_shuf_new, db 0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9
                            db 4, 5, 6, 7, 6, 7, 8, 9, 8, 9, 10, 11, 10, 11, 12, 13

ALIGN 64
interp8_hpp_shuf1_load_avx512: times 4 db 0, 1, 2, 3, 4, 5, 6, 7, 2, 3, 4, 5, 6, 7, 8, 9
interp8_hpp_shuf2_load_avx512: times 4 db 4, 5, 6, 7, 8, 9, 10, 11, 6, 7, 8, 9, 10, 11, 12, 13
interp8_hpp_shuf1_store_avx512: times 4 db 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15

SECTION .text
cextern pd_8
cextern pd_32
cextern pw_pixel_max
cextern pd_524416
cextern pd_n32768
cextern pd_n131072
cextern pw_2000
cextern idct8_shuf2

%macro FILTER_LUMA_HOR_4_sse2 1
    movu        m4,     [r0 + %1]       ; m4 = src[0-7]
    movu        m5,     [r0 + %1 + 2]   ; m5 = src[1-8]
    pmaddwd     m4,     m0
    pmaddwd     m5,     m0
    pshufd      m2,     m4,     q2301
    paddd       m4,     m2
    pshufd      m2,     m5,     q2301
    paddd       m5,     m2
    pshufd      m4,     m4,     q3120
    pshufd      m5,     m5,     q3120
    punpcklqdq  m4,     m5

    movu        m5,     [r0 + %1 + 4]   ; m5 = src[2-9]
    movu        m3,     [r0 + %1 + 6]   ; m3 = src[3-10]
    pmaddwd     m5,     m0
    pmaddwd     m3,     m0
    pshufd      m2,     m5,     q2301
    paddd       m5,     m2
    pshufd      m2,     m3,     q2301
    paddd       m3,     m2
    pshufd      m5,     m5,     q3120
    pshufd      m3,     m3,     q3120
    punpcklqdq  m5,     m3

    pshufd      m2,     m4,     q2301
    paddd       m4,     m2
    pshufd      m2,     m5,     q2301
    paddd       m5,     m2
    pshufd      m4,     m4,     q3120
    pshufd      m5,     m5,     q3120
    punpcklqdq  m4,     m5
    paddd       m4,     m1
%endmacro

%macro FILTER_LUMA_HOR_8_sse2 1
    movu        m4,     [r0 + %1]       ; m4 = src[0-7]
    movu        m5,     [r0 + %1 + 2]   ; m5 = src[1-8]
    pmaddwd     m4,     m0
    pmaddwd     m5,     m0
    pshufd      m2,     m4,     q2301
    paddd       m4,     m2
    pshufd      m2,     m5,     q2301
    paddd       m5,     m2
    pshufd      m4,     m4,     q3120
    pshufd      m5,     m5,     q3120
    punpcklqdq  m4,     m5

    movu        m5,     [r0 + %1 + 4]   ; m5 = src[2-9]
    movu        m3,     [r0 + %1 + 6]   ; m3 = src[3-10]
    pmaddwd     m5,     m0
    pmaddwd     m3,     m0
    pshufd      m2,     m5,     q2301
    paddd       m5,     m2
    pshufd      m2,     m3,     q2301
    paddd       m3,     m2
    pshufd      m5,     m5,     q3120
    pshufd      m3,     m3,     q3120
    punpcklqdq  m5,     m3

    pshufd      m2,     m4,     q2301
    paddd       m4,     m2
    pshufd      m2,     m5,     q2301
    paddd       m5,     m2
    pshufd      m4,     m4,     q3120
    pshufd      m5,     m5,     q3120
    punpcklqdq  m4,     m5
    paddd       m4,     m1

    movu        m5,     [r0 + %1 + 8]   ; m5 = src[4-11]
    movu        m6,     [r0 + %1 + 10]  ; m6 = src[5-12]
    pmaddwd     m5,     m0
    pmaddwd     m6,     m0
    pshufd      m2,     m5,     q2301
    paddd       m5,     m2
    pshufd      m2,     m6,     q2301
    paddd       m6,     m2
    pshufd      m5,     m5,     q3120
    pshufd      m6,     m6,     q3120
    punpcklqdq  m5,     m6

    movu        m6,     [r0 + %1 + 12]  ; m6 = src[6-13]
    movu        m3,     [r0 + %1 + 14]  ; m3 = src[7-14]
    pmaddwd     m6,     m0
    pmaddwd     m3,     m0
    pshufd      m2,     m6,     q2301
    paddd       m6,     m2
    pshufd      m2,     m3,     q2301
    paddd       m3,     m2
    pshufd      m6,     m6,     q3120
    pshufd      m3,     m3,     q3120
    punpcklqdq  m6,     m3

    pshufd      m2,     m5,     q2301
    paddd       m5,     m2
    pshufd      m2,     m6,     q2301
    paddd       m6,     m2
    pshufd      m5,     m5,     q3120
    pshufd      m6,     m6,     q3120
    punpcklqdq  m5,     m6
    paddd       m5,     m1
%endmacro

;------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_p%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;------------------------------------------------------------------------------------------------------------
%macro FILTER_HOR_LUMA_sse2 3
INIT_XMM sse2
cglobal interp_8tap_horiz_%3_%1x%2, 4, 7, 8
    mov         r4d,    r4m
    sub         r0,     6
    shl         r4d,    4
    add         r1d,    r1d
    add         r3d,    r3d

%ifdef PIC
    lea         r6,     [h_tab_LumaCoeff]
    mova        m0,     [r6 + r4]
%else
    mova        m0,     [h_tab_LumaCoeff + r4]
%endif

%ifidn %3, pp
    mova        m1,     [pd_32]
    pxor        m7,     m7
%else
    mova        m1,     [INTERP_OFFSET_PS]
%endif

    mov         r4d,    %2
%ifidn %3, ps
    cmp         r5m,    byte 0
    je          .loopH
    lea         r6,     [r1 + 2 * r1]
    sub         r0,     r6
    add         r4d,    7
%endif

.loopH:
%assign x 0
%rep %1/8
    FILTER_LUMA_HOR_8_sse2 x

%ifidn %3, pp
    psrad       m4,     6
    psrad       m5,     6
    packssdw    m4,     m5
    CLIPW       m4,     m7,     [pw_pixel_max]
%else
  %if BIT_DEPTH == 10
    psrad       m4,     2
    psrad       m5,     2
  %elif BIT_DEPTH == 12
    psrad       m4,     4
    psrad       m5,     4
  %endif
    packssdw    m4,     m5
%endif

    movu        [r2 + x], m4
%assign x x+16
%endrep

%rep (%1 % 8)/4
    FILTER_LUMA_HOR_4_sse2 x

%ifidn %3, pp
    psrad       m4,     6
    packssdw    m4,     m4
    CLIPW       m4,     m7,     [pw_pixel_max]
%else
  %if BIT_DEPTH == 10
    psrad       m4,     2
  %elif BIT_DEPTH == 12
    psrad       m4,     4
  %endif
    packssdw    m4,     m4
%endif

    movh        [r2 + x], m4
%endrep

    add         r0,     r1
    add         r2,     r3

    dec         r4d
    jnz         .loopH
    RET

%endmacro

;------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;------------------------------------------------------------------------------------------------------------
%if ARCH_X86_64
	FILTER_HOR_LUMA_sse2 4, 4, pp
    FILTER_HOR_LUMA_sse2 4, 8, pp
    FILTER_HOR_LUMA_sse2 4, 16, pp
    FILTER_HOR_LUMA_sse2 8, 4, pp
    FILTER_HOR_LUMA_sse2 8, 8, pp
    FILTER_HOR_LUMA_sse2 8, 16, pp
    FILTER_HOR_LUMA_sse2 8, 32, pp
    FILTER_HOR_LUMA_sse2 12, 16, pp
    FILTER_HOR_LUMA_sse2 16, 4, pp
    FILTER_HOR_LUMA_sse2 16, 8, pp
    FILTER_HOR_LUMA_sse2 16, 12, pp
    FILTER_HOR_LUMA_sse2 16, 16, pp
    FILTER_HOR_LUMA_sse2 16, 32, pp
    FILTER_HOR_LUMA_sse2 16, 64, pp
    FILTER_HOR_LUMA_sse2 24, 32, pp
    FILTER_HOR_LUMA_sse2 32, 8, pp
    FILTER_HOR_LUMA_sse2 32, 16, pp
    FILTER_HOR_LUMA_sse2 32, 24, pp
    FILTER_HOR_LUMA_sse2 32, 32, pp
    FILTER_HOR_LUMA_sse2 32, 64, pp
    FILTER_HOR_LUMA_sse2 48, 64, pp
    FILTER_HOR_LUMA_sse2 64, 16, pp
    FILTER_HOR_LUMA_sse2 64, 32, pp
    FILTER_HOR_LUMA_sse2 64, 48, pp
    FILTER_HOR_LUMA_sse2 64, 64, pp

;---------------------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_ps_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;---------------------------------------------------------------------------------------------------------------------------
    FILTER_HOR_LUMA_sse2 4, 4, ps
    FILTER_HOR_LUMA_sse2 4, 8, ps
    FILTER_HOR_LUMA_sse2 4, 16, ps
    FILTER_HOR_LUMA_sse2 8, 4, ps
    FILTER_HOR_LUMA_sse2 8, 8, ps
    FILTER_HOR_LUMA_sse2 8, 16, ps
    FILTER_HOR_LUMA_sse2 8, 32, ps
    FILTER_HOR_LUMA_sse2 12, 16, ps
    FILTER_HOR_LUMA_sse2 16, 4, ps
    FILTER_HOR_LUMA_sse2 16, 8, ps
    FILTER_HOR_LUMA_sse2 16, 12, ps
    FILTER_HOR_LUMA_sse2 16, 16, ps
    FILTER_HOR_LUMA_sse2 16, 32, ps
    FILTER_HOR_LUMA_sse2 16, 64, ps
    FILTER_HOR_LUMA_sse2 24, 32, ps
    FILTER_HOR_LUMA_sse2 32, 8, ps
    FILTER_HOR_LUMA_sse2 32, 16, ps
    FILTER_HOR_LUMA_sse2 32, 24, ps
    FILTER_HOR_LUMA_sse2 32, 32, ps
    FILTER_HOR_LUMA_sse2 32, 64, ps
    FILTER_HOR_LUMA_sse2 48, 64, ps
    FILTER_HOR_LUMA_sse2 64, 16, ps
    FILTER_HOR_LUMA_sse2 64, 32, ps
    FILTER_HOR_LUMA_sse2 64, 48, ps
    FILTER_HOR_LUMA_sse2 64, 64, ps
%endif

;-----------------------------------------------------------------------------
; void interp_4tap_horiz_%3_%1x%2(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------
%macro FILTER_HOR_CHROMA_sse3 3
INIT_XMM sse3
cglobal interp_4tap_horiz_%3_%1x%2, 4, 7, 8
    add         r3,     r3
    add         r1,     r1
    sub         r0,     2
    mov         r4d,    r4m
    add         r4d,    r4d

%ifdef PIC
    lea         r6,     [tab_ChromaCoeff]
    movddup     m0,     [r6 + r4 * 4]
%else
    movddup     m0,     [tab_ChromaCoeff + r4 * 4]
%endif

%ifidn %3, ps
    mova        m1,     [INTERP_OFFSET_PS]
    cmp         r5m,    byte 0
%if %1 <= 6
    lea         r4,     [r1 * 3]
    lea         r5,     [r3 * 3]
%endif
    je          .skip
    sub         r0,     r1
%if %1 <= 6
%assign y 1
%else
%assign y 3
%endif
%assign z 0
%rep y
%assign x 0
%rep %1/8
    FILTERH_W8_1_sse3 x, %3
%assign x x+16
%endrep
%if %1 == 4 || (%1 == 6 && z == 0) || (%1 == 12 && z == 0)
    FILTERH_W4_2_sse3 x, %3
    FILTERH_W4_1_sse3 x
%assign x x+8
%endif
%if %1 == 2 || (%1 == 6 && z == 0)
    FILTERH_W2_3_sse3 x
%endif
%if %1 <= 6
    lea         r0,     [r0 + r4]
    lea         r2,     [r2 + r5]
%else
    lea         r0,     [r0 + r1]
    lea         r2,     [r2 + r3]
%endif
%assign z z+1
%endrep
.skip:
%elifidn %3, pp
    pxor        m7,     m7
    mova        m6,     [pw_pixel_max]
    mova        m1,     [tab_c_32]
%if %1 == 2 || %1 == 6
    lea         r4,     [r1 * 3]
    lea         r5,     [r3 * 3]
%endif
%endif

%if %1 == 2
%assign y %2/4
%elif %1 <= 6
%assign y %2/2
%else
%assign y %2
%endif
%assign z 0
%rep y
%assign x 0
%rep %1/8
    FILTERH_W8_1_sse3 x, %3
%assign x x+16
%endrep
%if %1 == 4 || %1 == 6 || (%1 == 12 && (z % 2) == 0)
    FILTERH_W4_2_sse3 x, %3
%assign x x+8
%endif
%if %1 == 2 || (%1 == 6 && (z % 2) == 0)
    FILTERH_W2_4_sse3 x, %3
%endif
%assign z z+1
%if z < y
%if %1 == 2
    lea         r0,     [r0 + 4 * r1]
    lea         r2,     [r2 + 4 * r3]
%elif %1 <= 6
    lea         r0,     [r0 + 2 * r1]
    lea         r2,     [r2 + 2 * r3]
%else
    lea         r0,     [r0 + r1]
    lea         r2,     [r2 + r3]
%endif
%endif ;z < y
%endrep

    RET
%endmacro

%macro FILTER_P2S_2_4_sse2 1
    movd        m0,     [r0 + %1]
    movd        m2,     [r0 + r1 * 2 + %1]
    movhps      m0,     [r0 + r1 + %1]
    movhps      m2,     [r0 + r4 + %1]
    psllw       m0,     (14 - BIT_DEPTH)
    psllw       m2,     (14 - BIT_DEPTH)
    psubw       m0,     m1
    psubw       m2,     m1

    movd        [r2 + r3 * 0 + %1], m0
    movd        [r2 + r3 * 2 + %1], m2
    movhlps     m0,     m0
    movhlps     m2,     m2
    movd        [r2 + r3 * 1 + %1], m0
    movd        [r2 + r5 + %1], m2
%endmacro

%macro FILTER_P2S_4_4_sse2 1
    movh        m0,     [r0 + %1]
    movhps      m0,     [r0 + r1 + %1]
    psllw       m0,     (14 - BIT_DEPTH)
    psubw       m0,     m1
    movh        [r2 + r3 * 0 + %1], m0
    movhps      [r2 + r3 * 1 + %1], m0

    movh        m2,     [r0 + r1 * 2 + %1]
    movhps      m2,     [r0 + r4 + %1]
    psllw       m2,     (14 - BIT_DEPTH)
    psubw       m2,     m1
    movh        [r2 + r3 * 2 + %1], m2
    movhps      [r2 + r5 + %1], m2
%endmacro

%macro FILTER_P2S_4_2_sse2 0
    movh        m0,     [r0]
    movhps      m0,     [r0 + r1 * 2]
    psllw       m0,     (14 - BIT_DEPTH)
    psubw       m0,     [pw_2000]
    movh        [r2 + r3 * 0], m0
    movhps      [r2 + r3 * 2], m0
%endmacro

%macro FILTER_P2S_8_4_sse2 1
    movu        m0,     [r0 + %1]
    movu        m2,     [r0 + r1 + %1]
    psllw       m0,     (14 - BIT_DEPTH)
    psllw       m2,     (14 - BIT_DEPTH)
    psubw       m0,     m1
    psubw       m2,     m1
    movu        [r2 + r3 * 0 + %1], m0
    movu        [r2 + r3 * 1 + %1], m2

    movu        m3,     [r0 + r1 * 2 + %1]
    movu        m4,     [r0 + r4 + %1]
    psllw       m3,     (14 - BIT_DEPTH)
    psllw       m4,     (14 - BIT_DEPTH)
    psubw       m3,     m1
    psubw       m4,     m1
    movu        [r2 + r3 * 2 + %1], m3
    movu        [r2 + r5 + %1], m4
%endmacro

%macro FILTER_P2S_8_2_sse2 1
    movu        m0,     [r0 + %1]
    movu        m2,     [r0 + r1 + %1]
    psllw       m0,     (14 - BIT_DEPTH)
    psllw       m2,     (14 - BIT_DEPTH)
    psubw       m0,     m1
    psubw       m2,     m1
    movu        [r2 + r3 * 0 + %1], m0
    movu        [r2 + r3 * 1 + %1], m2
%endmacro

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
%macro FILTER_PIX_TO_SHORT_sse2 2
INIT_XMM sse2
cglobal filterPixelToShort_%1x%2, 4, 6, 3
%if %2 == 2
%if %1 == 4
    FILTER_P2S_4_2_sse2
%elif %1 == 8
    add        r1d, r1d
    add        r3d, r3d
    mova       m1, [pw_2000]
    FILTER_P2S_8_2_sse2 0
%endif
%else
    add        r1d, r1d
    add        r3d, r3d
    mova       m1, [pw_2000]
    lea        r4, [r1 * 3]
    lea        r5, [r3 * 3]
%assign y 1
%rep %2/4
%assign x 0
%rep %1/8
    FILTER_P2S_8_4_sse2 x
%if %2 == 6
    lea         r0,     [r0 + 4 * r1]
    lea         r2,     [r2 + 4 * r3]
    FILTER_P2S_8_2_sse2 x
%endif
%assign x x+16
%endrep
%rep (%1 % 8)/4
    FILTER_P2S_4_4_sse2 x
%assign x x+8
%endrep
%rep (%1 % 4)/2
    FILTER_P2S_2_4_sse2 x
%endrep
%if y < %2/4
    lea         r0,     [r0 + 4 * r1]
    lea         r2,     [r2 + 4 * r3]
%assign y y+1
%endif
%endrep
%endif
RET
%endmacro

    FILTER_PIX_TO_SHORT_sse2 2, 4
    FILTER_PIX_TO_SHORT_sse2 2, 8
    FILTER_PIX_TO_SHORT_sse2 2, 16
    FILTER_PIX_TO_SHORT_sse2 4, 2
    FILTER_PIX_TO_SHORT_sse2 4, 4
    FILTER_PIX_TO_SHORT_sse2 4, 8
    FILTER_PIX_TO_SHORT_sse2 4, 16
    FILTER_PIX_TO_SHORT_sse2 4, 32
    FILTER_PIX_TO_SHORT_sse2 6, 8
    FILTER_PIX_TO_SHORT_sse2 6, 16
    FILTER_PIX_TO_SHORT_sse2 8, 2
    FILTER_PIX_TO_SHORT_sse2 8, 4
    FILTER_PIX_TO_SHORT_sse2 8, 6
    FILTER_PIX_TO_SHORT_sse2 8, 8
    FILTER_PIX_TO_SHORT_sse2 8, 12
    FILTER_PIX_TO_SHORT_sse2 8, 16
    FILTER_PIX_TO_SHORT_sse2 8, 32
    FILTER_PIX_TO_SHORT_sse2 8, 64
    FILTER_PIX_TO_SHORT_sse2 12, 16
    FILTER_PIX_TO_SHORT_sse2 12, 32
    FILTER_PIX_TO_SHORT_sse2 16, 4
    FILTER_PIX_TO_SHORT_sse2 16, 8
    FILTER_PIX_TO_SHORT_sse2 16, 12
    FILTER_PIX_TO_SHORT_sse2 16, 16
    FILTER_PIX_TO_SHORT_sse2 16, 24
    FILTER_PIX_TO_SHORT_sse2 16, 32
    FILTER_PIX_TO_SHORT_sse2 16, 64
    FILTER_PIX_TO_SHORT_sse2 24, 32
    FILTER_PIX_TO_SHORT_sse2 24, 64
    FILTER_PIX_TO_SHORT_sse2 32, 8
    FILTER_PIX_TO_SHORT_sse2 32, 16
    FILTER_PIX_TO_SHORT_sse2 32, 24
    FILTER_PIX_TO_SHORT_sse2 32, 32
    FILTER_PIX_TO_SHORT_sse2 32, 48
    FILTER_PIX_TO_SHORT_sse2 32, 64
    FILTER_PIX_TO_SHORT_sse2 48, 64
    FILTER_PIX_TO_SHORT_sse2 64, 16
    FILTER_PIX_TO_SHORT_sse2 64, 32
    FILTER_PIX_TO_SHORT_sse2 64, 48
    FILTER_PIX_TO_SHORT_sse2 64, 64

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
    lea         r6, [h_tab_LumaCoeff]
    mova        m0, [r6 + r4]
%else
    mova        m0, [h_tab_LumaCoeff + r4]
%endif

%ifidn %3, pp
    mova        m1, [pd_32]
    pxor        m6, m6
    mova        m7, [pw_pixel_max]
%else
    mova        m1, [INTERP_OFFSET_PS]
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
    psrad       m4, INTERP_SHIFT_PS
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
    lea         r6, [h_tab_LumaCoeff]
    mova        m0, [r6 + r4]
%else
    mova        m0, [h_tab_LumaCoeff + r4]
%endif

%ifidn %3, pp
    mova        m1, [pd_32]
    pxor        m7, m7
%else
    mova        m1, [INTERP_OFFSET_PS]
%endif

    mov         r4d, %2
%ifidn %3, ps
    cmp         r5m, byte 0
    je         .loopH
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
    psrad       m4, INTERP_SHIFT_PS
    psrad       m5, INTERP_SHIFT_PS
    packssdw    m4, m5
%endif

    movu        [r2], m4

    add         r0, r1
    add         r2, r3

    dec         r4d
    jnz        .loopH
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
    lea         r6, [h_tab_LumaCoeff]
    mova        m0, [r6 + r4]
%else
    mova        m0, [h_tab_LumaCoeff + r4]
%endif
%ifidn %3, pp
    mova        m1, [INTERP_OFFSET_PP]
%else
    mova        m1, [INTERP_OFFSET_PS]
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
    psrad       m4, INTERP_SHIFT_PP
    psrad       m5, INTERP_SHIFT_PP
    packusdw    m4, m5
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, INTERP_SHIFT_PS
    psrad       m5, INTERP_SHIFT_PS
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
    psrad       m4, INTERP_SHIFT_PP
    packusdw    m4, m4
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, INTERP_SHIFT_PS
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
    lea         r6, [h_tab_LumaCoeff]
    mova        m0, [r6 + r4]
%else
    mova        m0, [h_tab_LumaCoeff + r4]
%endif

%ifidn %3, pp
    mova        m1, [pd_32]
%else
    mova        m1, [INTERP_OFFSET_PS]
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
    psrad       m4, INTERP_SHIFT_PP
    psrad       m5, INTERP_SHIFT_PP
    packusdw    m4, m5
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, INTERP_SHIFT_PS
    psrad       m5, INTERP_SHIFT_PS
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
    psrad       m4, INTERP_SHIFT_PP
    psrad       m5, INTERP_SHIFT_PP
    packusdw    m4, m5
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, INTERP_SHIFT_PS
    psrad       m5, INTERP_SHIFT_PS
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
    lea         r6, [h_tab_LumaCoeff]
    mova        m0, [r6 + r4]
%else
    mova        m0, [h_tab_LumaCoeff + r4]
%endif
%ifidn %3, pp
    mova        m1, [pd_32]
%else
    mova        m1, [INTERP_OFFSET_PS]
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
    psrad       m4, INTERP_SHIFT_PP
    psrad       m5, INTERP_SHIFT_PP
    packusdw    m4, m5
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, INTERP_SHIFT_PS
    psrad       m5, INTERP_SHIFT_PS
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
    psrad       m4, INTERP_SHIFT_PP
    psrad       m5, INTERP_SHIFT_PP
    packusdw    m4, m5
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, INTERP_SHIFT_PS
    psrad       m5, INTERP_SHIFT_PS
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
    psrad       m4, INTERP_SHIFT_PP
    psrad       m5, INTERP_SHIFT_PP
    packusdw    m4, m5
    pxor        m5, m5
    CLIPW       m4, m5, [pw_pixel_max]
%else
    psrad       m4, INTERP_SHIFT_PS
    psrad       m5, INTERP_SHIFT_PS
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

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
%macro FILTER_HOR_LUMA_W4_avx2 1
INIT_YMM avx2
cglobal interp_8tap_horiz_pp_4x%1, 4,7,7
    add              r1d, r1d
    add              r3d, r3d
    sub              r0, 6
    mov              r4d, r4m
    shl              r4d, 4
%ifdef PIC
    lea              r5, [h_tab_LumaCoeff]
    vpbroadcastq     m0, [r5 + r4]
    vpbroadcastq     m1, [r5 + r4 + 8]
%else
    vpbroadcastq     m0, [h_tab_LumaCoeff + r4]
    vpbroadcastq     m1, [h_tab_LumaCoeff + r4 + 8]
%endif
    lea              r6, [pw_pixel_max]
    mova             m3, [interp8_hpp_shuf]
    mova             m6, [pd_32]
    pxor             m2, m2

    ; register map
    ; m0 , m1 interpolate coeff

    mov              r4d, %1/2

.loop:
    vbroadcasti128   m4, [r0]
    vbroadcasti128   m5, [r0 + 8]
    pshufb           m4, m3
    pshufb           m5, m3

    pmaddwd          m4, m0
    pmaddwd          m5, m1
    paddd            m4, m5

    phaddd           m4, m4
    vpermq           m4, m4, q3120
    paddd            m4, m6
    psrad            m4, INTERP_SHIFT_PP

    packusdw         m4, m4
    vpermq           m4, m4, q2020
    CLIPW            m4, m2, [r6]
    movq             [r2], xm4

    vbroadcasti128   m4, [r0 + r1]
    vbroadcasti128   m5, [r0 + r1 + 8]
    pshufb           m4, m3
    pshufb           m5, m3

    pmaddwd          m4, m0
    pmaddwd          m5, m1
    paddd            m4, m5

    phaddd           m4, m4
    vpermq           m4, m4, q3120
    paddd            m4, m6
    psrad            m4, INTERP_SHIFT_PP

    packusdw         m4, m4
    vpermq           m4, m4, q2020
    CLIPW            m4, m2, [r6]
    movq             [r2 + r3], xm4

    lea              r2, [r2 + 2 * r3]
    lea              r0, [r0 + 2 * r1]
    dec              r4d
    jnz              .loop
    RET
%endmacro
FILTER_HOR_LUMA_W4_avx2 4
FILTER_HOR_LUMA_W4_avx2 8
FILTER_HOR_LUMA_W4_avx2 16

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_IPFILTER_LUMA_PP_8x2_AVX2 0
    movu            xm7,        [r0]
    movu            xm8,        [r0 + 8]
    vinserti128     m7,        m7,        [r0 + r1],          1
    vinserti128     m8,        m8,        [r0 + r1 + 8],      1
    pshufb          m10,       m7,        m14
    pshufb          m7,                   m13
    pshufb          m11,       m8,        m14
    pshufb          m8,                   m13

    pmaddwd         m7,        m0
    pmaddwd         m10,       m1
    paddd           m7,        m10
    pmaddwd         m10,       m11,       m3
    pmaddwd         m9,        m8,        m2
    paddd           m10,       m9
    paddd           m7,        m10
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PP

    movu            xm9,        [r0 + 16]
    vinserti128     m9,        m9,        [r0 + r1 + 16],      1
    pshufb          m10,       m9,        m14
    pshufb          m9,                   m13
    pmaddwd         m8,        m0
    pmaddwd         m11,       m1
    paddd           m8,        m11
    pmaddwd         m10,       m3
    pmaddwd         m9,        m2
    paddd           m9,        m10
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        INTERP_SHIFT_PP

    packusdw        m7,        m8
    pshufb          m7,        m12
    CLIPW           m7,        m5,         m6
    movu            [r2],      xm7
    vextracti128    [r2 + r3], m7,         1
%endmacro

%macro IPFILTER_LUMA_AVX2_8xN 1
INIT_YMM avx2
cglobal interp_8tap_horiz_pp_8x%1, 5,6,15
    shl              r1d,        1
    shl              r3d,        1
    sub              r0,         6
    mov              r4d,        r4m
    shl              r4d,        4

%ifdef PIC
    lea              r5,         [h_tab_LumaCoeff]
    vpbroadcastd     m0,         [r5 + r4]
    vpbroadcastd     m1,         [r5 + r4 + 4]
    vpbroadcastd     m2,         [r5 + r4 + 8]
    vpbroadcastd     m3,         [r5 + r4 + 12]
%else
    vpbroadcastd     m0,         [h_tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [h_tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [h_tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [h_tab_LumaCoeff + r4 + 12]
%endif
    mova             m13,        [interp8_hpp_shuf1_load_avx512]
    mova             m14,        [interp8_hpp_shuf2_load_avx512]
    mova             m12,        [interp8_hpp_shuf1_store_avx512]
    mova             m4,         [pd_32]
    pxor             m5,         m5
    mova             m6,         [pw_pixel_max]

%rep %1/2 - 1
    PROCESS_IPFILTER_LUMA_PP_8x2_AVX2
    lea              r0,         [r0 + 2 * r1]
    lea              r2,         [r2 + 2 * r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_8x2_AVX2
    RET
%endmacro

%if ARCH_X86_64
    IPFILTER_LUMA_AVX2_8xN 4
    IPFILTER_LUMA_AVX2_8xN 8
    IPFILTER_LUMA_AVX2_8xN 16
    IPFILTER_LUMA_AVX2_8xN 32
%endif

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_IPFILTER_LUMA_PP_16x1_AVX2 0
    movu            m7,        [r0]
    movu            m8,        [r0 + 8]

    pshufb          m10,       m7,        m14
    pshufb          m7,                   m13
    pshufb          m11,       m8,        m14
    pshufb          m8,                   m13

    pmaddwd         m7,        m0
    pmaddwd         m10,       m1
    paddd           m7,        m10
    pmaddwd         m10,       m11,       m3
    pmaddwd         m9,        m8,        m2
    paddd           m10,       m9
    paddd           m7,        m10
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PP

    movu            m9,        [r0 + 16]
    pshufb          m10,       m9,        m14
    pshufb          m9,                   m13
    pmaddwd         m8,        m0
    pmaddwd         m11,       m1
    paddd           m8,        m11
    pmaddwd         m10,       m3
    pmaddwd         m9,        m2
    paddd           m9,        m10
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        INTERP_SHIFT_PP

    packusdw        m7,        m8
    pshufb          m7,        m12
    CLIPW           m7,        m5,         m6
    movu            [r2],      m7
%endmacro

%macro IPFILTER_LUMA_AVX2_16xN 1
INIT_YMM avx2
cglobal interp_8tap_horiz_pp_16x%1, 5,6,15
    shl              r1d,        1
    shl              r3d,        1
    sub              r0,         6
    mov              r4d,        r4m
    shl              r4d,        4

%ifdef PIC
    lea              r5,         [h_tab_LumaCoeff]
    vpbroadcastd     m0,         [r5 + r4]
    vpbroadcastd     m1,         [r5 + r4 + 4]
    vpbroadcastd     m2,         [r5 + r4 + 8]
    vpbroadcastd     m3,         [r5 + r4 + 12]
%else
    vpbroadcastd     m0,         [h_tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [h_tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [h_tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [h_tab_LumaCoeff + r4 + 12]
%endif
    mova             m13,        [interp8_hpp_shuf1_load_avx512]
    mova             m14,        [interp8_hpp_shuf2_load_avx512]
    mova             m12,        [interp8_hpp_shuf1_store_avx512]
    mova             m4,         [pd_32]
    pxor             m5,         m5
    mova             m6,         [pw_pixel_max]

%rep %1 - 1
    PROCESS_IPFILTER_LUMA_PP_16x1_AVX2
    lea              r0,         [r0 + r1]
    lea              r2,         [r2 + r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_16x1_AVX2
    RET
%endmacro

%if ARCH_X86_64
    IPFILTER_LUMA_AVX2_16xN 4
    IPFILTER_LUMA_AVX2_16xN 8
    IPFILTER_LUMA_AVX2_16xN 12
    IPFILTER_LUMA_AVX2_16xN 16
    IPFILTER_LUMA_AVX2_16xN 32
    IPFILTER_LUMA_AVX2_16xN 64
%endif

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_IPFILTER_LUMA_PP_32x1_AVX2 0
    PROCESS_IPFILTER_LUMA_PP_16x1_AVX2

    movu            m7,        [r0 + mmsize]
    movu            m8,        [r0 + 8 + mmsize]

    pshufb          m10,       m7,        m14
    pshufb          m7,                   m13
    pshufb          m11,       m8,        m14
    pshufb          m8,                   m13

    pmaddwd         m7,        m0
    pmaddwd         m10,       m1
    paddd           m7,        m10
    pmaddwd         m10,       m11,       m3
    pmaddwd         m9,        m8,        m2
    paddd           m10,       m9
    paddd           m7,        m10
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PP

    movu            m9,        [r0 + 16 + mmsize]
    pshufb          m10,       m9,        m14
    pshufb          m9,                   m13
    pmaddwd         m8,        m0
    pmaddwd         m11,       m1
    paddd           m8,        m11
    pmaddwd         m10,       m3
    pmaddwd         m9,        m2
    paddd           m9,        m10
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        INTERP_SHIFT_PP

    packusdw        m7,        m8
    pshufb          m7,        m12
    CLIPW           m7,        m5,         m6
    movu            [r2 + mmsize],         m7
%endmacro

%macro IPFILTER_LUMA_AVX2_32xN 1
INIT_YMM avx2
cglobal interp_8tap_horiz_pp_32x%1, 5,6,15
    shl              r1d,        1
    shl              r3d,        1
    sub              r0,         6
    mov              r4d,        r4m
    shl              r4d,        4

%ifdef PIC
    lea              r5,         [h_tab_LumaCoeff]
    vpbroadcastd     m0,         [r5 + r4]
    vpbroadcastd     m1,         [r5 + r4 + 4]
    vpbroadcastd     m2,         [r5 + r4 + 8]
    vpbroadcastd     m3,         [r5 + r4 + 12]
%else
    vpbroadcastd     m0,         [h_tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [h_tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [h_tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [h_tab_LumaCoeff + r4 + 12]
%endif
    mova             m13,        [interp8_hpp_shuf1_load_avx512]
    mova             m14,        [interp8_hpp_shuf2_load_avx512]
    mova             m12,        [interp8_hpp_shuf1_store_avx512]
    mova             m4,         [pd_32]
    pxor             m5,         m5
    mova             m6,         [pw_pixel_max]

%rep %1 - 1
    PROCESS_IPFILTER_LUMA_PP_32x1_AVX2
    lea              r0,         [r0 + r1]
    lea              r2,         [r2 + r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_32x1_AVX2
    RET
%endmacro

%if ARCH_X86_64
    IPFILTER_LUMA_AVX2_32xN 8
    IPFILTER_LUMA_AVX2_32xN 16
    IPFILTER_LUMA_AVX2_32xN 24
    IPFILTER_LUMA_AVX2_32xN 32
    IPFILTER_LUMA_AVX2_32xN 64
%endif

%macro PROCESS_IPFILTER_LUMA_PP_64x1_AVX2 0
    PROCESS_IPFILTER_LUMA_PP_16x1_AVX2
%assign x 32
%rep 3
    movu            m7,        [r0 + x]
    movu            m8,        [r0 + 8 + x]

    pshufb          m10,       m7,        m14
    pshufb          m7,                   m13
    pshufb          m11,       m8,        m14
    pshufb          m8,                   m13

    pmaddwd         m7,        m0
    pmaddwd         m10,       m1
    paddd           m7,        m10
    pmaddwd         m10,       m11,       m3
    pmaddwd         m9,        m8,        m2
    paddd           m10,       m9
    paddd           m7,        m10
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PP

    movu            m9,        [r0 + 16 + x]
    pshufb          m10,       m9,        m14
    pshufb          m9,                   m13
    pmaddwd         m8,        m0
    pmaddwd         m11,       m1
    paddd           m8,        m11
    pmaddwd         m10,       m3
    pmaddwd         m9,        m2
    paddd           m9,        m10
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        INTERP_SHIFT_PP

    packusdw        m7,        m8
    pshufb          m7,        m12
    CLIPW           m7,        m5,         m6
    movu            [r2 + x],  m7
%assign x x+32
%endrep
%endmacro

%macro IPFILTER_LUMA_AVX2_64xN 1
INIT_YMM avx2
cglobal interp_8tap_horiz_pp_64x%1, 5,6,15
    shl              r1d,        1
    shl              r3d,        1
    sub              r0,         6
    mov              r4d,        r4m
    shl              r4d,        4

%ifdef PIC
    lea              r5,         [h_tab_LumaCoeff]
    vpbroadcastd     m0,         [r5 + r4]
    vpbroadcastd     m1,         [r5 + r4 + 4]
    vpbroadcastd     m2,         [r5 + r4 + 8]
    vpbroadcastd     m3,         [r5 + r4 + 12]
%else
    vpbroadcastd     m0,         [h_tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [h_tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [h_tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [h_tab_LumaCoeff + r4 + 12]
%endif
    mova             m13,        [interp8_hpp_shuf1_load_avx512]
    mova             m14,        [interp8_hpp_shuf2_load_avx512]
    mova             m12,        [interp8_hpp_shuf1_store_avx512]
    mova             m4,         [pd_32]
    pxor             m5,         m5
    mova             m6,         [pw_pixel_max]

%rep %1 - 1
    PROCESS_IPFILTER_LUMA_PP_64x1_AVX2
    lea              r0,         [r0 + r1]
    lea              r2,         [r2 + r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_64x1_AVX2
    RET
%endmacro

%if ARCH_X86_64
    IPFILTER_LUMA_AVX2_64xN 16
    IPFILTER_LUMA_AVX2_64xN 32
    IPFILTER_LUMA_AVX2_64xN 48
    IPFILTER_LUMA_AVX2_64xN 64
%endif

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
cglobal interp_8tap_horiz_pp_12x16, 4,6,8
    add              r1d, r1d
    add              r3d, r3d
    sub              r0, 6
    mov              r4d, r4m
    shl              r4d, 4
%ifdef PIC
    lea              r5, [h_tab_LumaCoeff]
    vpbroadcastq     m0, [r5 + r4]
    vpbroadcastq     m1, [r5 + r4 + 8]
%else
    vpbroadcastq     m0, [h_tab_LumaCoeff + r4]
    vpbroadcastq     m1, [h_tab_LumaCoeff + r4 + 8]
%endif
    mova             m3, [interp8_hpp_shuf]
    mova             m7, [pd_32]
    pxor             m2, m2

    ; register map
    ; m0 , m1 interpolate coeff

    mov              r4d, 16

.loop:
    vbroadcasti128   m4, [r0]
    vbroadcasti128   m5, [r0 + 8]
    pshufb           m4, m3
    pshufb           m5, m3

    pmaddwd          m4, m0
    pmaddwd          m5, m1
    paddd            m4, m5

    vbroadcasti128   m5, [r0 + 8]
    vbroadcasti128   m6, [r0 + 16]
    pshufb           m5, m3
    pshufb           m6, m3

    pmaddwd          m5, m0
    pmaddwd          m6, m1
    paddd            m5, m6

    phaddd           m4, m5
    vpermq           m4, m4, q3120
    paddd            m4, m7
    psrad            m4, INTERP_SHIFT_PP

    packusdw         m4, m4
    vpermq           m4, m4, q2020
    CLIPW            m4, m2, [pw_pixel_max]
    movu             [r2], xm4

    vbroadcasti128   m4, [r0 + 16]
    vbroadcasti128   m5, [r0 + 24]
    pshufb           m4, m3
    pshufb           m5, m3

    pmaddwd          m4, m0
    pmaddwd          m5, m1
    paddd            m4, m5

    vbroadcasti128   m5, [r0 + 24]
    vbroadcasti128   m6, [r0 + 32]
    pshufb           m5, m3
    pshufb           m6, m3

    pmaddwd          m5, m0
    pmaddwd          m6, m1
    paddd            m5, m6

    phaddd           m4, m5
    vpermq           m4, m4, q3120
    paddd            m4, m7
    psrad            m4, INTERP_SHIFT_PP

    packusdw         m4, m4
    vpermq           m4, m4, q2020
    CLIPW            m4, m2, [pw_pixel_max]
    movq             [r2 + 16], xm4

    add              r2, r3
    add              r0, r1
    dec              r4d
    jnz              .loop
    RET

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
INIT_YMM avx2
cglobal interp_8tap_horiz_pp_24x32, 4,6,8
    add              r1d, r1d
    add              r3d, r3d
    sub              r0, 6
    mov              r4d, r4m
    shl              r4d, 4
%ifdef PIC
    lea              r5, [h_tab_LumaCoeff]
    vpbroadcastq     m0, [r5 + r4]
    vpbroadcastq     m1, [r5 + r4 + 8]
%else
    vpbroadcastq     m0, [h_tab_LumaCoeff + r4]
    vpbroadcastq     m1, [h_tab_LumaCoeff + r4 + 8]
%endif
    mova             m3, [interp8_hpp_shuf]
    mova             m7, [pd_32]
    pxor             m2, m2

    ; register map
    ; m0 , m1 interpolate coeff

    mov              r4d, 32

.loop:
    vbroadcasti128   m4, [r0]
    vbroadcasti128   m5, [r0 + 8]
    pshufb           m4, m3
    pshufb           m5, m3

    pmaddwd          m4, m0
    pmaddwd          m5, m1
    paddd            m4, m5

    vbroadcasti128   m5, [r0 + 8]
    vbroadcasti128   m6, [r0 + 16]
    pshufb           m5, m3
    pshufb           m6, m3

    pmaddwd          m5, m0
    pmaddwd          m6, m1
    paddd            m5, m6

    phaddd           m4, m5
    vpermq           m4, m4, q3120
    paddd            m4, m7
    psrad            m4, INTERP_SHIFT_PP

    packusdw         m4, m4
    vpermq           m4, m4, q2020
    CLIPW            m4, m2, [pw_pixel_max]
    movu             [r2], xm4

    vbroadcasti128   m4, [r0 + 16]
    vbroadcasti128   m5, [r0 + 24]
    pshufb           m4, m3
    pshufb           m5, m3

    pmaddwd          m4, m0
    pmaddwd          m5, m1
    paddd            m4, m5

    vbroadcasti128   m5, [r0 + 24]
    vbroadcasti128   m6, [r0 + 32]
    pshufb           m5, m3
    pshufb           m6, m3

    pmaddwd          m5, m0
    pmaddwd          m6, m1
    paddd            m5, m6

    phaddd           m4, m5
    vpermq           m4, m4, q3120
    paddd            m4, m7
    psrad            m4, INTERP_SHIFT_PP

    packusdw         m4, m4
    vpermq           m4, m4, q2020
    CLIPW            m4, m2, [pw_pixel_max]
    movu             [r2 + 16], xm4

    vbroadcasti128   m4, [r0 + 32]
    vbroadcasti128   m5, [r0 + 40]
    pshufb           m4, m3
    pshufb           m5, m3

    pmaddwd          m4, m0
    pmaddwd          m5, m1
    paddd            m4, m5

    vbroadcasti128   m5, [r0 + 40]
    vbroadcasti128   m6, [r0 + 48]
    pshufb           m5, m3
    pshufb           m6, m3

    pmaddwd          m5, m0
    pmaddwd          m6, m1
    paddd            m5, m6

    phaddd           m4, m5
    vpermq           m4, m4, q3120
    paddd            m4, m7
    psrad            m4, INTERP_SHIFT_PP

    packusdw         m4, m4
    vpermq           m4, m4, q2020
    CLIPW            m4, m2, [pw_pixel_max]
    movu             [r2 + 32], xm4

    add              r2, r3
    add              r0, r1
    dec              r4d
    jnz              .loop
    RET

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_horiz_pp(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_IPFILTER_LUMA_PP_48x1_AVX2 0
    PROCESS_IPFILTER_LUMA_PP_32x1_AVX2

    movu            m7,        [r0 + 2 * mmsize]
    movu            m8,        [r0 + 8 + 2 * mmsize]

    pshufb          m10,       m7,        m14
    pshufb          m7,                   m13
    pshufb          m11,       m8,        m14
    pshufb          m8,                   m13

    pmaddwd         m7,        m0
    pmaddwd         m10,       m1
    paddd           m7,        m10
    pmaddwd         m10,       m11,       m3
    pmaddwd         m9,        m8,        m2
    paddd           m10,       m9
    paddd           m7,        m10
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PP

    movu            m9,        [r0 + 16 + 2 * mmsize]
    pshufb          m10,       m9,        m14
    pshufb          m9,                   m13
    pmaddwd         m8,        m0
    pmaddwd         m11,       m1
    paddd           m8,        m11
    pmaddwd         m10,       m3
    pmaddwd         m9,        m2
    paddd           m9,        m10
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        INTERP_SHIFT_PP

    packusdw        m7,        m8
    pshufb          m7,        m12
    CLIPW           m7,        m5,         m6
    movu            [r2 + 2 * mmsize],     m7
%endmacro

%if ARCH_X86_64
INIT_YMM avx2
cglobal interp_8tap_horiz_pp_48x64, 5,6,15
    shl              r1d,        1
    shl              r3d,        1
    sub              r0,         6
    mov              r4d,        r4m
    shl              r4d,        4

%ifdef PIC
    lea              r5,         [h_tab_LumaCoeff]
    vpbroadcastd     m0,         [r5 + r4]
    vpbroadcastd     m1,         [r5 + r4 + 4]
    vpbroadcastd     m2,         [r5 + r4 + 8]
    vpbroadcastd     m3,         [r5 + r4 + 12]
%else
    vpbroadcastd     m0,         [h_tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [h_tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [h_tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [h_tab_LumaCoeff + r4 + 12]
%endif
    mova             m13,        [interp8_hpp_shuf1_load_avx512]
    mova             m14,        [interp8_hpp_shuf2_load_avx512]
    mova             m12,        [interp8_hpp_shuf1_store_avx512]
    mova             m4,         [pd_32]
    pxor             m5,         m5
    mova             m6,         [pw_pixel_max]

%rep 63
    PROCESS_IPFILTER_LUMA_PP_48x1_AVX2
    lea              r0,         [r0 + r1]
    lea              r2,         [r2 + r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_48x1_AVX2
    RET
%endif

;-----------------------------------------------------------------------------------------------------------------------------
;void interp_horiz_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
;-----------------------------------------------------------------------------------------------------------------------------

%macro IPFILTER_LUMA_PS_4xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_horiz_ps_4x%1, 6,8,7
    mov                         r5d,               r5m
    mov                         r4d,               r4m
    add                         r1d,               r1d
    add                         r3d,               r3d

%ifdef PIC
    lea                         r6,                [h_tab_LumaCoeff]
    lea                         r4,                [r4 * 8]
    vbroadcasti128              m0,                [r6 + r4 * 2]
%else
    lea                         r4,                [r4 * 8]
    vbroadcasti128              m0,                [h_tab_LumaCoeff + r4 * 2]
%endif

    vbroadcasti128              m2,                [INTERP_OFFSET_PS]

    ; register map
    ; m0 - interpolate coeff
    ; m1 - shuffle order table
    ; m2 - pw_2000

    sub                         r0,                6
    test                        r5d,               r5d
    mov                         r7d,               %1                                    ; loop count variable - height
    jz                         .preloop
    lea                         r6,                [r1 * 3]                              ; r6 = (N / 2 - 1) * srcStride
    sub                         r0,                r6                                    ; r0(src) - 3 * srcStride
    add                         r7d,               6                                     ;7 - 1(since last row not in loop)                            ; need extra 7 rows, just set a specially flag here, blkheight += N - 1  (7 - 3 = 4 ; since the last three rows not in loop)

.preloop:
    lea                         r6,                [r3 * 3]
.loop:
    ; Row 0
    movu                        xm3,                [r0]                                 ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    movu                        xm4,                [r0 + 2]                             ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    vinserti128                 m3,                 m3,                xm4,       1
    movu                        xm4,                [r0 + 4]
    movu                        xm5,                [r0 + 6]
    vinserti128                 m4,                 m4,                xm5,       1
    pmaddwd                     m3,                m0
    pmaddwd                     m4,                m0
    phaddd                      m3,                m4                                    ; DWORD [R1D R1C R0D R0C R1B R1A R0B R0A]

    ; Row 1
    movu                        xm4,                [r0 + r1]                            ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    movu                        xm5,                [r0 + r1 + 2]                        ; [x x x x x A 9 8 7 6 5 4 3 2 1 0]
    vinserti128                 m4,                 m4,                xm5,       1
    movu                        xm5,                [r0 + r1 + 4]
    movu                        xm6,                [r0 + r1 + 6]
    vinserti128                 m5,                 m5,                xm6,       1
    pmaddwd                     m4,                m0
    pmaddwd                     m5,                m0
    phaddd                      m4,                m5                                     ; DWORD [R3D R3C R2D R2C R3B R3A R2B R2A]
    phaddd                      m3,                m4                                     ; all rows and col completed.

    mova                        m5,                [interp8_hps_shuf]
    vpermd                      m3,                m5,                  m3
    paddd                       m3,                m2
    vextracti128                xm4,               m3,                  1
    psrad                       xm3,               INTERP_SHIFT_PS
    psrad                       xm4,               INTERP_SHIFT_PS
    packssdw                    xm3,               xm3
    packssdw                    xm4,               xm4

    movq                        [r2],              xm3                                   ;row 0
    movq                        [r2 + r3],         xm4                                   ;row 1
    lea                         r0,                [r0 + r1 * 2]                         ; first loop src ->5th row(i.e 4)
    lea                         r2,                [r2 + r3 * 2]                         ; first loop dst ->5th row(i.e 4)

    sub                         r7d,               2
    jg                          .loop
    test                        r5d,               r5d
    jz                          .end

    ; Row 10
    movu                        xm3,                [r0]
    movu                        xm4,                [r0 + 2]
    vinserti128                 m3,                 m3,                 xm4,      1
    movu                        xm4,                [r0 + 4]
    movu                        xm5,                [r0 + 6]
    vinserti128                 m4,                 m4,                 xm5,      1
    pmaddwd                     m3,                m0
    pmaddwd                     m4,                m0
    phaddd                      m3,                m4

    ; Row11
    phaddd                      m3,                m4                                    ; all rows and col completed.

    mova                        m5,                [interp8_hps_shuf]
    vpermd                      m3,                m5,                  m3
    paddd                       m3,                m2
    vextracti128                xm4,               m3,                  1
    psrad                       xm3,               INTERP_SHIFT_PS
    psrad                       xm4,               INTERP_SHIFT_PS
    packssdw                    xm3,               xm3
    packssdw                    xm4,               xm4

    movq                        [r2],              xm3                                   ;row 0
.end:
    RET
%endif
%endmacro

    IPFILTER_LUMA_PS_4xN_AVX2 4
    IPFILTER_LUMA_PS_4xN_AVX2 8
    IPFILTER_LUMA_PS_4xN_AVX2 16

   %macro PROCESS_IPFILTER_LUMA_PS_8x1_AVX2 1

     %assign x 0
    %rep %1/8
    vbroadcasti128      m4, [r0 + x]
    vbroadcasti128      m5, [r0 + 8+ x]
    pshufb              m4, m3
    pshufb              m7, m5, m3
    pmaddwd             m4, m0
    pmaddwd             m7, m1
    paddd               m4, m7

    vbroadcasti128      m6, [r0 + 16 + x]
    pshufb              m5, m3
    pshufb              m6, m3
    pmaddwd             m5, m0
    pmaddwd             m6, m1
    paddd               m5, m6

    phaddd              m4, m5
    vpermq              m4, m4, q3120
    paddd               m4, m2
    vextracti128        xm5,m4, 1
    psrad               xm4, INTERP_SHIFT_PS
    psrad               xm5, INTERP_SHIFT_PS
    packssdw            xm4, xm5
    movu                [r2 + x], xm4
    %assign x x+16
     %endrep
    %endmacro

%macro IPFILTER_LUMA_PS_8xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_horiz_ps_8x%1, 4, 6, 8
    shl                 r1d, 1
    shl                 r3d, 1
    mov                 r4d, r4m
    mov                 r5d, r5m
    shl                 r4d, 4
%ifdef PIC
    lea                 r6, [h_tab_LumaCoeff]
    vpbroadcastq        m0, [r6 + r4]
    vpbroadcastq        m1, [r6 + r4 + 8]
%else
    vpbroadcastq        m0, [h_tab_LumaCoeff + r4]
    vpbroadcastq        m1, [h_tab_LumaCoeff + r4 + 8]
%endif
    mova                m3, [interp8_hpp_shuf]
    vbroadcasti128      m2, [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 6
    test                r5d, r5d
    mov                 r4d, %1
    jz                  .loop0
    lea                 r6, [r1*3]
    sub                 r0, r6
    add                 r4d, 7

.loop0:
    PROCESS_IPFILTER_LUMA_PS_8x1_AVX2 8
    add                 r2, r3
    add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif
%endmacro

    IPFILTER_LUMA_PS_8xN_AVX2 4
    IPFILTER_LUMA_PS_8xN_AVX2 8
    IPFILTER_LUMA_PS_8xN_AVX2 16
    IPFILTER_LUMA_PS_8xN_AVX2 32

INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_horiz_ps_24x32, 4, 6, 8
    add                 r1d, r1d
    add                 r3d, r3d
    mov                 r4d, r4m
    mov                 r5d, r5m
    shl                 r4d, 4
%ifdef PIC
    lea                 r6, [h_tab_LumaCoeff]
    vpbroadcastq        m0, [r6 + r4]
    vpbroadcastq        m1, [r6 + r4 + 8]
%else
    vpbroadcastq        m0, [h_tab_LumaCoeff + r4]
    vpbroadcastq        m1, [h_tab_LumaCoeff + r4 + 8]
%endif
    mova                m3, [interp8_hpp_shuf]
    vbroadcasti128      m2, [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 6
    test                r5d, r5d
    mov                 r4d, 32
    jz                  .loop0
    lea                 r6, [r1*3]
    sub                 r0, r6
    add                 r4d, 7


.loop0:
    PROCESS_IPFILTER_LUMA_PS_8x1_AVX2 24
    add                 r2, r3
    add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif


%macro PROCESS_IPFILTER_LUMA_PS_16x1_AVX2 0
    movu            m7,        [r0]
    movu            m8,        [r0 + 8]
    pshufb          m10,       m7,        m14
    pshufb          m7,                   m13
    pshufb          m11,       m8,        m14
    pshufb          m8,                   m13

    pmaddwd         m7,        m0
    pmaddwd         m10,       m1
    paddd           m7,        m10
    pmaddwd         m10,       m11,       m3
    pmaddwd         m9,        m8,        m2
    paddd           m10,       m9
    paddd           m7,        m10
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS
    movu            m9,        [r0 + 16]
    pshufb          m10,       m9,        m14
    pshufb          m9,                   m13
    pmaddwd         m8,        m0
    pmaddwd         m11,       m1
    paddd           m8,        m11
    pmaddwd         m10,       m3
    pmaddwd         m9,        m2
    paddd           m9,        m10
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        INTERP_SHIFT_PS
    packssdw        m7,        m8
    pshufb          m7,        m12
    movu            [r2],      m7
%endmacro

%macro IPFILTER_LUMA_PS_16xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_horiz_ps_16x%1, 5, 6, 15

    shl                 r1d, 1
    shl                 r3d, 1
    mov                 r4d, r4m
    mov                 r5d, r5m
    shl                 r4d, 4
%ifdef PIC
    lea                 r6, [h_tab_LumaCoeff]
    vpbroadcastd     m0,         [r6 + r4]
    vpbroadcastd     m1,         [r6 + r4 + 4]
    vpbroadcastd     m2,         [r6 + r4 + 8]
    vpbroadcastd     m3,         [r6 + r4 + 12]
%else
    vpbroadcastd     m0,         [h_tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [h_tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [h_tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [h_tab_LumaCoeff + r4 + 12]
%endif
    mova             m13,        [interp8_hpp_shuf1_load_avx512]
    mova             m14,        [interp8_hpp_shuf2_load_avx512]
    mova             m12,        [interp8_hpp_shuf1_store_avx512]
    vbroadcasti128           m4,         [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 6
    test                r5d, r5d
    mov                 r4d, %1
    jz                  .loop0
    lea                 r6, [r1*3]
    sub                 r0, r6
    add                 r4d, 7

.loop0:

     PROCESS_IPFILTER_LUMA_PS_16x1_AVX2
     lea              r0,         [r0 + r1]
    lea              r2,         [r2 + r3]
    ;add                 r2, r3
    ;add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif
%endmacro

    IPFILTER_LUMA_PS_16xN_AVX2 4
    IPFILTER_LUMA_PS_16xN_AVX2 8
    IPFILTER_LUMA_PS_16xN_AVX2 12
    IPFILTER_LUMA_PS_16xN_AVX2 16
    IPFILTER_LUMA_PS_16xN_AVX2 32
    IPFILTER_LUMA_PS_16xN_AVX2 64
%macro PROCESS_IPFILTER_LUMA_PS_32x1_AVX2 0
     PROCESS_IPFILTER_LUMA_PS_16x1_AVX2
    movu            m7,        [r0 + mmsize]
    movu            m8,        [r0 + 8+ mmsize]
    pshufb          m10,       m7,        m14
    pshufb          m7,                   m13
    pshufb          m11,       m8,        m14
    pshufb          m8,                   m13

    pmaddwd         m7,        m0
    pmaddwd         m10,       m1
    paddd           m7,        m10
    pmaddwd         m10,       m11,       m3
    pmaddwd         m9,        m8,        m2
    paddd           m10,       m9
    paddd           m7,        m10
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS
    movu            m9,        [r0 + 16+ mmsize]
    pshufb          m10,       m9,        m14
    pshufb          m9,                   m13
    pmaddwd         m8,        m0
    pmaddwd         m11,       m1
    paddd           m8,        m11
    pmaddwd         m10,       m3
    pmaddwd         m9,        m2
    paddd           m9,        m10
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        INTERP_SHIFT_PS
    packssdw        m7,        m8
    pshufb          m7,        m12
    movu            [r2+ mmsize],      m7
%endmacro

%macro IPFILTER_LUMA_PS_32xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64
cglobal interp_8tap_horiz_ps_32x%1, 5, 6, 15

    shl                 r1d, 1
    shl                 r3d, 1
    mov                 r4d, r4m
    mov                 r5d, r5m
    shl                 r4d, 4
%ifdef PIC
    lea                 r6, [h_tab_LumaCoeff]
    vpbroadcastd     m0,         [r6 + r4]
    vpbroadcastd     m1,         [r6 + r4 + 4]
    vpbroadcastd     m2,         [r6 + r4 + 8]
    vpbroadcastd     m3,         [r6 + r4 + 12]
%else
    vpbroadcastd     m0,         [h_tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [h_tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [h_tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [h_tab_LumaCoeff + r4 + 12]
%endif
    mova             m13,        [interp8_hpp_shuf1_load_avx512]
    mova             m14,        [interp8_hpp_shuf2_load_avx512]
    mova             m12,        [interp8_hpp_shuf1_store_avx512]
    vbroadcasti128           m4,         [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 6
    test                r5d, r5d
    mov                 r4d, %1
    jz                  .loop0
    lea                 r6, [r1*3]
    sub                 r0, r6
    add                 r4d, 7

.loop0:
    PROCESS_IPFILTER_LUMA_PS_32x1_AVX2
    lea              r0,         [r0 + r1]
    lea              r2,         [r2 + r3]
    ;add                 r2, r3
    ;add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif
%endmacro

    IPFILTER_LUMA_PS_32xN_AVX2 8
    IPFILTER_LUMA_PS_32xN_AVX2 16
    IPFILTER_LUMA_PS_32xN_AVX2 24
    IPFILTER_LUMA_PS_32xN_AVX2 32
    IPFILTER_LUMA_PS_32xN_AVX2 64

%macro PROCESS_IPFILTER_LUMA_PS_64x1_AVX2 0
     PROCESS_IPFILTER_LUMA_PS_16x1_AVX2
%assign x 32
%rep 3
    movu            m7,        [r0 + x]
    movu            m8,        [r0 + 8+ x]
    pshufb          m10,       m7,        m14
    pshufb          m7,                   m13
    pshufb          m11,       m8,        m14
    pshufb          m8,                   m13

    pmaddwd         m7,        m0
    pmaddwd         m10,       m1
    paddd           m7,        m10
    pmaddwd         m10,       m11,       m3
    pmaddwd         m9,        m8,        m2
    paddd           m10,       m9
    paddd           m7,        m10
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS
    movu            m9,        [r0 + 16+ x]
    pshufb          m10,       m9,        m14
    pshufb          m9,                   m13
    pmaddwd         m8,        m0
    pmaddwd         m11,       m1
    paddd           m8,        m11
    pmaddwd         m10,       m3
    pmaddwd         m9,        m2
    paddd           m9,        m10
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        INTERP_SHIFT_PS
    packssdw        m7,        m8
    pshufb          m7,        m12
    movu            [r2+ x],      m7
%assign x x+32
%endrep
%endmacro

%macro IPFILTER_LUMA_PS_64xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64
cglobal interp_8tap_horiz_ps_64x%1, 5, 6, 15

    shl                 r1d, 1
    shl                 r3d, 1
    mov                 r4d, r4m
    mov                 r5d, r5m
    shl                 r4d, 4
%ifdef PIC
    lea                 r6, [h_tab_LumaCoeff]
    vpbroadcastd     m0,         [r6 + r4]
    vpbroadcastd     m1,         [r6 + r4 + 4]
    vpbroadcastd     m2,         [r6 + r4 + 8]
    vpbroadcastd     m3,         [r6 + r4 + 12]
%else
    vpbroadcastd     m0,         [h_tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [h_tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [h_tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [h_tab_LumaCoeff + r4 + 12]
%endif
    mova             m13,        [interp8_hpp_shuf1_load_avx512]
    mova             m14,        [interp8_hpp_shuf2_load_avx512]
    mova             m12,        [interp8_hpp_shuf1_store_avx512]
    vbroadcasti128           m4,         [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 6
    test                r5d, r5d
    mov                 r4d, %1
    jz                  .loop0
    lea                 r6, [r1*3]
    sub                 r0, r6
    add                 r4d, 7

.loop0:
    PROCESS_IPFILTER_LUMA_PS_64x1_AVX2
    lea              r0,         [r0 + r1]
    lea              r2,         [r2 + r3]
    ;add                 r2, r3
    ;add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif
%endmacro

    IPFILTER_LUMA_PS_64xN_AVX2 16
    IPFILTER_LUMA_PS_64xN_AVX2 32
    IPFILTER_LUMA_PS_64xN_AVX2 48
    IPFILTER_LUMA_PS_64xN_AVX2 64

%macro IPFILTER_LUMA_PS_48xN_AVX2 1
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_horiz_ps_48x%1, 5, 9,15

    add                 r1d, r1d
    add                 r3d, r3d
    mov                 r4d, r4m
    mov                 r5d, r5m
    shl                 r4d, 6
%ifdef PIC
    lea                 r6, [h_tab_LumaCoeffV]
    movu                m0, [r6 + r4]
    movu                m1, [r6 + r4 + mmsize]
%else
    movu                m0, [h_tab_LumaCoeffV + r4]
    movu                m1, [h_tab_LumaCoeffV + r4 + mmsize]
%endif
    mova                m3, [interp8_hpp_shuf_new]
    vbroadcasti128      m2, [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 6
    test                r5d, r5d
    mov                 r4d, %1
    jz                 .loop0
    lea                 r6, [r1*3]
    sub                 r0, r6
    add                 r4d, 7

.loop0:
%assign x 0
%rep 3
    vbroadcasti128      m4, [r0 + x]
    vbroadcasti128      m5, [r0 + 4 * SIZEOF_PIXEL + x]
    pshufb              m4, m3
    pshufb              m5, m3

    pmaddwd             m4, m0
    pmaddwd             m7, m5, m1
    paddd               m4, m7
    vextracti128        xm7, m4, 1
    paddd               xm4, xm7
    paddd               xm4, xm2
    psrad               xm4, INTERP_SHIFT_PS

    vbroadcasti128      m6, [r0 + 16 + x]
    pshufb              m6, m3

    pmaddwd             m5, m0
    pmaddwd             m7, m6, m1
    paddd               m5, m7
    vextracti128        xm7, m5, 1
    paddd               xm5, xm7
    paddd               xm5, xm2
    psrad               xm5, INTERP_SHIFT_PS

    packssdw            xm4, xm5
    movu                [r2 + x], xm4

    vbroadcasti128      m5, [r0 + 24 + x]
    pshufb              m5, m3

    pmaddwd             m6, m0
    pmaddwd             m7, m5, m1
    paddd               m6, m7
    vextracti128        xm7, m6, 1
    paddd               xm6, xm7
    paddd               xm6, xm2
    psrad               xm6, INTERP_SHIFT_PS

    vbroadcasti128      m7, [r0 + 32 + x]
    pshufb              m7, m3

    pmaddwd             m5, m0
    pmaddwd             m7, m1
    paddd               m5, m7
    vextracti128        xm7, m5, 1
    paddd               xm5, xm7
    paddd               xm5, xm2
    psrad               xm5, INTERP_SHIFT_PS

    packssdw            xm6, xm5
    movu                [r2 + 16 + x], xm6

%assign x x+32
%endrep

    add                 r2, r3
    add                 r0, r1
    dec                 r4d
    jnz                .loop0
    RET
%endif
%endmacro
      IPFILTER_LUMA_PS_48xN_AVX2 64
INIT_YMM avx2
%if ARCH_X86_64 == 1
cglobal interp_8tap_horiz_ps_12x16, 4, 6, 8
    add                 r1d, r1d
    add                 r3d, r3d
    mov                 r4d, r4m
    mov                 r5d, r5m
    shl                 r4d, 4
%ifdef PIC
    lea                 r6, [h_tab_LumaCoeff]
    vpbroadcastq        m0, [r6 + r4]
    vpbroadcastq        m1, [r6 + r4 + 8]
%else
    vpbroadcastq        m0, [h_tab_LumaCoeff + r4]
    vpbroadcastq        m1, [h_tab_LumaCoeff + r4 + 8]
%endif
    mova                m3, [interp8_hpp_shuf]
    vbroadcasti128      m2, [INTERP_OFFSET_PS]

    ; register map
    ; m0 , m1 interpolate coeff

    sub                 r0, 6
    test                r5d, r5d
    mov                 r4d, 16
    jz                  .loop0
    lea                 r6, [r1*3]
    sub                 r0, r6
    add                 r4d, 7

.loop0:
    vbroadcasti128      m4, [r0]
    vbroadcasti128      m5, [r0 + 8]
    pshufb              m4, m3
    pshufb              m7, m5, m3
    pmaddwd             m4, m0
    pmaddwd             m7, m1
    paddd               m4, m7

    vbroadcasti128      m6, [r0 + 16]
    pshufb              m5, m3
    pshufb              m7, m6, m3
    pmaddwd             m5, m0
    pmaddwd             m7, m1
    paddd               m5, m7

    phaddd              m4, m5
    vpermq              m4, m4, q3120
    paddd               m4, m2
    vextracti128        xm5,m4, 1
    psrad               xm4, INTERP_SHIFT_PS
    psrad               xm5, INTERP_SHIFT_PS
    packssdw            xm4, xm5
    movu                [r2], xm4

    vbroadcasti128      m5, [r0 + 24]
    pshufb              m6, m3
    pshufb              m5, m3
    pmaddwd             m6, m0
    pmaddwd             m5, m1
    paddd               m6, m5

    phaddd              m6, m6
    vpermq              m6, m6, q3120
    paddd               xm6, xm2
    psrad               xm6, INTERP_SHIFT_PS
    packssdw            xm6, xm6
    movq                [r2 + 16], xm6

    add                 r2, r3
    add                 r0, r1
    dec                 r4d
    jnz                 .loop0
    RET
%endif
