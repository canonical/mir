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

#ifndef MIR_GRAPHICS_ANDROID_DEVICE_DETECTOR_H_
#define MIR_GRAPHICS_ANDROID_DEVICE_DETECTOR_H_

#include <hybris/properties/properties.h>
#include <string>

namespace mir
{
namespace graphics
{

class PropertiesWrapper
{
public:
    PropertiesWrapper() = default;
    virtual ~PropertiesWrapper() = default;
    virtual int property_get(
        char const key[PROP_NAME_MAX],
        char value[PROP_VALUE_MAX],
        char const default_value[PROP_VALUE_MAX]) const = 0;
private:
    PropertiesWrapper(PropertiesWrapper const&) = delete;
    PropertiesWrapper& operator=(PropertiesWrapper const&) = delete;
};

class AndroidPropertiesOps : public PropertiesWrapper
{
public:
    int property_get(
        char const key[PROP_NAME_MAX] ,
        char value[PROP_VALUE_MAX],
        char const default_value[PROP_VALUE_MAX]) const;
};

class DeviceDetector
{
public:
    DeviceDetector(PropertiesWrapper const& properties);

    bool android_device_present() const;
    std::string device_name() const;
private:
    std::string device_name_;
    bool android_device_present_; 
};

}
}
#endif /* MIR_GRAPHICS_ANDROID_DEVICE_DETECTOR_H_ */
