/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_DEVICE_H_
#define MIR_INPUT_DEVICE_H_

#include "mir/input/device_capability.h"
#include "mir_toolkit/event.h"

#include <memory>

namespace mir
{
namespace input
{

class PointerSettings;
class TouchPadSettings;

class Device
{
public:
    Device() = default;
    virtual ~Device() = default;
    virtual MirInputDeviceId id() const = 0;
    virtual DeviceCapabilities capabilities() const = 0;
    virtual std::string name() const = 0;
    virtual std::string unique_id() const = 0;

private:
    Device(Device const&) = delete;
    Device& operator=(Device const&) = delete;
};

}
}

#endif
