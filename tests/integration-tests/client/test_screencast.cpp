/*
 * Copyright © 2014 Canonical Ltd.
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

#include "mir_protobuf.pb.h"
#include "src/client/default_connection_configuration.h"
#include "src/client/rpc/mir_display_server.h"
#include "src/client/rpc/mir_basic_rpc_channel.h"
#include "mir/dispatch/threaded_dispatcher.h"
#include "mir/dispatch/dispatchable.h"

#include "mir/frontend/connector.h"
#include "mir/test/test_protobuf_server.h"
#include "mir/test/stub_server_tool.h"
#include "mir/test/pipe.h"
#include "mir/test/wait_object.h"

#include <gtest/gtest.h>

#include <thread>

namespace mcl = mir::client;
namespace mt = mir::test;
namespace md = mir::dispatch;

namespace
{

struct StubScreencastServerTool : mt::StubServerTool
{
    void create_screencast(
        mir::protobuf::ScreencastParameters const*,
        mir::protobuf::Screencast* response,
        google::protobuf::Closure* done) override
    {
        response->mutable_buffer_stream()->mutable_buffer()->add_fd(pipe.read_fd());
        done->Run();
    }

    void screencast_buffer(
        mir::protobuf::ScreencastId const*,
        mir::protobuf::Buffer* response,
        google::protobuf::Closure* done) override
    {
        response->add_fd(pipe.read_fd());
        done->Run();
    }

    mt::Pipe pipe;
};

struct MirScreencastTest : public testing::Test
{
    MirScreencastTest()
        : server_tool{std::make_shared<StubScreencastServerTool>()}
    {
        std::remove(test_socket);
        test_server =
            std::make_shared<mt::TestProtobufServer>(test_socket, server_tool);
        test_server->comm->start();

        rpc_channel =
            mcl::DefaultConnectionConfiguration{test_socket}.the_rpc_channel();
        protobuf_server =
            std::make_shared<mir::client::rpc::DisplayServer>(rpc_channel);
        eventloop =
            std::make_shared<md::ThreadedDispatcher>(
                "Mir/Client IPC",
                std::dynamic_pointer_cast<md::Dispatchable>(rpc_channel));
    }

    char const* const test_socket = "./test_socket_screencast";
    std::shared_ptr<StubScreencastServerTool> const server_tool;
    std::shared_ptr<mt::TestProtobufServer> test_server;
    std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> rpc_channel;
    std::shared_ptr<mir::protobuf::DisplayServer> protobuf_server;
    std::shared_ptr<mir::dispatch::ThreadedDispatcher> eventloop;
};

}

TEST_F(MirScreencastTest, gets_buffer_fd_when_creating_screencast)
{
    std::vector<char> const cookie{'2','3','l','$'};

    ASSERT_EQ(static_cast<ssize_t>(cookie.size()),
              write(server_tool->pipe.write_fd(), cookie.data(), cookie.size()));

    mir::protobuf::ScreencastParameters protobuf_parameters;
    protobuf_parameters.set_width(0);
    protobuf_parameters.set_height(0);
    protobuf_parameters.mutable_region()->set_left(0);
    protobuf_parameters.mutable_region()->set_top(0);
    protobuf_parameters.mutable_region()->set_width(0);
    protobuf_parameters.mutable_region()->set_height(0);
    protobuf_parameters.set_pixel_format(0);
    mir::protobuf::Screencast protobuf_screencast;

    mt::WaitObject wait_rpc;

    protobuf_server->create_screencast(
        &protobuf_parameters,
        &protobuf_screencast,
        google::protobuf::NewCallback(&wait_rpc, &mt::WaitObject::notify_ready));

    wait_rpc.wait_until_ready();

    ASSERT_EQ(1, protobuf_screencast.buffer_stream().buffer().fd_size());
    auto const read_fd = protobuf_screencast.buffer_stream().buffer().fd(0);

    std::vector<char> received(cookie.size(), '\0');
    EXPECT_EQ(static_cast<ssize_t>(cookie.size()),
              read(read_fd, received.data(), received.size()));
    EXPECT_EQ(cookie, received);

    close(read_fd);
}

TEST_F(MirScreencastTest, gets_buffer_fd_when_getting_screencast_buffer)
{
    std::vector<char> const cookie{'X','%','q','S'};

    ASSERT_EQ(static_cast<ssize_t>(cookie.size()),
              write(server_tool->pipe.write_fd(), cookie.data(), cookie.size()));

    mir::protobuf::ScreencastId protobuf_screencast_id;
    protobuf_screencast_id.set_value(0);
    mir::protobuf::Buffer protobuf_buffer;

    mt::WaitObject wait_rpc;

    protobuf_server->screencast_buffer(
        &protobuf_screencast_id,
        &protobuf_buffer,
        google::protobuf::NewCallback(&wait_rpc, &mt::WaitObject::notify_ready));

    wait_rpc.wait_until_ready();

    ASSERT_EQ(1, protobuf_buffer.fd_size());
    auto const read_fd = protobuf_buffer.fd(0);

    std::vector<char> received(cookie.size(), '\0');
    EXPECT_EQ(static_cast<ssize_t>(cookie.size()),
              read(read_fd, received.data(), received.size()));
    EXPECT_EQ(cookie, received);

    close(read_fd);
}
