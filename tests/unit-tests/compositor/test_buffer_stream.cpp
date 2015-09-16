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

#include "src/server/compositor/buffer_stream_surfaces.h"
#include "src/server/scene/legacy_surface_change_notification.h"

#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/mock_buffer_bundle.h"
#include "mir/test/gmock_fixes.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <condition_variable>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

class BufferStreamTest : public ::testing::Test
{
protected:
    BufferStreamTest()
    {
        mock_buffer = std::make_shared<mtd::StubBuffer>();
        mock_bundle = std::make_shared<testing::NiceMock<mtd::MockBufferBundle>>();

        // Two of the tests care about this, the rest should not...
        EXPECT_CALL(*mock_bundle, force_requests_to_complete())
            .Times(::testing::AnyNumber());
        ON_CALL(*mock_bundle, properties())
            .WillByDefault(testing::Return(properties));
    }

    std::shared_ptr<mtd::StubBuffer> mock_buffer;
    std::shared_ptr<mtd::MockBufferBundle> mock_bundle;
    geom::Size size{4, 5};
    MirPixelFormat format{mir_pixel_format_abgr_8888};
    mg::BufferProperties properties {size, format, mg::BufferUsage::hardware};
};

TEST_F(BufferStreamTest, size_query)
{
    EXPECT_CALL(*mock_bundle, properties())
        .WillOnce(testing::Return(properties));

    mc::BufferStreamSurfaces buffer_stream(mock_bundle);
    auto returned_size = buffer_stream.stream_size();
    EXPECT_EQ(size, returned_size);
}

TEST_F(BufferStreamTest, pixel_format_query)
{
    mc::BufferStreamSurfaces buffer_stream(mock_bundle);
    EXPECT_THAT(buffer_stream.pixel_format(), testing::Eq(format));
}

TEST_F(BufferStreamTest, force_requests_to_complete)
{
    EXPECT_CALL(*mock_bundle, force_requests_to_complete())
        .Times(2);  // Once explcit, once on destruction

    mc::BufferStreamSurfaces buffer_stream(mock_bundle);
    buffer_stream.force_requests_to_complete();
}

TEST_F(BufferStreamTest, requests_are_completed_before_destruction)
{
    EXPECT_CALL(*mock_bundle, force_requests_to_complete())
        .Times(1);

    mc::BufferStreamSurfaces buffer_stream(mock_bundle);
}

TEST_F(BufferStreamTest, get_buffer_for_compositor_handles_resources)
{
    using namespace testing;

    EXPECT_CALL(*mock_bundle, compositor_acquire(_))
        .Times(1)
        .WillOnce(Return(mock_buffer));
    EXPECT_CALL(*mock_bundle, compositor_release(_))
        .Times(1);

    mc::BufferStreamSurfaces buffer_stream(mock_bundle);

    buffer_stream.lock_compositor_buffer(0);
}

TEST_F(BufferStreamTest, get_buffer_for_compositor_can_lock)
{
    using namespace testing;

    EXPECT_CALL(*mock_bundle, compositor_acquire(_))
        .Times(1)
        .WillOnce(Return(mock_buffer));
    EXPECT_CALL(*mock_bundle, compositor_release(_))
        .Times(1);

    mc::BufferStreamSurfaces buffer_stream(mock_bundle);

    buffer_stream.lock_compositor_buffer(0);
}

TEST_F(BufferStreamTest, get_buffer_for_client_releases_resources)
{
    std::mutex mutex;
    std::condition_variable cv;
    mg::Buffer* buffer{nullptr};
    bool done = false;

    auto const callback =
        [&](mg::Buffer* new_buffer)
        {
           std::unique_lock<decltype(mutex)> lock(mutex);
           buffer = new_buffer;
           done = true;
           cv.notify_one();
        };

    using namespace testing;
    mc::BufferStreamSurfaces buffer_stream(mock_bundle);

    InSequence seq;
    EXPECT_CALL(*mock_bundle, client_acquire(_))
        .Times(1)
        .WillOnce(InvokeArgument<0>(mock_buffer.get()));
    EXPECT_CALL(*mock_bundle, client_release(_))
        .Times(1);
    EXPECT_CALL(*mock_bundle, client_acquire(_))
        .Times(1)
        .WillOnce(InvokeArgument<0>(mock_buffer.get()));

    buffer_stream.acquire_client_buffer(callback);

    {
        std::unique_lock<decltype(mutex)> lock(mutex);

        cv.wait(lock, [&]{ return done; });
    }

    done = false;

    if (buffer)
        buffer_stream.release_client_buffer(buffer);
    buffer_stream.acquire_client_buffer(callback);

    {
        std::unique_lock<decltype(mutex)> lock(mutex);

        cv.wait(lock, [&]{ return done; });
    }
}

TEST_F(BufferStreamTest, allow_framedropping_device)
{
    EXPECT_CALL(*mock_bundle, allow_framedropping(true))
        .Times(1);

    mc::BufferStreamSurfaces buffer_stream(mock_bundle);
    buffer_stream.allow_framedropping(true);
}

TEST_F(BufferStreamTest, resizes_bundle)
{
    geom::Size const new_size{66, 77};

    EXPECT_CALL(*mock_bundle, resize(new_size))
        .Times(1);

    mc::BufferStreamSurfaces buffer_stream(mock_bundle);
    buffer_stream.resize(new_size);
}

TEST_F(BufferStreamTest, swap_buffer)
{
    using namespace testing;
    EXPECT_CALL(*mock_bundle, client_acquire(_))
        .Times(2)
        .WillRepeatedly(InvokeArgument<0>(mock_buffer.get()));
    EXPECT_CALL(*mock_bundle, client_release(_))
        .Times(1);

    mc::BufferStreamSurfaces buffer_stream(mock_bundle);
    mir::graphics::Buffer* buffer = nullptr;
    auto const callback = [&](mir::graphics::Buffer* new_buffer) { buffer = new_buffer;};
    // The first call is with a nullptr buffer, so no frame posted.
    buffer_stream.swap_buffers(buffer, callback);
    // The second call posts the buffer returned by first, and should notify
    buffer_stream.swap_buffers(buffer, callback);
}

TEST_F(BufferStreamTest, notifies_on_swap)
{
    using namespace testing;
    ON_CALL(*mock_bundle, client_acquire(_))
        .WillByDefault(InvokeArgument<0>(mock_buffer.get()));
    struct MockCallback
    {
        MOCK_METHOD0(call, void());
    };
    NiceMock<MockCallback> mock_cb;
    EXPECT_CALL(mock_cb, call()).Times(3);
    auto observer = std::make_shared<mir::scene::LegacySurfaceChangeNotification>(
        []{ FAIL() << "buffer stream shouldnt notify of scene changes.";},
        std::bind(&MockCallback::call, &mock_cb));

    mc::BufferStreamSurfaces buffer_stream(mock_bundle);
    buffer_stream.add_observer(observer);

    mg::Buffer* buffer{nullptr};
    auto const complete = [&buffer](mg::Buffer* new_buffer){ buffer = new_buffer; };
    buffer_stream.swap_buffers(buffer, complete);
    buffer_stream.swap_buffers(buffer, complete);
    buffer_stream.swap_buffers(buffer, complete);
    buffer_stream.swap_buffers(buffer, complete);
}

TEST_F(BufferStreamTest, allocate_and_release_not_supported)
{
    mg::BufferProperties properties;
    mc::BufferStreamSurfaces buffer_stream(mock_bundle);
    EXPECT_THROW({
        buffer_stream.allocate_buffer(properties);
    }, std::logic_error);
    EXPECT_THROW({
        buffer_stream.remove_buffer(mg::BufferID{3});
    }, std::logic_error);
}

TEST_F(BufferStreamTest, scale_resizes_and_sets_size_appropriately)
{
    using namespace testing;
    auto const scale = 2.0f; 
    auto scaled_size = scale * size;

    mg::BufferProperties non_scaled{size, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};
    Sequence seq;
    EXPECT_CALL(*mock_bundle, properties())
        .InSequence(seq)
        .WillOnce(testing::Return(non_scaled));
    EXPECT_CALL(*mock_bundle, resize(scaled_size))
        .InSequence(seq);

    mc::BufferStreamSurfaces buffer_stream(mock_bundle);
    buffer_stream.set_scale(scale);
    EXPECT_THAT(buffer_stream.stream_size(), Eq(size));
}

TEST_F(BufferStreamTest, scaled_resizes_appropriately)
{
    using namespace testing;
    auto const scale = 2.0f; 
    auto scaled_size = scale * size;

    mg::BufferProperties non_scaled{size, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};
    geom::Size logical_resize_request{10, 20};
    geom::Size physical_resize_request{20, 40};
    
    InSequence seq;
    EXPECT_CALL(*mock_bundle, properties())
        .WillOnce(testing::Return(non_scaled));
    EXPECT_CALL(*mock_bundle, resize(scaled_size));
    EXPECT_CALL(*mock_bundle, resize(physical_resize_request));

    mc::BufferStreamSurfaces buffer_stream(mock_bundle);
    buffer_stream.set_scale(scale);
    buffer_stream.resize(logical_resize_request);
}

TEST_F(BufferStreamTest, cannot_set_silly_scale_values)
{
    mc::BufferStreamSurfaces buffer_stream(mock_bundle);
    EXPECT_THROW(
        buffer_stream.set_scale(0.0f);
    , std::logic_error);
    EXPECT_THROW(
        buffer_stream.set_scale(-1.0f);
    , std::logic_error);
}
