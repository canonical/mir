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

#include "mir/compositor/compositor_report.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/dmabuf_buffer.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/display_sink.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/output_filter.h"
#include "mir/graphics/platform.h"
#include "mir/compositor/buffer_stream.h"
#include "mir/renderer/renderer.h"
#include "occlusion.h"
#include <memory>

#define MIR_LOG_COMPONENT "compositor"
#include "mir/log.h"

namespace mc = mir::compositor;
namespace mg = mir::graphics;


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

    completed_first_render = true;
    report->began_frame(this);

    auto const& view_area = display_sink.view_area();
    auto const& occlusions = mc::filter_occlusions_from(scene_elements, view_area);

    for (auto const& element : occlusions)
        element->occluded();

    mg::RenderableList renderable_list;
    renderable_list.reserve(scene_elements.size());
    for (auto const& element : scene_elements)
    {
        element->rendered();
        renderable_list.push_back(element->renderable());
    }

    /*
     * Note: Buffer lifetimes are ensured by the two objects holding
     *       references to them; scene_elements and renderable_list.
     *       So no buffer is going to be releaswayland_executored back to the client till
     *       both of those containers get destroyed (end of the function).
     *       Actually, there's a third reference held by the texture cache
     *       in GLRenderer, but that gets released earlier in render().
     */
    scene_elements.clear();  // Those in use are still in renderable_list

    std::vector<mg::DisplayElement> framebuffers;
    framebuffers.reserve(renderable_list.size());

    for (auto const& renderable : renderable_list)
    {
        auto fb = fb_adaptor->buffer_to_framebuffer(renderable->buffer());
        if (!fb)
        {
            break;
        }
        geometry::Rectangle clipped_dest;
        if (renderable->clip_area())
        {
            clipped_dest = intersection_of(renderable->screen_position(), *renderable->clip_area());
        }
        else
        {
            clipped_dest = renderable->screen_position();
        }
        geometry::SizeF const source_size{
            clipped_dest.size.width.as_value(),
            clipped_dest.size.height.as_value()};
        geometry::PointF const source_origin{
            clipped_dest.top_left.x.as_value() - renderable->screen_position().top_left.x.as_value(),
            clipped_dest.top_left.y.as_value() - renderable->screen_position().top_left.y.as_value()
        };

        framebuffers.emplace_back(mg::DisplayElement{
            renderable->screen_position(),
            geometry::RectangleF{source_origin, source_size},
            std::move(fb)
        });
    }

    if (framebuffers.size() == renderable_list.size() && display_sink.overlay(framebuffers))
    {
        report->renderables_in_frame(this, renderable_list);
        renderer->suspend();
    }
    else
    {
        renderer->set_output_transform(display_sink.transformation());
        renderer->set_viewport(view_area);
        renderer->set_output_filter(output_filter->filter());

        display_sink.set_next_image(fb_adaptor->buffer_to_framebuffer(renderer->render(renderable_list)));

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
