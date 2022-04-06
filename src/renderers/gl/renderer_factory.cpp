/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include <boost/throw_exception.hpp>

namespace mrg = mir::renderer::gl;

std::unique_ptr<mir::renderer::Renderer>
mrg::RendererFactory::create_renderer_for(
    graphics::DisplayBuffer& display_buffer)
{
    auto const render_target = dynamic_cast<renderer::gl::RenderTarget*>(display_buffer.native_display_buffer());
    if (!render_target)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("DisplayBuffer does not support GL rendering"));
    }
    return std::make_unique<Renderer>(*render_target, display_buffer.view_area());
}
