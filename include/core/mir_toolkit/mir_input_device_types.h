/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIR_TOOLKIT_MIR_INPUT_DEVICE_TYPES_H_
#define MIR_TOOLKIT_MIR_INPUT_DEVICE_TYPES_H_

#include <stdint.h>

/**
 * \addtogroup mir_toolkit
 * @{
 */
#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t MirInputDeviceId;

typedef enum MirPointerHandedness
{
    mir_pointer_handedness_right = 0,
    mir_pointer_handedness_left = 1
} MirPointerHandedness;

/**
 * MirPointerAcceleration describes the way pointer movement is filtered:
 * - mir_pointer_acceleration_none: (acceleration bias + 1.0) is applied as
 *   a factor to the current velocity of the pointer. So a bias of 0 to results
 *   to no change of velocity.
 * - mir_pointer_acceleration_adaptive: acceleration bias selects an
 *   acceleration function based on the current velocity that usually consists
 *   of two linear inclines separated by a plateau.
 */
typedef enum MirPointerAcceleration
{
    mir_pointer_acceleration_none = 1,
    mir_pointer_acceleration_adaptive = 2
} MirPointerAcceleration;

/**
 * MirTouchpadClickMode configures how the touchpad itself should generate
 * pointer button events. The available click modes may be active
 * simultaneously.
 * - mir_touchpad_click_mode_none: no active click mode
 * - mir_touchpad_click_mode_area_to_click: simulate pointer buttons using
 *   click areas on the touchpad
 * - mir_touchpad_click_mode_finger_count: simulate pointer buttons using the
 *   number of fingers down
 */
typedef enum MirTouchpadClickMode
{
    mir_touchpad_click_mode_none          = 0,
    mir_touchpad_click_mode_area_to_click = 1 << 0,
    mir_touchpad_click_mode_finger_count  = 1 << 1
} MirTouchpadClickMode;
typedef unsigned int MirTouchpadClickModes;

/**
 * MirTouchpadScrollMode configures how the touchpad should generate scroll
 * events.
 * - mir_touchpad_scroll_mode_none: no scroll
 * - mir_touchpad_scroll_mode_two_finger_scroll: two finger movement generates
 *   generates vertical and horizontal scroll events
 * - mir_touchpad_scroll_mode_edge_scroll: touch movement at the edge of the
 *   touchpad genertes scroll events
 * - mir_touchpad_scroll_mode_button_down_scroll: movement on the touchpad
 *   generates scroll events when a button is held down simultaneously
 */
typedef enum MirTouchpadScrollMode
{
    mir_touchpad_scroll_mode_none               = 0,
    mir_touchpad_scroll_mode_two_finger_scroll  = 1 << 0,
    mir_touchpad_scroll_mode_edge_scroll        = 1 << 1,
    mir_touchpad_scroll_mode_button_down_scroll = 1 << 2
} MirTouchpadScrollMode;
typedef unsigned int MirTouchpadScrollModes;

enum MirInputDeviceCapability
{
    mir_input_device_capability_none        = 0,
    mir_input_device_capability_pointer     = 1<<1,
    mir_input_device_capability_keyboard    = 1<<2,
    mir_input_device_capability_touchpad    = 1<<3,
    mir_input_device_capability_touchscreen = 1<<4,
    mir_input_device_capability_gamepad     = 1<<5,
    mir_input_device_capability_joystick    = 1<<6,
    mir_input_device_capability_switch      = 1<<7,
    mir_input_device_capability_multitouch  = 1<<8,  //! capable to detect multiple contacts
    mir_input_device_capability_alpha_numeric = 1<<9 //! offers enough keys for text entry
};
typedef unsigned int MirInputDeviceCapabilities;

/**
 * Mapping modes for touchscreen devices. The mode defines how coordinates
 * from the touchscreen frequently referred to as device coordinates are
 * translated into scene coordinates.
 *
 * This configuration mode is relevant for different classes of input devices,
 * i.e handheld devices with builtin touchscreens or external graphic tablets or
 * external monitors with touchscreen capabilities.
 */
typedef enum MirTouchscreenMappingMode
{
    /**
     * Map the device coordinates onto specific output.
     */
    mir_touchscreen_mapping_mode_to_output,
    /**
     * Map the device coordinates onto the whole wall of outputs.
     */
    mir_touchscreen_mapping_mode_to_display_wall
} MirTouchscreenMappingMode;


#ifdef __cplusplus
}
#endif
/**@}*/
#endif
