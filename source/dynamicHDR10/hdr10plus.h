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

#include <stdint.h>

#ifndef HDR10PLUS_H
#define HDR10PLUS_H


/* hdr10plus_json_to_frame_cim:
 *      Parses the json file containing the  Creative Intent Metadata DTM for a frame of 
 *      video into a bytestream to be encoded into the resulting video stream. 
 *      path is the file path of the JSON file containing the DTM for the video.
 *      frameNumber is the number of the frame to get the metadata for.
 *      cim will get filled with the byte array containing the actual metadata.
 *      Returns true in case of success.
 */
bool hdr10plus_json_to_frame_cim(const char* path, uint32_t frameNumber, uint8_t *&cim);

/* hdr10plus_json_to_movie_cim:
 *      Parses the json file containing the Creative Intent Metadata DTM for the video 
 *      into a bytestream to be encoded into the resulting video stream. 
 *      path is the file path of the JSON file containing the DTM for the video.
 *      frameNumber is the number of the frame to get the metadata for.
 *      cim will get filled with the byte array containing the actual metadata.
 *      return int: number of frames in the movie, -1 if the process fails to obtain the metadata. 
 */
int hdr10plus_json_to_movie_cim(const char* path, uint8_t **&cim);

/* hdr10plus_json_to_frame_eif:
*      Parses the json file containing the Extended InfoFrame metadata for a frame of video
*      into a bytestream to be encoded into the resulting video stream.
*      path is the file path of the JSON file containing the Extended InfoFrame metadata for the video.
*      frameNumber is the number of the frame to get the metadata for.
*      Extended InfoFrame will get filled with the byte array containing the actual metadata.
*      Returns true in case of success.
*/
bool hdr10plus_json_to_frame_eif(const char* path, uint32_t frameNumber, uint8_t *&eif);

/* hdr10plus_json_to_movie_eif:
*      Parses the json file containing the Extended InfoFrame metadata for the video
*      into a bytestream to be encoded into the resulting video stream.
*      path is the file path of the JSON file containing the Extended InfoFrame metadata for the video.
*      frameNumber is the number of the frame to get the metadata for.
*      cim will get filled with the byte array containing the actual metadata.
*      return int: number of frames in the movie, -1 if the process fails to obtain the metadata.
*/
int hdr10plus_json_to_movie_eif(const char* path, uint8_t **&eif);


/* hdr10plus_clear_movie_cim:
*       This function clears the allocated memory for the movie metadata array
*       clear: Clears the memory of the given array and size.
*       metadata: metadata array to be cleared.
*       numberOfFrames: number of frames in the metadata array.
* @return
*/

void hdr10plus_clear_movie(uint8_t**& metadata, const int numberOfFrames);


typedef struct hdr10plus_api
{
    /* hdr10plus public API functions, documented above with hdr10plus_ prefixes */
    bool          (*hdr10plus_json_to_frame_cim)(const char *, uint32_t, uint8_t *&);
    int           (*hdr10plus_json_to_movie_cim)(const char *, uint8_t **&);
    bool          (*hdr10plus_json_to_frame_eif)(const char *, uint32_t, uint8_t *&);
    int           (*hdr10plus_json_to_movie_eif)(const char *, uint8_t **&);
    void          (*hdr10plus_clear_movie)(uint8_t **&, const int);
} hdr10plus_api;

/* hdr10plus_api:
 *   Retrieve the programming interface for the linked hdr10plus library.
 */
const hdr10plus_api* hdr10plus_api_get();


#endif // HDR10PLUS_H

