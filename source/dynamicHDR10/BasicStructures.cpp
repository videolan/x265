/**
 * @file                       BasicStructures.cpp
 * @brief                      Defines the structure of metadata parameters
 * @author                     Daniel Maximiliano Valenzuela, Seongnam Oh.
 * @create date                03/01/2017
 * @version                    0.0.1
 *
 * Copyright @ 2017 Samsung Electronics, DMS Lab, Samsung Research America and Samsung Research Tijuana
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
**/

#include "BasicStructures.h"
#include "vector"

struct PercentileLuminance{

    float averageLuminance = 0.0;
    float maxRLuminance = 0.0;
    float maxGLuminance = 0.0;
    float maxBLuminance = 0.0;
    int order;
    std::vector<unsigned int> percentiles;
};



