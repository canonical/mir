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

#ifndef MIR_TEST_DOUBLES_MOCK_DEVICE_H_
#define MIR_TEST_DOUBLES_MOCK_DEVICE_H_

#include "mir/input/device.h"
#include "mir/input/device_capability.h"
#include "mir/input/mir_pointer_config.h"
#include "mir/input/mir_keyboard_config.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/input/mir_touchscreen_config.h"
#include "mir/optional_value.h"

#include <string>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockDevice : input::Device
{
    MockDevice(MirInputDeviceId id, input::DeviceCapabilities caps, std::string const& name, std::string const& unique_id)
    {
        ON_CALL(*this, id()).WillByDefault(testing::Return(id));
        ON_CALL(*this, name()).WillByDefault(testing::Return(name));
        ON_CALL(*this, unique_id()).WillByDefault(testing::Return(unique_id));
        ON_CALL(*this, capabilities()).WillByDefault(testing::Return(caps));
        ON_CALL(*this, pointer_configuration()).WillByDefault(testing::Return(MirPointerConfig{}));
        ON_CALL(*this, keyboard_configuration()).WillByDefault(testing::Return(MirKeyboardConfig{}));
        ON_CALL(*this, touchpad_configuration()).WillByDefault(testing::Return(MirTouchpadConfig{}));
        ON_CALL(*this, touchscreen_configuration()).WillByDefault(testing::Return(MirTouchscreenConfig{}));
    }

    MOCK_METHOD(MirInputDeviceId, id, (), (const));
    MOCK_METHOD(input::DeviceCapabilities, capabilities, (), (const));
    MOCK_METHOD(std::string, name, (), (const));
    MOCK_METHOD(std::string, unique_id, (), (const));
    MOCK_METHOD(optional_value<MirPointerConfig>, pointer_configuration, (), (const));
    MOCK_METHOD(optional_value<MirTouchpadConfig>, touchpad_configuration, (), (const));
    MOCK_METHOD(optional_value<MirKeyboardConfig>, keyboard_configuration, (), (const));
    MOCK_METHOD(optional_value<MirTouchscreenConfig>, touchscreen_configuration, (), (const));
    MOCK_METHOD(void, apply_pointer_configuration, (MirPointerConfig const&), (override));
    MOCK_METHOD(void, apply_touchpad_configuration, (MirTouchpadConfig const&), (override));
    MOCK_METHOD(void, apply_keyboard_configuration, (MirKeyboardConfig const&), (override));
    MOCK_METHOD(void, apply_touchscreen_configuration, (MirTouchscreenConfig const&), (override));
};
}
}
}

#endif
