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

/** \file     TEncPic.h
    \brief    class of picture which includes side information for encoder (header)
*/

#ifndef __TENCPIC__
#define __TENCPIC__

#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComPic.h"

//! \ingroup TLibEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// Unit block for storing image characteristics
class TEncQPAdaptationUnit
{
private:
  Double m_dActivity;

public:
  TEncQPAdaptationUnit();
  ~TEncQPAdaptationUnit();

  Void   setActivity( Double d ) { m_dActivity = d; }
  Double getActivity()           { return m_dActivity; }
};

/// Local image characteristics for CUs on a specific depth
class TEncPicQPAdaptationLayer
{
private:
  UInt                  m_uiAQPartWidth;
  UInt                  m_uiAQPartHeight;
  UInt                  m_uiNumAQPartInWidth;
  UInt                  m_uiNumAQPartInHeight;
  TEncQPAdaptationUnit* m_acTEncAQU;
  Double                m_dAvgActivity;

public:
  TEncPicQPAdaptationLayer();
  virtual ~TEncPicQPAdaptationLayer();

  Void  create( Int iWidth, Int iHeight, UInt uiAQPartWidth, UInt uiAQPartHeight );
  Void  destroy();

  UInt                   getAQPartWidth()        { return m_uiAQPartWidth;       }
  UInt                   getAQPartHeight()       { return m_uiAQPartHeight;      }
  UInt                   getNumAQPartInWidth()   { return m_uiNumAQPartInWidth;  }
  UInt                   getNumAQPartInHeight()  { return m_uiNumAQPartInHeight; }
  UInt                   getAQPartStride()       { return m_uiNumAQPartInWidth;  }
  TEncQPAdaptationUnit*  getQPAdaptationUnit()   { return m_acTEncAQU;           }
  Double                 getAvgActivity()        { return m_dAvgActivity;        }

  Void                   setAvgActivity( Double d )  { m_dAvgActivity = d; }
};

/// Picture class including local image characteristics information for QP adaptation
class TEncPic : public TComPic
{
private:
  TEncPicQPAdaptationLayer* m_acAQLayer;
  UInt                      m_uiMaxAQDepth;

public:
  TEncPic();
  virtual ~TEncPic();

  Void          create( Int iWidth, Int iHeight, UInt uiMaxWidth, UInt uiMaxHeight, UInt uiMaxDepth, UInt uiMaxAQDepth,   
                        Window &conformanceWindow, Window &defaultDisplayWindow, Int *numReorderPics, Bool bIsVirtual = false );
  virtual Void  destroy();

  TEncPicQPAdaptationLayer* getAQLayer( UInt uiDepth )  { return &m_acAQLayer[uiDepth]; }
  UInt                      getMaxAQDepth()             { return m_uiMaxAQDepth;        }
};

//! \}

#endif // __TENCPIC__
