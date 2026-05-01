/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIR_FRONTEND_WLR_SCREENCOPY_V1_H
#define MIR_FRONTEND_WLR_SCREENCOPY_V1_H

#include "wlr_screencopy_unstable_v1.h"
#include <mir/geometry/rectangle.h>

#include <memory>
#include <glm/glm.hpp>

namespace mir
{
class Executor;
namespace compositor
{
class ScreenShooterFactory;
}
namespace graphics
{
class GraphicBufferAllocator;
}
namespace scene
{
class SceneChangeNotification;
}
namespace compositor
{
class ScreenShooter;
}
namespace frontend
{
class OutputManager;
class SurfaceStack;
struct WlrScreencopyV1Ctx;

/// Tracks damage and captures frames when needed. Each instance used by a single manager (and thus a single client).
class WlrScreencopyV1DamageTracker : public wayland_rs::LifetimeTracker
{
public:
    struct FrameParams
    {
        wayland_rs::Weak<wayland_rs::WlOutputImpl> output;
        geometry::Rectangle output_space_area;
        geometry::Size buffer_size;
        glm::mat2 transform;
        bool overlay_cursor;

        auto operator==(FrameParams const& other) const -> bool
        {
            return output == other.output &&
                   output_space_area == other.output_space_area &&
                   buffer_size == other.buffer_size &&
                   transform == other.transform;
        }

        auto full_buffer_space_damage() const -> geometry::Rectangle
        {
            return {{}, buffer_size};
        }
    };

    class Frame : public std::enable_shared_from_this<Frame>
    {
    public:
        virtual ~Frame() = default;
        virtual auto destroyed_flag() const -> std::shared_ptr<bool const> = 0;
        virtual auto parameters() const -> FrameParams const& = 0;
        virtual void capture(geometry::Rectangle buffer_space_damage) = 0;
    };

    WlrScreencopyV1DamageTracker(Executor& wayland_executor, SurfaceStack& surface_stack);
    ~WlrScreencopyV1DamageTracker();

    void capture_on_damage(Frame* frame);

private:
    void create_change_notifier();

    enum class DamageAmount
    {
        none,
        partial,
        full,
    };

    /// Used to track damage to a specific area
    class Area;

    Executor& wayland_executor;
    SurfaceStack& surface_stack;

    /// Created on first use, so that screencopy managers that are not requested to copy frames with damage do not
    /// incur the overhead of a scene change notifier
    std::shared_ptr<scene::SceneChangeNotification> change_notifier;
    /// Frames that are waiting for damage before they are captured. If the frame object is null that means no damage
    /// has been received since a previous frame with the same params.
    std::vector<std::unique_ptr<Area>> areas;
};

class WlrScreencopyManagerV1
    : public wayland_rs::ZwlrScreencopyManagerV1Impl,
      public std::enable_shared_from_this<WlrScreencopyManagerV1>
{
public:
    WlrScreencopyManagerV1(std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<compositor::ScreenShooterFactory> const& screen_shooter_factory,
        std::shared_ptr<SurfaceStack> const& surface_stack);

    void capture_on_damage(WlrScreencopyV1DamageTracker::Frame* frame);

    std::unique_ptr<compositor::ScreenShooter> const screen_shooter;
    /// From mw::WlrScreencopyManagerV1
    /// @{
    auto capture_output(int32_t overlay_cursor, wayland_rs::Weak<wayland_rs::WlOutputImpl> const& output) -> std::shared_ptr<wayland_rs::ZwlrScreencopyFrameV1Impl> override;
    auto capture_output_region(int32_t overlay_cursor, wayland_rs::Weak<wayland_rs::WlOutputImpl> const& output, int32_t x, int32_t y, int32_t width, int32_t height) -> std::shared_ptr<wayland_rs::ZwlrScreencopyFrameV1Impl> override;
    /// @}
private:
    std::shared_ptr<WlrScreencopyV1Ctx> const ctx;
    WlrScreencopyV1DamageTracker damage_tracker;
};
}
}

#endif // MIR_FRONTEND_WLR_SCREENCOPY_V1_H
