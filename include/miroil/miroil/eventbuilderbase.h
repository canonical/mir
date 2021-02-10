/*
 * Copyright (C) 2016-2017 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIROIL_EVENT_BUILDER_BASE_H
#define MIROIL_EVENT_BUILDER_BASE_H
#include <miral/toolkit_event.h>
#include <mir_toolkit/mir_input_device_types.h>
#include <chrono>
#include <memory>
#include <sys/types.h>
#include <vector>

struct MirEvent;
struct MirInputEvent;

namespace mir { typedef std::unique_ptr<MirEvent, void(*)(MirEvent*)> EventUPtr; }

namespace miroil
{
    
/*
    Creates Mir input events out of Qt input events
    
    The class is splitt into miroil::EventBuilderBase which does the internal mir stuff, 
    and qtmir::EventBuilder which handles the qt stuff. 

    One important feature is that it's able to match a QInputEvent with the MirInputEvent that originated it, so
    it can make a MirInputEvent version of a QInputEvent containing also information that the latter does not carry,
    such as relative axis movement for pointer devices.
*/
    
class EventBuilderBase {
    
public:
    class EventInfo {
    public:
        void store(const MirInputEvent *mirInputEvent, ulong qtTimestamp);
        
        ulong qtTimestamp;
        MirInputDeviceId deviceId;
        std::vector<uint8_t> cookie;
        float relativeX{0};
        float relativeY{0};
    };
    
protected:    
    void addTouch(MirEvent &event, MirTouchId touch_id, MirTouchAction action,
        MirTouchTooltype tooltype, float x_axis_value, float y_axis_value,
        float pressure_value, float touch_major_value, float touch_minor_value, float size_value);
    
    // Key event
    mir::EventUPtr makeEvent(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
        std::vector<uint8_t> const& cookie, MirKeyboardAction action, xkb_keysym_t key_code,
        int scan_code, MirInputEventModifiers modifiers);

    // Touch event
    mir::EventUPtr makeEvent(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
        std::vector<uint8_t> const& mac, MirInputEventModifiers modifiers);

    // Pointer event
    mir::EventUPtr makeEvent(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
        std::vector<uint8_t> const& mac, MirInputEventModifiers modifiers, MirPointerAction action,
        MirPointerButtons buttons_pressed,
        float x_axis_value, float y_axis_value,
        float hscroll_value, float vscroll_value,
        float relative_x_value, float relative_y_value);
};

}

#endif // MIROIL_EVENT_BUILDER_BASE_H
