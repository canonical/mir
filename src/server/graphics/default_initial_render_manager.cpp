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
#include "mir/log.h"
#include "mir/options/option.h"
#include <mir/options/configuration.h>
#include <chrono>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mo = mir::options;

namespace
{
auto constexpr FADE_TIME = std::chrono::milliseconds{5000};

mg::SmoothSupportBehavior from_option(std::string const& option)
{
    if (option == "fade")
        return mg::SmoothSupportBehavior::fade;
    else if (option == "none")
        return mg::SmoothSupportBehavior::none;
    else
    {
        std::string error_message = "Invalid option string for smooth support behavior: " + option;
        BOOST_THROW_EXCEPTION(std::invalid_argument(error_message));
    }
}
}

mg::DefaultInitialRenderManager::DefaultInitialRenderManager(
    std::shared_ptr<options::Option> const& options,
    std::shared_ptr<Executor> const& scene_executor,
    std::shared_ptr<time::Clock> const& clock,
    time::AlarmFactory& alarm_factory,
    std::shared_ptr<input::Scene>  const& scene)
    : behavior{SmoothSupportBehavior::none},
      scene_executor{scene_executor},
      clock{clock},
      scene{scene},
      alarm{alarm_factory.create_alarm([&]{
         remove_renderables();
         alarm->cancel();
      })}
{
    if (options->is_set(mo::smooth_boot_opt))
        behavior = from_option(options->get<std::string>(mo::smooth_boot_opt));

    time::Timestamp scheduled_time = clock->now() + FADE_TIME;
    alarm->reschedule_for(scheduled_time);
}

mg::DefaultInitialRenderManager::~DefaultInitialRenderManager()
{
}

void mg::DefaultInitialRenderManager::add_initial_render(std::shared_ptr<InitialRender> const& initial_render)
{
    if (!initial_render)
        return;

    if (!scene)
    {
        mir::log_error("Unable to add the initial renderables because the scene does not exist");
        return;
    }

    initial_render->for_each_renderable([&](std::shared_ptr<Renderable> const& renderable)
    {
        if (!renderable)
            return;

        scene->add_input_visualization(renderable);
        renderable_list.push_back(renderable);
    });
}

void mg::DefaultInitialRenderManager::remove_renderables()
{
    if (!scene_executor)
    {
        mir::log_error("Unable to remove the initial renderables because the executor does not exist");
        return;
    }

    if (!scene)
    {
        mir::log_error("Unable to remove the initial renderables because the scene does not exist");
        return;
    }

    scene_executor->spawn([the_scene = scene, to_remove = renderable_list]()
    {
        for (auto const& renderable : to_remove)
            the_scene->remove_input_visualization(renderable);
    });
}