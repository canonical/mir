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

namespace
{
geom::Width w {1024};
geom::Height h {768};
geom::Stride s {1024};
mc::PixelFormat pf {mc::PixelFormat::rgba_8888};

struct BufferFixture
{
    BufferFixture()
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

TEST(buffer_swap_double, simple_swaps0)
{
    BufferFixture fix;

    mc::Buffer* const buf_a = fix.buf_a;
    mc::Buffer* const buf_b = fix.buf_b;
    mc::BufferSwapper * swapper = fix.swapper.get();

    mc::Buffer* buf_tmp;

    buf_tmp = swapper->dequeue_free_buffer();
    EXPECT_TRUE((buf_tmp == buf_a) || (buf_tmp == buf_b));

    swapper->queue_finished_buffer();
}


TEST(buffer_swap_double, simple_swaps1)
{
    BufferFixture fix;

    mc::Buffer* const buf_a = fix.buf_a;
    mc::Buffer* const buf_b = fix.buf_b;
    mc::BufferSwapper * swapper = fix.swapper.get();

    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;

    buf_tmp_a = swapper->dequeue_free_buffer();
    swapper->queue_finished_buffer();
    buf_tmp_b = swapper->dequeue_free_buffer();
    swapper->queue_finished_buffer();

    EXPECT_TRUE((buf_tmp_a == buf_a) || (buf_tmp_a == buf_b));
    EXPECT_TRUE((buf_tmp_b == buf_a) || (buf_tmp_b == buf_b));
    EXPECT_NE(buf_tmp_a, buf_tmp_b);
}

TEST(buffer_swap_double, simple_grabs0)
{
    BufferFixture fix;

    mc::Buffer* const buf_a = fix.buf_a;
    mc::Buffer* const buf_b = fix.buf_b;
    mc::BufferSwapper * swapper = fix.swapper.get();

    mc::Buffer* buf_tmp;

    swapper->dequeue_free_buffer();
    swapper->queue_finished_buffer();

    buf_tmp = swapper->grab_last_posted();
    EXPECT_TRUE((buf_tmp == buf_a) || (buf_tmp == buf_b)); /* we should get valid buffer we supplied in constructor */
}

TEST(buffer_swap_double, simple_grabs1)
{
    BufferFixture fix;

    mc::BufferSwapper * swapper = fix.swapper.get();

    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;

    buf_tmp_a = swapper->dequeue_free_buffer();
    swapper->queue_finished_buffer();

    buf_tmp_b = swapper->grab_last_posted();
    swapper->ungrab();

    EXPECT_EQ(buf_tmp_a, buf_tmp_b); /* whatever buf_tmp_a was, this was the last posted buffer */
}

TEST(buffer_swap_double, simple_grabs2)
{
    BufferFixture fix;

    mc::BufferSwapper * swapper = fix.swapper.get();

    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;

    buf_tmp_a = swapper->dequeue_free_buffer();
    swapper->queue_finished_buffer();

    buf_tmp_b = swapper->grab_last_posted();
    swapper->ungrab();

    buf_tmp_a = swapper->grab_last_posted();
    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
}
