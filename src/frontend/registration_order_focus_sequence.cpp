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

#include "mir/frontend/registration_order_focus_sequence.h"
#include "mir/frontend/session.h"
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
    std::shared_ptr<mf::Session> first;
    std::shared_ptr<mf::Session> result;
    bool found{false};

    session_container->for_each(
        [&](std::shared_ptr<mf::Session> const& session)
         {
             if (!first) first = session;

             if (found)
             {
                 if (!result) result = session;
             }
             else if (focused_app == session)
             {
                 found = true;
             }
         });

    if (result) return result;
    return first;
}

std::weak_ptr<mf::Session> mf::RegistrationOrderFocusSequence::predecessor_of(std::shared_ptr<mf::Session> const& focused_app)
{
    std::shared_ptr<mf::Session> last;
    std::shared_ptr<mf::Session> result;
    bool found{false};

    session_container->for_each(
        [&](std::shared_ptr<mf::Session> const& session)
        {
            last = session;

            if (focused_app == session)
            {
                found = true;
            }
            else if (!found)
            {
                result = session;
            }
        });

    if (result) return result;
    return last;
}
