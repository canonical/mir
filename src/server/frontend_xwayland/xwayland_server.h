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

#ifndef MIR_FRONTEND_XWAYLAND_SERVER_H
#define MIR_FRONTEND_XWAYLAND_SERVER_H

#include <memory>
#include <thread>

namespace mir
{
namespace dispatch
{
class ReadableFd;
class MultiplexingDispatchable;
} /*dispatch */
namespace frontend
{
class WaylandConnector;
class XWaylandWM;
class XWaylandServer
{
public:
    XWaylandServer(const int xdisp, std::shared_ptr<WaylandConnector> wc);
    ~XWaylandServer();

    enum Status {
      STARTING = 1,
      RUNNING = 2,
      STOPPED = -1,
      FAILED = -2
     };

    void setup_socket();
    void spawn_xserver_on_event_loop();
    void spawn_lazy_xserver();
    std::shared_ptr<dispatch::MultiplexingDispatchable> const get_dispatcher()
    {
        return dispatcher;
    }

    // Ugh, this is ugly!
    static bool xserver_ready;

private:
    void spawn();
    void new_spawn_thread();
    void bind_to_socket();
    void bind_to_abstract_socket();
    int create_lockfile();

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
    bool lazy;
    bool terminate = false;
    Status xserver_status = STOPPED;
    int xserver_spawn_tries = 0;
};
} /* frontend */
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_SERVER_H */
