/*
 * Copyright © Canonical Ltd.
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
 */

#include "default_display_buffer_compositor_factory.h"
#include "mir/renderer/renderer_factory.h"
#include "mir/renderer/renderer.h"
#include "mir/graphics/display_buffer.h"
#include "mir/renderer/gl/render_target.h"
#include "mir/graphics/platform.h"
#include "mir/renderer/gl/gl_surface.h"
#include "mir/graphics/gl_config.h"

#include "default_display_buffer_compositor.h"

#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::DefaultDisplayBufferCompositorFactory::DefaultDisplayBufferCompositorFactory(
    std::vector<std::shared_ptr<mg::GLRenderingProvider>> render_platforms,
    std::shared_ptr<mg::GLConfig> gl_config,
    std::shared_ptr<mir::renderer::RendererFactory> const& renderer_factory,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<mc::CompositorReport> const& report) :
        platforms{std::move(render_platforms)},
        gl_config{std::move(gl_config)},
        renderer_factory{renderer_factory},
        buffer_allocator{buffer_allocator},
        report{report}
{
}

std::unique_ptr<mc::DisplayBufferCompositor>
mc::DefaultDisplayBufferCompositorFactory::create_compositor_for(
    mg::DisplaySink& display_sink)
{
    /* TODO: There's scope for (GPU) memory optimisation here:
     * We unconditionally allocate a GL rendering surface for the renderer,
     * but with a different interface the DisplayBufferCompositor could choose
     * not to allocate a GL surface if everything is working with overlays.
     *
     * For simple cases, such as those targetted by Ubuntu Frame, not needing the
     * GL surface could be the common case, and not allocating it would save a
     * potentially-significant amount of GPU memory.
     */

    /* In a heterogeneous system, different providers may be better at driving a specific
     * display. Select the best one.
     */
    std::pair<mg::probe::Result, std::shared_ptr<mg::GLRenderingProvider>> best_provider = std::make_pair(mg::probe::unsupported, nullptr);
    for (auto const& provider : platforms)
    {
        auto suitability = provider->suitability_for_display(display_sink);
        // We also need to make sure that the GLRenderingProvider can access client buffers...
        if (provider->suitability_for_allocator(buffer_allocator) > mg::probe::unsupported && suitability > best_provider.first)
        {
            best_provider = std::make_pair(suitability, provider);
        }
    }
    if (best_provider.first == mg::probe::unsupported)
    {
        // We should not get here; the rendering platforms have already had
        // an opportunity to claim they don't support any present hardware
        BOOST_THROW_EXCEPTION((std::logic_error{"No rendering platform claims to support this output"}));
    }

    auto const chosen_allocator = best_provider.second;
    
    auto output_surface = chosen_allocator->surface_for_sink(
        display_sink, display_sink.pixel_size(), *gl_config);
    auto renderer = renderer_factory->create_renderer_for(std::move(output_surface), chosen_allocator);
    renderer->set_viewport(display_sink.view_area());
    return std::make_unique<DefaultDisplayBufferCompositor>(
        display_sink, *chosen_allocator, std::move(renderer), report);
}
