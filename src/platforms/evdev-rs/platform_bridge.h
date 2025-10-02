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

#ifndef MIR_INPUT_EVDEV_RS_PLATFORM_BRIDGE_H
#define MIR_INPUT_EVDEV_RS_PLATFORM_BRIDGE_H

#include <memory>

namespace mir
{
class ConsoleServices;

namespace input
{
namespace evdev_rs
{
class Platform;

class PlatformBridgeC
{
public:
    PlatformBridgeC(Platform* platform, std::shared_ptr<mir::ConsoleServices> const& console);
    virtual ~PlatformBridgeC() = default;
    int acquire_device(int major, int minor) const;
private:
    Platform* platform;
    std::shared_ptr<mir::ConsoleServices> console;
};
}
}
}

#endif // MIR_INPUT_EVDEV_RS_PLATFORM_BRIDGE_H
