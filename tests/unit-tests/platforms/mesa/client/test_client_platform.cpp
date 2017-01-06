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

#include "mir/client_platform.h"
#include "mir/shared_library.h"
#include "mir/raii.h"
#include "src/platforms/mesa/client/mesa_native_display_container.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"
#include "src/client/mir_connection.h"
#include "mir_test_framework/client_platform_factory.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_egl_native_surface.h"
#include "mir/dispatch/dispatchable.h"
#include "mir_protobuf.pb.h"
#include "mir_test_framework/stub_client_connection_configuration.h"
#include "mir/test/doubles/stub_connection_configuration.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mesa/native_display.h"
#include "mir_toolkit/mesa/platform_operation.h"
#include "mir_toolkit/extensions/gbm_buffer.h"

#include "gbm.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstring>

namespace mcl = mir::client;
namespace mclm = mir::client::mesa;
namespace mtf = mir_test_framework;

namespace
{

struct StubClientContext : mcl::ClientContext
{
    void populate_server_package(MirPlatformPackage& platform_package) override
    {
        platform_package.data_items = 0;
        platform_package.fd_items = 1;
    }

    void populate_graphics_module(MirModuleProperties& graphics_module) override
    {
        memset(&graphics_module, 0, sizeof(graphics_module));
    }
    MirWaitHandle* platform_operation(
        MirPlatformMessage const*, mir_platform_operation_callback, void*) override
    {
        return nullptr;
    }
    void allocate_buffer(
        mir::geometry::Size, MirPixelFormat, mir_buffer_callback, void*) override
    {
    }

    void allocate_buffer(mir::geometry::Size, uint32_t, uint32_t, mir_buffer_callback, void*) override
    {
    }

    void release_buffer(mcl::MirBuffer*) override
    {
    }
};

struct MesaClientPlatformTest : testing::Test
{
    MirPlatformMessage* set_gbm_device(gbm_device* dev)
    {
        auto request_msg = mir::raii::deleter_for(
            mir_platform_message_create(MirMesaPlatformOperation::set_gbm_device),
            &mir_platform_message_release);
        MirMesaSetGBMDeviceRequest const request{dev};
        mir_platform_message_set_data(request_msg.get(), &request, sizeof(request));

        return platform->platform_operation(request_msg.get());
    }

    StubClientContext client_context;
    std::shared_ptr<mir::client::ClientPlatform> platform =
        mtf::create_mesa_client_platform(&client_context);
    mir::test::doubles::MockEGL mock_egl;
};

}

TEST_F(MesaClientPlatformTest, egl_native_display_is_valid_until_released)
{
    auto platform_lib = mtf::get_platform_library();

    MirMesaEGLNativeDisplay* nd;
    {
        std::shared_ptr<EGLNativeDisplayType> native_display = platform->create_egl_native_display();

        nd = reinterpret_cast<MirMesaEGLNativeDisplay*>(*native_display);
        auto validate = platform_lib->load_function<int(*)(MirMesaEGLNativeDisplay*)>("mir_client_mesa_egl_native_display_is_valid");
        EXPECT_EQ(MIR_MESA_TRUE, validate(nd));
    }
    EXPECT_EQ(MIR_MESA_FALSE, mclm::mir_client_mesa_egl_native_display_is_valid(nd));
}

TEST_F(MesaClientPlatformTest, handles_set_gbm_device_platform_operation)
{
    using namespace testing;

    int const success{0};
    auto const gbm_dev_dummy = reinterpret_cast<gbm_device*>(this);

    auto response_msg = mir::raii::deleter_for(
        set_gbm_device(gbm_dev_dummy),
        &mir_platform_message_release);

    ASSERT_THAT(response_msg, NotNull());
    auto const response_data = mir_platform_message_get_data(response_msg.get());
    ASSERT_THAT(response_data.size, Eq(sizeof(MirMesaSetGBMDeviceResponse)));

    MirMesaSetGBMDeviceResponse response{-1};
    std::memcpy(&response, response_data.data, response_data.size);
    EXPECT_THAT(response.status, Eq(success));
}

TEST_F(MesaClientPlatformTest, appends_gbm_device_to_platform_package)
{
    using namespace testing;

    MirPlatformPackage pkg;
    platform->populate(pkg);
    int const previous_data_count{pkg.data_items};
    auto const gbm_dev_dummy = reinterpret_cast<gbm_device*>(this);

    auto response_msg = mir::raii::deleter_for(
        set_gbm_device(gbm_dev_dummy),
        &mir_platform_message_release);

    platform->populate(pkg);
    EXPECT_THAT(pkg.data_items,
                Eq(static_cast<int>(previous_data_count +
                                    (sizeof(gbm_dev_dummy) / sizeof(int)))));

    gbm_device* device_in_package{nullptr};
    std::memcpy(&device_in_package, &pkg.data[previous_data_count],
                sizeof(device_in_package));

    EXPECT_THAT(device_in_package, Eq(gbm_dev_dummy));
}

TEST_F(MesaClientPlatformTest, returns_gbm_compatible_pixel_formats_only)
{
    using namespace testing;

    auto const d = reinterpret_cast<EGLDisplay>(0x1234);
    auto const c = reinterpret_cast<EGLConfig>(0x5678);

    EXPECT_CALL(mock_egl, eglGetConfigAttrib(d, c, EGL_RED_SIZE, _))
        .WillRepeatedly(DoAll(SetArgPointee<3>(8), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglGetConfigAttrib(d, c, EGL_GREEN_SIZE, _))
        .WillRepeatedly(DoAll(SetArgPointee<3>(8), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglGetConfigAttrib(d, c, EGL_BLUE_SIZE, _))
        .WillRepeatedly(DoAll(SetArgPointee<3>(8), Return(EGL_TRUE)));

    EXPECT_CALL(mock_egl, eglGetConfigAttrib(d, c, EGL_ALPHA_SIZE, _))
        .WillOnce(DoAll(SetArgPointee<3>(8), Return(EGL_TRUE)))
        .WillOnce(DoAll(SetArgPointee<3>(0), Return(EGL_TRUE)))
        .WillOnce(DoAll(SetArgPointee<3>(666), Return(EGL_FALSE)));

    EXPECT_EQ(mir_pixel_format_argb_8888, platform->get_egl_pixel_format(d, c));
    EXPECT_EQ(mir_pixel_format_xrgb_8888, platform->get_egl_pixel_format(d, c));
    EXPECT_EQ(mir_pixel_format_invalid, platform->get_egl_pixel_format(d, c));
}

TEST_F(MesaClientPlatformTest, egl_native_window_is_set)
{
    testing::NiceMock<mtd::MockEGLNativeSurface> mock_surface;
    auto egl_native_window = platform->create_egl_native_window(&mock_surface);
    EXPECT_NE(nullptr, egl_native_window);
}

TEST_F(MesaClientPlatformTest, egl_native_window_can_be_set_with_null_native_surface)
{
    auto egl_native_window = platform->create_egl_native_window(nullptr);
    EXPECT_NE(nullptr, egl_native_window);
}

TEST_F(MesaClientPlatformTest, can_allocate_buffer)
{
    using namespace testing;
    using namespace std::literals::chrono_literals;

    mtd::StubConnectionConfiguration conf(platform);
    MirConnection connection(conf);
    mir_wait_for(connection.connect("", [](MirConnection*, void*){}, nullptr));

    int width = 32;
    int height = 90;
    auto ext = static_cast<MirExtensionGbmBufferV1*>(
        platform->request_interface("mir_extension_gbm_buffer", 1));
    ASSERT_THAT(ext, Ne(nullptr));
    ASSERT_THAT(ext->allocate_buffer_gbm, Ne(nullptr));

    auto call_count = conf.channel->channel_call_count;
    ext->allocate_buffer_gbm(
        &connection,
        width, height,
        GBM_FORMAT_ARGB8888, 0,
        [] (::MirBuffer*, void*) {}, nullptr);
    EXPECT_THAT(conf.channel->channel_call_count, Eq(call_count + 1));
}
