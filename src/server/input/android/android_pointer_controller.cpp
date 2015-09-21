/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir/input/input_region.h"
#include "mir/input/touch_visualizer.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/point.h"

#include "android_pointer_controller.h"

namespace mi = mir::input;
namespace mia = mi::android;
namespace geom = mir::geometry;

mia::PointerController::PointerController(
    std::shared_ptr<mi::InputRegion> const& input_region,
    std::shared_ptr<mi::CursorListener> const& cursor_listener,
    std::shared_ptr<mi::TouchVisualizer> const& touch_visualizer)
        : state(0),
          x(0.0),
          y(0.0),
          input_region(input_region),
          cursor_listener(cursor_listener),
          touch_visualizer(touch_visualizer)
{
}

void mia::PointerController::notify_listener()
{
    if (cursor_listener)
        cursor_listener->cursor_moved_to(x, y);
}

bool mia::PointerController::getBounds(
    float* out_min_x,
    float* out_min_y,
    float* out_max_x,
    float* out_max_y) const
{
    std::lock_guard<std::mutex> lg(guard);
    return get_bounds_locked(out_min_x, out_min_y, out_max_x, out_max_y);
}

// Differs in name as not dictated by android
bool mia::PointerController::get_bounds_locked(
    float *out_min_x,
    float* out_min_y,
    float* out_max_x,
    float* out_max_y) const
{
    auto bounds = input_region->bounding_rectangle();
    *out_min_x = bounds.top_left.x.as_float();
    *out_min_y = bounds.top_left.y.as_float();
    *out_max_x = bounds.top_left.x.as_float() + bounds.size.width.as_float();
    *out_max_y = bounds.top_left.y.as_float() + bounds.size.height.as_float();
    return true;
}

void mia::PointerController::move(float delta_x, float delta_y)
{
    auto new_x = x + delta_x;
    auto new_y = y + delta_y;
    setPosition(new_x, new_y);
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

    geom::Point p{new_x, new_y};
    input_region->confine(p);

    x = p.x.as_float();
    y = p.y.as_float();

    // I think it's correct to hold this lock while notifying the listener (i.e. cursor rendering update)
    // to prevent the InputReader from getting ahead of rendering. This may need to be thought about later.
    notify_listener();
}
void mia::PointerController::getPosition(float *out_x, float *out_y) const
{
    std::lock_guard<std::mutex> lg(guard);
    *out_x = x;
    *out_y = y;
}

void mia::PointerController::setSpots(const droidinput::PointerCoords* spot_coords, uint32_t spot_count)
{
    std::vector<mi::TouchVisualizer::Spot> touches;
    for (uint32_t i = 0; i < spot_count; i++)
    {
        droidinput::PointerCoords const& coords = spot_coords[i];
        
        auto x = coords.getX();
        auto y = coords.getY();
        auto pressure = coords.getAxisValue(AMOTION_EVENT_AXIS_PRESSURE);
        
        touches.push_back({{x, y}, pressure});
    }
    
    touch_visualizer->visualize_touches(touches);
}

void mia::PointerController::clearSpots()
{
    touch_visualizer->visualize_touches({});
}
