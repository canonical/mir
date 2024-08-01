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

#include "mir_test_framework/stub_input_platform.h"

#include "mir/input/input_device_registry.h"
#include "mir/dispatch/action_queue.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/module_deleter.h"

#include <algorithm>

namespace mtf = mir_test_framework;
namespace mi = mir::input;

mtf::StubInputPlatform::StubInputPlatform(
    std::shared_ptr<mi::InputDeviceRegistry> const& input_device_registry,
    std::shared_ptr<DeviceStore> const& device_store)
    : platform_dispatchable{std::make_shared<mir::dispatch::MultiplexingDispatchable>()},
    platform_queue{std::make_shared<mir::dispatch::ActionQueue>()},
    registry(input_device_registry),
    device_store(device_store)
{
    platform_dispatchable->add_watch(platform_queue);
}

mtf::StubInputPlatform::~StubInputPlatform()
{
    device_store->clear();
}

void mtf::StubInputPlatform::start()
{
    device_store->foreach_device([&](auto const& dev)
    {
        auto device = dev.lock();
        if (device)
            registry->add_device(device);
    });
}

std::shared_ptr<mir::dispatch::Dispatchable> mtf::StubInputPlatform::dispatchable()
{
    return platform_dispatchable;
}

void mtf::StubInputPlatform::stop()
{
    device_store->foreach_device([&](auto const& dev)
    {
        auto device = dev.lock();
        if (device)
            registry->remove_device(device);
    });
}

void mtf::StubInputPlatform::pause_for_config()
{
}

void mtf::StubInputPlatform::continue_after_config()
{
}

void mtf::StubInputPlatform::add(std::shared_ptr<mir::input::InputDevice> const& dev)
{
    platform_queue->enqueue(
        [registry=registry,dev]
        {
            registry->add_device(dev);
        });
}

void mtf::StubInputPlatform::remove(std::shared_ptr<mir::input::InputDevice> const& dev)
{
    platform_queue->enqueue(
        [ registry = registry, dev ]
        {
            registry->remove_device(dev);
        });
}

void mtf::StubInputPlatform::register_dispatchable(std::shared_ptr<mir::dispatch::Dispatchable> const& queue)
{
    platform_dispatchable->add_watch(queue);
}

void mtf::StubInputPlatform::unregister_dispatchable(std::shared_ptr<mir::dispatch::Dispatchable> const& queue)
{
    platform_dispatchable->remove_watch(queue);
}
