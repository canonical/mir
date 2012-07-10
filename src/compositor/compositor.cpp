/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/compositor/compositor.h"

#include "mir/graphics/display.h"
#include "mir/geometry/rectangle.h"
#include "mir/surfaces/scenegraph.h"

#include <cassert>

namespace mc = mir::compositor;
namespace ms = mir::surfaces;

mc::Compositor::Compositor(
    ms::Scenegraph* scenegraph,
    const std::shared_ptr<ms::SurfaceRenderer>& renderer)
        : scenegraph(scenegraph),
          renderer(renderer)
{
    assert(scenegraph);
    assert(renderer);
}

void mc::Compositor::render(graphics::Display* display)
{
    assert(display);

    auto surfaces_in_view_area = scenegraph->get_surfaces_in(display->view_area());
    assert(surfaces_in_view_area);
    
    surfaces_in_view_area->apply(renderer.get());
    
    display->notify_update();
}
