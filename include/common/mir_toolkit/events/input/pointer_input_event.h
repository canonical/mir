/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_TOOLKIT_POINTER_INPUT_EVENT_H_
#define MIR_TOOLKIT_POINTER_INPUT_EVENT_H_

#include <stdbool.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * An event type describing a change in pointer device state.
 */
typedef struct MirPointerInputEvent MirPointerInputEvent;

/**
 * Possible pointer actions
 */
typedef enum {
    /* A pointer button has come up */
    mir_pointer_input_event_action_button_up = 0,
    /* A pointer button has gone down */
    mir_pointer_input_event_action_button_down = 1,
    /* The pointer has entered the surface to which this event was delivered */
    mir_pointer_input_event_action_enter = 2,
    /* The pointer has left the surface to which this event was delivered */
    mir_pointer_input_event_action_leave = 3,
    /* Axis values have changed for the pointer */
    mir_pointer_input_event_action_motion = 4
} MirPointerInputEventAction;

/**
 * Identifiers for pointer axis
 */
typedef enum {
/* Absolute axis containing the x coordinate of the pointer */
    mir_pointer_input_axis_x = 0,
/* Absolute axis containing the y coordinate of the pointer */
    mir_pointer_input_axis_y = 1,
/* Relative axis containing ticks reported by the vertical scroll wheel */
    mir_pointer_input_axis_vscroll = 2,
/* Relative axis containing ticks reported by the horizontal scroll wheel */
    mir_pointer_input_axis_hscroll = 3
} MirPointerInputEventAxis;

/* 
 * Identifiers for pointer buttons
 */
typedef enum {
    mir_pointer_input_button_primary   = 1,
    mir_pointer_input_button_secondary = 2,
    mir_pointer_input_button_tertiary  = 3,
    mir_pointer_input_button_back      = 4,
    mir_pointer_input_button_forward   = 5
} MirPointerInputEventButton;

/**
 * Retrieve the modifier keys pressed when the pointer action occured.
 *
 *   \param [in] event The pointer event
 *   \return           The modifier mask
 */
MirInputEventModifiers mir_pointer_input_event_get_modifiers(MirPointerInputEvent const* event);

/**
 * Retrieve the action which occured to generate a given pointer event.
 *
 *  \param [in] event       The pointer event
 *  \return                 Action performed by the pointer
 */
MirPointerInputEventAction mir_pointer_input_event_get_action(MirPointerInputEvent const* event);

/**
 * Retrieve the state of a given pointer button when the action occurred.
 *
 * \param [in] event         The pointer event
 * \param [in] button        The button to check
 *
 * \return                   Whether the given button is depressed
 */
bool mir_pointer_input_event_get_button_state(MirPointerInputEvent const* event,
    MirPointerInputEventButton button);

/**
 * Retrieve the axis value reported by a given pointer event.
 *
 *  \param [in] event       The pointer event
 *  \param [in] axis        The axis to retreive a value from
 *  \return                 The value of the given axis
 */
float mir_pointer_input_event_get_axis_value(MirPointerInputEvent const* event, 
    MirPointerInputEventAxis axis);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_POINTER_INPUT_EVENT_H_ */
