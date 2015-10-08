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

#include "mir/input/event_builder.h"
#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h"
#include "mir/geometry/point.h"

#include <vector>
#include <unordered_map>

struct libinput_event;
struct libinput_event_keyboard;
struct libinput_event_touch;
struct libinput_event_pointer;
struct libinput_device_group;

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
    LibInputDevice(std::shared_ptr<InputReport> const& report, char const* path, LibInputDevicePtr dev);
    ~LibInputDevice();
    void start(InputSink* sink, EventBuilder* builder) override;
    void stop() override;
    InputDeviceInfo get_device_info() override;

    void process_event(libinput_event* event);
    ::libinput_device* device() const;
    ::libinput_device_group* group();
    void add_device_of_group(char const* path, LibInputDevicePtr ptr);
    bool is_in_group(char const* path);
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
    void update_device_info();

    std::shared_ptr<InputReport> report;
    std::shared_ptr<::libinput> lib;
    std::vector<std::string> paths;
    std::vector<LibInputDevicePtr> devices;
    std::shared_ptr<dispatch::Dispatchable> dispatchable_fd;

    InputSink* sink{nullptr};
    EventBuilder* builder{nullptr};
    EventUPtr accumulated_touch_event;

    InputDeviceInfo info;
    mir::geometry::Point pointer_pos;
    MirInputEventModifiers modifier_state;
    MirPointerButtons button_state;

    struct ContactData
    {
        ContactData() {}
        float x{0}, y{0}, major{0}, minor{0}, pressure{0};
    };
    std::unordered_map<MirTouchId,ContactData> last_seen_properties;

    void update_contact_data(ContactData &data, libinput_event_touch* touch);
};
}
}
}

#endif
