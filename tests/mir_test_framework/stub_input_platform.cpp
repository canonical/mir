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

#include "stub_input_platform.h"

#include "mir/input/input_device_registry.h"
#include "mir/dispatch/action_queue.h"
#include "mir/module_deleter.h"

#include <algorithm>

namespace mtf = mir_test_framework;
namespace mi = mir::input;

mtf::StubInputPlatform::StubInputPlatform(
    std::shared_ptr<mi::InputDeviceRegistry> const& input_device_registry)
    : platform_queue{mir::make_module_ptr<mir::dispatch::ActionQueue>()},
    registry(input_device_registry)
{
    stub_input_platform = this;
}

mtf::StubInputPlatform::~StubInputPlatform()
{
    device_store.clear();
    stub_input_platform = nullptr;
}

void mtf::StubInputPlatform::start()
{
    for (auto const& dev : device_store)
    {
        auto device = dev.lock();
        if (device)
            registry->add_device(device);
    }
}

std::shared_ptr<mir::dispatch::Dispatchable> mtf::StubInputPlatform::dispatchable()
{
    return platform_queue;
}

void mtf::StubInputPlatform::stop()
{
    for (auto const& dev : device_store)
    {
        auto device = dev.lock();
        if (device)
            registry->remove_device(device);
    }
}

void mtf::StubInputPlatform::add(std::shared_ptr<mir::input::InputDevice> const& dev)
{
    if (!stub_input_platform)
    {
        device_store.push_back(dev);
        return;
    }

    stub_input_platform->platform_queue->enqueue(
        [registry=stub_input_platform->registry,dev]
        {
            registry->add_device(dev);
        });
}

void mtf::StubInputPlatform::remove(std::shared_ptr<mir::input::InputDevice> const& dev)
{
    if (!stub_input_platform)
        BOOST_THROW_EXCEPTION(std::runtime_error("No stub input platform available"));

    stub_input_platform->platform_queue->enqueue(
        [registry=stub_input_platform->registry,dev]
        {
            registry->remove_device(dev);
        });
}

mtf::StubInputPlatform* mtf::StubInputPlatform::stub_input_platform = nullptr;
std::vector<std::weak_ptr<mir::input::InputDevice>> mtf::StubInputPlatform::device_store;
