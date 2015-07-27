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

// TODO: There's a lot to suggest (usage of real connection most prevalent)
// that this is perhaps a set of integration tests. But moving it there conflicts
// with test_mirsurface.cpp. Client MirSurface testing probably needs to be reviewed

#include "mir_protobuf.pb.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir/client_buffer.h"
#include "mir/client_buffer_factory.h"
#include "mir/client_platform.h"
#include "mir/client_platform_factory.h"
#include "src/client/mir_surface.h"
#include "src/client/mir_connection.h"
#include "src/client/default_connection_configuration.h"
#include "src/client/rpc/null_rpc_report.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/dispatch/threaded_dispatcher.h"

#include "mir/frontend/connector.h"
#include "mir/input/input_platform.h"

#include "mir/test/test_protobuf_server.h"
#include "mir/test/stub_server_tool.h"
#include "mir/test/gmock_fixes.h"
#include "mir/test/fake_shared.h"
#include "mir/test/pipe.h"
#include "mir/test/signal.h"
#include "mir/test/doubles/stub_client_buffer.h"
#include "mir/test/test_dispatchable.h"

#include <boost/throw_exception.hpp>
#include "mir/test/doubles/stub_client_buffer_factory.h"
#include "mir/test/doubles/stub_client_buffer_stream_factory.h"
#include "mir/test/doubles/mock_client_buffer_stream_factory.h"
#include "mir/test/doubles/mock_client_buffer_stream.h"

#include <cstring>
#include <map>
#include <atomic>

#include <fcntl.h>

namespace mcl = mir::client;
namespace mircv = mir::input::receiver;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd= mir::test::doubles;

namespace
{

struct MockServerPackageGenerator : public mt::StubServerTool
{
    MockServerPackageGenerator()
        : server_package(),
          width_sent(891), height_sent(458),
          pf_sent(mir_pixel_format_abgr_8888),
          stride_sent(66),
          input_fd(open("/dev/null", O_APPEND)),
          global_buffer_id(0)
    {
    }

    ~MockServerPackageGenerator()
    {
        close(input_fd);
        close_server_package_fds();
    }

    void create_surface(google::protobuf::RpcController*,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done) override
    {
        create_surface_response(response);
        surface_name = request->surface_name();
        done->Run();
    }

    void exchange_buffer(
        ::google::protobuf::RpcController* /*controller*/,
        ::mir::protobuf::BufferRequest const* /*request*/,
        ::mir::protobuf::Buffer* response,
        ::google::protobuf::Closure* done) override
    {
        create_buffer_response(response);
        done->Run();
    }

    MirBufferPackage server_package;
    int width_sent;
    int height_sent;
    int pf_sent;
    int stride_sent;

    static std::map<int, int> sent_surface_attributes;

private:
    void generate_unique_buffer()
    {
        global_buffer_id++;

        close_server_package_fds();

        int num_fd = 2, num_data = 8;

        server_package.fd_items = num_fd;
        for (auto i = 0; i < num_fd; i++)
        {
            server_package.fd[i] = open("/dev/null", O_APPEND);
        }

        server_package.data_items = num_data;
        for (auto i = 0; i < num_data; i++)
        {
            server_package.data[i] = (global_buffer_id + i) * 2;
        }

        server_package.stride = stride_sent;
        server_package.width = width_sent;
        server_package.height = height_sent;
    }

    void create_buffer_response(mir::protobuf::Buffer* response)
    {
        generate_unique_buffer();

        response->set_buffer_id(global_buffer_id);

        /* assemble buffers */
        response->set_fds_on_side_channel(server_package.fd_items);
        for (int i=0; i< server_package.data_items; i++)
        {
            response->add_data(server_package.data[i]);
        }
        for (int i=0; i< server_package.fd_items; i++)
        {
            response->add_fd(server_package.fd[i]);
        }

        response->set_stride(server_package.stride);
        response->set_width(server_package.width);
        response->set_height(server_package.height);
    }
    
    void create_surface_response(mir::protobuf::Surface* response)
    {
        unsigned int const id = 2;

        response->set_fds_on_side_channel(1);

        response->mutable_id()->set_value(id);
        response->set_width(width_sent);
        response->set_height(height_sent);
        response->set_pixel_format(pf_sent);
        response->add_fd(input_fd);
        
        for (auto const& kv : sent_surface_attributes)
        {
            auto setting = response->add_attributes();
            setting->mutable_surfaceid()->set_value(id);
            setting->set_attrib(kv.first);
            setting->set_ivalue(kv.second);            
        }

        create_buffer_response(response->mutable_buffer());
    }

    void close_server_package_fds()
    {
        for (int i = 0; i < server_package.fd_items; i++)
            close(server_package.fd[i]);
    }

    int input_fd;
    int global_buffer_id;
};

std::map<int, int> MockServerPackageGenerator::sent_surface_attributes = {
    { mir_surface_attrib_type, mir_surface_type_normal },
    { mir_surface_attrib_state, mir_surface_state_restored },
    { mir_surface_attrib_swapinterval, 1 },
    { mir_surface_attrib_focus, mir_surface_focused },
    { mir_surface_attrib_dpi, 19 },
    { mir_surface_attrib_visibility, mir_surface_visibility_exposed },
    { mir_surface_attrib_preferred_orientation, mir_orientation_mode_any }
};

// TODO: Deduplicate this class?
struct StubClientPlatform : public mcl::ClientPlatform
{
    MirPlatformType platform_type() const
    {
        return mir_platform_type_android;
    }

    void populate(MirPlatformPackage&) const override
    {
    }

    MirPlatformMessage* platform_operation(
        MirPlatformMessage const*) override
    {
        return nullptr;
    }

    std::shared_ptr<mcl::ClientBufferFactory> create_buffer_factory()
    {
        return std::make_shared<mtd::StubClientBufferFactory>();
    }

    std::shared_ptr<EGLNativeWindowType> create_egl_native_window(mcl::EGLNativeSurface* /*surface*/)
    {
        return std::shared_ptr<EGLNativeWindowType>();
    }

    std::shared_ptr<EGLNativeDisplayType> create_egl_native_display()
    {
        return std::shared_ptr<EGLNativeDisplayType>();
    }

    MirNativeBuffer* convert_native_buffer(mir::graphics::NativeBuffer*) const
    {
        return nullptr;
    }

    MirPixelFormat get_egl_pixel_format(EGLDisplay, EGLConfig) const override
    {
        return mir_pixel_format_invalid;
    }
};

struct StubClientPlatformFactory : public mcl::ClientPlatformFactory
{
    std::shared_ptr<mcl::ClientPlatform> create_client_platform(mcl::ClientContext* /*context*/)
    {
        return std::make_shared<StubClientPlatform>();
    }
};

struct StubClientInputPlatform : public mircv::InputPlatform
{
    std::shared_ptr<mir::dispatch::Dispatchable> create_input_receiver(int /* fd */, std::shared_ptr<mircv::XKBMapper> const&, std::function<void(MirEvent*)> const& /* callback */)
    {
        return std::shared_ptr<mir::dispatch::Dispatchable>();
    }
};

struct MockClientInputPlatform : public mircv::InputPlatform
{
    MOCK_METHOD3(create_input_receiver, std::shared_ptr<mir::dispatch::Dispatchable>(int, std::shared_ptr<mircv::XKBMapper> const&, std::function<void(MirEvent*)> const&));
};

class TestConnectionConfiguration : public mcl::DefaultConnectionConfiguration
{
public:
    TestConnectionConfiguration()
        : DefaultConnectionConfiguration("./test_socket_surface")
    {
    }

    std::shared_ptr<mcl::rpc::RpcReport> the_rpc_report() override
    {
        return std::make_shared<mcl::rpc::NullRpcReport>();
    }

    std::shared_ptr<mcl::ClientPlatformFactory> the_client_platform_factory() override
    {
        return std::make_shared<StubClientPlatformFactory>();
    }
};

struct FakeRpcChannel : public ::google::protobuf::RpcChannel
{
    void CallMethod(const google::protobuf::MethodDescriptor*,
                    google::protobuf::RpcController*,
                    const google::protobuf::Message*,
                    google::protobuf::Message*,
                    google::protobuf::Closure* closure) override
    {
        delete closure;
    }
};

void null_connected_callback(MirConnection* /*connection*/, void * /*client_context*/)
{
}

void null_surface_callback(MirSurface*, void*)
{
}

void null_event_callback(MirSurface*, MirEvent const*, void*)
{
}

void null_lifecycle_callback(MirConnection*, MirLifecycleState, void*)
{
}

struct MirClientSurfaceTest : public testing::Test
{
    MirClientSurfaceTest()
    {
        start_test_server();
        connect_to_test_server();
    }

    ~MirClientSurfaceTest()
    {
        // Clear the lifecycle callback in order not to get SIGHUP by the
        // default lifecycle handler during connection teardown
        connection->register_lifecycle_event_callback(null_lifecycle_callback, nullptr);
    }

    void start_test_server()
    {
        // In case an earlier test left a stray file
        std::remove("./test_socket_surface");
        test_server = std::make_shared<mt::TestProtobufServer>("./test_socket_surface", mock_server_tool);
        test_server->comm->start();
    }

    void connect_to_test_server()
    {
        mir::protobuf::ConnectParameters connect_parameters;
        connect_parameters.set_application_name("test");

        TestConnectionConfiguration conf;
        connection = std::make_shared<MirConnection>(conf);
        MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest",
                                                         null_connected_callback, 0);
        wait_handle->wait_for_all();
        client_comm_channel = std::make_shared<mir::protobuf::DisplayServer::Stub>(
                                  conf.the_rpc_channel().get());
    }

    std::shared_ptr<MirSurface> create_surface_with(
        mir::protobuf::DisplayServer::Stub& server_stub)
    {
        return std::make_shared<MirSurface>(
            connection.get(),
            server_stub,
            nullptr,
            stub_buffer_stream_factory,
            input_platform,
            spec,
            &null_surface_callback,
            nullptr);
    }

    std::shared_ptr<MirSurface> create_surface_with(
        mir::protobuf::DisplayServer::Stub& server_stub,
        std::shared_ptr<mcl::ClientBufferStreamFactory> const& buffer_stream_factory)
    {
        return std::make_shared<MirSurface>(
            connection.get(),
            server_stub,
            nullptr,
            buffer_stream_factory,
            input_platform,
            spec,
            &null_surface_callback,
            nullptr);
    }

    std::shared_ptr<MirSurface> create_and_wait_for_surface_with(
        mir::protobuf::DisplayServer::Stub& server_stub)
    {
        auto surface = create_surface_with(server_stub);
        surface->get_create_wait_handle()->wait_for_all();
        return surface;
    }

    std::shared_ptr<MirSurface> create_and_wait_for_surface_with(
        mir::protobuf::DisplayServer::Stub& server_stub,
        std::shared_ptr<mcl::ClientBufferStreamFactory> const& buffer_stream_factory)
    {
        auto surface = create_surface_with(server_stub, buffer_stream_factory);
        surface->get_create_wait_handle()->wait_for_all();
        return surface;
    }

    std::shared_ptr<MirConnection> connection;

    MirSurfaceSpec const spec{nullptr, 33, 45, mir_pixel_format_abgr_8888};
    std::shared_ptr<mtd::StubClientBufferStreamFactory> const stub_buffer_stream_factory =
        std::make_shared<mtd::StubClientBufferStreamFactory>();
    std::shared_ptr<StubClientInputPlatform> const input_platform =
        std::make_shared<StubClientInputPlatform>();
    std::shared_ptr<MockServerPackageGenerator> const mock_server_tool =
        std::make_shared<MockServerPackageGenerator>();

    std::shared_ptr<mt::TestProtobufServer> test_server;
    std::shared_ptr<mir::protobuf::DisplayServer::Stub> client_comm_channel;

    std::chrono::milliseconds const pause_time{10};
};

}

TEST_F(MirClientSurfaceTest, attributes_set_on_surface_creation)
{
    using namespace testing;

    auto surface = create_and_wait_for_surface_with(*client_comm_channel);
    
    for (int i = 0; i < mir_surface_attribs; i++)
    {
        EXPECT_EQ(MockServerPackageGenerator::sent_surface_attributes[i], surface->attrib(static_cast<MirSurfaceAttrib>(i)));
    }
}

TEST_F(MirClientSurfaceTest, create_wait_handle_really_blocks)
{
    using namespace testing;

    FakeRpcChannel fake_channel;
    mir::protobuf::DisplayServer::Stub unresponsive_server{&fake_channel};

    auto const surface = create_surface_with(unresponsive_server);
    auto wait_handle = surface->get_create_wait_handle();

    auto expected_end = std::chrono::steady_clock::now() + pause_time;
    wait_handle->wait_for_pending(pause_time);

    EXPECT_GE(std::chrono::steady_clock::now(), expected_end);
}

TEST_F(MirClientSurfaceTest, creates_input_thread_with_input_dispatcher_when_delegate_specified)
{
    using namespace ::testing;

    auto dispatched = std::make_shared<mt::Signal>();

    auto mock_input_dispatcher = std::make_shared<mt::TestDispatchable>([dispatched]() { dispatched->raise(); });
    auto mock_input_platform = std::make_shared<MockClientInputPlatform>();

    EXPECT_CALL(*mock_input_platform, create_input_receiver(_, _, _)).Times(1)
        .WillOnce(Return(mock_input_dispatcher));

    MirSurface surface{connection.get(), *client_comm_channel, nullptr,
        stub_buffer_stream_factory, mock_input_platform, spec, &null_surface_callback, nullptr};
    auto wait_handle = surface.get_create_wait_handle();
    wait_handle->wait_for_all();
    surface.set_event_handler(null_event_callback, nullptr);

    mock_input_dispatcher->trigger();

    EXPECT_TRUE(dispatched->wait_for(std::chrono::seconds{5}));
}

TEST_F(MirClientSurfaceTest, replacing_delegate_with_nullptr_prevents_further_dispatch)
{
    using namespace ::testing;

    auto dispatched = std::make_shared<mt::Signal>();

    auto mock_input_dispatcher = std::make_shared<mt::TestDispatchable>([dispatched]() { dispatched->raise(); });
    auto mock_input_platform = std::make_shared<MockClientInputPlatform>();

    EXPECT_CALL(*mock_input_platform, create_input_receiver(_, _, _)).Times(1)
        .WillOnce(Return(mock_input_dispatcher));

    MirSurface surface{connection.get(), *client_comm_channel, nullptr,
        stub_buffer_stream_factory, mock_input_platform, spec, &null_surface_callback, nullptr};
    auto wait_handle = surface.get_create_wait_handle();
    wait_handle->wait_for_all();
    surface.set_event_handler(null_event_callback, nullptr);

    // Should now not get dispatched.
    surface.set_event_handler(nullptr, nullptr);

    mock_input_dispatcher->trigger();

    EXPECT_FALSE(dispatched->wait_for(std::chrono::seconds{1}));
}


TEST_F(MirClientSurfaceTest, does_not_create_input_dispatcher_when_no_delegate_specified)
{
    using namespace ::testing;

    auto mock_input_platform = std::make_shared<MockClientInputPlatform>();

    EXPECT_CALL(*mock_input_platform, create_input_receiver(_, _, _)).Times(0);

    MirSurface surface{connection.get(), *client_comm_channel, nullptr,
        stub_buffer_stream_factory, mock_input_platform, spec, &null_surface_callback, nullptr};
    auto wait_handle = surface.get_create_wait_handle();
    wait_handle->wait_for_all();
}

TEST_F(MirClientSurfaceTest, valid_surface_is_valid)
{
    auto const surface = create_and_wait_for_surface_with(*client_comm_channel);

    EXPECT_TRUE(MirSurface::is_valid(surface.get()));
}

TEST_F(MirClientSurfaceTest, configure_cursor_wait_handle_really_blocks)
{
    using namespace testing;

    FakeRpcChannel fake_channel;
    mir::protobuf::DisplayServer::Stub unresponsive_server{&fake_channel};

    auto const surface = create_surface_with(unresponsive_server);

    auto cursor_config = mir_cursor_configuration_from_name(mir_default_cursor_name);
    auto cursor_wait_handle = surface->configure_cursor(cursor_config);

    auto expected_end = std::chrono::steady_clock::now() + pause_time;
    cursor_wait_handle->wait_for_pending(pause_time);

    EXPECT_GE(std::chrono::steady_clock::now(), expected_end);

    mir_cursor_configuration_destroy(cursor_config);
}

TEST_F(MirClientSurfaceTest, configure_wait_handle_really_blocks)
{
    using namespace testing;

    FakeRpcChannel fake_channel;
    mir::protobuf::DisplayServer::Stub unresponsive_server{&fake_channel};

    auto const surface = create_surface_with(unresponsive_server);

    auto configure_wait_handle = surface->configure(mir_surface_attrib_dpi, 100);

    auto expected_end = std::chrono::steady_clock::now() + pause_time;
    configure_wait_handle->wait_for_pending(pause_time);

    EXPECT_GE(std::chrono::steady_clock::now(), expected_end);
}
