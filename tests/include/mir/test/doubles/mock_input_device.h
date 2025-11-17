/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_DEVICE_H
#define MIR_TEST_DOUBLES_MOCK_INPUT_DEVICE_H

#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h" // needed for fake device setup
#include "mir/input/pointer_settings.h"
#include "mir/input/touchpad_settings.h"
#include "mir/input/touchscreen_settings.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockInputDevice : input::InputDevice
{
    MockInputDevice() = default;
    MockInputDevice(char const* name, char const* uid, input::DeviceCapabilities);

    MOCK_METHOD(void, start, (input::InputSink* destination, input::EventBuilder* builder), ());
    MOCK_METHOD(void, stop, (), ());
    MOCK_METHOD(input::InputDeviceInfo, get_device_info, (), ());
    MOCK_METHOD(mir::optional_value<input::PointerSettings>, get_pointer_settings, (), (const));
    MOCK_METHOD(void, apply_settings, (input::PointerSettings const&), ());
    MOCK_METHOD(mir::optional_value<input::TouchpadSettings>, get_touchpad_settings, (), (const));
    MOCK_METHOD(void, apply_settings, (input::TouchpadSettings const&), ());
    MOCK_METHOD(mir::optional_value<input::TouchscreenSettings>, get_touchscreen_settings, (), (const));
    MOCK_METHOD(void, apply_settings, (input::TouchscreenSettings const&), ());
};
}
}
}

#endif
