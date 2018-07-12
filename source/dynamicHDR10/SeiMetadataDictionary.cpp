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

#include "SeiMetadataDictionary.h"

using namespace SeiMetadataDictionary;

const std::string JsonDataKeys::LocalParameters = std::string("LocalParameters");
const std::string JsonDataKeys::TargetDisplayLuminance = std::string("TargetedSystemDisplayMaximumLuminance");
const std::string JsonDataKeys::NumberOfWindows = std::string("NumberOfWindows");

const std::string BezierCurveNames::TagName = std::string("BezierCurveData");
const std::string BezierCurveNames::NumberOfAnchors = std::string("NumberOfAnchors");
const std::string BezierCurveNames::KneePointX = std::string("KneePointX");
const std::string BezierCurveNames::KneePointY = std::string("KneePointY");
const std::string BezierCurveNames::AnchorsTag = std::string("Anchors");
const std::string BezierCurveNames::Anchors[] = {std::string("Anchor0"),
                                                 std::string("Anchor1"),
                                                 std::string("Anchor2"),
                                                 std::string("Anchor3"),
                                                 std::string("Anchor4"),
                                                 std::string("Anchor5"),
                                                 std::string("Anchor6"),
                                                 std::string("Anchor7"),
                                                 std::string("Anchor8"),
                                                 std::string("Anchor9"),
                                                 std::string("Anchor10"),
                                                 std::string("Anchor11"),
                                                 std::string("Anchor12"),
                                                 std::string("Anchor13")};


const std::string EllipseSelectionNames::WindowUpperLeftCornerX = std::string("WindowUpperLeftCornerX");
const std::string EllipseSelectionNames::WindowUpperLeftCornerY = std::string("WindowUpperLeftCornerY");
const std::string EllipseSelectionNames::WindowLowerRightCornerX = std::string("WindowLowerRightCornerX");
const std::string EllipseSelectionNames::WindowLowerRightCornerY = std::string("WindowLowerRightCornerY");
const std::string EllipseSelectionNames::WindowData = std::string("WindowData");


const std::string EllipseNames::TagName = std::string("EllipseData");
const std::string EllipseNames::RotationAngle = std::string("RotationAngle");
const std::string EllipseNames::CenterOfEllipseX = std::string("CenterOfEllipseX");
const std::string EllipseNames::CenterOfEllipseY = std::string("CenterOfEllipseY");
const std::string EllipseNames::OverlapProcessOption = std::string("OverlapProcessOption");
const std::string EllipseNames::SemiMajorAxisExternalEllipse = std::string("SemimajorAxisExternalEllipse");
const std::string EllipseNames::SemiMinorAxisExternalEllipse = std::string("SemiminorAxisExternalEllipse");
const std::string EllipseNames::SemiMajorAxisInternalEllipse = std::string("SemimajorAxisInternalEllipse");


const std::string PercentileNames::TagName = std::string("PercentileLuminance");
const std::string PercentileNames::NumberOfPercentiles = std::string("NumberOfPercentiles");
const std::string PercentileNames::DistributionIndex = std::string("DistributionIndex");
const std::string PercentileNames::DistributionValues = std::string("DistributionValues");
const std::string PercentileNames::PercentilePercentageValue[] = {std::string("PercentilePercentage0"),
                                                                  std::string("PercentilePercentage1"),
                                                                  std::string("PercentilePercentage2"),
                                                                  std::string("PercentilePercentage3"),
                                                                  std::string("PercentilePercentage4"),
                                                                  std::string("PercentilePercentage5"),
                                                                  std::string("PercentilePercentage6"),
                                                                  std::string("PercentilePercentage7"),
                                                                  std::string("PercentilePercentage8"),
                                                                  std::string("PercentilePercentage9"),
                                                                  std::string("PercentilePercentage10"),
                                                                  std::string("PercentilePercentage11"),
                                                                  std::string("PercentilePercentage12"),
                                                                  std::string("PercentilePercentage13"),
                                                                  std::string("PercentilePercentage14")};

const std::string PercentileNames::PercentileLuminanceValue[] = {std::string("PercentileLuminance0"),
                                                                  std::string("PercentileLuminance1"),
                                                                  std::string("PercentileLuminance2"),
                                                                  std::string("PercentileLuminance3"),
                                                                  std::string("PercentileLuminance4"),
                                                                  std::string("PercentileLuminance5"),
                                                                  std::string("PercentileLuminance6"),
                                                                  std::string("PercentileLuminance7"),
                                                                  std::string("PercentileLuminance8"),
                                                                  std::string("PercentileLuminance9"),
                                                                  std::string("PercentileLuminance10"),
                                                                  std::string("PercentileLuminance11"),
                                                                  std::string("PercentileLuminance12"),
                                                                  std::string("PercentileLuminance13"),
                                                                  std::string("PercentileLuminance14")};



const std::string LuminanceNames::TagName = std::string("LuminanceParameters");
const std::string LuminanceNames::LlcTagName = std::string("LuminanceDistributions");
const std::string LuminanceNames::AverageRGB = std::string("AverageRGB");
const std::string LuminanceNames::MaxSCL = std::string("MaxScl");
const std::string LuminanceNames::MaxSCL0 = std::string("MaxScl0");
const std::string LuminanceNames::MaxSCL1 = std::string("MaxScl1");
const std::string LuminanceNames::MaxSCL2 = std::string("MaxScl2");



