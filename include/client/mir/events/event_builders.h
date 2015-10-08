/*
 * Copyright © 2015 Canonical Ltd.
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
 * Author: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_EVENT_BUILDERS_H_
#define MIR_EVENT_BUILDERS_H_

#include "mir_toolkit/event.h"

#include "mir/geometry/size.h"
#include "mir/frontend/surface_id.h"

#include <memory>
#include <functional>
#include <chrono>

namespace mir
{
    typedef std::unique_ptr<MirEvent, void(*)(MirEvent*)> EventUPtr;
    
namespace events
{
// Surface orientation change event
EventUPtr make_event(frontend::SurfaceId const& surface_id, MirOrientation orientation);
// Prompt session state change event
EventUPtr make_event(MirPromptSessionState state);
// Surface resize event
EventUPtr make_event(frontend::SurfaceId const& surface_id, geometry::Size const& size);
// Surface configure event
EventUPtr make_event(frontend::SurfaceId const& surface_id, MirSurfaceAttrib attribute, int value);
// Close surface event
EventUPtr make_event(frontend::SurfaceId const& surface_id);
// Keymap event
EventUPtr make_event(frontend::SurfaceId const& surface_id, xkb_rule_names const& names);
// Surface output event
EventUPtr make_event(
    frontend::SurfaceId const& surface_id,
    int dpi,
    float scale,
    MirFormFactor form_factor);

// Key event
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    uint64_t mac, MirKeyboardAction action, xkb_keysym_t key_code,
    int scan_code, MirInputEventModifiers modifiers);

// Deprecated version without mac
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirKeyboardAction action, xkb_keysym_t key_code,
    int scan_code, MirInputEventModifiers modifiers) __attribute__ ((deprecated));

// Touch event
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    uint64_t mac, MirInputEventModifiers modifiers);

// Deprecated version without mac
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirInputEventModifiers modifiers) __attribute__ ((deprecated));

void add_touch(MirEvent &event, MirTouchId touch_id, MirTouchAction action,
    MirTouchTooltype tooltype, float x_axis_value, float y_axis_value,
    float pressure_value, float touch_major_value, float touch_minor_value, float size_value);

// Pointer event
// Deprecated version without relative axis
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    uint64_t mac, MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value) __attribute__ ((deprecated));

// Deprecated version without relative axis and mac
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value) __attribute__ ((deprecated));

EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    uint64_t mac, MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value,
    float relative_x_value, float relative_y_value);

// Deprecated version without mac
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value,
    float relative_x_value, float relative_y_value) __attribute__ ((deprecated));

// Input configuration event
EventUPtr make_event(MirInputConfigurationAction action,
    MirInputDeviceId id, std::chrono::nanoseconds time);
}
}

#endif // MIR_EVENT_BUILDERS_H_
