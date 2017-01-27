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
#include "src/client/mir_connection.h"
#include "mir/dispatch/dispatchable.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"
#include "mir_toolkit/extensions/android_buffer.h"
#include "mir_test_framework/stub_client_connection_configuration.h"
#include "mir/test/doubles/mock_client_context.h"
#include "mir/test/doubles/mock_egl_native_surface.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_buffer_registrar.h"
#include "mir/test/doubles/mock_android_native_buffer.h"
#include "mir/test/doubles/mock_android_hw.h"
#include "mir_test_framework/client_platform_factory.h"
#include "src/platforms/android/client/buffer.h"
#include "src/client/buffer.h"
#include <android/system/graphics.h>
#include <EGL/egl.h>
#include <system/window.h>
#include <hardware/gralloc.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <condition_variable>
#include <mutex>
#include "mir_protobuf.pb.h"

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
    testing::NiceMock<mtd::HardwareAccessMock> hw;
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

TEST_F(AndroidClientPlatformTest, translates_format_to_hal_pixel_formats)
{
    EXPECT_THAT(platform->native_format_for(mir_pixel_format_abgr_8888), Eq(HAL_PIXEL_FORMAT_RGBA_8888));
    EXPECT_THAT(platform->native_format_for(mir_pixel_format_xbgr_8888), Eq(HAL_PIXEL_FORMAT_RGBX_8888));
}

TEST_F(AndroidClientPlatformTest, translates_usage_to_gralloc_bits)
{
    auto hw_usage = GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER;
    auto sw_usage =
        GRALLOC_USAGE_SW_WRITE_OFTEN |
        GRALLOC_USAGE_SW_READ_OFTEN |
        GRALLOC_USAGE_HW_COMPOSER |
        GRALLOC_USAGE_HW_TEXTURE;
    EXPECT_THAT(platform->native_flags_for(mir_buffer_usage_software, {}), Eq(sw_usage));
    EXPECT_THAT(platform->native_flags_for(mir_buffer_usage_hardware, {}), Eq(hw_usage));
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
        extension(static_cast<MirExtensionFencedBuffersV1*>(
            platform->request_interface("mir_extension_fenced_buffers", 1)))
    {
    }

    std::shared_ptr<mcl::ClientPlatform> platform;
    MockEGL mock_egl;
    std::shared_ptr<native_handle_t const> const native_handle;
    std::shared_ptr<mtd::MockAndroidNativeBuffer> const mock_native_buffer;
    std::shared_ptr<mtd::MockBufferRegistrar> const mock_registrar;
    MirBufferPackage package;
    MirExtensionFencedBuffersV1* extension;
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
}

TEST_F(AndroidClientPlatformTest, can_allocate_buffer)
{
    struct DummyChannel : rpc::MirBasicRpcChannel,
        mir::dispatch::Dispatchable
    {
        void call_method(
            std::string const&,
            google::protobuf::MessageLite const*,
            google::protobuf::MessageLite*,
            google::protobuf::Closure* c) override
        {
            channel_call_count++;
            c->Run();
        }
        void discard_future_calls() override
        {
        }
        void wait_for_outstanding_calls() override
        {
        }
        mir::Fd watch_fd() const
        {
            int fd[2];
            if (pipe(fd))
                return mir::Fd{};
            mir::Fd{fd[1]};
            return mir::Fd{fd[0]};
        }
        bool dispatch(mir::dispatch::FdEvents)
        {
            return true;
        }
        mir::dispatch::FdEvents relevant_events() const { return {}; }
        int channel_call_count = 0;
    };
    auto channel = std::make_shared<DummyChannel>();
    struct StubConnection : mir_test_framework::StubConnectionConfiguration
    {
        StubConnection(
            std::string str,
            std::shared_ptr<DummyChannel> const& channel,
            std::shared_ptr<ClientPlatform> const& platform) :
            mir_test_framework::StubConnectionConfiguration(str),
            channel(channel),
            platform(platform)
        {
        }

        std::shared_ptr<ClientPlatformFactory> the_client_platform_factory() override
        {
            struct StubPlatformFactory : ClientPlatformFactory
            {
                StubPlatformFactory(std::shared_ptr<ClientPlatform> const& platform) :
                    platform(platform)
                {
                }

                std::shared_ptr<ClientPlatform> create_client_platform(ClientContext*) override
                {
                    return platform;
                }
                std::shared_ptr<ClientPlatform> platform;
            };
            return std::make_shared<StubPlatformFactory>(platform);
        }

        std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> the_rpc_channel() override
        {
            return channel;
        }
        std::shared_ptr<DummyChannel> channel;
        std::shared_ptr<ClientPlatform> platform;
    } conf(std::string{}, channel, platform);

    MirConnection connection(conf);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_wait_for(connection.connect("", [](MirConnection*, void*){}, nullptr));
#pragma GCC diagnostic pop

    int width = 32;
    int height = 90;
    auto ext = static_cast<MirExtensionAndroidBufferV1*>(
        platform->request_interface("mir_extension_android_buffer", 1));
    ASSERT_THAT(ext, Ne(nullptr));
    ASSERT_THAT(ext->allocate_buffer_android, Ne(nullptr));

    auto call_count = channel->channel_call_count;
    ext->allocate_buffer_android(
        &connection,
        width, height,
        HAL_PIXEL_FORMAT_RGBA_8888,
        GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE,
        [] (::MirBuffer*, void*) {}, nullptr);
    EXPECT_THAT(channel->channel_call_count, Eq(call_count+1));
}
