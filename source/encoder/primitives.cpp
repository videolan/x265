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

#include <string.h>
#include "primitives.h"

namespace x265
{

/* C (reference) versions of each primitive, implemented by various
 * C++ files (pixels.cpp, etc) */
EncoderPrimitives primitives_c;

/* These function tables are defined by C++ files in encoder/vec
 * Depending on your compiler, some of them may be undefined.
 * The #if logic here must match the file lists in vec/CMakeLists.txt */

#if defined (__GNUC__) || defined(_MSC_VER)
extern EncoderPrimitives primitives_vectorized_sse42;
extern EncoderPrimitives primitives_vectorized_sse41;
extern EncoderPrimitives primitives_vectorized_ssse3;
extern EncoderPrimitives primitives_vectorized_sse3;
extern EncoderPrimitives primitives_vectorized_sse2;
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1600
extern EncoderPrimitives primitives_vectorized_avx;
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1700
extern EncoderPrimitives primitives_vectorized_avx2;
#endif

/* the "authoritative" set of encoder primitives */
#if ENABLE_PRIMITIVES
EncoderPrimitives primitives;
#endif

/* Take all primitive functions from p which are non-NULL */
static void MergeFunctions(const EncoderPrimitives& p)
{
    /* too bad this isn't an introspecive language, but we can use macros */

#define TAKE_IF_NOT_NULL(FOO) \
    primitives.FOO = p.FOO ? p.FOO : primitives.FOO
#define TAKE_EACH_IF_NOT_NULL(FOO, COUNT) \
    for (int i = 0; i < COUNT; i++) \
        primitives.FOO[i] = p.FOO[i] ? p.FOO[i] : primitives.FOO[i]

    TAKE_EACH_IF_NOT_NULL(sad, NUM_PARTITIONS);
    TAKE_EACH_IF_NOT_NULL(satd, NUM_PARTITIONS);
}

/* cpuid == 0 - auto-detect CPU type, else
 * cpuid != 0 - force CPU type */
void SetupPrimitives(int cpuid)
{
#if ENABLE_PRIMITIVES
    memcpy((void *)&primitives, (void *)&primitives_c, sizeof(primitives));

    /* .. detect actual CPU type and pick best vector architecture
     * to use as a baseline.  Then upgrade functions with available
     * assembly code, as needed. */
    MergeFunctions(primitives_vectorized_sse2);
#endif

    cpuid = cpuid; // prevent compiler warning
}

}
