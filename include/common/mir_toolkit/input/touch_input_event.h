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

#ifndef MIR_TOOLKIT_TOUCH_INPUT_EVENT_H_
#define MIR_TOOLKIT_TOUCH_INPUT_EVENT_H_

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

typedef struct _MirTouchInputEvent MirTouchInputEvent;

typedef int64_t MirTouchInputEventTouchId;

typedef enum {
    mir_touch_input_event_action_up,
    mir_touch_input_event_action_down,
    mir_touch_input_event_action_change,
    mir_touch_input_event_action_cancel,
    mir_touch_input_event_action_none
} MirTouchInputEventTouchAction;

typedef enum {
// TODO
    mir_touch_input_event_touch_axis_foo
} MirTouchInputEventTouchAxis;
typedef double MirTouchInputEventTouchAxisValue;

typedef enum {
// TODO: Tooltypes
    mir_touch_input_event_touch_tooltype_foobar
} MirTouchInputEventTouchTooltype;

unsigned int mir_touch_input_event_get_touch_count(MirTouchInputEvent const* event);

// TODO: Namespace consistently or conveniently?
MirTouchInputEventTouchId mir_touch_input_event_get_touch_id(MirTouchInputEvent const* event, size_t touch_index);
MirTouchInputEventTouchAction mir_touch_input_event_get_touch_action(MirTouchInputEvent const* event, size_t touch_index);
MirTouchInputEventTouchTooltype mir_touch_input_event_get_touch_tooltype(MirTouchInputEvent const* event, size_t touch_index);
MirTouchInputEventTouchAxisValue mir_touch_input_event_get_touch_axis_value(MirTouchInputEvent const* event, 
    size_t touch_index, MirTouchInputEventTouchAxis axis);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_TOUCH_INPUT_EVENT_H_ */
