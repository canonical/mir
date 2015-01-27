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

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "mir/input/event_filter.h"
#include "src/server/input/android/event_filter_dispatcher_policy.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_event_filter.h"

#include <androidfw/Input.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mia = mi::android;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

TEST(EventFilterDispatcherPolicy, offers_key_events_to_filter)
{
    using namespace ::testing;
    droidinput::KeyEvent ev;
    mtd::MockEventFilter filter;
    mia::EventFilterDispatcherPolicy policy(mt::fake_shared(filter), true);
    uint32_t policy_flags;

    EXPECT_CALL(filter, handle(_)).Times(1).WillOnce(Return(false));

    // The policy filters ALL key events before queuing
    policy.interceptKeyBeforeQueueing(&ev, policy_flags);

    // If the event is unfiltered we will allow it to pass to applications
    EXPECT_TRUE(policy_flags & droidinput::POLICY_FLAG_PASS_TO_USER);

    // Android uses alternate notation...the policy returns true if the event was NOT handled (e.g. the EventFilter
    // returns false)
    EXPECT_TRUE(policy.filterInputEvent(&ev, 0));
}

TEST(EventFilterDispatcherPolicy, motion_events_are_allowed_to_pass_to_clients)
{
    using namespace ::testing;

    mtd::MockEventFilter filter;
    mia::EventFilterDispatcherPolicy policy(mt::fake_shared(filter), true);

    uint32_t policy_flags;
    policy.interceptMotionBeforeQueueing(0, policy_flags);

    // All motion events are allowed. Of course they could later be removed by the input filter.
    EXPECT_TRUE(policy_flags & droidinput::POLICY_FLAG_PASS_TO_USER);
}
