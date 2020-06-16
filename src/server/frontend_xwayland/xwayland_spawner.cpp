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

#include "xwayland_spawner.h"

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/threaded_dispatcher.h"
#include "mir/dispatch/readable_fd.h"
#include "mir/fd.h"
#include "mir/log.h"
#include "mir/fatal.h"
#include "mir/thread_name.h"

#include <csignal>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

namespace mf = mir::frontend;
namespace md = mir::dispatch;

namespace
{
auto const x11_lock_fmt = "/tmp/.X%d-lock";
auto const x11_socket_fmt = "/tmp/.X11-unix/X%d";

// TODO this can be written with more modern c++
int create_lockfile(int xdisplay)
{
    char lockfile[256];

    snprintf(lockfile, sizeof lockfile, x11_lock_fmt, xdisplay);

    mir::Fd const fd{open(lockfile, O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL, 0444)};
    if (fd < 0)
        return EEXIST;

    // We format for consistency with the corresponding code in xorg-server (os/utils.c) which
    // requires 11 characters. Vis:
    //
    //    snprintf(pid_str, sizeof(pid_str), "%10lu\n", (unsigned long) getpid());
    //    if (write(lfd, pid_str, 11) != 11)
    bool const success_writing_to_lock_file = dprintf(fd, "%10lu\n", (unsigned long) getpid()) == 11;

    // Check if anyone else has created the socket (even though we have the lockfile)
    char x11_socket[256];
    snprintf(x11_socket, sizeof x11_socket, x11_socket_fmt, xdisplay);

    if (!success_writing_to_lock_file || access(x11_socket, F_OK) == 0)
    {
        unlink(lockfile);
        return EEXIST;
    }

    return 0;
}

auto choose_display() -> int
{
    for (auto xdisplay = 0; xdisplay != 10000; ++xdisplay)
    {
        if (create_lockfile(xdisplay) == 0) return xdisplay;
    }

    mir::fatal_error("Cannot create X11 lockfile!");
    return -1;
}

auto create_socket(std::vector<mir::Fd>& fds, struct sockaddr_un *addr, size_t path_size)
{
    socklen_t size = offsetof(struct sockaddr_un, sun_path) + path_size + 1;

    mir::Fd const fd{socket(AF_UNIX, SOCK_STREAM, 0)};

    if (fd < 0)
    {
        mir::log_warning(
            "Failed to create socket %c%s",
            addr->sun_path[0] ? addr->sun_path[0] : '@',
            addr->sun_path + 1);
        return;
    }

    if (!mf::XWaylandSpawner::set_cloexec(fd, true))
    {
        return;
    }

    if (addr->sun_path[0])
    {
        unlink(addr->sun_path);
    }

    if (bind(fd, (struct sockaddr*)addr, size) < 0)
    {
        mir::log_warning(
            "Failed to bind socket %c%s",
            addr->sun_path[0] ? addr->sun_path[0] : '@',
            addr->sun_path + 1);
        if (addr->sun_path[0])
        {
            unlink(addr->sun_path);
        }
        return;
    }

    if (listen(fd, 1) < 0)
    {
        mir::log_warning(
            "Failed to listen to socket %c%s",
            addr->sun_path[0] ? addr->sun_path[0] : '@',
            addr->sun_path + 1);
        if (addr->sun_path[0])
        {
            unlink(addr->sun_path);
        }
        return;
    }

    fds.push_back(fd);
}

auto create_sockets(int xdisplay) -> std::vector<mir::Fd>
{
    std::vector<mir::Fd> result;
    struct sockaddr_un addr;
    size_t path_size;

    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = 0;
    path_size = snprintf(addr.sun_path + 1, sizeof(addr.sun_path) - 1, x11_socket_fmt, xdisplay);
    create_socket(result, &addr, path_size);

    path_size = snprintf(addr.sun_path, sizeof(addr.sun_path), x11_socket_fmt, xdisplay);
    create_socket(result, &addr, path_size);

    return result;
}

auto create_dispatchers(
    std::vector<mir::Fd> const& fds,
    std::function<void()> callback) -> std::vector<std::shared_ptr<md::ReadableFd>>
{
    std::vector<std::shared_ptr<md::ReadableFd>> result;
    for (auto const& fd : fds)
    {
        result.push_back(std::make_shared<md::ReadableFd>(fd, [callback]{ callback(); }));
    }
    return result;
}
}

mf::XWaylandSpawner::XWaylandSpawner(std::function<void()> spawn)
    : xdisplay{choose_display()},
      fds{create_sockets(xdisplay)},
      dispatcher{std::make_shared<md::MultiplexingDispatchable>()},
      spawn_thread{std::make_unique<dispatch::ThreadedDispatcher>(
          "Mir/X11 Spawner",
          dispatcher,
          []()
          {
              log(
                logging::Severity::error,
                MIR_LOG_COMPONENT,
                std::current_exception(),
                "Failed to spawn XWayland server.");
          })},
      dispatcher_fd{create_dispatchers(fds, spawn)}
{
    if (dispatcher_fd.empty())
    {
        // To get here, we were configured with "enable-x11". So this is fatal.
        mir::fatal_error("Cannot open any X11 socket (abstract or not)");
    }

    for (auto const& fd_dispatcher : dispatcher_fd)
    {
        dispatcher->add_watch(fd_dispatcher);
    }
}

mf::XWaylandSpawner::~XWaylandSpawner()
{
    char path[256];
    snprintf(path, sizeof path, x11_lock_fmt, xdisplay);
    unlink(path);
    snprintf(path, sizeof path, x11_socket_fmt, xdisplay);
    unlink(path);
}

auto mf::XWaylandSpawner::x11_display() const -> std::string
{
    return std::string(":") + std::to_string(xdisplay);
}

auto mf::XWaylandSpawner::socket_fds() const -> std::vector<Fd> const&
{
    return fds;
}

bool mf::XWaylandSpawner::set_cloexec(mir::Fd const& fd, bool cloexec)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags == -1) {
        mir::fatal_error("fcntl failed");
        return false;
    }
    if (cloexec) {
        flags = flags | FD_CLOEXEC;
    } else {
        flags = flags & ~FD_CLOEXEC;
    }
    if (fcntl(fd, F_SETFD, flags) == -1) {
        mir::fatal_error("fcntl failed");
        return false;
    }
    return true;
}
