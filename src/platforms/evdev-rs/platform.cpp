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
        fd = std::move(device_fd);
        box->activated(fd.value());
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
}

class miers::Platform::Self
{
public:
    Self(Platform* platform, std::shared_ptr<ConsoleServices> const& console,
        std::shared_ptr<InputDeviceRegistry> const& input_device_registry,
        std::shared_ptr<mi::InputReport> const& report)
        : platform_impl(evdev_rs_create(
            std::make_shared<PlatformBridge>(platform, console),
            input_device_registry,
            std::make_unique<miers::InputReport>(report))),
          dispatchable{std::make_shared<md::MultiplexingDispatchable>()} {}

    rust::Box<PlatformRs> platform_impl;
    std::shared_ptr<md::ReadableFd> libinput_dispatch;
    std::shared_ptr<md::MultiplexingDispatchable> dispatchable;
    /// One-shot queue used to defer udev_assign_seat() until after
    /// start_platforms() returns and the GLib main loop is free.
    std::shared_ptr<md::ActionQueue> assign_seat_queue;
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
    // Phase 1 (non-blocking): create the libinput context without assigning a
    // seat.  This returns quickly so that start_platforms() can return and the
    // GLib main loop thread is no longer blocked.
    // Returns false if already started (e.g. stop() was not called without an
    // error); in that case skip the rest of start() to avoid tearing down the
    // active watches or re-assigning the udev seat on an already-assigned
    // libinput context.
    if (!self->platform_impl->start())
        return;

    // Remove any stale fd watcher or assign_seat queue left over from a
    // previous start()/stop() cycle where stop() may not have cleaned up
    // (e.g. due to an earlier deadlock).  We only do this when start() actually
    // created a fresh context (returned true) — when it returns false the
    // platform is still running and removing its watchers would break dispatch.
    if (self->assign_seat_queue)
    {
        self->dispatchable->remove_watch(self->assign_seat_queue);
        self->assign_seat_queue.reset();
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
        Fd{IntOwnedFd{libinput_fd}},
        [this]() { self->platform_impl->process(); });
    self->dispatchable->add_watch(self->libinput_dispatch);

    // Phase 2 (deferred): assign the udev seat so that libinput opens input
    // devices.  Opening each device requires a logind TakeDevice D-Bus call
    // whose reply is processed by the GLib main loop.  We defer this work via
    // an ActionQueue so it executes on the input thread *after*
    // start_platforms() has returned — at which point the GLib main loop is
    // no longer blocked and can process those replies.
    mir::log_info("evdev-rs: Platform::start(): enqueueing assign_seat on input thread (libinput fd=%d)", libinput_fd);
    self->assign_seat_queue = std::make_shared<md::ActionQueue>();
    self->assign_seat_queue->enqueue(
        [this]()
        {
            mir::log_info("evdev-rs: assign_seat_queue action firing on input thread");
            self->platform_impl->assign_seat();
            mir::log_info("evdev-rs: assign_seat_queue action complete");
        });
    self->dispatchable->add_watch(self->assign_seat_queue);
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
    if (self->assign_seat_queue)
    {
        self->dispatchable->remove_watch(self->assign_seat_queue);
        self->assign_seat_queue.reset();
    }
    if (self->libinput_dispatch)
        self->dispatchable->remove_watch(self->libinput_dispatch);
    self->libinput_dispatch.reset();
    self->platform_impl->stop();
}

std::unique_ptr<miers::DeviceObserverWithFd> miers::Platform::create_device_observer()
{
    return std::make_unique<DeviceObserverWrapper<LibinputDeviceObserver>>(
        self->platform_impl->create_device_observer());
}

std::shared_ptr<mi::InputDevice> miers::Platform::create_input_device(int device_id) const
{
    return std::make_shared<::InputDevice<LibinputDevice>>(self->platform_impl->create_input_device(device_id));
}
