/*
 * Copyright (C) Marius Gripsgard <marius@ubports.com>
 * Copyright (C) Canonical Ltd.
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
#include "xwayland_spawner.h"
#include "wayland_connector.h"
#include "mir/log.h"
#include "mir/wayland/client.h"

#include <boost/throw_exception.hpp>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <condition_variable>
#include <cstring>

namespace mf = mir::frontend;
namespace md = mir::dispatch;
namespace mw = mir::wayland;
using namespace std::chrono_literals;

namespace
{
/// Returns a symmetrical pair of connected sockets
auto make_socket_pair() -> std::pair<mir::Fd, mir::Fd>
{
    int pipe[2];
    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, pipe) < 0)
    {
        BOOST_THROW_EXCEPTION(std::system_error(errno, std::system_category(), "Creating socket pair failed"));
    }
    return std::make_pair(mir::Fd{pipe[0]}, mir::Fd{pipe[1]});
}

void exec_xwayland(
    mf::XWaylandSpawner const& spawner,
    std::string const& xwayland_path,
    mir::Fd wayland_client_fd,
    mir::Fd x11_wm_server_fd,
    float scale)
{
    mf::XWaylandSpawner::set_cloexec(wayland_client_fd, false);
    mf::XWaylandSpawner::set_cloexec(x11_wm_server_fd, false);

    setenv("WAYLAND_SOCKET", std::to_string(wayland_client_fd).c_str(), 1);

    auto const x11_wm_server = std::to_string(x11_wm_server_fd);
    auto const dsp_str = spawner.x11_display();

    // This DPI only effects some apps (most app care about GDK_SCALE and other environment variables)
    unsigned const dpi = scale * 96;
    auto const dpi_str = std::to_string(dpi);

    std::vector<char const*> args =
        {
            xwayland_path.c_str(),
            dsp_str.c_str(),
            "-rootless",
            "-dpi",
            dpi_str.c_str(),
            "-wm", x11_wm_server.c_str(),
            "-terminate",
        };

    for (auto const& fd : spawner.socket_fds())
    {
        mf::XWaylandSpawner::set_cloexec(fd, false);
        args.push_back("-listenfd");
        // strdup() may look like a leak, but execvp() will trash all resource management
        args.push_back(strdup(std::to_string(fd).c_str()));
    }

    if (auto const xwayland_option = getenv("MIR_XWAYLAND_OPTION"))
        args.push_back(xwayland_option);

    args.push_back(nullptr);

    execvp(xwayland_path.c_str(), const_cast<char* const*>(args.data()));
    perror(("Failed to execute Xwayland binary: xwayland_path='" + xwayland_path + "'").c_str());
}

auto fork_xwayland_process(
    mf::XWaylandSpawner const& spawner,
    std::string const& xwayland_path,
    float scale) -> mf::XWaylandProcessInfo
{
    mir::log_info("Starting XWayland");
    auto const wayland_socket_pair = make_socket_pair();
    auto const x11_socket_pair = make_socket_pair();
    pid_t const xwayland_pid = fork();

    switch (xwayland_pid)
    {
    case -1:
        BOOST_THROW_EXCEPTION(std::system_error(errno, std::system_category(), "Failed to fork XWayland process"));

    case 0:
        exec_xwayland(spawner, xwayland_path, wayland_socket_pair.second, x11_socket_pair.second, scale);
        // Only reached if Xwayland was not executed
        abort();

    default:
        return mf::XWaylandProcessInfo{
            xwayland_pid,
            wayland_socket_pair.first,
            x11_socket_pair.first};
    }
}

auto connect_xwayland_wl_client(
    std::shared_ptr<mf::WaylandConnector> const& wayland_connector,
    mir::Fd const& wayland_fd,
    float scale) -> wl_client*
{
    struct CreateClientContext
    {
        std::mutex mutex;
        wl_client* client{nullptr};
        bool ready{false};
        std::condition_variable condition_variable;
    };

    auto const ctx = std::make_shared<CreateClientContext>();

    wayland_connector->run_on_wayland_display(
        [ctx, wayland_fd, scale](wl_display* display)
        {
            {
                std::lock_guard lock{ctx->mutex};
                ctx->client = wl_client_create(display, wayland_fd);
                mw::Client::from(ctx->client).set_output_geometry_scale(scale);
                ctx->ready = true;
            }
            ctx->condition_variable.notify_one();
        });

    std::unique_lock client_lock{ctx->mutex};
    if (!ctx->condition_variable.wait_for(client_lock, 10s, [ctx]{ return ctx->ready; }))
    {
        // "Shouldn't happen" but this is better than hanging.
        BOOST_THROW_EXCEPTION(std::runtime_error("Creating XWayland wl_client timed out"));
    }

    if (!ctx->client)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create XWayland wl_client"));
    }

    return ctx->client;
}
}

mf::XWaylandServer::XWaylandServer(
    std::shared_ptr<WaylandConnector> const& wayland_connector,
    XWaylandSpawner const& spawner,
    std::string const& xwayland_path,
    float scale)
    : xwayland{fork_xwayland_process(spawner, xwayland_path, scale)},
      wayland_client{connect_xwayland_wl_client(wayland_connector, xwayland.wayland_fd, scale)},
      running{true}
{
}

mf::XWaylandServer::~XWaylandServer()
{
    mir::log_info("Deiniting xwayland server");

    // Terminate any running xservers
    if (kill(xwayland.pid, SIGTERM) == 0)
    {
        std::this_thread::sleep_for(100ms);// After 100ms...
        if (is_running())
        {
            mir::log_info("Xwayland didn't close, killing it");
            kill(xwayland.pid, SIGKILL);     // ...then kill it!
        }
    }

    // Calling is_running() one more time will ensure the process is reaped
    if (is_running())
    {
        log_warning("Failed to kill Xwayland process with PID %d", xwayland.pid);
    }
}

auto mf::XWaylandServer::is_running() const -> bool
{
    std::lock_guard lock{mutex};

    if (running)
    {
        int status; // Special waitpid() status, not the process exit status
        if (waitpid(xwayland.pid, &status, WNOHANG) != 0)
        {
            running = false;
        }
    }

    return running;
}
