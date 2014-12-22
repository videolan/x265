;*****************************************************************************
;* Copyright (C) 2013 x265 project
;*
;* Authors: Min Chen <chenm001@163.com>
;*          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
;*          Nabajit Deka <nabajit@multicorewareinc.com>
;*          Dnyaneshwar Gorade <dnyaneshwar@multicorewareinc.com>
;*          Murugan Vairavel <murugan@multicorewareinc.com>
;*          Yuvaraj Venkatesh <yuvaraj@multicorewareinc.com>
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

SECTION_RODATA 32


SECTION .text
cextern pb_1
cextern pb_128
cextern pb_2


;============================================================================================================
; void saoCuOrgE0(pixel * rec, int8_t * offsetEo, int lcuWidth, int8_t signLeft)
;============================================================================================================
INIT_XMM sse4
cglobal saoCuOrgE0, 4, 4, 8, rec, offsetEo, lcuWidth, signLeft

    neg         r3                          ; r3 = -signLeft
    movzx       r3d, r3b
    movd        m0, r3d
    mova        m4, [pb_128]                ; m4 = [80]
    pxor        m5, m5                      ; m5 = 0
    movu        m6, [r1]                    ; m6 = offsetEo

.loop:
    movu        m7, [r0]                    ; m1 = rec[x]
    movu        m2, [r0 + 1]                ; m2 = rec[x+1]

    pxor        m1, m7, m4
    pxor        m3, m2, m4
    pcmpgtb     m2, m1, m3
    pcmpgtb     m3, m1
    pand        m2, [pb_1]
    por         m2, m3

    pslldq      m3, m2, 1
    por         m3, m0

    psignb      m3, m4                      ; m3 = signLeft
    pxor        m0, m0
    palignr     m0, m2, 15
    paddb       m2, m3
    paddb       m2, [pb_2]                  ; m1 = uiEdgeType
    pshufb      m3, m6, m2
    pmovzxbw    m2, m7                      ; rec
    punpckhbw   m7, m5
    pmovsxbw    m1, m3                      ; offsetEo
    punpckhbw   m3, m3
    psraw       m3, 8
    paddw       m2, m1
    paddw       m7, m3
    packuswb    m2, m7
    movu        [r0], m2

    add         r0q, 16
    sub         r2d, 16
    jnz        .loop
    RET
