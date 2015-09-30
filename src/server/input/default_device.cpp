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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "device_handle.h"

namespace mi = mir::input;

mi::DefaultDevice::DefaultDevice(MirInputDeviceId id, mi::InputDeviceInfo const& info) :
    device_id{id}, info(info)
{
}

mi::DeviceCapabilities mi::DefaultDevice::capabilities() const
{
    return info.capabilities;
}

std::string mi::DefaultDevice::name() const
{
    return info.name;
}

std::string mi::DefaultDevice::unique_id() const
{
    return info.unique_id;
}

MirInputDeviceId mi::DefaultDevice::id() const
{
    return device_id;
}
