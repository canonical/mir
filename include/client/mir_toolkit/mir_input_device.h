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
#include "mir_toolkit/mir_input_device_types.h"

/**
 * \addtogroup mir_toolkit
 * @{
 */
#ifdef __cplusplus
extern "C" {
#endif

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
 * The pointer returned stays valid until mir_input_config_release
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
 * The MirInputDevice returned stays valid until mir_input_config_release
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
 * The pointer returned stays valid until mir_input_config_release
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
 * The MirInputDevice returned stays valid until mir_input_config_release
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

/**
 * Retrieve a structure containing the pointer related config options
 * of the input device.
 *
 * If the input device does not control the mouse cursor, there will be no
 * config options, and the function will return a null pointer.
 *
 * \param [in] device   The input device
 *
 * \return              The pointer config
 */
MirPointerConfig const* mir_input_device_get_pointer_config(
    MirInputDevice const* device);

/**
 * Retrieve the cursor acceleration profile.
 *
 * \param [in] conf     The pointer config
 *
 * \return              The acceleration profile
 */
MirPointerAcceleration mir_pointer_config_get_acceleration(
    MirPointerConfig const* conf);

/**
 * Retrieve the cursor acceleration bias.
 *
 * The value will be in the range of [-1, 1]:
 *   - 0: default acceleration
 *   - [-1, 0): reduced acceleration
 *   - (0, 1]: increased acceleration
 *
 * \param [in] conf     The pointer config
 *
 * \return              The acceleration bias
 */
double mir_pointer_config_get_acceleration_bias(
    MirPointerConfig const* conf);

/**
 * Retrieve the horizontal scroll scale.
 *
 * The value is a signed linear scale of the horizontal scroll axis. A Negative
 * value indicates 'natural scrolling'.
 *
 * \param [in] conf     The pointer config
 *
 * \return              The horizontal scroll scale
 */
double mir_pointer_config_get_horizontal_scroll_scale(
    MirPointerConfig const* conf);

/**
 * Retrieve the vertical scroll scale.
 *
 * The value is a signed linear scale of the vertical scroll axis. A Negative
 * value indicates 'natural scrolling'.
 *
 * \param [in] conf     The pointer config
 *
 * \return              The vertical scroll scale
 */
double mir_pointer_config_get_vertical_scroll_scale(
    MirPointerConfig const* conf);

/**
 * Retrieve whether the pointer device is configured for right or left handed
 * use.
 *
 * \param [in] conf     The pointer config
 *
 * \return              Right or left handed
 */
MirPointerHandedness mir_pointer_config_get_handedness(
    MirPointerConfig const* conf);

/**
 * Retrieve a structure containing the pointer related config options
 * of the input device that can be manipulated.
 *
 * If the input device does not control the mouse cursor, there will be no
 * config options, and the function will return a null pointer.
 *
 * \param [in] device   The input device
 *
 * \return              The pointer config
 */
MirPointerConfig* mir_input_device_get_mutable_pointer_config(
    MirInputDevice* device);

/**
 * Set the acceleration mode of the pointer device.
 *
 * See \ref MirPointerAcceleration for reference.
 *
 * \param [in] conf          The pointer config
 * \param [in] acceleration  The acceleration mode
 */
void mir_pointer_config_set_acceleration(
    MirPointerConfig* conf,
    MirPointerAcceleration acceleration);

/**
 * Set the acceleration bias of the pointer device.
 *
 * The acceleration bias must be in the range of [-1, 1]:
 *   - 0: default acceleration
 *   - [-1, 0): reduced acceleration
 *   - (0, 1]: increased acceleration
 *
 * \param [in] conf              The pointer config
 * \param [in] acceleration_bias The acceleration bias 
 */
void mir_pointer_config_set_acceleration_bias(
    MirPointerConfig* conf,
    double acceleration_bias);

/**
 * Set the horizontal scroll scale.
 *
 * The horizontal scroll scale is a signed linear scale of scroll motion along
 * the horizontal axis. Negative scales can be used to configure 'natural
 * scrolling'.
 *
 * \param [in] conf                    The pointer config
 * \param [in] horizontal_scroll_scale The horizontal scroll scale
 */
void mir_pointer_config_set_horizontal_scroll_scale(
    MirPointerConfig* conf,
    double horizontal_scroll_scale);

/**
 * Set the vertical scroll scale.
 *
 * The vertical scroll scale is a signed linear scale of scroll motion along
 * the vertical axis. Negative scales can be used to configure 'natural
 * scrolling'.
 *
 * \param [in] conf                    The pointer config
 * \param [in] vertical_scroll_scale The vertical scroll scale
 */
void mir_pointer_config_set_vertical_scroll_scale(
    MirPointerConfig* conf,
    double vertical_scroll_scale);

/**
 * Configure left and right hand use of the pointer device.
 *
 * This configures which buttons will be used as primary and secondary buttons.
 *
 * \param [in] conf       The pointer config
 * \param [in] handedness left or right handed use
 */
void mir_pointer_config_set_handedness(
    MirPointerConfig* conf,
    MirPointerHandedness handedness);

/**
 * Retrieve a structure containing the touchpad related config options
 * of the input device.
 *
 * If the input device is not a touchpad this function will return null pointer.
 *
 * \param [in] device   The input device
 *
 * \return              The touchpad config
 */
MirTouchpadConfig const* mir_input_device_get_touchpad_config(
    MirInputDevice const* device);

/**
 * Retrieve the click modes of the touchpad.
 *
 * See \ref MirTouchpadClickMode for reference.
 *
 * \param [in] conf     The touchpad config
 *
 * \return              The touchpad click modes
 */
MirTouchpadClickModes mir_touchpad_config_get_click_modes(
    MirTouchpadConfig const* conf);

/**
 * Retrieve the scroll modes of the touchpad.
 *
 * See \ref MirTouchpadScrollMode for reference.
 *
 * \param [in] conf     The touchpad config
 *
 * \return              The touchpad click modes
 */
MirTouchpadScrollModes mir_touchpad_config_get_scroll_modes(
    MirTouchpadConfig const* conf);

/**
 * Retrieve the configured button down for button down scroll mode.
 *
 * See \ref MirTouchpadScrollMode for reference.
 *
 * \param [in] conf     The touchpad config
 *
 * \return              The touchpad click modes
 */
int mir_touchpad_config_get_button_down_scroll_button(
    MirTouchpadConfig const* conf);

/**
 * Retrieve whether a tap gesture generates pointer button events.
 *
 * \param [in] conf     The touchpad config
 *
 * \return              whether tap to click is enabled
 */
bool mir_touchpad_config_get_tap_to_click(
    MirTouchpadConfig const* conf);

/**
 * Retrieve whether middle mouse button should be emulated.
 *
 * \param [in] conf     The touchpad config
 *
 * \return              whether middle mouse button emulation is enabled
 */
bool mir_touchpad_config_get_middle_mouse_button_emulation(
    MirTouchpadConfig const* conf);

/**
 * Retrieve whether the touchpad should be disabled when an external pointer
 * device like a mouse is connected.
 *
 * \param [in] conf     The touchpad config
 *
 * \return              whether touchpad is disabled with an external mouse
 */
bool mir_touchpad_config_get_disable_with_mouse(
    MirTouchpadConfig const* conf);

/**
 * Retrieve whether the touchpad events should be blocked while the user types.
 *
 * \param [in] conf     The touchpad config
 *
 * \return              whether touchpad is disabled during typing
 */
bool mir_touchpad_config_get_disable_while_typing(
    MirTouchpadConfig const* conf);

/**
 * Retrieve a structure containing the touchpad related config options
 * of the input device that can be manipulated.
 *
 * If the input device is not a touchpad this function will return null pointer.
 *
 * \param [in] device   The input device
 *
 * \return              A mutable touchpad config
 */
MirTouchpadConfig* mir_input_device_get_mutable_touchpad_config(
    MirInputDevice* device);

/**
 * Configure the enable click modes for the touchpad.
 *
 * \param [in] conf     The touchpad config
 * \param [in] modes    the enabled click modes
 */
void mir_touchpad_config_set_click_modes(
    MirTouchpadConfig* conf, MirTouchpadClickModes modes);

/**
 * Configure the enabled scroll modes for the touchpad.
 *
 * \param [in] conf     The touchpad config
 * \param [in] modes    the enabled scroll modes
 */
void mir_touchpad_config_set_scroll_modes(
    MirTouchpadConfig* conf, MirTouchpadScrollModes modes);

/**
 * Configure the button for button down scroll mode
 *
 * \param [in] conf     The touchpad config
 * \param [in] button   the button
 */
void mir_touchpad_config_set_button_down_scroll_button(
    MirTouchpadConfig* conf, int button);

/**
 * Configure whether tap to click should be enabled
 *
 * \param [in] conf         The touchpad config
 * \param [in] tap_to_click
 */
void mir_touchpad_config_set_tap_to_click(
    MirTouchpadConfig* conf, bool tap_to_click);

/**
 * Configure whether middle mouse button emulation should be enabled
 *
 * \param [in] conf         The touchpad config
 * \param [in] middle_mouse_button_emulation
 */
void mir_touchpad_config_set_middle_mouse_button_emulation(
    MirTouchpadConfig* conf, bool middle_emulation);

/**
 * Configure whether the touchpad should be turned off while a mouse is attached.
 *
 * \param [in] conf         The touchpad config
 * \param [in] active       disable touchpad with mouse
 */
void mir_touchpad_config_set_disable_with_mouse(
    MirTouchpadConfig* conf, bool active);

/**
 * Configure whether the touchpad should be turned off while typing
 *
 * \param [in] conf         The touchpad config
 * \param [in] active       disable touchpad while typing
 */
void mir_touchpad_config_set_disable_while_typing(
    MirTouchpadConfig* conf, bool active);

#ifdef __cplusplus
}
#endif
/**@}*/
#endif
