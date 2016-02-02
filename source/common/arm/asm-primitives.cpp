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
}

namespace X265_NS {
// private x265 namespace

void setupAssemblyPrimitives(EncoderPrimitives &p, int cpuMask)
{
    if (cpuMask & X265_CPU_NEON)
    {
        p.pu[LUMA_16x16].copy_pp = PFX(blockcopy_pp_16x16_neon);
    }
}
} // namespace X265_NS
