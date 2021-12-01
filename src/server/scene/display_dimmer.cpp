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

#include "display_dimmer.h"
#include "mir/scene/idle_hub.h"
#include "mir/graphics/renderable.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/input/scene.h"

#include <mutex>

namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mrs = mir::renderer::software;

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

struct DimStateObserver : ms::IdleStateObserver
{
    DimStateObserver(
        std::shared_ptr<mi::Scene> const& input_scene,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator)
        : input_scene{input_scene},
          allocator{allocator}
    {
    }

    void idle_state_changed(ms::IdleState state) override
    {
        switch (state)
        {
        case ms::IdleState::awake:
        {
            std::lock_guard<std::mutex> lock{mutex};
            if (renderable)
            {
                input_scene->remove_input_visualization(renderable);
                renderable.reset();
            }
        }   break;

        case ms::IdleState::dim:
        {
            std::lock_guard<std::mutex> lock{mutex};
            if (!renderable)
            {
                renderable = std::make_shared<DimmingRenderable>(*allocator);
                input_scene->add_input_visualization(renderable);
            }
        }   break;

        default:;
        }
    }

private:
    std::shared_ptr<mi::Scene> const input_scene;
    std::shared_ptr<mg::GraphicBufferAllocator> const allocator;
    std::mutex mutex;
    std::shared_ptr<mg::Renderable> renderable;
};
}

ms::DisplayDimmer::DisplayDimmer(
    std::shared_ptr<IdleHub> const& idle_hub,
    std::shared_ptr<input::Scene> const& input_scene,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator)
    : idle_hub{idle_hub},
      idle_state_observer{std::make_shared<DimStateObserver>(input_scene, allocator)}
{
    idle_hub->register_interest(idle_state_observer);
}
