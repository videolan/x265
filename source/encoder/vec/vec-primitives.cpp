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

#include "primitives.h"

namespace x265 {
// private x265 namespace

/* These functions are defined by C++ files in this folder. Depending on your
 * compiler, some of them may be undefined.  The #if logic here must match the
 * file lists in CMakeLists.txt */
#if defined(__GNUC__) || defined(_MSC_VER)
extern void Setup_Vec_Primitives_sse42(EncoderPrimitives&);
extern void Setup_Vec_Primitives_sse41(EncoderPrimitives&);
extern void Setup_Vec_Primitives_ssse3(EncoderPrimitives&);
extern void Setup_Vec_Primitives_sse3(EncoderPrimitives&);
extern void Setup_Vec_Primitives_sse2(EncoderPrimitives&);
#endif
#if (defined(_MSC_VER) && _MSC_VER >= 1600) || defined(__GNUC__)
extern void Setup_Vec_Primitives_avx(EncoderPrimitives&);
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1700
extern void Setup_Vec_Primitives_avx2(EncoderPrimitives&);
#endif

/* Use primitives for the best available vector architecture */
void Setup_Vector_Primitives(EncoderPrimitives &p, int cpuid)
{
#if defined(__GNUC__) || defined(_MSC_VER)
    if (cpuid > 1) Setup_Vec_Primitives_sse2(p);
    if (cpuid > 2) Setup_Vec_Primitives_sse3(p);
    if (cpuid > 3) Setup_Vec_Primitives_ssse3(p);
    if (cpuid > 4) Setup_Vec_Primitives_sse41(p);
    if (cpuid > 5) Setup_Vec_Primitives_sse42(p);
#endif
#if (defined(_MSC_VER) && _MSC_VER >= 1600) || defined(__GNUC__)
    if (cpuid > 6) Setup_Vec_Primitives_avx(p);
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1700
    if (cpuid > 7) Setup_Vec_Primitives_avx2(p);
#endif
}

}
