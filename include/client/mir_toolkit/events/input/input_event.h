/*
 * Copyright © 2014-2016 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TOOLKIT_INPUT_EVENT_H_
#define MIR_TOOLKIT_INPUT_EVENT_H_

#include "mir_toolkit/events/event.h"

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

typedef int64_t MirInputDeviceId;

typedef enum {
    mir_input_event_type_key = 0,
    mir_input_event_type_touch = 1,
    mir_input_event_type_pointer = 2
} MirInputEventType;

/**
 * Description of key modifier state.
 */
typedef enum {
    mir_input_event_modifier_none        = 1 << 0,
    mir_input_event_modifier_alt         = 1 << 1,
    mir_input_event_modifier_alt_left    = 1 << 2,
    mir_input_event_modifier_alt_right   = 1 << 3,
    mir_input_event_modifier_shift       = 1 << 4,
    mir_input_event_modifier_shift_left  = 1 << 5,
    mir_input_event_modifier_shift_right = 1 << 6,
    mir_input_event_modifier_sym         = 1 << 7,
    mir_input_event_modifier_function    = 1 << 8,
    mir_input_event_modifier_ctrl        = 1 << 9,
    mir_input_event_modifier_ctrl_left   = 1 << 10,
    mir_input_event_modifier_ctrl_right  = 1 << 11,
    mir_input_event_modifier_meta        = 1 << 12,
    mir_input_event_modifier_meta_left   = 1 << 13,
    mir_input_event_modifier_meta_right  = 1 << 14,
    mir_input_event_modifier_caps_lock   = 1 << 15,
    mir_input_event_modifier_num_lock    = 1 << 16,
    mir_input_event_modifier_scroll_lock = 1 << 17
} MirInputEventModifier;
typedef unsigned int MirInputEventModifiers;

#ifdef __cplusplus
}
/**@}*/
#endif

#include "mir_toolkit/events/input/touch_event.h"
#include "mir_toolkit/events/input/keyboard_event.h"
#include "mir_toolkit/events/input/pointer_event.h"

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Retrieves the device id responsible for generating an input event.
 *
 * \param [in] event The input event
 * \return           The id of the generating device
 */
MirInputDeviceId mir_input_event_get_device_id(MirInputEvent const* ev);

/**
 * Retrieve the time at which an input event occurred.
 *
 * \param [in] event The input event
 * \return           A timestamp in nanoseconds-since-epoch
 */
int64_t mir_input_event_get_event_time(MirInputEvent const* ev);

/**
 * Retrieve the type of an input event. E.g. key, touch...
 *
 * \param [in] event The input event
 * \return           The input event type
 */
MirInputEventType mir_input_event_get_type(MirInputEvent const* ev);

/**
 * Retrieve the MirKeyboardEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirKeyboardEvent or NULL if event type is not 
 *                  mir_input_event_type_key
 */
MirKeyboardEvent const* mir_input_event_get_keyboard_event(MirInputEvent const* ev);

/**
 * Retrieve the MirTouchEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirTouchEvent or NULL if event type is not 
 *                  mir_input_event_type_touch
 */
MirTouchEvent const* mir_input_event_get_touch_event(MirInputEvent const* ev);

/**
 * Retrieve the MirPointerEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirPointerEvent or NULL if event type is not 
 *                  mir_input_event_type_pointer
 */
MirPointerEvent const* mir_input_event_get_pointer_event(MirInputEvent const* ev);

/**
 * Query if an input event contains a cookie
 *
 * \params[in] ev The input event
 * \return        True if the input event contains a cookie
 */
bool mir_input_event_has_cookie(MirInputEvent const* ev);

/**
 * Returns the cookie associated with an input event.
 *
 * \pre The input event must have a MirCookie
 * \params[in] ev An input event
 * \return        The cookie associated with the given input event
 *                The cookie must be released by calling mir_cookie_release
 */
MirCookie const* mir_input_event_get_cookie(MirInputEvent const* ev);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_INPUT_EVENT_H_
