/*
 * Copyright © 2015-2016 Canonical Ltd.
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
#include "mir/libname.h"

#include <stdexcept>

namespace mi = mir::input;
namespace mo = mir::options;

namespace
{
mir::UniqueModulePtr<mi::Platform> create_input_platform(
    mir::SharedLibrary const& lib, mir::options::Option const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& cleanup_registry,
    std::shared_ptr<mi::InputDeviceRegistry> const& registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mi::InputReport> const& report)
{
    auto desc = lib.load_function<mi::DescribeModule>("describe_input_module", MIR_SERVER_INPUT_PLATFORM_VERSION)();
    auto create = lib.load_function<mi::CreatePlatform>("create_input_platform", MIR_SERVER_INPUT_PLATFORM_VERSION);

    auto result = create(options, cleanup_registry, registry, console, report);

    mir::log_info(
        "Selected input driver: %s (version: %d.%d.%d)",
        desc->name, desc->major_version, desc->minor_version, desc->micro_version);

    return result;
}
}

mir::UniqueModulePtr<mi::Platform> mi::probe_input_platforms(
    mo::Option const& options,
    std::shared_ptr<EmergencyCleanupRegistry> const& emergency_cleanup,
    std::shared_ptr<mi::InputDeviceRegistry> const& device_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mi::InputReport> const& input_report,
    mir::SharedLibraryProberReport& prober_report)
{
    auto reject_platform_priority = mi::PlatformPriority::dummy;

    std::shared_ptr<mir::SharedLibrary> platform_module;
    std::vector<std::string> module_names;

    auto const module_selector = [&](std::shared_ptr<mir::SharedLibrary> const& module)
        {
            try
            {
                auto const probe = module->load_function<mi::ProbePlatform>(
                    "probe_input_platform", MIR_SERVER_INPUT_PLATFORM_VERSION);

                if (probe(options) > reject_platform_priority)
                {
                    platform_module = module;

                    return Selection::quit;
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

    if (!platform_module)
        BOOST_THROW_EXCEPTION(std::runtime_error{"No appropriate input platform module found"});

    return create_input_platform(*platform_module, options, emergency_cleanup, device_registry, console, input_report);
}

auto mi::input_platform_from_graphics_module(
    graphics::Platform const& graphics_platform,
    options::Option const& options,
    std::shared_ptr<EmergencyCleanupRegistry> const& emergency_cleanup,
    std::shared_ptr<InputDeviceRegistry> const& device_registry,
    std::shared_ptr<ConsoleServices> const& console,
    std::shared_ptr<InputReport> const& input_report)
-> mir::UniqueModulePtr<Platform>
{
    try
    {
        // Yes, this is dirty code that assumes the object layout. Sorry!
        auto* const vtab = (void*&)(graphics_platform);
        SharedLibrary const platform_module{detail::libname_impl(vtab)};

        return create_input_platform(platform_module, options, emergency_cleanup, device_registry, console, input_report);
    }
    catch (std::runtime_error const&)
    {
        // Assume the graphics platform is not also an input module.
        return {};
    }
}
