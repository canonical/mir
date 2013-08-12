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

#include "mir/geometry/rectangle.h"
#include "src/client/client_platform.h"
#include "src/client/client_platform_factory.h"
#include "src/client/mir_connection.h"
#include "src/client/default_connection_configuration.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"
#include "src/client/display_configuration.h"

#include "mir/frontend/resource_cache.h" /* needed by test_server.h */
#include "mir_test/test_protobuf_server.h"
#include "mir_test/stub_server_tool.h"

#include "mir_protobuf.pb.h"

#include <google/protobuf/descriptor.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace geom = mir::geometry;

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
        else if (method->name() == "configure_display")
        {
            configure_display_sent(static_cast<mp::DisplayConfiguration const*>(parameters));
        }

        complete->Run();
    }

    MOCK_METHOD1(drm_auth_magic, void(const mp::DRMMagic*));
    MOCK_METHOD2(connect, void(mp::ConnectParameters const*,mp::Connection*));
    MOCK_METHOD1(configure_display_sent, void(mp::DisplayConfiguration const*));
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

class TestConnectionConfiguration : public mcl::DefaultConnectionConfiguration
{
public:
    TestConnectionConfiguration(
        std::shared_ptr<mcl::ClientPlatform> const& platform,
        std::shared_ptr<mcl::rpc::MirBasicRpcChannel> const& channel)
        : DefaultConnectionConfiguration(""),
          disp_config(std::make_shared<mcl::DisplayConfiguration>()),
          platform{platform},
          channel{channel}
    {
    }

    std::shared_ptr<mcl::rpc::MirBasicRpcChannel> the_rpc_channel() override
    {
        return channel;
    }

    std::shared_ptr<mcl::ClientPlatformFactory> the_client_platform_factory() override
    {
        return std::make_shared<StubClientPlatformFactory>(platform);
    }

    std::shared_ptr<mcl::DisplayConfiguration> the_display_configuration() override
    {
        return disp_config;
    }
private:
    std::shared_ptr<mcl::DisplayConfiguration> disp_config;
    std::shared_ptr<mcl::ClientPlatform> const platform;
    std::shared_ptr<mcl::rpc::MirBasicRpcChannel> const channel;
};

}

struct MirConnectionTest : public testing::Test
{
    void SetUp()
    {
        using namespace testing;

        mock_platform = std::make_shared<NiceMock<MockClientPlatform>>();
        mock_channel = std::make_shared<NiceMock<MockRpcChannel>>();

        TestConnectionConfiguration conf{mock_platform, mock_channel};

        connection = std::make_shared<MirConnection>(conf);
    }

    std::shared_ptr<testing::NiceMock<MockClientPlatform>> mock_platform;
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
    wait_handle->wait_for_all();

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
    wait_handle->wait_for_all();

    int const no_error{0};
    int status{67};

    wait_handle = connection->drm_auth_magic(drm_magic, drm_auth_magic_callback, &status);
    wait_handle->wait_for_all();

    EXPECT_EQ(no_error, status);
}

namespace
{

std::vector<MirPixelFormat> const supported_output_formats{
    mir_pixel_format_abgr_8888,
    mir_pixel_format_xbgr_8888
};

unsigned int const number_of_displays = 4;
geom::Rectangle rects[number_of_displays] = {
    geom::Rectangle{geom::Point(1,2), geom::Size(14,15)},
    geom::Rectangle{geom::Point(3,4), geom::Size(12,13)},
    geom::Rectangle{geom::Point(5,6), geom::Size(10,11)},
    geom::Rectangle{geom::Point(7,8), geom::Size(9,10)},
};

void fill_display_configuration(mp::ConnectParameters const*, mp::Connection* response)
{
    auto protobuf_config = response->mutable_display_configuration();

    for (auto i = 0u; i < number_of_displays; i++)
    {
        auto output = protobuf_config->add_display_output();
        auto const& rect = rects[i];
        output->set_position_x(rect.top_left.x.as_uint32_t());
        output->set_position_y(rect.top_left.y.as_uint32_t());
        auto mode = output->add_mode();
        mode->set_horizontal_resolution(rect.size.width.as_uint32_t());
        mode->set_vertical_resolution(rect.size.height.as_uint32_t());
        for (auto pf : supported_output_formats)
            output->add_pixel_format(static_cast<uint32_t>(pf));
    }
}

}

TEST_F(MirConnectionTest, populates_display_output_correctly_on_startup)
{
    using namespace testing;

    EXPECT_CALL(*mock_channel, connect(_,_))
        .WillOnce(Invoke(fill_display_configuration));

    MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest",
                                                     connected_callback, 0);
    wait_handle->wait_for_all();

    auto configuration = connection->create_copy_of_display_config();

    ASSERT_EQ(number_of_displays, configuration->num_displays);
    for(auto i=0u; i < number_of_displays; i++)
    {
        auto output = configuration->displays[i];
        auto rect = rects[i];

        ASSERT_EQ(1u, output.num_modes);
        ASSERT_NE(nullptr, output.modes);
        EXPECT_EQ(rect.size.width.as_uint32_t(), output.modes[0].horizontal_resolution);
        EXPECT_EQ(rect.size.height.as_uint32_t(), output.modes[0].vertical_resolution);

        EXPECT_EQ(output.position_x, static_cast<int>(rect.top_left.x.as_uint32_t()));
        EXPECT_EQ(output.position_y, static_cast<int>(rect.top_left.y.as_uint32_t()));
 
        ASSERT_EQ(supported_output_formats.size(),
                  static_cast<uint32_t>(output.num_output_formats));

        for (size_t i = 0; i < supported_output_formats.size(); ++i)
        {
            EXPECT_EQ(supported_output_formats[i], output.output_formats[i]);
        }
    }

    mcl::delete_config_storage(configuration);
}

TEST_F(MirConnectionTest, user_tries_to_configure_incorrectly)
{
    using namespace testing;

    EXPECT_CALL(*mock_channel, connect(_,_))
        .WillOnce(Invoke(fill_display_configuration));

    MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest",
                                                     connected_callback, 0);
    wait_handle->wait_for_all();

    auto configuration = connection->create_copy_of_display_config();
    EXPECT_GT(configuration->num_displays, 0u);
    auto proper_num_displays = configuration->num_displays;
    auto proper_displays = configuration->displays;
    auto proper_output_id = configuration->displays[0].output_id;

    //user lies about num_displays
    configuration->num_displays = 0;
    EXPECT_EQ(nullptr, connection->configure_display(configuration));

    configuration->num_displays = proper_num_displays + 1;
    EXPECT_EQ(nullptr, connection->configure_display(configuration));

    configuration->num_displays = proper_num_displays;
   
    //user sends nullptr for displays 
    configuration->displays = nullptr;
    EXPECT_EQ(nullptr, connection->configure_display(configuration));

    configuration->displays = proper_displays;

    //user makes up own id
    configuration->displays[0].output_id = 4944949;
    EXPECT_EQ(nullptr, connection->configure_display(configuration));
    configuration->displays[0].output_id = proper_output_id; 

    //user tries to set nonsense mode
    configuration->displays[0].current_mode++;
    EXPECT_EQ(nullptr, connection->configure_display(configuration));

    mcl::delete_config_storage(configuration);
}

TEST_F(MirConnectionTest, populates_pfs_correctly)
{
    using namespace testing;

    EXPECT_CALL(*mock_channel, connect(_,_))
        .WillOnce(Invoke(fill_display_configuration));
    MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest",
                                                     connected_callback, 0);
    wait_handle->wait_for_all();

    unsigned int const formats_size = 5;
    unsigned int valid_formats = 0;
    MirPixelFormat formats[formats_size]; 

    connection->possible_pixel_formats(&formats[0], formats_size, valid_formats);

    ASSERT_EQ(2u, valid_formats);
    for (auto i=0u; i < valid_formats; i++)
    {
        EXPECT_EQ(supported_output_formats[i], formats[i]);
    }
}

TEST_F(MirConnectionTest, valid_display_configure_sent)
{
    using namespace testing;

    EXPECT_CALL(*mock_channel, connect(_,_))
        .WillOnce(Invoke(fill_display_configuration));

    MirDisplayOutput output;
    output.output_id = 0;
    output.current_mode = 0;
    output.used = 0;
    output.position_x = 4;
    output.position_y = 6;
    MirDisplayConfiguration user_config{1, &output, 0, nullptr};

    auto verify_display_change = [&](mp::DisplayConfiguration const* config)
    {
        ASSERT_NE(nullptr, config);
        ASSERT_EQ(1, config->display_output_size());
        auto const& disp1 = config->display_output(0);
        EXPECT_EQ(output.output_id, disp1.output_id());
        EXPECT_EQ(output.used, disp1.used());
        EXPECT_EQ(output.current_mode, disp1.current_mode());
        EXPECT_EQ(output.position_x, disp1.position_x());
        EXPECT_EQ(output.position_y, disp1.position_y());
    };

    EXPECT_CALL(*mock_channel, configure_display_sent(_))
        .Times(1)
        .WillOnce(Invoke(verify_display_change));

    MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest", connected_callback, 0);
    wait_handle->wait_for_all();

    auto config_wait_handle = connection->configure_display(&user_config);
    config_wait_handle->wait_for_all();
}
