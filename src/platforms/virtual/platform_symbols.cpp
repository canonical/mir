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

#include <mir/graphics/platform.h>
#include <optional>

#include "platform.h"

#include <mir/assert_module_entry_point.h>
#include <mir/libname.h>
#include <mir/options/program_option.h>

namespace mg = mir::graphics;
namespace mo = mir::options;
namespace mgv = mir::graphics::virt;

namespace
{
mir::ModuleProperties const description = {
    "mir:virt",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    mir::libname()
};
char const* virtual_displays_option_name{"virtual-output"};
}

mir::UniqueModulePtr<mg::DisplayPlatform> create_display_platform(
    mg::SupportedDevice const&,
    std::shared_ptr<mo::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const& /*console*/,
    std::shared_ptr<mg::DisplayReport> const& report)
{
    mir::assert_entry_point_signature<mg::CreateDisplayPlatform>(&create_display_platform);

    auto output_sizes = mgv::Platform::parse_output_sizes(options->get<std::string>(virtual_displays_option_name));
    return mir::make_module_ptr<mgv::Platform>(report, std::move(output_sizes));
}

void add_graphics_platform_options(boost::program_options::options_description& config)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
    config.add_options()
        (virtual_displays_option_name,
         boost::program_options::value<std::string>()->default_value("1280x1024"),
         "[mir-on-virtual specific] Colon separated list of WIDTHxHEIGHT sizes for \"output\" windows."
         " ^SCALE may also be appended to any output");
}

auto probe_display_platform(
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mir::udev::Context> const&,
    mo::ProgramOption const&) -> std::vector<mg::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_display_platform);
    std::vector<mg::SupportedDevice> result;
    result.push_back({
        nullptr,
        mg::PlatformPriority::hosted,
        nullptr
    });
    return result;
}

auto probe_rendering_platform(
    std::span<std::shared_ptr<mg::DisplayInterfaceProvider>> const&,
    mir::ConsoleServices&,
    std::shared_ptr<mir::udev::Context> const&,
    mir::options::ProgramOption const&) -> std::vector<mir::graphics::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::RenderProbe>(&probe_rendering_platform);
    std::vector<mg::SupportedDevice> result;
    result.emplace_back(
        mg::SupportedDevice {
            nullptr,
            mg::PlatformPriority::supported,
            nullptr
        });
    return result;
}

mir::ModuleProperties const* describe_graphics_module()
{
    mir::assert_entry_point_signature<mg::DescribeModule>(&describe_graphics_module);
    return &description;
}
