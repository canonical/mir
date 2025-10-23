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
#include "mir/log.h"

namespace mi = mir::input;
namespace miers = mi::evdev_rs;
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

    void start(mi::InputSink* sink, mi::EventBuilder* event_builder) override
    {
        impl->start(sink, event_builder);
    }

    void stop() override
    {
        impl->stop();
    }

    mi::InputDeviceInfo get_device_info() override
    {
        auto device_info = impl->get_device_info();
        if (!device_info->valid())
            mir::log_error("Unable to get device info for the input device.");

        return {
            std::string(device_info->name()),
            std::string(device_info->unique_id()),
            mi::DeviceCapabilities(device_info->capabilities()),
        };
    }

    mir::optional_value<mi::PointerSettings> get_pointer_settings() const override
    {
        auto pointer_settings = impl->get_pointer_settings();
        if (!pointer_settings->is_set)
            return mir::optional_value<mi::PointerSettings>{};

        if (pointer_settings->has_error)
        {
            mir::log_warning("Unable to get pointer settings for the input device.");
            return mir::optional_value<mi::PointerSettings>{};
        }

        mi::PointerSettings result;
        result.handedness = static_cast<MirPointerHandedness>(pointer_settings->handedness);
        result.acceleration = static_cast<MirPointerAcceleration>(pointer_settings->acceleration);
        result.cursor_acceleration_bias = pointer_settings->cursor_acceleration_bias;
        result.horizontal_scroll_scale = pointer_settings->horizontal_scroll_scale;
        result.vertical_scroll_scale = pointer_settings->vertical_scroll_scale;
        return result;
    }

    void apply_settings(mi::PointerSettings const& pointer_settings) override
    {
        miers::PointerSettingsC pointer_settings_c;
        pointer_settings_c.handedness = static_cast<MirPointerHandedness>(pointer_settings.handedness);
        pointer_settings_c.acceleration = static_cast<MirPointerAcceleration>(pointer_settings.acceleration);
        pointer_settings_c.cursor_acceleration_bias = pointer_settings.cursor_acceleration_bias;
        pointer_settings_c.horizontal_scroll_scale = pointer_settings.horizontal_scroll_scale;
        pointer_settings_c.vertical_scroll_scale = pointer_settings.vertical_scroll_scale;
        impl->set_pointer_settings(pointer_settings_c);
    }

    mir::optional_value<mi::TouchpadSettings> get_touchpad_settings() const override
    {
        // TODO(mattkae): Handle touch gestures and settings
        return mir::optional_value<mi::TouchpadSettings>{};
    }

    void apply_settings(mi::TouchpadSettings const&) override
    {
        // TODO(mattkae): Handle touch gestures and settings
    }

    mir::optional_value<mi::TouchscreenSettings> get_touchscreen_settings() const override
    {
        // TODO(mattkae): Handle touch gestures and settings
        return mir::optional_value<mi::TouchscreenSettings>{};
    }

    void apply_settings(mi::TouchscreenSettings const&) override
    {
        // TODO(mattkae): Handle touch gestures and settings
    }

private:
    rust::Box<Impl> impl;
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
    return nullptr;
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
