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

#ifndef MIR_TOOLKIT_EVENT_H_
#define MIR_TOOLKIT_EVENT_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif
/* TODO: To the moon. */
#define MIR_INPUT_EVENT_MAX_POINTER_COUNT 16

typedef int64_t nsecs_t;

typedef enum
{
    mir_event_type_key,
    mir_event_type_motion
} MirEventType;

enum {
    mir_key_action_down = 0,
    mir_key_action_up = 1,
    mir_key_action_multiple = 2
};

enum {
    mir_motion_action_down = 0,
    mir_motion_action_up = 1,
    mir_motion_action_move = 2,
    mir_motion_action_cancel = 3,
    mir_motion_action_outside = 4,
    mir_motion_action_pointer_down = 5,
    mir_motion_action_pointer_up = 6,
    mir_motion_action_hover_move = 7,
    mir_motion_action_scroll = 8,
    mir_motion_action_hover_enter = 9,
    mir_motion_action_hover_exit = 10
};

typedef struct
{
    MirEventType type;

    int32_t device_id;
    int32_t source_id;
    int32_t action;
    int32_t flags;
    int32_t meta_state;

    int32_t key_code;
    int32_t scan_code;
    int32_t repeat_count;
    nsecs_t down_time;
    nsecs_t event_time;
    int is_system_key;
} MirKeyEvent;

typedef struct
{
    MirEventType type;

    int32_t device_id;
    int32_t source_id;
    int32_t action;
    int32_t flags;
    int32_t meta_state;

    int32_t edge_flags;
    int32_t button_state;
    float x_offset;
    float y_offset;
    float x_precision;
    float y_precision;
    nsecs_t down_time;
    nsecs_t event_time;

    size_t pointer_count;
    struct
    {
        int id;
        float x, raw_x;
        float y, raw_y;
        float touch_major;
        float touch_minor;
        float size;
        float pressure;
        float orientation;
    } pointer_coordinates[MIR_INPUT_EVENT_MAX_POINTER_COUNT];
} MirMotionEvent;

typedef union
{
    MirEventType   type;
    MirKeyEvent    key;
    MirMotionEvent motion;
} MirEvent;

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_EVENT_H_ */
