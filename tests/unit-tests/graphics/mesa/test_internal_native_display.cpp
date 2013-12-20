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

#include "src/platform/graphics/mesa/internal_native_display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"

#include "mir_toolkit/mesa/native_display.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/stub_surface_builder.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/mock_buffer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{

struct InternalNativeDisplay : public testing::Test
{
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
}

TEST_F(InternalNativeDisplay, display_get_platform_matches_construction_platform)
{
    using namespace ::testing;

    mgm::InternalNativeDisplay native_display(platform_package);

    MirPlatformPackage test_package;
    memset(&test_package, 0, sizeof(MirPlatformPackage));
    native_display.display_get_platform(&native_display, &test_package);
    EXPECT_THAT(test_package, PackageMatches(platform_package));
}

