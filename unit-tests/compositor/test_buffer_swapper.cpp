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

#include <mir/compositor/buffer_swapper_double.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

#if MIR_TODO
TEST(buffer_swapper_double, simple_swaps)
{
    using namespace testing;

    geom::Width w{1024};
    geom::Height h{768};
    geom::Stride s{1024};
    mc::PixelFormat pf{mc::PixelFormat::rgba_8888};
    
    std::shared_ptr<mc::Buffer> buf_a(new mc::MockBuffer(w, h, s, pf));
    std::shared_ptr<mc::Buffer> buf_b(new mc::MockBuffer(w, h, s, pf));
    std::shared_ptr<mc::Buffer> buf_tmp_a;
    std::shared_ptr<mc::Buffer> buf_tmp_b;

    /* BufferSwapperDouble implements the BufferSwapper interface */
    mc::BufferSwapperDouble swapper_double(buf_a, buf_b);

    mc::BufferSwapper * swapper = &swapper_double;

    /* normal, one in, one out, pattern */
    /* first buffer is requested and returned */
    swapper->dequeue_free_buffer(buf_tmp_a);
    EXPECT_TRUE((buf_tmp_a == buf_a) || (buf_tmp_a == buf_b)); /* we should get valid buffer we supplied in constructor */
    swapper->queue_finished_buffer(buf_tmp_a);
    
    /* second buffer is requested and returned */
    swapper->dequeue_free_buffer(buf_tmp_b);
    EXPECT_TRUE((buf_tmp_a == buf_a) || (buf_tmp_a == buf_b)); /* we should get valid buffer we supplied in constructor */
    EXPECT_NE(buf_tmp_a, buf_tmp_b); /* we should have gotten a different buffer back this time */
    swapper->queue_finished_buffer(buf_tmp_b);

    /* third buffer is requested and returned */
    swapper->dequeue_free_buffer(buf_tmp_a);
    EXPECT_TRUE((buf_tmp_a == buf_a) || (buf_tmp_a == buf_b)); /* we should get valid buffer we supplied in constructor */
    EXPECT_NE(buf_tmp_a, buf_tmp_b); /* we should have gotten a different buffer back this time */
    swapper->queue_finished_buffer(buf_tmp_a);
} 

TEST(buffer_swapper_double, simple_grabs)
{
    geom::Width w{1024};
    geom::Height h{768};
    geom::Stride s{1024};
    mc::PixelFormat pf{mc::PixelFormat::rgba_8888};
    
    std::shared_ptr<mc::Buffer> buf_a(new mc::MockBuffer(w, h, s, pf));
    std::shared_ptr<mc::Buffer> buf_b(new mc::MockBuffer(w, h, s, pf));
    std::shared_ptr<mc::Buffer> buf_tmp_a;
    std::shared_ptr<mc::Buffer> buf_tmp_b;
    std::shared_ptr<mc::Buffer> buf_tmp_c;

    /* BufferSwapperDouble implements the BufferSwapper interface */
    mc::BufferSwapperDouble swapper_double(buf_a, buf_b);
    mc::BufferSwapper * swapper = &swapper_double;

    /* first buffer is requested and returned (this gives us a valid last posted buffer ) */
    swapper->dequeue_free_buffer(buf_tmp_a);
    EXPECT_TRUE((buf_tmp_a == buf_a) || (buf_tmp_a == buf_b)); /* we should get valid buffer we supplied in constructor */
    swapper->queue_finished_buffer(buf_tmp_a);

    swapper->grab_last_posted(buf_tmp_b);
    EXPECT_EQ(buf_tmp_a, buf_tmp_b); /* whatever buf_tmp_a was, this was the last posted buffer */
    swapper->ungrab(buf_tmp_b);

    swapper->dequeue_free_buffer(buf_tmp_c);
    EXPECT_TRUE((buf_tmp_c == buf_a) || (buf_tmp_c == buf_b)); /* we should get valid buffer we supplied in constructor */
    EXPECT_NE(buf_tmp_c, buf_tmp_a); /* we should have gotten a different buffer back this time */
    swapper->queue_finished_buffer(buf_tmp_a);

    swapper->grab_last_posted(buf_tmp_a);
    EXPECT_EQ(buf_tmp_a, buf_tmp_c); /* whatever buf_tmp_c was, this was the last posted buffer */
    swapper->ungrab(buf_tmp_b);

}
#endif

/* this tests the start-up behavior of the swap algorithm */
TEST(buffer_swapper, init_test)
{
    using namespace testing;

    geom::Width w{1024};
    geom::Height h{768};
    geom::Stride s{1024};
    mc::PixelFormat pf{mc::PixelFormat::rgba_8888};
    
    std::shared_ptr<mc::Buffer> buf_a(new mc::MockBuffer(w, h, s, pf));
    std::shared_ptr<mc::Buffer> buf_b(new mc::MockBuffer(w, h, s, pf));
    std::shared_ptr<mc::Buffer> buf_tmp;

    /* BufferSwapperDouble implements the BufferSwapper interface */
    mc::BufferSwapperDouble swapper_double(buf_a, buf_b);

    mc::BufferSwapper * swapper = &swapper_double;

    swapper->grab_last_posted(buf_tmp);
    /* note: kdub, the grab_last_posted has something of undefined behavior if nothing has ever been posted */
    EXPECT_EQ(buf_tmp, nullptr); /* no one has posted yet, so we should be returning nothing (or error) */

}
