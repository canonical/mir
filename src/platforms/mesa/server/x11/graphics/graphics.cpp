/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "mir/graphics/display_report.h"
#include "mir/options/option.h"
#include "platform.h"
#include "gbm_platform.h"
#include "platform_common.h"
#include "../X11_resources.h"
#include "mir/module_deleter.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"

#include <boost/throw_exception.hpp>

namespace mo = mir::options;
namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mx = mir::X;
namespace mgx = mg::X;
namespace geom = mir::geometry;

namespace
{
char const* x11_displays_option_name{"x11-output"};
}

mir::UniqueModulePtr<mg::Platform> create_host_platform(
    std::shared_ptr<mo::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mg::DisplayReport> const& report,
    std::shared_ptr<mir::logging::Logger> const& /*logger*/)
{
    mir::assert_entry_point_signature<mg::CreateHostPlatform>(&create_host_platform);
    if (!mx::X11Resources::instance.get_conn())
        BOOST_THROW_EXCEPTION(std::runtime_error("Need valid x11 output"));

    auto output_sizes = mgx::Platform::parse_output_sizes(options->get<std::string>(x11_displays_option_name));

    return mir::make_module_ptr<mgx::Platform>(
        mx::X11Resources::instance.get_conn(),
        output_sizes,
        report
    );
}

void add_graphics_platform_options(boost::program_options::options_description& config)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
    config.add_options()
        (x11_displays_option_name,
         boost::program_options::value<std::string>()->default_value("1280x1024"),
         "[mir-on-X specific] Comma separated list of WIDTHxHEIGHT for \"output\" windows.");
}

mg::PlatformPriority probe_graphics_platform(
    std::shared_ptr<mir::ConsoleServices> const&,
        mo::ProgramOption const& /*options*/)
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_graphics_platform);
    auto dpy = XOpenDisplay(nullptr);
    if (dpy)
    {
        XCloseDisplay(dpy);

        auto udev = std::make_shared<mir::udev::Context>();

        mir::udev::Enumerator drm_devices{udev};
        drm_devices.match_subsystem("drm");
        drm_devices.match_sysname("renderD[0-9]*");
        drm_devices.scan_devices();

        if (drm_devices.begin() != drm_devices.end())
            return mg::PlatformPriority::supported;
    }
    return mg::PlatformPriority::unsupported;
}

namespace
{
mir::ModuleProperties const description = {
    "mir:mesa-x11",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    mir::libname()
};
}

mir::ModuleProperties const* describe_graphics_module()
{
    mir::assert_entry_point_signature<mg::DescribeModule>(&describe_graphics_module);
    return &description;
}

mir::UniqueModulePtr<mir::graphics::DisplayPlatform> create_display_platform(
    std::shared_ptr<mo::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mg::DisplayReport> const& report,
    std::shared_ptr<mir::logging::Logger> const&)
{
    mir::assert_entry_point_signature<mg::CreateDisplayPlatform>(&create_display_platform);

    if (!mx::X11Resources::instance.get_conn())
        BOOST_THROW_EXCEPTION(std::runtime_error("Need valid x11 output"));

    auto output_sizes = mgx::Platform::parse_output_sizes(options->get<std::string>(x11_displays_option_name));

    return mir::make_module_ptr<mgx::Platform>(
        mx::X11Resources::instance.get_conn(),
        output_sizes,
        report);
}

mir::UniqueModulePtr<mir::graphics::RenderingPlatform> create_rendering_platform(
    std::shared_ptr<mir::options::Option> const&,
    std::shared_ptr<mir::graphics::PlatformAuthentication> const&)
{
    mir::assert_entry_point_signature<mg::CreateRenderingPlatform>(&create_rendering_platform);
    auto udev = std::make_shared<mir::udev::Context>();
    return mir::make_module_ptr<mgm::GBMPlatform>(
        mgm::BypassOption::prohibited, mgm::BufferImportMethod::dma_buf,
        udev,
        mgm::helpers::DRMHelper::open_any_render_node(udev));
}
