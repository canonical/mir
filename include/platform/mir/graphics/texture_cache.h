/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_TEXTURE_CACHE_H_
#define MIR_GRAPHICS_TEXTURE_CACHE_H_

#include "mir/graphics/gl_program.h"
#include "mir/graphics/renderable.h"
#include <memory>

namespace mir
{
namespace graphics
{

class Texture
{
public:
    //If you need to tweak the TexParameters, best to do it within the class
    //so the object has a cogent idea of what the texture it represents is doing
    Texture() :
        id(generate_id())
    {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void gl_bind() const
    {
        glBindTexture(GL_TEXTURE_2D, id);
    }

    ~Texture()
    {
        glDeleteTextures(1, &id);
    }

private:
    GLuint generate_id()
    {
        GLuint id;
        glGenTextures(1, &id);
        return id;
    }
    GLuint const id;
};


class TextureCache
{
public:
    virtual ~TextureCache() = default;

    /** Loads texture and ensures that it is bound.
     * \param [in] renderable
     *     The Renderable that will be bound as the active texture of the GL context
     * \param [in] force_upload
     *     Force an upload of the pixels. TODO: remove this parameter
    **/ 
    virtual void bind_texture_from(Renderable const&) = 0;

    virtual void invalidate() = 0;

    /** Release the resources associated with the bound textures in the cache **/
    virtual void release_live_texture_resources() = 0;

protected:
    TextureCache() = default;

private:
    TextureCache(TextureCache const&) = delete;
    TextureCache& operator=(TextureCache const&) = delete;

};

}
}

#endif /* MIR_GRAPHICS_TEXTURE_CACHE_H_ */
