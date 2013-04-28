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

/** \file     TAppEncCfg.h
    \brief    Handle encoder configuration parameters (header)
*/

#ifndef __TAPPENCCFG__
#define __TAPPENCCFG__

#include "TLibCommon/CommonDef.h"
#include "input/input.h"
#include "output/output.h"
#include "x265.h"
#include <sstream>

//! \ingroup TAppEncoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// encoder configuration class
class TAppEncCfg : public x265_params
{
protected:
    x265::Input*  m_input;
    x265::Output* m_recon;

    int       m_inputBitDepth;                  ///< bit-depth of input file
    int       m_outputBitDepth;                 ///< bit-depth of output file
    uint32_t  m_FrameSkip;                      ///< number of skipped frames from the beginning
    int       m_framesToBeEncoded;              ///< number of encoded frames

    // file I/O
    Char*     m_pchBitstreamFile;               ///< output bitstream file
    Double    m_adLambdaModifier[MAX_TLAYER];   ///< Lambda modifier array for each temporal layer

    // profile/level
    Profile::Name m_profile;
    Level::Tier   m_levelTier;
    Level::Name   m_level;

    // coding structure
    GOPEntry  m_GOPList[MAX_GOP];               ///< the coding structure entries from the config file
    Int       m_numReorderPics[MAX_TLAYER];     ///< total number of reorder pictures
    Int       m_maxDecPicBuffering[MAX_TLAYER]; ///< total number of pictures in the decoded picture buffer
    // coding quality
    Double    m_fQP;                            ///< QP value of key-picture (floating point)
    Char*     m_pchdQPFile;                     ///< QP offset for each slice (initialized from external file)
    Int*      m_aidQP;                          ///< array of slice QP values

    Char*     m_pchColumnWidth;
    Char*     m_pchRowHeight;
    UInt*     m_pColumnWidth;
    UInt*     m_pRowHeight;

    Int*      m_startOfCodedInterval;
    Int*      m_codedPivotValue;
    Int*      m_targetPivotValue;

    Int       m_useScalingListId;
    Char*     m_scalingListFile;                ///< quantization matrix file name

    // internal member functions
    Void      xSetGlobal();                     ///< set global variables
    Void      xCheckParameter();                ///< check validity of configuration values
    Void      xPrintParameter();                ///< print configuration values
    Void      xPrintUsage();                    ///< print usage

public:

    TAppEncCfg();
    virtual ~TAppEncCfg();

public:

    Void  create();                             ///< create option handling class
    Void  destroy();                            ///< destroy option handling class
    Bool  parseCfg(Int argc, Char* argv[]);     ///< parse configuration file to fill member variables
}; // END CLASS DEFINITION TAppEncCfg

//! \}

#endif // __TAPPENCCFG__
