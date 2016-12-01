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
#include "src/client/mir_connection.h"
#include "mir/dispatch/dispatchable.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"
#include "mir_toolkit/extensions/android_buffer.h"
#include "mir_test_framework/stub_client_connection_configuration.h"
#include "mir/test/doubles/mock_client_context.h"
#include "mir/test/doubles/mock_egl_native_surface.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_android_hw.h"
#include "mir_test_framework/client_platform_factory.h"
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

TEST_F(AndroidClientPlatformTest, can_allocate_buffer)
{
    using namespace std::literals::chrono_literals;
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
        mir::Fd watch_fd() const
        {
            int fd[2];
            pipe(fd);
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
    mir_wait_for(connection.connect("", [](MirConnection*, void*){}, nullptr));

    int width = 32;
    int height = 90;
    auto ext = static_cast<MirExtensionAndroidBuffer*>(
        platform->request_interface(
            MIR_EXTENSION_ANDROID_BUFFER,MIR_EXTENSION_ANDROID_BUFFER_VERSION_1));
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
