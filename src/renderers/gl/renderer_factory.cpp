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

#include "renderer_factory.h"
#include "renderer.h"
#include "mir/graphics/display_buffer.h"

namespace mrg = mir::renderer::gl;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

std::unique_ptr<mc::Renderer>
mrg::RendererFactory::create_renderer_for(
    graphics::DisplayBuffer& display_buffer)
{
    return std::make_unique<Renderer>(display_buffer);
}
