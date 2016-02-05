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

namespace
{
inline std::vector<mir::input::DeviceData> const& as_vector(MirInputDevices const* devs)
{
    return *reinterpret_cast<std::vector<mir::input::DeviceData>const*>(devs);
}

}

size_t mir_input_devices_count(MirInputDevices const* devices)
{
    auto device_vector = as_vector(devices);

    return device_vector.size();
}

uint32_t mir_input_devices_get_capabilities(MirInputDevices const* devices, size_t index)
{
    auto device_vector = as_vector(devices);

    return device_vector[index].caps;
}

MirInputDeviceId mir_input_devices_get_id(MirInputDevices const* devices, size_t index)
{
    auto device_vector = as_vector(devices);

    return device_vector[index].id;
}

char const* mir_input_devices_get_name(MirInputDevices const* devices, size_t index)
{
    auto device_vector = as_vector(devices);

    return device_vector[index].name.c_str();
}

char const* mir_input_devices_get_unique_id(MirInputDevices const* devices, size_t index)
{
    auto device_vector = as_vector(devices);

    return device_vector[index].unique_id.c_str();
}
