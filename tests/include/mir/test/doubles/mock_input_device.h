/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_DEVICE_H
#define MIR_TEST_DOUBLES_MOCK_INPUT_DEVICE_H

#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h" // needed for fake device setup
#include "mir/input/pointer_settings.h"
#include "mir/input/touchpad_settings.h"

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

    MOCK_METHOD2(start, void(input::InputSink* destination, input::EventBuilder* builder));
    MOCK_METHOD0(stop, void());
    MOCK_METHOD0(get_device_info, input::InputDeviceInfo());
    MOCK_CONST_METHOD0(get_pointer_settings, mir::optional_value<input::PointerSettings>());
    MOCK_METHOD1(apply_settings, void(input::PointerSettings const&));
    MOCK_CONST_METHOD0(get_touchpad_settings, mir::optional_value<input::TouchpadSettings>());
    MOCK_METHOD1(apply_settings, void(input::TouchpadSettings const&));
};
}
}
}

#endif
