/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_GL_BUFFER_H_
#define MIR_TEST_DOUBLES_MOCK_GL_BUFFER_H_

#include "mock_buffer.h"
#include "mir/renderer/gl/texture_bindable.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct MockGLBuffer : public MockBuffer,
                      public renderer::gl::TextureBindable
{
 public:
    using MockBuffer::MockBuffer;

    MOCK_METHOD0(gl_bind_to_texture, void());
};

}
}
}

#endif
