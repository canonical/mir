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

#include "mir_protobuf.pb.h"
#include "mir_toolkit/mir_client_library.h"
#include "src/client/client_buffer.h"
#include "src/client/client_buffer_factory.h"
#include "src/client/client_platform.h"
#include "src/client/client_platform_factory.h"
#include "src/client/mir_surface.h"
#include "src/client/mir_connection.h"
#include "src/client/default_connection_configuration.h"
#include "src/client/rpc/null_rpc_report.h"
#include "src/client/rpc/make_rpc_channel.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"

#include "mir/frontend/resource_cache.h"
#include "mir/frontend/communicator.h"
#include "mir/input/input_platform.h"
#include "mir/input/input_receiver_thread.h"

#include "mir_test/test_protobuf_server.h"
#include "mir_test/stub_server_tool.h"
#include "mir_test/test_protobuf_client.h"
#include "mir_test/gmock_fixes.h"
#include "mir_test/fake_shared.h"

#include <cstring>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fcntl.h>

namespace mcl = mir::client;
namespace mircv = mir::input::receiver;
namespace mp = mir::protobuf;
namespace geom = mir::geometry;

namespace mir
{
namespace test
{

struct MockServerPackageGenerator : public StubServerTool
{
    MockServerPackageGenerator()
     : global_buffer_id(0)
    {
        width_sent  = 891;
        height_sent = 458;
        pf_sent = mir_pixel_format_abgr_8888;
        stride_sent = 66;

        input_fd = open("/dev/null", O_APPEND);
    }
    ~MockServerPackageGenerator()
    {
        close(input_fd);
        for (int i = 0; i < server_package.fd_items; i++)
            close(server_package.fd[0]);
    }

    void create_surface(google::protobuf::RpcController*,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        create_surface_response(response);
        surface_name = request->surface_name();
        done->Run();
    }

    void next_buffer(
        ::google::protobuf::RpcController* /*controller*/,
        ::mir::protobuf::SurfaceId const* /*request*/,
        ::mir::protobuf::Buffer* response,
        ::google::protobuf::Closure* done)
    {
        create_buffer_response(response);
        done->Run();
    }

/* helpers */
    void generate_unique_buffer()
    {
        global_buffer_id++;

        int num_fd = 2, num_data = 8;
        for (auto i=0; i<num_fd; i++)
        {
            if (server_package.fd[i])
                close(server_package.fd[i]);
            server_package.fd[i] = open("/dev/null", O_APPEND);
        }
        server_package.fd_items = num_fd;
        server_package.data_items = num_data;
        for (auto i=0; i<num_data; i++)
        {
            server_package.data[i] = (global_buffer_id + i) * 2;
        }
        server_package.stride = stride_sent;
    }

    MirBufferPackage server_package;

    int width_sent;
    int height_sent;
    int pf_sent;
    int stride_sent;
    
    int input_fd;

    private:
    int global_buffer_id;

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
    }

    void create_surface_response(mir::protobuf::Surface* response)
    {
        response->set_fds_on_side_channel(1);

        response->mutable_id()->set_value(2);
        response->set_width(width_sent);
        response->set_height(height_sent);
        response->set_pixel_format(pf_sent);
        response->add_fd(input_fd);

        create_buffer_response(response->mutable_buffer());
    }
};

struct MockBuffer : public mcl::ClientBuffer
{
    MockBuffer()
    {
    }
    ~MockBuffer() noexcept
    {
    }

    MOCK_METHOD0(secure_for_cpu_write, std::shared_ptr<mcl::MemoryRegion>());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(stride, geom::Stride());
    MOCK_CONST_METHOD0(pixel_format, geom::PixelFormat());
    MOCK_CONST_METHOD0(age, uint32_t());
    MOCK_METHOD0(increment_age, void());
    MOCK_METHOD0(mark_as_submitted, void());
    MOCK_CONST_METHOD0(native_buffer_handle, std::shared_ptr<MirNativeBuffer>());
};

struct MockClientBufferFactory : public mcl::ClientBufferFactory
{
    MockClientBufferFactory()
    {
        using namespace testing;

        emptybuffer=std::make_shared<NiceMock<MockBuffer>>();

        ON_CALL(*this, create_buffer(_,_,_))
            .WillByDefault(DoAll(SaveArg<0>(&current_package),
                                 InvokeWithoutArgs([this] () {this->current_buffer = std::make_shared<NiceMock<MockBuffer>>();}),
                                 ReturnPointee(&current_buffer)));
    }

    MOCK_METHOD3(create_buffer,
                 std::shared_ptr<mcl::ClientBuffer>(std::shared_ptr<MirBufferPackage> const&,
                                                    geom::Size, geom::PixelFormat));

    std::shared_ptr<MirBufferPackage> current_package;
    std::shared_ptr<mcl::ClientBuffer> current_buffer;
    std::shared_ptr<mcl::ClientBuffer> emptybuffer;
};

struct StubClientPlatform : public mcl::ClientPlatform
{
    MirPlatformType platform_type() const
    {
        return mir_platform_type_android; 
    } 
    std::shared_ptr<mcl::ClientBufferFactory> create_buffer_factory()
    {
        return std::shared_ptr<MockClientBufferFactory>();
    }

    std::shared_ptr<EGLNativeWindowType> create_egl_native_window(mcl::ClientSurface* /*surface*/)
    {
        return std::shared_ptr<EGLNativeWindowType>();
    }

    std::shared_ptr<EGLNativeDisplayType> create_egl_native_display()
    {
        return std::shared_ptr<EGLNativeDisplayType>();
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
    std::shared_ptr<mircv::InputReceiverThread> create_input_thread(int /* fd */, std::function<void(MirEvent*)> const& /* callback */)
    {
        return std::shared_ptr<mircv::InputReceiverThread>();
    }
};

struct MockClientInputPlatform : public mircv::InputPlatform
{
    MOCK_METHOD2(create_input_thread, std::shared_ptr<mircv::InputReceiverThread>(int, std::function<void(MirEvent*)> const&));
};

struct MockInputReceiverThread : public mircv::InputReceiverThread
{
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());
    MOCK_METHOD0(join, void());
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

}
}

namespace mt = mir::test;

void connected_callback(MirConnection* /*connection*/, void * /*client_context*/)
{
}

struct CallBack
{
    void msg() {}
};

struct MirClientSurfaceTest : public testing::Test
{
    void SetUp()
    {
        mock_server_tool = std::make_shared<mt::MockServerPackageGenerator>();
        test_server = std::make_shared<mt::TestProtobufServer>("./test_socket_surface", mock_server_tool);

        test_server->comm->start();

        mock_buffer_factory = std::make_shared<mt::MockClientBufferFactory>();

        input_platform = std::make_shared<mt::StubClientInputPlatform>();

        params = MirSurfaceParameters{"test", 33, 45, mir_pixel_format_abgr_8888,
                                      mir_buffer_usage_hardware,
                                      mir_display_output_id_invalid};

        /* connect dummy server */
        connect_parameters.set_application_name("test");

        /* connect client */
        mt::TestConnectionConfiguration conf;
        connection = std::make_shared<MirConnection>(conf);
        MirWaitHandle* wait_handle = connection->connect("MirClientSurfaceTest",
                                                         connected_callback, 0);
        wait_handle->wait_for_all();
        client_comm_channel = std::make_shared<mir::protobuf::DisplayServer::Stub>(
                                  conf.the_rpc_channel().get());
    }

    void TearDown()
    {
        test_server.reset();
    }

    std::shared_ptr<MirConnection> connection;

    MirSurfaceParameters params;
    std::shared_ptr<mt::MockClientBufferFactory> mock_buffer_factory;
    std::shared_ptr<mt::StubClientInputPlatform> input_platform;

    mir::protobuf::Connection response;
    mir::protobuf::ConnectParameters connect_parameters;

    std::shared_ptr<mt::TestProtobufServer> test_server;
    std::shared_ptr<mt::TestProtobufClient> client_tools;
    std::shared_ptr<mt::MockServerPackageGenerator> mock_server_tool;

    CallBack callback;

    std::shared_ptr<mir::protobuf::DisplayServer::Stub> client_comm_channel;
};

void empty_callback(MirSurface*, void*) { }
TEST_F(MirClientSurfaceTest, client_buffer_created_on_surface_creation)
{
    using namespace testing;

    EXPECT_CALL(*mock_buffer_factory, create_buffer(_,_,_))
        .Times(1);

    auto surface = std::make_shared<MirSurface> (connection.get(), *client_comm_channel, mock_buffer_factory, input_platform, params, &empty_callback, nullptr);

    auto wait_handle = surface->get_create_wait_handle();
    wait_handle->wait_for_all();
}

namespace
{
void empty_surface_callback(MirSurface*, void*) {}
}

TEST_F(MirClientSurfaceTest, client_buffer_created_on_next_buffer)
{
    using namespace testing;

    EXPECT_CALL(*mock_buffer_factory, create_buffer(_,_,_))
        .Times(1);

    auto surface = std::make_shared<MirSurface> (connection.get(), *client_comm_channel, mock_buffer_factory, input_platform, params, &empty_callback, nullptr);

    auto wait_handle = surface->get_create_wait_handle();
    wait_handle->wait_for_all();

    EXPECT_CALL(*mock_buffer_factory, create_buffer(_,_,_))
        .Times(1);
    auto buffer_wait_handle = surface->next_buffer(&empty_surface_callback, nullptr);
    buffer_wait_handle->wait_for_all();
}

MATCHER_P(BufferPackageMatches, package, "")
{
    // Can't simply use memcmp() on the whole struct because age is not sent over the wire
    if (package.data_items != arg.data_items)
        return false;
    // Note we can not compare the fd's directly as they may change when being sent over the wire.
    if (package.fd_items != arg.fd_items)
        return false;
    if (memcmp(package.data, arg.data, sizeof(package.data[0]) * package.data_items))
        return false;
    if (package.stride != arg.stride)
        return false;
    return true;
}

TEST_F(MirClientSurfaceTest, client_buffer_uses_ipc_message_from_server_on_create)
{
    using namespace testing;

    std::shared_ptr<MirBufferPackage> submitted_package;
    EXPECT_CALL(*mock_buffer_factory, create_buffer(_,_,_))
        .Times(1)
        .WillOnce(DoAll(SaveArg<0>(&submitted_package),
                        Return(mock_buffer_factory->emptybuffer)));

    auto surface = std::make_shared<MirSurface> (connection.get(), *client_comm_channel, mock_buffer_factory, input_platform, params, &empty_callback, nullptr);

    auto wait_handle = surface->get_create_wait_handle();
    wait_handle->wait_for_all();

    /* check for same contents */
    EXPECT_THAT(*submitted_package, BufferPackageMatches(mock_server_tool->server_package));
}

TEST_F(MirClientSurfaceTest, message_width_used_in_buffer_creation )
{
    using namespace testing;

    geom::Size sz;
    std::shared_ptr<MirBufferPackage> submitted_package;

    EXPECT_CALL(*mock_buffer_factory, create_buffer(_,_,_))
        .Times(1)
        .WillOnce(DoAll(SaveArg<1>(&sz),
                        Return(mock_buffer_factory->emptybuffer)));

    auto surface = std::make_shared<MirSurface> (connection.get(), *client_comm_channel, mock_buffer_factory, input_platform, params, &empty_callback, nullptr);

    auto wait_handle = surface->get_create_wait_handle();
    wait_handle->wait_for_all();

    EXPECT_EQ(sz.width.as_uint32_t(), (unsigned int) mock_server_tool->width_sent);
}

TEST_F(MirClientSurfaceTest, message_height_used_in_buffer_creation )
{
    using namespace testing;

    geom::Size sz;
    std::shared_ptr<MirBufferPackage> submitted_package;

    EXPECT_CALL(*mock_buffer_factory, create_buffer(_,_,_))
        .Times(1)
        .WillOnce(DoAll(SaveArg<1>(&sz),
                        Return(mock_buffer_factory->emptybuffer)));

    auto surface = std::make_shared<MirSurface> (connection.get(), *client_comm_channel, mock_buffer_factory, input_platform, params, &empty_callback, nullptr);

    auto wait_handle = surface->get_create_wait_handle();
    wait_handle->wait_for_all();

    EXPECT_EQ(sz.height.as_uint32_t(), (unsigned int) mock_server_tool->height_sent);
}

TEST_F(MirClientSurfaceTest, message_pf_used_in_buffer_creation )
{
    using namespace testing;

    geom::PixelFormat pf;
    std::shared_ptr<MirBufferPackage> submitted_package;

    EXPECT_CALL(*mock_buffer_factory, create_buffer(_,_,_))
        .Times(1)
        .WillOnce(DoAll(SaveArg<2>(&pf),
                        Return(mock_buffer_factory->emptybuffer)));

    auto surface = std::make_shared<MirSurface> (connection.get(), *client_comm_channel, mock_buffer_factory, input_platform, params, &empty_callback, nullptr);

    auto wait_handle = surface->get_create_wait_handle();
    wait_handle->wait_for_all();

    EXPECT_EQ(pf, geom::PixelFormat::abgr_8888);
}



namespace
{
static void null_event_callback(MirSurface*, MirEvent const*, void*)
{
}
}

TEST_F(MirClientSurfaceTest, input_fd_used_to_create_input_thread_when_delegate_specified)
{
    using namespace ::testing;
    
    auto mock_input_platform = std::make_shared<mt::MockClientInputPlatform>();
    auto mock_input_thread = std::make_shared<NiceMock<mt::MockInputReceiverThread>>();
    MirEventDelegate delegate = {null_event_callback, nullptr};

    EXPECT_CALL(*mock_buffer_factory, create_buffer(_,_,_)).Times(2);

    EXPECT_CALL(*mock_input_platform, create_input_thread(_, _)).Times(1)
        .WillOnce(Return(mock_input_thread));
    EXPECT_CALL(*mock_input_thread, start()).Times(1);
    EXPECT_CALL(*mock_input_thread, stop()).Times(1);

    {
        auto surface = std::make_shared<MirSurface> (connection.get(), *client_comm_channel,
            mock_buffer_factory, mock_input_platform, params, &empty_callback, nullptr);
        auto wait_handle = surface->get_create_wait_handle();
        wait_handle->wait_for_all();
        surface->set_event_handler(&delegate);
    }

    {
        // This surface should not trigger a call to the input platform as no input delegate is specified.
        auto surface = std::make_shared<MirSurface> (connection.get(), *client_comm_channel,
            mock_buffer_factory, mock_input_platform, params, &empty_callback, nullptr);
        auto wait_handle = surface->get_create_wait_handle();
        wait_handle->wait_for_all();
    }
}

TEST_F(MirClientSurfaceTest, get_buffer_returns_last_received_buffer_package)
{
    using namespace testing;

    EXPECT_CALL(*mock_buffer_factory, create_buffer(_,_,_))
        .Times(1);

    auto surface = std::make_shared<MirSurface> (connection.get(),
                                                 *client_comm_channel,
                                                 mock_buffer_factory,
                                                 input_platform,
                                                 params,
                                                 &empty_callback,
                                                 nullptr);
    auto wait_handle = surface->get_create_wait_handle();
    wait_handle->wait_for_all();
    Mock::VerifyAndClearExpectations(mock_buffer_factory.get());

    EXPECT_CALL(*mock_buffer_factory, create_buffer(_,_,_))
        .Times(1);
    auto buffer_wait_handle = surface->next_buffer(&empty_surface_callback, nullptr);
    buffer_wait_handle->wait_for_all();
    Mock::VerifyAndClearExpectations(mock_buffer_factory.get());
}

TEST_F(MirClientSurfaceTest, default_surface_type)
{
    using namespace testing;
    using namespace mir::protobuf;

    EXPECT_CALL(*mock_buffer_factory, create_buffer(_,_,_))
        .Times(1);

    auto surface = std::make_shared<MirSurface> (connection.get(),
                                                 *client_comm_channel,
                                                 mock_buffer_factory,
                                                 input_platform,
                                                 params,
                                                 &empty_callback,
                                                 nullptr);
    surface->get_create_wait_handle()->wait_for_all();

    EXPECT_EQ(mir_surface_type_normal,
              surface->attrib(mir_surface_attrib_type));
}

TEST_F(MirClientSurfaceTest, default_surface_state)
{
    auto surface = std::make_shared<MirSurface> (connection.get(),
                                                 *client_comm_channel,
                                                 mock_buffer_factory,
                                                 input_platform,
                                                 params,
                                                 &empty_callback,
                                                 nullptr);
    surface->get_create_wait_handle()->wait_for_all();

    // Test the default cached state value. It is always unknown until we
    // get a real answer from the server.
    EXPECT_EQ(mir_surface_state_unknown,
              surface->attrib(mir_surface_attrib_state));
}

namespace
{

struct StubBuffer : public mcl::ClientBuffer
{
    StubBuffer(geom::Size size, geom::Stride stride, geom::PixelFormat pf)
        : size_{size}, stride_{stride}, pf_{pf}
    {
    }

    std::shared_ptr<mcl::MemoryRegion> secure_for_cpu_write()
    {
        auto raw = new mcl::MemoryRegion{size_.width,
                                         size_.height,
                                         stride_,
                                         pf_,
                                         nullptr};

        return std::shared_ptr<mcl::MemoryRegion>(raw);
    }

    geom::Size size() const { return size_; }
    geom::Stride stride() const { return stride_; }
    geom::PixelFormat pixel_format() const { return pf_; }
    uint32_t age() const { return 0; }
    void increment_age() {}
    void mark_as_submitted() {}

    std::shared_ptr<MirNativeBuffer> native_buffer_handle() const
    {
        return std::shared_ptr<MirNativeBuffer>();
    }

    geom::Size size_;
    geom::Stride stride_;
    geom::PixelFormat pf_;
};

struct StubClientBufferFactory : public mcl::ClientBufferFactory
{
    std::shared_ptr<mcl::ClientBuffer> create_buffer(
        std::shared_ptr<MirBufferPackage> const& package,
        geom::Size size, geom::PixelFormat pf)
    {
        return std::make_shared<StubBuffer>(size,
                                            geom::Stride{package->stride},
                                            pf);
    }
};

}
TEST_F(MirClientSurfaceTest, get_cpu_region_returns_correct_data)
{
    using namespace testing;

    struct TestDataEntry
    {
        int width;
        int height;
        int stride;
        MirPixelFormat pf;
    };

    std::vector<TestDataEntry> test_data{
        {100, 200, 300, mir_pixel_format_argb_8888},
        {101, 201, 301, mir_pixel_format_xrgb_8888},
        {102, 202, 302, mir_pixel_format_bgr_888}
    };

    auto stub_buffer_factory = std::make_shared<StubClientBufferFactory>();

    for (auto& td : test_data)
    {
        mock_server_tool->width_sent = td.width;
        mock_server_tool->height_sent = td.height;
        mock_server_tool->stride_sent = td.stride;
        mock_server_tool->pf_sent = td.pf;

        auto surface = std::make_shared<MirSurface>(connection.get(),
                                                    *client_comm_channel,
                                                    stub_buffer_factory,
                                                    input_platform,
                                                    params,
                                                    &empty_callback,
                                                    nullptr);

        auto wait_handle = surface->get_create_wait_handle();
        wait_handle->wait_for_all();

        MirGraphicsRegion region;
        surface->get_cpu_region(region);

        EXPECT_EQ(mock_server_tool->width_sent, region.width);
        EXPECT_EQ(mock_server_tool->height_sent, region.height);
        EXPECT_EQ(mock_server_tool->stride_sent, region.stride);
        EXPECT_EQ(mock_server_tool->pf_sent, region.pixel_format);
    }
}
