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

#include "input.h"
#include "yuv.h"

using namespace x265;

YUVInput::YUVInput(const char *filename)
{
    fp = fopen(filename, "rb");
    width = height = 0;
    depth = 8;
    buf = NULL;
}

YUVInput::~YUVInput()
{
    if (fp) fclose(fp);
    if (buf) delete[] buf;
}

int  YUVInput::guessFrameCount() const
{
    /* TODO: Get file size, divide by bufsize */
    return 0;
}

void YUVInput::skipFrames(int numFrames)
{
    int pixelbytes = depth > 8 ? 2 : 1;

    int framesize = (width * height * 3 / 2) * pixelbytes;

    fseek(fp, framesize * numFrames, SEEK_CUR);
}

// TODO: only supports 4:2:0 chroma sampling
bool YUVInput::readPicture(Picture& pic)
{
    int pixelbytes = depth > 8 ? 2 : 1;

    int bufsize = (width * height * 3 / 2) * pixelbytes;

    if (!buf)
    {
        buf = new uint8_t[bufsize];
    }

    pic.planes[0] = buf;

    pic.planes[1] = (char*)(pic.planes[0]) + width * height * pixelbytes;

    pic.planes[2] = (char*)(pic.planes[1]) + ((width * height * pixelbytes) >> 2);

    pic.bitDepth = depth;

    pic.stride[0] = width * pixelbytes;

    pic.stride[1] = pic.stride[2] = pic.stride[0] >> 1;

    return fread(buf, 1, bufsize, fp) == bufsize;
}
