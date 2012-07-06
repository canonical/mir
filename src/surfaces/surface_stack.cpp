/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/compositor/buffer_bundle.h"
#include "mir/compositor/buffer_bundle_factory.h"
#include "mir/compositor/buffer_texture_binder.h"
#include "mir/surfaces/surface.h"
#include "mir/surfaces/surface_stack.h"

#include <cassert>
#include <memory>

namespace ms = mir::surfaces;

ms::SurfaceStack::SurfaceStack(mc::BufferBundleFactory* bb_factory) : buffer_bundle_factory(bb_factory)
{
    assert(buffer_bundle_factory);
}

ms::SurfacesToRender ms::SurfaceStack::get_surfaces_in(geometry::Rectangle const& /*display_area*/)
{
	return SurfacesToRender();
}

std::weak_ptr<ms::Surface> ms::SurfaceStack::create_surface(const ms::SurfaceCreationParameters& params)
{
    std::shared_ptr<ms::Surface> surface(
        new ms::Surface(
            params,
            std::dynamic_pointer_cast<mc::BufferTextureBinder>(buffer_bundle_factory->create_buffer_bundle(
                params.width,
                params.height,
                mc::PixelFormat::rgba_8888))));
    return surface;
}

void ms::SurfaceStack::destroy_surface(std::weak_ptr<ms::Surface> /*surface*/)
{
}
