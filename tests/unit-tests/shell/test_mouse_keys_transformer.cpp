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
#include "mir/test/signal.h"
#include "src/server/input/default_input_device_hub.h"
#include "src/server/shell/mouse_keys_transformer.h"
#include "src/server/input/default_event_builder.h"

#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/mock_input_seat.h"
#include "mir/test/doubles/stub_main_loop.h"
#include "mir/test/doubles/stub_alarm.h"
#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/fake_shared.h"

#include "gmock/gmock.h"
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <pthread.h>
#include <list>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace mev = mir::events;
namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace ::testing;

namespace
{
/// When this alarm receives is scheduled or cancelled, it will notify the caller.
/// All of other methods are provided a "stub" implementation.
class StubNotifyingAlarm : public mtd::StubAlarm
{
public:
    explicit StubNotifyingAlarm(
        std::function<void(StubNotifyingAlarm const*)> const& on_rescheduled,
        std::function<void(StubNotifyingAlarm const*)> const& on_cancelled)
        : on_rescheduled(on_rescheduled),
          on_cancelled(on_cancelled) {}

    ~StubNotifyingAlarm() override
    {
        on_cancelled(this);
    }

    bool reschedule_in(std::chrono::milliseconds) override
    {
        on_rescheduled(this);
        return true;
    }

    bool reschedule_for(mir::time::Timestamp) override
    {
        on_rescheduled(this);
        return true;
    }

private:
    std::function<void(StubNotifyingAlarm const*)> on_rescheduled;
    std::function<void(StubNotifyingAlarm const*)> on_cancelled;
};

/// This MainLoop is a stub aside from the code that handles the creation of alarms.
/// When an alarm is scheduled, this class will add it to a list of functions that
/// need to be called. When an alarm is removed, this class will remove all functions
/// queued on that alarm from the list so that they are not called.
class QueuedAlarmStubMainLoop : public mtd::StubMainLoop
{
public:
    std::unique_ptr<mir::time::Alarm> create_alarm(std::function<void()> const& f) override
    {
        return std::make_unique<StubNotifyingAlarm>(
            [this, f=f](StubNotifyingAlarm const* alarm)
            {
                pending.push_back(std::make_shared<AlarmData>(alarm, f));
            },
            [this](StubNotifyingAlarm const* alarm)
            {
                pending.erase(std::remove_if(pending.begin(), pending.end(), [alarm](std::shared_ptr<AlarmData> const& data)
                {
                    return data->alarm == alarm;
                }), pending.end());
            }
        );
    }

    bool call_queued()
    {
        if (pending.empty())
            return false;

        pending.front()->call();
        pending.pop_front();
        return true;
    }

private:
    struct AlarmData
    {
        StubNotifyingAlarm const* alarm;
        std::function<void()> call;
    };
    std::list<std::shared_ptr<AlarmData>> pending;
};
}

struct TestMouseKeysTransformer : testing::Test
{
    TestMouseKeysTransformer() :
        main_loop{std::make_shared<QueuedAlarmStubMainLoop>()},
        transformer{
            std::make_shared<mi::BasicMouseKeysTransformer>(main_loop, mt::fake_shared(clock)),
        },
        default_event_builder(0, mt::fake_shared(clock)),
        dispatch([&](std::shared_ptr<MirEvent> const& event)
        {
            this->on_dispatch(event);
        })
    {
    }

    mtd::AdvanceableClock clock;
    std::shared_ptr<QueuedAlarmStubMainLoop> const main_loop;
    std::shared_ptr<mi::MouseKeysTransformer> const transformer;
    mi::DefaultEventBuilder default_event_builder;
    std::function<void(std::shared_ptr<MirEvent> const& event)> dispatch;

    MOCK_METHOD1(on_dispatch, void(std::shared_ptr<MirEvent> const& event));

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

namespace
{
auto data_from_pointer_event(std::shared_ptr<MirEvent const> const& event)
    -> std::tuple<MirPointerAction, mir::geometry::DisplacementF, MirPointerButtons>
{
    auto* const pointer_event = event->to_input()->to_pointer();
    return {pointer_event->action(), pointer_event->motion(), pointer_event->buttons()};
}
}

struct TestOneAxisMovement : public TestMouseKeysTransformer, public WithParamInterface<mir::input::XkbSymkey>
{
};

TEST_P(TestOneAxisMovement, single_keyboard_event_to_single_pointer_motion_event)
{
    EXPECT_CALL(*this, on_dispatch(_))
        .Times(3)
        .WillRepeatedly(
            [](std::shared_ptr<MirEvent> const& event)
            {
                auto const [pointer_action, pointer_relative_motion, _] = data_from_pointer_event(event);

                EXPECT_EQ(pointer_action, mir_pointer_action_motion);

                // Make sure the movement is on one axis
                if (pointer_relative_motion.dx.as_value() != 0.0f)
                {
                    EXPECT_EQ(pointer_relative_motion.dy.as_value(), 0.0f);
                }

                if (pointer_relative_motion.dy.as_value() != 0.0f)
                {
                    EXPECT_EQ(pointer_relative_motion.dx.as_value(), 0.0f);
                }
            });

    auto const button = GetParam();
    transformer->transform_input_event(dispatch, &default_event_builder, *down_event(button));
    main_loop->call_queued();
    main_loop->call_queued();
    main_loop->call_queued();
    transformer->transform_input_event(dispatch, &default_event_builder, *up_event(button));
}

INSTANTIATE_TEST_SUITE_P(
    TestMouseKeysTransformer, TestOneAxisMovement, ::Values(XKB_KEY_KP_2, XKB_KEY_KP_4, XKB_KEY_KP_6, XKB_KEY_KP_8));

struct TestTwoKeyMovement :
    public TestMouseKeysTransformer,
    public WithParamInterface<std::pair<mir::input::XkbSymkey, mir::input::XkbSymkey>>
{
    void press_keys()
    {
        auto const [key1, key2] = GetParam();

        for (auto key : {key1, key2})
        {
            auto down = down_event(key);
            transformer->transform_input_event(dispatch, &default_event_builder, *down);
        }
    }

    void release_keys()
    {
        auto const [key1, key2] = GetParam();

        for (auto key : {key1, key2})
        {
            auto up = up_event(key);
            transformer->transform_input_event(dispatch, &default_event_builder, *up);
        }
    }
};

struct TestDiagonalMovement: public TestTwoKeyMovement
{
};

TEST_P(TestDiagonalMovement, multiple_keys_result_in_diagonal_movement)
{
    auto count_diagonal = 0;
    InSequence seq;
    EXPECT_CALL(*this, on_dispatch(_))
        .Times(3)
        .WillOnce(Return()) // Skip the first event where one button is pressed
        .WillRepeatedly(
            [&](std::shared_ptr<MirEvent> const& event)
            {
                auto const [pointer_action, pointer_relative_motion, _] = data_from_pointer_event(event);

                ASSERT_EQ(pointer_action, mir_pointer_action_motion);

                // Make sure there's movement on both axes
                if (std::abs(pointer_relative_motion.dx.as_value()) > 0.0f &&
                    std::abs(pointer_relative_motion.dy.as_value()) > 0.0f)
                    count_diagonal += 1;
            });

    press_keys();
    main_loop->call_queued();
    main_loop->call_queued();
    main_loop->call_queued();
    ASSERT_GT(count_diagonal, 0);
    release_keys();
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

struct TestOppositeDiagonalMovement :
    TestTwoKeyMovement
{
};

TEST_P(TestOppositeDiagonalMovement, opposite_keys_result_in_no_movement)
{
    InSequence seq;
    EXPECT_CALL(*this, on_dispatch(_))
        .Times(3)
        .WillOnce(Return())
        .WillRepeatedly(
            [&](std::shared_ptr<MirEvent> const& event)
            {
                auto const [pointer_action, pointer_relative_motion, _] = data_from_pointer_event(event);

                ASSERT_EQ(pointer_action, mir_pointer_action_motion);

                // Make sure there is no movement
                EXPECT_FLOAT_EQ(pointer_relative_motion.length_squared(), 0.0);
            });

    press_keys();
    main_loop->call_queued();
    main_loop->call_queued();
    main_loop->call_queued();
    release_keys();
}

INSTANTIATE_TEST_SUITE_P(
    TestMouseKeysTransformer,
    TestOppositeDiagonalMovement,
    ::Values(std::pair{XKB_KEY_KP_2, XKB_KEY_KP_8}, std::pair{XKB_KEY_KP_4, XKB_KEY_KP_6}));

TEST_F(TestMouseKeysTransformer, pressing_all_movement_buttons_generates_no_motion)
{
    InSequence seq;
    EXPECT_CALL(*this, on_dispatch(_))
        .Times(3)
        .WillOnce(Return())
        .WillOnce(Return())
        .WillOnce(Return())
        .WillRepeatedly(
            [&](std::shared_ptr<MirEvent> const& event)
            {
                auto const [pointer_action, pointer_relative_motion, _] = data_from_pointer_event(event);

                ASSERT_EQ(pointer_action, mir_pointer_action_motion);

                // Make sure there is no movement
                EXPECT_FLOAT_EQ(pointer_relative_motion.length_squared(), 0.0);
            });

    auto const all_movement_keys = {XKB_KEY_KP_2, XKB_KEY_KP_4, XKB_KEY_KP_6, XKB_KEY_KP_8};
    for (auto key : all_movement_keys)
    {
        auto down = down_event(key);
        transformer->transform_input_event(dispatch, &default_event_builder, *down);
    }

    main_loop->call_queued();
    main_loop->call_queued();
    main_loop->call_queued();

    for (auto key : all_movement_keys)
    {
        auto up = up_event(key);
        transformer->transform_input_event(dispatch, &default_event_builder, *up);
    }
}

struct ClicksDispatchDownAndUpEvents :
    public TestMouseKeysTransformer,
    public WithParamInterface<std::pair<mir::input::XkbSymkey, MirPointerButton>>
{
};

TEST_P(ClicksDispatchDownAndUpEvents, clicks_dispatch_pointer_down_and_up_events)
{
    auto const [switching_button, expected_action] = GetParam();

    auto const check_up = [](auto const& event)
    {
        auto const [pointer_action, _, pointer_buttons] = data_from_pointer_event(event);

        EXPECT_EQ(pointer_action, mir_pointer_action_button_up);
        EXPECT_EQ(pointer_buttons, 0);
    };

    InSequence seq;
    EXPECT_CALL(*this, on_dispatch(_))
        .Times(3) // up (switching), down, up
        .WillOnce(
            [&check_up](std::shared_ptr<MirEvent> const& event)
            {
                check_up(event);
            })
        .WillOnce(
            [expected_action](std::shared_ptr<MirEvent> const& event)
            {
                auto const [pointer_action, _, pointer_buttons] = data_from_pointer_event(event);
                EXPECT_EQ(pointer_action, mir_pointer_action_button_down);
                EXPECT_EQ(pointer_buttons, expected_action);
            })
        .WillOnce(
            [&check_up](std::shared_ptr<MirEvent> const& event) mutable
            {
                check_up(event);
            });

    transformer->transform_input_event(dispatch, &default_event_builder, *up_event(switching_button));
    transformer->transform_input_event(dispatch, &default_event_builder, *up_event(XKB_KEY_KP_5));

    main_loop->call_queued();
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
    auto const expect_down =
        [](auto const& event)
    {
        auto const [pointer_action, _, __] = data_from_pointer_event(event);
        EXPECT_EQ(pointer_action, mir_pointer_action_button_down);
    };

    auto const expect_up = [](auto const& event)
    {
        auto const [pointer_action, _, pointer_buttons] = data_from_pointer_event(event);
        EXPECT_EQ(pointer_action, mir_pointer_action_button_up);
        EXPECT_EQ(pointer_buttons, 0);
    };

    InSequence seq;
    EXPECT_CALL(*this, on_dispatch(_))
        .Times(4)
        .WillOnce(expect_down)
        .WillOnce(expect_up)
        .WillOnce(expect_down)
        .WillOnce(expect_up);

    transformer->transform_input_event(dispatch, &default_event_builder, *up_event(XKB_KEY_KP_Add));
    main_loop->call_queued();
}

TEST_F(TestMouseKeysTransformer, drag_start_and_end_dispatch_down_and_up_events)
{
    InSequence seq;
    EXPECT_CALL(*this, on_dispatch(_))
        .Times(2)
        .WillOnce(
            [](auto const& event)
            {
                auto const [pointer_action, _, __] = data_from_pointer_event(event);
                EXPECT_EQ(pointer_action, mir_pointer_action_button_down);
            })
        .WillOnce(
            [](auto const& event)
            {
                auto const [pointer_action, _, pointer_buttons] = data_from_pointer_event(event);
                EXPECT_EQ(pointer_action, mir_pointer_action_button_up);
                EXPECT_EQ(pointer_buttons, 0);
            });

    transformer->transform_input_event(dispatch, &default_event_builder, *up_event(XKB_KEY_KP_0));
    transformer->transform_input_event(dispatch, &default_event_builder, *up_event(XKB_KEY_KP_Decimal));
    main_loop->call_queued();
}

TEST_F(TestMouseKeysTransformer, receiving_a_key_not_in_keymap_doesnt_dispatch_event)
{
    EXPECT_CALL(*this, on_dispatch(_)).Times(0);
    transformer->keymap(mir::input::MouseKeysKeymap{});

    using enum mir::input::MouseKeysKeymap::Action;
    auto const test_keymap = mir::input::MouseKeysKeymap{
        {XKB_KEY_w, move_up},
        {XKB_KEY_s, move_down},
        {XKB_KEY_a, move_left},
        {XKB_KEY_d, move_right},
        {XKB_KEY_y, button_primary},
        {XKB_KEY_u, button_tertiary},
        {XKB_KEY_i, button_secondary},
        {XKB_KEY_j, click},
    };
    test_keymap.for_each_key_action_pair(
        [&](auto key, auto)
        {
            transformer->transform_input_event(dispatch, &default_event_builder, *down_event(key));
            main_loop->call_queued();
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
    EXPECT_CALL(*this, on_dispatch(_))
        .Times(1)
        .WillOnce(
            [expected_speed_squared](std::shared_ptr<MirEvent> const& event)
            {
                auto [_, pointer_motion, __] = data_from_pointer_event(event);
                ASSERT_FLOAT_EQ(std::sqrt(pointer_motion.length_squared()), expected_speed_squared);
            });

    // Don't want speed limits interfering with our test case
    transformer->max_speed(0, 0);

    transformer->acceleration_factors(curve.constant, curve.linear, curve.quadratic);
    transformer->transform_input_event(dispatch, &default_event_builder, *down_event(XKB_KEY_KP_6));

    clock.advance_by(std::chrono::milliseconds(2));
    main_loop->call_queued();

    transformer->transform_input_event(dispatch, &default_event_builder, *up_event(XKB_KEY_KP_6));
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

    EXPECT_CALL(*this, on_dispatch(_))
        .Times(1)
        .WillOnce(
            [expected_speed](std::shared_ptr<MirEvent> const& event)
            {
                // Motion events are in pixels/alarm invocation
                // Which occurs about every 2ms
                auto const motion = event->to_input()->to_pointer()->motion();
                EXPECT_EQ(motion.dx, expected_speed.dx * dt);
                EXPECT_EQ(motion.dy, expected_speed.dy * dt);
            });

    // Immediately force clamping
    transformer->acceleration_factors(1000000, 1000000, 1000000);

    transformer->max_speed(max_speed.dx.as_value(), max_speed.dy.as_value());
    transformer->transform_input_event(dispatch, &default_event_builder, *down_event(XKB_KEY_KP_6));
    transformer->transform_input_event(dispatch, &default_event_builder, *down_event(XKB_KEY_KP_2));

    clock.advance_by(std::chrono::seconds(1));
    main_loop->call_queued();

    transformer->transform_input_event(dispatch, &default_event_builder, *up_event(XKB_KEY_KP_6));
    transformer->transform_input_event(dispatch, &default_event_builder, *up_event(XKB_KEY_KP_2));
}

INSTANTIATE_TEST_SUITE_P(
    TestMouseKeysTransformer,
    TestMaxSpeed,
    ::Values(
        TestMaxSpeedParams{{100, 1}, {100, 1}},
        TestMaxSpeedParams{{1, 100}, {1, 100}},
        TestMaxSpeedParams{{100, 100}, {100, 100}}));
