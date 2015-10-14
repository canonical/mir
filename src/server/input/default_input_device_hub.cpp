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
#include "device_handle.h"

#include "seat.h"
#include "mir/input/input_dispatcher.h"
#include "mir/input/input_device.h"
#include "mir/input/input_device_observer.h"
#include "mir/input/input_region.h"
#include "mir/geometry/point.h"
#include "mir/events/event_builders.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/server_action_queue.h"
#include "mir/cookie_factory.h"
#define MIR_LOG_COMPONENT "Input"
#include "mir/log.h"

#include "boost/throw_exception.hpp"

#include <algorithm>
#include <atomic>

namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mev = mir::events;

mi::DefaultInputDeviceHub::DefaultInputDeviceHub(
    std::shared_ptr<mi::InputDispatcher> const& input_dispatcher,
    std::shared_ptr<dispatch::MultiplexingDispatchable> const& input_multiplexer,
    std::shared_ptr<mir::ServerActionQueue> const& observer_queue,
    std::shared_ptr<TouchVisualizer> const& touch_visualizer,
    std::shared_ptr<CursorListener> const& cursor_listener,
    std::shared_ptr<InputRegion> const& input_region,
    std::shared_ptr<mir::cookie::CookieFactory> const& cookie_factory)
    : input_dispatcher(input_dispatcher),
      input_dispatchable{input_multiplexer},
      observer_queue(observer_queue),
      input_region(input_region),
      cookie_factory(cookie_factory),
      seat(touch_visualizer, cursor_listener),
      device_id_generator{0}
{
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
        // send input device info to observer loop..
        devices.push_back(std::make_unique<RegisteredDevice>(
            device, create_new_device_id(), input_dispatcher, input_dispatchable, cookie_factory, this, &seat));

        auto const& dev = devices.back();
        seat.add_device(dev->id());

        auto handle = std::make_shared<DefaultDevice>(
            dev->id(), device->get_device_info());


        // pass input device handle to observer loop..
        observer_queue->enqueue(this,
                                [this, handle]()
                                {
                                    add_device_handle(handle);
                                });

        // TODO let shell decide if device should be observed / exposed to clients.
        devices.back()->start();
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
                item->stop();
                seat.remove_device(item->id());

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
    std::shared_ptr<InputDispatcher> const& dispatcher,
    std::shared_ptr<dispatch::MultiplexingDispatchable> const& multiplexer,
    std::shared_ptr<mir::cookie::CookieFactory> const& cookie_factory,
    DefaultInputDeviceHub* hub,
    Seat* seat)
    : device_id(device_id), builder(device_id, cookie_factory), device(dev), dispatcher(dispatcher),
      multiplexer(multiplexer), hub(hub), seat(seat)
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

    if (type != mir_event_type_input)
        BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid input event receivd from device"));

    seat->update_seat_properties(mir_event_get_input_event(&event));

    mev::set_modifier(event, seat->event_modifier());

    dispatcher->dispatch(event);
}

bool mi::DefaultInputDeviceHub::RegisteredDevice::device_matches(std::shared_ptr<InputDevice> const& dev) const
{
    return dev == device;
}

void mi::DefaultInputDeviceHub::RegisteredDevice::start()
{
    device->start(this, &builder);
}

void mi::DefaultInputDeviceHub::RegisteredDevice::stop()
{
    device->stop();
}

void mi::DefaultInputDeviceHub::RegisteredDevice::confine_pointer(mir::geometry::Point& position)
{
    hub->input_region->confine(position);
}

mir::geometry::Rectangle mi::DefaultInputDeviceHub::RegisteredDevice::bounding_rectangle() const
{
    // TODO touchscreens only need the bounding rectangle of one output
    return hub->input_region->bounding_rectangle();
}

void mi::DefaultInputDeviceHub::add_observer(std::shared_ptr<InputDeviceObserver> const& observer)
{
    observer_queue->enqueue(
        this,
        [observer,this]
        {
            observers.push_back(observer);
            for (auto const& item : handles)
            {
                observer->device_added(item);
            }
            observer->changes_complete();
        }
        );
}

void mi::DefaultInputDeviceHub::remove_observer(std::weak_ptr<InputDeviceObserver> const& element)
{
    auto observer = element.lock();

    observer_queue->enqueue(this,
                            [observer, this]
                            {
                                observers.erase(remove(begin(observers), end(observers), observer), end(observers));
                            });
}

void mi::DefaultInputDeviceHub::add_device_handle(std::shared_ptr<DefaultDevice> const& handle)
{
    handles.push_back(handle);

    for (auto const& observer : observers)
    {
        observer->device_added(handles.back());
        observer->changes_complete();
    }
}

void mi::DefaultInputDeviceHub::remove_device_handle(MirInputDeviceId id)
{
    auto handle_it = remove_if(begin(handles),
                               end(handles),
                               [&id](auto const& handle){return handle->id() == id;});

    if (handle_it == end(handles))
        return;
    for (auto const& observer : observers)
    {
        observer->device_removed(*handle_it);
        observer->changes_complete();
    }

    handles.erase(handle_it, end(handles));
}
