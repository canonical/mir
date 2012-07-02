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
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/geometry/dimensions.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_manager_client.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

const geom::Width width{1024};
const geom::Height height{768};
const geom::Stride stride{geom::dim_cast<geom::Stride>(width)};
const mc::PixelFormat pixel_format{mc::PixelFormat::rgba_8888};

namespace {
struct MockBuffer : public mc::Buffer
{
 public:
	MockBuffer()
	{
	    using namespace testing;
		ON_CALL(*this, width()).       WillByDefault(Return(::width));
		ON_CALL(*this, height()).      WillByDefault(Return(::height));
		ON_CALL(*this, stride()).      WillByDefault(Return(::stride));
		ON_CALL(*this, pixel_format()).WillByDefault(Return(::pixel_format));
	}

    MOCK_CONST_METHOD0(width, geom::Width());
    MOCK_CONST_METHOD0(height, geom::Height());
    MOCK_CONST_METHOD0(stride, geom::Stride());
    MOCK_CONST_METHOD0(pixel_format, mc::PixelFormat());

    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(bind_to_texture, void());

};

}

/* testing adding and removing buffers */
TEST(buffer_manager_client, add_rm_buffers) 
{
    using namespace testing;

    mc::BufferManagerClient bm_client;
    MockBuffer mock_buffer;
    std::vector<mc::Buffer*> buffers_removed;

    bm_client.add_buffer(&mock_buffer);
    bm_client.add_buffer(&mock_buffer);
    bm_client.add_buffer(&mock_buffer);

    buffers_removed = bm_client.remove_all_buffers();

    ASSERT_EQ(buffers_removed.size(), 3); 
    for(int i=0; i<3; i++) { 
        EXPECT_EQ(buffers_removed[i], &mock_buffer);
    }

    bm_client.add_buffer(&mock_buffer);
    bm_client.add_buffer(&mock_buffer);
    buffers_removed = bm_client.remove_all_buffers();

    ASSERT_EQ(buffers_removed.size(), 2); 
    for(int i=0; i<2; i++) { 
        EXPECT_EQ(buffers_removed[i], &mock_buffer);
    }
}

/* this would simulate binding and locking a back buffer for the compositor's use */
TEST(buffer_manager_client, add_buffers_and_bind)
{
    using namespace testing;
   
    mc::BufferManagerClient bm_client;
    MockBuffer mock_buffer;

    bm_client.add_buffer(&mock_buffer);

    EXPECT_CALL(mock_buffer, bind_to_texture())
            .Times(AtLeast(1));
    EXPECT_CALL(mock_buffer, lock())
            .Times(AtLeast(1));

    bm_client.bind_back_buffer();
}

/* this would simulate locking a buffer for a client's use */
TEST(buffer_manager_client, add_buffers_and_distribute) {
    using namespace testing;
   
    mc::BufferManagerClient bm_client;
    MockBuffer mock_buffer;

    bm_client.add_buffer(&mock_buffer);

    EXPECT_CALL(mock_buffer, lock())
            .Times(AtLeast(1));

    bm_client.dequeue_client_buffer();

}

TEST(buffer_manager_client, add_buffers_bind_and_distribute) {
    using namespace testing;

    mc::BufferManagerClient bm_client;
    MockBuffer mock_buffer_cli;
    MockBuffer mock_buffer_com;

    bm_client.add_buffer(&mock_buffer_cli);
    bm_client.add_buffer(&mock_buffer_com);

    EXPECT_CALL(mock_buffer_cli, lock())
            .Times(AtLeast(1));

    EXPECT_CALL(mock_buffer_com, bind_to_texture())
            .Times(AtLeast(1));
    EXPECT_CALL(mock_buffer_com, lock())
            .Times(AtLeast(1));

    bm_client.bind_back_buffer();
    bm_client.dequeue_client_buffer();

}
