/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_TEST_ADD_VIRTUAL_DEVICE_H
#define MIRAL_TEST_ADD_VIRTUAL_DEVICE_H

#include "mir/input/input_device_observer.h"
#include "mir/input/virtual_input_device.h"
#include "mir/input/device.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/input_device_registry.h"
#include "mir/test/signal.h"

namespace miral
{
namespace test
{
// Mostly to account for CI slowness.
// Adds a virtual device and waits till it's added before proceeding
inline std::shared_ptr<mir::input::VirtualInputDevice> add_test_device(
    std::shared_ptr<mir::input::InputDeviceRegistry> const& input_device_registry,
    std::shared_ptr<mir::input::InputDeviceHub> const& input_device_hub,
    mir::input::DeviceCapability capability)
{
    std::shared_ptr<mir::input::VirtualInputDevice> test_device =
        std::make_shared<mir::input::VirtualInputDevice>("test-device", capability);

    struct SignalingObserver : public mir::input::InputDeviceObserver
    {
        using Device = mir::input::Device;
        std::shared_ptr<Device> const device_to_wait_for;
        std::weak_ptr<mir::input::InputDeviceHub> const device_hub;
        mir::test::Signal device_added_;

        SignalingObserver(std::shared_ptr<Device> const& to_wait_for) :
            device_to_wait_for{to_wait_for}
        {
        }

        virtual void device_added(std::shared_ptr<Device> const& device) override
        {
            if (device->unique_id() == device_to_wait_for->unique_id())
                device_added_.raise();
        }

        virtual void device_changed(std::shared_ptr<Device> const&)
        {
        }
        virtual void device_removed(std::shared_ptr<Device> const&)
        {
        }
        virtual void changes_complete()
        {
        }

        void wait()
        {
            device_added_.wait();
        }
    };

    auto input_device_observer =
        std::make_shared<SignalingObserver>(input_device_registry->add_device(test_device).lock());
    input_device_hub->add_observer(input_device_observer);
    input_device_observer->wait();
    input_device_hub->remove_observer(input_device_observer);

    return test_device;
}
}
}

#endif // MIRAL_TEST_ADD_VIRTUAL_DEVICE_H
