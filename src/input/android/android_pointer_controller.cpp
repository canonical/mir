/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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
#include "mir/graphics/viewable_area.h"

#include "android_pointer_controller.h"

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mia = mi::android;


mia::PointerController::PointerController(std::shared_ptr<mg::ViewableArea> const& viewable_area) :
    state(0),
    x(0.0), 
    y(0.0),
    viewable_area(viewable_area),
    cursor_listener(std::shared_ptr<mi::CursorListener>())
{
}


mia::PointerController::PointerController(std::shared_ptr<mg::ViewableArea> const& viewable_area,
					  std::shared_ptr<mi::CursorListener> const& cursor_listener) : 
    state(0),
    x(0.0), 
    y(0.0),
    viewable_area(viewable_area),
    cursor_listener(cursor_listener)

{
}

void mia::PointerController::notify_listener()
{
    if (cursor_listener)
	cursor_listener->moved(x, y);
}

bool mia::PointerController::getBounds(float* out_min_x, float* out_min_y, float* out_max_x, float* out_max_y) const
{
    std::lock_guard<std::mutex> lg(guard);
    auto bounds = viewable_area->view_area();
    *out_min_x = bounds.top_left.x.as_float();
    *out_min_y = bounds.top_left.y.as_float();
    *out_max_x = bounds.top_left.x.as_float() + bounds.size.width.as_float();
    *out_max_y = bounds.top_left.y.as_float() + bounds.size.height.as_float();
    return true;
}
void mia::PointerController::move(float delta_x, float delta_y)
{
    std::lock_guard<std::mutex> lg(guard);
    x += delta_x;
    y += delta_y;
    // I think it's correct to hold this lock while notifying the listener (i.e. cursor rendering update)
    // to prevent the InputReader from getting ahead of rendering. This may need to be thought about later.
    notify_listener();
}
void mia::PointerController::setButtonState(int32_t button_state)
{
    std::lock_guard<std::mutex> lg(guard);
    state = button_state;
}
int32_t mia::PointerController::getButtonState() const
{
    std::lock_guard<std::mutex> lg(guard);
    return state;
}
void mia::PointerController::setPosition(float new_x, float new_y)
{
    std::lock_guard<std::mutex> lg(guard);
    // Needs bounds checking
    x = new_x;
    y = new_y;
    // See comment in ::move
    notify_listener();
}
void mia::PointerController::getPosition(float *out_x, float *out_y) const
{
    std::lock_guard<std::mutex> lg(guard);
    *out_x = x;
    *out_y = y;
}
