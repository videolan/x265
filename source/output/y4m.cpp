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

#include "output.h"
#include "y4m.h"
#include <stdio.h>
#include <assert.h>

using namespace x265;

Y4MOutput::Y4MOutput(const char *filename, int t_width, int t_height, int bitdepth)
{
    fp = fopen(filename, "wb");
    width = t_width;
    height = t_height;
    assert(bitdepth == 8);
    buf = new char[width];
    if (fp)
    {
        // TODO: need to get frame rate
        fprintf(fp, "YUV4MPEG2 W%d H%d F30:1 Ip C420\n", width, height);
    }
}

Y4MOutput::~Y4MOutput()
{
    if (fp) fclose(fp);
    if (buf) delete [] buf;
}


bool Y4MOutput::writePicture(const x265_picture& pic)
{
    fprintf(fp, "FRAME\n");

    if (pic.bitDepth > 8)
    {
        // encoder gave us short pixels, downscale, then write
        short *Y = (short*)pic.planes[0];
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
                buf[j] = (char) Y[j];
            fwrite(buf, sizeof(char), width, fp);
            Y += pic.stride[0];
        }
        short *U = (short*)pic.planes[1];
        for (int i = 0; i < height >> 1; i++)
        {
            for (int j = 0; j < width >> 1; j++)
                buf[j] = (char) U[j];
            fwrite(buf, sizeof(char), width >> 1, fp);
            U += pic.stride[1];
        }
        short *V = (short*)pic.planes[2];
        for (int i = 0; i < height >> 1; i++)
        {
            for (int j = 0; j < width >> 1; j++)
                buf[j] = (char) V[j];
            fwrite(buf, sizeof(char), width >> 1, fp);
            V += pic.stride[2];
        }
    }
    else
    {
        char *Y = (char*)pic.planes[0];
        for (int i = 0; i < height; i++)
        {
            fwrite(Y, sizeof(char), width, fp);
            Y += pic.stride[0];
        }
        char *U = (char*)pic.planes[1];
        for (int i = 0; i < height >> 1; i++)
        {
            fwrite(U, sizeof(char), width >> 1, fp);
            U += pic.stride[1];
        }
        char *V = (char*)pic.planes[2];
        for (int i = 0; i < height >> 1; i++)
        {
            fwrite(V, sizeof(char), width >> 1, fp);
            V += pic.stride[2];
        }
    }

    return true;
}
