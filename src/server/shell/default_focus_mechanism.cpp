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

#include "mir/frontend/session.h"
#include "mir/shell/default_focus_mechanism.h"
#include "mir/shell/input_targeter.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"
#include "mir/shell/surface_controller.h"
#include "mir/shell/display_changer.h"

namespace mf = mir::frontend;
namespace msh = mir::shell;

msh::DefaultFocusMechanism::DefaultFocusMechanism(std::shared_ptr<msh::InputTargeter> const& input_targeter,
                                                  std::shared_ptr<msh::SurfaceController> const& surface_controller,
                                                  std::shared_ptr<msh::DisplayChanger> const& changer)
  : input_targeter(input_targeter),
    surface_controller(surface_controller),
    display_changer(changer)
{
}

void msh::DefaultFocusMechanism::set_focus_to(std::shared_ptr<Session> const& focus_session)
{
    // TODO: This path should be encapsulated in a seperate clear_focus message
    // TODO: also, no nullptr passing!
    if (!focus_session)
    {
        input_targeter->focus_cleared();
        return;
    }
    
    auto surface = focus_session->default_surface();
    if (surface)
    {
        display_changer->set_focus_to(focus_session);
        surface->raise(surface_controller);
        surface->take_input_focus(input_targeter);
    }
    else
    {
        input_targeter->focus_cleared();
    }
}
