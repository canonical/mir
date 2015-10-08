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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/input/input_probe.h"
#include "mir/input/platform.h"

#include "mir/options/configuration.h"
#include "mir/options/option.h"

#include "mir/shared_library_prober.h"
#include "mir/shared_library.h"
#include "mir/log.h"
#include "mir/module_deleter.h"

#include <stdexcept>

namespace mi = mir::input;
namespace mo = mir::options;

namespace
{
auto probe_input_platform(mir::SharedLibrary const& lib, mir::options::Option const& options)
{
    auto probe = lib.load_function<mi::ProbePlatform>("probe_input_platform", MIR_SERVER_INPUT_PLATFORM_VERSION);

    return probe(options);
}

void describe_input_platform(mir::SharedLibrary const& lib)
{
    auto describe =
        lib.load_function<mi::DescribeModule>("describe_input_module", MIR_SERVER_INPUT_PLATFORM_VERSION);
    auto desc = describe();
    mir::log_info("Found input driver: %s", desc->name);
}

mir::UniqueModulePtr<mi::Platform> create_input_platform(
    mir::SharedLibrary const& lib, mir::options::Option const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& cleanup_registry,
    std::shared_ptr<mi::InputDeviceRegistry> const& registry, std::shared_ptr<mi::InputReport> const& report)
{

    auto create = lib.load_function<mi::CreatePlatform>("create_input_platform", MIR_SERVER_INPUT_PLATFORM_VERSION);

    return create(options, cleanup_registry, registry, report);
}
}

std::vector<mir::UniqueModulePtr<mi::Platform>> mi::probe_input_platforms(
    mo::Option const& options, std::shared_ptr<EmergencyCleanupRegistry> const& emergency_cleanup,
    std::shared_ptr<mi::InputDeviceRegistry> const& device_registry, std::shared_ptr<mi::InputReport> const& input_report,
    mir::SharedLibraryProberReport& prober_report)
{
    std::vector<UniqueModulePtr<Platform>> platforms;

    if (options.is_set(mo::platform_input_lib))
    {
        mir::SharedLibrary lib(options.get<std::string>(mo::platform_input_lib));

        platforms.emplace_back(create_input_platform(lib, options, emergency_cleanup, device_registry, input_report));

        describe_input_platform(lib);
    }
    else
    {
        auto const& path = options.get<std::string>(mo::platform_path);
        auto platforms_libs = mir::libraries_for_path(path, prober_report);

        for (auto const& platform_lib : platforms_libs)
        {
            try
            {
                if (probe_input_platform(*platform_lib, options) > mi::PlatformPriority::dummy)
                {
                    platforms.emplace_back(
                        create_input_platform(*platform_lib, options, emergency_cleanup, device_registry, input_report));

                    describe_input_platform(*platform_lib);
                }
            }
            catch (std::runtime_error const&)
            {
            }
        }
    }

    return platforms;
}
