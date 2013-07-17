/*
 * Copyright © 2012, 2013 Canonical Ltd.
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


#include "mir_test_doubles/stub_buffer.h"

#include "mir/compositor/buffer_swapper_multi.h"
#include "mir/graphics/buffer_id.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <future>
#include <thread>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{
struct BufferSwapperDouble : testing::Test
{
    void SetUp()
    {
        buffer_a = std::make_shared<mtd::StubBuffer>();
        buffer_b = std::make_shared<mtd::StubBuffer>();

        auto double_list = std::vector<std::shared_ptr<mg::Buffer>>{buffer_a, buffer_b};
        swapper = std::make_shared<mc::BufferSwapperMulti>(double_list, double_list.size());

    }

    std::shared_ptr<mg::Buffer> buffer_a;
    std::shared_ptr<mg::Buffer> buffer_b;

    std::shared_ptr<mc::BufferSwapper> swapper;
};

}

TEST_F(BufferSwapperDouble, test_valid_buffer_returned)
{
    auto buffer = swapper->client_acquire();

    EXPECT_TRUE((buffer == buffer_a) || (buffer == buffer_b));
}

TEST_F(BufferSwapperDouble, test_valid_and_unique_with_two_acquires)
{
    auto buffer_1 = swapper->client_acquire();
    swapper->client_release(buffer_1);

    auto buffer_tmp = swapper->compositor_acquire();
    swapper->compositor_release(buffer_tmp);

    auto buffer_2 = swapper->client_acquire();
    swapper->client_release(buffer_2);

    EXPECT_NE(buffer_1, buffer_2);

    EXPECT_TRUE((buffer_1 == buffer_a) || (buffer_1 == buffer_b));
    EXPECT_TRUE((buffer_2 == buffer_a) || (buffer_2 == buffer_b));
}

TEST_F(BufferSwapperDouble, test_compositor_gets_valid)
{
    auto buffer = swapper->compositor_acquire();
    EXPECT_TRUE((buffer == buffer_a) || (buffer == buffer_b));
}

TEST_F(BufferSwapperDouble, test_compositor_gets_last_posted)
{
    auto buffer_1 = swapper->client_acquire();
    swapper->client_release(buffer_1);

    auto buffer_2 = swapper->compositor_acquire();
    swapper->compositor_release(buffer_2);

    EXPECT_EQ(buffer_1, buffer_2);
}

TEST_F(BufferSwapperDouble, test_two_grabs_without_a_client_release)
{
    swapper->client_acquire();

    auto buffer_1 = swapper->compositor_acquire();
    swapper->compositor_release(buffer_1);

    auto buffer_2 = swapper->compositor_acquire();
    EXPECT_EQ(buffer_1, buffer_2);
}

TEST_F(BufferSwapperDouble, test_client_and_compositor_advance_buffers_in_serial_pattern)
{
    auto buffer_1 = swapper->client_acquire();
    swapper->client_release(buffer_1);

    auto buffer_2 = swapper->compositor_acquire();
    swapper->compositor_release(buffer_2);

    auto buffer_3 = swapper->client_acquire();
    swapper->client_release(buffer_3);

    auto buffer_4 = swapper->compositor_acquire();

    EXPECT_NE(buffer_1, buffer_3);
    EXPECT_NE(buffer_2, buffer_4);
}

TEST_F(BufferSwapperDouble, force_client_abort_works_causes_rc_failure)
{
    auto client_buffer = swapper->client_acquire();
    swapper->client_release(client_buffer);

    client_buffer = swapper->client_acquire();
    swapper->client_release(client_buffer);

    /*
     * Unblock the the upcoming client_acquire() call. There is no guarantee
     * that force_client_abort will run after client_acquire() is
     * blocked, but it doesn't matter. As long as the client uses only one
     * buffer at a time, BufferSwapperMulti::force_client_abort() ensures
     * that either a currently blocked request will unblock, or the next
     * request will not block.
     */
    auto f = std::async(std::launch::async,
                [this]
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds{10});
                    swapper->force_client_abort();
                });

    EXPECT_THROW({
        swapper->client_acquire();
    }, std::runtime_error);
}

TEST_F(BufferSwapperDouble, force_client_abort_works)
{
    swapper->client_acquire();

    auto f = std::async(std::launch::async,
                [this]
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds{10});
                    swapper->force_client_abort();
                });

    EXPECT_THROW({
        swapper->client_acquire();
    }, std::runtime_error);
}

TEST_F(BufferSwapperDouble, force_requests_to_complete_works)
{
    auto client_buffer = swapper->client_acquire();
    swapper->client_release(client_buffer);

    /*
     * Unblock the the upcoming client_acquire() call. There is no guarantee
     * that force_requests_to_complete will run after client_acquire() is
     * blocked, but it doesn't matter. As long as the client uses only one
     * buffer at a time, BufferSwapperMulti::force_requests_to_complete() ensures
     * that either a currently blocked request will unblock, or the next
     * request will not block.
     */
    auto f = std::async(std::launch::async,
                 [this]
                 {
                     std::this_thread::sleep_for(std::chrono::milliseconds{10});
                     swapper->force_requests_to_complete();
                 });

    client_buffer = swapper->client_acquire();

    EXPECT_NE(nullptr, client_buffer);
}

TEST_F(BufferSwapperDouble, force_requests_to_complete_throws_if_all_buffers_are_acquired)
{
    auto client_buffer = swapper->client_acquire();
    swapper->client_release(client_buffer);

    client_buffer = swapper->client_acquire();
    auto comp_buffer = swapper->compositor_acquire();

    EXPECT_THROW({
        swapper->force_requests_to_complete();
    }, std::logic_error);
}
