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

#include "input/input.h"
#include "output/output.h"
#include "threadpool.h"
#include "common.h"
#include "x265.h"

#include <list>
#include <ostream>

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

    Encoder() : m_profile(Profile::Name::MAIN), m_levelTier(Level::Tier::MAIN), m_level(Level::Name::NONE) {};

    virtual ~Encoder() {}

    void configure(x265_param_t *param);
};
}
#endif
