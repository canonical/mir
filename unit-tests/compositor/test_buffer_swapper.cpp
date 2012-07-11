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

struct Buffers
{
    mc::MockBuffer buf_a;
    mc::MockBuffer buf_b;

    mc::Buffer* const a;
    mc::Buffer* const b;

    Buffers() :
        buf_a(w, h, s, pf),
        buf_b(w, h, s, pf),
        a(&buf_a),
        b(&buf_b)
    {
    }
};
}

TEST(buffer_swap_double, simple_swaps0)
{
    Buffers buf;

    mc::Buffer* buf_tmp;

    /* BufferSwapperDouble implements the BufferSwapper interface */
    mc::BufferSwapperDouble swapper_double(buf.a, buf.b);
    mc::BufferSwapper * swapper = &swapper_double;

    swapper->dequeue_free_buffer(buf_tmp);
    EXPECT_TRUE((buf_tmp == buf.a) || (buf_tmp == buf.b));
    swapper->queue_finished_buffer();
}


TEST(buffer_swap_double, simple_swaps1)
{
    Buffers buf;

    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;

    /* BufferSwapperDouble implements the BufferSwapper interface */
    mc::BufferSwapperDouble swapper_double(buf.a, buf.b);
    mc::BufferSwapper * swapper = &swapper_double;

    swapper->dequeue_free_buffer(buf_tmp_a);
    swapper->queue_finished_buffer();
    swapper->dequeue_free_buffer(buf_tmp_b);
    swapper->queue_finished_buffer();

    EXPECT_TRUE((buf_tmp_a == buf.a) || (buf_tmp_a == buf.b));
    EXPECT_TRUE((buf_tmp_b == buf.a) || (buf_tmp_b == buf.b));
    EXPECT_NE(buf_tmp_a, buf_tmp_b);
}

TEST(buffer_swap_double, simple_grabs0)
{
    Buffers buf;

    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;

    /* BufferSwapperDouble implements the BufferSwapper interface */
    mc::BufferSwapperDouble swapper_double(buf.a, buf.b);
    mc::BufferSwapper * swapper = &swapper_double;

    swapper->dequeue_free_buffer(buf_tmp_a);
    swapper->queue_finished_buffer();

    swapper->grab_last_posted(buf_tmp_b);
    EXPECT_TRUE((buf_tmp_a == buf.a) || (buf_tmp_a == buf.b)); /* we should get valid buffer we supplied in constructor */
}

TEST(buffer_swap_double, simple_grabs1)
{
    Buffers buf;

    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;

    /* BufferSwapperDouble implements the BufferSwapper interface */
    mc::BufferSwapperDouble swapper_double(buf.a, buf.b);
    mc::BufferSwapper * swapper = &swapper_double;

    swapper->dequeue_free_buffer(buf_tmp_a);
    swapper->queue_finished_buffer();

    swapper->grab_last_posted(buf_tmp_b);
    swapper->ungrab();

    EXPECT_EQ(buf_tmp_a, buf_tmp_b); /* whatever buf_tmp_a was, this was the last posted buffer */
}

TEST(buffer_swap_double, simple_grabs2)
{
    Buffers buf;

    mc::Buffer* buf_tmp_a;
    mc::Buffer* buf_tmp_b;

    /* BufferSwapperDouble implements the BufferSwapper interface */
    mc::BufferSwapperDouble swapper_double(buf.a, buf.b);
    mc::BufferSwapper * swapper = &swapper_double;

    swapper->dequeue_free_buffer(buf_tmp_a);
    swapper->queue_finished_buffer();

    swapper->grab_last_posted(buf_tmp_b);
    swapper->ungrab();

    swapper->grab_last_posted(buf_tmp_a);
    EXPECT_EQ(buf_tmp_a, buf_tmp_b);
}
