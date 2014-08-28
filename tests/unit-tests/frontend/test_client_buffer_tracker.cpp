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

#include "src/server/frontend/client_buffer_tracker.h"
#include "src/server/frontend/surface_tracker.h"
#include "mir/graphics/buffer_id.h"
#include "mir_test_doubles/stub_buffer.h"

#include <gtest/gtest.h>

namespace mtd = mir::test::doubles;
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

struct SurfaceTracker : public testing::Test
{
    mtd::StubBuffer stub_buffer0;
    mtd::StubBuffer stub_buffer1;
    mtd::StubBuffer stub_buffer2;
    mtd::StubBuffer stub_buffer3;
    mf::SurfaceId surf_id0{0};
    mf::SurfaceId surf_id1{1};
    size_t const client_cache_size{3};
};

TEST_F(SurfaceTracker, only_returns_true_if_buffer_already_tracked)
{
    mf::SurfaceTracker tracker{client_cache_size};

    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer0));
    EXPECT_TRUE(tracker.track_buffer(surf_id0, &stub_buffer0));

    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer1));
    EXPECT_FALSE(tracker.track_buffer(surf_id1, &stub_buffer0));
    EXPECT_FALSE(tracker.track_buffer(surf_id1, &stub_buffer1));
}

TEST_F(SurfaceTracker, removals_remove_buffer_instances)
{
    mf::SurfaceTracker tracker{client_cache_size};
    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer0));
    EXPECT_FALSE(tracker.track_buffer(surf_id1, &stub_buffer0));

    EXPECT_TRUE(tracker.track_buffer(surf_id0, &stub_buffer0));
    EXPECT_TRUE(tracker.track_buffer(surf_id1, &stub_buffer0));
    EXPECT_EQ(&stub_buffer0, tracker.last_buffer(surf_id0));

    tracker.remove_surface(surf_id0);
    EXPECT_EQ(nullptr, tracker.last_buffer(surf_id0));

    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer0));
    EXPECT_TRUE(tracker.track_buffer(surf_id1, &stub_buffer0));

    tracker.remove_surface(mf::SurfaceId{33});
}

TEST_F(SurfaceTracker, last_client_buffer)
{
    mf::SurfaceTracker tracker{client_cache_size};

    tracker.track_buffer(surf_id0, &stub_buffer0);
    EXPECT_EQ(&stub_buffer0, tracker.last_buffer(surf_id0));
    tracker.track_buffer(surf_id0, &stub_buffer1);
    EXPECT_EQ(&stub_buffer1, tracker.last_buffer(surf_id0));

    EXPECT_EQ(nullptr, tracker.last_buffer(surf_id1));

    tracker.track_buffer(surf_id1, &stub_buffer0);
    EXPECT_EQ(&stub_buffer0, tracker.last_buffer(surf_id1));
}

TEST_F(SurfaceTracker, buffers_expire_if_they_overrun_cache_size)
{
    mf::SurfaceTracker tracker{client_cache_size};

    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer0));
    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer1));
    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer2));
    //stub_buffer0 forced out
    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer3));
    //tracker reports its never seen stub_buffer0
    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer0));
    EXPECT_TRUE(tracker.track_buffer(surf_id0, &stub_buffer3));

    EXPECT_EQ(&stub_buffer3, tracker.last_buffer(surf_id0));

    EXPECT_TRUE(tracker.track_buffer(surf_id0, &stub_buffer3));
    EXPECT_TRUE(tracker.track_buffer(surf_id0, &stub_buffer0));
    EXPECT_TRUE(tracker.track_buffer(surf_id0, &stub_buffer2));
    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer1));
}

TEST_F(SurfaceTracker, buffers_only_affect_associated_surfaces)
{
    mf::SurfaceTracker tracker{client_cache_size};

    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer0));
    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer1));
    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer2));
    EXPECT_EQ(&stub_buffer2, tracker.last_buffer(surf_id0));
    
    EXPECT_FALSE(tracker.track_buffer(surf_id1, &stub_buffer0));
    EXPECT_EQ(&stub_buffer2, tracker.last_buffer(surf_id0));
    EXPECT_EQ(&stub_buffer0, tracker.last_buffer(surf_id1));

    EXPECT_FALSE(tracker.track_buffer(surf_id0, &stub_buffer3));
    EXPECT_EQ(&stub_buffer3, tracker.last_buffer(surf_id0));
    EXPECT_EQ(&stub_buffer0, tracker.last_buffer(surf_id1));
}
