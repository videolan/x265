/*****************************************************************************
 * Copyright (C) 2013 x265 project
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
 * For more information, contact us at licensing@multicorewareinc.com.
 *****************************************************************************/

#ifndef __MV__
#define __MV__

#include <stdint.h>

namespace x265 {
// private x265 namespace

#if _MSC_VER
#pragma warning(disable: 4201)
#endif
class MV
{
public:

    union {
        struct { int16_t x, y; };
        int32_t word;
    };

    MV() : word(0)                             {}

    MV(int32_t _w) : word(_w)                  {}

    MV(int16_t _x, int16_t _y) : x(_x), y(_y)  {}

    const MV& operator =(uint32_t w)           { word = w; return *this; }

    const MV& operator +=(const MV& other)     { x += other.x; y += other.y; return *this; }

    const MV& operator -=(const MV& other)     { x -= other.x; y -= other.y; return *this; }

    const MV& operator >>=(int i)              { x >>= i; y >>= i; return *this; }

    const MV& operator <<=(int i)              { x <<= i; y <<= i; return *this; }

    MV operator >>(int i) const                { return MV(x >> i, y >> i); }

    MV operator <<(int i) const                { return MV(x << i, y << i); }

    MV operator *(int16_t i) const             { return MV(x * i, y * i); }

    const MV operator -(const MV& other) const { return MV(x - other.x, y - other.y); }

    const MV operator +(const MV& other) const { return MV(x + other.x, y + other.y); }

    bool operator ==(const MV& other) const    { return word == other.word; }

    bool operator !=(const MV& other) const    { return word != other.word; }

    // Scale down a QPEL mv to FPEL mv, rounding up by one HPEL offset
    MV roundToFPel() const                     { return MV(x + 2, y + 2) >> 2; }

    MV toFPel() const                          { return *this >> 2; }

    // Scale up an FPEL mv to QPEL by shifting up two bits
    MV toQPel() const                          { return *this << 2; }

    bool inline notZero() const                { return this->word != 0; }

    MV mvmin(const MV& m) const                { return MV(x > m.x ? m.x : x, y > m.y ? m.y : y); }
    MV mvmax(const MV& m) const                { return MV(x < m.x ? m.x : x, y < m.y ? m.y : y); }

    MV clipped(const MV& _min, const MV& _max) const
    {
        MV cl = mvmin(_max);
        return cl.mvmax(_min);
    }

    // returns true if MV is within range (inclusive)
    bool checkRange(const MV& _min, const MV& _max) const
    {
        return x >= _min.x && x <= _max.x && y >= _min.y && y <= _max.y;
    }

    /* For compatibility with TComMV */
    void  set(int16_t _x, int16_t _y) { x = _x; y = _y; }

    void  setHor(short i)         { x = i; }

    void  setVer(short i)         { y = i; }

    short getHor() const          { return x; }

    short getVer() const          { return y; }
};
}

#endif // ifndef __MV__
