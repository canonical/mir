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

#include "src/server/graphics/gbm/internal_native_display.h"
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
namespace mgg = mir::graphics::gbm;
namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{
struct InternalNativeDisplay : public testing::Test
{
    InternalNativeDisplay()
    {
    }

    void SetUp()
    {
        using namespace ::testing;

        platform_package = std::make_shared<mg::PlatformIPCPackage>();
        platform_package->ipc_data = {1, 2};
        platform_package->ipc_fds = {2, 3};
    }

    std::shared_ptr<mg::PlatformIPCPackage> platform_package;
};

MATCHER_P(PackageMatches, package, "")
{
    if (arg.data_items != static_cast<int>(package->ipc_data.size()))
        return false;
    for (uint i = 0; i < package->ipc_data.size(); i++)
    {
        if (arg.data[i] != package->ipc_data[i]) return false;
    }
    if (arg.fd_items != static_cast<int>(package->ipc_fds.size()))
        return false;
    for (uint i = 0; i < package->ipc_fds.size(); i++)
    {
        if (arg.fd[i] != package->ipc_fds[i]) return false;
    }
    return true;
}

MATCHER_P(StrideMatches, package, "")
{
    if (arg.stride != package->stride)
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

TEST_F(InternalNativeDisplay, display_get_platform_matches_construction_platform)
{
    using namespace ::testing;
    
    mgg::InternalNativeDisplay native_display(platform_package); 
    
    MirPlatformPackage test_package;
    memset(&test_package, 0, sizeof(MirPlatformPackage));
    native_display.display_get_platform(&native_display, &test_package);
    EXPECT_THAT(test_package, PackageMatches(platform_package));
}

TEST_F(InternalNativeDisplay, surface_get_current_buffer)
{
    using namespace ::testing;
    
    mtd::MockSurface surface(std::make_shared<mtd::StubSurfaceBuilder>());
    auto buffer = std::make_shared<mtd::MockBuffer>(geom::Size(), geom::Stride(), geom::PixelFormat());
    
    auto test_buffer_package = std::make_shared<mc::BufferIPCPackage>();
    
    test_buffer_package->ipc_data = {1, 2};
    test_buffer_package->ipc_fds = {2, 3};
    test_buffer_package->stride = 77;

    mgg::InternalNativeDisplay native_display(platform_package); 
    
    EXPECT_CALL(surface, client_buffer()).Times(1)
        .WillOnce(Return(buffer));
    EXPECT_CALL(*buffer, get_ipc_package()).Times(1)
        .WillOnce(Return(test_buffer_package));
    
    MirBufferPackage buffer_package;
    memset(&buffer_package, 0, sizeof(MirBufferPackage));
    native_display.surface_get_current_buffer(
        &native_display, static_cast<MirEGLNativeWindowType>(&surface), &buffer_package);
    EXPECT_THAT(buffer_package, AllOf(PackageMatches(test_buffer_package), StrideMatches(test_buffer_package)));
}

TEST_F(InternalNativeDisplay, surface_advance_buffer)
{
    using namespace ::testing;
    
    mtd::MockSurface surface(std::make_shared<mtd::StubSurfaceBuilder>());
    
    mgg::InternalNativeDisplay native_display(platform_package); 
    
    EXPECT_CALL(surface, advance_client_buffer()).Times(1);
    
    native_display.surface_advance_buffer(
        &native_display, static_cast<MirEGLNativeWindowType>(&surface));
}

TEST_F(InternalNativeDisplay, surface_get_parameters)
{
    using namespace ::testing;
    
    geom::Size const test_surface_size = geom::Size{geom::Width{17},
                                                    geom::Height{29}};
    geom::PixelFormat const test_pixel_format = geom::PixelFormat::xrgb_8888;

    mgg::InternalNativeDisplay native_display(platform_package); 

    mtd::MockSurface surface(std::make_shared<mtd::StubSurfaceBuilder>());
    EXPECT_CALL(surface, size()).Times(AtLeast(1)).WillRepeatedly(Return(test_surface_size));
    EXPECT_CALL(surface, pixel_format()).Times(AtLeast(1)).WillRepeatedly(Return(test_pixel_format));
    
    MirSurfaceParameters parameters;
    memset(&parameters, 0, sizeof(MirSurfaceParameters));
    native_display.surface_get_parameters(
        &native_display, static_cast<MirEGLNativeWindowType>(&surface), &parameters);

    EXPECT_THAT(parameters, ParametersHaveSize(test_surface_size));
    EXPECT_EQ(parameters.pixel_format, static_cast<MirPixelFormat>(geom::PixelFormat::xrgb_8888));

    // TODO: What to do about buffer usage besides hardware? ~racarr
    EXPECT_EQ(parameters.buffer_usage, mir_buffer_usage_hardware);
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
