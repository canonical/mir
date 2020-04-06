/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/run_mir.h"
#include "mir/terminate_with_current_exception.h"
#include "mir/display_server.h"
#include "mir/fatal.h"
#include "mir/main_loop.h"
#include "mir/server_configuration.h"
#include "mir/frontend/connector.h"
#include "mir/raii.h"
#include "mir/emergency_cleanup.h"

#include <atomic>
#include <exception>
#include <mutex>
#include <csignal>
#include <cstdlib>
#include <cassert>
#include <sys/types.h>
#include <unistd.h>

namespace
{
auto const intercepted = { SIGQUIT, SIGABRT, SIGFPE, SIGSEGV, SIGBUS };

std::weak_ptr<mir::EmergencyCleanup> weak_emergency_cleanup;

extern "C" void perform_emergency_cleanup()
{
    if (auto emergency_cleanup = weak_emergency_cleanup.lock())
    {
        weak_emergency_cleanup.reset();
        (*emergency_cleanup)();
    }
}

extern "C" { typedef void (*sig_handler)(int); }

volatile sig_handler old_handler[NSIG]  = { nullptr };

extern "C" void fatal_signal_cleanup(int sig)
{
    perform_emergency_cleanup();

    signal(sig, old_handler[sig]);
    raise(sig);
}
}

void mir::run_mir(ServerConfiguration& config, std::function<void(DisplayServer&)> init)
{
    run_mir(config, init, {});
}

void mir::run_mir(
    ServerConfiguration& config,
    std::function<void(DisplayServer&)> init,
    std::function<void(int)> const& terminator)
{
    DisplayServer* server_ptr{nullptr};
    clear_termination_exception();

    auto const main_loop = config.the_main_loop();

    if (terminator)
    {
        main_loop->register_signal_handler({SIGINT, SIGTERM}, terminator);
    }
    else
    {
        main_loop->register_signal_handler(
            {SIGINT, SIGTERM},
            [&server_ptr](int)
            {
                assert(server_ptr);
                server_ptr->stop();
            });
    }

    FatalErrorStrategy fatal_error_strategy{config.the_fatal_error_strategy()};

    DisplayServer server(config);
    server_ptr = &server;

    weak_emergency_cleanup = config.the_emergency_cleanup();

    static std::atomic<unsigned int> concurrent_calls{0};

    if (!concurrent_calls)
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

    auto const raii = raii::paired_calls(
        [&]{ if (!concurrent_calls++) for (auto sig : intercepted) old_handler[sig] = signal(sig, fatal_signal_cleanup); },
        [&]{ if (!--concurrent_calls) for (auto sig : intercepted) signal(sig, old_handler[sig]); });

    init(server);
    server.run();

    check_for_termination_exception();
}
