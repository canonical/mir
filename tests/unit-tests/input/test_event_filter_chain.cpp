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

#include "src/server/input/event_filter_chain.h"
#include "mir_test_doubles/mock_event_filter.h"
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

struct EventFilterChain : public ::testing::Test
{
    mir::EventUPtr const event = mir::events::make_event(MirInputDeviceId(), std::chrono::nanoseconds(0),
        MirKeyboardAction(), xkb_keysym_t(), 0, MirInputEventModifiers());
};
}

TEST_F(EventFilterChain, offers_events_to_filters)
{
    auto filter = mock_filter();
    mi::EventFilterChain filter_chain{filter, filter};
    
    // Filter will pass the event on twice
    EXPECT_CALL(*filter, handle(_)).Times(2).WillRepeatedly(Return(false));
    // So the filter chain should also reject the event
    EXPECT_FALSE(filter_chain.handle(*event));
}

TEST_F(EventFilterChain, prepends_appends_filters)
{
    auto filter1 = mock_filter();
    auto filter2 = mock_filter();
    auto filter3 = mock_filter();

    mi::EventFilterChain filter_chain{filter2};
    filter_chain.append(filter3);
    filter_chain.prepend(filter1);

    {
        InSequence s;
        EXPECT_CALL(*filter1, handle(_)).WillOnce(Return(false));
        EXPECT_CALL(*filter2, handle(_)).WillOnce(Return(false));
        EXPECT_CALL(*filter3, handle(_)).WillOnce(Return(false));
    }

    // So the filter chain should also reject the event
    EXPECT_FALSE(filter_chain.handle(*event));
}

TEST_F(EventFilterChain, accepting_event_halts_emission)
{
    auto filter = mock_filter();

    mi::EventFilterChain filter_chain{filter, filter, filter};

    // First filter will reject, second will accept, third one should not be asked.
    {
        InSequence seq;
        EXPECT_CALL(*filter, handle(_)).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*filter, handle(_)).Times(1).WillOnce(Return(true));
    }
    // So the chain should accept
    EXPECT_TRUE(filter_chain.handle(*event));
}

TEST_F(EventFilterChain, does_not_own_event_filters)
{
    auto filter = mock_filter();

    mi::EventFilterChain filter_chain{filter};
    EXPECT_CALL(*filter, handle(_)).Times(1).WillOnce(Return(true));
    EXPECT_TRUE(filter_chain.handle(*event));
    filter.reset();
    EXPECT_FALSE(filter_chain.handle(*event));
}
