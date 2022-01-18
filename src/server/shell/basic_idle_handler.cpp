/*
 * Copyright © 2021 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "basic_idle_handler.h"
#include "mir/scene/idle_hub.h"
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

using namespace std::literals::chrono_literals;

namespace
{
unsigned char const black_pixel_data[4] = {0, 0, 0, 255};
const int coverage_size = 100000;

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

    auto clip_area() const -> std::experimental::optional<geom::Rectangle> override
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

private:
    std::shared_ptr<mg::Buffer> const buffer_;
};

struct Dimmer : ms::IdleStateObserver
{
    Dimmer(
        std::shared_ptr<mi::Scene> const& input_scene,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator)
        : input_scene{input_scene},
          allocator{allocator}
    {
    }

    void active() override
    {
        std::lock_guard<std::mutex> lock{mutex};
        if (renderable)
        {
            input_scene->remove_input_visualization(renderable);
            renderable.reset();
        }

    }

    void idle() override
    {
        std::lock_guard<std::mutex> lock{mutex};
        if (!renderable)
        {
            renderable = std::make_shared<DimmingRenderable>(*allocator);
            input_scene->add_input_visualization(renderable);
        }
    }

private:
    std::shared_ptr<mi::Scene> const input_scene;
    std::shared_ptr<mg::GraphicBufferAllocator> const allocator;
    std::mutex mutex;
    std::shared_ptr<mg::Renderable> renderable;
};

struct PowerModeSetter : ms::IdleStateObserver
{
    PowerModeSetter(std::shared_ptr<msh::DisplayConfigurationController> const& controller, MirPowerMode power_mode)
        : controller{controller},
          power_mode{power_mode}
    {
    }

    void active() override
    {
        controller->set_power_mode(mir_power_mode_on);
    }

    void idle() override
    {
        controller->set_power_mode(power_mode);
    }

private:
    std::shared_ptr<msh::DisplayConfigurationController> const controller;
    MirPowerMode const power_mode;
};
}

msh::BasicIdleHandler::BasicIdleHandler(
    std::shared_ptr<ms::IdleHub> const& idle_hub,
    std::shared_ptr<input::Scene> const& input_scene,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<msh::DisplayConfigurationController> const& display_config_controller)
    : idle_hub{idle_hub},
      timeouts{
          Timeout{300s, std::make_shared<Dimmer>(input_scene, allocator)},
          Timeout{310s, std::make_shared<PowerModeSetter>(display_config_controller, mir_power_mode_off)},
      }
{
    for (auto const& timeout : timeouts)
    {
        idle_hub->register_interest(timeout.observer, timeout.time);
    }
}

msh::BasicIdleHandler::~BasicIdleHandler()
{
    for (auto const& timeout : timeouts)
    {
        idle_hub->unregister_interest(*timeout.observer);
    }
}
