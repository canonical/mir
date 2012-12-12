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

#include "mir_test_doubles/mock_buffer.h"

#include "mir/compositor/buffer_swapper_multi.h"
#include "mir/compositor/buffer_id.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace
{
geom::Size size {geom::Width{1024}, geom::Height{768}};
geom::Height h {768};
geom::Stride s {1024};
geom::PixelFormat pf {geom::PixelFormat::rgba_8888};

struct BufferSwapperTriple : testing::Test
{
    BufferSwapperTriple()
    {
        std::shared_ptr<mc::Buffer> buffer_a(new mtd::MockBuffer(size, s, pf));
        std::shared_ptr<mc::Buffer> buffer_b(new mtd::MockBuffer(size, s, pf));
        std::shared_ptr<mc::Buffer> buffer_c(new mtd::MockBuffer(size, s, pf));

        buffer_id_a = mc::BufferID(7);
        buffer_id_b = mc::BufferID(8);
        buffer_id_c = mc::BufferID(9);
        buffer_a_addr = buffer_a.get();
        buffer_b_addr = buffer_b.get();
        buffer_c_addr = buffer_c.get();
        swapper = std::make_shared<mc::BufferSwapperMulti>(buffer_a, buffer_id_a, buffer_b, buffer_id_b, buffer_c, buffer_id_c);

    }

    bool check_ref_to_id(mc::BufferID id, std::weak_ptr<mc::Buffer> buffer)
    {
        if (id == buffer_id_a)
        {
            return ( buffer.lock().get() == buffer_a_addr);
        }

        if (id == buffer_id_b)
        {
            return ( buffer.lock().get() == buffer_b_addr);
        }
        return false;
    }

    mc::Buffer* buffer_a_addr;
    mc::Buffer* buffer_b_addr;
    mc::Buffer* buffer_c_addr;
    mc::BufferID buffer_id_a;
    mc::BufferID buffer_id_b;
    mc::BufferID buffer_id_c;

    std::shared_ptr<mc::BufferSwapper> swapper;
};

}

TEST_F(BufferSwapperTriple, test_valid_buffer_returned)
{
    std::weak_ptr<mc::Buffer> buffer_ref;
    mc::BufferID buf_tmp;

    swapper->client_acquire(buffer_ref, buf_tmp);
    swapper->client_release(buf_tmp);

    EXPECT_TRUE((buf_tmp == buffer_id_a) || (buf_tmp == buffer_id_b) || (buf_tmp == buffer_id_c));
    EXPECT_TRUE(check_ref_to_id(buf_tmp, buffer_ref));
}

TEST_F(BufferSwapperTriple, test_valid_and_unique_with_two_acquires)
{
    std::weak_ptr<mc::Buffer> buffer_ref;
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

    EXPECT_TRUE((buf_tmp_a == buffer_id_a) || (buf_tmp_a == buffer_id_b) || (buf_tmp_a == buffer_id_c));
    EXPECT_TRUE((buf_tmp_b == buffer_id_a) || (buf_tmp_b == buffer_id_b) || (buf_tmp_b == buffer_id_c));
    EXPECT_TRUE((buf_tmp_c == buffer_id_a) || (buf_tmp_c == buffer_id_b) || (buf_tmp_c == buffer_id_c));

    EXPECT_NE(buf_tmp_a, buf_tmp_b);
    EXPECT_NE(buf_tmp_a, buf_tmp_c);
    EXPECT_NE(buf_tmp_b, buf_tmp_c);
}

TEST_F(BufferSwapperTriple, test_compositor_gets_valid)
{
    std::weak_ptr<mc::Buffer> buffer_ref;
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;

    swapper->client_acquire(buffer_ref, buf_tmp_b);
    swapper->client_release(buf_tmp_b);

    swapper->compositor_acquire(buffer_ref, buf_tmp_a);
    EXPECT_TRUE((buf_tmp_a == buffer_id_a) || (buf_tmp_a == buffer_id_b) || (buf_tmp_a == buffer_id_c));
}

/* this would stall a double buffer */
TEST_F(BufferSwapperTriple, test_client_can_get_two_buffers_without_compositor)
{
    std::weak_ptr<mc::Buffer> buffer_ref;
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
    std::weak_ptr<mc::Buffer> buffer_ref;
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
