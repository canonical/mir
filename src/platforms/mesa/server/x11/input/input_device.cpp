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

#include "mir/input/input_device_info.h"
#include "input_device.h"
#include "dispatchable.h"
#include "../xserver_connection.h"

namespace mi = mir::input;
namespace mix = mi::X;
namespace md = mir::dispatch;
namespace mx = mir::X;

extern std::shared_ptr<mx::X11Connection> x11_connection;

mix::XInputDevice::XInputDevice()
    : fd(XConnectionNumber(x11_connection->dpy)), x_dispatchable(std::make_shared<mix::XDispatchable>(fd))
{
}

std::shared_ptr<md::Dispatchable> mix::XInputDevice::dispatchable()
{
    return x_dispatchable;
}

void mix::XInputDevice::start(mi::InputSink* destination)
{
    x_dispatchable->set_input_sink(destination);
}

void mix::XInputDevice::stop()
{
    x_dispatchable->unset_input_sink();
}

mi::InputDeviceInfo mix::XInputDevice::get_device_info()
{
    return mi::InputDeviceInfo{0,"x11-keyboard-device","x11-key-dev-1", mi::DeviceCapability::keyboard};
}
