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

/* the "authoritative" set of encoder primitives */
EncoderPrimitives primitives;

/* cpuid == 0 - auto-detect CPU type, else
 * cpuid != 0 - force CPU type */
void SetupPrimitives( int cpuid = 0 )
{
    /* .. detect actual CPU type and pick best vector architecture
     * to use as a baseline.  Then upgrade functions with available
     * assembly code, as needed. */
    memcpy( (void*)&primitives, (void*)&primitives_vectorized_sse2, sizeof(primitives));

    cpuid = cpuid; // prevent compiler warning
}

}
