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

#include "input_platform.h"
#include "mir/module_properties.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"

namespace mo = mir::options;
namespace mi = mir::input;
namespace miv = mir::input::virt;

mir::UniqueModulePtr<mi::Platform> create_input_platform(
    mo::Option const& /*options*/,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<mi::InputDeviceRegistry> const& /*input_device_registry*/,
    std::shared_ptr<mir::ConsoleServices> const& /*console*/,
    std::shared_ptr<mi::InputReport> const& /*report*/)
{
    mir::assert_entry_point_signature<mi::CreatePlatform>(&create_input_platform);
    return mir::make_module_ptr<miv::VirtualInputPlatform>();
}

void add_input_platform_options(
    boost::program_options::options_description& /*config*/)
{
    mir::assert_entry_point_signature<mi::AddPlatformOptions>(&add_input_platform_options);
}

mi::PlatformPriority probe_input_platform(
    mo::Option const& /*options*/,
    mir::ConsoleServices&)
{
    mir::assert_entry_point_signature<mi::ProbePlatform>(&probe_input_platform);
    return mi::PlatformPriority::supported;
}

namespace
{
mir::ModuleProperties const description = {
    "mir:virt-input",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    mir::libname()
};
}

mir::ModuleProperties const* describe_input_module()
{
    mir::assert_entry_point_signature<mi::DescribeModule>(&describe_input_module);
    return &description;
}
