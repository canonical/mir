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

#ifndef MIR_INPUT_CURSOR_CONTROLLER_H_
#define MIR_INPUT_CURSOR_CONTROLLER_H_

#include "mir/input/cursor_observer.h"
#include "mir/frontend/drag_icon_controller.h"
#include "mir/geometry/point.h"

#include <memory>
#include <mutex>

namespace mir
{
namespace graphics
{
class Cursor;
class CursorImage;
}
namespace scene
{
class Observer;
}

namespace input
{
class Scene;

class CursorController : public CursorObserver, public frontend::DragIconController
{
public:
    CursorController(std::shared_ptr<Scene> const& input_targets,
        std::shared_ptr<graphics::Cursor> const& cursor,
        std::shared_ptr<graphics::CursorImage> const& default_cursor_image);
    virtual ~CursorController();

    void cursor_moved_to(float abs_x, float abs_y) override;

    // Trigger an update of the cursor image without cursor motion, e.g.
    // in response to scene changes.
    void update_cursor_image();

    void pointer_usable() override;

    void pointer_unusable() override;

    void set_drag_icon(std::weak_ptr<scene::Surface> icon) override;

private:
    std::shared_ptr<Scene> const input_targets;
    std::shared_ptr<graphics::Cursor> const cursor;
    std::shared_ptr<graphics::CursorImage> const default_cursor_image;
    std::weak_ptr<scene::Observer> observer;    // Not mutated after construction

    std::mutex cursor_state_guard;
    geometry::Point cursor_location;
    std::shared_ptr<graphics::CursorImage> current_cursor;
    bool usable = false;
    std::weak_ptr<scene::Surface> drag_icon;

    // Used only to serialize calls to pointer_usable()/pointer_unusable()
    std::mutex serialize_pointer_usable_unusable;

    void update_cursor_image_locked(std::unique_lock<std::mutex>&);
    void set_cursor_image_locked(std::unique_lock<std::mutex>&, std::shared_ptr<graphics::CursorImage> const& image);
};

}
}

#endif // MIR_INPUT_CURSOR_CONTROLLER_H_
