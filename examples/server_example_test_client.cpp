/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_test_client.h"
#include "mir/server.h"
#include "mir/main_loop.h"
#include "mir/logging/logger.h"
#include "mir/options/option.h"

#include <csignal>
#include <sys/wait.h>

#include <chrono>

namespace me = mir::examples;
namespace ml = mir::logging;

namespace
{
static auto const component = "server_example_test_client.cpp";

bool exit_success(pid_t pid)
{
    int status;

    auto const wait_rc = waitpid(pid, &status, WNOHANG);

    if (wait_rc == 0)
    {
        ml::log(ml::Severity::informational, "Terminating client", component);
        kill(pid, SIGKILL);
    }
    else if (wait_rc != pid)
    {
        ml::log(ml::Severity::informational, "No status available for client", component);
        return false;
    }
    else if (WIFEXITED(status))
    {
        auto const exit_status = WEXITSTATUS(status);
        if (exit_status != EXIT_SUCCESS)
        {
            char const format[] = "Client has exited with status %d";
            char buffer[sizeof format + 10];
            snprintf(buffer, sizeof buffer, format, exit_status);
            ml::log(ml::Severity::informational, buffer, component);
            return false;
        }
    }
    else if (WIFSIGNALED(status))
    {
        auto const signal = WTERMSIG(status);
        char const format[] = "Client terminated by signal %d";
        char buffer[sizeof format];
        snprintf(buffer, sizeof buffer, format, signal);
        ml::log(ml::Severity::informational, buffer, component);
        return false;
    }
    else
    {
        ml::log(ml::Severity::informational, "Client died mysteriously", component);
        return false;
    }

    return true;
}
}

void me::add_test_client_option_to(mir::Server& server, me::ClientContext& context)
{
    static const char* const test_client_opt = "test-client";
    static const char* const test_client_descr = "client executable";

    static const char* const test_timeout_opt = "test-timeout";
    static const char* const test_timeout_descr = "Seconds to run before sending SIGTERM to client";

    server.add_configuration_option(test_client_opt, test_client_descr, mir::OptionType::string);
    server.add_configuration_option(test_timeout_opt, test_timeout_descr, 10);

    server.add_init_callback([&server, &context]
    {
        const auto options = server.get_options();

        if (options->is_set(test_client_opt))
        {
            context.test_failed = true;

            auto const pid = fork();

            if (pid == 0)
            {
                auto const client = options->get<std::string>(test_client_opt).c_str();
                execl(client, client, static_cast<char const*>(nullptr));
                abort(); // If execl() returns then something is badly wrong
            }
            else if (pid > 0)
            {
                context.client_kill_action = server.the_main_loop()->create_alarm(
                    [pid]
                    {
                        kill(pid, SIGTERM);
                    });

                context.server_stop_action = server.the_main_loop()->create_alarm(
                    [pid, &server, &context]()
                    {
                        context.test_failed = !exit_success(pid);
                        server.stop();
                    });

                context.client_kill_action->reschedule_in(std::chrono::seconds(options->get<int>(test_timeout_opt)));
                context.server_stop_action->reschedule_in(std::chrono::seconds(options->get<int>(test_timeout_opt)+1));
            }
            else
            {
                BOOST_THROW_EXCEPTION(std::runtime_error("Client failed to launch"));
            }
        }
        else
        {
            context.test_failed = false;
        }
    });
}
