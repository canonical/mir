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

/**
 * Retrieve a structure containing the pointer related configuration options
 * of the input device.
 *
 * If the input device does not control the mouse cursor, there will be no
 * configuration options, and the function will return a null pointer.
 *
 * \param [in] device   The input device
 *
 * \return              The pointer configuration
 */
MirPointerConfiguration const* mir_input_device_get_pointer_configuration(
    MirInputDevice const* device);

/**
 * Retrieve the cursor acceleration profile.
 *
 * \param [in] conf     The pointer configuration
 *
 * \return              The acceleration profile
 */
MirPointerAcceleration mir_pointer_configuration_get_acceleration(
    MirPointerConfiguration const* conf);

/**
 * Retrieve the cursor acceleration bias.
 *
 * The value will be in the range of [-1, 1]:
 *   - 0: default acceleration
 *   - [-1, 0): reduced acceleration
 *   - (0, 1]: increased acceleration
 *
 * \param [in] conf     The pointer configuration
 *
 * \return              The acceleration bias
 */
double mir_pointer_configuration_get_acceleration_bias(
    MirPointerConfiguration const* conf);

/**
 * Retrieve the horizontal scroll scale.
 *
 * The value is a signed linear scale of the horizontal scroll axis. A Negative
 * value indicates 'natural scrolling'.
 *
 * \param [in] conf     The pointer configuration
 *
 * \return              The horizontal scroll scale
 */
double mir_pointer_configuration_get_horizontal_scroll_scale(
    MirPointerConfiguration const* conf);

/**
 * Retrieve the vertical scroll scale.
 *
 * The value is a signed linear scale of the vertical scroll axis. A Negative
 * value indicates 'natural scrolling'.
 *
 * \param [in] conf     The pointer configuration
 *
 * \return              The vertical scroll scale
 */
double mir_pointer_configuration_get_vertical_scroll_scale(
    MirPointerConfiguration const* conf);

/**
 * Retrieve whether the pointer device is configured for right or left handed
 * use.
 *
 * \param [in] conf     The pointer configuration
 *
 * \return              Right or left handed
 */
MirPointerHandedness mir_pointer_configuration_get_handedness(
    MirPointerConfiguration const* conf);

/**
 * Retrieve a structure containing the pointer related configuration options
 * of the input device that can be manipulated.
 *
 * If the input device does not control the mouse cursor, there will be no
 * configuration options, and the function will return a null pointer.
 *
 * \param [in] device   The input device
 *
 * \return              The pointer configuration
 */
MirPointerConfiguration* mir_input_device_get_mutable_pointer_configuration(
    MirInputDevice* device);

/**
 * Set the acceleration mode of the pointer device.
 *
 * See \ref MirPointerAcceleration for reference.
 *
 * \param [in] conf          The pointer configuration
 * \param [in] acceleration  The acceleration mode
 */
void mir_pointer_configuration_set_acceleration(
    MirPointerConfiguration* conf,
    MirPointerAcceleration acceleration);

/**
 * Set the acceleration bias of the pointer device.
 *
 * The acceleration bias must be in the range of [-1, 1]:
 *   - 0: default acceleration
 *   - [-1, 0): reduced acceleration
 *   - (0, 1]: increased acceleration
 *
 * \param [in] conf              The pointer configuration
 * \param [in] acceleration_bias The acceleration bias 
 */
void mir_pointer_configuration_set_acceleration_bias(
    MirPointerConfiguration* conf,
    double acceleration_bias);

/**
 * Set the horizontal scroll scale.
 *
 * The horizontal scroll scale is a signed linear scale of scroll motion along
 * the horizontal axis. Negative scales can be used to configure 'natural
 * scrolling'.
 *
 * \param [in] conf                    The pointer configuration
 * \param [in] horizontal_scroll_scale The horizontal scroll scale
 */
void mir_pointer_configuration_set_horizontal_scroll_scale(
    MirPointerConfiguration* conf,
    double horizontal_scroll_scale);

/**
 * Set the vertical scroll scale.
 *
 * The vertical scroll scale is a signed linear scale of scroll motion along
 * the vertical axis. Negative scales can be used to configure 'natural
 * scrolling'.
 *
 * \param [in] conf                    The pointer configuration
 * \param [in] vertical_scroll_scale The vertical scroll scale
 */
void mir_pointer_configuration_set_vertical_scroll_scale(
    MirPointerConfiguration* conf,
    double vertical_scroll_scale);

/**
 * Configure left and right hand use of the pointer device.
 *
 * This configures which buttons will be used as primary and secondary buttons.
 *
 * \param [in] conf       The pointer configuration
 * \param [in] handedness left or right handed use
 */
void mir_pointer_configuration_set_handedness(
    MirPointerConfiguration* conf,
    MirPointerHandedness handedness);

/**
 * Retrieve a structure containing the touchpad related configuration options
 * of the input device.
 *
 * If the input device is not a touchpad this function will return null pointer.
 *
 * \param [in] device   The input device
 *
 * \return              The touchpad configuration
 */
MirTouchpadConfiguration const* mir_input_device_get_touchpad_configuration(
    MirInputDevice const* device);

/**
 * Retrieve the click modes of the touchpad.
 *
 * See \ref MirTouchpadClickMode for reference.
 *
 * \param [in] conf     The touchpad configuration
 *
 * \return              The touchpad click modes
 */
MirTouchpadClickModes mir_touchpad_configuration_get_click_modes(
    MirTouchpadConfiguration const* conf);

/**
 * Retrieve the scroll modes of the touchpad.
 *
 * See \ref MirTouchpadScrollMode for reference.
 *
 * \param [in] conf     The touchpad configuration
 *
 * \return              The touchpad click modes
 */
MirTouchpadScrollModes mir_touchpad_configuration_get_scroll_modes(
    MirTouchpadConfiguration const* conf);

/**
 * Retrieve the configured button down for button down scroll mode.
 *
 * See \ref MirTouchpadScrollMode for reference.
 *
 * \param [in] conf     The touchpad configuration
 *
 * \return              The touchpad click modes
 */
int mir_touchpad_configuration_get_button_down_scroll_button(
    MirTouchpadConfiguration const* conf);

/**
 * Retrieve whether a tap gesture generates pointer button events.
 *
 * \param [in] conf     The touchpad configuration
 *
 * \return              whether tap to click is enabled
 */
bool mir_touchpad_configuration_get_tap_to_click(
    MirTouchpadConfiguration const* conf);

/**
 * Retrieve whether middle mouse button should be emulated.
 *
 * \param [in] conf     The touchpad configuration
 *
 * \return              whether middle mouse button emulation is enabled
 */
bool mir_touchpad_configuration_get_middle_mouse_button_emulation(
    MirTouchpadConfiguration const* conf);

/**
 * Retrieve whether the touchpad should be disabled when an external pointer
 * device like a mouse is connected.
 *
 * \param [in] conf     The touchpad configuration
 *
 * \return              whether touchpad is disabled with an external mouse
 */
bool mir_touchpad_configuration_get_disable_with_mouse(
    MirTouchpadConfiguration const* conf);

/**
 * Retrieve whether the touchpad events should be blocked while the user types.
 *
 * \param [in] conf     The touchpad configuration
 *
 * \return              whether touchpad is disabled during typing
 */
bool mir_touchpad_configuration_get_disable_while_typing(
    MirTouchpadConfiguration const* conf);

/**
 * Retrieve a structure containing the touchpad related configuration options
 * of the input device that can be manipulated.
 *
 * If the input device is not a touchpad this function will return null pointer.
 *
 * \param [in] device   The input device
 *
 * \return              A mutable touchpad configuration
 */
MirTouchpadConfiguration* mir_input_device_get_mutable_touchpad_configuration(
    MirInputDevice* device);

/**
 * Configure the enable click modes for the touchpad.
 *
 * \param [in] conf     The touchpad configuration
 * \param [in] modes    the enabled click modes
 */
void mir_touchpad_configuration_set_click_modes(
    MirTouchpadConfiguration* conf, MirTouchpadClickModes modes);

/**
 * Configure the enabled scroll modes for the touchpad.
 *
 * \param [in] conf     The touchpad configuration
 * \param [in] modes    the enabled scroll modes
 */
void mir_touchpad_configuration_set_scroll_modes(
    MirTouchpadConfiguration* conf, MirTouchpadScrollModes modes);

/**
 * Configure the button for button down scroll mode
 *
 * \param [in] conf     The touchpad configuration
 * \param [in] button   the button
 */
void mir_touchpad_configuration_set_button_down_scroll_button(
    MirTouchpadConfiguration* conf, int button);

/**
 * Configure whether tap to click should be enabled
 *
 * \param [in] conf         The touchpad configuration
 * \param [in] tap_to_click
 */
void mir_touchpad_configuration_set_tap_to_click(
    MirTouchpadConfiguration* conf, bool tap_to_click);

/**
 * Configure whether middle mouse button emulation should be enabled
 *
 * \param [in] conf         The touchpad configuration
 * \param [in] middle_mouse_button_emulation
 */
void mir_touchpad_configuration_set_middle_mouse_button_emulation(
    MirTouchpadConfiguration* conf, bool middle_emulation);

/**
 * Configure whether the touchpad should be turned off while a mouse is attached.
 *
 * \param [in] conf         The touchpad configuration
 * \param [in] active       disable touchpad with mouse
 */
void mir_touchpad_configuration_set_disable_with_mouse(
    MirTouchpadConfiguration* conf, bool active);

/**
 * Configure whether the touchpad should be turned off while typing
 *
 * \param [in] conf         The touchpad configuration
 * \param [in] active       disable touchpad while typing
 */
void mir_touchpad_configuration_set_disable_while_typing(
    MirTouchpadConfiguration* conf, bool active);
#ifdef __cplusplus
}
#endif
/**@}*/
#endif
