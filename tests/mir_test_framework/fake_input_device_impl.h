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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_FRAMEWORK_FAKE_INPUT_DEVICE_IMPL_H_
#define MIR_TEST_FRAMEWORK_FAKE_INPUT_DEVICE_IMPL_H_

#include "mir_test_framework/fake_input_device.h"

#include "mir/input/input_device.h"
#include "mir/input/pointer_settings.h"
#include "mir/input/touchscreen_settings.h"
#include "mir/input/input_device_info.h"
#include "mir/geometry/point.h"

#include <mutex>
#include <functional>
#include <vector>

namespace mir
{
namespace input
{
class OutputInfo;
}
namespace dispatch
{
class ActionQueue;
}
}

namespace mir_test_framework
{
class FakeInputDeviceImpl : public FakeInputDevice
{
public:
    FakeInputDeviceImpl(mir::input::InputDeviceInfo const& info);
    void emit_device_removal() override;
    void emit_runtime_error() override;
    void emit_event(mir::input::synthesis::KeyParameters const& key_params) override;
    void emit_event(mir::input::synthesis::ButtonParameters const& button) override;
    void emit_event(mir::input::synthesis::MotionParameters const& motion) override;
    void emit_event(mir::input::synthesis::TouchParameters const& touch) override;
    void emit_touch_sequence(std::function<mir::input::synthesis::TouchParameters(int)> const& event_generator,
                             int count,
                             std::chrono::duration<double> delay) override;
    void emit_key_state(std::vector<uint32_t> const& key_syms) override;
    virtual void on_new_configuration_do(std::function<void(mir::input::InputDevice const& device)> callback) override;

private:
    class InputDevice : public mir::input::InputDevice
    {
    public:
        InputDevice(mir::input::InputDeviceInfo const& info,
                    std::shared_ptr<mir::dispatch::Dispatchable> const& dispatchable);

        void start(mir::input::InputSink* destination, mir::input::EventBuilder* builder) override;
        void stop() override;

        void synthesize_events(mir::input::synthesis::KeyParameters const& key_params);
        void synthesize_events(mir::input::synthesis::ButtonParameters const& button);
        void synthesize_events(mir::input::synthesis::MotionParameters const& motion);
        void synthesize_events(mir::input::synthesis::TouchParameters const& touch);

        void emit_key_state(std::vector<uint32_t> const& scan_codes);
        mir::input::InputDeviceInfo get_device_info() override
        {
            return info;
        }

        mir::optional_value<mir::input::PointerSettings> get_pointer_settings() const override;
        void apply_settings(mir::input::PointerSettings const& settings) override;
        mir::optional_value<mir::input::TouchpadSettings> get_touchpad_settings() const override;
        void apply_settings(mir::input::TouchpadSettings const& settings) override;
        mir::optional_value<mir::input::TouchscreenSettings> get_touchscreen_settings() const override;
        void apply_settings(mir::input::TouchscreenSettings const& settings) override;
        void set_apply_settings_callback(std::function<void(mir::input::InputDevice const&)> const& callback);

    private:
        MirPointerAction update_buttons(mir::input::synthesis::EventAction action, MirPointerButton button);
        void update_position(int rel_x, int rel_y);
        void map_touch_coordinates(float& x, float& y);
        mir::input::OutputInfo get_output_info() const;
        bool is_output_active() const;
        void trigger_callback() const;

        mir::input::InputSink* sink{nullptr};
        mir::input::EventBuilder* builder{nullptr};
        mir::input::InputDeviceInfo info;
        std::shared_ptr<mir::dispatch::Dispatchable> const queue;
        mir::geometry::Point pos, scroll;
        MirPointerButtons buttons;
        mir::input::PointerSettings settings;
        mir::input::TouchscreenSettings touchscreen;
        mutable std::mutex config_callback_mutex;
        std::function<void(mir::input::InputDevice const&)> callback;
    };
    std::shared_ptr<mir::dispatch::ActionQueue> queue;
    std::shared_ptr<InputDevice> device;
};

}

#endif
