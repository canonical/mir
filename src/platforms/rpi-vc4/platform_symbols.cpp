/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#define MIR_LOG_COMPONENT "rpi-vc4"

#include "mir/log.h"
#include "mir/graphics/platform.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"
#include "mir/module_deleter.h"
#include "mir/options/option.h"
#include "mir/options/program_option.h"
#include "mir/udev/wrapper.h"
#include "mir/fd.h"
#include "mir/signal_blocker.h"

#include "platform.h"
#include "display_platform.h"
#include "rendering_platform.h"

#include <bcm_host.h>

#include <fcntl.h>

namespace mg = mir::graphics;
namespace mo = mir::options;

mir::UniqueModulePtr<mg::Platform> create_host_platform(
    std::shared_ptr<mo::Option> const& /*options*/,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<mir::ConsoleServices> const& /*console*/,
    std::shared_ptr<mg::DisplayReport> const& /*report*/,
    std::shared_ptr<mir::logging::Logger> const& /*logger*/)
{
    mir::assert_entry_point_signature<mg::CreateHostPlatform>(&create_host_platform);

    {
        // bcm_host_init() spawns threads, which must not receive signals
        mir::SignalBlocker blocker;
        // bcm_host_init() must be called before any other call which touches the GPU
        // Conveniently, however, it is idempotent.
        bcm_host_init();
    }

    return mir::make_module_ptr<mg::rpi::Platform>();
}

void add_graphics_platform_options(boost::program_options::options_description& /*config*/)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
}

mg::PlatformPriority probe_graphics_platform(std::shared_ptr<mir::ConsoleServices> const& /*console*/,
                                             mo::ProgramOption const& /*options*/)
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_graphics_platform);

    auto udev = std::make_shared<mir::udev::Context>();

    mir::udev::Enumerator drm_devices{udev};
    drm_devices.match_subsystem("drm");
    drm_devices.match_sysname("card[0-9]*");
    drm_devices.scan_devices();

    /*
     * The proprietary DispmanX drivers do not provide KMS, so if there are
     * KMS devices then we're probably not using the proprietary drivers.
     */
    if (drm_devices.begin() != drm_devices.end())
    {
        return mg::PlatformPriority::unsupported;
    }

    /*
     * bcm_host_init() will (transitively) attempt to open this device node,
     * and “conveniently” exit(-1) if it fails, so let's use this as a signal
     * that we're not on a supported platform.
     */
    auto vchiq_dev_fd = mir::Fd{open("/dev/vchiq", O_RDWR)};
    if (vchiq_dev_fd == mir::Fd::invalid)
        return mg::PlatformPriority::unsupported;

    /*
     * Sadly we can't actually *try* to probe the dispmanx driver; attempting
     * to call bcm_host_init() will terminate the process if it fails, so let's
     * just say we're plausible
     */
    return mg::PlatformPriority::supported;
}

namespace
{
mir::ModuleProperties const description = {
    "mir:rpi-vc4",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    mir::libname()};
}

mir::ModuleProperties const* describe_graphics_module()
{
    mir::assert_entry_point_signature<mg::DescribeModule>(&describe_graphics_module);
    return &description;
}

mir::UniqueModulePtr<mir::graphics::DisplayPlatform> create_display_platform(
    std::shared_ptr<mo::Option> const&,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mg::DisplayReport> const&,
    std::shared_ptr<mir::logging::Logger> const&)
{
    mir::assert_entry_point_signature<mg::CreateDisplayPlatform>(&create_display_platform);

    {
        // bcm_host_init() spawns threads, which must not receive signals
        mir::SignalBlocker blocker;
        // bcm_host_init() must be called before any other call which touches the GPU
        // Conveniently, however, it is idempotent.
        bcm_host_init();
    }
    return mir::make_module_ptr<mg::rpi::DisplayPlatform>();
}

mir::UniqueModulePtr<mir::graphics::RenderingPlatform> create_rendering_platform(
    std::shared_ptr<mir::options::Option> const&,
    std::shared_ptr<mir::graphics::PlatformAuthentication> const&)
{
    mir::assert_entry_point_signature<mg::CreateRenderingPlatform>(&create_rendering_platform);

    {
        // bcm_host_init() spawns threads, which must not receive signals
        mir::SignalBlocker blocker;
        // bcm_host_init() must be called before any other call which touches the GPU
        // Conveniently, however, it is idempotent.
        bcm_host_init();
    }
    return mir::make_module_ptr<mg::rpi::RenderingPlatform>();
}
