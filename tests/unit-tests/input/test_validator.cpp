/*
 * Copyright© 2015 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/input/validator.h"

#include "mir/events/event_private.h"
#include "mir/events/event_builders.h"

#include "mir/test/fake_shared.h"
#include "mir/test/event_matchers.h"
#include "mir/test/doubles/mock_input_dispatcher.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mev = mir::events;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace ::testing;


namespace mir
{
struct MockEventSink
{
    MOCK_METHOD1(handle, void(MirEvent const&));
};
}

namespace
{

struct Validator : public ::testing::Test
{
    Validator()
        : rewriter([this](MirEvent const& ev) {input_sink.handle(ev);})
    {
    }

    mir::MockEventSink input_sink;
    mi::Validator rewriter;
};

void add_another_touch(mir::EventUPtr const& ev, MirTouchId id, MirTouchAction action)
{
    mev::add_touch(*ev, id, action, mir_touch_tooltype_finger,
                   0, 0, 0, 0, 0, 0);
}
    
mir::EventUPtr make_touch(MirTouchId id, MirTouchAction action)
{
    auto ev = mev::make_event(MirInputDeviceId(0), std::chrono::nanoseconds(0),
                              0, mir_input_event_modifier_none);
    add_another_touch(ev, id, action);
    return ev;
}

}

// We make a touch that represents two unseen touch ID's changing
// this way we expect the server to generate two seperate events to first
// report the down actions
TEST_F(Validator, missing_touch_downs_are_inserted)
{
    auto touch = make_touch(0, mir_touch_action_change);
    add_another_touch(touch, 1, mir_touch_action_change);

    auto inserted_down_id0 = make_touch(0, mir_touch_action_down);
    auto inserted_down_id1 = make_touch(0, mir_touch_action_change);
    add_another_touch(inserted_down_id1, 1, mir_touch_action_down);

    InSequence seq;
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*inserted_down_id0)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*inserted_down_id1)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch)));
    
    rewriter.validate_and_dispatch(*touch);
}

// In this case we first put two touches down, then we show an event which
// reports one of them changing without the others, in this case we
// must insert a touch up event for the ID which has gone missing.
TEST_F(Validator, missing_touch_releases_are_inserted)
{
    auto touch_1 = make_touch(0, mir_touch_action_down);
    auto touch_2 = make_touch(0, mir_touch_action_change);
    add_another_touch(touch_2, 1, mir_touch_action_down);
    auto touch_3 = make_touch(0, mir_touch_action_change);

    auto inserted_release = make_touch(0, mir_touch_action_change);
    add_another_touch(inserted_release, 1, mir_touch_action_up);

    InSequence seq;
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_1)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_2)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*inserted_release)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_3)));

    rewriter.validate_and_dispatch(*touch_1);
    rewriter.validate_and_dispatch(*touch_2);
    rewriter.validate_and_dispatch(*touch_3);
}

// Here we put 3 touches down (0, 1, 2) and then send an event which
// shows a change only for touch (2). This means we have to insert missing
// releases for both touches which have gone missing.
TEST_F(Validator, multiple_missing_releases_are_inserted)
{
    auto touch_1 = make_touch(0, mir_touch_action_down);
    auto touch_2 = make_touch(0, mir_touch_action_change);
    add_another_touch(touch_2, 1, mir_touch_action_down);
    auto touch_3 = make_touch(0, mir_touch_action_change);
    add_another_touch(touch_3, 1, mir_touch_action_change);
    add_another_touch(touch_3, 2, mir_touch_action_down);
    auto touch_4 = make_touch(2, mir_touch_action_change);

    auto inserted_release_id1 = make_touch(0, mir_touch_action_change);
    add_another_touch(inserted_release_id1, 1, mir_touch_action_up);
    add_another_touch(inserted_release_id1, 2, mir_touch_action_change);
    auto inserted_release_id0 = make_touch(0, mir_touch_action_up);
    add_another_touch(inserted_release_id0, 2, mir_touch_action_change);

    InSequence seq;
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_1)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_2)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_3)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*inserted_release_id1)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*inserted_release_id0)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_4)));

    rewriter.validate_and_dispatch(*touch_1);
    rewriter.validate_and_dispatch(*touch_2);
    rewriter.validate_and_dispatch(*touch_3);
    rewriter.validate_and_dispatch(*touch_4);
}

// In this case we put two touches down (0, 1) and then we show a touch
// with (0, 2). Here we expect point 1 to be released and then point 2 to be put down
// before reporting the touch with ids (0, 2)
TEST_F(Validator, missing_up_and_down_is_inserted)
{
    auto touch_1 = make_touch(0, mir_touch_action_down);
    auto touch_2 = make_touch(0, mir_touch_action_change);
    add_another_touch(touch_2, 1, mir_touch_action_down);
    auto touch_3 = make_touch(0, mir_touch_action_change);
    add_another_touch(touch_3, 1, mir_touch_action_change);
    auto touch_4 = make_touch(0, mir_touch_action_change);
    add_another_touch(touch_4, 2, mir_touch_action_change);

    auto inserted_release_id1 = make_touch(0, mir_touch_action_change);
    add_another_touch(inserted_release_id1, 1, mir_touch_action_up);
    auto inserted_down_id2 = make_touch(0, mir_touch_action_change);
    add_another_touch(inserted_down_id2, 2, mir_touch_action_down);

    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_1)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_2)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_3)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*inserted_release_id1)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*inserted_down_id2)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_4)));

    rewriter.validate_and_dispatch(*touch_1);
    rewriter.validate_and_dispatch(*touch_2);
    rewriter.validate_and_dispatch(*touch_3);
    rewriter.validate_and_dispatch(*touch_4);
}

// This is a variation of the previous test
TEST_F(Validator, missing_up_and_down_and_up_is_inserted_variation)
{
    auto touch_1 = make_touch(0, mir_touch_action_down);
    auto touch_2 = make_touch(0, mir_touch_action_change);
    add_another_touch(touch_2, 1, mir_touch_action_down);
    auto touch_3 = make_touch(1, mir_touch_action_change);
    add_another_touch(touch_3, 2, mir_touch_action_up);

    auto inserted_up_id0 = make_touch(0, mir_touch_action_up);
    add_another_touch(inserted_up_id0, 1, mir_touch_action_change);
    auto inserted_down_id2 = make_touch(1, mir_touch_action_change);
    add_another_touch(inserted_down_id2, 2, mir_touch_action_down);

    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_1)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_2)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*inserted_up_id0)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*inserted_down_id2)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_3)));

    rewriter.validate_and_dispatch(*touch_1);
    rewriter.validate_and_dispatch(*touch_2);
    rewriter.validate_and_dispatch(*touch_3);
}

// In this case we put two touches down (0, 1) and then we release touch 1 
// now as the next event we just show (0,1) changing as if point 1 had never
// been released. We ensure that a touch down for 1 is inserted before we
// show the change event.
TEST_F(Validator, down_is_inserted_before_released_touch_reappears)
{
    auto touch_1 = make_touch(0, mir_touch_action_down);
    auto touch_2 = make_touch(0, mir_touch_action_change);
    add_another_touch(touch_2, 1, mir_touch_action_down);
    auto touch_3 = make_touch(0, mir_touch_action_change);
    add_another_touch(touch_3, 1, mir_touch_action_change);
    auto touch_4 = make_touch(0, mir_touch_action_change);
    add_another_touch(touch_4, 1, mir_touch_action_up);
    auto const& touch_5 = touch_3;

    auto inserted_down_id1 = make_touch(0, mir_touch_action_change);
    add_another_touch(inserted_down_id1, 1, mir_touch_action_down);

    InSequence seq;
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_1)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_2)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_3)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_4)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*inserted_down_id1)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_5)));

    rewriter.validate_and_dispatch(*touch_1);
    rewriter.validate_and_dispatch(*touch_2);
    rewriter.validate_and_dispatch(*touch_3);
    rewriter.validate_and_dispatch(*touch_4);
    rewriter.validate_and_dispatch(*touch_5);
        
}

// Here we put a single point down and then show it dissapearing while another ID appears...similar
// to the missing up and down is inserted case but with only one touch point. We have to inject a release
// for the first touch point and a down for the new touch point.
TEST_F(Validator, up_and_down_inserted_when_id_changes)
{
    auto touch_1 = make_touch(0, mir_touch_action_down);
    auto touch_2 = make_touch(1, mir_touch_action_change);

    auto inserted_up_id0 = make_touch(0, mir_touch_action_up);
    auto inserted_down_id1 = make_touch(1, mir_touch_action_down);

    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_1)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*inserted_up_id0)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*inserted_down_id1)));
    EXPECT_CALL(input_sink, handle(mt::MirTouchEventMatches(*touch_2)));

    rewriter.validate_and_dispatch(*touch_1);
    rewriter.validate_and_dispatch(*touch_2);
}
