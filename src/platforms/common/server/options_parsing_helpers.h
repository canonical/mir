/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_PLATFORM_OPTIONS_PARSING_HELPERS_H
#define MIR_PLATFORM_OPTIONS_PARSING_HELPERS_H

#include <string>
#include <mir/geometry/size.h>

namespace mir
{
namespace graphics
{
namespace common
{

auto parse_size(std::string const& str) -> mir::geometry::Size;
auto parse_size_dimension(std::string const& str) -> int;

}
}
}

#endif //MIR_PLATFORM_OPTIONS_PARSING_HELPERS_H
