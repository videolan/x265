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
    %define INTERP_OFFSET_SP        pd_524800
%elif BIT_DEPTH == 12
    %define INTERP_SHIFT_PS         4
    %define INTERP_OFFSET_PS        pd_n131072
    %define INTERP_SHIFT_SP         8
    %define INTERP_OFFSET_SP        pd_524416
%else
    %error Unsupport bit depth!
%endif


SECTION_RODATA 64

tab_c_524800:     times 4 dd 524800
tab_c_n8192:      times 8 dw -8192
pd_524800:        times 8 dd 524800

tab_ChromaCoeff:  dw  0, 64,  0,  0
                  dw -2, 58, 10, -2
                  dw -4, 54, 16, -2
                  dw -6, 46, 28, -4
                  dw -4, 36, 36, -4
                  dw -4, 28, 46, -6
                  dw -2, 16, 54, -4
                  dw -2, 10, 58, -2
				
tab_LumaCoeff:    dw   0, 0,  0,  64,  0,   0,  0,  0
                  dw  -1, 4, -10, 58,  17, -5,  1,  0
                  dw  -1, 4, -11, 40,  40, -11, 4, -1
                  dw   0, 1, -5,  17,  58, -10, 4, -1

ALIGN 64
tab_LumaCoeffH_avx512:
                  times 4 dw  0, 0,  0,  64,  0,   0,  0,  0
                  times 4 dw  -1, 4, -10, 58,  17, -5,  1,  0
                  times 4 dw  -1, 4, -11, 40,  40, -11, 4, -1
                  times 4 dw   0, 1, -5,  17,  58, -10, 4, -1

ALIGN 32
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
				  
ALIGN 64
const tab_ChromaCoeffV_avx512,  times 16 dw 0, 64
                                times 16 dw 0, 0

                                times 16 dw -2, 58
                                times 16 dw 10, -2

                                times 16 dw -4, 54
                                times 16 dw 16, -2

                                times 16 dw -6, 46
                                times 16 dw 28, -4

                                times 16 dw -4, 36
                                times 16 dw 36, -4

                                times 16 dw -4, 28
                                times 16 dw 46, -6

                                times 16 dw -2, 16
                                times 16 dw 54, -4

                                times 16 dw -2, 10
                                times 16 dw 58, -2

ALIGN 64
tab_LumaCoeffVer_avx512: times 16 dw 0, 0
                         times 16 dw 0, 64
                         times 16 dw 0, 0
                         times 16 dw 0, 0

                         times 16 dw -1, 4
                         times 16 dw -10, 58
                         times 16 dw 17, -5
                         times 16 dw 1, 0

                         times 16 dw -1, 4
                         times 16 dw -11, 40
                         times 16 dw 40, -11
                         times 16 dw 4, -1

                         times 16 dw 0, 1
                         times 16 dw -5, 17
                         times 16 dw 58, -10
                         times 16 dw 4, -1

ALIGN 64
const interp8_hpp_shuf1_load_avx512, times 4 db 0, 1, 2, 3, 4, 5, 6, 7, 2, 3, 4, 5, 6, 7, 8, 9

ALIGN 64
const interp8_hpp_shuf2_load_avx512, times 4 db 4, 5, 6, 7, 8, 9, 10, 11, 6, 7, 8, 9, 10, 11, 12, 13

ALIGN 64
const interp8_hpp_shuf1_store_avx512, times 4 db 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15
 
SECTION .text
cextern pd_8
cextern pd_32
cextern pw_pixel_max
cextern pd_524416
cextern pd_n32768
cextern pd_n131072
cextern pw_2000
cextern idct8_shuf2

%macro PROCESS_LUMA_VER_W4_4R_sse2 0
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
; void interp_8tap_vert_%1_%2x%3(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;--------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_LUMA_sse2 3
INIT_XMM sse2
cglobal interp_8tap_vert_%1_%2x%3, 5, 7, 8

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

%ifidn %1,pp
    mova      m7, [INTERP_OFFSET_PP]
%define SHIFT 6
%elifidn %1,ps
    mova      m7, [INTERP_OFFSET_PS]
  %if BIT_DEPTH == 10
    %define SHIFT 2
  %elif BIT_DEPTH == 12
    %define SHIFT 4
  %endif
%endif

    mov         r4d, %3/4
.loopH:
%assign x 0
%rep %2/4
    PROCESS_LUMA_VER_W4_4R_sse2

    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7

    psrad     m0, SHIFT
    psrad     m1, SHIFT
    psrad     m2, SHIFT
    psrad     m3, SHIFT

    packssdw  m0, m1
    packssdw  m2, m3

%ifidn %1,pp
    pxor      m1, m1
    CLIPW2    m0, m2, m1, [pw_pixel_max]
%endif

    movh      [r2 + x], m0
    movhps    [r2 + r3 + x], m0
    lea       r5, [r2 + 2 * r3]
    movh      [r5 + x], m2
    movhps    [r5 + r3 + x], m2

    lea       r5, [8 * r1 - 2 * 4]
    sub       r0, r5
%assign x x+8
%endrep

    lea       r0, [r0 + 4 * r1 - 2 * %2]
    lea       r2, [r2 + 4 * r3]

    dec         r4d
    jnz       .loopH

    RET
%endmacro

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_pp_%2x%3(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
%if ARCH_X86_64
    FILTER_VER_LUMA_sse2 pp, 4, 4
    FILTER_VER_LUMA_sse2 pp, 8, 8
    FILTER_VER_LUMA_sse2 pp, 8, 4
    FILTER_VER_LUMA_sse2 pp, 4, 8
    FILTER_VER_LUMA_sse2 pp, 16, 16
    FILTER_VER_LUMA_sse2 pp, 16, 8
    FILTER_VER_LUMA_sse2 pp, 8, 16
    FILTER_VER_LUMA_sse2 pp, 16, 12
    FILTER_VER_LUMA_sse2 pp, 12, 16
    FILTER_VER_LUMA_sse2 pp, 16, 4
    FILTER_VER_LUMA_sse2 pp, 4, 16
    FILTER_VER_LUMA_sse2 pp, 32, 32
    FILTER_VER_LUMA_sse2 pp, 32, 16
    FILTER_VER_LUMA_sse2 pp, 16, 32
    FILTER_VER_LUMA_sse2 pp, 32, 24
    FILTER_VER_LUMA_sse2 pp, 24, 32
    FILTER_VER_LUMA_sse2 pp, 32, 8
    FILTER_VER_LUMA_sse2 pp, 8, 32
    FILTER_VER_LUMA_sse2 pp, 64, 64
    FILTER_VER_LUMA_sse2 pp, 64, 32
    FILTER_VER_LUMA_sse2 pp, 32, 64
    FILTER_VER_LUMA_sse2 pp, 64, 48
    FILTER_VER_LUMA_sse2 pp, 48, 64
    FILTER_VER_LUMA_sse2 pp, 64, 16
    FILTER_VER_LUMA_sse2 pp, 16, 64

;-------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert_ps_%2x%3(pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int coeffIdx)
;-------------------------------------------------------------------------------------------------------------
    FILTER_VER_LUMA_sse2 ps, 4, 4
    FILTER_VER_LUMA_sse2 ps, 8, 8
    FILTER_VER_LUMA_sse2 ps, 8, 4
    FILTER_VER_LUMA_sse2 ps, 4, 8
    FILTER_VER_LUMA_sse2 ps, 16, 16
    FILTER_VER_LUMA_sse2 ps, 16, 8
    FILTER_VER_LUMA_sse2 ps, 8, 16
    FILTER_VER_LUMA_sse2 ps, 16, 12
    FILTER_VER_LUMA_sse2 ps, 12, 16
    FILTER_VER_LUMA_sse2 ps, 16, 4
    FILTER_VER_LUMA_sse2 ps, 4, 16
    FILTER_VER_LUMA_sse2 ps, 32, 32
    FILTER_VER_LUMA_sse2 ps, 32, 16
    FILTER_VER_LUMA_sse2 ps, 16, 32
    FILTER_VER_LUMA_sse2 ps, 32, 24
    FILTER_VER_LUMA_sse2 ps, 24, 32
    FILTER_VER_LUMA_sse2 ps, 32, 8
    FILTER_VER_LUMA_sse2 ps, 8, 32
    FILTER_VER_LUMA_sse2 ps, 64, 64
    FILTER_VER_LUMA_sse2 ps, 64, 32
    FILTER_VER_LUMA_sse2 ps, 32, 64
    FILTER_VER_LUMA_sse2 ps, 64, 48
    FILTER_VER_LUMA_sse2 ps, 48, 64
    FILTER_VER_LUMA_sse2 ps, 64, 16
    FILTER_VER_LUMA_sse2 ps, 16, 64
%endif

;-----------------------------------------------------------------------------
;p2s and p2s_aligned avx512 code start
;-----------------------------------------------------------------------------
%macro P2S_64x4_AVX512 0
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    movu       m2, [r0 + r1 * 2]
    movu       m3, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4
    movu       [r2], m0
    movu       [r2 + r3], m1
    movu       [r2 + r3 * 2], m2
    movu       [r2 + r4], m3

    movu       m0, [r0 + mmsize]
    movu       m1, [r0 + r1 + mmsize]
    movu       m2, [r0 + r1 * 2 + mmsize]
    movu       m3, [r0 + r5 + mmsize]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4
    movu       [r2 + mmsize], m0
    movu       [r2 + r3 + mmsize], m1
    movu       [r2 + r3 * 2 + mmsize], m2
    movu       [r2 + r4 + mmsize], m3
%endmacro

%macro P2S_ALIGNED_64x4_AVX512 0
    mova       m0, [r0]
    mova       m1, [r0 + r1]
    mova       m2, [r0 + r1 * 2]
    mova       m3, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4
    mova       [r2], m0
    mova       [r2 + r3], m1
    mova       [r2 + r3 * 2], m2
    mova       [r2 + r4], m3

    mova       m0, [r0 + mmsize]
    mova       m1, [r0 + r1 + mmsize]
    mova       m2, [r0 + r1 * 2 + mmsize]
    mova       m3, [r0 + r5 + mmsize]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4
    mova       [r2 + mmsize], m0
    mova       [r2 + r3 + mmsize], m1
    mova       [r2 + r3 * 2 + mmsize], m2
    mova       [r2 + r4 + mmsize], m3
%endmacro

%macro P2S_32x4_AVX512 0
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    movu       m2, [r0 + r1 * 2]
    movu       m3, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4
    movu       [r2], m0
    movu       [r2 + r3], m1
    movu       [r2 + r3 * 2], m2
    movu       [r2 + r4], m3
%endmacro

%macro P2S_ALIGNED_32x4_AVX512 0
    mova       m0, [r0]
    mova       m1, [r0 + r1]
    mova       m2, [r0 + r1 * 2]
    mova       m3, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4
    mova       [r2], m0
    mova       [r2 + r3], m1
    mova       [r2 + r3 * 2], m2
    mova       [r2 + r4], m3
%endmacro

%macro P2S_48x4_AVX512 0
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    movu       m2, [r0 + r1 * 2]
    movu       m3, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4
    movu       [r2], m0
    movu       [r2 + r3], m1
    movu       [r2 + r3 * 2], m2
    movu       [r2 + r4], m3

    movu       ym0, [r0 + mmsize]
    movu       ym1, [r0 + r1 + mmsize]
    movu       ym2, [r0 + r1 * 2 + mmsize]
    movu       ym3, [r0 + r5 + mmsize]
    psllw      ym0, (14 - BIT_DEPTH)
    psllw      ym1, (14 - BIT_DEPTH)
    psllw      ym2, (14 - BIT_DEPTH)
    psllw      ym3, (14 - BIT_DEPTH)
    psubw      ym0, ym4
    psubw      ym1, ym4
    psubw      ym2, ym4
    psubw      ym3, ym4
    movu       [r2 + mmsize], ym0
    movu       [r2 + r3 + mmsize], ym1
    movu       [r2 + r3 * 2 + mmsize], ym2
    movu       [r2 + r4 + mmsize], ym3
%endmacro

%macro P2S_ALIGNED_48x4_AVX512 0
    mova       m0, [r0]
    mova       m1, [r0 + r1]
    mova       m2, [r0 + r1 * 2]
    mova       m3, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4
    mova       [r2], m0
    mova       [r2 + r3], m1
    mova       [r2 + r3 * 2], m2
    mova       [r2 + r4], m3

    mova       ym0, [r0 + mmsize]
    mova       ym1, [r0 + r1 + mmsize]
    mova       ym2, [r0 + r1 * 2 + mmsize]
    mova       ym3, [r0 + r5 + mmsize]
    psllw      ym0, (14 - BIT_DEPTH)
    psllw      ym1, (14 - BIT_DEPTH)
    psllw      ym2, (14 - BIT_DEPTH)
    psllw      ym3, (14 - BIT_DEPTH)
    psubw      ym0, ym4
    psubw      ym1, ym4
    psubw      ym2, ym4
    psubw      ym3, ym4
    mova       [r2 + mmsize], ym0
    mova       [r2 + r3 + mmsize], ym1
    mova       [r2 + r3 * 2 + mmsize], ym2
    mova       [r2 + r4 + mmsize], ym3
%endmacro

;-----------------------------------------------------------------------------
; void filterPixelToShort(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride)
;-----------------------------------------------------------------------------
INIT_ZMM avx512
cglobal filterPixelToShort_64x16, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 3
    P2S_64x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_64x4_AVX512
    RET


INIT_ZMM avx512
cglobal filterPixelToShort_64x32, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 7
    P2S_64x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_64x48, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 11
    P2S_64x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_64x64, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 15
    P2S_64x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_32x8, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
    P2S_32x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
    P2S_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_32x16, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 3
    P2S_32x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_32x24, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 5
    P2S_32x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_32x32, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 7
    P2S_32x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_32x48, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 11
    P2S_32x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_32x64, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 15
    P2S_32x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_48x64, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 15
    P2S_48x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_48x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_64x16, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 3
    P2S_ALIGNED_64x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_ALIGNED_64x4_AVX512
    RET


INIT_ZMM avx512
cglobal filterPixelToShort_aligned_64x32, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 7
    P2S_ALIGNED_64x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_ALIGNED_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_64x48, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 11
    P2S_ALIGNED_64x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_ALIGNED_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_64x64, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 15
    P2S_ALIGNED_64x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_ALIGNED_64x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_32x8, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
    P2S_ALIGNED_32x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
    P2S_ALIGNED_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_32x16, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 3
    P2S_ALIGNED_32x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_ALIGNED_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_32x24, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 5
    P2S_ALIGNED_32x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_ALIGNED_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_32x32, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 7
    P2S_ALIGNED_32x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_ALIGNED_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_32x48, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 11
    P2S_ALIGNED_32x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_ALIGNED_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_32x64, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 15
    P2S_ALIGNED_32x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_ALIGNED_32x4_AVX512
    RET

INIT_ZMM avx512
cglobal filterPixelToShort_aligned_48x64, 4, 6, 5
    add        r1d, r1d
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load constant
    vbroadcasti32x8    m4, [pw_2000]
%rep 15
    P2S_ALIGNED_48x4_AVX512
    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    P2S_ALIGNED_48x4_AVX512
    RET
;-----------------------------------------------------------------------------------------------------------------------------
;p2s and p2s_aligned avx512 code end
;-----------------------------------------------------------------------------------------------------------------------------

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

    mova      m7, [INTERP_OFFSET_PP]

    mov       dword [rsp], %2/4
.loopH:
    mov       r4d, (%1/4)
.loopW:
    PROCESS_LUMA_VER_W4_4R

    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7

    psrad     m0, INTERP_SHIFT_PP
    psrad     m1, INTERP_SHIFT_PP
    psrad     m2, INTERP_SHIFT_PP
    psrad     m3, INTERP_SHIFT_PP

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
    vbroadcasti128  m6, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m6, [INTERP_OFFSET_PS]
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
    psrad           m0, INTERP_SHIFT_PP
    psrad           m2, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m0, INTERP_SHIFT_SP
    psrad           m2, INTERP_SHIFT_SP
%else
    psrad           m0, INTERP_SHIFT_PS
    psrad           m2, INTERP_SHIFT_PS
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
    vbroadcasti128  m11, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m11, [INTERP_OFFSET_PS]
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
    psrad           m0, INTERP_SHIFT_PP
    psrad           m1, INTERP_SHIFT_PP
    psrad           m2, INTERP_SHIFT_PP
    psrad           m3, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m0, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
    psrad           m2, INTERP_SHIFT_SP
    psrad           m3, INTERP_SHIFT_SP
%else
    psrad           m0, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
    psrad           m2, INTERP_SHIFT_PS
    psrad           m3, INTERP_SHIFT_PS
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
    psrad           m4, INTERP_SHIFT_PP
    psrad           m5, INTERP_SHIFT_PP
    psrad           m6, INTERP_SHIFT_PP
    psrad           m7, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m4, INTERP_SHIFT_SP
    psrad           m5, INTERP_SHIFT_SP
    psrad           m6, INTERP_SHIFT_SP
    psrad           m7, INTERP_SHIFT_SP
%else
    psrad           m4, INTERP_SHIFT_PS
    psrad           m5, INTERP_SHIFT_PS
    psrad           m6, INTERP_SHIFT_PS
    psrad           m7, INTERP_SHIFT_PS
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
    psrad           m0, INTERP_SHIFT_PP
    psrad           m1, INTERP_SHIFT_PP
    psrad           m2, INTERP_SHIFT_PP
    psrad           m3, INTERP_SHIFT_PP
    psrad           m4, INTERP_SHIFT_PP
    psrad           m5, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m0, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
    psrad           m2, INTERP_SHIFT_SP
    psrad           m3, INTERP_SHIFT_SP
    psrad           m4, INTERP_SHIFT_SP
    psrad           m5, INTERP_SHIFT_SP
%else
    psrad           m0, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
    psrad           m2, INTERP_SHIFT_PS
    psrad           m3, INTERP_SHIFT_PS
    psrad           m4, INTERP_SHIFT_PS
    psrad           m5, INTERP_SHIFT_PS
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
    psrad           m6, INTERP_SHIFT_PP
    psrad           m7, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m6, INTERP_SHIFT_SP
    psrad           m7, INTERP_SHIFT_SP
%else
    psrad           m6, INTERP_SHIFT_PS
    psrad           m7, INTERP_SHIFT_PS
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
    psrad           m8, INTERP_SHIFT_PP
    psrad           m9, INTERP_SHIFT_PP
    psrad           m10, INTERP_SHIFT_PP
    psrad           m11, INTERP_SHIFT_PP
    psrad           m12, INTERP_SHIFT_PP
    psrad           m13, INTERP_SHIFT_PP
    psrad           m0, INTERP_SHIFT_PP
    psrad           m1, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m8, INTERP_SHIFT_SP
    psrad           m9, INTERP_SHIFT_SP
    psrad           m10, INTERP_SHIFT_SP
    psrad           m11, INTERP_SHIFT_SP
    psrad           m12, INTERP_SHIFT_SP
    psrad           m13, INTERP_SHIFT_SP
    psrad           m0, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
%else
    psrad           m8, INTERP_SHIFT_PS
    psrad           m9, INTERP_SHIFT_PS
    psrad           m10, INTERP_SHIFT_PS
    psrad           m11, INTERP_SHIFT_PS
    psrad           m12, INTERP_SHIFT_PS
    psrad           m13, INTERP_SHIFT_PS
    psrad           m0, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
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
    vbroadcasti128  m14, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m14, [INTERP_OFFSET_PS]
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
    vbroadcasti128  m14, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m14, [INTERP_OFFSET_PS]
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
    vbroadcasti128  m14, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m14, [INTERP_OFFSET_PS]
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
    psrad           m0, INTERP_SHIFT_PP
    psrad           m1, INTERP_SHIFT_PP
    psrad           m2, INTERP_SHIFT_PP
    psrad           m3, INTERP_SHIFT_PP
    psrad           m4, INTERP_SHIFT_PP
    psrad           m5, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m0, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
    psrad           m2, INTERP_SHIFT_SP
    psrad           m3, INTERP_SHIFT_SP
    psrad           m4, INTERP_SHIFT_SP
    psrad           m5, INTERP_SHIFT_SP
%else
    psrad           m0, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
    psrad           m2, INTERP_SHIFT_PS
    psrad           m3, INTERP_SHIFT_PS
    psrad           m4, INTERP_SHIFT_PS
    psrad           m5, INTERP_SHIFT_PS
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
    psrad           m6, INTERP_SHIFT_PP
    psrad           m7, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m6, INTERP_SHIFT_SP
    psrad           m7, INTERP_SHIFT_SP
%else
    psrad           m6, INTERP_SHIFT_PS
    psrad           m7, INTERP_SHIFT_PS
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
    psrad           m8, INTERP_SHIFT_PP
    psrad           m9, INTERP_SHIFT_PP
    psrad           m10, INTERP_SHIFT_PP
    psrad           m11, INTERP_SHIFT_PP
    psrad           m12, INTERP_SHIFT_PP
    psrad           m13, INTERP_SHIFT_PP
    psrad           m0, INTERP_SHIFT_PP
    psrad           m1, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m8, INTERP_SHIFT_SP
    psrad           m9, INTERP_SHIFT_SP
    psrad           m10, INTERP_SHIFT_SP
    psrad           m11, INTERP_SHIFT_SP
    psrad           m12, INTERP_SHIFT_SP
    psrad           m13, INTERP_SHIFT_SP
    psrad           m0, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
%else
    psrad           m8, INTERP_SHIFT_PS
    psrad           m9, INTERP_SHIFT_PS
    psrad           m10, INTERP_SHIFT_PS
    psrad           m11, INTERP_SHIFT_PS
    psrad           m12, INTERP_SHIFT_PS
    psrad           m13, INTERP_SHIFT_PS
    psrad           m0, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
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
    psrad           m0, INTERP_SHIFT_PP
    psrad           m1, INTERP_SHIFT_PP
    psrad           m2, INTERP_SHIFT_PP
    psrad           m3, INTERP_SHIFT_PP
    psrad           m4, INTERP_SHIFT_PP
    psrad           m5, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m0, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
    psrad           m2, INTERP_SHIFT_SP
    psrad           m3, INTERP_SHIFT_SP
    psrad           m4, INTERP_SHIFT_SP
    psrad           m5, INTERP_SHIFT_SP
%else
    psrad           m0, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
    psrad           m2, INTERP_SHIFT_PS
    psrad           m3, INTERP_SHIFT_PS
    psrad           m4, INTERP_SHIFT_PS
    psrad           m5, INTERP_SHIFT_PS
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
    psrad           m6, INTERP_SHIFT_PP
    psrad           m7, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m6, INTERP_SHIFT_SP
    psrad           m7, INTERP_SHIFT_SP
%else
    psrad           m6, INTERP_SHIFT_PS
    psrad           m7, INTERP_SHIFT_PS
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
    vbroadcasti128  m11, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m11, [INTERP_OFFSET_PS]
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
    vbroadcasti128  m14, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m14, [INTERP_OFFSET_PS]
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
    psrad           m0, INTERP_SHIFT_PP
    psrad           m1, INTERP_SHIFT_PP
    psrad           m2, INTERP_SHIFT_PP
    psrad           m3, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m0, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
    psrad           m2, INTERP_SHIFT_SP
    psrad           m3, INTERP_SHIFT_SP
%else
    psrad           m0, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
    psrad           m2, INTERP_SHIFT_PS
    psrad           m3, INTERP_SHIFT_PS
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
cglobal interp_8tap_vert_%1_16x4, 4, 7, 8, 0-gprsize
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
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
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
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
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
    vbroadcasti128  m14, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m14, [INTERP_OFFSET_PS]
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
    psrad           m0, INTERP_SHIFT_PP
    psrad           m1, INTERP_SHIFT_PP
    psrad           m2, INTERP_SHIFT_PP
    psrad           m3, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m0, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
    psrad           m2, INTERP_SHIFT_SP
    psrad           m3, INTERP_SHIFT_SP
%else
    psrad           m0, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
    psrad           m2, INTERP_SHIFT_PS
    psrad           m3, INTERP_SHIFT_PS
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
    psrad           m4, INTERP_SHIFT_PP
    psrad           m5, INTERP_SHIFT_PP
    psrad           m6, INTERP_SHIFT_PP
    psrad           m7, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m4, INTERP_SHIFT_SP
    psrad           m5, INTERP_SHIFT_SP
    psrad           m6, INTERP_SHIFT_SP
    psrad           m7, INTERP_SHIFT_SP
%else
    psrad           m4, INTERP_SHIFT_PS
    psrad           m5, INTERP_SHIFT_PS
    psrad           m6, INTERP_SHIFT_PS
    psrad           m7, INTERP_SHIFT_PS
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
    psrad           m8, INTERP_SHIFT_PP
    psrad           m9, INTERP_SHIFT_PP
    psrad           m10, INTERP_SHIFT_PP
    psrad           m11, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m8, INTERP_SHIFT_SP
    psrad           m9, INTERP_SHIFT_SP
    psrad           m10, INTERP_SHIFT_SP
    psrad           m11, INTERP_SHIFT_SP
%else
    psrad           m8, INTERP_SHIFT_PS
    psrad           m9, INTERP_SHIFT_PS
    psrad           m10, INTERP_SHIFT_PS
    psrad           m11, INTERP_SHIFT_PS
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
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
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
    psrad           m0, INTERP_SHIFT_PP
    psrad           m2, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m0, INTERP_SHIFT_SP
    psrad           m2, INTERP_SHIFT_SP
%else
    psrad           m0, INTERP_SHIFT_PS
    psrad           m2, INTERP_SHIFT_PS
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
    psrad           m4, INTERP_SHIFT_PP
    psrad           m1, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m4, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
%else
    psrad           m4, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
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
    psrad           m0, INTERP_SHIFT_PP
    psrad           m2, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m0, INTERP_SHIFT_SP
    psrad           m2, INTERP_SHIFT_SP
%else
    psrad           m0, INTERP_SHIFT_PS
    psrad           m2, INTERP_SHIFT_PS
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
    psrad           m4, INTERP_SHIFT_PP
    psrad           m1, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m4, INTERP_SHIFT_SP
    psrad           m1, INTERP_SHIFT_SP
%else
    psrad           m4, INTERP_SHIFT_PS
    psrad           m1, INTERP_SHIFT_PS
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
    psrad           m6, INTERP_SHIFT_PP
    psrad           m5, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m6, INTERP_SHIFT_SP
    psrad           m5, INTERP_SHIFT_SP
%else
    psrad           m6, INTERP_SHIFT_PS
    psrad           m5, INTERP_SHIFT_PS
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
    psrad           m0, INTERP_SHIFT_PP
    psrad           m3, INTERP_SHIFT_PP
%elifidn %1, sp
    psrad           m0, INTERP_SHIFT_SP
    psrad           m3, INTERP_SHIFT_SP
%else
    psrad           m0, INTERP_SHIFT_PS
    psrad           m3, INTERP_SHIFT_PS
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
    vbroadcasti128  m7, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m7, [INTERP_OFFSET_PS]
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
    vbroadcasti128  m14, [INTERP_OFFSET_SP]
%else
    vbroadcasti128  m14, [INTERP_OFFSET_PS]
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

    mova      m7, [INTERP_OFFSET_PS]

    mov       dword [rsp], %2/4
.loopH:
    mov       r4d, (%1/4)
.loopW:
    PROCESS_LUMA_VER_W4_4R

    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7

    psrad     m0, INTERP_SHIFT_PS
    psrad     m1, INTERP_SHIFT_PS
    psrad     m2, INTERP_SHIFT_PS
    psrad     m3, INTERP_SHIFT_PS

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

    mova      m7, [INTERP_OFFSET_SP]

    mov       dword [rsp], %2/4
.loopH:
    mov       r4d, (%1/4)
.loopW:
    PROCESS_LUMA_VER_W4_4R

    paddd     m0, m7
    paddd     m1, m7
    paddd     m2, m7
    paddd     m3, m7

    psrad     m0, INTERP_SHIFT_SP
    psrad     m1, INTERP_SHIFT_SP
    psrad     m2, INTERP_SHIFT_SP
    psrad     m3, INTERP_SHIFT_SP

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
%macro P2S_H_2xN 1
INIT_XMM sse4
cglobal filterPixelToShort_2x%1, 3, 6, 2
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r1 * 3]
    lea        r5, [r3 * 3]

    ; load constant
    mova       m1, [pw_2000]

%rep %1/4
    movd       m0, [r0]
    movhps     m0, [r0 + r1]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m1

    movd       [r2 + r3 * 0], m0
    pextrd     [r2 + r3 * 1], m0, 2

    movd       m0, [r0 + r1 * 2]
    movhps     m0, [r0 + r4]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m1

    movd       [r2 + r3 * 2], m0
    pextrd     [r2 + r5], m0, 2

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]
%endrep
    RET
%endmacro
P2S_H_2xN 4
P2S_H_2xN 8
P2S_H_2xN 16

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
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m1
    movh       [r2 + r3 * 0], m0
    movhps     [r2 + r3 * 1], m0

    movh       m0, [r0 + r1 * 2]
    movhps     m0, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
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
    psllw      m0, (14 - BIT_DEPTH)
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

.loop:
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movh       [r2 + r3 * 0], m0
    pextrd     [r2 + r3 * 0 + 8], m0, 2
    movh       [r2 + r3 * 1], m1
    pextrd     [r2 + r3 * 1 + 8], m1, 2

    movu       m0, [r0 + r1 * 2]
    movu       m1, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
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

.loop:
    movu       m0, [r0]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m1
    movu       [r2 + r3 * 0], m0

    movu       m0, [r0 + r1]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m1
    movu       [r2 + r3 * 1], m0

    movu       m0, [r0 + r1 * 2]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m1
    movu       [r2 + r3 * 2], m0

    movu       m0, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
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

    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, [pw_2000]
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

    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m3
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m1, m3
    psllw      m2, (14 - BIT_DEPTH)
    psubw      m2, m3

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1
    movu       [r2 + r3 * 2], m2

    movu       m0, [r0 + r4]
    movu       m1, [r0 + r1 * 4]
    movu       m2, [r0 + r5 ]

    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m3
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m1, m3
    psllw      m2, (14 - BIT_DEPTH)
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

.loop:
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m2
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m1, m2

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1

    movu       m0, [r0 + r1 * 2]
    movu       m1, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m2
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m1, m2

    movu       [r2 + r3 * 2], m0
    movu       [r2 + r4], m1

    movu       m0, [r0 + 16]
    movu       m1, [r0 + r1 + 16]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m2
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m1, m2

    movu       [r2 + r3 * 0 + 16], m0
    movu       [r2 + r3 * 1 + 16], m1

    movu       m0, [r0 + r1 * 2 + 16]
    movu       m1, [r0 + r5 + 16]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m2
    psllw      m1, (14 - BIT_DEPTH)
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
%macro P2S_H_16xN_avx2 1
INIT_YMM avx2
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

.loop:
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m2
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m1, m2

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1

    movu       m0, [r0 + r1 * 2]
    movu       m1, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m2
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m1, m2

    movu       [r2 + r3 * 2], m0
    movu       [r2 + r4], m1

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET
%endmacro
P2S_H_16xN_avx2 16
P2S_H_16xN_avx2 4
P2S_H_16xN_avx2 8
P2S_H_16xN_avx2 12
P2S_H_16xN_avx2 32
P2S_H_16xN_avx2 64
P2S_H_16xN_avx2 24

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

.loop:
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    movu       m2, [r0 + r1 * 2]
    movu       m3, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1
    movu       [r2 + r3 * 2], m2
    movu       [r2 + r4], m3

    movu       m0, [r0 + 16]
    movu       m1, [r0 + r1 + 16]
    movu       m2, [r0 + r1 * 2 + 16]
    movu       m3, [r0 + r5 + 16]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 16], m0
    movu       [r2 + r3 * 1 + 16], m1
    movu       [r2 + r3 * 2 + 16], m2
    movu       [r2 + r4 + 16], m3

    movu       m0, [r0 + 32]
    movu       m1, [r0 + r1 + 32]
    movu       m2, [r0 + r1 * 2 + 32]
    movu       m3, [r0 + r5 + 32]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 32], m0
    movu       [r2 + r3 * 1 + 32], m1
    movu       [r2 + r3 * 2 + 32], m2
    movu       [r2 + r4 + 32], m3

    movu       m0, [r0 + 48]
    movu       m1, [r0 + r1 + 48]
    movu       m2, [r0 + r1 * 2 + 48]
    movu       m3, [r0 + r5 + 48]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
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
%macro P2S_H_32xN_avx2 1
INIT_YMM avx2
cglobal filterPixelToShort_32x%1, 3, 7, 3
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load height
    mov        r6d, %1/4

    ; load constant
    mova       m2, [pw_2000]

.loop:
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m2
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m1, m2

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1

    movu       m0, [r0 + r1 * 2]
    movu       m1, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 2], m0
    movu       [r2 + r4], m1

    movu       m0, [r0 + 32]
    movu       m1, [r0 + r1 + 32]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 0 + 32], m0
    movu       [r2 + r3 * 1 + 32], m1

    movu       m0, [r0 + r1 * 2 + 32]
    movu       m1, [r0 + r5 + 32]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 2 + 32], m0
    movu       [r2 + r4 + 32], m1

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET
%endmacro
P2S_H_32xN_avx2 32
P2S_H_32xN_avx2 8
P2S_H_32xN_avx2 16
P2S_H_32xN_avx2 24
P2S_H_32xN_avx2 64
P2S_H_32xN_avx2 48

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

.loop:
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    movu       m2, [r0 + r1 * 2]
    movu       m3, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1
    movu       [r2 + r3 * 2], m2
    movu       [r2 + r4], m3

    movu       m0, [r0 + 16]
    movu       m1, [r0 + r1 + 16]
    movu       m2, [r0 + r1 * 2 + 16]
    movu       m3, [r0 + r5 + 16]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 16], m0
    movu       [r2 + r3 * 1 + 16], m1
    movu       [r2 + r3 * 2 + 16], m2
    movu       [r2 + r4 + 16], m3

    movu       m0, [r0 + 32]
    movu       m1, [r0 + r1 + 32]
    movu       m2, [r0 + r1 * 2 + 32]
    movu       m3, [r0 + r5 + 32]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 32], m0
    movu       [r2 + r3 * 1 + 32], m1
    movu       [r2 + r3 * 2 + 32], m2
    movu       [r2 + r4 + 32], m3

    movu       m0, [r0 + 48]
    movu       m1, [r0 + r1 + 48]
    movu       m2, [r0 + r1 * 2 + 48]
    movu       m3, [r0 + r5 + 48]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 48], m0
    movu       [r2 + r3 * 1 + 48], m1
    movu       [r2 + r3 * 2 + 48], m2
    movu       [r2 + r4 + 48], m3

    movu       m0, [r0 + 64]
    movu       m1, [r0 + r1 + 64]
    movu       m2, [r0 + r1 * 2 + 64]
    movu       m3, [r0 + r5 + 64]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 64], m0
    movu       [r2 + r3 * 1 + 64], m1
    movu       [r2 + r3 * 2 + 64], m2
    movu       [r2 + r4 + 64], m3

    movu       m0, [r0 + 80]
    movu       m1, [r0 + r1 + 80]
    movu       m2, [r0 + r1 * 2 + 80]
    movu       m3, [r0 + r5 + 80]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 80], m0
    movu       [r2 + r3 * 1 + 80], m1
    movu       [r2 + r3 * 2 + 80], m2
    movu       [r2 + r4 + 80], m3

    movu       m0, [r0 + 96]
    movu       m1, [r0 + r1 + 96]
    movu       m2, [r0 + r1 * 2 + 96]
    movu       m3, [r0 + r5 + 96]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 96], m0
    movu       [r2 + r3 * 1 + 96], m1
    movu       [r2 + r3 * 2 + 96], m2
    movu       [r2 + r4 + 96], m3

    movu       m0, [r0 + 112]
    movu       m1, [r0 + r1 + 112]
    movu       m2, [r0 + r1 * 2 + 112]
    movu       m3, [r0 + r5 + 112]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
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
%macro P2S_H_64xN_avx2 1
INIT_YMM avx2
cglobal filterPixelToShort_64x%1, 3, 7, 3
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load height
    mov        r6d, %1/4

    ; load constant
    mova       m2, [pw_2000]

.loop:
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1

    movu       m0, [r0 + r1 * 2]
    movu       m1, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 2], m0
    movu       [r2 + r4], m1

    movu       m0, [r0 + 32]
    movu       m1, [r0 + r1 + 32]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 0 + 32], m0
    movu       [r2 + r3 * 1 + 32], m1

    movu       m0, [r0 + r1 * 2 + 32]
    movu       m1, [r0 + r5 + 32]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 2 + 32], m0
    movu       [r2 + r4 + 32], m1

    movu       m0, [r0 + 64]
    movu       m1, [r0 + r1 + 64]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 0 + 64], m0
    movu       [r2 + r3 * 1 + 64], m1

    movu       m0, [r0 + r1 * 2 + 64]
    movu       m1, [r0 + r5 + 64]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 2 + 64], m0
    movu       [r2 + r4 + 64], m1

    movu       m0, [r0 + 96]
    movu       m1, [r0 + r1 + 96]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 0 + 96], m0
    movu       [r2 + r3 * 1 + 96], m1

    movu       m0, [r0 + r1 * 2 + 96]
    movu       m1, [r0 + r5 + 96]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 2 + 96], m0
    movu       [r2 + r4 + 96], m1

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET
%endmacro
P2S_H_64xN_avx2 64
P2S_H_64xN_avx2 16
P2S_H_64xN_avx2 32
P2S_H_64xN_avx2 48

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

.loop:
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    movu       m2, [r0 + r1 * 2]
    movu       m3, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1
    movu       [r2 + r3 * 2], m2
    movu       [r2 + r4], m3

    movu       m0, [r0 + 16]
    movu       m1, [r0 + r1 + 16]
    movu       m2, [r0 + r1 * 2 + 16]
    movu       m3, [r0 + r5 + 16]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 16], m0
    movu       [r2 + r3 * 1 + 16], m1
    movu       [r2 + r3 * 2 + 16], m2
    movu       [r2 + r4 + 16], m3

    movu       m0, [r0 + 32]
    movu       m1, [r0 + r1 + 32]
    movu       m2, [r0 + r1 * 2 + 32]
    movu       m3, [r0 + r5 + 32]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
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
%macro P2S_H_24xN_avx2 1
INIT_YMM avx2
cglobal filterPixelToShort_24x%1, 3, 7, 3
    add        r1d, r1d
    mov        r3d, r3m
    add        r3d, r3d
    lea        r4, [r3 * 3]
    lea        r5, [r1 * 3]

    ; load height
    mov        r6d, %1/4

    ; load constant
    mova       m2, [pw_2000]

.loop:
    movu       m0, [r0]
    movu       m1, [r0 + 32]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2
    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 0 + 32], xm1

    movu       m0, [r0 + r1]
    movu       m1, [r0 + r1 + 32]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2
    movu       [r2 + r3 * 1], m0
    movu       [r2 + r3 * 1 + 32], xm1

    movu       m0, [r0 + r1 * 2]
    movu       m1, [r0 + r1 * 2 + 32]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2
    movu       [r2 + r3 * 2], m0
    movu       [r2 + r3 * 2 + 32], xm1

    movu       m0, [r0 + r5]
    movu       m1, [r0 + r5 + 32]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2
    movu       [r2 + r4], m0
    movu       [r2 + r4 + 32], xm1

    lea        r0, [r0 + r1 * 4]
    lea        r2, [r2 + r3 * 4]

    dec        r6d
    jnz        .loop
    RET
%endmacro
P2S_H_24xN_avx2 32
P2S_H_24xN_avx2 64

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

.loop:
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1

    movu       m0, [r0 + r1 * 2]
    movu       m1, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psubw      m0, m2
    psubw      m1, m2

    movu       [r2 + r3 * 2], m0
    movu       [r2 + r4], m1

    movh       m0, [r0 + 16]
    movhps     m0, [r0 + r1 + 16]
    psllw      m0, (14 - BIT_DEPTH)
    psubw      m0, m2

    movh       [r2 + r3 * 0 + 16], m0
    movhps     [r2 + r3 * 1 + 16], m0

    movh       m0, [r0 + r1 * 2 + 16]
    movhps     m0, [r0 + r5 + 16]
    psllw      m0, (14 - BIT_DEPTH)
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

.loop:
    movu       m0, [r0]
    movu       m1, [r0 + r1]
    movu       m2, [r0 + r1 * 2]
    movu       m3, [r0 + r5]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0], m0
    movu       [r2 + r3 * 1], m1
    movu       [r2 + r3 * 2], m2
    movu       [r2 + r4], m3

    movu       m0, [r0 + 16]
    movu       m1, [r0 + r1 + 16]
    movu       m2, [r0 + r1 * 2 + 16]
    movu       m3, [r0 + r5 + 16]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 16], m0
    movu       [r2 + r3 * 1 + 16], m1
    movu       [r2 + r3 * 2 + 16], m2
    movu       [r2 + r4 + 16], m3

    movu       m0, [r0 + 32]
    movu       m1, [r0 + r1 + 32]
    movu       m2, [r0 + r1 * 2 + 32]
    movu       m3, [r0 + r5 + 32]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 32], m0
    movu       [r2 + r3 * 1 + 32], m1
    movu       [r2 + r3 * 2 + 32], m2
    movu       [r2 + r4 + 32], m3

    movu       m0, [r0 + 48]
    movu       m1, [r0 + r1 + 48]
    movu       m2, [r0 + r1 * 2 + 48]
    movu       m3, [r0 + r5 + 48]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 48], m0
    movu       [r2 + r3 * 1 + 48], m1
    movu       [r2 + r3 * 2 + 48], m2
    movu       [r2 + r4 + 48], m3

    movu       m0, [r0 + 64]
    movu       m1, [r0 + r1 + 64]
    movu       m2, [r0 + r1 * 2 + 64]
    movu       m3, [r0 + r5 + 64]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
    psubw      m3, m4

    movu       [r2 + r3 * 0 + 64], m0
    movu       [r2 + r3 * 1 + 64], m1
    movu       [r2 + r3 * 2 + 64], m2
    movu       [r2 + r4 + 64], m3

    movu       m0, [r0 + 80]
    movu       m1, [r0 + r1 + 80]
    movu       m2, [r0 + r1 * 2 + 80]
    movu       m3, [r0 + r5 + 80]
    psllw      m0, (14 - BIT_DEPTH)
    psllw      m1, (14 - BIT_DEPTH)
    psllw      m2, (14 - BIT_DEPTH)
    psllw      m3, (14 - BIT_DEPTH)
    psubw      m0, m4
    psubw      m1, m4
    psubw      m2, m4
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

;-------------------------------------------------------------------------------------------------------------
;ipfilter_chroma_avx512 code start
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
; avx512 chroma_hpp code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_IPFILTER_CHROMA_PP_8x4_AVX512 0
    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m3  shuffle order table
    ; m4 - pd_32
    ; m5 - zero
    ; m6 - pw_pixel_max

    movu            xm7,       [r0]
    vinserti32x4    m7,        [r0 + r1],      1
    vinserti32x4    m7,        [r0 + 2 * r1],  2
    vinserti32x4    m7,        [r0 + r6],      3

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    movu            xm8,       [r0 + 8]
    vinserti32x4    m8,        [r0 + r1 + 8],      1
    vinserti32x4    m8,        [r0 + 2 * r1 + 8],  2
    vinserti32x4    m8,        [r0 + r6 + 8],      3

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2],      xm7
    vextracti32x4   [r2 + r3],     m7,        1
    vextracti32x4   [r2 + 2 * r3], m7,        2
    vextracti32x4   [r2 + r7],     m7,        3
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PP_16x2_AVX512 0
    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m3  shuffle order table
    ; m4 - pd_32
    ; m5 - zero
    ; m6 - pw_pixel_max

    movu            ym7,       [r0]
    vinserti32x8    m7,        [r0 + r1],      1
    movu            ym8,       [r0 + 8]
    vinserti32x8    m8,        [r0 + r1 + 8],  1

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2],      ym7
    vextracti32x8   [r2 + r3], m7,        1
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PP_24x4_AVX512 0
    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m3  shuffle order table
    ; m4 - pd_32
    ; m5 - zero
    ; m6 - pw_pixel_max

    movu            ym7,       [r0]
    vinserti32x8    m7,        [r0 + r1],      1
    movu            ym8,       [r0 + 8]
    vinserti32x8    m8,        [r0 + r1 + 8],  1

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2],      ym7
    vextracti32x8   [r2 + r3], m7,        1

    movu            ym7,       [r0 + 2 * r1]
    vinserti32x8    m7,        [r0 + r6],      1
    movu            ym8,       [r0 + 2 * r1 + 8]
    vinserti32x8    m8,        [r0 + r6 + 8],  1

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2 + 2 * r3],        ym7
    vextracti32x8   [r2 + r7], m7,        1

    movu            xm7,       [r0 + mmsize/2]
    vinserti32x4    m7,        [r0 + r1 + mmsize/2],      1
    vinserti32x4    m7,        [r0 + 2 * r1 + mmsize/2],  2
    vinserti32x4    m7,        [r0 + r6 + mmsize/2],      3

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    movu            xm8,       [r0 + mmsize/2 + 8]
    vinserti32x4    m8,        [r0 + r1 + mmsize/2 + 8],      1
    vinserti32x4    m8,        [r0 + 2 * r1 + mmsize/2 + 8],  2
    vinserti32x4    m8,        [r0 + r6 + mmsize/2 + 8],      3

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2 + mmsize/2],      xm7
    vextracti32x4   [r2 + r3 + mmsize/2],     m7,        1
    vextracti32x4   [r2 + 2 * r3 + mmsize/2], m7,        2
    vextracti32x4   [r2 + r7 + mmsize/2],     m7,        3
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PP_32x2_AVX512 0
    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m3  shuffle order table
    ; m4 - pd_32
    ; m5 - zero
    ; m6 - pw_pixel_max

    movu            m7,        [r0]
    movu            m8,        [r0 + 8]

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2],      m7

    movu            m7,        [r0 + r1]
    movu            m8,        [r0 + r1 + 8]

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2 + r3], m7
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PP_48x2_AVX512 0
    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m3  shuffle order table
    ; m4 - pd_32
    ; m5 - zero
    ; m6 - pw_pixel_max

    movu            m7,        [r0]
    movu            m8,        [r0 + 8]

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2],      m7

    movu            m7,        [r0 + r1]
    movu            m8,        [r0 + r1 + 8]

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2 + r3], m7

    movu            ym7,       [r0 + mmsize]
    vinserti32x8    m7,        [r0 + r1 + mmsize],     1
    movu            ym8,       [r0 + mmsize + 8]
    vinserti32x8    m8,        [r0 + r1 + mmsize + 8],  1

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2 + mmsize],      ym7
    vextracti32x8   [r2 + r3 + mmsize], m7,        1
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PP_64x2_AVX512 0
    ; register map
    ; m0 , m1 interpolate coeff
    ; m2 , m3  shuffle order table
    ; m4 - pd_32
    ; m5 - zero
    ; m6 - pw_pixel_max

    movu            m7,        [r0]
    movu            m8,        [r0 + 8]

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2],      m7

    movu            m7,        [r0 + mmsize]
    movu            m8,        [r0 + mmsize + 8]

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2 + mmsize],        m7

    movu            m7,        [r0 + r1]
    movu            m8,        [r0 + r1 + 8]

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2 + r3], m7

    movu            m7,        [r0 + r1 + mmsize]
    movu            m8,        [r0 + r1 + mmsize + 8]

    pshufb          m9,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m9,        m1
    paddd           m7,        m9
    paddd           m7,        m4
    psrad           m7,        6

    pshufb          m9,        m8,        m3
    pshufb          m8,        m2
    pmaddwd         m8,        m0
    pmaddwd         m9,        m1
    paddd           m8,        m9
    paddd           m8,        m4
    psrad           m8,        6

    packusdw        m7,        m8
    CLIPW           m7,        m5,        m6
    pshufb          m7,        m10
    movu            [r2 + r3 + mmsize],   m7
%endmacro
;-------------------------------------------------------------------------------------------------------------
; void interp_4tap_horiz_pp(pixel *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx
;-------------------------------------------------------------------------------------------------------------
%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_4tap_horiz_pp_8x4, 5,8,11
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
    lea             r6, [3 * r1]
    lea             r7, [3 * r3]
%ifdef PIC
    lea             r5, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r5 + r4 * 8]
    vpbroadcastd    m1, [r5 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8 m2, [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8 m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8 m4, [pd_32]
    pxor            m5, m5
    vbroadcasti32x8 m6, [pw_pixel_max]
    vbroadcasti32x8 m10, [interp8_hpp_shuf1_store_avx512]

    PROCESS_IPFILTER_CHROMA_PP_8x4_AVX512
    RET
%endif

%macro IPFILTER_CHROMA_AVX512_8xN 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_pp_8x%1, 5,8,11
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
    lea             r6, [3 * r1]
    lea             r7, [3 * r3]
%ifdef PIC
    lea             r5, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r5 + r4 * 8]
    vpbroadcastd    m1, [r5 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8 m2, [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8 m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8 m4, [pd_32]
    pxor            m5, m5
    vbroadcasti32x8 m6, [pw_pixel_max]
    vbroadcasti32x8 m10, [interp8_hpp_shuf1_store_avx512]

%rep %1/4 - 1
    PROCESS_IPFILTER_CHROMA_PP_8x4_AVX512
    lea             r0, [r0 + 4 * r1]
    lea             r2, [r2 + 4 * r3]
%endrep
    PROCESS_IPFILTER_CHROMA_PP_8x4_AVX512
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_CHROMA_AVX512_8xN 8
IPFILTER_CHROMA_AVX512_8xN 12
IPFILTER_CHROMA_AVX512_8xN 16
IPFILTER_CHROMA_AVX512_8xN 32
IPFILTER_CHROMA_AVX512_8xN 64
%endif

%macro IPFILTER_CHROMA_AVX512_16xN 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_pp_16x%1, 5,6,11
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r5 + r4 * 8]
    vpbroadcastd    m1, [r5 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8 m2, [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8 m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8 m4, [pd_32]
    pxor            m5, m5
    vbroadcasti32x8 m6, [pw_pixel_max]
    vbroadcasti32x8 m10, [interp8_hpp_shuf1_store_avx512]

%rep %1/2 - 1
    PROCESS_IPFILTER_CHROMA_PP_16x2_AVX512
    lea             r0, [r0 + 2 * r1]
    lea             r2, [r2 + 2 * r3]
%endrep
    PROCESS_IPFILTER_CHROMA_PP_16x2_AVX512
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_CHROMA_AVX512_16xN 4
IPFILTER_CHROMA_AVX512_16xN 8
IPFILTER_CHROMA_AVX512_16xN 12
IPFILTER_CHROMA_AVX512_16xN 16
IPFILTER_CHROMA_AVX512_16xN 24
IPFILTER_CHROMA_AVX512_16xN 32
IPFILTER_CHROMA_AVX512_16xN 64
%endif

%macro IPFILTER_CHROMA_AVX512_24xN 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_pp_24x%1, 5,8,11
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
    lea             r6, [3 * r1]
    lea             r7, [3 * r3]
%ifdef PIC
    lea             r5, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r5 + r4 * 8]
    vpbroadcastd    m1, [r5 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8 m2, [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8 m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8 m4, [pd_32]
    pxor            m5, m5
    vbroadcasti32x8 m6, [pw_pixel_max]
    vbroadcasti32x8 m10, [interp8_hpp_shuf1_store_avx512]

%rep %1/4 - 1
    PROCESS_IPFILTER_CHROMA_PP_24x4_AVX512
    lea             r0, [r0 + 4 * r1]
    lea             r2, [r2 + 4 * r3]
%endrep
    PROCESS_IPFILTER_CHROMA_PP_24x4_AVX512
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_CHROMA_AVX512_24xN 32
IPFILTER_CHROMA_AVX512_24xN 64
%endif

%macro IPFILTER_CHROMA_AVX512_32xN 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_pp_32x%1, 5,6,11
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r5 + r4 * 8]
    vpbroadcastd    m1, [r5 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8 m2, [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8 m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8 m4, [pd_32]
    pxor            m5, m5
    vbroadcasti32x8 m6, [pw_pixel_max]
    vbroadcasti32x8 m10, [interp8_hpp_shuf1_store_avx512]

%rep %1/2 - 1
    PROCESS_IPFILTER_CHROMA_PP_32x2_AVX512
    lea             r0, [r0 + 2 * r1]
    lea             r2, [r2 + 2 * r3]
%endrep
    PROCESS_IPFILTER_CHROMA_PP_32x2_AVX512
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_CHROMA_AVX512_32xN 8
IPFILTER_CHROMA_AVX512_32xN 16
IPFILTER_CHROMA_AVX512_32xN 24
IPFILTER_CHROMA_AVX512_32xN 32
IPFILTER_CHROMA_AVX512_32xN 48
IPFILTER_CHROMA_AVX512_32xN 64
%endif

%macro IPFILTER_CHROMA_AVX512_64xN 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_pp_64x%1, 5,6,11
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r5 + r4 * 8]
    vpbroadcastd    m1, [r5 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8 m2, [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8 m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8 m4, [pd_32]
    pxor            m5, m5
    vbroadcasti32x8 m6, [pw_pixel_max]
    vbroadcasti32x8 m10, [interp8_hpp_shuf1_store_avx512]

%rep %1/2 - 1
    PROCESS_IPFILTER_CHROMA_PP_64x2_AVX512
    lea             r0, [r0 + 2 * r1]
    lea             r2, [r2 + 2 * r3]
%endrep
    PROCESS_IPFILTER_CHROMA_PP_64x2_AVX512
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_CHROMA_AVX512_64xN 16
IPFILTER_CHROMA_AVX512_64xN 32
IPFILTER_CHROMA_AVX512_64xN 48
IPFILTER_CHROMA_AVX512_64xN 64
%endif

%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_4tap_horiz_pp_48x64, 5,6,11
    add             r1d, r1d
    add             r3d, r3d
    sub             r0, 2
    mov             r4d, r4m
%ifdef PIC
    lea             r5, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r5 + r4 * 8]
    vpbroadcastd    m1, [r5 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8 m2, [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8 m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8 m4, [pd_32]
    pxor            m5, m5
    vbroadcasti32x8 m6, [pw_pixel_max]
    vbroadcasti32x8 m10, [interp8_hpp_shuf1_store_avx512]

%rep 31
    PROCESS_IPFILTER_CHROMA_PP_48x2_AVX512
    lea             r0, [r0 + 2 * r1]
    lea             r2, [r2 + 2 * r3]
%endrep
    PROCESS_IPFILTER_CHROMA_PP_48x2_AVX512
    RET
%endif
;-------------------------------------------------------------------------------------------------------------
; avx512 chroma_hpp code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
; avx512 chroma_vpp code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_CHROMA_VERT_PP_8x8_AVX512 0
    movu                  xm1,                [r0]
    lea                   r6,                 [r0 + 2 * r1]
    lea                   r8,                 [r0 + 4 * r1]
    lea                   r9,                 [r8 + 2 * r1]
    vinserti32x4          m1,                 [r6],                1
    vinserti32x4          m1,                 [r8],                2
    vinserti32x4          m1,                 [r9],                3
    movu                  xm3,                [r0 + r1]
    vinserti32x4          m3,                 [r6 + r1],           1
    vinserti32x4          m3,                 [r8 + r1],           2
    vinserti32x4          m3,                 [r9 + r1],           3
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 [r5]
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 [r5]

    movu                  xm4,                [r0 + 2 * r1]
    vinserti32x4          m4,                 [r6 + 2 * r1],       1
    vinserti32x4          m4,                 [r8 + 2 * r1],       2
    vinserti32x4          m4,                 [r9 + 2 * r1],       3
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 [r5]
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 [r5]

    movu                  xm5,                [r0 + r10]
    vinserti32x4          m5,                 [r6 + r10],          1
    vinserti32x4          m5,                 [r8 + r10],          2
    vinserti32x4          m5,                 [r9 + r10],          3
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 [r5 + mmsize]
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 [r5 + mmsize]
    paddd                 m1,                 m4

    movu                  xm4,                [r0 + 4 * r1]
    vinserti32x4          m4,                 [r6 + 4 * r1],       1
    vinserti32x4          m4,                 [r8 + 4 * r1],       2
    vinserti32x4          m4,                 [r9 + 4 * r1],       3
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 [r5 + mmsize]
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 [r5 + mmsize]
    paddd                 m3,                 m5

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_PP
    psrad                 m1,                 INTERP_SHIFT_PP
    psrad                 m2,                 INTERP_SHIFT_PP
    psrad                 m3,                 INTERP_SHIFT_PP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    pxor                  m5,                 m5
    CLIPW2                m0,                 m2,                  m5,                 m8
    movu                  [r2],               xm0
    movu                  [r2 + r3],          xm2
    vextracti32x4         [r2 + 2 * r3],      m0,                  1
    vextracti32x4         [r2 + r7],          m2,                  1
    lea                   r2,                 [r2 + 4 * r3]
    vextracti32x4         [r2],               m0,                  2
    vextracti32x4         [r2 + r3],          m2,                  2
    vextracti32x4         [r2 + 2 * r3],      m0,                  3
    vextracti32x4         [r2 + r7],          m2,                  3
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_4tap_vert_pp_8x8, 5, 11, 9
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x8       m7,                 [INTERP_OFFSET_PP]
    vbroadcasti32x8       m8,                 [pw_pixel_max]
    lea                   r10,                [3 * r1]
    lea                   r7,                 [3 * r3]
    PROCESS_CHROMA_VERT_PP_8x8_AVX512
    RET
%endif

%macro FILTER_VER_PP_CHROMA_8xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_pp_8x%1, 5, 11, 9
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x8       m7,                 [INTERP_OFFSET_PP]
    vbroadcasti32x8       m8,                 [pw_pixel_max]
    lea                   r10,                [3 * r1]
    lea                   r7,                 [3 * r3]
%rep %1/8 - 1
    PROCESS_CHROMA_VERT_PP_8x8_AVX512
    lea                   r0,                 [r8 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_PP_8x8_AVX512
    RET
%endmacro

%if ARCH_X86_64
FILTER_VER_PP_CHROMA_8xN_AVX512 16
FILTER_VER_PP_CHROMA_8xN_AVX512 32
FILTER_VER_PP_CHROMA_8xN_AVX512 64
%endif

%macro PROCESS_CHROMA_VERT_PP_16x4_AVX512 0
    movu                  ym1,                [r0]
    lea                   r6,                 [r0 + 2 * r1]
    vinserti32x8          m1,                 [r6],                1
    movu                  ym3,                [r0 + r1]
    vinserti32x8          m3,                 [r6 + r1],           1
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 [r5]
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 [r5]

    movu                  ym4,                [r0 + 2 * r1]
    vinserti32x8          m4,                 [r6 + 2 * r1],       1
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 [r5]
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 [r5]

    lea                   r0,                 [r0 + 2 * r1]
    lea                   r6,                 [r6 + 2 * r1]

    movu                  ym5,                [r0 + r1]
    vinserti32x8          m5,                 [r6 + r1],           1
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 [r5 + mmsize]
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 [r5 + mmsize]
    paddd                 m1,                 m4

    movu                  ym4,                [r0 + 2 * r1]
    vinserti32x8          m4,                 [r6 + 2 * r1],       1
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 [r5 + mmsize]
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 [r5 + mmsize]
    paddd                 m3,                 m5

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_PP
    psrad                 m1,                 INTERP_SHIFT_PP
    psrad                 m2,                 INTERP_SHIFT_PP
    psrad                 m3,                 INTERP_SHIFT_PP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    pxor                  m5,                 m5
    CLIPW2                m0,                 m2,                  m5,                 m8
    movu                  [r2],               ym0
    movu                  [r2 + r3],          ym2
    vextracti32x8         [r2 + 2 * r3],      m0,                  1
    vextracti32x8         [r2 + r7],          m2,                  1
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_4tap_vert_pp_16x4, 5, 8, 9
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x8       m7,                 [INTERP_OFFSET_PP]
    vbroadcasti32x8       m8,                 [pw_pixel_max]
    lea                   r7,                 [3 * r3]
    PROCESS_CHROMA_VERT_PP_16x4_AVX512
    RET
%endif

%macro FILTER_VER_PP_CHROMA_16xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_pp_16x%1, 5, 8, 9
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x8       m7,                 [INTERP_OFFSET_PP]
    vbroadcasti32x8       m8,                 [pw_pixel_max]
    lea                   r7,                 [3 * r3]
%rep %1/4 - 1
    PROCESS_CHROMA_VERT_PP_16x4_AVX512
    lea                   r0,                 [r0 + 2 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_PP_16x4_AVX512
    RET
%endmacro

%if ARCH_X86_64
FILTER_VER_PP_CHROMA_16xN_AVX512 8
FILTER_VER_PP_CHROMA_16xN_AVX512 12
FILTER_VER_PP_CHROMA_16xN_AVX512 16
FILTER_VER_PP_CHROMA_16xN_AVX512 24
FILTER_VER_PP_CHROMA_16xN_AVX512 32
FILTER_VER_PP_CHROMA_16xN_AVX512 64
%endif

%macro PROCESS_CHROMA_VERT_PP_24x8_AVX512 0
    movu                  ym1,                [r0]
    lea                   r6,                 [r0 + 2 * r1]
    lea                   r8,                 [r0 + 4 * r1]
    lea                   r9,                 [r8 + 2 * r1]

    movu                  ym10,               [r8]
    movu                  ym3,                [r0 + r1]
    movu                  ym12,               [r8 + r1]
    vinserti32x8          m1,                 [r6],                1
    vinserti32x8          m10,                [r9],                1
    vinserti32x8          m3,                 [r6 + r1],           1
    vinserti32x8          m12,                [r9 + r1],           1

    punpcklwd             m0,                 m1,                  m3
    punpcklwd             m9,                 m10,                 m12
    pmaddwd               m0,                 [r5]
    pmaddwd               m9,                 [r5]
    punpckhwd             m1,                 m3
    punpckhwd             m10,                m12
    pmaddwd               m1,                 [r5]
    pmaddwd               m10,                [r5]

    movu                  ym4,                [r0 + 2 * r1]
    movu                  ym13,               [r8 + 2 * r1]
    vinserti32x8          m4,                 [r6 + 2 * r1],       1
    vinserti32x8          m13,                [r9 + 2 * r1],       1
    punpcklwd             m2,                 m3,                  m4
    punpcklwd             m11,                m12,                 m13
    pmaddwd               m2,                 [r5]
    pmaddwd               m11,                [r5]
    punpckhwd             m3,                 m4
    punpckhwd             m12,                m13
    pmaddwd               m3,                 [r5]
    pmaddwd               m12,                [r5]

    movu                  ym5,                [r0 + r10]
    vinserti32x8          m5,                 [r6 + r10],          1
    movu                  ym14,               [r8 + r10]
    vinserti32x8          m14,                [r9 + r10],          1
    punpcklwd             m6,                 m4,                  m5
    punpcklwd             m15,                m13,                 m14
    pmaddwd               m6,                 [r5 + mmsize]
    pmaddwd               m15,                [r5 + mmsize]
    paddd                 m0,                 m6
    paddd                 m9,                 m15
    punpckhwd             m4,                 m5
    punpckhwd             m13,                m14
    pmaddwd               m4,                 [r5 + mmsize]
    pmaddwd               m13,                [r5 + mmsize]
    paddd                 m1,                 m4
    paddd                 m10,                m13

    movu                  ym4,                [r0 + 4 * r1]
    vinserti32x8          m4,                 [r6 + 4 * r1],       1
    movu                  ym13,               [r8 + 4 * r1]
    vinserti32x8          m13,                [r9 + 4 * r1],       1
    punpcklwd             m6,                 m5,                  m4
    punpcklwd             m15,                m14,                 m13
    pmaddwd               m6,                 [r5 + mmsize]
    pmaddwd               m15,                [r5 + mmsize]
    paddd                 m2,                 m6
    paddd                 m11,                m15
    punpckhwd             m5,                 m4
    punpckhwd             m14,                m13
    pmaddwd               m5,                 [r5 + mmsize]
    pmaddwd               m14,                [r5 + mmsize]
    paddd                 m3,                 m5
    paddd                 m12,                m14

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7
    paddd                 m9,                 m7
    paddd                 m10,                m7
    paddd                 m11,                m7
    paddd                 m12,                m7

    psrad                 m0,                 INTERP_SHIFT_PP
    psrad                 m1,                 INTERP_SHIFT_PP
    psrad                 m2,                 INTERP_SHIFT_PP
    psrad                 m3,                 INTERP_SHIFT_PP
    psrad                 m9,                 INTERP_SHIFT_PP
    psrad                 m10,                INTERP_SHIFT_PP
    psrad                 m11,                INTERP_SHIFT_PP
    psrad                 m12,                INTERP_SHIFT_PP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packssdw              m9,                 m10
    packssdw              m11,                m12
    pxor                  m5,                 m5
    CLIPW2                m0,                 m2,                  m5,                 m8
    CLIPW2                m9,                 m11,                 m5,                 m8	
    movu                  [r2],               ym0
    movu                  [r2 + r3],          ym2
    vextracti32x8         [r2 + 2 * r3],      m0,                  1
    vextracti32x8         [r2 + r7],          m2,                  1
    lea                   r11,                [r2 + 4 * r3]
    movu                  [r11],              ym9
    movu                  [r11 + r3],         ym11
    vextracti32x8         [r11 + 2 * r3],     m9,                  1
    vextracti32x8         [r11 + r7],         m11,                 1

    movu                  xm1,                [r0 + mmsize/2]
    vinserti32x4          m1,                 [r6 + mmsize/2],                1
    vinserti32x4          m1,                 [r8 + mmsize/2],                2
    vinserti32x4          m1,                 [r9 + mmsize/2],                3
    movu                  xm3,                [r0 + r1 + mmsize/2]
    vinserti32x4          m3,                 [r6 + r1 + mmsize/2],           1
    vinserti32x4          m3,                 [r8 + r1 + mmsize/2],           2
    vinserti32x4          m3,                 [r9 + r1 + mmsize/2],           3
    punpcklwd             m0,                 m1,                             m3
    pmaddwd               m0,                 [r5]
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 [r5]

    movu                  xm4,                [r0 + 2 * r1 + mmsize/2]
    vinserti32x4          m4,                 [r6 + 2 * r1 + mmsize/2],       1
    vinserti32x4          m4,                 [r8 + 2 * r1 + mmsize/2],       2
    vinserti32x4          m4,                 [r9 + 2 * r1 + mmsize/2],       3
    punpcklwd             m2,                 m3,                             m4
    pmaddwd               m2,                 [r5]
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 [r5]

    movu                  xm5,                [r0 + r10 + mmsize/2]
    vinserti32x4          m5,                 [r6 + r10 + mmsize/2],          1
    vinserti32x4          m5,                 [r8 + r10 + mmsize/2],          2
    vinserti32x4          m5,                 [r9 + r10 + mmsize/2],          3
    punpcklwd             m6,                 m4,                             m5
    pmaddwd               m6,                 [r5 + mmsize]
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 [r5 + mmsize]
    paddd                 m1,                 m4

    movu                  xm4,                [r0 + 4 * r1 + mmsize/2]
    vinserti32x4          m4,                 [r6 + 4 * r1 + mmsize/2],       1
    vinserti32x4          m4,                 [r8 + 4 * r1 + mmsize/2],       2
    vinserti32x4          m4,                 [r9 + 4 * r1 + mmsize/2],       3
    punpcklwd             m6,                 m5,                             m4
    pmaddwd               m6,                 [r5 + mmsize]
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 [r5 + mmsize]
    paddd                 m3,                 m5

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_PP
    psrad                 m1,                 INTERP_SHIFT_PP
    psrad                 m2,                 INTERP_SHIFT_PP
    psrad                 m3,                 INTERP_SHIFT_PP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    pxor                  m5,                 m5
    CLIPW2                m0,                 m2,        m5,                  m8
    movu                  [r2 + mmsize/2],               xm0
    movu                  [r2 + r3 + mmsize/2],          xm2
    vextracti32x4         [r2 + 2 * r3 + mmsize/2],      m0,                  1
    vextracti32x4         [r2 + r7 + mmsize/2],          m2,                  1
    lea                   r2,                            [r2 + 4 * r3]
    vextracti32x4         [r2 + mmsize/2],               m0,                  2
    vextracti32x4         [r2 + r3 + mmsize/2],          m2,                  2
    vextracti32x4         [r2 + 2 * r3 + mmsize/2],      m0,                  3
    vextracti32x4         [r2 + r7 + mmsize/2],          m2,                  3	
%endmacro

%macro FILTER_VER_PP_CHROMA_24xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_pp_24x%1, 5, 12, 16
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x8       m7,                 [INTERP_OFFSET_PP]
    vbroadcasti32x8       m8,                 [pw_pixel_max]
    lea                   r10,                [3 * r1]
    lea                   r7,                 [3 * r3]
%rep %1/8 - 1
    PROCESS_CHROMA_VERT_PP_24x8_AVX512
    lea                   r0,                 [r8 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_PP_24x8_AVX512
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_PP_CHROMA_24xN_AVX512 32
    FILTER_VER_PP_CHROMA_24xN_AVX512 64
%endif

%macro PROCESS_CHROMA_VERT_PP_32x2_AVX512 0
    movu                  m1,                 [r0]
    movu                  m3,                 [r0 + r1]
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 [r5]
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 [r5]

    movu                  m4,                 [r0 + 2 * r1]
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 [r5]
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 [r5]

    lea                   r0,                 [r0 + 2 * r1]
    movu                  m5,                 [r0 + r1]
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 [r5 + mmsize]
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 [r5 + mmsize]
    paddd                 m1,                 m4

    movu                  m4,                 [r0 + 2 * r1]
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 [r5 + mmsize]
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 [r5 + mmsize]
    paddd                 m3,                 m5

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_PP
    psrad                 m1,                 INTERP_SHIFT_PP
    psrad                 m2,                 INTERP_SHIFT_PP
    psrad                 m3,                 INTERP_SHIFT_PP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    pxor                  m5,                 m5
    CLIPW2                m0,                 m2,                  m5,                 m8
    movu                  [r2],               m0
    movu                  [r2 + r3],          m2
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_PP_CHROMA_32xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_pp_32x%1, 5, 7, 9
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x8       m7,                 [INTERP_OFFSET_PP]
    vbroadcasti32x8       m8,                 [pw_pixel_max]

%rep %1/2 - 1
    PROCESS_CHROMA_VERT_PP_32x2_AVX512
    lea                   r2,                 [r2 + 2 * r3]
%endrep
    PROCESS_CHROMA_VERT_PP_32x2_AVX512
    RET
%endmacro

%if ARCH_X86_64
FILTER_VER_PP_CHROMA_32xN_AVX512 8
FILTER_VER_PP_CHROMA_32xN_AVX512 16
FILTER_VER_PP_CHROMA_32xN_AVX512 24
FILTER_VER_PP_CHROMA_32xN_AVX512 32
FILTER_VER_PP_CHROMA_32xN_AVX512 48
FILTER_VER_PP_CHROMA_32xN_AVX512 64
%endif

%macro PROCESS_CHROMA_VERT_PP_48x4_AVX512 0
    movu                  m1,                 [r0]
    lea                   r6,                 [r0 + 2 * r1]
    movu                  m10,                [r6]
    movu                  m3,                 [r0 + r1]
    movu                  m12,                [r6 + r1]
    punpcklwd             m0,                 m1,                  m3
    punpcklwd             m9,                 m10,                 m12
    pmaddwd               m0,                 [r5]
    pmaddwd               m9,                 [r5]
    punpckhwd             m1,                 m3
    punpckhwd             m10,                m12
    pmaddwd               m1,                 [r5]
    pmaddwd               m10,                [r5]

    movu                  m4,                 [r0 + 2 * r1]
    movu                  m13,                [r6 + 2 * r1]
    punpcklwd             m2,                 m3,                  m4
    punpcklwd             m11,                m12,                 m13
    pmaddwd               m2,                 [r5]
    pmaddwd               m11,                [r5]
    punpckhwd             m3,                 m4
    punpckhwd             m12,                m13
    pmaddwd               m3,                 [r5]
    pmaddwd               m12,                [r5]

    movu                  m5,                 [r0 + r7]
    movu                  m14,                [r6 + r7]
    punpcklwd             m6,                 m4,                  m5
    punpcklwd             m15,                m13,                 m14
    pmaddwd               m6,                 [r5 + mmsize]
    pmaddwd               m15,                [r5 + mmsize]
    paddd                 m0,                 m6
    paddd                 m9,                 m15
    punpckhwd             m4,                 m5
    punpckhwd             m13,                m14
    pmaddwd               m4,                 [r5 + mmsize]
    pmaddwd               m13,                [r5 + mmsize]
    paddd                 m1,                 m4
    paddd                 m10,                m13

    movu                  m4,                 [r0 + 4 * r1]
    movu                  m13,                [r6 + 4 * r1]
    punpcklwd             m6,                 m5,                  m4
    punpcklwd             m15,                m14,                 m13
    pmaddwd               m6,                 [r5 + mmsize]
    pmaddwd               m15,                [r5 + mmsize]
    paddd                 m2,                 m6
    paddd                 m11,                m15
    punpckhwd             m5,                 m4
    punpckhwd             m14,                m13
    pmaddwd               m5,                 [r5 + mmsize]
    pmaddwd               m14,                [r5 + mmsize]
    paddd                 m3,                 m5
    paddd                 m12,                m14

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7
    paddd                 m9,                 m7
    paddd                 m10,                m7
    paddd                 m11,                m7
    paddd                 m12,                m7

    psrad                 m0,                 INTERP_SHIFT_PP
    psrad                 m1,                 INTERP_SHIFT_PP
    psrad                 m2,                 INTERP_SHIFT_PP
    psrad                 m3,                 INTERP_SHIFT_PP
    psrad                 m9,                 INTERP_SHIFT_PP
    psrad                 m10,                INTERP_SHIFT_PP
    psrad                 m11,                INTERP_SHIFT_PP
    psrad                 m12,                INTERP_SHIFT_PP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packssdw              m9,                 m10
    packssdw              m11,                m12
    CLIPW2                m0,                 m2,                 m16,                 m8
    CLIPW2                m9,                 m11,                m16,                 m8
    movu                  [r2],               m0
    movu                  [r2 + r3],          m2
    movu                  [r2 + 2 * r3],      m9
    movu                  [r2 + r8],          m11

    movu                  ym1,                [r0 + mmsize]
    vinserti32x8          m1,                 [r6 + mmsize],       1
    movu                  ym3,                [r0 + r1 + mmsize]
    vinserti32x8          m3,                 [r6 + r1 + mmsize],  1
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 [r5]
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 [r5]

    movu                  ym4,                [r0 + 2 * r1 + mmsize]
    vinserti32x8          m4,                 [r6 + 2 * r1 + mmsize],  1
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 [r5]
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 [r5]

    movu                  ym5,                [r0 + r7 + mmsize]
    vinserti32x8          m5,                 [r6 + r7 + mmsize],  1
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 [r5 + mmsize]
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 [r5 + mmsize]
    paddd                 m1,                 m4

    movu                  ym4,                [r0 + 4 * r1 + mmsize]
    vinserti32x8          m4,                 [r6 + 4 * r1 + mmsize],  1
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 [r5 + mmsize]
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 [r5 + mmsize]
    paddd                 m3,                 m5

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_PP
    psrad                 m1,                 INTERP_SHIFT_PP
    psrad                 m2,                 INTERP_SHIFT_PP
    psrad                 m3,                 INTERP_SHIFT_PP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    CLIPW2                m0,                 m2,                 m16,                 m8
    movu                  [r2 + mmsize],               ym0
    movu                  [r2 + r3 + mmsize],          ym2
    vextracti32x8         [r2 + 2 * r3 + mmsize],      m0,                  1
    vextracti32x8         [r2 + r8 + mmsize],          m2,                  1
%endmacro

%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_4tap_vert_pp_48x64, 5, 9, 17
     add                   r1d,                r1d
     add                   r3d,                r3d
     sub                   r0,                 r1
     shl                   r4d,                7
%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    lea                   r7,                 [3 * r1]
    lea                   r8,                 [3 * r3]
    vbroadcasti32x8       m7,                 [INTERP_OFFSET_PP]
    vbroadcasti32x8       m8,                 [pw_pixel_max]
    pxor                  m16,                m16

%rep 15
    PROCESS_CHROMA_VERT_PP_48x4_AVX512
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_PP_48x4_AVX512
    RET
%endif

%macro PROCESS_CHROMA_VERT_PP_64x2_AVX512 0
    movu                 m1,                  [r0]
    movu                 m3,                  [r0 + r1]
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  [r5]
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  [r5]

    movu                 m9,                  [r0 + mmsize]
    movu                 m11,                 [r0 + r1 + mmsize]
    punpcklwd            m8,                  m9,                     m11
    pmaddwd              m8,                  [r5]
    punpckhwd            m9,                  m11
    pmaddwd              m9,                  [r5]

    movu                 m4,                  [r0 + 2 * r1]
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  [r5]
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  [r5]

    movu                 m12,                 [r0 + 2 * r1 + mmsize]
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 [r5]
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 [r5]

    lea                  r0,                  [r0 + 2 * r1]
    movu                 m5,                  [r0 + r1]
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  [r5 + 1 * mmsize]
    paddd                m0,                  m6
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  [r5 + 1 * mmsize]
    paddd                m1,                  m4

    movu                 m13,                 [r0 + r1 + mmsize]
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 [r5 + 1 * mmsize]
    paddd                m8,                  m14
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 [r5 + 1 * mmsize]
    paddd                m9,                  m12

    movu                 m4,                  [r0 + 2 * r1]
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  [r5 + 1 * mmsize]
    paddd                m2,                  m6
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  [r5 + 1 * mmsize]
    paddd                m3,                  m5

    movu                 m12,                 [r0 + 2 * r1 + mmsize]
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 [r5 + 1 * mmsize]
    paddd                m10,                 m14
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 [r5 + 1 * mmsize]
    paddd                m11,                 m13

    paddd                m0,                  m7
    paddd                m1,                  m7
    paddd                m2,                  m7
    paddd                m3,                  m7
    paddd                m8,                  m7
    paddd                m9,                  m7
    paddd                m10,                 m7
    paddd                m11,                 m7

    psrad                m0,                  INTERP_SHIFT_PP
    psrad                m1,                  INTERP_SHIFT_PP
    psrad                m2,                  INTERP_SHIFT_PP
    psrad                m3,                  INTERP_SHIFT_PP
    psrad                m8,                  INTERP_SHIFT_PP
    psrad                m9,                  INTERP_SHIFT_PP
    psrad                m10,                 INTERP_SHIFT_PP
    psrad                m11,                 INTERP_SHIFT_PP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    packssdw             m8,                  m9
    packssdw             m10,                 m11
    pxor                 m5,                  m5
    CLIPW2               m0,                  m2,                  m5,                 m15
    CLIPW2               m8,                  m10,                 m5,                 m15
    movu                 [r2],                m0
    movu                 [r2 + r3],           m2
    movu                 [r2 + mmsize],       m8
    movu                 [r2 + r3 + mmsize],  m10
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_PP_CHROMA_64xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_pp_64x%1, 5, 7, 16
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x8       m7,                 [INTERP_OFFSET_PP]
    vbroadcasti32x8       m15,                [pw_pixel_max]

%rep %1/2 - 1
    PROCESS_CHROMA_VERT_PP_64x2_AVX512
    lea                   r2,                 [r2 + 2 * r3]
%endrep
    PROCESS_CHROMA_VERT_PP_64x2_AVX512
    RET
%endmacro

%if ARCH_X86_64
FILTER_VER_PP_CHROMA_64xN_AVX512 16
FILTER_VER_PP_CHROMA_64xN_AVX512 32
FILTER_VER_PP_CHROMA_64xN_AVX512 48
FILTER_VER_PP_CHROMA_64xN_AVX512 64
%endif
;-------------------------------------------------------------------------------------------------------------
; avx512 chroma_vpp code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
; avx512 chroma_hps code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_IPFILTER_CHROMA_PS_32x2_AVX512 0
    ; register map
    ; m0 , m1 - interpolate coeff
    ; m2 , m3 - shuffle load order table
    ; m4      - INTERP_OFFSET_PS
    ; m5      - shuffle store order table

    movu            m6,        [r0]
    movu            m7,        [r0 + 8]

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2],      m6

    movu            m6,        [r0 + r1]
    movu            m7,        [r0 + r1 + 8]

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2 + r3], m6
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PS_32x1_AVX512 0
    movu            m6,        [r0]
    movu            m7,        [r0 + 8]

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2],      m6
%endmacro

%macro IPFILTER_CHROMA_PS_AVX512_32xN 1
%if ARCH_X86_64 == 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_ps_32x%1, 4,7,9
    shl             r1d, 1
    shl             r3d, 1
    mov             r4d, r4m
    mov             r5d, r5m
%ifdef PIC
    lea             r6, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r6 + r4 * 8]
    vpbroadcastd    m1, [r6 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8 m2, [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8 m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x4 m4, [INTERP_OFFSET_PS]
    vbroadcasti32x8 m5, [interp8_hpp_shuf1_store_avx512]

    mov               r6d,         %1
    sub               r0,          2
    test              r5d,         r5d
    jz                .loop
    sub               r0,          r1
    add               r6d,         3
    PROCESS_IPFILTER_CHROMA_PS_32x1_AVX512
    lea               r0, [r0 + r1]
    lea               r2, [r2 + r3]
    dec               r6d

.loop:
    PROCESS_IPFILTER_CHROMA_PS_32x2_AVX512
    lea             r0,  [r0 + 2 * r1]
    lea             r2,  [r2 + 2 * r3]
    sub             r6d, 2
    jnz             .loop
    RET
%endif
%endmacro

IPFILTER_CHROMA_PS_AVX512_32xN 8
IPFILTER_CHROMA_PS_AVX512_32xN 16
IPFILTER_CHROMA_PS_AVX512_32xN 24
IPFILTER_CHROMA_PS_AVX512_32xN 32
IPFILTER_CHROMA_PS_AVX512_32xN 48
IPFILTER_CHROMA_PS_AVX512_32xN 64

%macro PROCESS_IPFILTER_CHROMA_PS_64x2_AVX512 0
    ; register map
    ; m0 , m1 - interpolate coeff
    ; m2 , m3 -shuffle order table
    ; m4      - INTERP_OFFSET_PS
    ; m5      - shuffle store order table


    movu            m6,        [r0]
    movu            m7,        [r0 + 8]

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2],      m6

    movu            m6,        [r0 + mmsize]
    movu            m7,        [r0 + mmsize + 8]

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2 + mmsize],        m6

    movu            m6,        [r0 + r1]
    movu            m7,        [r0 + r1 + 8]

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2 + r3], m6

    movu            m6,        [r0 + r1 + mmsize]
    movu            m7,        [r0 + r1 + mmsize + 8]

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2 + r3 + mmsize],   m6
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PS_64x1_AVX512 0
    movu            m6,        [r0]
    movu            m7,        [r0 + 8]

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2],      m6

    movu            m6,        [r0 + mmsize]
    movu            m7,        [r0 + mmsize + 8]

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2 + mmsize],        m6
%endmacro

%macro IPFILTER_CHROMA_PS_AVX512_64xN 1
%if ARCH_X86_64 == 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_ps_64x%1, 4,7,9
    shl             r1d, 1
    shl             r3d, 1
    mov             r4d, r4m
    mov             r5d, r5m
%ifdef PIC
    lea             r6, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r6 + r4 * 8]
    vpbroadcastd    m1, [r6 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8 m2, [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8 m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x4 m4, [INTERP_OFFSET_PS]
    vbroadcasti32x8 m5, [interp8_hpp_shuf1_store_avx512]
    mov               r6d,         %1
    sub               r0,          2
    test              r5d,         r5d
    jz                .loop
    sub               r0,          r1
    add               r6d,         3
    PROCESS_IPFILTER_CHROMA_PS_64x1_AVX512
    lea               r0, [r0 + r1]
    lea               r2, [r2 + r3]
    dec               r6d

.loop:
    PROCESS_IPFILTER_CHROMA_PS_64x2_AVX512
    lea             r0,  [r0 + 2 * r1]
    lea             r2,  [r2 + 2 * r3]
    sub             r6d, 2
    jnz             .loop
    RET
%endif
%endmacro

IPFILTER_CHROMA_PS_AVX512_64xN 16
IPFILTER_CHROMA_PS_AVX512_64xN 32
IPFILTER_CHROMA_PS_AVX512_64xN 48
IPFILTER_CHROMA_PS_AVX512_64xN 64

%macro PROCESS_IPFILTER_CHROMA_PS_16x2_AVX512 0
    ; register map
    ; m0 , m1 - interpolate coeff
    ; m2 , m3 - shuffle order table
    ; m4      - INTERP_OFFSET_PS
    ; m5      - shuffle store order table

    movu            ym6,       [r0]
    vinserti32x8    m6,        [r0 + r1],      1
    movu            ym7,       [r0 + 8]
    vinserti32x8    m7,        [r0 + r1 + 8],  1

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2],      ym6
    vextracti32x8   [r2 + r3], m6,        1
%endmacro
%macro PROCESS_IPFILTER_CHROMA_PS_16x1_AVX512 0
    movu            ym6,       [r0]
    vinserti32x8    m6,        [r0 + 8],  1

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    vextracti32x8   ym7,       m6,        1
    packssdw        ym6,       ym7
    pshufb          ym6,       ym5
    movu            [r2],      ym6
%endmacro
%macro IPFILTER_CHROMA_PS_AVX512_16xN 1
%if ARCH_X86_64 == 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_ps_16x%1, 4,7,9
    shl             r1d, 1
    shl             r3d, 1
    mov             r4d, r4m
    mov             r5d, r5m
%ifdef PIC
    lea             r6, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r6 + r4 * 8]
    vpbroadcastd    m1, [r6 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    mova            m2, [interp8_hpp_shuf1_load_avx512]
    mova            m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x4 m4, [INTERP_OFFSET_PS]
    mova            m5, [interp8_hpp_shuf1_store_avx512]
    mov               r6d,         %1
    sub               r0,          2
    test              r5d,         r5d
    jz                .loop
    sub               r0,          r1
    add               r6d,         3
    PROCESS_IPFILTER_CHROMA_PS_16x1_AVX512
    lea               r0, [r0 + r1]
    lea               r2, [r2 + r3]
    dec               r6d

.loop:
    PROCESS_IPFILTER_CHROMA_PS_16x2_AVX512
    lea             r0,  [r0 + 2 * r1]
    lea             r2,  [r2 + 2 * r3]
    sub             r6d, 2
    jnz             .loop
    RET
%endif
%endmacro

IPFILTER_CHROMA_PS_AVX512_16xN 4
IPFILTER_CHROMA_PS_AVX512_16xN 8
IPFILTER_CHROMA_PS_AVX512_16xN 12
IPFILTER_CHROMA_PS_AVX512_16xN 16
IPFILTER_CHROMA_PS_AVX512_16xN 24
IPFILTER_CHROMA_PS_AVX512_16xN 32
IPFILTER_CHROMA_PS_AVX512_16xN 64

%macro PROCESS_IPFILTER_CHROMA_PS_48x2_AVX512 0
    ; register map
    ; m0 , m1 - interpolate coeff
    ; m2 , m3 - shuffle load order table
    ; m4      - INTERP_OFFSET_PS
    ; m5      - shuffle store order table

    movu            m6,        [r0]
    movu            m7,        [r0 + 8]

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2],      m6

    movu            m6,        [r0 + r1]
    movu            m7,        [r0 + r1 + 8]

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2 + r3], m6

    movu            ym6,       [r0 + mmsize]
    vinserti32x8    m6,        [r0 + r1 + mmsize],     1
    movu            ym7,       [r0 + mmsize + 8]
    vinserti32x8    m7,        [r0 + r1 + mmsize + 8],  1

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2 + mmsize],      ym6
    vextracti32x8   [r2 + r3 + mmsize], m6,        1
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PS_48x1_AVX512 0
    ; register map
    ; m0 , m1 - interpolate coeff
    ; m2 , m3 - shuffle load order table
    ; m4      - INTERP_OFFSET_PS
    ; m5      - shuffle store order table

    movu            m6,        [r0]
    movu            m7,        [r0 + 8]

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2],      m6

    movu            ym6,       [r0 + mmsize]
    movu            ym7,       [r0 + mmsize + 8]

    pshufb          ym8,        ym6,        ym3
    pshufb          ym6,        ym2
    pmaddwd         ym6,        ym0
    pmaddwd         ym8,        ym1
    paddd           ym6,        ym8
    paddd           ym6,        ym4
    psrad           ym6,        INTERP_SHIFT_PS

    pshufb          ym8,        ym7,        ym3
    pshufb          ym7,        ym2
    pmaddwd         ym7,        ym0
    pmaddwd         ym8,        ym1
    paddd           ym7,        ym8
    paddd           ym7,        ym4
    psrad           ym7,        INTERP_SHIFT_PS

    packssdw        ym6,        ym7
    pshufb          ym6,        ym5
    movu            [r2 + mmsize],       ym6
%endmacro

%if ARCH_X86_64 == 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_ps_48x64, 4,7,9
    shl             r1d, 1
    shl             r3d, 1
    mov             r4d, r4m
    mov             r5d, r5m

%ifdef PIC
    lea             r6, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r6 + r4 * 8]
    vpbroadcastd    m1, [r6 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8 m2, [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8 m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x4 m4, [INTERP_OFFSET_PS]
    vbroadcasti32x8 m5, [interp8_hpp_shuf1_store_avx512]

    mov               r6d,         64
    sub               r0,          2
    test              r5d,         r5d
    jz                .loop
    sub               r0,          r1
    add               r6d,         3
    PROCESS_IPFILTER_CHROMA_PS_48x1_AVX512
    lea               r0, [r0 + r1]
    lea               r2, [r2 + r3]
    dec               r6d
.loop:
    PROCESS_IPFILTER_CHROMA_PS_48x2_AVX512
    lea             r0,  [r0 + 2 * r1]
    lea             r2,  [r2 + 2 * r3]
    sub             r6d, 2
    jnz             .loop
    RET
%endif

%macro PROCESS_IPFILTER_CHROMA_PS_8x4_AVX512 0
    ; register map
    ; m0 , m1 - interpolate coeff
    ; m2 , m3 - shuffle load order table
    ; m4      - INTERP_OFFSET_PS
    ; m5      - shuffle store order table

    movu            xm6,       [r0]
    vinserti32x4    m6,        [r0 + r1],      1
    vinserti32x4    m6,        [r0 + 2 * r1],  2
    vinserti32x4    m6,        [r0 + r6],      3

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    movu            xm7,       [r0 + 8]
    vinserti32x4    m7,        [r0 + r1 + 8],      1
    vinserti32x4    m7,        [r0 + 2 * r1 + 8],  2
    vinserti32x4    m7,        [r0 + r6 + 8],      3

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2],      xm6
    vextracti32x4   [r2 + r3],     m6,        1
    vextracti32x4   [r2 + 2 * r3], m6,        2
    vextracti32x4   [r2 + r7],     m6,        3
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PS_8x3_AVX512 0
    movu            xm6,       [r0]
    vinserti32x4    m6,        [r0 + r1],      1
    vinserti32x4    m6,        [r0 + 2 * r1],  2

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    movu            xm7,       [r0 + 8]
    vinserti32x4    m7,        [r0 + r1 + 8],      1
    vinserti32x4    m7,        [r0 + 2 * r1 + 8],  2

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2],      xm6
    vextracti32x4   [r2 + r3],     m6,        1
    vextracti32x4   [r2 + 2 * r3], m6,        2
%endmacro

%macro IPFILTER_CHROMA_PS_AVX512_8xN 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_ps_8x%1, 4,9,9
    shl             r1d, 1
    shl             r3d, 1
    mov             r4d, r4m
    mov             r5d, r5m

    lea             r6, [3 * r1]
    lea             r7, [3 * r3]
%ifdef PIC
    lea             r8, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r8 + r4 * 8]
    vpbroadcastd    m1, [r8 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8 m2, [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8 m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x4 m4, [INTERP_OFFSET_PS]
    vbroadcasti32x8 m5, [interp8_hpp_shuf1_store_avx512]

    mov               r8d,         %1
    sub               r0,          2
    test              r5d,         r5d
    jz                .loop
    sub               r0,          r1
    add               r8d,         3
    PROCESS_IPFILTER_CHROMA_PS_8x3_AVX512
    lea               r0,  [r0 + r6]
    lea               r2,  [r2 + r7]
    sub               r8d, 3

.loop:
    PROCESS_IPFILTER_CHROMA_PS_8x4_AVX512
    lea             r0,  [r0 + 4 * r1]
    lea             r2,  [r2 + 4 * r3]
    sub             r8d, 4
    jnz             .loop
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_CHROMA_PS_AVX512_8xN 4
IPFILTER_CHROMA_PS_AVX512_8xN 8
IPFILTER_CHROMA_PS_AVX512_8xN 12
IPFILTER_CHROMA_PS_AVX512_8xN 16
IPFILTER_CHROMA_PS_AVX512_8xN 32
IPFILTER_CHROMA_PS_AVX512_8xN 64
%endif

%macro PROCESS_IPFILTER_CHROMA_PS_24x4_AVX512 0
    ; register map
    ; m0 , m1 - interpolate coeff
    ; m2 , m3 - shuffle order table
    ; m4      - INTERP_OFFSET_PS
    ; m5      - shuffle store order table

    movu            ym6,       [r0]
    vinserti32x8    m6,        [r0 + r1],      1
    movu            ym7,       [r0 + 8]
    vinserti32x8    m7,        [r0 + r1 + 8],  1

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2],      ym6
    vextracti32x8   [r2 + r3], m6,        1

    movu            ym6,       [r0 + 2 * r1]
    vinserti32x8    m6,        [r0 + r6],      1
    movu            ym7,       [r0 + 2 * r1 + 8]
    vinserti32x8    m7,        [r0 + r6 + 8],  1

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2 + 2 * r3],        ym6
    vextracti32x8   [r2 + r7], m6,        1

    movu            xm6,       [r0 + mmsize/2]
    vinserti32x4    m6,        [r0 + r1 + mmsize/2],      1
    vinserti32x4    m6,        [r0 + 2 * r1 + mmsize/2],  2
    vinserti32x4    m6,        [r0 + r6 + mmsize/2],      3

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    movu            xm7,       [r0 + mmsize/2 + 8]
    vinserti32x4    m7,        [r0 + r1 + mmsize/2 + 8],      1
    vinserti32x4    m7,        [r0 + 2 * r1 + mmsize/2 + 8],  2
    vinserti32x4    m7,        [r0 + r6 + mmsize/2 + 8],      3

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2 + mmsize/2],      xm6
    vextracti32x4   [r2 + r3 + mmsize/2],     m6,        1
    vextracti32x4   [r2 + 2 * r3 + mmsize/2], m6,        2
    vextracti32x4   [r2 + r7 + mmsize/2],     m6,        3
%endmacro

%macro PROCESS_IPFILTER_CHROMA_PS_24x3_AVX512 0
    movu            ym6,       [r0]
    vinserti32x8    m6,        [r0 + r1],      1
    movu            ym7,       [r0 + 8]
    vinserti32x8    m7,        [r0 + r1 + 8],  1

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2],      ym6
    vextracti32x8   [r2 + r3], m6,        1

    movu            ym6,       [r0 + 2 * r1]
    movu            ym7,       [r0 + 2 * r1 + 8]

    pshufb          ym8,        ym6,        ym3
    pshufb          ym6,        ym2
    pmaddwd         ym6,        ym0
    pmaddwd         ym8,        ym1
    paddd           ym6,        ym8
    paddd           ym6,        ym4
    psrad           ym6,        INTERP_SHIFT_PS

    pshufb          ym8,        ym7,        ym3
    pshufb          ym7,        ym2
    pmaddwd         ym7,        ym0
    pmaddwd         ym8,        ym1
    paddd           ym7,        ym8
    paddd           ym7,        ym4
    psrad           ym7,        INTERP_SHIFT_PS

    packssdw        ym6,        ym7
    pshufb          ym6,        ym5
    movu            [r2 + 2 * r3],        ym6

    movu            xm6,       [r0 + mmsize/2]
    vinserti32x4    m6,        [r0 + r1 + mmsize/2],      1
    vinserti32x4    m6,        [r0 + 2 * r1 + mmsize/2],  2

    pshufb          m8,        m6,        m3
    pshufb          m6,        m2
    pmaddwd         m6,        m0
    pmaddwd         m8,        m1
    paddd           m6,        m8
    paddd           m6,        m4
    psrad           m6,        INTERP_SHIFT_PS

    movu            xm7,       [r0 + mmsize/2 + 8]
    vinserti32x4    m7,        [r0 + r1 + mmsize/2 + 8],      1
    vinserti32x4    m7,        [r0 + 2 * r1 + mmsize/2 + 8],  2

    pshufb          m8,        m7,        m3
    pshufb          m7,        m2
    pmaddwd         m7,        m0
    pmaddwd         m8,        m1
    paddd           m7,        m8
    paddd           m7,        m4
    psrad           m7,        INTERP_SHIFT_PS

    packssdw        m6,        m7
    pshufb          m6,        m5
    movu            [r2 + mmsize/2],      xm6
    vextracti32x4   [r2 + r3 + mmsize/2],     m6,        1
    vextracti32x4   [r2 + 2 * r3 + mmsize/2], m6,        2
%endmacro

%macro IPFILTER_CHROMA_PS_AVX512_24xN 1
INIT_ZMM avx512
cglobal interp_4tap_horiz_ps_24x%1, 4,9,9
    shl             r1d, 1
    shl             r3d, 1
    mov             r4d, r4m
    mov             r5d, r5m

    lea             r6, [3 * r1]
    lea             r7, [3 * r3]
%ifdef PIC
    lea             r8, [tab_ChromaCoeff]
    vpbroadcastd    m0, [r8 + r4 * 8]
    vpbroadcastd    m1, [r8 + r4 * 8 + 4]
%else
    vpbroadcastd    m0, [tab_ChromaCoeff + r4 * 8]
    vpbroadcastd    m1, [tab_ChromaCoeff + r4 * 8 + 4]
%endif
    vbroadcasti32x8 m2, [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8 m3, [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x4 m4, [INTERP_OFFSET_PS]
    vbroadcasti32x8 m5,[interp8_hpp_shuf1_store_avx512]

    mov               r8d,         %1
    sub               r0,          2
    test              r5d,         r5d
    jz                .loop
    sub               r0,          r1
    add               r8d,         3
    PROCESS_IPFILTER_CHROMA_PS_24x3_AVX512
    lea               r0,  [r0 + r6]
    lea               r2,  [r2 + r7]
    sub               r8d, 3

.loop:
    PROCESS_IPFILTER_CHROMA_PS_24x4_AVX512
    lea             r0,  [r0 + 4 * r1]
    lea             r2,  [r2 + 4 * r3]
    sub             r8d, 4
    jnz             .loop
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_CHROMA_PS_AVX512_24xN 32
IPFILTER_CHROMA_PS_AVX512_24xN 64
%endif
;-------------------------------------------------------------------------------------------------------------
; avx512 chroma_hps code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
; avx512 chroma_vps code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_CHROMA_VERT_PS_8x8_AVX512 0
    movu                  xm1,                [r0]
    lea                   r6,                 [r0 + 2 * r1]
    lea                   r8,                 [r0 + 4 * r1]
    lea                   r9,                 [r8 + 2 * r1]
    vinserti32x4          m1,                 [r6],                1
    vinserti32x4          m1,                 [r8],                2
    vinserti32x4          m1,                 [r9],                3
    movu                  xm3,                [r0 + r1]
    vinserti32x4          m3,                 [r6 + r1],           1
    vinserti32x4          m3,                 [r8 + r1],           2
    vinserti32x4          m3,                 [r9 + r1],           3
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 [r5]
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 [r5]

    movu                  xm4,                [r0 + 2 * r1]
    vinserti32x4          m4,                 [r6 + 2 * r1],       1
    vinserti32x4          m4,                 [r8 + 2 * r1],       2
    vinserti32x4          m4,                 [r9 + 2 * r1],       3
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 [r5]
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 [r5]

    movu                  xm5,                [r0 + r10]
    vinserti32x4          m5,                 [r6 + r10],          1
    vinserti32x4          m5,                 [r8 + r10],          2
    vinserti32x4          m5,                 [r9 + r10],          3
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 [r5 + mmsize]
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 [r5 + mmsize]
    paddd                 m1,                 m4

    movu                  xm4,                [r0 + 4 * r1]
    vinserti32x4          m4,                 [r6 + 4 * r1],       1
    vinserti32x4          m4,                 [r8 + 4 * r1],       2
    vinserti32x4          m4,                 [r9 + 4 * r1],       3
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 m9
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m9
    paddd                 m3,                 m5

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_PS
    psrad                 m1,                 INTERP_SHIFT_PS
    psrad                 m2,                 INTERP_SHIFT_PS
    psrad                 m3,                 INTERP_SHIFT_PS

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    movu                  [r2],               xm0
    movu                  [r2 + r3],          xm2
    vextracti32x4         [r2 + 2 * r3],      m0,                  1
    vextracti32x4         [r2 + r7],          m2,                  1
    lea                   r2,                 [r2 + 4 * r3]
    vextracti32x4         [r2],               m0,                  2
    vextracti32x4         [r2 + r3],          m2,                  2
    vextracti32x4         [r2 + 2 * r3],      m0,                  3
    vextracti32x4         [r2 + r7],          m2,                  3
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_4tap_vert_ps_8x8, 5, 11, 10
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_PS]
    lea                   r10,                [3 * r1]
    lea                   r7,                 [3 * r3]
    mova                  m8,                 [r5]
    mova                  m9,                 [r5 + mmsize]
    PROCESS_CHROMA_VERT_PS_8x8_AVX512
    RET
%endif

%macro FILTER_VER_PS_CHROMA_8xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_ps_8x%1, 5, 11, 10
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_PS]
    lea                   r10,                [3 * r1]
    lea                   r7,                 [3 * r3]
    mova                  m8,                 [r5]
    mova                  m9,                 [r5 + mmsize]
%rep %1/8 - 1
    PROCESS_CHROMA_VERT_PS_8x8_AVX512
    lea                   r0,                 [r8 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_PS_8x8_AVX512
    RET
%endmacro

%if ARCH_X86_64
FILTER_VER_PS_CHROMA_8xN_AVX512 16
FILTER_VER_PS_CHROMA_8xN_AVX512 32
FILTER_VER_PS_CHROMA_8xN_AVX512 64
%endif

%macro PROCESS_CHROMA_VERT_PS_16x4_AVX512 0
    movu                  ym1,                [r0]
    lea                   r6,                 [r0 + 2 * r1]
    vinserti32x8          m1,                 [r6],                1
    movu                  ym3,                [r0 + r1]
    vinserti32x8          m3,                 [r6 + r1],           1
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m8
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m8

    movu                  ym4,                [r0 + 2 * r1]
    vinserti32x8          m4,                 [r6 + 2 * r1],       1
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 m8
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m8

    movu                  ym5,                [r0 + r8]
    vinserti32x8          m5,                 [r6 + r8],           1
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 m9
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m9
    paddd                 m1,                 m4

    movu                  ym4,                [r0 + 4 * r1]
    vinserti32x8          m4,                 [r6 + 4 * r1],       1
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 m9
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m9
    paddd                 m3,                 m5

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_PS
    psrad                 m1,                 INTERP_SHIFT_PS
    psrad                 m2,                 INTERP_SHIFT_PS
    psrad                 m3,                 INTERP_SHIFT_PS

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    movu                  [r2],               ym0
    movu                  [r2 + r3],          ym2
    vextracti32x8         [r2 + 2 * r3],      m0,                  1
    vextracti32x8         [r2 + r7],          m2,                  1
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_4tap_vert_ps_16x4, 5, 9, 10
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_PS]
    lea                   r7,                 [3 * r3]
    lea                   r8,                 [3 * r1]
    mova                  m8,                 [r5]
    mova                  m9,                 [r5 + mmsize]
    PROCESS_CHROMA_VERT_PS_16x4_AVX512
    RET
%endif

%macro FILTER_VER_PS_CHROMA_16xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_ps_16x%1, 5, 9, 10
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_PS]
    lea                   r7,                 [3 * r3]
    lea                   r8,                 [3 * r1]
    mova                  m8,                 [r5]
    mova                  m9,                 [r5 + mmsize]
%rep %1/4 - 1
    PROCESS_CHROMA_VERT_PS_16x4_AVX512
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_PS_16x4_AVX512
    RET
%endmacro

%if ARCH_X86_64
FILTER_VER_PS_CHROMA_16xN_AVX512 8
FILTER_VER_PS_CHROMA_16xN_AVX512 12
FILTER_VER_PS_CHROMA_16xN_AVX512 16
FILTER_VER_PS_CHROMA_16xN_AVX512 24
FILTER_VER_PS_CHROMA_16xN_AVX512 32
FILTER_VER_PS_CHROMA_16xN_AVX512 64
%endif

%macro PROCESS_CHROMA_VERT_PS_24x8_AVX512 0
    movu                  ym1,                [r0]
    lea                   r6,                 [r0 + 2 * r1]
    lea                   r8,                 [r0 + 4 * r1]
    lea                   r9,                 [r8 + 2 * r1]

    movu                  ym10,               [r8]
    movu                  ym3,                [r0 + r1]
    movu                  ym12,               [r8 + r1]
    vinserti32x8          m1,                 [r6],                1
    vinserti32x8          m10,                [r9],                1
    vinserti32x8          m3,                 [r6 + r1],           1
    vinserti32x8          m12,                [r9 + r1],           1

    punpcklwd             m0,                 m1,                  m3
    punpcklwd             m9,                 m10,                 m12
    pmaddwd               m0,                 m16
    pmaddwd               m9,                 m16
    punpckhwd             m1,                 m3
    punpckhwd             m10,                m12
    pmaddwd               m1,                 m16
    pmaddwd               m10,                m16

    movu                  ym4,                [r0 + 2 * r1]
    movu                  ym13,               [r8 + 2 * r1]
    vinserti32x8          m4,                 [r6 + 2 * r1],       1
    vinserti32x8          m13,                [r9 + 2 * r1],       1
    punpcklwd             m2,                 m3,                  m4
    punpcklwd             m11,                m12,                 m13
    pmaddwd               m2,                 m16
    pmaddwd               m11,                m16
    punpckhwd             m3,                 m4
    punpckhwd             m12,                m13
    pmaddwd               m3,                 m16
    pmaddwd               m12,                m16

    movu                  ym5,                [r0 + r10]
    vinserti32x8          m5,                 [r6 + r10],          1
    movu                  ym14,               [r8 + r10]
    vinserti32x8          m14,                [r9 + r10],          1
    punpcklwd             m6,                 m4,                  m5
    punpcklwd             m15,                m13,                 m14
    pmaddwd               m6,                 m17
    pmaddwd               m15,                m17
    paddd                 m0,                 m6
    paddd                 m9,                 m15
    punpckhwd             m4,                 m5
    punpckhwd             m13,                m14
    pmaddwd               m4,                 m17
    pmaddwd               m13,                m17
    paddd                 m1,                 m4
    paddd                 m10,                m13

    movu                  ym4,                [r0 + 4 * r1]
    vinserti32x8          m4,                 [r6 + 4 * r1],       1
    movu                  ym13,               [r8 + 4 * r1]
    vinserti32x8          m13,                [r9 + 4 * r1],       1
    punpcklwd             m6,                 m5,                  m4
    punpcklwd             m15,                m14,                 m13
    pmaddwd               m6,                 m17
    pmaddwd               m15,                m17
    paddd                 m2,                 m6
    paddd                 m11,                m15
    punpckhwd             m5,                 m4
    punpckhwd             m14,                m13
    pmaddwd               m5,                 m17
    pmaddwd               m14,                m17
    paddd                 m3,                 m5
    paddd                 m12,                m14

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7
    paddd                 m9,                 m7
    paddd                 m10,                m7
    paddd                 m11,                m7
    paddd                 m12,                m7

    psrad                 m0,                 INTERP_SHIFT_PS
    psrad                 m1,                 INTERP_SHIFT_PS
    psrad                 m2,                 INTERP_SHIFT_PS
    psrad                 m3,                 INTERP_SHIFT_PS
    psrad                 m9,                 INTERP_SHIFT_PS
    psrad                 m10,                INTERP_SHIFT_PS
    psrad                 m11,                INTERP_SHIFT_PS
    psrad                 m12,                INTERP_SHIFT_PS

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packssdw              m9,                 m10
    packssdw              m11,                m12
    movu                  [r2],               ym0
    movu                  [r2 + r3],          ym2
    vextracti32x8         [r2 + 2 * r3],      m0,                  1
    vextracti32x8         [r2 + r7],          m2,                  1
    lea                   r11,                [r2 + 4 * r3]
    movu                  [r11],              ym9
    movu                  [r11 + r3],         ym11
    vextracti32x8         [r11 + 2 * r3],     m9,                  1
    vextracti32x8         [r11 + r7],         m11,                 1

    movu                  xm1,                [r0 + mmsize/2]
    vinserti32x4          m1,                 [r6 + mmsize/2],                1
    vinserti32x4          m1,                 [r8 + mmsize/2],                2
    vinserti32x4          m1,                 [r9 + mmsize/2],                3
    movu                  xm3,                [r0 + r1 + mmsize/2]
    vinserti32x4          m3,                 [r6 + r1 + mmsize/2],           1
    vinserti32x4          m3,                 [r8 + r1 + mmsize/2],           2
    vinserti32x4          m3,                 [r9 + r1 + mmsize/2],           3
    punpcklwd             m0,                 m1,                             m3
    pmaddwd               m0,                 m16
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m16

    movu                  xm4,                [r0 + 2 * r1 + mmsize/2]
    vinserti32x4          m4,                 [r6 + 2 * r1 + mmsize/2],       1
    vinserti32x4          m4,                 [r8 + 2 * r1 + mmsize/2],       2
    vinserti32x4          m4,                 [r9 + 2 * r1 + mmsize/2],       3
    punpcklwd             m2,                 m3,                             m4
    pmaddwd               m2,                 m16
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m16

    movu                  xm5,                [r0 + r10 + mmsize/2]
    vinserti32x4          m5,                 [r6 + r10 + mmsize/2],          1
    vinserti32x4          m5,                 [r8 + r10 + mmsize/2],          2
    vinserti32x4          m5,                 [r9 + r10 + mmsize/2],          3
    punpcklwd             m6,                 m4,                             m5
    pmaddwd               m6,                 m17
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m17
    paddd                 m1,                 m4

    movu                  xm4,                [r0 + 4 * r1 + mmsize/2]
    vinserti32x4          m4,                 [r6 + 4 * r1 + mmsize/2],       1
    vinserti32x4          m4,                 [r8 + 4 * r1 + mmsize/2],       2
    vinserti32x4          m4,                 [r9 + 4 * r1 + mmsize/2],       3
    punpcklwd             m6,                 m5,                             m4
    pmaddwd               m6,                 m17
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m17
    paddd                 m3,                 m5

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_PS
    psrad                 m1,                 INTERP_SHIFT_PS
    psrad                 m2,                 INTERP_SHIFT_PS
    psrad                 m3,                 INTERP_SHIFT_PS

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    movu                  [r2 + mmsize/2],               xm0
    movu                  [r2 + r3 + mmsize/2],          xm2
    vextracti32x4         [r2 + 2 * r3 + mmsize/2],      m0,                  1
    vextracti32x4         [r2 + r7 + mmsize/2],          m2,                  1
    lea                   r2,                            [r2 + 4 * r3]
    vextracti32x4         [r2 + mmsize/2],               m0,                  2
    vextracti32x4         [r2 + r3 + mmsize/2],          m2,                  2
    vextracti32x4         [r2 + 2 * r3 + mmsize/2],      m0,                  3
    vextracti32x4         [r2 + r7 + mmsize/2],          m2,                  3
%endmacro

%macro FILTER_VER_PS_CHROMA_24xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_ps_24x%1, 5, 12, 18
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_PS]
    lea                   r10,                [3 * r1]
    lea                   r7,                 [3 * r3]
    mova                  m16,                [r5]
    mova                  m17,                [r5 + mmsize]
%rep %1/8 - 1
    PROCESS_CHROMA_VERT_PS_24x8_AVX512
    lea                   r0,                 [r8 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_PS_24x8_AVX512
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_PS_CHROMA_24xN_AVX512 32
    FILTER_VER_PS_CHROMA_24xN_AVX512 64
%endif

%macro PROCESS_CHROMA_VERT_PS_32x2_AVX512 0
    movu                  m1,                 [r0]
    movu                  m3,                 [r0 + r1]
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m9
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m9

    movu                  m4,                 [r0 + 2 * r1]
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 m9
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m9

    lea                   r0,                 [r0 + 2 * r1]
    movu                  m5,                 [r0 + r1]
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 m10
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m10
    paddd                 m1,                 m4

    movu                  m4,                 [r0 + 2 * r1]
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 m10
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m10
    paddd                 m3,                 m5

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7
    psrad                 m0,                 INTERP_SHIFT_PS
    psrad                 m1,                 INTERP_SHIFT_PS
    psrad                 m2,                 INTERP_SHIFT_PS
    psrad                 m3,                 INTERP_SHIFT_PS

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    movu                  [r2],               m0
    movu                  [r2 + r3],          m2
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_PS_CHROMA_32xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_ps_32x%1, 5, 7, 11
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_PS]
    mova                  m9,                 [r5]
    mova                  m10,                [r5 + mmsize]
%rep %1/2 - 1
    PROCESS_CHROMA_VERT_PS_32x2_AVX512
    lea                   r2,                 [r2 + 2 * r3]
%endrep
    PROCESS_CHROMA_VERT_PS_32x2_AVX512
    RET
%endmacro

%if ARCH_X86_64
FILTER_VER_PS_CHROMA_32xN_AVX512 8
FILTER_VER_PS_CHROMA_32xN_AVX512 16
FILTER_VER_PS_CHROMA_32xN_AVX512 24
FILTER_VER_PS_CHROMA_32xN_AVX512 32
FILTER_VER_PS_CHROMA_32xN_AVX512 48
FILTER_VER_PS_CHROMA_32xN_AVX512 64
%endif

%macro PROCESS_CHROMA_VERT_PS_48x4_AVX512 0
    movu                  m1,                 [r0]
    lea                   r6,                 [r0 + 2 * r1]
    movu                  m10,                [r6]
    movu                  m3,                 [r0 + r1]
    movu                  m12,                [r6 + r1]
    punpcklwd             m0,                 m1,                  m3
    punpcklwd             m9,                 m10,                 m12
    pmaddwd               m0,                 m16
    pmaddwd               m9,                 m16
    punpckhwd             m1,                 m3
    punpckhwd             m10,                m12
    pmaddwd               m1,                 m16
    pmaddwd               m10,                m16

    movu                  m4,                 [r0 + 2 * r1]
    movu                  m13,                [r6 + 2 * r1]
    punpcklwd             m2,                 m3,                  m4
    punpcklwd             m11,                m12,                 m13
    pmaddwd               m2,                 m16
    pmaddwd               m11,                m16
    punpckhwd             m3,                 m4
    punpckhwd             m12,                m13
    pmaddwd               m3,                 m16
    pmaddwd               m12,                m16

    movu                  m5,                 [r0 + r7]
    movu                  m14,                [r6 + r7]
    punpcklwd             m6,                 m4,                  m5
    punpcklwd             m15,                m13,                 m14
    pmaddwd               m6,                 m17
    pmaddwd               m15,                m17
    paddd                 m0,                 m6
    paddd                 m9,                 m15
    punpckhwd             m4,                 m5
    punpckhwd             m13,                m14
    pmaddwd               m4,                 m17
    pmaddwd               m13,                m17
    paddd                 m1,                 m4
    paddd                 m10,                m13

    movu                  m4,                 [r0 + 4 * r1]
    movu                  m13,                [r6 + 4 * r1]
    punpcklwd             m6,                 m5,                  m4
    punpcklwd             m15,                m14,                 m13
    pmaddwd               m6,                 m17
    pmaddwd               m15,                m17
    paddd                 m2,                 m6
    paddd                 m11,                m15
    punpckhwd             m5,                 m4
    punpckhwd             m14,                m13
    pmaddwd               m5,                 m17
    pmaddwd               m14,                m17
    paddd                 m3,                 m5
    paddd                 m12,                m14

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7
    paddd                 m9,                 m7
    paddd                 m10,                m7
    paddd                 m11,                m7
    paddd                 m12,                m7

    psrad                 m0,                 INTERP_SHIFT_PS
    psrad                 m1,                 INTERP_SHIFT_PS
    psrad                 m2,                 INTERP_SHIFT_PS
    psrad                 m3,                 INTERP_SHIFT_PS
    psrad                 m9,                 INTERP_SHIFT_PS
    psrad                 m10,                INTERP_SHIFT_PS
    psrad                 m11,                INTERP_SHIFT_PS
    psrad                 m12,                INTERP_SHIFT_PS

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packssdw              m9,                 m10
    packssdw              m11,                m12
    movu                  [r2],               m0
    movu                  [r2 + r3],          m2
    movu                  [r2 + 2 * r3],      m9
    movu                  [r2 + r8],          m11

    movu                  ym1,                [r0 + mmsize]
    vinserti32x8          m1,                 [r6 + mmsize],       1
    movu                  ym3,                [r0 + r1 + mmsize]
    vinserti32x8          m3,                 [r6 + r1 + mmsize],  1
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m16
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m16

    movu                  ym4,                [r0 + 2 * r1 + mmsize]
    vinserti32x8          m4,                 [r6 + 2 * r1 + mmsize],  1
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 m16
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m16

    movu                  ym5,                [r0 + r7 + mmsize]
    vinserti32x8          m5,                 [r6 + r7 + mmsize],  1
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 m17
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m17
    paddd                 m1,                 m4

    movu                  ym4,                [r0 + 4 * r1 + mmsize]
    vinserti32x8          m4,                 [r6 + 4 * r1 + mmsize],  1
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 m17
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m17
    paddd                 m3,                 m5

    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_PS
    psrad                 m1,                 INTERP_SHIFT_PS
    psrad                 m2,                 INTERP_SHIFT_PS
    psrad                 m3,                 INTERP_SHIFT_PS

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    movu                  [r2 + mmsize],               ym0
    movu                  [r2 + r3 + mmsize],          ym2
    vextracti32x8         [r2 + 2 * r3 + mmsize],      m0,                  1
    vextracti32x8         [r2 + r8 + mmsize],          m2,                  1
%endmacro

%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_4tap_vert_ps_48x64, 5, 9, 18
     add                   r1d,                r1d
     add                   r3d,                r3d
     sub                   r0,                 r1
     shl                   r4d,                7
%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    lea                   r7,                 [3 * r1]
    lea                   r8,                 [3 * r3]
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_PS]
    mova                  m16,                [r5]
    mova                  m17,                [r5 + mmsize]
%rep 15
    PROCESS_CHROMA_VERT_PS_48x4_AVX512
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_PS_48x4_AVX512
    RET
%endif

%macro PROCESS_CHROMA_VERT_PS_64x2_AVX512 0
    movu                 m1,                  [r0]
    movu                 m3,                  [r0 + r1]
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 m9,                  [r0 + mmsize]
    movu                 m11,                 [r0 + r1 + mmsize]
    punpcklwd            m8,                  m9,                     m11
    pmaddwd              m8,                  m15
    punpckhwd            m9,                  m11
    pmaddwd              m9,                  m15

    movu                 m4,                  [r0 + 2 * r1]
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 m12,                 [r0 + 2 * r1 + mmsize]
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m15
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m15

    lea                  r0,                  [r0 + 2 * r1]
    movu                 m5,                  [r0 + r1]
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    paddd                m0,                  m6
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16
    paddd                m1,                  m4

    movu                 m13,                 [r0 + r1 + mmsize]
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m16
    paddd                m8,                  m14
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m16
    paddd                m9,                  m12

    movu                 m4,                  [r0 + 2 * r1]
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    paddd                m2,                  m6
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16
    paddd                m3,                  m5

    movu                 m12,                 [r0 + 2 * r1 + mmsize]
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m16
    paddd                m10,                 m14
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m16
    paddd                m11,                 m13

    paddd                m0,                  m7
    paddd                m1,                  m7
    paddd                m2,                  m7
    paddd                m3,                  m7
    paddd                m8,                  m7
    paddd                m9,                  m7
    paddd                m10,                 m7
    paddd                m11,                 m7

    psrad                m0,                  INTERP_SHIFT_PS
    psrad                m1,                  INTERP_SHIFT_PS
    psrad                m2,                  INTERP_SHIFT_PS
    psrad                m3,                  INTERP_SHIFT_PS
    psrad                m8,                  INTERP_SHIFT_PS
    psrad                m9,                  INTERP_SHIFT_PS
    psrad                m10,                 INTERP_SHIFT_PS
    psrad                m11,                 INTERP_SHIFT_PS

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    packssdw             m8,                  m9
    packssdw             m10,                 m11
    movu                 [r2],                m0
    movu                 [r2 + r3],           m2
    movu                 [r2 + mmsize],       m8
    movu                 [r2 + r3 + mmsize],  m10
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_PS_CHROMA_64xN_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_ps_64x%1, 5, 7, 17
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7

%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    lea                   r5,                 [r5 + r4]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
%endif
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_PS]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + mmsize]

%rep %1/2 - 1
    PROCESS_CHROMA_VERT_PS_64x2_AVX512
    lea                   r2,                 [r2 + 2 * r3]
%endrep
    PROCESS_CHROMA_VERT_PS_64x2_AVX512
    RET
%endmacro

%if ARCH_X86_64
FILTER_VER_PS_CHROMA_64xN_AVX512 16
FILTER_VER_PS_CHROMA_64xN_AVX512 32
FILTER_VER_PS_CHROMA_64xN_AVX512 48
FILTER_VER_PS_CHROMA_64xN_AVX512 64
%endif
;-------------------------------------------------------------------------------------------------------------
; avx512 chroma_vps code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
; avx512 chroma_vsp and chroma_vss code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_CHROMA_VERT_S_8x8_AVX512 1
    movu                  xm1,                [r0]
    lea                   r6,                 [r0 + 2 * r1]
    lea                   r8,                 [r0 + 4 * r1]
    lea                   r9,                 [r8 + 2 * r1]
    vinserti32x4          m1,                 [r6],                1
    vinserti32x4          m1,                 [r8],                2
    vinserti32x4          m1,                 [r9],                3
    movu                  xm3,                [r0 + r1]
    vinserti32x4          m3,                 [r6 + r1],           1
    vinserti32x4          m3,                 [r8 + r1],           2
    vinserti32x4          m3,                 [r9 + r1],           3
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m8
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m8

    movu                  xm4,                [r0 + 2 * r1]
    vinserti32x4          m4,                 [r6 + 2 * r1],       1
    vinserti32x4          m4,                 [r8 + 2 * r1],       2
    vinserti32x4          m4,                 [r9 + 2 * r1],       3
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 m8
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m8

    movu                  xm5,                [r0 + r10]
    vinserti32x4          m5,                 [r6 + r10],          1
    vinserti32x4          m5,                 [r8 + r10],          2
    vinserti32x4          m5,                 [r9 + r10],          3
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 m9
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m9
    paddd                 m1,                 m4

    movu                  xm4,                [r0 + 4 * r1]
    vinserti32x4          m4,                 [r6 + 4 * r1],       1
    vinserti32x4          m4,                 [r8 + 4 * r1],       2
    vinserti32x4          m4,                 [r9 + 4 * r1],       3
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 m9
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m9
    paddd                 m3,                 m5

%ifidn %1,sp
    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_SP
    psrad                 m1,                 INTERP_SHIFT_SP
    psrad                 m2,                 INTERP_SHIFT_SP
    psrad                 m3,                 INTERP_SHIFT_SP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    CLIPW2                m0,                 m2,                  m10,                 m11
%else
    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6
    packssdw              m0,                 m1
    packssdw              m2,                 m3
%endif

    movu                  [r2],               xm0
    movu                  [r2 + r3],          xm2
    vextracti32x4         [r2 + 2 * r3],      m0,                  1
    vextracti32x4         [r2 + r7],          m2,                  1
    lea                   r2,                 [r2 + 4 * r3]
    vextracti32x4         [r2],               m0,                  2
    vextracti32x4         [r2 + r3],          m2,                  2
    vextracti32x4         [r2 + 2 * r3],      m0,                  3
    vextracti32x4         [r2 + r7],          m2,                  3
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro CHROMA_VERT_S_8x8_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_8x8, 5, 11, 12
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7
%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    mova                  m8,                 [r5 + r4]
    mova                  m9,                 [r5 + r4 + mmsize]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
    mova                  m8,                 [r5]
    mova                  m9,                 [r5 + mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_SP]
    pxor                  m10,                m10
    vbroadcasti32x8       m11,                [pw_pixel_max]
%endif
    lea                   r10,                [3 * r1]
    lea                   r7,                 [3 * r3]

    PROCESS_CHROMA_VERT_S_8x8_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    CHROMA_VERT_S_8x8_AVX512 ss
    CHROMA_VERT_S_8x8_AVX512 sp
%endif
%macro FILTER_VER_S_CHROMA_8xN_AVX512 2
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_8x%2, 5, 11, 10
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7
%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    mova                  m8,                 [r5 + r4]
    mova                  m9,                 [r5 + r4 + mmsize]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
    mova                  m8,                 [r5]
    mova                  m9,                 [r5 + mmsize]
%endif

%ifidn %1, sp
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_SP]
    pxor                  m10,                m10
    vbroadcasti32x8       m11,                [pw_pixel_max]
%endif
    lea                   r10,                [3 * r1]
    lea                   r7,                 [3 * r3]

%rep %2/8 - 1
    PROCESS_CHROMA_VERT_S_8x8_AVX512 %1
    lea                   r0,                 [r8 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_S_8x8_AVX512 %1
    RET
%endmacro
%if ARCH_X86_64
    FILTER_VER_S_CHROMA_8xN_AVX512 ss, 16
    FILTER_VER_S_CHROMA_8xN_AVX512 ss, 32
    FILTER_VER_S_CHROMA_8xN_AVX512 ss, 64
    FILTER_VER_S_CHROMA_8xN_AVX512 sp, 16
    FILTER_VER_S_CHROMA_8xN_AVX512 sp, 32
    FILTER_VER_S_CHROMA_8xN_AVX512 sp, 64
%endif
%macro PROCESS_CHROMA_VERT_S_16x4_AVX512 1
    movu                  ym1,                [r0]
    lea                   r6,                 [r0 + 2 * r1]
    vinserti32x8          m1,                 [r6],                1
    movu                  ym3,                [r0 + r1]
    vinserti32x8          m3,                 [r6 + r1],           1
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m8
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m8

    movu                  ym4,                [r0 + 2 * r1]
    vinserti32x8          m4,                 [r6 + 2 * r1],       1
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 m8
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m8

    movu                  ym5,                [r0 + r8]
    vinserti32x8          m5,                 [r6 + r8],           1
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 m9
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m9
    paddd                 m1,                 m4

    movu                  ym4,                [r0 + 4 * r1]
    vinserti32x8          m4,                 [r6 + 4 * r1],       1
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 m9
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m9
    paddd                 m3,                 m5

%ifidn %1,sp
    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_SP
    psrad                 m1,                 INTERP_SHIFT_SP
    psrad                 m2,                 INTERP_SHIFT_SP
    psrad                 m3,                 INTERP_SHIFT_SP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    CLIPW2                m0,                 m2,                  m10,                 m11
%else
    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6
    packssdw              m0,                 m1
    packssdw              m2,                 m3
%endif

    movu                  [r2],               ym0
    movu                  [r2 + r3],          ym2
    vextracti32x8         [r2 + 2 * r3],      m0,                  1
    vextracti32x8         [r2 + r7],          m2,                  1
%endmacro

;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro CHROMA_VERT_S_16x4_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_16x4, 5, 9, 12
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7
%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    mova                  m8,                 [r5 + r4]
    mova                  m9,                 [r5 + r4 + mmsize]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
    mova                  m8,                 [r5]
    mova                  m9,                 [r5 + mmsize]
%endif

%ifidn %1, sp
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_SP]
    pxor                  m10,                m10
    vbroadcasti32x8       m11,                [pw_pixel_max]
%endif
    lea                   r7,                 [3 * r3]
    lea                   r8,                 [3 * r1]
    PROCESS_CHROMA_VERT_S_16x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    CHROMA_VERT_S_16x4_AVX512 ss
    CHROMA_VERT_S_16x4_AVX512 sp
%endif
%macro FILTER_VER_S_CHROMA_16xN_AVX512 2
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_16x%2, 5, 9, 12
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7
%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    mova                  m8,                 [r5 + r4]
    mova                  m9,                 [r5 + r4 + mmsize]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
    mova                  m8,                 [r5]
    mova                  m9,                 [r5 + mmsize]
%endif

%ifidn %1, sp
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_SP]
    pxor                  m10,                m10
    vbroadcasti32x8       m11,                [pw_pixel_max]
%endif
    lea                   r7,                 [3 * r3]
    lea                   r8,                 [3 * r1]
%rep %2/4 - 1
    PROCESS_CHROMA_VERT_S_16x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_S_16x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 8
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 12
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 16
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 24
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 32
    FILTER_VER_S_CHROMA_16xN_AVX512 ss, 64
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 8
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 12
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 16
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 24
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 32
    FILTER_VER_S_CHROMA_16xN_AVX512 sp, 64
%endif

%macro PROCESS_CHROMA_VERT_S_24x8_AVX512 1
    movu                  ym1,                [r0]
    lea                   r6,                 [r0 + 2 * r1]
    lea                   r8,                 [r0 + 4 * r1]
    lea                   r9,                 [r8 + 2 * r1]

    movu                  ym10,               [r8]
    movu                  ym3,                [r0 + r1]
    movu                  ym12,               [r8 + r1]
    vinserti32x8          m1,                 [r6],                1
    vinserti32x8          m10,                [r9],                1
    vinserti32x8          m3,                 [r6 + r1],           1
    vinserti32x8          m12,                [r9 + r1],           1

    punpcklwd             m0,                 m1,                  m3
    punpcklwd             m9,                 m10,                 m12
    pmaddwd               m0,                 m16
    pmaddwd               m9,                 m16
    punpckhwd             m1,                 m3
    punpckhwd             m10,                m12
    pmaddwd               m1,                 m16
    pmaddwd               m10,                m16

    movu                  ym4,                [r0 + 2 * r1]
    movu                  ym13,               [r8 + 2 * r1]
    vinserti32x8          m4,                 [r6 + 2 * r1],       1
    vinserti32x8          m13,                [r9 + 2 * r1],       1
    punpcklwd             m2,                 m3,                  m4
    punpcklwd             m11,                m12,                 m13
    pmaddwd               m2,                 m16
    pmaddwd               m11,                m16
    punpckhwd             m3,                 m4
    punpckhwd             m12,                m13
    pmaddwd               m3,                 m16
    pmaddwd               m12,                m16

    movu                  ym5,                [r0 + r10]
    vinserti32x8          m5,                 [r6 + r10],          1
    movu                  ym14,               [r8 + r10]
    vinserti32x8          m14,                [r9 + r10],          1
    punpcklwd             m6,                 m4,                  m5
    punpcklwd             m15,                m13,                 m14
    pmaddwd               m6,                 m17
    pmaddwd               m15,                m17
    paddd                 m0,                 m6
    paddd                 m9,                 m15
    punpckhwd             m4,                 m5
    punpckhwd             m13,                m14
    pmaddwd               m4,                 m17
    pmaddwd               m13,                m17
    paddd                 m1,                 m4
    paddd                 m10,                m13

    movu                  ym4,                [r0 + 4 * r1]
    vinserti32x8          m4,                 [r6 + 4 * r1],       1
    movu                  ym13,               [r8 + 4 * r1]
    vinserti32x8          m13,                [r9 + 4 * r1],       1
    punpcklwd             m6,                 m5,                  m4
    punpcklwd             m15,                m14,                 m13
    pmaddwd               m6,                 m17
    pmaddwd               m15,                m17
    paddd                 m2,                 m6
    paddd                 m11,                m15
    punpckhwd             m5,                 m4
    punpckhwd             m14,                m13
    pmaddwd               m5,                 m17
    pmaddwd               m14,                m17
    paddd                 m3,                 m5
    paddd                 m12,                m14

%ifidn %1,sp
    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7
    paddd                 m9,                 m7
    paddd                 m10,                m7
    paddd                 m11,                m7
    paddd                 m12,                m7

    psrad                 m0,                 INTERP_SHIFT_SP
    psrad                 m1,                 INTERP_SHIFT_SP
    psrad                 m2,                 INTERP_SHIFT_SP
    psrad                 m3,                 INTERP_SHIFT_SP
    psrad                 m9,                 INTERP_SHIFT_SP
    psrad                 m10,                INTERP_SHIFT_SP
    psrad                 m11,                INTERP_SHIFT_SP
    psrad                 m12,                INTERP_SHIFT_SP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packssdw              m9,                 m10
    packssdw              m11,                m12
    CLIPW2                m0,                 m2,                m18,                m19
    CLIPW2                m9,                 m11,               m18,                m19
%else
    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6
    psrad                 m9,                 6
    psrad                 m10,                6
    psrad                 m11,                6
    psrad                 m12,                6

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packssdw              m9,                 m10
    packssdw              m11,                m12
%endif

    movu                  [r2],               ym0
    movu                  [r2 + r3],          ym2
    vextracti32x8         [r2 + 2 * r3],      m0,                  1
    vextracti32x8         [r2 + r7],          m2,                  1
    lea                   r11,                [r2 + 4 * r3]
    movu                  [r11],              ym9
    movu                  [r11 + r3],         ym11
    vextracti32x8         [r11 + 2 * r3],     m9,                  1
    vextracti32x8         [r11 + r7],         m11,                 1

    movu                  xm1,                [r0 + mmsize/2]
    vinserti32x4          m1,                 [r6 + mmsize/2],                1
    vinserti32x4          m1,                 [r8 + mmsize/2],                2
    vinserti32x4          m1,                 [r9 + mmsize/2],                3
    movu                  xm3,                [r0 + r1 + mmsize/2]
    vinserti32x4          m3,                 [r6 + r1 + mmsize/2],           1
    vinserti32x4          m3,                 [r8 + r1 + mmsize/2],           2
    vinserti32x4          m3,                 [r9 + r1 + mmsize/2],           3
    punpcklwd             m0,                 m1,                             m3
    pmaddwd               m0,                 m16
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m16

    movu                  xm4,                [r0 + 2 * r1 + mmsize/2]
    vinserti32x4          m4,                 [r6 + 2 * r1 + mmsize/2],       1
    vinserti32x4          m4,                 [r8 + 2 * r1 + mmsize/2],       2
    vinserti32x4          m4,                 [r9 + 2 * r1 + mmsize/2],       3
    punpcklwd             m2,                 m3,                             m4
    pmaddwd               m2,                 m16
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m16

    movu                  xm5,                [r0 + r10 + mmsize/2]
    vinserti32x4          m5,                 [r6 + r10 + mmsize/2],          1
    vinserti32x4          m5,                 [r8 + r10 + mmsize/2],          2
    vinserti32x4          m5,                 [r9 + r10 + mmsize/2],          3
    punpcklwd             m6,                 m4,                             m5
    pmaddwd               m6,                 m17
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m17
    paddd                 m1,                 m4

    movu                  xm4,                [r0 + 4 * r1 + mmsize/2]
    vinserti32x4          m4,                 [r6 + 4 * r1 + mmsize/2],       1
    vinserti32x4          m4,                 [r8 + 4 * r1 + mmsize/2],       2
    vinserti32x4          m4,                 [r9 + 4 * r1 + mmsize/2],       3
    punpcklwd             m6,                 m5,                             m4
    pmaddwd               m6,                 m17
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m17
    paddd                 m3,                 m5

%ifidn %1,sp
    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_SP
    psrad                 m1,                 INTERP_SHIFT_SP
    psrad                 m2,                 INTERP_SHIFT_SP
    psrad                 m3,                 INTERP_SHIFT_SP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    CLIPW2                m0,                 m2,                 m18,                m19
%else
    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6

    packssdw              m0,                 m1
    packssdw              m2,                 m3
%endif

    movu                  [r2 + mmsize/2],               xm0
    movu                  [r2 + r3 + mmsize/2],          xm2
    vextracti32x4         [r2 + 2 * r3 + mmsize/2],      m0,                  1
    vextracti32x4         [r2 + r7 + mmsize/2],          m2,                  1
    lea                   r2,                            [r2 + 4 * r3]
    vextracti32x4         [r2 + mmsize/2],               m0,                  2
    vextracti32x4         [r2 + r3 + mmsize/2],          m2,                  2
    vextracti32x4         [r2 + 2 * r3 + mmsize/2],      m0,                  3
    vextracti32x4         [r2 + r7 + mmsize/2],          m2,                  3
%endmacro
%macro FILTER_VER_S_CHROMA_24xN_AVX512 2
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_24x%2, 5, 12, 20
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7
%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    mova                  m16,                [r5 + r4]
    mova                  m17,                [r5 + r4 + mmsize]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
    mova                  m16,                [r5]
    mova                  m17,                [r5 + mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_SP]
    pxor                  m18,                m18
    vbroadcasti32x8       m19,                [pw_pixel_max]
%endif
    lea                   r10,                [3 * r1]
    lea                   r7,                 [3 * r3]
%rep %2/8 - 1
    PROCESS_CHROMA_VERT_S_24x8_AVX512 %1
    lea                   r0,                 [r8 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_S_24x8_AVX512 %1
    RET
%endmacro
%if ARCH_X86_64
    FILTER_VER_S_CHROMA_24xN_AVX512 ss, 32
    FILTER_VER_S_CHROMA_24xN_AVX512 ss, 64
    FILTER_VER_S_CHROMA_24xN_AVX512 sp, 32
    FILTER_VER_S_CHROMA_24xN_AVX512 sp, 64
%endif

%macro PROCESS_CHROMA_VERT_S_32x4_AVX512 1
    movu                  m1,                 [r0]
    lea                   r6,                 [r0 + 2 * r1]
    movu                  m10,                [r6]
    movu                  m3,                 [r0 + r1]
    movu                  m12,                [r6 + r1]
    punpcklwd             m0,                 m1,                  m3
    punpcklwd             m9,                 m10,                 m12
    pmaddwd               m0,                 m16
    pmaddwd               m9,                 m16
    punpckhwd             m1,                 m3
    punpckhwd             m10,                m12
    pmaddwd               m1,                 m16
    pmaddwd               m10,                m16
    movu                  m4,                 [r0 + 2 * r1]
    movu                  m13,                [r6 + 2 * r1]
    punpcklwd             m2,                 m3,                  m4
    punpcklwd             m11,                m12,                 m13
    pmaddwd               m2,                 m16
    pmaddwd               m11,                m16
    punpckhwd             m3,                 m4
    punpckhwd             m12,                m13
    pmaddwd               m3,                 m16
    pmaddwd               m12,                m16

    movu                  m5,                 [r0 + r7]
    movu                  m14,                [r6 + r7]
    punpcklwd             m6,                 m4,                  m5
    punpcklwd             m15,                m13,                 m14
    pmaddwd               m6,                 m17
    pmaddwd               m15,                m17
    paddd                 m0,                 m6
    paddd                 m9,                 m15
    punpckhwd             m4,                 m5
    punpckhwd             m13,                m14
    pmaddwd               m4,                 m17
    pmaddwd               m13,                m17
    paddd                 m1,                 m4
    paddd                 m10,                m13

    movu                  m4,                 [r0 + 4 * r1]
    movu                  m13,                [r6 + 4 * r1]
    punpcklwd             m6,                 m5,                  m4
    punpcklwd             m15,                m14,                 m13
    pmaddwd               m6,                 m17
    pmaddwd               m15,                m17
    paddd                 m2,                 m6
    paddd                 m11,                m15
    punpckhwd             m5,                 m4
    punpckhwd             m14,                m13
    pmaddwd               m5,                 m17
    pmaddwd               m14,                m17
    paddd                 m3,                 m5
    paddd                 m12,                m14
%ifidn %1,sp
    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7
    paddd                 m9,                 m7
    paddd                 m10,                m7
    paddd                 m11,                m7
    paddd                 m12,                m7

    psrad                 m0,                 INTERP_SHIFT_SP
    psrad                 m1,                 INTERP_SHIFT_SP
    psrad                 m2,                 INTERP_SHIFT_SP
    psrad                 m3,                 INTERP_SHIFT_SP
    psrad                 m9,                 INTERP_SHIFT_SP
    psrad                 m10,                INTERP_SHIFT_SP
    psrad                 m11,                INTERP_SHIFT_SP
    psrad                 m12,                INTERP_SHIFT_SP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packssdw              m9,                 m10
    packssdw              m11,                m12
    CLIPW2                m0,                 m2,                m18,              m19
    CLIPW2                m9,                 m11,               m18,              m19
%else
    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6
    psrad                 m9,                 6
    psrad                 m10,                6
    psrad                 m11,                6
    psrad                 m12,                6

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packssdw              m9,                 m10
    packssdw              m11,                m12
%endif

    movu                  [r2],               m0
    movu                  [r2 + r3],          m2
    movu                  [r2 + 2 * r3],      m9
    movu                  [r2 + r8],          m11
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_CHROMA_32xN_AVX512 2
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_32x%2, 5, 9, 20
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7
%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    mova                  m16,                [r5 + r4]
    mova                  m17,                [r5 + r4 + mmsize]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
    mova                  m16,                [r5]
    mova                  m17,                [r5 + mmsize]
%endif
    lea                   r7,                 [3 * r1]
    lea                   r8,                 [3 * r3]
%ifidn %1, sp
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_SP]
    pxor                  m18,                m18
    vbroadcasti32x8       m19,                [pw_pixel_max]
%endif

%rep %2/4 - 1
    PROCESS_CHROMA_VERT_S_32x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_S_32x4_AVX512 %1
    RET
%endmacro
%if ARCH_X86_64
    FILTER_VER_S_CHROMA_32xN_AVX512 ss, 8
    FILTER_VER_S_CHROMA_32xN_AVX512 ss, 16
    FILTER_VER_S_CHROMA_32xN_AVX512 ss, 24
    FILTER_VER_S_CHROMA_32xN_AVX512 ss, 32
    FILTER_VER_S_CHROMA_32xN_AVX512 ss, 48
    FILTER_VER_S_CHROMA_32xN_AVX512 ss, 64
    FILTER_VER_S_CHROMA_32xN_AVX512 sp, 8
    FILTER_VER_S_CHROMA_32xN_AVX512 sp, 16
    FILTER_VER_S_CHROMA_32xN_AVX512 sp, 24
    FILTER_VER_S_CHROMA_32xN_AVX512 sp, 32
    FILTER_VER_S_CHROMA_32xN_AVX512 sp, 48
    FILTER_VER_S_CHROMA_32xN_AVX512 sp, 64
%endif
%macro PROCESS_CHROMA_VERT_S_48x4_AVX512 1
    movu                  m1,                 [r0]
    lea                   r6,                 [r0 + 2 * r1]
    movu                  m10,                [r6]
    movu                  m3,                 [r0 + r1]
    movu                  m12,                [r6 + r1]
    punpcklwd             m0,                 m1,                  m3
    punpcklwd             m9,                 m10,                 m12
    pmaddwd               m0,                 m16
    pmaddwd               m9,                 m16
    punpckhwd             m1,                 m3
    punpckhwd             m10,                m12
    pmaddwd               m1,                 m16
    pmaddwd               m10,                m16

    movu                  m4,                 [r0 + 2 * r1]
    movu                  m13,                [r6 + 2 * r1]
    punpcklwd             m2,                 m3,                  m4
    punpcklwd             m11,                m12,                 m13
    pmaddwd               m2,                 m16
    pmaddwd               m11,                m16
    punpckhwd             m3,                 m4
    punpckhwd             m12,                m13
    pmaddwd               m3,                 m16
    pmaddwd               m12,                m16

    movu                  m5,                 [r0 + r7]
    movu                  m14,                [r6 + r7]
    punpcklwd             m6,                 m4,                  m5
    punpcklwd             m15,                m13,                 m14
    pmaddwd               m6,                 m17
    pmaddwd               m15,                m17
    paddd                 m0,                 m6
    paddd                 m9,                 m15
    punpckhwd             m4,                 m5
    punpckhwd             m13,                m14
    pmaddwd               m4,                 m17
    pmaddwd               m13,                m17
    paddd                 m1,                 m4
    paddd                 m10,                m13

    movu                  m4,                 [r0 + 4 * r1]
    movu                  m13,                [r6 + 4 * r1]
    punpcklwd             m6,                 m5,                  m4
    punpcklwd             m15,                m14,                 m13
    pmaddwd               m6,                 m17
    pmaddwd               m15,                m17
    paddd                 m2,                 m6
    paddd                 m11,                m15
    punpckhwd             m5,                 m4
    punpckhwd             m14,                m13
    pmaddwd               m5,                 m17
    pmaddwd               m14,                m17
    paddd                 m3,                 m5
    paddd                 m12,                m14

%ifidn %1,sp
    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7
    paddd                 m9,                 m7
    paddd                 m10,                m7
    paddd                 m11,                m7
    paddd                 m12,                m7

    psrad                 m0,                 INTERP_SHIFT_SP
    psrad                 m1,                 INTERP_SHIFT_SP
    psrad                 m2,                 INTERP_SHIFT_SP
    psrad                 m3,                 INTERP_SHIFT_SP
    psrad                 m9,                 INTERP_SHIFT_SP
    psrad                 m10,                INTERP_SHIFT_SP
    psrad                 m11,                INTERP_SHIFT_SP
    psrad                 m12,                INTERP_SHIFT_SP

    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packssdw              m9,                 m10
    packssdw              m11,                m12
    CLIPW2                m0,                 m2,               m18,                 m19
    CLIPW2                m9,                 m11,              m18,                 m19
%else
    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6
    psrad                 m9,                 6
    psrad                 m10,                6
    psrad                 m11,                6
    psrad                 m12,                6
    packssdw              m0,                 m1
    packssdw              m2,                 m3
    packssdw              m9,                 m10
    packssdw              m11,                m12
%endif

    movu                  [r2],               m0
    movu                  [r2 + r3],          m2
    movu                  [r2 + 2 * r3],      m9
    movu                  [r2 + r8],          m11

    movu                  ym1,                [r0 + mmsize]
    vinserti32x8          m1,                 [r6 + mmsize],       1
    movu                  ym3,                [r0 + r1 + mmsize]
    vinserti32x8          m3,                 [r6 + r1 + mmsize],  1
    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m16
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m16

    movu                  ym4,                [r0 + 2 * r1 + mmsize]
    vinserti32x8          m4,                 [r6 + 2 * r1 + mmsize],  1
    punpcklwd             m2,                 m3,                  m4
    pmaddwd               m2,                 m16
    punpckhwd             m3,                 m4
    pmaddwd               m3,                 m16

    movu                  ym5,                [r0 + r7 + mmsize]
    vinserti32x8          m5,                 [r6 + r7 + mmsize],  1
    punpcklwd             m6,                 m4,                  m5
    pmaddwd               m6,                 m17
    paddd                 m0,                 m6
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m17
    paddd                 m1,                 m4

    movu                  ym4,                [r0 + 4 * r1 + mmsize]
    vinserti32x8          m4,                 [r6 + 4 * r1 + mmsize],  1
    punpcklwd             m6,                 m5,                  m4
    pmaddwd               m6,                 m17
    paddd                 m2,                 m6
    punpckhwd             m5,                 m4
    pmaddwd               m5,                 m17
    paddd                 m3,                 m5

%ifidn %1,sp
    paddd                 m0,                 m7
    paddd                 m1,                 m7
    paddd                 m2,                 m7
    paddd                 m3,                 m7

    psrad                 m0,                 INTERP_SHIFT_SP
    psrad                 m1,                 INTERP_SHIFT_SP
    psrad                 m2,                 INTERP_SHIFT_SP
    psrad                 m3,                 INTERP_SHIFT_SP
    packssdw              m0,                 m1
    packssdw              m2,                 m3
    CLIPW2                m0,                 m2,                m18,                 m19
%else
    psrad                 m0,                 6
    psrad                 m1,                 6
    psrad                 m2,                 6
    psrad                 m3,                 6
    packssdw              m0,                 m1
    packssdw              m2,                 m3
%endif

    movu                  [r2 + mmsize],               ym0
    movu                  [r2 + r3 + mmsize],          ym2
    vextracti32x8         [r2 + 2 * r3 + mmsize],      m0,                  1
    vextracti32x8         [r2 + r8 + mmsize],          m2,                  1
%endmacro
%macro CHROMA_VERT_S_48x4_AVX512 1
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_48x64, 5, 9, 20
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7
%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    mova                  m16,                [r5 + r4]
    mova                  m17,                [r5 + r4 + mmsize]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
    mova                  m16,                [r5]
    mova                  m17,                [r5 + mmsize]
%endif
    lea                   r7,                 [3 * r1]
    lea                   r8,                 [3 * r3]
%ifidn %1, sp
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_SP]
    pxor                  m18,                m18
    vbroadcasti32x8       m19,                [pw_pixel_max]
%endif
%rep 15
    PROCESS_CHROMA_VERT_S_48x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_CHROMA_VERT_S_48x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    CHROMA_VERT_S_48x4_AVX512 sp
    CHROMA_VERT_S_48x4_AVX512 ss
%endif
%macro PROCESS_CHROMA_VERT_S_64x2_AVX512 1
    movu                 m1,                  [r0]
    movu                 m3,                  [r0 + r1]
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 m9,                  [r0 + mmsize]
    movu                 m11,                 [r0 + r1 + mmsize]
    punpcklwd            m8,                  m9,                     m11
    pmaddwd              m8,                  m15
    punpckhwd            m9,                  m11
    pmaddwd              m9,                  m15
    movu                 m4,                  [r0 + 2 * r1]
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15
    movu                 m12,                 [r0 + 2 * r1 + mmsize]
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m15
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m15

    lea                  r0,                  [r0 + 2 * r1]
    movu                 m5,                  [r0 + r1]
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    paddd                m0,                  m6
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16
    paddd                m1,                  m4

    movu                 m13,                 [r0 + r1 + mmsize]
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m16
    paddd                m8,                  m14
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m16
    paddd                m9,                  m12

    movu                 m4,                  [r0 + 2 * r1]
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    paddd                m2,                  m6
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16
    paddd                m3,                  m5

    movu                 m12,                 [r0 + 2 * r1 + mmsize]
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m16
    paddd                m10,                 m14
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m16
    paddd                m11,                 m13

%ifidn %1,sp
    paddd                m0,                  m7
    paddd                m1,                  m7
    paddd                m2,                  m7
    paddd                m3,                  m7
    paddd                m8,                  m7
    paddd                m9,                  m7
    paddd                m10,                 m7
    paddd                m11,                 m7

    psrad                m0,                  INTERP_SHIFT_SP
    psrad                m1,                  INTERP_SHIFT_SP
    psrad                m2,                  INTERP_SHIFT_SP
    psrad                m3,                  INTERP_SHIFT_SP
    psrad                m8,                  INTERP_SHIFT_SP
    psrad                m9,                  INTERP_SHIFT_SP
    psrad                m10,                 INTERP_SHIFT_SP
    psrad                m11,                 INTERP_SHIFT_SP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    packssdw             m8,                  m9
    packssdw             m10,                 m11
    CLIPW2               m0,                  m2,                   m17,              m18
    CLIPW2               m8,                  m10,                  m17,              m18
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6
    psrad                m8,                  6
    psrad                m9,                  6
    psrad                m10,                 6
    psrad                m11,                 6

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    packssdw             m8,                  m9
    packssdw             m10,                 m11
%endif

    movu                 [r2],                m0
    movu                 [r2 + r3],           m2
    movu                 [r2 + mmsize],       m8
    movu                 [r2 + r3 + mmsize],  m10
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_CHROMA_64xN_AVX512 2
INIT_ZMM avx512
cglobal interp_4tap_vert_%1_64x%2, 5, 7, 19
    add                   r1d,                r1d
    add                   r3d,                r3d
    sub                   r0,                 r1
    shl                   r4d,                7
%ifdef PIC
    lea                   r5,                 [tab_ChromaCoeffV_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + mmsize]
%else
    lea                   r5,                 [tab_ChromaCoeffV_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m7,                 [INTERP_OFFSET_SP]
    pxor                  m17,                m17
    vbroadcasti32x8       m18,                [pw_pixel_max]
%endif
%rep %2/2 - 1
    PROCESS_CHROMA_VERT_S_64x2_AVX512 %1
    lea                   r2,                 [r2 + 2 * r3]
%endrep
    PROCESS_CHROMA_VERT_S_64x2_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_CHROMA_64xN_AVX512 ss, 16
    FILTER_VER_S_CHROMA_64xN_AVX512 ss, 32
    FILTER_VER_S_CHROMA_64xN_AVX512 ss, 48
    FILTER_VER_S_CHROMA_64xN_AVX512 ss, 64
    FILTER_VER_S_CHROMA_64xN_AVX512 sp, 16
    FILTER_VER_S_CHROMA_64xN_AVX512 sp, 32
    FILTER_VER_S_CHROMA_64xN_AVX512 sp, 48
    FILTER_VER_S_CHROMA_64xN_AVX512 sp, 64
%endif
;-------------------------------------------------------------------------------------------------------------
; avx512 chroma_vsp and chroma_vss code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
;ipfilter_chroma_avx512 code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
;ipfilter_luma_avx512 code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_IPFILTER_LUMA_PP_8x4_AVX512 0
    ; register map
    ; m0 , m1, m2, m3 - interpolate coeff
    ; m4 , m5  load shuffle order table
    ; m6 - pd_32
    ; m7 - zero
    ; m8 - pw_pixel_max
    ; m9 - store shuffle order table

    movu            xm10,      [r0]
    movu            xm11,      [r0 + 8]
    movu            xm12,      [r0 + 16]

    vinserti32x4     m10,      [r0 + r1],      1
    vinserti32x4     m11,      [r0 + r1 + 8],  1
    vinserti32x4     m12,      [r0 + r1 + 16], 1

    vinserti32x4     m10,      [r0 + 2 * r1],           2
    vinserti32x4     m11,      [r0 + 2 * r1 + 8],       2
    vinserti32x4     m12,      [r0 + 2 * r1 + 16],      2

    vinserti32x4     m10,      [r0 + r6],      3
    vinserti32x4     m11,      [r0 + r6 + 8],  3
    vinserti32x4     m12,      [r0 + r6 + 16], 3

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m15,       m3
    pmaddwd         m12,       m2
    paddd           m12,       m15
    paddd           m11,       m12
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2],      xm10
    vextracti32x4   [r2 + r3],     m10,        1
    vextracti32x4   [r2 + 2 * r3], m10,        2
    vextracti32x4   [r2 + r7],     m10,        3
%endmacro

%macro PROCESS_IPFILTER_LUMA_PP_16x4_AVX512 0
    ; register map
    ; m0 , m1, m2, m3 - interpolate coeff
    ; m4 , m5  load shuffle order table
    ; m6 - pd_32
    ; m7 - zero
    ; m8 - pw_pixel_max
    ; m9 - store shuffle order table

    movu            ym10,      [r0]
    vinserti32x8     m10,      [r0 + r1],      1
    movu            ym11,      [r0 + 8]
    vinserti32x8     m11,      [r0 + r1 + 8],  1
    movu            ym12,      [r0 + 16]
    vinserti32x8     m12,      [r0 + r1 + 16], 1

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m15,       m3
    pmaddwd         m12,       m2
    paddd           m12,       m15
    paddd           m11,       m12
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2],      ym10
    vextracti32x8   [r2 + r3], m10,        1

    movu            ym10,      [r0 + 2 * r1]
    vinserti32x8     m10,      [r0 + r6],            1
    movu            ym11,      [r0 + 2 * r1 + 8]
    vinserti32x8     m11,      [r0 + r6 + 8],        1
    movu            ym12,      [r0 + 2 * r1 + 16]
    vinserti32x8     m12,      [r0 + r6 + 16],       1

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m14,       m15,       m3
    pmaddwd         m16,       m12,       m2
    paddd           m14,       m16
    paddd           m11,       m14
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2 + 2 * r3],        ym10
    vextracti32x8   [r2 + r7],      m10,     1
%endmacro

%macro PROCESS_IPFILTER_LUMA_PP_24x4_AVX512 0
    ; register map
    ; m0 , m1, m2, m3 - interpolate coeff
    ; m4 , m5  load shuffle order table
    ; m6 - pd_32
    ; m7 - zero
    ; m8 - pw_pixel_max
    ; m9 - store shuffle order table

    PROCESS_IPFILTER_LUMA_PP_16x4_AVX512

    movu            xm10,      [r0 + mmsize/2]
    movu            xm11,      [r0 + mmsize/2 + 8]
    movu            xm12,      [r0 + mmsize/2 + 16]

    vinserti32x4     m10,      [r0 + r1 + mmsize/2],      1
    vinserti32x4     m11,      [r0 + r1 + mmsize/2 + 8],  1
    vinserti32x4     m12,      [r0 + r1 + mmsize/2 + 16], 1

    vinserti32x4     m10,      [r0 + 2 * r1 + mmsize/2],           2
    vinserti32x4     m11,      [r0 + 2 * r1 + mmsize/2 + 8],       2
    vinserti32x4     m12,      [r0 + 2 * r1 + mmsize/2 + 16],      2

    vinserti32x4     m10,      [r0 + r6 + mmsize/2],      3
    vinserti32x4     m11,      [r0 + r6 + mmsize/2 + 8],  3
    vinserti32x4     m12,      [r0 + r6 + mmsize/2 + 16], 3

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m15,       m3
    pmaddwd         m12,       m2
    paddd           m12,       m15
    paddd           m11,       m12
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2 + mmsize/2],      xm10
    vextracti32x4   [r2 + r3 + mmsize/2],     m10,        1
    vextracti32x4   [r2 + 2 * r3 + mmsize/2], m10,        2
    vextracti32x4   [r2 + r7 + mmsize/2],     m10,        3
%endmacro

%macro PROCESS_IPFILTER_LUMA_PP_32x2_AVX512 0
    ; register map
    ; m0 , m1, m2, m3 - interpolate coeff
    ; m4 , m5  load shuffle order table
    ; m6 - pd_32
    ; m7 - zero
    ; m8 - pw_pixel_max
    ; m9 - store shuffle order table

    movu            m10,       [r0]
    movu            m11,       [r0 + 8]
    movu            m12,       [r0 + 16]

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m15,       m3
    pmaddwd         m12,       m2
    paddd           m12,       m15
    paddd           m11,       m12
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2],      m10

    movu            m10,       [r0 + r1]
    movu            m11,       [r0 + r1 + 8]
    movu            m12,       [r0 + r1 + 16]

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m14,       m15,       m3
    pmaddwd         m16,       m12,       m2
    paddd           m14,       m16
    paddd           m11,       m14
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2 + r3], m10
%endmacro

%macro PROCESS_IPFILTER_LUMA_PP_48x4_AVX512 0
    ; register map
    ; m0 , m1, m2, m3 - interpolate coeff
    ; m4 , m5  load shuffle order table
    ; m6 - pd_32
    ; m7 - zero
    ; m8 - pw_pixel_max
    ; m9 - store shuffle order table

    movu            m10,       [r0]
    movu            m11,       [r0 + 8]
    movu            m12,       [r0 + 16]

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m15,       m3
    pmaddwd         m12,       m2
    paddd           m12,       m15
    paddd           m11,       m12
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2],      m10

    movu            m10,       [r0 + r1]
    movu            m11,       [r0 + r1 + 8]
    movu            m12,       [r0 + r1 + 16]

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m14,       m15,       m3
    pmaddwd         m16,       m12,       m2
    paddd           m14,       m16
    paddd           m11,       m14
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2 + r3], m10

    movu            m10,       [r0 + 2 * r1]
    movu            m11,       [r0 + 2 * r1 + 8]
    movu            m12,       [r0 + 2 * r1 + 16]

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m15,       m3
    pmaddwd         m12,       m2
    paddd           m12,       m15
    paddd           m11,       m12
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2 + 2 * r3],         m10

    movu            m10,       [r0 + r6]
    movu            m11,       [r0 + r6 + 8]
    movu            m12,       [r0 + r6 + 16]

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m14,       m15,       m3
    pmaddwd         m16,       m12,       m2
    paddd           m14,       m16
    paddd           m11,       m14
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2 + r7], m10

    movu            ym10,      [r0 + mmsize]
    vinserti32x8     m10,      [r0 + r1 + mmsize],      1
    movu            ym11,      [r0 + mmsize + 8]
    vinserti32x8     m11,      [r0 + r1 + mmsize + 8],  1
    movu            ym12,      [r0 + mmsize + 16]
    vinserti32x8     m12,      [r0 + r1 + mmsize + 16], 1

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m15,       m3
    pmaddwd         m12,       m2
    paddd           m12,       m15
    paddd           m11,       m12
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2 + mmsize],      ym10
    vextracti32x8   [r2 + r3 + mmsize], m10,        1

    movu            ym10,      [r0 + 2 * r1 + mmsize]
    vinserti32x8     m10,      [r0 + r6 + mmsize],            1
    movu            ym11,      [r0 + 2 * r1 + mmsize + 8]
    vinserti32x8     m11,      [r0 + r6 + mmsize + 8],        1
    movu            ym12,      [r0 + 2 * r1 + mmsize + 16]
    vinserti32x8     m12,      [r0 + r6 + mmsize + 16],       1

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m14,       m15,       m3
    pmaddwd         m16,       m12,       m2
    paddd           m14,       m16
    paddd           m11,       m14
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2 + 2 * r3 + mmsize],        ym10
    vextracti32x8   [r2 + r7 + mmsize],      m10,     1
%endmacro

%macro PROCESS_IPFILTER_LUMA_PP_64x2_AVX512 0
    ; register map
    ; m0 , m1, m2, m3 - interpolate coeff
    ; m4 , m5  load shuffle order table
    ; m6 - pd_32
    ; m7 - zero
    ; m8 - pw_pixel_max
    ; m9 - store shuffle order table

    movu            m10,       [r0]
    movu            m11,       [r0 + 8]
    movu            m12,       [r0 + 16]

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m15,       m3
    pmaddwd         m12,       m2
    paddd           m12,       m15
    paddd           m11,       m12
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2],      m10

    movu            m10,       [r0 + mmsize]
    movu            m11,       [r0 + mmsize + 8]
    movu            m12,       [r0 + mmsize + 16]

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m15,       m3
    pmaddwd         m12,       m2
    paddd           m12,       m15
    paddd           m11,       m12
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2 + mmsize],         m10

    movu            m10,       [r0 + r1]
    movu            m11,       [r0 + r1 + 8]
    movu            m12,       [r0 + r1 + 16]

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m14,       m15,       m3
    pmaddwd         m16,       m12,       m2
    paddd           m14,       m16
    paddd           m11,       m14
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2 + r3], m10

    movu            m10,       [r0 + r1 + mmsize]
    movu            m11,       [r0 + r1 + mmsize + 8]
    movu            m12,       [r0 + r1 + mmsize + 16]

    pshufb          m13,       m10,        m5
    pshufb          m10,       m4
    pshufb          m14,       m11,        m5
    pshufb          m11,       m4
    pshufb          m15,       m12,        m5
    pshufb          m12,       m4

    pmaddwd         m10,       m0
    pmaddwd         m13,       m1
    paddd           m10,       m13
    pmaddwd         m13,       m14,       m3
    pmaddwd         m16,       m11,       m2
    paddd           m13,       m16
    paddd           m10,       m13
    paddd           m10,       m6
    psrad           m10,       INTERP_SHIFT_PP

    pmaddwd         m11,       m0
    pmaddwd         m14,       m1
    paddd           m11,       m14
    pmaddwd         m14,       m15,       m3
    pmaddwd         m16,       m12,       m2
    paddd           m14,       m16
    paddd           m11,       m14
    paddd           m11,       m6
    psrad           m11,       INTERP_SHIFT_PP

    packusdw        m10,       m11
    CLIPW           m10,       m7,         m8
    pshufb          m10,       m9
    movu            [r2 + r3 + mmsize],    m10
%endmacro

%macro IPFILTER_LUMA_AVX512_8xN 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_pp_8x%1, 5, 8, 17
    add              r1d,        r1d
    add              r3d,        r3d
    sub              r0,         6
    mov              r4d,        r4m
    shl              r4d,        4

%ifdef PIC
    lea              r5,         [tab_LumaCoeff]
    vpbroadcastd     m0,         [r5 + r4]
    vpbroadcastd     m1,         [r5 + r4 + 4]
    vpbroadcastd     m2,         [r5 + r4 + 8]
    vpbroadcastd     m3,         [r5 + r4 + 12]
%else
    vpbroadcastd     m0,         [tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [tab_LumaCoeff + r4 + 12]
%endif
    vbroadcasti32x8  m4,         [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8  m5,         [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8  m6,         [pd_32]
    pxor             m7,         m7
    vbroadcasti32x8  m8,         [pw_pixel_max]
    vbroadcasti32x8  m9,         [interp8_hpp_shuf1_store_avx512]
    lea              r6,         [3 * r1]
    lea              r7,         [3 * r3]

%rep %1/4 - 1
    PROCESS_IPFILTER_LUMA_PP_8x4_AVX512
    lea              r0,         [r0 + 4 * r1]
    lea              r2,         [r2 + 4 * r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_8x4_AVX512
    RET
%endmacro

%if ARCH_X86_64
    IPFILTER_LUMA_AVX512_8xN 4
    IPFILTER_LUMA_AVX512_8xN 8
    IPFILTER_LUMA_AVX512_8xN 16
    IPFILTER_LUMA_AVX512_8xN 32
%endif

%macro IPFILTER_LUMA_AVX512_16xN 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_pp_16x%1, 5,8,17
    add              r1d,        r1d
    add              r3d,        r3d
    sub              r0,         6
    mov              r4d,        r4m
    shl              r4d,        4

%ifdef PIC
    lea              r5,         [tab_LumaCoeff]
    vpbroadcastd     m0,         [r5 + r4]
    vpbroadcastd     m1,         [r5 + r4 + 4]
    vpbroadcastd     m2,         [r5 + r4 + 8]
    vpbroadcastd     m3,         [r5 + r4 + 12]
%else
    vpbroadcastd     m0,         [tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [tab_LumaCoeff + r4 + 12]
%endif
    vbroadcasti32x8  m4,         [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8  m5,         [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8  m6,         [pd_32]
    pxor             m7,         m7
    vbroadcasti32x8  m8,         [pw_pixel_max]
    vbroadcasti32x8  m9,         [interp8_hpp_shuf1_store_avx512]
    lea              r6,         [3 * r1]
    lea              r7,         [3 * r3]

%rep %1/4 - 1
    PROCESS_IPFILTER_LUMA_PP_16x4_AVX512
    lea              r0,         [r0 + 4 * r1]
    lea              r2,         [r2 + 4 * r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_16x4_AVX512
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_LUMA_AVX512_16xN 4
IPFILTER_LUMA_AVX512_16xN 8
IPFILTER_LUMA_AVX512_16xN 12
IPFILTER_LUMA_AVX512_16xN 16
IPFILTER_LUMA_AVX512_16xN 32
IPFILTER_LUMA_AVX512_16xN 64
%endif

%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_8tap_horiz_pp_24x32, 5, 8, 17
    add              r1d,        r1d
    add              r3d,        r3d
    sub              r0,         6
    mov              r4d,        r4m
    shl              r4d,        4

%ifdef PIC
    lea              r5,         [tab_LumaCoeff]
    vpbroadcastd     m0,         [r5 + r4]
    vpbroadcastd     m1,         [r5 + r4 + 4]
    vpbroadcastd     m2,         [r5 + r4 + 8]
    vpbroadcastd     m3,         [r5 + r4 + 12]
%else
    vpbroadcastd     m0,         [tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [tab_LumaCoeff + r4 + 12]
%endif
    vbroadcasti32x8  m4,         [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8  m5,         [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8  m6,         [pd_32]
    pxor             m7,         m7
    vbroadcasti32x8  m8,         [pw_pixel_max]
    vbroadcasti32x8  m9,         [interp8_hpp_shuf1_store_avx512]
    lea              r6,         [3 * r1]
    lea              r7,         [3 * r3]

%rep 7
    PROCESS_IPFILTER_LUMA_PP_24x4_AVX512
    lea              r0,         [r0 + 4 * r1]
    lea              r2,         [r2 + 4 * r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_24x4_AVX512
    RET
%endif

%macro IPFILTER_LUMA_AVX512_32xN 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_pp_32x%1, 5,6,17
    add              r1d,        r1d
    add              r3d,        r3d
    sub              r0,         6
    mov              r4d,        r4m
    shl              r4d,        4

%ifdef PIC
    lea              r5,         [tab_LumaCoeff]
    vpbroadcastd     m0,         [r5 + r4]
    vpbroadcastd     m1,         [r5 + r4 + 4]
    vpbroadcastd     m2,         [r5 + r4 + 8]
    vpbroadcastd     m3,         [r5 + r4 + 12]
%else
    vpbroadcastd     m0,         [tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [tab_LumaCoeff + r4 + 12]
%endif
    vbroadcasti32x8  m4,         [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8  m5,         [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8  m6,         [pd_32]
    pxor             m7,         m7
    vbroadcasti32x8  m8,         [pw_pixel_max]
    vbroadcasti32x8  m9,         [interp8_hpp_shuf1_store_avx512]

%rep %1/2 - 1
    PROCESS_IPFILTER_LUMA_PP_32x2_AVX512
    lea              r0,         [r0 + 2 * r1]
    lea              r2,         [r2 + 2 * r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_32x2_AVX512
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_LUMA_AVX512_32xN 8
IPFILTER_LUMA_AVX512_32xN 16
IPFILTER_LUMA_AVX512_32xN 24
IPFILTER_LUMA_AVX512_32xN 32
IPFILTER_LUMA_AVX512_32xN 64
%endif

%macro IPFILTER_LUMA_AVX512_64xN 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_pp_64x%1, 5,6,17
    add              r1d,        r1d
    add              r3d,        r3d
    sub              r0,         6
    mov              r4d,        r4m
    shl              r4d,        4

%ifdef PIC
    lea              r5,         [tab_LumaCoeff]
    vpbroadcastd     m0,         [r5 + r4]
    vpbroadcastd     m1,         [r5 + r4 + 4]
    vpbroadcastd     m2,         [r5 + r4 + 8]
    vpbroadcastd     m3,         [r5 + r4 + 12]
%else
    vpbroadcastd     m0,         [tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [tab_LumaCoeff + r4 + 12]
%endif
    vbroadcasti32x8  m4,         [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8  m5,         [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8  m6,         [pd_32]
    pxor             m7,         m7
    vbroadcasti32x8  m8,         [pw_pixel_max]
    vbroadcasti32x8  m9,         [interp8_hpp_shuf1_store_avx512]

%rep %1/2 - 1
    PROCESS_IPFILTER_LUMA_PP_64x2_AVX512
    lea              r0,         [r0 + 2 * r1]
    lea              r2,         [r2 + 2 * r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_64x2_AVX512
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_LUMA_AVX512_64xN 16
IPFILTER_LUMA_AVX512_64xN 32
IPFILTER_LUMA_AVX512_64xN 48
IPFILTER_LUMA_AVX512_64xN 64
%endif

%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_8tap_horiz_pp_48x64, 5,8,17
    add              r1d,        r1d
    add              r3d,        r3d
    sub              r0,         6
    mov              r4d,        r4m
    shl              r4d,        4

%ifdef PIC
    lea              r5,         [tab_LumaCoeff]
    vpbroadcastd     m0,         [r5 + r4]
    vpbroadcastd     m1,         [r5 + r4 + 4]
    vpbroadcastd     m2,         [r5 + r4 + 8]
    vpbroadcastd     m3,         [r5 + r4 + 12]
%else
    vpbroadcastd     m0,         [tab_LumaCoeff + r4]
    vpbroadcastd     m1,         [tab_LumaCoeff + r4 + 4]
    vpbroadcastd     m2,         [tab_LumaCoeff + r4 + 8]
    vpbroadcastd     m3,         [tab_LumaCoeff + r4 + 12]
%endif
    vbroadcasti32x8  m4,         [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8  m5,         [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x8  m6,         [pd_32]
    pxor             m7,         m7
    vbroadcasti32x8  m8,         [pw_pixel_max]
    vbroadcasti32x8  m9,         [interp8_hpp_shuf1_store_avx512]
    lea              r6,         [3 * r1]
    lea              r7,         [3 * r3]

%rep 15
    PROCESS_IPFILTER_LUMA_PP_48x4_AVX512
    lea              r0,         [r0 + 4 * r1]
    lea              r2,         [r2 + 4 * r3]
%endrep
    PROCESS_IPFILTER_LUMA_PP_48x4_AVX512
    RET
%endif
;-------------------------------------------------------------------------------------------------------------
;avx512 luma_hps code start
;-------------------------------------------------------------------------------------------------------------

%macro PROCESS_IPFILTER_LUMA_PS_32x2_AVX512 0
    ; register map
    ; m0, m1, m2, m3 - interpolate coeff
    ; m4, m5         - shuffle load order table
    ; m6             - INTERP_OFFSET_PS
    ; m7             - shuffle store order table

    movu            m8,       [r0]
    movu            m9,       [r0 + 8]
    movu            m10,      [r0 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14

    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13

    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2],     m8

    movu            m8,       [r0 + r1]
    movu            m9,       [r0 + r1 + 8]
    movu            m10,      [r0 + r1 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14

    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m12,      m13,       m3
    pmaddwd         m14,      m10,       m2
    paddd           m12,      m14

    paddd           m9,       m12
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + r3],m8
%endmacro

%macro PROCESS_IPFILTER_LUMA_PS_32x1_AVX512 0
    movu            m8,       [r0]
    movu            m9,       [r0 + 8]
    movu            m10,      [r0 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14

    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13

    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2],     m8
%endmacro

%macro IPFILTER_LUMA_PS_AVX512_32xN 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_ps_32x%1, 4,7,15
    shl              r1d,        1
    shl              r3d,        1
    mov              r4d,        r4m
    mov              r5d,        r5m
    shl              r4d,        6

%ifdef PIC
    lea              r6,         [tab_LumaCoeffH_avx512]
    vpbroadcastd     m0,         [r6 + r4]
    vpbroadcastd     m1,         [r6 + r4 + 4]
    vpbroadcastd     m2,         [r6 + r4 + 8]
    vpbroadcastd     m3,         [r6 + r4 + 12]
%else
    vpbroadcastd     m0,         [tab_LumaCoeffH_avx512 + r4]
    vpbroadcastd     m1,         [tab_LumaCoeffH_avx512 + r4 + 4]
    vpbroadcastd     m2,         [tab_LumaCoeffH_avx512 + r4 + 8]
    vpbroadcastd     m3,         [tab_LumaCoeffH_avx512 + r4 + 12]
%endif
    vbroadcasti32x8  m4,         [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8  m5,         [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x4  m6,         [INTERP_OFFSET_PS]
    vbroadcasti32x8  m7,         [interp8_hpp_shuf1_store_avx512]

    sub              r0,  6
    mov              r4d, %1
    test             r5d, r5d
    jz               .loop
    lea              r6,  [r1 * 3]
    sub              r0,  r6
    add              r4d, 7
    PROCESS_IPFILTER_LUMA_PS_32x1_AVX512
    lea              r0,  [r0 + r1]
    lea              r2,  [r2 + r3]
    dec              r4d

.loop:
    PROCESS_IPFILTER_LUMA_PS_32x2_AVX512
    lea              r0,  [r0 + 2 * r1]
    lea              r2,  [r2 + 2 * r3]
    sub              r4d, 2
    jnz              .loop
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_LUMA_PS_AVX512_32xN 8
IPFILTER_LUMA_PS_AVX512_32xN 16
IPFILTER_LUMA_PS_AVX512_32xN 24
IPFILTER_LUMA_PS_AVX512_32xN 32
IPFILTER_LUMA_PS_AVX512_32xN 64
%endif

%macro PROCESS_IPFILTER_LUMA_PS_64x2_AVX512 0
    ; register map
    ; m0, m1, m2, m3 - interpolate coeff
    ; m4, m5         - shuffle load order table
    ; m6             - INTERP_OFFSET_PS
    ; m7             - shuffle store order table

    movu            m8,       [r0]
    movu            m9,       [r0 + 8]
    movu            m10,      [r0 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14

    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13

    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2],     m8

    movu            m8,       [r0 + mmsize]
    movu            m9,       [r0 + mmsize + 8]
    movu            m10,      [r0 + mmsize + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13
    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + mmsize],       m8

    movu            m8,       [r0 + r1]
    movu            m9,       [r0 + r1 + 8]
    movu            m10,      [r0 + r1 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,       m1
    paddd           m9,       m12
    pmaddwd         m12,       m13,      m3
    pmaddwd         m14,       m10,      m2
    paddd           m12,       m14
    paddd           m9,       m12
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + r3],m8

    movu            m8,       [r0 + r1 + mmsize]
    movu            m9,       [r0 + r1 + mmsize + 8]
    movu            m10,      [r0 + r1 + mmsize + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m12,      m13,       m3
    pmaddwd         m14,      m10,       m2
    paddd           m12,      m14
    paddd           m9,       m12
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + r3 + mmsize],  m8
%endmacro

%macro PROCESS_IPFILTER_LUMA_PS_64x1_AVX512 0

    movu            m8,       [r0]
    movu            m9,       [r0 + 8]
    movu            m10,      [r0 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13
    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2],     m8

    movu            m8,       [r0 + mmsize]
    movu            m9,       [r0 + mmsize + 8]
    movu            m10,      [r0 + mmsize + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13
    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + mmsize],       m8
%endmacro

%macro IPFILTER_LUMA_PS_AVX512_64xN 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_ps_64x%1, 4,7,15
    shl              r1d,        1
    shl              r3d,        1
    mov              r4d,        r4m
    mov              r5d,        r5m
    shl              r4d,        6

%ifdef PIC
    lea              r6,         [tab_LumaCoeffH_avx512]
    vpbroadcastd     m0,         [r6 + r4]
    vpbroadcastd     m1,         [r6 + r4 + 4]
    vpbroadcastd     m2,         [r6 + r4 + 8]
    vpbroadcastd     m3,         [r6 + r4 + 12]
%else
    vpbroadcastd     m0,         [tab_LumaCoeffH_avx512 + r4]
    vpbroadcastd     m1,         [tab_LumaCoeffH_avx512 + r4 + 4]
    vpbroadcastd     m2,         [tab_LumaCoeffH_avx512 + r4 + 8]
    vpbroadcastd     m3,         [tab_LumaCoeffH_avx512 + r4 + 12]
%endif
    vbroadcasti32x8  m4,         [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8  m5,         [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x4  m6,         [INTERP_OFFSET_PS]
    vbroadcasti32x8  m7,         [interp8_hpp_shuf1_store_avx512]

    sub              r0,  6
    mov              r4d, %1
    test             r5d, r5d
    jz               .loop
    lea              r6,  [r1 * 3]
    sub              r0,  r6
    add              r4d, 7
    PROCESS_IPFILTER_LUMA_PS_64x1_AVX512
    lea              r0,  [r0 + r1]
    lea              r2,  [r2 + r3]
    dec              r4d

.loop:
    PROCESS_IPFILTER_LUMA_PS_64x2_AVX512
    lea              r0,  [r0 + 2 * r1]
    lea              r2,  [r2 + 2 * r3]
    sub              r4d, 2
    jnz              .loop
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_LUMA_PS_AVX512_64xN 16
IPFILTER_LUMA_PS_AVX512_64xN 32
IPFILTER_LUMA_PS_AVX512_64xN 48
IPFILTER_LUMA_PS_AVX512_64xN 64
%endif

%macro PROCESS_IPFILTER_LUMA_PS_16x4_AVX512 0
    ; register map
    ; m0, m1, m2, m3 - interpolate coeff
    ; m4, m5         - shuffle load order table
    ; m6             - INTERP_OFFSET_PS
    ; m7             - shuffle store order table

    movu            ym8,      [r0]
    vinserti32x8     m8,      [r0 + r1],      1
    movu            ym9,      [r0 + 8]
    vinserti32x8     m9,      [r0 + r1 + 8],  1
    movu            ym10,     [r0 + 16]
    vinserti32x8     m10,     [r0 + r1 + 16], 1

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13
    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2],     ym8
    vextracti32x8   [r2 + r3],m8,        1

    movu            ym8,      [r0 + 2 * r1]
    vinserti32x8     m8,      [r0 + r6],            1
    movu            ym9,      [r0 + 2 * r1 + 8]
    vinserti32x8     m9,      [r0 + r6 + 8],        1
    movu            ym10,     [r0 + 2 * r1 + 16]
    vinserti32x8     m10,     [r0 + r6 + 16],       1

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m12,      m13,       m3
    pmaddwd         m14,      m10,       m2
    paddd           m12,      m14
    paddd           m9,       m12
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + 2 * r3],       ym8
    vextracti32x8   [r2 + r7],      m8,  1
%endmacro

%macro PROCESS_IPFILTER_LUMA_PS_16x3_AVX512 0
    movu            ym8,      [r0]
    vinserti32x8     m8,      [r0 + r1],      1
    movu            ym9,      [r0 + 8]
    vinserti32x8     m9,      [r0 + r1 + 8],  1
    movu            ym10,     [r0 + 16]
    vinserti32x8     m10,     [r0 + r1 + 16], 1

    pshufb          m11,      m8,             m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,             m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,            m3
    pmaddwd         m14,      m9,             m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,            m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13
    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2],     ym8
    vextracti32x8   [r2 + r3],m8,             1

    movu            ym8,      [r0 + 2 * r1]
    movu            ym9,      [r0 + 2 * r1 + 8]
    movu            ym10,     [r0 + 2 * r1 + 16]

    pshufb          ym11,     ym8,            ym5
    pshufb          ym8,      ym4
    pmaddwd         ym8,      ym0
    pmaddwd         ym11,     ym1
    paddd           ym8,      ym11
    pshufb          ym12,     ym9,            ym5
    pshufb          ym9,      ym4
    pmaddwd         ym11,     ym12,           ym3
    pmaddwd         ym14,     ym9,            ym2
    paddd           ym11,     ym14
    paddd           ym8,      ym11
    paddd           ym8,      ym6
    psrad           ym8,      INTERP_SHIFT_PS

    pshufb          ym13,     ym10,           ym5
    pshufb          ym10,     ym4
    pmaddwd         ym9,      ym0
    pmaddwd         ym12,     ym1
    paddd           ym9,      ym12
    pmaddwd         ym12,     ym13,           ym3
    pmaddwd         ym14,     ym10,           ym2
    paddd           ym12,     ym14
    paddd           ym9,      ym12
    paddd           ym9,      ym6
    psrad           ym9,      INTERP_SHIFT_PS

    packssdw        ym8,      ym9
    pshufb          ym8,      ym7
    movu            [r2 + 2 * r3],            ym8
%endmacro


%macro IPFILTER_LUMA_PS_AVX512_16xN 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_ps_16x%1, 4,9,15
    shl              r1d,        1
    shl              r3d,        1
    mov              r4d,        r4m
    mov              r5d,        r5m
    shl              r4d,        6

    lea              r6,         [3 * r1]
    lea              r7,         [3 * r3]	
%ifdef PIC
    lea              r8,         [tab_LumaCoeffH_avx512]
    vpbroadcastd     m0,         [r8 + r4]
    vpbroadcastd     m1,         [r8 + r4 + 4]
    vpbroadcastd     m2,         [r8 + r4 + 8]
    vpbroadcastd     m3,         [r8 + r4 + 12]
%else
    vpbroadcastd     m0,         [tab_LumaCoeffH_avx512 + r4]
    vpbroadcastd     m1,         [tab_LumaCoeffH_avx512 + r4 + 4]
    vpbroadcastd     m2,         [tab_LumaCoeffH_avx512 + r4 + 8]
    vpbroadcastd     m3,         [tab_LumaCoeffH_avx512 + r4 + 12]
%endif
    vbroadcasti32x8  m4,         [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8  m5,         [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x4  m6,         [INTERP_OFFSET_PS]
    vbroadcasti32x8  m7,         [interp8_hpp_shuf1_store_avx512]

    sub              r0,  6
    mov              r4d, %1
    test             r5d, r5d
    jz               .loop
    lea              r6,  [r1 * 3]
    sub              r0,  r6
    add              r4d, 7
    PROCESS_IPFILTER_LUMA_PS_16x3_AVX512
    lea              r0,  [r0 + r6]
    lea              r2,  [r2 + r7]
    sub              r4d, 3

.loop:
    PROCESS_IPFILTER_LUMA_PS_16x4_AVX512
    lea              r0,  [r0 + 4 * r1]
    lea              r2,  [r2 + 4 * r3]
    sub              r4d, 4
    jnz              .loop
    RET
%endmacro

%if ARCH_X86_64
IPFILTER_LUMA_PS_AVX512_16xN 4
IPFILTER_LUMA_PS_AVX512_16xN 8
IPFILTER_LUMA_PS_AVX512_16xN 12
IPFILTER_LUMA_PS_AVX512_16xN 16
IPFILTER_LUMA_PS_AVX512_16xN 32
IPFILTER_LUMA_PS_AVX512_16xN 64
%endif

%macro PROCESS_IPFILTER_LUMA_PS_48x4_AVX512 0
    ; register map
    ; m0, m1, m2, m3 - interpolate coeff
    ; m4, m5         - shuffle load order table
    ; m6             - INTERP_OFFSET_PS
    ; m7             - shuffle store order table

    movu            m8,       [r0]
    movu            m9,       [r0 + 8]
    movu            m10,      [r0 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13
    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2],     m8

    movu            m8,       [r0 + r1]
    movu            m9,       [r0 + r1 + 8]
    movu            m10,      [r0 + r1 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m12,      m13,       m3
    pmaddwd         m14,      m10,       m2
    paddd           m12,      m14
    paddd           m9,       m12
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + r3],m8

    movu            m8,       [r0 + 2 * r1]
    movu            m9,       [r0 + 2 * r1 + 8]
    movu            m10,      [r0 + 2 * r1 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13
    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + 2 * r3],       m8

    movu            m8,       [r0 + r6]
    movu            m9,       [r0 + r6 + 8]
    movu            m10,      [r0 + r6 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m12,      m13,       m3
    pmaddwd         m14,      m10,       m2
    paddd           m12,      m14
    paddd           m9,       m12
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + r7],m8

    movu            ym8,      [r0 + mmsize]
    vinserti32x8     m8,      [r0 + r1 + mmsize],      1
    movu            ym9,      [r0 + mmsize + 8]
    vinserti32x8     m9,      [r0 + r1 + mmsize + 8],  1
    movu            ym10,     [r0 + mmsize + 16]
    vinserti32x8     m10,     [r0 + r1 + mmsize + 16], 1

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13
    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + mmsize],      ym8
    vextracti32x8   [r2 + r3 + mmsize], m8,        1

    movu            ym8,      [r0 + 2 * r1 + mmsize]
    vinserti32x8     m8,      [r0 + r6 + mmsize],            1
    movu            ym9,      [r0 + 2 * r1 + mmsize + 8]
    vinserti32x8     m9,      [r0 + r6 + mmsize + 8],        1
    movu            ym10,     [r0 + 2 * r1 + mmsize + 16]
    vinserti32x8     m10,     [r0 + r6 + mmsize + 16],       1

    pshufb          m11,      m8,       m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,       m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,      m3
    pmaddwd         m14,      m9,       m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,      m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m12,      m13,      m3
    pmaddwd         m14,      m10,      m2
    paddd           m12,      m14
    paddd           m9,       m12
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + 2 * r3 + mmsize],        ym8
    vextracti32x8   [r2 + r7 + mmsize],     m8,      1
%endmacro

%macro PROCESS_IPFILTER_LUMA_PS_48x3_AVX512 0
    movu            m8,       [r0]
    movu            m9,       [r0 + 8]
    movu            m10,      [r0 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13
    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2],     m8

    movu            m8,       [r0 + r1]
    movu            m9,       [r0 + r1 + 8]
    movu            m10,      [r0 + r1 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m12,      m13,       m3
    pmaddwd         m14,      m10,       m2
    paddd           m12,      m14
    paddd           m9,       m12
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + r3],m8

    movu            m8,       [r0 + 2 * r1]
    movu            m9,       [r0 + 2 * r1 + 8]
    movu            m10,      [r0 + 2 * r1 + 16]

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13
    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + 2 * r3],       m8

    movu            ym8,      [r0 + mmsize]
    vinserti32x8     m8,      [r0 + r1 + mmsize],      1
    movu            ym9,      [r0 + mmsize + 8]
    vinserti32x8     m9,      [r0 + r1 + mmsize + 8],  1
    movu            ym10,     [r0 + mmsize + 16]
    vinserti32x8     m10,     [r0 + r1 + mmsize + 16], 1

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14
    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13
    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + mmsize],      ym8
    vextracti32x8   [r2 + r3 + mmsize], m8,        1

    movu            ym8,      [r0 + 2 * r1 + mmsize]
    movu            ym9,      [r0 + 2 * r1 + mmsize + 8]
    movu            ym10,     [r0 + 2 * r1 + mmsize + 16]

    pshufb          ym11,      ym8,       ym5
    pshufb          ym8,       ym4
    pmaddwd         ym8,       ym0
    pmaddwd         ym11,      ym1
    paddd           ym8,       ym11
    pshufb          ym12,      ym9,       ym5
    pshufb          ym9,       ym4
    pmaddwd         ym11,      ym12,      ym3
    pmaddwd         ym14,      ym9,       ym2
    paddd           ym11,      ym14
    paddd           ym8,       ym11
    paddd           ym8,       ym6
    psrad           ym8,       INTERP_SHIFT_PS

    pshufb          ym13,      ym10,      ym5
    pshufb          ym10,      ym4
    pmaddwd         ym9,       ym0
    pmaddwd         ym12,      ym1
    paddd           ym9,       ym12
    pmaddwd         ym12,      ym13,      ym3
    pmaddwd         ym14,      ym10,      ym2
    paddd           ym12,      ym14
    paddd           ym9,       ym12
    paddd           ym9,       ym6
    psrad           ym9,       INTERP_SHIFT_PS

    packssdw        ym8,       ym9
    pshufb          ym8,       ym7
    movu            [r2 + 2 * r3 + mmsize],        ym8
%endmacro

%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_8tap_horiz_ps_48x64, 4,9,15
    shl              r1d,        1
    shl              r3d,        1
    mov              r4d,        r4m
    mov              r5d,        r5m
    shl              r4d,        6
    lea              r6,         [3 * r1]
    lea              r7,         [3 * r3]
%ifdef PIC
    lea              r8,         [tab_LumaCoeffH_avx512]
    vpbroadcastd     m0,         [r8 + r4]
    vpbroadcastd     m1,         [r8 + r4 + 4]
    vpbroadcastd     m2,         [r8 + r4 + 8]
    vpbroadcastd     m3,         [r8 + r4 + 12]
%else
    vpbroadcastd     m0,         [tab_LumaCoeffH_avx512 + r4]
    vpbroadcastd     m1,         [tab_LumaCoeffH_avx512 + r4 + 4]
    vpbroadcastd     m2,         [tab_LumaCoeffH_avx512 + r4 + 8]
    vpbroadcastd     m3,         [tab_LumaCoeffH_avx512 + r4 + 12]
%endif
    vbroadcasti32x8  m4,         [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8  m5,         [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x4  m6,         [INTERP_OFFSET_PS]
    vbroadcasti32x8  m7,         [interp8_hpp_shuf1_store_avx512]

    sub              r0,  6
    mov              r4d, 64
    test             r5d, r5d
    jz               .loop
    lea              r6,  [r1 * 3]
    sub              r0,  r6
    add              r4d, 7
    PROCESS_IPFILTER_LUMA_PS_48x4_AVX512
    lea              r0,  [r0 + r6]
    lea              r2,  [r2 + r7]
    sub              r4d, 3

.loop:
    PROCESS_IPFILTER_LUMA_PS_48x4_AVX512
    lea              r0,  [r0 + 4 * r1]
    lea              r2,  [r2 + 4 * r3]
    sub              r4d, 4
    jnz              .loop
    RET
%endif

%macro PROCESS_IPFILTER_LUMA_PS_24x4_AVX512 0
    ; register map
    ; m0 , m1, m2, m3 - interpolate coeff table
    ; m4 , m5         - load shuffle order table
    ; m6              - INTERP_OFFSET_PS
    ; m7              - store shuffle order table

    PROCESS_IPFILTER_LUMA_PS_16x4_AVX512

    movu            xm8,      [r0 + mmsize/2]
    movu            xm9,      [r0 + mmsize/2 + 8]
    movu            xm10,     [r0 + mmsize/2 + 16]

    vinserti32x4     m8,      [r0 + r1 + mmsize/2],      1
    vinserti32x4     m9,      [r0 + r1 + mmsize/2 + 8],  1
    vinserti32x4     m10,     [r0 + r1 + mmsize/2 + 16], 1

    vinserti32x4     m8,      [r0 + 2 * r1 + mmsize/2],           2
    vinserti32x4     m9,      [r0 + 2 * r1 + mmsize/2 + 8],       2
    vinserti32x4     m10,     [r0 + 2 * r1 + mmsize/2 + 16],      2

    vinserti32x4     m8,      [r0 + r6 + mmsize/2],      3
    vinserti32x4     m9,      [r0 + r6 + mmsize/2 + 8],  3
    vinserti32x4     m10,     [r0 + r6 + mmsize/2 + 16], 3

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14

    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13

    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + mmsize/2],      xm8
    vextracti32x4   [r2 + r3 + mmsize/2],     m8,        1
    vextracti32x4   [r2 + 2 * r3 + mmsize/2], m8,        2
    vextracti32x4   [r2 + r7 + mmsize/2],     m8,        3
%endmacro

%macro PROCESS_IPFILTER_LUMA_PS_24x3_AVX512 0

    PROCESS_IPFILTER_LUMA_PS_16x3_AVX512

    movu            xm8,      [r0 + mmsize/2]
    movu            xm9,      [r0 + mmsize/2 + 8]
    movu            xm10,     [r0 + mmsize/2 + 16]

    vinserti32x4     m8,      [r0 + r1 + mmsize/2],      1
    vinserti32x4     m9,      [r0 + r1 + mmsize/2 + 8],  1
    vinserti32x4     m10,     [r0 + r1 + mmsize/2 + 16], 1

    vinserti32x4     m8,      [r0 + 2 * r1 + mmsize/2],           2
    vinserti32x4     m9,      [r0 + 2 * r1 + mmsize/2 + 8],       2
    vinserti32x4     m10,     [r0 + 2 * r1 + mmsize/2 + 16],      2

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,        m2
    paddd           m11,      m14

    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,       m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13

    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2 + mmsize/2],      xm8
    vextracti32x4   [r2 + r3 + mmsize/2],     m8,        1
    vextracti32x4   [r2 + 2 * r3 + mmsize/2], m8,        2
%endmacro

%if ARCH_X86_64
INIT_ZMM avx512
cglobal interp_8tap_horiz_ps_24x32, 4, 9, 15
    shl              r1d,        1
    shl              r3d,        1
    mov              r4d,        r4m
    mov              r5d,        r5m
    shl              r4d,        6

    lea              r6,         [3 * r1]
    lea              r7,         [3 * r3]

%ifdef PIC
    lea              r8,         [tab_LumaCoeffH_avx512]
    vpbroadcastd     m0,         [r8 + r4]
    vpbroadcastd     m1,         [r8 + r4 + 4]
    vpbroadcastd     m2,         [r8 + r4 + 8]
    vpbroadcastd     m3,         [r8 + r4 + 12]
%else
    vpbroadcastd     m0,         [tab_LumaCoeffH_avx512 + r4]
    vpbroadcastd     m1,         [tab_LumaCoeffH_avx512 + r4 + 4]
    vpbroadcastd     m2,         [tab_LumaCoeffH_avx512 + r4 + 8]
    vpbroadcastd     m3,         [tab_LumaCoeffH_avx512 + r4 + 12]
%endif
    vbroadcasti32x8  m4,         [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8  m5,         [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x4  m6,         [INTERP_OFFSET_PS]
    vbroadcasti32x8  m7,         [interp8_hpp_shuf1_store_avx512]

    sub              r0,         6
    mov              r4d, 32
    test             r5d, r5d
    jz               .loop
    sub              r0,  r6
    add              r4d, 7
    PROCESS_IPFILTER_LUMA_PS_24x3_AVX512
    lea              r0,  [r0 + r6]
    lea              r2,  [r2 + r7]
    sub              r4d, 3

.loop:
    PROCESS_IPFILTER_LUMA_PS_24x4_AVX512
    lea              r0,         [r0 + 4 * r1]
    lea              r2,         [r2 + 4 * r3]
    sub              r4d,        4
    jnz              .loop
    RET
%endif
%macro PROCESS_IPFILTER_LUMA_PS_8x4_AVX512 0
    ; register map
    ; m0 , m1, m2, m3 - interpolate coeff table
    ; m4 , m5         - load shuffle order table
    ; m6              - INTERP_OFFSET_PS
    ; m7              - store shuffle order table

    movu            xm8,      [r0]
    movu            xm9,      [r0 + 8]
    movu            xm10,     [r0 + 16]

    vinserti32x4     m8,      [r0 + r1],      1
    vinserti32x4     m9,      [r0 + r1 + 8],  1
    vinserti32x4     m10,     [r0 + r1 + 16], 1

    vinserti32x4     m8,      [r0 + 2 * r1],           2
    vinserti32x4     m9,      [r0 + 2 * r1 + 8],       2
    vinserti32x4     m10,     [r0 + 2 * r1 + 16],      2

    vinserti32x4     m8,      [r0 + r6],      3
    vinserti32x4     m9,      [r0 + r6 + 8],  3
    vinserti32x4     m10,     [r0 + r6 + 16], 3

    pshufb          m11,      m8,         m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,         m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,       m2
    paddd           m11,      m14

    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,        m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13

    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2],     xm8
    vextracti32x4   [r2 + r3],     m8,        1
    vextracti32x4   [r2 + 2 * r3], m8,        2
    vextracti32x4   [r2 + r7],     m8,        3
%endmacro

%macro PROCESS_IPFILTER_LUMA_PS_8x3_AVX512 0
    movu            xm8,      [r0]
    movu            xm9,      [r0 + 8]
    movu            xm10,     [r0 + 16]

    vinserti32x4     m8,      [r0 + r1],      1
    vinserti32x4     m9,      [r0 + r1 + 8],  1
    vinserti32x4     m10,     [r0 + r1 + 16], 1

    vinserti32x4     m8,      [r0 + 2 * r1],           2
    vinserti32x4     m9,      [r0 + 2 * r1 + 8],       2
    vinserti32x4     m10,     [r0 + 2 * r1 + 16],      2

    pshufb          m11,      m8,        m5
    pshufb          m8,       m4
    pmaddwd         m8,       m0
    pmaddwd         m11,      m1
    paddd           m8,       m11
    pshufb          m12,      m9,        m5
    pshufb          m9,       m4
    pmaddwd         m11,      m12,       m3
    pmaddwd         m14,      m9,       m2
    paddd           m11,      m14

    paddd           m8,       m11
    paddd           m8,       m6
    psrad           m8,       INTERP_SHIFT_PS

    pshufb          m13,      m10,        m5
    pshufb          m10,      m4
    pmaddwd         m9,       m0
    pmaddwd         m12,      m1
    paddd           m9,       m12
    pmaddwd         m13,      m3
    pmaddwd         m10,      m2
    paddd           m10,      m13

    paddd           m9,       m10
    paddd           m9,       m6
    psrad           m9,       INTERP_SHIFT_PS

    packssdw        m8,       m9
    pshufb          m8,       m7
    movu            [r2],     xm8
    vextracti32x4   [r2 + r3],     m8,        1
    vextracti32x4   [r2 + 2 * r3], m8,        2
%endmacro

%macro IPFILTER_LUMA_PS_AVX512_8xN 1
INIT_ZMM avx512
cglobal interp_8tap_horiz_ps_8x%1, 4, 9, 15
    shl              r1d,        1
    shl              r3d,        1
    mov              r4d,        r4m
    mov              r5d,        r5m
    shl              r4d,        6

    lea              r6,         [3 * r1]
    lea              r7,         [3 * r3]

%ifdef PIC
    lea              r8,         [tab_LumaCoeffH_avx512]
    vpbroadcastd     m0,         [r8 + r4]
    vpbroadcastd     m1,         [r8 + r4 + 4]
    vpbroadcastd     m2,         [r8 + r4 + 8]
    vpbroadcastd     m3,         [r8 + r4 + 12]
%else
    vpbroadcastd     m0,         [tab_LumaCoeffH_avx512 + r4]
    vpbroadcastd     m1,         [tab_LumaCoeffH_avx512 + r4 + 4]
    vpbroadcastd     m2,         [tab_LumaCoeffH_avx512 + r4 + 8]
    vpbroadcastd     m3,         [tab_LumaCoeffH_avx512 + r4 + 12]
%endif
    vbroadcasti32x8  m4,         [interp8_hpp_shuf1_load_avx512]
    vbroadcasti32x8  m5,         [interp8_hpp_shuf2_load_avx512]
    vbroadcasti32x4  m6,         [INTERP_OFFSET_PS]
    vbroadcasti32x8  m7,         [interp8_hpp_shuf1_store_avx512]

    sub              r0,         6
    mov              r4d, %1
    test             r5d, r5d
    jz               .loop
    sub              r0,  r6
    add              r4d, 7
    PROCESS_IPFILTER_LUMA_PS_8x3_AVX512
    lea              r0,  [r0 + r6]
    lea              r2,  [r2 + r7]
    sub              r4d, 3

.loop:
    PROCESS_IPFILTER_LUMA_PS_8x4_AVX512
    lea              r0,         [r0 + 4 * r1]
    lea              r2,         [r2 + 4 * r3]
    sub              r4d,        4
    jnz              .loop
    RET
%endmacro

%if ARCH_X86_64
    IPFILTER_LUMA_PS_AVX512_8xN 4
    IPFILTER_LUMA_PS_AVX512_8xN 8
    IPFILTER_LUMA_PS_AVX512_8xN 16
    IPFILTER_LUMA_PS_AVX512_8xN 32
%endif

;-------------------------------------------------------------------------------------------------------------
;avx512 luma_hps code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
;avx512 luma_vss and luma_vsp code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_LUMA_VERT_S_8x8_AVX512 1
    lea                  r6,                  [r0 + 4 * r1]
    movu                 xm1,                 [r0]                           ;0 row
    vinserti32x4         m1,                  [r0 + 2 * r1],          1
    vinserti32x4         m1,                  [r0 + 4 * r1],          2
    vinserti32x4         m1,                  [r6 + 2 * r1],          3
    movu                 xm3,                 [r0 + r1]                      ;1 row
    vinserti32x4         m3,                  [r0 + r7],              1
    vinserti32x4         m3,                  [r6 + r1],              2
    vinserti32x4         m3,                  [r6 + r7],              3
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 xm4,                 [r0 + 2 * r1]                  ;2 row
    vinserti32x4         m4,                  [r0 + 4 * r1],          1
    vinserti32x4         m4,                  [r6 + 2 * r1],          2
    vinserti32x4         m4,                  [r6 + 4 * r1],          3
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    lea                  r4,                  [r6 + 4 * r1]
    movu                 xm5,                 [r0 + r7]                      ;3 row
    vinserti32x4         m5,                  [r6 + r1],              1
    vinserti32x4         m5,                  [r6 + r7],              2
    vinserti32x4         m5,                  [r4 + r1],              3
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 xm4,                 [r0 + 4 * r1]                  ;4 row
    vinserti32x4         m4,                  [r6 + 2 * r1],              1
    vinserti32x4         m4,                  [r6 + 4 * r1],              2
    vinserti32x4         m4,                  [r4 + 2 * r1],              3
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 xm11,                [r6 + r1]                      ;5 row
    vinserti32x4         m11,                 [r6 + r7],              1
    vinserti32x4         m11,                 [r4 + r1],              2
    vinserti32x4         m11,                 [r4 + r7],              3
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 xm12,                [r6 + 2 * r1]                  ;6 row
    vinserti32x4         m12,                 [r6 + 4 * r1],          1
    vinserti32x4         m12,                 [r4 + 2 * r1],          2
    vinserti32x4         m12,                 [r4 + 4 * r1],          3
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    lea                  r8,                  [r4 + 4 * r1]
    movu                 xm13,                [r6 + r7]                      ;7 row
    vinserti32x4         m13,                 [r4 + r1],              1
    vinserti32x4         m13,                 [r4 + r7],              2
    vinserti32x4         m13,                 [r8 + r1],              3
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 xm12,                [r6 + 4 * r1]                 ; 8 row
    vinserti32x4         m12,                 [r4 + 2 * r1],          1
    vinserti32x4         m12,                 [r4 + 4 * r1],          2
    vinserti32x4         m12,                 [r8 + 2 * r1],          3
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  INTERP_SHIFT_SP
    psrad                m1,                  INTERP_SHIFT_SP
    psrad                m2,                  INTERP_SHIFT_SP
    psrad                m3,                  INTERP_SHIFT_SP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif

    movu                 [r2],                xm0
    movu                 [r2 + r3],           xm2
    vextracti32x4        [r2 + 2 * r3],       m0,                  1
    vextracti32x4        [r2 + r5],           m2,                  1
    lea                  r2,                  [r2 + 4 * r3]
    vextracti32x4        [r2],                m0,                  2
    vextracti32x4        [r2 + r3],           m2,                  2
    vextracti32x4        [r2 + 2 * r3],       m0,                  3
    vextracti32x4        [r2 + r5],           m2,                  3
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_LUMA_8xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_8x%2, 5, 9, 22
    add                   r1d,                r1d
    add                   r3d,                r3d
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [tab_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m19,                [INTERP_OFFSET_SP]
    pxor                  m20,                m20
    vbroadcasti32x8       m21,                [pw_pixel_max]
%endif
    lea                   r5,                 [3 * r3]

%rep %2/8 - 1
    PROCESS_LUMA_VERT_S_8x8_AVX512 %1
    lea                   r0,                 [r4]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_S_8x8_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_LUMA_8xN_AVX512 ss, 8
    FILTER_VER_S_LUMA_8xN_AVX512 ss, 16
    FILTER_VER_S_LUMA_8xN_AVX512 ss, 32
    FILTER_VER_S_LUMA_8xN_AVX512 sp, 8
    FILTER_VER_S_LUMA_8xN_AVX512 sp, 16
    FILTER_VER_S_LUMA_8xN_AVX512 sp, 32
%endif

%macro PROCESS_LUMA_VERT_S_16x4_AVX512 1
    movu                 ym1,                 [r0]
    movu                 ym3,                 [r0 + r1]
    vinserti32x8         m1,                  [r0 + 2 * r1],          1
    vinserti32x8         m3,                  [r0 + r7],              1
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    lea                  r6,                  [r0 + 4 * r1]
    movu                 ym4,                 [r0 + 2 * r1]
    vinserti32x8         m4,                  [r6],                   1
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 ym5,                 [r0 + r7]
    vinserti32x8         m5,                  [r6 + r1],              1
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 ym4,                 [r6]
    vinserti32x8         m4,                  [r6 + 2 * r1],          1
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 ym11,                [r6 + r1]
    vinserti32x8         m11,                 [r6 + r7],              1
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 ym12,                [r6 + 2 * r1]
    vinserti32x8         m12,                 [r6 + 4 * r1],          1
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    lea                  r4,                  [r6 + 4 * r1]
    movu                 ym13,                [r6 + r7]
    vinserti32x8         m13,                 [r4 + r1],              1
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 ym12,                [r6 + 4 * r1]
    vinserti32x8         m12,                 [r4 + 2 * r1],          1
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  INTERP_SHIFT_SP
    psrad                m1,                  INTERP_SHIFT_SP
    psrad                m2,                  INTERP_SHIFT_SP
    psrad                m3,                  INTERP_SHIFT_SP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif

    movu                 [r2],                ym0
    movu                 [r2 + r3],           ym2
    vextracti32x8        [r2 + 2 * r3],       m0,                1
    vextracti32x8        [r2 + r5],           m2,                1
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_LUMA_16xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_16x%2, 5, 8, 22
    add                   r1d,                r1d
    add                   r3d,                r3d
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [tab_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m19,                [INTERP_OFFSET_SP]
    pxor                  m20,                m20
    vbroadcasti32x8       m21,                [pw_pixel_max]
%endif
    lea                   r5,                 [3 * r3]
%rep %2/4 - 1
    PROCESS_LUMA_VERT_S_16x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_S_16x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_LUMA_16xN_AVX512 ss, 4
    FILTER_VER_S_LUMA_16xN_AVX512 ss, 8
    FILTER_VER_S_LUMA_16xN_AVX512 ss, 12
    FILTER_VER_S_LUMA_16xN_AVX512 ss, 16
    FILTER_VER_S_LUMA_16xN_AVX512 ss, 32
    FILTER_VER_S_LUMA_16xN_AVX512 ss, 64
    FILTER_VER_S_LUMA_16xN_AVX512 sp, 4
    FILTER_VER_S_LUMA_16xN_AVX512 sp, 8
    FILTER_VER_S_LUMA_16xN_AVX512 sp, 12
    FILTER_VER_S_LUMA_16xN_AVX512 sp, 16
    FILTER_VER_S_LUMA_16xN_AVX512 sp, 32
    FILTER_VER_S_LUMA_16xN_AVX512 sp, 64
%endif

%macro PROCESS_LUMA_VERT_S_24x8_AVX512 1
    PROCESS_LUMA_VERT_S_16x4_AVX512 %1
    lea                  r4,                  [r6 + 4 * r1]
    lea                  r8,                  [r4 + 4 * r1]
    movu                 ym1,                 [r6]
    movu                 ym3,                 [r6 + r1]
    vinserti32x8         m1,                  [r6 + 2 * r1],          1
    vinserti32x8         m3,                  [r6 + r7],              1
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 ym4,                 [r6 + 2 * r1]
    vinserti32x8         m4,                  [r4],                   1
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 ym5,                 [r6 + r7]
    vinserti32x8         m5,                  [r4 + r1],              1
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 ym4,                 [r4]
    vinserti32x8         m4,                  [r4 + 2 * r1],          1
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 ym11,                [r4 + r1]
    vinserti32x8         m11,                 [r4 + r7],              1
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 ym12,                [r4 + 2 * r1]
    vinserti32x8         m12,                 [r4 + 4 * r1],          1
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 ym13,                [r4 + r7]
    vinserti32x8         m13,                 [r8 + r1],              1
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 ym12,                [r4 + 4 * r1]
    vinserti32x8         m12,                 [r8 + 2 * r1],          1
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  INTERP_SHIFT_SP
    psrad                m1,                  INTERP_SHIFT_SP
    psrad                m2,                  INTERP_SHIFT_SP
    psrad                m3,                  INTERP_SHIFT_SP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif
    lea                  r9,                  [r2 + 4 * r3]
    movu                 [r9],                ym0
    movu                 [r9 + r3],           ym2
    vextracti32x8        [r9 + 2 * r3],       m0,                1
    vextracti32x8        [r9 + r5],           m2,                1

    movu                 xm1,                 [r0 + mmsize/2]
    vinserti32x4         m1,                  [r0 + 2 * r1 + mmsize/2],          1
    vinserti32x4         m1,                  [r0 + 4 * r1 + mmsize/2],          2
    vinserti32x4         m1,                  [r6 + 2 * r1 + mmsize/2],          3
    movu                 xm3,                 [r0 + r1 + mmsize/2]
    vinserti32x4         m3,                  [r0 + r7 + mmsize/2],              1
    vinserti32x4         m3,                  [r6 + r1 + mmsize/2],              2
    vinserti32x4         m3,                  [r6 + r7 + mmsize/2],              3
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 xm4,                 [r0 + 2 * r1 + mmsize/2]
    vinserti32x4         m4,                  [r0 + 4 * r1 + mmsize/2],          1
    vinserti32x4         m4,                  [r6 + 2 * r1 + mmsize/2],          2
    vinserti32x4         m4,                  [r6 + 4 * r1 + mmsize/2],          3
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 xm5,                 [r0 + r7 + mmsize/2]
    vinserti32x4         m5,                  [r6 + r1 + mmsize/2],              1
    vinserti32x4         m5,                  [r6 + r7 + mmsize/2],              2
    vinserti32x4         m5,                  [r4 + r1 + mmsize/2],              3
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 xm4,                 [r0 + 4 * r1 + mmsize/2]
    vinserti32x4         m4,                  [r6 + 2 * r1 + mmsize/2],              1
    vinserti32x4         m4,                  [r6 + 4 * r1 + mmsize/2],              2
    vinserti32x4         m4,                  [r4 + 2 * r1 + mmsize/2],              3
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 xm11,                [r6 + r1 + mmsize/2]
    vinserti32x4         m11,                 [r6 + r7 + mmsize/2],              1
    vinserti32x4         m11,                 [r4 + r1 + mmsize/2],              2
    vinserti32x4         m11,                 [r4 + r7 + mmsize/2],              3
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 xm12,                [r6 + 2 * r1 + mmsize/2]
    vinserti32x4         m12,                 [r6 + 4 * r1 + mmsize/2],          1
    vinserti32x4         m12,                 [r4 + 2 * r1 + mmsize/2],          2
    vinserti32x4         m12,                 [r4 + 4 * r1 + mmsize/2],          3
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 xm13,                [r6 + r7 + mmsize/2]
    vinserti32x4         m13,                 [r4 + r1 + mmsize/2],              1
    vinserti32x4         m13,                 [r4 + r7 + mmsize/2],              2
    vinserti32x4         m13,                 [r8 + r1 + mmsize/2],              3
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 xm12,                [r6 + 4 * r1 + mmsize/2]
    vinserti32x4         m12,                 [r4 + 2 * r1 + mmsize/2],          1
    vinserti32x4         m12,                 [r4 + 4 * r1 + mmsize/2],          2
    vinserti32x4         m12,                 [r8 + 2 * r1 + mmsize/2],          3
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  INTERP_SHIFT_SP
    psrad                m1,                  INTERP_SHIFT_SP
    psrad                m2,                  INTERP_SHIFT_SP
    psrad                m3,                  INTERP_SHIFT_SP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif

    movu                 [r2 + mmsize/2],                xm0
    movu                 [r2 + r3 + mmsize/2],           xm2
    vextracti32x4        [r2 + 2 * r3 + mmsize/2],       m0,                  1
    vextracti32x4        [r2 + r5 + mmsize/2],           m2,                  1
    lea                  r2,                             [r2 + 4 * r3]
    vextracti32x4        [r2 + mmsize/2],                m0,                  2
    vextracti32x4        [r2 + r3 + mmsize/2],           m2,                  2
    vextracti32x4        [r2 + 2 * r3 + mmsize/2],       m0,                  3
    vextracti32x4        [r2 + r5 + mmsize/2],           m2,                  3
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_LUMA_24x32_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_24x32, 5, 10, 22
    add                   r1d,                r1d
    add                   r3d,                r3d
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [tab_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m19,                [INTERP_OFFSET_SP]
    pxor                  m20,                m20
    vbroadcasti32x8       m21,                [pw_pixel_max]
%endif
    lea                   r5,                 [3 * r3]

%rep 3
    PROCESS_LUMA_VERT_S_24x8_AVX512 %1
    lea                   r0,                 [r4]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_S_24x8_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_LUMA_24x32_AVX512 ss
    FILTER_VER_S_LUMA_24x32_AVX512 sp
%endif

%macro PROCESS_LUMA_VERT_S_32x2_AVX512 1
    movu                 m1,                  [r0]                           ;0 row
    movu                 m3,                  [r0 + r1]                      ;1 row
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 m4,                  [r0 + 2 * r1]                  ;2 row
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 m5,                  [r0 + r7]                      ;3 row
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 m4,                  [r0 + 4 * r1]                  ;4 row
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    lea                  r6,                  [r0 + 4 * r1]

    movu                 m11,                 [r6 + r1]                      ;5 row
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 m12,                 [r6 + 2 * r1]                  ;6 row
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 m13,                 [r6 + r7]                      ;7 row
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 m12,                 [r6 + 4 * r1]                 ; 8 row
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  INTERP_SHIFT_SP
    psrad                m1,                  INTERP_SHIFT_SP
    psrad                m2,                  INTERP_SHIFT_SP
    psrad                m3,                  INTERP_SHIFT_SP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif

    movu                 [r2],                m0
    movu                 [r2 + r3],           m2
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_LUMA_32xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_32x%2, 5, 8, 22
    add                   r1d,                r1d
    add                   r3d,                r3d
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [tab_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m19,                [INTERP_OFFSET_SP]
    pxor                  m20,                m20
    vbroadcasti32x8       m21,                [pw_pixel_max]
%endif

%rep %2/2 - 1
    PROCESS_LUMA_VERT_S_32x2_AVX512 %1
    lea                   r0,                 [r0 + 2 * r1]
    lea                   r2,                 [r2 + 2 * r3]
%endrep
    PROCESS_LUMA_VERT_S_32x2_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_LUMA_32xN_AVX512 ss, 8
    FILTER_VER_S_LUMA_32xN_AVX512 ss, 16
    FILTER_VER_S_LUMA_32xN_AVX512 ss, 32
    FILTER_VER_S_LUMA_32xN_AVX512 ss, 24
    FILTER_VER_S_LUMA_32xN_AVX512 ss, 64
    FILTER_VER_S_LUMA_32xN_AVX512 sp, 8
    FILTER_VER_S_LUMA_32xN_AVX512 sp, 16
    FILTER_VER_S_LUMA_32xN_AVX512 sp, 32
    FILTER_VER_S_LUMA_32xN_AVX512 sp, 24
    FILTER_VER_S_LUMA_32xN_AVX512 sp, 64
%endif

%macro PROCESS_LUMA_VERT_S_48x4_AVX512 1
    PROCESS_LUMA_VERT_S_32x2_AVX512 %1
    movu                 m1,                  [r0 + 2 * r1]
    movu                 m3,                  [r0 + r7]
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 m4,                  [r0 + 4 * r1]
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 m5,                  [r6 + r1]
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    lea                  r4,                  [r6 + 4 * r1]

    movu                 m4,                  [r6 + 2 * r1]
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 m11,                 [r6 + r7]
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 m12,                 [r4]
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 m13,                 [r4 + r1]
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 m12,                 [r4 + 2 * r1]
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  INTERP_SHIFT_SP
    psrad                m1,                  INTERP_SHIFT_SP
    psrad                m2,                  INTERP_SHIFT_SP
    psrad                m3,                  INTERP_SHIFT_SP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif

    movu                 [r2 + 2 * r3],       m0
    movu                 [r2 + r5],           m2

    movu                 ym1,                 [r0 + mmsize]
    movu                 ym3,                 [r0 + r1 + mmsize]
    vinserti32x8         m1,                  [r0 + 2 * r1 + mmsize], 1
    vinserti32x8         m3,                  [r0 + r7 + mmsize],     1
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 ym4,                 [r0 + 2 * r1 + mmsize]
    vinserti32x8         m4,                  [r6 + mmsize],          1
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 ym5,                 [r0 + r7 + mmsize]
    vinserti32x8         m5,                  [r6 + r1 + mmsize],     1
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 ym4,                 [r6 + mmsize]
    vinserti32x8         m4,                  [r6 + 2 * r1 + mmsize], 1
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 ym11,                [r6 + r1 + mmsize]
    vinserti32x8         m11,                 [r6 + r7 + mmsize],     1
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 ym12,                [r6 + 2 * r1 + mmsize]
    vinserti32x8         m12,                 [r6 + 4 * r1 + mmsize], 1
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 ym13,                [r6 + r7 + mmsize]
    vinserti32x8         m13,                 [r4 + r1 + mmsize],     1
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 ym12,                [r6 + 4 * r1 + mmsize]
    vinserti32x8         m12,                 [r4 + 2 * r1 + mmsize], 1
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  INTERP_SHIFT_SP
    psrad                m1,                  INTERP_SHIFT_SP
    psrad                m2,                  INTERP_SHIFT_SP
    psrad                m3,                  INTERP_SHIFT_SP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif

    movu                 [r2 + mmsize],                ym0
    movu                 [r2 + r3 + mmsize],           ym2
    vextracti32x8        [r2 + 2 * r3 + mmsize],       m0,                1
    vextracti32x8        [r2 + r5 + mmsize],           m2,                1
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_LUMA_48x64_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_48x64, 5, 8, 22
    add                   r1d,                r1d
    add                   r3d,                r3d
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [tab_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m19,                [INTERP_OFFSET_SP]
    pxor                  m20,                m20
    vbroadcasti32x8       m21,                [pw_pixel_max]
%endif

    lea                   r5,                 [3 * r3]
%rep 15
    PROCESS_LUMA_VERT_S_48x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_S_48x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_LUMA_48x64_AVX512 ss
    FILTER_VER_S_LUMA_48x64_AVX512 sp
%endif

%macro PROCESS_LUMA_VERT_S_64x2_AVX512 1
    movu                 m1,                  [r0]                           ;0 row
    movu                 m3,                  [r0 + r1]                      ;1 row
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 m4,                  [r0 + 2 * r1]                  ;2 row
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 m5,                  [r0 + r7]                      ;3 row
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 m4,                  [r0 + 4 * r1]                  ;4 row
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    lea                  r6,                  [r0 + 4 * r1]

    movu                 m11,                 [r6 + r1]                      ;5 row
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 m12,                 [r6 + 2 * r1]                  ;6 row
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 m13,                 [r6 + r7]                      ;7 row
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 m12,                 [r6 + 4 * r1]                 ; 8 row
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  INTERP_SHIFT_SP
    psrad                m1,                  INTERP_SHIFT_SP
    psrad                m2,                  INTERP_SHIFT_SP
    psrad                m3,                  INTERP_SHIFT_SP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif

    movu                 [r2],                m0
    movu                 [r2 + r3],           m2

    movu                 m1,                  [r0 + mmsize]                  ;0 row
    movu                 m3,                  [r0 + r1 + mmsize]             ;1 row
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 m4,                  [r0 + 2 * r1 + mmsize]         ;2 row
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 m5,                  [r0 + r7 + mmsize]             ;3 row
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 m4,                  [r0 + 4 * r1 + mmsize]         ;4 row
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 m11,                 [r6 + r1 + mmsize]             ;5 row
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 m12,                 [r6 + 2 * r1 + mmsize]         ;6 row
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 m13,                 [r6 + r7 + mmsize]             ;7 row
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 m12,                 [r6 + 4 * r1 + mmsize]         ; 8 row
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

%ifidn %1, sp
    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

    psrad                m0,                  INTERP_SHIFT_SP
    psrad                m1,                  INTERP_SHIFT_SP
    psrad                m2,                  INTERP_SHIFT_SP
    psrad                m3,                  INTERP_SHIFT_SP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  6
    psrad                m1,                  6
    psrad                m2,                  6
    psrad                m3,                  6

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif

    movu                 [r2 + mmsize],       m0
    movu                 [r2 + r3 + mmsize],  m2
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_S_LUMA_64xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_64x%2, 5, 8, 22
    add                   r1d,                r1d
    add                   r3d,                r3d
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [tab_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, sp
    vbroadcasti32x4       m19,                [INTERP_OFFSET_SP]
    pxor                  m20,                m20
    vbroadcasti32x8       m21,                [pw_pixel_max]
%endif

%rep %2/2 - 1
    PROCESS_LUMA_VERT_S_64x2_AVX512 %1
    lea                   r0,                 [r0 + 2 * r1]
    lea                   r2,                 [r2 + 2 * r3]
%endrep
    PROCESS_LUMA_VERT_S_64x2_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_S_LUMA_64xN_AVX512 ss, 16
    FILTER_VER_S_LUMA_64xN_AVX512 ss, 32
    FILTER_VER_S_LUMA_64xN_AVX512 ss, 48
    FILTER_VER_S_LUMA_64xN_AVX512 ss, 64
    FILTER_VER_S_LUMA_64xN_AVX512 sp, 16
    FILTER_VER_S_LUMA_64xN_AVX512 sp, 32
    FILTER_VER_S_LUMA_64xN_AVX512 sp, 48
    FILTER_VER_S_LUMA_64xN_AVX512 sp, 64
%endif
;-------------------------------------------------------------------------------------------------------------
;avx512 luma_vss and luma_vsp code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
;avx512 luma_vpp and luma_vps code start
;-------------------------------------------------------------------------------------------------------------
%macro PROCESS_LUMA_VERT_P_16x4_AVX512 1
    lea                  r5,                  [r0 + 4 * r1]
    movu                 ym1,                 [r0]
    movu                 ym3,                 [r0 + r1]
    vinserti32x8         m1,                  [r0 + 2 * r1],          1
    vinserti32x8         m3,                  [r0 + r7],              1
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 ym4,                 [r0 + 2 * r1]
    vinserti32x8         m4,                  [r0 + 4 * r1],          1
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 ym5,                 [r0 + r7]
    vinserti32x8         m5,                  [r5 + r1],              1
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 ym4,                 [r5]
    vinserti32x8         m4,                  [r5 + 2 * r1],          1
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    lea                  r4,                  [r5 + 4 * r1]
    movu                 ym11,                [r5 + r1]
    vinserti32x8         m11,                 [r5 + r7],              1
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 ym12,                [r5 + 2 * r1]
    vinserti32x8         m12,                 [r4],                   1
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 ym13,                [r5 + r7]
    vinserti32x8         m13,                 [r4 + r1],              1
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 ym12,                [r4]
    vinserti32x8         m12,                 [r4 + 2 * r1],          1
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

%ifidn %1, pp
    psrad                m0,                  INTERP_SHIFT_PP
    psrad                m1,                  INTERP_SHIFT_PP
    psrad                m2,                  INTERP_SHIFT_PP
    psrad                m3,                  INTERP_SHIFT_PP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  INTERP_SHIFT_PS
    psrad                m1,                  INTERP_SHIFT_PS
    psrad                m2,                  INTERP_SHIFT_PS
    psrad                m3,                  INTERP_SHIFT_PS

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif

    movu                 [r2],                ym0
    movu                 [r2 + r3],           ym2
    vextracti32x8        [r2 + 2 * r3],       m0,                    1
    vextracti32x8        [r2 + r8],           m2,                    1
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_P_LUMA_16xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_16x%2, 5, 9, 22
    add                   r1d,                r1d
    add                   r3d,                r3d
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [tab_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, pp
    vbroadcasti32x4       m19,                [INTERP_OFFSET_PP]
    pxor                  m20,                m20
    vbroadcasti32x8       m21,                [pw_pixel_max]
%else
    vbroadcasti32x4       m19,                [INTERP_OFFSET_PS]
%endif
    lea                   r7,                 [3 * r1]
    lea                   r8,                 [3 * r3]
    sub                   r0,                 r7

%rep %2/4 - 1
    PROCESS_LUMA_VERT_P_16x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_P_16x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_P_LUMA_16xN_AVX512 ps, 4
    FILTER_VER_P_LUMA_16xN_AVX512 ps, 8
    FILTER_VER_P_LUMA_16xN_AVX512 ps, 12
    FILTER_VER_P_LUMA_16xN_AVX512 ps, 16
    FILTER_VER_P_LUMA_16xN_AVX512 ps, 32
    FILTER_VER_P_LUMA_16xN_AVX512 ps, 64
    FILTER_VER_P_LUMA_16xN_AVX512 pp, 4
    FILTER_VER_P_LUMA_16xN_AVX512 pp, 8
    FILTER_VER_P_LUMA_16xN_AVX512 pp, 12
    FILTER_VER_P_LUMA_16xN_AVX512 pp, 16
    FILTER_VER_P_LUMA_16xN_AVX512 pp, 32
    FILTER_VER_P_LUMA_16xN_AVX512 pp, 64
%endif

%macro PROCESS_LUMA_VERT_P_24x4_AVX512 1
    PROCESS_LUMA_VERT_P_16x4_AVX512 %1
    movu                  xm1,                [r0 + mmsize/2]
    movu                  xm3,                [r0 + r1 + mmsize/2]
    vinserti32x4          m1,                 [r0 + r1 + mmsize/2],           1
    vinserti32x4          m3,                 [r0 + 2 * r1 + mmsize/2],       1
    vinserti32x4          m1,                 [r0 + 2 * r1 + mmsize/2],       2
    vinserti32x4          m3,                 [r0 + r7 + mmsize/2],           2
    vinserti32x4          m1,                 [r0 + r7 + mmsize/2],           3
    vinserti32x4          m3,                 [r0 + 4 * r1 + mmsize/2],       3

    punpcklwd             m0,                 m1,                  m3
    pmaddwd               m0,                 m15
    punpckhwd             m1,                 m3
    pmaddwd               m1,                 m15

    movu                  xm4,                [r0 + 2 * r1 + mmsize/2]
    movu                  xm5,                [r0 + r7 + mmsize/2]
    vinserti32x4          m4,                 [r0 + r7 + mmsize/2],           1
    vinserti32x4          m5,                 [r5 + mmsize/2],                1
    vinserti32x4          m4,                 [r5 + mmsize/2],                2
    vinserti32x4          m5,                 [r5 + r1 + mmsize/2],           2
    vinserti32x4          m4,                 [r5 + r1 + mmsize/2],           3
    vinserti32x4          m5,                 [r5 + 2 * r1 + mmsize/2],       3

    punpcklwd             m3,                 m4,                  m5
    pmaddwd               m3,                 m16
    punpckhwd             m4,                 m5
    pmaddwd               m4,                 m16

    paddd                 m0,                 m3
    paddd                 m1,                 m4

    movu                  xm3,                [r5 + mmsize/2]
    movu                  xm5,                [r5 + r1 + mmsize/2]
    vinserti32x4          m3,                 [r5 + r1 + mmsize/2],           1
    vinserti32x4          m5,                 [r5 + 2 * r1 + mmsize/2],       1
    vinserti32x4          m3,                 [r5 + 2 * r1 + mmsize/2],       2
    vinserti32x4          m5,                 [r5 + r7 + mmsize/2],           2
    vinserti32x4          m3,                 [r5 + r7 + mmsize/2],           3
    vinserti32x4          m5,                 [r5 + 4 * r1 + mmsize/2],       3

    punpcklwd             m2,                 m3,                  m5
    pmaddwd               m2,                 m17
    punpckhwd             m3,                 m5
    pmaddwd               m3,                 m17

    movu                  xm6,                [r5 + 2 * r1 + mmsize/2]
    movu                  xm7,                [r5 + r7 + mmsize/2]
    vinserti32x4          m6,                 [r5 + r7 + mmsize/2],           1
    vinserti32x4          m7,                 [r4 + mmsize/2],                1
    vinserti32x4          m6,                 [r4 + mmsize/2],                2
    vinserti32x4          m7,                 [r4 + r1 + mmsize/2],           2
    vinserti32x4          m6,                 [r4 + r1 + mmsize/2],           3
    vinserti32x4          m7,                 [r4 + 2 * r1 + mmsize/2],       3

    punpcklwd             m5,                 m6,                  m7
    pmaddwd               m5,                 m18
    punpckhwd             m6,                 m7
    pmaddwd               m6,                 m18

    paddd                 m2,                 m5
    paddd                 m3,                 m6
    paddd                 m0,                 m2
    paddd                 m1,                 m3

    paddd                 m0,                 m19
    paddd                 m1,                 m19

%ifidn %1, pp
    psrad                 m0,                 INTERP_SHIFT_PP
    psrad                 m1,                 INTERP_SHIFT_PP
    packssdw              m0,                 m1
    CLIPW                 m0,                 m20,                 m21
%else
    psrad                 m0,                 INTERP_SHIFT_PS
    psrad                 m1,                 INTERP_SHIFT_PS
    packssdw              m0,                 m1
%endif

    movu                 [r2 + mmsize/2],                xm0
    vextracti32x4        [r2 + r3 + mmsize/2],           m0,                    1
    vextracti32x4        [r2 + 2 * r3 + mmsize/2],       m0,                    2
    vextracti32x4        [r2 + r8 + mmsize/2],           m0,                    3
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_P_LUMA_24xN_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_24x32, 5, 9, 22
    add                   r1d,                r1d
    add                   r3d,                r3d
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [tab_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, pp
    vbroadcasti32x4       m19,                [INTERP_OFFSET_PP]
    pxor                  m20,                m20
    vbroadcasti32x8       m21,                [pw_pixel_max]
%else
    vbroadcasti32x4       m19,                [INTERP_OFFSET_PS]
%endif
    lea                   r7,                 [3 * r1]
    lea                   r8,                 [3 * r3]
    sub                   r0,                 r7

%rep 7
    PROCESS_LUMA_VERT_P_24x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_P_24x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_P_LUMA_24xN_AVX512 ps
    FILTER_VER_P_LUMA_24xN_AVX512 pp
%endif

%macro PROCESS_LUMA_VERT_P_32x2_AVX512 1
    movu                 m1,                  [r0]                           ;0 row
    movu                 m3,                  [r0 + r1]                      ;1 row
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 m4,                  [r0 + 2 * r1]                  ;2 row
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 m5,                  [r0 + r7]                      ;3 row
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 m4,                  [r0 + 4 * r1]                  ;4 row
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    lea                  r6,                  [r0 + 4 * r1]

    movu                 m11,                 [r6 + r1]                      ;5 row
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 m12,                 [r6 + 2 * r1]                  ;6 row
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 m13,                 [r6 + r7]                      ;7 row
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 m12,                 [r6 + 4 * r1]                 ; 8 row
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

%ifidn %1, pp
    psrad                m0,                  INTERP_SHIFT_PP
    psrad                m1,                  INTERP_SHIFT_PP
    psrad                m2,                  INTERP_SHIFT_PP
    psrad                m3,                  INTERP_SHIFT_PP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  INTERP_SHIFT_PS
    psrad                m1,                  INTERP_SHIFT_PS
    psrad                m2,                  INTERP_SHIFT_PS
    psrad                m3,                  INTERP_SHIFT_PS

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif

    movu                 [r2],                m0
    movu                 [r2 + r3],           m2
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_P_LUMA_32xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_32x%2, 5, 8, 22
    add                   r1d,                r1d
    add                   r3d,                r3d
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [tab_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, pp
    vbroadcasti32x4       m19,                [INTERP_OFFSET_PP]
    pxor                  m20,                m20
    vbroadcasti32x8       m21,                [pw_pixel_max]
%else
    vbroadcasti32x4       m19,                [INTERP_OFFSET_PS]
%endif
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7

%rep %2/2 - 1
    PROCESS_LUMA_VERT_P_32x2_AVX512 %1
    lea                   r0,                 [r0 + 2 * r1]
    lea                   r2,                 [r2 + 2 * r3]
%endrep
    PROCESS_LUMA_VERT_P_32x2_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_P_LUMA_32xN_AVX512 ps, 8
    FILTER_VER_P_LUMA_32xN_AVX512 ps, 16
    FILTER_VER_P_LUMA_32xN_AVX512 ps, 32
    FILTER_VER_P_LUMA_32xN_AVX512 ps, 24
    FILTER_VER_P_LUMA_32xN_AVX512 ps, 64
    FILTER_VER_P_LUMA_32xN_AVX512 pp, 8
    FILTER_VER_P_LUMA_32xN_AVX512 pp, 16
    FILTER_VER_P_LUMA_32xN_AVX512 pp, 32
    FILTER_VER_P_LUMA_32xN_AVX512 pp, 24
    FILTER_VER_P_LUMA_32xN_AVX512 pp, 64
%endif

%macro PROCESS_LUMA_VERT_P_48x4_AVX512 1
    PROCESS_LUMA_VERT_P_32x2_AVX512 %1
    movu                 m1,                  [r0 + 2 * r1]
    movu                 m3,                  [r0 + r7]
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 m4,                  [r0 + 4 * r1]
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 m5,                  [r6 + r1]
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 m4,                  [r6 + 2 * r1]
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    lea                  r4,                  [r6 + 4 * r1]

    movu                 m11,                 [r6 + r7]
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 m12,                 [r6 + 4 * r1]
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 m13,                 [r4 + r1]
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 m12,                 [r4 + 2 * r1]
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

%ifidn %1, pp
    psrad                m0,                  INTERP_SHIFT_PP
    psrad                m1,                  INTERP_SHIFT_PP
    psrad                m2,                  INTERP_SHIFT_PP
    psrad                m3,                  INTERP_SHIFT_PP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  INTERP_SHIFT_PS
    psrad                m1,                  INTERP_SHIFT_PS
    psrad                m2,                  INTERP_SHIFT_PS
    psrad                m3,                  INTERP_SHIFT_PS

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif
    movu                 [r2 + 2 * r3],       m0
    movu                 [r2 + r8],           m2

    movu                 ym1,                 [r0 + mmsize]
    movu                 ym3,                 [r0 + r1 + mmsize]
    vinserti32x8         m1,                  [r0 + 2 * r1 + mmsize], 1
    vinserti32x8         m3,                  [r0 + r7 + mmsize],     1
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 ym4,                 [r0 + 2 * r1 + mmsize]
    vinserti32x8         m4,                  [r0 + 4 * r1 + mmsize], 1
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 ym5,                 [r0 + r7 + mmsize]
    vinserti32x8         m5,                  [r6 + r1 + mmsize],     1
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 ym4,                 [r6 + mmsize]
    vinserti32x8         m4,                  [r6 + 2 * r1 + mmsize], 1
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 ym11,                [r6 + r1 + mmsize]
    vinserti32x8         m11,                 [r6 + r7 + mmsize],     1
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 ym12,                [r6 + 2 * r1 + mmsize]
    vinserti32x8         m12,                 [r4 + mmsize],          1
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 ym13,                [r6 + r7 + mmsize]
    vinserti32x8         m13,                 [r4 + r1 + mmsize],     1
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 ym12,                [r4 + mmsize]
    vinserti32x8         m12,                 [r4 + 2 * r1 + mmsize], 1
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

%ifidn %1, pp
    psrad                m0,                  INTERP_SHIFT_PP
    psrad                m1,                  INTERP_SHIFT_PP
    psrad                m2,                  INTERP_SHIFT_PP
    psrad                m3,                  INTERP_SHIFT_PP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  INTERP_SHIFT_PS
    psrad                m1,                  INTERP_SHIFT_PS
    psrad                m2,                  INTERP_SHIFT_PS
    psrad                m3,                  INTERP_SHIFT_PS

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif

    movu                 [r2 + mmsize],                ym0
    movu                 [r2 + r3 + mmsize],           ym2
    vextracti32x8        [r2 + 2 * r3 + mmsize],       m0,                    1
    vextracti32x8        [r2 + r8 + mmsize],           m2,                    1
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_4tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_P_LUMA_48x64_AVX512 1
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_48x64, 5, 9, 22
    add                   r1d,                r1d
    add                   r3d,                r3d
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [tab_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, pp
    vbroadcasti32x4       m19,                [INTERP_OFFSET_PP]
    pxor                  m20,                m20
    vbroadcasti32x8       m21,                [pw_pixel_max]
%else
    vbroadcasti32x4       m19,                [INTERP_OFFSET_PS]
%endif
    lea                   r7,                 [3 * r1]
    lea                   r8,                 [3 * r3]
    sub                   r0,                 r7

%rep 15
    PROCESS_LUMA_VERT_P_48x4_AVX512 %1
    lea                   r0,                 [r0 + 4 * r1]
    lea                   r2,                 [r2 + 4 * r3]
%endrep
    PROCESS_LUMA_VERT_P_48x4_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_P_LUMA_48x64_AVX512 ps
    FILTER_VER_P_LUMA_48x64_AVX512 pp
%endif

%macro PROCESS_LUMA_VERT_P_64x2_AVX512 1
    PROCESS_LUMA_VERT_P_32x2_AVX512 %1
    movu                 m1,                  [r0 + mmsize]
    movu                 m3,                  [r0 + r1 + mmsize]
    punpcklwd            m0,                  m1,                     m3
    pmaddwd              m0,                  m15
    punpckhwd            m1,                  m3
    pmaddwd              m1,                  m15

    movu                 m4,                  [r0 + 2 * r1 + mmsize]
    punpcklwd            m2,                  m3,                     m4
    pmaddwd              m2,                  m15
    punpckhwd            m3,                  m4
    pmaddwd              m3,                  m15

    movu                 m5,                  [r0 + r7 + mmsize]
    punpcklwd            m6,                  m4,                     m5
    pmaddwd              m6,                  m16
    punpckhwd            m4,                  m5
    pmaddwd              m4,                  m16

    paddd                m0,                  m6
    paddd                m1,                  m4

    movu                 m4,                  [r0 + 4 * r1 + mmsize]
    punpcklwd            m6,                  m5,                     m4
    pmaddwd              m6,                  m16
    punpckhwd            m5,                  m4
    pmaddwd              m5,                  m16

    paddd                m2,                  m6
    paddd                m3,                  m5

    movu                 m11,                 [r6 + r1 + mmsize]
    punpcklwd            m8,                  m4,                     m11
    pmaddwd              m8,                  m17
    punpckhwd            m4,                  m11
    pmaddwd              m4,                  m17

    movu                 m12,                 [r6 + 2 * r1 + mmsize]
    punpcklwd            m10,                 m11,                    m12
    pmaddwd              m10,                 m17
    punpckhwd            m11,                 m12
    pmaddwd              m11,                 m17

    movu                 m13,                 [r6 + r7 + mmsize]
    punpcklwd            m14,                 m12,                    m13
    pmaddwd              m14,                 m18
    punpckhwd            m12,                 m13
    pmaddwd              m12,                 m18

    paddd                m8,                  m14
    paddd                m4,                  m12
    paddd                m0,                  m8
    paddd                m1,                  m4

    movu                 m12,                 [r6 + 4 * r1 + mmsize]
    punpcklwd            m14,                 m13,                    m12
    pmaddwd              m14,                 m18
    punpckhwd            m13,                 m12
    pmaddwd              m13,                 m18

    paddd                m10,                 m14
    paddd                m11,                 m13
    paddd                m2,                  m10
    paddd                m3,                  m11

    paddd                m0,                  m19
    paddd                m1,                  m19
    paddd                m2,                  m19
    paddd                m3,                  m19

%ifidn %1, pp
    psrad                m0,                  INTERP_SHIFT_PP
    psrad                m1,                  INTERP_SHIFT_PP
    psrad                m2,                  INTERP_SHIFT_PP
    psrad                m3,                  INTERP_SHIFT_PP

    packssdw             m0,                  m1
    packssdw             m2,                  m3
    CLIPW2               m0,                  m2,                   m20,                 m21
%else
    psrad                m0,                  INTERP_SHIFT_PS
    psrad                m1,                  INTERP_SHIFT_PS
    psrad                m2,                  INTERP_SHIFT_PS
    psrad                m3,                  INTERP_SHIFT_PS

    packssdw             m0,                  m1
    packssdw             m2,                  m3
%endif

    movu                 [r2 + mmsize],       m0
    movu                 [r2 + r3 + mmsize],  m2
%endmacro
;-----------------------------------------------------------------------------------------------------------------
; void interp_8tap_vert(int16_t *src, intptr_t srcStride, int16_t *dst, intptr_t dstStride, int coeffIdx)
;-----------------------------------------------------------------------------------------------------------------
%macro FILTER_VER_P_LUMA_64xN_AVX512 2
INIT_ZMM avx512
cglobal interp_8tap_vert_%1_64x%2, 5, 8, 22
    add                   r1d,                r1d
    add                   r3d,                r3d
    shl                   r4d,                8
%ifdef PIC
    lea                   r5,                 [tab_LumaCoeffVer_avx512]
    mova                  m15,                [r5 + r4]
    mova                  m16,                [r5 + r4 + 1 * mmsize]
    mova                  m17,                [r5 + r4 + 2 * mmsize]
    mova                  m18,                [r5 + r4 + 3 * mmsize]
%else
    lea                   r5,                 [tab_LumaCoeffVer_avx512 + r4]
    mova                  m15,                [r5]
    mova                  m16,                [r5 + 1 * mmsize]
    mova                  m17,                [r5 + 2 * mmsize]
    mova                  m18,                [r5 + 3 * mmsize]
%endif
%ifidn %1, pp
    vbroadcasti32x4       m19,                [INTERP_OFFSET_PP]
    pxor                  m20,                m20
    vbroadcasti32x8       m21,                [pw_pixel_max]
%else
    vbroadcasti32x4       m19,                [INTERP_OFFSET_PS]
%endif
    lea                   r7,                 [3 * r1]
    sub                   r0,                 r7

%rep %2/2 - 1
    PROCESS_LUMA_VERT_P_64x2_AVX512 %1
    lea                   r0,                 [r0 + 2 * r1]
    lea                   r2,                 [r2 + 2 * r3]
%endrep
    PROCESS_LUMA_VERT_P_64x2_AVX512 %1
    RET
%endmacro

%if ARCH_X86_64
    FILTER_VER_P_LUMA_64xN_AVX512 ps, 16
    FILTER_VER_P_LUMA_64xN_AVX512 ps, 32
    FILTER_VER_P_LUMA_64xN_AVX512 ps, 48
    FILTER_VER_P_LUMA_64xN_AVX512 ps, 64
    FILTER_VER_P_LUMA_64xN_AVX512 pp, 16
    FILTER_VER_P_LUMA_64xN_AVX512 pp, 32
    FILTER_VER_P_LUMA_64xN_AVX512 pp, 48
    FILTER_VER_P_LUMA_64xN_AVX512 pp, 64
%endif
;-------------------------------------------------------------------------------------------------------------
;avx512 luma_vpp and luma_vps code end
;-------------------------------------------------------------------------------------------------------------
;-------------------------------------------------------------------------------------------------------------
;ipfilter_luma_avx512 code end
;-------------------------------------------------------------------------------------------------------------
