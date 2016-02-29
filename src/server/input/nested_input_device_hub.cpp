/*
 * Copyright © 2016 Canonical Ltd.
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

#include "nested_input_device_hub.h"
#include "mir/input/device.h"
#include "mir/input/device_capability.h"
#include "mir/input/pointer_configuration.h"
#include "mir/input/touchpad_configuration.h"
#include "mir/input/input_device_observer.h"
#include "mir/server_action_queue.h"

#include <algorithm>
#include <iostream>

namespace mi = mir::input;
namespace mf = mir::frontend;

namespace
{

mir::input::UniqueInputConfig make_input_config(MirConnection* con)
{
    return mir::input::UniqueInputConfig(mir_connection_create_input_config(con), mir_input_config_destroy);
}

class NestedDevice : public mi::Device
{
public:
    NestedDevice(MirInputDevice const* dev)
    {
        update(dev);
    }

    void update(MirInputDevice const* dev)
    {
        device_id = mir_input_device_get_id(dev);
        device_name = mir_input_device_get_name(dev);
        unique_device_id = mir_input_device_get_unique_id(dev);
        caps = mi::DeviceCapabilities(mir_input_device_get_capabilities(dev));
    }

    MirInputDeviceId id() const
    {
        return device_id;
    }

    mi::DeviceCapabilities capabilities() const
    {
        return caps;
    }

    std::string name() const
    {
        return device_name;
    }
    std::string unique_id() const
    {
        return unique_device_id;
    }

    mir::optional_value<mi::PointerConfiguration> pointer_configuration() const
    {
        return pointer_conf;
    }
    void apply_pointer_configuration(mi::PointerConfiguration const&)
    {
        // TODO requires c api support
    }

    mir::optional_value<mi::TouchpadConfiguration> touchpad_configuration() const
    {
        return touchpad_conf;
    }
    void apply_touchpad_configuration(mi::TouchpadConfiguration const&)
    {
        // TODO requires c api support
    }
private:
    MirInputDeviceId device_id;
    std::string device_name;
    std::string unique_device_id;
    mi::DeviceCapabilities caps;
    mir::optional_value<mi::PointerConfiguration> pointer_conf;
    mir::optional_value<mi::TouchpadConfiguration> touchpad_conf;
};
}

mi::NestedInputDeviceHub::NestedInputDeviceHub(std::shared_ptr<MirConnection> const& connection,
                                               std::shared_ptr<mf::EventSink> const& sink,
                                               std::shared_ptr<mir::ServerActionQueue> const& observer_queue)
    : connection{connection},
      sink{sink},
      observer_queue{observer_queue},
      config{make_input_config(connection.get())}
{
    mir_connection_set_input_config_change_callback(
        connection.get(),
        [](MirConnection*, void* context)
        {
            auto obj = static_cast<mi::NestedInputDeviceHub*>(context);
            obj->update_devices();
        },
        this);

    update_devices();
}

void mir::input::NestedInputDeviceHub::update_devices()
{
    /// lock this thing or not?
    std::unique_lock<std::mutex> lock(devices_guard);
    config = make_input_config(connection.get());

    auto deleted = std::move(devices);
    std::vector<std::shared_ptr<mi::Device>> new_devs;
    auto config_ptr = config.get();
    for (size_t i = 0, e = mir_input_config_device_count(config_ptr); i!=e; ++i)
    {
        auto dev = mir_input_config_get_device(config_ptr, i);
        auto it = std::find_if(
            begin(deleted),
            end(deleted),
            [id = mir_input_device_get_id(dev)](auto const& dev)
            {
                return id == dev->id();
            });
        if (it != end(deleted))
        {
           std::static_pointer_cast<NestedDevice>(*it)->update(dev);
           devices.push_back(*it);
           deleted.erase(it);
        }
        else
        {
           devices.push_back(std::make_shared<NestedDevice>(dev));
           new_devs.push_back(devices.back());
        }
    }

    if ((deleted.empty() && new_devs.empty()) || observers.empty())
        return;

    observer_queue->enqueue(
        this,
        [this, new_devs = std::move(new_devs), deleted = std::move(deleted)]
        {
            std::unique_lock<std::mutex> lock(devices_guard);
            for (auto const observer : observers)
            {
                for (auto const& item : new_devs)
                    observer->device_added(item);
                for (auto const& item : deleted)
                    observer->device_removed(item);
                observer->changes_complete();
            }
        });
}

void mi::NestedInputDeviceHub::add_observer(std::shared_ptr<InputDeviceObserver> const& observer)
{
    observer_queue->enqueue(
        this,
        [observer,this]
        {
            std::unique_lock<std::mutex> lock(devices_guard);
            observers.push_back(observer);
            for (auto const& item : devices)
            {
                observer->device_added(item);
            }
            observer->changes_complete();
        }
        );
}

void mi::NestedInputDeviceHub::remove_observer(std::weak_ptr<InputDeviceObserver> const& element)
{
    auto observer = element.lock();

    observer_queue->enqueue(this,
                            [observer, this]
                            {
                                observers.erase(remove(begin(observers), end(observers), observer), end(observers));
                            });
}

void mi::NestedInputDeviceHub::for_each_input_device(std::function<void(std::shared_ptr<Device>const& device)> const& callback)
{
    std::unique_lock<std::mutex> lock(devices_guard);
    for (auto const& item : devices)
        callback(item);

}
