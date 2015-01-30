/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_SWAPPING_GL_CONTEXT_H_
#define MIR_TEST_DOUBLES_STUB_SWAPPING_GL_CONTEXT_H_

#include "src/platforms/android/server/gl_context.h"
#include "stub_buffer.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubSwappingGLContext : public graphics::android::SwappingGLContext
{
    StubSwappingGLContext(std::shared_ptr<graphics::Buffer> const& buffer) :
        buffer(buffer)
    {
    }

    StubSwappingGLContext() :
        StubSwappingGLContext(std::make_shared<StubBuffer>())
    {
    }
    void make_current() const {}
    void release_current() const {}
    void swap_buffers() const {}
    std::shared_ptr<graphics::Buffer> last_rendered_buffer() const
    {
        return buffer;
    }
private:
    std::shared_ptr<graphics::Buffer> const buffer;
};

}
}
}
#endif // MIR_TEST_DOUBLES_STUB_COMPOSITING_CRITERIA_H_
