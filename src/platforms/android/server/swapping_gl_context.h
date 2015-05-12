/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_SWAPPING_GL_CONTEXT_H_
#define MIR_GRAPHICS_ANDROID_SWAPPING_GL_CONTEXT_H_

#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;
namespace android
{
class SwappingGLContext
{
public:
    virtual ~SwappingGLContext() = default;
    virtual void make_current() const = 0;
    virtual void release_current() const = 0;
    virtual void swap_buffers() const = 0;
    virtual std::shared_ptr<Buffer> last_rendered_buffer() const = 0;

protected:
    SwappingGLContext() = default;
    SwappingGLContext(SwappingGLContext const&) = delete;
    SwappingGLContext& operator=(SwappingGLContext const&) = delete;
};
}
}
}

#endif /* MIR_GRAPHICS_ANDROID_SWAPPING_GL_CONTEXT_H_ */
