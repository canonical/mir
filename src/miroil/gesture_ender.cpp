/*
* Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gesture_ender.h"

#include <mir/frontend/pointer_input_dispatcher.h>
#include <mir/input/composite_event_filter.h>
#include <mir/input/event_filter.h>
#include <mir/server.h>

namespace
{
struct PointerInputDispatcher : mir::frontend::PointerInputDispatcher
{
    PointerInputDispatcher(mir::Server& server);
    void disable_dispatch_to_gesture_owner(std::function<void()> on_end_gesture) override;

    std::shared_ptr<mir::input::EventFilter> const event_filter;
};

struct EventFilter : mir::input::EventFilter
{
    bool handle(const MirEvent& event) override;
};

std::mutex mutex;

std::weak_ptr<mir::frontend::PointerInputDispatcher> pointer_input_dispatcher;
std::function<void()> on_end_gesture = []{};

PointerInputDispatcher::PointerInputDispatcher(mir::Server& server) : event_filter(std::make_shared<EventFilter>())
{
    server.the_composite_event_filter()->prepend(event_filter);
}

void PointerInputDispatcher::disable_dispatch_to_gesture_owner(std::function<void()> on_end_gesture)
{
    std::lock_guard lock{mutex};
    ::on_end_gesture = on_end_gesture;
}

void for_pressed_buttons(MirPointerEvent const* pev, std::function<void(MirPointerButton)> const& exec)
{
    auto const buttons = {
        mir_pointer_button_primary,
        mir_pointer_button_secondary,
        mir_pointer_button_tertiary,
        mir_pointer_button_back,
        mir_pointer_button_forward
    };
    for (auto button : buttons)
        if (mir_pointer_event_button_state(pev, button)) exec(button);
}

bool is_gesture_terminator(MirEvent const& event)
{
    if (mir_event_get_type(&event) == mir_event_type_input)
    {
        auto const* iev = mir_event_get_input_event(&event);

        if (mir_input_event_get_type(iev) == mir_input_event_type_pointer)
        {
            auto const* pev = mir_input_event_get_pointer_event(iev);
            bool any_pressed = false;
            for_pressed_buttons(pev, [&any_pressed](MirPointerButton){ any_pressed = true; });
            return !any_pressed && mir_pointer_event_action(pev) == mir_pointer_action_button_up;
        }
    }

    return false;
}

bool EventFilter::handle(const MirEvent& event)
{
    if (is_gesture_terminator(event))
    {
        decltype(on_end_gesture) end_gesture = []{};

        {
            std::lock_guard lock{mutex};
            std::swap(on_end_gesture, end_gesture);
        }

        end_gesture();
    }

    return false;
}
}

auto miroil::the_pointer_input_dispatcher(mir::Server& server) -> std::shared_ptr<mir::frontend::PointerInputDispatcher>
{
    std::lock_guard lock{mutex};

    if (auto result = pointer_input_dispatcher.lock())
    {
        return result;
    }
    else
    {
        result = std::make_shared<PointerInputDispatcher>(server);
        pointer_input_dispatcher = result;
        return result;
    }
}
