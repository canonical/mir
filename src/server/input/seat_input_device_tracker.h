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

#ifndef MIR_INPUT_SEAT_INPUT_DEVICE_TRACKER_H
#define MIR_INPUT_SEAT_INPUT_DEVICE_TRACKER_H

#include "mir/input/touch_visualizer.h"
#include "mir/geometry/point.h"
#include "mir/geometry/rectangles.h"
#include "mir_toolkit/event.h"
#include <unordered_map>
#include <memory>

namespace mir
{
namespace input
{
class CursorListener;
class InputRegion;
class InputDispatcher;

/*
 * The SeatInputDeviceTracker bundles the input device properties of a group of devices defined by a seat:
 *  - a single cursor position,
 *  - modifier key states (i.e alt, ctrl ..)
 *  - a single mouse button state for all pointing devices
 *  - visible touch spots
 */
class SeatInputDeviceTracker
{
public:
    SeatInputDeviceTracker(std::shared_ptr<InputDispatcher> const& dispatcher,
                           std::shared_ptr<TouchVisualizer> const& touch_visualizer,
                           std::shared_ptr<CursorListener> const& cursor_listener,
                           std::shared_ptr<InputRegion> const& input_region);
    void add_device(MirInputDeviceId);
    void remove_device(MirInputDeviceId);

    void dispatch(MirEvent & event);

    MirPointerButtons button_state() const;
    geometry::Point cursor_position() const;
    MirInputEventModifiers event_modifier() const;
    MirInputEventModifiers event_modifier(MirInputDeviceId) const;

    void set_confinement_regions(geometry::Rectangles const& region);
    void reset_confinement_regions();
private:
    void update_seat_properties(MirInputEvent const* event);
    void update_cursor(MirPointerEvent const* event);
    void update_spots();
    void update_states();

    void confine_pointer(std::function<void(mir::geometry::Point&)> const& confine);

    std::shared_ptr<InputDispatcher> const dispatcher;
    std::shared_ptr<TouchVisualizer> const touch_visualizer;
    std::shared_ptr<CursorListener> const cursor_listener;
    std::shared_ptr<InputRegion> const input_region;

    struct DeviceData
    {
        DeviceData() {}
        bool update_modifier(MirKeyboardAction action, int scan_code);
        bool update_button_state(MirPointerButtons button_state);
        bool update_spots(MirTouchEvent const* event);

        MirInputEventModifiers mod{0};
        MirPointerButtons buttons{0};
        std::vector<TouchVisualizer::Spot> spots;
    };

    // Libinput's acceleration curve means the cursor moves by non-integer
    // increments, and often less than 1.0, so float is required...
    float cursor_x = 0.0f, cursor_y = 0.0f;

    MirInputEventModifiers modifier;
    MirPointerButtons buttons;
    std::unordered_map<MirInputDeviceId, DeviceData> device_data;
    std::vector<TouchVisualizer::Spot> spots;
    geometry::Rectangles pointer_confinment_regions;
};

}
}

#endif
