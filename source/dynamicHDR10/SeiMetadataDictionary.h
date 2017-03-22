/**
 * @file                       SeiMetadataDictionary.h
 * @brief                      Defines the tagname for each metadata value in a JSON dynamic tone mapping file.
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

#ifndef SEIMETADATADICTIONARY_H
#define SEIMETADATADICTIONARY_H

#include "SeiMetadataDictionary.h"

#include <string>

namespace SeiMetadataDictionary
{

    class JsonDataKeys
    {
        public:
        static const std::string LocalParameters;
        static const std::string TargetDisplayLuminance;
    };

    //Bezier Curve Data
    class BezierCurveNames
    {
        public:
        static const std::string TagName;
        static const std::string NumberOfAnchors;
        static const std::string KneePointX;
        static const std::string KneePointY;
        static const std::string Anchors[14];
    };
    //Ellipse Selection Data
    class EllipseSelectionNames
    {
        public:
        static const std::string WindowUpperLeftCornerX;
        static const std::string WindowUpperLeftCornerY;
        static const std::string WindowLowerRightCornerX;
        static const std::string WindowLowerRightCornerY;
        static const std::string WindowData;
    };
    //Ellipse Data
    class EllipseNames
    {
        public:
        static const std::string TagName;
        static const std::string RotationAngle;
        static const std::string CenterOfEllipseX;
        static const std::string CenterOfEllipseY;
        static const std::string OverlapProcessOption;
        static const std::string SemiMajorAxisExternalEllipse;
        static const std::string SemiMinorAxisExternalEllipse;
        static const std::string SemiMajorAxisInternalEllipse;
    };
    //Percentile Luminance
    class PercentileNames
    {
        public:
        static const std::string TagName;
        static const std::string NumberOfPercentiles;
        static const std::string PercentilePercentageValue[15];
        static const std::string PercentileLuminanceValue[15];
    };
    //Luminance Parameters
    class LuminanceNames
    {
        public:
        static const std::string TagName;
        static const std::string AverageRGB;
        static const std::string MaxSCL0;
        static const std::string MaxSCL1;
        static const std::string MaxSCL2;
    };
}

#endif // SEIMETADATADICTIONARY_H
