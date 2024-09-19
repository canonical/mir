/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "default_input_device_hub.h"
#include "default_device.h"

#include "mir/input/input_device.h"
#include "mir/input/input_device_observer.h"
#include "mir/input/mir_pointer_config.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/input/mir_touchscreen_config.h"
#include "mir/input/mir_keyboard_config.h"
#include "mir/geometry/point.h"
#include "mir/server_status_listener.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/action_queue.h"
#include "mir/server_action_queue.h"
#define MIR_LOG_COMPONENT "input-hub"
#include "mir/log.h"

#include "boost/throw_exception.hpp"
#include "mir/input/led_observer_registrar.h"
#include "mir/input/key_mapper.h"

#include <algorithm>
#include <atomic>
#include <memory>

namespace mi = mir::input;

struct mi::ExternalInputDeviceHub::Internal : InputDeviceObserver
{
    Internal(std::shared_ptr<ServerActionQueue> const& queue) :
        observer_queue{queue}
    {}
    void device_added(std::shared_ptr<Device> const& device) override;
    void device_changed(std::shared_ptr<Device> const& device) override;
    void device_removed(std::shared_ptr<Device> const& device) override;
    void changes_complete() override;

    std::shared_ptr<ServerActionQueue> const observer_queue;
    ThreadSafeList<std::shared_ptr<InputDeviceObserver>> observers;

    // Needs to be a recursive mutex so that initial device notifications can be sent under lock in add_observer()
    std::recursive_mutex mutex;
    std::vector<std::shared_ptr<Device>> devices_added;
    std::vector<std::shared_ptr<Device>> devices_changed;
    std::vector<std::shared_ptr<Device>> devices_removed;
    std::vector<std::shared_ptr<Device>> handles;
};

mi::ExternalInputDeviceHub::ExternalInputDeviceHub(std::shared_ptr<mi::InputDeviceHub> const& hub, std::shared_ptr<mir::ServerActionQueue> const& queue)
    : data{std::make_shared<mi::ExternalInputDeviceHub::Internal>(queue)}, hub{hub}
{
    hub->add_observer(data);
}

mi::ExternalInputDeviceHub::~ExternalInputDeviceHub()
{
    data->observer_queue->pause_processing_for(data.get());
}

void mi::ExternalInputDeviceHub::add_observer(std::shared_ptr<InputDeviceObserver> const& observer)
{
    data->observer_queue->enqueue(
        data.get(),
        [observer, data = this->data]
        {
            std::lock_guard lock{data->mutex};
            for (auto const& item : data->handles)
                observer->device_added(item);
            observer->changes_complete();
            // Need to add observer under lock so that new handles can not be added between sending off initial
            // observations and when the observer is added
            data->observers.add(observer);
        });
}

void mi::ExternalInputDeviceHub::remove_observer(std::weak_ptr<InputDeviceObserver> const& obs)
{
    auto observer = obs.lock();
    if (observer)
    {
        std::mutex mutex;
        std::condition_variable cv;
        bool removed{false};

        data->observer_queue->enqueue_with_guaranteed_execution(
            [&,this]
            {
                {
                    std::lock_guard lock{data->mutex};
                    data->observers.remove(observer);

                    std::lock_guard cond_var_lock{mutex};
                    removed = true;
                }
                cv.notify_one();
            });

        std::unique_lock lock{mutex};

        // Before returning wait for the remove - otherwise notifications can still happen
        cv.wait(lock, [&] { return removed; });
    }
}

void mi::ExternalInputDeviceHub::for_each_input_device(std::function<void(Device const& device)> const& callback)
{
    hub->for_each_input_device(callback);
}

void mi::ExternalInputDeviceHub::for_each_mutable_input_device(std::function<void(Device& device)> const& callback)
{
    hub->for_each_mutable_input_device(callback);
}

void mi::ExternalInputDeviceHub::Internal::device_added(std::shared_ptr<Device> const& device)
{
    std::lock_guard lock{mutex};
    devices_added.push_back(device);
}

void mi::ExternalInputDeviceHub::Internal::device_changed(std::shared_ptr<Device> const& device)
{
    std::lock_guard lock{mutex};
    devices_changed.push_back(device);
}

void mi::ExternalInputDeviceHub::Internal::device_removed(std::shared_ptr<Device> const& device)
{
    std::lock_guard lock{mutex};
    devices_removed.push_back(device);
}

template <>
struct std::formatter<std::shared_ptr<mi::DefaultDevice>>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(std::shared_ptr<mi::DefaultDevice> const& handle, std::format_context& ctx) const;
};

void mi::ExternalInputDeviceHub::Internal::changes_complete()
{
    std::lock_guard lock{mutex};

    decltype(devices_added) added, changed, removed;
    std::swap(devices_added, added);
    std::swap(devices_changed, changed);
    std::swap(devices_removed, removed);

    if (!(added.empty() && changed.empty() && removed.empty()))
        observer_queue->enqueue(
            this,
            [this, added, changed, removed]
            {
                observers.for_each([&](std::shared_ptr<InputDeviceObserver> const& observer)
                    {
                        for (auto const& dev : added)
                            observer->device_added(dev);
                        for (auto const& dev : changed)
                            observer->device_changed(dev);
                        for (auto const& dev : removed)
                            observer->device_removed(dev);
                        observer->changes_complete();
                    });

                for (auto const& dev : removed)
                {
                    handles.erase(remove(begin(handles), end(handles), dev), end(handles));
                }

                for (auto const& dev : added)
                {
                    if (auto handle = std::dynamic_pointer_cast<DefaultDevice>(dev))
                        log_info(std::format("Device configuration: {}", handle));
                    handles.push_back(dev);
                }
            });
}

mi::DefaultInputDeviceHub::DefaultInputDeviceHub(
    std::shared_ptr<mi::Seat> const& seat,
    std::shared_ptr<dispatch::MultiplexingDispatchable> const& input_multiplexer,
    std::shared_ptr<time::Clock> const& clock,
    std::shared_ptr<mi::KeyMapper> const& key_mapper,
    std::shared_ptr<mir::ServerStatusListener> const& server_status_listener,
    std::shared_ptr<LedObserverRegistrar> led_observer_registrar)
    : seat{seat},
      input_dispatchable{input_multiplexer},
      device_queue(std::make_shared<dispatch::ActionQueue>()),
      clock(clock),
      key_mapper(key_mapper),
      server_status_listener(server_status_listener),
      led_observer_registrar{std::move(led_observer_registrar)},
      device_id_generator{0}
{
    input_dispatchable->add_watch(device_queue);
}

template <>
struct std::formatter<mi::DeviceCapabilities>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    constexpr auto format(mi::DeviceCapabilities const& c, std::format_context& ctx) const
    {
        using mi::DeviceCapability;

        auto out = std::format_to(ctx.out(), "{{");

        auto write_delim = [&out, caps_written = false] () mutable
        {
            if (caps_written) out = std::format_to(out, "|");
            caps_written = true;
        };

        if (contains(c, DeviceCapability::pointer))
        {
            write_delim();
            out = std::format_to(out, "pointer");
        }

        if (contains(c, DeviceCapability::keyboard))
        {
            write_delim();
            out = std::format_to(out, "keyboard");
        }

        if (contains(c, DeviceCapability::touchpad))
        {
            write_delim();
            out = std::format_to(out, "touchpad");
        }

        if (contains(c, DeviceCapability::touchscreen))
        {
            write_delim();
            out = std::format_to(out, "touchscreen");
        }

        if (contains(c, DeviceCapability::gamepad))
        {
            write_delim();
            out = std::format_to(out, "gamepad");
        }

        if (contains(c, DeviceCapability::joystick))
        {
            write_delim();
            out = std::format_to(out, "joystick");
        }

        if (contains(c, DeviceCapability::switch_))
        {
            write_delim();
            out = std::format_to(out, "switch");
        }

        if (contains(c, DeviceCapability::multitouch))
        {
            write_delim();
            out = std::format_to(out, "multitouch");
        }

        if (contains(c, DeviceCapability::alpha_numeric))
        {
            write_delim();
            out = std::format_to(out, "alpha_numeric");
        }

        return std::format_to(out, "}}");
    }
};

template <>
struct std::formatter<MirPointerConfig>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    constexpr auto format(MirPointerConfig const& c, std::format_context& ctx) const
    {
        auto out = std::format_to(ctx.out(), "{{");

        switch (c.handedness())
        {
        case mir_pointer_handedness_right:
            out = std::format_to(out, "handedness=right,");
            break;

        case mir_pointer_handedness_left:
            out = std::format_to(out, "handedness=left,");
            break;
        }

        switch (c.acceleration())
        {
        case mir_pointer_acceleration_adaptive:
            out = std::format_to(out, "cursor-acceleration=adaptive,");
            break;

        case mir_pointer_acceleration_none:
            out = std::format_to(out, "cursor-acceleration=none,");
            break;
        }

        out = std::format_to(out, "cursor-acceleration-bias={:.2f},", c.cursor_acceleration_bias());
        out = std::format_to(out, "scroll-hspeed={:.2f},", c.horizontal_scroll_scale());
        return std::format_to(out, "scroll-vspeed={:.2f}}}", c.vertical_scroll_scale());
    }
};

template <>
struct std::formatter<MirTouchpadConfig>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    constexpr auto format(MirTouchpadConfig const& c, std::format_context& ctx) const
    {
        auto out = std::format_to(ctx.out(), "{{");

        auto write_delim = [&out, caps_written = false] () mutable
        {
            if (caps_written) out = std::format_to(out, "|");
            caps_written = true;
        };

        switch (c.click_mode())
        {
        case mir_touchpad_click_mode_none:
            write_delim();
            out = std::format_to(out, "click-mode=none");
            break;
        case mir_touchpad_click_mode_area_to_click:
            write_delim();
            out = std::format_to(out, "click-mode=area");
            break;
        case mir_touchpad_click_mode_finger_count:
            write_delim();
            out = std::format_to(out, "click-mode=clickfinger");
            break;
        }

        switch (c.scroll_mode())
        {
        case mir_touchpad_scroll_mode_none:
            write_delim();
            out = std::format_to(out, "scroll-mode=none");
            break;

        case mir_touchpad_scroll_mode_two_finger_scroll:
            write_delim();
            out = std::format_to(out, "scroll-mode=two-finger");
            break;

        case mir_touchpad_scroll_mode_edge_scroll:
            write_delim();
            out = std::format_to(out, "scroll-mode=edge");
            break;

        case mir_touchpad_scroll_mode_button_down_scroll:
            write_delim();
            out = std::format_to(out, "scroll-mode=button-down,touchpad-scroll-button={},", c.button_down_scroll_button());
            break;
        }

        if (c.tap_to_click())
        {
            write_delim();
            out = std::format_to(out, "tap-to-click");
        }

        if (c.middle_mouse_button_emulation())
        {
            write_delim();
            out = std::format_to(out, "middle-mouse-button-emulation");
        }


        if (c.disable_with_external_mouse())
        {
            write_delim();
            out = std::format_to(out, "disable-with-external-mouse");
        }

        if (c.disable_while_typing())
        {
            write_delim();
            out = std::format_to(out, "disable-while-typing");
        }

        return std::format_to(out, "}}");
    }
};

template <>
struct std::formatter<MirTouchscreenConfig>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    constexpr auto format(MirTouchscreenConfig const& c, std::format_context& ctx) const
    {
        switch (c.mapping_mode())
        {
        case mir_touchscreen_mapping_mode_to_output:
            return std::format_to(ctx.out(), "{{mapping-mode=to-output}}");

        case mir_touchscreen_mapping_mode_to_display_wall:
            return std::format_to(ctx.out(), "{{mapping-mode=to-display-wall}}");

        default:
            return ctx.out();
        }
    }
};

auto std::formatter<std::shared_ptr<mi::DefaultDevice>>::format(std::shared_ptr<mi::DefaultDevice> const& handle, std::format_context& ctx) const
{
    auto out = std::format_to(ctx.out(), "{}, capabilities={}", handle->name(), handle->capabilities());

    if (auto const pointer_config = handle->pointer_configuration())
    {
        out = std::format_to(out, ", pointer_config={}", pointer_config.value());
    }

    if (auto const touchpad_config = handle->touchpad_configuration())
    {
        out = std::format_to(out, ", touchpad_config={}", touchpad_config.value());
    }

    if (auto const touchscreen_config = handle->touchscreen_configuration())
    {
        out = std::format_to(out, ", touchscreen_config={}", touchscreen_config.value());
    }
    return out;
}

auto mi::DefaultInputDeviceHub::add_device(std::shared_ptr<InputDevice> const& device) -> std::weak_ptr<Device>
{
    if (!device)
        BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid input device"));

    std::lock_guard lock{mutex};

    auto it = find_if(devices.cbegin(),
                      devices.cend(),
                      [&device](auto const& item)
                      {
                          return item->device_matches(device);
                      });

    if (it == end(devices))
    {
        auto queue = std::make_shared<dispatch::ActionQueue>();
        auto const handle = restore_or_create_device(lock, *device, queue);
        // send input device info to observer loop..
        devices.push_back(std::make_unique<RegisteredDevice>(
            device,
            handle->id(),
            queue,
            clock,
            handle));

        auto const& dev = devices.back();
        add_device_handle(lock, handle);

        seat->add_device(*handle);
        dev->start(seat, input_dispatchable);

        if (auto const observer = std::dynamic_pointer_cast<LedObserver>(device))
        {
            led_observer_registrar->register_interest(observer, handle->id());
        }

        return handle;
    }
    else
    {
        log_error("Input device %s added twice", device->get_device_info().name.c_str());
        BOOST_THROW_EXCEPTION(std::logic_error("Input device already managed by server"));
    }
}

void mi::DefaultInputDeviceHub::remove_device(std::shared_ptr<InputDevice> const& device)
{
    if (!device)
        BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid input device"));

    std::lock_guard lock{mutex};

    auto pos = remove_if(
        begin(devices),
        end(devices),
        [&](auto const& item)
        {
            if (item->device_matches(device))
            {
                if (auto const observer = std::dynamic_pointer_cast<LedObserver>(device))
                {
                    led_observer_registrar->unregister_interest(*observer, item->id());
                }

                store_device_config(lock, *item->handle);
                auto seat = item->seat;
                if (seat)
                {
                    seat->remove_device(*item->handle);
                    item->stop(input_dispatchable);
                }
                remove_device_handle(lock, item->id());

                return true;
            }
            return false;
        });
    if (pos == end(devices))
    {
        log_error("Input device %s not found", device->get_device_info().name.c_str());
        BOOST_THROW_EXCEPTION(std::logic_error("Input device not managed by server"));
    }

    devices.erase(pos, end(devices));
}

mi::DefaultInputDeviceHub::RegisteredDevice::RegisteredDevice(
    std::shared_ptr<InputDevice> const& dev,
    MirInputDeviceId device_id,
    std::shared_ptr<dispatch::ActionQueue> const& queue,
    std::shared_ptr<time::Clock> const& clock,
    std::shared_ptr<mi::DefaultDevice> const& handle)
    : handle(handle),
      device_id(device_id),
      clock(clock),
      device(dev),
      queue(queue)
{
}

MirInputDeviceId mi::DefaultInputDeviceHub::create_new_device_id(std::lock_guard<std::recursive_mutex> const&)
{
    return ++device_id_generator;
}

MirInputDeviceId mi::DefaultInputDeviceHub::RegisteredDevice::id()
{
    return device_id;
}

void mi::DefaultInputDeviceHub::RegisteredDevice::handle_input(std::shared_ptr<MirEvent> const& event)
{
    auto type = mir_event_get_type(event.get());

    if (type != mir_event_type_input &&
        type != mir_event_type_input_device_state)
        BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid input event received from device"));

    if (!seat)
        return;

    seat->dispatch_event(event);
}

bool mi::DefaultInputDeviceHub::RegisteredDevice::device_matches(std::shared_ptr<InputDevice> const& dev) const
{
    return dev == device;
}

void mi::DefaultInputDeviceHub::RegisteredDevice::start(std::shared_ptr<Seat> const& seat, std::shared_ptr<dispatch::MultiplexingDispatchable> const& multiplexer)
{
    multiplexer->add_watch(queue);

    this->seat = seat;
    builder = std::make_unique<DefaultEventBuilder>(device_id, clock);
    device->start(this, builder.get());
}

void mi::DefaultInputDeviceHub::RegisteredDevice::stop(std::shared_ptr<dispatch::MultiplexingDispatchable> const& multiplexer)
{
    multiplexer->remove_watch(queue);
    handle->disable_queue();

    device->stop();
    seat = nullptr;
    queue.reset();
    builder.reset();
}

mir::geometry::Rectangle mi::DefaultInputDeviceHub::RegisteredDevice::bounding_rectangle() const
{
    if (!seat)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device not started and has no seat assigned"));

    return seat->bounding_rectangle();
}

mi::OutputInfo mi::DefaultInputDeviceHub::RegisteredDevice::output_info(uint32_t output_id) const
{
    if (!seat)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device not started and has no seat assigned"));

    return seat->output_info(output_id);
}

void mi::DefaultInputDeviceHub::RegisteredDevice::key_state(std::vector<uint32_t> const& scan_codes)
{
    if (!seat)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device not started and has no seat assigned"));

    seat->set_key_state(*handle, scan_codes);
}

void mi::DefaultInputDeviceHub::RegisteredDevice::pointer_state(MirPointerButtons buttons)
{
    if (!seat)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device not started and has no seat assigned"));

    seat->set_pointer_state(*handle, buttons);
}

void mi::DefaultInputDeviceHub::add_observer(std::shared_ptr<InputDeviceObserver> const& observer)
{
    device_queue->enqueue(
        [this,observer]()
        {
            std::unique_lock lock(mutex);
            for (auto const& item : handles)
                observer->device_added(item);
            observer->changes_complete();
            // Need to add observer under lock so that new handles can not be added between sending off initial
            // observations and when the observer is added
            observers.add(observer);
        });
}

void mi::DefaultInputDeviceHub::for_each_input_device(std::function<void(Device const&)> const& callback)
{
    std::unique_lock lock{mutex};
    for (auto const& item : handles)
        callback(*item);
}

void mi::DefaultInputDeviceHub::for_each_mutable_input_device(std::function<void(Device&)> const& callback)
{
    std::unique_lock lock{mutex};
    // perform_transaction is true if no transaction is already in-progress
    bool const perform_transaction = !pending_changes;
    if (perform_transaction)
    {
        pending_changes.emplace();
    }
    auto const handles_copy = handles;
    lock.unlock();

    for (auto const& item : handles_copy)
        callback(*item);

    if (perform_transaction)
    {
        complete_transaction();
    }
}

void mi::DefaultInputDeviceHub::remove_observer(std::weak_ptr<InputDeviceObserver> const& element)
{
    auto observer = element.lock();
    observers.remove(observer);
}

void mi::DefaultInputDeviceHub::add_device_handle(
    std::lock_guard<std::recursive_mutex> const&,
    std::shared_ptr<DefaultDevice> const& handle)
{
    handles.push_back(handle);

    observers.for_each([&handle](std::shared_ptr<InputDeviceObserver> const& observer)
        {
            observer->device_added(handle);
            observer->changes_complete();
        });

    if (!ready)
    {
        server_status_listener->ready_for_user_input();
        ready = true;
    }
}

void mi::DefaultInputDeviceHub::remove_device_handle(std::lock_guard<std::recursive_mutex> const&, MirInputDeviceId id)
{
    std::vector<std::shared_ptr<Device>> removed_devices;

    auto handle_it = remove_if(
        begin(handles),
        end(handles),
        [&](auto const& handle)
            {
            if (handle->id() != id)
                return false;
            removed_devices.push_back(handle);
            return true;
            });

    if (handle_it == end(handles))
        return;

    handles.erase(handle_it, end(handles));
    auto const no_of_devices = handles.size();

    for (auto const& handle : removed_devices)
    {
        observers.for_each([&](std::shared_ptr<InputDeviceObserver> const& observer)
            {
                observer->device_removed(handle);
                observer->changes_complete();
            });
    }

    removed_devices.clear();

    if (ready && 0 == no_of_devices)
    {
        ready = false;
        server_status_listener->stop_receiving_input();
    }
}

void mi::DefaultInputDeviceHub::device_changed(Device* dev)
{
    std::unique_lock lock{mutex};
    auto dev_it = find_if(begin(handles), end(handles), [dev](auto const& ptr){return ptr.get() == dev;});
    std::shared_ptr<Device> const dev_shared = *dev_it;
    if (pending_changes)
    {
        pending_changes->push_back(dev_shared);
    }
    else
    {
        // No transaction is in progress, so send out change and complete notification immediately
        lock.unlock();
        observers.for_each([&](std::shared_ptr<InputDeviceObserver> const& observer)
            {
                observer->device_changed(dev_shared);
                observer->changes_complete();
            });
    }
}

void mi::DefaultInputDeviceHub::complete_transaction()
{
    std::unique_lock lock{mutex};
    std::vector<std::shared_ptr<mi::Device>> devices_to_notify;
    if (pending_changes)
    {
        auto const devices_to_notify = std::move(pending_changes.value());
        pending_changes.reset();
        lock.unlock();

        observers.for_each([&](std::shared_ptr<InputDeviceObserver> const& observer)
            {
                for (auto const& dev : devices_to_notify)
                    observer->device_changed(dev);
                observer->changes_complete();
            });
    }
}

void mi::DefaultInputDeviceHub::store_device_config(std::lock_guard<std::recursive_mutex> const&, mi::DefaultDevice const& dev)
{
    stored_devices.push_back(dev.config());
}


auto mi::DefaultInputDeviceHub::get_stored_device_config(
    std::lock_guard<std::recursive_mutex> const&,
    std::string const& id) -> std::optional<MirInputDevice>
{
    std::optional<MirInputDevice> optional_config;
    auto pos = remove_if(
        begin(stored_devices),
        end(stored_devices),
        [&optional_config,id](auto const& handle)
        {
            if (id == handle.unique_id())
            {
                optional_config = handle;
                return true;
            }
            return false;
        });
    stored_devices.erase(pos, end(stored_devices));

    return optional_config;
}

std::shared_ptr<mi::DefaultDevice> mi::DefaultInputDeviceHub::restore_or_create_device(
    std::lock_guard<std::recursive_mutex> const& lock,
    mi::InputDevice& device,
    std::shared_ptr<mir::dispatch::ActionQueue> const& queue)
{
    auto device_config = get_stored_device_config(lock, device.get_device_info().unique_id);

    if (device_config)
        return std::make_shared<DefaultDevice>(
            device_config.value(),
            queue,
            device,
            key_mapper,
            [this](Device *d){device_changed(d);});
    else
        return std::make_shared<DefaultDevice>(
            create_new_device_id(lock),
            queue,
            device,
            key_mapper,
            [this](Device *d){device_changed(d);});
}
