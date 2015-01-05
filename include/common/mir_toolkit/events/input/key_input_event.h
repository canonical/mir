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

#ifndef MIR_TOOLKIT_KEY_INPUT_EVENT_H_
#define MIR_TOOLKIT_KEY_INPUT_EVENT_H_

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
 */
typedef struct MirKeyInputEvent MirKeyInputEvent;

/**
 * Possible actions for changing key state
 */
typedef enum {
    /* A key has gone down */
    mir_key_input_event_action_up,
    /* A key has come up */
    mir_key_input_event_action_down,
    /* System policy has triggered a key repeat on a key
       which was already down */
    mir_key_input_event_action_repeat
} MirKeyInputEventAction;

/**
 * Retrieve the action which triggered a given key event.
 * 
 *  \param [in] event The key event
 *  \return           The associated action
 */
MirKeyInputEventAction mir_key_input_event_get_action(MirKeyInputEvent const* event);

/**
 * Retrieve the xkb mapped keycode associated with the key acted on.. May
 * be interpreted as per <xkbcommon/xkb-keysyms.h>
 *
 *   \param [in] event The key event
 *   \return           The xkb_keysym
 */
xkb_keysym_t mir_key_input_event_get_key_code(MirKeyInputEvent const* event);

/**
 * Retrieve the raw hardware scan code associated with the key acted on. May
 * be interpreted as per <linux/input.h>
 *
 *   \param [in] event The key event
 *   \return           The scancode
 */
int mir_key_input_event_get_scan_code(MirKeyInputEvent const* event);

/**
 * Retrieve the modifier keys pressed when the key action occured.
 *
 *   \param [in] event The key event
 *   \return           The modifier mask
 */
MirInputEventModifiers mir_key_input_event_get_modifiers(MirKeyInputEvent const* event);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_KEY_INPUT_EVENT_H_ */
