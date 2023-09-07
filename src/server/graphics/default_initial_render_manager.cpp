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
#include "mir/graphics/renderable.h"
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
auto constexpr FADE_TIME = std::chrono::milliseconds{1000};
auto constexpr UPDATE_TIME = std::chrono::milliseconds{16};

class FadingRenderable : public mg::Renderable
{
public:
    FadingRenderable(
        std::shared_ptr<mg::Renderable> const& renderable,
        mir::time::AlarmFactory& alarm_factory,
        std::shared_ptr<mir::time::Clock> const& clock)
        : renderable{renderable},
          clock{clock},
          start_time{clock->now()},
          alarm{alarm_factory.create_alarm([&]{
              auto delta_time = (clock->now().time_since_epoch() - start_time.time_since_epoch()) / std::chrono::milliseconds(1);
              _alpha = 1.f - (static_cast<float>(delta_time) / static_cast<float>(FADE_TIME.count()));
              mir::log_info("WE ARE HERE WITH ALPHA: %zu %zu\n", delta_time, FADE_TIME.count());

              if (_alpha <= 0)
              {
                  alarm->cancel();
              }
              else
              {
                  mir::time::Timestamp scheduled_time = clock->now() + UPDATE_TIME;
                  alarm->reschedule_for(scheduled_time);
              }
          })}
    {
        mir::time::Timestamp scheduled_time = clock->now() + UPDATE_TIME;
        alarm->reschedule_for(scheduled_time);
    }

    auto id() const -> mg::Renderable::ID override
    {
        return renderable->id();
    }

    auto buffer() const -> std::shared_ptr<mg::Buffer> override
    {
        return renderable->buffer();
    }

    auto screen_position() const -> mir::geometry::Rectangle override
    {
        return renderable->screen_position();
    }

    auto clip_area() const -> std::optional<mir::geometry::Rectangle> override
    {
        return renderable->clip_area();
    }

    auto alpha() const -> float override
    {
        return _alpha;
    }

    auto transformation() const -> glm::mat4 override
    {
        return renderable->transformation();
    }

    auto shaped() const -> bool override
    {
        return renderable->shaped();
    }

private:
    std::shared_ptr<mg::Renderable> const renderable;
    std::shared_ptr<mir::time::Clock> const clock;
    mir::time::Timestamp start_time;
    std::unique_ptr<mir::time::Alarm> const alarm;
    float _alpha = 1.0f;
};

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
    std::shared_ptr<time::AlarmFactory> const& alarm_factory,
    std::shared_ptr<input::Scene>  const& scene)
    : behavior{SmoothSupportBehavior::none},
      scene_executor{scene_executor},
      clock{clock},
      scene{scene},
      alarm_factory{alarm_factory},
      alarm{this->alarm_factory->create_alarm([&]{
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

        auto fading_renderable = std::make_shared<FadingRenderable>(renderable, *alarm_factory, clock);
        scene->add_input_visualization(fading_renderable);
        renderable_list.push_back(fading_renderable);
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
