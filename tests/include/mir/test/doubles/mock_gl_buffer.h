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

#ifndef MIR_TEST_DOUBLES_MOCK_GL_BUFFER_H_
#define MIR_TEST_DOUBLES_MOCK_GL_BUFFER_H_

#include "mock_buffer.h"
#include "mir/graphics/texture.h"
#include "mir/graphics/program.h"
#include "mir/graphics/program_factory.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct MockTextureBuffer : public MockBuffer,
                           public graphics::gl::Texture
{
public:
    using MockBuffer::MockBuffer;

    MOCK_METHOD(graphics::gl::Program const&, shader, (graphics::gl::ProgramFactory&), (const override));
    MOCK_METHOD(Layout, layout, (), (const override));
    MOCK_METHOD(void, bind, (), (override));
    MOCK_METHOD(void, add_syncpoint, (), (override));
    MOCK_METHOD(GLuint, tex_id, (), (const override));
};
}
}
}

#endif
