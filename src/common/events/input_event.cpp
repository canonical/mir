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

#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include "mir/events/pointer_event.h"
#include "mir/events/touch_event.h"

MirInputEvent::MirInputEvent(MirInputEventType input_type,
                             MirInputDeviceId dev,
                             std::chrono::nanoseconds et,
                             MirInputEventModifiers mods,
                             std::vector<uint8_t> const& cookie) :
    MirEvent{mir_event_type_input},
    input_type_{input_type},
    device_id_{dev},
    event_time_{et},
    cookie_{cookie},
    modifiers_{mods}
{
}

MirInputEventType MirInputEvent::input_type() const
{
    return input_type_;
}

int MirInputEvent::window_id() const
{
    return window_id_;
}

void MirInputEvent::set_window_id(int id)
{
    window_id_ = id;
}

MirInputDeviceId MirInputEvent::device_id() const
{
    return device_id_;
}

void MirInputEvent::set_device_id(MirInputDeviceId id)
{
    device_id_ = id;
}

MirKeyboardEvent* MirInputEvent::to_keyboard()
{
    return static_cast<MirKeyboardEvent*>(this);
}

MirKeyboardEvent const* MirInputEvent::to_keyboard() const
{
    return static_cast<MirKeyboardEvent const*>(this);
}

MirPointerEvent* MirInputEvent::to_pointer()
{
    return static_cast<MirPointerEvent*>(this);
}

MirPointerEvent const* MirInputEvent::to_pointer() const
{
    return static_cast<MirPointerEvent const*>(this);
}

MirTouchEvent* MirInputEvent::to_touch()
{
    return static_cast<MirTouchEvent*>(this);
}

MirTouchEvent const* MirInputEvent::to_touch() const
{
    return static_cast<MirTouchEvent const*>(this);
}

std::chrono::nanoseconds MirInputEvent::event_time() const
{
    return event_time_;
}

void MirInputEvent::set_event_time(std::chrono::nanoseconds const& event_time)
{
    event_time_ = event_time;
}

MirInputEventModifiers MirInputEvent::modifiers() const
{
    return modifiers_;
}

void MirInputEvent::set_modifiers(MirInputEventModifiers modifiers)
{
    modifiers_ = modifiers;
}

MirInputEvent::MirInputEvent(MirInputEventType input_type) :
    MirEvent{mir_event_type_input},
    input_type_{input_type}
{
}
