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

#include "src/platforms/android/framebuffers.h"
#include "src/platforms/android/graphic_buffer_allocator.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_fb_hal_device.h"

#include <future>
#include <initializer_list>
#include <thread>
#include <stdexcept>
#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

namespace
{

struct MockGraphicBufferAllocator : public mga::GraphicBufferAllocator
{
    MOCK_METHOD3(alloc_buffer_platform, std::shared_ptr<mg::Buffer>(
        geom::Size, MirPixelFormat, mga::BufferUsage use));
};

static int const display_width = 180;
static int const display_height = 1010101;

class PostingFBBundleTest : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        using namespace testing;
        fbnum = 2;
        format = HAL_PIXEL_FORMAT_RGBA_8888;
        buffer1 = std::make_shared<mtd::MockBuffer>();
        buffer2 = std::make_shared<mtd::MockBuffer>();
        buffer3 = std::make_shared<mtd::MockBuffer>();
        mock_allocator = std::make_shared<MockGraphicBufferAllocator>();
        mock_fb_hal = std::make_shared<mtd::MockFBHalDevice>(display_width, display_height, format, fbnum);
        EXPECT_CALL(*mock_allocator, alloc_buffer_platform(_,_,_))
            .Times(AtLeast(0))
            .WillOnce(Return(buffer1))
            .WillOnce(Return(buffer2))
            .WillRepeatedly(Return(buffer3));
    }

    int format;
    int fbnum;
    double vrefresh_hz{55.330};
    std::shared_ptr<mtd::MockFBHalDevice> mock_fb_hal;
    std::shared_ptr<MockGraphicBufferAllocator> mock_allocator;
    std::shared_ptr<mg::Buffer> buffer1;
    std::shared_ptr<mg::Buffer> buffer2;
    std::shared_ptr<mg::Buffer> buffer3;
    geom::Size display_size{display_width, display_height};
    testing::NiceMock<mtd::MockEGL> mock_egl;
};

}

TEST_F(PostingFBBundleTest, hwc_fb_size_allocation)
{
    using namespace testing;

    EXPECT_CALL(*mock_allocator, alloc_buffer_platform(display_size, _, mga::BufferUsage::use_framebuffer_gles))
        .Times(2)
        .WillRepeatedly(Return(nullptr));

    mga::Framebuffers framebuffers(mock_allocator, display_size, vrefresh_hz, 2u);
    EXPECT_EQ(display_size, framebuffers.fb_size());
    EXPECT_EQ(vrefresh_hz, framebuffers.fb_refresh_rate());
}

TEST_F(PostingFBBundleTest, hwc_fb_format_selection)
{
    using namespace testing;
    EGLint const expected_egl_config_attr [] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_FRAMEBUFFER_TARGET_ANDROID, EGL_TRUE,
        EGL_NONE
    };

    int visual_id = HAL_PIXEL_FORMAT_BGRA_8888;
    EGLDisplay fake_display = reinterpret_cast<EGLDisplay>(0x11235813);
    EGLConfig fake_egl_config = reinterpret_cast<EGLConfig>(0x44);

    Sequence seq;
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .InSequence(seq)
        .WillOnce(Return(fake_display));
    EXPECT_CALL(mock_egl, eglInitialize(fake_display,_,_))
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglChooseConfig(fake_display,mtd::AttrMatches(expected_egl_config_attr),_,1,_))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<2>(fake_egl_config), SetArgPointee<4>(1), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglGetConfigAttrib(fake_display, fake_egl_config, EGL_NATIVE_VISUAL_ID, _))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<3>(visual_id), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglTerminate(fake_display))
        .InSequence(seq);

    mga::Framebuffers framebuffers(mock_allocator, display_size, vrefresh_hz, 2u);
    EXPECT_EQ(mir_pixel_format_argb_8888, framebuffers.fb_format());
}

//not all hwc11 implementations give a hint about their framebuffer formats in their configuration.
//prefer abgr_8888 if we can't figure things out
TEST_F(PostingFBBundleTest, hwc_version_11_format_selection_failure)
{
    using namespace testing;
    EGLDisplay fake_display = reinterpret_cast<EGLDisplay>(0x11235813);

    Sequence seq;
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .InSequence(seq)
        .WillOnce(Return(fake_display));
    EXPECT_CALL(mock_egl, eglInitialize(fake_display,_,_))
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglChooseConfig(_,_,_,_,_))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<4>(0), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglTerminate(fake_display))
        .InSequence(seq);

    mga::Framebuffers framebuffers(mock_allocator, display_size, vrefresh_hz, 2u);
    EXPECT_EQ(mir_pixel_format_abgr_8888, framebuffers.fb_format());
}

TEST_F(PostingFBBundleTest, bundle_from_fb)
{
    using namespace testing;
    auto display_size = geom::Size{display_width, display_height};
    EXPECT_CALL(*mock_allocator, alloc_buffer_platform(display_size, mir_pixel_format_abgr_8888, mga::BufferUsage::use_framebuffer_gles))
        .Times(fbnum)
        .WillRepeatedly(Return(nullptr));

    mga::Framebuffers framebuffers(mock_allocator, display_size, vrefresh_hz, mock_fb_hal);
    EXPECT_EQ(display_size, framebuffers.fb_size());
    EXPECT_EQ(mir_pixel_format_abgr_8888, framebuffers.fb_format());
}

//some drivers incorrectly report 0 buffers available. if this is true, we should alloc 2, the minimum requirement
TEST_F(PostingFBBundleTest, determine_fbnum_always_reports_2_minimum)
{
    using namespace testing;
    auto slightly_malformed_fb_hal_mock = std::make_shared<mtd::MockFBHalDevice>(display_width, display_height, format, 0);
    EXPECT_CALL(*mock_allocator, alloc_buffer_platform(_,_,_))
        .Times(2)
        .WillRepeatedly(Return(nullptr));

    mga::Framebuffers framebuffers(mock_allocator, display_size, vrefresh_hz, slightly_malformed_fb_hal_mock);
}

TEST_F(PostingFBBundleTest, last_rendered_returns_valid)
{
    mga::Framebuffers framebuffers(mock_allocator, display_size, vrefresh_hz, mock_fb_hal);

    auto test_buffer = framebuffers.last_rendered_buffer();
    EXPECT_TRUE((test_buffer == buffer1) || (test_buffer == buffer2));

    auto first_buffer = framebuffers.buffer_for_render();
    auto first_buffer_ptr = first_buffer.get();
    EXPECT_NE(first_buffer, framebuffers.last_rendered_buffer());
    first_buffer.reset();
    EXPECT_EQ(first_buffer_ptr, framebuffers.last_rendered_buffer().get());
}

TEST_F(PostingFBBundleTest, last_rendered_is_first_returned_from_driver)
{
    mga::Framebuffers framebuffers(mock_allocator, display_size, vrefresh_hz, 2u);
    auto buffer1 = framebuffers.buffer_for_render().get();
    EXPECT_EQ(buffer1, framebuffers.last_rendered_buffer().get());
    auto buffer2 = framebuffers.buffer_for_render().get();
    EXPECT_EQ(buffer2, framebuffers.last_rendered_buffer().get());
}

TEST_F(PostingFBBundleTest, no_rendering_returns_same_buffer)
{
    mga::Framebuffers framebuffers(mock_allocator, display_size, vrefresh_hz, 2u);
    framebuffers.buffer_for_render().get();
    auto buffer = framebuffers.last_rendered_buffer();
    EXPECT_EQ(buffer, framebuffers.last_rendered_buffer());
}

TEST_F(PostingFBBundleTest, three_buffers_for_hwc)
{
    mga::Framebuffers framebuffers(mock_allocator, display_size, vrefresh_hz, 3u);

    auto buffer1 = framebuffers.buffer_for_render().get();
    framebuffers.buffer_for_render();
    framebuffers.buffer_for_render();
    auto buffer4 = framebuffers.buffer_for_render().get();

    EXPECT_EQ(buffer1, buffer4);
}
