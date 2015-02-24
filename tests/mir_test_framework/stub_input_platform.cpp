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

#include "mir/input/input_event_handler_register.h"
#include "mir/input/input_device_registry.h"

#include <algorithm>

namespace mtf = mir_test_framework;
namespace mi = mir::input;

void mtf::StubInputPlatform::start(mi::InputEventHandlerRegister& loop,
           std::shared_ptr<mi::InputDeviceRegistry> const& input_device_registry)
{
    std::unique_lock<std::mutex> lock(platform_mutex);
    registry = input_device_registry;
    event_handler = &loop;
}

void mtf::StubInputPlatform::stop(mi::InputEventHandlerRegister&)
{
    std::unique_lock<std::mutex> lock(platform_mutex);

    if (registry)
    {
        for (auto const & dev : registered_devs)
            registry->remove_device(dev);
    }

    registered_devs.clear();

    registry.reset();
    event_handler = nullptr;
}

void mtf::StubInputPlatform::add(std::shared_ptr<mir::input::InputDevice> const& dev)
{
    std::unique_lock<std::mutex> lock(platform_mutex);
    registered_devs.push_back(dev);

    if (event_handler)
    {
        event_handler->register_handler(
            [dev]()
            {
                registry->add_device(dev);
            });
    }
}
void mtf::StubInputPlatform::remove(std::shared_ptr<mir::input::InputDevice> const& dev)
{
    std::unique_lock<std::mutex> lock(platform_mutex);
    if (event_handler)
    {
        event_handler->register_handler(
            [dev]()
            {
                registry->remove_device(dev);
            });
    }


    registered_devs.erase(
        std::remove(
            begin(registered_devs),
            end(registered_devs), dev),
        end(registered_devs)
        );
}

std::mutex mtf::StubInputPlatform::platform_mutex;
std::vector<std::shared_ptr<mi::InputDevice>> mtf::StubInputPlatform::registered_devs;
std::shared_ptr<mi::InputDeviceRegistry> mtf::StubInputPlatform::registry;
mi::InputEventHandlerRegister* mtf::StubInputPlatform::event_handler = nullptr;

