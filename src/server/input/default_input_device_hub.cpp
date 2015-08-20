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

#include "mir/input/input_dispatcher.h"
#include "mir/input/input_device.h"
#include "mir/input/input_device_observer.h"
#include "mir/input/cursor_listener.h"
#include "mir/input/input_region.h"
#include "mir/events/event_private.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/server_action_queue.h"
#define MIR_LOG_COMPONENT "Input"
#include "mir/log.h"

#include "boost/throw_exception.hpp"

#include <algorithm>
#include <atomic>

namespace mi = mir::input;

mi::DefaultInputDeviceHub::DefaultInputDeviceHub(
    std::shared_ptr<mi::InputDispatcher> const& input_dispatcher,
    std::shared_ptr<dispatch::MultiplexingDispatchable> const& input_multiplexer,
    std::shared_ptr<mir::ServerActionQueue> const& observer_queue,
    std::shared_ptr<TouchVisualizer> const& touch_visualizer,
    std::shared_ptr<CursorListener> const& cursor_listener,
    std::shared_ptr<InputRegion> const& input_region)
    : input_dispatcher(input_dispatcher), input_dispatchable{input_multiplexer}, observer_queue(observer_queue),
      touch_visualizer(touch_visualizer), cursor_listener(cursor_listener), input_region(input_region)
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
        devices.push_back(std::make_unique<RegisteredDevice>(device, input_dispatcher, input_dispatchable, this));

        auto info = devices.back()->get_device_info();

        // send input device info to observer loop..
        observer_queue->enqueue(this,
                                [this,info]()
                                {
                                    add_device_info(info);
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

                // send input device info to observer queue..
                observer_queue->enqueue(
                    this,
                    [this,id = item->id()]()
                    {
                        remove_device_info(id);
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
    std::shared_ptr<InputDispatcher> const& dispatcher,
    std::shared_ptr<dispatch::MultiplexingDispatchable> const& multiplexer,
    DefaultInputDeviceHub *hub)
    : device_id(create_new_device_id()), device(dev), dispatcher(dispatcher), multiplexer(multiplexer), hub(hub)
{
}

mi::InputDeviceInfo mi::DefaultInputDeviceHub::RegisteredDevice::get_device_info()
{
    InputDeviceInfo ret = device->get_device_info();
    // TODO consider storing device id outside of InputDeviceInfo
    ret.id = device_id;
    return ret;
}

int32_t mi::DefaultInputDeviceHub::RegisteredDevice::create_new_device_id()
{
    static int32_t device_id{0};
    return ++device_id;
}

int32_t mi::DefaultInputDeviceHub::RegisteredDevice::id()
{
    return device_id;
}

void mi::DefaultInputDeviceHub::RegisteredDevice::handle_input(MirEvent& event)
{
    // we attach the device id here, since this instance the first being able to maintains the uniqueness of the ids..
    if (event.type == mir_event_type_key)
    {
        event.key.device_id = device_id;
    }
    if (event.type == mir_event_type_motion)
    {
        event.motion.device_id = device_id;
    }
    auto type = mir_event_get_type(&event);

    if (type != mir_event_type_input)
        BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid input event receivd from device"));

    update_spots(mir_event_get_input_event(&event));
    notify_cursor_listener(mir_event_get_input_event(&event));
    dispatcher->dispatch(event);
}

void mi::DefaultInputDeviceHub::RegisteredDevice::notify_cursor_listener(MirInputEvent const* event)
{
    if (mir_input_event_get_type(event) != mir_input_event_type_pointer)
        return;

    auto pointer_ev = mir_input_event_get_pointer_event(event);
    hub->cursor_listener->cursor_moved_to(
        mir_pointer_event_axis_value(pointer_ev, mir_pointer_axis_x),
        mir_pointer_event_axis_value(pointer_ev, mir_pointer_axis_y)
        );
}

void mi::DefaultInputDeviceHub::RegisteredDevice::update_spots(MirInputEvent const* event)
{
    if (mir_input_event_get_type(event) != mir_input_event_type_touch)
        return;

    auto const* touch_event = mir_input_event_get_touch_event(event);
    auto count = mir_touch_event_point_count(touch_event);
    touch_spots.reserve(count);
    touch_spots.clear();
    for (decltype(count) i = 0; i != count; ++i)
    {
        if (mir_touch_event_action(touch_event, i) == mir_touch_action_up)
            continue;
        touch_spots.push_back(mi::TouchVisualizer::Spot{
            {mir_touch_event_axis_value(touch_event, i, mir_touch_axis_x),
             mir_touch_event_axis_value(touch_event, i, mir_touch_axis_y)},
            mir_touch_event_axis_value(touch_event, i, mir_touch_axis_pressure)});
    }

    hub->update_spots();
}

std::vector<mir::input::TouchVisualizer::Spot> const& mi::DefaultInputDeviceHub::RegisteredDevice::spots() const
{
    return touch_spots;
}

bool mi::DefaultInputDeviceHub::RegisteredDevice::device_matches(std::shared_ptr<InputDevice> const& dev) const
{
    return dev == device;
}

void mi::DefaultInputDeviceHub::RegisteredDevice::start()
{
    device->start(this);
    multiplexer->add_watch(device->dispatchable());
}

void mi::DefaultInputDeviceHub::RegisteredDevice::stop()
{
    multiplexer->remove_watch(device->dispatchable());
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
            for (auto const& item : infos)
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

void mi::DefaultInputDeviceHub::add_device_info(InputDeviceInfo const& info)
{
    infos.push_back(info);

    for (auto const& observer : observers)
    {
        observer->device_added(infos.back());
        observer->changes_complete();
    }
}

void mi::DefaultInputDeviceHub::remove_device_info(int32_t id)
{
    auto info_it = remove_if(begin(infos), end(infos), [&id](auto const& info){return info.id == id;});

    if (info_it == end(infos))
        return;
    for (auto const& observer : observers)
    {
        observer->device_removed(*info_it);
        observer->changes_complete();
    }

    infos.erase(info_it, end(infos));
}

void mi::DefaultInputDeviceHub::update_spots()
{
    std::vector<TouchVisualizer::Spot> spots;

    for (auto const& dev : devices)
        spots.insert(end(spots), begin(dev->spots()), end(dev->spots()));

    touch_visualizer->visualize_touches(spots);
}
