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

#ifndef MIR_INPUT_EVDEV_LIBINPUT_DEVICE_PTR_H_
#define MIR_INPUT_EVDEV_LIBINPUT_DEVICE_PTR_H_

#include <memory>

struct libinput_device;
struct libinput;

namespace mir
{
namespace input
{
namespace evdev
{
using LibInputDevicePtr = std::unique_ptr<libinput_device, libinput_device*(*)(libinput_device*)>;

LibInputDevicePtr make_libinput_device(libinput* lib, char const* path);
}
}
}

#endif
