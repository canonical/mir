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

#ifndef MIR_TOOLKIT_TOUCH_EVENT_H_
#define MIR_TOOLKIT_TOUCH_EVENT_H_

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * An event type describing a change in touch device state.
 */
typedef struct MirTouchEvent MirTouchEvent;

/** 
 * An identifier for a touch-point. TouchId's are unique per-gesture.
 * That is to say, once a touch has gone down at time T, no other touch will
 * use that touch's ID until all touches at time T have come up.
 */
typedef int32_t MirTouchId;

/**
 * Possible per touch actions for state changing
 */
typedef enum {
    /* This touch point is going up */
    mir_touch_action_up = 0,
    /* This touch point is going down */
    mir_touch_action_down = 1,
    /* Axis values have changed on this touch point */
    mir_touch_action_change = 2
} MirTouchAction;

/**
 * Identifiers for touch axis
 */
typedef enum {
/* Axis representing the x coordinate for the touch */
    mir_touch_axis_x = 0,
/* Axis representing the y coordinate for the touch */
    mir_touch_axis_y = 1,
/* Axis representing pressure of the touch */
    mir_touch_axis_pressure = 2,
/* Axis representing the length of the major axis of an ellipse
   centered at the touch point */
    mir_touch_axis_touch_major = 3,
/* Axis representing the length of the minor axis of an ellipse
   centered at the touch point */
    mir_touch_axis_touch_minor = 4,
/* Axis representing the diameter of a circle centered on the touch
   point */
    mir_touch_axis_size = 5
} MirTouchAxis;

/**
 * Identifiers for per-touch tool types
 */
typedef enum {
// Tool type could not be determined
    mir_touch_tooltype_unknown = 0,
// Touch is made with a finger
    mir_touch_tooltype_finger = 1,
// Touch is made with a stylus
    mir_touch_tooltype_stylus = 2
} MirTouchTooltype;

/**
 * Retrieve the modifier keys pressed when the touch action occured.
 *
 *   \param [in] event The key event
 *   \return           The modifier mask
 */
MirInputEventModifiers mir_touch_event_modifiers(MirTouchEvent const* event);

/**
 * Retrieve the number of touches reported for a given touch event. Each touch
 * is said to be index in the event and may be accessed by index 0, 1, ... , (touch_count - 1)
 *
 *   \param [in] event The touch event
 *   \return           The number of touches
 */
unsigned int mir_touch_event_point_count(MirTouchEvent const* event);

/**
 * Retrieve the TouchID for a touch at given index.
 *
 *  \param [in] event       The touch event
 *  \param [in] touch_index The touch index. Must be less than (touch_count - 1).
 *  \return                 ID of the touch at index
 */
MirTouchId mir_touch_event_id(MirTouchEvent const* event, size_t touch_index);

/**
 * Retrieve the action which occured for a touch at given index.
 *
 *  \param [in] event       The touch event
 *  \param [in] touch_index The touch index. Must be less than (touch_count - 1).
 *  \return                 Action performed for the touch at index.
 */
MirTouchAction mir_touch_event_action(MirTouchEvent const* event, size_t touch_index);

/**
 * Retrieve the tooltype for touch at given index.
 *
 *  \param [in] event       The touch event
 *  \param [in] touch_index The touch index. Must be less than (touch_count - 1).
 *  \return                 Tooltype used for the touch at index
 */
MirTouchTooltype mir_touch_event_tooltype(MirTouchEvent const* event,
    size_t touch_index);


/**
 * Retrieve the axis value for a given axis on an indexed touch.
 *
 *  \param [in] event       The touch event
 *  \param [in] touch_index The touch index. Must be less than (touch_count - 1).
 *  \param [in] axis        The axis to retreive a value from
 *  \return                 The value of the given axis
 */
float mir_touch_event_axis_value(MirTouchEvent const* event, 
    size_t touch_index, MirTouchAxis axis);

/**
 * Retrieve the corresponding input event.
 *
 * \param [in] event The touch event
 * \return           The input event
 */
MirInputEvent const* mir_touch_event_input_event(MirTouchEvent const* event);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_TOUCH_EVENT_H_ */
