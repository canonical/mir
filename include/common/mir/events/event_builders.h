/*
 * Copyright © Canonical Ltd.
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
#include "mir/events/scroll_axis.h"
#include "mir/events/touch_contact.h"

#include <memory>
#include <functional>
#include <chrono>
#include <vector>
#include <optional>

namespace mir
{
    typedef std::unique_ptr<MirEvent, void(*)(MirEvent*)> EventUPtr;
    
namespace events
{
// Surface orientation change event
EventUPtr make_surface_orientation_event(frontend::SurfaceId const& surface_id, MirOrientation orientation);
// Prompt session state change event
EventUPtr make_prompt_session_state_event(MirPromptSessionState state);
// Surface resize event
EventUPtr make_window_resize_event(frontend::SurfaceId const& surface_id, geometry::Size const& size);
// Window configure event
EventUPtr make_window_configure_event(frontend::SurfaceId const& surface_id, MirWindowAttrib attribute, int value);
// Close surface event
EventUPtr make_window_close_event(frontend::SurfaceId const& surface_id);

// Surface output event
EventUPtr make_window_output_event(
    frontend::SurfaceId const& surface_id,
    int dpi,
    float scale,
    double refresh_rate,
    MirFormFactor form_factor,
    uint32_t id);

/// Surface placement event
EventUPtr make_window_placement_event(frontend::SurfaceId const& surface_id, geometry::Rectangle placement);

// Key event
EventUPtr make_key_event(
    MirInputDeviceId device_id,
    std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& cookie,
    MirKeyboardAction action,
    xkb_keysym_t keysym,
    int scan_code,
    MirInputEventModifiers modifiers);

void set_modifier(MirEvent& event, MirInputEventModifiers modifiers);
void set_cursor_position(MirEvent& event, mir::geometry::Point const& pos);
void set_cursor_position(MirEvent& event, float x, float y);
void set_button_state(MirEvent& event, MirPointerButtons button_state);

// Touch event
EventUPtr make_touch_event(
    MirInputDeviceId device_id,
    std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& mac,
    MirInputEventModifiers modifiers);

void add_touch(
    MirEvent &event,
    MirTouchId touch_id,
    MirTouchAction action,
    MirTouchTooltype tooltype,
    float x_axis_value,
    float y_axis_value,
    float pressure_value,
    float touch_major_value,
    float touch_minor_value,
    float size_value);

// Pointer event with all properties, using Mir geometry types
EventUPtr make_pointer_event(
    MirInputDeviceId device_id,
    std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& cookie,
    MirInputEventModifiers mods,
    MirPointerAction action,
    MirPointerButtons buttons,
    std::optional<mir::geometry::PointF> position,
    mir::geometry::DisplacementF motion,
    MirPointerAxisSource axis_source,
    events::ScrollAxisH h_scroll,
    events::ScrollAxisV v_scroll);

// Pointer event
EventUPtr make_pointer_event(
    MirInputDeviceId device_id,
    std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& mac,
    MirInputEventModifiers modifiers,
    MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value,
    float y_axis_value,
    float hscroll_value,
    float vscroll_value,
    float relative_x_value,
    float relative_y_value);

// Pointer axis event
EventUPtr make_pointer_axis_event(
    MirPointerAxisSource axis_source,
    MirInputDeviceId device_id,
    std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& mac,
    MirInputEventModifiers modifiers,
    MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value,
    float y_axis_value,
    float hscroll_value,
    float vscroll_value,
    float relative_x_value,
    float relative_y_value);

// Pointer axis with stop event
EventUPtr make_pointer_axis_with_stop_event(
    MirPointerAxisSource axis_source,
    MirInputDeviceId device_id,
    std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& mac,
    MirInputEventModifiers modifiers,
    MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value,
    float y_axis_value,
    float hscroll_value,
    float vscroll_value,
    bool hscroll_stop,
    bool vscroll_stop,
    float relative_x_value,
    float relative_y_value);

// Pointer axis discrete scroll event
EventUPtr make_pointer_axis_discrete_scroll_event(
    MirPointerAxisSource axis_source,
    MirInputDeviceId device_id,
    std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& mac,
    MirInputEventModifiers modifiers,
    MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float hscroll_value,
    float vscroll_value,
    float hscroll_discrete,
    float vscroll_discrete);

// Input configuration event
EventUPtr make_input_configure_event(
    std::chrono::nanoseconds timestamp,
    MirPointerButtons pointer_buttons,
    MirInputEventModifiers modifiers,
    float x_axis_value,
    float y_axis_value,
    std::vector<InputDeviceState>&& device_states);

EventUPtr make_touch_event(
    MirInputDeviceId device_id,
    std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& mac,
    MirInputEventModifiers modifiers,
    std::vector<TouchContactV1> const& contacts);

EventUPtr make_touch_event(
    MirInputDeviceId device_id,
    std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& mac,
    MirInputEventModifiers modifiers,
    std::vector<TouchContact> const& contacts);

EventUPtr clone_event(MirEvent const& event);
void set_window_id(MirEvent& event, int window_id);

EventUPtr make_start_drag_and_drop_event(frontend::SurfaceId const& surface_id, std::vector<uint8_t> const& handle);
void set_drag_and_drop_handle(MirEvent& event, std::vector<uint8_t> const& handle);

[[deprecated("Internally functions from event_helpers.h should be used, externally this should not be needed")]]
void transform_positions(MirEvent& event, mir::geometry::Displacement const& movement);
[[deprecated("Internally functions from event_helpers.h should be used, externally this should not be needed")]]
void scale_positions(MirEvent& event, float scale);
}
}

#endif // MIR_EVENT_BUILDERS_H_
