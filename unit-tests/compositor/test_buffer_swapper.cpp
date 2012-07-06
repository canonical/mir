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
