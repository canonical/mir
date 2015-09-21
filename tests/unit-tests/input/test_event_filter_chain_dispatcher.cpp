/*
 * Copyright © 2012 Canonical Ltd.
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

#include "src/server/input/event_filter_chain_dispatcher.h"
#include "src/server/input/null_input_dispatcher.h"
#include "mir/test/doubles/mock_event_filter.h"
#include "mir/test/doubles/mock_input_dispatcher.h"
#include "mir/events/event_builders.h"
#include "mir/events/event_private.h"

#include <androidfw/Input.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mtd = mir::test::doubles;

using namespace ::testing;

namespace
{
std::shared_ptr<mtd::MockEventFilter> mock_filter()
{
    return std::make_shared<mtd::MockEventFilter>();
}

struct EventFilterChainDispatcher : public ::testing::Test
{
    mir::EventUPtr const event = mir::events::make_event(MirInputDeviceId(),
        std::chrono::nanoseconds(0), 0, MirKeyboardAction(),
        xkb_keysym_t(), 0, MirInputEventModifiers());
};
}

TEST_F(EventFilterChainDispatcher, offers_events_to_filters)
{
    auto filter = mock_filter();
    mi::EventFilterChainDispatcher filter_chain({filter, filter},
        std::make_shared<mi::NullInputDispatcher>());
    
    // Filter will pass the event on twice
    EXPECT_CALL(*filter, handle(_)).Times(2).WillRepeatedly(Return(false));
    // So the filter chain should also reject the event
    EXPECT_FALSE(filter_chain.handle(*event));
}

TEST_F(EventFilterChainDispatcher, prepends_appends_filters)
{
    auto filter1 = mock_filter();
    auto filter2 = mock_filter();
    auto filter3 = mock_filter();

    mi::EventFilterChainDispatcher filter_chain({filter2},
        std::make_shared<mi::NullInputDispatcher>());
    
    filter_chain.append(filter3);
    filter_chain.prepend(filter1);

    {
        InSequence s;
        EXPECT_CALL(*filter1, handle(_)).WillOnce(Return(false));
        EXPECT_CALL(*filter2, handle(_)).WillOnce(Return(false));
        EXPECT_CALL(*filter3, handle(_)).WillOnce(Return(false));
    }

    filter_chain.handle(*event);
}

TEST_F(EventFilterChainDispatcher, accepting_event_halts_emission)
{
    auto filter = mock_filter();

    mi::EventFilterChainDispatcher filter_chain({filter, filter, filter}, std::make_shared<mi::NullInputDispatcher>());

    // First filter will reject, second will accept, third one should not be asked.
    {
        InSequence seq;
        EXPECT_CALL(*filter, handle(_)).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*filter, handle(_)).Times(1).WillOnce(Return(true));
    }

    filter_chain.handle(*event);
}

TEST_F(EventFilterChainDispatcher, does_not_own_event_filters)
{
    auto filter = mock_filter();

    mi::EventFilterChainDispatcher filter_chain({filter}, std::make_shared<mi::NullInputDispatcher>());
    EXPECT_CALL(*filter, handle(_)).Times(1).WillOnce(Return(true));
    EXPECT_TRUE(filter_chain.handle(*event));
    filter.reset();

    EXPECT_FALSE(filter_chain.handle(*event));
}

TEST_F(EventFilterChainDispatcher, forwards_start_and_stop)
{
    auto mock_next_dispatcher = std::make_shared<mtd::MockInputDispatcher>();
    mi::EventFilterChainDispatcher filter_chain({}, mock_next_dispatcher);

    InSequence seq;
    EXPECT_CALL(*mock_next_dispatcher, start()).Times(1);
    EXPECT_CALL(*mock_next_dispatcher, stop()).Times(1);

    filter_chain.start();
    filter_chain.stop();
}
