/*****************************************************************************
* Copyright (C) 2013-2020 MulticoreWare, Inc
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
* For more information, contact us at license @ x265.com.
*****************************************************************************/

#ifndef X265_SEI_H
#define X265_SEI_H

#include "common.h"
#include "bitstream.h"
#include "slice.h"
#include "nal.h"
#include "md5.h"

namespace X265_NS {
// private namespace

class SEI : public SyntaxElementWriter
{
public:
    /* SEI users call writeSEImessages() to marshal an SEI to a bitstream.
    * The writeSEImessages() method calls writeSEI() which encodes the header */
    void writeSEImessages(Bitstream& bs, const SPS& sps, NalUnitType nalUnitType, NALList& list, int isNested);
    void setSize(uint32_t size);
    static char* base64Decode(char encodedString[], int base64EncodeLength);
    virtual ~SEI() {}
protected:
    SEIPayloadType  m_payloadType;
    uint32_t        m_payloadSize;
    virtual void writeSEI(const SPS&) = 0;
    void writeByteAlign();
};

//seongnam.oh@samsung.com :: for the Creative Intent Meta Data Encoding
class SEIuserDataRegistered : public SEI
{
public:
    SEIuserDataRegistered()
    {
        m_payloadType = USER_DATA_REGISTERED_ITU_T_T35;
        m_payloadSize = 0;
    }

    uint8_t *m_userData;

    // daniel.vt@samsung.com :: for the Creative Intent Meta Data Encoding ( seongnam.oh@samsung.com )
    void writeSEI(const SPS&)
    {
        if (!m_userData)
            return;

        uint32_t i = 0;
        for (; i < m_payloadSize; ++i)
            WRITE_CODE(m_userData[i], 8, "creative_intent_metadata");
    }
};

/* Film grain characteristics */
class FilmGrainCharacteristics : public SEI
{
  public:

    FilmGrainCharacteristics()
    {
        m_payloadType = FILM_GRAIN_CHARACTERISTICS;
        m_payloadSize = 0;
    }

    struct CompModelIntensityValues
    {
        uint8_t intensityIntervalLowerBound;
        uint8_t intensityIntervalUpperBound;
        int*    compModelValue;
    };

    struct CompModel
    {
        bool    bPresentFlag;
        uint8_t numModelValues;
        uint8_t m_filmGrainNumIntensityIntervalMinus1;
        CompModelIntensityValues* intensityValues;
    };

    CompModel   m_compModel[MAX_NUM_COMPONENT];
    bool        m_filmGrainCharacteristicsPersistenceFlag;
    bool        m_filmGrainCharacteristicsCancelFlag;
    bool        m_separateColourDescriptionPresentFlag;
    bool        m_filmGrainFullRangeFlag;
    uint8_t     m_filmGrainModelId;
    uint8_t     m_blendingModeId;
    uint8_t     m_log2ScaleFactor;
    uint8_t     m_filmGrainBitDepthLumaMinus8;
    uint8_t     m_filmGrainBitDepthChromaMinus8;
    uint8_t     m_filmGrainColourPrimaries;
    uint8_t     m_filmGrainTransferCharacteristics;
    uint8_t     m_filmGrainMatrixCoeffs;

    void writeSEI(const SPS&)
    {
        WRITE_FLAG(m_filmGrainCharacteristicsCancelFlag, "film_grain_characteristics_cancel_flag");

        if (!m_filmGrainCharacteristicsCancelFlag)
        {
            WRITE_CODE(m_filmGrainModelId, 2, "film_grain_model_id");
            WRITE_FLAG(m_separateColourDescriptionPresentFlag, "separate_colour_description_present_flag");
            if (m_separateColourDescriptionPresentFlag)
            {
                WRITE_CODE(m_filmGrainBitDepthLumaMinus8, 3, "film_grain_bit_depth_luma_minus8");
                WRITE_CODE(m_filmGrainBitDepthChromaMinus8, 3, "film_grain_bit_depth_chroma_minus8");
                WRITE_FLAG(m_filmGrainFullRangeFlag, "film_grain_full_range_flag");
                WRITE_CODE(m_filmGrainColourPrimaries, X265_BYTE, "film_grain_colour_primaries");
                WRITE_CODE(m_filmGrainTransferCharacteristics, X265_BYTE, "film_grain_transfer_characteristics");
                WRITE_CODE(m_filmGrainMatrixCoeffs, X265_BYTE, "film_grain_matrix_coeffs");
            }
            WRITE_CODE(m_blendingModeId, 2, "blending_mode_id");
            WRITE_CODE(m_log2ScaleFactor, 4, "log2_scale_factor");
            for (uint8_t c = 0; c < 3; c++)
            {
                WRITE_FLAG(m_compModel[c].bPresentFlag && m_compModel[c].m_filmGrainNumIntensityIntervalMinus1 + 1 > 0 && m_compModel[c].numModelValues > 0, "comp_model_present_flag[c]");
            }
            for (uint8_t c = 0; c < 3; c++)
            {
                if (m_compModel[c].bPresentFlag && m_compModel[c].m_filmGrainNumIntensityIntervalMinus1 + 1 > 0 && m_compModel[c].numModelValues > 0)
                {
                    assert(m_compModel[c].m_filmGrainNumIntensityIntervalMinus1 + 1 <= 256);
                    assert(m_compModel[c].numModelValues <= X265_BYTE);
                    WRITE_CODE(m_compModel[c].m_filmGrainNumIntensityIntervalMinus1 , X265_BYTE, "num_intensity_intervals_minus1[c]");
                    WRITE_CODE(m_compModel[c].numModelValues - 1, 3, "num_model_values_minus1[c]");
                    for (uint8_t interval = 0; interval < m_compModel[c].m_filmGrainNumIntensityIntervalMinus1 + 1; interval++)
                    {
                        WRITE_CODE(m_compModel[c].intensityValues[interval].intensityIntervalLowerBound, X265_BYTE, "intensity_interval_lower_bound[c][i]");
                        WRITE_CODE(m_compModel[c].intensityValues[interval].intensityIntervalUpperBound, X265_BYTE, "intensity_interval_upper_bound[c][i]");
                        for (uint8_t j = 0; j < m_compModel[c].numModelValues; j++)
                        {
                            WRITE_SVLC(m_compModel[c].intensityValues[interval].compModelValue[j],"comp_model_value[c][i]");
                        }
                    }
                }
            }
            WRITE_FLAG(m_filmGrainCharacteristicsPersistenceFlag, "film_grain_characteristics_persistence_flag");
        }
        if (m_bitIf->getNumberOfWrittenBits() % X265_BYTE != 0)
        {
            WRITE_FLAG(1, "payload_bit_equal_to_one");
            while (m_bitIf->getNumberOfWrittenBits() % X265_BYTE != 0)
            {
                WRITE_FLAG(0, "payload_bit_equal_to_zero");
            }
        }
    }
};

static const uint32_t ISO_IEC_11578_LEN = 16;

class SEIuserDataUnregistered : public SEI
{
public:
    SEIuserDataUnregistered() : m_userData(NULL)
    {
        m_payloadType = USER_DATA_UNREGISTERED;
        m_payloadSize = 0;
    }
    static const uint8_t m_uuid_iso_iec_11578[ISO_IEC_11578_LEN];
    uint8_t *m_userData;
    void writeSEI(const SPS&)
    {
        for (uint32_t i = 0; i < ISO_IEC_11578_LEN; i++)
            WRITE_CODE(m_uuid_iso_iec_11578[i], 8, "sei.uuid_iso_iec_11578[i]");
        for (uint32_t i = 0; i < m_payloadSize; i++)
            WRITE_CODE(m_userData[i], 8, "user_data");
    }
};

class SEIMasteringDisplayColorVolume : public SEI
{
public:
    SEIMasteringDisplayColorVolume()
    {
        m_payloadType = MASTERING_DISPLAY_INFO;
        m_payloadSize = (8 * 2 + 2 * 4);
    }
    uint16_t displayPrimaryX[3];
    uint16_t displayPrimaryY[3];
    uint16_t whitePointX, whitePointY;
    uint32_t maxDisplayMasteringLuminance;
    uint32_t minDisplayMasteringLuminance;
    bool parse(const char* value)
    {
        return sscanf(value, "G(%hu,%hu)B(%hu,%hu)R(%hu,%hu)WP(%hu,%hu)L(%u,%u)",
                      &displayPrimaryX[0], &displayPrimaryY[0],
                      &displayPrimaryX[1], &displayPrimaryY[1],
                      &displayPrimaryX[2], &displayPrimaryY[2],
                      &whitePointX, &whitePointY,
                      &maxDisplayMasteringLuminance, &minDisplayMasteringLuminance) == 10;
    }
    void writeSEI(const SPS&)
    {
        for (uint32_t i = 0; i < 3; i++)
        {
            WRITE_CODE(displayPrimaryX[i], 16, "display_primaries_x[ c ]");
            WRITE_CODE(displayPrimaryY[i], 16, "display_primaries_y[ c ]");
        }
        WRITE_CODE(whitePointX, 16, "white_point_x");
        WRITE_CODE(whitePointY, 16, "white_point_y");
        WRITE_CODE(maxDisplayMasteringLuminance, 32, "max_display_mastering_luminance");
        WRITE_CODE(minDisplayMasteringLuminance, 32, "min_display_mastering_luminance");
    }
};

class SEIContentLightLevel : public SEI
{
public:
    SEIContentLightLevel()
    {
        m_payloadType = CONTENT_LIGHT_LEVEL_INFO;
        m_payloadSize = 4;
    }
    uint16_t max_content_light_level;
    uint16_t max_pic_average_light_level;
    void writeSEI(const SPS&)
    {
        WRITE_CODE(max_content_light_level,     16, "max_content_light_level");
        WRITE_CODE(max_pic_average_light_level, 16, "max_pic_average_light_level");
    }
};

class SEIDecodedPictureHash : public SEI
{
public:
    SEIDecodedPictureHash()
    {
        m_payloadType = DECODED_PICTURE_HASH;
        m_payloadSize = 0;
    }
    enum Method
    {
        MD5,
        CRC,
        CHECKSUM,
    } m_method;

    MD5Context m_state[3];
    uint32_t   m_crc[3];
    uint32_t   m_checksum[3];
    uint8_t    m_digest[3][16];

    void writeSEI(const SPS& sps)
    {
        int planes = (sps.chromaFormatIdc != X265_CSP_I400) ? 3 : 1;
        WRITE_CODE(m_method, 8, "hash_type");
        for (int yuvIdx = 0; yuvIdx < planes; yuvIdx++)
        {
            if (m_method == MD5)
            {
                for (uint32_t i = 0; i < 16; i++)
                    WRITE_CODE(m_digest[yuvIdx][i], 8, "picture_md5");
            }
            else if (m_method == CRC)
            {
                uint32_t val = (m_digest[yuvIdx][0] << 8) + m_digest[yuvIdx][1];
                WRITE_CODE(val, 16, "picture_crc");
            }
            else if (m_method == CHECKSUM)
            {
                uint32_t val = (m_digest[yuvIdx][0] << 24) + (m_digest[yuvIdx][1] << 16) + (m_digest[yuvIdx][2] << 8) + m_digest[yuvIdx][3];
                WRITE_CODE(val, 32, "picture_checksum");
            }
        }
    }
};

class SEIActiveParameterSets : public SEI
{
public:
    SEIActiveParameterSets()
    {
        m_payloadType = ACTIVE_PARAMETER_SETS;
        m_payloadSize = 0;
    }
    bool m_selfContainedCvsFlag;
    bool m_noParamSetUpdateFlag;

    void writeSEI(const SPS&)
    {
        WRITE_CODE(0, 4, "active_vps_id");
        WRITE_FLAG(m_selfContainedCvsFlag, "self_contained_cvs_flag");
        WRITE_FLAG(m_noParamSetUpdateFlag, "no_param_set_update_flag");
        WRITE_UVLC(0, "num_sps_ids_minus1");
        WRITE_UVLC(0, "active_seq_param_set_id");
        writeByteAlign();
    }
};

class SEIBufferingPeriod : public SEI
{
public:
    SEIBufferingPeriod()
        : m_cpbDelayOffset(0)
        , m_dpbDelayOffset(0)
        , m_concatenationFlag(0)
        , m_auCpbRemovalDelayDelta(1)
    {
        m_payloadType = BUFFERING_PERIOD;
        m_payloadSize = 0;
    }
    bool     m_cpbDelayOffset;
    bool     m_dpbDelayOffset;
    bool     m_concatenationFlag;
    uint32_t m_initialCpbRemovalDelay;
    uint32_t m_initialCpbRemovalDelayOffset;
    uint32_t m_auCpbRemovalDelayDelta;

    void writeSEI(const SPS& sps)
    {
        const HRDInfo& hrd = sps.vuiParameters.hrdParameters;

        WRITE_UVLC(0, "bp_seq_parameter_set_id");
        WRITE_FLAG(0, "rap_cpb_params_present_flag");
        WRITE_FLAG(m_concatenationFlag, "concatenation_flag");
        WRITE_CODE(m_auCpbRemovalDelayDelta - 1,   hrd.cpbRemovalDelayLength,       "au_cpb_removal_delay_delta_minus1");
        WRITE_CODE(m_initialCpbRemovalDelay,       hrd.initialCpbRemovalDelayLength,        "initial_cpb_removal_delay");
        WRITE_CODE(m_initialCpbRemovalDelayOffset, hrd.initialCpbRemovalDelayLength, "initial_cpb_removal_delay_offset");

        writeByteAlign();
    }
};

class SEIPictureTiming : public SEI
{
public:
    SEIPictureTiming()
    {
        m_payloadType = PICTURE_TIMING;
        m_payloadSize = 0;
    }
    uint32_t  m_picStruct;
    uint32_t  m_sourceScanType;
    bool      m_duplicateFlag;

    uint32_t  m_auCpbRemovalDelay;
    uint32_t  m_picDpbOutputDelay;

    void writeSEI(const SPS& sps)
    {
        const VUI *vui = &sps.vuiParameters;
        const HRDInfo *hrd = &vui->hrdParameters;

        if (vui->frameFieldInfoPresentFlag)
        {
            WRITE_CODE(m_picStruct, 4,          "pic_struct");
            WRITE_CODE(m_sourceScanType, 2,     "source_scan_type");
            WRITE_FLAG(m_duplicateFlag,         "duplicate_flag");
        }

        if (vui->hrdParametersPresentFlag)
        {
            WRITE_CODE(m_auCpbRemovalDelay - 1, hrd->cpbRemovalDelayLength, "au_cpb_removal_delay_minus1");
            WRITE_CODE(m_picDpbOutputDelay, hrd->dpbOutputDelayLength, "pic_dpb_output_delay");
            /* Removed sub-pic signaling June 2014 */
        }
        writeByteAlign();
    }
};

class SEIRecoveryPoint : public SEI
{
public:
    SEIRecoveryPoint()
    {
        m_payloadType = RECOVERY_POINT;
        m_payloadSize = 0;
    }
    int  m_recoveryPocCnt;
    bool m_exactMatchingFlag;
    bool m_brokenLinkFlag;

    void writeSEI(const SPS&)
    {
        WRITE_SVLC(m_recoveryPocCnt,    "recovery_poc_cnt");
        WRITE_FLAG(m_exactMatchingFlag, "exact_matching_flag");
        WRITE_FLAG(m_brokenLinkFlag,    "broken_link_flag");
        writeByteAlign();
    }
};

class SEIAlternativeTC : public SEI
{
public:
    int m_preferredTransferCharacteristics;
    SEIAlternativeTC()
    {
        m_payloadType = ALTERNATIVE_TRANSFER_CHARACTERISTICS;
        m_payloadSize = 0;
        m_preferredTransferCharacteristics = -1;
    }

    void writeSEI(const SPS&)
    {
        WRITE_CODE(m_preferredTransferCharacteristics, 8, "Preferred transfer characteristics");
    }
};

}
#endif // ifndef X265_SEI_H
