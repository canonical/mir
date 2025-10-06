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

#include "platform_bridge.h"

#include "platform.h"
#include "mir_platforms_evdev_rs/src/lib.rs.h"

#include "mir/console_services.h"
#include "mir/log.h"
#include "mir/fd.h"

namespace miers = mir::input::evdev_rs;

miers::PlatformBridgeC::PlatformBridgeC(
    Platform* platform,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mir::input::InputDeviceRegistry> const& input_device_registry)
    : platform(platform),
      console(console),
    input_device_registry_(input_device_registry) {}

std::unique_ptr<miers::DeviceBridgeC> miers::PlatformBridgeC::acquire_device(int major, int minor) const
{
    mir::log_info("Acquiring device: %d.%d", major, minor);
    auto observer = platform->create_device_observer();
    DeviceObserverWithFd* raw_observer = observer.get();
    auto future = console->acquire_device(major, minor, std::move(observer));
    future.wait();
    return std::make_unique<DeviceBridgeC>(future.get(), raw_observer->raw_fd().value());
}

std::shared_ptr<mir::input::InputDeviceRegistry> miers::PlatformBridgeC::input_device_registry() const
{
    return input_device_registry_;
}

miers::DeviceBridgeC::DeviceBridgeC(std::unique_ptr<Device> device, int fd)
    : device(std::move(device)),
      fd(fd)
{
}

int miers::DeviceBridgeC::raw_fd() const
{
    return fd;
}
