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

#include <cmath>
#include <cstdlib>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>

#include "TLibCommon/TComRom.h"
#include "TVideoIOY4m.h"

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
    Pel offset = 1 << (shiftbits - 1);

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
Void TVideoIOY4m::open(Char*        pchFile,
                       Bool         bWriteMode,
                       Int          internalBitDepthY,
                       Int          fileBitDepthY,
                       Int          internalBitDepthC,
                       Int          fileBitDepthC,
                       hnd_t* &     handler,
                       video_info_t video_info,
                       Int          aiPad[2])
{
    y4m_hnd_t* y4m_handler = NULL;

    if ((y4m_hnd_t*)handler == NULL)
    {
        y4m_handler = new y4m_hnd_t();
        //yuv_hnd_t *yuv_handler = malloc( sizeof(yuv_hnd_t) );

        if (bWriteMode)
        {
            y4m_handler->m_cHandle.open(pchFile, ios::binary | ios::out);

            if (y4m_handler->m_cHandle.fail())
            {
                printf("\nfailed to write reconstructed YUV file\n");
                exit(0);
            }
        }
        else
        {
            y4m_handler->m_cHandle.open(pchFile, ios::binary | ios::in);

            if (y4m_handler->m_cHandle.fail())
            {
                printf("\nfailed to open Input YUV file\n");
                exit(0);
            }
        }
    }
    else
    {
        y4m_handler = (y4m_hnd_t*)handler;
        y4m_handler->m_cHandle.seekg(y4m_handler->headerLength);
    }

    y4m_handler->bitDepthShiftY = internalBitDepthY - fileBitDepthY;
    y4m_handler->bitDepthShiftC = internalBitDepthC - fileBitDepthC;
    y4m_handler->fileBitDepthY = fileBitDepthY;
    y4m_handler->fileBitDepthC = fileBitDepthC;
    y4m_handler->aiPad[0] = aiPad[0];
    y4m_handler->aiPad[1] = aiPad[1];
    handler =  (hnd_t*)y4m_handler;
}

Void  TVideoIOY4m::getVideoInfo(video_info_t &video_info, hnd_t* &handler)
{
    y4m_hnd_t* y4m_handler = (y4m_hnd_t*)handler;
    /*stripping off the plaintext, quasi-freeform header */
    Char source[5];
    Int width = 0;
    Int height = 0;
    Int rateNumerator = 0;
    Int rateDenominator = 0;
    //Int headerLength = 0 ;
    Double rate = 30.0;

    while (1)
    {
        source[0] = 0x0;

        while ((source[0] != 0x20) && (source[0] != 0x0a))
        {
            y4m_handler->m_cHandle.read(source, 1);
            //headerLength++;
            if (y4m_handler->m_cHandle.eof() || y4m_handler->m_cHandle.fail())
            {
                break;
            }
        }

        if (source[0] == 0x00)
        {
            break;
        }

        while (source[0] == 0x20)
        {
            //  read parameter identifier

            y4m_handler->m_cHandle.read(source + 1, 1);
            //headerLength++;
            if (source[1] == 'W')
            {
                width = 0;
                while (true)
                {
                    y4m_handler->m_cHandle.read(source, 1);

                    if (source[0] == 0x20 || source[0] == 0x0a)
                    {
                        break;
                    }
                    else
                    {
                        width = width * 10 + (source[0] - '0');
                    }
                }

                continue;
            }

            if (source[1] == 'H')
            {
                height = 0;
                while (true)
                {
                    y4m_handler->m_cHandle.read(source, 1);
                    if (source[0] == 0x20 || source[0] == 0x0a)
                    {
                        break;
                    }
                    else
                    {
                        height = height * 10 + (source[0] - '0');
                    }
                }

                continue;
            }

            if (source[1] == 'F')
            {
                rateNumerator = 0;
                rateDenominator = 0;
                while (true)
                {
                    y4m_handler->m_cHandle.read(source, 1);
                    if (source[0] == '.')
                    {
                        rateDenominator = 1;
                        while (true)
                        {
                            y4m_handler->m_cHandle.read(source, 1);
                            if (source[0] == 0x20 || source[0] == 0x10)
                            {
                                break;
                            }
                            else
                            {
                                rateNumerator = rateNumerator * 10 + (source[0] - '0');
                                rateDenominator = rateDenominator * 10;
                            }
                        }

                        rate = (Double)rateNumerator / rateDenominator;
                        break;
                    }
                    else if (source[0] == ':')
                    {
                        while (true)
                        {
                            y4m_handler->m_cHandle.read(source, 1);
                            if (source[0] == 0x20 || source[0] == 0x0a)
                            {
                                break;
                            }
                            else
                                rateDenominator = rateDenominator * 10 + (source[0] - '0');
                        }

                        rate = (Double)rateNumerator / rateDenominator;
                        break;
                    }
                    else
                    {
                        rateNumerator = rateNumerator * 10 + (source[0] - '0');
                    }
                }

                continue;
            }

            break;
        }

        if (source[0] == 0x0a)
        {
            break;
        }
    }

    y4m_handler->headerLength = y4m_handler->m_cHandle.tellg();
    video_info.width = width;
    video_info.height = height;
    video_info.FrameRate = ceil(rate);
}

Void TVideoIOY4m::close(hnd_t* &handler)
{
    y4m_hnd_t* y4m_handler = (y4m_hnd_t*)handler;

    y4m_handler->m_cHandle.close();
}

Bool TVideoIOY4m::isEof(hnd_t* &handler)
{
    y4m_hnd_t* y4m_handler = (y4m_hnd_t*)handler;

    return y4m_handler->m_cHandle.eof();
}

Bool TVideoIOY4m::isFail(hnd_t* &handler)
{
    y4m_hnd_t* y4m_handler = (y4m_hnd_t*)handler;

    return y4m_handler->m_cHandle.fail();
}

/**
 * Skip numFrames in input.
 *
 * This function correctly handles cases where the input file is not
 * seekable, by consuming bytes.
 */
void TVideoIOY4m::skipFrames(UInt numFrames, UInt width, UInt height, hnd_t* &handler)
{
    y4m_hnd_t* y4m_handler = (y4m_hnd_t*)handler;

    if (!numFrames)
        return;

    const UInt wordsize = (y4m_handler->fileBitDepthY > 8 || y4m_handler->fileBitDepthC > 8) ? 2 : 1;
    const streamoff framesize = wordsize * width * height * 3 / 2;
    const streamoff offset = framesize * numFrames;

    /* attempt to seek */
    if (!!y4m_handler->m_cHandle.seekg(offset, ios::cur))
        return; /* success */

    y4m_handler->m_cHandle.clear();

    /* fall back to consuming the input */
    Char buf[512];
    const UInt offset_mod_bufsize = offset % sizeof(buf);
    for (streamoff i = 0; i < offset - offset_mod_bufsize; i += sizeof(buf))
    {
        y4m_handler->m_cHandle.read(buf, sizeof(buf));
    }

    y4m_handler->m_cHandle.read(buf, offset_mod_bufsize);
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
        if (fd.eof() || fd.fail())
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
                dst[x] = (buf[2 * x + 1] << 8) | buf[2 * x];
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
                buf[x] = (UChar)src[x];
            }
        }
        else
        {
            for (Int x = 0; x < width; x++)
            {
                buf[2 * x] = src[x] & 0xff;
                buf[2 * x + 1] = (src[x] >> 8) & 0xff;
            }
        }

        fd.write(reinterpret_cast<Char*>(buf), write_len);
        if (fd.eof() || fd.fail())
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
Bool TVideoIOY4m::read(TComPicYuv* pPicYuv,  hnd_t* &handler)
{
    y4m_hnd_t* y4m_handler = (y4m_hnd_t*)handler;

    // check end-of-file
    if (isEof(handler)) return false;

    Int   iStride = pPicYuv->getStride();

    // compute actual YUV width & height excluding padding size
    UInt pad_h = y4m_handler->aiPad[0];
    UInt pad_v = y4m_handler->aiPad[1];
    UInt width_full = pPicYuv->getWidth();
    UInt height_full = pPicYuv->getHeight();
    UInt width  = width_full - pad_h;
    UInt height = height_full - pad_v;
    Bool is16bit = y4m_handler->fileBitDepthY > 8 || y4m_handler->fileBitDepthC > 8;

    Int desired_bitdepthY = y4m_handler->fileBitDepthY + y4m_handler->bitDepthShiftY;
    Int desired_bitdepthC = y4m_handler->fileBitDepthC + y4m_handler->bitDepthShiftC;
    Pel minvalY = 0;
    Pel minvalC = 0;
    Pel maxvalY = (1 << desired_bitdepthY) - 1;
    Pel maxvalC = (1 << desired_bitdepthC) - 1;
#if CLIP_TO_709_RANGE
    if (y4m_handler->bitdepthShiftY < 0 && desired_bitdepthY >= 8)
    {
        /* ITU-R BT.709 compliant clipping for converting say 10b to 8b */
        minvalY = 1 << (desired_bitdepthY - 8);
        maxvalY = (0xff << (desired_bitdepthY - 8)) - 1;
    }

    if (y4m_handler->bitdepthShiftC < 0 && desired_bitdepthC >= 8)
    {
        /* ITU-R BT.709 compliant clipping for converting say 10b to 8b */
        minvalC = 1 << (desired_bitdepthC - 8);
        maxvalC = (0xff << (desired_bitdepthC - 8)) - 1;
    }

#endif // if CLIP_TO_709_RANGE

    /*stripe off the FRAME header */
    char byte[1];
    Int cur_pointer = y4m_handler->m_cHandle.tellg();
    cur_pointer += Y4M_FRAME_MAGIC;
    y4m_handler->m_cHandle.seekg(cur_pointer);
    while (1)
    {
        byte[0] = 0;
        y4m_handler->m_cHandle.read(byte, 1);
        if (y4m_handler->m_cHandle.eof() || y4m_handler->m_cHandle.fail())
        {
            break;
        }

        //if(*byte==0x0a){
        //	break;
        //}
label:
        while (*byte != 0x0a)
        {
            y4m_handler->m_cHandle.read(byte, 1);
        }

        y4m_handler->m_cHandle.read(byte, 1);
        if (*(byte) == 0x20)
        {
            goto label;
        }
        else
        {
            break;
        }
    }

    cur_pointer = y4m_handler->m_cHandle.tellg();
    y4m_handler->m_cHandle.seekg(cur_pointer - 1);
    cur_pointer = y4m_handler->m_cHandle.tellg();

    if (!readPlane(pPicYuv->getLumaAddr(), y4m_handler->m_cHandle, is16bit, iStride, width, height, pad_h, pad_v))
        return false;

    cur_pointer = y4m_handler->m_cHandle.tellg();
    scalePlane(pPicYuv->getLumaAddr(), iStride, width_full, height_full, y4m_handler->bitDepthShiftY, minvalY, maxvalY);

    iStride >>= 1;
    width_full >>= 1;
    height_full >>= 1;
    width >>= 1;
    height >>= 1;
    pad_h >>= 1;
    pad_v >>= 1;

    if (!readPlane(pPicYuv->getCbAddr(), y4m_handler->m_cHandle, is16bit, iStride, width, height, pad_h, pad_v))
        return false;

    cur_pointer = y4m_handler->m_cHandle.tellg();
    scalePlane(pPicYuv->getCbAddr(), iStride, width_full, height_full, y4m_handler->bitDepthShiftC, minvalC, maxvalC);

    if (!readPlane(pPicYuv->getCrAddr(), y4m_handler->m_cHandle, is16bit, iStride, width, height, pad_h, pad_v))
        return false;

    cur_pointer = y4m_handler->m_cHandle.tellg();
    scalePlane(pPicYuv->getCrAddr(), iStride, width_full, height_full, y4m_handler->bitDepthShiftC, minvalC, maxvalC);
    cur_pointer = y4m_handler->m_cHandle.tellg();
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
Bool TVideoIOY4m::write(TComPicYuv* pPicYuv, hnd_t* &handler, Int confLeft, Int confRight, Int confTop, Int confBottom)
{
    y4m_hnd_t* y4m_handler = (y4m_hnd_t*)handler;
    // compute actual YUV frame size excluding padding size
    Int   iStride = pPicYuv->getStride();
    UInt  width  = pPicYuv->getWidth()  - confLeft - confRight;
    UInt  height = pPicYuv->getHeight() - confTop  - confBottom;
    Bool is16bit = y4m_handler->fileBitDepthY > 8 || y4m_handler->fileBitDepthC > 8;
    TComPicYuv *dstPicYuv = NULL;
    Bool retval = true;

    if (y4m_handler->bitDepthShiftY != 0 || y4m_handler->bitDepthShiftC != 0)
    {
        dstPicYuv = new TComPicYuv;
        dstPicYuv->create(pPicYuv->getWidth(), pPicYuv->getHeight(), 1, 1, 0);
        pPicYuv->copyToPic(dstPicYuv);

        Pel minvalY = 0;
        Pel minvalC = 0;
        Pel maxvalY = (1 << y4m_handler->fileBitDepthY) - 1;
        Pel maxvalC = (1 << y4m_handler->fileBitDepthC) - 1;
#if CLIP_TO_709_RANGE
        if (-y4m_handler->bitDepthShiftY < 0 && y4m_handler->fileBitDepthY >= 8)
        {
            /* ITU-R BT.709 compliant clipping for converting say 10b to 8b */
            minvalY = 1 << (y4m_handler->fileBitDepthY - 8);
            maxvalY = (0xff << (y4m_handler->fileBitDepthY - 8)) - 1;
        }

        if (-y4m_handler->bitDepthShiftC < 0 && y4m_handler->fileBitDepthC >= 8)
        {
            /* ITU-R BT.709 compliant clipping for converting say 10b to 8b */
            minvalC = 1 << (y4m_handler->fileBitDepthC - 8);
            maxvalC = (0xff << (y4m_handler->fileBitDepthC - 8)) - 1;
        }

#endif // if CLIP_TO_709_RANGE
        scalePlane(dstPicYuv->getLumaAddr(), dstPicYuv->getStride(), dstPicYuv->getWidth(),
                   dstPicYuv->getHeight(), -y4m_handler->bitDepthShiftY, minvalY, maxvalY);
        scalePlane(dstPicYuv->getCbAddr(), dstPicYuv->getCStride(), dstPicYuv->getWidth() >> 1,
                   dstPicYuv->getHeight() >> 1, -y4m_handler->bitDepthShiftC, minvalC, maxvalC);
        scalePlane(dstPicYuv->getCrAddr(), dstPicYuv->getCStride(), dstPicYuv->getWidth() >> 1,
                   dstPicYuv->getHeight() >> 1, -y4m_handler->bitDepthShiftC, minvalC, maxvalC);
    }
    else
    {
        dstPicYuv = pPicYuv;
    }

    // location of upper left pel in a plane
    Int planeOffset = confLeft + confTop * iStride;

    if (!writePlane(y4m_handler->m_cHandle, dstPicYuv->getLumaAddr() + planeOffset, is16bit, iStride, width, height))
    {
        retval = false;
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

    if (!writePlane(y4m_handler->m_cHandle, dstPicYuv->getCbAddr() + planeOffset, is16bit, iStride, width, height))
    {
        retval = false;
        goto exit;
    }

    if (!writePlane(y4m_handler->m_cHandle, dstPicYuv->getCrAddr() + planeOffset, is16bit, iStride, width, height))
    {
        retval = false;
        goto exit;
    }

exit:
    if (y4m_handler->bitDepthShiftY != 0 || y4m_handler->bitDepthShiftC != 0)
    {
        dstPicYuv->destroy();
        delete dstPicYuv;
    }

    return retval;
}
