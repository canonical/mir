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
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/thread/all.h"

#include "mir_test_framework/display_server_test_fixture.h"

#include "mir_client/mir_client_library.h"

#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtf = mir_test_framework;

namespace mir /* So that std::this_thread::yield() can be found on android... */
{
namespace
{

class StubDisplay : public mg::Display
{
public:
    geom::Rectangle view_area() const { return rectangle; }
    void clear() { std::this_thread::yield(); }
    bool post_update() { return true; }

    static geom::Rectangle const rectangle;
};

geom::Rectangle const StubDisplay::rectangle{geom::Point{geom::X{25}, geom::Y{36}},
                                             geom::Size{geom::Width{49}, geom::Height{64}}};

}
}

using mir::StubDisplay;

namespace
{

char const* const mir_test_socket = mtf::test_socket_file().c_str();

class StubBuffer : public mc::Buffer
{
    geom::Size size() const { return geom::Size(); }

    geom::Stride stride() const { return geom::Stride(); }

    geom::PixelFormat pixel_format() const { return geom::PixelFormat(); }

    std::shared_ptr<mc::BufferIPCPackage> get_ipc_package() const
    {
        return std::make_shared<mc::BufferIPCPackage>();
    }

    void bind_to_texture() {}
};

class StubGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
public:
    std::unique_ptr<mc::Buffer> alloc_buffer(mc::BufferProperties const&)
    {
        return std::unique_ptr<mc::Buffer>(new StubBuffer());
    }

    std::vector<geom::PixelFormat> supported_pixel_formats()
    {
        return pixel_formats;
    }

    static std::vector<geom::PixelFormat> const pixel_formats;
};

std::vector<geom::PixelFormat> const StubGraphicBufferAllocator::pixel_formats{
    geom::PixelFormat::rgb_888,
    geom::PixelFormat::rgba_8888,
    geom::PixelFormat::rgbx_8888
};

class StubPlatform : public mg::Platform
{
public:
    std::shared_ptr<mc::GraphicBufferAllocator> create_buffer_allocator(
            std::shared_ptr<mg::BufferInitializer> const& /*buffer_initializer*/)
    {
        return std::make_shared<StubGraphicBufferAllocator>();
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
        std::shared_ptr<mg::Platform> make_graphics_platform()
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
