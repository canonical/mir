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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/surfaces/surface_controller.h"
#include "mir/surfaces/surface.h"
#include "mir/surfaces/surface_stack_model.h"

#include <cassert>

namespace ms = mir::surfaces;

ms::SurfaceController::SurfaceController(ms::SurfaceStackModel* surface_stack) : surface_stack(surface_stack)
{
    assert(surface_stack);
}

std::weak_ptr<ms::Surface> ms::SurfaceController::create_surface(const sessions::SurfaceCreationParameters& params)
{
    return surface_stack->create_surface(params);
}

void ms::SurfaceController::destroy_surface(std::weak_ptr<ms::Surface> const& surface)
{
    surface_stack->destroy_surface(surface);
}

void ms::SurfaceController::hide_surface(std::weak_ptr<ms::Surface> const& surface)
{
    surface.lock()->set_hidden(true);
}

void ms::SurfaceController::show_surface(std::weak_ptr<ms::Surface> const& surface)
{
    surface.lock()->set_hidden(false);
}
