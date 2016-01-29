/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "basic_seat.h"
#include "mir/input/device.h"
#include "mir/input/input_region.h"

#include <algorithm>

namespace mi = mir::input;

mi::BasicSeat::BasicSeat(std::shared_ptr<mi::InputDispatcher> const& dispatcher,
                          std::shared_ptr<mi::TouchVisualizer> const& touch_visualizer,
                          std::shared_ptr<mi::CursorListener> const& cursor_listener,
                          std::shared_ptr<mi::InputRegion> const& input_region)
    : input_state_tracker{dispatcher, touch_visualizer, cursor_listener, input_region}, input_region{input_region}
{
}

void mi::BasicSeat::add_device(std::shared_ptr<input::Device> const& device)
{
    input_state_tracker.add_device(device->id());
    devices.push_back(device);
}

void mi::BasicSeat::remove_device(std::shared_ptr<input::Device> const& device)
{
    devices.erase(std::remove(begin(devices), end(devices), device));
    input_state_tracker.remove_device(device->id());
}

void mi::BasicSeat::dispatch_event(MirEvent& event)
{
    input_state_tracker.dispatch(event);
}

mir::geometry::Rectangle mi::BasicSeat::get_rectangle_for(std::shared_ptr<input::Device> const&)
{
    // TODO: With knowledge of the outputs attached to this seat this method
    // should return the outputs rectangle instead of the rectangle of the
    // first output
    return input_region->bounding_rectangle();
}
