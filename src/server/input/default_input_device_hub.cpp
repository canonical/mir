/*
 * Copyright © 2014 Canonical Ltd.
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

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "default_input_device_hub.h"

#include "mir/input/input_dispatcher.h"
#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h"
#include "mir/input/input_sink.h"
#include "mir/server_action_queue.h"

#include <algorithm>
#include <atomic>
#include <iostream>

namespace mi = mir::input;

mi::DefaultInputDeviceHub::DefaultInputDeviceHub(
    std::shared_ptr<mi::InputDispatcher> const& input_dispatcher,
    std::shared_ptr<mir::MainLoop> const& input_loop,
    std::shared_ptr<mir::ServerActionQueue> const& observer_queue)
    : input_dispatcher(input_dispatcher), input_handler_register(input_loop),
    observer_queue(observer_queue)
{
}

void mi::DefaultInputDeviceHub::add_device(std::shared_ptr<InputDevice> const& device)
{
    auto it = find_if(devices.cbegin(),
            devices.cend(),
            [&device](std::unique_ptr<RegisteredDevice> const& item)
            {
                return item->device_matches(device);
            }
            );

    if (it == end(devices))
    {
        devices.push_back(std::unique_ptr<RegisteredDevice>(new RegisteredDevice{device, input_dispatcher}));

        auto info = devices.back()->get_device_info();

        // send input device info to observer loop..
        observer_queue->enqueue(this,
                               [this,info]()
                               {
                                   add_device_info(info);
                               }
                              );


        // TODO let shell decide if device should be observed / exposed to clients.
        devices.back()->start(input_handler_register);
    }
}

void mi::DefaultInputDeviceHub::remove_device(std::shared_ptr<InputDevice> const& device)
{
    devices.erase(
        remove_if(
            begin(devices),
            end(devices),
            [&device,this](std::unique_ptr<RegisteredDevice> const& item)
            {
                if (item->device_matches(device))
                {
                    item->stop(input_handler_register);
                    auto const && info = item->get_device_info();

                    // send input device info to observer queue..
                    observer_queue->enqueue(this,
                                            [this,info]()
                                            {
                                                remove_device_info(info);
                                            }
                                           );
                    return true;
                }
                return false;
            }),
        end(devices)
        );
}

mi::DefaultInputDeviceHub::RegisteredDevice::RegisteredDevice(std::shared_ptr<InputDevice> const& dev, std::shared_ptr<InputDispatcher> const& dispatcher)
    : device_id(create_new_device_id()),
    device(dev), dispatcher(dispatcher)
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
    // TODO come up with a reasonable solution..
    static int32_t device_id{0};
    return ++device_id;
}

void mi::DefaultInputDeviceHub::RegisteredDevice::handle_input(MirEvent& event)
{
    if (event.type == mir_event_type_key)
    {
        event.key.device_id = device_id;
    }
    if (event.type == mir_event_type_motion)
    {
        event.motion.device_id = device_id;
    }
    auto type = mir_event_get_type(&event);

    if (type == mir_event_type_input)
    {
        dispatcher->dispatch(event);
    }
}

bool mi::DefaultInputDeviceHub::RegisteredDevice::device_matches(std::shared_ptr<InputDevice> const& dev) const
{
    return dev == device;
}

void mi::DefaultInputDeviceHub::RegisteredDevice::start(mi::InputEventHandlerRegister& registry)
{
    device->start(registry, *this);
}

void mi::DefaultInputDeviceHub::RegisteredDevice::stop(mi::InputEventHandlerRegister& registry)
{
    device->stop(registry);
}


void mi::DefaultInputDeviceHub::add_observer(std::shared_ptr<InputDeviceObserver> const& observer)
{
    observer_queue->enqueue(
        this,
        [observer,this]
        {
            observers.push_back(observer);
            for(auto const& item : infos)
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

    observer_queue->enqueue(
        this,
        [observer,this]
        {
            observers.erase(
                remove(
                    begin(observers),
                    end(observers),
                    observer
                    ),
                end(observers)
                );
        }
        );
}

void mi::DefaultInputDeviceHub::add_device_info(InputDeviceInfo const& info)
{
    infos.push_back(info);

    for(auto const& observer : observers)
    {
        observer->device_added(infos.back());
        observer->changes_complete();
    }
}

void mi::DefaultInputDeviceHub::remove_device_info(InputDeviceInfo const& info)
{
    for(auto const& observer : observers)
    {
        observer->device_removed(info);
        observer->changes_complete();
    }

    infos.erase(
        remove(
            begin(infos),
            end(infos),
            info),
        end(infos)
        );
}

