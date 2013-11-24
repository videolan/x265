;*****************************************************************************
;* pixel.asm: x86 pixel metrics
;*****************************************************************************
;* Copyright (C) 2003-2013 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Holger Lubitz <holger@lubitz.org>
;*          Laurent Aimar <fenrir@via.ecp.fr>
;*          Alex Izvorski <aizvorksi@gmail.com>
;*          Jason Garrett-Glaser <darkshikari@gmail.com>
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
;* For more information, contact us at licensing@x264.com.
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
mask_ff:   times 16 db 0xff
           times 16 db 0
mask_ac4:  times 2 dw 0, -1, -1, -1, 0, -1, -1, -1
mask_ac4b: times 2 dw 0, -1, 0, -1, -1, -1, -1, -1
mask_ac8:  times 2 dw 0, -1, -1, -1, -1, -1, -1, -1
%if BIT_DEPTH == 10
ssim_c1:   times 4 dd 6697.7856    ; .01*.01*1023*1023*64
ssim_c2:   times 4 dd 3797644.4352 ; .03*.03*1023*1023*64*63
pf_64:     times 4 dd 64.0
pf_128:    times 4 dd 128.0
%elif BIT_DEPTH == 9
ssim_c1:   times 4 dd 1671         ; .01*.01*511*511*64
ssim_c2:   times 4 dd 947556       ; .03*.03*511*511*64*63
%else ; 8-bit
ssim_c1:   times 4 dd 416          ; .01*.01*255*255*64
ssim_c2:   times 4 dd 235963       ; .03*.03*255*255*64*63
%endif
hmul_4p:   times 2 db 1, 1, 1, 1, 1, -1, 1, -1
mask_10:   times 4 dw 0, -1
mask_1100: times 2 dd 0, -1
pb_pppm:   times 4 db 1,1,1,-1
deinterleave_shuf: db 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15

ALIGN 32
pw_s00112233:  dw 0x8000,0x8000,0x8001,0x8001,0x8002,0x8002,0x8003,0x8003
pw_s00001111:  dw 0x8000,0x8000,0x8000,0x8000,0x8001,0x8001,0x8001,0x8001

transd_shuf1: SHUFFLE_MASK_W 0, 8, 2, 10, 4, 12, 6, 14
transd_shuf2: SHUFFLE_MASK_W 1, 9, 3, 11, 5, 13, 7, 15

sw_f0:     dq 0xfff0, 0
pd_f0:     times 4 dd 0xffff0000

pw_76543210: dw 0, 1, 2, 3, 4, 5, 6, 7

ads_mvs_shuffle:
%macro ADS_MVS_SHUFFLE 8
    %assign y x
    %rep 8
        %rep 7
            %rotate (~y)&1
            %assign y y>>((~y)&1)
        %endrep
        db %1*2, %1*2+1
        %rotate 1
        %assign y y>>1
    %endrep
%endmacro
%assign x 0
%rep 256
    ADS_MVS_SHUFFLE 0, 1, 2, 3, 4, 5, 6, 7
%assign x x+1
%endrep

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
cextern hsub_mul
cextern popcnt_table

;=============================================================================
; SSD
;=============================================================================

%if HIGH_BIT_DEPTH
;-----------------------------------------------------------------------------
; int pixel_ssd_WxH( uint16_t *, intptr_t, uint16_t *, intptr_t )
;-----------------------------------------------------------------------------
%macro SSD_ONE 2
cglobal pixel_ssd_%1x%2, 4,7,6
    FIX_STRIDES r1, r3
%if mmsize == %1*2
    %define offset0_1 r1
    %define offset0_2 r1*2
    %define offset0_3 r5
    %define offset1_1 r3
    %define offset1_2 r3*2
    %define offset1_3 r6
    lea     r5, [3*r1]
    lea     r6, [3*r3]
%elif mmsize == %1
    %define offset0_1 mmsize
    %define offset0_2 r1
    %define offset0_3 r1+mmsize
    %define offset1_1 mmsize
    %define offset1_2 r3
    %define offset1_3 r3+mmsize
%elif mmsize == %1/2
    %define offset0_1 mmsize
    %define offset0_2 mmsize*2
    %define offset0_3 mmsize*3
    %define offset1_1 mmsize
    %define offset1_2 mmsize*2
    %define offset1_3 mmsize*3
%endif
    %assign %%n %2/(2*mmsize/%1)
%if %%n > 1
    mov    r4d, %%n
%endif
    pxor    m0, m0
.loop
    mova    m1, [r0]
    mova    m2, [r0+offset0_1]
    mova    m3, [r0+offset0_2]
    mova    m4, [r0+offset0_3]
    psubw   m1, [r2]
    psubw   m2, [r2+offset1_1]
    psubw   m3, [r2+offset1_2]
    psubw   m4, [r2+offset1_3]
%if %%n > 1
    lea     r0, [r0+r1*(%2/%%n)]
    lea     r2, [r2+r3*(%2/%%n)]
%endif
    pmaddwd m1, m1
    pmaddwd m2, m2
    pmaddwd m3, m3
    pmaddwd m4, m4
    paddd   m1, m2
    paddd   m3, m4
    paddd   m0, m1
    paddd   m0, m3
%if %%n > 1
    dec    r4d
    jg .loop
%endif
    HADDD   m0, m5
    movd   eax, xm0
%if (cpuflags == cpuflags_mmx)
    emms
%endif
    RET
%endmacro

INIT_MMX mmx2
SSD_ONE     4,  4
SSD_ONE     4,  8
SSD_ONE     4, 16
SSD_ONE     8,  4
SSD_ONE     8,  8
SSD_ONE     8, 16
SSD_ONE    16,  8
SSD_ONE    16, 16
INIT_XMM sse2
SSD_ONE     8,  4
SSD_ONE     8,  8
SSD_ONE     8, 16
SSD_ONE    16,  8
SSD_ONE    16, 16
INIT_YMM avx2
SSD_ONE    16,  8
SSD_ONE    16, 16
%endif ; HIGH_BIT_DEPTH

%if HIGH_BIT_DEPTH == 0
%macro SSD_LOAD_FULL 5
    mova      m1, [t0+%1]
    mova      m2, [t2+%2]
    mova      m3, [t0+%3]
    mova      m4, [t2+%4]
%if %5==1
    add       t0, t1
    add       t2, t3
%elif %5==2
    lea       t0, [t0+2*t1]
    lea       t2, [t2+2*t3]
%endif
%endmacro

%macro LOAD 5
    movh      m%1, %3
    movh      m%2, %4
%if %5
    lea       t0, [t0+2*t1]
%endif
%endmacro

%macro JOIN 7
    movh      m%3, %5
    movh      m%4, %6
%if %7
    lea       t2, [t2+2*t3]
%endif
    punpcklbw m%1, m7
    punpcklbw m%3, m7
    psubw     m%1, m%3
    punpcklbw m%2, m7
    punpcklbw m%4, m7
    psubw     m%2, m%4
%endmacro

%macro JOIN_SSE2 7
    movh      m%3, %5
    movh      m%4, %6
%if %7
    lea       t2, [t2+2*t3]
%endif
    punpcklqdq m%1, m%2
    punpcklqdq m%3, m%4
    DEINTB %2, %1, %4, %3, 7
    psubw m%2, m%4
    psubw m%1, m%3
%endmacro

%macro JOIN_SSSE3 7
    movh      m%3, %5
    movh      m%4, %6
%if %7
    lea       t2, [t2+2*t3]
%endif
    punpcklbw m%1, m%3
    punpcklbw m%2, m%4
%endmacro

%macro LOAD_AVX2 5
    mova     xm%1, %3
    vinserti128 m%1, m%1, %4, 1
%if %5
    lea       t0, [t0+2*t1]
%endif
%endmacro

%macro JOIN_AVX2 7
    mova     xm%2, %5
    vinserti128 m%2, m%2, %6, 1
%if %7
    lea       t2, [t2+2*t3]
%endif
    SBUTTERFLY bw, %1, %2, %3
%endmacro

%macro SSD_LOAD_HALF 5
    LOAD      1, 2, [t0+%1], [t0+%3], 1
    JOIN      1, 2, 3, 4, [t2+%2], [t2+%4], 1
    LOAD      3, 4, [t0+%1], [t0+%3], %5
    JOIN      3, 4, 5, 6, [t2+%2], [t2+%4], %5
%endmacro

%macro SSD_CORE 7-8
%ifidn %8, FULL
    mova      m%6, m%2
    mova      m%7, m%4
    psubusb   m%2, m%1
    psubusb   m%4, m%3
    psubusb   m%1, m%6
    psubusb   m%3, m%7
    por       m%1, m%2
    por       m%3, m%4
    punpcklbw m%2, m%1, m%5
    punpckhbw m%1, m%5
    punpcklbw m%4, m%3, m%5
    punpckhbw m%3, m%5
%endif
    pmaddwd   m%1, m%1
    pmaddwd   m%2, m%2
    pmaddwd   m%3, m%3
    pmaddwd   m%4, m%4
%endmacro

%macro SSD_CORE_SSE2 7-8
%ifidn %8, FULL
    DEINTB %6, %1, %7, %2, %5
    psubw m%6, m%7
    psubw m%1, m%2
    SWAP %6, %2, %1
    DEINTB %6, %3, %7, %4, %5
    psubw m%6, m%7
    psubw m%3, m%4
    SWAP %6, %4, %3
%endif
    pmaddwd   m%1, m%1
    pmaddwd   m%2, m%2
    pmaddwd   m%3, m%3
    pmaddwd   m%4, m%4
%endmacro

%macro SSD_CORE_SSSE3 7-8
%ifidn %8, FULL
    punpckhbw m%6, m%1, m%2
    punpckhbw m%7, m%3, m%4
    punpcklbw m%1, m%2
    punpcklbw m%3, m%4
    SWAP %6, %2, %3
    SWAP %7, %4
%endif
    pmaddubsw m%1, m%5
    pmaddubsw m%2, m%5
    pmaddubsw m%3, m%5
    pmaddubsw m%4, m%5
    pmaddwd   m%1, m%1
    pmaddwd   m%2, m%2
    pmaddwd   m%3, m%3
    pmaddwd   m%4, m%4
%endmacro

%macro SSD_ITER 6
    SSD_LOAD_%1 %2,%3,%4,%5,%6
    SSD_CORE  1, 2, 3, 4, 7, 5, 6, %1
    paddd     m1, m2
    paddd     m3, m4
    paddd     m0, m1
    paddd     m0, m3
%endmacro

;-----------------------------------------------------------------------------
; int pixel_ssd_16x16( uint8_t *, intptr_t, uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
%macro SSD 2
%if %1 != %2
    %assign function_align 8
%else
    %assign function_align 16
%endif
cglobal pixel_ssd_%1x%2, 0,0,0
    mov     al, %1*%2/mmsize/2

%if %1 != %2
    jmp mangle(x265_pixel_ssd_%1x%1 %+ SUFFIX %+ .startloop)
%else

.startloop:
%if ARCH_X86_64
    DECLARE_REG_TMP 0,1,2,3
    PROLOGUE 0,0,8
%else
    PROLOGUE 0,5
    DECLARE_REG_TMP 1,2,3,4
    mov t0, r0m
    mov t1, r1m
    mov t2, r2m
    mov t3, r3m
%endif

%if cpuflag(ssse3)
    mova    m7, [hsub_mul]
%elifidn cpuname, sse2
    mova    m7, [pw_00ff]
%elif %1 >= mmsize
    pxor    m7, m7
%endif
    pxor    m0, m0

ALIGN 16
.loop:
%if %1 > mmsize
    SSD_ITER FULL, 0, 0, mmsize, mmsize, 1
%elif %1 == mmsize
    SSD_ITER FULL, 0, 0, t1, t3, 2
%else
    SSD_ITER HALF, 0, 0, t1, t3, 2
%endif
    dec     al
    jg .loop
%if mmsize==32
    vextracti128 xm1, m0, 1
    paddd  xm0, xm1
    HADDD  xm0, xm1
    movd   eax, xm0
%else
    HADDD   m0, m1
    movd   eax, m0
%endif
%if (mmsize == 8)
    emms
%endif
    RET
%endif
%endmacro

%macro HEVC_SSD 0
SSD 32, 64
SSD 16, 64
SSD 32, 32
SSD 32, 16
SSD 16, 32
SSD 32, 8
SSD 8,  32
SSD 32, 24
SSD 24, 24 ; not used, but resolves x265_pixel_ssd_24x24_sse2.startloop symbol
SSD 8,  4
SSD 8,  8
SSD 16, 16
SSD 16, 12
SSD 16, 8
SSD 8,  16
SSD 16, 4
%endmacro

INIT_MMX mmx
SSD 16, 16
SSD 16,  8
SSD  8,  8
SSD  8, 16
SSD  4,  4
SSD  8,  4
SSD  4,  8
SSD  4, 16
INIT_XMM sse2slow
SSD 16, 16
SSD  8,  8
SSD 16,  8
SSD  8, 16
SSD  8,  4
INIT_XMM sse2
%define SSD_CORE SSD_CORE_SSE2
%define JOIN JOIN_SSE2
HEVC_SSD
INIT_XMM ssse3
%define SSD_CORE SSD_CORE_SSSE3
%define JOIN JOIN_SSSE3
HEVC_SSD
INIT_XMM avx
HEVC_SSD
INIT_MMX ssse3
SSD  4,  4
SSD  4,  8
SSD  4, 16
INIT_XMM xop
SSD 16, 16
SSD  8,  8
SSD 16,  8
SSD  8, 16
SSD  8,  4
%define LOAD LOAD_AVX2
%define JOIN JOIN_AVX2
INIT_YMM avx2
SSD 16, 16
SSD 16,  8
%assign function_align 16
%endif ; !HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; int pixel_ssd_12x16( uint8_t *, intptr_t, uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_ssd_12x16, 4, 5, 7, src1, stride1, src2, stride2

    pxor        m6,     m6
    mov         r4d,    4

.loop
    movu        m0,    [r0]
    movu        m1,    [r2]
    movu        m2,    [r0 + r1]
    movu        m3,    [r2 + r3]

    punpckhdq   m4,    m0,    m2
    punpckhdq   m5,    m1,    m3

    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5

    psubw       m0,    m1
    psubw       m2,    m3
    psubw       m4,    m5

    pmaddwd     m0,    m0
    pmaddwd     m2,    m2
    pmaddwd     m4,    m4

    paddd       m0,    m2
    paddd       m6,    m4
    paddd       m6,    m0

    movu        m0,    [r0 + 2 * r1]
    movu        m1,    [r2 + 2 * r3]
    lea         r0,    [r0 + 2 * r1]
    lea         r2,    [r2 + 2 * r3]
    movu        m2,    [r0 + r1]
    movu        m3,    [r2 + r3]

    punpckhdq   m4,    m0,    m2
    punpckhdq   m5,    m1,    m3

    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5

    psubw       m0,    m1
    psubw       m2,    m3
    psubw       m4,    m5

    pmaddwd     m0,    m0
    pmaddwd     m2,    m2
    pmaddwd     m4,    m4

    paddd       m0,    m2
    paddd       m6,    m4
    paddd       m6,    m0

    dec    r4d
    lea       r0,                    [r0 + 2 * r1]
    lea       r2,                    [r2 + 2 * r3]
    jnz    .loop

    HADDD   m6, m1
    movd   eax, m6

    RET

;-----------------------------------------------------------------------------
; int pixel_ssd_24x32( uint8_t *, intptr_t, uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_ssd_24x32, 4, 5, 8, src1, stride1, src2, stride2

    pxor    m7,     m7
    pxor    m6,     m6
    mov     r4d,    16

.loop
    movu         m1,    [r0]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    pmovzxbw     m2,    [r0 + 16]
    movu         m4,    [r2]
    pmovzxbw     m3,    m4
    punpckhbw    m4,    m6
    pmovzxbw     m5,    [r2 + 16]

    psubw        m0,    m3
    psubw        m1,    m4
    psubw        m2,    m5

    pmaddwd      m0,    m0
    pmaddwd      m1,    m1
    pmaddwd      m2,    m2

    paddd        m0,    m1
    paddd        m7,    m2
    paddd        m7,    m0

    movu         m1,    [r0 + r1]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    pmovzxbw     m2,    [r0 + r1 + 16]
    movu         m4,    [r2 + r3]
    pmovzxbw     m3,    m4
    punpckhbw    m4,    m6
    pmovzxbw     m5,    [r2 + r3 + 16]

    psubw        m0,    m3
    psubw        m1,    m4
    psubw        m2,    m5

    pmaddwd      m0,    m0
    pmaddwd      m1,    m1
    pmaddwd      m2,    m2

    paddd        m0,    m1
    paddd        m7,    m2
    paddd        m7,    m0

    dec    r4d
    lea    r0,    [r0 + 2 * r1]
    lea    r2,    [r2 + 2 * r3]
    jnz    .loop

    HADDD   m7, m1
    movd   eax, m7

    RET

%macro PIXEL_SSD_16x4 0
    movu         m1,    [r0]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r2]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m0,    m2
    psubw        m1,    m3

    movu         m5,    [r0 + r1]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    movu         m3,    [r2 + r3]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m4,    m2
    psubw        m5,    m3

    pmaddwd      m0,    m0
    pmaddwd      m1,    m1
    pmaddwd      m4,    m4
    pmaddwd      m5,    m5

    paddd        m0,    m1
    paddd        m4,    m5
    paddd        m4,    m0
    paddd        m7,    m4

    movu         m1,    [r0 + r6]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r2 + 2 * r3]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m0,    m2
    psubw        m1,    m3

    lea          r0,    [r0 + r6]
    lea          r2,    [r2 + 2 * r3]
    movu         m5,    [r0 + r1]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    movu         m3,    [r2 + r3]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m4,    m2
    psubw        m5,    m3

    pmaddwd      m0,    m0
    pmaddwd      m1,    m1
    pmaddwd      m4,    m4
    pmaddwd      m5,    m5

    paddd        m0,    m1
    paddd        m4,    m5
    paddd        m4,    m0
    paddd        m7,    m4
%endmacro

cglobal pixel_ssd_16x16_internal
    PIXEL_SSD_16x4
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    PIXEL_SSD_16x4
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    PIXEL_SSD_16x4
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    PIXEL_SSD_16x4
    ret

;-----------------------------------------------------------------------------
; int pixel_ssd_48x64( uint8_t *, intptr_t, uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_ssd_48x64, 4, 7, 8, src1, stride1, src2, stride2

    pxor    m7,    m7
    pxor    m6,    m6
    mov     r4,    r0
    mov     r5,    r2
    lea     r6,    [r1 * 2]

    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 16]
    lea     r2,    [r5 + 16]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 32]
    lea     r2,    [r5 + 32]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal

    HADDD    m7,     m1
    movd     eax,    m7

    RET

;-----------------------------------------------------------------------------
; int pixel_ssd_64x16( uint8_t *, intptr_t, uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_ssd_64x16, 4, 7, 8, src1, stride1, src2, stride2

    pxor    m7,    m7
    pxor    m6,    m6
    mov     r4,    r0
    mov     r5,    r2
    lea     r6,    [r1 * 2]

    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 16]
    lea     r2,    [r5 + 16]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 32]
    lea     r2,    [r5 + 32]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 48]
    lea     r2,    [r5 + 48]
    call    pixel_ssd_16x16_internal

    HADDD    m7,      m1
    movd     eax,     m7

    RET

;-----------------------------------------------------------------------------
; int pixel_ssd_64x32( uint8_t *, intptr_t, uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_ssd_64x32, 4, 7, 8, src1, stride1, src2, stride2

    pxor    m7,    m7
    pxor    m6,    m6
    mov     r4,    r0
    mov     r5,    r2
    lea     r6,    [r1 * 2]

    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 16]
    lea     r2,    [r5 + 16]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 32]
    lea     r2,    [r5 + 32]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 48]
    lea     r2,    [r5 + 48]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal

    HADDD    m7,     m1
    movd     eax,    m7

    RET

;-----------------------------------------------------------------------------
; int pixel_ssd_64x48( uint8_t *, intptr_t, uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_ssd_64x48, 4, 7, 8, src1, stride1, src2, stride2

    pxor    m7,    m7
    pxor    m6,    m6
    mov     r4,    r0
    mov     r5,    r2
    lea     r6,    [r1 * 2]

    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 16]
    lea     r2,    [r5 + 16]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 32]
    lea     r2,    [r5 + 32]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 48]
    lea     r2,    [r5 + 48]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal

    HADDD    m7,     m1
    movd     eax,    m7

    RET

;-----------------------------------------------------------------------------
; int pixel_ssd_64x64( uint8_t *, intptr_t, uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_ssd_64x64, 4, 7, 8, src1, stride1, src2, stride2

    pxor    m7,    m7
    pxor    m6,    m6
    mov     r4,    r0
    mov     r5,    r2
    lea     r6,    [r1 * 2]

    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 16]
    lea     r2,    [r5 + 16]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 32]
    lea     r2,    [r5 + 32]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r4 + 48]
    lea     r2,    [r5 + 48]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal
    lea     r0,    [r0 + r6]
    lea     r2,    [r2 + 2 * r3]
    call    pixel_ssd_16x16_internal

    HADDD    m7,     m1
    movd     eax,    m7

    RET

;-----------------------------------------------------------------------------
; void pixel_ssd_nv12_core( uint16_t *pixuv1, intptr_t stride1, uint16_t *pixuv2, intptr_t stride2,
;                           int width, int height, uint64_t *ssd_u, uint64_t *ssd_v )
;
; The maximum width this function can handle without risk of overflow is given
; in the following equation: (mmsize in bits)
;
;   2 * mmsize/32 * (2^32 - 1) / (2^BIT_DEPTH - 1)^2
;
; For 10-bit MMX this means width >= 16416 and for XMM >= 32832. At sane
; distortion levels it will take much more than that though.
;-----------------------------------------------------------------------------
%if HIGH_BIT_DEPTH
%macro SSD_NV12 0
cglobal pixel_ssd_nv12_core, 6,7,7
    shl        r4d, 2
    FIX_STRIDES r1, r3
    add         r0, r4
    add         r2, r4
    xor         r6, r6
    pxor        m4, m4
    pxor        m5, m5
    pxor        m6, m6
.loopy:
    mov         r6, r4
    neg         r6
    pxor        m2, m2
    pxor        m3, m3
.loopx:
    mova        m0, [r0+r6]
    mova        m1, [r0+r6+mmsize]
    psubw       m0, [r2+r6]
    psubw       m1, [r2+r6+mmsize]
    PSHUFLW     m0, m0, q3120
    PSHUFLW     m1, m1, q3120
%if mmsize >= 16
    pshufhw     m0, m0, q3120
    pshufhw     m1, m1, q3120
%endif
    pmaddwd     m0, m0
    pmaddwd     m1, m1
    paddd       m2, m0
    paddd       m3, m1
    add         r6, 2*mmsize
    jl .loopx
%if mmsize == 32 ; avx2 may overread by 32 bytes, that has to be handled
    jz .no_overread
    psubd       m3, m1
.no_overread:
%endif
%if mmsize >= 16 ; using HADDD would remove the mmsize/32 part from the
                 ; equation above, putting the width limit at 8208
    punpckhdq   m0, m2, m6
    punpckhdq   m1, m3, m6
    punpckldq   m2, m6
    punpckldq   m3, m6
    paddq       m3, m2
    paddq       m1, m0
    paddq       m4, m3
    paddq       m4, m1
%else ; unfortunately paddq is sse2
      ; emulate 48 bit precision for mmx2 instead
    mova        m0, m2
    mova        m1, m3
    punpcklwd   m2, m6
    punpcklwd   m3, m6
    punpckhwd   m0, m6
    punpckhwd   m1, m6
    paddd       m3, m2
    paddd       m1, m0
    paddd       m4, m3
    paddd       m5, m1
%endif
    add         r0, r1
    add         r2, r3
    dec        r5d
    jg .loopy
    mov         r3, r6m
    mov         r4, r7m
%if mmsize == 32
    vextracti128 xm0, m4, 1
    paddq      xm4, xm0
%endif
%if mmsize >= 16
    movq      [r3], xm4
    movhps    [r4], xm4
%else ; fixup for mmx2
    SBUTTERFLY dq, 4, 5, 0
    mova        m0, m4
    psrld       m4, 16
    paddd       m5, m4
    pslld       m0, 16
    SBUTTERFLY dq, 0, 5, 4
    psrlq       m0, 16
    psrlq       m5, 16
    movq      [r3], m0
    movq      [r4], m5
%endif
    RET
%endmacro ; SSD_NV12
%endif ; HIGH_BIT_DEPTH

%if HIGH_BIT_DEPTH == 0
;-----------------------------------------------------------------------------
; void pixel_ssd_nv12_core( uint8_t *pixuv1, intptr_t stride1, uint8_t *pixuv2, intptr_t stride2,
;                           int width, int height, uint64_t *ssd_u, uint64_t *ssd_v )
;
; This implementation can potentially overflow on image widths >= 11008 (or
; 6604 if interlaced), since it is called on blocks of height up to 12 (resp
; 20). At sane distortion levels it will take much more than that though.
;-----------------------------------------------------------------------------
%macro SSD_NV12 0
cglobal pixel_ssd_nv12_core, 6,7
    add    r4d, r4d
    add     r0, r4
    add     r2, r4
    pxor    m3, m3
    pxor    m4, m4
    mova    m5, [pw_00ff]
.loopy:
    mov     r6, r4
    neg     r6
.loopx:
%if mmsize == 32 ; only 16-byte alignment is guaranteed
    movu    m2, [r0+r6]
    movu    m1, [r2+r6]
%else
    mova    m2, [r0+r6]
    mova    m1, [r2+r6]
%endif
    psubusb m0, m2, m1
    psubusb m1, m2
    por     m0, m1
    psrlw   m2, m0, 8
    pand    m0, m5
    pmaddwd m2, m2
    pmaddwd m0, m0
    paddd   m3, m0
    paddd   m4, m2
    add     r6, mmsize
    jl .loopx
%if mmsize == 32 ; avx2 may overread by 16 bytes, that has to be handled
    jz .no_overread
    pcmpeqb xm1, xm1
    pandn   m0, m1, m0 ; zero the lower half
    pandn   m2, m1, m2
    psubd   m3, m0
    psubd   m4, m2
.no_overread:
%endif
    add     r0, r1
    add     r2, r3
    dec    r5d
    jg .loopy
    mov     r3, r6m
    mov     r4, r7m
    HADDD   m3, m0
    HADDD   m4, m0
    pxor   xm0, xm0
    punpckldq xm3, xm0
    punpckldq xm4, xm0
    movq  [r3], xm3
    movq  [r4], xm4
    RET
%endmacro ; SSD_NV12
%endif ; !HIGH_BIT_DEPTH

INIT_MMX mmx2
SSD_NV12
INIT_XMM sse2
SSD_NV12
INIT_XMM avx
SSD_NV12
INIT_YMM avx2
SSD_NV12

;=============================================================================
; variance
;=============================================================================

%macro VAR_START 1
    pxor  m5, m5    ; sum
    pxor  m6, m6    ; sum squared
%if HIGH_BIT_DEPTH == 0
%if %1
    mova  m7, [pw_00ff]
%elif mmsize < 32
    pxor  m7, m7    ; zero
%endif
%endif ; !HIGH_BIT_DEPTH
%endmacro

%macro VAR_END 2
%if HIGH_BIT_DEPTH
%if mmsize == 8 && %1*%2 == 256
    HADDUW  m5, m2
%else
    HADDW   m5, m2
%endif
%else ; !HIGH_BIT_DEPTH
    HADDW   m5, m2
%endif ; HIGH_BIT_DEPTH
    HADDD   m6, m1
%if ARCH_X86_64
    punpckldq m5, m6
    movq   rax, m5
%else
    movd   eax, m5
    movd   edx, m6
%endif
    RET
%endmacro

%macro VAR_CORE 0
    paddw     m5, m0
    paddw     m5, m3
    paddw     m5, m1
    paddw     m5, m4
    pmaddwd   m0, m0
    pmaddwd   m3, m3
    pmaddwd   m1, m1
    pmaddwd   m4, m4
    paddd     m6, m0
    paddd     m6, m3
    paddd     m6, m1
    paddd     m6, m4
%endmacro

%macro VAR_2ROW 2
    mov      r2d, %2
.loop:
%if HIGH_BIT_DEPTH
    mova      m0, [r0]
    mova      m1, [r0+mmsize]
    mova      m3, [r0+%1]
    mova      m4, [r0+%1+mmsize]
%else ; !HIGH_BIT_DEPTH
    mova      m0, [r0]
    punpckhbw m1, m0, m7
    mova      m3, [r0+%1]
    mova      m4, m3
    punpcklbw m0, m7
%endif ; HIGH_BIT_DEPTH
%ifidn %1, r1
    lea       r0, [r0+%1*2]
%else
    add       r0, r1
%endif
%if HIGH_BIT_DEPTH == 0
    punpcklbw m3, m7
    punpckhbw m4, m7
%endif ; !HIGH_BIT_DEPTH
    VAR_CORE
    dec r2d
    jg .loop
%endmacro

;-----------------------------------------------------------------------------
; int pixel_var_wxh( uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
INIT_MMX mmx2
cglobal pixel_var_16x16, 2,3
    FIX_STRIDES r1
    VAR_START 0
    VAR_2ROW 8*SIZEOF_PIXEL, 16
    VAR_END 16, 16

cglobal pixel_var_8x16, 2,3
    FIX_STRIDES r1
    VAR_START 0
    VAR_2ROW r1, 8
    VAR_END 8, 16

cglobal pixel_var_8x8, 2,3
    FIX_STRIDES r1
    VAR_START 0
    VAR_2ROW r1, 4
    VAR_END 8, 8

%if HIGH_BIT_DEPTH
%macro VAR 0
cglobal pixel_var_16x16, 2,3,8
    FIX_STRIDES r1
    VAR_START 0
    VAR_2ROW r1, 8
    VAR_END 16, 16

cglobal pixel_var_8x8, 2,3,8
    lea       r2, [r1*3]
    VAR_START 0
    mova      m0, [r0]
    mova      m1, [r0+r1*2]
    mova      m3, [r0+r1*4]
    mova      m4, [r0+r2*2]
    lea       r0, [r0+r1*8]
    VAR_CORE
    mova      m0, [r0]
    mova      m1, [r0+r1*2]
    mova      m3, [r0+r1*4]
    mova      m4, [r0+r2*2]
    VAR_CORE
    VAR_END 8, 8
%endmacro ; VAR

INIT_XMM sse2
VAR
INIT_XMM avx
VAR
INIT_XMM xop
VAR
%endif ; HIGH_BIT_DEPTH

%if HIGH_BIT_DEPTH == 0
%macro VAR 0
cglobal pixel_var_16x16, 2,3,8
    VAR_START 1
    mov      r2d, 8
.loop:
    mova      m0, [r0]
    mova      m3, [r0+r1]
    DEINTB    1, 0, 4, 3, 7
    lea       r0, [r0+r1*2]
    VAR_CORE
    dec r2d
    jg .loop
    VAR_END 16, 16

cglobal pixel_var_8x8, 2,4,8
    VAR_START 1
    mov      r2d, 2
    lea       r3, [r1*3]
.loop:
    movh      m0, [r0]
    movh      m3, [r0+r1]
    movhps    m0, [r0+r1*2]
    movhps    m3, [r0+r3]
    DEINTB    1, 0, 4, 3, 7
    lea       r0, [r0+r1*4]
    VAR_CORE
    dec r2d
    jg .loop
    VAR_END 8, 8

cglobal pixel_var_8x16, 2,4,8
    VAR_START 1
    mov      r2d, 4
    lea       r3, [r1*3]
.loop:
    movh      m0, [r0]
    movh      m3, [r0+r1]
    movhps    m0, [r0+r1*2]
    movhps    m3, [r0+r3]
    DEINTB    1, 0, 4, 3, 7
    lea       r0, [r0+r1*4]
    VAR_CORE
    dec r2d
    jg .loop
    VAR_END 8, 16
%endmacro ; VAR

INIT_XMM sse2
VAR
INIT_XMM avx
VAR
INIT_XMM xop
VAR

INIT_YMM avx2
cglobal pixel_var_16x16, 2,4,7
    VAR_START 0
    mov      r2d, 4
    lea       r3, [r1*3]
.loop:
    pmovzxbw  m0, [r0]
    pmovzxbw  m3, [r0+r1]
    pmovzxbw  m1, [r0+r1*2]
    pmovzxbw  m4, [r0+r3]
    lea       r0, [r0+r1*4]
    VAR_CORE
    dec r2d
    jg .loop
    vextracti128 xm0, m5, 1
    vextracti128 xm1, m6, 1
    paddw  xm5, xm0
    paddd  xm6, xm1
    HADDW  xm5, xm2
    HADDD  xm6, xm1
%if ARCH_X86_64
    punpckldq xm5, xm6
    movq   rax, xm5
%else
    movd   eax, xm5
    movd   edx, xm6
%endif
    RET
%endif ; !HIGH_BIT_DEPTH

%macro VAR2_END 3
    HADDW   %2, xm1
    movd   r1d, %2
    imul   r1d, r1d
    HADDD   %3, xm1
    shr    r1d, %1
    movd   eax, %3
    movd  [r4], %3
    sub    eax, r1d  ; sqr - (sum * sum >> shift)
    RET
%endmacro

;-----------------------------------------------------------------------------
; int pixel_var2_8x8( pixel *, intptr_t, pixel *, intptr_t, int * )
;-----------------------------------------------------------------------------
%macro VAR2_8x8_MMX 2
cglobal pixel_var2_8x%1, 5,6
    FIX_STRIDES r1, r3
    VAR_START 0
    mov      r5d, %1
.loop:
%if HIGH_BIT_DEPTH
    mova      m0, [r0]
    mova      m1, [r0+mmsize]
    psubw     m0, [r2]
    psubw     m1, [r2+mmsize]
%else ; !HIGH_BIT_DEPTH
    movq      m0, [r0]
    movq      m1, m0
    movq      m2, [r2]
    movq      m3, m2
    punpcklbw m0, m7
    punpckhbw m1, m7
    punpcklbw m2, m7
    punpckhbw m3, m7
    psubw     m0, m2
    psubw     m1, m3
%endif ; HIGH_BIT_DEPTH
    paddw     m5, m0
    paddw     m5, m1
    pmaddwd   m0, m0
    pmaddwd   m1, m1
    paddd     m6, m0
    paddd     m6, m1
    add       r0, r1
    add       r2, r3
    dec       r5d
    jg .loop
    VAR2_END %2, m5, m6
%endmacro

%if ARCH_X86_64 == 0
INIT_MMX mmx2
VAR2_8x8_MMX  8, 6
VAR2_8x8_MMX 16, 7
%endif

%macro VAR2_8x8_SSE2 2
cglobal pixel_var2_8x%1, 5,6,8
    VAR_START 1
    mov      r5d, %1/2
.loop:
%if HIGH_BIT_DEPTH
    mova      m0, [r0]
    mova      m1, [r0+r1*2]
    mova      m2, [r2]
    mova      m3, [r2+r3*2]
%else ; !HIGH_BIT_DEPTH
    movq      m1, [r0]
    movhps    m1, [r0+r1]
    movq      m3, [r2]
    movhps    m3, [r2+r3]
    DEINTB    0, 1, 2, 3, 7
%endif ; HIGH_BIT_DEPTH
    psubw     m0, m2
    psubw     m1, m3
    paddw     m5, m0
    paddw     m5, m1
    pmaddwd   m0, m0
    pmaddwd   m1, m1
    paddd     m6, m0
    paddd     m6, m1
    lea       r0, [r0+r1*2*SIZEOF_PIXEL]
    lea       r2, [r2+r3*2*SIZEOF_PIXEL]
    dec      r5d
    jg .loop
    VAR2_END %2, m5, m6
%endmacro

INIT_XMM sse2
VAR2_8x8_SSE2  8, 6
VAR2_8x8_SSE2 16, 7

%if HIGH_BIT_DEPTH == 0
%macro VAR2_8x8_SSSE3 2
cglobal pixel_var2_8x%1, 5,6,8
    pxor      m5, m5    ; sum
    pxor      m6, m6    ; sum squared
    mova      m7, [hsub_mul]
    mov      r5d, %1/4
.loop:
    movq      m0, [r0]
    movq      m2, [r2]
    movq      m1, [r0+r1]
    movq      m3, [r2+r3]
    lea       r0, [r0+r1*2]
    lea       r2, [r2+r3*2]
    punpcklbw m0, m2
    punpcklbw m1, m3
    movq      m2, [r0]
    movq      m3, [r2]
    punpcklbw m2, m3
    movq      m3, [r0+r1]
    movq      m4, [r2+r3]
    punpcklbw m3, m4
    pmaddubsw m0, m7
    pmaddubsw m1, m7
    pmaddubsw m2, m7
    pmaddubsw m3, m7
    paddw     m5, m0
    paddw     m5, m1
    paddw     m5, m2
    paddw     m5, m3
    pmaddwd   m0, m0
    pmaddwd   m1, m1
    pmaddwd   m2, m2
    pmaddwd   m3, m3
    paddd     m6, m0
    paddd     m6, m1
    paddd     m6, m2
    paddd     m6, m3
    lea       r0, [r0+r1*2]
    lea       r2, [r2+r3*2]
    dec      r5d
    jg .loop
    VAR2_END %2, m5, m6
%endmacro

INIT_XMM ssse3
VAR2_8x8_SSSE3  8, 6
VAR2_8x8_SSSE3 16, 7
INIT_XMM xop
VAR2_8x8_SSSE3  8, 6
VAR2_8x8_SSSE3 16, 7

%macro VAR2_8x8_AVX2 2
cglobal pixel_var2_8x%1, 5,6,6
    pxor      m3, m3    ; sum
    pxor      m4, m4    ; sum squared
    mova      m5, [hsub_mul]
    mov      r5d, %1/4
.loop:
    movq     xm0, [r0]
    movq     xm1, [r2]
    vinserti128 m0, m0, [r0+r1], 1
    vinserti128 m1, m1, [r2+r3], 1
    lea       r0, [r0+r1*2]
    lea       r2, [r2+r3*2]
    punpcklbw m0, m1
    movq     xm1, [r0]
    movq     xm2, [r2]
    vinserti128 m1, m1, [r0+r1], 1
    vinserti128 m2, m2, [r2+r3], 1
    lea       r0, [r0+r1*2]
    lea       r2, [r2+r3*2]
    punpcklbw m1, m2
    pmaddubsw m0, m5
    pmaddubsw m1, m5
    paddw     m3, m0
    paddw     m3, m1
    pmaddwd   m0, m0
    pmaddwd   m1, m1
    paddd     m4, m0
    paddd     m4, m1
    dec      r5d
    jg .loop
    vextracti128 xm0, m3, 1
    vextracti128 xm1, m4, 1
    paddw    xm3, xm0
    paddd    xm4, xm1
    VAR2_END %2, xm3, xm4
%endmacro

INIT_YMM avx2
VAR2_8x8_AVX2  8, 6
VAR2_8x8_AVX2 16, 7

%endif ; !HIGH_BIT_DEPTH

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

%macro SATD_4x8_SSE 3
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
    SATD_8x4_SSE %1, 0, 1, 2, 3, 4, 5, 7, %3
%endmacro

;-----------------------------------------------------------------------------
; int pixel_satd_8x4( uint8_t *, intptr_t, uint8_t *, intptr_t )
;-----------------------------------------------------------------------------
%macro SATDS_SSE2 0
%define vertical ((notcpuflag(ssse3) || cpuflag(atom)) || HIGH_BIT_DEPTH)

%if vertical==0 || HIGH_BIT_DEPTH
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
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1, 0
    SATD_8x4_1_SSE vertical, 0, 1, 2, 3, 4, 5, 6, 12, 13
%else
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1, 0
    SATD_8x4_1_SSE vertical, 0, 1, 2, 3, 4, 5, 6, 4, 5
    LOAD_SUMSUB_8x4P 0, 1, 2, 3, 4, 5, 7, r0, r2, 1, 0
    SATD_8x4_1_SSE vertical, 0, 1, 2, 3, 4, 5, 6, 4, 5
%endif
    ret

; 16x8 regresses on phenom win64, 16x16 is almost the same (too many spilled registers)
; These aren't any faster on AVX systems with fast movddup (Bulldozer, Sandy Bridge)
%if HIGH_BIT_DEPTH == 0 && (WIN64 || UNIX64) && notcpuflag(avx)

cglobal pixel_satd_16x4_internal
    LOAD_SUMSUB_16x4P 0, 1, 2, 3, 4, 8, 5, 9, 6, 7, r0, r2, 11
    lea  r2, [r2+4*r3]
    lea  r0, [r0+4*r1]
    ; always use horizontal mode here
    SATD_8x4_SSE 0, 0, 1, 2, 3, 6, 11, 10
    SATD_8x4_SSE 0, 4, 8, 5, 9, 6, 3, 10
    ret

cglobal pixel_satd_16x4_internal2
    LOAD_SUMSUB_16x4P 0, 1, 2, 3, 4, 8, 5, 9, 6, 7, r0, r2, 11
    lea  r2, [r2+4*r3]
    lea  r0, [r0+4*r1]
    SATD_8x4_1_SSE 0, 0, 1, 2, 3, 6, 11, 10, 12, 13
    SATD_8x4_1_SSE 0, 4, 8, 5, 9, 6, 3, 10, 12, 13
    ret

cglobal pixel_satd_16x4, 4,6,12
    SATD_START_SSE2 m10, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal
    SATD_END_SSE2 m10

cglobal pixel_satd_16x8, 4,6,12
    SATD_START_SSE2 m10, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    jmp %%pixel_satd_16x8_internal

cglobal pixel_satd_16x12, 4,6,12
    SATD_START_SSE2 m10, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal
    jmp %%pixel_satd_16x8_internal
    SATD_END_SSE2 m10  

cglobal pixel_satd_16x32, 4,6,12
    SATD_START_SSE2 m10, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    jmp %%pixel_satd_16x8_internal
    SATD_END_SSE2 m10

cglobal pixel_satd_16x64, 4,6,12
    SATD_START_SSE2 m10, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    jmp %%pixel_satd_16x8_internal
    SATD_END_SSE2 m10    

cglobal pixel_satd_16x16, 4,6,12
    SATD_START_SSE2 m10, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
%%pixel_satd_16x8_internal:
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    SATD_END_SSE2 m10

cglobal pixel_satd_32x8, 4,8,8    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    SATD_END_SSE2 m10

cglobal pixel_satd_32x16, 4,8,8    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    SATD_END_SSE2 m10

cglobal pixel_satd_32x24, 4,8,8    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    SATD_END_SSE2 m10

cglobal pixel_satd_32x32, 4,8,8    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    SATD_END_SSE2 m10

cglobal pixel_satd_32x64, 4,8,8    ;if WIN64 && notcpuflag(avx)
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

cglobal pixel_satd_48x64, 4,8,8    ;if WIN64 && notcpuflag(avx)
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

cglobal pixel_satd_64x16, 4,8,8    ;if WIN64 && notcpuflag(avx)
    SATD_START_SSE2 m10, m7
    mov r6, r0
    mov r7, r2
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    lea r0, [r6 + 32]
    lea r2, [r7 + 32]
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    lea r0, [r6 + 48]
    lea r2, [r7 + 48]
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    call pixel_satd_16x4_internal
    SATD_END_SSE2 m10

cglobal pixel_satd_64x32, 4,8,8    ;if WIN64 && notcpuflag(avx)
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

cglobal pixel_satd_64x48, 4,8,8    ;if WIN64 && notcpuflag(avx)
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

cglobal pixel_satd_64x64, 4,8,8    ;if WIN64 && notcpuflag(avx)
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
cglobal pixel_satd_32x8, 4,8,8    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal
    lea r0, [r6 + 8]
    lea r2, [r7 + 8]
    call pixel_satd_8x8_internal
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_8x8_internal
    lea r0, [r6 + 24]
    lea r2, [r7 + 24]
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6
%else
cglobal pixel_satd_32x8, 4,7,8,0-4    ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal
    lea r0, [r6 + 8]
    mov r2, [rsp]
    add r2, 8
    call pixel_satd_8x8_internal
    lea r0, [r6 + 16]
    mov r2, [rsp]
    add r2, 16
    call pixel_satd_8x8_internal
    lea r0, [r6 + 24]
    mov r2, [rsp]
    add r2, 24
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6
%endif

%if WIN64
cglobal pixel_satd_32x16, 4,8,8    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 8]
    lea r2, [r7 + 8]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 24]
    lea r2, [r7 + 24]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6
%else
cglobal pixel_satd_32x16, 4,7,8,0-4    ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 8]
    mov r2, [rsp]
    add r2, 8
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 16]
    mov r2, [rsp]
    add r2, 16
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 24]
    mov r2, [rsp]
    add r2, 24
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6
%endif

%if WIN64
cglobal pixel_satd_32x24, 4,8,8    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 8]
    lea r2, [r7 + 8]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 24]
    lea r2, [r7 + 24]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6
%else
cglobal pixel_satd_32x24, 4,7,8,0-4    ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 8]
    mov r2, [rsp]
    add r2, 8
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 16]
    mov r2, [rsp]
    add r2, 16
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 24]
    mov r2, [rsp]
    add r2, 24
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6
%endif

%if WIN64
cglobal pixel_satd_32x32, 4,8,8    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 8]
    lea r2, [r7 + 8]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 24]
    lea r2, [r7 + 24]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6


%else   
cglobal pixel_satd_32x32, 4,7,8,0-4    ;if !WIN64

    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 8]
    mov r2, [rsp]
    add r2, 8
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 16]
    mov r2, [rsp]
    add r2, 16
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 24]
    mov r2, [rsp]
    add r2, 24
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6

%endif

%if WIN64
cglobal pixel_satd_32x64, 4,8,8    ;if WIN64 && cpuflag(avx)
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
    lea r0, [r6 + 8]
    lea r2, [r7 + 8]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24]
    lea r2, [r7 + 24]
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
cglobal pixel_satd_32x64, 4,7,8,0-4    ;if !WIN64
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
    lea r0, [r6 + 8]
    mov r2, [rsp]
    add r2, 8
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16]
    mov r2, [rsp]
    add r2, 16
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24]
    mov r2, [rsp]
    add r2, 24
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
cglobal pixel_satd_48x64, 4,8,8    ;if WIN64 && cpuflag(avx)
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
    lea r0, [r6 + 8]
    lea r2, [r7 + 8]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24]
    lea r2, [r7 + 24]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32]
    lea r2, [r7 + 32]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40]
    lea r2, [r7 + 40]
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
cglobal pixel_satd_48x64, 4,7,8,0-4    ;if !WIN64
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
    lea r0, [r6 + 8]
    mov r2, [rsp]
    add r2,8
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16]
    mov r2, [rsp]
    add r2,16
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24]
    mov r2, [rsp]
    add r2,24
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32]
    mov r2, [rsp]
    add r2,32
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40]
    mov r2, [rsp]
    add r2,40
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
cglobal pixel_satd_64x16, 4,8,8    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 8]
    lea r2, [r7 + 8]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 24]
    lea r2, [r7 + 24]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 32]
    lea r2, [r7 + 32]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 40]
    lea r2, [r7 + 40]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 48]
    lea r2, [r7 + 48]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 56]
    lea r2, [r7 + 56]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6
%else
cglobal pixel_satd_64x16, 4,7,8,0-4    ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 8]
    mov r2, [rsp]
    add r2,8
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 16]
    mov r2, [rsp]
    add r2,16
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 24]
    mov r2, [rsp]
    add r2,24
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 32]
    mov r2, [rsp]
    add r2,32
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 40]
    mov r2, [rsp]
    add r2,40
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 48]
    mov r2, [rsp]
    add r2,48
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 56]
    mov r2, [rsp]
    add r2,56
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6
%endif

%if WIN64
cglobal pixel_satd_64x32, 4,8,9    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8]
    lea r2, [r7 + 8]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24]
    lea r2, [r7 + 24]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32]
    lea r2, [r7 + 32]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40]
    lea r2, [r7 + 40]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48]
    lea r2, [r7 + 48]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56]
    lea r2, [r7 + 56]
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
cglobal pixel_satd_64x32, 4,7,8,0-4    ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8]
    mov r2, [rsp]
    add r2, 8
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16]
    mov r2, [rsp]
    add r2, 16
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24]
    mov r2, [rsp]
    add r2, 24
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32]
    mov r2, [rsp]
    add r2, 32
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40]
    mov r2, [rsp]
    add r2, 40
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48]
    mov r2, [rsp]
    add r2, 48
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56]
    mov r2, [rsp]
    add r2, 56
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
cglobal pixel_satd_64x48, 4,8,9    ;if WIN64 && cpuflag(avx)
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8]
    lea r2, [r7 + 8]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24]
    lea r2, [r7 + 24]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32]
    lea r2, [r7 + 32]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40]
    lea r2, [r7 + 40]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48]
    lea r2, [r7 + 48]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56]
    lea r2, [r7 + 56]
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
cglobal pixel_satd_64x48, 4,7,8,0-4    ;if !WIN64
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 8]
    mov r2, [rsp]
    add r2, 8
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16]
    mov r2, [rsp]
    add r2, 16
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24]
    mov r2, [rsp]
    add r2, 24
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32]
    mov r2, [rsp]
    add r2, 32
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40]
    mov r2, [rsp]
    add r2, 40
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48]
    mov r2, [rsp]
    add r2, 48
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56]
    mov r2, [rsp]
    add r2, 56
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
cglobal pixel_satd_64x64, 4,8,9    ;if WIN64 && cpuflag(avx)
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
    lea r0, [r6 + 8]
    lea r2, [r7 + 8]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24]
    lea r2, [r7 + 24]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32]
    lea r2, [r7 + 32]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40]
    lea r2, [r7 + 40]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48]
    lea r2, [r7 + 48]
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56]
    lea r2, [r7 + 56]
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
cglobal pixel_satd_64x64, 4,7,8,0-4    ;if !WIN64
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
    lea r0, [r6 + 8]
    mov r2, [rsp]
    add r2, 8
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 16]
    mov r2, [rsp]
    add r2, 16
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 24]
    mov r2, [rsp]
    add r2, 24
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 32]
    mov r2, [rsp]
    add r2, 32
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 40]
    mov r2, [rsp]
    add r2, 40
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 48]
    mov r2, [rsp]
    add r2, 48
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    call pixel_satd_8x8_internal2
    lea r0, [r6 + 56]
    mov r2, [rsp]
    add r2, 56
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

cglobal pixel_satd_16x4, 4,6,8
    SATD_START_SSE2 m6, m7
    BACKUP_POINTERS
    call %%pixel_satd_8x4_internal
    RESTORE_AND_INC_POINTERS
    call %%pixel_satd_8x4_internal
    SATD_END_SSE2 m6

cglobal pixel_satd_16x8, 4,6,8
    SATD_START_SSE2 m6, m7
    BACKUP_POINTERS
    call pixel_satd_8x8_internal
    RESTORE_AND_INC_POINTERS
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6

cglobal pixel_satd_16x12, 4,6,8
    SATD_START_SSE2 m6, m7, 1
    BACKUP_POINTERS
    call pixel_satd_8x8_internal
    call %%pixel_satd_8x4_internal
    SATD_ACCUM m6, m0, m7
    RESTORE_AND_INC_POINTERS
    call pixel_satd_8x8_internal
    call %%pixel_satd_8x4_internal
    SATD_END_SSE2 m6, m7

cglobal pixel_satd_16x16, 4,6,8
    SATD_START_SSE2 m6, m7, 1
    BACKUP_POINTERS
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_ACCUM m6, m0, m7
    RESTORE_AND_INC_POINTERS
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6, m7

cglobal pixel_satd_16x32, 4,6,8
    SATD_START_SSE2 m6, m7, 1
    BACKUP_POINTERS
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_ACCUM m6, m0, m7
    RESTORE_AND_INC_POINTERS
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6, m7

cglobal pixel_satd_16x64, 4,6,8
    SATD_START_SSE2 m6, m7, 1
    BACKUP_POINTERS
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_ACCUM m6, m0, m7
    RESTORE_AND_INC_POINTERS
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6, m7
%endif

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
    lea r0, [r6 + 4]
    lea r2, [r7 + 4]
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r6 + 8]
    lea r2, [r7 + 8]
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    HADDW m7, m1
    movd eax, m7
    RET
%else
cglobal pixel_satd_12x16, 4,7,8,0-4
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
    lea r0, [r6 + 4]
    mov r2, [rsp]
    add r2, 4
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r6 + 8]
    mov r2, [rsp]
    add r2, 8
    SATD_4x8_SSE vertical, 1, add
    lea r0, [r0 + r1*2*SIZEOF_PIXEL]
    lea r2, [r2 + r3*2*SIZEOF_PIXEL]
    SATD_4x8_SSE vertical, 1, add
    HADDW m7, m1
    movd eax, m7
    RET
%endif

%if WIN64
cglobal pixel_satd_24x32, 4,8,8
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov r7, r2
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 8]
    lea r2, [r7 + 8]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 16]
    lea r2, [r7 + 16]
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6
%else
cglobal pixel_satd_24x32, 4,7,8,0-4
    SATD_START_SSE2 m6, m7
    mov r6, r0
    mov [rsp], r2
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 8]
    mov r2, [rsp]
    add r2, 8
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    lea r0, [r6 + 16]
    mov r2, [rsp]
    add r2, 16
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6
%endif    ;WIN64

cglobal pixel_satd_8x32, 4,6,8
    SATD_START_SSE2 m6, m7
%if vertical
    mova m7, [pw_00ff]
%endif
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6

cglobal pixel_satd_8x16, 4,6,8
    SATD_START_SSE2 m6, m7
    call pixel_satd_8x8_internal
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6

cglobal pixel_satd_8x8, 4,6,8
    SATD_START_SSE2 m6, m7
    call pixel_satd_8x8_internal
    SATD_END_SSE2 m6

cglobal pixel_satd_8x4, 4,6,8
    SATD_START_SSE2 m6, m7
    call %%pixel_satd_8x4_internal
    SATD_END_SSE2 m6
%endmacro ; SATDS_SSE2

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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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

cglobal pixel_sa8d_8x16, 4,8,12
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

cglobal pixel_sa8d_8x32, 4,8,12
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

cglobal pixel_sa8d_16x8, 4,8,12
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    movd eax, m12
    RET

cglobal pixel_sa8d_16x32, 4,8,12
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

cglobal pixel_sa8d_16x64, 4,8,12
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

cglobal pixel_sa8d_24x32, 4,8,12
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    sub r0, 8
    sub r2, 8
    SA8D_8x8
    sub r0, 8
    sub r2, 8
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    sub r0, 8
    sub r2, 8
    SA8D_8x8
    sub r0, 8
    sub r2, 8
    SA8D_8x8
    movd eax, m12
    RET

cglobal pixel_sa8d_32x8, 4,8,12
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    movd eax, m12
    RET

cglobal pixel_sa8d_32x16, 4,8,12
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
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_32x24, 4,8,12
    FIX_STRIDES r1, r3
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    pxor m12, m12
%if vertical == 0
    mova m7, [hmul_8p]
%endif
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    sub r0, 8
    sub r2, 8
    SA8D_8x8
    sub r0, 8
    sub r2, 8
    SA8D_8x8
    sub r0, 8
    sub r2, 8
    SA8D_8x8
    lea r0, [r0 + r1*8]
    lea r2, [r2 + r3*8]
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    add r0, 8
    add r2, 8
    SA8D_8x8
    movd eax, m12
    RET

cglobal pixel_sa8d_32x32, 4,8,12
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
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_32x64, 4,8,12
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
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_48x64, 4,8,12
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
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_64x16, 4,8,12
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
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_64x32, 4,8,12
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
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_64x48, 4,8,12
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
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    movd eax, m12
    RET

cglobal pixel_sa8d_64x64, 4,8,12
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
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    add  r2, 16
    add  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea r0, [r0+8*r1]
    lea r2, [r2+8*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
    lea  r4, [3*r1]
    lea  r5, [3*r3]
    SA8D_16x16
    lea  r4, [8*r1]
    lea  r5, [8*r3]
    sub  r2, r4
    sub  r0, r5
    sub  r2, 16
    sub  r0, 16
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov dword [esp+36], r4d

    mov  r0, [r6+20]
    mov  r2, [r6+28]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r0, [r0 + r1*8]
    lea  r2, [r2 + r3*8]
    lea  r4, [r1 + 2*r1]
    call pixel_sa8d_8x8_internal2
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
    movd r4d, m0
    add  r4d, 1
    shr  r4d, 1
    add r4d, dword [esp+36]
    mov dword [esp+36], r4d

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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
    paddusw m0, [esp+48]
    HADDUW m0, m1
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
; SA8D_SATD
;=============================================================================

; %1: vertical/horizontal mode
; %2-%5: sa8d output regs (m0,m1,m2,m3,m4,m5,m8,m9)
; m10: satd result
; m6, m11-15: tmp regs
%macro SA8D_SATD_8x4 5
%if %1
    LOAD_DIFF_8x4P %2, %3, %4, %5, 6, 11, 7, r0, r2, 1
    HADAMARD   0, sumsub, %2, %3, 6
    HADAMARD   0, sumsub, %4, %5, 6
    SBUTTERFLY        wd, %2, %3, 6
    SBUTTERFLY        wd, %4, %5, 6
    HADAMARD2_2D  %2, %4, %3, %5, 6, dq

    mova   m12, m%2
    mova   m13, m%3
    mova   m14, m%4
    mova   m15, m%5
    HADAMARD 0, sumsub, %2, %3, 6
    HADAMARD 0, sumsub, %4, %5, 6
    SBUTTERFLY     qdq, 12, 13, 6
    HADAMARD   0, amax, 12, 13, 6
    SBUTTERFLY     qdq, 14, 15, 6
    paddw m10, m12
    HADAMARD   0, amax, 14, 15, 6
    paddw m10, m14
%else
    LOAD_SUMSUB_8x4P %2, %3, %4, %5, 6, 11, 7, r0, r2, 1
    HADAMARD4_V %2, %3, %4, %5, 6

    pabsw    m12, m%2 ; doing the abs first is a slight advantage
    pabsw    m14, m%4
    pabsw    m13, m%3
    pabsw    m15, m%5
    HADAMARD 1, max, 12, 14, 6, 11
    paddw    m10, m12
    HADAMARD 1, max, 13, 15, 6, 11
    paddw    m10, m13
%endif
%endmacro ; SA8D_SATD_8x4

; %1: add spilled regs?
; %2: spill regs?
%macro SA8D_SATD_ACCUM 2
%if HIGH_BIT_DEPTH
    pmaddwd m10, [pw_1]
    HADDUWD  m0, m1
%if %1
    paddd   m10, temp1
    paddd    m0, temp0
%endif
%if %2
    mova  temp1, m10
    pxor    m10, m10
%endif
%elif %1
    paddw    m0, temp0
%endif
%if %2
    mova  temp0, m0
%endif
%endmacro

%macro SA8D_SATD 0
%define vertical ((notcpuflag(ssse3) || cpuflag(atom)) || HIGH_BIT_DEPTH)
cglobal pixel_sa8d_satd_8x8_internal
    SA8D_SATD_8x4 vertical, 0, 1, 2, 3
    SA8D_SATD_8x4 vertical, 4, 5, 8, 9

%if vertical ; sse2-style
    HADAMARD2_2D 0, 4, 2, 8, 6, qdq, amax
    HADAMARD2_2D 1, 5, 3, 9, 6, qdq, amax
%else        ; complete sa8d
    SUMSUB_BADC w, 0, 4, 1, 5, 12
    HADAMARD 2, sumsub, 0, 4, 12, 11
    HADAMARD 2, sumsub, 1, 5, 12, 11
    SUMSUB_BADC w, 2, 8, 3, 9, 12
    HADAMARD 2, sumsub, 2, 8, 12, 11
    HADAMARD 2, sumsub, 3, 9, 12, 11
    HADAMARD 1, amax, 0, 4, 12, 11
    HADAMARD 1, amax, 1, 5, 12, 4
    HADAMARD 1, amax, 2, 8, 12, 4
    HADAMARD 1, amax, 3, 9, 12, 4
%endif

    ; create sa8d sub results
    paddw    m1, m2
    paddw    m0, m3
    paddw    m0, m1

    SAVE_MM_PERMUTATION
    ret

;-------------------------------------------------------------------------------
; uint64_t pixel_sa8d_satd_16x16( pixel *, intptr_t, pixel *, intptr_t )
;-------------------------------------------------------------------------------
cglobal pixel_sa8d_satd_16x16, 4,8-(mmsize/32),16,SIZEOF_PIXEL*mmsize
    %define temp0 [rsp+0*mmsize]
    %define temp1 [rsp+1*mmsize]
    FIX_STRIDES r1, r3
%if vertical==0
    mova     m7, [hmul_8p]
%endif
    lea      r4, [3*r1]
    lea      r5, [3*r3]
    pxor    m10, m10

%if mmsize==32
    call pixel_sa8d_satd_8x8_internal
    SA8D_SATD_ACCUM 0, 1
    call pixel_sa8d_satd_8x8_internal
    SA8D_SATD_ACCUM 1, 0
    vextracti128 xm1, m0, 1
    vextracti128 xm2, m10, 1
    paddw   xm0, xm1
    paddw  xm10, xm2
%else
    lea      r6, [r2+8*SIZEOF_PIXEL]
    lea      r7, [r0+8*SIZEOF_PIXEL]

    call pixel_sa8d_satd_8x8_internal
    SA8D_SATD_ACCUM 0, 1
    call pixel_sa8d_satd_8x8_internal
    SA8D_SATD_ACCUM 1, 1

    mov      r0, r7
    mov      r2, r6

    call pixel_sa8d_satd_8x8_internal
    SA8D_SATD_ACCUM 1, 1
    call pixel_sa8d_satd_8x8_internal
    SA8D_SATD_ACCUM 1, 0
%endif

; xop already has fast horizontal sums
%if cpuflag(sse4) && notcpuflag(xop) && HIGH_BIT_DEPTH==0
    pmaddwd xm10, [pw_1]
    HADDUWD xm0, xm1
    phaddd  xm0, xm10       ;  sa8d1  sa8d2  satd1  satd2
    pshufd  xm1, xm0, q2301 ;  sa8d2  sa8d1  satd2  satd1
    paddd   xm0, xm1        ;   sa8d   sa8d   satd   satd
    movd    r0d, xm0
    pextrd  eax, xm0, 2
%else
%if HIGH_BIT_DEPTH
    HADDD   xm0, xm1
    HADDD  xm10, xm2
%else
    HADDUW  xm0, xm1
    HADDW  xm10, xm2
%endif
    movd    r0d, xm0
    movd    eax, xm10
%endif
    add     r0d, 1
    shl     rax, 32
    shr     r0d, 1
    or      rax, r0
    RET
%endmacro ; SA8D_SATD

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
%if ARCH_X86_64
SA8D_SATD
%endif

%if HIGH_BIT_DEPTH == 0
INIT_XMM ssse3,atom
SATDS_SSE2
SA8D
%if ARCH_X86_64
SA8D_SATD
%endif
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
%if ARCH_X86_64
SA8D_SATD
%endif
%undef movdqa ; nehalem doesn't like movaps
%undef movdqu ; movups
%undef punpcklqdq ; or movlhps

%define TRANS TRANS_SSE4
%define LOAD_DUP_4x8P LOAD_DUP_4x8P_PENRYN
INIT_XMM sse4
SATDS_SSE2
SA8D
%if ARCH_X86_64
SA8D_SATD
%endif

; Sandy/Ivy Bridge and Bulldozer do movddup in the load unit, so
; it's effectively free.
%define LOAD_DUP_4x8P LOAD_DUP_4x8P_CONROE
INIT_XMM avx
SATDS_SSE2
SA8D
%if ARCH_X86_64
SA8D_SATD
%endif

%define TRANS TRANS_XOP
INIT_XMM xop
SATDS_SSE2
SA8D
%if ARCH_X86_64
SA8D_SATD
%endif


%if HIGH_BIT_DEPTH == 0
%define LOAD_SUMSUB_8x4P LOAD_SUMSUB8_16x4P_AVX2
%define LOAD_DUP_4x8P LOAD_DUP_4x16P_AVX2
%define TRANS TRANS_SSE4
%if ARCH_X86_64
SA8D_SATD
%endif

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

;=============================================================================
; SSIM
;=============================================================================

;-----------------------------------------------------------------------------
; void pixel_ssim_4x4x2_core( const uint8_t *pix1, intptr_t stride1,
;                             const uint8_t *pix2, intptr_t stride2, int sums[2][4] )
;-----------------------------------------------------------------------------
%macro SSIM_ITER 1
%if HIGH_BIT_DEPTH
    movdqu    m5, [r0+(%1&1)*r1]
    movdqu    m6, [r2+(%1&1)*r3]
%else
    movq      m5, [r0+(%1&1)*r1]
    movq      m6, [r2+(%1&1)*r3]
    punpcklbw m5, m0
    punpcklbw m6, m0
%endif
%if %1==1
    lea       r0, [r0+r1*2]
    lea       r2, [r2+r3*2]
%endif
%if %1==0
    movdqa    m1, m5
    movdqa    m2, m6
%else
    paddw     m1, m5
    paddw     m2, m6
%endif
    pmaddwd   m7, m5, m6
    pmaddwd   m5, m5
    pmaddwd   m6, m6
    ACCUM  paddd, 3, 5, %1
    ACCUM  paddd, 4, 7, %1
    paddd     m3, m6
%endmacro

%macro SSIM 0
cglobal pixel_ssim_4x4x2_core, 4,4,8
    FIX_STRIDES r1, r3
    pxor      m0, m0
    SSIM_ITER 0
    SSIM_ITER 1
    SSIM_ITER 2
    SSIM_ITER 3
    ; PHADDW m1, m2
    ; PHADDD m3, m4
    movdqa    m7, [pw_1]
    pshufd    m5, m3, q2301
    pmaddwd   m1, m7
    pmaddwd   m2, m7
    pshufd    m6, m4, q2301
    packssdw  m1, m2
    paddd     m3, m5
    pshufd    m1, m1, q3120
    paddd     m4, m6
    pmaddwd   m1, m7
    punpckhdq m5, m3, m4
    punpckldq m3, m4

%if UNIX64
    %define t0 r4
%else
    %define t0 rax
    mov t0, r4mp
%endif

    movq      [t0+ 0], m1
    movq      [t0+ 8], m3
    movhps    [t0+16], m1
    movq      [t0+24], m5
    RET

;-----------------------------------------------------------------------------
; float pixel_ssim_end( int sum0[5][4], int sum1[5][4], int width )
;-----------------------------------------------------------------------------
cglobal pixel_ssim_end4, 3,3,7
    movdqa    m0, [r0+ 0]
    movdqa    m1, [r0+16]
    movdqa    m2, [r0+32]
    movdqa    m3, [r0+48]
    movdqa    m4, [r0+64]
    paddd     m0, [r1+ 0]
    paddd     m1, [r1+16]
    paddd     m2, [r1+32]
    paddd     m3, [r1+48]
    paddd     m4, [r1+64]
    paddd     m0, m1
    paddd     m1, m2
    paddd     m2, m3
    paddd     m3, m4
    movdqa    m5, [ssim_c1]
    movdqa    m6, [ssim_c2]
    TRANSPOSE4x4D  0, 1, 2, 3, 4

;   s1=m0, s2=m1, ss=m2, s12=m3
%if BIT_DEPTH == 10
    cvtdq2ps  m0, m0
    cvtdq2ps  m1, m1
    cvtdq2ps  m2, m2
    cvtdq2ps  m3, m3
    mulps     m2, [pf_64] ; ss*64
    mulps     m3, [pf_128] ; s12*128
    movdqa    m4, m1
    mulps     m4, m0      ; s1*s2
    mulps     m1, m1      ; s2*s2
    mulps     m0, m0      ; s1*s1
    addps     m4, m4      ; s1*s2*2
    addps     m0, m1      ; s1*s1 + s2*s2
    subps     m2, m0      ; vars
    subps     m3, m4      ; covar*2
    addps     m4, m5      ; s1*s2*2 + ssim_c1
    addps     m0, m5      ; s1*s1 + s2*s2 + ssim_c1
    addps     m2, m6      ; vars + ssim_c2
    addps     m3, m6      ; covar*2 + ssim_c2
%else
    pmaddwd   m4, m1, m0  ; s1*s2
    pslld     m1, 16
    por       m0, m1
    pmaddwd   m0, m0  ; s1*s1 + s2*s2
    pslld     m4, 1
    pslld     m3, 7
    pslld     m2, 6
    psubd     m3, m4  ; covar*2
    psubd     m2, m0  ; vars
    paddd     m0, m5
    paddd     m4, m5
    paddd     m3, m6
    paddd     m2, m6
    cvtdq2ps  m0, m0  ; (float)(s1*s1 + s2*s2 + ssim_c1)
    cvtdq2ps  m4, m4  ; (float)(s1*s2*2 + ssim_c1)
    cvtdq2ps  m3, m3  ; (float)(covar*2 + ssim_c2)
    cvtdq2ps  m2, m2  ; (float)(vars + ssim_c2)
%endif
    mulps     m4, m3
    mulps     m0, m2
    divps     m4, m0  ; ssim

    cmp       r2d, 4
    je .skip ; faster only if this is the common case; remove branch if we use ssim on a macroblock level
    neg       r2
%ifdef PIC
    lea       r3, [mask_ff + 16]
    movdqu    m1, [r3 + r2*4]
%else
    movdqu    m1, [mask_ff + r2*4 + 16]
%endif
    pand      m4, m1
.skip:
    movhlps   m0, m4
    addps     m0, m4
    pshuflw   m4, m0, q0032
    addss     m0, m4
%if ARCH_X86_64 == 0
    movd     r0m, m0
    fld     dword r0m
%endif
    RET
%endmacro ; SSIM

INIT_XMM sse2
SSIM
INIT_XMM avx
SSIM

;-----------------------------------------------------------------------------
; int pixel_asd8( pixel *pix1, intptr_t stride1, pixel *pix2, intptr_t stride2, int height );
;-----------------------------------------------------------------------------
%macro ASD8 0
cglobal pixel_asd8, 5,5
    pxor     m0, m0
    pxor     m1, m1
.loop:
%if HIGH_BIT_DEPTH
    paddw    m0, [r0]
    paddw    m1, [r2]
    paddw    m0, [r0+2*r1]
    paddw    m1, [r2+2*r3]
    lea      r0, [r0+4*r1]
    paddw    m0, [r0]
    paddw    m1, [r2+4*r3]
    lea      r2, [r2+4*r3]
    paddw    m0, [r0+2*r1]
    paddw    m1, [r2+2*r3]
    lea      r0, [r0+4*r1]
    lea      r2, [r2+4*r3]
%else
    movq     m2, [r0]
    movq     m3, [r2]
    movhps   m2, [r0+r1]
    movhps   m3, [r2+r3]
    lea      r0, [r0+2*r1]
    psadbw   m2, m1
    psadbw   m3, m1
    movq     m4, [r0]
    movq     m5, [r2+2*r3]
    lea      r2, [r2+2*r3]
    movhps   m4, [r0+r1]
    movhps   m5, [r2+r3]
    lea      r0, [r0+2*r1]
    paddw    m0, m2
    psubw    m0, m3
    psadbw   m4, m1
    psadbw   m5, m1
    lea      r2, [r2+2*r3]
    paddw    m0, m4
    psubw    m0, m5
%endif
    sub     r4d, 4
    jg .loop
%if HIGH_BIT_DEPTH
    psubw    m0, m1
    HADDW    m0, m1
    ABSD     m1, m0
%else
    movhlps  m1, m0
    paddw    m0, m1
    ABSW     m1, m0
%endif
    movd    eax, m1
    RET
%endmacro

INIT_XMM sse2
ASD8
INIT_XMM ssse3
ASD8
%if HIGH_BIT_DEPTH
INIT_XMM xop
ASD8
%endif

;=============================================================================
; Successive Elimination ADS
;=============================================================================

%macro ADS_START 0
%if UNIX64
    movsxd  r5, r5d
%else
    mov    r5d, r5m
%endif
    mov    r0d, r5d
    lea     r6, [r4+r5+(mmsize-1)]
    and     r6, ~(mmsize-1)
    shl     r2d,  1
%endmacro

%macro ADS_END 1 ; unroll_size
    add     r1, 8*%1
    add     r3, 8*%1
    add     r6, 4*%1
    sub    r0d, 4*%1
    jg .loop
    WIN64_RESTORE_XMM rsp
%if mmsize==32
    vzeroupper
%endif
    lea     r6, [r4+r5+(mmsize-1)]
    and     r6, ~(mmsize-1)
%if cpuflag(ssse3)
    jmp ads_mvs_ssse3
%else
    jmp ads_mvs_mmx
%endif
%endmacro

;-----------------------------------------------------------------------------
; int pixel_ads4( int enc_dc[4], uint16_t *sums, int delta,
;                 uint16_t *cost_mvx, int16_t *mvs, int width, int thresh )
;-----------------------------------------------------------------------------
INIT_MMX mmx2
cglobal pixel_ads4, 5,7
    mova    m6, [r0]
    mova    m4, [r0+8]
    pshufw  m7, m6, 0
    pshufw  m6, m6, q2222
    pshufw  m5, m4, 0
    pshufw  m4, m4, q2222
    ADS_START
.loop:
    movu      m0, [r1]
    movu      m1, [r1+16]
    psubw     m0, m7
    psubw     m1, m6
    ABSW      m0, m0, m2
    ABSW      m1, m1, m3
    movu      m2, [r1+r2]
    movu      m3, [r1+r2+16]
    psubw     m2, m5
    psubw     m3, m4
    paddw     m0, m1
    ABSW      m2, m2, m1
    ABSW      m3, m3, m1
    paddw     m0, m2
    paddw     m0, m3
    pshufw    m1, r6m, 0
    paddusw   m0, [r3]
    psubusw   m1, m0
    packsswb  m1, m1
    movd    [r6], m1
    ADS_END 1

cglobal pixel_ads2, 5,7
    mova      m6, [r0]
    pshufw    m5, r6m, 0
    pshufw    m7, m6, 0
    pshufw    m6, m6, q2222
    ADS_START
.loop:
    movu      m0, [r1]
    movu      m1, [r1+r2]
    psubw     m0, m7
    psubw     m1, m6
    ABSW      m0, m0, m2
    ABSW      m1, m1, m3
    paddw     m0, m1
    paddusw   m0, [r3]
    mova      m4, m5
    psubusw   m4, m0
    packsswb  m4, m4
    movd    [r6], m4
    ADS_END 1

cglobal pixel_ads1, 5,7
    pshufw    m7, [r0], 0
    pshufw    m6, r6m, 0
    ADS_START
.loop:
    movu      m0, [r1]
    movu      m1, [r1+8]
    psubw     m0, m7
    psubw     m1, m7
    ABSW      m0, m0, m2
    ABSW      m1, m1, m3
    paddusw   m0, [r3]
    paddusw   m1, [r3+8]
    mova      m4, m6
    mova      m5, m6
    psubusw   m4, m0
    psubusw   m5, m1
    packsswb  m4, m5
    mova    [r6], m4
    ADS_END 2

%macro ADS_XMM 0
%if mmsize==32
cglobal pixel_ads4, 5,7,8
    vpbroadcastw m7, [r0+ 0]
    vpbroadcastw m6, [r0+ 4]
    vpbroadcastw m5, [r0+ 8]
    vpbroadcastw m4, [r0+12]
%else
cglobal pixel_ads4, 5,7,12
    mova      m4, [r0]
    pshuflw   m7, m4, q0000
    pshuflw   m6, m4, q2222
    pshufhw   m5, m4, q0000
    pshufhw   m4, m4, q2222
    punpcklqdq m7, m7
    punpcklqdq m6, m6
    punpckhqdq m5, m5
    punpckhqdq m4, m4
%endif
%if ARCH_X86_64 && mmsize == 16
    movd      m8, r6m
    SPLATW    m8, m8
    ADS_START
    movu     m10, [r1]
    movu     m11, [r1+r2]
.loop:
    psubw     m0, m10, m7
    movu     m10, [r1+16]
    psubw     m1, m10, m6
    ABSW      m0, m0, m2
    ABSW      m1, m1, m3
    psubw     m2, m11, m5
    movu     m11, [r1+r2+16]
    paddw     m0, m1
    psubw     m3, m11, m4
    movu      m9, [r3]
    ABSW      m2, m2, m1
    ABSW      m3, m3, m1
    paddw     m0, m2
    paddw     m0, m3
    paddusw   m0, m9
    psubusw   m1, m8, m0
%else
    ADS_START
.loop:
    movu      m0, [r1]
    movu      m1, [r1+16]
    psubw     m0, m7
    psubw     m1, m6
    ABSW      m0, m0, m2
    ABSW      m1, m1, m3
    movu      m2, [r1+r2]
    movu      m3, [r1+r2+16]
    psubw     m2, m5
    psubw     m3, m4
    paddw     m0, m1
    ABSW      m2, m2, m1
    ABSW      m3, m3, m1
    paddw     m0, m2
    paddw     m0, m3
    movu      m2, [r3]
%if mmsize==32
    vpbroadcastw m1, r6m
%else
    movd      m1, r6m
    pshuflw   m1, m1, 0
    punpcklqdq m1, m1
%endif
    paddusw   m0, m2
    psubusw   m1, m0
%endif ; ARCH
    packsswb  m1, m1
%if mmsize==32
    vpermq    m1, m1, q3120
    mova    [r6], xm1
%else
    movh    [r6], m1
%endif
    ADS_END mmsize/8

cglobal pixel_ads2, 5,7,8
%if mmsize==32
    vpbroadcastw m7, [r0+0]
    vpbroadcastw m6, [r0+4]
    vpbroadcastw m5, r6m
%else
    movq      m6, [r0]
    movd      m5, r6m
    pshuflw   m7, m6, 0
    pshuflw   m6, m6, q2222
    pshuflw   m5, m5, 0
    punpcklqdq m7, m7
    punpcklqdq m6, m6
    punpcklqdq m5, m5
%endif
    ADS_START
.loop:
    movu      m0, [r1]
    movu      m1, [r1+r2]
    psubw     m0, m7
    psubw     m1, m6
    movu      m4, [r3]
    ABSW      m0, m0, m2
    ABSW      m1, m1, m3
    paddw     m0, m1
    paddusw   m0, m4
    psubusw   m1, m5, m0
    packsswb  m1, m1
%if mmsize==32
    vpermq    m1, m1, q3120
    mova    [r6], xm1
%else
    movh    [r6], m1
%endif
    ADS_END mmsize/8

cglobal pixel_ads1, 5,7,8
%if mmsize==32
    vpbroadcastw m7, [r0]
    vpbroadcastw m6, r6m
%else
    movd      m7, [r0]
    movd      m6, r6m
    pshuflw   m7, m7, 0
    pshuflw   m6, m6, 0
    punpcklqdq m7, m7
    punpcklqdq m6, m6
%endif
    ADS_START
.loop:
    movu      m0, [r1]
    movu      m1, [r1+mmsize]
    psubw     m0, m7
    psubw     m1, m7
    movu      m2, [r3]
    movu      m3, [r3+mmsize]
    ABSW      m0, m0, m4
    ABSW      m1, m1, m5
    paddusw   m0, m2
    paddusw   m1, m3
    psubusw   m4, m6, m0
    psubusw   m5, m6, m1
    packsswb  m4, m5
%if mmsize==32
    vpermq    m4, m4, q3120
%endif
    mova    [r6], m4
    ADS_END mmsize/4
%endmacro

INIT_XMM sse2
ADS_XMM
INIT_XMM ssse3
ADS_XMM
INIT_XMM avx
ADS_XMM
INIT_YMM avx2
ADS_XMM

; int pixel_ads_mvs( int16_t *mvs, uint8_t *masks, int width )
; {
;     int nmv=0, i, j;
;     *(uint32_t*)(masks+width) = 0;
;     for( i=0; i<width; i+=8 )
;     {
;         uint64_t mask = *(uint64_t*)(masks+i);
;         if( !mask ) continue;
;         for( j=0; j<8; j++ )
;             if( mask & (255<<j*8) )
;                 mvs[nmv++] = i+j;
;     }
;     return nmv;
; }

%macro TEST 1
    mov     [r4+r0*2], r1w
    test    r2d, 0xff<<(%1*8)
    setne   r3b
    add     r0d, r3d
    inc     r1d
%endmacro

INIT_MMX mmx
cglobal pixel_ads_mvs, 0,7,0
ads_mvs_mmx:
    ; mvs = r4
    ; masks = r6
    ; width = r5
    ; clear last block in case width isn't divisible by 8. (assume divisible by 4, so clearing 4 bytes is enough.)
    xor     r0d, r0d
    xor     r1d, r1d
    mov     [r6+r5], r0d
    jmp .loopi
ALIGN 16
.loopi0:
    add     r1d, 8
    cmp     r1d, r5d
    jge .end
.loopi:
    mov     r2,  [r6+r1]
%if ARCH_X86_64
    test    r2,  r2
%else
    mov     r3,  r2
    add    r3d, [r6+r1+4]
%endif
    jz .loopi0
    xor     r3d, r3d
    TEST 0
    TEST 1
    TEST 2
    TEST 3
%if ARCH_X86_64
    shr     r2,  32
%else
    mov     r2d, [r6+r1]
%endif
    TEST 0
    TEST 1
    TEST 2
    TEST 3
    cmp     r1d, r5d
    jl .loopi
.end:
    movifnidn eax, r0d
    RET

INIT_XMM ssse3
cglobal pixel_ads_mvs, 0,7,0
ads_mvs_ssse3:
    mova      m3, [pw_8]
    mova      m4, [pw_76543210]
    pxor      m5, m5
    add       r5, r6
    xor      r0d, r0d ; nmv
    mov     [r5], r0d
%ifdef PIC
    lea       r1, [$$]
    %define GLOBAL +r1-$$
%else
    %define GLOBAL
%endif
.loop:
    movh      m0, [r6]
    pcmpeqb   m0, m5
    pmovmskb r2d, m0
    xor      r2d, 0xffff                         ; skipping if r2d is zero is slower (branch mispredictions)
    movzx    r3d, byte [r2+popcnt_table GLOBAL]  ; popcnt
    add      r2d, r2d
    ; shuffle counters based on mv mask
    pshufb    m2, m4, [r2*8+ads_mvs_shuffle GLOBAL]
    movu [r4+r0*2], m2
    add      r0d, r3d
    paddw     m4, m3                             ; {i*8+0, i*8+1, i*8+2, i*8+3, i*8+4, i*8+5, i*8+6, i*8+7}
    add       r6, 8
    cmp       r6, r5
    jl .loop
    movifnidn eax, r0d
    RET

;-----------------------------------------------------------------
; void transpose_4x4(pixel *dst, pixel *src, intptr_t stride)
;-----------------------------------------------------------------
INIT_XMM sse2
cglobal transpose4, 3, 3, 4, dest, src, stride

    movd         m0,    [r1]
    movd         m1,    [r1 + r2]
    movd         m2,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movd         m3,    [r1 + r2]

    punpcklbw    m0,    m1
    punpcklbw    m2,    m3
    punpcklwd    m0,    m2
    movu         [r0],    m0

    RET

;-----------------------------------------------------------------
; void transpose_8x8(pixel *dst, pixel *src, intptr_t stride)
;-----------------------------------------------------------------
INIT_XMM sse2
cglobal transpose8, 3, 3, 8, dest, src, stride

    movh         m0,    [r1]
    movh         m1,    [r1 + r2]
    movh         m2,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movh         m3,    [r1 + r2]
    movh         m4,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movh         m5,    [r1 + r2]
    movh         m6,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movh         m7,    [r1 + r2]

    punpcklbw    m0,    m1
    punpcklbw    m2,    m3
    punpcklbw    m4,    m5
    punpcklbw    m6,    m7

    punpckhwd    m1,    m0,    m2
    punpcklwd    m0,    m2
    punpckhwd    m5,    m4,    m6
    punpcklwd    m4,    m6
    punpckhdq    m2,    m0,    m4
    punpckldq    m0,    m4
    punpckhdq    m3,    m1,    m5
    punpckldq    m1,    m5

    movu         [r0],         m0
    movu         [r0 + 16],    m2
    movu         [r0 + 32],    m1
    movu         [r0 + 48],    m3

    RET

%macro TRANSPOSE_8x8 1

    movh         m0,    [r1]
    movh         m1,    [r1 + r2]
    movh         m2,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movh         m3,    [r1 + r2]
    movh         m4,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movh         m5,    [r1 + r2]
    movh         m6,    [r1 + 2 * r2]
    lea          r1,    [r1 + 2 * r2]
    movh         m7,    [r1 + r2]

    punpcklbw    m0,    m1
    punpcklbw    m2,    m3
    punpcklbw    m4,    m5
    punpcklbw    m6,    m7

    punpckhwd    m1,    m0,    m2
    punpcklwd    m0,    m2
    punpckhwd    m5,    m4,    m6
    punpcklwd    m4,    m6
    punpckhdq    m2,    m0,    m4
    punpckldq    m0,    m4
    punpckhdq    m3,    m1,    m5
    punpckldq    m1,    m5

    movlps         [r0],             m0
    movhps         [r0 + %1],        m0
    movlps         [r0 + 2 * %1],    m2
    lea            r0,               [r0 + 2 * %1]
    movhps         [r0 + %1],        m2
    movlps         [r0 + 2 * %1],    m1
    lea            r0,               [r0 + 2 * %1]
    movhps         [r0 + %1],        m1
    movlps         [r0 + 2 * %1],    m3
    lea            r0,               [r0 + 2 * %1]
    movhps         [r0 + %1],        m3

%endmacro


;-----------------------------------------------------------------
; void transpose_16x16(pixel *dst, pixel *src, intptr_t stride)
;-----------------------------------------------------------------
INIT_XMM sse2
cglobal transpose16, 3, 5, 8, dest, src, stride

    mov    r3,    r0
    mov    r4,    r1
    TRANSPOSE_8x8 16
    lea    r1,    [r1 + 2 * r2]
    lea    r0,    [r3 + 8]
    TRANSPOSE_8x8 16
    lea    r1,    [r4 + 8]
    lea    r0,    [r3 + 8 * 16]
    TRANSPOSE_8x8 16
    lea    r1,    [r1 + 2 * r2]
    lea    r0,    [r3 + 8 * 16 + 8]
    TRANSPOSE_8x8 16

    RET

cglobal transpose16_internal
    TRANSPOSE_8x8 r6
    lea    r1,    [r1 + 2 * r2]
    lea    r0,    [r5 + 8]
    TRANSPOSE_8x8 r6
    lea    r1,    [r1 + 2 * r2]
    neg    r2
    lea    r1,    [r1 + r2 * 8]
    lea    r1,    [r1 + r2 * 8 + 8]
    neg    r2
    lea    r0,    [r5 + 8 * r6]
    TRANSPOSE_8x8 r6
    lea    r1,    [r1 + 2 * r2]
    lea    r0,    [r5 + 8 * r6 + 8]
    TRANSPOSE_8x8 r6
    ret

;-----------------------------------------------------------------
; void transpose_32x32(pixel *dst, pixel *src, intptr_t stride)
;-----------------------------------------------------------------
INIT_XMM sse2
cglobal transpose32, 3, 7, 8, dest, src, stride

    mov    r3,    r0
    mov    r4,    r1
    mov    r5,    r0
    mov    r6,    32
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 16]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r4 + 16]
    lea    r0,    [r3 + 16 * 32]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 16 * 32 + 16]
    mov    r5,    r0
    call   transpose16_internal

    RET

;-----------------------------------------------------------------
; void transpose_64x64(pixel *dst, pixel *src, intptr_t stride)
;-----------------------------------------------------------------
INIT_XMM sse2
cglobal transpose64, 3, 7, 8, dest, src, stride

    mov    r3,    r0
    mov    r4,    r1
    mov    r5,    r0
    mov    r6,    64
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 16]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 32]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 48]
    mov    r5,    r0
    call   transpose16_internal

    lea    r1,    [r4 + 16]
    lea    r0,    [r3 + 16 * 64]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 16 * 64 + 16]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 16 * 64 + 32]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 16 * 64 + 48]
    mov    r5,    r0
    call   transpose16_internal

    lea    r1,    [r4 + 32]
    lea    r0,    [r3 + 32 * 64]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 32 * 64 + 16]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 32 * 64 + 32]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 32 * 64 + 48]
    mov    r5,    r0
    call   transpose16_internal

    lea    r1,    [r4 + 48]
    lea    r0,    [r3 + 48 * 64]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 48 * 64 + 16]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 48 * 64 + 32]
    mov    r5,    r0
    call   transpose16_internal
    lea    r1,    [r1 - 8 + 2 * r2]
    lea    r0,    [r3 + 48 * 64 + 48]
    mov    r5,    r0
    call   transpose16_internal

    RET

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_2x4(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
INIT_XMM sse4
%if ARCH_X86_64
    cglobal pixel_sub_ps_2x4, 6, 8, 0

    %define tmp_r1     r1
    DECLARE_REG_TMP    6, 7
%else
    cglobal pixel_sub_ps_2x4, 6, 7, 0, 0-4

    %define tmp_r1     dword [rsp]
    DECLARE_REG_TMP    6, 1
%endif ; ARCH_X86_64

    add    r1,         r1

%if ARCH_X86_64 == 0
    mov    tmp_r1,     r1

%endif

movzx    t0d,      byte [r2]
movzx    t1d,      byte [r3]

sub      t0d,      t1d

mov      [r0],     t0w

movzx    t0d,      byte [r2 + 1]
movzx    t1d,      byte [r3 + 1]

sub      t0d,      t1d

mov      [r0 + 2], t0w

add      r0,       tmp_r1

movzx    t0d,      byte [r2 + r4]
movzx    t1d,      byte [r3 + r5]

sub      t0d,      t1d

mov      [r0],     t0w

movzx    t0d,      byte [r2 + r4 + 1]
movzx    t1d,      byte [r3 + r5 + 1]

sub      t0d,      t1d

mov      [r0 + 2], t0w

add      r0,       tmp_r1

movzx    t0d,      byte [r2 + r4 * 2]
movzx    t1d,      byte [r3 + r5 * 2]

sub      t0d,      t1d

mov      [r0],     t0w

movzx    t0d,      byte [r2 + r4 * 2 + 1]
movzx    t1d,      byte [r3 + r5 * 2 + 1]

sub      t0d,      t1d

mov      [r0 + 2], t0w

add      r0,       tmp_r1

lea      r2,       [r2 + r4 * 2]
lea      r3,       [r3 + r5 * 2]

movzx    t0d,      byte [r2 + r4]
movzx    t1d,      byte [r3 + r5]

sub      t0d,      t1d

mov      [r0],     t0w

movzx    t0d,      byte [r2 + r4 + 1]
movzx    t1d,      byte [r3 + r5 + 1]

sub      t0d,      t1d

mov      [r0 + 2], t0w

RET

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_2x8(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
INIT_XMM sse4
%if ARCH_X86_64
    cglobal pixel_sub_ps_2x8, 6, 8, 0

    %define tmp_r1     r1
    DECLARE_REG_TMP    6, 7
%else
    cglobal pixel_sub_ps_2x8, 6, 7, 0, 0-4

    %define tmp_r1     dword [rsp]
    DECLARE_REG_TMP    6, 1
%endif ; ARCH_X86_64

    add    r1,         r1

%if ARCH_X86_64 == 0
    mov    tmp_r1,     r1

%endif

    movzx    t0d,      byte [r2]
    movzx    t1d,      byte [r3]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + 1]
    movzx    t1d,      byte [r3 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    movzx    t0d,      byte [r2 + r4]
    movzx    t1d,      byte [r3 + r5]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 + 1]
    movzx    t1d,      byte [r3 + r5 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    movzx    t0d,      byte [r2 + r4 * 2]
    movzx    t1d,      byte [r3 + r5 * 2]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 * 2 + 1]
    movzx    t1d,      byte [r3 + r5 * 2 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    lea      r2,       [r2 + r4 * 2]
    lea      r3,       [r3 + r5 * 2]

    movzx    t0d,      byte [r2 + r4]
    movzx    t1d,      byte [r3 + r5]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 + 1]
    movzx    t1d,      byte [r3 + r5 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    movzx    t0d,      byte [r2 + r4 * 2]
    movzx    t1d,      byte [r3 + r5 * 2]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 * 2 + 1]
    movzx    t1d,      byte [r3 + r5 * 2 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    lea      r2,       [r2 + r4 * 2]
    lea      r3,       [r3 + r5 * 2]

    movzx    t0d,      byte [r2 + r4]
    movzx    t1d,      byte [r3 + r5]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 + 1]
    movzx    t1d,      byte [r3 + r5 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    movzx    t0d,      byte [r2 + r4 * 2]
    movzx    t1d,      byte [r3 + r5 * 2]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 * 2 + 1]
    movzx    t1d,      byte [r3 + r5 * 2 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

    add      r0,       tmp_r1

    lea      r2,       [r2 + r4 * 2]
    lea      r3,       [r3 + r5 * 2]

    movzx    t0d,      byte [r2 + r4]
    movzx    t1d,      byte [r3 + r5]

    sub      t0d,      t1d

    mov      [r0],     t0w
    movzx    t0d,      byte [r2 + r4 + 1]
    movzx    t1d,      byte [r3 + r5 + 1]

    sub      t0d,      t1d

    mov      [r0 + 2], t0w

RET

;-----------------------------------------------------------------------------
; void pixel_sub_sp_c_4x2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_sub_ps_4x2, 6, 6, 4, dest, deststride, src0, src1, srcstride0, srcstride1

add          r1,    r1

movd         m0,    [r2]
movd         m1,    [r3]

movd         m2,    [r2 + r4]
movd         m3,    [r3 + r5]

punpckldq    m0,    m2
punpckldq    m1,    m3
pmovzxbw     m0,    m0
pmovzxbw     m1,    m1

psubw        m0,    m1

movlps    [r0],         m0
movhps    [r0 + r1],    m0

RET

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_4x4(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_sub_ps_4x4, 6, 6, 8, dest, deststride, src0, src1, srcstride0, srcstride1

add          r1,    r1

movd         m0,    [r2]
movd         m1,    [r3]

movd         m2,    [r2 + r4]
movd         m3,    [r3 + r5]

movd         m4,    [r2 + 2 * r4]
movd         m5,    [r3 + 2 * r5]

lea          r2,    [r2 + 2 * r4]
lea          r3,    [r3 + 2 * r5]

movd         m6,    [r2 + r4]
movd         m7,    [r3 + r5]

punpckldq    m0,    m2
punpckldq    m1,    m3
punpckldq    m4,    m6
punpckldq    m5,    m7

pmovzxbw     m0,    m0
pmovzxbw     m1,    m1
pmovzxbw     m4,    m4
pmovzxbw     m5,    m5

psubw        m0,    m1
psubw        m4,    m5

movlps    [r0],             m0
movhps    [r0 + r1],        m0
movlps    [r0 + 2 * r1],    m4

lea       r0,               [r0 + 2 * r1]

movhps    [r0 + r1],        m4

RET

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W4_H4 2
INIT_XMM sse4
cglobal pixel_sub_ps_%1x%2, 6, 7, 8, dest, deststride, src0, src1, srcstride0, srcstride1

add    r1,     r1
mov    r6d,    %2/4

.loop

    movd         m0,    [r2]
    movd         m1,    [r3]

    movd         m2,    [r2 + r4]
    movd         m3,    [r3 + r5]

    movd         m4,    [r2 + 2 * r4]
    movd         m5,    [r3 + 2 * r5]

    lea          r2,    [r2 + 2 * r4]
    lea          r3,    [r3 + 2 * r5]

    movd         m6,    [r2 + r4]
    movd         m7,    [r3 + r5]

    punpckldq    m0,    m2
    punpckldq    m1,    m3
    punpckldq    m4,    m6
    punpckldq    m5,    m7

    pmovzxbw     m0,    m0
    pmovzxbw     m1,    m1
    pmovzxbw     m4,    m4
    pmovzxbw     m5,    m5

    psubw        m0,    m1
    psubw        m4,    m5

    movlps    [r0],             m0
    movhps    [r0 + r1],        m0
    movlps    [r0 + 2 * r1],    m4

    lea       r0,               [r0 + 2 * r1]

    movhps    [r0 + r1],        m4

    lea       r2,               [r2 + 2 * r4]
    lea       r3,               [r3 + 2 * r5]
    lea       r0,               [r0 + 2 * r1]

    dec       r6d

jnz    .loop

RET
%endmacro

PIXELSUB_PS_W4_H4 4, 8
PIXELSUB_PS_W4_H4 4, 16

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W6_H4 2
INIT_XMM sse4
cglobal pixel_sub_ps_%1x%2, 6, 7, 8, dest, deststride, src0, src1, srcstride0, srcstride1

add    r1,     r1
mov    r6d,    %2/4

.loop

    movh        m0,    [r2]
    movh        m1,    [r3]

    movh        m2,    [r2 + r4]
    movh        m3,    [r3 + r5]

    movh        m4,    [r2 + 2 * r4]
    movh        m5,    [r3 + 2 * r5]

    lea         r2,    [r2 + 2 * r4]
    lea         r3,    [r3 + 2 * r5]

    movh        m6,    [r2 + r4]
    movh        m7,    [r3 + r5]

    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5
    pmovzxbw    m6,    m6
    pmovzxbw    m7,    m7

    psubw       m0,    m1
    psubw       m2,    m3
    psubw       m4,    m5
    psubw       m6,    m7

    movh      [r0],                 m0
    pextrd    [r0 + 8],             m0,    2
    movh      [r0 + r1],            m2
    pextrd    [r0 + r1 + 8],        m2,    2
    movh      [r0 + 2* r1],         m4
    pextrd    [r0 + 2 * r1 + 8],    m4,    2

    lea       r0,                   [r0 + 2 * r1]

    movh      [r0 + r1],            m6
    pextrd    [r0 + r1 + 8],        m6,    2

    lea       r2,                   [r2 + 2 * r4]
    lea       r3,                   [r3 + 2 * r5]
    lea       r0,                   [r0 + 2 * r1]

    dec     r6d

jnz    .loop

RET
%endmacro

PIXELSUB_PS_W6_H4 6, 8

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_8x2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_sub_ps_8x2, 6, 6, 4, dest, deststride, src0, src1, srcstride0, srcstride1

add         r1,    r1

movh        m0,    [r2]
movh        m1,    [r3]
pmovzxbw    m0,    m0
pmovzxbw    m1,    m1

movh        m2,    [r2 + r4]
movh        m3,    [r3 + r5]
pmovzxbw    m2,    m2
pmovzxbw    m3,    m3

psubw       m0,    m1
psubw       m2,    m3

movu    [r0],         m0
movu    [r0 + r1],    m2

RET

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_8x4(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_sub_ps_8x4, 6, 6, 8, dest, deststride, src0, src1, srcstride0, srcstride1

add         r1,    r1

movh        m0,    [r2]
movh        m1,    [r3]
pmovzxbw    m0,    m0
pmovzxbw    m1,    m1

movh        m2,    [r2 + r4]
movh        m3,    [r3 + r5]
pmovzxbw    m2,    m2
pmovzxbw    m3,    m3

movh        m4,    [r2 + 2 * r4]
movh        m5,    [r3 + 2 * r5]
pmovzxbw    m4,    m4
pmovzxbw    m5,    m5

psubw       m0,    m1
psubw       m2,    m3
psubw       m4,    m5

lea         r2,    [r2 + 2 * r4]
lea         r3,    [r3 + 2 * r5]

movh        m6,    [r2 + r4]
movh        m7,    [r3 + r5]
pmovzxbw    m6,    m6
pmovzxbw    m7,    m7

psubw       m6,    m7

movu    [r0],             m0
movu    [r0 + r1],        m2
movu    [r0 + 2 * r1],    m4

lea     r0,               [r0 + 2 * r1]

movu    [r0 + r1],        m6

RET

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_8x6(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_sub_ps_8x6, 6, 6, 8, dest, deststride, src0, src1, srcstride0, srcstride1

add         r1,    r1

movh        m0,    [r2]
movh        m1,    [r3]
pmovzxbw    m0,    m0
pmovzxbw    m1,    m1

movh        m2,    [r2 + r4]
movh        m3,    [r3 + r5]
pmovzxbw    m2,    m2
pmovzxbw    m3,    m3

movh        m4,    [r2 + 2 * r4]
movh        m5,    [r3 + 2 * r5]
pmovzxbw    m4,    m4
pmovzxbw    m5,    m5

psubw       m0,    m1
psubw       m2,    m3
psubw       m4,    m5

lea         r2,    [r2 + 2 * r4]
lea         r3,    [r3 + 2 * r5]

movh        m6,    [r2 + r4]
movh        m7,    [r3 + r5]
pmovzxbw    m6,    m6
pmovzxbw    m7,    m7

movh        m1,    [r2 + 2 * r4]
movh        m3,    [r3 + 2 * r5]
pmovzxbw    m1,    m1
pmovzxbw    m3,    m3

psubw       m6,    m7
psubw       m1,    m3

lea         r2,    [r2 + 2 * r4]
lea         r3,    [r3 + 2 * r5]

movh        m3,    [r2 + r4]
movh        m5,    [r3 + r5]
pmovzxbw    m3,    m3
pmovzxbw    m5,    m5

psubw       m3,     m5

movu    [r0],             m0
movu    [r0 + r1],        m2
movu    [r0 + 2 * r1],    m4

lea     r0,               [r0 + 2 * r1]

movu    [r0 + r1],        m6
movu    [r0 + 2 * r1],    m1

lea     r0,               [r0 + 2 * r1]

movu    [r0 + r1],        m3

RET

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W8_H4 2
INIT_XMM sse4
cglobal pixel_sub_ps_%1x%2, 6, 7, 8, dest, deststride, src0, src1, srcstride0, srcstride1

add    r1,     r1
mov    r6d,    %2/4

.loop

    movh        m0,    [r2]
    movh        m1,    [r3]
    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1

    movh        m2,    [r2 + r4]
    movh        m3,    [r3 + r5]
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3

    movh        m4,    [r2 + 2 * r4]
    movh        m5,    [r3 + 2 * r5]
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5

    psubw       m0,    m1
    psubw       m2,    m3
    psubw       m4,    m5

    lea         r2,    [r2 + 2 * r4]
    lea         r3,    [r3 + 2 * r5]

    movh        m6,    [r2 + r4]
    movh        m7,    [r3 + r5]
    pmovzxbw    m6,    m6
    pmovzxbw    m7,    m7

    psubw       m6,    m7

    movu    [r0],             m0
    movu    [r0 + r1],        m2
    movu    [r0 + 2 * r1],    m4

    lea     r0,               [r0 + 2 * r1]

    movu    [r0 + r1],        m6

    lea     r2,               [r2 + 2 * r4]
    lea     r3,               [r3 + 2 * r5]
    lea     r0,               [r0 + 2 * r1]

    dec     r6d

jnz    .loop

RET
%endmacro

PIXELSUB_PS_W8_H4 8, 8
PIXELSUB_PS_W8_H4 8, 16
PIXELSUB_PS_W8_H4 8, 32

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W12_H4 2
INIT_XMM sse4
cglobal pixel_sub_ps_%1x%2, 6, 7, 6, dest, deststride, src0, src1, srcstride0, srcstride1

add    r1,     r1
mov    r6d,    %2/4

.loop

    movu        m0,    [r2]
    movu        m1,    [r3]
    movu        m2,    [r2 + r4]
    movu        m3,    [r3 + r5]

    mova        m4,    m0
    mova        m5,    m1
    punpckhdq   m4,    m2
    punpckhdq   m5,    m3

    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5

    psubw       m0,    m1
    psubw       m2,    m3
    psubw       m4,    m5

    movu      [r0],              m0
    movlps    [r0 + 16],         m4
    movu      [r0 + r1],         m2
    movhps    [r0 + r1 + 16],    m4

    movu      m0,    [r2 + 2 * r4]
    movu      m1,    [r3 + 2 * r5]

    lea       r2,    [r2 + 2 * r4]
    lea       r3,    [r3 + 2 * r5]

    movu      m2,    [r2 + r4]
    movu      m3,    [r3 + r5]

    mova         m4,    m0
    mova         m5,    m1
    punpckhdq    m4,    m2
    punpckhdq    m5,    m3

    pmovzxbw     m0,    m0
    pmovzxbw     m1,    m1
    pmovzxbw     m2,    m2
    pmovzxbw     m3,    m3
    pmovzxbw     m4,    m4
    pmovzxbw     m5,    m5

    psubw        m0,    m1
    psubw        m2,    m3
    psubw        m4,    m5

    movu      [r0 + 2 * r1],         m0
    movlps    [r0 + 2 * r1 + 16],    m4

    lea       r0,                    [r0 + 2 * r1]

    movu      [r0 + r1],             m2
    movhps    [r0 + r1 + 16],        m4

    lea       r2,                    [r2 + 2 * r4]
    lea       r3,                    [r3 + 2 * r5]
    lea       r0,                    [r0 + 2 * r1]

    dec    r6d

jnz    .loop

RET
%endmacro

PIXELSUB_PS_W12_H4 12, 16

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W16_H4 2
INIT_XMM sse4
cglobal pixel_sub_ps_%1x%2, 6, 7, 7, dest, deststride, src0, src1, srcstride0, srcstride1

add    r1,     r1
mov    r6d,    %2/4
pxor   m6,     m6

.loop

    movu         m1,    [r2]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m0,    m2
    psubw        m1,    m3

    movu         m5,    [r2 + r4]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    movu         m3,    [r3 + r5]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m4,    m2
    psubw        m5,    m3

    movu    [r0],              m0
    movu    [r0 + 16],         m1
    movu    [r0 + r1],         m4
    movu    [r0 + r1 + 16],    m5

    movu         m1,    [r2 + 2 * r4]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3 + 2 * r5]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    lea          r2,    [r2 + 2 * r4]
    lea          r3,    [r3 + 2 * r5]

    psubw        m0,    m2
    psubw        m1,    m3

    movu         m5,    [r2 + r4]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    movu         m3,    [r3 + r5]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m4,    m2
    psubw        m5,    m3

    movu    [r0 + 2 * r1],         m0
    movu    [r0 + 2 * r1 + 16],    m1

    lea     r0,                    [r0 + 2 * r1]

    movu    [r0 + r1],             m4
    movu    [r0 + r1 + 16],        m5

    lea     r2,                    [r2 + 2 * r4]
    lea     r3,                    [r3 + 2 * r5]
    lea     r0,                    [r0 + 2 * r1]

    dec    r6d

jnz    .loop

RET
%endmacro

PIXELSUB_PS_W16_H4 16, 4
PIXELSUB_PS_W16_H4 16, 8
PIXELSUB_PS_W16_H4 16, 12
PIXELSUB_PS_W16_H4 16, 16
PIXELSUB_PS_W16_H4 16, 32
PIXELSUB_PS_W16_H4 16, 64

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W24_H2 2
INIT_XMM sse4
cglobal pixel_sub_ps_%1x%2, 6, 7, 7, dest, deststride, src0, src1, srcstride0, srcstride1

add    r1,     r1
mov    r6d,    %2/2
pxor   m6,     m6

.loop

    movu         m1,    [r2]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movh         m2,    [r2 + 16]
    pmovzxbw     m2,    m2
    movu         m4,    [r3]
    pmovzxbw     m3,    m4
    punpckhbw    m4,    m6
    movh         m5,    [r3 + 16]
    pmovzxbw     m5,    m5

    psubw        m0,    m3
    psubw        m1,    m4
    psubw        m2,    m5

    movu    [r0],         m0
    movu    [r0 + 16],    m1
    movu    [r0 + 32],    m2

    movu         m1,    [r2 + r4]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movh         m2,    [r2 + r4 + 16]
    pmovzxbw     m2,    m2
    movu         m4,    [r3 + r5]
    pmovzxbw     m3,    m4
    punpckhbw    m4,    m6
    movh         m5,    [r3 + r5 + 16]
    pmovzxbw     m5,    m5

    psubw        m0,    m3
    psubw        m1,    m4
    psubw        m2,    m5

    movu    [r0 + r1],         m0
    movu    [r0 + r1 + 16],    m1
    movu    [r0 + r1 + 32],    m2

    lea    r2,    [r2 + 2 * r4]
    lea    r3,    [r3 + 2 * r5]
    lea    r0,    [r0 + 2 * r1]

    dec    r6d

jnz    .loop

RET
%endmacro

PIXELSUB_PS_W24_H2 24, 32

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W32_H2 2
INIT_XMM sse4
cglobal pixel_sub_ps_%1x%2, 6, 7, 8, dest, deststride, src0, src1, srcstride0, srcstride1

add    r1,     r1
mov    r6d,    %2/2

.loop

    movh        m0,    [r2]
    movh        m1,    [r2 + 8]
    movh        m2,    [r2 + 16]
    movh        m3,    [r2 + 24]
    movh        m4,    [r3]
    movh        m5,    [r3 + 8]
    movh        m6,    [r3 + 16]
    movh        m7,    [r3 + 24]

    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5
    pmovzxbw    m6,    m6
    pmovzxbw    m7,    m7

    psubw       m0,    m4
    psubw       m1,    m5
    psubw       m2,    m6
    psubw       m3,    m7

    movu    [r0],         m0
    movu    [r0 + 16],    m1
    movu    [r0 + 32],    m2
    movu    [r0 + 48],    m3

    movh        m0,    [r2 + r4]
    movh        m1,    [r2 + r4 + 8]
    movh        m2,    [r2 + r4 + 16]
    movh        m3,    [r2 + r4 + 24]
    movh        m4,    [r3 + r5]
    movh        m5,    [r3 + r5 + 8]
    movh        m6,    [r3 + r5 + 16]
    movh        m7,    [r3 + r5 + 24]

    pmovzxbw    m0,    m0
    pmovzxbw    m1,    m1
    pmovzxbw    m2,    m2
    pmovzxbw    m3,    m3
    pmovzxbw    m4,    m4
    pmovzxbw    m5,    m5
    pmovzxbw    m6,    m6
    pmovzxbw    m7,    m7

    psubw       m0,    m4
    psubw       m1,    m5
    psubw       m2,    m6
    psubw       m3,    m7

    movu    [r0 + r1],         m0
    movu    [r0 + r1 + 16],    m1
    movu    [r0 + r1 + 32],    m2
    movu    [r0 + r1 + 48],    m3

    lea    r2,    [r2 + 2 * r4]
    lea    r3,    [r3 + 2 * r5]
    lea    r0,    [r0 + 2 * r1]

    dec    r6d

jnz    .loop

RET
%endmacro

PIXELSUB_PS_W32_H2 32, 8
PIXELSUB_PS_W32_H2 32, 16
PIXELSUB_PS_W32_H2 32, 24
PIXELSUB_PS_W32_H2 32, 32
PIXELSUB_PS_W32_H2 32, 64

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W48_H2 2
INIT_XMM sse4
cglobal pixel_sub_ps_%1x%2, 6, 7, 7, dest, deststride, src0, src1, srcstride0, srcstride1

add    r1,     r1
mov    r6d,    %2/2
pxor   m6,    m6

.loop

    movu         m1,    [r2]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    movu         m5,    [r2 + 16]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6

    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0],         m0
    movu    [r0 + 16],    m1

    movu         m3,    [r3 + 16]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m4,    m2
    psubw        m5,    m3

    movu    [r0 + 32],    m4
    movu    [r0 + 48],    m5

    movu         m1,    [r2 + 32]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3 + 32]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0 + 64],    m0
    movu    [r0 + 80],    m1

    movu         m1,    [r2 + r4]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3 + r5]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    movu         m5,    [r2 + r5 + 16]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6

    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0 + r1],         m0
    movu    [r0 + r1 + 16],    m1

    movu         m3,    [r3 + r4 + 16]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m4,    m2
    psubw        m5,    m3

    movu    [r0 + r1 + 32],    m4
    movu    [r0 + r1 + 48],    m5

    movu         m1,    [r2 + r4 + 32]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3 + r5 + 32]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0 + r1 + 64],    m0
    movu    [r0 + r1 + 80],    m1

    lea     r2,                [r2 + 2 * r4]
    lea     r3,                [r3 + 2 * r5]
    lea     r0,                [r0 + 2 * r1]

    dec    r6d

jnz    .loop

RET
%endmacro

PIXELSUB_PS_W48_H2 48, 64

;-----------------------------------------------------------------------------
; void pixel_sub_ps_c_%1x%2(int16_t *dest, intptr_t destride, pixel *src0, pixel *src1, intptr_t srcstride0, intptr_t srcstride1);
;-----------------------------------------------------------------------------
%macro PIXELSUB_PS_W64_H2 2
INIT_XMM sse4
cglobal pixel_sub_ps_%1x%2, 6, 7, 7, dest, deststride, src0, src1, srcstride0, srcstride1

add    r1,     r1
mov    r6d,    %2/2
pxor   m6,    m6

.loop

    movu         m1,    [r2]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r3]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    movu         m5,    [r2 + 16]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6

    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0],         m0
    movu    [r0 + 16],    m1

    movu         m1,    [r3 + 16]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r2 + 32]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m4,    m0
    psubw        m5,    m1

    movu    [r0 + 32],    m4
    movu    [r0 + 48],    m5

    movu         m5,    [r3 + 32]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    movu         m1,    [r2 + 48]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6

    psubw        m2,    m4
    psubw        m3,    m5

    movu    [r0 + 64],    m2
    movu    [r0 + 80],    m3

    movu         m3,    [r3 + 48]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    movu         m5,    [r2 + r4]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6

    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0 + 96],     m0
    movu    [r0 + 112],    m1

    movu         m1,    [r3 + r5]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6
    movu         m3,    [r2 + r4 + 16]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6

    psubw        m4,    m0
    psubw        m5,    m1

    movu    [r0 + r1],         m4
    movu    [r0 + r1 + 16],    m5

    movu         m5,    [r3 + r5 + 16]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6
    movu         m1,    [r2 + r4 + 32]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6

    psubw        m2,    m4
    psubw        m3,    m5

    movu    [r0 + r1 + 32],    m2
    movu    [r0 + r1 + 48],    m3

    movu         m3,    [r3 + r5 + 32]
    pmovzxbw     m2,    m3
    punpckhbw    m3,    m6
    movu         m5,    [r2 + r4 + 48]
    pmovzxbw     m4,    m5
    punpckhbw    m5,    m6

    psubw        m0,    m2
    psubw        m1,    m3

    movu    [r0 + r1 + 64],    m0
    movu    [r0 + r1 + 80],    m1

    movu         m1,    [r3 + r5 + 48]
    pmovzxbw     m0,    m1
    punpckhbw    m1,    m6

    psubw        m4,    m0
    psubw        m5,    m1

    movu    [r0 + r1 + 96],     m4
    movu    [r0 + r1 + 112],    m5

    lea     r2,                 [r2 + 2 * r4]
    lea     r3,                 [r3 + 2 * r5]
    lea     r0,                 [r0 + 2 * r1]

    dec    r6d

jnz    .loop

RET
%endmacro

PIXELSUB_PS_W64_H2 64, 16
PIXELSUB_PS_W64_H2 64, 32
PIXELSUB_PS_W64_H2 64, 48
PIXELSUB_PS_W64_H2 64, 64

;-----------------------------------------------------------------
; void scale1D_128to64(pixel *dst, pixel *src, intptr_t /*stride*/)
;-----------------------------------------------------------------
INIT_XMM ssse3
cglobal scale1D_128to64, 2, 2, 8, dest, src1, stride

    mova        m7,      [deinterleave_shuf]

    movu        m0,      [r1]
    palignr     m1,      m0,    1
    movu        m2,      [r1 + 16]
    palignr     m3,      m2,    1
    movu        m4,      [r1 + 32]
    palignr     m5,      m4,    1
    movu        m6,      [r1 + 48]

    pavgb       m0,      m1

    palignr     m1,      m6,    1

    pavgb       m2,      m3
    pavgb       m4,      m5
    pavgb       m6,      m1

    pshufb      m0,      m0,    m7
    pshufb      m2,      m2,    m7
    pshufb      m4,      m4,    m7
    pshufb      m6,      m6,    m7

    punpcklqdq    m0,           m2
    movu          [r0],         m0
    punpcklqdq    m4,           m6
    movu          [r0 + 16],    m4

    movu        m0,      [r1 + 64]
    palignr     m1,      m0,    1
    movu        m2,      [r1 + 80]
    palignr     m3,      m2,    1
    movu        m4,      [r1 + 96]
    palignr     m5,      m4,    1
    movu        m6,      [r1 + 112]

    pavgb       m0,      m1

    palignr     m1,      m6,    1

    pavgb       m2,      m3
    pavgb       m4,      m5
    pavgb       m6,      m1

    pshufb      m0,      m0,    m7
    pshufb      m2,      m2,    m7
    pshufb      m4,      m4,    m7
    pshufb      m6,      m6,    m7

    punpcklqdq    m0,           m2
    movu          [r0 + 32],    m0
    punpcklqdq    m4,           m6
    movu          [r0 + 48],    m4

RET
