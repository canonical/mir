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

#include "mir/geometry/dimensions.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_bundle.h"
#include "mir/compositor/buffer_swapper_double.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{
const geom::Width width {1024};
const geom::Height height {768};
const geom::Stride stride {geom::dim_cast<geom::Stride>(width)};
const mc::PixelFormat pixel_format {mc::PixelFormat::rgba_8888};

struct EmptyDeleter
{
    template<typename T>
    void operator()(T* )
    {
    }
};

}

struct MockSwapper : public mc::BufferSwapper
{
    public:
        MOCK_METHOD0(dequeue_free_buffer,   mc::Buffer*(void));
        MOCK_METHOD0(queue_finished_buffer, void(void));
        MOCK_METHOD0(grab_last_posted,   mc::Buffer*(void));
        MOCK_METHOD0(ungrab,   void(void));
};

TEST(buffer_bundle, get_buffer_for_compositor)
{
    using namespace testing;

    mc::MockBuffer mock_buffer {width, height, stride, pixel_format};

    MockSwapper *mock_swapper = new MockSwapper();
    std::unique_ptr<mc::BufferSwapper> swapper_handle(mock_swapper);
    mc::BufferBundle buffer_bundle(std::move(swapper_handle));

    EXPECT_CALL(*mock_swapper, grab_last_posted())
        .Times(1)
        .WillRepeatedly(Return(&mock_buffer));

    /* expectations on the mock buffer */
    EXPECT_CALL(mock_buffer, bind_to_texture())
        .Times(1);

    /* if binding doesn't work, this is a case where we may have an exception */
    ASSERT_NO_THROW(
    {
        buffer_bundle.lock_and_bind_back_buffer();
    });

}

TEST(buffer_bundle, get_buffer_for_client)
{
    using namespace testing;

    mc::MockBuffer mock_buffer {width, height, stride, pixel_format};

    MockSwapper *mock_swapper = new MockSwapper();
    std::unique_ptr<mc::BufferSwapper> swapper_handle(mock_swapper);
    mc::BufferBundle buffer_bundle(std::move(swapper_handle));

    EXPECT_CALL(*mock_swapper, dequeue_free_buffer())
        .Times(1)
        .WillRepeatedly(Return(&mock_buffer));

    /* expectations on the mock buffer */
    EXPECT_CALL(mock_buffer, lock())
        .Times(1);

    /* if binding doesn't work, this is a case where we may have an exception */
    std::shared_ptr<mc::Buffer> sent_buffer;
    ASSERT_NO_THROW(
    {
        /* todo: (kdub) sent_buffer could be swapped out with an IPC-friendly
           data bundle in the future */
        sent_buffer = buffer_bundle.dequeue_client_buffer();
        EXPECT_NE(nullptr, sent_buffer);
    });

}

