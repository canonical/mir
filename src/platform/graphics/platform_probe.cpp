/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/log.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_probe.h"

#include <boost/throw_exception.hpp>

std::shared_ptr<mir::SharedLibrary>
mir::graphics::module_for_device(
    std::vector<std::shared_ptr<SharedLibrary>> const& modules,
    mir::options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console)
{
    mir::graphics::PlatformPriority best_priority_so_far = mir::graphics::unsupported;
    std::shared_ptr<mir::SharedLibrary> best_module_so_far;
    for (auto& module : modules)
    {
        try
        {
            auto probe =
                [module]() -> std::function<std::remove_pointer<PlatformProbe>::type>
                {
                    try
                    {
                        return module->load_function<PlatformProbe>(
                            "probe_graphics_platform",
                            MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
                    }
                    catch (std::runtime_error const&)
                    {
                        // Maybe we can load an earlier version?
                        auto obsolete_probe = module->load_function<obsolete_0_27::PlatformProbe>(
                            "probe_graphics_platform",
                            obsolete_0_27::symbol_version);

                        return [obsolete_probe](auto, auto const& options)
                            {
                                auto const priority = static_cast<unsigned int>(obsolete_probe(options));

                                /*
                                 * Cap obsolete modules to just less than PlatformPriority::supported.
                                 * If *any* current module that will work, we want that instead.
                                 */
                                return priority >= PlatformPriority::supported ?
                                    static_cast<PlatformPriority>(PlatformPriority::supported - 1) :
                                    static_cast<PlatformPriority>(priority);
                            };
                    }
                }();

            auto module_priority = probe(console, options);
            if (module_priority > best_priority_so_far)
            {
                best_priority_so_far = module_priority;
                best_module_so_far = module;
            }

            auto describe =
                [module]()
                {
                    try
                    {
                        return module->load_function<DescribeModule>(
                            "describe_graphics_module",
                            MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

                    }
                    catch (std::runtime_error const&)
                    {
                        return module->load_function<DescribeModule>(
                            "describe_graphics_module",
                            obsolete_0_27::symbol_version);

                    }
                }() ;
            auto desc = describe();
            mir::log_info("Found graphics driver: %s (version %d.%d.%d)",
                          desc->name,
                          desc->major_version,
                          desc->minor_version,
                          desc->micro_version);
        }
        catch (std::runtime_error const&)
        {
        }
    }
    if (best_priority_so_far > mir::graphics::unsupported)
    {
        return best_module_so_far;
    }
    BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find platform for current system"}));
}
