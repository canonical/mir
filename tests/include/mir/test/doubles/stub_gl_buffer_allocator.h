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

#ifndef MIR_TEST_DOUBLES_STUB_GL_BUFFER_ALLOCATOR_H_
#define MIR_TEST_DOUBLES_STUB_GL_BUFFER_ALLOCATOR_H_

#include "stub_buffer_allocator.h"
#include "stub_gl_buffer.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubGLBufferAllocator : public StubBufferAllocator
{
    std::shared_ptr<graphics::Buffer> alloc_buffer(
        graphics::BufferProperties const& properties) override
    {
        return std::make_shared<StubGLBuffer>(properties);
    }
};

}
}
}

#endif
