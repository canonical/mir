/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_DEVICE_HUB_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_DEVICE_HUB_H_

#include "mir/input/input_device_hub.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct MockInputDeviceHub : input::InputDeviceHub
{
    MOCK_METHOD1(add_observer, void(std::shared_ptr<input::InputDeviceObserver> const&));
    MOCK_METHOD1(remove_observer, void(std::weak_ptr<input::InputDeviceObserver> const&));
    MOCK_METHOD1(for_each_input_device, void(std::function<void(input::Device const&)> const&));

};

}
}
}

#endif
