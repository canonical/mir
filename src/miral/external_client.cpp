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

namespace mo = mir::options;

struct miral::ExternalClientLauncher::Self
{
    mir::Server* server = nullptr;
    pid_t pid = -1;
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
    auto const x11_display = self->server->x11_display();
    auto const mir_socket = self->server->mir_socket_name();

    self->pid = launch_app(command_line, wayland_display, mir_socket, x11_display);
}

miral::ExternalClientLauncher::ExternalClientLauncher() : self{std::make_shared<Self>()} {}

auto miral::ExternalClientLauncher::pid() const -> pid_t
{
    return self->pid;
}

void miral::ExternalClientLauncher::launch_using_x11(std::vector<std::string> const& command_line) const
{
    if (!self->server)
        throw std::logic_error("Cannot launch apps when server has not started");

    mir::optional_value<std::string> const wayland_display;
    mir::optional_value<std::string> const mir_socket;

    if (auto const x11_display = self->server->x11_display())
    {
        self->pid = launch_app(command_line, wayland_display, mir_socket, x11_display);
    }
}

miral::ExternalClientLauncher::~ExternalClientLauncher() = default;
