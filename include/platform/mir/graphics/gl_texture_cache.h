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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 *              Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_GL_TEXTURE_CACHE_H_
#define MIR_GRAPHICS_GL_TEXTURE_CACHE_H_

#include "mir/graphics/renderable.h"

namespace mir
{
namespace graphics
{
class GLTexture;
class GLTextureCache
{
public:
    virtual ~GLTextureCache() = default;

    /** Loads texture from the renderable. Should be called with a current gl context
     * \param [in] renderable
     *     The Renderable that needs to be used as a texture
     * \returns 
     *     The texture that represents the renderable.
    **/ 
    virtual std::shared_ptr<GLTexture> load(Renderable const&) = 0;

    /** mark the entries in the cache as having invalid bindings **/ 
    virtual void invalidate() = 0;

    /** cache selects textures to release. Should be called with a current gl context **/
    virtual void drop_unused() = 0;

protected:
    GLTextureCache() = default;
private:
    GLTextureCache(GLTextureCache const&) = delete;
    GLTextureCache& operator=(GLTextureCache const&) = delete;
};
}
}
#endif /* MIR_GRAPHICS_GL_TEXTURE_CACHE_H_ */
