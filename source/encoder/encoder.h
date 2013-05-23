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

#ifndef __ENCODER__
#define __ENCODER__

#include "TLibEncoder/TEncTop.h"
#include "common.h"
#include "x265.h"

namespace x265 {
// private namespace

class Encoder : public TEncTop
{
protected:
    // profile/level
    Profile::Name m_profile;
    Level::Tier   m_levelTier;
    Level::Name   m_level;

    // coding structure
    GOPEntry  m_GOPList[MAX_GOP];               ///< the coding structure entries from the config file
    int       m_maxTempLayer;                   ///< Max temporal layer
    int       m_numReorderPics[MAX_TLAYER];     ///< total number of reorder pictures
    int       m_maxDecPicBuffering[MAX_TLAYER]; ///< total number of pictures in the decoded picture buffer

public:
    int       m_iGOPSize;                       ///< GOP size of hierarchical structure
    TComList<TComPicYuv *> m_cListPicYuvRec;    ///< list of reconstructed YUV files
    TComList<TComPicYuv *> m_cListRecQueue;
    x265_param_t *m_param;

    Encoder() : m_profile(Profile::MAIN), m_levelTier(Level::MAIN), m_level(Level::NONE) {};

    virtual ~Encoder() {}

    void configure(x265_param_t *param);
};
}
#endif
