/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_COMMON_INPUT_EVENT_H_
#define MIR_COMMON_INPUT_EVENT_H_

#include "mir/events/event.h"

struct MirInputEvent : MirEvent
{
    MirInputEventType input_type() const;
    MirInputDeviceId device_id() const;
    void set_device_id(MirInputDeviceId id);

    std::chrono::nanoseconds event_time() const;
    void set_event_time(std::chrono::nanoseconds const& event_time);

    std::vector<uint8_t> cookie() const;
    void set_cookie(std::vector<uint8_t> const& cookie);

    MirInputEventModifiers modifiers() const;
    void set_modifiers(MirInputEventModifiers mods);

    MirKeyboardEvent* to_keyboard();
    MirKeyboardEvent const* to_keyboard() const;

    MirPointerEvent* to_pointer();
    MirPointerEvent const* to_pointer() const;

    MirTouchEvent* to_touch();
    MirTouchEvent const* to_touch() const;

protected:
    MirInputEvent(MirInputDeviceId dev,
                  std::chrono::nanoseconds et,
                  MirInputEventModifiers mods,
                  std::vector<uint8_t> const& cookie);

    MirInputEvent() = default;
    MirInputEvent(MirInputEvent const& event) = default;
    MirInputEvent& operator=(MirInputEvent const& event) = default;
};

#endif /* MIR_COMMON_INPUT_EVENT_H_ */
