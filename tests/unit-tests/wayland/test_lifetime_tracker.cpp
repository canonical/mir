/*
 * Copyright © 2020 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "mir/wayland/wayland_base.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mw = mir::wayland;

using namespace testing;

class MockTracker : public virtual mw::LifetimeTracker
{
public:
    // Expose protected method
    void mark_destroyed() const
    {
        LifetimeTracker::mark_destroyed();
    }
};

struct MockListener
{
    MOCK_METHOD0(callback, void());
};

TEST(LifetimeTrackerTest, first_destroy_listener_id_is_not_zero)
{
    MockTracker tracker;
    auto const id = tracker.add_destroy_listener([](){});
    EXPECT_THAT(id.as_value(), Ne(0));
}

TEST(LifetimeTrackerTest, destroy_listener_is_called)
{
    MockListener listener;
    EXPECT_CALL(listener, callback()).Times(1);
    MockTracker tracker;
    tracker.add_destroy_listener([&](){ listener.callback(); });
}

TEST(LifetimeTrackerTest, destroy_listener_only_called_once_when_marked_as_destroyed)
{
    MockListener listener;
    EXPECT_CALL(listener, callback()).Times(1);
    MockTracker tracker;
    tracker.add_destroy_listener([&](){ listener.callback(); });
    tracker.mark_destroyed();
}

TEST(LifetimeTrackerTest, destroy_listener_can_be_removed)
{
    MockListener listener_a;
    MockListener listener_b;
    EXPECT_CALL(listener_a, callback()).Times(0);
    EXPECT_CALL(listener_b, callback()).Times(1);
    MockTracker tracker;
    auto const id_a = tracker.add_destroy_listener([&](){ listener_a.callback(); });
    tracker.add_destroy_listener([&](){ listener_b.callback(); });
    tracker.remove_destroy_listener(id_a);
}

TEST(LifetimeTrackerTest, is_still_alive_when_destroy_listener_called)
{
    MockListener listener;
    EXPECT_CALL(listener, callback()).Times(1);
    MockTracker tracker;
    mw::Weak<MockTracker> weak{&tracker};
    tracker.add_destroy_listener(
        [&]()
        {
            listener.callback();
            EXPECT_THAT(weak, IsTrue());
        });
}

TEST(LifetimeTrackerTest, can_be_marked_as_destroyed_from_within_listener)
{
    MockListener listener;
    EXPECT_CALL(listener, callback()).Times(1);
    MockTracker tracker;
    tracker.add_destroy_listener(
        [&]()
        {
            listener.callback();
            // why would you want to do this? idk, but you can!
            tracker.mark_destroyed();
        });
}

TEST(LifetimeTrackerTest, removing_invalid_destroy_listener_ids_does_not_cause_problem)
{
    MockTracker tracker;
    tracker.remove_destroy_listener(mw::DestroyListenerId{0});
    tracker.remove_destroy_listener(mw::DestroyListenerId{125});
}
