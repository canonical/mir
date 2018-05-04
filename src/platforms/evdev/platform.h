/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_INPUT_EVDEV_PLATFORM_H_
#define MIR_INPUT_EVDEV_PLATFORM_H_

#include "libinput_ptr.h"
#include "libinput_device_ptr.h"

#include "mir/input/platform.h"

#include <vector>

struct libinput_device_group;
struct libinput_device;

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
class Dispatchable;
}
namespace input
{
class InputDeviceRegistry;
namespace evdev
{

class LibInputDevice;

class Platform : public input::Platform
{
public:
    Platform(std::shared_ptr<InputDeviceRegistry> const& registry,
             std::shared_ptr<InputReport> const& report,
             std::unique_ptr<udev::Context>&& udev_context);
    std::shared_ptr<mir::dispatch::Dispatchable> dispatchable() override;
    void start() override;
    void stop() override;
    void pause_for_config() override;
    void continue_after_config() override;

private:
    void device_added(libinput_device* dev);
    void device_removed(libinput_device* dev);
    void process_input_events();

    std::shared_ptr<InputReport> const report;
    std::shared_ptr<udev::Context> const udev_context;
    std::shared_ptr<InputDeviceRegistry> const input_device_registry;
    std::shared_ptr<dispatch::MultiplexingDispatchable> const platform_dispatchable;
    std::shared_ptr<::libinput> lib;
    std::shared_ptr<dispatch::ReadableFd> libinput_dispatchable;
    std::shared_ptr<dispatch::Dispatchable> udev_dispatchable;

    std::vector<std::shared_ptr<LibInputDevice>> devices;
    auto find_device(libinput_device_group const* group) -> decltype(devices)::iterator;
};
}
}
}

#endif
