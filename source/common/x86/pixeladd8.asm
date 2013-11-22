;*****************************************************************************
;* Copyright (C) 2013 x265 project
;*
;* Authors: Praveen Kumar Tiwari <praveen@multicorewareinc.com>
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

SECTION_RODATA 32

SECTION .text



;-----------------------------------------------------------------------------
; void pixel_add_ps_2x4(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_add_ps_2x4, 6, 6, 2, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

pmovzxbw    m0,            [r2]
movd        m1,            [r3]

paddw       m0,            m1
packuswb    m0,            m0

pextrw      [r0],          m0,      0

pmovzxbw    m0,            [r2 + r4]
movd        m1,            [r3 + r5]

paddw       m0,            m1
packuswb    m0,            m0

pextrw      [r0 + r1],     m0,     0

pmovzxbw    m0,            [r2 + 2 * r4]
movd        m1,            [r3 + 2 * r5]

paddw       m0,            m1
packuswb    m0,            m0

pextrw      [r0 + 2 * r1], m0,     0

lea         r0,            [r0 + 2 * r1]
lea         r2,            [r2 + 2 * r4]
lea         r3,            [r3 + 2 * r5]

pmovzxbw    m0,            [r2 + r4]
movd        m1,            [r3 + r5]

paddw       m0,            m1
packuswb    m0,            m0

pextrw      [r0 + r1],     m0,     0

RET


;-----------------------------------------------------------------------------
; void pixel_add_ps_%1x%2(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
%macro PIXEL_ADD_PS_W2_H4 2
INIT_XMM sse4
cglobal pixel_add_ps_%1x%2, 6, 7, 2, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

mov         r6d,           %2/4

.loop
      pmovzxbw    m0,            [r2]
      movd        m1,            [r3]

      paddw       m0,            m1
      packuswb    m0,            m0

      pextrw      [r0],          m0,      0

      pmovzxbw    m0,            [r2 + r4]
      movd        m1,            [r3 + r5]

      paddw       m0,            m1
      packuswb    m0,            m0

      pextrw      [r0 + r1],     m0,      0

      pmovzxbw    m0,            [r2 + 2 * r4]
      movd        m1,            [r3 + 2 * r5]

      paddw       m0,            m1
      packuswb    m0,            m0

      pextrw      [r0 + 2 * r1], m0,      0

      lea         r0,            [r0 + 2 * r1]
      lea         r2,            [r2 + 2 * r4]
      lea         r3,            [r3 + 2 * r5]

      pmovzxbw    m0,            [r2 + r4]
      movd        m1,            [r3 + r5]

      paddw       m0,            m1
      packuswb    m0,            m0

      pextrw      [r0 + r1],     m0,      0

      lea         r0,            [r0 + 2 * r1]
      lea         r2,            [r2 + 2 * r4]
      lea         r3,            [r3 + 2 * r5]

      dec         r6d
      jnz         .loop

RET
%endmacro

PIXEL_ADD_PS_W2_H4   2, 8

;-----------------------------------------------------------------------------
; void pixel_add_ps_4x2(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_add_ps_4x2, 6, 6, 2, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

pmovzxbw    m0,            [r2]
movh        m1,            [r3]

paddw       m0,            m1
packuswb    m0,            m0

movd        [r0],          m0

pmovzxbw    m0,            [r2 + r4]
movh        m1,            [r3 + r5]

paddw       m0,            m1
packuswb    m0,            m0

movd        [r0 + r1],     m0

RET

;-----------------------------------------------------------------------------
; void pixel_add_ps_4x4(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_add_ps_4x4, 6, 6, 2, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

pmovzxbw    m0,            [r2]
movh        m1,            [r3]

paddw       m0,            m1
packuswb    m0,            m0

movd        [r0],          m0

pmovzxbw    m0,            [r2 + r4]
movh        m1,            [r3 + r5]

paddw       m0,            m1
packuswb    m0,            m0

movd        [r0 + r1],     m0

pmovzxbw    m0,            [r2 + 2 * r4]
movh        m1,            [r3 + 2 * r5]

paddw       m0,            m1
packuswb    m0,            m0

movd        [r0 + 2 * r1], m0

lea         r0,            [r0 + 2 * r1]
lea         r2,            [r2 + 2 * r4]
lea         r3,            [r3 + 2 * r5]

pmovzxbw    m0,            [r2 + r4]
movh        m1,            [r3 + r5]

paddw       m0,            m1
packuswb    m0,            m0

movd        [r0 + r1],     m0

RET

;-----------------------------------------------------------------------------
; void pixel_add_ps_%1x%2(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
%macro PIXEL_ADD_PS_W4_H4 2
INIT_XMM sse4
cglobal pixel_add_ps_%1x%2, 6, 7, 2, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

mov         r6d,           %2/4

.loop
      pmovzxbw    m0,            [r2]
      movh        m1,            [r3]

      paddw       m0,            m1
      packuswb    m0,            m0

      movd        [r0],          m0

      pmovzxbw    m0,            [r2 + r4]
      movh        m1,            [r3 + r5]

      paddw       m0,            m1
      packuswb    m0,            m0

      movd        [r0 + r1],     m0

      pmovzxbw    m0,            [r2 + 2 * r4]
      movh        m1,            [r3 + 2 * r5]

      paddw       m0,            m1
      packuswb    m0,            m0

      movd        [r0 + 2 * r1], m0

      lea         r0,            [r0 + 2 * r1]
      lea         r2,            [r2 + 2 * r4]
      lea         r3,            [r3 + 2 * r5]

      pmovzxbw    m0,            [r2 + r4]
      movh        m1,            [r3 + r5]

      paddw       m0,            m1
      packuswb    m0,            m0

      movd        [r0 + r1],     m0

      lea         r0,            [r0 + 2 * r1]
      lea         r2,            [r2 + 2 * r4]
      lea         r3,            [r3 + 2 * r5]

      dec         r6d
      jnz         .loop

RET
%endmacro

PIXEL_ADD_PS_W4_H4   4,  8
PIXEL_ADD_PS_W4_H4   4, 16


;-----------------------------------------------------------------------------
; void pixel_add_ps_%1x%2(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
%macro PIXEL_ADD_PS_W6_H4 2
INIT_XMM sse4
cglobal pixel_add_ps_%1x%2, 6, 7, 2, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

mov         r6d,           %2/4

.loop
      pmovzxbw    m0,                [r2]
      movu        m1,                [r3]

      paddw       m0,                m1
      packuswb    m0,                m0

      movd        [r0],              m0
      pextrw      [r0 + 4],          m0,        2

      pmovzxbw    m0,                [r2 + r4]
      movu        m1,                [r3 + r5]

      paddw       m0,                m1
      packuswb    m0,                m0

      movd        [r0 + r1],         m0
      pextrw      [r0 + r1 + 4],     m0,        2

      pmovzxbw    m0,                [r2 + 2 * r4]
      movu        m1,                [r3 + 2 * r5]

      paddw       m0,                m1
      packuswb    m0,                m0

      movd        [r0 + 2 * r1],     m0
      pextrw      [r0 + 2 * r1 + 4], m0,        2

      lea         r0,                [r0 + 2 * r1]
      lea         r2,                [r2 + 2 * r4]
      lea         r3,                [r3 + 2 * r5]

      pmovzxbw    m0,                [r2 + r4]
      movu        m1,                [r3 + r5]

      paddw       m0,                m1
      packuswb    m0,                m0

      movd        [r0 + r1],         m0
      pextrw      [r0 + r1 + 4],     m0,        2

      lea         r0,                [r0 + 2 * r1]
      lea         r2,                [r2 + 2 * r4]
      lea         r3,                [r3 + 2 * r5]

      dec         r6d
      jnz         .loop

RET
%endmacro

PIXEL_ADD_PS_W6_H4 6,  8

;-----------------------------------------------------------------------------
; void pixel_add_ps_8x2(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_add_ps_8x2, 6, 6, 2, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

pmovzxbw    m0,            [r2]
movu        m1,            [r3]

paddw       m0,            m1
packuswb    m0,            m0

movh        [r0],          m0

pmovzxbw    m0,            [r2 + r4]
movu        m1,            [r3 + r5]

paddw       m0,            m1
packuswb    m0,            m0

movh        [r0 + r1],     m0

RET

;-----------------------------------------------------------------------------
; void pixel_add_ps_8x4(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_add_ps_8x4, 6, 6, 2, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

pmovzxbw    m0,            [r2]
movu        m1,            [r3]

paddw       m0,            m1
packuswb    m0,            m0

movh        [r0],          m0

pmovzxbw    m0,            [r2 + r4]
movu        m1,            [r3 + r5]

paddw       m0,            m1
packuswb    m0,            m0

movh        [r0 + r1],     m0

pmovzxbw    m0,            [r2 + 2 * r4]
movu        m1,            [r3 + 2 * r5]

paddw       m0,            m1
packuswb    m0,            m0

movh        [r0 + 2 * r1], m0

lea         r0,            [r0 + 2 * r1]
lea         r2,            [r2 + 2 * r4]
lea         r3,            [r3 + 2 * r5]

pmovzxbw    m0,            [r2 + r4]
movu        m1,            [r3 + r5]

paddw       m0,            m1
packuswb    m0,            m0

movh        [r0 + r1],     m0

RET

;-----------------------------------------------------------------------------
; void pixel_add_ps_8x6(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_add_ps_8x6, 6, 6, 2, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

pmovzxbw    m0,            [r2]
movu        m1,            [r3]

paddw       m0,            m1
packuswb    m0,            m0

movh        [r0],          m0

pmovzxbw    m0,            [r2 + r4]
movu        m1,            [r3 + r5]

paddw       m0,            m1
packuswb    m0,            m0

movh        [r0 + r1],     m0

pmovzxbw    m0,            [r2 + 2 * r4]
movu        m1,            [r3 + 2 * r5]

paddw       m0,            m1
packuswb    m0,            m0

movh        [r0 + 2 * r1], m0

lea         r0,            [r0 + 2 * r1]
lea         r2,            [r2 + 2 * r4]
lea         r3,            [r3 + 2 * r5]

pmovzxbw    m0,            [r2 + r4]
movu        m1,            [r3 + r5]

paddw       m0,            m1
packuswb    m0,            m0

movh        [r0 + r1],     m0

pmovzxbw    m0,            [r2 + 2 * r4]
movu        m1,            [r3 + 2 * r5]

paddw       m0,            m1
packuswb    m0,            m0

movh        [r0 + 2 * r1], m0

lea         r0,            [r0 + 2 * r1]
lea         r2,            [r2 + 2 * r4]
lea         r3,            [r3 + 2 * r5]

pmovzxbw    m0,            [r2 + r4]
movu        m1,            [r3 + r5]

paddw       m0,            m1
packuswb    m0,            m0

movh        [r0 + r1],     m0

RET

;-----------------------------------------------------------------------------
; void pixel_add_ps_%1x%2(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
%macro PIXEL_ADD_PS_W8_H4 2
INIT_XMM sse4
cglobal pixel_add_ps_%1x%2, 6, 7, 2, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

mov         r6d,           %2/4

.loop
      pmovzxbw    m0,            [r2]
      movu        m1,            [r3]

      paddw       m0,            m1
      packuswb    m0,            m0

      movh        [r0],          m0

      pmovzxbw    m0,            [r2 + r4]
      movu        m1,            [r3 + r5]

      paddw       m0,            m1
      packuswb    m0,            m0

      movh        [r0 + r1],     m0

      pmovzxbw    m0,            [r2 + 2 * r4]
      movu        m1,            [r3 + 2 * r5]

      paddw       m0,            m1
      packuswb    m0,            m0

      movh        [r0 + 2 * r1], m0

      lea         r0,            [r0 + 2 * r1]
      lea         r2,            [r2 + 2 * r4]
      lea         r3,            [r3 + 2 * r5]

      pmovzxbw    m0,            [r2 + r4]
      movu        m1,            [r3 + r5]

      paddw       m0,            m1
      packuswb    m0,            m0

      movh        [r0 + r1],     m0

      lea         r0,            [r0 + 2 * r1]
      lea         r2,            [r2 + 2 * r4]
      lea         r3,            [r3 + 2 * r5]

      dec         r6d
      jnz         .loop

RET
%endmacro

PIXEL_ADD_PS_W8_H4 8,  8
PIXEL_ADD_PS_W8_H4 8, 16
PIXEL_ADD_PS_W8_H4 8, 32


;-----------------------------------------------------------------------------
; void pixel_add_ps_%1x%2(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
%macro PIXEL_ADD_PS_W12_H4 2
INIT_XMM sse4
cglobal pixel_add_ps_%1x%2, 6, 7, 4, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

mov         r6d,           %2/4

.loop
      pmovzxbw    m0,                [r2]
      pmovzxbw    m1,                [r2 + 8]

      movu        m2,                [r3]
      movh        m3,                [r3 + 16]

      paddw       m0,                m2
      paddw       m1,                m3

      packuswb    m0,                m1

      movh        [r0],              m0
      movhlps     m0,                m0
      movd        [r0 + 8],          m0

      pmovzxbw    m0,                [r2 + r4]
      pmovzxbw    m1,                [r2 + r4 + 8]

      movu        m2,                [r3 + r5]
      movh        m3,                [r3 + r5 + 16]

      paddw       m0,                m2
      paddw       m1,                m3

      packuswb    m0,                m1

      movh        [r0 + r1],         m0
      movhlps     m0,                m0
      movd        [r0 + r1 + 8],     m0

      pmovzxbw    m0,                [r2 + 2 * r4]
      pmovzxbw    m1,                [r2 + 2 * r4 + 8]

      movu        m2,                [r3 + 2 * r5]
      movh        m3,                [r3 + 2 * r5 + 16]

      paddw       m0,                m2
      paddw       m1,                m3

      packuswb    m0,                m1

      movh        [r0 + 2 * r1],     m0
      movhlps     m0,                m0
      movd        [r0 + 2 * r1 + 8], m0

      lea         r0,                [r0 + 2 * r1]
      lea         r2,                [r2 + 2 * r4]
      lea         r3,                [r3 + 2 * r5]

      pmovzxbw    m0,                [r2 + r4]
      pmovzxbw    m1,                [r2 + r4 + 8]

      movu        m2,                [r3 + r5]
      movh        m3,                [r3 + r5 + 16]

      paddw       m0,                m2
      paddw       m1,                m3

      packuswb    m0,                m1

      movh        [r0 + r1],         m0
      movhlps     m0,                m0
      movd        [r0 + r1 + 8],     m0

      lea         r0,                [r0 + 2 * r1]
      lea         r2,                [r2 + 2 * r4]
      lea         r3,                [r3 + 2 * r5]

      dec         r6d
      jnz         .loop

RET
%endmacro

PIXEL_ADD_PS_W12_H4 12, 16

;-----------------------------------------------------------------------------
; void pixel_add_ps_16x4(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
INIT_XMM sse4
cglobal pixel_add_ps_16x4, 6, 6, 4, dest, destride, src0, scr1, srcStride0, srcStride1

    add         r5,            r5

    pmovzxbw    m0,            [r2]
    pmovzxbw    m1,            [r2 + 8]

    movu        m2,            [r3]
    movu        m3,            [r3 + 16]

    paddw       m0,            m2
    paddw       m1,            m3

    packuswb    m0,            m1

    movu        [r0],          m0

    pmovzxbw    m0,            [r2 + r4]
    pmovzxbw    m1,            [r2 + r4 + 8]

    movu        m2,            [r3 + r5]
    movu        m3,            [r3 + r5 + 16]

    paddw       m0,            m2
    paddw       m1,            m3

    packuswb    m0,            m1

    movu        [r0 + r1],     m0

    pmovzxbw    m0,            [r2 + 2 * r4]
    pmovzxbw    m1,            [r2 + 2 * r4 + 8]

    movu        m2,            [r3 + 2 * r5]
    movu        m3,            [r3 + 2 * r5 + 16]

    paddw       m0,            m2
    paddw       m1,            m3

    packuswb    m0,            m1

    movu        [r0 + 2 * r1], m0

    lea         r0,            [r0 + 2 * r1]
    lea         r2,            [r2 + 2 * r4]
    lea         r3,            [r3 + 2 * r5]

    pmovzxbw    m0,            [r2 + r4]
    pmovzxbw    m1,            [r2 + r4 + 8]

    movu        m2,            [r3 + r5]
    movu        m3,            [r3 + r5 + 16]

    paddw       m0,            m2
    paddw       m1,            m3

    packuswb    m0,            m1

    movu        [r0 + r1],     m0
    RET

;-----------------------------------------------------------------------------
; void pixel_add_ps_%1x%2(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
%macro PIXEL_ADD_PS_W16_H4 2
INIT_XMM sse4
cglobal pixel_add_ps_%1x%2, 6, 7, 8, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

mov         r6d,           %2/4

.loop
      pmovzxbw    m0,            [r2]
      pmovzxbw    m1,            [r2 + 8]

      movu        m2,            [r3]
      movu        m3,            [r3 + 16]

      paddw       m0,            m2
      paddw       m1,            m3

      packuswb    m0,            m1

      movu        [r0],          m0

      pmovzxbw    m0,            [r2 + r4]
      pmovzxbw    m1,            [r2 + r4 + 8]

      movu        m2,            [r3 + r5]
      movu        m3,            [r3 + r5 + 16]

      paddw       m0,            m2
      paddw       m1,            m3

      packuswb    m0,            m1

      movu        [r0 + r1],     m0

      pmovzxbw    m0,            [r2 + 2 * r4]
      pmovzxbw    m1,            [r2 + 2 * r4 + 8]

      movu        m2,            [r3 + 2 * r5]
      movu        m3,            [r3 + 2 * r5 + 16]

      paddw       m0,            m2
      paddw       m1,            m3

      packuswb    m0,            m1

      movu        [r0 + 2 * r1], m0

      lea         r0,            [r0 + 2 * r1]
      lea         r2,            [r2 + 2 * r4]
      lea         r3,            [r3 + 2 * r5]

      pmovzxbw    m0,            [r2 + r4]
      pmovzxbw    m1,            [r2 + r4 + 8]

      movu        m2,            [r3 + r5]
      movu        m3,            [r3 + r5 + 16]

      paddw       m0,            m2
      paddw       m1,            m3

      packuswb    m0,            m1

      movu        [r0 + r1],     m0

      lea         r0,            [r0 + 2 * r1]
      lea         r2,            [r2 + 2 * r4]
      lea         r3,            [r3 + 2 * r5]

      dec         r6d
      jnz         .loop

RET
%endmacro

PIXEL_ADD_PS_W16_H4 16,  8
PIXEL_ADD_PS_W16_H4 16, 12
PIXEL_ADD_PS_W16_H4 16, 16
PIXEL_ADD_PS_W16_H4 16, 32
PIXEL_ADD_PS_W16_H4 16, 64

;-----------------------------------------------------------------------------
; void pixel_add_ps_%1x%2(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
%macro PIXEL_ADD_PS_W24_H2 2
INIT_XMM sse4
cglobal pixel_add_ps_%1x%2, 6, 7, 6, dest, destride, src0, scr1, srcStride0, srcStride1

    add         r5,            r5
    mov         r6d,           %2/2

.loop
    pmovzxbw    m0,             [r2]
    pmovzxbw    m1,             [r2 + 8]
    pmovzxbw    m2,             [r2 + 16]

    movu        m3,             [r3]
    movu        m4,             [r3 + 16]
    movu        m5,             [r3 + 32]

    paddw       m0,             m3
    paddw       m1,             m4
    paddw       m2,             m5

    packuswb    m0,             m1
    packuswb    m2,             m2

    movu        [r0],           m0
    movh        [r0 + 16],      m2

    pmovzxbw    m0,             [r2 + r4]
    pmovzxbw    m1,             [r2 + r4 + 8]
    pmovzxbw    m2,             [r2 + r4 + 16]

    movu        m3,             [r3 + r5]
    movu        m4,             [r3 + r5 + 16]
    movu        m5,             [r3 + r5 + 32]

    paddw       m0,             m3
    paddw       m1,             m4
    paddw       m2,             m5

    packuswb    m0,             m1
    packuswb    m2,             m2

    movu        [r0 + r1],      m0
    movh        [r0 + r1 + 16], m2

    lea         r0,             [r0 + 2 * r1]
    lea         r2,             [r2 + 2 * r4]
    lea         r3,             [r3 + 2 * r5]

    dec         r6d
    jnz         .loop

    RET
%endmacro

PIXEL_ADD_PS_W24_H2 24, 32

;-----------------------------------------------------------------------------
; void pixel_add_ps_%1x%2(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
%macro PIXEL_ADD_PS_W32_H2 2
INIT_XMM sse4
cglobal pixel_add_ps_%1x%2, 6, 7, 8, dest, destride, src0, scr1, srcStride0, srcStride1

    add         r5,            r5
    mov         r6d,           %2/2

.loop
    pmovzxbw    m0,             [r2]
    pmovzxbw    m1,             [r2 + 8]
    pmovzxbw    m2,             [r2 + 16]
    pmovzxbw    m3,             [r2 + 24]

    movu        m4,             [r3]
    movu        m5,             [r3 + 16]
    movu        m6,             [r3 + 32]
    movu        m7,             [r3 + 48]

    paddw       m0,             m4
    paddw       m1,             m5
    paddw       m2,             m6
    paddw       m3,             m7

    packuswb    m0,             m1
    packuswb    m2,             m3

    movu        [r0],           m0
    movu        [r0 + 16],      m2

    pmovzxbw    m0,             [r2 + r4]
    pmovzxbw    m1,             [r2 + r4 + 8]
    pmovzxbw    m2,             [r2 + r4 + 16]
    pmovzxbw    m3,             [r2 + r4 + 24]

    movu        m4,             [r3 + r5]
    movu        m5,             [r3 + r5 + 16]
    movu        m6,             [r3 + r5 + 32]
    movu        m7,             [r3 + r5 + 48]

    paddw       m0,             m4
    paddw       m1,             m5
    paddw       m2,             m6
    paddw       m3,             m7

    packuswb    m0,             m1
    packuswb    m2,             m3

    movu        [r0 + r1],      m0
    movu        [r0 + r1 + 16], m2

    lea         r0,             [r0 + 2 * r1]
    lea         r2,             [r2 + 2 * r4]
    lea         r3,             [r3 + 2 * r5]

    dec         r6d
    jnz        .loop

    RET
%endmacro

PIXEL_ADD_PS_W32_H2 32,  8
PIXEL_ADD_PS_W32_H2 32, 16
PIXEL_ADD_PS_W32_H2 32, 24
PIXEL_ADD_PS_W32_H2 32, 32
PIXEL_ADD_PS_W32_H2 32, 64

;-----------------------------------------------------------------------------
; void pixel_add_ps_%1x%2(pixel *dest, intptr_t destride, pixel *src0, int16_t *scr1, intptr_t srcStride0, intptr_t srcStride1)
;-----------------------------------------------------------------------------
%macro PIXEL_ADD_PS_W48_H2 2
INIT_XMM sse4
cglobal pixel_add_ps_%1x%2, 6, 7, 8, dest, destride, src0, scr1, srcStride0, srcStride1

add         r5,            r5

mov         r6d,           %2/2

.loop
      pmovzxbw    m0,             [r2]
      pmovzxbw    m1,             [r2 + 8]
      pmovzxbw    m2,             [r2 + 16]
      pmovzxbw    m3,             [r2 + 24]

      movu        m4,             [r3]
      movu        m5,             [r3 + 16]
      movu        m6,             [r3 + 32]
      movu        m7,             [r3 + 48]

      paddw       m0,             m4
      paddw       m1,             m5
      paddw       m2,             m6
      paddw       m3,             m7

      packuswb    m0,             m1
      packuswb    m2,             m3

      movu        [r0],           m0
      movu        [r0 + 16],      m2

      pmovzxbw    m0,             [r2 + 32]
      pmovzxbw    m1,             [r2 + 40]

      movu        m2,             [r3 + 64]
      movu        m3,             [r3 + 80]

      paddw       m0,             m2
      paddw       m1,             m3

      packuswb    m0,             m1

      movu        [r0 + 32],      m0

      pmovzxbw    m0,             [r2 + r4]
      pmovzxbw    m1,             [r2 + r4 + 8]
      pmovzxbw    m2,             [r2 + r4 + 16]
      pmovzxbw    m3,             [r2 + r4 + 24]

      movu        m4,             [r3 + r5]
      movu        m5,             [r3 + r5 + 16]
      movu        m6,             [r3 + r5 + 32]
      movu        m7,             [r3 + r5 + 48]

      paddw       m0,             m4
      paddw       m1,             m5
      paddw       m2,             m6
      paddw       m3,             m7

      packuswb    m0,             m1
      packuswb    m2,             m3

      movu        [r0 + r1],      m0
      movu        [r0 + r1 + 16], m2

      pmovzxbw    m0,             [r2 + r4 + 32]
      pmovzxbw    m1,             [r2 + r4 + 40]

      movu        m2,             [r3 + r5 + 64]
      movu        m3,             [r3 + r5 + 80]

      paddw       m0,             m2
      paddw       m1,             m3

      packuswb    m0,             m1

      movu        [r0 + r1 + 32], m0

      lea         r0,             [r0 + 2 * r1]
      lea         r2,             [r2 + 2 * r4]
      lea         r3,             [r3 + 2 * r5]

      dec         r6d
      jnz         .loop

RET
%endmacro

PIXEL_ADD_PS_W48_H2 48, 64
