/*****************************************************************************
 * Copyright (C) 2016 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Praveen Kumar Tiwari <praveen@multicorewareinc.com>
 *          Min Chen <chenm003@163.com> <min.chen@multicorewareinc.com>
 *          Dnyaneshwar Gorade <dnyaneshwar@multicorewareinc.com>
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "common.h"
#include "primitives.h"
#include "x265.h"
#include "cpu.h"

extern "C" {
#include "blockcopy8.h"
#include "pixel.h"
}

namespace X265_NS {
// private x265 namespace

void setupAssemblyPrimitives(EncoderPrimitives &p, int cpuMask)
{
    if (cpuMask & X265_CPU_NEON)
    {
        p.pu[LUMA_16x16].copy_pp = PFX(blockcopy_pp_16x16_neon);
        p.pu[LUMA_8x4].copy_pp = PFX(blockcopy_pp_8x4_neon);
        p.pu[LUMA_8x8].copy_pp = PFX(blockcopy_pp_8x8_neon);
        p.pu[LUMA_8x16].copy_pp = PFX(blockcopy_pp_8x16_neon);
        p.pu[LUMA_8x32].copy_pp = PFX(blockcopy_pp_8x32_neon);
        p.pu[LUMA_12x16].copy_pp = PFX(blockcopy_pp_12x16_neon); 
        p.pu[LUMA_4x4].copy_pp = PFX(blockcopy_pp_4x4_neon);
        p.pu[LUMA_4x8].copy_pp = PFX(blockcopy_pp_4x8_neon);
        p.pu[LUMA_4x16].copy_pp = PFX(blockcopy_pp_4x16_neon);
        p.pu[LUMA_16x4].copy_pp = PFX(blockcopy_pp_16x4_neon);
        p.pu[LUMA_16x8].copy_pp = PFX(blockcopy_pp_16x8_neon);
        p.pu[LUMA_16x12].copy_pp = PFX(blockcopy_pp_16x12_neon);
        p.pu[LUMA_16x32].copy_pp = PFX(blockcopy_pp_16x32_neon);        
        p.pu[LUMA_16x64].copy_pp = PFX(blockcopy_pp_16x64_neon);
        p.pu[LUMA_24x32].copy_pp = PFX(blockcopy_pp_24x32_neon);
        p.pu[LUMA_32x8].copy_pp = PFX(blockcopy_pp_32x8_neon);
        p.pu[LUMA_32x16].copy_pp = PFX(blockcopy_pp_32x16_neon);
        p.pu[LUMA_32x24].copy_pp = PFX(blockcopy_pp_32x24_neon);
        p.pu[LUMA_32x32].copy_pp = PFX(blockcopy_pp_32x32_neon);
        p.pu[LUMA_32x64].copy_pp = PFX(blockcopy_pp_32x64_neon);
        p.pu[LUMA_48x64].copy_pp = PFX(blockcopy_pp_48x64_neon);
        p.pu[LUMA_64x16].copy_pp = PFX(blockcopy_pp_64x16_neon);
        p.pu[LUMA_64x32].copy_pp = PFX(blockcopy_pp_64x32_neon);
        p.pu[LUMA_64x48].copy_pp = PFX(blockcopy_pp_64x48_neon);
        p.pu[LUMA_64x64].copy_pp = PFX(blockcopy_pp_64x64_neon);
    }
    if (cpuMask & X265_CPU_ARMV6)
    {
	 p.pu[LUMA_4x4].sad=PFX(pixel_sad_4x4_armv6);
         p.pu[LUMA_4x8].sad=PFX(pixel_sad_4x8_armv6);
    }
}
} // namespace X265_NS
