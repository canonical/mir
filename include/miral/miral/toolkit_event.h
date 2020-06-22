/*
 * Copyright © 2020 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_TOOLKIT_EVENT_H_
#define MIRAL_TOOLKIT_EVENT_H_

#include <mir_toolkit/events/enums.h>

#include <xkbcommon/xkbcommon.h>

struct MirEvent;
struct MirKeyboardEvent;
struct MirTouchEvent;
struct MirPointerEvent;
struct MirInputEvent;

/**
 * An identifier for a touch-point. TouchId's are unique per-gesture.
 * That is to say, once a touch has gone down at time T, no other touch will
 * use that touch's ID until all touches at time T have come up.
 */
typedef int32_t MirTouchId;

namespace miral
{
namespace toolkit
{
/**
 * Retrieves the type of a MirEvent. Now preferred over direct access to ev->type.
 * In particular ev->type will never be mir_event_type_input and mir_event_get_type
 * is the only way to ensure mir_event_get_input_event will succeed.
 *
 * \param [in] event The event
 * \return           The event type
 */
MirEventType mir_event_get_type(MirEvent const* event);

/**
 * Retrieve the MirInputEvent associated with a MirEvent of
 * type mir_event_type_input. See <mir_toolkit/events/input/input_event.h>
 * for accessors.
 *
 * \param [in] event The event
 * \return           The associated MirInputEvent
 */
MirInputEvent const* mir_event_get_input_event(MirEvent const* event);

///**
// * Retrieves the device id responsible for generating an input event.
// *
// * \param [in] event The input event
// * \return           The id of the generating device
// */
//MirInputDeviceId mir_input_event_get_device_id(MirInputEvent const* event);

/**
 * Retrieve the time at which an input event occurred.
 *
 * \param [in] event The input event
 * \return           A timestamp in nanoseconds-since-epoch
 */
int64_t mir_input_event_get_event_time(MirInputEvent const* event);

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
 * \return          The MirKeyboardEvent or NULL if event type is not
 *                  mir_input_event_type_key
 */
MirKeyboardEvent const* mir_input_event_get_keyboard_event(MirInputEvent const* event);

/**
 * Retrieve the MirTouchEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirTouchEvent or NULL if event type is not
 *                  mir_input_event_type_touch
 */
MirTouchEvent const* mir_input_event_get_touch_event(MirInputEvent const* event);

/**
 * Retrieve the MirPointerEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirPointerEvent or NULL if event type is not
 *                  mir_input_event_type_pointer
 */
MirPointerEvent const* mir_input_event_get_pointer_event(MirInputEvent const* event);

/**
 * Query if an input event contains a cookie
 *
 * \param [in] ev The input event
 * \return        True if the input event contains a cookie
 */
bool mir_input_event_has_cookie(MirInputEvent const* ev);

///**
// * Returns the cookie associated with an input event.
// *
// * \pre The input event must have a MirCookie
// * \param [in] ev An input event
// * \return        The cookie associated with the given input event
// *                The cookie must be released by calling mir_cookie_release
// */
//MirCookie const* mir_input_event_get_cookie(MirInputEvent const* ev);

/**
 * Retrieve the MirEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirEvent
 */
MirEvent const* mir_input_event_get_event(MirInputEvent const* event);

/**
 * Retrieve the action which triggered a given key event.
 *
 *  \param [in] event The key event
 *  \return           The associated action
 */
MirKeyboardAction mir_keyboard_event_action(MirKeyboardEvent const* event);

/**
 * Retrieve the xkb mapped keycode associated with the key acted on.. May
 * be interpreted as per <xkbcommon/xkb-keysyms.h>
 *
 *   \param [in] event The key event
 *   \return           The xkb_keysym
 */
xkb_keysym_t mir_keyboard_event_key_code(MirKeyboardEvent const* event);

/**
 * Retrieve the raw hardware scan code associated with the key acted on. May
 * be interpreted as per <linux/input.h>
 *
 *   \param [in] event The key event
 *   \return           The scancode
 */
int mir_keyboard_event_scan_code(MirKeyboardEvent const* event);

/**
 * Retrieve the text the key press would emit as null terminated utf8 string.
 *
 * The text will only be available to key down and key repeat events.
 * For mir_keyboard_action_up or key presses that do produce text an empty
 * string will be returned.
 *
 *   \param [in] event The key event
 *   \return           The text
 */
char const* mir_keyboard_event_key_text(MirKeyboardEvent const* event);

/**
 * Retrieve the modifier keys pressed when the key action occured.
 *
 *   \param [in] event The key event
 *   \return           The modifier mask
 */
MirInputEventModifiers mir_keyboard_event_modifiers(MirKeyboardEvent const* event);

/**
 * Retrieve the corresponding input event.
 *
 * \param [in] event The keyboard event
 * \return           The input event
 */
MirInputEvent const* mir_keyboard_event_input_event(MirKeyboardEvent const* event);

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
MirTouchId mir_touch_event_id(MirTouchEvent const* event, unsigned int touch_index);

/**
 * Retrieve the action which occured for a touch at given index.
 *
 *  \param [in] event       The touch event
 *  \param [in] touch_index The touch index. Must be less than (touch_count - 1).
 *  \return                 Action performed for the touch at index.
 */
MirTouchAction mir_touch_event_action(MirTouchEvent const* event, unsigned int touch_index);

/**
 * Retrieve the tooltype for touch at given index.
 *
 *  \param [in] event       The touch event
 *  \param [in] touch_index The touch index. Must be less than (touch_count - 1).
 *  \return                 Tooltype used for the touch at index
 */
MirTouchTooltype mir_touch_event_tooltype(MirTouchEvent const* event, unsigned int touch_index);


/**
 * Retrieve the axis value for a given axis on an indexed touch.
 *
 *  \param [in] event       The touch event
 *  \param [in] touch_index The touch index. Must be less than (touch_count - 1).
 *  \param [in] axis        The axis to retreive a value from
 *  \return                 The value of the given axis
 */
float mir_touch_event_axis_value(MirTouchEvent const* event, unsigned int touch_index, MirTouchAxis axis);

/**
 * Retrieve the corresponding input event.
 *
 * \param [in] event The touch event
 * \return           The input event
 */
MirInputEvent const* mir_touch_event_input_event(MirTouchEvent const* event);


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
 * Retrieve the corresponding input event.
 *
 * \param [in] event The pointer event
 * \return           The input event
 */
MirInputEvent const* mir_pointer_event_input_event(MirPointerEvent const* event);
}
}

#endif //MIRAL_TOOLKIT_EVENT_H_
