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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/display.h"
#include "mir/graphics/buffer.h"
#include "mir/compositor/graphic_buffer_allocator.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/null_platform.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

namespace
{

class StubDisplay : public mtd::NullDisplay
{
public:
    StubDisplay()
        : display_buffer{rectangle}
    {
    }

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f) override
    {
        f(display_buffer);
    }

    static geom::Rectangle rectangle;
    static geom::Size const expected_dimensions_mm;

private:
    mtd::StubDisplayBuffer display_buffer;
};

geom::Size const StubDisplay::expected_dimensions_mm{0,0};
geom::Rectangle StubDisplay::rectangle{geom::Point{25,36}, geom::Size{49,64}};

char const* const mir_test_socket = mtf::test_socket_file().c_str();

class StubGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
public:
    std::shared_ptr<mg::Buffer> alloc_buffer(mc::BufferProperties const&)
    {
        return std::shared_ptr<mg::Buffer>(new mtd::StubBuffer());
    }

    std::vector<geom::PixelFormat> supported_pixel_formats()
    {
        return pixel_formats;
    }

    static std::vector<geom::PixelFormat> const pixel_formats;
};

std::vector<geom::PixelFormat> const StubGraphicBufferAllocator::pixel_formats{
    geom::PixelFormat::bgr_888,
    geom::PixelFormat::abgr_8888,
    geom::PixelFormat::xbgr_8888
};

class StubPlatform : public mtd::NullPlatform
{
public:
    std::shared_ptr<mc::GraphicBufferAllocator> create_buffer_allocator(
            std::shared_ptr<mg::BufferInitializer> const& /*buffer_initializer*/) override
    {
        return std::make_shared<StubGraphicBufferAllocator>();
    }

    std::shared_ptr<mg::Display> create_display(
        std::shared_ptr<mg::DisplayConfigurationPolicy> const&) override
    {
        return std::make_shared<StubDisplay>();
    }
};

void connection_callback(MirConnection* connection, void* context)
{
    auto connection_ptr = static_cast<MirConnection**>(context);
    *connection_ptr = connection;
}

}

TEST_F(BespokeDisplayServerTestFixture, display_info_reaches_client)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mg::Platform> the_graphics_platform()
        {
            using namespace testing;

            if (!platform)
                platform = std::make_shared<StubPlatform>();

            return platform;
        }

        std::shared_ptr<StubPlatform> platform;
    } server_config;

    launch_server_process(server_config);

    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            MirConnection* connection{nullptr};
            mir_wait_for(mir_connect(mir_test_socket, __PRETTY_FUNCTION__,
                                     connection_callback, &connection));


            auto configuration = mir_connection_create_display_config(connection);

            /* TODO: expand test to test multimonitor situations */
            ASSERT_EQ(1u, configuration->num_displays);
            auto const& info = configuration->displays[0];
            geom::Rectangle const& expected_rect = StubDisplay::rectangle;

            //state
            EXPECT_EQ(1u, info.connected);
            EXPECT_EQ(1u, info.used);

            //id's
            EXPECT_EQ(0u, info.output_id);
            EXPECT_EQ(0u, info.card_id);

            //sizing
            EXPECT_EQ(StubDisplay::expected_dimensions_mm.width.as_uint32_t(), info.physical_width_mm);
            EXPECT_EQ(StubDisplay::expected_dimensions_mm.height.as_uint32_t(), info.physical_height_mm);

            //position
            EXPECT_EQ(static_cast<int>(expected_rect.top_left.x.as_uint32_t()), info.position_x); 
            EXPECT_EQ(static_cast<int>(expected_rect.top_left.y.as_uint32_t()), info.position_y); 

            //mode selection
            EXPECT_EQ(1u, info.num_modes);
            ASSERT_EQ(0u, info.current_mode);
            auto const& mode = info.modes[info.current_mode];

            //current mode 
            EXPECT_EQ(expected_rect.size.width.as_uint32_t(), mode.horizontal_resolution);
            EXPECT_EQ(expected_rect.size.height.as_uint32_t(), mode.vertical_resolution);
            EXPECT_FLOAT_EQ(60.0f, mode.refresh_rate);

            //pixel formats
            ASSERT_EQ(StubGraphicBufferAllocator::pixel_formats.size(),
                      static_cast<uint32_t>(info.num_output_formats));
            for (auto i=0u; i < info.num_output_formats; ++i)
            {
                EXPECT_EQ(StubGraphicBufferAllocator::pixel_formats[i],
                          static_cast<geom::PixelFormat>(info.output_formats[i]));
            }
            EXPECT_EQ(0u, info.current_output_format);

            mir_display_config_destroy(configuration);
            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}

/* TODO: this test currently checks the same format list against both the surface formats
         and display formats. Improve test to return different format lists for both concepts */ 
TEST_F(BespokeDisplayServerTestFixture, display_surface_pfs_reaches_client)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mg::Platform> the_graphics_platform()
        {
            using namespace testing;

            if (!platform)
                platform = std::make_shared<StubPlatform>();

            return platform;
        }

        std::shared_ptr<StubPlatform> platform;
    } server_config;

    launch_server_process(server_config);

    struct Client : TestingClientConfiguration
    {
        void exec()
        {
            MirConnection* connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);

            unsigned int const format_storage_size = 4;
            MirPixelFormat formats[format_storage_size]; 
            unsigned int returned_format_size = 0;
            mir_connection_get_available_surface_formats(connection,
                formats, format_storage_size, &returned_format_size);

            ASSERT_EQ(returned_format_size, StubGraphicBufferAllocator::pixel_formats.size());
            for (auto i=0u; i < returned_format_size; ++i)
            {
                EXPECT_EQ(StubGraphicBufferAllocator::pixel_formats[i],
                          static_cast<geom::PixelFormat>(formats[i]));
            }

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}
