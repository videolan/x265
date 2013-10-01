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

#ifndef X265_REFERENCE_H
#define X265_REFERENCE_H

#include "primitives.h"

namespace x265 {
// private x265 namespace

class TComPicYuv;
class TComPic;
struct WpScalingParam;
typedef WpScalingParam wpScalingParam;

struct ReferencePlanes
{
    ReferencePlanes() : isWeighted(false), isLowres(false) {}

    void setWeight(const wpScalingParam&);
    bool matchesWeight(const wpScalingParam&);

    pixel* fpelPlane;
    pixel* lowresPlane[4];

    bool isWeighted;
    bool isLowres;
    int  lumaStride;
    int  weight;
    int  offset;
    int  shift;
    int  round;
};

class MotionReference : public ReferencePlanes
{
public:

    MotionReference();
    ~MotionReference();
    int  init(TComPicYuv*, wpScalingParam* w = NULL);

    MotionReference *m_next;
    TComPicYuv      *m_reconPic;

protected:

    intptr_t         m_startPad;

    MotionReference& operator =(const MotionReference&);
};
}

#endif // ifndef X265_REFERENCE_H
