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

MirKeyboardEvent::MirKeyboardEvent() :
    MirInputEvent(mir_event_type_key)
{
}

int32_t MirKeyboardEvent::device_id() const
{
    return device_id_;
}

void MirKeyboardEvent::set_device_id(int32_t id)
{
    device_id_ = id;
}

int32_t MirKeyboardEvent::source_id() const
{
    return source_id_;
}

void MirKeyboardEvent::set_source_id(int32_t id)
{
    source_id_ = id;
}

MirKeyboardAction MirKeyboardEvent::action() const
{
    return action_;
}

void MirKeyboardEvent::set_action(MirKeyboardAction action)
{
    action_ = action;
}

MirInputEventModifiers MirKeyboardEvent::modifiers() const
{
    return modifiers_;
}

void MirKeyboardEvent::set_modifiers(MirInputEventModifiers modifiers)
{
    modifiers_ = modifiers;
}

int32_t MirKeyboardEvent::key_code() const
{
    return key_code_;
}

void MirKeyboardEvent::set_key_code(int32_t key_code)
{
    key_code_ = key_code;
}

int32_t MirKeyboardEvent::scan_code() const
{
    return scan_code_;
}

void MirKeyboardEvent::set_scan_code(int32_t scan_code)
{
    scan_code_ = scan_code;
}

std::chrono::nanoseconds MirKeyboardEvent::event_time() const
{
    return event_time_;
}

void MirKeyboardEvent::set_event_time(std::chrono::nanoseconds const& event_time)
{
    event_time_ = event_time;
}

mir::cookie::Blob MirKeyboardEvent::cookie() const
{
    return cookie_;
}

void MirKeyboardEvent::set_cookie(mir::cookie::Blob const& cookie)
{
    cookie_ = cookie;
}
