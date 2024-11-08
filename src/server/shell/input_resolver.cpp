/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "mir/shell/input_resolver.h"

#include "mir/events/pointer_event.h"
#include "mir/events/touch_event.h"
#include "mir_toolkit/events/event.h"


namespace msd = mir::shell::decoration;

void msd::InputResolver::handle_input_event(std::shared_ptr<MirEvent const> const& event)
{
    MirInputEvent const* const input_ev = mir_event_get_input_event(event.get());
    switch (mir_input_event_get_type(input_ev))
    {
    case mir_input_event_type_pointer:
        {
            MirPointerEvent const* const pointer_ev = mir_input_event_get_pointer_event(input_ev);
            switch (mir_pointer_event_action(pointer_ev))
            {
            case mir_pointer_action_button_up:
            case mir_pointer_action_button_down:
            case mir_pointer_action_motion:
            case mir_pointer_action_enter:
                {
                    if (auto const position = pointer_ev->local_position())
                    {
                        bool pressed = mir_pointer_event_button_state(pointer_ev, mir_pointer_button_primary);
                        pointer_event(event, geometry::Point{position.value()}, pressed);
                    }
                }
                break;
            case mir_pointer_action_leave:
                {
                    pointer_leave(event);
                }
                break;
            case mir_pointer_actions:
                break;
            }
        }
        break;

    case mir_input_event_type_touch:
        {
            MirTouchEvent const* const touch_ev = mir_input_event_get_touch_event(input_ev);
            for (unsigned int i = 0; i < mir_touch_event_point_count(touch_ev); i++)
            {
                auto const id = mir_touch_event_id(touch_ev, i);
                switch (mir_touch_event_action(touch_ev, i))
                {
                case mir_touch_action_down:
                case mir_touch_action_change:
                    {
                        if (auto const position = touch_ev->local_position(i))
                        {
                            touch_event(event, id, geometry::Point{position.value()});
                        }
                    }
                    break;
                case mir_touch_action_up:
                    {
                        touch_up(event, id);
                        break;
                    }
                case mir_touch_actions:
                    break;
                }
            }
        }
        break;

    case mir_input_event_type_key:
    case mir_input_event_type_keyboard_resync:
    case mir_input_event_types:
        break;
    }
}

auto msd::InputResolver::latest_event() -> std::shared_ptr<MirEvent const> const
{
    return latest_event_;
}

void msd::InputResolver::pointer_event(std::shared_ptr<MirEvent const> const& event, geometry::Point location, bool pressed)
{
    std::lock_guard lock{mutex};
    latest_event_ = event;
    if (!pointer)
    {
        pointer = DeviceEvent{location, pressed};
        process_enter(pointer.value());
    }
    if (pointer.value().location != location)
    {
        pointer.value().location = location;
        if (pointer.value().pressed)
            process_drag(pointer.value());
        else
            process_move(pointer.value());
    }
    if (pointer.value().pressed != pressed)
    {
        pointer.value().pressed = pressed;
        if (pressed)
            process_down();
        else
            process_up();
    }
}

void msd::InputResolver::pointer_leave(std::shared_ptr<MirEvent const> const& event)
{
    std::lock_guard lock{mutex};
    latest_event_ = event;
    if (pointer)
        process_leave();
    pointer = std::nullopt;
}

void msd::InputResolver::touch_event(std::shared_ptr<MirEvent const> const& event, int32_t id, geometry::Point location)
{
    std::lock_guard lock{mutex};
    latest_event_ = event;
    auto device = touches.find(id);
    if (device == touches.end())
    {
        device = touches.insert(std::make_pair(id, DeviceEvent{location, false})).first;
        process_enter(device->second);
        device->second.pressed = true;
        process_down();
    }
    if (device->second.location != location)
    {
        device->second.location = location;
        process_drag(device->second);
    }
}

void msd::InputResolver::touch_up(std::shared_ptr<MirEvent const> const& event, int32_t id)
{
    std::lock_guard lock{mutex};
    latest_event_ = event;
    auto device = touches.find(id);
    if (device != touches.end())
    {
        process_up();
        process_leave();
        touches.erase(device);
    }
}

