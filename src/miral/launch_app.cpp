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

#include "launch_app.h"

#include <unistd.h>

#include <stdexcept>
#include <cstring>


void miral::launch_app(std::vector<std::string> const& app,
                       mir::optional_value<std::string> const& wayland_display,
                       mir::optional_value<std::string> const& mir_socket)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        throw std::runtime_error("Failed to fork process");
    }

    if (pid == 0)
    {
        if (mir_socket)
        {
            setenv("MIR_SOCKET", mir_socket.value().c_str(),  true);   // configure Mir socket
        }
        else
        {
            unsetenv("MIR_SOCKET");
        }

        if (wayland_display)
        {
            setenv("WAYLAND_DISPLAY", wayland_display.value().c_str(),  true);   // configure Wayland socket
        }
        else
        {
            unsetenv("WAYLAND_DISPLAY");
        }

        setenv("GDK_BACKEND", "wayland,mir", true);         // configure GTK to use Wayland (or Mir)
        setenv("QT_QPA_PLATFORM", "wayland", true);         // configure Qt to use Wayland
        unsetenv("QT_QPA_PLATFORMTHEME");                   // Discourage Qt from unsupported theme
        setenv("SDL_VIDEODRIVER", "wayland", true);         // configure SDL to use Wayland

        std::vector<char const*> exec_args;

        for (auto const& arg : app)
            exec_args.push_back(arg.c_str());

        exec_args.push_back(nullptr);

        execvp(exec_args[0], const_cast<char*const*>(exec_args.data()));

        throw std::logic_error(std::string("Failed to execute client (") + exec_args[0] + ") error: " + strerror(errno));
    }
}


