/*
 * Copyright © 2014-2020 Canonical Ltd.
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
 *
 */

#ifndef MIR_TOOLKIT_EVENT_ENUMS_H_
#define MIR_TOOLKIT_EVENT_ENUMS_H_

#include <mir_toolkit/common.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
typedef enum
{
    MIR_DEPRECATED_ENUM(mir_event_type_key, "mir_event_type_input"),     // UNUSED since Mir 0.26
    MIR_DEPRECATED_ENUM(mir_event_type_motion, "mir_event_type_input"),  // UNUSED since Mir 0.26
    MIR_DEPRECATED_ENUM(mir_event_type_surface, "mir_event_type_window"),
    mir_event_type_window = mir_event_type_surface,
    mir_event_type_resize,
    mir_event_type_prompt_session_state_change,
    mir_event_type_orientation,
    MIR_DEPRECATED_ENUM(mir_event_type_close_surface, "mir_event_type_close_window"),
    mir_event_type_close_window = mir_event_type_close_surface,
    /* Type for new style input event will be returned from mir_event_get_type
       when old style event type was mir_event_type_key or mir_event_type_motion */
    mir_event_type_input,
    mir_event_type_keymap,
    MIR_DEPRECATED_ENUM(mir_event_type_input_configuration, "mir_connection_set_input_config_change_callback and mir_event_type_input_device_state"),
    MIR_DEPRECATED_ENUM(mir_event_type_surface_output, "mir_event_type_window_output"),
    mir_event_type_window_output = mir_event_type_surface_output,
    mir_event_type_input_device_state,
    MIR_DEPRECATED_ENUM(mir_event_type_surface_placement, "mir_event_type_window_placement"),
    mir_event_type_window_placement = mir_event_type_surface_placement,
} MirEventType;
#pragma GCC diagnostic pop

typedef enum {
    mir_input_event_type_key = 0,
    mir_input_event_type_touch = 1,
    mir_input_event_type_pointer = 2,

    mir_input_event_types
} MirInputEventType;

/**
 * Description of key modifier state.
 */
typedef enum {
    mir_input_event_modifier_none        = 1 << 0,
    mir_input_event_modifier_alt         = 1 << 1,
    mir_input_event_modifier_alt_left    = 1 << 2,
    mir_input_event_modifier_alt_right   = 1 << 3,
    mir_input_event_modifier_shift       = 1 << 4,
    mir_input_event_modifier_shift_left  = 1 << 5,
    mir_input_event_modifier_shift_right = 1 << 6,
    mir_input_event_modifier_sym         = 1 << 7,
    mir_input_event_modifier_function    = 1 << 8,
    mir_input_event_modifier_ctrl        = 1 << 9,
    mir_input_event_modifier_ctrl_left   = 1 << 10,
    mir_input_event_modifier_ctrl_right  = 1 << 11,
    mir_input_event_modifier_meta        = 1 << 12,
    mir_input_event_modifier_meta_left   = 1 << 13,
    mir_input_event_modifier_meta_right  = 1 << 14,
    mir_input_event_modifier_caps_lock   = 1 << 15,
    mir_input_event_modifier_num_lock    = 1 << 16,
    mir_input_event_modifier_scroll_lock = 1 << 17
} MirInputEventModifier;
typedef unsigned int MirInputEventModifiers;

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
    mir_keyboard_action_repeat,

    mir_keyboard_actions
} MirKeyboardAction;

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_EVENT_ENUMS_H_ */
