;*****************************************************************************
;* pixel.asm: x86 pixel metrics
;*****************************************************************************
;* Copyright (C) 2003-2013 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Holger Lubitz <holger@lubitz.org>
;*          Laurent Aimar <fenrir@via.ecp.fr>
;*          Alex Izvorski <aizvorksi@gmail.com>
;*          Fiona Glaser <fiona@x264.com>
;*          Oskar Arvidsson <oskar@irock.se>
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
;*****************************************************************************

%include "x86inc.asm"
%include "x86util.asm"

SECTION_RODATA 32
hmul_16p:  times 16 db 1
           times 8 db 1, -1
hmul_8p:   times 8 db 1
           times 4 db 1, -1
           times 8 db 1
           times 4 db 1, -1
hmul_4p:   times 2 db 1, 1, 1, 1, 1, -1, 1, -1
mask_10:   times 4 dw 0, -1
mask_1100: times 2 dd 0, -1

ALIGN 32
transd_shuf1: SHUFFLE_MASK_W 0, 8, 2, 10, 4, 12, 6, 14
transd_shuf2: SHUFFLE_MASK_W 1, 9, 3, 11, 5, 13, 7, 15

sw_f0:     dq 0xfff0, 0
pd_f0:     times 4 dd 0xffff0000

pw_76543210: dw 0, 1, 2, 3, 4, 5, 6, 7

SECTION .text

cextern pb_0
cextern pb_1
cextern pw_1
cextern pw_8
cextern pw_16
cextern pw_32
cextern pw_00ff
cextern pw_ppppmmmm
cextern pw_ppmmppmm
cextern pw_pmpmpmpm
cextern pw_pmmpzzzz
cextern pd_1
cextern popcnt_table

;=============================================================================
; SATD
;=============================================================================

%macro JDUP 2
%if cpuflag(sse4)
    ; just use shufps on anything post conroe
    shufps %1, %2, 0
%elif cpuflag(ssse3) && notcpuflag(atom)
    ; join 2x 32 bit and duplicate them
    ; emulating shufps is faster on conroe
    punpcklqdq %1, %2
    movsldup %1, %1
%else
    ; doesn't need to dup. sse2 does things by zero extending to words and full h_2d
    punpckldq %1, %2
%endif
%endmacro

%macro HSUMSUB 5
    pmaddubsw m%2, m%5
    pmaddubsw m%1, m%5
    pmaddubsw m%4, m%5
    pmaddubsw m%3, m%5
%endmacro

%macro DIFF_UNPACK_SSE2 5
    punpcklbw m%1, m%5
    punpcklbw m%2, m%5
    punpcklbw m%3, m%5
    punpcklbw m%4, m%5
    psubw m%1, m%2
    psubw m%3, m%4
%endmacro

%macro DIFF_SUMSUB_SSSE3 5
    HSUMSUB %1, %2, %3, %4, %5
    psubw m%1, m%2
    psubw m%3, m%4
%endmacro

%macro LOAD_DUP_2x4P 4 ; dst, tmp, 2* pointer
    movd %1, %3
    movd %2, %4
    JDUP %1, %2
%endmacro

%macro LOAD_DUP_4x8P_CONROE 8 ; 4*dst, 4*pointer
    movddup m%3, %6
    movddup m%4, %8
    movddup m%1, %5
    movddup m%2, %7
%endmacro

%macro LOAD_DUP_4x8P_PENRYN 8
    ; penryn and nehalem run punpcklqdq and movddup in different units
    movh m%3, %6
    movh m%4, %8
    punpcklqdq m%3, m%3
    movddup m%1, %5
    punpcklqdq m%4, m%4
    movddup m%2, %7
%endmacro

%macro LOAD_SUMSUB_8x2P 9
    LOAD_DUP_4x8P %1, %2, %3, %4, %6, %7, %8, %9
    DIFF_SUMSUB_SSSE3 %1, %3, %2, %4, %5
%endmacro

%macro LOAD_SUMSUB_8x4P_SSSE3 7-11 r0, r2, 0, 0
; 4x dest, 2x tmp, 1x mul, [2* ptr], [increment?]
    LOAD_SUMSUB_8x2P %1, %2, %5, %6, %7, [%8], [%9], [%8+r1], [%9+r3]
    LOAD_SUMSUB_8x2P %3, %4, %5, %6, %7, [%8+2*r1], [%9+2*r3], [%8+r4], [%9+r5]
%if %10
    lea %8, [%8+4*r1]
    lea %9, [%9+4*r3]
%endif
%endmacro

%macro LOAD_SUMSUB_16P_SSSE3 7 ; 2*dst, 2*tmp, mul, 2*ptr
    movddup m%1, [%7]
    movddup m%2, [%7+8]
    mova m%4, [%6]
    movddup m%3, m%4
    punpckhqdq m%4, m%4
    DIFF_SUMSUB_SSSE3 %1, %3, %2, %4, %5
%endmacro

%macro LOAD_SUMSUB_16P_SSE2 7 ; 2*dst, 2*tmp, mask, 2*ptr
    movu  m%4, [%7]
    mova  m%2, [%6]
    DEINTB %1, %2, %3, %4, %5
    psubw m%1, m%3
    psubw m%2, m%4
    SUMSUB_BA w, %1, %2, %3
%endmacro

%macro LOAD_SUMSUB_16x4P 10-13 r0, r2, none
; 8x dest, 1x tmp, 1x mul, [2* ptr] [2nd tmp]
    LOAD_SUMSUB_16P %1, %5, %2, %3, %10, %11, %12
    LOAD_SUMSUB_16P %2, %6, %3, %4, %10, %11+r1, %12+r3
    LOAD_SUMSUB_16P %3, %7, %4, %9, %10, %11+2*r1, %12+2*r3
    LOAD_SUMSUB_16P %4, %8, %13, %9, %10, %11+r4, %12+r5
%endmacro

%macro LOAD_SUMSUB_16x2P_AVX2 9
; 2*dst, 2*tmp, mul, 4*ptr
    vbroadcasti128 m%1, [%6]
    vbroadcasti128 m%3, [%7]
    vbroadcasti128 m%2, [%8]
    vbroadcasti128 m%4, [%9]
    DIFF_SUMSUB_SSSE3 %1, %3, %2, %4, %5
%endmacro

%macro LOAD_SUMSUB_16x4P_AVX2 7-11 r0, r2, 0, 0
; 4x dest, 2x tmp, 1x mul, [2* ptr], [increment?]
    LOAD_SUMSUB_16x2P_AVX2 %1, %2, %5, %6, %7, %8, %9, %8+r1, %9+r3
    LOAD_SUMSUB_16x2P_AVX2 %3, %4, %5, %6, %7, %8+2*r1, %9+2*r3, %8+r4, %9+r5
%if %10
    lea  %8, [%8+4*r1]
    lea  %9, [%9+4*r3]
%endif
%endmacro

%macro LOAD_DUP_4x16P_AVX2 8 ; 4*dst, 4*pointer
    mova  xm%3, %6
    mova  xm%4, %8
    mova  xm%1, %5
    mova  xm%2, %7
    vpermq m%3, m%3, q0011
    vpermq m%4, m%4, q0011
    vpermq m%1, m%1, q0011
    vpermq m%2, m%2, q0011
%endmacro

%macro LOAD_SUMSUB8_16x2P_AVX2 9
; 2*dst, 2*tmp, mul, 4*ptr
    LOAD_DUP_4x16P_AVX2 %1, %2, %3, %4, %6, %7, %8, %9
    DIFF_SUMSUB_SSSE3 %1, %3, %2, %4, %5
%endmacro

%macro LOAD_SUMSUB8_16x4P_AVX2 7-11 r0, r2, 0, 0
; 4x dest, 2x tmp, 1x mul, [2* ptr], [increment?]
    LOAD_SUMSUB8_16x2P_AVX2 %1, %2, %5, %6, %7, [%8], [%9], [%8+r1], [%9+r3]
    LOAD_SUMSUB8_16x2P_AVX2 %3, %4, %5, %6, %7, [%8+2*r1], [%9+2*r3], [%8+r4], [%9+r5]
%if %10
    lea  %8, [%8+4*r1]
    lea  %9, [%9+4*r3]
%endif
%endmacro

; in: r4=3*stride1, r5=3*stride2
; in: %2 = horizontal offset
; in: %3 = whether we need to increment pix1 and pix2
; clobber: m3..m7
; out: %1 = satd
%macro SATD_4x4_MMX 3
    %xdefine %%n n%1
    %assign offset %2*SIZEOF_PIXEL
    LOAD_DIFF m4, m3, none, [r0+     offset], [r2+     offset]
    LOAD_DIFF m5, m3, none, [r0+  r1+offset], [r2+  r3+offset]
    LOAD_DIFF m6, m3, none, [r0+2*r1+offset], [r2+2*r3+offset]
    LOAD_DIFF m7, m3, none, [r0+  r4+offset], [r2+  r5+offset]
%if %3
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endif
    HADAMARD4_2D 4, 5, 6, 7, 3, %%n
    paddw m4, m6
    SWAP %%n, 4
%endmacro

; in: %1 = horizontal if 0, vertical if 1
%macro SATD_8x4_SSE 8-9
%if %1
    HADAMARD4_2D_SSE %2, %3, %4, %5, %6, amax
%else
    HADAMARD4_V %2, %3, %4, %5, %6
    ; doing the abs first is a slight advantage
    ABSW2 m%2, m%4, m%2, m%4, m%6, m%7
    ABSW2 m%3, m%5, m%3, m%5, m%6, m%7
    HADAMARD 1, max, %2, %4, %6, %7
%endif
%ifnidn %9, swap
    paddw m%8, m%2
%else
    SWAP %8, %2
%endif
%if %1
    paddw m%8, m%4
%else
    HADAMARD 1, max, %3, %5, %6, %7
    paddw m%8, m%3
%endif
%endmacro

%macro SATD_8x4_1_SSE 10
%if %1
    HADAMARD4_2D_SSE %2, %3, %4, %5, %6, amax
%else
    HADAMARD4_V %2, %3, %4, %5, %6
    ; doing the abs first is a slight advantage
    ABSW2 m%2, m%4, m%2, m%4, m%6, m%7
    ABSW2 m%3, m%5, m%3, m%5, m%6, m%7
    HADAMARD 1, max, %2, %4, %6, %7
%endif

    pxor m%10, m%10
    mova m%9, m%2
    punpcklwd m%9, m%10
    paddd m%8, m%9
    mova m%9, m%2
    punpckhwd m%9, m%10
    paddd m%8, m%9

%if %1
    pxor m%10, m%10
    mova m%9, m%4
    punpcklwd m%9, m%10
    paddd m%8, m%9
    mova m%9, m%4
    punpckhwd m%9, m%10
    paddd m%8, m%9
%else
    HADAMARD 1, max, %3, %5, %6, %7
    pxor m%10, m%10
    mova m%9, m%3
    punpcklwd m%9, m%10
    paddd m%8, m%9
    mova m%9, m%3
    punpckhwd m%9, m%10
    paddd m%8, m%9
%endif
%endmacro

%macro SATD_START_MMX 0
    FIX_STRIDES r1, r3
    lea  r4, [3*r1] ; 3*stride1
    lea  r5, [3*r3] ; 3*stride2
%endmacro

%macro SATD_END_MMX 0
%if HIGH_BIT_DEPTH
    HADDUW      m0, m1
    movd       eax, m0
%else ; !HIGH_BIT_DEPTH
    pshufw      m1, m0, q1032
    paddw       m0, m1
    pshufw      m1, m0, q2301
    paddw       m0, m1
    movd       eax, m0
    and        eax, 0xffff
%endif ; HIGH_BIT_DEPTH
    RET
%endmacro

; FIXME avoid the spilling of regs to hold 3*stride.
; for small blocks on x86_32, modify pixel pointer instead.

;-----------------------------------------------------------------------------
; int pixel_satd_16x16( uint8_t *, intptr_t, uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
INIT_MMX mmx2
cglobal pixel_satd_16x4_internal
    SATD_4x4_MMX m2,  0, 0
    SATD_4x4_MMX m1,  4, 0
    paddw        m0, m2
    SATD_4x4_MMX m2,  8, 0
    paddw        m0, m1
    SATD_4x4_MMX m1, 12, 0
    paddw        m0, m2
    paddw        m0, m1
    ret

cglobal pixel_satd_8x8_internal
    SATD_4x4_MMX m2,  0, 0
    SATD_4x4_MMX m1,  4, 1
    paddw        m0, m2
    paddw        m0, m1
pixel_satd_8x4_internal_mmx2:
    SATD_4x4_MMX m2,  0, 0
    SATD_4x4_MMX m1,  4, 0
    paddw        m0, m2
    paddw        m0, m1
    ret

%if HIGH_BIT_DEPTH
%macro SATD_MxN_MMX 3
cglobal pixel_satd_%1x%2, 4,7
    SATD_START_MMX
    pxor   m0, m0
    call pixel_satd_%1x%3_internal_mmx2
    HADDUW m0, m1
    movd  r6d, m0
%rep %2/%3-1
    pxor   m0, m0
    lea    r0, [r0+4*r1]
    lea    r2, [r2+4*r3]
    call pixel_satd_%1x%3_internal_mmx2
    movd   m2, r4
    HADDUW m0, m1
    movd   r4, m0
    add    r6, r4
    movd   r4, m2
%endrep
    movifnidn eax, r6d
    RET
%endmacro

SATD_MxN_MMX 16, 16, 4
SATD_MxN_MMX 16,  8, 4
SATD_MxN_MMX  8, 16, 8
%endif ; HIGH_BIT_DEPTH

%if HIGH_BIT_DEPTH == 0
cglobal pixel_satd_16x16, 4,6
    SATD_START_MMX
    pxor   m0, m0
%rep 3
    call pixel_satd_16x4_internal_mmx2
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endrep
    call pixel_satd_16x4_internal_mmx2
    HADDUW m0, m1
    movd  eax, m0
    RET

cglobal pixel_satd_16x8, 4,6
    SATD_START_MMX
    pxor   m0, m0
    call pixel_satd_16x4_internal_mmx2
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
    call pixel_satd_16x4_internal_mmx2
    SATD_END_MMX

cglobal pixel_satd_8x16, 4,6
    SATD_START_MMX
    pxor   m0, m0
    call pixel_satd_8x8_internal_mmx2
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
    call pixel_satd_8x8_internal_mmx2
    SATD_END_MMX
%endif ; !HIGH_BIT_DEPTH

cglobal pixel_satd_8x8, 4,6
    SATD_START_MMX
    pxor   m0, m0
    call pixel_satd_8x8_internal_mmx2
    SATD_END_MMX

cglobal pixel_satd_8x4, 4,6
    SATD_START_MMX
    pxor   m0, m0
    call pixel_satd_8x4_internal_mmx2
    SATD_END_MMX

cglobal pixel_satd_4x16, 4,6
    SATD_START_MMX
    SATD_4x4_MMX m0, 0, 1
    SATD_4x4_MMX m1, 0, 1
    paddw  m0, m1
    SATD_4x4_MMX m1, 0, 1
    paddw  m0, m1
    SATD_4x4_MMX m1, 0, 0
    paddw  m0, m1
    SATD_END_MMX

cglobal pixel_satd_4x8, 4,6
    SATD_START_MMX
    SATD_4x4_MMX m0, 0, 1
    SATD_4x4_MMX m1, 0, 0
    paddw  m0, m1
    SATD_END_MMX

cglobal pixel_satd_4x4, 4,6
    SATD_START_MMX
    SATD_4x4_MMX m0, 0, 0
    SATD_END_MMX

%macro SATD_START_SSE2 2-3 0
    FIX_STRIDES r1, r3
%if HIGH_BIT_DEPTH && %3
    pxor    %2, %2
%elif cpuflag(ssse3) && notcpuflag(atom)
%if mmsize==32
    mova    %2, [hmul_16p]
%else
    mova    %2, [hmul_8p]
%endif
%endif
    lea     r4, [3*r1]
    lea     r5, [3*r3]
    pxor    %1, %1
%endmacro

%macro SATD_END_SSE2 1-2
%if HIGH_BIT_DEPTH
    HADDUW  %1, xm0
%if %0 == 2
    paddd   %1, %2
%endif
%else
    HADDW   %1, xm7
%endif
    movd   eax, %1
    RET
%endmacro

%macro SATD_ACCUM 3
%if HIGH_BIT_DEPTH
    HADDUW %1, %2
    paddd  %3, %1
    pxor   %1, %1
%endif
%endmacro

%macro BACKUP_POINTERS 0
%if ARCH_X86_64
%if WIN64
    PUSH r7
%endif
    mov     r6, r0
    mov     r7, r2
%endif
%endmacro

%macro RESTORE_AND_INC_POINTERS 0
%if ARCH_X86_64
    lea     r0, [r6+8*SIZEOF_PIXEL]
    lea     r2, [r7+8*SIZEOF_PIXEL]
%if WIN64
    POP r7
%endif
%else
    mov     r0, r0mp
    mov     r2, r2mp
    add     r0, 8*SIZEOF_PIXEL
    add     r2, 8*SIZEOF_PIXEL
%endif
%endmacro

%macro SATD_4x8_SSE 3-4
%if HIGH_BIT_DEPTH
    movh    m0, [r0+0*r1]
    movh    m4, [r2+0*r3]
    movh    m1, [r0+1*r1]
    movh    m5, [r2+1*r3]
    movhps  m0, [r0+4*r1]
    movhps  m4, [r2+4*r3]
    movh    m2, [r0+2*r1]
    movh    m6, [r2+2*r3]
    psubw   m0, m4
    movh    m3, [r0+r4]
    movh    m4, [r2+r5]
    lea     r0, [r0+4*r1]
    lea     r2, [r2+4*r3]
    movhps  m1, [r0+1*r1]
    movhps  m5, [r2+1*r3]
    movhps  m2, [r0+2*r1]
    movhps  m6, [r2+2*r3]
    psubw   m1, m5
    movhps  m3, [r0+r4]
    movhps  m4, [r2+r5]
    psubw   m2, m6
    psubw   m3, m4
%else ; !HIGH_BIT_DEPTH
    movd m4, [r2]
    movd m5, [r2+r3]
    movd m6, [r2+2*r3]
    add r2, r5
    movd m0, [r0]
    movd m1, [r0+r1]
    movd m2, [r0+2*r1]
    add r0, r4
    movd m3, [r2+r3]
    JDUP m4, m3
    movd m3, [r0+r1]
    JDUP m0, m3
    movd m3, [r2+2*r3]
    JDUP m5, m3
    movd m3, [r0+2*r1]
    JDUP m1, m3
%if %1==0 && %2==1
    mova m3, [hmul_4p]
    DIFFOP 0, 4, 1, 5, 3
%else
    DIFFOP 0, 4, 1, 5, 7
%endif
    movd m5, [r2]
    add r2, r5
    movd m3, [r0]
    add r0, r4
    movd m4, [r2]
    JDUP m6, m4
    movd m4, [r0]
    JDUP m2, m4
    movd m4, [r2+r3]
    JDUP m5, m4
    movd m4, [r0+r1]
    JDUP m3, m4
%if %1==0 && %2==1
    mova m4, [hmul_4p]
    DIFFOP 2, 6, 3, 5, 4
%else
    DIFFOP 2, 6, 3, 5, 7
%endif
%endif ; HIGH_BIT_DEPTH
%if %0 == 4
    SATD_8x4_1_SSE %1, 0, 1, 2, 3, 4, 5, 7, %3, %4
%else
    SATD_8x4_SSE %1, 0, 1, 2, 3, 4, 5, 7, %3
%endif
%endmacro

;-----------------------------------------------------------------------------
; int pixel_satd_8x4( uint8_t *, intptr_t, uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
%macro SATDS_SSE2 0
%define vertical ((notcpuflag(ssse3) || cpuflag(atom)) || HIGH_BIT_DEPTH)

%if cpuflag(ssse3) && (vertical==0 || HIGH_BIT_DEPTH)
cglobal pixel_satd_4x4, 4, 6, 6
    SATD_START_MMX
    mova m4, [hmul_4p]
    LOAD_DUP_2x4P m2, m5, [r2], [r2+r3]
    LOAD_DUP_2x4P m3, m5, [r2+2*r3], [r2+r5]
    LOAD_DUP_2x4P m0, m5, [r0], [r0+r1]
    LOAD_DUP_2x4P m1, m5, [r0+2*r1], [r0+r4]
    DIFF_SUMSUB_SSSE3 0, 2, 1, 3, 4
    HADAMARD 0, sumsub, 0, 1, 2, 3
    HADAMARD 4, sumsub, 0, 1, 2, 3
    HADAMARD 1, amax, 0, 1, 2, 3
    HADDW m0, m1
    movd eax, m0
    RET
%endif

cglobal pixel_satd_4x8, 4, 6, 8
    SATD_START_MMX
%if vertical==0
    mova m7, [hmul_4p]
%endif
    SATD_4x8_SSE vertical, 0, swap
    HADDW m7, m1
    movd eax, m7
    RET

cglobal pixel_satd_4x16, 4, 6, 8
    SATD_START_MMX
%if vertical==0
    mova m7, [hmul_4p]
%endif
    SATD_4x8_SSE vertical, 0, swap
    lea r0, [r0+r1*2*SIZEOF_PIXEL]
    lea r2, [r2+r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    HADDW m7, m1
    movd eax, m7
    RET

cglobal pixel_satd_8x8_internal
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1, 0
    SATD_8x4_SSE vertical, 0, 1, 2, 3, 4, 5, 6
%%pixel_satd_8x4_internal:
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1, 0
    SATD_8x4_SSE vertical, 0, 1, 2, 3, 4, 5, 6
    ret

cglobal pixel_satd_8x8_internal2
%if WIN64
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1, 0
    SATD_8x4_1_SSE vertical, 0, 1, 2, 3, 4, 5, 6, 12, 13
%%pixel_satd_8x4_internal2:
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1, 0
    SATD_8x4_1_SSE vertical, 0, 1, 2, 3, 4, 5, 6, 12, 13
%else
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1, 0
    SATD_8x4_1_SSE vertical, 0, 1, 2, 3, 4, 5, 6, 4, 5
%%pixel_satd_8x4_internal2:
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1, 0
    SATD_8x4_1_SSE vertical, 0, 1, 2, 3, 4, 5, 6, 4, 5
%endif
    ret

; 16x8 regresses on phenom win64, 16x16 is almost the same (too many spilled registers)
; These aren't any faster on AVX systems with fast movddup (Bulldozer, Sandy Bridge)
%if HIGH_BIT_DEPTH == 0 && (WIN64 || UNIX64) && notcpuflag(avx)

cglobal pixel_satd_16x4_internal2
    LOAD_SUMSUB_16x4P 0, 1, 2, 3, 4, 8, 5, 9, 6, 7, r0, r2, 11
    lea  r2, [r2+4*r3]
    lea  r0, [r0+4*r1]
    SATD_8x4_1_SSE 0, 0, 1, 2, 3, 6, 11, 10, 12, 13
    SATD_8x4_1_SSE 0, 4, 8, 5, 9, 6, 3, 10, 12, 13
    ret

cglobal pixel_satd_16x4, 4,6,14
    SATD_START_SSE2 m10, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    pxor     m9, m9
    movhlps  m9, m10
    paddd   m10, m9
    pshufd   m9, m10, 1
    paddd   m10, m9
    movd    eax, m10
    RET

cglobal pixel_satd_16x8, 4,6,14
    SATD_START_SSE2 m10, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    jmp %%pixel_satd_16x8_internal

cglobal pixel_satd_16x12, 4,6,14
    SATD_START_SSE2 m10, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    jmp %%pixel_satd_16x8_internal

cglobal pixel_satd_16x32, 4,6,14
    SATD_START_SSE2 m10, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    jmp %%pixel_satd_16x8_internal

cglobal pixel_satd_16x64, 4,6,14
    SATD_START_SSE2 m10, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    jmp %%pixel_satd_16x8_internal

cglobal pixel_satd_16x16, 4,6,14
    SATD_START_SSE2 m10, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
%%pixel_satd_16x8_internal:
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    pxor     m9, m9
    movhlps  m9, m10
    paddd   m10, m9
    pshufd   m9, m10, 1
    paddd   m10, m9
    movd    eax, m10
    RET

cglobal pixel_satd_32x8, 4,8,14    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    pxor     m9, m9
    movhlps  m9, m10
    paddd   m10, m9
    pshufd   m9, m10, 1
    paddd   m10, m9
    movd    eax, m10
    RET

cglobal pixel_satd_32x16, 4,8,14    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    pxor     m9, m9
    movhlps  m9, m10
    paddd   m10, m9
    pshufd   m9, m10, 1
    paddd   m10, m9
    movd    eax, m10
    RET

cglobal pixel_satd_32x24, 4,8,14    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    pxor     m9, m9
    movhlps  m9, m10
    paddd   m10, m9
    pshufd   m9, m10, 1
    paddd   m10, m9
    movd    eax, m10
    RET

cglobal pixel_satd_32x32, 4,8,14    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    pxor     m9, m9
    movhlps  m9, m10
    paddd   m10, m9
    pshufd   m9, m10, 1
    paddd   m10, m9
    movd    eax, m10
    RET

cglobal pixel_satd_32x64, 4,8,14    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    pxor     m9, m9
    movhlps  m9, m10
    paddd   m10, m9
    pshufd   m9, m10, 1
    paddd   m10, m9
    movd    eax, m10
    RET

cglobal pixel_satd_48x64, 4,8,14    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 32]
    lea r2, [r7 + 32]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    pxor     m9, m9
    movhlps  m9, m10
    paddd   m10, m9
    pshufd   m9, m10, 1
    paddd   m10, m9
    movd    eax, m10
    RET

cglobal pixel_satd_64x16, 4,8,14    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 32]
    lea r2, [r7 + 32]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 48]
    lea r2, [r7 + 48]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    pxor     m9, m9
    movhlps  m9, m10
    paddd   m10, m9
    pshufd   m9, m10, 1
    paddd   m10, m9
    movd    eax, m10
    RET

cglobal pixel_satd_64x32, 4,8,14    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 32]
    lea r2, [r7 + 32]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 48]
    lea r2, [r7 + 48]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2

    pxor     m9, m9
    movhlps  m9, m10
    paddd   m10, m9
    pshufd   m9, m10, 1
    paddd   m10, m9
    movd    eax, m10
    RET

cglobal pixel_satd_64x48, 4,8,14    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 32]
    lea r2, [r7 + 32]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 48]
    lea r2, [r7 + 48]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2

    pxor     m9, m9
    movhlps  m9, m10
    paddd   m10, m9
    pshufd   m9, m10, 1
    paddd   m10, m9
    movd    eax, m10
    RET

cglobal pixel_satd_64x64, 4,8,14    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 32]
    lea r2, [r7 + 32]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    lea r0, [r6 + 48]
    lea r2, [r7 + 48]
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2
    call pixel_satd_16x4_internal2

    pxor     m9, m9
    movhlps  m9, m10
    paddd   m10, m9
    pshufd   m9, m10, 1
    paddd   m10, m9
    movd    eax, m10
    RET

%else

%if WIN64
cglobal pixel_satd_32x8, 4,8,14    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    lea r2, [r7 + 16*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    lea r2, [r7 + 24*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%else
cglobal pixel_satd_32x8, 4,7,8,0-gprsize    ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 8*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 16*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 24*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%endif

%if WIN64
cglobal pixel_satd_32x16, 4,8,14    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    lea r2, [r7 + 16*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    lea r2, [r7 + 24*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%else
cglobal pixel_satd_32x16, 4,7,8,0-gprsize   ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 8*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 16*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 24*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%endif

%if WIN64
cglobal pixel_satd_32x24, 4,8,14    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    lea r2, [r7 + 16*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    lea r2, [r7 + 24*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%else
cglobal pixel_satd_32x24, 4,7,8,0-gprsize   ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 8*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 16*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 24*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%endif

%if WIN64
cglobal pixel_satd_32x32, 4,8,14    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    lea r2, [r7 + 16*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    lea r2, [r7 + 24*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%else
cglobal pixel_satd_32x32, 4,7,8,0-gprsize   ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 8*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 16*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 24*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%endif

%if WIN64
cglobal pixel_satd_32x64, 4,8,14    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    lea r2, [r7 + 16*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    lea r2, [r7 + 24*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%else
cglobal pixel_satd_32x64, 4,7,8,0-gprsize   ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 8*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 16*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 24*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%endif

%if WIN64
cglobal pixel_satd_48x64, 4,8,14    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    lea r2, [r7 + 16*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    lea r2, [r7 + 24*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32*SIZEOF_PIXEL]
    lea r2, [r7 + 32*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40*SIZEOF_PIXEL]
    lea r2, [r7 + 40*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%else
cglobal pixel_satd_48x64, 4,7,8,0-gprsize   ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2,8*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2,16*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2,24*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2,32*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2,40*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%endif


%if WIN64
cglobal pixel_satd_64x16, 4,8,14    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    lea r2, [r7 + 16*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    lea r2, [r7 + 24*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32*SIZEOF_PIXEL]
    lea r2, [r7 + 32*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40*SIZEOF_PIXEL]
    lea r2, [r7 + 40*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48*SIZEOF_PIXEL]
    lea r2, [r7 + 48*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56*SIZEOF_PIXEL]
    lea r2, [r7 + 56*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%else
cglobal pixel_satd_64x16, 4,7,8,0-gprsize   ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2,8*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2,16*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2,24*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2,32*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2,40*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2,48*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2,56*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%endif

%if WIN64
cglobal pixel_satd_64x32, 4,8,14    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    lea r2, [r7 + 16*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    lea r2, [r7 + 24*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32*SIZEOF_PIXEL]
    lea r2, [r7 + 32*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40*SIZEOF_PIXEL]
    lea r2, [r7 + 40*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48*SIZEOF_PIXEL]
    lea r2, [r7 + 48*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56*SIZEOF_PIXEL]
    lea r2, [r7 + 56*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%else
cglobal pixel_satd_64x32, 4,7,8,0-gprsize   ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 8*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 16*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 24*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 32*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 40*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 48*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 56*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%endif

%if WIN64
cglobal pixel_satd_64x48, 4,8,14    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    lea r2, [r7 + 16*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    lea r2, [r7 + 24*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32*SIZEOF_PIXEL]
    lea r2, [r7 + 32*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40*SIZEOF_PIXEL]
    lea r2, [r7 + 40*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48*SIZEOF_PIXEL]
    lea r2, [r7 + 48*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56*SIZEOF_PIXEL]
    lea r2, [r7 + 56*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m8, m8
    movhlps m8, m6
    paddd   m6, m8
    pshufd  m8, m6, 1
    paddd   m6, m8
    movd   eax, m6
    RET
%else
cglobal pixel_satd_64x48, 4,7,8,0-gprsize   ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 8*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 16*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 24*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 32*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 40*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 48*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 56*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%endif

%if WIN64
cglobal pixel_satd_64x64, 4,8,14    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    lea r2, [r7 + 16*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    lea r2, [r7 + 24*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32*SIZEOF_PIXEL]
    lea r2, [r7 + 32*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40*SIZEOF_PIXEL]
    lea r2, [r7 + 40*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48*SIZEOF_PIXEL]
    lea r2, [r7 + 48*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56*SIZEOF_PIXEL]
    lea r2, [r7 + 56*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m8, m8
    movhlps m8, m6
    paddd   m6, m8
    pshufd  m8, m6, 1
    paddd   m6, m8
    movd   eax, m6
    RET
%else
cglobal pixel_satd_64x64, 4,7,8,0-gprsize   ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 8*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 16*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 24*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 32*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 40*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 48*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 56*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%endif

%if WIN64
cglobal pixel_satd_16x4, 4,6,14
%else
cglobal pixel_satd_16x4, 4,6,8
%endif
    SATD_START_SSE2 m6, m7
    BACKUP_POINTERS
    call %%pixel_satd_8x4_internal2
    RESTORE_AND_INC_POINTERS
    call %%pixel_satd_8x4_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET

%if WIN64
cglobal pixel_satd_16x8, 4,6,14
%else
cglobal pixel_satd_16x8, 4,6,8
%endif
    SATD_START_SSE2 m6, m7
    BACKUP_POINTERS
    call pixel_satd_8x8_internal2
    RESTORE_AND_INC_POINTERS
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET

%if WIN64
cglobal pixel_satd_16x12, 4,6,14
%else
cglobal pixel_satd_16x12, 4,6,8
%endif
    SATD_START_SSE2 m6, m7, 1
    BACKUP_POINTERS
    call pixel_satd_8x8_internal2
    call %%pixel_satd_8x4_internal2
    RESTORE_AND_INC_POINTERS
    call pixel_satd_8x8_internal2
    call %%pixel_satd_8x4_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET

%if WIN64
cglobal pixel_satd_16x16, 4,6,14
%else
cglobal pixel_satd_16x16, 4,6,8
%endif
    SATD_START_SSE2 m6, m7, 1
    BACKUP_POINTERS
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    RESTORE_AND_INC_POINTERS
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET

%if WIN64
cglobal pixel_satd_16x32, 4,6,14
%else
cglobal pixel_satd_16x32, 4,6,8
%endif
    SATD_START_SSE2 m6, m7, 1
    BACKUP_POINTERS
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    RESTORE_AND_INC_POINTERS
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET

%if WIN64
cglobal pixel_satd_16x64, 4,6,14
%else
cglobal pixel_satd_16x64, 4,6,8
%endif
    SATD_START_SSE2 m6, m7, 1
    BACKUP_POINTERS
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    RESTORE_AND_INC_POINTERS
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%endif

%if HIGH_BIT_DEPTH
%if WIN64
cglobal pixel_satd_12x16, 4,8,8
    SATD_START_MMX
    mov r6, r0
    mov r7, r2
    pxor m7, m7
    SATD_4x8_SSE vertical, 0, 4, 5
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, 4, 5
    lea r0, [r6 + 4*SIZEOF_PIXEL]
    lea r2, [r7 + 4*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, 4, 5
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, 4, 5
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, 4, 5
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, 4, 5
    pxor    m1, m1
    movhlps m1, m7
    paddd   m7, m1
    pshufd  m1, m7, 1
    paddd   m7, m1
    movd   eax, m7
    RET
%else
cglobal pixel_satd_12x16, 4,7,8,0-gprsize
    SATD_START_MMX
    mov r6, r0
    mov [rsp], r2
    pxor m7, m7
    SATD_4x8_SSE vertical, 0, 4, 5
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, 4, 5
    lea r0, [r6 + 4*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 4*SIZEOF_PIXEL
    SATD_4x8_SSE vertical, 1, 4, 5
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, 4, 5
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 8*SIZEOF_PIXEL
    SATD_4x8_SSE vertical, 1, 4, 5
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, 4, 5
    pxor    m1, m1
    movhlps m1, m7
    paddd   m7, m1
    pshufd  m1, m7, 1
    paddd   m7, m1
    movd   eax, m7
    RET
%endif
%else    ;HIGH_BIT_DEPTH
%if WIN64
cglobal pixel_satd_12x16, 4,8,8
    SATD_START_MMX
    mov r6, r0
    mov r7, r2
%if vertical==0
    mova m7, [hmul_4p]
%endif
    SATD_4x8_SSE vertical, 0, swap
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r6 + 4*SIZEOF_PIXEL]
    lea r2, [r7 + 4*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    HADDW m7, m1
    movd eax, m7
    RET
%else
cglobal pixel_satd_12x16, 4,7,8,0-gprsize
    SATD_START_MMX
    mov r6, r0
    mov [rsp], r2
%if vertical==0
    mova m7, [hmul_4p]
%endif
    SATD_4x8_SSE vertical, 0, swap
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r6 + 4*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 4*SIZEOF_PIXEL
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 8*SIZEOF_PIXEL
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    HADDW m7, m1
    movd eax, m7
    RET
%endif
%endif

%if WIN64
cglobal pixel_satd_24x32, 4,8,14
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    lea r2, [r7 + 8*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    lea r2, [r7 + 16*SIZEOF_PIXEL]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%else
cglobal pixel_satd_24x32, 4,7,8,0-gprsize
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 8*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16*SIZEOF_PIXEL]
    mov r2, [rsp]
    add r2, 16*SIZEOF_PIXEL
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET
%endif    ;WIN64

%if WIN64
cglobal pixel_satd_8x32, 4,6,14
%else
cglobal pixel_satd_8x32, 4,6,8
%endif
    SATD_START_SSE2 m6, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET

%if WIN64
cglobal pixel_satd_8x16, 4,6,14
%else
cglobal pixel_satd_8x16, 4,6,8
%endif
    SATD_START_SSE2 m6, m7
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    pxor    m7, m7
    movhlps m7, m6
    paddd   m6, m7
    pshufd  m7, m6, 1
    paddd   m6, m7
    movd   eax, m6
    RET

cglobal pixel_satd_8x8, 4,6,8
    SATD_START_SSE2 m6, m7
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6

%if WIN64
cglobal pixel_satd_8x4, 4,6,14
%else
cglobal pixel_satd_8x4, 4,6,8
%endif
    SATD_START_SSE2 m6, m7
    call %%pixel_satd_8x4_internal2
    SATD_END_SSE2 m6
%endmacro ; SATDS_SSE2


;=============================================================================
; SA8D
;=============================================================================

%macro SA8D_INTER 0
%if ARCH_X86_64
    %define lh m10
    %define rh m0
%else
    %define lh m0
    %define rh [esp+48]
%endif
%if HIGH_BIT_DEPTH
    HADDUW  m0, m1
    paddd   lh, rh
%else
    paddusw lh, rh
%endif ; HIGH_BIT_DEPTH
%endmacro

%macro SA8D_8x8 0
    call pixel_sa8d_8x8_internal
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%else
    HADDW m0, m1
%endif ; HIGH_BIT_DEPTH
    paddd  m0, [pd_1]
    psrld  m0, 1
    paddd  m12, m0
%endmacro

%macro SA8D_16x16 0
    call pixel_sa8d_8x8_internal ; pix[0]
    add  r2, 8*SIZEOF_PIXEL
    add  r0, 8*SIZEOF_PIXEL
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova m10, m0
    call pixel_sa8d_8x8_internal ; pix[8]
    lea  r2, [r2+8*r3]
    lea  r0, [r0+8*r1]
    SA8D_INTER
    call pixel_sa8d_8x8_internal ; pix[8*stride+8]
    sub  r2, 8*SIZEOF_PIXEL
    sub  r0, 8*SIZEOF_PIXEL
    SA8D_INTER
    call pixel_sa8d_8x8_internal ; pix[8*stride]
    SA8D_INTER
    SWAP 0, 10
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    paddd  m0, [pd_1]
    psrld  m0, 1
    paddd  m12, m0
%endmacro

%macro AVG_16x16 0
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov dword [esp+36], r4d
%endmacro

%macro SA8D 0
; sse2 doesn't seem to like the horizontal way of doing things
%define vertical ((notcpuflag(ssse3) || cpuflag(atom)) || HIGH_BIT_DEPTH)

%if ARCH_X86_64
;-----------------------------------------------------------------------------
; int pixel_sa8d_8x8( uint8_t *, intptr_t, uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
cglobal pixel_sa8d_8x8_internal
    lea  r6, [r0+4*r1]
    lea  r7, [r2+4*r3]
    LOAD_SUMSUB_8x4P 0, 1, 2, 8, 5, 6, 7, r0, r2
    LOAD_SUMSUB_8x4P 4, 5, 3, 9, 11, 6, 7, r6, r7
%if vertical
    HADAMARD8_2D 0, 1, 2, 8, 4, 5, 3, 9, 6, amax
%else ; non-sse2
    HADAMARD8_2D_HMUL 0, 1, 2, 8, 4, 5, 3, 9, 6, 11
%endif
    paddw m0, m1
    paddw m0, m2
    paddw m0, m8
    SAVE_MM_PERMUTATION
    ret

cglobal pixel_sa8d_8x8, 4,8,12
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    call pixel_sa8d_8x8_internal
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%else
    HADDW m0, m1
%endif ; HIGH_BIT_DEPTH
    movd eax, m0
    add eax, 1
    shr eax, 1
    RET

cglobal pixel_sa8d_16x16, 4,8,12
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    call pixel_sa8d_8x8_internal ; pix[0]
    add  r2, 8*SIZEOF_PIXEL
    add  r0, 8*SIZEOF_PIXEL
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova m10, m0
    call pixel_sa8d_8x8_internal ; pix[8]
    lea  r2, [r2+8*r3]
    lea  r0, [r0+8*r1]
    SA8D_INTER
    call pixel_sa8d_8x8_internal ; pix[8*stride+8]
    sub  r2, 8*SIZEOF_PIXEL
    sub  r0, 8*SIZEOF_PIXEL
    SA8D_INTER
    call pixel_sa8d_8x8_internal ; pix[8*stride]
    SA8D_INTER
    SWAP 0, 10
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd eax, m0
    add  eax, 1
    shr  eax, 1
    RET

cglobal pixel_sa8d_8x16, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_8x8
    lea r0, [r0 + 8*r1]
    lea r2, [r2 + 8*r3]
    SA8D_8x8
    movd eax, m12
    RET

cglobal pixel_sa8d_8x32, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    movd eax, m12
    RET

cglobal pixel_sa8d_16x8, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    movd eax, m12
    RET

cglobal pixel_sa8d_16x32, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_16x64, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_24x32, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    sub r0, 8*SIZEOF_PIXEL
    sub r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    sub r0, 8*SIZEOF_PIXEL
    sub r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    sub r0, 8*SIZEOF_PIXEL
    sub r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    sub r0, 8*SIZEOF_PIXEL
    sub r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    movd eax, m12
    RET

cglobal pixel_sa8d_32x8, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    movd eax, m12
    RET

cglobal pixel_sa8d_32x16, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_32x24, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    sub r0, 8*SIZEOF_PIXEL
    sub r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    sub r0, 8*SIZEOF_PIXEL
    sub r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    sub r0, 8*SIZEOF_PIXEL
    sub r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    add r0, 8*SIZEOF_PIXEL
    add r2, 8*SIZEOF_PIXEL
    SA8D_8x8
    movd eax, m12
    RET

cglobal pixel_sa8d_32x32, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_32x64, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_48x64, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_64x16, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_64x32, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_64x48, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_64x64, 4,8,13
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    add  r2, 16*SIZEOF_PIXEL
    add  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r0, r4
    sub  r2, r5
    sub  r2, 16*SIZEOF_PIXEL
    sub  r0, 16*SIZEOF_PIXEL
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

%else ; ARCH_X86_32
%if mmsize == 16
cglobal pixel_sa8d_8x8_internal
    %define spill0 [esp+4]
    %define spill1 [esp+20]
    %define spill2 [esp+36]
%if vertical
    LOAD_DIFF_8x4P 0, 1, 2, 3, 4, 5, 6, r0, r2, 1
    HADAMARD4_2D 0, 1, 2, 3, 4
    movdqa spill0, m3
    LOAD_DIFF_8x4P 4, 5, 6, 7, 3, 3, 2, r0, r2, 1
    HADAMARD4_2D 4, 5, 6, 7, 3
    HADAMARD2_2D 0, 4, 1, 5, 3, qdq, amax
    movdqa m3, spill0
    paddw m0, m1
    HADAMARD2_2D 2, 6, 3, 7, 5, qdq, amax
%else ; mmsize == 8
    mova m7, [hmul_8p]
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 5, 6, 7, r0, r2, 1
    ; could do first HADAMARD4_V here to save spilling later
    ; surprisingly, not a win on conroe or even p4
    mova spill0, m2
    mova spill1, m3
    mova spill2, m1
    SWAP 1, 7
    LOAD_SUMSUB_8x4P 4, 5, 6, 7, 2, 3, 1, r0, r2, 1
    HADAMARD4_V 4, 5, 6, 7, 3
    mova m1, spill2
    mova m2, spill0
    mova m3, spill1
    mova spill0, m6
    mova spill1, m7
    HADAMARD4_V 0, 1, 2, 3, 7
    SUMSUB_BADC w, 0, 4, 1, 5, 7
    HADAMARD 2, sumsub, 0, 4, 7, 6
    HADAMARD 2, sumsub, 1, 5, 7, 6
    HADAMARD 1, amax, 0, 4, 7, 6
    HADAMARD 1, amax, 1, 5, 7, 6
    mova m6, spill0
    mova m7, spill1
    paddw m0, m1
    SUMSUB_BADC w, 2, 6, 3, 7, 4
    HADAMARD 2, sumsub, 2, 6, 4, 5
    HADAMARD 2, sumsub, 3, 7, 4, 5
    HADAMARD 1, amax, 2, 6, 4, 5
    HADAMARD 1, amax, 3, 7, 4, 5
%endif ; sse2/non-sse2
    paddw m0, m2
    paddw m0, m3
    SAVE_MM_PERMUTATION
    ret
%endif ; ifndef mmx2

cglobal pixel_sa8d_8x8_internal2
    %define spill0 [esp+4]
    LOAD_DIFF_8x4P 0, 1, 2, 3, 4, 5, 6, r0, r2, 1
    HADAMARD4_2D 0, 1, 2, 3, 4
    movdqa spill0, m3
    LOAD_DIFF_8x4P 4, 5, 6, 7, 3, 3, 2, r0, r2, 1
    HADAMARD4_2D 4, 5, 6, 7, 3
    HADAMARD2_2D 0, 4, 1, 5, 3, qdq, amax
    movdqa m3, spill0
    paddw m0, m1
    HADAMARD2_2D 2, 6, 3, 7, 5, qdq, amax
    paddw m0, m2
    paddw m0, m3
    SAVE_MM_PERMUTATION
    ret

cglobal pixel_sa8d_8x8, 4,7
    FIX_STRIDES r1, r3
    mov    r6, esp
    and   esp, ~15
    sub   esp, 48
    lea    r4, [3*r1]
    lea    r5, [3*r3]
    call pixel_sa8d_8x8_internal
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%else
    HADDW  m0, m1
%endif ; HIGH_BIT_DEPTH
    movd  eax, m0
    add   eax, 1
    shr   eax, 1
    mov   esp, r6
    RET

cglobal pixel_sa8d_16x16, 4,7
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    call pixel_sa8d_8x8_internal
%if mmsize == 8
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%endif
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal
    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal
%if mmsize == 8
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
%else
    SA8D_INTER
%endif
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal
%if HIGH_BIT_DEPTH
    SA8D_INTER
%else ; !HIGH_BIT_DEPTH
    paddusw m0, [esp+64-mmsize]
%if mmsize == 16
    HADDUW m0, m1
%else
    mova m2, [esp+48]
    pxor m7, m7
    mova m1, m0
    mova m3, m2
    punpcklwd m0, m7
    punpckhwd m1, m7
    punpcklwd m2, m7
    punpckhwd m3, m7
    paddd m0, m1
    paddd m2, m3
    paddd m0, m2
    HADDD m0, m1
%endif
%endif ; HIGH_BIT_DEPTH
    movd eax, m0
    add  eax, 1
    shr  eax, 1
    mov  esp, r6
    RET

cglobal pixel_sa8d_8x16, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_8x32, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_16x8, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_16x32, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [rsp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_16x64, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [rsp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_24x32, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_32x8, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_32x16, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [rsp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_32x24, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add  r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_32x32, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [rsp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_32x64, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [rsp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_48x64, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [rsp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_64x16, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [rsp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 48*SIZEOF_PIXEL
    add  r2, 48*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 56*SIZEOF_PIXEL
    add  r2, 56*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_64x32, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [rsp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 48*SIZEOF_PIXEL
    add  r2, 48*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 56*SIZEOF_PIXEL
    add  r2, 56*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 48*SIZEOF_PIXEL
    add  r2, 48*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 56*SIZEOF_PIXEL
    add  r2, 56*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_64x48, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [rsp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 48*SIZEOF_PIXEL
    add  r2, 48*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 56*SIZEOF_PIXEL
    add  r2, 56*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 48*SIZEOF_PIXEL
    add  r2, 48*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 56*SIZEOF_PIXEL
    add  r2, 56*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 48*SIZEOF_PIXEL
    add  r2, 48*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 56*SIZEOF_PIXEL
    add  r2, 56*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET

cglobal pixel_sa8d_64x64, 4,7,8
    FIX_STRIDES r1, r3
    mov  r6, esp
    and  esp, ~15
    sub  esp, 64

    lea  r4, [r1 + 2*r1]
    lea  r5, [r3 + 2*r3]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [rsp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 48*SIZEOF_PIXEL
    add  r2, 48*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 56*SIZEOF_PIXEL
    add  r2, 56*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 48*SIZEOF_PIXEL
    add  r2, 48*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 56*SIZEOF_PIXEL
    add  r2, 56*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 48*SIZEOF_PIXEL
    add  r2, 48*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 56*SIZEOF_PIXEL
    add  r2, 56*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    mov  [r6+20], r0
    mov  [r6+28], r2

    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 8*SIZEOF_PIXEL
    add  r2, 8*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 16*SIZEOF_PIXEL
    add  r2, 16*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 24*SIZEOF_PIXEL
    add  r2, 24*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 32*SIZEOF_PIXEL
    add  r2, 32*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 40*SIZEOF_PIXEL
    add  r2, 40*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    AVG_16x16

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 48*SIZEOF_PIXEL
    add  r2, 48*SIZEOF_PIXEL
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
%if HIGH_BIT_DEPTH
    HADDUW m0, m1
%endif
    mova [esp+48], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+48], m0

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    add  r0, 56*SIZEOF_PIXEL
    add  r2, 56*SIZEOF_PIXEL
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
    mova [esp+64-mmsize], m0
    call pixel_sa8d_8x8_internal2
    SA8D_INTER
%if HIGH_BIT_DEPTH == 0
    HADDUW m0, m1
%endif
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov eax, r4d
    mov esp, r6
    RET
%endif ; !ARCH_X86_64
%endmacro ; SA8D

;=============================================================================
; INTRA SATD
;=============================================================================
%define TRANS TRANS_SSE2
%define DIFFOP DIFF_UNPACK_SSE2
%define LOAD_SUMSUB_8x4P LOAD_DIFF_8x4P
%define LOAD_SUMSUB_16P  LOAD_SUMSUB_16P_SSE2
%define movdqa movaps ; doesn't hurt pre-nehalem, might as well save size
%define movdqu movups
%define punpcklqdq movlhps
INIT_XMM sse2
SA8D
SATDS_SSE2

%if HIGH_BIT_DEPTH == 0
INIT_XMM ssse3,atom
SATDS_SSE2
SA8D
%endif

%define DIFFOP DIFF_SUMSUB_SSSE3
%define LOAD_DUP_4x8P LOAD_DUP_4x8P_CONROE
%if HIGH_BIT_DEPTH == 0
%define LOAD_SUMSUB_8x4P LOAD_SUMSUB_8x4P_SSSE3
%define LOAD_SUMSUB_16P  LOAD_SUMSUB_16P_SSSE3
%endif
INIT_XMM ssse3
SATDS_SSE2
SA8D
%undef movdqa ; nehalem doesn't like movaps
%undef movdqu ; movups
%undef punpcklqdq ; or movlhps

%define TRANS TRANS_SSE4
%define LOAD_DUP_4x8P LOAD_DUP_4x8P_PENRYN
INIT_XMM sse4
SATDS_SSE2
SA8D

; Sandy/Ivy Bridge and Bulldozer do movddup in the load unit, so
; it's effectively free.
%define LOAD_DUP_4x8P LOAD_DUP_4x8P_CONROE
INIT_XMM avx
SATDS_SSE2
SA8D

%define TRANS TRANS_XOP
INIT_XMM xop
SATDS_SSE2
SA8D


%if HIGH_BIT_DEPTH == 0
%define LOAD_SUMSUB_8x4P LOAD_SUMSUB8_16x4P_AVX2
%define LOAD_DUP_4x8P LOAD_DUP_4x16P_AVX2
%define TRANS TRANS_SSE4

%macro LOAD_SUMSUB_8x8P_AVX2 7 ; 4*dst, 2*tmp, mul]
    movq   xm%1, [r0]
    movq   xm%3, [r2]
    movq   xm%2, [r0+r1]
    movq   xm%4, [r2+r3]
    vinserti128 m%1, m%1, [r0+4*r1], 1
    vinserti128 m%3, m%3, [r2+4*r3], 1
    vinserti128 m%2, m%2, [r0+r4], 1
    vinserti128 m%4, m%4, [r2+r5], 1
    punpcklqdq m%1, m%1
    punpcklqdq m%3, m%3
    punpcklqdq m%2, m%2
    punpcklqdq m%4, m%4
    DIFF_SUMSUB_SSSE3 %1, %3, %2, %4, %7
    lea      r0, [r0+2*r1]
    lea      r2, [r2+2*r3]

    movq   xm%3, [r0]
    movq   xm%5, [r2]
    movq   xm%4, [r0+r1]
    movq   xm%6, [r2+r3]
    vinserti128 m%3, m%3, [r0+4*r1], 1
    vinserti128 m%5, m%5, [r2+4*r3], 1
    vinserti128 m%4, m%4, [r0+r4], 1
    vinserti128 m%6, m%6, [r2+r5], 1
    punpcklqdq m%3, m%3
    punpcklqdq m%5, m%5
    punpcklqdq m%4, m%4
    punpcklqdq m%6, m%6
    DIFF_SUMSUB_SSSE3 %3, %5, %4, %6, %7
%endmacro

%macro SATD_START_AVX2 2-3 0
    FIX_STRIDES r1, r3
%if %3
    mova    %2, [hmul_8p]
    lea     r4, [5*r1]
    lea     r5, [5*r3]
%else
    mova    %2, [hmul_16p]
    lea     r4, [3*r1]
    lea     r5, [3*r3]
%endif
    pxor    %1, %1
%endmacro

%define TRANS TRANS_SSE4
INIT_YMM avx2
cglobal pixel_satd_16x8_internal
    LOAD_SUMSUB_16x4P_AVX2 0, 1, 2, 3, 4, 5, 7, r0, r2, 1
    SATD_8x4_SSE 0, 0, 1, 2, 3, 4, 5, 6
    LOAD_SUMSUB_16x4P_AVX2 0, 1, 2, 3, 4, 5, 7, r0, r2, 0
    SATD_8x4_SSE 0, 0, 1, 2, 3, 4, 5, 6
    ret

cglobal pixel_satd_16x16, 4,6,8
    SATD_START_AVX2 m6, m7
    call pixel_satd_16x8_internal
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
pixel_satd_16x8_internal:
    call pixel_satd_16x8_internal
    vextracti128 xm0, m6, 1
    paddw        xm0, xm6
    SATD_END_SSE2 xm0
    RET

cglobal pixel_satd_16x8, 4,6,8
    SATD_START_AVX2 m6, m7
    jmp pixel_satd_16x8_internal

cglobal pixel_satd_8x8_internal
    LOAD_SUMSUB_8x8P_AVX2 0, 1, 2, 3, 4, 5, 7
    SATD_8x4_SSE 0, 0, 1, 2, 3, 4, 5, 6
    ret

cglobal pixel_satd_8x16, 4,6,8
    SATD_START_AVX2 m6, m7, 1
    call pixel_satd_8x8_internal
    lea  r0, [r0+2*r1]
    lea  r2, [r2+2*r3]
    lea  r0, [r0+4*r1]
    lea  r2, [r2+4*r3]
    call pixel_satd_8x8_internal
    vextracti128 xm0, m6, 1
    paddw        xm0, xm6
    SATD_END_SSE2 xm0
    RET

cglobal pixel_satd_8x8, 4,6,8
    SATD_START_AVX2 m6, m7, 1
    call pixel_satd_8x8_internal
    vextracti128 xm0, m6, 1
    paddw        xm0, xm6
    SATD_END_SSE2 xm0
    RET

cglobal pixel_sa8d_8x8_internal
    LOAD_SUMSUB_8x8P_AVX2 0, 1, 2, 3, 4, 5, 7
    HADAMARD4_V 0, 1, 2, 3, 4
    HADAMARD 8, sumsub, 0, 1, 4, 5
    HADAMARD 8, sumsub, 2, 3, 4, 5
    HADAMARD 2, sumsub, 0, 1, 4, 5
    HADAMARD 2, sumsub, 2, 3, 4, 5
    HADAMARD 1, amax, 0, 1, 4, 5
    HADAMARD 1, amax, 2, 3, 4, 5
    paddw  m6, m0
    paddw  m6, m2
    ret

cglobal pixel_sa8d_8x8, 4,6,8
    SATD_START_AVX2 m6, m7, 1
    call pixel_sa8d_8x8_internal
    vextracti128 xm1, m6, 1
    paddw xm6, xm1
    HADDW xm6, xm1
    movd  eax, xm6
    add   eax, 1
    shr   eax, 1
    RET
%endif ; HIGH_BIT_DEPTH

; Input 16bpp, Output 8bpp
;------------------------------------------------------------------------------------------------------------------------
;void planecopy_sp(uint16_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height, int shift, uint16_t mask)
;------------------------------------------------------------------------------------------------------------------------
INIT_XMM sse2
cglobal downShift_16, 7,7,3
    movd        m0, r6d        ; m0 = shift
    add         r1, r1
    dec         r5d
.loopH:
    xor         r6, r6
.loopW:
    movu        m1, [r0 + r6 * 2]
    movu        m2, [r0 + r6 * 2 + 16]
    psrlw       m1, m0
    psrlw       m2, m0
    packuswb    m1, m2
    movu        [r2 + r6], m1

    add         r6, 16
    cmp         r6d, r4d
    jl          .loopW

    ; move to next row
    add         r0, r1
    add         r2, r3
    dec         r5d
    jnz         .loopH

;processing last row of every frame [To handle width which not a multiple of 16]

.loop16:
    movu        m1, [r0]
    movu        m2, [r0 + 16]
    psrlw       m1, m0
    psrlw       m2, m0
    packuswb    m1, m2
    movu        [r2], m1

    add         r0, 2 * mmsize
    add         r2, mmsize
    sub         r4d, 16
    jz          .end
    cmp         r4d, 15
    jg          .loop16

    cmp         r4d, 8
    jl          .process4
    movu        m1, [r0]
    psrlw       m1, m0
    packuswb    m1, m1
    movh        [r2], m1

    add         r0, mmsize
    add         r2, 8
    sub         r4d, 8
    jz          .end

.process4:
    cmp         r4d, 4
    jl          .process2
    movh        m1,[r0]
    psrlw       m1, m0
    packuswb    m1, m1
    movd        [r2], m1

    add         r0, 8
    add         r2, 4
    sub         r4d, 4
    jz          .end

.process2:
    cmp         r4d, 2
    jl          .process1
    movd        m1, [r0]
    psrlw       m1, m0
    packuswb    m1, m1
    movd        r6, m1
    mov         [r2], r6w

    add         r0, 4
    add         r2, 2
    sub         r4d, 2
    jz          .end

.process1:
    movd        m1, [r0]
    psrlw       m1, m0
    packuswb    m1, m1
    movd        r3, m1
    mov         [r2], r3b
.end:
    RET

; Input 8bpp, Output 16bpp
;---------------------------------------------------------------------------------------------------------------------
;void planecopy_cp(uint8_t *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int width, int height, int shift)
;---------------------------------------------------------------------------------------------------------------------
INIT_XMM sse4
cglobal upShift_8, 7,7,3

    movd        m2, r6d        ; m0 = shift
    add         r3, r3
    dec         r5d

.loopH:
    xor         r6, r6
.loopW:
    pmovzxbw    m0,[r0 + r6]
    pmovzxbw    m1,[r0 + r6 + 8]
    psllw       m0, m2
    psllw       m1, m2
    movu        [r2 + r6 * 2], m0
    movu        [r2 + r6 * 2 + 16], m1

    add         r6, 16
    cmp         r6d, r4d
    jl          .loopW

    ; move to next row
    add         r0, r1
    add         r2, r3
    dec         r5d
    jnz         .loopH

;processing last row of every frame [To handle width which not a multiple of 16]

.loop16:
    pmovzxbw    m0,[r0]
    pmovzxbw    m1,[r0 + 8]
    psllw       m0, m2
    psllw       m1, m2
    movu        [r2], m0
    movu        [r2 + 16], m1

    add         r0, mmsize
    add         r2, 2 * mmsize
    sub         r4d, 16
    jz          .end
    cmp         r4d, 15
    jg          .loop16

    cmp         r4d, 8
    jl          .process4
    pmovzxbw    m0,[r0]
    psllw       m0, m2
    movu        [r2], m0

    add         r0, 8
    add         r2, mmsize
    sub         r4d, 8
    jz          .end

.process4:
    cmp         r4d, 4
    jl          .process2
    movd        m0,[r0]
    pmovzxbw    m0,m0
    psllw       m0, m2
    movh        [r2], m0

    add         r0, 4
    add         r2, 8
    sub         r4d, 4
    jz          .end

.process2:
    cmp         r4d, 2
    jl          .process1
    movzx       r3d, byte [r0]
    shl         r3d, 2
    mov         [r2], r3w
    movzx       r3d, byte [r0 + 1]
    shl         r3d, 2
    mov         [r2 + 2], r3w

    add         r0, 2
    add         r2, 4
    sub         r4d, 2
    jz          .end

.process1:
    movzx       r3d, byte [r0]
    shl         r3d, 2
    mov         [r2], r3w
.end:
    RET
