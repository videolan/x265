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

/** \file     TAppDecCfg.cpp
    \brief    Decoder configuration class
*/

#include <cstdio>
#include <cstring>
#include <string>
#include "TAppDecCfg.h"
#include "TAppCommon/program_options_lite.h"

#ifdef WIN32
#define strdup _strdup
#endif

using namespace std;
namespace po = df::program_options_lite;

//! \ingroup TAppDecoder
//! \{

// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/** \param argc number of arguments
    \param argv array of arguments
 */
Bool TAppDecCfg::parseCfg( Int argc, Char* argv[] )
{
  Bool do_help = false;
  string cfg_BitstreamFile;
  string cfg_ReconFile;
  string cfg_TargetDecLayerIdSetFile;

  po::Options opts;
  opts.addOptions()
  ("help", do_help, false, "this help text")
  ("BitstreamFile,b", cfg_BitstreamFile, string(""), "bitstream input file name")
  ("ReconFile,o",     cfg_ReconFile,     string(""), "reconstructed YUV output file name\n"
                                                     "YUV writing is skipped if omitted")
  ("SkipFrames,s", m_iSkipFrame, 0, "number of frames to skip before random access")
  ("OutputBitDepth,d", m_outputBitDepthY, 0, "bit depth of YUV output luma component (default: use 0 for native depth)")
  ("OutputBitDepthC,d", m_outputBitDepthC, 0, "bit depth of YUV output chroma component (default: use 0 for native depth)")
  ("MaxTemporalLayer,t", m_iMaxTemporalLayer, -1, "Maximum Temporal Layer to be decoded. -1 to decode all layers")
  ("SEIDecodedPictureHash", m_decodedPictureHashSEIEnabled, 1, "Control handling of decoded picture hash SEI messages\n"
                                              "\t1: check hash in SEI messages if available in the bitstream\n"
                                              "\t0: ignore SEI message")
  ("SEIpictureDigest", m_decodedPictureHashSEIEnabled, 1, "deprecated alias for SEIDecodedPictureHash")
  ("TarDecLayerIdSetFile,l", cfg_TargetDecLayerIdSetFile, string(""), "targetDecLayerIdSet file name. The file should include white space separated LayerId values to be decoded. Omitting the option or a value of -1 in the file decodes all layers.")
  ("RespectDefDispWindow,w", m_respectDefDispWindow, 0, "Only output content inside the default display window\n")
  ;
  po::setDefaults(opts);
  const list<const Char*>& argv_unhandled = po::scanArgv(opts, argc, (const Char**) argv);

  for (list<const Char*>::const_iterator it = argv_unhandled.begin(); it != argv_unhandled.end(); it++)
  {
    fprintf(stderr, "Unhandled argument ignored: `%s'\n", *it);
  }

  if (argc == 1 || do_help)
  {
    po::doHelp(cout, opts);
    return false;
  }

  /* convert std::string to c string for compatability */
  m_pchBitstreamFile = cfg_BitstreamFile.empty() ? NULL : strdup(cfg_BitstreamFile.c_str());
  m_pchReconFile = cfg_ReconFile.empty() ? NULL : strdup(cfg_ReconFile.c_str());

  if (!m_pchBitstreamFile)
  {
    fprintf(stderr, "No input file specifed, aborting\n");
    return false;
  }

  if ( !cfg_TargetDecLayerIdSetFile.empty() )
  {
    FILE* targetDecLayerIdSetFile = fopen ( cfg_TargetDecLayerIdSetFile.c_str(), "r" );
    if ( targetDecLayerIdSetFile )
    {
      Bool isLayerIdZeroIncluded = false;
      while ( !feof(targetDecLayerIdSetFile) )
      {
        Int layerIdParsed = 0;
        if ( fscanf( targetDecLayerIdSetFile, "%d ", &layerIdParsed ) != 1 )
        {
          if ( m_targetDecLayerIdSet.size() == 0 )
          {
            fprintf(stderr, "No LayerId could be parsed in file %s. Decoding all LayerIds as default.\n", cfg_TargetDecLayerIdSetFile.c_str() );
          }
          break;
        }
        if ( layerIdParsed  == -1 ) // The file includes a -1, which means all LayerIds are to be decoded.
        {
          m_targetDecLayerIdSet.clear(); // Empty set means decoding all layers.
          break;
        }
        if ( layerIdParsed < 0 || layerIdParsed >= MAX_NUM_LAYER_IDS )
        {
          fprintf(stderr, "Warning! Parsed LayerId %d is not withing allowed range [0,%d]. Ignoring this value.\n", layerIdParsed, MAX_NUM_LAYER_IDS-1 );
        }
        else
        {
          isLayerIdZeroIncluded = layerIdParsed == 0 ? true : isLayerIdZeroIncluded;
          m_targetDecLayerIdSet.push_back ( layerIdParsed );
        }
      }
      fclose (targetDecLayerIdSetFile);
      if ( m_targetDecLayerIdSet.size() > 0 && !isLayerIdZeroIncluded )
      {
        fprintf(stderr, "TargetDecLayerIdSet must contain LayerId=0, aborting" );
        return false;
      }
    }
    else
    {
      fprintf(stderr, "File %s could not be opened. Using all LayerIds as default.\n", cfg_TargetDecLayerIdSetFile.c_str() );
    }
  }

  return true;
}

//! \}
