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


#include "mock_buffer.h"

#include "mir/compositor/buffer_swapper_double.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;


#include <iostream>
namespace
{
geom::Width w {1024};
geom::Height h {768};
geom::Stride s {1024};
mc::PixelFormat pf {mc::PixelFormat::rgba_8888};

struct BufferSwapper : testing::Test
{
    BufferSwapper()
    {
        std::unique_ptr<mc::Buffer> buffer_a(new mc::MockBuffer(w, h, s, pf));
        std::unique_ptr<mc::Buffer> buffer_b(new mc::MockBuffer(w, h, s, pf));

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

TEST_F(BufferSwapper, simple_swaps0)
{
    mc::Buffer* buf_tmp;

    buf_tmp = swapper->client_acquire();
    EXPECT_TRUE((buf_tmp == buf_a) || (buf_tmp == buf_b));

    swapper->client_release(buf_tmp);
}

TEST_F(BufferSwapper, simple_swaps1)
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

/* tests that grab returns valid buffer */
TEST_F(BufferSwapper, simple_grabs0)
{
    mc::Buffer* buf_tmp, *buf_tmp_b;

    buf_tmp_b = swapper->client_acquire();
    swapper->client_release(buf_tmp_b);

    buf_tmp = swapper->compositor_acquire();
    EXPECT_TRUE((buf_tmp == buf_a) || (buf_tmp == buf_b)); /* we should get valid buffer we supplied in constructor */
}

/* note: tests expectation that we have the last posted buffer */
TEST_F(BufferSwapper, simple_grabs1)
{
    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;

    buf_tmp_a = swapper->client_acquire();
    swapper->client_release(buf_tmp_a);

    buf_tmp_b = swapper->compositor_acquire();
    swapper->compositor_release(buf_tmp_b);

    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
}


/* note: tests expectation that two grabs in a row should return same thing */
TEST_F(BufferSwapper, simple_grabs2)
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

TEST_F(BufferSwapper, simple_grabs3)
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

TEST_F(BufferSwapper, simple_grabs4)
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
