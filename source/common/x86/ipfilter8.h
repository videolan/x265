/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#ifndef X265_IPFILTER8_H
#define X265_IPFILTER8_H

#define SETUP_CHROMA_FUNC_DEF(W, H) \
    void x265_interp_4tap_horiz_pp_ ## W ## x ## H ## _sse4(pixel * src, intptr_t srcStride, pixel * dst, intptr_t dstStride, int coeffIdx);
SETUP_CHROMA_FUNC_DEF(2, 4);
SETUP_CHROMA_FUNC_DEF(2, 8);
SETUP_CHROMA_FUNC_DEF(4, 2);
SETUP_CHROMA_FUNC_DEF(4, 4);
SETUP_CHROMA_FUNC_DEF(4, 8);
SETUP_CHROMA_FUNC_DEF(4, 16);
SETUP_CHROMA_FUNC_DEF(6, 8);
SETUP_CHROMA_FUNC_DEF(8, 2);
SETUP_CHROMA_FUNC_DEF(8, 4);
SETUP_CHROMA_FUNC_DEF(8, 6);
SETUP_CHROMA_FUNC_DEF(8, 8);
SETUP_CHROMA_FUNC_DEF(8, 16);
SETUP_CHROMA_FUNC_DEF(8, 32);
SETUP_CHROMA_FUNC_DEF(12, 16);
SETUP_CHROMA_FUNC_DEF(16, 4);
SETUP_CHROMA_FUNC_DEF(16, 8);
SETUP_CHROMA_FUNC_DEF(16, 12);
SETUP_CHROMA_FUNC_DEF(16, 16);
SETUP_CHROMA_FUNC_DEF(16, 32);
SETUP_CHROMA_FUNC_DEF(32, 8);
SETUP_CHROMA_FUNC_DEF(32, 16);
SETUP_CHROMA_FUNC_DEF(32, 24);
SETUP_CHROMA_FUNC_DEF(32, 32);

#endif // ifndef X265_MC_H
