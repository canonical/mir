/*
 * Copyright © 2013 Canonical Ltd.
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

#include "src/server/graphics/android/server_render_window.h"
#include "src/server/graphics/android/fb_swapper.h"

#include "mir_test_doubles/mock_display_support_provider.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_interpreter_resource_cache.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>

namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;
namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mc=mir::compositor;

namespace
{

struct StubFence : public mga::SyncObject
{
    virtual ~StubFence() noexcept {}
    MOCK_METHOD0(wait, void());
};

struct MockFBSwapper : public mga::FBSwapper
{
    ~MockFBSwapper() noexcept {}
    MOCK_METHOD0(compositor_acquire, std::shared_ptr<mg::Buffer>());
    MOCK_METHOD1(compositor_release, void(std::shared_ptr<mg::Buffer> const& released_buffer));
};

struct ServerRenderWindowTest : public ::testing::Test
{
    virtual void SetUp()
    {
        using namespace testing;
        mock_buffer1 = std::make_shared<NiceMock<mtd::MockBuffer>>();
        mock_buffer2 = std::make_shared<NiceMock<mtd::MockBuffer>>();
        mock_buffer3 = std::make_shared<NiceMock<mtd::MockBuffer>>();
        mock_swapper = std::make_shared<NiceMock<MockFBSwapper>>();
        mock_display_poster = std::make_shared<NiceMock<mtd::MockDisplaySupportProvider>>();
        mock_cache = std::make_shared<mtd::MockInterpreterResourceCache>();
        ON_CALL(*mock_display_poster, display_format())
            .WillByDefault(Return(geom::PixelFormat::abgr_8888));
        stub_sync = std::make_shared<StubFence>();
    }

    std::shared_ptr<mtd::MockBuffer> mock_buffer1;
    std::shared_ptr<mtd::MockBuffer> mock_buffer2;
    std::shared_ptr<mtd::MockBuffer> mock_buffer3;
    std::shared_ptr<mtd::MockInterpreterResourceCache> mock_cache;
    std::shared_ptr<MockFBSwapper> mock_swapper;
    std::shared_ptr<mtd::MockDisplaySupportProvider> mock_display_poster;
    std::shared_ptr<StubFence> stub_sync;
};
}

TEST_F(ServerRenderWindowTest, driver_wants_a_buffer)
{
    using namespace testing;
    mga::ServerRenderWindow render_window(mock_swapper, mock_display_poster, mock_cache);

    auto stub_anw = std::make_shared<ANativeWindowBuffer>();

    EXPECT_CALL(*mock_swapper, compositor_acquire())
        .Times(1)
        .WillOnce(Return(mock_buffer1));
    EXPECT_CALL(*mock_buffer1, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(stub_anw));

    std::shared_ptr<mg::Buffer> tmp = mock_buffer1;
    EXPECT_CALL(*mock_cache, store_buffer(tmp, stub_anw.get()))
        .Times(1);

    auto rc_buffer = render_window.driver_requests_buffer();
    EXPECT_EQ(stub_anw.get(), rc_buffer);
}

TEST_F(ServerRenderWindowTest, driver_is_done_with_a_buffer_properly)
{
    using namespace testing;
    auto stub_anw = std::make_shared<ANativeWindowBuffer>();
    EXPECT_CALL(*mock_cache, retrieve_buffer(stub_anw.get()))
        .Times(1)
        .WillOnce(Return(mock_buffer1));
    mga::ServerRenderWindow render_window(mock_swapper, mock_display_poster, mock_cache);

    EXPECT_CALL(*mock_swapper, compositor_acquire())
        .Times(1)
        .WillOnce(Return(mock_buffer1));
    EXPECT_CALL(*mock_buffer1, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(stub_anw));

    render_window.driver_requests_buffer();
    testing::Mock::VerifyAndClearExpectations(mock_swapper.get());

    std::shared_ptr<mg::Buffer> buf1 = mock_buffer1;
    EXPECT_CALL(*mock_swapper, compositor_release(buf1))
        .Times(1);
    EXPECT_CALL(*stub_sync, wait())
        .Times(1);

    render_window.driver_returns_buffer(stub_anw.get(), stub_sync);
    testing::Mock::VerifyAndClearExpectations(mock_swapper.get());
}

TEST_F(ServerRenderWindowTest, driver_returns_buffer_posts_to_fb)
{
    using namespace testing;
    auto stub_anw = std::make_shared<ANativeWindowBuffer>();
    EXPECT_CALL(*mock_cache, retrieve_buffer(_))
        .Times(1)
        .WillOnce(Return(mock_buffer1));
    mga::ServerRenderWindow render_window(mock_swapper, mock_display_poster, mock_cache);

    mg::BufferID id{442}, returned_id;
    EXPECT_CALL(*mock_swapper, compositor_acquire())
        .Times(1)
        .WillOnce(Return(mock_buffer1));
    EXPECT_CALL(*mock_swapper, compositor_release(_))
        .Times(1);
    std::shared_ptr<mg::Buffer> buf1 = mock_buffer1;
    EXPECT_CALL(*mock_buffer1, native_buffer_handle())
        .Times(1)
        .WillOnce(Return(stub_anw));
    EXPECT_CALL(*mock_display_poster, set_next_frontbuffer(buf1))
        .Times(1);

    auto handle1 = render_window.driver_requests_buffer();
    render_window.driver_returns_buffer(handle1, stub_sync);
}

TEST_F(ServerRenderWindowTest, driver_inquires_about_format)
{
    using namespace testing; 

    EXPECT_CALL(*mock_display_poster, display_format())
        .Times(1)
        .WillOnce(Return(geom::PixelFormat::abgr_8888));

    mga::ServerRenderWindow render_window(mock_swapper, mock_display_poster, mock_cache);

    EXPECT_EQ(HAL_PIXEL_FORMAT_RGBA_8888, render_window.driver_requests_info(NATIVE_WINDOW_FORMAT));
}

TEST_F(ServerRenderWindowTest, driver_inquires_about_format_after_format_set)
{
    using namespace testing;

    mga::ServerRenderWindow render_window(mock_swapper, mock_display_poster, mock_cache);
    EXPECT_CALL(*mock_display_poster, display_format())
        .Times(0);

    render_window.dispatch_driver_request_format(HAL_PIXEL_FORMAT_RGBX_8888);
    auto rc_format = render_window.driver_requests_info(NATIVE_WINDOW_FORMAT);
    EXPECT_EQ(HAL_PIXEL_FORMAT_RGBX_8888, rc_format); 
}

TEST_F(ServerRenderWindowTest, driver_inquires_about_size_without_having_been_set)
{
    using namespace testing;
    geom::Size test_size{4, 5};
    EXPECT_CALL(*mock_display_poster, display_size())
        .Times(4)
        .WillRepeatedly(Return(test_size));

    mga::ServerRenderWindow render_window(mock_swapper, mock_display_poster, mock_cache);

    unsigned int rc_width = render_window.driver_requests_info(NATIVE_WINDOW_DEFAULT_WIDTH);
    unsigned int rc_height = render_window.driver_requests_info(NATIVE_WINDOW_DEFAULT_HEIGHT);
    EXPECT_EQ(test_size.width.as_uint32_t(), rc_width); 
    EXPECT_EQ(test_size.height.as_uint32_t(), rc_height);

    rc_width = render_window.driver_requests_info(NATIVE_WINDOW_WIDTH);
    rc_height = render_window.driver_requests_info(NATIVE_WINDOW_HEIGHT);
    EXPECT_EQ(test_size.width.as_uint32_t(), rc_width); 
    EXPECT_EQ(test_size.height.as_uint32_t(), rc_height); 
}

TEST_F(ServerRenderWindowTest, driver_inquires_about_transform)
{
    using namespace testing;

    mga::ServerRenderWindow render_window(mock_swapper, mock_display_poster, mock_cache);

    EXPECT_EQ(0, render_window.driver_requests_info(NATIVE_WINDOW_TRANSFORM_HINT));
}

TEST_F(ServerRenderWindowTest, driver_unknown_inquiry)
{
    using namespace testing;
    mga::ServerRenderWindow render_window(mock_swapper, mock_display_poster, mock_cache);

    EXPECT_THROW({
        render_window.driver_requests_info(NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND);
    }, std::runtime_error);
}

TEST_F(ServerRenderWindowTest, driver_swapinterval_request)
{
    mga::ServerRenderWindow render_window(mock_swapper, mock_display_poster, mock_cache);

    EXPECT_CALL(*mock_display_poster, sync_to_display(false))
        .Times(1);

    render_window.sync_to_display(false);
}
