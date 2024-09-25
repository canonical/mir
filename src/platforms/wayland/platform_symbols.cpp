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

#include "wayland_display.h"
#include "platform.h"

#include <mir/assert_module_entry_point.h>
#include <mir/libname.h>
#include <mir/options/program_option.h>
#include <mir/udev/wrapper.h>

namespace mg = mir::graphics;
namespace mo = mir::options;
namespace mgw = mir::graphics::wayland;
namespace mpw = mir::platform::wayland;

namespace
{
mir::ModuleProperties const description = {
    "mir:wayland",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    mir::libname()
};

char const* wayland_window_app_id_option{"wayland-window-app-id"};
char const* wayland_window_app_id_option_description{"Defines the XdgToplevel app id on the surface created by the wayland platform"};
}

mir::UniqueModulePtr<mg::DisplayPlatform> create_display_platform(
    mg::SupportedDevice const&,
    std::shared_ptr<mo::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const& /*console*/,
    std::shared_ptr<mg::DisplayReport> const& report)
{
    std::optional<std::string> app_id;
    if (options->is_set(wayland_window_app_id_option))
        app_id = options->get<std::string>(wayland_window_app_id_option);

    mir::assert_entry_point_signature<mg::CreateDisplayPlatform>(&create_display_platform);
    return mir::make_module_ptr<mgw::Platform>(mpw::connection(*options), report, app_id);
}

void add_graphics_platform_options(boost::program_options::options_description& config)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
    mpw::add_connection_options(config);
    config.add_options()
        (wayland_window_app_id_option,
         boost::program_options::value<std::string>(),
         wayland_window_app_id_option_description);
}

auto probe_graphics_platform(
    mo::Option const& options) -> std::optional<mg::SupportedDevice>
{
    if (mpw::connection_options_supplied(options))
    {
        return mg::SupportedDevice {
            nullptr,
            mg::probe::nested,
            nullptr
        };
    }
    return {};
}

auto probe_display_platform(
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mir::udev::Context> const&,
    mo::Option const& options) -> std::vector<mg::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_display_platform);
    if (auto probe = probe_graphics_platform(options))
    {
        std::vector<mg::SupportedDevice> result;
        result.emplace_back(std::move(probe.value()));
        return result;
    }
    return {};
}

mir::ModuleProperties const* describe_graphics_module()
{
    mir::assert_entry_point_signature<mg::DescribeModule>(&describe_graphics_module);
    return &description;
}
