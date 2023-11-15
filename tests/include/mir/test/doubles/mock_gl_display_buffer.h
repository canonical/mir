/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_GL_DISPLAY_BUFFER_H_
#define MIR_TEST_DOUBLES_MOCK_GL_DISPLAY_BUFFER_H_

#include "mock_display_sink.h"
#include "mir/renderer/gl/render_target.h"

namespace mir
{
namespace test
{
namespace doubles
{

class MockGLDisplayBuffer : public MockDisplayBuffer,
                            public renderer::gl::RenderTarget
{
public:
    MOCK_METHOD(geometry::Size, size, (), (const, override));
    MOCK_METHOD(void, make_current, (), (override));
    MOCK_METHOD(void, release_current, (), (override));
    MOCK_METHOD(void, swap_buffers, (), (override));
    MOCK_METHOD(void, bind, (), (override));
};

}
}
}

#endif
