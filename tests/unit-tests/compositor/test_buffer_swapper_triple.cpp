/*
 * Copyright © 2012 Canonical Ltd.
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
#include "mir/compositor/buffer_id.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace
{

struct BufferSwapperTriple : testing::Test
{
    BufferSwapperTriple()
    {
        auto buffer_a = std::make_shared<mtd::StubBuffer>();
        auto buffer_b = std::make_shared<mtd::StubBuffer>();
        auto buffer_c = std::make_shared<mtd::StubBuffer>();

        buffer_a_addr = buffer_a.get();
        buffer_b_addr = buffer_b.get();
        buffer_c_addr = buffer_c.get();

        auto triple_list = std::initializer_list<std::shared_ptr<mc::Buffer>>{buffer_a, buffer_b, buffer_c};
        swapper = std::make_shared<mc::BufferSwapperMulti>(triple_list);

    }

    mc::Buffer* buffer_a_addr;
    mc::Buffer* buffer_b_addr;
    mc::Buffer* buffer_c_addr;

    std::shared_ptr<mc::BufferSwapper> swapper;
};

}

TEST_F(BufferSwapperTriple, test_valid_buffer_returned)
{
    std::shared_ptr<mc::Buffer> buffer_ref;
    mc::BufferID buf_tmp;

    swapper->client_acquire(buffer_ref, buf_tmp);
    swapper->client_release(buf_tmp);

    auto addr = buffer_ref.get();
    EXPECT_TRUE((addr == buffer_a_addr) || (addr == buffer_b_addr) || (addr = buffer_c_addr));
}

TEST_F(BufferSwapperTriple, test_valid_and_unique_with_two_acquires)
{
    std::shared_ptr<mc::Buffer> buffer_ref;
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;
    mc::BufferID buf_tmp_c;

    swapper->client_acquire(buffer_ref, buf_tmp_a);
    swapper->client_release(buf_tmp_a);

    swapper->compositor_acquire(buffer_ref, buf_tmp_b);
    swapper->compositor_release(buf_tmp_b);

    swapper->client_acquire(buffer_ref, buf_tmp_b);
    swapper->client_release(buf_tmp_b);

    swapper->client_acquire(buffer_ref, buf_tmp_c);
    swapper->client_release(buf_tmp_c);

    EXPECT_NE(buf_tmp_a, buf_tmp_b);
    EXPECT_NE(buf_tmp_a, buf_tmp_c);
    EXPECT_NE(buf_tmp_b, buf_tmp_c);
}

TEST_F(BufferSwapperTriple, test_compositor_gets_valid)
{
    std::shared_ptr<mc::Buffer> buffer_ref;
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;

    swapper->client_acquire(buffer_ref, buf_tmp_b);
    swapper->client_release(buf_tmp_b);

    swapper->compositor_acquire(buffer_ref, buf_tmp_a);
}

/* this would stall a double buffer */
TEST_F(BufferSwapperTriple, test_client_can_get_two_buffers_without_compositor)
{
    std::shared_ptr<mc::Buffer> buffer_ref;
    mc::BufferID buf_tmp;

    swapper->compositor_acquire(buffer_ref, buf_tmp);

    swapper->client_acquire(buffer_ref, buf_tmp);
    swapper->client_release(buf_tmp);

    swapper->client_acquire(buffer_ref, buf_tmp);
    swapper->client_release(buf_tmp);

    SUCCEED();
}

TEST_F(BufferSwapperTriple, test_compositor_gets_last_posted_in_order)
{
    std::shared_ptr<mc::Buffer> buffer_ref;
    mc::BufferID first_comp_buffer;
    mc::BufferID first_client_buffer;
    mc::BufferID second_comp_buffer;
    mc::BufferID second_client_buffer;
    mc::BufferID third_comp_buffer;

    swapper->compositor_acquire(buffer_ref, first_comp_buffer);

    swapper->client_acquire(buffer_ref, first_client_buffer);
    swapper->client_release(first_client_buffer);

    swapper->client_acquire(buffer_ref, second_client_buffer);
    swapper->client_release(second_client_buffer);

    swapper->compositor_release(first_comp_buffer);

    swapper->compositor_acquire(buffer_ref, second_comp_buffer);
    swapper->compositor_release(second_comp_buffer);

    swapper->compositor_acquire(buffer_ref, third_comp_buffer);

    EXPECT_EQ(second_comp_buffer, first_client_buffer);
    EXPECT_EQ(third_comp_buffer, second_client_buffer);
}
