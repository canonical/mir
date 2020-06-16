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

#ifndef MIR_FRONTEND_XWAYLAND_SPAWNER_H
#define MIR_FRONTEND_XWAYLAND_SPAWNER_H

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace mir
{
class Fd;
namespace dispatch
{
class ReadableFd;
class ThreadedDispatcher;
class MultiplexingDispatchable;
}
namespace frontend
{

/// Responsible for finding an available X11 display and waiting for connections on it
/// Calls the spawn callback when a client tries to connect
class XWaylandSpawner
{
public:
    XWaylandSpawner(std::function<void()> spawn);
    ~XWaylandSpawner();

    XWaylandSpawner(XWaylandSpawner const&) = delete;
    XWaylandSpawner& operator=(XWaylandSpawner const&) = delete;

    /// The name of the X11 display (such as ":0")
    auto x11_display() const -> std::string;
    /// \returns whatever sockets we're waiting on
    /// (on construction we try to open both an abstract and non-abstrack socket)
    auto socket_fds() const -> std::vector<Fd> const&;

    /// Enables or disables the CLOEXEC flag for the given fd
    /// \returns if the operation succeeded
    static bool set_cloexec(mir::Fd const& fd, bool cloexec);

private:
    int const xdisplay;
    std::vector<Fd> const fds;
    std::shared_ptr<dispatch::MultiplexingDispatchable> const dispatcher;
    std::unique_ptr<dispatch::ThreadedDispatcher> const spawn_thread;
    std::vector<std::shared_ptr<dispatch::ReadableFd>> const dispatcher_fd;
};

}
}

#endif // MIR_FRONTEND_XWAYLAND_SPAWNER_H
