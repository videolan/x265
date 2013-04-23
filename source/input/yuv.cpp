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

#include "yuv.h"
#include "PPA/ppa.h"
#include <stdio.h>
#include <string.h>

using namespace x265;
using namespace std;

YUVInput::YUVInput(const char *filename)
{
    ifs.open(filename, ios::binary | ios::in);
    width = height = 0;
    depth = 8;
    buf = NULL;
}

YUVInput::~YUVInput()
{
    ifs.close();
    if (buf) delete[] buf;
}

int YUVInput::guessFrameCount()
{
    long cur = ifs.tellg();
    ifs.seekg (0, ios::end);
    long size = ifs.tellg();
    ifs.seekg (cur, ios::beg);
    int pixelbytes = depth > 8 ? 2 : 1;

    return (size - cur) / (width * height * pixelbytes * 3 / 2);
}

void YUVInput::skipFrames(int numFrames)
{
    int pixelbytes = depth > 8 ? 2 : 1;

    int framesize = (width * height * 3 / 2) * pixelbytes;

    ifs.seekg(framesize * numFrames, ios::cur);
}

// TODO: only supports 4:2:0 chroma sampling
bool YUVInput::readPicture(x265_picture& pic)
{
    PPAStartCpuEventFunc(read_yuv);

    int pixelbytes = depth > 8 ? 2 : 1;

    int bufsize = (width * height * 3 / 2) * pixelbytes;

    if (!buf)
    {
        buf = new char[bufsize];
    }

    pic.planes[0] = buf;

    pic.planes[1] = (char*)(pic.planes[0]) + width * height * pixelbytes;

    pic.planes[2] = (char*)(pic.planes[1]) + ((width * height * pixelbytes) >> 2);

    pic.bitDepth = depth;

    pic.stride[0] = width * pixelbytes;

    pic.stride[1] = pic.stride[2] = pic.stride[0] >> 1;

    ifs.read(buf, bufsize);
    PPAStopCpuEventFunc(read_yuv);

    return ifs.good();
}
