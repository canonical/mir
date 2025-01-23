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
#include <mir/log.h>

#include <boost/throw_exception.hpp>

#include <string>
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

    pid_t pid = fork();

    if (pid < 0)
    {
        BOOST_THROW_EXCEPTION((std::system_error{errno, std::system_category(), "Failed to fork process"}));
    }

    if (pid == 0)
    {
        sigset_t all_signals;
        sigfillset(&all_signals);
        pthread_sigmask(SIG_UNBLOCK, &all_signals, nullptr);

        // execvpe() isn't listed as being async-signal-safe, but the implementation looks fine and rewriting seems
        // unnecessary
        execvpe(exec_args[0], const_cast<char* const*>(exec_args.data()), const_cast<char* const*>(exec_env.data()));

        mir::log_warning("Failed to execute client (\"%s\") error: %s", exec_args[0], strerror(errno));
        exit(EXIT_FAILURE);
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
