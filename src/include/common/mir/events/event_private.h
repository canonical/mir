/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

// ==================================
// 
// Direct access to MirEvent deprecated. Prefer mir_event_* family of functions.
//
// ==================================

#ifndef MIR_COMMON_EVENT_PRIVATE_H_
#define MIR_COMMON_EVENT_PRIVATE_H_

#include <stddef.h>
#include <stdint.h>
#include "mir_toolkit/event.h"
#include "mir_toolkit/common.h"

#include <xkbcommon/xkbcommon.h>
#include <chrono>

#include <chrono>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif
/* TODO: To the moon. */
#define MIR_INPUT_EVENT_MAX_POINTER_COUNT 16

typedef enum {
    mir_motion_action_down         = 0,
    mir_motion_action_up           = 1,
    mir_motion_action_move         = 2,
    mir_motion_action_cancel       = 3,
    mir_motion_action_outside      = 4,
    mir_motion_action_pointer_down = 5,
    mir_motion_action_pointer_up   = 6,
    mir_motion_action_hover_move   = 7,
    mir_motion_action_scroll       = 8,
    mir_motion_action_hover_enter  = 9,
    mir_motion_action_hover_exit   = 10
} MirMotionAction;

// PRIVATE
// Direct access to MirKeyEvent is deprecated. Please use mir_event_get_input_event
// and the mir_input_event* family of functions.
typedef struct
{
    MirEventType type;

    int32_t device_id;
    int32_t source_id;
    MirKeyboardAction action;
    MirInputEventModifiers modifiers;

    int32_t key_code;
    int32_t scan_code;

    std::chrono::nanoseconds event_time;
} MirKeyEvent;

typedef struct
{
    int id;
    float x;
    float y;
    float touch_major;
    float touch_minor;
    float size;
    float pressure;
    float orientation;
    float vscroll;
    float hscroll;
    MirTouchTooltype tool_type;
} MirMotionPointer;

// PRIVATE
// Direct access to MirMotionEvent is deprecated. Please use mir_event_get_input_event
// and the mir_input_event* family of functions.
typedef struct
{
    MirEventType type;

    int32_t device_id;
    int32_t source_id;
    /*
     * TODO(racarr): We would like to store this as a MirMotionAction but the android input stack
     * encodes some non enumerable values in it. It's convenient to keep things
     * this way for now until we can drop SF/Hybris support in QtUbuntu.
     */
    int action;
    MirInputEventModifiers modifiers;

    MirPointerButtons buttons;
    std::chrono::nanoseconds event_time;

    size_t pointer_count;
    MirMotionPointer pointer_coordinates[MIR_INPUT_EVENT_MAX_POINTER_COUNT];
    /* "_coordinates" is a misnomer here because there's plenty more info than
       just coordinates, but renaming it accurately would be an API break */
} MirMotionEvent;
 
struct MirInputConfigurationEvent
{
    MirEventType type;

    MirInputConfigurationAction action;
    std::chrono::nanoseconds when;
    MirInputDeviceId id;
};

struct MirSurfaceEvent
{
    MirEventType type;

    int id;
    MirSurfaceAttrib attrib;
    int value;
};

struct MirResizeEvent
{
    MirEventType type;

    int surface_id;
    int width;
    int height;
};

struct MirPromptSessionEvent
{
    MirEventType type;

    MirPromptSessionState new_state;
};

struct MirOrientationEvent
{
    MirEventType type;

    int surface_id;
    MirOrientation direction;
};

struct MirCloseSurfaceEvent
{
    MirEventType type;

    int surface_id;
};

struct MirKeymapEvent
{
    MirEventType type;

    int surface_id;
    struct xkb_rule_names rules;
};

// Access to MirEvent is deprecated
union MirEvent
{
    MirEventType    type;
    MirKeyEvent     key;
    MirMotionEvent  motion;
    MirSurfaceEvent surface;
    MirResizeEvent  resize;
    MirPromptSessionEvent  prompt_session;
    MirOrientationEvent orientation;
    MirCloseSurfaceEvent   close_surface;
    MirKeymapEvent keymap;
    MirInputConfigurationEvent input_configuration;
};

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_COMMON_EVENT_PRIVATE_H_ */
