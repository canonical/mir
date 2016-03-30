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

#ifndef MIR_COMMON_MOTION_EVENT_H_
#define MIR_COMMON_MOTION_EVENT_H_

/* TODO: To the moon. */
#define MIR_INPUT_EVENT_MAX_POINTER_COUNT 16

#include <chrono>
#include <cstdint>

#include "mir/events/input_event.h"
#include "mir/cookie/blob.h"

struct MirMotionPointer
{
    int id;
    float x;
    float y;
    float dx;
    float dy;
    float touch_major;
    float touch_minor;
    float size;
    float pressure;
    float orientation;
    float vscroll;
    float hscroll;
    MirTouchTooltype tool_type;

    // TODO: We would like to store this as a MirTouchAction but we still encode pointer actions
    // here as well.
    int action;
};

struct MirMotionEvent : MirInputEvent
{
    MirMotionEvent();

    int32_t device_id() const;
    void set_device_id(int32_t id);

    int32_t source_id() const;
    void set_source_id(int32_t id);

    MirInputEventModifiers modifiers() const;
    void set_modifiers(MirInputEventModifiers modifiers);

    MirPointerButtons buttons() const;
    void set_buttons(MirPointerButtons buttons);

    std::chrono::nanoseconds event_time() const;
    void set_event_time(std::chrono::nanoseconds const& event_time);

    mir::cookie::Blob cookie() const;
    void set_cookie(mir::cookie::Blob const& blob);

    size_t pointer_count() const;
    void set_pointer_count(size_t count);

    int id(size_t index) const;
    void set_id(size_t index, int id);

    float x(size_t index) const;
    void set_x(size_t index, float x);

    float y(size_t index) const;
    void set_y(size_t index, float y);

    float dx(size_t index) const;
    void set_dx(size_t index, float dx);

    float dy(size_t index) const;
    void set_dy(size_t index, float dy);

    float touch_major(size_t index) const;
    void set_touch_major(size_t index, float major);

    float touch_minor(size_t index) const;
    void set_touch_minor(size_t index, float minor);

    float size(size_t index) const;
    void set_size(size_t index, float size);

    float pressure(size_t index) const;
    void set_pressure(size_t index, float pressure);

    float orientation(size_t index) const;
    void set_orientation(size_t index, float orientation);

    float vscroll(size_t index) const;
    void set_vscroll(size_t index, float vscroll);

    float hscroll(size_t index) const;
    void set_hscroll(size_t index, float hscroll);

    MirTouchTooltype tool_type(size_t index) const;
    void set_tool_type(size_t index, MirTouchTooltype tool_type);

    int action(size_t index) const;
    void set_action(size_t index, int action);

    MirTouchEvent* to_touch();
    MirTouchEvent const* to_touch() const;

    MirPointerEvent* to_pointer();
    MirPointerEvent const* to_pointer() const;

private:
    int32_t device_id_{0};
    int32_t source_id_{0};

    MirInputEventModifiers modifiers_{0};

    MirPointerButtons buttons_{0};
    std::chrono::nanoseconds event_time_{0};
    mir::cookie::Blob cookie_;

    size_t pointer_count_{0};
    MirMotionPointer pointer_coordinates_[MIR_INPUT_EVENT_MAX_POINTER_COUNT];
    /* "_coordinates" is a misnomer here because there's plenty more info than
       just coordinates, but renaming it accurately would be an API break */
};

// These are left empty as they are just aliases for a MirMotionEvent,
// but nice to be implicit cast to the base class
struct MirTouchEvent : MirMotionEvent
{
};

struct MirPointerEvent : MirMotionEvent
{
};

#endif /* MIR_COMMON_MOTION_EVENT_H_ */
