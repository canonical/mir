/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_input_device.h"

#include <signal.h>

#include <iostream>
#include <future>
#include <algorithm>

std::promise<bool> stop;
void shutdown(int)
{
    stop.set_value(true);
}

auto query_input_config(MirConnection* con)
{
    return std::unique_ptr<MirInputConfig, void (*)(MirInputConfig const*)>(mir_connection_create_input_config(con),
                                                                            &mir_input_config_release);
}

std::string capability_to_string(MirInputDeviceCapabilities caps)
{
    std::string cap_string;
    if (caps & mir_input_device_capability_touchpad) cap_string += " touchpad";
    else if (caps & mir_input_device_capability_pointer) cap_string += " pointer";
    if (caps & mir_input_device_capability_alpha_numeric) cap_string += " full keyboard";
    else if (caps & mir_input_device_capability_keyboard) cap_string += " keys";
    if (caps & mir_input_device_capability_touchscreen) cap_string += " touchscreen";
    if (caps & mir_input_device_capability_switch) cap_string += " switch";
    if (caps & mir_input_device_capability_gamepad) cap_string += " gamepad";
    if (caps & mir_input_device_capability_joystick) cap_string += " joystick";
    return cap_string;
}

std::ostream& operator<<(std::ostream& out, MirInputDevice const& dev)
{
    return out << mir_input_device_get_id(&dev)
            << ':' << mir_input_device_get_name(&dev)
            << ':' << mir_input_device_get_unique_id(&dev)
            << ' ' << capability_to_string(mir_input_device_get_capabilities(&dev));
}

std::ostream& operator<<(std::ostream& out, MirInputConfig const& config)
{
    out << "Attached input devices:" << std::endl;
    for (size_t i = 0; i != mir_input_config_device_count(&config); ++i)
        out << *mir_input_config_get_device(&config, i) << std::endl;

    return out << std::endl;
}

void on_config_change(MirConnection* connection, void*)
{
    auto config = query_input_config(connection);
    std::cout << *config;
}

int main(int argc, char const* argv[])
{
    std::future<bool> wait_for_stop = stop.get_future();
    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
    signal(SIGHUP, shutdown);
    char const* server = nullptr;

    for (int a = 1; a < argc; ++a)
    {
        const char *arg = argv[a];
        if (arg[0] == '-')
        {
            if (arg[1] == '-' && arg[2] == '\0')
                break;
            std::cout << "Usage: " << argv[0] << " [<socket-file>] [--]\n"
                "Options:\n"
                "    --  Ignore the rest of the command line."
                << std::endl;
            return 0;
        }
        else
        {
            server = arg;
        }
    }

    MirConnection *conn = mir_connect_sync(server, argv[0]);
    if (!mir_connection_is_valid(conn))
    {
        std::cerr << "Could not connect to display server: " << mir_connection_get_error_message(conn) << std::endl;
        return 1;
    }

    on_config_change(conn, nullptr);
    mir_connection_set_input_config_change_callback(conn, on_config_change, nullptr);
    wait_for_stop.get();
    return 0;
}
