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

#include "default_display_buffer_compositor.h"

#include <mir/compositor/compositor_report.h>
#include <mir/compositor/scene.h>
#include <mir/compositor/scene_element.h>
#include <mir/graphics/dmabuf_buffer.h>
#include <mir/graphics/renderable.h>
#include <mir/graphics/display_sink.h>
#include <mir/graphics/buffer.h>
#include <mir/graphics/output_filter.h>
#include <mir/graphics/platform.h>
#include <mir/graphics/rendering_providers.h>
#include <mir/compositor/buffer_stream.h>
#include <mir/renderer/renderer.h>
#include <mir/debug_frame_trace.h>
#include "occlusion.h"
#include <drm_fourcc.h>
#include <format>
#include <memory>
#include <mutex>

#define MIR_LOG_COMPONENT "compositor"
#include <mir/log.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
auto direct_presentation_allowed(MirOutputFilter filter, glm::mat2 const& transform) -> bool
{
    return filter == mir_output_filter_none && transform == glm::mat2{1};
}

auto import_display_elements(
    mg::RenderableList const& renderable_list,
    mg::RenderingProvider::FramebufferProvider& fb_adaptor) -> std::vector<mg::DisplayElement>
{
    std::vector<mg::DisplayElement> framebuffers;
    framebuffers.reserve(renderable_list.size());

    for (auto const& renderable : renderable_list)
    {
        auto fb = fb_adaptor.buffer_to_framebuffer(renderable->buffer());
        if (!fb)
        {
            break;
        }
        geom::Rectangle clipped_dest;
        if (renderable->clip_area())
        {
            clipped_dest = intersection_of(renderable->screen_position(), *renderable->clip_area());
        }
        else
        {
            clipped_dest = renderable->screen_position();
        }
        geom::SizeF const source_size{
            clipped_dest.size.width.as_value(),
            clipped_dest.size.height.as_value()};
        geom::PointF const source_origin{
            clipped_dest.top_left.x.as_value() - renderable->screen_position().top_left.x.as_value(),
            clipped_dest.top_left.y.as_value() - renderable->screen_position().top_left.y.as_value()
        };

        framebuffers.emplace_back(mg::DisplayElement{
            renderable->screen_position(),
            geom::RectangleF{source_origin, source_size},
            std::move(fb)
        });
    }

    return framebuffers;
}

bool try_bypass(
    mg::RenderableList const& renderable_list,
    mg::RenderingProvider::FramebufferProvider& fb_adaptor,
    mg::DisplaySink& display_sink,
    MirOutputFilter filter,
    glm::mat2 const& transform)
{
    auto const can_present_directly = direct_presentation_allowed(filter, transform);
    if (!can_present_directly)
        return false;

    auto const display_elements =  import_display_elements(renderable_list, fb_adaptor);

    if (display_elements.size() != renderable_list.size())
        return false;

    return display_sink.overlay(display_elements);
}
}


mc::DefaultDisplayBufferCompositor::DefaultDisplayBufferCompositor(
    mg::DisplaySink& display_sink,
    graphics::GLRenderingProvider& gl_provider,
    std::shared_ptr<mir::renderer::Renderer> const& renderer,
    std::shared_ptr<mir::graphics::OutputFilter> const& output_filter,
    std::shared_ptr<CompositorReport> const& report) :
    display_sink(display_sink),
    renderer(renderer),
    output_filter(output_filter),
    fb_adaptor{gl_provider.make_framebuffer_provider(display_sink)},
    report(report)
{
}

bool mc::DefaultDisplayBufferCompositor::composite(mc::SceneElementSequence&& scene_elements)
{
    if (scene_elements.size() == 0 && !completed_first_render)
        return false;

    /* THROWAWAY (#5325): `global`/`global-finish` serialise all compositing (and screen
     * capture), approximating a single-threaded renderer.
     */
    auto const sync_mode = mir::debug::buffer_sync_mode();
    auto const serialise_globally =
        sync_mode == mir::debug::BufferSyncMode::global ||
        sync_mode == mir::debug::BufferSyncMode::global_finish;
    std::unique_lock<std::mutex> global_lock;
    if (serialise_globally)
        global_lock = std::unique_lock{mir::debug::global_render_mutex()};

    auto const& view_area_for_key = display_sink.view_area();
    auto const trace_key = mir::debug::output_key(
        view_area_for_key.size.width.as_int(),
        view_area_for_key.size.height.as_int(),
        view_area_for_key.top_left.x.as_int(),
        view_area_for_key.top_left.y.as_int());

    completed_first_render = true;
    report->began_frame(this);

    auto const& view_area = display_sink.view_area();
    auto [occluded_elements, visible_elements] = mc::split_occluded_and_visible(std::move(scene_elements), view_area);

    for (auto const& element : occluded_elements)
        element->occluded();

    mg::RenderableList renderable_list;
    renderable_list.reserve(visible_elements.size());
    for (auto const& element : visible_elements)
    {
        element->rendered();
        renderable_list.push_back(element->renderable());
    }

    /*
     * Note: Buffer lifetimes are ensured by the two objects holding
     *       references to them; visible_elements and renderable_list.
     *       So no buffer is going to be released back to the client till
     *       both of those containers get destroyed (end of the function).
     *       Actually, there's a third reference held by the texture cache
     *       in GLRenderer, but that gets released earlier in render().
     */
    visible_elements.clear();  // Those in use are still in renderable_list

    auto const filter = output_filter->filter();
    auto const transform = display_sink.transformation();

    if (mir::debug::tracing())
    {
        std::string renderables;
        for (auto const& renderable : renderable_list)
        {
            auto const buffer = renderable->buffer();
            auto const buffer_id = buffer->id().as_value();
            mir::debug::note_buffer_sampled(buffer_id, trace_key);

            auto const modifier =
                [&]() -> uint64_t
                {
                    if (auto const dmabuf = dynamic_cast<mg::DMABufBuffer const*>(buffer->native_buffer_base()))
                        return dmabuf->modifier().value_or(DRM_FORMAT_MOD_INVALID);
                    return DRM_FORMAT_MOD_INVALID;
                }();

            renderables += std::format(
                " [buf={} surface={} pos={}x{}+{}+{} alpha={:.2f} modifier=0x{:016x}]",
                buffer_id, renderable->id(),
                renderable->screen_position().size.width.as_int(),
                renderable->screen_position().size.height.as_int(),
                renderable->screen_position().top_left.x.as_int(),
                renderable->screen_position().top_left.y.as_int(),
                renderable->alpha(), modifier);
        }

        mir::debug::trace(
            "composite begin output={} comp={} elements={}{}",
            trace_key, static_cast<void const*>(this), renderable_list.size(), renderables);

        auto const last_buffer_id =
            renderable_list.empty() ? 0u : renderable_list.front()->buffer()->id().as_value();
        mir::debug::update_output_trace(
            trace_key,
            [&](mir::debug::OutputTrace& trace)
            {
                ++trace.composites;
                trace.last_composite = mir::debug::now_seconds();
                trace.last_buffer_id = last_buffer_id;
                trace.last_element_count = renderable_list.size();
            });
    }

    if (try_bypass(renderable_list, *fb_adaptor, display_sink, filter, transform))
    {
        report->renderables_in_frame(this, renderable_list);
        renderer->suspend();

        if (mir::debug::tracing())
        {
            mir::debug::trace("composite end   output={} bypassed", trace_key);
            mir::debug::update_output_trace(
                trace_key, [](mir::debug::OutputTrace& trace) { trace.last_bypassed = true; });
        }
    }
    else
    {
        renderer->set_output_transform(transform);
        renderer->set_viewport(view_area);
        renderer->set_output_filter(filter);

        display_sink.set_next_image(renderer->render(renderable_list));

        if (sync_mode == mir::debug::BufferSyncMode::global_finish)
            mir::debug::gl_finish();

        if (mir::debug::tracing())
        {
            mir::debug::trace("composite end   output={} rendered", trace_key);
            mir::debug::update_output_trace(
                trace_key, [](mir::debug::OutputTrace& trace) { trace.last_bypassed = false; });
        }

        report->renderables_in_frame(this, renderable_list);
        report->rendered_frame(this);

        /*
         * This is used for the 'early release' optimization to release buffers
         * we did use back to clients before starting on the potentially slow
         * post() call.
         * FIXME: This clear() call is blocking a little because we drive IPC
         *        here (LP: #1395421). However if the early release
         *        optimization is disabled or absent (LP: #1561418) then this
         *        clear() doesn't contribute anything. In that case the
         *        problematic IPC (LP: #1395421) will instead occur in buffer
         *        acquisition calls when we composite the next frame.
         */
        renderable_list.clear();
    }

    report->finished_frame(this);
    return true;
}
