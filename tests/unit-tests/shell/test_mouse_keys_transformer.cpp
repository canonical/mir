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

struct TestOneAxisMovement : public TestMouseKeysTransformer, public WithParamInterface<mir::input::XkbSymkey>
{
};

TEST_P(TestOneAxisMovement, single_keyboard_event_to_single_pointer_motion_event)
{
    auto const timeout = std::chrono::milliseconds(20);
    auto const repeat_delay = std::chrono::milliseconds(2);
    auto const num_invocations = timeout / repeat_delay;

    EXPECT_CALL(mock_seat, dispatch_event(_))
        .Times(AtLeast(num_invocations / 2))
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

    auto button = GetParam();
    input_event_transformer.handle(*down_event(button));

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

INSTANTIATE_TEST_SUITE_P(
    TestMouseKeysTransformer, TestOneAxisMovement, ::Values(XKB_KEY_KP_2, XKB_KEY_KP_4, XKB_KEY_KP_6, XKB_KEY_KP_8));

struct TestDiagonalMovement :
    public TestMouseKeysTransformer,
    public WithParamInterface<std::pair<mir::input::XkbSymkey, mir::input::XkbSymkey>>
{
};

TEST_P(TestDiagonalMovement, multiple_keys_result_in_diagonal_movement)
{
    auto const [key1, key2] = GetParam();
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

INSTANTIATE_TEST_SUITE_P(
    TestMouseKeysTransformer,
    TestDiagonalMovement,
    ::Values(
        std::pair{XKB_KEY_KP_2, XKB_KEY_KP_4},
        std::pair{XKB_KEY_KP_2, XKB_KEY_KP_6},
        std::pair{XKB_KEY_KP_4, XKB_KEY_KP_8},
        std::pair{XKB_KEY_KP_4, XKB_KEY_KP_2},
        std::pair{XKB_KEY_KP_6, XKB_KEY_KP_8},
        std::pair{XKB_KEY_KP_6, XKB_KEY_KP_2},
        std::pair{XKB_KEY_KP_8, XKB_KEY_KP_4},
        std::pair{XKB_KEY_KP_8, XKB_KEY_KP_6}));

struct ClicksDispatchDownAndUpEvents :
    public TestMouseKeysTransformer,
    public WithParamInterface<std::pair<mir::input::XkbSymkey, MirPointerButton>>
{
};

TEST_P(ClicksDispatchDownAndUpEvents, clicks_dispatch_pointer_down_and_up_events)
{
    auto const [switching_button, expected_action] = GetParam();
    mt::Signal finished;

    auto const check_up = [&finished](auto const& event)
    {
        auto* const pointer_event = event->to_input()->to_pointer();
        auto const pointer_action = pointer_event->action();
        auto const pointer_buttons = pointer_event->buttons();

        ASSERT_EQ(pointer_action, mir_pointer_action_button_up);
        ASSERT_EQ(pointer_buttons, 0);
        finished.raise();
    };

    EXPECT_CALL(mock_seat, dispatch_event(_))
        .Times(3) // up (switching), down, up
        .WillOnce(
            [&check_up](std::shared_ptr<MirEvent> const& event)
            {
                check_up(event);
            })
        .WillOnce(
            [expected_action](std::shared_ptr<MirEvent> const& event)
            {
                auto* const pointer_event = event->to_input()->to_pointer();
                auto const pointer_action = pointer_event->action();
                auto const pointer_buttons = pointer_event->buttons();
                ASSERT_EQ(pointer_action, mir_pointer_action_button_down);
                ASSERT_EQ(pointer_buttons, expected_action);
            })
        .WillOnce(
            [&check_up](std::shared_ptr<MirEvent> const& event) mutable
            {
                check_up(event);
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

INSTANTIATE_TEST_SUITE_P(
    TestMouseKeysTransformer,
    ClicksDispatchDownAndUpEvents,
    ::Values(
        std::pair{XKB_KEY_KP_Divide, mir_pointer_button_primary},
        std::pair{XKB_KEY_KP_Multiply, mir_pointer_button_tertiary},
        std::pair{XKB_KEY_KP_Subtract, mir_pointer_button_secondary}));

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

struct TestAccelerationCurveParams
{
    struct
    {
        double quadratic, linear, constant;
    } curve;
    double expected_speed;
};
struct TestAccelerationCurve: public TestMouseKeysTransformer, WithParamInterface<TestAccelerationCurveParams>
{
};

TEST_P(TestAccelerationCurve, acceleration_curve_constants_evaluate_properly)
{
    auto const [curve, expected_speed_squared] = GetParam();
    EXPECT_CALL(mock_seat, dispatch_event(_))
        .Times(1)
        .WillOnce(
            [expected_speed_squared](std::shared_ptr<MirEvent> const& event)
            {
                ASSERT_FLOAT_EQ(std::sqrt(event->to_input()->to_pointer()->motion().length_squared()), expected_speed_squared);
            });

    // Don't want speed limits interfering with our test case
    transformer->max_speed(0, 0);

    transformer->acceleration_factors(curve.constant, curve.linear, curve.quadratic);
    input_event_transformer.handle(*down_event(XKB_KEY_KP_6));

    clock.advance_by(std::chrono::milliseconds(2));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    input_event_transformer.handle(*up_event(XKB_KEY_KP_6));
}

// Speed is computed as:
//              (
//              constant factor
//              + linear factor * time since motion start
//              + quadratic factor * time since motion started ^ 2
//              )  * time between alarm invocations
//
// time between alarm invocations is hardcoded as 2ms
// time since motion start is also 2ms
INSTANTIATE_TEST_SUITE_P(
    TestMouseKeysTransformer,
    TestAccelerationCurve,
    ::Values(
        TestAccelerationCurveParams{{0, 0, 0}, 0.0},
        TestAccelerationCurveParams{{0, 0, 500}, 1.0},
        TestAccelerationCurveParams{{0, 500, 0}, 500 * 0.002 * 0.002},
        TestAccelerationCurveParams{{500, 0, 0}, 500 * (0.002 * 0.002) * 0.002}));

using TestMaxSpeedParams = std::pair<mir::geometry::Displacement, mir::geometry::DisplacementF>;
struct TestMaxSpeed : public TestMouseKeysTransformer, WithParamInterface<TestMaxSpeedParams>
{
};

TEST_P(TestMaxSpeed, max_speed_caps_speed_properly)
{
    auto constexpr dt = 0.002;
    auto const [max_speed, expected_speed] = GetParam();

    EXPECT_CALL(mock_seat, dispatch_event(_))
        .Times(1)
        .WillOnce(
            [expected_speed](std::shared_ptr<MirEvent> const& event)
            {
                // Motion events are in pixels/alarm invocation
                // Which occurs about every 2ms
                auto const motion = event->to_input()->to_pointer()->motion();
                ASSERT_EQ(motion.dx, expected_speed.dx * dt);
                ASSERT_EQ(motion.dy, expected_speed.dy * dt);
            });

    // Immediately force clamping
    transformer->acceleration_factors(1000000, 1000000, 1000000);

    transformer->max_speed(max_speed.dx.as_value(), max_speed.dy.as_value());
    input_event_transformer.handle(*down_event(XKB_KEY_KP_6));
    input_event_transformer.handle(*down_event(XKB_KEY_KP_2));

    clock.advance_by(std::chrono::seconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    input_event_transformer.handle(*up_event(XKB_KEY_KP_6));
    input_event_transformer.handle(*up_event(XKB_KEY_KP_2));
}

INSTANTIATE_TEST_SUITE_P(
    TestMouseKeysTransformer,
    TestMaxSpeed,
    ::Values(
        TestMaxSpeedParams{{100, 1}, {100, 1}},
        TestMaxSpeedParams{{1, 100}, {1, 100}},
        TestMaxSpeedParams{{100, 100}, {100, 100}}));
