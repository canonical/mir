/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
 * Copyright (C) 2019-2020 Canonical Ltd.
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

#ifndef MIR_FRONTEND_XWAYLAND_SERVER_H
#define MIR_FRONTEND_XWAYLAND_SERVER_H

#include "mir/fd.h"

#include <memory>
#include <string>
#include <mutex>
#include <experimental/optional>

struct wl_client;

namespace mir
{
namespace frontend
{
class WaylandConnector;
class XWaylandSpawner;

class XWaylandServer
{
public:
    XWaylandServer(
        std::shared_ptr<WaylandConnector> const& wayland_connector,
        XWaylandSpawner const& spawner,
        std::string const& xwayland_path,
        std::pair<mir::Fd, mir::Fd> const& wayland_socket_pair,
        mir::Fd const& x11_server_fd);
    ~XWaylandServer();

    auto client() const -> wl_client* { return wayland_client; }
    auto is_running() const -> bool;

    // Returns a symmetrical pair of connected sockets
    static auto make_socket_pair() -> std::pair<mir::Fd, mir::Fd>;

private:
    class StartupSignalHandler;

    XWaylandServer(XWaylandServer const&) = delete;
    XWaylandServer& operator=(XWaylandServer const&) = delete;

    /// Created at the beginning of the constructor and destroyed at the end
    std::unique_ptr<StartupSignalHandler> startup_signal_handler;

    pid_t const xwayland_pid;
    mir::Fd const wayland_server_fd;
    wl_client* const wayland_client{nullptr};

    mutable std::mutex mutex;
    mutable bool running;
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_SERVER_H
