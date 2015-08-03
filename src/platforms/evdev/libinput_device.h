/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_EVDEV_LIBINPUT_DEVICE_H_
#define MIR_INPUT_EVDEV_LIBINPUT_DEVICE_H_

#include "libinput_ptr.h"
#include "libinput_device_ptr.h"
#include "mir/input/input_device.h"
#include "mir/geometry/point.h"

#include "mir_toolkit/event.h"
#include "mir/events/event_builders.h"

struct libinput_event;
struct libinput_event_keyboard;
struct libinput_event_touch;
struct libinput_event_pointer;

namespace mir
{
namespace input
{
class InputReport;
namespace evdev
{
struct PointerState;
struct KeyboardState;

class LibInputDevice : public input::InputDevice
{
public:
    LibInputDevice(std::shared_ptr<InputReport> const& report, LibInputPtr lib, char const* path);
    ~LibInputDevice();
    std::shared_ptr<dispatch::Dispatchable> dispatchable() override;
    void start(InputSink* sink) override;
    void stop() override;
    virtual InputDeviceInfo get_device_info() override;

    void process_event(libinput_event* event);
    ::libinput_device* device();
private:
    EventUPtr convert_event(libinput_event_keyboard* keyboard);
    EventUPtr convert_button_event(libinput_event_pointer* pointer);
    EventUPtr convert_motion_event(libinput_event_pointer* pointer);
    EventUPtr convert_absolute_motion_event(libinput_event_pointer* pointer);
    EventUPtr convert_axis_event(libinput_event_pointer* pointer);
    void add_touch_down_event(libinput_event_touch* touch);
    void add_touch_up_event(libinput_event_touch* touch);
    void add_touch_motion_event(libinput_event_touch* touch);
    MirEvent& get_accumulated_touch_event(std::chrono::nanoseconds timestamp);

    std::shared_ptr<InputReport> report;
    LibInputPtr lib;
    std::string path;
    LibInputDevicePtr dev;
    std::shared_ptr<dispatch::Dispatchable> dispatchable_fd;

    InputSink *sink{nullptr};
    EventUPtr accumulated_touch_event;

    mir::geometry::Point pointer_pos;
    MirInputEventModifiers modifier_state;
    MirPointerButtons button_state;
};
}
}
}

#endif
