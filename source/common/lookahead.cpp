/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Gopu Govindaswamy <gopu@multicorewareinc.com>
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

#include "TLibCommon/TComPic.h"
#include "lookahead.h"
#include "mv.h"

using namespace x265;

void LookaheadFrame::create(TComPic *pic, int _bframes)
{
    TComPicYuv *orig = pic->getPicYuvOrg();
    TComPicSym *sym = pic->getPicSym();

    width = orig->getWidth() / 2;
    lines = orig->getHeight() / 2;
    stride = width + 2 * orig->getLumaMarginX();
    bframes = _bframes;

    /* allocate lowres buffers */
    for (int i = 0; i < 4; i++)
    {
        buffer[i] = (Pel*)X265_MALLOC(Pel, stride * (lines + 2 * orig->getLumaMarginY()));
    }

    int padoffset = stride * orig->getLumaMarginY() + orig->getLumaMarginX();
    m_lumaPlane[0][0] = buffer[0] + padoffset;
    m_lumaPlane[2][0] = buffer[1] + padoffset;
    m_lumaPlane[0][2] = buffer[2] + padoffset;
    m_lumaPlane[2][2] = buffer[3] + padoffset;

    /* for now, use HPEL planes for QPEL offsets */
    m_lumaPlane[0][1] = m_lumaPlane[1][0] = m_lumaPlane[1][1] = m_lumaPlane[0][0];
    m_lumaPlane[2][1] = m_lumaPlane[3][0] = m_lumaPlane[3][1] = m_lumaPlane[2][0];
    m_lumaPlane[0][3] = m_lumaPlane[1][2] = m_lumaPlane[1][3] = m_lumaPlane[0][2];
    m_lumaPlane[2][3] = m_lumaPlane[3][2] = m_lumaPlane[3][3] = m_lumaPlane[2][2];

    for (int i = 0; i < bframes + 2; i++)
    {
        for (int j = 0; j < bframes + 2; j++)
        {   
            rowSatds[i][j] = (int*)X265_MALLOC(int, sym->getFrameHeightInCU());
            lowresCosts[i][j] = (uint16_t*)X265_MALLOC(uint16_t, sym->getNumberOfCUsInFrame());
        }
    }

    for (int i = 0; i < bframes + 1; i++)
    {
        lowresMvs[0][i] = (MV*)X265_MALLOC(MV, sym->getNumberOfCUsInFrame());
        lowresMvs[1][i] = (MV*)X265_MALLOC(MV, sym->getNumberOfCUsInFrame());
        lowresMvCosts[0][i] = (int*)X265_MALLOC(int, sym->getNumberOfCUsInFrame());
        lowresMvCosts[1][i] = (int*)X265_MALLOC(int, sym->getNumberOfCUsInFrame());
    }
}
