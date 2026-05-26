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
#include <mir/udev/wrapper.h>
#include <mir/log.h>

#include <unistd.h>

namespace mi = mir::input;
namespace miers = mi::evdev_rs;
namespace md = mir::dispatch;
namespace mu = mir::udev;

namespace
{

/// Dispatchable that monitors udev for input device events and forwards
/// them to the Rust platform logic for decision-making.
class DispatchableUDevMonitor : public md::Dispatchable
{
public:
    DispatchableUDevMonitor(
        mu::Context& context,
        std::function<void(mu::Monitor::EventType, mu::Device const&)> on_event)
        : monitor(context),
          on_event{std::move(on_event)}
    {
        monitor.filter_by_subsystem("input");
        monitor.enable();

        auto fake_shared_context = std::shared_ptr<mu::Context>{&context, [](auto){}};
        mu::Enumerator device_enumerator{fake_shared_context};

        device_enumerator.match_subsystem("input");
        device_enumerator.scan_devices();

        for (auto const& device : device_enumerator)
        {
            if (device.initialised())
            {
                this->on_event(mu::Monitor::EventType::ADDED, device);
            }
        }
    }

    mir::Fd watch_fd() const override
    {
        return mir::Fd{mir::IntOwnedFd{monitor.fd()}};
    }

    bool dispatch(md::FdEvents events) override
    {
        if (events & md::FdEvent::error)
            return false;
        monitor.process_events(on_event);
        return true;
    }

    md::FdEvents relevant_events() const override
    {
        return md::FdEvent::readable;
    }

private:
    mu::Monitor monitor;
    std::function<void(mu::Monitor::EventType, mu::Device const&)> const on_event;
};

/// Thin observer that forwards device lifecycle events to the Rust platform
/// via an ActionQueue (ensuring they run on the input dispatch thread).
class InputDeviceObserver : public mir::Device::Observer
{
public:
    InputDeviceObserver(
        std::string devnode,
        uint64_t devnum,
        std::shared_ptr<md::ActionQueue> device_queue,
        rust::Box<miers::PlatformRs>& platform_impl)
        : devnode{std::move(devnode)},
          devnum{devnum},
          device_queue{std::move(device_queue)},
          platform_impl{platform_impl}
    {
    }

    void activated(mir::Fd&& device_fd) override
    {
        if (!platform_impl->is_running())
            return;

        // dup() so that mir::Fd's destructor closes the original without
        // invalidating the fd we hand to libinput via open_restricted().
        int const duped = ::dup(static_cast<int>(device_fd));
        if (duped < 0)
        {
            mir::log_error("evdev-rs: dup() failed in activated() for %s", devnode.c_str());
            return;
        }

        device_queue->enqueue(
            [devnode = devnode, devnum = devnum, fd = duped, &platform_impl = platform_impl]()
            {
                if (!platform_impl->is_running())
                {
                    ::close(fd);
                    return;
                }
                platform_impl->on_device_activated(devnode, devnum, fd);
            });
    }

    void suspended() override
    {
        if (!platform_impl->is_running())
            return;

        device_queue->enqueue(
            [devnode = devnode, devnum = devnum, &platform_impl = platform_impl]()
            {
                if (!platform_impl->is_running())
                    return;
                platform_impl->on_device_suspended(devnode, devnum);
            });
    }

    void removed() override
    {
        if (!platform_impl->is_running())
            return;

        device_queue->enqueue(
            [devnode = devnode, devnum = devnum, &platform_impl = platform_impl]()
            {
                if (!platform_impl->is_running())
                    return;
                platform_impl->on_device_removed(devnode, devnum);
            });
    }

private:
    std::string devnode;
    uint64_t devnum;
    std::shared_ptr<md::ActionQueue> device_queue;
    rust::Box<miers::PlatformRs>& platform_impl;
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
    rust::Box<PlatformRs> platform_impl;
    std::shared_ptr<md::ReadableFd> libinput_dispatch;
    std::shared_ptr<md::MultiplexingDispatchable> dispatchable;
    std::shared_ptr<md::ActionQueue> device_queue;
    std::shared_ptr<DispatchableUDevMonitor> udev_dispatchable;
    std::unique_ptr<mu::Context> udev_context;
};

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
    if (!self->platform_impl->start())
        return;

    // Clean up any stale watches from a previous start()/stop() cycle.
    if (self->device_queue)
    {
        self->dispatchable->remove_watch(self->device_queue);
        self->device_queue.reset();
    }
    if (self->udev_dispatchable)
    {
        self->dispatchable->remove_watch(self->udev_dispatchable);
        self->udev_dispatchable.reset();
    }
    if (self->libinput_dispatch)
    {
        self->dispatchable->remove_watch(self->libinput_dispatch);
        self->libinput_dispatch.reset();
    }

    auto const libinput_fd = self->platform_impl->libinput_fd();
    if (libinput_fd < 0)
    {
        mir::log_error("evdev-rs platform: failed to obtain libinput fd after start(); resetting platform state");
        self->platform_impl->stop();
        return;
    }

    self->libinput_dispatch = std::make_shared<md::ReadableFd>(
        mir::Fd{mir::IntOwnedFd{libinput_fd}},
        [this]() { self->platform_impl->process(); });

    self->device_queue = std::make_shared<md::ActionQueue>();
    self->udev_context = std::make_unique<mu::Context>();

    self->udev_dispatchable = std::make_shared<DispatchableUDevMonitor>(
        *self->udev_context,
        [this](mu::Monitor::EventType type, mu::Device const& device)
        {
            try
            {
                // Re-fetch via syspath to work around umockdev missing properties.
                auto ctx = std::make_shared<mu::Context>();
                auto workaround_device = ctx->device_from_syspath(device.syspath());

                char const* devnode = workaround_device->devnode();
                if (!devnode)
                    return;

                char const* sysname = workaround_device->sysname();
                if (!sysname)
                    return;

                auto const devnum = static_cast<uint64_t>(workaround_device->devnum());

                miers::UdevEventType event_type;
                switch (type)
                {
                case mu::Monitor::ADDED:
                    event_type = miers::UdevEventType::Added;
                    break;
                case mu::Monitor::REMOVED:
                    event_type = miers::UdevEventType::Removed;
                    break;
                default:
                    return;
                }

                self->platform_impl->on_udev_event(
                    event_type,
                    rust::Str(devnode, strlen(devnode)),
                    devnum,
                    rust::Str(sysname, strlen(sysname)));
            }
            catch (std::exception const&)
            {
                mir::log(mir::logging::Severity::warning, MIR_LOG_COMPONENT,
                    std::current_exception(),
                    std::string{"Failed to handle UDev event for "} + device.syspath());
            }
        });

    self->dispatchable->add_watch(self->libinput_dispatch);
    self->dispatchable->add_watch(self->device_queue);
    self->dispatchable->add_watch(self->udev_dispatchable);
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

    // Remove watches so no further lambdas are dispatched.
    if (self->udev_dispatchable)
    {
        self->dispatchable->remove_watch(self->udev_dispatchable);
        self->udev_dispatchable.reset();
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

    self->udev_context.reset();
}

std::unique_ptr<mir::Device::Observer> miers::Platform::create_device_observer(
    std::string const& devnode, uint64_t devnum)
{
    return std::make_unique<InputDeviceObserver>(
        devnode, devnum, self->device_queue, self->platform_impl);
}

std::shared_ptr<mi::InputDevice> miers::Platform::create_input_device(int device_id) const
{
    return std::make_shared<::InputDevice<LibinputDevice>>(self->platform_impl->create_input_device(device_id));
}
