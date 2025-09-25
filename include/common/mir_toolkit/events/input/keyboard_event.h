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

#ifndef MIR_TOOLKIT_KEYBOARD_EVENT_H_
#define MIR_TOOLKIT_KEYBOARD_EVENT_H_

#include <xkbcommon/xkbcommon.h>

#ifdef __cplusplus
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
 * Retrieve the action which triggered a given key event.
 *
 *  \param [in] event The key event
 *  \return           The associated action
 */
MirKeyboardAction mir_keyboard_event_action(MirKeyboardEvent const* event);

/**
 * Retrieve the xkb mapped keysym associated with the key acted on.. May
 * be interpreted as per <xkbcommon/xkb-keysyms.h>
 *
 *   \param [in] event The key event
 *   \return           The xkb_keysym
 */
xkb_keysym_t mir_keyboard_event_keysym(MirKeyboardEvent const* event);

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

#ifdef __cplusplus
}
#endif

#endif /* MIR_TOOLKIT_KEYBOARD_EVENT_H_ */
