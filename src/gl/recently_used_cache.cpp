/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 *              Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "recently_used_cache.h"
#include "mir/graphics/buffer.h"
#include "mir/renderer/gl/texture_source.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgl = mir::gl;
namespace geom = mir::geometry;

std::shared_ptr<mgl::Texture> mgl::RecentlyUsedCache::load(mg::Renderable const& renderable)
{
    auto const& buffer = renderable.buffer();
    auto buffer_id = buffer->id();
    auto& texture = textures[renderable.id()];
    texture.texture->bind();

    if ((texture.last_bound_buffer != buffer_id) || (!texture.valid_binding))
    {
        auto const texture_source =
            dynamic_cast<mir::renderer::gl::TextureSource*>(
                    buffer->native_buffer_base());
        if (!texture_source)
            BOOST_THROW_EXCEPTION(std::logic_error("Buffer does not support GL rendering"));

        texture_source->gl_bind_to_texture();
        texture.resource = buffer;
        texture.last_bound_buffer = buffer_id;
    }
    texture.valid_binding = true;
    texture.used = true;

    return texture.texture;
}

void mgl::RecentlyUsedCache::invalidate()
{
    for (auto &t : textures)
        t.second.valid_binding = false;
}

void mgl::RecentlyUsedCache::drop_unused()
{
    auto t = textures.begin();
    while (t != textures.end())
    {
        auto& tex = t->second;
        tex.resource.reset();
        if (tex.used)
        {
            tex.used = false;
            ++t;
        }
        else
        {
            t = textures.erase(t);
        }
    }
}
