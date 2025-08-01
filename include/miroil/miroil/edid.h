/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIROIL_EDID_H
#define MIROIL_EDID_H

#include <cstdint>
#include <string>
#include <vector>

namespace miroil
{

struct Edid
{
    std::string vendor;
    uint16_t product_code{0};
    uint32_t serial_number{0};

    struct PhysicalSizeMM { int width; int height; };
    PhysicalSizeMM size{0,0};

    struct Descriptor {
        std::string string_value() const { return {}; };
    };

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    Descriptor descriptors[0];
#pragma GCC diagnostic pop
};

}

#endif // MIROIL_EDID_H
