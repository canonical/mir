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
std::string determine_device_name(mga::PropertiesWrapper const& properties)
{
    char const default_value[] = "";
    char value[PROP_VALUE_MAX] = "";
    char const key[] = "ro.product.device"; 
    properties.property_get(key, value, default_value);
    return std::string{value};
}

unsigned int num_framebuffers_for(std::string const& device_name)
{
    if (device_name == std::string{"mx3"})
        return 3;
    else
        return 2;
}

unsigned int gralloc_reopenable_after_close_for(std::string const& device_name)
{
    return device_name != std::string{"krillin"}; 
}
}

mga::DeviceQuirks::DeviceQuirks(PropertiesWrapper const& properties)
    : device_name(determine_device_name(properties)),
      num_framebuffers_(num_framebuffers_for(device_name)),
      gralloc_reopenable_after_close_(gralloc_reopenable_after_close_for(device_name))
{
}

unsigned int mga::DeviceQuirks::num_framebuffers() const
{
    return num_framebuffers_;
}

bool mga::DeviceQuirks::gralloc_reopenable_after_close() const
{
    return gralloc_reopenable_after_close_;
}
