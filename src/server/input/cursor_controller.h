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

#include <memory>

namespace mir
{
namespace graphics
{
class Cursor;
class CursorImage;
}

namespace input
{
class InputTargets;

class CursorController : public CursorListener
{
public:
    CursorController(std::shared_ptr<graphics::Cursor> const& cursor,
                     std::shared_ptr<graphics::CursorImage> const& default_cursor_image);
    virtual ~CursorController() = default;

    void cursor_moved_to(float abs_x, float abs_y);
    
    // Needed to break initialization cycle in server configuration.
    void set_input_targets(std::shared_ptr<InputTargets> const& targets);

private:
    std::shared_ptr<graphics::Cursor> const cursor;
    std::shared_ptr<graphics::CursorImage> const default_cursor_image;
    std::shared_ptr<InputTargets> input_targets;
};

}
}

#endif // MIR_INPUT_CURSOR_CONTROLLER_H_
