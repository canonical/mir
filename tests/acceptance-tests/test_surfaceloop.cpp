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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/geometry/pixel_format.h"
#include "mir/geometry/size.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_debug.h"
#include "src/client/mir_connection.h"

#include "mir_protobuf.pb.h"

#include "mir_test_framework/display_server_test_fixture.h"

#include <mutex>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"

namespace geom = mir::geometry;
namespace mcl = mir::client;
namespace mtf = mir_test_framework;

namespace
{
char const* const mir_test_socket = mtf::test_socket_file().c_str();

geom::Size const size{640, 480};
}

namespace mir
{
namespace
{
struct SurfaceSync
{
    SurfaceSync() :
        surface(0)
    {
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


    std::mutex guard;
    std::condition_variable wait_condition;
    MirSurface * surface;
};

struct ClientConfigCommon : TestingClientConfiguration
{
    ClientConfigCommon()
        : connection(NULL)
    {
    }

    static void connection_callback(MirConnection * connection, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
        config->connected(connection);
    }

    void connected(MirConnection * new_connection)
    {
        std::unique_lock<std::mutex> lock(guard);
        connection = new_connection;
        wait_condition.notify_all();
    }

    void wait_for_connect()
    {
        std::unique_lock<std::mutex> lock(guard);
        while (!connection)
            wait_condition.wait(lock);
    }

    std::mutex guard;
    std::condition_variable wait_condition;
    MirConnection * connection;
    static const int max_surface_count = 5;
    SurfaceSync ssync[max_surface_count];
};
const int ClientConfigCommon::max_surface_count;
}
}

using mir::SurfaceSync;
using mir::ClientConfigCommon;

namespace
{
void create_surface_callback(MirSurface* surface, void * context)
{
    SurfaceSync* config = reinterpret_cast<SurfaceSync*>(context);
    config->surface_created(surface);
}

void release_surface_callback(MirSurface* surface, void * context)
{
    SurfaceSync* config = reinterpret_cast<SurfaceSync*>(context);
    config->surface_released(surface);
}

void wait_for_surface_create(SurfaceSync* context)
{
    context->wait_for_surface_create();
}

void wait_for_surface_release(SurfaceSync* context)
{
    context->wait_for_surface_release();
}
}

TEST_F(DefaultDisplayServerTestFixture, creates_surface_of_correct_size)
{
    struct Client : ClientConfigCommon
    {
        void exec()
        {
            mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this);

            wait_for_connect();

            MirSurfaceParameters request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware,
                mir_display_output_id_invalid
            };

            mir_connection_create_surface(connection, &request_params, create_surface_callback, ssync);
            wait_for_surface_create(ssync);

            request_params.width = 1600;
            request_params.height = 1200;

            mir_connection_create_surface(connection, &request_params, create_surface_callback, ssync+1);
            wait_for_surface_create(ssync+1);

            MirSurfaceParameters response_params;
            mir_surface_get_parameters(ssync->surface, &response_params);
            EXPECT_EQ(640, response_params.width);
            EXPECT_EQ(480, response_params.height);
            EXPECT_EQ(mir_pixel_format_abgr_8888, response_params.pixel_format);
            EXPECT_EQ(mir_buffer_usage_hardware, response_params.buffer_usage);

            mir_surface_get_parameters(ssync[1].surface, &response_params);
            EXPECT_EQ(1600, response_params.width);
            EXPECT_EQ(1200, response_params.height);
            EXPECT_EQ(mir_pixel_format_abgr_8888, response_params.pixel_format);
            EXPECT_EQ(mir_buffer_usage_hardware, response_params.buffer_usage);

            mir_surface_release(ssync[1].surface, release_surface_callback, ssync+1);
            wait_for_surface_release(ssync+1);

            mir_surface_release(ssync->surface, release_surface_callback, ssync);
            wait_for_surface_release(ssync);

            mir_connection_release(connection);
        }
    } client_creates_surfaces;

    launch_client_process(client_creates_surfaces);
}

TEST_F(DefaultDisplayServerTestFixture, surfaces_have_distinct_ids)
{
    struct Client : ClientConfigCommon
    {
        void exec()
        {
            mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this);

            wait_for_connect();

            MirSurfaceParameters request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware,
                mir_display_output_id_invalid
            };

            mir_connection_create_surface(connection, &request_params, create_surface_callback, ssync);
            wait_for_surface_create(ssync);

            request_params.width = 1600;
            request_params.height = 1200;

            mir_connection_create_surface(connection, &request_params, create_surface_callback, ssync+1);
            wait_for_surface_create(ssync+1);

            EXPECT_NE(
                mir_debug_surface_id(ssync[0].surface),
                mir_debug_surface_id(ssync[1].surface));

            mir_surface_release(ssync[1].surface, release_surface_callback, ssync+1);
            wait_for_surface_release(ssync+1);

            mir_surface_release(ssync[0].surface, release_surface_callback, ssync);
            wait_for_surface_release(ssync);

            mir_connection_release(connection);
        }
    } client_creates_surfaces;

    launch_client_process(client_creates_surfaces);
}

TEST_F(DefaultDisplayServerTestFixture, creates_multiple_surfaces_async)
{
    struct Client : ClientConfigCommon
    {
        void exec()
        {
            mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this);

            wait_for_connect();

            MirSurfaceParameters request_params =
            {
                __PRETTY_FUNCTION__,
                640, 480,
                mir_pixel_format_abgr_8888,
                mir_buffer_usage_hardware,
                mir_display_output_id_invalid
            };

            for (int i = 0; i != max_surface_count; ++i)
                mir_connection_create_surface(connection, &request_params, create_surface_callback, ssync+i);

            for (int i = 0; i != max_surface_count; ++i)
                wait_for_surface_create(ssync+i);

            for (int i = 0; i != max_surface_count; ++i)
            {
                for (int j = 0; j != max_surface_count; ++j)
                {
                    if (i == j)
                        EXPECT_EQ(
                            mir_debug_surface_id(ssync[i].surface),
                            mir_debug_surface_id(ssync[j].surface));
                    else
                        EXPECT_NE(
                            mir_debug_surface_id(ssync[i].surface),
                            mir_debug_surface_id(ssync[j].surface));
                }
            }

            for (int i = 0; i != max_surface_count; ++i)
                mir_surface_release(ssync[i].surface, release_surface_callback, ssync+i);

            for (int i = 0; i != max_surface_count; ++i)
                wait_for_surface_release(ssync+i);

            mir_connection_release(connection);
        }
    } client_creates_surfaces;

    launch_client_process(client_creates_surfaces);
}

