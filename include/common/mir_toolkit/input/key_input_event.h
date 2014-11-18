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

typedef struct _MirKeyInputEvent MirKeyInputEvent;

typedef enum {
    mir_key_input_event_action_up,
    // TODO: Is there value in supporting cancellation?
    mir_key_input_event_action_down,
    mir_key_input_event_action_repeat
} MirKeyInputEventAction;

// TODO: Investigate ~racarr
typedef enum {
    mir_key_input_event_modifier_none        = 0,
    mir_key_input_event_modifier_alt         = 0x02,
    mir_key_input_event_modifier_alt_left    = 0x10,
    mir_key_input_event_modifier_alt_right   = 0x20,
    mir_key_input_event_modifier_shift       = 0x01,
    mir_key_input_event_modifier_shift_left  = 0x40,
    mir_key_input_event_modifier_shift_right = 0x80,
    mir_key_input_event_modifier_sym         = 0x04,
    mir_key_input_event_modifier_function    = 0x08,
    mir_key_input_event_modifier_ctrl        = 0x1000,
    mir_key_input_event_modifier_ctrl_left   = 0x2000,
    mir_key_input_event_modifier_ctrl_right  = 0x4000,
    mir_key_input_event_modifier_meta        = 0x10000,
    mir_key_input_event_modifier_meta_left   = 0x20000,
    mir_key_input_event_modifier_meta_right  = 0x40000,
    mir_key_input_event_modifier_caps_lock   = 0x100000,
    mir_key_input_event_modifier_num_lock    = 0x200000,
    mir_key_input_event_modifier_scroll_lock = 0x400000
} MirKeyInputEventModifiers;

MirKeyInputEventAction mir_key_input_event_get_action(MirKeyInputEvent const* event);

xkb_keysym_t mir_key_input_event_get_key_code(MirKeyInputEvent const* event);
int mir_key_input_event_get_scan_code(MirKeyInputEvent const* event);

MirKeyInputEventModifiers mir_key_input_event_get_modifiers(MirKeyInputEvent const* event);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_KEY_INPUT_EVENT_H_ */
