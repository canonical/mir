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

#include <boost/throw_exception.hpp>

#include <unistd.h>
#include <signal.h>

#include <cstring>
#include <stdexcept>
#include <system_error>
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
}  // namespace

auto miral::launch_app_env(
    std::vector<std::string> const& app,
    mir::optional_value<std::string> const& wayland_display,
    mir::optional_value<std::string> const& x11_display,
    miral::AppEnvironment const& app_env) -> pid_t
{
    pid_t pid = fork();

    if (pid < 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to fork process"}));
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
