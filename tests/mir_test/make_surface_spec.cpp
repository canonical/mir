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

#include "mir/test/make_surface_spec.h"
#include "mir/compositor/buffer_stream.h"

namespace mt = mir::test;
namespace mc = mir::compositor;
namespace msh = mir::shell;

auto mt::make_surface_spec(std::shared_ptr<mc::BufferStream> const& stream) -> msh::SurfaceSpecification
{
    msh::SurfaceSpecification result;
    result.streams = {msh::StreamSpecification{stream, {}}};
    return result;
}
