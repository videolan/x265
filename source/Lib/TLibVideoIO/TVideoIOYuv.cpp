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

/** \file     TVideoIOYuv.cpp
    \brief    YUV file I/O class
*/

#include <cstdlib>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>

#include "TLibCommon/TComRom.h"
#include "TVideoIOYuv.h"

using namespace std;

/**
 * Perform division with rounding of all pixels in img by
 * 2<sup>shiftbits</sup>. All pixels are clipped to [minval, maxval]
 *
 * @param img        pointer to image to be transformed
 * @param stride     distance between vertically adjacent pixels of img.
 * @param width      width of active area in img.
 * @param height     height of active area in img.
 * @param shiftbits  number of rounding bits
 * @param minval     minimum clipping value
 * @param maxval     maximum clipping value
 */
static void invScalePlane(Pel* img, UInt stride, UInt width, UInt height,
                       UInt shiftbits, Pel minval, Pel maxval)
{
  Pel offset = 1 << (shiftbits-1);
  for (UInt y = 0; y < height; y++)
  {
    for (UInt x = 0; x < width; x++)
    {
      Pel val = (img[x] + offset) >> shiftbits;
      img[x] = Clip3(minval, maxval, val);
    }
    img += stride;
  }
}

/**
 * Multiply all pixels in img by 2<sup>shiftbits</sup>.
 *
 * @param img        pointer to image to be transformed
 * @param stride     distance between vertically adjacent pixels of img.
 * @param width      width of active area in img.
 * @param height     height of active area in img.
 * @param shiftbits  number of bits to shift
 */
static void scalePlane(Pel* img, UInt stride, UInt width, UInt height,
                       UInt shiftbits)
{
  for (UInt y = 0; y < height; y++)
  {
    for (UInt x = 0; x < width; x++)
    {
      img[x] <<= shiftbits;
    }
    img += stride;
  }
}

/**
 * Scale all pixels in img depending upon sign of shiftbits by a factor of
 * 2<sup>shiftbits</sup>.
 *
 * @param img        pointer to image to be transformed
 * @param stride  distance between vertically adjacent pixels of img.
 * @param width   width of active area in img.
 * @param height  height of active area in img.
 * @param shiftbits if zero, no operation performed
 *                  if > 0, multiply by 2<sup>shiftbits</sup>, see scalePlane()
 *                  if < 0, divide and round by 2<sup>shiftbits</sup> and clip,
 *                          see invScalePlane().
 * @param minval  minimum clipping value when dividing.
 * @param maxval  maximum clipping value when dividing.
 */
static void scalePlane(Pel* img, UInt stride, UInt width, UInt height,
                       Int shiftbits, Pel minval, Pel maxval)
{
  if (shiftbits == 0)
  {
    return;
  }

  if (shiftbits > 0)
  {
    scalePlane(img, stride, width, height, shiftbits);
  }
  else
  {
    invScalePlane(img, stride, width, height, -shiftbits, minval, maxval);
  }
}


// ====================================================================================================================
// Public member functions
// ====================================================================================================================

/**
 * Open file for reading/writing Y'CbCr frames.
 *
 * Frames read/written have bitdepth fileBitDepth, and are automatically
 * formatted as 8 or 16 bit word values (see TVideoIOYuv::write()).
 *
 * Image data read or written is converted to/from internalBitDepth
 * (See scalePlane(), TVideoIOYuv::read() and TVideoIOYuv::write() for
 * further details).
 *
 * \param pchFile          file name string
 * \param bWriteMode       file open mode: true=read, false=write
 * \param fileBitDepthY     bit-depth of input/output file data (luma component).
 * \param fileBitDepthC     bit-depth of input/output file data (chroma components).
 * \param internalBitDepthY bit-depth to scale image data to/from when reading/writing (luma component).
 * \param internalBitDepthC bit-depth to scale image data to/from when reading/writing (chroma components).
 */
Void TVideoIOYuv::open( Char* pchFile, Bool bWriteMode, Int fileBitDepthY, Int fileBitDepthC, Int internalBitDepthY, Int internalBitDepthC)
{
  m_bitDepthShiftY = internalBitDepthY - fileBitDepthY;
  m_bitDepthShiftC = internalBitDepthC - fileBitDepthC;
  m_fileBitDepthY = fileBitDepthY;
  m_fileBitDepthC = fileBitDepthC;

  if ( bWriteMode )
  {
    m_cHandle.open( pchFile, ios::binary | ios::out );
    
    if( m_cHandle.fail() )
    {
      printf("\nfailed to write reconstructed YUV file\n");
      exit(0);
    }
  }
  else
  {
    m_cHandle.open( pchFile, ios::binary | ios::in );
    
    if( m_cHandle.fail() )
    {
      printf("\nfailed to open Input YUV file\n");
      exit(0);
    }
  }
  
  return;
}

Void TVideoIOYuv::close()
{
  m_cHandle.close();
}

Bool TVideoIOYuv::isEof()
{
  return m_cHandle.eof();
}

Bool TVideoIOYuv::isFail()
{
  return m_cHandle.fail();
}

/**
 * Skip numFrames in input.
 *
 * This function correctly handles cases where the input file is not
 * seekable, by consuming bytes.
 */
void TVideoIOYuv::skipFrames(UInt numFrames, UInt width, UInt height)
{
  if (!numFrames)
    return;

  const UInt wordsize = (m_fileBitDepthY > 8 || m_fileBitDepthC > 8) ? 2 : 1;
  const streamoff framesize = wordsize * width * height * 3 / 2;
  const streamoff offset = framesize * numFrames;

  /* attempt to seek */
  if (!!m_cHandle.seekg(offset, ios::cur))
    return; /* success */
  m_cHandle.clear();

  /* fall back to consuming the input */
  Char buf[512];
  const UInt offset_mod_bufsize = offset % sizeof(buf);
  for (streamoff i = 0; i < offset - offset_mod_bufsize; i += sizeof(buf))
  {
    m_cHandle.read(buf, sizeof(buf));
  }
  m_cHandle.read(buf, offset_mod_bufsize);
}

/**
 * Read width*height pixels from fd into dst, optionally
 * padding the left and right edges by edge-extension.  Input may be
 * either 8bit or 16bit little-endian lsb-aligned words.
 *
 * @param dst     destination image
 * @param fd      input file stream
 * @param is16bit true if input file carries > 8bit data, false otherwise.
 * @param stride  distance between vertically adjacent pixels of dst.
 * @param width   width of active area in dst.
 * @param height  height of active area in dst.
 * @param pad_x   length of horizontal padding.
 * @param pad_y   length of vertical padding.
 * @return true for success, false in case of error
 */
static Bool readPlane(Pel* dst, istream& fd, Bool is16bit,
                      UInt stride,
                      UInt width, UInt height,
                      UInt pad_x, UInt pad_y)
{
  Int read_len = width * (is16bit ? 2 : 1);
  UChar *buf = new UChar[read_len];
  for (Int y = 0; y < height; y++)
  {
    fd.read(reinterpret_cast<Char*>(buf), read_len);
    if (fd.eof() || fd.fail() )
    {
      delete[] buf;
      return false;
    }

    if (!is16bit)
    {
      for (Int x = 0; x < width; x++)
      {
        dst[x] = buf[x];
      }
    }
    else
    {
      for (Int x = 0; x < width; x++)
      {
        dst[x] = (buf[2*x+1] << 8) | buf[2*x];
      }
    }

    for (Int x = width; x < width + pad_x; x++)
    {
      dst[x] = dst[width - 1];
    }
    dst += stride;
  }
  for (Int y = height; y < height + pad_y; y++)
  {
    for (Int x = 0; x < width + pad_x; x++)
    {
      dst[x] = (dst - stride)[x];
    }
    dst += stride;
  }
  delete[] buf;
  return true;
}

/**
 * Write width*height pixels info fd from src.
 *
 * @param fd      output file stream
 * @param src     source image
 * @param is16bit true if input file carries > 8bit data, false otherwise.
 * @param stride  distance between vertically adjacent pixels of src.
 * @param width   width of active area in src.
 * @param height  height of active area in src.
 * @return true for success, false in case of error
 */
static Bool writePlane(ostream& fd, Pel* src, Bool is16bit,
                       UInt stride,
                       UInt width, UInt height)
{
  Int write_len = width * (is16bit ? 2 : 1);
  UChar *buf = new UChar[write_len];
  for (Int y = 0; y < height; y++)
  {
    if (!is16bit) 
    {
      for (Int x = 0; x < width; x++)
      {
        buf[x] = (UChar) src[x];
      }
    }
    else 
    {
      for (Int x = 0; x < width; x++)
      {
        buf[2*x] = src[x] & 0xff;
        buf[2*x+1] = (src[x] >> 8) & 0xff;
      }
    }

    fd.write(reinterpret_cast<Char*>(buf), write_len);
    if (fd.eof() || fd.fail() )
    {
      delete[] buf;
      return false;
    }
    src += stride;
  }
  delete[] buf;
  return true;
}

/**
 * Read one Y'CbCr frame, performing any required input scaling to change
 * from the bitdepth of the input file to the internal bit-depth.
 *
 * If a bit-depth reduction is required, and internalBitdepth >= 8, then
 * the input file is assumed to be ITU-R BT.601/709 compliant, and the
 * resulting data is clipped to the appropriate legal range, as if the
 * file had been provided at the lower-bitdepth compliant to Rec601/709.
 *
 * @param pPicYuv      input picture YUV buffer class pointer
 * @param aiPad        source padding size, aiPad[0] = horizontal, aiPad[1] = vertical
 * @return true for success, false in case of error
 */
Bool TVideoIOYuv::read ( TComPicYuv*  pPicYuv, Int aiPad[2] )
{
  // check end-of-file
  if ( isEof() ) return false;
  
  Int   iStride = pPicYuv->getStride();
  
  // compute actual YUV width & height excluding padding size
  UInt pad_h = aiPad[0];
  UInt pad_v = aiPad[1];
  UInt width_full = pPicYuv->getWidth();
  UInt height_full = pPicYuv->getHeight();
  UInt width  = width_full - pad_h;
  UInt height = height_full - pad_v;
  Bool is16bit = m_fileBitDepthY > 8 || m_fileBitDepthC > 8;

  Int desired_bitdepthY = m_fileBitDepthY + m_bitDepthShiftY;
  Int desired_bitdepthC = m_fileBitDepthC + m_bitDepthShiftC;
  Pel minvalY = 0;
  Pel minvalC = 0;
  Pel maxvalY = (1 << desired_bitdepthY) - 1;
  Pel maxvalC = (1 << desired_bitdepthC) - 1;
#if CLIP_TO_709_RANGE
  if (m_bitdepthShiftY < 0 && desired_bitdepthY >= 8)
  {
    /* ITU-R BT.709 compliant clipping for converting say 10b to 8b */
    minvalY = 1 << (desired_bitdepthY - 8);
    maxvalY = (0xff << (desired_bitdepthY - 8)) -1;
  }
  if (m_bitdepthShiftC < 0 && desired_bitdepthC >= 8)
  {
    /* ITU-R BT.709 compliant clipping for converting say 10b to 8b */
    minvalC = 1 << (desired_bitdepthC - 8);
    maxvalC = (0xff << (desired_bitdepthC - 8)) -1;
  }
#endif
  
  if (! readPlane(pPicYuv->getLumaAddr(), m_cHandle, is16bit, iStride, width, height, pad_h, pad_v))
    return false;
  scalePlane(pPicYuv->getLumaAddr(), iStride, width_full, height_full, m_bitDepthShiftY, minvalY, maxvalY);

  iStride >>= 1;
  width_full >>= 1;
  height_full >>= 1;
  width >>= 1;
  height >>= 1;
  pad_h >>= 1;
  pad_v >>= 1;

  if (! readPlane(pPicYuv->getCbAddr(), m_cHandle, is16bit, iStride, width, height, pad_h, pad_v))
    return false;
  scalePlane(pPicYuv->getCbAddr(), iStride, width_full, height_full, m_bitDepthShiftC, minvalC, maxvalC);

  if (! readPlane(pPicYuv->getCrAddr(), m_cHandle, is16bit, iStride, width, height, pad_h, pad_v))
    return false;
  scalePlane(pPicYuv->getCrAddr(), iStride, width_full, height_full, m_bitDepthShiftC, minvalC, maxvalC);

  return true;
}

/**
 * Write one Y'CbCr frame. No bit-depth conversion is performed, pcPicYuv is
 * assumed to be at TVideoIO::m_fileBitdepth depth.
 *
 * @param pPicYuv     input picture YUV buffer class pointer
 * @param aiPad       source padding size, aiPad[0] = horizontal, aiPad[1] = vertical
 * @return true for success, false in case of error
 */
Bool TVideoIOYuv::write( TComPicYuv* pPicYuv, Int confLeft, Int confRight, Int confTop, Int confBottom )
{
  // compute actual YUV frame size excluding padding size
  Int   iStride = pPicYuv->getStride();
  UInt  width  = pPicYuv->getWidth()  - confLeft - confRight;
  UInt  height = pPicYuv->getHeight() - confTop  - confBottom;
  Bool is16bit = m_fileBitDepthY > 8 || m_fileBitDepthC > 8;
  TComPicYuv *dstPicYuv = NULL;
  Bool retval = true;

  if (m_bitDepthShiftY != 0 || m_bitDepthShiftC != 0)
  {
    dstPicYuv = new TComPicYuv;
    dstPicYuv->create( pPicYuv->getWidth(), pPicYuv->getHeight(), 1, 1, 0 );
    pPicYuv->copyToPic(dstPicYuv);

    Pel minvalY = 0;
    Pel minvalC = 0;
    Pel maxvalY = (1 << m_fileBitDepthY) - 1;
    Pel maxvalC = (1 << m_fileBitDepthC) - 1;
#if CLIP_TO_709_RANGE
    if (-m_bitDepthShiftY < 0 && m_fileBitDepthY >= 8)
    {
      /* ITU-R BT.709 compliant clipping for converting say 10b to 8b */
      minvalY = 1 << (m_fileBitDepthY - 8);
      maxvalY = (0xff << (m_fileBitDepthY - 8)) -1;
    }
    if (-m_bitDepthShiftC < 0 && m_fileBitDepthC >= 8)
    {
      /* ITU-R BT.709 compliant clipping for converting say 10b to 8b */
      minvalC = 1 << (m_fileBitDepthC - 8);
      maxvalC = (0xff << (m_fileBitDepthC - 8)) -1;
    }
#endif
    scalePlane(dstPicYuv->getLumaAddr(), dstPicYuv->getStride(), dstPicYuv->getWidth(), dstPicYuv->getHeight(), -m_bitDepthShiftY, minvalY, maxvalY);
    scalePlane(dstPicYuv->getCbAddr(), dstPicYuv->getCStride(), dstPicYuv->getWidth()>>1, dstPicYuv->getHeight()>>1, -m_bitDepthShiftC, minvalC, maxvalC);
    scalePlane(dstPicYuv->getCrAddr(), dstPicYuv->getCStride(), dstPicYuv->getWidth()>>1, dstPicYuv->getHeight()>>1, -m_bitDepthShiftC, minvalC, maxvalC);
  }
  else
  {
    dstPicYuv = pPicYuv;
  }
  // location of upper left pel in a plane
  Int planeOffset = confLeft + confTop * iStride;
  
  if (! writePlane(m_cHandle, dstPicYuv->getLumaAddr() + planeOffset, is16bit, iStride, width, height))
  {
    retval=false; 
    goto exit;
  }

  width >>= 1;
  height >>= 1;
  iStride >>= 1;
  confLeft >>= 1;
  confRight >>= 1;
  confTop >>= 1;
  confBottom >>= 1;

  planeOffset = confLeft + confTop * iStride;

  if (! writePlane(m_cHandle, dstPicYuv->getCbAddr() + planeOffset, is16bit, iStride, width, height))
  {
    retval=false; 
    goto exit;
  }
  if (! writePlane(m_cHandle, dstPicYuv->getCrAddr() + planeOffset, is16bit, iStride, width, height))
  {
    retval=false; 
    goto exit;
  }
  
exit:
  if (m_bitDepthShiftY != 0 || m_bitDepthShiftC != 0)
  {
    dstPicYuv->destroy();
    delete dstPicYuv;
  }  
  return retval;
}

