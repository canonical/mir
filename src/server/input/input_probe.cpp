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

#include "mir/input/input_probe.h"
#include "mir/input/platform.h"

#include "mir/options/configuration.h"
#include "mir/options/option.h"

#include "mir/shared_library_prober.h"
#include "mir/shared_library.h"
#include "mir/log.h"

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


auto select_platforms_from_list(
    std::string const& selection,
    std::vector<std::shared_ptr<mir::SharedLibrary>> const& modules
) -> std::shared_ptr<mir::SharedLibrary>
{
    std::vector<std::string> found_module_names;

    for (auto const& module : modules)
    {
        try
        {
            auto describe_module = module->load_function<mi::DescribeModule>(
                "describe_input_module",
                MIR_SERVER_INPUT_PLATFORM_VERSION);
            auto const description = describe_module();
            found_module_names.emplace_back(description->name);

            if (description->name == selection)
                return module;
        }
        catch (std::exception const&)
        {
            // Should we log anything here?
        }
    }

    std::stringstream error_msg;
    error_msg << "Failed to find the requested platform module." << std::endl;
    error_msg << "Detected modules are: " << std::endl;
    for (auto const& module : found_module_names)
    {
        error_msg << "\t" << module << std::endl;
    }
    error_msg << "Failed to find: " << selection << std::endl;
    BOOST_THROW_EXCEPTION((std::runtime_error{error_msg.str()}));
}
}

mir::UniqueModulePtr<mi::Platform> mi::probe_input_platforms(
    mo::Option const& options,
    std::shared_ptr<EmergencyCleanupRegistry> const& emergency_cleanup,
    std::shared_ptr<mi::InputDeviceRegistry> const& device_registry,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mi::InputReport> const& input_report,
    std::vector<std::shared_ptr<SharedLibrary>> const& loaded_platforms,
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

                auto const priority = probe(options, *console);
                if (priority > reject_platform_priority)
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

    // First, check if any already-loaded graphics modules double as an input platform
    mir::UniqueModulePtr<Platform> loaded_platform;
    for (auto const& module : loaded_platforms)
    {
        loaded_platform = input_platform_from_graphics_module(
            *module,
            options,
            emergency_cleanup,
            device_registry,
            console,
            input_report);
        if (loaded_platform)
        {
            // If this platform was manually specified AND it's already loaded, we can return it here
            if (options.is_set(mo::platform_input_lib))
            {
                auto describe_module = module->load_function<mi::DescribeModule>(
                    "describe_input_module",
                    MIR_SERVER_INPUT_PLATFORM_VERSION);
                auto const description = describe_module();
                if (description->name == options.get<std::string>(mo::platform_input_lib))
                {
                    return loaded_platform;
                }
            }
            break;
        }
    }

    // Otherwise, try and load a manually specified platform, as that takes precedence over the loaded platform
    if (options.is_set(mo::platform_input_lib))
    {
        auto platforms = mir::libraries_for_path(options.get<std::string>(mo::platform_path), prober_report);
        reject_platform_priority = PlatformPriority::unsupported;
        module_selector(select_platforms_from_list(options.get<std::string>(mo::platform_input_lib), platforms));
    }
    else
    {
        // We haven't specified a platform to load, so if the already-loaded platform exists, we can return that
        if (loaded_platform)
            return loaded_platform;

        // If all else fails, let's look for the best platform
        select_libraries_for_path(options.get<std::string>(mo::platform_path), module_selector, prober_report);
    }

    if (!platform_module)
        BOOST_THROW_EXCEPTION(std::runtime_error{"No appropriate input platform module found"});

    return create_input_platform(*platform_module, options, emergency_cleanup, device_registry, console, input_report);
}

auto mi::input_platform_from_graphics_module(
    SharedLibrary const& graphics_platform,
    options::Option const& options,
    std::shared_ptr<EmergencyCleanupRegistry> const& emergency_cleanup,
    std::shared_ptr<InputDeviceRegistry> const& device_registry,
    std::shared_ptr<ConsoleServices> const& console,
    std::shared_ptr<InputReport> const& input_report) -> mir::UniqueModulePtr<Platform>
{
    try
    {
        return create_input_platform(graphics_platform, options, emergency_cleanup, device_registry, console, input_report);
    }
    catch (std::runtime_error const&)
    {
        // Assume the graphics platform is not also an input module.
        return {};
    }
}
