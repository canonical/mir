/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_COMMON_PROMPT_SESSION_EVENT_H_
#define MIR_COMMON_PROMPT_SESSION_EVENT_H_

#include <mir/events/event.h>

struct MirPromptSessionEvent : MirEvent
{
    MirPromptSessionEvent();
    auto clone() const -> MirPromptSessionEvent* override;

    MirPromptSessionState new_state() const;
    void set_new_state(MirPromptSessionState state);

private:
    MirPromptSessionState state = mir_prompt_session_state_stopped;
};

#endif /* MIR_COMMON_PROMPT_SESSION_EVENT_H_ */
