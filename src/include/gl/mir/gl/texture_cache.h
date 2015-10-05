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

#ifndef MIR_GL_TEXTURE_CACHE_H_
#define MIR_GL_TEXTURE_CACHE_H_

#include <memory>

namespace mir
{
namespace graphics { class Renderable; }
namespace gl
{
class Texture;
class TextureCache
{
public:
    virtual ~TextureCache() = default;

    /**
     * Loads texture from the renderable. Must be called with a current GL
     * context.
     *   \param [in] renderable
     *       The Renderable that needs to be used as a texture
     *   \returns
     *       The texture that represents the renderable.
     */
    virtual std::shared_ptr<Texture> load(graphics::Renderable const&) = 0;

    /**
     * Mark all entries in the cache as out-of-date to ensure fresh textures
     * are loaded next time. This function _must_ be implemented in a way that
     * does not require a GL context, as it will typically be called without
     * one.
     */
    virtual void invalidate() = 0;

    /**
     * Free textures that were not used (loaded) since the last
     * drop/invalidate. Must be called with a current GL context.
     */
    virtual void drop_unused() = 0;

protected:
    TextureCache() = default;
private:
    TextureCache(TextureCache const&) = delete;
    TextureCache& operator=(TextureCache const&) = delete;
};
}
}
#endif /* MIR_GL_TEXTURE_CACHE_H_ */
