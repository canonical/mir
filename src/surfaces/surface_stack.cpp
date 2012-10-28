/*
 * Copyright © 2012 Canonical Ltd.
 *
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

#include "mir/compositor/buffer_bundle_factory.h"
#include "mir/graphics/renderer.h"
#include "mir/surfaces/surface.h"
#include "mir/surfaces/surface_stack.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <set>

namespace ms = mir::surfaces;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

ms::SurfaceStack::SurfaceStack(mc::BufferBundleFactory* bb_factory) : buffer_bundle_factory(bb_factory)
{
    assert(buffer_bundle_factory);
}

void ms::SurfaceStack::apply(mc::RenderableFilter& filter, mc::RenderableOperator& renderable_operator)
{
    std::lock_guard<std::mutex> lock(guard);
    for (auto it = surfaces.begin(); it != surafces.end(); ++it)
    {
	    auto surface = **it;
		if (filter(surface)) renderable_operator(surface);
	}
}


std::weak_ptr<ms::Surface> ms::SurfaceStack::create_surface(const ms::SurfaceCreationParameters& params)
{
    std::lock_guard<std::mutex> lg(guard);

    std::shared_ptr<ms::Surface> surface(
        new ms::Surface(
            params.name,
            buffer_bundle_factory->create_buffer_bundle(
                geom::Size{params.size.width, params.size.height},
                geom::PixelFormat::rgba_8888)));

    surfaces.insert(surface);
    return surface;
}

void ms::SurfaceStack::destroy_surface(std::weak_ptr<ms::Surface> surface)
{
    std::lock_guard<std::mutex> lg(guard);

    auto const p = std::find(surfaces.begin(), surfaces.end(), surface.lock());

    if (p != surfaces.end()) surfaces.erase(p);
    // else; TODO error logging
}

