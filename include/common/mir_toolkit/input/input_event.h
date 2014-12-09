/*
 * Copyright © 2014 Canonical Ltd.
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

#include "mir_toolkit/event.h"
#include "mir_toolkit/input/key_input_event.h"
#include "mir_toolkit/input/touch_input_event.h"

#include <stdint.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

typedef struct MirInputEvent MirInputEvent;

typedef int64_t MirInputDeviceId;

typedef enum {
    mir_input_event_type_key = 0,
    mir_input_event_type_touch = 1
} MirInputEventType;

/*
 * Retrieve the MirInputEvent associated with a MirEvent of 
 * type mir_event_type_input.
 *
 * \param [in] event The event
 * \return           The associated MirInputEvent
 */
MirInputEvent const* mir_event_get_input_event(MirEvent const* ev);

/*
 * Retrieves the device id responsible for generating an input event.
 *
 * \param [in] event The input event
 * \return           The id of the generating device
 */
MirInputDeviceId mir_input_event_get_device_id(MirInputEvent const* ev);

/*
 * Retrieve the time at which an input event occured.
 *
 * \param [in] event The input event
 * \return           A timestamp in nanoseconds-since-epoch
 */
int64_t mir_input_event_get_event_time(MirInputEvent const* ev);

/*
 * Retrieve the type of an input event (e.g. key, touch...)
 *
 * \param [in] event The input event
 * \return           The input event type
 */
MirInputEventType mir_input_event_get_type(MirInputEvent const* ev);

/*
 * Retrieve the MirKeyInputEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirKeyInputEvent or NULL if event type is not 
 *                  mir_input_event_type_key
 */
MirKeyInputEvent const* mir_input_event_get_key_input_event(MirInputEvent const* ev);

/*
 * Retrieve the MirTouchInputEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirTouchInputEvent or NULL if event type is not 
 *                  mir_input_event_type_touch
 */
MirTouchInputEvent const* mir_input_event_get_touch_input_event(MirInputEvent const* ev);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_INPUT_EVENT_H_
