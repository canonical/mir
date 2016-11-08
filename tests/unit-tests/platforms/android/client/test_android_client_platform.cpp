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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_native_window.h"
#include "mir/client_platform.h"
#include "mir/fd.h"
#include "mir_toolkit/extensions/fenced_buffers.h"
#include "mir/test/doubles/mock_client_context.h"
#include "mir/test/doubles/mock_egl_native_surface.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_buffer_registrar.h"
#include "mir/test/doubles/mock_android_native_buffer.h"
#include "mir_test_framework/client_platform_factory.h"
#include "src/platforms/android/client/buffer.h"
#include "src/client/buffer.h"
#include <android/system/graphics.h>
#include <EGL/egl.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace mir::client;
using namespace mir::test;
using namespace mir::test::doubles;
using namespace mir_test_framework;
namespace mcla = mir::client::android;
namespace mga = mir::graphics::android;
namespace mcl = mir::client;
namespace geom = mir::geometry;

struct AndroidClientPlatformTest : public Test
{
    AndroidClientPlatformTest()
        : platform{create_android_client_platform()}
    {
    }

    std::shared_ptr<mir::client::ClientPlatform> platform;
    MockEGL mock_egl;
};

TEST_F(AndroidClientPlatformTest, egl_native_display_is_egl_default_display)
{
    MockEGLNativeSurface surface;
    auto mock_egl_native_surface = std::make_shared<MockEGLNativeSurface>();
    auto native_display = platform->create_egl_native_display();
    EXPECT_EQ(EGL_DEFAULT_DISPLAY, *native_display);
}

TEST_F(AndroidClientPlatformTest, egl_native_window_is_set)
{
    MockEGLNativeSurface surface;
    auto mock_egl_native_surface = std::make_shared<MockEGLNativeSurface>();
    auto egl_native_window = platform->create_egl_native_window(&surface);
    EXPECT_NE(nullptr, egl_native_window);
}

TEST_F(AndroidClientPlatformTest, egl_native_window_can_be_set_with_null_native_surface)
{
    auto egl_native_window = platform->create_egl_native_window(nullptr);
    EXPECT_NE(nullptr, egl_native_window);
}

TEST_F(AndroidClientPlatformTest, error_interpreter_used_with_null_native_surface)
{
    auto egl_native_window = platform->create_egl_native_window(nullptr);
    auto native_window = reinterpret_cast<mir::graphics::android::MirNativeWindow*>(egl_native_window.get());
    ANativeWindow& window = *native_window;
    EXPECT_EQ(window.setSwapInterval(&window, 1), -1);
}

TEST_F(AndroidClientPlatformTest, egl_pixel_format_asks_the_driver)
{
    auto const d = reinterpret_cast<EGLDisplay>(0x1234);
    auto const c = reinterpret_cast<EGLConfig>(0x5678);

    EXPECT_CALL(mock_egl, eglGetConfigAttrib(d, c, EGL_NATIVE_VISUAL_ID, _))
        .WillOnce(DoAll(SetArgPointee<3>(HAL_PIXEL_FORMAT_RGB_565),
                        Return(EGL_TRUE)))
        .WillOnce(DoAll(SetArgPointee<3>(HAL_PIXEL_FORMAT_RGB_888),
                        Return(EGL_TRUE)))
        .WillOnce(DoAll(SetArgPointee<3>(HAL_PIXEL_FORMAT_BGRA_8888),
                        Return(EGL_TRUE)))
        .WillOnce(DoAll(SetArgPointee<3>(0),
                        Return(EGL_FALSE)));

    EXPECT_EQ(mir_pixel_format_rgb_565, platform->get_egl_pixel_format(d, c));
    EXPECT_EQ(mir_pixel_format_rgb_888, platform->get_egl_pixel_format(d, c));
    EXPECT_EQ(mir_pixel_format_argb_8888, platform->get_egl_pixel_format(d, c));
    EXPECT_EQ(mir_pixel_format_invalid, platform->get_egl_pixel_format(d, c));
}

//TODO: platform helper uses anon namespace in header
struct ClientExtensions : Test
{
    std::shared_ptr<mtd::MockBufferRegistrar> create_registrar()
    {
        auto r = std::make_shared<NiceMock<mtd::MockBufferRegistrar>>();
        ON_CALL(*r, register_buffer(_, _))
            .WillByDefault(Return(mock_native_buffer));
        return r;
    }

    ClientExtensions() :
        platform{create_android_client_platform()},
        native_handle{std::make_shared<native_handle_t>()},
        mock_native_buffer{std::make_shared<NiceMock<mtd::MockAndroidNativeBuffer>>(geom::Size{1, 1})},
        mock_registrar{create_registrar()},
        extension(static_cast<MirExtensionFencedBuffers*>(
            platform->request_interface(
                MIR_EXTENSION_FENCED_BUFFERS, MIR_EXTENSION_FENCED_BUFFERS_VERSION_1)))
    {
    }

    std::shared_ptr<mcl::ClientPlatform> platform;
    MockEGL mock_egl;
    std::shared_ptr<native_handle_t const> const native_handle;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> const mock_native_buffer;
    std::shared_ptr<mtd::MockBufferRegistrar> const mock_registrar;
    MirBufferPackage package;
    MirExtensionFencedBuffers* extension;
    int fake_fence = 8482;
    mcl::Buffer buffer{
        nullptr, nullptr, 0,
        std::make_shared<mcla::Buffer>(mock_registrar, package, mir_pixel_format_abgr_8888),
        nullptr, mir_buffer_usage_software};
    ::MirBuffer* mir_buffer = reinterpret_cast<::MirBuffer*>(&buffer);
};
TEST_F(ClientExtensions, can_update_fences)
{
    Sequence seq;
    EXPECT_CALL(*mock_native_buffer, update_usage(fake_fence, mga::BufferAccess::write))
        .InSequence(seq);
    EXPECT_CALL(*mock_native_buffer, update_usage(_, mga::BufferAccess::read))
        .InSequence(seq);


    ASSERT_THAT(extension, Ne(nullptr)); 
    ASSERT_THAT(extension->associate_fence, Ne(nullptr)); 
    extension->associate_fence(mir_buffer, fake_fence, mir_read_write);
    extension->associate_fence(mir_buffer, fake_fence, mir_read);
}

TEST_F(ClientExtensions, updating_fences_with_invalid_resets_fence)
{
    ASSERT_THAT(extension, Ne(nullptr)); 
    ASSERT_THAT(extension->associate_fence, Ne(nullptr));
    extension->associate_fence(mir_buffer, mir::Fd::invalid, mir_read_write);
}

TEST_F(ClientExtensions, can_retreive_fences)
{
    EXPECT_CALL(*mock_native_buffer, fence())
        .WillOnce(Return(fake_fence));
    
    ASSERT_THAT(extension, Ne(nullptr)); 
    ASSERT_THAT(extension->get_fence, Ne(nullptr));
    EXPECT_THAT(extension->get_fence(mir_buffer), Eq(fake_fence));
}

TEST_F(ClientExtensions, can_wait_fence)
{
    ASSERT_THAT(extension, Ne(nullptr)); 
    ASSERT_THAT(extension->wait_for_access, Ne(nullptr));

    using namespace std::literals::chrono_literals;
    Sequence seq;
    EXPECT_CALL(*mock_native_buffer, ensure_available_for(mga::BufferAccess::read, 0ms))
        .InSequence(seq)
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_native_buffer, ensure_available_for(mga::BufferAccess::write, 10ms))
        .InSequence(seq)
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_native_buffer, ensure_available_for(mga::BufferAccess::write, 10ms))
        .InSequence(seq)
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_native_buffer, ensure_available_for(mga::BufferAccess::write, 10ms))
        .InSequence(seq)
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_native_buffer, ensure_available_for(mga::BufferAccess::write, 11ms))
        .InSequence(seq)
        .WillOnce(Return(true));

    EXPECT_FALSE(extension->wait_for_access(mir_buffer, mir_read, 150));
    EXPECT_TRUE(extension->wait_for_access(mir_buffer, mir_read_write, 10499999));
    EXPECT_TRUE(extension->wait_for_access(mir_buffer, mir_read_write, 10500001));
    EXPECT_TRUE(extension->wait_for_access(mir_buffer, mir_read_write, 10999999));
    EXPECT_TRUE(extension->wait_for_access(mir_buffer, mir_read_write, 11000001));
}
