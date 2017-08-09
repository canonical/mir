/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/mock_event_sink.h"
#include "mir/test/fake_shared.h"
#include "src/server/compositor/stream.h"
#include "mir/scene/null_surface_observer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir/test/gmock_fixes.h"
using namespace testing;
namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace
{
struct MockSurfaceObserver : mir::scene::NullSurfaceObserver
{
    MOCK_METHOD2(frame_posted, void(int, geom::Size const&));
};

struct Stream : Test
{
    Stream() :
        buffers{
            std::make_shared<mtd::StubBuffer>(initial_size),
            std::make_shared<mtd::StubBuffer>(initial_size),
            std::make_shared<mtd::StubBuffer>(initial_size)}
    {
    }
    
    MOCK_METHOD1(called, void(mg::Buffer&));

    geom::Size initial_size{44,2};
    std::vector<std::shared_ptr<mg::Buffer>> buffers;
    MirPixelFormat construction_format{mir_pixel_format_rgb_565};
    mc::Stream stream{
        initial_size, construction_format};
};
}

TEST_F(Stream, transitions_from_queuing_to_framedropping)
{
    for(auto& buffer : buffers)
        stream.submit_buffer(buffer);
    stream.allow_framedropping(true);

    std::vector<std::shared_ptr<mg::Buffer>> cbuffers;
    while(stream.buffers_ready_for_compositor(this))
        cbuffers.push_back(stream.lock_compositor_buffer(this));
    // Transition to framedropping should have dropped all queued buffers but the last...
    ASSERT_THAT(cbuffers, SizeIs(1));
    EXPECT_THAT(cbuffers[0]->id(), Eq(buffers.back()->id()));

    for (unsigned long i = 0; i < buffers.size() - 1; ++i)
    {
        // ...and so all the previous buffers should no longer have external references
        EXPECT_TRUE(buffers[i].unique());
    }
}

TEST_F(Stream, transitions_from_framedropping_to_queuing)
{
    stream.allow_framedropping(true);

    for(auto& buffer : buffers)
        stream.submit_buffer(buffer);

    // Only the last buffer should be owned by the stream...
    EXPECT_THAT(
        std::make_tuple(buffers.data(), buffers.size() - 1),
        Each(Property(&std::shared_ptr<mg::Buffer>::unique, Eq(true))));

    stream.allow_framedropping(false);
    for(auto& buffer : buffers)
        stream.submit_buffer(buffer);

    // All buffers should be now owned by the the stream
    EXPECT_THAT(
        buffers,
        Each(Property(&std::shared_ptr<mg::Buffer>::unique, Eq(false))));

    std::vector<std::shared_ptr<mg::Buffer>> cbuffers;
    while(stream.buffers_ready_for_compositor(this))
        cbuffers.push_back(stream.lock_compositor_buffer(this));
    EXPECT_THAT(cbuffers, SizeIs(buffers.size()));
}

TEST_F(Stream, indicates_buffers_ready_when_queueing)
{
    for(auto& buffer : buffers)
        stream.submit_buffer(buffer);

    for(auto i = 0u; i < buffers.size(); i++)
    {
        EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(1));
        stream.lock_compositor_buffer(this);
    }

    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(0));
}

TEST_F(Stream, indicates_buffers_ready_when_dropping)
{
    stream.allow_framedropping(true);

    for(auto& buffer : buffers)
        stream.submit_buffer(buffer);

    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(1));
    stream.lock_compositor_buffer(this);
    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(0));
}

TEST_F(Stream, tracks_has_buffer)
{
    EXPECT_FALSE(stream.has_submitted_buffer());
    stream.submit_buffer(buffers[0]);
    EXPECT_TRUE(stream.has_submitted_buffer());
}

TEST_F(Stream, calls_observers_after_scheduling_on_submissions)
{
    auto observer = std::make_shared<MockSurfaceObserver>();
    EXPECT_CALL(*observer, frame_posted(1, initial_size));
    stream.add_observer(observer);
    stream.submit_buffer(buffers[0]);
    stream.remove_observer(observer);
    stream.submit_buffer(buffers[0]);
}

TEST_F(Stream, calls_observers_call_doesnt_hold_lock)
{
    auto observer = std::make_shared<MockSurfaceObserver>();
    EXPECT_CALL(*observer, frame_posted(1,_))
        .WillOnce(InvokeWithoutArgs([&]{
            EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(1));
            EXPECT_TRUE(stream.has_submitted_buffer());
        }));
    stream.add_observer(observer);
    stream.submit_buffer(buffers[0]);
}

TEST_F(Stream, flattens_queue_out_when_told_to_drop)
{
    for(auto& buffer : buffers)
        stream.submit_buffer(buffer);

    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(1));
    stream.drop_old_buffers();
    stream.lock_compositor_buffer(this);
    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(0));
}

TEST_F(Stream, forces_a_new_buffer_when_told_to_drop_buffers)
{
    int that{0};
    stream.submit_buffer(buffers[0]);
    stream.submit_buffer(buffers[1]);
    stream.submit_buffer(buffers[2]);

    auto a = stream.lock_compositor_buffer(this);
    stream.drop_old_buffers();
    auto b = stream.lock_compositor_buffer(&that);
    auto c = stream.lock_compositor_buffer(this);
    EXPECT_THAT(b->id(), Eq(c->id()));
    EXPECT_THAT(a->id(), Ne(b->id())); 
    EXPECT_THAT(a->id(), Ne(c->id())); 
}

TEST_F(Stream, throws_on_nullptr_submissions)
{
    auto observer = std::make_shared<MockSurfaceObserver>();
    EXPECT_CALL(*observer, frame_posted(_,_)).Times(0);
    stream.add_observer(observer);
    EXPECT_THROW({
        stream.submit_buffer(nullptr);
    }, std::invalid_argument);
    EXPECT_FALSE(stream.has_submitted_buffer());
}

//it doesnt quite make sense that the stream has a size, esp given that there could be different-sized buffers
//in the stream, and the surface has the onscreen size info
TEST_F(Stream, reports_size)
{
    geom::Size new_size{333,139};
    EXPECT_THAT(stream.stream_size(), Eq(initial_size));
    stream.resize(new_size);
    EXPECT_THAT(stream.stream_size(), Eq(new_size));
}

//Likewise, no reason buffers couldn't all be a different pixel format
TEST_F(Stream, reports_format)
{
    EXPECT_THAT(stream.pixel_format(), Eq(construction_format));
}

TEST_F(Stream, returns_buffers_to_client_when_told_to_bring_queue_up_to_date)
{
    stream.submit_buffer(buffers[0]);
    stream.submit_buffer(buffers[1]);
    stream.submit_buffer(buffers[2]);

    // Buffers should be owned by the stream, and our test
    ASSERT_THAT(buffers[0].use_count(), Eq(2));
    ASSERT_THAT(buffers[1].use_count(), Eq(2));
    ASSERT_THAT(buffers[2].use_count(), Eq(2));

    stream.drop_old_buffers();

    // Stream should have released ownership of all but the most recent buffer
    EXPECT_THAT(buffers[0].use_count(), Eq(1));
    EXPECT_THAT(buffers[1].use_count(), Eq(1));
    EXPECT_THAT(buffers[2].use_count(), Eq(2));
}
