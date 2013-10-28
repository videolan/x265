/*****************************************************************************
 * Copyright (C) 2013 x265 project
 *
 * Authors: Chung Shin Yee <shinyee@multicorewareinc.com>
 *          Min Chen <chenm003@163.com>
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

#ifndef X265_FRAMEFILTER_H
#define X265_FRAMEFILTER_H

#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComLoopFilter.h"
#include "TLibEncoder/TEncSampleAdaptiveOffset.h"

namespace x265 {
// private x265 namespace

class Encoder;

// Manages the processing of a single frame loopfilter
class FrameFilter
{
public:

    FrameFilter();

    virtual ~FrameFilter() {}

    void init(Encoder *top, int numRows, TEncSbac* rdGoOnSbacCoder);

    void destroy();

    void start(TComPic *pic);
    void end();

    void processRow(int row);
    void processRowPost(int row);
    void processSao(int row);

protected:

    TEncCfg*                    m_cfg;
    TComPic*                    m_pic;

public:

    TComLoopFilter              m_loopFilter;
    TEncSampleAdaptiveOffset    m_sao;
    int                         m_numRows;
    int                         m_saoRowDelay;

    // SAO
    TEncEntropy                 m_entropyCoder;
    TEncSbac                    m_rdGoOnSbacCoder;
    TEncBinCABAC                m_rdGoOnBinCodersCABAC;
    TComBitCounter              m_bitCounter;
    TEncSbac*                   m_rdGoOnSbacCoderRow0;  // for bitstream exact only, depends on HM's bug
    /* Temp storage for ssim computation that doesn't need repeated malloc */
    void*                       m_ssimBuf;
};
}

#endif // ifndef X265_FRAMEFILTER_H
