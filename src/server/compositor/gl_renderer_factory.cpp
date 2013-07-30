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

#include "mir/compositor/gl_renderer_factory.h"
#include "mir/geometry/rectangle.h"
#include "gl_renderer.h"

namespace mc = mir::compositor;
namespace geom = mir::geometry;

std::unique_ptr<mc::Renderer>
mc::GLRendererFactory::create_renderer_for(geom::Rectangle const& rect)
{
    auto raw = new GLRenderer(rect);
    return std::unique_ptr<mc::Renderer>(raw);
}
