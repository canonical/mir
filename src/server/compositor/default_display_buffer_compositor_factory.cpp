/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "default_display_buffer_compositor_factory.h"
#include "mir/renderer/renderer_factory.h"
#include "mir/renderer/renderer.h"
#include "mir/graphics/display_buffer.h"
#include "mir/renderer/gl/render_target.h"

#include "default_display_buffer_compositor.h"

#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::DefaultDisplayBufferCompositorFactory::DefaultDisplayBufferCompositorFactory(
    std::shared_ptr<mir::renderer::RendererFactory> const& renderer_factory,
    std::shared_ptr<mc::CompositorReport> const& report) :
    renderer_factory{renderer_factory},
    report{report}
{
}

std::unique_ptr<mc::DisplayBufferCompositor>
mc::DefaultDisplayBufferCompositorFactory::create_compositor_for(
    mg::DisplayBuffer& display_buffer)
{
    auto const render_target = dynamic_cast<renderer::gl::RenderTarget*>(display_buffer.native_display_buffer());
    if (!render_target)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("DisplayBuffer does not support GL rendering"));
    }
    auto renderer = renderer_factory->create_renderer_for(*render_target);
    renderer->set_viewport(display_buffer.view_area());
    return std::make_unique<DefaultDisplayBufferCompositor>(
         display_buffer, std::move(renderer), report);
}
