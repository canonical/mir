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

#include "mir_toolkit/c_types.h"
#include "mir_toolkit/mesa/native_display.h"

#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mgeglm = mg::egl::mesa;
namespace mc = mir::compositor;
namespace mt = mir::test;

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

// TODO: Fixture ~racarr
TEST(MirServerMesaEGLNativeDisplay, display_get_platform_queries_server_display)
{
    using namespace ::testing;

    MockDisplayServer mock_server;
    MockGraphicsPlatform graphics_platform;
    mg::PlatformIPCPackage platform_package;
    
    platform_package.ipc_data = {1, 2};
    platform_package.ipc_fds = {2, 3};

    EXPECT_CALL(mock_server, graphics_platform()).Times(1)
        .WillOnce(Return(mt::fake_shared(graphics_platform))); // TODO: Test that this is cached once we have a fixture ~racarr
    EXPECT_CALL(graphics_platform, get_ipc_package()).Times(1)
        .WillOnce(Return(mt::fake_shared(platform_package)));

    auto display = mgeglm::create_native_display(&mock_server);
    
    mir_toolkit::MirPlatformPackage package;
    display->display_get_platform(display.get(), &package);
    EXPECT_THAT(package, PlatformPackageMatches(platform_package));
}
