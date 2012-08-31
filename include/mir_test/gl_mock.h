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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_MIR_TEST_GL_MOCK_H_
#define MIR_MIR_TEST_GL_MOCK_H_

#include <gmock/gmock.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace mir
{
class GLMock
{
public:
    GLMock();
    ~GLMock();

    MOCK_METHOD2(glEGLImageTargetTexture2DOES, void(GLenum, GLeglImageOES));

};
}

#endif /* MIR_MIR_TEST_GL_MOCK_H_ */
