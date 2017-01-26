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
#include "mir/input/mir_pointer_config.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/require.h"
#include "handle_event_exception.h"

size_t mir_input_config_device_count(MirInputConfig const* input_config) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);
    return input_config->size();
})

MirInputDevice* mir_input_config_get_mutable_device(MirInputConfig* input_config, size_t index) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);
    mir::require(index < input_config->size());
    return &input_config->get_device_config_by_index(index);
})

MirInputDevice* mir_input_config_get_mutable_device_by_id(MirInputConfig* input_config, MirInputDeviceId id) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);

    return input_config->get_device_config_by_id(id);
})

MirInputDevice const* mir_input_config_get_device(MirInputConfig const* input_config, size_t index) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);
    mir::require(index < input_config->size());
    return &input_config->get_device_config_by_index(index);
})

MirInputDevice const* mir_input_config_get_device_by_id(MirInputConfig const* input_config, MirInputDeviceId id) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);

    return input_config->get_device_config_by_id(id);
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

MirPointerConfig const* mir_input_device_get_pointer_config(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    if (device->has_pointer_config())
        return &device->pointer_config();

    return nullptr;
})

MirPointerAcceleration mir_pointer_config_get_acceleration(MirPointerConfig const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->acceleration();
})

double mir_pointer_config_get_acceleration_bias(MirPointerConfig const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->cursor_acceleration_bias();
})

double mir_pointer_config_get_horizontal_scroll_scale(MirPointerConfig const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->horizontal_scroll_scale();
})

double mir_pointer_config_get_vertical_scroll_scale(MirPointerConfig const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->vertical_scroll_scale();
})

MirPointerHandedness mir_pointer_config_get_handedness(MirPointerConfig const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->handedness();
})

MirPointerConfig* mir_input_device_get_mutable_pointer_config(MirInputDevice* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    if (device->has_pointer_config())
        return &device->pointer_config();
    return nullptr;
})

void mir_pointer_config_set_acceleration(MirPointerConfig* conf, MirPointerAcceleration acceleration) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->acceleration(acceleration);
})

void mir_pointer_config_set_acceleration_bias(MirPointerConfig* conf, double acceleration_bias) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->cursor_acceleration_bias(acceleration_bias);
})

void mir_pointer_config_set_horizontal_scroll_scale(MirPointerConfig* conf,
                                                           double horizontal_scroll_scale) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->horizontal_scroll_scale(horizontal_scroll_scale);
})

void mir_pointer_config_set_vertical_scroll_scale(MirPointerConfig* conf, double vertical_scroll_scale) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->vertical_scroll_scale(vertical_scroll_scale);
})

void mir_pointer_config_set_handedness(MirPointerConfig* conf, MirPointerHandedness handedness) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->handedness(handedness);
})

MirTouchpadConfig const* mir_input_device_get_touchpad_config(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    if (device->has_touchpad_config())
        return &device->touchpad_config();
    return nullptr;
})

MirTouchpadClickModes mir_touchpad_config_get_click_modes(MirTouchpadConfig const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->click_mode();
})

MirTouchpadScrollModes mir_touchpad_config_get_scroll_modes(MirTouchpadConfig const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->scroll_mode();
})

int mir_touchpad_config_get_button_down_scroll_button(MirTouchpadConfig const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->button_down_scroll_button();
})

bool mir_touchpad_config_get_tap_to_click(MirTouchpadConfig const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->tap_to_click();
})

bool mir_touchpad_config_get_middle_mouse_button_emulation(MirTouchpadConfig const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->middle_mouse_button_emulation();
})

bool mir_touchpad_config_get_disable_with_mouse(MirTouchpadConfig const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->disable_with_mouse();
})

bool mir_touchpad_config_get_disable_while_typing(MirTouchpadConfig const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return conf->disable_while_typing();
})

MirTouchpadConfig* mir_input_device_get_mutable_touchpad_config(MirInputDevice* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    if (device->has_touchpad_config())
        return &device->touchpad_config();
    return nullptr;
})

void mir_touchpad_config_set_click_modes(MirTouchpadConfig* conf, MirTouchpadClickModes modes) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->click_mode(modes);
})

void mir_touchpad_config_set_scroll_modes(MirTouchpadConfig* conf, MirTouchpadScrollModes modes) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->scroll_mode(modes);
})

void mir_touchpad_config_set_button_down_scroll_button(MirTouchpadConfig* conf, int button) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->button_down_scroll_button(button);
})

void mir_touchpad_config_set_tap_to_click(MirTouchpadConfig* conf, bool tap_to_click) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->tap_to_click(tap_to_click);
})

void mir_touchpad_config_set_middle_mouse_button_emulation(MirTouchpadConfig* conf, bool middle_emulation) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->middle_mouse_button_emulation(middle_emulation);
})

void mir_touchpad_config_set_disable_with_mouse(MirTouchpadConfig* conf, bool active) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->disable_with_mouse(active);
})

void mir_touchpad_config_set_disable_while_typing(MirTouchpadConfig* conf, bool active) MIR_HANDLE_EVENT_EXCEPTION(
{
    conf->disable_while_typing(active);
})
