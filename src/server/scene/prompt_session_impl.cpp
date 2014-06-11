/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "prompt_session_impl.h"
#include "mir/scene/session.h"
#include "mir/scene/prompt_session_listener.h"

namespace ms = mir::scene;

ms::PromptSessionImpl::PromptSessionImpl(std::weak_ptr<Session> const& session) :
    helper(session)
{
    if (auto const& helper_locked = helper.lock())
    {
        helper_locked->start_prompt_session();
    }
}

std::weak_ptr<ms::Session> ms::PromptSessionImpl::get_helper() const
{
    return helper;
}
