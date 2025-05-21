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

#include "src/server/input/default_event_builder.h"
#include "src/server/shell/basic_hover_click_transformer.h"

#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/queued_alarm_stub_main_loop.h"
#include "mir/test/fake_shared.h"
#include "mir/test/signal.h"

#include "mir/geometry/displacement.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;
using namespace std::chrono_literals;

namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace 
{
    constexpr static auto test_hover_delay = 500ms;
    constexpr static auto test_cancel_displacement_threshold = 10;
    constexpr static auto grace_period_percentage = 0.1f;
}

struct TestHoverClickTransformer : Test
{
    TestHoverClickTransformer()
    {
        transformer->hover_duration(test_hover_delay);
        transformer->cancel_displacement_threshold(test_cancel_displacement_threshold);
    }

    mtd::AdvanceableClock clock;
    mtd::QueuedAlarmStubMainLoop main_loop{mt::fake_shared(clock)};
    std::shared_ptr<mir::shell::HoverClickTransformer> const transformer{std::make_shared<mir::shell::BasicHoverClickTransformer>(mt::fake_shared(main_loop))};
    mir::input::DefaultEventBuilder event_builder{0, mt::fake_shared(clock)};
    std::function<void(std::shared_ptr<MirEvent> const&)> dispatcher{[](auto) { }};

    mir::geometry::PointF pointer_location{0, 0};

    mir::EventUPtr motion_event(mir::geometry::DisplacementF displacement)
    {
        pointer_location += displacement;
        return event_builder.pointer_event(
            std::nullopt,
            mir_pointer_action_motion,
            0,
            pointer_location,
            displacement,
            mir_pointer_axis_source_none,
            mir::events::ScrollAxisV1H{},
            mir::events::ScrollAxisV1V{});
    }
};

struct TestStartCallback: public TestHoverClickTransformer, public WithParamInterface<float>
{
};

TEST_P(TestStartCallback, start_event_called_only_after_grace_period)
{
    // How long to wait relative to the hover click delay 
    auto const delay_percentage = GetParam(); 
    // 0.1 is the hardcoded grace period in the basic transformer
    auto const expect_start_called = delay_percentage >= grace_period_percentage;

    mt::Signal hover_start_signal;
    transformer->on_hover_start([&hover_start_signal] { hover_start_signal.raise(); });

    auto initial_pointer_move_event = motion_event(mir::geometry::DisplacementF{10, 0});
    transformer->transform_input_event(dispatcher, &event_builder, *initial_pointer_move_event);
    
    // Hover click is active now
    // If we move before the grace period, no start callback will be called
    // Otherwise, a start event will be called.
    // If we move after that, a cancel event will be calld.

    clock.advance_by(std::chrono::duration_cast<std::chrono::milliseconds>(delay_percentage * test_hover_delay));

    main_loop.call_queued();
    EXPECT_THAT(hover_start_signal.raised(), Eq(expect_start_called));
}

INSTANTIATE_TEST_SUITE_P(
    TestHoverClickTransformer, TestStartCallback, ::Values(0.99 * grace_period_percentage, grace_period_percentage));

struct TestCancelCallback: public TestHoverClickTransformer, public WithParamInterface<float>
{
};

TEST_P(TestCancelCallback, cancel_called_if_pointer_moves_before_hover_delay)
{
    // How long to wait relative to the hover click delay 
    auto const delay_percentage = GetParam(); 

    mt::Signal hover_cancel_signal;
    transformer->on_hover_cancel([&hover_cancel_signal]{ hover_cancel_signal.raise(); });

    mt::Signal click_dipatched_signal;
    transformer->on_click_dispatched([&click_dipatched_signal]{ click_dipatched_signal.raise(); });

    // Invoke hover click
    auto initial_pointer_move_event = motion_event(mir::geometry::DisplacementF{10, 0});
    transformer->transform_input_event(dispatcher, &event_builder, *initial_pointer_move_event);

    clock.advance_by(std::chrono::duration_cast<std::chrono::milliseconds>(grace_period_percentage * test_hover_delay));
    main_loop.call_queued();

    // Have to split into two because one alarm kicks off the other
    // By this point, the "real" hover click alarm should be enqueued

    auto const remaining_percentage = std::clamp(delay_percentage * (1.0 - grace_period_percentage), 0.0, 1.0);
    clock.advance_by(std::chrono::duration_cast<std::chrono::milliseconds>(remaining_percentage * test_hover_delay));
    main_loop.call_queued();

    auto potentially_cancelling_motion_event =
        motion_event(mir::geometry::DisplacementF{test_cancel_displacement_threshold + 1, 0});
    transformer->transform_input_event(dispatcher, &event_builder, *potentially_cancelling_motion_event);

    main_loop.call_queued();

    EXPECT_THAT(hover_cancel_signal.raised(), Eq(delay_percentage < 1.0));
    EXPECT_THAT(click_dipatched_signal.raised(), Eq(delay_percentage >= 1.0));
}

INSTANTIATE_TEST_SUITE_P(TestHoverClickTransformer, TestCancelCallback, ::Values(0.95, 1.01, 1.1));
