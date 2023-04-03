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

#include "basic_screen_shooter.h"
#include "mir/renderer/gl/buffer_render_target.h"
#include "mir/renderer/renderer.h"
#include "mir/renderer/gl/context.h"
#include "mir/compositor/scene_element.h"
#include "mir/compositor/scene.h"
#include "mir/log.h"
#include "mir/executor.h"

namespace mc = mir::compositor;
namespace mr = mir::renderer;
namespace mg = mir::graphics;
namespace mrg = mir::renderer::gl;
namespace mrs = mir::renderer::software;
namespace geom = mir::geometry;

mc::BasicScreenShooter::Self::Self(
    std::shared_ptr<Scene> const& scene,
    std::shared_ptr<time::Clock> const& clock,
    std::unique_ptr<mrg::BufferOutputSurface>&& render_target,
    std::unique_ptr<mr::Renderer>&& renderer)
    : scene{scene},
      render_target{std::move(render_target)},
      renderer{std::move(renderer)},
      clock{clock}
{
}

auto mc::BasicScreenShooter::Self::render(
    std::shared_ptr<mrs::WriteMappableBuffer> const& buffer,
    geom::Rectangle const& area) -> time::Timestamp
{
    std::lock_guard lock{mutex};

    auto scene_elements = scene->scene_elements_for(this);
    auto const captured_time = clock->now();
    mg::RenderableList renderable_list;
    renderable_list.reserve(scene_elements.size());
    for (auto const& element : scene_elements)
    {
        renderable_list.push_back(element->renderable());
    }
    scene_elements.clear();

    render_target->make_current();
    render_target->set_buffer(buffer);

    render_target->bind();
    renderer->set_viewport(area);
//    renderer->render(renderable_list);

    renderable_list.clear();

    return captured_time;
}

mc::BasicScreenShooter::BasicScreenShooter(
    std::shared_ptr<Scene> const& scene,
    std::shared_ptr<time::Clock> const& clock,
    Executor& executor,
    std::unique_ptr<mrg::BufferOutputSurface>&& render_target,
    std::unique_ptr<mr::Renderer>&& renderer)
    : self{std::make_shared<Self>(scene, clock, std::move(render_target), std::move(renderer))},
      executor{executor}
{
}

void mc::BasicScreenShooter::capture(
    std::shared_ptr<mrs::WriteMappableBuffer> const& buffer,
    geom::Rectangle const& area,
    std::function<void(std::optional<time::Timestamp>)>&& callback)
{
    // TODO: use an atomic to keep track of number of in-flight captures, and error if it's too many

    executor.spawn([weak_self=std::weak_ptr<Self>{self}, buffer, area, callback=std::move(callback)]
        {
            if (auto const self = weak_self.lock())
            {
                try
                {
                    callback(self->render(buffer, area));
                    return;
                }
                catch (...)
                {
                    mir::log(
                        ::mir::logging::Severity::error,
                        "BasicScreenShooter",
                        std::current_exception(),
                        "failed to capture screen");
                }
            }

            callback(std::nullopt);
        });
}
