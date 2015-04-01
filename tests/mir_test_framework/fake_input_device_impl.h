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

namespace mi = mir::input;
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
    FakeInputDeviceImpl(mi::InputDeviceInfo const& info);
    void emit_event(synthesis::KeyParameters const& key_params) override;
private:
    class InputDevice : public mi::InputDevice
    {
    public:
        InputDevice(mi::InputDeviceInfo const& info,
                    std::shared_ptr<mir::dispatch::Dispatchable> const& dispatchable);
        std::shared_ptr<mir::dispatch::Dispatchable> dispatchable() override;
        void start(mi::InputSink* destination) override;
        void stop() override;

        void synthesize_events(synthesis::KeyParameters const& key_params);
        mi::InputDeviceInfo get_device_info() override
        {
            return info;
        }

        mi::InputSink* sink{nullptr};
        mi::InputDeviceInfo info;
        std::shared_ptr<mir::dispatch::Dispatchable> const queue;
        uint32_t modifiers{0};
    };
    std::shared_ptr<mir::dispatch::ActionQueue> queue;
    std::shared_ptr<InputDevice> device;
};

}

#endif
