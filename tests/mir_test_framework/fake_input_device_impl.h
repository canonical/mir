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

#include <mutex>
#include <map>

namespace mi = mir::input;
namespace mir_test_framework
{
class FakeInputDeviceImpl : public FakeInputDevice
{
public:
    FakeInputDeviceImpl(mi::InputDeviceInfo const& info);
    ~FakeInputDeviceImpl();
    void emit_event(synthesis::KeyParameters const& key_params) override;
private:
    class InputDevice : public mi::InputDevice
    {
    public:
        InputDevice(mi::InputDeviceInfo const& info);
        void start(mi::InputEventHandlerRegister& registry, mi::InputSink& destination) override;
        void stop(mi::InputEventHandlerRegister& registry) override;

        // implemented as template to soon also slope through the other types like ButtonParameters, MotionParameter, ...
        template<typename T>
        void emit_event(T const& parameters);
        void synthesize_events(synthesis::KeyParameters const& key_params);
        mi::InputDeviceInfo get_device_info() override
        {
            return info;
        }

        std::mutex mutex;
        mi::InputSink* sink{nullptr};
        mi::InputEventHandlerRegister* event_handler{nullptr};
        mi::InputDeviceInfo info;

        uint32_t modifiers; // not unified accross devices, yet
        std::map<int32_t, std::chrono::nanoseconds> down_times;
    };
    std::shared_ptr<InputDevice> device;
};

}

#endif
