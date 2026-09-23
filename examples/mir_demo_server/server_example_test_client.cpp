/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "server_example_test_client.h"
#include <mir/errno_utils.h>
#include <mir/fd.h>
#include <mir/server.h>
#include <mir/main_loop.h>
#include <mir/logging/logger.h>
#define MIR_LOG_DEFAULT_TAGS { mir::logging::uncategorised() }
#include <mir/log.h>
#include <mir/options/option.h>

#include <chrono>
#include <format>
#include <future>

#include <sys/wait.h>

namespace me = mir::examples;
namespace ml = mir::logging;
using namespace std::chrono_literals;

namespace
{
struct Self
{
    std::unique_ptr<mir::time::Alarm> server_stop_action;
    std::atomic<bool> test_failed;
};

void check_exit_status(pid_t pid, mir::Server& server, Self* self, int& countdown)
{
    countdown--;
    int status;

    auto const wait_rc = waitpid(pid, &status, WNOHANG);

    if (wait_rc == 0)
    {
        if (countdown > 0)
        {
            self->server_stop_action->reschedule_in(100ms);
            return;
        }
        else
        {
            mir::log(ml::Severity::warning, {ml::uncategorised()}, "Client has not exited: Terminating client");
            kill(pid, SIGKILL);
            self->test_failed = true;
        }
    }
    else if (wait_rc != pid)
    {
        mir::log(ml::Severity::informational, {ml::uncategorised()}, "No status available for client");
        self->test_failed = true;
    }
    else if (WIFEXITED(status))
    {
        auto const exit_status = WEXITSTATUS(status);
        if (exit_status == EXIT_SUCCESS)
        {
            mir::log(ml::Severity::informational, {ml::uncategorised()}, "Client exited successfully");
            self->test_failed = false;
        }
        else
        {
            mir::log(ml::Severity::informational, {ml::uncategorised()}, "Client has exited with status {}", exit_status);
            self->test_failed = true;
        }
    }
    else if (WIFSIGNALED(status))
    {
        auto const signal = WTERMSIG(status);
        mir::log(ml::Severity::informational, {ml::uncategorised()}, "Client terminated by signal {}", signal);
        self->test_failed = true;
    }
    else
    {
        mir::log(ml::Severity::informational, {ml::uncategorised()}, "Client died mysteriously");
        self->test_failed = true;
    }

    server.stop();
}
}

struct me::TestClientRunner::Self : ::Self
{
    std::unique_ptr<time::Alarm> client_kill_action;
};

me::TestClientRunner::TestClientRunner() : self{std::make_shared<Self>()} {}

void me::TestClientRunner::operator()(mir::Server& server)
{
    static const char* const test_client_opt = "test-client";
    static const char* const test_client_descr = "Client executable";

    static const char* const test_timeout_opt = "test-timeout";
    static const char* const test_timeout_descr = "Seconds to run before sending SIGTERM to client";

    server.add_configuration_option(test_client_opt, test_client_descr, OptionType::string);
    server.add_configuration_option(test_timeout_opt, test_timeout_descr, 10);

    server.add_init_callback([&server, self = self]
    {
        const auto options1 = server.get_options();

        if (options1->is_set(test_client_opt))
        {
            self->test_failed = true;

            auto const pid = fork();

            if (pid == 0)
            {
                // Enable tests with toolkits supporting Wayland
                auto const wayland_display = server.wayland_display();
                setenv("WAYLAND_DISPLAY", wayland_display.value().c_str(),  true);   // configure Wayland socket
                setenv("GDK_BACKEND", "wayland", true);             // configure GTK to use Wayland
                setenv("QT_QPA_PLATFORM", "wayland", true);         // configure Qt to use Wayland
                unsetenv("QT_QPA_PLATFORMTHEME");                   // Discourage Qt from unsupported theme
                setenv("SDL_VIDEODRIVER", "wayland", true);         // configure SDL to use Wayland

                auto const client = options1->get<std::string>(test_client_opt);
                mir::log(logging::Severity::informational, {ml::uncategorised()},
                    "Starting test client: {}", client);
                execlp(client.c_str(), client.c_str(), static_cast<char const*>(nullptr));
                // If execl() returns then something is badly wrong
                mir::log(logging::Severity::critical, {ml::uncategorised()},
                    "Failed to execute client ({}) error: {}", client, mir::errno_to_cstr(errno));
                abort();
            }
            else if (pid > 0)
            {
                self->client_kill_action = server.the_main_loop()->create_alarm(
                    [pid, self]
                    {
                        kill(pid, SIGTERM);
                        self->server_stop_action->reschedule_in(100ms);
                    });

                self->server_stop_action = server.the_main_loop()->create_alarm(
                    [pid, &server, self, countdown = 200]() mutable
                    {
                        check_exit_status(pid, server, self.get(), countdown);
                    });

                self->client_kill_action->reschedule_in(std::chrono::seconds(options1->get<int>(test_timeout_opt)));
            }
            else
            {
                BOOST_THROW_EXCEPTION(std::runtime_error("Client failed to launch"));
            }
        }
        else
        {
            self->test_failed = false;
        }
    });
}

bool me::TestClientRunner::test_failed() const { return self->test_failed; }
