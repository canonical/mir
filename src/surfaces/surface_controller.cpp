/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/surfaces/surface_controller.h"
#include "mir/surfaces/surface.h"
#include "mir/surfaces/surface_stack_model.h"
#include "mir/shell/surface.h"

#include "proxy_surface.h"

#include <cassert>

namespace ms = mir::surfaces;
namespace msh = mir::shell;

ms::SurfaceController::SurfaceController(std::shared_ptr<SurfaceStackModel> const& surface_stack) : surface_stack(surface_stack)
{
    assert(surface_stack);
}

std::shared_ptr<msh::Surface> ms::SurfaceController::create_surface(const msh::SurfaceCreationParameters& params)
{
    return std::make_shared<ProxySurface>(surface_stack.get(), params);
}
