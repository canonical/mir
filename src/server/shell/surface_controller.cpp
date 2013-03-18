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

#include "mir/shell/surface_controller.h"
#include "mir/surfaces/surface_stack_model.h"
#include "mir/frontend/surface.h"
#include "mir/input/input_channel_factory.h"

#include "surface.h"

#include <cassert>

namespace ms = mir::surfaces;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace mf = mir::frontend;


msh::SurfaceController::SurfaceController(std::shared_ptr<ms::SurfaceStackModel> const& surface_stack,
					  std::shared_ptr<mi::InputChannelFactory> const& input_factory)
  : surface_stack(surface_stack),
    input_factory(input_factory)
{
    assert(surface_stack);
}

std::shared_ptr<mf::Surface> msh::SurfaceController::create_surface(const mf::SurfaceCreationParameters& params)
{
    return std::make_shared<Surface>(
        surface_stack->create_surface(params),
	input_factory->make_input_channel(),
        [=] (std::weak_ptr<mir::surfaces::Surface> const& surface) { surface_stack->destroy_surface(surface); });
}

