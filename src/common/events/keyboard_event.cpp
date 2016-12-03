/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"

MirKeyboardEvent::MirKeyboardEvent()
{
    event.initKey();
}

int32_t MirKeyboardEvent::device_id() const
{
    return event.asReader().getKey().getDeviceId().getId();
}

void MirKeyboardEvent::set_device_id(int32_t id)
{
    event.getKey().getDeviceId().setId(id);
}

int32_t MirKeyboardEvent::source_id() const
{
    return event.asReader().getKey().getSourceId();
}

void MirKeyboardEvent::set_source_id(int32_t id)
{
    event.getKey().setSourceId(id);
}

MirKeyboardAction MirKeyboardEvent::action() const
{
    return static_cast<MirKeyboardAction>(event.asReader().getKey().getAction());
}

void MirKeyboardEvent::set_action(MirKeyboardAction action)
{
    event.getKey().setAction(static_cast<mir::capnp::KeyboardEvent::Action>(action));
}

MirInputEventModifiers MirKeyboardEvent::modifiers() const
{
    return event.asReader().getKey().getModifiers();
}

void MirKeyboardEvent::set_modifiers(MirInputEventModifiers modifiers)
{
    event.getKey().setModifiers(modifiers);
}

int32_t MirKeyboardEvent::key_code() const
{
    return event.asReader().getKey().getKeyCode();
}

void MirKeyboardEvent::set_key_code(int32_t key_code)
{
    event.getKey().setKeyCode(key_code);
}

int32_t MirKeyboardEvent::scan_code() const
{
    return event.asReader().getKey().getScanCode();
}

void MirKeyboardEvent::set_scan_code(int32_t scan_code)
{
    event.getKey().setScanCode(scan_code);
}

std::chrono::nanoseconds MirKeyboardEvent::event_time() const
{
    return std::chrono::nanoseconds{event.asReader().getKey().getEventTime().getCount()};
}

void MirKeyboardEvent::set_event_time(std::chrono::nanoseconds const& event_time)
{
    event.getKey().getEventTime().setCount(event_time.count());
}

std::vector<uint8_t> MirKeyboardEvent::cookie() const
{
    auto cookie = event.asReader().getKey().getCookie();
    std::vector<uint8_t> vec_cookie(cookie.size());
    std::copy(std::begin(cookie), std::end(cookie), std::begin(vec_cookie));

    return vec_cookie;
}

void MirKeyboardEvent::set_cookie(std::vector<uint8_t> const& cookie)
{
    ::capnp::Data::Reader cookie_data(cookie.data(), cookie.size());
    event.getKey().setCookie(cookie_data);
}
