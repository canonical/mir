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

#ifndef MIR_TEST_DOUBLES_STUB_GL_DISPLAY_BUFFER_H_
#define MIR_TEST_DOUBLES_STUB_GL_DISPLAY_BUFFER_H_

#include "mir/test/doubles/stub_display_buffer.h"
#include "mir/renderer/gl/render_target.h"

namespace mir
{
namespace test
{
namespace doubles
{

class StubGLDisplayBuffer : public StubDisplaySink,
                            public renderer::gl::RenderTarget
{
public:
    using StubDisplaySink::StubDisplaySink;
    StubGLDisplayBuffer(StubGLDisplayBuffer const& s) : StubDisplaySink(s) {}

    auto size() const -> geometry::Size override { return {}; }
    void make_current() override {}
    void release_current() override {}
    void swap_buffers() override {}
    void bind() override {}
};

}
}
}

#endif
