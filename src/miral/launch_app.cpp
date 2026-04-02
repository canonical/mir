/*
 * Copyright © Canonical Ltd.
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
 */

#include "launch_app.h"

#include <boost/throw_exception.hpp>

#include <string>
#include <spawn.h>
#include <unistd.h>
#include <signal.h>

#include <cstring>
#include <system_error>
#include <vector>

namespace
{
class Environment
{
public:
    Environment();

    void setenv(std::string const& name, std::string const& value);
    void unsetenv(std::string const& name);
    auto exec_env() const & -> std::vector<char const*>;
private:
    std::vector<std::string> env_strings;
};

Environment::Environment()
{
    static char const mir_prefix[] = "MIR_";

    for (auto var = environ; *var; ++var)
    {
        auto const var_begin = *var;
        if (strncmp(var_begin, mir_prefix, sizeof(mir_prefix) - 1) != 0)
        {
            env_strings.emplace_back(var_begin);
        }
    }
}

void Environment::setenv(std::string const& name, std::string const& value)
{
    auto const entry = name + "=" + value;

    for (auto& e : env_strings)
    {
        if (strncmp(e.c_str(), name.c_str(), name.size()) == 0)
        {
            e = entry;
            return;
        }
    }

    env_strings.emplace_back(entry);
}

void Environment::unsetenv(std::string const& name)
{
    for (auto it = env_strings.begin(); it != env_strings.end(); ++it)
    {
        if (strncmp(it->c_str(), name.c_str(), name.size()) == 0)
        {
            env_strings.erase(it);
            return;
        }
    }
}

auto Environment::exec_env() const & -> std::vector<char const*>
{
    std::vector<char const*> result;

    for (auto const& arg : env_strings)
        result.push_back(arg.c_str());

    result.push_back(nullptr);
    return result;
}

void assign_or_unset(Environment& env, std::string const& key, auto optional_value)
{
    if (optional_value)
    {
        env.setenv(key, optional_value.value());
    }
    else
    {
        env.unsetenv(key);
    }
}

auto execute_with_environment(std::vector<std::string> const app, Environment& application_environment) -> pid_t
{
    auto const exec_env = application_environment.exec_env();

    std::vector<char const*> exec_args;

    for (auto const& arg : app)
        exec_args.push_back(arg.c_str());

    exec_args.push_back(nullptr);

    // Use posix_spawnp() instead of fork()+exec() to avoid ASAN deadlocks.
    // When fork() is called from a multithreaded process, ASAN's internal mutexes
    // locked by other threads remain locked in the child, causing deadlock before
    // exec() can run. posix_spawnp() uses vfork()/clone() internally, bypassing
    // this issue.
    posix_spawnattr_t attr;
    if (auto const error = posix_spawnattr_init(&attr))
    {
        BOOST_THROW_EXCEPTION((std::system_error{error, std::system_category(), "Failed to init spawn attributes"}));
    }

    // Unblock all signals and reset their dispositions to defaults in the child
    sigset_t all_signals;
    sigfillset(&all_signals);
    sigset_t no_signals;
    sigemptyset(&no_signals);
    // These calls only fail if attr is null/invalid (it isn't) or if flags are invalid (they aren't)
    posix_spawnattr_setsigdefault(&attr, &all_signals);
    posix_spawnattr_setsigmask(&attr, &no_signals);
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK);

    pid_t pid = -1;
    auto const error = posix_spawnp(
        &pid,
        exec_args[0],
        nullptr,
        &attr,
        const_cast<char* const*>(exec_args.data()),
        const_cast<char* const*>(exec_env.data()));

    posix_spawnattr_destroy(&attr);

    if (error != 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{error, std::system_category(), "Failed to spawn process"}));
    }

    return pid;
}
}  // namespace


auto miral::launch_app_env(
    std::vector<std::string> const& app,
    std::optional<std::string> const& wayland_display,
    std::optional<std::string> const& x11_display,
    std::optional<std::string> const& xdg_activation_token,
    miral::AppEnvironment const& app_env) -> pid_t
{
    Environment application_environment;

    for (auto const& [key, value]: app_env)
        assign_or_unset(application_environment, key, value);

    assign_or_unset(application_environment, "WAYLAND_DISPLAY", wayland_display);           // configure Wayland socket
    assign_or_unset(application_environment, "DISPLAY", x11_display);                       // configure X11 socket
    assign_or_unset(application_environment, "XDG_ACTIVATION_TOKEN", xdg_activation_token);
    assign_or_unset(application_environment, "DESKTOP_STARTUP_ID", xdg_activation_token);

    return execute_with_environment(app, application_environment);
}
