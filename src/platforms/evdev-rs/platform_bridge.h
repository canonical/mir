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
#include "/home/matthew/Github/mir/include/platform/mir/console_services.h"

namespace mir
{
class ConsoleServices;
class Device;

namespace input
{
class InputDeviceRegistry;
namespace evdev_rs
{
class Platform;

class DeviceBridgeC
{
public:
    DeviceBridgeC(std::unique_ptr<Device> device, int fd);
    int raw_fd() const;

private:
    std::unique_ptr<Device> device;
    int fd;
};

class PlatformBridgeC
{
public:
    PlatformBridgeC(
        Platform* platform,
        std::shared_ptr<mir::ConsoleServices> const& console,
        std::shared_ptr<mir::input::InputDeviceRegistry> const& input_device_registry);
    virtual ~PlatformBridgeC() = default;
    std::unique_ptr<DeviceBridgeC> acquire_device(int major, int minor) const;
    std::shared_ptr<mir::input::InputDeviceRegistry> input_device_registry() const;

    // void handle_motion_event(
    //     int64_t timestamp_nanoseconds,
    //     float dx,
    //     float dy
    // );
    // void handle_absolute_motion_event(
    //     int64_t timestamp_nanoseconds,
    //     float dx,
    //     float dy
    // );

private:
    Platform* platform;
    std::shared_ptr<mir::ConsoleServices> console;
    std::shared_ptr<mir::input::InputDeviceRegistry> input_device_registry_;
};
}
}
}

#endif // MIR_INPUT_EVDEV_RS_PLATFORM_BRIDGE_H
