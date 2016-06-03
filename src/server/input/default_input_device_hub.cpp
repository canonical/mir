/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "default_input_device_hub.h"
#include "default_device.h"

#include "mir/input/input_device.h"
#include "mir/input/input_device_observer.h"
#include "mir/geometry/point.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/action_queue.h"
#include "mir/frontend/event_sink.h"
#include "mir/server_action_queue.h"
#include "mir/cookie/authority.h"
#define MIR_LOG_COMPONENT "Input"
#include "mir/log.h"

#include "boost/throw_exception.hpp"

#include <algorithm>
#include <atomic>

namespace mi = mir::input;
namespace mf = mir::frontend;

mi::DefaultInputDeviceHub::DefaultInputDeviceHub(
    std::shared_ptr<mf::EventSink> const& sink,
    std::shared_ptr<mi::Seat> const& seat,
    std::shared_ptr<dispatch::MultiplexingDispatchable> const& input_multiplexer,
    std::shared_ptr<mir::ServerActionQueue> const& observer_queue,
    std::shared_ptr<mir::cookie::Authority> const& cookie_authority,
    std::shared_ptr<mi::KeyMapper> const& key_mapper)
    : seat{seat},
      sink{sink},
      input_dispatchable{input_multiplexer},
      observer_queue(observer_queue),
      device_queue(std::make_shared<dispatch::ActionQueue>()),
      cookie_authority(cookie_authority),
      key_mapper(key_mapper),
      device_id_generator{0}
{
    input_dispatchable->add_watch(device_queue);
}

void mi::DefaultInputDeviceHub::add_device(std::shared_ptr<InputDevice> const& device)
{
    if (!device)
        BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid input device"));

    auto it = find_if(devices.cbegin(),
                      devices.cend(),
                      [&device](std::unique_ptr<RegisteredDevice> const& item)
                      {
                          return item->device_matches(device);
                      });

    if (it == end(devices))
    {
        auto id = create_new_device_id();
        auto handle = std::make_shared<DefaultDevice>(id, device_queue, *device, key_mapper);
        // send input device info to observer loop..
        devices.push_back(std::make_unique<RegisteredDevice>(
            device, id, input_dispatchable, cookie_authority, handle));

        auto const& dev = devices.back();

        seat->add_device(*handle);
        dev->start(seat);

        // pass input device handle to observer loop..
        observer_queue->enqueue(this,
                                [this, handle]()
                                {
                                    add_device_handle(handle);
                                });

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

    auto pos = remove_if(
        begin(devices),
        end(devices),
        [&device,this](std::unique_ptr<RegisteredDevice> const& item)
        {
            if (item->device_matches(device))
            {
                auto seat = item->seat;
                if (seat)
                {
                    seat->remove_device(*item->handle);
                    item->stop();
                }
                // send input device info to observer queue..
                observer_queue->enqueue(
                    this,
                    [this,id = item->id()]()
                    {
                        remove_device_handle(id);
                    });

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
    std::shared_ptr<dispatch::MultiplexingDispatchable> const& multiplexer,
    std::shared_ptr<mir::cookie::Authority> const& cookie_authority,
    std::shared_ptr<mi::DefaultDevice> const& handle)
    : handle(handle),
      device_id(device_id),
      cookie_authority(cookie_authority),
      device(dev),
      multiplexer(multiplexer)
{
}

MirInputDeviceId mi::DefaultInputDeviceHub::create_new_device_id()
{
    return ++device_id_generator;
}

MirInputDeviceId mi::DefaultInputDeviceHub::RegisteredDevice::id()
{
    return device_id;
}

void mi::DefaultInputDeviceHub::RegisteredDevice::handle_input(MirEvent& event)
{
    auto type = mir_event_get_type(&event);

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

void mi::DefaultInputDeviceHub::RegisteredDevice::start(std::shared_ptr<Seat> const& seat)
{
    this->seat = seat;
    builder = std::make_unique<DefaultEventBuilder>(device_id, cookie_authority, seat);
    device->start(this, builder.get());
}

void mi::DefaultInputDeviceHub::RegisteredDevice::stop()
{
    device->stop();
    seat = nullptr;
    builder.reset();
}

mir::geometry::Rectangle mi::DefaultInputDeviceHub::RegisteredDevice::bounding_rectangle() const
{
    if (!seat)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device not started and has no seat assigned"));

    return seat->get_rectangle_for(*handle);
}

void mi::DefaultInputDeviceHub::RegisteredDevice::set_key_state(std::vector<uint32_t> const& scan_codes)
{
    if (!seat)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device not started and has no seat assigned"));

    seat->set_key_state(*handle, scan_codes);
}

void mi::DefaultInputDeviceHub::RegisteredDevice::set_pointer_state(MirPointerButtons buttons)
{
    if (!seat)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device not started and has no seat assigned"));

    seat->set_pointer_state(*handle, buttons);
}

void mi::DefaultInputDeviceHub::add_observer(std::shared_ptr<InputDeviceObserver> const& observer)
{
    observer_queue->enqueue(
        this,
        [observer,this]
        {
            std::unique_lock<std::mutex> lock(observer_guard);
            observers.push_back(observer);
            for (auto const& item : handles)
            {
                observer->device_added(item);
            }
            observer->changes_complete();
        }
        );
}

void mi::DefaultInputDeviceHub::for_each_input_device(std::function<void(Device const&)> const& callback)
{
    std::unique_lock<std::mutex> lock(observer_guard);
    for (auto const& item : handles)
        callback(*item);
}

void mi::DefaultInputDeviceHub::remove_observer(std::weak_ptr<InputDeviceObserver> const& element)
{
    auto observer = element.lock();

    observer_queue->enqueue(this,
                            [observer, this]
                            {
                                std::unique_lock<std::mutex> lock(observer_guard);
                                observers.erase(remove(begin(observers), end(observers), observer), end(observers));
                            });
}

void mi::DefaultInputDeviceHub::add_device_handle(std::shared_ptr<DefaultDevice> const& handle)
{
    std::unique_lock<std::mutex> lock(observer_guard);
    handles.push_back(handle);
    sink->handle_input_device_change(handles);

    for (auto const& observer : observers)
    {
        observer->device_added(handles.back());
        observer->changes_complete();
    }
}

void mi::DefaultInputDeviceHub::remove_device_handle(MirInputDeviceId id)
{
    std::unique_lock<std::mutex> lock(observer_guard);
    auto handle_it = remove_if(
        begin(handles),
        end(handles),
        [this,&id](auto const& handle)
        {
            if (handle->id() != id)
                return false;
            for (auto const& observer : observers)
            {
                observer->device_removed(handle);
                observer->changes_complete();
            }
            return true;
        });

    if (handle_it == end(handles))
        return;

    handles.erase(handle_it, end(handles));
    sink->handle_input_device_change(handles);
}
