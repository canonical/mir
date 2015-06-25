/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/input/input_device_info.h"
#include "mir/input/input_device_observer.h"
#include "mir/input/input_device_hub.h"

#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir/test/wait_condition.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

namespace
{

struct MockInputDeviceObserver : public mi::InputDeviceObserver
{
    MOCK_METHOD1(device_added, void (mi::InputDeviceInfo const& device));
    MOCK_METHOD1(device_changed, void(mi::InputDeviceInfo const& device));
    MOCK_METHOD1(device_removed, void(mi::InputDeviceInfo const& device));
    MOCK_METHOD0(changes_complete, void());
};

struct TestInputDeviceHub : mtf::HeadlessInProcessServer
{
    MockInputDeviceObserver observer;
    mt::WaitCondition observer_registered;
    mt::WaitCondition callbacks_received;
    std::shared_ptr<mtf::FakeInputDevice> keep_on_living;
};

}

TEST_F(TestInputDeviceHub, calls_observers_with_changes_complete_on_registry)
{
    using namespace testing;
    EXPECT_CALL(observer, changes_complete())
        .WillOnce(mt::WakeUp(&observer_registered));

    server.the_input_device_hub()->add_observer(mt::fake_shared(observer));
    observer_registered.wait_for_at_most_seconds(4);
}

TEST_F(TestInputDeviceHub, notifies_input_device_observer_about_available_devices)
{
    using namespace testing;
    InSequence seq;
    EXPECT_CALL(observer, changes_complete())
        .WillOnce(mt::WakeUp(&observer_registered));

    EXPECT_CALL(observer, device_added(_));
    EXPECT_CALL(observer, changes_complete())
        .WillOnce(mt::WakeUp(&callbacks_received));

    server.the_input_device_hub()->add_observer(mt::fake_shared(observer));
    observer_registered.wait_for_at_most_seconds(4);

    keep_on_living = mtf::add_fake_input_device(mi::InputDeviceInfo{0, "keyboard", "keyboard-uid", mir::input::DeviceCapability::keyboard});

    callbacks_received.wait_for_at_most_seconds(4);
}
