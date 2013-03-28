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

#ifndef X265_PRIMITIVES_H
#define X265_PRIMITIVES_H

#include <stdint.h>

namespace x265
{

enum Partitions {
    PARTITION_4x4,
    PARTITION_8x4,
    PARTITION_4x8,
    PARTITION_8x8,
    PARTITION_8x16,
    PARTITION_16x8,
    PARTITION_16x16,
    PARTITION_16x32,
    PARTITION_32x16,
    PARTITION_32x32,
    NUM_PARTITIONS
};

extern "C"
typedef int (*pixelcmp)( uint8_t *fenc, intptr_t fencstride, uint8_t *fref, intptr_t frefstride );

/* Define a structure containing function pointers to optimized encoder
 * primitives.  Each pointer can reference either an assembly routine,
 * a vectorized primitive, or a C function. */
struct EncoderPrimitives
{
    /* All pixel comparison functions take the same arguments */
    pixelcmp sad[NUM_PARTITIONS];   // Sum of Differences for each size
    pixelcmp satd[NUM_PARTITIONS];  // Sum of Transformed differences (HADAMARD)

    /* .. Define primitives for more things .. */
};

/* This copy of the table is what gets used by all by the encoder.
 * It must be initialized before the encoder begins. */
extern EncoderPrimitives primitives;

void SetupPrimitives( int cpuid = 0 );

}

#endif
