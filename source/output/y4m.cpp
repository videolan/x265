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
#include "common.h"
#include "output.h"
#include "y4m.h"

using namespace x265;
using namespace std;

Y4MOutput::Y4MOutput(const char *filename, int w, int h, int rate)
    : width(w)
    , height(h)
{
    ofs.open(filename, ios::binary | ios::out);
    buf = new char[width];
    if (ofs)
    {
        ofs << "YUV4MPEG2 W" << width << " H" << height << " F" << rate << ":1 Ip C420\n";
        header = ofs.tellp();
    }
}

Y4MOutput::~Y4MOutput()
{
    ofs.close();
    delete [] buf;
}

bool Y4MOutput::writePicture(const x265_picture& pic)
{
    PPAStartCpuEventFunc(write_yuv);
    std::ofstream::pos_type frameSize = (6 + 3 * (width * height) / 2);
    ofs.seekp(header + frameSize * pic.poc);
    ofs << "FRAME\n";

    if (pic.bitDepth > 8)
    {
        if (pic.poc == 0)
        {
            x265_log(NULL, X265_LOG_WARNING, "y4m: down-shifting reconstructed pixels to 8 bits\n");
        }
        // encoder gave us short pixels, downshift, then write
        uint16_t *Y = (uint16_t*)pic.planes[0];
        int shift = pic.bitDepth - 8;
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                buf[j] = (char)(Y[j] >> shift);
            }

            ofs.write(buf, width);
            Y += pic.stride[0];
        }

        uint16_t *U = (uint16_t*)pic.planes[1];
        for (int i = 0; i < height >> 1; i++)
        {
            for (int j = 0; j < width >> 1; j++)
            {
                buf[j] = (char)(U[j] >> shift);
            }

            ofs.write(buf, width >> 1);
            U += pic.stride[1];
        }

        uint16_t *V = (uint16_t*)pic.planes[2];
        for (int i = 0; i < height >> 1; i++)
        {
            for (int j = 0; j < width >> 1; j++)
            {
                buf[j] = (char)(V[j] >> shift);
            }

            ofs.write(buf, width >> 1);
            V += pic.stride[2];
        }
    }
    else
    {
        char *Y = (char*)pic.planes[0];
        for (int i = 0; i < height; i++)
        {
            ofs.write(Y, width);
            Y += pic.stride[0];
        }

        char *U = (char*)pic.planes[1];
        for (int i = 0; i < height >> 1; i++)
        {
            ofs.write(U, width >> 1);
            U += pic.stride[1];
        }

        char *V = (char*)pic.planes[2];
        for (int i = 0; i < height >> 1; i++)
        {
            ofs.write(V, width >> 1);
            V += pic.stride[2];
        }
    }

    PPAStopCpuEventFunc(write_yuv);
    return true;
}
