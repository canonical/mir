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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "platform.h"
#include "mir/udev/wrapper.h"
#include "mir/fd.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <memory>
#include <string>
#include <iostream>

namespace mo = mir::options;
namespace mi = mir::input;
namespace mu = mir::udev;
namespace mie = mi::evdev;

namespace
{
char const* const host_socket_opt = "host-socket";

mir::ModuleProperties const description = {
    "mir:evdev-input",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    mir::libname()
};

bool can_open_input_devices()
{
    mu::Enumerator input_enumerator{std::make_shared<mu::Context>()};
    input_enumerator.match_subsystem("input");
    input_enumerator.scan_devices();

    bool device_found = false;

    for (auto& device : input_enumerator)
    {
        if (device.devnode() != nullptr)
        {
            device_found = true;

            mir::Fd input_device(::open(device.devnode(), O_RDONLY|O_NONBLOCK));
            if (input_device > 0)
                return true;
        }
    }
    return ! device_found;
}

}

mir::UniqueModulePtr<mi::Platform> create_input_platform(
    mo::Option const& /*options*/,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<mi::InputDeviceRegistry> const& input_device_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mi::InputReport> const& report)
{
    mir::assert_entry_point_signature<mi::CreatePlatform>(&create_input_platform);
    return mir::make_module_ptr<mie::Platform>(
        input_device_registry,
        report,
        std::make_unique<mu::Context>(),
        console);
}

void add_input_platform_options(
    boost::program_options::options_description& /*config*/)
{
    mir::assert_entry_point_signature<mi::AddPlatformOptions>(&add_input_platform_options);
    // no options to add yet
}

mi::PlatformPriority probe_input_platform(
    mo::Option const& options)
{
    mir::assert_entry_point_signature<mi::ProbePlatform>(&probe_input_platform);
    if (options.is_set(host_socket_opt))
    {
        return mi::PlatformPriority::unsupported;
    }

    if (can_open_input_devices())
        return mi::PlatformPriority::supported;

    return mi::PlatformPriority::unsupported;
}

mir::ModuleProperties const* describe_input_module()
{
    mir::assert_entry_point_signature<mi::DescribeModule>(&describe_input_module);
    return &description;
}
