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
#include <mir/renderer/blitter_renderer_factory.h>
#include <mir/renderer/renderer_factory.h>
#include <mir/renderer/renderer.h>
#include <mir/graphics/display_sink.h>
#include <mir/renderer/gl/render_target.h>
#include <mir/graphics/platform.h>
#include <mir/graphics/rendering_providers.h>
#include <mir/renderer/gl/gl_surface.h>
#include <mir/graphics/gl_config.h>

#include "default_display_buffer_compositor.h"

#define MIR_LOG_COMPONENT "compositor"
#include <mir/log.h>

#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::DefaultDisplayBufferCompositorFactory::DefaultDisplayBufferCompositorFactory(
    std::vector<std::shared_ptr<mg::GLRenderingProvider>> render_platforms,
    std::vector<std::shared_ptr<mg::BlitterRenderingProvider>> blitter_platforms,
    std::shared_ptr<mg::GLConfig> gl_config,
    std::shared_ptr<mir::renderer::RendererFactory> const& renderer_factory,
    std::shared_ptr<mir::renderer::BlitterRendererFactory> const& blitter_renderer_factory,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<mc::CompositorReport> const& report,
    std::shared_ptr<mg::OutputFilter> const& output_filter) :
        platforms{std::move(render_platforms)},
        blitter_platforms{std::move(blitter_platforms)},
        gl_config{std::move(gl_config)},
        renderer_factory{renderer_factory},
        blitter_renderer_factory{blitter_renderer_factory},
        buffer_allocator{buffer_allocator},
        report{report},
        output_filter{output_filter}
{
}

/**
 * Build a compositor that renders with a blitter engine, if one can drive this output.
 *
 * A blitter renders directly into the display's own buffers, so this needs both
 * a suitable BlitterRenderingProvider and a display that hands out
 * CPU-addressable framebuffers.
 *
 * \return nullptr if this output should be composited with GL instead
 */
auto mc::DefaultDisplayBufferCompositorFactory::create_blitter_compositor_for(
    mg::DisplaySink& display_sink) -> std::unique_ptr<mc::DisplayBufferCompositor>
{
    auto best_provider = std::make_pair(mg::probe::unsupported, std::shared_ptr<mg::BlitterRenderingProvider>{});
    for (auto const& provider : blitter_platforms)
    {
        auto const suitability = provider->suitability_for_display(display_sink);
        // The blitter also has to be able to read the buffers clients give us...
        if (provider->suitability_for_allocator(buffer_allocator) > mg::probe::unsupported &&
            suitability > best_provider.first)
        {
            best_provider = std::make_pair(suitability, provider);
        }
    }

    if (best_provider.first == mg::probe::unsupported)
        return nullptr;

    auto* const allocator = display_sink.acquire_compatible_allocator<mg::CPUAddressableDisplayAllocator>();
    if (!allocator)
        return nullptr;

    auto const chosen_provider = best_provider.second;
    try
    {
        auto renderer = blitter_renderer_factory->create_renderer_for(chosen_provider, *allocator, gl_config);
        renderer->set_viewport(display_sink.view_area());
        return std::make_unique<DefaultDisplayBufferCompositor>(
            display_sink, *chosen_provider, std::move(renderer), output_filter, report);
    }
    catch (std::exception const& err)
    {
        mir::log_warning("Failed to set up blitter compositing (%s); falling back to GL", err.what());
        return nullptr;
    }
}

std::unique_ptr<mc::DisplayBufferCompositor>
mc::DefaultDisplayBufferCompositorFactory::create_compositor_for(
    mg::DisplaySink& display_sink)
{
    if (auto compositor = create_blitter_compositor_for(display_sink))
        return compositor;

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
        display_sink, *gl_config);
    auto renderer = renderer_factory->create_renderer_for(std::move(output_surface), chosen_allocator);
    renderer->set_viewport(display_sink.view_area());
    return std::make_unique<DefaultDisplayBufferCompositor>(
        display_sink, *chosen_allocator, std::move(renderer), output_filter, report);
}
