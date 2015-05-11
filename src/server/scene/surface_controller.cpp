/*
 * Copyright © 2013-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "surface_controller.h"
#include "surface_stack_model.h"
#include "mir/scene/surface_factory.h"
#include "mir/scene/surface.h"

namespace ms = mir::scene;

ms::SurfaceController::SurfaceController(
    std::shared_ptr<SurfaceFactory> const& surface_factory,
    std::shared_ptr<SurfaceStackModel> const& surface_stack) :
    surface_factory(surface_factory),
    surface_stack(surface_stack)
{
}

void ms::SurfaceController::add_surface(
    std::shared_ptr<ms::Surface> const& surface,
    ms::DepthId depth, 
    mir::input::InputReceptionMode const& input_mode,
    Session* /*session*/)
{
    surface_stack->add_surface(surface, depth, input_mode);
}

void ms::SurfaceController::remove_surface(std::weak_ptr<Surface> const& surface)
{
    surface_stack->remove_surface(surface);
}

void ms::SurfaceController::raise(std::weak_ptr<Surface> const& surface)
{
    surface_stack->raise(surface);
}

void ms::SurfaceController::raise(SurfaceSet const& surfaces)
{
    surface_stack->raise(surfaces);
}

auto ms::SurfaceController::surface_at(geometry::Point cursor) const
-> std::shared_ptr<Surface>
{
    return surface_stack->surface_at(cursor);
}
