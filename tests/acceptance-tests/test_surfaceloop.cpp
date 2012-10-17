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
#include "mir/compositor/buffer_swapper_double.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"

#include "mir_client/mir_client_library.h"
#include "mir_client/mir_logger.h"
#include "mir/thread/all.h"

#include "display_server_test_fixture.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
char const* const mir_test_socket = mir::test_socket_file().c_str();

class StubBuffer : public mc::Buffer
{
    geom::Size size() const { return geom::Size(); }
    geom::Stride stride() const { return geom::Stride(); }
    geom::PixelFormat pixel_format() const { return geom::PixelFormat(); }
    std::shared_ptr<mc::BufferIPCPackage> get_ipc_package() const { return std::make_shared<mc::BufferIPCPackage>(); }
    void bind_to_texture() {}
};

class StubDisplay : public mg::Display
{
 public:
    geom::Rectangle view_area() const { return geom::Rectangle(); }
    void clear() {}
    bool post_update() { return true; }
};

struct MockBufferAllocationStrategy : public mc::BufferAllocationStrategy
{
    MockBufferAllocationStrategy()
    {
        using testing::_;
        ON_CALL(*this, create_swapper(_,_))
            .WillByDefault(testing::Invoke(this, &MockBufferAllocationStrategy::on_create_swapper));
    }

    MOCK_METHOD2(
        create_swapper,
        std::unique_ptr<mc::BufferSwapper>(geom::Size, geom::PixelFormat));

    std::unique_ptr<mc::BufferSwapper> on_create_swapper(geom::Size, geom::PixelFormat)
    {
        return std::unique_ptr<mc::BufferSwapper>(
            new mc::BufferSwapperDouble(
                std::unique_ptr<mc::Buffer>(new StubBuffer()),
                std::unique_ptr<mc::Buffer>(new StubBuffer())));
    }
};

class MockGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
 public:
    MockGraphicBufferAllocator()
    {
        using testing::_;
        ON_CALL(*this, alloc_buffer(_,_))
            .WillByDefault(testing::Invoke(this, &MockGraphicBufferAllocator::on_create_swapper));
    }

    MOCK_METHOD2(
        alloc_buffer,
        std::unique_ptr<mc::Buffer> (geom::Size, geom::PixelFormat));

    std::unique_ptr<mc::Buffer> on_create_swapper(geom::Size, geom::PixelFormat)
    {
        return std::unique_ptr<mc::Buffer>(new StubBuffer());
    }
};


geom::Size const size{geom::Width{640}, geom::Height{480}};
geom::PixelFormat const format{geom::PixelFormat::rgba_8888};
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
using mir::TestingClientConfiguration;
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

TEST_F(BespokeDisplayServerTestFixture,
       creating_a_client_surface_allocates_buffer_swapper_on_server)
{
#ifndef ANDROID
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mc::BufferAllocationStrategy> make_buffer_allocation_strategy(
                std::shared_ptr<mc::GraphicBufferAllocator> const& /*buffer_allocator*/)
        {
            if (!buffer_allocation_strategy)
                buffer_allocation_strategy = std::make_shared<MockBufferAllocationStrategy>();

            EXPECT_CALL(*buffer_allocation_strategy, create_swapper(size, format)).Times(1);

            return buffer_allocation_strategy;
        }

        std::shared_ptr<MockBufferAllocationStrategy> buffer_allocation_strategy;
    } server_config;
#else
    TestingServerConfiguration server_config;
#endif

    launch_server_process(server_config);

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
            mir_surface_create(connection, &request_params, create_surface_callback, ssync);

            wait_for_surface_create(ssync);

            ASSERT_TRUE(ssync->surface != NULL);
            EXPECT_TRUE(mir_surface_is_valid(ssync->surface));
            EXPECT_STREQ(mir_surface_get_error_message(ssync->surface), "");

            MirSurfaceParameters response_params;
            mir_surface_get_parameters(ssync->surface, &response_params);
            EXPECT_EQ(request_params.width, response_params.width);
            EXPECT_EQ(request_params.height, response_params.height);
            EXPECT_EQ(request_params.pixel_format, response_params.pixel_format);

            mir_surface_release(ssync->surface, release_surface_callback, ssync);

            wait_for_surface_release(ssync);

            ASSERT_TRUE(ssync->surface == NULL);

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

namespace
{

/*
 * Need to declare outside method, because g++ 4.4 doesn't support local types
 * as template parameters (in std::make_shared<StubPlatform>()).
 */
struct ServerConfigAllocatesBuffersOnServer : TestingServerConfiguration
{
#ifndef ANDROID
    struct ServerConfig : TestingServerConfiguration
    {
     public:
        std::shared_ptr<mc::GraphicBufferAllocator> create_buffer_allocator(
                const std::shared_ptr<mg::BufferInitializer>& /*buffer_initializer*/)
        {
            using testing::AtLeast;

            auto buffer_allocator = std::make_shared<testing::NiceMock<MockGraphicBufferAllocator>>();
            EXPECT_CALL(*buffer_allocator,alloc_buffer(size, format)).Times(AtLeast(2));
            return buffer_allocator;
        }

        std::shared_ptr<MockGraphicBufferAllocator> buffer_allocator;
    } server_config;
#else
    TestingServerConfiguration server_config;
#endif

    class StubPlatform : public mg::Platform
    {
        std::shared_ptr<mc::GraphicBufferAllocator> create_buffer_allocator(
                const std::shared_ptr<mg::BufferInitializer>& /*buffer_initializer*/)
        {
            using testing::AtLeast;

            auto buffer_allocator = std::make_shared<testing::NiceMock<MockGraphicBufferAllocator>>();
            EXPECT_CALL(*buffer_allocator,alloc_buffer(size, format)).Times(AtLeast(2));
            return buffer_allocator;
        }

        std::shared_ptr<mg::Display> create_display()
        {
            return std::make_shared<StubDisplay>();
        }

        std::shared_ptr<mg::PlatformIPCPackage> get_ipc_package()
        {
            return std::make_shared<mg::PlatformIPCPackage>();
        }
    };

    std::shared_ptr<mg::Platform> make_graphics_platform()
    {
        if (!platform)
            platform = std::make_shared<StubPlatform>();

        return platform;
    }

    std::shared_ptr<mg::Platform> platform;
};

}

TEST_F(BespokeDisplayServerTestFixture,
       creating_a_client_surface_allocates_buffers_on_server)
{

    ServerConfigAllocatesBuffersOnServer server_config;

    launch_server_process(server_config);

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
                {__PRETTY_FUNCTION__, 640, 480, mir_pixel_format_rgba_8888};
            mir_surface_create(connection, &request_params, create_surface_callback, ssync);

            wait_for_surface_create(ssync);

            ASSERT_TRUE(ssync->surface != NULL);
            EXPECT_TRUE(mir_surface_is_valid(ssync->surface));
            EXPECT_STREQ(mir_surface_get_error_message(ssync->surface), "");

            MirSurfaceParameters response_params;
            mir_surface_get_parameters(ssync->surface, &response_params);
            EXPECT_EQ(request_params.width, response_params.width);
            EXPECT_EQ(request_params.height, response_params.height);
            EXPECT_EQ(request_params.pixel_format, response_params.pixel_format);


            mir_surface_release(ssync->surface, release_surface_callback, ssync);

            wait_for_surface_release(ssync);

            ASSERT_TRUE(ssync->surface == NULL);

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
            mir_connect(mir_test_socket, __PRETTY_FUNCTION__, connection_callback, this);

            wait_for_connect();

            MirSurfaceParameters request_params =
                {__PRETTY_FUNCTION__, 640, 480, mir_pixel_format_rgba_8888};

            mir_surface_create(connection, &request_params, create_surface_callback, ssync);
            wait_for_surface_create(ssync);

            request_params.width = 1600;
            request_params.height = 1200;

            mir_surface_create(connection, &request_params, create_surface_callback, ssync+1);
            wait_for_surface_create(ssync+1);

            MirSurfaceParameters response_params;
            mir_surface_get_parameters(ssync->surface, &response_params);
            EXPECT_EQ(640, response_params.width);
            EXPECT_EQ(480, response_params.height);
            EXPECT_EQ(mir_pixel_format_rgba_8888, response_params.pixel_format);

            mir_surface_get_parameters(ssync[1].surface, &response_params);
            EXPECT_EQ(1600, response_params.width);
            EXPECT_EQ(1200, response_params.height);
            EXPECT_EQ(mir_pixel_format_rgba_8888, response_params.pixel_format);

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
                {__PRETTY_FUNCTION__, 640, 480, mir_pixel_format_rgba_8888};

            mir_surface_create(connection, &request_params, create_surface_callback, ssync);
            wait_for_surface_create(ssync);

            request_params.width = 1600;
            request_params.height = 1200;

            mir_surface_create(connection, &request_params, create_surface_callback, ssync+1);
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
                {__PRETTY_FUNCTION__, 640, 480, mir_pixel_format_rgba_8888};

            for (int i = 0; i != max_surface_count; ++i)
                mir_surface_create(connection, &request_params, create_surface_callback, ssync+i);

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
                mir_surface_release( ssync[i].surface, release_surface_callback, ssync+i);

            for (int i = 0; i != max_surface_count; ++i)
                wait_for_surface_release(ssync+i);

            mir_connection_release(connection);
        }
    } client_creates_surfaces;

    launch_client_process(client_creates_surfaces);
}
