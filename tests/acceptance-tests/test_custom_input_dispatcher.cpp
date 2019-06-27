/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/input/input_device_info.h"

#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir/test/doubles/mock_input_dispatcher.h"
#include "mir/test/signal_actions.h"
#include "mir/test/fake_shared.h"
#include "mir/test/event_matchers.h"
#include "mir/test/event_factory.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <linux/input.h>

namespace mi = mir::input;
namespace mt = mir::test;
namespace mis = mir::input::synthesis;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

namespace
{
struct TestCustomInputDispatcher : mtf::HeadlessInProcessServer
{
    testing::NiceMock<mtd::MockInputDispatcher> input_dispatcher;

    void SetUp()
    {
    }

    std::unique_ptr<mtf::FakeInputDevice> fake_keyboard{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"keyboard", "keyboard-uid" , mi::DeviceCapability::keyboard})
        };
    std::unique_ptr<mtf::FakeInputDevice> fake_pointer{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"mouse", "mouse-uid" , mi::DeviceCapability::pointer})
        };
    mir::test::Signal all_keys_received;
    mir::test::Signal all_pointer_events_received;
};
}

TEST_F(TestCustomInputDispatcher, gets_started_and_stopped)
{
    server.override_the_input_dispatcher(
            [this]()
            {
                testing::InSequence seq;
                EXPECT_CALL(input_dispatcher, start());
                EXPECT_CALL(input_dispatcher, stop());
                return mt::fake_shared(input_dispatcher);
            });
    start_server();
}

TEST_F(TestCustomInputDispatcher, receives_input)
{
    server.override_the_input_dispatcher([this](){return mt::fake_shared(input_dispatcher);});
    start_server();
    using namespace testing;

    // the order of those two occuring is not guranteed, since the input is simulated from
    // separate devices - if the sequence of events is required in a test, better use
    // just one device with the superset of the capabilities instead.
    EXPECT_CALL(input_dispatcher, dispatch(mt::PointerEventWithPosition(1, 1))).Times(1)
        .WillOnce(mt::ReturnFalseAndWakeUp(&all_pointer_events_received));
    EXPECT_CALL(input_dispatcher, dispatch(mt::KeyDownEvent())).Times(1)
        .WillOnce(mt::ReturnFalseAndWakeUp(&all_keys_received));

    fake_pointer->emit_event(mis::a_pointer_event().with_movement(1, 1));
    fake_keyboard->emit_event(mis::a_key_down_event().of_scancode(KEY_M));

    all_keys_received.wait_for(std::chrono::seconds{10});
    all_pointer_events_received.wait_for(std::chrono::seconds{10});
}
