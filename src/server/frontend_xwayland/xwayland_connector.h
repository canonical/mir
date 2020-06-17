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

#ifndef MIR_FRONTEND_XWAYLAND_CONNECTOR_H
#define MIR_FRONTEND_XWAYLAND_CONNECTOR_H

#include "mir/frontend/connector.h"

namespace mir
{
namespace frontend
{
class WaylandConnector;
class XWaylandServer;
class XWaylandSpawner;
class XWaylandConnector : public Connector
{
public:
    XWaylandConnector(
        std::shared_ptr<WaylandConnector> const& wayland_connector,
        std::string const& xwayland_path);
    ~XWaylandConnector() override;

    void start() override;
    void stop() override;

    int client_socket_fd() const override;
    int client_socket_fd(
        std::function<void(std::shared_ptr<scene::Session> const& session)> const& connect_handler) const override;

    auto socket_name() const -> optional_value<std::string> override;

private:
    std::shared_ptr<WaylandConnector> const wayland_connector;
    std::string const xwayland_path;

    std::unique_ptr<XWaylandSpawner> xwayland_spawner;
    std::unique_ptr<XWaylandServer> xwayland_server;
};
} /* frontend */
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_CONNECTOR_H */
