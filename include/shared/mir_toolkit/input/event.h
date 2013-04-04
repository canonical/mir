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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_TOOLKIT_INPUT_EVENT_H_
#define MIR_TOOLKIT_INPUT_EVENT_H_

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
        MIR_INPUT_EVENT_TYPE_KEY,
        MIR_INPUT_EVENT_TYPE_MOTION,
        MIR_INPUT_EVENT_TYPE_HW_SWITCH
    } MirEventType;

    typedef struct MirEvent
    {
        /* Generic event properties */
        MirEventType type;
        int32_t device_id;
        int32_t source_id;
        int32_t action;
        int32_t flags;
        int32_t meta_state;
        /* Information specific to key/motion event types */
        union
        {
            struct HardwareSwitchEvent
            {
                nsecs_t event_time;
                uint32_t policy_flags;
                int32_t switch_code;
                int32_t switch_value;
            } hw_switch;
            struct KeyEvent
            {
                int32_t key_code;
                int32_t scan_code;
                int32_t repeat_count;
                nsecs_t down_time;
                nsecs_t event_time;
                int is_system_key;
            } key;
            struct MotionEvent
            {
                int32_t edge_flags;
                int32_t button_state;
                float x_offset;
                float y_offset;
                float x_precision;
                float y_precision;
                nsecs_t down_time;
                nsecs_t event_time;

                size_t pointer_count;
                struct PointerCoordinates
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
            } motion;
        } details;
    } MirEvent;

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_INPUT_EVENT_H_ */
