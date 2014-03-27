/*
 * Copyright © 2013-2014 Canonical Ltd.
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
#include "basic_surface_factory.h"

namespace ms = mir::scene;
namespace msh = mir::shell;

ms::SurfaceController::SurfaceController(
    std::shared_ptr<BasicSurfaceFactory> const& surface_factory,
    std::shared_ptr<SurfaceStackModel> const& surface_stack) :
    surface_factory(surface_factory),
    surface_stack(surface_stack)
{
}

std::weak_ptr<ms::Surface> ms::SurfaceController::create_surface(
    frontend::SurfaceId id,
    shell::SurfaceCreationParameters const& params,
    std::shared_ptr<frontend::EventSink> const& event_sink,
    std::shared_ptr<shell::SurfaceConfigurator> const& configurator)
{
    auto const surface = surface_factory->create_surface(id, params, event_sink, configurator);
    surface_stack->add_surface(surface, params.depth, params.input_mode);
    return surface;
}

void ms::SurfaceController::destroy_surface(std::weak_ptr<Surface> const& surface)
{
    surface_stack->remove_surface(surface);
}

void ms::SurfaceController::raise(std::weak_ptr<Surface> const& surface)
{
    surface_stack->raise(surface);
}
