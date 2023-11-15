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

#include "mir_test_framework/headless_display_buffer_compositor_factory.h"
#include "mir_test_framework/passthrough_tracker.h"
#include "mir/graphics/display_sink.h"
#include "mir/graphics/texture.h"
#include "mir/graphics/platform.h"
#include "mir/renderer/gl/gl_surface.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/buffer.h"
#include "mir/geometry/rectangle.h"

namespace mtf = mir_test_framework;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

mtf::HeadlessDisplayBufferCompositorFactory::HeadlessDisplayBufferCompositorFactory(
    std::shared_ptr<mg::GLRenderingProvider> render_platform,
    std::shared_ptr<mg::GLConfig> gl_config)
    : render_platform{std::move(render_platform)},
      gl_config{std::move(gl_config)},
      tracker(nullptr)
{
}

mtf::HeadlessDisplayBufferCompositorFactory::HeadlessDisplayBufferCompositorFactory(
    std::shared_ptr<mg::GLRenderingProvider> render_platform,
    std::shared_ptr<mg::GLConfig> gl_config,
    std::shared_ptr<PassthroughTracker> const& tracker)
    : render_platform{std::move(render_platform)},
      gl_config{std::move(gl_config)},
      tracker(tracker)
{
}

std::unique_ptr<mir::compositor::DisplayBufferCompositor>
mtf::HeadlessDisplayBufferCompositorFactory::create_compositor_for(mg::DisplaySink& display_sink)
{
    struct HeadlessDBC : mir::compositor::DisplayBufferCompositor
    {
        HeadlessDBC(
            mg::DisplaySink& sink,
            std::unique_ptr<mg::gl::OutputSurface> output,
            std::shared_ptr<mg::GLRenderingProvider> render_platform,
            std::shared_ptr<mtf::PassthroughTracker> const& tracker) :
            display_sink{sink},
            output{std::move(output)},
            render_platform{std::move(render_platform)},
            tracker(tracker)
        {
        }

        mg::RenderableList filter(
            mc::SceneElementSequence& elements,
            geom::Rectangle const& display_area)
        {
            mg::RenderableList renderables;
            for (auto it = elements.rbegin(); it != elements.rend(); it++)
            {
                auto const area = intersection_of((*it)->renderable()->screen_position(), display_area);
                bool offscreen = (area == geom::Rectangle{});
                bool occluded = false;
                for(auto& r : renderables)
                    occluded |= r->screen_position().contains(area) && !r->shaped() && (r->alpha() == 1.0f);

                if (offscreen || occluded)
                {
                    (*it)->occluded();
                }
                else
                {
                    (*it)->rendered();
                    renderables.push_back((*it)->renderable());
                }
            }
            std::reverse(renderables.begin(), renderables.end());
            return renderables;
        }

        bool composite(mir::compositor::SceneElementSequence&& seq) override
        {
            auto renderlist = filter(seq, display_sink.view_area());

            // TODO: Might need to attempt to overlay in order to test those codepaths?

            output->bind();

            // We need to consume a buffer to unblock client tests
            for (auto const& renderable : renderlist)
            {
                auto buf = renderable->buffer();
                if (auto tex = render_platform->as_texture(buf))
                {
                    // bind() is what drives the Wayland frame event.
                    tex->bind();
                }
            }

            output->commit();
            return true;
        }
        mg::DisplaySink& display_sink;
        std::unique_ptr<mg::gl::OutputSurface> const output;
        std::shared_ptr<mg::GLRenderingProvider> const render_platform;
        std::shared_ptr<PassthroughTracker> const tracker;
    };
    auto output_surface =
        render_platform->surface_for_sink(display_sink, display_sink.pixel_size(), *gl_config);
    return std::make_unique<HeadlessDBC>(display_sink, std::move(output_surface), render_platform, tracker);
}
