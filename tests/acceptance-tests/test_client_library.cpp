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
#include "client_buffer.h"

#include "mir/chrono/chrono.h"
#include "mir/frontend/protobuf_asio_communicator.h"
#include "mir/thread/all.h"

#include "mir_client/mir_logger.h"

#include "mir_protobuf.pb.h"

#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mc = mir::compositor;
namespace mcl = mir::client;

namespace
{
    char const* const mir_test_socket = mir::test_socket_file().c_str();
}

namespace mir
{
namespace
{
struct ClientConfigCommon : TestingClientConfiguration
{
    ClientConfigCommon()
        : connection(0)
        , surface(0),
        buffers(0)
    {
    }

    static void connection_callback(MirConnection * connection, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->connected(connection);
    }

    static void create_surface_callback(MirSurface * surface, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_created(surface);
    }

    static void next_buffer_callback(MirSurface * surface, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->next_buffer(surface);
    }

    static void release_surface_callback(MirSurface * surface, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_released(surface);
    }

    virtual void connected(MirConnection * new_connection)
    {
        std::unique_lock<std::mutex> lock(guard);
        connection = new_connection;
        wait_condition.notify_all();
    }

    virtual void surface_created(MirSurface * new_surface)
    {
        std::unique_lock<std::mutex> lock(guard);
        surface = new_surface;
        wait_condition.notify_all();
    }

    virtual void next_buffer(MirSurface*)
    {
        std::unique_lock<std::mutex> lock(guard);
        ++buffers;
        wait_condition.notify_all();
    }

    virtual void surface_released(MirSurface * /*released_surface*/)
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

    void wait_for_next_buffer()
    {
        std::unique_lock<std::mutex> lock(guard);
        while (!buffers)
            wait_condition.wait(lock);
    }

    void wait_for_surface_release()
    {
        std::unique_lock<std::mutex> lock(guard);
        while (surface)
            wait_condition.wait(lock);
    }

    std::mutex guard;
    std::condition_variable wait_condition;
    MirConnection* connection;
    MirSurface* surface;
    int buffers;
};
}

TEST_F(DefaultDisplayServerTestFixture, client_library_connects_and_disconnects)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this);

            wait_for_connect();

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, client_library_creates_surface)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {

            mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this);

            wait_for_connect();

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");

            MirSurfaceParameters const request_params =
                { __PRETTY_FUNCTION__, 640, 480, mir_pixel_format_rgba_8888};

            mir_surface_create(connection, &request_params, create_surface_callback, this);

            wait_for_surface_create();

            ASSERT_TRUE(surface != NULL);
            EXPECT_TRUE(mir_surface_is_valid(surface));
            EXPECT_STREQ(mir_surface_get_error_message(surface), "");

            MirSurfaceParameters response_params;
            mir_surface_get_parameters(surface, &response_params);
            EXPECT_EQ(request_params.width, response_params.width);
            EXPECT_EQ(request_params.height, response_params.height);
            EXPECT_EQ(request_params.pixel_format, response_params.pixel_format);


            mir_surface_release(connection, surface, release_surface_callback, this);

            wait_for_surface_release();

            ASSERT_TRUE(surface == NULL);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, client_library_creates_multiple_surfaces)
{
    int const n_surfaces = 13;

    struct ClientConfig : ClientConfigCommon
    {
        ClientConfig(int n_surfaces) : n_surfaces(n_surfaces)
        {
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
            mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this);

            wait_for_connect();

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");

            for (int i = 0; i != n_surfaces; ++i)
            {
                old_surface_count = current_surface_count();

                MirSurfaceParameters const request_params =
                    {__PRETTY_FUNCTION__, 640, 480, mir_pixel_format_rgba_8888};
                mir_surface_create(connection, &request_params, create_surface_callback, this);

                wait_for_surface_create();

                ASSERT_EQ(old_surface_count + 1, current_surface_count());
            }

            for (int i = 0; i != n_surfaces; ++i)
            {
                old_surface_count = current_surface_count();

                ASSERT_NE(old_surface_count, 0u);

                MirSurface * surface = any_surface();
                mir_surface_release(connection, surface, release_surface_callback, this);

                wait_for_surface_release();

                ASSERT_EQ(old_surface_count - 1, current_surface_count());
            }

            mir_connection_release(connection);
        }

        int n_surfaces;
        std::set<MirSurface *> surfaces;
        size_t old_surface_count;
    } client_config(n_surfaces);

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, client_library_accesses_and_advances_buffers)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {

            mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this);

            wait_for_connect();

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");

            MirSurfaceParameters const request_params =
                { __PRETTY_FUNCTION__, 640, 480, mir_pixel_format_rgba_8888};

            mir_surface_create(connection, &request_params, create_surface_callback, this);

            wait_for_surface_create();

            ASSERT_TRUE(surface != NULL);

            MirBufferPackage buffer_package;
            mir_surface_get_current_buffer(surface, &buffer_package);

            mir_surface_next_buffer(surface, next_buffer_callback, this);
            wait_for_next_buffer();

            mir_surface_release(connection, surface, release_surface_callback, this);

            wait_for_surface_release();

            ASSERT_TRUE(surface == NULL);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, client_library_accesses_platform_package)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {
            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this));
            ASSERT_TRUE(connection != NULL);

            MirPlatformPackage platform_package;
            platform_package.data_items = -1;
            platform_package.fd_items = -1;

            mir_connection_get_platform(connection, &platform_package);
            EXPECT_GE(0, platform_package.data_items);
            EXPECT_GE(0, platform_package.fd_items);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, client_library_using_mir_wait_for)
{
    struct ClientConfig : ClientConfigCommon
    {
        void exec()
        {

            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this));

            ASSERT_TRUE(connection != NULL);
            EXPECT_TRUE(mir_connection_is_valid(connection));
            EXPECT_STREQ(mir_connection_get_error_message(connection), "");

            MirSurfaceParameters const request_params =
                { __PRETTY_FUNCTION__, 640, 480, mir_pixel_format_rgba_8888};

            mir_wait_for(mir_surface_create(connection, &request_params, create_surface_callback, this));

            ASSERT_TRUE(surface != NULL);

            MirGraphicsRegion buffer_package;
            mir_surface_get_graphics_region(surface, &buffer_package);

            mir_wait_for(mir_surface_next_buffer(surface, next_buffer_callback, this));

            mir_wait_for(mir_surface_release(connection, surface, release_surface_callback, this));

            ASSERT_TRUE(surface == NULL);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}
}
