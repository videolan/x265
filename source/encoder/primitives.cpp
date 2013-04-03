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
#include <assert.h>
#include <stdint.h>

namespace x265
{

static int8_t psize[8][8] =
{   // 4, 8, 12, 16, 20, 24, 28, 32
    { PARTITION_4x4, PARTITION_4x8, -1, PARTITION_4x16, -1, -1, -1, PARTITION_4x32 },
    { PARTITION_8x4, PARTITION_8x8, -1, PARTITION_8x16, -1, -1, -1, PARTITION_8x32 },
    { -1, -1, -1, -1, -1, -1, -1, -1 },
    { PARTITION_16x4, PARTITION_16x8, -1, PARTITION_16x16, -1, -1, -1, PARTITION_16x32 },
    { -1, -1, -1, -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1, -1, -1, -1 },
    { -1, -1, -1, -1, -1, -1, -1, -1 },
    { PARTITION_32x4, PARTITION_32x8, -1, PARTITION_32x16, -1, -1, -1, PARTITION_32x32 },
};

// Returns a Partitions enum if the size matches a supported performance primitive,
// else returns -1 (in which case you should use the slow path)
int PartitionFromSizes(int Width, int Height)
{
    // If either of these are possible, we must add if() checks for them
    assert( ((Width | Height) & 3) == 0);
    assert( Width <= 64 && Height <= 64);
    if ((Width >=32) || (Height >= 32))
        return -1;
    return (int) psize[(Width>>2)-1][(Height>>2)-1];
}

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

/* cpuid == 0 - auto-detect CPU type, else
 * cpuid != 0 - force CPU type */
void SetupPrimitives(int cpuid)
{
#if ENABLE_PRIMITIVES
    Setup_C_Primitives(&primitives_c);
	
    memcpy((void *)&primitives, (void *)&primitives_c, sizeof(primitives));

    /* Depending on the cpuid, call the appropriate setup_vec_primitive_arch */
    
    //NAME(Setup_Vec_Primitives)(&primitives);
#endif
    cpuid = cpuid; // prevent compiler warning
}

}
