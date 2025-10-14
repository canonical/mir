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

#ifndef MIR_TOOLKIT_INPUT_EVENT_H_
#define MIR_TOOLKIT_INPUT_EVENT_H_

#include <mir_toolkit/events/event.h>
#include <mir_toolkit/mir_input_device_types.h>

#include <stdint.h>
#include <stdbool.h>

#include <mir_toolkit/events/input/touch_event.h>
#include <mir_toolkit/events/input/keyboard_event.h>
#include <mir_toolkit/events/input/pointer_event.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Retrieves the device id responsible for generating an input event.
 *
 * \param [in] event The input event
 * \return           The id of the generating device
 */
MirInputDeviceId mir_input_event_get_device_id(MirInputEvent const* event);

/**
 * Retrieve the time at which an input event occurred.
 *
 * \param [in] event The input event
 * \return           A timestamp in nanoseconds-since-epoch
 */
int64_t mir_input_event_get_event_time(MirInputEvent const* event);

/**
 * Retrieve the event time as a Wayland timestamp.
 *
 * \param [in] event The input event
 * \return           A wayland-compatible timestamp (miliseconds-since-epoch)
 */
uint32_t mir_input_event_get_wayland_timestamp(MirInputEvent const* event);

/**
 * Retrieve the type of an input event. E.g. key, touch...
 *
 * \param [in] event The input event
 * \return           The input event type
 */
MirInputEventType mir_input_event_get_type(MirInputEvent const* event);

/**
 * Retrieve the MirKeyboardEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirKeyboardEvent
 * \pre             The event type is mir_input_event_type_key
 */
MirKeyboardEvent const* mir_input_event_get_keyboard_event(MirInputEvent const* event);

/**
 * Retrieve the MirTouchEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirTouchEvent
 * \pre             The event type is mir_input_event_type_touch
 */
MirTouchEvent const* mir_input_event_get_touch_event(MirInputEvent const* event);

/**
 * Retrieve the MirPointerEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirPointerEvent
 * \pre             The event type is mir_input_event_type_pointer
 */
MirPointerEvent const* mir_input_event_get_pointer_event(MirInputEvent const* event);

/**
 * Retrieve the MirEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirEvent
 */
MirEvent const* mir_input_event_get_event(MirInputEvent const* event);

#ifdef __cplusplus
}
#endif

#endif // MIR_TOOLKIT_INPUT_EVENT_H_
