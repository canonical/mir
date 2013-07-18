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
#include "mir_test_doubles/null_platform.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

namespace mir /* So that std::this_thread::yield() can be found on android... */
{
namespace
{

class StubDisplay : public mtd::NullDisplay
{
public:
    geom::Rectangle view_area() const { return rectangle; }

    static geom::Rectangle const rectangle;
};

geom::Rectangle const StubDisplay::rectangle{geom::Point{geom::X{25}, geom::Y{36}},
                                             geom::Size{49, 64}};

}
}

using mir::StubDisplay;

namespace
{

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

            MirDisplayInfo info;

            mir_connection_get_display_info(connection, &info);

            EXPECT_EQ(StubDisplay::rectangle.size.width.as_uint32_t(),
                      static_cast<uint32_t>(info.width));
            EXPECT_EQ(StubDisplay::rectangle.size.height.as_uint32_t(),
                      static_cast<uint32_t>(info.height));

            ASSERT_EQ(StubGraphicBufferAllocator::pixel_formats.size(),
                      static_cast<uint32_t>(info.supported_pixel_format_items));

            for (int i = 0; i < info.supported_pixel_format_items; ++i)
            {
                EXPECT_EQ(StubGraphicBufferAllocator::pixel_formats[i],
                          static_cast<geom::PixelFormat>(info.supported_pixel_format[i]));
            }

            mir_connection_release(connection);
        }
    } client_config;

    launch_client_process(client_config);
}
