/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miral/external_client.h"

#include "launch_app.h"

#include <mir/server.h>

struct miral::ExternalClientLauncher::Self
{
    mir::Server* server = nullptr;
};

void miral::ExternalClientLauncher::operator()(mir::Server& server)
{
    self->server = &server;
}

void miral::ExternalClientLauncher::launch(std::vector<std::string> const& command_line) const
{
    if (!self->server)
        throw std::logic_error("Cannot launch apps when server has not started");

    auto const wayland_display = self->server->wayland_display();
    auto const mir_socket = self->server->mir_socket_name();

    launch_app(command_line, wayland_display, mir_socket);
}

miral::ExternalClientLauncher::ExternalClientLauncher() : self{std::make_shared<Self>()} {}
miral::ExternalClientLauncher::~ExternalClientLauncher() = default;
