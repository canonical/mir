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

#include "src/client/mir_connection.h"
#include "src/client/default_connection_configuration.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"
#include "src/client/display_configuration.h"
#include "src/client/mir_surface.h"

#include "mir/client_platform.h"
#include "mir/client_platform_factory.h"
#include "mir/client_buffer_factory.h"
#include "mir/raii.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/events/event_builders.h"
#include "mir/geometry/rectangle.h"

#include "src/server/frontend/resource_cache.h" /* needed by test_server.h */
#include "mir/test/test_protobuf_server.h"
#include "mir/test/stub_server_tool.h"
#include "mir/test/doubles/stub_client_buffer_factory.h"

#include "mir_protobuf.pb.h"

#include <google/protobuf/descriptor.h>

#include <sys/eventfd.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mcl = mir::client;
namespace mf = mir::frontend;
namespace mp = mir::protobuf;
namespace mev = mir::events;
namespace md = mir::dispatch;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{

struct MockRpcChannel : public mir::client::rpc::MirBasicRpcChannel,
                        public mir::dispatch::Dispatchable
{
    MockRpcChannel()
        : pollable_fd{eventfd(0, EFD_CLOEXEC)}
    {
        ON_CALL(*this, watch_fd()).WillByDefault(testing::Return(pollable_fd));
    }

    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController*,
                    const google::protobuf::Message* parameters,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* complete)
    {
        if (method->name() == "connect")
        {
            static_cast<mp::Connection*>(response)->clear_error();
            connect(static_cast<mp::ConnectParameters const*>(parameters),
                    static_cast<mp::Connection*>(response));
        }
        else if (method->name() == "configure_display")
        {
            configure_display_sent(static_cast<mp::DisplayConfiguration const*>(parameters));
        }
        else if (method->name() == "platform_operation")
        {
            platform_operation(static_cast<mp::PlatformOperationMessage const*>(parameters),
                               static_cast<mp::PlatformOperationMessage*>(response));
        }

        complete->Run();
    }

    MOCK_METHOD2(connect, void(mp::ConnectParameters const*,mp::Connection*));
    MOCK_METHOD1(configure_display_sent, void(mp::DisplayConfiguration const*));
    MOCK_METHOD2(platform_operation,
                 void(mp::PlatformOperationMessage const*, mp::PlatformOperationMessage*));

    MOCK_CONST_METHOD0(watch_fd, mir::Fd());
    MOCK_METHOD1(dispatch, bool(md::FdEvents));
    MOCK_CONST_METHOD0(relevant_events, md::FdEvents());
private:
    mir::Fd pollable_fd;
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
        ON_CALL(*this, create_buffer_factory())
            .WillByDefault(Return(std::make_shared<mtd::StubClientBufferFactory>()));
        ON_CALL(*this, create_egl_native_window(_))
            .WillByDefault(Return(std::shared_ptr<EGLNativeWindowType>()));
        ON_CALL(*this, platform_operation(_))
            .WillByDefault(Return(nullptr));
    }

    void set_client_context(mcl::ClientContext* ctx)
    {
        client_context = ctx;
    }

    void populate(MirPlatformPackage& pkg) const override
    {
        client_context->populate_server_package(pkg);
    }

    MOCK_CONST_METHOD1(convert_native_buffer, MirNativeBuffer*(mir::graphics::NativeBuffer*));
    MOCK_CONST_METHOD0(platform_type, MirPlatformType());
    MOCK_METHOD1(platform_operation, MirPlatformMessage*(MirPlatformMessage const*));
    MOCK_METHOD0(create_buffer_factory, std::shared_ptr<mcl::ClientBufferFactory>());
    MOCK_METHOD1(create_egl_native_window, std::shared_ptr<EGLNativeWindowType>(mcl::EGLNativeSurface*));
    MOCK_METHOD0(create_egl_native_display, std::shared_ptr<EGLNativeDisplayType>());
    MOCK_CONST_METHOD2(get_egl_pixel_format,
        MirPixelFormat(EGLDisplay, EGLConfig));

    mcl::ClientContext* client_context = nullptr;
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

    std::shared_ptr<::google::protobuf::RpcChannel> the_rpc_channel() override
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
    MirConnectionTest()
        : mock_platform{std::make_shared<testing::NiceMock<MockClientPlatform>>()},
          mock_channel{std::make_shared<testing::NiceMock<MockRpcChannel>>()},
          conf{mock_platform, mock_channel},
          connection{std::make_shared<MirConnection>(conf)}
    {
        mock_platform->set_client_context(connection.get());
    }

    std::shared_ptr<testing::NiceMock<MockClientPlatform>> const mock_platform;
    std::shared_ptr<testing::NiceMock<MockRpcChannel>> const mock_channel;
    TestConnectionConfiguration conf;
    std::shared_ptr<MirConnection> const connection;
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

namespace
{

std::vector<MirPixelFormat> const supported_output_formats{
    mir_pixel_format_abgr_8888,
    mir_pixel_format_xbgr_8888
};

unsigned int const number_of_outputs = 4;
geom::Rectangle rects[number_of_outputs] = {
    geom::Rectangle{geom::Point(1,2), geom::Size(14,15)},
    geom::Rectangle{geom::Point(3,4), geom::Size(12,13)},
    geom::Rectangle{geom::Point(5,6), geom::Size(10,11)},
    geom::Rectangle{geom::Point(7,8), geom::Size(9,10)},
};

void fill_display_configuration(mp::ConnectParameters const*, mp::Connection* response)
{
    auto protobuf_config = response->mutable_display_configuration();

    for (auto i = 0u; i < number_of_outputs; i++)
    {
        auto output = protobuf_config->add_display_output();
        output->set_output_id(i);
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

std::vector<MirPixelFormat> const supported_surface_formats{
    mir_pixel_format_argb_8888,
    mir_pixel_format_bgr_888
};

void fill_surface_pixel_formats(mp::ConnectParameters const*, mp::Connection* response)
{
    for (auto pf : supported_surface_formats)
        response->add_surface_pixel_format(static_cast<uint32_t>(pf));
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

    ASSERT_EQ(number_of_outputs, configuration->num_outputs);
    for(auto i=0u; i < number_of_outputs; i++)
    {
        auto output = configuration->outputs[i];
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
    EXPECT_GT(configuration->num_outputs, 0u);
    auto proper_num_outputs = configuration->num_outputs;
    auto proper_outputs = configuration->outputs;
    auto proper_output_id = configuration->outputs[0].output_id;

    //user lies about num_outputs
    configuration->num_outputs = 0;
    EXPECT_EQ(nullptr, connection->configure_display(configuration));

    configuration->num_outputs = proper_num_outputs + 1;
    EXPECT_EQ(nullptr, connection->configure_display(configuration));

    configuration->num_outputs = proper_num_outputs;

    //user sends nullptr for outputs
    configuration->outputs = nullptr;
    EXPECT_EQ(nullptr, connection->configure_display(configuration));

    configuration->outputs = proper_outputs;

    //user makes up own id
    configuration->outputs[0].output_id = 4944949;
    EXPECT_EQ(nullptr, connection->configure_display(configuration));
    configuration->outputs[0].output_id = proper_output_id;

    //user tries to set nonsense mode on a connected output
    configuration->outputs[0].current_mode++;
    configuration->outputs[0].connected = 1;
    EXPECT_EQ(nullptr, connection->configure_display(configuration));

    mcl::delete_config_storage(configuration);
}

TEST_F(MirConnectionTest, display_configuration_validation_succeeds_for_invalid_mode_in_disconnected_output)
{
    using namespace testing;

    EXPECT_CALL(*mock_channel, connect(_,_))
        .WillOnce(Invoke(fill_display_configuration));

    MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest",
                                                     connected_callback, 0);
    wait_handle->wait_for_all();

    auto configuration = connection->create_copy_of_display_config();
    EXPECT_GT(configuration->num_outputs, 0u);

    configuration->outputs[0].current_mode++;
    configuration->outputs[0].connected = 0;
    EXPECT_NE(nullptr, connection->configure_display(configuration));

    mcl::delete_config_storage(configuration);
}

TEST_F(MirConnectionTest, display_configuration_validation_uses_updated_configuration)
{
    using namespace testing;

    EXPECT_CALL(*mock_channel, connect(_,_))
        .WillOnce(Invoke(fill_display_configuration));

    MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest",
                                                     connected_callback, 0);
    wait_handle->wait_for_all();

    auto old_configuration = connection->create_copy_of_display_config();

    /* Update the configuration */
    uint32_t const output1_id{11};
    uint32_t const output2_id{12};

    mp::DisplayConfiguration protobuf_config;
    auto output1 = protobuf_config.add_display_output();
    output1->set_output_id(output1_id);
    auto output2 = protobuf_config.add_display_output();
    output2->set_output_id(output2_id);

    auto display_config = conf.the_display_configuration();
    display_config->set_configuration(protobuf_config);

    /* Check that the old config cannot be validated */
    EXPECT_EQ(nullptr, connection->configure_display(old_configuration));

    /* Check that the new config can be validated */
    auto configuration = connection->create_copy_of_display_config();
    EXPECT_NE(nullptr, connection->configure_display(configuration));

    mcl::delete_config_storage(old_configuration);
    mcl::delete_config_storage(configuration);
}

TEST_F(MirConnectionTest, populates_pfs_correctly)
{
    using namespace testing;

    EXPECT_CALL(*mock_channel, connect(_,_))
        .WillOnce(Invoke(fill_surface_pixel_formats));
    MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest",
                                                     connected_callback, 0);
    wait_handle->wait_for_all();

    unsigned int const formats_size = 5;
    unsigned int valid_formats = 0;
    MirPixelFormat formats[formats_size];

    connection->available_surface_formats(&formats[0], formats_size, valid_formats);

    ASSERT_EQ(supported_surface_formats.size(), valid_formats);
    for (auto i=0u; i < valid_formats; i++)
    {
        EXPECT_EQ(supported_surface_formats[i], formats[i]) << "i=" << i;
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
    output.current_format = mir_pixel_format_xbgr_8888;
    output.used = 0;
    output.position_x = 4;
    output.position_y = 6;
    output.connected = 0;
    MirDisplayConfiguration user_config{1, &output, 0, nullptr};

    auto verify_display_change = [&](mp::DisplayConfiguration const* config)
    {
        ASSERT_NE(nullptr, config);
        ASSERT_EQ(1, config->display_output_size());
        auto const& disp1 = config->display_output(0);
        EXPECT_TRUE(disp1.has_output_id());
        EXPECT_EQ(output.output_id, disp1.output_id());
        EXPECT_TRUE(disp1.has_used());
        EXPECT_EQ(output.used, disp1.used());
        EXPECT_TRUE(disp1.has_current_mode());
        EXPECT_EQ(output.current_mode, disp1.current_mode());
        EXPECT_TRUE(disp1.has_current_format());
        EXPECT_EQ(output.current_format, disp1.current_format());
        EXPECT_TRUE(disp1.has_position_x());
        EXPECT_EQ(output.position_x, disp1.position_x());
        EXPECT_TRUE(disp1.has_position_y());
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

static MirSurface *surface;
static void surface_callback(MirSurface* surf, void*)
{
    surface = surf;
}

static bool unfocused_received;
static void surface_event_callback(MirSurface *, MirEvent const *ev, void *)
{
    if (mir_event_type_surface != mir_event_get_type(ev))
        return;
    auto surface_ev = mir_event_get_surface_event(ev);
    if (mir_surface_attrib_focus != mir_surface_event_get_attribute(surface_ev))
        return;
    if (mir_surface_unfocused != mir_surface_event_get_attribute_value(surface_ev))
        return;
    unfocused_received = true;
}

TEST_F(MirConnectionTest, focused_window_synthesises_unfocus_event_on_release)
{
    using namespace testing;

    MirSurfaceSpec params{nullptr, 640, 480, mir_pixel_format_abgr_8888};
    params.surface_name = __PRETTY_FUNCTION__;

    unfocused_received = false;

    MirWaitHandle *wait_handle = connection->connect("MirClientSurfaceTest", &connected_callback, nullptr);
    wait_handle->wait_for_all();

    wait_handle = connection->create_surface(params, &surface_callback, nullptr);
    wait_handle->wait_for_all();

    surface->handle_event(*mev::make_event(mf::SurfaceId{surface->id()}, mir_surface_attrib_focus, mir_surface_focused));

    surface->set_event_handler(&surface_event_callback, nullptr);

    wait_handle = connection->release_surface(surface, &surface_callback, nullptr);
    wait_handle->wait_for_all();

    wait_handle = connection->disconnect();
    wait_handle->wait_for_all();

    EXPECT_TRUE(unfocused_received);
}

TEST_F(MirConnectionTest, unfocused_window_does_not_synthesise_unfocus_event_on_release)
{
    using namespace testing;

    MirSurfaceSpec params{nullptr, 640, 480, mir_pixel_format_abgr_8888};
    params.surface_name = __PRETTY_FUNCTION__;

    unfocused_received = false;

    MirWaitHandle *wait_handle = connection->connect("MirClientSurfaceTest", &connected_callback, nullptr);
    wait_handle->wait_for_all();

    wait_handle = connection->create_surface(params, &surface_callback, nullptr);
    wait_handle->wait_for_all();

    surface->handle_event(*mev::make_event(mf::SurfaceId{surface->id()}, mir_surface_attrib_focus, mir_surface_unfocused));

    surface->set_event_handler(&surface_event_callback, nullptr);

    wait_handle = connection->release_surface(surface, &surface_callback, nullptr);
    wait_handle->wait_for_all();

    wait_handle = connection->disconnect();
    wait_handle->wait_for_all();

    EXPECT_FALSE(unfocused_received);
}

namespace
{

void assign_response(MirConnection*, MirPlatformMessage* response, void* context)
{
    auto response_ptr = static_cast<MirPlatformMessage**>(context);
    *response_ptr = response;
}

ACTION(CopyRequestToResponse)
{
    *arg1 = *arg0;
}
}

TEST_F(MirConnectionTest, uses_client_platform_for_platform_operation)
{
    using namespace testing;

    unsigned int const opcode{42};
    auto const request = mir::raii::deleter_for(
        mir_platform_message_create(opcode),
        &mir_platform_message_release);
    auto const response = mir::raii::deleter_for(
        mir_platform_message_create(opcode),
        &mir_platform_message_release);

    EXPECT_CALL(*mock_platform, platform_operation(request.get()))
        .WillOnce(Return(response.get()));
    EXPECT_CALL(*mock_channel, platform_operation(_,_))
        .Times(0);

    auto connect_wh =
        connection->connect("MirClientSurfaceTest", &connected_callback, nullptr);
    mir_wait_for(connect_wh);

    MirPlatformMessage* returned_response{nullptr};

    auto op_wh = connection->platform_operation(
        request.get(), assign_response, &returned_response);
    mir_wait_for(op_wh);

    EXPECT_THAT(returned_response, Eq(response.get()));
}

TEST_F(MirConnectionTest, contacts_server_if_client_platform_cannot_handle_platform_operation)
{
    using namespace testing;

    unsigned int const opcode{42};
    auto const request = mir::raii::deleter_for(
        mir_platform_message_create(opcode),
        &mir_platform_message_release);

    EXPECT_CALL(*mock_platform, platform_operation(_))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*mock_channel, platform_operation(_,_))
        .WillOnce(CopyRequestToResponse());

    auto connect_wh =
        connection->connect("MirClientSurfaceTest", &connected_callback, nullptr);
    mir_wait_for(connect_wh);

    MirPlatformMessage* returned_response{nullptr};

    auto op_wh = connection->platform_operation(
        request.get(), assign_response, &returned_response);
    mir_wait_for(op_wh);

    EXPECT_THAT(mir_platform_message_get_opcode(returned_response), Eq(opcode));
    mir_platform_message_release(returned_response);
}
