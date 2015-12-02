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

#ifndef MIR_RENDERER_GL_TEXTURE_SOURCE_H_
#define MIR_RENDERER_GL_TEXTURE_SOURCE_H_

namespace mir
{
namespace renderer
{
namespace gl
{

//FIXME: (kdub) we're not hiding the differences in texture upload approaches between our platforms
//       very well with this interface.
class TextureSource
{
public:
    virtual ~TextureSource() = default;

    // \warning deprecated, provided for convenience and legacy transition.
    //will call bind() and then secure_for_render()
    virtual void gl_bind_to_texture() = 0;
    //Uploads texture.
    virtual void bind() = 0;
    //add synchronization points to the command stream to ensure resources
    //are present during the draw. Will not upload texture.
    //should be called if an already uploaded texture is reused.
    virtual void secure_for_render() = 0;

protected:
    TextureSource() = default;
    TextureSource(TextureSource const&) = delete;
    TextureSource& operator=(TextureSource const&) = delete;
};

}
}
}

#endif
