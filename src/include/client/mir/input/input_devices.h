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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_INPUT_DEVICES_H_
#define MIR_INPUT_INPUT_DEVICES_H_

#include "mir_toolkit/mir_input_device.h"
#include "mir_toolkit/client_types.h"

#include <vector>
#include <mutex>
#include <string>

namespace mir
{
namespace input
{
struct DeviceData
{
    DeviceData() = default;
    DeviceData(MirInputDeviceId id, uint32_t caps, std::string const& name, std::string const& unique_id)
        : id(id), caps(caps), name(name), unique_id(unique_id)
    {
    }
    MirInputDeviceId id;
    uint32_t caps;
    std::string name;
    std::string unique_id;
};

class InputDevices
{
public:
    InputDevices() = default;
    void update_devices(std::vector<DeviceData> && data);
    std::vector<DeviceData> copy_devices();
    void set_change_callback(std::function<void()> const& callback);
private:
    std::mutex devices_access;
    std::vector<DeviceData> devices;
    std::function<void()> callback;
};

}
}

#endif
