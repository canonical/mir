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

#include "src/server/input/idle_poking_dispatcher.h"

#include <mir/executor.h>
#include <mir/events/keyboard_event.h>
#include <mir/events/keyboard_resync_event.h>
#include <mir/events/pointer_event.h>
#include <mir/events/touch_event.h>

#include <mir/test/fake_shared.h>
#include <mir/test/event_matchers.h>
#include <mir/test/doubles/mock_input_dispatcher.h>
#include <mir/test/doubles/mock_idle_hub.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mev = mir::events;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace ::testing;

namespace
{
struct IdlePokingDispatcher : public testing::Test
{
    IdlePokingDispatcher()
        : dispatcher(mt::fake_shared(mock_next_dispatcher), mt::fake_shared(mock_hub))
    {
    }

    NiceMock<mtd::MockInputDispatcher> mock_next_dispatcher;
    NiceMock<mtd::MockIdleHub> mock_hub;
    mi::IdlePokingDispatcher dispatcher;
};

auto key_ev() -> std::shared_ptr<MirEvent>
{
    auto const key_ev = std::make_shared<MirKeyboardEvent>();
    key_ev->set_action(mir_keyboard_action_down);
    return key_ev;
}

auto resync_ev() -> std::shared_ptr<MirEvent>
{
    auto const resync_ev = std::make_shared<MirKeyboardResyncEvent>();
    return resync_ev;
}

auto pointer_ev() -> std::shared_ptr<MirEvent>
{
    auto const pointer_ev = std::make_shared<MirPointerEvent>();
    pointer_ev->set_action(mir_pointer_action_enter);
    return pointer_ev;
}

auto touch_ev() -> std::shared_ptr<MirEvent>
{
    auto const touch_ev = std::make_shared<MirTouchEvent>();
    touch_ev->set_pointer_count(1);
    touch_ev->set_action(0, mir_touch_action_down);
    touch_ev->set_position(0, {});
    return touch_ev;
}
}

TEST_F(IdlePokingDispatcher, forwards_events)
{
    InSequence seq;
    EXPECT_CALL(mock_next_dispatcher, dispatch(mt::KeyDownEvent()));
    EXPECT_CALL(mock_next_dispatcher, dispatch(mt::KeybaordResyncEvent()));
    EXPECT_CALL(mock_next_dispatcher, dispatch(mt::PointerEnterEvent()));
    EXPECT_CALL(mock_next_dispatcher, dispatch(mt::TouchEvent(0, 0)));

    dispatcher.dispatch(key_ev());
    dispatcher.dispatch(resync_ev());
    dispatcher.dispatch(pointer_ev());
    dispatcher.dispatch(touch_ev());
}

TEST_F(IdlePokingDispatcher, forwards_start_stop)
{
    InSequence seq;
    EXPECT_CALL(mock_next_dispatcher, start());
    EXPECT_CALL(mock_next_dispatcher, stop());

    dispatcher.start();
    dispatcher.stop();
}

TEST_F(IdlePokingDispatcher, pokes_idle_hub_on_input_event)
{
    EXPECT_CALL(mock_hub, poke()).Times(3);

    dispatcher.dispatch(key_ev());
    dispatcher.dispatch(pointer_ev());
    dispatcher.dispatch(touch_ev());
}

TEST_F(IdlePokingDispatcher, does_not_poke_idle_hub_on_resync_event)
{
    EXPECT_CALL(mock_hub, poke()).Times(0);

    dispatcher.dispatch(resync_ev());
}
