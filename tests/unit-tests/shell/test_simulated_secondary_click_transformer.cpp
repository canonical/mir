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

#include "mir/events/event.h"
#include "mir/events/event_builders.h"
#include "mir/events/input_event.h"
#include "mir/events/pointer_event.h"
#include "src/server/input/default_event_builder.h"
#include "src/server/shell/basic_simulated_secondary_click_transformer.h"
#include "transformer_common.h"

#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace ::testing;
using namespace std::chrono_literals;

struct TestSimulatedSecondaryClickTransformer : Test
{
    static auto constexpr virtual_event_builder_device_id = 1;

    TestSimulatedSecondaryClickTransformer() :
        main_loop{std::make_shared<mtd::QueuedAlarmStubMainLoop>(mt::fake_shared(clock))},
        transformer{
            std::make_shared<mir::shell::BasicSimulatedSecondaryClickTransformer>(main_loop),
        },
        real_event_builder{0, mt::fake_shared(clock)},
        virtual_event_builder{virtual_event_builder_device_id, mt::fake_shared(clock)},
        dispatch{[&](std::shared_ptr<MirEvent> const& event)
                 {
                     this->on_dispatch(event);
                 }}
    {
    }

    MOCK_METHOD1(on_dispatch, void(std::shared_ptr<MirEvent> const& event));

    mtd::AdvanceableClock clock;
    std::shared_ptr<mtd::QueuedAlarmStubMainLoop> const main_loop;
    std::shared_ptr<mir::shell::SimulatedSecondaryClickTransformer> const transformer;
    mi::DefaultEventBuilder real_event_builder, virtual_event_builder;
    std::function<void(std::shared_ptr<MirEvent> const& event)> dispatch;

    auto pointer_down_event() -> mir::EventUPtr
    {
        return real_event_builder.pointer_event(
            std::nullopt,
            mir_pointer_action_button_down,
            MirPointerButtons{mir_pointer_button_primary},
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f);
    }

    auto pointer_up_event() -> mir::EventUPtr
    {
        return real_event_builder.pointer_event(
            std::nullopt,
            mir_pointer_action_button_up,
            MirPointerButtons{0},
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f);
    }
};

struct TestDifferentReleaseTimings :
    public TestSimulatedSecondaryClickTransformer,
    public WithParamInterface<std::tuple<std::chrono::milliseconds, MirPointerButton>>
{
};

TEST_P(
    TestDifferentReleaseTimings,
    releasing_primary_button_before_secondary_click_delay_dispatches_a_primary_click)
{
    auto const [release_delay, expected_button] = GetParam();

    // If we cancel a simulated secondary click, we dispatch a synthesized primary click. The original primary click is
    // consumed and should not be forwarded to `dispatch`
    EXPECT_CALL(*this, on_dispatch(_))
        .WillOnce(
            [expected_button](auto const& event)
            {
                EXPECT_THAT(event->to_input()->to_pointer()->action(), Eq(mir_pointer_action_button_down));
                EXPECT_THAT(event->to_input()->to_pointer()->buttons() & expected_button, Ne(0));
            })
        .WillOnce(
            [expected_button](auto const& event)
            {
                EXPECT_THAT(event->to_input()->to_pointer()->action(), Eq(mir_pointer_action_button_up));
                EXPECT_THAT(event->to_input()->to_pointer()->buttons() & expected_button, Eq(0));
            });

    transformer->transform_input_event(dispatch, &virtual_event_builder, *pointer_down_event());
    clock.advance_by(release_delay);
    main_loop->call_queued();
    transformer->transform_input_event(dispatch, &virtual_event_builder, *pointer_up_event());
    main_loop->call_queued();
}

INSTANTIATE_TEST_SUITE_P(
    TestSimulatedSecondaryClickTransformer,
    TestDifferentReleaseTimings,
    ::Values(
        std::tuple(500ms, mir_pointer_button_primary),
        std::tuple(999ms, mir_pointer_button_primary),
        std::tuple(1001ms, mir_pointer_button_secondary),
        std::tuple(1500ms, mir_pointer_button_secondary)));

struct TestCallbacksParameters
{
    std::chrono::milliseconds hold_duration;
    // The secondary click succeeding implies that it wasn't cancelled.
    // Thus, the cancel callback will not be called.
    bool start_called{false}, secondary_click_called{false};
};

struct TestCallbacks :
    public TestSimulatedSecondaryClickTransformer,
    public WithParamInterface<TestCallbacksParameters>
{
};

TEST_P(TestCallbacks, releasing_left_pointer_button_at_different_times_calls_the_expected_callbacks)
{
    bool hold_start_called = false;
    bool hold_cancel_called = false;
    bool secondary_click_called = false;

    transformer->hold_start([&hold_start_called] { hold_start_called = true; });
    transformer->hold_cancel([&hold_cancel_called] { hold_cancel_called = true; });
    transformer->secondary_click([&secondary_click_called] { secondary_click_called = true; });

    auto const [release_delay, start_called, cancel_called] = GetParam();

    transformer->transform_input_event(dispatch, &virtual_event_builder, *pointer_down_event());
    clock.advance_by(release_delay);
    main_loop->call_queued();
    transformer->transform_input_event(dispatch, &virtual_event_builder, *pointer_up_event());
    main_loop->call_queued();

    EXPECT_THAT(hold_start_called, Eq(start_called));
    EXPECT_THAT(hold_cancel_called, Eq(cancel_called));
    EXPECT_THAT(secondary_click_called, Ne(cancel_called));
}

INSTANTIATE_TEST_SUITE_P(
    TestSimulatedSecondaryClickTransformer,
    TestCallbacks,
    ::Values(
        TestCallbacksParameters{500ms, true, true},
        TestCallbacksParameters{999ms, true, true},
        TestCallbacksParameters{1001ms, true, false},
        TestCallbacksParameters{1500ms, true, false}));
