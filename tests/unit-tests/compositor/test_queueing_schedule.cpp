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

#include "src/server/compositor/queueing_schedule.h"
#include "mir/test/doubles/stub_buffer.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace mc = mir::compositor;

namespace
{
struct QueueingSchedule : Test
{
    QueueingSchedule()
    {
        for(auto i = 0u; i < num_buffers; i++)
            buffers.emplace_back(std::make_shared<mtd::StubBuffer>());
    }
    unsigned int const num_buffers{5};
    std::vector<std::shared_ptr<mg::Buffer>> buffers;

    mc::QueueingSchedule schedule;
    std::vector<std::shared_ptr<mg::Buffer>> drain_queue()
    {
        std::vector<std::shared_ptr<mg::Buffer>> scheduled_buffers;
        while(schedule.anything_scheduled())
            scheduled_buffers.emplace_back(schedule.next_buffer());
        return scheduled_buffers;
    }
};
}

TEST_F(QueueingSchedule, throws_if_no_buffers)
{
    EXPECT_FALSE(schedule.anything_scheduled());
    EXPECT_THROW({
        schedule.next_buffer();
    }, std::logic_error);
}

TEST_F(QueueingSchedule, queues_buffers_up)
{
    EXPECT_FALSE(schedule.anything_scheduled());

    std::vector<std::shared_ptr<mg::Buffer>> scheduled_buffers {
        buffers[1], buffers[3], buffers[0], buffers[2], buffers[4]
    };

    for (auto& buffer : scheduled_buffers)
        schedule.schedule(buffer);

    EXPECT_TRUE(schedule.anything_scheduled());

    EXPECT_THAT(drain_queue(), ContainerEq(scheduled_buffers));

    EXPECT_FALSE(schedule.anything_scheduled());
}

TEST_F(QueueingSchedule, queuing_the_same_buffer_moves_it_to_front_of_queue)
{
    for(auto i = 0u; i < num_buffers; i++)
        schedule.schedule(buffers[i]);
    schedule.schedule(buffers[0]);

    EXPECT_THAT(drain_queue(),
        ElementsAre(buffers[1], buffers[2], buffers[3], buffers[4], buffers[0]));
}
