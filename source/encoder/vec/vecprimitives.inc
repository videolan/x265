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

/* This header is included into the per-architecture CPP files.  Each
 * one will define ARCH to a different value. */

#include "primitives.h"

using namespace x265;

namespace {
// each of these headers implements a portion of the performance
// primitives and declares a Setup_Vec_FOOPrimitves() method.
#include "pixel.inc"
#include "macroblock.inc"
}

#define PASTER(name, val) name ## _ ## val
#define EVALUATOR(x, y)   PASTER(x, y)
#define NAME(func)        EVALUATOR(func, ARCH)

namespace x265 {
// private x265 namespace

/* initialize function table with functions compiled for this vector
 * architecture.  This is the only symbol exported from each file. */
void NAME(Setup_Vec_Primitives) (EncoderPrimitives &p)
{
    Setup_Vec_PixelPrimitives(p);
    Setup_Vec_MacroblockPrimitives(p);
}
}
