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

#include "mir/graphics/default_initial_render_manager.h"
#include "mir/input/scene.h"
#include "mir/time/alarm_factory.h"
#include "mir/time/clock.h"
#include "mir/executor.h"
#include <chrono>

namespace mg = mir::graphics;

mg::DefaultInitialRenderManager::DefaultInitialRenderManager(
    std::shared_ptr<Executor> const& scene_executor,
    std::shared_ptr<time::Clock>&clock,
    time::AlarmFactory &alarm_factory,
    std::shared_ptr<input::Scene>& scene)
    : scene_executor{scene_executor},
      clock{clock},
      scene{scene},
      alarm{alarm_factory.create_alarm([&]{
         remove_renderables();
         alarm->cancel();
      })}
{
    time::Timestamp scheduled_time = clock->now() + std::chrono::seconds {5};
    alarm->reschedule_for(scheduled_time);
}

mg::DefaultInitialRenderManager::~DefaultInitialRenderManager()
{
}

void mg::DefaultInitialRenderManager::add_initial_render(std::shared_ptr<InitialRender> const& initial_render)
{
    if (!initial_render)
        return;

    for (auto const& renderable : initial_render->get_renderables())
    {
        scene_executor->spawn([scene = scene, to_add = renderable]()
          {
              scene->add_input_visualization(to_add);
          });
        renderable_list.push_back(renderable);
    }
}

void mg::DefaultInitialRenderManager::remove_renderables()
{
    for (auto const& renderable : renderable_list)
    {
        scene_executor->spawn([scene = scene, to_remove = renderable]()
          {
              scene->remove_input_visualization(to_remove);
          });
    }
    renderable_list.clear();
}