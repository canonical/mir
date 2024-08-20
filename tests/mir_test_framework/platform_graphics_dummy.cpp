/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "mir/libname.h"
#include "mir/graphics/platform.h"
#include "mir/assert_module_entry_point.h"
#include "mir/udev/wrapper.h"

namespace mg = mir::graphics;

namespace mir
{
}

auto probe_display_platform(
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mir::udev::Context> const&,
    mir::options::Option const&) -> std::vector<mir::graphics::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_display_platform);
    std::vector<mg::SupportedDevice> result;
    result.emplace_back(
        mg::SupportedDevice {
            nullptr,
            mg::probe::dummy,
            nullptr
         });
    return result;
}

auto probe_rendering_platform(
    std::span<std::shared_ptr<mg::DisplayPlatform>> const&,
    mir::ConsoleServices&,
    std::shared_ptr<mir::udev::Context> const&,
    mir::options::Option const&) -> std::vector<mir::graphics::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::RenderProbe>(&probe_rendering_platform);
    std::vector<mg::SupportedDevice> result;
    result.emplace_back(
        mg::SupportedDevice {
            nullptr,
            mg::probe::dummy,
            nullptr
         });
    return result;
}

namespace
{
mir::ModuleProperties const module_properties = {
    "mir:stub-graphics",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    mir::libname()
};
}

mir::ModuleProperties const* describe_graphics_module()
{
    mir::assert_entry_point_signature<mir::graphics::DescribeModule>(&describe_graphics_module);
    return &module_properties;
}
