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
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test/gl_mock.h"
#include <gtest/gtest.h>

namespace
{
mir::GLMock* global_gl_mock = NULL;
}

mir::GLMock::GLMock()
{
    using namespace testing;
    assert(global_gl_mock == NULL && "Only one mock object per process is allowed");

    global_gl_mock = this;

}

mir::GLMock::~GLMock()
{
    global_gl_mock = NULL;
}

#define CHECK_GLOBAL_VOID_MOCK()            \
    if (!global_gl_mock)                    \
    {                                       \
        using namespace ::testing;          \
        ADD_FAILURE_AT(__FILE__,__LINE__);  \
        return;                             \
    }

#define CHECK_GLOBAL_MOCK(rettype)          \
    if (!global_gl_mock)                    \
    {                                       \
        using namespace ::testing;          \
        ADD_FAILURE_AT(__FILE__,__LINE__);  \
        rettype type = (rettype) 0;         \
        return type;                        \
    }

void glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_gl_mock->glEGLImageTargetTexture2DOES(target, image);
}

const GLubyte* glGetString(GLenum name)
{
    CHECK_GLOBAL_MOCK(const GLubyte*);
    return global_gl_mock->glGetString(name);
}

void mir::GLMock::silence_uninteresting()
{
    using namespace testing;
    EXPECT_CALL(*this, glEGLImageTargetTexture2DOES(_,_))
        .Times(AtLeast(0));
}
