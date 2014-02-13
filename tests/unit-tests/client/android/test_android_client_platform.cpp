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
#include "src/client/android/client_platform_factory.h"
#include "mir_test_doubles/mock_client_context.h"
#include "mir_test_doubles/mock_client_surface.h"

#include <EGL/egl.h>

#include <gtest/gtest.h>

namespace mcl = mir::client;
namespace mt = mir::test;
namespace mtd = mt::doubles;

TEST(AndroidClientPlatformTest, egl_native_display_is_egl_default_display)
{
    mtd::MockClientContext context;
    mcl::android::ClientPlatformFactory factory;
    mtd::MockClientSurface surface;
    auto platform = factory.create_client_platform(&context);
    auto mock_client_surface = std::make_shared<mtd::MockClientSurface>();
    auto native_display = platform->create_egl_native_display();
    EXPECT_EQ(EGL_DEFAULT_DISPLAY, *native_display);
}

TEST(AndroidClientPlatformTest, egl_native_window_is_set)
{
    mtd::MockClientContext context;
    mcl::android::ClientPlatformFactory factory;
    mtd::MockClientSurface surface;
    auto platform = factory.create_client_platform(&context);
    auto mock_client_surface = std::make_shared<mtd::MockClientSurface>();
    auto egl_native_window = platform->create_egl_native_window(&surface);
    EXPECT_NE(nullptr, egl_native_window);
}
