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
    bool disconnected;
    google::protobuf::int32 next_surface_id;
    int surfaces_created, surfaces_released;
    std::mutex guard;
    std::condition_variable wait_condition;

    ActivityCounter() : disconnected(0), next_surface_id(0), surfaces_created(0), surfaces_released(0)
    {
    }

    ActivityCounter(ActivityCounter const &) = delete;

    void create_surface(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::SurfaceParameters* request,
                 mir::protobuf::Surface* response,
                 google::protobuf::Closure* done)
    {
        response->set_width(request->width());
        response->set_height(request->height());
        response->set_pixel_format(request->pixel_format());

        std::unique_lock<std::mutex> lock(guard);
        response->mutable_id()->set_value(next_surface_id++);
        ++surfaces_created;
        wait_condition.notify_one();

        done->Run();
    }

    void release_surface(google::protobuf::RpcController* /*controller*/,
                         const mir::protobuf::SurfaceId*,
                         mir::protobuf::Void*,
                         google::protobuf::Closure* done)
    {
        // TODO track and check surface ids
        std::unique_lock<std::mutex> lock(guard);
        ++surfaces_released;
        wait_condition.notify_one();
        done->Run();
    }

    void disconnect(google::protobuf::RpcController* /*controller*/,
                 const mir::protobuf::Void* /*request*/,
                 mir::protobuf::Void* /*response*/,
                 google::protobuf::Closure* done)
    {
        std::unique_lock<std::mutex> lock(guard);
        disconnected = true;
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

// Tests using this configuration must disconnect from the MIR server.
// This is to allow server side activity checking on exit.
struct ServerConfig : TestingServerConfiguration
{
    std::shared_ptr<mf::ProtobufIpcFactory> make_ipc_factory(
        std::shared_ptr<mc::BufferAllocationStrategy> const& )
    {
        return std::make_shared<StubIpcFactory>(counter);
    }

    ActivityCounter counter;
};

}

TEST_F(BespokeDisplayServerTestFixture, client_library_connects_and_disconnects)
{
    ServerConfig server_config;

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        ClientConfig() : connection(NULL)
        {
        }

        static void connection_callback(MirConnection * connection, void * context)
        {
            ClientConfig * config = reinterpret_cast<ClientConfig *>(context);
            config->connected(connection);
        }
        static void connection_release_callback(void * context)
        {
            ClientConfig * config = reinterpret_cast<ClientConfig *>(context);
            config->disconnected();
        }

        void connected(MirConnection * new_connection)
        {
            std::unique_lock<std::mutex> lock(guard);
            connection = new_connection;
            wait_condition.notify_one();
        }

        void disconnected()
        {
            std::unique_lock<std::mutex> lock(guard);
            connection = NULL;
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

        void disconnect()
        {
            mir_connection_release(connection);
        }

        void exec()
        {
            connect();
            disconnect();
        }

        std::mutex guard;
        std::condition_variable wait_condition;
        MirConnection * connection;
    } client_config;

    launch_client_process(client_config);
}

TEST_F(BespokeDisplayServerTestFixture, client_library_creates_surface)
{
    ServerConfig server_config;
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

        static void release_surface_callback(MirSurface * surface, void * context)
        {
            ClientConfig * config = reinterpret_cast<ClientConfig *>(context);
            config->surface_released(surface);
        }

        static void connection_release_callback(void * context)
        {
            ClientConfig * config = reinterpret_cast<ClientConfig *>(context);
            config->disconnected();
        }

        void connected(MirConnection * new_connection)
        {
            std::unique_lock<std::mutex> lock(guard);
            connection = new_connection;
            wait_condition.notify_all();
        }

        void disconnected()
        {
            std::unique_lock<std::mutex> lock(guard);
            connection = NULL;
            wait_condition.notify_one();
        }

        void surface_created(MirSurface * new_surface)
        {
            std::unique_lock<std::mutex> lock(guard);
            surface = new_surface;
            wait_condition.notify_all();
        }

        void surface_released(MirSurface * /*released_surface*/)
        {
            std::unique_lock<std::mutex> lock(guard);
            surface = NULL;
            wait_condition.notify_all();
        }

        void wait_for_connect()
        {
            std::unique_lock<std::mutex> lock(guard);
            while (!connection)
                wait_condition.wait(lock);
        }

        void wait_for_surface_create()
        {
            std::unique_lock<std::mutex> lock(guard);
            while (!surface)
                wait_condition.wait(lock);
        }

        void wait_for_surface_release()
        {
            std::unique_lock<std::mutex> lock(guard);
            while (surface)
                wait_condition.wait(lock);
        }

        void exec()
        {

            mir_connect(connection_callback, this);

            wait_for_connect();

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");

            MirSurfaceParameters const request_params{640, 480, mir_pixel_format_rgba_8888};
            mir_surface_create(connection, &request_params, create_surface_callback, this);

            wait_for_surface_create();

            ASSERT_TRUE(surface != NULL);
            EXPECT_TRUE(mir_surface_is_valid(surface));
            EXPECT_STREQ(mir_surface_get_error_message(surface), "");

            MirSurfaceParameters const response_params = mir_surface_get_parameters(surface);
            EXPECT_EQ(request_params.width, response_params.width);
            EXPECT_EQ(request_params.height, response_params.height);
            EXPECT_EQ(request_params.pixel_format, response_params.pixel_format);


            mir_surface_release(surface, release_surface_callback, this);

            wait_for_surface_release();

            ASSERT_TRUE(surface == NULL);

            mir_connection_release(connection);
        }

        std::mutex guard;
        std::condition_variable wait_condition;
        MirConnection * connection;
        MirSurface * surface;
    } client_config;

    launch_client_process(client_config);
}

TEST_F(BespokeDisplayServerTestFixture, client_library_creates_multiple_surfaces)
{
    int const n_surfaces = 13;
    ServerConfig server_config;

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        ClientConfig(int n_surfaces) : n_surfaces(n_surfaces), connection(NULL)
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

        static void release_surface_callback(MirSurface * surface, void * context)
        {
            ClientConfig * config = reinterpret_cast<ClientConfig *>(context);
            config->surface_released(surface);
        }

        void connected(MirConnection * new_connection)
        {
            std::unique_lock<std::mutex> lock(guard);
            connection = new_connection;
            wait_condition.notify_all();
        }

        void surface_created(MirSurface * new_surface)
        {
            std::unique_lock<std::mutex> lock(guard);
            surfaces.insert(new_surface);
            wait_condition.notify_all();
        }

        void surface_released(MirSurface * surface)
        {
            std::unique_lock<std::mutex> lock(guard);
            surfaces.erase(surface);
            wait_condition.notify_all();
        }

        MirSurface * any_surface()
        {
            std::unique_lock<std::mutex> lock(guard);
            return *surfaces.begin();
        }

        void wait_for_connect()
        {
            std::unique_lock<std::mutex> lock(guard);
            while (!connection)
                wait_condition.wait(lock);
        }

        size_t current_surface_count()
        {
            std::unique_lock<std::mutex> lock(guard);
            return surfaces.size();
        }

        void wait_for_surface_create()
        {
            std::unique_lock<std::mutex> lock(guard);
            while (surfaces.size() == old_surface_count)
                wait_condition.wait(lock);
        }

        void wait_for_surface_release()
        {
            std::unique_lock<std::mutex> lock(guard);
            if (surfaces.size() == old_surface_count)
                wait_condition.wait(lock);
        }

        void exec()
        {
            mir_connect(connection_callback, this);

            wait_for_connect();

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");

            for (int i = 0; i != n_surfaces; ++i)
            {
                old_surface_count = current_surface_count();

                MirSurfaceParameters const request_params{640, 480, mir_pixel_format_rgba_8888};
                mir_surface_create(connection, &request_params, create_surface_callback, this);

                wait_for_surface_create();

                ASSERT_EQ(old_surface_count + 1, current_surface_count());
            }

            for (int i = 0; i != n_surfaces; ++i)
            {
                old_surface_count = current_surface_count();

                ASSERT_NE(old_surface_count, 0u);

                MirSurface * surface = any_surface();
                mir_surface_release(surface, release_surface_callback, this);

                wait_for_surface_release();

                ASSERT_EQ(old_surface_count - 1, current_surface_count());
            }

            mir_connection_release(connection);
        }

        std::mutex guard;
        std::condition_variable wait_condition;
        int n_surfaces;
        MirConnection * connection;
        std::set<MirSurface *> surfaces;
        size_t old_surface_count;
    } client_config(n_surfaces);

    launch_client_process(client_config);
}
}
