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
extern "C" {
    #include "pixel.h"
}

namespace x265 {
// private x265 namespace

// Meaningful compiler definitions
// HIGH_BIT_DEPTH
// HAVE_ALIGNED_STACK
// ARCH_X86_64

void Setup_Assembly_Primitives(EncoderPrimitives &p, int cpuid)
{
#if HIGH_BIT_DEPTH && defined(_WIN32)
    // This is not linking properly yet
    if (cpuid > 0)
        p.sa8d_16x16 = p.sa8d_16x16; // placeholder to prevent warnings
#else
    if (cpuid > 0)
    {
        p.satd[PARTITION_4x4] = x264_pixel_satd_4x4_mmx2;
        p.satd[PARTITION_4x8] = x264_pixel_satd_4x8_mmx2;
        p.satd[PARTITION_8x4] = x264_pixel_satd_8x4_mmx2;
        p.satd[PARTITION_8x8] = x264_pixel_satd_8x8_mmx2;
    }
#endif
}

}
