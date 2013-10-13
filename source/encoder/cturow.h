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

#ifndef X265_CTUROW_H
#define X265_CTUROW_H

#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComPic.h"

#include "TLibEncoder/TEncCu.h"
#include "TLibEncoder/TEncSearch.h"
#include "TLibEncoder/TEncSbac.h"
#include "TLibEncoder/TEncBinCoderCABAC.h"

namespace x265 {
// private x265 namespace

class Encoder;

/* manages the state of encoding one row of CTU blocks.  When
 * WPP is active, several rows will be simultaneously encoded.
 * When WPP is inactive, only one CTURow instance is used. */
class CTURow
{
public:
    CTURow() : m_rdGoOnBinCodersCABAC(true) {}

    TEncSbac               m_sbacCoder;
    TEncSbac               m_rdGoOnSbacCoder;
    TEncSbac               m_bufferSbacCoder;
    TEncBinCABAC           m_binCoderCABAC;
    TEncBinCABAC           m_rdGoOnBinCodersCABAC;
    TComBitCounter         m_bitCounter;
    TComRdCost             m_rdCost;
    TEncEntropy            m_entropyCoder;
    TEncSearch             m_search;
    TEncCu                 m_cuCoder;
    TComTrQuant            m_trQuant;
    TEncSbac            ***m_rdSbacCoders;
    TEncBinCABAC        ***m_binCodersCABAC;

    void create(Encoder* top);

    void destroy();

    void init(TComSlice *slice)
    {
        m_active = 0;

        // Note: Reset status to avoid frame parallelism output mistake on different thread number
        for (UInt depth = 0; depth < g_maxCUDepth + 1; depth++)
        {
            for (int ciIdx = 0; ciIdx < CI_NUM; ciIdx++)
            {
                m_rdSbacCoders[depth][ciIdx]->setSlice(slice);
                m_rdSbacCoders[depth][ciIdx]->resetEntropy();
                m_binCodersCABAC[depth][ciIdx]->m_fracBits = 0;
            }
        }
        m_rdGoOnSbacCoder.setSlice(slice);
        m_rdGoOnSbacCoder.resetEntropy();
    }

    void processCU(TComDataCU *cu, TComSlice *slice, TEncSbac *bufferSBac, bool bSaveCabac);

    /* Threading */
    Lock                m_lock;
    volatile bool       m_active;
    volatile uint32_t   m_completed;
};

}

#endif // ifndef X265_CTUROW_H
