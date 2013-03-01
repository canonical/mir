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

#include "src/client/client_platform.h"
#include "src/client/native_client_platform_factory.h"
#include "mir_test_doubles/mock_client_context.h"
#include "mir_test_doubles/mock_client_surface.h"

#include <gtest/gtest.h>

namespace mcl = mir::client;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

TEST(GBMClientPlatformTest, egl_native_window_is_client_surface)
{
    mtd::MockClientContext context;
    mcl::NativeClientPlatformFactory factory;
    mtd::MockClientSurface surface;
    auto platform = factory.create_client_platform(&context);
    auto mock_client_surface = std::make_shared<mtd::MockClientSurface>();
    auto native_window = platform->create_egl_native_window(&surface);
    EXPECT_EQ(reinterpret_cast<EGLNativeWindowType>(&surface), *native_window);
}

TEST(GBMClientPlatformTest, egl_native_display_is_valid_until_released)
{
    mtd::MockClientContext context;
    mcl::NativeClientPlatformFactory factory;
    auto platform = factory.create_client_platform(&context);

    EGLNativeDisplayType nd;
    {
        std::shared_ptr<EGLNativeDisplayType> native_display = platform->create_egl_native_display();

        nd = *native_display;
        EXPECT_TRUE(mir_egl_native_display_is_valid(nd));
    }
    EXPECT_FALSE(mir_egl_native_display_is_valid(nd));
}
