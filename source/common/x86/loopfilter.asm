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

pw_2:    times 16 db  2

SECTION .text

;============================================================================================================
; void saoCuOrgE0(pixel * rec, int8_t * offsetEo, int lcuWidth, int8_t signLeft)
;============================================================================================================
INIT_XMM sse4
cglobal saoCuOrgE0, 4, 4, 8, rec, offsetEo, lcuWidth, signLeft

    neg         r3                 ; r3 = -iSignLeft
    movd        m0,    r3d
    pslldq      m0,    15          ; m0 = [iSignLeft x .. x]
    pcmpeqb     m4,    m4          ; m4 = [pb -1]
    pxor        m5,    m5          ; m5 = 0
    movu        m6,    [r1]        ; m6 = m_iOffsetEo

.loop:
    movu        m7,    [r0]        ; m1 = pRec[x]
    mova        m1,    m7
    movu        m2,    [r0+1]      ; m2 = pRec[x+1]

    psubusb     m3,    m2, m7
    psubusb     m1,    m2
    pcmpeqb     m3,    m5
    pcmpeqb     m1,    m5
    pcmpeqb     m2,    m7

    pabsb       m3,    m3          ; m1 = (pRec[x] - pRec[x+1]) > 0) ?  1 : 0
    por         m1,    m3          ; m1 = iSignRight
    pandn       m2,    m1

    palignr     m3,    m2, m0, 15  ; m3 = -iSignLeft
    psignb      m3,    m4          ; m3 = iSignLeft
    mova        m0,    m4
    pslldq      m0,    15
    pand        m0,    m2          ; [pb 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1]
    paddb       m2,    m3
    paddb       m2,    [pw_2]      ; m1 = uiEdgeType
    pshufb      m3,    m6, m2
    pmovzxbw    m2,    m7          ; rec
    punpckhbw   m7,    m5
    pmovsxbw    m1,    m3          ; iOffsetEo
    punpckhbw   m3,    m3
    psraw       m3,    8
    paddw       m2,    m1
    paddw       m7,    m3
    packuswb    m2,    m7
    movu        [r0],  m2

    add         r0q,   16
    sub         r2d,   16
    jnz        .loop
    RET
