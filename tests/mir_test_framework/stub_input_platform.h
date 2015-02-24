/*
 * Copyright © 2014-2015 Canonical Ltd.
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
#ifndef MIR_TEST_FRAMEWORK_STUB_INPUT_PLATFORM_H_
#define MIR_TEST_FRAMEWORK_STUB_INPUT_PLATFORM_H_

#include "mir/input/platform.h"
#include <mutex>
#include <memory>
#include <vector>

namespace mir
{
namespace input
{
class InputDevice;
class InputDeviceInfo;
}
}
namespace mir_test_framework
{
class FakeInputDevice;
class StubInputPlatform : public mir::input::Platform
{
public:
    void start(mir::input::InputEventHandlerRegister& loop,
               std::shared_ptr<mir::input::InputDeviceRegistry> const& input_device_registry);
    void stop(mir::input::InputEventHandlerRegister& loop) override;

    static void add(std::shared_ptr<mir::input::InputDevice> const& dev);
    static void remove(std::shared_ptr<mir::input::InputDevice> const& dev);
    static std::mutex platform_mutex;
    static std::vector<std::shared_ptr<mir::input::InputDevice>> registered_devs;
    static std::shared_ptr<mir::input::InputDeviceRegistry> registry;
    static mir::input::InputEventHandlerRegister* event_handler;
};

}

#endif
