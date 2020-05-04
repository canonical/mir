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

#include "xwayland_server.h"
#include "xwayland_wm.h"

#include "wayland_connector.h"

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/readable_fd.h"
#include "mir/fd.h"
#include "mir/log.h"
#include "mir/terminate_with_current_exception.h"
#include <mir/thread_name.h>

#include <csignal>
#include <fcntl.h>
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
    std::shared_ptr<mf::WaylandConnector> wayland_connector,
    std::string const& xwayland_path) :
    wayland_connector{wayland_connector},
    dispatcher{std::make_shared<md::MultiplexingDispatchable>()},
    xserver_thread{std::make_unique<dispatch::ThreadedDispatcher>(
        "Mir/X11 Reader", dispatcher, []() { terminate_with_current_exception(); })},
    sockets{},
    dispatcher_fd{},
    xwayland_path{xwayland_path}
{
    for (auto const& fd : sockets.fd)
    {
        dispatcher_fd.push_back(std::make_shared<md::ReadableFd>(fd, [this]{ new_spawn_thread(); }));
    }

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

mf::XWaylandServer::~XWaylandServer()
{
    mir::log_info("Deiniting xwayland server");

    // Terminate any running xservers
    {
        std::lock_guard<decltype(spawn_thread_mutex)> lock(spawn_thread_mutex);

        if (spawn_thread_xserver_status > 0)
        {
            spawn_thread_terminate = true;

            if (kill(spawn_thread_pid, SIGTERM) == 0)
            {
                std::this_thread::sleep_for(100ms);// After 100ms...
                if (kill(spawn_thread_pid, 0) == 0)    // ...if Xwayland is still running...
                {
                    mir::log_info("Xwayland didn't close, killing it");
                    kill(spawn_thread_pid, SIGKILL);     // ...then kill it!
                }
            }
        }
    }

    if (spawn_thread.joinable())
        spawn_thread.join();
}

void mf::XWaylandServer::spawn()
{
    set_thread_name("XWaylandServer::spawn");

    enum { server, client, size };
    int wl_client_fd[size], wm_fd[size];

    int xserver_spawn_tries = 0;
    std::unique_lock<decltype(spawn_thread_mutex)> lock{spawn_thread_mutex};

    do
    {
        // Try 5 times
        if (xserver_spawn_tries >= 5) {
          mir::log_error("Xwayland failed to start 5 times in a row, disabling Xserver");
          return;
        }
        spawn_thread_xserver_status = STARTING;
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
        spawn_thread_pid = fork();

        switch (spawn_thread_pid)
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
            connect_wm_to_xwayland(wl_client_fd[server], wm_fd[server], lock);
            if (spawn_thread_xserver_status != STARTING)
            {
                // Reset the tries since the server started
                xserver_spawn_tries = 0;
            }

            break;
        }
    }
    while (spawn_thread_xserver_status != STOPPED);
}

namespace
{
bool set_cloexec(int fd, bool cloexec) {
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
}

void mf::XWaylandServer::execl_xwayland(int wl_client_client_fd, int wm_client_fd)
{
    setenv("EGL_PLATFORM", "DRM", 1);

    auto const wl_client_fd = dup(wl_client_client_fd);
    if (wl_client_fd < 0)
        mir::log_error("Failed to duplicate xwayland FD");
    else
        setenv("WAYLAND_SOCKET", std::to_string(wl_client_fd).c_str(), 1);

    auto const wm_fd = dup(wm_client_fd);
    if (wm_fd < 0)
    mir::log_error("Failed to duplicate xwayland wm FD");
    auto const wm_fd_str = std::to_string(wm_fd);

    auto const dsp_str = ":" + std::to_string(sockets.xdisplay);

    // Propagate SIGUSR1 to parent process
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = SIG_IGN;
    sigaction(SIGUSR1, &action, nullptr);

    std::vector<char const*> args =
        {
            xwayland_path.c_str(),
            dsp_str.c_str(),
            "-rootless",
            "-wm", wm_fd_str.c_str(),
            "-terminate",
        };

    for (auto const& fd : sockets.fd)
    {
        set_cloexec(fd, false);
        args.push_back("-listen");
        // strdup() may look like a leak, but execvp() will trash all resource management
        args.push_back(strdup(std::to_string(fd).c_str()));
    }

    args.push_back("-wm");
    args.push_back(wm_fd_str.c_str());
    args.push_back("-terminate");
    args.push_back(NULL);

    execvp(xwayland_path.c_str(), const_cast<char* const*>(args.data()));
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

void mf::XWaylandServer::connect_wm_to_xwayland(
    int wl_client_server_fd, int wm_server_fd, std::unique_lock<decltype(spawn_thread_mutex)>& spawn_thread_lock)
{
    // We need to set up the signal handling before connecting wl_client_server_fd
    static sig_atomic_t xserver_ready{ false };

    // In practice, there ought to be no contention on xserver_ready, but let's be certain
    static std::mutex xserver_ready_mutex;
    std::lock_guard<decltype(xserver_ready_mutex)> lock{xserver_ready_mutex};
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

        wayland_connector->run_on_wayland_display(
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

    try
    {
        XWaylandWM wm{wayland_connector, client, wm_server_fd};
        mir::log_info("XServer is running");
        spawn_thread_xserver_status = RUNNING;
        auto const pid = spawn_thread_pid; // For clarity only as this is only written on this thread

        // Unlock access to spawn_thread_* while Xwayland is running
        spawn_thread_lock.unlock();
        int status;
        waitpid(pid, &status, 0);  // Blocking
        spawn_thread_lock.lock();

        if (WIFEXITED(status) || spawn_thread_terminate) {
            mir::log_info("Xserver stopped");
            spawn_thread_xserver_status = STOPPED;
        } else {
            // Failed, crash or killed
            mir::log_info("Xserver crashed or got killed");
            spawn_thread_xserver_status = FAILED;
        }
    }
    catch (std::exception const& e)
    {
        mir::log_error("X11 failed: %s", e.what());
        // don't touch spawn_thread_xserver_status
        // it should be left in a STARTING state to the caller knows we didn't successfully start'
    }
}

namespace
{
void create_socket(std::vector<mir::Fd>& fds, struct sockaddr_un *addr, size_t path_size)
{
    int fd;
    socklen_t size = offsetof(struct sockaddr_un, sun_path) + path_size + 1;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
    {
        mir::log_warning(
            "Failed to create socket %c%s", addr->sun_path[0] ? addr->sun_path[0] : '@', addr->sun_path + 1);
        return;
    }
    if (!set_cloexec(fd, true))
    {
        close(fd);
        return;
    }

    if (addr->sun_path[0])
    {
        unlink(addr->sun_path);
    }

    if (bind(fd, (struct sockaddr*)addr, size) < 0)
    {
        mir::log_warning("Failed to bind socket %c%s", addr->sun_path[0] ? addr->sun_path[0] : '@', addr->sun_path + 1);
        close(fd);
        if (addr->sun_path[0])
            unlink(addr->sun_path);
        return;
    }
    if (listen(fd, 1) < 0)
    {
        mir::log_warning(
            "Failed to listen to socket %c%s", addr->sun_path[0] ? addr->sun_path[0] : '@', addr->sun_path + 1);
        close(fd);
        if (addr->sun_path[0])
            unlink(addr->sun_path);
        return;
    }

    fds.push_back(mir::Fd{fd});
}

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
}

mf::XWaylandServer::SocketFd::SocketFd() :
    xdisplay{choose_display()}
{
    struct sockaddr_un addr;
    size_t path_size;

    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = 0;
    path_size = snprintf(addr.sun_path + 1, sizeof(addr.sun_path) - 1, x11_socket_fmt, xdisplay);
    create_socket(fd, &addr, path_size);

    path_size = snprintf(addr.sun_path, sizeof(addr.sun_path), x11_socket_fmt, xdisplay);
    create_socket(fd, &addr, path_size);
}

mf::XWaylandServer::SocketFd::~SocketFd()
{
    char path[256];
    snprintf(path, sizeof path, x11_lock_fmt, xdisplay);
    unlink(path);
    snprintf(path, sizeof path, x11_socket_fmt, xdisplay);
    unlink(path);
}

void mf::XWaylandServer::new_spawn_thread()
{
    std::lock_guard<decltype(spawn_thread_mutex)> lock(spawn_thread_mutex);

    // Don't run the server more then once!
    if (spawn_thread_xserver_status > 0) return;

    spawn_thread_xserver_status = STARTING;

    if (spawn_thread.joinable()) spawn_thread.join();
    spawn_thread = std::thread{&mf::XWaylandServer::spawn, this};
}

auto mir::frontend::XWaylandServer::x11_display() const -> std::string
{
    return std::string(":") + std::to_string(sockets.xdisplay);
}
