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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "prompt_session_impl.h"

#include "mir/scene/session.h"

namespace ms = mir::scene;

ms::PromptSessionImpl::PromptSessionImpl() :
    current_state(mir_prompt_session_state_stopped)
{
}

void ms::PromptSessionImpl::start(std::shared_ptr<Session> const& helper_session)
{
    std::lock_guard<std::mutex> lk(guard);

    if (current_state == mir_prompt_session_state_stopped)
    {
        current_state = mir_prompt_session_state_started;
        if (helper_session)
            helper_session->start_prompt_session();
    }
}

void ms::PromptSessionImpl::stop(std::shared_ptr<Session> const& helper_session)
{
    std::lock_guard<std::mutex> lk(guard);

    if (current_state != mir_prompt_session_state_stopped)
    {
        current_state = mir_prompt_session_state_stopped;
        if (helper_session)
            helper_session->stop_prompt_session();
    }
}

void ms::PromptSessionImpl::suspend(std::shared_ptr<Session> const& helper_session)
{
    std::lock_guard<std::mutex> lk(guard);

    if (current_state == mir_prompt_session_state_started)
    {
        current_state = mir_prompt_session_state_suspended;
        if (helper_session)
            helper_session->suspend_prompt_session();
    }
}

void ms::PromptSessionImpl::resume(std::shared_ptr<Session> const& helper_session)
{
    std::lock_guard<std::mutex> lk(guard);

    if (current_state == mir_prompt_session_state_suspended)
    {
        current_state = mir_prompt_session_state_started;
        if (helper_session)
            helper_session->resume_prompt_session();
    }
}

MirPromptSessionState ms::PromptSessionImpl::state() const
{
    return current_state;
}
