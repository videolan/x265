/*****************************************************************************
 * Copyright (C) 2014 x265 project
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
#include "TLibCommon/TComRom.h"
#include "x265.h"

namespace x265 {
// private namespace

class ShortYuv;

class PicYuv
{
public:

    pixel*  m_picBuf[3];  // full allocated buffers, including margins
    pixel*  m_picOrg[3];  // pointers to plane starts

    int      m_picWidth;
    int      m_picHeight;
    int      m_stride;
    int      m_strideC;

    int      m_numCuInWidth;
    int      m_numCuInHeight;

    int      m_picCsp;
    int      m_hChromaShift;
    int      m_vChromaShift;

    int32_t* m_cuOffsetY;
    int32_t* m_cuOffsetC;
    int32_t* m_buOffsetY;
    int32_t* m_buOffsetC;

    int      m_lumaMarginX;
    int      m_lumaMarginY;
    int      m_chromaMarginX;
    int      m_chromaMarginY;

    PicYuv();

    bool  create(int picWidth, int picHeight, int csp);
    void  destroy();
    void  copyFromPicture(const x265_picture&, int padx, int pady);

    uint32_t getCUHeight(int rowNum) const;
    int32_t  getChromaAddrOffset(int ctuAddr, int absZOrderIdx) const { return m_cuOffsetC[ctuAddr] + m_buOffsetC[absZOrderIdx]; }

    /* get pointer to CTU start address */
    pixel*  getLumaAddr(int ctuAddr)                                   { return m_picOrg[0] + m_cuOffsetY[ctuAddr]; }
    pixel*  getCbAddr(int ctuAddr)                                     { return m_picOrg[1] + m_cuOffsetC[ctuAddr]; }
    pixel*  getCrAddr(int ctuAddr)                                     { return m_picOrg[2] + m_cuOffsetC[ctuAddr]; }
    pixel*  getChromaAddr(int chromaId, int ctuAddr)                   { return m_picOrg[chromaId] + m_cuOffsetC[ctuAddr]; }
    pixel*  getPlaneAddr(int plane, int ctuAddr)                       { return m_picOrg[plane] + (plane ? m_cuOffsetC[ctuAddr] : m_cuOffsetY[ctuAddr]); }

    /* get pointer to CU start address */
    pixel*  getLumaAddr(int ctuAddr, int absZOrderIdx)                 { return m_picOrg[0] + m_cuOffsetY[ctuAddr] + m_buOffsetY[absZOrderIdx]; }
    pixel*  getCbAddr(int ctuAddr, int absZOrderIdx)                   { return m_picOrg[1] + m_cuOffsetC[ctuAddr] + m_buOffsetC[absZOrderIdx]; }
    pixel*  getCrAddr(int ctuAddr, int absZOrderIdx)                   { return m_picOrg[2] + m_cuOffsetC[ctuAddr] + m_buOffsetC[absZOrderIdx]; }
    pixel*  getChromaAddr(int chromaId, int ctuAddr, int absZOrderIdx) { return m_picOrg[chromaId] + m_cuOffsetC[ctuAddr] + m_buOffsetC[absZOrderIdx]; }
};

void updateChecksum(const pixel* plane, uint32_t& checksumVal, uint32_t height, uint32_t width, uint32_t stride, int row, uint32_t cuHeight);
void updateCRC(const pixel* plane, uint32_t& crcVal, uint32_t height, uint32_t width, uint32_t stride);
void crcFinish(uint32_t & crc, uint8_t digest[16]);
void checksumFinish(uint32_t checksum, uint8_t digest[16]);
void updateMD5Plane(MD5Context& md5, const pixel* plane, uint32_t width, uint32_t height, uint32_t stride);
}

#endif // ifndef X265_PICYUV_H
