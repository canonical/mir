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

#include "mir_test/egl_mock.h"
#include "gtest/gtest.h"

TEST(EglMock, demo)
{
    using namespace ::testing;

    mir::EglMock mock;
    EXPECT_CALL(mock, eglGetError()).Times(Exactly(1));
    eglGetError();
}

#define EGL_MOCK_TEST(test, egl_call, egl_mock_call)                   \
    TEST(EglMock, test_##test)                                         \
    {                                                                  \
        using namespace ::testing;                                     \
                                                                       \
    mir::EglMock mock;                                                 \
    EXPECT_CALL(mock, egl_mock_call).Times(Exactly(1));                \
    egl_call;                                                          \
    }                                                                  \

EGL_MOCK_TEST(eglGetError, eglGetError(), eglGetError())
EGL_MOCK_TEST(eglGetDisplay, eglGetDisplay(NULL), eglGetDisplay(NULL))
