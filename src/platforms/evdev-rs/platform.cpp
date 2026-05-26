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
#include <mir/constexpr_utils.h>
#include <mir/log.h>

#include <atomic>
#include <future>
#include <unordered_map>
#include <unistd.h>

namespace mi = mir::input;
namespace miers = mi::evdev_rs;
namespace md = mir::dispatch;
namespace mu = mir::udev;

namespace
{

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

/// Observer for a single input device being acquired via ConsoleServices.
///
/// When activated() fires (on the GLib main loop thread):
///   1. dup() the fd and store it in the bridge's pending-fd map.
///   2. Enqueue path_add_device() on the input dispatch thread via device_queue.
///
/// On suspended() / removed(): enqueue path_remove_device() similarly.
///
/// References to pending_devices, device_watchers, and platform_impl are safe
/// because the device_queue is removed from the dispatchable before Platform::stop()
/// returns, so enqueued lambdas never run after the platform is torn down.
class InputDeviceObserver : public mir::Device::Observer
{
public:
    InputDeviceObserver(
        std::string devnode,
        dev_t devnum,
        std::shared_ptr<miers::PlatformBridge> bridge,
        std::shared_ptr<md::ActionQueue> device_queue,
        std::atomic<bool> const& running,
        std::unordered_map<dev_t, std::future<std::unique_ptr<mir::Device>>>& pending_devices,
        std::unordered_map<dev_t, std::unique_ptr<mir::Device>>& device_watchers,
        rust::Box<miers::PlatformRs>& platform_impl)
        : devnode{std::move(devnode)},
          devnum{devnum},
          bridge{std::move(bridge)},
          device_queue{std::move(device_queue)},
          running{running},
          pending_devices{pending_devices},
          device_watchers{device_watchers},
          platform_impl{platform_impl}
    {
    }

    void activated(mir::Fd&& device_fd) override
    {
        if (!running.load())
            return;

        // dup() so that mir::Fd's destructor closes the original without
        // invalidating the fd we hand to libinput via open_restricted().
        int const duped = ::dup(static_cast<int>(device_fd));
        if (duped < 0)
        {
            mir::log_error("evdev-rs: dup() failed in activated() for %s", devnode.c_str());
            return;
        }
        // device_fd goes out of scope here and closes the original fd.

        bridge->store_pending_fd(devnode, duped);

        device_queue->enqueue(
            [this,
             devnum = devnum,
             devnode = devnode,
             &running = running,
             &pending_devices = pending_devices,
             &device_watchers = device_watchers,
             &platform_impl = platform_impl]()
            {
                if (!running.load())
                    return;

                // Transfer the Device from pending to active watchers.
                // .get() is non-blocking because activated() already fired,
                // meaning the future is resolved.
                auto pending_it = pending_devices.find(devnum);
                if (pending_it != pending_devices.end())
                {
                    device_watchers.emplace(devnum, pending_it->second.get());
                    pending_devices.erase(pending_it);
                }

                platform_impl->path_add_device(devnode);
            });
    }

    void suspended() override
    {
        if (!running.load())
            return;

        device_queue->enqueue(
            [this,
             devnum = devnum,
             devnode = devnode,
             &running = running,
             &device_watchers = device_watchers,
             &platform_impl = platform_impl]()
            {
                if (!running.load())
                    return;
                device_watchers.erase(devnum);
                platform_impl->path_remove_device(devnode);
            });
    }

    void removed() override
    {
        if (!running.load())
            return;

        device_queue->enqueue(
            [this,
             devnum = devnum,
             devnode = devnode,
             &running = running,
             &device_watchers = device_watchers,
             &platform_impl = platform_impl]()
            {
                if (!running.load())
                    return;
                device_watchers.erase(devnum);
                platform_impl->path_remove_device(devnode);
            });
    }

private:
    std::string devnode;
    dev_t devnum;
    std::shared_ptr<miers::PlatformBridge> bridge;
    std::shared_ptr<md::ActionQueue> device_queue;
    std::atomic<bool> const& running;
    std::unordered_map<dev_t, std::future<std::unique_ptr<mir::Device>>>& pending_devices;
    std::unordered_map<dev_t, std::unique_ptr<mir::Device>>& device_watchers;
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
          console{console},
          platform_impl{evdev_rs_create(
              bridge,
              input_device_registry,
              std::make_unique<miers::InputReport>(report))},
          dispatchable{std::make_shared<md::MultiplexingDispatchable>()}
    {
    }

    std::shared_ptr<PlatformBridge> bridge;
    std::shared_ptr<ConsoleServices> console;
    rust::Box<PlatformRs> platform_impl;
    std::shared_ptr<md::ReadableFd> libinput_dispatch;
    std::shared_ptr<md::MultiplexingDispatchable> dispatchable;
    std::shared_ptr<md::ActionQueue> device_queue;
    std::shared_ptr<DispatchableUDevMonitor> udev_dispatchable;
    std::unique_ptr<mu::Context> udev_context;

    std::unordered_map<dev_t, std::future<std::unique_ptr<mir::Device>>> pending_devices;
    std::unordered_map<dev_t, std::unique_ptr<mir::Device>> device_watchers;
    std::atomic<bool> running{false};
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
    self->running.store(true);
    self->udev_context = std::make_unique<mu::Context>();

    self->udev_dispatchable = std::make_shared<DispatchableUDevMonitor>(
        *self->udev_context,
        [this](mu::Monitor::EventType type, mu::Device const& device)
        {
            using namespace std::string_literals;
            try
            {
                switch (type)
                {
                case mu::Monitor::ADDED:
                {
                    if (!device.devnode())
                        break;

                    // Re-fetch via syspath to work around umockdev missing properties.
                    auto ctx = std::make_shared<mu::Context>();
                    auto workaround_device = ctx->device_from_syspath(device.syspath());

                    if (!workaround_device->devnode())
                        break;

                    // Libinput only processes "event" devices.
                    if (strncmp(workaround_device->sysname(), "event", strlen_c("event")) != 0)
                        break;

                    auto const devnum = workaround_device->devnum();
                    if (self->pending_devices.count(devnum) > 0 ||
                        self->device_watchers.count(devnum) > 0)
                        break; // already handled

                    std::string devnode{workaround_device->devnode()};

                    self->pending_devices.emplace(
                        devnum,
                        self->console->acquire_device(
                            major(devnum),
                            minor(devnum),
                            std::make_unique<InputDeviceObserver>(
                                devnode,
                                devnum,
                                self->bridge,
                                self->device_queue,
                                self->running,
                                self->pending_devices,
                                self->device_watchers,
                                self->platform_impl)));
                    break;
                }
                case mu::Monitor::REMOVED:
                {
                    auto const devnum = device.devnum();

                    // If still pending (activated() never fired), just drop the future.
                    self->pending_devices.erase(devnum);

                    // If already active, remove from libinput.
                    if (self->device_watchers.count(devnum) > 0)
                    {
                        std::string devnode{device.devnode() ? device.devnode() : ""};
                        self->device_watchers.erase(devnum);
                        if (!devnode.empty())
                            self->platform_impl->path_remove_device(devnode);
                    }
                    break;
                }
                default:
                    break;
                }
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
    self->running.store(false);

    // Remove watches first so no further lambdas are dispatched from device_queue.
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

    // Release Device handles.  The mir::Device destructor guarantees no further
    // Observer callbacks will fire once destruction completes.
    self->pending_devices.clear();
    self->device_watchers.clear();
    self->udev_context.reset();

    self->platform_impl->stop();
}

std::shared_ptr<mi::InputDevice> miers::Platform::create_input_device(int device_id) const
{
    return std::make_shared<::InputDevice<LibinputDevice>>(self->platform_impl->create_input_device(device_id));
}
