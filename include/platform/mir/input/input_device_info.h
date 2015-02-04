/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_INPUT_INPUT_DEVICE_INFO_H_
#define MIR_INPUT_INPUT_DEVICE_INFO_H_

#include "mir/input/device_capability.h"

#include <cstdint>
#include <string>

namespace mir
{
namespace input
{

#if 0
struct MotionRange
{
    uint32_t axis;
};
#endif

struct InputDeviceInfo
{
    int32_t id;
    std::string name;
    std::string unique_id;
    mir::input::DeviceCapability capabilities; // see mir::input::DeviceCapability
    // DeviceLocation location; builtin or external through usb or bt
    // std::vector<MotionRange> value_ranges; // declar joystick action ranges
    // std::vector<Button> buttons; // available buttons ?// keys?
    // std::vector<Switch> switches; // available ?
    // std::vector<KeyData> keys; // available ?
};

inline bool operator==(InputDeviceInfo const& lhs, InputDeviceInfo const& rhs)
{
    return lhs.id == rhs.id && lhs.name == rhs.name && lhs.unique_id == rhs.unique_id;
}

}
}

#endif
