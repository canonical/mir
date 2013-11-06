/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_server_configuration.h"
#include "buffer_stream_factory.h"
#include "default_display_buffer_compositor_factory.h"
#include "multi_threaded_compositor.h"
#include "overlay_renderer.h"
#include "gl_renderer_factory.h"

namespace mc = mir::compositor;
namespace ms = mir::surfaces;

std::shared_ptr<ms::BufferStreamFactory>
mir::DefaultServerConfiguration::the_buffer_stream_factory()
{
    return buffer_stream_factory(
        [this]()
        {
            return std::make_shared<mc::BufferStreamFactory>(the_buffer_allocator());
        });
}

std::shared_ptr<mc::DisplayBufferCompositorFactory>
mir::DefaultServerConfiguration::the_display_buffer_compositor_factory()
{
    return display_buffer_compositor_factory(
        [this]()
        {
            return std::make_shared<mc::DefaultDisplayBufferCompositorFactory>(
                the_scene(), the_renderer_factory(), the_overlay_renderer());
        });
}

std::shared_ptr<mc::Compositor>
mir::DefaultServerConfiguration::the_compositor()
{
    return compositor(
        [this]()
        {
            return std::make_shared<mc::MultiThreadedCompositor>(the_display(),
                                                                 the_scene(),
                                                                 the_display_buffer_compositor_factory());
        });
}

std::shared_ptr<mc::OverlayRenderer>
mir::DefaultServerConfiguration::the_overlay_renderer()
{
    struct NullOverlayRenderer : public mc::OverlayRenderer
    {
        virtual void render(
            geometry::Rectangle const&,
            std::function<void(std::shared_ptr<void> const&)>) {}
    };
    return overlay_renderer(
        [this]()
        {
            return std::make_shared<NullOverlayRenderer>();
        });
}

std::shared_ptr<mc::RendererFactory> mir::DefaultServerConfiguration::the_renderer_factory()
{
    return renderer_factory(
        []()
        {
            return std::make_shared<mc::GLRendererFactory>();
        });
}
