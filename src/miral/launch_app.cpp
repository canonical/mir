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

#include "launch_app.h"
#include <mir/log.h>

#include <unistd.h>
#include <signal.h>

#include <stdexcept>
#include <cstring>
#include <vector>

namespace
{
void strip_mir_env_variables()
{
    static char const mir_prefix[] = "MIR_";

    std::vector<std::string> vars_to_remove;

    for (auto var = environ; *var; ++var)
    {
        auto const var_begin = *var;
        if (strncmp(var_begin, mir_prefix, sizeof(mir_prefix) - 1) == 0)
        {
            if (auto var_end = strchr(var_begin, '='))
            {
                vars_to_remove.emplace_back(var_begin, var_end);
            }
            else
            {
                vars_to_remove.emplace_back(var_begin);
            }
        }
    }

    for (auto const& var : vars_to_remove)
    {
        unsetenv(var.c_str());
    }
}
}

auto miral::launch_app(
    std::vector<std::string> const& app,
    mir::optional_value<std::string> const& wayland_display,
    mir::optional_value<std::string> const& mir_socket,
    mir::optional_value<std::string> const& x11_display) -> pid_t
{
    pid_t pid = fork();

    if (pid < 0)
    {
        throw std::runtime_error("Failed to fork process");
    }

    if (pid == 0)
    {
        strip_mir_env_variables();

        {
            // POSIX.1-2001 specifies that if the disposition of SIGCHLD is set to
            // SIG_IGN or the SA_NOCLDWAIT flag is set for SIGCHLD, then children
            // that terminate do not become zombie.
            // We don't want any children to become zombies...
            struct sigaction act;
            act.sa_handler = SIG_IGN;
            sigemptyset(&act.sa_mask);
            act.sa_flags = SA_NOCLDWAIT;
            sigaction(SIGCHLD, &act, NULL);
        }

        std::string gdk_backend;
        std::string qt_qpa_platform;
        std::string sdl_videodriver;

        if (x11_display)
        {
            setenv("DISPLAY", x11_display.value().c_str(),  true);   // configure X11 socket
            gdk_backend = "x11";
            qt_qpa_platform = "xcb";
            sdl_videodriver = "x11";
        }
        else
        {
            unsetenv("DISPLAY");
        }

        if (mir_socket)
        {
            setenv("MIR_SOCKET", mir_socket.value().c_str(),  true);   // configure Mir socket
            if (gdk_backend.empty())
            {
                gdk_backend = "mir";
            }
            else
            {
                gdk_backend = "mir," + gdk_backend;
            }

            qt_qpa_platform = "mir";
        }
        else
        {
            unsetenv("MIR_SOCKET");
        }

        if (wayland_display)
        {
            setenv("WAYLAND_DISPLAY", wayland_display.value().c_str(),  true);   // configure Wayland socket
            if (gdk_backend.empty())
            {
                gdk_backend = "wayland";
            }
            else
            {
                gdk_backend = "wayland," + gdk_backend;
            }
            qt_qpa_platform = "wayland";
            sdl_videodriver = "wayland";
        }
        else
        {
            unsetenv("WAYLAND_DISPLAY");
        }

        setenv("GDK_BACKEND", gdk_backend.c_str(), true);
        setenv("QT_QPA_PLATFORM", qt_qpa_platform.c_str(), true);
        setenv("SDL_VIDEODRIVER", sdl_videodriver.c_str(), true);
        setenv("_JAVA_AWT_WM_NONREPARENTING", "1", true);

        std::vector<char const*> exec_args;

        for (auto const& arg : app)
            exec_args.push_back(arg.c_str());

        exec_args.push_back(nullptr);

        execvp(exec_args[0], const_cast<char*const*>(exec_args.data()));

        mir::log_warning("Failed to execute client (\"%s\") error: %s", exec_args[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    return pid;
}

auto miral::launch_app_env(
    std::vector<std::string> const& app, mir::optional_value<std::string> const& wayland_display,
    mir::optional_value<std::string> const& mir_socket, mir::optional_value<std::string> const& x11_display,
    miral::AppEnvironment const& app_env) -> pid_t
{
    pid_t pid = fork();

    if (pid < 0)
    {
        throw std::runtime_error("Failed to fork process");
    }

    if (pid == 0)
    {
        strip_mir_env_variables();

        if (x11_display)
        {
            setenv("DISPLAY", x11_display.value().c_str(),  true);   // configure X11 socket
        }
        else
        {
            unsetenv("DISPLAY");
        }

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

        for (auto const& env : app_env)
        {
            if (env.second)
            {
                setenv(env.first.c_str(), env.second.value().c_str(), true);
            }
            else
            {
                unsetenv(env.first.c_str());
            }
        }

        std::vector<char const*> exec_args;

        for (auto const& arg : app)
            exec_args.push_back(arg.c_str());

        exec_args.push_back(nullptr);

        execvp(exec_args[0], const_cast<char*const*>(exec_args.data()));

        mir::log_warning("Failed to execute client (\"%s\") error: %s", exec_args[0], strerror(errno));
        exit(EXIT_FAILURE);
    }

    return pid;
}
