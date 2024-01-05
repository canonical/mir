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

#define MIR_LOG_COMPONENT "event-access"

#include "handle_event_exception.h"

#include "mir_toolkit/events/event_type_to_string.h"
#include "mir/log.h"

#include "mir_toolkit/events/event.h"
#include "mir/events/event_private.h"
#include "mir/events/window_placement_event.h"

#include "mir_toolkit/events/resize_event.h"
#include "mir_toolkit/events/prompt_session_event.h"
#include "mir_toolkit/events/orientation_event.h"
#include "mir_toolkit/events/input_device_state_event.h"

#include <cstdlib>
#include <cstring>

namespace ml = mir::logging;

namespace
{
template <typename EventType>
void expect_event_type(EventType const* ev, MirEventType t) MIR_HANDLE_EVENT_EXCEPTION(
{
    auto type = ev->type();
    if (type != t)
    {
        mir::log_critical("Expected " + mir::event_type_to_string(t) + " but event is of type " +
            mir::event_type_to_string(type));
        abort();
    }
})

void expect_index_in_range(size_t size, size_t index)
{
    if (index >= size)
    {
        mir::log_critical("Index out of range in event data access");
        abort();
    }
}
}

std::string mir::event_type_to_string(MirEventType t)
{
    switch (t)
    {
    case mir_event_type_window:
        return "mir_event_type_window";
    case mir_event_type_resize:
        return "mir_event_type_resize";
    case mir_event_type_prompt_session_state_change:
        return "mir_event_type_prompt_session_state_change";
    case mir_event_type_orientation:
        return "mir_event_type_orientation";
    case mir_event_type_close_window:
        return "mir_event_type_close_window";
    case mir_event_type_input:
        return "mir_event_type_input";
    case mir_event_type_input_device_state:
        return "mir_event_type_input_device_state";
    case mir_event_type_window_output:
        return "mir_event_type_window_output";
    default:
        abort();
    }
}


MirEventType mir_event_get_type(MirEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    return ev->type();
})

MirInputEvent const* mir_event_get_input_event(MirEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    auto const type = ev->type();
    if (type != mir_event_type_input)
    {
        mir::log_critical("Expected input event but event is of type " +
            mir::event_type_to_string(type));
        abort();
    }

    return ev->to_input();
})

MirWindowEvent const* mir_event_get_window_event(MirEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_window);

    return ev->to_surface();
})

MirResizeEvent const* mir_event_get_resize_event(MirEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_resize);

    return ev->to_resize();
})

MirPromptSessionEvent const* mir_event_get_prompt_session_event(MirEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_prompt_session_state_change);

    return ev->to_prompt_session();
})

MirOrientationEvent const* mir_event_get_orientation_event(MirEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_orientation);

    return ev->to_orientation();
})

MirWindowOutputEvent const* mir_event_get_window_output_event(MirEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_window_output);

    return ev->to_window_output();
})

MirInputDeviceStateEvent const* mir_event_get_input_device_state_event(MirEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input_device_state);

    return ev->to_input_device_state();
})


/* Window event accessors */

MirWindowAttrib mir_window_event_get_attribute(MirWindowEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_window);

    return ev->attrib();
})

int mir_window_event_get_attribute_value(MirWindowEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_window);

    return ev->value();
})

/* Resize event accessors */

int mir_resize_event_get_width(MirResizeEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_resize);
    return ev->width();
})

int mir_resize_event_get_height(MirResizeEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_resize);
    return ev->height();
})

/* Prompt session event accessors */

MirPromptSessionState mir_prompt_session_event_get_state(MirPromptSessionEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_prompt_session_state_change);
    return ev->new_state();
})

/* Orientation event accessors */

MirOrientation mir_orientation_event_get_direction(MirOrientationEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_orientation);
    return ev->direction();
})

/* Window output event accessors */

int mir_window_output_event_get_dpi(MirWindowOutputEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_window_output);
    return ev->dpi();
})

MirFormFactor mir_window_output_event_get_form_factor(MirWindowOutputEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_window_output);
    return ev->form_factor();
})

float mir_window_output_event_get_scale(MirWindowOutputEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_window_output);
    return ev->scale();
})

double mir_window_output_event_get_refresh_rate(MirWindowOutputEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_window_output);
    return ev->refresh_rate();
})

uint32_t mir_window_output_event_get_output_id(MirWindowOutputEvent const *ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_window_output);
    return ev->output_id();
})

MirPointerButtons mir_input_device_state_event_pointer_buttons(MirInputDeviceStateEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input_device_state);
    return ev->pointer_buttons();
})

float mir_input_device_state_event_pointer_axis(MirInputDeviceStateEvent const* ev, MirPointerAxis axis) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input_device_state);
    return ev->pointer_axis(axis);
})

int64_t mir_input_device_state_event_time(MirInputDeviceStateEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input_device_state);
    return ev->when().count();
})

MirInputEventModifiers mir_input_device_state_event_modifiers(MirInputDeviceStateEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input_device_state);
    return ev->modifiers();
})

uint32_t mir_input_device_state_event_device_count(MirInputDeviceStateEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input_device_state);
    return ev->device_count();
})

MirInputDeviceId mir_input_device_state_event_device_id(MirInputDeviceStateEvent const* ev, uint32_t index) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input_device_state);
    expect_index_in_range(ev->device_count(), index);
    return ev->device_id(index);
})

uint32_t mir_input_device_state_event_device_pressed_keys_for_index(MirInputDeviceStateEvent const* ev,
                                                                    uint32_t index,
                                                                    uint32_t pressed_index) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input_device_state);
    expect_index_in_range(ev->device_count(), index);
    return ev->device_pressed_keys_for_index(index, pressed_index);
})

uint32_t mir_input_device_state_event_device_pressed_keys_count(MirInputDeviceStateEvent const* ev, uint32_t index) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input_device_state);
    expect_index_in_range(ev->device_count(), index);
    return ev->device_pressed_keys_count(index);
})

MirPointerButtons mir_input_device_state_event_device_pointer_buttons(MirInputDeviceStateEvent const* ev, uint32_t index) MIR_HANDLE_EVENT_EXCEPTION(
{
    expect_event_type(ev, mir_event_type_input_device_state);
    expect_index_in_range(ev->device_count(), index);
    return ev->device_pointer_buttons(index);
})

MirWindowPlacementEvent const* mir_event_get_window_placement_event(MirEvent const* event) MIR_HANDLE_EVENT_EXCEPTION(
{
    return event->to_window_placement();
})

MirRectangle mir_window_placement_get_relative_position(MirWindowPlacementEvent const* event) MIR_HANDLE_EVENT_EXCEPTION(
{
    return event->placement();
})

// TODO: Until we opaquify the MirEvent structure and add
// a ref count ref is implemented as copy.
MirEvent const* mir_event_ref(MirEvent const* ev) MIR_HANDLE_EVENT_EXCEPTION(
{
    return ev->clone();
})

void mir_event_unref(MirEvent const* ev)
{
    delete ev;
}
