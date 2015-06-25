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
 * Authored by:
 * Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/test/doubles/mock_egl.h"
#include "gtest/gtest.h"

namespace mtd = mir::test::doubles;

TEST(EglMock, demo)
{
    using namespace ::testing;

    mtd::MockEGL mock;
    EXPECT_CALL(mock, eglGetError()).Times(Exactly(1));
    eglGetError();
}

#define EGL_MOCK_TEST(test, egl_call, mock_egl_call)                   \
    TEST(EglMock, test_##test)                                         \
    {                                                                  \
        using namespace ::testing;                                     \
                                                                       \
    mtd::MockEGL mock;                                                 \
    EXPECT_CALL(mock, mock_egl_call).Times(Exactly(1));                \
    egl_call;                                                          \
    }                                                                  \

EGL_MOCK_TEST(eglGetError, eglGetError(), eglGetError())
EGL_MOCK_TEST(eglGetDisplay, eglGetDisplay(NULL), eglGetDisplay(NULL))
