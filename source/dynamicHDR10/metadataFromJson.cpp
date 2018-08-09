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

#include "metadataFromJson.h"

#include <fstream>
#include <iostream>
#include <math.h>
#include "sstream"
#include "sys/stat.h"

#include "BasicStructures.h"
#include "SeiMetadataDictionary.h"
using namespace SeiMetadataDictionary;
class metadataFromJson::DynamicMetaIO
{
public:
    DynamicMetaIO() :
        mCurrentStreamBit(8),
        mCurrentStreamByte(0)
    {}

    ~DynamicMetaIO(){}

    int mCurrentStreamBit;
    int mCurrentStreamByte;

    bool luminanceParamFromJson(const Json &data, LuminanceParameters &obj, const JsonType jsonType)
    {
        JsonObject lumJsonData = data.object_items();
        if(!lumJsonData.empty())
        {
			switch(jsonType)
			{
				case LEGACY:
				{
					obj.averageLuminance = static_cast<float>(lumJsonData[LuminanceNames::AverageRGB].number_value());
					obj.maxRLuminance = static_cast<float>(lumJsonData[LuminanceNames::MaxSCL0].number_value());
					obj.maxGLuminance = static_cast<float>(lumJsonData[LuminanceNames::MaxSCL1].number_value());
					obj.maxBLuminance = static_cast<float>(lumJsonData[LuminanceNames::MaxSCL2].number_value());

					JsonObject percentileData = lumJsonData[PercentileNames::TagName].object_items();
					obj.order = percentileData[PercentileNames::NumberOfPercentiles].int_value();
					if(!percentileData.empty())
					{
						obj.percentiles.resize(obj.order);
						for(int i = 0; i < obj.order; ++i)
						{
							std::string percentileTag = PercentileNames::TagName;
							percentileTag += std::to_string(i);
							obj.percentiles[i] = static_cast<unsigned int>(percentileData[percentileTag].int_value());
						}
					}
					return true;
				} break;
				case LLC:
				{
					obj.averageLuminance = static_cast<float>(lumJsonData[LuminanceNames::AverageRGB].number_value());
					JsonArray maxScl = lumJsonData[LuminanceNames::MaxSCL].array_items();
					obj.maxRLuminance = static_cast<float>(maxScl[0].number_value());
					obj.maxGLuminance = static_cast<float>(maxScl[1].number_value());
					obj.maxBLuminance = static_cast<float>(maxScl[2].number_value());

					JsonObject percentileData = lumJsonData[LuminanceNames::LlcTagName].object_items();
					if(!percentileData.empty())
					{
						JsonArray distributionValues = percentileData[PercentileNames::DistributionValues].array_items();
						obj.order = static_cast<int>(distributionValues.size());
						obj.percentiles.resize(obj.order);
						for(int i = 0; i < obj.order; ++i)
						{
							obj.percentiles[i] = static_cast<unsigned int>(distributionValues[i].int_value());
						}
					}
					return true;
				} break;
			}
		}
        return false;
    }

    bool percentagesFromJson(const Json &data, std::vector<unsigned int> &percentages, const JsonType jsonType)
    {
        JsonObject jsonData = data.object_items();
        if(!jsonData.empty())
        {
			switch(jsonType)
			{
				case LEGACY:
				{
					JsonObject percentileData = jsonData[PercentileNames::TagName].object_items();
					int order = percentileData[PercentileNames::NumberOfPercentiles].int_value();
					percentages.resize(order);
					for(int i = 0; i < order; ++i)
					{
						std::string percentileTag = PercentileNames::PercentilePercentageValue[i];
						percentages[i] = static_cast<unsigned int>(percentileData[percentileTag].int_value());
					}
					return true;
				} break;
				case LLC:
				{
					JsonObject percentileData = jsonData[LuminanceNames::LlcTagName].object_items();
					if(!percentileData.empty())
					{
						JsonArray percentageValues = percentileData[PercentileNames::DistributionIndex].array_items();
						int order = static_cast<int>(percentageValues.size());
						percentages.resize(order);
						for(int i = 0; i < order; ++i)
						{
							percentages[i] = static_cast<unsigned int>(percentageValues[i].int_value());
						}
					} 
					return true;
				} break;
			}

        }
        return false;
    }

    bool bezierCurveFromJson(const Json &data, BezierCurveData &obj, const JsonType jsonType)
    {
        JsonObject jsonData = data.object_items();
        if(!jsonData.empty())
        {
			switch(jsonType)
			{
				case LEGACY:
				{
				    obj.sPx = jsonData[BezierCurveNames::KneePointX].int_value();
					obj.sPy = jsonData[BezierCurveNames::KneePointY].int_value();
					obj.order = jsonData[BezierCurveNames::NumberOfAnchors].int_value();
					obj.coeff.resize(obj.order);
					for(int i = 0; i < obj.order; ++i)
					{
						obj.coeff[i] = jsonData[BezierCurveNames::Anchors[i]].int_value();
					}
					return true;	
				} break;
				case LLC:
				{
					obj.sPx = jsonData[BezierCurveNames::KneePointX].int_value();
					obj.sPy = jsonData[BezierCurveNames::KneePointY].int_value();
					JsonArray anchorValues = data[BezierCurveNames::AnchorsTag].array_items();
					obj.order = static_cast<int>(anchorValues.size());
					obj.coeff.resize(obj.order);
					for(int i = 0; i < obj.order; ++i)
					{
						obj.coeff[i] = anchorValues[i].int_value();
					}
					return true;
				} break;
			}
        }
        return false;
    }

    template<typename T>
    void appendBits(uint8_t *dataStream, T data, int bitsToWrite)
    {
        //TODO: Check if bitsToWrite is <= sizeOf(T);
        while (bitsToWrite > 0)
        {
            /* if all data to write fits inside the current byte */
            if (bitsToWrite < mCurrentStreamBit )
            {
                int bitshift = mCurrentStreamBit - bitsToWrite;
                dataStream[mCurrentStreamByte] += static_cast<uint8_t>(data << bitshift);
                mCurrentStreamBit -= bitsToWrite;
                bitsToWrite = 0;
            }
            /* if all data to write needs more than the current byte space */
            else
            {
                int bitshift = bitsToWrite - mCurrentStreamBit;
                dataStream[mCurrentStreamByte] += static_cast<uint8_t>(data >> bitshift);
                bitsToWrite -= mCurrentStreamBit ;
                mCurrentStreamBit = 8;
                mCurrentStreamByte++;
            }
        }
    }

    void setPayloadSize(uint8_t *dataStream, int positionOnStream, int payload)
    {
        int payloadBytes = 1;
        for(;payload >= 0xFF; payload -= 0xFF, ++payloadBytes);
        if(payloadBytes > 1)
        {
            shiftData(dataStream, payloadBytes-1, mCurrentStreamByte, positionOnStream);
            mCurrentStreamByte += payloadBytes-1;

            for(int i = 0; i < payloadBytes; ++i)
            {
                if(payloadBytes-1 == i)
                {
                    dataStream[positionOnStream++] = static_cast<uint8_t>(payload);
                }
                else
                {
                    dataStream[positionOnStream++] = 0xFF;
                }
            }
        }
        else
        {
            dataStream[positionOnStream] = static_cast<uint8_t>(payload);
        }
    }

    void shiftData(uint8_t *dataStream, int shiftSize, int streamSize, int startPoint = 0)
    {
        for(int i = streamSize; i > startPoint; --i)
        {
            dataStream[i + shiftSize] = dataStream[i];
        }
    }

};

metadataFromJson::metadataFromJson() :
    mPimpl(new DynamicMetaIO())
{

}

metadataFromJson::~metadataFromJson()
{
    delete mPimpl;
}

bool metadataFromJson::frameMetadataFromJson(const char* filePath,
                                              int frame,
                                              uint8_t *&metadata)
{
    std::string path(filePath);
    JsonArray fileData = JsonHelper::readJsonArray(path);
	JsonType jsonType = LEGACY;
    if(fileData.empty())
    {
		jsonType = LLC;
        fileData = JsonHelper::readJson(filePath).at("SceneInfo").array_items();
    }

//    frame = frame + 1; //index on the array start at 0 frames starts at 1
    int numFrames = static_cast<int>(fileData.size());

    if(frame >= numFrames)
    {
        return false;
    }

    int mSEIBytesToRead = 509;
    if(metadata)
    {
        delete(metadata);
    }
    metadata = new uint8_t[mSEIBytesToRead];
    mPimpl->mCurrentStreamBit = 8;
    mPimpl->mCurrentStreamByte = 1;
    memset(metadata, 0, mSEIBytesToRead);

    fillMetadataArray(fileData, frame, jsonType, metadata);
    mPimpl->setPayloadSize(metadata, 0, mPimpl->mCurrentStreamByte);
    return true;
}

int metadataFromJson::movieMetadataFromJson(const char* filePath, uint8_t **&metadata)
{
    std::string path(filePath);
    JsonArray fileData = JsonHelper::readJsonArray(path);
	JsonType jsonType = LEGACY;
    if (fileData.empty())
    {
		jsonType = LLC;
        fileData = JsonHelper::readJson(filePath).at("SceneInfo").array_items();
    }

    int numFrames = static_cast<int>(fileData.size());
    metadata = new uint8_t*[numFrames];
    for (int frame = 0; frame < numFrames; ++frame)
    {
        metadata[frame] = new uint8_t[509];
        memset(metadata[frame], 0, 509);
        mPimpl->mCurrentStreamBit = 8;
        mPimpl->mCurrentStreamByte = 1;

        fillMetadataArray(fileData, frame, jsonType, metadata[frame]);
        mPimpl->setPayloadSize(metadata[frame], 0, mPimpl->mCurrentStreamByte);
    }

    return numFrames;
}

bool metadataFromJson::extendedInfoFrameMetadataFromJson(const char* filePath,
    int frame,
    uint8_t *&metadata)
{
    std::string path(filePath);
    JsonArray fileData = JsonHelper::readJsonArray(path);

    if (fileData.empty())
    {
        return false;
    }

    int numFrames = static_cast<int>(fileData.size());
    if (frame >= numFrames)
    {
        return false;
    }

    int mSEIBytesToRead = 509;

    if (metadata)
    {
        delete(metadata);
    }
    metadata = new uint8_t[mSEIBytesToRead];
    mPimpl->mCurrentStreamBit = 8;
    mPimpl->mCurrentStreamByte = 0;

    for (int j = 0; j < mSEIBytesToRead; ++j)
    {
        (metadata)[j] = 0;
    }

    const uint16_t extendedInfoframeType = 0x0004;
    mPimpl->appendBits(metadata, extendedInfoframeType, 16);

    /* NOTE: We leave TWO BYTES of space for the payload */
    mPimpl->mCurrentStreamByte += 2;

    fillMetadataArray(fileData, frame, LEGACY, metadata);

    /* Set payload in bytes 2 & 3 as indicated in Extended InfoFrame Type syntax */
    metadata[2] = (mPimpl->mCurrentStreamByte & 0xFF00) >> 8;
    metadata[3] = (mPimpl->mCurrentStreamByte & 0x00FF);
    return true;
}

int metadataFromJson::movieExtendedInfoFrameMetadataFromJson(const char* filePath, uint8_t **&metadata)
{
    std::string path(filePath);
    JsonArray fileData = JsonHelper::readJsonArray(path);
    if(fileData.empty())
    {
        return -1;
    }

    int numFrames = static_cast<int>(fileData.size());
    metadata = new uint8_t*[numFrames];
    for(int frame = 0; frame < numFrames; ++frame)
    {
        metadata[frame] = new uint8_t[509];
        for(int i = 0; i < 509; ++i) 
        {
            metadata[frame][i] = 0;
        }
        mPimpl->mCurrentStreamBit = 8;
        mPimpl->mCurrentStreamByte = 0;

        const uint16_t extendedInfoframeType = 0x0004;
        mPimpl->appendBits(metadata[frame], extendedInfoframeType, 16);

        /* NOTE: We leave TWO BYTES of space for the payload */
        mPimpl->mCurrentStreamByte += 2;

        fillMetadataArray(fileData, frame, LEGACY, metadata[frame]);

        /* Set payload in bytes 2 & 3 as indicated in Extended InfoFrame Type syntax */
        metadata[frame][2] = (mPimpl->mCurrentStreamByte & 0xFF00) >> 8;
        metadata[frame][3] = (mPimpl->mCurrentStreamByte & 0x00FF);
    }

    return numFrames;
}

void metadataFromJson::fillMetadataArray(const JsonArray &fileData, int frame, const JsonType jsonType, uint8_t *&metadata)
{
    const uint8_t countryCode = 0xB5;
    const uint16_t terminalProviderCode = 0x003C;
    const uint16_t terminalProviderOrientedCode = 0x0001;
    const uint8_t applicationIdentifier = 4;
    const uint8_t applicationVersion = 1;

    mPimpl->appendBits(metadata, countryCode, 8);
    mPimpl->appendBits(metadata, terminalProviderCode, 16);
    mPimpl->appendBits(metadata, terminalProviderOrientedCode, 16);

    mPimpl->appendBits(metadata, applicationIdentifier, 8);
    mPimpl->appendBits(metadata, applicationVersion, 8);

    uint16_t numWindows = 0;
    /* HDR10+ LLC doesn't consider local windows */
    if(jsonType & LLC)
    {
        numWindows = 1;
        mPimpl->appendBits(metadata, numWindows, 2);
    }
    else
    {
        //Note: Validated only add up to two local selections, ignore the rest
        JsonArray jsonArray = fileData[frame][JsonDataKeys::LocalParameters].array_items();
        int ellipsesNum = static_cast<int>(jsonArray.size() > 2 ? 2 : jsonArray.size());
        numWindows = (uint16_t)fileData[frame][JsonDataKeys::NumberOfWindows].int_value();
        mPimpl->appendBits(metadata, numWindows, 2);
        for (int i = 0; i < ellipsesNum; ++i)
        {
            mPimpl->appendBits(metadata, jsonArray[i][EllipseSelectionNames::WindowData]
                    [EllipseSelectionNames::WindowUpperLeftCornerX].int_value(), 16);
            mPimpl->appendBits(metadata, jsonArray[i][EllipseSelectionNames::WindowData]
                    [EllipseSelectionNames::WindowUpperLeftCornerY].int_value(), 16);
            mPimpl->appendBits(metadata, jsonArray[i][EllipseSelectionNames::WindowData]
                    [EllipseSelectionNames::WindowLowerRightCornerX].int_value(), 16);
            mPimpl->appendBits(metadata, jsonArray[i][EllipseSelectionNames::WindowData]
                    [EllipseSelectionNames::WindowLowerRightCornerY].int_value(), 16);

            JsonObject ellipseJsonObject = jsonArray[i][EllipseNames::TagName].object_items();

            mPimpl->appendBits(metadata,
                               static_cast<uint16_t>(ellipseJsonObject[EllipseNames::CenterOfEllipseX].int_value()),
                    16);

            mPimpl->appendBits(metadata,
                               static_cast<uint16_t>(ellipseJsonObject[EllipseNames::CenterOfEllipseY].int_value()),
                    16);

            int angle = ellipseJsonObject[EllipseNames::RotationAngle].int_value();
            uint8_t rotationAngle = static_cast<uint8_t>((angle > 180.0) ? angle - 180.0 : angle);
            mPimpl->appendBits(metadata, rotationAngle, 8);

            uint16_t semimajorExternalAxis =
                    static_cast<uint16_t>(ellipseJsonObject[EllipseNames::SemiMajorAxisExternalEllipse].int_value());

            uint16_t semiminorExternalAxis =
                    static_cast<uint16_t>(ellipseJsonObject[EllipseNames::SemiMinorAxisExternalEllipse].int_value());

            uint16_t semimajorInternalEllipse =
                    static_cast<uint16_t>(ellipseJsonObject[EllipseNames::SemiMajorAxisInternalEllipse].int_value());

            mPimpl->appendBits(metadata, semimajorInternalEllipse, 16);

            mPimpl->appendBits(metadata, semimajorExternalAxis, 16);
            mPimpl->appendBits(metadata, semiminorExternalAxis, 16);
            uint8_t overlapProcessOption = static_cast<uint8_t>(ellipseJsonObject[EllipseNames::OverlapProcessOption].int_value());
            //TODO: Uses Layering method, the value is "1"
            mPimpl->appendBits(metadata, overlapProcessOption, 1);
        }
    }

    /* Targeted System Display Data */
    uint32_t monitorPeak = fileData[frame][JsonDataKeys::TargetDisplayLuminance].int_value();
    mPimpl->appendBits(metadata, monitorPeak, 27);

    uint8_t targetedSystemDisplayActualPeakLuminanceFlag = 0;
    mPimpl->appendBits(metadata, targetedSystemDisplayActualPeakLuminanceFlag, 1);
    if (targetedSystemDisplayActualPeakLuminanceFlag)
    {
        //TODO
    }

    /* Max RGB values (maxScl)*/
    /* Luminance values/percentile for each window */
    for (int w = 0; w < numWindows; ++w)
    {
        Json lumObj = fileData[frame][LuminanceNames::TagName];
        LuminanceParameters luminanceData;
        if(!mPimpl->luminanceParamFromJson(lumObj, luminanceData, jsonType))
        {
            std::cout << "error parsing luminance parameters frame: " << w << std::endl;
        }

        /* NOTE: Maxscl from 0 to 100,000 based on data that says in values of 0.00001
        * one for each channel R,G,B
        */
        mPimpl->appendBits(metadata, static_cast<uint8_t>(((int)luminanceData.maxRLuminance & 0x10000) >> 16), 1);
        mPimpl->appendBits(metadata, static_cast<uint16_t>((int)luminanceData.maxRLuminance & 0xFFFF), 16);
        mPimpl->appendBits(metadata, static_cast<uint8_t>(((int)luminanceData.maxGLuminance & 0x10000) >> 16), 1);
        mPimpl->appendBits(metadata, static_cast<uint16_t>((int)luminanceData.maxGLuminance & 0xFFFF), 16);
        mPimpl->appendBits(metadata, static_cast<uint8_t>(((int)luminanceData.maxBLuminance & 0x10000) >> 16), 1);
        mPimpl->appendBits(metadata, static_cast<uint16_t>((int)luminanceData.maxBLuminance & 0xFFFF), 16);
        mPimpl->appendBits(metadata, static_cast<uint8_t>(((int)luminanceData.averageLuminance & 0x10000) >> 16), 1);
        mPimpl->appendBits(metadata, static_cast<uint16_t>((int)luminanceData.averageLuminance & 0xFFFF), 16);

        /* Percentiles */
        uint8_t numDistributionMaxrgbPercentiles = static_cast<uint8_t>(luminanceData.order);
        mPimpl->appendBits(metadata, numDistributionMaxrgbPercentiles, 4);

        std::vector<unsigned int>percentilePercentages;
        mPimpl->percentagesFromJson(lumObj, percentilePercentages, jsonType);

        for (int i = 0; i < numDistributionMaxrgbPercentiles; ++i)
        {
            uint8_t distributionMaxrgbPercentage = static_cast<uint8_t>(percentilePercentages.at(i));
            mPimpl->appendBits(metadata, distributionMaxrgbPercentage, 7);

            /* 17bits: 1bit then 16 */
            unsigned int ithPercentile = luminanceData.percentiles.at(i);
            uint8_t highValue = static_cast<uint8_t>((ithPercentile & 0x10000) >> 16);
            uint16_t lowValue = static_cast<uint16_t>(ithPercentile & 0xFFFF);
            mPimpl->appendBits(metadata, highValue, 1);
            mPimpl->appendBits(metadata, lowValue, 16);
        }

        /* 10bits: Fraction bright pixels */
        uint16_t fractionBrightPixels = 0;
        mPimpl->appendBits(metadata, fractionBrightPixels, 10);

    }

    /* Note: Set to false by now as requested  */
    uint8_t masteringDisplayActualPeakLuminanceFlag = 0;
    mPimpl->appendBits(metadata, masteringDisplayActualPeakLuminanceFlag, 1);
    if (masteringDisplayActualPeakLuminanceFlag)
    {
        //TODO
    }
    /* Bezier Curve Data */
    for (int w = 0; w < numWindows; ++w)
    {
        uint8_t toneMappingFlag = 0;
		/* Check if the window contains tone mapping bezier curve data and set toneMappingFlag appropriately */
        BezierCurveData curveData;
		/* Select curve data based on global window */
        if (w == 0)
        {			
            if (mPimpl->bezierCurveFromJson(fileData[frame][BezierCurveNames::TagName], curveData, jsonType))
            {
                toneMappingFlag = 1;
            }
        }
        /* Select curve data based on local window */
        else
        {
            JsonArray jsonArray = fileData[frame][JsonDataKeys::LocalParameters].array_items();
            if (mPimpl->bezierCurveFromJson(jsonArray[w - 1][BezierCurveNames::TagName], curveData, jsonType))
            {
                toneMappingFlag = 1;
            }
        }		
        mPimpl->appendBits(metadata, toneMappingFlag, 1);
        if (toneMappingFlag)
        {
            uint16_t kneePointX = static_cast<uint16_t>(curveData.sPx);
            mPimpl->appendBits(metadata, kneePointX, 12);
            uint16_t kneePointY = static_cast<uint16_t>(curveData.sPy);
            mPimpl->appendBits(metadata, kneePointY, 12);

            uint8_t numBezierCurveAnchors = static_cast<uint8_t>(curveData.order);// - 1;
            mPimpl->appendBits(metadata, numBezierCurveAnchors, 4);

            /* Curve anchors 10bits */
            for (int i = 0; i < numBezierCurveAnchors; ++i)
            {
                uint16_t anchor = static_cast<uint16_t>(curveData.coeff.at(i));
                mPimpl->appendBits(metadata, anchor, 10);
            }
        }
	}
    /* Set to false as requested */
    bool colorSaturationMappingFlag = 0;
    mPimpl->appendBits(metadata, colorSaturationMappingFlag, 1);
    if (colorSaturationMappingFlag)
    {
        //TODO
    }

    if (mPimpl->mCurrentStreamBit == 8)
    {
        mPimpl->mCurrentStreamByte -= 1;
    }
}

void metadataFromJson::clear(uint8_t **&metadata, const int numberOfFrames)
{
    if (metadata && numberOfFrames > 0)
    {
        for (int i = 0; i < numberOfFrames; ++i)
        {
            if (metadata[i])
            {
                delete[] metadata[i];
            }
        }
        delete[] metadata;
        metadata = NULL;
    }
}
