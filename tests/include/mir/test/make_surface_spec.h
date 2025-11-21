/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_SURFACE_SPECIFICATION_H_
#define MIR_TEST_SURFACE_SPECIFICATION_H_

#include <mir/shell/surface_specification.h>

#include <functional>

namespace mir
{
namespace compositor
{
class BufferStream;
}
namespace test
{

auto make_surface_spec(std::shared_ptr<compositor::BufferStream> const& stream) -> shell::SurfaceSpecification;

}
}

#endif // MIR_TEST_TEST_DISPATCHABLE_H_
