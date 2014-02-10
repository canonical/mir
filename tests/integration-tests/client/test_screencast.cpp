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

#include "mir/frontend/connector.h"
#include "mir_test/test_protobuf_server.h"
#include "mir_test/stub_server_tool.h"
#include "mir_test/pipe.h"

#include <gtest/gtest.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
namespace mt = mir::test;

namespace
{

class WaitObject
{
public:
    void notify_ready()
    {
        std::unique_lock<std::mutex> lock{mutex};
        ready = true;
        cv.notify_all();
    }

    void wait_until_ready()
    {
        std::unique_lock<std::mutex> lock{mutex};
        if (!cv.wait_for(lock, std::chrono::seconds{10}, [this] { return ready; }))
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("WaitObject timed out"));
        }
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    bool ready = false;
};

struct StubScreencastServerTool : mt::StubServerTool
{
    void create_screencast(
        google::protobuf::RpcController*,
        const mir::protobuf::ScreencastParameters*,
        mir::protobuf::Screencast* response,
        google::protobuf::Closure* done) override
    {
        response->mutable_buffer()->add_fd(pipe.read_fd());
        done->Run();
    }

    void screencast_buffer(
        ::google::protobuf::RpcController*,
        ::mir::protobuf::ScreencastId const*,
        ::mir::protobuf::Buffer* response,
        ::google::protobuf::Closure* done) override
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
            std::make_shared<mir::protobuf::DisplayServer::Stub>(rpc_channel.get());
    }

    char const* const test_socket = "./test_socket_screencast";
    std::shared_ptr<StubScreencastServerTool> const server_tool;
    std::shared_ptr<mt::TestProtobufServer> test_server;
    std::shared_ptr<google::protobuf::RpcChannel> rpc_channel;
    std::shared_ptr<mir::protobuf::DisplayServer> protobuf_server;
};

}

TEST_F(MirScreencastTest, gets_buffer_fd_when_creating_screencast)
{
    std::vector<char> const cookie{'2','3','l','$'};

    ASSERT_EQ(static_cast<ssize_t>(cookie.size()),
              write(server_tool->pipe.write_fd(), cookie.data(), cookie.size()));

    mir::protobuf::ScreencastParameters protobuf_parameters;
    protobuf_parameters.set_output_id(0);
    protobuf_parameters.set_width(0);
    protobuf_parameters.set_height(0);
    mir::protobuf::Screencast protobuf_screencast;

    WaitObject wait_rpc;

    protobuf_server->create_screencast(
        nullptr,
        &protobuf_parameters,
        &protobuf_screencast,
        google::protobuf::NewCallback(&wait_rpc, &WaitObject::notify_ready));

    wait_rpc.wait_until_ready();

    ASSERT_EQ(1, protobuf_screencast.buffer().fd_size());
    auto const read_fd = protobuf_screencast.buffer().fd(0);

    // The received FD should be different from the original pipe fd,
    // since we are sending it over our IPC mechanism, which for
    // the purposes of this test, lives in the same process.
    // TODO: Don't depend on IPC implementation details
    EXPECT_NE(read_fd, server_tool->pipe.read_fd());

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

    WaitObject wait_rpc;

    protobuf_server->screencast_buffer(
        nullptr,
        &protobuf_screencast_id,
        &protobuf_buffer,
        google::protobuf::NewCallback(&wait_rpc, &WaitObject::notify_ready));

    wait_rpc.wait_until_ready();

    ASSERT_EQ(1, protobuf_buffer.fd_size());
    auto const read_fd = protobuf_buffer.fd(0);

    // The received FD should be different from the original pipe fd,
    // since we are sending it over our IPC mechanism, which, for
    // the purposes of this test, lives in the same process.
    // TODO: Don't depend on IPC implementation details
    EXPECT_NE(read_fd, server_tool->pipe.read_fd());

    std::vector<char> received(cookie.size(), '\0');
    EXPECT_EQ(static_cast<ssize_t>(cookie.size()),
              read(read_fd, received.data(), received.size()));
    EXPECT_EQ(cookie, received);

    close(read_fd);
}
