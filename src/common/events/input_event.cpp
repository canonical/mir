/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include "mir/events/pointer_event.h"
#include "mir/events/touch_event.h"

#include <stdlib.h>

MirInputEvent::MirInputEvent(MirInputDeviceId dev,
                             std::chrono::nanoseconds et,
                             MirInputEventModifiers mods,
                             std::vector<uint8_t> const& cookie)
{
    event.initInput();
    auto input = event.getInput();
    input.getDeviceId().setId(dev);
    input.getEventTime().setCount(et.count());
    input.setModifiers(mods);

    ::capnp::Data::Reader cookie_data(cookie.data(), cookie.size());
    input.setCookie(cookie_data);
}

MirInputEventType MirInputEvent::input_type() const
{
    switch (event.asReader().getInput().which())
    {
    case mir::capnp::InputEvent::Which::KEY:
        return mir_input_event_type_key;
    case mir::capnp::InputEvent::Which::TOUCH:
        return mir_input_event_type_touch;
    case mir::capnp::InputEvent::Which::POINTER:
        return mir_input_event_type_pointer;
    default:
        abort();
    }
}

int MirInputEvent::window_id() const
{
    return event.asReader().getInput().getWindowId();
}

void MirInputEvent::set_window_id(int id)
{
    event.getInput().setWindowId(id);
}

MirInputDeviceId MirInputEvent::device_id() const
{
    return event.asReader().getInput().getDeviceId().getId();
}

void MirInputEvent::set_device_id(MirInputDeviceId id)
{
    event.getInput().getDeviceId().setId(id);
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
    return std::chrono::nanoseconds{event.asReader().getInput().getEventTime().getCount()};
}

void MirInputEvent::set_event_time(std::chrono::nanoseconds const& event_time)
{
    event.getInput().getEventTime().setCount(event_time.count());
}

std::vector<uint8_t> MirInputEvent::cookie() const
{
    auto cookie = event.asReader().getInput().getCookie();
    std::vector<uint8_t> vec_cookie(cookie.size());
    std::copy(std::begin(cookie), std::end(cookie), std::begin(vec_cookie));

    return vec_cookie;
}

void MirInputEvent::set_cookie(std::vector<uint8_t> const& cookie)
{
    ::capnp::Data::Reader cookie_data(cookie.data(), cookie.size());
    event.getInput().setCookie(cookie_data);
}

MirInputEventModifiers MirInputEvent::modifiers() const
{
    return event.asReader().getInput().getModifiers();
}

void MirInputEvent::set_modifiers(MirInputEventModifiers modifiers)
{
    event.getInput().setModifiers(modifiers);
}
