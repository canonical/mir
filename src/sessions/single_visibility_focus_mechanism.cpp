/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/sessions/session_container.h"
#include "mir/sessions/session.h"
#include "mir/sessions/single_visibility_focus_mechanism.h"

namespace msess = mir::sessions;

msess::SingleVisibilityFocusMechanism::SingleVisibilityFocusMechanism(std::shared_ptr<msess::SessionContainer> const& app_container) :
  app_container(app_container)
{
}

void msess::SingleVisibilityFocusMechanism::set_focus_to(std::shared_ptr<msess::Session> const& focus_session)
{
    app_container->for_each(
        [&](std::shared_ptr<msess::Session> const& session) {
        if (session == focus_session)
        {
            session->show();
        }
        else
        {
            session->hide();
        }
    });
}
