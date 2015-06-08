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

#ifndef MIR_GRAPHICS_ANDROID_DEVICE_QUIRKS_H_
#define MIR_GRAPHICS_ANDROID_DEVICE_QUIRKS_H_

#include <hybris/properties/properties.h>
#include <string>

namespace boost{ namespace program_options {class options_description;}}

namespace mir
{
namespace options{ class Option; }
namespace graphics
{
namespace android
{
class PropertiesWrapper
{
public:
    PropertiesWrapper() = default;
    virtual ~PropertiesWrapper() = default;
    virtual int property_get(
        char const* key,
        char* value,
        char const* default_value) const = 0;
private:
    PropertiesWrapper(PropertiesWrapper const&) = delete;
    PropertiesWrapper& operator=(PropertiesWrapper const&) = delete;
};

class PropertiesOps : public PropertiesWrapper
{
public:
    int property_get(
        char const* key,
        char* value,
        char const* default_value) const;
};

class DeviceQuirks
{
public:
    DeviceQuirks(PropertiesWrapper const& properties);
    DeviceQuirks(PropertiesWrapper const& properties, mir::options::Option const& options);

    unsigned int num_framebuffers() const;
    bool gralloc_reopenable_after_close() const;
    int aligned_width(int width) const;

    static void add_options(boost::program_options::options_description& config);

private:
    DeviceQuirks(DeviceQuirks const&) = delete;
    DeviceQuirks & operator=(DeviceQuirks const&) = delete;
    std::string const device_name;
    unsigned int const num_framebuffers_;
    bool const gralloc_reopenable_after_close_;
    bool const enable_width_alignment_quirk;
};
}
}
}
#endif /* MIR_GRAPHICS_ANDROID_DEVICE_QUIRKS_H_ */
