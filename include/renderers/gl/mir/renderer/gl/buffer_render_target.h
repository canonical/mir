/*
 * Copyright © 2022 Canonical Ltd.
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

#ifndef MIR_RENDERER_GL_BUFFER_RENDER_TARGET_H_
#define MIR_RENDERER_GL_BUFFER_RENDER_TARGET_H_

#include "render_target.h"
#include "mir/geometry/size.h"

#include <memory>

namespace mir
{
namespace renderer
{
namespace software
{
class WriteMappableBuffer;
}
namespace gl
{

/// Not threadsafe, do not use concurrently
class BufferRenderTarget: public RenderTarget
{
public:
    virtual void set_buffer(std::shared_ptr<software::WriteMappableBuffer> const& buffer) = 0;
};

}
}
}

#endif // MIR_RENDERER_GL_BUFFER_RENDER_TARGET_H_
