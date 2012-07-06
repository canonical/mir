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
#include "mir/compositor/buffer_swapper.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace {
const geom::Width width{1024};
const geom::Height height{768};
const geom::Stride stride{geom::dim_cast<geom::Stride>(width)};
const mc::PixelFormat pixel_format{mc::PixelFormat::rgba_8888};

struct EmptyDeleter
{
    template<typename T>
    void operator()(T* )
    {
    }    
};

struct MockSwapper : public mc::BufferSwapper
{
    MOCK_METHOD1(dequeue_free_buffer, void(std::shared_ptr<mc::Buffer>&));
    MOCK_METHOD1(queue_finished_buffer, void(std::shared_ptr<mc::Buffer>&));
    MOCK_METHOD1(grab_last_posted, void(std::shared_ptr<mc::Buffer>&));
    MOCK_METHOD1(ungrab, void(std::shared_ptr<mc::Buffer>&));
};

}
/* testing adding and removing buffers */
TEST(buffer_bundle, add_rm_buffers) 
{
    using namespace testing;

    mc::BufferBundle buffer_bundle;
    mc::MockBuffer mock_buffer{width, height, stride, pixel_format};
    std::shared_ptr<mc::MockBuffer> default_buffer(
        &mock_buffer,
        EmptyDeleter()); 
    int buffers_removed;

    buffer_bundle.add_buffer(default_buffer);
    buffer_bundle.add_buffer(default_buffer);
    buffer_bundle.add_buffer(default_buffer);

    buffers_removed = buffer_bundle.remove_all_buffers();

    EXPECT_EQ(buffers_removed, 3); 

    buffer_bundle.add_buffer(default_buffer);
    buffer_bundle.add_buffer(default_buffer);
    buffers_removed = buffer_bundle.remove_all_buffers();

    EXPECT_EQ(buffers_removed, 2); 
}

/* this would simulate binding and locking a back buffer for the compositor's use */
/* tests the BufferBundle's implementation of the BufferTextureBinder interface */
TEST(buffer_bundle, add_buffers_and_bind)
{
    using namespace testing;
   
    mc::BufferBundle buffer_bundle;
    mc::MockBuffer mock_buffer{width, height, stride, pixel_format};
    std::shared_ptr<mc::MockBuffer> default_buffer(
        &mock_buffer,
        EmptyDeleter());
    MockSwapper mock_swapper; 

    buffer_bundle.add_buffer(default_buffer);
    buffer_bundle.add_buffer(default_buffer);
    buffer_bundle.set_swap_pattern(&mock_swapper);

    int num_iterations = 5;
    EXPECT_CALL(mock_buffer, bind_to_texture())
            .Times(AtLeast(num_iterations));
    EXPECT_CALL(mock_buffer, lock())
            .Times(AtLeast(num_iterations));
    EXPECT_CALL(mock_buffer, unlock())
            .Times(AtLeast(num_iterations));
    EXPECT_CALL(mock_swapper, grab_last_posted(Eq(default_buffer)))
            .Times(num_iterations);
    EXPECT_CALL(mock_swapper, ungrab(Eq(default_buffer)))
            .Times(num_iterations);

    mc::BufferTextureBinder *binder;
    binder = &buffer_bundle;

    for(int i=0; i<num_iterations; i++) {
        /* if binding doesn't work, this is a case where we may have an exception */
        ASSERT_NO_THROW({
                binder->lock_and_bind_back_buffer();
            });
    }
}

/* this would simulate locking a buffer for a client's use */
/* tests the BufferBundle's implemantation of the BufferQueue interface */
TEST(buffer_bundle, add_buffers_and_distribute) {
    using namespace testing;
   
    mc::BufferBundle buffer_bundle;
    mc::MockBuffer mock_buffer{width, height, stride, pixel_format};
    std::shared_ptr<mc::MockBuffer> default_buffer(
        &mock_buffer,
        EmptyDeleter()); 
    MockSwapper mock_swapper; 

    buffer_bundle.add_buffer(default_buffer);
    buffer_bundle.add_buffer(default_buffer);
    buffer_bundle.set_swap_pattern(&mock_swapper);

    mc::BufferQueue * queue;
    queue = &buffer_bundle;
    int num_iterations = 5;
    EXPECT_CALL(mock_buffer, lock())
            .Times(AtLeast(num_iterations));
    EXPECT_CALL(mock_buffer, unlock())
            .Times(AtLeast(num_iterations));
    EXPECT_CALL(mock_swapper, dequeue_free_buffer(Eq(default_buffer)))
            .Times(num_iterations);
    EXPECT_CALL(mock_swapper, queue_finished_buffer(Eq(default_buffer)))
            .Times(num_iterations);

    std::shared_ptr<mc::Buffer> sent_buffer;
    for(int i=0; i<num_iterations; i++) {
        /* todo: (kdub) sent_buffer could be swapped out with an IPC-friendly
           data bundle in the future */
        sent_buffer = nullptr;
        sent_buffer = queue->dequeue_client_buffer();
        EXPECT_NE(nullptr, sent_buffer);
        queue->queue_client_buffer(sent_buffer);
    }
}

TEST(buffer_bundle, add_buffers_bind_and_distribute) {
    using namespace testing;

    mc::BufferBundle buffer_bundle;
    mc::MockBuffer mock_buffer_cli{width, height, stride, pixel_format};
    std::shared_ptr<mc::MockBuffer> default_buffer_cli(
        &mock_buffer_cli,
        EmptyDeleter());
    
    mc::MockBuffer mock_buffer_com{width, height, stride, pixel_format};
    std::shared_ptr<mc::MockBuffer> default_buffer_com(
        &mock_buffer_com,
        EmptyDeleter());
    MockSwapper mock_swapper;

    buffer_bundle.add_buffer(default_buffer_com);
    buffer_bundle.add_buffer(default_buffer_cli);
    buffer_bundle.set_swap_pattern(&mock_swapper);

    EXPECT_CALL(mock_buffer_cli, lock())
            .Times(AtLeast(1));
    EXPECT_CALL(mock_buffer_cli, unlock())
            .Times(AtLeast(1));
    EXPECT_CALL(mock_buffer_com, bind_to_texture())
            .Times(AtLeast(1));
    EXPECT_CALL(mock_buffer_com, lock())
            .Times(AtLeast(1));
    EXPECT_CALL(mock_buffer_com, unlock())
            .Times(AtLeast(1));

    buffer_bundle.lock_and_bind_back_buffer();
    buffer_bundle.dequeue_client_buffer();

}
