/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/fake_shared.h"
#include "src/server/graphics/nested/platform_image_factory.h"
#include "src/server/graphics/nested/native_buffer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtd = mir::test::doubles;
//namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;
using namespace testing;

namespace
{
struct MockNativeBuffer : mgn::NativeBuffer
{
    MOCK_CONST_METHOD0(client_handle, MirBuffer*());
    MOCK_METHOD0(get_native_handle, MirNativeBuffer*());
    MOCK_METHOD0(get_graphics_region, MirGraphicsRegion());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(format, MirPixelFormat());
    MOCK_METHOD2(sync, void(MirBufferAccess, std::chrono::nanoseconds));
};
struct PlatformImageFactory : Test
{
    PlatformImageFactory()
    {
    }

    NiceMock<mtd::MockGL> mock_gl;
    NiceMock<mtd::MockEGL> mock_egl;

    MockNativeBuffer buffer;
    EGLint attr = 4;
    EGLDisplay display = mock_egl.fake_egl_display;
};
}

TEST_F(PlatformImageFactory, creates_and_frees_buffer_correctly_for_android)
{
    EXPECT_CALL(mock_egl, eglCreateImageKHR(
        display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, nullptr, &attr));
    EXPECT_CALL(mock_egl, eglDestroyImageKHR(display, _));

    mgn::AndroidImageFactory factory;
    factory.create_egl_image_from(buffer, display, &attr);
}
