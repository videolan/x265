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
 * one will define ARCH to a different value
 */

#define PASTER(name,val) name ## _ ## val
#define EVALUATOR(x,y)   PASTER(x,y)
#define NAME(fun)        EVALUATOR(fun, ARCH)

extern "C"
{

uint16_t NAME(sad_8x8)( uint8_t *piOrg, uint32_t strideOrg, uint8_t *piCur, uint32_t strideCur )
{ // NOTE: This is completely untested, and ignores alignment issues
    uint16_t uiSum = 0;

    for( uint16_t Row = 0; Row < 8; Row++ )
    {
        Vec8s m1, n1;
        m1.load(piOrg);
        n1.load(piCur);
        m1 = m1 - n1;
        m1 = abs(m1);
        uiSum += horizontal_add_x(m1);

        piOrg += strideOrg;
        piCur += strideCur;
    }

    return uiSum;
}

};
