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

#include "platform.h"
#include "platform_bridge.h"

#include "mir/dispatch/dispatchable.h"
#include "rust/cxx.h"
#include "mir/optional_value.h"
#include "mir_platforms_evdev_rs/src/lib.rs.h"
#include "mir/input/pointer_settings.h"
#include "mir/input/touchpad_settings.h"
#include "mir/input/touchscreen_settings.h"
#include "mir/input/input_device_info.h"

namespace mi = mir::input;
namespace miers = mir::input::evdev_rs;
namespace md = mir::dispatch;

namespace
{
template <typename Impl>
class DeviceObserverWrapper : public miers::DeviceObserverWithFd
{
public:
    explicit DeviceObserverWrapper(rust::Box<Impl>&& box)
        : box(std::move(box)) {}

    void activated(mir::Fd&& device_fd) override
    {
        fd = device_fd;
        box->activated(device_fd);
    }

    void suspended() override
    {
        box->suspended();
    }

    void removed() override
    {
        box->removed();
    }

    std::optional<mir::Fd> raw_fd() const override
    {
        return fd;
    }

private:
    rust::Box<Impl> box;
    std::optional<mir::Fd> fd;
};

template <typename Impl>
class InputDevice : public mi::InputDevice
{
public:
    explicit InputDevice(rust::Box<Impl>&& impl) : impl(std::move(impl)) {}

    void start(mir::input::InputSink* sink, mir::input::EventBuilder* event_builder) override
    {
        impl->start(sink, event_builder);
    }

    void stop() override
    {
        impl->stop();
    }

    mir::input::InputDeviceInfo get_device_info() override
    {
        auto device_info = impl->get_device_info();
        auto const name = device_info->name();
        return {
            std::string(device_info->name()),
            std::string(device_info->unique_id()),
            mir::input::DeviceCapabilities(0)
        };
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

class DispatchableStub : public md::Dispatchable
{
public:
    mir::Fd watch_fd() const override
    {
        return mir::Fd{0};
    }

    bool dispatch(md::FdEvents) override
    {
        return false;
    }

    mir::dispatch::FdEvents relevant_events() const override
    {
        return md::FdEvent::readable;
    }
};
}

class miers::Platform::Self
{
public:
    Self(Platform* platform, std::shared_ptr<ConsoleServices> const& console,
        std::shared_ptr<InputDeviceRegistry> const& input_device_registry)
        : platform_impl(evdev_rs_create(std::make_shared<PlatformBridgeC>(platform, console), input_device_registry)) {}

    rust::Box<PlatformRs> platform_impl;
};

miers::Platform::Platform(
    std::shared_ptr<ConsoleServices> const& console,
    std::shared_ptr<InputDeviceRegistry> const& input_device_registry)
    : self(std::make_shared<Self>(this, console, input_device_registry))
{
}

std::shared_ptr<mir::dispatch::Dispatchable> miers::Platform::dispatchable()
{
    return std::make_shared<DispatchableStub>();
}

void miers::Platform::start()
{
    self->platform_impl->start();
}

void miers::Platform::continue_after_config()
{
    self->platform_impl->continue_after_config();
}

void miers::Platform::pause_for_config()
{
    self->platform_impl->pause_for_config();
}

void miers::Platform::stop()
{
    self->platform_impl->stop();
}

std::unique_ptr<miers::DeviceObserverWithFd> miers::Platform::create_device_observer()
{
    return std::make_unique<DeviceObserverWrapper<DeviceObserverRs>>(
        self->platform_impl->create_device_observer());
}

std::shared_ptr<mi::InputDevice> miers::Platform::create_input_device(int device_id) const
{
    return std::make_shared<::InputDevice<InputDeviceRs>>(self->platform_impl->create_input_device(device_id));
}
