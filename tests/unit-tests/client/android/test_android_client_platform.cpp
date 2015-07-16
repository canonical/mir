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
#include "mir/test/doubles/mock_client_context.h"
#include "mir/test/doubles/mock_egl_native_surface.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir_test_framework/client_platform_factory.h"

#include <EGL/egl.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
using namespace mir::client;
using namespace mir::test;
using namespace mir::test::doubles;
using namespace mir_test_framework;

struct AndroidClientPlatformTest : public Test
{
    AndroidClientPlatformTest()
        : platform{create_android_client_platform()}
    {
    }

    std::shared_ptr<mir::client::ClientPlatform> platform;
    MockEGL mock_egl;
};

TEST_F(AndroidClientPlatformTest, egl_native_display_is_egl_default_display)
{
    MockEGLNativeSurface surface;
    auto mock_egl_native_surface = std::make_shared<MockEGLNativeSurface>();
    auto native_display = platform->create_egl_native_display();
    EXPECT_EQ(EGL_DEFAULT_DISPLAY, *native_display);
}

TEST_F(AndroidClientPlatformTest, egl_native_window_is_set)
{
    MockEGLNativeSurface surface;
    auto mock_egl_native_surface = std::make_shared<MockEGLNativeSurface>();
    auto egl_native_window = platform->create_egl_native_window(&surface);
    EXPECT_NE(nullptr, egl_native_window);
}

TEST_F(AndroidClientPlatformTest, egl_pixel_format_asks_the_driver)
{
    auto const d = reinterpret_cast<EGLDisplay>(0x1234);
    auto const c = reinterpret_cast<EGLConfig>(0x5678);

    // Verify Android is using:
    // https://www.khronos.org/registry/egl/extensions/KHR/EGL_KHR_platform_android.txt
    // and that's all we can hope for.
    EXPECT_CALL(mock_egl, eglGetConfigAttrib(d, c, EGL_NATIVE_VISUAL_ID, _))
        .WillOnce(Return(EGL_TRUE));

    platform->get_egl_pixel_format(d, c);
}
