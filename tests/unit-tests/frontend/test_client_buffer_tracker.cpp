/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "../../src/server/frontend/client_buffer_tracker.h"
#include "mir/graphics/buffer_id.h"

#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;

TEST(ClientBufferTracker, just_added_buffer_is_known_by_client)
{
    mf::ClientBufferTracker tracker(3);
    mg::BufferID const id{5};

    tracker.add(id);
    EXPECT_TRUE(tracker.client_has(id));
}

TEST(ClientBufferTracker, unadded_buffer_is_unknown_by_client)
{
    mf::ClientBufferTracker tracker(3);

    tracker.add(mg::BufferID{5});
    EXPECT_FALSE(tracker.client_has(mg::BufferID{6}));
}

TEST(ClientBufferTracker, tracks_sequence_of_buffers)
{
    mf::ClientBufferTracker tracker(3);
    mg::BufferID const one{1};
    mg::BufferID const two{2};
    mg::BufferID const three{3};
    mg::BufferID const four{4};

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
    mf::ClientBufferTracker tracker(3);

    mg::BufferID const one{1};
    mg::BufferID const two{2};
    mg::BufferID const three{3};
    mg::BufferID const four{4};

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

TEST(ClientBufferTracker, tracks_correct_number_of_buffers)
{
    mg::BufferID ids[10];
    for (unsigned int i = 0; i < 10; ++i)
        ids[i] = mg::BufferID{i};

    for (unsigned int tracker_size = 2; tracker_size < 10; ++tracker_size)
    {
        mf::ClientBufferTracker tracker{tracker_size};

        for (unsigned int i = 0; i < tracker_size; ++i)
            tracker.add(ids[i]);

        tracker.add(ids[tracker_size]);

        EXPECT_FALSE(tracker.client_has(ids[0]));
        for (unsigned int i = 1; i <= tracker_size; ++i)
            EXPECT_TRUE(tracker.client_has(ids[i]));
    }
}
