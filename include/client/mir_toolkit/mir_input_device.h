/*
 * Copyright © 2015-2016 Canonical Ltd.
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

#include "mir_toolkit/client_types.h"

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
 * Retrieve the number of available input devices.
 *
 * \param [in] config   The input configuration snapshot
 *
 * \return              Number of input devices
 */
size_t mir_input_config_device_count(MirInputConfig const* config);

/**
 * Retrieve the input device at given \a index.
 *
 * The pointer returned stays valid until mir_input_config_destroy
 * is called with \a config.
 *
 * \param [in] config   The input configuration snapshot
 * \param [in] index    The index of the input device to return.
 * \return              input device
 */
MirInputDevice const* mir_input_config_get_device(
    MirInputConfig const* config,
    size_t index);

/**
 * Retrieve the input device by \a id.
 *
 * The MirInputDevice returned stays valid until mir_input_config_destroy
 * is called with \a config. If no device with the given \a id is found
 * NULL will be returned.
 *
 * \param [in] config   The input configuration snapshot
 * \param [in] id       The input device id to search for
 *
 * \return              input device
 */
MirInputDevice const* mir_input_config_get_device_by_id(
    MirInputConfig const* config,
    MirInputDeviceId id);

/**
 * Retrieve the input device at given \a index.
 *
 * The pointer returned stays valid until mir_input_config_destroy
 * is called with \a config.
 *
 * \param [in] config   The input configuration snapshot
 * \param [in] index    The index of the input device to return.
 * \return              input device
 */
MirInputDevice* mir_input_config_get_mutable_device(
    MirInputConfig* config,
    size_t index);

/**
 * Retrieve the input device by \a id.
 *
 * The MirInputDevice returned stays valid until mir_input_config_destroy
 * is called with \a config. If no device with the given \a id is found
 * NULL will be returned.
 *
 * \param [in] config   The input configuration snapshot
 * \param [in] id       The input device id to search for
 *
 * \return              input device
 */
MirInputDevice* mir_input_config_get_mutable_device_by_id(
    MirInputConfig* config,
    MirInputDeviceId id);

/**
 * Retrieve the capabilities of the input device at the given index.
 *
 * \param [in] device   The input device
 *
 * \return              The capability flags of the input device
 */
MirInputDeviceCapabilities mir_input_device_get_capabilities(
    MirInputDevice const* device);

/**
 * Retrieve the device id of the input device.
 * The device id is a unique integer value, only valid while the device is
 * attached. The device id matches the device id attached every input event.
 *
 * \param [in] device   The input device
 *
 * \return              The device id of the input device
 */
MirInputDeviceId mir_input_device_get_id(MirInputDevice const* device);

/**
 * Retrieve the name of the input device.
 * The string pointed to will be valid as long as MirInputDevice is valid.
 * The name may be empty but never NULL.
 *
 * \param [in] device   The input device
 *
 * \return              The name of the input device
 */
char const* mir_input_device_get_name(MirInputDevice const* device);

/**
 * Retrieve the unique id of the input device.
 * The string pointed to will be valid as long as \a device is valid.
 * The value of the unique id of a given device should be valid across mir
 * connections session and servers of the same version.
 *
 * \param [in] device   The input device
 *
 * \return              The unique id of the input device
 */
char const* mir_input_device_get_unique_id(MirInputDevice const* device);

#ifdef __cplusplus
}
#endif

#endif
