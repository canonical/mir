/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_SEAT_INPUT_DEVICE_TRACKER_H
#define MIR_INPUT_SEAT_INPUT_DEVICE_TRACKER_H

#include "mir/input/touch_visualizer.h"
#include "mir/geometry/point.h"
#include "mir/geometry/rectangles.h"
#include "mir/geometry/size.h"
#include "mir/optional_value.h"
#include "mir_toolkit/event.h"

#include <unordered_map>
#include <memory>
#include <mutex>

namespace mir
{
using EventUPtr = std::unique_ptr<MirEvent, void(*)(MirEvent*)>;
namespace time
{
class Clock;
}
namespace input
{
class CursorListener;
class InputDispatcher;
class KeyMapper;
class SeatObserver;

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
                           std::shared_ptr<KeyMapper> const& key_mapper,
                           std::shared_ptr<time::Clock> const& clock,
                           std::shared_ptr<SeatObserver> const& observer);
    void add_device(MirInputDeviceId);
    void remove_device(MirInputDeviceId);

    void dispatch(std::shared_ptr<MirEvent> const& event);

    MirPointerButtons button_state() const;
    geometry::Point cursor_position() const;
    EventUPtr create_device_state() const;

    void set_key_state(MirInputDeviceId id, std::vector<uint32_t> const& scan_codes);
    void set_pointer_state(MirInputDeviceId id, MirPointerButtons buttons);
    void set_cursor_position(float cursor_x, float cursor_y);
    void set_confinement_regions(geometry::Rectangles const& region);
    void reset_confinement_regions();

    void update_outputs(geometry::Rectangles const& outputs);
private:
    void update_seat_properties(MirInputEvent const* event);
    void update_cursor(MirPointerEvent const* event);
    void update_spots();
    void update_states();
    bool filter_input_event(MirInputEvent const* event);
    void confine_function(mir::geometry::Point& p) const;
    void confine_pointer();

    std::shared_ptr<InputDispatcher> const dispatcher;
    std::shared_ptr<TouchVisualizer> const touch_visualizer;
    std::shared_ptr<CursorListener> const cursor_listener;
    std::shared_ptr<KeyMapper> const key_mapper;
    std::shared_ptr<time::Clock> const clock;
    std::shared_ptr<SeatObserver> const observer;

    struct DeviceData
    {
        DeviceData() {}
        bool update_button_state(MirPointerButtons button_state);
        bool update_spots(MirTouchEvent const* event);
        void update_scan_codes(MirKeyboardEvent const* event);
        bool allowed_scan_code_action(MirKeyboardEvent const* event) const;

        MirPointerButtons buttons{0};
        std::vector<TouchVisualizer::Spot> spots;
        std::vector<uint32_t> scan_codes;

        MirTouchscreenMappingMode mapping_mode{mir_touchscreen_mapping_mode_to_output};
        mir::optional_value<uint32_t> output_id;
    };

    // Libinput's acceleration curve means the cursor moves by non-integer
    // increments, and often less than 1.0, so float is required...
    float cursor_x = 0.0f, cursor_y = 0.0f;

    MirPointerButtons buttons;
    std::unordered_map<MirInputDeviceId, DeviceData> device_data;
    std::vector<TouchVisualizer::Spot> spots;
    mir::geometry::Rectangles confined_region;

    std::mutex mutable device_state_mutex;
    std::mutex mutable region_mutex;

    std::mutex mutable output_mutex;
    geometry::Rectangles input_region;
};

}
}

#endif
