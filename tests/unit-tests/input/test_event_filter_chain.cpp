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

#include "mir_test/empty_deleter"

#include <androidfw/Input.h>

namespace mi = mir::input;

namespace
{
struct MockEventFilter : public mi::EventFilter
{
    MOCK_METHOD1(filter_event, bool(android::InputEvent*));
}
}

TEST(EventFilterChain, offers_events_to_filters)
{
    using namespace ::testing;
    mi::FilterChain filter_chain;
    auto filter = std::make_shared<MockEventFilter>(mir::EmptyDeleter());
    auto ev = new android::InputEvent();
    
    filter_chain.add_filter(filter);
    filter_chain.add_filter(filter);

    ON_CALL(*filter, filter_event(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filter, filter_event(_)).Times(2);
    
    filter_chain.filter_event(ev);    
}

