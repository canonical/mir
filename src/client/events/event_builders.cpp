/*
 * Copyright © 2015-2016 Canonical Ltd.
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

#include "mir/events/event_builders.h"

#define MIR_LOG_COMPONENT "event-builders"
#include "mir/log.h"

#include "mir/events/event_private.h"
#include "mir/events/surface_placement_event.h"
#include "mir/cookie/blob.h"
#include "mir/input/xkb_mapper.h"
#include "mir/input/keymap.h"

#include <string.h>

#include <boost/throw_exception.hpp>

#include <algorithm>
#include <stdexcept>

namespace mi = mir::input;
namespace mf = mir::frontend;
namespace mev = mir::events;
namespace geom = mir::geometry;

namespace
{

template<typename Type, typename... Args>
auto new_event(Args&&... args) -> Type*
{
    return new Type(std::forward<Args>(args)...);
}

template <class T>
mir::EventUPtr make_uptr_event(T* e)
{
    return mir::EventUPtr(e, ([](MirEvent* e) { delete reinterpret_cast<T*>(e); }));
}
}

mir::EventUPtr mev::make_event(mf::SurfaceId const& surface_id, MirOrientation orientation)
{
    auto e = new_event<MirOrientationEvent>();

    e->set_surface_id(surface_id.as_value());
    e->set_direction(orientation);

    return make_uptr_event(e);
}

mir::EventUPtr mev::make_event(MirPromptSessionState state)
{
    auto e = new_event<MirPromptSessionEvent>();

    e->set_new_state(state);

    return make_uptr_event(e);
}

mir::EventUPtr mev::make_event(mf::SurfaceId const& surface_id, geom::Size const& size)
{
    auto e = new_event<MirResizeEvent>();

    e->set_surface_id(surface_id.as_value());
    e->set_width(size.width.as_int());
    e->set_height(size.height.as_int());

    return make_uptr_event(e);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
mir::EventUPtr mev::make_event(mf::SurfaceId const& surface_id, MirSurfaceAttrib attribute, int value)
{
    auto e = new_event<MirSurfaceEvent>();

    e->set_id(surface_id.as_value());
    e->set_attrib(static_cast<MirWindowAttrib>(attribute));
    e->set_value(value);

    return make_uptr_event(e);
}
#pragma GCC diagnostic pop

mir::EventUPtr mev::make_event(mf::SurfaceId const& surface_id, MirWindowAttrib attribute, int value)
{
    auto e = new_event<MirWindowEvent>();

    e->set_id(surface_id.as_value());
    e->set_attrib(attribute);
    e->set_value(value);

    return make_uptr_event(e);
}

auto mev::make_start_drag_and_drop_event(frontend::SurfaceId const& surface_id, std::vector<uint8_t> const& handle)
    -> EventUPtr
{
    auto e = new_event<MirWindowEvent>();

    e->set_id(surface_id.as_value());
    e->set_attrib(mir_window_attribs);
    e->set_value(0);
    e->set_dnd_handle(handle);

    return make_uptr_event(e);

}

mir::EventUPtr mev::make_event(mf::SurfaceId const& surface_id)
{
    auto e = new_event<MirCloseWindowEvent>();

    e->set_surface_id(surface_id.as_value());

    return make_uptr_event(e);
}

mir::EventUPtr mev::make_event(
    mf::SurfaceId const& surface_id,
    int dpi,
    float scale,
    double refresh_rate,
    MirFormFactor form_factor,
    uint32_t output_id)
{
    auto e = new_event<MirWindowOutputEvent>();

    e->set_surface_id(surface_id.as_value());
    e->set_dpi(dpi);
    e->set_scale(scale);
    e->set_refresh_rate(refresh_rate);
    e->set_form_factor(form_factor);
    e->set_output_id(output_id);

    return make_uptr_event(e);
}

mir::EventUPtr mev::make_event(frontend::SurfaceId const& surface_id, geometry::Rectangle placement)
{
    auto e = new_event<MirWindowPlacementEvent>();

    e->set_id(surface_id.as_value());
    e->set_placement({
        placement.left().as_int(),
        placement.top().as_int(),
        placement.size.width.as_uint32_t(),
        placement.size.height.as_uint32_t()});

    return make_uptr_event(e);
}

mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& cookie, MirKeyboardAction action, xkb_keysym_t key_code,
    int scan_code, MirInputEventModifiers modifiers)
{
    auto e = new_event<MirKeyboardEvent>();

    e->set_device_id(device_id);
    e->set_event_time(timestamp);
    e->set_cookie(cookie);
    e->set_action(action);
    e->set_key_code(key_code);
    e->set_scan_code(scan_code);
    e->set_modifiers(modifiers);

    return make_uptr_event(e);
}

void mev::set_modifier(MirEvent& event, MirInputEventModifiers modifiers)
{
    if (event.type() == mir_event_type_input)
        event.to_input()->set_modifiers(modifiers);
    else
        BOOST_THROW_EXCEPTION(std::invalid_argument("Input event modifiers are only valid for input events."));
}

void mev::set_cursor_position(MirEvent& event, mir::geometry::Point const& pos)
{
    set_cursor_position(event, pos.x.as_int(), pos.y.as_int());
}

void mev::set_cursor_position(MirEvent& event, float x, float y)
{
    if (event.type() != mir_event_type_input ||
        event.to_input()->input_type() != mir_input_event_type_pointer)
        BOOST_THROW_EXCEPTION(std::invalid_argument("Cursor position is only valid for pointer events."));

    event.to_input()->to_pointer()->set_x(x);
    event.to_input()->to_pointer()->set_y(y);
}

void mev::set_button_state(MirEvent& event, MirPointerButtons button_state)
{
    if (event.type() != mir_event_type_input ||
        event.to_input()->input_type() != mir_input_event_type_pointer)
        BOOST_THROW_EXCEPTION(std::invalid_argument("Updating button state is only valid for pointer events."));

    event.to_input()->to_pointer()->set_buttons(button_state);
}

// Deprecated version with uint64_t mac
mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    uint64_t /*mac*/, MirKeyboardAction action, xkb_keysym_t key_code,
    int scan_code, MirInputEventModifiers modifiers)
{
    return make_event(device_id, timestamp, std::vector<uint8_t>{}, action, key_code, scan_code, modifiers);
}

// Deprecated version without mac
mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirKeyboardAction action, xkb_keysym_t key_code,
    int scan_code, MirInputEventModifiers modifiers)
{
    return make_event(device_id, timestamp, std::vector<uint8_t>{}, action, key_code, scan_code, modifiers);
}

mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& cookie, MirInputEventModifiers modifiers)
{
    auto e = new_event<MirTouchEvent>();

    e->set_device_id(device_id);
    e->set_event_time(timestamp);
    e->set_cookie(cookie);
    e->set_modifiers(modifiers);

    return make_uptr_event(e);
}

// Deprecated version with uint64_t mac
mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    uint64_t /*mac*/, MirInputEventModifiers modifiers)
{
    return make_event(device_id, timestamp, std::vector<uint8_t>{}, modifiers);
}

// Deprecated version without mac
mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirInputEventModifiers modifiers)
{
    return make_event(device_id, timestamp, std::vector<uint8_t>{}, modifiers);
}

void mev::add_touch(MirEvent &event, MirTouchId touch_id, MirTouchAction action,
    MirTouchTooltype tooltype, float x_axis_value, float y_axis_value,
    float pressure_value, float touch_major_value, float touch_minor_value, float)
{
    auto tev = event.to_input()->to_touch();
    auto current_index = tev->pointer_count();
    tev->set_pointer_count(current_index + 1);

    tev->set_id(current_index, touch_id);
    tev->set_tool_type(current_index, tooltype);
    tev->set_x(current_index, x_axis_value);
    tev->set_y(current_index, y_axis_value);
    tev->set_pressure(current_index, pressure_value);
    tev->set_touch_major(current_index, touch_major_value);
    tev->set_touch_minor(current_index, touch_minor_value);
    tev->set_action(current_index, action);
}

mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    std::vector<uint8_t> const& cookie, MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value,
    float relative_x_value, float relative_y_value)
{
    auto e = new_event<MirPointerEvent>(device_id, timestamp, modifiers, cookie, action, buttons_pressed, x_axis_value,
                                        y_axis_value, relative_x_value, relative_y_value, vscroll_value, hscroll_value);
    return make_uptr_event(e);
}

// Deprecated version with uint64_t mac
mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    uint64_t /*mac*/, MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value,
    float relative_x_value, float relative_y_value)
{
    return make_event(device_id, timestamp, std::vector<uint8_t>{}, modifiers, action,
                      buttons_pressed, x_axis_value, y_axis_value, hscroll_value,
                      vscroll_value, relative_x_value, relative_y_value);
}

// Deprecated version without mac
mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value,
    float relative_x_value, float relative_y_value)
{
    return make_event(device_id, timestamp, std::vector<uint8_t>{}, modifiers, action,
                      buttons_pressed, x_axis_value, y_axis_value, hscroll_value,
                      vscroll_value, relative_x_value, relative_y_value);
}

// Deprecated version without relative axis
mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    uint64_t /*mac*/, MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,                               
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value)
{
    return make_event(device_id, timestamp, std::vector<uint8_t>{}, modifiers, action, buttons_pressed,
                      x_axis_value, y_axis_value, hscroll_value, vscroll_value, 0, 0);
}

// Deprecated version without relative axis, and mac
mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirInputEventModifiers modifiers, MirPointerAction action,
    MirPointerButtons buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value)
{
    return make_event(device_id, timestamp, std::vector<uint8_t>{}, modifiers, action, buttons_pressed,
                      x_axis_value, y_axis_value, hscroll_value, vscroll_value, 0, 0);
}

mir::EventUPtr mev::make_event(mf::SurfaceId const& surface_id, MirInputDeviceId id, std::string const& model,
                               std::string const& layout, std::string const& variant, std::string const& options)
{
    auto e = new_event<MirKeymapEvent>();
    auto ep = make_uptr_event(e);

    auto ctx = mi::make_unique_context();
    auto map = mi::make_unique_keymap(ctx.get(), mi::Keymap{model, layout, variant, options});

    if (!map.get())
        BOOST_THROW_EXCEPTION(std::runtime_error("failed to assemble keymap from given parameters"));

    e->set_surface_id(surface_id.as_value());
    e->set_device_id(id);
    // TODO consider caching compiled keymaps
    auto buffer = xkb_keymap_get_as_string(map.get(), XKB_KEYMAP_FORMAT_TEXT_V1);
    e->set_buffer(buffer);
    std::free(buffer);

    return ep;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
mir::EventUPtr mev::make_event(MirInputConfigurationAction action, MirInputDeviceId id, std::chrono::nanoseconds time)
{
    auto e = new_event<MirInputConfigurationEvent>();

    e->set_action(action);
    e->set_when(time);
    e->set_id(id);

    return make_uptr_event(e);
}
#pragma GCC diagnostic pop

mir::EventUPtr mev::make_event(std::chrono::nanoseconds timestamp,
                               MirPointerButtons pointer_buttons,
                               MirInputEventModifiers modifiers,
                               float x_axis_value,
                               float y_axis_value,
                               std::vector<InputDeviceState>&& device_states)
{
    auto e = new_event<MirInputDeviceStateEvent>();
    e->set_when(timestamp);
    e->set_modifiers(modifiers);
    e->set_pointer_buttons(pointer_buttons);
    e->set_pointer_axis(mir_pointer_axis_x, x_axis_value);
    e->set_pointer_axis(mir_pointer_axis_y, y_axis_value);
    e->set_device_states(device_states);

    return make_uptr_event(e);
}

mir::EventUPtr mev::clone_event(MirEvent const& event)
{
    return make_uptr_event(new MirEvent(event));
}

void mev::transform_positions(MirEvent& event, mir::geometry::Displacement const& movement)
{
    if (event.type() == mir_event_type_input)
    {
        auto const input_type = event.to_input()->input_type();
        if (input_type == mir_input_event_type_pointer)
        {
            auto pev = event.to_input()->to_pointer();
            pev->set_x(pev->x() - movement.dx.as_int());
            pev->set_y(pev->y() - movement.dy.as_int());
        }
        else if (input_type == mir_input_event_type_touch)
        {
            auto tev = event.to_input()->to_touch();
            for (unsigned i = 0; i < tev->pointer_count(); i++)
            {
                auto x = tev->x(i);
                auto y = tev->y(i);
                tev->set_x(i, x - movement.dx.as_int());
                tev->set_y(i, y - movement.dy.as_int());
            }
        }
    }
}

mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
                               std::vector<uint8_t> const& cookie, MirInputEventModifiers modifiers,
                               std::vector<mev::ContactState> const& contacts)
{
    auto e = new_event<MirTouchEvent>(device_id, timestamp, cookie, modifiers, contacts);
    return make_uptr_event(e);
}

void mev::set_window_id(MirEvent& event, int window_id)
{
    switch (event.type())
    {
    case mir_event_type_input:
        event.to_input()->set_window_id(window_id);
        break;
    case mir_event_type_input_device_state:
        event.to_input_device_state()->set_window_id(window_id);
        break;
    case mir_event_type_window:
        event.to_surface()->set_id(window_id);
        break;
    case mir_event_type_resize:
        event.to_resize()->set_surface_id(window_id);
        break;
    case mir_event_type_orientation:
        event.to_orientation()->set_surface_id(window_id);
        break;
    case mir_event_type_close_window:
        event.to_close_window()->set_surface_id(window_id);
        break;
    case mir_event_type_keymap:
        event.to_keymap()->set_surface_id(window_id);
        break;
    case mir_event_type_window_output:
        event.to_window_output()->set_surface_id(window_id);
        break;
    case mir_event_type_window_placement:
        event.to_window_placement()->set_id(window_id);
        break;
    default:
        BOOST_THROW_EXCEPTION(std::invalid_argument("Event has no window id."));
    }
}

void mev::set_drag_and_drop_handle(MirEvent& event, std::vector<uint8_t> const& handle)
{
    if (event.type() == mir_event_type_input)
    {
        auto const input_event = event.to_input();
        if (mir_input_event_get_type(input_event) == mir_input_event_type_pointer)
            const_cast<MirPointerEvent*>(mir_input_event_get_pointer_event(input_event))->set_dnd_handle(handle);
    }
}

