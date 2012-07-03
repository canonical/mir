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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/surfaces/surface_controller.h"
#include "mir/surfaces/surface_stack.h"

#include <cassert>

namespace ms = mir::surfaces;

ms::SurfaceController::SurfaceController(ms::SurfaceStack* surface_stack) : surface_stack(surface_stack)
{
    assert(surface_stack);
}

void ms::SurfaceController::add_surface(std::weak_ptr<ms::Surface> surface)
{
    surface_stack->add_surface(surface);
}

void ms::SurfaceController::remove_surface(std::weak_ptr<ms::Surface> surface)
{
    surface_stack->remove_surface(surface);
}
