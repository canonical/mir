/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "libinput.h"
#include "libinput_device_ptr.h"

namespace mie = mir::input::evdev;

void mie::LibInputDeviceDeleter::operator()(::libinput_device* device) const
{
    libinput_device_unref(device);
}

mie::LibInputDevicePtr mie::make_libinput_device(std::shared_ptr<libinput> const& lib, libinput_device* dev)
{
    auto ret = mie::LibInputDevicePtr(dev, lib);
    if (ret)
        libinput_device_ref(ret.get());
    return ret;
}
