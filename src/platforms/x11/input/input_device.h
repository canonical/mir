/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_INPUT_X_INPUT_DEVICE_H_
#define MIR_INPUT_X_INPUT_DEVICE_H_

#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h"
#include "mir_toolkit/event.h"
#include "mir/geometry/point.h"
#include "mir/geometry/displacement.h"
#include "mir/optional_value.h"

#include <chrono>

namespace mir
{
namespace input
{

namespace X
{

class XInputDevice : public input::InputDevice
{
public:
    XInputDevice(InputDeviceInfo const& info);

    std::shared_ptr<dispatch::Dispatchable> dispatchable();
    void start(InputSink* destination, EventBuilder* builder) override;
    void stop() override;
    InputDeviceInfo get_device_info() override;

    optional_value<PointerSettings> get_pointer_settings() const override;
    void apply_settings(PointerSettings const& settings) override;
    optional_value<TouchpadSettings> get_touchpad_settings() const override;
    void apply_settings(TouchpadSettings const& settings) override;
    virtual optional_value<TouchscreenSettings> get_touchscreen_settings() const override;
    virtual void apply_settings(TouchscreenSettings const&) override;

    bool started() const;
    void key_press(std::chrono::nanoseconds event_time, xkb_keysym_t key_sym, int32_t key_code);
    void key_release(std::chrono::nanoseconds event_time, xkb_keysym_t key_sym, int32_t key_code);
    void update_button_state(int button);
    void pointer_press(std::chrono::nanoseconds event_time, int button, mir::geometry::Point const& pos, mir::geometry::Displacement scroll);
    void pointer_release(std::chrono::nanoseconds event_time, int button, mir::geometry::Point const& pos, mir::geometry::Displacement scroll);
    void pointer_motion(std::chrono::nanoseconds event_time, mir::geometry::Point const& pos, mir::geometry::Displacement scroll);

private:
    MirPointerButtons button_state{0};
    InputSink* sink{nullptr};
    EventBuilder* builder{nullptr};
    geometry::Point pointer_pos;
    InputDeviceInfo info;
};

}

}
}

#endif // MIR_INPUT_X_INPUT_DEVICE_H_
