/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/log.h"
#include "mir/graphics/platform.h"
#include "platform_probe.h"

#include <boost/throw_exception.hpp>

namespace
{
auto probe_module(
    mir::graphics::PlatformProbe const& probe,
    mir::SharedLibrary& module,
    mir::options::ProgramOption const& options,
    std::shared_ptr<mir::ConsoleServices> const& console) -> mir::graphics::PlatformPriority
{

    auto module_priority = probe(console, options);

    auto describe = module.load_function<mir::graphics::DescribeModule>(
        "describe_graphics_module",
        MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

    auto desc = describe();
    mir::log_info("Found graphics driver: %s (version %d.%d.%d) Support priority: %d",
                  desc->name,
                  desc->major_version,
                  desc->minor_version,
                  desc->micro_version,
                  module_priority);
    return module_priority;
}
}

auto mir::graphics::probe_display_module(
    SharedLibrary& module,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console) -> PlatformPriority
{
    return probe_module(
        module.load_function<mir::graphics::PlatformProbe>(
            "probe_display_platform",
            MIR_SERVER_GRAPHICS_PLATFORM_VERSION),
        module,
        options,
        console);
}

auto mir::graphics::probe_rendering_module(
    SharedLibrary& module,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console) -> PlatformPriority
{
    return probe_module(
        module.load_function<mir::graphics::PlatformProbe>(
            "probe_rendering_platform",
            MIR_SERVER_GRAPHICS_PLATFORM_VERSION),
        module,
        options,
        console);
}

namespace
{
enum class ModuleType
{
    Rendering,
    Display
};

auto modules_for_device(
    ModuleType type,
    std::vector<std::shared_ptr<mir::SharedLibrary>> const& modules,
    mir::options::ProgramOption const& options,
    std::shared_ptr<mir::ConsoleServices> const& console)
-> std::vector<std::shared_ptr<mir::SharedLibrary>>
{
    mir::graphics::PlatformPriority best_priority_so_far = mir::graphics::unsupported;
    std::vector<std::shared_ptr<mir::SharedLibrary>> best_modules_so_far;
    for (auto& module : modules)
    {
        try
        {
            /* TODO: We will (pretty shortly) need to have some concept of binding-a-device.
             * What we really want to do here is:
             * foreach device in system:
             *   find best driver for device
             *
             * For now, hopefully “load each platform that claims to best support (at least some)
             * device” will work.
             */
            mir::graphics::PlatformPriority module_priority;
            switch (type)
            {
            case ModuleType::Rendering:
                module_priority = mir::graphics::probe_rendering_module(*module, options, console);
                break;
            case ModuleType::Display:
                module_priority = mir::graphics::probe_display_module(*module, options, console);
                break;
            }
            if (module_priority > best_priority_so_far)
            {
                best_priority_so_far = module_priority;
                best_modules_so_far.clear();
                best_modules_so_far.push_back(module);
            }
            else if (module_priority == best_priority_so_far)
            {
                best_modules_so_far.push_back(module);
            }
        }
        catch (std::runtime_error const&)
        {
        }
    }
    if (best_priority_so_far > mir::graphics::unsupported)
    {
        return best_modules_so_far;
    }
    BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find any platforms for current system"}));
}
}

auto mir::graphics::display_modules_for_device(
    std::vector<std::shared_ptr<SharedLibrary>> const& modules,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console) -> std::vector<std::shared_ptr<SharedLibrary>>
{
    return modules_for_device(
        ModuleType::Display,
        modules,
        options,
        console);
}

auto mir::graphics::rendering_modules_for_device(
    std::vector<std::shared_ptr<SharedLibrary>> const& modules,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console) -> std::vector<std::shared_ptr<SharedLibrary>>
{
    return modules_for_device(
        ModuleType::Rendering,
        modules,
        options,
        console);
}
