/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "X_input_platform.h"

#include "mir/input/input_device_registry.h"
#include "mir/dispatch/action_queue.h"
#include "mir/module_deleter.h"
#include "../debug.h"

//#include <algorithm>

namespace mi = mir::input;
namespace mix = mi::X;

mix::XInputPlatform::XInputPlatform(
    std::shared_ptr<mi::InputDeviceRegistry> const& input_device_registry)
    : platform_queue{mir::make_module_ptr<mir::dispatch::ActionQueue>()},
      registry(input_device_registry)
{
    CALLED
}

mix::XInputPlatform::~XInputPlatform()
{
    CALLED
}

void mix::XInputPlatform::start()
{
    CALLED
//    auto device = dev.lock();
//    registry->add_device(device);
}

std::shared_ptr<mir::dispatch::Dispatchable> mix::XInputPlatform::dispatchable()
{
    CALLED
    return platform_queue;
}

void mix::XInputPlatform::stop()
{
    CALLED
}
