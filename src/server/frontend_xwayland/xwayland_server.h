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

#ifndef MIR_FRONTEND_XWAYLAND_SERVER_H
#define MIR_FRONTEND_XWAYLAND_SERVER_H

#include <memory>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>

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
class XWaylandWM;
class XWaylandServer
{
public:
    XWaylandServer(const int xdisp, std::shared_ptr<WaylandConnector> wc, std::string const& xwayland_path);
    ~XWaylandServer();

    enum Status {
      STARTING = 1,
      RUNNING = 2,
      STOPPED = -1,
      FAILED = -2
     };

    // Ugh, this is ugly!
    static bool xserver_ready;

private:
    void setup_socket();
    void spawn_xserver_on_event_loop();

    /// Forks off the XWayland process
    void spawn();
    /// Called after fork() if we should turn into XWayland
    void execl_xwayland(int wl_client_client_fd, int wm_client_fd);
    /// Called after fork() if we should continue on as Mir
    void connect_to_xwayland(int wl_client_server_fd, int wm_server_fd);
    void new_spawn_thread();
    int create_lockfile();
    int create_socket(struct sockaddr_un *addr, size_t path_size);
    bool set_cloexec(int fd, bool cloexec);

    std::unique_ptr<dispatch::ThreadedDispatcher> xserver_thread;
    std::shared_ptr<XWaylandWM> wm;
    int xdisplay;
    std::shared_ptr<WaylandConnector> wlc;
    pid_t pid;
    std::shared_ptr<dispatch::MultiplexingDispatchable> dispatcher;
    std::shared_ptr<dispatch::ReadableFd> afd_dispatcher;
    std::shared_ptr<dispatch::ReadableFd> fd_dispatcher;
    std::unique_ptr<std::thread> spawn_thread;
    int socket_fd;
    int abstract_socket_fd;
    bool terminate = false;
    Status xserver_status = STOPPED;
    int xserver_spawn_tries = 0;
    std::string const xwayland_path;
};
} /* frontend */
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_SERVER_H */
