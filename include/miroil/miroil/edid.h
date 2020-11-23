/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#ifndef MIRAL_EDID_H
#define MIRAL_EDID_H

#include <string>
#include <vector>

// Prototyping namespace for later incorporation in MirAL
namespace miral
{

struct Edid
{
    std::string vendor;
    uint16_t product_code{0};
    uint32_t serial_number{0};

    struct PhysicalSizeMM { int width; int height; };
    PhysicalSizeMM size{0,0};

    struct Descriptor {
        enum class Type : uint8_t {
            timing_identifiers = 0xfa,
            white_point_data = 0xfb,
            monitor_name = 0xfc,
            monitor_limits = 0xfd,
            unspecified_text = 0xfe,
            serial_number = 0xff,

            undefined = 0x00,
        };

        union Value {
            char monitor_name[13];
            char unspecified_text[13];
            char serial_number[13];
        };

        Type type{Type::undefined};
        Value value{{0}};

        std::string string_value() const;
    };
    Descriptor descriptors[4];

    Edid& parse_data(std::vector<uint8_t> const&);
};

}

#endif // MIRAL_EDID_H
