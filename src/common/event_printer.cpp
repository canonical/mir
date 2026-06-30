/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/event_printer.h>

#include "events/event_private.h"

#include <mir/events/window_placement_event.h>
#include <mir/logging/input_timestamp.h>
#include <mir/time/steady_clock.h>

std::ostream& mir::operator<<(std::ostream& out, MirInputEventModifier modifier)
{
    bool none = true;
#define PRINT_FLAG(base,name) if ((modifier & base ## _ ## name) == base ## _ ## name)\
    {\
        if (!none) out << ' ';\
        out << # name;\
        none= false;\
    }

    PRINT_FLAG(mir_input_event_modifier, alt);
    PRINT_FLAG(mir_input_event_modifier, alt_right);
    PRINT_FLAG(mir_input_event_modifier, alt_left);
    PRINT_FLAG(mir_input_event_modifier, shift);
    PRINT_FLAG(mir_input_event_modifier, shift_left);
    PRINT_FLAG(mir_input_event_modifier, shift_right);
    PRINT_FLAG(mir_input_event_modifier, sym);
    PRINT_FLAG(mir_input_event_modifier, function);
    PRINT_FLAG(mir_input_event_modifier, ctrl);
    PRINT_FLAG(mir_input_event_modifier, ctrl_left);
    PRINT_FLAG(mir_input_event_modifier, ctrl_right);
    PRINT_FLAG(mir_input_event_modifier, meta);
    PRINT_FLAG(mir_input_event_modifier, meta_left);
    PRINT_FLAG(mir_input_event_modifier, meta_right);
    PRINT_FLAG(mir_input_event_modifier, caps_lock);
    PRINT_FLAG(mir_input_event_modifier, num_lock);
    PRINT_FLAG(mir_input_event_modifier, scroll_lock);
    if (none) out << "none";
#undef PRINT_FLAG
    return out;
}

#define PRINT(base,name) case base ## _ ## name: return out << #name;

std::ostream& mir::operator<<(std::ostream& out, MirKeyboardAction action)
{
    switch (action)
    {
    PRINT(mir_keyboard_action,up);
    PRINT(mir_keyboard_action,down);
    PRINT(mir_keyboard_action,repeat);
    default:
        return out << static_cast<int>(action) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirTouchAction action)
{
    switch (action)
    {
    PRINT(mir_touch_action,up);
    PRINT(mir_touch_action,down);
    PRINT(mir_touch_action,change);
    default:
        return out << static_cast<int>(action) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirTouchTooltype type)
{
    switch (type)
    {
    PRINT(mir_touch_tooltype,finger);
    PRINT(mir_touch_tooltype,stylus);
    PRINT(mir_touch_tooltype,unknown);
    default:
        return out << static_cast<int>(type) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirPointerAction action)
{
    switch (action)
    {
    PRINT(mir_pointer_action,button_up);
    PRINT(mir_pointer_action,button_down);
    PRINT(mir_pointer_action,enter);
    PRINT(mir_pointer_action,leave);
    PRINT(mir_pointer_action,motion);
    default:
        return out << static_cast<int>(action) << "<INVALID>";
    }
}


std::ostream& mir::operator<<(std::ostream& out, MirOrientation orientation)
{
    switch (orientation)
    {
    PRINT(mir_orientation,right);
    PRINT(mir_orientation,inverted);
    PRINT(mir_orientation,left);
    PRINT(mir_orientation,normal);
    default:
        return out << static_cast<int>(orientation) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowAttrib attribute)
{
    switch (attribute)
    {
    PRINT(mir_window_attrib,type);
    PRINT(mir_window_attrib,dpi);
    PRINT(mir_window_attrib,focus);
    PRINT(mir_window_attrib,state);
    PRINT(mir_window_attrib,visibility);
    PRINT(mir_window_attrib,preferred_orientation);
    default:
        return out << static_cast<int>(attribute) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowFocusState state)
{
    switch (state)
    {
    PRINT(mir_window_focus_state,focused);
    PRINT(mir_window_focus_state,unfocused);
    default:
        return out << static_cast<int>(state) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowVisibility state)
{
    switch (state)
    {
    PRINT(mir_window_visibility,exposed);
    PRINT(mir_window_visibility,occluded);
    default:
        return out << static_cast<int>(state) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowType type)
{
    switch (type)
    {
    PRINT(mir_window_type,normal);
    PRINT(mir_window_type,utility);
    PRINT(mir_window_type,dialog);
    PRINT(mir_window_type,gloss);
    PRINT(mir_window_type,freestyle);
    PRINT(mir_window_type,menu);
    PRINT(mir_window_type,inputmethod);
    PRINT(mir_window_type,satellite);
    PRINT(mir_window_type,tip);
    PRINT(mir_window_type,decoration);
    default:
        return out << static_cast<int>(type) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowState state)
{
    switch (state)
    {
    PRINT(mir_window_state,unknown);
    PRINT(mir_window_state,restored);
    PRINT(mir_window_state,minimized);
    PRINT(mir_window_state,maximized);
    PRINT(mir_window_state,vertmaximized);
    PRINT(mir_window_state,fullscreen);
    PRINT(mir_window_state,horizmaximized);
    PRINT(mir_window_state,hidden);
    PRINT(mir_window_state,attached);
    default:
        return out << static_cast<int>(state) << "<INVALID>";
    }
}

#undef PRINT

std::ostream& mir::operator<<(std::ostream& out, MirInputEvent const& event)
{
    auto event_time = mir::logging::input_timestamp(mir::time::SteadyClock{}, std::chrono::nanoseconds(mir_input_event_get_event_time(&event)));
    auto device_id = mir_input_event_get_device_id(&event);
    auto window_id = event.window_id();
    switch (mir_input_event_get_type(&event))
    {
    case mir_input_event_type_key:
        {
            auto key_event = mir_input_event_get_keyboard_event(&event);
            return out << "key_event(when=" << event_time << ", from=" << device_id << ", window_id=" << window_id << ", "
                << mir_keyboard_event_action(key_event)
                << ", code=" << mir_keyboard_event_keysym(key_event)
                << ", scan=" << mir_keyboard_event_scan_code(key_event) << ", modifiers=" << std::hex
                << static_cast<MirInputEventModifier>(mir_keyboard_event_modifiers(key_event)) << std::dec << ')';
        }
    case mir_input_event_type_touch:
        {
            auto touch_event = mir_input_event_get_touch_event(&event);
            out << "touch_event(when=" << event_time << ", from=" << device_id << ", window_id=" << window_id << ", touch = {";

            for (unsigned int index = 0, count = mir_touch_event_point_count(touch_event); index != count; ++index)
                out << "{id=" << mir_touch_event_id(touch_event, index)
                    << ", action=" << mir_touch_event_action(touch_event, index)
                    << ", tool=" << mir_touch_event_tooltype(touch_event, index)
                    << ", x=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_x)
                    << ", y=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_y)
                    << ", pressure=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_pressure)
                    << ", major=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_touch_major)
                    << ", minor=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_touch_minor)
                    << ", size=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_size) << '}';

            return out << ", modifiers=" << static_cast<MirInputEventModifier>(mir_touch_event_modifiers(touch_event)) << ')';
        }
    case mir_input_event_type_pointer:
        {
            auto pointer_event = mir_input_event_get_pointer_event(&event);
            unsigned int button_state = 0;

            for (auto const a : {mir_pointer_button_primary, mir_pointer_button_secondary, mir_pointer_button_tertiary,
                 mir_pointer_button_back, mir_pointer_button_forward})
                button_state |= mir_pointer_event_button_state(pointer_event, a) ? a : 0;

            return out << "pointer_event(when=" << event_time << ", from=" << device_id << ", window_id=" << window_id << ", "
                << mir_pointer_event_action(pointer_event) << ", button_state=" << button_state
                << ", x=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x)
                << ", y=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y)
                << ", dx=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_relative_x)
                << ", dy=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_relative_y)
                << ", vscroll=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_vscroll)
                << ", hscroll=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_hscroll)
                << ", modifiers=" << static_cast<MirInputEventModifier>(mir_pointer_event_modifiers(pointer_event)) << ')';
        }
    default:
        return out << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirResizeEvent const& event)
{
    return out << "resize_event(state=" << mir_resize_event_get_width(&event) << "x"
        << mir_resize_event_get_height(&event) << ")";
}

std::ostream& mir::operator<<(std::ostream& out, MirOrientationEvent const& event)
{
    return out << "orientation_event(" << mir_orientation_event_get_direction(&event) << ")";
}

std::ostream& mir::operator<<(std::ostream& out, MirCloseWindowEvent const&)
{
    return out << "close_window_event()";
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowEvent const& event)
{
    out << "window_event("
        << mir_window_event_get_attribute(&event)<< '=';
    auto value = mir_window_event_get_attribute_value(&event);
    switch (mir_window_event_get_attribute(&event))
    {
    case mir_window_attrib_type:
        out << static_cast<MirWindowType>(value);
        break;
    case mir_window_attrib_state:
        out << static_cast<MirWindowState>(value);
        break;
    case mir_window_attrib_focus:
        out << static_cast<MirWindowFocusState>(value);
        break;
    case mir_window_attrib_dpi:
        out << value;
        break;
    case mir_window_attrib_visibility:
        out << static_cast<MirWindowVisibility>(value);
        break;
    case mir_window_attrib_preferred_orientation:
        out << value;
        break;
    default:
        break;
    }

    return out << ')';
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowOutputEvent const& event)
{
    return out << "window_output_event({"
               << mir_window_output_event_get_dpi(&event) << ", "
               << mir_window_output_event_get_form_factor(&event) << ", "
               << mir_window_output_event_get_scale(&event) << ", "
               << mir_window_output_event_get_refresh_rate(&event) << ", "
               << mir_window_output_event_get_output_id(&event) << "})";
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowPlacementEvent const& event)
{
    auto const& placement = event.placement();
    return out << "window_placement_event({"
               << placement.left << ", "
               << placement.top << ", "
               << placement.width << ", "
               << placement.height << "})";
}

std::ostream& mir::operator<<(std::ostream& out, MirInputDeviceStateEvent const& event)
{
    auto window_id = event.window_id();
    out << "input_device_state(ts="
        << mir_input_device_state_event_time(&event)
        << ", window_id=" << window_id
        << ", mod=" << MirInputEventModifier(mir_input_device_state_event_modifiers(&event))
        << ", btns=" << mir_input_device_state_event_pointer_buttons(&event)
        << ", x=" << mir_input_device_state_event_pointer_axis(&event, mir_pointer_axis_x)
        << ", y=" << mir_input_device_state_event_pointer_axis(&event, mir_pointer_axis_y)
        << " [";

    for (size_t size = mir_input_device_state_event_device_count(&event), index = 0; index != size; ++index)
    {
        out << mir_input_device_state_event_device_id(&event, index)
            << " btns=" << mir_input_device_state_event_device_pointer_buttons(&event, index)
            << " pressed=(";
        auto key_count = mir_input_device_state_event_device_pressed_keys_count(&event, index);
        for (uint32_t i = 0; i < key_count; i++)
        {
            out << mir_input_device_state_event_device_pressed_keys_for_index(&event, index, i);
            if (i + 1 < key_count)
                out << ", ";
        }
        out << ")";
        if (index + 1 < size)
            out << ", ";
    }
    return out << "]";
}

#define PRINT_EVENT(type) case mir_event_type_ ## type : return out << *mir_event_get_ ## type ## _event(&event);

std::ostream& mir::operator<<(std::ostream& out, MirEvent const& event)
{
    auto type = mir_event_get_type(&event);
    switch (type)
    {
        PRINT_EVENT(window);
        PRINT_EVENT(resize);
        PRINT_EVENT(orientation);
        PRINT_EVENT(input);
        PRINT_EVENT(input_device_state);
        PRINT_EVENT(window_placement);
        PRINT_EVENT(window_output);
    case mir_event_type_close_window :
        return out << "close_window_event()";
    default:
        return out << static_cast<int>(type) << "<INVALID>";
    }
}

namespace
{
char const* form_factor_name(MirFormFactor form_factor)
{
    switch (form_factor)
    {
    case mir_form_factor_monitor:
        return "monitor";
    case mir_form_factor_projector:
        return "projector";
    case mir_form_factor_tv:
        return "TV";
    case mir_form_factor_phone:
        return "phone";
    case mir_form_factor_tablet:
        return "tablet";
    default:
        return "UNKNOWN";
    }
}

auto format_input_event_modifier(MirInputEventModifier modifier, std::format_context::iterator out)
    -> std::format_context::iterator
{
    bool none{true};
    auto const append = [&out, &none](char const* name)
    {
        if (!none)
            out = std::format_to(out, " ");
        out = std::format_to(out, "{}", name);
        none = false;
    };

    if ((modifier & mir_input_event_modifier_alt) == mir_input_event_modifier_alt) append("alt");
    if ((modifier & mir_input_event_modifier_alt_right) == mir_input_event_modifier_alt_right) append("alt_right");
    if ((modifier & mir_input_event_modifier_alt_left) == mir_input_event_modifier_alt_left) append("alt_left");
    if ((modifier & mir_input_event_modifier_shift) == mir_input_event_modifier_shift) append("shift");
    if ((modifier & mir_input_event_modifier_shift_left) == mir_input_event_modifier_shift_left) append("shift_left");
    if ((modifier & mir_input_event_modifier_shift_right) == mir_input_event_modifier_shift_right) append("shift_right");
    if ((modifier & mir_input_event_modifier_sym) == mir_input_event_modifier_sym) append("sym");
    if ((modifier & mir_input_event_modifier_function) == mir_input_event_modifier_function) append("function");
    if ((modifier & mir_input_event_modifier_ctrl) == mir_input_event_modifier_ctrl) append("ctrl");
    if ((modifier & mir_input_event_modifier_ctrl_left) == mir_input_event_modifier_ctrl_left) append("ctrl_left");
    if ((modifier & mir_input_event_modifier_ctrl_right) == mir_input_event_modifier_ctrl_right) append("ctrl_right");
    if ((modifier & mir_input_event_modifier_meta) == mir_input_event_modifier_meta) append("meta");
    if ((modifier & mir_input_event_modifier_meta_left) == mir_input_event_modifier_meta_left) append("meta_left");
    if ((modifier & mir_input_event_modifier_meta_right) == mir_input_event_modifier_meta_right) append("meta_right");
    if ((modifier & mir_input_event_modifier_caps_lock) == mir_input_event_modifier_caps_lock) append("caps_lock");
    if ((modifier & mir_input_event_modifier_num_lock) == mir_input_event_modifier_num_lock) append("num_lock");
    if ((modifier & mir_input_event_modifier_scroll_lock) == mir_input_event_modifier_scroll_lock) append("scroll_lock");

    return none ? std::format_to(out, "none") : out;
}

template<typename T>
auto format_enum(T value, std::format_context::iterator out) -> std::format_context::iterator
{
    return std::format_to(out, "{}<INVALID>", static_cast<int>(value));
}
}

auto std::formatter<MirInputEventModifier>::format(MirInputEventModifier modifier, std::format_context& ctx) const ->
    std::format_context::iterator
{
    return format_input_event_modifier(modifier, ctx.out());
}

auto std::formatter<MirKeyboardAction>::format(MirKeyboardAction action, std::format_context& ctx) const ->
    std::format_context::iterator
{
    switch (action)
    {
    case mir_keyboard_action_up:
        return std::format_to(ctx.out(), "up");
    case mir_keyboard_action_down:
        return std::format_to(ctx.out(), "down");
    case mir_keyboard_action_repeat:
        return std::format_to(ctx.out(), "repeat");
    default:
        return format_enum(action, ctx.out());
    }
}

auto std::formatter<MirTouchAction>::format(MirTouchAction action, std::format_context& ctx) const ->
    std::format_context::iterator
{
    switch (action)
    {
    case mir_touch_action_up:
        return std::format_to(ctx.out(), "up");
    case mir_touch_action_down:
        return std::format_to(ctx.out(), "down");
    case mir_touch_action_change:
        return std::format_to(ctx.out(), "change");
    default:
        return format_enum(action, ctx.out());
    }
}

auto std::formatter<MirTouchTooltype>::format(MirTouchTooltype tool, std::format_context& ctx) const ->
    std::format_context::iterator
{
    switch (tool)
    {
    case mir_touch_tooltype_finger:
        return std::format_to(ctx.out(), "finger");
    case mir_touch_tooltype_stylus:
        return std::format_to(ctx.out(), "stylus");
    case mir_touch_tooltype_unknown:
        return std::format_to(ctx.out(), "unknown");
    default:
        return format_enum(tool, ctx.out());
    }
}

auto std::formatter<MirPointerAction>::format(MirPointerAction action, std::format_context& ctx) const ->
    std::format_context::iterator
{
    switch (action)
    {
    case mir_pointer_action_button_up:
        return std::format_to(ctx.out(), "button_up");
    case mir_pointer_action_button_down:
        return std::format_to(ctx.out(), "button_down");
    case mir_pointer_action_enter:
        return std::format_to(ctx.out(), "enter");
    case mir_pointer_action_leave:
        return std::format_to(ctx.out(), "leave");
    case mir_pointer_action_motion:
        return std::format_to(ctx.out(), "motion");
    default:
        return format_enum(action, ctx.out());
    }
}

auto std::formatter<MirOrientation>::format(MirOrientation orientation, std::format_context& ctx) const ->
    std::format_context::iterator
{
    switch (orientation)
    {
    case mir_orientation_right:
        return std::format_to(ctx.out(), "right");
    case mir_orientation_inverted:
        return std::format_to(ctx.out(), "inverted");
    case mir_orientation_left:
        return std::format_to(ctx.out(), "left");
    case mir_orientation_normal:
        return std::format_to(ctx.out(), "normal");
    default:
        return format_enum(orientation, ctx.out());
    }
}

auto std::formatter<MirWindowAttrib>::format(MirWindowAttrib attribute, std::format_context& ctx) const ->
    std::format_context::iterator
{
    switch (attribute)
    {
    case mir_window_attrib_type:
        return std::format_to(ctx.out(), "type");
    case mir_window_attrib_dpi:
        return std::format_to(ctx.out(), "dpi");
    case mir_window_attrib_focus:
        return std::format_to(ctx.out(), "focus");
    case mir_window_attrib_state:
        return std::format_to(ctx.out(), "state");
    case mir_window_attrib_visibility:
        return std::format_to(ctx.out(), "visibility");
    case mir_window_attrib_preferred_orientation:
        return std::format_to(ctx.out(), "preferred_orientation");
    default:
        return format_enum(attribute, ctx.out());
    }
}

auto std::formatter<MirWindowFocusState>::format(MirWindowFocusState state, std::format_context& ctx) const ->
    std::format_context::iterator
{
    switch (state)
    {
    case mir_window_focus_state_focused:
        return std::format_to(ctx.out(), "focused");
    case mir_window_focus_state_unfocused:
        return std::format_to(ctx.out(), "unfocused");
    default:
        return format_enum(state, ctx.out());
    }
}

auto std::formatter<MirWindowVisibility>::format(MirWindowVisibility state, std::format_context& ctx) const ->
    std::format_context::iterator
{
    switch (state)
    {
    case mir_window_visibility_exposed:
        return std::format_to(ctx.out(), "exposed");
    case mir_window_visibility_occluded:
        return std::format_to(ctx.out(), "occluded");
    default:
        return format_enum(state, ctx.out());
    }
}

auto std::formatter<MirWindowType>::format(MirWindowType type, std::format_context& ctx) const ->
    std::format_context::iterator
{
    switch (type)
    {
    case mir_window_type_normal:
        return std::format_to(ctx.out(), "normal");
    case mir_window_type_utility:
        return std::format_to(ctx.out(), "utility");
    case mir_window_type_dialog:
        return std::format_to(ctx.out(), "dialog");
    case mir_window_type_gloss:
        return std::format_to(ctx.out(), "gloss");
    case mir_window_type_freestyle:
        return std::format_to(ctx.out(), "freestyle");
    case mir_window_type_menu:
        return std::format_to(ctx.out(), "menu");
    case mir_window_type_inputmethod:
        return std::format_to(ctx.out(), "inputmethod");
    case mir_window_type_satellite:
        return std::format_to(ctx.out(), "satellite");
    case mir_window_type_tip:
        return std::format_to(ctx.out(), "tip");
    case mir_window_type_decoration:
        return std::format_to(ctx.out(), "decoration");
    default:
        return format_enum(type, ctx.out());
    }
}

auto std::formatter<MirWindowState>::format(MirWindowState state, std::format_context& ctx) const ->
    std::format_context::iterator
{
    switch (state)
    {
    case mir_window_state_unknown:
        return std::format_to(ctx.out(), "unknown");
    case mir_window_state_restored:
        return std::format_to(ctx.out(), "restored");
    case mir_window_state_minimized:
        return std::format_to(ctx.out(), "minimized");
    case mir_window_state_maximized:
        return std::format_to(ctx.out(), "maximized");
    case mir_window_state_vertmaximized:
        return std::format_to(ctx.out(), "vertmaximized");
    case mir_window_state_fullscreen:
        return std::format_to(ctx.out(), "fullscreen");
    case mir_window_state_horizmaximized:
        return std::format_to(ctx.out(), "horizmaximized");
    case mir_window_state_hidden:
        return std::format_to(ctx.out(), "hidden");
    case mir_window_state_attached:
        return std::format_to(ctx.out(), "attached");
    default:
        return format_enum(state, ctx.out());
    }
}

auto std::formatter<MirResizeEvent>::format(MirResizeEvent const& event, std::format_context& ctx) const ->
    std::format_context::iterator
{
    return std::format_to(ctx.out(), "resize_event(state={}x{})", mir_resize_event_get_width(&event), mir_resize_event_get_height(&event));
}

auto std::formatter<MirOrientationEvent>::format(MirOrientationEvent const& event, std::format_context& ctx) const ->
    std::format_context::iterator
{
    return std::format_to(ctx.out(), "orientation_event({})", mir_orientation_event_get_direction(&event));
}

auto std::formatter<MirInputEvent>::format(MirInputEvent const& event, std::format_context& ctx) const ->
    std::format_context::iterator
{
    auto event_time = mir::logging::input_timestamp(mir::time::SteadyClock{}, std::chrono::nanoseconds(mir_input_event_get_event_time(&event)));
    auto const device_id = mir_input_event_get_device_id(&event);
    auto const window_id = event.window_id();

    switch (mir_input_event_get_type(&event))
    {
    case mir_input_event_type_key:
    {
        auto const key_event = mir_input_event_get_keyboard_event(&event);
        return std::format_to(
            ctx.out(),
            "key_event(when={}, from={}, window_id={}, {}, code={}, scan={}, modifiers={})",
            event_time,
            device_id,
            window_id,
            mir_keyboard_event_action(key_event),
            mir_keyboard_event_keysym(key_event),
            mir_keyboard_event_scan_code(key_event),
            static_cast<MirInputEventModifier>(mir_keyboard_event_modifiers(key_event)));
    }
    case mir_input_event_type_touch:
    {
        auto const touch_event = mir_input_event_get_touch_event(&event);
        auto out = std::format_to(
            ctx.out(),
            "touch_event(when={}, from={}, window_id={}, touch = {{",
            event_time,
            device_id,
            window_id);

        for (unsigned int index = 0, count = mir_touch_event_point_count(touch_event); index != count; ++index)
        {
            out = std::format_to(
                out,
                "{{id={}, action={}, tool={}, x={}, y={}, pressure={}, major={}, minor={}, size={}}}",
                mir_touch_event_id(touch_event, index),
                mir_touch_event_action(touch_event, index),
                mir_touch_event_tooltype(touch_event, index),
                mir_touch_event_axis_value(touch_event, index, mir_touch_axis_x),
                mir_touch_event_axis_value(touch_event, index, mir_touch_axis_y),
                mir_touch_event_axis_value(touch_event, index, mir_touch_axis_pressure),
                mir_touch_event_axis_value(touch_event, index, mir_touch_axis_touch_major),
                mir_touch_event_axis_value(touch_event, index, mir_touch_axis_touch_minor),
                mir_touch_event_axis_value(touch_event, index, mir_touch_axis_size));
        }

        return std::format_to(out, ", modifiers={})", static_cast<MirInputEventModifier>(mir_touch_event_modifiers(touch_event)));
    }
    case mir_input_event_type_pointer:
    {
        auto const pointer_event = mir_input_event_get_pointer_event(&event);
        unsigned int button_state = 0;

        for (auto const a : {mir_pointer_button_primary, mir_pointer_button_secondary, mir_pointer_button_tertiary,
             mir_pointer_button_back, mir_pointer_button_forward})
            button_state |= mir_pointer_event_button_state(pointer_event, a) ? a : 0;

        return std::format_to(
            ctx.out(),
            "pointer_event(when={}, from={}, window_id={}, {}, button_state={}, x={}, y={}, dx={}, dy={}, vscroll={}, hscroll={}, modifiers={})",
            event_time,
            device_id,
            window_id,
            mir_pointer_event_action(pointer_event),
            button_state,
            mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x),
            mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y),
            mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_relative_x),
            mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_relative_y),
            mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_vscroll),
            mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_hscroll),
            static_cast<MirInputEventModifier>(mir_pointer_event_modifiers(pointer_event)));
    }
    default:
        return std::format_to(ctx.out(), "<INVALID>");
    }
}

auto std::formatter<MirCloseWindowEvent>::format(MirCloseWindowEvent const&, std::format_context& ctx) const ->
    std::format_context::iterator
{
    return std::format_to(ctx.out(), "close_window_event()");
}

auto std::formatter<MirWindowEvent>::format(MirWindowEvent const& event, std::format_context& ctx) const ->
    std::format_context::iterator
{
    auto out = std::format_to(ctx.out(), "window_event({}=", mir_window_event_get_attribute(&event));
    auto const value = mir_window_event_get_attribute_value(&event);

    switch (mir_window_event_get_attribute(&event))
    {
    case mir_window_attrib_type:
        out = std::format_to(out, "{}", static_cast<MirWindowType>(value));
        break;
    case mir_window_attrib_state:
        out = std::format_to(out, "{}", static_cast<MirWindowState>(value));
        break;
    case mir_window_attrib_focus:
        out = std::format_to(out, "{}", static_cast<MirWindowFocusState>(value));
        break;
    case mir_window_attrib_dpi:
        out = std::format_to(out, "{}", value);
        break;
    case mir_window_attrib_visibility:
        out = std::format_to(out, "{}", static_cast<MirWindowVisibility>(value));
        break;
    case mir_window_attrib_preferred_orientation:
        out = std::format_to(out, "{}", value);
        break;
    default:
        break;
    }

    return std::format_to(out, ")");
}

auto std::formatter<MirInputDeviceStateEvent>::format(MirInputDeviceStateEvent const& event, std::format_context& ctx) const ->
    std::format_context::iterator
{
    auto const window_id = event.window_id();
    auto out = std::format_to(
        ctx.out(),
        "input_device_state(ts={}, window_id={}, mod={}, btns={}, x={}, y={} [",
        mir_input_device_state_event_time(&event),
        window_id,
        static_cast<MirInputEventModifier>(mir_input_device_state_event_modifiers(&event)),
        mir_input_device_state_event_pointer_buttons(&event),
        mir_input_device_state_event_pointer_axis(&event, mir_pointer_axis_x),
        mir_input_device_state_event_pointer_axis(&event, mir_pointer_axis_y));

    for (size_t size = mir_input_device_state_event_device_count(&event), index = 0; index != size; ++index)
    {
        out = std::format_to(
            out,
            "{} btns={} pressed=(",
            mir_input_device_state_event_device_id(&event, index),
            mir_input_device_state_event_device_pointer_buttons(&event, index));
        auto const key_count = mir_input_device_state_event_device_pressed_keys_count(&event, index);
        for (uint32_t i = 0; i < key_count; i++)
        {
            out = std::format_to(out, "{}", mir_input_device_state_event_device_pressed_keys_for_index(&event, index, i));
            if (i + 1 < key_count)
                out = std::format_to(out, ", ");
        }
        out = std::format_to(out, ")");
        if (index + 1 < size)
            out = std::format_to(out, ", ");
    }

    return std::format_to(out, "]");
}

auto std::formatter<MirWindowPlacementEvent>::format(MirWindowPlacementEvent const& event, std::format_context& ctx) const ->
    std::format_context::iterator
{
    auto const& placement = event.placement();
    return std::format_to(
        ctx.out(),
        "window_placement_event({{{}, {}, {}, {}}})",
        placement.left,
        placement.top,
        placement.width,
        placement.height);
}

auto std::formatter<MirWindowOutputEvent>::format(MirWindowOutputEvent const& event, std::format_context& ctx) const ->
    std::format_context::iterator
{
    return std::format_to(
        ctx.out(),
        "window_output_event({{{}, {}, {}, {}, {}}})",
        mir_window_output_event_get_dpi(&event),
        form_factor_name(mir_window_output_event_get_form_factor(&event)),
        mir_window_output_event_get_scale(&event),
        mir_window_output_event_get_refresh_rate(&event),
        mir_window_output_event_get_output_id(&event));
}

auto std::formatter<MirEvent>::format(MirEvent const& event, std::format_context& ctx) const ->
    std::format_context::iterator
{
    auto const type = mir_event_get_type(&event);
    switch (type)
    {
    case mir_event_type_window:
        return std::format_to(ctx.out(), "{}", *mir_event_get_window_event(&event));
    case mir_event_type_resize:
        return std::format_to(ctx.out(), "{}", *mir_event_get_resize_event(&event));
    case mir_event_type_orientation:
        return std::format_to(ctx.out(), "{}", *mir_event_get_orientation_event(&event));
    case mir_event_type_input:
        return std::format_to(ctx.out(), "{}", *mir_event_get_input_event(&event));
    case mir_event_type_input_device_state:
        return std::format_to(ctx.out(), "{}", *mir_event_get_input_device_state_event(&event));
    case mir_event_type_window_placement:
        return std::format_to(ctx.out(), "{}", *mir_event_get_window_placement_event(&event));
    case mir_event_type_window_output:
        return std::format_to(ctx.out(), "{}", *mir_event_get_window_output_event(&event));
    case mir_event_type_close_window:
        return std::format_to(ctx.out(), "close_window_event()");
    default:
        return std::format_to(ctx.out(), "{}<INVALID>", static_cast<int>(type));
    }
}
