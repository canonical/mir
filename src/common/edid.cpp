/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/graphics/edid.h"
#include <endian.h>
#include <cstring>

using mir::graphics::Edid;

size_t Edid::get_monitor_name(MonitorName str) const
{
    size_t len = get_string(string_monitor_name, str);
    if (char* pad = strchr(str, '\n'))
    {
        *pad = '\0';
        len = pad - str;
    }
    return len;
}

size_t Edid::get_manufacturer(Manufacturer str) const
{
    // Confusingly this field is more like big endian. Others are little.
    auto man = static_cast<uint16_t>(manufacturer[0]) << 8 | manufacturer[1];
    str[0] = ((man >> 10) & 31) + 'A' - 1;
    str[1] = ((man >> 5) & 31) + 'A' - 1;
    str[2] = (man & 31) + 'A' - 1;
    str[3] = '\0';
    return 3;
}

uint16_t Edid::product_code() const
{
    return le16toh(product_code_le);
}

size_t Edid::get_string(StringDescriptorType type, char str[14]) const
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
