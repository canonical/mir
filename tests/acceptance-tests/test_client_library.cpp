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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "display_server_test_fixture.h"

#include "mir_client/mir_client_library.h"

#include "mir/chrono/chrono.h"
#include "mir/frontend/protobuf_asio_communicator.h"
#include "mir/thread/all.h"

#include "mir_client/mir_surface.h"
#include "mir_client/mir_logger.h"

#include "mir_protobuf.pb.h"

#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mc = mir::compositor;

namespace mir
{
namespace
{

struct ActivityCounter : mir::protobuf::DisplayServer
{
    int session_count;
    int surface_count;
    std::mutex guard;
    std::condition_variable wait_condition;

    ActivityCounter() : session_count(0), surface_count(0)
    {
    }

    ActivityCounter(ActivityCounter const &) = delete;

    void connect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::ConnectMessage* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        response->set_width(request->width());
        response->set_height(request->height());
        response->set_pixel_format(request->pixel_format());

        std::unique_lock<std::mutex> lock(guard);
        ++session_count;
        wait_condition.notify_one();

        done->Run();
    }

    void create_surface(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::Surface* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        response->set_width(request->width());
        response->set_height(request->height());
        response->set_pixel_format(request->pixel_format());

        std::unique_lock<std::mutex> lock(guard);
        ++surface_count;
        wait_condition.notify_one();

        done->Run();
    }

    void disconnect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::Void* /*request*/,
                 mir::protobuf::Void* /*response*/,
                 google::protobuf::Closure* done)
    {
        std::unique_lock<std::mutex> lock(guard);
        --session_count;
        wait_condition.notify_one();
        done->Run();
    }
};

struct NullDeleter
{
    void operator()(void* )
    {
    }
};

class StubIpcFactory : public mf::ProtobufIpcFactory
{
public:
    StubIpcFactory(mir::protobuf::DisplayServer& server) :
        server(server) {}
private:
    virtual std::shared_ptr<mir::protobuf::DisplayServer> make_ipc_server()
    {
        return std::shared_ptr<mir::protobuf::DisplayServer>(&server, NullDeleter());
    }
    mir::protobuf::DisplayServer& server;
};

struct ServerConfig : TestingServerConfiguration
{
    ServerConfig(int session_count, int surface_count)
        : session_count(session_count), surface_count(surface_count)
    {
    }
    std::shared_ptr<mf::ProtobufIpcFactory> make_ipc_factory(
        std::shared_ptr<mc::BufferAllocationStrategy> const& )
    {
        return std::make_shared<StubIpcFactory>(counter);
    }

    void on_exit(mir::DisplayServer* )
    {
        check_sessions();
        check_surfaces();
    }

    void check_sessions()
    {
        std::unique_lock<std::mutex> lock(counter.guard);
        if (counter.session_count != session_count)
            counter.wait_condition.wait_for(lock, std::chrono::milliseconds(100));
        EXPECT_EQ(session_count, counter.session_count);
    }

    void check_surfaces()
    {
        std::unique_lock<std::mutex> lock(counter.guard);
        if (counter.surface_count != surface_count)
            counter.wait_condition.wait_for(lock, std::chrono::milliseconds(100));
        EXPECT_EQ(surface_count, counter.surface_count);
    }

    ActivityCounter counter;
    int session_count, surface_count;
};

}

TEST_F(BespokeDisplayServerTestFixture, client_library_connects)
{
    ServerConfig server_config(1, 0);

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        ClientConfig()
            : connection(NULL)
        {
        }

        static void connection_callback(MirConnection * connection, void * context)
        {
            ClientConfig * config = reinterpret_cast<ClientConfig *>(context);
            config->connected(connection);
        }

        void connected(MirConnection * new_connection)
        {
            std::unique_lock<std::mutex> lock(guard);
            connection = new_connection;
            wait_condition.notify_one();
        }

        void exec()
        {
            mir_connect(connection_callback, this);
            std::unique_lock<std::mutex> lock(guard);
            if (!connection)
                wait_condition.wait_for(lock, std::chrono::milliseconds(100));

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");
        }

        std::mutex guard;
        std::condition_variable wait_condition;
        MirConnection * connection;
    } client_config;

    launch_client_process(client_config);
}

TEST_F(BespokeDisplayServerTestFixture, client_library_creates_surface)
{
    ServerConfig server_config(1, 1);
    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        ClientConfig()
            : connection(NULL)
            , surface(NULL)
        {
        }

        static void connection_callback(MirConnection * connection, void * context)
        {
            ClientConfig * config = reinterpret_cast<ClientConfig *>(context);
            config->connected(connection);
        }

        static void create_surface_callback(MirSurface * surface, void * context)
        {
            ClientConfig * config = reinterpret_cast<ClientConfig *>(context);
            config->surface_created(surface);
        }

        void connected(MirConnection * new_connection)
        {
            std::unique_lock<std::mutex> lock(guard);
            connection = new_connection;
            wait_condition.notify_one();
        }

        void surface_created(MirSurface * new_surface)
        {
            std::unique_lock<std::mutex> lock(guard);
            surface = new_surface;
            wait_condition.notify_one();
        }

        void connect()
        {
            mir_connect(connection_callback, this);

            std::unique_lock<std::mutex> lock(guard);
            if (!connection)
                wait_condition.wait_for(lock, std::chrono::milliseconds(100));

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");
        }

        void create_surface()
        {
            MirSurfaceParameters const request_params{640, 480, mir_pixel_format_rgba_8888};
            mir_create_surface(connection, &request_params, create_surface_callback, this);

            std::unique_lock<std::mutex> lock(guard);
            if (!surface)
                wait_condition.wait_for(lock, std::chrono::milliseconds(1000));

            ASSERT_TRUE(surface != NULL);
            EXPECT_TRUE(mir_surface_is_valid(surface));
            EXPECT_STREQ(mir_surface_get_error_message(surface), "");

            MirSurfaceParameters const response_params = mir_surface_get_parameters(surface);
            EXPECT_EQ(request_params.width, response_params.width);
            EXPECT_EQ(request_params.height, response_params.height);
            EXPECT_EQ(request_params.pixel_format, response_params.pixel_format);
        }

        void exec()
        {
            connect();
            create_surface();
        }

        std::mutex guard;
        std::condition_variable wait_condition;
        MirConnection * connection;
        MirSurface * surface;
    } client_config;

    launch_client_process(client_config);
}

}
