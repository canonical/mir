/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "input_platform.h"
#include "mir/module_properties.h"
#include "mir/input/platform.h"

namespace mo = mir::options;
namespace mi = mir::input;
namespace mix = mi::X;

extern "C" mir::UniqueModulePtr<mi::Platform> create_input_platform(
    std::shared_ptr<mo::Option> const& /*options*/,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<mi::InputDeviceRegistry> const& input_device_registry,
    std::shared_ptr<mi::InputReport> const& /*report*/)
{
    return mir::make_module_ptr<mix::XInputPlatform>(input_device_registry);
}

extern "C" void add_input_platform_options(
    boost::program_options::options_description& /*config*/)
{
}

extern "C" mi::PlatformPriority probe_input_platform(
    mo::Option const& /*options*/)
{
    return mi::PlatformPriority::best;
}

namespace
{
mir::ModuleProperties const description = {
    "x11-input",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO
};
}

extern "C" mir::ModuleProperties const* describe_input_module()
{
    return &description;
}
