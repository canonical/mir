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

#include "mir/client_platform.h"
#include "mir_test_doubles/mock_client_context.h"
#include "mir_test_doubles/mock_egl_native_surface.h"
#include "mir_test_framework/client_platform_factory.h"

#include <EGL/egl.h>

#include <gtest/gtest.h>

namespace mcl = mir::client;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

TEST(AndroidClientPlatformTest, egl_native_display_is_egl_default_display)
{
    mtd::MockEGLNativeSurface surface;
    auto platform = mtf::create_android_client_platform();
    auto mock_egl_native_surface = std::make_shared<mtd::MockEGLNativeSurface>();
    auto native_display = platform->create_egl_native_display();
    EXPECT_EQ(EGL_DEFAULT_DISPLAY, *native_display);
}

TEST(AndroidClientPlatformTest, egl_native_window_is_set)
{
    mtd::MockEGLNativeSurface surface;
    auto platform = mtf::create_android_client_platform();
    auto mock_egl_native_surface = std::make_shared<mtd::MockEGLNativeSurface>();
    auto egl_native_window = platform->create_egl_native_window(&surface);
    EXPECT_NE(nullptr, egl_native_window);
}
