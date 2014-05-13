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

#include "mir/graphics/renderable.h"

namespace mir
{
namespace graphics
{
class TextureCache
{
public:
    virtual ~TextureCache() = default;

    /** Loads texture from the renderable and ensures that it is bound.
     * \param [in] renderable
     *     The Renderable that will be bound as the active texture of the GL context
    **/ 
    virtual void load_texture(Renderable const&) = 0;

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
