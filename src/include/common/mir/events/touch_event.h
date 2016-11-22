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

#ifndef MIR_COMMON_TOUCH_EVENT_H_
#define MIR_COMMON_TOUCH_EVENT_H_

#include <chrono>
#include <cstdint>

#include "mir/events/input_event.h"

struct MirTouchEvent : MirInputEvent
{
    MirTouchEvent();
    MirTouchEvent(MirInputDeviceId id,
                  std::chrono::nanoseconds timestamp,
                  std::vector<uint8_t> const& cookie,
                  MirInputEventModifiers modifiers,
                  std::vector<mir::events::ContactState> const& contacts);

    size_t pointer_count() const;
    void set_pointer_count(size_t count);

    int id(size_t index) const;
    void set_id(size_t index, int id);

    float x(size_t index) const;
    void set_x(size_t index, float x);

    float y(size_t index) const;
    void set_y(size_t index, float y);

    float touch_major(size_t index) const;
    void set_touch_major(size_t index, float major);

    float touch_minor(size_t index) const;
    void set_touch_minor(size_t index, float minor);

    float pressure(size_t index) const;
    void set_pressure(size_t index, float pressure);

    float orientation(size_t index) const;
    void set_orientation(size_t index, float orientation);

    MirTouchTooltype tool_type(size_t index) const;
    void set_tool_type(size_t index, MirTouchTooltype tool_type);

    MirTouchAction action(size_t index) const;
    void set_action(size_t index, MirTouchAction action);

private:
    void throw_if_out_of_bounds(size_t index) const;

};

#endif /* MIR_COMMON_TOUCH_EVENT_H */
