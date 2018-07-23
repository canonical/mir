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

#include "xwayland_connector.h"

#define MIR_LOG_COMPONENT "xwaylandconnector"
#include "mir/log.h"

#include "mir/optional_value.h"
#include "mir/terminate_with_current_exception.h"
#include "wayland_connector.h"
#include "xwayland_server.h"

namespace mf = mir::frontend;

mf::XWaylandConnector::XWaylandConnector(const int xdisplay, std::shared_ptr<mf::WaylandConnector> wc)
    : enabled(!!wc->get_extension("x11-support"))
{
    if (enabled)
        xwayland_server = std::make_shared<mf::XWaylandServer>(xdisplay, wc);
}

void mf::XWaylandConnector::start()
{
    if (!enabled)
        return;

    if (getenv("MIR_X11_LAZY"))
    {
        xwayland_server->spawn_lazy_xserver();
        return;
    }

    xwayland_server->setup_socket();
    xwayland_server->spawn_xserver_on_event_loop();
    xserver_thread = std::make_unique<mir::dispatch::ThreadedDispatcher>(
        "Mir/X11 Reader", xwayland_server->get_dispatcher(), []() { mir::terminate_with_current_exception(); });
    mir::log_info("XWayland loop started");
}
void mf::XWaylandConnector::stop()
{
    if (!enabled)
        return;

    xwayland_server.reset();
    xserver_thread.reset();
}
int mf::XWaylandConnector::client_socket_fd() const
{
    return -1;
}
int mf::XWaylandConnector::client_socket_fd(
    std::function<void(std::shared_ptr<Session> const& session)> const& /*connect_handler*/) const
{
    return -1;
}

auto mf::XWaylandConnector::socket_name() const -> optional_value<std::string>
{
    return optional_value<std::string>();
}
