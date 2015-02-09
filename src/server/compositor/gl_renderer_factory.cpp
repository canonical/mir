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
#include "mir/compositor/recently_used_cache.h"
#include "mir/compositor/gl_renderer.h"
#include "mir/graphics/gl_program.h"
#include "mir/geometry/rectangle.h"

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

std::unique_ptr<mc::Renderer>
mc::GLRendererFactory::create_renderer_for(geom::Rectangle const& rect,
        DestinationAlpha dest_alpha)
{
    auto raw = new GLRenderer(
        std::unique_ptr<mg::GLTextureCache>(new RecentlyUsedCache()),
        rect, dest_alpha);
    return std::unique_ptr<mc::Renderer>(raw);
}
