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

#ifndef MIR_TOOLKIT_KEYBOARD_EVENT_H_
#define MIR_TOOLKIT_KEYBOARD_EVENT_H_

#include <xkbcommon/xkbcommon.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * An event type describing a change in keyboard state
 *
 * Apology #1: Keyboard events almost always come from a keyboard, except they
 * can also come from system buttons (power, volume, home). This is an issue
 * we've inherited from the Linux kernel and Android, but could be solved in
 * future by giving such system switch events their own input group such as
 * MirSwitchEvent.
 */
typedef struct MirKeyboardEvent MirKeyboardEvent;

/**
 * Possible actions for changing key state
 */
typedef enum {
    /* A key has come up (released) */
    mir_keyboard_action_up,
    /* A key has gone down (pressed) */
    mir_keyboard_action_down,
    /* System policy has triggered a key repeat on a key
       which was already down */
    mir_keyboard_action_repeat
} MirKeyboardAction;

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

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_KEYBOARD_EVENT_H_ */
