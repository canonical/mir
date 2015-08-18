/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/egl_extensions.h"
#include "mir/test/doubles/mock_egl.h"
#include <stdexcept>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

class EGLExtensionsTest  : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }

    testing::NiceMock<mtd::MockEGL> mock_egl;
};

TEST_F(EGLExtensionsTest, constructor_throws_if_egl_image_not_supported)
{
    using namespace testing;
    typedef mtd::MockEGL::generic_function_pointer_t func_ptr_t;

    ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglCreateImageKHR")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(0)));
    ON_CALL(mock_egl, eglGetProcAddress(StrEq("eglDestroyImageKHR")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(0)));

    EXPECT_THROW({
        mg::EGLExtensions extensions;
    }, std::runtime_error);
}

TEST_F(EGLExtensionsTest, constructor_throws_if_gl_oes_egl_image_not_supported)
{
    using namespace testing;
    typedef mtd::MockEGL::generic_function_pointer_t func_ptr_t;

    ON_CALL(mock_egl, eglGetProcAddress(StrEq("glEGLImageTargetTexture2DOES")))
        .WillByDefault(Return(reinterpret_cast<func_ptr_t>(0)));

    EXPECT_THROW({
        mg::EGLExtensions extensions;
    }, std::runtime_error);
}

TEST_F(EGLExtensionsTest, success_has_sane_function_hooks)
{
    mg::EGLExtensions extensions;
    EXPECT_NE(nullptr, extensions.eglCreateImageKHR);
    EXPECT_NE(nullptr, extensions.eglDestroyImageKHR);
    EXPECT_NE(nullptr, extensions.glEGLImageTargetTexture2DOES);
}

