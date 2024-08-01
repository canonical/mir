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

#include "mir_test_framework/stub_input_platform_accessor.h"

namespace mir_test_framework
{
class StaticDeviceStore : public mir_test_framework::DeviceStore
{
public:
    void foreach_device(std::function<void(std::weak_ptr<mir::input::InputDevice> const&)> const& f) override
    {
        std::lock_guard lk{device_store_guard};
        for (auto const& dev : device_store)
            f(dev);
    }

    void clear() override
    {
        std::lock_guard lk{device_store_guard};
        device_store.clear();
        StubInputPlatformAccessor::clear();
    }

    static std::mutex device_store_guard;
    static std::vector<std::weak_ptr<mir::input::InputDevice>> device_store;
};

std::vector<std::weak_ptr<mir::input::InputDevice>> StaticDeviceStore::device_store;
std::mutex StaticDeviceStore::device_store_guard;
}

mir::UniqueModulePtr<mir::input::Platform> mir_test_framework::StubInputPlatformAccessor::get(
    std::shared_ptr<mir::input::InputDeviceRegistry> const& input_device_registry)
{
    auto ptr = mir::make_module_ptr<mir_test_framework::StubInputPlatform>(
        input_device_registry, std::make_shared<StaticDeviceStore>());
    stub_input_platform = ptr.get();
    return ptr;
}


void mir_test_framework::StubInputPlatformAccessor::add(std::shared_ptr<mir::input::InputDevice> const& dev)
{
    auto input_platform = stub_input_platform.load();
    if (!input_platform)
    {
        std::lock_guard lk{StaticDeviceStore::device_store_guard};
        StaticDeviceStore::device_store.push_back(dev);
        return;
    }

    input_platform->add(dev);
}

void mir_test_framework::StubInputPlatformAccessor::remove(std::shared_ptr<mir::input::InputDevice> const& dev)
{
    auto input_platform = stub_input_platform.load();
    if (!input_platform)
        BOOST_THROW_EXCEPTION(std::runtime_error("No stub input platform available"));

    std::lock_guard lk{StaticDeviceStore::device_store_guard};
    StaticDeviceStore::device_store.erase(
        std::remove_if(begin(StaticDeviceStore::device_store),
                       end(StaticDeviceStore::device_store),
                       [dev](auto weak_dev)
                       {
                           return (weak_dev.lock() == dev);
                       }),
        end(StaticDeviceStore::device_store));
    input_platform->remove(dev);
}

void mir_test_framework::StubInputPlatformAccessor::register_dispatchable(std::shared_ptr<mir::dispatch::Dispatchable> const& queue)
{
    auto input_platform = stub_input_platform.load();
    if (!input_platform)
        BOOST_THROW_EXCEPTION(std::runtime_error("No stub input platform available"));

    input_platform->register_dispatchable(queue);
}

void mir_test_framework::StubInputPlatformAccessor::unregister_dispatchable(std::shared_ptr<mir::dispatch::Dispatchable> const& queue)
{
    auto input_platform = stub_input_platform.load();
    if (!input_platform)
        BOOST_THROW_EXCEPTION(std::runtime_error("No stub input platform available"));

    input_platform->unregister_dispatchable(queue);
}

void mir_test_framework::StubInputPlatformAccessor::clear()
{
    stub_input_platform = nullptr;
}

std::atomic<mir_test_framework::StubInputPlatform*> mir_test_framework::StubInputPlatformAccessor::stub_input_platform{nullptr};