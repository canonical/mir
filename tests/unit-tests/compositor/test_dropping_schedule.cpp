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

#include "src/server/compositor/dropping_schedule.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/fake_shared.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace
{

struct DroppingSchedule : Test
{
    DroppingSchedule()
    {
        for(auto i = 0u; i < num_buffers; i++)
            buffers.emplace_back(std::make_shared<mtd::StubBuffer>());
    }
    unsigned int const num_buffers{5};
    std::vector<std::shared_ptr<mg::Buffer>> buffers;

    mc::DroppingSchedule schedule;
    std::vector<std::shared_ptr<mg::Buffer>> drain_queue()
    {
        std::vector<std::shared_ptr<mg::Buffer>> scheduled_buffers;
        while(schedule.num_scheduled())
            scheduled_buffers.emplace_back(schedule.next_buffer());
        return scheduled_buffers;
    }
};
}

TEST_F(DroppingSchedule, throws_if_no_buffers)
{
    EXPECT_FALSE(schedule.num_scheduled());
    EXPECT_THROW({
        schedule.next_buffer();
    }, std::logic_error);
}

TEST_F(DroppingSchedule, drops_excess_buffers)
{
    for(auto i = 0u; i < num_buffers; i++)
        schedule.schedule(buffers[i]);

    auto queue = drain_queue();
    ASSERT_THAT(queue, SizeIs(1));

    // The 5th buffer should be scheduled...
    EXPECT_THAT(queue[0]->id(), Eq(buffers[4]->id()));
    for (int i = 0; i < 4 ; ++i)
    {
        // ...and all the others should have no external references
        EXPECT_TRUE(buffers[i].unique());
    }
}

TEST_F(DroppingSchedule, queueing_same_buffer_many_times_doesnt_drop)
{
    schedule.schedule(buffers[2]);
    schedule.schedule(buffers[2]);
    schedule.schedule(buffers[2]);

    auto queue = drain_queue();
    ASSERT_THAT(queue, SizeIs(1));
    EXPECT_THAT(queue[0]->id(), Eq(buffers[2]->id()));
}
