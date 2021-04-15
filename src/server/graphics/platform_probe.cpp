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

auto mir::graphics::probe_module(
    mir::SharedLibrary& module,
    mir::options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console) -> mir::graphics::PlatformPriority
{
    auto probe = module.load_function<PlatformProbe>(
        "probe_graphics_platform",
        MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

    auto module_priority = probe(console, options);

    auto describe = module.load_function<DescribeModule>(
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


auto mir::graphics::modules_for_device(
    std::vector<std::shared_ptr<SharedLibrary>> const& modules,
    mir::options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console)
    -> std::vector<std::shared_ptr<SharedLibrary>>
{
    mir::graphics::PlatformPriority best_priority_so_far = mir::graphics::unsupported;
    std::vector<std::shared_ptr<SharedLibrary>> best_modules_so_far;
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
            auto module_priority = probe_module(*module, options, console);
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
