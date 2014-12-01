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
 * Description of key modifier state.
 */
typedef enum {
    mir_key_input_event_modifier_none        = 1 << 0,
    mir_key_input_event_modifier_alt         = 1 << 1,
    mir_key_input_event_modifier_alt_left    = 1 << 2,
    mir_key_input_event_modifier_alt_right   = 1 << 3,
    mir_key_input_event_modifier_shift       = 1 << 4,
    mir_key_input_event_modifier_shift_left  = 1 << 5,
    mir_key_input_event_modifier_shift_right = 1 << 6,
    mir_key_input_event_modifier_sym         = 1 << 7,
    mir_key_input_event_modifier_function    = 1 << 8,
    mir_key_input_event_modifier_ctrl        = 1 << 9,
    mir_key_input_event_modifier_ctrl_left   = 1 << 10,
    mir_key_input_event_modifier_ctrl_right  = 1 << 11,
    mir_key_input_event_modifier_meta        = 1 << 12,
    mir_key_input_event_modifier_meta_left   = 1 << 13,
    mir_key_input_event_modifier_meta_right  = 1 << 14,
    mir_key_input_event_modifier_caps_lock   = 1 << 15,
    mir_key_input_event_modifier_num_lock    = 1 << 16,
    mir_key_input_event_modifier_scroll_lock = 1 << 17
} MirKeyInputEventModifiers;

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
MirKeyInputEventModifiers mir_key_input_event_get_modifiers(MirKeyInputEvent const* event);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_KEY_INPUT_EVENT_H_ */
