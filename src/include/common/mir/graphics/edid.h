/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_EDID_H_
#define MIR_GRAPHICS_EDID_H_

#include <cstdint>
#include <cstddef>

namespace mir { namespace graphics {

struct Edid
{
    Edid() = delete;
    Edid(Edid const&) = delete;
    Edid(Edid const&&) = delete;

    enum { minimum_size = 128 };
    typedef char MonitorName[14];  // up to 13 characters
    typedef char Manufacturer[4];  // always 3 characters

    size_t get_monitor_name(MonitorName str) const;
    size_t get_manufacturer(Manufacturer str) const;
    uint16_t product_code() const;
    uint32_t serial_number() const;

private:
    /* Pretty much every field in an EDID requires some kind of conversion
       and reinterpretation. So keep those details private... */
   
    enum StringDescriptorType
    {
        string_monitor_serial_number = 0xff,
        string_unspecified_text = 0xfe,
        string_monitor_name = 0xfc,
    };

    size_t get_string(StringDescriptorType type, char str[14]) const;

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

} } // namespace mir::graphics

#endif // MIR_GRAPHICS_EDID_H_
