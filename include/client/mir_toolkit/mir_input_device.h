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

typedef enum MirTouchpadClickMode
{
    mir_touchpad_click_mode_none          = 0,
    mir_touchpad_click_mode_area_to_click = 1 << 0,
    mir_touchpad_click_mode_finger_count  = 1 << 1
} MirTouchpadClickMode;
typedef unsigned int MirTouchpadClickModes;

typedef enum MirTouchpadScrollMode
{
    mir_touchpad_scroll_mode_none               = 0,
    mir_touchpad_scroll_mode_two_finger_scroll  = 1 << 0,
    mir_touchpad_scroll_mode_edge_scroll        = 1 << 1,
    mir_touchpad_scroll_mode_button_down_scroll = 1 << 2
} MirTouchpadScrollMode;
typedef unsigned int MirTouchpadScrollModes;

typedef struct MirInputDevices MirInputDevices;

/**
 * Retrieve the number of available input devices.
 *
 * \param [in] devices  The input devices
 *
 * \return              Number of input devices
 */
size_t mir_input_devices_count(MirInputDevices const* devices);

/**
 * Release this copy of the available input devices.
 * This invalidates any pointers retrieved from this set of input devices.
 *
 * \param [in] devices  The input devices
 */
void mir_input_devices_destroy(MirInputDevices const* devices);

/**
 * Retrieve the capabilities of the input device at the given index.
 *
 * \param [in] devices  The input devices
 * \param [in] index    The device index. Must be less than
 *                      (mir_input_devices_count - 1).
 *
 * \return              The capability flags of the input device at index.
 */
uint32_t mir_input_devices_get_capabilities(MirInputDevices const* devices, size_t index);

/**
 * Retrieve the device id of the input device at the given index.
 * The device id is a unique integer value, only valid while the device is
 * attached. The device id matches the device ids attached to input events.
 *
 * \param [in] devices  The input devices
 * \param [in] index    The device index. Must be less than
 *                      (mir_input_devices_count - 1).
 *
 * \return              The device id of the input device at the given index.
 */
MirInputDeviceId mir_input_devices_get_id(MirInputDevices const* devices, size_t index);

/**
 * Retrieve the name of the input device at the given index.
 * The string pointed to will be valid as long as MirInputDevices is valid.
 * The name may be empty but never NULL.
 *
 * \param [in] devices  The input devices
 * \param [in] index    The device index. Must be less than
 *                      (mir_input_devices_count - 1).
 *
 * \return              The name of the input device at the given index.
 */
char const* mir_input_devices_get_name(MirInputDevices const* devices, size_t index);

/**
 * Retrieve the unique id of the input device at the given index.
 * The string pointed to will be valid as long as MirInputDevices is valid.
 * The value of the unique id of a given device should be valid across mir
 * connections session and servers of the same version.
 *
 * \param [in] devices  The input devices
 * \param [in] index    The device index. Must be less than
 *                      (mir_input_devices_count - 1).
 *
 * \return              The unique id of the input device at the given index.
 */
char const* mir_input_devices_get_unique_id(MirInputDevices const* devices, size_t index);

#ifdef __cplusplus
}
#endif

#endif
