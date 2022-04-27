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
 */

#define MIR_LOG_COMPONENT "rpi-dispmanx"

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

#include "display_platform.h"
#include "rendering_platform.h"

#include <bcm_host.h>

#include <fcntl.h>

namespace mg = mir::graphics;
namespace mo = mir::options;

mir::UniqueModulePtr<mg::DisplayPlatform> create_display_platform(
    mg::SupportedDevice const&,
    std::shared_ptr<mo::Option> const& /*options*/,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<mir::ConsoleServices> const& /*console*/,
    std::shared_ptr<mg::DisplayReport> const& /*report*/)
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

auto create_rendering_platform(
    mg::SupportedDevice const&,
    std::vector<std::shared_ptr<mg::DisplayPlatform>> const&,
    mo::Option const&,
    mir::EmergencyCleanupRegistry&) -> mir::UniqueModulePtr<mg::RenderingPlatform>
{
    mir::assert_entry_point_signature<mg::CreateRenderPlatform>(&create_rendering_platform);

    {
        // bcm_host_init() spawns threads, which must not receive signals
        mir::SignalBlocker blocker;
        // bcm_host_init() must be called before any other call which touches the GPU
        // Conveniently, however, it is idempotent.
        bcm_host_init();
    }

    return mir::make_module_ptr<mg::rpi::RenderingPlatform>();
}

void add_graphics_platform_options(boost::program_options::options_description& /*config*/)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
}

auto probe_graphics_platform(std::shared_ptr<mir::udev::Context> const& udev) -> std::vector<mg::SupportedDevice>
{
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
        return {};
    }

    /*
     * bcm_host_init() will (transitively) attempt to open this device node,
     * and “conveniently” exit(-1) if it fails, so let's use this as a signal
     * that we're not on a supported platform.
     */
    auto vchiq_dev_fd = mir::Fd{open("/dev/vchiq", O_RDWR)};
    if (vchiq_dev_fd == mir::Fd::invalid)
        return {};

    /*
     * Sadly we can't actually *try* to probe the dispmanx driver; attempting
     * to call bcm_host_init() will terminate the process if it fails, so let's
     * just say we're plausible
     */
    std::vector<mg::SupportedDevice> devices;
    devices.emplace_back(
        mg::SupportedDevice {
            nullptr,
            mg::PlatformPriority::supported,
            nullptr
        });

    return devices;
}

auto probe_display_platform(
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mir::udev::Context> const& udev,
    mo::ProgramOption const&) -> std::vector<mg::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_display_platform);
    return probe_graphics_platform(udev);
}

auto probe_rendering_platform(
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mir::udev::Context> const& udev,
    mo::ProgramOption const&) -> std::vector<mg::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_rendering_platform);
    return probe_graphics_platform(udev);
}

namespace
{
mir::ModuleProperties const description = {
    "mir:rpi-dispmanx",
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

