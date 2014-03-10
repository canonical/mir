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

#include "mir/device_detector.h"

int mir::PropertiesOps::property_get(
    char const key[PROP_NAME_MAX],
    char value[PROP_VALUE_MAX],
    char const default_value[PROP_VALUE_MAX]) const
{
    return ::property_get(key, value, default_value);
}

mir::DeviceDetector::DeviceDetector(PropertiesWrapper const& properties)
{
    static char const key[PROP_NAME_MAX] = "ro.product.device"; 
    static char const default_value[PROP_VALUE_MAX] = "";
    static char value[PROP_VALUE_MAX] = "";
    properties.property_get(key, value, default_value);
    device_name_ = std::string{value};
    android_device_present_ = !(device_name_ == std::string{default_value});
}

bool mir::DeviceDetector::android_device_present() const
{
    return android_device_present_;
}

std::string mir::DeviceDetector::device_name() const
{
    return device_name_;
}
