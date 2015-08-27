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

#ifndef MIR_INPUT_EVDEV_PLATFORM_H_
#define MIR_INPUT_EVDEV_PLATFORM_H_

#include "libinput_ptr.h"
#include "libinput_device_ptr.h"

#include "mir/input/platform.h"

#include <vector>

struct libinput_device_group;

namespace mir
{
namespace udev
{
class Device;
class Monitor;
class Context;
}
namespace dispatch
{
class MultiplexingDispatchable;
class ReadableFd;
}
namespace input
{
class InputDeviceRegistry;
namespace evdev
{

struct MonitorDispatchable;
class LibInputDevice;

class Platform : public input::Platform
{
public:
    Platform(std::shared_ptr<InputDeviceRegistry> const& registry,
             std::shared_ptr<InputReport> const& report,
             std::unique_ptr<udev::Context>&& udev_context,
             std::unique_ptr<udev::Monitor>&& monitor);
    std::shared_ptr<mir::dispatch::Dispatchable> dispatchable() override;
    void start() override;
    void stop() override;

private:
    void scan_for_devices();
    void process_changes();
    void device_added(udev::Device const& dev);
    void device_removed(udev::Device const& dev);
    void device_changed(udev::Device const& dev);
    void process_input_events();

    std::shared_ptr<LibInputDevice> create_device(udev::Device const& dev) const;

    std::shared_ptr<InputReport> const report;
    std::shared_ptr<udev::Context> const udev_context;
    std::unique_ptr<udev::Monitor> const monitor;
    std::shared_ptr<InputDeviceRegistry> const input_device_registry;
    std::shared_ptr<dispatch::MultiplexingDispatchable> const platform_dispatchable;
    std::shared_ptr<dispatch::ReadableFd> const monitor_dispatchable;
    std::shared_ptr<::libinput> const lib;
    std::shared_ptr<dispatch::ReadableFd> const libinput_dispatchable;

    std::vector<std::shared_ptr<LibInputDevice>> devices;
    auto find_device(char const* devnode) -> decltype(devices)::iterator;
    auto find_device(libinput_device *) -> decltype(devices)::iterator;
    auto find_device(libinput_device_group const* group) -> decltype(devices)::iterator;

    friend class MonitorDispatchable;
};
}
}
}

#endif
