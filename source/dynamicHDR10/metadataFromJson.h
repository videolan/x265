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

#ifndef METADATAFROMJSON_H
#define METADATAFROMJSON_H

#include<stdint.h>
#include<cstring>
#include "JsonHelper.h"

class metadataFromJson
{

public:
    metadataFromJson();
    ~metadataFromJson();

	enum JsonType{
		LEGACY,
		LLC
	};
		

    /**
     * @brief frameMetadataFromJson: Generates a sigle frame metadata array from Json file with all
     *          metadata information from movie.
     * @param filePath: path to Json file containing movie metadata information.
     * @param frame: frame Id number in respect to the movie.
     * @param metadata (output): receives an empty pointer that will be set to point to metadata
     *          array. Note: if pointer is set it will be deleted.
     * @return True if succesful
     */
    bool frameMetadataFromJson(const char* filePath,
                                int frame,
                                uint8_t *&metadata);
	
    /**
     * @brief movieMetadataFromJson: Generates metadata array from Json file with all metadata
     *          information from movie.
     * @param filePath: path to Json file containing movie metadata information.
     * @param metadata (output): receives an empty pointer that will be set to point to metadata
     *          array. Note: if pointer is set it will be deleted.
     * @return int: number of frames in the movie, -1 if the process fails to obtain the metadata.
     */
    int movieMetadataFromJson(const char* filePath,
                                uint8_t **&metadata);

    /**
    * @brief extendedInfoFrameMetadataFromJson: Generates Extended InfoFrame metadata array from Json file
    *           with all metadata information from movie.
    * @param filePath: path to Json file containing movie metadata information.
    * @param metadata (output): receives an empty pointer that will be set to point to metadata
    *          array. Note: if pointer is set it will be deleted.
    * @return int: number of frames in the movie, -1 if the process fails to obtain the metadata.
    */
    bool extendedInfoFrameMetadataFromJson(const char* filePath,
                                            int frame,
                                            uint8_t *&metadata);

    /**
    * @brief movieMetadataFromJson: Generates Extended InfoFrame metadata array from Json file with all metadata
    *          information from movie.
    * @param filePath: path to Json file containing movie Extended InfoFrame metadata information.
    * @param metadata (output): receives an empty pointer that will be set to point to metadata
    *          array. Note: if pointer is set it will be deleted.
    * @return int: number of frames in the movie, -1 if the process fails to obtain the metadata.
    */
    int movieExtendedInfoFrameMetadataFromJson(const char* filePath,
        uint8_t **&metadata);

    /**

    * @brief clear: Clears the memory of the given array and size.
    * @param metadata: metadata array to be cleared.
    * @param numberOfFrames: number of frames in the metadata array.
    * @return
    */
    void clear(uint8_t **&metadata,
                const int numberOfFrames);

private:

    class DynamicMetaIO;
    DynamicMetaIO *mPimpl;
    void fillMetadataArray(const JsonArray &fileData, int frame, const JsonType jsonType, uint8_t *&metadata);
};

#endif // METADATAFROMJSON_H
