/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir_test_framework/input_device_faker.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/fake_input_device.h"

#include "mir/test/signal_actions.h"

#include <mir/input/input_device_hub.h>
#include <mir/input/input_device_observer.h>
#include "mir/input/null_input_device_observer.h"
#include <mir/raii.h>
#include <mir/server.h>

#include <gtest/gtest.h>

#include <chrono>


auto mir_test_framework::InputDeviceFaker::add_fake_input_device(mir::input::InputDeviceInfo const& info) -> mir::UniqueModulePtr<FakeInputDevice>
{
    auto result = mir_test_framework::add_fake_input_device(info);
    ++expected_number_of_input_devices;
    return result;
}

void mir_test_framework::InputDeviceFaker::wait_for_input_devices_added_to(mir::Server& server)
{
    using namespace std::chrono_literals;
    using namespace testing;

    struct DeviceCounter : mir::input::NullInputDeviceObserver
    {
        DeviceCounter(std::function<void(size_t)> const& callback)
            : callback{callback}
        {}

        void device_added(std::shared_ptr<mir::input::Device> const&)   override { ++count_devices; }
        void device_removed(std::shared_ptr<mir::input::Device> const&) override { --count_devices; }
        void changes_complete()                                         override { callback(count_devices); }
        std::function<void(size_t)> const callback;
        int count_devices{0};
    };

    mir::test::Signal devices_available;

    // The fake input devices are registered from within the input thread, as soon as the
    // input manager starts. So clients may connect to the server before those additions
    // have been processed.
    auto counter = std::make_shared<DeviceCounter>(
        [&](int count)
            {
                if (count == expected_number_of_input_devices)
                    devices_available.raise();
            });

    auto hub = server.the_input_device_hub();

    auto const register_counter = mir::raii::paired_calls(
        [&]{ hub->add_observer(counter); },
        [&]{ hub->remove_observer(counter); });

    devices_available.wait_for(5s);
    ASSERT_THAT(counter->count_devices, Eq(expected_number_of_input_devices));
}
