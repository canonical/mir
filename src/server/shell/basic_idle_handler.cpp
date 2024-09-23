/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "basic_idle_handler.h"
#include "mir/fatal.h"
#include "mir/scene/idle_hub.h"
#include "mir/scene/session_lock.h"
#include "mir/graphics/renderable.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/input/scene.h"
#include "mir/shell/display_configuration_controller.h"

#include <mutex>

namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mrs = mir::renderer::software;
namespace msh = mir::shell;

namespace
{
auto const dim_time_before_off = std::chrono::seconds{10};
unsigned char const black_pixel_data[4] = {0, 0, 0, 255};
int const coverage_size = 100000;

struct DimmingRenderable : public mg::Renderable
{
public:
    DimmingRenderable(mg::GraphicBufferAllocator& allocator)
        : buffer_{mrs::alloc_buffer_with_content(
              allocator,
              black_pixel_data,
              {1, 1},
              geom::Stride{4}, // 1 pixel, 4 bytes per pixel
              mir_pixel_format_abgr_8888)}
    {
    }

    auto id() const -> mg::Renderable::ID override
    {
        return this;
    }

    auto buffer() const -> std::shared_ptr<mg::Buffer> override
    {
        return buffer_;
    }

    auto screen_position() const -> geom::Rectangle override
    {
        return {{-coverage_size / 2, -coverage_size / 2}, {coverage_size, coverage_size}};
    }

    auto src_bounds() const -> geom::RectangleD override
    {
        return {{0, 0}, buffer_->size()};
    }

    auto clip_area() const -> std::optional<geom::Rectangle> override
    {
        return {};
    }

    auto alpha() const -> float override
    {
        return 0.5f;
    }

    auto transformation() const -> glm::mat4 override
    {
        return glm::mat4{1};
    }

    auto shaped() const -> bool override
    {
        return false;
    }

    auto surface_if_any() const
        -> std::optional<mir::scene::Surface const*> override
    {
        return std::nullopt;
    }

private:
    std::shared_ptr<mg::Buffer> const buffer_;
};

struct Dimmer : ms::IdleStateObserver
{
    Dimmer(
        std::shared_ptr<mi::Scene> const& input_scene,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
        msh::IdleHandlerObserver& observer)
        : input_scene{input_scene},
          allocator{allocator},
          observer{observer}
    {
    }

    void active() override
    {
        if (renderable)
        {
            input_scene->remove_input_visualization(renderable);
            renderable.reset();
            observer.wake();
        }

    }

    void idle() override
    {
        if (!renderable)
        {
            renderable = std::make_shared<DimmingRenderable>(*allocator);
            input_scene->add_input_visualization(renderable);
            observer.dim();
        }
    }

private:
    std::shared_ptr<mi::Scene> const input_scene;
    std::shared_ptr<mg::GraphicBufferAllocator> const allocator;
    std::shared_ptr<mg::Renderable> renderable;
    msh::IdleHandlerObserver& observer;
};

struct PowerModeSetter : ms::IdleStateObserver
{
    PowerModeSetter(
        std::shared_ptr<msh::DisplayConfigurationController> const& controller,
        MirPowerMode power_mode,
        msh::IdleHandlerObserver& observer)
        : controller{controller},
          power_mode{power_mode},
          observer{observer}
    {
    }

    void active() override
    {
        controller->set_power_mode(mir_power_mode_on);
    }

    void idle() override
    {
        controller->set_power_mode(power_mode);
        observer.off();
    }

private:
    std::shared_ptr<msh::DisplayConfigurationController> const controller;
    MirPowerMode const power_mode;
    msh::IdleHandlerObserver& observer;
};

struct TimeoutRestorer : ms::IdleStateObserver
{
    explicit TimeoutRestorer(
        msh::BasicIdleHandler* idle_handler,
        std::optional<mir::time::Duration> timeout)
        : idle_handler{idle_handler},
          timeout{timeout}
    {
    }

    void active() override {
        if (!restored)
        {
            restored = true;
            idle_handler->set_display_off_timeout(timeout);
        }
    }

    void idle() override
    {
    }

private:
    mir::shell::BasicIdleHandler* const idle_handler;
    std::optional<mir::time::Duration> const timeout;

    bool restored{false};
};
}

class msh::BasicIdleHandler::SessionLockListener : public ms::SessionLockObserver
{
public:
    explicit SessionLockListener(std::function<void()> const& on_lock) : on_lock_{on_lock} {}

private:
    void on_lock() override { on_lock_(); }
    void on_unlock() override {}

    std::function<void()> const on_lock_;
};

msh::BasicIdleHandler::BasicIdleHandler(
    std::shared_ptr<ms::IdleHub> const& idle_hub,
    std::shared_ptr<input::Scene> const& input_scene,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<msh::DisplayConfigurationController> const& display_config_controller,
    std::shared_ptr<ms::SessionLock> const& session_lock)
    : idle_hub{idle_hub},
      input_scene{input_scene},
      allocator{allocator},
      display_config_controller{display_config_controller},
      session_lock{session_lock},
      session_lock_monitor{std::make_shared<SessionLockListener>([this]
          {
              std::lock_guard lock{mutex};
              if (auto const timeout_on_lock{current_off_timeout_on_lock})
              {
                  if (timeout_on_lock != current_off_timeout)
                  {
                      auto const previous_off_timeout{current_off_timeout};
                      current_off_timeout = timeout_on_lock;
                      clear_observers(lock);
                      register_observers(lock);
                      auto const timeout_restorer{std::make_shared<TimeoutRestorer>(this, previous_off_timeout)};
                      observers.push_back(timeout_restorer);
                      this->idle_hub->register_interest(timeout_restorer, time::Duration{0});
                  }
              }
          })}
{
    session_lock->register_interest(session_lock_monitor);
}

msh::BasicIdleHandler::~BasicIdleHandler()
{
    session_lock->unregister_interest(*session_lock_monitor);
    std::lock_guard lock{mutex};
    for (auto const& observer : observers)
    {
        idle_hub->unregister_interest(*observer);
    }
}

void msh::BasicIdleHandler::set_display_off_timeout(std::optional<time::Duration> timeout)
{
    std::lock_guard lock{mutex};
    if (timeout != current_off_timeout)
    {
        current_off_timeout = timeout;
        clear_observers(lock);
        register_observers(lock);
    }
}

void msh::BasicIdleHandler::set_display_off_timeout_on_lock(std::optional<time::Duration> timeout)
{
    std::lock_guard lock{mutex};
    current_off_timeout_on_lock = timeout;
}

void msh::BasicIdleHandler::register_observers(ProofOfMutexLock const&)
{
    if (current_off_timeout)
    {
        auto const off_timeout = current_off_timeout.value();
        if (off_timeout < time::Duration{0})
        {
            fatal_error("BasicIdleHandler given invalid timeout %d, should be >=0", off_timeout.count());
        }
        if (off_timeout >= dim_time_before_off * 2)
        {
            auto const dim_timeout = off_timeout - dim_time_before_off;
            auto const dimmer = std::make_shared<Dimmer>(input_scene, allocator, multiplexer);
            observers.push_back(dimmer);
            idle_hub->register_interest(dimmer, dim_timeout);
        }
        auto const power_setter = std::make_shared<PowerModeSetter>(
            display_config_controller, mir_power_mode_off, multiplexer);
        observers.push_back(power_setter);
        idle_hub->register_interest(power_setter, off_timeout);
    }
}

void msh::BasicIdleHandler::clear_observers(ProofOfMutexLock const&)
{
    for (auto const& observer : observers)
    {
        // If we remove observers in an idle state, the display might become stuck off or dim
        observer->active();
        idle_hub->unregister_interest(*observer);
    }
    observers.clear();
}

void msh::BasicIdleHandler::register_interest(std::weak_ptr<IdleHandlerObserver> const& observer)
{
    multiplexer.register_interest(observer);
}

void msh::BasicIdleHandler::register_interest(
    std::weak_ptr<IdleHandlerObserver> const& observer,
    Executor& executor)
{
    multiplexer.register_interest(observer, executor);
}

void msh::BasicIdleHandler::register_early_observer(
    std::weak_ptr<IdleHandlerObserver> const& observer,
    Executor& executor)
{
    multiplexer.register_interest(observer, executor);
}

void msh::BasicIdleHandler::unregister_interest(IdleHandlerObserver const& observer)
{
    multiplexer.unregister_interest(observer);
}
