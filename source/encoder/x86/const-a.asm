;*****************************************************************************
;* const-a.asm: x86 global constants
;*****************************************************************************
;* Copyright (C) 2010-2013 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Jason Garrett-Glaser <darkshikari@gmail.com>
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

SECTION_RODATA

const pb_01,       times  8 db 0,1
const pb_0,        times 16 db 0
const pb_a1,       times 16 db 0xa1
const pb_1,        times 16 db 1
const pb_3,        times 16 db 3
const hsub_mul,    times  8 db 1, -1
const pb_shuf8x8c, db 0,0,0,0,2,2,2,2,4,4,4,4,6,6,6,6

const pw_1,        times 8 dw 1
const pw_2,        times 8 dw 2
const pw_m2,       times 8 dw -2
const pw_4,        times 8 dw 4
const pw_8,        times 8 dw 8
const pw_16,       times 8 dw 16
const pw_32,       times 8 dw 32
const pw_64,       times 8 dw 64
const pw_32_0,     times 4 dw 32,
                   times 4 dw 0
const pw_8000,     times 8 dw 0x8000
const pw_3fff,     times 8 dw 0x3fff
const pw_pixel_max,times 8 dw ((1 << BIT_DEPTH)-1)
const pw_ppppmmmm, dw 1,1,1,1,-1,-1,-1,-1
const pw_ppmmppmm, dw 1,1,-1,-1,1,1,-1,-1
const pw_pmpmpmpm, dw 1,-1,1,-1,1,-1,1,-1
const pw_pmmpzzzz, dw 1,-1,-1,1,0,0,0,0

const pd_1,        times 4 dd 1
const pd_32,       times 4 dd 32
const pd_1024,     times 4 dd 1024
const pd_ffff,     times 4 dd 0xffff
const pw_00ff,     times 8 dw 0x00ff
const pw_ff00,     times 8 dw 0xff00

const sw_64,       dd 64
