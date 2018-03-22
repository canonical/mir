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

#ifndef MIR_GRAPHICS_PLATFORM_PROBE_H_
#define MIR_GRAPHICS_PLATFORM_PROBE_H_

#include <vector>
#include <memory>
#include "mir/shared_library.h"
#include "mir/options/program_option.h"

namespace mir
{
class ConsoleServices;

namespace graphics
{
class Platform;

std::shared_ptr<SharedLibrary> module_for_device(
    std::vector<std::shared_ptr<SharedLibrary>> const& modules,
    options::ProgramOption const& options,
    std::shared_ptr<ConsoleServices> const& console);

}
}

#endif // MIR_GRAPHICS_PLATFORM_PROBE_H_
