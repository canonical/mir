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

#include "xwayland_connector.h"

#include "mir/log.h"

#include "wayland_connector.h"
#include "xwayland_server.h"
#include "xwayland_spawner.h"
#include "xwayland_wm.h"

namespace mf = mir::frontend;

mf::XWaylandConnector::XWaylandConnector(
    std::shared_ptr<WaylandConnector> const& wayland_connector,
    std::string const& xwayland_path)
    : wayland_connector{wayland_connector},
      xwayland_path{xwayland_path}
{
}

void mf::XWaylandConnector::start()
{
    if (wayland_connector->get_extension("x11-support"))
    {
        std::lock_guard<std::mutex> lock{mutex};

        spawner = std::make_unique<XWaylandSpawner>([this]() { spawn(); });
        mir::log_info("XWayland started");
    }
}

void mf::XWaylandConnector::stop()
{
    std::unique_lock<std::mutex> lock{mutex};

    bool const was_running{server};

    auto local_spawner{std::move(spawner)};
    auto local_wm{std::move(wm)};
    auto local_server{std::move(server)};

    lock.unlock();

    local_spawner.reset();
    local_wm.reset();
    local_server.reset();

    if (was_running)
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

auto mf::XWaylandConnector::socket_name() const -> optional_value<std::string>
{
    std::lock_guard<std::mutex> lock{mutex};

    if (spawner)
    {
        return spawner->x11_display();
    }
    else
    {
        return optional_value<std::string>();
    }
}

mf::XWaylandConnector::~XWaylandConnector() = default;

void mf::XWaylandConnector::spawn()
{
    std::lock_guard<std::mutex> lock{mutex};

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
            xwayland_path);
        wm = std::make_unique<XWaylandWM>(
            wayland_connector,
            server->client(),
            server->wm_fd());
        mir::log_info("XWayland is running");
    }
    catch (...)
    {
        log(
            logging::Severity::error,
            MIR_LOG_COMPONENT,
            std::current_exception(),
            "Spawning XWayland failed");
    }
}
