/*
 * Copyright © 2013-2020 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_server_configuration.h"

#include "mir/shell/shell.h"
#include "buffer_stream_factory.h"
#include "default_display_buffer_compositor_factory.h"
#include "multi_threaded_compositor.h"
#include "gl/renderer_factory.h"
#include "mir/main_loop.h"

#include "mir/options/configuration.h"

#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace mf = mir::frontend;

std::shared_ptr<ms::BufferStreamFactory>
mir::DefaultServerConfiguration::the_buffer_stream_factory()
{
    return buffer_stream_factory(
        []()
        {
            return std::make_shared<mc::BufferStreamFactory>();
        });
}

std::shared_ptr<mc::DisplayBufferCompositorFactory>
mir::DefaultServerConfiguration::the_display_buffer_compositor_factory()
{
    return display_buffer_compositor_factory(
        [this]()
        {
            return wrap_display_buffer_compositor_factory(std::make_shared<mc::DefaultDisplayBufferCompositorFactory>(
                the_renderer_factory(), the_compositor_report()));
        });
}

std::shared_ptr<mc::DisplayBufferCompositorFactory>
mir::DefaultServerConfiguration::wrap_display_buffer_compositor_factory(
    std::shared_ptr<mc::DisplayBufferCompositorFactory> const& wrapped)
{
    return wrapped;
}

std::shared_ptr<mc::Compositor>
mir::DefaultServerConfiguration::the_compositor()
{
    return compositor(
        [this]()
        {
            std::chrono::milliseconds const composite_delay(
                the_options()->get<int>(options::composite_delay_opt));

            return std::make_shared<mc::MultiThreadedCompositor>(
                the_display(),
                the_scene(),
                the_display_buffer_compositor_factory(),
                the_shell(),
                the_compositor_report(),
                composite_delay,
                true);
        });
}

std::shared_ptr<mir::renderer::RendererFactory> mir::DefaultServerConfiguration::the_renderer_factory()
{
    return renderer_factory(
        []()
        {
            return std::make_shared<mir::renderer::gl::RendererFactory>();
        });
}
