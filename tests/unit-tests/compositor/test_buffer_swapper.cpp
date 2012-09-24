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

#include "mir/compositor/buffer_swapper_double.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{
geom::Size size {geom::Width{1024}, geom::Height{768}};
geom::Height h {768};
geom::Stride s {1024};
mc::PixelFormat pf {mc::PixelFormat::rgba_8888};

struct BufferSwapper : testing::Test
{
    BufferSwapper()
    {
        std::unique_ptr<mc::Buffer> buffer_a(new mc::MockBuffer(size, s, pf));
        std::unique_ptr<mc::Buffer> buffer_b(new mc::MockBuffer(size, s, pf));

        buf_a = buffer_a.get();
        buf_b = buffer_b.get();
        swapper = std::make_shared<mc::BufferSwapperDouble>(
                std::move(buffer_a),
                std::move(buffer_b));

    }

    mc::Buffer* buf_a;
    mc::Buffer* buf_b;

    std::shared_ptr<mc::BufferSwapper> swapper;
};

}

TEST_F(BufferSwapper, test_valid_buffer_returned)
{
    mc::Buffer* buf_tmp;

    buf_tmp = swapper->client_acquire();
    EXPECT_TRUE((buf_tmp == buf_a) || (buf_tmp == buf_b));

    swapper->client_release(buf_tmp);
}

TEST_F(BufferSwapper, test_valid_and_unique_with_two_acquires)
{
    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;

    buf_tmp_a = swapper->client_acquire();
    swapper->client_release(buf_tmp_a);

    buf_tmp_b = swapper->compositor_acquire();
    swapper->compositor_release(buf_tmp_b);

    buf_tmp_b = swapper->client_acquire();
    swapper->client_release(buf_tmp_b);

    EXPECT_TRUE((buf_tmp_a == buf_a) || (buf_tmp_a == buf_b));
    EXPECT_TRUE((buf_tmp_b == buf_a) || (buf_tmp_b == buf_b));
    EXPECT_NE(buf_tmp_a, buf_tmp_b);
}

TEST_F(BufferSwapper, test_compositor_gets_valid)
{
    mc::Buffer* buf_tmp, *buf_tmp_b;

    buf_tmp_b = swapper->client_acquire();
    swapper->client_release(buf_tmp_b);

    buf_tmp = swapper->compositor_acquire();
    EXPECT_TRUE((buf_tmp == buf_a) || (buf_tmp == buf_b)); /* we should get valid buffer we supplied in constructor */
}

TEST_F(BufferSwapper, test_compositor_gets_last_posted)
{
    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;

    buf_tmp_a = swapper->client_acquire();
    swapper->client_release(buf_tmp_a);

    buf_tmp_b = swapper->compositor_acquire();
    swapper->compositor_release(buf_tmp_b);

    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
}


TEST_F(BufferSwapper, test_two_grabs_without_a_client_release)
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

TEST_F(BufferSwapper, test_two_grabs_with_client_updates)
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

TEST_F(BufferSwapper, test_grab_release_pattern)
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
