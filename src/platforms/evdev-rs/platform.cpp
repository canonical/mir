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
#include "input_report.h"

#include <mir/optional_value.h>
#include <mir/dispatch/dispatchable.h>
#include <mir/dispatch/multiplexing_dispatchable.h>
#include <mir/dispatch/readable_fd.h>
#include <mir_platforms_evdev_rs/src/lib.rs.h>
#include <rust/cxx.h>
#include <mir/input/pointer_settings.h>
#include <mir/input/touchpad_settings.h>
#include <mir/input/touchscreen_settings.h>
#include <mir/input/input_device_info.h>
#include <mir/dispatch/action_queue.h>
#include <mir/log.h>

#include <unistd.h>

namespace mi = mir::input;
namespace miers = mi::evdev_rs;
namespace md = mir::dispatch;

/// Thin observer that forwards device lifecycle events to the Rust platform
/// via an ActionQueue (ensuring they run on the input dispatch thread).
class InputDeviceObserver : public mir::Device::Observer
{
public:
    InputDeviceObserver(
        std::string devnode,
        uint64_t devnum,
        std::shared_ptr<md::ActionQueue> device_queue,
        std::weak_ptr<miers::Platform::Self> platform_self)
        : devnode{std::move(devnode)},
          devnum{devnum},
          device_queue{std::move(device_queue)},
          platform_self{std::move(platform_self)}
    {
    }

    void activated(mir::Fd&& device_fd) override;
    void suspended() override;
    void removed() override;

private:
    std::string devnode;
    uint64_t devnum;
    std::shared_ptr<md::ActionQueue> device_queue;
    std::weak_ptr<miers::Platform::Self> platform_self;
};

namespace
{

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
        miers::SetPointerSettingsData pointer_settings_c;
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

} // namespace

class miers::Platform::Self
{
public:
    Self(Platform* platform,
         std::shared_ptr<ConsoleServices> const& console,
         std::shared_ptr<InputDeviceRegistry> const& input_device_registry,
         std::shared_ptr<mi::InputReport> const& report)
        : bridge{std::make_shared<PlatformBridge>(platform, console)},
          platform_impl{evdev_rs_create(
              bridge,
              input_device_registry,
              std::make_unique<miers::InputReport>(report))},
          dispatchable{std::make_shared<md::MultiplexingDispatchable>()}
    {
    }

    std::shared_ptr<PlatformBridge> bridge;
    rust::Box<miers::PlatformRs> platform_impl;
    std::shared_ptr<md::ReadableFd> libinput_dispatch;
    std::shared_ptr<md::ReadableFd> udev_dispatch;
    std::shared_ptr<md::MultiplexingDispatchable> dispatchable;
    std::shared_ptr<md::ActionQueue> device_queue;
};

void InputDeviceObserver::activated(mir::Fd&& device_fd)
{
    mir::log_info("evdev-rs: observer activated() for %s (devnum=%lu, fd=%d)",
                   devnode.c_str(), static_cast<unsigned long>(devnum), static_cast<int>(device_fd));

    auto const locked = platform_self.lock();
    if (!locked)
    {
        mir::log_error("evdev-rs: activated() ignored — platform is not allocated");
        return;
    }

    auto& platform_impl = locked->platform_impl;
    if (!platform_impl->is_running())
    {
        mir::log_info("evdev-rs: activated() ignored — platform not running");
        return;
    }

    mir::log_info("evdev-rs: enqueuing on_device_activated for %s (duped fd=%d)", devnode.c_str(), duped);
    device_queue->enqueue(
        [devnode = devnode, devnum = devnum, device_fd = device_fd, &platform_impl = platform_impl]()
        {
            // dup() so that mir::Fd's destructor closes the original without
            // invalidating the fd we hand to libinput via open_restricted().
            int const duped = ::dup(static_cast<int>(device_fd));
            if (duped < 0)
            {
                mir::log_error("evdev-rs: dup() failed in activated() for %s", devnode.c_str());
                return;
            }

            if (!platform_impl->is_running())
            {
                mir::log_info("evdev-rs: on_device_activated dequeued but platform not running, closing fd=%d", fd);
                ::close(duped);
                return;
            }
            platform_impl->on_device_activated(devnode, devnum, duped);
        });
}

void InputDeviceObserver::suspended()
{
    mir::log_info("evdev-rs: observer suspended() for %s (devnum=%lu)",
                   devnode.c_str(), static_cast<unsigned long>(devnum));

    auto const locked = platform_self.lock();
    if (!locked)
    {
        mir::log_error("evdev-rs: suspended() ignored — platform is not allocated");
        return;
    }

    auto& platform_impl = locked->platform_impl;

    if (!platform_impl->is_running())
    {
        mir::log_info("evdev-rs: suspended() ignored — platform not running");
        return;
    }

    device_queue->enqueue(
        [devnode = devnode, devnum = devnum, &platform_impl = platform_impl]()
        {
            if (!platform_impl->is_running())
                return;
            platform_impl->on_device_suspended(devnode, devnum);
        });
}

void InputDeviceObserver::removed()
{
    mir::log_info("evdev-rs: observer removed() for %s (devnum=%lu)",
                   devnode.c_str(), static_cast<unsigned long>(devnum));

    auto const locked = platform_self.lock();
    if (!locked)
    {
        mir::log_error("evdev-rs: removed() ignored — platform is not allocated");
        return;
    }

    auto& platform_impl = locked->platform_impl;

    if (!platform_impl->is_running())
    {
        mir::log_info("evdev-rs: removed() ignored — platform not running");
        return;
    }

    device_queue->enqueue(
        [devnode = devnode, devnum = devnum, &platform_impl = platform_impl]()
        {
            if (!platform_impl->is_running())
                return;
            platform_impl->on_device_removed(devnode, devnum);
        });
}

miers::Platform::Platform(
    std::shared_ptr<ConsoleServices> const& console,
    std::shared_ptr<InputDeviceRegistry> const& input_device_registry,
    std::shared_ptr<mi::InputReport> const& report)
    : self(std::make_shared<Self>(this, console, input_device_registry, report))
{
}

std::shared_ptr<mir::dispatch::Dispatchable> miers::Platform::dispatchable()
{
    return self->dispatchable;
}

void miers::Platform::start()
{
    // Clean up any stale watches from a previous start()/stop() cycle.
    if (self->device_queue)
    {
        self->dispatchable->remove_watch(self->device_queue);
        self->device_queue.reset();
    }
    if (self->udev_dispatch)
    {
        self->dispatchable->remove_watch(self->udev_dispatch);
        self->udev_dispatch.reset();
    }
    if (self->libinput_dispatch)
    {
        self->dispatchable->remove_watch(self->libinput_dispatch);
        self->libinput_dispatch.reset();
    }

    // Create the device_queue before start() because Rust's start()
    // enumerates existing udev devices, which triggers acquire_device()
    // and may fire activated() synchronously — needing the queue.
    self->device_queue = std::make_shared<md::ActionQueue>();

    if (!self->platform_impl->start())
    {
        self->device_queue.reset();
        return;
    }

    auto const libinput_fd = self->platform_impl->libinput_fd();
    if (libinput_fd < 0)
    {
        mir::log_error("evdev-rs platform: failed to obtain libinput fd after start(); resetting platform state");
        self->platform_impl->stop();
        self->device_queue.reset();
        return;
    }

    self->libinput_dispatch = std::make_shared<md::ReadableFd>(
        mir::Fd{mir::IntOwnedFd{libinput_fd}},
        [this]() { self->platform_impl->process(); });

    auto const udev_fd = self->platform_impl->udev_monitor_fd();
    if (udev_fd < 0)
    {
        mir::log_error("evdev-rs platform: failed to obtain udev monitor fd; resetting platform state");
        self->platform_impl->stop();
        self->device_queue.reset();
        return;
    }

    self->udev_dispatch = std::make_shared<md::ReadableFd>(
        mir::Fd{mir::IntOwnedFd{udev_fd}},
        [this]() { self->platform_impl->process_udev_events(); });

    self->dispatchable->add_watch(self->libinput_dispatch);
    self->dispatchable->add_watch(self->device_queue);
    self->dispatchable->add_watch(self->udev_dispatch);
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
    // Remove watches before stopping Rust, because stop() closes the libinput
    // and udev fds. Calling remove_watch() after the fds are closed causes
    // epoll_ctl(EPOLL_CTL_DEL) to return EBADF and throw.
    if (self->udev_dispatch)
    {
        self->dispatchable->remove_watch(self->udev_dispatch);
        self->udev_dispatch.reset();
    }
    if (self->device_queue)
    {
        self->dispatchable->remove_watch(self->device_queue);
        self->device_queue.reset();
    }
    if (self->libinput_dispatch)
    {
        self->dispatchable->remove_watch(self->libinput_dispatch);
        self->libinput_dispatch.reset();
    }

    self->platform_impl->stop();
}

std::unique_ptr<mir::Device::Observer> miers::Platform::create_device_observer(
    std::string const& devnode, uint64_t devnum)
{
    return std::make_unique<InputDeviceObserver>(
        devnode, devnum, self->device_queue, self);
}

std::shared_ptr<mi::InputDevice> miers::Platform::create_input_device(int device_id) const
{
    return std::make_shared<::InputDevice<LibinputDevice>>(self->platform_impl->create_input_device(device_id));
}
