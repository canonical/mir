/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/test/doubles/mock_event_sink.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "src/server/compositor/buffer_schedule.h"

#include <gtest/gtest.h>
using namespace testing;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mf = mir::frontend;
namespace geom = mir::geometry;

namespace
{
struct MockBufferAllocator : public mg::GraphicBufferAllocator
{
    MOCK_METHOD1(alloc_buffer, std::shared_ptr<mg::Buffer>(mg::BufferProperties const&));
    MOCK_METHOD0(supported_pixel_formats, std::vector<MirPixelFormat>());
};

struct StubBufferAllocator : public mg::GraphicBufferAllocator
{
    std::shared_ptr<mg::Buffer> alloc_buffer(mg::BufferProperties const&)
    {
        auto b = std::make_shared<mtd::StubBuffer>();
        map[b->id()] = b;
        ids.push_back(b->id());
        return b;
    }
    std::vector<MirPixelFormat> supported_pixel_formats()
    {
        return {};
    }
    std::map<mg::BufferID, std::shared_ptr<mg::Buffer>> map;
    std::vector<mg::BufferID> ids;
};

struct BufferSchedule : public Test
{
    testing::NiceMock<mtd::MockEventSink> mock_sink;
    mf::BufferStreamId stream_id{44};
    mg::BufferProperties properties{geom::Size{42,43}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};
    StubBufferAllocator allocator;
    mc::BufferSchedule schedule{stream_id, mt::fake_shared(mock_sink), mt::fake_shared(allocator)};
};

struct StartedBufferSchedule : BufferSchedule
{
    StartedBufferSchedule()
    {
        schedule.add_buffer(properties);
        schedule.add_buffer(properties);
        Mock::VerifyAndClearExpectations(&mock_sink);

        for(auto& pair : allocator.map)
            ids.push_back(pair.first);
        id1 = ids[0]; 
        id2 = ids[1];
        buffer1 = allocator.map[id1]; 
        buffer2 = allocator.map[id2]; 
    }

    std::vector<mg::BufferID> ids;
    mg::BufferID id1;
    mg::BufferID id2;
    std::shared_ptr<mg::Buffer> buffer1;
    std::shared_ptr<mg::Buffer> buffer2;
};
}

TEST_F(BufferSchedule, sends_full_buffer_on_allocation)
{
    auto stub_buffer = std::make_shared<mtd::StubBuffer>();
    MockBufferAllocator mock_allocator;
    EXPECT_CALL(mock_allocator, alloc_buffer(Ref(properties)))
        .WillOnce(Return(stub_buffer));
    EXPECT_CALL(mock_sink, send_buffer(stream_id, Ref(*stub_buffer), mg::BufferIpcMsgType::full_msg));
    mc::BufferSchedule schedule{stream_id, mt::fake_shared(mock_sink), mt::fake_shared(mock_allocator)};
    schedule.add_buffer(properties);
}

TEST_F(BufferSchedule, removal_of_nonexistent_buffer_throws)
{
    auto stub_buffer = std::make_unique<mtd::StubBuffer>();
    EXPECT_THROW({
        schedule.remove_buffer(stub_buffer->id());
    }, std::logic_error);
}

TEST_F(BufferSchedule, compositor_access_before_any_submission_throws)
{
    EXPECT_THROW({
        schedule.compositor_acquire(this);
    }, std::logic_error);
    schedule.add_buffer(properties);
    EXPECT_THROW({
        schedule.compositor_acquire(this);
    }, std::logic_error);
}

TEST_F(BufferSchedule, compositor_access)
{
    schedule.add_buffer(properties);
    ASSERT_THAT(allocator.ids, SizeIs(1));
    auto buffer_id = allocator.ids[0];
    schedule.schedule_buffer(buffer_id);
    auto cbuffer = schedule.compositor_acquire(this);
    EXPECT_THAT(cbuffer->id(), Eq(buffer_id));
}


TEST_F(StartedBufferSchedule, compositor_release_sends_buffer_back)
{
    EXPECT_CALL(mock_sink, send_buffer(stream_id, Ref(*buffer1), mg::BufferIpcMsgType::update_msg));

    schedule.schedule_buffer(id1);

    auto cbuffer = schedule.compositor_acquire(this);
    schedule.schedule_buffer(id2);
    schedule.compositor_release(cbuffer);
}

TEST_F(StartedBufferSchedule, compositor_can_acquire_different_buffers_if_submission_happens)
{
    EXPECT_CALL(mock_sink, send_buffer(_, Ref(*buffer1), mg::BufferIpcMsgType::update_msg));

    schedule.schedule_buffer(id1);
    auto cbuffer1 = schedule.compositor_acquire(this);
    schedule.schedule_buffer(id2);
    auto cbuffer2 = schedule.compositor_acquire(this);
    EXPECT_THAT(cbuffer1, Ne(cbuffer2));
    schedule.compositor_release(cbuffer2);
    schedule.compositor_release(cbuffer1);
}

TEST_F(StartedBufferSchedule, compositor_can_acquire_a_few_times_and_only_sends_on_the_last_release)
{
    schedule.schedule_buffer(id1);
    auto cbuffer1 = schedule.compositor_acquire(this);
    auto cbuffer2 = schedule.compositor_acquire(this);
    EXPECT_THAT(cbuffer1, Eq(cbuffer2));
    EXPECT_CALL(mock_sink, send_buffer(_, Ref(*cbuffer1), mg::BufferIpcMsgType::update_msg)).Times(Exactly(1));
    schedule.schedule_buffer(id2);
    schedule.compositor_release(cbuffer2);
    schedule.compositor_release(cbuffer1);
}

TEST_F(StartedBufferSchedule, scheduling_wont_send_front_if_no_compositor_has_seen_buffer)
{
    EXPECT_CALL(mock_sink, send_buffer(_,_,_)).Times(0);
    schedule.schedule_buffer(id1);
    schedule.schedule_buffer(id2);
}

TEST_F(StartedBufferSchedule, scheduling_can_send_front_buffer_if_no_compositors_still_have_it)
{
    schedule.schedule_buffer(id1);
    auto cbuffer1 = schedule.compositor_acquire(this);
    auto cbuffer2 = schedule.compositor_acquire(this);
    EXPECT_THAT(cbuffer1, Eq(cbuffer2));

    EXPECT_CALL(mock_sink, send_buffer(_, Ref(*cbuffer1), mg::BufferIpcMsgType::update_msg)).Times(Exactly(1));
    schedule.compositor_release(cbuffer2);
    schedule.compositor_release(cbuffer1);
    schedule.schedule_buffer(id2);
}

TEST_F(StartedBufferSchedule, removal_of_the_compositor_buffer_happens_after_compositor_release)
{
    EXPECT_CALL(mock_sink, send_buffer(_,_,mg::BufferIpcMsgType::update_msg))
        .Times(0);

    schedule.schedule_buffer(id1);
    schedule.remove_buffer(id1);

    auto cbuffer = schedule.compositor_acquire(this); 
    EXPECT_THAT(cbuffer->id(), Eq(id1));
    schedule.schedule_buffer(id2);
    schedule.compositor_release(cbuffer);
}

//mc::BufferQueue's current approach, alternative approaches exist
TEST_F(StartedBufferSchedule, compositor_buffer_syncs_to_fastest)
{
    int comp_id1{0};
    int comp_id2{0};

    schedule.schedule_buffer(id1);
    auto cbuffer1 = schedule.compositor_acquire(&comp_id1); 
    auto cbuffer2 = schedule.compositor_acquire(&comp_id2);

    schedule.schedule_buffer(id2);
    auto cbuffer3 = schedule.compositor_acquire(&comp_id1);

    schedule.schedule_buffer(id1);
    auto cbuffer4 = schedule.compositor_acquire(&comp_id1); 
    auto cbuffer5 = schedule.compositor_acquire(&comp_id2);

    schedule.schedule_buffer(id2);
    auto cbuffer6 = schedule.compositor_acquire(&comp_id2);
    auto cbuffer7 = schedule.compositor_acquire(&comp_id2);

    EXPECT_THAT(cbuffer1, Eq(allocator.map[id1]));
    EXPECT_THAT(cbuffer2, Eq(allocator.map[id1]));
    EXPECT_THAT(cbuffer3, Eq(allocator.map[id2]));
    EXPECT_THAT(cbuffer4, Eq(allocator.map[id1]));
    EXPECT_THAT(cbuffer5, Eq(allocator.map[id1]));
    EXPECT_THAT(cbuffer6, Eq(allocator.map[id2]));
    EXPECT_THAT(cbuffer7, Eq(allocator.map[id2]));
}

TEST_F(StartedBufferSchedule, compositor_buffer_syncs_to_fastest_with_more_queueing)
{
    schedule.add_buffer(properties);
    schedule.add_buffer(properties);
    schedule.add_buffer(properties);
    schedule.add_buffer(properties);
    auto id2 = allocator.ids[2];
    auto id3 = allocator.ids[3];
    auto id4 = allocator.ids[4];
    auto id5 = allocator.ids[5];
    auto buffer2 = allocator.map[id2];
    auto buffer3 = allocator.map[id3];
    auto buffer4 = allocator.map[id4];
    auto buffer5 = allocator.map[id5];

    int comp_id1{0};
    int comp_id2{0};

    schedule.schedule_buffer(id1);
    schedule.schedule_buffer(id2);
    schedule.schedule_buffer(id3);
    schedule.schedule_buffer(id4);
    schedule.schedule_buffer(id5);

    auto cbuffer1 = schedule.compositor_acquire(&comp_id1); //1
    auto cbuffer2 = schedule.compositor_acquire(&comp_id2); //1

    auto cbuffer3 = schedule.compositor_acquire(&comp_id1); //2

    auto cbuffer4 = schedule.compositor_acquire(&comp_id1); //3
    auto cbuffer5 = schedule.compositor_acquire(&comp_id2); //3

    auto cbuffer6 = schedule.compositor_acquire(&comp_id2); //4

    auto cbuffer7 = schedule.compositor_acquire(&comp_id2); //5
    auto cbuffer8 = schedule.compositor_acquire(&comp_id1); //5

    EXPECT_THAT(cbuffer1, Eq(allocator.map[id1]));
    EXPECT_THAT(cbuffer2, Eq(allocator.map[id1]));

    EXPECT_THAT(cbuffer3, Eq(allocator.map[id2]));


    EXPECT_THAT(cbuffer4, Eq(allocator.map[id3]));
    EXPECT_THAT(cbuffer5, Eq(allocator.map[id3]));

    EXPECT_THAT(cbuffer6, Eq(allocator.map[id2]));
    EXPECT_THAT(cbuffer7, Eq(allocator.map[id2]));
}


#if 0
//good idea?
TEST_F(BufferSchedule, submitted_buffer_can_be_ejected)
{
    schedule.schedule_buffer(buffer2);
    schedule.schedule_buffer(buffer1);
    schedule.eject(buffer1);

    EXPECT_THAT(schedule.compositor_acquire(id), Eq(buffer2));
}
#endif
