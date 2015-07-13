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
#include "mir/frontend/client_buffers.h"

#include <gtest/gtest.h>
using namespace testing;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mf = mir::frontend;

namespace
{
struct MockBufferMap : mf::ClientBuffers
{
    MOCK_METHOD1(add_buffer, void(mg::BufferProperties const&));
    MOCK_METHOD1(remove_buffer, void(mg::BufferID id));
    MOCK_METHOD1(send_buffer, void(mg::BufferID id));
    MOCK_METHOD1(at, std::shared_ptr<mg::Buffer>&(mg::BufferID));
    std::shared_ptr<mg::Buffer>& operator[](mg::BufferID id) { return at(id); }
};

struct ConsumptionArbiter : Test
{
    ConsumptionArbiter()
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
    mc::MultiMonitorArbiter schedule{
        mt::fake_shared(mock_map), std::make_unique<mc::QueueingSchedule>()};
};
}

TEST_F(ConsumptionArbiter, compositor_access_before_any_submission_throws)
{
    //nothing owned
    EXPECT_THROW({
        schedule.compositor_acquire(this);
    }, std::logic_error);

    schedule.schedule_buffer(ids[0]);

    //something scheduled, should be ok
    schedule.compositor_acquire(this);
}

TEST_F(ConsumptionArbiter, compositor_access)
{
    schedule.schedule_buffer(ids[0]);
    auto cbuffer = schedule.compositor_acquire(this);
    EXPECT_THAT(cbuffer->id(), Eq(ids[0]));
}

TEST_F(ConsumptionArbiter, compositor_release_sends_buffer_back)
{
    EXPECT_CALL(mock_map, send_buffer(ids[0]));

    schedule.schedule_buffer(ids[0]);

    auto cbuffer = schedule.compositor_acquire(this);
    schedule.schedule_buffer(ids[1]);
    schedule.compositor_release(cbuffer);
}

TEST_F(ConsumptionArbiter, compositor_can_acquire_different_buffers_if_submission_happens)
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

TEST_F(ConsumptionArbiter, compositor_can_acquire_a_few_times_and_only_sends_on_the_last_release)
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

TEST_F(ConsumptionArbiter, compositor_buffer_syncs_to_fastest_compositor)
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

TEST_F(ConsumptionArbiter, compositor_consumes_all_buffers_when_operating_as_a_composited_scene_would)
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

TEST_F(ConsumptionArbiter, compositor_consumes_all_buffers_when_operating_as_a_bypassed_buffer_would)
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

TEST_F(ConsumptionArbiter, multimonitor_compositor_buffer_syncs_to_fastest_with_more_queueing)
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
TEST_F(ConsumptionArbiter, scheduling_can_send_buffer_if_framedropping_and_no_compositors_still_have_it)
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

TEST_F(ConsumptionArbiter, removal_of_the_compositor_buffer_happens_after_compositor_release)
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
TEST_F(MultiMonitorArbiter, submitted_buffer_can_be_ejected)
{
    schedule.schedule_buffer(buffer2);
    schedule.schedule_buffer(buffer1);
    schedule.eject(buffer1);

    EXPECT_THAT(schedule.compositor_acquire(id), Eq(buffer2));
}
#endif
#if 0
TEST_F(ConsumptionArbiter, scheduling_wont_send_front_if_no_compositor_has_seen_buffer)
{
    EXPECT_CALL(mock_sink, send_buffer(_,_,_)).Times(0);
    schedule.schedule_buffer(id1);
    schedule.schedule_buffer(id2);
}
#endif

