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
    mir::log_info("Deiniting xwayland server");

    // Terminate any running xservers
    if (xserver_status > 0) {
      terminate = true;

      if (xserver_status == RUNNING)
        wm->destroy();

      if (kill(pid, SIGTERM) == 0)
      {
          usleep(100000);           // After 100ms...
          if (kill(pid, 0) == 0)    // ...if Xwayland is still running...
            kill(pid, SIGKILL);     // ...then kill it!
      }
      spawn_thread->join();
    }

    if (lazy)
      return;

    char path[256];
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
    int status;
    std::string fd_str, abs_fd_str, wm_fd_str;

    xserver_status = STARTING;

    // Try 5 times
    if (xserver_spawn_tries >= 5) {
      mir::log_error("Xwayland failed to start 5 times in a row, disabling Xserver");
      return;
    }
    xserver_status = STARTING;
    xserver_spawn_tries++;

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
    // If not it will have a race condition on start that might end
    // up in a never ending wait
    std::signal(SIGUSR1, [](int) {
        xserver_ready = true;
        mir::log_info("Xwayland running");
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

        set_cloexec(socket_fd, false);
        set_cloexec(abstract_socket_fd, false);

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

        // Last second abort
        if (terminate) return;

        if (lazy)
            execl("/usr/bin/Xwayland",
                  "Xwayland",
                  dsp_str.c_str(),
                  "-rootless",
                  "-listen", abs_fd_str.c_str(),
                  "-listen", fd_str.c_str(),
                  "-wm", wm_fd_str.c_str(),
                  NULL);
        else
            execl("/usr/bin/Xwayland",
                "Xwayland",
                dsp_str.c_str(),
                "-rootless",
                "-listen", abs_fd_str.c_str(),
                "-listen", fd_str.c_str(),
                "-wm", wm_fd_str.c_str(),
                "-terminate",
                NULL);
        break;
    case -1:
        mir::log_error("Failed to fork");
        break;
    default:
        close(wl_client_fd[1]);
        close(wm_fd[1]);
        auto wlclient = wl_client_create(wlc->get_wl_display(), wl_client_fd[0]);

        // More ugliness
        int tries = 0;
        while (!xserver_ready)
        {
          // Last second abort
          if (terminate) return;

          // Check for stalled startup
          if (waitpid(pid, NULL, WNOHANG) != 0 || tries > 200) {
            xserver_status = FAILED;
            mir::log_info("Stalled start of Xserver, trying to start again!");
            spawn();
            return;
          }

          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          tries++;
        }

        // Last second abort
        if (terminate) return;
        wm->start(wlclient, wm_fd[0]);
        mir::log_info("XServer is running");
        xserver_status = RUNNING;

        // Reset the tries since server is running now
        xserver_spawn_tries = 0;

        waitpid(pid, &status, 0);  // Blocking
        if (WIFEXITED(status)) {
           mir::log_info("Xserver stopped");
           xserver_status = STOPPED;
       } else {
          // Failed, crash or killed
          mir::log_info("Xserver crashed or got killed");
          xserver_status = FAILED;
       }

        if (terminate) return;
        wm->destroy();
        xserver_ready = false;

        if (xserver_status == FAILED) {
          mir::log_info("Trying to start Xwayland again!");
          spawn();
          return;
        }

        spawn_xserver_on_event_loop();

        mir::log_info("Xwayland stopped");
        break;
    }
}

bool mf::XWaylandServer::set_cloexec(int fd, bool cloexec) {
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
        mir::log_info("set ok");
      	return true;
}

int mf::XWaylandServer::create_socket(struct sockaddr_un *addr, size_t path_size) {
      int fd;
      socklen_t size = offsetof(struct sockaddr_un, sun_path) + path_size + 1;

    	fd = socket(AF_UNIX, SOCK_STREAM, 0);
    	if (fd < 0) {
    		mir::fatal_error("Failed to create socket %c%s",
    			addr->sun_path[0] ? addr->sun_path[0] : '@',
    			addr->sun_path + 1);
    		return -1;
    	}
    	if (!set_cloexec(fd, true)) {
    		close(fd);
    		return -1;
    	}

    	if (addr->sun_path[0]) {
    		unlink(addr->sun_path);
    	}
    	if (bind(fd, (struct sockaddr*)addr, size) < 0) {
    		mir::fatal_error("Failed to bind socket %c%s",
    			addr->sun_path[0] ? addr->sun_path[0] : '@',
    			addr->sun_path + 1);
          close(fd);
          if (addr->sun_path[0])
            unlink(addr->sun_path);
          return -1;
    	}
    	if (listen(fd, 1) < 0) {
    		mir::fatal_error("Failed to listen to socket %c%s",
    			addr->sun_path[0] ? addr->sun_path[0] : '@',
    			addr->sun_path + 1);
          close(fd);
          if (addr->sun_path[0])
            unlink(addr->sun_path);
          return -1;
    	}

    	return fd;

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
            return EEXIST;
        }

        /* Trim the trailing LF, or at least ensure it's NULL. */
        pid[10] = '\0';

        other = std::stoi(pid);

        if (kill(other, 0) < 0 && errno == ESRCH)
        {
            /* stale lock file; unlink and try again */
            close(fd);
            if (unlink(lockfile))
                return EEXIST;
            else
                return EAGAIN;
        }

        close(fd);
        return EEXIST;
    }
    else if (fd < 0)
        return EEXIST;

    size = dprintf(fd, "%10d\n", getpid());
    if (size != 11)
    {
        unlink(lockfile);
        close(fd);
        return EEXIST;
    }

    close(fd);

    return 0;
}

void mf::XWaylandServer::setup_socket()
{
    char path[256];
    struct sockaddr_un addr;
    size_t path_size;

    int i = create_lockfile();
    while (i == EAGAIN)
    {
        i = create_lockfile();
    }
    if (i != 0) {
      mir::fatal_error("X11 lockfile already exists!");
      return;
    }

    // Make sure we remove the Xserver socket before creating new ones
    snprintf(path, sizeof path, "/tmp/.X11-unix/X%d", xdisplay);
    unlink(path);

    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = 0;
    path_size = snprintf(addr.sun_path + 1, sizeof(addr.sun_path) - 1, "/tmp/.X11-unix/X%d", xdisplay);
    abstract_socket_fd = create_socket(&addr, path_size);
    if (abstract_socket_fd < 0) {
      return;
    }

    path_size = snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/.X11-unix/X%d", xdisplay);
    socket_fd = create_socket(&addr, path_size);
    if (socket_fd < 0) {
      close(abstract_socket_fd);
      abstract_socket_fd = -1;
      return;
    }
}

void mf::XWaylandServer::new_spawn_thread() {
  // Maks sure we don't run the server more then once!
  if (xserver_status > 0) return;
  xserver_status = STARTING;

  spawn_thread = std::make_unique<std::thread>(&mf::XWaylandServer::spawn, this);
}

void mf::XWaylandServer::spawn_xserver_on_event_loop()
{
    auto func = [this]() {
      dispatcher->remove_watch(afd_dispatcher);
      dispatcher->remove_watch(fd_dispatcher);
      afd_dispatcher.reset();
      fd_dispatcher.reset();

      new_spawn_thread();
    };

    afd_dispatcher = std::make_shared<md::ReadableFd>(mir::Fd{mir::IntOwnedFd{abstract_socket_fd}}, func);
    fd_dispatcher = std::make_shared<md::ReadableFd>(mir::Fd{mir::IntOwnedFd{socket_fd}}, func);
    dispatcher->add_watch(afd_dispatcher);
    dispatcher->add_watch(fd_dispatcher);
}

void mf::XWaylandServer::spawn_lazy_xserver()
{
    lazy = true;
    setup_socket();
    new_spawn_thread();
}
