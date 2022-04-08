/*
 * Copyright © 2022 Canonical Ltd.
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include "basic_screen_shooter.h"
#include "mir/renderer/gl/buffer_render_target.h"
#include "mir/renderer/renderer_factory.h"
#include "mir/renderer/renderer.h"
#include "mir/renderer/gl/buffer_render_target.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/renderer/gl/context.h"
#include "mir/compositor/scene_element.h"
#include "mir/compositor/scene.h"
#include "mir/system_executor.h"
#include "mir/raii.h"

namespace mc = mir::compositor;
namespace mr = mir::renderer;
namespace mg = mir::graphics;
namespace mrg = mir::renderer::gl;
namespace mrs = mir::renderer::software;
namespace geom = mir::geometry;

mc::BasicScreenShooter::Self::Self(
    std::shared_ptr<Scene> const& scene,
    renderer::gl::ContextSource& context_source,
    mr::RendererFactory& renderer_factory,
    std::shared_ptr<time::Clock> const& clock)
    : scene{scene},
      render_target{std::make_unique<mrg::BufferRenderTarget>(context_source.create_gl_context())},
      renderer{renderer_factory.create_renderer_for(*render_target)},
      clock{clock}
{
}

mc::BasicScreenShooter::BasicScreenShooter(
    std::shared_ptr<Scene> const& scene,
    renderer::gl::ContextSource& context_source,
    mr::RendererFactory& renderer_factory,
    std::shared_ptr<time::Clock> const& clock)
    : self{std::make_shared<Self>(scene, context_source, renderer_factory, clock)}
{
}

void mc::BasicScreenShooter::capture(
    std::shared_ptr<mrs::WriteMappableBuffer> const& buffer,
    geom::Rectangle const& area,
    std::function<void(std::optional<time::Timestamp>)>&& callback)
{
    // TODO: use an atomic to keep track of number of in-flight captures, and error if it's too many

    system_executor.spawn([weak_self=std::weak_ptr<Self>{self}, buffer, area, callback=std::move(callback)]
        {
            // This will make sure the callback is called no matter what happens
            std::optional<time::Timestamp> completed;
            raii::PairedCalls cleanup{[](){},
                [&]()
                {
                    callback(completed);
                }
            };

            if (auto const self = weak_self.lock())
            {
                std::lock_guard<std::mutex> lock{self->mutex};

                auto scene_elements = self->scene->scene_elements_for(self.get());
                auto const captured_time = self->clock->now();
                mg::RenderableList renderable_list;
                renderable_list.reserve(scene_elements.size());
                for (auto const& element : scene_elements)
                {
                    renderable_list.push_back(element->renderable());
                }
                scene_elements.clear();

                self->render_target->make_current();
                self->render_target->set_buffer(buffer, area.size);

                self->render_target->bind();
                self->renderer->set_output_transform(glm::mat2{1});
                self->renderer->set_viewport(area);
                self->renderer->render(renderable_list);

                self->render_target->release_current();
                renderable_list.clear();

                completed = captured_time;
            }
        });
}
