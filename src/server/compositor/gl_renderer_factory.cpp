/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "gl_renderer_factory.h"
#include "renderable_lru_cache.h"
#include "mir/compositor/gl_renderer.h"
#include "mir/graphics/gl_program.h"
#include "mir/graphics/texture_cache.h"
#include "mir/graphics/buffer.h"
#include "mir/geometry/rectangle.h"

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

void mc::RenderableLRUCache::bind_texture_from(mg::Renderable const& renderable)
{
    auto const& buffer = renderable.buffer();
    auto const& id = renderable.id();

    auto buffer_id = buffer->id();
    auto& texture = textures[id];
    texture.texture.gl_bind();

    if (texture.last_bound_buffer != buffer_id)
        buffer->bind_to_texture();

    texture.resource = buffer;
    texture.last_bound_buffer = buffer_id;
    texture.used = true;
}

void mc::RenderableLRUCache::invalidate()
{
    mg::BufferID invalid_id;
    for(auto &t : textures)
        t.second.last_bound_buffer = invalid_id;
}

void mc::RenderableLRUCache::release_live_texture_resources()
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

mc::GLRendererFactory::GLRendererFactory(std::shared_ptr<mg::GLProgramFactory> const& factory) :
    program_factory(factory)
{
}

std::unique_ptr<mc::Renderer>
mc::GLRendererFactory::create_renderer_for(geom::Rectangle const& rect)
{
    auto raw = new GLRenderer(*program_factory, std::unique_ptr<mg::TextureCache>(new RenderableLRUCache()), rect);
    return std::unique_ptr<mc::Renderer>(raw);
}
