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
    XWaylandServer(std::shared_ptr<WaylandConnector> wayland_connector, std::string const& xwayland_path);
    ~XWaylandServer();

    void new_spawn_thread(XWaylandSpawner const& spawner);

private:
    XWaylandServer(XWaylandServer const&) = delete;
    XWaylandServer& operator=(XWaylandServer const&) = delete;

    /// Forks off the XWayland process
    void spawn(XWaylandSpawner const& spawner);
    /// Called after fork() if we should turn into XWayland
    void execl_xwayland(XWaylandSpawner const& spawner, int wl_client_client_fd, int wm_client_fd);
    /// Called after fork() if we should continue on as Mir
    void connect_wm_to_xwayland();

    enum Status {
        STARTING = 1,
        RUNNING = 2,
        STOPPED = -1,
        FAILED = -2
    };

    std::shared_ptr<WaylandConnector> const wayland_connector;
    std::string const xwayland_path;

    std::mutex mutable spawn_thread_mutex;
    std::thread spawn_thread;
    pid_t spawn_thread_pid;
    wl_client* spawn_thread_client{nullptr};
    Fd spawn_thread_wm_server_fd;
    Fd spawn_thread_wl_client_fd;
    Status spawn_thread_xserver_status{Status::STOPPED};
    bool spawn_thread_terminate{false};
    std::shared_ptr<XWaylandWM> spawn_thread_wm;
};
} /* frontend */
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_SERVER_H */
