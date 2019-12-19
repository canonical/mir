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

#include "xwayland_server.h"

#include "mir/log.h"

#include "wayland_connector.h"
#include "xwayland_wm.h"

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/readable_fd.h"
#include "mir/fd.h"
#include "mir/terminate_with_current_exception.h"
#include <mir/thread_name.h>

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
using namespace std::chrono_literals;

mf::XWaylandServer::XWaylandServer(
    const int xdisplay,
    std::shared_ptr<mf::WaylandConnector> wc,
    std::string const& xwayland_path) :
    wm(std::make_shared<XWaylandWM>(wc)),
    xdisplay(xdisplay),
    wlc(wc),
    dispatcher{std::make_shared<md::MultiplexingDispatchable>()},
    xwayland_path{xwayland_path}
{
    setup_socket();
    spawn_xserver_on_event_loop();
    xserver_thread = std::make_unique<dispatch::ThreadedDispatcher>(
        "Mir/X11 Reader", dispatcher, []()
            { terminate_with_current_exception(); });
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
    }

    if (spawn_thread && spawn_thread->joinable())
        spawn_thread->join();

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
    set_thread_name("XWaylandServer::spawn");

    enum { server, client, size };
    int wl_client_fd[size], wm_fd[size];

    int xserver_spawn_tries = 0;

    do
    {
        // Try 5 times
        if (xserver_spawn_tries >= 5) {
          mir::log_error("Xwayland failed to start 5 times in a row, disabling Xserver");
          return;
        }
        xserver_status = STARTING;
        xserver_spawn_tries++;

        if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, wl_client_fd) < 0)
        {
            // "Shouldn't happen" but continuing is weird.
            mir::fatal_error("wl connection socketpair failed");
            return; // doesn't reach here
        }

        if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, wm_fd) < 0)
        {
            // "Shouldn't happen" but continuing is weird.
            mir::fatal_error("wm fd socketpair failed");
            return; // doesn't reach here
        }

        mir::log_info("Starting Xwayland");
        pid = fork();

        switch (pid)
        {
        case -1:
            mir::fatal_error("Failed to fork");
            return; // Keep compiler happy (doesn't reach here)

        case 0:
            close(wl_client_fd[server]);
            close(wm_fd[server]);
            execl_xwayland(wl_client_fd[client], wm_fd[client]);
            return; // Keep compiler happy (doesn't reach here)

        default:
            close(wl_client_fd[client]);
            close(wm_fd[client]);
            connect_to_xwayland(wl_client_fd[server], wm_fd[server]);
            if (xserver_status != STARTING)
            {
                // Reset the tries since the server started
                xserver_spawn_tries = 0;
            }

            break;
        }
    }
    while (xserver_status != STOPPED);
}

void mf::XWaylandServer::execl_xwayland(int wl_client_client_fd, int wm_client_fd)
{
    setenv("EGL_PLATFORM", "DRM", 1);

    auto const wl_client_fd = dup(wl_client_client_fd);
    if (wl_client_fd < 0)
        mir::log_error("Failed to duplicate xwayland FD");
    else
        setenv("WAYLAND_SOCKET", std::to_string(wl_client_fd).c_str(), 1);

    set_cloexec(socket_fd, false);
    set_cloexec(abstract_socket_fd, false);

    auto const socket_fd = dup(socket_fd);
    if (socket_fd < 0)
        mir::log_error("Failed to duplicate xwayland FD");
    auto const socket_fd_str = std::to_string(socket_fd);

    auto const abstract_socket_fd = dup(abstract_socket_fd);
    if (abstract_socket_fd < 0)
        mir::log_error("Failed to duplicate xwayland abstract FD");
    auto const abstract_socket_fd_str = std::to_string(abstract_socket_fd);

    auto const wm_fd = dup(wm_client_fd);
    if (wm_fd < 0)
    mir::log_error("Failed to duplicate xwayland wm FD");
    auto const wm_fd_str = std::to_string(wm_fd);

    auto const dsp_str = ":" + std::to_string(xdisplay);

    // Last second abort
    if (terminate) return;

    // Propagate SIGUSR1 to parent process
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = SIG_IGN;
    sigaction(SIGUSR1, &action, nullptr);

    execl(xwayland_path.c_str(),
    "Xwayland",
    dsp_str.c_str(),
    "-rootless",
    "-listen", abstract_socket_fd_str.c_str(),
    "-listen", socket_fd_str.c_str(),
    "-wm", wm_fd_str.c_str(),
    "-terminate",
    NULL);
}

namespace
{
bool spin_wait_for(sig_atomic_t& xserver_ready)
{
    auto const end = std::chrono::steady_clock::now() + 5s;

    while (std::chrono::steady_clock::now() < end && !xserver_ready)
    {
        std::this_thread::sleep_for(100ms);
    }

    return xserver_ready;
}
}

void mf::XWaylandServer::connect_to_xwayland(int wl_client_server_fd, int wm_server_fd)
{
    // We need to set up the signal handling before connecting wl_client_server_fd
    static sig_atomic_t xserver_ready{ false };
    xserver_ready = false;

    struct sigaction action;
    struct sigaction old_action;
    action.sa_handler = [](int) { xserver_ready = true; };
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGUSR1, &action, &old_action);

    wl_client* client = nullptr;
    {
        std::mutex client_mutex;
        std::condition_variable client_ready;

        wlc->run_on_wayland_display(
            [wl_client_server_fd, &client, &client_mutex, &client_ready](wl_display* display)
            {
                std::lock_guard<std::mutex> lock{client_mutex};
                client = wl_client_create(display, wl_client_server_fd);
                client_ready.notify_all();
            });

        std::unique_lock<std::mutex> lock{client_mutex};
        if (!client_ready.wait_for(lock, 10s, [&]{ return client; }))
        {
            // "Shouldn't happen" but this is better than hanging.
            mir::fatal_error("Failed to create wl_client for Xwayland");
        }
    }

    //The client can connect, now wait for it to signal ready (SIGUSR1)
    auto const xwayland_startup_timed_out = !spin_wait_for(xserver_ready);
    sigaction(SIGUSR1, &old_action, nullptr);

    if (xwayland_startup_timed_out)
    {
        mir::log_info("Stalled start of Xserver, trying to start again!");
        close(wm_server_fd);
        return;
    }

    // Last second abort
    if (terminate)
    {
        close(wm_server_fd);
        xserver_status = STOPPED;
        return;
    }

    wm->start(client, wm_server_fd);
    mir::log_info("XServer is running");
    xserver_status = RUNNING;

    int status;
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

