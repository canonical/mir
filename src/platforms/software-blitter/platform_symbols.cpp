/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "rendering_platform.h"

#include <mir/module_deleter.h>
#include <mir/assert_module_entry_point.h>
#include <mir/libname.h>
#include <mir/udev/wrapper.h>
#include <mir/graphics/platform.h>

namespace mg = mir::graphics;
namespace mgsb = mg::software_blitter;
namespace mo = mir::options;

auto create_rendering_platform(
    mg::SupportedDevice const&,
    std::vector<std::shared_ptr<mg::DisplayPlatform>> const&,
    mo::Option const&,
    mir::EmergencyCleanupRegistry&) -> mir::UniqueModulePtr<mg::RenderingPlatform>
{
    mir::assert_entry_point_signature<mg::CreateRenderPlatform>(&create_rendering_platform);

    return mir::make_module_ptr<mgsb::SoftwareBlitterRenderingPlatform>();
}

void add_graphics_platform_options(boost::program_options::options_description&)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
}

auto probe_rendering_platform(
    std::span<std::shared_ptr<mg::DisplayPlatform>> const&,
    mir::ConsoleServices&,
    std::shared_ptr<mir::udev::Context> const&,
    mo::Option const&) -> std::vector<mg::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::RenderProbe>(&probe_rendering_platform);

    std::vector<mg::SupportedDevice> supported_devices;
    supported_devices.push_back(mg::SupportedDevice{nullptr, mg::probe::supported, nullptr});
    return supported_devices;
}

namespace
{
mir::ModuleProperties const description =
    {"mir:software-blitter", MIR_VERSION_MAJOR, MIR_VERSION_MINOR, MIR_VERSION_MICRO, mir::libname()};
}

mir::ModuleProperties const* describe_graphics_module()
{
    mir::assert_entry_point_signature<mg::DescribeModule>(&describe_graphics_module);
    return &description;
}
