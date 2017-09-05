/**
 * Copyright (C) 2013-2017 MulticoreWare, Inc
 *
 * Authors: Bhavna Hariharan <bhavna@multicorewareinc.com>
 *          Kavitha Sampath <kavitha@multicorewareinc.com>
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
**/

#ifndef BASICSTRUCTURES_H
#define BASICSTRUCTURES_H

#include <vector>


struct LuminanceParameters
{
    float averageLuminance = 0.0;
    float maxRLuminance = 0.0;
    float maxGLuminance = 0.0;
    float maxBLuminance = 0.0;
    int order = 0;
    std::vector<unsigned int> percentiles;
};

struct BezierCurveData
{
    int order = 0;
    int sPx = 0;
    int sPy = 0;
    std::vector<int> coeff;
};

struct PercentileLuminance{

    float averageLuminance = 0.0;
    float maxRLuminance = 0.0;
    float maxGLuminance = 0.0;
    float maxBLuminance = 0.0;
    int order = 0;
    std::vector<unsigned int> percentiles;
};

#endif // BASICSTRUCTURES_H
