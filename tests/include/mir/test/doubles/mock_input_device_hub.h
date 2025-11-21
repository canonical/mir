/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_DEVICE_HUB_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_DEVICE_HUB_H_

#include "mir/input/input_device_hub.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockInputDeviceHub : input::InputDeviceHub
{
    MOCK_METHOD(void, add_observer, (std::shared_ptr<input::InputDeviceObserver> const&), (override));
    MOCK_METHOD(void, remove_observer, (std::weak_ptr<input::InputDeviceObserver> const&), (override));
    MOCK_METHOD(void, for_each_input_device, (std::function<void(input::Device const&)> const&), (override));
    MOCK_METHOD(void, for_each_mutable_input_device, (std::function<void(input::Device&)> const&), (override));
};

}
}
}

#endif
