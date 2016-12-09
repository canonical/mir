/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_EDID_H_
#define MIR_EDID_H_

#include <endian.h>
#include <cstdint>
#include <cstring>

namespace mir
{

struct EDID
{
    EDID() = delete;
    EDID(EDID const&) = delete;
    EDID(EDID const&&) = delete;

    enum { minimum_size = 128 };
    typedef char MonitorName[14];  // up to 13 characters
    typedef char Manufacturer[4];  // always 3 characters

    size_t get_monitor_name(MonitorName str) const
    {
        size_t len = get_string(string_monitor_name, str);
        if (char* pad = strchr(str, '\n'))
        {
            *pad = '\0';
            len = pad - str;
        }
        return len;
    }

    size_t get_manufacturer(Manufacturer str) const
    {
        // Confusingly this field is more like big endian. Others are little.
        auto man = static_cast<uint16_t>(manufacturer[0]) << 8 | manufacturer[1];
        str[0] = ((man >> 10) & 31) + 'A' - 1;
        str[1] = ((man >> 5) & 31) + 'A' - 1;
        str[2] = (man & 31) + 'A' - 1;
        str[3] = '\0';
        return 3;
    }

    uint16_t product_code() const
    {
        return le16toh(product_code_le);
    }

private:
    /* Pretty much every field in an EDID requires some kind of conversion
       and reinterpretation. So keep those details private... */
   
    enum StringDescriptorType
    {
        string_monitor_serial_number = 0xff,
        string_unspecified_text = 0xfe,
        string_monitor_name = 0xfc,
    };

    size_t get_string(StringDescriptorType type, char str[14]) const
    {
        size_t len = 0;
        for (int d = 0; d < 4; ++d)
        {
            auto& desc = descriptor[d];
            if (!desc.other.zero0 && desc.other.type == type)
            {
                len = sizeof desc.other.text;
                memcpy(str, desc.other.text, len);
                break;
            }
        }
        str[len] = '\0';
        return len;
    }

    union Descriptor
    {
        struct
        {
            uint16_t pixel_clock_le;
            uint8_t  todo[16];
        } detailed_timing;
        struct
        {
            uint16_t zero0;
            uint8_t  zero2;
            uint8_t  type;
            uint8_t  zero4;
            uint8_t  text[13];
        } other;
    };

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif
    /* 0x00 */ uint8_t  header[8];
    /* 0x08 */ uint8_t  manufacturer[2];
    /* 0x0a */ uint16_t product_code_le;
    /* 0x0c */ uint32_t serial_number_le;
    /* 0x10 */ uint8_t  week_of_manufacture;
    /* 0x11 */ uint8_t  year_of_manufacture;
    /* 0x12 */ uint8_t  edid_version;
    /* 0x13 */ uint8_t  edid_revision;
    /* 0x14 */ uint8_t  input_bitmap;
    /* 0x15 */ uint8_t  max_horz_cm;
    /* 0x16 */ uint8_t  max_vert_cm;
    /* 0x17 */ uint8_t  gamma;
    /* 0x18 */ uint8_t  features_bitmap;
    /* 0x19 */ uint8_t  red_green_bits_1to0;
    /* 0x1a */ uint8_t  blue_white_bits_1to0;
    /* 0x1b */ uint8_t  red_x_bits_9to2;
    /* 0x1c */ uint8_t  red_y_bits_9to2;
    /* 0x1d */ uint8_t  green_x_bits_9to2;
    /* 0x1e */ uint8_t  green_y_bits_9to2;
    /* 0x1f */ uint8_t  blue_x_bits_9to2;
    /* 0x20 */ uint8_t  blue_y_bits_9to2;
    /* 0x21 */ uint8_t  white_x_bits_9to2;
    /* 0x22 */ uint8_t  white_y_bits_9to2;
    /* 0x23 */ uint8_t  established_timings[2];
    /* 0x25 */ uint8_t  reserved_timings;
    /* 0x26 */ uint8_t  standard_timings[2][8];
    /* 0x36 */ Descriptor descriptor[4];
    /* 0x7e */ uint8_t  num_extensions;  /* each is another 128-byte block */
    /* 0x7f */ uint8_t  checksum;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
};

} // namespace mir

#endif // MIR_EDID_H_
