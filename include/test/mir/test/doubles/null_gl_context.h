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

#ifndef MIR_TEST_DOUBLES_NULL_GL_CONTEXT_H_
#define MIR_TEST_DOUBLES_NULL_GL_CONTEXT_H_

#include <mir/renderer/gl/context.h>

namespace mir
{
namespace test
{
namespace doubles
{

class NullGLContext : public renderer::gl::Context
{
public:
    void make_current() const override {}
    void release_current() const override {}
    auto make_share_context() const -> std::unique_ptr<Context> override { return {}; }
    explicit operator EGLContext() override { return EGL_NO_DISPLAY; }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_NULL_GL_CONTEXT_H_ */
