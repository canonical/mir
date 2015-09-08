/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

#include "platform.h"
#include "../X11_resources.h"
#include <boost/throw_exception.hpp>

namespace mo = mir::options;
namespace mg = mir::graphics;
namespace mx = mir::X;
namespace mgx = mg::X;

mx::X11Resources x11_resources;

std::shared_ptr<mg::Platform> create_host_platform(
    std::shared_ptr<mo::Option> const& /*options*/,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<mg::DisplayReport> const& /*report*/)
{
    if (!x11_resources.get_conn())
        BOOST_THROW_EXCEPTION(std::runtime_error("Need valid x11 display"));

    return std::make_shared<mgx::Platform>(x11_resources.get_conn());
}

std::shared_ptr<mg::Platform> create_guest_platform(
    std::shared_ptr<mg::DisplayReport> const& /*report*/,
    std::shared_ptr<mg::NestedContext> const&)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Guest platform isn't supported under X"));
    return nullptr;
}

void add_graphics_platform_options(boost::program_options::options_description& /*config*/)
{
}

mg::PlatformPriority probe_graphics_platform(mo::ProgramOption const& /*options*/)
{
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
            return mg::PlatformPriority::best;
    }
    return mg::PlatformPriority::unsupported;
}

mir::ModuleProperties const description = {
    "mesa-x11",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO
};

mir::ModuleProperties const* describe_graphics_module()
{
    return &description;
}
