/*
 * Copyright © 2018-2020 Canonical Ltd.
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

#include <mir/options/option.h>
#include <mir/server.h>

#include <map>
#include <stdexcept>

namespace mo = mir::options;

struct miral::ExternalClientLauncher::Self
{
    mir::Server* server = nullptr;
    pid_t pid = -1;

    AppEnvironment env;
    AppEnvironment x11_env;
};

namespace
{
void parse_env(std::string const& value, miral::AppEnvironment& map)
{
    for (auto i = begin(value); i != end(value); )
    {
        auto const j = find(i, end(value), ':');

        auto equals = find(i, j, '=');

        auto const key = std::string(i, equals);
        if (j != equals) ++equals;
        auto const val = std::string(equals, j);

        if (key[0] == '-')
        {
            map[key.substr(1)] = std::experimental::nullopt;
        }
        else
        {
            map[key] = val;
        }

        if ((i = j) != end(value)) ++i;
    }
}
}

void miral::ExternalClientLauncher::operator()(mir::Server& server)
{
    self->server = &server;

    static auto const app_env = "app-env";
    static auto const default_env = "GDK_BACKEND=wayland:QT_QPA_PLATFORM=wayland:SDL_VIDEODRIVER=wayland:_JAVA_AWT_WM_NONREPARENTING=1";
    static auto const app_x11_env = "app-env-x11";
    static auto const default_x11_env = "GDK_BACKEND=x11:QT_QPA_PLATFORM=xcb:SDL_VIDEODRIVER=x11";

    server.add_configuration_option(
        app_env,
        "Environment for launched apps",
        default_env);

    server.add_configuration_option(
        app_x11_env,
        "X11 changes to --app-env for launched apps",
        default_x11_env);

    server.add_init_callback([self=self, &server]
         {
             auto const options = server.get_options();

             parse_env(options->get(app_env, default_env), self->env);

             self->x11_env = self->env; // base the X11 environment on the "normal" one
             parse_env(options->get(app_x11_env, default_x11_env), self->x11_env);
         });
}

void miral::ExternalClientLauncher::launch(std::vector<std::string> const& command_line) const
{
    if (!self->server)
        throw std::logic_error("Cannot launch apps when server has not started");

    auto const wayland_display = self->server->wayland_display();
    auto const x11_display = self->server->x11_display();
    auto const mir_socket = self->server->mir_socket_name();

    self->pid = launch_app_env(command_line, wayland_display, mir_socket, x11_display, self->env);
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


    if (auto const x11_display = self->server->x11_display())
    {
        auto const wayland_display = self->server->wayland_display();
        auto const mir_socket = self->server->mir_socket_name();
        self->pid = launch_app_env(command_line, wayland_display, mir_socket, x11_display, self->x11_env);
    }
}

miral::ExternalClientLauncher::~ExternalClientLauncher() = default;
