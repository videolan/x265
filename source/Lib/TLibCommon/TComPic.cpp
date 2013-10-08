/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TComPic.cpp
    \brief    picture class
*/

#include "TComPic.h"
#include "SEI.h"
#include "mv.h"

using namespace x265;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Constructor / destructor / create / destroy
// ====================================================================================================================

TComPic::TComPic()
    : m_picSym(NULL)
    , m_origPicYuv(NULL)
    , m_reconPicYuv(NULL)
    , m_bUsedByCurr(false)
    , m_bIsLongTerm(false)
    , m_bCheckLTMSB(false)
{
    m_reconRowCount = 0;
    m_countRefEncoders = 0;
    memset(&m_lowres, 0, sizeof(m_lowres));
    m_next = NULL;
    m_prev = NULL;
}

TComPic::~TComPic()
{}

void TComPic::create(int width, int height, UInt maxWidth, UInt maxHeight, UInt maxDepth, Window &conformanceWindow, Window &defaultDisplayWindow, int bframes)
{
    m_picSym = new TComPicSym;
    m_picSym->create(width, height, maxWidth, maxHeight, maxDepth);

    m_origPicYuv = new TComPicYuv;
    m_origPicYuv->create(width, height, maxWidth, maxHeight, maxDepth);

    m_reconPicYuv = new TComPicYuv;
    m_reconPicYuv->create(width, height, maxWidth, maxHeight, maxDepth);

    /* store conformance window parameters with picture */
    m_conformanceWindow = conformanceWindow;

    /* store display window parameters with picture */
    m_defaultDisplayWindow = defaultDisplayWindow;

    /* configure lowres dimensions */
    m_lowres.create(this, bframes);
}

void TComPic::destroy(int bframes)
{
    if (m_picSym)
    {
        m_picSym->destroy();
        delete m_picSym;
        m_picSym = NULL;
    }

    if (m_origPicYuv)
    {
        m_origPicYuv->destroy();
        delete m_origPicYuv;
        m_origPicYuv = NULL;
    }

    if (m_reconPicYuv)
    {
        m_reconPicYuv->destroy();
        delete m_reconPicYuv;
        m_reconPicYuv = NULL;
    }

    m_lowres.destroy(bframes);
}

//! \}
