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

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/events/event_builders.h"
#include "mir/glib_main_loop.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/input_device_registry.h"
#include "mir/input/input_event_transformer.h"
#include "mir/input/transformer.h"
#include "mir/input/virtual_input_device.h"
#include "src/server/input/default_input_device_hub.h"

#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/mock_input_seat.h"
#include "mir/test/doubles/mock_key_mapper.h"
#include "mir/test/doubles/mock_led_observer_registrar.h"
#include "mir/test/doubles/mock_server_status_listener.h"
#include "mir/test/fake_shared.h"
#include "mir/test/fd_utils.h"
#include "mir/test/signal.h"

#include <functional>
#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <xkbcommon/xkbcommon-compat.h>

namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mev = mir::events;

using namespace testing;

struct TestInputEventTransformer : testing::Test
{
    mir::dispatch::MultiplexingDispatchable multiplexer;
    NiceMock<mtd::MockInputSeat> mock_seat;
    mtd::AdvanceableClock clock;

    void SetUp() override
    {
        expect_and_execute_multiplexer();
    }

    void TearDown() override
    {
    }

    // Borrowed from `test_single_seat_setup.cpp`
    void expect_and_execute_multiplexer()
    {
        (void)mt::fd_becomes_readable(multiplexer.watch_fd(), std::chrono::milliseconds(100));
        multiplexer.dispatch(mir::dispatch::FdEvent::readable);
    }

    mir::EventUPtr make_key_event()
    {
        return mev::make_key_event(
            MirInputDeviceId{0},
            clock.now().time_since_epoch(),
            mir_keyboard_action_up,
            XKB_KEY_W,
            0,
            mir_input_event_modifier_none);
    }

    mir::input::InputEventTransformer input_event_transformer{mt::fake_shared(mock_seat), mt::fake_shared(clock)};
};

struct MockTransformer : public mir::input::Transformer
{
    MOCK_METHOD(
        (bool),
        transform_input_event,
        (mir::input::Transformer::EventDispatcher const&, mir::input::EventBuilder*, MirEvent const&),
        (override));
};

TEST_F(TestInputEventTransformer, transformer_gets_called)
{
    auto mock_transformer = std::make_shared<MockTransformer>();
    EXPECT_CALL(*mock_transformer, transform_input_event(_, _,_));

    input_event_transformer.append(mock_transformer);

    input_event_transformer.transform(*make_key_event());
}

TEST_F(TestInputEventTransformer, events_block_correctly)
{
    auto mock_transformer_1 = std::make_shared<MockTransformer>();
    EXPECT_CALL(*mock_transformer_1, transform_input_event(_, _, _))
        .WillOnce(
            [](auto*...)
            {
                return true;
            });

    auto mock_transformer_2 = std::make_shared<MockTransformer>();
    EXPECT_CALL(*mock_transformer_2, transform_input_event(_, _, _)).Times(0);

    input_event_transformer.append(mock_transformer_1);
    input_event_transformer.append(mock_transformer_2);

    input_event_transformer.transform(*make_key_event());
}

TEST_F(TestInputEventTransformer, events_cascade_correctly)
{
    auto mock_transformer_1 = std::make_shared<MockTransformer>();
    EXPECT_CALL(*mock_transformer_1, transform_input_event(_, _, _))
        .WillOnce(
            [](auto*...)
            {
                return false;
            });

    auto mock_transformer_2 = std::make_shared<MockTransformer>();
    EXPECT_CALL(*mock_transformer_2, transform_input_event(_, _, _));

    input_event_transformer.append(mock_transformer_1);
    input_event_transformer.append(mock_transformer_2);

    input_event_transformer.transform(*make_key_event());
}

TEST_F(TestInputEventTransformer, transformer_not_called_after_removal)
{
    auto mock_transformer = std::make_shared<MockTransformer>();
    EXPECT_CALL(*mock_transformer, transform_input_event(_, _, _))
        .Times(1)
        .WillOnce(
            [](auto*...)
            {
                return true;
            });

    input_event_transformer.append(mock_transformer);
    input_event_transformer.transform(*make_key_event());
    input_event_transformer.remove(mock_transformer);
    input_event_transformer.transform(*make_key_event());
}

TEST_F(TestInputEventTransformer, adding_transformer_twice_has_no_effect_on_expected_handling_of_events)
{
    auto mock_transformer_1 = std::make_shared<MockTransformer>();
    EXPECT_CALL(*mock_transformer_1, transform_input_event(_, _, _)).Times(1);

    input_event_transformer.append(mock_transformer_1);
    input_event_transformer.append(mock_transformer_1);
    input_event_transformer.transform(*make_key_event());
}
