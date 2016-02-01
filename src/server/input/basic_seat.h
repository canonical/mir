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

#ifndef MIR_BASIC_SEAT_H_
#define MIR_BASIC_SEAT_H_

#include "mir/input/seat.h"
#include "mir/frontend/event_sink.h"
#include "seat_input_device_tracker.h"

#include <mutex>

namespace mir
{
namespace input
{
class TouchVisualizer;
class CursorListener;
class InputRegion;
class InputDispatcher;

class BasicSeat : public Seat
{
public:
    BasicSeat(std::shared_ptr<frontend::EventSink> const& event_sink,
              std::shared_ptr<InputDispatcher> const& dispatcher,
              std::shared_ptr<TouchVisualizer> const& touch_visualizer,
              std::shared_ptr<CursorListener> const& cursor_listener,
              std::shared_ptr<InputRegion> const& input_region);
    // Seat methods:
    void add_device(std::shared_ptr<Device> const& device) override;
    void remove_device(std::shared_ptr<Device> const& device) override;
    void dispatch_event(MirEvent& event) override;
    geometry::Rectangle get_rectangle_for(std::shared_ptr<Device> const& dev) override;
    void for_each_input_device(std::function<void(std::shared_ptr<Device>const& dev)> const& callback) override;

private:
    std::mutex devices_guard;
    std::vector<std::shared_ptr<Device>> devices;
    SeatInputDeviceTracker input_state_tracker;
    std::shared_ptr<InputRegion> const input_region;
    std::shared_ptr<frontend::EventSink> const event_sink;
};
}
}

#endif
