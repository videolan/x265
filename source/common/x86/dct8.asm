;*****************************************************************************
;* Copyright (C) 2013 x265 project
;*
;* Authors: Nabajit Deka <nabajit@multicorewareinc.com>
;*          Min Chen <chenm003@163.com> <min.chen@multicorewareinc.com>
;*          Li Cao <li@multicorewareinc.com>
;*          Praveen Kumar Tiwari <Praveen@multicorewareinc.com>
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

;TO-DO : Further optimize the routines.

%include "x86inc.asm"
%include "x86util.asm"
SECTION_RODATA 32
tab_dct8:       dw 64, 64, 64, 64, 64, 64, 64, 64
                dw 89, 75, 50, 18, -18, -50, -75, -89
                dw 83, 36, -36, -83, -83, -36, 36, 83
                dw 75, -18, -89, -50, 50, 89, 18, -75
                dw 64, -64, -64, 64, 64, -64, -64, 64
                dw 50, -89, 18, 75, -75, -18, 89, -50
                dw 36, -83, 83, -36, -36, 83, -83, 36
                dw 18, -50, 75, -89, 89, -75, 50, -18

dct8_shuf:      times 2 db 6, 7, 4, 5, 2, 3, 0, 1, 14, 15, 12, 13, 10, 11, 8, 9

tab_dct16_1:    dw 64, 64, 64, 64, 64, 64, 64, 64
                dw 90, 87, 80, 70, 57, 43, 25,  9
                dw 89, 75, 50, 18, -18, -50, -75, -89
                dw 87, 57,  9, -43, -80, -90, -70, -25
                dw 83, 36, -36, -83, -83, -36, 36, 83
                dw 80,  9, -70, -87, -25, 57, 90, 43
                dw 75, -18, -89, -50, 50, 89, 18, -75
                dw 70, -43, -87,  9, 90, 25, -80, -57
                dw 64, -64, -64, 64, 64, -64, -64, 64
                dw 57, -80, -25, 90, -9, -87, 43, 70
                dw 50, -89, 18, 75, -75, -18, 89, -50
                dw 43, -90, 57, 25, -87, 70,  9, -80
                dw 36, -83, 83, -36, -36, 83, -83, 36
                dw 25, -70, 90, -80, 43,  9, -57, 87
                dw 18, -50, 75, -89, 89, -75, 50, -18
                dw  9, -25, 43, -57, 70, -80, 87, -90


tab_dct16_2:    dw 64, 64, 64, 64, 64, 64, 64, 64
                dw -9, -25, -43, -57, -70, -80, -87, -90
                dw -89, -75, -50, -18, 18, 50, 75, 89
                dw 25, 70, 90, 80, 43, -9, -57, -87
                dw 83, 36, -36, -83, -83, -36, 36, 83
                dw -43, -90, -57, 25, 87, 70, -9, -80
                dw -75, 18, 89, 50, -50, -89, -18, 75
                dw 57, 80, -25, -90, -9, 87, 43, -70
                dw 64, -64, -64, 64, 64, -64, -64, 64
                dw -70, -43, 87,  9, -90, 25, 80, -57
                dw -50, 89, -18, -75, 75, 18, -89, 50
                dw 80, -9, -70, 87, -25, -57, 90, -43
                dw 36, -83, 83, -36, -36, 83, -83, 36
                dw -87, 57, -9, -43, 80, -90, 70, -25
                dw -18, 50, -75, 89, -89, 75, -50, 18
                dw 90, -87, 80, -70, 57, -43, 25, -9

dct16_shuf1:     times 2 db 14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1

dct16_shuf2:    times 2 db 0, 1, 14, 15, 2, 3, 12, 13, 4, 5, 10, 11, 6, 7, 8, 9

tab_dct32_1:    dw 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
                dw 90, 90, 88, 85, 82, 78, 73, 67, 61, 54, 46, 38, 31, 22, 13,  4
                dw 90, 87, 80, 70, 57, 43, 25,  9, -9, -25, -43, -57, -70, -80, -87, -90
                dw 90, 82, 67, 46, 22, -4, -31, -54, -73, -85, -90, -88, -78, -61, -38, -13
                dw 89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89
                dw 88, 67, 31, -13, -54, -82, -90, -78, -46, -4, 38, 73, 90, 85, 61, 22
                dw 87, 57,  9, -43, -80, -90, -70, -25, 25, 70, 90, 80, 43, -9, -57, -87
                dw 85, 46, -13, -67, -90, -73, -22, 38, 82, 88, 54, -4, -61, -90, -78, -31
                dw 83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83
                dw 82, 22, -54, -90, -61, 13, 78, 85, 31, -46, -90, -67,  4, 73, 88, 38
                dw 80,  9, -70, -87, -25, 57, 90, 43, -43, -90, -57, 25, 87, 70, -9, -80
                dw 78, -4, -82, -73, 13, 85, 67, -22, -88, -61, 31, 90, 54, -38, -90, -46
                dw 75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75
                dw 73, -31, -90, -22, 78, 67, -38, -90, -13, 82, 61, -46, -88, -4, 85, 54
                dw 70, -43, -87,  9, 90, 25, -80, -57, 57, 80, -25, -90, -9, 87, 43, -70
                dw 67, -54, -78, 38, 85, -22, -90,  4, 90, 13, -88, -31, 82, 46, -73, -61
                dw 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64
                dw 61, -73, -46, 82, 31, -88, -13, 90, -4, -90, 22, 85, -38, -78, 54, 67
                dw 57, -80, -25, 90, -9, -87, 43, 70, -70, -43, 87,  9, -90, 25, 80, -57
                dw 54, -85, -4, 88, -46, -61, 82, 13, -90, 38, 67, -78, -22, 90, -31, -73
                dw 50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50
                dw 46, -90, 38, 54, -90, 31, 61, -88, 22, 67, -85, 13, 73, -82,  4, 78
                dw 43, -90, 57, 25, -87, 70,  9, -80, 80, -9, -70, 87, -25, -57, 90, -43
                dw 38, -88, 73, -4, -67, 90, -46, -31, 85, -78, 13, 61, -90, 54, 22, -82
                dw 36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36
                dw 31, -78, 90, -61,  4, 54, -88, 82, -38, -22, 73, -90, 67, -13, -46, 85
                dw 25, -70, 90, -80, 43,  9, -57, 87, -87, 57, -9, -43, 80, -90, 70, -25
                dw 22, -61, 85, -90, 73, -38, -4, 46, -78, 90, -82, 54, -13, -31, 67, -88
                dw 18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18
                dw 13, -38, 61, -78, 88, -90, 85, -73, 54, -31,  4, 22, -46, 67, -82, 90
                dw 9, -25, 43, -57, 70, -80, 87, -90, 90, -87, 80, -70, 57, -43, 25, -9
                dw 4, -13, 22, -31, 38, -46, 54, -61, 67, -73, 78, -82, 85, -88, 90, -90

tab_dct32_2:    dw 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
                dw -4, -13, -22, -31, -38, -46, -54, -61, -67, -73, -78, -82, -85, -88, -90, -90
                dw -90, -87, -80, -70, -57, -43, -25, -9,  9, 25, 43, 57, 70, 80, 87, 90
                dw 13, 38, 61, 78, 88, 90, 85, 73, 54, 31,  4, -22, -46, -67, -82, -90
                dw 89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89
                dw -22, -61, -85, -90, -73, -38,  4, 46, 78, 90, 82, 54, 13, -31, -67, -88
                dw -87, -57, -9, 43, 80, 90, 70, 25, -25, -70, -90, -80, -43,  9, 57, 87
                dw 31, 78, 90, 61,  4, -54, -88, -82, -38, 22, 73, 90, 67, 13, -46, -85
                dw 83, 36, -36, -83, -83, -36, 36, 83, 83, 36, -36, -83, -83, -36, 36, 83
                dw -38, -88, -73, -4, 67, 90, 46, -31, -85, -78, -13, 61, 90, 54, -22, -82
                dw -80, -9, 70, 87, 25, -57, -90, -43, 43, 90, 57, -25, -87, -70,  9, 80
                dw 46, 90, 38, -54, -90, -31, 61, 88, 22, -67, -85, -13, 73, 82,  4, -78
                dw 75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75
                dw -54, -85,  4, 88, 46, -61, -82, 13, 90, 38, -67, -78, 22, 90, 31, -73
                dw -70, 43, 87, -9, -90, -25, 80, 57, -57, -80, 25, 90,  9, -87, -43, 70
                dw 61, 73, -46, -82, 31, 88, -13, -90, -4, 90, 22, -85, -38, 78, 54, -67
                dw 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64
                dw -67, -54, 78, 38, -85, -22, 90,  4, -90, 13, 88, -31, -82, 46, 73, -61
                dw -57, 80, 25, -90,  9, 87, -43, -70, 70, 43, -87, -9, 90, -25, -80, 57
                dw 73, 31, -90, 22, 78, -67, -38, 90, -13, -82, 61, 46, -88,  4, 85, -54
                dw 50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50
                dw -78, -4, 82, -73, -13, 85, -67, -22, 88, -61, -31, 90, -54, -38, 90, -46
                dw -43, 90, -57, -25, 87, -70, -9, 80, -80,  9, 70, -87, 25, 57, -90, 43
                dw 82, -22, -54, 90, -61, -13, 78, -85, 31, 46, -90, 67,  4, -73, 88, -38
                dw 36, -83, 83, -36, -36, 83, -83, 36, 36, -83, 83, -36, -36, 83, -83, 36
                dw -85, 46, 13, -67, 90, -73, 22, 38, -82, 88, -54, -4, 61, -90, 78, -31
                dw -25, 70, -90, 80, -43, -9, 57, -87, 87, -57,  9, 43, -80, 90, -70, 25
                dw 88, -67, 31, 13, -54, 82, -90, 78, -46,  4, 38, -73, 90, -85, 61, -22
                dw 18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18
                dw -90, 82, -67, 46, -22, -4, 31, -54, 73, -85, 90, -88, 78, -61, 38, -13
                dw -9, 25, -43, 57, -70, 80, -87, 90, -90, 87, -80, 70, -57, 43, -25,  9
                dw 90, -90, 88, -85, 82, -78, 73, -67, 61, -54, 46, -38, 31, -22, 13, -4

tab_idct16_1:   dw 90, 87, 80, 70, 57, 43, 25, 9
                dw 87, 57, 9, -43, -80, -90, -70, -25
                dw 80, 9, -70, -87, -25, 57, 90, 43
                dw 70, -43, -87, 9, 90, 25, -80, -57
                dw 57, -80, -25, 90, -9, -87, 43, 70
                dw 43, -90, 57, 25, -87, 70, 9, -80
                dw 25, -70, 90, -80, 43, 9, -57, 87
                dw 9, -25, 43, -57, 70, -80, 87, -90

tab_idct16_2:   dw 64, 89, 83, 75, 64, 50, 36, 18
                dw 64, 75, 36, -18, -64, -89, -83, -50
                dw 64, 50, -36, -89, -64, 18, 83, 75
                dw 64, 18, -83, -50, 64, 75, -36, -89
                dw 64, -18, -83, 50, 64, -75, -36, 89
                dw 64, -50, -36, 89, -64, -18, 83, -75
                dw 64, -75, 36, 18, -64, 89, -83, 50
                dw 64, -89, 83, -75, 64, -50, 36, -18

idct16_shuff:   dd 0, 4, 2, 6, 1, 5, 3, 7

idct16_shuff1:  dd 2, 6, 0, 4, 3, 7, 1, 5

avx2_dct4:      dw 64, 64, 64, 64, 64, 64, 64, 64, 64, -64, 64, -64, 64, -64, 64, -64
                dw 83, 36, 83, 36, 83, 36, 83, 36, 36, -83, 36, -83, 36, -83, 36, -83

tab_dct4:       times 4 dw 64, 64
                times 4 dw 83, 36
                times 4 dw 64, -64
                times 4 dw 36, -83

dct4_shuf:      db 0, 1, 2, 3, 8, 9, 10, 11, 6, 7, 4, 5, 14, 15, 12, 13

tab_dst4:       times 2 dw 29, 55, 74, 84
                times 2 dw 74, 74,  0, -74
                times 2 dw 84, -29, -74, 55
                times 2 dw 55, -84, 74, -29

tab_idst4:      times 4 dw 29, +84
                times 4 dw +74, +55
                times 4 dw 55, -29
                times 4 dw +74, -84
                times 4 dw 74, -74
                times 4 dw 0, +74
                times 4 dw 84, +55
                times 4 dw -74, -29

tab_dct8_1:     times 2 dw 89, 50, 75, 18
                times 2 dw 75, -89, -18, -50
                times 2 dw 50, 18, -89, 75
                times 2 dw 18, 75, -50, -89

tab_dct8_2:     times 2 dd 83, 36
                times 2 dd 36, 83
                times 1 dd 89, 75, 50, 18
                times 1 dd 75, -18, -89, -50
                times 1 dd 50, -89, 18, 75
                times 1 dd 18, -50, 75, -89

tab_idct8_3:    times 4 dw 89, 75
                times 4 dw 50, 18
                times 4 dw 75, -18
                times 4 dw -89, -50
                times 4 dw 50, -89
                times 4 dw 18, 75
                times 4 dw 18, -50
                times 4 dw 75, -89

pb_unpackhlw1:  db 0,1,8,9,2,3,10,11,4,5,12,13,6,7,14,15

pb_idct8even:   db 0, 1, 8, 9, 4, 5, 12, 13, 0, 1, 8, 9, 4, 5, 12, 13

tab_idct8_1:    times 1 dw 64, -64, 36, -83, 64, 64, 83, 36

tab_idct8_2:    times 1 dw 89, 75, 50, 18, 75, -18, -89, -50
                times 1 dw 50, -89, 18, 75, 18, -50, 75, -89

pb_idct8odd:    db 2, 3, 6, 7, 10, 11, 14, 15, 2, 3, 6, 7, 10, 11, 14, 15

SECTION .text
cextern pd_1
cextern pd_2
cextern pd_4
cextern pd_8
cextern pd_16
cextern pd_32
cextern pd_64
cextern pd_128
cextern pd_256
cextern pd_512
cextern pd_1024
cextern pd_2048
cextern pw_ppppmmmm

;------------------------------------------------------
;void dct4(int16_t *src, int32_t *dst, intptr_t stride)
;------------------------------------------------------
INIT_XMM sse2
cglobal dct4, 3, 4, 8
%if BIT_DEPTH == 10
  %define       DCT_SHIFT 3
  mova          m7, [pd_4]
%elif BIT_DEPTH == 8
  %define       DCT_SHIFT 1
  mova          m7, [pd_1]
%else
  %error Unsupported BIT_DEPTH!
%endif
    add         r2d, r2d
    lea         r3, [tab_dct4]

    mova        m4, [r3 + 0 * 16]
    mova        m5, [r3 + 1 * 16]
    mova        m6, [r3 + 2 * 16]
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
    psrad       m0, DCT_SHIFT
    pmaddwd     m3, m2, m5
    paddd       m3, m7
    psrad       m3, DCT_SHIFT
    packssdw    m0, m3
    pshufd      m0, m0, 0xD8
    pshufhw     m0, m0, 0xB1
    pmaddwd     m1, m6
    paddd       m1, m7
    psrad       m1, DCT_SHIFT
    pmaddwd     m2, [r3 + 3 * 16]
    paddd       m2, m7
    psrad       m2, DCT_SHIFT
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

; DCT 4x4
;
; Input parameters:
; - r0:     source
; - r1:     destination
; - r2:     source stride
INIT_YMM avx2
cglobal dct4, 3, 4, 8, src, dst, srcStride
%if BIT_DEPTH == 10
    %define DCT_SHIFT 3
    vbroadcasti128 m7, [pd_4]
%elif BIT_DEPTH == 8
    %define DCT_SHIFT 1
    vbroadcasti128 m7, [pd_1]
%else
    %error Unsupported BIT_DEPTH!
%endif
    add             r2d, r2d
    lea             r3, [avx2_dct4]

    vbroadcasti128  m4, [dct4_shuf]
    mova            m5, [r3]
    mova            m6, [r3 + 32]
    movq            xm0, [r0]
    movhps          xm0, [r0 + r2]
    lea             r0, [r0 + 2 * r2]
    movq            xm1, [r0]
    movhps          xm1, [r0 + r2]

    vinserti128     m0, m0, xm1, 1
    pshufb          m0, m4
    vpermq          m1, m0, 11011101b
    vpermq          m0, m0, 10001000b
    paddw           m2, m0, m1
    psubw           m0, m1

    pmaddwd         m2, m5
    paddd           m2, m7
    psrad           m2, DCT_SHIFT

    pmaddwd         m0, m6
    paddd           m0, m7
    psrad           m0, DCT_SHIFT

    packssdw        m2, m0
    pshufb          m2, m4
    vpermq          m1, m2, 11011101b
    vpermq          m2, m2, 10001000b
    vbroadcasti128  m7, [pd_128]

    pmaddwd         m0, m2, m5
    pmaddwd         m3, m1, m5
    paddd           m3, m0
    paddd           m3, m7
    psrad           m3, 8

    pmaddwd         m2, m6
    pmaddwd         m1, m6
    psubd           m2, m1
    paddd           m2, m7
    psrad           m2, 8

    movu            [r1], xm3
    movu            [r1 + mmsize/2], m2
    vextracti128    [r1 + mmsize], m3, 1
    vextracti128    [r1 + mmsize + mmsize/2], m2, 1
    RET

;-------------------------------------------------------
;void idct4(int32_t *src, int16_t *dst, intptr_t stride)
;-------------------------------------------------------
INIT_XMM sse2
cglobal idct4, 3, 4, 7
%if BIT_DEPTH == 8
  %define IDCT4_OFFSET  [pd_2048]
  %define IDCT4_SHIFT   12
%elif BIT_DEPTH == 10
  %define IDCT4_OFFSET  [pd_512]
  %define IDCT4_SHIFT   10
%else
  %error Unsupported BIT_DEPTH!
%endif
    add         r2d, r2d
    lea         r3, [tab_dct4]

    mova        m6, [pd_64]

    movu        m0, [r0 + 0 * 16]
    movu        m1, [r0 + 1 * 16]
    packssdw    m0, m1

    movu        m1, [r0 + 2 * 16]
    movu        m2, [r0 + 3 * 16]
    packssdw    m1, m2

    punpcklwd   m2, m0, m1
    pmaddwd     m3, m2, [r3 + 0 * 16]       ; m3 = E1
    paddd       m3, m6

    pmaddwd     m2, [r3 + 2 * 16]           ; m2 = E2
    paddd       m2, m6

    punpckhwd   m0, m1
    pmaddwd     m1, m0, [r3 + 1 * 16]       ; m1 = O1
    pmaddwd     m0, [r3 + 3 * 16]           ; m0 = O2

    paddd       m4, m3, m1
    psrad       m4, 7                       ; m4 = m128iA
    paddd       m5, m2, m0
    psrad       m5, 7
    packssdw    m4, m5                      ; m4 = m128iA

    psubd       m2, m0
    psrad       m2, 7
    psubd       m3, m1
    psrad       m3, 7
    packssdw    m2, m3                      ; m2 = m128iD

    punpcklwd   m1, m4, m2                  ; m1 = S0
    punpckhwd   m4, m2                      ; m4 = S8

    punpcklwd   m0, m1, m4                  ; m0 = m128iA
    punpckhwd   m1, m4                      ; m1 = m128iD

    mova        m6, IDCT4_OFFSET

    punpcklwd   m2, m0, m1
    pmaddwd     m3, m2, [r3 + 0 * 16]
    paddd       m3, m6                      ; m3 = E1

    pmaddwd     m2, [r3 + 2 * 16]
    paddd       m2, m6                      ; m2 = E2

    punpckhwd   m0, m1
    pmaddwd     m1, m0, [r3 + 1 * 16]       ; m1 = O1
    pmaddwd     m0, [r3 + 3 * 16]           ; m0 = O2

    paddd       m4, m3, m1
    psrad       m4, IDCT4_SHIFT             ; m4 = m128iA
    paddd       m5, m2, m0
    psrad       m5, IDCT4_SHIFT
    packssdw    m4, m5                      ; m4 = m128iA

    psubd       m2, m0
    psrad       m2, IDCT4_SHIFT
    psubd       m3, m1
    psrad       m3, IDCT4_SHIFT
    packssdw    m2, m3                      ; m2 = m128iD

    punpcklwd   m1, m4, m2
    punpckhwd   m4, m2

    punpcklwd   m0, m1, m4
    movlps      [r1 + 0 * r2], m0
    movhps      [r1 + 1 * r2], m0

    punpckhwd   m1, m4
    movlps      [r1 + 2 * r2], m1
    lea         r1, [r1 + 2 * r2]
    movhps      [r1 + r2], m1

    RET

;------------------------------------------------------
;void dst4(int16_t *src, int32_t *dst, intptr_t stride)
;------------------------------------------------------
INIT_XMM ssse3
%if ARCH_X86_64
cglobal dst4, 3, 4, 8+2
  %define       coef2   m8
  %define       coef3   m9
%else ; ARCH_X86_64 = 0
cglobal dst4, 3, 4, 8
  %define       coef2   [r3 + 2 * 16]
  %define       coef3   [r3 + 3 * 16]
%endif ; ARCH_X86_64
%define         coef0   m6
%define         coef1   m7

%if BIT_DEPTH == 8
  %define       DST_SHIFT 1
  mova          m5, [pd_1]
%elif BIT_DEPTH == 10
  %define       DST_SHIFT 3
  mova          m5, [pd_4]
%endif
    add         r2d, r2d
    lea         r3, [tab_dst4]
    mova        coef0, [r3 + 0 * 16]
    mova        coef1, [r3 + 1 * 16]
%if ARCH_X86_64
    mova        coef2, [r3 + 2 * 16]
    mova        coef3, [r3 + 3 * 16]
%endif
    movh        m0, [r0 + 0 * r2]            ; load
    movh        m1, [r0 + 1 * r2]
    punpcklqdq  m0, m1
    lea         r0, [r0 + 2 * r2]
    movh        m1, [r0]
    movh        m2, [r0 + r2]
    punpcklqdq  m1, m2
    pmaddwd     m2, m0, coef0                ; DST1
    pmaddwd     m3, m1, coef0
    phaddd      m2, m3
    paddd       m2, m5
    psrad       m2, DST_SHIFT
    pmaddwd     m3, m0, coef1
    pmaddwd     m4, m1, coef1
    phaddd      m3, m4
    paddd       m3, m5
    psrad       m3, DST_SHIFT
    packssdw    m2, m3                       ; m2 = T70
    pmaddwd     m3, m0, coef2
    pmaddwd     m4, m1, coef2
    phaddd      m3, m4
    paddd       m3, m5
    psrad       m3, DST_SHIFT
    pmaddwd     m0, coef3
    pmaddwd     m1, coef3
    phaddd      m0, m1
    paddd       m0, m5
    psrad       m0, DST_SHIFT
    packssdw    m3, m0                       ; m3 = T71
    mova        m5, [pd_128]

    pmaddwd     m0, m2, coef0                ; DST2
    pmaddwd     m1, m3, coef0
    phaddd      m0, m1
    paddd       m0, m5
    psrad       m0, 8
    movu        [r1 + 0 * 16], m0

    pmaddwd     m0, m2, coef1
    pmaddwd     m1, m3, coef1
    phaddd      m0, m1
    paddd       m0, m5
    psrad       m0, 8
    movu        [r1 + 1 * 16], m0

    pmaddwd     m0, m2, coef2
    pmaddwd     m1, m3, coef2
    phaddd      m0, m1
    paddd       m0, m5
    psrad       m0, 8
    movu        [r1 + 2 * 16], m0

    pmaddwd     m2, coef3
    pmaddwd     m3, coef3
    phaddd      m2, m3
    paddd       m2, m5
    psrad       m2, 8
    movu        [r1 + 3 * 16], m2

    RET

;-------------------------------------------------------
;void idst4(int32_t *src, int16_t *dst, intptr_t stride)
;-------------------------------------------------------
INIT_XMM sse2
cglobal idst4, 3, 4, 7
%if BIT_DEPTH == 8
  mova m6, [pd_2048]
  %define IDCT4_SHIFT 12
%elif BIT_DEPTH == 10
  mova m6, [pd_512]
  %define IDCT4_SHIFT 10
%else
  %error Unsupported BIT_DEPTH!
%endif
    add         r2d, r2d
    lea         r3, [tab_idst4]
    mova        m5, [pd_64]

    movu        m0, [r0 + 0 * 16]
    movu        m1, [r0 + 1 * 16]
    packssdw    m0, m1

    movu        m1, [r0 + 2 * 16]
    movu        m2, [r0 + 3 * 16]
    packssdw    m1, m2

    punpcklwd   m2, m0, m1                  ; m2 = m128iAC
    punpckhwd   m0, m1                      ; m0 = m128iBD

    pmaddwd     m1, m2, [r3 + 0 * 16]
    pmaddwd     m3, m0, [r3 + 1 * 16]
    paddd       m1, m3
    paddd       m1, m5
    psrad       m1, 7                       ; m1 = S0

    pmaddwd     m3, m2, [r3 + 2 * 16]
    pmaddwd     m4, m0, [r3 + 3 * 16]
    paddd       m3, m4
    paddd       m3, m5
    psrad       m3, 7                       ; m3 = S8
    packssdw    m1, m3                      ; m1 = m128iA

    pmaddwd     m3, m2, [r3 + 4 * 16]
    pmaddwd     m4, m0, [r3 + 5 * 16]
    paddd       m3, m4
    paddd       m3, m5
    psrad       m3, 7                       ; m3 = S0

    pmaddwd     m2, [r3 + 6 * 16]
    pmaddwd     m0, [r3 + 7 * 16]
    paddd       m2, m0
    paddd       m2, m5
    psrad       m2, 7                       ; m2 = S8
    packssdw    m3, m2                      ; m3 = m128iD

    punpcklwd   m0, m1, m3
    punpckhwd   m1, m3

    punpcklwd   m2, m0, m1
    punpckhwd   m0, m1
    punpcklwd   m1, m2, m0
    punpckhwd   m2, m0
    pmaddwd     m0, m1, [r3 + 0 * 16]
    pmaddwd     m3, m2, [r3 + 1 * 16]
    paddd       m0, m3
    paddd       m0, m6
    psrad       m0, IDCT4_SHIFT             ; m0 = S0
    pmaddwd     m3, m1, [r3 + 2 * 16]
    pmaddwd     m4, m2, [r3 + 3 * 16]
    paddd       m3, m4
    paddd       m3, m6
    psrad       m3, IDCT4_SHIFT             ; m3 = S8
    packssdw    m0, m3                      ; m0 = m128iA
    pmaddwd     m3, m1, [r3 + 4 * 16]
    pmaddwd     m4, m2, [r3 + 5 * 16]
    paddd       m3, m4
    paddd       m3, m6
    psrad       m3, IDCT4_SHIFT             ; m3 = S0
    pmaddwd     m1, [r3 + 6 * 16]
    pmaddwd     m2, [r3 + 7 * 16]
    paddd       m1, m2
    paddd       m1, m6
    psrad       m1, IDCT4_SHIFT             ; m1 = S8
    packssdw    m3, m1                      ; m3 = m128iD
    punpcklwd   m1, m0, m3
    punpckhwd   m0, m3

    punpcklwd   m2, m1, m0
    movlps      [r1 + 0 * r2], m2
    movhps      [r1 + 1 * r2], m2

    punpckhwd   m1, m0
    movlps      [r1 + 2 * r2], m1
    lea         r1, [r1 + 2 * r2]
    movhps      [r1 + r2], m1
    RET


;-------------------------------------------------------
; void dct8(int16_t *src, int32_t *dst, intptr_t stride)
;-------------------------------------------------------
INIT_XMM sse4
cglobal dct8, 3,6,7,0-16*mmsize
    ;------------------------
    ; Stack Mapping(dword)
    ;------------------------
    ; Row0[0-3] Row1[0-3]
    ; ...
    ; Row6[0-3] Row7[0-3]
    ; Row0[0-3] Row7[0-3]
    ; ...
    ; Row6[4-7] Row7[4-7]
    ;------------------------
%if BIT_DEPTH == 10
  %define       DCT_SHIFT 4
  mova          m6, [pd_8]
%elif BIT_DEPTH == 8
  %define       DCT_SHIFT 2
  mova          m6, [pd_2]
%else
  %error Unsupported BIT_DEPTH!
%endif

    add         r2, r2
    lea         r3, [r2 * 3]
    mov         r5, rsp
%assign x 0
%rep 2
    movu        m0, [r0]
    movu        m1, [r0 + r2]
    movu        m2, [r0 + r2 * 2]
    movu        m3, [r0 + r3]

    punpcklwd   m4, m0, m1
    punpckhwd   m0, m1
    punpcklwd   m5, m2, m3
    punpckhwd   m2, m3
    punpckldq   m1, m4, m5          ; m1 = [1 0]
    punpckhdq   m4, m5              ; m4 = [3 2]
    punpckldq   m3, m0, m2
    punpckhdq   m0, m2
    pshufd      m2, m3, 0x4E        ; m2 = [4 5]
    pshufd      m0, m0, 0x4E        ; m0 = [6 7]

    paddw       m3, m1, m0
    psubw       m1, m0              ; m1 = [d1 d0]
    paddw       m0, m4, m2
    psubw       m4, m2              ; m4 = [d3 d2]
    punpcklqdq  m2, m3, m0          ; m2 = [s2 s0]
    punpckhqdq  m3, m0
    pshufd      m3, m3, 0x4E        ; m3 = [s1 s3]

    punpcklwd   m0, m1, m4          ; m0 = [d2/d0]
    punpckhwd   m1, m4              ; m1 = [d3/d1]
    punpckldq   m4, m0, m1          ; m4 = [d3 d1 d2 d0]
    punpckhdq   m0, m1              ; m0 = [d3 d1 d2 d0]

    ; odd
    lea         r4, [tab_dct8_1]
    pmaddwd     m1, m4, [r4 + 0*16]
    pmaddwd     m5, m0, [r4 + 0*16]
    phaddd      m1, m5
    paddd       m1, m6
    psrad       m1, DCT_SHIFT
  %if x == 1
    pshufd      m1, m1, 0x1B
  %endif
    mova        [r5 + 1*2*mmsize], m1 ; Row 1

    pmaddwd     m1, m4, [r4 + 1*16]
    pmaddwd     m5, m0, [r4 + 1*16]
    phaddd      m1, m5
    paddd       m1, m6
    psrad       m1, DCT_SHIFT
  %if x == 1
    pshufd      m1, m1, 0x1B
  %endif
    mova        [r5 + 3*2*mmsize], m1 ; Row 3

    pmaddwd     m1, m4, [r4 + 2*16]
    pmaddwd     m5, m0, [r4 + 2*16]
    phaddd      m1, m5
    paddd       m1, m6
    psrad       m1, DCT_SHIFT
  %if x == 1
    pshufd      m1, m1, 0x1B
  %endif
    mova        [r5 + 5*2*mmsize], m1 ; Row 5

    pmaddwd     m4, [r4 + 3*16]
    pmaddwd     m0, [r4 + 3*16]
    phaddd      m4, m0
    paddd       m4, m6
    psrad       m4, DCT_SHIFT
  %if x == 1
    pshufd      m4, m4, 0x1B
  %endif
    mova        [r5 + 7*2*mmsize], m4; Row 7

    ; even
    lea         r4, [tab_dct4]
    paddw       m0, m2, m3          ; m0 = [EE1 EE0]
    pshufb      m0, [pb_unpackhlw1]
    psubw       m2, m3              ; m2 = [EO1 EO0]
    psignw      m2, [pw_ppppmmmm]
    pshufb      m2, [pb_unpackhlw1]
    pmaddwd     m3, m0, [r4 + 0*16]
    paddd       m3, m6
    psrad       m3, DCT_SHIFT
  %if x == 1
    pshufd      m3, m3, 0x1B
  %endif
    mova        [r5 + 0*2*mmsize], m3 ; Row 0
    pmaddwd     m0, [r4 + 2*16]
    paddd       m0, m6
    psrad       m0, DCT_SHIFT
  %if x == 1
    pshufd      m0, m0, 0x1B
  %endif
    mova        [r5 + 4*2*mmsize], m0 ; Row 4
    pmaddwd     m3, m2, [r4 + 1*16]
    paddd       m3, m6
    psrad       m3, DCT_SHIFT
  %if x == 1
    pshufd      m3, m3, 0x1B
  %endif
    mova        [r5 + 2*2*mmsize], m3 ; Row 2
    pmaddwd     m2, [r4 + 3*16]
    paddd       m2, m6
    psrad       m2, DCT_SHIFT
  %if x == 1
    pshufd      m2, m2, 0x1B
  %endif
    mova        [r5 + 6*2*mmsize], m2 ; Row 6

  %if x != 1
    lea         r0, [r0 + r2 * 4]
    add         r5, mmsize
  %endif
%assign x x+1
%endrep

    mov         r2, 2
    mov         r0, rsp                 ; r0 = pointer to Low Part
    lea         r4, [tab_dct8_2]
    mova        m6, [pd_256]

.pass2:
%rep 2
    mova        m0, [r0 + 0*2*mmsize]     ; [3 2 1 0]
    mova        m1, [r0 + 1*2*mmsize]
    paddd       m2, m0, [r0 + (0*2+1)*mmsize]
    pshufd      m2, m2, 0x9C            ; m2 = [s2 s1 s3 s0]
    paddd       m3, m1, [r0 + (1*2+1)*mmsize]
    pshufd      m3, m3, 0x9C            ; m3 = ^^
    psubd       m0, [r0 + (0*2+1)*mmsize]     ; m0 = [d3 d2 d1 d0]
    psubd       m1, [r0 + (1*2+1)*mmsize]     ; m1 = ^^

    ; even
    phaddd      m4, m2, m3              ; m4 = [EE1 EE0 EE1 EE0]
    phsubd      m2, m3                  ; m2 = [EO1 EO0 EO1 EO0]

    pslld       m4, 6                   ; m4 = [64*EE1 64*EE0]
    pmulld      m5, m2, [r4 + 0*16]     ; m5 = [36*EO1 83*EO0]
    pmulld      m2, [r4 + 1*16]         ; m2 = [83*EO1 36*EO0]

    phaddd      m3, m4, m5              ; m3 = [Row2 Row0]
    paddd       m3, m6
    psrad       m3, 9
    phsubd      m4, m2                  ; m4 = [Row6 Row4]
    paddd       m4, m6
    psrad       m4, 9
    movh        [r1 + 0*2*mmsize], m3
    movhps      [r1 + 2*2*mmsize], m3
    movh        [r1 + 4*2*mmsize], m4
    movhps      [r1 + 6*2*mmsize], m4

    ; odd
    pmulld      m2, m0, [r4 + 2*16]
    pmulld      m3, m1, [r4 + 2*16]
    pmulld      m4, m0, [r4 + 3*16]
    pmulld      m5, m1, [r4 + 3*16]
    phaddd      m2, m3
    phaddd      m4, m5
    phaddd      m2, m4                  ; m2 = [Row3 Row1]
    paddd       m2, m6
    psrad       m2, 9
    movh        [r1 + 1*2*mmsize], m2
    movhps      [r1 + 3*2*mmsize], m2

    pmulld      m2, m0, [r4 + 4*16]
    pmulld      m3, m1, [r4 + 4*16]
    pmulld      m4, m0, [r4 + 5*16]
    pmulld      m5, m1, [r4 + 5*16]
    phaddd      m2, m3
    phaddd      m4, m5
    phaddd      m2, m4                  ; m2 = [Row7 Row5]
    paddd       m2, m6
    psrad       m2, 9
    movh        [r1 + 5*2*mmsize], m2
    movhps      [r1 + 7*2*mmsize], m2

    add         r1, mmsize/2
    add         r0, 2*2*mmsize
%endrep

    dec         r2
    jnz        .pass2
    RET

;-------------------------------------------------------
; void idct8(int32_t *src, int16_t *dst, intptr_t stride)
;-------------------------------------------------------
INIT_XMM ssse3

cglobal patial_butterfly_inverse_internal_pass1
    movu        m0, [r0]
    movu        m1, [r0 + 4 * 32]
    movu        m2, [r0 + 2 * 32]
    movu        m3, [r0 + 6 * 32]
    packssdw    m0, m2
    packssdw    m1, m3
    punpckhwd   m2, m0, m1                  ; [2 6]
    punpcklwd   m0, m1                      ; [0 4]
    pmaddwd     m1, m0, [r6]                ; EE[0]
    pmaddwd     m0, [r6 + 32]               ; EE[1]
    pmaddwd     m3, m2, [r6 + 16]           ; EO[0]
    pmaddwd     m2, [r6 + 48]               ; EO[1]

    paddd       m4, m1, m3                  ; E[0]
    psubd       m1, m3                      ; E[3]
    paddd       m3, m0, m2                  ; E[1]
    psubd       m0, m2                      ; E[2]

    ;E[K] = E[k] + add
    mova        m5, [pd_64]
    paddd       m0, m5
    paddd       m1, m5
    paddd       m3, m5
    paddd       m4, m5

    movu        m2, [r0 + 32]
    movu        m5, [r0 + 5 * 32]
    packssdw    m2, m5
    movu        m5, [r0 + 3 * 32]
    movu        m6, [r0 + 7 * 32]
    packssdw    m5, m6
    punpcklwd   m6, m2, m5                  ;[1 3]
    punpckhwd   m2, m5                      ;[5 7]

    pmaddwd     m5, m6, [r4]
    pmaddwd     m7, m2, [r4 + 16]
    paddd       m5, m7                      ; O[0]

    paddd       m7, m4, m5
    psrad       m7, 7

    psubd       m4, m5
    psrad       m4, 7

    packssdw    m7, m4
    movh        [r5 + 0 * 16], m7
    movhps      [r5 + 7 * 16], m7

    pmaddwd     m5, m6, [r4 + 32]
    pmaddwd     m4, m2, [r4 + 48]
    paddd       m5, m4                      ; O[1]

    paddd       m4, m3, m5
    psrad       m4, 7

    psubd       m3, m5
    psrad       m3, 7

    packssdw    m4, m3
    movh        [r5 + 1 * 16], m4
    movhps      [r5 + 6 * 16], m4

    pmaddwd     m5, m6, [r4 + 64]
    pmaddwd     m4, m2, [r4 + 80]
    paddd       m5, m4                      ; O[2]

    paddd       m4, m0, m5
    psrad       m4, 7

    psubd       m0, m5
    psrad       m0, 7

    packssdw    m4, m0
    movh        [r5 + 2 * 16], m4
    movhps      [r5 + 5 * 16], m4

    pmaddwd     m5, m6, [r4 + 96]
    pmaddwd     m4, m2, [r4 + 112]
    paddd       m5, m4                      ; O[3]

    paddd       m4, m1, m5
    psrad       m4, 7

    psubd       m1, m5
    psrad       m1, 7

    packssdw    m4, m1
    movh        [r5 + 3 * 16], m4
    movhps      [r5 + 4 * 16], m4

    ret

%macro PARTIAL_BUTTERFLY_PROCESS_ROW 1
%if BIT_DEPTH == 10
    %define     IDCT_SHIFT 10
%elif BIT_DEPTH == 8
    %define     IDCT_SHIFT 12
%else
    %error Unsupported BIT_DEPTH!
%endif
    pshufb      m4, %1, [pb_idct8even]
    pmaddwd     m4, [tab_idct8_1]
    phsubd      m5, m4
    pshufd      m4, m4, 0x4E
    phaddd      m4, m4
    punpckhqdq  m4, m5                      ;m4 = dd e[ 0 1 2 3]
    paddd       m4, m6

    pshufb      %1, %1, [r6]
    pmaddwd     m5, %1, [r4]
    pmaddwd     %1, [r4 + 16]
    phaddd      m5, %1                      ; m5 = dd O[0, 1, 2, 3]

    paddd       %1, m4, m5
    psrad       %1, IDCT_SHIFT

    psubd       m4, m5
    psrad       m4, IDCT_SHIFT
    pshufd      m4, m4, 0x1B

    packssdw    %1, m4
%undef IDCT_SHIFT
%endmacro

cglobal patial_butterfly_inverse_internal_pass2

    mova        m0, [r5]
    PARTIAL_BUTTERFLY_PROCESS_ROW m0
    movu        [r1], m0

    mova        m2, [r5 + 16]
    PARTIAL_BUTTERFLY_PROCESS_ROW m2
    movu        [r1 + r2], m2

    mova        m1, [r5 + 32]
    PARTIAL_BUTTERFLY_PROCESS_ROW m1
    movu        [r1 + 2 * r2], m1

    mova        m3, [r5 + 48]
    PARTIAL_BUTTERFLY_PROCESS_ROW m3
    movu        [r1 + r3], m3

    ret

cglobal idct8, 3,7,8 ;,0-16*mmsize
    ; alignment stack to 64-bytes
    mov         r5, rsp
    sub         rsp, 16*mmsize + gprsize
    and         rsp, ~(64-1)
    mov         [rsp + 16*mmsize], r5
    mov         r5, rsp

    lea         r4, [tab_idct8_3]
    lea         r6, [tab_dct4]

    call        patial_butterfly_inverse_internal_pass1

    add         r0, 16
    add         r5, 8

    call        patial_butterfly_inverse_internal_pass1

%if BIT_DEPTH == 10
    mova        m6, [pd_512]
%elif BIT_DEPTH == 8
    mova        m6, [pd_2048]
%else
  %error Unsupported BIT_DEPTH!
%endif
    add         r2, r2
    lea         r3, [r2 * 3]
    lea         r4, [tab_idct8_2]
    lea         r6, [pb_idct8odd]
    sub         r5, 8

    call        patial_butterfly_inverse_internal_pass2

    lea         r1, [r1 + 4 * r2]
    add         r5, 64

    call        patial_butterfly_inverse_internal_pass2

    ; restore origin stack pointer
    mov         rsp, [rsp + 16*mmsize]
    RET


;-----------------------------------------------------------------------------
; void denoise_dct(int32_t *dct, uint32_t *sum, uint16_t *offset, int size)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal denoise_dct, 4, 4, 6
    pxor     m5,  m5
    shr      r3d, 2
.loop:
    mova     m0, [r0]
    pabsd    m1, m0
    mova     m2, [r1]
    paddd    m2, m1
    mova     [r1], m2
    pmovzxwd m3, [r2]
    psubd    m1, m3
    pcmpgtd  m4, m1, m5
    pand     m1, m4
    psignd   m1, m0
    mova     [r0], m1
    add      r0, 16
    add      r1, 16
    add      r2, 8
    dec      r3d
    jnz .loop
    RET

INIT_YMM avx2
cglobal denoise_dct, 4, 4, 6
    pxor     m5,  m5
    shr      r3d, 3
.loop:
    movu     m0, [r0]
    pabsd    m1, m0
    movu     m2, [r1]
    paddd    m2, m1
    movu     [r1], m2
    pmovzxwd m3, [r2]
    psubd    m1, m3
    pcmpgtd  m4, m1, m5
    pand     m1, m4
    psignd   m1, m0
    movu     [r0], m1
    add      r0, 32
    add      r1, 32
    add      r2, 16
    dec      r3d
    jnz .loop
    RET
%if ARCH_X86_64 == 1
%macro DCT8_PASS_1 4
    vpbroadcastq    m0,                 [r6 + %1]
    pmaddwd         m2,                 m%3, m0
    pmaddwd         m0,                 m%4
    phaddd          m2,                 m0
    paddd           m2,                 m5
    psrad           m2,                 DCT_SHIFT
    packssdw        m2,                 m2
    vpermq          m2,                 m2, 0x08
    mova            [r5 + %2],          xm2
%endmacro

%macro DCT8_PASS_2 1
    vbroadcasti128  m4,                 [r6 + %1]
    pmaddwd         m6,                 m0, m4
    pmaddwd         m7,                 m1, m4
    pmaddwd         m8,                 m2, m4
    pmaddwd         m9,                 m3, m4
    phaddd          m6,                 m7
    phaddd          m8,                 m9
    phaddd          m6,                 m8
    paddd           m6,                 m5
    psrad           m6,                 DCT_SHIFT2
%endmacro

INIT_YMM avx2
cglobal dct8, 3, 7, 10, 0-8*16
%if BIT_DEPTH == 10
    %define         DCT_SHIFT          4
    vbroadcasti128  m5,                [pd_8]
%elif BIT_DEPTH == 8
    %define         DCT_SHIFT          2
    vbroadcasti128  m5,                [pd_2]
%else
    %error Unsupported BIT_DEPTH!
%endif
%define             DCT_SHIFT2         9

    add             r2d,               r2d
    lea             r3,                [r2 * 3]
    lea             r4,                [r0 + r2 * 4]
    mov             r5,                rsp
    lea             r6,                [tab_dct8]
    mova            m6,                [dct8_shuf]

    ;pass1
    mova            xm0,               [r0]
    vinserti128     m0,                m0, [r4], 1
    mova            xm1,               [r0 + r2]
    vinserti128     m1,                m1, [r4 + r2], 1
    mova            xm2,               [r0 + r2 * 2]
    vinserti128     m2,                m2, [r4 + r2 * 2], 1
    mova            xm3,               [r0 + r3]
    vinserti128     m3,                m3,  [r4 + r3], 1

    punpcklqdq      m4,                m0, m1
    punpckhqdq      m0,                m1
    punpcklqdq      m1,                m2, m3
    punpckhqdq      m2,                m3

    pshufb          m0,                m6
    pshufb          m2,                m6

    paddw           m3,                m4, m0
    paddw           m7,                m1, m2

    psubw           m4,                m0
    psubw           m1,                m2

    DCT8_PASS_1     0 * 16,             0 * 16, 3, 7
    DCT8_PASS_1     1 * 16,             2 * 16, 4, 1
    DCT8_PASS_1     2 * 16,             4 * 16, 3, 7
    DCT8_PASS_1     3 * 16,             6 * 16, 4, 1
    DCT8_PASS_1     4 * 16,             1 * 16, 3, 7
    DCT8_PASS_1     5 * 16,             3 * 16, 4, 1
    DCT8_PASS_1     6 * 16,             5 * 16, 3, 7
    DCT8_PASS_1     7 * 16,             7 * 16, 4, 1

    ;pass2
    mov             r2d,               32
    lea             r3,                [r2 * 3]
    lea             r4,                [r1 + r2 * 4]
    vbroadcasti128  m5,                [pd_256]

    mova            m0,                [r5]
    mova            m1,                [r5 + 32]
    mova            m2,                [r5 + 64]
    mova            m3,                [r5 + 96]

    DCT8_PASS_2     0 * 16
    movu            [r1],              m6
    DCT8_PASS_2     1 * 16
    movu            [r1 + r2],         m6
    DCT8_PASS_2     2 * 16
    movu            [r1 + r2 * 2],     m6
    DCT8_PASS_2     3 * 16
    movu            [r1 + r3],         m6
    DCT8_PASS_2     4 * 16
    movu            [r4],              m6
    DCT8_PASS_2     5 * 16
    movu            [r4 + r2],         m6
    DCT8_PASS_2     6 * 16
    movu            [r4 + r2 * 2],     m6
    DCT8_PASS_2     7 * 16
    movu            [r4 + r3],         m6
    RET

%macro DCT16_PASS_1_E 2
    vpbroadcastq    m7,                [r7 + %1]

    pmaddwd         m4,                m0, m7
    pmaddwd         m6,                m2, m7
    phaddd          m4,                m6

    paddd           m4,                m9
    psrad           m4,                DCT_SHIFT

    packssdw        m4,                m4
    vpermq          m4,                m4, 0x08

    mova            [r5 + %2],         xm4
%endmacro

%macro DCT16_PASS_1_O 2
    vbroadcasti128  m7,                [r7 + %1]

    pmaddwd         m10,               m0, m7
    pmaddwd         m11,               m2, m7
    phaddd          m10,               m11                 ; [d0 d0 d1 d1 d4 d4 d5 d5]

    pmaddwd         m11,               m4, m7
    pmaddwd         m12,               m6, m7
    phaddd          m11,               m12                 ; [d2 d2 d3 d3 d6 d6 d7 d7]

    phaddd          m10,               m11                 ; [d0 d1 d2 d3 d4 d5 d6 d7]

    paddd           m10,               m9
    psrad           m10,               DCT_SHIFT

    packssdw        m10,               m10                 ; [w0 w1 w2 w3 - - - - w4 w5 w6 w7 - - - -]
    vpermq          m10,               m10, 0x08

    mova            [r5 + %2],         xm10
%endmacro

%macro DCT16_PASS_2 1
    vbroadcasti128  m8,                [r7 + %1]
    vbroadcasti128  m13,               [r8 + %1]

    pmaddwd         m10,               m0, m8
    pmaddwd         m11,               m1, m13
    paddd           m10,               m11

    pmaddwd         m11,               m2, m8
    pmaddwd         m12,               m3, m13
    paddd           m11,               m12
    phaddd          m10,               m11

    pmaddwd         m11,               m4, m8
    pmaddwd         m12,               m5, m13
    paddd           m11,               m12

    pmaddwd         m12,               m6, m8
    pmaddwd         m13,               m7, m13
    paddd           m12,               m13
    phaddd          m11,               m12

    phaddd          m10,               m11
    paddd           m10,               m9
    psrad           m10,               DCT_SHIFT2
%endmacro
INIT_YMM avx2
cglobal dct16, 3, 9, 15, 0-16*mmsize
%if BIT_DEPTH == 10
    %define         DCT_SHIFT          5
    vbroadcasti128  m9,                [pd_16]
%elif BIT_DEPTH == 8
    %define         DCT_SHIFT          3
    vbroadcasti128  m9,                [pd_4]
%else
    %error Unsupported BIT_DEPTH!
%endif
%define             DCT_SHIFT2         10

    add             r2d,               r2d

    mova            m13,               [dct16_shuf1]
    mova            m14,               [dct16_shuf2]
    lea             r7,                [tab_dct16_1 + 8 * 16]
    lea             r8,                [tab_dct16_2 + 8 * 16]
    lea             r3,                [r2 * 3]
    mov             r5,                rsp
    mov             r4d,               2                   ; Each iteration process 8 rows, so 16/8 iterations

.pass1:
    lea             r6,                [r0 + r2 * 4]

    movu            m2,                [r0]
    movu            m1,                [r6]
    vperm2i128      m0,                m2, m1, 0x20        ; [row0lo  row4lo]
    vperm2i128      m1,                m2, m1, 0x31        ; [row0hi  row4hi]

    movu            m4,                [r0 + r2]
    movu            m3,                [r6 + r2]
    vperm2i128      m2,                m4, m3, 0x20        ; [row1lo  row5lo]
    vperm2i128      m3,                m4, m3, 0x31        ; [row1hi  row5hi]

    movu            m6,                [r0 + r2 * 2]
    movu            m5,                [r6 + r2 * 2]
    vperm2i128      m4,                m6, m5, 0x20        ; [row2lo  row6lo]
    vperm2i128      m5,                m6, m5, 0x31        ; [row2hi  row6hi]

    movu            m8,                [r0 + r3]
    movu            m7,                [r6 + r3]
    vperm2i128      m6,                m8, m7, 0x20        ; [row3lo  row7lo]
    vperm2i128      m7,                m8, m7, 0x31        ; [row3hi  row7hi]

    pshufb          m1,                m13
    pshufb          m3,                m13
    pshufb          m5,                m13
    pshufb          m7,                m13

    paddw           m8,                m0, m1              ;E
    psubw           m0,                m1                  ;O

    paddw           m1,                m2, m3              ;E
    psubw           m2,                m3                  ;O

    paddw           m3,                m4, m5              ;E
    psubw           m4,                m5                  ;O

    paddw           m5,                m6, m7              ;E
    psubw           m6,                m7                  ;O

    DCT16_PASS_1_O  -7 * 16,           1 * 32
    DCT16_PASS_1_O  -5 * 16,           3 * 32
    DCT16_PASS_1_O  -3 * 16,           1 * 32 + 16
    DCT16_PASS_1_O  -1 * 16,           3 * 32 + 16
    DCT16_PASS_1_O  1 * 16,            5 * 32
    DCT16_PASS_1_O  3 * 16,            7 * 32
    DCT16_PASS_1_O  5 * 16,            5 * 32 + 16
    DCT16_PASS_1_O  7 * 16,            7 * 32 + 16

    pshufb          m8,                m14
    pshufb          m1,                m14
    phaddw          m0,                m8, m1

    pshufb          m3,                m14
    pshufb          m5,                m14
    phaddw          m2,                m3, m5

    DCT16_PASS_1_E  -8 * 16,           0 * 32
    DCT16_PASS_1_E  -4 * 16,           0 * 32 + 16
    DCT16_PASS_1_E  0 * 16,            4 * 32
    DCT16_PASS_1_E  4 * 16,            4 * 32 + 16

    phsubw          m0,                m8, m1
    phsubw          m2,                m3, m5

    DCT16_PASS_1_E  -6 * 16,           2 * 32
    DCT16_PASS_1_E  -2 * 16,           2 * 32 + 16
    DCT16_PASS_1_E  2 * 16,            6 * 32
    DCT16_PASS_1_E  6 * 16,            6 * 32 + 16

    lea             r0,                [r0 + 8 * r2]
    add             r5,                256

    dec             r4d
    jnz             .pass1

    mov             r5,                rsp
    mov             r4d,               2
    mov             r2d,               64
    lea             r3,                [r2 * 3]
    vbroadcasti128  m9,                [pd_512]

.pass2:
    mova            m0,                [r5 + 0 * 32]        ; [row0lo  row4lo]
    mova            m1,                [r5 + 8 * 32]        ; [row0hi  row4hi]

    mova            m2,                [r5 + 1 * 32]        ; [row1lo  row5lo]
    mova            m3,                [r5 + 9 * 32]        ; [row1hi  row5hi]

    mova            m4,                [r5 + 2 * 32]        ; [row2lo  row6lo]
    mova            m5,                [r5 + 10 * 32]       ; [row2hi  row6hi]

    mova            m6,                [r5 + 3 * 32]        ; [row3lo  row7lo]
    mova            m7,                [r5 + 11 * 32]       ; [row3hi  row7hi]

    DCT16_PASS_2    -8 * 16
    movu            [r1],              m10
    DCT16_PASS_2    -7 * 16
    movu            [r1 + r2],         m10
    DCT16_PASS_2    -6 * 16
    movu            [r1 + r2 * 2],     m10
    DCT16_PASS_2    -5 * 16
    movu            [r1 + r3],         m10

    lea             r6,                [r1 + r2 * 4]
    DCT16_PASS_2    -4 * 16
    movu            [r6],              m10
    DCT16_PASS_2    -3 * 16
    movu            [r6 + r2],         m10
    DCT16_PASS_2    -2 * 16
    movu            [r6 + r2 * 2],     m10
    DCT16_PASS_2    -1 * 16
    movu            [r6 + r3],         m10

    lea             r6,                [r6 + r2 * 4]
    DCT16_PASS_2    0 * 16
    movu            [r6],              m10
    DCT16_PASS_2    1 * 16
    movu            [r6 + r2],         m10
    DCT16_PASS_2    2 * 16
    movu            [r6 + r2 * 2],     m10
    DCT16_PASS_2    3 * 16
    movu            [r6 + r3],         m10

    lea             r6,                [r6 + r2 * 4]
    DCT16_PASS_2    4 * 16
    movu            [r6],              m10
    DCT16_PASS_2    5 * 16
    movu            [r6 + r2],         m10
    DCT16_PASS_2    6 * 16
    movu            [r6 + r2 * 2],     m10
    DCT16_PASS_2    7 * 16
    movu            [r6 + r3],         m10

    add             r1,                32
    add             r5,                128

    dec             r4d
    jnz             .pass2
    RET

%macro DCT32_PASS_1 4
    vbroadcasti128  m8,                [r7 + %1]

    pmaddwd         m11,               m%3, m8
    pmaddwd         m12,               m%4, m8
    phaddd          m11,               m12

    vbroadcasti128  m8,                [r7 + %1 + 32]
    vbroadcasti128  m10,               [r7 + %1 + 48]
    pmaddwd         m12,               m5, m8
    pmaddwd         m13,               m6, m10
    phaddd          m12,               m13

    pmaddwd         m13,               m4, m8
    pmaddwd         m14,               m7, m10
    phaddd          m13,               m14

    phaddd          m12,               m13

    phaddd          m11,               m12
    paddd           m11,               m9
    psrad           m11,               DCT_SHIFT

    vpermq          m11,               m11, 0xD8
    packssdw        m11,               m11
    movq            [r5 + %2],         xm11
    vextracti128    xm10,              m11, 1
    movq            [r5 + %2 + 64],    xm10
%endmacro

%macro DCT32_PASS_2 1
    mova            m8,                [r7 + %1]
    mova            m10,               [r8 + %1]
    pmaddwd         m11,               m0, m8
    pmaddwd         m12,               m1, m10
    paddd           m11,               m12

    pmaddwd         m12,               m2, m8
    pmaddwd         m13,               m3, m10
    paddd           m12,               m13

    phaddd          m11,               m12

    pmaddwd         m12,               m4, m8
    pmaddwd         m13,               m5, m10
    paddd           m12,               m13

    pmaddwd         m13,               m6, m8
    pmaddwd         m14,               m7, m10
    paddd           m13,               m14

    phaddd          m12,               m13

    phaddd          m11,               m12
    vextracti128    xm10,              m11, 1
    paddd           xm11,              xm10

    paddd           xm11,               xm9
    psrad           xm11,               DCT_SHIFT2

%endmacro

INIT_YMM avx2
cglobal dct32, 3, 9, 16, 0-64*mmsize
%if BIT_DEPTH == 10
    %define         DCT_SHIFT          6
    vpbroadcastq    m9,                [pd_32]
%elif BIT_DEPTH == 8
    %define         DCT_SHIFT          4
    vpbroadcastq    m9,                [pd_8]
%else
    %error Unsupported BIT_DEPTH!
%endif
%define             DCT_SHIFT2         11

    add             r2d,               r2d

    lea             r7,                [tab_dct32_1]
    lea             r8,                [tab_dct32_2]
    lea             r3,                [r2 * 3]
    mov             r5,                rsp
    mov             r4d,               8
    mova            m15,               [dct16_shuf1]

.pass1:
    movu            m2,                [r0]
    movu            m1,                [r0 + 32]
    pshufb          m1,                m15
    vpermq          m1,                m1, 0x4E
    psubw           m7,                m2, m1
    paddw           m2,                m1

    movu            m1,                [r0 + r2 * 2]
    movu            m0,                [r0 + r2 * 2 + 32]
    pshufb          m0,                m15
    vpermq          m0,                m0, 0x4E
    psubw           m8,                m1, m0
    paddw           m1,                m0
    vperm2i128      m0,                m2, m1, 0x20        ; [row0lo  row2lo] for E
    vperm2i128      m3,                m2, m1, 0x31        ; [row0hi  row2hi] for E
    pshufb          m3,                m15
    psubw           m1,                m0, m3
    paddw           m0,                m3

    vperm2i128      m5,                m7, m8, 0x20        ; [row0lo  row2lo] for O
    vperm2i128      m6,                m7, m8, 0x31        ; [row0hi  row2hi] for O


    movu            m4,                [r0 + r2]
    movu            m2,                [r0 + r2 + 32]
    pshufb          m2,                m15
    vpermq          m2,                m2, 0x4E
    psubw           m10,               m4, m2
    paddw           m4,                m2

    movu            m3,                [r0 + r3]
    movu            m2,                [r0 + r3 + 32]
    pshufb          m2,                m15
    vpermq          m2,                m2, 0x4E
    psubw           m11,               m3, m2
    paddw           m3,                m2
    vperm2i128      m2,                m4, m3, 0x20        ; [row1lo  row3lo] for E
    vperm2i128      m8,                m4, m3, 0x31        ; [row1hi  row3hi] for E
    pshufb          m8,                m15
    psubw           m3,                m2, m8
    paddw           m2,                m8

    vperm2i128      m4,                m10, m11, 0x20      ; [row1lo  row3lo] for O
    vperm2i128      m7,                m10, m11, 0x31      ; [row1hi  row3hi] for O


    DCT32_PASS_1    0 * 32,            0 * 64, 0, 2
    DCT32_PASS_1    2 * 32,            2 * 64, 1, 3
    DCT32_PASS_1    4 * 32,            4 * 64, 0, 2
    DCT32_PASS_1    6 * 32,            6 * 64, 1, 3
    DCT32_PASS_1    8 * 32,            8 * 64, 0, 2
    DCT32_PASS_1    10 * 32,           10 * 64, 1, 3
    DCT32_PASS_1    12 * 32,           12 * 64, 0, 2
    DCT32_PASS_1    14 * 32,           14 * 64, 1, 3
    DCT32_PASS_1    16 * 32,           16 * 64, 0, 2
    DCT32_PASS_1    18 * 32,           18 * 64, 1, 3
    DCT32_PASS_1    20 * 32,           20 * 64, 0, 2
    DCT32_PASS_1    22 * 32,           22 * 64, 1, 3
    DCT32_PASS_1    24 * 32,           24 * 64, 0, 2
    DCT32_PASS_1    26 * 32,           26 * 64, 1, 3
    DCT32_PASS_1    28 * 32,           28 * 64, 0, 2
    DCT32_PASS_1    30 * 32,           30 * 64, 1, 3

    add             r5,                8
    lea             r0,                [r0 + r2 * 4]

    dec             r4d
    jnz             .pass1

    mov             r2d,               128
    lea             r3,                [r2 * 3]
    mov             r5,                rsp
    mov             r4d,               8
    vpbroadcastq    m9,                [pd_1024]

.pass2:
    mova            m0,                [r5 + 0 * 64]
    mova            m1,                [r5 + 0 * 64 + 32]

    mova            m2,                [r5 + 1 * 64]
    mova            m3,                [r5 + 1 * 64 + 32]

    mova            m4,                [r5 + 2 * 64]
    mova            m5,                [r5 + 2 * 64 + 32]

    mova            m6,                [r5 + 3 * 64]
    mova            m7,                [r5 + 3 * 64 + 32]

    DCT32_PASS_2    0 * 32
    movu            [r1],              xm11
    DCT32_PASS_2    1 * 32
    movu            [r1 + r2],         xm11
    DCT32_PASS_2    2 * 32
    movu            [r1 + r2 * 2],     xm11
    DCT32_PASS_2    3 * 32
    movu            [r1 + r3],         xm11

    lea             r6,                [r1 + r2 * 4]
    DCT32_PASS_2    4 * 32
    movu            [r6],              xm11
    DCT32_PASS_2    5 * 32
    movu            [r6 + r2],         xm11
    DCT32_PASS_2    6 * 32
    movu            [r6 + r2 * 2],     xm11
    DCT32_PASS_2    7 * 32
    movu            [r6 + r3],         xm11

    lea             r6,                [r6 + r2 * 4]
    DCT32_PASS_2    8 * 32
    movu            [r6],              xm11
    DCT32_PASS_2    9 * 32
    movu            [r6 + r2],         xm11
    DCT32_PASS_2    10 * 32
    movu            [r6 + r2 * 2],     xm11
    DCT32_PASS_2    11 * 32
    movu            [r6 + r3],         xm11

    lea             r6,                [r6 + r2 * 4]
    DCT32_PASS_2    12 * 32
    movu            [r6],              xm11
    DCT32_PASS_2    13 * 32
    movu            [r6 + r2],         xm11
    DCT32_PASS_2    14 * 32
    movu            [r6 + r2 * 2],     xm11
    DCT32_PASS_2    15 * 32
    movu            [r6 + r3],         xm11

    lea             r6,                [r6 + r2 * 4]
    DCT32_PASS_2    16 * 32
    movu            [r6],              xm11
    DCT32_PASS_2    17 * 32
    movu            [r6 + r2],         xm11
    DCT32_PASS_2    18 * 32
    movu            [r6 + r2 * 2],     xm11
    DCT32_PASS_2    19 * 32
    movu            [r6 + r3],         xm11

    lea             r6,                [r6 + r2 * 4]
    DCT32_PASS_2    20 * 32
    movu            [r6],              xm11
    DCT32_PASS_2    21 * 32
    movu            [r6 + r2],         xm11
    DCT32_PASS_2    22 * 32
    movu            [r6 + r2 * 2],     xm11
    DCT32_PASS_2    23 * 32
    movu            [r6 + r3],         xm11

    lea             r6,                [r6 + r2 * 4]
    DCT32_PASS_2    24 * 32
    movu            [r6],              xm11
    DCT32_PASS_2    25 * 32
    movu            [r6 + r2],         xm11
    DCT32_PASS_2    26 * 32
    movu            [r6 + r2 * 2],     xm11
    DCT32_PASS_2    27 * 32
    movu            [r6 + r3],         xm11

    lea             r6,                [r6 + r2 * 4]
    DCT32_PASS_2    28 * 32
    movu            [r6],              xm11
    DCT32_PASS_2    29 * 32
    movu            [r6 + r2],         xm11
    DCT32_PASS_2    30 * 32
    movu            [r6 + r2 * 2],     xm11
    DCT32_PASS_2    31 * 32
    movu            [r6 + r3],         xm11

    add             r5,                256
    add             r1,                16

    dec             r4d
    jnz             .pass2
    RET

%macro IDCT_PASS1 2
    vbroadcasti128  m5, [tab_idct16_2 + %1 * 16]

    pmaddwd         m9, m0, m5
    pmaddwd         m10, m7, m5
    phaddd          m9, m10

    pmaddwd         m10, m6, m5
    pmaddwd         m11, m8, m5
    phaddd          m10, m11

    phaddd          m9, m10
    vbroadcasti128  m5, [tab_idct16_1 + %1 * 16]

    pmaddwd         m10, m1, m5
    pmaddwd         m11, m3, m5
    phaddd          m10, m11

    pmaddwd         m11, m4, m5
    pmaddwd         m12, m2, m5
    phaddd          m11, m12

    phaddd          m10, m11

    paddd           m11, m9, m10
    paddd           m11, m14
    psrad           m11, IDCT_SHIFT1

    psubd           m9, m10
    paddd           m9, m14
    psrad           m9, IDCT_SHIFT1

    vbroadcasti128  m5, [tab_idct16_2 + %1 * 16 + 16]

    pmaddwd         m10, m0, m5
    pmaddwd         m12, m7, m5
    phaddd          m10, m12

    pmaddwd         m12, m6, m5
    pmaddwd         m13, m8, m5
    phaddd          m12, m13

    phaddd          m10, m12
    vbroadcasti128  m5, [tab_idct16_1 + %1 * 16  + 16]

    pmaddwd         m12, m1, m5
    pmaddwd         m13, m3, m5
    phaddd          m12, m13

    pmaddwd         m13, m4, m5
    pmaddwd         m5, m2
    phaddd          m13, m5

    phaddd          m12, m13

    paddd           m5, m10, m12
    paddd           m5, m14
    psrad           m5, IDCT_SHIFT1

    psubd           m10, m12
    paddd           m10, m14
    psrad           m10, IDCT_SHIFT1

    packssdw        m11, m5
    packssdw        m9, m10

    mova            m10, [idct16_shuff]
    mova            m5,  [idct16_shuff1]

    vpermd          m12, m10, m11
    vpermd          m13, m5, m9
    mova            [r3 + %1 * 16 * 2], xm12
    mova            [r3 + %2 * 16 * 2], xm13
    vextracti128    [r3 + %2 * 16 * 2 + 32], m13, 1
    vextracti128    [r3 + %1 * 16 * 2 + 32], m12, 1
%endmacro

;-------------------------------------------------------
; void idct16(int32_t *src, int16_t *dst, intptr_t stride)
;-------------------------------------------------------
INIT_YMM avx2
cglobal idct16, 3, 7, 16, 0-16*mmsize
%if BIT_DEPTH == 10
    %define         IDCT_SHIFT2        10
    vpbroadcastd    m15,                [pd_512]
%elif BIT_DEPTH == 8
    %define         IDCT_SHIFT2        12
    vpbroadcastd    m15,                [pd_2048]
%else
    %error Unsupported BIT_DEPTH!
%endif
%define             IDCT_SHIFT1         7

    vbroadcasti128  m14,               [pd_64]

    add             r2d,               r2d
    mov             r3, rsp
    mov             r4d, 2

.pass1:
    movu            m0, [r0 +  0 * 64]
    movu            m1, [r0 +  8 * 64]
    packssdw        m0, m1                    ;[0L 8L 0H 8H]

    movu            m1, [r0 +  1 * 64]
    movu            m2, [r0 +  9 * 64]
    packssdw        m1, m2                    ;[1L 9L 1H 9H]

    movu            m2, [r0 +  2 * 64]
    movu            m3, [r0 + 10 * 64]
    packssdw        m2, m3                    ;[2L 10L 2H 10H]

    movu            m3, [r0 +  3 * 64]
    movu            m4, [r0 + 11 * 64]
    packssdw        m3, m4                    ;[3L 11L 3H 11H]

    movu            m4, [r0 +  4 * 64]
    movu            m5, [r0 + 12 * 64]
    packssdw        m4, m5                    ;[4L 12L 4H 12H]

    movu            m5, [r0 +  5 * 64]
    movu            m6, [r0 + 13 * 64]
    packssdw        m5, m6                    ;[5L 13L 5H 13H]

    movu            m6, [r0 +  6 * 64]
    movu            m7, [r0 + 14 * 64]
    packssdw        m6, m7                    ;[6L 14L 6H 14H]

    movu            m7, [r0 +  7 * 64]
    movu            m8, [r0 + 15 * 64]
    packssdw        m7, m8                    ;[7L 15L 7H 15H]

    punpckhwd       m8, m0, m2                ;[8 10]
    punpcklwd       m0, m2                    ;[0 2]

    punpckhwd       m2, m1, m3                ;[9 11]
    punpcklwd       m1, m3                    ;[1 3]

    punpckhwd       m3, m4, m6                ;[12 14]
    punpcklwd       m4, m6                    ;[4 6]

    punpckhwd       m6, m5, m7                ;[13 15]
    punpcklwd       m5, m7                    ;[5 7]

    punpckhdq       m7, m0, m4                ;[02 22 42 62 03 23 43 63 06 26 46 66 07 27 47 67]
    punpckldq       m0, m4                    ;[00 20 40 60 01 21 41 61 04 24 44 64 05 25 45 65]

    punpckhdq       m4, m8, m3                ;[82 102 122 142 83 103 123 143 86 106 126 146 87 107 127 147]
    punpckldq       m8, m3                    ;[80 100 120 140 81 101 121 141 84 104 124 144 85 105 125 145]

    punpckhdq       m3, m1, m5                ;[12 32 52 72 13 33 53 73 16 36 56 76 17 37 57 77]
    punpckldq       m1, m5                    ;[10 30 50 70 11 31 51 71 14 34 54 74 15 35 55 75]

    punpckhdq       m5, m2, m6                ;[92 112 132 152 93 113 133 153 96 116 136 156 97 117 137 157]
    punpckldq       m2, m6                    ;[90 110 130 150 91 111 131 151 94 114 134 154 95 115 135 155]

    punpckhqdq      m6, m0, m8                ;[01 21 41 61 81 101 121 141 05 25 45 65 85 105 125 145]
    punpcklqdq      m0, m8                    ;[00 20 40 60 80 100 120 140 04 24 44 64 84 104 124 144]

    punpckhqdq      m8, m7, m4                ;[03 23 43 63 43 103 123 143 07 27 47 67 87 107 127 147]
    punpcklqdq      m7, m4                    ;[02 22 42 62 82 102 122 142 06 26 46 66 86 106 126 146]

    punpckhqdq      m4, m1, m2                ;[11 31 51 71 91 111 131 151 15 35 55 75 95 115 135 155]
    punpcklqdq      m1, m2                    ;[10 30 50 70 90 110 130 150 14 34 54 74 94 114 134 154]

    punpckhqdq      m2, m3, m5                ;[13 33 53 73 93 113 133 153 17 37 57 77 97 117 137 157]
    punpcklqdq      m3, m5                    ;[12 32 52 72 92 112 132 152 16 36 56 76 96 116 136 156]

    IDCT_PASS1      0, 14
    IDCT_PASS1      2, 12
    IDCT_PASS1      4, 10
    IDCT_PASS1      6, 8

    add             r0, 32
    add             r3, 16
    dec             r4d
    jnz             .pass1

    mov             r3, rsp
    mov             r4d, 8
    lea             r5, [tab_idct16_2]
    lea             r6, [tab_idct16_1]

    vbroadcasti128  m7,  [r5]
    vbroadcasti128  m8,  [r5 + 16]
    vbroadcasti128  m9,  [r5 + 32]
    vbroadcasti128  m10, [r5 + 48]
    vbroadcasti128  m11, [r5 + 64]
    vbroadcasti128  m12, [r5 + 80]
    vbroadcasti128  m13, [r5 + 96]

.pass2:
    movu            m1, [r3]
    vpermq          m0, m1, 0xD8

    pmaddwd         m1, m0, m7
    pmaddwd         m2, m0, m8
    phaddd          m1, m2

    pmaddwd         m2, m0, m9
    pmaddwd         m3, m0, m10
    phaddd          m2, m3

    phaddd          m1, m2

    pmaddwd         m2, m0, m11
    pmaddwd         m3, m0, m12
    phaddd          m2, m3

    vbroadcasti128  m14, [r5 + 112]
    pmaddwd         m3, m0, m13
    pmaddwd         m4, m0, m14
    phaddd          m3, m4

    phaddd          m2, m3

    movu            m3, [r3 + 32]
    vpermq          m0, m3, 0xD8

    vbroadcasti128  m14, [r6]
    pmaddwd         m3, m0, m14
    vbroadcasti128  m14, [r6 + 16]
    pmaddwd         m4, m0, m14
    phaddd          m3, m4

    vbroadcasti128  m14, [r6 + 32]
    pmaddwd         m4, m0, m14
    vbroadcasti128  m14, [r6 + 48]
    pmaddwd         m5, m0, m14
    phaddd          m4, m5

    phaddd          m3, m4

    vbroadcasti128  m14, [r6 + 64]
    pmaddwd         m4, m0, m14
    vbroadcasti128  m14, [r6 + 80]
    pmaddwd         m5, m0, m14
    phaddd          m4, m5

    vbroadcasti128  m14, [r6 + 96]
    pmaddwd         m6, m0, m14
    vbroadcasti128  m14, [r6 + 112]
    pmaddwd         m0, m14
    phaddd          m6, m0

    phaddd          m4, m6

    paddd           m5, m1, m3
    paddd           m5, m15
    psrad           m5, IDCT_SHIFT2

    psubd           m1, m3
    paddd           m1, m15
    psrad           m1, IDCT_SHIFT2

    paddd           m6, m2, m4
    paddd           m6, m15
    psrad           m6, IDCT_SHIFT2

    psubd           m2, m4
    paddd           m2, m15
    psrad           m2, IDCT_SHIFT2

    packssdw        m5, m6
    packssdw        m1, m2
    pshufb          m2, m1, [dct16_shuf1]

    mova            [r1], xm5
    mova            [r1 + 16], xm2
    vextracti128    [r1 + r2], m5, 1
    vextracti128    [r1 + r2 + 16], m2, 1

    lea             r1, [r1 + 2 * r2]
    add             r3, 64
    dec             r4d
    jnz             .pass2
    RET
%endif
