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

#include "PPA/ppa.h"
#include "output.h"
#include "yuv.h"

using namespace x265;
using namespace std;

YUVOutput::YUVOutput(const char *filename, int w, int h, int d)
    : width(w)
    , height(h)
    , depth(d)
{
    ofs.open(filename, ios::binary | ios::out);
    buf = new char[width];
}

YUVOutput::~YUVOutput()
{
    ofs.close();
    if (buf) delete [] buf;
}

bool YUVOutput::writePicture(const x265_picture_t& pic)
{
    PPAStartCpuEventFunc(write_yuv);
    int pixelbytes = (depth > 8) ? 2 : 1;

    if (pic.bitDepth > 8 && depth == 8)
    {
        // encoder gave us short pixels, downscale, then write
        short *Y = (short*)pic.planes[0];
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                buf[j] = (char)Y[j];
            }

            ofs.write(buf, width);
            Y += pic.stride[0];
        }

        short *U = (short*)pic.planes[1];
        for (int i = 0; i < height >> 1; i++)
        {
            for (int j = 0; j < width >> 1; j++)
            {
                buf[j] = (char)U[j];
            }

            ofs.write(buf, width >> 1);
            U += pic.stride[1];
        }

        short *V = (short*)pic.planes[2];
        for (int i = 0; i < height >> 1; i++)
        {
            for (int j = 0; j < width >> 1; j++)
            {
                buf[j] = (char)V[j];
            }

            ofs.write(buf, width >> 1);
            V += pic.stride[2];
        }
    }
    else
    {
        // encoder pixels same size as output pixels, write them directly
        char *Y = (char*)pic.planes[0];
        for (int i = 0; i < height; i++)
        {
            ofs.write(Y, width * pixelbytes);
            Y += pic.stride[0] * pixelbytes;
        }

        char *U = (char*)pic.planes[1];
        for (int i = 0; i < height >> 1; i++)
        {
            ofs.write(U, (width >> 1) * pixelbytes);
            U += pic.stride[1] * pixelbytes;
        }

        char *V = (char*)pic.planes[2];
        for (int i = 0; i < height >> 1; i++)
        {
            ofs.write(V, (width >> 1) * pixelbytes);
            V += pic.stride[2] * pixelbytes;
        }
    }

    PPAStopCpuEventFunc(write_yuv);
    return true;
}
