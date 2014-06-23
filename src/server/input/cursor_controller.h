/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_CURSOR_CONTROLLER_H_
#define MIR_INPUT_CURSOR_CONTROLLER_H_

#include "mir/input/cursor_listener.h"
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
class InputTargets;

class CursorController : public CursorListener
{
public:
    CursorController(std::shared_ptr<InputTargets> const& input_targets,
        std::shared_ptr<graphics::Cursor> const& cursor,
        std::shared_ptr<graphics::CursorImage> const& default_cursor_image);
    virtual ~CursorController();

    void cursor_moved_to(float abs_x, float abs_y);

    // Trigger an update of the cursor image without cursor motion, e.g.
    // in response to scene changes.
    void update_cursor_image();

private:
    std::shared_ptr<InputTargets> const input_targets;
    std::shared_ptr<graphics::Cursor> const cursor;
    std::shared_ptr<graphics::CursorImage> const default_cursor_image;

    std::mutex cursor_state_guard;
    geometry::Point cursor_location;
    std::shared_ptr<graphics::CursorImage> current_cursor;

    std::weak_ptr<scene::Observer> observer;
    

    void update_cursor_image_locked(std::lock_guard<std::mutex> const&);
    void set_cursor_image_locked(std::lock_guard<std::mutex> const&, std::shared_ptr<graphics::CursorImage> const& image);
};

}
}

#endif // MIR_INPUT_CURSOR_CONTROLLER_H_
