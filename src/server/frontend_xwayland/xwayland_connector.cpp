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

#include "xwayland_connector.h"

#include "wayland_connector.h"
#include "xwayland_server.h"
#include "xwayland_spawner.h"
#include "xwayland_wm.h"
#include "mir/log.h"
#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/dispatch/readable_fd.h"
#include "mir/executor.h"

#include <unistd.h>

namespace mf = mir::frontend;
namespace md = mir::dispatch;

mf::XWaylandConnector::XWaylandConnector(
    std::shared_ptr<Executor> const& main_loop,
    std::shared_ptr<WaylandConnector> const& wayland_connector,
    std::string const& xwayland_path,
    float scale)
    : main_loop{main_loop},
      wayland_connector{wayland_connector},
      xwayland_path{xwayland_path},
      scale{scale}
{
    if (access(xwayland_path.c_str(), F_OK | X_OK) != 0)
    {
        fatal_error("Cannot execute Xwayland: --xwayland-path %s", xwayland_path.c_str());
    }
}

mf::XWaylandConnector::~XWaylandConnector()
{
    if (is_started || spawner || server || wm || wm_event_thread)
    {
        fatal_error(
            "XWaylandConnector was not stopped before being destroyed "
            "(is_started: %s, spawner: %s, server: %s, wm: %s, wm_event_thread: %s)",
            is_started ? "yes" : "no",
            spawner ? "exists" : "null",
            wm ? "exists" : "null",
            spawner ? "exists" : "null",
            wm_event_thread ? "exists" : "null");
    }
}

void mf::XWaylandConnector::start()
{
    if (wayland_connector->get_extension("x11-support"))
    {
        std::unique_lock lock{mutex};
        is_started = true;
        maybe_create_spawner(lock);
        mir::log_info("XWayland started on X11 display %s", spawner->x11_display().c_str());
    }
}

void mf::XWaylandConnector::stop()
{
    std::unique_lock lock{mutex};

    bool const was_started = is_started;
    is_started = false;

    clean_up(std::move(lock));
    // note that lock is no longer locked now

    if (was_started)
    {
        mir::log_info("XWayland stopped");
    }
}

int mf::XWaylandConnector::client_socket_fd() const
{
    return -1;
}

int mf::XWaylandConnector::client_socket_fd(
    std::function<void(std::shared_ptr<scene::Session> const& session)> const& /*connect_handler*/) const
{
    return -1;
}

auto mf::XWaylandConnector::socket_name() const -> std::optional<std::string>
{
    std::lock_guard lock{mutex};

    if (spawner)
    {
        return spawner->x11_display();
    }
    else
    {
        return std::nullopt;
    }
}

void mf::XWaylandConnector::maybe_create_spawner(std::unique_lock<std::mutex> const& lock)
{
    if (!lock.owns_lock())
    {
        fatal_error("XWaylandConnector::maybe_create_spawner() given unlocked lock");
    }

    if (is_started && !spawner)
    {
        // If we should be running but a spawner does not exist, create one
        spawner = std::make_unique<XWaylandSpawner>([this]() { spawn(); });
    }
}

void mf::XWaylandConnector::clean_up(std::unique_lock<std::mutex> lock)
{
    if (!lock.owns_lock())
    {
        fatal_error("XWaylandConnector::clean_up() given unlocked lock");
    }

    auto local_server{std::move(server)};
    auto local_wm{std::move(wm)};
    auto local_wm_event_thread{std::move(wm_event_thread)};
    auto local_spawner{std::move(spawner)};

    lock.unlock();

    // Local objects are now dropped with the mutex not locked
}

void mf::XWaylandConnector::spawn()
{
    std::unique_lock lock{mutex};

    if (server || !spawner)
    {
        // If we have a server then we've already spawned
        // If we don't have a spawner then this connector has been stopped or never started (and shouldn't spawn)
        // Either way, nothing to do
        return;
    }

    try
    {
        server = std::make_unique<XWaylandServer>(
            wayland_connector,
            *spawner,
            xwayland_path,
            scale);
        auto const wm_dispatcher = std::make_shared<md::MultiplexingDispatchable>();
        wm_dispatcher->add_watch(std::make_shared<md::ReadableFd>(server->x11_wm_fd(), [this]()
            {
                std::lock_guard lock{mutex};
                if (wm)
                {
                    wm->handle_events();
                }
            }));
        wm_event_thread = std::make_unique<mir::dispatch::ThreadedDispatcher>(
            "Mir/X11 WM Reader",
            wm_dispatcher,
            [this]()
            {
                // The window manager threw an exception handling X11 events

                log(
                    logging::Severity::error,
                    MIR_LOG_COMPONENT,
                    std::current_exception(),
                    "X11 window manager error");

                // This lambda is called by the window manager event dispatcher. It is a runtime error to destroy a
                // ThreadedDispatcher from its own call, so we clean up on the main loop.
                main_loop->spawn([weak_self=weak_from_this()]()
                    {
                        if (auto const self = weak_self.lock())
                        {
                            self->clean_up(std::unique_lock{self->mutex});
                            log_info("Restarting XWayland");
                            self->maybe_create_spawner(std::unique_lock{self->mutex});
                        }
                    });
            });
        wm = std::make_unique<XWaylandWM>(
            wayland_connector,
            server->client(),
            server->x11_wm_fd(),
            wm_dispatcher,
            scale);
        mir::log_info("XWayland is running");
    }
    catch (...)
    {
        log(
            logging::Severity::error,
            MIR_LOG_COMPONENT,
            std::current_exception(),
            "Spawning XWayland failed");

        // This lambda is called by the spawner's ThreadedDispatcher. It is a runtime error to destroy a
        // ThreadedDispatcher from its own call, so we clean up on the main loop.
        main_loop->spawn([weak_self=weak_from_this()]()
            {
                if (auto const self = weak_self.lock())
                {
                    self->clean_up(std::unique_lock{self->mutex});
                    log_info("Restarting XWayland");
                    // XWaylandConnector::spawn() is only called when a client tries to connect, so restarting the
                    // on failure should not result in an endless loop.
                    self->maybe_create_spawner(std::unique_lock{self->mutex});
                }
            });
    }
}
