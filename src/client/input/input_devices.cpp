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

#include "mir/input/input_devices.h"

namespace mi = mir::input;
mi::InputDevices::InputDevices() = default; //necessary to make sure the linker does not discard this yet unused class

void mi::InputDevices::clear()
{
    devices.clear();
}

void  mi::InputDevices::add_device(DeviceData && dev)
{
    devices.emplace_back(std::move(dev));
}

void  mi::InputDevices::notify_changes()
{
    // does nothing yet
}

