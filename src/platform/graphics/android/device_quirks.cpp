/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "device_quirks.h"

namespace mga=mir::graphics::android;

int mga::PropertiesOps::property_get(
    char const* key,
    char* value,
    char const* default_value) const
{
    return ::property_get(key, value, default_value);
}

namespace
{
unsigned int determine_num_framebuffers(mga::PropertiesWrapper const& properties)
{
    char const key[] = "ro.product.device"; 
    char const default_value[] = "";
    char value[PROP_VALUE_MAX] = "";
    properties.property_get(key, value, default_value);
    if (std::string{"mx3"} == std::string{value})
        return 3;
    else
        return 2;
}
}

mga::DeviceQuirks::DeviceQuirks(PropertiesWrapper const& properties)
    : num_framebuffers_(determine_num_framebuffers(properties))
{
}

unsigned int mga::DeviceQuirks::num_framebuffers() const
{
    return num_framebuffers_;
}
