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

#ifndef MIR_INPUT_DEFAULT_SEAT_H_
#define MIR_INPUT_DEFAULT_SEAT_H_

#include "mir/input/touch_visualizer.h"
#include "mir_toolkit/event.h"
#include <unordered_map>
#include <memory>

namespace mir
{
namespace input
{
class CursorListener;

/*
 * The seat bundles a group of devices. A cursor position, input event modifiers and the visible touch spots are properties
 * controlled by this grouping of input devices.
 */
class Seat
{
public:
    Seat(std::shared_ptr<TouchVisualizer> const& touch_visualizer, std::shared_ptr<CursorListener> const& cursor_listener);
    void add_device(MirInputDeviceId);
    void remove_device(MirInputDeviceId);

    MirInputEventModifiers event_modifier() const;
    MirInputEventModifiers event_modifier(MirInputDeviceId) const;
    void update_seat_properties(MirInputEvent const* event);
private:
    void update_cursor(MirPointerEvent const* event);
    void update_spots();
    void update_modifier();

    std::shared_ptr<TouchVisualizer> const touch_visualizer;
    std::shared_ptr<CursorListener> const cursor_listener;

    struct DeviceData
    {
        DeviceData() {}
        bool update_modifier(MirKeyboardAction action, int scan_code);
        bool update_spots(MirTouchEvent const* event);

        MirInputEventModifiers mod{0};
        std::vector<TouchVisualizer::Spot> spots;
    };

    MirInputEventModifiers modifier;
    std::unordered_map<MirInputDeviceId, DeviceData> device_data;
    std::vector<TouchVisualizer::Spot> spots;
};

}
}

#endif
