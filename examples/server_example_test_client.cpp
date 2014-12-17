/*
 * Copyright © 2014 Canonical Ltd.
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

void me::add_test_client_option_to(mir::Server& server, std::atomic<bool>& test_failed)
{
    static auto const component = __PRETTY_FUNCTION__;
    static const char* const test_client_opt = "test-client";
    static const char* const test_client_descr = "client executable";

    static const char* const test_timeout_opt = "test-timeout";
    static const char* const test_timeout_descr = "Seconds to run before sending SIGTERM to client";

    server.add_configuration_option(test_client_opt, test_client_descr, mir::OptionType::string);
    server.add_configuration_option(test_timeout_opt, test_timeout_descr, 10);

    server.add_init_callback([&]
    {
        const auto options = server.get_options();
        if (options->is_set(test_client_opt))
        {
            auto const pid = fork();

            if (pid == 0)
            {
                auto const client = options->get<std::string>(test_client_opt).c_str();
                execl(client, client, static_cast<char const*>(nullptr));
                abort(); // If execl() returns then something is badly wrong
            }
            else if (pid > 0)
            {
                static auto const kill_action = server.the_main_loop()->notify_in(
                    std::chrono::seconds(options->get<int>(test_timeout_opt)),
                    [pid]{ kill(pid, SIGTERM); });

                static auto const exit_action = server.the_main_loop()->notify_in(
                    std::chrono::seconds(options->get<int>(test_timeout_opt)+1),
                    [pid, &server, &test_failed]
                    {
                        int status;

                        auto const wait_rc = waitpid(pid, &status, WNOHANG);

                        if (wait_rc == 0)
                        {
                            ml::log(ml::Severity::informational, "Terminating client", component);
                            kill(pid, SIGKILL);
                        }
                        else if (wait_rc == pid && WIFEXITED(status))
                        {
                            if (WEXITSTATUS(status) != EXIT_SUCCESS)
                            {
                                ml::log(ml::Severity::informational, "Client has exited with error", component);
                                test_failed = true;
                            }
                        }
                        else
                        {
                            ml::log(ml::Severity::informational, "Client has NOT exited", component);
                            test_failed = true;
                        }

                        server.stop();
                    });
            }
            else
            {
                ml::log(ml::Severity::informational, "Client failed to launch", component);
                test_failed = true;
            }
        }
    });
}
