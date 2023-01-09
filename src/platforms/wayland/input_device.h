/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_INPUT_WAYLAND_INPUT_DEVICE_H_
#define MIR_INPUT_WAYLAND_INPUT_DEVICE_H_

#include "display_input.h"

#include <mir/events/event_builders.h>
#include <mir/input/input_device.h>
#include <mir/input/input_device_info.h>

#include <xkbcommon/xkbcommon.h>

#include <chrono>
#include <functional>
#include <mutex>

typedef unsigned int MirPointerButtons;

namespace mir
{
namespace dispatch
{
class ActionQueue;
}
namespace input
{
namespace wayland
{

class GenericInputDevice : public InputDevice
{
protected:

    GenericInputDevice(InputDeviceInfo const& info, std::shared_ptr<dispatch::ActionQueue> const& action_queue);

    void start(InputSink* destination, EventBuilder* builder) override;

    void stop() override;

    InputDeviceInfo get_device_info() override;

    optional_value<PointerSettings> get_pointer_settings() const override;

    void apply_settings(PointerSettings const& settings) override;

    optional_value<TouchpadSettings> get_touchpad_settings() const override;

    void apply_settings(TouchpadSettings const& settings) override;

    virtual optional_value<TouchscreenSettings> get_touchscreen_settings() const override;

    virtual void apply_settings(TouchscreenSettings const&) override;

    void enqueue(std::function<EventUPtr(EventBuilder* builder)> const& event);

private:
    bool started() const;

    InputDeviceInfo const info;
    std::shared_ptr<dispatch::ActionQueue> const action_queue;

    std::mutex mutable mutex;
    InputSink* sink{nullptr};
    EventBuilder* builder{nullptr};
};

class KeyboardInputDevice : public GenericInputDevice, public KeyboardInput
{
public:
    explicit KeyboardInputDevice(std::shared_ptr<dispatch::ActionQueue> const& action_queue);

private:
    void key_press(std::chrono::nanoseconds event_time, xkb_keysym_t keysym, int32_t scan_code) override;
    void key_release(std::chrono::nanoseconds event_time, xkb_keysym_t keysym, int32_t scan_code) override;
};

class PointerInputDevice : public GenericInputDevice, public PointerInput
{
public:
    PointerInputDevice(std::shared_ptr<dispatch::ActionQueue> const& action_queue);

private:
    MirPointerButtons button_state{0};
    geometry::PointF cached_pos;

    void pointer_press(
        std::chrono::nanoseconds event_time,
        int button,
        geometry::PointF const& pos,
        geometry::DisplacementF const& scroll) override;
    void pointer_release(
        std::chrono::nanoseconds event_time,
        int button,
        geometry::PointF const& pos,
        geometry::DisplacementF const& scroll) override;
    void pointer_motion(
        std::chrono::nanoseconds event_time,
        geometry::PointF const& pos,
        geometry::DisplacementF const& scroll) override;

public:
    void pointer_axis_motion(
        MirPointerAxisSource pointer_axis_source, std::chrono::nanoseconds event_time, geometry::PointF const& pos,
        geometry::DisplacementF const& scroll) override;
};

class TouchInputDevice : public GenericInputDevice, public TouchInput
{
public:
    TouchInputDevice(std::shared_ptr<dispatch::ActionQueue> const& action_queue);

private:
    void touch_event(std::chrono::nanoseconds event_time, std::vector<events::TouchContact> const& contacts) override;
};
}
}
}

#endif // MIR_INPUT_WAYLAND_INPUT_DEVICE_H_
