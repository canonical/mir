/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_FRAMEWORK_INPUT_DEVICE_FAKER_H_
#define MIR_TEST_FRAMEWORK_INPUT_DEVICE_FAKER_H_

#include <mir/module_deleter.h>

namespace mir { class Server; }
namespace mir { namespace input { class InputDeviceInfo; } }

namespace mir_test_framework
{
struct FakeInputDevice;

class InputDeviceFaker
{
public:
    InputDeviceFaker(mir::Server& server) : server_ref{server} {}

    mir::UniqueModulePtr<FakeInputDevice> add_fake_input_device(mir::input::InputDeviceInfo const& info);

    void wait_for_input_devices();

private:
    int expected_number_of_input_devices = 0;

    mir::Server& server_ref;
};
}

#endif //MIR_TEST_FRAMEWORK_INPUT_DEVICE_FAKER_H_
