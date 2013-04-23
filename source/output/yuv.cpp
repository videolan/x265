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
#include "yuv.h"

using namespace x265;

YUVOutput::YUVOutput(const char *filename, int t_width, int t_height, int t_bitdepth)
{
    fp = fopen(filename, "wb");
    width = t_width;
    height = t_height;
    depth = t_bitdepth;
}

YUVOutput::~YUVOutput()
{
    if (fp) fclose(fp);
}

bool YUVOutput::writePicture(const x265_picture& pic)
{
    char *Y = (char*)pic.planes[0];
    int pixelbytes = (depth > 8) ? 2 : 1;
    for (int i = 0; i < height; i++)
    {
        fwrite(Y, sizeof(char), width * pixelbytes, fp);
        Y += pic.stride[0];
    }
    char *U = (char*)pic.planes[1];
    for (int i = 0; i < height >> 1; i++)
    {
        fwrite(U, sizeof(char), (width>>1) * pixelbytes, fp);
        U += pic.stride[1];
    }
    char *V = (char*)pic.planes[2];
    for (int i = 0; i < height >> 1; i++)
    {
        fwrite(V, sizeof(uint8_t), (width>>1) * pixelbytes, fp);
        V += pic.stride[2];
    }
    return true;
}
