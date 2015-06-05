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
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#ifndef X265_Y4M_H
#define X265_Y4M_H

#include "output.h"
#include <fstream>

namespace X265_NS {
// private x265 namespace

class Y4MOutput : public ReconFile
{
protected:

    int width;

    int height;

    int colorSpace;

    uint32_t frameSize;

    std::ofstream ofs;

    std::ofstream::pos_type header;

    char *buf;

    void writeHeader();

public:

    Y4MOutput(const char *filename, int width, int height, uint32_t fpsNum, uint32_t fpsDenom, int csp);

    virtual ~Y4MOutput();

    const char *getName() const                   { return "y4m"; }

    bool isFail() const                           { return ofs.fail(); }

    void release()                                { delete this; }

    bool writePicture(const x265_picture& pic);
};
}

#endif // ifndef X265_Y4M_H
