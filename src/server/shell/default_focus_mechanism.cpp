/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored By: Robert Carr <robert.carr@canonical.com>
 */

#include "default_focus_mechanism.h"
#include "mir/frontend/session.h"
#include "mir/scene/surface_coordinator.h"
#include "mir/scene/surface.h"
#include "mir/shell/input_targeter.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

msh::DefaultFocusMechanism::DefaultFocusMechanism(std::shared_ptr<msh::InputTargeter> const& input_targeter,
                                                  std::shared_ptr<ms::SurfaceCoordinator> const& surface_coordinator)
  : input_targeter(input_targeter),
    surface_coordinator(surface_coordinator)
{
}

void msh::DefaultFocusMechanism::set_focus_to(std::shared_ptr<ms::Session> const& focus_session)
{
    // TODO: This path should be encapsulated in a seperate clear_focus message
    if (!focus_session)
    {
        input_targeter->focus_cleared();
        return;
    }

    auto surface = focus_session->default_surface();
    if (surface)
    {
        std::lock_guard<std::mutex> lg(surface_focus_lock);

        // Ensure the surface has really taken the focus before notifying it that it is focused
        surface_coordinator->raise(surface);
        surface->take_input_focus(input_targeter);

        auto current_focus = currently_focused_surface.lock();
        if (current_focus)
            current_focus->configure(mir_surface_attrib_focus, mir_surface_unfocused);
        surface->configure(mir_surface_attrib_focus, mir_surface_focused);
        currently_focused_surface = surface;
    }
    else
    {
        input_targeter->focus_cleared();
    }
}
