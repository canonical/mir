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

#include <mir/options/option.h>
#include <boost/program_options/options_description.hpp>

namespace mga=mir::graphics::android;
namespace mo = mir::options;

int mga::PropertiesOps::property_get(
    char const* key,
    char* value,
    char const* default_value) const
{
    return ::property_get(key, value, default_value);
}

namespace
{
char const* const num_framebuffers_opt = "enable-num-framebuffers-quirk";
char const* const single_gralloc_instance_opt = "enable-single-gralloc-instance-quirk";
char const* const width_alignment_opt = "enable-width-alignment-quirk";

std::string determine_device_name(mga::PropertiesWrapper const& properties)
{
    char const default_value[] = "";
    char value[PROP_VALUE_MAX] = "";
    char const key[] = "ro.product.device"; 
    properties.property_get(key, value, default_value);
    return std::string{value};
}

unsigned int num_framebuffers_for(std::string const& device_name, bool quirk_enabled)
{
    if (quirk_enabled && device_name == std::string{"mx3"})
        return 3;
    else
        return 2;
}

unsigned int gralloc_reopenable_after_close_for(std::string const& device_name, bool quirk_enabled)
{
    return !(quirk_enabled && device_name == std::string{"krillin"});
}
}

mga::DeviceQuirks::DeviceQuirks(PropertiesWrapper const& properties)
    : device_name(determine_device_name(properties)),
      num_framebuffers_(num_framebuffers_for(device_name, true)),
      gralloc_reopenable_after_close_(gralloc_reopenable_after_close_for(device_name, true)),
      enable_width_alignment_quirk{true}
{
}

mga::DeviceQuirks::DeviceQuirks(PropertiesWrapper const& properties, mo::Option const& options)
    : device_name(determine_device_name(properties)),
      num_framebuffers_(num_framebuffers_for(device_name, options.get(num_framebuffers_opt, true))),
      gralloc_reopenable_after_close_(gralloc_reopenable_after_close_for(device_name, options.get(single_gralloc_instance_opt, true))),
      enable_width_alignment_quirk(options.get(width_alignment_opt, true))
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

int mga::DeviceQuirks::aligned_width(int width) const
{
    if (enable_width_alignment_quirk && width == 720 && device_name == std::string{"vegetahd"})
        return 736;
    return width;
}

void mga::DeviceQuirks::add_options(boost::program_options::options_description& config)
{
    config.add_options()
        (num_framebuffers_opt,
         boost::program_options::value<bool>()->default_value(true),
         "[platform-specific] Enable allocating 3 framebuffers (MX3 quirk) [{true,false}]")
        (single_gralloc_instance_opt,
         boost::program_options::value<bool>()->default_value(true),
         "[platform-specific] Allocate a single gralloc instance (krillin quirk)  [{true,false}]")
         (width_alignment_opt,
          boost::program_options::value<bool>()->default_value(true),
          "[platform-specific] Enable width alignment (vegetahd quirk) [{true,false}]");
}
