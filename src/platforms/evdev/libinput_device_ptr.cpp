/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

mie::LibInputDevicePtr mie::make_libinput_device(libinput* lib, char const* path)
{
    auto ret = mie::LibInputDevicePtr(::libinput_path_add_device(lib, path), libinput_device_unref);
    if (ret)
        libinput_device_ref(ret.get());
    return ret;
}
