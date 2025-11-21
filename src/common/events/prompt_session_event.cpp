/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/events/prompt_session_event.h>

MirPromptSessionEvent::MirPromptSessionEvent() : MirEvent(mir_event_type_prompt_session_state_change)
{
}

MirPromptSessionState MirPromptSessionEvent::new_state() const
{
    return state;
}

void MirPromptSessionEvent::set_new_state(MirPromptSessionState state)
{
    this->state = state;
}

auto MirPromptSessionEvent::clone() const -> MirPromptSessionEvent*
{
    return new MirPromptSessionEvent(*this);
}
