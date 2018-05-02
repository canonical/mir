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

#include "xwayland_server.h"

#define MIR_LOG_COMPONENT "xwaylandserver"
#include "mir/log.h"

#include "wayland_connector.h"
#include "xwayland_wm.h"

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/readable_fd.h"
#include "mir/fd.h"
#include <csignal>
#include <fcntl.h>
#include <memory>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <thread>

namespace mf = mir::frontend;
namespace md = mir::dispatch;

bool mf::XWaylandServer::xserver_ready = false;

mf::XWaylandServer::XWaylandServer(const int xdisplay, std::shared_ptr<mf::WaylandConnector> wc)
    : wm(std::make_shared<XWaylandWM>(wc)),
      xdisplay(xdisplay),
      wlc(wc),
      dispatcher{std::make_shared<md::MultiplexingDispatchable>()}
{
}

mf::XWaylandServer::~XWaylandServer()
{
    mir::log_info("deiniting xwayland server");
    char path[256];

    dispatcher->remove_watch(afd_dispatcher);
    dispatcher->remove_watch(fd_dispatcher);

    snprintf(path, sizeof path, "/tmp/.X%d-lock", xdisplay);
    unlink(path);
    snprintf(path, sizeof path, "/tmp/.X11-unix/X%d", xdisplay);
    unlink(path);
    close(abstract_socket_fd);
    close(socket_fd);
}

void mf::XWaylandServer::spawn()
{
    int fd;
    int wl_client_fd[2], wm_fd[2];
    std::string fd_str, abs_fd_str, wm_fd_str;

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, wl_client_fd) < 0)
    {
        mir::log_error("wl connection socketpair failed");
        return;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, wm_fd) < 0)
    {
        mir::log_error("wm fd socketpair failed");
        return;
    }

    std::ostringstream _dsp_str;
    _dsp_str << ":" << xdisplay;
    auto dsp_str = _dsp_str.str();

    // This is not pretty! but SIGUSR1 is the only way we know the xserver
    // is ready to accept connections to the wm fd
    // If not it will have a rase condidion on start that might end
    // up in a never ending wait
    std::signal(SIGUSR1, [](int) {
        xserver_ready = true;
        mir::log_info("stated");
    });

    mir::log_info("Starting Xwayland");
    pid = fork();
    switch (pid)
    {
    case 0:
        fd = dup(wl_client_fd[1]);
        if (fd < 0)
            mir::log_error("Failed to duplicate xwayland FD");
        setenv("WAYLAND_SOCKET", std::to_string(fd).c_str(), 1);
        setenv("EGL_PLATFORM", "DRM", 1);

        fd = dup(socket_fd);
        if (fd < 0)
            mir::log_error("Failed to duplicate xwayland FD");
        fd_str = std::to_string(fd);

        fd = dup(abstract_socket_fd);
        if (fd < 0)
            mir::log_error("Failed to duplicate xwayland abstract FD");
        abs_fd_str = std::to_string(fd);

        fd = dup(wm_fd[1]);
        if (fd < 0)
            mir::log_error("Failed to duplicate xwayland wm FD");
        wm_fd_str = std::to_string(fd);

        // forward SIGUSR1 to parent thread (us)
        signal(SIGUSR1, SIG_IGN);
            execl("/usr/bin/Xwayland", "Xwayland", dsp_str.c_str(), "-rootless", "-terminate", "-listen",
                  abs_fd_str.c_str(), "-listen", fd_str.c_str(), "-wm", wm_fd_str.c_str(), NULL);
        break;
    case -1:
        mir::log_error("Failed to fork");
        break;
    default:
        close(wl_client_fd[1]);
        auto wlclient = wl_client_create(wlc->get_wl_display(), wl_client_fd[0]);

        close(wm_fd[1]);

        // More ugliness
        // TODO handle xserver startup failures!
        mir::log_info(xserver_ready ? "xs true" : "xs false");
        while (!xserver_ready)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        mir::log_info(xserver_ready ? "xs true" : "xs false");

        int status;

        wm->start(wlclient, wm_fd[0]);
        waitpid(pid, &status, 0);  // Blocking
        wm.reset();
        xserver_ready = false;
        mir::log_info("Xwayland stopped");
        break;
    }
}

void mf::XWaylandServer::bind_to_socket()
{
    struct sockaddr_un addr;
    socklen_t size, name_size;
    addr.sun_family = AF_LOCAL;
    socket_fd = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);

    name_size = snprintf(addr.sun_path, sizeof addr.sun_path, "/tmp/.X11-unix/X%d", xdisplay) + 1;
    size = offsetof(struct sockaddr_un, sun_path) + name_size;
    unlink(addr.sun_path);

    if (bind(socket_fd, (struct sockaddr *)&addr, size) < 0)
    {
        mir::log_error("Failed to bind to x11 socket");
        close(socket_fd);
        return;
    }

    if (listen(socket_fd, 1) < 0)
    {
        mir::log_error("Failed to listen to x11 socket");
        unlink(addr.sun_path);
        close(socket_fd);
        return;
    }
}

void mf::XWaylandServer::bind_to_abstract_socket()
{
    struct sockaddr_un addr;
    socklen_t size, name_size;
    addr.sun_family = AF_LOCAL;

    abstract_socket_fd = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);

    name_size = snprintf(addr.sun_path, sizeof addr.sun_path, "%c/tmp/.X11-unix/X%d", 0, xdisplay);
    size = offsetof(struct sockaddr_un, sun_path) + name_size;
    if (bind(abstract_socket_fd, (struct sockaddr *)&addr, size) < 0)
    {
        mir::log_error("Failed to bind to abstract socket");
        close(abstract_socket_fd);
        return;
    }

    if (listen(abstract_socket_fd, 1) < 0)
    {
        mir::log_error("Failed to listen to abstract socket");
        close(abstract_socket_fd);
        return;
    }
}

// TODO this can be written with more modern c++
int mf::XWaylandServer::create_lockfile()
{
    char lockfile[256];
    char pid[11];
    int fd, size;
    pid_t other;

    snprintf(lockfile, sizeof lockfile, "/tmp/.X%d-lock", xdisplay);
    fd = open(lockfile, O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL, 0444);
    if (fd < 0 && errno == EEXIST)
    {
        fd = open(lockfile, O_CLOEXEC | O_RDONLY);
        if (fd < 0 || read(fd, pid, 11) != 11)
        {
            if (fd >= 0)
                close(fd);

            errno = EEXIST;
            return -1;
        }

        /* Trim the trailing LF, or at least ensure it's NULL. */
        pid[10] = '\0';

        other = std::stoi(pid);

        if (kill(other, 0) < 0 && errno == ESRCH)
        {
            /* stale lock file; unlink and try again */
            close(fd);
            if (unlink(lockfile))
                /* If we fail to unlink, return EEXIST
                   so we try the next display number.*/
                errno = EEXIST;
            else
                errno = EAGAIN;
            return -1;
        }

        close(fd);
        errno = EEXIST;
        return -1;
    }
    else if (fd < 0)
    {
        return -1;
    }

    size = dprintf(fd, "%10d\n", getpid());
    if (size != 11)
    {
        unlink(lockfile);
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}

void mf::XWaylandServer::setup_socket()
{
// TODO I dont really like goto's, should remove
again:
    if (create_lockfile() < 0)
    {
        if (errno == EAGAIN)
        {
            goto again;
        }
        else if (errno == EEXIST)
        {
            mir::log_error("X11 lock alredy exist!");
            return;
        }
        else
        {
            mir::log_error("Failed to create X11 lockfile!");
            return;
        }
    }
    bind_to_abstract_socket();
    bind_to_socket();
}

void mf::XWaylandServer::spawn_xserver_on_event_loop()
{
    afd_dispatcher =
        std::make_shared<md::ReadableFd>(mir::Fd{mir::IntOwnedFd{abstract_socket_fd}}, [this]() { spawn(); });

    fd_dispatcher = std::make_shared<md::ReadableFd>(mir::Fd{mir::IntOwnedFd{socket_fd}}, [this]() { spawn(); });
    dispatcher->add_watch(afd_dispatcher);
    dispatcher->add_watch(fd_dispatcher);
}
