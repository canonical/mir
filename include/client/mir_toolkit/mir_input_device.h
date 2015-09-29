/*
 * Copyright © 2015 Canonical Ltd.
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
 */

#ifndef MIR_TOOLKIT_MIR_INPUT_DEVICE_H_
#define MIR_TOOLKIT_MIR_INPUT_DEVICE_H_

/**
 * \addtogroup mir_toolkit
 * @{
 */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum MirTouchPadClickModes
{
    mir_touch_pad_click_mode_none          = 0,
    mir_touch_pad_click_mode_area_to_click = 1 << 1,
    mir_touch_pad_click_mode_finger_count  = 1 << 2
} MirTouchPadClickModes;
typedef unsigned int MirTouchPadClickMode;

typedef enum MirTouchPadScrollModes
{
    mir_touch_pad_scroll_mode_none               = 0,
    mir_touch_pad_scroll_mode_two_finger_scroll  = 1 << 1,
    mir_touch_pad_scroll_mode_edge_scroll        = 1 << 2,
    mir_touch_pad_scroll_mode_button_down_scroll = 1 << 3
} MirTouchPadScrollModes;
typedef unsigned int MirTouchPadScrollMode;


#ifdef __cplusplus
}
#endif

#endif
