/*****************************************************************************
 * Copyright (C) 2013-2015 x265 project
 *
 * Authors: Steve Borho <steve@borho.org>
 *          Xinyue Lu <i@7086.in>
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

#ifndef X265_HEVC_RAW_H
#define X265_HEVC_RAW_H

#include "output.h"
#include "common.h"
#include <fstream>
#include <iostream>

namespace X265_NS {
class RAWOutput : public OutputFile
{
protected:

    std::ostream* ofs;

    bool b_fail;

public:

    RAWOutput(const char* fname, InputFileInfo&);

    bool isFail() const { return b_fail; }

    bool needPTS() const { return false; }

    void release() { delete this; }

    const char* getName() const { return "raw"; }

    void setParam(x265_param* param);

    int writeHeaders(const x265_nal* nal, uint32_t nalcount);

    int writeFrame(const x265_nal* nal, uint32_t nalcount, x265_picture&);

    void closeFile(int64_t largest_pts, int64_t second_largest_pts);
};
}

#endif // ifndef X265_HEVC_RAW_H
