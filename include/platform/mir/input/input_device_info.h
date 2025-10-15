/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_INPUT_INPUT_DEVICE_INFO_H_
#define MIR_INPUT_INPUT_DEVICE_INFO_H_

#include <mir/input/device_capability.h>

#include <cstdint>
#include <string>

namespace mir
{
namespace input
{

struct InputDeviceInfo
{
    std::string name;
    std::string unique_id;
    DeviceCapabilities capabilities;
};

}
}
#endif
