/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/shell/session_container.h"
#include "mir/frontend/session.h"
#include "mir/shell/single_visibility_focus_mechanism.h"
#include "mir/shell/input_focus_selector.h"

#include "mir/shell/session.h"
#include "surface.h" // TODO: Cleanup headers ~racarr

namespace mf = mir::frontend;
namespace msh = mir::shell;

msh::SingleVisibilityFocusMechanism::SingleVisibilityFocusMechanism(std::shared_ptr<msh::SessionContainer> const& app_container,
    std::shared_ptr<msh::InputFocusSelector> const& input_selector)
  : app_container(app_container),
    input_selector(input_selector)
{
}

void msh::SingleVisibilityFocusMechanism::set_focus_to(std::shared_ptr<msh::Session> const& focus_session)
{
    app_container->for_each(
        [&](std::shared_ptr<mf::Session> const& session) {
        if (session == focus_session)
        {
            session->show();

            input_selector->set_input_focus_to(focus_session, focus_session->default_surface());
        }
        else
        {
            session->hide();
        }
    });
}
