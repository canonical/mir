/*
 * Copyright (C) Marius Gripsgard <marius@ubports.com>
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
#include "mir/options/default_configuration.h"
#include "mir/main_loop.h"
#include "wayland_connector.h"
#include "xwayland_connector.h"

#include <boost/lexical_cast.hpp>

#include <string>
#include <cstdlib>

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
        auto const x11_enabled = options->is_set(mo::x11_display_opt) && options->get<bool>(mo::x11_display_opt);

        if (x11_enabled)
        {
            try
            {
                auto const scale = boost::lexical_cast<float>(options->get<std::string>(mo::x11_scale_opt));
                if (scale < 0.01f || scale > 100.0f)
                {
                    BOOST_THROW_EXCEPTION(std::runtime_error("scale outside of valid range"));
                }
                auto wayland_connector = std::static_pointer_cast<mf::WaylandConnector>(the_wayland_connector());
                return std::make_shared<mf::XWaylandConnector>(
                    the_main_loop(),
                    wayland_connector,
                    options->get<std::string>("xwayland-path"),
                    scale);
            }
            catch (std::exception& x)
            {
                mir::fatal_error(
                    "Failed to start XWaylandConnector: %s\n  (xwayland-path is '%s', x11-scale is '%s')",
                    x.what(),
                    options->get<std::string>("xwayland-path").c_str(),
                    options->get<std::string>(mo::x11_scale_opt).c_str());
            }
        }

        return std::make_shared<NullConnector>();
    });
}
