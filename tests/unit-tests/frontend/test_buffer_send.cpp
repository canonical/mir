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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/frontend/protobuf_asio_communicator.h"
#include "mir/frontend/resource_cache.h"

#include "mir_protobuf.pb.h"
#include "mir_client/mir_rpc_channel.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <stdexcept>
#include <memory>

namespace mf = mir::frontend;

namespace mir
{
namespace
{
struct StubServer : mir::protobuf::DisplayServer
{
    static const int file_descriptors = 5;

    std::string app_name;
    std::string surface_name;
    int surface_count;
    std::mutex guard;
    std::condition_variable wait_condition;
    int file_descriptor[file_descriptors];

    StubServer() : surface_count(0)
    {
        for (auto i = file_descriptor; i != file_descriptor+file_descriptors; ++i)
            *i = 0;
    }

    StubServer(StubServer const &) = delete;
    void create_surface(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        response->mutable_id()->set_value(13); // TODO distinct numbers & tracking
        response->set_width(request->width());
        response->set_height(request->height());
        response->set_pixel_format(request->pixel_format());
        response->mutable_buffer();

        std::unique_lock<std::mutex> lock(guard);
        surface_name = request->surface_name();
        ++surface_count;
        wait_condition.notify_one();

        done->Run();
    }

    void next_buffer(
        ::google::protobuf::RpcController* /*controller*/,
        ::mir::protobuf::SurfaceId const* /*request*/,
        ::mir::protobuf::Buffer* /*response*/,
        ::google::protobuf::Closure* done)
    {
        std::unique_lock<std::mutex> lock(guard);
        wait_condition.notify_one();
        done->Run();
    }


    void release_surface(::google::protobuf::RpcController* /*controller*/,
                         const ::mir::protobuf::SurfaceId* /*request*/,
                         ::mir::protobuf::Void* /*response*/,
                         ::google::protobuf::Closure* /*done*/)
    {
        // TODO need some tests for releasing surfaces
    }


    void connect(
        ::google::protobuf::RpcController*,
                         const ::mir::protobuf::ConnectParameters* request,
                         ::mir::protobuf::Connection*,
                         ::google::protobuf::Closure* done)
    {
        app_name = request->application_name();
        done->Run();
    }

    void disconnect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::Void* /*request*/,
                 mir::protobuf::Void* /*response*/,
                 google::protobuf::Closure* done)
    {
        std::unique_lock<std::mutex> lock(guard);
        wait_condition.notify_one();
        done->Run();
    }

    void test_file_descriptors(::google::protobuf::RpcController* ,
                         const ::mir::protobuf::Void* ,
                         ::mir::protobuf::Buffer* fds,
                         ::google::protobuf::Closure* done)
    {
        for (int i = 0; i != file_descriptors; ++i)
        {
            static char const test_file_fmt[] = "fd_test_file%d";
            char test_file[sizeof test_file_fmt];
            sprintf(test_file, test_file_fmt, i);
            remove(test_file);
            file_descriptor[i] = open(test_file, O_CREAT, S_IWUSR|S_IRUSR);

            fds->add_fd(file_descriptor[i]);
        }

        done->Run();
    }

    void close_files()
    {
        for (auto i = file_descriptor; i != file_descriptor+file_descriptors; ++i)
            close(*i), *i = 0;
    }
};

const int StubServer::file_descriptors;

struct ErrorServer : mir::protobuf::DisplayServer
{
    static std::string const test_exception_text;

    void create_surface(
        google::protobuf::RpcController*,
        const protobuf::SurfaceParameters*,
        protobuf::Surface*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }

    void release_surface(
        google::protobuf::RpcController*,
        const protobuf::SurfaceId*,
        protobuf::Void*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }


    void connect(
        ::google::protobuf::RpcController*,
        const ::mir::protobuf::ConnectParameters*,
        ::mir::protobuf::Connection*,
        ::google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }

    void disconnect(
        google::protobuf::RpcController*,
        const protobuf::Void*,
        protobuf::Void*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }

    void test_file_descriptors(
        google::protobuf::RpcController*,
        const protobuf::Void*,
        protobuf::Buffer*,
        google::protobuf::Closure*)
    {
        throw std::runtime_error(test_exception_text);
    }
};

std::string const ErrorServer::test_exception_text{"test exception text"};

struct NullDeleter
{
    void operator()(void* )
    {
    }
};

class MockLogger : public mir::client::Logger
{
    mir::client::ConsoleLogger real_logger;

public:
    MockLogger()
    {
        using mir::client::ConsoleLogger;

        ON_CALL(*this, error())
            .WillByDefault(::testing::Invoke(&real_logger, &ConsoleLogger::error));
        EXPECT_CALL(*this, error()).Times(0);

        ON_CALL(*this, debug())
            .WillByDefault(::testing::Invoke(&real_logger, &ConsoleLogger::debug));
        EXPECT_CALL(*this, debug()).Times(testing::AtLeast(0));
    }

    MOCK_METHOD0(error,std::ostream& ());
    MOCK_METHOD0(debug,std::ostream& ());
};

class MockIpcFactory : public mf::ProtobufIpcFactory
{
public:
    MockIpcFactory(mir::protobuf::DisplayServer& server) :
        server(&server, NullDeleter()),
        cache(std::make_shared<mf::ResourceCache>())
    {
        using namespace testing;

        ON_CALL(*this, make_ipc_server()).WillByDefault(Return(this->server));

        // called during initialisation:
        // there's always a server awaiting the next connection
        EXPECT_CALL(*this, make_ipc_server()).Times(1);
    }

    MOCK_METHOD0(make_ipc_server, std::shared_ptr<mir::protobuf::DisplayServer>());

private:
    virtual std::shared_ptr<mf::ResourceCache> resource_cache()
    {
        return cache;
    }

    std::shared_ptr<mir::protobuf::DisplayServer> server;
    std::shared_ptr<mf::ResourceCache> const cache;
};

struct TestServer
{
    static std::string const & socket_name()
    {
        static std::string socket_name("./mir_test_pb_asio_socket");
        return socket_name;
    }

    TestServer() :
        factory(std::make_shared<MockIpcFactory>(stub_services)),
        comm(socket_name(), factory)
    {
    }

    void expect_surface_count(int expected_count)
    {
        std::unique_lock<std::mutex> ul(stub_services.guard);
        while (stub_services.surface_count != expected_count)
            stub_services.wait_condition.wait(ul);

        EXPECT_EQ(expected_count, stub_services.surface_count);
    }

    // "Server" side
    StubServer stub_services;
    std::shared_ptr<MockIpcFactory> factory;
    mf::ProtobufAsioCommunicator comm;
};

struct TestClient
{
    TestClient() :
        logger(std::make_shared<MockLogger>()),
        channel(TestServer::socket_name(), logger),
        display_server(&channel),
        connect_done_called(false),
        create_surface_called(false),
        next_buffer_called(false),
        disconnect_done_called(false),
        tfd_done_called(false),
        connect_done_count(0),
        create_surface_done_count(0),
        disconnect_done_count(0)
    {
        surface_parameters.set_width(640);
        surface_parameters.set_height(480);
        surface_parameters.set_pixel_format(0);

        ON_CALL(*this, connect_done()).WillByDefault(testing::Invoke(this, &TestClient::on_connect_done));
        ON_CALL(*this, create_surface_done()).WillByDefault(testing::Invoke(this, &TestClient::on_create_surface_done));
        ON_CALL(*this, next_buffer_done()).WillByDefault(testing::Invoke(this, &TestClient::on_next_buffer_done));
        ON_CALL(*this, disconnect_done()).WillByDefault(testing::Invoke(this, &TestClient::on_disconnect_done));
    }

    std::shared_ptr<MockLogger> logger;
    mir::client::MirRpcChannel channel;
    mir::protobuf::DisplayServer::Stub display_server;
    mir::protobuf::ConnectParameters connect_parameters;
    mir::protobuf::SurfaceParameters surface_parameters;
    mir::protobuf::Surface surface;
    mir::protobuf::Void ignored;
    mir::protobuf::Connection connection;

    MOCK_METHOD0(connect_done, void ());
    MOCK_METHOD0(create_surface_done, void ());
    MOCK_METHOD0(next_buffer_done, void ());
    MOCK_METHOD0(disconnect_done, void ());

    void on_connect_done()
    {
        connect_done_called.store(true);

        auto old = connect_done_count.load();

        while (!connect_done_count.compare_exchange_weak(old, old+1));
    }

    void on_create_surface_done()
    {
        create_surface_called.store(true);

        auto old = create_surface_done_count.load();

        while (!create_surface_done_count.compare_exchange_weak(old, old+1));
    }

    void on_next_buffer_done()
    {
        next_buffer_called.store(true);
    }

    void on_disconnect_done()
    {
        disconnect_done_called.store(true);

        auto old = disconnect_done_count.load();

        while (!disconnect_done_count.compare_exchange_weak(old, old+1));
    }

    void wait_for_connect_done()
    {
        for (int i = 0; !connect_done_called.load() && i < 100; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::this_thread::yield();
        }
        connect_done_called.store(false);
    }

    void wait_for_create_surface()
    {
        for (int i = 0; !create_surface_called.load() && i < 100; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::this_thread::yield();
        }
        create_surface_called.store(false);
    }

    void wait_for_next_buffer()
    {
        for (int i = 0; !next_buffer_called.load() && i < 100; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::this_thread::yield();
        }
        next_buffer_called.store(false);
    }

    void wait_for_disconnect_done()
    {
        for (int i = 0; !disconnect_done_called.load() && i < 100; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::this_thread::yield();
        }
        disconnect_done_called.store(false);
    }

    void wait_for_surface_count(int count)
    {
        for (int i = 0; count != create_surface_done_count.load() && i < 10000; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::this_thread::yield();
        }
    }
    void wait_for_disconnect_count(int count)
    {
        for (int i = 0; count != disconnect_done_count.load() && i < 10000; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            std::this_thread::yield();
        }
    }

    void tfd_done()
    {
        tfd_done_called.store(true);
    }

    void wait_for_tfd_done()
    {
        for (int i = 0; !tfd_done_called.load() && i < 100; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        tfd_done_called.store(false);
    }

    std::atomic<bool> connect_done_called;
    std::atomic<bool> create_surface_called;
    std::atomic<bool> next_buffer_called;
    std::atomic<bool> disconnect_done_called;
    std::atomic<bool> tfd_done_called;

    std::atomic<int> connect_done_count;
    std::atomic<int> create_surface_done_count;
    std::atomic<int> disconnect_done_count;
};

struct ProtobufAsioCommunicatorTestFixture
{
    void SetUp()
    {
        ::testing::Mock::VerifyAndClearExpectations(server.factory.get());
        EXPECT_CALL(*server.factory, make_ipc_server()).Times(1);
        server.comm.start();
    }

    void TearDown()
    {
        server.comm.stop();
    }

    TestClient client;
    TestServer server;
};

TEST_F(SendBuffer, buffer_connect)
{
    client.display_server.create_surface(
        0,
        &client.surface_parameters,
        &client.surface,
        google::protobuf::NewCallback(&client, &TestClient::create_surface_done));

    client.wait_for_create_surface();

    server.expect_surface_count(1);
}
}
