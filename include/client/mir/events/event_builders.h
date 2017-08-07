/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
#include "mir/geometry/point.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/displacement.h"
#include "mir/frontend/surface_id.h"
#include "mir/events/input_device_state.h"
#include "mir/events/contact_state.h"

#include <memory>
#include <functional>
#include <chrono>
#include <vector>

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
EventUPtr make_event(frontend::SurfaceId const& surface_id, MirSurfaceAttrib attribute, int value)
MIR_FOR_REMOVAL_IN_VERSION_1("use make_event with MirWindowAttribute instead");
// Window configure event
EventUPtr make_event(frontend::SurfaceId const& surface_id, MirWindowAttrib attribute, int value);
// Close surface event
EventUPtr make_event(frontend::SurfaceId const& surface_id);
// Keymap event
EventUPtr make_event(frontend::SurfaceId const& surface_id, MirInputDeviceId id, std::string const& model,
                     std::string const& layout, std::string const& variant, std::string const& options);
// Surface output event
EventUPtr make_event(
    frontend::SurfaceId const& surface_id,
    int dpi,
    float scale,
    double refresh_rate,
    MirFormFactor form_factor,
    uint32_t id);

/// Surface placement event
EventUPtr make_event(frontend::SurfaceId const& surface_id, geometry::Rectangle placement);

// Key event
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& cookie, MirKeyboardAction action, xkb_keysym_t key_code,
    int scan_code, MirInputEventModifiers modifiers);

void set_modifier(MirEvent& event, MirInputEventModifiers modifiers);
void set_cursor_position(MirEvent& event, mir::geometry::Point const& pos);
void set_cursor_position(MirEvent& event, float x, float y);
void set_button_state(MirEvent& event, MirPointerButtons button_state);

// Deprecated version with uint64_t mac
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    uint64_t mac, MirKeyboardAction action, xkb_keysym_t key_code,
    int scan_code, MirInputEventModifiers modifiers) MIR_FOR_REMOVAL_IN_VERSION_1("unused");

// Deprecated version without mac
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirKeyboardAction action, xkb_keysym_t key_code,
    int scan_code, MirInputEventModifiers modifiers) MIR_FOR_REMOVAL_IN_VERSION_1("unused");

// Touch event
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& mac, MirInputEventModifiers modifiers);

// Deprecated version with uint64_t mac
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    uint64_t mac, MirInputEventModifiers modifiers) MIR_FOR_REMOVAL_IN_VERSION_1("unused");

// Deprecated version without mac
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirInputEventModifiers modifiers) MIR_FOR_REMOVAL_IN_VERSION_1("unused");

void add_touch(MirEvent &event, MirTouchId touch_id, MirTouchAction action,
    MirTouchTooltype tooltype, float x_axis_value, float y_axis_value,
    float pressure_value, float touch_major_value, float touch_minor_value, float size_value);

// Pointer event
// Deprecated version without relative axis
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    uint64_t mac, MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value) MIR_FOR_REMOVAL_IN_VERSION_1("unused");

// Deprecated version without relative axis and mac
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value) MIR_FOR_REMOVAL_IN_VERSION_1("unused");

EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& mac, MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value,
    float relative_x_value, float relative_y_value);

// Deprecated version with uint64_t mac
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    uint64_t mac, MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value,
    float relative_x_value, float relative_y_value) MIR_FOR_REMOVAL_IN_VERSION_1("unused");

// Deprecated version without mac
EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value,
    float relative_x_value, float relative_y_value) MIR_FOR_REMOVAL_IN_VERSION_1("unused");

// Input configuration event
EventUPtr make_event(MirInputConfigurationAction action,
    MirInputDeviceId id, std::chrono::nanoseconds time) MIR_FOR_REMOVAL_IN_VERSION_1("unused");

EventUPtr make_event(std::chrono::nanoseconds timestamp,
                     MirPointerButtons pointer_buttons,
                     MirInputEventModifiers modifiers,
                     float x_axis_value,
                     float y_axis_value,
                     std::vector<InputDeviceState>&& device_states);

EventUPtr make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& mac, MirInputEventModifiers modifiers,
    std::vector<ContactState> const& contacts);

EventUPtr clone_event(MirEvent const& event);
void transform_positions(MirEvent& event, mir::geometry::Displacement const& movement);
void set_window_id(MirEvent& event, int window_id);

EventUPtr make_start_drag_and_drop_event(frontend::SurfaceId const& surface_id, std::vector<uint8_t> const& handle);
void set_drag_and_drop_handle(MirEvent& event, std::vector<uint8_t> const& handle);
}
}

#endif // MIR_EVENT_BUILDERS_H_
