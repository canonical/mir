/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_RENDERER_GL_RENDER_TARGET_H_
#define MIR_RENDERER_GL_RENDER_TARGET_H_

namespace mir
{
namespace renderer
{
namespace gl
{

class RenderTarget
{
public:
    virtual ~RenderTarget() = default;

    /** Makes the the current GL render target. */
    virtual void make_current() = 0;
    /** Releases the current GL render target. */
    virtual void release_current() = 0;
    /**
     * Swap buffers for OpenGL rendering.
     * After this method returns is the earliest time that it is safe to
     * free GL-related resources such as textures and buffers.
     */
    virtual void swap_buffers() = 0;

protected:
    RenderTarget() = default;
    RenderTarget(RenderTarget const&) = delete;
    RenderTarget& operator=(RenderTarget const&) = delete;
};

}
}
}

#endif
