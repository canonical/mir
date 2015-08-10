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
#include <atomic>
#include <memory>
#include <vector>

namespace mir
{
namespace dispatch
{
class ActionQueue;
}
namespace input
{
class InputDevice;
}
}
namespace mir_test_framework
{
class FakeInputDevice;
class StubInputPlatform : public mir::input::Platform
{
public:
    explicit StubInputPlatform(std::shared_ptr<mir::input::InputDeviceRegistry> const& input_device_registry);
    ~StubInputPlatform();

    std::shared_ptr<mir::dispatch::Dispatchable> dispatchable() override;
    void start() override;
    void stop() override;

    static void add(std::shared_ptr<mir::input::InputDevice> const& dev);
    static void remove(std::shared_ptr<mir::input::InputDevice> const& dev);

private:
    std::shared_ptr<mir::dispatch::ActionQueue> const platform_queue;
    std::shared_ptr<mir::input::InputDeviceRegistry> const registry;
    static std::atomic<StubInputPlatform*> stub_input_platform;
    static std::vector<std::weak_ptr<mir::input::InputDevice>> device_store;
};

}

#endif
