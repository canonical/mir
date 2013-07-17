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

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace
{

struct BufferSwapperTriple : testing::Test
{
    void SetUp()
    {
        buffer_a = std::make_shared<mtd::StubBuffer>();
        buffer_b = std::make_shared<mtd::StubBuffer>();
        buffer_c = std::make_shared<mtd::StubBuffer>();

        auto triple_list = std::vector<std::shared_ptr<mg::Buffer>>{buffer_a, buffer_b, buffer_c};
        swapper = std::make_shared<mc::BufferSwapperMulti>(triple_list, triple_list.size());

    }

    std::shared_ptr<mg::Buffer> buffer_a;
    std::shared_ptr<mg::Buffer> buffer_b;
    std::shared_ptr<mg::Buffer> buffer_c;

    std::shared_ptr<mc::BufferSwapper> swapper;
};

}

TEST_F(BufferSwapperTriple, test_valid_buffer_returned)
{
    auto buf = swapper->client_acquire();

    EXPECT_TRUE((buf == buffer_a) || (buf == buffer_b) || (buf = buffer_c));
}

TEST_F(BufferSwapperTriple, test_valid_and_unique_with_two_acquires)
{
    auto buffer_1 = swapper->client_acquire();
    swapper->client_release(buffer_1);

    //just so one thread operation is ok
    auto buffer_tmp = swapper->compositor_acquire();
    swapper->compositor_release(buffer_tmp);

    auto buffer_2 = swapper->client_acquire();
    swapper->client_release(buffer_2);

    auto buffer_3 = swapper->client_acquire();
    swapper->client_release(buffer_3);

    EXPECT_NE(buffer_1, buffer_2);
    EXPECT_NE(buffer_1, buffer_3);
    EXPECT_NE(buffer_2, buffer_3);
}

TEST_F(BufferSwapperTriple, test_compositor_gets_clients_last_buffer)
{
    auto buffer_a = swapper->client_acquire();
    swapper->client_release(buffer_a);

    auto comp_buffer = swapper->compositor_acquire();
    EXPECT_EQ(buffer_a, comp_buffer);
}

/* this would stall a double buffer */
TEST_F(BufferSwapperTriple, test_client_can_get_two_buffers_without_compositor_consuming_two_buffers)
{
    auto buffer_1 = swapper->compositor_acquire();

    auto buffer_2 = swapper->client_acquire();
    swapper->client_release(buffer_2);

    auto buffer_3 = swapper->client_acquire();
    swapper->client_release(buffer_3);

    EXPECT_NE(buffer_2, buffer_3);
}

TEST_F(BufferSwapperTriple, test_compositor_gets_last_posted_in_order)
{
    auto first_comp_buffer = swapper->compositor_acquire();

    auto first_client_buffer = swapper->client_acquire();
    swapper->client_release(first_client_buffer);

    auto second_client_buffer = swapper->client_acquire();
    swapper->client_release(second_client_buffer);

    swapper->compositor_release(first_comp_buffer);

    auto second_comp_buffer = swapper->compositor_acquire();
    swapper->compositor_release(second_comp_buffer);

    auto third_comp_buffer = swapper->compositor_acquire();

    EXPECT_EQ(second_comp_buffer, first_client_buffer);
    EXPECT_EQ(third_comp_buffer, second_client_buffer);
}
