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
#include "mir/graphics/egl/mesa_native_display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/compositor/buffer_ipc_package.h"

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
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{

struct MockGraphicsPlatform : public mg::Platform
{
    MOCK_METHOD1(create_buffer_allocator, std::shared_ptr<mc::GraphicBufferAllocator>(std::shared_ptr<mg::BufferInitializer> const&));
    MOCK_METHOD0(create_display, std::shared_ptr<mg::Display>());
    MOCK_METHOD0(get_ipc_package, std::shared_ptr<mg::PlatformIPCPackage>());
    MOCK_METHOD0(shell_egl_display, EGLNativeDisplayType());
};

struct MirServerMesaEGLNativeDisplaySetup : public testing::Test
{
    MirServerMesaEGLNativeDisplaySetup()
    {
        using namespace ::testing;

        test_platform_package.ipc_data = {1, 2};
        test_platform_package.ipc_fds = {2, 3};
        
        ON_CALL(mock_graphics_platform, get_ipc_package())
            .WillByDefault(Return(mt::fake_shared(test_platform_package)));
    }

    // Test dependencies
    MockGraphicsPlatform mock_graphics_platform;

    // Useful stub data
    mg::PlatformIPCPackage test_platform_package;
};

MATCHER_P(PackageMatches, package, "")
{
    if (arg.data_items != static_cast<int>(package.ipc_data.size()))
        return false;
    for (uint i = 0; i < package.ipc_data.size(); i++)
    {
        if (arg.data[i] != package.ipc_data[i]) return false;
    }
    if (arg.fd_items != static_cast<int>(package.ipc_fds.size()))
        return false;
    for (uint i = 0; i < package.ipc_fds.size(); i++)
    {
        if (arg.fd[i] != package.ipc_fds[i]) return false;
    }
    return true;
}

MATCHER_P(StrideMatches, package, "")
{
    if (arg.stride != package.stride)
    {
        return false;
    }
    return true;
}

MATCHER_P(ParametersHaveSize, size, "")
{
    if (static_cast<uint32_t>(arg.width) != size.width.as_uint32_t())
        return false;
    if (static_cast<uint32_t>(arg.height) != size.height.as_uint32_t())
        return false;
    return true;
}

}

TEST_F(MirServerMesaEGLNativeDisplaySetup, display_get_platform_is_cached_platform_package)
{
    using namespace ::testing;
    
    EXPECT_CALL(mock_graphics_platform, get_ipc_package()).Times(1);

    auto display = mgeglm::create_native_display(mt::fake_shared(mock_graphics_platform));
    
    MirPlatformPackage package;
    memset(&package, 0, sizeof(MirPlatformPackage));
    display->display_get_platform(display.get(), &package);
    EXPECT_THAT(package, PackageMatches(test_platform_package));

    MirPlatformPackage requeried_package;
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
    auto buffer = std::make_shared<mtd::MockBuffer>(geom::Size(), geom::Stride(), geom::PixelFormat());
    
    mc::BufferIPCPackage test_buffer_package;
    
    test_buffer_package.ipc_data = {1, 2};
    test_buffer_package.ipc_fds = {2, 3};
    test_buffer_package.stride = 77;

    auto display = mgeglm::create_native_display(mt::fake_shared(mock_graphics_platform));
    
    EXPECT_CALL(surface, client_buffer()).Times(1)
        .WillOnce(Return(buffer));
    EXPECT_CALL(*buffer, get_ipc_package()).Times(1)
        .WillOnce(Return(mt::fake_shared(test_buffer_package)));
    
    MirBufferPackage buffer_package;
    memset(&buffer_package, 0, sizeof(MirBufferPackage));
    display->surface_get_current_buffer(
        display.get(), static_cast<MirEGLNativeWindowType>(&surface), &buffer_package);
    EXPECT_THAT(buffer_package, AllOf(PackageMatches(test_buffer_package), StrideMatches(test_buffer_package)));
}

TEST_F(MirServerMesaEGLNativeDisplaySetup, surface_advance_buffer)
{
    using namespace ::testing;
    
    mtd::MockSurface surface(std::make_shared<mtd::StubSurfaceBuilder>());
    
    auto display = mgeglm::create_native_display(mt::fake_shared(mock_graphics_platform));
    
    EXPECT_CALL(surface, advance_client_buffer()).Times(1);
    
    display->surface_advance_buffer(
        display.get(), static_cast<MirEGLNativeWindowType>(&surface));
}

TEST(MirServerMesaEGLNativeDisplayInvariants, pixel_format_formats_are_castable)
{
    EXPECT_EQ(static_cast<MirPixelFormat>(geom::PixelFormat::invalid), mir_pixel_format_invalid);
    EXPECT_EQ(static_cast<MirPixelFormat>(geom::PixelFormat::abgr_8888), mir_pixel_format_abgr_8888);
    EXPECT_EQ(static_cast<MirPixelFormat>(geom::PixelFormat::xbgr_8888), mir_pixel_format_xbgr_8888);
    EXPECT_EQ(static_cast<MirPixelFormat>(geom::PixelFormat::argb_8888), mir_pixel_format_argb_8888);
    EXPECT_EQ(static_cast<MirPixelFormat>(geom::PixelFormat::xrgb_8888), mir_pixel_format_xrgb_8888);
    EXPECT_EQ(static_cast<MirPixelFormat>(geom::PixelFormat::bgr_888), mir_pixel_format_bgr_888);
}

TEST_F(MirServerMesaEGLNativeDisplaySetup, surface_get_parameters)
{
    using namespace ::testing;
    
    geom::Size const test_surface_size = geom::Size{geom::Width{17},
                                                    geom::Height{29}};
    geom::PixelFormat const test_pixel_format = geom::PixelFormat::xrgb_8888;

    auto display = mgeglm::create_native_display(mt::fake_shared(mock_graphics_platform));

    mtd::MockSurface surface(std::make_shared<mtd::StubSurfaceBuilder>());
    EXPECT_CALL(surface, size()).Times(AtLeast(1)).WillRepeatedly(Return(test_surface_size));
    EXPECT_CALL(surface, pixel_format()).Times(AtLeast(1)).WillRepeatedly(Return(test_pixel_format));
    
    MirSurfaceParameters parameters;
    memset(&parameters, 0, sizeof(MirSurfaceParameters));
    display->surface_get_parameters(
        display.get(), static_cast<MirEGLNativeWindowType>(&surface), &parameters);

    EXPECT_THAT(parameters, ParametersHaveSize(test_surface_size));
    EXPECT_EQ(parameters.pixel_format, static_cast<MirPixelFormat>(geom::PixelFormat::xrgb_8888));

    // TODO: What to do about buffer usage besides hardware? ~racarr
    EXPECT_EQ(parameters.buffer_usage, mir_buffer_usage_hardware);
}
