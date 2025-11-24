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

#include "src/server/input/keyboard_resync_dispatcher.h"

#include <mir/events/keyboard_event.h>

#include <mir/test/fake_shared.h>
#include <mir/test/event_matchers.h>
#include <mir/test/doubles/mock_input_dispatcher.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mev = mir::events;
namespace mt = mir::test;
namespace mtd = mt::doubles;

using namespace ::testing;

namespace
{
struct KeyboardResyncDispatcher : public testing::Test
{
    KeyboardResyncDispatcher()
        : dispatcher(mt::fake_shared(mock_next_dispatcher))
    {
    }

    StrictMock<mtd::MockInputDispatcher> mock_next_dispatcher;
    mi::KeyboardResyncDispatcher dispatcher;
};
}

TEST_F(KeyboardResyncDispatcher, forwards_events)
{
    EXPECT_CALL(mock_next_dispatcher, dispatch(mt::KeyDownEvent()));

    MirKeyboardEvent key_ev;
    key_ev.set_action(mir_keyboard_action_down);
    dispatcher.dispatch(mt::fake_shared(key_ev));
}

TEST_F(KeyboardResyncDispatcher, forwards_start_stop)
{
    EXPECT_CALL(mock_next_dispatcher, dispatch(_)).Times(AnyNumber());
    Expectation start_called = EXPECT_CALL(mock_next_dispatcher, start());
    EXPECT_CALL(mock_next_dispatcher, stop()).After(start_called);

    dispatcher.start();
    dispatcher.stop();
}

TEST_F(KeyboardResyncDispatcher, sends_resync_on_start)
{
    EXPECT_CALL(mock_next_dispatcher, dispatch(mt::KeybaordResyncEvent()));
    EXPECT_CALL(mock_next_dispatcher, start()).Times(AnyNumber());
    EXPECT_CALL(mock_next_dispatcher, stop()).Times(AnyNumber());

    dispatcher.start();
}

TEST_F(KeyboardResyncDispatcher, sends_resync_every_start)
{
    EXPECT_CALL(mock_next_dispatcher, dispatch(mt::KeybaordResyncEvent())).Times(3);
    EXPECT_CALL(mock_next_dispatcher, start()).Times(AnyNumber());
    EXPECT_CALL(mock_next_dispatcher, stop()).Times(AnyNumber());

    dispatcher.start();
    dispatcher.stop();
    dispatcher.start();
    dispatcher.stop();
    dispatcher.start();
}
