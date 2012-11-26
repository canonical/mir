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

#include "mir/frontend/registration_order_focus_selection_strategy.h"
#include "mir/frontend/application_session.h"
#include "mir/frontend/session_container.h"

#include <memory>
#include <cassert>
#include <algorithm>


namespace mf = mir::frontend;

mf::RegistrationOrderFocusSequence::RegistrationOrderFocusSequence(std::shared_ptr<mf::SessionContainer> const& app_container) :
  session_container(app_container)
{

}

std::weak_ptr<mf::Session> mf::RegistrationOrderFocusSequence::successor_of(std::shared_ptr<mf::Session> const& focused_app)
{
    auto it = session_container->iterator();

    if (focused_app == NULL)
    {
        return **it;
    }

    bool found = false;

    while (it->is_valid())
    {
        auto stacked_app = **it;
        
        if (found) return stacked_app;

        if (stacked_app == focused_app)
        {
            found = true;
        }
        it->advance();
    }
    it->reset();
    // This is programmer error if not found...
    // later we can make this use LockingIterators?
    assert(found);

    return **it;
}

std::weak_ptr<mf::Session> mf::RegistrationOrderFocusSequence::predecessor_of(std::shared_ptr<mf::Session> const& focused_app)
{
    auto it = session_container->iterator();
    
    if (focused_app == NULL)
    {
        return **it;
    }
    
    std::weak_ptr<mf::Session> last_app = **it;

    it->advance();
    while (it->is_valid())
    {
        auto stacked_app = **it;
        
        if (stacked_app == focused_app)
        {
            return last_app;
        }
        last_app = stacked_app;
        it->advance();
    }
    // If we didn't focus_app it was the first app so we happily return the last app
    return last_app;
}
