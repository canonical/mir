/*
 * Copyright © 2017 Canonical Ltd.
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
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
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

    MOCK_CONST_METHOD0(id, MirInputDeviceId());
    MOCK_CONST_METHOD0(capabilities, input::DeviceCapabilities());
    MOCK_CONST_METHOD0(name, std::string());
    MOCK_CONST_METHOD0(unique_id, std::string());
    MOCK_CONST_METHOD0(pointer_configuration, optional_value<MirPointerConfig>());
    MOCK_CONST_METHOD0(touchpad_configuration, optional_value<MirTouchpadConfig>());
    MOCK_CONST_METHOD0(keyboard_configuration, optional_value<MirKeyboardConfig>());
    MOCK_CONST_METHOD0(touchscreen_configuration, optional_value<MirTouchscreenConfig>());
    MOCK_METHOD1(apply_pointer_configuration, void(MirPointerConfig const&));
    MOCK_METHOD1(apply_touchpad_configuration, void(MirTouchpadConfig const&));
    MOCK_METHOD1(apply_keyboard_configuration, void(MirKeyboardConfig const&));
    MOCK_METHOD1(apply_touchscreen_configuration, void(MirTouchscreenConfig const&));
};
}
}
}

#endif

