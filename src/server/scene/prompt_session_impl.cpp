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

namespace ms = mir::scene;

ms::PromptSessionImpl::PromptSessionImpl() :
    m_state(mir_prompt_session_state_stopped)
{
}

void ms::PromptSessionImpl::set_state(MirPromptSessionState state)
{
    std::unique_lock<std::mutex> lk(guard);
    m_state = state;
}

MirPromptSessionState ms::PromptSessionImpl::state() const
{
    std::unique_lock<std::mutex> lk(guard);
    return m_state;
}
