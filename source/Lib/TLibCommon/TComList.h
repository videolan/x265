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

/** \file     TComList.h
    \brief    general list class (header)
*/

#ifndef _TCOMLIST_
#define _TCOMLIST_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <list>
#include <assert.h>
#include "CommonDef.h"

#include <cstdlib>
using namespace std;

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// list template
template< class C >
class TComList : public std::list< C >
{
public:
  typedef typename std::list<C>::iterator TComIterator;
  
  TComList& operator += ( const TComList& rcTComList)
  {
    if( ! rcTComList.empty() )
    {
      insert( this->end(), rcTComList.begin(), rcTComList.end());
    }
    return *this;
  } // leszek
  
  C popBack()
  {
    C cT = this->back();
    this->pop_back();
    return cT;
  }
  
  C popFront()
  {
    C cT = this->front();
    this->pop_front();
    return cT;
  }
  
  Void pushBack( const C& rcT )
  {
    /*assert( sizeof(C) == 4);*/
    if( rcT != NULL )
    {
      this->push_back( rcT);
    }
  }
  
  Void pushFront( const C& rcT )
  {
    /*assert( sizeof(C) == 4);*/
    if( rcT != NULL )
    {
      this->push_front( rcT);
    }
  }
  
  TComIterator find( const C& rcT ) // leszek
  {
    return find( this->begin(), this->end(), rcT );
  }
};

//! \}

#endif
