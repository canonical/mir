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
#include "src/server/frontend/buffer_stream_tracker.h"
#include "mir/graphics/buffer_id.h"
#include "mir/test/doubles/stub_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;
namespace mg = mir::graphics;

struct ClientBufferTracker : public testing::Test
{
    mtd::StubBuffer stub_buffer0;
    mtd::StubBuffer stub_buffer1;
    mtd::StubBuffer stub_buffer2;
    mtd::StubBuffer stub_buffer3;
    mtd::StubBuffer stub_buffer4;
};

TEST_F(ClientBufferTracker, just_added_buffer_is_known_by_client)
{
    mf::ClientBufferTracker tracker(3);
    mg::BufferID const id{stub_buffer0.id()};

    tracker.add(&stub_buffer0);
    EXPECT_TRUE(tracker.client_has(id));
    EXPECT_THAT(tracker.buffer_from(id), testing::Eq(&stub_buffer0));
}

TEST_F(ClientBufferTracker, nullptrs_dont_affect_owned_buffers)
{
    mf::ClientBufferTracker tracker(3);
    mg::BufferID const id{stub_buffer0.id()};

    tracker.add(&stub_buffer0);
    tracker.add(nullptr);
    tracker.add(nullptr);
    tracker.add(nullptr);
    EXPECT_TRUE(tracker.client_has(id));
    EXPECT_THAT(tracker.buffer_from(id), testing::Eq(&stub_buffer0));
}

TEST_F(ClientBufferTracker, dead_buffers_are_still_tracked)
{
    mf::ClientBufferTracker tracker(3);
    mg::BufferID id{stub_buffer0.id()};
    mg::Buffer* dead_buffer_ptr{nullptr};

    {
        mtd::StubBuffer dead_buffer;
        tracker.add(&dead_buffer);
        id = dead_buffer.id();
        dead_buffer_ptr = &dead_buffer;
    }

    EXPECT_TRUE(tracker.client_has(id));
    EXPECT_THAT(tracker.buffer_from(id), testing::Eq(dead_buffer_ptr));
}

TEST_F(ClientBufferTracker, unadded_buffer_is_unknown_by_client)
{
    mf::ClientBufferTracker tracker(3);

    tracker.add(&stub_buffer0);
    EXPECT_FALSE(tracker.client_has(stub_buffer1.id()));
}

TEST_F(ClientBufferTracker, tracks_sequence_of_buffers)
{
    mf::ClientBufferTracker tracker(3);

    tracker.add(&stub_buffer0);
    tracker.add(&stub_buffer1);
    tracker.add(&stub_buffer2);

    EXPECT_TRUE(tracker.client_has(stub_buffer0.id()));
    EXPECT_TRUE(tracker.client_has(stub_buffer1.id()));
    EXPECT_TRUE(tracker.client_has(stub_buffer2.id()));
    EXPECT_FALSE(tracker.client_has(stub_buffer3.id()));
}

TEST_F(ClientBufferTracker, old_buffers_expire_from_tracker)
{
    mf::ClientBufferTracker tracker(3);

    tracker.add(&stub_buffer0);
    tracker.add(&stub_buffer1);
    tracker.add(&stub_buffer2);

    ASSERT_TRUE(tracker.client_has(stub_buffer0.id()));
    ASSERT_TRUE(tracker.client_has(stub_buffer1.id()));
    ASSERT_TRUE(tracker.client_has(stub_buffer2.id()));

    tracker.add(&stub_buffer1);
    tracker.add(&stub_buffer2);
    tracker.add(&stub_buffer3);

    EXPECT_FALSE(tracker.client_has(stub_buffer0.id()));
    EXPECT_TRUE(tracker.client_has(stub_buffer1.id()));
    EXPECT_TRUE(tracker.client_has(stub_buffer2.id()));
    EXPECT_TRUE(tracker.client_has(stub_buffer3.id()));
}

TEST_F(ClientBufferTracker, tracks_correct_number_of_buffers)
{
    std::vector<mtd::StubBuffer> buffers(10);
    for (unsigned int tracker_size = 2; tracker_size < 10; ++tracker_size)
    {
        mf::ClientBufferTracker tracker{tracker_size};

        for (unsigned int i = 0; i <= tracker_size; ++i)
            tracker.add(&buffers[i]);

        EXPECT_FALSE(tracker.client_has(buffers[0].id()));
        for (unsigned int i = 1; i <= tracker_size; ++i)
            EXPECT_TRUE(tracker.client_has(buffers[i].id()));
    }
}

struct BufferStreamTracker : public testing::Test
{
    mtd::StubBuffer stub_buffer0;
    mtd::StubBuffer stub_buffer1;
    mtd::StubBuffer stub_buffer2;
    mtd::StubBuffer stub_buffer3;
    mtd::StubBuffer stub_buffer4;
    mf::BufferStreamId stream_id0{0};
    mf::BufferStreamId stream_id1{1};
    size_t const client_cache_size{3};
};

TEST_F(BufferStreamTracker, only_returns_true_if_buffer_already_tracked)
{
    mf::BufferStreamTracker tracker{client_cache_size};

    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer0));
    EXPECT_TRUE(tracker.track_buffer(stream_id0, &stub_buffer0));

    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer1));
    EXPECT_FALSE(tracker.track_buffer(stream_id1, &stub_buffer2));
    EXPECT_FALSE(tracker.track_buffer(stream_id1, &stub_buffer3));
}

TEST_F(BufferStreamTracker, removals_remove_buffer_instances)
{
    mf::BufferStreamTracker tracker{client_cache_size};
    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer0));
    EXPECT_FALSE(tracker.track_buffer(stream_id1, &stub_buffer1));

    EXPECT_TRUE(tracker.track_buffer(stream_id0, &stub_buffer0));
    EXPECT_TRUE(tracker.track_buffer(stream_id1, &stub_buffer1));
    EXPECT_EQ(&stub_buffer0, tracker.last_buffer(stream_id0));

    tracker.remove_buffer_stream(stream_id0);
    EXPECT_EQ(nullptr, tracker.last_buffer(stream_id0));

    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer0));
    EXPECT_TRUE(tracker.track_buffer(stream_id1, &stub_buffer1));

    tracker.remove_buffer_stream(mf::BufferStreamId{33});
}

TEST_F(BufferStreamTracker, last_client_buffer)
{
    mf::BufferStreamTracker tracker{client_cache_size};

    tracker.track_buffer(stream_id0, &stub_buffer0);
    EXPECT_EQ(&stub_buffer0, tracker.last_buffer(stream_id0));
    tracker.track_buffer(stream_id0, &stub_buffer1);
    EXPECT_EQ(&stub_buffer1, tracker.last_buffer(stream_id0));

    EXPECT_EQ(nullptr, tracker.last_buffer(stream_id1));

    tracker.track_buffer(stream_id1, &stub_buffer2);
    EXPECT_EQ(&stub_buffer2, tracker.last_buffer(stream_id1));
}

TEST_F(BufferStreamTracker, buffers_expire_if_they_overrun_cache_size)
{
    mf::BufferStreamTracker tracker{client_cache_size};

    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer0));
    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer1));
    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer2));
    //stub_buffer0 forced out
    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer3));
    //tracker reports its never seen stub_buffer0
    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer0));
    EXPECT_TRUE(tracker.track_buffer(stream_id0, &stub_buffer3));

    EXPECT_EQ(&stub_buffer3, tracker.last_buffer(stream_id0));

    EXPECT_TRUE(tracker.track_buffer(stream_id0, &stub_buffer3));
    EXPECT_TRUE(tracker.track_buffer(stream_id0, &stub_buffer0));
    EXPECT_TRUE(tracker.track_buffer(stream_id0, &stub_buffer2));
    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer1));
}

TEST_F(BufferStreamTracker, buffers_only_affect_associated_surfaces)
{
    mf::BufferStreamTracker tracker{client_cache_size};

    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer0));
    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer1));
    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer2));
    EXPECT_EQ(&stub_buffer2, tracker.last_buffer(stream_id0));
    
    EXPECT_FALSE(tracker.track_buffer(stream_id1, &stub_buffer3));
    EXPECT_EQ(&stub_buffer2, tracker.last_buffer(stream_id0));
    EXPECT_EQ(&stub_buffer3, tracker.last_buffer(stream_id1));

    EXPECT_FALSE(tracker.track_buffer(stream_id0, &stub_buffer4));
    EXPECT_EQ(&stub_buffer4, tracker.last_buffer(stream_id0));
    EXPECT_EQ(&stub_buffer3, tracker.last_buffer(stream_id1));
}

TEST_F(BufferStreamTracker, can_lookup_a_buffer_from_a_buffer_id)
{
    using namespace testing;
    mf::BufferStreamTracker tracker{client_cache_size};

    tracker.track_buffer(stream_id0, &stub_buffer0);
    tracker.track_buffer(stream_id0, &stub_buffer1);
    tracker.track_buffer(stream_id0, &stub_buffer2);

    EXPECT_THAT(tracker.buffer_from(stub_buffer0.id()), Eq(&stub_buffer0));
    EXPECT_THAT(tracker.buffer_from(stub_buffer1.id()), Eq(&stub_buffer1));
    EXPECT_THAT(tracker.buffer_from(stub_buffer2.id()), Eq(&stub_buffer2));
    EXPECT_THROW({
        tracker.buffer_from(stub_buffer3.id());
    }, std::logic_error);

    tracker.track_buffer(stream_id0, &stub_buffer3);
    EXPECT_THAT(tracker.buffer_from(stub_buffer3.id()), Eq(&stub_buffer3));
    EXPECT_THROW({
        tracker.buffer_from(stub_buffer0.id());
    }, std::logic_error);
}
