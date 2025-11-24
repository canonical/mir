/*
 * Copyright © Canonical Ltd.
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
 */

#include "idle_poking_dispatcher.h"
#include <mir/events/event.h>
#include <mir/scene/idle_hub.h>

namespace mi = mir::input;

mi::IdlePokingDispatcher::IdlePokingDispatcher(
    std::shared_ptr<InputDispatcher> const& next_dispatcher,
    std::shared_ptr<scene::IdleHub> const& idle_hub)
    : next_dispatcher{next_dispatcher},
      idle_hub{idle_hub}
{
}

bool mi::IdlePokingDispatcher::dispatch(std::shared_ptr<MirEvent const> const& event)
{
    bool const result = next_dispatcher->dispatch(event);
    switch (mir_event_get_type(event.get()))
    {
    case mir_event_type_input:
        switch (mir_input_event_get_type(mir_event_get_input_event(event.get())))
        {
            case mir_input_event_type_key:
            case mir_input_event_type_pointer:
            case mir_input_event_type_touch:
                idle_hub->poke();
                break;

            case mir_input_event_type_keyboard_resync:
            case mir_input_event_types:
                break;
        }
        break;

    default:;
    }
    return result;
}

void mi::IdlePokingDispatcher::start()
{
    next_dispatcher->start();
}

void mi::IdlePokingDispatcher::stop()
{
    next_dispatcher->stop();
}
