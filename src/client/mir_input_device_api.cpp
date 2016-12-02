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
#include "mir/input/pointer_configuration.h"
#include "mir/input/touchpad_configuration.h"
#include "mir/require.h"
#include "handle_event_exception.h"

namespace mi = mir::input;

namespace
{
inline auto as_input_configuration(MirInputConfig const* config)
{
    mir::require(config);
    return reinterpret_cast<mi::InputConfiguration const*>(config);
}

inline auto as_input_configuration(MirInputConfig* config)
{
    mir::require(config);
    return reinterpret_cast<mi::InputConfiguration*>(config);
}

inline auto as_device(mi::DeviceConfiguration * data)
{
    mir::require(data);
    auto ret = reinterpret_cast<MirInputDevice*>(data);
    return ret;
}

inline auto as_device(mi::DeviceConfiguration const* data)
{
    mir::require(data);
    auto ret = reinterpret_cast<MirInputDevice const*>(data);
    return ret;
}

inline auto as_device_configuration(MirInputDevice const* device)
{
    mir::require(device);
    auto ret = reinterpret_cast<mi::DeviceConfiguration const*>(device);
    return ret;
}

inline auto as_device_configuration(MirInputDevice* device)
{
    mir::require(device);
    auto ret = reinterpret_cast<mi::DeviceConfiguration*>(device);
    return ret;
}

inline auto as_pointer_config(MirPointerConfiguration const* config)
{
    mir::require(config);
    auto ret = reinterpret_cast<mi::PointerConfiguration const*>(config);
    return ret;
}

inline auto as_pointer_config(MirPointerConfiguration* config)
{
    mir::require(config);
    return reinterpret_cast<mi::PointerConfiguration*>(config);
}

inline auto as_touchpad_config(MirTouchpadConfiguration const* config)
{
    mir::require(config);
    return reinterpret_cast<mi::TouchpadConfiguration const*>(config);
}

inline auto as_touchpad_config(MirTouchpadConfiguration* config)
{
    mir::require(config);
    return reinterpret_cast<mi::TouchpadConfiguration*>(config);
}
}

size_t mir_input_config_device_count(MirInputConfig const* input_config) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);
    auto device_infos = as_input_configuration(input_config);

    return device_infos->size();
})

MirInputDevice* mir_input_config_get_mutable_device(MirInputConfig* input_config, size_t index) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);
    auto device_infos = as_input_configuration(input_config);

    mir::require(index < device_infos->size());
    return as_device(&device_infos->get_device_configuration_by_index(index));
})

MirInputDevice* mir_input_config_get_mutable_device_by_id(MirInputConfig* input_config, MirInputDeviceId id) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);

    auto device_infos = as_input_configuration(input_config);
    return as_device(device_infos->get_device_configuration_by_id(id));
})

MirInputDevice const* mir_input_config_get_device(MirInputConfig const* input_config, size_t index) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);
    auto device_infos = as_input_configuration(input_config);

    mir::require(index < device_infos->size());
    return as_device(&device_infos->get_device_configuration_by_index(index));
})

MirInputDevice const* mir_input_config_get_device_by_id(MirInputConfig const* input_config, MirInputDeviceId id) MIR_HANDLE_EVENT_EXCEPTION(
{
    mir::require(input_config);

    auto device_infos = as_input_configuration(input_config);
    return as_device(device_infos->get_device_configuration_by_id(id));
})

uint32_t mir_input_device_get_capabilities(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_device_configuration(device)->capabilities().value();
})

MirInputDeviceId mir_input_device_get_id(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_device_configuration(device)->id();
})

char const* mir_input_device_get_name(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_device_configuration(device)->name().c_str();
})

char const* mir_input_device_get_unique_id(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    auto device_info = as_device_configuration(device);
    return device_info->unique_id().c_str();
})

MirPointerConfiguration const* mir_input_device_get_pointer_configuration(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    auto device_info = as_device_configuration(device);
    if (device_info->has_pointer_configuration())
        return reinterpret_cast<MirPointerConfiguration const*>(&device_info->pointer_configuration());

    return nullptr;
})

MirPointerAcceleration mir_pointer_configuration_get_acceleration(MirPointerConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_pointer_config(conf)->acceleration;
})

double mir_pointer_configuration_get_acceleration_bias(MirPointerConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_pointer_config(conf)->cursor_acceleration_bias;
})

double mir_pointer_configuration_get_horizontal_scroll_scale(MirPointerConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_pointer_config(conf)->horizontal_scroll_scale;
})

double mir_pointer_configuration_get_vertical_scroll_scale(MirPointerConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_pointer_config(conf)->vertical_scroll_scale;
})

MirPointerHandedness mir_pointer_configuration_get_handedness(MirPointerConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_pointer_config(conf)->handedness;
})

MirPointerConfiguration* mir_input_device_get_mutable_pointer_configuration(MirInputDevice* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    auto device_info = as_device_configuration(device);
    if (device_info->has_pointer_configuration())
        return reinterpret_cast<MirPointerConfiguration*>(&device_info->pointer_configuration());
    return nullptr;
})

void mir_pointer_configuration_set_acceleration(MirPointerConfiguration* conf, MirPointerAcceleration acceleration) MIR_HANDLE_EVENT_EXCEPTION(
{
    as_pointer_config(conf)->acceleration = acceleration;
})

void mir_pointer_configuration_set_acceleration_bias(MirPointerConfiguration* conf, double acceleration_bias) MIR_HANDLE_EVENT_EXCEPTION(
{
    as_pointer_config(conf)->cursor_acceleration_bias = acceleration_bias;
})

void mir_pointer_configuration_set_horizontal_scroll_scale(MirPointerConfiguration* conf,
                                                           double horizontal_scroll_scale) MIR_HANDLE_EVENT_EXCEPTION(
{
    as_pointer_config(conf)->horizontal_scroll_scale = horizontal_scroll_scale;
})

void mir_pointer_configuration_set_vertical_scroll_scale(MirPointerConfiguration* conf, double vertical_scroll_scale) MIR_HANDLE_EVENT_EXCEPTION(
{
    as_pointer_config(conf)->vertical_scroll_scale = vertical_scroll_scale;
})

void mir_pointer_configuration_set_handedness(MirPointerConfiguration* conf, MirPointerHandedness handedness) MIR_HANDLE_EVENT_EXCEPTION(
{
    as_pointer_config(conf)->handedness = handedness;
})

MirTouchpadConfiguration const* mir_input_device_get_touchpad_configuration(MirInputDevice const* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    auto device_info = as_device_configuration(device);

    if (device_info->has_touchpad_configuration())
        return reinterpret_cast<MirTouchpadConfiguration const*>(&device_info->touchpad_configuration());
    return nullptr;
})

MirTouchpadClickModes mir_touchpad_configuration_get_click_modes(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_touchpad_config(conf)->click_mode;
})

MirTouchpadScrollModes mir_touchpad_configuration_get_scroll_modes(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_touchpad_config(conf)->scroll_mode;
})

int mir_touchpad_configuration_get_button_down_scroll_button(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_touchpad_config(conf)->button_down_scroll_button;
})

bool mir_touchpad_configuration_get_tap_to_click(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_touchpad_config(conf)->tap_to_click;
})

bool mir_touchpad_configuration_get_middle_mouse_button_emulation(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_touchpad_config(conf)->middle_mouse_button_emulation;
})

bool mir_touchpad_configuration_get_disable_with_mouse(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_touchpad_config(conf)->disable_with_mouse;
})

bool mir_touchpad_configuration_get_disable_while_typing(MirTouchpadConfiguration const* conf) MIR_HANDLE_EVENT_EXCEPTION(
{
    return as_touchpad_config(conf)->disable_while_typing;
})

MirTouchpadConfiguration* mir_input_device_get_mutable_touchpad_configuration(MirInputDevice* device) MIR_HANDLE_EVENT_EXCEPTION(
{
    auto device_info = as_device_configuration(device);

    if (device_info->has_touchpad_configuration())
        return reinterpret_cast<MirTouchpadConfiguration*>(&device_info->touchpad_configuration());
    return nullptr;
})

void mir_touchpad_configuration_set_click_modes(MirTouchpadConfiguration* conf, MirTouchpadClickModes modes) MIR_HANDLE_EVENT_EXCEPTION(
{
    as_touchpad_config(conf)->click_mode = modes;
})

void mir_touchpad_configuration_set_scroll_modes(MirTouchpadConfiguration* conf, MirTouchpadScrollModes modes) MIR_HANDLE_EVENT_EXCEPTION(
{
    as_touchpad_config(conf)->scroll_mode = modes;
})

void mir_touchpad_configuration_set_button_down_scroll_button(MirTouchpadConfiguration* conf, int button) MIR_HANDLE_EVENT_EXCEPTION(
{
    as_touchpad_config(conf)->button_down_scroll_button = button;
})

void mir_touchpad_configuration_set_tap_to_click(MirTouchpadConfiguration* conf, bool tap_to_click) MIR_HANDLE_EVENT_EXCEPTION(
{
    as_touchpad_config(conf)->tap_to_click = tap_to_click;
})

void mir_touchpad_configuration_set_middle_mouse_button_emulation(MirTouchpadConfiguration* conf, bool middle_emulation) MIR_HANDLE_EVENT_EXCEPTION(
{
    auto touchpad_info = as_touchpad_config(conf);
    touchpad_info->middle_mouse_button_emulation = middle_emulation;
})

void mir_touchpad_configuration_set_disable_with_mouse(MirTouchpadConfiguration* conf, bool active) MIR_HANDLE_EVENT_EXCEPTION(
{
    auto touchpad_info = as_touchpad_config(conf);
    touchpad_info->disable_with_mouse = active;
})

void mir_touchpad_configuration_set_disable_while_typing(MirTouchpadConfiguration* conf, bool active) MIR_HANDLE_EVENT_EXCEPTION(
{
    auto touchpad_info = as_touchpad_config(conf);
    touchpad_info->disable_while_typing = active;
})
