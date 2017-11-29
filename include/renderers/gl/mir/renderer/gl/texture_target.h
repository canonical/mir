/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

#ifndef MIR_RENDERER_GL_TEXTURE_TARGET_H_
#define MIR_RENDERER_GL_TEXTURE_TARGET_H_

namespace mir
{
namespace renderer
{
namespace gl
{

class TextureTarget
{
public:
    virtual ~TextureTarget() = default;

    /* Bind the target as a texture suitable for rendering to via glFramebufferTexture2D */
    virtual void bind_for_write() = 0;

    /// Memory backed targets need to secure the pixels after rendering
    virtual void secure_pixels() = 0;

protected:
    TextureTarget() = default;
    TextureTarget(TextureTarget const&) = delete;
    TextureTarget& operator=(TextureTarget const&) = delete;
};

}
}
}

#endif /* MIR_RENDERER_GL_TEXTURE_TARGET_H_ */
