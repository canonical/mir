/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/graphics/egl_extensions.h>
#include <mir/test/doubles/mock_egl.h>
#include <stdexcept>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
using namespace testing;

typedef mtd::MockEGL::generic_function_pointer_t func_ptr_t;
class EGLExtensions  : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        mock_egl.provide_egl_extensions();
    }

    testing::NiceMock<mtd::MockEGL> mock_egl;
};

TEST_F(EGLExtensions, constructor_throws_if_egl_image_not_supported)
{
    ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglCreateImageKHR")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(0)));
    ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglDestroyImageKHR")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(0)));

    mg::EGLExtensions extensions;
    EGLDisplay dpy = eglGetDisplay(nullptr);

    EXPECT_THROW({
        extensions.base(dpy);
    }, std::runtime_error);
}

TEST_F(EGLExtensions, base_throws_if_gl_oes_egl_image_not_supported)
{
    ON_CALL(mock_egl, eglGetProcAddress(StrEq("glEGLImageTargetTexture2DOES")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(0)));

    mg::EGLExtensions extensions;
    EGLDisplay dpy = eglGetDisplay(nullptr);

    EXPECT_THROW({
        extensions.base(dpy);
    }, std::runtime_error);
}

TEST_F(EGLExtensions, success_has_sane_function_hooks)
{
    mg::EGLExtensions extensions;
    EGLDisplay dpy = eglGetDisplay(nullptr);
    EXPECT_NE(nullptr, extensions.base(dpy).eglCreateImageKHR);
    EXPECT_NE(nullptr, extensions.base(dpy).eglDestroyImageKHR);
    EXPECT_NE(nullptr, extensions.base(dpy).glEGLImageTargetTexture2DOES);
}
