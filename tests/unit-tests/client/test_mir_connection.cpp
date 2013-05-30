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

#include "src/client/mir_logger.h"
#include "src/client/client_platform.h"
#include "src/client/client_platform_factory.h"
#include "src/client/mir_connection.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"

#include "mir/frontend/resource_cache.h" /* needed by test_server.h */
#include "mir_test/test_protobuf_server.h"
#include "mir_test/stub_server_tool.h"

#include "mir_protobuf.pb.h"

#include <google/protobuf/descriptor.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace mp = mir::protobuf;

namespace
{

struct MockRpcChannel : public mir::client::rpc::MirBasicRpcChannel
{
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController*,
                    const google::protobuf::Message* parameters,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* complete)
    {
        if (method->name() == "drm_auth_magic")
        {
            drm_auth_magic(static_cast<const mp::DRMMagic*>(parameters));
        }
        else if (method->name() == "connect")
        {
            static_cast<mp::Connection*>(response)->clear_error();
            connect(static_cast<mp::ConnectParameters const*>(parameters),
                    static_cast<mp::Connection*>(response));
        }

        complete->Run();
    }

    MOCK_METHOD1(drm_auth_magic, void(const mp::DRMMagic*));
    MOCK_METHOD2(connect, void(mp::ConnectParameters const*,mp::Connection*));

    void set_event_handler(mir::events::EventSink *) {}
};

struct MockClientPlatform : public mcl::ClientPlatform
{
    MockClientPlatform()
    {
        using namespace testing;

        auto native_display = std::make_shared<EGLNativeDisplayType>();
        *native_display = reinterpret_cast<EGLNativeDisplayType>(0x0);

        ON_CALL(*this, create_egl_native_display())
            .WillByDefault(Return(native_display));
    }

    MOCK_CONST_METHOD0(platform_type, MirPlatformType()); 
    MOCK_METHOD0(create_buffer_factory, std::shared_ptr<mcl::ClientBufferFactory>());
    MOCK_METHOD1(create_egl_native_window, std::shared_ptr<EGLNativeWindowType>(mcl::ClientSurface*));
    MOCK_METHOD0(create_egl_native_display, std::shared_ptr<EGLNativeDisplayType>());
};

struct StubClientPlatformFactory : public mcl::ClientPlatformFactory
{
    StubClientPlatformFactory(std::shared_ptr<mcl::ClientPlatform> const& platform)
        : platform{platform}
    {
    }

    std::shared_ptr<mcl::ClientPlatform> create_client_platform(mcl::ClientContext*)
    {
        return platform;
    }

    std::shared_ptr<mcl::ClientPlatform> platform;
};

void connected_callback(MirConnection* /*connection*/, void * /*client_context*/)
{
}

void drm_auth_magic_callback(int status, void* client_context)
{
    auto status_ptr = static_cast<int*>(client_context);
    *status_ptr = status;
}

}

struct MirConnectionTest : public testing::Test
{
    void SetUp()
    {
        using namespace testing;

        logger = std::make_shared<mcl::ConsoleLogger>();
        mock_platform = std::make_shared<NiceMock<MockClientPlatform>>();
        platform_factory = std::make_shared<StubClientPlatformFactory>(mock_platform);
        mock_channel = std::make_shared<NiceMock<MockRpcChannel>>();

        connection = std::make_shared<MirConnection>(mock_channel, logger,
                                                     platform_factory);
    }

    std::shared_ptr<mcl::Logger> logger;
    std::shared_ptr<testing::NiceMock<MockClientPlatform>> mock_platform;
    std::shared_ptr<StubClientPlatformFactory> platform_factory;
    std::shared_ptr<testing::NiceMock<MockRpcChannel>> mock_channel;
    std::shared_ptr<MirConnection> connection;
};

TEST_F(MirConnectionTest, returns_correct_egl_native_display)
{
    using namespace testing;

    EGLNativeDisplayType native_display_raw = reinterpret_cast<EGLNativeDisplayType>(0xabcdef);
    auto native_display = std::make_shared<EGLNativeDisplayType>();
    *native_display = native_display_raw;

    EXPECT_CALL(*mock_platform, create_egl_native_display())
        .WillOnce(Return(native_display));

    MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest",
                                                     connected_callback, 0);
    wait_handle->wait_for_result();

    EGLNativeDisplayType connection_native_display = connection->egl_native_display();

    ASSERT_EQ(native_display_raw, connection_native_display);
}

MATCHER_P(has_drm_magic, magic, "")
{
    return arg->magic() == magic;
}

TEST_F(MirConnectionTest, client_drm_auth_magic_calls_server_drm_auth_magic)
{
    using namespace testing;

    unsigned int const drm_magic{0x10111213};

    EXPECT_CALL(*mock_channel, drm_auth_magic(has_drm_magic(drm_magic)))
        .Times(1);

    MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest",
                                                     connected_callback, 0);
    wait_handle->wait_for_result();

    int const no_error{0};
    int status{67};

    wait_handle = connection->drm_auth_magic(drm_magic, drm_auth_magic_callback, &status);
    wait_handle->wait_for_result();

    EXPECT_EQ(no_error, status);
}

namespace
{

std::vector<MirPixelFormat> const supported_pixel_formats{
    mir_pixel_format_abgr_8888,
    mir_pixel_format_xbgr_8888
};

void fill_display_info(mp::ConnectParameters const*, mp::Connection* response)
{
    auto info = response->mutable_display_info();
    for (auto pf : supported_pixel_formats)
        info->add_supported_pixel_format(static_cast<uint32_t>(pf));
}

void fill_display_info_100(mp::ConnectParameters const*, mp::Connection* response)
{
    auto info = response->mutable_display_info();
    for (int i = 0; i < 100; i++)
        info->add_supported_pixel_format(static_cast<uint32_t>(mir_pixel_format_xbgr_8888));
}

}

TEST_F(MirConnectionTest, populates_display_info_correctly)
{
    using namespace testing;

    EXPECT_CALL(*mock_channel, connect(_,_))
        .WillOnce(Invoke(fill_display_info));

    MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest",
                                                     connected_callback, 0);
    wait_handle->wait_for_result();

    MirDisplayInfo info;

    connection->populate(info);

    ASSERT_EQ(supported_pixel_formats.size(),
              static_cast<uint32_t>(info.supported_pixel_format_items));

    for (size_t i = 0; i < supported_pixel_formats.size(); ++i)
    {
        EXPECT_EQ(supported_pixel_formats[i],
                  info.supported_pixel_format[i]);
    }
}

TEST_F(MirConnectionTest, populates_display_info_without_overflowing)
{
    using namespace testing;

    EXPECT_CALL(*mock_channel, connect(_,_))
        .WillOnce(Invoke(fill_display_info_100));

    MirWaitHandle* wait_handle = connection->connect("MirConnectionTest",
                                                     connected_callback, 0);
    wait_handle->wait_for_result();

    MirDisplayInfo info;

    connection->populate(info);

    ASSERT_EQ(mir_supported_pixel_format_max,
              info.supported_pixel_format_items);

    for (size_t i = 0; i < mir_supported_pixel_format_max; ++i)
    {
        EXPECT_EQ(mir_pixel_format_xbgr_8888,
                  info.supported_pixel_format[i]);
    }
}
