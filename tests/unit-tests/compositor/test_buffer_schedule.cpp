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
}

struct BufferMap : public Test
{
    testing::NiceMock<mtd::MockEventSink> mock_sink;
    mf::BufferStreamId stream_id{44};
    mg::BufferProperties properties{geom::Size{42,43}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};
    MockBufferAllocator mock_allocator;
    StubBufferAllocator stub_allocator;
    mc::BufferMap map{stream_id, mt::fake_shared(mock_sink), mt::fake_shared(stub_allocator)};
};

TEST_F(BufferMap, sends_full_buffer_on_allocation)
{
    auto stub_buffer = std::make_shared<mtd::StubBuffer>();
    EXPECT_CALL(mock_allocator, alloc_buffer(Ref(properties)))
        .WillOnce(Return(stub_buffer));
    EXPECT_CALL(mock_sink, send_buffer(stream_id, Ref(*stub_buffer), mg::BufferIpcMsgType::full_msg));
    mc::BufferMap map{stream_id, mt::fake_shared(mock_sink), mt::fake_shared(mock_allocator)};
    map.add_buffer(properties);
}

TEST_F(BufferMap, removal_of_nonexistent_buffer_throws)
{
    auto stub_buffer = std::make_unique<mtd::StubBuffer>();
    EXPECT_THROW({
        map.remove_buffer(stub_buffer->id());
    }, std::logic_error);
}

TEST_F(BufferMap, can_access_once_added)
{
    map.add_buffer(properties);
    ASSERT_THAT(stub_allocator.map, SizeIs(1));
    ASSERT_THAT(stub_allocator.ids, SizeIs(1));
    auto buffer = map[stub_allocator.ids[0]];
    EXPECT_THAT(buffer, Eq(stub_allocator.map[stub_allocator.ids[0]]));
}

TEST_F(BufferMap, sends_update_msg_to_send_buffer)
{
    map.add_buffer(properties);
    ASSERT_THAT(stub_allocator.map, SizeIs(1));
    ASSERT_THAT(stub_allocator.ids, SizeIs(1));
    auto buffer = map[stub_allocator.ids[0]];
    EXPECT_CALL(mock_sink, send_buffer(stream_id, Ref(*buffer), mg::BufferIpcMsgType::update_msg));
    map.send_buffer(stub_allocator.ids[0]);
}

TEST_F(BufferMap, sends_no_update_msg_if_buffer_is_not_around)
{
    map.add_buffer(properties);
    ASSERT_THAT(stub_allocator.map, SizeIs(1));
    ASSERT_THAT(stub_allocator.ids, SizeIs(1));
    auto buffer = map[stub_allocator.ids[0]];
    EXPECT_CALL(mock_sink, send_buffer(stream_id, Ref(*buffer), mg::BufferIpcMsgType::update_msg))
        .Times(0);
    map.remove_buffer(stub_allocator.ids[0]);
    map.send_buffer(stub_allocator.ids[0]);
}
























struct MockBufferMap : mc::ClientBufferMap
{
    MOCK_METHOD1(add_buffer, void(mg::BufferProperties const&));
    MOCK_METHOD1(remove_buffer, void(mg::BufferID id));
    MOCK_METHOD1(send_buffer, void(mg::BufferID id));
    MOCK_METHOD1(at, std::shared_ptr<mg::Buffer>&(mg::BufferID));
    std::shared_ptr<mg::Buffer>& operator[](mg::BufferID id) { return at(id); }
};

struct MonitorSchedule : Test
{
    MonitorSchedule()
    {
        for(auto i = 0u; i < num_buffers; i++)
            buffers.emplace_back(std::make_shared<mtd::StubBuffer>());
        for(auto i = 0u; i < num_buffers; i++)
        {
            ids.emplace_back(buffers[i]->id());
            ON_CALL(mock_map, at(ids.back())).WillByDefault(ReturnRef(buffers[i]));
        }
    }

    unsigned int const num_buffers{6u};
    std::vector<std::shared_ptr<mg::Buffer>> buffers;
    std::vector<mg::BufferID> ids;
    NiceMock<MockBufferMap> mock_map;
    mc::BufferSchedule schedule{
        mt::fake_shared(mock_map), std::make_unique<mc::QueueingSchedule>()};
};

TEST_F(MonitorSchedule, compositor_access_before_any_submission_throws)
{
    //nothing owned
    EXPECT_THROW({
        schedule.compositor_acquire(this);
    }, std::logic_error);

    schedule.schedule_buffer(ids[0]);

    //something scheduled, should be ok
    schedule.compositor_acquire(this);
}

TEST_F(MonitorSchedule, compositor_access)
{
    schedule.schedule_buffer(ids[0]);
    auto cbuffer = schedule.compositor_acquire(this);
    EXPECT_THAT(cbuffer->id(), Eq(ids[0]));
}

TEST_F(MonitorSchedule, compositor_release_sends_buffer_back)
{
    EXPECT_CALL(mock_map, send_buffer(ids[0]));

    schedule.schedule_buffer(ids[0]);

    auto cbuffer = schedule.compositor_acquire(this);
    schedule.schedule_buffer(ids[1]);
    schedule.compositor_release(cbuffer);
}

TEST_F(MonitorSchedule, compositor_can_acquire_different_buffers_if_submission_happens)
{
    EXPECT_CALL(mock_map, send_buffer(ids[0]));

    schedule.schedule_buffer(ids[0]);
    auto cbuffer1 = schedule.compositor_acquire(this);
    schedule.schedule_buffer(ids[1]);
    auto cbuffer2 = schedule.compositor_acquire(this);
    EXPECT_THAT(cbuffer1, Ne(cbuffer2));
    schedule.compositor_release(cbuffer2);
    schedule.compositor_release(cbuffer1);
}

TEST_F(MonitorSchedule, compositor_can_acquire_a_few_times_and_only_sends_on_the_last_release)
{
    schedule.schedule_buffer(ids[0]);
    auto cbuffer1 = schedule.compositor_acquire(this);
    auto cbuffer2 = schedule.compositor_acquire(this);
    EXPECT_THAT(cbuffer1, Eq(cbuffer2));
    EXPECT_CALL(mock_map, send_buffer(ids[0])).Times(Exactly(1));
    schedule.schedule_buffer(ids[1]);
    schedule.compositor_release(cbuffer2);
    schedule.compositor_release(cbuffer1);
}

TEST_F(MonitorSchedule, compositor_buffer_syncs_to_fastest_compositor)
{
    int comp_id1{0};
    int comp_id2{0};

    schedule.schedule_buffer(ids[0]);
    auto cbuffer1 = schedule.compositor_acquire(&comp_id1); 
    auto cbuffer2 = schedule.compositor_acquire(&comp_id2);

    schedule.schedule_buffer(ids[1]);
    auto cbuffer3 = schedule.compositor_acquire(&comp_id1);

    schedule.schedule_buffer(ids[0]);
    auto cbuffer4 = schedule.compositor_acquire(&comp_id1); 
    auto cbuffer5 = schedule.compositor_acquire(&comp_id2);

    schedule.schedule_buffer(ids[1]);
    auto cbuffer6 = schedule.compositor_acquire(&comp_id2);
    auto cbuffer7 = schedule.compositor_acquire(&comp_id2);

    EXPECT_THAT(cbuffer1, Eq(buffers[0]));
    EXPECT_THAT(cbuffer2, Eq(buffers[0]));
    EXPECT_THAT(cbuffer3, Eq(buffers[1]));
    EXPECT_THAT(cbuffer4, Eq(buffers[0]));
    EXPECT_THAT(cbuffer5, Eq(buffers[0]));
    EXPECT_THAT(cbuffer6, Eq(buffers[1]));
    EXPECT_THAT(cbuffer7, Eq(buffers[1]));
}

TEST_F(MonitorSchedule, compositor_consumes_all_buffers_when_operating_as_a_composited_scene_would)
{
    schedule.schedule_buffer(ids[0]);
    schedule.schedule_buffer(ids[1]);
    schedule.schedule_buffer(ids[2]);
    schedule.schedule_buffer(ids[3]);
    schedule.schedule_buffer(ids[4]);

    auto cbuffer1 = schedule.compositor_acquire(this);
    schedule.compositor_release(cbuffer1);
    auto cbuffer2 = schedule.compositor_acquire(this);
    schedule.compositor_release(cbuffer2);
    auto cbuffer3 = schedule.compositor_acquire(this);
    schedule.compositor_release(cbuffer3);
    auto cbuffer4 = schedule.compositor_acquire(this);
    schedule.compositor_release(cbuffer4);
    auto cbuffer5 = schedule.compositor_acquire(this);
    schedule.compositor_release(cbuffer5);

    EXPECT_THAT(cbuffer1, Eq(buffers[0]));
    EXPECT_THAT(cbuffer2, Eq(buffers[1]));
    EXPECT_THAT(cbuffer3, Eq(buffers[2]));
    EXPECT_THAT(cbuffer4, Eq(buffers[3]));
    EXPECT_THAT(cbuffer5, Eq(buffers[4]));
}

TEST_F(MonitorSchedule, compositor_consumes_all_buffers_when_operating_as_a_bypassed_buffer_would)
{
    schedule.schedule_buffer(ids[0]);
    schedule.schedule_buffer(ids[1]);
    schedule.schedule_buffer(ids[2]);
    schedule.schedule_buffer(ids[3]);
    schedule.schedule_buffer(ids[4]);

    auto cbuffer1 = schedule.compositor_acquire(this);
    auto cbuffer2 = schedule.compositor_acquire(this);
    schedule.compositor_release(cbuffer1);
    auto cbuffer3 = schedule.compositor_acquire(this);
    schedule.compositor_release(cbuffer2);
    auto cbuffer4 = schedule.compositor_acquire(this);
    schedule.compositor_release(cbuffer3);
    auto cbuffer5 = schedule.compositor_acquire(this);
    schedule.compositor_release(cbuffer4);
    schedule.compositor_release(cbuffer5);

    EXPECT_THAT(cbuffer1, Eq(buffers[0]));
    EXPECT_THAT(cbuffer2, Eq(buffers[1]));
    EXPECT_THAT(cbuffer3, Eq(buffers[2]));
    EXPECT_THAT(cbuffer4, Eq(buffers[3]));
    EXPECT_THAT(cbuffer5, Eq(buffers[4]));
}

TEST_F(MonitorSchedule, multimonitor_compositor_buffer_syncs_to_fastest_with_more_queueing)
{
    int comp_id1{0};
    int comp_id2{0};

    schedule.schedule_buffer(ids[0]);
    schedule.schedule_buffer(ids[1]);
    schedule.schedule_buffer(ids[2]);
    schedule.schedule_buffer(ids[3]);
    schedule.schedule_buffer(ids[4]);

    auto cbuffer1 = schedule.compositor_acquire(&comp_id1); //1
    auto cbuffer2 = schedule.compositor_acquire(&comp_id2); //1

    auto cbuffer3 = schedule.compositor_acquire(&comp_id1); //2

    auto cbuffer4 = schedule.compositor_acquire(&comp_id1); //3
    auto cbuffer5 = schedule.compositor_acquire(&comp_id2); //3

    auto cbuffer6 = schedule.compositor_acquire(&comp_id2); //4

    auto cbuffer7 = schedule.compositor_acquire(&comp_id2); //5
    auto cbuffer8 = schedule.compositor_acquire(&comp_id1); //5

    EXPECT_THAT(cbuffer1, Eq(buffers[0]));
    EXPECT_THAT(cbuffer2, Eq(buffers[0]));

    EXPECT_THAT(cbuffer3, Eq(buffers[1]));

    EXPECT_THAT(cbuffer4, Eq(buffers[2]));
    EXPECT_THAT(cbuffer5, Eq(buffers[2]));

    EXPECT_THAT(cbuffer6, Eq(buffers[3]));

    EXPECT_THAT(cbuffer7, Eq(buffers[4]));
    EXPECT_THAT(cbuffer8, Eq(buffers[4]));
}

#if 0 //evaluate on monday if this is valid after teasing out
TEST_F(MonitorSchedule, scheduling_can_send_buffer_if_framedropping_and_no_compositors_still_have_it)
{
    schedule.schedule_buffer(id1);
    auto cbuffer1 = schedule.compositor_acquire(this);
    auto cbuffer2 = schedule.compositor_acquire(this);
    EXPECT_THAT(cbuffer1, Eq(cbuffer2));

    EXPECT_CALL(mock_map, send_buffer(id1));
    schedule.compositor_release(cbuffer2);
    schedule.compositor_release(cbuffer1);
    schedule.schedule_buffer(id2);
}

TEST_F(MonitorSchedule, removal_of_the_compositor_buffer_happens_after_compositor_release)
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
#endif


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
#if 0
TEST_F(MonitorSchedule, scheduling_wont_send_front_if_no_compositor_has_seen_buffer)
{
    EXPECT_CALL(mock_sink, send_buffer(_,_,_)).Times(0);
    schedule.schedule_buffer(id1);
    schedule.schedule_buffer(id2);
}
#endif

