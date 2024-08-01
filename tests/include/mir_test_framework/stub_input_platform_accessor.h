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

#ifndef MIR_TEST_FRAMEWORK_STUB_INPUT_PLATFORM_ACCESSOR_H
#define MIR_TEST_FRAMEWORK_STUB_INPUT_PLATFORM_ACCESSOR_H

#include "stub_input_platform.h"

namespace mir_test_framework
{
class StubInputPlatformAccessor
{
public:
    static mir::UniqueModulePtr<mir::input::Platform> get(std::shared_ptr<mir::input::InputDeviceRegistry> const& input_device_registry);
    static void add(std::shared_ptr<mir::input::InputDevice> const& dev);
    static void remove(std::shared_ptr<mir::input::InputDevice> const& dev);
    static void register_dispatchable(std::shared_ptr<mir::dispatch::Dispatchable> const& queue);
    static void unregister_dispatchable(std::shared_ptr<mir::dispatch::Dispatchable> const& queue);
    static void clear();

private:
    static std::atomic<StubInputPlatform*> stub_input_platform;

};
}

#endif //MIR_TEST_FRAMEWORK_STUB_INPUT_PLATFORM_ACCESSOR_H
