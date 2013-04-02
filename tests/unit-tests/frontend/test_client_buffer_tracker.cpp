/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "../../src/server/frontend/client_buffer_tracker.h"
#include "mir/compositor/buffer_id.h"

#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mc = mir::compositor;

TEST(ClientBufferTracker, just_added_buffer_is_known_by_client)
{
    mf::ClientBufferTracker tracker;
    mc::BufferID id{5};

    tracker.add(id);
    EXPECT_TRUE(tracker.client_has(id));
}

TEST(ClientBufferTracker, unadded_buffer_is_unknown_by_client)
{
    mf::ClientBufferTracker tracker;

    tracker.add(mc::BufferID{5});
    EXPECT_FALSE(tracker.client_has(mc::BufferID{6}));
}

TEST(ClientBufferTracker, tracks_sequence_of_buffers)
{
    mf::ClientBufferTracker tracker;
    mc::BufferID one{1};
    mc::BufferID two{2};
    mc::BufferID three{3};
    mc::BufferID four{4};

    tracker.add(one);
    tracker.add(two);
    tracker.add(three);

    EXPECT_TRUE(tracker.client_has(one));
    EXPECT_TRUE(tracker.client_has(two));
    EXPECT_TRUE(tracker.client_has(three));
    EXPECT_FALSE(tracker.client_has(four));
}

TEST(ClientBufferTracker, old_buffers_expire_from_tracker)
{
    mf::ClientBufferTracker tracker;

    mc::BufferID one{1};
    mc::BufferID two{2};
    mc::BufferID three{3};
    mc::BufferID four{4};

    tracker.add(one);
    tracker.add(two);
    tracker.add(three);

    ASSERT_TRUE(tracker.client_has(one));
    ASSERT_TRUE(tracker.client_has(two));
    ASSERT_TRUE(tracker.client_has(three));

    tracker.add(two);
    tracker.add(three);
    tracker.add(four);

    EXPECT_FALSE(tracker.client_has(one));
    EXPECT_TRUE(tracker.client_has(two));
    EXPECT_TRUE(tracker.client_has(three));
    EXPECT_TRUE(tracker.client_has(four));
}