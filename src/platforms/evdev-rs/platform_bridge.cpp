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

#include "mir/console_services.h"
#include "mir/log.h"
#include "mir/fd.h"
#include "mir/input/pointer_settings.h"
#include "mir/input/touchpad_settings.h"
#include "mir/input/touchscreen_settings.h"
#include "mir/input/input_device_info.h"

namespace mi = mir::input;
namespace miers = mir::input::evdev_rs;

namespace
{
template <typename Impl>
class InputDevice : public mi::InputDevice
{
public:
    explicit InputDevice(rust::Box<Impl>&& impl) : impl(std::move(impl)) {}

    void start(mir::input::InputSink* destination, mir::input::EventBuilder* builder) override
    {
        impl->start(destination, builder);
    }

    void stop() override
    {
        impl->stop();
    }

    mir::input::InputDeviceInfo get_device_info() override
    {
        return impl->get_device_info();
    }

    mir::optional_value<mir::input::PointerSettings> get_pointer_settings() const override
    {
        return mir::optional_value<mir::input::PointerSettings>{};
    }

    void apply_settings(mir::input::PointerSettings const&) override
    {

    }

    mir::optional_value<mir::input::TouchpadSettings> get_touchpad_settings() const override
    {
        return mir::optional_value<mir::input::TouchpadSettings>{};
    }

    void apply_settings(mir::input::TouchpadSettings const&) override
    {

    }

    mir::optional_value<mir::input::TouchscreenSettings> get_touchscreen_settings() const override
    {
        return mir::optional_value<mir::input::TouchscreenSettings>{};
    }

    void apply_settings(mir::input::TouchscreenSettings const&) override
    {

    }

private:
    rust::Box<Impl> impl;
};
}

miers::PlatformBridgeC::PlatformBridgeC(
    Platform* platform,
    std::shared_ptr<mir::ConsoleServices> const& console)
    : platform(platform), console(console) {}

std::unique_ptr<miers::DeviceBridgeC> miers::PlatformBridgeC::acquire_device(int major, int minor) const
{
    mir::log_info("Acquiring device: %d.%d", major, minor);
    auto observer = platform->create_device_observer();
    DeviceObserverWithFd* raw_observer = observer.get();
    auto future = console->acquire_device(major, minor, std::move(observer));
    future.wait();
    return std::make_unique<DeviceBridgeC>(future.get(), raw_observer->raw_fd().value());
}

std::shared_ptr<mi::InputDevice> miers::PlatformBridgeC::create_input_device() const
{
    return nullptr;
    // return std::make_shared<::InputDevice>(miers::evdev_rs_create_input_device());
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
