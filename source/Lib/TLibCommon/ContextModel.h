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

/** \file     ContextModel.h
    \brief    context model class (header)
*/

#ifndef X265_CONTEXTMODEL_H
#define X265_CONTEXTMODEL_H

#include "CommonDef.h"

//! \ingroup TLibCommon
//! \{

namespace x265 {
// private namespace


// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// context model class
class ContextModel
{
public:

    ContextModel()   { m_state = 0; bBinsCoded = 0; }

    ~ContextModel()  {}

    UChar getState() { return m_state >> 1; } ///< get current state

    UChar getMps()   { return m_state  & 1; } ///< get curret MPS

    void init(int qp, int initValue);   ///< initialize state with initial probability

    void updateLPS()
    {
        m_state = s_nextStateLPS[m_state];
    }

    void updateMPS()
    {
        m_state = s_nextStateMPS[m_state];
    }

    int getEntropyBits(UInt val) { return s_entropyBits[m_state ^ val]; }

    void update(int binVal)
    {
        m_state = m_nextState[m_state][binVal];
    }

    static void buildNextStateTable();
    static int getEntropyBitsTrm(int val) { return s_entropyBits[126 ^ val]; }

    void setBinsCoded(UInt val)   { bBinsCoded = (UChar)val;  }

    UInt getBinsCoded()           { return bBinsCoded;   }

private:

    UChar         m_state;  ///< internal state variable
    UChar         bBinsCoded;
    static const UChar s_nextStateMPS[128];
    static const UChar s_nextStateLPS[128];
    static const int   s_entropyBits[128];
    static UChar  m_nextState[128][2];
};
}
//! \}

#endif // ifndef X265_CONTEXTMODEL_H
