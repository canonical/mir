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

struct BufferSwapperDouble : testing::Test
{
    BufferSwapperDouble()
    {
        std::shared_ptr<mc::Buffer> buffer_a(new mtd::MockBuffer(size, s, pf));
        std::shared_ptr<mc::Buffer> buffer_b(new mtd::MockBuffer(size, s, pf));

        buffer_id_a = mc::BufferID(8);
        buffer_id_b = mc::BufferID(9);
        buffer_a_addr = buffer_a.get();
        buffer_b_addr = buffer_b.get();
        swapper = std::make_shared<mc::BufferSwapperMulti>(buffer_a, buffer_id_a, buffer_b, buffer_id_b);

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
    mc::BufferID buffer_id_a;
    mc::BufferID buffer_id_b;

    std::shared_ptr<mc::BufferSwapper> swapper;
};

}

TEST_F(BufferSwapperDouble, test_valid_buffer_returned)
{
    mc::BufferID buf_tmp;
    std::weak_ptr<mc::Buffer> buffer_ref;

    swapper->client_acquire(buffer_ref, buf_tmp);
    EXPECT_TRUE((buf_tmp == buffer_id_a) || (buf_tmp == buffer_id_b));
    EXPECT_TRUE(check_ref_to_id(buf_tmp, buffer_ref));

    swapper->client_release(buf_tmp);
}

TEST_F(BufferSwapperDouble, test_valid_and_unique_with_two_acquires)
{
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;
    std::weak_ptr<mc::Buffer> buffer_ref_a;
    std::weak_ptr<mc::Buffer> buffer_ref_b;

    swapper->client_acquire(buffer_ref_a, buf_tmp_a);
    swapper->client_release(buf_tmp_a);

    swapper->client_acquire(buffer_ref_b, buf_tmp_b);
    swapper->compositor_release(buf_tmp_b);

    swapper->client_acquire(buffer_ref_b, buf_tmp_b);
    swapper->client_release(buf_tmp_b);

    EXPECT_TRUE((buf_tmp_a == buffer_id_a) || (buf_tmp_a == buffer_id_b));
    EXPECT_TRUE((buf_tmp_b == buffer_id_a) || (buf_tmp_b == buffer_id_b));
    EXPECT_NE(buf_tmp_a, buf_tmp_b);


    EXPECT_TRUE(check_ref_to_id(buf_tmp_a, buffer_ref_a));
    EXPECT_TRUE(check_ref_to_id(buf_tmp_b, buffer_ref_b));
}
#if 0
TEST_F(BufferSwapperDouble, test_compositor_gets_valid)
{
    mc::BufferID buf_tmp, *buf_tmp_b;
    std::weak_ptr<mc::Buffer> buffer_ref;

    buf_tmp_b = swapper->client_acquire(buffer_ref);
    swapper->client_release(buf_tmp_b);

    buf_tmp = swapper->compositor_acquire(buffer_ref);
    EXPECT_TRUE((buf_tmp == buffer_id_a) || (buf_tmp == buffer_id_b)); /* we should get valid buffer we supplied in constructor */
}

TEST_F(BufferSwapperDouble, test_compositor_gets_last_posted)
{
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;
    std::weak_ptr<mc::Buffer> buffer_ref;

    buf_tmp_a = swapper->client_acquire(buffer_ref);
    swapper->client_release(buf_tmp_a);

    buf_tmp_b = swapper->compositor_acquire(buffer_ref);
    swapper->compositor_release(buf_tmp_b);

    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
}


TEST_F(BufferSwapperDouble, test_two_grabs_without_a_client_release)
{
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;
    mc::BufferID buf_tmp_c;
    std::weak_ptr<mc::Buffer> buffer_ref;

    buf_tmp_c = swapper->client_acquire(buffer_ref);
    swapper->client_release(buf_tmp_c);

    buf_tmp_b = swapper->compositor_acquire(buffer_ref);
    swapper->compositor_release(buf_tmp_b);

    buf_tmp_a = swapper->compositor_acquire(buffer_ref);
    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
}

TEST_F(BufferSwapperDouble, test_two_grabs_with_client_updates)
{
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;
    mc::BufferID buf_tmp_c;
    std::weak_ptr<mc::Buffer> buffer_ref;

    buf_tmp_a = swapper->client_acquire(buffer_ref);
    swapper->client_release(buf_tmp_a);

    buf_tmp_c = swapper->compositor_acquire(buffer_ref);
    swapper->compositor_release(buf_tmp_c);

    buf_tmp_c = swapper->client_acquire(buffer_ref);
    swapper->client_release(buf_tmp_c);

    buf_tmp_b = swapper->compositor_acquire(buffer_ref);
    EXPECT_NE(buf_tmp_a, buf_tmp_b);

}

TEST_F(BufferSwapperDouble, test_grab_release_pattern)
{
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;
    mc::BufferID buf_tmp_c;
    mc::BufferID buf_tmp_d;
    std::weak_ptr<mc::Buffer> buffer_ref;

    buf_tmp_d = swapper->client_acquire(buffer_ref);
    swapper->client_release(buf_tmp_d);

    buf_tmp_c = swapper->compositor_acquire(buffer_ref);
    swapper->compositor_release(buf_tmp_c);

    buf_tmp_b = swapper->client_acquire(buffer_ref);
    swapper->client_release(buf_tmp_b);

    buf_tmp_a = swapper->compositor_acquire(buffer_ref);
    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
    EXPECT_NE(buf_tmp_a, buf_tmp_c);
}
#endif
