/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
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
 */

#include "mir/default_server_configuration.h"
#include "mir/log.h"
#include "wayland_connector.h"
#include "xwayland_connector.h"

#include <string>

#include "mir/options/default_configuration.h"

namespace mf = mir::frontend;

std::shared_ptr<mf::Connector> mir::DefaultServerConfiguration::the_xwayland_connector()
{
    return xwayland_connector([this]() -> std::shared_ptr<mf::Connector> {
        // For now just use a env var to set X11 display
        // If not set use 0
        int disp = 0;
        try
        {
            disp = std::stoi(getenv("MIR_X11_DISPLAY"));
        }
        catch (...)
        {
            mir::log_error("No valid x11 display vaule provided, must be number only!");
            disp = 0;
        }

        auto wc = std::static_pointer_cast<mf::WaylandConnector>(the_wayland_connector());
        return std::make_shared<mf::XWaylandConnector>(disp, wc);
    });
}
