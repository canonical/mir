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

#include "cursor_controller.h"

#include "mir/input/input_targets.h"
#include "mir/input/surface.h"
#include "mir/graphics/cursor.h"

#include <assert.h>

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{

std::shared_ptr<mi::Surface> topmost_surface_containing_point(
    std::shared_ptr<mi::InputTargets> const& targets, geom::Point const& point)
{
    std::shared_ptr<mi::Surface> top_surface_at_point = nullptr;
    targets->for_each([&top_surface_at_point, &point]
        (std::shared_ptr<mi::Surface> const& surface) 
        {
            if (surface->input_area_contains(point))
                top_surface_at_point = surface;
        });
    return top_surface_at_point;
}

}

mi::CursorController::CursorController(std::shared_ptr<mg::Cursor> const& cursor,
    std::shared_ptr<mg::CursorImage> const& default_cursor_image) :
        cursor(cursor),
        default_cursor_image(default_cursor_image),
        input_targets(nullptr)
{
}

void mi::CursorController::cursor_moved_to(float abs_x, float abs_y)
{
    assert(input_targets);

    auto new_position = geom::Point{geom::X{abs_x}, geom::Y{abs_y}};

    auto surface = topmost_surface_containing_point(input_targets, new_position);
    if (surface)
    {
        auto image = surface->cursor_image();
        if(image)
            cursor->show(*image);
        else
            cursor->hide();
    }
    else
    {
        cursor->show(*default_cursor_image);
    }

    cursor->move_to(new_position);
}

void mi::CursorController::set_input_targets(std::shared_ptr<InputTargets> const& targets)
{
    // TODO: May need a guard on input targets;
    assert(!input_targets);
    input_targets = targets;
}
