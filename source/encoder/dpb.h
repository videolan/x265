/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_DPB_H
#define X265_DPB_H

#include "piclist.h"

namespace X265_NS {
// private namespace for x265

class Frame;
class FrameData;
class Slice;

class DPB
{
public:

    int                m_lastIDR;
    int                m_pocCRA;
    int                m_bOpenGOP;
    int                m_bhasLeadingPicture;
    bool               m_bRefreshPending;
    bool               m_bTemporalSublayer;
    PicList            m_picList;
    PicList            m_freeList;
    FrameData*         m_frameDataFreeList;

    DPB(x265_param *param)
    {
        m_lastIDR = 0;
        m_pocCRA = 0;
        m_bhasLeadingPicture = param->radl;
        if (param->bResetZoneConfig)
        {
            for (int i = 0; i < param->rc.zonefileCount ; i++)
            {
                if (param->rc.zones[i].zoneParam->radl)
                {
                    m_bhasLeadingPicture = param->rc.zones[i].zoneParam->radl;
                    break;
                }
            }
        }
        m_bRefreshPending = false;
        m_frameDataFreeList = NULL;
        m_bOpenGOP = param->bOpenGOP;
        m_bTemporalSublayer = (param->bEnableTemporalSubLayers > 2);
    }

    ~DPB();

    void prepareEncode(Frame*);

    void recycleUnreferenced();

protected:

    void computeRPS(int curPoc,int tempId, bool isRAP, RPS * rps, unsigned int maxDecPicBuffer);

    void applyReferencePictureSet(RPS *rps, int curPoc, int tempId, bool isTSAPicture);
    bool getTemporalLayerNonReferenceFlag();
    void decodingRefreshMarking(int pocCurr, NalUnitType nalUnitType);
    bool isTemporalLayerSwitchingPoint(int curPoc, int tempId);
    bool isStepwiseTemporalLayerSwitchingPoint(RPS *rps, int curPoc, int tempId);

    NalUnitType getNalUnitType(int curPoc, bool bIsKeyFrame);
};
}

#endif // X265_DPB_H
