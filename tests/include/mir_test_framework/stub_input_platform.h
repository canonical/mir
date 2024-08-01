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
#ifndef MIR_TEST_FRAMEWORK_STUB_INPUT_PLATFORM_H_
#define MIR_TEST_FRAMEWORK_STUB_INPUT_PLATFORM_H_

#include "mir/input/platform.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace mir
{
namespace dispatch
{
class ActionQueue;
class MultiplexingDispatchable;
}
namespace input
{
class InputDevice;
}
}
namespace mir_test_framework
{

class DeviceStore
{
public:
    virtual ~DeviceStore() = default;
    virtual void foreach_device(std::function<void(std::weak_ptr<mir::input::InputDevice> const&)> const&) = 0;
    virtual void clear() = 0;
};

class FakeInputDevice;
class StubInputPlatform : public mir::input::Platform
{
public:
    explicit StubInputPlatform(
        std::shared_ptr<mir::input::InputDeviceRegistry> const& input_device_registry,
        std::shared_ptr<DeviceStore> const& device_store);
    ~StubInputPlatform();

    std::shared_ptr<mir::dispatch::Dispatchable> dispatchable() override;
    void start() override;
    void stop() override;
    void pause_for_config() override;
    void continue_after_config() override;

    void add(std::shared_ptr<mir::input::InputDevice> const& dev);
    void remove(std::shared_ptr<mir::input::InputDevice> const& dev);
    void register_dispatchable(std::shared_ptr<mir::dispatch::Dispatchable> const& queue);
    void unregister_dispatchable(std::shared_ptr<mir::dispatch::Dispatchable> const& queue);

private:
    std::shared_ptr<mir::dispatch::MultiplexingDispatchable> const platform_dispatchable;
    std::shared_ptr<mir::dispatch::ActionQueue> const platform_queue;
    std::shared_ptr<mir::input::InputDeviceRegistry> const registry;
    std::shared_ptr<DeviceStore> const device_store;
};

}

#endif
