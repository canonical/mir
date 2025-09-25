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

#ifndef MIR_TOOLKIT_POINTER_EVENT_H_
#define MIR_TOOLKIT_POINTER_EVENT_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An event type describing a change in pointer device state.
 */
typedef struct MirPointerEvent MirPointerEvent;

/**
 * Retrieve the source of the pointer axis event.
 *
 *   \param [in] event The pointer event
 *   \return           The modifier mask
 */
MirPointerAxisSource mir_pointer_event_axis_source(MirPointerEvent const* event);

/**
 * Retrieve the modifier keys pressed when the pointer action occured.
 *
 *   \param [in] event The pointer event
 *   \return           The modifier mask
 */
MirInputEventModifiers mir_pointer_event_modifiers(MirPointerEvent const* event);

/**
 * Retrieve the action which occured to generate a given pointer event.
 *
 *  \param [in] event       The pointer event
 *  \return                 Action performed by the pointer
 */
MirPointerAction mir_pointer_event_action(MirPointerEvent const* event);

/**
 * Retrieve the state of a given pointer button when the action occurred.
 *
 * \param [in] event         The pointer event
 * \param [in] button        The button to check
 *
 * \return                   Whether the given button is depressed
 */
bool mir_pointer_event_button_state(MirPointerEvent const* event,
    MirPointerButton button);

/**
 * Retreive the pointer button state as a masked set of values.
 *
 * \param [in] event         The pointer event
 *
 * \return                   The button state
 */
MirPointerButtons mir_pointer_event_buttons(MirPointerEvent const* event);

/**
 * Retrieve the axis value reported by a given pointer event.
 *
 *  \param [in] event       The pointer event
 *  \param [in] axis        The axis to retreive a value from
 *  \return                 The value of the given axis
 */
float mir_pointer_event_axis_value(MirPointerEvent const* event,
    MirPointerAxis axis);

/**
 * Retrieve if this is a stop event for the given scroll axis. When scroll
 * events are generated with a source of finger they always end in an axis
 * stop when the finger is raised. Other scroll events may in with an axis
 * stop. Any axis other than vscroll and hscroll always returns false.
 *
 *  \param [in] event       The pointer event
 *  \param [in] axis        The axis to check
 *  \return                 If this is an axis stop event for the given axis
 */
bool mir_pointer_event_axis_stop(MirPointerEvent const* event,
    MirPointerAxis axis);

/**
 * Retrieve the corresponding input event.
 *
 * \param [in] event The pointer event
 * \return           The input event
 */
MirInputEvent const* mir_pointer_event_input_event(MirPointerEvent const* event);

#ifdef __cplusplus
}
#endif

#endif /* MIR_TOOLKIT_POINTER_EVENT_H_ */
