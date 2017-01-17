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
#include "mir/input/mir_input_config_serialization.h"

namespace mi = mir::input;

void mi::InputDevices::update_devices(std::string const& config_buffer)
{
    std::function<void()> stored_callback;

    {
        std::unique_lock<std::mutex> lock(devices_access);
        configuration = mi::deserialize_input_config(config_buffer);
        stored_callback = callback;
    }

    if (stored_callback)
        stored_callback();
}

MirInputConfig mi::InputDevices::devices()
{
    std::unique_lock<std::mutex> lock(devices_access);
    return configuration;
}

void mi::InputDevices::set_change_callback(std::function<void()> const& new_callback)
{
    std::unique_lock<std::mutex> lock(devices_access);
    callback = new_callback;
}
