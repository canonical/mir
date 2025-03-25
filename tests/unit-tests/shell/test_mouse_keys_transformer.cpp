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
#include "mir/events/pointer_event.h"
#include "mir/geometry/forward.h"
#include "mir/glib_main_loop.h"
#include "mir/input/input_event_transformer.h"
#include "mir/main_loop.h"
#include "mir/test/auto_unblock_thread.h"
#include "mir/test/signal.h"
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
#include <gtest/gtest.h>
#include <memory>
#include <pthread.h>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace mev = mir::events;
namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace ::testing;

struct TestMouseKeysTransformer : testing::Test
{
    using AccelerationParameters = mir::input::BasicMouseKeysTransformer::AccelerationParameters;

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
        transformer{
            std::make_shared<mi::BasicMouseKeysTransformer>(main_loop, mt::fake_shared(clock)),
        },
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

    enum class State
    {
        waiting_for_down,
        waiting_for_up
    };

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
                    if (pointer_relative_motion.dx.as_value() != 0.0f)
                    {
                        ASSERT_EQ(pointer_relative_motion.dy.as_value(), 0.0f);
                    }

                    if (pointer_relative_motion.dy.as_value() != 0.0f)
                    {
                        ASSERT_EQ(pointer_relative_motion.dx.as_value(), 0.0f);
                    }
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

    for (auto const& [key1, key2] : combinations)
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

TEST_F(TestMouseKeysTransformer, clicks_dispatch_pointer_down_and_up_events)
{
    for (auto switching_button : {XKB_KEY_KP_Divide, XKB_KEY_KP_Multiply, XKB_KEY_KP_Subtract})
    {
        mt::Signal finished;
        auto state = State::waiting_for_up;

        EXPECT_CALL(mock_seat, dispatch_event(_))
            .Times(3) // up (switching), down, up
            .WillRepeatedly(
                [&state , &finished, switching_button](
                    std::shared_ptr<MirEvent> const& event) mutable
                {
                    auto* const pointer_event = event->to_input()->to_pointer();

                    auto const pointer_action = pointer_event->action();
                    auto const pointer_buttons = pointer_event->buttons();
                    switch (state)
                    {
                    case State::waiting_for_down:

                        ASSERT_EQ(pointer_action, mir_pointer_action_button_down);
                        switch (switching_button)
                        {
                        case XKB_KEY_KP_Divide:
                            ASSERT_EQ(pointer_buttons, mir_pointer_button_primary);
                            break;
                        case XKB_KEY_KP_Multiply:
                            ASSERT_EQ(pointer_buttons, mir_pointer_button_tertiary);
                            break;
                        case XKB_KEY_KP_Subtract:
                            ASSERT_EQ(pointer_buttons, mir_pointer_button_secondary);
                            break;
                        }

                        state = State::waiting_for_up;
                        break;
                    case State::waiting_for_up:
                        ASSERT_EQ(pointer_action, mir_pointer_action_button_up);
                        ASSERT_EQ(pointer_buttons, 0);
                        state = State::waiting_for_down;
                        finished.raise();
                        break;
                    }
                });

        input_event_transformer.handle(*up_event(switching_button));
        input_event_transformer.handle(*up_event(XKB_KEY_KP_5));

        // Advance so that the up portion of the click can be processed
        clock.advance_by(std::chrono::milliseconds(50));

        auto constexpr num_expected_up_events = 2;
        for(auto i = 0; i < num_expected_up_events; i++)
        {
            if(finished.wait_for(std::chrono::milliseconds(10)))
                finished.reset();
        }
    }
}

TEST_F(TestMouseKeysTransformer, double_click_dispatch_four_events)
{
    auto state = State::waiting_for_down;
    mt::Signal finished;

    EXPECT_CALL(mock_seat, dispatch_event(_))
        .Times(4) // down, up, down, up
        .WillRepeatedly(
            [&state, &finished](std::shared_ptr<MirEvent> const& event)
            {
                auto* const pointer_event = event->to_input()->to_pointer();

                auto const pointer_action = pointer_event->action();
                auto const pointer_buttons = pointer_event->buttons();
                switch (state)
                {
                case State::waiting_for_down:
                    ASSERT_EQ(pointer_action, mir_pointer_action_button_down);
                    state = State::waiting_for_up;
                    break;
                case State::waiting_for_up:
                    ASSERT_EQ(pointer_action, mir_pointer_action_button_up);
                    ASSERT_EQ(pointer_buttons, 0);
                    state = State::waiting_for_down;
                    finished.raise();
                    break;
                }
            });

    input_event_transformer.handle(*up_event(XKB_KEY_KP_Add));

    for (auto i = 0; i < 2; i++)
    {
        clock.advance_by(std::chrono::milliseconds(100));
        if(finished.wait_for(std::chrono::milliseconds(10)))
            finished.reset();
    }
}

TEST_F(TestMouseKeysTransformer, drag_start_and_end_dispatch_down_and_up_events)
{
    auto state = State::waiting_for_down;
    mt::Signal finished;

    EXPECT_CALL(mock_seat, dispatch_event(_))
        .Times(2) // down, up
        .WillRepeatedly(
            [&state, &finished](std::shared_ptr<MirEvent> const& event)
            {
                auto* const pointer_event = event->to_input()->to_pointer();

                auto const pointer_action = pointer_event->action();
                auto const pointer_buttons = pointer_event->buttons();
                switch (state)
                {
                case State::waiting_for_down:
                    ASSERT_EQ(pointer_action, mir_pointer_action_button_down);
                    state = State::waiting_for_up;
                    break;
                case State::waiting_for_up:
                    ASSERT_EQ(pointer_action, mir_pointer_action_button_up);
                    ASSERT_EQ(pointer_buttons, 0);
                    state = State::waiting_for_down;
                    finished.raise();
                    break;
                }
            });

    input_event_transformer.handle(*up_event(XKB_KEY_KP_0));
    input_event_transformer.handle(*up_event(XKB_KEY_KP_Decimal));

    finished.wait_for(std::chrono::milliseconds(10));
}

TEST_F(TestMouseKeysTransformer, receiving_a_key_not_in_keymap_doesnt_dispatch_event)
{
    EXPECT_CALL(mock_seat, dispatch_event(_)).Times(0);
    transformer->keymap(mir::input::MouseKeysKeymap{});

    mir::input::BasicMouseKeysTransformer::default_keymap.for_each_key_action_pair(
        [&](auto key, auto)
        {
            input_event_transformer.handle(*down_event(key));
        });
}

TEST_F(TestMouseKeysTransformer, acceleration_curve_constants_evaluate_properly)
{
    EXPECT_CALL(mock_seat, dispatch_event(_))
        .Times(4)
        .WillOnce(
            [](std::shared_ptr<MirEvent> const& event)
            {
                ASSERT_EQ(event->to_input()->to_pointer()->motion().length_squared(), 0.0);
            })
        .WillOnce(
            [](std::shared_ptr<MirEvent> const& event)
            {
                ASSERT_EQ(event->to_input()->to_pointer()->motion().length_squared(), 1);
            })
        .WillOnce(
            [](std::shared_ptr<MirEvent> const& event)
            {
                // Speed is computed as:
                //              (
                //              constant factor
                //              + linear factor * time since motion start
                //              + quadratic factor * time since motion started ^ 2
                //              )  * time between alarm invocations
                //
                // time between alarm invocations is hardcoded as 2ms
                // time since motion start is also 2ms
                // So the speed should be the linear factor (500) * 2ms * 2ms
                EXPECT_FLOAT_EQ(std::sqrt(event->to_input()->to_pointer()->motion().length_squared()), (500 * 0.002 * 0.002));
            })
        .WillOnce(
            [](std::shared_ptr<MirEvent> const& event)
            {
                EXPECT_FLOAT_EQ(
                    std::sqrt(event->to_input()->to_pointer()->motion().length_squared()),
                    (500 * (0.002 * 0.002) * 0.002));
            });

    auto const parameters = {
        AccelerationParameters{0, 0, 0}, // a, b, c
        {0, 0, 500},
        {0, 500, 0},
        {500, 0, 0},
    };

    for(auto const& param: parameters)
    {
        transformer->acceleration_factors(param.c, param.b, param.a);
        input_event_transformer.handle(*down_event(XKB_KEY_KP_6));

        clock.advance_by(std::chrono::milliseconds(2));
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        input_event_transformer.handle(*up_event(XKB_KEY_KP_6));
    }
}

TEST_F(TestMouseKeysTransformer, max_speed_caps_speed_properly)
{
    auto constexpr dt = 0.002;

    EXPECT_CALL(mock_seat, dispatch_event(_))
        .Times(3)
        .WillOnce(
            [](std::shared_ptr<MirEvent> const& event)
            {
                // Motion events are in pixels/alarm invocation
                // Which occurs about every 2ms
                auto const motion = event->to_input()->to_pointer()->motion();
                ASSERT_EQ(motion.dx, mir::geometry::DeltaXF{100} * dt);
                ASSERT_EQ(motion.dy, mir::geometry::DeltaYF{1} * dt);
            })
        .WillOnce(
            [](std::shared_ptr<MirEvent> const& event)
            {
                auto const motion = event->to_input()->to_pointer()->motion();
                ASSERT_EQ(motion.dx, mir::geometry::DeltaXF{1} * dt);
                ASSERT_EQ(motion.dy, mir::geometry::DeltaYF{100} * dt);
            })
        .WillOnce(
            [](std::shared_ptr<MirEvent> const& event)
            {
                auto const motion = event->to_input()->to_pointer()->motion();
                ASSERT_EQ(motion.dx, mir::geometry::DeltaXF{100} * dt);
                ASSERT_EQ(motion.dy, mir::geometry::DeltaYF{100} * dt);
            });

    // Immediately force clamping
    transformer->acceleration_factors(1000000, 1000000, 1000000);

    // Limits in pixels/second
    auto const parameters = {
        std::pair{100, 1},
        {1, 100},
        {100, 100},
    };

    for(auto const& param: parameters)
    {
        transformer->max_speed(param.first, param.second);
        input_event_transformer.handle(*down_event(XKB_KEY_KP_6));
        input_event_transformer.handle(*down_event(XKB_KEY_KP_2));

        clock.advance_by(std::chrono::seconds(1));
        std::this_thread::sleep_for(std::chrono::milliseconds(2));

        input_event_transformer.handle(*up_event(XKB_KEY_KP_6));
        input_event_transformer.handle(*up_event(XKB_KEY_KP_2));
    }
}
