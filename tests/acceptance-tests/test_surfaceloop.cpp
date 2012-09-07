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

#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/compositor/buffer_swapper.h"

#include "mir_client/mir_client_library.h"
#include "mir_client/mir_logger.h"

#include "display_server_test_fixture.h"



#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"
#include "mir/thread/all.h"

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
struct MockBufferAllocationStrategy : public mc::BufferAllocationStrategy
{
    MOCK_METHOD3(
        create_swapper,
        std::unique_ptr<mc::BufferSwapper>(geom::Width, geom::Height, mc::PixelFormat));
};

class MockGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
 public:
    MOCK_METHOD3(
        alloc_buffer,
        std::unique_ptr<mc::Buffer> (geom::Width, geom::Height, mc::PixelFormat));
};


geom::Width const width{640};
geom::Height const height{480};
mc::PixelFormat const format{mc::PixelFormat::rgba_8888};

struct ClientConfigCommon : TestingClientConfiguration
{
    ClientConfigCommon()
        : connection(NULL)
        , surface(NULL)
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

    static void release_surface_callback(MirSurface * surface, void * context)
    {
        ClientConfigCommon * config = reinterpret_cast<ClientConfigCommon *>(context);
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


    std::mutex guard;
    std::condition_variable wait_condition;
    MirConnection * connection;
    MirSurface * surface;
};

}

TEST_F(BespokeDisplayServerTestFixture,
       creating_a_client_surface_allocates_buffer_swapper_on_server)
{

    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mc::BufferAllocationStrategy> make_buffer_allocation_strategy()
        {
            if (!buffer_allocation_strategy)
                buffer_allocation_strategy = std::make_shared<MockBufferAllocationStrategy>();

            EXPECT_CALL(*buffer_allocation_strategy, create_swapper(width, height, format)).Times(1);

            return buffer_allocation_strategy;
        }

        std::shared_ptr<MockBufferAllocationStrategy> buffer_allocation_strategy;
    } server_config;

    launch_server_process(server_config);

    struct ClientConfig : ClientConfigCommon
    {
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
    } client_config;

    launch_client_process(client_config);
}

TEST_F(BespokeDisplayServerTestFixture,
       creating_a_client_surface_allocates_buffers_on_server)
{

    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mc::GraphicBufferAllocator> make_graphic_buffer_allocator()
        {
            using testing::AtLeast;

            if (!buffer_allocator)
                buffer_allocator = std::make_shared<MockGraphicBufferAllocator>();

            EXPECT_CALL(*buffer_allocator, alloc_buffer(width, height, format)).Times(AtLeast(2));

            return buffer_allocator;
        }

        std::shared_ptr<MockGraphicBufferAllocator> buffer_allocator;
    } server_config;

    launch_server_process(server_config);

    struct ClientConfig : ClientConfigCommon
    {
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
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, creates_surface_of_correct_size)
{
    struct Client : ClientConfigCommon
    {
        void exec()
        {
            mir_connect(connection_callback, this);

            wait_for_connect();

            MirSurfaceParameters request_params = {640, 480, mir_pixel_format_rgba_8888};

            mir_surface_create(connection, &request_params, create_surface_callback, this);
            wait_for_surface_create();

            // A bit inelegant as ClientConfigCommon only "knows" one surface variable.
            MirSurface* surface1 = surface;
            surface = 0;

            request_params.width = 1600;
            request_params.height = 1200;

            mir_surface_create(connection, &request_params, create_surface_callback, this);
            wait_for_surface_create();

            MirSurfaceParameters response_params = mir_surface_get_parameters(surface1);
            EXPECT_EQ(640, response_params.width);
            EXPECT_EQ(480, response_params.height);
            EXPECT_EQ(mir_pixel_format_rgba_8888, response_params.pixel_format);

            response_params = mir_surface_get_parameters(surface);
            EXPECT_EQ(1600, response_params.width);
            EXPECT_EQ(1200, response_params.height);
            EXPECT_EQ(mir_pixel_format_rgba_8888, response_params.pixel_format);

            mir_surface_release(surface, release_surface_callback, this);
            wait_for_surface_release();

            // A bit inelegant as ClientConfigCommon only "knows" one surface variable.
            surface = surface1;

            mir_surface_release(surface1, release_surface_callback, this);
            wait_for_surface_release();

            mir_connection_release(connection);
        }
    } client_creates_surfaces;

    launch_client_process(client_creates_surfaces);
}
