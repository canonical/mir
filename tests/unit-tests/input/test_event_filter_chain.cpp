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

#include "mir/input/event_filter_chain.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mir_test/empty_deleter.h"

#include <androidfw/Input.h>

namespace mi = mir::input;

namespace
{
struct MockEventFilter : public mi::EventFilter
{
    MOCK_METHOD1(handles, bool(android::InputEvent*));
};
}

TEST(EventFilterChain, offers_events_to_filters)
{
    using namespace ::testing;
    mi::EventFilterChain filter_chain;
    auto filter = std::make_shared<MockEventFilter>();
    auto ev = new android::KeyEvent();
    
    filter_chain.add_filter(filter);
    filter_chain.add_filter(filter);

    // Filter will pass the event on twice
    EXPECT_CALL(*filter, handles(_)).Times(2).WillRepeatedly(Return(false));
    
    // So the filter chain should also reject the event
    EXPECT_EQ(filter_chain.handles(ev), false);    
}

TEST(EventFilterChain, accepting_event_halts_emission)
{
    using namespace ::testing;
    mi::EventFilterChain filter_chain;
    auto filter = std::make_shared<MockEventFilter>();
    auto ev = new android::KeyEvent();
    
    filter_chain.add_filter(filter);
    filter_chain.add_filter(filter);
    filter_chain.add_filter(filter);

    // First filter will reject, second will accept, third one should not be asked.
    {
        InSequence seq;
        EXPECT_CALL(*filter, handles(_)).Times(1).WillOnce(Return(false));
        EXPECT_CALL(*filter, handles(_)).Times(1).WillOnce(Return(true));
    }
    
    // So the chain should accept
    EXPECT_EQ(filter_chain.handles(ev), true);    
}

