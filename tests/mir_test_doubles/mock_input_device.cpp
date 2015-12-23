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

#include "mir/test/doubles/mock_input_device.h"
#include "mir/input/device_capability.h"
#include "mir/input/pointer_settings.h"
#include "mir/input/touchpad_settings.h"

namespace mtd = mir::test::doubles;

mtd::MockInputDevice::MockInputDevice(char const* name, char const* uid, input::DeviceCapabilities caps)
{
    using namespace testing;
    ON_CALL(*this, get_device_info())
        .WillByDefault(Return(input::InputDeviceInfo{name, uid, caps}));
    if (contains(caps, input::DeviceCapability::pointer))
        ON_CALL(*this, get_pointer_settings())
            .WillByDefault(Return(input::PointerSettings()));
    else
        ON_CALL(*this, get_pointer_settings())
            .WillByDefault(Return(mir::optional_value<input::PointerSettings>()));
    if (contains(caps, input::DeviceCapability::touchpad))
        ON_CALL(*this, get_touchpad_settings())
            .WillByDefault(Return(input::TouchpadSettings()));
    else
        ON_CALL(*this, get_touchpad_settings())
            .WillByDefault(Return(mir::optional_value<input::TouchpadSettings>()));
}
