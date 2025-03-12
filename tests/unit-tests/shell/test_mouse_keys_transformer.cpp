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
#include "mir/events/event.h"
#include "mir/events/event_builders.h"
#include "mir/events/input_event.h"
#include "mir/events/pointer_event.h"
#include "mir/geometry/forward.h"
#include "mir/glib_main_loop.h"
#include "mir/input/input_event_transformer.h"
#include "mir/main_loop.h"
#include "mir/test/auto_unblock_thread.h"
#include "mir/test/spin_wait.h"
#include "src/server/input/default_input_device_hub.h"
#include "src/server/shell/mouse_keys_transformer.h"

#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/mock_input_seat.h"
#include "mir/test/doubles/mock_key_mapper.h"
#include "mir/test/doubles/mock_led_observer_registrar.h"
#include "mir/test/doubles/mock_server_status_listener.h"
#include "mir/test/fake_shared.h"

#include "gmock/gmock.h"
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <memory>
#include <pthread.h>

namespace mev = mir::events;
namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace ::testing;

struct TestMouseKeysTransformer : testing::Test
{
    using AccelerationParameters = mir::input::MouseKeysTransformer::AccelerationParameters;

    mir::dispatch::MultiplexingDispatchable multiplexer;
    NiceMock<mtd::MockLedObserverRegistrar> led_observer_registrar;
    NiceMock<mtd::MockInputSeat> mock_seat;
    NiceMock<mtd::MockKeyMapper> mock_key_mapper;
    NiceMock<mtd::MockServerStatusListener> mock_server_status_listener;
    mtd::AdvanceableClock clock;

    TestMouseKeysTransformer() :
        main_loop{std::make_shared<mir::GLibMainLoop>(mt::fake_shared(clock))},
        input_device_hub{std::make_shared<mir::input::DefaultInputDeviceHub>(
            mt::fake_shared(mock_seat),
            mt::fake_shared(multiplexer),
            mt::fake_shared(clock),
            mt::fake_shared(mock_key_mapper),
            mt::fake_shared(mock_server_status_listener),
            mt::fake_shared(led_observer_registrar))},
        input_event_transformer{input_device_hub, main_loop},
        transformer{std::make_shared<mi::MouseKeysTransformer>(
            main_loop, mir::geometry::Displacement{0, 0}, AccelerationParameters{1, 1, 1})},
        main_loop_thread{
            [this]()
            {
                main_loop->stop();
            },
            [this]()
            {
                main_loop->run();
            }}
    {
        input_event_transformer.append(transformer);
    }

    std::shared_ptr<mir::MainLoop> const main_loop;
    std::shared_ptr<mi::DefaultInputDeviceHub> const input_device_hub;
    mi::InputEventTransformer input_event_transformer;
    std::shared_ptr<mi::MouseKeysTransformer> const transformer;
    mt::AutoUnblockThread main_loop_thread;

    auto down_event(int button) const -> mir::EventUPtr
    {
        return mev::make_key_event(
            MirInputDeviceId{0},
            clock.now().time_since_epoch(),
            mir_keyboard_action_down,
            button,
            0,
            mir_input_event_modifier_none);
    }

    auto up_event(int button) const -> mir::EventUPtr
    {
        return mev::make_key_event(
            MirInputDeviceId{0},
            clock.now().time_since_epoch(),
            mir_keyboard_action_up,
            button,
            0,
            mir_input_event_modifier_none);
    }
};

TEST_F(TestMouseKeysTransformer, single_keyboard_event_to_single_pointer_motion_event)
{
    for (auto const button : {XKB_KEY_KP_2, XKB_KEY_KP_4, XKB_KEY_KP_6, XKB_KEY_KP_8})
    {
        auto const timeout = std::chrono::milliseconds(20);
        auto const repeat_delay = std::chrono::milliseconds(2);
        auto const num_invocations = timeout / repeat_delay;

        EXPECT_CALL(mock_seat, dispatch_event(_))
            .Times(AtLeast(num_invocations / 2)) // Account for various timing inconsistencies
            .WillRepeatedly(
                [](std::shared_ptr<MirEvent> const& event)
                {
                    auto* const pointer_event = event->to_input()->to_pointer();

                    auto const pointer_action = pointer_event->action();
                    auto const pointer_relative_motion = pointer_event->motion();

                    ASSERT_EQ(pointer_action, mir_pointer_action_motion);

                    // Make sure the movement is on one axis
                    if(pointer_relative_motion.dx.as_value() != 0.0f)
                        ASSERT_EQ(pointer_relative_motion.dy.as_value(), 0.0f);

                    if(pointer_relative_motion.dy.as_value() != 0.0f)
                        ASSERT_EQ(pointer_relative_motion.dx.as_value(), 0.0f);
                });

        auto down = down_event(button);
        input_event_transformer.handle(*down);

        // Should generate a bunch of motion events until we handle the up
        // event
        auto const step = std::chrono::milliseconds(1);
        mt::spin_wait_for_condition_or_timeout(
            [&]
            {
                clock.advance_by(step);
                return false;
            },
            timeout,
            step);

        input_event_transformer.handle(*up_event(button));
    }
}

TEST_F(TestMouseKeysTransformer, multiple_keys_result_in_diagonal_movement)
{
    auto const combinations = {
        std::pair{XKB_KEY_KP_2, XKB_KEY_KP_4},
        std::pair{XKB_KEY_KP_2, XKB_KEY_KP_6},
        std::pair{XKB_KEY_KP_4, XKB_KEY_KP_8},
        std::pair{XKB_KEY_KP_4, XKB_KEY_KP_2},
        std::pair{XKB_KEY_KP_6, XKB_KEY_KP_8},
        std::pair{XKB_KEY_KP_6, XKB_KEY_KP_2},
        std::pair{XKB_KEY_KP_8, XKB_KEY_KP_4},
        std::pair{XKB_KEY_KP_8, XKB_KEY_KP_6},
    };

    for(auto const[key1, key2]: combinations)
    {
        auto const timeout = std::chrono::milliseconds(20);
        auto const repeat_delay = std::chrono::milliseconds(2);
        auto const num_invocations = timeout / repeat_delay;

        auto count_diagonal = 0;
        EXPECT_CALL(mock_seat, dispatch_event(_))
            .Times(AtLeast(num_invocations / 2)) // Account for various timing inconsistencies
            .WillRepeatedly(
                [&](std::shared_ptr<MirEvent> const& event)
                {
                    auto* const pointer_event = event->to_input()->to_pointer();

                    auto const pointer_action = pointer_event->action();
                    auto const pointer_relative_motion = pointer_event->motion();

                    ASSERT_EQ(pointer_action, mir_pointer_action_motion);

                    // Make sure there's movement on both axes
                    if (std::abs(pointer_relative_motion.dx.as_value()) > 0.0f &&
                        std::abs(pointer_relative_motion.dy.as_value()) > 0.0f)
                        count_diagonal += 1;
                });


        auto const step = std::chrono::milliseconds(1);

        auto down1 = down_event(key1);
        input_event_transformer.handle(*down1);
        clock.advance_by(step);

        auto down2 = down_event(key2);
        input_event_transformer.handle(*down2);
        clock.advance_by(step);

        // Should generate a bunch of motion events until we handle the up
        // event
        mt::spin_wait_for_condition_or_timeout(
            [&]
            {
                clock.advance_by(step);
                return false;
            },
            timeout,
            step);

        input_event_transformer.handle(*up_event(key1));
        input_event_transformer.handle(*up_event(key2));

        ASSERT_GT(count_diagonal, 0);
    }
}
