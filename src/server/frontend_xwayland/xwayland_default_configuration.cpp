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
namespace ms = mir::scene;
namespace mo = mir::options;

namespace
{
struct NullConnector : mf::Connector
{
    void start() override
    {
    }

    void stop() override
    {
    }

    int client_socket_fd() const override
    {
        return 0;
    }

    int client_socket_fd(std::function<void(std::shared_ptr<ms::Session> const&)> const&) const override
    {
        return -1;
    }

    mir::optional_value<std::string> socket_name() const override
    {
        return mir::optional_value<std::string>();
    }
};
}

std::shared_ptr<mf::Connector> mir::DefaultServerConfiguration::the_xwayland_connector()
{
    return xwayland_connector([this]() -> std::shared_ptr<mf::Connector> {

        auto options = the_options();
        if (options->is_set(mo::x11_display_opt))
        {
            try
            {
                auto wayland_connector = std::static_pointer_cast<mf::WaylandConnector>(the_wayland_connector());
                return std::make_shared<mf::XWaylandConnector>(
                    wayland_connector,
                    options->get<std::string>("xwayland-path"));
            }
            catch (std::exception& x)
            {
                mir::fatal_error("Failed to start XWaylandConnector: %s", x.what());
            }
        }

        return std::make_shared<NullConnector>();
    });
}
