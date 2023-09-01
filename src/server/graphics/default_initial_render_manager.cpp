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

namespace mg = mir::graphics;

mg::DefaultInitialRenderManager::DefaultInitialRenderManager(
    std::shared_ptr<time::Clock>&clock,
    time::AlarmFactory &alarm_factory,
    std::shared_ptr<input::Scene>& scene)
    : clock{clock},
      alarm_factory{alarm_factory},
      scene{scene}
{

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
        scene->add_input_visualization(renderable);
    }
    renderable_list.push_back(initial_render);
}