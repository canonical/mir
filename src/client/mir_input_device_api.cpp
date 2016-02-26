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
#include "mir/require.h"

namespace
{
inline std::vector<mir::input::DeviceData>const& as_vector(MirInputConfig const* config)
{
    mir::require(config);
    return *reinterpret_cast<std::vector<mir::input::DeviceData>const*>(config);
}

inline std::vector<mir::input::DeviceData>& as_vector(MirInputConfig* config)
{
    mir::require(config);
    return *reinterpret_cast<std::vector<mir::input::DeviceData>*>(config);
}

inline auto as_device(mir::input::DeviceData* data)
{
    mir::require(data);
    return reinterpret_cast<MirInputDevice*>(data);
}

inline auto as_data(MirInputDevice* device)
{
    mir::require(device);
    return reinterpret_cast<mir::input::DeviceData*>(device);
}

inline auto as_device(mir::input::DeviceData const* data)
{
    mir::require(data);
    return reinterpret_cast<MirInputDevice const*>(data);
}

inline auto as_data(MirInputDevice const* device)
{
    mir::require(device);
    return reinterpret_cast<mir::input::DeviceData const*>(device);
}

}

size_t mir_input_configuration_count(MirInputConfig const* config)
{
    auto& device_vector = as_vector(config);

    return device_vector.size();
}

size_t mir_input_config_device_count(MirInputConfig const* devices)
{
    auto& device_vector = as_vector(devices);

    return device_vector.size();
}

MirInputDevice* mir_input_config_get_mutable_device(MirInputConfig* config, size_t index)
{
    auto& device_vector = as_vector(config);

    mir::require(index < device_vector.size());
    return as_device(&device_vector[index]);
}

MirInputDevice* mir_input_config_get_mutable_device_by_id(MirInputConfig* config, MirInputDeviceId id)
{
    mir::require(config);

    auto& device_vector = as_vector(config);
    for (auto& dev : device_vector)
        if (dev.id == id)
            return as_device(&dev);
    return nullptr;
}

MirInputDevice const* mir_input_config_get_device(MirInputConfig const* config, size_t index)
{
    auto& device_vector = as_vector(config);

    mir::require(index < device_vector.size());
    return as_device(&device_vector[index]);
}

MirInputDevice const* mir_input_config_get_device_by_id(MirInputConfig const* config, MirInputDeviceId id)
{
    mir::require(config);

    auto& device_vector = as_vector(config);
    for (auto const& dev : device_vector)
        if (dev.id == id)
            return as_device(&dev);
    return nullptr;
}

uint32_t mir_input_device_get_capabilities(MirInputDevice const* device)
{
    mir::require(device);
    auto data = as_data(device);
    return data->caps;
}

MirInputDeviceId mir_input_device_get_id(MirInputDevice const* device)
{
    mir::require(device);
    auto data = as_data(device);
    return data->id;
}

char const* mir_input_device_get_name(MirInputDevice const* device)
{
    mir::require(device);
    auto data = as_data(device);
    return data->name.c_str();
}

char const* mir_input_device_get_unique_id(MirInputDevice const* device)
{
    mir::require(device);
    auto data = as_data(device);
    return data->unique_id.c_str();
}
