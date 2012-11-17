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

#include "mir_client/client_platform.h"
#include "mir_client/native_client_platform_factory.h"
#include "mir_test/mock_client_context.h"
#include "mir_test/mock_client_surface.h"

#include <EGL/egl.h>

#include <gtest/gtest.h>

namespace mcl = mir::client;
namespace mt = mir::test;

TEST(AndroidClientPlatformTest, egl_native_display_is_egl_default_display)
{
    mt::MockClientContext context;
    mcl::NativeClientPlatformFactory factory;
    mt::MockClientSurface surface;
    auto platform = factory.create_client_platform(&context);
    auto mock_client_surface = std::make_shared<mt::MockClientSurface>();
    auto native_display = platform->create_egl_native_display();
    EGLNativeDisplayType egl_native_display = native_display->get_egl_native_display();
    EXPECT_EQ(EGL_DEFAULT_DISPLAY, egl_native_display);
}

TEST(AndroidClientPlatformTest, egl_native_window_is_set)
{
    mt::MockClientContext context;
    mcl::NativeClientPlatformFactory factory;
    mt::MockClientSurface surface;
    auto platform = factory.create_client_platform(&context);
    auto mock_client_surface = std::make_shared<mt::MockClientSurface>();
    auto egl_native_window = platform->create_egl_window(&surface);
    EXPECT_NE(nullptr, egl_native_window);
}
