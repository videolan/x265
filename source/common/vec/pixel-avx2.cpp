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
 * For more information, contact us at licensing@multicorewareinc.com
 *****************************************************************************/

/* this file instantiates AVX2 versions of the vectorized primitives */

#include "primitives.h"

#define INSTRSET 8
#include "vectorclass.h"

#define ARCH avx2
using namespace x265;

namespace {
// each of these headers implements a portion of the performance
// primitives and declares a Setup_Vec_FOOPrimitves() method.
#if HIGH_BIT_DEPTH
    #include "pixel16.inc"
#else
    #include "pixel8.inc"
#endif
}

#include "pixel.inc"
