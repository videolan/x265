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

#ifndef X265_DPB_H
#define X265_DPB_H

#include "piclist.h"

namespace x265 {
// private namespace for x265

class FrameEncoder;
class TComPic;
class TComSlice;
class TEncCfg;

class DPB
{
public:

    int                m_lastIDR;
    int                m_pocCRA;
    bool               m_bRefreshPending;
    TEncCfg*           m_cfg;
    PicList            m_picList;
    int                m_maxRefL0;
    int                m_maxRefL1;

    DPB(TEncCfg *cfg)
        : m_cfg(cfg)
    {
        m_lastIDR = 0;
        m_pocCRA = 0;
        m_bRefreshPending = false;
        m_maxRefL0 = cfg->param.maxNumReferences;
        m_maxRefL1 = 1;
    }

    ~DPB();

    void prepareEncode(TComPic*);

    void recycleUnreferenced(PicList& freeList);

protected:

    void computeRPS(int curPoc, bool isRAP, TComReferencePictureSet * rps, unsigned int maxDecPicBuffer);

    void applyReferencePictureSet(TComReferencePictureSet *rps, int curPoc);
    void decodingRefreshMarking(int pocCurr, NalUnitType nalUnitType);

    void arrangeLongtermPicturesInRPS(TComSlice *);

    NalUnitType getNalUnitType(int curPoc, int lastIdr, TComPic* pic);
};
}

#endif // X265_DPB_H
