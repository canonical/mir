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

/**
 * Possible per touch actions for state changing
 */
typedef enum {
    /* This touch point is going up */
    mir_touch_action_up = 0,
    /* This touch point is going down */
    mir_touch_action_down = 1,
    /* Axis values have changed on this touch point */
    mir_touch_action_change = 2,

    mir_touch_actions
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
    mir_touch_axis_size = 5,

    mir_touch_axes
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
    mir_touch_tooltype_stylus = 2,

    mir_touch_tooltypes
} MirTouchTooltype;


/**
 * Possible pointer actions
 */
typedef enum {
    /* A pointer button has come up */
    mir_pointer_action_button_up = 0,
    /* A pointer button has gone down */
    mir_pointer_action_button_down = 1,
    /* The pointer has entered the surface to which this event was delivered */
    mir_pointer_action_enter = 2,
    /* The pointer has left the surface to which this event was delivered */
    mir_pointer_action_leave = 3,
    /* Axis values have changed for the pointer */
    mir_pointer_action_motion = 4,

    mir_pointer_actions
} MirPointerAction;

/**
 * Identifiers for pointer axis
 */
typedef enum {
/* Absolute axis containing the x coordinate of the pointer */
    mir_pointer_axis_x = 0,
/* Absolute axis containing the y coordinate of the pointer */
    mir_pointer_axis_y = 1,
/* Relative axis containing ticks reported by the vertical scroll wheel */
    mir_pointer_axis_vscroll = 2,
/* Relative axis containing ticks reported by the horizontal scroll wheel */
    mir_pointer_axis_hscroll = 3,
/* Relative axis containing the last reported x differential from the pointer */
    mir_pointer_axis_relative_x = 4,
/* Relative axis containing the last reported y differential from the pointer */
    mir_pointer_axis_relative_y = 5,

    mir_pointer_axes
} MirPointerAxis;

/*
 * Identifiers for pointer buttons
 */
typedef enum {
    mir_pointer_button_primary   = 1 << 0,
    mir_pointer_button_secondary = 1 << 1,
    mir_pointer_button_tertiary  = 1 << 2,
    mir_pointer_button_back      = 1 << 3,
    mir_pointer_button_forward   = 1 << 4,
    mir_pointer_button_side      = 1 << 5,
    mir_pointer_button_extra     = 1 << 6,
    mir_pointer_button_task      = 1 << 7
} MirPointerButton;

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_EVENT_ENUMS_H_ */
