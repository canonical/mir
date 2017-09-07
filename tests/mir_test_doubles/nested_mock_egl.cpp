/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/test/doubles/nested_mock_egl.h"

namespace mtd = mir::test::doubles;

using namespace ::testing;

mtd::NestedMockEGL::NestedMockEGL()
{
    {
        InSequence init_before_terminate;
        EXPECT_CALL(*this, eglGetDisplay(_)).Times(1);
        EXPECT_CALL(*this, eglTerminate(_)).Times(1);
    }

    provide_egl_extensions();
    provide_stub_platform_buffer_swapping();


    ON_CALL(*this, eglChooseConfig(_, _, _, _, _)).WillByDefault(
        DoAll(WithArgs<2, 4>(Invoke(this, &NestedMockEGL::egl_choose_config)), Return(EGL_TRUE)));

    {
        InSequence context_lifecycle;
        EXPECT_CALL(*this, eglCreateContext(_, _, _, _)).Times(AnyNumber()).WillRepeatedly(Return((EGLContext)this));
        EXPECT_CALL(*this, eglDestroyContext(_, _)).Times(AnyNumber()).WillRepeatedly(Return(EGL_TRUE));
    }
}

void mtd::NestedMockEGL::egl_initialize(EGLint* major, EGLint* minor)
{
    *major = 1;
    *minor = 4;
}

void mtd::NestedMockEGL::egl_choose_config(EGLConfig* config, EGLint*  num_config)
{
    *config = this;
    *num_config = 1;
}
