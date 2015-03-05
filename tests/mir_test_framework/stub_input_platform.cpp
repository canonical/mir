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

#include <algorithm>

namespace mtf = mir_test_framework;
namespace mi = mir::input;

mtf::StubInputPlatform::StubInputPlatform(
    std::shared_ptr<mi::InputDeviceRegistry> const& input_device_registry)
{
    registry = input_device_registry;
}

mtf::StubInputPlatform::~StubInputPlatform()
{
    registry.reset();
}

void mtf::StubInputPlatform::start()
{
    for (auto const & dev : registered_devs)
        registry->add_device(dev);
}

std::shared_ptr<mir::dispatch::Dispatchable> mtf::StubInputPlatform::get_dispatchable()
{
    return platform_queue;
}

void mtf::StubInputPlatform::stop()
{
    for (auto const & dev : registered_devs)
        registry->remove_device(dev);

    registered_devs.clear();
}

void mtf::StubInputPlatform::add(std::shared_ptr<mir::input::InputDevice> const& dev)
{
    platform_queue->enqueue([dev]
                            {
                                registry->add_device(dev);
                                registered_devs.push_back(dev);
                            });
}
void mtf::StubInputPlatform::remove(std::shared_ptr<mir::input::InputDevice> const& dev)
{
    platform_queue->enqueue([dev]
                            {
                                registry->remove_device(dev);
                                registered_devs.erase(
                                    std::remove(begin(registered_devs),
                                           end(registered_devs),
                                           dev));
                            });
}

std::vector<std::shared_ptr<mi::InputDevice>> mtf::StubInputPlatform::registered_devs;
std::shared_ptr<mi::InputDeviceRegistry> mtf::StubInputPlatform::registry;
std::shared_ptr<mir::dispatch::ActionQueue> mtf::StubInputPlatform::platform_queue = std::make_shared<mir::dispatch::ActionQueue>();

