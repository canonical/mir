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
#include "src/server/compositor/multi_monitor_arbiter.h"
#include "src/server/compositor/schedule.h"
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

struct FixedSchedule : mc::Schedule
{
    void schedule(std::shared_ptr<mg::Buffer> const&)
    {
        throw std::runtime_error("this stub doesnt support this");
    }
    void cancel(std::shared_ptr<mg::Buffer> const&)
    {
        throw std::runtime_error("this stub doesnt support this");
    }
    bool anything_scheduled()
    {
        return current != sched.size();
    }
    std::shared_ptr<mg::Buffer> next_buffer()
    {
        if (sched.empty() || current == sched.size())
            throw std::runtime_error("no buffer scheduled");
        return sched[current++];
    }
    void set_schedule(std::vector<std::shared_ptr<mg::Buffer>> s)
    {
        current = 0;
        sched = s;
    }
private:
    unsigned int current{0};
    std::vector<std::shared_ptr<mg::Buffer>> sched;
};

struct MultiMonitorArbiter : Test
{
    MultiMonitorArbiter()
    {
        for(auto i = 0u; i < num_buffers; i++)
            buffers.emplace_back(std::make_shared<mtd::StubBuffer>());
    }
    unsigned int const num_buffers{6u};
    std::vector<std::shared_ptr<mg::Buffer>> buffers;
    NiceMock<MockBufferMap> mock_map;
    FixedSchedule schedule;
    mc::MultiMonitorArbiter arbiter{mt::fake_shared(mock_map), mt::fake_shared(schedule)};
};
}

TEST_F(MultiMonitorArbiter, compositor_access_before_any_submission_throws)
{
    //nothing owned
    EXPECT_THROW({
        arbiter.compositor_acquire(this);
    }, std::logic_error);

    schedule.set_schedule({buffers[0]});

    //something scheduled, should be ok
    arbiter.compositor_acquire(this);
}

TEST_F(MultiMonitorArbiter, compositor_access)
{
    schedule.set_schedule({buffers[0]});
    auto cbuffer = arbiter.compositor_acquire(this);
    EXPECT_THAT(cbuffer, Eq(buffers[0]));
}

TEST_F(MultiMonitorArbiter, compositor_release_sends_buffer_back)
{
    EXPECT_CALL(mock_map, send_buffer(buffers[0]->id()));

    schedule.set_schedule({buffers[0]});

    auto cbuffer = arbiter.compositor_acquire(this);
    schedule.set_schedule({buffers[1]});
    arbiter.compositor_release(cbuffer);
}

TEST_F(MultiMonitorArbiter, compositor_can_acquire_different_buffers_if_submission_happens)
{
    EXPECT_CALL(mock_map, send_buffer(buffers[0]->id()));

    schedule.set_schedule({buffers[0]});
    auto cbuffer1 = arbiter.compositor_acquire(this);
    schedule.set_schedule({buffers[1]});
    auto cbuffer2 = arbiter.compositor_acquire(this);
    EXPECT_THAT(cbuffer1, Ne(cbuffer2));
    arbiter.compositor_release(cbuffer2);
    arbiter.compositor_release(cbuffer1);
}

TEST_F(MultiMonitorArbiter, compositor_can_acquire_a_few_times_and_only_sends_on_the_last_release)
{
    schedule.set_schedule({buffers[0]});
    auto cbuffer1 = arbiter.compositor_acquire(this);
    auto cbuffer2 = arbiter.compositor_acquire(this);
    EXPECT_THAT(cbuffer1, Eq(cbuffer2));
    EXPECT_CALL(mock_map, send_buffer(buffers[0]->id())).Times(Exactly(1));
    schedule.set_schedule({buffers[1]});
    arbiter.compositor_release(cbuffer2);
    arbiter.compositor_release(cbuffer1);
}

TEST_F(MultiMonitorArbiter, compositor_buffer_syncs_to_fastest_compositor)
{
    int comp_id1{0};
    int comp_id2{0};

    schedule.set_schedule({buffers[0]});
    auto cbuffer1 = arbiter.compositor_acquire(&comp_id1); 
    auto cbuffer2 = arbiter.compositor_acquire(&comp_id2);

    schedule.set_schedule({buffers[1]});
    auto cbuffer3 = arbiter.compositor_acquire(&comp_id1);

    schedule.set_schedule({buffers[0]});
    auto cbuffer4 = arbiter.compositor_acquire(&comp_id1); 
    auto cbuffer5 = arbiter.compositor_acquire(&comp_id2);

    schedule.set_schedule({buffers[1]});
    auto cbuffer6 = arbiter.compositor_acquire(&comp_id2);
    auto cbuffer7 = arbiter.compositor_acquire(&comp_id2);

    EXPECT_THAT(cbuffer1, Eq(buffers[0]));
    EXPECT_THAT(cbuffer2, Eq(buffers[0]));
    EXPECT_THAT(cbuffer3, Eq(buffers[1]));
    EXPECT_THAT(cbuffer4, Eq(buffers[0]));
    EXPECT_THAT(cbuffer5, Eq(buffers[0]));
    EXPECT_THAT(cbuffer6, Eq(buffers[1]));
    EXPECT_THAT(cbuffer7, Eq(buffers[1]));
}

TEST_F(MultiMonitorArbiter, compositor_consumes_all_buffers_when_operating_as_a_composited_scene_would)
{
    schedule.set_schedule({buffers[0],buffers[1],buffers[2],buffers[3],buffers[4]});

    auto cbuffer1 = arbiter.compositor_acquire(this);
    arbiter.compositor_release(cbuffer1);
    auto cbuffer2 = arbiter.compositor_acquire(this);
    arbiter.compositor_release(cbuffer2);
    auto cbuffer3 = arbiter.compositor_acquire(this);
    arbiter.compositor_release(cbuffer3);
    auto cbuffer4 = arbiter.compositor_acquire(this);
    arbiter.compositor_release(cbuffer4);
    auto cbuffer5 = arbiter.compositor_acquire(this);
    arbiter.compositor_release(cbuffer5);

    EXPECT_THAT(cbuffer1, Eq(buffers[0]));
    EXPECT_THAT(cbuffer2, Eq(buffers[1]));
    EXPECT_THAT(cbuffer3, Eq(buffers[2]));
    EXPECT_THAT(cbuffer4, Eq(buffers[3]));
    EXPECT_THAT(cbuffer5, Eq(buffers[4]));
}

TEST_F(MultiMonitorArbiter, compositor_consumes_all_buffers_when_operating_as_a_bypassed_buffer_would)
{
    schedule.set_schedule({buffers[0],buffers[1],buffers[2],buffers[3],buffers[4]});

    auto cbuffer1 = arbiter.compositor_acquire(this);
    auto cbuffer2 = arbiter.compositor_acquire(this);
    arbiter.compositor_release(cbuffer1);
    auto cbuffer3 = arbiter.compositor_acquire(this);
    arbiter.compositor_release(cbuffer2);
    auto cbuffer4 = arbiter.compositor_acquire(this);
    arbiter.compositor_release(cbuffer3);
    auto cbuffer5 = arbiter.compositor_acquire(this);
    arbiter.compositor_release(cbuffer4);
    arbiter.compositor_release(cbuffer5);

    EXPECT_THAT(cbuffer1, Eq(buffers[0]));
    EXPECT_THAT(cbuffer2, Eq(buffers[1]));
    EXPECT_THAT(cbuffer3, Eq(buffers[2]));
    EXPECT_THAT(cbuffer4, Eq(buffers[3]));
    EXPECT_THAT(cbuffer5, Eq(buffers[4]));
}

TEST_F(MultiMonitorArbiter, multimonitor_compositor_buffer_syncs_to_fastest_with_more_queueing)
{
    int comp_id1{0};
    int comp_id2{0};

    schedule.set_schedule({buffers[0],buffers[1],buffers[2],buffers[3],buffers[4]});

    auto cbuffer1 = arbiter.compositor_acquire(&comp_id1); //buffer[0]
    auto cbuffer2 = arbiter.compositor_acquire(&comp_id2); //buffer[0]

    auto cbuffer3 = arbiter.compositor_acquire(&comp_id1); //buffer[1]

    auto cbuffer4 = arbiter.compositor_acquire(&comp_id1); //buffer[2]
    auto cbuffer5 = arbiter.compositor_acquire(&comp_id2); //buffer[2]

    auto cbuffer6 = arbiter.compositor_acquire(&comp_id2); //buffer[3]

    auto cbuffer7 = arbiter.compositor_acquire(&comp_id2); //buffer[4]
    auto cbuffer8 = arbiter.compositor_acquire(&comp_id1); //buffer[4]

    EXPECT_THAT(cbuffer1, Eq(buffers[0]));
    EXPECT_THAT(cbuffer2, Eq(buffers[0]));

    EXPECT_THAT(cbuffer3, Eq(buffers[1]));

    EXPECT_THAT(cbuffer4, Eq(buffers[2]));
    EXPECT_THAT(cbuffer5, Eq(buffers[2]));

    EXPECT_THAT(cbuffer6, Eq(buffers[3]));

    EXPECT_THAT(cbuffer7, Eq(buffers[4]));
    EXPECT_THAT(cbuffer8, Eq(buffers[4]));
}

TEST_F(MultiMonitorArbiter, can_set_a_new_schedule)
{
    FixedSchedule another_schedule;
    schedule.set_schedule({buffers[3],buffers[4]});
    another_schedule.set_schedule({buffers[0],buffers[1]});

    auto cbuffer1 = arbiter.compositor_acquire(this);
    arbiter.set_schedule(mt::fake_shared(another_schedule));
    auto cbuffer2 = arbiter.compositor_acquire(this);

    EXPECT_THAT(cbuffer1, Eq(buffers[3]));
    EXPECT_THAT(cbuffer2, Eq(buffers[0])); 
}
