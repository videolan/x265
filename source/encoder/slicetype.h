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

#ifndef _SLICETYPE_H_
#define _SLICETYPE_H_ 1

#include "TLibCommon/TComList.h"
#include "motion.h"

class TComPic;
class TEncCfg;

// arbitrary, but low because SATD scores are 1/4 normal
#define X265_LOOKAHEAD_QP (12 + QP_BD_OFFSET)

namespace x265 {
// private namespace

struct LookaheadFrame;

struct Lookahead
{
    TEncCfg         *cfg;
    MotionEstimate   me;
    LookaheadFrame **frames;
    int              bframes;
    int              frameQueueSize;
    int              bAdaptMode;
    int              numDecided;

    TComList<TComPic*> inputQueue;       // input pictures in order received
    TComList<TComPic*> outputQueue;      // pictures to be encoded, in encode order

    Lookahead(TEncCfg *);
    ~Lookahead();

    void addPicture(TComPic*);
    void flush();
    void slicetypeDecide();

    int estimateFrameCost(int p0, int p1, int b, int bIntraPenalty);
    int estimateCUCost(int cux, int cuy, int p0, int p1, int b, int do_search[2]);
};

}

#endif
