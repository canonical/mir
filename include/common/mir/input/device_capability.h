/*
 * Copyright © 2014-2015 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_DEVICE_CAPABILITY_H_
#define MIR_INPUT_DEVICE_CAPABILITY_H_

#include "mir/flags.h"

#include <cstdint>

namespace mir
{
namespace input
{

enum class DeviceCapability : uint32_t
{
    unknown     = 0,
    pointer     = 1<<1,
    keyboard    = 1<<2,
    touchpad    = 1<<3,
    touchscreen = 1<<4,
    gamepad     = 1<<5,
    joystick    = 1<<6,
    switch_     = 1<<7,
    multitouch  = 1<<8,   // multitouch capable
    alpha_numeric = 1<<9 // enough keys for text entry
};

DeviceCapability mir_enable_enum_bit_operators(DeviceCapability);
using DeviceCapabilities = mir::Flags<DeviceCapability>;

}
}

#endif
