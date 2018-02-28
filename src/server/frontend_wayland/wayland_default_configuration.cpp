/*
 * Copyright © 2015, 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/default_server_configuration.h"
#include "wayland_connector.h"

#include "mir/frontend/display_changer.h"
#include "mir/graphics/platform.h"
#include "mir/options/default_configuration.h"

namespace mf = mir::frontend;

std::shared_ptr<mf::Connector>
    mir::DefaultServerConfiguration::the_wayland_connector()
{
    return wayland_connector(
        [this]() -> std::shared_ptr<mf::Connector>
        {
            bool const arw_socket = the_options()->is_set(options::arw_server_socket_opt);

            optional_value<std::string> display_name;

            if (the_options()->is_set(options::wayland_socket_name_opt))
                display_name = the_options()->get<std::string>(options::wayland_socket_name_opt);

            return std::make_shared<mf::WaylandConnector>(
                display_name,
                the_frontend_shell(),
                *the_frontend_display_changer(),
                the_input_device_hub(),
                the_seat(),
                the_buffer_allocator(),
                the_session_authorizer(),
                arw_socket);
        });
}
