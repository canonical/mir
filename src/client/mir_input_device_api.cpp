/*
 * Copyright © 2016 Canonical Ltd.
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

#include "mir_toolkit/mir_input_device.h"
#include "mir/input/input_devices.h"
#include "mir/input/mir_pointer_configuration.h"
#include "mir/input/mir_touchpad_configuration.h"
#include "mir/require.h"
#include "handle_event_exception.h"

size_t mir_input_config_device_count(MirInputConfiguration const* input_config) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);
    return input_config->size();
})

MirInputDevice* mir_input_config_get_mutable_device(MirInputConfiguration* input_config, size_t index) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);
    mir::require(index < input_config->size());
    return &input_config->get_device_configuration_by_index(index);
})

MirInputDevice* mir_input_config_get_mutable_device_by_id(MirInputConfiguration* input_config, MirInputDeviceId id) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);

    return input_config->get_device_configuration_by_id(id);
})

MirInputDevice const* mir_input_config_get_device(MirInputConfiguration const* input_config, size_t index) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);
    mir::require(index < input_config->size());
    return &input_config->get_device_configuration_by_index(index);
})

MirInputDevice const* mir_input_config_get_device_by_id(MirInputConfiguration const* input_config, MirInputDeviceId id) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);

    return input_config->get_device_configuration_by_id(id);
})

uint32_t mir_input_device_get_capabilities(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    return device->capabilities().value();
})

MirInputDeviceId mir_input_device_get_id(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    return device->id();
})

char const* mir_input_device_get_name(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    return device->name().c_str();
})

char const* mir_input_device_get_unique_id(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    return device->unique_id().c_str();
})

MirPointerConfiguration const* mir_input_device_get_pointer_configuration(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    if (device->has_pointer_configuration())
        return &device->pointer_configuration();

    return nullptr;
})

MirPointerAcceleration mir_pointer_configuration_get_acceleration(MirPointerConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->acceleration();
})

double mir_pointer_configuration_get_acceleration_bias(MirPointerConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->cursor_acceleration_bias();
})

double mir_pointer_configuration_get_horizontal_scroll_scale(MirPointerConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->horizontal_scroll_scale();
})

double mir_pointer_configuration_get_vertical_scroll_scale(MirPointerConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->vertical_scroll_scale();
})

MirPointerHandedness mir_pointer_configuration_get_handedness(MirPointerConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->handedness();
})

MirPointerConfiguration* mir_input_device_get_mutable_pointer_configuration(MirInputDevice* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    if (device->has_pointer_configuration())
        return &device->pointer_configuration();
    return nullptr;
})

void mir_pointer_configuration_set_acceleration(MirPointerConfiguration* conf, MirPointerAcceleration acceleration) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->acceleration(acceleration);
})

void mir_pointer_configuration_set_acceleration_bias(MirPointerConfiguration* conf, double acceleration_bias) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->cursor_acceleration_bias(acceleration_bias);
})

void mir_pointer_configuration_set_horizontal_scroll_scale(MirPointerConfiguration* conf,
                                                           double horizontal_scroll_scale) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->horizontal_scroll_scale(horizontal_scroll_scale);
})

void mir_pointer_configuration_set_vertical_scroll_scale(MirPointerConfiguration* conf, double vertical_scroll_scale) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->vertical_scroll_scale(vertical_scroll_scale);
})

void mir_pointer_configuration_set_handedness(MirPointerConfiguration* conf, MirPointerHandedness handedness) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->handedness(handedness);
})

MirTouchpadConfiguration const* mir_input_device_get_touchpad_configuration(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{

    if (device->has_touchpad_configuration())
        return &device->touchpad_configuration();
    return nullptr;
})

MirTouchpadClickModes mir_touchpad_configuration_get_click_modes(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->click_mode();
})

MirTouchpadScrollModes mir_touchpad_configuration_get_scroll_modes(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->scroll_mode();
})

int mir_touchpad_configuration_get_button_down_scroll_button(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->button_down_scroll_button();
})

bool mir_touchpad_configuration_get_tap_to_click(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->tap_to_click();
})

bool mir_touchpad_configuration_get_middle_mouse_button_emulation(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->middle_mouse_button_emulation();
})

bool mir_touchpad_configuration_get_disable_with_mouse(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->disable_with_mouse();
})

bool mir_touchpad_configuration_get_disable_while_typing(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->disable_while_typing();
})

MirTouchpadConfiguration* mir_input_device_get_mutable_touchpad_configuration(MirInputDevice* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    if (device->has_touchpad_configuration())
        return &device->touchpad_configuration();
    return nullptr;
})

void mir_touchpad_configuration_set_click_modes(MirTouchpadConfiguration* conf, MirTouchpadClickModes modes) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->click_mode(modes);
})

void mir_touchpad_configuration_set_scroll_modes(MirTouchpadConfiguration* conf, MirTouchpadScrollModes modes) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->scroll_mode(modes);
})

void mir_touchpad_configuration_set_button_down_scroll_button(MirTouchpadConfiguration* conf, int button) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->button_down_scroll_button(button);
})

void mir_touchpad_configuration_set_tap_to_click(MirTouchpadConfiguration* conf, bool tap_to_click) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->tap_to_click(tap_to_click);
})

void mir_touchpad_configuration_set_middle_mouse_button_emulation(MirTouchpadConfiguration* conf, bool middle_emulation) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->middle_mouse_button_emulation(middle_emulation);
})

void mir_touchpad_configuration_set_disable_with_mouse(MirTouchpadConfiguration* conf, bool active) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->disable_with_mouse(active);
})

void mir_touchpad_configuration_set_disable_while_typing(MirTouchpadConfiguration* conf, bool active) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->disable_while_typing(active);
})
