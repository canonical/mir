/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
mir::graphics::module_for_device(std::vector<std::shared_ptr<SharedLibrary>> const& modules, mir::options::ProgramOption const& options)
{
    mir::graphics::PlatformPriority best_priority_so_far = mir::graphics::unsupported;
    std::shared_ptr<mir::SharedLibrary> best_module_so_far;
    for (auto& module : modules)
    {
        try
        {
            auto probe = module->load_function<PlatformProbe>(
                 "probe_graphics_platform",
                 MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

            auto module_priority = probe(options);
            if (module_priority > best_priority_so_far)
            {
                best_priority_so_far = module_priority;
                best_module_so_far = module;
            }

            auto describe = module->load_function<DescribeModule>(
                "describe_graphics_module",
                MIR_SERVER_GRAPHICS_PLATFORM_VERSION);
            auto desc = describe();
            mir::log_info("Found graphics driver: %s", desc->name);
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
