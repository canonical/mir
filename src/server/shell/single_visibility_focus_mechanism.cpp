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
#include "mir/shell/session_container.h"
#include "mir/shell/single_visibility_focus_mechanism.h"
#include "mir/shell/input_targeter.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"

namespace mf = mir::frontend;
namespace msh = mir::shell;

msh::SingleVisibilityFocusMechanism::SingleVisibilityFocusMechanism(std::shared_ptr<msh::SessionContainer> const& app_container,
                                                                    std::shared_ptr<msh::InputTargeter> const& input_targeter)
  : app_container(app_container),
    input_targeter(input_targeter)
{
}

void msh::SingleVisibilityFocusMechanism::set_focus_to(std::shared_ptr<Session> const& focus_session)
{
    bool set_input_focus = false;
    app_container->for_each(
        [&](std::shared_ptr<mf::Session> const& session) {
        if (session == focus_session)
        {
            auto surface = focus_session->default_surface();
            if (surface)
            {
                surface->take_input_focus(input_targeter);
                set_input_focus = true;
            }
            session->show();
        }
        else
        {
            session->hide();
        }
    });

    if (set_input_focus == false)
        input_targeter->focus_cleared();
}
