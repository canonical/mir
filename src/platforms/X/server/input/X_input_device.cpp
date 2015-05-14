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
#include "X_dispatchable.h"
#include "../debug.h"

namespace mi = mir::input;
namespace mix = mi::X;
namespace md = mir::dispatch;

extern ::Display *x_dpy;

mix::XInputDevice::XInputDevice()
    : fd(XConnectionNumber(x_dpy)), x_dispatchable(std::make_shared<mix::XDispatchable>(fd))
{
    CALLED
}

std::shared_ptr<md::Dispatchable> mix::XInputDevice::dispatchable()
{
    CALLED

    return x_dispatchable;
}

void mix::XInputDevice::start(mi::InputSink* destination)
{
    CALLED

    x_dispatchable->set_input_sink(destination);
}

void mix::XInputDevice::stop()
{
    CALLED

    x_dispatchable->unset_input_sink();
}

mi::InputDeviceInfo mix::XInputDevice::get_device_info()
{
    CALLED

    return info;
}
