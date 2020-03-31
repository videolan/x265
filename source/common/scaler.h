/*****************************************************************************
 * Copyright (C) 2013-2020 MulticoreWare, Inc
 *
 * Authors: Pooja Venkatesan <pooja@multicorewareinc.com>
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_SCALER_H
#define X265_SCALER_H

#include "common.h"

namespace X265_NS {
//x265 private namespace

class ScalerSlice;
class VideoDesc;

#define MAX_NUM_LINES_AHEAD 4
#define SCALER_ALIGN(x, j) (((x)+(j)-1)&~((j)-1))
#define X265_ABS(j) ((j) >= 0 ? (j) : (-(j)))
#define SCALER_MAX_REDUCE_CUTOFF 0.002
#define SCALER_BITEXACT  0x80000
#define ROUNDED_DIVISION(i,j) (((i)>0 ? (i) + ((j)>>1) : (i) - ((j)>>1))/(j))
#define UH_CEIL_SHIFTR(i,j) (!scale_builtin_constant_p(j) ? -((-(i)) >> (j)) \
                                                          : ((i) + (1<<(j)) - 1) >> (j))

#if defined(__GNUC__) || defined(__clang__)
#    define scale_builtin_constant_p __builtin_constant_p
#else
#    define scale_builtin_constant_p(x) 0
#endif

enum ResFactor
{
    RES_FACTOR_64, RES_FACTOR_32, RES_FACTOR_16, RES_FACTOR_8,
    RES_FACTOR_4, RES_FACTOR_DEF, NUM_RES_FACTOR
};

enum ScalerFactor
{
    FACTOR_4, FACTOR_8, NUM_FACTOR
};

enum FilterSize
{
    FIL_4, FIL_6, FIL_8, FIL_9, FIL_10, FIL_11, FIL_13, FIL_15,
    FIL_16, FIL_17, FIL_19, FIL_22, FIL_24, FIL_DEF, NUM_FIL
};

class ScalerFilter {
public:
    int             m_filtLen;
    int32_t*        m_filtPos;      // Array of horizontal/vertical starting pos for each dst for luma / chroma planes.
    int16_t*        m_filt;         // Array of horizontal/vertical filter coefficients for luma / chroma planes.
    ScalerSlice*    m_sourceSlice;  // Source slice
    ScalerSlice*    m_destSlice;    // Output slice
    ScalerFilter();
    virtual ~ScalerFilter();
    virtual void process(int sliceVer, int sliceHor) = 0;
    int initCoeff(int flag, int inc, int srcW, int dstW, int filtAlign, int one, int sourcePos, int destPos);
    void setSlice(ScalerSlice* source, ScalerSlice* dest) { m_sourceSlice = source; m_destSlice = dest; }
};

class VideoDesc {
public:
    int         m_width;
    int         m_height;
    int         m_csp;
    int         m_inputDepth;

    VideoDesc(int w, int h, int csp, int bitDepth)
    {
        m_width = w;
        m_height = h;
        m_csp = csp;
        m_inputDepth = bitDepth;
    }
};

typedef struct ScalerPlane
{
    int       availLines; // max number of lines that can be held by this plane
    int       sliceVer;   // index of first line
    int       sliceHor;   // number of lines
    uint8_t** lineBuf;    // line buffer
} ScalerPlane;

// Assist horizontal filtering, base class
class HFilterScaler {
public:
    int m_bitDepth;
public:
    HFilterScaler() :m_bitDepth(0) {};
    virtual ~HFilterScaler() {};
    virtual void doScaling(int16_t *dst, int dstW, const uint8_t *src, const int16_t *filter, const int32_t *filterPos, int filterSize) = 0;
};

// Assist vertical filtering, base class
class VFilterScaler {
public:
    int m_bitDepth;
public:
    VFilterScaler() :m_bitDepth(0) {};
    virtual ~VFilterScaler() {};
    virtual void yuv2PlaneX(const int16_t *filter, int filterSize, const int16_t **src, uint8_t *dest, int dstW) = 0;
};

//  Assist horizontal filtering, process 8 bit case
class HFilterScaler8Bit : public HFilterScaler {
public:
    HFilterScaler8Bit() { m_bitDepth = 8; }
    virtual void doScaling(int16_t *dst, int dstW, const uint8_t *src, const int16_t *filter, const int32_t *filterPos, int filterSize);
};

//  Assist horizontal filtering, process 10 bit case
class HFilterScaler10Bit : public HFilterScaler {
public:
    HFilterScaler10Bit() { m_bitDepth = 10; }
    virtual void doScaling(int16_t *dst, int dstW, const uint8_t *src, const int16_t *filter, const int32_t *filterPos, int filterSize);
};

//  Assist vertical filtering, process 8 bit case
class VFilterScaler8Bit : public VFilterScaler {
public:
    VFilterScaler8Bit() { m_bitDepth = 8; }
    virtual void yuv2PlaneX(const int16_t *filter, int filterSize, const int16_t **src, uint8_t *dest, int dstW);
};

//  Assist vertical filtering, process 10 bit case
class VFilterScaler10Bit : public VFilterScaler {
public:
    VFilterScaler10Bit() { m_bitDepth = 10; }
    virtual void yuv2PlaneX(const int16_t *filter, int filterSize, const int16_t **src, uint8_t *dest, int dstW);
};

// Horizontal filter for luma
class ScalerHLumFilter : public ScalerFilter {
private:
    HFilterScaler* m_hFilterScaler;
public:
    ScalerHLumFilter(int bitDepth) { bitDepth == 8 ? m_hFilterScaler = new HFilterScaler8Bit : bitDepth == 10 ? m_hFilterScaler = new HFilterScaler10Bit : NULL;}
    ~ScalerHLumFilter() { if (m_hFilterScaler) X265_FREE(m_hFilterScaler); }
    virtual void process(int sliceVer, int sliceHor);
};

// Horizontal filter for chroma
class ScalerHCrFilter : public ScalerFilter {
private:
    HFilterScaler* m_hFilterScaler;
public:
    ScalerHCrFilter(int bitDepth) { bitDepth == 8 ? m_hFilterScaler = new HFilterScaler8Bit : bitDepth == 10 ? m_hFilterScaler = new HFilterScaler10Bit : NULL;}
    ~ScalerHCrFilter() { if (m_hFilterScaler) X265_FREE(m_hFilterScaler); }
    virtual void process(int sliceVer, int sliceHor);
};

// Vertical filter for luma
class ScalerVLumFilter : public ScalerFilter {
private:
    VFilterScaler* m_vFilterScaler;
public:
    ScalerVLumFilter(int bitDepth) { bitDepth == 8 ? m_vFilterScaler = new VFilterScaler8Bit : bitDepth == 10 ? m_vFilterScaler = new VFilterScaler10Bit : NULL;}
    ~ScalerVLumFilter() { if (m_vFilterScaler) X265_FREE(m_vFilterScaler); }
    virtual void process(int sliceVer, int sliceHor);
};

// Vertical filter for chroma
class ScalerVCrFilter : public ScalerFilter {
private:
    VFilterScaler*    m_vFilterScaler;
public:
    ScalerVCrFilter(int bitDepth) { bitDepth == 8 ? m_vFilterScaler = new VFilterScaler8Bit : bitDepth == 10 ? m_vFilterScaler = new VFilterScaler10Bit : NULL;}
    ~ScalerVCrFilter() { if (m_vFilterScaler) X265_FREE(m_vFilterScaler); }
    virtual void process(int sliceVer, int sliceHor);
};

class ScalerSlice
{
private:
    enum ScalerSlicePlaneNum { m_numSlicePlane = 4 };
public:
    int m_width;        // Slice line width
    int m_hCrSubSample; // horizontal Chroma subsampling factor
    int m_vCrSubSample; // vertical chroma subsampling factor
    int m_isRing;       // flag to identify if this ScalerSlice is a ring buffer
    int m_destroyLines; // flag to identify if there are dynamic allocated lines
    ScalerPlane m_plane[m_numSlicePlane];
public:
    ScalerSlice();
    ~ScalerSlice() { destroy(); }
    int rotate(int lum, int cr);
    void fillOnes(int n, int is16bit);
    int create(int lumLines, int crLines, int h_sub_sample, int v_sub_sample, int ring);
    int createLines(int size, int width);
    void destroyLines();
    void destroy();
    int initFromSrc(uint8_t *src[4], const int stride[4], int srcW, int lumY, int lumH, int crY, int crH, int relative);
};

class ScalerFilterManager {
private:
    enum ScalerFilterNum { m_numSlice = 3, m_numFilter = 4 };

private:
    int                     m_bitDepth;
    int                     m_algorithmFlags;  // 1, bilinear; 4 bicubic, default is bicubic
    int                     m_srcW;            // Width  of source luma planes.
    int                     m_srcH;            // Height of source luma planes.
    int                     m_dstW;            // Width of dest luma planes.
    int                     m_dstH;            // Height of dest luma planes.
    int                     m_crSrcW;          // Width  of source chroma planes.
    int                     m_crSrcH;          // Height of source chroma planes.
    int                     m_crDstW;          // Width  of dest chroma planes.
    int                     m_crDstH;          // Height of dest chroma planes.
    int                     m_crSrcHSubSample; // Binary log of horizontal subsampling factor between Y and Cr planes in src  image.
    int                     m_crSrcVSubSample; // Binary log of vertical   subsampling factor between Y and Cr planes in src  image.
    int                     m_crDstHSubSample; // Binary log of horizontal subsampling factor between Y and Cr planes in dest image.
    int                     m_crDstVSubSample; // Binary log of vertical   subsampling factor between Y and Cr planes in dest image.
    ScalerSlice*            m_slices[m_numSlice];
    ScalerFilter*           m_ScalerFilters[m_numFilter];
private:
    int getLocalPos(int crSubSample, int pos);
    void getMinBufferSize(int *out_lum_size, int *out_cr_size);
    int initScalerSlice();
public:
    ScalerFilterManager();
    ~ScalerFilterManager() {
        for (int i = 0; i < m_numSlice; i++)
            if (m_slices[i]) { m_slices[i]->destroy(); delete m_slices[i]; m_slices[i] = NULL; }
        for (int i = 0; i < m_numFilter; i++)
            if (m_ScalerFilters[i]) { delete m_ScalerFilters[i]; m_ScalerFilters[i] = NULL; }
    }
    int init(int algorithmFlags, VideoDesc* srcVideoDesc, VideoDesc* dstVideoDesc);
    int scale_pic(void** src, void** dst, int* srcStride, int* dstStride);
};
}

#endif //ifndef X265_SCALER_H
