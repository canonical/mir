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

#include "mir/input/input_devices.h"
#include "mir/input/mir_input_config_serialization.h"
#include "mir/input/mir_input_config_serialization.h"
#include "mir/input/xkb_mapper.h"
#include "../mir_surface.h"

#include <unordered_set>

namespace mi = mir::input;

namespace
{
std::unordered_set<MirInputDeviceId> get_removed_devices(MirInputConfig const& old_config, MirInputConfig const& new_config)
{
    std::unordered_set<MirInputDeviceId> removed;
    old_config.for_each(
        [&new_config, &removed](MirInputDevice const& dev)
        {
            if (nullptr == new_config.get_device_config_by_id(dev.id()))
                removed.insert(dev.id());
        });
    return removed;
}
}

mi::InputDevices::InputDevices(std::shared_ptr<client::SurfaceMap> const& windows) :
    windows{windows},
    callback{[]{}}
{
}

void mi::InputDevices::update_devices(std::string const& config_buffer)
{
    std::unordered_set<MirInputDeviceId> removed_devices;
    decltype(callback) stored_callback;

    {
        std::lock_guard<std::mutex> lock(devices_access);

        auto const& new_configuration = mi::deserialize_input_config(config_buffer);
        removed_devices = get_removed_devices(new_configuration, configuration);
        configuration = new_configuration;
        stored_callback = callback;
    }

    if (auto const window_map = windows.lock())
    {
        window_map->with_all_windows_do([&removed_devices](MirWindow* window)
            {
                auto const& keymapper = window->get_keymapper();

                for (auto const& id : removed_devices)
                    keymapper->clear_keymap_for_device(id);
            });
    }

    stored_callback();
}

MirInputConfig mi::InputDevices::devices()
{
    std::lock_guard<std::mutex> lock(devices_access);
    return configuration;
}

void mi::InputDevices::set_change_callback(std::function<void()> const& new_callback)
{
    std::lock_guard<std::mutex> lock(devices_access);
    callback = new_callback;
}
