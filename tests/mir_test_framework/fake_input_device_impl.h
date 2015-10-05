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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_TEST_FRAMEWORK_FAKE_INPUT_DEVICE_IMPL_H_
#define MIR_TEST_FRAMEWORK_FAKE_INPUT_DEVICE_IMPL_H_

#include "mir_test_framework/fake_input_device.h"

#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h"
#include "mir/geometry/point.h"

namespace mir
{
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
    void emit_runtime_error() override;
    void emit_event(synthesis::KeyParameters const& key_params) override;
    void emit_event(synthesis::ButtonParameters const& button) override;
    void emit_event(synthesis::MotionParameters const& motion) override;
    void emit_event(synthesis::TouchParameters const& touch) override;

private:
    class InputDevice : public mir::input::InputDevice
    {
    public:
        InputDevice(mir::input::InputDeviceInfo const& info,
                    std::shared_ptr<mir::dispatch::Dispatchable> const& dispatchable);

        void start(mir::input::InputSink* destination, mir::input::EventBuilder* builder) override;
        void stop() override;

        void synthesize_events(synthesis::KeyParameters const& key_params);
        void synthesize_events(synthesis::ButtonParameters const& button);
        void synthesize_events(synthesis::MotionParameters const& motion);
        void synthesize_events(synthesis::TouchParameters const& touch);
        mir::input::InputDeviceInfo get_device_info() override
        {
            return info;
        }

    private:
        MirPointerAction update_buttons(synthesis::EventAction action, MirPointerButton button);
        void update_position(int rel_x, int rel_y);
        void map_touch_coordinates(float& x, float& y);

        mir::input::InputSink* sink{nullptr};
        mir::input::EventBuilder* builder{nullptr};
        mir::input::InputDeviceInfo info;
        std::shared_ptr<mir::dispatch::Dispatchable> const queue;
        uint32_t modifiers{0};
        mir::geometry::Point pos, scroll;
        MirPointerButtons buttons;
    };
    std::shared_ptr<mir::dispatch::ActionQueue> queue;
    std::shared_ptr<InputDevice> device;
};

}

#endif
