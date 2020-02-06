/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
 * Copyright (C) 2019 Canonical Ltd.
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

#include "xwayland_connector.h"

#include "mir/log.h"

#include "wayland_connector.h"
#include "xwayland_server.h"

namespace mf = mir::frontend;

mf::XWaylandConnector::XWaylandConnector(
    std::shared_ptr<WaylandConnector> const& wayland_connector, std::string const& xwayland_path) :
    start_xwayland{wayland_connector->get_extension("x11-support") ?
        [=]{ return std::make_unique<XWaylandServer>(wayland_connector, xwayland_path); } :
        decltype(start_xwayland){[]{ return std::unique_ptr<XWaylandServer>{}; }}}
{
}

void mf::XWaylandConnector::start()
{
    xwayland_server = start_xwayland();
    mir::log_info("XWayland loop started");
}

void mf::XWaylandConnector::stop()
{
    xwayland_server.reset();
    mir::log_info("XWayland loop stopped");
}

int mf::XWaylandConnector::client_socket_fd() const
{
    return -1;
}

int mf::XWaylandConnector::client_socket_fd(
    std::function<void(std::shared_ptr<scene::Session> const& session)> const& /*connect_handler*/) const
{
    return -1;
}

auto mf::XWaylandConnector::socket_name() const -> optional_value<std::string>
{
    if (xwayland_server)
    {
        return xwayland_server->x11_display();
    }
    else
    {
        return optional_value<std::string>();
    }
}

mf::XWaylandConnector::~XWaylandConnector() = default;
