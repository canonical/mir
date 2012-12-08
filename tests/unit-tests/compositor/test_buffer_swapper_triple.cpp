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


#include "mir_test/mock_buffer.h"

#include "mir/compositor/buffer_swapper_multi.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

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
        std::unique_ptr<mc::Buffer> buffer_a(new mc::MockBuffer(size, s, pf));
        std::unique_ptr<mc::Buffer> buffer_b(new mc::MockBuffer(size, s, pf));
        std::unique_ptr<mc::Buffer> buffer_c(new mc::MockBuffer(size, s, pf));

        buf_a = buffer_a.get();
        buf_b = buffer_b.get();
        buf_c = buffer_c.get();
        swapper = std::make_shared<mc::BufferSwapperMulti>(
                std::move(buffer_a),
                std::move(buffer_b),
                std::move(buffer_c));

    }

    mc::Buffer* buf_a;
    mc::Buffer* buf_b;
    mc::Buffer* buf_c;

    std::shared_ptr<mc::BufferSwapper> swapper;
};

}

TEST_F(BufferSwapperTriple, test_valid_buffer_returned)
{
    auto buf_tmp = swapper->client_acquire();
    swapper->client_release(buf_tmp);

    EXPECT_TRUE((buf_tmp == buf_a) || (buf_tmp == buf_b) || (buf_tmp == buf_c));
}

TEST_F(BufferSwapperTriple, test_valid_and_unique_with_two_acquires)
{
    auto buf_tmp_a = swapper->client_acquire();
    swapper->client_release(buf_tmp_a);

    auto buf_tmp_b = swapper->compositor_acquire();
    swapper->compositor_release(buf_tmp_b);

    buf_tmp_b = swapper->client_acquire();
    swapper->client_release(buf_tmp_b);

    auto buf_tmp_c = swapper->client_acquire();
    swapper->client_release(buf_tmp_c);

    EXPECT_TRUE((buf_tmp_a == buf_a) || (buf_tmp_a == buf_b) || (buf_tmp_a == buf_c));
    EXPECT_TRUE((buf_tmp_b == buf_a) || (buf_tmp_b == buf_b) || (buf_tmp_b == buf_c));
    EXPECT_TRUE((buf_tmp_c == buf_a) || (buf_tmp_c == buf_b) || (buf_tmp_c == buf_c));

    EXPECT_NE(buf_tmp_a, buf_tmp_b);
    EXPECT_NE(buf_tmp_a, buf_tmp_c);
    EXPECT_NE(buf_tmp_b, buf_tmp_c);
}

TEST_F(BufferSwapperTriple, test_compositor_gets_valid)
{
    mc::Buffer* buf_tmp, *buf_tmp_b;

    buf_tmp_b = swapper->client_acquire();
    swapper->client_release(buf_tmp_b);

    buf_tmp = swapper->compositor_acquire();
    EXPECT_TRUE((buf_tmp == buf_a) || (buf_tmp == buf_b) || (buf_tmp == buf_c));
}

/* this would stall a double buffer */
TEST_F(BufferSwapperTriple, test_client_can_get_two_buffers_without_compositor)
{
    mc::Buffer* buf_tmp_c;

    swapper->compositor_acquire();

    buf_tmp_c = swapper->client_acquire();
    swapper->client_release(buf_tmp_c);

    buf_tmp_c = swapper->client_acquire();
    swapper->client_release(buf_tmp_c);

    SUCCEED();
}

/* test that the triple returns in order */
TEST_F(BufferSwapperTriple, test_compositor_gets_last_posted_in_order)
{
    auto first_comp_buffer = swapper->compositor_acquire();

    auto first_client_buffer = swapper->client_acquire();
    swapper->client_release(first_client_buffer);

    auto second_client_buffer = swapper->client_acquire();
    swapper->client_release(second_client_buffer);

    swapper->compositor_release(first_comp_buffer);

    auto second_compositor_buffer = swapper->compositor_acquire();
    swapper->compositor_release(second_compositor_buffer);
  
    auto third_compositor_buffer = swapper->compositor_acquire();

    EXPECT_EQ(second_compositor_buffer, first_client_buffer);
    EXPECT_EQ(third_compositor_buffer, second_client_buffer);
}

#if 0
TEST_F(BufferSwapperTriple, test_compositor_gets_last_posted)
{
    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;

    buf_tmp_a = swapper->client_acquire();
    swapper->client_release(buf_tmp_a);

    buf_tmp_b = swapper->compositor_acquire();
    swapper->compositor_release(buf_tmp_b);

    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
}


TEST_F(BufferSwapperTriple, test_two_grabs_without_a_client_release)
{
    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;
    mc::Buffer* buf_tmp_c;

    buf_tmp_c = swapper->client_acquire();
    swapper->client_release(buf_tmp_c);

    buf_tmp_b = swapper->compositor_acquire();
    swapper->compositor_release(buf_tmp_b);

    buf_tmp_a = swapper->compositor_acquire();
    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
}

TEST_F(BufferSwapperTriple, test_two_grabs_with_client_updates)
{
    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;
    mc::Buffer* buf_tmp_c;

    buf_tmp_a = swapper->client_acquire();
    swapper->client_release(buf_tmp_a);

    buf_tmp_c = swapper->compositor_acquire();
    swapper->compositor_release(buf_tmp_c);

    buf_tmp_c = swapper->client_acquire();
    swapper->client_release(buf_tmp_c);

    buf_tmp_b = swapper->compositor_acquire();
    EXPECT_NE(buf_tmp_a, buf_tmp_b);

}

TEST_F(BufferSwapperTriple, test_grab_release_pattern)
{
    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;
    mc::Buffer* buf_tmp_c;
    mc::Buffer* buf_tmp_d;

    buf_tmp_d = swapper->client_acquire();
    swapper->client_release(buf_tmp_d);

    buf_tmp_c = swapper->compositor_acquire();
    swapper->compositor_release(buf_tmp_c);

    buf_tmp_b = swapper->client_acquire();
    swapper->client_release(buf_tmp_b);

    buf_tmp_a = swapper->compositor_acquire();
    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
    EXPECT_NE(buf_tmp_a, buf_tmp_c);
}
#endif
