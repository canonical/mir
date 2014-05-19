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

#include "device_detector.h"

namespace mga=mir::graphics::android;

int mga::PropertiesOps::property_get(
    char const* key,
    char* value,
    char const* default_value) const
{
    return ::property_get(key, value, default_value);
}

mga::DeviceDetector::DeviceDetector(PropertiesWrapper const& properties)
{
    char const key[] = "ro.product.device"; 
    char const default_value[] = "";
    char value[PROP_VALUE_MAX] = "";
    properties.property_get(key, value, default_value);
    device_name_ = std::string{value};
}

std::string mga::DeviceDetector::device_name() const
{
    return device_name_;
}
