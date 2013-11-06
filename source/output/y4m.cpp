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

#include "common.h"
#include "PPA/ppa.h"
#include "common.h"
#include "output.h"
#include "y4m.h"

using namespace x265;
using namespace std;

Y4MOutput::Y4MOutput(const char *filename, int w, int h, int rate, int csp)
    : width(w)
    , height(h)
    , colorSpace(csp)
    , frameSize(0)
{
    ofs.open(filename, ios::binary | ios::out);
    buf = new char[width];

    const char *cf = (csp >= X265_CSP_I444) ? "444" : (csp >= X265_CSP_I422) ? "422" : "420";

    if (ofs)
    {
        ofs << "YUV4MPEG2 W" << width << " H" << height << " F" << rate << ":1 Ip" << " C" << cf << "\n";
        header = ofs.tellp();
    }

    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
    {
        frameSize += (uint32_t)((width >> x265_cli_csps[colorSpace].width[i]) * (height >> x265_cli_csps[colorSpace].height[i]));
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
    std::ofstream::pos_type outPicPos = header;
    outPicPos += pic.poc * (6 + frameSize);
    ofs.seekp(outPicPos);
    ofs << "FRAME\n";

#if HIGH_BIT_DEPTH    
    // encoder gave us short pixels, downshift, then write
    int shift = pic.bitDepth - 8;
    if (pic.poc == 0 && shift)
    {
        x265_log(NULL, X265_LOG_WARNING, "y4m: down-shifting reconstructed pixels to 8 bits\n");
    }
    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
    {
        uint16_t *src = (uint16_t*)pic.planes[i];
        for (int h = 0; h < height >> x265_cli_csps[colorSpace].height[i]; h++)
        {
            for (int w = 0; w < width >> x265_cli_csps[colorSpace].width[i]; w++)
            {
                buf[w] = (char)(src[w] >> shift);
            }

            ofs.write(buf, width >> x265_cli_csps[colorSpace].width[i]);
            src += pic.stride[i];
        }
    }
#else    
    for (int i = 0; i < x265_cli_csps[colorSpace].planes; i++)
    {
        char *src = (char*)pic.planes[i];
        for (int h = 0; h < height >> x265_cli_csps[colorSpace].height[i]; h++)
        {
            ofs.write(src, width >> x265_cli_csps[colorSpace].width[i]);
            src += pic.stride[i];
        }
    }
#endif

    PPAStopCpuEventFunc(write_yuv);
    return true;
}
