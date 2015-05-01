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

#include "X_input_device.h"
#include "mir/dispatch/dispatchable.h"
#include "../debug.h"

namespace mi = mir::input;
namespace mix = mi::X;
namespace md = mir::dispatch;

std::shared_ptr<md::Dispatchable> mix::XInputDevice::dispatchable()
{
    CALLED
    return nullptr;
}

void mix::XInputDevice::start(InputSink* /*destination*/)
{
    CALLED
}

void mix::XInputDevice::stop()
{
    CALLED
}

mi::InputDeviceInfo mix::XInputDevice::get_device_info()
{
    CALLED
    return info;
}
