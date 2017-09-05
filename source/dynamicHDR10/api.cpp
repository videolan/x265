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

#include "hdr10plus.h"
#include "metadataFromJson.h"

bool hdr10plus_json_to_frame_cim(const char* path, uint32_t frameNumber, uint8_t *&cim)
{
      metadataFromJson meta;
      return meta.frameMetadataFromJson(path,
                                        frameNumber,
                                        cim);
}

int hdr10plus_json_to_movie_cim(const char* path, uint8_t **&cim)
{
      metadataFromJson meta;
      return meta.movieMetadataFromJson(path, cim);
}

bool hdr10plus_json_to_frame_eif(const char* path, uint32_t frameNumber, uint8_t *&eif)
{
    metadataFromJson meta;
    return meta.extendedInfoFrameMetadataFromJson(path,
                                                  frameNumber,
                                                  eif);
}

int hdr10plus_json_to_movie_eif(const char* path, uint8_t **&eif)
{
    metadataFromJson meta;
    return meta.movieExtendedInfoFrameMetadataFromJson(path, eif);
}


void hdr10plus_clear_movie(uint8_t **&metadata, const int numberOfFrames)
{
    if(metadata)
    {
        metadataFromJson meta;
        meta.clear(metadata, numberOfFrames);
    }
}

static const hdr10plus_api libapi =
{
    &hdr10plus_json_to_frame_cim,
    &hdr10plus_json_to_movie_cim,
    &hdr10plus_json_to_frame_eif,
    &hdr10plus_json_to_movie_eif,
    &hdr10plus_clear_movie,
};

const hdr10plus_api* hdr10plus_api_get()
{
    return &libapi;
}
