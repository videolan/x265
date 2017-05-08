;*****************************************************************************
;* Copyright (C) 2013-2017 MulticoreWare, Inc
;*
;* Authors: Jayashri Murugan <jayashri@multicorewareinc.com>
;*          Vignesh V Menon <vignesh@multicorewareinc.com>
;*          Praveen Tiwari <praveen@multicorewareinc.com>
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

SECTION .text 

;-----------------------------------------------------------------------------
;void integral_init4v_c(uint32_t *sum4, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal integral4v, 2, 3, 2
    mov r2, r1
    shl r2, 4

.loop
    movu    m0, [r0]
    movu    m1, [r0 + r2]
    psubd   m1, m0
    movu    [r0], m1
    add     r0, 32
    sub     r1, 8
    cmp     r1, 0
    jnz     .loop
    RET

;-----------------------------------------------------------------------------
;void integral_init8v_c(uint32_t *sum8, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal integral8v, 2, 3, 2
    mov r2, r1
    shl r2, 5

.loop
    movu    m0, [r0]
    movu    m1, [r0 + r2]
    psubd   m1, m0
    movu    [r0], m1
    add     r0, 32
    sub     r1, 8
    cmp     r1, 0
    jnz     .loop
    RET

;-----------------------------------------------------------------------------
;void integral_init12v_c(uint32_t *sum12, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal integral12v, 2, 2, 0
 
    RET

;-----------------------------------------------------------------------------
;void integral_init16v_c(uint32_t *sum16, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal integral16v, 2, 2, 0
 
    RET

;-----------------------------------------------------------------------------
;void integral_init24v_c(uint32_t *sum24, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal integral24v, 2, 2, 0
 
    RET

;-----------------------------------------------------------------------------
;void integral_init32v_c(uint32_t *sum32, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal integral32v, 2, 2, 0
 
    RET

;-----------------------------------------------------------------------------
;static void integral_init4h_c(uint32_t *sum, pixel *pix, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal integral4h, 3, 3, 0
 
    RET

;-----------------------------------------------------------------------------
;static void integral_init8h_c(uint32_t *sum, pixel *pix, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal integral8h, 3, 3, 0
 
    RET

;-----------------------------------------------------------------------------
;static void integral_init12h_c(uint32_t *sum, pixel *pix, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal integral12h, 3, 3, 0
 
    RET

;-----------------------------------------------------------------------------
;static void integral_init16h_c(uint32_t *sum, pixel *pix, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal integral16h, 3, 3, 0
 
    RET

;-----------------------------------------------------------------------------
;static void integral_init24h_c(uint32_t *sum, pixel *pix, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal integral24h, 3, 3, 0
 
    RET

;-----------------------------------------------------------------------------
;static void integral_init32h_c(uint32_t *sum, pixel *pix, intptr_t stride)
;-----------------------------------------------------------------------------
INIT_YMM avx2
cglobal integral32h, 3, 3, 0
 
    RET
