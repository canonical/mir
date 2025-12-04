/*
 * Copyright (C) Marius Gripsgard <marius@ubports.com>
 * Copyright (C) Canonical Ltd.
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

#include <mir/fd.h>

#include <memory>
#include <string>
#include <mutex>
#include <optional>

struct wl_client;

namespace mir
{
namespace frontend
{
class WaylandConnector;
class XWaylandSpawner;

struct XWaylandProcessInfo
{
    pid_t const pid;
    Fd const wayland_fd;
    Fd const x11_fd;
};

class XWaylandServer
{
public:
    XWaylandServer(
        std::shared_ptr<WaylandConnector> const& wayland_connector,
        XWaylandSpawner const& spawner,
        std::string const& xwayland_path,
        float scale);
    ~XWaylandServer();

    auto client() const -> wl_client* { return wayland_client; }
    auto x11_wm_fd() const -> Fd const& { return xwayland.x11_fd; }
    auto is_running() const -> bool;

private:
    XWaylandServer(XWaylandServer const&) = delete;
    XWaylandServer& operator=(XWaylandServer const&) = delete;

    XWaylandProcessInfo const xwayland;
    wl_client* const wayland_client{nullptr};

    mutable std::mutex mutex;
    mutable bool running;
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_SERVER_H
