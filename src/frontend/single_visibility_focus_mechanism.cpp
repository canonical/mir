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

#include "mir/frontend/session_container.h"
#include "mir/frontend/application_session.h"
#include "mir/frontend/application_surface_organiser.h"
#include "mir/frontend/single_visibility_focus_mechanism.h"

#include <memory>
#include <cassert>
#include <algorithm>

#include <stdio.h>


namespace mf = mir::frontend;
namespace ms = mir::surfaces;

mf::SingleVisibilityFocusMechanism::SingleVisibilityFocusMechanism(std::shared_ptr<mf::SessionContainer> const& app_container) :
  app_container(app_container)
{
}

void mf::SingleVisibilityFocusMechanism::set_focus_to(std::shared_ptr<mf::Session> const& focus_session)
{
    auto it = app_container->iterator();
    while (it->is_valid())
    {
        auto session = **it;
        if (session == focus_session)
        {
            session->show();
        }
        else
        {
            session->hide();
        }
        it->advance();
    }
}
