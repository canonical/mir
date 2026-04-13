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

#include <mir/fatal.h>
#include <mir/log.h>

#include <string>
#include <spawn.h>
#include <unistd.h>
#include <signal.h>

#include <cstdlib>
#include <cstring>
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
        mir::fatal_error_abort("Failed to init spawn attributes: %s", strerror(error));
    }

    // Unblock all signals in the child, preserving their existing dispositions
    sigset_t no_signals;
    sigemptyset(&no_signals);

    auto const check_spawn_attr = [&](int result, char const* what)
    {
        if (result != 0)
        {
            posix_spawnattr_destroy(&attr);
            mir::fatal_error_abort("%s: %s", what, strerror(result));
        }
    };

    check_spawn_attr(
        posix_spawnattr_setsigmask(&attr, &no_signals),
        "Failed to set signal mask for spawned process");
    check_spawn_attr(
        posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGMASK),
        "Failed to set spawn flags for spawned process");

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
        mir::log_warning("Failed to execute client (\"%s\") error: %s", exec_args[0], strerror(error));
        // Fork a placeholder child that immediately exits, so the caller can
        // safely call waitpid() on the returned pid (matching the old fork()+exec()
        // contract where a real pid was always returned, even on exec failure).
        // The child calls _exit() directly — no ASAN-instrumented code is run.
        pid_t const placeholder = fork();
        if (placeholder < 0)
            mir::fatal_error_abort("Failed to fork placeholder process: %s", strerror(errno));
        if (placeholder == 0)
            _exit(EXIT_FAILURE);
        return placeholder;
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
