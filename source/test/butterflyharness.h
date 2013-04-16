/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Siva Viswanathan <siva@multicorewareinc.com>
 *          Praveen Tiwari <praveen@multicorewareinc.com>
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

#ifndef _BUTTERFLYHARNESS_H_1
#define _BUTTERFLYHARNESS_H_1 1

#include "testharness.h"
#include "primitives.h"

class ButterflyHarness : public TestHarness
{
protected:

    short *bbuf1, *bbuf2, *bbuf3;

    int bb_t_size;

    bool check_butterfly16_primitive(x265::butterfly ref, x265::butterfly opt);

public:

    ButterflyHarness();

    virtual ~ButterflyHarness();

    bool testCorrectness(const x265::EncoderPrimitives& ref, const x265::EncoderPrimitives& opt);

    void measureSpeed(const x265::EncoderPrimitives& ref, const x265::EncoderPrimitives& opt);
};

#endif // ifndef _BUTTERFLYHARNESS_H_1
