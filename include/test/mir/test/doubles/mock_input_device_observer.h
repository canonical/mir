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
#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_DEVICE_OBSERVER_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_DEVICE_OBSERVER_H_

#include "mir/input/input_device_observer.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockInputDeviceObserver : input::InputDeviceObserver
{
    MOCK_METHOD1(device_added, void(std::shared_ptr<input::Device> const& device));
    MOCK_METHOD1(device_changed, void(std::shared_ptr<input::Device> const& device));
    MOCK_METHOD1(device_removed, void(std::shared_ptr<input::Device> const& device));
    MOCK_METHOD0(changes_complete, void());
};

}
}
}

#endif
