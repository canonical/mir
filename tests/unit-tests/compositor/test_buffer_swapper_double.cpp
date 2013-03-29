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
struct BufferSwapperDouble : testing::Test
{
    BufferSwapperDouble()
    {
        auto buffer_a = std::make_shared<mtd::StubBuffer>();
        auto buffer_b = std::make_shared<mtd::StubBuffer>();

        buffer_a_addr = buffer_a.get();
        buffer_b_addr = buffer_b.get();

        auto double_list = std::initializer_list<std::shared_ptr<mc::Buffer>>{buffer_a, buffer_b};
        swapper = std::make_shared<mc::BufferSwapperMulti>(double_list);

    }

    mc::Buffer* buffer_a_addr;
    mc::Buffer* buffer_b_addr;

    std::shared_ptr<mc::BufferSwapper> swapper;
};

}

TEST_F(BufferSwapperDouble, test_valid_buffer_returned)
{
    mc::BufferID buf_tmp;
    std::shared_ptr<mc::Buffer> buffer_ref;

    swapper->client_acquire(buffer_ref, buf_tmp);
    auto addr = buffer_ref.get();
    EXPECT_TRUE((addr == buffer_a_addr) || (addr == buffer_b_addr));

    swapper->client_release(buf_tmp);
}

TEST_F(BufferSwapperDouble, test_valid_and_unique_with_two_acquires)
{
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;
    std::shared_ptr<mc::Buffer> buffer_ref_a;
    std::shared_ptr<mc::Buffer> buffer_ref_b;

    swapper->client_acquire(buffer_ref_a, buf_tmp_a);
    swapper->client_release(buf_tmp_a);

    swapper->compositor_acquire(buffer_ref_b, buf_tmp_b);
    swapper->compositor_release(buf_tmp_b);

    swapper->client_acquire(buffer_ref_b, buf_tmp_b);
    swapper->client_release(buf_tmp_b);

    EXPECT_NE(buf_tmp_a, buf_tmp_b);

    auto addr_a = buffer_ref_a.get();
    auto addr_b = buffer_ref_b.get();
    EXPECT_TRUE((addr_a == buffer_a_addr) || (addr_a == buffer_b_addr));
    EXPECT_TRUE((addr_b == buffer_a_addr) || (addr_b == buffer_b_addr));
}

TEST_F(BufferSwapperDouble, test_compositor_gets_valid)
{
    mc::BufferID buf_tmp, buf_tmp_b;
    std::shared_ptr<mc::Buffer> buffer_ref;

    swapper->client_acquire(buffer_ref, buf_tmp_b);
    swapper->client_release(buf_tmp_b);

    swapper->compositor_acquire(buffer_ref, buf_tmp);
    auto addr_a = buffer_ref.get();
    EXPECT_TRUE((addr_a == buffer_a_addr) || (addr_a == buffer_b_addr));
}

TEST_F(BufferSwapperDouble, test_compositor_gets_last_posted)
{
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;
    std::shared_ptr<mc::Buffer> buffer_ref;

    swapper->client_acquire(buffer_ref, buf_tmp_a);
    swapper->client_release(buf_tmp_a);

    swapper->compositor_acquire(buffer_ref, buf_tmp_b);
    swapper->compositor_release(buf_tmp_b);

    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
}

TEST_F(BufferSwapperDouble, test_two_grabs_without_a_client_release)
{
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;
    mc::BufferID buf_tmp_c;
    std::shared_ptr<mc::Buffer> buffer_ref;

    swapper->client_acquire(buffer_ref, buf_tmp_c);
    swapper->client_release(buf_tmp_c);

    swapper->compositor_acquire(buffer_ref, buf_tmp_b);
    swapper->compositor_release(buf_tmp_b);

    swapper->compositor_acquire(buffer_ref, buf_tmp_a);
    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
}

TEST_F(BufferSwapperDouble, test_two_grabs_with_client_updates)
{
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;
    mc::BufferID buf_tmp_c;
    std::shared_ptr<mc::Buffer> buffer_ref;

    swapper->client_acquire(buffer_ref, buf_tmp_a);
    swapper->client_release(buf_tmp_a);

    swapper->compositor_acquire(buffer_ref, buf_tmp_c);
    swapper->compositor_release(buf_tmp_c);

    swapper->client_acquire(buffer_ref, buf_tmp_c);
    swapper->client_release(buf_tmp_c);

    swapper->compositor_acquire(buffer_ref, buf_tmp_b);
    EXPECT_NE(buf_tmp_a, buf_tmp_b);
}

TEST_F(BufferSwapperDouble, test_grab_release_pattern)
{
    mc::BufferID buf_tmp_a;
    mc::BufferID buf_tmp_b;
    mc::BufferID buf_tmp_c;
    mc::BufferID buf_tmp_d;
    std::shared_ptr<mc::Buffer> buffer_ref;

    swapper->client_acquire(buffer_ref, buf_tmp_d);
    swapper->client_release(buf_tmp_d);

    swapper->compositor_acquire(buffer_ref, buf_tmp_c);
    swapper->compositor_release(buf_tmp_c);

    swapper->client_acquire(buffer_ref, buf_tmp_b);
    swapper->client_release(buf_tmp_b);

    swapper->compositor_acquire(buffer_ref, buf_tmp_a);
    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
    EXPECT_NE(buf_tmp_a, buf_tmp_c);
}
