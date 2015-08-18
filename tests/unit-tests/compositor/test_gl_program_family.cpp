/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir/compositor/gl_program_family.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/mock_egl.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;

// Regression test for LP: #1454201
TEST(GLProgramFamily, releases_gl_context_before_deleting_shader_objects)
{
    using namespace testing;

    NiceMock<mtd::MockGL> mock_gl;
    NiceMock<mtd::MockEGL> mock_egl;

    ON_CALL(mock_gl, glCreateShader(_)).WillByDefault(Return(1));
    ON_CALL(mock_gl, glCreateProgram()).WillByDefault(Return(1));

    {
        mc::GLProgramFamily family;
        family.add_program("vertex shader", "fragment shader");

        InSequence seq;
        EXPECT_CALL(mock_egl, eglMakeCurrent(_,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT));
        EXPECT_CALL(mock_gl, glDeleteProgram(_)).Times(1);
        EXPECT_CALL(mock_gl, glDeleteShader(_)).Times(2);
    }
}
