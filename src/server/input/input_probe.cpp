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
mir::UniqueModulePtr<mi::Platform> create_input_platform(
    mir::SharedLibrary const& lib, mir::options::Option const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& cleanup_registry,
    std::shared_ptr<mi::InputDeviceRegistry> const& registry, std::shared_ptr<mi::InputReport> const& report)
{

    auto create = lib.load_function<mi::CreatePlatform>("create_input_platform", MIR_SERVER_INPUT_PLATFORM_VERSION);

    return create(options, cleanup_registry, registry, report);
}
}

mir::UniqueModulePtr<mi::Platform> mi::probe_input_platforms(
    mo::Option const& options, std::shared_ptr<EmergencyCleanupRegistry> const& emergency_cleanup,
    std::shared_ptr<mi::InputDeviceRegistry> const& device_registry, std::shared_ptr<mi::InputReport> const& input_report,
    mir::SharedLibraryProberReport& prober_report)
{
    auto reject_platform_priority = mi::PlatformPriority::dummy;

    std::vector<std::shared_ptr<mir::SharedLibrary>> platform_modules;
    std::vector<std::string> module_names;

    auto const module_selector = [&](std::shared_ptr<mir::SharedLibrary> const& module)
        {
            try
            {
                auto const probe = module->load_function<mi::ProbePlatform>(
                    "probe_input_platform", MIR_SERVER_INPUT_PLATFORM_VERSION);
                auto const desc = module->load_function<mi::DescribeModule>(
                    "describe_input_module", MIR_SERVER_INPUT_PLATFORM_VERSION)();

                // We only take the first found of duplicate modules, as that will be the most recent.
                // This is a heuristic that assumes we're always looking for the most up-to-date driver,
                // TODO find a way to coordinate the selection of mesa-x11 and input platforms
                auto const duplicate = find(begin(module_names), end(module_names), desc->name) != end(module_names);

                if (probe(options) > reject_platform_priority && !duplicate)
                {
                    platform_modules.push_back(module);

                    module_names.push_back(desc->name);
                }
            }
            catch (std::runtime_error const&)
            {
                // Assume we were handed a SharedLibrary that's not an input module of the correct vintage.
            }

            return Selection::persist;
        };

    if (options.is_set(mo::platform_input_lib))
    {
        reject_platform_priority = PlatformPriority::unsupported;
        module_selector(std::make_shared<mir::SharedLibrary>(options.get<std::string>(mo::platform_input_lib)));
    }
    else
    {
        select_libraries_for_path(options.get<std::string>(mo::platform_path), module_selector, prober_report);
    }

    // We process the modules in decending .sonumber order so, luckily, we try mesa-x11 before evdev.
    // But only because the graphics platform version is currently higher than the input platform version.
    // TODO find a way to coordinate the selection of mesa-x11 and input platforms
    for (auto& module : platform_modules)
    {
        auto const desc = module->load_function<mi::DescribeModule>(
            "describe_input_module", MIR_SERVER_INPUT_PLATFORM_VERSION)();

        auto result = create_input_platform(*module, options, emergency_cleanup, device_registry, input_report);

        mir::log_info(
            "Selected input driver: %s (version: %d.%d.%d)",
            desc->name, desc->major_version, desc->minor_version, desc->micro_version);

        return result;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error{"No appropriate input platform module found"});
}
