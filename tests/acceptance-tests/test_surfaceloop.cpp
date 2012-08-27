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

#include "mir_client/mir_surface.h"
#include "mir_client/mir_logger.h"

#include "display_server_test_fixture.h"



#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"

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

    struct ClientConfig : TestingClientConfiguration
    {
        void exec()
        {
            using ::mir::client::Surface;
            using ::mir::client::ConsoleLogger;

            Surface mysurface(mir::test_socket_file(), 640, 480, 0, std::make_shared<ConsoleLogger>());
            EXPECT_TRUE(mysurface.is_valid());
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

    struct ClientConfig : TestingClientConfiguration
    {
        void exec()
        {
            using ::mir::client::Surface;
            using ::mir::client::ConsoleLogger;

            Surface mysurface(mir::test_socket_file(), 640, 480, 0, std::make_shared<ConsoleLogger>());
            EXPECT_TRUE(mysurface.is_valid());
        }
    } client_config;

    launch_client_process(client_config);
}

TEST_F(DefaultDisplayServerTestFixture, creates_surface_of_correct_size)
{
    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            using ::mir::client::Surface;
            using ::mir::client::ConsoleLogger;

            auto const logger = std::make_shared<ConsoleLogger>();

            Surface mysurface(mir::test_socket_file(), 640, 480, 0, logger);

            EXPECT_EQ(640, mysurface.width());
            EXPECT_EQ(480, mysurface.height());
            EXPECT_EQ(0, mysurface.pix_format());

            mysurface = Surface(mir::test_socket_file(), 1600, 1200, 0, logger);

            EXPECT_EQ(1600, mysurface.width());
            EXPECT_EQ(1200, mysurface.height());
            EXPECT_EQ(0, mysurface.pix_format());
        }
    } client_connects_and_disconnects;

    launch_client_process(client_connects_and_disconnects);
}
