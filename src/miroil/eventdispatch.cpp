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

#include "miroil/eventdispatch.h"

#include "gesture_ender.h"

#include <miral/window.h>
#include <mir/scene/surface.h>
#include <mir/events/event_builders.h>
#include <mir/events/input_event.h>
#include <mir/events/event_helpers.h>

namespace mev = mir::events;
namespace geom = mir::geometry;

namespace
{
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

bool is_gesture_terminator(MirInputEvent const* event)
{
    if (mir_event_get_type(event) == mir_event_type_input)
    {
        auto const* iev = mir_event_get_input_event(event);

        if (auto const* pev = mir_input_event_get_pointer_event(iev))
        {
            bool any_pressed = false;
            for_pressed_buttons(pev, [&any_pressed](MirPointerButton){ any_pressed = true; });
            return !any_pressed && mir_pointer_event_action(pev) == mir_pointer_action_button_up;
        }
    }

    return false;
}
}

void miroil::dispatch_input_event(const miral::Window& window, MirInputEvent const* event)
{
    if (auto surface = std::shared_ptr<mir::scene::Surface>(window))
    {
        std::shared_ptr<MirEvent> const clone = mev::clone_event(*event);
        mev::map_positions(*clone, [&](auto global, auto local)
            {
                if (!local)
                {
                    // If local position is not set, local position is confunsingly being stored in global positon. This
                    // is a remnant of before Mir stored local and global positon in separate variables within the
                    // event.
                    local = global;
                    std::shared_ptr<mir::scene::Surface> const surface{window};
                    auto surface_displacement = geom::DisplacementF{as_displacement(surface->input_bounds().top_left)};
                    global = local.value() + surface_displacement;
                }
                return std::make_pair(global, local);
            });
        surface->consume(clone);
    }

    if (is_gesture_terminator(event)) end_gesture();
}
