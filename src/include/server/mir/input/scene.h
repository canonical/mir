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

#ifndef MIR_INPUT_INPUT_SCENE_H_
#define MIR_INPUT_INPUT_SCENE_H_

#include <memory>
#include <functional>

#include "mir/geometry/point.h"

namespace mir
{
namespace scene
{
class Observer;
}
namespace graphics
{
class Renderable;
}

namespace input
{
class Surface;

class Scene
{
public:
    virtual ~Scene() = default;

    virtual auto input_surface_at(geometry::Point point) const -> std::shared_ptr<input::Surface> = 0;

    virtual void add_observer(std::shared_ptr<scene::Observer> const& observer) = 0;
    virtual void remove_observer(std::weak_ptr<scene::Observer> const& observer) = 0;

    /// An interface which the input stack can use to add certain non-interactive input visualizations
    /// in to the scene (i.e. cursors, touchspots). Overlay renderables will be rendered above all surfaces.
    /// Within the set of overlay renderables, the render order is such that the first-added will appear below
    /// the second-added, the second-added will appear below the third-added, and so on. Renderables added
    /// via [add_bottom_input_visualization] are guaranteed to be below any renderable added with
    /// [add_input_visualization].
    virtual void add_input_visualization(std::shared_ptr<graphics::Renderable> const& overlay) = 0;
    /// Similar to [add_input_visualization] except that the added renderable is guaranteed to be rendered below
    /// any other input visualization but above any non-input visualization renderable. If called more than once, the
    /// renderables will be displayed such that the first-added is above the second-added, the second-added is above
    /// the third-added, and so on.
    virtual void add_bottom_input_visualization(std::shared_ptr<graphics::Renderable> const& overlay) = 0;
    virtual void remove_input_visualization(std::weak_ptr<graphics::Renderable> const& overlay) = 0;
    
    // As input visualizations added through the overlay system will not use the standard SurfaceObserver
    // mechanism, we require this method to trigger recomposition.
    // TODO: How can something like SurfaceObserver be adapted to work with non surface renderables?
    virtual void emit_scene_changed() = 0;

    /// Returns if the screen is currently locked
    virtual auto screen_is_locked() const -> bool = 0;

protected:
    Scene() = default;
    Scene(Scene const&) = delete;
    Scene& operator=(Scene const&) = delete;
};

}
}

#endif // MIR_INPUT_INPUT_SCENE
