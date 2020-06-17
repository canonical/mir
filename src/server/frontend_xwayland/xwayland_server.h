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
#include <mutex>
#include <thread>
#include <string>
#include <vector>

struct wl_client;

namespace mir
{
namespace dispatch
{
class ReadableFd;
class ThreadedDispatcher;
class MultiplexingDispatchable;
} /*dispatch */
namespace frontend
{
class WaylandConnector;
class XWaylandSpawner;
class XWaylandWM;

class XWaylandServer
{
public:
    XWaylandServer(
        std::shared_ptr<WaylandConnector> const& wayland_connector,
        XWaylandSpawner const& spawner,
        std::string const& xwayland_path);
    ~XWaylandServer();

    auto client() const -> wl_client* { return wayland_client; }
    auto wm_fd() const -> Fd const& { return xwayland_process.x11_fd; }

    struct XWaylandProcess
    {
        pid_t pid;
        Fd x11_fd;
        Fd wayland_fd;
    };

private:
    XWaylandServer(XWaylandServer const&) = delete;
    XWaylandServer& operator=(XWaylandServer const&) = delete;

    XWaylandProcess const xwayland_process;
    wl_client* const wayland_client{nullptr};
};
} /* frontend */
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_SERVER_H */
