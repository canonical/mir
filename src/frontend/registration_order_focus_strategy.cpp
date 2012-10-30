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

#include "mir/frontend/registration_order_focus_strategy.h"
#include "mir/frontend/application_session.h"

#include <memory>
#include <cassert>
#include <algorithm>


namespace mf = mir::frontend;

mf::RegistrationOrderFocusStrategy::RegistrationOrderFocusStrategy (std::shared_ptr<mf::ApplicationSessionContainer> container) : app_container(container)
{

}

std::weak_ptr<mf::ApplicationSession> mf::RegistrationOrderFocusStrategy::next_focus_app (std::shared_ptr<mf::ApplicationSession> focused_app)
{
    auto it = app_container->iterator();

    if (focused_app == NULL)
    {
        return **it;
    }

    bool found = false;

    while (it->is_valid())
    {
        auto stacked_app = **it;
        
        if (found) return stacked_app;

        if (stacked_app->get_name() == focused_app->get_name())
        {
            found = true;
        }
        it->advance();
    }
    // What if we didn't find the previous app?
    it->reset();

    return **it;
}

