/*****************************************************************************
 * Copyright (C) 2015 x265 project
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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_PICYUV_H
#define X265_PICYUV_H

#include "common.h"
#include "md5.h"
#include "x265.h"

namespace X265_NS {
// private namespace

class ShortYuv;
struct SPS;

class PicYuv
{
public:

    pixel*   m_picBuf[3];  // full allocated buffers, including margins
    pixel*   m_picOrg[3];  // pointers to plane starts

    uint32_t m_picWidth;
    uint32_t m_picHeight;
    intptr_t m_stride;
    intptr_t m_strideC;

    uint32_t m_picCsp;
    uint32_t m_hChromaShift;
    uint32_t m_vChromaShift;

    intptr_t* m_cuOffsetY;  /* these four buffers are owned by the top-level encoder */
    intptr_t* m_cuOffsetC;
    intptr_t* m_buOffsetY;
    intptr_t* m_buOffsetC;

    uint32_t m_lumaMarginX;
    uint32_t m_lumaMarginY;
    uint32_t m_chromaMarginX;
    uint32_t m_chromaMarginY;

    uint16_t m_maxLumaLevel;
    double   m_avgLumaLevel;

    PicYuv();

    bool  create(uint32_t picWidth, uint32_t picHeight, uint32_t csp);
    bool  createOffsets(const SPS& sps);
    void  destroy();

    void  copyFromPicture(const x265_picture&, const x265_param& param, int padx, int pady);

    intptr_t getChromaAddrOffset(uint32_t ctuAddr, uint32_t absPartIdx) const { return m_cuOffsetC[ctuAddr] + m_buOffsetC[absPartIdx]; }

    /* get pointer to CTU start address */
    pixel*  getLumaAddr(uint32_t ctuAddr)                      { return m_picOrg[0] + m_cuOffsetY[ctuAddr]; }
    pixel*  getCbAddr(uint32_t ctuAddr)                        { return m_picOrg[1] + m_cuOffsetC[ctuAddr]; }
    pixel*  getCrAddr(uint32_t ctuAddr)                        { return m_picOrg[2] + m_cuOffsetC[ctuAddr]; }
    pixel*  getChromaAddr(uint32_t chromaId, uint32_t ctuAddr) { return m_picOrg[chromaId] + m_cuOffsetC[ctuAddr]; }
    pixel*  getPlaneAddr(uint32_t plane, uint32_t ctuAddr)     { return m_picOrg[plane] + (plane ? m_cuOffsetC[ctuAddr] : m_cuOffsetY[ctuAddr]); }
    const pixel* getLumaAddr(uint32_t ctuAddr) const           { return m_picOrg[0] + m_cuOffsetY[ctuAddr]; }
    const pixel* getCbAddr(uint32_t ctuAddr) const             { return m_picOrg[1] + m_cuOffsetC[ctuAddr]; }
    const pixel* getCrAddr(uint32_t ctuAddr) const             { return m_picOrg[2] + m_cuOffsetC[ctuAddr]; }
    const pixel* getChromaAddr(uint32_t chromaId, uint32_t ctuAddr) const { return m_picOrg[chromaId] + m_cuOffsetC[ctuAddr]; }
    const pixel* getPlaneAddr(uint32_t plane, uint32_t ctuAddr) const     { return m_picOrg[plane] + (plane ? m_cuOffsetC[ctuAddr] : m_cuOffsetY[ctuAddr]); }

    /* get pointer to CU start address */
    pixel*  getLumaAddr(uint32_t ctuAddr, uint32_t absPartIdx) { return m_picOrg[0] + m_cuOffsetY[ctuAddr] + m_buOffsetY[absPartIdx]; }
    pixel*  getCbAddr(uint32_t ctuAddr, uint32_t absPartIdx)   { return m_picOrg[1] + m_cuOffsetC[ctuAddr] + m_buOffsetC[absPartIdx]; }
    pixel*  getCrAddr(uint32_t ctuAddr, uint32_t absPartIdx)   { return m_picOrg[2] + m_cuOffsetC[ctuAddr] + m_buOffsetC[absPartIdx]; }
    pixel*  getChromaAddr(uint32_t chromaId, uint32_t ctuAddr, uint32_t absPartIdx) { return m_picOrg[chromaId] + m_cuOffsetC[ctuAddr] + m_buOffsetC[absPartIdx]; }
    const pixel* getLumaAddr(uint32_t ctuAddr, uint32_t absPartIdx) const { return m_picOrg[0] + m_cuOffsetY[ctuAddr] + m_buOffsetY[absPartIdx]; }
    const pixel* getCbAddr(uint32_t ctuAddr, uint32_t absPartIdx) const   { return m_picOrg[1] + m_cuOffsetC[ctuAddr] + m_buOffsetC[absPartIdx]; }
    const pixel* getCrAddr(uint32_t ctuAddr, uint32_t absPartIdx) const   { return m_picOrg[2] + m_cuOffsetC[ctuAddr] + m_buOffsetC[absPartIdx]; }
    const pixel* getChromaAddr(uint32_t chromaId, uint32_t ctuAddr, uint32_t absPartIdx) const { return m_picOrg[chromaId] + m_cuOffsetC[ctuAddr] + m_buOffsetC[absPartIdx]; }
};

void updateChecksum(const pixel* plane, uint32_t& checksumVal, uint32_t height, uint32_t width, intptr_t stride, int row, uint32_t cuHeight);
void updateCRC(const pixel* plane, uint32_t& crcVal, uint32_t height, uint32_t width, intptr_t stride);
void crcFinish(uint32_t & crc, uint8_t digest[16]);
void checksumFinish(uint32_t checksum, uint8_t digest[16]);
void updateMD5Plane(MD5Context& md5, const pixel* plane, uint32_t width, uint32_t height, intptr_t stride);
}

#endif // ifndef X265_PICYUV_H
