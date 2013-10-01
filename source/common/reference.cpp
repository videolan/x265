/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Deepthi Devaki <deepthidevaki@multicorewareinc.com>
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

#include "TLibCommon/TypeDef.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComSlice.h"
#include "primitives.h"
#include "reference.h"

using namespace x265;

void ReferencePlanes::setWeight(const wpScalingParam& w)
{
    weight = w.inputWeight;
    offset = w.inputOffset * (1 << (X265_DEPTH - 8));
    shift  = w.log2WeightDenom;
    round  = (w.log2WeightDenom >= 1) ? (1 << (w.log2WeightDenom - 1)) : (0);
    isWeighted = true;
}

bool ReferencePlanes::matchesWeight(const wpScalingParam& w)
{
    if (!isWeighted)
        return false;

    if ((weight == w.inputWeight) && (shift == (int)w.log2WeightDenom) &&
        (offset == w.inputOffset * (1 << (X265_DEPTH - 8))))
        return true;

    return false;
}

MotionReference::MotionReference()
{}

int MotionReference::init(TComPicYuv* pic, wpScalingParam *w)
{
    m_reconPic = pic;
    lumaStride = pic->getStride();
    m_startPad = pic->m_lumaMarginY * lumaStride + pic->m_lumaMarginX;
    m_next = NULL;
    isWeighted = false;

    if (w)
    {
        int width = pic->getWidth();
        int height = pic->getHeight();
        size_t padwidth = width + pic->m_lumaMarginX * 2;
        size_t padheight = height + pic->m_lumaMarginY * 2;

        setWeight(*w);
        fpelPlane = (pixel*)X265_MALLOC(pixel,  padwidth * padheight);
        if (fpelPlane) fpelPlane += m_startPad;
        else return -1;
    }
    else
    {
        /* directly reference the pre-extended integer pel plane */
        fpelPlane = pic->m_picBufY + m_startPad;
    }
    return 0;
}

MotionReference::~MotionReference()
{
    if (isWeighted && fpelPlane)
        X265_FREE(fpelPlane - m_startPad);
}
