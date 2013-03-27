/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/display_server.h"
#include "mir/graphics/egl/mesa_native_display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/compositor/buffer_ipc_package.h"

#include "mir_toolkit/c_types.h"
#include "mir_toolkit/mesa/native_display.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/stub_surface_builder.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/mock_buffer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mgeglm = mg::egl::mesa;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{

struct MockDisplayServer : public mir::DisplayServer
{
    MOCK_METHOD0(graphics_platform, std::shared_ptr<mg::Platform>());
};

struct MockGraphicsPlatform : public mg::Platform
{
    MOCK_METHOD1(create_buffer_allocator, std::shared_ptr<mc::GraphicBufferAllocator>(std::shared_ptr<mg::BufferInitializer> const&));
    MOCK_METHOD0(create_display, std::shared_ptr<mg::Display>());
    MOCK_METHOD0(get_ipc_package, std::shared_ptr<mg::PlatformIPCPackage>());
};

struct MirServerMesaEGLNativeDisplaySetup : public testing::Test
{
    MirServerMesaEGLNativeDisplaySetup()
    {
        using namespace ::testing;

        test_platform_package.ipc_data = {1, 2};
        test_platform_package.ipc_fds = {2, 3};
        
        ON_CALL(mock_server, graphics_platform())
            .WillByDefault(Return(mt::fake_shared(graphics_platform))); 

        ON_CALL(graphics_platform, get_ipc_package())
            .WillByDefault(Return(mt::fake_shared(test_platform_package)));
    }

    // Test dependencies
    MockDisplayServer mock_server;
    MockGraphicsPlatform graphics_platform;

    // Useful stub data
    mg::PlatformIPCPackage test_platform_package;
};

MATCHER_P(PlatformPackageMatches, package, "")
{
    if (arg.data_items != (int)package.ipc_data.size())
        return false;
    if (arg.fd_items != (int)package.ipc_fds.size())
        return false;
    // TODO: Match contents ~racarr
    return true;
}

}

TEST_F(MirServerMesaEGLNativeDisplaySetup, display_get_platform_is_cached_platform_package)
{
    using namespace ::testing;
    
    EXPECT_CALL(mock_server, graphics_platform()).Times(1);
    EXPECT_CALL(graphics_platform, get_ipc_package()).Times(1);

    auto display = mgeglm::create_native_display(&mock_server);
    
    mir_toolkit::MirPlatformPackage package;
    display->display_get_platform(display.get(), &package);
    EXPECT_THAT(package, PlatformPackageMatches(test_platform_package));

    mir_toolkit::MirPlatformPackage requeried_package;
    // The package is cached to allow the package creator to control lifespan of fd members
    // and so this should not trigger another call to get_ipc_package()
    display->display_get_platform(display.get(), &requeried_package);
    
    EXPECT_FALSE(memcmp(&requeried_package.data, &package.data, sizeof(package.data[0])*package.data_items));
    EXPECT_FALSE(memcmp(&requeried_package.fd, &package.fd, sizeof(package.fd[0])*package.fd_items));
}

TEST_F(MirServerMesaEGLNativeDisplaySetup, surface_get_current_buffer)
{
    using namespace ::testing;
    
    mtd::MockSurface surface(std::make_shared<mtd::StubSurfaceBuilder>());
    auto buffer = std::make_shared<mtd::MockBuffer>(geom::Size(), geom::Stride(), geom::PixelFormat()); // TODO: Populate with some contents ~racarr
    
    mc::BufferIPCPackage test_buffer_package;
    
    test_buffer_package.ipc_data = {1, 2};
    test_buffer_package.ipc_fds = {2, 3};

    EXPECT_CALL(mock_server, graphics_platform()).Times(1);
    auto display = mgeglm::create_native_display(&mock_server);
    
    EXPECT_CALL(surface, client_buffer()).Times(1)
        .WillOnce(Return(buffer));
    EXPECT_CALL(*buffer, get_ipc_package()).Times(1)
        .WillOnce(Return(mt::fake_shared(test_buffer_package)));
    
    mir_toolkit::MirBufferPackage buffer_package;
    display->surface_get_current_buffer(
        display.get(), static_cast<mir_toolkit::MirEGLNativeWindowType>(&surface), &buffer_package);
    // TODO: Match contents
}

TEST_F(MirServerMesaEGLNativeDisplaySetup, surface_advance_buffer)
{
    using namespace ::testing;
    
    mtd::MockSurface surface(std::make_shared<mtd::StubSurfaceBuilder>());
    
    EXPECT_CALL(mock_server, graphics_platform()).Times(1);
    auto display = mgeglm::create_native_display(&mock_server);
    
    EXPECT_CALL(surface, advance_client_buffer()).Times(1);
    
    display->surface_advance_buffer(
        display.get(), static_cast<mir_toolkit::MirEGLNativeWindowType>(&surface));
    // TODO: Match contents
}

TEST_F(MirServerMesaEGLNativeDisplaySetup, surface_get_parameters)
{
    using namespace ::testing;
    
    EXPECT_CALL(mock_server, graphics_platform()).Times(1);
    auto display = mgeglm::create_native_display(&mock_server);

    mtd::MockSurface surface(std::make_shared<mtd::StubSurfaceBuilder>());
    // TODO: Contents for matching ~racarr
    EXPECT_CALL(surface, size()).Times(AtLeast(1)).WillRepeatedly(Return(geom::Size()));
    EXPECT_CALL(surface, pixel_format()).Times(AtLeast(1)).WillRepeatedly(Return(geom::PixelFormat()));
    
    mir_toolkit::MirSurfaceParameters parameters;
    display->surface_get_parameters(
        display.get(), static_cast<mir_toolkit::MirEGLNativeWindowType>(&surface), &parameters);
    // TODO: Match contents

    // TODO: What to do about buffer usage besides hardware? ~racarr
    EXPECT_EQ(parameters.buffer_usage, mir_toolkit::mir_buffer_usage_hardware);
}
