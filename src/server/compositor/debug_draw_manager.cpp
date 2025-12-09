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

#include "mir/compositor/debug_draw_manager.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/renderable.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/graphics/buffer.h"
#include <mir/time/clock.h>

#include <glm/mat4x4.hpp>
#include <ranges>
#include <algorithm>
#include <cmath>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace
{
struct DebugSceneElement: public mc::SceneElement
{
public:
    explicit DebugSceneElement(std::shared_ptr<mg::Renderable> renderable)
        : renderable_{std::move(renderable)}
    {
    }

    std::shared_ptr<mg::Renderable> renderable() const override
    {
        return renderable_;
    }

    void rendered() override
    {
    }

    void occluded() override
    {
    }

private:
    std::shared_ptr<mg::Renderable> const renderable_;
};

class DebugRenderable : public mg::Renderable
{
public:
    DebugRenderable(std::shared_ptr<mg::Buffer> const& buffer, mir::geometry::Rectangle const& rect) :
        buffer_{buffer},
        rect_{rect}
    {
    }

    // Implement Renderable interface
    ID id() const override
    {
        return this;
    }
    std::shared_ptr<mg::Buffer> buffer() const override
    {
        return buffer_;
    }
    mir::geometry::Rectangle screen_position() const override
    {
        return rect_;
    }

    mir::geometry::RectangleD src_bounds() const override
    {
        if (buffer_)
        {
            auto size = buffer_->size();
            return mir::geometry::RectangleD{
                {0.0, 0.0}, {static_cast<double>(size.width.as_int()), static_cast<double>(size.height.as_int())}};
        }
        return mir::geometry::RectangleD{{0.0, 0.0}, {0.0, 0.0}};
    }

    std::optional<mir::geometry::Rectangle> clip_area() const override
    {
        return std::nullopt;
    }

    float alpha() const override
    {
        return 1.0f;
    }

    glm::mat4 transformation() const override
    {
        return glm::mat4(1.0f); // Identity matrix
    }

    MirOrientation orientation() const override
    {
        return mir_orientation_normal;
    }

    MirMirrorMode mirror_mode() const override
    {
        return mir_mirror_mode_none;
    }

    bool shaped() const override
    {
        return true;
    }

    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override
    {
        return std::nullopt;
    }

    auto opaque_region() const -> std::optional<mir::geometry::Rectangles> override
    {
        return std::nullopt;
    }

private:
    std::shared_ptr<mg::Buffer> buffer_;
    mir::geometry::Rectangle rect_;
};

auto draw_command_to_scene_element(
    mir::debug::DrawCommand const& command, std::shared_ptr<mg::GraphicBufferAllocator> const& allocator)
-> std::shared_ptr<mc::SceneElement>
{
    struct DrawCommandVisitor
    {
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator;
        auto operator()(
            mir::debug::CircleDrawCommand const& command)
        {
            // Calculate the bounding rectangle for the circle
            auto const& center = command.center;
            auto const& radius = command.radius;

            auto const left = center.x.as_int() - radius;
            auto const top = center.y.as_int() - radius;
            auto const width = radius * 2;
            auto const height = radius * 2;

            mir::geometry::Rectangle rect{
                mir::geometry::Point{left, top},
                mir::geometry::Size{width, height}};

            // Allocate a software buffer
            auto buffer = allocator->alloc_software_buffer(
                mir::geometry::Size{mir::geometry::Width{width}, mir::geometry::Height{height}},
                mir_pixel_format_argb_8888);


            // Draw the filled circle into the buffer
            auto mapped = mir::renderer::software::as_write_mappable(buffer);
            auto writable = mapped->map_writeable();
            auto stride = mapped->stride().as_int();

            // Draw a filled circle using Bresenham-like algorithm
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    // Calculate distance from center
                    int dx = x - radius;
                    int dy = y - radius;
                    int dist_sq = dx * dx + dy * dy;

                    uint32_t* pixel = reinterpret_cast<uint32_t*>(writable->data() + y * stride + x * 4);
                    // Check if point is inside circle
                    if (dist_sq <= radius * radius)
                    {
                        // Write pixel in ARGB format
                        uint8_t a = static_cast<uint8_t>(command.color.a * 255.0f);
                        uint8_t r = static_cast<uint8_t>(command.color.r * 255.0f);
                        uint8_t g = static_cast<uint8_t>(command.color.g * 255.0f);
                        uint8_t b = static_cast<uint8_t>(command.color.b * 255.0f);
                        *pixel = (a << 24) | (r << 16) | (g << 8) | b;
                    }
                    else
                    {
                        // Write transparent pixel
                        *pixel = 0x00000000;
                    }
                }
            }

            // Create a scene element from the renderable
            return std::make_shared<DebugSceneElement>(std::make_shared<DebugRenderable>(buffer, rect));
        }

        auto operator()(mir::debug::LineDrawCommand const& command)
        {
            auto const& start = command.start;
            auto const& end = command.end;
            auto const thickness = command.thickness;
            auto const half_thickness = thickness / 2.0f;

            // Calculate bounding box
            auto min_x = std::min(start.x.as_int(), end.x.as_int()) - static_cast<int>(std::ceil(half_thickness));
            auto min_y = std::min(start.y.as_int(), end.y.as_int()) - static_cast<int>(std::ceil(half_thickness));
            auto max_x = std::max(start.x.as_int(), end.x.as_int()) + static_cast<int>(std::ceil(half_thickness));
            auto max_y = std::max(start.y.as_int(), end.y.as_int()) + static_cast<int>(std::ceil(half_thickness));

            auto width = max_x - min_x;
            auto height = max_y - min_y;

            // Ensure valid dimensions
            if (width <= 0)
                width = 1;
            if (height <= 0)
                height = 1;

            mir::geometry::Rectangle rect{mir::geometry::Point{min_x, min_y}, mir::geometry::Size{width, height}};

            // Allocate a software buffer
            auto buffer = allocator->alloc_software_buffer(
                mir::geometry::Size{mir::geometry::Width{width}, mir::geometry::Height{height}},
                mir_pixel_format_argb_8888);

            auto mapped = mir::renderer::software::as_write_mappable(buffer);
            auto writable = mapped->map_writeable();
            auto stride = mapped->stride().as_int();

            // Precompute line segment vectors
            float lx = static_cast<float>(end.x.as_int() - start.x.as_int());
            float ly = static_cast<float>(end.y.as_int() - start.y.as_int());
            float len_sq = lx * lx + ly * ly;

            // Color
            uint8_t a = static_cast<uint8_t>(command.color.a * 255.0f);
            uint8_t r = static_cast<uint8_t>(command.color.r * 255.0f);
            uint8_t g = static_cast<uint8_t>(command.color.g * 255.0f);
            uint8_t b = static_cast<uint8_t>(command.color.b * 255.0f);
            uint32_t color_val = (a << 24) | (r << 16) | (g << 8) | b;

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    // Pixel position in global coords (center of pixel)
                    float px = static_cast<float>(min_x + x) + 0.5f;
                    float py = static_cast<float>(min_y + y) + 0.5f;

                    // Vector from start to pixel
                    float dx = px - static_cast<float>(start.x.as_int());
                    float dy = py - static_cast<float>(start.y.as_int());

                    // Project onto line segment
                    float t = 0.0f;
                    if (len_sq > 0.0f)
                        t = (dx * lx + dy * ly) / len_sq;

                    t = std::max(0.0f, std::min(1.0f, t));

                    // Closest point on segment
                    float cx = static_cast<float>(start.x.as_int()) + t * lx;
                    float cy = static_cast<float>(start.y.as_int()) + t * ly;

                    // Distance squared
                    float dist_sq = (px - cx) * (px - cx) + (py - cy) * (py - cy);

                    uint32_t* pixel = reinterpret_cast<uint32_t*>(writable->data() + y * stride + x * 4);

                    if (dist_sq <= half_thickness * half_thickness)
                    {
                        *pixel = color_val;
                    }
                    else
                    {
                        *pixel = 0x00000000;
                    }
                }
            }

            return std::make_shared<DebugSceneElement>(std::make_shared<DebugRenderable>(buffer, rect));
        }
    };

    return std::visit(DrawCommandVisitor{allocator}, command);
}
} // namespace

mc::DebugDrawManager::DebugDrawManager(
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mir::time::Clock> const& clock)
    : allocator{allocator},
      clock{clock},
      last_render_time{clock->now()}
{
}

void mc::DebugDrawManager::add(mir::debug::DrawCommand&& command)
{
    commands.lock()->push_back(std::move(command));
}

auto mc::DebugDrawManager::process_commands() -> std::vector<std::shared_ptr<mc::SceneElement>>
{
    using namespace std::literals::chrono_literals;

    auto const now = clock->now();
    // Clamping to account for times when the compositor is asleep
    auto const dt = std::clamp(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_render_time),
        0ms,
        16ms);
    last_render_time = now;

    auto cmds = commands.lock();

    std::erase_if(
        *cmds,
        [&](auto& draw_command)
        {
            return std::visit(
                [&](auto& cmd)
                {
                    cmd.lifetime -= dt;
                    return cmd.lifetime < 0ms;
                },
                draw_command);
        });

    return *cmds |
        std::views::transform(
            [&](auto const& draw_command)
            {
                return draw_command_to_scene_element(draw_command, allocator);
            }) |
        std::ranges::to<std::vector>();
}