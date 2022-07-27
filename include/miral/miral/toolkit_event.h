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
 * \remark Since MirAL 2.4
 */
MirEventType mir_event_get_type(MirEvent const* event);

/**
 * Retrieve the MirInputEvent associated with a MirEvent of
 * type mir_event_type_input. See <mir_toolkit/events/input/input_event.h>
 * for accessors.
 *
 * \param [in] event The event
 * \return           The associated MirInputEvent
 * \remark Since MirAL 2.4
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
 * \remark Since MirAL 2.4
 */
int64_t mir_input_event_get_event_time(MirInputEvent const* event);

/**
 * Retrieve the type of an input event. E.g. key, touch...
 *
 * \param [in] event The input event
 * \return           The input event type
 * \remark Since MirAL 2.4
 */
MirInputEventType mir_input_event_get_type(MirInputEvent const* event);

/**
 * Retrieve the MirKeyboardEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirKeyboardEvent or NULL if event type is not
 *                  mir_input_event_type_key
 * \remark Since MirAL 2.4
 */
MirKeyboardEvent const* mir_input_event_get_keyboard_event(MirInputEvent const* event);

/**
 * Retrieve the MirTouchEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirTouchEvent or NULL if event type is not
 *                  mir_input_event_type_touch
 * \remark Since MirAL 2.4
 */
MirTouchEvent const* mir_input_event_get_touch_event(MirInputEvent const* event);

/**
 * Retrieve the MirPointerEvent associated with a given input event.
 *
 * \param[in] event The input event
 * \return          The MirPointerEvent or NULL if event type is not
 *                  mir_input_event_type_pointer
 * \remark Since MirAL 2.4
 */
MirPointerEvent const* mir_input_event_get_pointer_event(MirInputEvent const* event);

/**
 * Query if an input event contains a cookie
 *
 * \param [in] ev The input event
 * \return        True if the input event contains a cookie
 * \remark Since MirAL 2.4
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
 * \remark Since MirAL 2.4
 */
MirEvent const* mir_input_event_get_event(MirInputEvent const* event);

/**
 * Retrieve the action which triggered a given key event.
 *
 *  \param [in] event The key event
 *  \return           The associated action
 *  \remark Since MirAL 2.4
 */
MirKeyboardAction mir_keyboard_event_action(MirKeyboardEvent const* event);

/**
 * Retrieve the xkb mapped keysym associated with the key acted on.. May
 * be interpreted as per <xkbcommon/xkbcommon-keysyms.h>
 *
 *   \param [in] event The key event
 *   \return           The xkb_keysym
 *   \remark Since MirAL 3.3
 */
xkb_keysym_t mir_keyboard_event_keysym(MirKeyboardEvent const* event);

/**
 *   \deprecated       Returns the same thing as mir_keyboard_event_keysym(), which should be used instead.
 *   \remark Since MirAL 2.4
 */
xkb_keysym_t mir_keyboard_event_key_code(MirKeyboardEvent const* event);

/**
 * Retrieve the raw hardware scan code associated with the key acted on. May
 * be interpreted as per <linux/input.h>
 *
 *   \param [in] event The key event
 *   \return           The scancode
 *   \remark Since MirAL 2.4
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
 *   \remark Since MirAL 2.4
 */
MirInputEventModifiers mir_keyboard_event_modifiers(MirKeyboardEvent const* event);

/**
 * Retrieve the corresponding input event.
 *
 * \param [in] event The keyboard event
 * \return           The input event
 * \remark Since MirAL 2.4
 */
MirInputEvent const* mir_keyboard_event_input_event(MirKeyboardEvent const* event);

/**
 * Retrieve the modifier keys pressed when the touch action occured.
 *
 *   \param [in] event The key event
 *   \return           The modifier mask
 *   \remark Since MirAL 2.4
 */
MirInputEventModifiers mir_touch_event_modifiers(MirTouchEvent const* event);

/**
 * Retrieve the number of touches reported for a given touch event. Each touch
 * is said to be index in the event and may be accessed by index 0, 1, ... , (touch_count - 1)
 *
 *   \param [in] event The touch event
 *   \return           The number of touches
 *   \remark Since MirAL 2.4
 */
unsigned int mir_touch_event_point_count(MirTouchEvent const* event);

/**
 * Retrieve the TouchID for a touch at given index.
 *
 *  \param [in] event       The touch event
 *  \param [in] touch_index The touch index. Must be less than (touch_count - 1).
 *  \return                 ID of the touch at index
 *  \remark Since MirAL 2.4
 */
MirTouchId mir_touch_event_id(MirTouchEvent const* event, unsigned int touch_index);

/**
 * Retrieve the action which occured for a touch at given index.
 *
 *  \param [in] event       The touch event
 *  \param [in] touch_index The touch index. Must be less than (touch_count - 1).
 *  \return                 Action performed for the touch at index.
 *  \remark Since MirAL 2.4
 */
MirTouchAction mir_touch_event_action(MirTouchEvent const* event, unsigned int touch_index);

/**
 * Retrieve the tooltype for touch at given index.
 *
 *  \param [in] event       The touch event
 *  \param [in] touch_index The touch index. Must be less than (touch_count - 1).
 *  \return                 Tooltype used for the touch at index
 *  \remark Since MirAL 2.4
 */
MirTouchTooltype mir_touch_event_tooltype(MirTouchEvent const* event, unsigned int touch_index);


/**
 * Retrieve the axis value for a given axis on an indexed touch.
 *
 *  \param [in] event       The touch event
 *  \param [in] touch_index The touch index. Must be less than (touch_count - 1).
 *  \param [in] axis        The axis to retreive a value from
 *  \return                 The value of the given axis
 *  \remark Since MirAL 2.4
 */
float mir_touch_event_axis_value(MirTouchEvent const* event, unsigned int touch_index, MirTouchAxis axis);

/**
 * Retrieve the corresponding input event.
 *
 * \param [in] event The touch event
 * \return           The input event
 * \remark Since MirAL 2.4
 */
MirInputEvent const* mir_touch_event_input_event(MirTouchEvent const* event);


/**
 * Retrieve the modifier keys pressed when the pointer action occured.
 *
 *   \param [in] event The pointer event
 *   \return           The modifier mask
 *   \remark Since MirAL 2.4
 */
MirInputEventModifiers mir_pointer_event_modifiers(MirPointerEvent const* event);

/**
 * Retrieve the action which occured to generate a given pointer event.
 *
 *  \param [in] event       The pointer event
 *  \return                 Action performed by the pointer
 *  \remark Since MirAL 2.4
 */
MirPointerAction mir_pointer_event_action(MirPointerEvent const* event);

/**
 * Retrieve the state of a given pointer button when the action occurred.
 *
 * \param [in] event         The pointer event
 * \param [in] button        The button to check
 *
 * \return                   Whether the given button is depressed
 * \remark Since MirAL 2.4
 */
bool mir_pointer_event_button_state(MirPointerEvent const* event,
    MirPointerButton button);

/**
 * Retreive the pointer button state as a masked set of values.
 *
 * \param [in] event         The pointer event
 *
 * \return                   The button state
 * \remark Since MirAL 2.4
 */
MirPointerButtons mir_pointer_event_buttons(MirPointerEvent const* event);

/**
 * Retrieve the axis value reported by a given pointer event.
 *
 *  \param [in] event       The pointer event
 *  \param [in] axis        The axis to retreive a value from
 *  \return                 The value of the given axis
 *  \remark Since MirAL 2.4
 */
float mir_pointer_event_axis_value(MirPointerEvent const* event,
    MirPointerAxis axis);

/**
 * Retrieve the corresponding input event.
 *
 * \param [in] event The pointer event
 * \return           The input event
 * \remark Since MirAL 2.4
 */
MirInputEvent const* mir_pointer_event_input_event(MirPointerEvent const* event);
}
}

#endif //MIRAL_TOOLKIT_EVENT_H_
